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

#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>
#include "cc-content-chooser.h"
#include <string.h>

struct _CcContentChooser
{
  GtkWindow parent;
  GtkWidget *buttonstack;
  GtkWidget *selectbutton;
  GtkWidget *stack;
  GtkWidget *applist;
  GtkWidget *fileframe;
  GtkWidget *filelist;
  GtkWidget *filechooser;
  char *uri;
  char **mime_types;
};

struct _CcContentChooserClass
{
  GtkWindowClass parent_class;
};

enum
{
  CANCEL,
  BACK,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(CcContentChooser, cc_content_chooser, GTK_TYPE_WINDOW)

static void
cancel_clicked (CcContentChooser *chooser)
{
  g_free (chooser->uri);
  chooser->uri = NULL;
  gtk_widget_hide (GTK_WIDGET (chooser));
}

static void
select_clicked (CcContentChooser *chooser)
{
  gtk_widget_hide (GTK_WIDGET (chooser));
}

static void
back_clicked (CcContentChooser *chooser)
{
  gtk_stack_set_visible_child_name (GTK_STACK (chooser->stack), "list");
}

static void
file_row_activated (CcContentChooser *chooser)
{
  gtk_stack_set_visible_child_name (GTK_STACK (chooser->stack), "filechooser");
}

static void
stack_child_changed (CcContentChooser *chooser)
{
  const char *visible_child;
  const char *visible_button;

  visible_child = gtk_stack_get_visible_child_name (GTK_STACK (chooser->stack));

  if (strcmp (visible_child, "list") == 0)
    visible_button = "cancel";
  else
    visible_button = "back";

  gtk_stack_set_visible_child_name (GTK_STACK (chooser->buttonstack), visible_button);
}

static void
file_selection_changed (CcContentChooser *chooser)
{
  g_free (chooser->uri);
  chooser->uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (chooser->filechooser));
  gtk_widget_set_sensitive (chooser->selectbutton, chooser->uri != NULL);
}

static void
cc_content_chooser_init (CcContentChooser *chooser)
{
  gtk_widget_init_template (GTK_WIDGET (chooser));
}

static void
cc_content_chooser_finalize (GObject *object)
{
  CcContentChooser *chooser = CC_CONTENT_CHOOSER (object);

  g_free (chooser->uri);
  g_strfreev (chooser->mime_types);

  G_OBJECT_CLASS (cc_content_chooser_parent_class)->finalize (object);
}

static void
cc_content_chooser_class_init (CcContentChooserClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkBindingSet *binding_set;

  object_class->finalize = cc_content_chooser_finalize;

  signals[CANCEL] =
    g_signal_new_class_handler ("cancel",
                                G_TYPE_FROM_CLASS (object_class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (cancel_clicked),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 0);
  signals[BACK] =
    g_signal_new_class_handler ("back",
                                G_TYPE_FROM_CLASS (object_class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (back_clicked),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 0);

  binding_set = gtk_binding_set_by_class (class);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Escape, 0, "cancel", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Back, 0, "back", 0);
  if (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL)
    gtk_binding_entry_add_signal (binding_set, GDK_KEY_Right, GDK_MOD1_MASK, "back", 0);
  else
    gtk_binding_entry_add_signal (binding_set, GDK_KEY_Left, GDK_MOD1_MASK, "back", 0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/freedesktop/content-chooser/content-chooser.ui");

  gtk_widget_class_bind_template_callback (widget_class, stack_child_changed);
  gtk_widget_class_bind_template_callback (widget_class, cancel_clicked);
  gtk_widget_class_bind_template_callback (widget_class, select_clicked);
  gtk_widget_class_bind_template_callback (widget_class, back_clicked);
  gtk_widget_class_bind_template_callback (widget_class, file_row_activated);
  gtk_widget_class_bind_template_callback (widget_class, file_selection_changed);

  gtk_widget_class_bind_template_child (widget_class, CcContentChooser, buttonstack);
  gtk_widget_class_bind_template_child (widget_class, CcContentChooser, selectbutton);
  gtk_widget_class_bind_template_child (widget_class, CcContentChooser, stack);
  gtk_widget_class_bind_template_child (widget_class, CcContentChooser, applist);
  gtk_widget_class_bind_template_child (widget_class, CcContentChooser, fileframe);
  gtk_widget_class_bind_template_child (widget_class, CcContentChooser, filelist);
  gtk_widget_class_bind_template_child (widget_class, CcContentChooser, filechooser);
}

GtkWidget *
cc_content_chooser_new (void)
{
  return (GtkWidget *)g_object_new (cc_content_chooser_get_type (), NULL);
}

const char *
cc_content_chooser_get_uri (CcContentChooser *chooser)
{
  return chooser->uri;
}

static gboolean
filter_any (const GtkFileFilterInfo *filter_info,
            gpointer data)
{
  return TRUE;
}

void
cc_content_chooser_set_mime_types (CcContentChooser *chooser,
                                   const char **mime_types)
{
  chooser->mime_types = g_strdupv ((char **)mime_types);

  if (chooser->mime_types && *chooser->mime_types)
    {
      int i;
      GtkFileFilter *no_filter;
      gboolean text_added, image_added;

      no_filter = gtk_file_filter_new ();
      gtk_file_filter_set_name (no_filter, "All files");
      gtk_file_filter_add_custom (no_filter, 0, filter_any, NULL, NULL);
      gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser->filechooser), no_filter);
      gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (chooser->filechooser), no_filter);

      /* FIXME: not sure this is really useful.
       * Files is already the fallback option
       */
      text_added = FALSE;
      image_added = FALSE;
      for (i = 0; chooser->mime_types[i]; i++)
        {
          GtkFileFilter *filter;

          if (g_content_type_is_a (chooser->mime_types[i], "text/*"))
            {
              if (!text_added)
                {
                  text_added = TRUE;
                  filter = gtk_file_filter_new ();
                  gtk_file_filter_set_name (filter, "Text files");
                  gtk_file_filter_add_mime_type (filter, "text/*");
                  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser->filechooser), filter);
                }
            }
          else if (g_content_type_is_a (chooser->mime_types[i], "image/*"))
            {
              if (!image_added)
                {
                  image_added = TRUE;
                  filter = gtk_file_filter_new ();
                  gtk_file_filter_set_name (filter, "Images");
                  gtk_file_filter_add_mime_type (filter, "image/*");
                  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser->filechooser), filter);
                }
            }
        }
    }
}

void
cc_content_chooser_set_app_id (CcContentChooser *chooser,
                               const char       *app_id)
{
  char *desktop_id;
  g_autoptr (GAppInfo) app = NULL;

  desktop_id = g_strconcat (app_id, ".desktop", NULL);
  app = G_APP_INFO (g_desktop_app_info_new (desktop_id));

  if (app != NULL)
    {
      char *title;

      title = g_strdup_printf (_("Select an item for %s"),
                               g_app_info_get_display_name (app));
      gtk_window_set_title (GTK_WINDOW (chooser), title);
      g_free (title);
    }
}
