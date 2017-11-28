/* Support for debug methods in Python.

   Copyright (C) 2013-2016 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "extension-priv.h"
#include "objfiles.h"
#include "value.h"
#include "language.h"

#include "python.h"
#include "python-internal.h"

static const char enabled_field_name[] = "enabled";
static const char match_method_name[] = "match";
static const char get_arg_types_method_name[] = "get_arg_types";
static const char get_result_type_method_name[] = "get_result_type";
static const char matchers_attr_str[] = "xmethods";

static PyObject *py_match_method_name = NULL;
static PyObject *py_get_arg_types_method_name = NULL;

struct gdbpy_worker_data
{
  PyObject *worker;
  PyObject *this_type;
};

static struct xmethod_worker *new_python_xmethod_worker (PyObject *item,
							 PyObject *py_obj_type);

/* Implementation of free_xmethod_worker_data for Python.  */

void
gdbpy_free_xmethod_worker_data (const struct extension_language_defn *extlang,
				void *data)
{
  struct gdbpy_worker_data *worker_data = (struct gdbpy_worker_data *) data;
  struct cleanup *cleanups;

  gdb_assert (worker_data->worker != NULL && worker_data->this_type != NULL);

  /* We don't do much here, but we still need the GIL.  */
  cleanups = ensure_python_env (get_current_arch (), current_language);

  Py_DECREF (worker_data->worker);
  Py_DECREF (worker_data->this_type);
  xfree (worker_data);

  do_cleanups (cleanups);
}

/* Implementation of clone_xmethod_worker_data for Python.  */

void *
gdbpy_clone_xmethod_worker_data (const struct extension_language_defn *extlang,
				 void *data)
{
  struct gdbpy_worker_data *worker_data
    = (struct gdbpy_worker_data *) data, *new_data;
  struct cleanup *cleanups;

  gdb_assert (worker_data->worker != NULL && worker_data->this_type != NULL);

  /* We don't do much here, but we still need the GIL.  */
  cleanups = ensure_python_env (get_current_arch (), current_language);

  new_data = XCNEW (struct gdbpy_worker_data);
  new_data->worker = worker_data->worker;
  new_data->this_type = worker_data->this_type;
  Py_INCREF (new_data->worker);
  Py_INCREF (new_data->this_type);

  do_cleanups (cleanups);

  return new_data;
}

/* Invoke the "match" method of the MATCHER and return a new reference
   to the result.  Returns NULL on error.  */

static PyObject *
invoke_match_method (PyObject *matcher, PyObject *py_obj_type,
		     const char *xmethod_name)
{
  PyObject *py_xmethod_name;
  PyObject *match_method, *enabled_field, *match_result;
  struct cleanup *cleanups;
  int enabled;

  cleanups = make_cleanup (null_cleanup, NULL);

  enabled_field = PyObject_GetAttrString (matcher, enabled_field_name);
  if (enabled_field == NULL)
    {
      do_cleanups (cleanups);
      return NULL;
    }
  make_cleanup_py_decref (enabled_field);

  enabled = PyObject_IsTrue (enabled_field);
  if (enabled == -1)
    {
      do_cleanups (cleanups);
      return NULL;
    }
  if (enabled == 0)
    {
      /* Return 'None' if the matcher is not enabled.  */
      do_cleanups (cleanups);
      Py_RETURN_NONE;
    }

  match_method = PyObject_GetAttrString (matcher, match_method_name);
  if (match_method == NULL)
    {
      do_cleanups (cleanups);
      return NULL;
    }
  make_cleanup_py_decref (match_method);

  py_xmethod_name = PyString_FromString (xmethod_name);
  if (py_xmethod_name == NULL)
    {
      do_cleanups (cleanups);
      return NULL;
    }
  make_cleanup_py_decref (py_xmethod_name);

  match_result = PyObject_CallMethodObjArgs (matcher,
					     py_match_method_name,
					     py_obj_type,
					     py_xmethod_name,
					     NULL);

  do_cleanups (cleanups);

  return match_result;
}

/* Implementation of get_matching_xmethod_workers for Python.  */

enum ext_lang_rc
gdbpy_get_matching_xmethod_workers
  (const struct extension_language_defn *extlang,
   struct type *obj_type, const char *method_name,
   xmethod_worker_vec **dm_vec)
{
  struct cleanup *cleanups;
  struct objfile *objfile;
  VEC (xmethod_worker_ptr) *worker_vec = NULL;
  PyObject *py_type, *py_progspace;
  PyObject *py_xmethod_matcher_list = NULL, *list_iter, *matcher;

  gdb_assert (obj_type != NULL && method_name != NULL);

  cleanups = ensure_python_env (get_current_arch (), current_language);

  py_type = type_to_type_object (obj_type);
  if (py_type == NULL)
    {
      gdbpy_print_stack ();
      do_cleanups (cleanups);

      return EXT_LANG_RC_ERROR;
    }
  make_cleanup_py_decref (py_type);

  /* Create an empty list of debug methods.  */
  py_xmethod_matcher_list = PyList_New (0);
  if (py_xmethod_matcher_list == NULL)
    {
      gdbpy_print_stack ();
      do_cleanups (cleanups);

      return EXT_LANG_RC_ERROR;
    }

  /* Gather debug method matchers registered with the object files.
     This could be done differently by iterating over each objfile's matcher
     list individually, but there's no data yet to show it's needed.  */
  ALL_OBJFILES (objfile)
    {
      PyObject *py_objfile = objfile_to_objfile_object (objfile);
      PyObject *objfile_matchers, *temp = py_xmethod_matcher_list;

      if (py_objfile == NULL)
	{
	  gdbpy_print_stack ();
	  Py_DECREF (py_xmethod_matcher_list);
	  do_cleanups (cleanups);

	  return EXT_LANG_RC_ERROR;
	}

      objfile_matchers = objfpy_get_xmethods (py_objfile, NULL);
      py_xmethod_matcher_list = PySequence_Concat (temp, objfile_matchers);
      Py_DECREF (temp);
      Py_DECREF (objfile_matchers);
      if (py_xmethod_matcher_list == NULL)
	{
	  gdbpy_print_stack ();
	  do_cleanups (cleanups);

	  return EXT_LANG_RC_ERROR;
	}
    }

  /* Gather debug methods matchers registered with the current program
     space.  */
  py_progspace = pspace_to_pspace_object (current_program_space);
  if (py_progspace != NULL)
    {
      PyObject *temp = py_xmethod_matcher_list;
      PyObject *pspace_matchers = pspy_get_xmethods (py_progspace, NULL);

      py_xmethod_matcher_list = PySequence_Concat (temp, pspace_matchers);
      Py_DECREF (temp);
      Py_DECREF (pspace_matchers);
      if (py_xmethod_matcher_list == NULL)
	{
	  gdbpy_print_stack ();
	  do_cleanups (cleanups);

	  return EXT_LANG_RC_ERROR;
	}
    }
  else
    {
      gdbpy_print_stack ();
      Py_DECREF (py_xmethod_matcher_list);
      do_cleanups (cleanups);

      return EXT_LANG_RC_ERROR;
    }

  /* Gather debug method matchers registered globally.  */
  if (gdb_python_module != NULL
      && PyObject_HasAttrString (gdb_python_module, matchers_attr_str))
    {
      PyObject *gdb_matchers;
      PyObject *temp = py_xmethod_matcher_list;

      gdb_matchers = PyObject_GetAttrString (gdb_python_module,
					     matchers_attr_str);
      if (gdb_matchers != NULL)
	{
	  py_xmethod_matcher_list = PySequence_Concat (temp, gdb_matchers);
	  Py_DECREF (temp);
	  Py_DECREF (gdb_matchers);
	  if (py_xmethod_matcher_list == NULL)
	    {
	      gdbpy_print_stack ();
	      do_cleanups (cleanups);

	      return EXT_LANG_RC_ERROR;
	    }
	}
      else
	{
	  gdbpy_print_stack ();
	  Py_DECREF (py_xmethod_matcher_list);
	  do_cleanups (cleanups);

	  return EXT_LANG_RC_ERROR;
	}
    }

  /* Safe to make a cleanup for py_xmethod_matcher_list now as it
     will not change any more.  */
  make_cleanup_py_decref (py_xmethod_matcher_list);

  list_iter = PyObject_GetIter (py_xmethod_matcher_list);
  if (list_iter == NULL)
    {
      gdbpy_print_stack ();
      do_cleanups (cleanups);

      return EXT_LANG_RC_ERROR;
    }
  while ((matcher = PyIter_Next (list_iter)) != NULL)
    {
      PyObject *match_result = invoke_match_method (matcher, py_type,
						    method_name);

      if (match_result == NULL)
	{
	  gdbpy_print_stack ();
	  Py_DECREF (matcher);
	  do_cleanups (cleanups);

	  return EXT_LANG_RC_ERROR;
	}
      if (match_result == Py_None)
	; /* This means there was no match.  */
      else if (PySequence_Check (match_result))
	{
	  PyObject *iter = PyObject_GetIter (match_result);
	  PyObject *py_worker;

	  if (iter == NULL)
	    {
	      gdbpy_print_stack ();
	      Py_DECREF (matcher);
	      Py_DECREF (match_result);
	      do_cleanups (cleanups);

	      return EXT_LANG_RC_ERROR;
	    }
	  while ((py_worker = PyIter_Next (iter)) != NULL)
	    {
	      struct xmethod_worker *worker;

	      worker = new_python_xmethod_worker (py_worker, py_type);
	      VEC_safe_push (xmethod_worker_ptr, worker_vec, worker);
	      Py_DECREF (py_worker);
	    }
	  Py_DECREF (iter);
	  /* Report any error that could have occurred while iterating.  */
	  if (PyErr_Occurred ())
	    {
	      gdbpy_print_stack ();
	      Py_DECREF (matcher);
	      Py_DECREF (match_result);
	      do_cleanups (cleanups);

	      return EXT_LANG_RC_ERROR;
	    }
	}
      else
	{
	  struct xmethod_worker *worker;

	  worker = new_python_xmethod_worker (match_result, py_type);
	  VEC_safe_push (xmethod_worker_ptr, worker_vec, worker);
	}

      Py_DECREF (match_result);
      Py_DECREF (matcher);
    }
  Py_DECREF (list_iter);
  /* Report any error that could have occurred while iterating.  */
  if (PyErr_Occurred ())
    {
      gdbpy_print_stack ();
      do_cleanups (cleanups);

      return EXT_LANG_RC_ERROR;
    }

  do_cleanups (cleanups);
  *dm_vec = worker_vec;

  return EXT_LANG_RC_OK;
}

/* Implementation of get_xmethod_arg_types for Python.  */

enum ext_lang_rc
gdbpy_get_xmethod_arg_types (const struct extension_language_defn *extlang,
			     struct xmethod_worker *worker,
			     int *nargs, struct type ***arg_types)
{
  struct gdbpy_worker_data *worker_data
    = (struct gdbpy_worker_data *) worker->data;
  PyObject *py_worker = worker_data->worker;
  PyObject *get_arg_types_method;
  PyObject *py_argtype_list, *list_iter = NULL, *item;
  struct cleanup *cleanups;
  struct type **type_array, *obj_type;
  int i = 1, arg_count;

  /* Set nargs to -1 so that any premature return from this function returns
     an invalid/unusable number of arg types.  */
  *nargs = -1;

  cleanups = ensure_python_env (get_current_arch (), current_language);

  get_arg_types_method =  PyObject_GetAttrString (py_worker,
						  get_arg_types_method_name);
  if (get_arg_types_method == NULL)
    {
      gdbpy_print_stack ();
      do_cleanups (cleanups);

      return EXT_LANG_RC_ERROR;
    }
  make_cleanup_py_decref (get_arg_types_method);

  py_argtype_list = PyObject_CallMethodObjArgs (py_worker,
						py_get_arg_types_method_name,
						NULL);
  if (py_argtype_list == NULL)
    {
      gdbpy_print_stack ();
      do_cleanups (cleanups);

      return EXT_LANG_RC_ERROR;
    }
  make_cleanup_py_decref (py_argtype_list);
  if (py_argtype_list == Py_None)
    arg_count = 0;
  else if (PySequence_Check (py_argtype_list))
    {
      arg_count = PySequence_Size (py_argtype_list);
      if (arg_count == -1)
	{
	  gdbpy_print_stack ();
	  do_cleanups (cleanups);

	  return EXT_LANG_RC_ERROR;
	}

      list_iter = PyObject_GetIter (py_argtype_list);
      if (list_iter == NULL)
	{
	  gdbpy_print_stack ();
	  do_cleanups (cleanups);

	  return EXT_LANG_RC_ERROR;
	}
      make_cleanup_py_decref (list_iter);
    }
  else
    arg_count = 1;

  /* Include the 'this' argument in the size.  */
  type_array = XCNEWVEC (struct type *, arg_count + 1);
  i = 1;
  if (list_iter != NULL)
    {
      while ((item = PyIter_Next (list_iter)) != NULL)
	{
	  struct type *arg_type = type_object_to_type (item);

	  Py_DECREF (item);
	  if (arg_type == NULL)
	    {
	      PyErr_SetString (PyExc_TypeError,
			       _("Arg type returned by the get_arg_types "
				 "method of a debug method worker object is "
				 "not a gdb.Type object."));
	      break;
	    }

	  type_array[i] = arg_type;
	  i++;
	}
    }
  else if (arg_count == 1)
    {
      /* py_argtype_list is not actually a list but a single gdb.Type
	 object.  */
      struct type *arg_type = type_object_to_type (py_argtype_list);

      if (arg_type == NULL)
	{
	  PyErr_SetString (PyExc_TypeError,
			   _("Arg type returned by the get_arg_types method "
			     "of an xmethod worker object is not a gdb.Type "
			     "object."));
	}
      else
	{
	  type_array[i] = arg_type;
	  i++;
	}
    }
  if (PyErr_Occurred ())
    {
      gdbpy_print_stack ();
      do_cleanups (cleanups);
      xfree (type_array);

      return EXT_LANG_RC_ERROR;
    }

  /* Add the type of 'this' as the first argument.  The 'this' pointer should
     be a 'const' value.  Hence, create a 'const' variant of the 'this' pointer
     type.  */
  obj_type = type_object_to_type (worker_data->this_type);
  type_array[0] = make_cv_type (1, 0, lookup_pointer_type (obj_type), NULL);
  *nargs = i;
  *arg_types = type_array;
  do_cleanups (cleanups);

  return EXT_LANG_RC_OK;
}

/* Implementation of get_xmethod_result_type for Python.  */

enum ext_lang_rc
gdbpy_get_xmethod_result_type (const struct extension_language_defn *extlang,
			       struct xmethod_worker *worker,
			       struct value *obj,
			       struct value **args, int nargs,
			       struct type **result_type_ptr)
{
  struct gdbpy_worker_data *worker_data
    = (struct gdbpy_worker_data *) worker->data;
  PyObject *py_worker = worker_data->worker;
  PyObject *py_value_obj, *py_arg_tuple, *py_result_type;
  PyObject *get_result_type_method;
  struct type *obj_type, *this_type;
  struct cleanup *cleanups;
  int i;

  cleanups = ensure_python_env (get_current_arch (), current_language);

  /* First see if there is a get_result_type method.
     If not this could be an old xmethod (pre 7.9.1).  */
  get_result_type_method
    = PyObject_GetAttrString (py_worker, get_result_type_method_name);
  if (get_result_type_method == NULL)
    {
      PyErr_Clear ();
      do_cleanups (cleanups);
      *result_type_ptr = NULL;
      return EXT_LANG_RC_OK;
    }
  make_cleanup_py_decref (get_result_type_method);

  obj_type = check_typedef (value_type (obj));
  this_type = check_typedef (type_object_to_type (worker_data->this_type));
  if (TYPE_CODE (obj_type) == TYPE_CODE_PTR)
    {
      struct type *this_ptr = lookup_pointer_type (this_type);

      if (!types_equal (obj_type, this_ptr))
	obj = value_cast (this_ptr, obj);
    }
  else if (TYPE_CODE (obj_type) == TYPE_CODE_REF)
    {
      struct type *this_ref = lookup_reference_type (this_type);

      if (!types_equal (obj_type, this_ref))
	obj = value_cast (this_ref, obj);
    }
  else
    {
      if (!types_equal (obj_type, this_type))
	obj = value_cast (this_type, obj);
    }
  py_value_obj = value_to_value_object (obj);
  if (py_value_obj == NULL)
    goto Fail;
  make_cleanup_py_decref (py_value_obj);

  py_arg_tuple = PyTuple_New (nargs + 1);
  if (py_arg_tuple == NULL)
    goto Fail;
  make_cleanup_py_decref (py_arg_tuple);

  /* PyTuple_SET_ITEM steals the reference of the element.  Hence INCREF the
     reference to the 'this' object as we have a cleanup to DECREF it.  */
  Py_INCREF (py_value_obj);
  PyTuple_SET_ITEM (py_arg_tuple, 0, py_value_obj);

  for (i = 0; i < nargs; i++)
    {
      PyObject *py_value_arg = value_to_value_object (args[i]);

      if (py_value_arg == NULL)
	goto Fail;
      PyTuple_SET_ITEM (py_arg_tuple, i + 1, py_value_arg);
    }

  py_result_type = PyObject_CallObject (get_result_type_method, py_arg_tuple);
  if (py_result_type == NULL)
    goto Fail;
  make_cleanup_py_decref (py_result_type);

  *result_type_ptr = type_object_to_type (py_result_type);
  if (*result_type_ptr == NULL)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Type returned by the get_result_type method of an"
			 " xmethod worker object is not a gdb.Type object."));
      goto Fail;
    }

  do_cleanups (cleanups);
  return EXT_LANG_RC_OK;

 Fail:
  gdbpy_print_stack ();
  do_cleanups (cleanups);
  return EXT_LANG_RC_ERROR;
}

/* Implementation of invoke_xmethod for Python.  */

struct value *
gdbpy_invoke_xmethod (const struct extension_language_defn *extlang,
		      struct xmethod_worker *worker,
		      struct value *obj, struct value **args, int nargs)
{
  int i;
  struct cleanup *cleanups;
  PyObject *py_value_obj, *py_arg_tuple, *py_result;
  struct type *obj_type, *this_type;
  struct value *res = NULL;
  struct gdbpy_worker_data *worker_data
    = (struct gdbpy_worker_data *) worker->data;
  PyObject *xmethod_worker = worker_data->worker;

  cleanups = ensure_python_env (get_current_arch (), current_language);

  obj_type = check_typedef (value_type (obj));
  this_type = check_typedef (type_object_to_type (worker_data->this_type));
  if (TYPE_CODE (obj_type) == TYPE_CODE_PTR)
    {
      struct type *this_ptr = lookup_pointer_type (this_type);

      if (!types_equal (obj_type, this_ptr))
	obj = value_cast (this_ptr, obj);
    }
  else if (TYPE_CODE (obj_type) == TYPE_CODE_REF)
    {
      struct type *this_ref = lookup_reference_type (this_type);

      if (!types_equal (obj_type, this_ref))
	obj = value_cast (this_ref, obj);
    }
  else
    {
      if (!types_equal (obj_type, this_type))
	obj = value_cast (this_type, obj);
    }
  py_value_obj = value_to_value_object (obj);
  if (py_value_obj == NULL)
    {
      gdbpy_print_stack ();
      error (_("Error while executing Python code."));
    }
  make_cleanup_py_decref (py_value_obj);

  py_arg_tuple = PyTuple_New (nargs + 1);
  if (py_arg_tuple == NULL)
    {
      gdbpy_print_stack ();
      error (_("Error while executing Python code."));
    }
  make_cleanup_py_decref (py_arg_tuple);

  /* PyTuple_SET_ITEM steals the reference of the element.  Hence INCREF the
     reference to the 'this' object as we have a cleanup to DECREF it.  */
  Py_INCREF (py_value_obj);
  PyTuple_SET_ITEM (py_arg_tuple, 0, py_value_obj);

  for (i = 0; i < nargs; i++)
    {
      PyObject *py_value_arg = value_to_value_object (args[i]);

      if (py_value_arg == NULL)
	{
	  gdbpy_print_stack ();
	  error (_("Error while executing Python code."));
	}

      PyTuple_SET_ITEM (py_arg_tuple, i + 1, py_value_arg);
    }

  py_result = PyObject_CallObject (xmethod_worker, py_arg_tuple);
  if (py_result == NULL)
    {
      gdbpy_print_stack ();
      error (_("Error while executing Python code."));
    }
  make_cleanup_py_decref (py_result);

  if (py_result != Py_None)
    {
      res = convert_value_from_python (py_result);
      if (res == NULL)
	{
	  gdbpy_print_stack ();
	  error (_("Error while executing Python code."));
	}
    }
  else
    {
      res = allocate_value (lookup_typename (python_language, python_gdbarch,
					     "void", NULL, 0));
    }

  do_cleanups (cleanups);

  return res;
}

/* Creates a new Python xmethod_worker object.
   The new object has data of type 'struct gdbpy_worker_data' composed
   with the components PY_WORKER and THIS_TYPE.  */

static struct xmethod_worker *
new_python_xmethod_worker (PyObject *py_worker, PyObject *this_type)
{
  struct gdbpy_worker_data *data;

  gdb_assert (py_worker != NULL && this_type != NULL);

  data = XCNEW (struct gdbpy_worker_data);
  data->worker = py_worker;
  data->this_type = this_type;
  Py_INCREF (py_worker);
  Py_INCREF (this_type);

  return new_xmethod_worker (&extension_language_python, data);
}

int
gdbpy_initialize_xmethods (void)
{
  py_match_method_name = PyString_FromString (match_method_name);
  if (py_match_method_name == NULL)
    return -1;

  py_get_arg_types_method_name
    = PyString_FromString (get_arg_types_method_name);
  if (py_get_arg_types_method_name == NULL)
    return -1;

  return 1;
}
