/* gdb commands implemented in Python

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
#include "value.h"
#include "python-internal.h"
#include "charset.h"
#include "gdbcmd.h"
#include "cli/cli-decode.h"
#include "completer.h"
#include "language.h"
#include "py-ref.h"

/* Struct representing built-in completion types.  */
struct cmdpy_completer
{
  /* Python symbol name.  */
  const char *name;
  /* Completion function.  */
  completer_ftype *completer;
};

static const struct cmdpy_completer completers[] =
{
  { "COMPLETE_NONE", noop_completer },
  { "COMPLETE_FILENAME", filename_completer },
  { "COMPLETE_LOCATION", location_completer },
  { "COMPLETE_COMMAND", command_completer },
  { "COMPLETE_SYMBOL", make_symbol_completion_list_fn },
  { "COMPLETE_EXPRESSION", expression_completer },
};

#define N_COMPLETERS (sizeof (completers) / sizeof (completers[0]))

/* A gdb command.  For the time being only ordinary commands (not
   set/show commands) are allowed.  */
struct cmdpy_object
{
  PyObject_HEAD

  /* The corresponding gdb command object, or NULL if the command is
     no longer installed.  */
  struct cmd_list_element *command;

  /* A prefix command requires storage for a list of its sub-commands.
     A pointer to this is passed to add_prefix_command, and to add_cmd
     for sub-commands of that prefix.  If this Command is not a prefix
     command, then this field is unused.  */
  struct cmd_list_element *sub_list;
};

typedef struct cmdpy_object cmdpy_object;

extern PyTypeObject cmdpy_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("cmdpy_object");

/* Constants used by this module.  */
static PyObject *invoke_cst;
static PyObject *complete_cst;



/* Python function which wraps dont_repeat.  */
static PyObject *
cmdpy_dont_repeat (PyObject *self, PyObject *args)
{
  dont_repeat ();
  Py_RETURN_NONE;
}



/* Called if the gdb cmd_list_element is destroyed.  */

static void
cmdpy_destroyer (struct cmd_list_element *self, void *context)
{
  gdbpy_enter enter_py (get_current_arch (), current_language);

  /* Release our hold on the command object.  */
  gdbpy_ref<cmdpy_object> cmd ((cmdpy_object *) context);
  cmd->command = NULL;

  /* We allocated the name, doc string, and perhaps the prefix
     name.  */
  xfree ((char *) self->name);
  xfree ((char *) self->doc);
  xfree ((char *) self->prefixname);
}

/* Called by gdb to invoke the command.  */

static void
cmdpy_function (struct cmd_list_element *command,
		char *args_entry, int from_tty)
{
  const char *args = args_entry;
  cmdpy_object *obj = (cmdpy_object *) get_cmd_context (command);

  gdbpy_enter enter_py (get_current_arch (), current_language);

  if (! obj)
    error (_("Invalid invocation of Python command object."));
  if (! PyObject_HasAttr ((PyObject *) obj, invoke_cst))
    {
      if (obj->command->prefixname)
	{
	  /* A prefix command does not need an invoke method.  */
	  return;
	}
      error (_("Python command object missing 'invoke' method."));
    }

  if (! args)
    args = "";
  gdbpy_ref<> argobj (PyUnicode_Decode (args, strlen (args), host_charset (),
					NULL));
  if (argobj == NULL)
    {
      gdbpy_print_stack ();
      error (_("Could not convert arguments to Python string."));
    }

  gdbpy_ref<> ttyobj (from_tty ? Py_True : Py_False);
  Py_INCREF (ttyobj.get ());
  gdbpy_ref<> result (PyObject_CallMethodObjArgs ((PyObject *) obj, invoke_cst,
						  argobj.get (), ttyobj.get (),
						  NULL));

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
	  printf_filtered (_("An error occurred in a Python command\n"
			     "and then another occurred computing the "
			     "error message.\n"));
	  gdbpy_print_stack ();
	}

      /* Don't print the stack for gdb.GdbError exceptions.
	 It is generally used to flag user errors.

	 We also don't want to print "Error occurred in Python command"
	 for user errors.  However, a missing message for gdb.GdbError
	 exceptions is arguably a bug, so we flag it as such.  */

      if (! PyErr_GivenExceptionMatches (ptype, gdbpy_gdberror_exc)
	  || msg == NULL || *msg == '\0')
	{
	  PyErr_Restore (ptype, pvalue, ptraceback);
	  gdbpy_print_stack ();
	  if (msg != NULL && *msg != '\0')
	    error (_("Error occurred in Python command: %s"), msg.get ());
	  else
	    error (_("Error occurred in Python command."));
	}
      else
	{
	  Py_XDECREF (ptype);
	  Py_XDECREF (pvalue);
	  Py_XDECREF (ptraceback);
	  error ("%s", msg.get ());
	}
    }
}

/* Helper function for the Python command completers (both "pure"
   completer and brkchar handler).  This function takes COMMAND, TEXT
   and WORD and tries to call the Python method for completion with
   these arguments.

   This function is usually called twice: once when we are figuring out
   the break characters to be used, and another to perform the real
   completion itself.  The reason for this two step dance is that we
   need to know the set of "brkchars" to use early on, before we
   actually try to perform the completion.  But if a Python command
   supplies a "complete" method then we have to call that method
   first: it may return as its result the kind of completion to
   perform and that will in turn specify which brkchars to use.  IOW,
   we need the result of the "complete" method before we actually
   perform the completion.  The only situation when this function is
   not called twice is when the user uses the "complete" command: in
   this scenario, there is no call to determine the "brkchars".

   Ideally, it would be nice to cache the result of the first call (to
   determine the "brkchars") and return this value directly in the
   second call (to perform the actual completion).  However, due to
   the peculiarity of the "complete" command mentioned above, it is
   possible to put GDB in a bad state if you perform a TAB-completion
   and then a "complete"-completion sequentially.  Therefore, we just
   recalculate everything twice for TAB-completions.

   This function returns the PyObject representing the Python method
   call.  */

static PyObject *
cmdpy_completer_helper (struct cmd_list_element *command,
			const char *text, const char *word)
{
  cmdpy_object *obj = (cmdpy_object *) get_cmd_context (command);

  if (obj == NULL)
    error (_("Invalid invocation of Python command object."));
  if (!PyObject_HasAttr ((PyObject *) obj, complete_cst))
    {
      /* If there is no complete method, don't error.  */
      return NULL;
    }

  gdbpy_ref<> textobj (PyUnicode_Decode (text, strlen (text), host_charset (),
					 NULL));
  if (textobj == NULL)
    error (_("Could not convert argument to Python string."));
  gdbpy_ref<> wordobj (PyUnicode_Decode (word, strlen (word), host_charset (),
					 NULL));
  if (wordobj == NULL)
    error (_("Could not convert argument to Python string."));

  gdbpy_ref<> resultobj (PyObject_CallMethodObjArgs ((PyObject *) obj,
						     complete_cst,
						     textobj.get (),
						     wordobj.get (), NULL));
  if (resultobj == NULL)
    {
      /* Just swallow errors here.  */
      PyErr_Clear ();
    }

  return resultobj.release ();
}

/* Python function called to determine the break characters of a
   certain completer.  We are only interested in knowing if the
   completer registered by the user will return one of the integer
   codes (see COMPLETER_* symbols).  */

static void
cmdpy_completer_handle_brkchars (struct cmd_list_element *command,
				 const char *text, const char *word)
{
  gdbpy_enter enter_py (get_current_arch (), current_language);

  /* Calling our helper to obtain the PyObject of the Python
     function.  */
  gdbpy_ref<> resultobj (cmdpy_completer_helper (command, text, word));

  /* Check if there was an error.  */
  if (resultobj == NULL)
    return;

  if (PyInt_Check (resultobj.get ()))
    {
      /* User code may also return one of the completion constants,
	 thus requesting that sort of completion.  We are only
	 interested in this kind of return.  */
      long value;

      if (!gdb_py_int_as_long (resultobj.get (), &value))
	{
	  /* Ignore.  */
	  PyErr_Clear ();
	}
      else if (value >= 0 && value < (long) N_COMPLETERS)
	{
	  /* This is the core of this function.  Depending on which
	     completer type the Python function returns, we have to
	     adjust the break characters accordingly.  */
	  set_gdb_completion_word_break_characters
	    (completers[value].completer);
	}
    }
}

/* Called by gdb for command completion.  */

static VEC (char_ptr) *
cmdpy_completer (struct cmd_list_element *command,
		 const char *text, const char *word)
{
  VEC (char_ptr) *result = NULL;

  gdbpy_enter enter_py (get_current_arch (), current_language);

  /* Calling our helper to obtain the PyObject of the Python
     function.  */
  gdbpy_ref<> resultobj (cmdpy_completer_helper (command, text, word));

  /* If the result object of calling the Python function is NULL, it
     means that there was an error.  In this case, just give up and
     return NULL.  */
  if (resultobj == NULL)
    return NULL;

  result = NULL;
  if (PyInt_Check (resultobj.get ()))
    {
      /* User code may also return one of the completion constants,
	 thus requesting that sort of completion.  */
      long value;

      if (! gdb_py_int_as_long (resultobj.get (), &value))
	{
	  /* Ignore.  */
	  PyErr_Clear ();
	}
      else if (value >= 0 && value < (long) N_COMPLETERS)
	result = completers[value].completer (command, text, word);
    }
  else
    {
      gdbpy_ref<> iter (PyObject_GetIter (resultobj.get ()));

      if (iter == NULL)
	return NULL;

      while (true)
	{
	  gdbpy_ref<> elt (PyIter_Next (iter.get ()));
	  if (elt == NULL)
	    break;

	  if (! gdbpy_is_string (elt.get ()))
	    {
	      /* Skip problem elements.  */
	      continue;
	    }
	  gdb::unique_xmalloc_ptr<char>
	    item (python_string_to_host_string (elt.get ()));
	  if (item == NULL)
	    {
	      /* Skip problem elements.  */
	      PyErr_Clear ();
	      continue;
	    }
	  VEC_safe_push (char_ptr, result, item.release ());
	}

      /* If we got some results, ignore problems.  Otherwise, report
	 the problem.  */
      if (result != NULL && PyErr_Occurred ())
	PyErr_Clear ();
    }

  return result;
}

/* Helper for cmdpy_init which locates the command list to use and
   pulls out the command name.

   NAME is the command name list.  The final word in the list is the
   name of the new command.  All earlier words must be existing prefix
   commands.

   *BASE_LIST is set to the final prefix command's list of
   *sub-commands.

   START_LIST is the list in which the search starts.

   This function returns the xmalloc()d name of the new command.  On
   error sets the Python error and returns NULL.  */

char *
gdbpy_parse_command_name (const char *name,
			  struct cmd_list_element ***base_list,
			  struct cmd_list_element **start_list)
{
  struct cmd_list_element *elt;
  int len = strlen (name);
  int i, lastchar;
  char *prefix_text;
  const char *prefix_text2;
  char *result;

  /* Skip trailing whitespace.  */
  for (i = len - 1; i >= 0 && (name[i] == ' ' || name[i] == '\t'); --i)
    ;
  if (i < 0)
    {
      PyErr_SetString (PyExc_RuntimeError, _("No command name found."));
      return NULL;
    }
  lastchar = i;

  /* Find first character of the final word.  */
  for (; i > 0 && (isalnum (name[i - 1])
		   || name[i - 1] == '-'
		   || name[i - 1] == '_');
       --i)
    ;
  result = (char *) xmalloc (lastchar - i + 2);
  memcpy (result, &name[i], lastchar - i + 1);
  result[lastchar - i + 1] = '\0';

  /* Skip whitespace again.  */
  for (--i; i >= 0 && (name[i] == ' ' || name[i] == '\t'); --i)
    ;
  if (i < 0)
    {
      *base_list = start_list;
      return result;
    }

  prefix_text = (char *) xmalloc (i + 2);
  memcpy (prefix_text, name, i + 1);
  prefix_text[i + 1] = '\0';

  prefix_text2 = prefix_text;
  elt = lookup_cmd_1 (&prefix_text2, *start_list, NULL, 1);
  if (elt == NULL || elt == CMD_LIST_AMBIGUOUS)
    {
      PyErr_Format (PyExc_RuntimeError, _("Could not find command prefix %s."),
		    prefix_text);
      xfree (prefix_text);
      xfree (result);
      return NULL;
    }

  if (elt->prefixlist)
    {
      xfree (prefix_text);
      *base_list = elt->prefixlist;
      return result;
    }

  PyErr_Format (PyExc_RuntimeError, _("'%s' is not a prefix command."),
		prefix_text);
  xfree (prefix_text);
  xfree (result);
  return NULL;
}

/* Object initializer; sets up gdb-side structures for command.

   Use: __init__(NAME, COMMAND_CLASS [, COMPLETER_CLASS][, PREFIX]]).

   NAME is the name of the command.  It may consist of multiple words,
   in which case the final word is the name of the new command, and
   earlier words must be prefix commands.

   COMMAND_CLASS is the kind of command.  It should be one of the COMMAND_*
   constants defined in the gdb module.

   COMPLETER_CLASS is the kind of completer.  If not given, the
   "complete" method will be used.  Otherwise, it should be one of the
   COMPLETE_* constants defined in the gdb module.

   If PREFIX is True, then this command is a prefix command.

   The documentation for the command is taken from the doc string for
   the python class.  */

static int
cmdpy_init (PyObject *self, PyObject *args, PyObject *kw)
{
  cmdpy_object *obj = (cmdpy_object *) self;
  const char *name;
  int cmdtype;
  int completetype = -1;
  char *docstring = NULL;
  struct cmd_list_element **cmd_list;
  char *cmd_name, *pfx_name;
  static const char *keywords[] = { "name", "command_class", "completer_class",
				    "prefix", NULL };
  PyObject *is_prefix = NULL;
  int cmp;

  if (obj->command)
    {
      /* Note: this is apparently not documented in Python.  We return
	 0 for success, -1 for failure.  */
      PyErr_Format (PyExc_RuntimeError,
		    _("Command object already initialized."));
      return -1;
    }

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "si|iO",
					keywords, &name, &cmdtype,
					&completetype, &is_prefix))
    return -1;

  if (cmdtype != no_class && cmdtype != class_run
      && cmdtype != class_vars && cmdtype != class_stack
      && cmdtype != class_files && cmdtype != class_support
      && cmdtype != class_info && cmdtype != class_breakpoint
      && cmdtype != class_trace && cmdtype != class_obscure
      && cmdtype != class_maintenance && cmdtype != class_user)
    {
      PyErr_Format (PyExc_RuntimeError, _("Invalid command class argument."));
      return -1;
    }

  if (completetype < -1 || completetype >= (int) N_COMPLETERS)
    {
      PyErr_Format (PyExc_RuntimeError,
		    _("Invalid completion type argument."));
      return -1;
    }

  cmd_name = gdbpy_parse_command_name (name, &cmd_list, &cmdlist);
  if (! cmd_name)
    return -1;

  pfx_name = NULL;
  if (is_prefix != NULL)
    {
      cmp = PyObject_IsTrue (is_prefix);
      if (cmp == 1)
	{
	  int i, out;
	
	  /* Make a normalized form of the command name.  */
	  pfx_name = (char *) xmalloc (strlen (name) + 2);
	
	  i = 0;
	  out = 0;
	  while (name[i])
	    {
	      /* Skip whitespace.  */
	      while (name[i] == ' ' || name[i] == '\t')
		++i;
	      /* Copy non-whitespace characters.  */
	      while (name[i] && name[i] != ' ' && name[i] != '\t')
		pfx_name[out++] = name[i++];
	      /* Add a single space after each word -- including the final
		 word.  */
	      pfx_name[out++] = ' ';
	    }
	  pfx_name[out] = '\0';
	}
      else if (cmp < 0)
	{
	  xfree (cmd_name);
	  return -1;
	}
    }
  if (PyObject_HasAttr (self, gdbpy_doc_cst))
    {
      gdbpy_ref<> ds_obj (PyObject_GetAttr (self, gdbpy_doc_cst));

      if (ds_obj != NULL && gdbpy_is_string (ds_obj.get ()))
	{
	  docstring = python_string_to_host_string (ds_obj.get ()).release ();
	  if (docstring == NULL)
	    {
	      xfree (cmd_name);
	      xfree (pfx_name);
	      return -1;
	    }
	}
    }
  if (! docstring)
    docstring = xstrdup (_("This command is not documented."));

  Py_INCREF (self);

  TRY
    {
      struct cmd_list_element *cmd;

      if (pfx_name)
	{
	  int allow_unknown;

	  /* If we have our own "invoke" method, then allow unknown
	     sub-commands.  */
	  allow_unknown = PyObject_HasAttr (self, invoke_cst);
	  cmd = add_prefix_cmd (cmd_name, (enum command_class) cmdtype,
				NULL, docstring, &obj->sub_list,
				pfx_name, allow_unknown, cmd_list);
	}
      else
	cmd = add_cmd (cmd_name, (enum command_class) cmdtype, NULL,
		       docstring, cmd_list);

      /* There appears to be no API to set this.  */
      cmd->func = cmdpy_function;
      cmd->destroyer = cmdpy_destroyer;

      obj->command = cmd;
      set_cmd_context (cmd, self);
      set_cmd_completer (cmd, ((completetype == -1) ? cmdpy_completer
			       : completers[completetype].completer));
      if (completetype == -1)
	set_cmd_completer_handle_brkchars (cmd,
					   cmdpy_completer_handle_brkchars);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      xfree (cmd_name);
      xfree (docstring);
      xfree (pfx_name);
      Py_DECREF (self);
      PyErr_Format (except.reason == RETURN_QUIT
		    ? PyExc_KeyboardInterrupt : PyExc_RuntimeError,
		    "%s", except.message);
      return -1;
    }
  END_CATCH

  return 0;
}



/* Initialize the 'commands' code.  */

int
gdbpy_initialize_commands (void)
{
  int i;

  cmdpy_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&cmdpy_object_type) < 0)
    return -1;

  /* Note: alias and user are special; pseudo appears to be unused,
     and there is no reason to expose tui, I think.  */
  if (PyModule_AddIntConstant (gdb_module, "COMMAND_NONE", no_class) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_RUNNING", class_run) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_DATA", class_vars) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_STACK", class_stack) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_FILES", class_files) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_SUPPORT",
				  class_support) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_STATUS", class_info) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_BREAKPOINTS",
				  class_breakpoint) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_TRACEPOINTS",
				  class_trace) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_OBSCURE",
				  class_obscure) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_MAINTENANCE",
				  class_maintenance) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_USER", class_user) < 0)
    return -1;

  for (i = 0; i < N_COMPLETERS; ++i)
    {
      if (PyModule_AddIntConstant (gdb_module, completers[i].name, i) < 0)
	return -1;
    }

  if (gdb_pymodule_addobject (gdb_module, "Command",
			      (PyObject *) &cmdpy_object_type) < 0)
    return -1;

  invoke_cst = PyString_FromString ("invoke");
  if (invoke_cst == NULL)
    return -1;
  complete_cst = PyString_FromString ("complete");
  if (complete_cst == NULL)
    return -1;

  return 0;
}



static PyMethodDef cmdpy_object_methods[] =
{
  { "dont_repeat", cmdpy_dont_repeat, METH_NOARGS,
    "Prevent command repetition when user enters empty line." },

  { 0 }
};

PyTypeObject cmdpy_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Command",		  /*tp_name*/
  sizeof (cmdpy_object),	  /*tp_basicsize*/
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
  "GDB command object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  cmdpy_object_methods,		  /* tp_methods */
  0,				  /* tp_members */
  0,				  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  cmdpy_init,			  /* tp_init */
  0,				  /* tp_alloc */
};



/* Utility to build a buildargv-like result from ARGS.
   This intentionally parses arguments the way libiberty/argv.c:buildargv
   does.  It splits up arguments in a reasonable way, and we want a standard
   way of parsing arguments.  Several gdb commands use buildargv to parse their
   arguments.  Plus we want to be able to write compatible python
   implementations of gdb commands.  */

PyObject *
gdbpy_string_to_argv (PyObject *self, PyObject *args)
{
  const char *input;

  if (!PyArg_ParseTuple (args, "s", &input))
    return NULL;

  gdbpy_ref<> py_argv (PyList_New (0));
  if (py_argv == NULL)
    return NULL;

  /* buildargv uses NULL to represent an empty argument list, but we can't use
     that in Python.  Instead, if ARGS is "" then return an empty list.
     This undoes the NULL -> "" conversion that cmdpy_function does.  */

  if (*input != '\0')
    {
      char **c_argv = gdb_buildargv (input);
      int i;

      for (i = 0; c_argv[i] != NULL; ++i)
	{
	  gdbpy_ref<> argp (PyString_FromString (c_argv[i]));

	  if (argp == NULL
	      || PyList_Append (py_argv.get (), argp.get ()) < 0)
	    {
	      freeargv (c_argv);
	      return NULL;
	    }
	}

      freeargv (c_argv);
    }

  return py_argv.release ();
}
