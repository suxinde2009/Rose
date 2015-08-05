/* $Id: lobby_player_info.hpp 48440 2011-02-07 20:57:31Z mordante $ */
/*
   Copyright (C) 2009 - 2011 by Tomasz Sniatowski <kailoran@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_DIALOGS_CHART_THEME_HPP_INCLUDED
#define GUI_DIALOGS_CHART_THEME_HPP_INCLUDED

#include "gui/dialogs/theme.hpp"
#include "config.hpp"
#include "gui/widgets/widget.hpp"

class chart_display;
class chart_controller;
class unit_map;

namespace gui2 {

class treport;
class tstacked_widget;

class tchart_theme: public ttheme
{
public:
	enum { ZOOM, POSITION, NUM_REPORTS};

	enum {
		HOTKEY_RETURN = HOTKEY_MIN,
		HOTKEY_STATUS,
		HOTKEY_GRID,
		HOTKEY_SWITCH,
		HOTKEY_RCLICK,

		HOTKEY_RUN,
		HOTKEY_SETTING, HOTKEY_SPECIAL_SETTING, HOTKEY_LINKED_GROUP, HOTKEY_ERASE,
		HOTKEY_INSERT_TOP, HOTKEY_INSERT_BOTTOM, HOTKEY_ERASE_ROW,
		HOTKEY_INSERT_LEFT, HOTKEY_INSERT_RIGHT, HOTKEY_ERASE_COLUMN,
		HOTKEY_INSERT_CHILD, HOTKEY_ERASE_CHILD, HOTKEY_UNPACK
	};

	tchart_theme(chart_display& disp, chart_controller& controller, const config& cfg);

private:
	void app_pre_show();

private:
	chart_controller& controller_;
	tstacked_widget* page_panel_;
};

} //end namespace gui2

#endif
