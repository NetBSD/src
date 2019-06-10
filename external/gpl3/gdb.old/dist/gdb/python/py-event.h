/* Python interface to inferior events.

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

#ifndef GDB_PY_EVENT_H
#define GDB_PY_EVENT_H

#include "py-events.h"
#include "command.h"
#include "python-internal.h"
#include "inferior.h"
#include "py-ref.h"

/* This macro creates the following functions:

     gdbpy_initialize_{NAME}_event
     Used to add the newly created event type to the gdb module.

   and the python type data structure for the event:

     struct PyTypeObject {NAME}_event_object_type

  NAME is the name of the event.
  PY_PATH is a string representing the module and python name of
    the event.
  PY_NAME a string representing what the event should be called in
    python.
  DOC Python documentation for the new event type
  BASE the base event for this event usually just event_object_type.
*/

#define GDBPY_NEW_EVENT_TYPE(name, py_path, py_name, doc, base) \
\
  PyTypeObject name##_event_object_type		    \
        CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object") \
    = { \
      PyVarObject_HEAD_INIT (NULL, 0)				\
      py_path,                                    /* tp_name */ \
      sizeof (event_object),                      /* tp_basicsize */ \
      0,                                          /* tp_itemsize */ \
      evpy_dealloc,                               /* tp_dealloc */ \
      0,                                          /* tp_print */ \
      0,                                          /* tp_getattr */ \
      0,                                          /* tp_setattr */ \
      0,                                          /* tp_compare */ \
      0,                                          /* tp_repr */ \
      0,                                          /* tp_as_number */ \
      0,                                          /* tp_as_sequence */ \
      0,                                          /* tp_as_mapping */ \
      0,                                          /* tp_hash  */ \
      0,                                          /* tp_call */ \
      0,                                          /* tp_str */ \
      0,                                          /* tp_getattro */ \
      0,                                          /* tp_setattro */ \
      0,                                          /* tp_as_buffer */ \
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */ \
      doc,                                        /* tp_doc */ \
      0,                                          /* tp_traverse */ \
      0,                                          /* tp_clear */ \
      0,                                          /* tp_richcompare */ \
      0,                                          /* tp_weaklistoffset */ \
      0,                                          /* tp_iter */ \
      0,                                          /* tp_iternext */ \
      0,                                          /* tp_methods */ \
      0,                                          /* tp_members */ \
      0,                                          /* tp_getset */ \
      &base,                                      /* tp_base */ \
      0,                                          /* tp_dict */ \
      0,                                          /* tp_descr_get */ \
      0,                                          /* tp_descr_set */ \
      0,                                          /* tp_dictoffset */ \
      0,                                          /* tp_init */ \
      0                                           /* tp_alloc */ \
    }; \
\
int \
gdbpy_initialize_##name##_event (void) \
{ \
  return gdbpy_initialize_event_generic (&name##_event_object_type, \
					 py_name);		    \
}

typedef struct
{
  PyObject_HEAD

  PyObject *dict;
} event_object;

extern int emit_continue_event (ptid_t ptid);
extern int emit_exited_event (const LONGEST *exit_code, struct inferior *inf);

/* For inferior function call events, discriminate whether event is
   before or after the call. */

typedef enum
{
  /* Before the call */
  INFERIOR_CALL_PRE,
  /* after the call */
  INFERIOR_CALL_POST,
} inferior_call_kind;

extern int emit_inferior_call_event (inferior_call_kind kind,
				     ptid_t thread, CORE_ADDR addr);
extern int emit_register_changed_event (struct frame_info *frame,
				        int regnum);
extern int emit_memory_changed_event (CORE_ADDR addr, ssize_t len);
extern int evpy_emit_event (PyObject *event,
                            eventregistry_object *registry);

extern PyObject *create_event_object (PyTypeObject *py_type);
extern PyObject *create_thread_event_object (PyTypeObject *py_type);
extern int emit_new_objfile_event (struct objfile *objfile);
extern int emit_clear_objfiles_event (void);

extern void evpy_dealloc (PyObject *self);
extern int evpy_add_attribute (PyObject *event,
                               const char *name, PyObject *attr)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_event_generic (PyTypeObject *type, const char *name)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;

#endif /* GDB_PY_EVENT_H */
