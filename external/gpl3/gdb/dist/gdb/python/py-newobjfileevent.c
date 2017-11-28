/* Python interface to new object file loading events.

   Copyright (C) 2011-2017 Free Software Foundation, Inc.

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

extern PyTypeObject new_objfile_event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");
extern PyTypeObject clear_objfiles_event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");

static PyObject *
create_new_objfile_event_object (struct objfile *objfile)
{
  PyObject *py_objfile;

  gdbpy_ref<> objfile_event
    (create_event_object (&new_objfile_event_object_type));
  if (objfile_event == NULL)
    return NULL;

  /* Note that objfile_to_objfile_object returns a borrowed reference,
     so we don't need a decref here.  */
  py_objfile = objfile_to_objfile_object (objfile);
  if (!py_objfile || evpy_add_attribute (objfile_event.get (),
                                         "new_objfile",
                                         py_objfile) < 0)
    return NULL;

  return objfile_event.release ();
}

/* Callback function which notifies observers when a new objfile event occurs.
   This function will create a new Python new_objfile event object.
   Return -1 if emit fails.  */

int
emit_new_objfile_event (struct objfile *objfile)
{
  if (evregpy_no_listeners_p (gdb_py_events.new_objfile))
    return 0;

  gdbpy_ref<> event (create_new_objfile_event_object (objfile));
  if (event != NULL)
    return evpy_emit_event (event.get (), gdb_py_events.new_objfile);
  return -1;
}

GDBPY_NEW_EVENT_TYPE (new_objfile,
                      "gdb.NewObjFileEvent",
                      "NewObjFileEvent",
                      "GDB new object file event object",
                      event_object_type);

/* Subroutine of emit_clear_objfiles_event to simplify it.  */

static PyObject *
create_clear_objfiles_event_object (void)
{
  PyObject *py_progspace;

  gdbpy_ref<> objfile_event
    (create_event_object (&clear_objfiles_event_object_type));
  if (objfile_event == NULL)
    return NULL;

  /* Note that pspace_to_pspace_object returns a borrowed reference,
     so we don't need a decref here.  */
  py_progspace = pspace_to_pspace_object (current_program_space);
  if (!py_progspace || evpy_add_attribute (objfile_event.get (),
					   "progspace",
					   py_progspace) < 0)
    return NULL;

  return objfile_event.release ();
}

/* Callback function which notifies observers when the "clear objfiles"
   event occurs.
   This function will create a new Python clear_objfiles event object.
   Return -1 if emit fails.  */

int
emit_clear_objfiles_event (void)
{
  if (evregpy_no_listeners_p (gdb_py_events.clear_objfiles))
    return 0;

  gdbpy_ref<> event (create_clear_objfiles_event_object ());
  if (event != NULL)
    return evpy_emit_event (event.get (), gdb_py_events.clear_objfiles);
  return -1;
}

GDBPY_NEW_EVENT_TYPE (clear_objfiles,
		      "gdb.ClearObjFilesEvent",
		      "ClearObjFilesEvent",
		      "GDB clear object files event object",
		      event_object_type);
