/* $Id: title_screen.hpp 48740 2011-03-05 10:01:34Z mordante $ */
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

#ifndef GUI_DIALOGS_SLEEP_HPP_INCLUDED
#define GUI_DIALOGS_SLEEP_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"
#include "lobby.hpp"

class display;
class hero_map;
class hero;

namespace gui2 {

class treport;
class tstacked_widget;

/**
 * This class implements the title screen.
 *
 * The menu buttons return a result back to the caller with the button pressed.
 * So at the moment it only handles the tips itself.
 *
 * @todo Evaluate whether we can handle more buttons in this class.
 */
class tsleep : public tdialog, public tlobby::thandler
{
public:
	enum {TITLE_PAGE, HISTORY_PAGE, PREFERENCES_PAGE, LOGIN_PAGE};

	tsleep(display& disp, hero& player_hero);

	/**
	 * Values for the menu-items of the main menu.
	 *
	 * @todo Evaluate the best place for these items.
	 */
	enum tresult {
			EDIT_DIALOG = 1     /**< Let user select a campaign to play */
			, PLAYER
			, SNORE_CHART
			, MOTION_CHART
			, CHANGE_LANGUAGE
			, EDIT_PREFERENCES
			, QUIT_GAME
			, NOTHING             /**<
			                       * Default, nothing done, no redraw needed
			                       */
			};

	bool handle(int tag, tsock::ttype type, const config& data);
private:

	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	/** Inherited from tdialog. */
	virtual void post_build(CVideo& video, twindow& window);

	/** Inherited from tdialog. */
	void pre_show(CVideo& video, twindow& window);

	void post_show(twindow& window);

	void pre_title(twindow& window);
	void toggle_report(twidget* widget);
	void set_retval(twindow& window, int retval);

private:
	display& disp_;

	hero& player_hero_;
	treport* navigate_;
	tstacked_widget* main_layers_;

	twindow* window_;
};

} // namespace gui2

#endif
