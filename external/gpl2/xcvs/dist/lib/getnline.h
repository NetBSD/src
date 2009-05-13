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

#ifndef GETNLINE_H
#define GETNLINE_H 1

#include <stdio.h>
#include <sys/types.h>

#define GETNLINE_NO_LIMIT ((size_t) -1)

/* Read a line, up to the next newline, from STREAM, and store it in *LINEPTR.
   *LINEPTR is a pointer returned from malloc (or NULL), pointing to *LINESIZE
   bytes of space.  It is realloc'd as necessary.  Reallocation is limited to
   NMAX bytes; if the line is longer than that, the extra bytes are read but
   thrown away.
   Return the number of bytes read and stored at *LINEPTR (not including the
   NUL terminator), or -1 on error or EOF.  */
extern ssize_t getnline (char **lineptr, size_t *linesize, size_t nmax,
			 FILE *stream);

/* Read a line, up to the next occurrence of DELIMITER, from STREAM, and store
   it in *LINEPTR.
   *LINEPTR is a pointer returned from malloc (or NULL), pointing to *LINESIZE
   bytes of space.  It is realloc'd as necessary.  Reallocation is limited to
   NMAX bytes; if the line is longer than that, the extra bytes are read but
   thrown away.
   Return the number of bytes read and stored at *LINEPTR (not including the
   NUL terminator), or -1 on error or EOF.  */
extern ssize_t getndelim (char **lineptr, size_t *linesize, size_t nmax,
			  int delimiter, FILE *stream);

#endif /* GETNLINE_H */
