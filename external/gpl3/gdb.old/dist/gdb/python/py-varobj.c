/* Copyright (C) 2013-2016 Free Software Foundation, Inc.

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
#include "varobj.h"
#include "varobj-iter.h"

/* A dynamic varobj iterator "class" for python pretty-printed
   varobjs.  This inherits struct varobj_iter.  */

struct py_varobj_iter
{
  /* The 'base class'.  */
  struct varobj_iter base;

  /* The python iterator returned by the printer's 'children' method,
     or NULL if not available.  */
  PyObject *iter;
};

/* Implementation of the 'dtor' method of pretty-printed varobj
   iterators.  */

static void
py_varobj_iter_dtor (struct varobj_iter *self)
{
  struct py_varobj_iter *dis = (struct py_varobj_iter *) self;
  struct cleanup *back_to = varobj_ensure_python_env (self->var);

  Py_XDECREF (dis->iter);

  do_cleanups (back_to);
}

/* Implementation of the 'next' method of pretty-printed varobj
   iterators.  */

static varobj_item *
py_varobj_iter_next (struct varobj_iter *self)
{
  struct py_varobj_iter *t = (struct py_varobj_iter *) self;
  struct cleanup *back_to;
  PyObject *item;
  PyObject *py_v;
  varobj_item *vitem;
  const char *name = NULL;

  if (!gdb_python_initialized)
    return NULL;

  back_to = varobj_ensure_python_env (self->var);

  item = PyIter_Next (t->iter);

  if (item == NULL)
    {
      /* Normal end of iteration.  */
      if (!PyErr_Occurred ())
	return NULL;

      /* If we got a memory error, just use the text as the item.  */
      if (PyErr_ExceptionMatches (gdbpy_gdb_memory_error))
	{
	  PyObject *type, *value, *trace;
	  char *name_str, *value_str;

	  PyErr_Fetch (&type, &value, &trace);
	  value_str = gdbpy_exception_to_string (type, value);
	  Py_XDECREF (type);
	  Py_XDECREF (value);
	  Py_XDECREF (trace);
	  if (value_str == NULL)
	    {
	      gdbpy_print_stack ();
	      return NULL;
	    }

	  name_str = xstrprintf ("<error at %d>",
				 self->next_raw_index++);
	  item = Py_BuildValue ("(ss)", name_str, value_str);
	  xfree (name_str);
	  xfree (value_str);
	  if (item == NULL)
	    {
	      gdbpy_print_stack ();
	      return NULL;
	    }
	}
      else
	{
	  /* Any other kind of error.  */
	  gdbpy_print_stack ();
	  return NULL;
	}
    }

  if (!PyArg_ParseTuple (item, "sO", &name, &py_v))
    {
      gdbpy_print_stack ();
      error (_("Invalid item from the child list"));
    }

  vitem = XNEW (struct varobj_item);
  vitem->value = convert_value_from_python (py_v);
  if (vitem->value == NULL)
    gdbpy_print_stack ();
  vitem->name = xstrdup (name);

  self->next_raw_index++;
  do_cleanups (back_to);
  return vitem;
}

/* The 'vtable' of pretty-printed python varobj iterators.  */

static const struct varobj_iter_ops py_varobj_iter_ops =
{
  py_varobj_iter_dtor,
  py_varobj_iter_next
};

/* Constructor of pretty-printed varobj iterators.  VAR is the varobj
   whose children the iterator will be iterating over.  PYITER is the
   python iterator actually responsible for the iteration.  */

static void CPYCHECKER_STEALS_REFERENCE_TO_ARG (3)
py_varobj_iter_ctor (struct py_varobj_iter *self,
		      struct varobj *var, PyObject *pyiter)
{
  self->base.var = var;
  self->base.ops = &py_varobj_iter_ops;
  self->base.next_raw_index = 0;
  self->iter = pyiter;
}

/* Allocate and construct a pretty-printed varobj iterator.  VAR is
   the varobj whose children the iterator will be iterating over.
   PYITER is the python iterator actually responsible for the
   iteration.  */

static struct py_varobj_iter * CPYCHECKER_STEALS_REFERENCE_TO_ARG (2)
py_varobj_iter_new (struct varobj *var, PyObject *pyiter)
{
  struct py_varobj_iter *self;

  self = XNEW (struct py_varobj_iter);
  py_varobj_iter_ctor (self, var, pyiter);
  return self;
}

/* Return a new pretty-printed varobj iterator suitable to iterate
   over VAR's children.  */

struct varobj_iter *
py_varobj_get_iterator (struct varobj *var, PyObject *printer)
{
  PyObject *children;
  PyObject *iter;
  struct py_varobj_iter *py_iter;
  struct cleanup *back_to = varobj_ensure_python_env (var);

  if (!PyObject_HasAttr (printer, gdbpy_children_cst))
    {
      do_cleanups (back_to);
      return NULL;
    }

  children = PyObject_CallMethodObjArgs (printer, gdbpy_children_cst,
					 NULL);
  if (children == NULL)
    {
      gdbpy_print_stack ();
      error (_("Null value returned for children"));
    }

  make_cleanup_py_decref (children);

  iter = PyObject_GetIter (children);
  if (iter == NULL)
    {
      gdbpy_print_stack ();
      error (_("Could not get children iterator"));
    }

  py_iter = py_varobj_iter_new (var, iter);

  do_cleanups (back_to);

  return &py_iter->base;
}
