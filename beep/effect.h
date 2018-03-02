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

#ifndef EFFECT_H
#define EFFECT_H

#include <glib.h>

typedef struct _EffectPluginData EffectPluginData;

struct _EffectPluginData {
    GList *effect_list;
    GList *enabled_list;
    /* FIXME: Needed? */
    gboolean playing;
    gboolean paused;
};

GList *get_effect_list(void);
void effect_about(gint i);
void effect_configure(gint i);
GList *get_effect_enabled_list(void);
void enable_effect_plugin(gint i, gboolean enable);
gboolean effect_enabled(gint i);
gchar *effect_stringify_enabled_list(void);
void effect_enable_from_stringified_list(const gchar * list);

extern EffectPluginData ep_data;

#endif
