/* Python pretty-printing

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
#include "exceptions.h"
#include "objfiles.h"
#include "symtab.h"
#include "language.h"
#include "valprint.h"

#include "python.h"

#ifdef HAVE_PYTHON
#include "python-internal.h"

/* Return type of print_string_repr.  */

enum string_repr_result
  {
    /* The string method returned None.  */
    string_repr_none,
    /* The string method had an error.  */
    string_repr_error,
    /* Everything ok.  */
    string_repr_ok
  };

/* Helper function for find_pretty_printer which iterates over a list,
   calls each function and inspects output.  This will return a
   printer object if one recognizes VALUE.  If no printer is found, it
   will return None.  On error, it will set the Python error and
   return NULL.  */

static PyObject *
search_pp_list (PyObject *list, PyObject *value)
{
  Py_ssize_t pp_list_size, list_index;
  PyObject *function, *printer = NULL;

  pp_list_size = PyList_Size (list);
  for (list_index = 0; list_index < pp_list_size; list_index++)
    {
      function = PyList_GetItem (list, list_index);
      if (! function)
	return NULL;

      /* Skip if disabled.  */
      if (PyObject_HasAttr (function, gdbpy_enabled_cst))
	{
	  PyObject *attr = PyObject_GetAttr (function, gdbpy_enabled_cst);
	  int cmp;

	  if (!attr)
	    return NULL;
	  cmp = PyObject_IsTrue (attr);
	  Py_DECREF (attr);
	  if (cmp == -1)
	    return NULL;

	  if (!cmp)
	    continue;
	}

      printer = PyObject_CallFunctionObjArgs (function, value, NULL);
      if (! printer)
	return NULL;
      else if (printer != Py_None)
	return printer;

      Py_DECREF (printer);
    }

  Py_RETURN_NONE;
}

/* Subroutine of find_pretty_printer to simplify it.
   Look for a pretty-printer to print VALUE in all objfiles.
   The result is NULL if there's an error and the search should be terminated.
   The result is Py_None, suitably inc-ref'd, if no pretty-printer was found.
   Otherwise the result is the pretty-printer function, suitably inc-ref'd.  */

static PyObject *
find_pretty_printer_from_objfiles (PyObject *value)
{
  PyObject *pp_list;
  PyObject *function;
  struct objfile *obj;

  ALL_OBJFILES (obj)
  {
    PyObject *objf = objfile_to_objfile_object (obj);
    if (!objf)
      {
	/* Ignore the error and continue.  */
	PyErr_Clear ();
	continue;
      }

    pp_list = objfpy_get_printers (objf, NULL);
    function = search_pp_list (pp_list, value);
    Py_XDECREF (pp_list);

    /* If there is an error in any objfile list, abort the search and exit.  */
    if (! function)
      return NULL;

    if (function != Py_None)
      return function;

    Py_DECREF (function);
  }

  Py_RETURN_NONE;
}

/* Subroutine of find_pretty_printer to simplify it.
   Look for a pretty-printer to print VALUE in the current program space.
   The result is NULL if there's an error and the search should be terminated.
   The result is Py_None, suitably inc-ref'd, if no pretty-printer was found.
   Otherwise the result is the pretty-printer function, suitably inc-ref'd.  */

static PyObject *
find_pretty_printer_from_progspace (PyObject *value)
{
  PyObject *pp_list;
  PyObject *function;
  PyObject *obj = pspace_to_pspace_object (current_program_space);

  if (!obj)
    return NULL;
  pp_list = pspy_get_printers (obj, NULL);
  function = search_pp_list (pp_list, value);
  Py_XDECREF (pp_list);
  return function;
}

/* Subroutine of find_pretty_printer to simplify it.
   Look for a pretty-printer to print VALUE in the gdb module.
   The result is NULL if there's an error and the search should be terminated.
   The result is Py_None, suitably inc-ref'd, if no pretty-printer was found.
   Otherwise the result is the pretty-printer function, suitably inc-ref'd.  */

static PyObject *
find_pretty_printer_from_gdb (PyObject *value)
{
  PyObject *pp_list;
  PyObject *function;

  /* Fetch the global pretty printer list.  */
  if (gdb_python_module == NULL
      || ! PyObject_HasAttrString (gdb_python_module, "pretty_printers"))
    Py_RETURN_NONE;
  pp_list = PyObject_GetAttrString (gdb_python_module, "pretty_printers");
  if (pp_list == NULL || ! PyList_Check (pp_list))
    {
      Py_XDECREF (pp_list);
      Py_RETURN_NONE;
    }

  function = search_pp_list (pp_list, value);
  Py_XDECREF (pp_list);
  return function;
}

/* Find the pretty-printing constructor function for VALUE.  If no
   pretty-printer exists, return None.  If one exists, return a new
   reference.  On error, set the Python error and return NULL.  */

static PyObject *
find_pretty_printer (PyObject *value)
{
  PyObject *function;

  /* Look at the pretty-printer list for each objfile
     in the current program-space.  */
  function = find_pretty_printer_from_objfiles (value);
  if (function == NULL || function != Py_None)
    return function;
  Py_DECREF (function);

  /* Look at the pretty-printer list for the current program-space.  */
  function = find_pretty_printer_from_progspace (value);
  if (function == NULL || function != Py_None)
    return function;
  Py_DECREF (function);

  /* Look at the pretty-printer list in the gdb module.  */
  function = find_pretty_printer_from_gdb (value);
  return function;
}

/* Pretty-print a single value, via the printer object PRINTER.
   If the function returns a string, a PyObject containing the string
   is returned.  If the function returns Py_NONE that means the pretty
   printer returned the Python None as a value.  Otherwise, if the
   function returns a value,  *OUT_VALUE is set to the value, and NULL
   is returned.  On error, *OUT_VALUE is set to NULL, NULL is
   returned, with a python exception set.  */

static PyObject *
pretty_print_one_value (PyObject *printer, struct value **out_value)
{
  volatile struct gdb_exception except;
  PyObject *result = NULL;

  *out_value = NULL;
  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      result = PyObject_CallMethodObjArgs (printer, gdbpy_to_string_cst, NULL);
      if (result)
	{
	  if (! gdbpy_is_string (result) && ! gdbpy_is_lazy_string (result)
	      && result != Py_None)
	    {
	      *out_value = convert_value_from_python (result);
	      if (PyErr_Occurred ())
		*out_value = NULL;
	      Py_DECREF (result);
	      result = NULL;
	    }
	}
    }

  return result;
}

/* Return the display hint for the object printer, PRINTER.  Return
   NULL if there is no display_hint method, or if the method did not
   return a string.  On error, print stack trace and return NULL.  On
   success, return an xmalloc()d string.  */
char *
gdbpy_get_display_hint (PyObject *printer)
{
  PyObject *hint;
  char *result = NULL;

  if (! PyObject_HasAttr (printer, gdbpy_display_hint_cst))
    return NULL;

  hint = PyObject_CallMethodObjArgs (printer, gdbpy_display_hint_cst, NULL);
  if (hint)
    {
      if (gdbpy_is_string (hint))
	{
	  result = python_string_to_host_string (hint);
	  if (result == NULL)
	    gdbpy_print_stack ();
	}
      Py_DECREF (hint);
    }
  else
    gdbpy_print_stack ();

  return result;
}

/* A wrapper for gdbpy_print_stack that ignores MemoryError.  */

static void
print_stack_unless_memory_error (struct ui_file *stream)
{
  if (PyErr_ExceptionMatches (gdbpy_gdb_memory_error))
    {
      struct cleanup *cleanup;
      PyObject *type, *value, *trace;
      char *msg;

      PyErr_Fetch (&type, &value, &trace);
      cleanup = make_cleanup_py_decref (type);
      make_cleanup_py_decref (value);
      make_cleanup_py_decref (trace);

      msg = gdbpy_exception_to_string (type, value);
      make_cleanup (xfree, msg);

      if (msg == NULL || *msg == '\0')
	fprintf_filtered (stream, _("<error reading variable>"));
      else
	fprintf_filtered (stream, _("<error reading variable: %s>"), msg);

      do_cleanups (cleanup);
    }
  else
    gdbpy_print_stack ();
}

/* Helper for apply_val_pretty_printer which calls to_string and
   formats the result.  */

static enum string_repr_result
print_string_repr (PyObject *printer, const char *hint,
		   struct ui_file *stream, int recurse,
		   const struct value_print_options *options,
		   const struct language_defn *language,
		   struct gdbarch *gdbarch)
{
  struct value *replacement = NULL;
  PyObject *py_str = NULL;
  enum string_repr_result result = string_repr_ok;

  py_str = pretty_print_one_value (printer, &replacement);
  if (py_str)
    {
      struct cleanup *cleanup = make_cleanup_py_decref (py_str);

      if (py_str == Py_None)
	result = string_repr_none;
      else if (gdbpy_is_lazy_string (py_str))
	{
	  CORE_ADDR addr;
	  long length;
	  struct type *type;
	  char *encoding = NULL;
	  struct value_print_options local_opts = *options;

	  make_cleanup (free_current_contents, &encoding);
	  gdbpy_extract_lazy_string (py_str, &addr, &type,
				     &length, &encoding);

	  local_opts.addressprint = 0;
	  val_print_string (type, encoding, addr, (int) length,
			    stream, &local_opts);
	}
      else
	{
	  PyObject *string;

	  string = python_string_to_target_python_string (py_str);
	  if (string)
	    {
	      char *output;
	      long length;
	      struct type *type;

	      make_cleanup_py_decref (string);
#ifdef IS_PY3K
	      output = PyBytes_AS_STRING (string);
	      length = PyBytes_GET_SIZE (string);
#else
	      output = PyString_AsString (string);
	      length = PyString_Size (string);
#endif
	      type = builtin_type (gdbarch)->builtin_char;

	      if (hint && !strcmp (hint, "string"))
		LA_PRINT_STRING (stream, type, (gdb_byte *) output,
				 length, NULL, 0, options);
	      else
		fputs_filtered (output, stream);
	    }
	  else
	    {
	      result = string_repr_error;
	      print_stack_unless_memory_error (stream);
	    }
	}

      do_cleanups (cleanup);
    }
  else if (replacement)
    {
      struct value_print_options opts = *options;

      opts.addressprint = 0;
      common_val_print (replacement, stream, recurse, &opts, language);
    }
  else
    {
      result = string_repr_error;
      print_stack_unless_memory_error (stream);
    }

  return result;
}

#ifndef IS_PY3K
static void
py_restore_tstate (void *p)
{
  PyFrameObject *frame = p;
  PyThreadState *tstate = PyThreadState_GET ();

  tstate->frame = frame;
}

/* Create a dummy PyFrameObject, needed to work around
   a Python-2.4 bug with generators.  */
static PyObject *
push_dummy_python_frame (void)
{
  PyObject *empty_string, *null_tuple, *globals;
  PyCodeObject *code;
  PyFrameObject *frame;
  PyThreadState *tstate;

  empty_string = PyString_FromString ("");
  if (!empty_string)
    return NULL;

  null_tuple = PyTuple_New (0);
  if (!null_tuple)
    {
      Py_DECREF (empty_string);
      return NULL;
    }

  code = PyCode_New (0,			/* argcount */
		     0,			/* nlocals */
		     0,			/* stacksize */
		     0,			/* flags */
		     empty_string,	/* code */
		     null_tuple,	/* consts */
		     null_tuple,	/* names */
		     null_tuple,	/* varnames */
#if PYTHON_API_VERSION >= 1010
		     null_tuple,	/* freevars */
		     null_tuple,	/* cellvars */
#endif
		     empty_string,	/* filename */
		     empty_string,	/* name */
		     1,			/* firstlineno */
		     empty_string	/* lnotab */
		    );

  Py_DECREF (empty_string);
  Py_DECREF (null_tuple);

  if (!code)
    return NULL;

  globals = PyDict_New ();
  if (!globals)
    {
      Py_DECREF (code);
      return NULL;
    }

  tstate = PyThreadState_GET ();

  frame = PyFrame_New (tstate, code, globals, NULL);

  Py_DECREF (globals);
  Py_DECREF (code);

  if (!frame)
    return NULL;

  tstate->frame = frame;
  make_cleanup (py_restore_tstate, frame->f_back);
  return (PyObject *) frame;
}
#endif

/* Helper for apply_val_pretty_printer that formats children of the
   printer, if any exist.  If is_py_none is true, then nothing has
   been printed by to_string, and format output accordingly. */
static void
print_children (PyObject *printer, const char *hint,
		struct ui_file *stream, int recurse,
		const struct value_print_options *options,
		const struct language_defn *language,
		int is_py_none)
{
  int is_map, is_array, done_flag, pretty;
  unsigned int i;
  PyObject *children, *iter;
#ifndef IS_PY3K
  PyObject *frame;
#endif
  struct cleanup *cleanups;

  if (! PyObject_HasAttr (printer, gdbpy_children_cst))
    return;

  /* If we are printing a map or an array, we want some special
     formatting.  */
  is_map = hint && ! strcmp (hint, "map");
  is_array = hint && ! strcmp (hint, "array");

  children = PyObject_CallMethodObjArgs (printer, gdbpy_children_cst,
					 NULL);
  if (! children)
    {
      print_stack_unless_memory_error (stream);
      return;
    }

  cleanups = make_cleanup_py_decref (children);

  iter = PyObject_GetIter (children);
  if (!iter)
    {
      print_stack_unless_memory_error (stream);
      goto done;
    }
  make_cleanup_py_decref (iter);

  /* Use the prettyformat_arrays option if we are printing an array,
     and the pretty option otherwise.  */
  if (is_array)
    pretty = options->prettyformat_arrays;
  else
    {
      if (options->prettyformat == Val_prettyformat)
	pretty = 1;
      else
	pretty = options->prettyformat_structs;
    }

  /* Manufacture a dummy Python frame to work around Python 2.4 bug,
     where it insists on having a non-NULL tstate->frame when
     a generator is called.  */
#ifndef IS_PY3K
  frame = push_dummy_python_frame ();
  if (!frame)
    {
      gdbpy_print_stack ();
      goto done;
    }
  make_cleanup_py_decref (frame);
#endif

  done_flag = 0;
  for (i = 0; i < options->print_max; ++i)
    {
      PyObject *py_v, *item = PyIter_Next (iter);
      const char *name;
      struct cleanup *inner_cleanup;

      if (! item)
	{
	  if (PyErr_Occurred ())
	    print_stack_unless_memory_error (stream);
	  /* Set a flag so we can know whether we printed all the
	     available elements.  */
	  else	
	    done_flag = 1;
	  break;
	}

      if (! PyArg_ParseTuple (item, "sO", &name, &py_v))
	{
	  gdbpy_print_stack ();
	  Py_DECREF (item);
	  continue;
	}
      inner_cleanup = make_cleanup_py_decref (item);

      /* Print initial "{".  For other elements, there are three
	 cases:
	 1. Maps.  Print a "," after each value element.
	 2. Arrays.  Always print a ",".
	 3. Other.  Always print a ",".  */
      if (i == 0)
	{
         if (is_py_none)
           fputs_filtered ("{", stream);
         else
           fputs_filtered (" = {", stream);
       }

      else if (! is_map || i % 2 == 0)
	fputs_filtered (pretty ? "," : ", ", stream);

      /* In summary mode, we just want to print "= {...}" if there is
	 a value.  */
      if (options->summary)
	{
	  /* This increment tricks the post-loop logic to print what
	     we want.  */
	  ++i;
	  /* Likewise.  */
	  pretty = 0;
	  break;
	}

      if (! is_map || i % 2 == 0)
	{
	  if (pretty)
	    {
	      fputs_filtered ("\n", stream);
	      print_spaces_filtered (2 + 2 * recurse, stream);
	    }
	  else
	    wrap_here (n_spaces (2 + 2 *recurse));
	}

      if (is_map && i % 2 == 0)
	fputs_filtered ("[", stream);
      else if (is_array)
	{
	  /* We print the index, not whatever the child method
	     returned as the name.  */
	  if (options->print_array_indexes)
	    fprintf_filtered (stream, "[%d] = ", i);
	}
      else if (! is_map)
	{
	  fputs_filtered (name, stream);
	  fputs_filtered (" = ", stream);
	}

      if (gdbpy_is_lazy_string (py_v))
	{
	  CORE_ADDR addr;
	  struct type *type;
	  long length;
	  char *encoding = NULL;
	  struct value_print_options local_opts = *options;

	  make_cleanup (free_current_contents, &encoding);
	  gdbpy_extract_lazy_string (py_v, &addr, &type, &length, &encoding);

	  local_opts.addressprint = 0;
	  val_print_string (type, encoding, addr, (int) length, stream,
			    &local_opts);
	}
      else if (gdbpy_is_string (py_v))
	{
	  char *output;

	  output = python_string_to_host_string (py_v);
	  if (!output)
	    gdbpy_print_stack ();
	  else
	    {
	      fputs_filtered (output, stream);
	      xfree (output);
	    }
	}
      else
	{
	  struct value *value = convert_value_from_python (py_v);

	  if (value == NULL)
	    {
	      gdbpy_print_stack ();
	      error (_("Error while executing Python code."));
	    }
	  else
	    common_val_print (value, stream, recurse + 1, options, language);
	}

      if (is_map && i % 2 == 0)
	fputs_filtered ("] = ", stream);

      do_cleanups (inner_cleanup);
    }

  if (i)
    {
      if (!done_flag)
	{
	  if (pretty)
	    {
	      fputs_filtered ("\n", stream);
	      print_spaces_filtered (2 + 2 * recurse, stream);
	    }
	  fputs_filtered ("...", stream);
	}
      if (pretty)
	{
	  fputs_filtered ("\n", stream);
	  print_spaces_filtered (2 * recurse, stream);
	}
      fputs_filtered ("}", stream);
    }

 done:
  do_cleanups (cleanups);
}

int
apply_val_pretty_printer (struct type *type, const gdb_byte *valaddr,
			  int embedded_offset, CORE_ADDR address,
			  struct ui_file *stream, int recurse,
			  const struct value *val,
			  const struct value_print_options *options,
			  const struct language_defn *language)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  PyObject *printer = NULL;
  PyObject *val_obj = NULL;
  struct value *value;
  char *hint = NULL;
  struct cleanup *cleanups;
  int result = 0;
  enum string_repr_result print_result;

  /* No pretty-printer support for unavailable values.  */
  if (!value_bytes_available (val, embedded_offset, TYPE_LENGTH (type)))
    return 0;

  if (!gdb_python_initialized)
    return 0;

  cleanups = ensure_python_env (gdbarch, language);

  /* Instantiate the printer.  */
  if (valaddr)
    valaddr += embedded_offset;
  value = value_from_contents_and_address (type, valaddr,
					   address + embedded_offset);

  set_value_component_location (value, val);
  /* set_value_component_location resets the address, so we may
     need to set it again.  */
  if (VALUE_LVAL (value) != lval_internalvar
      && VALUE_LVAL (value) != lval_internalvar_component
      && VALUE_LVAL (value) != lval_computed)
    set_value_address (value, address + embedded_offset);

  val_obj = value_to_value_object (value);
  if (! val_obj)
    goto done;

  /* Find the constructor.  */
  printer = find_pretty_printer (val_obj);
  Py_DECREF (val_obj);

  if (printer == NULL)
    goto done;

  make_cleanup_py_decref (printer);
  if (printer == Py_None)
    goto done;

  /* If we are printing a map, we want some special formatting.  */
  hint = gdbpy_get_display_hint (printer);
  make_cleanup (free_current_contents, &hint);

  /* Print the section */
  print_result = print_string_repr (printer, hint, stream, recurse,
				    options, language, gdbarch);
  if (print_result != string_repr_error)
    print_children (printer, hint, stream, recurse, options, language,
		    print_result == string_repr_none);

  result = 1;


 done:
  if (PyErr_Occurred ())
    print_stack_unless_memory_error (stream);
  do_cleanups (cleanups);
  return result;
}


/* Apply a pretty-printer for the varobj code.  PRINTER_OBJ is the
   print object.  It must have a 'to_string' method (but this is
   checked by varobj, not here) which takes no arguments and
   returns a string.  The printer will return a value and in the case
   of a Python string being returned, this function will return a
   PyObject containing the string.  For any other type, *REPLACEMENT is
   set to the replacement value and this function returns NULL.  On
   error, *REPLACEMENT is set to NULL and this function also returns
   NULL.  */
PyObject *
apply_varobj_pretty_printer (PyObject *printer_obj,
			     struct value **replacement,
			     struct ui_file *stream)
{
  PyObject *py_str = NULL;

  *replacement = NULL;
  py_str = pretty_print_one_value (printer_obj, replacement);

  if (*replacement == NULL && py_str == NULL)
    print_stack_unless_memory_error (stream);

  return py_str;
}

/* Find a pretty-printer object for the varobj module.  Returns a new
   reference to the object if successful; returns NULL if not.  VALUE
   is the value for which a printer tests to determine if it
   can pretty-print the value.  */
PyObject *
gdbpy_get_varobj_pretty_printer (struct value *value)
{
  PyObject *val_obj;
  PyObject *pretty_printer = NULL;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      value = value_copy (value);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  val_obj = value_to_value_object (value);
  if (! val_obj)
    return NULL;

  pretty_printer = find_pretty_printer (val_obj);
  Py_DECREF (val_obj);
  return pretty_printer;
}

/* A Python function which wraps find_pretty_printer and instantiates
   the resulting class.  This accepts a Value argument and returns a
   pretty printer instance, or None.  This function is useful as an
   argument to the MI command -var-set-visualizer.  */
PyObject *
gdbpy_default_visualizer (PyObject *self, PyObject *args)
{
  PyObject *val_obj;
  PyObject *cons;
  struct value *value;

  if (! PyArg_ParseTuple (args, "O", &val_obj))
    return NULL;
  value = value_object_to_value (val_obj);
  if (! value)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Argument must be a gdb.Value."));
      return NULL;
    }

  cons = find_pretty_printer (val_obj);
  return cons;
}

#else /* HAVE_PYTHON */

int
apply_val_pretty_printer (struct type *type, const gdb_byte *valaddr,
			  int embedded_offset, CORE_ADDR address,
			  struct ui_file *stream, int recurse,
			  const struct value *val,
			  const struct value_print_options *options,
			  const struct language_defn *language)
{
  return 0;
}

#endif /* HAVE_PYTHON */
