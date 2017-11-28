/* Support for printing C values for GDB, the GNU debugger.

   Copyright (C) 1986-2017 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "valprint.h"
#include "language.h"
#include "c-lang.h"
#include "cp-abi.h"
#include "target.h"
#include "objfiles.h"


/* A helper for c_textual_element_type.  This checks the name of the
   typedef.  This is bogus but it isn't apparent that the compiler
   provides us the help we may need.  */

static int
textual_name (const char *name)
{
  return (!strcmp (name, "wchar_t")
	  || !strcmp (name, "char16_t")
	  || !strcmp (name, "char32_t"));
}

/* Apply a heuristic to decide whether an array of TYPE or a pointer
   to TYPE should be printed as a textual string.  Return non-zero if
   it should, or zero if it should be treated as an array of integers
   or pointer to integers.  FORMAT is the current format letter, or 0
   if none.

   We guess that "char" is a character.  Explicitly signed and
   unsigned character types are also characters.  Integer data from
   vector types is not.  The user can override this by using the /s
   format letter.  */

int
c_textual_element_type (struct type *type, char format)
{
  struct type *true_type, *iter_type;

  if (format != 0 && format != 's')
    return 0;

  /* We also rely on this for its side effect of setting up all the
     typedef pointers.  */
  true_type = check_typedef (type);

  /* TYPE_CODE_CHAR is always textual.  */
  if (TYPE_CODE (true_type) == TYPE_CODE_CHAR)
    return 1;

  /* Any other character-like types must be integral.  */
  if (TYPE_CODE (true_type) != TYPE_CODE_INT)
    return 0;

  /* We peel typedefs one by one, looking for a match.  */
  iter_type = type;
  while (iter_type)
    {
      /* Check the name of the type.  */
      if (TYPE_NAME (iter_type) && textual_name (TYPE_NAME (iter_type)))
	return 1;

      if (TYPE_CODE (iter_type) != TYPE_CODE_TYPEDEF)
	break;

      /* Peel a single typedef.  If the typedef doesn't have a target
	 type, we use check_typedef and hope the result is ok -- it
	 might be for C++, where wchar_t is a built-in type.  */
      if (TYPE_TARGET_TYPE (iter_type))
	iter_type = TYPE_TARGET_TYPE (iter_type);
      else
	iter_type = check_typedef (iter_type);
    }

  if (format == 's')
    {
      /* Print this as a string if we can manage it.  For now, no wide
	 character support.  */
      if (TYPE_CODE (true_type) == TYPE_CODE_INT
	  && TYPE_LENGTH (true_type) == 1)
	return 1;
    }
  else
    {
      /* If a one-byte TYPE_CODE_INT is missing the not-a-character
	 flag, then we treat it as text; otherwise, we assume it's
	 being used as data.  */
      if (TYPE_CODE (true_type) == TYPE_CODE_INT
	  && TYPE_LENGTH (true_type) == 1
	  && !TYPE_NOTTEXT (true_type))
	return 1;
    }

  return 0;
}

/* Decorations for C.  */

static const struct generic_val_print_decorations c_decorations =
{
  "",
  " + ",
  " * I",
  "true",
  "false",
  "void",
  "{",
  "}"
};

/* Print a pointer based on the type of its target.

   Arguments to this functions are roughly the same as those in c_val_print.
   A difference is that ADDRESS is the address to print, with embedded_offset
   already added.  UNRESOLVED_ELTTYPE and ELTTYPE represent the pointed type,
   respectively before and after check_typedef.  */

static void
print_unpacked_pointer (struct type *type, struct type *elttype,
			struct type *unresolved_elttype,
			const gdb_byte *valaddr, int embedded_offset,
			CORE_ADDR address, struct ui_file *stream, int recurse,
			const struct value_print_options *options)
{
  int want_space = 0;
  struct gdbarch *gdbarch = get_type_arch (type);

  if (TYPE_CODE (elttype) == TYPE_CODE_FUNC)
    {
      /* Try to print what function it points to.  */
      print_function_pointer_address (options, gdbarch, address, stream);
      return;
    }

  if (options->symbol_print)
    want_space = print_address_demangle (options, gdbarch, address, stream,
					 demangle);
  else if (options->addressprint)
    {
      fputs_filtered (paddress (gdbarch, address), stream);
      want_space = 1;
    }

  /* For a pointer to a textual type, also print the string
     pointed to, unless pointer is null.  */

  if (c_textual_element_type (unresolved_elttype, options->format)
      && address != 0)
    {
      if (want_space)
	fputs_filtered (" ", stream);
      val_print_string (unresolved_elttype, NULL, address, -1, stream, options);
    }
  else if (cp_is_vtbl_member (type))
    {
      /* Print vtbl's nicely.  */
      CORE_ADDR vt_address = unpack_pointer (type, valaddr + embedded_offset);
      struct bound_minimal_symbol msymbol =
	lookup_minimal_symbol_by_pc (vt_address);

      /* If 'symbol_print' is set, we did the work above.  */
      if (!options->symbol_print
	  && (msymbol.minsym != NULL)
	  && (vt_address == BMSYMBOL_VALUE_ADDRESS (msymbol)))
	{
	  if (want_space)
	    fputs_filtered (" ", stream);
	  fputs_filtered (" <", stream);
	  fputs_filtered (MSYMBOL_PRINT_NAME (msymbol.minsym), stream);
	  fputs_filtered (">", stream);
	  want_space = 1;
	}

      if (vt_address && options->vtblprint)
	{
	  struct value *vt_val;
	  struct symbol *wsym = NULL;
	  struct type *wtype;
	  struct block *block = NULL;
	  struct field_of_this_result is_this_fld;

	  if (want_space)
	    fputs_filtered (" ", stream);

	  if (msymbol.minsym != NULL)
	    wsym = lookup_symbol (MSYMBOL_LINKAGE_NAME(msymbol.minsym), block,
				  VAR_DOMAIN, &is_this_fld).symbol;

	  if (wsym)
	    {
	      wtype = SYMBOL_TYPE (wsym);
	    }
	  else
	    {
	      wtype = unresolved_elttype;
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
}

/* c_val_print helper for TYPE_CODE_ARRAY.  */

static void
c_val_print_array (struct type *type, const gdb_byte *valaddr,
		   int embedded_offset, CORE_ADDR address,
		   struct ui_file *stream, int recurse,
		   struct value *original_value,
		   const struct value_print_options *options)
{
  struct type *unresolved_elttype = TYPE_TARGET_TYPE (type);
  struct type *elttype = check_typedef (unresolved_elttype);
  struct gdbarch *arch = get_type_arch (type);
  int unit_size = gdbarch_addressable_memory_unit_size (arch);

  if (TYPE_LENGTH (type) > 0 && TYPE_LENGTH (unresolved_elttype) > 0)
    {
      LONGEST low_bound, high_bound;
      int eltlen, len;
      struct gdbarch *gdbarch = get_type_arch (type);
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      unsigned int i = 0;	/* Number of characters printed.  */

      if (!get_array_bounds (type, &low_bound, &high_bound))
	error (_("Could not determine the array high bound"));

      eltlen = TYPE_LENGTH (elttype);
      len = high_bound - low_bound + 1;
      if (options->prettyformat_arrays)
	{
	  print_spaces_filtered (2 + 2 * recurse, stream);
	}

      /* Print arrays of textual chars with a string syntax, as
	 long as the entire array is valid.  */
      if (c_textual_element_type (unresolved_elttype,
				  options->format)
	  && value_bytes_available (original_value, embedded_offset,
				    TYPE_LENGTH (type))
	  && !value_bits_any_optimized_out (original_value,
					    TARGET_CHAR_BIT * embedded_offset,
					    TARGET_CHAR_BIT * TYPE_LENGTH (type)))
	{
	  int force_ellipses = 0;

	  /* If requested, look for the first null char and only
	     print elements up to it.  */
	  if (options->stop_print_at_null)
	    {
	      unsigned int temp_len;

	      for (temp_len = 0;
		   (temp_len < len
		    && temp_len < options->print_max
		    && extract_unsigned_integer (valaddr
						 + embedded_offset * unit_size
						 + temp_len * eltlen,
						 eltlen, byte_order) != 0);
		   ++temp_len)
		;

	      /* Force LA_PRINT_STRING to print ellipses if
		 we've printed the maximum characters and
		 the next character is not \000.  */
	      if (temp_len == options->print_max && temp_len < len)
		{
		  ULONGEST val
		    = extract_unsigned_integer (valaddr
						+ embedded_offset * unit_size
						+ temp_len * eltlen,
						eltlen, byte_order);
		  if (val != 0)
		    force_ellipses = 1;
		}

	      len = temp_len;
	    }

	  LA_PRINT_STRING (stream, unresolved_elttype,
			   valaddr + embedded_offset * unit_size, len,
			   NULL, force_ellipses, options);
	  i = len;
	}
      else
	{
	  fprintf_filtered (stream, "{");
	  /* If this is a virtual function table, print the 0th
	     entry specially, and the rest of the members
	     normally.  */
	  if (cp_is_vtbl_ptr_type (elttype))
	    {
	      i = 1;
	      fprintf_filtered (stream, _("%d vtable entries"),
				len - 1);
	    }
	  else
	    {
	      i = 0;
	    }
	  val_print_array_elements (type, embedded_offset,
				    address, stream,
				    recurse, original_value, options, i);
	  fprintf_filtered (stream, "}");
	}
    }
  else
    {
      /* Array of unspecified length: treat like pointer to first elt.  */
      print_unpacked_pointer (type, elttype, unresolved_elttype, valaddr,
			      embedded_offset, address + embedded_offset,
			      stream, recurse, options);
    }
}

/* c_val_print helper for TYPE_CODE_PTR.  */

static void
c_val_print_ptr (struct type *type, const gdb_byte *valaddr,
		 int embedded_offset, struct ui_file *stream, int recurse,
		 struct value *original_value,
		 const struct value_print_options *options)
{
  struct gdbarch *arch = get_type_arch (type);
  int unit_size = gdbarch_addressable_memory_unit_size (arch);

  if (options->format && options->format != 's')
    {
      val_print_scalar_formatted (type, embedded_offset,
				  original_value, options, 0, stream);
    }
  else if (options->vtblprint && cp_is_vtbl_ptr_type (type))
    {
      /* Print the unmangled name if desired.  */
      /* Print vtable entry - we only get here if we ARE using
	 -fvtable_thunks.  (Otherwise, look under
	 TYPE_CODE_STRUCT.)  */
      CORE_ADDR addr
	= extract_typed_address (valaddr + embedded_offset, type);
      struct gdbarch *gdbarch = get_type_arch (type);

      print_function_pointer_address (options, gdbarch, addr, stream);
    }
  else
    {
      struct type *unresolved_elttype = TYPE_TARGET_TYPE (type);
      struct type *elttype = check_typedef (unresolved_elttype);
      CORE_ADDR addr = unpack_pointer (type,
				       valaddr + embedded_offset * unit_size);

      print_unpacked_pointer (type, elttype, unresolved_elttype, valaddr,
			      embedded_offset, addr, stream, recurse, options);
    }
}

/* c_val_print helper for TYPE_CODE_STRUCT.  */

static void
c_val_print_struct (struct type *type, const gdb_byte *valaddr,
		    int embedded_offset, CORE_ADDR address,
		    struct ui_file *stream, int recurse,
		    struct value *original_value,
		    const struct value_print_options *options)
{
  if (options->vtblprint && cp_is_vtbl_ptr_type (type))
    {
      /* Print the unmangled name if desired.  */
      /* Print vtable entry - we only get here if NOT using
	 -fvtable_thunks.  (Otherwise, look under
	 TYPE_CODE_PTR.)  */
      struct gdbarch *gdbarch = get_type_arch (type);
      int offset = (embedded_offset
		    + TYPE_FIELD_BITPOS (type,
					 VTBL_FNADDR_OFFSET) / 8);
      struct type *field_type = TYPE_FIELD_TYPE (type, VTBL_FNADDR_OFFSET);
      CORE_ADDR addr = extract_typed_address (valaddr + offset, field_type);

      print_function_pointer_address (options, gdbarch, addr, stream);
    }
  else
    cp_print_value_fields_rtti (type, valaddr,
				embedded_offset, address,
				stream, recurse,
				original_value, options,
				NULL, 0);
}

/* c_val_print helper for TYPE_CODE_UNION.  */

static void
c_val_print_union (struct type *type, const gdb_byte *valaddr,
		   int embedded_offset, CORE_ADDR address,
		   struct ui_file *stream, int recurse,
		   struct value *original_value,
		   const struct value_print_options *options)
{
  if (recurse && !options->unionprint)
    {
      fprintf_filtered (stream, "{...}");
     }
  else
    {
      c_val_print_struct (type, valaddr, embedded_offset, address, stream,
			  recurse, original_value, options);
    }
}

/* c_val_print helper for TYPE_CODE_INT.  */

static void
c_val_print_int (struct type *type, struct type *unresolved_type,
		 const gdb_byte *valaddr, int embedded_offset,
		 struct ui_file *stream, struct value *original_value,
		 const struct value_print_options *options)
{
  struct gdbarch *arch = get_type_arch (type);
  int unit_size = gdbarch_addressable_memory_unit_size (arch);

  if (options->format || options->output_format)
    {
      struct value_print_options opts = *options;

      opts.format = (options->format ? options->format
		     : options->output_format);
      val_print_scalar_formatted (type, embedded_offset,
				  original_value, &opts, 0, stream);
    }
  else
    {
      val_print_type_code_int (type, valaddr + embedded_offset * unit_size,
			       stream);
      /* C and C++ has no single byte int type, char is used
	 instead.  Since we don't know whether the value is really
	 intended to be used as an integer or a character, print
	 the character equivalent as well.  */
      if (c_textual_element_type (unresolved_type, options->format))
	{
	  fputs_filtered (" ", stream);
	  LA_PRINT_CHAR (unpack_long (type,
				      valaddr + embedded_offset * unit_size),
			 unresolved_type, stream);
	}
    }
}

/* c_val_print helper for TYPE_CODE_MEMBERPTR.  */

static void
c_val_print_memberptr (struct type *type, const gdb_byte *valaddr,
		       int embedded_offset, CORE_ADDR address,
		       struct ui_file *stream, int recurse,
		       struct value *original_value,
		       const struct value_print_options *options)
{
  if (!options->format)
    {
      cp_print_class_member (valaddr + embedded_offset, type, stream, "&");
    }
  else
    {
      generic_val_print (type, embedded_offset, address, stream,
			 recurse, original_value, options, &c_decorations);
    }
}

/* See val_print for a description of the various parameters of this
   function; they are identical.  */

void
c_val_print (struct type *type,
	     int embedded_offset, CORE_ADDR address,
	     struct ui_file *stream, int recurse,
	     struct value *original_value,
	     const struct value_print_options *options)
{
  struct type *unresolved_type = type;
  const gdb_byte *valaddr = value_contents_for_printing (original_value);

  type = check_typedef (type);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_ARRAY:
      c_val_print_array (type, valaddr, embedded_offset, address, stream,
			 recurse, original_value, options);
      break;

    case TYPE_CODE_METHODPTR:
      cplus_print_method_ptr (valaddr + embedded_offset, type, stream);
      break;

    case TYPE_CODE_PTR:
      c_val_print_ptr (type, valaddr, embedded_offset, stream, recurse,
		       original_value, options);
      break;

    case TYPE_CODE_UNION:
      c_val_print_union (type, valaddr, embedded_offset, address, stream,
			 recurse, original_value, options);
      break;

    case TYPE_CODE_STRUCT:
      c_val_print_struct (type, valaddr, embedded_offset, address, stream,
			  recurse, original_value, options);
      break;

    case TYPE_CODE_INT:
      c_val_print_int (type, unresolved_type, valaddr, embedded_offset, stream,
		       original_value, options);
      break;

    case TYPE_CODE_MEMBERPTR:
      c_val_print_memberptr (type, valaddr, embedded_offset, address, stream,
			     recurse, original_value, options);
      break;

    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_FLT:
    case TYPE_CODE_DECFLOAT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_UNDEF:
    case TYPE_CODE_COMPLEX:
    case TYPE_CODE_CHAR:
    default:
      generic_val_print (type, embedded_offset, address,
			 stream, recurse, original_value, options,
			 &c_decorations);
      break;
    }
  gdb_flush (stream);
}

void
c_value_print (struct value *val, struct ui_file *stream, 
	       const struct value_print_options *options)
{
  struct type *type, *real_type, *val_type;
  int full, using_enc;
  LONGEST top;
  struct value_print_options opts = *options;

  opts.deref_ref = 1;

  /* If it is a pointer, indicate what it points to.

     Print type also if it is a reference.

     C++: if it is a member pointer, we will take care
     of that when we print it.  */

  /* Preserve the original type before stripping typedefs.  We prefer
     to pass down the original type when possible, but for local
     checks it is better to look past the typedefs.  */
  val_type = value_type (val);
  type = check_typedef (val_type);

  if (TYPE_CODE (type) == TYPE_CODE_PTR || TYPE_IS_REFERENCE (type))
    {
      /* Hack:  remove (char *) for char strings.  Their
         type is indicated by the quoted string anyway.
         (Don't use c_textual_element_type here; quoted strings
         are always exactly (char *), (wchar_t *), or the like.  */
      if (TYPE_CODE (val_type) == TYPE_CODE_PTR
	  && TYPE_NAME (val_type) == NULL
	  && TYPE_NAME (TYPE_TARGET_TYPE (val_type)) != NULL
	  && (strcmp (TYPE_NAME (TYPE_TARGET_TYPE (val_type)),
		      "char") == 0
	      || textual_name (TYPE_NAME (TYPE_TARGET_TYPE (val_type)))))
	{
	  /* Print nothing.  */
	}
      else if (options->objectprint
	       && (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_STRUCT))
	{
	  int is_ref = TYPE_IS_REFERENCE (type);
	  enum type_code refcode = TYPE_CODE_UNDEF;

	  if (is_ref)
	    {
	      val = value_addr (val);
	      refcode = TYPE_CODE (type);
	    }

	  /* Pointer to class, check real type of object.  */
	  fprintf_filtered (stream, "(");

	  if (value_entirely_available (val))
	    {
	      real_type = value_rtti_indirect_type (val, &full, &top,
						    &using_enc);
	      if (real_type)
		{
		  /* RTTI entry found.  */
		  type = real_type;

		  /* Need to adjust pointer value.  */
		  val = value_from_pointer (real_type,
					    value_as_address (val) - top);

		  /* Note: When we look up RTTI entries, we don't get
		     any information on const or volatile
		     attributes.  */
		}
	    }

	  if (is_ref)
	    {
	      val = value_ref (value_ind (val), refcode);
	      type = value_type (val);
	    }

	  type_print (type, "", stream, -1);
	  fprintf_filtered (stream, ") ");
	  val_type = type;
	}
      else
	{
	  /* normal case */
	  fprintf_filtered (stream, "(");
	  type_print (value_type (val), "", stream, -1);
	  fprintf_filtered (stream, ") ");
	}
    }

  if (!value_initialized (val))
    fprintf_filtered (stream, " [uninitialized] ");

  if (options->objectprint && (TYPE_CODE (type) == TYPE_CODE_STRUCT))
    {
      /* Attempt to determine real type of object.  */
      real_type = value_rtti_type (val, &full, &top, &using_enc);
      if (real_type)
	{
	  /* We have RTTI information, so use it.  */
	  val = value_full_object (val, real_type, 
				   full, top, using_enc);
	  fprintf_filtered (stream, "(%s%s) ",
			    TYPE_NAME (real_type),
			    full ? "" : _(" [incomplete object]"));
	  /* Print out object: enclosing type is same as real_type if
	     full.  */
	  val_print (value_enclosing_type (val),
		     0,
		     value_address (val), stream, 0,
		     val, &opts, current_language);
	  return;
          /* Note: When we look up RTTI entries, we don't get any
             information on const or volatile attributes.  */
	}
      else if (type != check_typedef (value_enclosing_type (val)))
	{
	  /* No RTTI information, so let's do our best.  */
	  fprintf_filtered (stream, "(%s ?) ",
			    TYPE_NAME (value_enclosing_type (val)));
	  val_print (value_enclosing_type (val),
		     0,
		     value_address (val), stream, 0,
		     val, &opts, current_language);
	  return;
	}
      /* Otherwise, we end up at the return outside this "if".  */
    }

  val_print (val_type,
	     value_embedded_offset (val),
	     value_address (val),
	     stream, 0,
	     val, &opts, current_language);
}
