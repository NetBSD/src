/* Python interface to inferior events.

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

void
evpy_dealloc (PyObject *self)
{
  Py_XDECREF (((event_object *) self)->dict);
  Py_TYPE (self)->tp_free (self);
}

PyObject *
create_event_object (PyTypeObject *py_type)
{
  event_object *event_obj;

  event_obj = PyObject_New (event_object, py_type);
  if (!event_obj)
    goto fail;

  event_obj->dict = PyDict_New ();
  if (!event_obj->dict)
    goto fail;

  return (PyObject*) event_obj;

 fail:
  Py_XDECREF (event_obj);
  return NULL;
}

/* Add the attribute ATTR to the event object EVENT.  In
   python this attribute will be accessible by the name NAME.
   returns 0 if the operation succeeds and -1 otherwise.  This
   function acquires a new reference to ATTR.  */

int
evpy_add_attribute (PyObject *event, char *name, PyObject *attr)
{
  return PyObject_SetAttrString (event, name, attr);
}

/* Initialize the Python event code.  */

int
gdbpy_initialize_event (void)
{
  return gdbpy_initialize_event_generic (&event_object_type,
					 "Event");
}

/* Initialize the given event type.  If BASE is not NULL it will
  be set as the types base.
  Returns 0 if initialization was successful -1 otherwise.  */

int
gdbpy_initialize_event_generic (PyTypeObject *type,
                                char *name)
{
  if (PyType_Ready (type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, name, (PyObject *) type);
}


/* Notify the list of listens that the given EVENT has occurred.
   returns 0 if emit is successful -1 otherwise.  */

int
evpy_emit_event (PyObject *event,
                 eventregistry_object *registry)
{
  PyObject *callback_list_copy = NULL;
  Py_ssize_t i;

  /* Create a copy of call back list and use that for
     notifying listeners to avoid skipping callbacks
     in the case of a callback being disconnected during
     a notification.  */
  callback_list_copy = PySequence_List (registry->callbacks);
  if (!callback_list_copy)
    goto fail;

  for (i = 0; i < PyList_Size (callback_list_copy); i++)
    {
      PyObject *func = PyList_GetItem (callback_list_copy, i);
      PyObject *func_result;

      if (func == NULL)
	goto fail;

      func_result = PyObject_CallFunctionObjArgs (func, event, NULL);

      if (func_result == NULL)
	{
	  /* Print the trace here, but keep going -- we want to try to
	     call all of the callbacks even if one is broken.  */
	  gdbpy_print_stack ();
	}
      else
	{
	  Py_DECREF (func_result);
	}
    }

  Py_XDECREF (callback_list_copy);
  Py_XDECREF (event);
  return 0;

 fail:
  gdbpy_print_stack ();
  Py_XDECREF (callback_list_copy);
  Py_XDECREF (event);
  return -1;
}

static PyGetSetDef event_object_getset[] =
{
  { "__dict__", gdb_py_generic_dict, NULL,
    "The __dict__ for this event.", &event_object_type },
  { NULL }
};

PyTypeObject event_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Event",                                /* tp_name */
  sizeof (event_object),                      /* tp_basicsize */
  0,                                          /* tp_itemsize */
  evpy_dealloc,                               /* tp_dealloc */
  0,                                          /* tp_print */
  0,                                          /* tp_getattr */
  0,                                          /* tp_setattr */
  0,                                          /* tp_compare */
  0,                                          /* tp_repr */
  0,                                          /* tp_as_number */
  0,                                          /* tp_as_sequence */
  0,                                          /* tp_as_mapping */
  0,                                          /* tp_hash  */
  0,                                          /* tp_call */
  0,                                          /* tp_str */
  0,                                          /* tp_getattro */
  0,                                          /* tp_setattro */
  0,                                          /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
  "GDB event object",                         /* tp_doc */
  0,                                          /* tp_traverse */
  0,                                          /* tp_clear */
  0,                                          /* tp_richcompare */
  0,                                          /* tp_weaklistoffset */
  0,                                          /* tp_iter */
  0,                                          /* tp_iternext */
  0,                                          /* tp_methods */
  0,                                          /* tp_members */
  event_object_getset,			      /* tp_getset */
  0,                                          /* tp_base */
  0,                                          /* tp_dict */
  0,                                          /* tp_descr_get */
  0,                                          /* tp_descr_set */
  offsetof (event_object, dict),              /* tp_dictoffset */
  0,                                          /* tp_init */
  0                                           /* tp_alloc */
};
