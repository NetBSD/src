/* Shared general utility routines for GDB, the GNU debugger.

   Copyright (C) 1986-2013 Free Software Foundation, Inc.

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

#ifdef GDBSERVER
#include "server.h"
#else
#include "defs.h"
#endif

#include "gdb_assert.h"
#include "gdb_string.h"

#include <stdlib.h>
#include <stdio.h>

/* The xmalloc() (libiberty.h) family of memory management routines.

   These are like the ISO-C malloc() family except that they implement
   consistent semantics and guard against typical memory management
   problems.  */

/* NOTE: These are declared using PTR to ensure consistency with
   "libiberty.h".  xfree() is GDB local.  */

PTR                            /* ARI: PTR */
xmalloc (size_t size)
{
  void *val;

  /* See libiberty/xmalloc.c.  This function need's to match that's
     semantics.  It never returns NULL.  */
  if (size == 0)
    size = 1;

  val = malloc (size);         /* ARI: malloc */
  if (val == NULL)
    malloc_failure (size);

  return val;
}

PTR                              /* ARI: PTR */
xrealloc (PTR ptr, size_t size)          /* ARI: PTR */
{
  void *val;

  /* See libiberty/xmalloc.c.  This function need's to match that's
     semantics.  It never returns NULL.  */
  if (size == 0)
    size = 1;

  if (ptr != NULL)
    val = realloc (ptr, size);	/* ARI: realloc */
  else
    val = malloc (size);	        /* ARI: malloc */
  if (val == NULL)
    malloc_failure (size);

  return val;
}

PTR                            /* ARI: PTR */           
xcalloc (size_t number, size_t size)
{
  void *mem;

  /* See libiberty/xmalloc.c.  This function need's to match that's
     semantics.  It never returns NULL.  */
  if (number == 0 || size == 0)
    {
      number = 1;
      size = 1;
    }

  mem = calloc (number, size);      /* ARI: xcalloc */
  if (mem == NULL)
    malloc_failure (number * size);

  return mem;
}

void *
xzalloc (size_t size)
{
  return xcalloc (1, size);
}

void
xfree (void *ptr)
{
  if (ptr != NULL)
    free (ptr);		/* ARI: free */
}

/* Like asprintf/vasprintf but get an internal_error if the call
   fails. */

char *
xstrprintf (const char *format, ...)
{
  char *ret;
  va_list args;

  va_start (args, format);
  ret = xstrvprintf (format, args);
  va_end (args);
  return ret;
}

char *
xstrvprintf (const char *format, va_list ap)
{
  char *ret = NULL;
  int status = vasprintf (&ret, format, ap);

  /* NULL is returned when there was a memory allocation problem, or
     any other error (for instance, a bad format string).  A negative
     status (the printed length) with a non-NULL buffer should never
     happen, but just to be sure.  */
  if (ret == NULL || status < 0)
    internal_error (__FILE__, __LINE__, _("vasprintf call failed"));
  return ret;
}

int
xsnprintf (char *str, size_t size, const char *format, ...)
{
  va_list args;
  int ret;

  va_start (args, format);
  ret = vsnprintf (str, size, format, args);
  gdb_assert (ret < size);
  va_end (args);

  return ret;
}

char *
savestring (const char *ptr, size_t len)
{
  char *p = (char *) xmalloc (len + 1);

  memcpy (p, ptr, len);
  p[len] = 0;
  return p;
}
