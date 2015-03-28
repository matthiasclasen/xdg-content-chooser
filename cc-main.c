/* xdg-content-chooser
 * Copyright (C) 2015 Matthias Clasen
 *
 * This probram is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <gtk/gtk.h>
#include "cc-content-chooser.h"

int
main (int argc, char *argv[])
{
  GtkWidget *chooser;
  const char *uri;

  gtk_init (NULL, NULL);

  chooser = cc_content_chooser_new ();

  if (argc > 1)
    {
      char **types;
      int i, j;

      types = g_new (char *, argc);
      for (i = 1, j = 0; i < argc; i++)
        {
          types[j] = g_content_type_from_mime_type (argv[i]);
          if (types[j] == NULL)
            {
              g_printerr ("Not a valid mime type: %s\n", argv[i]);
              continue;
            }

          j++;
        }
      types[j] = NULL;

      cc_content_chooser_set_mime_types (CC_CONTENT_CHOOSER (chooser), (const char **)types);

      g_strfreev (types);
    }

  g_signal_connect (chooser, "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
  g_signal_connect (chooser, "hide", G_CALLBACK (gtk_main_quit), NULL);

  gtk_window_present (GTK_WINDOW (chooser));

  gtk_main ();

  uri = cc_content_chooser_get_uri (CC_CONTENT_CHOOSER (chooser));
  if (uri != NULL)
    {
      g_print ("%s", uri);
      return 0;
    }

  return 1;
}
