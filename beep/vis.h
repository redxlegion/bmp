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
#ifndef VIS_H
#define VIS_H

#include <glib.h>
#include <gdk/gdk.h>

#include "widget.h"

typedef enum {
    VIS_ANALYZER, VIS_SCOPE, VIS_OFF
} VisType;

typedef enum {
    ANALYZER_NORMAL, ANALYZER_FIRE, ANALYZER_VLINES
} AnalyzerMode;

typedef enum {
    ANALYZER_LINES, ANALYZER_BARS
} AnalyzerType;

typedef enum {
    SCOPE_DOT, SCOPE_LINE, SCOPE_SOLID
} ScopeMode;

typedef enum {
    VU_NORMAL, VU_SMOOTH
} VUMode;

typedef enum {
    REFRESH_FULL, REFRESH_HALF, REFRESH_QUARTER, REFRESH_EIGTH
} RefreshRate;

typedef enum {
    FALLOFF_SLOWEST, FALLOFF_SLOW, FALLOFF_MEDIUM, FALLOFF_FAST,
    FALLOFF_FASTEST
} FalloffSpeed;

#define VIS(x)  ((Vis *)(x))
struct _Vis {
    Widget vs_widget;
    GdkWindow *vs_window;
    gfloat vs_data[75], vs_peak[75], vs_peak_speed[75];
    gint vs_refresh_delay;
};

typedef struct _Vis Vis;

void vis_draw(Widget * w);

Vis *create_vis(GList ** wlist, GdkPixmap * parent, GdkWindow * window,
                GdkGC * gc, gint x, gint y, gint width);
void vis_timeout_func(Vis * vis, guchar * data);
void vis_clear_data(Vis * vis);
void vis_clear(Vis * vis);
void vis_set_window(Vis * vis, GdkWindow * window);

#endif
