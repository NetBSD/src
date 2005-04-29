/* Java CLASSPATH handling.
   Copyright (C) 2003 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2003.

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

#include <stdbool.h>

/* Return the new CLASSPATH value.  The given classpaths are prepended to
   the current CLASSPATH value.   If use_minimal_classpath, the current
   CLASSPATH is ignored.  */
extern char * new_classpath (const char * const *classpaths,
			     unsigned int classpaths_count,
			     bool use_minimal_classpath);

/* Set CLASSPATH and returns a safe copy of its old value.  */
extern char * set_classpath (const char * const *classpaths,
			     unsigned int classpaths_count,
			     bool use_minimal_classpath, bool verbose);

/* Restore CLASSPATH to its previous value.  */
extern void reset_classpath (char *old_classpath);
