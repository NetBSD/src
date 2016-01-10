/*	$NetBSD: hard-locale.c,v 1.2 2016/01/10 22:16:40 christos Exp $	*/

/* hard-locale.c -- Determine whether a locale is hard.
   Copyright 1997, 1998, 1999 Free Software Foundation, Inc.

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

#ifndef __GNUC__
# ifdef HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #  pragma alloca
#  else
#   ifdef _WIN32
#    include <malloc.h>
#    include <io.h>
#   else
#    ifndef alloca
char *alloca ();
#    endif
#   endif
#  endif
# endif
#endif

#if HAVE_LOCALE_H
# include <locale.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif

/* Return nonzero if the current CATEGORY locale is hard, i.e. if you
   can't get away with assuming traditional C or POSIX behavior.  */
int
hard_locale (int category)
{
#if ! (defined ENABLE_NLS && HAVE_SETLOCALE)
  return 0;
#else

  int hard = 1;
  char const *p = setlocale (category, 0);

  if (p)
    {
# if defined __GLIBC__ && __GLIBC__ >= 2
      if (strcmp (p, "C") == 0 || strcmp (p, "POSIX") == 0)
	hard = 0;
# else
      char *locale = alloca (strlen (p) + 1);
      strcpy (locale, p);

      /* Temporarily set the locale to the "C" and "POSIX" locales to
	 find their names, so that we can determine whether one or the
	 other is the caller's locale.  */
      if (((p = setlocale (category, "C")) && strcmp (p, locale) == 0)
	  || ((p = setlocale (category, "POSIX")) && strcmp (p, locale) == 0))
	hard = 0;

      /* Restore the caller's locale.  */
      setlocale (category, locale);
# endif
    }

  return hard;

#endif
}
