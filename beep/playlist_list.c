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

/*
 *  A note about Pango and some funky spacey fonts: Weirdly baselined
 *  fonts, or fonts with weird ascents or descents _will_ display a
 *  little bit weird in the playlist widget, but the display engine
 *  won't make it look too bad, just a little deranged.  I honestly
 *  don't think it's worth fixing (around...), it doesn't have to be
 *  perfectly fitting, just the general look has to be ok, which it
 *  IMHO is.
 *
 *  A second note: The numbers aren't perfectly aligned, but in the
 *  end it looks better when using a single Pango layout for each
 *  number.
 */

#include "playlist_list.h"

#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "input.h"
#include "playback.h"
#include "playlist.h"
#include "playlistwin.h"
#include "util.h"

#include "debug.h"

static PangoFontDescription *playlist_list_font = NULL;
static gint ascent, descent, width_delta_digit_one;
static gboolean has_slant;
static guint padding;

/* FIXME: the following globals should not be needed. */
static gint width_approx_letters;
static gint width_colon, width_colon_third;
static gint width_approx_digits, width_approx_digits_half;

static gboolean
playlist_list_auto_drag_down_func(gpointer data)
{
    PlayList_List *pl = data;

    if (pl->pl_auto_drag_down) {
        playlist_list_move_down(pl);
        pl->pl_first++;
        playlistwin_update_list();
        return TRUE;
    }
    return FALSE;
}

static gboolean
playlist_list_auto_drag_up_func(gpointer data)
{
    PlayList_List *pl = data;

    if (pl->pl_auto_drag_up) {
        playlist_list_move_up(pl);
        pl->pl_first--;
        playlistwin_update_list();
        return TRUE;

    }
    return FALSE;
}

void
playlist_list_move_up(PlayList_List * pl)
{
    GList *list;

    PLAYLIST_LOCK();
    if ((list = playlist_get()) == NULL) {
        PLAYLIST_UNLOCK();
        return;
    }
    if (PLAYLIST_ENTRY(list->data)->selected) {
        /* We are at the top */
        PLAYLIST_UNLOCK();
        return;
    }
    while (list) {
        if (PLAYLIST_ENTRY(list->data)->selected)
            glist_moveup(list);
        list = g_list_next(list);
    }
    PLAYLIST_UNLOCK();
    if (pl->pl_prev_selected != -1)
        pl->pl_prev_selected--;
    if (pl->pl_prev_min != -1)
        pl->pl_prev_min--;
    if (pl->pl_prev_max != -1)
        pl->pl_prev_max--;
}

void
playlist_list_move_down(PlayList_List * pl)
{
    GList *list;

    PLAYLIST_LOCK();

    if (!(list = g_list_last(playlist_get()))) {
        PLAYLIST_UNLOCK();
        return;
    }

    if (PLAYLIST_ENTRY(list->data)->selected) {
        /* We are at the bottom */
        PLAYLIST_UNLOCK();
        return;
    }

    while (list) {
        if (PLAYLIST_ENTRY(list->data)->selected)
            glist_movedown(list);
        list = g_list_previous(list);
    }

    PLAYLIST_UNLOCK();

    if (pl->pl_prev_selected != -1)
        pl->pl_prev_selected++;
    if (pl->pl_prev_min != -1)
        pl->pl_prev_min++;
    if (pl->pl_prev_max != -1)
        pl->pl_prev_max++;
}

static void
playlist_list_button_press_cb(GtkWidget * widget,
                              GdkEventButton * event,
                              PlayList_List * pl)
{
    gint nr, y;

    if (event->button == 1 && pl->pl_fheight &&
        widget_contains(&pl->pl_widget, event->x, event->y)) {

        y = event->y - pl->pl_widget.y;
        nr = (y / pl->pl_fheight) + pl->pl_first;

        if (nr >= playlist_get_length())
            nr = playlist_get_length() - 1;

        if (!(event->state & GDK_CONTROL_MASK))
            playlist_select_all(FALSE);

        if (event->state & GDK_SHIFT_MASK && pl->pl_prev_selected != -1) {
            playlist_select_range(pl->pl_prev_selected, nr, TRUE);
            pl->pl_prev_min = pl->pl_prev_selected;
            pl->pl_prev_max = nr;
            pl->pl_drag_pos = nr - pl->pl_first;
        }
        else {
            if (playlist_select_invert(nr)) {
                if (event->state & GDK_CONTROL_MASK) {
                    if (pl->pl_prev_min == -1) {
                        pl->pl_prev_min = pl->pl_prev_selected;
                        pl->pl_prev_max = pl->pl_prev_selected;
                    }
                    if (nr < pl->pl_prev_min)
                        pl->pl_prev_min = nr;
                    else if (nr > pl->pl_prev_max)
                        pl->pl_prev_max = nr;
                }
                else
                    pl->pl_prev_min = -1;
                pl->pl_prev_selected = nr;
                pl->pl_drag_pos = nr - pl->pl_first;
            }
        }
        if (event->type == GDK_2BUTTON_PRESS) {
            /*
             * Ungrab the pointer to prevent us from
             * hanging on to it during the sometimes slow
             * bmp_playback_initiate().
             */
            gdk_pointer_ungrab(GDK_CURRENT_TIME);
            gdk_flush();
            playlist_set_position(nr);
            if (!bmp_playback_get_playing())
                bmp_playback_initiate();
        }

        pl->pl_dragging = TRUE;
        playlistwin_update_list();
    }
}

gint
playlist_list_get_playlist_position(PlayList_List * pl,
                                    gint x,
                                    gint y)
{
    gint iy, length;

    if (!widget_contains(WIDGET(pl), x, y) || !pl->pl_fheight)
        return -1;

    if ((length = playlist_get_length()) == 0)
        return -1;
    iy = y - pl->pl_widget.y;

    return (MIN((iy / pl->pl_fheight) + pl->pl_first, length - 1));
}

static void
playlist_list_motion_cb(GtkWidget * widget,
                        GdkEventMotion * event,
                        PlayList_List * pl)
{
    gint nr, y, off, i;

    if (pl->pl_dragging) {
        y = event->y - pl->pl_widget.y;
        nr = (y / pl->pl_fheight);
        if (nr < 0) {
            nr = 0;
            if (!pl->pl_auto_drag_up) {
                pl->pl_auto_drag_up = TRUE;
                pl->pl_auto_drag_up_tag =
                    gtk_timeout_add(100, playlist_list_auto_drag_up_func, pl);
            }
        }
        else if (pl->pl_auto_drag_up)
            pl->pl_auto_drag_up = FALSE;

        if (nr >= pl->pl_num_visible) {
            nr = pl->pl_num_visible - 1;
            if (!pl->pl_auto_drag_down) {
                pl->pl_auto_drag_down = TRUE;
                pl->pl_auto_drag_down_tag =
                    gtk_timeout_add(100, playlist_list_auto_drag_down_func,
                                    pl);
            }
        }
        else if (pl->pl_auto_drag_down)
            pl->pl_auto_drag_down = FALSE;

        off = nr - pl->pl_drag_pos;
        if (off) {
            for (i = 0; i < abs(off); i++) {
                if (off < 0)
                    playlist_list_move_up(pl);
                else
                    playlist_list_move_down(pl);

            }
            playlistwin_update_list();
        }
        pl->pl_drag_pos = nr;
    }
}

static void
playlist_list_button_release_cb(GtkWidget * widget,
                                GdkEventButton * event,
                                PlayList_List * pl)
{
    pl->pl_dragging = FALSE;
    pl->pl_auto_drag_down = FALSE;
    pl->pl_auto_drag_up = FALSE;
}

static void
playlist_list_draw_string(PlayList_List * pl,
                          PangoFontDescription * font,
                          gint line,
                          gint width,
                          const gchar * text,
                          guint ppos)
{

    gint len;
    gint len_pixmap;
    guint plist_length_int;
    PangoLayout *layout;
    gchar *text_clipped;

    REQUIRE_STATIC_LOCK(playlist);

    len = g_utf8_strlen(text, -1);
    len_pixmap = (width_approx_letters * len);

    while (len_pixmap > width && len > 4) {
        len--;
        len_pixmap -= width_approx_letters;
    }

    /* FIXME: Is it possible to overflow text_clipped when text is non
       UTF-8? - descender */

    text_clipped = g_new0(gchar, strlen(text)+1);
    g_utf8_strncpy(text_clipped, text, len);

    if (cfg.show_numbers_in_pl) {
        gchar *pos_string = g_strdup_printf("%d", ppos);
        plist_length_int =
            gint_count_digits(playlist_get_length_nolock()) + 1;

        padding = plist_length_int;
        padding = ((padding + 1) * width_approx_digits);

        layout = gtk_widget_create_pango_layout(playlistwin, pos_string);
        pango_layout_set_font_description(layout, playlist_list_font);
        pango_layout_set_width(layout, plist_length_int * 100);

        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
        gdk_draw_layout(pl->pl_widget.parent, pl->pl_widget.gc,
                        pl->pl_widget.x +
                        (width_approx_digits *
                         (-1 + plist_length_int - strlen(pos_string))) +
                        (width_approx_digits / 4),
                        pl->pl_widget.y + (line - 1) * pl->pl_fheight +
                        ascent + abs(descent), layout);
        g_free(pos_string);
        g_object_unref(layout);
    }
    else {
        padding = 3;
    }

    layout = gtk_widget_create_pango_layout(playlistwin, text_clipped);

    pango_layout_set_font_description(layout, playlist_list_font);
    gdk_draw_layout(pl->pl_widget.parent, pl->pl_widget.gc,
                    pl->pl_widget.x + padding + (width_approx_letters / 4),
                    pl->pl_widget.y + (line - 1) * pl->pl_fheight +
                    ascent + abs(descent), layout);

    g_object_unref(layout);
    g_free(text_clipped);
}

void
playlist_list_draw(Widget * w)
{
    PlayList_List *pl = PLAYLIST_LIST(w);
    GList *list;
    GdkGC *gc;
    GdkPixmap *obj;
    PangoLayout *layout;
    gchar *title;
    gint width, height;
    gint i, max_first;
    guint padding, padding_dwidth, padding_plength;
    guint max_time_len = 0;
    gint queue_tailpadding = 0;

    gchar tail[100];
    gchar queuepos[255];         /* FIXME CRITICAL: Allows for a limited number of queue positions only */
    gchar length[40];

    gchar **frags;
    gchar *frag0;

    gint plw_w, plw_h;

    GdkRectangle *playlist_rect;

    gc = pl->pl_widget.gc;

    width = pl->pl_widget.width;
    height = pl->pl_widget.height;

    obj = pl->pl_widget.parent;

    gtk_window_get_size(GTK_WINDOW(playlistwin), &plw_w, &plw_h);

    playlist_rect = g_new0(GdkRectangle, 1);

    playlist_rect->x = 0;
    playlist_rect->y = 0;
    playlist_rect->width = plw_w - 17;
    playlist_rect->height = plw_h - 36;

    gdk_gc_set_clip_origin(gc, 31, 58);
    gdk_gc_set_clip_rectangle(gc, playlist_rect);
    gdk_gc_set_foreground(gc,
                          skin_get_color(bmp_active_skin,
                                         SKIN_PLEDIT_NORMALBG));
    gdk_draw_rectangle(obj, gc, TRUE, pl->pl_widget.x, pl->pl_widget.y,
                       width, height);

    if (!playlist_list_font) {
        g_critical("Couldn't open playlist font");
        return;
    }

    pl->pl_fheight = (ascent + abs(descent));
    pl->pl_num_visible = height / pl->pl_fheight;

    max_first = playlist_get_length() - pl->pl_num_visible;
    max_first = MAX(max_first, 0);

    pl->pl_first = CLAMP(pl->pl_first, 0, max_first);

    PLAYLIST_LOCK();
    list = playlist_get();

    for (i = 0; i < pl->pl_first; i++)
        list = g_list_next(list);

    for (i = pl->pl_first;
         list && i < pl->pl_first + pl->pl_num_visible;
         list = g_list_next(list), i++) {
        gint pos;
        PlaylistEntry *entry = list->data;

        if (entry->selected) {
            gdk_gc_set_foreground(gc,
                                  skin_get_color(bmp_active_skin,
                                                 SKIN_PLEDIT_SELECTEDBG));
            gdk_draw_rectangle(obj, gc, TRUE, pl->pl_widget.x,
                               pl->pl_widget.y +
                               ((i - pl->pl_first) * pl->pl_fheight),
                               width, pl->pl_fheight);
        }
        if (i == playlist_get_position_nolock())
            gdk_gc_set_foreground(gc,
                                  skin_get_color(bmp_active_skin,
                                                 SKIN_PLEDIT_CURRENT));
        else
            gdk_gc_set_foreground(gc,
                                  skin_get_color(bmp_active_skin,
                                                 SKIN_PLEDIT_NORMAL));

        /* FIXME: entry->title should NEVER be NULL, and there should
           NEVER be a need to do a UTF-8 conversion. Playlist title
           strings should be kept properly. */

        if (!entry->title) {
            gchar *basename = g_path_get_basename(entry->filename);
            title = filename_to_utf8(basename);
            g_free(basename);
        }
        else
            title = str_to_utf8(entry->title);

        pos = playlist_get_queue_position(entry);

        tail[0] = 0;
        queuepos[0] = 0;
        length[0] = 0;

        if (pos != -1)
            g_snprintf(queuepos, sizeof(queuepos), "%d", pos + 1);

        if (entry->length != -1)
            g_snprintf(length, sizeof(length), "%d:%-2.2d",
                       entry->length / 60000, (entry->length / 1000) % 60);

        if (pos != -1 || entry->length != -1) {
            gint x, y;
            guint tail_width;
            guint tail_len;

            strncat(tail, length, sizeof(tail));
            tail_len = strlen(tail);

            max_time_len = MAX(max_time_len, tail_len);

            /* FIXME: This is just an approximate alignment, maybe
               something still fast, but exact could be done */

            tail_width = width - (tail_len * width_approx_digits) +
                (width_approx_digits_half) - 3;

            if (i == playlist_get_position_nolock())
                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_CURRENT));
            else
                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_NORMAL));
            playlist_list_draw_string(pl, playlist_list_font,
                                      i - pl->pl_first, tail_width, title,
                                      i + 1);

            x = pl->pl_widget.x + width - width_approx_digits * 2;
            y = pl->pl_widget.y + ((i - pl->pl_first) -
                                   1) * pl->pl_fheight + ascent;

            if (entry->selected) {
                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_SELECTEDBG));
            }
            else {
                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_NORMALBG));
            }

            /* This isn't very cool, but i don't see a way to
             * calculate row widths with Pango fast enough here */

            gdk_draw_rectangle(obj, gc, TRUE,
                               pl->pl_widget.x + pl->pl_widget.width -
                               (width_approx_digits * 6), 
				 y + abs(descent),
                               (width_approx_digits * 6), pl->pl_fheight - 1);

            if (i == playlist_get_position_nolock())
                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_CURRENT));
            else
                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_NORMAL));

	    frags = NULL;
	    frag0 = NULL;

	    if ( (strlen(tail)>0) && (tail != NULL) ) {

            frags = g_strsplit(tail, ":", 0);
            frag0 = g_strconcat(frags[0], ":", NULL);

            layout = gtk_widget_create_pango_layout(playlistwin, frags[1]);
       	    pango_layout_set_font_description(layout, playlist_list_font);
            pango_layout_set_width(layout, tail_len * 100);
       	    pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
            gdk_draw_layout(obj, gc, x - (0.5 * width_approx_digits),
       	                    y + abs(descent), layout);
            g_object_unref(layout);

            layout = gtk_widget_create_pango_layout(playlistwin, frag0);
            pango_layout_set_font_description(layout, playlist_list_font);
            pango_layout_set_width(layout, tail_len * 100);
            pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
            gdk_draw_layout(obj, gc, x - (0.75 * width_approx_digits),
                            y + abs(descent), layout);
            g_object_unref(layout);

            g_free(frag0);
            g_strfreev(frags);

	    }

            if (pos != -1) {

                if (i == playlist_get_position_nolock())
                    gdk_gc_set_foreground(gc,
                                          skin_get_color(bmp_active_skin,
                                                         SKIN_PLEDIT_CURRENT));
                else
                    gdk_gc_set_foreground(gc,
                                          skin_get_color(bmp_active_skin,
                                                         SKIN_PLEDIT_NORMAL));

                /* DON'T remove the commented code yet please     -- Milosz */

                queue_tailpadding = 5;

                gdk_draw_rectangle(obj, gc, FALSE,
                                   x -
                                   (((queue_tailpadding +
                                      strlen(queuepos)) *
                                     width_approx_digits) +
                                    (width_approx_digits / 4)),
                                   y + abs(descent) + 1,
                                   (strlen(queuepos)) *
                                   width_approx_digits +
                                   (width_approx_digits / 2),
                                   pl->pl_fheight - 2);

                layout =
                    gtk_widget_create_pango_layout(playlistwin, queuepos);
                pango_layout_set_font_description(layout, playlist_list_font);
                pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

                gdk_draw_layout(obj, gc,
                                x -
                                ((queue_tailpadding +
                                  strlen(queuepos)) * width_approx_digits),
                                y + abs(descent), layout);
                g_object_unref(layout);

            }



        }
        else {
            if (i == playlist_get_position_nolock())
                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_CURRENT));
            else
                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_NORMAL));

            playlist_list_draw_string(pl, playlist_list_font,
                                      i - pl->pl_first, width, title, i + 1);
        }

        g_free(title);
    }


    /*
     * Drop target hovering over the playlist, so draw some hint where the
     * drop will occur.
     *
     * This is (currently? unfixably?) broken when dragging files from Qt/KDE apps,
     * probably due to DnD signaling problems (actually i have no clue).
     *
     */

    if (pl->pl_drag_motion) {
        guint pos, x, y, plx, ply, plength, lpadding;

        if (cfg.show_numbers_in_pl) {
            lpadding = gint_count_digits(playlist_get_length_nolock()) + 1;
            lpadding = ((lpadding + 1) * width_approx_digits);
        }
        else {
            lpadding = 3;
        };

        /* We already hold the mutex and have the playlist locked, so call
           the non-locking function. */
        plength = playlist_get_length_nolock();

        x = pl->drag_motion_x;
        y = pl->drag_motion_y;

        plx = pl->pl_widget.x;
        ply = pl->pl_widget.y;

        if ((x > pl->pl_widget.x) && !(x > pl->pl_widget.width)) {

            if ((y > pl->pl_widget.y)
                && !(y > (pl->pl_widget.height + ply))) {

                pos = ((y - ((Widget *) pl)->y) / pl->pl_fheight) +
                    pl->pl_first;

                if (pos > (plength)) {
                    pos = plength;
                }

                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_CURRENT));

                gdk_draw_line(obj, gc, pl->pl_widget.x,
/*		pl->pl_widget.x + lpadding + (width_approx_letters / 4),*/
                              pl->pl_widget.y +
                              ((pos - pl->pl_first) * pl->pl_fheight),
                              pl->pl_widget.width + pl->pl_widget.x - 1,
                              pl->pl_widget.y +
                              ((pos - pl->pl_first) * pl->pl_fheight));
            }

        }

        /* When dropping on the borders of the playlist, outside the text area,
         * files get appended at the end of the list. Show that too.
         */

        if ((y < ply) || (y > pl->pl_widget.height + ply)) {
            if ((y >= 0) || (y <= (pl->pl_widget.height + ply))) {
                pos = plength;
                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_CURRENT));

                gdk_draw_line(obj, gc, pl->pl_widget.x,
/*	pl->pl_widget.x + lpadding + (width_approx_letters / 4), */
                              pl->pl_widget.y +
                              ((pos - pl->pl_first) * pl->pl_fheight),
                              pl->pl_widget.width + pl->pl_widget.x - 1,
                              pl->pl_widget.y +
                              ((pos - pl->pl_first) * pl->pl_fheight));

            }
        }


    }

    gdk_gc_set_foreground(gc,
                          skin_get_color(bmp_active_skin,
                                         SKIN_PLEDIT_NORMAL));

    if (cfg.show_numbers_in_pl) {

        padding_plength = playlist_get_length_nolock();

        if (padding_plength == 0) {
            padding_dwidth = 0;
        }
        else {
            padding_dwidth = gint_count_digits(playlist_get_length_nolock());
        }

        padding =
            (padding_dwidth *
             width_approx_digits) + width_approx_digits;


        /* For italic or oblique fonts we add another half of the
         * approximate width */
        if (has_slant)
            padding += width_approx_digits_half;

        gdk_draw_line(obj, gc,
                      pl->pl_widget.x + padding,
                      pl->pl_widget.y,
                      pl->pl_widget.x + padding,
                      (pl->pl_widget.y + pl->pl_widget.height));
    }

    playlist_rect->x = 0;
    playlist_rect->y = 0;
    playlist_rect->width = plw_w;
    playlist_rect->height = plw_h;

    gdk_gc_set_clip_origin(gc, 0, 0);
    gdk_gc_set_clip_rectangle(gc, NULL);

    PLAYLIST_UNLOCK();
}


PlayList_List *
create_playlist_list(GList ** wlist,
                     GdkPixmap * parent,
                     GdkGC * gc,
                     gint x, gint y,
                     gint w, gint h)
{
    PlayList_List *pl;

    pl = g_new0(PlayList_List, 1);
    widget_init(&pl->pl_widget, parent, gc, x, y, w, h, TRUE);

    pl->pl_widget.button_press_cb =
        (WidgetButtonPressFunc) playlist_list_button_press_cb;
    pl->pl_widget.button_release_cb =
        (WidgetButtonReleaseFunc) playlist_list_button_release_cb;
    pl->pl_widget.motion_cb = (WidgetMotionFunc) playlist_list_motion_cb;
    pl->pl_widget.draw = playlist_list_draw;

    pl->pl_prev_selected = -1;
    pl->pl_prev_min = -1;
    pl->pl_prev_max = -1;

    widget_list_add(wlist, WIDGET(pl));

    return pl;
}

void
playlist_list_set_font(const gchar * font)
{

    /* Welcome to bad hack central 2k3 */

    gchar *font_lower;
    gint width_temp;
    gint width_temp_0;

    playlist_list_font = pango_font_description_from_string(font);

    text_get_extents(font,
                     "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz ",
                     &width_approx_letters, NULL, &ascent, &descent);

    width_approx_letters = (width_approx_letters / 53);

    /* Experimental: We don't weigh the 1 into total because it's width is almost always
     * very different from the rest
     */
    text_get_extents(font, "023456789", &width_approx_digits, NULL, NULL,
                     NULL);
    width_approx_digits = (width_approx_digits / 9);

    /* Precache some often used calculations */
    width_approx_digits_half = width_approx_digits / 2;

    /* FIXME: We assume that any other number is broader than the "1" */
    text_get_extents(font, "1", &width_temp, NULL, NULL, NULL);
    text_get_extents(font, "2", &width_temp_0, NULL, NULL, NULL);

    if (abs(width_temp_0 - width_temp) < 2) {
        width_delta_digit_one = 0;
    }
    else {
        width_delta_digit_one = ((width_temp_0 - width_temp) / 2) + 2;
    }

    text_get_extents(font, ":", &width_colon, NULL, NULL, NULL);
    width_colon_third = width_colon / 4;

    font_lower = g_utf8_strdown(font, strlen(font));
    /* This doesn't take any i18n into account, but i think there is none with TTF fonts
     * FIXME: This can probably be retrieved trough Pango too
     */
    has_slant = g_strstr_len(font_lower, strlen(font_lower), "oblique")
        || g_strstr_len(font_lower, strlen(font_lower), "italic");

    g_free(font_lower);
}
