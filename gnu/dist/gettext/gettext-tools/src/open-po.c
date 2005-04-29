/* open-po - search for .po file along search path list and open for reading
   Copyright (C) 1995-1996, 2000-2003 Free Software Foundation, Inc.
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
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "open-po.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dir-list.h"
#include "pathname.h"
#include "xalloc.h"
#include "error.h"
#include "gettext.h"

#define _(str) gettext (str)

/* This macro is used to determine the number of elements in an erray.  */
#define SIZEOF(a) (sizeof(a)/sizeof(a[0]))

static FILE *
try_open_po_file (const char *input_name, char **real_file_name_p)
{
  static const char *extension[] = { "", ".po", ".pot", };
  char *file_name;
  FILE *ret_val;
  int j;
  size_t k;
  const char *dir;

  if (strcmp (input_name, "-") == 0 || strcmp (input_name, "/dev/stdin") == 0)
    {
      *real_file_name_p = xstrdup (_("<stdin>"));
      return stdin;
    }

  /* We have a real name for the input file.  If the name is absolute,
     try the various extensions, but ignore the directory search list.  */
  if (IS_ABSOLUTE_PATH (input_name))
    {
      for (k = 0; k < SIZEOF (extension); ++k)
	{
	  file_name = concatenated_pathname ("", input_name, extension[k]);

	  ret_val = fopen (file_name, "r");
	  if (ret_val != NULL || errno != ENOENT)
	    {
	      /* We found the file.  */
	      *real_file_name_p = file_name;
	      return ret_val;
	    }

	  free (file_name);
	}
    }
  else
    {
      /* For relative file names, look through the directory search list,
	 trying the various extensions.  If no directory search list is
	 specified, the current directory is used.  */
      for (j = 0; (dir = dir_list_nth (j)) != NULL; ++j)
	for (k = 0; k < SIZEOF (extension); ++k)
	  {
	    file_name = concatenated_pathname (dir, input_name, extension[k]);

	    ret_val = fopen (file_name, "r");
	    if (ret_val != NULL || errno != ENOENT)
	      {
		*real_file_name_p = file_name;
		return ret_val;
	      }

	    free (file_name);
	  }
    }

  /* File does not exist.  */
  *real_file_name_p = xstrdup (input_name);
  errno = ENOENT;
  return NULL;
}

/* Open the input file with the name INPUT_NAME.  The ending .po is added
   if necessary.  If INPUT_NAME is not an absolute file name and the file is
   not found, the list of directories in "dir-list.h" is searched.  The
   file's pathname is returned in *REAL_FILE_NAME_P, for error message
   purposes.  */
FILE *
open_po_file (const char *input_name, char **real_file_name_p,
	      bool exit_on_error)
{
  FILE *fp = try_open_po_file (input_name, real_file_name_p);

  if (fp == NULL && exit_on_error)
    error (EXIT_FAILURE, errno,
	   _("error while opening \"%s\" for reading"), *real_file_name_p);

  return fp;
}
