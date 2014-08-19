/* Support for printing Pascal values for GDB, the GNU debugger.

   Copyright (C) 2000-2014 Free Software Foundation, Inc.

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

/* This file is derived from c-valprint.c */

#include "defs.h"
#include "gdb_obstack.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "command.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "demangle.h"
#include "valprint.h"
#include "typeprint.h"
#include "language.h"
#include "target.h"
#include "annotate.h"
#include "p-lang.h"
#include "cp-abi.h"
#include "cp-support.h"
#include "exceptions.h"


/* Decorations for Pascal.  */

static const struct generic_val_print_decorations p_decorations =
{
  "",
  " + ",
  " * I",
  "true",
  "false",
  "void"
};

/* See val_print for a description of the various parameters of this
   function; they are identical.  */

void
pascal_val_print (struct type *type, const gdb_byte *valaddr,
		  int embedded_offset, CORE_ADDR address,
		  struct ui_file *stream, int recurse,
		  const struct value *original_value,
		  const struct value_print_options *options)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int i = 0;	/* Number of characters printed */
  unsigned len;
  LONGEST low_bound, high_bound;
  struct type *elttype;
  unsigned eltlen;
  int length_pos, length_size, string_pos;
  struct type *char_type;
  CORE_ADDR addr;
  int want_space = 0;

  CHECK_TYPEDEF (type);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_ARRAY:
      if (get_array_bounds (type, &low_bound, &high_bound))
	{
	  len = high_bound - low_bound + 1;
	  elttype = check_typedef (TYPE_TARGET_TYPE (type));
	  eltlen = TYPE_LENGTH (elttype);
	  if (options->prettyformat_arrays)
	    {
	      print_spaces_filtered (2 + 2 * recurse, stream);
	    }
	  /* If 's' format is used, try to print out as string.
	     If no format is given, print as string if element type
	     is of TYPE_CODE_CHAR and element size is 1,2 or 4.  */
	  if (options->format == 's'
	      || ((eltlen == 1 || eltlen == 2 || eltlen == 4)
		  && TYPE_CODE (elttype) == TYPE_CODE_CHAR
		  && options->format == 0))
	    {
	      /* If requested, look for the first null char and only print
	         elements up to it.  */
	      if (options->stop_print_at_null)
		{
		  unsigned int temp_len;

		  /* Look for a NULL char.  */
		  for (temp_len = 0;
		       extract_unsigned_integer (valaddr + embedded_offset +
						 temp_len * eltlen, eltlen,
						 byte_order)
		       && temp_len < len && temp_len < options->print_max;
		       temp_len++);
		  len = temp_len;
		}

	      LA_PRINT_STRING (stream, TYPE_TARGET_TYPE (type),
			       valaddr + embedded_offset, len, NULL, 0,
			       options);
	      i = len;
	    }
	  else
	    {
	      fprintf_filtered (stream, "{");
	      /* If this is a virtual function table, print the 0th
	         entry specially, and the rest of the members normally.  */
	      if (pascal_object_is_vtbl_ptr_type (elttype))
		{
		  i = 1;
		  fprintf_filtered (stream, "%d vtable entries", len - 1);
		}
	      else
		{
		  i = 0;
		}
	      val_print_array_elements (type, valaddr, embedded_offset,
					address, stream, recurse,
					original_value, options, i);
	      fprintf_filtered (stream, "}");
	    }
	  break;
	}
      /* Array of unspecified length: treat like pointer to first elt.  */
      addr = address + embedded_offset;
      goto print_unpacked_pointer;

    case TYPE_CODE_PTR:
      if (options->format && options->format != 's')
	{
	  val_print_scalar_formatted (type, valaddr, embedded_offset,
				      original_value, options, 0, stream);
	  break;
	}
      if (options->vtblprint && pascal_object_is_vtbl_ptr_type (type))
	{
	  /* Print the unmangled name if desired.  */
	  /* Print vtable entry - we only get here if we ARE using
	     -fvtable_thunks.  (Otherwise, look under TYPE_CODE_STRUCT.)  */
	  /* Extract the address, assume that it is unsigned.  */
	  addr = extract_unsigned_integer (valaddr + embedded_offset,
					   TYPE_LENGTH (type), byte_order);
	  print_address_demangle (options, gdbarch, addr, stream, demangle);
	  break;
	}
      check_typedef (TYPE_TARGET_TYPE (type));

      addr = unpack_pointer (type, valaddr + embedded_offset);
    print_unpacked_pointer:
      elttype = check_typedef (TYPE_TARGET_TYPE (type));

      if (TYPE_CODE (elttype) == TYPE_CODE_FUNC)
	{
	  /* Try to print what function it points to.  */
	  print_address_demangle (options, gdbarch, addr, stream, demangle);
	  return;
	}

      if (options->addressprint && options->format != 's')
	{
	  fputs_filtered (paddress (gdbarch, addr), stream);
	  want_space = 1;
	}

      /* For a pointer to char or unsigned char, also print the string
	 pointed to, unless pointer is null.  */
      if (((TYPE_LENGTH (elttype) == 1
	   && (TYPE_CODE (elttype) == TYPE_CODE_INT
	      || TYPE_CODE (elttype) == TYPE_CODE_CHAR))
	  || ((TYPE_LENGTH (elttype) == 2 || TYPE_LENGTH (elttype) == 4)
	      && TYPE_CODE (elttype) == TYPE_CODE_CHAR))
	  && (options->format == 0 || options->format == 's')
	  && addr != 0)
	{
	  if (want_space)
	    fputs_filtered (" ", stream);
	  /* No wide string yet.  */
	  i = val_print_string (elttype, NULL, addr, -1, stream, options);
	}
      /* Also for pointers to pascal strings.  */
      /* Note: this is Free Pascal specific:
	 as GDB does not recognize stabs pascal strings
	 Pascal strings are mapped to records
	 with lowercase names PM.  */
      if (is_pascal_string_type (elttype, &length_pos, &length_size,
				 &string_pos, &char_type, NULL)
	  && addr != 0)
	{
	  ULONGEST string_length;
	  void *buffer;

	  if (want_space)
	    fputs_filtered (" ", stream);
	  buffer = xmalloc (length_size);
	  read_memory (addr + length_pos, buffer, length_size);
	  string_length = extract_unsigned_integer (buffer, length_size,
						    byte_order);
	  xfree (buffer);
	  i = val_print_string (char_type, NULL,
				addr + string_pos, string_length,
				stream, options);
	}
      else if (pascal_object_is_vtbl_member (type))
	{
	  /* Print vtbl's nicely.  */
	  CORE_ADDR vt_address = unpack_pointer (type,
						 valaddr + embedded_offset);
	  struct bound_minimal_symbol msymbol =
	    lookup_minimal_symbol_by_pc (vt_address);

	  /* If 'symbol_print' is set, we did the work above.  */
	  if (!options->symbol_print
	      && (msymbol.minsym != NULL)
	      && (vt_address == SYMBOL_VALUE_ADDRESS (msymbol.minsym)))
	    {
	      if (want_space)
		fputs_filtered (" ", stream);
	      fputs_filtered ("<", stream);
	      fputs_filtered (SYMBOL_PRINT_NAME (msymbol.minsym), stream);
	      fputs_filtered (">", stream);
	      want_space = 1;
	    }
	  if (vt_address && options->vtblprint)
	    {
	      struct value *vt_val;
	      struct symbol *wsym = (struct symbol *) NULL;
	      struct type *wtype;
	      struct block *block = (struct block *) NULL;
	      struct field_of_this_result is_this_fld;

	      if (want_space)
		fputs_filtered (" ", stream);

	      if (msymbol.minsym != NULL)
		wsym = lookup_symbol (SYMBOL_LINKAGE_NAME (msymbol.minsym),
				      block,
				      VAR_DOMAIN, &is_this_fld);

	      if (wsym)
		{
		  wtype = SYMBOL_TYPE (wsym);
		}
	      else
		{
		  wtype = TYPE_TARGET_TYPE (type);
		}
	      vt_val = value_at (wtype, vt_address);
	      common_val_print (vt_val, stream, recurse + 1, options,
				current_language);
	      if (options->prettyformat)
		{
		  fprintf_filtered (stream, "\n");
		  print_spaces_filtered (2 + 2 * recurse, stream);
		}
	    }
	}

      return;

    case TYPE_CODE_REF:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_UNDEF:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
      generic_val_print (type, valaddr, embedded_offset, address,
			 stream, recurse, original_value, options,
			 &p_decorations);
      break;

    case TYPE_CODE_UNION:
      if (recurse && !options->unionprint)
	{
	  fprintf_filtered (stream, "{...}");
	  break;
	}
      /* Fall through.  */
    case TYPE_CODE_STRUCT:
      if (options->vtblprint && pascal_object_is_vtbl_ptr_type (type))
	{
	  /* Print the unmangled name if desired.  */
	  /* Print vtable entry - we only get here if NOT using
	     -fvtable_thunks.  (Otherwise, look under TYPE_CODE_PTR.)  */
	  /* Extract the address, assume that it is unsigned.  */
	  print_address_demangle
	    (options, gdbarch,
	     extract_unsigned_integer (valaddr + embedded_offset
				       + TYPE_FIELD_BITPOS (type,
							    VTBL_FNADDR_OFFSET) / 8,
				       TYPE_LENGTH (TYPE_FIELD_TYPE (type,
								     VTBL_FNADDR_OFFSET)),
				       byte_order),
	     stream, demangle);
	}
      else
	{
          if (is_pascal_string_type (type, &length_pos, &length_size,
                                     &string_pos, &char_type, NULL))
	    {
	      len = extract_unsigned_integer (valaddr + embedded_offset
					      + length_pos, length_size,
					      byte_order);
	      LA_PRINT_STRING (stream, char_type,
			       valaddr + embedded_offset + string_pos,
			       len, NULL, 0, options);
	    }
	  else
	    pascal_object_print_value_fields (type, valaddr, embedded_offset,
					      address, stream, recurse,
					      original_value, options,
					      NULL, 0);
	}
      break;

    case TYPE_CODE_SET:
      elttype = TYPE_INDEX_TYPE (type);
      CHECK_TYPEDEF (elttype);
      if (TYPE_STUB (elttype))
	{
	  fprintf_filtered (stream, "<incomplete type>");
	  gdb_flush (stream);
	  break;
	}
      else
	{
	  struct type *range = elttype;
	  LONGEST low_bound, high_bound;
	  int i;
	  int need_comma = 0;

	  fputs_filtered ("[", stream);

	  i = get_discrete_bounds (range, &low_bound, &high_bound);
	  if (low_bound == 0 && high_bound == -1 && TYPE_LENGTH (type) > 0)
	    {
	      /* If we know the size of the set type, we can figure out the
	      maximum value.  */
	      i = 0;
	      high_bound = TYPE_LENGTH (type) * TARGET_CHAR_BIT - 1;
	      TYPE_HIGH_BOUND (range) = high_bound;
	    }
	maybe_bad_bstring:
	  if (i < 0)
	    {
	      fputs_filtered ("<error value>", stream);
	      goto done;
	    }

	  for (i = low_bound; i <= high_bound; i++)
	    {
	      int element = value_bit_index (type,
					     valaddr + embedded_offset, i);

	      if (element < 0)
		{
		  i = element;
		  goto maybe_bad_bstring;
		}
	      if (element)
		{
		  if (need_comma)
		    fputs_filtered (", ", stream);
		  print_type_scalar (range, i, stream);
		  need_comma = 1;

		  if (i + 1 <= high_bound
		      && value_bit_index (type,
					  valaddr + embedded_offset, ++i))
		    {
		      int j = i;

		      fputs_filtered ("..", stream);
		      while (i + 1 <= high_bound
			     && value_bit_index (type,
						 valaddr + embedded_offset,
						 ++i))
			j = i;
		      print_type_scalar (range, j, stream);
		    }
		}
	    }
	done:
	  fputs_filtered ("]", stream);
	}
      break;

    default:
      error (_("Invalid pascal type code %d in symbol table."),
	     TYPE_CODE (type));
    }
  gdb_flush (stream);
}

void
pascal_value_print (struct value *val, struct ui_file *stream,
		    const struct value_print_options *options)
{
  struct type *type = value_type (val);
  struct value_print_options opts = *options;

  opts.deref_ref = 1;

  /* If it is a pointer, indicate what it points to.

     Print type also if it is a reference.

     Object pascal: if it is a member pointer, we will take care
     of that when we print it.  */
  if (TYPE_CODE (type) == TYPE_CODE_PTR
      || TYPE_CODE (type) == TYPE_CODE_REF)
    {
      /* Hack:  remove (char *) for char strings.  Their
         type is indicated by the quoted string anyway.  */
      if (TYPE_CODE (type) == TYPE_CODE_PTR
	  && TYPE_NAME (type) == NULL
	  && TYPE_NAME (TYPE_TARGET_TYPE (type)) != NULL
	  && strcmp (TYPE_NAME (TYPE_TARGET_TYPE (type)), "char") == 0)
	{
	  /* Print nothing.  */
	}
      else
	{
	  fprintf_filtered (stream, "(");
	  type_print (type, "", stream, -1);
	  fprintf_filtered (stream, ") ");
	}
    }
  common_val_print (val, stream, 0, &opts, current_language);
}


static void
show_pascal_static_field_print (struct ui_file *file, int from_tty,
				struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Printing of pascal static members is %s.\n"),
		    value);
}

static struct obstack dont_print_vb_obstack;
static struct obstack dont_print_statmem_obstack;

static void pascal_object_print_static_field (struct value *,
					      struct ui_file *, int,
					      const struct value_print_options *);

static void pascal_object_print_value (struct type *, const gdb_byte *,
				       int,
				       CORE_ADDR, struct ui_file *, int,
				       const struct value *,
				       const struct value_print_options *,
				       struct type **);

/* It was changed to this after 2.4.5.  */
const char pascal_vtbl_ptr_name[] =
{'_', '_', 'v', 't', 'b', 'l', '_', 'p', 't', 'r', '_', 't', 'y', 'p', 'e', 0};

/* Return truth value for assertion that TYPE is of the type
   "pointer to virtual function".  */

int
pascal_object_is_vtbl_ptr_type (struct type *type)
{
  const char *typename = type_name_no_tag (type);

  return (typename != NULL
	  && strcmp (typename, pascal_vtbl_ptr_name) == 0);
}

/* Return truth value for the assertion that TYPE is of the type
   "pointer to virtual function table".  */

int
pascal_object_is_vtbl_member (struct type *type)
{
  if (TYPE_CODE (type) == TYPE_CODE_PTR)
    {
      type = TYPE_TARGET_TYPE (type);
      if (TYPE_CODE (type) == TYPE_CODE_ARRAY)
	{
	  type = TYPE_TARGET_TYPE (type);
	  if (TYPE_CODE (type) == TYPE_CODE_STRUCT	/* If not using
							   thunks.  */
	      || TYPE_CODE (type) == TYPE_CODE_PTR)	/* If using thunks.  */
	    {
	      /* Virtual functions tables are full of pointers
	         to virtual functions.  */
	      return pascal_object_is_vtbl_ptr_type (type);
	    }
	}
    }
  return 0;
}

/* Mutually recursive subroutines of pascal_object_print_value and
   c_val_print to print out a structure's fields:
   pascal_object_print_value_fields and pascal_object_print_value.

   TYPE, VALADDR, ADDRESS, STREAM, RECURSE, and OPTIONS have the
   same meanings as in pascal_object_print_value and c_val_print.

   DONT_PRINT is an array of baseclass types that we
   should not print, or zero if called from top level.  */

void
pascal_object_print_value_fields (struct type *type, const gdb_byte *valaddr,
				  int offset,
				  CORE_ADDR address, struct ui_file *stream,
				  int recurse,
				  const struct value *val,
				  const struct value_print_options *options,
				  struct type **dont_print_vb,
				  int dont_print_statmem)
{
  int i, len, n_baseclasses;
  char *last_dont_print = obstack_next_free (&dont_print_statmem_obstack);

  CHECK_TYPEDEF (type);

  fprintf_filtered (stream, "{");
  len = TYPE_NFIELDS (type);
  n_baseclasses = TYPE_N_BASECLASSES (type);

  /* Print out baseclasses such that we don't print
     duplicates of virtual baseclasses.  */
  if (n_baseclasses > 0)
    pascal_object_print_value (type, valaddr, offset, address,
			       stream, recurse + 1, val,
			       options, dont_print_vb);

  if (!len && n_baseclasses == 1)
    fprintf_filtered (stream, "<No data fields>");
  else
    {
      struct obstack tmp_obstack = dont_print_statmem_obstack;
      int fields_seen = 0;

      if (dont_print_statmem == 0)
	{
	  /* If we're at top level, carve out a completely fresh
	     chunk of the obstack and use that until this particular
	     invocation returns.  */
	  obstack_finish (&dont_print_statmem_obstack);
	}

      for (i = n_baseclasses; i < len; i++)
	{
	  /* If requested, skip printing of static fields.  */
	  if (!options->pascal_static_field_print
	      && field_is_static (&TYPE_FIELD (type, i)))
	    continue;
	  if (fields_seen)
	    fprintf_filtered (stream, ", ");
	  else if (n_baseclasses > 0)
	    {
	      if (options->prettyformat)
		{
		  fprintf_filtered (stream, "\n");
		  print_spaces_filtered (2 + 2 * recurse, stream);
		  fputs_filtered ("members of ", stream);
		  fputs_filtered (type_name_no_tag (type), stream);
		  fputs_filtered (": ", stream);
		}
	    }
	  fields_seen = 1;

	  if (options->prettyformat)
	    {
	      fprintf_filtered (stream, "\n");
	      print_spaces_filtered (2 + 2 * recurse, stream);
	    }
	  else
	    {
	      wrap_here (n_spaces (2 + 2 * recurse));
	    }

	  annotate_field_begin (TYPE_FIELD_TYPE (type, i));

	  if (field_is_static (&TYPE_FIELD (type, i)))
	    fputs_filtered ("static ", stream);
	  fprintf_symbol_filtered (stream, TYPE_FIELD_NAME (type, i),
				   language_cplus,
				   DMGL_PARAMS | DMGL_ANSI);
	  annotate_field_name_end ();
	  fputs_filtered (" = ", stream);
	  annotate_field_value ();

	  if (!field_is_static (&TYPE_FIELD (type, i))
	      && TYPE_FIELD_PACKED (type, i))
	    {
	      struct value *v;

	      /* Bitfields require special handling, especially due to byte
	         order problems.  */
	      if (TYPE_FIELD_IGNORE (type, i))
		{
		  fputs_filtered ("<optimized out or zero length>", stream);
		}
	      else if (value_bits_synthetic_pointer (val,
						     TYPE_FIELD_BITPOS (type,
									i),
						     TYPE_FIELD_BITSIZE (type,
									 i)))
		{
		  fputs_filtered (_("<synthetic pointer>"), stream);
		}
	      else if (!value_bits_valid (val, TYPE_FIELD_BITPOS (type, i),
					  TYPE_FIELD_BITSIZE (type, i)))
		{
		  val_print_optimized_out (val, stream);
		}
	      else
		{
		  struct value_print_options opts = *options;

		  v = value_field_bitfield (type, i, valaddr, offset, val);

		  opts.deref_ref = 0;
		  common_val_print (v, stream, recurse + 1, &opts,
				    current_language);
		}
	    }
	  else
	    {
	      if (TYPE_FIELD_IGNORE (type, i))
		{
		  fputs_filtered ("<optimized out or zero length>", stream);
		}
	      else if (field_is_static (&TYPE_FIELD (type, i)))
		{
		  /* struct value *v = value_static_field (type, i);
		     v4.17 specific.  */
		  struct value *v;

		  v = value_field_bitfield (type, i, valaddr, offset, val);

		  if (v == NULL)
		    val_print_optimized_out (NULL, stream);
		  else
		    pascal_object_print_static_field (v, stream, recurse + 1,
						      options);
		}
	      else
		{
		  struct value_print_options opts = *options;

		  opts.deref_ref = 0;
		  /* val_print (TYPE_FIELD_TYPE (type, i),
		     valaddr + TYPE_FIELD_BITPOS (type, i) / 8,
		     address + TYPE_FIELD_BITPOS (type, i) / 8, 0,
		     stream, format, 0, recurse + 1, pretty); */
		  val_print (TYPE_FIELD_TYPE (type, i),
			     valaddr, offset + TYPE_FIELD_BITPOS (type, i) / 8,
			     address, stream, recurse + 1, val, &opts,
			     current_language);
		}
	    }
	  annotate_field_end ();
	}

      if (dont_print_statmem == 0)
	{
	  /* Free the space used to deal with the printing
	     of the members from top level.  */
	  obstack_free (&dont_print_statmem_obstack, last_dont_print);
	  dont_print_statmem_obstack = tmp_obstack;
	}

      if (options->prettyformat)
	{
	  fprintf_filtered (stream, "\n");
	  print_spaces_filtered (2 * recurse, stream);
	}
    }
  fprintf_filtered (stream, "}");
}

/* Special val_print routine to avoid printing multiple copies of virtual
   baseclasses.  */

static void
pascal_object_print_value (struct type *type, const gdb_byte *valaddr,
			   int offset,
			   CORE_ADDR address, struct ui_file *stream,
			   int recurse,
			   const struct value *val,
			   const struct value_print_options *options,
			   struct type **dont_print_vb)
{
  struct type **last_dont_print
    = (struct type **) obstack_next_free (&dont_print_vb_obstack);
  struct obstack tmp_obstack = dont_print_vb_obstack;
  int i, n_baseclasses = TYPE_N_BASECLASSES (type);

  if (dont_print_vb == 0)
    {
      /* If we're at top level, carve out a completely fresh
         chunk of the obstack and use that until this particular
         invocation returns.  */
      /* Bump up the high-water mark.  Now alpha is omega.  */
      obstack_finish (&dont_print_vb_obstack);
    }

  for (i = 0; i < n_baseclasses; i++)
    {
      int boffset = 0;
      struct type *baseclass = check_typedef (TYPE_BASECLASS (type, i));
      const char *basename = type_name_no_tag (baseclass);
      const gdb_byte *base_valaddr = NULL;
      int thisoffset;
      volatile struct gdb_exception ex;
      int skip = 0;

      if (BASETYPE_VIA_VIRTUAL (type, i))
	{
	  struct type **first_dont_print
	    = (struct type **) obstack_base (&dont_print_vb_obstack);

	  int j = (struct type **) obstack_next_free (&dont_print_vb_obstack)
	    - first_dont_print;

	  while (--j >= 0)
	    if (baseclass == first_dont_print[j])
	      goto flush_it;

	  obstack_ptr_grow (&dont_print_vb_obstack, baseclass);
	}

      thisoffset = offset;

      TRY_CATCH (ex, RETURN_MASK_ERROR)
	{
	  boffset = baseclass_offset (type, i, valaddr, offset, address, val);
	}
      if (ex.reason < 0 && ex.error == NOT_AVAILABLE_ERROR)
	skip = -1;
      else if (ex.reason < 0)
	skip = 1;
      else
	{
	  skip = 0;

	  /* The virtual base class pointer might have been clobbered by the
	     user program. Make sure that it still points to a valid memory
	     location.  */

	  if (boffset < 0 || boffset >= TYPE_LENGTH (type))
	    {
	      gdb_byte *buf;
	      struct cleanup *back_to;

	      buf = xmalloc (TYPE_LENGTH (baseclass));
	      back_to = make_cleanup (xfree, buf);

	      base_valaddr = buf;
	      if (target_read_memory (address + boffset, buf,
				      TYPE_LENGTH (baseclass)) != 0)
		skip = 1;
	      address = address + boffset;
	      thisoffset = 0;
	      boffset = 0;
	      do_cleanups (back_to);
	    }
	  else
	    base_valaddr = valaddr;
	}

      if (options->prettyformat)
	{
	  fprintf_filtered (stream, "\n");
	  print_spaces_filtered (2 * recurse, stream);
	}
      fputs_filtered ("<", stream);
      /* Not sure what the best notation is in the case where there is no
         baseclass name.  */

      fputs_filtered (basename ? basename : "", stream);
      fputs_filtered ("> = ", stream);

      if (skip < 0)
	val_print_unavailable (stream);
      else if (skip > 0)
	val_print_invalid_address (stream);
      else
	pascal_object_print_value_fields (baseclass, base_valaddr,
					  thisoffset + boffset, address,
					  stream, recurse, val, options,
		     (struct type **) obstack_base (&dont_print_vb_obstack),
					  0);
      fputs_filtered (", ", stream);

    flush_it:
      ;
    }

  if (dont_print_vb == 0)
    {
      /* Free the space used to deal with the printing
         of this type from top level.  */
      obstack_free (&dont_print_vb_obstack, last_dont_print);
      /* Reset watermark so that we can continue protecting
         ourselves from whatever we were protecting ourselves.  */
      dont_print_vb_obstack = tmp_obstack;
    }
}

/* Print value of a static member.
   To avoid infinite recursion when printing a class that contains
   a static instance of the class, we keep the addresses of all printed
   static member classes in an obstack and refuse to print them more
   than once.

   VAL contains the value to print, STREAM, RECURSE, and OPTIONS
   have the same meanings as in c_val_print.  */

static void
pascal_object_print_static_field (struct value *val,
				  struct ui_file *stream,
				  int recurse,
				  const struct value_print_options *options)
{
  struct type *type = value_type (val);
  struct value_print_options opts;

  if (value_entirely_optimized_out (val))
    {
      val_print_optimized_out (val, stream);
      return;
    }

  if (TYPE_CODE (type) == TYPE_CODE_STRUCT)
    {
      CORE_ADDR *first_dont_print, addr;
      int i;

      first_dont_print
	= (CORE_ADDR *) obstack_base (&dont_print_statmem_obstack);
      i = (CORE_ADDR *) obstack_next_free (&dont_print_statmem_obstack)
	- first_dont_print;

      while (--i >= 0)
	{
	  if (value_address (val) == first_dont_print[i])
	    {
	      fputs_filtered ("\
<same as static member of an already seen type>",
			      stream);
	      return;
	    }
	}

      addr = value_address (val);
      obstack_grow (&dont_print_statmem_obstack, (char *) &addr,
		    sizeof (CORE_ADDR));

      CHECK_TYPEDEF (type);
      pascal_object_print_value_fields (type,
					value_contents_for_printing (val),
					value_embedded_offset (val),
					addr,
					stream, recurse,
					val, options, NULL, 1);
      return;
    }

  opts = *options;
  opts.deref_ref = 0;
  common_val_print (val, stream, recurse, &opts, current_language);
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_pascal_valprint;

void
_initialize_pascal_valprint (void)
{
  add_setshow_boolean_cmd ("pascal_static-members", class_support,
			   &user_print_options.pascal_static_field_print, _("\
Set printing of pascal static members."), _("\
Show printing of pascal static members."), NULL,
			   NULL,
			   show_pascal_static_field_print,
			   &setprintlist, &showprintlist);
}
