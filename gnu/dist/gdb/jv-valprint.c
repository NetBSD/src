/* Support for printing Java values for GDB, the GNU debugger.
   Copyright 1997 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "demangle.h"
#include "valprint.h"
#include "language.h"
#include "jv-lang.h"
#include "c-lang.h"

int
java_value_print (val, stream, format, pretty)
     value_ptr val;
     GDB_FILE *stream;
     int format;
     enum val_prettyprint pretty;
{
  struct type *type = VALUE_TYPE (val);
  CORE_ADDR address = VALUE_ADDRESS (val) + VALUE_OFFSET (val);
  if (is_object_type (type))
    {
      CORE_ADDR obj_addr = unpack_pointer (type, VALUE_CONTENTS (val));
      if (obj_addr != 0)
	{
	  value_ptr obj_val
	    = value_at (TYPE_TARGET_TYPE (type), obj_addr, NULL);
	  type = type_from_class (java_class_from_object (obj_val));
	  type = lookup_pointer_type (type);
	}
    }
  if (TYPE_CODE (type) == TYPE_CODE_PTR && ! value_logical_not (val))
    {
      type_print (TYPE_TARGET_TYPE (type), "", stream, -1);
    }

  if (TYPE_CODE (type) == TYPE_CODE_STRUCT && TYPE_TAG_NAME (type) != NULL
      && TYPE_TAG_NAME (type)[0] == '[')
    {
      char buf4[4];
      long length;
      unsigned int things_printed = 0;
      int i = 0;
      int reps; 
      read_memory (address + JAVA_OBJECT_SIZE, buf4, 4);
      length = (long) extract_signed_integer (buf4, 4);
      fprintf_filtered (stream, "{length: %ld", length);
      if (TYPE_TAG_NAME (type)[1] == 'L'
	  || TYPE_TAG_NAME (type)[1] == '[')
	{
	  CORE_ADDR element, next_element;
	  address += JAVA_OBJECT_SIZE + 4; /* Skip object header and length. */
	  while (i < length && things_printed < print_max)
	    {
	      char buf[TARGET_PTR_BIT / HOST_CHAR_BIT];
	      fputs_filtered (", ", stream);
	      wrap_here (n_spaces (2));
	      if (i > 0)
		element = next_element;
	      else
		{
		  read_memory (address, buf, sizeof(buf));
		  address += TARGET_PTR_BIT / HOST_CHAR_BIT;
		  element = extract_address (buf, sizeof(buf));
		}
	      for (reps = 1;  i + reps < length;  reps++)
		{
		  read_memory (address, buf, sizeof(buf));
		  address += TARGET_PTR_BIT / HOST_CHAR_BIT;
		  next_element = extract_address (buf, sizeof(buf));
		  if (next_element != element)
		    break;
		}
	      if (reps == 1)
		fprintf_filtered (stream, "%d: ", i);
	      else
		fprintf_filtered (stream, "%d..%d: ", i, i + reps - 1);
	      if (element == 0)
		fprintf_filtered (stream, "null");
	      else
		fprintf_filtered (stream, "@%x", element);
	      things_printed++;
	      i += reps;
	    }
	}
      else
	{
	  struct type *el_type = java_primitive_type (TYPE_TAG_NAME (type)[1]);
	  value_ptr v = allocate_value (el_type);
	  value_ptr next_v = allocate_value (el_type);
	  VALUE_ADDRESS (v) = address + JAVA_OBJECT_SIZE + 4;
	  VALUE_ADDRESS (next_v) = VALUE_ADDRESS (v);

	  while (i < length && things_printed < print_max)
	    {
	      fputs_filtered (", ", stream);
	      wrap_here (n_spaces (2));
	      if (i > 0)
		{
		  value_ptr tmp = next_v;  next_v = v;   v = tmp;
		}
	      else
		{
		  VALUE_LAZY (v) = 1;
		  VALUE_OFFSET (v) = 0;
		}
	      VALUE_OFFSET (next_v) = VALUE_OFFSET (v);
	      for (reps = 1;  i + reps < length;  reps++)
		{
		  VALUE_LAZY (next_v) = 1;
		  VALUE_OFFSET (next_v) += TYPE_LENGTH (el_type);
		  if (memcmp (VALUE_CONTENTS (v), VALUE_CONTENTS (next_v),
			      TYPE_LENGTH (el_type)) != 0)
		    break;
		}
	      if (reps == 1)
		fprintf_filtered (stream, "%d: ", i);
	      else
		fprintf_filtered (stream, "%d..%d: ", i, i + reps - 1);
	      val_print (VALUE_TYPE (v), VALUE_CONTENTS (v), 0,
			 stream, format, 2, 1, pretty);
	      things_printed++;
	      i += reps;
	    }
	}

      if (i < length)
	fprintf_filtered (stream, "...");
      fprintf_filtered (stream, "}");
      return 0;
    }

  return (val_print (type, VALUE_CONTENTS (val), address,
		     stream, format, 1, 0, pretty));
}

/* TYPE, VALADDR, ADDRESS, STREAM, RECURSE, and PRETTY have the
   same meanings as in cp_print_value and c_val_print.

   DONT_PRINT is an array of baseclass types that we
   should not print, or zero if called from top level.  */

void
java_print_value_fields (type, valaddr, address, stream,
			 format, recurse, pretty)
     struct type *type;
     char *valaddr;
     CORE_ADDR address;
     GDB_FILE *stream;
     int format;
     int recurse;
     enum val_prettyprint pretty;
{
  int i, len, n_baseclasses;

  CHECK_TYPEDEF (type);

  fprintf_filtered (stream, "{");
  len = TYPE_NFIELDS (type);
  n_baseclasses = TYPE_N_BASECLASSES (type);

  if (n_baseclasses > 0)
    {
      int i, n_baseclasses = TYPE_N_BASECLASSES (type);

      for (i = 0; i < n_baseclasses; i++)
	{
	  int boffset;
	  struct type *baseclass = check_typedef (TYPE_BASECLASS (type, i));
	  char *basename = TYPE_NAME (baseclass);
	  char *base_valaddr;
	  
	  if (BASETYPE_VIA_VIRTUAL (type, i))
	    continue;

	  if (basename != NULL && strcmp (basename, "java.lang.Object") == 0)
	    continue;

	  boffset = 0;

	  if (pretty)
	    {
	      fprintf_filtered (stream, "\n");
	      print_spaces_filtered (2 * (recurse+1), stream);
	    }
	  fputs_filtered ("<", stream);
	  /* Not sure what the best notation is in the case where there is no
	     baseclass name.  */
	  fputs_filtered (basename ? basename : "", stream);
	  fputs_filtered ("> = ", stream);

	  base_valaddr = valaddr;

	  java_print_value_fields (baseclass, base_valaddr, address + boffset,
				   stream, format, recurse+1, pretty);
	  fputs_filtered (", ", stream);
	  
	flush_it:
	  ;
	}

    }

  if (!len && n_baseclasses == 1)
    fprintf_filtered (stream, "<No data fields>");
  else
    {
      extern int inspect_it;
      int fields_seen = 0;

      for (i = n_baseclasses; i < len; i++)
	{
	  /* If requested, skip printing of static fields.  */
	  if (TYPE_FIELD_STATIC (type, i))
	    {
	      char *name = TYPE_FIELD_NAME (type, i);
	      if (!static_field_print)
		continue;
	      if (name != NULL && strcmp (name, "class") == 0)
		continue;
	    }
	  if (fields_seen)
	    fprintf_filtered (stream, ", ");
	  else if (n_baseclasses > 0)
	    {
	      if (pretty)
		{
		  fprintf_filtered (stream, "\n");
		  print_spaces_filtered (2 + 2 * recurse, stream);
		  fputs_filtered ("members of ", stream);
		  fputs_filtered (type_name_no_tag (type), stream);
		  fputs_filtered (": ", stream);
		}
	    }
	  fields_seen = 1;

	  if (pretty)
	    {
	      fprintf_filtered (stream, "\n");
	      print_spaces_filtered (2 + 2 * recurse, stream);
	    }
	  else 
	    {
	      wrap_here (n_spaces (2 + 2 * recurse));
	    }
	  if (inspect_it)
	    {
	      if (TYPE_CODE (TYPE_FIELD_TYPE (type, i)) == TYPE_CODE_PTR)
		fputs_filtered ("\"( ptr \"", stream);
	      else
		fputs_filtered ("\"( nodef \"", stream);
	      if (TYPE_FIELD_STATIC (type, i))
		fputs_filtered ("static ", stream);
	      fprintf_symbol_filtered (stream, TYPE_FIELD_NAME (type, i),
				       language_cplus,
				       DMGL_PARAMS | DMGL_ANSI);
	      fputs_filtered ("\" \"", stream);
	      fprintf_symbol_filtered (stream, TYPE_FIELD_NAME (type, i),
				       language_cplus,
				       DMGL_PARAMS | DMGL_ANSI);
	      fputs_filtered ("\") \"", stream);
	    }
	  else
	    {
	      annotate_field_begin (TYPE_FIELD_TYPE (type, i));

	      if (TYPE_FIELD_STATIC (type, i))
		fputs_filtered ("static ", stream);
	      fprintf_symbol_filtered (stream, TYPE_FIELD_NAME (type, i),
				       language_cplus,
				       DMGL_PARAMS | DMGL_ANSI);
	      annotate_field_name_end ();
	      fputs_filtered (": ", stream);
	      annotate_field_value ();
	    }

	  if (!TYPE_FIELD_STATIC (type, i) && TYPE_FIELD_PACKED (type, i))
	    {
	      value_ptr v;

	      /* Bitfields require special handling, especially due to byte
		 order problems.  */
	      if (TYPE_FIELD_IGNORE (type, i))
		{
		   fputs_filtered ("<optimized out or zero length>", stream);
		}
	      else
		{
	           v = value_from_longest (TYPE_FIELD_TYPE (type, i),
				   unpack_field_as_long (type, valaddr, i));

                   val_print (TYPE_FIELD_TYPE(type, i), VALUE_CONTENTS (v), 0,
			      stream, format, 0, recurse + 1, pretty);
		}
	    }
	  else
	    {
	      if (TYPE_FIELD_IGNORE (type, i))
		{
		   fputs_filtered ("<optimized out or zero length>", stream);
		}
	      else if (TYPE_FIELD_STATIC (type, i))
		{
		  value_ptr v = value_static_field (type, i);
		  if (v == NULL)
		    fputs_filtered ("<optimized out>", stream);
		  else
		    {
		      struct type *t = check_typedef (VALUE_TYPE (v));
		      if (TYPE_CODE (t) == TYPE_CODE_STRUCT)
			v = value_addr (v);
		      val_print (VALUE_TYPE (v),
				 VALUE_CONTENTS (v), VALUE_ADDRESS (v),
				 stream, format, 0, recurse+1, pretty);
		    }
		}
	      else
		{
	           val_print (TYPE_FIELD_TYPE (type, i), 
			      valaddr + TYPE_FIELD_BITPOS (type, i) / 8,
			      address + TYPE_FIELD_BITPOS (type, i) / 8,
			      stream, format, 0, recurse + 1, pretty);
		}
	    }
	  annotate_field_end ();
	}

      if (pretty)
	{
	  fprintf_filtered (stream, "\n");
	  print_spaces_filtered (2 * recurse, stream);
	}
    }
  fprintf_filtered (stream, "}");
}

int
java_val_print (type, valaddr, address, stream, format, deref_ref, recurse,
	     pretty)
     struct type *type;
     char *valaddr;
     CORE_ADDR address;
     GDB_FILE *stream;
     int format;
     int deref_ref;
     int recurse;
     enum val_prettyprint pretty;
{
  register unsigned int i = 0;		/* Number of characters printed */
  struct type *elttype;
  CORE_ADDR addr;

  CHECK_TYPEDEF (type);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_PTR:
      if (format && format != 's')
	{
	  print_scalar_formatted (valaddr, type, format, 0, stream);
	  break;
	}
#if 0
      if (vtblprint && cp_is_vtbl_ptr_type(type))
	{
          /* Print the unmangled name if desired.  */
	  /* Print vtable entry - we only get here if we ARE using
	     -fvtable_thunks.  (Otherwise, look under TYPE_CODE_STRUCT.) */
	  print_address_demangle(extract_address (valaddr, TYPE_LENGTH (type)),
				 stream, demangle);
	  break;
	}
#endif
      addr = unpack_pointer (type, valaddr);
      if (addr == 0)
	{
	  fputs_filtered ("null", stream);
	  return i;
	}
      elttype = check_typedef (TYPE_TARGET_TYPE (type));
	{
	print_unpacked_pointer:
          elttype = check_typedef (TYPE_TARGET_TYPE (type));

	  if (TYPE_CODE (elttype) == TYPE_CODE_FUNC)
	    {
	      /* Try to print what function it points to.  */
	      print_address_demangle (addr, stream, demangle);
	      /* Return value is irrelevant except for string pointers.  */
	      return (0);
	    }

	  if (addressprint && format != 's')
	    {
	      fputs_filtered ("@", stream);
	      print_longest (stream, 'x', 0, (ULONGEST) addr);
	    }
	  return i;
	}
    case TYPE_CODE_CHAR:
      format = format ? format : output_format;
      if (format)
	{
	  print_scalar_formatted (valaddr, type, format, 0, stream);
	}
      else
	{
	  LA_PRINT_CHAR ((int) unpack_long (type, valaddr), stream);
	}
      break;
    case TYPE_CODE_STRUCT:
      java_print_value_fields (type, valaddr, address, stream, format,
			       recurse, pretty);
      break;
    default:
      return c_val_print (type, valaddr, address, stream, format,
			  deref_ref, recurse, pretty);
    }
}
