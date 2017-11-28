/* Python frame filters

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
#include "objfiles.h"
#include "symtab.h"
#include "language.h"
#include "arch-utils.h"
#include "python.h"
#include "ui-out.h"
#include "valprint.h"
#include "annotate.h"
#include "hashtab.h"
#include "demangle.h"
#include "mi/mi-cmds.h"
#include "python-internal.h"

enum mi_print_types
{
  MI_PRINT_ARGS,
  MI_PRINT_LOCALS
};

/* Helper  function  to  extract  a  symbol, a  name  and  a  language
   definition from a Python object that conforms to the "Symbol Value"
   interface.  OBJ  is the Python  object to extract the  values from.
   NAME is a  pass-through argument where the name of  the symbol will
   be written.  NAME is allocated in  this function, but the caller is
   responsible for clean up.  SYM is a pass-through argument where the
   symbol will be written and  SYM_BLOCK is a pass-through argument to
   write  the block where the symbol lies in.  In the case of the  API
   returning a  string,  this will be set to NULL.  LANGUAGE is also a
   pass-through  argument  denoting  the  language  attributed  to the
   Symbol.  In the case of SYM being  NULL, this  will be  set to  the
   current  language.  Returns  EXT_LANG_BT_ERROR  on  error  with the
   appropriate Python exception set, and EXT_LANG_BT_OK on success.  */

static enum ext_lang_bt_status
extract_sym (PyObject *obj, char **name, struct symbol **sym,
	     struct block **sym_block, const struct language_defn **language)
{
  PyObject *result = PyObject_CallMethod (obj, "symbol", NULL);

  if (result == NULL)
    return EXT_LANG_BT_ERROR;

  /* For 'symbol' callback, the function can return a symbol or a
     string.  */
  if (gdbpy_is_string (result))
    {
      *name = python_string_to_host_string (result);
      Py_DECREF (result);

      if (*name == NULL)
	return EXT_LANG_BT_ERROR;
      /* If the API returns a string (and not a symbol), then there is
	no symbol derived language available and the frame filter has
	either overridden the symbol with a string, or supplied a
	entirely synthetic symbol/value pairing.  In that case, use
	python_language.  */
      *language = python_language;
      *sym = NULL;
      *sym_block = NULL;
    }
  else
    {
      /* This type checks 'result' during the conversion so we
	 just call it unconditionally and check the return.  */
      *sym = symbol_object_to_symbol (result);
      /* TODO: currently, we have no way to recover the block in which SYMBOL
	 was found, so we have no block to return.  Trying to evaluate SYMBOL
	 will yield an incorrect value when it's located in a FRAME and
	 evaluated from another frame (as permitted in nested functions).  */
      *sym_block = NULL;

      Py_DECREF (result);

      if (*sym == NULL)
	{
	  PyErr_SetString (PyExc_RuntimeError,
			   _("Unexpected value.  Expecting a "
			     "gdb.Symbol or a Python string."));
	  return EXT_LANG_BT_ERROR;
	}

      /* Duplicate the symbol name, so the caller has consistency
	 in garbage collection.  */
      *name = xstrdup (SYMBOL_PRINT_NAME (*sym));

      /* If a symbol is specified attempt to determine the language
	 from the symbol.  If mode is not "auto", then the language
	 has been explicitly set, use that.  */
      if (language_mode == language_mode_auto)
	*language = language_def (SYMBOL_LANGUAGE (*sym));
      else
	*language = current_language;
    }

  return EXT_LANG_BT_OK;
}

/* Helper function to extract a value from an object that conforms to
   the "Symbol Value" interface.  OBJ is the Python object to extract
   the value from.  VALUE is a pass-through argument where the value
   will be written.  If the object does not have the value attribute,
   or provides the Python None for a value, VALUE will be set to NULL
   and this function will return as successful.  Returns EXT_LANG_BT_ERROR
   on error with the appropriate Python exception set, and EXT_LANG_BT_OK on
   success.  */

static enum ext_lang_bt_status
extract_value (PyObject *obj, struct value **value)
{
  if (PyObject_HasAttrString (obj, "value"))
    {
      PyObject *vresult = PyObject_CallMethod (obj, "value", NULL);

      if (vresult == NULL)
	return EXT_LANG_BT_ERROR;

      /* The Python code has returned 'None' for a value, so we set
	 value to NULL.  This flags that GDB should read the
	 value.  */
      if (vresult == Py_None)
	{
	  Py_DECREF (vresult);
	  *value = NULL;
	  return EXT_LANG_BT_OK;
	}
      else
	{
	  *value = convert_value_from_python (vresult);
	  Py_DECREF (vresult);

	  if (*value == NULL)
	    return EXT_LANG_BT_ERROR;

	  return EXT_LANG_BT_OK;
	}
    }
  else
    *value = NULL;

  return EXT_LANG_BT_OK;
}

/* MI prints only certain values according to the type of symbol and
   also what the user has specified.  SYM is the symbol to check, and
   MI_PRINT_TYPES is an enum specifying what the user wants emitted
   for the MI command in question.  */
static int
mi_should_print (struct symbol *sym, enum mi_print_types type)
{
  int print_me = 0;

  switch (SYMBOL_CLASS (sym))
    {
    default:
    case LOC_UNDEF:	/* catches errors        */
    case LOC_CONST:	/* constant              */
    case LOC_TYPEDEF:	/* local typedef         */
    case LOC_LABEL:	/* local label           */
    case LOC_BLOCK:	/* local function        */
    case LOC_CONST_BYTES:	/* loc. byte seq.        */
    case LOC_UNRESOLVED:	/* unresolved static     */
    case LOC_OPTIMIZED_OUT:	/* optimized out         */
      print_me = 0;
      break;

    case LOC_ARG:	/* argument              */
    case LOC_REF_ARG:	/* reference arg         */
    case LOC_REGPARM_ADDR:	/* indirect register arg */
    case LOC_LOCAL:	/* stack local           */
    case LOC_STATIC:	/* static                */
    case LOC_REGISTER:	/* register              */
    case LOC_COMPUTED:	/* computed location     */
      if (type == MI_PRINT_LOCALS)
	print_me = ! SYMBOL_IS_ARGUMENT (sym);
      else
	print_me = SYMBOL_IS_ARGUMENT (sym);
    }
  return print_me;
}

/* Helper function which outputs a type name extracted from VAL to a
   "type" field in the output stream OUT.  OUT is the ui-out structure
   the type name will be output too, and VAL is the value that the
   type will be extracted from.  Returns EXT_LANG_BT_ERROR on error, with
   any GDB exceptions converted to a Python exception, or EXT_LANG_BT_OK on
   success.  */

static enum ext_lang_bt_status
py_print_type (struct ui_out *out, struct value *val)
{

  TRY
    {
      struct ui_file *stb;
      struct cleanup *cleanup;

      stb = mem_fileopen ();
      cleanup = make_cleanup_ui_file_delete (stb);
      check_typedef (value_type (val));
      type_print (value_type (val), "", stb, -1);
      ui_out_field_stream (out, "type", stb);
      do_cleanups (cleanup);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      gdbpy_convert_exception (except);
      return EXT_LANG_BT_ERROR;
    }
  END_CATCH

  return EXT_LANG_BT_OK;
}

/* Helper function which outputs a value to an output field in a
   stream.  OUT is the ui-out structure the value will be output to,
   VAL is the value that will be printed, OPTS contains the value
   printing options, ARGS_TYPE is an enumerator describing the
   argument format, and LANGUAGE is the language_defn that the value
   will be printed with.  Returns EXT_LANG_BT_ERROR on error, with any GDB
   exceptions converted to a Python exception, or EXT_LANG_BT_OK on
   success. */

static enum ext_lang_bt_status
py_print_value (struct ui_out *out, struct value *val,
		const struct value_print_options *opts,
		int indent,
		enum ext_lang_frame_args args_type,
		const struct language_defn *language)
{
  int should_print = 0;

  /* MI does not print certain values, differentiated by type,
     depending on what ARGS_TYPE indicates.  Test type against option.
     For CLI print all values.  */
  if (args_type == MI_PRINT_SIMPLE_VALUES
      || args_type == MI_PRINT_ALL_VALUES)
    {
      struct type *type = NULL;

      TRY
	{
	  type = check_typedef (value_type (val));
	}
      CATCH (except, RETURN_MASK_ALL)
	{
	  gdbpy_convert_exception (except);
	  return EXT_LANG_BT_ERROR;
	}
      END_CATCH

      if (args_type == MI_PRINT_ALL_VALUES)
	should_print = 1;
      else if (args_type == MI_PRINT_SIMPLE_VALUES
	       && TYPE_CODE (type) != TYPE_CODE_ARRAY
	       && TYPE_CODE (type) != TYPE_CODE_STRUCT
	       && TYPE_CODE (type) != TYPE_CODE_UNION)
	should_print = 1;
    }
  else if (args_type != NO_VALUES)
    should_print = 1;

  if (should_print)
    {
      TRY
	{
	  struct ui_file *stb;
	  struct cleanup *cleanup;

	  stb = mem_fileopen ();
	  cleanup = make_cleanup_ui_file_delete (stb);
	  common_val_print (val, stb, indent, opts, language);
	  ui_out_field_stream (out, "value", stb);
	  do_cleanups (cleanup);
	}
      CATCH (except, RETURN_MASK_ALL)
	{
	  gdbpy_convert_exception (except);
	  return EXT_LANG_BT_ERROR;
	}
      END_CATCH
    }

  return EXT_LANG_BT_OK;
}

/* Helper function to call a Python method and extract an iterator
   from the result.  If the function returns anything but an iterator
   the exception is preserved and NULL is returned.  FILTER is the
   Python object to call, and FUNC is the name of the method.  Returns
   a PyObject, or NULL on error with the appropriate exception set.
   This function can return an iterator, or NULL.  */

static PyObject *
get_py_iter_from_func (PyObject *filter, char *func)
{
  if (PyObject_HasAttrString (filter, func))
    {
      PyObject *result = PyObject_CallMethod (filter, func, NULL);

      if (result != NULL)
	{
	  if (result == Py_None)
	    {
	      return result;
	    }
	  else
	    {
	      PyObject *iterator = PyObject_GetIter (result);

	      Py_DECREF (result);
	      return iterator;
	    }
	}
    }
  else
    Py_RETURN_NONE;

  return NULL;
}

/*  Helper function to output a single frame argument and value to an
    output stream.  This function will account for entry values if the
    FV parameter is populated, the frame argument has entry values
    associated with them, and the appropriate "set entry-value"
    options are set.  Will output in CLI or MI like format depending
    on the type of output stream detected.  OUT is the output stream,
    SYM_NAME is the name of the symbol.  If SYM_NAME is populated then
    it must have an accompanying value in the parameter FV.  FA is a
    frame argument structure.  If FA is populated, both SYM_NAME and
    FV are ignored.  OPTS contains the value printing options,
    ARGS_TYPE is an enumerator describing the argument format,
    PRINT_ARGS_FIELD is a flag which indicates if we output "ARGS=1"
    in MI output in commands where both arguments and locals are
    printed.  Returns EXT_LANG_BT_ERROR on error, with any GDB exceptions
    converted to a Python exception, or EXT_LANG_BT_OK on success.  */

static enum ext_lang_bt_status
py_print_single_arg (struct ui_out *out,
		     const char *sym_name,
		     struct frame_arg *fa,
		     struct value *fv,
		     const struct value_print_options *opts,
		     enum ext_lang_frame_args args_type,
		     int print_args_field,
		     const struct language_defn *language)
{
  struct value *val;
  enum ext_lang_bt_status retval = EXT_LANG_BT_OK;

  if (fa != NULL)
    {
      if (fa->val == NULL && fa->error == NULL)
	return EXT_LANG_BT_OK;
      language = language_def (SYMBOL_LANGUAGE (fa->sym));
      val = fa->val;
    }
  else
    val = fv;

  TRY
    {
      struct cleanup *cleanups = make_cleanup (null_cleanup, NULL);

      /*  MI has varying rules for tuples, but generally if there is only
      one element in each item in the list, do not start a tuple.  The
      exception is -stack-list-variables which emits an ARGS="1" field
      if the value is a frame argument.  This is denoted in this
      function with PRINT_ARGS_FIELD which is flag from the caller to
      emit the ARGS field.  */
      if (ui_out_is_mi_like_p (out))
	{
	  if (print_args_field || args_type != NO_VALUES)
	    make_cleanup_ui_out_tuple_begin_end (out, NULL);
	}

      annotate_arg_begin ();

      /* If frame argument is populated, check for entry-values and the
	 entry value options.  */
      if (fa != NULL)
	{
	  struct ui_file *stb;

	  stb = mem_fileopen ();
	  make_cleanup_ui_file_delete (stb);
	  fprintf_symbol_filtered (stb, SYMBOL_PRINT_NAME (fa->sym),
				   SYMBOL_LANGUAGE (fa->sym),
				   DMGL_PARAMS | DMGL_ANSI);
	  if (fa->entry_kind == print_entry_values_compact)
	    {
	      fputs_filtered ("=", stb);

	      fprintf_symbol_filtered (stb, SYMBOL_PRINT_NAME (fa->sym),
				       SYMBOL_LANGUAGE (fa->sym),
				       DMGL_PARAMS | DMGL_ANSI);
	    }
	  if (fa->entry_kind == print_entry_values_only
	      || fa->entry_kind == print_entry_values_compact)
	    {
	      fputs_filtered ("@entry", stb);
	    }
	  ui_out_field_stream (out, "name", stb);
	}
      else
	/* Otherwise, just output the name.  */
	ui_out_field_string (out, "name", sym_name);

      annotate_arg_name_end ();

      if (! ui_out_is_mi_like_p (out))
	ui_out_text (out, "=");

      if (print_args_field)
	ui_out_field_int (out, "arg", 1);

      /* For MI print the type, but only for simple values.  This seems
	 weird, but this is how MI choose to format the various output
	 types.  */
      if (args_type == MI_PRINT_SIMPLE_VALUES && val != NULL)
	{
	  if (py_print_type (out, val) == EXT_LANG_BT_ERROR)
	    {
	      retval = EXT_LANG_BT_ERROR;
	      do_cleanups (cleanups);
	    }
	}

      if (retval != EXT_LANG_BT_ERROR)
	{
	  if (val != NULL)
	    annotate_arg_value (value_type (val));

	  /* If the output is to the CLI, and the user option "set print
	     frame-arguments" is set to none, just output "...".  */
	  if (! ui_out_is_mi_like_p (out) && args_type == NO_VALUES)
	    ui_out_field_string (out, "value", "...");
	  else
	    {
	      /* Otherwise, print the value for both MI and the CLI, except
		 for the case of MI_PRINT_NO_VALUES.  */
	      if (args_type != NO_VALUES)
		{
		  if (val == NULL)
		    {
		      gdb_assert (fa != NULL && fa->error != NULL);
		      ui_out_field_fmt (out, "value",
					_("<error reading variable: %s>"),
					fa->error);
		    }
		  else if (py_print_value (out, val, opts, 0, args_type, language)
			   == EXT_LANG_BT_ERROR)
		    retval = EXT_LANG_BT_ERROR;
		}
	    }

	  do_cleanups (cleanups);
	}
    }
  CATCH (except, RETURN_MASK_ERROR)
    {
      gdbpy_convert_exception (except);
    }
  END_CATCH

  return retval;
}

/* Helper function to loop over frame arguments provided by the
   "frame_arguments" Python API.  Elements in the iterator must
   conform to the "Symbol Value" interface.  ITER is the Python
   iterable object, OUT is the output stream, ARGS_TYPE is an
   enumerator describing the argument format, PRINT_ARGS_FIELD is a
   flag which indicates if we output "ARGS=1" in MI output in commands
   where both arguments and locals are printed, and FRAME is the
   backing frame.  Returns EXT_LANG_BT_ERROR on error, with any GDB
   exceptions converted to a Python exception, or EXT_LANG_BT_OK on
   success.  */

static enum ext_lang_bt_status
enumerate_args (PyObject *iter,
		struct ui_out *out,
		enum ext_lang_frame_args args_type,
		int print_args_field,
		struct frame_info *frame)
{
  PyObject *item;
  struct value_print_options opts;

  get_user_print_options (&opts);

  if (args_type == CLI_SCALAR_VALUES)
    {
      /* True in "summary" mode, false otherwise.  */
      opts.summary = 1;
    }

  opts.deref_ref = 1;

  TRY
    {
      annotate_frame_args ();
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      gdbpy_convert_exception (except);
      goto error;
    }
  END_CATCH

  /*  Collect the first argument outside of the loop, so output of
      commas in the argument output is correct.  At the end of the
      loop block collect another item from the iterator, and, if it is
      not null emit a comma.  */
  item = PyIter_Next (iter);
  if (item == NULL && PyErr_Occurred ())
    goto error;

  while (item)
    {
      const struct language_defn *language;
      char *sym_name;
      struct symbol *sym;
      struct block *sym_block;
      struct value *val;
      enum ext_lang_bt_status success = EXT_LANG_BT_ERROR;

      success = extract_sym (item, &sym_name, &sym, &sym_block, &language);
      if (success == EXT_LANG_BT_ERROR)
	{
	  Py_DECREF (item);
	  goto error;
	}

      success = extract_value (item, &val);
      if (success == EXT_LANG_BT_ERROR)
	{
	  xfree (sym_name);
	  Py_DECREF (item);
	  goto error;
	}

      Py_DECREF (item);
      item = NULL;

      if (sym && ui_out_is_mi_like_p (out)
	  && ! mi_should_print (sym, MI_PRINT_ARGS))
	{
	  xfree (sym_name);
	  continue;
	}

      /* If the object did not provide a value, read it using
	 read_frame_args and account for entry values, if any.  */
      if (val == NULL)
	{
	  struct frame_arg arg, entryarg;

	  /* If there is no value, and also no symbol, set error and
	     exit.  */
	  if (sym == NULL)
	    {
	      PyErr_SetString (PyExc_RuntimeError,
			       _("No symbol or value provided."));
	      xfree (sym_name);
	      goto error;
	    }

	  TRY
	    {
	      read_frame_arg (sym, frame, &arg, &entryarg);
	    }
	  CATCH (except, RETURN_MASK_ALL)
	    {
	      xfree (sym_name);
	      gdbpy_convert_exception (except);
	      goto error;
	    }
	  END_CATCH

	  /* The object has not provided a value, so this is a frame
	     argument to be read by GDB.  In this case we have to
	     account for entry-values.  */

	  if (arg.entry_kind != print_entry_values_only)
	    {
	      if (py_print_single_arg (out, NULL, &arg,
				       NULL, &opts,
				       args_type,
				       print_args_field,
				       NULL) == EXT_LANG_BT_ERROR)
		{
		  xfree (arg.error);
		  xfree (entryarg.error);
		  xfree (sym_name);
		  goto error;
		}
	    }

	  if (entryarg.entry_kind != print_entry_values_no)
	    {
	      if (arg.entry_kind != print_entry_values_only)
		{
		  TRY
		    {
		      ui_out_text (out, ", ");
		      ui_out_wrap_hint (out, "    ");
		    }
		  CATCH (except, RETURN_MASK_ALL)
		    {
		      xfree (arg.error);
		      xfree (entryarg.error);
		      xfree (sym_name);
		      gdbpy_convert_exception (except);
		      goto error;
		    }
		  END_CATCH
		}

	      if (py_print_single_arg (out, NULL, &entryarg, NULL, &opts,
				       args_type, print_args_field, NULL)
		  == EXT_LANG_BT_ERROR)
		{
		      xfree (arg.error);
		      xfree (entryarg.error);
		      xfree (sym_name);
		      goto error;
		}
	    }

	  xfree (arg.error);
	  xfree (entryarg.error);
	}
      else
	{
	  /* If the object has provided a value, we just print that.  */
	  if (val != NULL)
	    {
	      if (py_print_single_arg (out, sym_name, NULL, val, &opts,
				       args_type, print_args_field,
				       language) == EXT_LANG_BT_ERROR)
		{
		  xfree (sym_name);
		  goto error;
		}
	    }
	}

      xfree (sym_name);

      /* Collect the next item from the iterator.  If
	 this is the last item, do not print the
	 comma.  */
      item = PyIter_Next (iter);
      if (item != NULL)
	{
	  TRY
	    {
	      ui_out_text (out, ", ");
	    }
	  CATCH (except, RETURN_MASK_ALL)
	    {
	      Py_DECREF (item);
	      gdbpy_convert_exception (except);
	      goto error;
	    }
	  END_CATCH
	}
      else if (PyErr_Occurred ())
	goto error;

      TRY
	{
	  annotate_arg_end ();
	}
      CATCH (except, RETURN_MASK_ALL)
	{
	  Py_DECREF (item);
	  gdbpy_convert_exception (except);
	  goto error;
	}
      END_CATCH
    }

  return EXT_LANG_BT_OK;

 error:
  return EXT_LANG_BT_ERROR;
}


/* Helper function to loop over variables provided by the
   "frame_locals" Python API.  Elements in the iterable must conform
   to the "Symbol Value" interface.  ITER is the Python iterable
   object, OUT is the output stream, INDENT is whether we should
   indent the output (for CLI), ARGS_TYPE is an enumerator describing
   the argument format, PRINT_ARGS_FIELD is flag which indicates
   whether to output the ARGS field in the case of
   -stack-list-variables and FRAME is the backing frame.  Returns
   EXT_LANG_BT_ERROR on error, with any GDB exceptions converted to a Python
   exception, or EXT_LANG_BT_OK on success.  */

static enum ext_lang_bt_status
enumerate_locals (PyObject *iter,
		  struct ui_out *out,
		  int indent,
		  enum ext_lang_frame_args args_type,
		  int print_args_field,
		  struct frame_info *frame)
{
  PyObject *item;
  struct value_print_options opts;

  get_user_print_options (&opts);
  opts.deref_ref = 1;

  while ((item = PyIter_Next (iter)))
    {
      const struct language_defn *language;
      char *sym_name;
      struct value *val;
      enum ext_lang_bt_status success = EXT_LANG_BT_ERROR;
      struct symbol *sym;
      struct block *sym_block;
      int local_indent = 8 + (8 * indent);
      struct cleanup *locals_cleanups;

      locals_cleanups = make_cleanup_py_decref (item);

      success = extract_sym (item, &sym_name, &sym, &sym_block, &language);
      if (success == EXT_LANG_BT_ERROR)
	{
	  do_cleanups (locals_cleanups);
	  goto error;
	}

      make_cleanup (xfree, sym_name);

      success = extract_value (item, &val);
      if (success == EXT_LANG_BT_ERROR)
	{
	  do_cleanups (locals_cleanups);
	  goto error;
	}

      if (sym != NULL && ui_out_is_mi_like_p (out)
	  && ! mi_should_print (sym, MI_PRINT_LOCALS))
	{
	  do_cleanups (locals_cleanups);
	  continue;
	}

      /* If the object did not provide a value, read it.  */
      if (val == NULL)
	{
	  TRY
	    {
	      val = read_var_value (sym, sym_block, frame);
	    }
	  CATCH (except, RETURN_MASK_ERROR)
	    {
	      gdbpy_convert_exception (except);
	      do_cleanups (locals_cleanups);
	      goto error;
	    }
	  END_CATCH
	}

      /* With PRINT_NO_VALUES, MI does not emit a tuple normally as
	 each output contains only one field.  The exception is
	 -stack-list-variables, which always provides a tuple.  */
      if (ui_out_is_mi_like_p (out))
	{
	  if (print_args_field || args_type != NO_VALUES)
	    make_cleanup_ui_out_tuple_begin_end (out, NULL);
	}
      TRY
	{
	  if (! ui_out_is_mi_like_p (out))
	    {
	      /* If the output is not MI we indent locals.  */
	      ui_out_spaces (out, local_indent);
	    }

	  ui_out_field_string (out, "name", sym_name);

	  if (! ui_out_is_mi_like_p (out))
	    ui_out_text (out, " = ");
	}
      CATCH (except, RETURN_MASK_ERROR)
	{
	  gdbpy_convert_exception (except);
	  do_cleanups (locals_cleanups);
	  goto error;
	}
      END_CATCH

      if (args_type == MI_PRINT_SIMPLE_VALUES)
	{
	  if (py_print_type (out, val) == EXT_LANG_BT_ERROR)
	    {
	      do_cleanups (locals_cleanups);
	      goto error;
	    }
	}

      /* CLI always prints values for locals.  MI uses the
	 simple/no/all system.  */
      if (! ui_out_is_mi_like_p (out))
	{
	  int val_indent = (indent + 1) * 4;

	  if (py_print_value (out, val, &opts, val_indent, args_type,
			      language) == EXT_LANG_BT_ERROR)
	    {
	      do_cleanups (locals_cleanups);
	      goto error;
	    }
	}
      else
	{
	  if (args_type != NO_VALUES)
	    {
	      if (py_print_value (out, val, &opts, 0, args_type,
				  language) == EXT_LANG_BT_ERROR)
		{
		  do_cleanups (locals_cleanups);
		  goto error;
		}
	    }
	}

      do_cleanups (locals_cleanups);

      TRY
	{
	  ui_out_text (out, "\n");
	}
      CATCH (except, RETURN_MASK_ERROR)
	{
	  gdbpy_convert_exception (except);
	  goto error;
	}
      END_CATCH
    }

  if (item == NULL && PyErr_Occurred ())
    goto error;

  return EXT_LANG_BT_OK;

 error:
  return EXT_LANG_BT_ERROR;
}

/*  Helper function for -stack-list-variables.  Returns EXT_LANG_BT_ERROR on
    error, or EXT_LANG_BT_OK on success.  */

static enum ext_lang_bt_status
py_mi_print_variables (PyObject *filter, struct ui_out *out,
		       struct value_print_options *opts,
		       enum ext_lang_frame_args args_type,
		       struct frame_info *frame)
{
  struct cleanup *old_chain;
  PyObject *args_iter;
  PyObject *locals_iter;

  args_iter = get_py_iter_from_func (filter, "frame_args");
  old_chain = make_cleanup_py_xdecref (args_iter);
  if (args_iter == NULL)
    goto error;

  locals_iter = get_py_iter_from_func (filter, "frame_locals");
  if (locals_iter == NULL)
    goto error;

  make_cleanup_py_decref (locals_iter);
  make_cleanup_ui_out_list_begin_end (out, "variables");

  if (args_iter != Py_None)
    if (enumerate_args (args_iter, out, args_type, 1, frame)
	== EXT_LANG_BT_ERROR)
      goto error;

  if (locals_iter != Py_None)
    if (enumerate_locals (locals_iter, out, 1, args_type, 1, frame)
	== EXT_LANG_BT_ERROR)
      goto error;

  do_cleanups (old_chain);
  return EXT_LANG_BT_OK;

 error:
  do_cleanups (old_chain);
  return EXT_LANG_BT_ERROR;
}

/* Helper function for printing locals.  This function largely just
   creates the wrapping tuple, and calls enumerate_locals.  Returns
   EXT_LANG_BT_ERROR on error, or EXT_LANG_BT_OK on success.  */

static enum ext_lang_bt_status
py_print_locals (PyObject *filter,
		 struct ui_out *out,
		 enum ext_lang_frame_args args_type,
		 int indent,
		 struct frame_info *frame)
{
  PyObject *locals_iter = get_py_iter_from_func (filter,
						 "frame_locals");
  struct cleanup *old_chain = make_cleanup_py_xdecref (locals_iter);

  if (locals_iter == NULL)
    goto locals_error;

  make_cleanup_ui_out_list_begin_end (out, "locals");

  if (locals_iter != Py_None)
    if (enumerate_locals (locals_iter, out, indent, args_type,
			  0, frame) == EXT_LANG_BT_ERROR)
      goto locals_error;

  do_cleanups (old_chain);
  return EXT_LANG_BT_OK;

 locals_error:
  do_cleanups (old_chain);
  return EXT_LANG_BT_ERROR;
}

/* Helper function for printing frame arguments.  This function
   largely just creates the wrapping tuple, and calls enumerate_args.
   Returns EXT_LANG_BT_ERROR on error, with any GDB exceptions converted to
   a Python exception, or EXT_LANG_BT_OK on success.  */

static enum ext_lang_bt_status
py_print_args (PyObject *filter,
	       struct ui_out *out,
	       enum ext_lang_frame_args args_type,
	       struct frame_info *frame)
{
  PyObject *args_iter  = get_py_iter_from_func (filter, "frame_args");
  struct cleanup *old_chain = make_cleanup_py_xdecref (args_iter);

  if (args_iter == NULL)
    goto args_error;

  make_cleanup_ui_out_list_begin_end (out, "args");

  TRY
    {
      annotate_frame_args ();
      if (! ui_out_is_mi_like_p (out))
	ui_out_text (out, " (");
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      gdbpy_convert_exception (except);
      goto args_error;
    }
  END_CATCH

  if (args_iter != Py_None)
    if (enumerate_args (args_iter, out, args_type, 0, frame)
	== EXT_LANG_BT_ERROR)
      goto args_error;

  TRY
    {
      if (! ui_out_is_mi_like_p (out))
	ui_out_text (out, ")");
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      gdbpy_convert_exception (except);
      goto args_error;
    }
  END_CATCH

  do_cleanups (old_chain);
  return EXT_LANG_BT_OK;

 args_error:
  do_cleanups (old_chain);
  return EXT_LANG_BT_ERROR;
}

/*  Print a single frame to the designated output stream, detecting
    whether the output is MI or console, and formatting the output
    according to the conventions of that protocol.  FILTER is the
    frame-filter associated with this frame.  FLAGS is an integer
    describing the various print options.  The FLAGS variables is
    described in "apply_frame_filter" function.  ARGS_TYPE is an
    enumerator describing the argument format.  OUT is the output
    stream to print, INDENT is the level of indention for this frame
    (in the case of elided frames), and LEVELS_PRINTED is a hash-table
    containing all the frames level that have already been printed.
    If a frame level has been printed, do not print it again (in the
    case of elided frames).  Returns EXT_LANG_BT_ERROR on error, with any
    GDB exceptions converted to a Python exception, or EXT_LANG_BT_COMPLETED
    on success.  It can also throw an exception RETURN_QUIT.  */

static enum ext_lang_bt_status
py_print_frame (PyObject *filter, int flags,
		enum ext_lang_frame_args args_type,
		struct ui_out *out, int indent, htab_t levels_printed)
{
  int has_addr = 0;
  CORE_ADDR address = 0;
  struct gdbarch *gdbarch = NULL;
  struct frame_info *frame = NULL;
  struct cleanup *cleanup_stack;
  struct value_print_options opts;
  PyObject *py_inf_frame;
  int print_level, print_frame_info, print_args, print_locals;

  /* Extract print settings from FLAGS.  */
  print_level = (flags & PRINT_LEVEL) ? 1 : 0;
  print_frame_info = (flags & PRINT_FRAME_INFO) ? 1 : 0;
  print_args = (flags & PRINT_ARGS) ? 1 : 0;
  print_locals = (flags & PRINT_LOCALS) ? 1 : 0;

  get_user_print_options (&opts);

  /* Get the underlying frame.  This is needed to determine GDB
  architecture, and also, in the cases of frame variables/arguments to
  read them if they returned filter object requires us to do so.  */
  py_inf_frame = PyObject_CallMethod (filter, "inferior_frame", NULL);
  if (py_inf_frame == NULL)
    return EXT_LANG_BT_ERROR;

  frame = frame_object_to_frame_info (py_inf_frame);;

  Py_DECREF (py_inf_frame);

  if (frame == NULL)
    return EXT_LANG_BT_ERROR;

  TRY
    {
      gdbarch = get_frame_arch (frame);
    }
  CATCH (except, RETURN_MASK_ERROR)
    {
      gdbpy_convert_exception (except);
      return EXT_LANG_BT_ERROR;
    }
  END_CATCH

  /* stack-list-variables.  */
  if (print_locals && print_args && ! print_frame_info)
    {
      if (py_mi_print_variables (filter, out, &opts,
				 args_type, frame) == EXT_LANG_BT_ERROR)
	return EXT_LANG_BT_ERROR;
      return EXT_LANG_BT_COMPLETED;
    }

  cleanup_stack = make_cleanup (null_cleanup, NULL);

  /* -stack-list-locals does not require a
     wrapping frame attribute.  */
  if (print_frame_info || (print_args && ! print_locals))
    make_cleanup_ui_out_tuple_begin_end (out, "frame");

  if (print_frame_info)
    {
      /* Elided frames are also printed with this function (recursively)
	 and are printed with indention.  */
      if (indent > 0)
	{
	  TRY
	    {
	      ui_out_spaces (out, indent*4);
	    }
	  CATCH (except, RETURN_MASK_ERROR)
	    {
	      gdbpy_convert_exception (except);
	      do_cleanups (cleanup_stack);
	      return EXT_LANG_BT_ERROR;
	    }
	  END_CATCH
	}

      /* The address is required for frame annotations, and also for
	 address printing.  */
      if (PyObject_HasAttrString (filter, "address"))
	{
	  PyObject *paddr = PyObject_CallMethod (filter, "address", NULL);

	  if (paddr == NULL)
	    {
	      do_cleanups (cleanup_stack);
	      return EXT_LANG_BT_ERROR;
	    }

	  if (paddr != Py_None)
	    {
	      address = PyLong_AsLong (paddr);
	      has_addr = 1;
	    }
	  Py_DECREF (paddr);
	}
    }

  /* Print frame level.  MI does not require the level if
     locals/variables only are being printed.  */
  if ((print_frame_info || print_args) && print_level)
    {
      struct frame_info **slot;
      int level;

      slot = (struct frame_info **) htab_find_slot (levels_printed,
						    frame, INSERT);
      TRY
	{
	  level = frame_relative_level (frame);

	  /* Check if this frame has already been printed (there are cases
	     where elided synthetic dummy-frames have to 'borrow' the frame
	     architecture from the eliding frame.  If that is the case, do
	     not print 'level', but print spaces.  */
	  if (*slot == frame)
	    ui_out_field_skip (out, "level");
	  else
	    {
	      *slot = frame;
	      annotate_frame_begin (print_level ? level : 0,
				    gdbarch, address);
	      ui_out_text (out, "#");
	      ui_out_field_fmt_int (out, 2, ui_left, "level",
				    level);
	    }
	}
      CATCH (except, RETURN_MASK_ERROR)
	{
	  gdbpy_convert_exception (except);
	  do_cleanups (cleanup_stack);
	  return EXT_LANG_BT_ERROR;
	}
      END_CATCH
    }

  if (print_frame_info)
    {
      /* Print address to the address field.  If an address is not provided,
	 print nothing.  */
      if (opts.addressprint && has_addr)
	{
	  TRY
	    {
	      annotate_frame_address ();
	      ui_out_field_core_addr (out, "addr", gdbarch, address);
	      annotate_frame_address_end ();
	      ui_out_text (out, " in ");
	    }
	  CATCH (except, RETURN_MASK_ERROR)
	    {
	      gdbpy_convert_exception (except);
	      do_cleanups (cleanup_stack);
	      return EXT_LANG_BT_ERROR;
	    }
	  END_CATCH
	}

      /* Print frame function name.  */
      if (PyObject_HasAttrString (filter, "function"))
	{
	  PyObject *py_func = PyObject_CallMethod (filter, "function", NULL);
	  struct cleanup *py_func_cleanup;
	  const char *function = NULL;

	  if (py_func == NULL)
	    {
	      do_cleanups (cleanup_stack);
	      return EXT_LANG_BT_ERROR;
	    }
	  py_func_cleanup = make_cleanup_py_decref (py_func);

	  if (gdbpy_is_string (py_func))
	    {
	      char *function_to_free;

	      function = function_to_free =
		python_string_to_host_string (py_func);

	      if (function == NULL)
		{
		  do_cleanups (cleanup_stack);
		  return EXT_LANG_BT_ERROR;
		}
	      make_cleanup (xfree, function_to_free);
	    }
	  else if (PyLong_Check (py_func))
	    {
	      CORE_ADDR addr = PyLong_AsUnsignedLongLong (py_func);
	      struct bound_minimal_symbol msymbol;

	      if (PyErr_Occurred ())
		{
		  do_cleanups (cleanup_stack);
		  return EXT_LANG_BT_ERROR;
		}

	      msymbol = lookup_minimal_symbol_by_pc (addr);
	      if (msymbol.minsym != NULL)
		function = MSYMBOL_PRINT_NAME (msymbol.minsym);
	    }
	  else if (py_func != Py_None)
	    {
	      PyErr_SetString (PyExc_RuntimeError,
			       _("FrameDecorator.function: expecting a " \
				 "String, integer or None."));
	      do_cleanups (cleanup_stack);
	      return EXT_LANG_BT_ERROR;
	    }

	  TRY
	    {
	      annotate_frame_function_name ();
	      if (function == NULL)
		ui_out_field_skip (out, "func");
	      else
		ui_out_field_string (out, "func", function);
	    }
	  CATCH (except, RETURN_MASK_ERROR)
	    {
	      gdbpy_convert_exception (except);
	      do_cleanups (cleanup_stack);
	      return EXT_LANG_BT_ERROR;
	    }
	  END_CATCH

	  do_cleanups (py_func_cleanup);
	}
    }


  /* Frame arguments.  Check the result, and error if something went
     wrong.  */
  if (print_args)
    {
      if (py_print_args (filter, out, args_type, frame) == EXT_LANG_BT_ERROR)
	{
	  do_cleanups (cleanup_stack);
	  return EXT_LANG_BT_ERROR;
	}
    }

  /* File name/source/line number information.  */
  if (print_frame_info)
    {
      TRY
	{
	  annotate_frame_source_begin ();
	}
      CATCH (except, RETURN_MASK_ERROR)
	{
	  gdbpy_convert_exception (except);
	  do_cleanups (cleanup_stack);
	  return EXT_LANG_BT_ERROR;
	}
      END_CATCH

      if (PyObject_HasAttrString (filter, "filename"))
	{
	  PyObject *py_fn = PyObject_CallMethod (filter, "filename", NULL);
	  struct cleanup *py_fn_cleanup;

	  if (py_fn == NULL)
	    {
	      do_cleanups (cleanup_stack);
	      return EXT_LANG_BT_ERROR;
	    }
	  py_fn_cleanup = make_cleanup_py_decref (py_fn);

	  if (py_fn != Py_None)
	    {
	      char *filename = python_string_to_host_string (py_fn);

	      if (filename == NULL)
		{
		  do_cleanups (cleanup_stack);
		  return EXT_LANG_BT_ERROR;
		}

	      make_cleanup (xfree, filename);
	      TRY
		{
		  ui_out_wrap_hint (out, "   ");
		  ui_out_text (out, " at ");
		  annotate_frame_source_file ();
		  ui_out_field_string (out, "file", filename);
		  annotate_frame_source_file_end ();
		}
	      CATCH (except, RETURN_MASK_ERROR)
		{
		  gdbpy_convert_exception (except);
		  do_cleanups (cleanup_stack);
		  return EXT_LANG_BT_ERROR;
		}
	      END_CATCH
	    }
	  do_cleanups (py_fn_cleanup);
	}

      if (PyObject_HasAttrString (filter, "line"))
	{
	  PyObject *py_line = PyObject_CallMethod (filter, "line", NULL);
	  struct cleanup *py_line_cleanup;
	  int line;

	  if (py_line == NULL)
	    {
	      do_cleanups (cleanup_stack);
	      return EXT_LANG_BT_ERROR;
	    }
	  py_line_cleanup = make_cleanup_py_decref (py_line);

	  if (py_line != Py_None)
	    {
	      line = PyLong_AsLong (py_line);
	      TRY
		{
		  ui_out_text (out, ":");
		  annotate_frame_source_line ();
		  ui_out_field_int (out, "line", line);
		}
	      CATCH (except, RETURN_MASK_ERROR)
		{
		  gdbpy_convert_exception (except);
		  do_cleanups (cleanup_stack);
		  return EXT_LANG_BT_ERROR;
		}
	      END_CATCH
	    }
	  do_cleanups (py_line_cleanup);
	}
    }

  /* For MI we need to deal with the "children" list population of
     elided frames, so if MI output detected do not send newline.  */
  if (! ui_out_is_mi_like_p (out))
    {
      TRY
	{
	  annotate_frame_end ();
	  ui_out_text (out, "\n");
	}
      CATCH (except, RETURN_MASK_ERROR)
	{
	  gdbpy_convert_exception (except);
	  do_cleanups (cleanup_stack);
	  return EXT_LANG_BT_ERROR;
	}
      END_CATCH
    }

  if (print_locals)
    {
      if (py_print_locals (filter, out, args_type, indent,
			   frame) == EXT_LANG_BT_ERROR)
	{
	  do_cleanups (cleanup_stack);
	  return EXT_LANG_BT_ERROR;
	}
    }

  {
    PyObject *elided;
    struct cleanup *elided_cleanup;

    /* Finally recursively print elided frames, if any.  */
    elided = get_py_iter_from_func (filter, "elided");
    if (elided == NULL)
      {
	do_cleanups (cleanup_stack);
	return EXT_LANG_BT_ERROR;
      }
    elided_cleanup = make_cleanup_py_decref (elided);

    if (elided != Py_None)
      {
	PyObject *item;

	make_cleanup_ui_out_list_begin_end (out, "children");

	if (! ui_out_is_mi_like_p (out))
	  indent++;

	while ((item = PyIter_Next (elided)))
	  {
	    struct cleanup *item_cleanup = make_cleanup_py_decref (item);

	    enum ext_lang_bt_status success = py_print_frame (item, flags,
							      args_type, out,
							      indent,
							      levels_printed);

	    do_cleanups (item_cleanup);

	    if (success == EXT_LANG_BT_ERROR)
	      {
		do_cleanups (cleanup_stack);
		return EXT_LANG_BT_ERROR;
	      }
	  }
	if (item == NULL && PyErr_Occurred ())
	  {
	    do_cleanups (cleanup_stack);
	    return EXT_LANG_BT_ERROR;
	  }
      }
    do_cleanups (elided_cleanup);
  }

  do_cleanups (cleanup_stack);
  return EXT_LANG_BT_COMPLETED;
}

/* Helper function to initiate frame filter invocation at starting
   frame FRAME.  */

static PyObject *
bootstrap_python_frame_filters (struct frame_info *frame,
				int frame_low, int frame_high)
{
  struct cleanup *cleanups =
    make_cleanup (null_cleanup, NULL);
  PyObject *module, *sort_func, *iterable, *frame_obj, *iterator;
  PyObject *py_frame_low, *py_frame_high;

  frame_obj = frame_info_to_frame_object (frame);
  if (frame_obj == NULL)
    goto error;
  make_cleanup_py_decref (frame_obj);

  module = PyImport_ImportModule ("gdb.frames");
  if (module == NULL)
    goto error;
  make_cleanup_py_decref (module);

  sort_func = PyObject_GetAttrString (module, "execute_frame_filters");
  if (sort_func == NULL)
    goto error;
  make_cleanup_py_decref (sort_func);

  py_frame_low = PyInt_FromLong (frame_low);
  if (py_frame_low == NULL)
    goto error;
  make_cleanup_py_decref (py_frame_low);

  py_frame_high = PyInt_FromLong (frame_high);
  if (py_frame_high == NULL)
    goto error;
  make_cleanup_py_decref (py_frame_high);

  iterable = PyObject_CallFunctionObjArgs (sort_func, frame_obj,
					   py_frame_low,
					   py_frame_high,
					   NULL);
  if (iterable == NULL)
    goto error;

  do_cleanups (cleanups);

  if (iterable != Py_None)
    {
      iterator = PyObject_GetIter (iterable);
      Py_DECREF (iterable);
    }
  else
    {
      return iterable;
    }

  return iterator;

 error:
  do_cleanups (cleanups);
  return NULL;
}

/*  This is the only publicly exported function in this file.  FRAME
    is the source frame to start frame-filter invocation.  FLAGS is an
    integer holding the flags for printing.  The following elements of
    the FRAME_FILTER_FLAGS enum denotes the make-up of FLAGS:
    PRINT_LEVEL is a flag indicating whether to print the frame's
    relative level in the output.  PRINT_FRAME_INFO is a flag that
    indicates whether this function should print the frame
    information, PRINT_ARGS is a flag that indicates whether to print
    frame arguments, and PRINT_LOCALS, likewise, with frame local
    variables.  ARGS_TYPE is an enumerator describing the argument
    format, OUT is the output stream to print.  FRAME_LOW is the
    beginning of the slice of frames to print, and FRAME_HIGH is the
    upper limit of the frames to count.  Returns EXT_LANG_BT_ERROR on error,
    or EXT_LANG_BT_COMPLETED on success.  */

enum ext_lang_bt_status
gdbpy_apply_frame_filter (const struct extension_language_defn *extlang,
			  struct frame_info *frame, int flags,
			  enum ext_lang_frame_args args_type,
			  struct ui_out *out, int frame_low, int frame_high)
{
  struct gdbarch *gdbarch = NULL;
  struct cleanup *cleanups;
  enum ext_lang_bt_status success = EXT_LANG_BT_ERROR;
  PyObject *iterable;
  PyObject *item;
  htab_t levels_printed;

  if (!gdb_python_initialized)
    return EXT_LANG_BT_NO_FILTERS;

  TRY
    {
      gdbarch = get_frame_arch (frame);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      /* Let gdb try to print the stack trace.  */
      return EXT_LANG_BT_NO_FILTERS;
    }
  END_CATCH

  cleanups = ensure_python_env (gdbarch, current_language);

  iterable = bootstrap_python_frame_filters (frame, frame_low, frame_high);

  if (iterable == NULL)
    {
      /* Normally if there is an error GDB prints the exception,
	 abandons the backtrace and exits.  The user can then call "bt
	 no-filters", and get a default backtrace (it would be
	 confusing to automatically start a standard backtrace halfway
	 through a Python filtered backtrace).  However in the case
	 where GDB cannot initialize the frame filters (most likely
	 due to incorrect auto-load paths), GDB has printed nothing.
	 In this case it is OK to print the default backtrace after
	 printing the error message.  GDB returns EXT_LANG_BT_NO_FILTERS
	 here to signify there are no filters after printing the
	 initialization error.  This return code will trigger a
	 default backtrace.  */

      gdbpy_print_stack ();
      do_cleanups (cleanups);
      return EXT_LANG_BT_NO_FILTERS;
    }

  /* If iterable is None, then there are no frame filters registered.
     If this is the case, defer to default GDB printing routines in MI
     and CLI.  */
  make_cleanup_py_decref (iterable);
  if (iterable == Py_None)
    {
      success = EXT_LANG_BT_NO_FILTERS;
      goto done;
    }

  levels_printed = htab_create (20,
				htab_hash_pointer,
				htab_eq_pointer,
				NULL);
  make_cleanup_htab_delete (levels_printed);

  while ((item = PyIter_Next (iterable)))
    {
      struct cleanup *item_cleanup = make_cleanup_py_decref (item);

      success = py_print_frame (item, flags, args_type, out, 0,
				levels_printed);

      do_cleanups (item_cleanup);

      /* Do not exit on error printing a single frame.  Print the
	 error and continue with other frames.  */
      if (success == EXT_LANG_BT_ERROR)
	gdbpy_print_stack ();
    }

  if (item == NULL && PyErr_Occurred ())
    goto error;

 done:
  do_cleanups (cleanups);
  return success;

  /* Exit and abandon backtrace on error, printing the exception that
     is set.  */
 error:
  gdbpy_print_stack ();
  do_cleanups (cleanups);
  return EXT_LANG_BT_ERROR;
}
