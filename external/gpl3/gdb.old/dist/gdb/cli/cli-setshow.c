/* Handle set and show GDB commands.

   Copyright (C) 2000-2016 Free Software Foundation, Inc.

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
#include "observer.h"

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
      if (strncmp (arg, "on", length) == 0
	  || strncmp (arg, "1", length) == 0
	  || strncmp (arg, "yes", length) == 0
	  || strncmp (arg, "enable", length) == 0)
	return AUTO_BOOLEAN_TRUE;
      else if (strncmp (arg, "off", length) == 0
	       || strncmp (arg, "0", length) == 0
	       || strncmp (arg, "no", length) == 0
	       || strncmp (arg, "disable", length) == 0)
	return AUTO_BOOLEAN_FALSE;
      else if (strncmp (arg, "auto", length) == 0
	       || (strncmp (arg, "-1", length) == 0 && length > 1))
	return AUTO_BOOLEAN_AUTO;
    }
  error (_("\"on\", \"off\" or \"auto\" expected."));
  return AUTO_BOOLEAN_AUTO; /* Pacify GCC.  */
}

/* See cli-setshow.h.  */

int
parse_cli_boolean_value (const char *arg)
{
  int length;

  if (!arg || !*arg)
    return 1;

  length = strlen (arg);

  while (arg[length - 1] == ' ' || arg[length - 1] == '\t')
    length--;

  if (strncmp (arg, "on", length) == 0
      || strncmp (arg, "1", length) == 0
      || strncmp (arg, "yes", length) == 0
      || strncmp (arg, "enable", length) == 0)
    return 1;
  else if (strncmp (arg, "off", length) == 0
	   || strncmp (arg, "0", length) == 0
	   || strncmp (arg, "no", length) == 0
	   || strncmp (arg, "disable", length) == 0)
    return 0;
  else
    return -1;
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
  /* Print doc minus "show" at start.  */
  print_doc_line (gdb_stdout, c->doc + 5);
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

static int
is_unlimited_literal (const char *arg)
{
  size_t len = sizeof ("unlimited") - 1;

  arg = skip_spaces_const (arg);

  return (strncmp (arg, "unlimited", len) == 0
	  && (isspace (arg[len]) || arg[len] == '\0'));
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

  switch (c->var_type)
    {
    case var_string:
      {
	char *newobj;
	const char *p;
	char *q;
	int ch;

	if (arg == NULL)
	  arg = "";
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
      if (arg == NULL)
	arg = "";

      if (*(char **) c->var == NULL || strcmp (*(char **) c->var, arg) != 0)
	{
	  xfree (*(char **) c->var);
	  *(char **) c->var = xstrdup (arg);

	  option_changed = 1;
	}
      break;
    case var_filename:
      if (arg == NULL)
	error_no_arg (_("filename to set it to."));
      /* FALLTHROUGH */
    case var_optional_filename:
      {
	char *val = NULL;

	if (arg != NULL)
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
	if (val != *(int *) c->var)
	  {
	    *(int *) c->var = val;

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
	LONGEST val;

	if (arg == NULL)
	  {
	    if (c->var_type == var_uinteger)
	      error_no_arg (_("integer to set it to, or \"unlimited\"."));
	    else
	      error_no_arg (_("integer to set it to."));
	  }

	if (c->var_type == var_uinteger && is_unlimited_literal (arg))
	  val = 0;
	else
	  val = parse_and_eval_long (arg);

	if (c->var_type == var_uinteger && val == 0)
	  val = UINT_MAX;
	else if (val < 0
		 /* For var_uinteger, don't let the user set the value
		    to UINT_MAX directly, as that exposes an
		    implementation detail to the user interface.  */
		 || (c->var_type == var_uinteger && val >= UINT_MAX)
		 || (c->var_type == var_zuinteger && val > UINT_MAX))
	  error (_("integer %s out of range"), plongest (val));

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

	if (arg == NULL)
	  {
	    if (c->var_type == var_integer)
	      error_no_arg (_("integer to set it to, or \"unlimited\"."));
	    else
	      error_no_arg (_("integer to set it to."));
	  }

	if (c->var_type == var_integer && is_unlimited_literal (arg))
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
	int i;
	int len;
	int nmatches;
	const char *match = NULL;
	const char *p;

	/* If no argument was supplied, print an informative error
	   message.  */
	if (arg == NULL)
	  {
	    char *msg;
	    int msg_len = 0;

	    for (i = 0; c->enums[i]; i++)
	      msg_len += strlen (c->enums[i]) + 2;

	    msg = (char *) xmalloc (msg_len);
	    *msg = '\0';
	    make_cleanup (xfree, msg);

	    for (i = 0; c->enums[i]; i++)
	      {
		if (i != 0)
		  strcat (msg, ", ");
		strcat (msg, c->enums[i]);
	      }
	    error (_("Requires an argument. Valid arguments are %s."), 
		   msg);
	  }

	p = strchr (arg, ' ');

	if (p)
	  len = p - arg;
	else
	  len = strlen (arg);

	nmatches = 0;
	for (i = 0; c->enums[i]; i++)
	  if (strncmp (arg, c->enums[i], len) == 0)
	    {
	      if (c->enums[i][len] == '\0')
		{
		  match = c->enums[i];
		  nmatches = 1;
		  break; /* Exact match.  */
		}
	      else
		{
		  match = c->enums[i];
		  nmatches++;
		}
	    }

	if (nmatches <= 0)
	  error (_("Undefined item: \"%s\"."), arg);

	if (nmatches > 1)
	  error (_("Ambiguous item \"%s\"."), arg);

	if (*(const char **) c->var != match)
	  {
	    *(const char **) c->var = match;

	    option_changed = 1;
	  }
      }
      break;
    case var_zuinteger_unlimited:
      {
	LONGEST val;

	if (arg == NULL)
	  error_no_arg (_("integer to set it to, or \"unlimited\"."));

	if (is_unlimited_literal (arg))
	  val = -1;
	else
	  val = parse_and_eval_long (arg);

	if (val > INT_MAX)
	  error (_("integer %s out of range"), plongest (val));
	else if (val < -1)
	  error (_("only -1 is allowed to set as unlimited"));

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
	  observer_notify_command_param_changed (name, *(char **) c->var);
	  break;
	case var_boolean:
	  {
	    const char *opt = *(int *) c->var ? "on" : "off";

	    observer_notify_command_param_changed (name, opt);
	  }
	  break;
	case var_auto_boolean:
	  {
	    const char *s = auto_boolean_enums[*(enum auto_boolean *) c->var];

	    observer_notify_command_param_changed (name, s);
	  }
	  break;
	case var_uinteger:
	case var_zuinteger:
	  {
	    char s[64];

	    xsnprintf (s, sizeof s, "%u", *(unsigned int *) c->var);
	    observer_notify_command_param_changed (name, s);
	  }
	  break;
	case var_integer:
	case var_zinteger:
	case var_zuinteger_unlimited:
	  {
	    char s[64];

	    xsnprintf (s, sizeof s, "%d", *(int *) c->var);
	    observer_notify_command_param_changed (name, s);
	  }
	  break;
	}
      xfree (name);
    }
}

/* Do a "show" command.  ARG is NULL if no argument, or the
   text of the argument, and FROM_TTY is nonzero if this command is
   being entered directly by the user (i.e. these are just like any
   other command).  C is the command list element for the command.  */

void
do_show_command (const char *arg, int from_tty, struct cmd_list_element *c)
{
  struct ui_out *uiout = current_uiout;
  struct cleanup *old_chain;
  struct ui_file *stb;

  gdb_assert (c->type == show_cmd);

  stb = mem_fileopen ();
  old_chain = make_cleanup_ui_file_delete (stb);

  /* Possibly call the pre hook.  */
  if (c->pre_show_hook)
    (c->pre_show_hook) (c);

  switch (c->var_type)
    {
    case var_string:
      if (*(char **) c->var)
	fputstr_filtered (*(char **) c->var, '"', stb);
      break;
    case var_string_noescape:
    case var_optional_filename:
    case var_filename:
    case var_enum:
      if (*(char **) c->var)
	fputs_filtered (*(char **) c->var, stb);
      break;
    case var_boolean:
      fputs_filtered (*(int *) c->var ? "on" : "off", stb);
      break;
    case var_auto_boolean:
      switch (*(enum auto_boolean*) c->var)
	{
	case AUTO_BOOLEAN_TRUE:
	  fputs_filtered ("on", stb);
	  break;
	case AUTO_BOOLEAN_FALSE:
	  fputs_filtered ("off", stb);
	  break;
	case AUTO_BOOLEAN_AUTO:
	  fputs_filtered ("auto", stb);
	  break;
	default:
	  internal_error (__FILE__, __LINE__,
			  _("do_show_command: "
			    "invalid var_auto_boolean"));
	  break;
	}
      break;
    case var_uinteger:
    case var_zuinteger:
      if (c->var_type == var_uinteger
	  && *(unsigned int *) c->var == UINT_MAX)
	fputs_filtered ("unlimited", stb);
      else
	fprintf_filtered (stb, "%u", *(unsigned int *) c->var);
      break;
    case var_integer:
    case var_zinteger:
      if (c->var_type == var_integer
	  && *(int *) c->var == INT_MAX)
	fputs_filtered ("unlimited", stb);
      else
	fprintf_filtered (stb, "%d", *(int *) c->var);
      break;
    case var_zuinteger_unlimited:
      {
	if (*(int *) c->var == -1)
	  fputs_filtered ("unlimited", stb);
	else
	  fprintf_filtered (stb, "%d", *(int *) c->var);
      }
      break;
    default:
      error (_("gdb internal error: bad var_type in do_show_command"));
    }


  /* FIXME: cagney/2005-02-10: Need to split this in half: code to
     convert the value into a string (esentially the above); and
     code to print the value out.  For the latter there should be
     MI and CLI specific versions.  */

  if (ui_out_is_mi_like_p (uiout))
    ui_out_field_stream (uiout, "value", stb);
  else
    {
      char *value = ui_file_xstrdup (stb, NULL);

      make_cleanup (xfree, value);
      if (c->show_value_func != NULL)
	c->show_value_func (gdb_stdout, from_tty, c, value);
      else
	deprecated_show_value_hack (gdb_stdout, from_tty, c, value);
    }
  do_cleanups (old_chain);

  c->func (c, NULL, from_tty);
}

/* Show all the settings in a list of show commands.  */

void
cmd_show_list (struct cmd_list_element *list, int from_tty, const char *prefix)
{
  struct cleanup *showlist_chain;
  struct ui_out *uiout = current_uiout;

  showlist_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "showlist");
  for (; list != NULL; list = list->next)
    {
      /* If we find a prefix, run its list, prefixing our output by its
         prefix (with "show " skipped).  */
      if (list->prefixlist && !list->abbrev_flag)
	{
	  struct cleanup *optionlist_chain
	    = make_cleanup_ui_out_tuple_begin_end (uiout, "optionlist");
	  const char *new_prefix = strstr (list->prefixname, "show ") + 5;

	  if (ui_out_is_mi_like_p (uiout))
	    ui_out_field_string (uiout, "prefix", new_prefix);
	  cmd_show_list (*list->prefixlist, from_tty, new_prefix);
	  /* Close the tuple.  */
	  do_cleanups (optionlist_chain);
	}
      else
	{
	  if (list->theclass != no_set_class)
	    {
	      struct cleanup *option_chain
		= make_cleanup_ui_out_tuple_begin_end (uiout, "option");

	      ui_out_text (uiout, prefix);
	      ui_out_field_string (uiout, "name", list->name);
	      ui_out_text (uiout, ":  ");
	      if (list->type == show_cmd)
		do_show_command ((char *) NULL, from_tty, list);
	      else
		cmd_func (list, NULL, from_tty);
	      /* Close the tuple.  */
	      do_cleanups (option_chain);
	    }
	}
    }
  /* Close the tuple.  */
  do_cleanups (showlist_chain);
}

