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

/* TODO: enforce default sizes! */

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "equalizer.h"
#include "main.h"
#include "playlistwin.h"
#include "skin.h"
#include "skinwin.h"
#include "util.h"

#include "debug.h"

#include <gdk/gdkx.h>
#include <X11/Xlib.h>


struct _SkinPixmapIdMapping {
    SkinPixmapId id;
    const gchar *name;
    const gchar *alt_name;
    gint width, height;
};

struct _SkinMaskInfo {
    gint width, height;
    gchar *inistr;
};

typedef struct _SkinPixmapIdMapping SkinPixmapIdMapping;
typedef struct _SkinMaskInfo SkinMaskInfo;


Skin *bmp_active_skin = NULL;

static gint skin_current_num;

static SkinMaskInfo skin_mask_info[] = {
    {275, 116, "Normal"},
    {275, 16,  "WindowShade"},
    {275, 116, "Equalizer"},
    {275, 16,  "EqualizerWS"}
};

static SkinPixmapIdMapping skin_pixmap_id_map[] = {
    {SKIN_MAIN, "main", NULL},
    {SKIN_CBUTTONS, "cbuttons", NULL},
    {SKIN_SHUFREP, "shufrep", NULL},
    {SKIN_TEXT, "text", NULL},
    {SKIN_TITLEBAR, "titlebar", NULL},
    {SKIN_VOLUME, "volume", NULL},
    {SKIN_BALANCE, "balance", "volume"},
    {SKIN_MONOSTEREO, "monoster", NULL},
    {SKIN_PLAYPAUSE, "playpaus", NULL},
    {SKIN_NUMBERS, "nums_ex", "numbers"},
    {SKIN_POSBAR, "posbar", NULL},
    {SKIN_EQMAIN, "eqmain", NULL},
    {SKIN_PLEDIT, "pledit", NULL},
    {SKIN_EQ_EX, "eq_ex", NULL}
};

static guint skin_pixmap_id_map_size = G_N_ELEMENTS(skin_pixmap_id_map);

static const guchar skin_default_viscolor[24][3] = {
    {9, 34, 53},
    {10, 18, 26},
    {0, 54, 108},
    {0, 58, 116},
    {0, 62, 124},
    {0, 66, 132},
    {0, 70, 140},
    {0, 74, 148},
    {0, 78, 156},
    {0, 82, 164},
    {0, 86, 172},
    {0, 92, 184},
    {0, 98, 196},
    {0, 104, 208},
    {0, 110, 220},
    {0, 116, 232},
    {0, 122, 244},
    {0, 128, 255},
    {0, 128, 255},
    {0, 104, 208},
    {0, 80, 160},
    {0, 56, 112},
    {0, 32, 64},
    {200, 200, 200}
};

static GdkBitmap *
skin_create_transparent_mask(const gchar *,
                             const gchar *,
                             const gchar *,
                             GdkWindow *,
                             gint, gint);

static void
skin_setup_masks(Skin * skin);

static void
skin_set_default_vis_color(Skin * skin);


void
skin_lock(Skin * skin)
{
    g_mutex_lock(skin->lock);
}

void
skin_unlock(Skin * skin)
{
    g_mutex_unlock(skin->lock);
}

gboolean
bmp_active_skin_reload(void) 
{
    return bmp_active_skin_load(bmp_active_skin->path);	
}

gboolean
bmp_active_skin_load(const gchar * path)
{
    g_return_val_if_fail(bmp_active_skin != NULL, FALSE);

    if (!skin_load(bmp_active_skin, path))
        return FALSE;

    skin_setup_masks(bmp_active_skin);
    draw_main_window(TRUE);
    draw_playlist_window(TRUE);
    draw_equalizer_window(TRUE);

    vis_set_window(mainwin_vis, mainwin->window);
    playlistwin_update_list();

    return TRUE;
}

void
skin_pixmap_free(SkinPixmap * p)
{
    g_return_if_fail(p != NULL);
    g_return_if_fail(p->pixmap != NULL);

    g_object_unref(p->pixmap);
    p->pixmap = NULL;
}

Skin *
skin_new(void)
{
    Skin *skin;
    skin = g_new0(Skin, 1);
    skin->lock = g_mutex_new();
    return skin;
}

void
skin_free(Skin * skin)
{
    gint i;

    g_return_if_fail(skin != NULL);

    skin_lock(skin);

    for (i = 0; i < SKIN_PIXMAP_COUNT; i++)
        skin_pixmap_free(&skin->pixmaps[i]);

    for (i = 0; i < SKIN_PIXMAP_COUNT; i++) {
        if (skin->masks[i])
            g_object_unref(skin->masks[i]);

        skin->masks[i] = NULL;
    }

    skin_set_default_vis_color(skin);
    skin_unlock(skin);
}

void
skin_destroy(Skin * skin)
{
    g_return_if_fail(skin != NULL);
    skin_free(skin);
    g_mutex_free(skin->lock);
    g_free(skin);
}

const SkinPixmapIdMapping *
skin_pixmap_id_lookup(gint id)
{
    gint i;

    for (i = 0; i < skin_pixmap_id_map_size; i++) {
        if (id == skin_pixmap_id_map[i].id) {
            return &skin_pixmap_id_map[i];
        }
    }

    return NULL;
}

const gchar *
skin_pixmap_id_to_name(SkinPixmapId id)
{
    gint i;

    for (i = 0; i < skin_pixmap_id_map_size; i++) {
        if (id == skin_pixmap_id_map[i].id)
            return skin_pixmap_id_map[i].name;
    }
    return NULL;
}

static void
skin_set_default_vis_color(Skin * skin)
{
    memcpy(skin->vis_color, skin_default_viscolor,
           sizeof(skin_default_viscolor));
}

gchar *
skin_pixmap_locate(const gchar * dirname, const gchar * basename,
                   const gchar * alt_basename)
{
    gchar *filename;

    if (!(filename = find_file_recursively(dirname, basename))) {
        g_warning(G_STRLOC ": couldn't locate %s in directory %s",
                  basename, dirname);
        g_free(filename);

        if (alt_basename) {
            g_message(G_STRLOC ": trying %s instead", alt_basename);
            return skin_pixmap_locate(dirname, alt_basename, NULL);
        }
    }

    return filename;
}

/* FIXME: this function is temporary. It will be removed when the skinning system
   uses GdkPixbuf in place of GdkPixmap */

static GdkPixmap *
pixmap_new_from_file(const gchar * filename)
{
    GdkPixbuf *pixbuf;
    GdkPixmap *pixmap;
    gint width, height;

    if (!(pixbuf = gdk_pixbuf_new_from_file(filename, NULL)))
        return NULL;

    width = gdk_pixbuf_get_width(pixbuf);
    height = gdk_pixbuf_get_height(pixbuf);

    if (!(pixmap = gdk_pixmap_new(mainwin->window, width, height,
                                  gdk_rgb_get_visual()->depth))) {
        g_object_unref(pixbuf);
        return NULL;
    }

    gdk_pixbuf_render_to_drawable(pixbuf, pixmap, mainwin_gc, 0, 0, 0, 0,
                                  width, height, GDK_RGB_DITHER_MAX, 0, 0);
    g_object_unref(pixbuf);

    return pixmap;
}

static gboolean
skin_load_pixmap_id(Skin * skin, SkinPixmapId id, const gchar * path_p)
{
    const gchar *path;
    gchar *basename = NULL, *alt_basename = NULL;
    gchar *filename;
    gint width, height;
    const SkinPixmapIdMapping *pixmap_id_mapping;
    GdkPixmap *gpm;
    SkinPixmap *pm = NULL;

    g_return_val_if_fail(skin != NULL, FALSE);
    g_return_val_if_fail(id < SKIN_PIXMAP_COUNT, FALSE);

    pixmap_id_mapping = skin_pixmap_id_lookup(id);
    g_return_val_if_fail(pixmap_id_mapping != NULL, FALSE);

    basename = g_strdup_printf("%s.bmp", pixmap_id_mapping->name);
    if (pixmap_id_mapping->alt_name)
        alt_basename = g_strdup_printf("%s.bmp", pixmap_id_mapping->alt_name);

    path = path_p ? path_p : skin->path;
    filename = skin_pixmap_locate(path, basename, alt_basename);
    g_free(basename);
    g_free(alt_basename);

    if (!(gpm = pixmap_new_from_file(filename))) {
        g_warning("loading of %s failed", filename);
        g_free(filename);
        return FALSE;
    }

    g_free(filename);

    gdk_window_get_size(gpm, &width, &height);
    pm = &skin->pixmaps[id];
    pm->pixmap = gpm;
    pm->width = width;
    pm->height = height;
    pm->current_width = width;
    pm->current_height = height;

    return TRUE;
}

void
skin_mask_create(Skin * skin,
                 const gchar * path,
                 gint id,
                 GdkWindow * window)
{
    skin->masks[id] =
        skin_create_transparent_mask(path, "region.txt",
                                     skin_mask_info[id].inistr, window,
                                     skin_mask_info[id].width,
                                     skin_mask_info[id].height);
}

static void
skin_setup_masks(Skin * skin)
{
    GdkBitmap *mask;

    if (cfg.show_wm_decorations)
        return;

    if (cfg.player_visible) {
        mask = skin_get_mask(skin, SKIN_MASK_MAIN + cfg.player_shaded);
        gtk_widget_shape_combine_mask(mainwin, mask, 0, 0);
    }

    mask = skin_get_mask(skin, SKIN_MASK_EQ + cfg.equalizer_shaded);
    gtk_widget_shape_combine_mask(equalizerwin, mask, 0, 0);
}

static GdkBitmap *
create_default_mask(GdkWindow * parent, gint w, gint h)
{
    GdkBitmap *ret;
    GdkGC *gc;
    GdkColor pattern;

    ret = gdk_pixmap_new(parent, w, h, 1);
    gc = gdk_gc_new(ret);
    pattern.pixel = 1;
    gdk_gc_set_foreground(gc, &pattern);
    gdk_draw_rectangle(ret, gc, TRUE, 0, 0, w, h);
    gdk_gc_destroy(gc);

    return ret;
}

static void
skin_query_color(GdkColormap * cm, GdkColor * c)
{
    XColor xc = { 0 };

    xc.pixel = c->pixel;
    XQueryColor(GDK_COLORMAP_XDISPLAY(cm), GDK_COLORMAP_XCOLORMAP(cm), &xc);
    c->red = xc.red;
    c->green = xc.green;
    c->blue = xc.blue;
}

static glong
skin_calc_luminance(GdkColor * c)
{
    return (0.212671 * c->red + 0.715160 * c->green + 0.072169 * c->blue);
}

static void
skin_get_textcolors(GdkPixmap * text, GdkColor * bgc, GdkColor * fgc)
{
    /*
     * Try to extract reasonable background and foreground colors
     * from the font pixmap
     */

    GdkImage *gi;
    GdkColormap *cm;
    gint i;

    g_return_if_fail(text != NULL);

    /* Get the first line of text */
    gi = gdk_drawable_get_image(text, 0, 0, 152, 6);
    cm = gdk_window_get_colormap(playlistwin->window);
    g_return_if_fail(GDK_IS_WINDOW(playlistwin->window));

    for (i = 0; i < 6; i++) {
        GdkColor c;
        gint x;
        glong d, max_d;

        /* Get a pixel from the middle of the space character */
        bgc[i].pixel = gdk_image_get_pixel(gi, 151, i);
        skin_query_color(cm, &bgc[i]);

        max_d = 0;
        for (x = 1; x < 150; x++) {
            c.pixel = gdk_image_get_pixel(gi, x, i);
            skin_query_color(cm, &c);

            d = labs(skin_calc_luminance(&c) - skin_calc_luminance(&bgc[i]));
            if (d > max_d) {
                memcpy(&fgc[i], &c, sizeof(GdkColor));
                max_d = d;
            }
        }
    }
    gdk_image_destroy(gi);
}

gboolean
init_skins(const gchar * path)
{
    bmp_active_skin = skin_new();

    if (!bmp_active_skin_load(path)) {
        /* FIXME: Oddly, g_message() causes a crash if path is NULL on
         * Solaris (see bug #165) */
        if (path) 
            g_message("Unable to load skin (%s), trying default...", path);

        /* can't load configured skin, retry with default */
        if (!bmp_active_skin_load(BMP_DEFAULT_SKIN_PATH)) {
            g_message("Unable to load default skin (%s)! Giving up.",
                      BMP_DEFAULT_SKIN_PATH);
            return FALSE;
        }
    }

    if (cfg.random_skin_on_play)
        skinlist_update();

    return TRUE;
}

static guint
hex_chars_to_int(gchar hi, gchar lo)
{
    /*
     * Converts a value in the range 0x00-0xFF
     * to a integer in the range 0-65535
     */
    gchar str[3];

    str[0] = hi;
    str[1] = lo;
    str[2] = 0;

    return (CLAMP(strtol(str, NULL, 16), 0, 0xFF) << 8);
}

GdkColor *
skin_load_color(const gchar * path, const gchar * file,
                const gchar * section, const gchar * key)
{
    gchar *filename, *value;
    GdkColor *color = NULL;

    filename = find_file_recursively(path, file);
    if (filename) {
        value = read_ini_string(filename, section, key);
        if (value) {
            gchar *ptr = value;
            gint len;

            color = g_new0(GdkColor, 1);
            g_strstrip(value);

            if (value[0] == '#')
                ptr++;
            len = strlen(ptr);

            /*
             * The handling of incomplete values is done this way
             * to maximize winamp compatibility
             */
            if (len >= 6) {
                color->red = hex_chars_to_int(*ptr, *(ptr + 1));
                ptr += 2;
            }
            if (len >= 4) {
                color->green = hex_chars_to_int(*ptr, *(ptr + 1));
                ptr += 2;
            }
            if (len >= 2)
                color->blue = hex_chars_to_int(*ptr, *(ptr + 1));


            gdk_color_alloc(gdk_window_get_colormap(playlistwin->window),
                            color);
            g_free(value);
        }
        g_free(filename);
    }
    return color;
}



GdkBitmap *
skin_create_transparent_mask(const gchar * path,
                             const gchar * file,
                             const gchar * section,
                             GdkWindow * window,
                             gint width,
                             gint height)
{
    GdkBitmap *mask = NULL;
    GdkGC *gc = NULL;
    GdkColor pattern;
    GdkPoint *gpoints;

    gchar *filename = NULL;
    gboolean created_mask = FALSE;
    GArray *num, *point;
    gint i, j, k;

    if (path)
        filename = find_file_recursively(path, file);

    /* filename will be null if path wasn't set */
    if (!filename) {
        return create_default_mask(window, width, height);
    }

    if ((num = read_ini_array(filename, section, "NumPoints")) == NULL) {
        g_free(filename);
        return NULL;
    }

    if ((point = read_ini_array(filename, section, "PointList")) == NULL) {
        g_array_free(num, TRUE);
        g_free(filename);
        return NULL;
    }

    mask = gdk_pixmap_new(window, width, height, 1);
    gc = gdk_gc_new(mask);

    pattern.pixel = 0;
    gdk_gc_set_foreground(gc, &pattern);
    gdk_draw_rectangle(mask, gc, TRUE, 0, 0, width, height);
    pattern.pixel = 1;
    gdk_gc_set_foreground(gc, &pattern);

    j = 0;
    for (i = 0; i < num->len; i++) {
        if ((point->len - j) >= (g_array_index(num, gint, i) * 2)) {
            created_mask = TRUE;
            gpoints = g_new(GdkPoint, g_array_index(num, gint, i));
            for (k = 0; k < g_array_index(num, gint, i); k++) {
                gpoints[k].x = g_array_index(point, gint, j + k * 2);
                gpoints[k].y = g_array_index(point, gint, j + k * 2 + 1);
            }
            j += k * 2;
            gdk_draw_polygon(mask, gc, TRUE, gpoints,
                             g_array_index(num, gint, i));
            g_free(gpoints);
        }
    }
    g_array_free(num, TRUE);
    g_array_free(point, TRUE);
    g_free(filename);

    if (!created_mask)
        gdk_draw_rectangle(mask, gc, TRUE, 0, 0, width, height);

    gdk_gc_destroy(gc);

    return mask;
}

void
skin_load_viscolor(Skin * skin, const gchar * path, const gchar * basename)
{
    FILE *file;
    gint i, c;
    gchar line[256], *filename;
    GArray *a;

    g_return_if_fail(skin != NULL);
    g_return_if_fail(path != NULL);
    g_return_if_fail(basename != NULL);

    skin_set_default_vis_color(skin);

    filename = find_file_recursively(path, basename);
    if (!filename)
        return;

    if (!(file = fopen(filename, "r"))) {
        g_free(filename);
        return;
    }

    g_free(filename);

    for (i = 0; i < 24; i++) {
        if (fgets(line, 255, file)) {
            a = string_to_garray(line);
            if (a->len > 2) {
                for (c = 0; c < 3; c++)
                    skin->vis_color[i][c] = g_array_index(a, gint, c);
            }
            g_array_free(a, TRUE);
        }
        else
            break;
    }

    fclose(file);
}

#if 0
static void
skin_numbers_generate_dash(Skin * skin)
{
    GdkGC *gc;
    GdkPixmap *pixmap;
    SkinPixmap *numbers;

    g_return_if_fail(skin != NULL);

    numbers = &skin->pixmaps[SKIN_NUMBERS];
    if (!numbers->pixmap || numbers->current_width < 99)
        return;

    gc = gdk_gc_new(numbers->pixmap);
    pixmap = gdk_pixmap_new(mainwin->window, 108,
                            numbers->current_height,
                            -1);

    skin_draw_pixmap(skin, pixmap, gc, SKIN_NUMBERS, 0, 0, 0, 0, 99, 13);
    skin_draw_pixmap(skin, pixmap, gc, SKIN_NUMBERS, 90, 0, 99, 0, 9, 13);
    skin_draw_pixmap(skin, pixmap, gc, SKIN_NUMBERS, 20, 6, 101, 6, 5, 1);

    g_object_unref(numbers->pixmap);
    g_object_unref(gc);

    numbers->pixmap = pixmap;
    numbers->current_width = 108;
}
#endif

static void
skin_load_cursor(Skin * skin, const gchar * dirname)
{
    const gchar * basename = "normal.cur";
    gchar * filename = NULL;
    GdkPixbuf * cursor_pixbuf = NULL;
    GdkPixbufAnimation * cursor_animated = NULL;
    GdkCursor * cursor_gdk = NULL;
    GError * error = NULL;
 
    filename = find_file_recursively(dirname, basename);

    if (filename && cfg.custom_cursors)	{
    	cursor_animated = gdk_pixbuf_animation_new_from_file(filename, &error);
        cursor_pixbuf = gdk_pixbuf_animation_get_static_image(cursor_animated);
	cursor_gdk = gdk_cursor_new_from_pixbuf(gdk_display_get_default(),
                                                cursor_pixbuf, 0, 0);
    } else {
	cursor_gdk = gdk_cursor_new(GDK_LEFT_PTR);
    }

    gdk_window_set_cursor(mainwin->window, cursor_gdk);
    gdk_window_set_cursor(playlistwin->window, cursor_gdk);
    gdk_window_set_cursor(equalizerwin->window, cursor_gdk);
}

static void
skin_load_pixmaps(Skin * skin, const gchar * path)
{
    GdkPixmap *text_pm;
    guint i;

    for (i = 0; i < SKIN_PIXMAP_COUNT; i++)
        skin_load_pixmap_id(skin, i, path);

    text_pm = skin->pixmaps[SKIN_TEXT].pixmap;

    if (text_pm)
        skin_get_textcolors(text_pm, skin->textbg, skin->textfg);

#if 0
    if (skin->pixmaps[SKIN_NUMBERS].pixmap)
        skin_numbers_generate_dash(skin);
#endif

    skin->colors[SKIN_PLEDIT_NORMAL] =
        skin_load_color(path, "pledit.txt", "text", "normal");
    skin->colors[SKIN_PLEDIT_CURRENT] =
        skin_load_color(path, "pledit.txt", "text", "current");
    skin->colors[SKIN_PLEDIT_NORMALBG] =
        skin_load_color(path, "pledit.txt", "text", "normalbg");
    skin->colors[SKIN_PLEDIT_SELECTEDBG] =
        skin_load_color(path, "pledit.txt", "text", "selectedbg");

    skin_mask_create(skin, path, SKIN_MASK_MAIN, mainwin->window);
    skin_mask_create(skin, path, SKIN_MASK_MAIN_SHADE, mainwin->window);

    skin_mask_create(skin, path, SKIN_MASK_EQ, equalizerwin->window);
    skin_mask_create(skin, path, SKIN_MASK_EQ_SHADE, equalizerwin->window);

    skin_load_viscolor(skin, path, "viscolor.txt");
}

static gboolean
skin_load_nolock(Skin * skin, const gchar * path, gboolean force)
{
    gchar *cpath;

    g_return_val_if_fail(skin != NULL, FALSE);
    g_return_val_if_fail(path != NULL, FALSE);
    REQUIRE_LOCK(skin->lock);

    if (!g_file_test(path, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_IS_DIR))
	return FALSE;
   
    if (!force) {
        if (skin->path)
            if (!strcmp(skin->path, path))
                return FALSE;
    }
      
    skin_current_num++;

    skin->path = g_strdup(path);

    
    if (!file_is_archive(path)) {
        skin_load_pixmaps(skin, path);
        skin_load_cursor(skin, path);
        return TRUE;
    }

    if (!(cpath = archive_decompress(path))) {
        g_message("Unable to extract skin archive (%s)", path);
        return FALSE;
    }

    skin_load_pixmaps(skin, cpath);
    skin_load_cursor(skin, cpath);
    del_directory(cpath);
    g_free(cpath);

    return TRUE;
}

void
skin_install_skin(const gchar * path)
{
    gchar *command;

    g_return_if_fail(path != NULL);

    command = g_strdup_printf("cp %s %s", path, bmp_paths[BMP_PATH_USER_SKIN_DIR]);
    if (system(command)) {
        g_message("Unable to install skin (%s) into user directory (%s)",
                  path, bmp_paths[BMP_PATH_USER_SKIN_DIR]);
    }
    g_free(command);
}


gboolean
skin_load(Skin * skin, const gchar * path)
{
    gboolean error;

    g_return_val_if_fail(skin != NULL, FALSE);
    g_return_val_if_fail(path != NULL, FALSE);

    skin_lock(skin);
    error = skin_load_nolock(skin, path, FALSE);
    skin_unlock(skin);
    
    return error;
}

gboolean
skin_reload_forced(void) 
{
   gboolean error;

   skin_lock(bmp_active_skin);
   error = skin_load_nolock(bmp_active_skin, bmp_active_skin->path, TRUE);
   skin_unlock(bmp_active_skin);

   return error;
}

void
skin_reload(Skin * skin)
{
    g_return_if_fail(skin != NULL);
    skin_load_nolock(skin, skin->path, TRUE);
}


static SkinPixmap *
skin_get_pixmap(Skin * skin, SkinPixmapId map_id)
{
    g_return_val_if_fail(skin != NULL, NULL);
    g_return_val_if_fail(map_id < SKIN_PIXMAP_COUNT, NULL);

    return &skin->pixmaps[map_id];
}

GdkBitmap *
skin_get_mask(Skin * skin, SkinMaskId mi)
{
    g_return_val_if_fail(skin != NULL, NULL);
    g_return_val_if_fail(mi < SKIN_PIXMAP_COUNT, NULL);

    return skin->masks[mi];
}

GdkColor *
skin_get_color(Skin * skin, SkinColorId color_id)
{
    GdkColor *ret = NULL;

    g_return_val_if_fail(skin != NULL, NULL);

    switch (color_id) {
    case SKIN_TEXTBG:
        if (skin->pixmaps[SKIN_TEXT].pixmap)
            ret = skin->textbg;
        else
            ret = skin->def_textbg;
        break;
    case SKIN_TEXTFG:
        if (skin->pixmaps[SKIN_TEXT].pixmap)
            ret = skin->textfg;
        else
            ret = skin->def_textfg;
        break;
    default:
        if (color_id < SKIN_COLOR_COUNT)
            ret = skin->colors[color_id];
        break;
    }
    return ret;
}

void
skin_get_viscolor(Skin * skin, guchar vis_color[24][3])
{
    gint i;

    g_return_if_fail(skin != NULL);

    for (i = 0; i < 24; i++) {
        vis_color[i][0] = skin->vis_color[i][0];
        vis_color[i][1] = skin->vis_color[i][1];
        vis_color[i][2] = skin->vis_color[i][2];
    }
}

gint
skin_get_id(void)
{
    return skin_current_num;
}

void
skin_draw_pixmap(Skin * skin, GdkDrawable * drawable, GdkGC * gc,
                 SkinPixmapId pixmap_id,
                 gint xsrc, gint ysrc, gint xdest, gint ydest,
                 gint width, gint height)
{
    SkinPixmap *pixmap;

    g_return_if_fail(skin != NULL);

    pixmap = skin_get_pixmap(skin, pixmap_id);
    g_return_if_fail(pixmap != NULL);
    g_return_if_fail(pixmap->pixmap != NULL);

    if (xsrc > pixmap->width || ysrc > pixmap->height)
        return;

    width = MIN(width, pixmap->width - xsrc);
    height = MIN(height, pixmap->height - ysrc);
    gdk_draw_pixmap(drawable, gc, pixmap->pixmap, xsrc, ysrc,
                    xdest, ydest, width, height);
}

void
skin_get_eq_spline_colors(Skin * skin, guint32 colors[19])
{
    gint i;
    GdkPixmap *pixmap;
    GdkImage *img;
    SkinPixmap *eqmainpm;

    g_return_if_fail(skin != NULL);

    eqmainpm = &skin->pixmaps[SKIN_EQMAIN];
    if (eqmainpm->pixmap &&
        eqmainpm->current_width >= 116 && eqmainpm->current_height >= 313)
        pixmap = eqmainpm->pixmap;
    else
        return;

    if (!GDK_IS_DRAWABLE(pixmap))
        return;

    if (!(img = gdk_drawable_get_image(pixmap, 115, 294, 1, 19)))
        return;

    for (i = 0; i < 19; i++)
        colors[i] = gdk_image_get_pixel(img, 0, i);

    gdk_image_destroy(img);
}


static void
skin_draw_playlistwin_frame_top(Skin * skin,
                                GdkDrawable * drawable,
                                GdkGC * gc,
                                gint width, gint height, gboolean focus)
{
    /* The title bar skin consists of 2 sets of 4 images, 1 set
     * for focused state and the other for unfocused. The 4 images
     * are: 
     *
     * a. right corner (25,20)
     * b. left corner  (25,20)
     * c. tiler        (25,20)
     * d. title        (100,20)
     * 
     * min allowed width = 100+25+25 = 150
     */

    gint i, y, c;

    /* get y offset of the pixmap set to use */
    if (focus)
        y = 0;
    else
        y = 21;

    /* left corner */
    skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 0, y, 0, 0, 25, 20);

    /* titlebar title */
    skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 26, y,
                     (width - 100) / 2, 0, 100, 20);

    /* titlebar right corner  */
    skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 153, y,
                     width - 25, 0, 25, 20);

    /* tile draw the remaining frame */

    /* compute tile count */
    c = (width - (100 + 25 + 25)) / 25;

    for (i = 0; i < c / 2; i++) {
        /* left of title */
        skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 127, y,
                         25 + i * 25, 0, 25, 20);

        /* right of title */
        skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 127, y,
                         (width + 100) / 2 + i * 25, 0, 25, 20);
    }

    if (c & 1) {
        /* Odd tile count, so one remaining to draw. Here we split
         * it into two and draw half on either side of the title */
        skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 127, y,
                         ((c / 2) * 25) + 25, 0, 12, 20);
        skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 127, y,
                         (width / 2) + ((c / 2) * 25) + 50, 0, 13, 20);
    }
}

static void
skin_draw_playlistwin_frame_bottom(Skin * skin,
                                   GdkDrawable * drawable,
                                   GdkGC * gc,
                                   gint width, gint height, gboolean focus)
{
    /* The bottom frame skin consists of 1 set of 4 images. The 4
     * images are:
     *
     * a. left corner with menu buttons (125,38)
     * b. visualization window (75,38)
     * c. right corner with play buttons (150,38)
     * d. frame tile (25,38)
     * 
     * (min allowed width = 125+150+25=300
     */

    gint i, c;

    /* bottom left corner (menu buttons) */
    skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 0, 72,
                     0, height - 38, 125, 38);

    c = (width - 275) / 25;

    /* draw visualization window, if width allows */
    if (c >= 3) {
        c -= 3;
        skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 205, 0,
                         width - (150 + 75), height - 38, 75, 38);
    }

    /* Bottom right corner (playbuttons etc) */
    skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT,
                     126, 72, width - 150, height - 38, 150, 38);

    /* Tile draw the remaining undrawn portions */
    for (i = 0; i < c; i++)
        skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 179, 0,
                         125 + i * 25, height - 38, 25, 38);
}

static void
skin_draw_playlistwin_frame_sides(Skin * skin,
                                  GdkDrawable * drawable,
                                  GdkGC * gc,
                                  gint width, gint height, gboolean focus)
{
    /* The side frames consist of 2 tile images. 1 for the left, 1 for
     * the right. 
     * a. left  (12,29)
     * b. right (19,29)
     */

    gint i;

    /* frame sides */
    for (i = 0; i < (height - (20 + 38)) / 29; i++) {
        /* left */
        skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 0, 42,
                         0, 20 + i * 29, 12, 29);

        /* right */
        skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 32, 42,
                         width - 19, 20 + i * 29, 19, 29);
    }
}


void
skin_draw_playlistwin_frame(Skin * skin,
                            GdkDrawable * drawable, GdkGC * gc,
                            gint width, gint height, gboolean focus)
{
    skin_draw_playlistwin_frame_top(skin, drawable, gc, width, height, focus);
    skin_draw_playlistwin_frame_bottom(skin, drawable, gc, width, height,
                                       focus);
    skin_draw_playlistwin_frame_sides(skin, drawable, gc, width, height,
                                      focus);
}


void
skin_draw_playlistwin_shaded(Skin * skin,
                             GdkDrawable * drawable, GdkGC * gc,
                             gint width, gboolean focus)
{
    /* The shade mode titlebar skin consists of 4 images:
     * a) left corner               offset (72,42) size (25,14)
     * b) right corner, focused     offset (99,57) size (50,14)
     * c) right corner, unfocused   offset (99,42) size (50,14)
     * d) bar tile                  offset (72,57) size (25,14)
     */

    gint i;

    /* left corner */
    skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 72, 42, 0, 0, 25, 14);

    /* bar tile */
    for (i = 0; i < (width - 75) / 25; i++)
        skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 72, 57,
                         (i * 25) + 25, 0, 25, 14);

    /* right corner */
    skin_draw_pixmap(skin, drawable, gc, SKIN_PLEDIT, 99, focus ? 57 : 42,
                     width - 50, 0, 50, 14);
}


void
skin_draw_mainwin_titlebar(Skin * skin,
                           GdkDrawable * drawable, GdkGC * gc,
                           gboolean shaded, gboolean focus)
{
    /* The titlebar skin consists of 2 sets of 2 images, one for for
     * shaded and the other for unshaded mode, giving a total of 4.
     * The images are exactly 275x14 pixels, aligned and arranged
     * vertically on each other in the pixmap in the following order:
     * 
     * a) unshaded, focused      offset (27, 0)
     * b) unshaded, unfocused    offset (27, 15)
     * c) shaded, focused        offset (27, 29)
     * d) shaded, unfocused      offset (27, 42)
     */

    gint y_offset;

    if (shaded) {
        if (focus)
            y_offset = 29;
        else
            y_offset = 42;
    }
    else {
        if (focus)
            y_offset = 0;
        else
            y_offset = 15;
    }

    skin_draw_pixmap(skin, drawable, gc, SKIN_TITLEBAR, 27, y_offset,
                     0, 0, MAINWIN_WIDTH, MAINWIN_TITLEBAR_HEIGHT);
}

#if 0
void
skin_draw_mainwin(Skin * skin,
                  GdkDrawable * drawable, GdkGC gc,
                  gboolean doublesize, gboolean shaded, gboolean focus)
{

}
#endif
