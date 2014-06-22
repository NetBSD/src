/* Python interface to objfiles.

   Copyright (C) 2008-2014 Free Software Foundation, Inc.

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
#include "python-internal.h"
#include "charset.h"
#include "objfiles.h"
#include "language.h"

typedef struct
{
  PyObject_HEAD

  /* The corresponding objfile.  */
  struct objfile *objfile;

  /* The pretty-printer list of functions.  */
  PyObject *printers;

  /* The frame filter list of functions.  */
  PyObject *frame_filters;
  /* The type-printer list.  */
  PyObject *type_printers;
} objfile_object;

static PyTypeObject objfile_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("objfile_object");

static const struct objfile_data *objfpy_objfile_data_key;



/* An Objfile method which returns the objfile's file name, or None.  */
static PyObject *
objfpy_get_filename (PyObject *self, void *closure)
{
  objfile_object *obj = (objfile_object *) self;

  if (obj->objfile)
    return PyString_Decode (objfile_name (obj->objfile),
			    strlen (objfile_name (obj->objfile)),
			    host_charset (), NULL);
  Py_RETURN_NONE;
}

static void
objfpy_dealloc (PyObject *o)
{
  objfile_object *self = (objfile_object *) o;

  Py_XDECREF (self->printers);
  Py_XDECREF (self->frame_filters);
  Py_XDECREF (self->type_printers);
  Py_TYPE (self)->tp_free (self);
}

static PyObject *
objfpy_new (PyTypeObject *type, PyObject *args, PyObject *keywords)
{
  objfile_object *self = (objfile_object *) type->tp_alloc (type, 0);

  if (self)
    {
      self->objfile = NULL;

      self->printers = PyList_New (0);
      if (!self->printers)
	{
	  Py_DECREF (self);
	  return NULL;
	}

      self->frame_filters = PyDict_New ();
      if (!self->frame_filters)
	{
	  Py_DECREF (self);
	  return NULL;
	}

      self->type_printers = PyList_New (0);
      if (!self->type_printers)
	{
	  Py_DECREF (self);
	  return NULL;
	}
    }
  return (PyObject *) self;
}

PyObject *
objfpy_get_printers (PyObject *o, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  Py_INCREF (self->printers);
  return self->printers;
}

static int
objfpy_set_printers (PyObject *o, PyObject *value, void *ignore)
{
  PyObject *tmp;
  objfile_object *self = (objfile_object *) o;

  if (! value)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete the pretty_printers attribute."));
      return -1;
    }

  if (! PyList_Check (value))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The pretty_printers attribute must be a list."));
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  tmp = self->printers;
  Py_INCREF (value);
  self->printers = value;
  Py_XDECREF (tmp);

  return 0;
}

/* Return the Python dictionary attribute containing frame filters for
   this object file.  */
PyObject *
objfpy_get_frame_filters (PyObject *o, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  Py_INCREF (self->frame_filters);
  return self->frame_filters;
}

/* Set this object file's frame filters dictionary to FILTERS.  */
static int
objfpy_set_frame_filters (PyObject *o, PyObject *filters, void *ignore)
{
  PyObject *tmp;
  objfile_object *self = (objfile_object *) o;

  if (! filters)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete the frame filters attribute."));
      return -1;
    }

  if (! PyDict_Check (filters))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The frame_filters attribute must be a dictionary."));
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  tmp = self->frame_filters;
  Py_INCREF (filters);
  self->frame_filters = filters;
  Py_XDECREF (tmp);

  return 0;
}

/* Get the 'type_printers' attribute.  */

static PyObject *
objfpy_get_type_printers (PyObject *o, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  Py_INCREF (self->type_printers);
  return self->type_printers;
}

/* Set the 'type_printers' attribute.  */

static int
objfpy_set_type_printers (PyObject *o, PyObject *value, void *ignore)
{
  PyObject *tmp;
  objfile_object *self = (objfile_object *) o;

  if (! value)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete the type_printers attribute."));
      return -1;
    }

  if (! PyList_Check (value))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The type_printers attribute must be a list."));
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  tmp = self->type_printers;
  Py_INCREF (value);
  self->type_printers = value;
  Py_XDECREF (tmp);

  return 0;
}

/* Implementation of gdb.Objfile.is_valid (self) -> Boolean.
   Returns True if this object file still exists in GDB.  */

static PyObject *
objfpy_is_valid (PyObject *self, PyObject *args)
{
  objfile_object *obj = (objfile_object *) self;

  if (! obj->objfile)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}



/* Clear the OBJFILE pointer in an Objfile object and remove the
   reference.  */
static void
py_free_objfile (struct objfile *objfile, void *datum)
{
  struct cleanup *cleanup;
  objfile_object *object = datum;

  cleanup = ensure_python_env (get_objfile_arch (objfile), current_language);
  object->objfile = NULL;
  Py_DECREF ((PyObject *) object);
  do_cleanups (cleanup);
}

/* Return a borrowed reference to the Python object of type Objfile
   representing OBJFILE.  If the object has already been created,
   return it.  Otherwise, create it.  Return NULL and set the Python
   error on failure.  */
PyObject *
objfile_to_objfile_object (struct objfile *objfile)
{
  objfile_object *object;

  object = objfile_data (objfile, objfpy_objfile_data_key);
  if (!object)
    {
      object = PyObject_New (objfile_object, &objfile_object_type);
      if (object)
	{
	  object->objfile = objfile;

	  object->printers = PyList_New (0);
	  if (!object->printers)
	    {
	      Py_DECREF (object);
	      return NULL;
	    }

	  object->frame_filters = PyDict_New ();
	  if (!object->frame_filters)
	    {
	      Py_DECREF (object);
	      return NULL;
	    }

	  object->type_printers = PyList_New (0);
	  if (!object->type_printers)
	    {
	      Py_DECREF (object);
	      return NULL;
	    }

	  set_objfile_data (objfile, objfpy_objfile_data_key, object);
	}
    }

  return (PyObject *) object;
}

int
gdbpy_initialize_objfile (void)
{
  objfpy_objfile_data_key
    = register_objfile_data_with_cleanup (NULL, py_free_objfile);

  if (PyType_Ready (&objfile_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Objfile",
				 (PyObject *) &objfile_object_type);
}



static PyMethodDef objfile_object_methods[] =
{
  { "is_valid", objfpy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this object file is valid, false if not." },

  { NULL }
};

static PyGetSetDef objfile_getset[] =
{
  { "filename", objfpy_get_filename, NULL,
    "The objfile's filename, or None.", NULL },
  { "pretty_printers", objfpy_get_printers, objfpy_set_printers,
    "Pretty printers.", NULL },
  { "frame_filters", objfpy_get_frame_filters,
    objfpy_set_frame_filters, "Frame Filters.", NULL },
  { "type_printers", objfpy_get_type_printers, objfpy_set_type_printers,
    "Type printers.", NULL },
  { NULL }
};

static PyTypeObject objfile_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Objfile",		  /*tp_name*/
  sizeof (objfile_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  objfpy_dealloc,		  /*tp_dealloc*/
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
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB objfile object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  objfile_object_methods,	  /* tp_methods */
  0,				  /* tp_members */
  objfile_getset,		  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
  objfpy_new,			  /* tp_new */
};
