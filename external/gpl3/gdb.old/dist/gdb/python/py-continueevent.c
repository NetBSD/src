/* Python interface to inferior continue events.

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

#include "defs.h"
#include "py-event.h"
#include "py-ref.h"

extern PyTypeObject continue_event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");

static PyObject *
create_continue_event_object (void)
{
  return create_thread_event_object (&continue_event_object_type);
}

/* Callback function which notifies observers when a continue event occurs.
   This function will create a new Python continue event object.
   Return -1 if emit fails.  */

int
emit_continue_event (ptid_t ptid)
{
  if (evregpy_no_listeners_p (gdb_py_events.cont))
    return 0;

  gdbpy_ref<> event (create_continue_event_object ());
  if (event != NULL)
    return evpy_emit_event (event.get (), gdb_py_events.cont);
  return -1;
}

GDBPY_NEW_EVENT_TYPE (continue,
                      "gdb.ContinueEvent",
                      "ContinueEvent",
                      "GDB continue event object",
                      thread_event_object_type);
