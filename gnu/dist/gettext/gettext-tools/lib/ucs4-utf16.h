/* Conversion UCS-4 to UTF-16.
   Copyright (C) 2002 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2002.

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


#include <stddef.h>

/* Return the length (number of units) of the UTF-16 representation of uc,
   after storing it at S.  Return -1 upon failure, -2 if the number of
   available units, N, is too small.  */
static int
u16_uctomb_aux (unsigned short *s, unsigned int uc, int n)
{
  if (uc >= 0x10000)
    {
      if (uc < 0x110000)
	{
	  if (n >= 2)
	    {
	      s[0] = 0xd800 + ((uc - 0x10000) >> 10);
	      s[1] = 0xdc00 + ((uc - 0x10000) & 0x3ff);
	      return 2;
	    }
	}
      else
	return -1;
    }
  return -2;
}

static inline int
u16_uctomb (unsigned short *s, unsigned int uc, int n)
{
  if (uc < 0x10000 && n > 0)
    {
      s[0] = uc;
      return 1;
    }
  else
    return u16_uctomb_aux (s, uc, n);
}
