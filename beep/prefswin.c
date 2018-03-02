/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
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
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>

#include "glade.h"

#include "plugin.h"
#include "pluginenum.h"
#include "input.h"
#include "effect.h"
#include "general.h"
#include "output.h"
#include "visualization.h"

#include "main.h"
#include "skin.h"
#include "urldecode.h"
#include "util.h"
#include "dnd.h"
#include "libbeep/configdb.h"

#include "mainwin.h"
#include "playlistwin.h"
#include "skinwin.h"
#include "playlist_list.h"


enum CategoryViewCols {
    CATEGORY_VIEW_COL_ICON,
    CATEGORY_VIEW_COL_NAME,
    CATEGORY_VIEW_COL_ID,
    CATEGORY_VIEW_N_COLS
};

enum PluginViewCols {
    PLUGIN_VIEW_COL_ACTIVE,
    PLUGIN_VIEW_COL_DESC,
    PLUGIN_VIEW_COL_FILENAME,
    PLUGIN_VIEW_COL_ID,
    PLUGIN_VIEW_N_COLS
};


typedef struct {
    const gchar *icon_path;
    const gchar *name;
    gint id;
} Category;

typedef struct {
    const gchar *name;
    const gchar *tag;
}
TitleFieldTag;

static GtkWidget *prefswin = NULL;

static Category categories[] = {
    {DATA_DIR "/images/appearance.png", N_("Appearance"), 1},
    {DATA_DIR "/images/eq.png",         N_("Equalizer"), 4},
    {DATA_DIR "/images/mouse.png",      N_("Mouse"), 2},
    {DATA_DIR "/images/playlist.png",   N_("Playlist"), 3},
    {DATA_DIR "/images/plugins.png",    N_("Plugins"), 0},
};

static gint n_categories = G_N_ELEMENTS(categories);

static TitleFieldTag title_field_tags[] = {
    { N_("Artist")     , "%p" },
    { N_("Album")      , "%a" },
    { N_("Title")      , "%t" },
    { N_("Tracknumber"), "%n" },
    { N_("Genre")      , "%g" },
    { N_("Filename")   , "%f" },
    { N_("Filepath")   , "%F" },
    { N_("Date")       , "%d" },
    { N_("Year")       , "%y" },
    { N_("Comment")    , "%c" }
};

static const guint n_title_field_tags = G_N_ELEMENTS(title_field_tags);


static GladeXML *
prefswin_get_xml(void)
{
    return GLADE_XML(g_object_get_data(G_OBJECT(prefswin), "glade-xml"));
}

static void
change_category(GtkNotebook * notebook,
                GtkTreeSelection * selection)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint index;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, CATEGORY_VIEW_COL_ID, &index, -1);
    gtk_notebook_set_current_page(notebook, index);
}

void
prefswin_set_category(gint index)
{
    GladeXML *xml;
    GtkWidget *notebook;
    
    g_return_if_fail(index >= 0 && index < n_categories);

    xml = prefswin_get_xml();
    notebook = glade_xml_get_widget(xml, "category_view");
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), index);
}


static void
input_plugin_open_prefs(GtkTreeView * treeview,
                        gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);
    input_configure(id);
}

static void
input_plugin_open_info(GtkTreeView * treeview,
                       gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);
    input_about(id);
}

static void
output_plugin_open_prefs(GtkComboBox * cbox,
                         gpointer data)
{
    output_configure(gtk_combo_box_get_active(cbox));
}

static void
output_plugin_open_info(GtkComboBox * cbox,
                        gpointer data)
{
    output_about(gtk_combo_box_get_active(cbox));
}

static void
general_plugin_open_prefs(GtkTreeView * treeview,
                          gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);
    general_configure(id);
}

static void
general_plugin_open_info(GtkTreeView * treeview,
			 gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
	return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);
    general_about(id);
}

static void
input_plugin_toggle(GtkCellRendererToggle * cell,
                    const gchar * path_str,
                    gpointer data)
{
    GtkTreeModel *model = GTK_TREE_MODEL(data);
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean fixed;
    gint pluginnr;
    gchar *filename, *basename;
    /*GList *diplist, *tmplist; */

    /* get toggled iter */
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter,
                       PLUGIN_VIEW_COL_ACTIVE, &fixed,
                       PLUGIN_VIEW_COL_ID, &pluginnr,
                       PLUGIN_VIEW_COL_FILENAME, &filename,
                       -1);

    basename = g_path_get_basename(filename);
    g_free(filename);

    /* do something with the value */
    fixed ^= 1;

    g_hash_table_replace(plugin_matrix, basename, GINT_TO_POINTER(fixed));
    /*  g_hash_table_foreach(pluginmatrix, (GHFunc) disp_matrix, NULL); */

    /* set new value */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       PLUGIN_VIEW_COL_ACTIVE, fixed, -1);

    /* clean up */
    gtk_tree_path_free(path);
}


static void
vis_plugin_toggle(GtkCellRendererToggle * cell,
                  const gchar * path_str,
                  gpointer data)
{
    GtkTreeModel *model = GTK_TREE_MODEL(data);
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean fixed;
    gint pluginnr;

    /* get toggled iter */
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter,
                       PLUGIN_VIEW_COL_ACTIVE, &fixed,
                       PLUGIN_VIEW_COL_ID, &pluginnr, -1);

    /* do something with the value */
    fixed ^= 1;

    enable_vis_plugin(pluginnr, fixed);

    /* set new value */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       PLUGIN_VIEW_COL_ACTIVE, fixed, -1);

    /* clean up */
    gtk_tree_path_free(path);
}

static void
effect_plugin_toggle(GtkCellRendererToggle * cell,
                  const gchar * path_str,
                  gpointer data)
{
    GtkTreeModel *model = GTK_TREE_MODEL(data);
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean fixed;
    gint pluginnr;

    /* get toggled iter */
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter,
                       PLUGIN_VIEW_COL_ACTIVE, &fixed,
                       PLUGIN_VIEW_COL_ID, &pluginnr, -1);

    /* do something with the value */
    fixed ^= 1;

    enable_effect_plugin(pluginnr, fixed);

    /* set new value */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       PLUGIN_VIEW_COL_ACTIVE, fixed, -1);

    /* clean up */
    gtk_tree_path_free(path);
}
static void
general_plugin_toggle(GtkCellRendererToggle * cell,
                      const gchar * path_str,
                      gpointer data)
{
    GtkTreeModel *model = GTK_TREE_MODEL(data);
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean fixed;
    gint pluginnr;

    /* get toggled iter */
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter,
                       PLUGIN_VIEW_COL_ACTIVE, &fixed,
                       PLUGIN_VIEW_COL_ID, &pluginnr, -1);

    /* do something with the value */
    fixed ^= 1;

    enable_general_plugin(pluginnr, fixed);

    /* set new value */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       PLUGIN_VIEW_COL_ACTIVE, fixed, -1);

    /* clean up */
    gtk_tree_path_free(path);
}

static void
on_output_plugin_cbox_changed(GtkComboBox * combobox,
                              gpointer data)
{
    gint selected;
    selected = gtk_combo_box_get_active(combobox);

    /* Force playback to stop. There is NO way to change the output
       plugin in the middle of a playback, and NO way to know when the
       user closes the output plugin settings dialog. */
    mainwin_stop_pushed();
    set_current_output_plugin(selected);
}

static void
on_output_plugin_cbox_realize(GtkComboBox * cbox,
                              gpointer data)
{
    GList *olist = get_output_list();
    OutputPlugin *op, *cp = get_current_output_plugin();
    gint i = 0, selected = 0;

    if (!olist) {
        gtk_widget_set_sensitive(GTK_WIDGET(cbox), FALSE);
        return;
    }

    for (i = 0; olist; i++, olist = g_list_next(olist)) {
        op = OUTPUT_PLUGIN(olist->data);

        if (olist->data == cp)
            selected = i;

        gtk_combo_box_append_text(cbox, op->description);
    }

    gtk_combo_box_set_active(cbox, selected);
    g_signal_connect(cbox, "changed",
                     G_CALLBACK(on_output_plugin_cbox_changed), NULL);
}


static void
on_input_plugin_view_realize(GtkTreeView * treeview,
                             gpointer data)
{
    GtkListStore *store;
    GtkTreeIter iter;

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    GList *ilist;
    gchar *description[2];
    InputPlugin *ip;
    gint id = 0;

    gboolean enabled;

    store = gtk_list_store_new(PLUGIN_VIEW_N_COLS,
                               G_TYPE_BOOLEAN, G_TYPE_STRING,
                               G_TYPE_STRING, G_TYPE_INT);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Enabled"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_fixed_width(column, 50);

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled",
                     G_CALLBACK(input_plugin_toggle), store);
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "active",
                                        PLUGIN_VIEW_COL_ACTIVE, NULL);

    gtk_tree_view_append_column(treeview, column);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Description"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, TRUE);


    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text", PLUGIN_VIEW_COL_DESC, NULL);
    gtk_tree_view_append_column(treeview, column);

    column = gtk_tree_view_column_new();

    gtk_tree_view_column_set_title(column, _("Filename"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, TRUE);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "text",
                                        PLUGIN_VIEW_COL_FILENAME, NULL);

    gtk_tree_view_append_column(treeview, column);

    for (ilist = get_input_list(); ilist; ilist = g_list_next(ilist)) {
        ip = INPUT_PLUGIN(ilist->data);

        description[0] = g_strdup(ip->description);
        description[1] = g_strdup(ip->filename);

        enabled = input_is_enabled(description[1]);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           PLUGIN_VIEW_COL_ACTIVE, enabled,
                           PLUGIN_VIEW_COL_DESC, description[0],
                           PLUGIN_VIEW_COL_FILENAME, description[1],
                           PLUGIN_VIEW_COL_ID, id++, -1);

        g_free(description[1]);
        g_free(description[0]);
    }

    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
}


static void
on_general_plugin_view_realize(GtkTreeView * treeview,
                               gpointer data)
{
    GtkListStore *store;
    GtkTreeIter iter;

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    GList *ilist /*, *diplist */ ;
    gchar *description[2];
    GeneralPlugin *gp;
    gint id = 0;

    gboolean enabled;

    store = gtk_list_store_new(PLUGIN_VIEW_N_COLS,
                               G_TYPE_BOOLEAN, G_TYPE_STRING,
                               G_TYPE_STRING, G_TYPE_INT);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Enabled"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_fixed_width(column, 50);

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled",
                     G_CALLBACK(general_plugin_toggle), store);
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "active",
                                        PLUGIN_VIEW_COL_ACTIVE, NULL);

    gtk_tree_view_append_column(treeview, column);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Description"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, TRUE);


    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text", PLUGIN_VIEW_COL_DESC, NULL);

    gtk_tree_view_append_column(treeview, column);


    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Filename"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, TRUE);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "text",
                                        PLUGIN_VIEW_COL_FILENAME, NULL);

    gtk_tree_view_append_column(treeview, column);

    for (ilist = get_general_list(); ilist; ilist = g_list_next(ilist)) {
        gp = GENERAL_PLUGIN(ilist->data);

        description[0] = g_strdup(gp->description);
        description[1] = g_strdup(gp->filename);

        enabled = general_enabled(id);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           PLUGIN_VIEW_COL_ACTIVE, enabled,
                           PLUGIN_VIEW_COL_DESC, description[0],
                           PLUGIN_VIEW_COL_FILENAME, description[1],
                           PLUGIN_VIEW_COL_ID, id++, -1);

        g_free(description[1]);
        g_free(description[0]);
    }

    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
}


static void
on_vis_plugin_view_realize(GtkTreeView * treeview,
                           gpointer data)
{
    GtkListStore *store;
    GtkTreeIter iter;

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    GList *vlist;
    gchar *description[2];
    VisPlugin *vp;
    gint id = 0;

    gboolean enabled;


    store = gtk_list_store_new(PLUGIN_VIEW_N_COLS,
                               G_TYPE_BOOLEAN, G_TYPE_STRING,
                               G_TYPE_STRING, G_TYPE_INT);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Enabled"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_fixed_width(column, 50);

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled",
                     G_CALLBACK(vis_plugin_toggle), store);
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "active",
                                        PLUGIN_VIEW_COL_ACTIVE, NULL);

    gtk_tree_view_append_column(treeview, column);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Description"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, TRUE);


    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text", PLUGIN_VIEW_COL_DESC, NULL);

    gtk_tree_view_append_column(treeview, column);


    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Filename"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, TRUE);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "text",
                                        PLUGIN_VIEW_COL_FILENAME, NULL);

    gtk_tree_view_append_column(treeview, column);

    for (vlist = get_vis_list(); vlist; vlist = g_list_next(vlist)) {
        vp = VIS_PLUGIN(vlist->data);

        description[0] = g_strdup(vp->description);
        description[1] = g_strdup(vp->filename);

        enabled = vis_enabled(id);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           PLUGIN_VIEW_COL_ACTIVE, enabled,
                           PLUGIN_VIEW_COL_DESC, description[0],
                           PLUGIN_VIEW_COL_FILENAME, description[1],
                           PLUGIN_VIEW_COL_ID, id++, -1);

        g_free(description[1]);
        g_free(description[0]);
    }

    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
}

static void
editable_insert_text(GtkEditable * editable,
                     const gchar * text,
                     gint * pos)
{
    gtk_editable_insert_text(editable, text, strlen(text), pos);
}


static void
on_effect_plugin_view_realize(GtkTreeView * treeview,
                              gpointer data)
{
    GtkListStore *store;
    GtkTreeIter iter;

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    GList *elist;
    gchar *description[2];
    gint id = 0;

    gboolean enabled;


    store = gtk_list_store_new(PLUGIN_VIEW_N_COLS,
                               G_TYPE_BOOLEAN, G_TYPE_STRING,
                               G_TYPE_STRING, G_TYPE_INT);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Enabled"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_fixed_width(column, 50);

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled",
                     G_CALLBACK(effect_plugin_toggle), store);
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "active",
                                        PLUGIN_VIEW_COL_ACTIVE, NULL);

    gtk_tree_view_append_column(treeview, column);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Description"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, TRUE);


    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text", PLUGIN_VIEW_COL_DESC, NULL);

    gtk_tree_view_append_column(treeview, column);


    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Filename"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, TRUE);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "text",
                                        PLUGIN_VIEW_COL_FILENAME, NULL);

    gtk_tree_view_append_column(treeview, column);

    for (elist = get_effect_list(); elist; elist = g_list_next(elist)) {
        EffectPlugin *ep = EFFECT_PLUGIN(elist->data);

        description[0] = g_strdup(ep->description);
        description[1] = g_strdup(ep->filename);

        enabled = effect_enabled(id);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           PLUGIN_VIEW_COL_ACTIVE, enabled,
                           PLUGIN_VIEW_COL_DESC, description[0],
                           PLUGIN_VIEW_COL_FILENAME, description[1],
                           PLUGIN_VIEW_COL_ID, id++, -1);

        g_free(description[1]);
        g_free(description[0]);
    }

    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
}

static void
titlestring_tag_menu_callback(GtkMenuItem * menuitem,
                              gpointer data)
{
    const gchar *separator = " - ";
    GladeXML *xml;
    GtkWidget *entry;
    gint item = GPOINTER_TO_INT(data);
    gint pos;
    
    xml = prefswin_get_xml();
    entry = glade_xml_get_widget(xml, "titlestring_entry");

    pos = gtk_editable_get_position(GTK_EDITABLE(entry));

    /* insert separator as needed */
    if (g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(entry)), -1) > 0)
        editable_insert_text(GTK_EDITABLE(entry), separator, &pos);

    editable_insert_text(GTK_EDITABLE(entry), _(title_field_tags[item].tag),
                         &pos);

    gtk_editable_set_position(GTK_EDITABLE(entry), pos);
}

static void
on_titlestring_help_button_clicked(GtkButton * button,
                                   gpointer data) 
{
    
    GtkMenu *menu;
    MenuPos *pos = g_new0(MenuPos, 1);
    GdkWindow *parent;
  
    gint x_ro, y_ro;
    gint x_widget, y_widget;
    gint x_size, y_size;
  
    g_return_if_fail (button != NULL);
    g_return_if_fail (GTK_IS_MENU (data));

    parent = gtk_widget_get_parent_window(GTK_WIDGET(button));
  
    gdk_drawable_get_size(parent, &x_size, &y_size);	 
    gdk_window_get_root_origin(GTK_WIDGET(button)->window, &x_ro, &y_ro); 
    gdk_window_get_position(GTK_WIDGET(button)->window, &x_widget, &y_widget);
  
    pos->x = x_size + x_ro;
    pos->y = y_size + y_ro - 100;
  
    menu = GTK_MENU(data);
    gtk_menu_popup (menu, NULL, NULL, util_menu_position, pos, 
                    0, GDK_CURRENT_TIME);
}


static void
on_titlestring_entry_realize(GtkWidget * entry,
                             gpointer data)
{
    gtk_entry_set_text(GTK_ENTRY(entry), cfg.gentitle_format);
}

static void
on_titlestring_entry_changed(GtkWidget * entry,
                             gpointer data) 
{
    g_free(cfg.gentitle_format);
    cfg.gentitle_format = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void
on_titlestring_cbox_realize(GtkWidget * cbox,
                            gpointer data)
{
    gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), cfg.titlestring_preset);
    gtk_widget_set_sensitive(GTK_WIDGET(data), 
                             (cfg.titlestring_preset == n_titlestring_presets));
}

static void
on_titlestring_cbox_changed(GtkWidget * cbox,
                            gpointer data)
{
    gint position = gtk_combo_box_get_active(GTK_COMBO_BOX(cbox));
    
    cfg.titlestring_preset = position;
    gtk_widget_set_sensitive(GTK_WIDGET(data), (position == 4));
}

static void
on_mainwin_font_button_font_set(GtkFontButton * button,
                                gpointer data)
{
    g_free(cfg.mainwin_font);
    cfg.mainwin_font = g_strdup(gtk_font_button_get_font_name(button));

    textbox_set_xfont(mainwin_info, TRUE, cfg.mainwin_font);
    mainwin_set_info_text();
    draw_main_window(TRUE);
}

static void
on_mainwin_font_button_realize(GtkFontButton * button,
                               gpointer data)
{
    gtk_font_button_set_font_name(button, cfg.mainwin_font);
}

static void
on_playlist_font_button_font_set(GtkFontButton * button,
                                 gpointer data)
{
    g_free(cfg.playlist_font);
    cfg.playlist_font = g_strdup(gtk_font_button_get_font_name(button));

    playlist_list_set_font(cfg.playlist_font);
    playlistwin_update_list();
    draw_playlist_window(TRUE);
}

static void
on_playlist_font_button_realize(GtkFontButton * button,
                                gpointer data)
{
    gtk_font_button_set_font_name(button, cfg.playlist_font);
}

static void
on_playlist_show_pl_numbers_realize(GtkToggleButton * button,
                                    gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.show_numbers_in_pl);
}

static void
on_playlist_show_pl_numbers_toggled(GtkToggleButton * button,
                                    gpointer data)
{
    cfg.show_numbers_in_pl = gtk_toggle_button_get_active(button);
    playlistwin_update_list();
    draw_playlist_window(TRUE);
}

static void
input_plugin_enable_prefs(GtkTreeView * treeview,
                          GtkButton * button)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;

    GList *plist;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);

    plist = get_input_list();
    plist = g_list_nth(plist, id);

    gtk_widget_set_sensitive(GTK_WIDGET(button),
                             INPUT_PLUGIN(plist->data)->configure != NULL);
}

static void
input_plugin_enable_info(GtkTreeView * treeview,
                         GtkButton * button)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *plist;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);

    plist = get_input_list();
    plist = g_list_nth(plist, id);

    gtk_widget_set_sensitive(GTK_WIDGET(button),
                             INPUT_PLUGIN(plist->data)->about != NULL);
}


static void
output_plugin_enable_info(GtkComboBox * cbox, GtkButton * button)
{
    GList *plist;

    gint id = gtk_combo_box_get_active(cbox);

    plist = get_output_list();
    plist = g_list_nth(plist, id);

    gtk_widget_set_sensitive(GTK_WIDGET(button),
                             OUTPUT_PLUGIN(plist->data)->about != NULL);
}

static void
output_plugin_enable_prefs(GtkComboBox * cbox, GtkButton * button)
{
    GList *plist;
    gint id = gtk_combo_box_get_active(cbox);

    plist = get_output_list();
    plist = g_list_nth(plist, id);

    gtk_widget_set_sensitive(GTK_WIDGET(button),
                             OUTPUT_PLUGIN(plist->data)->configure != NULL);
}


static void
general_plugin_enable_info(GtkTreeView * treeview,
                           GtkButton * button)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *plist;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);

    plist = get_general_list();
    plist = g_list_nth(plist, id);

    gtk_widget_set_sensitive(GTK_WIDGET(button),
                             GENERAL_PLUGIN(plist->data)->about != NULL);
}

static void
general_plugin_enable_prefs(GtkTreeView * treeview,
                            GtkButton * button)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *plist;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);

    plist = get_general_list();
    plist = g_list_nth(plist, id);

    gtk_widget_set_sensitive(GTK_WIDGET(button),
                             GENERAL_PLUGIN(plist->data)->configure != NULL);
}



static void
vis_plugin_enable_prefs(GtkTreeView * treeview,
                            GtkButton * button)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *plist;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);

    plist = get_vis_list();
    plist = g_list_nth(plist, id);

    gtk_widget_set_sensitive(GTK_WIDGET(button),
                             VIS_PLUGIN(plist->data)->configure != NULL);
}

static void
vis_plugin_enable_info(GtkTreeView * treeview,
                           GtkButton * button)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *plist;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);

    plist = get_vis_list();
    plist = g_list_nth(plist, id);

    gtk_widget_set_sensitive(GTK_WIDGET(button),
                             VIS_PLUGIN(plist->data)->about != NULL);
}

static void
vis_plugin_open_prefs(GtkTreeView * treeview,
                          gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);
    vis_configure(id);
}


static void
vis_plugin_open_info(GtkTreeView * treeview,
			 gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
	return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);
    vis_about(id);
}






static void
effect_plugin_enable_prefs(GtkTreeView * treeview,
                            GtkButton * button)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *plist;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);

    plist = get_effect_list();
    plist = g_list_nth(plist, id);

    gtk_widget_set_sensitive(GTK_WIDGET(button),
                             EFFECT_PLUGIN(plist->data)->configure != NULL);
}

static void
effect_plugin_enable_info(GtkTreeView * treeview,
                           GtkButton * button)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *plist;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);

    plist = get_effect_list();
    plist = g_list_nth(plist, id);

    gtk_widget_set_sensitive(GTK_WIDGET(button),
                             EFFECT_PLUGIN(plist->data)->about != NULL);
}

static void
effect_plugin_open_prefs(GtkTreeView * treeview,
                          gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);
    effect_configure(id);
}


static void
effect_plugin_open_info(GtkTreeView * treeview,
			 gpointer data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint id;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
	return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_ID, &id, -1);
    effect_about(id);
}





static void
on_mouse_wheel_volume_realize(GtkSpinButton * button,
                              gpointer data)
{
    gtk_spin_button_set_value(button, cfg.mouse_change);
}

static void
on_mouse_wheel_volume_changed(GtkSpinButton * button,
                              gpointer data)
{
    cfg.mouse_change = gtk_spin_button_get_value_as_int(button);
}

static void
on_pause_between_songs_time_realize(GtkSpinButton * button,
                                    gpointer data)
{
    gtk_spin_button_set_value(button, cfg.pause_between_songs_time);
}

static void
on_pause_between_songs_time_changed(GtkSpinButton * button,
                                    gpointer data)
{
    cfg.pause_between_songs_time = gtk_spin_button_get_value_as_int(button);
}

static void
on_mouse_wheel_scroll_pl_realize(GtkSpinButton * button,
                                 gpointer data)
{
    gtk_spin_button_set_value(button, cfg.scroll_pl_by);
}

static void
on_mouse_wheel_scroll_pl_changed(GtkSpinButton * button,
                                 gpointer data)
{
    cfg.scroll_pl_by = gtk_spin_button_get_value_as_int(button);
}

static void
on_playlist_convert_underscore_realize(GtkToggleButton * button,
                                       gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.convert_underscore);
}

static void
on_playlist_convert_underscore_toggled(GtkToggleButton * button,
                                       gpointer data)
{
    cfg.convert_underscore = gtk_toggle_button_get_active(button);
}

static void
on_playlist_no_advance_realize(GtkToggleButton * button, gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.no_playlist_advance);
}

static void
on_playlist_no_advance_toggled(GtkToggleButton * button, gpointer data)
{
    cfg.no_playlist_advance = gtk_toggle_button_get_active(button);
}

static void
on_playlist_convert_twenty_realize(GtkToggleButton * button, gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.convert_twenty);
}

static void
on_playlist_convert_twenty_toggled(GtkToggleButton * button, gpointer data)
{
    cfg.convert_twenty = gtk_toggle_button_get_active(button);
}

#if 0
static void
on_playlist_update_clicked(GtkButton * button,
                           gpointer data)
{
    playlistwin_update_list();
    draw_playlist_window(TRUE);
}
#endif

static void
on_use_pl_metadata_realize(GtkToggleButton * button,
                           gpointer data)
{
    gboolean state = cfg.use_pl_metadata;
    gtk_toggle_button_set_active(button, state);
    gtk_widget_set_sensitive(GTK_WIDGET(data), state);
}

static void
on_use_pl_metadata_toggled(GtkToggleButton * button,
                           gpointer data)
{
    gboolean state = gtk_toggle_button_get_active(button);
    cfg.use_pl_metadata = state;
    gtk_widget_set_sensitive(GTK_WIDGET(data), state);
}

static void
on_pause_between_songs_realize(GtkToggleButton * button,
                               gpointer data)
{
    gboolean state = cfg.pause_between_songs;
    gtk_toggle_button_set_active(button, state);
    gtk_widget_set_sensitive(GTK_WIDGET(data), state);
}

static void
on_pause_between_songs_toggled(GtkToggleButton * button,
                               gpointer data)
{
    gboolean state = gtk_toggle_button_get_active(button);
    cfg.pause_between_songs = state;
    gtk_widget_set_sensitive(GTK_WIDGET(data), state);
}

static void
on_pl_metadata_on_load_realize(GtkRadioButton * button,
                               gpointer data)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 cfg.get_info_on_load);
}

static void
on_pl_metadata_on_display_realize(GtkRadioButton * button,
                                  gpointer data)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 cfg.get_info_on_demand);
}

static void
on_pl_metadata_on_load_toggled(GtkRadioButton * button,
                               gpointer data)
{
    cfg.get_info_on_load = 
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
}

static void
on_pl_metadata_on_display_toggled(GtkRadioButton * button,
                                  gpointer data)
{
    cfg.get_info_on_demand =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
}

static void
on_custom_cursors_realize(GtkToggleButton * button,
                          gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.custom_cursors);
}

static void
on_custom_cursors_toggled(GtkToggleButton *togglebutton,
                          gpointer data)
{
    cfg.custom_cursors = gtk_toggle_button_get_active(togglebutton);
    skin_reload_forced();
}

static void
on_eq_dir_preset_entry_realize(GtkEntry * entry,
                               gpointer data)
{
    gtk_entry_set_text(entry, cfg.eqpreset_default_file);
}

static void
on_eq_dir_preset_entry_changed(GtkEntry * entry,
                               gpointer data)
{
    g_free(cfg.eqpreset_default_file);
    cfg.eqpreset_default_file = g_strdup(gtk_entry_get_text(entry));
}

static void
on_eq_file_preset_entry_realize(GtkEntry * entry,
                                gpointer data)
{
    gtk_entry_set_text(entry, cfg.eqpreset_extension);
}

static void
on_eq_file_preset_entry_changed(GtkEntry * entry, gpointer data)
{
    const gchar *text = gtk_entry_get_text(entry);

    while (*text == '.')
        text++;

    g_free(cfg.eqpreset_extension);
    cfg.eqpreset_extension = g_strdup(text);
}


/* FIXME: implement these */

static void
on_eq_preset_view_realize(GtkTreeView * treeview,
                          gpointer data)
{}

static void
on_eq_preset_add_clicked(GtkButton * button,
                         gpointer data)
{}

static void
on_eq_preset_remove_clicked(GtkButton * button,
                            gpointer data)
{}


static void
prefswin_set_skin_update(gboolean state)
{
    g_object_set_data(G_OBJECT(prefswin), "update-skins",
                      GINT_TO_POINTER(state));
}

static gboolean
prefswin_get_skin_update(void)
{
    return (gboolean) g_object_get_data(G_OBJECT(prefswin), "update-skins");
}

static gboolean
on_skin_view_visibility_notify(GtkTreeView * treeview,
                               GdkEvent * event,
                               gpointer data)
{
    if (event->visibility.state == GDK_VISIBILITY_FULLY_OBSCURED)
        return FALSE;

    if (!prefswin_get_skin_update())
        return FALSE;

    prefswin_set_skin_update(FALSE);
    skin_view_update(treeview);

    return TRUE;
}

static void
on_category_view_realize(GtkTreeView * treeview,
                         GtkNotebook * notebook)
{
    GtkListStore *store;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GdkPixbuf *img;
    gint i;

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Category"));
    gtk_tree_view_append_column(treeview, column);
    gtk_tree_view_column_set_spacing(column, 2);

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "pixbuf", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "text", 1, NULL);

    store = gtk_list_store_new(CATEGORY_VIEW_N_COLS,
                               GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));

    for (i = 0; i < n_categories; i++) {
        img = gdk_pixbuf_new_from_file(categories[i].icon_path, NULL);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           CATEGORY_VIEW_COL_ICON, img,
                           CATEGORY_VIEW_COL_NAME,
                           gettext(categories[i].name), CATEGORY_VIEW_COL_ID,
                           categories[i].id, -1);
        g_object_unref(img);
    }

    selection = gtk_tree_view_get_selection(treeview);

    g_signal_connect_swapped(selection, "changed",
                             G_CALLBACK(change_category), notebook);
}

static void
mainwin_drag_data_received1(GtkWidget * widget,
                            GdkDragContext * context,
                            gint x, gint y,
                            GtkSelectionData * selection_data,
                            guint info, guint time,
                            gpointer user_data) 
{
    gchar *path, *decoded;

    if (!selection_data->data) {
        g_warning("DND data string is NULL");
        return;
    }

    path = (gchar *) selection_data->data;

    /* FIXME: use a real URL validator/parser */

    if (!str_has_prefix_nocase(path, "fonts:///"))
        return;

    path[strlen(path) - 2] = 0; /* Why the hell a CR&LF? */
    path += 8;

    /* plain, since we already stripped the first URI part */
    decoded = xmms_urldecode_plain(path);

    /* Get the old font's size, and add it to the dropped
     * font's name */
    cfg.playlist_font = g_strconcat(decoded+1,
                                    strrchr(cfg.playlist_font, ' '),
                                    NULL);
    playlist_list_set_font(cfg.playlist_font);
    playlistwin_update_list();
    gtk_font_button_set_font_name(user_data, cfg.playlist_font);	
    
    g_free(decoded);
}

static void
on_skin_view_drag_data_received(GtkWidget * widget,
                                GdkDragContext * context,
                                gint x, gint y,
                                GtkSelectionData * selection_data,
                                guint info, guint time,
                                gpointer user_data) 
{
    ConfigDb *db;
    gchar *path;

    if (!selection_data->data) {
        g_warning("DND data string is NULL");
        return;
    }

    path = (gchar *) selection_data->data;

    /* FIXME: use a real URL validator/parser */

    if (str_has_prefix_nocase(path, "file:///")) {
        path[strlen(path) - 2] = 0; /* Why the hell a CR&LF? */
        path += 7;
    }
    else if (str_has_prefix_nocase(path, "file:")) {
        path += 5;
    }

    if (file_is_archive(path)) {
        bmp_active_skin_load(path);
        skin_install_skin(path);
	skin_view_update(GTK_TREE_VIEW(widget));
        /* Change skin name in the config file */
        db = bmp_cfg_db_open();
        bmp_cfg_db_set_string(db, NULL, "skin", path);
        bmp_cfg_db_close(db);
    }
			   			   
}

/* FIXME: complete the map */
FUNC_MAP_BEGIN(prefswin_func_map)
    FUNC_MAP_ENTRY(on_input_plugin_view_realize)
    FUNC_MAP_ENTRY(on_output_plugin_cbox_realize)
    FUNC_MAP_ENTRY(on_general_plugin_view_realize)
    FUNC_MAP_ENTRY(on_vis_plugin_view_realize)
    FUNC_MAP_ENTRY(on_effect_plugin_view_realize)
    FUNC_MAP_ENTRY(on_custom_cursors_realize)
    FUNC_MAP_ENTRY(on_custom_cursors_toggled)
    FUNC_MAP_ENTRY(on_mainwin_font_button_realize)
    FUNC_MAP_ENTRY(on_mainwin_font_button_font_set)
    FUNC_MAP_ENTRY(on_mouse_wheel_volume_realize)
    FUNC_MAP_ENTRY(on_mouse_wheel_volume_changed)
    FUNC_MAP_ENTRY(on_mouse_wheel_scroll_pl_realize)
    FUNC_MAP_ENTRY(on_mouse_wheel_scroll_pl_changed)
    FUNC_MAP_ENTRY(on_pause_between_songs_time_realize)
    FUNC_MAP_ENTRY(on_pause_between_songs_time_changed)
    FUNC_MAP_ENTRY(on_pl_metadata_on_load_realize)
    FUNC_MAP_ENTRY(on_pl_metadata_on_load_toggled)
    FUNC_MAP_ENTRY(on_pl_metadata_on_display_realize)
    FUNC_MAP_ENTRY(on_pl_metadata_on_display_toggled)
    FUNC_MAP_ENTRY(on_playlist_show_pl_numbers_realize)
    FUNC_MAP_ENTRY(on_playlist_show_pl_numbers_toggled)
    FUNC_MAP_ENTRY(on_playlist_convert_twenty_realize)
    FUNC_MAP_ENTRY(on_playlist_convert_twenty_toggled)
    FUNC_MAP_ENTRY(on_playlist_convert_underscore_realize)
    FUNC_MAP_ENTRY(on_playlist_convert_underscore_toggled)
    FUNC_MAP_ENTRY(on_playlist_font_button_realize)
    FUNC_MAP_ENTRY(on_playlist_font_button_font_set)
    FUNC_MAP_ENTRY(on_playlist_no_advance_realize)
    FUNC_MAP_ENTRY(on_playlist_no_advance_toggled)
    FUNC_MAP_ENTRY(on_skin_view_visibility_notify)
    FUNC_MAP_ENTRY(on_titlestring_entry_realize)
    FUNC_MAP_ENTRY(on_titlestring_entry_changed)
    FUNC_MAP_ENTRY(on_eq_dir_preset_entry_realize)
    FUNC_MAP_ENTRY(on_eq_dir_preset_entry_changed)
    FUNC_MAP_ENTRY(on_eq_file_preset_entry_realize)
    FUNC_MAP_ENTRY(on_eq_file_preset_entry_changed)
    FUNC_MAP_ENTRY(on_eq_preset_view_realize)
    FUNC_MAP_ENTRY(on_eq_preset_add_clicked)
    FUNC_MAP_ENTRY(on_eq_preset_remove_clicked)
FUNC_MAP_END

void
create_prefs_window(void)
{
    const gchar *glade_file = DATA_DIR "/glade/prefswin.glade";

    GladeXML *xml;
    GtkWidget *widget, *widget2;

    GtkWidget *titlestring_tag_menu, *menu_item;
    gint i;
        
    /* load the interface */
    xml = glade_xml_new_or_die(_("Preferences Window"), glade_file, NULL,
                               NULL);


    /* connect the signals in the interface */
    glade_xml_signal_autoconnect_map(xml, prefswin_func_map);

    prefswin = glade_xml_get_widget(xml, "prefswin");
    g_object_set_data(G_OBJECT(prefswin), "glade-xml", xml);
    gtk_window_set_transient_for(GTK_WINDOW(prefswin), GTK_WINDOW(mainwin));

    /* create category view */
    widget = glade_xml_get_widget(xml, "category_view");
    widget2 = glade_xml_get_widget(xml, "category_notebook");
    g_signal_connect_after(G_OBJECT(widget), "realize",
                           G_CALLBACK(on_category_view_realize),
                           widget2);

    /* plugin->input page */

    widget = glade_xml_get_widget(xml, "input_plugin_view");
    widget2 = glade_xml_get_widget(xml, "input_plugin_prefs");
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(input_plugin_enable_prefs),
                     widget2);

    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(input_plugin_open_prefs),
                             widget);
    widget2 = glade_xml_get_widget(xml, "input_plugin_info");
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(input_plugin_enable_info),
                     widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(input_plugin_open_info),
                             widget);

    /* plugin->output page */

    widget = glade_xml_get_widget(xml, "output_plugin_cbox");

    widget2 = glade_xml_get_widget(xml, "output_plugin_prefs");
    g_signal_connect(G_OBJECT(widget), "changed",
                     G_CALLBACK(output_plugin_enable_prefs),
                     widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(output_plugin_open_prefs),
                             widget);

    widget2 = glade_xml_get_widget(xml, "output_plugin_info");
    g_signal_connect(G_OBJECT(widget), "changed",
                     G_CALLBACK(output_plugin_enable_info),
                     widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(output_plugin_open_info),
                             widget);

    /* plugin->general page */

    widget = glade_xml_get_widget(xml, "general_plugin_view");

    widget2 = glade_xml_get_widget(xml, "general_plugin_prefs");
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(general_plugin_enable_prefs),
                     widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(general_plugin_open_prefs),
                             widget);

    widget2 = glade_xml_get_widget(xml, "general_plugin_info");
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(general_plugin_enable_info),
                     widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(general_plugin_open_info),
                             widget);


    /* plugin->vis page */

    widget = glade_xml_get_widget(xml, "vis_plugin_view");
    widget2 = glade_xml_get_widget(xml, "vis_plugin_prefs");

    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(vis_plugin_open_prefs),
                             widget);
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(vis_plugin_enable_prefs), widget2);


    widget2 = glade_xml_get_widget(xml, "vis_plugin_info");
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(vis_plugin_enable_info), widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(vis_plugin_open_info),
                             widget);


    /* plugin->effects page */

    widget = glade_xml_get_widget(xml, "effect_plugin_view");
    widget2 = glade_xml_get_widget(xml, "effect_plugin_prefs");

    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(effect_plugin_open_prefs),
                             widget);
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(effect_plugin_enable_prefs), widget2);


    widget2 = glade_xml_get_widget(xml, "effect_plugin_info");
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(effect_plugin_enable_info), widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(effect_plugin_open_info),
                             widget);

    /* playlist page */

    widget = glade_xml_get_widget(xml, "pause_between_songs_box");
    widget2 = glade_xml_get_widget(xml, "pause_between_songs");
    g_signal_connect_after(G_OBJECT(widget2), "realize",
                           G_CALLBACK(on_pause_between_songs_realize),
                           widget);
    g_signal_connect(G_OBJECT(widget2), "toggled",
                     G_CALLBACK(on_pause_between_songs_toggled),
                     widget);

    widget = glade_xml_get_widget(xml, "playlist_use_metadata_box");
    widget2 = glade_xml_get_widget(xml, "playlist_use_metadata");
    g_signal_connect_after(G_OBJECT(widget2), "realize",
                           G_CALLBACK(on_use_pl_metadata_realize),
                           widget);
    g_signal_connect(G_OBJECT(widget2), "toggled",
                     G_CALLBACK(on_use_pl_metadata_toggled),
                     widget);

    widget = glade_xml_get_widget(xml, "skin_view");
    g_signal_connect(widget, "drag-data-received",
                     G_CALLBACK(on_skin_view_drag_data_received),
                     NULL);
    bmp_drag_dest_set(widget);

    g_signal_connect(mainwin, "drag-data-received",
                     G_CALLBACK(mainwin_drag_data_received),
                     widget);

    widget = glade_xml_get_widget(xml, "playlist_font_button");    
    g_signal_connect(mainwin, "drag-data-received",
                     G_CALLBACK(mainwin_drag_data_received1),
                     widget);

    widget = glade_xml_get_widget(xml, "titlestring_cbox");
    widget2 = glade_xml_get_widget(xml, "titlestring_entry");
    g_signal_connect(widget, "realize",
                     G_CALLBACK(on_titlestring_cbox_realize),
                     widget2);
    g_signal_connect(widget, "changed",
                     G_CALLBACK(on_titlestring_cbox_changed),
                     widget2);

    /* FIXME: move this into a function */
    /* create tag menu */
    titlestring_tag_menu = gtk_menu_new();
    for(i = 0; i < n_title_field_tags; i++) {
    	menu_item = gtk_menu_item_new_with_label(_(title_field_tags[i].name));
	gtk_menu_shell_append(GTK_MENU_SHELL(titlestring_tag_menu), menu_item);
        g_signal_connect(menu_item, "activate",
                         G_CALLBACK(titlestring_tag_menu_callback), 
                         GINT_TO_POINTER(i));
    };
    gtk_widget_show_all(titlestring_tag_menu);
    
    widget = glade_xml_get_widget(xml, "titlestring_help_button");
    widget2 = glade_xml_get_widget(xml, "titlestring_cbox");

    g_signal_connect(widget2, "changed",
                     G_CALLBACK(on_titlestring_cbox_changed),
                     widget);
    g_signal_connect(widget, "clicked",
                     G_CALLBACK(on_titlestring_help_button_clicked),
                     titlestring_tag_menu);
}

void
show_prefs_window(void)
{
    prefswin_set_skin_update(TRUE);
    gtk_widget_show(prefswin);
}
