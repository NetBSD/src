/* Like vsprintf but provides a pointer to malloc'd storage, which must
   be freed by the caller.
   Copyright (C) 1994, 1998, 1999, 2000-2003 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "vasprintf.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#ifdef TEST
size_t global_total_width;
#endif

static int
int_vasprintf (char **result, const char *format, va_list *args)
{
  const char *p = format;
  /* Add one to make sure that it is never zero, which might cause malloc
     to return NULL.  */
  size_t total_width = strlen (format) + 1;
  va_list ap = *args;

  while (*p != '\0')
    {
      if (*p++ == '%')
	{
	  while (strchr ("-+ #0", *p))
	    ++p;
	  if (*p == '*')
	    {
	      ++p;
	      total_width += abs (va_arg (ap, int));
	    }
	  else
	    total_width += strtoul (p, (char **) &p, 10);
	  if (*p == '.')
	    {
	      ++p;
	      if (*p == '*')
		{
		  ++p;
		  total_width += abs (va_arg (ap, int));
		}
	      else
		total_width += strtoul (p, (char **) &p, 10);
	    }
	  while (strchr ("hlLjtz", *p))
	    ++p;
	  /* Should be big enough for any format specifier except %s
	     and floats.  */
	  total_width += 30;
	  switch (*p)
	    {
	    case 'd':
	    case 'i':
	    case 'o':
	    case 'u':
	    case 'x':
	    case 'X':
	    case 'c':
	      (void) va_arg (ap, int);
	      break;
	    case 'f':
	    case 'F':
	      {
		double arg = va_arg (ap, double);
		if (arg >= 1.0 || arg <= -1.0)
		  /* Since an ieee double can have an exponent of 307, we'll
		     make the buffer wide enough to cover the gross case. */
		  total_width += 307;
	      }
	      break;
	    case 'e':
	    case 'E':
	    case 'g':
	    case 'G':
	      (void) va_arg (ap, double);
	      break;
	    case 's':
	      total_width += strlen (va_arg (ap, char *));
	      break;
	    case 'p':
	    case 'n':
	      (void) va_arg (ap, char *);
	      break;
	    }
	  p++;
	}
    }
#ifdef TEST
  global_total_width = total_width;
#endif
  *result = malloc (total_width);
  if (*result != NULL)
    return vsprintf (*result, format, *args);
  else
    return -1;
}

int
vasprintf (char **result, const char *format, va_list args)
{
  return int_vasprintf (result, format, &args);
}

int
asprintf (char **result, const char *format, ...)
{
  va_list args;
  int done;

  va_start (args, format);
  done = vasprintf (result, format, args);
  va_end (args);

  return done;
} 

/* ========================= test program ========================= */

#ifdef TEST

#include <float.h>

void
checkit (const char* format, ...)
{
  va_list args;
  char *result;

  va_start (args, format);
  vasprintf (&result, format, args);
  if (strlen (result) < global_total_width)
    printf ("PASS: ");
  else
    printf ("FAIL: ");
  printf ("%lu %s\n", (unsigned long) global_total_width, result);
}

int
main ()
{
  checkit ("%d", 0x12345678);
  checkit ("%200d", 5);
  checkit ("%.300d", 6);
  checkit ("%100.150d", 7);
  checkit ("%s", "jjjjjjjjjiiiiiiiiiiiiiiioooooooooooooooooppppppppppppaa\n\
777777777777777777333333333333366666666666622222222222777777777777733333");
  checkit ("%f%s%d%s", 1.0, "foo", 77, "asdjffffffffffffffiiiiiiiiiiixxxxx");
  checkit ("%e", DBL_MIN);
  checkit ("%e", DBL_MAX);
  checkit ("%.400f", DBL_MIN);
  checkit ("%f", DBL_MAX);
  checkit ("%g", DBL_MIN);
  checkit ("%g", DBL_MAX);
  return 0;
}

#endif /* TEST */
