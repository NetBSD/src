/* Construct a full pathname from a directory and a filename.
   Copyright (C) 2001-2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

/* Written by Bruno Haible <haible@clisp.cons.org>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "pathname.h"

#include <string.h>

#include "xalloc.h"
#include "stpcpy.h"

/* Concatenate a directory pathname, a relative pathname and an optional
   suffix.  The directory may end with the directory separator.  The second
   argument may not start with the directory separator (it is relative).
   Return a freshly allocated pathname.  */
char *
concatenated_pathname (const char *directory, const char *filename,
		       const char *suffix)
{
  char *result;
  char *p;

  if (strcmp (directory, ".") == 0)
    {
      /* No need to prepend the directory.  */
      result = (char *) xmalloc (strlen (filename)
				 + (suffix != NULL ? strlen (suffix) : 0)
				 + 1);
      p = result;
    }
  else
    {
      size_t directory_len = strlen (directory);
      int need_slash =
	(directory_len > FILE_SYSTEM_PREFIX_LEN (directory)
	 && !ISSLASH (directory[directory_len - 1]));
      result = (char *) xmalloc (directory_len + need_slash
				 + strlen (filename)
				 + (suffix != NULL ? strlen (suffix) : 0)
				 + 1);
      memcpy (result, directory, directory_len);
      p = result + directory_len;
      if (need_slash)
	*p++ = '/';
    }
  p = stpcpy (p, filename);
  if (suffix != NULL)
    stpcpy (p, suffix);
  return result;
}
