/* open_datafile.c -- Set up datafile context.

   Usage: Before including this template file in your source file,
   #define the prototype of the function under test in the CALL
   symbol, see tadd_tmpl.c for an example.

Copyright (C) 2012, 2013 INRIA

This file is part of GNU MPC.

GNU MPC is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

GNU MPC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this program. If not, see http://www.gnu.org/licenses/ .
*/

#include <string.h>
#include "mpc-tests.h"

void
open_datafile (mpc_datafile_context_t* datafile_context,
               const char * data_filename)
{
  char *src_dir;
  char default_srcdir[] = ".";

  src_dir = getenv ("srcdir");
  if (src_dir == NULL)
    src_dir = default_srcdir;

  datafile_context->pathname =
    (char *) malloc ((strlen (src_dir)) + strlen (data_filename) + 2);
  if (datafile_context->pathname == NULL)
    {
      fprintf (stderr, "Cannot allocate memory\n");
      exit (1);
    }
  sprintf (datafile_context->pathname, "%s/%s", src_dir, data_filename);
  datafile_context->fd = fopen (datafile_context->pathname, "r");
  if (datafile_context->fd == NULL)
    {
      fprintf (stderr, "Unable to open %s\n", datafile_context->pathname);
      exit (1);
    }

  datafile_context->line_number = 1;
  datafile_context->nextchar = getc (datafile_context->fd);
  tpl_skip_whitespace_comments (datafile_context);
}
