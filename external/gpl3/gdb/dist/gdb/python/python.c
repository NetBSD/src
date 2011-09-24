/* General python/gdb code

   Copyright (C) 2008, 2009, 2010, 2011 Free Software Foundation, Inc.

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
#include "exceptions.h"
#include "event-loop.h"
#include "serial.h"
#include "python.h"

#include <ctype.h>

/* True if we should print the stack when catching a Python error,
   false otherwise.  */
static int gdbpy_should_print_stack = 1;

#ifdef HAVE_PYTHON

#include "libiberty.h"
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

static PyMethodDef GdbMethods[];

PyObject *gdb_module;

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

/* Architecture and language to be used in callbacks from
   the Python interpreter.  */
struct gdbarch *python_gdbarch;
const struct language_defn *python_language;

/* Restore global language and architecture and Python GIL state
   when leaving the Python interpreter.  */

struct python_env
{
  PyGILState_STATE state;
  struct gdbarch *gdbarch;
  const struct language_defn *language;
  PyObject *error_type, *error_value, *error_traceback;
};

static void
restore_python_env (void *p)
{
  struct python_env *env = (struct python_env *)p;

  /* Leftover Python error is forbidden by Python Exception Handling.  */
  if (PyErr_Occurred ())
    {
      /* This order is similar to the one calling error afterwards. */
      gdbpy_print_stack ();
      warning (_("internal error: Unhandled Python exception"));
    }

  PyErr_Restore (env->error_type, env->error_value, env->error_traceback);

  PyGILState_Release (env->state);
  python_gdbarch = env->gdbarch;
  python_language = env->language;
  xfree (env);
}

/* Called before entering the Python interpreter to install the
   current language and architecture to be used for Python values.  */

struct cleanup *
ensure_python_env (struct gdbarch *gdbarch,
                   const struct language_defn *language)
{
  struct python_env *env = xmalloc (sizeof *env);

  env->state = PyGILState_Ensure ();
  env->gdbarch = python_gdbarch;
  env->language = python_language;

  python_gdbarch = gdbarch;
  python_language = language;

  /* Save it and ensure ! PyErr_Occurred () afterwards.  */
  PyErr_Fetch (&env->error_type, &env->error_value, &env->error_traceback);
  
  return make_cleanup (restore_python_env, env);
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

  script = xmalloc (size + 1);
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

void
eval_python_from_control_command (struct command_line *cmd)
{
  int ret;
  char *script;
  struct cleanup *cleanup;

  if (cmd->body_count != 1)
    error (_("Invalid \"python\" block structure."));

  cleanup = ensure_python_env (get_current_arch (), current_language);

  script = compute_python_string (cmd->body_list[0]);
  ret = PyRun_SimpleString (script);
  xfree (script);
  if (ret)
    {
      gdbpy_print_stack ();
      error (_("Error while executing Python code."));
    }

  do_cleanups (cleanup);
}

/* Implementation of the gdb "python" command.  */

static void
python_command (char *arg, int from_tty)
{
  struct cleanup *cleanup;

  cleanup = ensure_python_env (get_current_arch (), current_language);
  while (arg && *arg && isspace (*arg))
    ++arg;
  if (arg && *arg)
    {
      if (PyRun_SimpleString (arg))
	{
	  gdbpy_print_stack ();
	  error (_("Error while executing Python code."));
	}
    }
  else
    {
      struct command_line *l = get_command_line (python_control, "");

      make_cleanup_free_command_lines (&l);
      execute_control_command_untraced (l);
    }

  do_cleanups (cleanup);
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
	char *str = * (char **) var;

	if (! str)
	  str = "";
	return PyString_Decode (str, strlen (str), host_charset (), NULL);
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

PyObject *
gdbpy_parameter (PyObject *self, PyObject *args)
{
  struct cmd_list_element *alias, *prefix, *cmd;
  char *arg, *newarg;
  int found = -1;
  volatile struct gdb_exception except;

  if (! PyArg_ParseTuple (args, "s", &arg))
    return NULL;

  newarg = concat ("show ", arg, (char *) NULL);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      found = lookup_cmd_composition (newarg, &alias, &prefix, &cmd);
    }
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
  char *arg;
  PyObject *from_tty_obj = NULL, *to_string_obj = NULL;
  int from_tty, to_string;
  volatile struct gdb_exception except;
  static char *keywords[] = {"command", "from_tty", "to_string", NULL };
  char *result = NULL;

  if (! PyArg_ParseTupleAndKeywords (args, kw, "s|O!O!", keywords, &arg,
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

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      /* Copy the argument text in case the command modifies it.  */
      char *copy = xstrdup (arg);
      struct cleanup *cleanup = make_cleanup (xfree, copy);

      prevent_dont_repeat ();
      if (to_string)
	result = execute_command_to_string (copy, from_tty);
      else
	{
	  result = NULL;
	  execute_command (copy, from_tty);
	}

      do_cleanups (cleanup);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  /* Do any commands attached to breakpoint we stopped at.  */
  bpstat_do_actions ();

  if (result)
    {
      PyObject *r = PyString_FromString (result);
      xfree (result);
      return r;
    }
  Py_RETURN_NONE;
}

/* Implementation of gdb.solib_name (Long) -> String.
   Returns the name of the shared library holding a given address, or None.  */

static PyObject *
gdbpy_solib_name (PyObject *self, PyObject *args)
{
  char *soname;
  PyObject *str_obj;
  gdb_py_longest pc;

  if (!PyArg_ParseTuple (args, GDB_PY_LL_ARG, &pc))
    return NULL;

  soname = solib_name_from_address (current_program_space, pc);
  if (soname)
    str_obj = PyString_Decode (soname, strlen (soname), host_charset (), NULL);
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
  struct symtabs_and_lines sals = { NULL, 0 }; /* Initialize to
						  appease gcc.  */
  struct symtab_and_line sal;
  char *arg = NULL;
  char *copy = NULL;
  struct cleanup *cleanups;
  PyObject *result = NULL;
  PyObject *return_result = NULL;
  PyObject *unparsed = NULL;
  volatile struct gdb_exception except;

  if (! PyArg_ParseTuple (args, "|s", &arg))
    return NULL;

  cleanups = ensure_python_env (get_current_arch (), current_language);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      if (arg)
	{
	  arg = xstrdup (arg);
	  make_cleanup (xfree, arg);
	  copy = arg;
	  sals = decode_line_1 (&copy, 0, 0, 0, 0);
	  make_cleanup (xfree, sals.sals);
	}
      else
	{
	  set_default_source_symtab_and_line ();
	  sal = get_current_source_symtab_and_line ();
	  sals.sals = &sal;
	  sals.nelts = 1;
	}
    }
  if (except.reason < 0)
    {
      do_cleanups (cleanups);
      /* We know this will always throw.  */
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (sals.nelts)
    {
      int i;

      result = PyTuple_New (sals.nelts);
      if (! result)
	goto error;
      for (i = 0; i < sals.nelts; ++i)
	{
	  PyObject *obj;
	  char *str;

	  obj = symtab_and_line_to_sal_object (sals.sals[i]);
	  if (! obj)
	    {
	      Py_DECREF (result);
	      goto error;
	    }

	  PyTuple_SetItem (result, i, obj);
	}
    }
  else
    {
      result = Py_None;
      Py_INCREF (Py_None);
    }

  return_result = PyTuple_New (2);
  if (! return_result)
    {
      Py_DECREF (result);
      goto error;
    }

  if (copy && strlen (copy) > 0)
    unparsed = PyString_FromString (copy);
  else
    {
      unparsed = Py_None;
      Py_INCREF (Py_None);
    }

  PyTuple_SetItem (return_result, 0, unparsed);
  PyTuple_SetItem (return_result, 1, result);

  do_cleanups (cleanups);

  return return_result;

 error:
  do_cleanups (cleanups);
  return NULL;
}

/* Parse a string and evaluate it as an expression.  */
static PyObject *
gdbpy_parse_and_eval (PyObject *self, PyObject *args)
{
  char *expr_str;
  struct value *result = NULL;
  volatile struct gdb_exception except;

  if (!PyArg_ParseTuple (args, "s", &expr_str))
    return NULL;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      result = parse_and_eval (expr_str);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  return value_to_value_object (result);
}

/* Read a file as Python code.  STREAM is the input file; FILE is the
   name of the file.
   STREAM is not closed, that is the caller's responsibility.  */

void
source_python_script (FILE *stream, const char *file)
{
  struct cleanup *cleanup;

  cleanup = ensure_python_env (get_current_arch (), current_language);

  /* Note: If an exception occurs python will print the traceback and
     clear the error indicator.  */
  PyRun_SimpleFile (stream, file);

  do_cleanups (cleanup);
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

/* We use a file handler, and not an async handler, so that we can
   wake up the main thread even when it is blocked in poll().  */
static struct serial *gdbpy_event_fds[2];

/* The file handler callback.  This reads from the internal pipe, and
   then processes the Python event queue.  This will always be run in
   the main gdb thread.  */

static void
gdbpy_run_events (struct serial *scb, void *context)
{
  struct cleanup *cleanup;
  int r;

  cleanup = ensure_python_env (get_current_arch (), current_language);

  /* Flush the fd.  Do this before flushing the events list, so that
     any new event post afterwards is sure to re-awake the event
     loop.  */
  while (serial_readchar (gdbpy_event_fds[0], 0) >= 0)
    ;

  while (gdbpy_event_list)
    {
      /* Dispatching the event might push a new element onto the event
	 loop, so we update here "atomically enough".  */
      struct gdbpy_event *item = gdbpy_event_list;
      gdbpy_event_list = gdbpy_event_list->next;
      if (gdbpy_event_list == NULL)
	gdbpy_event_list_end = &gdbpy_event_list;

      /* Ignore errors.  */
      if (PyObject_CallObject (item->event, NULL) == NULL)
	PyErr_Clear ();

      Py_DECREF (item->event);
      xfree (item);
    }

  do_cleanups (cleanup);
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
    {
      char c = 'q';		/* Anything. */

      if (serial_write (gdbpy_event_fds[1], &c, 1))
        return PyErr_SetFromErrno (PyExc_IOError);
    }

  Py_RETURN_NONE;
}

/* Initialize the Python event handler.  */
static void
gdbpy_initialize_events (void)
{
  if (serial_pipe (gdbpy_event_fds) == 0)
    {
      gdbpy_event_list_end = &gdbpy_event_list;
      serial_async (gdbpy_event_fds[0], gdbpy_run_events, NULL);
    }
}

/* Printing.  */

/* A python function to write a single string using gdb's filtered
   output stream .  The optional keyword STREAM can be used to write
   to a particular stream.  The default stream is to gdb_stdout.  */

static PyObject *
gdbpy_write (PyObject *self, PyObject *args, PyObject *kw)
{
  char *arg;
  static char *keywords[] = {"text", "stream", NULL };
  int stream_type = 0;
  
  if (! PyArg_ParseTupleAndKeywords (args, kw, "s|i", keywords, &arg,
				     &stream_type))
    return NULL;

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
     
  Py_RETURN_NONE;
}

/* A python function to flush a gdb stream.  The optional keyword
   STREAM can be used to flush a particular stream.  The default stream
   is gdb_stdout.  */

static PyObject *
gdbpy_flush (PyObject *self, PyObject *args, PyObject *kw)
{
  static char *keywords[] = {"stream", NULL };
  int stream_type = 0;
  
  if (! PyArg_ParseTupleAndKeywords (args, kw, "|i", keywords,
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

/* Print a python exception trace, or print nothing and clear the
   python exception, depending on gdbpy_should_print_stack.  Only call
   this if a python exception is set.  */
void
gdbpy_print_stack (void)
{
  if (gdbpy_should_print_stack)
    {
      PyErr_Print ();
      /* PyErr_Print doesn't necessarily end output with a newline.
	 This works because Python's stdout/stderr is fed through
	 printf_filtered.  */
      begin_line ();
    }
  else
    PyErr_Clear ();
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
  PyObject *list;

  list = PyList_New (0);
  if (!list)
    return NULL;

  ALL_PSPACES (ps)
  {
    PyObject *item = pspace_to_pspace_object (ps);

    if (!item || PyList_Append (list, item) == -1)
      {
	Py_DECREF (list);
	return NULL;
      }
  }

  return list;
}



/* The "current" objfile.  This is set when gdb detects that a new
   objfile has been loaded.  It is only set for the duration of a call to
   source_python_script_for_objfile; it is NULL at other times.  */
static struct objfile *gdbpy_current_objfile;

/* Set the current objfile to OBJFILE and then read STREAM,FILE as
   Python code.  */

void
source_python_script_for_objfile (struct objfile *objfile,
				  FILE *stream, const char *file)
{
  struct cleanup *cleanups;

  cleanups = ensure_python_env (get_objfile_arch (objfile), current_language);
  gdbpy_current_objfile = objfile;

  /* Note: If an exception occurs python will print the traceback and
     clear the error indicator.  */
  PyRun_SimpleFile (stream, file);

  do_cleanups (cleanups);
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
  PyObject *list;

  list = PyList_New (0);
  if (!list)
    return NULL;

  ALL_OBJFILES (objf)
  {
    PyObject *item = objfile_to_objfile_object (objf);

    if (!item || PyList_Append (list, item) == -1)
      {
	Py_DECREF (list);
	return NULL;
      }
  }

  return list;
}

#else /* HAVE_PYTHON */

/* Dummy implementation of the gdb "python" command.  */

static void
python_command (char *arg, int from_tty)
{
  while (arg && *arg && isspace (*arg))
    ++arg;
  if (arg && *arg)
    error (_("Python scripting is not supported in this copy of GDB."));
  else
    {
      struct command_line *l = get_command_line (python_control, "");
      struct cleanup *cleanups = make_cleanup_free_command_lines (&l);

      execute_control_command_untraced (l);
      do_cleanups (cleanups);
    }
}

void
eval_python_from_control_command (struct command_line *cmd)
{
  error (_("Python scripting is not supported in this copy of GDB."));
}

void
source_python_script (FILE *stream, const char *file)
{
  throw_error (UNSUPPORTED_ERROR,
	       _("Python scripting is not supported in this copy of GDB."));
}

int
gdbpy_should_stop (struct breakpoint_object *bp_obj)
{
  internal_error (__FILE__, __LINE__,
		  _("gdbpy_should_stop called when Python scripting is  " \
		    "not supported."));
}

int
gdbpy_breakpoint_has_py_cond (struct breakpoint_object *bp_obj)
{
  internal_error (__FILE__, __LINE__,
		  _("gdbpy_breakpoint_has_py_cond called when Python " \
		    "scripting is not supported."));
}

#endif /* HAVE_PYTHON */



/* Lists for 'maint set python' commands.  */

struct cmd_list_element *set_python_list;
struct cmd_list_element *show_python_list;

/* Function for use by 'maint set python' prefix command.  */

static void
set_python (char *args, int from_tty)
{
  help_list (set_python_list, "maintenance set python ", -1, gdb_stdout);
}

/* Function for use by 'maint show python' prefix command.  */

static void
show_python (char *args, int from_tty)
{
  cmd_show_list (show_python_list, from_tty, "");
}

/* Initialize the Python code.  */

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_python;

void
_initialize_python (void)
{
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

  add_prefix_cmd ("python", no_class, show_python,
		  _("Prefix command for python maintenance settings."),
		  &show_python_list, "maintenance show python ", 0,
		  &maintenance_show_cmdlist);
  add_prefix_cmd ("python", no_class, set_python,
		  _("Prefix command for python maintenance settings."),
		  &set_python_list, "maintenance set python ", 0,
		  &maintenance_set_cmdlist);

  add_setshow_boolean_cmd ("print-stack", class_maintenance,
			   &gdbpy_should_print_stack, _("\
Enable or disable printing of Python stack dump on error."), _("\
Show whether Python stack will be printed on error."), _("\
Enables or disables printing of Python stack traces."),
			   NULL, NULL,
			   &set_python_list,
			   &show_python_list);

#ifdef HAVE_PYTHON
#ifdef WITH_PYTHON_PATH
  /* Work around problem where python gets confused about where it is,
     and then can't find its libraries, etc.
     NOTE: Python assumes the following layout:
     /foo/bin/python
     /foo/lib/pythonX.Y/...
     This must be done before calling Py_Initialize.  */
  Py_SetProgramName (concat (ldirname (python_libdir), SLASH_STRING, "bin",
			     SLASH_STRING, "python", NULL));
#endif

  Py_Initialize ();
  PyEval_InitThreads ();

  gdb_module = Py_InitModule ("gdb", GdbMethods);

  /* The casts to (char*) are for python 2.4.  */
  PyModule_AddStringConstant (gdb_module, "VERSION", (char*) version);
  PyModule_AddStringConstant (gdb_module, "HOST_CONFIG", (char*) host_name);
  PyModule_AddStringConstant (gdb_module, "TARGET_CONFIG",
			      (char*) target_name);

  /* Add stream constants.  */
  PyModule_AddIntConstant (gdb_module, "STDOUT", 0);
  PyModule_AddIntConstant (gdb_module, "STDERR", 1);
  PyModule_AddIntConstant (gdb_module, "STDLOG", 2);
  
  /* gdb.parameter ("data-directory") doesn't necessarily exist when the python
     script below is run (depending on order of _initialize_* functions).
     Define the initial value of gdb.PYTHONDIR here.  */
  {
    char *gdb_pythondir;

    gdb_pythondir = concat (gdb_datadir, SLASH_STRING, "python", NULL);
    PyModule_AddStringConstant (gdb_module, "PYTHONDIR", gdb_pythondir);
    xfree (gdb_pythondir);
  }

  gdbpy_gdb_error = PyErr_NewException ("gdb.error", PyExc_RuntimeError, NULL);
  PyModule_AddObject (gdb_module, "error", gdbpy_gdb_error);

  gdbpy_gdb_memory_error = PyErr_NewException ("gdb.MemoryError",
					       gdbpy_gdb_error, NULL);
  PyModule_AddObject (gdb_module, "MemoryError", gdbpy_gdb_memory_error);

  gdbpy_gdberror_exc = PyErr_NewException ("gdb.GdbError", NULL, NULL);
  PyModule_AddObject (gdb_module, "GdbError", gdbpy_gdberror_exc);

  gdbpy_initialize_auto_load ();
  gdbpy_initialize_values ();
  gdbpy_initialize_frames ();
  gdbpy_initialize_commands ();
  gdbpy_initialize_symbols ();
  gdbpy_initialize_symtabs ();
  gdbpy_initialize_blocks ();
  gdbpy_initialize_functions ();
  gdbpy_initialize_parameters ();
  gdbpy_initialize_types ();
  gdbpy_initialize_pspace ();
  gdbpy_initialize_objfile ();
  gdbpy_initialize_breakpoints ();
  gdbpy_initialize_lazy_string ();
  gdbpy_initialize_thread ();
  gdbpy_initialize_inferior ();
  gdbpy_initialize_events ();

  gdbpy_initialize_eventregistry ();
  gdbpy_initialize_py_events ();
  gdbpy_initialize_event ();
  gdbpy_initialize_stop_event ();
  gdbpy_initialize_signal_event ();
  gdbpy_initialize_breakpoint_event ();
  gdbpy_initialize_continue_event ();
  gdbpy_initialize_exited_event ();
  gdbpy_initialize_thread_event ();

  PyRun_SimpleString ("import gdb");
  PyRun_SimpleString ("gdb.pretty_printers = []");

  gdbpy_to_string_cst = PyString_FromString ("to_string");
  gdbpy_children_cst = PyString_FromString ("children");
  gdbpy_display_hint_cst = PyString_FromString ("display_hint");
  gdbpy_doc_cst = PyString_FromString ("__doc__");
  gdbpy_enabled_cst = PyString_FromString ("enabled");
  gdbpy_value_cst = PyString_FromString ("value");

  /* Release the GIL while gdb runs.  */
  PyThreadState_Swap (NULL);
  PyEval_ReleaseLock ();

#endif /* HAVE_PYTHON */
}

#ifdef HAVE_PYTHON

/* Perform the remaining python initializations.
   These must be done after GDB is at least mostly initialized.
   E.g., The "info pretty-printer" command needs the "info" prefix
   command installed.  */

void
finish_python_initialization (void)
{
  struct cleanup *cleanup;

  cleanup = ensure_python_env (get_current_arch (), current_language);

  PyRun_SimpleString ("\
import os\n\
import sys\n\
\n\
class GdbOutputFile:\n\
  def close(self):\n\
    # Do nothing.\n\
    return None\n\
\n\
  def isatty(self):\n\
    return False\n\
\n\
  def write(self, s):\n\
    gdb.write(s, stream=gdb.STDOUT)\n   \
\n\
  def writelines(self, iterable):\n\
    for line in iterable:\n\
      self.write(line)\n\
\n\
  def flush(self):\n\
    gdb.flush()\n\
\n\
sys.stdout = GdbOutputFile()\n\
\n\
class GdbOutputErrorFile:\n\
  def close(self):\n\
    # Do nothing.\n\
    return None\n\
\n\
  def isatty(self):\n\
    return False\n\
\n\
  def write(self, s):\n\
    gdb.write(s, stream=gdb.STDERR)\n		\
\n\
  def writelines(self, iterable):\n\
    for line in iterable:\n\
      self.write(line)\n \
\n\
  def flush(self):\n\
    gdb.flush()\n\
\n\
sys.stderr = GdbOutputErrorFile()\n\
\n\
# Ideally this would live in the gdb module, but it's intentionally written\n\
# in python, and we need this to bootstrap the gdb module.\n\
\n\
def GdbSetPythonDirectory (dir):\n\
  \"Set gdb.PYTHONDIR and update sys.path,etc.\"\n\
  old_dir = gdb.PYTHONDIR\n\
  gdb.PYTHONDIR = dir\n\
  # GDB's python scripts are stored inside gdb.PYTHONDIR.  So insert\n\
  # that directory name at the start of sys.path to allow the Python\n\
  # interpreter to find them.\n\
  if old_dir in sys.path:\n\
    sys.path.remove (old_dir)\n\
  sys.path.insert (0, gdb.PYTHONDIR)\n\
\n\
  # Tell python where to find submodules of gdb.\n\
  gdb.__path__ = [gdb.PYTHONDIR + '/gdb']\n\
\n\
  # The gdb module is implemented in C rather than in Python.  As a result,\n\
  # the associated __init.py__ script is not not executed by default when\n\
  # the gdb module gets imported.  Execute that script manually if it\n\
  # exists.\n\
  ipy = gdb.PYTHONDIR + '/gdb/__init__.py'\n\
  if os.path.exists (ipy):\n\
    execfile (ipy)\n\
\n\
# Install the default gdb.PYTHONDIR.\n\
GdbSetPythonDirectory (gdb.PYTHONDIR)\n\
");

  do_cleanups (cleanup);
}

#endif /* HAVE_PYTHON */



#ifdef HAVE_PYTHON

static PyMethodDef GdbMethods[] =
{
  { "history", gdbpy_history, METH_VARARGS,
    "Get a value from history" },
  { "execute", (PyCFunction) execute_gdb_command, METH_VARARGS | METH_KEYWORDS,
    "Execute a gdb command" },
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
  { "inferiors", gdbpy_inferiors, METH_NOARGS,
    "inferiors () -> (gdb.Inferior, ...).\n\
Return a tuple containing all inferiors." },
  {NULL, NULL, 0, NULL}
};

#endif /* HAVE_PYTHON */
