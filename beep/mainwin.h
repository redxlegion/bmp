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

#ifndef MAINWIN_H
#define MAINWIN_H

#include <gtk/gtk.h>

#include "number.h"
#include "pbutton.h"
#include "playstatus.h"
#include "tbutton.h"
#include "textbox.h"
#include "svis.h"
#include "vis.h"

/* yes, main window size is fixed */
#define MAINWIN_WIDTH            (gint)275
#define MAINWIN_HEIGHT           (gint)116
#define MAINWIN_TITLEBAR_HEIGHT  (gint)14
#define MAINWIN_SHADED_HEIGHT    MAINWIN_TITLEBAR_HEIGHT

#define MAINWIN_UPDATE_INTERVAL  10

#define MAINWIN_DEFAULT_POS_X    20
#define MAINWIN_DEFAULT_POS_Y    20

#define MAINWIN_DEFAULT_FONT     "Sans Bold 9"


typedef enum {
    TIMER_ELAPSED,
    TIMER_REMAINING
} TimerMode;

enum {
    MAINWIN_GENERAL_ABOUT,
    
    MAINWIN_GENERAL_PLAYFILE,
    MAINWIN_GENERAL_PLAYDIRECTORY,
    MAINWIN_GENERAL_PLAYLOCATION,

    MAINWIN_GENERAL_FILEINFO,
    MAINWIN_GENERAL_PREFS,

    MAINWIN_GENERAL_SHOWMWIN,
    MAINWIN_GENERAL_SHOWPLWIN,

    MAINWIN_GENERAL_FOCUSMWIN,
    MAINWIN_GENERAL_FOCUSPLWIN,

    MAINWIN_GENERAL_SHOWEQWIN,
    MAINWIN_GENERAL_PLAYCD,
    MAINWIN_GENERAL_EXIT,

    MAINWIN_GENERAL_ADDCD,

    MAINWIN_GENERAL_PREV,
    MAINWIN_GENERAL_PLAY,
    MAINWIN_GENERAL_PAUSE,
    MAINWIN_GENERAL_STOP,
    MAINWIN_GENERAL_NEXT,
    MAINWIN_GENERAL_STOPFADE,
    MAINWIN_GENERAL_BACK5SEC,
    MAINWIN_GENERAL_FWD5SEC,
    MAINWIN_GENERAL_START,
    MAINWIN_GENERAL_BACK10,
    MAINWIN_GENERAL_FWD10,
    MAINWIN_GENERAL_JTT,
    MAINWIN_GENERAL_JTF,
    MAINWIN_GENERAL_QUEUE,
    MAINWIN_GENERAL_CQUEUE,
    MAINWIN_GENERAL_VOLUP,
    MAINWIN_GENERAL_VOLDOWN
};

extern GtkWidget *mainwin;
extern GdkGC *mainwin_gc;

extern GtkAccelGroup *mainwin_accel;

extern gboolean mainwin_moving;
extern gboolean mainwin_focus;

extern GtkWidget *mainwin_jtf;

extern GtkItemFactory *mainwin_general_menu; 
extern GtkItemFactory *mainwin_vis_menu;
extern GtkItemFactory *mainwin_play_menu, *mainwin_view_menu;

extern TextBox *mainwin_info;
extern TButton *mainwin_shuffle, *mainwin_repeat, *mainwin_eq, *mainwin_pl;

extern Vis *active_vis;
extern Vis *mainwin_vis;
extern SVis *mainwin_svis;

extern PlayStatus *mainwin_playstatus;


void mainwin_create(void);
void read_volume(gint when);
void play_medium(void);
void add_medium(void);

void draw_main_window(gboolean);

void mainwin_quit_cb(void);
void mainwin_lock_info_text(const gchar * text);
void mainwin_release_info_text(void);
void mainwin_play_pushed(void);
void mainwin_stop_pushed(void);
void mainwin_eject_pushed(void);

void mainwin_set_back_pixmap(void);

void mainwin_adjust_volume_motion(gint v);
void mainwin_adjust_volume_release(void);
void mainwin_adjust_balance_motion(gint b);
void mainwin_adjust_balance_release(void);
void mainwin_set_volume_slider(gint percent);
void mainwin_set_balance_slider(gint percent);

void mainwin_vis_set_type(VisType mode);

void mainwin_set_info_text(void);
void mainwin_set_song_info(gint rate, gint freq, gint nch);
void mainwin_clear_song_info(void);

void mainwin_set_always_on_top(gboolean always);
void mainwin_set_volume_diff(gint diff);
void mainwin_set_balance_diff(gint diff);

void mainwin_show(gboolean);
void mainwin_real_show(void);
void mainwin_real_hide(void);
void mainwin_move(gint x, gint y);
void mainwin_shuffle_pushed(gboolean toggled);
void mainwin_repeat_pushed(gboolean toggled);
void mainwin_disable_seekbar(void);
void mainwin_set_title(const gchar * text);
void mainwin_run_dirbrowser(void);
void mainwin_show_add_url_window(void);
void mainwin_minimize_cb(void);
void mainwin_general_menu_callback(gpointer cb_data,
                                   guint action,
                                   GtkWidget * widget);

void mainwin_attach_idle_func(void);
void mainwin_drag_data_received(GtkWidget * widget,
                                GdkDragContext * context,
                                gint x,
                                gint y,
                                GtkSelectionData * selection_data,
                                guint info,
                                guint time,
                                gpointer user_data);

void mainwin_setup_menus(void);

void mainwin_jump_to_file(void);
void mainwin_jump_to_time(void);

void mainwin_ewmh_activate(void);

/* FIXME: placed here for now */
void playback_get_sample_params(gint * bitrate,
                                gint * frequency,
                                gint * numchannels);

#endif
