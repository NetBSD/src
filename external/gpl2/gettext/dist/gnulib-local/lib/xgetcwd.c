/* xgetcwd.c -- return current directory with unlimited length
   Copyright (C) 1992, 1996, 2000, 2003, 2005-2006 Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu>.  */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "pathmax.h"

#if HAVE_GETCWD
# ifdef VMS
   /* We want the directory in Unix syntax, not in VMS syntax.  */
#  define getcwd(Buf, Max) (getcwd) (Buf, Max, 0)
# else
char *getcwd ();
# endif
#else
char *getwd ();
# define getcwd(Buf, Max) getwd (Buf)
#endif

#include "xalloc.h"

/* Return the current directory, newly allocated, arbitrarily long.
   Return NULL and set errno on error. */

char *
xgetcwd ()
{
  char *ret;
  unsigned path_max;
  char buf[1024];

  errno = 0;
  ret = getcwd (buf, sizeof (buf));
  if (ret != NULL)
    return xstrdup (buf);
  if (errno != ERANGE)
    return NULL;

  path_max = (unsigned) PATH_MAX;
  path_max += 2;		/* The getcwd docs say to do this. */

  for (;;)
    {
      char *cwd = (char *) xmalloc (path_max);

      errno = 0;
      ret = getcwd (cwd, path_max);
      if (ret != NULL)
	return ret;
      if (errno != ERANGE)
	{
	  int save_errno = errno;
	  free (cwd);
	  errno = save_errno;
	  return NULL;
	}

      free (cwd);

      path_max += path_max / 16;
      path_max += 32;
    }
}
