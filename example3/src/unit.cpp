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

#include "unit.hpp"
#include "gettext.hpp"
#include "chart_display.hpp"
#include "chart_controller.hpp"
// #include "gui/dialogs/chart_theme.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/report.hpp"
#include "gui/auxiliary/window_builder/helper.hpp"
#include "game_config.hpp"

#include <boost/bind.hpp>


unit::~unit()
{
}

void unit::redraw_unit()
{
	if (!loc_.valid()) {
		return;
	}

	if (refreshing_) {
		return;
	}
	trefreshing_lock lock(*this);
	redraw_counter_ ++;

	const int xsrc = disp_.get_location_x(loc_);
	const int ysrc = disp_.get_location_y(loc_);

	int zoom = disp_.hex_width();
	surface surf = create_neutral_surface(zoom, zoom);

	if (surf) {
		disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT, loc_, xsrc, ysrc, surf);
	}

	int ellipse_floating = 0;
	if (loc_ == controller_.selected_hex()) {
		disp_.drawing_buffer_add(display::LAYER_UNIT_BG, loc_,
			xsrc, ysrc - ellipse_floating, image::get_image("misc/ellipse-top.png", image::SCALED_TO_ZOOM));
	
		disp_.drawing_buffer_add(display::LAYER_UNIT_FIRST, loc_,
			xsrc, ysrc - ellipse_floating, image::get_image("misc/ellipse-bottom.png", image::SCALED_TO_ZOOM));
	}
}

