/* Python interface to inferior stop events.

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

PyObject *
create_stop_event_object (PyTypeObject *py_type)
{
  PyObject *stop_event_obj = create_thread_event_object (py_type);

  if (!stop_event_obj)
    goto fail;

  return stop_event_obj;

  fail:
   Py_XDECREF (stop_event_obj);
   return NULL;
}

/* Callback observers when a stop event occurs.  This function will create a
   new Python stop event object.  If only a specific thread is stopped the
   thread object of the event will be set to that thread.  Otherwise, if all
   threads are stopped thread object will be set to None.
   return 0 if the event was created and emitted successfully otherwise
   returns -1.  */

int
emit_stop_event (struct bpstats *bs, enum target_signal stop_signal)
{
  PyObject *stop_event_obj = NULL; /* Appease GCC warning.  */

  if (evregpy_no_listeners_p (gdb_py_events.stop))
    return 0;

  if (bs && bs->breakpoint_at
      && bs->breakpoint_at->py_bp_object)
    {
      stop_event_obj = create_breakpoint_event_object ((PyObject *) bs
                                                       ->breakpoint_at
                                                       ->py_bp_object);
      if (!stop_event_obj)
	goto fail;
    }

  /* Check if the signal is "Signal 0" or "Trace/breakpoint trap".  */
  if (stop_signal != TARGET_SIGNAL_0
      && stop_signal != TARGET_SIGNAL_TRAP)
    {
      stop_event_obj =
	  create_signal_event_object (stop_signal);
      if (!stop_event_obj)
	goto fail;
    }

  /* If all fails emit an unknown stop event.  All event types should
     be known and this should eventually be unused.  */
  if (!stop_event_obj)
    {
      stop_event_obj = create_stop_event_object (&stop_event_object_type);
      if (!stop_event_obj)
	goto fail;
    }

  return evpy_emit_event (stop_event_obj, gdb_py_events.stop);

  fail:
   return -1;
}

GDBPY_NEW_EVENT_TYPE (stop,
                      "gdb.StopEvent",
                      "StopEvent",
                      "GDB stop event object",
                      thread_event_object_type,
                      /*no qual*/);
