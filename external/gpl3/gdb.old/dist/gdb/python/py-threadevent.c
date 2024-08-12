/* Copyright (C) 2009-2023 Free Software Foundation, Inc.

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
#include "gdbthread.h"

/* See py-event.h.  */

gdbpy_ref<>
py_get_event_thread (ptid_t ptid)
{
  if (non_stop)
    {
      thread_info *thread
	= find_thread_ptid (current_inferior ()->process_target (),
			    ptid);
      if (thread != nullptr)
	return thread_to_thread_object (thread);
      PyErr_SetString (PyExc_RuntimeError, "Could not find event thread");
      return NULL;
    }
  return gdbpy_ref<>::new_reference (Py_None);
}

gdbpy_ref<>
create_thread_event_object (PyTypeObject *py_type, PyObject *thread)
{
  gdb_assert (thread != NULL);

  gdbpy_ref<> thread_event_obj = create_event_object (py_type);
  if (thread_event_obj == NULL)
    return NULL;

  if (evpy_add_attribute (thread_event_obj.get (),
			  "inferior_thread",
			  thread) < 0)
    return NULL;

  return thread_event_obj;
}
