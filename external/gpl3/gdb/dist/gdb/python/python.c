/* General python/gdb code

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
#include "arch-utils.h"
#include "command.h"
#include "ui-out.h"
#include "cli/cli-script.h"
#include "gdbcmd.h"
#include "progspace.h"
#include "objfiles.h"
#include "value.h"
#include "language.h"
#include "event-loop.h"
#include "serial.h"
#include "readline/tilde.h"
#include "python.h"
#include "extension-priv.h"
#include "cli/cli-utils.h"
#include <ctype.h>
#include "location.h"
#include "ser-event.h"

/* Declared constants and enum for python stack printing.  */
static const char python_excp_none[] = "none";
static const char python_excp_full[] = "full";
static const char python_excp_message[] = "message";

/* "set python print-stack" choices.  */
static const char *const python_excp_enums[] =
  {
    python_excp_none,
    python_excp_full,
    python_excp_message,
    NULL
  };

/* The exception printing variable.  'full' if we want to print the
   error message and stack, 'none' if we want to print nothing, and
   'message' if we only want to print the error message.  'message' is
   the default.  */
static const char *gdbpy_should_print_stack = python_excp_message;

#ifdef HAVE_PYTHON
/* Forward decls, these are defined later.  */
extern const struct extension_language_script_ops python_extension_script_ops;
extern const struct extension_language_ops python_extension_ops;
#endif

/* The main struct describing GDB's interface to the Python
   extension language.  */
const struct extension_language_defn extension_language_python =
{
  EXT_LANG_PYTHON,
  "python",
  "Python",

  ".py",
  "-gdb.py",

  python_control,

#ifdef HAVE_PYTHON
  &python_extension_script_ops,
  &python_extension_ops
#else
  NULL,
  NULL
#endif
};

#ifdef HAVE_PYTHON

#include "cli/cli-decode.h"
#include "charset.h"
#include "top.h"
#include "solib.h"
#include "python-internal.h"
#include "linespec.h"
#include "source.h"
#include "version.h"
#include "target.h"
#include "gdbthread.h"
#include "interps.h"
#include "event-top.h"
#include "py-ref.h"
#include "py-event.h"

/* True if Python has been successfully initialized, false
   otherwise.  */

int gdb_python_initialized;

extern PyMethodDef python_GdbMethods[];

#ifdef IS_PY3K
extern struct PyModuleDef python_GdbModuleDef;
#endif

PyObject *gdb_module;
PyObject *gdb_python_module;

/* Some string constants we may wish to use.  */
PyObject *gdbpy_to_string_cst;
PyObject *gdbpy_children_cst;
PyObject *gdbpy_display_hint_cst;
PyObject *gdbpy_doc_cst;
PyObject *gdbpy_enabled_cst;
PyObject *gdbpy_value_cst;

/* The GdbError exception.  */
PyObject *gdbpy_gdberror_exc;

/* The `gdb.error' base class.  */
PyObject *gdbpy_gdb_error;

/* The `gdb.MemoryError' exception.  */
PyObject *gdbpy_gdb_memory_error;

static script_sourcer_func gdbpy_source_script;
static objfile_script_sourcer_func gdbpy_source_objfile_script;
static objfile_script_executor_func gdbpy_execute_objfile_script;
static void gdbpy_finish_initialization
  (const struct extension_language_defn *);
static int gdbpy_initialized (const struct extension_language_defn *);
static void gdbpy_eval_from_control_command
  (const struct extension_language_defn *, struct command_line *cmd);
static void gdbpy_start_type_printers (const struct extension_language_defn *,
				       struct ext_lang_type_printers *);
static enum ext_lang_rc gdbpy_apply_type_printers
  (const struct extension_language_defn *,
   const struct ext_lang_type_printers *, struct type *, char **);
static void gdbpy_free_type_printers (const struct extension_language_defn *,
				      struct ext_lang_type_printers *);
static void gdbpy_set_quit_flag (const struct extension_language_defn *);
static int gdbpy_check_quit_flag (const struct extension_language_defn *);
static enum ext_lang_rc gdbpy_before_prompt_hook
  (const struct extension_language_defn *, const char *current_gdb_prompt);

/* The interface between gdb proper and loading of python scripts.  */

const struct extension_language_script_ops python_extension_script_ops =
{
  gdbpy_source_script,
  gdbpy_source_objfile_script,
  gdbpy_execute_objfile_script,
  gdbpy_auto_load_enabled
};

/* The interface between gdb proper and python extensions.  */

const struct extension_language_ops python_extension_ops =
{
  gdbpy_finish_initialization,
  gdbpy_initialized,

  gdbpy_eval_from_control_command,

  gdbpy_start_type_printers,
  gdbpy_apply_type_printers,
  gdbpy_free_type_printers,

  gdbpy_apply_val_pretty_printer,

  gdbpy_apply_frame_filter,

  gdbpy_preserve_values,

  gdbpy_breakpoint_has_cond,
  gdbpy_breakpoint_cond_says_stop,

  gdbpy_set_quit_flag,
  gdbpy_check_quit_flag,

  gdbpy_before_prompt_hook,

  gdbpy_clone_xmethod_worker_data,
  gdbpy_free_xmethod_worker_data,
  gdbpy_get_matching_xmethod_workers,
  gdbpy_get_xmethod_arg_types,
  gdbpy_get_xmethod_result_type,
  gdbpy_invoke_xmethod
};

/* Architecture and language to be used in callbacks from
   the Python interpreter.  */
struct gdbarch *python_gdbarch;
const struct language_defn *python_language;

gdbpy_enter::gdbpy_enter  (struct gdbarch *gdbarch,
			   const struct language_defn *language)
: m_gdbarch (python_gdbarch),
  m_language (python_language)
{
  /* We should not ever enter Python unless initialized.  */
  if (!gdb_python_initialized)
    error (_("Python not initialized"));

  m_previous_active = set_active_ext_lang (&extension_language_python);

  m_state = PyGILState_Ensure ();

  python_gdbarch = gdbarch;
  python_language = language;

  /* Save it and ensure ! PyErr_Occurred () afterwards.  */
  PyErr_Fetch (&m_error_type, &m_error_value, &m_error_traceback);
}

gdbpy_enter::~gdbpy_enter ()
{
  /* Leftover Python error is forbidden by Python Exception Handling.  */
  if (PyErr_Occurred ())
    {
      /* This order is similar to the one calling error afterwards. */
      gdbpy_print_stack ();
      warning (_("internal error: Unhandled Python exception"));
    }

  PyErr_Restore (m_error_type, m_error_value, m_error_traceback);

  PyGILState_Release (m_state);
  python_gdbarch = m_gdbarch;
  python_language = m_language;

  restore_active_ext_lang (m_previous_active);
}

/* Set the quit flag.  */

static void
gdbpy_set_quit_flag (const struct extension_language_defn *extlang)
{
  PyErr_SetInterrupt ();
}

/* Return true if the quit flag has been set, false otherwise.  */

static int
gdbpy_check_quit_flag (const struct extension_language_defn *extlang)
{
  return PyOS_InterruptOccurred ();
}

/* Evaluate a Python command like PyRun_SimpleString, but uses
   Py_single_input which prints the result of expressions, and does
   not automatically print the stack on errors.  */

static int
eval_python_command (const char *command)
{
  PyObject *m, *d;

  m = PyImport_AddModule ("__main__");
  if (m == NULL)
    return -1;

  d = PyModule_GetDict (m);
  if (d == NULL)
    return -1;
  gdbpy_ref<> v (PyRun_StringFlags (command, Py_single_input, d, d, NULL));
  if (v == NULL)
    return -1;

#ifndef IS_PY3K
  if (Py_FlushLine ())
    PyErr_Clear ();
#endif

  return 0;
}

/* Implementation of the gdb "python-interactive" command.  */

static void
python_interactive_command (char *arg, int from_tty)
{
  struct ui *ui = current_ui;
  int err;

  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  arg = skip_spaces (arg);

  gdbpy_enter enter_py (get_current_arch (), current_language);

  if (arg && *arg)
    {
      int len = strlen (arg);
      char *script = (char *) xmalloc (len + 2);

      strcpy (script, arg);
      script[len] = '\n';
      script[len + 1] = '\0';
      err = eval_python_command (script);
      xfree (script);
    }
  else
    {
      err = PyRun_InteractiveLoop (ui->instream, "<stdin>");
      dont_repeat ();
    }

  if (err)
    {
      gdbpy_print_stack ();
      error (_("Error while executing Python code."));
    }
}

/* A wrapper around PyRun_SimpleFile.  FILE is the Python script to run
   named FILENAME.

   On Windows hosts few users would build Python themselves (this is no
   trivial task on this platform), and thus use binaries built by
   someone else instead.  There may happen situation where the Python
   library and GDB are using two different versions of the C runtime
   library.  Python, being built with VC, would use one version of the
   msvcr DLL (Eg. msvcr100.dll), while MinGW uses msvcrt.dll.
   A FILE * from one runtime does not necessarily operate correctly in
   the other runtime.

   To work around this potential issue, we create on Windows hosts the
   FILE object using Python routines, thus making sure that it is
   compatible with the Python library.  */

static void
python_run_simple_file (FILE *file, const char *filename)
{
#ifndef _WIN32

  PyRun_SimpleFile (file, filename);

#else /* _WIN32 */

  /* Because we have a string for a filename, and are using Python to
     open the file, we need to expand any tilde in the path first.  */
  gdb::unique_xmalloc_ptr<char> full_path (tilde_expand (filename));
  gdbpy_ref<> python_file (PyFile_FromString (full_path.get (), (char *) "r"));
  if (python_file == NULL)
    {
      gdbpy_print_stack ();
      error (_("Error while opening file: %s"), full_path.get ());
    }

  PyRun_SimpleFile (PyFile_AsFile (python_file.get ()), filename);

#endif /* _WIN32 */
}

/* Given a command_line, return a command string suitable for passing
   to Python.  Lines in the string are separated by newlines.  The
   return value is allocated using xmalloc and the caller is
   responsible for freeing it.  */

static char *
compute_python_string (struct command_line *l)
{
  struct command_line *iter;
  char *script = NULL;
  int size = 0;
  int here;

  for (iter = l; iter; iter = iter->next)
    size += strlen (iter->line) + 1;

  script = (char *) xmalloc (size + 1);
  here = 0;
  for (iter = l; iter; iter = iter->next)
    {
      int len = strlen (iter->line);

      strcpy (&script[here], iter->line);
      here += len;
      script[here++] = '\n';
    }
  script[here] = '\0';
  return script;
}

/* Take a command line structure representing a 'python' command, and
   evaluate its body using the Python interpreter.  */

static void
gdbpy_eval_from_control_command (const struct extension_language_defn *extlang,
				 struct command_line *cmd)
{
  int ret;
  char *script;

  if (cmd->body_count != 1)
    error (_("Invalid \"python\" block structure."));

  gdbpy_enter enter_py (get_current_arch (), current_language);

  script = compute_python_string (cmd->body_list[0]);
  ret = PyRun_SimpleString (script);
  xfree (script);
  if (ret)
    error (_("Error while executing Python code."));
}

/* Implementation of the gdb "python" command.  */

static void
python_command (char *arg, int from_tty)
{
  gdbpy_enter enter_py (get_current_arch (), current_language);

  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  arg = skip_spaces (arg);
  if (arg && *arg)
    {
      if (PyRun_SimpleString (arg))
	error (_("Error while executing Python code."));
    }
  else
    {
      command_line_up l = get_command_line (python_control, "");

      execute_control_command_untraced (l.get ());
    }
}



/* Transform a gdb parameters's value into a Python value.  May return
   NULL (and set a Python exception) on error.  Helper function for
   get_parameter.  */
PyObject *
gdbpy_parameter_value (enum var_types type, void *var)
{
  switch (type)
    {
    case var_string:
    case var_string_noescape:
    case var_optional_filename:
    case var_filename:
    case var_enum:
      {
	const char *str = *(char **) var;

	if (! str)
	  str = "";
	return host_string_to_python_string (str);
      }

    case var_boolean:
      {
	if (* (int *) var)
	  Py_RETURN_TRUE;
	else
	  Py_RETURN_FALSE;
      }

    case var_auto_boolean:
      {
	enum auto_boolean ab = * (enum auto_boolean *) var;

	if (ab == AUTO_BOOLEAN_TRUE)
	  Py_RETURN_TRUE;
	else if (ab == AUTO_BOOLEAN_FALSE)
	  Py_RETURN_FALSE;
	else
	  Py_RETURN_NONE;
      }

    case var_integer:
      if ((* (int *) var) == INT_MAX)
	Py_RETURN_NONE;
      /* Fall through.  */
    case var_zinteger:
      return PyLong_FromLong (* (int *) var);

    case var_uinteger:
      {
	unsigned int val = * (unsigned int *) var;

	if (val == UINT_MAX)
	  Py_RETURN_NONE;
	return PyLong_FromUnsignedLong (val);
      }
    }

  return PyErr_Format (PyExc_RuntimeError,
		       _("Programmer error: unhandled type."));
}

/* A Python function which returns a gdb parameter's value as a Python
   value.  */

static PyObject *
gdbpy_parameter (PyObject *self, PyObject *args)
{
  struct gdb_exception except = exception_none;
  struct cmd_list_element *alias, *prefix, *cmd;
  const char *arg;
  char *newarg;
  int found = -1;

  if (! PyArg_ParseTuple (args, "s", &arg))
    return NULL;

  newarg = concat ("show ", arg, (char *) NULL);

  TRY
    {
      found = lookup_cmd_composition (newarg, &alias, &prefix, &cmd);
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      except = ex;
    }
  END_CATCH

  xfree (newarg);
  GDB_PY_HANDLE_EXCEPTION (except);
  if (!found)
    return PyErr_Format (PyExc_RuntimeError,
			 _("Could not find parameter `%s'."), arg);

  if (! cmd->var)
    return PyErr_Format (PyExc_RuntimeError,
			 _("`%s' is not a parameter."), arg);
  return gdbpy_parameter_value (cmd->var_type, cmd->var);
}

/* Wrapper for target_charset.  */

static PyObject *
gdbpy_target_charset (PyObject *self, PyObject *args)
{
  const char *cset = target_charset (python_gdbarch);

  return PyUnicode_Decode (cset, strlen (cset), host_charset (), NULL);
}

/* Wrapper for target_wide_charset.  */

static PyObject *
gdbpy_target_wide_charset (PyObject *self, PyObject *args)
{
  const char *cset = target_wide_charset (python_gdbarch);

  return PyUnicode_Decode (cset, strlen (cset), host_charset (), NULL);
}

/* A Python function which evaluates a string using the gdb CLI.  */

static PyObject *
execute_gdb_command (PyObject *self, PyObject *args, PyObject *kw)
{
  const char *arg;
  PyObject *from_tty_obj = NULL, *to_string_obj = NULL;
  int from_tty, to_string;
  static const char *keywords[] = { "command", "from_tty", "to_string", NULL };

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|O!O!", keywords, &arg,
					&PyBool_Type, &from_tty_obj,
					&PyBool_Type, &to_string_obj))
    return NULL;

  from_tty = 0;
  if (from_tty_obj)
    {
      int cmp = PyObject_IsTrue (from_tty_obj);
      if (cmp < 0)
	return NULL;
      from_tty = cmp;
    }

  to_string = 0;
  if (to_string_obj)
    {
      int cmp = PyObject_IsTrue (to_string_obj);
      if (cmp < 0)
	return NULL;
      to_string = cmp;
    }

  std::string to_string_res;

  TRY
    {
      /* Copy the argument text in case the command modifies it.  */
      std::string copy (arg);
      struct interp *interp;

      scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

      scoped_restore save_uiout = make_scoped_restore (&current_uiout);

      /* Use the console interpreter uiout to have the same print format
	for console or MI.  */
      interp = interp_lookup (current_ui, "console");
      current_uiout = interp_ui_out (interp);

      scoped_restore preventer = prevent_dont_repeat ();
      if (to_string)
	to_string_res = execute_command_to_string (&copy[0], from_tty);
      else
	execute_command (&copy[0], from_tty);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }
  END_CATCH

  /* Do any commands attached to breakpoint we stopped at.  */
  bpstat_do_actions ();

  if (to_string)
    return PyString_FromString (to_string_res.c_str ());
  Py_RETURN_NONE;
}

/* Implementation of gdb.solib_name (Long) -> String.
   Returns the name of the shared library holding a given address, or None.  */

static PyObject *
gdbpy_solib_name (PyObject *self, PyObject *args)
{
  char *soname;
  PyObject *str_obj;
  gdb_py_ulongest pc;

  if (!PyArg_ParseTuple (args, GDB_PY_LLU_ARG, &pc))
    return NULL;

  soname = solib_name_from_address (current_program_space, pc);
  if (soname)
    str_obj = host_string_to_python_string (soname);
  else
    {
      str_obj = Py_None;
      Py_INCREF (Py_None);
    }

  return str_obj;
}

/* A Python function which is a wrapper for decode_line_1.  */

static PyObject *
gdbpy_decode_line (PyObject *self, PyObject *args)
{
  struct gdb_exception except = exception_none;
  struct symtabs_and_lines sals = { NULL, 0 }; /* Initialize to
						  appease gcc.  */
  struct symtab_and_line sal;
  char *arg = NULL;
  struct cleanup *cleanups;
  gdbpy_ref<> result;
  gdbpy_ref<> unparsed;
  event_location_up location;

  if (! PyArg_ParseTuple (args, "|s", &arg))
    return NULL;

  cleanups = make_cleanup (null_cleanup, NULL);

  sals.sals = NULL;

  if (arg != NULL)
    location = string_to_event_location_basic (&arg, python_language);

  TRY
    {
      if (location != NULL)
	sals = decode_line_1 (location.get (), 0, NULL, NULL, 0);
      else
	{
	  set_default_source_symtab_and_line ();
	  sal = get_current_source_symtab_and_line ();
	  sals.sals = &sal;
	  sals.nelts = 1;
	}
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      except = ex;
    }
  END_CATCH

  if (sals.sals != NULL && sals.sals != &sal)
    make_cleanup (xfree, sals.sals);

  if (except.reason < 0)
    {
      do_cleanups (cleanups);
      /* We know this will always throw.  */
      gdbpy_convert_exception (except);
      return NULL;
    }

  if (sals.nelts)
    {
      int i;

      result.reset (PyTuple_New (sals.nelts));
      if (result == NULL)
	{
	  do_cleanups (cleanups);
	  return NULL;
	}
      for (i = 0; i < sals.nelts; ++i)
	{
	  PyObject *obj;

	  obj = symtab_and_line_to_sal_object (sals.sals[i]);
	  if (! obj)
	    {
	      do_cleanups (cleanups);
	      return NULL;
	    }

	  PyTuple_SetItem (result.get (), i, obj);
	}
    }
  else
    {
      result.reset (Py_None);
      Py_INCREF (Py_None);
    }

  gdbpy_ref<> return_result (PyTuple_New (2));
  if (return_result == NULL)
    {
      do_cleanups (cleanups);
      return NULL;
    }

  if (arg != NULL && strlen (arg) > 0)
    {
      unparsed.reset (PyString_FromString (arg));
      if (unparsed == NULL)
	{
	  do_cleanups (cleanups);
	  return NULL;
	}
    }
  else
    {
      unparsed.reset (Py_None);
      Py_INCREF (Py_None);
    }

  PyTuple_SetItem (return_result.get (), 0, unparsed.release ());
  PyTuple_SetItem (return_result.get (), 1, result.release ());

  do_cleanups (cleanups);

  return return_result.release ();
}

/* Parse a string and evaluate it as an expression.  */
static PyObject *
gdbpy_parse_and_eval (PyObject *self, PyObject *args)
{
  const char *expr_str;
  struct value *result = NULL;

  if (!PyArg_ParseTuple (args, "s", &expr_str))
    return NULL;

  TRY
    {
      result = parse_and_eval (expr_str);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }
  END_CATCH

  return value_to_value_object (result);
}

/* Implementation of gdb.find_pc_line function.
   Returns the gdb.Symtab_and_line object corresponding to a PC value.  */

static PyObject *
gdbpy_find_pc_line (PyObject *self, PyObject *args)
{
  gdb_py_ulongest pc_llu;
  PyObject *result = NULL; /* init for gcc -Wall */

  if (!PyArg_ParseTuple (args, GDB_PY_LLU_ARG, &pc_llu))
    return NULL;

  TRY
    {
      struct symtab_and_line sal;
      CORE_ADDR pc;

      pc = (CORE_ADDR) pc_llu;
      sal = find_pc_line (pc, 0);
      result = symtab_and_line_to_sal_object (sal);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }
  END_CATCH

  return result;
}

/* Implementation of gdb.invalidate_cached_frames.  */

static PyObject *
gdbpy_invalidate_cached_frames (PyObject *self, PyObject *args)
{
  reinit_frame_cache ();
  Py_RETURN_NONE;
}

/* Read a file as Python code.
   This is the extension_language_script_ops.script_sourcer "method".
   FILE is the file to load.  FILENAME is name of the file FILE.
   This does not throw any errors.  If an exception occurs python will print
   the traceback and clear the error indicator.  */

static void
gdbpy_source_script (const struct extension_language_defn *extlang,
		     FILE *file, const char *filename)
{
  gdbpy_enter enter_py (get_current_arch (), current_language);
  python_run_simple_file (file, filename);
}



/* Posting and handling events.  */

/* A single event.  */
struct gdbpy_event
{
  /* The Python event.  This is just a callable object.  */
  PyObject *event;
  /* The next event.  */
  struct gdbpy_event *next;
};

/* All pending events.  */
static struct gdbpy_event *gdbpy_event_list;
/* The final link of the event list.  */
static struct gdbpy_event **gdbpy_event_list_end;

/* So that we can wake up the main thread even when it is blocked in
   poll().  */
static struct serial_event *gdbpy_serial_event;

/* The file handler callback.  This reads from the internal pipe, and
   then processes the Python event queue.  This will always be run in
   the main gdb thread.  */

static void
gdbpy_run_events (int error, gdb_client_data client_data)
{
  gdbpy_enter enter_py (get_current_arch (), current_language);

  /* Clear the event fd.  Do this before flushing the events list, so
     that any new event post afterwards is sure to re-awake the event
     loop.  */
  serial_event_clear (gdbpy_serial_event);

  while (gdbpy_event_list)
    {
      /* Dispatching the event might push a new element onto the event
	 loop, so we update here "atomically enough".  */
      struct gdbpy_event *item = gdbpy_event_list;
      gdbpy_event_list = gdbpy_event_list->next;
      if (gdbpy_event_list == NULL)
	gdbpy_event_list_end = &gdbpy_event_list;

      /* Ignore errors.  */
      gdbpy_ref<> call_result (PyObject_CallObject (item->event, NULL));
      if (call_result == NULL)
	PyErr_Clear ();

      Py_DECREF (item->event);
      xfree (item);
    }
}

/* Submit an event to the gdb thread.  */
static PyObject *
gdbpy_post_event (PyObject *self, PyObject *args)
{
  struct gdbpy_event *event;
  PyObject *func;
  int wakeup;

  if (!PyArg_ParseTuple (args, "O", &func))
    return NULL;

  if (!PyCallable_Check (func))
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Posted event is not callable"));
      return NULL;
    }

  Py_INCREF (func);

  /* From here until the end of the function, we have the GIL, so we
     can operate on our global data structures without worrying.  */
  wakeup = gdbpy_event_list == NULL;

  event = XNEW (struct gdbpy_event);
  event->event = func;
  event->next = NULL;
  *gdbpy_event_list_end = event;
  gdbpy_event_list_end = &event->next;

  /* Wake up gdb when needed.  */
  if (wakeup)
    serial_event_set (gdbpy_serial_event);

  Py_RETURN_NONE;
}

/* Initialize the Python event handler.  */
static int
gdbpy_initialize_events (void)
{
  gdbpy_event_list_end = &gdbpy_event_list;

  gdbpy_serial_event = make_serial_event ();
  add_file_handler (serial_event_fd (gdbpy_serial_event),
		    gdbpy_run_events, NULL);

  return 0;
}



/* This is the extension_language_ops.before_prompt "method".  */

static enum ext_lang_rc
gdbpy_before_prompt_hook (const struct extension_language_defn *extlang,
			  const char *current_gdb_prompt)
{
  if (!gdb_python_initialized)
    return EXT_LANG_RC_NOP;

  gdbpy_enter enter_py (get_current_arch (), current_language);

  if (!evregpy_no_listeners_p (gdb_py_events.before_prompt)
      && evpy_emit_event (NULL, gdb_py_events.before_prompt) < 0)
    return EXT_LANG_RC_ERROR;

  if (gdb_python_module
      && PyObject_HasAttrString (gdb_python_module, "prompt_hook"))
    {
      gdbpy_ref<> hook (PyObject_GetAttrString (gdb_python_module,
						"prompt_hook"));
      if (hook == NULL)
	{
	  gdbpy_print_stack ();
	  return EXT_LANG_RC_ERROR;
	}

      if (PyCallable_Check (hook.get ()))
	{
	  gdbpy_ref<> current_prompt (PyString_FromString (current_gdb_prompt));
	  if (current_prompt == NULL)
	    {
	      gdbpy_print_stack ();
	      return EXT_LANG_RC_ERROR;
	    }

	  gdbpy_ref<> result
	    (PyObject_CallFunctionObjArgs (hook.get (), current_prompt.get (),
					   NULL));
	  if (result == NULL)
	    {
	      gdbpy_print_stack ();
	      return EXT_LANG_RC_ERROR;
	    }

	  /* Return type should be None, or a String.  If it is None,
	     fall through, we will not set a prompt.  If it is a
	     string, set  PROMPT.  Anything else, set an exception.  */
	  if (result != Py_None && ! PyString_Check (result.get ()))
	    {
	      PyErr_Format (PyExc_RuntimeError,
			    _("Return from prompt_hook must " \
			      "be either a Python string, or None"));
	      gdbpy_print_stack ();
	      return EXT_LANG_RC_ERROR;
	    }

	  if (result != Py_None)
	    {
	      gdb::unique_xmalloc_ptr<char>
		prompt (python_string_to_host_string (result.get ()));

	      if (prompt == NULL)
		{
		  gdbpy_print_stack ();
		  return EXT_LANG_RC_ERROR;
		}

	      set_prompt (prompt.get ());
	      return EXT_LANG_RC_OK;
	    }
	}
    }

  return EXT_LANG_RC_NOP;
}



/* Printing.  */

/* A python function to write a single string using gdb's filtered
   output stream .  The optional keyword STREAM can be used to write
   to a particular stream.  The default stream is to gdb_stdout.  */

static PyObject *
gdbpy_write (PyObject *self, PyObject *args, PyObject *kw)
{
  const char *arg;
  static const char *keywords[] = { "text", "stream", NULL };
  int stream_type = 0;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|i", keywords, &arg,
					&stream_type))
    return NULL;

  TRY
    {
      switch (stream_type)
        {
        case 1:
          {
	    fprintf_filtered (gdb_stderr, "%s", arg);
	    break;
          }
        case 2:
          {
	    fprintf_filtered (gdb_stdlog, "%s", arg);
	    break;
          }
        default:
          fprintf_filtered (gdb_stdout, "%s", arg);
        }
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }
  END_CATCH

  Py_RETURN_NONE;
}

/* A python function to flush a gdb stream.  The optional keyword
   STREAM can be used to flush a particular stream.  The default stream
   is gdb_stdout.  */

static PyObject *
gdbpy_flush (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "stream", NULL };
  int stream_type = 0;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "|i", keywords,
					&stream_type))
    return NULL;

  switch (stream_type)
    {
    case 1:
      {
	gdb_flush (gdb_stderr);
	break;
      }
    case 2:
      {
	gdb_flush (gdb_stdlog);
	break;
      }
    default:
      gdb_flush (gdb_stdout);
    }

  Py_RETURN_NONE;
}

/* Return non-zero if print-stack is not "none".  */

int
gdbpy_print_python_errors_p (void)
{
  return gdbpy_should_print_stack != python_excp_none;
}

/* Print a python exception trace, print just a message, or print
   nothing and clear the python exception, depending on
   gdbpy_should_print_stack.  Only call this if a python exception is
   set.  */
void
gdbpy_print_stack (void)
{

  /* Print "none", just clear exception.  */
  if (gdbpy_should_print_stack == python_excp_none)
    {
      PyErr_Clear ();
    }
  /* Print "full" message and backtrace.  */
  else if (gdbpy_should_print_stack == python_excp_full)
    {
      PyErr_Print ();
      /* PyErr_Print doesn't necessarily end output with a newline.
	 This works because Python's stdout/stderr is fed through
	 printf_filtered.  */
      TRY
	{
	  begin_line ();
	}
      CATCH (except, RETURN_MASK_ALL)
	{
	}
      END_CATCH
    }
  /* Print "message", just error print message.  */
  else
    {
      PyObject *ptype, *pvalue, *ptraceback;

      PyErr_Fetch (&ptype, &pvalue, &ptraceback);

      /* Fetch the error message contained within ptype, pvalue.  */
      gdb::unique_xmalloc_ptr<char>
	msg (gdbpy_exception_to_string (ptype, pvalue));
      gdb::unique_xmalloc_ptr<char> type (gdbpy_obj_to_string (ptype));

      TRY
	{
	  if (msg == NULL)
	    {
	      /* An error occurred computing the string representation of the
		 error message.  */
	      fprintf_filtered (gdb_stderr,
				_("Error occurred computing Python error" \
				  "message.\n"));
	    }
	  else
	    fprintf_filtered (gdb_stderr, "Python Exception %s %s: \n",
			      type.get (), msg.get ());
	}
      CATCH (except, RETURN_MASK_ALL)
	{
	}
      END_CATCH

      Py_XDECREF (ptype);
      Py_XDECREF (pvalue);
      Py_XDECREF (ptraceback);
    }
}



/* Return the current Progspace.
   There always is one.  */

static PyObject *
gdbpy_get_current_progspace (PyObject *unused1, PyObject *unused2)
{
  PyObject *result;

  result = pspace_to_pspace_object (current_program_space);
  if (result)
    Py_INCREF (result);
  return result;
}

/* Return a sequence holding all the Progspaces.  */

static PyObject *
gdbpy_progspaces (PyObject *unused1, PyObject *unused2)
{
  struct program_space *ps;

  gdbpy_ref<> list (PyList_New (0));
  if (list == NULL)
    return NULL;

  ALL_PSPACES (ps)
  {
    PyObject *item = pspace_to_pspace_object (ps);

    if (!item || PyList_Append (list.get (), item) == -1)
      return NULL;
  }

  return list.release ();
}



/* The "current" objfile.  This is set when gdb detects that a new
   objfile has been loaded.  It is only set for the duration of a call to
   gdbpy_source_objfile_script and gdbpy_execute_objfile_script; it is NULL
   at other times.  */
static struct objfile *gdbpy_current_objfile;

/* Set the current objfile to OBJFILE and then read FILE named FILENAME
   as Python code.  This does not throw any errors.  If an exception
   occurs python will print the traceback and clear the error indicator.
   This is the extension_language_script_ops.objfile_script_sourcer
   "method".  */

static void
gdbpy_source_objfile_script (const struct extension_language_defn *extlang,
			     struct objfile *objfile, FILE *file,
			     const char *filename)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (get_objfile_arch (objfile), current_language);
  gdbpy_current_objfile = objfile;

  python_run_simple_file (file, filename);

  gdbpy_current_objfile = NULL;
}

/* Set the current objfile to OBJFILE and then execute SCRIPT
   as Python code.  This does not throw any errors.  If an exception
   occurs python will print the traceback and clear the error indicator.
   This is the extension_language_script_ops.objfile_script_executor
   "method".  */

static void
gdbpy_execute_objfile_script (const struct extension_language_defn *extlang,
			      struct objfile *objfile, const char *name,
			      const char *script)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (get_objfile_arch (objfile), current_language);
  gdbpy_current_objfile = objfile;

  PyRun_SimpleString (script);

  gdbpy_current_objfile = NULL;
}

/* Return the current Objfile, or None if there isn't one.  */

static PyObject *
gdbpy_get_current_objfile (PyObject *unused1, PyObject *unused2)
{
  PyObject *result;

  if (! gdbpy_current_objfile)
    Py_RETURN_NONE;

  result = objfile_to_objfile_object (gdbpy_current_objfile);
  if (result)
    Py_INCREF (result);
  return result;
}

/* Return a sequence holding all the Objfiles.  */

static PyObject *
gdbpy_objfiles (PyObject *unused1, PyObject *unused2)
{
  struct objfile *objf;

  gdbpy_ref<> list (PyList_New (0));
  if (list == NULL)
    return NULL;

  ALL_OBJFILES (objf)
  {
    PyObject *item = objfile_to_objfile_object (objf);

    if (!item || PyList_Append (list.get (), item) == -1)
      return NULL;
  }

  return list.release ();
}

/* Compute the list of active python type printers and store them in
   EXT_PRINTERS->py_type_printers.  The product of this function is used by
   gdbpy_apply_type_printers, and freed by gdbpy_free_type_printers.
   This is the extension_language_ops.start_type_printers "method".  */

static void
gdbpy_start_type_printers (const struct extension_language_defn *extlang,
			   struct ext_lang_type_printers *ext_printers)
{
  PyObject *printers_obj = NULL;

  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (get_current_arch (), current_language);

  gdbpy_ref<> type_module (PyImport_ImportModule ("gdb.types"));
  if (type_module == NULL)
    {
      gdbpy_print_stack ();
      return;
    }

  gdbpy_ref<> func (PyObject_GetAttrString (type_module.get (),
					    "get_type_recognizers"));
  if (func == NULL)
    {
      gdbpy_print_stack ();
      return;
    }

  printers_obj = PyObject_CallFunctionObjArgs (func.get (), (char *) NULL);
  if (printers_obj == NULL)
    gdbpy_print_stack ();
  else
    ext_printers->py_type_printers = printers_obj;
}

/* If TYPE is recognized by some type printer, store in *PRETTIED_TYPE
   a newly allocated string holding the type's replacement name, and return
   EXT_LANG_RC_OK.  The caller is responsible for freeing the string.
   If there's a Python error return EXT_LANG_RC_ERROR.
   Otherwise, return EXT_LANG_RC_NOP.
   This is the extension_language_ops.apply_type_printers "method".  */

static enum ext_lang_rc
gdbpy_apply_type_printers (const struct extension_language_defn *extlang,
			   const struct ext_lang_type_printers *ext_printers,
			   struct type *type, char **prettied_type)
{
  PyObject *printers_obj = (PyObject *) ext_printers->py_type_printers;
  gdb::unique_xmalloc_ptr<char> result;

  if (printers_obj == NULL)
    return EXT_LANG_RC_NOP;

  if (!gdb_python_initialized)
    return EXT_LANG_RC_NOP;

  gdbpy_enter enter_py (get_current_arch (), current_language);

  gdbpy_ref<> type_obj (type_to_type_object (type));
  if (type_obj == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  gdbpy_ref<> type_module (PyImport_ImportModule ("gdb.types"));
  if (type_module == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  gdbpy_ref<> func (PyObject_GetAttrString (type_module.get (),
					    "apply_type_recognizers"));
  if (func == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  gdbpy_ref<> result_obj (PyObject_CallFunctionObjArgs (func.get (),
							printers_obj,
							type_obj.get (),
							(char *) NULL));
  if (result_obj == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  if (result_obj == Py_None)
    return EXT_LANG_RC_NOP;

  result = python_string_to_host_string (result_obj.get ());
  if (result == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  *prettied_type = result.release ();
  return EXT_LANG_RC_OK;
}

/* Free the result of start_type_printers.
   This is the extension_language_ops.free_type_printers "method".  */

static void
gdbpy_free_type_printers (const struct extension_language_defn *extlang,
			  struct ext_lang_type_printers *ext_printers)
{
  PyObject *printers = (PyObject *) ext_printers->py_type_printers;

  if (printers == NULL)
    return;

  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (get_current_arch (), current_language);
  Py_DECREF (printers);
}

#else /* HAVE_PYTHON */

/* Dummy implementation of the gdb "python-interactive" and "python"
   command. */

static void
python_interactive_command (char *arg, int from_tty)
{
  arg = skip_spaces (arg);
  if (arg && *arg)
    error (_("Python scripting is not supported in this copy of GDB."));
  else
    {
      command_line_up l = get_command_line (python_control, "");

      execute_control_command_untraced (l.get ());
    }
}

static void
python_command (char *arg, int from_tty)
{
  python_interactive_command (arg, from_tty);
}

#endif /* HAVE_PYTHON */



/* Lists for 'set python' commands.  */

static struct cmd_list_element *user_set_python_list;
static struct cmd_list_element *user_show_python_list;

/* Function for use by 'set python' prefix command.  */

static void
user_set_python (char *args, int from_tty)
{
  help_list (user_set_python_list, "set python ", all_commands,
	     gdb_stdout);
}

/* Function for use by 'show python' prefix command.  */

static void
user_show_python (char *args, int from_tty)
{
  cmd_show_list (user_show_python_list, from_tty, "");
}

/* Initialize the Python code.  */

#ifdef HAVE_PYTHON

/* This is installed as a final cleanup and cleans up the
   interpreter.  This lets Python's 'atexit' work.  */

static void
finalize_python (void *ignore)
{
  struct active_ext_lang_state *previous_active;

  /* We don't use ensure_python_env here because if we ever ran the
     cleanup, gdb would crash -- because the cleanup calls into the
     Python interpreter, which we are about to destroy.  It seems
     clearer to make the needed calls explicitly here than to create a
     cleanup and then mysteriously discard it.  */

  /* This is only called as a final cleanup so we can assume the active
     SIGINT handler is gdb's.  We still need to tell it to notify Python.  */
  previous_active = set_active_ext_lang (&extension_language_python);

  (void) PyGILState_Ensure ();
  python_gdbarch = target_gdbarch ();
  python_language = current_language;

  Py_Finalize ();

  restore_active_ext_lang (previous_active);
}
#endif

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_python;

#ifdef HAVE_PYTHON

static bool
do_start_initialization ()
{
  char *progname;
#ifdef IS_PY3K
  int i;
  size_t progsize, count;
  char *oldloc;
  wchar_t *progname_copy;
#endif

#ifdef WITH_PYTHON_PATH
  /* Work around problem where python gets confused about where it is,
     and then can't find its libraries, etc.
     NOTE: Python assumes the following layout:
     /foo/bin/python
     /foo/lib/pythonX.Y/...
     This must be done before calling Py_Initialize.  */
  progname = concat (ldirname (python_libdir).c_str (), SLASH_STRING, "bin",
		     SLASH_STRING, "python", (char *) NULL);
#ifdef IS_PY3K
  oldloc = xstrdup (setlocale (LC_ALL, NULL));
  setlocale (LC_ALL, "");
  progsize = strlen (progname);
  progname_copy = (wchar_t *) PyMem_Malloc ((progsize + 1) * sizeof (wchar_t));
  if (!progname_copy)
    {
      xfree (oldloc);
      fprintf (stderr, "out of memory\n");
      return false;
    }
  count = mbstowcs (progname_copy, progname, progsize + 1);
  if (count == (size_t) -1)
    {
      xfree (oldloc);
      fprintf (stderr, "Could not convert python path to string\n");
      return false;
    }
  setlocale (LC_ALL, oldloc);
  xfree (oldloc);

  /* Note that Py_SetProgramName expects the string it is passed to
     remain alive for the duration of the program's execution, so
     it is not freed after this call.  */
  Py_SetProgramName (progname_copy);
#else
  Py_SetProgramName (progname);
#endif
#endif

  Py_Initialize ();
  PyEval_InitThreads ();

#ifdef IS_PY3K
  gdb_module = PyModule_Create (&python_GdbModuleDef);
  /* Add _gdb module to the list of known built-in modules.  */
  _PyImport_FixupBuiltin (gdb_module, "_gdb");
#else
  gdb_module = Py_InitModule ("_gdb", python_GdbMethods);
#endif
  if (gdb_module == NULL)
    return false;

  /* The casts to (char*) are for python 2.4.  */
  if (PyModule_AddStringConstant (gdb_module, "VERSION", (char*) version) < 0
      || PyModule_AddStringConstant (gdb_module, "HOST_CONFIG",
				     (char*) host_name) < 0
      || PyModule_AddStringConstant (gdb_module, "TARGET_CONFIG",
				     (char*) target_name) < 0)
    return false;

  /* Add stream constants.  */
  if (PyModule_AddIntConstant (gdb_module, "STDOUT", 0) < 0
      || PyModule_AddIntConstant (gdb_module, "STDERR", 1) < 0
      || PyModule_AddIntConstant (gdb_module, "STDLOG", 2) < 0)
    return false;

  gdbpy_gdb_error = PyErr_NewException ("gdb.error", PyExc_RuntimeError, NULL);
  if (gdbpy_gdb_error == NULL
      || gdb_pymodule_addobject (gdb_module, "error", gdbpy_gdb_error) < 0)
    return false;

  gdbpy_gdb_memory_error = PyErr_NewException ("gdb.MemoryError",
					       gdbpy_gdb_error, NULL);
  if (gdbpy_gdb_memory_error == NULL
      || gdb_pymodule_addobject (gdb_module, "MemoryError",
				 gdbpy_gdb_memory_error) < 0)
    return false;

  gdbpy_gdberror_exc = PyErr_NewException ("gdb.GdbError", NULL, NULL);
  if (gdbpy_gdberror_exc == NULL
      || gdb_pymodule_addobject (gdb_module, "GdbError",
				 gdbpy_gdberror_exc) < 0)
    return false;

  gdbpy_initialize_gdb_readline ();

  if (gdbpy_initialize_auto_load () < 0
      || gdbpy_initialize_values () < 0
      || gdbpy_initialize_frames () < 0
      || gdbpy_initialize_commands () < 0
      || gdbpy_initialize_instruction () < 0
      || gdbpy_initialize_record () < 0
      || gdbpy_initialize_btrace () < 0
      || gdbpy_initialize_symbols () < 0
      || gdbpy_initialize_symtabs () < 0
      || gdbpy_initialize_blocks () < 0
      || gdbpy_initialize_functions () < 0
      || gdbpy_initialize_parameters () < 0
      || gdbpy_initialize_types () < 0
      || gdbpy_initialize_pspace () < 0
      || gdbpy_initialize_objfile () < 0
      || gdbpy_initialize_breakpoints () < 0
      || gdbpy_initialize_finishbreakpoints () < 0
      || gdbpy_initialize_lazy_string () < 0
      || gdbpy_initialize_linetable () < 0
      || gdbpy_initialize_thread () < 0
      || gdbpy_initialize_inferior () < 0
      || gdbpy_initialize_events () < 0
      || gdbpy_initialize_eventregistry () < 0
      || gdbpy_initialize_py_events () < 0
      || gdbpy_initialize_event () < 0
      || gdbpy_initialize_stop_event () < 0
      || gdbpy_initialize_signal_event () < 0
      || gdbpy_initialize_breakpoint_event () < 0
      || gdbpy_initialize_continue_event () < 0
      || gdbpy_initialize_inferior_call_pre_event () < 0
      || gdbpy_initialize_inferior_call_post_event () < 0
      || gdbpy_initialize_register_changed_event () < 0
      || gdbpy_initialize_memory_changed_event () < 0
      || gdbpy_initialize_exited_event () < 0
      || gdbpy_initialize_thread_event () < 0
      || gdbpy_initialize_new_objfile_event ()  < 0
      || gdbpy_initialize_clear_objfiles_event ()  < 0
      || gdbpy_initialize_arch () < 0
      || gdbpy_initialize_xmethods () < 0
      || gdbpy_initialize_unwind () < 0)
    return false;

  gdbpy_to_string_cst = PyString_FromString ("to_string");
  if (gdbpy_to_string_cst == NULL)
    return false;
  gdbpy_children_cst = PyString_FromString ("children");
  if (gdbpy_children_cst == NULL)
    return false;
  gdbpy_display_hint_cst = PyString_FromString ("display_hint");
  if (gdbpy_display_hint_cst == NULL)
    return false;
  gdbpy_doc_cst = PyString_FromString ("__doc__");
  if (gdbpy_doc_cst == NULL)
    return false;
  gdbpy_enabled_cst = PyString_FromString ("enabled");
  if (gdbpy_enabled_cst == NULL)
    return false;
  gdbpy_value_cst = PyString_FromString ("value");
  if (gdbpy_value_cst == NULL)
    return false;

  /* Release the GIL while gdb runs.  */
  PyThreadState_Swap (NULL);
  PyEval_ReleaseLock ();

  make_final_cleanup (finalize_python, NULL);

  /* Only set this when initialization has succeeded.  */
  gdb_python_initialized = 1;
  return true;
}

#endif /* HAVE_PYTHON */

void
_initialize_python (void)
{
  add_com ("python-interactive", class_obscure,
	   python_interactive_command,
#ifdef HAVE_PYTHON
	   _("\
Start an interactive Python prompt.\n\
\n\
To return to GDB, type the EOF character (e.g., Ctrl-D on an empty\n\
prompt).\n\
\n\
Alternatively, a single-line Python command can be given as an\n\
argument, and if the command is an expression, the result will be\n\
printed.  For example:\n\
\n\
    (gdb) python-interactive 2 + 3\n\
    5\n\
")
#else /* HAVE_PYTHON */
	   _("\
Start a Python interactive prompt.\n\
\n\
Python scripting is not supported in this copy of GDB.\n\
This command is only a placeholder.")
#endif /* HAVE_PYTHON */
	   );
  add_com_alias ("pi", "python-interactive", class_obscure, 1);

  add_com ("python", class_obscure, python_command,
#ifdef HAVE_PYTHON
	   _("\
Evaluate a Python command.\n\
\n\
The command can be given as an argument, for instance:\n\
\n\
    python print 23\n\
\n\
If no argument is given, the following lines are read and used\n\
as the Python commands.  Type a line containing \"end\" to indicate\n\
the end of the command.")
#else /* HAVE_PYTHON */
	   _("\
Evaluate a Python command.\n\
\n\
Python scripting is not supported in this copy of GDB.\n\
This command is only a placeholder.")
#endif /* HAVE_PYTHON */
	   );
  add_com_alias ("py", "python", class_obscure, 1);

  /* Add set/show python print-stack.  */
  add_prefix_cmd ("python", no_class, user_show_python,
		  _("Prefix command for python preference settings."),
		  &user_show_python_list, "show python ", 0,
		  &showlist);

  add_prefix_cmd ("python", no_class, user_set_python,
		  _("Prefix command for python preference settings."),
		  &user_set_python_list, "set python ", 0,
		  &setlist);

  add_setshow_enum_cmd ("print-stack", no_class, python_excp_enums,
			&gdbpy_should_print_stack, _("\
Set mode for Python stack dump on error."), _("\
Show the mode of Python stack printing on error."), _("\
none  == no stack or message will be printed.\n\
full == a message and a stack will be printed.\n\
message == an error message without a stack will be printed."),
			NULL, NULL,
			&user_set_python_list,
			&user_show_python_list);

#ifdef HAVE_PYTHON
  if (!do_start_initialization () && PyErr_Occurred ())
    gdbpy_print_stack ();
#endif /* HAVE_PYTHON */
}

#ifdef HAVE_PYTHON

/* Helper function for gdbpy_finish_initialization.  This does the
   work and then returns false if an error has occurred and must be
   displayed, or true on success.  */

static bool
do_finish_initialization (const struct extension_language_defn *extlang)
{
  PyObject *m;
  PyObject *sys_path;

  /* Add the initial data-directory to sys.path.  */

  std::string gdb_pythondir = (std::string (gdb_datadir) + SLASH_STRING
			       + "python");

  sys_path = PySys_GetObject ("path");

  /* If sys.path is not defined yet, define it first.  */
  if (!(sys_path && PyList_Check (sys_path)))
    {
#ifdef IS_PY3K
      PySys_SetPath (L"");
#else
      PySys_SetPath ("");
#endif
      sys_path = PySys_GetObject ("path");
    }
  if (sys_path && PyList_Check (sys_path))
    {
      gdbpy_ref<> pythondir (PyString_FromString (gdb_pythondir.c_str ()));
      if (pythondir == NULL || PyList_Insert (sys_path, 0, pythondir.get ()))
	return false;
    }
  else
    return false;

  /* Import the gdb module to finish the initialization, and
     add it to __main__ for convenience.  */
  m = PyImport_AddModule ("__main__");
  if (m == NULL)
    return false;

  /* Keep the reference to gdb_python_module since it is in a global
     variable.  */
  gdb_python_module = PyImport_ImportModule ("gdb");
  if (gdb_python_module == NULL)
    {
      gdbpy_print_stack ();
      /* This is passed in one call to warning so that blank lines aren't
	 inserted between each line of text.  */
      warning (_("\n"
		 "Could not load the Python gdb module from `%s'.\n"
		 "Limited Python support is available from the _gdb module.\n"
		 "Suggest passing --data-directory=/path/to/gdb/data-directory.\n"),
	       gdb_pythondir.c_str ());
      /* We return "success" here as we've already emitted the
	 warning.  */
      return true;
    }

  return gdb_pymodule_addobject (m, "gdb", gdb_python_module) >= 0;
}

/* Perform the remaining python initializations.
   These must be done after GDB is at least mostly initialized.
   E.g., The "info pretty-printer" command needs the "info" prefix
   command installed.
   This is the extension_language_ops.finish_initialization "method".  */

static void
gdbpy_finish_initialization (const struct extension_language_defn *extlang)
{
  gdbpy_enter enter_py (get_current_arch (), current_language);

  if (!do_finish_initialization (extlang))
    {
      gdbpy_print_stack ();
      warning (_("internal error: Unhandled Python exception"));
    }
}

/* Return non-zero if Python has successfully initialized.
   This is the extension_languages_ops.initialized "method".  */

static int
gdbpy_initialized (const struct extension_language_defn *extlang)
{
  return gdb_python_initialized;
}

#endif /* HAVE_PYTHON */



#ifdef HAVE_PYTHON

PyMethodDef python_GdbMethods[] =
{
  { "history", gdbpy_history, METH_VARARGS,
    "Get a value from history" },
  { "execute", (PyCFunction) execute_gdb_command, METH_VARARGS | METH_KEYWORDS,
    "execute (command [, from_tty] [, to_string]) -> [String]\n\
Evaluate command, a string, as a gdb CLI command.  Optionally returns\n\
a Python String containing the output of the command if to_string is\n\
set to True." },
  { "parameter", gdbpy_parameter, METH_VARARGS,
    "Return a gdb parameter's value" },

  { "breakpoints", gdbpy_breakpoints, METH_NOARGS,
    "Return a tuple of all breakpoint objects" },

  { "default_visualizer", gdbpy_default_visualizer, METH_VARARGS,
    "Find the default visualizer for a Value." },

  { "current_progspace", gdbpy_get_current_progspace, METH_NOARGS,
    "Return the current Progspace." },
  { "progspaces", gdbpy_progspaces, METH_NOARGS,
    "Return a sequence of all progspaces." },

  { "current_objfile", gdbpy_get_current_objfile, METH_NOARGS,
    "Return the current Objfile being loaded, or None." },
  { "objfiles", gdbpy_objfiles, METH_NOARGS,
    "Return a sequence of all loaded objfiles." },

  { "newest_frame", gdbpy_newest_frame, METH_NOARGS,
    "newest_frame () -> gdb.Frame.\n\
Return the newest frame object." },
  { "selected_frame", gdbpy_selected_frame, METH_NOARGS,
    "selected_frame () -> gdb.Frame.\n\
Return the selected frame object." },
  { "frame_stop_reason_string", gdbpy_frame_stop_reason_string, METH_VARARGS,
    "stop_reason_string (Integer) -> String.\n\
Return a string explaining unwind stop reason." },

  { "start_recording", gdbpy_start_recording, METH_VARARGS,
    "start_recording ([method] [, format]) -> gdb.Record.\n\
Start recording with the given method.  If no method is given, will fall back\n\
to the system default method.  If no format is given, will fall back to the\n\
default format for the given method."},
  { "current_recording", gdbpy_current_recording, METH_NOARGS,
    "current_recording () -> gdb.Record.\n\
Return current recording object." },
  { "stop_recording", gdbpy_stop_recording, METH_NOARGS,
    "stop_recording () -> None.\n\
Stop current recording." },

  { "lookup_type", (PyCFunction) gdbpy_lookup_type,
    METH_VARARGS | METH_KEYWORDS,
    "lookup_type (name [, block]) -> type\n\
Return a Type corresponding to the given name." },
  { "lookup_symbol", (PyCFunction) gdbpy_lookup_symbol,
    METH_VARARGS | METH_KEYWORDS,
    "lookup_symbol (name [, block] [, domain]) -> (symbol, is_field_of_this)\n\
Return a tuple with the symbol corresponding to the given name (or None) and\n\
a boolean indicating if name is a field of the current implied argument\n\
`this' (when the current language is object-oriented)." },
  { "lookup_global_symbol", (PyCFunction) gdbpy_lookup_global_symbol,
    METH_VARARGS | METH_KEYWORDS,
    "lookup_global_symbol (name [, domain]) -> symbol\n\
Return the symbol corresponding to the given name (or None)." },

  { "lookup_objfile", (PyCFunction) gdbpy_lookup_objfile,
    METH_VARARGS | METH_KEYWORDS,
    "lookup_objfile (name, [by_build_id]) -> objfile\n\
Look up the specified objfile.\n\
If by_build_id is True, the objfile is looked up by using name\n\
as its build id." },

  { "block_for_pc", gdbpy_block_for_pc, METH_VARARGS,
    "Return the block containing the given pc value, or None." },
  { "solib_name", gdbpy_solib_name, METH_VARARGS,
    "solib_name (Long) -> String.\n\
Return the name of the shared library holding a given address, or None." },
  { "decode_line", gdbpy_decode_line, METH_VARARGS,
    "decode_line (String) -> Tuple.  Decode a string argument the way\n\
that 'break' or 'edit' does.  Return a tuple containing two elements.\n\
The first element contains any unparsed portion of the String parameter\n\
(or None if the string was fully parsed).  The second element contains\n\
a tuple that contains all the locations that match, represented as\n\
gdb.Symtab_and_line objects (or None)."},
  { "parse_and_eval", gdbpy_parse_and_eval, METH_VARARGS,
    "parse_and_eval (String) -> Value.\n\
Parse String as an expression, evaluate it, and return the result as a Value."
  },
  { "find_pc_line", gdbpy_find_pc_line, METH_VARARGS,
    "find_pc_line (pc) -> Symtab_and_line.\n\
Return the gdb.Symtab_and_line object corresponding to the pc value." },

  { "post_event", gdbpy_post_event, METH_VARARGS,
    "Post an event into gdb's event loop." },

  { "target_charset", gdbpy_target_charset, METH_NOARGS,
    "target_charset () -> string.\n\
Return the name of the current target charset." },
  { "target_wide_charset", gdbpy_target_wide_charset, METH_NOARGS,
    "target_wide_charset () -> string.\n\
Return the name of the current target wide charset." },

  { "string_to_argv", gdbpy_string_to_argv, METH_VARARGS,
    "string_to_argv (String) -> Array.\n\
Parse String and return an argv-like array.\n\
Arguments are separate by spaces and may be quoted."
  },
  { "write", (PyCFunction)gdbpy_write, METH_VARARGS | METH_KEYWORDS,
    "Write a string using gdb's filtered stream." },
  { "flush", (PyCFunction)gdbpy_flush, METH_VARARGS | METH_KEYWORDS,
    "Flush gdb's filtered stdout stream." },
  { "selected_thread", gdbpy_selected_thread, METH_NOARGS,
    "selected_thread () -> gdb.InferiorThread.\n\
Return the selected thread object." },
  { "selected_inferior", gdbpy_selected_inferior, METH_NOARGS,
    "selected_inferior () -> gdb.Inferior.\n\
Return the selected inferior object." },
  { "inferiors", gdbpy_inferiors, METH_NOARGS,
    "inferiors () -> (gdb.Inferior, ...).\n\
Return a tuple containing all inferiors." },

  { "invalidate_cached_frames", gdbpy_invalidate_cached_frames, METH_NOARGS,
    "invalidate_cached_frames () -> None.\n\
Invalidate any cached frame objects in gdb.\n\
Intended for internal use only." },

  {NULL, NULL, 0, NULL}
};

#ifdef IS_PY3K
struct PyModuleDef python_GdbModuleDef =
{
  PyModuleDef_HEAD_INIT,
  "_gdb",
  NULL,
  -1,
  python_GdbMethods,
  NULL,
  NULL,
  NULL,
  NULL
};
#endif
#endif /* HAVE_PYTHON */
