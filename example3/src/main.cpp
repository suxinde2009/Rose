/* Require Rose v0.0.7 or above. $ */

#define GETTEXT_DOMAIN "wesnoth-lib"

#include "gui/dialogs/sleep.hpp"

#include "base_instance.hpp"
#include "preferences_display.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/dialogs/chat.hpp"
#include "gui/dialogs/combo_box.hpp"
#include "gui/dialogs/login.hpp"
#include "gui/widgets/window.hpp"
#include "chart_controller.hpp"
#include "game_end_exceptions.hpp"
#include "wml_exception.hpp"
#include "gettext.hpp"
#include "hotkeys.hpp"
#include "loadscreen.hpp"
#include "formula_string_utils.hpp"
#include "version.hpp"
#include "help.hpp"

#include <errno.h>
#include <iostream>

#include <boost/foreach.hpp>

/*
 * rose require callback
 */
void set_zoom_to_default(int zoom)
{
	display::default_zoom_ = zoom;
	image::set_zoom(display::default_zoom_);
}

namespace http {
bool register_user(display& disp, hero_map& heros, bool check_exist)
{
	return true;
}
}

bool hero::check_valid() const
{
	return true;
}

bool hero::confirm_carry_to(hero& h2, int carry_to)
{
	return true;
}


class game_instance: public base_instance
{
public:
	game_instance(int argc, char** argv);

	void fill_anim_tags(std::map<const std::string, int>& tags);
	void start_chart(bool snore);
};

game_instance::game_instance(int argc, char** argv)
	: base_instance(argc, argv)
{
}

void game_instance::fill_anim_tags(std::map<const std::string, int>& tags)
{
	// although don't use below animation, but pass program verify, define them still.
	int at = anim2::MIN_APP_ANIM;
	tags.insert(std::make_pair("reinforce", at ++));
	tags.insert(std::make_pair("individuality", at ++));
	tags.insert(std::make_pair("tactic", at ++));
	tags.insert(std::make_pair("formation_attack", at ++));
	tags.insert(std::make_pair("perfect", at ++));
	tags.insert(std::make_pair("stratagem_up", at ++));
	tags.insert(std::make_pair("stratagem_down", at ++));
	tags.insert(std::make_pair("load_scenario", at ++));
	tags.insert(std::make_pair("flags", at ++));
	tags.insert(std::make_pair("text", at ++));
	tags.insert(std::make_pair("place", at ++));
	tags.insert(std::make_pair("upgrade", at ++));
	tags.insert(std::make_pair("card", at ++));
	tags.insert(std::make_pair("pass_scenario", at ++));

	tags.insert(std::make_pair("blade", at ++));
	tags.insert(std::make_pair("pierce", at ++));
	tags.insert(std::make_pair("impact", at ++));
	tags.insert(std::make_pair("archery", at ++));
	tags.insert(std::make_pair("collapse", at ++));
	tags.insert(std::make_pair("arcane", at ++));
	tags.insert(std::make_pair("fire", at ++));
	tags.insert(std::make_pair("cold", at ++));
	tags.insert(std::make_pair("strike", at ++));
	tags.insert(std::make_pair("magic", at ++));
	tags.insert(std::make_pair("heal", at ++));
	tags.insert(std::make_pair("destruct", at ++));
	tags.insert(std::make_pair("formation_defend", at ++));
	tags.insert(std::make_pair("income", at ++));
}

void game_instance::start_chart(bool snore)
{
	display_lock lock(disp());
	hotkey::scope_changer changer(game_config(), "hotkey_chart");

	chart_controller chart(game_config(), video_, snore);
	chart.main_loop();
}

class tlobby_manager
{
public:
	tlobby_manager()
	{
		lobby = new tlobby();
	}
	~tlobby_manager()
	{
		if (lobby) {
			delete lobby;
			lobby = NULL;
		}
	}
};

/**
 * Setups the game environment and enters
 * the titlescreen or game loops.
 */
static int do_gameloop(int argc, char** argv)
{
	srand((unsigned int)time(NULL));

	// setup userdata directory
#if defined(__APPLE__) || defined(ANDROID)
	set_preferences_dir("Documents");
#else
	set_preferences_dir("kingdom");
#endif

	// modify some game_config variable
	game_config::init("sleep", "Sleep", "#sleep");
	game_config::wesnoth_program_dir = directory_name(argv[0]);
	game_config::version = game_config::rose_version;
	game_config::wesnoth_version = version_info(game_config::version);
	game_config::logo_png = "misc/rose-logo.png";

	game_instance game(argc, argv);
	tinstance_manager instance_manager(game);

	// always connect to lobby server.
	tlobby_manager lobby_manager;
	const network::manager net_manager(1, 1);
	lobby->chat.set_host("chat.freenode.net", 6665);
	lobby->join();
	lobby->set_nick2(group.leader().name());

	bool res;

	// do initialize fonts before reading the game config, to have game
	// config error messages displayed. fonts will be re-initialized later
	// when the language is read from the game config.
	res = font::load_font_config();
	if(res == false) {
		std::cerr << "could not initialize fonts\n";
		return 1;
	}

	res = game.init_language();
	if (res == false) {
		std::cerr << "could not initialize the language\n";
		return 1;
	}

	res = game.init_video();
	if(res == false) {
		std::cerr << "could not initialize display\n";
		return 1;
	}

	const cursor::manager cursor_manager;
	cursor::set(cursor::WAIT);

	loadscreen::global_loadscreen_manager loadscreen_manager(game.disp().video());

	loadscreen::start_stage("init gui");
	gui2::init();
	const gui2::event::tmanager gui_event_manager;

	try {
		// form this, code can use VALIDATE macro.
		loadscreen::start_stage("load config");
		res = game.init_config(false);
		if (res == false) {
			std::cerr << "could not initialize game config\n";
			return 1;
		}
		game_config::images::game_title = "dialogs/page-background.png";

		loadscreen::start_stage("init fonts");

		res = font::load_font_config();
		if (res == false) {
			std::cerr << "could not re-initialize fonts for the current language\n";
			return 1;
		}


		loadscreen::start_stage("titlescreen");

		for (;;) {
			loadscreen_manager.reset();

			gui2::tsleep::tresult res = gui2::tsleep::NOTHING;

			const font::floating_label_context label_manager(game.disp().video().getSurface());

			cursor::set(cursor::NORMAL);

			if (res == gui2::tsleep::NOTHING) {
				// load/reload hero_map from file
				
				gui2::tsleep dlg(game.disp(), group.leader());
				dlg.show(game.disp().video());
				res = static_cast<gui2::tsleep::tresult>(dlg.get_retval());
			}

			if (res == gui2::tsleep::QUIT_GAME) {
				posix_print("do_gameloop, received QUIT_GAME, will exit!\n");
				return 0;

			} else if (res == gui2::tsleep::SNORE_CHART) {
				game.start_chart(true);

			} else if (res == gui2::tsleep::MOTION_CHART) {
				game.start_chart(false);

			} else if (res == gui2::tsleep::PLAYER) {
				gui2::tlogin dlg(game.disp(), game.heros(), "");
				dlg.show(game.disp().video());

			} else if (res == gui2::tsleep::CHANGE_LANGUAGE) {
				if (game.change_language()) {
					t_string::reset_translations();
					image::flush_cache();
				}

			} else if (res == gui2::tsleep::EDIT_PREFERENCES) {
				preferences::show_preferences_dialog(game.disp(), true);

			}
		}

	} catch (twml_exception& e) {
		e.show(game.disp());

	} catch (type_error& e) {
		gui2::show_error_message(game.disp().video(), std::string("formula type error: ") + e.message);
	}

	return 0;
}

#include "setjmp.h"
jmp_buf env1;

// I think stack is regular, if you want, you can do.
// But kingdom spend huge memory, it is better clear to start.
// If want normal may wait reduce memory.
void handle_app_event(Uint32 type)
{
	if (type == SDL_APP_TERMINATING) {
		longjmp(env1, 1);

	} else if (type == SDL_APP_WILLENTERBACKGROUND) {
		longjmp(env1, 2);

	} else if (type == SDL_APP_DIDENTERBACKGROUND) {

	} else if (type == SDL_APP_WILLENTERFOREGROUND) {
		
	} else if (type == SDL_APP_DIDENTERFOREGROUND) {
		
	}
}

int main(int argc, char** argv)
{
#if defined(__APPLE__) && TARGET_OS_IPHONE
	SDL_SetHint(SDL_HINT_IDLE_TIMER_DISABLED, "1");
#endif

#ifdef _WIN32
	_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	// result to memroy leak on purpose.
	char * leak_p = (char*)malloc(13);
#endif

	if (setjmp(env1)) {
		posix_print("game.cpp, main, setjmp: non-zero\n");
#if defined(__APPLE__) && TARGET_OS_IPHONE
		SDL_SetHint(SDL_HINT_IDLE_TIMER_DISABLED, "0");
#endif
		if (lobby) {
			delete lobby;
		}
		preferences::write_preferences();
		return 1;
	}

	if (SDL_Init(SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return(1);
	}

	try {
		std::cerr << "Battle for Wesnoth v" << game_config::version << '\n';
		const time_t t = time(NULL);
		std::cerr << "Started on " << ctime(&t) << "\n";

		std::string exe_dir;
#ifdef ANDROID
		exe_dir = argv[0];
#else
		exe_dir = get_exe_dir();
#endif

		if(!exe_dir.empty() && file_exists(exe_dir + "/data/_main.cfg")) {
			std::cerr << "Automatically found a possible data directory at "
			          << exe_dir << '\n';
			game_config::path = exe_dir;
		}

		const int res = do_gameloop(argc,argv);
		exit(res);
	} catch(CVideo::error&) {
		std::cerr << "Could not initialize video. Exiting.\n";
		return 1;
	} catch(font::manager::error&) {
		std::cerr << "Could not initialize fonts. Exiting.\n";
		return 1;
	} catch(config::error& e) {
		std::cerr << e.message << "\n";
		return 1;
	} catch(CVideo::quit&) {
		//just means the game should quit
		posix_print("SDL_main, catched CVideo::quit\n");
	} catch(end_level_exception&) {
		std::cerr << "caught end_level_exception (quitting)\n";
	} catch(twml_exception& e) {
		std::cerr << "WML exception:\nUser message: "
			<< e.user_message << "\nDev message: " << e.dev_message << '\n';
		return 1;
	} catch(game_logic::formula_error& e) {
		posix_print_mb("%s", e.what());
		std::cerr << e.what()
			<< "\n\nGame will be aborted.\n";
		return 1;
	} catch(game::error &) {
		// A message has already been displayed.
		return 1;
	} catch(std::bad_alloc&) {
		std::cerr << "Ran out of memory. Aborted.\n";
		return ENOMEM;
	}

	return 0;
} // end main