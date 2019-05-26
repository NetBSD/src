/* Python pretty-printing

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
#include "objfiles.h"
#include "symtab.h"
#include "language.h"
#include "valprint.h"
#include "extension-priv.h"
#include "python.h"
#include "python-internal.h"
#include "py-ref.h"

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

  pp_list_size = PyList_Size (list);
  for (list_index = 0; list_index < pp_list_size; list_index++)
    {
      PyObject *function = PyList_GetItem (list, list_index);
      if (! function)
	return NULL;

      /* Skip if disabled.  */
      if (PyObject_HasAttr (function, gdbpy_enabled_cst))
	{
	  gdbpy_ref<> attr (PyObject_GetAttr (function, gdbpy_enabled_cst));
	  int cmp;

	  if (attr == NULL)
	    return NULL;
	  cmp = PyObject_IsTrue (attr.get ());
	  if (cmp == -1)
	    return NULL;

	  if (!cmp)
	    continue;
	}

      gdbpy_ref<> printer (PyObject_CallFunctionObjArgs (function, value,
							 NULL));
      if (printer == NULL)
	return NULL;
      else if (printer != Py_None)
	return printer.release ();
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

    gdbpy_ref<> pp_list (objfpy_get_printers (objf, NULL));
    gdbpy_ref<> function (search_pp_list (pp_list.get (), value));

    /* If there is an error in any objfile list, abort the search and exit.  */
    if (function == NULL)
      return NULL;

    if (function != Py_None)
      return function.release ();
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
  PyObject *obj = pspace_to_pspace_object (current_program_space);

  if (!obj)
    return NULL;
  gdbpy_ref<> pp_list (pspy_get_printers (obj, NULL));
  return search_pp_list (pp_list.get (), value);
}

/* Subroutine of find_pretty_printer to simplify it.
   Look for a pretty-printer to print VALUE in the gdb module.
   The result is NULL if there's an error and the search should be terminated.
   The result is Py_None, suitably inc-ref'd, if no pretty-printer was found.
   Otherwise the result is the pretty-printer function, suitably inc-ref'd.  */

static PyObject *
find_pretty_printer_from_gdb (PyObject *value)
{
  /* Fetch the global pretty printer list.  */
  if (gdb_python_module == NULL
      || ! PyObject_HasAttrString (gdb_python_module, "pretty_printers"))
    Py_RETURN_NONE;
  gdbpy_ref<> pp_list (PyObject_GetAttrString (gdb_python_module,
					       "pretty_printers"));
  if (pp_list == NULL || ! PyList_Check (pp_list.get ()))
    Py_RETURN_NONE;

  return search_pp_list (pp_list.get (), value);
}

/* Find the pretty-printing constructor function for VALUE.  If no
   pretty-printer exists, return None.  If one exists, return a new
   reference.  On error, set the Python error and return NULL.  */

static PyObject *
find_pretty_printer (PyObject *value)
{
  /* Look at the pretty-printer list for each objfile
     in the current program-space.  */
  gdbpy_ref<> function (find_pretty_printer_from_objfiles (value));
  if (function == NULL || function != Py_None)
    return function.release ();

  /* Look at the pretty-printer list for the current program-space.  */
  function.reset (find_pretty_printer_from_progspace (value));
  if (function == NULL || function != Py_None)
    return function.release ();

  /* Look at the pretty-printer list in the gdb module.  */
  return find_pretty_printer_from_gdb (value);
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
  gdbpy_ref<> result;

  *out_value = NULL;
  TRY
    {
      result.reset (PyObject_CallMethodObjArgs (printer, gdbpy_to_string_cst,
						NULL));
      if (result != NULL)
	{
	  if (! gdbpy_is_string (result.get ())
	      && ! gdbpy_is_lazy_string (result.get ())
	      && result != Py_None)
	    {
	      *out_value = convert_value_from_python (result.get ());
	      if (PyErr_Occurred ())
		*out_value = NULL;
	      result = NULL;
	    }
	}
    }
  CATCH (except, RETURN_MASK_ALL)
    {
    }
  END_CATCH

  return result.release ();
}

/* Return the display hint for the object printer, PRINTER.  Return
   NULL if there is no display_hint method, or if the method did not
   return a string.  On error, print stack trace and return NULL.  On
   success, return an xmalloc()d string.  */
gdb::unique_xmalloc_ptr<char>
gdbpy_get_display_hint (PyObject *printer)
{
  gdb::unique_xmalloc_ptr<char> result;

  if (! PyObject_HasAttr (printer, gdbpy_display_hint_cst))
    return NULL;

  gdbpy_ref<> hint (PyObject_CallMethodObjArgs (printer, gdbpy_display_hint_cst,
						NULL));
  if (hint != NULL)
    {
      if (gdbpy_is_string (hint.get ()))
	{
	  result = python_string_to_host_string (hint.get ());
	  if (result == NULL)
	    gdbpy_print_stack ();
	}
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
      PyObject *type, *value, *trace;

      PyErr_Fetch (&type, &value, &trace);

      gdbpy_ref<> type_ref (type);
      gdbpy_ref<> value_ref (value);
      gdbpy_ref<> trace_ref (trace);

      gdb::unique_xmalloc_ptr<char>
	msg (gdbpy_exception_to_string (type, value));

      if (msg == NULL || *msg == '\0')
	fprintf_filtered (stream, _("<error reading variable>"));
      else
	fprintf_filtered (stream, _("<error reading variable: %s>"),
			  msg.get ());
    }
  else
    gdbpy_print_stack ();
}

/* Helper for gdbpy_apply_val_pretty_printer which calls to_string and
   formats the result.  */

static enum string_repr_result
print_string_repr (PyObject *printer, const char *hint,
		   struct ui_file *stream, int recurse,
		   const struct value_print_options *options,
		   const struct language_defn *language,
		   struct gdbarch *gdbarch)
{
  struct value *replacement = NULL;
  enum string_repr_result result = string_repr_ok;

  gdbpy_ref<> py_str (pretty_print_one_value (printer, &replacement));
  if (py_str != NULL)
    {
      if (py_str == Py_None)
	result = string_repr_none;
      else if (gdbpy_is_lazy_string (py_str.get ()))
	{
	  CORE_ADDR addr;
	  long length;
	  struct type *type;
	  gdb::unique_xmalloc_ptr<char> encoding;
	  struct value_print_options local_opts = *options;

	  gdbpy_extract_lazy_string (py_str.get (), &addr, &type,
				     &length, &encoding);

	  local_opts.addressprint = 0;
	  val_print_string (type, encoding.get (), addr, (int) length,
			    stream, &local_opts);
	}
      else
	{
	  gdbpy_ref<> string
	    (python_string_to_target_python_string (py_str.get ()));
	  if (string != NULL)
	    {
	      char *output;
	      long length;
	      struct type *type;

#ifdef IS_PY3K
	      output = PyBytes_AS_STRING (string.get ());
	      length = PyBytes_GET_SIZE (string.get ());
#else
	      output = PyString_AsString (string.get ());
	      length = PyString_Size (string.get ());
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

/* Create a dummy PyFrameObject, needed to work around
   a Python-2.4 bug with generators.  */
class dummy_python_frame
{
 public:

  dummy_python_frame ();

  ~dummy_python_frame ()
  {
    if (m_valid)
      m_tstate->frame = m_saved_frame;
  }

  bool failed () const
  {
    return !m_valid;
  }

 private:

  bool m_valid;
  PyFrameObject *m_saved_frame;
  gdbpy_ref<> m_frame;
  PyThreadState *m_tstate;
};

dummy_python_frame::dummy_python_frame ()
: m_valid (false),
  m_saved_frame (NULL),
  m_tstate (NULL)
{
  PyCodeObject *code;
  PyFrameObject *frame;

  gdbpy_ref<> empty_string (PyString_FromString (""));
  if (empty_string == NULL)
    return;

  gdbpy_ref<> null_tuple (PyTuple_New (0));
  if (null_tuple == NULL)
    return;

  code = PyCode_New (0,			  /* argcount */
		     0,			  /* locals */
		     0,			  /* stacksize */
		     0,			  /* flags */
		     empty_string.get (), /* code */
		     null_tuple.get (),	  /* consts */
		     null_tuple.get (),	  /* names */
		     null_tuple.get (),	  /* varnames */
#if PYTHON_API_VERSION >= 1010
		     null_tuple.get (),	  /* freevars */
		     null_tuple.get (),	  /* cellvars */
#endif
		     empty_string.get (), /* filename */
		     empty_string.get (), /* name */
		     1,			  /* firstlineno */
		     empty_string.get ()  /* lnotab */
		     );
  if (code == NULL)
    return;
  gdbpy_ref<> code_holder ((PyObject *) code);

  gdbpy_ref<> globals (PyDict_New ());
  if (globals == NULL)
    return;

  m_tstate = PyThreadState_GET ();
  frame = PyFrame_New (m_tstate, code, globals.get (), NULL);
  if (frame == NULL)
    return;

  m_frame.reset ((PyObject *) frame);
  m_tstate->frame = frame;
  m_saved_frame = frame->f_back;
  m_valid = true;
}
#endif

/* Helper for gdbpy_apply_val_pretty_printer that formats children of the
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

  if (! PyObject_HasAttr (printer, gdbpy_children_cst))
    return;

  /* If we are printing a map or an array, we want some special
     formatting.  */
  is_map = hint && ! strcmp (hint, "map");
  is_array = hint && ! strcmp (hint, "array");

  gdbpy_ref<> children (PyObject_CallMethodObjArgs (printer, gdbpy_children_cst,
						    NULL));
  if (children == NULL)
    {
      print_stack_unless_memory_error (stream);
      return;
    }

  gdbpy_ref<> iter (PyObject_GetIter (children.get ()));
  if (iter == NULL)
    {
      print_stack_unless_memory_error (stream);
      return;
    }

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
  dummy_python_frame frame;
  if (frame.failed ())
    {
      gdbpy_print_stack ();
      return;
    }
#endif

  done_flag = 0;
  for (i = 0; i < options->print_max; ++i)
    {
      PyObject *py_v;
      const char *name;

      gdbpy_ref<> item (PyIter_Next (iter.get ()));
      if (item == NULL)
	{
	  if (PyErr_Occurred ())
	    print_stack_unless_memory_error (stream);
	  /* Set a flag so we can know whether we printed all the
	     available elements.  */
	  else	
	    done_flag = 1;
	  break;
	}

      if (! PyTuple_Check (item.get ()) || PyTuple_Size (item.get ()) != 2)
	{
	  PyErr_SetString (PyExc_TypeError,
			   _("Result of children iterator not a tuple"
			     " of two elements."));
	  gdbpy_print_stack ();
	  continue;
	}
      if (! PyArg_ParseTuple (item.get (), "sO", &name, &py_v))
	{
	  /* The user won't necessarily get a stack trace here, so provide
	     more context.  */
	  if (gdbpy_print_python_errors_p ())
	    fprintf_unfiltered (gdb_stderr,
				_("Bad result from children iterator.\n"));
	  gdbpy_print_stack ();
	  continue;
	}

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
	  gdb::unique_xmalloc_ptr<char> encoding;
	  struct value_print_options local_opts = *options;

	  gdbpy_extract_lazy_string (py_v, &addr, &type, &length, &encoding);

	  local_opts.addressprint = 0;
	  val_print_string (type, encoding.get (), addr, (int) length, stream,
			    &local_opts);
	}
      else if (gdbpy_is_string (py_v))
	{
	  gdb::unique_xmalloc_ptr<char> output;

	  output = python_string_to_host_string (py_v);
	  if (!output)
	    gdbpy_print_stack ();
	  else
	    fputs_filtered (output.get (), stream);
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
}

enum ext_lang_rc
gdbpy_apply_val_pretty_printer (const struct extension_language_defn *extlang,
				struct type *type,
				LONGEST embedded_offset, CORE_ADDR address,
				struct ui_file *stream, int recurse,
				struct value *val,
				const struct value_print_options *options,
				const struct language_defn *language)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  struct value *value;
  enum string_repr_result print_result;
  const gdb_byte *valaddr = value_contents_for_printing (val);

  /* No pretty-printer support for unavailable values.  */
  if (!value_bytes_available (val, embedded_offset, TYPE_LENGTH (type)))
    return EXT_LANG_RC_NOP;

  if (!gdb_python_initialized)
    return EXT_LANG_RC_NOP;

  gdbpy_enter enter_py (gdbarch, language);

  /* Instantiate the printer.  */
  value = value_from_component (val, type, embedded_offset);

  gdbpy_ref<> val_obj (value_to_value_object (value));
  if (val_obj == NULL)
    {
      print_stack_unless_memory_error (stream);
      return EXT_LANG_RC_ERROR;
    }

  /* Find the constructor.  */
  gdbpy_ref<> printer (find_pretty_printer (val_obj.get ()));
  if (printer == NULL)
    {
      print_stack_unless_memory_error (stream);
      return EXT_LANG_RC_ERROR;
    }

  if (printer == Py_None)
    return EXT_LANG_RC_NOP;

  /* If we are printing a map, we want some special formatting.  */
  gdb::unique_xmalloc_ptr<char> hint (gdbpy_get_display_hint (printer.get ()));

  /* Print the section */
  print_result = print_string_repr (printer.get (), hint.get (), stream,
				    recurse, options, language, gdbarch);
  if (print_result != string_repr_error)
    print_children (printer.get (), hint.get (), stream, recurse, options,
		    language, print_result == string_repr_none);

  if (PyErr_Occurred ())
    print_stack_unless_memory_error (stream);
  return EXT_LANG_RC_OK;
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
  TRY
    {
      value = value_copy (value);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }
  END_CATCH

  gdbpy_ref<> val_obj (value_to_value_object (value));
  if (val_obj == NULL)
    return NULL;

  return find_pretty_printer (val_obj.get ());
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
