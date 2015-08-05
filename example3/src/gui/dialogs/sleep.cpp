/* $Id: title_screen.cpp 48740 2011-03-05 10:01:34Z mordante $ */
/*
   Copyright (C) 2008 - 2011 by Mark de Wever <koraq@xs4all.nl>
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

#include "gui/dialogs/sleep.hpp"

#include "display.hpp"
#include "game_config.hpp"
#include "preferences.hpp"
#include "gettext.hpp"
#include "gui/auxiliary/timer.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/progress_bar.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/text_box.hpp"
#include "gui/widgets/report.hpp"
#include "gui/widgets/stacked_widget.hpp"
#include "gui/widgets/window.hpp"
#include "preferences_display.hpp"
#include "help.hpp"
#include "version.hpp"
#include <hero.hpp>
#include <time.h>
#include "sound.hpp"

#include <boost/bind.hpp>

#include <algorithm>

namespace gui2 {

/*WIKI
 * @page = GUIWindowDefinitionWML
 * @order = 2_title_screen
 *
 * == Title screen ==
 *
 * This shows the title screen.
 *
 * @begin{table}{dialog_widgets}
 * tutorial & & button & m &
 *         The button to start the tutorial. $
 *
 * campaign & & button & m &
 *         The button to start a campaign. $
 *
 * multiplayer & & button & m &
 *         The button to start multiplayer mode. $
 *
 * load & & button & m &
 *         The button to load a saved game. $
 *
 * editor & & button & m &
 *         The button to start the editor. $
 *
 * addons & & button & m &
 *         The button to start managing the addons. $
 *
 * language & & button & m &
 *         The button to select the game language. $
 *
 * credits & & button & m &
 *         The button to show Wesnoth's contributors. $
 *
 * quit & & button & m &
 *         The button to quit Wesnoth. $
 *
 * tips & & multi_page & m &
 *         A multi_page to hold all tips, when this widget is used the area of
 *         the tips doesn't need to be resized when the next or previous button
 *         is pressed. $
 *
 * -tip & & label & o &
 *         Shows the text of the current tip. $
 *
 * -source & & label & o &
 *         The source (the one who's quoted or the book referenced) of the
 *         current tip. $
 *
 * next_tip & & button & m &
 *         The button show the next tip of the day. $
 *
 * previous_tip & & button & m &
 *         The button show the previous tip of the day. $
 *
 * logo & & progress_bar & o &
 *         A progress bar to "animate" the Wesnoth logo. $
 *
 * revision_number & & control & o &
 *         A widget to show the version number when the version number is
 *         known. $
 *
 * @end{table}
 */

REGISTER_DIALOG(sleep)

tsleep::tsleep(display& disp, hero& player_hero)
	: disp_(disp)
	, navigate_(NULL)
	, player_hero_(player_hero)
	, window_(NULL)
{
}

void tsleep::post_build(CVideo& video, twindow& window)
{
}

void tsleep::pre_show(CVideo& video, twindow& window)
{
	window_ = &window;

	set_restore(false);
	window.set_click_dismiss(false);
	window.set_escape_disabled(true);

	// prepare navigate bar.
	std::vector<std::string> labels;
	labels.push_back(_("Sleep Test"));
	labels.push_back(_("History"));
	labels.push_back(_("Preferences"));
	labels.push_back(_("Login"));

	navigate_ = find_widget<treport>(&window, "navigate", false, true);
	navigate_->tabbar_init(true, "tab");
	navigate_->set_boddy(find_widget<twidget>(&window, "main_panel", false, true));
	int index = 0;
	for (std::vector<std::string>::const_iterator it = labels.begin(); it != labels.end(); ++ it) {
		tcontrol* widget = navigate_->create_child(null_str, null_str, reinterpret_cast<void*>(index ++));
		widget->set_label(*it);
		navigate_->insert_child(*widget);
	}
	navigate_->select(TITLE_PAGE);
	navigate_->replacement_children();

	pre_title(window);

	main_layers_ = find_widget<tstacked_widget>(&window, "main_layers", false, true);
	main_layers_->set_radio_layer(TITLE_PAGE);
	
	tlobby::thandler::join();
}

void tsleep::post_show(twindow& window)
{
}

void tsleep::pre_title(twindow& window)
{
	std::stringstream strstr;

	tlabel* label = find_widget<tlabel>(&window, "top_tip", true, true);
	label->set_label(_("Kindly reminder: Please put it under your pillow, Turn off network mode!"));

	if (!game_config::images::game_title.empty()) {
		window.canvas()[0].set_variable("background_image",	variant(game_config::images::game_title));
	}

	/***** Set the logo *****/
	tcontrol* logo = find_widget<tcontrol>(&window, "logo", false, false);
	if (logo) {
		logo->set_label(game_config::logo_png);
	}

	tbutton* b = find_widget<tbutton>(&window, "player", false, false);
	b->set_canvas_variable("image", variant("icons/sleep.png"));

	b = find_widget<tbutton>(&window, "chart", false, true);
	connect_signal_mouse_left_click(
		*b
		, boost::bind(
			&tsleep::set_retval
			, this
			, boost::ref(window)
			, (int)SNORE_CHART));
}

bool tsleep::handle(int tag, tsock::ttype type, const config& data)
{
	if (tag != tlobby::tag_chat) {
		return false;
	}

	if (type != tsock::t_data) {
		return false;
	}
	if (const config& c = data.child("whisper")) {
		tbutton* b = find_widget<tbutton>(window_, "message", false, false);
		if (b->label().empty()) {
			b->set_label("misc/red-dot12.png");
		}
		sound::play_UI_sound(game_config::sounds::receive_message);
	}
	return false;
}

void tsleep::toggle_report(twidget* widget)
{
	treport* report = treport::get_report(widget);

	int page = (int)reinterpret_cast<long>(widget->cookie());
	main_layers_->set_radio_layer(page);

	tdialog::toggle_report(widget);
}

void tsleep::set_retval(twindow& window, int retval)
{
	window.set_retval(retval);
}

} // namespace gui2

