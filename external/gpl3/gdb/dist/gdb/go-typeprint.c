/* Support for printing Go types for GDB, the GNU debugger.

   Copyright (C) 2012-2013 Free Software Foundation, Inc.

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

/* TODO:
   - lots
   - if the more complex types get Python pretty-printers, we'll
     want a Python API for type printing
*/

#include "defs.h"
#include "gdbtypes.h"
#include "c-lang.h"
#include "go-lang.h"

/* Print a description of a type TYPE.
   Output goes to STREAM (via stdio).
   If VARSTRING is a non-empty string, print as an Ada variable/field
       declaration.
   SHOW+1 is the maximum number of levels of internal type structure
      to show (this applies to record types, enumerated types, and
      array types).
   SHOW is the number of levels of internal type structure to show
      when there is a type name for the SHOWth deepest level (0th is
      outer level).
   When SHOW<0, no inner structure is shown.
   LEVEL indicates level of recursion (for nested definitions).  */

void
go_print_type (struct type *type, const char *varstring,
	       struct ui_file *stream, int show, int level,
	       const struct type_print_options *flags)
{
  /* Borrowed from c-typeprint.c.  */
  if (show > 0)
    CHECK_TYPEDEF (type);

  /* Print the type of "abc" as "string", not char[4].  */
  if (TYPE_CODE (type) == TYPE_CODE_ARRAY
      && TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_CHAR)
    {
      fputs_filtered ("string", stream);
      return;
    }

  /* Punt the rest to C for now.  */
  c_print_type (type, varstring, stream, show, level, flags);
}
