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

#include <glib.h>
#include <string.h>
#include "plugin.h"
#include "general.h"

GeneralPluginData gp_data = {
    NULL,
    NULL
};

GList *
get_general_list(void)
{
    return gp_data.general_list;
}

GList *
get_general_enabled_list(void)
{
    return gp_data.enabled_list;
}

static GeneralPlugin *
get_general_plugin(gint i)
{
    GList *node = g_list_nth(get_general_list(), i);

    if (!node)
        return NULL;

    return GENERAL_PLUGIN(node->data);
}


void
general_about(gint i)
{
    GeneralPlugin *plugin = get_general_plugin(i);

    if (!plugin || !plugin->about)
        return;

    plugin->about();
}

void
general_configure(gint i)
{
    GeneralPlugin *plugin = get_general_plugin(i);

    if (!plugin || !plugin->configure)
        return;

    plugin->configure();
}

static gboolean
general_plugin_is_enabled(GeneralPlugin * plugin)
{
    return (g_list_find(get_general_enabled_list(), plugin) != NULL);
}

void
enable_general_plugin(gint i, gboolean enable)
{
    GeneralPlugin *plugin = get_general_plugin(i);

    if (!plugin)
        return;

    if (enable && !general_plugin_is_enabled(plugin)) {
        gp_data.enabled_list = g_list_append(gp_data.enabled_list, plugin);
        if (plugin->init)
            plugin->init();
    }
    else if (!enable && general_plugin_is_enabled(plugin)) {
        gp_data.enabled_list = g_list_remove(gp_data.enabled_list, plugin);
        if (plugin->cleanup)
            plugin->cleanup();
    }
}

gboolean
general_enabled(gint i)
{
    return (g_list_find(gp_data.enabled_list,
                        get_general_plugin(i)) != NULL);
}

gchar *
general_stringify_enabled_list(void)
{
    GString *enable_str;
    gchar *name;
    GList *node = get_general_enabled_list();

    if (!node)
        return NULL;

    name = g_path_get_basename(GENERAL_PLUGIN(node->data)->filename);
    enable_str = g_string_new(name);
    g_free(name);

    for (node = g_list_next(node); node; node = g_list_next(node)) {
        name = g_path_get_basename(GENERAL_PLUGIN(node->data)->filename);
        g_string_append_c(enable_str, ',');
        g_string_append(enable_str, name);
        g_free(name);
    }

    return g_string_free(enable_str, FALSE);
}

void
general_enable_from_stringified_list(const gchar * list_str)
{
    gchar **list, **str;
    GeneralPlugin *plugin;

    if (!list_str || !strcmp(list_str, ""))
        return;

    list = g_strsplit(list_str, ",", 0);

    for (str = list; *str; str++) {
        GList *node;

        for (node = get_general_list(); node; node = g_list_next(node)) {
            gchar *base;

            base = g_path_get_basename(GENERAL_PLUGIN(node->data)->filename);

            if (!strcmp(*str, base)) {
                plugin = GENERAL_PLUGIN(node->data);
                gp_data.enabled_list = g_list_append(gp_data.enabled_list,
                                                      plugin);
                if (plugin->init)
                    plugin->init();
            }

            g_free(base);
        }
    }

    g_strfreev(list);
}
