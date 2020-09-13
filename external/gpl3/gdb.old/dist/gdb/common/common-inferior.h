/* Functions to deal with the inferior being executed on GDB or
   GDBserver.

   Copyright (C) 1986-2019 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef COMMON_COMMON_INFERIOR_H
#define COMMON_COMMON_INFERIOR_H

/* Return the exec wrapper to be used when starting the inferior, or NULL
   otherwise.  */
extern const char *get_exec_wrapper ();

/* Return the name of the executable file as a string.
   ERR nonzero means get error if there is none specified;
   otherwise return 0 in that case.  */
extern char *get_exec_file (int err);

/* Return the inferior's current working directory.  If nothing has
   been set, then return NULL.  */
extern const char *get_inferior_cwd ();

/* Set the inferior current working directory.  If CWD is NULL, unset
   the directory.  */
extern void set_inferior_cwd (const char *cwd);

#endif /* COMMON_COMMON_INFERIOR_H */
