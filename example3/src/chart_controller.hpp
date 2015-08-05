/* $Id: editor_controller.hpp 47816 2010-12-05 18:08:38Z mordante $ */
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

#ifndef STUDIO_MKWIN_CONTROLLER_HPP_INCLUDED
#define STUDIO_MKWIN_CONTROLLER_HPP_INCLUDED

#include "controller_base.hpp"
#include "mouse_handler_base.hpp"
#include "chart_display.hpp"
#include "unit_map.hpp"
#include "map.hpp"
#include "gui/auxiliary/window_builder.hpp"

/**
 * The editor_controller class containts the mouse and keyboard event handling
 * routines for the editor. It also serves as the main editor class with the
 * general logic.
 */
class chart_controller : public controller_base, public events::mouse_handler_base
{
public:
	static int original_width;
	static int original_height;

	/**
	 * The constructor. A initial map context can be specified here, the controller
	 * will assume ownership and delete the pointer during destruction, but changes
	 * to the map can be retrieved between the main loop's end and the controller's
	 * destruction.
	 */
	chart_controller(const config &top_config, CVideo& video, bool theme);

	~chart_controller();

	/** Editor main loop */
	void main_loop();

	void init_gui(CVideo& video);
	void init_music(const config& top_config);

	chart_display& gui() { return *gui_; }
	const chart_display& gui() const { return *gui_; }

	const map_location& selected_hex() const { return selected_hex_; }

	void mouse_motion(int x, int y, const bool browse, bool update);
	bool left_click(int x, int y, const bool browse);
	bool right_click(int x, int y, const bool browse);

	bool toggle_report(gui2::twidget* widget);

	bool in_context_menu(const std::string& id) const;
	bool actived_context_menu(const std::string& id) const;

	void do_right_click();
	void select_unit(unit* u);

	void post_change_resolution();

	/* controller_base overrides */
	mouse_handler_base& get_mouse_handler_base();
	chart_display& get_display() { return *gui_; }
	const chart_display& get_display() const { return *gui_; }

	unit_map& get_units() { return units_; }
	const unit_map& get_units() const { return units_; }

private:
	/** command_executor override */
	void execute_command2(int command, const std::string& sparam);

	void goto_title();

private:
	/** The display object used and owned by the editor. */
	chart_display* gui_;

	gamemap map_;
	unit_map units_;

	map_location selected_hex_;
	bool snore_;
	
	/** Quit main loop flag */
	bool do_quit_;
};

#endif
