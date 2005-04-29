/* Conversion UTF-16 to UCS-4.
   Copyright (C) 2001-2002 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

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

/* Return the length (number of units) of the first character in S, putting
   its 'ucs4_t' representation in *PUC.  */
static int
u16_mbtouc_aux (unsigned int *puc, const unsigned short *s, size_t n)
{
  unsigned short c = *s;

  if (c < 0xdc00)
    {
      if (n >= 2)
        {
          if (s[1] >= 0xdc00 && s[1] < 0xe000)
            {
              *puc = 0x10000 + ((c - 0xd800) << 10) + (s[1] - 0xdc00);
              return 2;
            }
          /* invalid multibyte character */
        }
      else
        {
          /* incomplete multibyte character */
          *puc = 0xfffd;
          return n;
        }
    }
  /* invalid multibyte character */
  *puc = 0xfffd;
  return 1;
}
static inline int
u16_mbtouc (unsigned int *puc, const unsigned short *s, size_t n)
{
  unsigned short c = *s;

  if (c < 0xd800 || c >= 0xe000)
    {
      *puc = c;
      return 1;
    }
  else
    return u16_mbtouc_aux (puc, s, n);
}
