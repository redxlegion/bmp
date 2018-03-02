/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 2000-2003  Haavard Kvaalen
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

#include "about.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "beep_logo.xpm"


enum {
    COL_LEFT,
    COL_RIGHT,
    N_COLS
};


static const gchar *bmp_brief =
    N_("<big><b>Beep Media Player %s</b></big>\n"
       "A media player based on XMMS\n"
       "\n"
       "Copyright (C) 2003-2005 BMP Development Team\n");

static const gchar *credit_text[] = {
    N_("Developers:"),
    N_("Artem Baguinski"),
    N_("Edward Brocklesby"),
    N_("Chong Kai Xiong"),
    N_("Milosz Derezynski"),
    N_("David Lau"),
    N_("Ole Andre Vadla Ravnaas"),
    N_("Michiel Sikkes"),
    NULL,

    N_("With Additional Help:"),
    N_("Andrei Badea"),
    N_("Peter Behroozi"),
    N_("Bernard Blackham"),
    N_("Oliver Blin"),
    N_("Tomas Bzatek"),
    N_("Liviu Danicel"),
    N_("Jon Dowland"),
    N_("Artur Frysiak"),
    N_("Sebastian Kapfer"),
    N_("Lukas Koberstein"),
    N_("Dan Korostelev"),
    N_("Jolan Luff"),
    N_("Michael Marineau"),
    N_("Tim-Philipp Muller"),
    N_("Julien Portalier"),
    N_("Andrew Ruder"),
    N_("Olivier Samyn"),
    N_("Martijn Vernooij"),
    NULL,

    N_("ClearSky skin:"),
    N_("Will Reinhart"),
    NULL,

    N_("Homepage and Graphics:"),
    N_("Milosz Derezynski"),
    N_("Edward Brocklesby"),
    NULL,

    NULL
};

static const gchar *translators[] = {
    N_("Brazilian Portuguese:"),
    N_("Philipi Pinto"),
    NULL,
    N_("Breton:"),
    N_("Thierry Vignaud"),
    NULL,
    N_("Chinese:"),
    N_("Chong Kai Xiong"),
    N_("Chao-Hsiung Liao"),
    NULL,
    N_("Czech:"),
    N_("Jan Narovec"),
    NULL,
    N_("Dutch:"),
    N_("Laurens Buhler"),
    NULL,
    N_("Finnish:"),
    N_("Pauli Virtanen"),
    NULL,
    N_("French:"),
    N_("David Le Brun"),
    NULL,
    N_("German:"),
    N_("Matthias Debus"),
    NULL,
    N_("Georgian: "),
    N_("George Machitidze"),
    NULL,
    N_("Greek:"),
    N_("Kouzinopoulos Haris"),
    N_("Stavros Giannouris"),
    NULL,
    N_("Hindi:"),
    N_("Dhananjaya Sharma"),
    NULL,
    N_("Hungarian:"),
    N_("Laszlo Dvornik"),
    NULL,
    N_("Italian:"),
    N_("Alessio D'Ascanio"),
    NULL,
    N_("Japanese:"),
    N_("Takeshi Aihana"),
    NULL,
    N_("Korean:"),
    N_("DongCheon Park"),
    NULL,
    N_("Lithuanian:"),
    N_("Rimas Kudelis"),
    NULL,
    N_("Macedonian:"),
    N_("Arangel Angov"),
    NULL,
    N_("Polish:"),
    N_("Jacek Wolszczak"),
    NULL,
    N_("Romanian:"),
    N_("Liviu Danicel"),
    NULL,
    N_("Russian:"),
    N_("Pavlo Bohmat"),
    N_("Dan Korostelev"),
    N_("Vitaly Lipatov"),
    NULL,
    N_("Slovak:"),
    N_("Pavel Kanzelsberger"),
    NULL,
    N_("Spanish:"),
    N_("Francisco Javier F. Serrador"),
    NULL,
    N_("Swedish:"),
    N_("Martin Persenius"),
    NULL,
    N_("Ukrainian:"),
    N_("Mykola Lynnyk"),
    NULL,
    N_("Welsh:"),
    N_("Edward Brocklesby"),
    NULL,

    NULL
};


static GtkWidget *
generate_credit_list(const gchar * text[], gboolean sec_space)
{
    GtkWidget *scrollwin;
    GtkWidget *treeview;
    GtkListStore *list_store;
    GtkTreeIter iter;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    const gchar *const *item;

    list_store = gtk_list_store_new(N_COLS, G_TYPE_STRING, G_TYPE_STRING);

    item = text;

    while (*item) {
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter,
                           COL_LEFT, _(item[0]), COL_RIGHT, _(item[1]), -1);
        item += 2;

        while (*item) {
            gtk_list_store_append(list_store, &iter);
            gtk_list_store_set(list_store, &iter,
                               COL_LEFT, "", COL_RIGHT, _(*item++), -1);
        }

        ++item;

        if (*item && sec_space) {
            gtk_list_store_append(list_store, &iter);
            gtk_list_store_set(list_store, &iter,
                               COL_LEFT, "", COL_RIGHT, "", -1);
        }
    }

    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(treeview), FALSE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)),
        GTK_SELECTION_NONE);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", 1.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("Left", renderer,
                                                      "text", COL_LEFT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", 0.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("Right", renderer,
                                                      "text", COL_RIGHT,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    scrollwin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwin), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(scrollwin), treeview);
    gtk_container_set_border_width(GTK_CONTAINER(scrollwin), 10);

    gtk_widget_show_all(scrollwin);

    return scrollwin;
}

void
show_about_window(void)
{
    static GtkWidget *about_window = NULL;

    GdkPixmap *beep_logo_pmap = NULL, *beep_logo_mask = NULL;
    GtkWidget *about_vbox;
    GtkWidget *about_credits_logo_box, *about_credits_logo_frame;
    GtkWidget *about_credits_logo;
    GtkWidget *about_notebook;
    GtkWidget *list;
    GtkWidget *bbox, *close_btn;
    GtkWidget *label;
    gchar *text;

    if (about_window)
        return;

    about_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(about_window),
                             GDK_WINDOW_TYPE_HINT_DIALOG);

    gtk_window_set_default_size(GTK_WINDOW(about_window), -1, 480);
    gtk_window_set_title(GTK_WINDOW(about_window), _("About BMP"));
    gtk_window_set_position(GTK_WINDOW(about_window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(about_window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(about_window), 10);

    g_signal_connect(about_window, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &about_window);

    gtk_widget_realize(about_window);

    about_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(about_window), about_vbox);

    if (!beep_logo_pmap)
        beep_logo_pmap =
            gdk_pixmap_create_from_xpm_d(about_window->window,
                                         &beep_logo_mask, NULL, beep_logo);

    about_credits_logo_box = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(about_vbox), about_credits_logo_box,
                       FALSE, FALSE, 0);

    about_credits_logo_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(about_credits_logo_frame),
                              GTK_SHADOW_ETCHED_OUT);
    gtk_box_pack_start(GTK_BOX(about_credits_logo_box),
                       about_credits_logo_frame, FALSE, FALSE, 0);

    about_credits_logo = gtk_pixmap_new(beep_logo_pmap, beep_logo_mask);
    gtk_container_add(GTK_CONTAINER(about_credits_logo_frame),
                      about_credits_logo);

    label = gtk_label_new(NULL);
    text = g_strdup_printf(_(bmp_brief), VERSION);
    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    g_free(text);

    gtk_box_pack_start(GTK_BOX(about_vbox), label, FALSE, FALSE, 0);

    about_notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(about_vbox), about_notebook, TRUE, TRUE, 0);

    list = generate_credit_list(credit_text, TRUE);
    gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
                             gtk_label_new(_("Credits")));

    list = generate_credit_list(translators, FALSE);
    gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
                             gtk_label_new(_("Translators")));

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(about_vbox), bbox, FALSE, FALSE, 0);

    close_btn = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(close_btn, "clicked",
                             G_CALLBACK(gtk_widget_destroy), about_window);
    GTK_WIDGET_SET_FLAGS(close_btn, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), close_btn, TRUE, TRUE, 0);
    gtk_widget_grab_default(close_btn);

    gtk_widget_show_all(about_window);
}
