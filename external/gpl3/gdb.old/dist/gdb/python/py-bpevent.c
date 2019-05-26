/* Python interface to inferior breakpoint stop events.

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
#include "py-stopevent.h"
#include "py-ref.h"

extern PyTypeObject breakpoint_event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");

/* Create and initialize a BreakpointEvent object.  This acquires new
   references to BREAKPOINT_LIST and FIRST_BP.  */

PyObject *
create_breakpoint_event_object (PyObject *breakpoint_list, PyObject *first_bp)
{
  gdbpy_ref<> breakpoint_event_obj
    (create_stop_event_object (&breakpoint_event_object_type));

  if (breakpoint_event_obj == NULL)
    return NULL;

  if (evpy_add_attribute (breakpoint_event_obj.get (),
                          "breakpoint",
                          first_bp) < 0)
    return NULL;
  if (evpy_add_attribute (breakpoint_event_obj.get (),
                          "breakpoints",
                          breakpoint_list) < 0)
    return NULL;

  return breakpoint_event_obj.release ();
}

GDBPY_NEW_EVENT_TYPE (breakpoint,
                      "gdb.BreakpointEvent",
                      "BreakpointEvent",
                      "GDB breakpoint stop event object",
                      stop_event_object_type);
