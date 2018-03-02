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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "skinwin.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "skin.h"
#include "util.h"

#include <gdk/gdkx.h>


#define THUMBNAIL_WIDTH  90
#define THUMBNAIL_HEIGHT 40


enum SkinViewCols {
    SKIN_VIEW_COL_PREVIEW,
    SKIN_VIEW_COL_NAME,
    SKIN_VIEW_N_COLS
};


GList *skinlist = NULL;



static gchar *
get_thumbnail_filename(const gchar * path)
{
    gchar *basename, *pngname, *thumbname;

    g_return_val_if_fail(path != NULL, NULL);

    basename = g_path_get_basename(path);
    pngname = g_strconcat(basename, ".png", NULL);

    thumbname = g_build_filename(bmp_paths[BMP_PATH_SKIN_THUMB_DIR],
                                 pngname, NULL);

    g_free(basename);
    g_free(pngname);

    return thumbname;
}


static GdkPixbuf *
skin_get_preview(const gchar * path)
{
    GdkPixbuf *preview = NULL;
    gchar *dec_path, *preview_path;
    gboolean is_archive = FALSE;

    if (file_is_archive(path)) {
        if (!(dec_path = archive_decompress(path)))
            return NULL;

        is_archive = TRUE;
    }
    else {
        dec_path = g_strdup(path);
    }

    preview_path = find_file_recursively(dec_path, "main.bmp");

    if (preview_path) {
        preview = gdk_pixbuf_new_from_file(preview_path, NULL);
        g_free(preview_path);
    }

    if (is_archive)
        del_directory(dec_path);

    g_free(dec_path);

    return preview;
}


static GdkPixbuf *
skin_get_thumbnail(const gchar * path)
{
    GdkPixbuf *scaled = NULL;
    GdkPixbuf *preview;
    gchar *thumbname;

    g_return_val_if_fail(path != NULL, NULL);

    if (g_str_has_suffix(path, BMP_SKIN_THUMB_DIR_BASENAME))
        return NULL;

    thumbname = get_thumbnail_filename(path);

    if (g_file_test(thumbname, G_FILE_TEST_EXISTS)) {
        scaled = gdk_pixbuf_new_from_file(thumbname, NULL);
        g_free(thumbname);
        return scaled;
    }

    if (!(preview = skin_get_preview(path))) {
        g_free(thumbname);
        return NULL;
    }

    scaled = gdk_pixbuf_scale_simple(preview,
                                     THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT,
                                     GDK_INTERP_BILINEAR);
    g_object_unref(preview);

    gdk_pixbuf_save(scaled, thumbname, "png", NULL, NULL);
    g_free(thumbname);

    return scaled;
}

static void
skinlist_add(const gchar * filename)
{
    SkinNode *node;
    gchar *basename;

    g_return_if_fail(filename != NULL);

    node = g_new0(SkinNode, 1);
    node->path = g_strdup(filename);

    basename = g_path_get_basename(filename);

    if (file_is_archive(filename)) {
        node->name = archive_basename(basename);
        g_free(basename);
    }
    else {
        node->name = basename;
    }

    skinlist = g_list_prepend(skinlist, node);
}

static gboolean
scan_skindir_func(const gchar * path, const gchar * basename, gpointer data)
{
    if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
        if (file_is_archive(path)) {
            skinlist_add(path);
        }
    }
    else if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        skinlist_add(path);
    }

    return FALSE;
}

static void
scan_skindir(const gchar * path)
{
    GError *error = NULL;

    g_return_if_fail(path != NULL);

    if (path[0] == '.')
        return;

    if (!dir_foreach(path, scan_skindir_func, NULL, &error)) {
        g_warning("Failed to open directory (%s): %s", path, error->message);
        g_error_free(error);
        return;
    }
}

static gint
skinlist_compare_func(gconstpointer a, gconstpointer b)
{
    g_return_val_if_fail(a != NULL && SKIN_NODE(a)->name != NULL, 1);
    g_return_val_if_fail(b != NULL && SKIN_NODE(b)->name != NULL, 1);
    return strcasecmp(SKIN_NODE(a)->name, SKIN_NODE(b)->name);
}

static void
skin_free_func(gpointer data)
{
    g_return_if_fail(data != NULL);
    g_free(SKIN_NODE(data)->name);
    g_free(SKIN_NODE(data)->path);
    g_free(data);
}


static void
skinlist_clear(void)
{
    if (!skinlist)
        return;

    g_list_foreach(skinlist, (GFunc) skin_free_func, NULL);
    g_list_free(skinlist);
    skinlist = NULL;
}

void
skinlist_update(void)
{
    gchar *skinsdir;

    skinlist_clear();

    scan_skindir(bmp_paths[BMP_PATH_USER_SKIN_DIR]);
    scan_skindir(DATA_DIR G_DIR_SEPARATOR_S BMP_SKIN_DIR_BASENAME);

    skinsdir = getenv("SKINSDIR");
    if (skinsdir) {
        gchar **dir_list = g_strsplit(skinsdir, ":", 0);
        gchar **dir;

        for (dir = dir_list; *dir; dir++)
            scan_skindir(*dir);
        g_strfreev(dir_list);
    }

    skinlist = g_list_sort(skinlist, skinlist_compare_func);

    g_assert(skinlist != NULL);
}

void
skin_view_update(GtkTreeView * treeview)
{
    GtkTreeSelection *selection = NULL;
    GtkListStore *store;
    GtkTreeIter iter, iter_current_skin;
    GtkTreePath *path;

    GdkPixbuf *thumbnail;
    gchar *name;
    GList *entry;

    gtk_widget_set_sensitive(GTK_WIDGET(treeview), FALSE);

    store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

    gtk_list_store_clear(store);

    skinlist_update();

    for (entry = skinlist; entry; entry = g_list_next(entry)) {
        thumbnail = skin_get_thumbnail(SKIN_NODE(entry->data)->path);

        if (!thumbnail)
            continue;

        name = SKIN_NODE(entry->data)->name;

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           SKIN_VIEW_COL_PREVIEW, thumbnail,
                           SKIN_VIEW_COL_NAME, name, -1);
        g_object_unref(thumbnail);

        if (g_strstr_len(bmp_active_skin->path,
                         strlen(bmp_active_skin->path), name)) {
	    iter_current_skin = iter;
	}

        while (gtk_events_pending())
            gtk_main_iteration();
    }

    selection = gtk_tree_view_get_selection(treeview);
    gtk_tree_selection_select_iter(selection, &iter_current_skin);

    path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter_current_skin);
    gtk_tree_view_scroll_to_cell(treeview, path, NULL, TRUE, 0.5, 0.5);
    gtk_tree_path_free(path);

    gtk_widget_set_sensitive(GTK_WIDGET(treeview), TRUE);
}


static void
skin_view_on_cursor_changed(GtkTreeView * treeview,
                            gpointer data)
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;

    GList *node;
    gchar *name;
    gchar *comp = NULL;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, SKIN_VIEW_COL_NAME, &name, -1);

    /* FIXME: store name in skinlist */
    for (node = skinlist; node; node = g_list_next(node)) {
        comp = SKIN_NODE(node->data)->path;
        if (g_strrstr(comp, name))
            break;
    }

    g_free(name);

    bmp_active_skin_load(comp);
}


void
skin_view_realize(GtkTreeView * treeview)
{
    GtkListStore *store;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    GtkTreeSelection *selection;

    gtk_widget_show_all(GTK_WIDGET(treeview));
    
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);

    store = gtk_list_store_new(SKIN_VIEW_N_COLS, GDK_TYPE_PIXBUF,
                               G_TYPE_STRING);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 16);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview),
                                GTK_TREE_VIEW_COLUMN(column));

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "pixbuf",
                                        SKIN_VIEW_COL_PREVIEW, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "text",
                                        SKIN_VIEW_COL_NAME, NULL);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);

    g_signal_connect(treeview, "cursor-changed",
                     G_CALLBACK(skin_view_on_cursor_changed), NULL);
}
