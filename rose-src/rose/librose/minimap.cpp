/* $Id: minimap.cpp 47608 2010-11-21 01:56:29Z shadowmaster $ */
/*
   Copyright (C) 2003 - 2010 by David White <dave@whitevine.net>
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

#include "global.hpp"
#include "minimap.hpp"

#include "gettext.hpp"
#include "image.hpp"
#include "log.hpp"
#include "map.hpp"
#include "sdl_utils.hpp"
#include "builder.hpp"
#include "display.hpp"

#include "wml_exception.hpp"
#include "formula_string_utils.hpp"

static lg::log_domain log_display("display");
#define DBG_DP LOG_STREAM(debug, log_display)
#define WRN_DP LOG_STREAM(warn, log_display)

namespace image {

surface getMinimap(int w, int h, const gamemap &map, const display* disp)
{
	const size_t map_width = map.w() * scale_ratio_w;
	const size_t map_height = map.h() * scale_ratio_h;
	if (map_width == 0 || map_height == 0) {
		return surface(NULL);
	}

	surface minimap(create_neutral_surface(map_width, map_height));
	if (minimap == NULL) {
		return surface(NULL);
	}

	typedef mini_terrain_cache_map cache_map;
	cache_map *normal_cache = &mini_terrain_cache;
	cache_map *fog_cache = &mini_fogged_terrain_cache;

	SDL_Rect tilerect = empty_rect;
	for (int y = 0; y != map.total_height(); ++y) {
		for (int x = 0; x != map.total_width(); ++x) {

			surface surf(NULL);

			const map_location loc(x,y);
			if(map.on_board(loc)) {

				bool shrouded = false;
				bool fogged = false;
				if (disp) {
					disp->shrouded_and_fogged(loc, shrouded, fogged);
				}
				const t_translation::t_terrain terrain = shrouded ?
						t_translation::VOID_TERRAIN : map[loc];
				const terrain_type& terrain_info = map.get_terrain_info(terrain);

				bool need_fogging = false;

				cache_map* cache = fogged ? fog_cache : normal_cache;
				cache_map::iterator i = cache->find(terrain);

				if (fogged && i == cache->end()) {
					// we don't have the fogged version in cache
					// try the normal cache and ask fogging the image
					cache = normal_cache;
					i = cache->find(terrain);
					need_fogging = true;
				}

				if(i == cache->end()) {
					std::string base_file =
						image::terrain_prefix + terrain_info.minimap_image() + ".png";
					// surface tile = get_image(base_file,image::HEXED);
					surface tile = get_hexed(base_file);
					
					//Compose images of base and overlay if necessary
					// NOTE we also skip overlay when base is missing (to avoid hiding the error)
					if(tile != NULL && map.get_terrain_info(terrain).is_combined()) {
						std::string overlay_file =
								image::terrain_prefix + terrain_info.minimap_image_overlay() + ".png";
						// surface overlay = get_image(overlay_file,image::HEXED);
						surface overlay = get_hexed(overlay_file);

						if(overlay != NULL && overlay != tile) {
							surface combined = create_compatible_surface(tile, tile->w, tile->h);
							SDL_Rect r = create_rect(0,0,0,0);
							sdl_blit(tile, NULL, combined, &r);
							r.x = std::max(0, (tile->w - overlay->w)/2);
							r.y = std::max(0, (tile->h - overlay->h)/2);
							//blit_surface needs neutral surface
							surface overlay_neutral = make_neutral_surface(overlay);
							blit_surface(overlay_neutral, NULL, combined, &r);
							tile = combined;
						}
					}

					surf = scale_surface_blended(tile, scale_ratio, scale_ratio);

					i = normal_cache->insert(cache_map::value_type(terrain,surf)).first;
				}

				surf = i->second;

				if (need_fogging) {
					surf = adjust_surface_color(surf,-50,-50,-50);
					fog_cache->insert(cache_map::value_type(terrain,surf));
				}

				// we need a balanced shift up and down of the hexes.
				// if not, only the bottom half-hexes are clipped
				// and it looks asymmetrical.

				// also do 1-pixel shift because the scaling
				// function seems to do it with its rounding
				tilerect.x = x;
				tilerect.y = y;
				minimap_tile_dst(tilerect.x, tilerect.y);

				if (surf != NULL) {
					sdl_blit(surf, NULL, minimap, &tilerect);
				}
			}
		}
	}

	double wratio = w*1.0 / minimap->w;
	double hratio = h*1.0 / minimap->h;
	double ratio = std::min<double>(wratio, hratio);

	minimap = scale_surface(minimap,
		static_cast<int>(minimap->w * ratio), static_cast<int>(minimap->h * ratio));

	DBG_DP << "done generating minimap\n";

	return minimap;
}
}
