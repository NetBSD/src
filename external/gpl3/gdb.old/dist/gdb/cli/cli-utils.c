/* CLI utilities.

   Copyright (C) 2011-2016 Free Software Foundation, Inc.

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

void
init_number_or_range (struct get_number_or_range_state *state,
		      const char *string)
{
  memset (state, 0, sizeof (*state));
  state->string = string;
}

/* See documentation in cli-utils.h.  */

int
get_number_or_range (struct get_number_or_range_state *state)
{
  if (state->in_range)
    {
      /* All number-parsing has already been done.  Return the next
	 integer value (one greater than the saved previous value).
	 Do not advance the token pointer until the end of range is
	 reached.  */

      if (++state->last_retval == state->end_value)
	{
	  /* End of range reached; advance token pointer.  */
	  state->string = state->end_ptr;
	  state->in_range = 0;
	}
    }
  else if (*state->string != '-')
    {
      /* Default case: state->string is pointing either to a solo
	 number, or to the first number of a range.  */
      state->last_retval = get_number_trailer (&state->string, '-');
      if (*state->string == '-')
	{
	  const char **temp;

	  /* This is the start of a range (<number1> - <number2>).
	     Skip the '-', parse and remember the second number,
	     and also remember the end of the final token.  */

	  temp = &state->end_ptr; 
	  state->end_ptr = skip_spaces_const (state->string + 1);
	  state->end_value = get_number_const (temp);
	  if (state->end_value < state->last_retval) 
	    {
	      error (_("inverted range"));
	    }
	  else if (state->end_value == state->last_retval)
	    {
	      /* Degenerate range (number1 == number2).  Advance the
		 token pointer so that the range will be treated as a
		 single number.  */ 
	      state->string = state->end_ptr;
	    }
	  else
	    state->in_range = 1;
	}
    }
  else
    error (_("negative value"));
  state->finished = *state->string == '\0';
  return state->last_retval;
}

/* See documentation in cli-utils.h.  */

void
number_range_setup_range (struct get_number_or_range_state *state,
			  int start_value, int end_value, const char *end_ptr)
{
  gdb_assert (start_value > 0);

  state->in_range = 1;
  state->end_ptr = end_ptr;
  state->last_retval = start_value - 1;
  state->end_value = end_value;
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
  struct get_number_or_range_state state;

  if (list == NULL || *list == '\0')
    return 1;

  init_number_or_range (&state, list);
  while (!state.finished)
    {
      int gotnum = get_number_or_range (&state);

      if (gotnum == 0)
	error (_("Args must be numbers or '$' variables."));
      if (gotnum == number)
	return 1;
    }
  return 0;
}

/* See documentation in cli-utils.h.  */

char *
remove_trailing_whitespace (const char *start, char *s)
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
check_for_argument (char **str, char *arg, int arg_len)
{
  if (strncmp (*str, arg, arg_len) == 0
      && ((*str)[arg_len] == '\0' || isspace ((*str)[arg_len])))
    {
      *str += arg_len;
      return 1;
    }
  return 0;
}
