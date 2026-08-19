# pragma once

#include <glib-object.h>

#include <meta/types.h>

/**
 * MetaTileConstraint:
 * @META_TILE_CONSTRAINT_NONE: No constraint
 * @META_TILE_CONSTRAINT_WINDOW: Window constraint
 * @META_TILE_CONSTRAINT_MONITOR: Monitor constraint
 */
typedef enum
{
  META_WINDOW_CONSTRAINT_NONE,
  META_WINDOW_CONSTRAINT_WINDOW,
  META_WINDOW_CONSTRAINT_MONITOR,
} MetaWindowConstraint;


META_EXPORT
void
meta_window_override_constraints (MetaWindow           *window,
                                  MetaWindowConstraint  top,
                                  MetaWindowConstraint  left,
                                  MetaWindowConstraint  right,
                                  MetaWindowConstraint  bottom);
