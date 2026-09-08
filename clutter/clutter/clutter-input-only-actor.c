/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Copyright (C) 2022  Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
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
 */

#include "config.h"

#include "clutter/clutter-input-only-actor.h"

#include "clutter/clutter/clutter-actor-private.h"
#include "clutter/clutter-input-only-action.h"

struct _ClutterInputOnlyActor
{
  ClutterActor parent;

  ClutterActor *actor_for_cursor;
};

enum
{
  PROP_0,

  PROP_ACTOR_FOR_CURSOR,

  PROP_LAST
};

static GParamSpec *obj_props[PROP_LAST];

G_DEFINE_TYPE (ClutterInputOnlyActor, clutter_input_only_actor,
               CLUTTER_TYPE_ACTOR)

static ClutterCursor *
clutter_input_only_actor_get_cursor_for_sprite (ClutterActor *actor,
                                                ClutterSprite *sprite)
{
  ClutterInputOnlyActor *input_only_actor = CLUTTER_INPUT_ONLY_ACTOR (actor);
  ClutterActorClass *parent_class = CLUTTER_ACTOR_CLASS (clutter_input_only_actor_parent_class);

  if (!input_only_actor->actor_for_cursor)
    return parent_class->get_cursor_for_sprite (actor, sprite);

  return clutter_actor_get_cursor_for_sprite (input_only_actor->actor_for_cursor,
                                              sprite);
}

static void
clutter_input_only_actor_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  ClutterInputOnlyActor *input_only_actor = CLUTTER_INPUT_ONLY_ACTOR (object);

  switch (prop_id)
    {
    case PROP_ACTOR_FOR_CURSOR:
      g_set_object (&input_only_actor->actor_for_cursor,
                    g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clutter_input_only_actor_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  ClutterInputOnlyActor *input_only_actor = CLUTTER_INPUT_ONLY_ACTOR (object);

  switch (prop_id)
    {
    case PROP_ACTOR_FOR_CURSOR:
      g_value_set_object (value, input_only_actor->actor_for_cursor);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clutter_input_only_actor_finalize (GObject *object)
{
  ClutterInputOnlyActor *input_only_actor = CLUTTER_INPUT_ONLY_ACTOR (object);

  g_clear_object (&input_only_actor->actor_for_cursor);

  G_OBJECT_CLASS (clutter_input_only_actor_parent_class)->finalize (object);
}

static void
clutter_input_only_actor_class_init (ClutterInputOnlyActorClass *klass)
{
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  actor_class->get_cursor_for_sprite = clutter_input_only_actor_get_cursor_for_sprite;

  obj_props[PROP_ACTOR_FOR_CURSOR] =
    g_param_spec_object ("actor-for-cursor",
                         "Actor for Cursor",
                         "The actor to use for determining the cursor when the pointer is grabbed by this input-only actor",
                         CLUTTER_TYPE_ACTOR,
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS);

  gobject_class->finalize = clutter_input_only_actor_finalize;
  gobject_class->set_property = clutter_input_only_actor_set_property;
  gobject_class->get_property = clutter_input_only_actor_get_property;
  g_object_class_install_properties (gobject_class,
                                     PROP_LAST,
                                     obj_props);
}

static void
clutter_input_only_actor_init (ClutterInputOnlyActor *input_only_actor)
{
}

ClutterInputOnlyActor *
clutter_input_only_actor_new (ClutterInputOnlyHandleEvent handle_event,
                              gpointer                    user_data,
                              GDestroyNotify              user_data_destroy)
{
  ClutterInputOnlyAction *input_only_action;

  input_only_action = clutter_input_only_action_new (handle_event,
                                                     user_data,
                                                     user_data_destroy);
  return g_object_new (CLUTTER_TYPE_INPUT_ONLY_ACTOR,
                       "reactive", TRUE,
                       "actions", input_only_action,
                       NULL);
}
