/* Print values for GDB, the GNU debugger.

   Copyright (C) 1986-2013 Free Software Foundation, Inc.

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
#include "gdb_string.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "value.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "target.h"
#include "language.h"
#include "annotate.h"
#include "valprint.h"
#include "floatformat.h"
#include "doublest.h"
#include "exceptions.h"
#include "dfp.h"
#include "python/python.h"
#include "ada-lang.h"
#include "gdb_obstack.h"
#include "charset.h"
#include <ctype.h>

#include <errno.h>

/* Maximum number of wchars returned from wchar_iterate.  */
#define MAX_WCHARS 4

/* A convenience macro to compute the size of a wchar_t buffer containing X
   characters.  */
#define WCHAR_BUFLEN(X) ((X) * sizeof (gdb_wchar_t))

/* Character buffer size saved while iterating over wchars.  */
#define WCHAR_BUFLEN_MAX WCHAR_BUFLEN (MAX_WCHARS)

/* A structure to encapsulate state information from iterated
   character conversions.  */
struct converted_character
{
  /* The number of characters converted.  */
  int num_chars;

  /* The result of the conversion.  See charset.h for more.  */
  enum wchar_iterate_result result;

  /* The (saved) converted character(s).  */
  gdb_wchar_t chars[WCHAR_BUFLEN_MAX];

  /* The first converted target byte.  */
  const gdb_byte *buf;

  /* The number of bytes converted.  */
  size_t buflen;

  /* How many times this character(s) is repeated.  */
  int repeat_count;
};

typedef struct converted_character converted_character_d;
DEF_VEC_O (converted_character_d);


/* Prototypes for local functions */

static int partial_memory_read (CORE_ADDR memaddr, gdb_byte *myaddr,
				int len, int *errnoptr);

static void show_print (char *, int);

static void set_print (char *, int);

static void set_radix (char *, int);

static void show_radix (char *, int);

static void set_input_radix (char *, int, struct cmd_list_element *);

static void set_input_radix_1 (int, unsigned);

static void set_output_radix (char *, int, struct cmd_list_element *);

static void set_output_radix_1 (int, unsigned);

void _initialize_valprint (void);

#define PRINT_MAX_DEFAULT 200	/* Start print_max off at this value.  */

struct value_print_options user_print_options =
{
  Val_pretty_default,		/* pretty */
  0,				/* prettyprint_arrays */
  0,				/* prettyprint_structs */
  0,				/* vtblprint */
  1,				/* unionprint */
  1,				/* addressprint */
  0,				/* objectprint */
  PRINT_MAX_DEFAULT,		/* print_max */
  10,				/* repeat_count_threshold */
  0,				/* output_format */
  0,				/* format */
  0,				/* stop_print_at_null */
  0,				/* print_array_indexes */
  0,				/* deref_ref */
  1,				/* static_field_print */
  1,				/* pascal_static_field_print */
  0,				/* raw */
  0,				/* summary */
  1				/* symbol_print */
};

/* Initialize *OPTS to be a copy of the user print options.  */
void
get_user_print_options (struct value_print_options *opts)
{
  *opts = user_print_options;
}

/* Initialize *OPTS to be a copy of the user print options, but with
   pretty-printing disabled.  */
void
get_raw_print_options (struct value_print_options *opts)
{  
  *opts = user_print_options;
  opts->pretty = Val_no_prettyprint;
}

/* Initialize *OPTS to be a copy of the user print options, but using
   FORMAT as the formatting option.  */
void
get_formatted_print_options (struct value_print_options *opts,
			     char format)
{
  *opts = user_print_options;
  opts->format = format;
}

static void
show_print_max (struct ui_file *file, int from_tty,
		struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Limit on string chars or array "
		      "elements to print is %s.\n"),
		    value);
}


/* Default input and output radixes, and output format letter.  */

unsigned input_radix = 10;
static void
show_input_radix (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Default input radix for entering numbers is %s.\n"),
		    value);
}

unsigned output_radix = 10;
static void
show_output_radix (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Default output radix for printing of values is %s.\n"),
		    value);
}

/* By default we print arrays without printing the index of each element in
   the array.  This behavior can be changed by setting PRINT_ARRAY_INDEXES.  */

static void
show_print_array_indexes (struct ui_file *file, int from_tty,
		          struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Printing of array indexes is %s.\n"), value);
}

/* Print repeat counts if there are more than this many repetitions of an
   element in an array.  Referenced by the low level language dependent
   print routines.  */

static void
show_repeat_count_threshold (struct ui_file *file, int from_tty,
			     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Threshold for repeated print elements is %s.\n"),
		    value);
}

/* If nonzero, stops printing of char arrays at first null.  */

static void
show_stop_print_at_null (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Printing of char arrays to stop "
		      "at first null char is %s.\n"),
		    value);
}

/* Controls pretty printing of structures.  */

static void
show_prettyprint_structs (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Prettyprinting of structures is %s.\n"), value);
}

/* Controls pretty printing of arrays.  */

static void
show_prettyprint_arrays (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Prettyprinting of arrays is %s.\n"), value);
}

/* If nonzero, causes unions inside structures or other unions to be
   printed.  */

static void
show_unionprint (struct ui_file *file, int from_tty,
		 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Printing of unions interior to structures is %s.\n"),
		    value);
}

/* If nonzero, causes machine addresses to be printed in certain contexts.  */

static void
show_addressprint (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Printing of addresses is %s.\n"), value);
}

static void
show_symbol_print (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Printing of symbols when printing pointers is %s.\n"),
		    value);
}



/* A helper function for val_print.  When printing in "summary" mode,
   we want to print scalar arguments, but not aggregate arguments.
   This function distinguishes between the two.  */

static int
scalar_type_p (struct type *type)
{
  CHECK_TYPEDEF (type);
  while (TYPE_CODE (type) == TYPE_CODE_REF)
    {
      type = TYPE_TARGET_TYPE (type);
      CHECK_TYPEDEF (type);
    }
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_SET:
    case TYPE_CODE_STRING:
      return 0;
    default:
      return 1;
    }
}

/* See its definition in value.h.  */

int
valprint_check_validity (struct ui_file *stream,
			 struct type *type,
			 int embedded_offset,
			 const struct value *val)
{
  CHECK_TYPEDEF (type);

  if (TYPE_CODE (type) != TYPE_CODE_UNION
      && TYPE_CODE (type) != TYPE_CODE_STRUCT
      && TYPE_CODE (type) != TYPE_CODE_ARRAY)
    {
      if (!value_bits_valid (val, TARGET_CHAR_BIT * embedded_offset,
			     TARGET_CHAR_BIT * TYPE_LENGTH (type)))
	{
	  val_print_optimized_out (stream);
	  return 0;
	}

      if (value_bits_synthetic_pointer (val, TARGET_CHAR_BIT * embedded_offset,
					TARGET_CHAR_BIT * TYPE_LENGTH (type)))
	{
	  fputs_filtered (_("<synthetic pointer>"), stream);
	  return 0;
	}

      if (!value_bytes_available (val, embedded_offset, TYPE_LENGTH (type)))
	{
	  val_print_unavailable (stream);
	  return 0;
	}
    }

  return 1;
}

void
val_print_optimized_out (struct ui_file *stream)
{
  fprintf_filtered (stream, _("<optimized out>"));
}

void
val_print_unavailable (struct ui_file *stream)
{
  fprintf_filtered (stream, _("<unavailable>"));
}

void
val_print_invalid_address (struct ui_file *stream)
{
  fprintf_filtered (stream, _("<invalid address>"));
}

/* A generic val_print that is suitable for use by language
   implementations of the la_val_print method.  This function can
   handle most type codes, though not all, notably exception
   TYPE_CODE_UNION and TYPE_CODE_STRUCT, which must be implemented by
   the caller.
   
   Most arguments are as to val_print.
   
   The additional DECORATIONS argument can be used to customize the
   output in some small, language-specific ways.  */

void
generic_val_print (struct type *type, const gdb_byte *valaddr,
		   int embedded_offset, CORE_ADDR address,
		   struct ui_file *stream, int recurse,
		   const struct value *original_value,
		   const struct value_print_options *options,
		   const struct generic_val_print_decorations *decorations)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  unsigned int i = 0;	/* Number of characters printed.  */
  unsigned len;
  struct type *elttype, *unresolved_elttype;
  struct type *unresolved_type = type;
  LONGEST val;
  CORE_ADDR addr;

  CHECK_TYPEDEF (type);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_ARRAY:
      unresolved_elttype = TYPE_TARGET_TYPE (type);
      elttype = check_typedef (unresolved_elttype);
      if (TYPE_LENGTH (type) > 0 && TYPE_LENGTH (unresolved_elttype) > 0)
	{
          LONGEST low_bound, high_bound;

          if (!get_array_bounds (type, &low_bound, &high_bound))
            error (_("Could not determine the array high bound"));

	  if (options->prettyprint_arrays)
	    {
	      print_spaces_filtered (2 + 2 * recurse, stream);
	    }

	  fprintf_filtered (stream, "{");
	  val_print_array_elements (type, valaddr, embedded_offset,
				    address, stream,
				    recurse, original_value, options, 0);
	  fprintf_filtered (stream, "}");
	  break;
	}
      /* Array of unspecified length: treat like pointer to first
	 elt.  */
      addr = address + embedded_offset;
      goto print_unpacked_pointer;

    case TYPE_CODE_MEMBERPTR:
      val_print_scalar_formatted (type, valaddr, embedded_offset,
				  original_value, options, 0, stream);
      break;

    case TYPE_CODE_PTR:
      if (options->format && options->format != 's')
	{
	  val_print_scalar_formatted (type, valaddr, embedded_offset,
				      original_value, options, 0, stream);
	  break;
	}
      unresolved_elttype = TYPE_TARGET_TYPE (type);
      elttype = check_typedef (unresolved_elttype);
	{
	  addr = unpack_pointer (type, valaddr + embedded_offset);
	print_unpacked_pointer:

	  if (TYPE_CODE (elttype) == TYPE_CODE_FUNC)
	    {
	      /* Try to print what function it points to.  */
	      print_function_pointer_address (options, gdbarch, addr, stream);
	      return;
	    }

	  if (options->symbol_print)
	    print_address_demangle (options, gdbarch, addr, stream, demangle);
	  else if (options->addressprint)
	    fputs_filtered (paddress (gdbarch, addr), stream);
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
	      struct value *deref_val;

	      deref_val = coerce_ref_if_computed (original_value);
	      if (deref_val != NULL)
		{
		  /* More complicated computed references are not supported.  */
		  gdb_assert (embedded_offset == 0);
		}
	      else
		deref_val = value_at (TYPE_TARGET_TYPE (type),
				      unpack_pointer (type,
						      (valaddr
						       + embedded_offset)));

	      common_val_print (deref_val, stream, recurse, options,
				current_language);
	    }
	  else
	    fputs_filtered ("???", stream);
	}
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
	  if (val == TYPE_FIELD_ENUMVAL (type, i))
	    {
	      break;
	    }
	}
      if (i < len)
	{
	  fputs_filtered (TYPE_FIELD_NAME (type, i), stream);
	}
      else if (TYPE_FLAG_ENUM (type))
	{
	  int first = 1;

	  /* We have a "flag" enum, so we try to decompose it into
	     pieces as appropriate.  A flag enum has disjoint
	     constants by definition.  */
	  fputs_filtered ("(", stream);
	  for (i = 0; i < len; ++i)
	    {
	      QUIT;

	      if ((val & TYPE_FIELD_ENUMVAL (type, i)) != 0)
		{
		  if (!first)
		    fputs_filtered (" | ", stream);
		  first = 0;

		  val &= ~TYPE_FIELD_ENUMVAL (type, i);
		  fputs_filtered (TYPE_FIELD_NAME (type, i), stream);
		}
	    }

	  if (first || val != 0)
	    {
	      if (!first)
		fputs_filtered (" | ", stream);
	      fputs_filtered ("unknown: ", stream);
	      print_longest (stream, 'd', 0, val);
	    }

	  fputs_filtered (")", stream);
	}
      else
	print_longest (stream, 'd', 0, val);
      break;

    case TYPE_CODE_FLAGS:
      if (options->format)
	val_print_scalar_formatted (type, valaddr, embedded_offset,
				    original_value, options, 0, stream);
      else
	val_print_type_code_flags (type, valaddr + embedded_offset,
				   stream);
      break;

    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
      if (options->format)
	{
	  val_print_scalar_formatted (type, valaddr, embedded_offset,
				      original_value, options, 0, stream);
	  break;
	}
      /* FIXME, we should consider, at least for ANSI C language,
         eliminating the distinction made between FUNCs and POINTERs
         to FUNCs.  */
      fprintf_filtered (stream, "{");
      type_print (type, "", stream, -1);
      fprintf_filtered (stream, "} ");
      /* Try to print what function it points to, and its address.  */
      print_address_demangle (options, gdbarch, address, stream, demangle);
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
	    fputs_filtered (decorations->false_name, stream);
	  else if (val == 1)
	    fputs_filtered (decorations->true_name, stream);
	  else
	    print_longest (stream, 'd', 0, val);
	}
      break;

    case TYPE_CODE_RANGE:
      /* FIXME: create_range_type does not set the unsigned bit in a
         range type (I think it probably should copy it from the
         target type), so we won't print values which are too large to
         fit in a signed integer correctly.  */
      /* FIXME: Doesn't handle ranges of enums correctly.  (Can't just
         print with the target type, though, because the size of our
         type and the target type might differ).  */

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
	  LA_PRINT_CHAR (val, unresolved_type, stream);
	}
      break;

    case TYPE_CODE_FLT:
      if (options->format)
	{
	  val_print_scalar_formatted (type, valaddr, embedded_offset,
				      original_value, options, 0, stream);
	}
      else
	{
	  print_floating (valaddr + embedded_offset, type, stream);
	}
      break;

    case TYPE_CODE_DECFLOAT:
      if (options->format)
	val_print_scalar_formatted (type, valaddr, embedded_offset,
				    original_value, options, 0, stream);
      else
	print_decimal_floating (valaddr + embedded_offset,
				type, stream);
      break;

    case TYPE_CODE_VOID:
      fputs_filtered (decorations->void_name, stream);
      break;

    case TYPE_CODE_ERROR:
      fprintf_filtered (stream, "%s", TYPE_ERROR_NAME (type));
      break;

    case TYPE_CODE_UNDEF:
      /* This happens (without TYPE_FLAG_STUB set) on systems which
         don't use dbx xrefs (NO_DBX_XREFS in gcc) if a file has a
         "struct foo *bar" and no complete type for struct foo in that
         file.  */
      fprintf_filtered (stream, _("<incomplete type>"));
      break;

    case TYPE_CODE_COMPLEX:
      fprintf_filtered (stream, "%s", decorations->complex_prefix);
      if (options->format)
	val_print_scalar_formatted (TYPE_TARGET_TYPE (type),
				    valaddr, embedded_offset,
				    original_value, options, 0, stream);
      else
	print_floating (valaddr + embedded_offset,
			TYPE_TARGET_TYPE (type),
			stream);
      fprintf_filtered (stream, "%s", decorations->complex_infix);
      if (options->format)
	val_print_scalar_formatted (TYPE_TARGET_TYPE (type),
				    valaddr,
				    embedded_offset
				    + TYPE_LENGTH (TYPE_TARGET_TYPE (type)),
				    original_value,
				    options, 0, stream);
      else
	print_floating (valaddr + embedded_offset
			+ TYPE_LENGTH (TYPE_TARGET_TYPE (type)),
			TYPE_TARGET_TYPE (type),
			stream);
      fprintf_filtered (stream, "%s", decorations->complex_suffix);
      break;

    case TYPE_CODE_UNION:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_METHODPTR:
    default:
      error (_("Unhandled type code %d in symbol table."),
	     TYPE_CODE (type));
    }
  gdb_flush (stream);
}

/* Print using the given LANGUAGE the data of type TYPE located at
   VALADDR + EMBEDDED_OFFSET (within GDB), which came from the
   inferior at address ADDRESS + EMBEDDED_OFFSET, onto stdio stream
   STREAM according to OPTIONS.  VAL is the whole object that came
   from ADDRESS.  VALADDR must point to the head of VAL's contents
   buffer.

   The language printers will pass down an adjusted EMBEDDED_OFFSET to
   further helper subroutines as subfields of TYPE are printed.  In
   such cases, VALADDR is passed down unadjusted, as well as VAL, so
   that VAL can be queried for metadata about the contents data being
   printed, using EMBEDDED_OFFSET as an offset into VAL's contents
   buffer.  For example: "has this field been optimized out", or "I'm
   printing an object while inspecting a traceframe; has this
   particular piece of data been collected?".

   RECURSE indicates the amount of indentation to supply before
   continuation lines; this amount is roughly twice the value of
   RECURSE.  */

void
val_print (struct type *type, const gdb_byte *valaddr, int embedded_offset,
	   CORE_ADDR address, struct ui_file *stream, int recurse,
	   const struct value *val,
	   const struct value_print_options *options,
	   const struct language_defn *language)
{
  volatile struct gdb_exception except;
  int ret = 0;
  struct value_print_options local_opts = *options;
  struct type *real_type = check_typedef (type);

  if (local_opts.pretty == Val_pretty_default)
    local_opts.pretty = (local_opts.prettyprint_structs
			 ? Val_prettyprint : Val_no_prettyprint);

  QUIT;

  /* Ensure that the type is complete and not just a stub.  If the type is
     only a stub and we can't find and substitute its complete type, then
     print appropriate string and return.  */

  if (TYPE_STUB (real_type))
    {
      fprintf_filtered (stream, _("<incomplete type>"));
      gdb_flush (stream);
      return;
    }

  if (!valprint_check_validity (stream, real_type, embedded_offset, val))
    return;

  if (!options->raw)
    {
      ret = apply_val_pretty_printer (type, valaddr, embedded_offset,
				      address, stream, recurse,
				      val, options, language);
      if (ret)
	return;
    }

  /* Handle summary mode.  If the value is a scalar, print it;
     otherwise, print an ellipsis.  */
  if (options->summary && !scalar_type_p (type))
    {
      fprintf_filtered (stream, "...");
      return;
    }

  TRY_CATCH (except, RETURN_MASK_ERROR)
    {
      language->la_val_print (type, valaddr, embedded_offset, address,
			      stream, recurse, val,
			      &local_opts);
    }
  if (except.reason < 0)
    fprintf_filtered (stream, _("<error reading variable>"));
}

/* Check whether the value VAL is printable.  Return 1 if it is;
   return 0 and print an appropriate error message to STREAM according to
   OPTIONS if it is not.  */

static int
value_check_printable (struct value *val, struct ui_file *stream,
		       const struct value_print_options *options)
{
  if (val == 0)
    {
      fprintf_filtered (stream, _("<address of value unknown>"));
      return 0;
    }

  if (value_entirely_optimized_out (val))
    {
      if (options->summary && !scalar_type_p (value_type (val)))
	fprintf_filtered (stream, "...");
      else
	val_print_optimized_out (stream);
      return 0;
    }

  if (TYPE_CODE (value_type (val)) == TYPE_CODE_INTERNAL_FUNCTION)
    {
      fprintf_filtered (stream, _("<internal function %s>"),
			value_internal_function_name (val));
      return 0;
    }

  return 1;
}

/* Print using the given LANGUAGE the value VAL onto stream STREAM according
   to OPTIONS.

   This is a preferable interface to val_print, above, because it uses
   GDB's value mechanism.  */

void
common_val_print (struct value *val, struct ui_file *stream, int recurse,
		  const struct value_print_options *options,
		  const struct language_defn *language)
{
  if (!value_check_printable (val, stream, options))
    return;

  if (language->la_language == language_ada)
    /* The value might have a dynamic type, which would cause trouble
       below when trying to extract the value contents (since the value
       size is determined from the type size which is unknown).  So
       get a fixed representation of our value.  */
    val = ada_to_fixed_value (val);

  val_print (value_type (val), value_contents_for_printing (val),
	     value_embedded_offset (val), value_address (val),
	     stream, recurse,
	     val, options, language);
}

/* Print on stream STREAM the value VAL according to OPTIONS.  The value
   is printed using the current_language syntax.  */

void
value_print (struct value *val, struct ui_file *stream,
	     const struct value_print_options *options)
{
  if (!value_check_printable (val, stream, options))
    return;

  if (!options->raw)
    {
      int r = apply_val_pretty_printer (value_type (val),
					value_contents_for_printing (val),
					value_embedded_offset (val),
					value_address (val),
					stream, 0,
					val, options, current_language);

      if (r)
	return;
    }

  LA_VALUE_PRINT (val, stream, options);
}

/* Called by various <lang>_val_print routines to print
   TYPE_CODE_INT's.  TYPE is the type.  VALADDR is the address of the
   value.  STREAM is where to print the value.  */

void
val_print_type_code_int (struct type *type, const gdb_byte *valaddr,
			 struct ui_file *stream)
{
  enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (type));

  if (TYPE_LENGTH (type) > sizeof (LONGEST))
    {
      LONGEST val;

      if (TYPE_UNSIGNED (type)
	  && extract_long_unsigned_integer (valaddr, TYPE_LENGTH (type),
					    byte_order, &val))
	{
	  print_longest (stream, 'u', 0, val);
	}
      else
	{
	  /* Signed, or we couldn't turn an unsigned value into a
	     LONGEST.  For signed values, one could assume two's
	     complement (a reasonable assumption, I think) and do
	     better than this.  */
	  print_hex_chars (stream, (unsigned char *) valaddr,
			   TYPE_LENGTH (type), byte_order);
	}
    }
  else
    {
      print_longest (stream, TYPE_UNSIGNED (type) ? 'u' : 'd', 0,
		     unpack_long (type, valaddr));
    }
}

void
val_print_type_code_flags (struct type *type, const gdb_byte *valaddr,
			   struct ui_file *stream)
{
  ULONGEST val = unpack_long (type, valaddr);
  int bitpos, nfields = TYPE_NFIELDS (type);

  fputs_filtered ("[ ", stream);
  for (bitpos = 0; bitpos < nfields; bitpos++)
    {
      if (TYPE_FIELD_BITPOS (type, bitpos) != -1
	  && (val & ((ULONGEST)1 << bitpos)))
	{
	  if (TYPE_FIELD_NAME (type, bitpos))
	    fprintf_filtered (stream, "%s ", TYPE_FIELD_NAME (type, bitpos));
	  else
	    fprintf_filtered (stream, "#%d ", bitpos);
	}
    }
  fputs_filtered ("]", stream);
}

/* Print a scalar of data of type TYPE, pointed to in GDB by VALADDR,
   according to OPTIONS and SIZE on STREAM.  Format i is not supported
   at this level.

   This is how the elements of an array or structure are printed
   with a format.  */

void
val_print_scalar_formatted (struct type *type,
			    const gdb_byte *valaddr, int embedded_offset,
			    const struct value *val,
			    const struct value_print_options *options,
			    int size,
			    struct ui_file *stream)
{
  gdb_assert (val != NULL);
  gdb_assert (valaddr == value_contents_for_printing_const (val));

  /* If we get here with a string format, try again without it.  Go
     all the way back to the language printers, which may call us
     again.  */
  if (options->format == 's')
    {
      struct value_print_options opts = *options;
      opts.format = 0;
      opts.deref_ref = 0;
      val_print (type, valaddr, embedded_offset, 0, stream, 0, val, &opts,
		 current_language);
      return;
    }

  /* A scalar object that does not have all bits available can't be
     printed, because all bits contribute to its representation.  */
  if (!value_bits_valid (val, TARGET_CHAR_BIT * embedded_offset,
			      TARGET_CHAR_BIT * TYPE_LENGTH (type)))
    val_print_optimized_out (stream);
  else if (!value_bytes_available (val, embedded_offset, TYPE_LENGTH (type)))
    val_print_unavailable (stream);
  else
    print_scalar_formatted (valaddr + embedded_offset, type,
			    options, size, stream);
}

/* Print a number according to FORMAT which is one of d,u,x,o,b,h,w,g.
   The raison d'etre of this function is to consolidate printing of 
   LONG_LONG's into this one function.  The format chars b,h,w,g are 
   from print_scalar_formatted().  Numbers are printed using C
   format.

   USE_C_FORMAT means to use C format in all cases.  Without it, 
   'o' and 'x' format do not include the standard C radix prefix
   (leading 0 or 0x). 
   
   Hilfinger/2004-09-09: USE_C_FORMAT was originally called USE_LOCAL
   and was intended to request formating according to the current
   language and would be used for most integers that GDB prints.  The
   exceptional cases were things like protocols where the format of
   the integer is a protocol thing, not a user-visible thing).  The
   parameter remains to preserve the information of what things might
   be printed with language-specific format, should we ever resurrect
   that capability.  */

void
print_longest (struct ui_file *stream, int format, int use_c_format,
	       LONGEST val_long)
{
  const char *val;

  switch (format)
    {
    case 'd':
      val = int_string (val_long, 10, 1, 0, 1); break;
    case 'u':
      val = int_string (val_long, 10, 0, 0, 1); break;
    case 'x':
      val = int_string (val_long, 16, 0, 0, use_c_format); break;
    case 'b':
      val = int_string (val_long, 16, 0, 2, 1); break;
    case 'h':
      val = int_string (val_long, 16, 0, 4, 1); break;
    case 'w':
      val = int_string (val_long, 16, 0, 8, 1); break;
    case 'g':
      val = int_string (val_long, 16, 0, 16, 1); break;
      break;
    case 'o':
      val = int_string (val_long, 8, 0, 0, use_c_format); break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("failed internal consistency check"));
    } 
  fputs_filtered (val, stream);
}

/* This used to be a macro, but I don't think it is called often enough
   to merit such treatment.  */
/* Convert a LONGEST to an int.  This is used in contexts (e.g. number of
   arguments to a function, number in a value history, register number, etc.)
   where the value must not be larger than can fit in an int.  */

int
longest_to_int (LONGEST arg)
{
  /* Let the compiler do the work.  */
  int rtnval = (int) arg;

  /* Check for overflows or underflows.  */
  if (sizeof (LONGEST) > sizeof (int))
    {
      if (rtnval != arg)
	{
	  error (_("Value out of range."));
	}
    }
  return (rtnval);
}

/* Print a floating point value of type TYPE (not always a
   TYPE_CODE_FLT), pointed to in GDB by VALADDR, on STREAM.  */

void
print_floating (const gdb_byte *valaddr, struct type *type,
		struct ui_file *stream)
{
  DOUBLEST doub;
  int inv;
  const struct floatformat *fmt = NULL;
  unsigned len = TYPE_LENGTH (type);
  enum float_kind kind;

  /* If it is a floating-point, check for obvious problems.  */
  if (TYPE_CODE (type) == TYPE_CODE_FLT)
    fmt = floatformat_from_type (type);
  if (fmt != NULL)
    {
      kind = floatformat_classify (fmt, valaddr);
      if (kind == float_nan)
	{
	  if (floatformat_is_negative (fmt, valaddr))
	    fprintf_filtered (stream, "-");
	  fprintf_filtered (stream, "nan(");
	  fputs_filtered ("0x", stream);
	  fputs_filtered (floatformat_mantissa (fmt, valaddr), stream);
	  fprintf_filtered (stream, ")");
	  return;
	}
      else if (kind == float_infinite)
	{
	  if (floatformat_is_negative (fmt, valaddr))
	    fputs_filtered ("-", stream);
	  fputs_filtered ("inf", stream);
	  return;
	}
    }

  /* NOTE: cagney/2002-01-15: The TYPE passed into print_floating()
     isn't necessarily a TYPE_CODE_FLT.  Consequently, unpack_double
     needs to be used as that takes care of any necessary type
     conversions.  Such conversions are of course direct to DOUBLEST
     and disregard any possible target floating point limitations.
     For instance, a u64 would be converted and displayed exactly on a
     host with 80 bit DOUBLEST but with loss of information on a host
     with 64 bit DOUBLEST.  */

  doub = unpack_double (type, valaddr, &inv);
  if (inv)
    {
      fprintf_filtered (stream, "<invalid float value>");
      return;
    }

  /* FIXME: kettenis/2001-01-20: The following code makes too much
     assumptions about the host and target floating point format.  */

  /* NOTE: cagney/2002-02-03: Since the TYPE of what was passed in may
     not necessarily be a TYPE_CODE_FLT, the below ignores that and
     instead uses the type's length to determine the precision of the
     floating-point value being printed.  */

  if (len < sizeof (double))
      fprintf_filtered (stream, "%.9g", (double) doub);
  else if (len == sizeof (double))
      fprintf_filtered (stream, "%.17g", (double) doub);
  else
#ifdef PRINTF_HAS_LONG_DOUBLE
    fprintf_filtered (stream, "%.35Lg", doub);
#else
    /* This at least wins with values that are representable as
       doubles.  */
    fprintf_filtered (stream, "%.17g", (double) doub);
#endif
}

void
print_decimal_floating (const gdb_byte *valaddr, struct type *type,
			struct ui_file *stream)
{
  enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (type));
  char decstr[MAX_DECIMAL_STRING];
  unsigned len = TYPE_LENGTH (type);

  decimal_to_string (valaddr, len, byte_order, decstr);
  fputs_filtered (decstr, stream);
  return;
}

void
print_binary_chars (struct ui_file *stream, const gdb_byte *valaddr,
		    unsigned len, enum bfd_endian byte_order)
{

#define BITS_IN_BYTES 8

  const gdb_byte *p;
  unsigned int i;
  int b;

  /* Declared "int" so it will be signed.
     This ensures that right shift will shift in zeros.  */

  const int mask = 0x080;

  /* FIXME: We should be not printing leading zeroes in most cases.  */

  if (byte_order == BFD_ENDIAN_BIG)
    {
      for (p = valaddr;
	   p < valaddr + len;
	   p++)
	{
	  /* Every byte has 8 binary characters; peel off
	     and print from the MSB end.  */

	  for (i = 0; i < (BITS_IN_BYTES * sizeof (*p)); i++)
	    {
	      if (*p & (mask >> i))
		b = 1;
	      else
		b = 0;

	      fprintf_filtered (stream, "%1d", b);
	    }
	}
    }
  else
    {
      for (p = valaddr + len - 1;
	   p >= valaddr;
	   p--)
	{
	  for (i = 0; i < (BITS_IN_BYTES * sizeof (*p)); i++)
	    {
	      if (*p & (mask >> i))
		b = 1;
	      else
		b = 0;

	      fprintf_filtered (stream, "%1d", b);
	    }
	}
    }
}

/* VALADDR points to an integer of LEN bytes.
   Print it in octal on stream or format it in buf.  */

void
print_octal_chars (struct ui_file *stream, const gdb_byte *valaddr,
		   unsigned len, enum bfd_endian byte_order)
{
  const gdb_byte *p;
  unsigned char octa1, octa2, octa3, carry;
  int cycle;

  /* FIXME: We should be not printing leading zeroes in most cases.  */


  /* Octal is 3 bits, which doesn't fit.  Yuk.  So we have to track
   * the extra bits, which cycle every three bytes:
   *
   * Byte side:       0            1             2          3
   *                         |             |            |            |
   * bit number   123 456 78 | 9 012 345 6 | 78 901 234 | 567 890 12 |
   *
   * Octal side:   0   1   carry  3   4  carry ...
   *
   * Cycle number:    0             1            2
   *
   * But of course we are printing from the high side, so we have to
   * figure out where in the cycle we are so that we end up with no
   * left over bits at the end.
   */
#define BITS_IN_OCTAL 3
#define HIGH_ZERO     0340
#define LOW_ZERO      0016
#define CARRY_ZERO    0003
#define HIGH_ONE      0200
#define MID_ONE       0160
#define LOW_ONE       0016
#define CARRY_ONE     0001
#define HIGH_TWO      0300
#define MID_TWO       0070
#define LOW_TWO       0007

  /* For 32 we start in cycle 2, with two bits and one bit carry;
     for 64 in cycle in cycle 1, with one bit and a two bit carry.  */

  cycle = (len * BITS_IN_BYTES) % BITS_IN_OCTAL;
  carry = 0;

  fputs_filtered ("0", stream);
  if (byte_order == BFD_ENDIAN_BIG)
    {
      for (p = valaddr;
	   p < valaddr + len;
	   p++)
	{
	  switch (cycle)
	    {
	    case 0:
	      /* No carry in, carry out two bits.  */

	      octa1 = (HIGH_ZERO & *p) >> 5;
	      octa2 = (LOW_ZERO & *p) >> 2;
	      carry = (CARRY_ZERO & *p);
	      fprintf_filtered (stream, "%o", octa1);
	      fprintf_filtered (stream, "%o", octa2);
	      break;

	    case 1:
	      /* Carry in two bits, carry out one bit.  */

	      octa1 = (carry << 1) | ((HIGH_ONE & *p) >> 7);
	      octa2 = (MID_ONE & *p) >> 4;
	      octa3 = (LOW_ONE & *p) >> 1;
	      carry = (CARRY_ONE & *p);
	      fprintf_filtered (stream, "%o", octa1);
	      fprintf_filtered (stream, "%o", octa2);
	      fprintf_filtered (stream, "%o", octa3);
	      break;

	    case 2:
	      /* Carry in one bit, no carry out.  */

	      octa1 = (carry << 2) | ((HIGH_TWO & *p) >> 6);
	      octa2 = (MID_TWO & *p) >> 3;
	      octa3 = (LOW_TWO & *p);
	      carry = 0;
	      fprintf_filtered (stream, "%o", octa1);
	      fprintf_filtered (stream, "%o", octa2);
	      fprintf_filtered (stream, "%o", octa3);
	      break;

	    default:
	      error (_("Internal error in octal conversion;"));
	    }

	  cycle++;
	  cycle = cycle % BITS_IN_OCTAL;
	}
    }
  else
    {
      for (p = valaddr + len - 1;
	   p >= valaddr;
	   p--)
	{
	  switch (cycle)
	    {
	    case 0:
	      /* Carry out, no carry in */

	      octa1 = (HIGH_ZERO & *p) >> 5;
	      octa2 = (LOW_ZERO & *p) >> 2;
	      carry = (CARRY_ZERO & *p);
	      fprintf_filtered (stream, "%o", octa1);
	      fprintf_filtered (stream, "%o", octa2);
	      break;

	    case 1:
	      /* Carry in, carry out */

	      octa1 = (carry << 1) | ((HIGH_ONE & *p) >> 7);
	      octa2 = (MID_ONE & *p) >> 4;
	      octa3 = (LOW_ONE & *p) >> 1;
	      carry = (CARRY_ONE & *p);
	      fprintf_filtered (stream, "%o", octa1);
	      fprintf_filtered (stream, "%o", octa2);
	      fprintf_filtered (stream, "%o", octa3);
	      break;

	    case 2:
	      /* Carry in, no carry out */

	      octa1 = (carry << 2) | ((HIGH_TWO & *p) >> 6);
	      octa2 = (MID_TWO & *p) >> 3;
	      octa3 = (LOW_TWO & *p);
	      carry = 0;
	      fprintf_filtered (stream, "%o", octa1);
	      fprintf_filtered (stream, "%o", octa2);
	      fprintf_filtered (stream, "%o", octa3);
	      break;

	    default:
	      error (_("Internal error in octal conversion;"));
	    }

	  cycle++;
	  cycle = cycle % BITS_IN_OCTAL;
	}
    }

}

/* VALADDR points to an integer of LEN bytes.
   Print it in decimal on stream or format it in buf.  */

void
print_decimal_chars (struct ui_file *stream, const gdb_byte *valaddr,
		     unsigned len, enum bfd_endian byte_order)
{
#define TEN             10
#define CARRY_OUT(  x ) ((x) / TEN)	/* extend char to int */
#define CARRY_LEFT( x ) ((x) % TEN)
#define SHIFT( x )      ((x) << 4)
#define LOW_NIBBLE(  x ) ( (x) & 0x00F)
#define HIGH_NIBBLE( x ) (((x) & 0x0F0) >> 4)

  const gdb_byte *p;
  unsigned char *digits;
  int carry;
  int decimal_len;
  int i, j, decimal_digits;
  int dummy;
  int flip;

  /* Base-ten number is less than twice as many digits
     as the base 16 number, which is 2 digits per byte.  */

  decimal_len = len * 2 * 2;
  digits = xmalloc (decimal_len);

  for (i = 0; i < decimal_len; i++)
    {
      digits[i] = 0;
    }

  /* Ok, we have an unknown number of bytes of data to be printed in
   * decimal.
   *
   * Given a hex number (in nibbles) as XYZ, we start by taking X and
   * decemalizing it as "x1 x2" in two decimal nibbles.  Then we multiply
   * the nibbles by 16, add Y and re-decimalize.  Repeat with Z.
   *
   * The trick is that "digits" holds a base-10 number, but sometimes
   * the individual digits are > 10.
   *
   * Outer loop is per nibble (hex digit) of input, from MSD end to
   * LSD end.
   */
  decimal_digits = 0;		/* Number of decimal digits so far */
  p = (byte_order == BFD_ENDIAN_BIG) ? valaddr : valaddr + len - 1;
  flip = 0;
  while ((byte_order == BFD_ENDIAN_BIG) ? (p < valaddr + len) : (p >= valaddr))
    {
      /*
       * Multiply current base-ten number by 16 in place.
       * Each digit was between 0 and 9, now is between
       * 0 and 144.
       */
      for (j = 0; j < decimal_digits; j++)
	{
	  digits[j] = SHIFT (digits[j]);
	}

      /* Take the next nibble off the input and add it to what
       * we've got in the LSB position.  Bottom 'digit' is now
       * between 0 and 159.
       *
       * "flip" is used to run this loop twice for each byte.
       */
      if (flip == 0)
	{
	  /* Take top nibble.  */

	  digits[0] += HIGH_NIBBLE (*p);
	  flip = 1;
	}
      else
	{
	  /* Take low nibble and bump our pointer "p".  */

	  digits[0] += LOW_NIBBLE (*p);
          if (byte_order == BFD_ENDIAN_BIG)
	    p++;
	  else
	    p--;
	  flip = 0;
	}

      /* Re-decimalize.  We have to do this often enough
       * that we don't overflow, but once per nibble is
       * overkill.  Easier this way, though.  Note that the
       * carry is often larger than 10 (e.g. max initial
       * carry out of lowest nibble is 15, could bubble all
       * the way up greater than 10).  So we have to do
       * the carrying beyond the last current digit.
       */
      carry = 0;
      for (j = 0; j < decimal_len - 1; j++)
	{
	  digits[j] += carry;

	  /* "/" won't handle an unsigned char with
	   * a value that if signed would be negative.
	   * So extend to longword int via "dummy".
	   */
	  dummy = digits[j];
	  carry = CARRY_OUT (dummy);
	  digits[j] = CARRY_LEFT (dummy);

	  if (j >= decimal_digits && carry == 0)
	    {
	      /*
	       * All higher digits are 0 and we
	       * no longer have a carry.
	       *
	       * Note: "j" is 0-based, "decimal_digits" is
	       *       1-based.
	       */
	      decimal_digits = j + 1;
	      break;
	    }
	}
    }

  /* Ok, now "digits" is the decimal representation, with
     the "decimal_digits" actual digits.  Print!  */

  for (i = decimal_digits - 1; i >= 0; i--)
    {
      fprintf_filtered (stream, "%1d", digits[i]);
    }
  xfree (digits);
}

/* VALADDR points to an integer of LEN bytes.  Print it in hex on stream.  */

void
print_hex_chars (struct ui_file *stream, const gdb_byte *valaddr,
		 unsigned len, enum bfd_endian byte_order)
{
  const gdb_byte *p;

  /* FIXME: We should be not printing leading zeroes in most cases.  */

  fputs_filtered ("0x", stream);
  if (byte_order == BFD_ENDIAN_BIG)
    {
      for (p = valaddr;
	   p < valaddr + len;
	   p++)
	{
	  fprintf_filtered (stream, "%02x", *p);
	}
    }
  else
    {
      for (p = valaddr + len - 1;
	   p >= valaddr;
	   p--)
	{
	  fprintf_filtered (stream, "%02x", *p);
	}
    }
}

/* VALADDR points to a char integer of LEN bytes.
   Print it out in appropriate language form on stream.
   Omit any leading zero chars.  */

void
print_char_chars (struct ui_file *stream, struct type *type,
		  const gdb_byte *valaddr,
		  unsigned len, enum bfd_endian byte_order)
{
  const gdb_byte *p;

  if (byte_order == BFD_ENDIAN_BIG)
    {
      p = valaddr;
      while (p < valaddr + len - 1 && *p == 0)
	++p;

      while (p < valaddr + len)
	{
	  LA_EMIT_CHAR (*p, type, stream, '\'');
	  ++p;
	}
    }
  else
    {
      p = valaddr + len - 1;
      while (p > valaddr && *p == 0)
	--p;

      while (p >= valaddr)
	{
	  LA_EMIT_CHAR (*p, type, stream, '\'');
	  --p;
	}
    }
}

/* Print function pointer with inferior address ADDRESS onto stdio
   stream STREAM.  */

void
print_function_pointer_address (const struct value_print_options *options,
				struct gdbarch *gdbarch,
				CORE_ADDR address,
				struct ui_file *stream)
{
  CORE_ADDR func_addr
    = gdbarch_convert_from_func_ptr_addr (gdbarch, address,
					  &current_target);

  /* If the function pointer is represented by a description, print
     the address of the description.  */
  if (options->addressprint && func_addr != address)
    {
      fputs_filtered ("@", stream);
      fputs_filtered (paddress (gdbarch, address), stream);
      fputs_filtered (": ", stream);
    }
  print_address_demangle (options, gdbarch, func_addr, stream, demangle);
}


/* Print on STREAM using the given OPTIONS the index for the element
   at INDEX of an array whose index type is INDEX_TYPE.  */
    
void  
maybe_print_array_index (struct type *index_type, LONGEST index,
                         struct ui_file *stream,
			 const struct value_print_options *options)
{
  struct value *index_value;

  if (!options->print_array_indexes)
    return; 
    
  index_value = value_from_longest (index_type, index);

  LA_PRINT_ARRAY_INDEX (index_value, stream, options);
}

/*  Called by various <lang>_val_print routines to print elements of an
   array in the form "<elem1>, <elem2>, <elem3>, ...".

   (FIXME?)  Assumes array element separator is a comma, which is correct
   for all languages currently handled.
   (FIXME?)  Some languages have a notation for repeated array elements,
   perhaps we should try to use that notation when appropriate.  */

void
val_print_array_elements (struct type *type,
			  const gdb_byte *valaddr, int embedded_offset,
			  CORE_ADDR address, struct ui_file *stream,
			  int recurse,
			  const struct value *val,
			  const struct value_print_options *options,
			  unsigned int i)
{
  unsigned int things_printed = 0;
  unsigned len;
  struct type *elttype, *index_type;
  unsigned eltlen;
  /* Position of the array element we are examining to see
     whether it is repeated.  */
  unsigned int rep1;
  /* Number of repetitions we have detected so far.  */
  unsigned int reps;
  LONGEST low_bound, high_bound;

  elttype = TYPE_TARGET_TYPE (type);
  eltlen = TYPE_LENGTH (check_typedef (elttype));
  index_type = TYPE_INDEX_TYPE (type);

  if (get_array_bounds (type, &low_bound, &high_bound))
    {
      /* The array length should normally be HIGH_BOUND - LOW_BOUND + 1.
         But we have to be a little extra careful, because some languages
	 such as Ada allow LOW_BOUND to be greater than HIGH_BOUND for
	 empty arrays.  In that situation, the array length is just zero,
	 not negative!  */
      if (low_bound > high_bound)
	len = 0;
      else
	len = high_bound - low_bound + 1;
    }
  else
    {
      warning (_("unable to get bounds of array, assuming null array"));
      low_bound = 0;
      len = 0;
    }

  annotate_array_section_begin (i, elttype);

  for (; i < len && things_printed < options->print_max; i++)
    {
      if (i != 0)
	{
	  if (options->prettyprint_arrays)
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
      maybe_print_array_index (index_type, i + low_bound,
                               stream, options);

      rep1 = i + 1;
      reps = 1;
      /* Only check for reps if repeat_count_threshold is not set to
	 UINT_MAX (unlimited).  */
      if (options->repeat_count_threshold < UINT_MAX)
	{
	  while (rep1 < len
		 && value_available_contents_eq (val,
						 embedded_offset + i * eltlen,
						 val,
						 (embedded_offset
						  + rep1 * eltlen),
						 eltlen))
	    {
	      ++reps;
	      ++rep1;
	    }
	}

      if (reps > options->repeat_count_threshold)
	{
	  val_print (elttype, valaddr, embedded_offset + i * eltlen,
		     address, stream, recurse + 1, val, options,
		     current_language);
	  annotate_elt_rep (reps);
	  fprintf_filtered (stream, " <repeats %u times>", reps);
	  annotate_elt_rep_end ();

	  i = rep1 - 1;
	  things_printed += options->repeat_count_threshold;
	}
      else
	{
	  val_print (elttype, valaddr, embedded_offset + i * eltlen,
		     address,
		     stream, recurse + 1, val, options, current_language);
	  annotate_elt ();
	  things_printed++;
	}
    }
  annotate_array_section_end ();
  if (i < len)
    {
      fprintf_filtered (stream, "...");
    }
}

/* Read LEN bytes of target memory at address MEMADDR, placing the
   results in GDB's memory at MYADDR.  Returns a count of the bytes
   actually read, and optionally an errno value in the location
   pointed to by ERRNOPTR if ERRNOPTR is non-null.  */

/* FIXME: cagney/1999-10-14: Only used by val_print_string.  Can this
   function be eliminated.  */

static int
partial_memory_read (CORE_ADDR memaddr, gdb_byte *myaddr,
		     int len, int *errnoptr)
{
  int nread;			/* Number of bytes actually read.  */
  int errcode;			/* Error from last read.  */

  /* First try a complete read.  */
  errcode = target_read_memory (memaddr, myaddr, len);
  if (errcode == 0)
    {
      /* Got it all.  */
      nread = len;
    }
  else
    {
      /* Loop, reading one byte at a time until we get as much as we can.  */
      for (errcode = 0, nread = 0; len > 0 && errcode == 0; nread++, len--)
	{
	  errcode = target_read_memory (memaddr++, myaddr++, 1);
	}
      /* If an error, the last read was unsuccessful, so adjust count.  */
      if (errcode != 0)
	{
	  nread--;
	}
    }
  if (errnoptr != NULL)
    {
      *errnoptr = errcode;
    }
  return (nread);
}

/* Read a string from the inferior, at ADDR, with LEN characters of WIDTH bytes
   each.  Fetch at most FETCHLIMIT characters.  BUFFER will be set to a newly
   allocated buffer containing the string, which the caller is responsible to
   free, and BYTES_READ will be set to the number of bytes read.  Returns 0 on
   success, or errno on failure.

   If LEN > 0, reads exactly LEN characters (including eventual NULs in
   the middle or end of the string).  If LEN is -1, stops at the first
   null character (not necessarily the first null byte) up to a maximum
   of FETCHLIMIT characters.  Set FETCHLIMIT to UINT_MAX to read as many
   characters as possible from the string.

   Unless an exception is thrown, BUFFER will always be allocated, even on
   failure.  In this case, some characters might have been read before the
   failure happened.  Check BYTES_READ to recognize this situation.

   Note: There was a FIXME asking to make this code use target_read_string,
   but this function is more general (can read past null characters, up to
   given LEN).  Besides, it is used much more often than target_read_string
   so it is more tested.  Perhaps callers of target_read_string should use
   this function instead?  */

int
read_string (CORE_ADDR addr, int len, int width, unsigned int fetchlimit,
	     enum bfd_endian byte_order, gdb_byte **buffer, int *bytes_read)
{
  int found_nul;		/* Non-zero if we found the nul char.  */
  int errcode;			/* Errno returned from bad reads.  */
  unsigned int nfetch;		/* Chars to fetch / chars fetched.  */
  unsigned int chunksize;	/* Size of each fetch, in chars.  */
  gdb_byte *bufptr;		/* Pointer to next available byte in
				   buffer.  */
  gdb_byte *limit;		/* First location past end of fetch buffer.  */
  struct cleanup *old_chain = NULL;	/* Top of the old cleanup chain.  */

  /* Decide how large of chunks to try to read in one operation.  This
     is also pretty simple.  If LEN >= zero, then we want fetchlimit chars,
     so we might as well read them all in one operation.  If LEN is -1, we
     are looking for a NUL terminator to end the fetching, so we might as
     well read in blocks that are large enough to be efficient, but not so
     large as to be slow if fetchlimit happens to be large.  So we choose the
     minimum of 8 and fetchlimit.  We used to use 200 instead of 8 but
     200 is way too big for remote debugging over a serial line.  */

  chunksize = (len == -1 ? min (8, fetchlimit) : fetchlimit);

  /* Loop until we either have all the characters, or we encounter
     some error, such as bumping into the end of the address space.  */

  found_nul = 0;
  *buffer = NULL;

  old_chain = make_cleanup (free_current_contents, buffer);

  if (len > 0)
    {
      *buffer = (gdb_byte *) xmalloc (len * width);
      bufptr = *buffer;

      nfetch = partial_memory_read (addr, bufptr, len * width, &errcode)
	/ width;
      addr += nfetch * width;
      bufptr += nfetch * width;
    }
  else if (len == -1)
    {
      unsigned long bufsize = 0;

      do
	{
	  QUIT;
	  nfetch = min (chunksize, fetchlimit - bufsize);

	  if (*buffer == NULL)
	    *buffer = (gdb_byte *) xmalloc (nfetch * width);
	  else
	    *buffer = (gdb_byte *) xrealloc (*buffer,
					     (nfetch + bufsize) * width);

	  bufptr = *buffer + bufsize * width;
	  bufsize += nfetch;

	  /* Read as much as we can.  */
	  nfetch = partial_memory_read (addr, bufptr, nfetch * width, &errcode)
		    / width;

	  /* Scan this chunk for the null character that terminates the string
	     to print.  If found, we don't need to fetch any more.  Note
	     that bufptr is explicitly left pointing at the next character
	     after the null character, or at the next character after the end
	     of the buffer.  */

	  limit = bufptr + nfetch * width;
	  while (bufptr < limit)
	    {
	      unsigned long c;

	      c = extract_unsigned_integer (bufptr, width, byte_order);
	      addr += width;
	      bufptr += width;
	      if (c == 0)
		{
		  /* We don't care about any error which happened after
		     the NUL terminator.  */
		  errcode = 0;
		  found_nul = 1;
		  break;
		}
	    }
	}
      while (errcode == 0	/* no error */
	     && bufptr - *buffer < fetchlimit * width	/* no overrun */
	     && !found_nul);	/* haven't found NUL yet */
    }
  else
    {				/* Length of string is really 0!  */
      /* We always allocate *buffer.  */
      *buffer = bufptr = xmalloc (1);
      errcode = 0;
    }

  /* bufptr and addr now point immediately beyond the last byte which we
     consider part of the string (including a '\0' which ends the string).  */
  *bytes_read = bufptr - *buffer;

  QUIT;

  discard_cleanups (old_chain);

  return errcode;
}

/* Return true if print_wchar can display W without resorting to a
   numeric escape, false otherwise.  */

static int
wchar_printable (gdb_wchar_t w)
{
  return (gdb_iswprint (w)
	  || w == LCST ('\a') || w == LCST ('\b')
	  || w == LCST ('\f') || w == LCST ('\n')
	  || w == LCST ('\r') || w == LCST ('\t')
	  || w == LCST ('\v'));
}

/* A helper function that converts the contents of STRING to wide
   characters and then appends them to OUTPUT.  */

static void
append_string_as_wide (const char *string,
		       struct obstack *output)
{
  for (; *string; ++string)
    {
      gdb_wchar_t w = gdb_btowc (*string);
      obstack_grow (output, &w, sizeof (gdb_wchar_t));
    }
}

/* Print a wide character W to OUTPUT.  ORIG is a pointer to the
   original (target) bytes representing the character, ORIG_LEN is the
   number of valid bytes.  WIDTH is the number of bytes in a base
   characters of the type.  OUTPUT is an obstack to which wide
   characters are emitted.  QUOTER is a (narrow) character indicating
   the style of quotes surrounding the character to be printed.
   NEED_ESCAPE is an in/out flag which is used to track numeric
   escapes across calls.  */

static void
print_wchar (gdb_wint_t w, const gdb_byte *orig,
	     int orig_len, int width,
	     enum bfd_endian byte_order,
	     struct obstack *output,
	     int quoter, int *need_escapep)
{
  int need_escape = *need_escapep;

  *need_escapep = 0;
  if (gdb_iswprint (w) && (!need_escape || (!gdb_iswdigit (w)
					    && w != LCST ('8')
					    && w != LCST ('9'))))
    {
      gdb_wchar_t wchar = w;

      if (w == gdb_btowc (quoter) || w == LCST ('\\'))
	obstack_grow_wstr (output, LCST ("\\"));
      obstack_grow (output, &wchar, sizeof (gdb_wchar_t));
    }
  else
    {
      switch (w)
	{
	case LCST ('\a'):
	  obstack_grow_wstr (output, LCST ("\\a"));
	  break;
	case LCST ('\b'):
	  obstack_grow_wstr (output, LCST ("\\b"));
	  break;
	case LCST ('\f'):
	  obstack_grow_wstr (output, LCST ("\\f"));
	  break;
	case LCST ('\n'):
	  obstack_grow_wstr (output, LCST ("\\n"));
	  break;
	case LCST ('\r'):
	  obstack_grow_wstr (output, LCST ("\\r"));
	  break;
	case LCST ('\t'):
	  obstack_grow_wstr (output, LCST ("\\t"));
	  break;
	case LCST ('\v'):
	  obstack_grow_wstr (output, LCST ("\\v"));
	  break;
	default:
	  {
	    int i;

	    for (i = 0; i + width <= orig_len; i += width)
	      {
		char octal[30];
		ULONGEST value;

		value = extract_unsigned_integer (&orig[i], width,
						  byte_order);
		/* If the value fits in 3 octal digits, print it that
		   way.  Otherwise, print it as a hex escape.  */
		if (value <= 0777)
		  xsnprintf (octal, sizeof (octal), "\\%.3o",
			     (int) (value & 0777));
		else
		  xsnprintf (octal, sizeof (octal), "\\x%lx", (long) value);
		append_string_as_wide (octal, output);
	      }
	    /* If we somehow have extra bytes, print them now.  */
	    while (i < orig_len)
	      {
		char octal[5];

		xsnprintf (octal, sizeof (octal), "\\%.3o", orig[i] & 0xff);
		append_string_as_wide (octal, output);
		++i;
	      }

	    *need_escapep = 1;
	  }
	  break;
	}
    }
}

/* Print the character C on STREAM as part of the contents of a
   literal string whose delimiter is QUOTER.  ENCODING names the
   encoding of C.  */

void
generic_emit_char (int c, struct type *type, struct ui_file *stream,
		   int quoter, const char *encoding)
{
  enum bfd_endian byte_order
    = gdbarch_byte_order (get_type_arch (type));
  struct obstack wchar_buf, output;
  struct cleanup *cleanups;
  gdb_byte *buf;
  struct wchar_iterator *iter;
  int need_escape = 0;

  buf = alloca (TYPE_LENGTH (type));
  pack_long (buf, type, c);

  iter = make_wchar_iterator (buf, TYPE_LENGTH (type),
			      encoding, TYPE_LENGTH (type));
  cleanups = make_cleanup_wchar_iterator (iter);

  /* This holds the printable form of the wchar_t data.  */
  obstack_init (&wchar_buf);
  make_cleanup_obstack_free (&wchar_buf);

  while (1)
    {
      int num_chars;
      gdb_wchar_t *chars;
      const gdb_byte *buf;
      size_t buflen;
      int print_escape = 1;
      enum wchar_iterate_result result;

      num_chars = wchar_iterate (iter, &result, &chars, &buf, &buflen);
      if (num_chars < 0)
	break;
      if (num_chars > 0)
	{
	  /* If all characters are printable, print them.  Otherwise,
	     we're going to have to print an escape sequence.  We
	     check all characters because we want to print the target
	     bytes in the escape sequence, and we don't know character
	     boundaries there.  */
	  int i;

	  print_escape = 0;
	  for (i = 0; i < num_chars; ++i)
	    if (!wchar_printable (chars[i]))
	      {
		print_escape = 1;
		break;
	      }

	  if (!print_escape)
	    {
	      for (i = 0; i < num_chars; ++i)
		print_wchar (chars[i], buf, buflen,
			     TYPE_LENGTH (type), byte_order,
			     &wchar_buf, quoter, &need_escape);
	    }
	}

      /* This handles the NUM_CHARS == 0 case as well.  */
      if (print_escape)
	print_wchar (gdb_WEOF, buf, buflen, TYPE_LENGTH (type),
		     byte_order, &wchar_buf, quoter, &need_escape);
    }

  /* The output in the host encoding.  */
  obstack_init (&output);
  make_cleanup_obstack_free (&output);

  convert_between_encodings (INTERMEDIATE_ENCODING, host_charset (),
			     (gdb_byte *) obstack_base (&wchar_buf),
			     obstack_object_size (&wchar_buf),
			     sizeof (gdb_wchar_t), &output, translit_char);
  obstack_1grow (&output, '\0');

  fputs_filtered (obstack_base (&output), stream);

  do_cleanups (cleanups);
}

/* Return the repeat count of the next character/byte in ITER,
   storing the result in VEC.  */

static int
count_next_character (struct wchar_iterator *iter,
		      VEC (converted_character_d) **vec)
{
  struct converted_character *current;

  if (VEC_empty (converted_character_d, *vec))
    {
      struct converted_character tmp;
      gdb_wchar_t *chars;

      tmp.num_chars
	= wchar_iterate (iter, &tmp.result, &chars, &tmp.buf, &tmp.buflen);
      if (tmp.num_chars > 0)
	{
	  gdb_assert (tmp.num_chars < MAX_WCHARS);
	  memcpy (tmp.chars, chars, tmp.num_chars * sizeof (gdb_wchar_t));
	}
      VEC_safe_push (converted_character_d, *vec, &tmp);
    }

  current = VEC_last (converted_character_d, *vec);

  /* Count repeated characters or bytes.  */
  current->repeat_count = 1;
  if (current->num_chars == -1)
    {
      /* EOF  */
      return -1;
    }
  else
    {
      gdb_wchar_t *chars;
      struct converted_character d;
      int repeat;

      d.repeat_count = 0;

      while (1)
	{
	  /* Get the next character.  */
	  d.num_chars
	    = wchar_iterate (iter, &d.result, &chars, &d.buf, &d.buflen);

	  /* If a character was successfully converted, save the character
	     into the converted character.  */
	  if (d.num_chars > 0)
	    {
	      gdb_assert (d.num_chars < MAX_WCHARS);
	      memcpy (d.chars, chars, WCHAR_BUFLEN (d.num_chars));
	    }

	  /* Determine if the current character is the same as this
	     new character.  */
	  if (d.num_chars == current->num_chars && d.result == current->result)
	    {
	      /* There are two cases to consider:

		 1) Equality of converted character (num_chars > 0)
		 2) Equality of non-converted character (num_chars == 0)  */
	      if ((current->num_chars > 0
		   && memcmp (current->chars, d.chars,
			      WCHAR_BUFLEN (current->num_chars)) == 0)
		  || (current->num_chars == 0
		      && current->buflen == d.buflen
		      && memcmp (current->buf, d.buf, current->buflen) == 0))
		++current->repeat_count;
	      else
		break;
	    }
	  else
	    break;
	}

      /* Push this next converted character onto the result vector.  */
      repeat = current->repeat_count;
      VEC_safe_push (converted_character_d, *vec, &d);
      return repeat;
    }
}

/* Print the characters in CHARS to the OBSTACK.  QUOTE_CHAR is the quote
   character to use with string output.  WIDTH is the size of the output
   character type.  BYTE_ORDER is the the target byte order.  OPTIONS
   is the user's print options.  */

static void
print_converted_chars_to_obstack (struct obstack *obstack,
				  VEC (converted_character_d) *chars,
				  int quote_char, int width,
				  enum bfd_endian byte_order,
				  const struct value_print_options *options)
{
  unsigned int idx;
  struct converted_character *elem;
  enum {START, SINGLE, REPEAT, INCOMPLETE, FINISH} state, last;
  gdb_wchar_t wide_quote_char = gdb_btowc (quote_char);
  int need_escape = 0;

  /* Set the start state.  */
  idx = 0;
  last = state = START;
  elem = NULL;

  while (1)
    {
      switch (state)
	{
	case START:
	  /* Nothing to do.  */
	  break;

	case SINGLE:
	  {
	    int j;

	    /* We are outputting a single character
	       (< options->repeat_count_threshold).  */

	    if (last != SINGLE)
	      {
		/* We were outputting some other type of content, so we
		   must output and a comma and a quote.  */
		if (last != START)
		  obstack_grow_wstr (obstack, LCST (", "));
		obstack_grow (obstack, &wide_quote_char, sizeof (gdb_wchar_t));
	      }
	    /* Output the character.  */
	    for (j = 0; j < elem->repeat_count; ++j)
	      {
		if (elem->result == wchar_iterate_ok)
		  print_wchar (elem->chars[0], elem->buf, elem->buflen, width,
			       byte_order, obstack, quote_char, &need_escape);
		else
		  print_wchar (gdb_WEOF, elem->buf, elem->buflen, width,
			       byte_order, obstack, quote_char, &need_escape);
	      }
	  }
	  break;

	case REPEAT:
	  {
	    int j;
	    char *s;

	    /* We are outputting a character with a repeat count
	       greater than options->repeat_count_threshold.  */

	    if (last == SINGLE)
	      {
		/* We were outputting a single string.  Terminate the
		   string.  */
		obstack_grow (obstack, &wide_quote_char, sizeof (gdb_wchar_t));
	      }
	    if (last != START)
	      obstack_grow_wstr (obstack, LCST (", "));

	    /* Output the character and repeat string.  */
	    obstack_grow_wstr (obstack, LCST ("'"));
	    if (elem->result == wchar_iterate_ok)
	      print_wchar (elem->chars[0], elem->buf, elem->buflen, width,
			   byte_order, obstack, quote_char, &need_escape);
	    else
	      print_wchar (gdb_WEOF, elem->buf, elem->buflen, width,
			   byte_order, obstack, quote_char, &need_escape);
	    obstack_grow_wstr (obstack, LCST ("'"));
	    s = xstrprintf (_(" <repeats %u times>"), elem->repeat_count);
	    for (j = 0; s[j]; ++j)
	      {
		gdb_wchar_t w = gdb_btowc (s[j]);
		obstack_grow (obstack, &w, sizeof (gdb_wchar_t));
	      }
	    xfree (s);
	  }
	  break;

	case INCOMPLETE:
	  /* We are outputting an incomplete sequence.  */
	  if (last == SINGLE)
	    {
	      /* If we were outputting a string of SINGLE characters,
		 terminate the quote.  */
	      obstack_grow (obstack, &wide_quote_char, sizeof (gdb_wchar_t));
	    }
	  if (last != START)
	    obstack_grow_wstr (obstack, LCST (", "));

	  /* Output the incomplete sequence string.  */
	  obstack_grow_wstr (obstack, LCST ("<incomplete sequence "));
	  print_wchar (gdb_WEOF, elem->buf, elem->buflen, width, byte_order,
		       obstack, 0, &need_escape);
	  obstack_grow_wstr (obstack, LCST (">"));

	  /* We do not attempt to outupt anything after this.  */
	  state = FINISH;
	  break;

	case FINISH:
	  /* All done.  If we were outputting a string of SINGLE
	     characters, the string must be terminated.  Otherwise,
	     REPEAT and INCOMPLETE are always left properly terminated.  */
	  if (last == SINGLE)
	    obstack_grow (obstack, &wide_quote_char, sizeof (gdb_wchar_t));

	  return;
	}

      /* Get the next element and state.  */
      last = state;
      if (state != FINISH)
	{
	  elem = VEC_index (converted_character_d, chars, idx++);
	  switch (elem->result)
	    {
	    case wchar_iterate_ok:
	    case wchar_iterate_invalid:
	      if (elem->repeat_count > options->repeat_count_threshold)
		state = REPEAT;
	      else
		state = SINGLE;
	      break;

	    case wchar_iterate_incomplete:
	      state = INCOMPLETE;
	      break;

	    case wchar_iterate_eof:
	      state = FINISH;
	      break;
	    }
	}
    }
}

/* Print the character string STRING, printing at most LENGTH
   characters.  LENGTH is -1 if the string is nul terminated.  TYPE is
   the type of each character.  OPTIONS holds the printing options;
   printing stops early if the number hits print_max; repeat counts
   are printed as appropriate.  Print ellipses at the end if we had to
   stop before printing LENGTH characters, or if FORCE_ELLIPSES.
   QUOTE_CHAR is the character to print at each end of the string.  If
   C_STYLE_TERMINATOR is true, and the last character is 0, then it is
   omitted.  */

void
generic_printstr (struct ui_file *stream, struct type *type, 
		  const gdb_byte *string, unsigned int length, 
		  const char *encoding, int force_ellipses,
		  int quote_char, int c_style_terminator,
		  const struct value_print_options *options)
{
  enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (type));
  unsigned int i;
  int width = TYPE_LENGTH (type);
  struct obstack wchar_buf, output;
  struct cleanup *cleanup;
  struct wchar_iterator *iter;
  int finished = 0;
  struct converted_character *last;
  VEC (converted_character_d) *converted_chars;

  if (length == -1)
    {
      unsigned long current_char = 1;

      for (i = 0; current_char; ++i)
	{
	  QUIT;
	  current_char = extract_unsigned_integer (string + i * width,
						   width, byte_order);
	}
      length = i;
    }

  /* If the string was not truncated due to `set print elements', and
     the last byte of it is a null, we don't print that, in
     traditional C style.  */
  if (c_style_terminator
      && !force_ellipses
      && length > 0
      && (extract_unsigned_integer (string + (length - 1) * width,
				    width, byte_order) == 0))
    length--;

  if (length == 0)
    {
      fputs_filtered ("\"\"", stream);
      return;
    }

  /* Arrange to iterate over the characters, in wchar_t form.  */
  iter = make_wchar_iterator (string, length * width, encoding, width);
  cleanup = make_cleanup_wchar_iterator (iter);
  converted_chars = NULL;
  make_cleanup (VEC_cleanup (converted_character_d), &converted_chars);

  /* Convert characters until the string is over or the maximum
     number of printed characters has been reached.  */
  i = 0;
  while (i < options->print_max)
    {
      int r;

      QUIT;

      /* Grab the next character and repeat count.  */
      r = count_next_character (iter, &converted_chars);

      /* If less than zero, the end of the input string was reached.  */
      if (r < 0)
	break;

      /* Otherwise, add the count to the total print count and get
	 the next character.  */
      i += r;
    }

  /* Get the last element and determine if the entire string was
     processed.  */
  last = VEC_last (converted_character_d, converted_chars);
  finished = (last->result == wchar_iterate_eof);

  /* Ensure that CONVERTED_CHARS is terminated.  */
  last->result = wchar_iterate_eof;

  /* WCHAR_BUF is the obstack we use to represent the string in
     wchar_t form.  */
  obstack_init (&wchar_buf);
  make_cleanup_obstack_free (&wchar_buf);

  /* Print the output string to the obstack.  */
  print_converted_chars_to_obstack (&wchar_buf, converted_chars, quote_char,
				    width, byte_order, options);

  if (force_ellipses || !finished)
    obstack_grow_wstr (&wchar_buf, LCST ("..."));

  /* OUTPUT is where we collect `char's for printing.  */
  obstack_init (&output);
  make_cleanup_obstack_free (&output);

  convert_between_encodings (INTERMEDIATE_ENCODING, host_charset (),
			     (gdb_byte *) obstack_base (&wchar_buf),
			     obstack_object_size (&wchar_buf),
			     sizeof (gdb_wchar_t), &output, translit_char);
  obstack_1grow (&output, '\0');

  fputs_filtered (obstack_base (&output), stream);

  do_cleanups (cleanup);
}

/* Print a string from the inferior, starting at ADDR and printing up to LEN
   characters, of WIDTH bytes a piece, to STREAM.  If LEN is -1, printing
   stops at the first null byte, otherwise printing proceeds (including null
   bytes) until either print_max or LEN characters have been printed,
   whichever is smaller.  ENCODING is the name of the string's
   encoding.  It can be NULL, in which case the target encoding is
   assumed.  */

int
val_print_string (struct type *elttype, const char *encoding,
		  CORE_ADDR addr, int len,
		  struct ui_file *stream,
		  const struct value_print_options *options)
{
  int force_ellipsis = 0;	/* Force ellipsis to be printed if nonzero.  */
  int errcode;			/* Errno returned from bad reads.  */
  int found_nul;		/* Non-zero if we found the nul char.  */
  unsigned int fetchlimit;	/* Maximum number of chars to print.  */
  int bytes_read;
  gdb_byte *buffer = NULL;	/* Dynamically growable fetch buffer.  */
  struct cleanup *old_chain = NULL;	/* Top of the old cleanup chain.  */
  struct gdbarch *gdbarch = get_type_arch (elttype);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int width = TYPE_LENGTH (elttype);

  /* First we need to figure out the limit on the number of characters we are
     going to attempt to fetch and print.  This is actually pretty simple.  If
     LEN >= zero, then the limit is the minimum of LEN and print_max.  If
     LEN is -1, then the limit is print_max.  This is true regardless of
     whether print_max is zero, UINT_MAX (unlimited), or something in between,
     because finding the null byte (or available memory) is what actually
     limits the fetch.  */

  fetchlimit = (len == -1 ? options->print_max : min (len,
						      options->print_max));

  errcode = read_string (addr, len, width, fetchlimit, byte_order,
			 &buffer, &bytes_read);
  old_chain = make_cleanup (xfree, buffer);

  addr += bytes_read;

  /* We now have either successfully filled the buffer to fetchlimit,
     or terminated early due to an error or finding a null char when
     LEN is -1.  */

  /* Determine found_nul by looking at the last character read.  */
  found_nul = extract_unsigned_integer (buffer + bytes_read - width, width,
					byte_order) == 0;
  if (len == -1 && !found_nul)
    {
      gdb_byte *peekbuf;

      /* We didn't find a NUL terminator we were looking for.  Attempt
         to peek at the next character.  If not successful, or it is not
         a null byte, then force ellipsis to be printed.  */

      peekbuf = (gdb_byte *) alloca (width);

      if (target_read_memory (addr, peekbuf, width) == 0
	  && extract_unsigned_integer (peekbuf, width, byte_order) != 0)
	force_ellipsis = 1;
    }
  else if ((len >= 0 && errcode != 0) || (len > bytes_read / width))
    {
      /* Getting an error when we have a requested length, or fetching less
         than the number of characters actually requested, always make us
         print ellipsis.  */
      force_ellipsis = 1;
    }

  /* If we get an error before fetching anything, don't print a string.
     But if we fetch something and then get an error, print the string
     and then the error message.  */
  if (errcode == 0 || bytes_read > 0)
    {
      LA_PRINT_STRING (stream, elttype, buffer, bytes_read / width,
		       encoding, force_ellipsis, options);
    }

  if (errcode != 0)
    {
      if (errcode == EIO)
	{
	  fprintf_filtered (stream, "<Address ");
	  fputs_filtered (paddress (gdbarch, addr), stream);
	  fprintf_filtered (stream, " out of bounds>");
	}
      else
	{
	  fprintf_filtered (stream, "<Error reading address ");
	  fputs_filtered (paddress (gdbarch, addr), stream);
	  fprintf_filtered (stream, ": %s>", safe_strerror (errcode));
	}
    }

  gdb_flush (stream);
  do_cleanups (old_chain);

  return (bytes_read / width);
}


/* The 'set input-radix' command writes to this auxiliary variable.
   If the requested radix is valid, INPUT_RADIX is updated; otherwise,
   it is left unchanged.  */

static unsigned input_radix_1 = 10;

/* Validate an input or output radix setting, and make sure the user
   knows what they really did here.  Radix setting is confusing, e.g.
   setting the input radix to "10" never changes it!  */

static void
set_input_radix (char *args, int from_tty, struct cmd_list_element *c)
{
  set_input_radix_1 (from_tty, input_radix_1);
}

static void
set_input_radix_1 (int from_tty, unsigned radix)
{
  /* We don't currently disallow any input radix except 0 or 1, which don't
     make any mathematical sense.  In theory, we can deal with any input
     radix greater than 1, even if we don't have unique digits for every
     value from 0 to radix-1, but in practice we lose on large radix values.
     We should either fix the lossage or restrict the radix range more.
     (FIXME).  */

  if (radix < 2)
    {
      input_radix_1 = input_radix;
      error (_("Nonsense input radix ``decimal %u''; input radix unchanged."),
	     radix);
    }
  input_radix_1 = input_radix = radix;
  if (from_tty)
    {
      printf_filtered (_("Input radix now set to "
			 "decimal %u, hex %x, octal %o.\n"),
		       radix, radix, radix);
    }
}

/* The 'set output-radix' command writes to this auxiliary variable.
   If the requested radix is valid, OUTPUT_RADIX is updated,
   otherwise, it is left unchanged.  */

static unsigned output_radix_1 = 10;

static void
set_output_radix (char *args, int from_tty, struct cmd_list_element *c)
{
  set_output_radix_1 (from_tty, output_radix_1);
}

static void
set_output_radix_1 (int from_tty, unsigned radix)
{
  /* Validate the radix and disallow ones that we aren't prepared to
     handle correctly, leaving the radix unchanged.  */
  switch (radix)
    {
    case 16:
      user_print_options.output_format = 'x';	/* hex */
      break;
    case 10:
      user_print_options.output_format = 0;	/* decimal */
      break;
    case 8:
      user_print_options.output_format = 'o';	/* octal */
      break;
    default:
      output_radix_1 = output_radix;
      error (_("Unsupported output radix ``decimal %u''; "
	       "output radix unchanged."),
	     radix);
    }
  output_radix_1 = output_radix = radix;
  if (from_tty)
    {
      printf_filtered (_("Output radix now set to "
			 "decimal %u, hex %x, octal %o.\n"),
		       radix, radix, radix);
    }
}

/* Set both the input and output radix at once.  Try to set the output radix
   first, since it has the most restrictive range.  An radix that is valid as
   an output radix is also valid as an input radix.

   It may be useful to have an unusual input radix.  If the user wishes to
   set an input radix that is not valid as an output radix, he needs to use
   the 'set input-radix' command.  */

static void
set_radix (char *arg, int from_tty)
{
  unsigned radix;

  radix = (arg == NULL) ? 10 : parse_and_eval_long (arg);
  set_output_radix_1 (0, radix);
  set_input_radix_1 (0, radix);
  if (from_tty)
    {
      printf_filtered (_("Input and output radices now set to "
			 "decimal %u, hex %x, octal %o.\n"),
		       radix, radix, radix);
    }
}

/* Show both the input and output radices.  */

static void
show_radix (char *arg, int from_tty)
{
  if (from_tty)
    {
      if (input_radix == output_radix)
	{
	  printf_filtered (_("Input and output radices set to "
			     "decimal %u, hex %x, octal %o.\n"),
			   input_radix, input_radix, input_radix);
	}
      else
	{
	  printf_filtered (_("Input radix set to decimal "
			     "%u, hex %x, octal %o.\n"),
			   input_radix, input_radix, input_radix);
	  printf_filtered (_("Output radix set to decimal "
			     "%u, hex %x, octal %o.\n"),
			   output_radix, output_radix, output_radix);
	}
    }
}


static void
set_print (char *arg, int from_tty)
{
  printf_unfiltered (
     "\"set print\" must be followed by the name of a print subcommand.\n");
  help_list (setprintlist, "set print ", -1, gdb_stdout);
}

static void
show_print (char *args, int from_tty)
{
  cmd_show_list (showprintlist, from_tty, "");
}

void
_initialize_valprint (void)
{
  add_prefix_cmd ("print", no_class, set_print,
		  _("Generic command for setting how things print."),
		  &setprintlist, "set print ", 0, &setlist);
  add_alias_cmd ("p", "print", no_class, 1, &setlist);
  /* Prefer set print to set prompt.  */
  add_alias_cmd ("pr", "print", no_class, 1, &setlist);

  add_prefix_cmd ("print", no_class, show_print,
		  _("Generic command for showing print settings."),
		  &showprintlist, "show print ", 0, &showlist);
  add_alias_cmd ("p", "print", no_class, 1, &showlist);
  add_alias_cmd ("pr", "print", no_class, 1, &showlist);

  add_setshow_uinteger_cmd ("elements", no_class,
			    &user_print_options.print_max, _("\
Set limit on string chars or array elements to print."), _("\
Show limit on string chars or array elements to print."), _("\
\"set print elements 0\" causes there to be no limit."),
			    NULL,
			    show_print_max,
			    &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("null-stop", no_class,
			   &user_print_options.stop_print_at_null, _("\
Set printing of char arrays to stop at first null char."), _("\
Show printing of char arrays to stop at first null char."), NULL,
			   NULL,
			   show_stop_print_at_null,
			   &setprintlist, &showprintlist);

  add_setshow_uinteger_cmd ("repeats", no_class,
			    &user_print_options.repeat_count_threshold, _("\
Set threshold for repeated print elements."), _("\
Show threshold for repeated print elements."), _("\
\"set print repeats 0\" causes all elements to be individually printed."),
			    NULL,
			    show_repeat_count_threshold,
			    &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("pretty", class_support,
			   &user_print_options.prettyprint_structs, _("\
Set prettyprinting of structures."), _("\
Show prettyprinting of structures."), NULL,
			   NULL,
			   show_prettyprint_structs,
			   &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("union", class_support,
			   &user_print_options.unionprint, _("\
Set printing of unions interior to structures."), _("\
Show printing of unions interior to structures."), NULL,
			   NULL,
			   show_unionprint,
			   &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("array", class_support,
			   &user_print_options.prettyprint_arrays, _("\
Set prettyprinting of arrays."), _("\
Show prettyprinting of arrays."), NULL,
			   NULL,
			   show_prettyprint_arrays,
			   &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("address", class_support,
			   &user_print_options.addressprint, _("\
Set printing of addresses."), _("\
Show printing of addresses."), NULL,
			   NULL,
			   show_addressprint,
			   &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("symbol", class_support,
			   &user_print_options.symbol_print, _("\
Set printing of symbol names when printing pointers."), _("\
Show printing of symbol names when printing pointers."),
			   NULL, NULL,
			   show_symbol_print,
			   &setprintlist, &showprintlist);

  add_setshow_zuinteger_cmd ("input-radix", class_support, &input_radix_1,
			     _("\
Set default input radix for entering numbers."), _("\
Show default input radix for entering numbers."), NULL,
			     set_input_radix,
			     show_input_radix,
			     &setlist, &showlist);

  add_setshow_zuinteger_cmd ("output-radix", class_support, &output_radix_1,
			     _("\
Set default output radix for printing of values."), _("\
Show default output radix for printing of values."), NULL,
			     set_output_radix,
			     show_output_radix,
			     &setlist, &showlist);

  /* The "set radix" and "show radix" commands are special in that
     they are like normal set and show commands but allow two normally
     independent variables to be either set or shown with a single
     command.  So the usual deprecated_add_set_cmd() and [deleted]
     add_show_from_set() commands aren't really appropriate.  */
  /* FIXME: i18n: With the new add_setshow_integer command, that is no
     longer true - show can display anything.  */
  add_cmd ("radix", class_support, set_radix, _("\
Set default input and output number radices.\n\
Use 'set input-radix' or 'set output-radix' to independently set each.\n\
Without an argument, sets both radices back to the default value of 10."),
	   &setlist);
  add_cmd ("radix", class_support, show_radix, _("\
Show the default input and output number radices.\n\
Use 'show input-radix' or 'show output-radix' to independently show each."),
	   &showlist);

  add_setshow_boolean_cmd ("array-indexes", class_support,
                           &user_print_options.print_array_indexes, _("\
Set printing of array indexes."), _("\
Show printing of array indexes"), NULL, NULL, show_print_array_indexes,
                           &setprintlist, &showprintlist);
}
