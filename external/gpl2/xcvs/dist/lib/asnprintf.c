/* Formatted output to strings.
   Copyright (C) 1999, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */
#include <sys/cdefs.h>
__RCSID("$NetBSD: asnprintf.c,v 1.2 2016/05/17 14:00:09 christos Exp $");


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "vasnprintf.h"

#include <stdarg.h>

char *
asnprintf (char *resultbuf, size_t *lengthp, const char *format, ...)
{
  va_list args;
  char *result;

  va_start (args, format);
  result = vasnprintf (resultbuf, lengthp, format, args);
  va_end (args);
  return result;
}
