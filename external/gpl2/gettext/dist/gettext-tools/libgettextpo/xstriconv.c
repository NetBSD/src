/* Charset conversion with out-of-memory checking.
   Copyright (C) 2001-2004, 2006 Free Software Foundation, Inc.
   Written by Bruno Haible.

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

#include <config.h>

/* Specification.  */
#include "xstriconv.h"

#include <errno.h>

#include "striconv.h"
#include "xalloc.h"


#if HAVE_ICONV

int
xmem_cd_iconv (const char *src, size_t srclen, iconv_t cd,
	       char **resultp, size_t *lengthp)
{
  int retval = mem_cd_iconv (src, srclen, cd, resultp, lengthp);

  if (retval < 0 && errno == ENOMEM)
    xalloc_die ();
  return retval;
}

char *
xstr_cd_iconv (const char *src, iconv_t cd)
{
  char *result = str_cd_iconv (src, cd);

  if (result == NULL && errno == ENOMEM)
    xalloc_die ();
  return result;
}

#endif

char *
xstr_iconv (const char *src, const char *from_codeset, const char *to_codeset)
{
  char *result = str_iconv (src, from_codeset, to_codeset);

  if (result == NULL && errno == ENOMEM)
    xalloc_die ();
  return result;
}
