/* getnline - Read a line from a stream, with bounded memory allocation.

   Copyright (C) 2003, 2004 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "getnline.h"

#include "getndelim2.h"

ssize_t
getndelim (char **lineptr, size_t *linesize, size_t nmax,
	   int delimiter, FILE *stream)
{
  return getndelim2 (lineptr, linesize, 0, nmax, delimiter, EOF, stream);
}

ssize_t
getnline (char **lineptr, size_t *linesize, size_t nmax, FILE *stream)
{
  return getndelim (lineptr, linesize, nmax, '\n', stream);
}
