/* Python interface to inferior exit events.

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

extern PyTypeObject exited_event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");

static PyObject *
create_exited_event_object (const LONGEST *exit_code, struct inferior *inf)
{
  gdbpy_ref<> exited_event (create_event_object (&exited_event_object_type));

  if (exited_event == NULL)
    return NULL;

  if (exit_code)
    {
      gdbpy_ref<> exit_code_obj (PyLong_FromLongLong (*exit_code));

      if (exit_code_obj == NULL)
	return NULL;
      if (evpy_add_attribute (exited_event.get (), "exit_code",
			      exit_code_obj.get ()) < 0)
	return NULL;
    }

  gdbpy_ref<> inf_obj (inferior_to_inferior_object (inf));
  if (inf_obj == NULL || evpy_add_attribute (exited_event.get (),
					     "inferior",
					     inf_obj.get ()) < 0)
    return NULL;

  return exited_event.release ();
}

/* Callback that is used when an exit event occurs.  This function
   will create a new Python exited event object.  */

int
emit_exited_event (const LONGEST *exit_code, struct inferior *inf)
{
  if (evregpy_no_listeners_p (gdb_py_events.exited))
    return 0;

  gdbpy_ref<> event (create_exited_event_object (exit_code, inf));

  if (event != NULL)
    return evpy_emit_event (event.get (), gdb_py_events.exited);

  return -1;
}


GDBPY_NEW_EVENT_TYPE (exited,
                      "gdb.ExitedEvent",
                      "ExitedEvent",
                      "GDB exited event object",
                      event_object_type);
