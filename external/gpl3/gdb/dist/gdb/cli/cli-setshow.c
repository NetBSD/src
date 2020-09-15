/* Handle set and show GDB commands.

   Copyright (C) 2000-2020 Free Software Foundation, Inc.

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
#include "readline/tilde.h"
#include "value.h"
#include <ctype.h>
#include "arch-utils.h"
#include "observable.h"

#include "ui-out.h"

#include "cli/cli-decode.h"
#include "cli/cli-cmds.h"
#include "cli/cli-setshow.h"
#include "cli/cli-utils.h"

/* Return true if the change of command parameter should be notified.  */

static int
notify_command_param_changed_p (int param_changed, struct cmd_list_element *c)
{
  if (param_changed == 0)
    return 0;

  if (c->theclass == class_maintenance || c->theclass == class_deprecated
      || c->theclass == class_obscure)
    return 0;

  return 1;
}


static enum auto_boolean
parse_auto_binary_operation (const char *arg)
{
  if (arg != NULL && *arg != '\0')
    {
      int length = strlen (arg);

      while (isspace (arg[length - 1]) && length > 0)
	length--;

      /* Note that "o" is ambiguous.  */

      if ((length == 2 && strncmp (arg, "on", length) == 0)
	  || strncmp (arg, "1", length) == 0
	  || strncmp (arg, "yes", length) == 0
	  || strncmp (arg, "enable", length) == 0)
	return AUTO_BOOLEAN_TRUE;
      else if ((length >= 2 && strncmp (arg, "off", length) == 0)
	       || strncmp (arg, "0", length) == 0
	       || strncmp (arg, "no", length) == 0
	       || strncmp (arg, "disable", length) == 0)
	return AUTO_BOOLEAN_FALSE;
      else if (strncmp (arg, "auto", length) == 0
	       || (length > 1 && strncmp (arg, "-1", length) == 0))
	return AUTO_BOOLEAN_AUTO;
    }
  error (_("\"on\", \"off\" or \"auto\" expected."));
  return AUTO_BOOLEAN_AUTO; /* Pacify GCC.  */
}

/* See cli-setshow.h.  */

int
parse_cli_boolean_value (const char **arg)
{
  const char *p = skip_to_space (*arg);
  size_t length = p - *arg;

  /* Note that "o" is ambiguous.  */

  if ((length == 2 && strncmp (*arg, "on", length) == 0)
      || strncmp (*arg, "1", length) == 0
      || strncmp (*arg, "yes", length) == 0
      || strncmp (*arg, "enable", length) == 0)
    {
      *arg = skip_spaces (*arg + length);
      return 1;
    }
  else if ((length >= 2 && strncmp (*arg, "off", length) == 0)
	   || strncmp (*arg, "0", length) == 0
	   || strncmp (*arg, "no", length) == 0
	   || strncmp (*arg, "disable", length) == 0)
    {
      *arg = skip_spaces (*arg + length);
      return 0;
    }
  else
    return -1;
}

/* See cli-setshow.h.  */

int
parse_cli_boolean_value (const char *arg)
{
  if (!arg || !*arg)
    return 1;

  int b = parse_cli_boolean_value (&arg);
  if (b >= 0 && *arg != '\0')
    return -1;

  return b;
}


void
deprecated_show_value_hack (struct ui_file *ignore_file,
			    int ignore_from_tty,
			    struct cmd_list_element *c,
			    const char *value)
{
  /* If there's no command or value, don't try to print it out.  */
  if (c == NULL || value == NULL)
    return;
  /* Print doc minus "Show " at start.  Tell print_doc_line that
     this is for a 'show value' prefix.  */
  print_doc_line (gdb_stdout, c->doc + 5, true);
  switch (c->var_type)
    {
    case var_string:
    case var_string_noescape:
    case var_optional_filename:
    case var_filename:
    case var_enum:
      printf_filtered ((" is \"%s\".\n"), value);
      break;
    default:
      printf_filtered ((" is %s.\n"), value);
      break;
    }
}

/* Returns true if ARG is "unlimited".  */

static bool
is_unlimited_literal (const char **arg, bool expression)
{
  *arg = skip_spaces (*arg);

  const char *unl_start = *arg;

  const char *p = skip_to_space (*arg);

  size_t len = p - *arg;

  if (len > 0 && strncmp ("unlimited", *arg, len) == 0)
    {
      *arg += len;

      /* If parsing an expression (i.e., parsing for a "set" command),
	 anything after "unlimited" is junk.  For options, anything
	 after "unlimited" might be a command argument or another
	 option.  */
      if (expression)
	{
	  const char *after = skip_spaces (*arg);
	  if (*after != '\0')
	    error (_("Junk after \"%.*s\": %s"),
		   (int) len, unl_start, after);
	}

      return true;
    }

  return false;
}

/* See cli-setshow.h.  */

unsigned int
parse_cli_var_uinteger (var_types var_type, const char **arg,
			bool expression)
{
  LONGEST val;

  if (*arg == nullptr || **arg == '\0')
    {
      if (var_type == var_uinteger)
	error_no_arg (_("integer to set it to, or \"unlimited\"."));
      else
	error_no_arg (_("integer to set it to."));
    }

  if (var_type == var_uinteger && is_unlimited_literal (arg, expression))
    val = 0;
  else if (expression)
    val = parse_and_eval_long (*arg);
  else
    val = get_ulongest (arg);

  if (var_type == var_uinteger && val == 0)
    val = UINT_MAX;
  else if (val < 0
	   /* For var_uinteger, don't let the user set the value
	      to UINT_MAX directly, as that exposes an
	      implementation detail to the user interface.  */
	   || (var_type == var_uinteger && val >= UINT_MAX)
	   || (var_type == var_zuinteger && val > UINT_MAX))
    error (_("integer %s out of range"), plongest (val));

  return val;
}

/* See cli-setshow.h.  */

int
parse_cli_var_zuinteger_unlimited (const char **arg, bool expression)
{
  LONGEST val;

  if (*arg == nullptr || **arg == '\0')
    error_no_arg (_("integer to set it to, or \"unlimited\"."));

  if (is_unlimited_literal (arg, expression))
    val = -1;
  else if (expression)
    val = parse_and_eval_long (*arg);
  else
    val = get_ulongest (arg);

  if (val > INT_MAX)
    error (_("integer %s out of range"), plongest (val));
  else if (val < -1)
    error (_("only -1 is allowed to set as unlimited"));

  return val;
}

/* See cli-setshow.h.  */

const char *
parse_cli_var_enum (const char **args, const char *const *enums)
{
  /* If no argument was supplied, print an informative error
     message.  */
  if (args == NULL || *args == NULL || **args == '\0')
    {
      std::string msg;

      for (size_t i = 0; enums[i]; i++)
	{
	  if (i != 0)
	    msg += ", ";
	  msg += enums[i];
	}
      error (_("Requires an argument. Valid arguments are %s."),
	     msg.c_str ());
    }

  const char *p = skip_to_space (*args);
  size_t len = p - *args;

  int nmatches = 0;
  const char *match = NULL;
  for (size_t i = 0; enums[i]; i++)
    if (strncmp (*args, enums[i], len) == 0)
      {
	if (enums[i][len] == '\0')
	  {
	    match = enums[i];
	    nmatches = 1;
	    break; /* Exact match.  */
	  }
	else
	  {
	    match = enums[i];
	    nmatches++;
	  }
      }

  if (nmatches == 0)
    error (_("Undefined item: \"%.*s\"."), (int) len, *args);

  if (nmatches > 1)
    error (_("Ambiguous item \"%.*s\"."), (int) len, *args);

  *args += len;
  return match;
}

/* Do a "set" command.  ARG is NULL if no argument, or the
   text of the argument, and FROM_TTY is nonzero if this command is
   being entered directly by the user (i.e. these are just like any
   other command).  C is the command list element for the command.  */

void
do_set_command (const char *arg, int from_tty, struct cmd_list_element *c)
{
  /* A flag to indicate the option is changed or not.  */
  int option_changed = 0;

  gdb_assert (c->type == set_cmd);

  if (arg == NULL)
    arg = "";

  switch (c->var_type)
    {
    case var_string:
      {
	char *newobj;
	const char *p;
	char *q;
	int ch;

	newobj = (char *) xmalloc (strlen (arg) + 2);
	p = arg;
	q = newobj;
	while ((ch = *p++) != '\000')
	  {
	    if (ch == '\\')
	      {
		/* \ at end of argument is used after spaces
		   so they won't be lost.  */
		/* This is obsolete now that we no longer strip
		   trailing whitespace and actually, the backslash
		   didn't get here in my test, readline or
		   something did something funky with a backslash
		   right before a newline.  */
		if (*p == 0)
		  break;
		ch = parse_escape (get_current_arch (), &p);
		if (ch == 0)
		  break;	/* C loses */
		else if (ch > 0)
		  *q++ = ch;
	      }
	    else
	      *q++ = ch;
	  }
#if 0
	if (*(p - 1) != '\\')
	  *q++ = ' ';
#endif
	*q++ = '\0';
	newobj = (char *) xrealloc (newobj, q - newobj);

	if (*(char **) c->var == NULL
	    || strcmp (*(char **) c->var, newobj) != 0)
	  {
	    xfree (*(char **) c->var);
	    *(char **) c->var = newobj;

	    option_changed = 1;
	  }
	else
	  xfree (newobj);
      }
      break;
    case var_string_noescape:
      if (*(char **) c->var == NULL || strcmp (*(char **) c->var, arg) != 0)
	{
	  xfree (*(char **) c->var);
	  *(char **) c->var = xstrdup (arg);

	  option_changed = 1;
	}
      break;
    case var_filename:
      if (*arg == '\0')
	error_no_arg (_("filename to set it to."));
      /* FALLTHROUGH */
    case var_optional_filename:
      {
	char *val = NULL;

	if (*arg != '\0')
	  {
	    /* Clear trailing whitespace of filename.  */
	    const char *ptr = arg + strlen (arg) - 1;
	    char *copy;

	    while (ptr >= arg && (*ptr == ' ' || *ptr == '\t'))
	      ptr--;
	    copy = xstrndup (arg, ptr + 1 - arg);

	    val = tilde_expand (copy);
	    xfree (copy);
	  }
	else
	  val = xstrdup ("");

	if (*(char **) c->var == NULL
	    || strcmp (*(char **) c->var, val) != 0)
	  {
	    xfree (*(char **) c->var);
	    *(char **) c->var = val;

	    option_changed = 1;
	  }
	else
	  xfree (val);
      }
      break;
    case var_boolean:
      {
	int val = parse_cli_boolean_value (arg);

	if (val < 0)
	  error (_("\"on\" or \"off\" expected."));
	if (val != *(bool *) c->var)
	  {
	    *(bool *) c->var = val;

	    option_changed = 1;
	  }
      }
      break;
    case var_auto_boolean:
      {
	enum auto_boolean val = parse_auto_binary_operation (arg);

	if (*(enum auto_boolean *) c->var != val)
	  {
	    *(enum auto_boolean *) c->var = val;

	    option_changed = 1;
	  }
      }
      break;
    case var_uinteger:
    case var_zuinteger:
      {
	unsigned int val = parse_cli_var_uinteger (c->var_type, &arg, true);

	if (*(unsigned int *) c->var != val)
	  {
	    *(unsigned int *) c->var = val;

	    option_changed = 1;
	  }
      }
      break;
    case var_integer:
    case var_zinteger:
      {
	LONGEST val;

	if (*arg == '\0')
	  {
	    if (c->var_type == var_integer)
	      error_no_arg (_("integer to set it to, or \"unlimited\"."));
	    else
	      error_no_arg (_("integer to set it to."));
	  }

	if (c->var_type == var_integer && is_unlimited_literal (&arg, true))
	  val = 0;
	else
	  val = parse_and_eval_long (arg);

	if (val == 0 && c->var_type == var_integer)
	  val = INT_MAX;
	else if (val < INT_MIN
		 /* For var_integer, don't let the user set the value
		    to INT_MAX directly, as that exposes an
		    implementation detail to the user interface.  */
		 || (c->var_type == var_integer && val >= INT_MAX)
		 || (c->var_type == var_zinteger && val > INT_MAX))
	  error (_("integer %s out of range"), plongest (val));

	if (*(int *) c->var != val)
	  {
	    *(int *) c->var = val;

	    option_changed = 1;
	  }
	break;
      }
    case var_enum:
      {
	const char *end_arg = arg;
	const char *match = parse_cli_var_enum (&end_arg, c->enums);

	int len = end_arg - arg;
	const char *after = skip_spaces (end_arg);
	if (*after != '\0')
	  error (_("Junk after item \"%.*s\": %s"), len, arg, after);

	if (*(const char **) c->var != match)
	  {
	    *(const char **) c->var = match;

	    option_changed = 1;
	  }
      }
      break;
    case var_zuinteger_unlimited:
      {
	int val = parse_cli_var_zuinteger_unlimited (&arg, true);

	if (*(int *) c->var != val)
	  {
	    *(int *) c->var = val;
	    option_changed = 1;
	  }
      }
      break;
    default:
      error (_("gdb internal error: bad var_type in do_setshow_command"));
    }
  c->func (c, NULL, from_tty);

  if (notify_command_param_changed_p (option_changed, c))
    {
      char *name, *cp;
      struct cmd_list_element **cmds;
      struct cmd_list_element *p;
      int i;
      int length = 0;

      /* Compute the whole multi-word command options.  If user types command
	 'set foo bar baz on', c->name is 'baz', and GDB can't pass "bar" to
	 command option change notification, because it is confusing.  We can
	 trace back through field 'prefix' to compute the whole options,
	 and pass "foo bar baz" to notification.  */

      for (i = 0, p = c; p != NULL; i++)
	{
	  length += strlen (p->name);
	  length++;

	  p = p->prefix;
	}
      cp = name = (char *) xmalloc (length);
      cmds = XNEWVEC (struct cmd_list_element *, i);

      /* Track back through filed 'prefix' and cache them in CMDS.  */
      for (i = 0, p = c; p != NULL; i++)
	{
	  cmds[i] = p;
	  p = p->prefix;
	}

      /* Don't trigger any observer notification if prefixlist is not
	 setlist.  */
      i--;
      if (cmds[i]->prefixlist != &setlist)
	{
	  xfree (cmds);
	  xfree (name);

	  return;
	}
      /* Traverse them in the reversed order, and copy their names into
	 NAME.  */
      for (i--; i >= 0; i--)
	{
	  memcpy (cp, cmds[i]->name, strlen (cmds[i]->name));
	  cp += strlen (cmds[i]->name);

	  if (i != 0)
	    {
	      cp[0] = ' ';
	      cp++;
	    }
	}
      cp[0] = 0;

      xfree (cmds);

      switch (c->var_type)
	{
	case var_string:
	case var_string_noescape:
	case var_filename:
	case var_optional_filename:
	case var_enum:
	  gdb::observers::command_param_changed.notify (name, *(char **) c->var);
	  break;
	case var_boolean:
	  {
	    const char *opt = *(bool *) c->var ? "on" : "off";

	    gdb::observers::command_param_changed.notify (name, opt);
	  }
	  break;
	case var_auto_boolean:
	  {
	    const char *s = auto_boolean_enums[*(enum auto_boolean *) c->var];

	    gdb::observers::command_param_changed.notify (name, s);
	  }
	  break;
	case var_uinteger:
	case var_zuinteger:
	  {
	    char s[64];

	    xsnprintf (s, sizeof s, "%u", *(unsigned int *) c->var);
	    gdb::observers::command_param_changed.notify (name, s);
	  }
	  break;
	case var_integer:
	case var_zinteger:
	case var_zuinteger_unlimited:
	  {
	    char s[64];

	    xsnprintf (s, sizeof s, "%d", *(int *) c->var);
	    gdb::observers::command_param_changed.notify (name, s);
	  }
	  break;
	}
      xfree (name);
    }
}

/* See cli/cli-setshow.h.  */

std::string
get_setshow_command_value_string (const cmd_list_element *c)
{
  string_file stb;

  switch (c->var_type)
    {
    case var_string:
      if (*(char **) c->var)
	stb.putstr (*(char **) c->var, '"');
      break;
    case var_string_noescape:
    case var_optional_filename:
    case var_filename:
    case var_enum:
      if (*(char **) c->var)
	stb.puts (*(char **) c->var);
      break;
    case var_boolean:
      stb.puts (*(bool *) c->var ? "on" : "off");
      break;
    case var_auto_boolean:
      switch (*(enum auto_boolean*) c->var)
	{
	case AUTO_BOOLEAN_TRUE:
	  stb.puts ("on");
	  break;
	case AUTO_BOOLEAN_FALSE:
	  stb.puts ("off");
	  break;
	case AUTO_BOOLEAN_AUTO:
	  stb.puts ("auto");
	  break;
	default:
	  gdb_assert_not_reached ("invalid var_auto_boolean");
	  break;
	}
      break;
    case var_uinteger:
    case var_zuinteger:
      if (c->var_type == var_uinteger
	  && *(unsigned int *) c->var == UINT_MAX)
	stb.puts ("unlimited");
      else
	stb.printf ("%u", *(unsigned int *) c->var);
      break;
    case var_integer:
    case var_zinteger:
      if (c->var_type == var_integer
	  && *(int *) c->var == INT_MAX)
	stb.puts ("unlimited");
      else
	stb.printf ("%d", *(int *) c->var);
      break;
    case var_zuinteger_unlimited:
      {
	if (*(int *) c->var == -1)
	  stb.puts ("unlimited");
	else
	  stb.printf ("%d", *(int *) c->var);
      }
      break;
    default:
      gdb_assert_not_reached ("bad var_type");
    }

  return std::move (stb.string ());
}


/* Do a "show" command.  ARG is NULL if no argument, or the
   text of the argument, and FROM_TTY is nonzero if this command is
   being entered directly by the user (i.e. these are just like any
   other command).  C is the command list element for the command.  */

void
do_show_command (const char *arg, int from_tty, struct cmd_list_element *c)
{
  struct ui_out *uiout = current_uiout;

  gdb_assert (c->type == show_cmd);

  /* Possibly call the pre hook.  */
  if (c->pre_show_hook)
    (c->pre_show_hook) (c);

  std::string val = get_setshow_command_value_string (c);

  /* FIXME: cagney/2005-02-10: There should be MI and CLI specific
     versions of code to print the value out.  */

  if (uiout->is_mi_like_p ())
    uiout->field_string ("value", val.c_str ());
  else
    {
      if (c->show_value_func != NULL)
	c->show_value_func (gdb_stdout, from_tty, c, val.c_str ());
      else
	deprecated_show_value_hack (gdb_stdout, from_tty, c, val.c_str ());
    }

  c->func (c, NULL, from_tty);
}

/* Show all the settings in a list of show commands.  */

void
cmd_show_list (struct cmd_list_element *list, int from_tty)
{
  struct ui_out *uiout = current_uiout;

  ui_out_emit_tuple tuple_emitter (uiout, "showlist");
  for (; list != NULL; list = list->next)
    {
      /* We skip show command aliases to avoid showing duplicated values.  */

      /* If we find a prefix, run its list, prefixing our output by its
         prefix (with "show " skipped).  */
      if (list->prefixlist && list->cmd_pointer == nullptr)
	{
	  ui_out_emit_tuple optionlist_emitter (uiout, "optionlist");
	  const char *new_prefix = strstr (list->prefixname, "show ") + 5;

	  if (uiout->is_mi_like_p ())
	    uiout->field_string ("prefix", new_prefix);
	  cmd_show_list (*list->prefixlist, from_tty);
	}
      else if (list->theclass != no_set_class && list->cmd_pointer == nullptr)
	{
	  ui_out_emit_tuple option_emitter (uiout, "option");

	  {
	    /* If we find a prefix, output it (with "show " skipped).  */
	    const char *prefixname
	      = (list->prefix == nullptr ? ""
		 : strstr (list->prefix->prefixname, "show ") + 5);
	    uiout->text (prefixname);
	  }
	  uiout->field_string ("name", list->name);
	  uiout->text (":  ");
	  if (list->type == show_cmd)
	    do_show_command (NULL, from_tty, list);
	  else
	    cmd_func (list, NULL, from_tty);
	}
    }
}


