/* $Id: simple_item_selector.cpp 48440 2011-02-07 20:57:31Z mordante $ */
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

#include "gui/dialogs/simple_item_selector.hpp"

#include "gui/widgets/button.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/listbox.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"

#include <boost/foreach.hpp>

namespace gui2 {

/*WIKI
 * @page = GUIWindowDefinitionWML
 * @order = 2_simple_item_selector
 *
 * == Simple item selector ==
 *
 * A simple one-column listbox with OK and Cancel buttons.
 *
 * @begin{table}{dialog_widgets}
 *
 * title & & label & m &
 *         Dialog title label. $
 *
 * message & & control & m &
 *         Text label displaying a description or instructions. $
 *
 * listbox & & listbox & m &
 *         Listbox displaying user choices. $
 *
 * -item & & control & m &
 *         Widget which shows a listbox item label. $
 *
 * ok & & button & m &
 *         OK button. $
 *
 * cancel & & button & m &
 *         Cancel button. $
 *
 * @end{table}
 */

REGISTER_DIALOG(simple_item_selector)

tsimple_item_selector::tsimple_item_selector(const std::string& title, const std::string& message, list_type const& items, bool title_uses_markup, bool message_uses_markup)
	: index_(-1)
	, title_(title)
	, msg_(message)
	, markup_title_(title_uses_markup)
	, markup_msg_(message_uses_markup)
	, single_button_(false)
	, items_(items)
	, ok_label_()
	, cancel_label_()
{
}

void tsimple_item_selector::pre_show(CVideo& /*video*/, twindow& window)
{
	tlabel& ltitle = find_widget<tlabel>(&window, "title", false);
	tcontrol& lmessage = find_widget<tcontrol>(&window, "message", false);
	tlistbox& list = find_widget<tlistbox>(&window, "listbox", false);
	window.keyboard_capture(&list);

	ltitle.set_label(title_);

	lmessage.set_label(msg_);

	BOOST_FOREACH (const tsimple_item_selector::item_type& it, items_) {
		std::map<std::string, string_map> data;
		string_map column;

		column["label"] = it;
		data.insert(std::make_pair("item", column));

		list.add_row(data);
	}

	if(index_ != -1 && index_ < list.get_item_count()) {
		list.select_row(index_);
	}

	index_ = -1;

	tbutton& button_ok = find_widget<tbutton>(&window, "ok", false);
	tbutton& button_cancel = find_widget<tbutton>(&window, "cancel", false);

	if(!ok_label_.empty()) {
		button_ok.set_label(ok_label_);
	}

	if(!cancel_label_.empty()) {
		button_cancel.set_label(cancel_label_);
	}

	if(single_button_) {
		button_cancel.set_visible(gui2::twidget::INVISIBLE);
	}
}

void tsimple_item_selector::post_show(twindow& window)
{
	if(get_retval() != twindow::OK) {
		return;
	}

	tlistbox& list = find_widget<tlistbox>(&window, "listbox", false);
	index_ = list.get_selected_row();
}

}
