/* $Id: editor_display.hpp 47608 2010-11-21 01:56:29Z shadowmaster $ */
/*
   Copyright (C) 2008 - 2010 by Tomasz Sniatowski <kailoran@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef SLEEP_CHART_DISPLAY_HPP_INCLUDED
#define SLEEP_CHART_DISPLAY_HPP_INCLUDED

#include "display.hpp"

class chart_controller;
class unit_map;
class base_unit;
class unit;

namespace gui2 {
class treport;
class ttoggle_button;
}

class chart_display : public display
{
public:
	chart_display(chart_controller& controller, unit_map& units, CVideo& video, const gamemap& map, const config& theme_cfg, const config& level);
	~chart_display();

	std::string get_theme_patch() const;
	gui2::ttheme* create_theme_dlg(const config& cfg);
	void pre_change_resolution(std::map<const std::string, bool>& actives);
	void post_change_resolution(const std::map<const std::string, bool>& actives);

	bool in_theme() const { return true; }
	
protected:
	void pre_draw();
	/**
	* The editor uses different rules for terrain highlighting (e.g. selections)
	*/
	// image::TYPE get_image_type(const map_location& loc);

	void draw_hex(const map_location& loc);

	// const SDL_Rect& get_clip_rect();
	void pre_draw(rect_of_hexes& hexes);
	void draw_sidebar();
	void draw_invalidated();
	void redraw_units(const std::vector<map_location>& invalidated_unit_locs);
	void post_zoom();

	void set_mouse_overlay(surface& image_fg);
	
private:
	chart_controller& controller_;
	unit_map& units_;
};

#endif
