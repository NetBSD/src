/* CLI utilities.

   Copyright (C) 2011-2019 Free Software Foundation, Inc.

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
#include "cli/cli-utils.h"
#include "value.h"

#include <ctype.h>

static std::string extract_arg_maybe_quoted (const char **arg);

/* See documentation in cli-utils.h.  */

int
get_number_trailer (const char **pp, int trailer)
{
  int retval = 0;	/* default */
  const char *p = *pp;
  bool negative = false;

  if (*p == '-')
    {
      ++p;
      negative = true;
    }

  if (*p == '$')
    {
      struct value *val = value_from_history_ref (p, &p);

      if (val)	/* Value history reference */
	{
	  if (TYPE_CODE (value_type (val)) == TYPE_CODE_INT)
	    retval = value_as_long (val);
	  else
	    {
	      printf_filtered (_("History value must have integer type.\n"));
	      retval = 0;
	    }
	}
      else	/* Convenience variable */
	{
	  /* Internal variable.  Make a copy of the name, so we can
	     null-terminate it to pass to lookup_internalvar().  */
	  char *varname;
	  const char *start = ++p;
	  LONGEST longest_val;

	  while (isalnum (*p) || *p == '_')
	    p++;
	  varname = (char *) alloca (p - start + 1);
	  strncpy (varname, start, p - start);
	  varname[p - start] = '\0';
	  if (get_internalvar_integer (lookup_internalvar (varname),
				       &longest_val))
	    retval = (int) longest_val;
	  else
	    {
	      printf_filtered (_("Convenience variable must "
				 "have integer value.\n"));
	      retval = 0;
	    }
	}
    }
  else
    {
      const char *p1 = p;
      while (*p >= '0' && *p <= '9')
	++p;
      if (p == p1)
	/* There is no number here.  (e.g. "cond a == b").  */
	{
	  /* Skip non-numeric token.  */
	  while (*p && !isspace((int) *p))
	    ++p;
	  /* Return zero, which caller must interpret as error.  */
	  retval = 0;
	}
      else
	retval = atoi (p1);
    }
  if (!(isspace (*p) || *p == '\0' || *p == trailer))
    {
      /* Trailing junk: return 0 and let caller print error msg.  */
      while (!(isspace (*p) || *p == '\0' || *p == trailer))
	++p;
      retval = 0;
    }
  p = skip_spaces (p);
  *pp = p;
  return negative ? -retval : retval;
}

/* See documentation in cli-utils.h.  */

int
get_number (const char **pp)
{
  return get_number_trailer (pp, '\0');
}

/* See documentation in cli-utils.h.  */

int
get_number (char **pp)
{
  int result;
  const char *p = *pp;

  result = get_number_trailer (&p, '\0');
  *pp = (char *) p;
  return result;
}

/* See documentation in cli-utils.h.  */

bool
extract_info_print_args (const char **args,
			 bool *quiet,
			 std::string *regexp,
			 std::string *t_regexp)
{
  /* Check for NAMEREGEXP or -- NAMEREGEXP.  */
  if (**args != '-' || check_for_argument (args, "--", 2))
    {
      *args = skip_spaces (*args);
      *regexp = *args;
      *args = NULL;
      return true;
    }

  if (check_for_argument (args, "-t", 2))
    {
      *t_regexp = extract_arg_maybe_quoted (args);
      *args = skip_spaces (*args);
      return true;
    }

  if (check_for_argument (args, "-q", 2))
    {
      *quiet = true;
      *args = skip_spaces (*args);
      return true;
    }

  return false;
}

/* See documentation in cli-utils.h.  */

void
report_unrecognized_option_error (const char *command, const char *args)
{
  std::string option = extract_arg (&args);

  error (_("Unrecognized option '%s' to %s command.  "
	   "Try \"help %s\"."), option.c_str (),
	 command, command);
}

/* See documentation in cli-utils.h.  */

const char *
info_print_args_help (const char *prefix,
		      const char *entity_kind)
{
  return xstrprintf (_("\
%sIf NAMEREGEXP is provided, only prints the %s whose name\n\
matches NAMEREGEXP.\n\
If -t TYPEREGEXP is provided, only prints the %s whose type\n\
matches TYPEREGEXP.  Note that the matching is done with the type\n\
printed by the 'whatis' command.\n\
By default, the command might produce headers and/or messages indicating\n\
why no %s can be printed.\n\
The flag -q disables the production of these headers and messages."),
		     prefix, entity_kind, entity_kind, entity_kind);
}

/* See documentation in cli-utils.h.  */

number_or_range_parser::number_or_range_parser (const char *string)
{
  init (string);
}

/* See documentation in cli-utils.h.  */

void
number_or_range_parser::init (const char *string)
{
  m_cur_tok = string;
  m_last_retval = 0;
  m_end_value = 0;
  m_end_ptr = NULL;
  m_in_range = false;
}

/* See documentation in cli-utils.h.  */

int
number_or_range_parser::get_number ()
{
  if (m_in_range)
    {
      /* All number-parsing has already been done.  Return the next
	 integer value (one greater than the saved previous value).
	 Do not advance the token pointer until the end of range is
	 reached.  */

      if (++m_last_retval == m_end_value)
	{
	  /* End of range reached; advance token pointer.  */
	  m_cur_tok = m_end_ptr;
	  m_in_range = false;
	}
    }
  else if (*m_cur_tok != '-')
    {
      /* Default case: state->m_cur_tok is pointing either to a solo
	 number, or to the first number of a range.  */
      m_last_retval = get_number_trailer (&m_cur_tok, '-');
      /* If get_number_trailer has found a -, it might be the start
	 of a command option.  So, do not parse a range if the - is
	 followed by an alpha.  */
      if (*m_cur_tok == '-' && !isalpha (*(m_cur_tok + 1)))
	{
	  const char **temp;

	  /* This is the start of a range (<number1> - <number2>).
	     Skip the '-', parse and remember the second number,
	     and also remember the end of the final token.  */

	  temp = &m_end_ptr;
	  m_end_ptr = skip_spaces (m_cur_tok + 1);
	  m_end_value = ::get_number (temp);
	  if (m_end_value < m_last_retval)
	    {
	      error (_("inverted range"));
	    }
	  else if (m_end_value == m_last_retval)
	    {
	      /* Degenerate range (number1 == number2).  Advance the
		 token pointer so that the range will be treated as a
		 single number.  */
	      m_cur_tok = m_end_ptr;
	    }
	  else
	    m_in_range = true;
	}
    }
  else
    {
      if (isdigit (*(m_cur_tok + 1)))
	error (_("negative value"));
      if (*(m_cur_tok + 1) == '$')
	{
	  /* Convenience variable.  */
	  m_last_retval = ::get_number (&m_cur_tok);
	  if (m_last_retval < 0)
	    error (_("negative value"));
	}
    }
  return m_last_retval;
}

/* See documentation in cli-utils.h.  */

void
number_or_range_parser::setup_range (int start_value, int end_value,
				     const char *end_ptr)
{
  gdb_assert (start_value > 0);

  m_in_range = true;
  m_end_ptr = end_ptr;
  m_last_retval = start_value - 1;
  m_end_value = end_value;
}

/* See documentation in cli-utils.h.  */

bool
number_or_range_parser::finished () const
{
  /* Parsing is finished when at end of string or null string,
     or we are not in a range and not in front of an integer, negative
     integer, convenience var or negative convenience var.  */
  return (m_cur_tok == NULL || *m_cur_tok == '\0'
	  || (!m_in_range
	      && !(isdigit (*m_cur_tok) || *m_cur_tok == '$')
	      && !(*m_cur_tok == '-'
		   && (isdigit (m_cur_tok[1]) || m_cur_tok[1] == '$'))));
}

/* Accept a number and a string-form list of numbers such as is 
   accepted by get_number_or_range.  Return TRUE if the number is
   in the list.

   By definition, an empty list includes all numbers.  This is to 
   be interpreted as typing a command such as "delete break" with 
   no arguments.  */

int
number_is_in_list (const char *list, int number)
{
  if (list == NULL || *list == '\0')
    return 1;

  number_or_range_parser parser (list);

  if (parser.finished ())
    error (_("Arguments must be numbers or '$' variables."));
  while (!parser.finished ())
    {
      int gotnum = parser.get_number ();

      if (gotnum == 0)
	error (_("Arguments must be numbers or '$' variables."));
      if (gotnum == number)
	return 1;
    }
  return 0;
}

/* See documentation in cli-utils.h.  */

const char *
remove_trailing_whitespace (const char *start, const char *s)
{
  while (s > start && isspace (*(s - 1)))
    --s;

  return s;
}

/* A helper function to extract an argument from *ARG.  An argument is
   delimited by whitespace, but it can also be optionally quoted.
   The quoting and special characters are handled similarly to
   the parsing done by gdb_argv.
   The return value is empty if no argument was found.  */

static std::string
extract_arg_maybe_quoted (const char **arg)
{
  bool squote = false;
  bool dquote = false;
  bool bsquote = false;
  std::string result;
  const char *p = *arg;

  /* Find the start of the argument.  */
  p = skip_spaces (p);

  /* Parse p similarly to gdb_argv buildargv function.  */
  while (*p != '\0')
    {
      if (isspace (*p) && !squote && !dquote && !bsquote)
	  break;
      else
	{
	  if (bsquote)
	    {
	      bsquote = false;
	      result += *p;
	    }
	  else if (*p == '\\')
	      bsquote = true;
	  else if (squote)
	    {
	      if (*p == '\'')
		  squote = false;
	      else
		  result += *p;
	    }
	  else if (dquote)
	    {
	      if (*p == '"')
		  dquote = false;
	      else
		  result += *p;
	    }
	  else
	    {
	      if (*p == '\'')
		  squote = true;
	      else if (*p == '"')
		  dquote = true;
	      else
		  result += *p;
	    }
	  p++;
	}
    }

  *arg = p;
  return result;
}

/* See documentation in cli-utils.h.  */

std::string
extract_arg (const char **arg)
{
  const char *result;

  if (!*arg)
    return std::string ();

  /* Find the start of the argument.  */
  *arg = skip_spaces (*arg);
  if (!**arg)
    return std::string ();
  result = *arg;

  /* Find the end of the argument.  */
  *arg = skip_to_space (*arg + 1);

  if (result == *arg)
    return std::string ();

  return std::string (result, *arg - result);
}

/* See documentation in cli-utils.h.  */

std::string
extract_arg (char **arg)
{
  const char *arg_const = *arg;
  std::string result;

  result = extract_arg (&arg_const);
  *arg += arg_const - *arg;
  return result;
}

/* See documentation in cli-utils.h.  */

int
check_for_argument (const char **str, const char *arg, int arg_len)
{
  if (strncmp (*str, arg, arg_len) == 0
      && ((*str)[arg_len] == '\0' || isspace ((*str)[arg_len])))
    {
      *str += arg_len;
      return 1;
    }
  return 0;
}

/* See documentation in cli-utils.h.  */

int
parse_flags (const char **str, const char *flags)
{
  const char *p = skip_spaces (*str);

  if (p[0] == '-'
      && isalpha (p[1])
      && (p[2] == '\0' || isspace (p[2])))
    {
      const char pf = p[1];
      const char *f = flags;

      while (*f != '\0')
	{
	  if (*f == pf)
	    {
	      *str = skip_spaces (p + 2);
	      return f - flags + 1;
	    }
	  f++;
	}
    }

  return 0;
}

/* See documentation in cli-utils.h.  */

bool
parse_flags_qcs (const char *which_command, const char **str,
		 qcs_flags *flags)
{
  switch (parse_flags (str, "qcs"))
    {
    case 0:
      return false;
    case 1:
      flags->quiet = true;
      break;
    case 2:
      flags->cont = true;
      break;
    case 3:
      flags->silent = true;
      break;
    default:
      gdb_assert_not_reached ("int qcs flag out of bound");
    }

  if (flags->cont && flags->silent)
    error (_("%s: -c and -s are mutually exclusive"), which_command);

  return true;
}
