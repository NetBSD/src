/* Shell quoting.
   Copyright (C) 2001-2002, 2004 Free Software Foundation, Inc.
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

/* When passing a command to a shell, we must quote the program name and
   arguments, since Unix shells interpret characters like " ", "'", "<", ">",
   "$" etc. in a special way.  */

#include <stddef.h>

/* Returns the number of bytes needed for the quoted string.  */
extern size_t shell_quote_length (const char *string);

/* Copies the quoted string to p and returns the incremented p.
   There must be room for shell_quote_length (string) + 1 bytes at p.  */
extern char * shell_quote_copy (char *p, const char *string);

/* Returns the freshly allocated quoted string.  */
extern char * shell_quote (const char *string);

/* Returns a freshly allocated string containing all argument strings, quoted,
   separated through spaces.  */
extern char * shell_quote_argv (char **argv);
