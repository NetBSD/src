/* Support for printing Modula 2 values for GDB, the GNU debugger.

   Copyright (C) 1986, 1988, 1989, 1991, 1992, 1996, 1998, 2000, 2005, 2006,
                 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.

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
#include "typeprint.h"
#include "c-lang.h"
#include "m2-lang.h"
#include "target.h"

static int print_unpacked_pointer (struct type *type,
				   CORE_ADDR address, CORE_ADDR addr,
				   const struct value_print_options *options,
				   struct ui_file *stream);
static void
m2_print_array_contents (struct type *type, const gdb_byte *valaddr,
			 int embedded_offset, CORE_ADDR address,
			 struct ui_file *stream, int recurse,
			 const struct value *val,
			 const struct value_print_options *options,
			 int len);


/* Print function pointer with inferior address ADDRESS onto stdio
   stream STREAM.  */

static void
print_function_pointer_address (struct gdbarch *gdbarch, CORE_ADDR address,
				struct ui_file *stream, int addressprint)
{
  CORE_ADDR func_addr = gdbarch_convert_from_func_ptr_addr (gdbarch, address,
							    &current_target);

  /* If the function pointer is represented by a description, print the
     address of the description.  */
  if (addressprint && func_addr != address)
    {
      fputs_filtered ("@", stream);
      fputs_filtered (paddress (gdbarch, address), stream);
      fputs_filtered (": ", stream);
    }
  print_address_demangle (gdbarch, func_addr, stream, demangle);
}

/* get_long_set_bounds - assigns the bounds of the long set to low and
                         high.  */

int
get_long_set_bounds (struct type *type, LONGEST *low, LONGEST *high)
{
  int len, i;

  if (TYPE_CODE (type) == TYPE_CODE_STRUCT)
    {
      len = TYPE_NFIELDS (type);
      i = TYPE_N_BASECLASSES (type);
      if (len == 0)
	return 0;
      *low = TYPE_LOW_BOUND (TYPE_INDEX_TYPE (TYPE_FIELD_TYPE (type, i)));
      *high = TYPE_HIGH_BOUND (TYPE_INDEX_TYPE (TYPE_FIELD_TYPE (type,
								 len-1)));
      return 1;
    }
  error (_("expecting long_set"));
  return 0;
}

static void
m2_print_long_set (struct type *type, const gdb_byte *valaddr,
		   int embedded_offset, CORE_ADDR address,
		   struct ui_file *stream)
{
  int empty_set        = 1;
  int element_seen     = 0;
  LONGEST previous_low = 0;
  LONGEST previous_high= 0;
  LONGEST i, low_bound, high_bound;
  LONGEST field_low, field_high;
  struct type *range;
  int len, field;
  struct type *target;
  int bitval;

  CHECK_TYPEDEF (type);

  fprintf_filtered (stream, "{");
  len = TYPE_NFIELDS (type);
  if (get_long_set_bounds (type, &low_bound, &high_bound))
    {
      field = TYPE_N_BASECLASSES (type);
      range = TYPE_INDEX_TYPE (TYPE_FIELD_TYPE (type, field));
    }
  else
    {
      fprintf_filtered (stream, " %s }", _("<unknown bounds of set>"));
      return;
    }

  target = TYPE_TARGET_TYPE (range);

  if (get_discrete_bounds (range, &field_low, &field_high) >= 0)
    {
      for (i = low_bound; i <= high_bound; i++)
	{
	  bitval = value_bit_index (TYPE_FIELD_TYPE (type, field),
				    (TYPE_FIELD_BITPOS (type, field) / 8) +
				    valaddr + embedded_offset, i);
	  if (bitval < 0)
	    error (_("bit test is out of range"));
	  else if (bitval > 0)
	    {
	      previous_high = i;
	      if (! element_seen)
		{
		  if (! empty_set)
		    fprintf_filtered (stream, ", ");
		  print_type_scalar (target, i, stream);
		  empty_set    = 0;
		  element_seen = 1;
		  previous_low = i;
		}
	    }
	  else
	    {
	      /* bit is not set */
	      if (element_seen)
		{
		  if (previous_low+1 < previous_high)
		    fprintf_filtered (stream, "..");
		  if (previous_low+1 < previous_high)
		    print_type_scalar (target, previous_high, stream);
		  element_seen = 0;
		}
	    }
	  if (i == field_high)
	    {
	      field++;
	      if (field == len)
		break;
	      range = TYPE_INDEX_TYPE (TYPE_FIELD_TYPE (type, field));
	      if (get_discrete_bounds (range, &field_low, &field_high) < 0)
		break;
	      target = TYPE_TARGET_TYPE (range);
	    }
	}
      if (element_seen)
	{
	  if (previous_low+1 < previous_high)
	    {
	      fprintf_filtered (stream, "..");
	      print_type_scalar (target, previous_high, stream);
	    }
	  element_seen = 0;
	}
      fprintf_filtered (stream, "}");
    }
}

static void
m2_print_unbounded_array (struct type *type, const gdb_byte *valaddr,
			  int embedded_offset, CORE_ADDR address,
			  struct ui_file *stream, int recurse,
			  const struct value_print_options *options)
{
  struct type *content_type;
  CORE_ADDR addr;
  LONGEST len;
  struct value *val;

  CHECK_TYPEDEF (type);
  content_type = TYPE_TARGET_TYPE (TYPE_FIELD_TYPE (type, 0));

  addr = unpack_pointer (TYPE_FIELD_TYPE (type, 0),
			 (TYPE_FIELD_BITPOS (type, 0) / 8) +
			 valaddr + embedded_offset);

  val = value_at_lazy (TYPE_TARGET_TYPE (TYPE_FIELD_TYPE (type, 0)),
		       addr);
  len = unpack_field_as_long (type, valaddr + embedded_offset, 1);

  fprintf_filtered (stream, "{");  
  m2_print_array_contents (value_type (val),
			   value_contents_for_printing (val),
			   value_embedded_offset (val), addr, stream,
			   recurse, val, options, len);
  fprintf_filtered (stream, ", HIGH = %d}", (int) len);
}

static int
print_unpacked_pointer (struct type *type,
			CORE_ADDR address, CORE_ADDR addr,
			const struct value_print_options *options,
			struct ui_file *stream)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  struct type *elttype = check_typedef (TYPE_TARGET_TYPE (type));

  if (TYPE_CODE (elttype) == TYPE_CODE_FUNC)
    {
      /* Try to print what function it points to.  */
      print_function_pointer_address (gdbarch, addr, stream,
				      options->addressprint);
      /* Return value is irrelevant except for string pointers.  */
      return 0;
    }

  if (options->addressprint && options->format != 's')
    fputs_filtered (paddress (gdbarch, address), stream);

  /* For a pointer to char or unsigned char, also print the string
     pointed to, unless pointer is null.  */

  if (TYPE_LENGTH (elttype) == 1
      && TYPE_CODE (elttype) == TYPE_CODE_INT
      && (options->format == 0 || options->format == 's')
      && addr != 0)
    return val_print_string (TYPE_TARGET_TYPE (type), NULL, addr, -1,
			     stream, options);
  
  return 0;
}

static void
print_variable_at_address (struct type *type,
			   const gdb_byte *valaddr,
			   struct ui_file *stream,
			   int recurse,
			   const struct value_print_options *options)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  CORE_ADDR addr = unpack_pointer (type, valaddr);
  struct type *elttype = check_typedef (TYPE_TARGET_TYPE (type));

  fprintf_filtered (stream, "[");
  fputs_filtered (paddress (gdbarch, addr), stream);
  fprintf_filtered (stream, "] : ");
  
  if (TYPE_CODE (elttype) != TYPE_CODE_UNDEF)
    {
      struct value *deref_val =
	value_at (TYPE_TARGET_TYPE (type), unpack_pointer (type, valaddr));

      common_val_print (deref_val, stream, recurse, options, current_language);
    }
  else
    fputs_filtered ("???", stream);
}


/* m2_print_array_contents - prints out the contents of an
                             array up to a max_print values.
                             It prints arrays of char as a string
                             and all other data types as comma
                             separated values.  */

static void
m2_print_array_contents (struct type *type, const gdb_byte *valaddr,
			 int embedded_offset, CORE_ADDR address,
			 struct ui_file *stream, int recurse,
			 const struct value *val,
			 const struct value_print_options *options,
			 int len)
{
  int eltlen;
  CHECK_TYPEDEF (type);

  if (TYPE_LENGTH (type) > 0)
    {
      eltlen = TYPE_LENGTH (type);
      if (options->prettyprint_arrays)
	print_spaces_filtered (2 + 2 * recurse, stream);
      /* For an array of chars, print with string syntax.  */
      if (eltlen == 1 &&
	  ((TYPE_CODE (type) == TYPE_CODE_INT)
	   || ((current_language->la_language == language_m2)
	       && (TYPE_CODE (type) == TYPE_CODE_CHAR)))
	  && (options->format == 0 || options->format == 's'))
	val_print_string (type, NULL, address, len+1, stream, options);
      else
	{
	  fprintf_filtered (stream, "{");
	  val_print_array_elements (type, valaddr, embedded_offset,
				    address, stream, recurse, val,
				    options, 0);
	  fprintf_filtered (stream, "}");
	}
    }
}


/* See val_print for a description of the various parameters of this
   function; they are identical.  The semantics of the return value is
   also identical to val_print.  */

int
m2_val_print (struct type *type, const gdb_byte *valaddr, int embedded_offset,
	      CORE_ADDR address, struct ui_file *stream, int recurse,
	      const struct value *original_value,
	      const struct value_print_options *options)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  unsigned int i = 0;	/* Number of characters printed.  */
  unsigned len;
  struct type *elttype;
  unsigned eltlen;
  LONGEST val;
  CORE_ADDR addr;

  CHECK_TYPEDEF (type);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_ARRAY:
      if (TYPE_LENGTH (type) > 0 && TYPE_LENGTH (TYPE_TARGET_TYPE (type)) > 0)
	{
	  elttype = check_typedef (TYPE_TARGET_TYPE (type));
	  eltlen = TYPE_LENGTH (elttype);
	  len = TYPE_LENGTH (type) / eltlen;
	  if (options->prettyprint_arrays)
	    print_spaces_filtered (2 + 2 * recurse, stream);
	  /* For an array of chars, print with string syntax.  */
	  if (eltlen == 1 &&
	      ((TYPE_CODE (elttype) == TYPE_CODE_INT)
	       || ((current_language->la_language == language_m2)
		   && (TYPE_CODE (elttype) == TYPE_CODE_CHAR)))
	      && (options->format == 0 || options->format == 's'))
	    {
	      /* If requested, look for the first null char and only print
	         elements up to it.  */
	      if (options->stop_print_at_null)
		{
		  unsigned int temp_len;

		  /* Look for a NULL char.  */
		  for (temp_len = 0;
		       (valaddr + embedded_offset)[temp_len]
			 && temp_len < len && temp_len < options->print_max;
		       temp_len++);
		  len = temp_len;
		}

	      LA_PRINT_STRING (stream, TYPE_TARGET_TYPE (type),
			       valaddr + embedded_offset, len, NULL,
			       0, options);
	      i = len;
	    }
	  else
	    {
	      fprintf_filtered (stream, "{");
	      val_print_array_elements (type, valaddr, embedded_offset,
					address, stream,
					recurse, original_value,
					options, 0);
	      fprintf_filtered (stream, "}");
	    }
	  break;
	}
      /* Array of unspecified length: treat like pointer to first elt.  */
      print_unpacked_pointer (type, address, address, options, stream);
      break;

    case TYPE_CODE_PTR:
      if (TYPE_CONST (type))
	print_variable_at_address (type, valaddr + embedded_offset,
				   stream, recurse, options);
      else if (options->format && options->format != 's')
	val_print_scalar_formatted (type, valaddr, embedded_offset,
				    original_value, options, 0, stream);
      else
	{
	  addr = unpack_pointer (type, valaddr + embedded_offset);
	  print_unpacked_pointer (type, addr, address, options, stream);
	}
      break;

    case TYPE_CODE_REF:
      elttype = check_typedef (TYPE_TARGET_TYPE (type));
      if (options->addressprint)
	{
	  CORE_ADDR addr
	    = extract_typed_address (valaddr + embedded_offset, type);

	  fprintf_filtered (stream, "@");
	  fputs_filtered (paddress (gdbarch, addr), stream);
	  if (options->deref_ref)
	    fputs_filtered (": ", stream);
	}
      /* De-reference the reference.  */
      if (options->deref_ref)
	{
	  if (TYPE_CODE (elttype) != TYPE_CODE_UNDEF)
	    {
	      struct value *deref_val =
		value_at
		(TYPE_TARGET_TYPE (type),
		 unpack_pointer (type, valaddr + embedded_offset));

	      common_val_print (deref_val, stream, recurse, options,
				current_language);
	    }
	  else
	    fputs_filtered ("???", stream);
	}
      break;

    case TYPE_CODE_UNION:
      if (recurse && !options->unionprint)
	{
	  fprintf_filtered (stream, "{...}");
	  break;
	}
      /* Fall through.  */
    case TYPE_CODE_STRUCT:
      if (m2_is_long_set (type))
	m2_print_long_set (type, valaddr, embedded_offset, address,
			   stream);
      else if (m2_is_unbounded_array (type))
	m2_print_unbounded_array (type, valaddr, embedded_offset,
				  address, stream, recurse, options);
      else
	cp_print_value_fields (type, type, valaddr, embedded_offset,
			       address, stream, recurse, original_value,
			       options, NULL, 0);
      break;

    case TYPE_CODE_ENUM:
      if (options->format)
	{
	  val_print_scalar_formatted (type, valaddr, embedded_offset,
				      original_value, options, 0, stream);
	  break;
	}
      len = TYPE_NFIELDS (type);
      val = unpack_long (type, valaddr + embedded_offset);
      for (i = 0; i < len; i++)
	{
	  QUIT;
	  if (val == TYPE_FIELD_BITPOS (type, i))
	    {
	      break;
	    }
	}
      if (i < len)
	{
	  fputs_filtered (TYPE_FIELD_NAME (type, i), stream);
	}
      else
	{
	  print_longest (stream, 'd', 0, val);
	}
      break;

    case TYPE_CODE_FUNC:
      if (options->format)
	{
	  val_print_scalar_formatted (type, valaddr, embedded_offset,
				      original_value, options, 0, stream);
	  break;
	}
      /* FIXME, we should consider, at least for ANSI C language, eliminating
         the distinction made between FUNCs and POINTERs to FUNCs.  */
      fprintf_filtered (stream, "{");
      type_print (type, "", stream, -1);
      fprintf_filtered (stream, "} ");
      /* Try to print what function it points to, and its address.  */
      print_address_demangle (gdbarch, address, stream, demangle);
      break;

    case TYPE_CODE_BOOL:
      if (options->format || options->output_format)
	{
	  struct value_print_options opts = *options;

	  opts.format = (options->format ? options->format
			 : options->output_format);
	  val_print_scalar_formatted (type, valaddr, embedded_offset,
				      original_value, &opts, 0, stream);
	}
      else
	{
	  val = unpack_long (type, valaddr + embedded_offset);
	  if (val == 0)
	    fputs_filtered ("FALSE", stream);
	  else if (val == 1)
	    fputs_filtered ("TRUE", stream);
	  else
	    fprintf_filtered (stream, "%ld)", (long int) val);
	}
      break;

    case TYPE_CODE_RANGE:
      if (TYPE_LENGTH (type) == TYPE_LENGTH (TYPE_TARGET_TYPE (type)))
	{
	  m2_val_print (TYPE_TARGET_TYPE (type), valaddr, embedded_offset,
			address, stream, recurse, original_value, options);
	  break;
	}
      /* FIXME: create_range_type does not set the unsigned bit in a
         range type (I think it probably should copy it from the target
         type), so we won't print values which are too large to
         fit in a signed integer correctly.  */
      /* FIXME: Doesn't handle ranges of enums correctly.  (Can't just
         print with the target type, though, because the size of our type
         and the target type might differ).  */
      /* FALLTHROUGH */

    case TYPE_CODE_INT:
      if (options->format || options->output_format)
	{
	  struct value_print_options opts = *options;

	  opts.format = (options->format ? options->format
			 : options->output_format);
	  val_print_scalar_formatted (type, valaddr, embedded_offset,
				      original_value, &opts, 0, stream);
	}
      else
	val_print_type_code_int (type, valaddr + embedded_offset, stream);
      break;

    case TYPE_CODE_CHAR:
      if (options->format || options->output_format)
	{
	  struct value_print_options opts = *options;

	  opts.format = (options->format ? options->format
			 : options->output_format);
	  val_print_scalar_formatted (type, valaddr, embedded_offset,
				      original_value, &opts, 0, stream);
	}
      else
	{
	  val = unpack_long (type, valaddr + embedded_offset);
	  if (TYPE_UNSIGNED (type))
	    fprintf_filtered (stream, "%u", (unsigned int) val);
	  else
	    fprintf_filtered (stream, "%d", (int) val);
	  fputs_filtered (" ", stream);
	  LA_PRINT_CHAR ((unsigned char) val, type, stream);
	}
      break;

    case TYPE_CODE_FLT:
      if (options->format)
	val_print_scalar_formatted (type, valaddr, embedded_offset,
				    original_value, options, 0, stream);
      else
	print_floating (valaddr + embedded_offset, type, stream);
      break;

    case TYPE_CODE_METHOD:
      break;

    case TYPE_CODE_BITSTRING:
    case TYPE_CODE_SET:
      elttype = TYPE_INDEX_TYPE (type);
      CHECK_TYPEDEF (elttype);
      if (TYPE_STUB (elttype))
	{
	  fprintf_filtered (stream, _("<incomplete type>"));
	  gdb_flush (stream);
	  break;
	}
      else
	{
	  struct type *range = elttype;
	  LONGEST low_bound, high_bound;
	  int i;
	  int is_bitstring = TYPE_CODE (type) == TYPE_CODE_BITSTRING;
	  int need_comma = 0;

	  if (is_bitstring)
	    fputs_filtered ("B'", stream);
	  else
	    fputs_filtered ("{", stream);

	  i = get_discrete_bounds (range, &low_bound, &high_bound);
	maybe_bad_bstring:
	  if (i < 0)
	    {
	      fputs_filtered (_("<error value>"), stream);
	      goto done;
	    }

	  for (i = low_bound; i <= high_bound; i++)
	    {
	      int element = value_bit_index (type, valaddr + embedded_offset,
					     i);

	      if (element < 0)
		{
		  i = element;
		  goto maybe_bad_bstring;
		}
	      if (is_bitstring)
		fprintf_filtered (stream, "%d", element);
	      else if (element)
		{
		  if (need_comma)
		    fputs_filtered (", ", stream);
		  print_type_scalar (range, i, stream);
		  need_comma = 1;

		  if (i + 1 <= high_bound
		      && value_bit_index (type, valaddr + embedded_offset,
					  ++i))
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
	  if (is_bitstring)
	    fputs_filtered ("'", stream);
	  else
	    fputs_filtered ("}", stream);
	}
      break;

    case TYPE_CODE_VOID:
      fprintf_filtered (stream, "void");
      break;

    case TYPE_CODE_ERROR:
      fprintf_filtered (stream, "%s", TYPE_ERROR_NAME (type));
      break;

    case TYPE_CODE_UNDEF:
      /* This happens (without TYPE_FLAG_STUB set) on systems which don't use
         dbx xrefs (NO_DBX_XREFS in gcc) if a file has a "struct foo *bar"
         and no complete type for struct foo in that file.  */
      fprintf_filtered (stream, _("<incomplete type>"));
      break;

    default:
      error (_("Invalid m2 type code %d in symbol table."), TYPE_CODE (type));
    }
  gdb_flush (stream);
  return (0);
}
