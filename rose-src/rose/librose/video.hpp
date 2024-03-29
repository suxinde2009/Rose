/* $Id: video.hpp 47615 2010-11-21 13:57:02Z mordante $ */
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
#ifndef VIDEO_HPP_INCLUDED
#define VIDEO_HPP_INCLUDED

#include "events.hpp"
#include "exceptions.hpp"
#include "lua_jailbreak_exception.hpp"

#include <boost/utility.hpp>

struct surface;

//possible flags when setting video modes
#define FULL_SCREEN SDL_FULLSCREEN
#define VIDEO_MEMORY SDL_HWSURFACE
#define SYSTEM_MEMORY SDL_SWSURFACE

surface display_format_alpha(surface surf);
surface get_video_surface();
SDL_Rect screen_area();

class CVideo : private boost::noncopyable 
{
public:
	CVideo();
	~CVideo();


	int modePossible( int x, int y, int bits_per_pixel, int flags);
	int setMode( int x, int y, int bits_per_pixel, int flags );

	//did the mode change, since the last call to the modeChanged() method?
	bool modeChanged();

	//functions to get the dimensions of the current video-mode
	int getx() const;
	int gety() const;
	SDL_Rect bound() const;

	void sdl_set_window_size(int width, int height);

	//blits a surface with black as alpha
	void blit_surface(int x, int y, surface surf, SDL_Rect* srcrect=NULL, SDL_Rect* clip_rect=NULL);
	void flip();

	surface& getSurface();
	SDL_Window* getWindow();
	Uint32 getformat() const;

	bool isFullScreen() const;

	struct error : public game::error
	{
		error() : game::error("Video initialization failed") {}
	};

	class quit
		: public tlua_jailbreak_exception
	{
	public:

		quit()
			: tlua_jailbreak_exception()
		{
		}

	private:

		IMPLEMENT_LUA_JAILBREAK_EXCEPTION(quit)
	};

	//functions to allow changing video modes when 16BPP is emulated
	void setBpp( int bpp );
	int getBpp();

	//functions to set and clear 'help strings'. A 'help string' is like a tooltip, but it appears
	//at the bottom of the screen, so as to not be intrusive. Setting a help string sets what
	//is currently displayed there.
	int set_help_string(const std::string& str);
	void clear_help_string(int handle);
	void clear_all_help_strings();

	//function to stop the screen being redrawn. Anything that happens while
	//the update is locked will be hidden from the user's view.
	//note that this function is re-entrant, meaning that if lock_updates(true)
	//is called twice, lock_updates(false) must be called twice to unlock
	//updates.
	void lock_updates(bool value);
	bool update_locked() const;

private:

	void initSDL();

	bool mode_changed_;

	int bpp_;	// Store real bits per pixel

	//variables for help strings
	int help_string_;

	int updatesLocked_;
};

//an object which will lock the display for the duration of its lifetime.
struct update_locker
{
	update_locker(CVideo& v, bool lock=true) : video(v), unlock(lock) {
		if(lock) {
			video.lock_updates(true);
		}
	}

	~update_locker() {
		unlock_update();
	}

	void unlock_update() {
		if(unlock) {
			video.lock_updates(false);
			unlock = false;
		}
	}

private:
	CVideo& video;
	bool unlock;
};

#endif
