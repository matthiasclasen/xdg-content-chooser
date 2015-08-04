#include "config.h"

#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include "xdp-dbus.h"
#include "xdp-util.h"
#include "xdp-error.h"


typedef struct {
  GDBusMethodInvocation *invocation;
  const char *app_id;
  gboolean create;
  guint32 handle;
  char *basename;
} ContentChooserData;

static void
got_permission_handle (GObject      *object,
                       GAsyncResult *result,
                       gpointer      user_data)
{
  GDBusConnection *connection = G_DBUS_CONNECTION (object);
  g_autofree ContentChooserData *data = user_data;
  g_autoptr(GVariant) res = NULL;
  g_autoptr(GError) error = NULL;
  g_autofree char *basename = data->basename;
  g_autofree char *path = g_strdup_printf ("%s/doc/%x/%s",
                                           g_get_user_runtime_dir(),
                                           data->handle, basename);

  res = g_dbus_connection_call_finish (connection, result, &error);
  if (res == NULL)
    g_dbus_method_invocation_return_error (data->invocation,
                                           XDP_ERROR, XDP_ERROR_FAILED,
                                           "Granting permissions failed: %s", error->message);
  else
    g_dbus_method_invocation_return_value (data->invocation,
                                           g_variant_new ("(^ay)", path));
}

static void
got_document_handle (GObject      *object,
                     GAsyncResult *result,
                     gpointer      user_data)
{
  GDBusConnection *connection = G_DBUS_CONNECTION (object);
  ContentChooserData *data = user_data;
  g_autoptr(GVariant) res = NULL;
  g_autoptr(GError) error = NULL;
  const char *permissions[4] = { "read", "write", "grant-permissions", NULL };

  res = g_dbus_connection_call_finish (connection, result, &error);
  if (res == NULL)
    {
      g_dbus_method_invocation_return_error (data->invocation,
                                             XDP_ERROR, XDP_ERROR_FAILED,
                                             "Adding document failed: %s", error->message);
      g_free (data);
      return;
    }

  g_variant_get (res, "(u)", &data->handle);

  g_dbus_connection_call (connection,
                          "org.freedesktop.portal.Documents",
                          "/org/freedesktop/portal/documents",
                          "org.freedesktop.portal.Documents",
                          "GrantPermissions",
                          g_variant_new ("(us^as)", data->handle, data->app_id, permissions),
                          G_VARIANT_TYPE ("()"),
                          G_DBUS_CALL_FLAGS_NONE,
                          30000,
                          NULL,
                          got_permission_handle,
                          data);
}

static void
content_chooser_done (GObject      *object,
                      GAsyncResult *result,
                      gpointer      user_data)
{
  g_autoptr(GSubprocess) subprocess = G_SUBPROCESS (object);
  ContentChooserData *data = user_data;
  g_autoptr(GBytes) stdout_buf = NULL;
  g_autoptr(GError) error = NULL;
  const char *uri = NULL;
  g_autoptr (GFile) file = NULL;
  g_autoptr(GDBusConnection) connection = NULL;

  if (!g_subprocess_communicate_finish (subprocess, result, &stdout_buf, NULL, &error))
    {
      g_dbus_method_invocation_return_error (data->invocation,
                                             XDP_ERROR, XDP_ERROR_FAILED,
                                             "Content chooser failed: %s", error->message);
      g_free (data);
      return;
    }

  if (!g_subprocess_get_if_exited (subprocess) ||
      g_subprocess_get_exit_status (subprocess) != 0)
    {
      g_dbus_method_invocation_return_error (data->invocation,
                                             XDP_ERROR, XDP_ERROR_FAILED,
                                             "Content chooser exit %d", g_subprocess_get_exit_status (subprocess));
      g_free (data);
      return;
    }

  uri = g_bytes_get_data (stdout_buf, NULL);
  file = g_file_new_for_uri (uri);
  data->basename = g_file_get_basename (file);

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  g_dbus_connection_call (connection,
                          "org.freedesktop.portal.Documents",
                          "/org/freedesktop/portal/documents",
                          "org.freedesktop.portal.Documents",
                          "Add",
                          g_variant_new ("(s)", uri),
                          G_VARIANT_TYPE ("(u)"),
                          G_DBUS_CALL_FLAGS_NONE,
                          30000,
                          NULL,
                          got_document_handle,
                          data);
}

static void
open_content_chooser (GDBusMethodInvocation *invocation,
                      const char            *app_id,
                      gboolean               create)
{
  GSubprocess *subprocess;
  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) args = NULL;
  ContentChooserData *data;
  GVariant *parameters;
  const char **types = NULL;
  const char *title = NULL;
  int i;

  parameters = g_dbus_method_invocation_get_parameters (invocation);
  if (create)
    g_variant_get (parameters, "(&s)", &title);
  else
    g_variant_get (parameters, "(^a&s)", &types);

  args = g_ptr_array_new ();
  g_ptr_array_add (args, LIBEXECDIR "/xdg-content-chooser");
  g_ptr_array_add (args, "--action");
  g_ptr_array_add (args, create ? "create" : "open");
  if (app_id && app_id[0])
    {
      g_ptr_array_add (args, "--caller");
      g_ptr_array_add (args, (gpointer)app_id);
    }
  if (title && title[0])
    {
      g_ptr_array_add (args, "--title");
      g_ptr_array_add (args, (gpointer)title);
    }
  if (types != NULL)
    {
      for (i = 0; types[i]; i++)
        g_ptr_array_add (args, (gpointer)types[i]);
    }
  g_ptr_array_add (args, NULL);

  subprocess = g_subprocess_newv ((const char **)args->pdata, G_SUBPROCESS_FLAGS_STDOUT_PIPE, &error);
  if (subprocess == NULL)
    {
      g_dbus_method_invocation_return_error (invocation,
                                             XDP_ERROR, XDP_ERROR_FAILED,
                                             "Failed to start content chooser: %s", error->message);
      return;
    }

  data = g_new0 (ContentChooserData, 1);
  data->invocation = invocation;
  data->app_id = app_id;
  data->create = create;

  g_subprocess_communicate_async (subprocess, NULL, NULL, content_chooser_done, data);
}

static void
got_app_id_cb (GObject      *source_object,
               GAsyncResult *res,
               gpointer      user_data)
{
  GDBusMethodInvocation *invocation = G_DBUS_METHOD_INVOCATION (source_object);
  gboolean create = GPOINTER_TO_INT (user_data);
  g_autoptr(GError) error = NULL;
  char *app_id;

  app_id = xdp_invocation_lookup_app_id_finish (invocation, res, &error);

  if (app_id == NULL)
    g_dbus_method_invocation_return_gerror (invocation, error);
  else
    open_content_chooser (invocation, app_id, create);
}

static gboolean
handle_open (XdpDbusContentPortal   *portal,
             GDBusMethodInvocation  *invocation,
             const char            **types)
{
  xdp_invocation_lookup_app_id (invocation, NULL, got_app_id_cb, GINT_TO_POINTER (0));
  return TRUE;
}

static gboolean
handle_create (XdpDbusContentPortal  *portal,
               GDBusMethodInvocation *invocation,
               const char            *title)
{
  xdp_invocation_lookup_app_id (invocation, NULL, got_app_id_cb, GINT_TO_POINTER (1));
  return TRUE;
}

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  XdpDbusContentPortal *helper;
  GError *error = NULL;

  helper = xdp_dbus_content_portal_skeleton_new ();

  g_signal_connect (helper, "handle-open",
                    G_CALLBACK (handle_open), NULL);

  g_signal_connect (helper, "handle-create",
                    G_CALLBACK (handle_create), NULL);

  xdp_connection_track_name_owners (connection);

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (helper),
                                         connection,
                                         "/org/freedesktop/portal/content",
                                         &error))
    {
      g_warning ("error: %s\n", error->message);
      g_error_free (error);
    }
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  exit (1);
}

int
main (int    argc,
      char **argv)
{
  guint owner_id;
  GMainLoop *loop;
  g_autoptr(GList) object_types = NULL;
  g_autoptr(GError) error = NULL;
  g_autofree char *data_path = NULL;
  g_autofree char *uri = NULL;
  g_autoptr(GFile) data_dir = NULL;
  g_autoptr(GFile) db_file = NULL;

  setlocale (LC_ALL, "");

  /* Avoid even loading gvfs to avoid accidental confusion */
  g_setenv ("GIO_USE_VFS", "local", TRUE);

  g_set_prgname (argv[0]);

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "org.freedesktop.portal.ContentPortal",
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             NULL,
                             NULL);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  g_bus_unown_name (owner_id);

  return 0;
}
