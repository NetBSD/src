/* open-po - search for .po file along search path list and open for reading
   Copyright (C) 1995, 1996 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, April 1995.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#if defined STDC_HEADERS || HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#include "dir-list.h"
#include "error.h"
#include "system.h"

#include <libintl.h>

#define _(str) gettext (str)

#ifndef errno
extern int errno;
#endif

/* Prototypes for helper functions.  */
extern char *xstrdup PARAMS ((const char *string));

/* This macro is used to determine the number of elements in an erray.  */
#define SIZEOF(a) (sizeof(a)/sizeof(a[0]))

/* Open the input file with the name INPUT_NAME.  The ending .po is added
   if necessary.  If INPUT_NAME is not an absolute file name and the file is
   not found, the list of directories in INPUT_PATH_LIST is searched.  */
FILE *
open_po_file (input_name, file_name)
     const char *input_name;
     char **file_name;
{
  static const char *extension[] = { "", ".po", ".pot", };
  FILE *ret_val;
  int j, k;
  const char *dir;
  const char *ext;

  if (strcmp (input_name, "-") == 0 || strcmp (input_name, "/dev/stdin") == 0)
    {
      *file_name = xstrdup (_("<stdin>"));
      return stdin;
    }

  /* We have a real name for the input file.  If the name is absolute,
     try the various extensions, but ignore the directory search list.  */
  if (*input_name == '/')
    {
      for (k = 0; k < SIZEOF (extension); ++k)
	{
	  ext = extension[k];
	  *file_name = xmalloc (strlen (input_name) + strlen (ext) + 1);
	  stpcpy (stpcpy (*file_name, input_name), ext);

	  ret_val = fopen (*file_name, "r");
	  if (ret_val != NULL || errno != ENOENT)
	    /* We found the file.  */
	    return ret_val;

	  free (*file_name);
	}

      /* File does not exist.  */
      *file_name = xstrdup (input_name);
      errno = ENOENT;
      return NULL;
    }

  /* For relative file names, look through the directory search list,
     trying the various extensions.  If no directory search list is
     specified, the current directory is used.  */
  for (j = 0; (dir = dir_list_nth (j)) != NULL; ++j)
    for (k = 0; k < SIZEOF (extension); ++k)
      {
	ext = extension[k];
	if (dir[0] == '.' && dir[1] == '\0')
	  {
	    *file_name = xmalloc (strlen(input_name) + strlen(ext) + 1);
	    stpcpy (stpcpy (*file_name, input_name), ext);
	  }
	else
	  {
	    *file_name = xmalloc (strlen (dir) + strlen (input_name)
				  + strlen (ext) + 2);
	    stpcpy (stpcpy (stpcpy (stpcpy (*file_name, dir), "/"),
			    input_name),
		    ext);
	  }

	ret_val = fopen (*file_name, "r");
	if (ret_val != NULL || errno != ENOENT)
	  return ret_val;

	free (*file_name);
      }

  /* File does not exist.  */
  *file_name = xstrdup (input_name);
  errno = ENOENT;
  return NULL;
}
