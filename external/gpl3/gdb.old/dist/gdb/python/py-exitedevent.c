/* Python interface to inferior exit events.

   Copyright (C) 2009-2014 Free Software Foundation, Inc.

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

static PyTypeObject exited_event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");

static PyObject *
create_exited_event_object (const LONGEST *exit_code, struct inferior *inf)
{
  PyObject *exited_event;
  PyObject *inf_obj = NULL;

  exited_event = create_event_object (&exited_event_object_type);

  if (!exited_event)
    goto fail;

  if (exit_code)
    {
      PyObject *exit_code_obj = PyLong_FromLongLong (*exit_code);
      int failed;

      if (exit_code_obj == NULL)
	goto fail;

      failed = evpy_add_attribute (exited_event, "exit_code",
				   exit_code_obj) < 0;
      Py_DECREF (exit_code_obj);
      if (failed)
	goto fail;
    }

  inf_obj = inferior_to_inferior_object (inf);
  if (!inf_obj || evpy_add_attribute (exited_event,
                                      "inferior",
                                      inf_obj) < 0)
    goto fail;
  Py_DECREF (inf_obj);

  return exited_event;

 fail:
  Py_XDECREF (inf_obj);
  Py_XDECREF (exited_event);
  return NULL;
}

/* Callback that is used when an exit event occurs.  This function
   will create a new Python exited event object.  */

int
emit_exited_event (const LONGEST *exit_code, struct inferior *inf)
{
  PyObject *event;

  if (evregpy_no_listeners_p (gdb_py_events.exited))
    return 0;

  event = create_exited_event_object (exit_code, inf);

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
