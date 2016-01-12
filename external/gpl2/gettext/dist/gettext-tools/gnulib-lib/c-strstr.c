/* Copyright (C) 1994, 1999, 2002-2003, 2005-2006 Free Software Foundation, Inc.
This file is part of the GNU C Library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/*
 * My personal strstr() implementation that beats most other algorithms.
 * Until someone tells me otherwise, I assume that this is the
 * fastest implementation of strstr() in C.
 * I deliberately chose not to comment it.  You should have at least
 * as much fun trying to understand it, as I had to write it :-).
 *
 * Stephen R. van den Berg, berg@pool.informatik.rwth-aachen.de	*/

#include <config.h>

/* Specification.  */
#include "c-strstr.h"

#include <string.h>

typedef unsigned chartype;

char *
c_strstr (const char *phaystack, const char *pneedle)
{
  register const unsigned char *haystack, *needle;
  register chartype b, c;

  haystack = (const unsigned char *) phaystack;
  needle = (const unsigned char *) pneedle;

  b = *needle;
  if (b != '\0')
    {
      haystack--;				/* possible ANSI violation */
      do
	{
	  c = *++haystack;
	  if (c == '\0')
	    goto ret0;
	}
      while (c != b);

      c = *++needle;
      if (c == '\0')
	goto foundneedle;
      ++needle;
      goto jin;

      for (;;)
        {
          register chartype a;
	  register const unsigned char *rhaystack, *rneedle;

	  do
	    {
	      a = *++haystack;
	      if (a == '\0')
		goto ret0;
	      if (a == b)
		break;
	      a = *++haystack;
	      if (a == '\0')
		goto ret0;
shloop:;    }
          while (a != b);

jin:	  a = *++haystack;
	  if (a == '\0')
	    goto ret0;

	  if (a != c)
	    goto shloop;

	  rhaystack = haystack-- + 1;
	  rneedle = needle;
	  a = *rneedle;

	  if (*rhaystack == a)
	    do
	      {
		if (a == '\0')
		  goto foundneedle;
		++rhaystack;
		a = *++needle;
		if (*rhaystack != a)
		  break;
		if (a == '\0')
		  goto foundneedle;
		++rhaystack;
		a = *++needle;
	      }
	    while (*rhaystack == a);

	  needle = rneedle;		   /* took the register-poor approach */

	  if (a == '\0')
	    break;
        }
    }
foundneedle:
  return (char *) haystack;
ret0:
  return 0;
}
