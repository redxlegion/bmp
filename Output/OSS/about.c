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

#include "OSS.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libbeep/util.h>


void
oss_about(void)
{
    static GtkWidget *dialog;

    if (dialog != NULL)
        return;

    dialog = xmms_show_message(_("About OSS Driver"),
                               _("XMMS OSS Driver\n\n "
                                 "This program is free software; you can redistribute it and/or modify\n"
                                 "it under the terms of the GNU General Public License as published by\n"
                                 "the Free Software Foundation; either version 2 of the License, or\n"
                                 "(at your option) any later version.\n"
                                 "\n"
                                 "This program is distributed in the hope that it will be useful,\n"
                                 "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
                                 "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
                                 "GNU General Public License for more details.\n"
                                 "\n"
                                 "You should have received a copy of the GNU General Public License\n"
                                 "along with this program; if not, write to the Free Software\n"
                                 "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,\n"
                                 "USA."), _("Ok"), FALSE, NULL, NULL);
    g_signal_connect(G_OBJECT(dialog), "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &dialog);
}
