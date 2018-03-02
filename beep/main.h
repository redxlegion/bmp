/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef MAIN_H
#define MAIN_H

#include "mainwin.h"
#include "textbox.h"
#include "vis.h"


#define BMP_USER_PLUGIN_DIR_BASENAME  "Plugins"
#define BMP_SKIN_DIR_BASENAME         "Skins"
#define BMP_SKIN_THUMB_DIR_BASENAME   ".thumbs"
#define BMP_ACCEL_BASENAME            "accels"
#define BMP_CONFIG_BASENAME           "config"
#define BMP_PLAYLIST_BASENAME         "bmp.m3u"
#define BMP_LOG_BASENAME              "log"


#define PLAYER_HEIGHT \
  (cfg.player_shaded ? MAINWIN_SHADED_HEIGHT : MAINWIN_HEIGHT)
#define PLAYER_WIDTH  MAINWIN_WIDTH

struct _BmpConfig {
    gint player_x, player_y;
    gint equalizer_x, equalizer_y;
    gint playlist_x, playlist_y;
    gint playlist_width, playlist_height;
    gint snap_distance;
    gboolean use_realtime;
    gboolean shuffle, repeat;
    gboolean doublesize, autoscroll;
    gboolean analyzer_peaks, equalizer_autoload, easy_move, equalizer_active;
    gboolean playlist_visible, equalizer_visible, player_visible;
    gboolean player_shaded, playlist_shaded, equalizer_shaded;
    gboolean allow_multiple_instances, always_show_cb;
    gboolean convert_underscore, convert_twenty;
    gboolean show_numbers_in_pl;
    gboolean snap_windows, save_window_position;
    gboolean dim_titlebar;
    gboolean get_info_on_load, get_info_on_demand;
    gboolean eq_doublesize_linked;
    gboolean sort_jump_to_file;
    gboolean use_eplugins;
    gboolean always_on_top, sticky;
    gboolean no_playlist_advance;
    gboolean smooth_title_scroll;
    gboolean use_pl_metadata;
    gboolean warn_about_unplayables;
    gboolean use_backslash_as_dir_delimiter;
    gboolean random_skin_on_play;
    gboolean use_fontsets;
    gboolean mainwin_use_xfont;
    gboolean custom_cursors;
    gboolean close_dialog_open;
    gboolean close_dialog_add;
    gfloat equalizer_preamp, equalizer_bands[10];
    gchar *skin;
    gchar *outputplugin;
    gchar *filesel_path;
    gchar *playlist_path;
    gchar *playlist_font, *mainwin_font;
    gchar *disabled_iplugins;
    gchar *enabled_gplugins, *enabled_vplugins, *enabled_eplugins;
    gchar *eqpreset_default_file, *eqpreset_extension;
    GList *url_history;
    gint timer_mode;
    gint vis_type;
    gint analyzer_mode, analyzer_type;
    gint scope_mode;
    gint vu_mode, vis_refresh;
    gint analyzer_falloff, peaks_falloff;
    gint playlist_position;
    gint pause_between_songs_time;
    gboolean pause_between_songs;
    gboolean show_wm_decorations;
    gint mouse_change;
    gboolean playlist_transparent;
    gint titlestring_preset;
    gchar *gentitle_format;
    gboolean softvolume_enable;
    gboolean xmms_compat_mode;
    gboolean eq_extra_filtering;
    gint scroll_pl_by;
};

typedef struct _BmpConfig BmpConfig;

enum {
    VOLSET_STARTUP,
    VOLSET_UPDATE,
    VOLUME_ADJUSTED,
    VOLUME_SET
};

enum {
    BMP_PATH_LOG_FILE,
    BMP_PATH_USER_DIR,
    BMP_PATH_USER_PLUGIN_DIR,
    BMP_PATH_USER_SKIN_DIR,
    BMP_PATH_SKIN_THUMB_DIR,
    BMP_PATH_ACCEL_FILE,
    BMP_PATH_CONFIG_FILE,
    BMP_PATH_PLAYLIST_FILE,
    BMP_PATH_COUNT
};

extern BmpConfig cfg;
extern BmpConfig bmp_default_config;

extern gchar *bmp_paths[];

extern const gchar *bmp_titlestring_presets[];
extern const guint n_titlestring_presets;

extern GList *dock_window_list;
extern gboolean pposition_broken;

void bmp_config_save(void);
void bmp_config_load(void);

#endif
