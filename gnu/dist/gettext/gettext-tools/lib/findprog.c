/* Locating a program in PATH.
   Copyright (C) 2001-2004 Free Software Foundation, Inc.
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
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Specification.  */
#include "findprog.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "xalloc.h"
#include "pathname.h"


const char *
find_in_path (const char *progname)
{
#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__ || defined __EMX__ || defined __DJGPP__
  /* Win32, Cygwin, OS/2, DOS */
  /* The searching rules with .COM, .EXE, .BAT, .CMD etc. suffixes are
     too complicated.  Leave it to the OS.  */
  return progname;
#else
  /* Unix */
  char *path;
  char *dir;
  char *cp;

  if (strchr (progname, '/') != NULL)
    /* If progname contains a slash, it is either absolute or relative to
       the current directory.  PATH is not used.  */
    return progname;

  path = getenv ("PATH");
  if (path == NULL || *path == '\0')
    /* If PATH is not set, the default search path is implementation
       dependent.  */
    return progname;

  /* Make a copy, to prepare for destructive modifications.  */
  path = xstrdup (path);
  for (dir = path; ; dir = cp + 1)
    {
      bool last;
      char *progpathname;

      /* Extract next directory in PATH.  */
      for (cp = dir; *cp != '\0' && *cp != ':'; cp++)
	;
      last = (*cp == '\0');
      *cp = '\0';

      /* Empty PATH components designate the current directory.  */
      if (dir == cp)
	dir = ".";

      /* Concatenate dir and progname.  */
      progpathname = concatenated_pathname (dir, progname, NULL);

      /* On systems which have the eaccess() system call, let's use it.
	 On other systems, let's hope that this program is not installed
	 setuid or setgid, so that it is ok to call access() despite its
	 design flaw.  */
      if (eaccess (progpathname, X_OK) == 0)
	{
	  /* Found!  */
	  if (strcmp (progpathname, progname) == 0)
	    {
	      free (progpathname);

	      /* Add the "./" prefix for real, that concatenated_pathname()
		 optimized away.  This avoids a second PATH search when the
		 caller uses execlp/execvp.  */
	      progpathname = xmalloc (2 + strlen (progname) + 1);
	      progpathname[0] = '.';
	      progpathname[1] = '/';
	      memcpy (progpathname + 2, progname, strlen (progname) + 1);
	    }

	  free (path);
	  return progpathname;
	}

      free (progpathname);

      if (last)
	break;
    }

  /* Not found in PATH.  An error will be signalled at the first call.  */
  free (path);
  return progname;
#endif
}
