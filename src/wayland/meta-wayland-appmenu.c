/*
 * Wayland Support
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Implements org_kde_kwin_appmenu, through which clients such as Chromium
 * tell where the com.canonical.dbusmenu menu of a surface is exported. The
 * address is exposed on the surface's MetaWindow for the shell to use.
 */

#include "config.h"

#include "wayland/meta-wayland-appmenu.h"

#include "core/window-private.h"
#include "wayland/meta-wayland-private.h"
#include "wayland/meta-wayland-surface-private.h"
#include "wayland/meta-wayland-versions.h"

#include "appmenu-server-protocol.h"

typedef struct _MetaWaylandAppmenu
{
  struct wl_resource *resource;
  MetaWaylandSurface *surface;
  gulong destroy_handler_id;
  gulong pre_state_applied_handler_id;

  char *service_name;
  char *object_path;
} MetaWaylandAppmenu;

struct _MetaWaylandAppmenuManager
{
  GObject parent;
};

#define META_TYPE_WAYLAND_APPMENU_MANAGER (meta_wayland_appmenu_manager_get_type ())
G_DECLARE_FINAL_TYPE (MetaWaylandAppmenuManager, meta_wayland_appmenu_manager,
                      META, WAYLAND_APPMENU_MANAGER, GObject)

G_DEFINE_TYPE (MetaWaylandAppmenuManager, meta_wayland_appmenu_manager,
               G_TYPE_OBJECT)

static void
appmenu_apply (MetaWaylandAppmenu *appmenu)
{
  MetaWindow *window;

  if (!appmenu->surface)
    return;

  window = meta_wayland_surface_get_window (appmenu->surface);
  if (!window)
    return;

  meta_window_set_dbus_appmenu (window,
                                appmenu->service_name,
                                appmenu->object_path);
}

static void
appmenu_detach_surface (MetaWaylandAppmenu *appmenu)
{
  if (!appmenu->surface)
    return;

  g_clear_signal_handler (&appmenu->destroy_handler_id, appmenu->surface);
  g_clear_signal_handler (&appmenu->pre_state_applied_handler_id,
                          appmenu->surface);
  appmenu->surface = NULL;
}

static void
on_surface_destroyed (MetaWaylandSurface *surface,
                      MetaWaylandAppmenu *appmenu)
{
  appmenu_detach_surface (appmenu);
}

/* The surface gets a new MetaWindow each time it is mapped again, so the
 * address is carried over to whichever window the surface has now. */
static void
on_surface_pre_state_applied (MetaWaylandSurface *surface,
                              MetaWaylandAppmenu *appmenu)
{
  MetaWindow *window = meta_wayland_surface_get_window (surface);

  if (window &&
      (g_strcmp0 (window->dbus_appmenu_service_name, appmenu->service_name) != 0 ||
       g_strcmp0 (window->dbus_appmenu_object_path, appmenu->object_path) != 0))
    appmenu_apply (appmenu);
}

static void
appmenu_destructor (struct wl_resource *resource)
{
  MetaWaylandAppmenu *appmenu = wl_resource_get_user_data (resource);

  /* Removing the object means the surface no longer has a menu. */
  g_clear_pointer (&appmenu->service_name, g_free);
  g_clear_pointer (&appmenu->object_path, g_free);
  appmenu_apply (appmenu);

  appmenu_detach_surface (appmenu);
  g_free (appmenu);
}

static void
appmenu_set_address (struct wl_client   *client,
                     struct wl_resource *resource,
                     const char         *service_name,
                     const char         *object_path)
{
  MetaWaylandAppmenu *appmenu = wl_resource_get_user_data (resource);

  g_free (appmenu->service_name);
  appmenu->service_name = g_strdup (service_name);
  g_free (appmenu->object_path);
  appmenu->object_path = g_strdup (object_path);

  appmenu_apply (appmenu);
}

static void
appmenu_release (struct wl_client   *client,
                 struct wl_resource *resource)
{
  wl_resource_destroy (resource);
}

static const struct org_kde_kwin_appmenu_interface meta_wayland_appmenu_interface = {
  appmenu_set_address,
  appmenu_release,
};

static void
appmenu_manager_create (struct wl_client   *client,
                        struct wl_resource *resource,
                        uint32_t            id,
                        struct wl_resource *surface_resource)
{
  MetaWaylandSurface *surface = wl_resource_get_user_data (surface_resource);
  MetaWaylandAppmenu *appmenu;

  appmenu = g_new0 (MetaWaylandAppmenu, 1);
  appmenu->surface = surface;
  appmenu->resource = wl_resource_create (client,
                                          &org_kde_kwin_appmenu_interface,
                                          wl_resource_get_version (resource),
                                          id);
  wl_resource_set_implementation (appmenu->resource,
                                  &meta_wayland_appmenu_interface,
                                  appmenu, appmenu_destructor);

  appmenu->destroy_handler_id =
    g_signal_connect (surface, "destroy",
                      G_CALLBACK (on_surface_destroyed), appmenu);
  appmenu->pre_state_applied_handler_id =
    g_signal_connect (surface, "pre-state-applied",
                      G_CALLBACK (on_surface_pre_state_applied), appmenu);
}

static void
appmenu_manager_release (struct wl_client   *client,
                         struct wl_resource *resource)
{
  wl_resource_destroy (resource);
}

static const struct org_kde_kwin_appmenu_manager_interface meta_wayland_appmenu_manager_interface = {
  appmenu_manager_create,
  appmenu_manager_release,
};

static void
bind_appmenu_manager (struct wl_client *client,
                      void             *data,
                      uint32_t          version,
                      uint32_t          id)
{
  struct wl_resource *resource;

  resource = wl_resource_create (client, &org_kde_kwin_appmenu_manager_interface,
                                 version, id);
  wl_resource_set_implementation (resource,
                                  &meta_wayland_appmenu_manager_interface,
                                  data, NULL);
}

static void
meta_wayland_appmenu_manager_init (MetaWaylandAppmenuManager *appmenu_manager)
{
}

static void
meta_wayland_appmenu_manager_class_init (MetaWaylandAppmenuManagerClass *klass)
{
}

static MetaWaylandAppmenuManager *
meta_wayland_appmenu_manager_new (MetaWaylandCompositor *compositor)
{
  MetaWaylandAppmenuManager *appmenu_manager;

  appmenu_manager = g_object_new (META_TYPE_WAYLAND_APPMENU_MANAGER, NULL);

  if (wl_global_create (compositor->wayland_display,
                        &org_kde_kwin_appmenu_manager_interface,
                        META_ORG_KDE_KWIN_APPMENU_MANAGER_VERSION,
                        appmenu_manager, bind_appmenu_manager) == NULL)
    g_error ("Failed to register a global org_kde_kwin_appmenu_manager object");

  return appmenu_manager;
}

void
meta_wayland_init_appmenu (MetaWaylandCompositor *compositor)
{
  g_object_set_data_full (G_OBJECT (compositor), "-meta-wayland-appmenu-manager",
                          meta_wayland_appmenu_manager_new (compositor),
                          g_object_unref);
}
