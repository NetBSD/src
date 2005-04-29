/* c-strncasecmp.c -- case insensitive string comparator in C locale
   Copyright (C) 1998, 1999, 2005 Free Software Foundation, Inc.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "c-strcase.h"

#include "c-ctype.h"

int
c_strncasecmp (const char *s1, const char *s2, size_t n)
{
  register const unsigned char *p1 = (const unsigned char *) s1;
  register const unsigned char *p2 = (const unsigned char *) s2;
  unsigned char c1, c2;

  if (p1 == p2 || n == 0)
    return 0;

  do
    {
      c1 = c_tolower (*p1);
      c2 = c_tolower (*p2);

      if (--n == 0 || c1 == '\0')
	break;

      ++p1;
      ++p2;
    }
  while (c1 == c2);

  return c1 - c2;
}
