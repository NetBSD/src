/* CLI utilities.

   Copyright (C) 2011-2017 Free Software Foundation, Inc.

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

/* See documentation in cli-utils.h.  */

int
get_number_trailer (const char **pp, int trailer)
{
  int retval = 0;	/* default */
  const char *p = *pp;

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
	  LONGEST val;

	  while (isalnum (*p) || *p == '_')
	    p++;
	  varname = (char *) alloca (p - start + 1);
	  strncpy (varname, start, p - start);
	  varname[p - start] = '\0';
	  if (get_internalvar_integer (lookup_internalvar (varname), &val))
	    retval = (int) val;
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
      if (*p == '-')
	++p;
      while (*p >= '0' && *p <= '9')
	++p;
      if (p == *pp)
	/* There is no number here.  (e.g. "cond a == b").  */
	{
	  /* Skip non-numeric token.  */
	  while (*p && !isspace((int) *p))
	    ++p;
	  /* Return zero, which caller must interpret as error.  */
	  retval = 0;
	}
      else
	retval = atoi (*pp);
    }
  if (!(isspace (*p) || *p == '\0' || *p == trailer))
    {
      /* Trailing junk: return 0 and let caller print error msg.  */
      while (!(isspace (*p) || *p == '\0' || *p == trailer))
	++p;
      retval = 0;
    }
  p = skip_spaces_const (p);
  *pp = p;
  return retval;
}

/* See documentation in cli-utils.h.  */

int
get_number_const (const char **pp)
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

number_or_range_parser::number_or_range_parser (const char *string)
{
  init (string);
}

/* See documentation in cli-utils.h.  */

void
number_or_range_parser::init (const char *string)
{
  m_finished = false;
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
      if (*m_cur_tok == '-')
	{
	  const char **temp;

	  /* This is the start of a range (<number1> - <number2>).
	     Skip the '-', parse and remember the second number,
	     and also remember the end of the final token.  */

	  temp = &m_end_ptr;
	  m_end_ptr = skip_spaces_const (m_cur_tok + 1);
	  m_end_value = get_number_const (temp);
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
    error (_("negative value"));
  m_finished = *m_cur_tok == '\0';
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
  while (!parser.finished ())
    {
      int gotnum = parser.get_number ();

      if (gotnum == 0)
	error (_("Args must be numbers or '$' variables."));
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

/* See documentation in cli-utils.h.  */

char *
extract_arg_const (const char **arg)
{
  const char *result;

  if (!*arg)
    return NULL;

  /* Find the start of the argument.  */
  *arg = skip_spaces_const (*arg);
  if (!**arg)
    return NULL;
  result = *arg;

  /* Find the end of the argument.  */
  *arg = skip_to_space_const (*arg + 1);

  if (result == *arg)
    return NULL;

  return savestring (result, *arg - result);
}

/* See documentation in cli-utils.h.  */

char *
extract_arg (char **arg)
{
  const char *arg_const = *arg;
  char *result;

  result = extract_arg_const (&arg_const);
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
