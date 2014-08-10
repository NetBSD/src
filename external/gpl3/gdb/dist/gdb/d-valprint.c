/* Support for printing D values for GDB, the GNU debugger.

   Copyright (C) 2008-2014 Free Software Foundation, Inc.

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

#include "defs.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "d-lang.h"
#include "c-lang.h"

/* Assuming that TYPE is a TYPE_CODE_STRUCT, verify that TYPE is a
   dynamic array, and then print its value to STREAM.  Return zero if
   TYPE is a dynamic array, non-zero otherwise.  */

static int
dynamic_array_type (struct type *type, const gdb_byte *valaddr,
		    int embedded_offset, CORE_ADDR address,
		    struct ui_file *stream, int recurse,
		    const struct value *val,
		    const struct value_print_options *options)
{
  if (TYPE_NFIELDS (type) == 2
      && TYPE_CODE (TYPE_FIELD_TYPE (type, 0)) == TYPE_CODE_INT
      && strcmp (TYPE_FIELD_NAME (type, 0), "length") == 0
      && strcmp (TYPE_FIELD_NAME (type, 1), "ptr") == 0
      && value_bits_valid (val, TARGET_CHAR_BIT * embedded_offset,
			   TARGET_CHAR_BIT * TYPE_LENGTH (type)))
    {
      CORE_ADDR addr;
      struct type *elttype;
      struct type *true_type;
      struct type *ptr_type;
      struct value *ival;
      int length;

      length = unpack_field_as_long (type, valaddr + embedded_offset, 0);

      ptr_type = TYPE_FIELD_TYPE (type, 1);
      elttype = check_typedef (TYPE_TARGET_TYPE (ptr_type));
      addr = unpack_pointer (ptr_type,
			     valaddr + TYPE_FIELD_BITPOS (type, 1) / 8
			     + embedded_offset);
      true_type = check_typedef (elttype);

      true_type = lookup_array_range_type (true_type, 0, length - 1);
      ival = value_at (true_type, addr);

      d_val_print (true_type,
		   value_contents_for_printing (ival),
		   value_embedded_offset (ival), addr,
		   stream, recurse + 1, ival, options);
      return 0;
    }
  return 1;
}

/* Implements the la_val_print routine for language D.  */
void
d_val_print (struct type *type, const gdb_byte *valaddr, int embedded_offset,
             CORE_ADDR address, struct ui_file *stream, int recurse,
	     const struct value *val,
             const struct value_print_options *options)
{
  int ret;

  CHECK_TYPEDEF (type);
  switch (TYPE_CODE (type))
    {
      case TYPE_CODE_STRUCT:
        ret = dynamic_array_type (type, valaddr, embedded_offset, address,
				  stream, recurse, val, options);
	if (ret == 0)
	  break;
      default:
	c_val_print (type, valaddr, embedded_offset, address, stream,
		     recurse, val, options);
    }
}
