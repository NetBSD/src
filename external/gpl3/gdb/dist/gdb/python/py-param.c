/* GDB parameters implemented in Python

   Copyright (C) 2008-2023 Free Software Foundation, Inc.

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
#include "language.h"
#include "arch-utils.h"

/* Parameter constants and their values.  */
static struct {
  const char *name;
  int value;
} parm_constants[] =
{
  { "PARAM_BOOLEAN", var_boolean }, /* ARI: var_boolean */
  { "PARAM_AUTO_BOOLEAN", var_auto_boolean },
  { "PARAM_UINTEGER", var_uinteger },
  { "PARAM_INTEGER", var_integer },
  { "PARAM_STRING", var_string },
  { "PARAM_STRING_NOESCAPE", var_string_noescape },
  { "PARAM_OPTIONAL_FILENAME", var_optional_filename },
  { "PARAM_FILENAME", var_filename },
  { "PARAM_ZINTEGER", var_zinteger },
  { "PARAM_ZUINTEGER", var_zuinteger },
  { "PARAM_ZUINTEGER_UNLIMITED", var_zuinteger_unlimited },
  { "PARAM_ENUM", var_enum },
  { NULL, 0 }
};

/* A union that can hold anything described by enum var_types.  */
union parmpy_variable
{
  /* Hold a boolean value.  */
  bool boolval;

  /* Hold an integer value.  */
  int intval;

  /* Hold an auto_boolean.  */
  enum auto_boolean autoboolval;

  /* Hold an unsigned integer value, for uinteger.  */
  unsigned int uintval;

  /* Hold a string, for the various string types.  The std::string is
     new-ed.  */
  std::string *stringval;

  /* Hold a string, for enums.  */
  const char *cstringval;
};

/* A GDB parameter.  */
struct parmpy_object
{
  PyObject_HEAD

  /* The type of the parameter.  */
  enum var_types type;

  /* The value of the parameter.  */
  union parmpy_variable value;

  /* For an enum command, the possible values.  The vector is
     allocated with xmalloc, as is each element.  It is
     NULL-terminated.  */
  const char **enumeration;
};

/* Wraps a setting around an existing parmpy_object.  This abstraction
   is used to manipulate the value in S->VALUE in a type safe manner using
   the setting interface.  */

static setting
make_setting (parmpy_object *s)
{
  if (var_type_uses<bool> (s->type))
    return setting (s->type, &s->value.boolval);
  else if (var_type_uses<int> (s->type))
    return setting (s->type, &s->value.intval);
  else if (var_type_uses<auto_boolean> (s->type))
    return setting (s->type, &s->value.autoboolval);
  else if (var_type_uses<unsigned int> (s->type))
    return setting (s->type, &s->value.uintval);
  else if (var_type_uses<std::string> (s->type))
    return setting (s->type, s->value.stringval);
  else if (var_type_uses<const char *> (s->type))
    return setting (s->type, &s->value.cstringval);
  else
    gdb_assert_not_reached ("unhandled var type");
}

extern PyTypeObject parmpy_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("parmpy_object");

/* Some handy string constants.  */
static PyObject *set_doc_cst;
static PyObject *show_doc_cst;



/* Get an attribute.  */
static PyObject *
get_attr (PyObject *obj, PyObject *attr_name)
{
  if (PyUnicode_Check (attr_name)
      && ! PyUnicode_CompareWithASCIIString (attr_name, "value"))
    {
      parmpy_object *self = (parmpy_object *) obj;

      return gdbpy_parameter_value (make_setting (self));
    }

  return PyObject_GenericGetAttr (obj, attr_name);
}

/* Set a parameter value from a Python value.  Return 0 on success.  Returns
   -1 on error, with a python exception set.  */
static int
set_parameter_value (parmpy_object *self, PyObject *value)
{
  int cmp;

  switch (self->type)
    {
    case var_string:
    case var_string_noescape:
    case var_optional_filename:
    case var_filename:
      if (! gdbpy_is_string (value)
	  && (self->type == var_filename
	      || value != Py_None))
	{
	  PyErr_SetString (PyExc_RuntimeError,
			   _("String required for filename."));

	  return -1;
	}
      if (value == Py_None)
	self->value.stringval->clear ();
      else
	{
	  gdb::unique_xmalloc_ptr<char>
	    string (python_string_to_host_string (value));
	  if (string == NULL)
	    return -1;

	  *self->value.stringval = string.get ();
	}
      break;

    case var_enum:
      {
	int i;

	if (! gdbpy_is_string (value))
	  {
	    PyErr_SetString (PyExc_RuntimeError,
			     _("ENUM arguments must be a string."));
	    return -1;
	  }

	gdb::unique_xmalloc_ptr<char>
	  str (python_string_to_host_string (value));
	if (str == NULL)
	  return -1;
	for (i = 0; self->enumeration[i]; ++i)
	  if (! strcmp (self->enumeration[i], str.get ()))
	    break;
	if (! self->enumeration[i])
	  {
	    PyErr_SetString (PyExc_RuntimeError,
			     _("The value must be member of an enumeration."));
	    return -1;
	  }
	self->value.cstringval = self->enumeration[i];
	break;
      }

    case var_boolean:
      if (! PyBool_Check (value))
	{
	  PyErr_SetString (PyExc_RuntimeError,
			   _("A boolean argument is required."));
	  return -1;
	}
      cmp = PyObject_IsTrue (value);
      if (cmp < 0)
	  return -1;
      self->value.boolval = cmp;
      break;

    case var_auto_boolean:
      if (! PyBool_Check (value) && value != Py_None)
	{
	  PyErr_SetString (PyExc_RuntimeError,
			   _("A boolean or None is required"));
	  return -1;
	}

      if (value == Py_None)
	self->value.autoboolval = AUTO_BOOLEAN_AUTO;
      else
	{
	  cmp = PyObject_IsTrue (value);
	  if (cmp < 0 )
	    return -1;	
	  if (cmp == 1)
	    self->value.autoboolval = AUTO_BOOLEAN_TRUE;
	  else
	    self->value.autoboolval = AUTO_BOOLEAN_FALSE;
	}
      break;

    case var_integer:
    case var_zinteger:
    case var_uinteger:
    case var_zuinteger:
    case var_zuinteger_unlimited:
      {
	long l;
	int ok;

	if (value == Py_None
	    && (self->type == var_uinteger || self->type == var_integer))
	  l = 0;
	else if (value == Py_None && self->type == var_zuinteger_unlimited)
	  l = -1;
	else if (!PyLong_Check (value))
	  {
	    PyErr_SetString (PyExc_RuntimeError,
			     _("The value must be integer."));
	    return -1;
	  }
	else if (! gdb_py_int_as_long (value, &l))
	  return -1;

	switch (self->type)
	  {
	  case var_uinteger:
	    if (l == 0)
	      l = UINT_MAX;
	    /* Fall through.  */
	  case var_zuinteger:
	    ok = (l >= 0 && l <= UINT_MAX);
	    break;

	  case var_zuinteger_unlimited:
	    ok = (l >= -1 && l <= INT_MAX);
	    break;

	  case var_integer:
	    ok = (l >= INT_MIN && l <= INT_MAX);
	    if (l == 0)
	      l = INT_MAX;
	    break;

	  case var_zinteger:
	    ok = (l >= INT_MIN && l <= INT_MAX);
	    break;

	  default:
	    gdb_assert_not_reached ("unknown var_ constant");
	  }

	if (! ok)
	  {
	    PyErr_SetString (PyExc_RuntimeError,
			     _("Range exceeded."));
	    return -1;
	  }

	if (self->type == var_uinteger || self->type == var_zuinteger)
	  self->value.uintval = (unsigned) l;
	else
	  self->value.intval = (int) l;
	break;
      }

    default:
      PyErr_SetString (PyExc_RuntimeError,
		       _("Unhandled type in parameter value."));
      return -1;
    }

  return 0;
}

/* Set an attribute.  Returns -1 on error, with a python exception set.  */
static int
set_attr (PyObject *obj, PyObject *attr_name, PyObject *val)
{
  if (PyUnicode_Check (attr_name)
      && ! PyUnicode_CompareWithASCIIString (attr_name, "value"))
    {
      if (!val)
	{
	  PyErr_SetString (PyExc_RuntimeError,
			   _("Cannot delete a parameter's value."));
	  return -1;
	}
      return set_parameter_value ((parmpy_object *) obj, val);
    }

  return PyObject_GenericSetAttr (obj, attr_name, val);
}

/* Build up the path to command C, but drop the first component of the
   command prefix.  This is only intended for use with the set/show
   parameters this file deals with, the first prefix should always be
   either 'set' or 'show'.

   As an example, if this full command is 'set prefix_a prefix_b command'
   this function will return the string 'prefix_a prefix_b command'.  */

static std::string
full_cmd_name_without_first_prefix (struct cmd_list_element *c)
{
  std::vector<std::string> components
    = c->command_components ();
  gdb_assert (components.size () > 1);
  std::string result = components[1];
  for (int i = 2; i < components.size (); ++i)
    result += " " + components[i];
  return result;
}

/* The different types of documentation string.  */

enum doc_string_type
{
  doc_string_set,
  doc_string_show,
  doc_string_description
};

/* A helper function which returns a documentation string for an
   object. */

static gdb::unique_xmalloc_ptr<char>
get_doc_string (PyObject *object, enum doc_string_type doc_type,
		const char *cmd_name)
{
  gdb::unique_xmalloc_ptr<char> result;

  PyObject *attr = nullptr;
  switch (doc_type)
    {
    case doc_string_set:
      attr = set_doc_cst;
      break;
    case doc_string_show:
      attr = show_doc_cst;
      break;
    case doc_string_description:
      attr = gdbpy_doc_cst;
      break;
    }
  gdb_assert (attr != nullptr);

  if (PyObject_HasAttr (object, attr))
    {
      gdbpy_ref<> ds_obj (PyObject_GetAttr (object, attr));

      if (ds_obj != NULL && gdbpy_is_string (ds_obj.get ()))
	{
	  result = python_string_to_host_string (ds_obj.get ());
	  if (result == NULL)
	    gdbpy_print_stack ();
	  else if (doc_type == doc_string_description)
	    result = gdbpy_fix_doc_string_indentation (std::move (result));
	}
    }

  if (result == nullptr)
    {
      if (doc_type == doc_string_description)
	result.reset (xstrdup (_("This command is not documented.")));
      else
	{
	  if (doc_type == doc_string_show)
	    result = xstrprintf (_("Show the current value of '%s'."),
				 cmd_name);
	  else
	    result = xstrprintf (_("Set the current value of '%s'."),
				 cmd_name);
	}
    }
  return result;
}

/* Helper function which will execute a METHOD in OBJ passing the
   argument ARG.  ARG can be NULL.  METHOD should return a Python
   string.  If this function returns NULL, there has been an error and
   the appropriate exception set.  */
static gdb::unique_xmalloc_ptr<char>
call_doc_function (PyObject *obj, PyObject *method, PyObject *arg)
{
  gdb::unique_xmalloc_ptr<char> data;
  gdbpy_ref<> result (PyObject_CallMethodObjArgs (obj, method, arg, NULL));

  if (result == NULL)
    return NULL;

  if (gdbpy_is_string (result.get ()))
    {
      data = python_string_to_host_string (result.get ());
      if (! data)
	return NULL;
    }
  else
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Parameter must return a string value."));
      return NULL;
    }

  return data;
}

/* A callback function that is registered against the respective
   add_setshow_* set_doc prototype.  This function calls the Python function
   "get_set_string" if it exists, which will return a string.  That string
   is then printed.  If "get_set_string" does not exist, or returns an
   empty string, then nothing is printed.  */
static void
get_set_value (const char *args, int from_tty,
	       struct cmd_list_element *c)
{
  PyObject *obj = (PyObject *) c->context ();
  gdb::unique_xmalloc_ptr<char> set_doc_string;

  gdbpy_enter enter_py;
  gdbpy_ref<> set_doc_func (PyUnicode_FromString ("get_set_string"));

  if (set_doc_func == NULL)
    {
      gdbpy_print_stack ();
      return;
    }

  if (PyObject_HasAttr (obj, set_doc_func.get ()))
    {
      set_doc_string = call_doc_function (obj, set_doc_func.get (), NULL);
      if (! set_doc_string)
	gdbpy_handle_exception ();
    }

  const char *str = set_doc_string.get ();
  if (str != nullptr && str[0] != '\0')
    gdb_printf ("%s\n", str);
}

/* A callback function that is registered against the respective
   add_setshow_* show_doc prototype.  This function will either call
   the Python function "get_show_string" or extract the Python
   attribute "show_doc" and return the contents as a string.  If
   neither exist, insert a string indicating the Parameter is not
   documented.  */
static void
get_show_value (struct ui_file *file, int from_tty,
		struct cmd_list_element *c,
		const char *value)
{
  PyObject *obj = (PyObject *) c->context ();
  gdb::unique_xmalloc_ptr<char> show_doc_string;

  gdbpy_enter enter_py;
  gdbpy_ref<> show_doc_func (PyUnicode_FromString ("get_show_string"));

  if (show_doc_func == NULL)
    {
      gdbpy_print_stack ();
      return;
    }

  if (PyObject_HasAttr (obj, show_doc_func.get ()))
    {
      gdbpy_ref<> val_obj (PyUnicode_FromString (value));

      if (val_obj == NULL)
	{
	  gdbpy_print_stack ();
	  return;
	}

      show_doc_string = call_doc_function (obj, show_doc_func.get (),
					   val_obj.get ());
      if (! show_doc_string)
	{
	  gdbpy_print_stack ();
	  return;
	}

      gdb_printf (file, "%s\n", show_doc_string.get ());
    }
  else
    {
      /* If there is no 'get_show_string' callback then we want to show
	 something sensible here.  In older versions of GDB (< 7.3) we
	 didn't support 'get_show_string', and instead we just made use of
	 GDB's builtin use of the show_doc.  However, GDB's builtin
	 show_doc adjustment is not i18n friendly, so, instead, we just
	 print this generic string.  */
      std::string cmd_path = full_cmd_name_without_first_prefix (c);
      gdb_printf (file, _("The current value of '%s' is \"%s\".\n"),
		  cmd_path.c_str (), value);
    }
}


/* A helper function that dispatches to the appropriate add_setshow
   function.  */
static void
add_setshow_generic (int parmclass, enum command_class cmdclass,
		     gdb::unique_xmalloc_ptr<char> cmd_name,
		     parmpy_object *self,
		     const char *set_doc, const char *show_doc,
		     const char *help_doc,
		     struct cmd_list_element **set_list,
		     struct cmd_list_element **show_list)
{
  set_show_commands commands;

  switch (parmclass)
    {
    case var_boolean:
      commands = add_setshow_boolean_cmd (cmd_name.get (), cmdclass,
					  &self->value.boolval, set_doc,
					  show_doc, help_doc, get_set_value,
					  get_show_value, set_list, show_list);

      break;

    case var_auto_boolean:
      commands = add_setshow_auto_boolean_cmd (cmd_name.get (), cmdclass,
					       &self->value.autoboolval,
					       set_doc, show_doc, help_doc,
					       get_set_value, get_show_value,
					       set_list, show_list);
      break;

    case var_uinteger:
      commands = add_setshow_uinteger_cmd (cmd_name.get (), cmdclass,
					   &self->value.uintval, set_doc,
					   show_doc, help_doc, get_set_value,
					   get_show_value, set_list, show_list);
      break;

    case var_integer:
      commands = add_setshow_integer_cmd (cmd_name.get (), cmdclass,
					  &self->value.intval, set_doc,
					  show_doc, help_doc, get_set_value,
					  get_show_value, set_list, show_list);
      break;

    case var_string:
      commands = add_setshow_string_cmd (cmd_name.get (), cmdclass,
					 self->value.stringval, set_doc,
					 show_doc, help_doc, get_set_value,
					 get_show_value, set_list, show_list);
      break;

    case var_string_noescape:
      commands = add_setshow_string_noescape_cmd (cmd_name.get (), cmdclass,
						  self->value.stringval,
						  set_doc, show_doc, help_doc,
						  get_set_value, get_show_value,
						  set_list, show_list);
      break;

    case var_optional_filename:
      commands = add_setshow_optional_filename_cmd (cmd_name.get (), cmdclass,
						    self->value.stringval,
						    set_doc, show_doc, help_doc,
						    get_set_value,
						    get_show_value, set_list,
						    show_list);
      break;

    case var_filename:
      commands = add_setshow_filename_cmd (cmd_name.get (), cmdclass,
					   self->value.stringval, set_doc,
					   show_doc, help_doc, get_set_value,
					   get_show_value, set_list, show_list);
      break;

    case var_zinteger:
      commands = add_setshow_zinteger_cmd (cmd_name.get (), cmdclass,
					   &self->value.intval, set_doc,
					   show_doc, help_doc, get_set_value,
					   get_show_value, set_list, show_list);
      break;

    case var_zuinteger:
      commands = add_setshow_zuinteger_cmd (cmd_name.get (), cmdclass,
					    &self->value.uintval, set_doc,
					    show_doc, help_doc, get_set_value,
					    get_show_value, set_list,
					    show_list);
      break;

    case var_zuinteger_unlimited:
      commands = add_setshow_zuinteger_unlimited_cmd (cmd_name.get (), cmdclass,
						      &self->value.intval,
						      set_doc, show_doc,
						      help_doc, get_set_value,
						      get_show_value, set_list,
						      show_list);
      break;

    case var_enum:
      /* Initialize the value, just in case.  */
      self->value.cstringval = self->enumeration[0];
      commands = add_setshow_enum_cmd (cmd_name.get (), cmdclass,
				       self->enumeration,
				       &self->value.cstringval, set_doc,
				       show_doc, help_doc, get_set_value,
				       get_show_value, set_list, show_list);
      break;

    default:
      gdb_assert_not_reached ("Unhandled parameter class.");
    }

  /* Register Python objects in both commands' context.  */
  commands.set->set_context (self);
  commands.show->set_context (self);

  /* We (unfortunately) currently leak the command name.  */
  cmd_name.release ();
}

/* A helper which computes enum values.  Returns 1 on success.  Returns 0 on
   error, with a python exception set.  */
static int
compute_enum_values (parmpy_object *self, PyObject *enum_values)
{
  Py_ssize_t size, i;

  if (! enum_values)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("An enumeration is required for PARAM_ENUM."));
      return 0;
    }

  if (! PySequence_Check (enum_values))
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("The enumeration is not a sequence."));
      return 0;
    }

  size = PySequence_Size (enum_values);
  if (size < 0)
    return 0;
  if (size == 0)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("The enumeration is empty."));
      return 0;
    }

  gdb_argv holder (XCNEWVEC (char *, size + 1));
  char **enumeration = holder.get ();

  for (i = 0; i < size; ++i)
    {
      gdbpy_ref<> item (PySequence_GetItem (enum_values, i));

      if (item == NULL)
	return 0;
      if (! gdbpy_is_string (item.get ()))
	{
	  PyErr_SetString (PyExc_RuntimeError,
			   _("The enumeration item not a string."));
	  return 0;
	}
      enumeration[i] = python_string_to_host_string (item.get ()).release ();
      if (enumeration[i] == NULL)
	return 0;
    }

  self->enumeration = const_cast<const char**> (holder.release ());
  return 1;
}

/* Object initializer; sets up gdb-side structures for command.

   Use: __init__(NAME, CMDCLASS, PARMCLASS, [ENUM])

   NAME is the name of the parameter.  It may consist of multiple
   words, in which case the final word is the name of the new command,
   and earlier words must be prefix commands.

   CMDCLASS is the kind of command.  It should be one of the COMMAND_*
   constants defined in the gdb module.

   PARMCLASS is the type of the parameter.  It should be one of the
   PARAM_* constants defined in the gdb module.

   If PARMCLASS is PARAM_ENUM, then the final argument should be a
   collection of strings.  These strings are the valid values for this
   parameter.

   The documentation for the parameter is taken from the doc string
   for the python class.

   Returns -1 on error, with a python exception set.  */

static int
parmpy_init (PyObject *self, PyObject *args, PyObject *kwds)
{
  parmpy_object *obj = (parmpy_object *) self;
  const char *name;
  gdb::unique_xmalloc_ptr<char> set_doc, show_doc, doc;
  int parmclass, cmdtype;
  PyObject *enum_values = NULL;
  struct cmd_list_element **set_list, **show_list;

  if (! PyArg_ParseTuple (args, "sii|O", &name, &cmdtype, &parmclass,
			  &enum_values))
    return -1;

  if (cmdtype != no_class && cmdtype != class_run
      && cmdtype != class_vars && cmdtype != class_stack
      && cmdtype != class_files && cmdtype != class_support
      && cmdtype != class_info && cmdtype != class_breakpoint
      && cmdtype != class_trace && cmdtype != class_obscure
      && cmdtype != class_maintenance)
    {
      PyErr_Format (PyExc_RuntimeError, _("Invalid command class argument."));
      return -1;
    }

  if (parmclass != var_boolean /* ARI: var_boolean */
      && parmclass != var_auto_boolean
      && parmclass != var_uinteger && parmclass != var_integer
      && parmclass != var_string && parmclass != var_string_noescape
      && parmclass != var_optional_filename && parmclass != var_filename
      && parmclass != var_zinteger && parmclass != var_zuinteger
      && parmclass != var_zuinteger_unlimited && parmclass != var_enum)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Invalid parameter class argument."));
      return -1;
    }

  if (enum_values && parmclass != var_enum)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Only PARAM_ENUM accepts a fourth argument."));
      return -1;
    }
  if (parmclass == var_enum)
    {
      if (! compute_enum_values (obj, enum_values))
	return -1;
    }
  else
    obj->enumeration = NULL;
  obj->type = (enum var_types) parmclass;
  memset (&obj->value, 0, sizeof (obj->value));

  if (var_type_uses<std::string> (obj->type))
    obj->value.stringval = new std::string;

  gdb::unique_xmalloc_ptr<char> cmd_name
    = gdbpy_parse_command_name (name, &set_list, &setlist);
  if (cmd_name == nullptr)
    return -1;

  cmd_name = gdbpy_parse_command_name (name, &show_list, &showlist);
  if (cmd_name == nullptr)
    return -1;

  set_doc = get_doc_string (self, doc_string_set, name);
  show_doc = get_doc_string (self, doc_string_show, name);
  doc = get_doc_string (self, doc_string_description, cmd_name.get ());

  Py_INCREF (self);

  try
    {
      add_setshow_generic (parmclass, (enum command_class) cmdtype,
			   std::move (cmd_name), obj,
			   set_doc.get (), show_doc.get (),
			   doc.get (), set_list, show_list);
    }
  catch (const gdb_exception &except)
    {
      Py_DECREF (self);
      gdbpy_convert_exception (except);
      return -1;
    }

  return 0;
}

/* Deallocate function for a gdb.Parameter.  */

static void
parmpy_dealloc (PyObject *obj)
{
  parmpy_object *parm_obj = (parmpy_object *) obj;

  if (var_type_uses<std::string> (parm_obj->type))
    delete parm_obj->value.stringval;
}

/* Initialize the 'parameters' module.  */
int
gdbpy_initialize_parameters (void)
{
  int i;

  parmpy_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&parmpy_object_type) < 0)
    return -1;

  set_doc_cst = PyUnicode_FromString ("set_doc");
  if (! set_doc_cst)
    return -1;
  show_doc_cst = PyUnicode_FromString ("show_doc");
  if (! show_doc_cst)
    return -1;

  for (i = 0; parm_constants[i].name; ++i)
    {
      if (PyModule_AddIntConstant (gdb_module,
				   parm_constants[i].name,
				   parm_constants[i].value) < 0)
	return -1;
    }

  return gdb_pymodule_addobject (gdb_module, "Parameter",
				 (PyObject *) &parmpy_object_type);
}



PyTypeObject parmpy_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Parameter",		  /*tp_name*/
  sizeof (parmpy_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  parmpy_dealloc,		  /*tp_dealloc*/
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
  get_attr,			  /*tp_getattro*/
  set_attr,			  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "GDB parameter object",	  /* tp_doc */
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
  parmpy_init,			  /* tp_init */
  0,				  /* tp_alloc */
};
