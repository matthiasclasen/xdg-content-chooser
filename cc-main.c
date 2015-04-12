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

static char *action = NULL;
static char *app_id = NULL;
static char *title = NULL;

static GOptionEntry entries[] = {
  { "action", 0, 0, G_OPTION_ARG_STRING, &action, "Action", "ACTION" },
  { "caller", 0, 0, G_OPTION_ARG_STRING, &app_id, "Application ID", "ID" },
  { "title", 0, 0, G_OPTION_ARG_STRING, &title, "Title", "TITLE" },
  { NULL }
};

int
main (int argc, char *argv[])
{
  GtkWidget *chooser;
  const char *uri;
  g_autoptr (GOptionContext) context = NULL;
  GError *error = NULL;

  gtk_init (NULL, NULL);

  chooser = cc_content_chooser_new ();

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, "");
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("Failed to parse commandline: %s\n", error->message);
      return 1;
    }

  if (app_id)
    cc_content_chooser_set_app_id (CC_CONTENT_CHOOSER (chooser), app_id);

  if (g_strcmp0 (action, "open") == 0)
    cc_content_chooser_set_action (CC_CONTENT_CHOOSER (chooser), CC_CONTENT_CHOOSER_ACTION_OPEN);
  else if (g_strcmp0 (action, "create") == 0)
    cc_content_chooser_set_action (CC_CONTENT_CHOOSER (chooser), CC_CONTENT_CHOOSER_ACTION_CREATE);
  else
    {
      g_printerr ("Not a valid content chooser action: %s\n", action);
      return 1;
    }

  if (title)
    cc_content_chooser_set_title (CC_CONTENT_CHOOSER (chooser), title);

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
