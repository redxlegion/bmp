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

#ifndef EQ_GRAPH_H
#define EQ_GRAPH_H

#include <glib.h>
#include <gdk/gdk.h>

#include "widget.h"

#define EQ_GRAPH(x)  ((EqGraph *)(x))
struct _EqGraph {
    Widget eg_widget;
};

typedef struct _EqGraph EqGraph;

EqGraph *create_eqgraph(GList ** wlist, GdkPixmap * parent, GdkGC * gc,
                        gint x, gint y);

#endif
