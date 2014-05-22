/* Python interface to inferior thread event registries.

   Copyright (C) 2009-2013 Free Software Foundation, Inc.

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
#include "command.h"
#include "py-events.h"

events_object gdb_py_events;

static PyTypeObject eventregistry_object_type;

/* Implementation of EventRegistry.connect () -> NULL.
   Add FUNCTION to the list of listeners.  */

static PyObject *
evregpy_connect (PyObject *self, PyObject *function)
{
  PyObject *func;
  PyObject *callback_list = (((eventregistry_object *) self)->callbacks);

  if (!PyArg_ParseTuple (function, "O", &func))
    return NULL;

  if (!PyCallable_Check (func))
    {
      PyErr_SetString (PyExc_RuntimeError, "Function is not callable");
      return NULL;
    }

  if (PyList_Append (callback_list, func) < 0)
    return NULL;

  Py_RETURN_NONE;
}

/* Implementation of EventRegistry.disconnect () -> NULL.
   Remove FUNCTION from the list of listeners.  */

static PyObject *
evregpy_disconnect (PyObject *self, PyObject *function)
{
  PyObject *func;
  int index;
  PyObject *callback_list = (((eventregistry_object *) self)->callbacks);

  if (!PyArg_ParseTuple (function, "O", &func))
    return NULL;

  index = PySequence_Index (callback_list, func);
  if (index < 0)
    Py_RETURN_NONE;

  if (PySequence_DelItem (callback_list, index) < 0)
    return NULL;

  Py_RETURN_NONE;
}

/* Create a new event registry.  This function uses PyObject_New
   and therefore returns a new reference that callers must handle.  */

eventregistry_object *
create_eventregistry_object (void)
{
  eventregistry_object *eventregistry_obj;

  eventregistry_obj = PyObject_New (eventregistry_object,
                                    &eventregistry_object_type);

  if (!eventregistry_obj)
    return NULL;

  eventregistry_obj->callbacks = PyList_New (0);
  if (!eventregistry_obj->callbacks)
    return NULL;

  return eventregistry_obj;
}

static void
evregpy_dealloc (PyObject *self)
{
  Py_XDECREF (((eventregistry_object *) self)->callbacks);
  Py_TYPE (self)->tp_free (self);
}

/* Initialize the Python event registry code.  */

void
gdbpy_initialize_eventregistry (void)
{
  if (PyType_Ready (&eventregistry_object_type) < 0)
    return;

  Py_INCREF (&eventregistry_object_type);
  PyModule_AddObject (gdb_module, "EventRegistry",
                      (PyObject *) &eventregistry_object_type);
}

/* Retern the number of listeners currently connected to this
   registry.  */

int
evregpy_no_listeners_p (eventregistry_object *registry)
{
  return PyList_Size (registry->callbacks) == 0;
}

static PyMethodDef eventregistry_object_methods[] =
{
  { "connect", evregpy_connect, METH_VARARGS, "Add function" },
  { "disconnect", evregpy_disconnect, METH_VARARGS, "Remove function" },
  { NULL } /* Sentinel.  */
};

static PyTypeObject eventregistry_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.EventRegistry",                        /* tp_name */
  sizeof (eventregistry_object),              /* tp_basicsize */
  0,                                          /* tp_itemsize */
  evregpy_dealloc,                            /* tp_dealloc */
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
  Py_TPFLAGS_DEFAULT,                         /* tp_flags */
  "GDB event registry object",                /* tp_doc */
  0,                                          /* tp_traverse */
  0,                                          /* tp_clear */
  0,                                          /* tp_richcompare */
  0,                                          /* tp_weaklistoffset */
  0,                                          /* tp_iter */
  0,                                          /* tp_iternext */
  eventregistry_object_methods,               /* tp_methods */
  0,                                          /* tp_members */
  0,                                          /* tp_getset */
  0,                                          /* tp_base */
  0,                                          /* tp_dict */
  0,                                          /* tp_descr_get */
  0,                                          /* tp_descr_set */
  0,                                          /* tp_dictoffset */
  0,                                          /* tp_init */
  0                                           /* tp_alloc */
};
