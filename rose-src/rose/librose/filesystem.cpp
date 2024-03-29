/* $Id: filesystem.cpp 47496 2010-11-07 15:53:11Z silene $ */
/*
   Copyright (C) 2003 - 2010 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/**
 * @file
 * File-IO
 */
#define GETTEXT_DOMAIN "wesnoth-lib"

#include "global.hpp"

// Include files for opendir(3), readdir(3), etc.
// These files may vary from platform to platform,
// since these functions are NOT ANSI-conforming functions.
// They may have to be altered to port to new platforms

//for mkdir
#include <sys/stat.h>

#ifdef _WIN32
#include <shlobj.h>	// CSIDL_PROGRAM_FILES
#include "filesystem_win32.ii"
#include <cctype>
#else /* !_WIN32 */
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#endif /* !_WIN32 */

// for getenv
#include <cerrno>
#include <fstream>
#include <iomanip>
#include <set>
#include <boost/algorithm/string.hpp>

// for strerror
#include <cstring>

#include "config.hpp"
#include "filesystem.hpp"
#include "game_config.hpp"
#include "log.hpp"
#include "loadscreen.hpp"
#include "scoped_resource.hpp"
#include "gettext.hpp"
#include "formula_string_utils.hpp"
#include "posix.h"
#include "saes.hpp"
#include "wml_exception.hpp"

#include <boost/foreach.hpp>

static lg::log_domain log_filesystem("filesystem");
#define DBG_FS LOG_STREAM(debug, log_filesystem)
#define LOG_FS LOG_STREAM(info, log_filesystem)
#define WRN_FS LOG_STREAM(warn, log_filesystem)
#define ERR_FS LOG_STREAM(err, log_filesystem)

namespace {
	const mode_t AccessMode = 00770;

	// These are the filenames that get special processing
	const std::string maincfg_filename = "_main.cfg";
	const std::string finalcfg_filename = "_final.cfg";
	const std::string initialcfg_filename = "_initial.cfg";

}

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFBase.h>
#endif

bool ends_with(const std::string& str, const std::string& suffix)
{
	return str.size() >= suffix.size() && std::equal(suffix.begin(),suffix.end(),str.end()-suffix.size());
}

void get_files_in_dir(const std::string &directory,
					  std::vector<std::string>* files,
					  std::vector<std::string>* dirs,
					  file_name_option mode,
					  int filter,
					  file_reorder_option reorder,
					  file_tree_checksum* checksum)
{
	// If we have a path to find directories in,
	// then convert relative pathnames to be rooted
	// on the wesnoth path
	if (!directory.empty() && directory[0] != '/' && !game_config::path.empty()){
		std::string dir = game_config::path + "/" + directory;
		if (is_directory(dir)) {
			get_files_in_dir(dir,files,dirs,mode,filter,reorder,checksum);
			return;
		}
	}

	struct stat st;

	if (reorder == DO_REORDER) {
		LOG_FS << "searching for _main.cfg in directory " << directory << '\n';
		std::string maincfg;
		if (directory.empty() || directory[directory.size()-1] == '/') {
			maincfg = directory + maincfg_filename;
		} else {
			maincfg = (directory + "/") + maincfg_filename;
		}

		if (::stat(maincfg.c_str(), &st) != -1) {
			LOG_FS << "_main.cfg found : " << maincfg << '\n';
			if (files != NULL) {
				if (mode == ENTIRE_FILE_PATH)
					files->push_back(maincfg);
				else
					files->push_back(maincfg_filename);
			}
			return;
		}
	}

	DIR* dir = opendir(directory.c_str());

	if(dir == NULL) {
		return;
	}

	struct dirent* entry;
	while((entry = readdir(dir)) != NULL) {
		if(entry->d_name[0] == '.')
			continue;
#ifdef __APPLE__
		// HFS Mac OS X decomposes filenames using combining unicode characters.
		// Try to get the precomposed form.
		char macname[MAXNAMLEN+1];
		CFStringRef cstr = CFStringCreateWithCString(NULL,
							 entry->d_name,
							 kCFStringEncodingUTF8);
		CFMutableStringRef mut_str = CFStringCreateMutableCopy(NULL,
							 0, cstr);
		CFStringNormalize(mut_str, kCFStringNormalizationFormC);
		CFStringGetCString(mut_str,
				macname,sizeof(macname)-1,
				kCFStringEncodingUTF8);
		CFRelease(cstr);
		CFRelease(mut_str);
		const std::string basename = macname;
#else
		// generic Unix
		const std::string basename = entry->d_name;
#endif /* !APPLE */

		std::string fullname;
		if (directory.empty() || directory[directory.size()-1] == '/') {
			fullname = directory + basename;
		} else {
			fullname = directory + "/" + basename;
		}

		if (::stat(fullname.c_str(), &st) != -1) {
			if (S_ISREG(st.st_mode)) {
/*
				if (basename == "generate.cfg") {
					continue;
				}
*/
				if (files != NULL) {
					if (mode == ENTIRE_FILE_PATH)
						files->push_back(fullname);
					else
						files->push_back(basename);
				}
				if (checksum != NULL) {
					if(st.st_mtime > checksum->modified) {
						checksum->modified = st.st_mtime;
					}
					checksum->sum_size += st.st_size;
					checksum->nfiles++;
				}
			} else if (S_ISDIR(st.st_mode)) {
				if ((filter & SKIP_MEDIA_DIR) && (basename == "images"|| basename == "sounds" || basename == "music")) {
					continue;
				}
				if ((filter & SKIP_SCENARIO_DIR) && (basename == "scenarios"|| basename == "maps" || basename == "music")) {
					continue;
				}
				if ((filter & SKIP_GUI_DIR) && basename == "gui") {
					continue;
				}
				if ((filter & SKIP_INTERNAL_DIR) && basename == "units-internal") {
					continue;
				}
				if ((filter & SKIP_BOOK) && basename == "book") {
					continue;
				}

				if (reorder == DO_REORDER &&
						::stat((fullname+"/"+maincfg_filename).c_str(), &st)!=-1 &&
						S_ISREG(st.st_mode)) {
					LOG_FS << "_main.cfg found : ";
					if (files != NULL) {
						if (mode == ENTIRE_FILE_PATH) {
							files->push_back(fullname + "/" + maincfg_filename);
							LOG_FS << fullname << "/" << maincfg_filename << '\n';
						} else {
							files->push_back(basename + "/" + maincfg_filename);
							LOG_FS << basename << "/" << maincfg_filename << '\n';
					}
					} else {
					// Show what I consider strange
						LOG_FS << fullname << "/" << maincfg_filename << " not used now but skip the directory \n";
					}
				} else if (dirs != NULL) {
					if (mode == ENTIRE_FILE_PATH)
						dirs->push_back(fullname);
					else
						dirs->push_back(basename);
				}
			}
		}
	}

	closedir(dir);

	if(files != NULL)
		std::sort(files->begin(),files->end());

	if (dirs != NULL)
		std::sort(dirs->begin(),dirs->end());

	if (files != NULL && reorder == DO_REORDER) {
		// move finalcfg_filename, if present, to the end of the vector
		for (unsigned int i = 0; i < files->size(); i++) {
			if (ends_with((*files)[i], "/" + finalcfg_filename)) {
				files->push_back((*files)[i]);
				files->erase(files->begin()+i);
				break;
			}
		}
		// move initialcfg_filename, if present, to the beginning of the vector
		int foundit = -1;
		for (unsigned int i = 0; i < files->size(); i++)
			if (ends_with((*files)[i], "/" + initialcfg_filename)) {
				foundit = i;
				break;
			}
		if (foundit > 0) {
			std::string initialcfg = (*files)[foundit];
			for (unsigned int i = foundit; i > 0; i--)
				(*files)[i] = (*files)[i-1];
			(*files)[0] = initialcfg;
		}
	}
}

std::string get_prefs_file()
{
	return get_user_data_dir_utf8() + "/preferences";
}

std::string create_if_not(const std::string& dir_utf8) 
{
	std::string dir_ansi = dir_utf8;
#ifdef _WIN32
	conv_ansi_utf8(dir_ansi, false);
#endif
	if (get_dir(dir_ansi).empty()) {
		return "";
	}
	return dir_utf8;
}

std::string get_saves_dir()
{
	const std::string dir_utf8 = get_user_data_dir_utf8() + "/saves";
	return create_if_not(dir_utf8);
}

std::string get_addon_campaigns_dir()
{
	const std::string dir_utf8 = get_user_data_dir_utf8() + "/data/add-ons";
	return create_if_not(dir_utf8);
}

std::string get_intl_dir()
{
#ifdef _WIN32
	// return get_cwd() + "/translations";
	if (!game_config::path.empty()) {
		return game_config::path + "/translations";
	} else {
		return get_cwd() + "/translations";
	}
#else

	return game_config::path + "/translations";
#endif
}

std::string get_screenshot_dir()
{
	const std::string dir_utf8 = get_user_data_dir_utf8() + "/screenshots";
	return create_if_not(dir_utf8);
}

std::string get_next_filename(const std::string& name, const std::string& extension)
{
	std::string next_filename;
	int counter = 0;

	do {
		std::stringstream filename;

		filename << name;
		filename.width(3);
		filename.fill('0');
		filename.setf(std::ios_base::right);
		filename << counter << extension;
		counter++;
		next_filename = filename.str();
	} while(file_exists(next_filename) && counter < 1000);
	return next_filename;
}


std::string get_dir(const std::string& dir_path)
{
	DIR* dir = opendir(dir_path.c_str());
	if(dir == NULL) {
		const int res = mkdir(dir_path.c_str(),AccessMode);
		if(res == 0) {
			dir = opendir(dir_path.c_str());
		} else {
			ERR_FS << "could not open or create directory: " << dir_path << '\n';
		}
	}

	if(dir == NULL)
		return "";

	closedir(dir);

	return dir_path;
}

bool make_directory(const std::string& path)
{
	return (mkdir(path.c_str(),AccessMode) == 0);
}

// This deletes a directory with no hidden files and subdirectories.
// Also deletes a single file.
bool delete_directory(const std::string& path)
{
	bool ret = true;
	std::vector<std::string> files;
	std::vector<std::string> dirs;

	get_files_in_dir(path, &files, &dirs, ENTIRE_FILE_PATH);

	if(!files.empty()) {
		for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i) {
			errno = 0;
			if(remove((*i).c_str()) != 0) {
				LOG_FS << "remove(" << (*i) << "): " << strerror(errno) << "\n";
				ret = false;
			}
		}
	}

	if(!dirs.empty()) {
		for(std::vector<std::string>::const_iterator j = dirs.begin(); j != dirs.end(); ++j) {
			if(!delete_directory(*j))
				ret = false;
		}
	}

	errno = 0;
#ifdef _WIN32
	// remove() doesn't delete directories on windows.
	int (*remove)(const char*);
	if(is_directory(path))
		remove = rmdir;
	else
		remove = ::remove;
#endif
	if(remove(path.c_str()) != 0) {
		LOG_FS << "remove(" << path << "): " << strerror(errno) << "\n";
		ret = false;
	}
	return ret;
}

std::string get_cwd()
{
	char buf[1024];
	const char* const res = getcwd(buf,sizeof(buf));
	if(res != NULL) {
		std::string str(res);

#ifdef _WIN32
		std::replace(str.begin(),str.end(),'\\','/');
#endif

		return str;
	} else {
		return "";
	}
}

std::string get_exe_dir()
{
#ifndef _WIN32
	char buf[1024];
	size_t path_size = readlink("/proc/self/exe", buf, 1024);
	if(path_size == static_cast<size_t>(-1))
		return std::string();
	buf[path_size] = 0;
	return std::string(dirname(buf));
#else
	return get_cwd();
#endif
}

bool create_directory_if_missing(const std::string& dirname)
{
	if(is_directory(dirname)) {
		DBG_FS << "directory " << dirname << " exists, not creating\n";
		return true;
	} else if(file_exists(dirname)) {
		ERR_FS << "cannot create directory " << dirname << "; file exists\n";
		return false;
	}
	DBG_FS << "creating missing directory " << dirname << '\n';
	return make_directory(dirname);
}
bool create_directory_if_missing_recursive(const std::string& dirname)
{
	DBG_FS<<"creating recursive directory: "<<dirname<<'\n';
	if (is_directory(dirname) == false && dirname.empty() == false)
	{
		std::string tmp_dirname = dirname;
		// remove trailing slashes or backslashes
		while ((tmp_dirname[tmp_dirname.size()-1] == '/' ||
			  tmp_dirname[tmp_dirname.size()-1] == '\\') &&
			  tmp_dirname.size()>0)
		{
			tmp_dirname.erase(tmp_dirname.size()-1);
		}

		// create the first non-existing directory
		size_t pos = tmp_dirname.rfind("/");

		// we get the most right directory and *skip* it
		// we are creating it when we get back here
		if (tmp_dirname.rfind('\\') != std::string::npos &&
			tmp_dirname.rfind('\\') > pos )
			pos = tmp_dirname.rfind('\\');

		if (pos != std::string::npos)
			create_directory_if_missing_recursive(tmp_dirname.substr(0,pos));

		return create_directory_if_missing(tmp_dirname);
	}
	return create_directory_if_missing(dirname);
}

static std::string user_data_dir, user_data_dir_utf8, user_config_dir, cache_dir;

static void setup_user_data_dir();

void set_preferences_dir(std::string path)
{
#ifdef _WIN32
	if(path.empty()) {
		game_config::preferences_dir = get_cwd() + "/userdata";
	} else if (path.size() > 2 && path[1] == ':') {
		//allow absolute path override
		game_config::preferences_dir = path;
	} else {
		char my_documents_path[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, my_documents_path))) {
            std::string mygames_path = std::string(my_documents_path) + "/" + "My Games";
			boost::algorithm::replace_all(mygames_path, std::string("\\"), std::string("/"));
			create_directory_if_missing(mygames_path);
			game_config::preferences_dir = mygames_path + "/" + path;

			// unicode to utf8
			WCHAR wc[MAX_PATH];
			SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, wc);
			WideCharToMultiByte(CP_UTF8, 0, wc, -1, my_documents_path, MAX_PATH, NULL, NULL);
			mygames_path = std::string(my_documents_path) + "/" + "My Games";
			boost::algorithm::replace_all(mygames_path, std::string("\\"), std::string("/"));
			game_config::preferences_dir_utf8 = mygames_path + "/" + path;
		} else {
			game_config::preferences_dir = get_cwd() + "/" + path;
		}
	}
	// conv_ansi_utf8(game_config::preferences_dir, true);

#elif defined(ANDROID)
	game_config::preferences_dir = game_config::path + std::string("/") + path;
	// non-win32, assume no tow-code character.
	game_config::preferences_dir_utf8 = game_config::preferences_dir;
#elif defined(__APPLE__) && !TARGET_OS_IPHONE
	game_config::preferences_dir = get_cwd() + std::string("/../") + path;
	// non-win32, assume no tow-code character.
	game_config::preferences_dir_utf8 = game_config::preferences_dir;
#else
#ifdef PREFERENCES_DIR
	if (path.empty()) path = PREFERENCES_DIR;
#endif

	std::string path2 = ".wesnoth" + game_config::version.substr(0,3);

#ifdef _X11
	const char *home_str = getenv("HOME");

	if (path.empty()) {
		char const *xdg_data = getenv("XDG_DATA_HOME");
		if (!xdg_data || xdg_data[0] == '\0') {
			if (!home_str) {
				path = path2;
				goto other;
			}
			user_data_dir = home_str;
			user_data_dir += "/.local/share";
		} else user_data_dir = xdg_data;
		user_data_dir += "/wesnoth/";
		user_data_dir += game_config::version.substr(0,3);
		create_directory_if_missing_recursive(user_data_dir);
		game_config::preferences_dir = user_data_dir;
	} else {
		other:
		std::string home = home_str ? home_str : ".";

		if (path[0] == '/')
			game_config::preferences_dir = path;
		else
			game_config::preferences_dir = home + "/" + path;
	}
#else
	if (path.empty()) path = path2;

	const char* home_str = getenv("HOME");
	std::string home = home_str ? home_str : ".";

	if (path[0] == '/')
		game_config::preferences_dir = path;
	else
		game_config::preferences_dir = home + std::string("/") + path;
#endif
	// non-win32, assume no tow-code character.
	game_config::preferences_dir_utf8 = game_config::preferences_dir;
#endif /*_WIN32*/
	user_data_dir = game_config::preferences_dir;
	posix_print("set_preferences, user_data_dir: %s", user_data_dir.c_str());

	user_data_dir_utf8 = game_config::preferences_dir_utf8;
	setup_user_data_dir();
}

static void setup_user_data_dir()
{
#ifdef _WIN32
	_mkdir(user_data_dir.c_str());
	_mkdir((user_data_dir + "/editor").c_str());
	_mkdir((user_data_dir + "/editor/maps").c_str());
	_mkdir((user_data_dir + "/data").c_str());
	_mkdir((user_data_dir + "/data/add-ons").c_str());
	_mkdir((user_data_dir + "/saves").c_str());
	_mkdir((user_data_dir + "/persist").c_str());

#else
	const std::string& dir_path = user_data_dir;

	const bool res = create_directory_if_missing(dir_path);
	// probe read permissions (if we could make the directory)
	DIR* const dir = res ? opendir(dir_path.c_str()) : NULL;
	if(dir == NULL) {
		ERR_FS << "could not open or create preferences directory at " << dir_path << '\n';
		return;
	}
	closedir(dir);

	// Create user data and add-on directories
	create_directory_if_missing(dir_path + "/editor");
	create_directory_if_missing(dir_path + "/editor/maps");
	create_directory_if_missing(dir_path + "/data");
	create_directory_if_missing(dir_path + "/data/add-ons");
	create_directory_if_missing(dir_path + "/saves");
	create_directory_if_missing(dir_path + "/persist");
#endif
}

const std::string& get_user_data_dir()
{
	// ensure setup gets called only once per session
	// FIXME: this is okay and optimized, but how should we react
	// if the user deletes a dir while we are running?
	if (user_data_dir.empty())
	{
		if (game_config::preferences_dir.empty())
			set_preferences_dir(std::string());
		else {
			user_data_dir = game_config::preferences_dir;
			setup_user_data_dir();
		}
	}
	return user_data_dir;
}

const std::string& get_user_data_dir_utf8()
{
	// ensure setup gets called only once per session
	// FIXME: this is okay and optimized, but how should we react
	// if the user deletes a dir while we are running?
	if (user_data_dir_utf8.empty())
	{
		if (game_config::preferences_dir_utf8.empty())
			set_preferences_dir(std::string());
		else {
			user_data_dir_utf8 = game_config::preferences_dir;
			setup_user_data_dir();
		}
	}
	return user_data_dir_utf8;
}

const std::string &get_user_config_dir()
{
	if (user_config_dir.empty())
	{
#if defined(_X11) && !defined(PREFERENCES_DIR)
		char const *xdg_config = getenv("XDG_CONFIG_HOME");
		if (!xdg_config || xdg_config[0] == '\0') {
			xdg_config = getenv("HOME");
			if (!xdg_config) {
				user_config_dir = get_user_data_dir();
				return user_config_dir;
			}
			user_config_dir = xdg_config;
			user_config_dir += "/.config";
		} else user_config_dir = xdg_config;
		user_config_dir += "/wesnoth";
		create_directory_if_missing_recursive(user_config_dir);
#else
		user_config_dir = get_user_data_dir();
#endif
	}
	return user_config_dir;
}

const std::string &get_cache_dir()
{
	if (cache_dir.empty())
	{
#if defined(_X11) && !defined(PREFERENCES_DIR)
		char const *xdg_cache = getenv("XDG_CACHE_HOME");
		if (!xdg_cache || xdg_cache[0] == '\0') {
			xdg_cache = getenv("HOME");
			if (!xdg_cache) {
				cache_dir = get_dir(get_user_data_dir() + "/cache");
				return cache_dir;
			}
			cache_dir = xdg_cache;
			cache_dir += "/.cache";
		} else cache_dir = xdg_cache;
		cache_dir += "/wesnoth";
		create_directory_if_missing_recursive(cache_dir);
#else
		cache_dir = get_dir(get_user_data_dir() + "/cache");
#endif
	}
	return cache_dir;
}

static std::string read_stream(std::istream& s)
{
	std::stringstream ss;
	ss << s.rdbuf();
	return ss.str();
}

std::istream *istream_file(const std::string &fname, bool to_utf16)
{
	LOG_FS << "Streaming " << fname << " for reading.\n";
	if (fname.empty())
	{
		ERR_FS << "Trying to open file with empty name.\n";
		std::ifstream *s = new std::ifstream();
		s->clear(std::ios_base::failbit);
		return s;
	}
	std::ifstream *s = NULL;
#ifndef _WIN32
	// TODO: Should also be done for Windows; but *nix systems should
	// already be sufficient to catch most offenders.
	if (fname[0] != '/') {
		WRN_FS << "Trying to open file with relative path: '" << fname << "'.\n";
#if 0
		std::ifstream *s = new std::ifstream();
		s->clear(std::ios_base::failbit);
		return s;
#endif
	}
	s = new std::ifstream(fname.c_str(),std::ios_base::binary);
#else
	if (to_utf16) {
		// utf8 ---> utf16
		int wlen = MultiByteToWideChar(CP_UTF8, 0, fname.c_str(), -1, NULL, 0);
		WCHAR *wc = new WCHAR[wlen];
		MultiByteToWideChar(CP_UTF8, 0, fname.c_str(), -1, wc, wlen);
			
		s = new std::ifstream(wc, std::ios_base::binary);
		delete [] wc;
	} else {
		s = new std::ifstream(fname.c_str(), std::ios_base::binary);
	}
#endif
	if (s->is_open())
		return s;
	ERR_FS << "Could not open '" << fname << "' for reading.\n";
	return s;

}

std::string read_file(const std::string &fname, bool to_utf16)
{
	scoped_istream s = istream_file(fname, to_utf16);
	return read_stream(*s);
}

std::ostream *ostream_file(std::string const &fname, bool to_utf16)
{
	LOG_FS << "streaming " << fname << " for writing.\n";
#ifdef _WIN32
	// utf8 ---> utf16
	if (to_utf16) {
		int wlen = MultiByteToWideChar(CP_UTF8, 0, fname.c_str(), -1, NULL, 0);
		WCHAR *wc = new WCHAR[wlen];
		MultiByteToWideChar(CP_UTF8, 0, fname.c_str(), -1, wc, wlen);
			
		std::ofstream* s = new std::ofstream(wc, std::ios_base::binary);
		delete [] wc;
		return s;
	}
#endif
	return new std::ofstream(fname.c_str(), std::ios_base::binary);
}

// Throws io_exception if an error occurs
void write_file(const std::string& fname, const std::string& data)
{
	//const util::scoped_resource<FILE*,close_FILE> file(fopen(fname.c_str(),"wb"));
	const util::scoped_FILE file(fopen(fname.c_str(),"wb"));
	if(file.get() == NULL) {
		throw io_exception("Could not open file for writing: '" + fname + "'");
	}

	const size_t block_size = 4096;
	char buf[block_size];

	for(size_t i = 0; i < data.size(); i += block_size) {
		const size_t bytes = std::min<size_t>(block_size,data.size() - i);
		std::copy(data.begin() + i, data.begin() + i + bytes,buf);
		const size_t res = fwrite(buf,1,bytes,file.get());
		if(res != bytes) {
			throw io_exception("Error writing to file: '" + fname + "'");
		}
	}
}

void write_file(const std::string& fname, const char* data, int len, bool to_utf16)
{
	posix_file_t fp;
	uint32_t bytertd;

#ifdef _WIN32
	if (to_utf16) {
		int wlen = MultiByteToWideChar(CP_UTF8, 0, fname.c_str(), -1, NULL, 0);
		WCHAR *wc = new WCHAR[wlen];
		MultiByteToWideChar(CP_UTF8, 0, fname.c_str(), -1, wc, wlen);
		
		fp = CreateFileW(wc, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			CREATE_ALWAYS, 0, NULL);

		delete [] wc;
	} else {
		posix_fopen(fname.c_str(), GENERIC_WRITE, CREATE_ALWAYS, fp);
	}
#else
	posix_fopen(fname.c_str(), GENERIC_WRITE, CREATE_ALWAYS, fp);
#endif

	if (fp == INVALID_FILE) {
		return;
	}
	posix_fwrite(fp, data, len, bytertd);
	posix_fclose(fp);
}

std::string read_map(const std::string& name)
{
	std::string res;
	std::string map_location = get_wml_location("maps/" + name);
	if(!map_location.empty()) {
		res = read_file(map_location);
	}

	if (res.empty()) {
		res = read_file(get_user_data_dir() + "/editor/maps/" + name);
	}

	return res;
}

static bool is_directory_internal(const std::string& fname)
{
#ifdef _WIN32
	_finddata_t info;
	const long handle = _findfirst((fname + "/*").c_str(),&info);
	if(handle >= 0) {
		_findclose(handle);
		return true;
	} else {
		return false;
	}

#else
	struct stat dir_stat;
	if(::stat(fname.c_str(), &dir_stat) == -1) {
		return false;
	}
	return S_ISDIR(dir_stat.st_mode);
#endif
}

bool is_directory(const std::string& fname)
{
	if(fname.empty()) {
		return false;
	}
	if(fname[0] != '/' && !game_config::path.empty()) {
		if(is_directory_internal(game_config::path + "/" + fname))
			return true;
	}

	return is_directory_internal(fname);
}

bool file_exists(const std::string& name)
{
#ifdef _WIN32
       struct stat st;
       return (::stat(name.c_str(), &st) == 0);
#else
	struct stat st;
	return (::stat(name.c_str(), &st) != -1);
#endif
}

void file_remove(const std::string& name)
{
	// name require utf8 code.
	std::string name2 = name;
#ifdef _WIN32
	conv_ansi_utf8(name2, false);
#endif
	remove(name2.c_str());
}

void file_rename(const std::string& from, const std::string& to)
{
	// from and to require utf8 code.
	std::string from2 = from;
	std::string to2 = to;
#ifdef _WIN32
	conv_ansi_utf8(from2, false);
	conv_ansi_utf8(to2, false);
#endif

	rename(from2.c_str(), to2.c_str());
}

time_t file_create_time(const std::string& fname)
{
	struct stat buf;
	if(::stat(fname.c_str(),&buf) == -1)
		return 0;

	return buf.st_mtime;
}

/**
 * Returns true if the file ends with '.gz'.
 *
 * @param filename                The name to test.
 */
bool is_gzip_file(const std::string& filename)
{
	return (filename.length() > 3
		&& filename.substr(filename.length() - 3) == ".gz");
}

file_tree_checksum::file_tree_checksum()
	: nfiles(0), sum_size(0), modified(0)
{}

file_tree_checksum::file_tree_checksum(const config& cfg) :
	nfiles	(lexical_cast_default<size_t>(cfg["nfiles"])),
	sum_size(lexical_cast_default<size_t>(cfg["size"])),
	modified(lexical_cast_default<time_t>(cfg["modified"]))
{
}

void file_tree_checksum::write(config& cfg) const
{
	cfg["nfiles"] = lexical_cast<std::string>(nfiles);
	cfg["size"] = lexical_cast<std::string>(sum_size);
	cfg["modified"] = lexical_cast<std::string>(modified);
}

bool file_tree_checksum::operator==(const file_tree_checksum &rhs) const
{
	return nfiles == rhs.nfiles && sum_size == rhs.sum_size &&
		modified == rhs.modified;
}

static void get_file_tree_checksum_internal(const std::string& path, file_tree_checksum& res, int filter)
{

	std::vector<std::string> dirs;
	get_files_in_dir(path,NULL,&dirs, ENTIRE_FILE_PATH, filter, DONT_REORDER, &res);
	loadscreen::increment_progress();

	for(std::vector<std::string>::const_iterator j = dirs.begin(); j != dirs.end(); ++j) {
		get_file_tree_checksum_internal(*j, res, filter);
	}
}

const file_tree_checksum& data_tree_checksum(bool reset, int filter)
{
	static file_tree_checksum checksum;
	if (reset)
		checksum.reset();
	if(checksum.nfiles == 0) {
		get_file_tree_checksum_internal("data/", checksum, filter);
		get_file_tree_checksum_internal(get_user_data_dir() + "/data/", checksum, filter);
		LOG_FS << "calculated data tree checksum: "
			   << checksum.nfiles << " files; "
			   << checksum.sum_size << " bytes\n";
	}

	return checksum;
}

void data_tree_checksum(const std::vector<std::string>& paths, file_tree_checksum& checksum, int filter)
{
	checksum.reset();
	for (std::vector<std::string>::const_iterator it = paths.begin(); it != paths.end(); ++ it) {
		get_file_tree_checksum_internal(*it, checksum, filter);
	}
}

int file_size(const std::string& fname)
{
	struct stat buf;
	if(::stat(fname.c_str(),&buf) == -1)
		return -1;

	return buf.st_size;
}

std::string file_name(const std::string& file)
// Analogous to POSIX basename(3), but for C++ string-object pathnames
{
#ifdef _WIN32
	static const std::string dir_separators = "\\/:";
#else
	static const std::string dir_separators = "/";
#endif

	std::string::size_type pos = file.find_last_of(dir_separators);

	if(pos == std::string::npos)
		return file;
	if(pos >= file.size()-1)
		return "";

	return file.substr(pos+1);
}

std::string directory_name(const std::string& file)
// Analogous to POSIX dirname(3), but for C++ string-object pathnames
{
#ifdef _WIN32
	static const std::string dir_separators = "\\/:";
#else
	static const std::string dir_separators = "/";
#endif

	std::string::size_type pos = file.find_last_of(dir_separators);

	if(pos == std::string::npos)
		return "";

	return file.substr(0,pos+1);
}

namespace {

#define PRIORITIEST_BINARY_PATHS	1
std::vector<std::string> binary_paths;

typedef std::map<std::string,std::vector<std::string> > paths_map;
paths_map binary_paths_cache;

}

static void init_binary_paths()
{
	if (binary_paths.empty()) {
		binary_paths.push_back(""); // userdata directory
		// here is self-add paths
		binary_paths.push_back(game_config::app_dir + "/");
		binary_paths.push_back("data/core/");
	}
}

binary_paths_manager::binary_paths_manager() : paths_()
{}

binary_paths_manager::binary_paths_manager(const config& cfg) : paths_()
{
	set_paths(cfg);
}

binary_paths_manager::~binary_paths_manager()
{
	cleanup();
}

void binary_paths_manager::set_paths(const config& cfg)
{
	cleanup();
	init_binary_paths();

	int inserted = 0;
	BOOST_FOREACH (const config &bp, cfg.child_range("binary_path")) {
		std::string path = bp["path"].str();
		if (path.find("..") != std::string::npos) {
			ERR_FS << "Invalid binary path '" << path << "'\n";
			continue;
		}
		if (!path.empty() && path[path.size()-1] != '/') {
			path += "/";
		}
		std::vector<std::string>::iterator it = std::find(binary_paths.begin(), binary_paths.end(), path);
		if (it == binary_paths.end()) {
			it = binary_paths.begin();
			std::advance(it, PRIORITIEST_BINARY_PATHS + inserted);
			binary_paths.insert(it, path);

			VALIDATE(std::find(paths_.begin(), paths_.end(), path) == paths_.end(), null_str);

			paths_.push_back(path);
			inserted ++;
		}
	}
}

void binary_paths_manager::cleanup()
{
	binary_paths_cache.clear();

	for (std::vector<std::string>::const_iterator i = paths_.begin(); i != paths_.end(); ++i) {
		std::vector<std::string>::iterator it2 = std::find(binary_paths.begin(), binary_paths.end(), *i);
		binary_paths.erase(it2);
	}
	paths_.clear();
}

void clear_binary_paths_cache()
{
	binary_paths_cache.clear();
}

const std::vector<std::string>& get_binary_paths(const std::string& type)
{
	const paths_map::const_iterator itor = binary_paths_cache.find(type);
	if(itor != binary_paths_cache.end()) {
		return itor->second;
	}

	if (type.find("..") != std::string::npos) {
		// Not an assertion, as language.cpp is passing user data as type.
		ERR_FS << "Invalid WML type '" << type << "' for binary paths\n";
		static std::vector<std::string> dummy;
		return dummy;
	}

	std::vector<std::string>& res = binary_paths_cache[type];

	init_binary_paths();

	BOOST_FOREACH (const std::string &path, binary_paths)
	{
		if (path.empty()) {
			res.push_back(get_user_data_dir() + "/");
			res.push_back(get_user_data_dir() + "/" + type + "/");
			res.push_back(game_config::path + "/");
		} else {
			res.push_back(game_config::path + "/" + path + type + "/");
		}
	}
	return res;
}

std::string get_binary_file_location(const std::string& type, const std::string& filename)
{
	DBG_FS << "Looking for '" << filename << "'.\n";

	if (filename.empty()) {
		LOG_FS << "  invalid filename (type: " << type <<")\n";
		return std::string();
	}

	// Some parts of Wesnoth enjoy putting ".." inside filenames. This is
	// bad and should be fixed. But in the meantime, deal with them in a dumb way.
	std::string::size_type pos = filename.rfind("../");
	if (pos != std::string::npos) {
		std::string nf = filename.substr(pos + 3);
		LOG_FS << "Illegal path '" << filename << "' replaced by '" << nf << "'\n";
		return get_binary_file_location(type, nf);
	}

	if (filename.find("..") != std::string::npos) {
		ERR_FS << "Illegal path '" << filename << "' (\"..\" not allowed).\n";
		return std::string();
	}

	BOOST_FOREACH (const std::string &path, get_binary_paths(type))
	{
		const std::string file = path + filename;
		DBG_FS << "  checking '" << path << "'\n";
		if(file_exists(file)) {
			DBG_FS << "  found at '" << file << "'\n";
			return file;
		}
	}

	DBG_FS << "  not found\n";
	return std::string();
}

std::string get_binary_dir_location(const std::string &type, const std::string &filename)
{
	DBG_FS << "Looking for '" << filename << "'.\n";

	if (filename.empty()) {
		LOG_FS << "  invalid filename (type: " << type <<")\n";
		return std::string();
	}

	if (filename.find("..") != std::string::npos) {
		ERR_FS << "Illegal path '" << filename << "' (\"..\" not allowed).\n";
		return std::string();
	}

	BOOST_FOREACH (const std::string &path, get_binary_paths(type))
	{
		const std::string file = path + filename;
		DBG_FS << "  checking '" << path << "'\n";
		if (is_directory(file)) {
			DBG_FS << "  found at '" << file << "'\n";
			return file;
		}
	}

	DBG_FS << "  not found\n";
	return std::string();
}

std::string get_wml_location(const std::string &filename, const std::string &current_dir)
{
	DBG_FS << "Looking for '" << filename << "'.\n";

	std::string result;

	if (filename.empty()) {
		LOG_FS << "  invalid filename\n";
		return result;
	}

	if (filename.find("..") != std::string::npos) {
		ERR_FS << "Illegal path '" << filename << "' (\"..\" not allowed).\n";
		return result;
	}

	bool already_found = false;

	if (filename[0] == '~') 
	{
		// If the filename starts with '~', look in the user data directory.
		result = get_user_data_dir() + "/data/" + filename.substr(1);
		DBG_FS << "  trying '" << result << "'\n";

		already_found = file_exists(result) || is_directory(result);
	}
	else if (filename.size() >= 2 && filename[0] == '.' && filename[1] == '/')
	{
		// If the filename begins with a "./", look in the same directory
		// as the file currrently being preprocessed.
		result = current_dir + filename.substr(2);
	}
	else if (filename[0] == '^') {
		// If the filename starts with '^', look in the topest directory.
		result = game_config::path + "/" + filename.substr(1);
	}
	else if (!game_config::path.empty())
		result = game_config::path + "/data/" + filename;

	DBG_FS << "  trying '" << result << "'\n";

	if (result.empty() ||
	    (!already_found && !file_exists(result) && !is_directory(result)))
	{
		DBG_FS << "  not found\n";
		result.clear();
	}
	else
		DBG_FS << "  found: '" << result << "'\n";

	return result;
}

std::string get_short_wml_path(const std::string &filename)
{
	std::string match = get_user_data_dir() + "/data/";
	if (filename.find(match) == 0) {
		return "~" + filename.substr(match.size());
	}
	match = game_config::path + "/data/";
	if (filename.find(match) == 0) {
		return filename.substr(match.size());
	}
	return filename;
}

std::string get_program_invocation(const std::string& program_name) {
#ifdef DEBUG
  #ifdef _WIN32
	const char *program_suffix = "-debug.exe";
  #else
	const char *program_suffix = "-debug";
  #endif
#else
  #ifdef _WIN32
	const char *program_suffix = ".exe";
  #else
	const char *program_suffix = "";
  #endif
#endif

	const std::string real_program_name(program_name + program_suffix);
	if(game_config::wesnoth_program_dir.empty()) return real_program_name;
#ifdef _WIN32
	return game_config::wesnoth_program_dir + "\\" + real_program_name;
#else
	return game_config::wesnoth_program_dir + "/" + real_program_name;
#endif
}

static bool is_path_sep(char c)
{
#ifdef _WIN32
	if (c == '/' || c == '\\') return true;
#else
	if (c == '/') return true;
#endif
	return false;
}

std::string normalize_path(const std::string &p1)
{
	if (p1.empty()) return p1;

	std::string p2;
#ifdef _WIN32
	if (p1.size() >= 2 && p1[1] == ':')
		// Windows relative paths with explicit drive name are not handled.
		p2 = p1;
	else
#endif
	if (!is_path_sep(p1[0]))
		p2 = get_cwd() + "/" + p1;
	else
		p2 = p1;

#ifdef _WIN32
	std::string drive;
	if (p2.size() >= 2 && p2[1] == ':') {
		drive = p2.substr(0, 2);
		p2.erase(0, 2);
	}
#endif

	std::vector<std::string> components(1);
	for (int i = 0, i_end = p2.size(); i <= i_end; ++i)
	{
		std::string &last = components[components.size() - 1];
		char c = p2.c_str()[i];
		if (is_path_sep(c) || c == 0)
		{
			if (last == ".")
				last.clear();
			else if (last == "..")
			{
				if (components.size() >= 2) {
					components.pop_back();
					components[components.size() - 1].clear();
				} else
					last.clear();
			}
			else if (!last.empty())
				components.push_back(std::string());
		}
		else
			last += c;
	}

	std::ostringstream p4;
	components.pop_back();

#ifdef _WIN32
	p4 << drive;
#endif

	BOOST_FOREACH (const std::string &s, components)
	{
		p4 << '/' << s;
	}

	DBG_FS << "Normalizing '" << p2 << "' to '" << p4.str() << "'\n";

	return p4.str();
}

scoped_istream& scoped_istream::operator=(std::istream *s)
{
	delete stream;
	stream = s;
	return *this;
}

scoped_istream::~scoped_istream()
{
	delete stream;
}

scoped_ostream& scoped_ostream::operator=(std::ostream *s)
{
	delete stream;
	stream = s;
	return *this;
}

scoped_ostream::~scoped_ostream()
{
	delete stream;
}

#ifdef _WIN32
/**
 * conv_ansi_utf8()
 *   - Convert a string between ANSI encoding (for Windows filename) and UTF-8
 *  string &name
 *     - filename to be converted
 *  bool a2u
 *     - if true, convert the string from ANSI to UTF-8.
 *     - if false, reverse. (convert it from UTF-8 to ANSI)
 */
void conv_ansi_utf8(std::string &name, bool a2u) 
{
	int wlen = MultiByteToWideChar(a2u ? CP_ACP : CP_UTF8, 0,
								   name.c_str(), -1, NULL, 0);
	if (wlen == 0) return;
	WCHAR *wc = new WCHAR[wlen];
	if (wc == NULL) return;
	if (MultiByteToWideChar(a2u ? CP_ACP : CP_UTF8, 0, name.c_str(), -1,
							wc, wlen) == 0) {
		delete [] wc;
		return;
	}
	int alen = WideCharToMultiByte(!a2u ? CP_ACP : CP_UTF8, 0, wc, wlen,
								   NULL, 0, NULL, NULL);
	if (alen == 0) {
		delete [] wc;
		return;
	}
	CHAR *ac = new CHAR[alen];
	if (ac == NULL) {
		delete [] wc;
		return;
	}
	WideCharToMultiByte(!a2u ? CP_ACP : CP_UTF8, 0, wc, wlen,
						ac, alen, NULL, NULL);
	delete [] wc;
	if (ac == NULL) {
		return;
	}
	name = ac;
	delete [] ac;

	return;
}

std::string conv_ansi_utf8_2(const std::string &name, bool a2u) 
{
	int wlen = MultiByteToWideChar(a2u ? CP_ACP : CP_UTF8, 0,
								   name.c_str(), -1, NULL, 0);
	if (wlen == 0) return "";
	WCHAR *wc = new WCHAR[wlen];
	if (wc == NULL) return "";
	if (MultiByteToWideChar(a2u ? CP_ACP : CP_UTF8, 0, name.c_str(), -1,
							wc, wlen) == 0) {
		delete [] wc;
		return "";
	}
	int alen = WideCharToMultiByte(!a2u ? CP_ACP : CP_UTF8, 0, wc, wlen,
								   NULL, 0, NULL, NULL);
	if (alen == 0) {
		delete [] wc;
		return "";
	}
	CHAR *ac = new CHAR[alen];
	if (ac == NULL) {
		delete [] wc;
		return "";
	}
	WideCharToMultiByte(!a2u ? CP_ACP : CP_UTF8, 0, wc, wlen,
						ac, alen, NULL, NULL);
	delete [] wc;
	if (ac == NULL) {
		return "";
	}
	std::string result = ac;
	delete [] ac;

	return result;
}

const char* utf8_2_ansi(const std::string& str)
{
	static std::string ret;
	ret = conv_ansi_utf8_2(str, false);
	return ret.c_str();
}

const char* ansi_2_utf8(const std::string& str)
{
	static std::string ret;
	ret = conv_ansi_utf8_2(str, true);
	return ret.c_str();
}

#else

void conv_ansi_utf8(std::string &name, bool a2u)
{
}

std::string conv_ansi_utf8_2(const std::string &name, bool a2u)
{
	return name;
}

const char* utf8_2_ansi(const std::string& str)
{
	static std::string ret = str;
	return ret.c_str();
}

const char* ansi_2_utf8(const std::string& str)
{
	static const std::string ret = str;
	return ret.c_str();
}

#endif

std::string format_time_ymd(time_t t)
{
	char time_buf[256] = {0};
	tm* tm_l = localtime(&t);
	if (tm_l) {
		const size_t res = strftime(time_buf, sizeof(time_buf), _("%b %d %y"), tm_l);
		if (res == 0) {
			time_buf[0] = 0;
		}
	}

	return time_buf;
}

std::string format_time_hms(time_t t)
{
	char time_buf[256] = {0};
	tm* tm_l = localtime(&t);
	if (tm_l) {
		const size_t res = strftime(time_buf, sizeof(time_buf), _("%H:%M:%S"), tm_l);
		if (res == 0) {
			time_buf[0] = 0;
		}
	}

	return time_buf;
}

std::string format_time_hm(time_t t)
{
	char time_buf[256] = {0};
	tm* tm_l = localtime(&t);
	if (tm_l) {
		const size_t res = strftime(time_buf, sizeof(time_buf), _("%H:%M"), tm_l);
		if (res == 0) {
			time_buf[0] = 0;
		}
	}

	return time_buf;
}

std::string format_time_date(time_t t)
{
	time_t curtime = time(NULL);
	const struct tm* timeptr = localtime(&curtime);
	if(timeptr == NULL) {
		return "";
	}

	const struct tm current_time = *timeptr;

	timeptr = localtime(&t);
	if(timeptr == NULL) {
		return "";
	}

	const struct tm save_time = *timeptr;

	const char* format_string = _("%b %d %y");

	if (current_time.tm_year == save_time.tm_year) {
		const int days_apart = current_time.tm_yday - save_time.tm_yday;

		if (days_apart == 0) {
			// save is from today
			format_string = _("%H:%M:%S");
		} else if(days_apart > 0 && days_apart <= current_time.tm_wday) {
			// save is from this week. On chinese, %A cannot display. use %m%d instead.
			format_string = _("%A, %H:%M");
		} else {
			// save is from current year
			// format_string = _("%b %d");
			format_string = _("%A, %H:%M");
		}
	} else {
		// save is from a different year
		format_string = _("%b %d %y");
	}

	char buf[64];
	const size_t res = strftime(buf, sizeof(buf), format_string, &save_time);
	if(res == 0) {
		buf[0] = 0;
	}
	
	return buf;
}

std::string format_time_local(time_t t)
{
	char time_buf[256] = {0};
	tm* tm_l = localtime(&t);
	if (tm_l) {
		const size_t res = strftime(time_buf,sizeof(time_buf),_("%a %b %d %H:%M %Y"),tm_l);
		if(res == 0) {
			time_buf[0] = 0;
		}
	}

	return time_buf;
}

std::string format_elapse(time_t elapse, bool align)
{
	if (elapse < 0) {
		elapse = 0;
	}
	int sec = elapse % 60;
	int min = (elapse / 60) % 60;
	int hour = (elapse / 3600) % 24;
	int day = elapse / (3600 * 24);

	std::stringstream strstr;
	if (day) {
		utils::string_map symbols;
		symbols["d"] = str_cast(day);
		strstr << vgettext("$d days", symbols) << " ";
	}
	strstr << std::setfill('0') << std::setw(2) << hour << ":";
	strstr << std::setfill('0') << std::setw(2) << min << ":";
	strstr << std::setfill('0') << std::setw(2) << sec;
	
	return strstr.str();
}

std::string format_elapse_hm(time_t elapse, bool align)
{
	if (elapse < 0) {
		elapse = 0;
	}
	int sec = elapse % 60;
	int min = (elapse / 60) % 60;
	int hour = (elapse / 3600) % 24;
	int day = elapse / (3600 * 24);

	std::stringstream ss;
	if (align) {
		ss << std::setfill('0') << std::setw(2);
	}
	ss << hour << "h";
	ss << std::setfill('0') << std::setw(2) << min << "m";
	
	return ss.str();
}

//
// encrypt/decrypt
//
// if return isn't NULL, caller need free heap.
char* saes_encrypt_heap(const char* ptext, int size, unsigned char* key)
{
	if (size == 0) {
		return NULL;
	}
	char* ctext = (char*)malloc((size & 1)? size + 1: size);
	saes_encrypt_stream((const unsigned char*)ptext, size, key, ctext);
	return ctext;
}

// if return isn't NULL, caller need free heap.
char* saes_decrypt_heap(const char* ctext, int size, unsigned char* key)
{
	if (size == 0 || (size & 1)) {
		return NULL;
	}
	char* ptext = (char*)malloc(size);
	saes_decrypt_stream((const unsigned char*)ctext, size, key, ptext);
	return ptext;
}

tsaes_encrypt::tsaes_encrypt(const char* ctext, int s, unsigned char* key)
	: size(0)
{
	buf = saes_encrypt_heap(ctext, s, key);
	if (buf) {
		size = s & 1? s + 1: s;
	}
}

tsaes_decrypt::tsaes_decrypt(const char* ptext, int s, unsigned char* key)
	: size(s)
{
	buf = saes_decrypt_heap(ptext, size, key);
	if (buf && !buf[size - 1]) {
		size --;
	}
}

