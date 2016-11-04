/* Python interface to inferior threads.

   Copyright (C) 2009-2016 Free Software Foundation, Inc.

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
#include "gdbthread.h"
#include "inferior.h"
#include "python-internal.h"

extern PyTypeObject thread_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("thread_object");

/* Require that INFERIOR be a valid inferior ID.  */
#define THPY_REQUIRE_VALID(Thread)				\
  do {								\
    if (!Thread->thread)					\
      {								\
	PyErr_SetString (PyExc_RuntimeError,			\
			 _("Thread no longer exists."));	\
	return NULL;						\
      }								\
  } while (0)

thread_object *
create_thread_object (struct thread_info *tp)
{
  thread_object *thread_obj;

  thread_obj = PyObject_New (thread_object, &thread_object_type);
  if (!thread_obj)
    return NULL;

  thread_obj->thread = tp;
  thread_obj->inf_obj = find_inferior_object (ptid_get_pid (tp->ptid));

  return thread_obj;
}

static void
thpy_dealloc (PyObject *self)
{
  Py_DECREF (((thread_object *) self)->inf_obj);
  Py_TYPE (self)->tp_free (self);
}

static PyObject *
thpy_get_name (PyObject *self, void *ignore)
{
  thread_object *thread_obj = (thread_object *) self;
  const char *name;

  THPY_REQUIRE_VALID (thread_obj);

  name = thread_obj->thread->name;
  if (name == NULL)
    name = target_thread_name (thread_obj->thread);

  if (name == NULL)
    Py_RETURN_NONE;

  return PyString_FromString (name);
}

static int
thpy_set_name (PyObject *self, PyObject *newvalue, void *ignore)
{
  thread_object *thread_obj = (thread_object *) self;
  char *name;

  if (! thread_obj->thread)
    {
      PyErr_SetString (PyExc_RuntimeError, _("Thread no longer exists."));
      return -1;
    }

  if (newvalue == NULL)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete `name' attribute."));
      return -1;
    }
  else if (newvalue == Py_None)
    name = NULL;
  else if (! gdbpy_is_string (newvalue))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The value of `name' must be a string."));
      return -1;
    }
  else
    {
      name = python_string_to_host_string (newvalue);
      if (! name)
	return -1;
    }

  xfree (thread_obj->thread->name);
  thread_obj->thread->name = name;

  return 0;
}

/* Getter for InferiorThread.num.  */

static PyObject *
thpy_get_num (PyObject *self, void *closure)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  return PyLong_FromLong (thread_obj->thread->per_inf_num);
}

/* Getter for InferiorThread.global_num.  */

static PyObject *
thpy_get_global_num (PyObject *self, void *closure)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  return PyLong_FromLong (thread_obj->thread->global_num);
}

/* Getter for InferiorThread.ptid  -> (pid, lwp, tid).
   Returns a tuple with the thread's ptid components.  */

static PyObject *
thpy_get_ptid (PyObject *self, void *closure)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  return gdbpy_create_ptid_object (thread_obj->thread->ptid);
}

/* Getter for InferiorThread.inferior -> Inferior.  */

static PyObject *
thpy_get_inferior (PyObject *self, void *ignore)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  return thread_obj->inf_obj;
}

/* Implementation of InferiorThread.switch ().
   Makes this the GDB selected thread.  */

static PyObject *
thpy_switch (PyObject *self, PyObject *args)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  TRY
    {
      switch_to_thread (thread_obj->thread->ptid);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }
  END_CATCH

  Py_RETURN_NONE;
}

/* Implementation of InferiorThread.is_stopped () -> Boolean.
   Return whether the thread is stopped.  */

static PyObject *
thpy_is_stopped (PyObject *self, PyObject *args)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  if (is_stopped (thread_obj->thread->ptid))
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Implementation of InferiorThread.is_running () -> Boolean.
   Return whether the thread is running.  */

static PyObject *
thpy_is_running (PyObject *self, PyObject *args)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  if (is_running (thread_obj->thread->ptid))
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Implementation of InferiorThread.is_exited () -> Boolean.
   Return whether the thread is exited.  */

static PyObject *
thpy_is_exited (PyObject *self, PyObject *args)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  if (is_exited (thread_obj->thread->ptid))
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Implementation of gdb.InfThread.is_valid (self) -> Boolean.
   Returns True if this inferior Thread object still exists
   in GDB.  */

static PyObject *
thpy_is_valid (PyObject *self, PyObject *args)
{
  thread_object *thread_obj = (thread_object *) self;

  if (! thread_obj->thread)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Return a reference to a new Python object representing a ptid_t.
   The object is a tuple containing (pid, lwp, tid). */
PyObject *
gdbpy_create_ptid_object (ptid_t ptid)
{
  int pid;
  long tid, lwp;
  PyObject *ret;

  ret = PyTuple_New (3);
  if (!ret)
    return NULL;

  pid = ptid_get_pid (ptid);
  lwp = ptid_get_lwp (ptid);
  tid = ptid_get_tid (ptid);

  PyTuple_SET_ITEM (ret, 0, PyInt_FromLong (pid));
  PyTuple_SET_ITEM (ret, 1, PyInt_FromLong (lwp));
  PyTuple_SET_ITEM (ret, 2, PyInt_FromLong (tid));
 
  return ret;
}

/* Implementation of gdb.selected_thread () -> gdb.InferiorThread.
   Returns the selected thread object.  */

PyObject *
gdbpy_selected_thread (PyObject *self, PyObject *args)
{
  PyObject *thread_obj;

  thread_obj = (PyObject *) find_thread_object (inferior_ptid);
  if (thread_obj)
    {
      Py_INCREF (thread_obj);
      return thread_obj;
    }

  Py_RETURN_NONE;
}

int
gdbpy_initialize_thread (void)
{
  if (PyType_Ready (&thread_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "InferiorThread",
				 (PyObject *) &thread_object_type);
}

static PyGetSetDef thread_object_getset[] =
{
  { "name", thpy_get_name, thpy_set_name,
    "The name of the thread, as set by the user or the OS.", NULL },
  { "num", thpy_get_num, NULL,
    "Per-inferior number of the thread, as assigned by GDB.", NULL },
  { "global_num", thpy_get_global_num, NULL,
    "Global number of the thread, as assigned by GDB.", NULL },
  { "ptid", thpy_get_ptid, NULL, "ID of the thread, as assigned by the OS.",
    NULL },
  { "inferior", thpy_get_inferior, NULL,
    "The Inferior object this thread belongs to.", NULL },

  { NULL }
};

static PyMethodDef thread_object_methods[] =
{
  { "is_valid", thpy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this inferior thread is valid, false if not." },
  { "switch", thpy_switch, METH_NOARGS,
    "switch ()\n\
Makes this the GDB selected thread." },
  { "is_stopped", thpy_is_stopped, METH_NOARGS,
    "is_stopped () -> Boolean\n\
Return whether the thread is stopped." },
  { "is_running", thpy_is_running, METH_NOARGS,
    "is_running () -> Boolean\n\
Return whether the thread is running." },
  { "is_exited", thpy_is_exited, METH_NOARGS,
    "is_exited () -> Boolean\n\
Return whether the thread is exited." },

  { NULL }
};

PyTypeObject thread_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.InferiorThread",		  /*tp_name*/
  sizeof (thread_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  thpy_dealloc,			  /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  0,				  /*tp_repr*/
  0,				  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  0,				  /*tp_as_mapping*/
  0,				  /*tp_hash */
  0,				  /*tp_call*/
  0,				  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER,  /*tp_flags*/
  "GDB thread object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  thread_object_methods,	  /* tp_methods */
  0,				  /* tp_members */
  thread_object_getset,		  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0				  /* tp_alloc */
};
