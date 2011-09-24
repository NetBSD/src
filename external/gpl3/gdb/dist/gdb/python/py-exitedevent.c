/* Python interface to inferior exit events.

   Copyright (C) 2009, 2010, 2011 Free Software Foundation, Inc.

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

#include "py-event.h"

static PyTypeObject exited_event_object_type;

static PyObject *
create_exited_event_object (const LONGEST *exit_code)
{
  PyObject *exited_event;

  exited_event = create_event_object (&exited_event_object_type);

  if (!exited_event)
    goto fail;

  if (exit_code
      && evpy_add_attribute (exited_event,
			     "exit_code",
			     PyLong_FromLongLong (*exit_code)) < 0)
    goto fail;

  return exited_event;

  fail:
   Py_XDECREF (exited_event);
   return NULL;
}

/* Callback that is used when an exit event occurs.  This function
   will create a new Python exited event object.  */

int
emit_exited_event (const LONGEST *exit_code)
{
  PyObject *event;

  if (evregpy_no_listeners_p (gdb_py_events.exited))
    return 0;

  event = create_exited_event_object (exit_code);

  if (event)
    return evpy_emit_event (event, gdb_py_events.exited);

  return -1;
}


GDBPY_NEW_EVENT_TYPE (exited,
                      "gdb.ExitedEvent",
                      "ExitedEvent",
                      "GDB exited event object",
                      event_object_type,
                      static);
