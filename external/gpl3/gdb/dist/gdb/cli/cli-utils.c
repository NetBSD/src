/* CLI utilities.

   Copyright (C) 2011-2013 Free Software Foundation, Inc.

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
#include "gdb_string.h"
#include "value.h"
#include "gdb_assert.h"

#include <ctype.h>

/* *PP is a string denoting a number.  Get the number of the.  Advance
   *PP after the string and any trailing whitespace.

   Currently the string can either be a number, or "$" followed by the
   name of a convenience variable, or ("$" or "$$") followed by digits.

   TRAILER is a character which can be found after the number; most
   commonly this is `-'.  If you don't want a trailer, use \0.  */

static int
get_number_trailer (char **pp, int trailer)
{
  int retval = 0;	/* default */
  char *p = *pp;

  if (*p == '$')
    {
      struct value *val = value_from_history_ref (p, &p);

      if (val)	/* Value history reference */
	{
	  if (TYPE_CODE (value_type (val)) == TYPE_CODE_INT)
	    retval = value_as_long (val);
	  else
	    {
	      printf_filtered (_("History value must have integer type."));
	      retval = 0;
	    }
	}
      else	/* Convenience variable */
	{
	  /* Internal variable.  Make a copy of the name, so we can
	     null-terminate it to pass to lookup_internalvar().  */
	  char *varname;
	  char *start = ++p;
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
  p = skip_spaces (p);
  *pp = p;
  return retval;
}

/* See documentation in cli-utils.h.  */

int
get_number (char **pp)
{
  return get_number_trailer (pp, '\0');
}

/* See documentation in cli-utils.h.  */

void
init_number_or_range (struct get_number_or_range_state *state,
		      char *string)
{
  memset (state, 0, sizeof (*state));
  state->string = string;
}

/* See documentation in cli-utils.h.  */

int
get_number_or_range (struct get_number_or_range_state *state)
{
  if (*state->string != '-')
    {
      /* Default case: state->string is pointing either to a solo
	 number, or to the first number of a range.  */
      state->last_retval = get_number_trailer (&state->string, '-');
      if (*state->string == '-')
	{
	  char **temp;

	  /* This is the start of a range (<number1> - <number2>).
	     Skip the '-', parse and remember the second number,
	     and also remember the end of the final token.  */

	  temp = &state->end_ptr; 
	  state->end_ptr = skip_spaces (state->string + 1);
	  state->end_value = get_number (temp);
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
  else if (! state->in_range)
    error (_("negative value"));
  else
    {
      /* state->string points to the '-' that betokens a range.  All
	 number-parsing has already been done.  Return the next
	 integer value (one greater than the saved previous value).
	 Do not advance the token pointer until the end of range
	 is reached.  */

      if (++state->last_retval == state->end_value)
	{
	  /* End of range reached; advance token pointer.  */
	  state->string = state->end_ptr;
	  state->in_range = 0;
	}
    }
  state->finished = *state->string == '\0';
  return state->last_retval;
}

/* Accept a number and a string-form list of numbers such as is 
   accepted by get_number_or_range.  Return TRUE if the number is
   in the list.

   By definition, an empty list includes all numbers.  This is to 
   be interpreted as typing a command such as "delete break" with 
   no arguments.  */

int
number_is_in_list (char *list, int number)
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
skip_spaces (char *chp)
{
  if (chp == NULL)
    return NULL;
  while (*chp && isspace (*chp))
    chp++;
  return chp;
}

/* A const-correct version of the above.  */

const char *
skip_spaces_const (const char *chp)
{
  if (chp == NULL)
    return NULL;
  while (*chp && isspace (*chp))
    chp++;
  return chp;
}

/* See documentation in cli-utils.h.  */

const char *
skip_to_space_const (const char *chp)
{
  if (chp == NULL)
    return NULL;
  while (*chp && !isspace (*chp))
    chp++;
  return chp;
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
extract_arg (char **arg)
{
  char *result, *copy;

  if (!*arg)
    return NULL;

  /* Find the start of the argument.  */
  *arg = skip_spaces (*arg);
  if (!**arg)
    return NULL;
  result = *arg;

  /* Find the end of the argument.  */
  *arg = skip_to_space (*arg + 1);

  if (result == *arg)
    return NULL;

  copy = xmalloc (*arg - result + 1);
  memcpy (copy, result, *arg - result);
  copy[*arg - result] = '\0';

  return copy;
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
