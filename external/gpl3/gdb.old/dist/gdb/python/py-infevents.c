/* Python interface to inferior function events.

   Copyright (C) 2013-2017 Free Software Foundation, Inc.

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

extern PyTypeObject inferior_call_pre_event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");
extern PyTypeObject inferior_call_post_event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");
extern PyTypeObject register_changed_event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");
extern PyTypeObject memory_changed_event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");

/* Construct either a gdb.InferiorCallPreEvent or a
   gdb.InferiorCallPostEvent. */

static PyObject *
create_inferior_call_event_object (inferior_call_kind flag, ptid_t ptid,
				   CORE_ADDR addr)
{
  gdbpy_ref<> event;
  int failed;

  switch (flag)
    {
    case INFERIOR_CALL_PRE:
      event.reset (create_event_object (&inferior_call_pre_event_object_type));
      break;
    case INFERIOR_CALL_POST:
      event.reset (create_event_object (&inferior_call_post_event_object_type));
      break;
    default:
      gdb_assert_not_reached ("invalid inferior_call_kind");
    }

  gdbpy_ref<> ptid_obj (gdbpy_create_ptid_object (ptid));
  if (ptid_obj == NULL)
    return NULL;

  if (evpy_add_attribute (event.get (), "ptid", ptid_obj.get ()) < 0)
    return NULL;

  gdbpy_ref<> addr_obj (PyLong_FromLongLong (addr));
  if (addr_obj == NULL)
    return NULL;

  if (evpy_add_attribute (event.get (), "address", addr_obj.get ()) < 0)
    return NULL;

  return event.release ();
}

/* Construct a gdb.RegisterChangedEvent containing the affected
   register number. */

static PyObject *
create_register_changed_event_object (struct frame_info *frame, 
				      int regnum)
{
  gdbpy_ref<> event (create_event_object (&register_changed_event_object_type));
  if (event == NULL)
    return NULL;

  gdbpy_ref<> frame_obj (frame_info_to_frame_object (frame));
  if (frame_obj == NULL)
    return NULL;

  if (evpy_add_attribute (event.get (), "frame", frame_obj.get ()) < 0)
    return NULL;

  gdbpy_ref<> regnum_obj (PyLong_FromLongLong (regnum));
  if (regnum_obj == NULL)
    return NULL;

  if (evpy_add_attribute (event.get (), "regnum", regnum_obj.get ()) < 0)
    return NULL;

  return event.release ();
}

/* Construct a gdb.MemoryChangedEvent describing the extent of the
   affected memory. */

static PyObject *
create_memory_changed_event_object (CORE_ADDR addr, ssize_t len)
{
  gdbpy_ref<> event (create_event_object (&memory_changed_event_object_type));

  if (event == NULL)
    return NULL;

  gdbpy_ref<> addr_obj (PyLong_FromLongLong (addr));
  if (addr_obj == NULL)
    return NULL;

  if (evpy_add_attribute (event.get (), "address", addr_obj.get ()) < 0)
    return NULL;

  gdbpy_ref<> len_obj (PyLong_FromLong (len));
  if (len_obj == NULL)
    return NULL;

  if (evpy_add_attribute (event.get (), "length", len_obj.get ()) < 0)
    return NULL;

  return event.release ();
}

/* Callback function which notifies observers when an event occurs which
   calls a function in the inferior.
   This function will create a new Python inferior-call event object.
   Return -1 if emit fails.  */

int
emit_inferior_call_event (inferior_call_kind flag, ptid_t thread,
			  CORE_ADDR addr)
{
  if (evregpy_no_listeners_p (gdb_py_events.inferior_call))
    return 0;

  gdbpy_ref<> event (create_inferior_call_event_object (flag, thread, addr));
  if (event != NULL)
    return evpy_emit_event (event.get (), gdb_py_events.inferior_call);
  return -1;
}

/* Callback when memory is modified by the user.  This function will
   create a new Python memory changed event object. */

int
emit_memory_changed_event (CORE_ADDR addr, ssize_t len)
{
  if (evregpy_no_listeners_p (gdb_py_events.memory_changed))
    return 0;

  gdbpy_ref<> event (create_memory_changed_event_object (addr, len));
  if (event != NULL)
    return evpy_emit_event (event.get (), gdb_py_events.memory_changed);
  return -1;
}

/* Callback when a register is modified by the user.  This function
   will create a new Python register changed event object. */

int
emit_register_changed_event (struct frame_info* frame, int regnum)
{
  if (evregpy_no_listeners_p (gdb_py_events.register_changed))
    return 0;

  gdbpy_ref<> event (create_register_changed_event_object (frame, regnum));
  if (event != NULL)
    return evpy_emit_event (event.get (), gdb_py_events.register_changed);
  return -1;
}


GDBPY_NEW_EVENT_TYPE (inferior_call_pre,
		      "gdb.InferiorCallPreEvent",
		      "InferiorCallPreEvent",
		      "GDB inferior function pre-call event object",
		      event_object_type);

GDBPY_NEW_EVENT_TYPE (inferior_call_post,
		      "gdb.InferiorCallPostEvent",
		      "InferiorCallPostEvent",
		      "GDB inferior function post-call event object",
		      event_object_type);

GDBPY_NEW_EVENT_TYPE (register_changed,
		      "gdb.RegisterChangedEvent",
		      "RegisterChangedEvent",
		      "GDB register change event object",
		      event_object_type);

GDBPY_NEW_EVENT_TYPE (memory_changed,
		      "gdb.MemoryChangedEvent",
		      "MemoryChangedEvent",
		      "GDB memory change event object",
		      event_object_type);
