/* Python interface to inferior events.

   Copyright (C) 2009-2017 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef GDB_PY_EVENTS_H
#define GDB_PY_EVENTS_H

#include "command.h"
#include "python-internal.h"
#include "inferior.h"

extern PyTypeObject thread_event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");

/* Stores a list of objects to be notified when the event for which this
   registry tracks occurs.  */

typedef struct
{
  PyObject_HEAD

  PyObject *callbacks;
} eventregistry_object;

/* Struct holding references to event registries both in python and c.
   This is meant to be a singleton.  */

typedef struct
{
  eventregistry_object *stop;
  eventregistry_object *cont;
  eventregistry_object *exited;
  eventregistry_object *new_objfile;
  eventregistry_object *clear_objfiles;
  eventregistry_object *inferior_call;
  eventregistry_object *memory_changed;
  eventregistry_object *register_changed;
  eventregistry_object *breakpoint_created;
  eventregistry_object *breakpoint_deleted;
  eventregistry_object *breakpoint_modified;
  eventregistry_object *before_prompt;

  PyObject *module;

} events_object;

/* Python events singleton.  */
extern events_object gdb_py_events;

extern eventregistry_object *create_eventregistry_object (void);
extern int evregpy_no_listeners_p (eventregistry_object *registry);

#endif /* GDB_PY_EVENTS_H */
