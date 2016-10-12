/* Support for printing Modula 2 values for GDB, the GNU debugger.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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

  type = check_typedef (type);

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
  CORE_ADDR addr;
  LONGEST len;
  struct value *val;

  type = check_typedef (type);

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
  int want_space = 0;

  if (TYPE_CODE (elttype) == TYPE_CODE_FUNC)
    {
      /* Try to print what function it points to.  */
      print_function_pointer_address (options, gdbarch, addr, stream);
      /* Return value is irrelevant except for string pointers.  */
      return 0;
    }

  if (options->addressprint && options->format != 's')
    {
      fputs_filtered (paddress (gdbarch, address), stream);
      want_space = 1;
    }

  /* For a pointer to char or unsigned char, also print the string
     pointed to, unless pointer is null.  */

  if (TYPE_LENGTH (elttype) == 1
      && TYPE_CODE (elttype) == TYPE_CODE_INT
      && (options->format == 0 || options->format == 's')
      && addr != 0)
    {
      if (want_space)
	fputs_filtered (" ", stream);
      return val_print_string (TYPE_TARGET_TYPE (type), NULL, addr, -1,
			       stream, options);
    }
  
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
  type = check_typedef (type);

  if (TYPE_LENGTH (type) > 0)
    {
      if (options->prettyformat_arrays)
	print_spaces_filtered (2 + 2 * recurse, stream);
      /* For an array of chars, print with string syntax.  */
      if (TYPE_LENGTH (type) == 1 &&
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

/* Decorations for Modula 2.  */

static const struct generic_val_print_decorations m2_decorations =
{
  "",
  " + ",
  " * I",
  "TRUE",
  "FALSE",
  "void",
  "{",
  "}"
};

/* See val_print for a description of the various parameters of this
   function; they are identical.  */

void
m2_val_print (struct type *type, const gdb_byte *valaddr, int embedded_offset,
	      CORE_ADDR address, struct ui_file *stream, int recurse,
	      const struct value *original_value,
	      const struct value_print_options *options)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  unsigned len;
  struct type *elttype;
  CORE_ADDR addr;

  type = check_typedef (type);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_ARRAY:
      if (TYPE_LENGTH (type) > 0 && TYPE_LENGTH (TYPE_TARGET_TYPE (type)) > 0)
	{
	  elttype = check_typedef (TYPE_TARGET_TYPE (type));
	  len = TYPE_LENGTH (type) / TYPE_LENGTH (elttype);
	  if (options->prettyformat_arrays)
	    print_spaces_filtered (2 + 2 * recurse, stream);
	  /* For an array of chars, print with string syntax.  */
	  if (TYPE_LENGTH (elttype) == 1 &&
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

    case TYPE_CODE_SET:
      elttype = TYPE_INDEX_TYPE (type);
      elttype = check_typedef (elttype);
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
	  int need_comma = 0;

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
	      if (element)
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
	  fputs_filtered ("}", stream);
	}
      break;

    case TYPE_CODE_RANGE:
      if (TYPE_LENGTH (type) == TYPE_LENGTH (TYPE_TARGET_TYPE (type)))
	{
	  m2_val_print (TYPE_TARGET_TYPE (type), valaddr, embedded_offset,
			address, stream, recurse, original_value, options);
	  break;
	}
      /* FIXME: create_static_range_type does not set the unsigned bit in a
         range type (I think it probably should copy it from the target
         type), so we won't print values which are too large to
         fit in a signed integer correctly.  */
      /* FIXME: Doesn't handle ranges of enums correctly.  (Can't just
         print with the target type, though, because the size of our type
         and the target type might differ).  */
      /* FALLTHROUGH */

    case TYPE_CODE_REF:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_UNDEF:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
    default:
      generic_val_print (type, valaddr, embedded_offset, address,
			 stream, recurse, original_value, options,
			 &m2_decorations);
      break;
    }
  gdb_flush (stream);
}
