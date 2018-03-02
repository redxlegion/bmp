/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2003  Haavard Kvaalen
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
#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <glib.h>


typedef enum {
    PLAYLIST_SORT_PATH,
    PLAYLIST_SORT_FILENAME,
    PLAYLIST_SORT_TITLE,
    PLAYLIST_SORT_DATE
} PlaylistSortType;

typedef enum {
    PLAYLIST_FORMAT_UNKNOWN = -1,
    PLAYLIST_FORMAT_M3U,
    PLAYLIST_FORMAT_PLS,
    PLAYLIST_FORMAT_COUNT
} PlaylistFormat;


#define PLAYLIST_ENTRY(x)  ((PlaylistEntry*)(x))
struct _PlaylistEntry {
    gchar *filename;
    gchar *title;
    gint length;
    gboolean selected;
};

typedef struct _PlaylistEntry PlaylistEntry;

PlaylistEntry *playlist_entry_new(const gchar * filename,
                                  const gchar * title, const gint len);
void playlist_entry_free(PlaylistEntry * entry);

void playlist_init(void);
void playlist_clear(void);
void playlist_delete(gboolean crop);

gboolean playlist_add(const gchar * filename);
gboolean playlist_ins(const gchar * filename, gint pos);
guint playlist_add_dir(const gchar * dir);
guint playlist_ins_dir(const gchar * dir, gint pos, gboolean background);
guint playlist_add_url(const gchar * url);
guint playlist_ins_url(const gchar * string, gint pos);

void playlist_play(void);
void playlist_set_info(const gchar * title, gint length, gint rate,
                       gint freq, gint nch);
void playlist_check_pos_current(void);
void playlist_next(void);
void playlist_prev(void);
void playlist_queue(void);
void playlist_queue_position(guint pos);
void playlist_queue_remove(guint pos);
gint playlist_queue_get_length(void);
gboolean playlist_is_position_queued(guint pos);
void playlist_clear_queue(void);
gint playlist_get_queue_position(PlaylistEntry * entry);
void playlist_eof_reached(void);
void playlist_set_position(guint pos);
gint playlist_get_length(void);
gint playlist_get_length_nolock(void);
gint playlist_get_position(void);
gint playlist_get_position_nolock(void);
gchar *playlist_get_info_text(void);
gint playlist_get_current_length(void);

gboolean playlist_save(const gchar * filename, PlaylistFormat format);
gboolean playlist_load(const gchar * filename);

GList *playlist_get(void);

void playlist_start_get_info_thread(void);
void playlist_stop_get_info_thread();
void playlist_start_get_info_scan(void);

void playlist_sort(PlaylistSortType type);
void playlist_sort_selected(PlaylistSortType type);

void playlist_reverse(void);
void playlist_random(void);
void playlist_remove_dead_files(void);

void playlist_fileinfo_current(void);
void playlist_fileinfo(guint pos);

void playlist_delete_index(guint pos);
void playlist_delete_filenames(GList * filenames);

const gchar *playlist_get_filename_to_play();
gchar *playlist_get_filename(guint pos);
gchar *playlist_get_songtitle(guint pos);
gint playlist_get_songtime(guint pos);

GList *playlist_get_selected(void);
GList *playlist_get_selected_list(void);
int playlist_get_num_selected(void);

void playlist_get_total_time(gulong * total_time, gulong * selection_time,
                             gboolean * total_more,
                             gboolean * selection_more);

void playlist_select_all(gboolean set);
void playlist_select_range(gint min, gint max, gboolean sel);
void playlist_select_invert_all(void);
gboolean playlist_select_invert(guint pos);

gboolean playlist_read_info_selection(void);
void playlist_read_info(guint pos);

void playlist_set_shuffle(gboolean shuffle);

void playlist_clear_selected(void);

GList *get_playlist_nth(guint);
gboolean playlist_set_current_name(const gchar * filename);
const gchar *playlist_get_current_name(void);
void playlist_new(void);

PlaylistFormat playlist_format_get_from_name(const gchar * filename);
gboolean is_playlist_name(const gchar * filename);

#define PLAYLIST_LOCK()    G_LOCK(playlist)
#define PLAYLIST_UNLOCK()  G_UNLOCK(playlist)

G_LOCK_EXTERN(playlist);

#endif
