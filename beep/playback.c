/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public Licensse as published by
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "libbeep/util.h"
#include "libbeep/configdb.h"

#include "input.h"
#include "main.h"
#include "mainwin.h"
#include "equalizer.h"
#include "output.h"
#include "playlist.h"
#include "playlistwin.h"
#include "playlist_list.h"
#include "skin.h"
#include "skinwin.h"
#include "urldecode.h"
#include "util.h"


#include "playback.h"


/* FIXME: yuck!! this shouldn't be here... */
void
bmp_playback_set_random_skin(void)
{
    SkinNode *node;
    guint32 randval;

    /* Get a random value to select the skin to use */
    randval = g_random_int_range(0, g_list_length(skinlist));
    node = g_list_nth(skinlist, randval)->data;
    bmp_active_skin_load(node->path);
}

gint
bmp_playback_get_time(void)
{
    if (!bmp_playback_get_playing())
        return -1;

    if (!get_current_input_plugin())
        return -1;

    return get_current_input_plugin()->get_time();
}

void
bmp_playback_initiate(void)
{
    const gchar *filename = NULL;

    if (playlist_get_length() == 0)
        return;

    if (bmp_playback_get_playing())
        bmp_playback_stop();

    vis_clear_data(mainwin_vis);
    vis_clear_data(playlistwin_vis);
    svis_clear_data(mainwin_svis);
    mainwin_disable_seekbar();

    filename = playlist_get_filename_to_play();

    if (!filename)
        return;

    if (!bmp_playback_play_file(filename))
        return;

    if (bmp_playback_get_time() != -1) {
        equalizerwin_load_auto_preset(filename);
        input_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                     cfg.equalizer_bands);
        output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                      cfg.equalizer_bands);
    }

    playlist_check_pos_current();
    mainwin_set_info_text();
}

void
bmp_playback_pause(void)
{
    if (!bmp_playback_get_playing())
        return;

    if (!get_current_input_plugin())
        return;

    ip_data.paused = !ip_data.paused;

    if (ip_data.paused)
        playstatus_set_status(mainwin_playstatus, STATUS_PAUSE);
    else
        playstatus_set_status(mainwin_playstatus, STATUS_PLAY);

    get_current_input_plugin()->pause(ip_data.paused);
}

void
bmp_playback_stop(void)
{
    if (ip_data.playing && get_current_input_plugin()) {
        ip_data.playing = FALSE;

        if (bmp_playback_get_paused())
            bmp_playback_pause();

        if (get_current_input_plugin()->stop)
            get_current_input_plugin()->stop();

        free_vis_data();
        ip_data.paused = FALSE;

        if (input_info_text) {
            g_free(input_info_text);
            input_info_text = NULL;
            mainwin_set_info_text();
        }
    }

    ip_data.playing = FALSE;
}


static void
run_no_output_plugin_dialog(void)
{
    const gchar *markup = 
        N_("<b><big>No output plugin selected.</big></b>\n"
           "You have not selected an output plugin.");

    GtkWidget *dialog =
        gtk_message_dialog_new_with_markup(GTK_WINDOW(mainwin),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_OK,
                                           _(markup));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

gboolean
bmp_playback_play_file(const gchar * filename)
{
    GList *node;
    InputPlugin *ip;
    gchar *filename_proxy;

    g_return_val_if_fail(filename != NULL, FALSE);

    if (!get_current_output_plugin()) {
        run_no_output_plugin_dialog();
        mainwin_stop_pushed();
        return FALSE;
    }

    if (cfg.random_skin_on_play)
        bmp_playback_set_random_skin();

    filename_proxy = g_strdup(filename);

    node = get_input_list();
    node = g_list_first(node);

    while (node) {

        ip = node->data;

        if (!ip)
            break;

        if (ip && input_is_enabled(ip->filename) &&
            ip->is_our_file(filename_proxy)) {


            set_current_input_plugin(ip);
            ip->output = get_current_output_plugin();
            ip->play_file(filename_proxy);

            /* FIXME: Why the hell (yes,hell!) doesn't the input
               plugin set this itself????  -mderezynski */
            ip_data.playing = TRUE;

            g_free(filename_proxy);
            return TRUE;
        }
        node = g_list_next(node);
    }

    input_file_not_playable(filename);
    set_current_input_plugin(NULL);
    mainwin_set_info_text();

    g_free(filename_proxy);

    return FALSE;
}

gboolean
bmp_playback_get_playing(void)
{
    return ip_data.playing;
}

gboolean
bmp_playback_get_paused(void)
{
    return ip_data.paused;
}

void
bmp_playback_seek(gint time)
{
    gboolean restore_pause = FALSE;
    gint l=0, r=0;

    if (!ip_data.playing)
        return;

    if (!get_current_input_plugin())
        return;

    /* FIXME WORKAROUND...that should work with all plugins
     * mute the volume, start playback again, do the seek, then pause again
     * -Patrick Sudowe  */
    if(ip_data.paused)
    {
	restore_pause = TRUE;
	output_get_volume(&l, &r);
	output_set_volume(0,0);
	bmp_playback_pause();	
    }
    
    free_vis_data();
    get_current_input_plugin()->seek(time);
    
    if(restore_pause)
    {
	bmp_playback_pause();
	output_set_volume(l, r);
    }
}

void
bmp_playback_seek_relative(gint offset)
{
    gint time = CLAMP(bmp_playback_get_time() / 1000 + offset,
                      0, playlist_get_current_length() / 1000 - 1);
    bmp_playback_seek(time);
}
