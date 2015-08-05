/* $Id: mkwin_display.cpp 47082 2010-10-18 00:44:43Z shadowmaster $ */
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
#define GETTEXT_DOMAIN "wesnoth-sleep"

#include "chart_display.hpp"
#include "chart_controller.hpp"
#include "unit_map.hpp"
#include "gui/dialogs/chart_theme.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/report.hpp"
#include "game_config.hpp"
#include "formula_string_utils.hpp"

#include <boost/bind.hpp>

chart_display::chart_display(chart_controller& controller, unit_map& units, CVideo& video, const gamemap& map,
		const config& theme_cfg, const config& level)
	: display(game_config::tile_square, &controller, video, &map, theme_cfg, level, gui2::tchart_theme::NUM_REPORTS)
	, controller_(controller)
	, units_(units)
{
	min_zoom_ = 48;
	max_zoom_ = 144;

	show_hover_over_ = false;

	// must use grid.
	set_grid(true);

	SDL_Rect screen = screen_area();
	std::string patch = get_theme_patch();
	theme_current_cfg_ = theme::set_resolution(theme_cfg, screen, patch, theme_cfg_);
	create_theme();
}

std::string chart_display::get_theme_patch() const
{
	std::string id;
	return id;
}

gui2::ttheme* chart_display::create_theme_dlg(const config& cfg)
{
	return new gui2::tchart_theme(*this, controller_, cfg);
}

chart_display::~chart_display()
{
}

void chart_display::pre_change_resolution(std::map<const std::string, bool>& actives)
{
}

void chart_display::post_change_resolution(const std::map<const std::string, bool>& actives)
{
	controller_.post_change_resolution();
}

void chart_display::pre_draw()
{
}

void chart_display::draw_hex(const map_location& loc)
{
	display::draw_hex(loc);
}

void chart_display::draw_sidebar()
{
	// Fill in the terrain report
	if (map_->on_board_with_border(mouseoverHex_)) {
		refresh_report(gui2::tchart_theme::POSITION, reports::report(reports::report::LABEL, lexical_cast<std::string>(mouseoverHex_), null_str));
	}
	std::stringstream ss;
	ss << zoom_ << "(" << int(get_zoom_factor() * 100) << "%)";
	refresh_report(gui2::tchart_theme::ZOOM, reports::report(reports::report::LABEL, ss.str(), null_str));
}

void chart_display::set_mouse_overlay(surface& image_fg)
{
	if (!image_fg) {
		set_mouseover_hex_overlay(NULL);
		return;
	}

	// Create a transparent surface of the right size.
	surface image = create_compatible_surface(image_fg, image_fg->w, image_fg->h);
	sdl_fill_rect(image, NULL, SDL_MapRGBA(image->format, 0, 0, 0, 0));

	// For efficiency the size of the tile is cached.
	// We assume all tiles are of the same size.
	// The zoom factor can change, so it's not cached.
	// NOTE: when zooming and not moving the mouse, there are glitches.
	// Since the optimal alpha factor is unknown, it has to be calculated
	// on the fly, and caching the surfaces makes no sense yet.
	static const Uint8 alpha = 196;
	static const int half_size = zoom_ / 2;
	static const int offset = 2;
	static const int new_size = half_size - 2;

	// Blit left side
	image_fg = scale_surface(image_fg, new_size, new_size);
	SDL_Rect rcDestLeft = create_rect(offset, offset, 0, 0);
	sdl_blit ( image_fg, NULL, image, &rcDestLeft );

	// Add the alpha factor and scale the image
	image = adjust_surface_alpha(image, alpha);

	// Set as mouseover
	set_mouseover_hex_overlay(image);
}

void chart_display::post_zoom()
{
	int ii = 0;
	return;

	double factor = double(zoom_) / double(last_zoom);
	SDL_Rect new_rect;
	// don't use units_, set_rect maybe change map_'s sequence.
	std::vector<unit*> top_units;
	for (unit_map::const_iterator it = units_.begin(); it != units_.end(); ++ it) {
		unit* u = dynamic_cast<unit*>(&*it);
		top_units.push_back(u);
	}
	for (std::vector<unit*>::const_iterator it = top_units.begin(); it != top_units.end(); ++ it) {
		unit* u = *it;
		const SDL_Rect& rect = u->get_rect();
		new_rect = create_rect(rect.x * factor, rect.y * factor, rect.w * factor, rect.h * factor);
		u->set_rect(new_rect);
	}
}

void chart_display::pre_draw(rect_of_hexes& hexes)
{
	// generate all units in current win.
	draw_area_unit_size_ = units_.units_from_rect(draw_area_unit_, draw_area_rect_);
}

void chart_display::draw_invalidated()
{
	std::vector<map_location> unit_invals;

	for (size_t i = 0; i < draw_area_unit_size_; i ++) {
		const base_unit* u = draw_area_unit_[i];

		const std::set<map_location>& draw_locs = u->get_draw_locations();
		invalidate(draw_locs);

		const map_location& loc = u->get_location();
		// draw_area_val(loc.x, loc.y) = INVALIDATE_UNIT;
		unit_invals.push_back(loc);
	}

	display::draw_invalidated();
	redraw_units(unit_invals);
}

static std::string miss_anim_err_str = "logic can only process map or canvas animation!";

void chart_display::redraw_units(const std::vector<map_location>& invalidated_unit_locs)
{
	// Units can overlap multiple hexes, so we need
	// to redraw them last and in the good sequence.
	BOOST_FOREACH (const map_location& loc, invalidated_unit_locs) {
		base_unit* u = units_.find_base_unit(loc, false);
		if (u) {
			u->redraw_unit();
		}
		u = units_.find_base_unit(loc, true);
		if (u) {
			u->redraw_unit();
		}
	}
	for (std::map<int, animation*>::iterator it = area_anims_.begin(); it != area_anims_.end(); ++ it) {
		tanim_type type = it->second->type();
		VALIDATE(type == anim_map || type == anim_canvas, miss_anim_err_str);
		if (type != anim_map) {
			continue;
		}

		it->second->update_last_draw_time();
		anim2::rt.type = it->second->type();
		it->second->redraw(screen_.getSurface(), empty_rect);
	}
}