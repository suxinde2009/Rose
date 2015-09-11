/* $Id: campaign_difficulty.cpp 49602 2011-05-22 17:56:13Z mordante $ */
/*
   Copyright (C) 2010 - 2011 by Ignacio Riquelme Morelle <shadowm2006@gmail.com>
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

#include "gui/dialogs/guide.hpp"

#include "gettext.hpp"

#include "display.hpp"
#include "gui/dialogs/helper.hpp"
#include "gui/widgets/listbox.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "gui/widgets/track.hpp"

#include <boost/bind.hpp>

namespace gui2 {

/*WIKI
 * @page = GUIWindowDefinitionWML
 * @order = 2_campaign_difficulty
 *
 * == Campaign difficulty ==
 *
 * The campaign mode difficulty menu.
 *
 * @begin{table}{dialog_widgets}
 *
 * title & & label & m &
 *         Dialog title label. $
 *
 * message & & scroll_label & o &
 *         Text label displaying a description or instructions. $
 *
 * listbox & & listbox & m &
 *         Listbox displaying user choices, defined by WML for each campaign. $
 *
 * -icon & & control & m &
 *         Widget which shows a listbox item icon, first item markup column. $
 *
 * -label & & control & m &
 *         Widget which shows a listbox item label, second item markup column. $
 *
 * -description & & control & m &
 *         Widget which shows a listbox item description, third item markup column. $
 *
 * @end{table}
 */

tguide_::tguide_(display& disp, const std::vector<std::string>& items, int retval)
	: disp_(disp)
	, items_(items)
	, retval_(retval)
	, cursel_(0)
	, can_exit_(false)
{
	VALIDATE(!items.empty(), "Must set items!");
}

void tguide_::pre_show(CVideo& /*video*/, twindow& window)
{
	ttrack* image = find_widget<ttrack>(&window, "image", false, true);
	image->set_canvas_variable("background_image", variant(null_str));
	image->set_callback_timer(boost::bind(&tguide::callback_timer, this, _1, _2, _3, _4, _5, twidget::npos));
	image->set_callback_control_drag_detect(boost::bind(&tguide_::callback_control_drag_detect, this, _1, _2, _3));
	image->set_callback_set_drag_coordinate(boost::bind(&tguide_::callback_set_drag_coordinate, this, _1, _2, _3));

	if (retval_ != twindow::NONE) {
		connect_signal_mouse_left_click(
			*image
			, boost::bind(
				&tguide::set_retval
				, this
				, boost::ref(window)
				, retval_));
	}
}

void tguide_::callback_timer(ttrack& widget, surface& frame_buffer, const tpoint& offset, int state2, bool from_timer, int drag_offset_x)
{
	if (from_timer && drag_offset_x == twidget::npos) {
		return;
	}
	if (!from_timer) {
		drag_offset_x = 0;
	}

	SDL_Rect widget_rect = widget.get_rect();

	if (from_timer) {
		sdl_blit(widget.background_surf(), NULL, frame_buffer, &widget_rect);
	}

	const int xsrc = offset.x + widget_rect.x;
	const int ysrc = offset.y + widget_rect.y;

	int left_sel = twidget::npos, right_sel = twidget::npos;
	if (drag_offset_x < 0) {
		left_sel = cursel_;
		if (cursel_ + 1 < (int)items_.size()) {
			right_sel = cursel_ + 1;
		}

	} else if (drag_offset_x > 0) {
		if (cursel_ > 0) {
			left_sel = cursel_ - 1;
		}
		right_sel = cursel_;

	} else {
		left_sel = cursel_;
	}
	
	std::string img;
	surface left_surf, right_surf;
	if (left_sel != twidget::npos) {
		img = items_[left_sel];
		std::map<std::string, surface>::const_iterator it = surfs_.find(img);
		if (it == surfs_.end()) {
			surface surf = scale_surface(image::get_image(img), widget_rect.w, widget_rect.h);
			it = surfs_.insert(std::make_pair(img, surf)).first;
		} 
		left_surf = it->second;
	}

	if (right_sel != twidget::npos) {
		img = items_[right_sel];
		std::map<std::string, surface>::const_iterator it = surfs_.find(img);
		if (it == surfs_.end()) {
			surface surf = scale_surface(image::get_image(img), widget_rect.w, widget_rect.h);
			it = surfs_.insert(std::make_pair(img, surf)).first;
		} 
		right_surf = it->second;
	}

	SDL_Rect src, dst;

	if (!drag_offset_x) {
		VALIDATE(left_sel != twidget::npos && right_sel == twidget::npos, null_str);
		dst = ::create_rect(xsrc, ysrc, 0, 0);
		sdl_blit(left_surf, NULL, frame_buffer, &dst);
		return;
	}

	const int abs_drag_offset_x = abs(drag_offset_x);
	if (left_surf) {
		if (drag_offset_x < 0) {
			src = ::create_rect(abs_drag_offset_x, 0, left_surf->w - abs_drag_offset_x, left_surf->h);
			dst = ::create_rect(xsrc, ysrc, 0, 0);
		} else {
			src = ::create_rect(left_surf->w - abs_drag_offset_x, 0, abs_drag_offset_x, left_surf->h);
			dst = ::create_rect(xsrc, ysrc, 0, 0);
		}

		sdl_blit(left_surf, &src, frame_buffer, &dst);
	}
	if (right_surf) {
		if (drag_offset_x < 0) {
			src = ::create_rect(0, 0, abs_drag_offset_x, right_surf->h);
			dst = ::create_rect(xsrc + left_surf->w - abs_drag_offset_x, ysrc, 0, 0);
		} else {
			src = ::create_rect(0, 0, right_surf->w - abs_drag_offset_x, right_surf->h);
			dst = ::create_rect(xsrc + abs_drag_offset_x, ysrc, 0, 0);
		}

		sdl_blit(right_surf, &src, frame_buffer, &dst);
	}
}

bool tguide_::callback_control_drag_detect(tcontrol* control, bool start, const twidget::tdrag_direction type)
{
	if (!start) {
		int drag_offset_x = control->last_drag_coordinate().x - control->first_drag_coordinate().x;

		if (abs(drag_offset_x) >= (int)control->get_width() / 3) {
			if (drag_offset_x < 0) {
				if (cursel_ + 1 < (int)items_.size()) {
					cursel_ ++;
				} else {
					can_exit_ = true;
				}
			} else if (drag_offset_x > 0 && cursel_ > 0) {
				cursel_ --;
			}
		}
		callback_set_drag_coordinate(control, control->last_drag_coordinate(), control->last_drag_coordinate());
	}

	return false;
}

bool tguide_::callback_set_drag_coordinate(tcontrol* control, const tpoint& first, const tpoint& last)
{
	int drag_offset_x = last.x - first.x;
	ttrack& widget = find_widget<ttrack>(control->get_window(), "image", false);
	callback_timer(widget, widget.get_frame_buffer(), widget.get_frame_offset(), widget.get_state2(), true, drag_offset_x);

	return false;
}

void tguide_::set_retval(twindow& window, int retval)
{
	if (!can_exit_) {
		return;
	}
	window.set_retval(retval);
}

REGISTER_DIALOG(guide)

tguide::tguide(display& disp, const std::vector<std::string>& items)
	: tguide_(disp, items, twindow::OK)
{}

void tguide::pre_show(CVideo& video, twindow& window)
{
	window.canvas()[0].set_variable("background_image",	variant("dialogs/white-background.png"));

	tguide_::pre_show(video, window);
}

}
