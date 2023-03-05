/* mpfr_inp_str -- input a number in base BASE from stdio stream STREAM
                   and store the result in ROP

Copyright 1999, 2001-2002, 2004, 2006-2023 Free Software Foundation, Inc.
Contributed by the AriC and Caramba projects, INRIA.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
https://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

#include <ctype.h>

#include "mpfr-impl.h"

/* The original version of this function came from GMP's mpf/inp_str.c;
   it has been adapted for MPFR. */

size_t
mpfr_inp_str (mpfr_ptr rop, FILE *stream, int base, mpfr_rnd_t rnd_mode)
{
  unsigned char *str;
  size_t alloc_size, str_size;
  int c;
  int retval;
  size_t nread;

  alloc_size = 100;
  str = (unsigned char *) mpfr_allocate_func (alloc_size);
  str_size = 0;
  nread = 0;

  /* Skip whitespace. EOF will be detected later. */
  do
    {
      c = getc (stream);
      nread++;
    }
  while (isspace (c));

  /* number of characters read is nread */

  for (;;)
    {
      MPFR_ASSERTD (str_size < (size_t) -1);
      if (str_size >= alloc_size)
        {
          size_t old_alloc_size = alloc_size;
          alloc_size = alloc_size / 2 * 3;
          /* Handle overflow in unsigned integer arithmetic... */
          if (MPFR_UNLIKELY (alloc_size <= old_alloc_size))
            alloc_size = (size_t) -1;
          MPFR_ASSERTD (str_size < alloc_size);
          str = (unsigned char *)
            mpfr_reallocate_func (str, old_alloc_size, alloc_size);
        }
      if (c == EOF || isspace (c))
        break;
      str[str_size++] = (unsigned char) c;
      if (str_size == (size_t) -1)
        break;
      c = getc (stream);
    }
  /* FIXME: The use of ungetc has been deprecated since C99 when it
     occurs at the beginning of a binary stream, and this may happen
     on /dev/null. One could add a "if (c != EOF)" test, but let's
     wait for some discussion in comp.std.c first... */
  ungetc (c, stream);

  if (MPFR_UNLIKELY (str_size == (size_t) -1 || str_size == 0 ||
                     (c == EOF && ! feof (stream))))
    {
      /* size_t overflow or I/O error */
      retval = -1;
    }
  else
    {
      /* number of characters read is nread + str_size - 1 */

      /* We exited the "for" loop by the first break instruction,
         then necessarily str_size >= alloc_size was checked, so
         now str_size < alloc_size. */
      MPFR_ASSERTD (str_size < alloc_size);

      str[str_size] = '\0';

      retval = mpfr_set_str (rop, (char *) str, base, rnd_mode);
    }

  mpfr_free_func (str, alloc_size);

  if (retval == -1)
    return 0;                   /* error */

  MPFR_ASSERTD (nread >= 1);
  str_size += nread - 1;
  if (MPFR_UNLIKELY (str_size < nread - 1))  /* size_t overflow */
    return 0;  /* however, rop has been set successfully */
  else
    return str_size;
}
