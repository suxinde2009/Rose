/* Require Rose v0.0.6 or above. $ */

#define GETTEXT_DOMAIN "wesnoth-lib"

#include "gui/dialogs/example2.hpp"

#include "display.hpp"
#include "formula_string_utils.hpp"
#include "gettext.hpp"
#include "preferences.hpp"
#include "preferences_display.hpp"

#include "gui/dialogs/helper.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/text_box.hpp"
#include "gui/widgets/scroll_text_box.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/stacked_widget.hpp"

#include <boost/bind.hpp>

namespace gui2 {

REGISTER_DIALOG(example2)

texample2::texample2(display& disp, bool first)
	: tchat_(disp, CHAT_PAGE)
	, disp_(disp)
	, start_page_(first? CHAT_PAGE: PREFERENCES_PAGE)
	, navigate_(true, false, "tab")
{
}

void texample2::pre_show(CVideo& /*video*/, twindow& window)
{
	// set backgroup image.
	window.canvas()[0].set_variable("background_image",	variant("dialogs/opaque-background.png"));

	// prepare navigate bar.
	std::vector<std::string> labels;
	labels.push_back(_("Chat"));
	labels.push_back(_("Preferences"));

	navigate_.set_report(find_widget<treport>(&window, "navigate", false, true));
	int index = 0;
	for (std::vector<std::string>::const_iterator it = labels.begin(); it != labels.end(); ++ it) {
		tcontrol* widget = navigate_.create_child(null_str, null_str, reinterpret_cast<void*>(index ++), null_str);
		widget->set_label(*it);
		navigate_.insert_child(*widget);
	}
	navigate_.select(start_page_);
	navigate_.replacement_children();

	page_panel_ = find_widget<tstacked_widget>(&window, "panel", false, true);
	swap_page(window, start_page_, false);

	// preferences grid
	ttoggle_button* toggle = find_widget<ttoggle_button>(&window, "fullscreen_button", false, true);

#if (defined(__APPLE__) && TARGET_OS_IPHONE) || defined(ANDROID)
	toggle->set_visible(twidget::INVISIBLE);
	find_widget<tbutton>(&window, "video_mode_button", false).set_visible(twidget::INVISIBLE);

#else
	toggle->set_callback_state_change(boost::bind(&texample2::fullscreen_toggled, this, _1));
	toggle->set_value(preferences::fullscreen());

	connect_signal_mouse_left_click(
		find_widget<tbutton>(&window, "video_mode_button", false)
		, boost::bind(
			&texample2::video_mode_button
			, this
			, boost::ref(window)));
#endif

	connect_signal_mouse_left_click(
			find_widget<tbutton>(&window, "change_language_button", false)
			, boost::bind(
				&texample2::set_retval
				, this
				, boost::ref(window)
				, (int)CHANGE_LANGUAGE));
}

void texample2::toggle_tabbar(twidget* widget)
{
	ttabbar* tabbar = ttabbar::get_tabbar(widget);
	if (tabbar != &navigate_) {
		// tchat_ has private tabbar, let tchat_ process them.
		tchat_::toggle_tabbar(widget);
		return;
	}
	int page = (int)reinterpret_cast<long>(widget->cookie());
	swap_page(*widget->get_window(), page, true);

	tdialog::toggle_tabbar(widget);
}

void texample2::fullscreen_toggled(twidget* widget)
{
	ttoggle_button* toggle = dynamic_cast<ttoggle_button*>(widget);

	twindow* window = widget->get_window();
	window->set_retval(toggle->get_value()? preferences::MAKE_FULLSCREEN: preferences::MAKE_WINDOWED);
}

void texample2::video_mode_button(twindow& window)
{
	window.set_retval(preferences::CHANGE_RESOLUTION);
}

void texample2::set_retval(twindow& window, int retval)
{
	window.set_retval(retval);
}

}
