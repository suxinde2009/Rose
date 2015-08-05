/* $Id: editor_controller.cpp 47755 2010-11-29 12:57:31Z shadowmaster $ */
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
#define GETTEXT_DOMAIN "wesnoth-lib"

#include "chart_controller.hpp"
#include "chart_display.hpp"

#include "gettext.hpp"
#include "integrate.hpp"
#include "formula_string_utils.hpp"
#include "preferences.hpp"
#include "sound.hpp"
#include "filesystem.hpp"
#include "hotkeys.hpp"
#include "config_cache.hpp"
#include "preferences_display.hpp"
#include "gui/dialogs/chart_theme.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/dialogs/browse.hpp"
#include "gui/widgets/window.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/listbox.hpp"
#include "serialization/parser.hpp"

#include <boost/foreach.hpp>
#include <boost/bind.hpp>

void square_fill_frame(t_translation::t_map& tiles, size_t start, const t_translation::t_terrain& terrain, bool front, bool back)
{
	if (front) {
		// first column(border)
		for (size_t n = start; n < tiles[0].size() - start; n ++) {
			tiles[start][n] = terrain;
		}
		// first line(border)
		for (size_t n = start + 1; n < tiles.size() - start; n ++) {
			tiles[n][start] = terrain;
		}
	}

	if (back) {
		// last column(border)
		for (size_t n = start; n < tiles[0].size() - start; n ++) {
			tiles[tiles.size() - start - 1][n] = terrain;
		}
		// last line(border)
		for (size_t n = start; n < tiles.size() - start - 1; n ++) {
			tiles[n][tiles[0].size() - start - 1] = terrain;
		}
	}
}

std::string generate_map_data(int width, int height, bool colorful)
{
	VALIDATE(width > 0 && height > 0, "map must not be empty!");

	const t_translation::t_terrain normal = t_translation::read_terrain_code("Gg");
	const t_translation::t_terrain border = t_translation::read_terrain_code("Gs");
	const t_translation::t_terrain control = t_translation::read_terrain_code("Gd");
	const t_translation::t_terrain forbidden = t_translation::read_terrain_code("Gll");

	t_translation::t_map tiles(width + 2, t_translation::t_list(height + 2, normal));
	if (colorful) {
		square_fill_frame(tiles, 0, border, true, true);
		square_fill_frame(tiles, 1, control, true, false);

		const size_t border_size = 1;
		tiles[border_size][border_size] = forbidden;
	}

	// tiles[border_size][tiles[0].size() - border_size - 1] = forbidden;

	// tiles[tiles.size() - border_size - 1][border_size] = forbidden;
	// tiles[tiles.size() - border_size - 1][tiles[0].size() - border_size - 1] = forbidden;

	std::string str = gamemap::default_map_header + t_translation::write_game_map(t_translation::t_map(tiles));

	return str;
}

int chart_controller::original_width = 12;
int chart_controller::original_height = 16;

chart_controller::chart_controller(const config &top_config, CVideo& video, bool snore)
	: controller_base(SDL_GetTicks(), top_config, video)
	, gui_(NULL)
	, map_(top_config, null_str)
	, units_(*this, map_, false)
	, snore_(snore)
	, do_quit_(false)
{
	map_ = gamemap(top_config, generate_map_data(original_width, original_height, snore_));

	units_.create_coor_map(map_.w(), map_.h());

	init_gui(video);
	init_music(top_config);
}

void chart_controller::init_gui(CVideo& video)
{
	const config &theme_cfg = get_theme(game_config_, "chart");
	gui_ = new chart_display(*this, units_, video, map_, theme_cfg, config());
}

void chart_controller::init_music(const config& game_config)
{
	const config &cfg = game_config.child("editor_music");
	if (!cfg) {
		return;
	}
	BOOST_FOREACH (const config &i, cfg.child_range("music")) {
		sound::play_music_config(i);
	}
	sound::commit_music_changes();
}


chart_controller::~chart_controller()
{
	if (gui_) {
		delete gui_;
		gui_ = NULL;
	}
}

void chart_controller::main_loop()
{
	try {
		while (!do_quit_) {
			play_slice();
		}
	} catch (twml_exception& e) {
		e.show(gui());
	}
}

events::mouse_handler_base& chart_controller::get_mouse_handler_base()
{
	return *this;
}

void chart_controller::execute_command2(int command, const std::string& sparam)
{
	using namespace gui2;
	unit* u = units_.find_unit(selected_hex_);

	switch (command) {
		case tchart_theme::HOTKEY_RETURN:
			goto_title();
			break;
/*
		case tmkwin_theme::HOTKEY_GRID:

			break;

		case tmkwin_theme::HOTKEY_RCLICK:
			do_right_click();
			break;

		case tmkwin_theme::HOTKEY_SWITCH:
			if (!theme_) {
				break;
			}
			if (!current_unit_) {
				if (u && u->has_child()) {
					theme_into_widget(u);
				}
			} else {
				theme_goto_top(true);
			}
			break;

		case tmkwin_theme::HOTKEY_INSERT_TOP:
			insert_row(u, true);
			break;
		case tmkwin_theme::HOTKEY_INSERT_BOTTOM:
			insert_row(u, false);
			break;
		case tmkwin_theme::HOTKEY_ERASE_ROW:
			erase_row(u);
			break;

		case tmkwin_theme::HOTKEY_INSERT_LEFT:
			insert_column(u, true);
			break;
		case tmkwin_theme::HOTKEY_INSERT_RIGHT:
			insert_column(u, false);
			break;
		case tmkwin_theme::HOTKEY_ERASE_COLUMN:
			erase_column(u);
			break;

		case tmkwin_theme::HOTKEY_SETTING:
			widget_setting(false);
			break;
		case tmkwin_theme::HOTKEY_LINKED_GROUP:
			linked_group_setting();
			break;
		case tmkwin_theme::HOTKEY_SPECIAL_SETTING:
			widget_setting(true);
			break;

		case HOTKEY_COPY:
			copy_widget(u, false);
			break;
		case HOTKEY_CUT:
			copy_widget(u, true);
			break;
		case HOTKEY_PASTE:
			paste_widget();
			break;
		case tmkwin_theme::HOTKEY_ERASE:
			erase_widget();
			break;

		case tmkwin_theme::HOTKEY_INSERT_CHILD:
			u->insert_child(default_child_width, default_child_height);
			if (!in_theme_top()) {
				layout_dirty();
			}
			break;
		case tmkwin_theme::HOTKEY_ERASE_CHILD:
			u->parent().u->erase_child(u->parent().number);
			if (!in_theme_top()) {
				layout_dirty();
			}
			break;

		case tmkwin_theme::HOTKEY_UNPACK:
			unpack_widget_tpl(u);
			break;

		case tmkwin_theme::HOTKEY_RUN:
			run();
			break;
*/
		case HOTKEY_SYSTEM:
			break;

		default:
			controller_base::execute_command2(command, sparam);
	}
}

void chart_controller::goto_title()
{
	do_quit_ = true;
}

void chart_controller::mouse_motion(int x, int y, const bool browse, bool update)
{
	mouse_handler_base::mouse_motion(x, y, browse, update);

	if (mouse_handler_base::mouse_motion_default(x, y, update)) return;
	map_location hex_clicked = gui_->hex_clicked_on(x, y);
	last_hex_ = hex_clicked;

	gui_->highlight_hex(hex_clicked);

	if (cursor::get() != cursor::WAIT) {
		if (!last_hex_.x || !last_hex_.y || !map_.on_board(last_hex_)) {
			// cursor::set(cursor::INTERIOR);
			cursor::set(cursor::NORMAL);
			gui_->set_mouseover_hex_overlay(NULL);

		} else {
			// no selected unit or we can't move it
			cursor::set(cursor::NORMAL);
		}
	}
}

bool chart_controller::left_click(int x, int y, const bool browse)
{
	if (mouse_handler_base::left_click(x, y, browse)) {
		return false;
	}
	if (!map_.on_board(last_hex_)) {
		return false;
	}
	
	return true;
}

bool chart_controller::right_click(int x, int y, const bool browse)
{
	if (mouse_handler_base::right_click(x, y, browse)) {
		const SDL_Rect& rect = gui_->map_outside_area();
		if (!point_in_rect(x, y, rect)) {
			return false;
		}
	}

	do_right_click();
	
	return false;
}

void chart_controller::do_right_click()
{
	selected_hex_ = map_location();
}

void chart_controller::post_change_resolution()
{
}

bool chart_controller::toggle_report(gui2::twidget* widget)
{
	long index = reinterpret_cast<long>(widget->cookie());

	gui2::tchart_theme* theme = dynamic_cast<gui2::tchart_theme*>(gui_->get_theme());
	if (index == 0) {
		
	} else if (index == 1) {
		
	}
	return true;
}

bool chart_controller::in_context_menu(const std::string& id) const
{
	using namespace gui2;
	std::pair<std::string, std::string> item = gui2::tcontext_menu::extract_item(id);
	int command = hotkey::get_hotkey(item.first).get_id();

	const unit* u = units_.find_unit(selected_hex_, true);

	switch(command) {
	// idle section
	case HOTKEY_ZOOM_IN:
	case HOTKEY_ZOOM_OUT:
	case HOTKEY_SYSTEM:
		return !selected_hex_.valid();
/*
	case tmkwin_theme::HOTKEY_SETTING: // setting
		return u && (u->type() != unit::WINDOW || (!u->parent().u || u->parent().u->is_stacked_widget()));
	case tmkwin_theme::HOTKEY_SPECIAL_SETTING:
		return u && u->has_special_setting();

	case HOTKEY_COPY:
		return can_copy(u, false);
	case HOTKEY_CUT:
		return can_copy(u, true);
	case HOTKEY_PASTE:
		return can_paste(u);

	// unit
	case tmkwin_theme::HOTKEY_ERASE: // erase
		return can_erase(u);

	// window
	case tmkwin_theme::HOTKEY_LINKED_GROUP:
		return u && u->type() == unit::WINDOW && !u->parent().u;

	// row
	case tmkwin_theme::HOTKEY_INSERT_TOP:
	case tmkwin_theme::HOTKEY_INSERT_BOTTOM:
	case tmkwin_theme::HOTKEY_ERASE_ROW:
		return !in_theme_top() && u && u->type() == unit::ROW && can_adjust_row(u);

	// column
	case tmkwin_theme::HOTKEY_INSERT_LEFT:
	case tmkwin_theme::HOTKEY_INSERT_RIGHT:
	case tmkwin_theme::HOTKEY_ERASE_COLUMN:
		return !in_theme_top() && u && u->type() == unit::COLUMN && can_adjust_column(u);

	case tmkwin_theme::HOTKEY_INSERT_CHILD: // add a page
		return u && u->is_stacked_widget();
	case tmkwin_theme::HOTKEY_ERASE_CHILD: // erase this page
		return u && u->type() == unit::WINDOW && u->parent().u && u->parent().u->is_stacked_widget() && u->parent().u->children().size() >= 2;

	case tmkwin_theme::HOTKEY_UNPACK:
		return u && u->is_tpl();
*/
	default:
		return false;
	}

	return false;
}

bool chart_controller::actived_context_menu(const std::string& id) const
{
	using namespace gui2;
	std::pair<std::string, std::string> item = gui2::tcontext_menu::extract_item(id);
	int command = hotkey::get_hotkey(item.first).get_id();

	const unit* u = units_.find_unit(selected_hex_, true);
	if (!u) {
		return true;
	}
/*
	const unit::tparent& parent = u->parent();
	const unit::tchild& child = parent.u? parent.u->child(parent.number): top_;

	switch(command) {
	// row
	case tmkwin_theme::HOTKEY_ERASE_ROW:
		return child.rows.size() >= 2;

	// column
	case tmkwin_theme::HOTKEY_ERASE_COLUMN:
		return child.cols.size() >= 2;

	}
*/
	return true;
}