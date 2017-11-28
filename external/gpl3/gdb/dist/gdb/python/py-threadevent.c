/* Copyright (C) 2009-2017 Free Software Foundation, Inc.

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
#include "infrun.h"

/* thread events can either be thread specific or process wide.  If gdb is
   running in non-stop mode then the event is thread specific, otherwise
   it is process wide.
   This function returns the currently stopped thread in non-stop mode and
   Py_None otherwise.  In each case it returns a borrowed reference.  */

static PyObject *get_event_thread (void)
  CPYCHECKER_RETURNS_BORROWED_REF;

static PyObject *
get_event_thread (void)
{
  PyObject *thread = NULL;

  if (non_stop)
    thread = (PyObject *) find_thread_object (inferior_ptid);
  else
    thread = Py_None;

  if (!thread)
    {
      PyErr_SetString (PyExc_RuntimeError, "Could not find event thread");
      return NULL;
    }

  return thread;
}

PyObject *
create_thread_event_object (PyTypeObject *py_type)
{
  PyObject *thread = NULL;

  gdbpy_ref<> thread_event_obj (create_event_object (py_type));
  if (thread_event_obj == NULL)
    return NULL;

  thread = get_event_thread ();
  if (!thread)
    return NULL;

  if (evpy_add_attribute (thread_event_obj.get (),
                          "inferior_thread",
                          thread) < 0)
    return NULL;

  return thread_event_obj.release ();
}

GDBPY_NEW_EVENT_TYPE (thread,
                      "gdb.ThreadEvent",
                      "ThreadEvent",
                      "GDB thread event object",
                      event_object_type);
