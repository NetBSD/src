/* Conversion UTF-8 to UCS-4.
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
u8_mbtouc_aux (unsigned int *puc, const unsigned char *s, size_t n)
{
  unsigned char c = *s;

  if (c >= 0xc2)
    {
      if (c < 0xe0)
        {
          if (n >= 2)
            {
              if ((s[1] ^ 0x80) < 0x40)
                {
                  *puc = ((unsigned int) (c & 0x1f) << 6)
                         | (unsigned int) (s[1] ^ 0x80);
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
      else if (c < 0xf0)
        {
          if (n >= 3)
            {
              if ((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
                  && (c >= 0xe1 || s[1] >= 0xa0))
                {
                  *puc = ((unsigned int) (c & 0x0f) << 12)
                         | ((unsigned int) (s[1] ^ 0x80) << 6)
                         | (unsigned int) (s[2] ^ 0x80);
                  return 3;
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
      else if (c < 0xf8)
        {
          if (n >= 4)
            {
              if ((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
                  && (s[3] ^ 0x80) < 0x40
                  && (c >= 0xf1 || s[1] >= 0x90)
#if 1
                  && (c < 0xf4 || (c == 0xf4 && s[1] < 0x90))
#endif
                 )
                {
                  *puc = ((unsigned int) (c & 0x07) << 18)
                         | ((unsigned int) (s[1] ^ 0x80) << 12)
                         | ((unsigned int) (s[2] ^ 0x80) << 6)
                         | (unsigned int) (s[3] ^ 0x80);
                  return 4;
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
#if 0
      else if (c < 0xfc)
        {
          if (n >= 5)
            {
              if ((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
                  && (s[3] ^ 0x80) < 0x40 && (s[4] ^ 0x80) < 0x40
                  && (c >= 0xf9 || s[1] >= 0x88))
                {
                  *puc = ((unsigned int) (c & 0x03) << 24)
                         | ((unsigned int) (s[1] ^ 0x80) << 18)
                         | ((unsigned int) (s[2] ^ 0x80) << 12)
                         | ((unsigned int) (s[3] ^ 0x80) << 6)
                         | (unsigned int) (s[4] ^ 0x80);
                  return 5;
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
      else if (c < 0xfe)
        {
          if (n >= 6)
            {
              if ((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
                  && (s[3] ^ 0x80) < 0x40 && (s[4] ^ 0x80) < 0x40
                  && (s[5] ^ 0x80) < 0x40
                  && (c >= 0xfd || s[1] >= 0x84))
                {
                  *puc = ((unsigned int) (c & 0x01) << 30)
                         | ((unsigned int) (s[1] ^ 0x80) << 24)
                         | ((unsigned int) (s[2] ^ 0x80) << 18)
                         | ((unsigned int) (s[3] ^ 0x80) << 12)
                         | ((unsigned int) (s[4] ^ 0x80) << 6)
                         | (unsigned int) (s[5] ^ 0x80);
                  return 6;
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
#endif
    }
  /* invalid multibyte character */
  *puc = 0xfffd;
  return 1;
}
static inline int
u8_mbtouc (unsigned int *puc, const unsigned char *s, size_t n)
{
  unsigned char c = *s;

  if (c < 0x80)
    {
      *puc = c;
      return 1;
    }
  else
    return u8_mbtouc_aux (puc, s, n);
}
