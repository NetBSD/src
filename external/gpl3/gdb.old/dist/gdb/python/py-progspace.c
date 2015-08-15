/* Python interface to program spaces.

   Copyright (C) 2010-2014 Free Software Foundation, Inc.

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
#include "progspace.h"
#include "objfiles.h"
#include "language.h"
#include "arch-utils.h"

typedef struct
{
  PyObject_HEAD

  /* The corresponding pspace.  */
  struct program_space *pspace;

  /* The pretty-printer list of functions.  */
  PyObject *printers;

  /* The frame filter list of functions.  */
  PyObject *frame_filters;
  /* The type-printer list.  */
  PyObject *type_printers;
} pspace_object;

static PyTypeObject pspace_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("pspace_object");

static const struct program_space_data *pspy_pspace_data_key;



/* An Objfile method which returns the objfile's file name, or None.  */

static PyObject *
pspy_get_filename (PyObject *self, void *closure)
{
  pspace_object *obj = (pspace_object *) self;

  if (obj->pspace)
    {
      struct objfile *objfile = obj->pspace->symfile_object_file;

      if (objfile)
	return PyString_Decode (objfile_name (objfile),
				strlen (objfile_name (objfile)),
				host_charset (), NULL);
    }
  Py_RETURN_NONE;
}

static void
pspy_dealloc (PyObject *self)
{
  pspace_object *ps_self = (pspace_object *) self;

  Py_XDECREF (ps_self->printers);
  Py_XDECREF (ps_self->frame_filters);
  Py_XDECREF (ps_self->type_printers);
  Py_TYPE (self)->tp_free (self);
}

static PyObject *
pspy_new (PyTypeObject *type, PyObject *args, PyObject *keywords)
{
  pspace_object *self = (pspace_object *) type->tp_alloc (type, 0);

  if (self)
    {
      self->pspace = NULL;

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
pspy_get_printers (PyObject *o, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  Py_INCREF (self->printers);
  return self->printers;
}

static int
pspy_set_printers (PyObject *o, PyObject *value, void *ignore)
{
  PyObject *tmp;
  pspace_object *self = (pspace_object *) o;

  if (! value)
    {
      PyErr_SetString (PyExc_TypeError,
		       "cannot delete the pretty_printers attribute");
      return -1;
    }

  if (! PyList_Check (value))
    {
      PyErr_SetString (PyExc_TypeError,
		       "the pretty_printers attribute must be a list");
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
   this program space.  */
PyObject *
pspy_get_frame_filters (PyObject *o, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  Py_INCREF (self->frame_filters);
  return self->frame_filters;
}

/* Set this object file's frame filters dictionary to FILTERS.  */
static int
pspy_set_frame_filters (PyObject *o, PyObject *frame, void *ignore)
{
  PyObject *tmp;
  pspace_object *self = (pspace_object *) o;

  if (! frame)
    {
      PyErr_SetString (PyExc_TypeError,
		       "cannot delete the frame filter attribute");
      return -1;
    }

  if (! PyDict_Check (frame))
    {
      PyErr_SetString (PyExc_TypeError,
		       "the frame filter attribute must be a dictionary");
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  tmp = self->frame_filters;
  Py_INCREF (frame);
  self->frame_filters = frame;
  Py_XDECREF (tmp);

  return 0;
}

/* Get the 'type_printers' attribute.  */

static PyObject *
pspy_get_type_printers (PyObject *o, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  Py_INCREF (self->type_printers);
  return self->type_printers;
}

/* Set the 'type_printers' attribute.  */

static int
pspy_set_type_printers (PyObject *o, PyObject *value, void *ignore)
{
  PyObject *tmp;
  pspace_object *self = (pspace_object *) o;

  if (! value)
    {
      PyErr_SetString (PyExc_TypeError,
		       "cannot delete the type_printers attribute");
      return -1;
    }

  if (! PyList_Check (value))
    {
      PyErr_SetString (PyExc_TypeError,
		       "the type_printers attribute must be a list");
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  tmp = self->type_printers;
  Py_INCREF (value);
  self->type_printers = value;
  Py_XDECREF (tmp);

  return 0;
}



/* Clear the PSPACE pointer in a Pspace object and remove the reference.  */

static void
py_free_pspace (struct program_space *pspace, void *datum)
{
  struct cleanup *cleanup;
  pspace_object *object = datum;
  struct gdbarch *arch = get_current_arch ();

  cleanup = ensure_python_env (arch, current_language);
  object->pspace = NULL;
  Py_DECREF ((PyObject *) object);
  do_cleanups (cleanup);
}

/* Return a borrowed reference to the Python object of type Pspace
   representing PSPACE.  If the object has already been created,
   return it.  Otherwise, create it.  Return NULL and set the Python
   error on failure.  */

PyObject *
pspace_to_pspace_object (struct program_space *pspace)
{
  pspace_object *object;

  object = program_space_data (pspace, pspy_pspace_data_key);
  if (!object)
    {
      object = PyObject_New (pspace_object, &pspace_object_type);
      if (object)
	{
	  object->pspace = pspace;

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

	  set_program_space_data (pspace, pspy_pspace_data_key, object);
	}
    }

  return (PyObject *) object;
}

int
gdbpy_initialize_pspace (void)
{
  pspy_pspace_data_key
    = register_program_space_data_with_cleanup (NULL, py_free_pspace);

  if (PyType_Ready (&pspace_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Progspace",
				 (PyObject *) &pspace_object_type);
}



static PyGetSetDef pspace_getset[] =
{
  { "filename", pspy_get_filename, NULL,
    "The progspace's main filename, or None.", NULL },
  { "pretty_printers", pspy_get_printers, pspy_set_printers,
    "Pretty printers.", NULL },
  { "frame_filters", pspy_get_frame_filters, pspy_set_frame_filters,
    "Frame filters.", NULL },
  { "type_printers", pspy_get_type_printers, pspy_set_type_printers,
    "Type printers.", NULL },
  { NULL }
};

static PyTypeObject pspace_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Progspace",		  /*tp_name*/
  sizeof (pspace_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  pspy_dealloc,			  /*tp_dealloc*/
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
  "GDB progspace object",	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  0,				  /* tp_methods */
  0,				  /* tp_members */
  pspace_getset,		  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
  pspy_new,			  /* tp_new */
};
