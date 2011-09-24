/* Python interface to inferior signal stop events.

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

#include "py-stopevent.h"

static PyTypeObject signal_event_object_type;

PyObject *
create_signal_event_object (enum target_signal stop_signal)
{
  const char *signal_name;
  PyObject *signal_event_obj =
      create_stop_event_object (&signal_event_object_type);

  if (!signal_event_obj)
    goto fail;

  signal_name = target_signal_to_name (stop_signal);

  if (evpy_add_attribute (signal_event_obj,
                          "stop_signal",
                          PyString_FromString (signal_name)) < 0)
    goto fail;

  return signal_event_obj;

  fail:
   Py_XDECREF (signal_event_obj);
   return NULL;
}

GDBPY_NEW_EVENT_TYPE (signal,
                      "gdb.SignalEvent",
                      "SignalEvent",
                      "GDB signal event object",
                      stop_event_object_type,
                      static);
