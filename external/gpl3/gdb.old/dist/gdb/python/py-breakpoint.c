/* Python interface to breakpoints

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
#include "value.h"
#include "exceptions.h"
#include "python-internal.h"
#include "python.h"
#include "charset.h"
#include "breakpoint.h"
#include "gdbcmd.h"
#include "gdbthread.h"
#include "observer.h"
#include "cli/cli-script.h"
#include "ada-lang.h"
#include "arch-utils.h"
#include "language.h"

/* Number of live breakpoints.  */
static int bppy_live;

/* Variables used to pass information between the Breakpoint
   constructor and the breakpoint-created hook function.  */
gdbpy_breakpoint_object *bppy_pending_object;

/* Function that is called when a Python condition is evaluated.  */
static char * const stop_func = "stop";

/* This is used to initialize various gdb.bp_* constants.  */
struct pybp_code
{
  /* The name.  */
  const char *name;
  /* The code.  */
  int code;
};

/* Entries related to the type of user set breakpoints.  */
static struct pybp_code pybp_codes[] =
{
  { "BP_NONE", bp_none},
  { "BP_BREAKPOINT", bp_breakpoint},
  { "BP_WATCHPOINT", bp_watchpoint},
  { "BP_HARDWARE_WATCHPOINT", bp_hardware_watchpoint},
  { "BP_READ_WATCHPOINT", bp_read_watchpoint},
  { "BP_ACCESS_WATCHPOINT", bp_access_watchpoint},
  {NULL} /* Sentinel.  */
};

/* Entries related to the type of watchpoint.  */
static struct pybp_code pybp_watch_types[] =
{
  { "WP_READ", hw_read},
  { "WP_WRITE", hw_write},
  { "WP_ACCESS", hw_access},
  {NULL} /* Sentinel.  */
};

/* Python function which checks the validity of a breakpoint object.  */
static PyObject *
bppy_is_valid (PyObject *self, PyObject *args)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;

  if (self_bp->bp)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

/* Python function to test whether or not the breakpoint is enabled.  */
static PyObject *
bppy_get_enabled (PyObject *self, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;

  BPPY_REQUIRE_VALID (self_bp);
  if (! self_bp->bp)
    Py_RETURN_FALSE;
  if (self_bp->bp->enable_state == bp_enabled)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

/* Python function to test whether or not the breakpoint is silent.  */
static PyObject *
bppy_get_silent (PyObject *self, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;

  BPPY_REQUIRE_VALID (self_bp);
  if (self_bp->bp->silent)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

/* Python function to set the enabled state of a breakpoint.  */
static int
bppy_set_enabled (PyObject *self, PyObject *newvalue, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;
  int cmp;
  volatile struct gdb_exception except;

  BPPY_SET_REQUIRE_VALID (self_bp);

  if (newvalue == NULL)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete `enabled' attribute."));

      return -1;
    }
  else if (! PyBool_Check (newvalue))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The value of `enabled' must be a boolean."));
      return -1;
    }

  cmp = PyObject_IsTrue (newvalue);
  if (cmp < 0)
    return -1;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      if (cmp == 1)
	enable_breakpoint (self_bp->bp);
      else
	disable_breakpoint (self_bp->bp);
    }
  GDB_PY_SET_HANDLE_EXCEPTION (except);

  return 0;
}

/* Python function to set the 'silent' state of a breakpoint.  */
static int
bppy_set_silent (PyObject *self, PyObject *newvalue, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;
  int cmp;

  BPPY_SET_REQUIRE_VALID (self_bp);

  if (newvalue == NULL)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete `silent' attribute."));
      return -1;
    }
  else if (! PyBool_Check (newvalue))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The value of `silent' must be a boolean."));
      return -1;
    }

  cmp = PyObject_IsTrue (newvalue);
  if (cmp < 0)
    return -1;
  else
    breakpoint_set_silent (self_bp->bp, cmp);

  return 0;
}

/* Python function to set the thread of a breakpoint.  */
static int
bppy_set_thread (PyObject *self, PyObject *newvalue, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;
  long id;

  BPPY_SET_REQUIRE_VALID (self_bp);

  if (newvalue == NULL)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete `thread' attribute."));
      return -1;
    }
  else if (PyInt_Check (newvalue))
    {
      if (! gdb_py_int_as_long (newvalue, &id))
	return -1;

      if (! valid_thread_id (id))
	{
	  PyErr_SetString (PyExc_RuntimeError,
			   _("Invalid thread ID."));
	  return -1;
	}
    }
  else if (newvalue == Py_None)
    id = -1;
  else
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The value of `thread' must be an integer or None."));
      return -1;
    }

  breakpoint_set_thread (self_bp->bp, id);

  return 0;
}

/* Python function to set the (Ada) task of a breakpoint.  */
static int
bppy_set_task (PyObject *self, PyObject *newvalue, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;
  long id;
  int valid_id = 0;
  volatile struct gdb_exception except;

  BPPY_SET_REQUIRE_VALID (self_bp);

  if (newvalue == NULL)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete `task' attribute."));
      return -1;
    }
  else if (PyInt_Check (newvalue))
    {
      if (! gdb_py_int_as_long (newvalue, &id))
	return -1;

      TRY_CATCH (except, RETURN_MASK_ALL)
	{
	  valid_id = valid_task_id (id);
	}
      GDB_PY_SET_HANDLE_EXCEPTION (except);

      if (! valid_id)
	{
	  PyErr_SetString (PyExc_RuntimeError,
			   _("Invalid task ID."));
	  return -1;
	}
    }
  else if (newvalue == Py_None)
    id = 0;
  else
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The value of `task' must be an integer or None."));
      return -1;
    }

  breakpoint_set_task (self_bp->bp, id);

  return 0;
}

/* Python function which deletes the underlying GDB breakpoint.  This
   triggers the breakpoint_deleted observer which will call
   gdbpy_breakpoint_deleted; that function cleans up the Python
   sections.  */

static PyObject *
bppy_delete_breakpoint (PyObject *self, PyObject *args)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;
  volatile struct gdb_exception except;

  BPPY_REQUIRE_VALID (self_bp);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      delete_breakpoint (self_bp->bp);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  Py_RETURN_NONE;
}


/* Python function to set the ignore count of a breakpoint.  */
static int
bppy_set_ignore_count (PyObject *self, PyObject *newvalue, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;
  long value;
  volatile struct gdb_exception except;

  BPPY_SET_REQUIRE_VALID (self_bp);

  if (newvalue == NULL)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete `ignore_count' attribute."));
      return -1;
    }
  else if (! PyInt_Check (newvalue))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The value of `ignore_count' must be an integer."));
      return -1;
    }

  if (! gdb_py_int_as_long (newvalue, &value))
    return -1;

  if (value < 0)
    value = 0;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      set_ignore_count (self_bp->number, (int) value, 0);
    }
  GDB_PY_SET_HANDLE_EXCEPTION (except);

  return 0;
}

/* Python function to set the hit count of a breakpoint.  */
static int
bppy_set_hit_count (PyObject *self, PyObject *newvalue, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;

  BPPY_SET_REQUIRE_VALID (self_bp);

  if (newvalue == NULL)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete `hit_count' attribute."));
      return -1;
    }
  else
    {
      long value;

      if (! gdb_py_int_as_long (newvalue, &value))
	return -1;

      if (value != 0)
	{
	  PyErr_SetString (PyExc_AttributeError,
			   _("The value of `hit_count' must be zero."));
	  return -1;
	}
    }

  self_bp->bp->hit_count = 0;

  return 0;
}

/* Python function to get the location of a breakpoint.  */
static PyObject *
bppy_get_location (PyObject *self, void *closure)
{
  char *str;
  gdbpy_breakpoint_object *obj = (gdbpy_breakpoint_object *) self;

  BPPY_REQUIRE_VALID (obj);

  if (obj->bp->type != bp_breakpoint)
    Py_RETURN_NONE;

  str = obj->bp->addr_string;

  if (! str)
    str = "";
  return PyString_Decode (str, strlen (str), host_charset (), NULL);
}

/* Python function to get the breakpoint expression.  */
static PyObject *
bppy_get_expression (PyObject *self, void *closure)
{
  char *str;
  gdbpy_breakpoint_object *obj = (gdbpy_breakpoint_object *) self;
  struct watchpoint *wp;

  BPPY_REQUIRE_VALID (obj);

  if (!is_watchpoint (obj->bp))
    Py_RETURN_NONE;

  wp = (struct watchpoint *) obj->bp;

  str = wp->exp_string;
  if (! str)
    str = "";

  return PyString_Decode (str, strlen (str), host_charset (), NULL);
}

/* Python function to get the condition expression of a breakpoint.  */
static PyObject *
bppy_get_condition (PyObject *self, void *closure)
{
  char *str;
  gdbpy_breakpoint_object *obj = (gdbpy_breakpoint_object *) self;

  BPPY_REQUIRE_VALID (obj);

  str = obj->bp->cond_string;
  if (! str)
    Py_RETURN_NONE;

  return PyString_Decode (str, strlen (str), host_charset (), NULL);
}

/* Returns 0 on success.  Returns -1 on error, with a python exception set.
   */

static int
bppy_set_condition (PyObject *self, PyObject *newvalue, void *closure)
{
  char *exp;
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;
  volatile struct gdb_exception except;

  BPPY_SET_REQUIRE_VALID (self_bp);

  if (newvalue == NULL)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete `condition' attribute."));
      return -1;
    }
  else if (newvalue == Py_None)
    exp = "";
  else
    {
      exp = python_string_to_host_string (newvalue);
      if (exp == NULL)
	return -1;
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      set_breakpoint_condition (self_bp->bp, exp, 0);
    }

  if (newvalue != Py_None)
    xfree (exp);

  GDB_PY_SET_HANDLE_EXCEPTION (except);

  return 0;
}

/* Python function to get the commands attached to a breakpoint.  */
static PyObject *
bppy_get_commands (PyObject *self, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;
  struct breakpoint *bp = self_bp->bp;
  long length;
  volatile struct gdb_exception except;
  struct ui_file *string_file;
  struct cleanup *chain;
  PyObject *result;
  char *cmdstr;

  BPPY_REQUIRE_VALID (self_bp);

  if (! self_bp->bp->commands)
    Py_RETURN_NONE;

  string_file = mem_fileopen ();
  chain = make_cleanup_ui_file_delete (string_file);

  ui_out_redirect (current_uiout, string_file);
  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      print_command_lines (current_uiout, breakpoint_commands (bp), 0);
    }
  ui_out_redirect (current_uiout, NULL);
  if (except.reason < 0)
    {
      do_cleanups (chain);
      gdbpy_convert_exception (except);
      return NULL;
    }

  cmdstr = ui_file_xstrdup (string_file, &length);
  make_cleanup (xfree, cmdstr);
  result = PyString_Decode (cmdstr, strlen (cmdstr), host_charset (), NULL);
  do_cleanups (chain);
  return result;
}

/* Python function to get the breakpoint type.  */
static PyObject *
bppy_get_type (PyObject *self, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;

  BPPY_REQUIRE_VALID (self_bp);

  return PyInt_FromLong (self_bp->bp->type);
}

/* Python function to get the visibility of the breakpoint.  */

static PyObject *
bppy_get_visibility (PyObject *self, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;

  BPPY_REQUIRE_VALID (self_bp);

  if (self_bp->bp->number < 0)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Python function to determine if the breakpoint is a temporary
   breakpoint.  */

static PyObject *
bppy_get_temporary (PyObject *self, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;

  BPPY_REQUIRE_VALID (self_bp);

  if (self_bp->bp->disposition == disp_del
      || self_bp->bp->disposition == disp_del_at_next_stop)
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Python function to get the breakpoint's number.  */
static PyObject *
bppy_get_number (PyObject *self, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;

  BPPY_REQUIRE_VALID (self_bp);

  return PyInt_FromLong (self_bp->number);
}

/* Python function to get the breakpoint's thread ID.  */
static PyObject *
bppy_get_thread (PyObject *self, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;

  BPPY_REQUIRE_VALID (self_bp);

  if (self_bp->bp->thread == -1)
    Py_RETURN_NONE;

  return PyInt_FromLong (self_bp->bp->thread);
}

/* Python function to get the breakpoint's task ID (in Ada).  */
static PyObject *
bppy_get_task (PyObject *self, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;

  BPPY_REQUIRE_VALID (self_bp);

  if (self_bp->bp->task == 0)
    Py_RETURN_NONE;

  return PyInt_FromLong (self_bp->bp->task);
}

/* Python function to get the breakpoint's hit count.  */
static PyObject *
bppy_get_hit_count (PyObject *self, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;

  BPPY_REQUIRE_VALID (self_bp);

  return PyInt_FromLong (self_bp->bp->hit_count);
}

/* Python function to get the breakpoint's ignore count.  */
static PyObject *
bppy_get_ignore_count (PyObject *self, void *closure)
{
  gdbpy_breakpoint_object *self_bp = (gdbpy_breakpoint_object *) self;

  BPPY_REQUIRE_VALID (self_bp);

  return PyInt_FromLong (self_bp->bp->ignore_count);
}

/* Python function to create a new breakpoint.  */
static int
bppy_init (PyObject *self, PyObject *args, PyObject *kwargs)
{
  static char *keywords[] = { "spec", "type", "wp_class", "internal",
			      "temporary", NULL };
  const char *spec;
  int type = bp_breakpoint;
  int access_type = hw_write;
  PyObject *internal = NULL;
  PyObject *temporary = NULL;
  int internal_bp = 0;
  int temporary_bp = 0;
  volatile struct gdb_exception except;

  if (! PyArg_ParseTupleAndKeywords (args, kwargs, "s|iiOO", keywords,
				     &spec, &type, &access_type,
				     &internal, &temporary))
    return -1;

  if (internal)
    {
      internal_bp = PyObject_IsTrue (internal);
      if (internal_bp == -1)
	return -1;
    }

  if (temporary != NULL)
    {
      temporary_bp = PyObject_IsTrue (temporary);
      if (temporary_bp == -1)
	return -1;
    }

  bppy_pending_object = (gdbpy_breakpoint_object *) self;
  bppy_pending_object->number = -1;
  bppy_pending_object->bp = NULL;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      char *copy = xstrdup (spec);
      struct cleanup *cleanup = make_cleanup (xfree, copy);

      switch (type)
	{
	case bp_breakpoint:
	  {
	    create_breakpoint (python_gdbarch,
			       copy, NULL, -1, NULL,
			       0,
			       temporary_bp, bp_breakpoint,
			       0,
			       AUTO_BOOLEAN_TRUE,
			       &bkpt_breakpoint_ops,
			       0, 1, internal_bp, 0);
	    break;
	  }
        case bp_watchpoint:
	  {
	    if (access_type == hw_write)
	      watch_command_wrapper (copy, 0, internal_bp);
	    else if (access_type == hw_access)
	      awatch_command_wrapper (copy, 0, internal_bp);
	    else if (access_type == hw_read)
	      rwatch_command_wrapper (copy, 0, internal_bp);
	    else
	      error(_("Cannot understand watchpoint access type."));
	    break;
	  }
	default:
	  error(_("Do not understand breakpoint type to set."));
	}

      do_cleanups (cleanup);
    }
  if (except.reason < 0)
    {
      PyErr_Format (except.reason == RETURN_QUIT
		    ? PyExc_KeyboardInterrupt : PyExc_RuntimeError,
		    "%s", except.message);
      return -1;
    }

  BPPY_SET_REQUIRE_VALID ((gdbpy_breakpoint_object *) self);
  return 0;
}



static int
build_bp_list (struct breakpoint *b, void *arg)
{
  PyObject *list = arg;
  PyObject *bp = (PyObject *) b->py_bp_object;
  int iserr = 0;

  /* Not all breakpoints will have a companion Python object.
     Only breakpoints that were created via bppy_new, or
     breakpoints that were created externally and are tracked by
     the Python Scripting API.  */
  if (bp)
    iserr = PyList_Append (list, bp);

  if (iserr == -1)
    return 1;

  return 0;
}

/* Static function to return a tuple holding all breakpoints.  */

PyObject *
gdbpy_breakpoints (PyObject *self, PyObject *args)
{
  PyObject *list, *tuple;

  if (bppy_live == 0)
    Py_RETURN_NONE;

  list = PyList_New (0);
  if (!list)
    return NULL;

  /* If iteratre_over_breakpoints returns non NULL it signals an error
     condition.  In that case abandon building the list and return
     NULL.  */
  if (iterate_over_breakpoints (build_bp_list, list) != NULL)
    {
      Py_DECREF (list);
      return NULL;
    }

  tuple = PyList_AsTuple (list);
  Py_DECREF (list);

  return tuple;
}

/* Call the "stop" method (if implemented) in the breakpoint
   class.  If the method returns True, the inferior  will be
   stopped at the breakpoint.  Otherwise the inferior will be
   allowed to continue.  */

int
gdbpy_should_stop (struct gdbpy_breakpoint_object *bp_obj)
{
  int stop = 1;

  PyObject *py_bp = (PyObject *) bp_obj;
  struct breakpoint *b = bp_obj->bp;
  struct gdbarch *garch = b->gdbarch ? b->gdbarch : get_current_arch ();
  struct cleanup *cleanup = ensure_python_env (garch, current_language);

  if (bp_obj->is_finish_bp)
    bpfinishpy_pre_stop_hook (bp_obj);

  if (PyObject_HasAttrString (py_bp, stop_func))
    {
      PyObject *result = PyObject_CallMethod (py_bp, stop_func, NULL);

      if (result)
	{
	  int evaluate = PyObject_IsTrue (result);

	  if (evaluate == -1)
	    gdbpy_print_stack ();

	  /* If the "stop" function returns False that means
	     the Python breakpoint wants GDB to continue.  */
	  if (! evaluate)
	    stop = 0;

	  Py_DECREF (result);
	}
      else
	gdbpy_print_stack ();
    }

  if (bp_obj->is_finish_bp)
    bpfinishpy_post_stop_hook (bp_obj);

  do_cleanups (cleanup);

  return stop;
}

/* Checks if the  "stop" method exists in this breakpoint.
   Used by condition_command to ensure mutual exclusion of breakpoint
   conditions.  */

int
gdbpy_breakpoint_has_py_cond (struct gdbpy_breakpoint_object *bp_obj)
{
  int has_func = 0;
  PyObject *py_bp = (PyObject *) bp_obj;
  struct gdbarch *garch = bp_obj->bp->gdbarch ? bp_obj->bp->gdbarch :
    get_current_arch ();
  struct cleanup *cleanup = ensure_python_env (garch, current_language);

  if (py_bp != NULL)
    has_func = PyObject_HasAttrString (py_bp, stop_func);

  do_cleanups (cleanup);

  return has_func;
}



/* Event callback functions.  */

/* Callback that is used when a breakpoint is created.  This function
   will create a new Python breakpoint object.  */
static void
gdbpy_breakpoint_created (struct breakpoint *bp)
{
  gdbpy_breakpoint_object *newbp;
  PyGILState_STATE state;

  if (bp->number < 0 && bppy_pending_object == NULL)
    return;

  if (bp->type != bp_breakpoint
      && bp->type != bp_watchpoint
      && bp->type != bp_hardware_watchpoint
      && bp->type != bp_read_watchpoint
      && bp->type != bp_access_watchpoint)
    return;

  state = PyGILState_Ensure ();

  if (bppy_pending_object)
    {
      newbp = bppy_pending_object;
      bppy_pending_object = NULL;
    }
  else
    newbp = PyObject_New (gdbpy_breakpoint_object, &breakpoint_object_type);
  if (newbp)
    {
      newbp->number = bp->number;
      newbp->bp = bp;
      newbp->bp->py_bp_object = newbp;
      newbp->is_finish_bp = 0;
      Py_INCREF (newbp);
      ++bppy_live;
    }
  else
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Error while creating breakpoint from GDB."));
      gdbpy_print_stack ();
    }

  PyGILState_Release (state);
}

/* Callback that is used when a breakpoint is deleted.  This will
   invalidate the corresponding Python object.  */
static void
gdbpy_breakpoint_deleted (struct breakpoint *b)
{
  int num = b->number;
  PyGILState_STATE state;
  struct breakpoint *bp = NULL;
  gdbpy_breakpoint_object *bp_obj;

  state = PyGILState_Ensure ();
  bp = get_breakpoint (num);
  if (bp)
    {
      bp_obj = bp->py_bp_object;
      if (bp_obj)
	{
	  bp_obj->bp = NULL;
	  --bppy_live;
	  Py_DECREF (bp_obj);
	}
    }
  PyGILState_Release (state);
}



/* Initialize the Python breakpoint code.  */
int
gdbpy_initialize_breakpoints (void)
{
  int i;

  breakpoint_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&breakpoint_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_module, "Breakpoint",
			      (PyObject *) &breakpoint_object_type) < 0)
    return -1;

  observer_attach_breakpoint_created (gdbpy_breakpoint_created);
  observer_attach_breakpoint_deleted (gdbpy_breakpoint_deleted);

  /* Add breakpoint types constants.  */
  for (i = 0; pybp_codes[i].name; ++i)
    {
      if (PyModule_AddIntConstant (gdb_module,
				   /* Cast needed for Python 2.4.  */
				   (char *) pybp_codes[i].name,
				   pybp_codes[i].code) < 0)
	return -1;
    }

  /* Add watchpoint types constants.  */
  for (i = 0; pybp_watch_types[i].name; ++i)
    {
      if (PyModule_AddIntConstant (gdb_module,
				   /* Cast needed for Python 2.4.  */
				   (char *) pybp_watch_types[i].name,
				   pybp_watch_types[i].code) < 0)
	return -1;
    }

  return 0;
}



/* Helper function that overrides this Python object's
   PyObject_GenericSetAttr to allow extra validation of the attribute
   being set.  */

static int
local_setattro (PyObject *self, PyObject *name, PyObject *v)
{
  gdbpy_breakpoint_object *obj = (gdbpy_breakpoint_object *) self;
  char *attr = python_string_to_host_string (name);

  if (attr == NULL)
    return -1;

  /* If the attribute trying to be set is the "stop" method,
     but we already have a condition set in the CLI, disallow this
     operation.  */
  if (strcmp (attr, stop_func) == 0 && obj->bp->cond_string)
    {
      xfree (attr);
      PyErr_SetString (PyExc_RuntimeError,
		       _("Cannot set 'stop' method.  There is an " \
			 "existing GDB condition attached to the " \
			 "breakpoint."));
      return -1;
    }

  xfree (attr);

  return PyObject_GenericSetAttr ((PyObject *)self, name, v);
}

static PyGetSetDef breakpoint_object_getset[] = {
  { "enabled", bppy_get_enabled, bppy_set_enabled,
    "Boolean telling whether the breakpoint is enabled.", NULL },
  { "silent", bppy_get_silent, bppy_set_silent,
    "Boolean telling whether the breakpoint is silent.", NULL },
  { "thread", bppy_get_thread, bppy_set_thread,
    "Thread ID for the breakpoint.\n\
If the value is a thread ID (integer), then this is a thread-specific breakpoint.\n\
If the value is None, then this breakpoint is not thread-specific.\n\
No other type of value can be used.", NULL },
  { "task", bppy_get_task, bppy_set_task,
    "Thread ID for the breakpoint.\n\
If the value is a task ID (integer), then this is an Ada task-specific breakpoint.\n\
If the value is None, then this breakpoint is not task-specific.\n\
No other type of value can be used.", NULL },
  { "ignore_count", bppy_get_ignore_count, bppy_set_ignore_count,
    "Number of times this breakpoint should be automatically continued.",
    NULL },
  { "number", bppy_get_number, NULL,
    "Breakpoint's number assigned by GDB.", NULL },
  { "hit_count", bppy_get_hit_count, bppy_set_hit_count,
    "Number of times the breakpoint has been hit.\n\
Can be set to zero to clear the count. No other value is valid\n\
when setting this property.", NULL },
  { "location", bppy_get_location, NULL,
    "Location of the breakpoint, as specified by the user.", NULL},
  { "expression", bppy_get_expression, NULL,
    "Expression of the breakpoint, as specified by the user.", NULL},
  { "condition", bppy_get_condition, bppy_set_condition,
    "Condition of the breakpoint, as specified by the user,\
or None if no condition set."},
  { "commands", bppy_get_commands, NULL,
    "Commands of the breakpoint, as specified by the user."},
  { "type", bppy_get_type, NULL,
    "Type of breakpoint."},
  { "visible", bppy_get_visibility, NULL,
    "Whether the breakpoint is visible to the user."},
  { "temporary", bppy_get_temporary, NULL,
    "Whether this breakpoint is a temporary breakpoint."},
  { NULL }  /* Sentinel.  */
};

static PyMethodDef breakpoint_object_methods[] =
{
  { "is_valid", bppy_is_valid, METH_NOARGS,
    "Return true if this breakpoint is valid, false if not." },
  { "delete", bppy_delete_breakpoint, METH_NOARGS,
    "Delete the underlying GDB breakpoint." },
  { NULL } /* Sentinel.  */
};

PyTypeObject breakpoint_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Breakpoint",		  /*tp_name*/
  sizeof (gdbpy_breakpoint_object), /*tp_basicsize*/
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
  (setattrofunc)local_setattro,   /*tp_setattro */
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
  "GDB breakpoint object",	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  breakpoint_object_methods,	  /* tp_methods */
  0,				  /* tp_members */
  breakpoint_object_getset,	  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  bppy_init,			  /* tp_init */
  0,				  /* tp_alloc */
};
