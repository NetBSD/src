/* Support for printing Go values for GDB, the GNU debugger.

   Copyright (C) 2012-2016 Free Software Foundation, Inc.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   NOTE: This currently only provides special support for printing gccgo
   strings.  6g objects are handled in Python.
   The remaining gccgo types may also be handled in Python.
   Strings are handled specially here, at least for now, in case the Python
   support is unavailable.  */

#include "defs.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "go-lang.h"
#include "c-lang.h"
#include "valprint.h"

/* Print a Go string.

   Note: We assume
   gdb_assert (go_classify_struct_type (type) == GO_TYPE_STRING).  */

static void
print_go_string (struct type *type, const gdb_byte *valaddr,
		 LONGEST embedded_offset, CORE_ADDR address,
		 struct ui_file *stream, int recurse,
		 const struct value *val,
		 const struct value_print_options *options)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  struct type *elt_ptr_type = TYPE_FIELD_TYPE (type, 0);
  struct type *elt_type = TYPE_TARGET_TYPE (elt_ptr_type);
  LONGEST length;
  /* TODO(dje): The encapsulation of what a pointer is belongs in value.c.
     I.e. If there's going to be unpack_pointer, there should be
     unpack_value_field_as_pointer.  Do this until we can get
     unpack_value_field_as_pointer.  */
  LONGEST addr;

  gdb_assert (valaddr == value_contents_for_printing_const (val));

  if (! unpack_value_field_as_long (type, valaddr, embedded_offset, 0,
				    val, &addr))
    error (_("Unable to read string address"));

  if (! unpack_value_field_as_long (type, valaddr, embedded_offset, 1,
				    val, &length))
    error (_("Unable to read string length"));

  /* TODO(dje): Print address of struct or actual string?  */
  if (options->addressprint)
    {
      fputs_filtered (paddress (gdbarch, addr), stream);
      fputs_filtered (" ", stream);
    }

  if (length < 0)
    {
      fputs_filtered (_("<invalid length: "), stream);
      fputs_filtered (plongest (addr), stream);
      fputs_filtered (">", stream);
      return;
    }

  /* TODO(dje): Perhaps we should pass "UTF8" for ENCODING.
     The target encoding is a global switch.
     Either choice is problematic.  */
  val_print_string (elt_type, NULL, addr, length, stream, options);
}

/* Implements the la_val_print routine for language Go.  */

void
go_val_print (struct type *type, const gdb_byte *valaddr, int embedded_offset,
	      CORE_ADDR address, struct ui_file *stream, int recurse,
	      const struct value *val,
	      const struct value_print_options *options)
{
  type = check_typedef (type);

  switch (TYPE_CODE (type))
    {
      case TYPE_CODE_STRUCT:
	{
	  enum go_type go_type = go_classify_struct_type (type);

	  switch (go_type)
	    {
	    case GO_TYPE_STRING:
	      if (! options->raw)
		{
		  print_go_string (type, valaddr, embedded_offset, address,
				   stream, recurse, val, options);
		  return;
		}
	      break;
	    default:
	      break;
	    }
	}
	/* Fall through.  */

      default:
	c_val_print (type, valaddr, embedded_offset, address, stream,
		     recurse, val, options);
	break;
    }
}
