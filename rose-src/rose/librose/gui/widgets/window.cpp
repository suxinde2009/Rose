/* $Id: window.cpp 54906 2012-07-29 19:52:01Z mordante $ */
/*
   Copyright (C) 2007 - 2012 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/**
 *  @file
 *  Implementation of window.hpp.
 */

#define GETTEXT_DOMAIN "wesnoth-lib"

#include "gui/widgets/window.hpp"
#include "font.hpp"
#include "display.hpp"
#include "gettext.hpp"
#include "log.hpp"
#include "gui/auxiliary/event/distributor.hpp"
#include "gui/auxiliary/event/message.hpp"
#include "gui/auxiliary/log.hpp"
#include "gui/auxiliary/window_builder/control.hpp"
#include "gui/dialogs/tip.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/listbox.hpp"
#ifdef DEBUG_WINDOW_LAYOUT_GRAPHS
#include "gui/widgets/debug.hpp"
#endif
#include "gui/widgets/scrollbar_panel.hpp"
#include <preferences.hpp>
#include "preferences_display.hpp"
#include "video.hpp"
#include "formula_string_utils.hpp"
#include "hotkeys.hpp"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#define LOG_SCOPE_HEADER get_control_type() + " [" + id() + "] " + __func__
#define LOG_HEADER LOG_SCOPE_HEADER + ':'

#define LOG_IMPL_SCOPE_HEADER window.get_control_type() \
		+ " [" + window.id() + "] " + __func__
#define LOG_IMPL_HEADER LOG_IMPL_SCOPE_HEADER + ':'

extern int cached_draw_events;

namespace gui2{

namespace implementation {
/** @todo See whether this hack can be removed. */
// Needed to fix a compiler error in REGISTER_WIDGET.
class tbuilder_window
	: public tbuilder_control
{
public:
	tbuilder_window(const config& cfg)
		: tbuilder_control(cfg)
	{
	}

	twidget* build() const { return NULL; }
};

} // namespace implementation
REGISTER_WIDGET(window)

unsigned twindow::sunset_ = 0;

namespace {
#ifdef DEBUG_WINDOW_LAYOUT_GRAPHS
	const unsigned MANUAL = tdebug_layout_graph::MANUAL;
	const unsigned SHOW = tdebug_layout_graph::SHOW;
	const unsigned LAYOUT = tdebug_layout_graph::LAYOUT;
#else
	// values are irrelavant when DEBUG_WINDOW_LAYOUT_GRAPHS is not defined.
	const unsigned MANUAL = 0;
	const unsigned SHOW = 0;
	const unsigned LAYOUT = 0;
#endif

/**
 * The interval between draw events.
 *
 * When the window is shown this value is set, the callback function always
 * uses this value instead of the parameter send, that way the window can stop
 * drawing when it wants.
 */
static int draw_interval = 0;

/**
 * SDL_AddTimer() callback for the draw event.
 *
 * When this callback is called it pushes a new draw event in the event queue.
 *
 * @returns                       The new timer interval, 0 to stop.
 */
static Uint32 draw_timer(Uint32, void*)
{
	// 3 layer window scenario, if <=0, 3th window's display will error.
	if (cached_draw_events <= 0) {
		SDL_Event event;
		SDL_UserEvent data;

		data.type = DRAW_EVENT;
		data.code = 0;
		data.data1 = NULL;
		data.data2 = NULL;

		event.type = DRAW_EVENT;
		event.user = data;

		SDL_PushEvent(&event);

		cached_draw_events ++;
	}
	return draw_interval;
}

/**
 * SDL_AddTimer() callback for delay_event.
 *
 * @param event                   The event to push in the event queue.
 *
 * @return                        The new timer interval (always 0).
 */
static Uint32 delay_event_callback(const Uint32, void* event)
{
	SDL_PushEvent(static_cast<SDL_Event*>(event));
	return 0;
}

/**
 * Allows an event to be delayed a certain amount of time.
 *
 * @note the delay is the minimum time, after the time has passed the event
 * will be pushed in the SDL event queue, so it might delay more.
 *
 * @param event                   The event to delay.
 * @param delay                   The number of ms to delay the event.
 */
static void delay_event(const SDL_Event& event, const Uint32 delay)
{
	SDL_AddTimer(delay, delay_event_callback, new SDL_Event(event));
}

/**
 * Adds a SHOW_HELPTIP event to the SDL event queue.
 *
 * The event is used to show the helptip for the currently focussed widget.
 */
static bool helptip()
{
	DBG_GUI_E << "Pushing SHOW_HELPTIP_EVENT event in queue.\n";

	SDL_Event event;
	SDL_UserEvent data;

	data.type = SHOW_HELPTIP_EVENT;
	data.code = 0;
	data.data1 = NULL;
	data.data2 = NULL;

	event.type = SHOW_HELPTIP_EVENT;
	event.user = data;

	SDL_PushEvent(&event);

	return true;
}

/**
 * Small helper class to get an unique id for every window instance.
 *
 * This is used to send event to the proper window, this allows windows to post
 * messages to themselves and let them delay for a certain amount of time.
 */
class tmanager
{
	tmanager();
public:

	static tmanager& instance();

	void add(twindow& window);

	void remove(twindow& window);

	unsigned get_id(twindow& window);

	twindow* window(const unsigned id);

private:

	// The number of active window should be rather small
	// so keep it simple and don't add a reverse lookup map.
	std::map<unsigned, twindow*> windows_;
};

tmanager::tmanager()
	: windows_()
{
}

tmanager& tmanager::instance()
{
	static tmanager window_manager;
	return window_manager;
}

void tmanager::add(twindow& window)
{
	static unsigned id;
	++id;
	windows_[id] = &window;
}

void tmanager::remove(twindow& window)
{
	for(std::map<unsigned, twindow*>::iterator itor = windows_.begin();
			itor != windows_.end(); ++itor) {

		if(itor->second == &window) {
			windows_.erase(itor);
			return;
		}
	}
	assert(false);
}

unsigned tmanager::get_id(twindow& window)
{
	for(std::map<unsigned, twindow*>::iterator itor = windows_.begin();
			itor != windows_.end(); ++itor) {

		if(itor->second == &window) {
			return itor->first;
		}
	}
	assert(false);

	return 0;
}

twindow* tmanager::window(const unsigned id)
{
	std::map<unsigned, twindow*>::iterator itor = windows_.find(id);

	if(itor == windows_.end()) {
		return NULL;
	} else {
		return itor->second;
	}
}

} // namespace

twindow::twindow(CVideo& video,
		tformula<unsigned>x,
		tformula<unsigned>y,
		tformula<unsigned>w,
		tformula<unsigned>h,
		const bool automatic_placement,
		const unsigned horizontal_placement,
		const unsigned vertical_placement,
		const unsigned maximum_width,
		const unsigned maximum_height,
		const std::string& definition,
		const bool theme,
		const twindow_builder::tresolution::ttip& tooltip,
		const twindow_builder::tresolution::ttip& helptip)
	: tpanel()
	, cursor::setter(cursor::NORMAL)
	, video_(video)
	, status_(NEW)
	, show_mode_(none)
	, retval_(NONE)
	, owner_(0)
	, need_layout_(true)
	, variables_()
	, invalidate_layout_blocked_(false)
	, suspend_drawing_(true)
	, restorer_()
	, automatic_placement_(automatic_placement)
	, horizontal_placement_(horizontal_placement)
	, vertical_placement_(vertical_placement)
	, maximum_width_(maximum_width)
	, maximum_height_(maximum_height)
	, x_(x)
	, y_(y)
	, w_(w)
	, h_(h)
	, keep_rect_(::create_rect(-1, -1, -1, -1))
	, tooltip_(tooltip)
	, helptip_(helptip)
	, click_dismiss_(false)
	, enter_disabled_(false)
	, escape_disabled_(false)
	, linked_size_()
	, dirty_list_()
	, bg_opaque_(t_unset)
	, fix_coordinate_(theme)
#ifdef DEBUG_WINDOW_LAYOUT_GRAPHS
	, debug_layout_(new tdebug_layout_graph(this))
#endif
	, event_distributor_(new event::tdistributor(
			*this, event::tdispatcher::front_child))
{
	// We load the config in here as exception.
	// Our caller did update the screen size so no need for us to do that again.
	set_definition(definition);
	load_config();

	tmanager::instance().add(*this);

	connect();

	connect_signal<event::DRAW>(boost::bind(&twindow::draw, this));

	connect_signal<event::SDL_VIDEO_RESIZE>(
			  boost::bind(&twindow::signal_handler_sdl_video_resize
				  , this, _2, _3, _5));

	connect_signal<event::SDL_ACTIVATE>(
			  boost::bind(&event::tdistributor::initialize_state
				  , event_distributor_));

	connect_signal<event::SDL_LEFT_BUTTON_DOWN>(
			  boost::bind(
				  &twindow::signal_handler_click_dismiss, this, _2, _3, _4)
			, event::tdispatcher::front_child);
	connect_signal<event::SDL_MIDDLE_BUTTON_DOWN>(
			  boost::bind(
				  &twindow::signal_handler_click_dismiss, this, _2, _3, _4)
			, event::tdispatcher::front_child);
	connect_signal<event::SDL_RIGHT_BUTTON_DOWN>(
			  boost::bind(
				  &twindow::signal_handler_click_dismiss, this, _2, _3, _4)
			, event::tdispatcher::front_child);

	connect_signal<event::SDL_KEY_DOWN>(
			  boost::bind(&twindow::signal_handler_sdl_key_down
				  , this, _2, _3, _5)
			, event::tdispatcher::back_pre_child);
	connect_signal<event::SDL_KEY_DOWN>(
			  boost::bind(&twindow::signal_handler_sdl_key_down
				  , this, _2, _3, _5));

	connect_signal<event::MESSAGE_SHOW_TOOLTIP>(
			  boost::bind(
				  &twindow::signal_handler_message_show_tooltip
				, this
				, _2
				, _3
				, _5)
			, event::tdispatcher::back_pre_child);

	connect_signal<event::MESSAGE_SHOW_HELPTIP>(
			  boost::bind(
				  &twindow::signal_handler_message_show_helptip
				, this
				, _2
				, _3
				, _5)
			, event::tdispatcher::back_pre_child);

	connect_signal<event::REQUEST_PLACEMENT>(
			  boost::bind(
				  &twindow::signal_handler_request_placement
				, this
				, _2
				, _3)
			, event::tdispatcher::back_pre_child);

	register_hotkey(GLOBAL__HELPTIP, boost::bind(gui2::helptip));
}

twindow::~twindow()
{
	/*
	 * We need to delete our children here instead of waiting for the grid to
	 * automatically do it. The reason is when the grid deletes its children
	 * they will try to unregister them self from the linked widget list. At
	 * this point the member of twindow are destroyed and we enter UB. (For
	 * some reason the bug didn't trigger on g++ but it does on MSVC.
	 */
	for(unsigned row = 0; row < grid().get_rows(); ++row) {
		for(unsigned col = 0; col < grid().get_cols(); ++col) {
			grid().remove_child(row, col);
		}
	}

	/*
	 * The tip needs to be closed if the window closes and the window is
	 * not a tip. If we don't do that the tip will unrender in the next
	 * window and cause drawing glitches.
	 * Another issue is that on smallgui and an MP game the tooltip not
	 * unrendered properly can capture the mouse and make playing impossible.
	 */
	if(show_mode_ == modal) {
		tip::remove();
	}

	tmanager::instance().remove(*this);

#ifdef DEBUG_WINDOW_LAYOUT_GRAPHS

	delete debug_layout_;

#endif
	delete event_distributor_;
}

twindow* twindow::window_instance(const unsigned handle)
{
	return tmanager::instance().window(handle);
}

void twindow::update_screen_size()
{
	// Only if we're the toplevel window we need to update the size, otherwise
	// it's done in the resize event.
	if (draw_interval == 0) {
		const SDL_Rect rect = screen_area();
		settings::screen_width = rect.w;
		settings::screen_height = rect.h;

		settings::gamemap_width = settings::screen_width;
		settings::gamemap_height = settings::screen_height;

		display* disp = display::get_singleton();
		if (disp) {
			const unsigned w = disp->map_outside_area().w;
			const unsigned h = disp->map_outside_area().h;
			if(w && h) {
				settings::gamemap_width = w;
				settings::gamemap_height = h;
			}
		}
	}
}

twindow::tretval twindow::get_retval_by_id(const std::string& id)
{
/*WIKI
 * @page = GUIToolkitWML
 * @order = 3_widget_window_2
 *
 * List if the id's that have generate a return value:
 * * ok confirms the dialog.
 * * cancel cancels the dialog.
 *
 */
	// Note it might change to a map later depending on the number
	// of items.
	if(id == "ok") {
		return OK;
	} else if(id == "cancel") {
		return CANCEL;

	// default if nothing matched
	} else {
		return NONE;
	}
}

void twindow::show_tooltip(/*const unsigned auto_close_timeout*/)
{
	log_scope2(log_gui_draw, "Window: show as tooltip.");

	generate_dot_file("show", SHOW);

	assert(status_ == NEW);

	set_mouse_behaviour(event::tdispatcher::none);
	set_want_keyboard_input(false);

	show_mode_ = tooltip;

	/*
	 * Before show has been called, some functions might have done some testing
	 * on the window and called layout, which can give glitches. So
	 * reinvalidate the window to avoid those glitches.
	 */
	invalidate_layout();
	suspend_drawing_ = false;
}

void twindow::show_non_modal(/*const unsigned auto_close_timeout*/)
{
	log_scope2(log_gui_draw, "Window: show non modal.");

	generate_dot_file("show", SHOW);

	assert(status_ == NEW);

	set_mouse_behaviour(event::tdispatcher::hit);

	show_mode_ = modal;

	/*
	 * Before show has been called, some functions might have done some testing
	 * on the window and called layout, which can give glitches. So
	 * reinvalidate the window to avoid those glitches.
	 */
	invalidate_layout();
	suspend_drawing_ = false;
}

int twindow::show(const bool restore, const unsigned auto_close_timeout)
{
	/*
	 * Removes the old tip if one shown. The show_tip doesn't remove
	 * the tip, since it's the tip.
	 */
	tip::remove();

	show_mode_ = modal;

	/**
	 * Helper class to set and restore the drawing interval.
	 *
	 * We need to make sure we restore the value when the function ends, be it
	 * normally or due to an exception.
	 */
	class tdraw_interval_setter
	{
	public:
		tdraw_interval_setter()
			: interval_(draw_interval)
		{
			if (interval_ == 0) {
				cached_draw_events = 0;
				draw_interval = 30;
				SDL_AddTimer(draw_interval, draw_timer, NULL);

				// There might be some time between creation and showing so
				// reupdate the sizes.
				update_screen_size();

			}
		}

		~tdraw_interval_setter()
		{
			draw_interval = interval_;
		}
	private:

		int interval_;
	};

	log_scope2(log_gui_draw, LOG_SCOPE_HEADER);

	generate_dot_file("show", SHOW);

	// assert(status_ == NEW);

	tdraw_interval_setter draw_interval_setter;

	/*
	 * Before show has been called, some functions might have done some testing
	 * on the window and called layout, which can give glitches. So
	 * reinvalidate the window to avoid those glitches.
	 */
	invalidate_layout();
	suspend_drawing_ = false;

	if(auto_close_timeout) {
		// Make sure we're drawn before we try to close ourselves, which can
		// happen if the timeout is small.
		draw();

		SDL_Event event;
		SDL_UserEvent data;

		data.type = CLOSE_WINDOW_EVENT;
		data.code = tmanager::instance().get_id(*this);
		data.data1 = NULL;
		data.data2 = NULL;

		event.type = CLOSE_WINDOW_EVENT;
		event.user = data;

		delay_event(event, auto_close_timeout);
	}

	try {
		// Start our loop drawing will happen here as well.
		for (status_ = (status_ == REQUEST_CLOSE)? status_: SHOWING; status_ != REQUEST_CLOSE; ) {
			// process installed callback if valid, to allow e.g. network polling
			events::pump();
			// Add a delay so we don't keep spinning if there's no event.
			SDL_Delay(10);
		}
	} catch(...) {
		/**
		 * @todo Clean up the code duplication.
		 *
		 * In the future the restoring shouldn't be needed so the duplication
		 * doesn't hurt too much but keep this todo as a reminder.
		 */
		suspend_drawing_ = true;

		// restore area
		if (restore) {
			SDL_Rect rect = get_rect();
			sdl_blit(restorer_, 0, video_.getSurface(), &rect);
			font::undraw_floating_labels(video_.getSurface());
		}
		throw;
	}

	suspend_drawing_ = true;

	// restore area
	if (restore) {
		SDL_Rect rect = get_rect();
		sdl_blit(restorer_, 0, video_.getSurface(), &rect);
		font::undraw_floating_labels(video_.getSurface());
	}

	return retval_;
}

int twindow::asyn_show()
{
	bg_opaque_ = t_true;
	show_mode_ = modal;
	suspend_drawing_ = false;
	return retval_;
}

void twindow::draw()
{
	display::tcanvas_drawing_buffer_lock lock(*display::get_singleton());
	/***** ***** ***** ***** Init ***** ***** ***** *****/
	// Prohibited from drawing?
	if (suspend_drawing_) {
		return;
	}

	surface frame_buffer = video_.getSurface();

	/***** ***** Layout and get dirty list ***** *****/
	if (need_layout_) {
		VALIDATE(!fix_coordinate_, "layout must not be false during draw.");

		// Restore old surface. In the future this phase will not be needed
		// since all will be redrawn when needed with dirty rects. Since that
		// doesn't work yet we need to undraw the window.
		if (restorer_) {
			SDL_Rect rect = get_rect();
			sdl_blit(restorer_, 0, frame_buffer, &rect);
			// Since the old area might be bigger as the new one, invalidate
			// it.
		}

		layout();

		// Get new surface for restoring
		SDL_Rect rect = get_rect();
		// We want the labels underneath the window so draw them and use them
		// as restore point.
		font::draw_floating_labels(frame_buffer);
		restorer_ = get_surface_portion(frame_buffer, rect);

		// Need full redraw so only set ourselves dirty.
		add_to_dirty_list(std::vector<twidget*>(1, this));
	} else {

		// Let widgets update themselves, which might dirty some things.
		layout_children();

		// Now find the widgets that are dirty.
		std::vector<twidget*> call_stack;
		populate_dirty_list(*this, call_stack);
	}

	if (dirty_list_.empty()) {
		if (preferences::use_color_cursors() || sunset_) {
			surface frame_buffer = get_video_surface();

			if (sunset_) {
				/** @todo should probably be moved to event::thandler::draw. */
				static unsigned i = 0;
				if(++i % sunset_ == 0) {
					SDL_Rect r = ::create_rect(0, 0, frame_buffer->w, frame_buffer->h);
					const Uint32 color =
							SDL_MapRGBA(frame_buffer->format,0,0,0,255);

					fill_rect_alpha(r, color, 1, frame_buffer);
				}
			}
		}
		return;
	}

	BOOST_FOREACH(tdirty_list& list, dirty_list_) {

		std::vector<twidget*>& item = list.dirty;
		std::vector<twidget*>& children = list.children;

		twidget* terminal = item.back();

		const SDL_Rect dirty_rect = terminal->get_dirty_rect();

		clip_rect_setter clip(frame_buffer, &dirty_rect);
		/*
		 * The actual update routine does the following:
		 * - Restore the background.
		 *
		 * - draw [begin, end) the back ground of all widgets.
		 *
		 * - draw the children of the last item in the list, if this item is
		 *   a container it's children get a full redraw. If it's not a
		 *   container nothing happens.
		 *
		 * - draw [rbegin, rend) the fore ground of all widgets. For items
		 *   which have two layers eg window or panel it draws the foreground
		 *   layer. For other widgets it's a nop.
		 *
		 * Before drawing there needs to be determined whether a dirty widget
		 * really needs to be redrawn. If the widget doesn't need to be
		 * redrawing either being not VISIBLE or has status NOT_DRAWN. If
		 * it's not drawn it's still set not dirty to avoid it keep getting
		 * on the dirty list.
		 */

		for (std::vector<twidget*>::iterator itor = item.begin(); itor != item.end(); ++itor) {

			if ((**itor).get_visible() != twidget::VISIBLE
					|| (**itor).get_drawing_action() == twidget::NOT_DRAWN) {

				for (std::vector<twidget*>::iterator citor = itor; citor != item.end(); ++citor) {

					(**citor).set_dirty(false);
				}

				item.erase(itor, item.end());
				break;
			}
		}

		// Restore.
		if (bg_opaque_ != t_true) {
			SDL_Rect rect = get_rect();
			sdl_blit(restorer_, 0, frame_buffer, &rect);
		}

		/**
		 * @todo Remove the if an always use the true branch.
		 *
		 * When removing that code the draw functions with only a frame_buffer
		 * as parameter should also be removed since they will be unused from
		 * that moment.
		 */

		// Background.
		for (std::vector<twidget*>::iterator itor = item.begin(); itor != item.end(); ++ itor) {
			twidget* widget = *itor;

			if (bg_opaque_ != t_unset) {
				widget->draw_background(frame_buffer, 0, 0);

			} else {
				surface_is_opaque.set_request();
				widget->draw_background(frame_buffer, 0, 0);
				bg_opaque_ = surface_is_opaque.retval()? t_true: t_false;
			}
			if (widget == terminal || (!children.empty() && std::count(children.begin(), children.end(), widget))) {
				widget->draw_children(frame_buffer, 0, 0);
			}
		}

/*		// Children.
		if (!item.empty()) {
			item.back()->draw_children(frame_buffer, 0, 0);
		}
*/

		// Foreground.
		for (std::vector<twidget*>::reverse_iterator ritor = item.rbegin(); ritor != item.rend(); ++ritor) {
			twidget& widget = **ritor;
			widget.draw_foreground(frame_buffer, 0, 0);
			widget.set_dirty(false);
		}
	}

	dirty_list_.clear();
/*
	std::vector<twidget*> call_stack;
	populate_dirty_list(*this, call_stack);
	VALIDATE(dirty_list_.empty(), "twinodw::draw, has dirty control!");
*/
}

std::vector<tdirty_list>& twindow::dirty_list()
{
	return dirty_list_;
}

void twindow::set_keep_rect(int x, int y, int w, int h)
{ 
	keep_rect_ = ::create_rect(x, y, w, h);
}

void twindow::undraw()
{
	if(restorer_) {
		SDL_Rect rect = get_rect();
		sdl_blit(restorer_, 0, video_.getSurface(), &rect);
		// Since the old area might be bigger as the new one, invalidate
		// it.
	}
}

twindow::tinvalidate_layout_blocker::tinvalidate_layout_blocker(twindow& window)
	: window_(window)
	, invalidate_layout_blocked_(window_.invalidate_layout_blocked_)
{
	window_.invalidate_layout_blocked_ = true;
}

twindow::tinvalidate_layout_blocker::~tinvalidate_layout_blocker()
{
	window_.invalidate_layout_blocked_ = invalidate_layout_blocked_;
}

void twindow::invalidate_layout()
{
	if (!fix_coordinate_ && !invalidate_layout_blocked_) {
		need_layout_ = true;
	}
}

void twindow::init_linked_size_group(const std::string& id,
		const bool fixed_width, const bool fixed_height, bool radio)
{
	assert(fixed_width || fixed_height);
	assert(!has_linked_size_group(id));

	linked_size_[id] = tlinked_size(fixed_width, fixed_height, radio);
}

bool twindow::has_linked_size_group(const std::string& id)
{
	return linked_size_.find(id) != linked_size_.end();
}

void twindow::add_linked_widget(const std::string& id, twidget* widget)
{
	assert(widget);
	std::stringstream err;
	err << "not defiend group id: " << id;
	VALIDATE(has_linked_size_group(id), err.str());

	std::vector<twidget*>& widgets = linked_size_[id].widgets;
	if (std::find(widgets.begin(), widgets.end(), widget) == widgets.end()) {
		widgets.push_back(widget);
	}
}

void twindow::remove_linked_widget(const std::string& id, const twidget* widget)
{
	assert(widget);
	// assert(has_linked_size_group(id));
	if (!has_linked_size_group(id)) {
		return;
	}

	std::vector<twidget*>& widgets = linked_size_[id].widgets;

	std::vector<twidget*>::iterator itor =
			std::find(widgets.begin(), widgets.end(), widget);

	if (itor != widgets.end()) {
		widgets.erase(itor);

		assert(std::find(widgets.begin(), widgets.end(), widget) == widgets.end());
	}
}

void twindow::layout()
{
	/***** Initialize. *****/

	boost::intrusive_ptr<const twindow_definition::tresolution> conf =
		boost::dynamic_pointer_cast<const twindow_definition::tresolution>
		(config());
	assert(conf);

	log_scope2(log_gui_layout, LOG_SCOPE_HEADER);

	get_screen_size_variables(variables_);

	// for automatic_placement_ = false, variables_ can use (volatile_width)/(volatile_height) 
	variables_.add("volatile_width", variant(settings::screen_width));
	variables_.add("volatile_height", variant(settings::screen_height));
	variables_.add("keyboard_height", variant(settings::keyboard_height));

	int maximum_width;
	if (keep_rect_.x != -1) {
		maximum_width = keep_rect_.w;
	} else {
		maximum_width = automatic_placement_
			?  maximum_width_
				? std::min(maximum_width_, settings::screen_width)
				: settings::screen_width
			: w_(variables_);
	}

	int maximum_height;
	if (keep_rect_.x != -1 && !settings::keyboard_height) {
		maximum_height = keep_rect_.h;
	} else {
		maximum_height = automatic_placement_
			? maximum_height_
				? std::min(maximum_height_, settings::screen_height)
				: settings::screen_height
			: h_(variables_);
	}

	if (fix_coordinate_) {
		keep_rect_ = ::create_rect(0, 0, maximum_width, maximum_height);
	}

	/***** Layout. *****/
	layout_init(true);
	generate_dot_file("layout_init", LAYOUT);

	layout_linked_widgets(NULL);

	/***** Get the best location for the window *****/
	tpoint size = get_best_size();

	if (size.x > static_cast<int>(maximum_width) || size.y > static_cast<int>(maximum_height)) {
		// @todo implement the scrollbars on the window.

		std::stringstream sstr;
		sstr << __FILE__ << ":" << __LINE__ << " in function '" << __func__
				<< "' found the following problem: Failed to size window;"
				<< " wanted size " << get_best_size()
				<< " available size "
				<< maximum_width << ',' << maximum_height
				<< " screen size "
				<< settings::screen_width << ',' << settings::screen_height
				<< '.';

		throw twml_exception(tintegrate::generate_format(id(), "yellow") + _("Failed to show a dialog, "
				"which doesn't fit on the screen."), sstr.str());
	}

	tpoint origin(0, 0);

	if (keep_rect_.x != -1) {
		origin.x = keep_rect_.x;
		origin.y = keep_rect_.y;
		size.x = keep_rect_.w;
		if (!settings::keyboard_height) {
			size.y = keep_rect_.h;
		}

	} else if (automatic_placement_) {
		switch (horizontal_placement_) {
			case tgrid::HORIZONTAL_ALIGN_LEFT :
				// Do nothing
				break;
			case tgrid::HORIZONTAL_ALIGN_CENTER :
				origin.x = (settings::screen_width - size.x) / 2;
				break;
			case tgrid::HORIZONTAL_ALIGN_RIGHT :
				origin.x = settings::screen_width - size.x;
				break;
			default :
				assert(false);
		}
		switch (vertical_placement_) {
			case tgrid::VERTICAL_ALIGN_TOP :
				// Do nothing
				break;
			case tgrid::VERTICAL_ALIGN_CENTER :
				origin.y = (settings::screen_height - size.y) / 2;
				break;
			case tgrid::VERTICAL_ALIGN_BOTTOM :
				origin.y = settings::screen_height - size.y;
				break;
			default :
				assert(false);
		}
	} else {
		variables_.add("volatile_width", variant(size.x));
		variables_.add("volatile_height", variant(size.y));

		size.x = w_(variables_);
		size.y = h_(variables_);

		variables_.add("width", variant(size.x));
		variables_.add("height", variant(size.y));

		origin.x = x_(variables_);
		origin.y = y_(variables_);
	}

	if (!fix_coordinate_) {
		/***** Set the window size *****/
		place(origin, size);

	} else {
		tcontrol::place(tpoint(0, 0), tpoint(maximum_width, maximum_height));
		grid().place_fix(tpoint(0, 0), tpoint(maximum_width, maximum_height));
	}

	generate_dot_file("layout_finished", LAYOUT);
	need_layout_ = false;

	event::init_mouse_location();
}

std::vector<twidget*> twindow::set_fix_coordinate(const SDL_Rect& map_area)
{
	fix_coordinate_ = true;

	std::vector<twidget*> result;
	for (tgrid::iterator it = grid().begin(); it != grid().end(); ++ it) {
		twidget* widget = *it;
		if (rects_overlap(widget->fix_rect(), map_area)) {
			result.push_back(widget);
			widget->set_volatile(true);
		}
	}
	return result;
}

tpoint twindow::calculate_best_size() const
{
	if (!fix_coordinate_) {
		return tcontainer_::calculate_best_size();
	} else {
		grid().calculate_best_size_fix();
		return tpoint(keep_rect_.w, keep_rect_.h);
	}
}

void twindow::layout_linked_widgets(const twidget* parent)
{
	// evaluate the group sizes
	std::map<const twidget*, tpoint> cache;
	typedef std::pair<const std::string, tlinked_size> hack;
	BOOST_FOREACH(hack& linked_size, linked_size_) {

		tpoint max_size(0, 0);
		cache.clear();

		// Determine the maximum size.
		BOOST_FOREACH(twidget* widget, linked_size.second.widgets) {

			if (widget->get_visible() == twidget::INVISIBLE) {
				continue;
			}

			const tpoint size = widget->get_best_size();
			cache.insert(std::make_pair(widget, size));

			if (size.x > max_size.x) {
				max_size.x = size.x;
			}
			if (size.y > max_size.y) {
				max_size.y = size.y;
			}
		}

		// Set the maximum size.
		BOOST_FOREACH(twidget* widget, linked_size.second.widgets) {

			if (widget->get_visible() == twidget::INVISIBLE) {
				continue;
			}

			// tpoint size = widget->get_best_size();
			tpoint size = cache.find(widget)->second;

			if (linked_size.second.width) {
				size.x = max_size.x;
			}
			if (linked_size.second.height) {
				size.y = max_size.y;
			}

			if (parent) {
				const twidget* tmp = widget;
				do {
					if (tmp == parent) {
						break;
					}
					tmp = tmp->parent_;
				} while (tmp);
				if (tmp != parent) {
					continue;
				}
			}
			widget->set_layout_size(size);
		}
	}
}

bool twindow::click_dismiss()
{
	if(does_click_dismiss()) {
		set_retval(OK);
		return true;
	}
	return false;
}

const std::string& twindow::get_control_type() const
{
	static const std::string type = "window";
	return type;
}

void twindow::draw(surface& /*surf*/, const bool /*force*/,
		const bool /*invalidate_background*/)
{
	assert(false);
}

namespace {

/**
 * Swaps an item in a grid for another one.*/
void swap_grid(tgrid* grid,
		tgrid* content_grid, twidget* widget, const std::string& id)
{
	assert(content_grid);
	assert(widget);

	// Make sure the new child has same id.
	widget->set_id(id);

	// Get the container containing the wanted widget.
	tgrid* parent_grid = NULL;
	if(grid) {
		parent_grid = find_widget<tgrid>(grid, id, false, false);
	}
	if(!parent_grid) {
		parent_grid = find_widget<tgrid>(content_grid, id, true, false);
		assert(parent_grid);
	}
	if(tgrid* g = dynamic_cast<tgrid*>(parent_grid->parent())) {
		widget = g->swap_child(id, widget, false);
	} else if(tcontainer_* c
			= dynamic_cast<tcontainer_*>(parent_grid->parent())) {

		widget = c->grid().swap_child(id, widget, true);
	} else {
		assert(false);
	}

	assert(widget);

	delete widget;
}

} // namespace

void twindow::finalize(const boost::intrusive_ptr<tbuilder_grid>& content_grid)
{
	swap_grid(NULL, &grid(), content_grid->build(), "_window_content_grid");
}

#ifdef DEBUG_WINDOW_LAYOUT_GRAPHS

void twindow::generate_dot_file(const std::string& generator,
		const unsigned domain)
{
	debug_layout_->generate_dot_file(generator, domain);
}
#endif

void twindow::radio_page_swap_uh(const tradio_page::tpage& page, twidget* holder, bool first)
{
	// earse linked_size_
	// typedef std::pair<const std::string, tlinked_size> hack;
	for (std::map<std::string, tlinked_size>::iterator hack = linked_size_.begin(); hack != linked_size_.end();) {
		if (!hack->second.radio) {
			hack ++;
			continue;
		}
		std::map<std::string, tlinked_size>::iterator erase_it = hack;
		hack ++;
		linked_size_.erase(erase_it);
	}

	// add linked_size_
	BOOST_FOREACH(const tlinked_group& lg, page.linked_groups) {

		if (has_linked_size_group(lg.id)) {
			utils::string_map symbols;
			symbols["id"] = lg.id;
			t_string msg = vgettext(
					  "Linked '$id' group has multiple definitions."
					, symbols);
			VALIDATE(false, msg);
		}

		init_linked_size_group(
				lg.id, lg.fixed_width, lg.fixed_height, true);
	}

	// keyboard focus maybe in delete control, set keyboard_focus_ to null.
	// keyboard_capture(NULL);

	if (page.row) {
		tlistbox* table = dynamic_cast<tlistbox*>(holder);

		table->set_list_builder(page.row);

		// _header_grid
		tgrid* grid_ptr = new tgrid();
		page.header->build(grid_ptr);
		grid_ptr->set_id("_header_grid");
		twidget* widget = table->content_grid()->swap_child("_header_grid", grid_ptr, true, this);
		VALIDATE(widget, "original grid cannot find table_header id");
		delete widget;
	} else {
		tscrollbar_panel* panel = dynamic_cast<tscrollbar_panel*>(holder);

		// _grid
		tgrid* grid_ptr = new tgrid();
		page.header->build(grid_ptr);
		grid_ptr->set_id("_grid");
		twidget* widget = panel->content_grid()->swap_child("_grid", grid_ptr, false, this);
		VALIDATE(widget, "original grid cannot find _grid id");
		delete widget;
	}
}

void twindow::radio_page_swap_bh(const tradio_page::tpage& page, twidget* holder)
{
	tscrollbar_container* container = dynamic_cast<tscrollbar_container*>(holder);

	// container->layout_init(true);
	// layout_linked_widgets(holder);
	container->invalidate_layout(true);
}

void twindow::mouse_capture(const bool capture)
{
	assert(event_distributor_);
	event_distributor_->capture_mouse(capture);
}

void twindow::keyboard_capture(twidget* widget)
{
	assert(event_distributor_);
	event_distributor_->keyboard_capture(widget);
}

void twindow::add_to_keyboard_chain(twidget* widget)
{
	assert(event_distributor_);
	event_distributor_->keyboard_add_to_chain(widget);
}

void twindow::remove_from_keyboard_chain(twidget* widget)
{
	assert(event_distributor_);
	event_distributor_->keyboard_remove_from_chain(widget);
}

void twindow::signal_handler_sdl_video_resize(
			const event::tevent event, bool& handled, const tpoint& new_size)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	if (new_size.x < preferences::min_allowed_width()
			|| new_size.y < preferences::min_allowed_height()) {

		DBG_GUI_E << LOG_HEADER << " resize aborted, too small.\n";
		return;
	}

	if (new_size.x == static_cast<int>(settings::screen_width)
			&& new_size.y == static_cast<int>(settings::screen_height)) {

		DBG_GUI_E << LOG_HEADER << " resize not needed.\n";
		handled = true;
		return;
	}

	display& disp = *display::get_singleton();
	if (!preferences::set_resolution(disp, new_size.x, new_size.y)) {

		LOG_GUI_E << LOG_HEADER
				<< " resize aborted, resize failed.\n";
		return;
	}

	settings::gamemap_width += new_size.x - settings::screen_width ;
	settings::gamemap_height += new_size.y - settings::screen_height ;
	settings::screen_width = new_size.x;
	settings::screen_height = new_size.y;
	invalidate_layout();

	handled = true;
}

void twindow::signal_handler_click_dismiss(
		const event::tevent event, bool& handled, bool& halt)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	handled = halt = click_dismiss();
}

void twindow::signal_handler_sdl_key_down(
		const event::tevent event, bool& handled, SDLKey key)
{
	if (key == SDLK_ESCAPE && !escape_disabled_) {
		set_retval(CANCEL);
		handled = true;
	} else if (key == SDLK_SPACE) {
		handled = click_dismiss();
	}
#ifdef DEBUG_WINDOW_LAYOUT_GRAPHS
	if (key == SDLK_F12) {
		debug_layout_->generate_dot_file(
				"manual", tdebug_layout_graph::MANUAL);
		handled = true;
	}
#endif
}

void twindow::signal_handler_message_show_tooltip(
		  const event::tevent event
		, bool& handled
		, event::tmessage& message)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	event::tmessage_show_tooltip& request =
			dynamic_cast<event::tmessage_show_tooltip&>(message);

	tip::show(video_, tooltip_.id, request.message, request.location);

	handled = true;
}

void twindow::signal_handler_message_show_helptip(
		  const event::tevent event
		, bool& handled
		, event::tmessage& message)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	event::tmessage_show_helptip& request =
			dynamic_cast<event::tmessage_show_helptip&>(message);

	tip::show(video_, helptip_.id, request.message, request.location);

	handled = true;
}

void twindow::signal_handler_request_placement(
		  const event::tevent event
		, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	invalidate_layout();

	handled = true;
}

} // namespace gui2


/**
 * @page layout_algorithm Layout algorithm
 *
 * @section introduction Introduction
 *
 * This page describes how the layout engine for the dialogs works. First
 * a global overview of some terms used in this document.
 *
 * - @ref gui2::twidget "Widget"; Any item which can be used in the widget
 *   toolkit. Not all widgets are visible. In general widgets can not be
 *   sized directly, but this is controlled by a window. A widget has an
 *   internal size cache and if the value in the cache is not equal to 0,0
 *   that value is its best size. This value gets set when the widget can
 *   honour a resize request.  It will be set with the value which honors
 *   the request.
 *
 * - @ref gui2::tgrid "Grid"; A grid is an invisible container which holds
 *   one or more widgets.  Several widgets have a grid in them to hold
 *   multiple widgets eg panels and windows.
 *
 * - @ref gui2::tgrid::tchild "Grid cell"; Every widget which is in a grid is
 *   put in a grid cell. These cells also hold the information about the gaps
 *   between widgets the behaviour on growing etc. All grid cells must have a
 *   widget inside them.
 *
 * - @ref gui2::twindow "Window"; A window is a top level item which has a
 *   grid with its children. The window handles the sizing of the window and
 *   makes sure everything fits.
 *
 * - @ref gui2::twindow::tlinked_size "Shared size group"; A shared size
 *   group is a number of widgets which share width and or height. These
 *   widgets are handled separately in the layout algorithm. All grid cells
 *   width such a widget will get the same height and or width and these
 *   widgets won't be resized when there's not enough size. To be sure that
 *   these widgets don't cause trouble for the layout algorithm, they must be
 *   in a container with scrollbars so there will always be a way to properly
 *   layout them. The engine must enforce this restriction so the shared
 *   layout property must be set by the engine after validation.
 *
 * - All visible grid cells; A grid cell is visible when the widget inside
 *   of it doesn't have the state INVISIBLE. Widgets which are HIDDEN are
 *   sized properly since when they become VISIBLE the layout shouldn't be
 *   invalidated. A grid cell that's invisible has size 0,0.
 *
 * - All resizable grid cells; A grid cell is resizable under the following
 *   conditions:
 *   - The widget is VISIBLE.
 *   - The widget is not in a shared size group.
 *
 * There are two layout algorithms with a different purpose.
 *
 * - The Window algorithm; this algorithm's goal is it to make sure all grid
 *   cells fit in the window. Sizing the grid cells depends on the widget
 *   size as well, but this algorithm only sizes the grid cells and doesn't
 *   handle the widgets inside them.
 *
 * - The Grid algorithm; after the Window algorithm made sure that all grid
 *   cells fit this algorithm makes sure the widgets are put in the optimal
 *   state in their grid cell.
 *
 * @section layout_algorithm_window Window
 *
 * Here is the algorithm used to layout the window:
 *
 * - Perform a full initialization
 *   (@ref gui2::twidget::layout_init (full_initialization = true)):
 *   - Clear the internal best size cache for all widgets.
 *   - For widgets with scrollbars hide them unless the
 *     @ref gui2::tscrollbar_container::tscrollbar_mode "scrollbar_mode" is
 *     always_visible or auto_visible.
 * - Handle shared sizes:
 *   - Height and width:
 *     - Get the best size for all widgets that share height and width.
 *     - Set the maximum of width and height as best size for all these
 *       widgets.
 *   - Width only:
 *     - Get the best width for all widgets which share their width.
 *     - Set the maximum width for all widgets, but keep their own height.
 *   - Height only:
 *     - Get the best height for all widgets which share their height.
 *     - Set the maximum height for all widgets, but keep their own width.
 * - Start layout loop:
 *   - Get best size.
 *   - If width <= maximum_width && height <= maximum_height we're done.
 *   - If width > maximum_width, optimize the width:
 *     - For every grid cell in a grid row there will be a resize request
 *       (@ref gui2::tgrid::reduce_width):
 *       - Sort the widgets in the row on the resize priority.
 *         - Loop through this priority queue until the row fits
 *           - If priority != 0 try to share the extra width else all
 *             widgets are tried to reduce the full size.
 *           - Try to shrink the widgets by either wrapping or using a
 *             scrollbar (@ref gui2::twidget::request_reduce_width).
 *           - If the row fits in the wanted width this row is done.
 *           - Else try the next priority.
 *         - All priorities done and the width still doesn't fit.
 *         - Loop through this priority queue until the row fits.
 *           - If priority != 0:
 *             - try to share the extra width
 *           -Else:
 *             - All widgets are tried to reduce the full size.
 *           - Try to shrink the widgets by sizing them smaller as really
 *             wanted (@ref gui2::twidget::demand_reduce_width).
 *             For labels, buttons etc. they get ellipsized.
 *           - If the row fits in the wanted width this row is done.
 *           - Else try the next priority.
 *         - All priorities done and the width still doesn't fit.
 *         - Throw a layout width doesn't fit exception.
 *   - If height > maximum_height, optimize the height
 *       (@ref gui2::tgrid::reduce_height):
 *     - For every grid cell in a grid column there will be a resize request:
 *       - Sort the widgets in the column on the resize priority.
 *         - Loop through this priority queue until the column fits:
 *           - If priority != 0 try to share the extra height else all
 *              widgets are tried to reduce the full size.
 *           - Try to shrink the widgets by using a scrollbar
 *             (@ref gui2::twidget::request_reduce_height).
 *             - If succeeded for a widget the width is influenced and the
 *               width might be invalid.
 *             - Throw a width modified exception.
 *           - If the column fits in the wanted height this column is done.
 *           - Else try the next priority.
 *         - All priorities done and the height still doesn't fit.
 *         - Loop through this priority queue until the column fits.
 *           - If priority != 0 try to share the extra height else all
 *             widgets are tried to reduce the full size.
 *           - Try to shrink the widgets by sizing them smaller as really
 *             wanted (@ref gui2::twidget::demand_reduce_width).
 *             For labels, buttons etc. they get ellipsized .
 *           - If the column fits in the wanted height this column is done.
 *           - Else try the next priority.
 *         - All priorities done and the height still doesn't fit.
 *         - Throw a layout height doesn't fit exception.
 * - End layout loop.
 *
 * - Catch @ref gui2::tlayout_exception_width_modified "width modified":
 *   - Goto relayout.
 *
 * - Catch
 *   @ref gui2::tlayout_exception_width_resize_failed "width resize failed":
 *   - If the window has a horizontal scrollbar which isn't shown but can be
 *     shown.
 *     - Show the scrollbar.
 *     - goto relayout.
 *   - Else show a layout failure message.
 *
 * - Catch
 *   @ref gui2::tlayout_exception_height_resize_failed "height resize failed":
 *   - If the window has a vertical scrollbar which isn't shown but can be
 *     shown:
 *     - Show the scrollbar.
 *     - goto relayout.
 *   - Else:
 *     - show a layout failure message.
 *
 * - Relayout:
 *   - Initialize all widgets
 *     (@ref gui2::twidget::layout_init (full_initialization = false))
 *   - Handle shared sizes, since the reinitialization resets that state.
 *   - Goto start layout loop.
 *
 * @section grid Grid
 *
 * This section will be documented later.
 */
