#define GETTEXT_DOMAIN "wesnoth-sleep"

#include "gui/dialogs/helper.hpp"
#include "gui/dialogs/chart_theme.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/report.hpp"
#include "gui/widgets/listbox.hpp"
#include "gui/widgets/stacked_widget.hpp"

#include "formula_string_utils.hpp"
#include "chart_display.hpp"
#include "chart_controller.hpp"
#include "hotkeys.hpp"

#include "gettext.hpp"

#include <boost/bind.hpp>

namespace gui2 {

tchart_theme::tchart_theme(chart_display& disp, chart_controller& controller, const config& cfg)
	: ttheme(disp, controller, cfg)
	, controller_(controller)
{
}

void tchart_theme::app_pre_show()
{
	// prepare status report.
	reports_.insert(std::make_pair(ZOOM, "zoom"));
	reports_.insert(std::make_pair(POSITION, "position"));

	// prepare hotkey
	hotkey::insert_hotkey(HOTKEY_RETURN, "return", _("Return to title"));
	hotkey::insert_hotkey(HOTKEY_STATUS, "status", _("Status"));
	hotkey::insert_hotkey(HOTKEY_GRID, "grid", _("Grid"));
	hotkey::insert_hotkey(HOTKEY_SWITCH, "switch", _("Go into"));
	hotkey::insert_hotkey(HOTKEY_RCLICK, "rclick", _("Right Click"));

	std::stringstream err;
	utils::string_map symbols;

	hotkey::insert_hotkey(HOTKEY_RUN, "validate", vgettext("wesnoth-lib", "Check for errors in the $window", symbols));
	hotkey::insert_hotkey(HOTKEY_SETTING, "settings", _("Settings"));
	hotkey::insert_hotkey(HOTKEY_SPECIAL_SETTING, "special_setting", _("Special Setting"));
	hotkey::insert_hotkey(HOTKEY_LINKED_GROUP, "linked_group", _("Linked group"));
	hotkey::insert_hotkey(HOTKEY_ERASE, "erase", _("Erase"));

	hotkey::insert_hotkey(HOTKEY_INSERT_TOP, "insert_top", _("Insert top"));
	hotkey::insert_hotkey(HOTKEY_INSERT_BOTTOM, "insert_bottom", _("Inert bottom"));
	hotkey::insert_hotkey(HOTKEY_ERASE_ROW, "erase_row", _("Erase row"));
	hotkey::insert_hotkey(HOTKEY_INSERT_LEFT, "insert_left", _("Insert left"));
	hotkey::insert_hotkey(HOTKEY_INSERT_RIGHT, "insert_right", _("Insert right"));
	hotkey::insert_hotkey(HOTKEY_ERASE_COLUMN, "erase_column", _("Erase column"));

	hotkey::insert_hotkey(HOTKEY_INSERT_CHILD, "insert_child", _("Insert child"));
	hotkey::insert_hotkey(HOTKEY_ERASE_CHILD, "erase_child", _("Erase child"));

	hotkey::insert_hotkey(HOTKEY_UNPACK, "unpack", _("Unpack"));

	tbutton* widget = dynamic_cast<tbutton*>(get_object("return"));
	click_generic_handler(*widget, null_str);

	widget = dynamic_cast<tbutton*>(get_object("zoomin"));
	click_generic_handler(*widget, null_str);

	widget = dynamic_cast<tbutton*>(get_object("zoomout"));
	click_generic_handler(*widget, null_str);
}

} //end namespace gui2
