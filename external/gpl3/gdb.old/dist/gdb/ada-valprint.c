/* Support for printing Ada values for GDB, the GNU debugger.

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
#include <ctype.h>
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "demangle.h"
#include "valprint.h"
#include "language.h"
#include "annotate.h"
#include "ada-lang.h"
#include "c-lang.h"
#include "infcall.h"
#include "objfiles.h"

static int print_field_values (struct type *, const gdb_byte *,
			       int,
			       struct ui_file *, int,
			       struct value *,
			       const struct value_print_options *,
			       int, struct type *, int,
			       const struct language_defn *);


/* Make TYPE unsigned if its range of values includes no negatives.  */
static void
adjust_type_signedness (struct type *type)
{
  if (type != NULL && TYPE_CODE (type) == TYPE_CODE_RANGE
      && TYPE_LOW_BOUND (type) >= 0)
    TYPE_UNSIGNED (type) = 1;
}

/* Assuming TYPE is a simple array type, prints its lower bound on STREAM,
   if non-standard (i.e., other than 1 for numbers, other than lower bound
   of index type for enumerated type).  Returns 1 if something printed,
   otherwise 0.  */

static int
print_optional_low_bound (struct ui_file *stream, struct type *type,
			  const struct value_print_options *options)
{
  struct type *index_type;
  LONGEST low_bound;
  LONGEST high_bound;

  if (options->print_array_indexes)
    return 0;

  if (!get_array_bounds (type, &low_bound, &high_bound))
    return 0;

  /* If this is an empty array, then don't print the lower bound.
     That would be confusing, because we would print the lower bound,
     followed by... nothing!  */
  if (low_bound > high_bound)
    return 0;

  index_type = TYPE_INDEX_TYPE (type);

  while (TYPE_CODE (index_type) == TYPE_CODE_RANGE)
    {
      /* We need to know what the base type is, in order to do the
         appropriate check below.  Otherwise, if this is a subrange
         of an enumerated type, where the underlying value of the
         first element is typically 0, we might test the low bound
         against the wrong value.  */
      index_type = TYPE_TARGET_TYPE (index_type);
    }

  switch (TYPE_CODE (index_type))
    {
    case TYPE_CODE_BOOL:
      if (low_bound == 0)
	return 0;
      break;
    case TYPE_CODE_ENUM:
      if (low_bound == TYPE_FIELD_ENUMVAL (index_type, 0))
	return 0;
      break;
    case TYPE_CODE_UNDEF:
      index_type = NULL;
      /* FALL THROUGH */
    default:
      if (low_bound == 1)
	return 0;
      break;
    }

  ada_print_scalar (index_type, low_bound, stream);
  fprintf_filtered (stream, " => ");
  return 1;
}

/*  Version of val_print_array_elements for GNAT-style packed arrays.
    Prints elements of packed array of type TYPE at bit offset
    BITOFFSET from VALADDR on STREAM.  Formats according to OPTIONS and
    separates with commas.  RECURSE is the recursion (nesting) level.
    TYPE must have been decoded (as by ada_coerce_to_simple_array).  */

static void
val_print_packed_array_elements (struct type *type, const gdb_byte *valaddr,
				 int offset,
				 int bitoffset, struct ui_file *stream,
				 int recurse,
				 struct value *val,
				 const struct value_print_options *options)
{
  unsigned int i;
  unsigned int things_printed = 0;
  unsigned len;
  struct type *elttype, *index_type;
  unsigned long bitsize = TYPE_FIELD_BITSIZE (type, 0);
  struct value *mark = value_mark ();
  LONGEST low = 0;

  elttype = TYPE_TARGET_TYPE (type);
  index_type = TYPE_INDEX_TYPE (type);

  {
    LONGEST high;

    if (get_discrete_bounds (index_type, &low, &high) < 0)
      len = 1;
    else
      len = high - low + 1;
  }

  i = 0;
  annotate_array_section_begin (i, elttype);

  while (i < len && things_printed < options->print_max)
    {
      struct value *v0, *v1;
      int i0;

      if (i != 0)
	{
	  if (options->prettyformat_arrays)
	    {
	      fprintf_filtered (stream, ",\n");
	      print_spaces_filtered (2 + 2 * recurse, stream);
	    }
	  else
	    {
	      fprintf_filtered (stream, ", ");
	    }
	}
      wrap_here (n_spaces (2 + 2 * recurse));
      maybe_print_array_index (index_type, i + low, stream, options);

      i0 = i;
      v0 = ada_value_primitive_packed_val (NULL, valaddr + offset,
					   (i0 * bitsize) / HOST_CHAR_BIT,
					   (i0 * bitsize) % HOST_CHAR_BIT,
					   bitsize, elttype);
      while (1)
	{
	  i += 1;
	  if (i >= len)
	    break;
	  v1 = ada_value_primitive_packed_val (NULL, valaddr + offset,
					       (i * bitsize) / HOST_CHAR_BIT,
					       (i * bitsize) % HOST_CHAR_BIT,
					       bitsize, elttype);
	  if (TYPE_LENGTH (check_typedef (value_type (v0)))
	      != TYPE_LENGTH (check_typedef (value_type (v1))))
	    break;
	  if (!value_contents_eq (v0, value_embedded_offset (v0),
				  v1, value_embedded_offset (v1),
				  TYPE_LENGTH (check_typedef (value_type (v0)))))
	    break;
	}

      if (i - i0 > options->repeat_count_threshold)
	{
	  struct value_print_options opts = *options;

	  opts.deref_ref = 0;
	  val_print (elttype,
		     value_embedded_offset (v0), 0, stream,
		     recurse + 1, v0, &opts, current_language);
	  annotate_elt_rep (i - i0);
	  fprintf_filtered (stream, _(" <repeats %u times>"), i - i0);
	  annotate_elt_rep_end ();

	}
      else
	{
	  int j;
	  struct value_print_options opts = *options;

	  opts.deref_ref = 0;
	  for (j = i0; j < i; j += 1)
	    {
	      if (j > i0)
		{
		  if (options->prettyformat_arrays)
		    {
		      fprintf_filtered (stream, ",\n");
		      print_spaces_filtered (2 + 2 * recurse, stream);
		    }
		  else
		    {
		      fprintf_filtered (stream, ", ");
		    }
		  wrap_here (n_spaces (2 + 2 * recurse));
		  maybe_print_array_index (index_type, j + low,
					   stream, options);
		}
	      val_print (elttype,
			 value_embedded_offset (v0), 0, stream,
			 recurse + 1, v0, &opts, current_language);
	      annotate_elt ();
	    }
	}
      things_printed += i - i0;
    }
  annotate_array_section_end ();
  if (i < len)
    {
      fprintf_filtered (stream, "...");
    }

  value_free_to_mark (mark);
}

static struct type *
printable_val_type (struct type *type, const gdb_byte *valaddr)
{
  return ada_to_fixed_type (ada_aligned_type (type), valaddr, 0, NULL, 1);
}

/* Print the character C on STREAM as part of the contents of a literal
   string whose delimiter is QUOTER.  TYPE_LEN is the length in bytes
   of the character.  */

void
ada_emit_char (int c, struct type *type, struct ui_file *stream,
	       int quoter, int type_len)
{
  /* If this character fits in the normal ASCII range, and is
     a printable character, then print the character as if it was
     an ASCII character, even if this is a wide character.
     The UCHAR_MAX check is necessary because the isascii function
     requires that its argument have a value of an unsigned char,
     or EOF (EOF is obviously not printable).  */
  if (c <= UCHAR_MAX && isascii (c) && isprint (c))
    {
      if (c == quoter && c == '"')
	fprintf_filtered (stream, "\"\"");
      else
	fprintf_filtered (stream, "%c", c);
    }
  else
    fprintf_filtered (stream, "[\"%0*x\"]", type_len * 2, c);
}

/* Character #I of STRING, given that TYPE_LEN is the size in bytes
   of a character.  */

static int
char_at (const gdb_byte *string, int i, int type_len,
	 enum bfd_endian byte_order)
{
  if (type_len == 1)
    return string[i];
  else
    return (int) extract_unsigned_integer (string + type_len * i,
                                           type_len, byte_order);
}

/* Print a floating-point value of type TYPE, pointed to in GDB by
   VALADDR, on STREAM.  Use Ada formatting conventions: there must be
   a decimal point, and at least one digit before and after the
   point.  We use the GNAT format for NaNs and infinities.  */

static void
ada_print_floating (const gdb_byte *valaddr, struct type *type,
		    struct ui_file *stream)
{
  string_file tmp_stream;

  print_floating (valaddr, type, &tmp_stream);

  std::string &s = tmp_stream.string ();
  size_t skip_count = 0;

  /* Modify for Ada rules.  */

  size_t pos = s.find ("inf");
  if (pos == std::string::npos)
    pos = s.find ("Inf");
  if (pos == std::string::npos)
    pos = s.find ("INF");
  if (pos != std::string::npos)
    s.replace (pos, 3, "Inf");

  if (pos == std::string::npos)
    {
      pos = s.find ("nan");
      if (pos == std::string::npos)
	pos = s.find ("NaN");
      if (pos == std::string::npos)
	pos = s.find ("Nan");
      if (pos != std::string::npos)
	{
	  s[pos] = s[pos + 2] = 'N';
	  if (s[0] == '-')
	    skip_count = 1;
	}
    }

  if (pos == std::string::npos
      && s.find ('.') == std::string::npos)
    {
      pos = s.find ('e');
      if (pos == std::string::npos)
	fprintf_filtered (stream, "%s.0", s.c_str ());
      else
	fprintf_filtered (stream, "%.*s.0%s", (int) pos, s.c_str (), &s[pos]);
    }
  else
    fprintf_filtered (stream, "%s", &s[skip_count]);
}

void
ada_printchar (int c, struct type *type, struct ui_file *stream)
{
  fputs_filtered ("'", stream);
  ada_emit_char (c, type, stream, '\'', TYPE_LENGTH (type));
  fputs_filtered ("'", stream);
}

/* [From print_type_scalar in typeprint.c].   Print VAL on STREAM in a
   form appropriate for TYPE, if non-NULL.  If TYPE is NULL, print VAL
   like a default signed integer.  */

void
ada_print_scalar (struct type *type, LONGEST val, struct ui_file *stream)
{
  unsigned int i;
  unsigned len;

  if (!type)
    {
      print_longest (stream, 'd', 0, val);
      return;
    }

  type = ada_check_typedef (type);

  switch (TYPE_CODE (type))
    {

    case TYPE_CODE_ENUM:
      len = TYPE_NFIELDS (type);
      for (i = 0; i < len; i++)
	{
	  if (TYPE_FIELD_ENUMVAL (type, i) == val)
	    {
	      break;
	    }
	}
      if (i < len)
	{
	  fputs_filtered (ada_enum_name (TYPE_FIELD_NAME (type, i)), stream);
	}
      else
	{
	  print_longest (stream, 'd', 0, val);
	}
      break;

    case TYPE_CODE_INT:
      print_longest (stream, TYPE_UNSIGNED (type) ? 'u' : 'd', 0, val);
      break;

    case TYPE_CODE_CHAR:
      LA_PRINT_CHAR (val, type, stream);
      break;

    case TYPE_CODE_BOOL:
      fprintf_filtered (stream, val ? "true" : "false");
      break;

    case TYPE_CODE_RANGE:
      ada_print_scalar (TYPE_TARGET_TYPE (type), val, stream);
      return;

    case TYPE_CODE_UNDEF:
    case TYPE_CODE_PTR:
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_SET:
    case TYPE_CODE_STRING:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_MEMBERPTR:
    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_REF:
      warning (_("internal error: unhandled type in ada_print_scalar"));
      break;

    default:
      error (_("Invalid type code in symbol table."));
    }
  gdb_flush (stream);
}

/* Print the character string STRING, printing at most LENGTH characters.
   Printing stops early if the number hits print_max; repeat counts
   are printed as appropriate.  Print ellipses at the end if we
   had to stop before printing LENGTH characters, or if FORCE_ELLIPSES.
   TYPE_LEN is the length (1 or 2) of the character type.  */

static void
printstr (struct ui_file *stream, struct type *elttype, const gdb_byte *string,
	  unsigned int length, int force_ellipses, int type_len,
	  const struct value_print_options *options)
{
  enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (elttype));
  unsigned int i;
  unsigned int things_printed = 0;
  int in_quotes = 0;
  int need_comma = 0;

  if (length == 0)
    {
      fputs_filtered ("\"\"", stream);
      return;
    }

  for (i = 0; i < length && things_printed < options->print_max; i += 1)
    {
      /* Position of the character we are examining
         to see whether it is repeated.  */
      unsigned int rep1;
      /* Number of repetitions we have detected so far.  */
      unsigned int reps;

      QUIT;

      if (need_comma)
	{
	  fputs_filtered (", ", stream);
	  need_comma = 0;
	}

      rep1 = i + 1;
      reps = 1;
      while (rep1 < length
	     && char_at (string, rep1, type_len, byte_order)
		== char_at (string, i, type_len, byte_order))
	{
	  rep1 += 1;
	  reps += 1;
	}

      if (reps > options->repeat_count_threshold)
	{
	  if (in_quotes)
	    {
	      fputs_filtered ("\", ", stream);
	      in_quotes = 0;
	    }
	  fputs_filtered ("'", stream);
	  ada_emit_char (char_at (string, i, type_len, byte_order),
			 elttype, stream, '\'', type_len);
	  fputs_filtered ("'", stream);
	  fprintf_filtered (stream, _(" <repeats %u times>"), reps);
	  i = rep1 - 1;
	  things_printed += options->repeat_count_threshold;
	  need_comma = 1;
	}
      else
	{
	  if (!in_quotes)
	    {
	      fputs_filtered ("\"", stream);
	      in_quotes = 1;
	    }
	  ada_emit_char (char_at (string, i, type_len, byte_order),
			 elttype, stream, '"', type_len);
	  things_printed += 1;
	}
    }

  /* Terminate the quotes if necessary.  */
  if (in_quotes)
    fputs_filtered ("\"", stream);

  if (force_ellipses || i < length)
    fputs_filtered ("...", stream);
}

void
ada_printstr (struct ui_file *stream, struct type *type,
	      const gdb_byte *string, unsigned int length,
	      const char *encoding, int force_ellipses,
	      const struct value_print_options *options)
{
  printstr (stream, type, string, length, force_ellipses, TYPE_LENGTH (type),
	    options);
}

static int
print_variant_part (struct type *type, int field_num,
		    const gdb_byte *valaddr, int offset,
		    struct ui_file *stream, int recurse,
		    struct value *val,
		    const struct value_print_options *options,
		    int comma_needed,
		    struct type *outer_type, int outer_offset,
		    const struct language_defn *language)
{
  struct type *var_type = TYPE_FIELD_TYPE (type, field_num);
  int which = ada_which_variant_applies (var_type, outer_type,
					 valaddr + outer_offset);

  if (which < 0)
    return 0;
  else
    return print_field_values
      (TYPE_FIELD_TYPE (var_type, which),
       valaddr,
       offset + TYPE_FIELD_BITPOS (type, field_num) / HOST_CHAR_BIT
       + TYPE_FIELD_BITPOS (var_type, which) / HOST_CHAR_BIT,
       stream, recurse, val, options,
       comma_needed, outer_type, outer_offset, language);
}

/* Print out fields of value at VALADDR + OFFSET having structure type TYPE.

   TYPE, VALADDR, OFFSET, STREAM, RECURSE, and OPTIONS have the same
   meanings as in ada_print_value and ada_val_print.

   OUTER_TYPE and OUTER_OFFSET give type and address of enclosing
   record (used to get discriminant values when printing variant
   parts).

   COMMA_NEEDED is 1 if fields have been printed at the current recursion
   level, so that a comma is needed before any field printed by this
   call.

   Returns 1 if COMMA_NEEDED or any fields were printed.  */

static int
print_field_values (struct type *type, const gdb_byte *valaddr,
		    int offset, struct ui_file *stream, int recurse,
		    struct value *val,
		    const struct value_print_options *options,
		    int comma_needed,
		    struct type *outer_type, int outer_offset,
		    const struct language_defn *language)
{
  int i, len;

  len = TYPE_NFIELDS (type);

  for (i = 0; i < len; i += 1)
    {
      if (ada_is_ignored_field (type, i))
	continue;

      if (ada_is_wrapper_field (type, i))
	{
	  comma_needed =
	    print_field_values (TYPE_FIELD_TYPE (type, i),
				valaddr,
				(offset
				 + TYPE_FIELD_BITPOS (type, i) / HOST_CHAR_BIT),
				stream, recurse, val, options,
				comma_needed, type, offset, language);
	  continue;
	}
      else if (ada_is_variant_part (type, i))
	{
	  comma_needed =
	    print_variant_part (type, i, valaddr,
				offset, stream, recurse, val,
				options, comma_needed,
				outer_type, outer_offset, language);
	  continue;
	}

      if (comma_needed)
	fprintf_filtered (stream, ", ");
      comma_needed = 1;

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
      fprintf_filtered (stream, "%.*s",
			ada_name_prefix_len (TYPE_FIELD_NAME (type, i)),
			TYPE_FIELD_NAME (type, i));
      annotate_field_name_end ();
      fputs_filtered (" => ", stream);
      annotate_field_value ();

      if (TYPE_FIELD_PACKED (type, i))
	{
	  /* Bitfields require special handling, especially due to byte
	     order problems.  */
	  if (HAVE_CPLUS_STRUCT (type) && TYPE_FIELD_IGNORE (type, i))
	    {
	      fputs_filtered (_("<optimized out or zero length>"), stream);
	    }
	  else
	    {
	      struct value *v;
	      int bit_pos = TYPE_FIELD_BITPOS (type, i);
	      int bit_size = TYPE_FIELD_BITSIZE (type, i);
	      struct value_print_options opts;

	      adjust_type_signedness (TYPE_FIELD_TYPE (type, i));
	      v = ada_value_primitive_packed_val
		    (NULL, valaddr,
		     offset + bit_pos / HOST_CHAR_BIT,
		     bit_pos % HOST_CHAR_BIT,
		     bit_size, TYPE_FIELD_TYPE (type, i));
	      opts = *options;
	      opts.deref_ref = 0;
	      val_print (TYPE_FIELD_TYPE (type, i),
			 value_embedded_offset (v), 0,
			 stream, recurse + 1, v,
			 &opts, language);
	    }
	}
      else
	{
	  struct value_print_options opts = *options;

	  opts.deref_ref = 0;
	  val_print (TYPE_FIELD_TYPE (type, i),
		     (offset + TYPE_FIELD_BITPOS (type, i) / HOST_CHAR_BIT),
		     0, stream, recurse + 1, val, &opts, language);
	}
      annotate_field_end ();
    }

  return comma_needed;
}

/* Implement Ada val_print'ing for the case where TYPE is
   a TYPE_CODE_ARRAY of characters.  */

static void
ada_val_print_string (struct type *type, const gdb_byte *valaddr,
		      int offset, int offset_aligned, CORE_ADDR address,
		      struct ui_file *stream, int recurse,
		      struct value *original_value,
		      const struct value_print_options *options)
{
  enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (type));
  struct type *elttype = TYPE_TARGET_TYPE (type);
  unsigned int eltlen;
  unsigned int len;

  /* We know that ELTTYPE cannot possibly be null, because we assume
     that we're called only when TYPE is a string-like type.
     Similarly, the size of ELTTYPE should also be non-null, since
     it's a character-like type.  */
  gdb_assert (elttype != NULL);
  gdb_assert (TYPE_LENGTH (elttype) != 0);

  eltlen = TYPE_LENGTH (elttype);
  len = TYPE_LENGTH (type) / eltlen;

  if (options->prettyformat_arrays)
    print_spaces_filtered (2 + 2 * recurse, stream);

  /* If requested, look for the first null char and only print
     elements up to it.  */
  if (options->stop_print_at_null)
    {
      int temp_len;

      /* Look for a NULL char.  */
      for (temp_len = 0;
	   (temp_len < len
	    && temp_len < options->print_max
	    && char_at (valaddr + offset_aligned,
			temp_len, eltlen, byte_order) != 0);
	   temp_len += 1);
      len = temp_len;
    }

  printstr (stream, elttype, valaddr + offset_aligned, len, 0,
	    eltlen, options);
}

/* Implement Ada val_print-ing for GNAT arrays (Eg. fat pointers,
   thin pointers, etc).  */

static void
ada_val_print_gnat_array (struct type *type, const gdb_byte *valaddr,
			  int offset, CORE_ADDR address,
			  struct ui_file *stream, int recurse,
			  struct value *original_value,
			  const struct value_print_options *options,
			  const struct language_defn *language)
{
  struct value *mark = value_mark ();
  struct value *val;

  val = value_from_contents_and_address (type, valaddr + offset, address);
  /* If this is a reference, coerce it now.  This helps taking care
     of the case where ADDRESS is meaningless because original_value
     was not an lval.  */
  val = coerce_ref (val);
  if (TYPE_CODE (type) == TYPE_CODE_TYPEDEF)  /* array access type.  */
    val = ada_coerce_to_simple_array_ptr (val);
  else
    val = ada_coerce_to_simple_array (val);
  if (val == NULL)
    {
      gdb_assert (TYPE_CODE (type) == TYPE_CODE_TYPEDEF);
      fprintf_filtered (stream, "0x0");
    }
  else
    val_print (value_type (val),
	       value_embedded_offset (val), value_address (val),
	       stream, recurse, val, options, language);
  value_free_to_mark (mark);
}

/* Implement Ada val_print'ing for the case where TYPE is
   a TYPE_CODE_PTR.  */

static void
ada_val_print_ptr (struct type *type, const gdb_byte *valaddr,
		   int offset, int offset_aligned, CORE_ADDR address,
		   struct ui_file *stream, int recurse,
		   struct value *original_value,
		   const struct value_print_options *options,
		   const struct language_defn *language)
{
  val_print (type, offset, address, stream, recurse,
	     original_value, options, language_def (language_c));

  if (ada_is_tag_type (type))
    {
      struct value *val =
	value_from_contents_and_address (type,
					 valaddr + offset_aligned,
					 address + offset_aligned);
      const char *name = ada_tag_name (val);

      if (name != NULL)
	fprintf_filtered (stream, " (%s)", name);
    }
}

/* Implement Ada val_print'ing for the case where TYPE is
   a TYPE_CODE_INT or TYPE_CODE_RANGE.  */

static void
ada_val_print_num (struct type *type, const gdb_byte *valaddr,
		   int offset, int offset_aligned, CORE_ADDR address,
		   struct ui_file *stream, int recurse,
		   struct value *original_value,
		   const struct value_print_options *options,
		   const struct language_defn *language)
{
  if (ada_is_fixed_point_type (type))
    {
      LONGEST v = unpack_long (type, valaddr + offset_aligned);

      fprintf_filtered (stream, TYPE_LENGTH (type) < 4 ? "%.11g" : "%.17g",
			(double) ada_fixed_to_float (type, v));
      return;
    }
  else if (TYPE_CODE (type) == TYPE_CODE_RANGE)
    {
      struct type *target_type = TYPE_TARGET_TYPE (type);

      if (TYPE_LENGTH (type) != TYPE_LENGTH (target_type))
	{
	  /* Obscure case of range type that has different length from
	     its base type.  Perform a conversion, or we will get a
	     nonsense value.  Actually, we could use the same
	     code regardless of lengths; I'm just avoiding a cast.  */
	  struct value *v1
	    = value_from_contents_and_address (type, valaddr + offset, 0);
	  struct value *v = value_cast (target_type, v1);

	  val_print (target_type,
		     value_embedded_offset (v), 0, stream,
		     recurse + 1, v, options, language);
	}
      else
	val_print (TYPE_TARGET_TYPE (type), offset,
		   address, stream, recurse, original_value,
		   options, language);
      return;
    }
  else
    {
      int format = (options->format ? options->format
		    : options->output_format);

      if (format)
	{
	  struct value_print_options opts = *options;

	  opts.format = format;
	  val_print_scalar_formatted (type, offset_aligned,
				      original_value, &opts, 0, stream);
	}
      else if (ada_is_system_address_type (type))
	{
	  /* FIXME: We want to print System.Address variables using
	     the same format as for any access type.  But for some
	     reason GNAT encodes the System.Address type as an int,
	     so we have to work-around this deficiency by handling
	     System.Address values as a special case.  */

	  struct gdbarch *gdbarch = get_type_arch (type);
	  struct type *ptr_type = builtin_type (gdbarch)->builtin_data_ptr;
	  CORE_ADDR addr = extract_typed_address (valaddr + offset_aligned,
						  ptr_type);

	  fprintf_filtered (stream, "(");
	  type_print (type, "", stream, -1);
	  fprintf_filtered (stream, ") ");
	  fputs_filtered (paddress (gdbarch, addr), stream);
	}
      else
	{
	  val_print_type_code_int (type, valaddr + offset_aligned, stream);
	  if (ada_is_character_type (type))
	    {
	      LONGEST c;

	      fputs_filtered (" ", stream);
	      c = unpack_long (type, valaddr + offset_aligned);
	      ada_printchar (c, type, stream);
	    }
	}
      return;
    }
}

/* Implement Ada val_print'ing for the case where TYPE is
   a TYPE_CODE_ENUM.  */

static void
ada_val_print_enum (struct type *type, const gdb_byte *valaddr,
		    int offset, int offset_aligned, CORE_ADDR address,
		    struct ui_file *stream, int recurse,
		    struct value *original_value,
		    const struct value_print_options *options,
		    const struct language_defn *language)
{
  int i;
  unsigned int len;
  LONGEST val;

  if (options->format)
    {
      val_print_scalar_formatted (type, offset_aligned,
				  original_value, options, 0, stream);
      return;
    }

  len = TYPE_NFIELDS (type);
  val = unpack_long (type, valaddr + offset_aligned);
  for (i = 0; i < len; i++)
    {
      QUIT;
      if (val == TYPE_FIELD_ENUMVAL (type, i))
	break;
    }

  if (i < len)
    {
      const char *name = ada_enum_name (TYPE_FIELD_NAME (type, i));

      if (name[0] == '\'')
	fprintf_filtered (stream, "%ld %s", (long) val, name);
      else
	fputs_filtered (name, stream);
    }
  else
    print_longest (stream, 'd', 0, val);
}

/* Implement Ada val_print'ing for the case where TYPE is
   a TYPE_CODE_FLT.  */

static void
ada_val_print_flt (struct type *type, const gdb_byte *valaddr,
		   int offset, int offset_aligned, CORE_ADDR address,
		   struct ui_file *stream, int recurse,
		   struct value *original_value,
		   const struct value_print_options *options,
		   const struct language_defn *language)
{
  if (options->format)
    {
      val_print (type, offset, address, stream, recurse,
		 original_value, options, language_def (language_c));
      return;
    }

  ada_print_floating (valaddr + offset, type, stream);
}

/* Implement Ada val_print'ing for the case where TYPE is
   a TYPE_CODE_STRUCT or TYPE_CODE_UNION.  */

static void
ada_val_print_struct_union
  (struct type *type, const gdb_byte *valaddr, int offset,
   int offset_aligned, CORE_ADDR address, struct ui_file *stream,
   int recurse, struct value *original_value,
   const struct value_print_options *options,
   const struct language_defn *language)
{
  if (ada_is_bogus_array_descriptor (type))
    {
      fprintf_filtered (stream, "(...?)");
      return;
    }

  fprintf_filtered (stream, "(");

  if (print_field_values (type, valaddr, offset_aligned,
			  stream, recurse, original_value, options,
			  0, type, offset_aligned, language) != 0
      && options->prettyformat)
    {
      fprintf_filtered (stream, "\n");
      print_spaces_filtered (2 * recurse, stream);
    }

  fprintf_filtered (stream, ")");
}

/* Implement Ada val_print'ing for the case where TYPE is
   a TYPE_CODE_ARRAY.  */

static void
ada_val_print_array (struct type *type, const gdb_byte *valaddr,
		     int offset, int offset_aligned, CORE_ADDR address,
		     struct ui_file *stream, int recurse,
		     struct value *original_value,
		     const struct value_print_options *options)
{
  /* For an array of characters, print with string syntax.  */
  if (ada_is_string_type (type)
      && (options->format == 0 || options->format == 's'))
    {
      ada_val_print_string (type, valaddr, offset, offset_aligned,
			    address, stream, recurse, original_value,
			    options);
      return;
    }

  fprintf_filtered (stream, "(");
  print_optional_low_bound (stream, type, options);
  if (TYPE_FIELD_BITSIZE (type, 0) > 0)
    val_print_packed_array_elements (type, valaddr, offset_aligned,
				     0, stream, recurse,
				     original_value, options);
  else
    val_print_array_elements (type, offset_aligned, address,
			      stream, recurse, original_value,
			      options, 0);
  fprintf_filtered (stream, ")");
}

/* Implement Ada val_print'ing for the case where TYPE is
   a TYPE_CODE_REF.  */

static void
ada_val_print_ref (struct type *type, const gdb_byte *valaddr,
		   int offset, int offset_aligned, CORE_ADDR address,
		   struct ui_file *stream, int recurse,
		   struct value *original_value,
		   const struct value_print_options *options,
		   const struct language_defn *language)
{
  /* For references, the debugger is expected to print the value as
     an address if DEREF_REF is null.  But printing an address in place
     of the object value would be confusing to an Ada programmer.
     So, for Ada values, we print the actual dereferenced value
     regardless.  */
  struct type *elttype = check_typedef (TYPE_TARGET_TYPE (type));
  struct value *deref_val;
  CORE_ADDR deref_val_int;

  if (TYPE_CODE (elttype) == TYPE_CODE_UNDEF)
    {
      fputs_filtered ("<ref to undefined type>", stream);
      return;
    }

  deref_val = coerce_ref_if_computed (original_value);
  if (deref_val)
    {
      if (ada_is_tagged_type (value_type (deref_val), 1))
	deref_val = ada_tag_value_at_base_address (deref_val);

      common_val_print (deref_val, stream, recurse + 1, options,
			language);
      return;
    }

  deref_val_int = unpack_pointer (type, valaddr + offset_aligned);
  if (deref_val_int == 0)
    {
      fputs_filtered ("(null)", stream);
      return;
    }

  deref_val
    = ada_value_ind (value_from_pointer (lookup_pointer_type (elttype),
					 deref_val_int));
  if (ada_is_tagged_type (value_type (deref_val), 1))
    deref_val = ada_tag_value_at_base_address (deref_val);

  /* Make sure that the object does not have an unreasonable size
     before trying to print it.  This can happen for instance with
     references to dynamic objects whose contents is uninitialized
     (Eg: an array whose bounds are not set yet).  */
  ada_ensure_varsize_limit (value_type (deref_val));

  if (value_lazy (deref_val))
    value_fetch_lazy (deref_val);

  val_print (value_type (deref_val),
	     value_embedded_offset (deref_val),
	     value_address (deref_val), stream, recurse + 1,
	     deref_val, options, language);
}

/* See the comment on ada_val_print.  This function differs in that it
   does not catch evaluation errors (leaving that to ada_val_print).  */

static void
ada_val_print_1 (struct type *type,
		 int offset, CORE_ADDR address,
		 struct ui_file *stream, int recurse,
		 struct value *original_value,
		 const struct value_print_options *options,
		 const struct language_defn *language)
{
  int offset_aligned;
  const gdb_byte *valaddr = value_contents_for_printing (original_value);

  type = ada_check_typedef (type);

  if (ada_is_array_descriptor_type (type)
      || (ada_is_constrained_packed_array_type (type)
	  && TYPE_CODE (type) != TYPE_CODE_PTR))
    {
      ada_val_print_gnat_array (type, valaddr, offset, address,
				stream, recurse, original_value,
				options, language);
      return;
    }

  offset_aligned = offset + ada_aligned_value_addr (type, valaddr) - valaddr;
  type = printable_val_type (type, valaddr + offset_aligned);
  type = resolve_dynamic_type (type, valaddr + offset_aligned,
			       address + offset_aligned);

  switch (TYPE_CODE (type))
    {
    default:
      val_print (type, offset, address, stream, recurse,
		 original_value, options, language_def (language_c));
      break;

    case TYPE_CODE_PTR:
      ada_val_print_ptr (type, valaddr, offset, offset_aligned,
			 address, stream, recurse, original_value,
			 options, language);
      break;

    case TYPE_CODE_INT:
    case TYPE_CODE_RANGE:
      ada_val_print_num (type, valaddr, offset, offset_aligned,
			 address, stream, recurse, original_value,
			 options, language);
      break;

    case TYPE_CODE_ENUM:
      ada_val_print_enum (type, valaddr, offset, offset_aligned,
			  address, stream, recurse, original_value,
			  options, language);
      break;

    case TYPE_CODE_FLT:
      ada_val_print_flt (type, valaddr, offset, offset_aligned,
			 address, stream, recurse, original_value,
			 options, language);
      break;

    case TYPE_CODE_UNION:
    case TYPE_CODE_STRUCT:
      ada_val_print_struct_union (type, valaddr, offset, offset_aligned,
				  address, stream, recurse,
				  original_value, options, language);
      break;

    case TYPE_CODE_ARRAY:
      ada_val_print_array (type, valaddr, offset, offset_aligned,
			   address, stream, recurse, original_value,
			   options);
      return;

    case TYPE_CODE_REF:
      ada_val_print_ref (type, valaddr, offset, offset_aligned,
			 address, stream, recurse, original_value,
			 options, language);
      break;
    }
}

/* See val_print for a description of the various parameters of this
   function; they are identical.  */

void
ada_val_print (struct type *type,
	       int embedded_offset, CORE_ADDR address,
	       struct ui_file *stream, int recurse,
	       struct value *val,
	       const struct value_print_options *options)
{

  /* XXX: this catches QUIT/ctrl-c as well.  Isn't that busted?  */
  TRY
    {
      ada_val_print_1 (type, embedded_offset, address,
		       stream, recurse, val, options,
		       current_language);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
    }
  END_CATCH
}

void
ada_value_print (struct value *val0, struct ui_file *stream,
		 const struct value_print_options *options)
{
  struct value *val = ada_to_fixed_value (val0);
  CORE_ADDR address = value_address (val);
  struct type *type = ada_check_typedef (value_enclosing_type (val));
  struct value_print_options opts;

  /* If it is a pointer, indicate what it points to.  */
  if (TYPE_CODE (type) == TYPE_CODE_PTR)
    {
      /* Hack:  don't print (char *) for char strings.  Their
         type is indicated by the quoted string anyway.  */
      if (TYPE_LENGTH (TYPE_TARGET_TYPE (type)) != sizeof (char)
	  || TYPE_CODE (TYPE_TARGET_TYPE (type)) != TYPE_CODE_INT 
	  || TYPE_UNSIGNED (TYPE_TARGET_TYPE (type)))
	{
	  fprintf_filtered (stream, "(");
	  type_print (type, "", stream, -1);
	  fprintf_filtered (stream, ") ");
	}
    }
  else if (ada_is_array_descriptor_type (type))
    {
      /* We do not print the type description unless TYPE is an array
	 access type (this is encoded by the compiler as a typedef to
	 a fat pointer - hence the check against TYPE_CODE_TYPEDEF).  */
      if (TYPE_CODE (type) == TYPE_CODE_TYPEDEF)
        {
	  fprintf_filtered (stream, "(");
	  type_print (type, "", stream, -1);
	  fprintf_filtered (stream, ") ");
	}
    }
  else if (ada_is_bogus_array_descriptor (type))
    {
      fprintf_filtered (stream, "(");
      type_print (type, "", stream, -1);
      fprintf_filtered (stream, ") (...?)");
      return;
    }

  opts = *options;
  opts.deref_ref = 1;
  val_print (type,
	     value_embedded_offset (val), address,
	     stream, 0, val, &opts, current_language);
}
