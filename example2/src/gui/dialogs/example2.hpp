/* Require Rose v0.0.6 or above. $ */

#ifndef GUI_DIALOGS_EXAMPLE2_HPP_INCLUDED
#define GUI_DIALOGS_EXAMPLE2_HPP_INCLUDED

#include "gui/dialogs/chat.hpp"
#include "gui/widgets/report.hpp"

namespace gui2 {

class texample2 : public tchat_
{
public:
	enum tresult {CHANGE_LANGUAGE = 1};

	enum {CHAT_PAGE, PREFERENCES_PAGE};

	explicit texample2(display& disp, bool first);

private:
	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	/** Inherited from tdialog. */
	void pre_show(CVideo& video, twindow& window);

	void toggle_tabbar(twidget* widget);
	void fullscreen_toggled(twidget* widget);
	void video_mode_button(twindow& window);
	void set_retval(twindow& window, int retval);

private:
	display& disp_;
	int start_page_;
	ttabbar navigate_;
};


}


#endif /* ! GUI_DIALOGS_CAMPAIGN_DIFFICULTY_HPP_INCLUDED */
