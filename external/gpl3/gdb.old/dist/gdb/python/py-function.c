/* Convenience functions implemented in Python.

   Copyright (C) 2008-2017 Free Software Foundation, Inc.

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
#include "value.h"
#include "python-internal.h"
#include "charset.h"
#include "gdbcmd.h"
#include "cli/cli-decode.h"
#include "completer.h"
#include "expression.h"
#include "language.h"
#include "py-ref.h"

extern PyTypeObject fnpy_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("PyObject");



static PyObject *
convert_values_to_python (int argc, struct value **argv)
{
  int i;
  gdbpy_ref<> result (PyTuple_New (argc));

  if (result == NULL)
    return NULL;

  for (i = 0; i < argc; ++i)
    {
      gdbpy_ref<> elt (value_to_value_object (argv[i]));
      if (elt == NULL)
	return NULL;
      PyTuple_SetItem (result.get (), i, elt.release ());
    }
  return result.release ();
}

/* Call a Python function object's invoke method.  */

static struct value *
fnpy_call (struct gdbarch *gdbarch, const struct language_defn *language,
	   void *cookie, int argc, struct value **argv)
{
  /* The gdbpy_enter object needs to be placed first, so that it's the last to
     be destroyed.  */
  gdbpy_enter enter_py (gdbarch, language);
  struct value *value;
  gdbpy_ref<> result;
  gdbpy_ref<> args (convert_values_to_python (argc, argv));

  /* convert_values_to_python can return NULL on error.  If we
     encounter this, do not call the function, but allow the Python ->
     error code conversion below to deal with the Python exception.
     Note, that this is different if the function simply does not
     have arguments.  */

  if (args != NULL)
    {
      gdbpy_ref<> callable (PyObject_GetAttrString ((PyObject *) cookie,
						    "invoke"));
      if (callable == NULL)
	error (_("No method named 'invoke' in object."));

      result.reset (PyObject_Call (callable.get (), args.get (), NULL));
    }

  if (result == NULL)
    {
      PyObject *ptype, *pvalue, *ptraceback;

      PyErr_Fetch (&ptype, &pvalue, &ptraceback);

      /* Try to fetch an error message contained within ptype, pvalue.
	 When fetching the error message we need to make our own copy,
	 we no longer own ptype, pvalue after the call to PyErr_Restore.  */

      gdb::unique_xmalloc_ptr<char>
	msg (gdbpy_exception_to_string (ptype, pvalue));

      if (msg == NULL)
	{
	  /* An error occurred computing the string representation of the
	     error message.  This is rare, but we should inform the user.  */

	  printf_filtered (_("An error occurred in a Python "
			     "convenience function\n"
			     "and then another occurred computing the "
			     "error message.\n"));
	  gdbpy_print_stack ();
	}

      /* Don't print the stack for gdb.GdbError exceptions.
	 It is generally used to flag user errors.

	 We also don't want to print "Error occurred in Python command"
	 for user errors.  However, a missing message for gdb.GdbError
	 exceptions is arguably a bug, so we flag it as such.  */

      if (!PyErr_GivenExceptionMatches (ptype, gdbpy_gdberror_exc)
	  || msg == NULL || *msg == '\0')
	{
	  PyErr_Restore (ptype, pvalue, ptraceback);
	  gdbpy_print_stack ();
	  if (msg != NULL && *msg != '\0')
	    error (_("Error occurred in Python convenience function: %s"),
		   msg.get ());
	  else
	    error (_("Error occurred in Python convenience function."));
	}
      else
	{
	  Py_XDECREF (ptype);
	  Py_XDECREF (pvalue);
	  Py_XDECREF (ptraceback);
	  error ("%s", msg.get ());
	}
    }

  value = convert_value_from_python (result.get ());
  if (value == NULL)
    {
      gdbpy_print_stack ();
      error (_("Error while executing Python code."));
    }

  return value;
}

/* Initializer for a Function object.  It takes one argument, the name
   of the function.  */

static int
fnpy_init (PyObject *self, PyObject *args, PyObject *kwds)
{
  const char *name;
  gdb::unique_xmalloc_ptr<char> docstring;

  if (! PyArg_ParseTuple (args, "s", &name))
    return -1;
  Py_INCREF (self);

  if (PyObject_HasAttrString (self, "__doc__"))
    {
      gdbpy_ref<> ds_obj (PyObject_GetAttrString (self, "__doc__"));
      if (ds_obj != NULL)
	{
	  if (gdbpy_is_string (ds_obj.get ()))
	    {
	      docstring = python_string_to_host_string (ds_obj.get ());
	      if (docstring == NULL)
		{
		  Py_DECREF (self);
		  return -1;
		}
	    }
	}
    }
  if (! docstring)
    docstring.reset (xstrdup (_("This function is not documented.")));

  add_internal_function (name, docstring.release (), fnpy_call, self);
  return 0;
}

/* Initialize internal function support.  */

int
gdbpy_initialize_functions (void)
{
  fnpy_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&fnpy_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Function",
				 (PyObject *) &fnpy_object_type);
}



PyTypeObject fnpy_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Function",		  /*tp_name*/
  sizeof (PyObject),		  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  0,				  /*tp_dealloc*/
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
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "GDB function object",	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  0,				  /* tp_methods */
  0,				  /* tp_members */
  0,				  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  fnpy_init,			  /* tp_init */
  0,				  /* tp_alloc */
};
