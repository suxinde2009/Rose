/* $Id: preprocessor.cpp 46399 2010-09-12 08:31:08Z silene $ */
/*
   Copyright (C) 2003 by David White <dave@whitevine.net>
   Copyright (C) 2005 - 2010 by Guillaume Melquiond <guillaume.melquiond@gmail.com>
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
 * WML preprocessor.
 */

#include "global.hpp"

#include "config.hpp"
#include "filesystem.hpp"
#include "log.hpp"
#include "loadscreen.hpp"
#include "wesconfig.h"
#include "serialization/binary_or_text.hpp"
#include "serialization/string_utils.hpp"
#include "serialization/parser.hpp"
#include "filesystem.hpp"
#include "util.hpp"
#include "wml_exception.hpp"

#include <boost/foreach.hpp>
#include <stdexcept>

static lg::log_domain log_config("config");
#define ERR_CF LOG_STREAM(err, log_config)
#define WRN_CF LOG_STREAM(warn, log_config)
#define LOG_CF LOG_STREAM(info, log_config)
#define DBG_CF LOG_STREAM(debug, log_config)

static lg::log_domain log_preprocessor("preprocessor");
#define LOG_PREPROC LOG_STREAM(info,log_preprocessor)

using std::streambuf;


// map associating each filename encountered to a number
typedef std::map<std::string, int> t_file_number_map;
static t_file_number_map file_number_map;

static bool encode_filename = true;

// get filename associated to this code
static std::string get_filename(const std::string& file_code){
	if(!encode_filename)
		return file_code;

	std::stringstream s;
	s << file_code;
	int n = 0;
	s >> std::hex >> n;

	BOOST_FOREACH (const t_file_number_map::value_type& p, file_number_map){
		if(p.second == n)
			return p.first;
	}
	return "<unknown>";
}

// get code associated to this filename
static std::string get_file_code(const std::string& filename){
	if(!encode_filename)
		return filename;

	// current number of encountered filenames
	static int current_file_number = 0;

	int& fnum = file_number_map[utils::escape(filename, " \\")];
	if(fnum == 0)
		fnum = ++current_file_number;

	std::ostringstream shex;
	shex << std::hex << fnum;
	return shex.str();
}

// decode the filenames placed in a location
static std::string get_location(const std::string& loc)
{
	std::string res;
	std::vector< std::string > pos = utils::quoted_split(loc, ' ');
	if(pos.empty())
		return res;
	std::vector< std::string >::const_iterator i = pos.begin(), end = pos.end();
	while (true) {
		res += get_filename(*(i++));
		if(i == end) break;
		res += ' ';
		res += *(i++);
		if(i == end) break;
		res += ' ';
	}
	return res;
}

bool preproc_define::operator==(preproc_define const &v) const {
	return value == v.value && arguments == v.arguments;
}

bool preproc_define::operator<(preproc_define const &v) const {
	if (location < v.location)
		return true;
	if (v.location < location)
		return false;
	return linenum < v.linenum;
}

void preproc_define::write_argument(config_writer& writer, const std::string& arg) const
{

	const std::string key = "argument";

	writer.open_child(key);

	writer.write_key_val("name", arg);
	writer.close_child(key);
}

void preproc_define::write(config_writer& writer, const std::string& name) const
{

	const std::string key = "preproc_define";
	writer.open_child(key);

	writer.write_key_val("name", name);
	writer.write_key_val("value", value);
	writer.write_key_val("textdomain", textdomain);
	writer.write_key_val("linenum", lexical_cast<std::string>(linenum));
	writer.write_key_val("location", get_location(location));

	BOOST_FOREACH (const std::string &arg, arguments)
		write_argument(writer, arg);

	writer.close_child(key);
}

void preproc_define::read_argument(const config &cfg)
{
	arguments.push_back(cfg["name"]);
}

void preproc_define::read(const config& cfg)
{
	value = cfg["value"].str();
	textdomain = cfg["textdomain"].str();
	linenum = cfg["linenum"];
	location = cfg["location"].str();

	BOOST_FOREACH (const config &arg, cfg.child_range("argument"))
		read_argument(arg);
}

preproc_map::value_type preproc_define::read_pair(const config &cfg)
{
	preproc_define second;
	second.read(cfg);
	return preproc_map::value_type(cfg["name"], second);
}

std::ostream& operator<<(std::ostream& stream, const preproc_define& def)
{
	return stream << "value: " << def.value << " arguments: " << def.location;
}

std::ostream& operator<<(std::ostream& stream, const preproc_map::value_type& def)
{
	return stream << def.second;
}

class preprocessor;
class preprocessor_file;
class preprocessor_data;
class preprocessor_streambuf;
struct preprocessor_deleter;

/**
 * Base class for preprocessing an input.
 */
class preprocessor
{
	preprocessor *const old_preprocessor_;
	std::string old_textdomain_;
	std::string old_location_;
	int old_linenum_;
protected:
	preprocessor_streambuf &target_;
	preprocessor(preprocessor_streambuf &);
public:
	/**
	 * Preprocesses and sends some text to the #target_ buffer.
	 * @return false when the input has no data left.
	 */
	virtual bool get_chunk() = 0;
	virtual ~preprocessor();
};

/**
 * Target for sending preprocessed output.
 * Objects of this class can be plugged into an STL stream.
 */
class preprocessor_streambuf: public streambuf
{
	std::string out_buffer_;      /**< Buffer read by the STL stream. */
	virtual int underflow();
	std::stringstream buffer_;    /**< Buffer filled by the #current_ preprocessor. */
	preprocessor *current_;       /**< Input preprocessor. */
	preproc_map *defines_;
	preproc_map default_defines_;
	std::string textdomain_;
	std::string location_;
	int linenum_;
	int depth_;
	/**
	 * Set to true if one preprocessor for this target started to read a string.
	 * Deeper-nested preprocessors are then forbidden to.
	 */
	bool quoted_;
	friend class preprocessor;
	friend class preprocessor_file;
	friend class preprocessor_data;
	friend struct preprocessor_deleter;
	preprocessor_streambuf(preprocessor_streambuf const &);
public:
	preprocessor_streambuf(preproc_map *);
	void error(const std::string &, int);
};

preprocessor_streambuf::preprocessor_streambuf(preproc_map *def) :
	streambuf(),
	out_buffer_(""),
	buffer_(),
	current_(NULL),
	defines_(def),
	default_defines_(),
	textdomain_(PACKAGE),
	location_(""),
	linenum_(0),
	depth_(0),
	quoted_(false)
{
}

preprocessor_streambuf::preprocessor_streambuf(preprocessor_streambuf const &t) :
	streambuf(),
	out_buffer_(""),
	buffer_(),
	current_(NULL),
	defines_(t.defines_),
	default_defines_(),
	textdomain_(PACKAGE),
	location_(""),
	linenum_(0),
	depth_(t.depth_),
	quoted_(t.quoted_)
{
}

/**
 * Called by an STL stream whenever it has reached the end of #out_buffer_.
 * Fills #buffer_ by calling the #current_ preprocessor, then copies its
 * content into #out_buffer_.
 * @return the first character of #out_buffer_ if any, EOF otherwise.
 */
int preprocessor_streambuf::underflow()
{
	unsigned sz = 0;
	if (char *gp = gptr()) {
		if (gp < egptr()) {
			// Sanity check: the internal buffer has not been totally consumed,
			// should we force the caller to use what remains first?
			return *gp;
		}
		// The buffer has been completely read; fill it again.
		// Keep part of the previous buffer, to ensure putback capabilities.
		sz = out_buffer_.size();
		buffer_.str(std::string());
		if (sz > 3) {
			buffer_ << out_buffer_.substr(sz - 3);
			sz = 3;
		}
		else
		{
			buffer_ << out_buffer_;
		}
	} else {
		// The internal get-data pointer is null
	}
	const int desired_fill_amount = 2000;
	while (current_ && buffer_.rdbuf()->in_avail() < desired_fill_amount)
	{
		// Process files and data chunks until the desired buffer size is reached
		if (!current_->get_chunk()) {
			 // Delete the current preprocessor item to restore its predecessor
			delete current_;
		}
	}
	// Update the internal state and data pointers
	out_buffer_ = buffer_.str();
	char *begin = &*out_buffer_.begin();
	unsigned bs = out_buffer_.size();
	setg(begin, begin + sz, begin + bs);
	if (sz >= bs)
		return EOF;
	return static_cast<unsigned char>(*(begin + sz));
}

std::string lineno_string(const std::string &lineno)
{
	std::vector< std::string > pos = utils::quoted_split(lineno, ' ');
	std::vector< std::string >::const_iterator i = pos.begin(), end = pos.end();
	std::string included_from = " included from ";
	std::string res;
	while (i != end) {
		std::string const &line = *(i++);
		if (!res.empty())
			res += included_from;
		if (i != end)
			res += get_filename(*(i++));
		else
			res += "<unknown>";
		res += ':' + line;
	}
	if (res.empty()) res = "???";
	return res;
}

void preprocessor_streambuf::error(const std::string& error_type, int l)
{
	utils::string_map i18n_symbols;
	std::string position, error;
	std::ostringstream pos;
	pos << l << ' ' << location_;
	position = lineno_string(pos.str());
	error = error_type + " at " + position;
	ERR_CF << error << '\n';
	VALIDATE(false, error);
}


/**
 * Sets up a new preprocessor for stream buffer \a t.
 * Saves the current preprocessing context of #target_. It will be
 * automatically restored on destruction.
 */
preprocessor::preprocessor(preprocessor_streambuf &t) :
	old_preprocessor_(t.current_),
	old_textdomain_(t.textdomain_),
	old_location_(t.location_),
	old_linenum_(t.linenum_),
	target_(t)
{
	++target_.depth_;
	target_.current_ = this;
}

/**
 * Restores the old preprocessing context of #target_.
 * Appends location and domain directives to the buffer, so that the parser
 * notices these changes.
 */
preprocessor::~preprocessor()
{
	assert(target_.current_ == this);
	if (!old_location_.empty()) {
		target_.buffer_ << "\376line " << old_linenum_ << ' ' << old_location_ << '\n';
	}
	if (!old_textdomain_.empty() && target_.textdomain_ != old_textdomain_) {
		target_.buffer_ << "\376textdomain " << old_textdomain_ << '\n';
	}
	target_.current_  = old_preprocessor_;
	target_.location_ = old_location_;
	target_.linenum_  = old_linenum_;
	target_.textdomain_ = old_textdomain_;
	--target_.depth_;
}

/**
 * Specialized preprocessor for handling a file or a set of files.
 * A preprocessor_file object is created when a preprocessor encounters an
 * inclusion directive that resolves to a file or directory, e.g. '{themes/}'.
 */
class preprocessor_file: preprocessor
{
	std::vector< std::string > files_;
	std::vector< std::string >::const_iterator pos_, end_;
public:
	preprocessor_file(preprocessor_streambuf &, std::string const &);
	virtual bool get_chunk();

	static bool open_unicode;
};

bool preprocessor_file::open_unicode = false;

topen_unicode_lock::topen_unicode_lock(bool unicode)
	: original_(preprocessor_file::open_unicode)
{
	preprocessor_file::open_unicode = unicode;
}

topen_unicode_lock::~topen_unicode_lock()
{
	preprocessor_file::open_unicode = original_;
}

/**
 * Specialized preprocessor for handling any kind of input stream.
 * This is the core of the preprocessor.
 */
class preprocessor_data: preprocessor
{
	/** Description of a preprocessing chunk. */
	struct token_desc
	{
		enum TOKEN_TYPE {
			START,        // Toplevel
			PROCESS_IF,   // Processing the "if" branch of a ifdef/ifndef (the "else" branch will be skipped)
			PROCESS_ELSE, // Processing the "else" branch of a ifdef/ifndef
			SKIP_IF,      // Skipping the "if" branch of a ifdef/ifndef (the "else" branch, if any, will be processed)
			SKIP_ELSE,    // Skipping the "else" branch of a ifdef/ifndef
			STRING,       // Processing a string
			VERBATIM,     // Processing a verbatim string
			MACRO_SPACE,  // Processing between chunks of a macro call (skip spaces)
			MACRO_CHUNK,  // Processing inside a chunk of a macro call (stop on space or '(')
			MACRO_PARENS  // Processing a parenthesized macro argument
		};
		token_desc(TOKEN_TYPE type, const int stack_pos, const int linenum)
			: type(type)
			, stack_pos(stack_pos)
			, linenum(linenum)
		{
		}
		TOKEN_TYPE type;
		/** Starting position in #strings_ of the delayed text for this chunk. */
		int stack_pos;
		int linenum;
	};
	scoped_istream in_; /**< Input stream. */
	std::string directory_;
	/** Buffer for delayed input processing. */
	std::vector< std::string > strings_;
	/** Mapping of macro arguments to their content. */
	std::map<std::string, std::string> *local_defines_;
	/** Stack of nested preprocessing chunks. */
	std::vector< token_desc > tokens_;
	/**
	 * Set to true whenever input tokens cannot be directly sent to the target
	 * buffer. For instance, this happens with macro arguments. In that case,
	 * the output is redirected toward #strings_ until it can be processed.
	 */
	int slowpath_;
	/**
	 * Non-zero when the preprocessor has to skip some input text.
	 * Increased whenever entering a conditional branch that is not useful,
	 * e.g. a ifdef that evaluates to false.
	 */
	int skipping_;
	int linenum_;

	std::string read_word();
	std::string read_line();
	std::string read_rest_of_line();
	void skip_spaces();
	void skip_eol();
	void push_token(token_desc::TOKEN_TYPE);
	void pop_token();
	void put(char);
	void put(std::string const & /*, int change_line
	= 0 */);
	void conditional_skip(bool skip);
public:
	preprocessor_data(preprocessor_streambuf &,
	                  std::istream *,
	                  std::string const &history,
	                  std::string const &name, int line,
	                  std::string const &dir, std::string const &domain,
	                  std::map<std::string, std::string> *defines);
	~preprocessor_data();
	virtual bool get_chunk();

	friend bool operator==(preprocessor_data::token_desc::TOKEN_TYPE, char);
	friend bool operator==(char, preprocessor_data::token_desc::TOKEN_TYPE);
	friend bool operator!=(preprocessor_data::token_desc::TOKEN_TYPE, char);
	friend bool operator!=(char, preprocessor_data::token_desc::TOKEN_TYPE);
};

bool operator==(preprocessor_data::token_desc::TOKEN_TYPE, char)
{
	throw std::logic_error("don't compare tokens with characters");
}
bool operator==(char lhs, preprocessor_data::token_desc::TOKEN_TYPE rhs){ return rhs == lhs; }
bool operator!=(preprocessor_data::token_desc::TOKEN_TYPE rhs, char lhs){ return !(lhs == rhs); }
bool operator!=(char lhs, preprocessor_data::token_desc::TOKEN_TYPE rhs){ return rhs != lhs; }

preprocessor_file::preprocessor_file(preprocessor_streambuf &t, std::string const &name) :
	preprocessor(t),
	files_(),
	pos_(),
	end_()
{
	if (is_directory(name)) {
		increment_preprocessor_progress(name, false);
		get_files_in_dir(name, &files_, NULL, ENTIRE_FILE_PATH, SKIP_MEDIA_DIR, DO_REORDER);
	} else {
		increment_preprocessor_progress(name, true);
		std::istream * file_stream = istream_file(name, open_unicode);
		if (!file_stream->good()) {
			ERR_CF << "Could not open file " << name << "\n";
			delete file_stream;
		}
		else
			new preprocessor_data(t, file_stream, "", get_short_wml_path(name),
				1, directory_name(name), t.textdomain_, NULL);
	}
	pos_ = files_.begin();
	end_ = files_.end();
}

/**
 * preprocessor_file::get_chunk()
 *
 * Inserts and processes the next file in the list of included files.
 * @return	false if there is no next file.
 */
bool preprocessor_file::get_chunk()
{
	while (pos_ != end_) {
		const std::string &name = *(pos_++);
		unsigned sz = name.size();
		// Use reverse iterator to optimize testing
		if (sz < 5 || !std::equal(name.rbegin(), name.rbegin() + 4, "gfc."))
			continue;
		new preprocessor_file(target_, name);
		return true;
	}
	return false;
}

preprocessor_data::preprocessor_data(preprocessor_streambuf &t,
	std::istream *i, std::string const &history, std::string const &name, int linenum,
	std::string const &directory, std::string const &domain,
	std::map<std::string, std::string> *defines) :
	preprocessor(t),
	in_(i),
	directory_(directory),
	strings_(),
	local_defines_(defines),
	tokens_(),
	slowpath_(0),
	skipping_(0),
	linenum_(linenum)
{
	std::ostringstream s;

	s << history;
	if (!name.empty()) {
		if (!history.empty())
			s << ' ';

		s << get_file_code(name);
	}

	if (!t.location_.empty())
		s << ' ' << t.linenum_ << ' ' << t.location_;
	t.location_ = s.str();
	t.linenum_ = linenum;

	t.buffer_ << "\376line " << linenum << ' ' << t.location_ << '\n';
	if (t.textdomain_ != domain) {
		t.buffer_ << "\376textdomain " << domain << '\n';
		t.textdomain_ = domain;
	}

	push_token(token_desc::START);
}

preprocessor_data::~preprocessor_data()
{
	delete local_defines_;
}

void preprocessor_data::push_token(token_desc::TOKEN_TYPE t)
{
	token_desc token(t, strings_.size(), linenum_);
	tokens_.push_back(token);
	if (t == token_desc::MACRO_SPACE) {
		// Macro expansions do not have any associated storage at start.
		return;
	} else if (t == token_desc::STRING || t == token_desc::VERBATIM) {
		/* Quoted strings are always inlined in the parent token. So
		   they need neither storage nor metadata, unless the parent
		   token is a macro expansion. */
		token_desc::TOKEN_TYPE &outer_type = tokens_[tokens_.size() - 2].type;
		if (outer_type != token_desc::MACRO_SPACE)
			return;
		outer_type = token_desc::MACRO_CHUNK;
		tokens_.back().stack_pos = strings_.size() + 1;
	}
	std::ostringstream s;
	if (!skipping_ && slowpath_) {
		s << "\376line " << linenum_ << ' ' << target_.location_
		  << "\n\376textdomain " << target_.textdomain_ << '\n';
	}
	strings_.push_back(s.str());
}

void preprocessor_data::pop_token()
{
	token_desc::TOKEN_TYPE inner_type = tokens_.back().type;
	unsigned stack_pos = tokens_.back().stack_pos;
	tokens_.pop_back();
	token_desc::TOKEN_TYPE &outer_type = tokens_.back().type;
	if (inner_type == token_desc::MACRO_PARENS) {
		// Parenthesized macro arguments are left on the stack.
		assert(outer_type == token_desc::MACRO_SPACE);
		return;
	}
	if (inner_type == token_desc::STRING || inner_type == token_desc::VERBATIM) {
		// Quoted strings are always inlined.
		assert(stack_pos == strings_.size());
		return;
	}
	if (outer_type == token_desc::MACRO_SPACE) {
		/* A macro expansion does not have any associated storage.
		   Instead, storage of the inner token is not discarded
		   but kept as a new macro argument. But if the inner token
		   was a macro expansion, it is about to be appended, so
		   prepare for it. */
		if (inner_type == token_desc::MACRO_SPACE || inner_type == token_desc::MACRO_CHUNK) {
			strings_.erase(strings_.begin() + stack_pos, strings_.end());
			strings_.push_back(std::string());
		}
		assert(stack_pos + 1 == strings_.size());
		outer_type = token_desc::MACRO_CHUNK;
		return;
	}
	strings_.erase(strings_.begin() + stack_pos, strings_.end());
}

void preprocessor_data::skip_spaces()
{
	for(;;) {
		int c = in_->peek();
		if (!in_->good() || (c != ' ' && c != '\t'))
			return;
		in_->get();
	}
}

void preprocessor_data::skip_eol()
{
	for(;;) {
		int c = in_->get();
		if (c == '\n') {
			++linenum_;
			return;
		}
		if (!in_->good())
			return;
	}
}

std::string preprocessor_data::read_word()
{
	std::string res;
	for(;;) {
		int c = in_->peek();
		if (c == preprocessor_streambuf::traits_type::eof() || utils::portable_isspace(c)) {
			// DBG_CF << "(" << res << ")\n";
			return res;
		}
		in_->get();
		res += static_cast<char>(c);
	}
}

std::string preprocessor_data::read_line()
{
	std::string res;
	for(;;) {
		int c = in_->get();
		if (c == '\n') {
			++linenum_;
			return res;
		}
		if (!in_->good())
			return res;
		if (c != '\r')
			res += static_cast<char>(c);
	}
}

std::string preprocessor_data::read_rest_of_line()
{
	std::string res;
	while(in_->good() && in_->peek() != '\n') {
		int c = in_->get();
		if (c != '\r')
			res += static_cast<char>(c);
	}
	return res;
}

void preprocessor_data::put(char c)
{
	if (skipping_)
		return;
	if (slowpath_) {
		strings_.back() += c;
		return;
	}

	int cond_linenum = c == '\n' ? linenum_ - 1 : linenum_;
	if (unsigned diff = cond_linenum - target_.linenum_)
	{
		target_.linenum_ = cond_linenum;
		if (diff <= target_.location_.size() + 11) {
			target_.buffer_ << std::string(diff, '\n');
		} else {
			target_.buffer_ << "\376line " << target_.linenum_
				<< ' ' << target_.location_ << '\n';
		}
	}

	if (c == '\n')
		++target_.linenum_;
	target_.buffer_ << c;
}

void preprocessor_data::put(std::string const &s /*, int line_change*/)
{
	if (skipping_)
		return;
	if (slowpath_) {
		strings_.back() += s;
		return;
	}
	target_.buffer_ << s;
//	target_.linenum_ += line_change;
}

void preprocessor_data::conditional_skip(bool skip)
{
	if (skip) ++skipping_;
	push_token(skip ? token_desc::SKIP_ELSE : token_desc::PROCESS_IF);
}

bool preprocessor_data::get_chunk()
{
	char c = in_->get();
	token_desc &token = tokens_.back();
	if (!in_->good()) {
		// The end of file was reached.
		// Make sure we don't have any incomplete tokens.
		char const *s;
		switch (token.type) {
		case token_desc::START: return false; // everything is fine
		case token_desc::PROCESS_IF:
		case token_desc::SKIP_IF:
		case token_desc::PROCESS_ELSE:
		case token_desc::SKIP_ELSE: s = "#ifdef or #ifndef"; break;
		case token_desc::STRING: s = "Quoted string"; break;
		case token_desc::VERBATIM: s = "Verbatim string"; break;
		case token_desc::MACRO_CHUNK:
		case token_desc::MACRO_SPACE: s = "Macro substitution"; break;
		case token_desc::MACRO_PARENS: s = "Macro argument"; break;
		default: s = "???";
		}
		target_.error(std::string(s) + " not terminated", token.linenum);
	}
	if (c == '\n')
		++linenum_;
	if (c == '\376') {
		std::string buffer(1, c);
		for(;;) {
			char d = in_->get();
			if (!in_->good() || d == '\n')
				break;
			buffer += d;
		}
		buffer += '\n';
		// line_change = 1-1 = 0
		put(buffer);
	} else if (token.type == token_desc::VERBATIM) {
		put(c);
		if (c == '>' && in_->peek() == '>') {
			put(in_->get());
			pop_token();
		}
	} else if (c == '<' && in_->peek() == '<') {
		in_->get();
		push_token(token_desc::VERBATIM);
		put('<');
		put('<');
	} else if (c == '"') {
		if (token.type == token_desc::STRING) {
			target_.quoted_ = false;
			put(c);
			pop_token();
		} else if (!target_.quoted_) {
			target_.quoted_ = true;
			push_token(token_desc::STRING);
			put(c);
		} else {
			target_.error("Nested quoted string" , linenum_);
		}
	} else if (c == '{') {
		push_token(token_desc::MACRO_SPACE);
		++slowpath_;
	} else if (c == ')' && token.type == token_desc::MACRO_PARENS) {
		pop_token();
	} else if (c == '#' && !target_.quoted_) {
		std::string command = read_word();
		bool comment = false;
		if (command == "define") {
			skip_spaces();
			int linenum = linenum_;
			std::vector< std::string > items = utils::split(read_line(), ' ');
			if (items.empty()) {
				target_.error("No macro name found after #define directive", linenum);
			}
			std::string symbol = items.front();
			items.erase(items.begin());
			int found_enddef = 0;
			std::string buffer;
			for(;;) {
				if (!in_->good())
					break;
				char d = in_->get();
				if (d == '\n')
					++linenum_;
				buffer += d;
				if (d == '#')
					found_enddef = 1;
				else if (found_enddef > 0)
					if (++found_enddef == 7) {
						if (std::equal(buffer.end() - 6, buffer.end(), "enddef"))
							break;
						else
							found_enddef = 0;
					}
			}
			if (found_enddef != 7) {
				target_.error("Unterminated preprocessor definition", linenum_);
			}
			if (!skipping_) {
				buffer.erase(buffer.end() - 7, buffer.end());
				(*target_.defines_)[symbol] = preproc_define(buffer, items, target_.textdomain_,
					                       linenum + 1, target_.location_);
				LOG_CF << "defining macro " << symbol << " (location " << get_location(target_.location_) << ")\n";
			}
		} else if (command == "ifdef") {
			skip_spaces();
			std::string const &symbol = read_word();
			bool found = target_.defines_->count(symbol) != 0;
			DBG_CF << "testing for macro " << symbol << ": "
				<< (found ? "defined" : "not defined") << '\n';
			conditional_skip(!found);
		} else if (command == "ifndef") {
			skip_spaces();
			std::string const &symbol = read_word();
			bool found = target_.defines_->count(symbol) != 0;
			DBG_CF << "testing for macro " << symbol << ": "
				<< (found ? "defined" : "not defined") << '\n';
			conditional_skip(found);
		} else if (command == "ifhave") {
			skip_spaces();
			std::string const &symbol = read_word();
			bool found = !get_wml_location(symbol, directory_).empty();
			DBG_CF << "testing for file or directory " << symbol << ": "
				<< (found ? "found" : "not found") << '\n';
			conditional_skip(!found);
		} else if (command == "ifnhave") {
			skip_spaces();
			std::string const &symbol = read_word();
			bool found = !get_wml_location(symbol, directory_).empty();
			DBG_CF << "testing for file or directory " << symbol << ": "
				<< (found ? "found" : "not found") << '\n';
			conditional_skip(found);
		} else if (command == "else") {
			if (token.type == token_desc::SKIP_ELSE) {
				pop_token();
				--skipping_;
				push_token(token_desc::PROCESS_ELSE);
			} else if (token.type == token_desc::PROCESS_IF) {
				pop_token();
				++skipping_;
				push_token(token_desc::SKIP_IF);
			} else {
				target_.error("Unexpected #else", linenum_);
			}
		} else if (command == "endif") {
			switch (token.type) {
			case token_desc::SKIP_IF:
			case token_desc::SKIP_ELSE: --skipping_;
			case token_desc::PROCESS_IF:
			case token_desc::PROCESS_ELSE: break;
			default:
				target_.error("Unexpected #endif", linenum_);
			}
			pop_token();
		} else if (command == "textdomain") {
			skip_spaces();
			std::string const &s = read_word();
			if (s != target_.textdomain_) {
				put("#textdomain ");
				put(s);
				target_.textdomain_ = s;
			}
			comment = true;
		} else if (command == "enddef") {
			target_.error("Unexpected #enddef", linenum_);
		} else if (command == "undef") {
			skip_spaces();
			std::string const &symbol = read_word();
			if (!skipping_) {
				target_.defines_->erase(symbol);
				LOG_CF << "undefine macro " << symbol << " (location " << get_location(target_.location_) << ")\n";
			}
		} else if (command == "error") {
			if (!skipping_) {
				skip_spaces();
				std::ostringstream error;
				error << "#error: \"" << read_rest_of_line() << '"';
				target_.error(error.str(), linenum_);
			} else
				DBG_CF << "Skipped an error\n";
		} else if (command == "warning") {
			if (!skipping_) {
				skip_spaces();
				std::string message = read_rest_of_line();
				WRN_CF << "#warning: \"" << message << "\" at "
					<< linenum_ << ' ' << get_location(target_.location_) << '\n';
			} else
				DBG_CF << "Skipped a warning\n";
		} else
			comment = token.type != token_desc::MACRO_SPACE;
		skip_eol();
		if (comment)
			put('\n');
	} else if (token.type == token_desc::MACRO_SPACE || token.type == token_desc::MACRO_CHUNK) {
		if (c == '(') {
			// If a macro argument was started, it is implicitly ended.
			token.type = token_desc::MACRO_SPACE;
			push_token(token_desc::MACRO_PARENS);
		} else if (utils::portable_isspace(c)) {
			// If a macro argument was started, it is implicitly ended.
			token.type = token_desc::MACRO_SPACE;
		} else if (c == '}') {
			--slowpath_;
			if (skipping_) {
				pop_token();
				return true;
			}
			// FIXME: is this obsolete?
			//if (token.type == token_desc::MACRO_SPACE) {
			//	if (!strings_.back().empty()) {
			//		std::ostringstream error;
			//		std::ostringstream location;
			//		error << "Can't parse new macro parameter with a macro call scope open";
			//		location<<linenum_<<' '<<target_.location_;
			//		target_.error(error.str(), location.str());
			//	}
			//	strings_.pop_back();
			//}

			std::string symbol = strings_[token.stack_pos];
			std::string::size_type pos;
			while ((pos = symbol.find('\376')) != std::string::npos) {
				std::string::iterator b = symbol.begin(); // invalidated at each iteration
				symbol.erase(b + pos, b + symbol.find('\n', pos + 1) + 1);
			}
			std::map<std::string, std::string>::const_iterator arg;
			preproc_map::const_iterator macro;
			// If this is a known pre-processing symbol, then we insert it,
			// otherwise we assume it's a file name to load.
			if (local_defines_ &&
			    (arg = local_defines_->find(symbol)) != local_defines_->end())
			{
				if (strings_.size() - token.stack_pos != 1)
				{
					std::ostringstream error;
					error << "macro argument '" << symbol
					      << "' does not expect any argument";
					target_.error(error.str(), linenum_);
				}
				std::ostringstream v;
				v << arg->second << "\376line " << linenum_ << ' ' << target_.location_
				  << "\n\376textdomain " << target_.textdomain_ << '\n';
				pop_token();
				put(v.str());
			}
			else if ((macro = target_.defines_->find(symbol)) != target_.defines_->end())
			{
				preproc_define const &val = macro->second;
				size_t nb_arg = strings_.size() - token.stack_pos - 1;
				if (nb_arg != val.arguments.size())
				{
					std::ostringstream error;
					error << "preprocessor symbol '" << symbol << "' expects "
					      << val.arguments.size() << " arguments, but has "
					      << nb_arg << " arguments";
					target_.error(error.str(), linenum_);
				}
				std::istringstream *buffer = new std::istringstream(val.value);
				std::map<std::string, std::string> *defines =
					new std::map<std::string, std::string>;
				for (size_t i = 0; i < nb_arg; ++i) {
					(*defines)[val.arguments[i]] = strings_[token.stack_pos + i + 1];
				}
				pop_token();
				std::string const &dir = directory_name(val.location.substr(0, val.location.find(' ')));
				if (!slowpath_) {
					DBG_CF << "substituting macro " << symbol << '\n';
					new preprocessor_data(target_, buffer, val.location, "",
					                      val.linenum, dir, val.textdomain, defines);
				} else {
					DBG_CF << "substituting (slow) macro " << symbol << '\n';
					std::ostringstream res;
					preprocessor_streambuf *buf =
						new preprocessor_streambuf(target_);
					{	std::istream in(buf);
						new preprocessor_data(*buf, buffer, val.location, "",
						                      val.linenum, dir, val.textdomain, defines);
						res << in.rdbuf(); }
					delete buf;
					put(res.str());
				}
			} else if (target_.depth_ < 40) {
				LOG_CF << "Macro definition not found for " << symbol << " , attempting to open as file.\n";
				pop_token();
				std::string prefix;
				std::string nfname = get_wml_location(symbol, directory_);
				if (!nfname.empty())
				{
					if (!slowpath_)
						new preprocessor_file(target_, nfname);
					else {
						std::ostringstream res;
						preprocessor_streambuf *buf =
							new preprocessor_streambuf(target_);
						{	std::istream in(buf);
							new preprocessor_file(*buf, nfname);
							res << in.rdbuf(); }
						delete buf;
						put(res.str());
					}
				}
				else
				{
					std::ostringstream error;
					error << "Macro/file '" << symbol << "' is missing";
					target_.error(error.str(), linenum_);
				}
			} else {
				target_.error("Too much nested preprocessing inclusions", linenum_);
			}
		}
		else if (!skipping_)
		{
			if (token.type == token_desc::MACRO_SPACE)
			{
				std::ostringstream s;
				s << "\376line " << linenum_ << ' ' << target_.location_
				  << "\n\376textdomain " << target_.textdomain_ << '\n';
				strings_.push_back(s.str());
				token.type = token_desc::MACRO_CHUNK;
			}
			put(c);
		}
	} else
		put(c);
	return true;
}

struct preprocessor_deleter: std::basic_istream<char>
{
	preprocessor_streambuf *buf_;
	preproc_map *defines_;
	preprocessor_deleter(preprocessor_streambuf *buf, preproc_map *defines);
	~preprocessor_deleter();
};

preprocessor_deleter::preprocessor_deleter(preprocessor_streambuf *buf,
		preproc_map *defines)
	: std::basic_istream<char>(buf), buf_(buf), defines_(defines)
{
}

preprocessor_deleter::~preprocessor_deleter()
{
	clear(std::ios_base::goodbit);
	exceptions(std::ios_base::goodbit);
	rdbuf(NULL);
	delete buf_;
	delete defines_;
}


std::istream *preprocess_file(std::string const &fname, preproc_map *defines)
{
	log_scope("preprocessing file...");
	preproc_map *owned_defines = NULL;
	if (!defines) {
		// If no preproc_map has been given, create a new one,
		// and ensure it is destroyed when the stream is
// ??
		// by giving it to the deleter.
		owned_defines = new preproc_map;
		defines = owned_defines;
	}
	preprocessor_streambuf *buf = new preprocessor_streambuf(defines);
	new preprocessor_file(*buf, fname);
	return new preprocessor_deleter(buf, owned_defines);
}

void preprocess_resource(const std::string& res_name, preproc_map *defines_map,
			 bool write_cfg, bool write_plain_cfg,std::string target_directory)
{
	if (is_directory(res_name))
	{
		std::vector<std::string> dirs,files;

		get_files_in_dir(res_name, &files, &dirs, ENTIRE_FILE_PATH, SKIP_MEDIA_DIR, DO_REORDER);

		// subdirectories
		BOOST_FOREACH (const std::string& dir, dirs)
		{
			LOG_PREPROC<<"processing sub-dir: "<<dir<<'\n';
			preprocess_resource(dir,defines_map,write_cfg,write_plain_cfg,target_directory);
		}

		// files in current directory
		BOOST_FOREACH (const std::string& file, files)
		{
			preprocess_resource(file,defines_map,write_cfg,write_plain_cfg,target_directory);
		}
		return;
	}

	// process only config files.
	if (ends_with(res_name,".cfg") == false)
		return;

	LOG_PREPROC<<"processing resource: "<<res_name<<'\n';

	//disable filename encoding to get clear #line in cfg.plain
	encode_filename = false;

	scoped_istream stream = preprocess_file(res_name, defines_map);
	std::stringstream ss;
	ss<<(*stream).rdbuf();
	std::string streamContent = ss.str();

	config cfg;

	if (write_cfg == true || write_plain_cfg == true)
	{
		read(cfg, streamContent);
		const std::string preproc_res_name = target_directory + "/" + file_name(res_name);

		// write the processed cfg file
		if (write_cfg == true)
		{
			LOG_PREPROC<<"writing cfg file: "<<preproc_res_name<<'\n';
			create_directory_if_missing_recursive(directory_name(preproc_res_name));
			scoped_ostream outStream(ostream_file(preproc_res_name));
			write(*outStream,cfg);
		}

		// write the plain cfg file
		if (write_plain_cfg == true)
		{
			LOG_PREPROC<<"writing plain cfg file: "<<(preproc_res_name+".plain")<<'\n';
			create_directory_if_missing_recursive(directory_name(preproc_res_name));
			write_file(preproc_res_name+".plain",streamContent);
		}
	}
}
