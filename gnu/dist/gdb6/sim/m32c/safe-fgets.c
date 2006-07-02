/* safe-fgets.c --- like fgets, but allocates its own static buffer.

Copyright (C) 2005 Free Software Foundation, Inc.
Contributed by Red Hat, Inc.

This file is part of the GNU simulators.

The GNU simulators are free software; you can redistribute them and/or
modify them under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU simulators are distributed in the hope that they will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with the GNU simulators; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA  */


#include <stdio.h>
#include <stdlib.h>

#include "safe-fgets.h"

static char *line_buf = 0;
static int line_buf_size = 0;

#define LBUFINCR 100

char *
safe_fgets (FILE *f)
{
  char *line_ptr;

  if (line_buf == 0)
    {
      line_buf = (char *) malloc (LBUFINCR);
      line_buf_size = LBUFINCR;
    }

  /* points to last byte */
  line_ptr = line_buf + line_buf_size - 1;

  /* so we can see if fgets put a 0 there */
  *line_ptr = 1;
  if (fgets (line_buf, line_buf_size, f) == 0)
    return 0;

  /* we filled the buffer? */
  while (line_ptr[0] == 0 && line_ptr[-1] != '\n')
    {
      /* Make the buffer bigger and read more of the line */
      line_buf_size += LBUFINCR;
      line_buf = (char *) realloc (line_buf, line_buf_size);

      /* points to last byte again */
      line_ptr = line_buf + line_buf_size - 1;
      /* so we can see if fgets put a 0 there */
      *line_ptr = 1;

      if (fgets (line_buf + line_buf_size - LBUFINCR - 1, LBUFINCR + 1, f) ==
	  0)
	return 0;
    }

  return line_buf;
}
