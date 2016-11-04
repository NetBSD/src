/* GDB parameters implemented in Guile.

   Copyright (C) 2008-2016 Free Software Foundation, Inc.

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
#include "charset.h"
#include "gdbcmd.h"
#include "cli/cli-decode.h"
#include "completer.h"
#include "language.h"
#include "arch-utils.h"
#include "guile-internal.h"

/* A union that can hold anything described by enum var_types.  */

union pascm_variable
{
  /* Hold an integer value, for boolean and integer types.  */
  int intval;

  /* Hold an auto_boolean.  */
  enum auto_boolean autoboolval;

  /* Hold an unsigned integer value, for uinteger.  */
  unsigned int uintval;

  /* Hold a string, for the various string types.  */
  char *stringval;

  /* Hold a string, for enums.  */
  const char *cstringval;
};

/* A GDB parameter.

   Note: Parameters are added to gdb using a two step process:
   1) Call make-parameter to create a <gdb:parameter> object.
   2) Call register-parameter! to add the parameter to gdb.
   It is done this way so that the constructor, make-parameter, doesn't have
   any side-effects.  This means that the smob needs to store everything
   that was passed to make-parameter.

   N.B. There is no free function for this smob.
   All objects pointed to by this smob must live in GC space.  */

typedef struct _param_smob
{
  /* This always appears first.  */
  gdb_smob base;

  /* The parameter name.  */
  char *name;

  /* The last word of the command.
     This is needed because add_cmd requires us to allocate space
     for it. :-(  */
  char *cmd_name;

  /* One of the COMMAND_* constants.  */
  enum command_class cmd_class;

  /* The type of the parameter.  */
  enum var_types type;

  /* The docs for the parameter.  */
  char *set_doc;
  char *show_doc;
  char *doc;

  /* The corresponding gdb command objects.
     These are NULL if the parameter has not been registered yet, or
     is no longer registered.  */
  struct cmd_list_element *set_command;
  struct cmd_list_element *show_command;

  /* The value of the parameter.  */
  union pascm_variable value;

  /* For an enum parameter, the possible values.  The vector lives in GC
     space, it will be freed with the smob.  */
  const char * const *enumeration;

  /* The set_func funcion or #f if not specified.
     This function is called *after* the parameter is set.
     It returns a string that will be displayed to the user.  */
  SCM set_func;

  /* The show_func function or #f if not specified.
     This function returns the string that is printed.  */
  SCM show_func;

  /* The <gdb:parameter> object we are contained in, needed to
     protect/unprotect the object since a reference to it comes from
     non-gc-managed space (the command context pointer).  */
  SCM containing_scm;
} param_smob;

static const char param_smob_name[] = "gdb:parameter";

/* The tag Guile knows the param smob by.  */
static scm_t_bits parameter_smob_tag;

/* Keywords used by make-parameter!.  */
static SCM command_class_keyword;
static SCM parameter_type_keyword;
static SCM enum_list_keyword;
static SCM set_func_keyword;
static SCM show_func_keyword;
static SCM doc_keyword;
static SCM set_doc_keyword;
static SCM show_doc_keyword;
static SCM initial_value_keyword;
static SCM auto_keyword;
static SCM unlimited_keyword;

static int pascm_is_valid (param_smob *);
static const char *pascm_param_type_name (enum var_types type);
static SCM pascm_param_value (enum var_types type, void *var,
			      int arg_pos, const char *func_name);

/* Administrivia for parameter smobs.  */

static int
pascm_print_param_smob (SCM self, SCM port, scm_print_state *pstate)
{
  param_smob *p_smob = (param_smob *) SCM_SMOB_DATA (self);
  SCM value;

  gdbscm_printf (port, "#<%s", param_smob_name);

  gdbscm_printf (port, " %s", p_smob->name);

  if (! pascm_is_valid (p_smob))
    scm_puts (" {invalid}", port);

  gdbscm_printf (port, " %s ", pascm_param_type_name (p_smob->type));

  value = pascm_param_value (p_smob->type, &p_smob->value,
			     GDBSCM_ARG_NONE, NULL);
  scm_display (value, port);

  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Create an empty (uninitialized) parameter.  */

static SCM
pascm_make_param_smob (void)
{
  param_smob *p_smob = (param_smob *)
    scm_gc_malloc (sizeof (param_smob), param_smob_name);
  SCM p_scm;

  memset (p_smob, 0, sizeof (*p_smob));
  p_smob->cmd_class = no_class;
  p_smob->type = var_boolean; /* ARI: var_boolean */
  p_smob->set_func = SCM_BOOL_F;
  p_smob->show_func = SCM_BOOL_F;
  p_scm = scm_new_smob (parameter_smob_tag, (scm_t_bits) p_smob);
  p_smob->containing_scm = p_scm;
  gdbscm_init_gsmob (&p_smob->base);

  return p_scm;
}

/* Returns non-zero if SCM is a <gdb:parameter> object.  */

static int
pascm_is_parameter (SCM scm)
{
  return SCM_SMOB_PREDICATE (parameter_smob_tag, scm);
}

/* (gdb:parameter? scm) -> boolean */

static SCM
gdbscm_parameter_p (SCM scm)
{
  return scm_from_bool (pascm_is_parameter (scm));
}

/* Returns the <gdb:parameter> object in SELF.
   Throws an exception if SELF is not a <gdb:parameter> object.  */

static SCM
pascm_get_param_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (pascm_is_parameter (self), self, arg_pos, func_name,
		   param_smob_name);

  return self;
}

/* Returns a pointer to the parameter smob of SELF.
   Throws an exception if SELF is not a <gdb:parameter> object.  */

static param_smob *
pascm_get_param_smob_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM p_scm = pascm_get_param_arg_unsafe (self, arg_pos, func_name);
  param_smob *p_smob = (param_smob *) SCM_SMOB_DATA (p_scm);

  return p_smob;
}

/* Return non-zero if parameter P_SMOB is valid.  */

static int
pascm_is_valid (param_smob *p_smob)
{
  return p_smob->set_command != NULL;
}

/* A helper function which return the default documentation string for
   a parameter (which is to say that it's undocumented).  */

static char *
get_doc_string (void)
{
  return xstrdup (_("This command is not documented."));
}

/* Subroutine of pascm_set_func, pascm_show_func to simplify them.
   Signal the error returned from calling set_func/show_func.  */

static void
pascm_signal_setshow_error (SCM exception, const char *msg)
{
  /* Don't print the stack if this was an error signalled by the command
     itself.  */
  if (gdbscm_user_error_p (gdbscm_exception_key (exception)))
    {
      char *excp_text = gdbscm_exception_message_to_string (exception);

      make_cleanup (xfree, excp_text);
      error ("%s", excp_text);
    }
  else
    {
      gdbscm_print_gdb_exception (SCM_BOOL_F, exception);
      error ("%s", msg);
    }
}

/* A callback function that is registered against the respective
   add_setshow_* set_func prototype.  This function will call
   the Scheme function "set_func" which must exist.
   Note: ARGS is always passed as NULL.  */

static void
pascm_set_func (char *args, int from_tty, struct cmd_list_element *c)
{
  param_smob *p_smob = (param_smob *) get_cmd_context (c);
  SCM self, result, exception;
  char *msg;
  struct cleanup *cleanups;

  gdb_assert (gdbscm_is_procedure (p_smob->set_func));

  self = p_smob->containing_scm;

  result = gdbscm_safe_call_1 (p_smob->set_func, self, gdbscm_user_error_p);

  if (gdbscm_is_exception (result))
    {
      pascm_signal_setshow_error (result,
				  _("Error occurred setting parameter."));
    }

  if (!scm_is_string (result))
    error (_("Result of %s set-func is not a string."), p_smob->name);

  msg = gdbscm_scm_to_host_string (result, NULL, &exception);
  if (msg == NULL)
    {
      gdbscm_print_gdb_exception (SCM_BOOL_F, exception);
      error (_("Error converting show text to host string."));
    }

  cleanups = make_cleanup (xfree, msg);
  /* GDB is usually silent when a parameter is set.  */
  if (*msg != '\0')
    fprintf_filtered (gdb_stdout, "%s\n", msg);
  do_cleanups (cleanups);
}

/* A callback function that is registered against the respective
   add_setshow_* show_func prototype.  This function will call
   the Scheme function "show_func" which must exist and must return a
   string that is then printed to FILE.  */

static void
pascm_show_func (struct ui_file *file, int from_tty,
		 struct cmd_list_element *c, const char *value)
{
  param_smob *p_smob = (param_smob *) get_cmd_context (c);
  SCM value_scm, self, result, exception;
  char *msg;
  struct cleanup *cleanups;

  gdb_assert (gdbscm_is_procedure (p_smob->show_func));

  value_scm = gdbscm_scm_from_host_string (value, strlen (value));
  if (gdbscm_is_exception (value_scm))
    {
      error (_("Error converting parameter value \"%s\" to Scheme string."),
	     value);
    }
  self = p_smob->containing_scm;

  result = gdbscm_safe_call_2 (p_smob->show_func, self, value_scm,
			       gdbscm_user_error_p);

  if (gdbscm_is_exception (result))
    {
      pascm_signal_setshow_error (result,
				  _("Error occurred showing parameter."));
    }

  msg = gdbscm_scm_to_host_string (result, NULL, &exception);
  if (msg == NULL)
    {
      gdbscm_print_gdb_exception (SCM_BOOL_F, exception);
      error (_("Error converting show text to host string."));
    }

  cleanups = make_cleanup (xfree, msg);
  fprintf_filtered (file, "%s\n", msg);
  do_cleanups (cleanups);
}

/* A helper function that dispatches to the appropriate add_setshow
   function.  */

static void
add_setshow_generic (enum var_types param_type, enum command_class cmd_class,
		     char *cmd_name, param_smob *self,
		     char *set_doc, char *show_doc, char *help_doc,
		     cmd_sfunc_ftype *set_func,
		     show_value_ftype *show_func,
		     struct cmd_list_element **set_list,
		     struct cmd_list_element **show_list,
		     struct cmd_list_element **set_cmd,
		     struct cmd_list_element **show_cmd)
{
  struct cmd_list_element *param = NULL;
  const char *tmp_name = NULL;

  switch (param_type)
    {
    case var_boolean:
      add_setshow_boolean_cmd (cmd_name, cmd_class,
			       &self->value.intval,
			       set_doc, show_doc, help_doc,
			       set_func, show_func,
			       set_list, show_list);

      break;

    case var_auto_boolean:
      add_setshow_auto_boolean_cmd (cmd_name, cmd_class,
				    &self->value.autoboolval,
				    set_doc, show_doc, help_doc,
				    set_func, show_func,
				    set_list, show_list);
      break;

    case var_uinteger:
      add_setshow_uinteger_cmd (cmd_name, cmd_class,
				&self->value.uintval,
				set_doc, show_doc, help_doc,
				set_func, show_func,
				set_list, show_list);
      break;

    case var_zinteger:
      add_setshow_zinteger_cmd (cmd_name, cmd_class,
				&self->value.intval,
				set_doc, show_doc, help_doc,
				set_func, show_func,
				set_list, show_list);
      break;

    case var_zuinteger:
      add_setshow_zuinteger_cmd (cmd_name, cmd_class,
				 &self->value.uintval,
				 set_doc, show_doc, help_doc,
				 set_func, show_func,
				 set_list, show_list);
      break;

    case var_zuinteger_unlimited:
      add_setshow_zuinteger_unlimited_cmd (cmd_name, cmd_class,
					   &self->value.intval,
					   set_doc, show_doc, help_doc,
					   set_func, show_func,
					   set_list, show_list);
      break;

    case var_string:
      add_setshow_string_cmd (cmd_name, cmd_class,
			      &self->value.stringval,
			      set_doc, show_doc, help_doc,
			      set_func, show_func,
			      set_list, show_list);
      break;

    case var_string_noescape:
      add_setshow_string_noescape_cmd (cmd_name, cmd_class,
				       &self->value.stringval,
				       set_doc, show_doc, help_doc,
				       set_func, show_func,
				       set_list, show_list);

      break;

    case var_optional_filename:
      add_setshow_optional_filename_cmd (cmd_name, cmd_class,
					 &self->value.stringval,
					 set_doc, show_doc, help_doc,
					 set_func, show_func,
					 set_list, show_list);
      break;

    case var_filename:
      add_setshow_filename_cmd (cmd_name, cmd_class,
				&self->value.stringval,
				set_doc, show_doc, help_doc,
				set_func, show_func,
				set_list, show_list);
      break;

    case var_enum:
      add_setshow_enum_cmd (cmd_name, cmd_class,
			    self->enumeration,
			    &self->value.cstringval,
			    set_doc, show_doc, help_doc,
			    set_func, show_func,
			    set_list, show_list);
      /* Initialize the value, just in case.  */
      self->value.cstringval = self->enumeration[0];
      break;

    default:
      gdb_assert_not_reached ("bad param_type value");
    }

  /* Lookup created parameter, and register Scheme object against the
     parameter context.  Perform this task against both lists.  */
  tmp_name = cmd_name;
  param = lookup_cmd (&tmp_name, *show_list, "", 0, 1);
  gdb_assert (param != NULL);
  set_cmd_context (param, self);
  *set_cmd = param;

  tmp_name = cmd_name;
  param = lookup_cmd (&tmp_name, *set_list, "", 0, 1);
  gdb_assert (param != NULL);
  set_cmd_context (param, self);
  *show_cmd = param;
}

/* Return an array of strings corresponding to the enum values for
   ENUM_VALUES_SCM.
   Throws an exception if there's a problem with the values.
   Space for the result is allocated from the GC heap.  */

static const char * const *
compute_enum_list (SCM enum_values_scm, int arg_pos, const char *func_name)
{
  long i, size;
  char **enum_values;
  const char * const *result;

  SCM_ASSERT_TYPE (gdbscm_is_true (scm_list_p (enum_values_scm)),
		   enum_values_scm, arg_pos, func_name, _("list"));

  size = scm_ilength (enum_values_scm);
  if (size == 0)
    {
      gdbscm_out_of_range_error (FUNC_NAME, arg_pos, enum_values_scm,
				 _("enumeration list is empty"));
    }

  enum_values = XCNEWVEC (char *, size + 1);

  i = 0;
  while (!scm_is_eq (enum_values_scm, SCM_EOL))
    {
      SCM value = scm_car (enum_values_scm);
      SCM exception;

      if (!scm_is_string (value))
	{
	  freeargv (enum_values);
	  SCM_ASSERT_TYPE (0, value, arg_pos, func_name, _("string"));
	}
      enum_values[i] = gdbscm_scm_to_host_string (value, NULL, &exception);
      if (enum_values[i] == NULL)
	{
	  freeargv (enum_values);
	  gdbscm_throw (exception);
	}
      ++i;
      enum_values_scm = scm_cdr (enum_values_scm);
    }
  gdb_assert (i == size);

  result = gdbscm_gc_dup_argv (enum_values);
  freeargv (enum_values);
  return result;
}

static const scheme_integer_constant parameter_types[] =
{
  /* Note: var_integer is deprecated, and intentionally does not
     appear here.  */
  { "PARAM_BOOLEAN", var_boolean }, /* ARI: var_boolean */
  { "PARAM_AUTO_BOOLEAN", var_auto_boolean },
  { "PARAM_ZINTEGER", var_zinteger },
  { "PARAM_UINTEGER", var_uinteger },
  { "PARAM_ZUINTEGER", var_zuinteger },
  { "PARAM_ZUINTEGER_UNLIMITED", var_zuinteger_unlimited },
  { "PARAM_STRING", var_string },
  { "PARAM_STRING_NOESCAPE", var_string_noescape },
  { "PARAM_OPTIONAL_FILENAME", var_optional_filename },
  { "PARAM_FILENAME", var_filename },
  { "PARAM_ENUM", var_enum },

  END_INTEGER_CONSTANTS
};

/* Return non-zero if PARAM_TYPE is a valid parameter type.  */

static int
pascm_valid_parameter_type_p (int param_type)
{
  int i;

  for (i = 0; parameter_types[i].name != NULL; ++i)
    {
      if (parameter_types[i].value == param_type)
	return 1;
    }

  return 0;
}

/* Return PARAM_TYPE as a string.  */

static const char *
pascm_param_type_name (enum var_types param_type)
{
  int i;

  for (i = 0; parameter_types[i].name != NULL; ++i)
    {
      if (parameter_types[i].value == param_type)
	return parameter_types[i].name;
    }

  gdb_assert_not_reached ("bad parameter type");
}

/* Return the value of a gdb parameter as a Scheme value.
   If TYPE is not supported, then a <gdb:exception> object is returned.  */

static SCM
pascm_param_value (enum var_types type, void *var,
		   int arg_pos, const char *func_name)
{
  /* Note: We *could* support var_integer here in case someone is trying to get
     the value of a Python-created parameter (which is the only place that
     still supports var_integer).  To further discourage its use we do not.  */

  switch (type)
    {
    case var_string:
    case var_string_noescape:
    case var_optional_filename:
    case var_filename:
    case var_enum:
      {
	char *str = * (char **) var;

	if (str == NULL)
	  str = "";
	return gdbscm_scm_from_host_string (str, strlen (str));
      }

    case var_boolean:
      {
	if (* (int *) var)
	  return SCM_BOOL_T;
	else
	  return SCM_BOOL_F;
      }

    case var_auto_boolean:
      {
	enum auto_boolean ab = * (enum auto_boolean *) var;

	if (ab == AUTO_BOOLEAN_TRUE)
	  return SCM_BOOL_T;
	else if (ab == AUTO_BOOLEAN_FALSE)
	  return SCM_BOOL_F;
	else
	  return auto_keyword;
      }

    case var_zuinteger_unlimited:
      if (* (int *) var == -1)
	return unlimited_keyword;
      gdb_assert (* (int *) var >= 0);
      /* Fall through.  */
    case var_zinteger:
      return scm_from_int (* (int *) var);

    case var_uinteger:
      if (* (unsigned int *) var == UINT_MAX)
	return unlimited_keyword;
      /* Fall through.  */
    case var_zuinteger:
      return scm_from_uint (* (unsigned int *) var);

    default:
      break;
    }

  return gdbscm_make_out_of_range_error (func_name, arg_pos,
					 scm_from_int (type),
					 _("program error: unhandled type"));
}

/* Set the value of a parameter of type TYPE in VAR from VALUE.
   ENUMERATION is the list of enum values for enum parameters, otherwise NULL.
   Throws a Scheme exception if VALUE_SCM is invalid for TYPE.  */

static void
pascm_set_param_value_x (enum var_types type, union pascm_variable *var,
			 const char * const *enumeration,
			 SCM value, int arg_pos, const char *func_name)
{
  switch (type)
    {
    case var_string:
    case var_string_noescape:
    case var_optional_filename:
    case var_filename:
      SCM_ASSERT_TYPE (scm_is_string (value)
		       || (type != var_filename
			   && gdbscm_is_false (value)),
		       value, arg_pos, func_name,
		       _("string or #f for non-PARAM_FILENAME parameters"));
      if (gdbscm_is_false (value))
	{
	  xfree (var->stringval);
	  if (type == var_optional_filename)
	    var->stringval = xstrdup ("");
	  else
	    var->stringval = NULL;
	}
      else
	{
	  char *string;
	  SCM exception;

	  string = gdbscm_scm_to_host_string (value, NULL, &exception);
	  if (string == NULL)
	    gdbscm_throw (exception);
	  xfree (var->stringval);
	  var->stringval = string;
	}
      break;

    case var_enum:
      {
	int i;
	char *str;
	SCM exception;

	SCM_ASSERT_TYPE (scm_is_string (value), value, arg_pos, func_name,
		       _("string"));
	str = gdbscm_scm_to_host_string (value, NULL, &exception);
	if (str == NULL)
	  gdbscm_throw (exception);
	for (i = 0; enumeration[i]; ++i)
	  {
	    if (strcmp (enumeration[i], str) == 0)
	      break;
	  }
	xfree (str);
	if (enumeration[i] == NULL)
	  {
	    gdbscm_out_of_range_error (func_name, arg_pos, value,
				       _("not member of enumeration"));
	  }
	var->cstringval = enumeration[i];
	break;
      }

    case var_boolean:
      SCM_ASSERT_TYPE (gdbscm_is_bool (value), value, arg_pos, func_name,
		       _("boolean"));
      var->intval = gdbscm_is_true (value);
      break;

    case var_auto_boolean:
      SCM_ASSERT_TYPE (gdbscm_is_bool (value)
		       || scm_is_eq (value, auto_keyword),
		       value, arg_pos, func_name,
		       _("boolean or #:auto"));
      if (scm_is_eq (value, auto_keyword))
	var->autoboolval = AUTO_BOOLEAN_AUTO;
      else if (gdbscm_is_true (value))
	var->autoboolval = AUTO_BOOLEAN_TRUE;
      else
	var->autoboolval = AUTO_BOOLEAN_FALSE;
      break;

    case var_zinteger:
    case var_uinteger:
    case var_zuinteger:
    case var_zuinteger_unlimited:
      if (type == var_uinteger
	  || type == var_zuinteger_unlimited)
	{
	  SCM_ASSERT_TYPE (gdbscm_is_bool (value)
			   || scm_is_eq (value, unlimited_keyword),
			   value, arg_pos, func_name,
			   _("integer or #:unlimited"));
	  if (scm_is_eq (value, unlimited_keyword))
	    {
	      if (type == var_uinteger)
		var->intval = UINT_MAX;
	      else
		var->intval = -1;
	      break;
	    }
	}
      else
	{
	  SCM_ASSERT_TYPE (scm_is_integer (value), value, arg_pos, func_name,
			   _("integer"));
	}

      if (type == var_uinteger
	  || type == var_zuinteger)
	{
	  unsigned int u = scm_to_uint (value);

	  if (type == var_uinteger && u == 0)
	    u = UINT_MAX;
	  var->uintval = u;
	}
      else
	{
	  int i = scm_to_int (value);

	  if (type == var_zuinteger_unlimited && i < -1)
	    {
	      gdbscm_out_of_range_error (func_name, arg_pos, value,
					 _("must be >= -1"));
	    }
	  var->intval = i;
	}
      break;

    default:
      gdb_assert_not_reached ("bad parameter type");
    }
}

/* Parameter Scheme functions.  */

/* (make-parameter name
     [#:command-class cmd-class] [#:parameter-type param-type]
     [#:enum-list enum-list] [#:set-func function] [#:show-func function]
     [#:doc <string>] [#:set-doc <string>] [#:show-doc <string>]
     [#:initial-value initial-value]) -> <gdb:parameter>

   NAME is the name of the parameter.  It may consist of multiple
   words, in which case the final word is the name of the new parameter,
   and earlier words must be prefix commands.

   CMD-CLASS is the kind of command.  It should be one of the COMMAND_*
   constants defined in the gdb module.

   PARAM_TYPE is the type of the parameter.  It should be one of the
   PARAM_* constants defined in the gdb module.

   If PARAM-TYPE is PARAM_ENUM, then ENUM-LIST is a list of strings that
   are the valid values for this parameter.  The first value is the default.

   SET-FUNC, if provided, is called after the parameter is set.
   It is a function of one parameter: the <gdb:parameter> object.
   It must return a string to be displayed to the user.
   Setting a parameter is typically a silent operation, so typically ""
   should be returned.

   SHOW-FUNC, if provided, returns the string that is printed.
   It is a function of two parameters: the <gdb:parameter> object
   and the current value of the parameter as a string.

   DOC, SET-DOC, SHOW-DOC are the doc strings for the parameter.

   INITIAL-VALUE is the initial value of the parameter.

   The result is the <gdb:parameter> Scheme object.
   The parameter is not available to be used yet, however.
   It must still be added to gdb with register-parameter!.  */

static SCM
gdbscm_make_parameter (SCM name_scm, SCM rest)
{
  const SCM keywords[] = {
    command_class_keyword, parameter_type_keyword, enum_list_keyword,
    set_func_keyword, show_func_keyword,
    doc_keyword, set_doc_keyword, show_doc_keyword,
    initial_value_keyword, SCM_BOOL_F
  };
  int cmd_class_arg_pos = -1, param_type_arg_pos = -1;
  int enum_list_arg_pos = -1, set_func_arg_pos = -1, show_func_arg_pos = -1;
  int doc_arg_pos = -1, set_doc_arg_pos = -1, show_doc_arg_pos = -1;
  int initial_value_arg_pos = -1;
  char *s;
  char *name;
  int cmd_class = no_class;
  int param_type = var_boolean; /* ARI: var_boolean */
  SCM enum_list_scm = SCM_BOOL_F;
  SCM set_func = SCM_BOOL_F, show_func = SCM_BOOL_F;
  char *doc = NULL, *set_doc = NULL, *show_doc = NULL;
  SCM initial_value_scm = SCM_BOOL_F;
  const char * const *enum_list = NULL;
  SCM p_scm;
  param_smob *p_smob;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, keywords, "s#iiOOOsssO",
			      name_scm, &name, rest,
			      &cmd_class_arg_pos, &cmd_class,
			      &param_type_arg_pos, &param_type,
			      &enum_list_arg_pos, &enum_list_scm,
			      &set_func_arg_pos, &set_func,
			      &show_func_arg_pos, &show_func,
			      &doc_arg_pos, &doc,
			      &set_doc_arg_pos, &set_doc,
			      &show_doc_arg_pos, &show_doc,
			      &initial_value_arg_pos, &initial_value_scm);

  /* If doc is NULL, leave it NULL.  See add_setshow_cmd_full.  */
  if (set_doc == NULL)
    set_doc = get_doc_string ();
  if (show_doc == NULL)
    show_doc = get_doc_string ();

  s = name;
  name = gdbscm_canonicalize_command_name (s, 0);
  xfree (s);
  if (doc != NULL)
    {
      s = doc;
      doc = gdbscm_gc_xstrdup (s);
      xfree (s);
    }
  s = set_doc;
  set_doc = gdbscm_gc_xstrdup (s);
  xfree (s);
  s = show_doc;
  show_doc = gdbscm_gc_xstrdup (s);
  xfree (s);

  if (!gdbscm_valid_command_class_p (cmd_class))
    {
      gdbscm_out_of_range_error (FUNC_NAME, cmd_class_arg_pos,
				 scm_from_int (cmd_class),
				 _("invalid command class argument"));
    }
  if (!pascm_valid_parameter_type_p (param_type))
    {
      gdbscm_out_of_range_error (FUNC_NAME, param_type_arg_pos,
				 scm_from_int (param_type),
				 _("invalid parameter type argument"));
    }
  if (enum_list_arg_pos > 0 && param_type != var_enum)
    {
      gdbscm_misc_error (FUNC_NAME, enum_list_arg_pos, enum_list_scm,
		_("#:enum-values can only be provided with PARAM_ENUM"));
    }
  if (enum_list_arg_pos < 0 && param_type == var_enum)
    {
      gdbscm_misc_error (FUNC_NAME, GDBSCM_ARG_NONE, SCM_BOOL_F,
			 _("PARAM_ENUM requires an enum-values argument"));
    }
  if (set_func_arg_pos > 0)
    {
      SCM_ASSERT_TYPE (gdbscm_is_procedure (set_func), set_func,
		       set_func_arg_pos, FUNC_NAME, _("procedure"));
    }
  if (show_func_arg_pos > 0)
    {
      SCM_ASSERT_TYPE (gdbscm_is_procedure (show_func), show_func,
		       show_func_arg_pos, FUNC_NAME, _("procedure"));
    }
  if (param_type == var_enum)
    {
      /* Note: enum_list lives in GC space, so we don't have to worry about
	 freeing it if we later throw an exception.  */
      enum_list = compute_enum_list (enum_list_scm, enum_list_arg_pos,
				     FUNC_NAME);
    }

  /* If initial-value is a function, we need the parameter object constructed
     to pass it to the function.  A typical thing the function may want to do
     is add an object-property to it to record the last known good value.  */
  p_scm = pascm_make_param_smob ();
  p_smob = (param_smob *) SCM_SMOB_DATA (p_scm);
  /* These are all stored in GC space so that we don't have to worry about
     freeing them if we throw an exception.  */
  p_smob->name = name;
  p_smob->cmd_class = (enum command_class) cmd_class;
  p_smob->type = (enum var_types) param_type;
  p_smob->doc = doc;
  p_smob->set_doc = set_doc;
  p_smob->show_doc = show_doc;
  p_smob->enumeration = enum_list;
  p_smob->set_func = set_func;
  p_smob->show_func = show_func;

  if (initial_value_arg_pos > 0)
    {
      if (gdbscm_is_procedure (initial_value_scm))
	{
	  initial_value_scm = gdbscm_safe_call_1 (initial_value_scm,
						  p_smob->containing_scm, NULL);
	  if (gdbscm_is_exception (initial_value_scm))
	    gdbscm_throw (initial_value_scm);
	}
      pascm_set_param_value_x (p_smob->type, &p_smob->value, enum_list,
			       initial_value_scm,
			       initial_value_arg_pos, FUNC_NAME);
    }

  return p_scm;
}

/* Subroutine of gdbscm_register_parameter_x to simplify it.
   Return non-zero if parameter NAME is already defined in LIST.  */

static int
pascm_parameter_defined_p (const char *name, struct cmd_list_element *list)
{
  struct cmd_list_element *c;

  c = lookup_cmd_1 (&name, list, NULL, 1);

  /* If the name is ambiguous that's ok, it's a new parameter still.  */
  return c != NULL && c != CMD_LIST_AMBIGUOUS;
}

/* (register-parameter! <gdb:parameter>) -> unspecified

   It is an error to register a pre-existing parameter.  */

static SCM
gdbscm_register_parameter_x (SCM self)
{
  param_smob *p_smob
    = pascm_get_param_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  char *cmd_name;
  struct cmd_list_element **set_list, **show_list;

  if (pascm_is_valid (p_smob))
    scm_misc_error (FUNC_NAME, _("parameter is already registered"), SCM_EOL);

  cmd_name = gdbscm_parse_command_name (p_smob->name, FUNC_NAME, SCM_ARG1,
					&set_list, &setlist);
  xfree (cmd_name);
  cmd_name = gdbscm_parse_command_name (p_smob->name, FUNC_NAME, SCM_ARG1,
					&show_list, &showlist);
  p_smob->cmd_name = gdbscm_gc_xstrdup (cmd_name);
  xfree (cmd_name);

  if (pascm_parameter_defined_p (p_smob->cmd_name, *set_list))
    {
      gdbscm_misc_error (FUNC_NAME, SCM_ARG1, self,
		_("parameter exists, \"set\" command is already defined"));
    }
  if (pascm_parameter_defined_p (p_smob->cmd_name, *show_list))
    {
      gdbscm_misc_error (FUNC_NAME, SCM_ARG1, self,
		_("parameter exists, \"show\" command is already defined"));
    }

  TRY
    {
      add_setshow_generic (p_smob->type, p_smob->cmd_class,
			   p_smob->cmd_name, p_smob,
			   p_smob->set_doc, p_smob->show_doc, p_smob->doc,
			   (gdbscm_is_procedure (p_smob->set_func)
			    ? pascm_set_func : NULL),
			   (gdbscm_is_procedure (p_smob->show_func)
			    ? pascm_show_func : NULL),
			   set_list, show_list,
			   &p_smob->set_command, &p_smob->show_command);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      GDBSCM_HANDLE_GDB_EXCEPTION (except);
    }
  END_CATCH

  /* Note: At this point the parameter exists in gdb.
     So no more errors after this point.  */

  /* The owner of this parameter is not in GC-controlled memory, so we need
     to protect it from GC until the parameter is deleted.  */
  scm_gc_protect_object (p_smob->containing_scm);

  return SCM_UNSPECIFIED;
}

/* (parameter-value <gdb:parameter>) -> value
   (parameter-value <string>) -> value */

static SCM
gdbscm_parameter_value (SCM self)
{
  SCM_ASSERT_TYPE (pascm_is_parameter (self) || scm_is_string (self),
		   self, SCM_ARG1, FUNC_NAME, _("<gdb:parameter> or string"));

  if (pascm_is_parameter (self))
    {
      param_smob *p_smob = pascm_get_param_smob_arg_unsafe (self, SCM_ARG1,
							    FUNC_NAME);

      return pascm_param_value (p_smob->type, &p_smob->value,
				SCM_ARG1, FUNC_NAME);
    }
  else
    {
      char *name;
      SCM except_scm;
      struct cmd_list_element *alias, *prefix, *cmd;
      const char *arg;
      char *newarg;
      int found = -1;
      struct gdb_exception except = exception_none;

      name = gdbscm_scm_to_host_string (self, NULL, &except_scm);
      if (name == NULL)
	gdbscm_throw (except_scm);
      newarg = concat ("show ", name, (char *) NULL);
      TRY
	{
	  found = lookup_cmd_composition (newarg, &alias, &prefix, &cmd);
	}
      CATCH (ex, RETURN_MASK_ALL)
	{
	  except = ex;
	}
      END_CATCH

      xfree (name);
      xfree (newarg);
      GDBSCM_HANDLE_GDB_EXCEPTION (except);
      if (!found)
	{
	  gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, self,
				     _("parameter not found"));
	}
      if (cmd->var == NULL)
	{
	  gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, self,
				     _("not a parameter"));
	}

      return pascm_param_value (cmd->var_type, cmd->var, SCM_ARG1, FUNC_NAME);
    }
}

/* (set-parameter-value! <gdb:parameter> value) -> unspecified */

static SCM
gdbscm_set_parameter_value_x (SCM self, SCM value)
{
  param_smob *p_smob = pascm_get_param_smob_arg_unsafe (self, SCM_ARG1,
							FUNC_NAME);

  pascm_set_param_value_x (p_smob->type, &p_smob->value, p_smob->enumeration,
			   value, SCM_ARG2, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

/* Initialize the Scheme parameter support.  */

static const scheme_function parameter_functions[] =
{
  { "make-parameter", 1, 0, 1, as_a_scm_t_subr (gdbscm_make_parameter),
    "\
Make a GDB parameter object.\n\
\n\
  Arguments: name\n\
      [#:command-class <cmd-class>] [#:parameter-type <parameter-type>]\n\
      [#:enum-list <enum-list>]\n\
      [#:set-func function] [#:show-func function]\n\
      [#:doc string] [#:set-doc string] [#:show-doc string]\n\
      [#:initial-value initial-value]\n\
    name: The name of the command.  It may consist of multiple words,\n\
      in which case the final word is the name of the new parameter, and\n\
      earlier words must be prefix commands.\n\
    cmd-class: The class of the command, one of COMMAND_*.\n\
      The default is COMMAND_NONE.\n\
    parameter-type: The kind of parameter, one of PARAM_*\n\
      The default is PARAM_BOOLEAN.\n\
    enum-list: If parameter-type is PARAM_ENUM, then this specifies the set\n\
      of values of the enum.\n\
    set-func: A function of one parameter: the <gdb:parameter> object.\n\
      Called *after* the parameter has been set.  Returns either \"\" or a\n\
      non-empty string to be displayed to the user.\n\
      If non-empty, GDB will add a trailing newline.\n\
    show-func: A function of two parameters: the <gdb:parameter> object\n\
      and the string representation of the current value.\n\
      The result is a string to be displayed to the user.\n\
      GDB will add a trailing newline.\n\
    doc: The \"doc string\" of the parameter.\n\
    set-doc: The \"doc string\" when setting the parameter.\n\
    show-doc: The \"doc string\" when showing the parameter.\n\
    initial-value: The initial value of the parameter." },

  { "register-parameter!", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_register_parameter_x),
    "\
Register a <gdb:parameter> object with GDB." },

  { "parameter?", 1, 0, 0, as_a_scm_t_subr (gdbscm_parameter_p),
    "\
Return #t if the object is a <gdb:parameter> object." },

  { "parameter-value", 1, 0, 0, as_a_scm_t_subr (gdbscm_parameter_value),
    "\
Return the value of a <gdb:parameter> object\n\
or any gdb parameter if param is a string naming the parameter." },

  { "set-parameter-value!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_parameter_value_x),
    "\
Set the value of a <gdb:parameter> object.\n\
\n\
  Arguments: <gdb:parameter> value" },

  END_FUNCTIONS
};

void
gdbscm_initialize_parameters (void)
{
  parameter_smob_tag
    = gdbscm_make_smob_type (param_smob_name, sizeof (param_smob));
  scm_set_smob_print (parameter_smob_tag, pascm_print_param_smob);

  gdbscm_define_integer_constants (parameter_types, 1);
  gdbscm_define_functions (parameter_functions, 1);

  command_class_keyword = scm_from_latin1_keyword ("command-class");
  parameter_type_keyword = scm_from_latin1_keyword ("parameter-type");
  enum_list_keyword = scm_from_latin1_keyword ("enum-list");
  set_func_keyword = scm_from_latin1_keyword ("set-func");
  show_func_keyword = scm_from_latin1_keyword ("show-func");
  doc_keyword = scm_from_latin1_keyword ("doc");
  set_doc_keyword = scm_from_latin1_keyword ("set-doc");
  show_doc_keyword = scm_from_latin1_keyword ("show-doc");
  initial_value_keyword = scm_from_latin1_keyword ("initial-value");
  auto_keyword = scm_from_latin1_keyword ("auto");
  unlimited_keyword = scm_from_latin1_keyword ("unlimited");
}
