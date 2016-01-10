/*	$NetBSD: strtoumax.c,v 1.1.1.1 2016/01/10 21:36:19 christos Exp $	*/

/* Convert string representation of a number into an uintmax_t value.
   Copyright 1999 Free Software Foundation, Inc.

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

/* Written by Paul Eggert. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifndef PARAMS
# if defined PROTOTYPES || defined __STDC__
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

#ifndef HAVE_DECL_STRTOUL
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_STRTOUL
unsigned long strtoul PARAMS ((char const *, char **, int));
#endif

#ifndef HAVE_DECL_STRTOULL
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_STRTOULL && HAVE_UNSIGNED_LONG_LONG
unsigned long long strtoull PARAMS ((char const *, char **, int));
#endif

uintmax_t
strtoumax (char const *ptr, char **endptr, int base)
{
#define USE_IF_EQUIVALENT(function) \
    if (sizeof (uintmax_t) == sizeof function (ptr, endptr, base)) \
      return function (ptr, endptr, base);

#if HAVE_UNSIGNED_LONG_LONG
    USE_IF_EQUIVALENT (strtoull)
#endif

  USE_IF_EQUIVALENT (strtoul)

  abort ();
}

#ifdef TESTING
# include <stdio.h>
int
main ()
{
  char *p, *endptr;
  printf ("sizeof uintmax_t: %d\n", sizeof (uintmax_t));
  printf ("sizeof strtoull(): %d\n", sizeof strtoull(p, &endptr, 10));
  printf ("sizeof strtoul(): %d\n", sizeof strtoul(p, &endptr, 10));
  exit (0);
}
#endif
