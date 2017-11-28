/* Rust language support routines for GDB, the GNU debugger.

   Copyright (C) 2016 Free Software Foundation, Inc.

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

#include "block.h"
#include "c-lang.h"
#include "charset.h"
#include "cp-support.h"
#include "demangle.h"
#include "gdbarch.h"
#include "infcall.h"
#include "objfiles.h"
#include "rust-lang.h"
#include "valprint.h"
#include "varobj.h"

extern initialize_file_ftype _initialize_rust_language;

/* Returns the last segment of a Rust path like foo::bar::baz.  Will
   not handle cases where the last segment contains generics.  This
   will return NULL if the last segment cannot be found.  */

static const char *
rust_last_path_segment (const char * path)
{
  const char *result = strrchr (path, ':');

  if (result == NULL)
    return NULL;
  return result + 1;
}

/* Find the Rust crate for BLOCK.  If no crate can be found, returns
   NULL.  Otherwise, returns a newly allocated string that the caller
   is responsible for freeing.  */

char *
rust_crate_for_block (const struct block *block)
{
  const char *scope = block_scope (block);

  if (scope[0] == '\0')
    return NULL;

  return xstrndup (scope, cp_find_first_component (scope));
}

/* Information about the discriminant/variant of an enum */

struct disr_info
{
  /* Name of field.  Must be freed by caller.  */
  char *name;
  /* Field number in union.  Negative on error.  For an encoded enum,
     the "hidden" member will always be field 1, and the "real" member
     will always be field 0.  */
  int field_no;
  /* True if this is an encoded enum that has a single "real" member
     and a single "hidden" member.  */
  unsigned int is_encoded : 1;
};

/* The prefix of a specially-encoded enum.  */

#define RUST_ENUM_PREFIX "RUST$ENCODED$ENUM$"

/* The number of the real field.  */

#define RUST_ENCODED_ENUM_REAL 0

/* The number of the hidden field.  */

#define RUST_ENCODED_ENUM_HIDDEN 1

/* Utility function to get discriminant info for a given value.  */

static struct disr_info
rust_get_disr_info (struct type *type, const gdb_byte *valaddr,
                    int embedded_offset, CORE_ADDR address,
                    const struct value *val)
{
  int i;
  struct disr_info ret;
  struct type *disr_type;
  struct ui_file *temp_file;
  struct value_print_options opts;
  struct cleanup *cleanup;
  const char *name_segment;

  get_no_prettyformat_print_options (&opts);

  ret.field_no = -1;
  ret.is_encoded = 0;

  if (TYPE_NFIELDS (type) == 0)
    error (_("Encountered void enum value"));

  /* If an enum has two values where one is empty and the other holds
     a pointer that cannot be zero; then the Rust compiler optimizes
     away the discriminant and instead uses a zero value in the
     pointer field to indicate the empty variant.  */
  if (strncmp (TYPE_FIELD_NAME (type, 0), RUST_ENUM_PREFIX,
	       strlen (RUST_ENUM_PREFIX)) == 0)
    {
      char *tail, *token, *name, *saveptr = NULL;
      unsigned long fieldno;
      struct type *member_type;
      LONGEST value;

      ret.is_encoded = 1;

      if (TYPE_NFIELDS (type) != 1)
	error (_("Only expected one field in %s type"), RUST_ENUM_PREFIX);

      /* Optimized enums have only one field.  */
      member_type = TYPE_FIELD_TYPE (type, 0);

      name = xstrdup (TYPE_FIELD_NAME (type, 0));
      cleanup = make_cleanup (xfree, name);
      tail = name + strlen (RUST_ENUM_PREFIX);

      /* The location of the value that doubles as a discriminant is
         stored in the name of the field, as
         RUST$ENCODED$ENUM$<fieldno>$<fieldno>$...$<variantname>
         where the fieldnos are the indices of the fields that should be
         traversed in order to find the field (which may be several fields deep)
         and the variantname is the name of the variant of the case when the
         field is zero.  */
      for (token = strtok_r (tail, "$", &saveptr);
           token != NULL;
           token = strtok_r (NULL, "$", &saveptr))
        {
	  if (sscanf (token, "%lu", &fieldno) != 1)
	    {
	      /* We have reached the enum name, which cannot start
		 with a digit.  */
	      break;
	    }
          if (fieldno >= TYPE_NFIELDS (member_type))
	    error (_("%s refers to field after end of member type"),
		   RUST_ENUM_PREFIX);

          embedded_offset += TYPE_FIELD_BITPOS (member_type, fieldno) / 8;
          member_type = TYPE_FIELD_TYPE (member_type, fieldno);
        }

      if (token == NULL)
	error (_("Invalid form for %s"), RUST_ENUM_PREFIX);
      value = unpack_long (member_type, valaddr + embedded_offset);

      if (value == 0)
	{
	  ret.field_no = RUST_ENCODED_ENUM_HIDDEN;
	  ret.name = concat (TYPE_NAME (type), "::", token, (char *) NULL);
	}
      else
	{
	  ret.field_no = RUST_ENCODED_ENUM_REAL;
	  ret.name = concat (TYPE_NAME (type), "::",
			     rust_last_path_segment (TYPE_NAME (TYPE_FIELD_TYPE (type, 0))),
			     (char *) NULL);
	}

      do_cleanups (cleanup);
      return ret;
    }

  disr_type = TYPE_FIELD_TYPE (type, 0);

  if (TYPE_NFIELDS (disr_type) == 0)
    {
      /* This is a bounds check and should never be hit unless Rust
	 has changed its debuginfo format.  */
      error (_("Could not find enum discriminant field"));
    }

  if (strcmp (TYPE_FIELD_NAME (disr_type, 0), "RUST$ENUM$DISR") != 0)
    error (_("Rust debug format has changed"));

  temp_file = mem_fileopen ();
  cleanup = make_cleanup_ui_file_delete (temp_file);
  /* The first value of the first field (or any field)
     is the discriminant value.  */
  c_val_print (TYPE_FIELD_TYPE (disr_type, 0), valaddr,
	       (embedded_offset + TYPE_FIELD_BITPOS (type, 0) / 8
		+ TYPE_FIELD_BITPOS (disr_type, 0) / 8),
	       address, temp_file,
	       0, val, &opts);

  ret.name = ui_file_xstrdup (temp_file, NULL);
  name_segment = rust_last_path_segment (ret.name);
  if (name_segment != NULL)
    {
      for (i = 0; i < TYPE_NFIELDS (type); ++i)
	{
	  /* Sadly, the discriminant value paths do not match the type
	     field name paths ('core::option::Option::Some' vs
	     'core::option::Some').  However, enum variant names are
	     unique in the last path segment and the generics are not
	     part of this path, so we can just compare those.  This is
	     hackish and would be better fixed by improving rustc's
	     metadata for enums.  */
	  const char *field_type = TYPE_NAME (TYPE_FIELD_TYPE (type, i));

	  if (field_type != NULL
	      && strcmp (name_segment,
			 rust_last_path_segment (field_type)) == 0)
	    {
	      ret.field_no = i;
	      break;
	    }
	}
    }

  if (ret.field_no == -1 && ret.name != NULL)
    {
      /* Somehow the discriminant wasn't found.  */
      make_cleanup (xfree, ret.name);
      error (_("Could not find variant of %s with discriminant %s"),
	     TYPE_TAG_NAME (type), ret.name);
    }

  do_cleanups (cleanup);
  return ret;
}

/* See rust-lang.h.  */

int
rust_tuple_type_p (struct type *type)
{
  /* The current implementation is a bit of a hack, but there's
     nothing else in the debuginfo to distinguish a tuple from a
     struct.  */
  return (TYPE_CODE (type) == TYPE_CODE_STRUCT
	  && TYPE_TAG_NAME (type) != NULL
	  && TYPE_TAG_NAME (type)[0] == '(');
}


/* Return true if all non-static fields of a structlike type are in a
   sequence like __0, __1, __2.  OFFSET lets us skip fields.  */

static int
rust_underscore_fields (struct type *type, int offset)
{
  int i, field_number;

  field_number = 0;

  if (TYPE_CODE (type) != TYPE_CODE_STRUCT)
    return 0;
  for (i = 0; i < TYPE_NFIELDS (type); ++i)
    {
      if (!field_is_static (&TYPE_FIELD (type, i)))
	{
	  if (offset > 0)
	    offset--;
	  else
	    {
	      char buf[20];

	      xsnprintf (buf, sizeof (buf), "__%d", field_number);
	      if (strcmp (buf, TYPE_FIELD_NAME (type, i)) != 0)
		return 0;
	      field_number++;
	    }
	}
    }
  return 1;
}

/* See rust-lang.h.  */

int
rust_tuple_struct_type_p (struct type *type)
{
  /* This is just an approximation until DWARF can represent Rust more
     precisely.  We exclude zero-length structs because they may not
     be tuple structs, and there's no way to tell.  */
  return TYPE_NFIELDS (type) > 0 && rust_underscore_fields (type, 0);
}

/* Return true if a variant TYPE is a tuple variant, false otherwise.  */

static int
rust_tuple_variant_type_p (struct type *type)
{
  /* First field is discriminant */
  return rust_underscore_fields (type, 1);
}

/* Return true if TYPE is a slice type, otherwise false.  */

static int
rust_slice_type_p (struct type *type)
{
  return (TYPE_CODE (type) == TYPE_CODE_STRUCT
	  && TYPE_TAG_NAME (type) != NULL
	  && strncmp (TYPE_TAG_NAME (type), "&[", 2) == 0);
}

/* Return true if TYPE is a range type, otherwise false.  */

static int
rust_range_type_p (struct type *type)
{
  int i;

  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
      || TYPE_NFIELDS (type) > 2
      || TYPE_TAG_NAME (type) == NULL
      || strstr (TYPE_TAG_NAME (type), "::Range") == NULL)
    return 0;

  if (TYPE_NFIELDS (type) == 0)
    return 1;

  i = 0;
  if (strcmp (TYPE_FIELD_NAME (type, 0), "start") == 0)
    {
      if (TYPE_NFIELDS (type) == 1)
	return 1;
      i = 1;
    }
  else if (TYPE_NFIELDS (type) == 2)
    {
      /* First field had to be "start".  */
      return 0;
    }

  return strcmp (TYPE_FIELD_NAME (type, i), "end") == 0;
}

/* Return true if TYPE seems to be the type "u8", otherwise false.  */

static int
rust_u8_type_p (struct type *type)
{
  return (TYPE_CODE (type) == TYPE_CODE_INT
	  && TYPE_UNSIGNED (type)
	  && TYPE_LENGTH (type) == 1);
}

/* Return true if TYPE is a Rust character type.  */

static int
rust_chartype_p (struct type *type)
{
  return (TYPE_CODE (type) == TYPE_CODE_CHAR
	  && TYPE_LENGTH (type) == 4
	  && TYPE_UNSIGNED (type));
}



/* la_emitchar implementation for Rust.  */

static void
rust_emitchar (int c, struct type *type, struct ui_file *stream, int quoter)
{
  if (!rust_chartype_p (type))
    generic_emit_char (c, type, stream, quoter,
		       target_charset (get_type_arch (type)));
  else if (c == '\\' || c == quoter)
    fprintf_filtered (stream, "\\%c", c);
  else if (c == '\n')
    fputs_filtered ("\\n", stream);
  else if (c == '\r')
    fputs_filtered ("\\r", stream);
  else if (c == '\t')
    fputs_filtered ("\\t", stream);
  else if (c == '\0')
    fputs_filtered ("\\0", stream);
  else if (c >= 32 && c <= 127 && isprint (c))
    fputc_filtered (c, stream);
  else if (c <= 255)
    fprintf_filtered (stream, "\\x%02x", c);
  else
    fprintf_filtered (stream, "\\u{%06x}", c);
}

/* la_printchar implementation for Rust.  */

static void
rust_printchar (int c, struct type *type, struct ui_file *stream)
{
  fputs_filtered ("'", stream);
  LA_EMIT_CHAR (c, type, stream, '\'');
  fputs_filtered ("'", stream);
}

/* la_printstr implementation for Rust.  */

static void
rust_printstr (struct ui_file *stream, struct type *type,
	       const gdb_byte *string, unsigned int length,
	       const char *user_encoding, int force_ellipses,
	       const struct value_print_options *options)
{
  /* Rust always uses UTF-8, but let the caller override this if need
     be.  */
  const char *encoding = user_encoding;
  if (user_encoding == NULL || !*user_encoding)
    {
      /* In Rust strings, characters are "u8".  */
      if (rust_u8_type_p (type))
	encoding = "UTF-8";
      else
	{
	  /* This is probably some C string, so let's let C deal with
	     it.  */
	  c_printstr (stream, type, string, length, user_encoding,
		      force_ellipses, options);
	  return;
	}
    }

  /* This is not ideal as it doesn't use our character printer.  */
  generic_printstr (stream, type, string, length, encoding, force_ellipses,
		    '"', 0, options);
}



static const struct generic_val_print_decorations rust_decorations =
{
  /* Complex isn't used in Rust, but we provide C-ish values just in
     case.  */
  "",
  " + ",
  " * I",
  "true",
  "false",
  "()",
  "[",
  "]"
};

/* la_val_print implementation for Rust.  */

static void
rust_val_print (struct type *type, const gdb_byte *valaddr, int embedded_offset,
		CORE_ADDR address, struct ui_file *stream, int recurse,
		const struct value *val,
		const struct value_print_options *options)
{
  type = check_typedef (type);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_PTR:
      {
	LONGEST low_bound, high_bound;
	
	if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_ARRAY
	    && rust_u8_type_p (TYPE_TARGET_TYPE (TYPE_TARGET_TYPE (type)))
	    && get_array_bounds (TYPE_TARGET_TYPE (type), &low_bound,
				 &high_bound)) {
	  /* We have a pointer to a byte string, so just print
	     that.  */
	  struct type *elttype = check_typedef (TYPE_TARGET_TYPE (type));
	  CORE_ADDR addr;
	  struct gdbarch *arch = get_type_arch (type);
	  int unit_size = gdbarch_addressable_memory_unit_size (arch);

	  addr = unpack_pointer (type, valaddr + embedded_offset * unit_size);
	  if (options->addressprint)
	    {
	      fputs_filtered (paddress (arch, addr), stream);
	      fputs_filtered (" ", stream);
	    }

	  fputs_filtered ("b", stream);
	  val_print_string (TYPE_TARGET_TYPE (elttype), "ASCII", addr,
			    high_bound - low_bound + 1, stream,
			    options);
	  break;
	}
      }
      /* Fall through.  */

    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_MEMBERPTR:
      c_val_print (type, valaddr, embedded_offset, address, stream,
		   recurse, val, options);
      break;

    case TYPE_CODE_INT:
      /* Recognize the unit type.  */
      if (TYPE_UNSIGNED (type) && TYPE_LENGTH (type) == 0
	  && TYPE_NAME (type) != NULL && strcmp (TYPE_NAME (type), "()") == 0)
	{
	  fputs_filtered ("()", stream);
	  break;
	}
      goto generic_print;

    case TYPE_CODE_STRING:
      {
	struct gdbarch *arch = get_type_arch (type);
	int unit_size = gdbarch_addressable_memory_unit_size (arch);
	LONGEST low_bound, high_bound;

	if (!get_array_bounds (type, &low_bound, &high_bound))
	  error (_("Could not determine the array bounds"));

	/* If we see a plain TYPE_CODE_STRING, then we're printing a
	   byte string, hence the choice of "ASCII" as the
	   encoding.  */
	fputs_filtered ("b", stream);
	rust_printstr (stream, TYPE_TARGET_TYPE (type),
		       valaddr + embedded_offset * unit_size,
		       high_bound - low_bound + 1, "ASCII", 0, options);
      }
      break;

    case TYPE_CODE_ARRAY:
      {
	LONGEST low_bound, high_bound;

	if (get_array_bounds (type, &low_bound, &high_bound)
	    && high_bound - low_bound + 1 == 0)
	  fputs_filtered ("[]", stream);
	else
	  goto generic_print;
      }
      break;

    case TYPE_CODE_UNION:
      {
	int j, nfields, first_field, is_tuple, start;
	struct type *variant_type;
	struct disr_info disr;
	struct value_print_options opts;
	struct cleanup *cleanup;

	opts = *options;
	opts.deref_ref = 0;

	disr = rust_get_disr_info (type, valaddr, embedded_offset, address,
				   val);
	cleanup = make_cleanup (xfree, disr.name);

	if (disr.is_encoded && disr.field_no == RUST_ENCODED_ENUM_HIDDEN)
	  {
	    fprintf_filtered (stream, "%s", disr.name);
	    goto cleanup;
	  }

	first_field = 1;
	variant_type = TYPE_FIELD_TYPE (type, disr.field_no);
	nfields = TYPE_NFIELDS (variant_type);

	is_tuple = (disr.is_encoded
		    ? rust_tuple_struct_type_p (variant_type)
		    : rust_tuple_variant_type_p (variant_type));
	start = disr.is_encoded ? 0 : 1;

	if (nfields > start)
	  {
	    /* In case of a non-nullary variant, we output 'Foo(x,y,z)'. */
	    if (is_tuple)
	      fprintf_filtered (stream, "%s(", disr.name);
	    else
	      {
		/* struct variant.  */
		fprintf_filtered (stream, "%s{", disr.name);
	      }
	  }
	else
	  {
	    /* In case of a nullary variant like 'None', just output
	       the name. */
	    fprintf_filtered (stream, "%s", disr.name);
	    goto cleanup;
	  }

	for (j = start; j < TYPE_NFIELDS (variant_type); j++)
	  {
	    if (!first_field)
	      fputs_filtered (", ", stream);
	    first_field = 0;

	    if (!is_tuple)
	      fprintf_filtered (stream, "%s: ",
				TYPE_FIELD_NAME (variant_type, j));

	    val_print (TYPE_FIELD_TYPE (variant_type, j),
		       valaddr,
		       (embedded_offset
			+ TYPE_FIELD_BITPOS (type, disr.field_no) / 8
			+ TYPE_FIELD_BITPOS (variant_type, j) / 8),
		       address,
		       stream, recurse + 1, val, &opts,
		       current_language);
	  }

	if (is_tuple)
	  fputs_filtered (")", stream);
	else
	  fputs_filtered ("}", stream);

      cleanup:
	do_cleanups (cleanup);
      }
      break;

    case TYPE_CODE_STRUCT:
      {
	int i;
	int first_field;
	int is_tuple = rust_tuple_type_p (type);
	int is_tuple_struct = !is_tuple && rust_tuple_struct_type_p (type);
	struct value_print_options opts;

	if (!is_tuple)
	  {
	    if (TYPE_TAG_NAME (type) != NULL)
	      fprintf_filtered (stream, "%s", TYPE_TAG_NAME (type));

	    if (TYPE_NFIELDS (type) == 0)
	      break;

	    if (TYPE_TAG_NAME (type) != NULL)
	      fputs_filtered (" ", stream);
	  }

	if (is_tuple || is_tuple_struct)
	  fputs_filtered ("(", stream);
	else
	  fputs_filtered ("{", stream);

	opts = *options;
	opts.deref_ref = 0;

	first_field = 1;
	for (i = 0; i < TYPE_NFIELDS (type); ++i)
	  {
	    if (field_is_static (&TYPE_FIELD (type, i)))
	      continue;

	    if (!first_field)
	      fputs_filtered (",", stream);

	    if (options->prettyformat)
	      {
		fputs_filtered ("\n", stream);
		print_spaces_filtered (2 + 2 * recurse, stream);
	      }
	    else if (!first_field)
	      fputs_filtered (" ", stream);

	    first_field = 0;

	    if (!is_tuple && !is_tuple_struct)
	      {
		fputs_filtered (TYPE_FIELD_NAME (type, i), stream);
		fputs_filtered (": ", stream);
	      }

	    val_print (TYPE_FIELD_TYPE (type, i),
		       valaddr,
		       embedded_offset + TYPE_FIELD_BITPOS (type, i) / 8,
		       address,
		       stream, recurse + 1, val, &opts,
		       current_language);
	  }

	if (options->prettyformat)
	  {
	    fputs_filtered ("\n", stream);
	    print_spaces_filtered (2 * recurse, stream);
	  }

	if (is_tuple || is_tuple_struct)
	  fputs_filtered (")", stream);
	else
	  fputs_filtered ("}", stream);
      }
      break;

    default:
    generic_print:
      /* Nothing special yet.  */
      generic_val_print (type, valaddr, embedded_offset, address, stream,
			 recurse, val, options, &rust_decorations);
    }
}



/* la_print_typedef implementation for Rust.  */

static void
rust_print_typedef (struct type *type,
		    struct symbol *new_symbol,
		    struct ui_file *stream)
{
  type = check_typedef (type);
  fprintf_filtered (stream, "type %s = ", SYMBOL_PRINT_NAME (new_symbol));
  type_print (type, "", stream, 0);
  fprintf_filtered (stream, ";\n");
}

/* la_print_type implementation for Rust.  */

static void
rust_print_type (struct type *type, const char *varstring,
		 struct ui_file *stream, int show, int level,
		 const struct type_print_options *flags)
{
  int i;

  QUIT;
  if (show <= 0
      && TYPE_NAME (type) != NULL)
    {
      /* Rust calls the unit type "void" in its debuginfo,
         but we don't want to print it as that.  */
      if (TYPE_CODE (type) == TYPE_CODE_VOID)
        fputs_filtered ("()", stream);
      else
        fputs_filtered (TYPE_NAME (type), stream);
      return;
    }

  type = check_typedef (type);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_VOID:
      fputs_filtered ("()", stream);
      break;

    case TYPE_CODE_FUNC:
      /* Delegate varargs to the C printer.  */
      if (TYPE_VARARGS (type))
	goto c_printer;

      fputs_filtered ("fn ", stream);
      if (varstring != NULL)
	fputs_filtered (varstring, stream);
      fputs_filtered ("(", stream);
      for (i = 0; i < TYPE_NFIELDS (type); ++i)
	{
	  QUIT;
	  if (i > 0)
	    fputs_filtered (", ", stream);
	  rust_print_type (TYPE_FIELD_TYPE (type, i), "", stream, -1, 0,
			   flags);
	}
      fputs_filtered (")", stream);
      /* If it returns unit, we can omit the return type.  */
      if (TYPE_CODE (TYPE_TARGET_TYPE (type)) != TYPE_CODE_VOID)
        {
          fputs_filtered (" -> ", stream);
          rust_print_type (TYPE_TARGET_TYPE (type), "", stream, -1, 0, flags);
        }
      break;

    case TYPE_CODE_ARRAY:
      {
	LONGEST low_bound, high_bound;

	fputs_filtered ("[", stream);
	rust_print_type (TYPE_TARGET_TYPE (type), NULL,
			 stream, show - 1, level, flags);
	fputs_filtered ("; ", stream);

	if (TYPE_HIGH_BOUND_KIND (TYPE_INDEX_TYPE (type)) == PROP_LOCEXPR
	    || TYPE_HIGH_BOUND_KIND (TYPE_INDEX_TYPE (type)) == PROP_LOCLIST)
	  fprintf_filtered (stream, "variable length");
	else if (get_array_bounds (type, &low_bound, &high_bound))
	  fprintf_filtered (stream, "%s", 
			    plongest (high_bound - low_bound + 1));
	fputs_filtered ("]", stream);
      }
      break;

    case TYPE_CODE_STRUCT:
      {
	int is_tuple_struct;

	/* Print a tuple type simply.  */
	if (rust_tuple_type_p (type))
	  {
	    fputs_filtered (TYPE_TAG_NAME (type), stream);
	    break;
	  }

	/* If we see a base class, delegate to C.  */
	if (TYPE_N_BASECLASSES (type) > 0)
	  goto c_printer;

	fputs_filtered ("struct ", stream);
	if (TYPE_TAG_NAME (type) != NULL)
	  fputs_filtered (TYPE_TAG_NAME (type), stream);

	is_tuple_struct = rust_tuple_struct_type_p (type);

	if (TYPE_NFIELDS (type) == 0 && !rust_tuple_type_p (type))
	  break;
	fputs_filtered (is_tuple_struct ? " (\n" : " {\n", stream);

	for (i = 0; i < TYPE_NFIELDS (type); ++i)
	  {
	    const char *name;

	    QUIT;
	    if (field_is_static (&TYPE_FIELD (type, i)))
	      continue;

	    /* We'd like to print "pub" here as needed, but rustc
	       doesn't emit the debuginfo, and our types don't have
	       cplus_struct_type attached.  */

	    /* For a tuple struct we print the type but nothing
	       else.  */
	    print_spaces_filtered (level + 2, stream);
	    if (!is_tuple_struct)
	      fprintf_filtered (stream, "%s: ", TYPE_FIELD_NAME (type, i));

	    rust_print_type (TYPE_FIELD_TYPE (type, i), NULL,
			     stream, show - 1, level + 2,
			     flags);
	    fputs_filtered (",\n", stream);
	  }

	fprintfi_filtered (level, stream, is_tuple_struct ? ")" : "}");
      }
      break;

    case TYPE_CODE_ENUM:
      {
	int i, len = 0;

	fputs_filtered ("enum ", stream);
	if (TYPE_TAG_NAME (type) != NULL)
	  {
	    fputs_filtered (TYPE_TAG_NAME (type), stream);
	    fputs_filtered (" ", stream);
	    len = strlen (TYPE_TAG_NAME (type));
	  }
	fputs_filtered ("{\n", stream);

	for (i = 0; i < TYPE_NFIELDS (type); ++i)
	  {
	    const char *name = TYPE_FIELD_NAME (type, i);

	    QUIT;

	    if (len > 0
		&& strncmp (name, TYPE_TAG_NAME (type), len) == 0
		&& name[len] == ':'
		&& name[len + 1] == ':')
	      name += len + 2;
	    fprintfi_filtered (level + 2, stream, "%s,\n", name);
	  }

	fputs_filtered ("}", stream);
      }
      break;

    case TYPE_CODE_UNION:
      {
	/* ADT enums.  */
	int i, len = 0;
	/* Skip the discriminant field.  */
	int skip_to = 1;

	fputs_filtered ("enum ", stream);
	if (TYPE_TAG_NAME (type) != NULL)
	  {
	    fputs_filtered (TYPE_TAG_NAME (type), stream);
	    fputs_filtered (" ", stream);
	  }
	fputs_filtered ("{\n", stream);

	if (strncmp (TYPE_FIELD_NAME (type, 0), RUST_ENUM_PREFIX,
		     strlen (RUST_ENUM_PREFIX)) == 0)
	  {
	    const char *zero_field = strrchr (TYPE_FIELD_NAME (type, 0), '$');
	    if (zero_field != NULL && strlen (zero_field) > 1)
	      {
		fprintfi_filtered (level + 2, stream, "%s,\n", zero_field + 1);
		/* There is no explicit discriminant field, skip nothing.  */
		skip_to = 0;
	      }
	  }

	for (i = 0; i < TYPE_NFIELDS (type); ++i)
	  {
	    struct type *variant_type = TYPE_FIELD_TYPE (type, i);
	    const char *name
	      = rust_last_path_segment (TYPE_NAME (variant_type));

	    fprintfi_filtered (level + 2, stream, "%s", name);

	    if (TYPE_NFIELDS (variant_type) > skip_to)
	      {
		int first = 1;
		int is_tuple = rust_tuple_variant_type_p (variant_type);
		int j;

		fputs_filtered (is_tuple ? "(" : "{", stream);
		for (j = skip_to; j < TYPE_NFIELDS (variant_type); j++)
		  {
		    if (first)
		      first = 0;
		    else
		      fputs_filtered (", ", stream);

		    if (!is_tuple)
		      fprintf_filtered (stream, "%s: ",
					TYPE_FIELD_NAME (variant_type, j));

		    rust_print_type (TYPE_FIELD_TYPE (variant_type, j), NULL,
				     stream, show - 1, level + 2,
				     flags);
		  }
		fputs_filtered (is_tuple ? ")" : "}", stream);
	      }

	    fputs_filtered (",\n", stream);
	  }

	fputs_filtered ("}", stream);
      }
      break;

    default:
    c_printer:
      c_print_type (type, varstring, stream, show, level, flags);
    }
}



/* Compute the alignment of the type T.  */

static int
rust_type_alignment (struct type *t)
{
  t = check_typedef (t);
  switch (TYPE_CODE (t))
    {
    default:
      error (_("Could not compute alignment of type"));

    case TYPE_CODE_PTR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_REF:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
      return TYPE_LENGTH (t);

    case TYPE_CODE_ARRAY:
    case TYPE_CODE_COMPLEX:
      return rust_type_alignment (TYPE_TARGET_TYPE (t));

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      {
	int i;
	int align = 1;

	for (i = 0; i < TYPE_NFIELDS (t); ++i)
	  {
	    int a = rust_type_alignment (TYPE_FIELD_TYPE (t, i));
	    if (a > align)
	      align = a;
	  }
	return align;
      }
    }
}

/* Like arch_composite_type, but uses TYPE to decide how to allocate
   -- either on an obstack or on a gdbarch.  */

static struct type *
rust_composite_type (struct type *original,
		     const char *name,
		     const char *field1, struct type *type1,
		     const char *field2, struct type *type2)
{
  struct type *result = alloc_type_copy (original);
  int i, nfields, bitpos;

  nfields = 0;
  if (field1 != NULL)
    ++nfields;
  if (field2 != NULL)
    ++nfields;

  TYPE_CODE (result) = TYPE_CODE_STRUCT;
  TYPE_NAME (result) = name;
  TYPE_TAG_NAME (result) = name;

  TYPE_NFIELDS (result) = nfields;
  TYPE_FIELDS (result)
    = (struct field *) TYPE_ZALLOC (result, nfields * sizeof (struct field));

  i = 0;
  bitpos = 0;
  if (field1 != NULL)
    {
      struct field *field = &TYPE_FIELD (result, i);

      SET_FIELD_BITPOS (*field, bitpos);
      bitpos += TYPE_LENGTH (type1) * TARGET_CHAR_BIT;

      FIELD_NAME (*field) = field1;
      FIELD_TYPE (*field) = type1;
      ++i;
    }
  if (field2 != NULL)
    {
      struct field *field = &TYPE_FIELD (result, i);
      int align = rust_type_alignment (type2);

      if (align != 0)
	{
	  int delta;

	  align *= TARGET_CHAR_BIT;
	  delta = bitpos % align;
	  if (delta != 0)
	    bitpos += align - delta;
	}
      SET_FIELD_BITPOS (*field, bitpos);

      FIELD_NAME (*field) = field2;
      FIELD_TYPE (*field) = type2;
      ++i;
    }

  if (i > 0)
    TYPE_LENGTH (result)
      = (TYPE_FIELD_BITPOS (result, i - 1) / TARGET_CHAR_BIT +
	 TYPE_LENGTH (TYPE_FIELD_TYPE (result, i - 1)));
  return result;
}

/* See rust-lang.h.  */

struct type *
rust_slice_type (const char *name, struct type *elt_type,
		 struct type *usize_type)
{
  struct type *type;

  elt_type = lookup_pointer_type (elt_type);
  type = rust_composite_type (elt_type, name,
			      "data_ptr", elt_type,
			      "length", usize_type);

  return type;
}

enum rust_primitive_types
{
  rust_primitive_bool,
  rust_primitive_char,
  rust_primitive_i8,
  rust_primitive_u8,
  rust_primitive_i16,
  rust_primitive_u16,
  rust_primitive_i32,
  rust_primitive_u32,
  rust_primitive_i64,
  rust_primitive_u64,
  rust_primitive_isize,
  rust_primitive_usize,
  rust_primitive_f32,
  rust_primitive_f64,
  rust_primitive_unit,
  rust_primitive_str,
  nr_rust_primitive_types
};

/* la_language_arch_info implementation for Rust.  */

static void
rust_language_arch_info (struct gdbarch *gdbarch,
			 struct language_arch_info *lai)
{
  const struct builtin_type *builtin = builtin_type (gdbarch);
  struct type *tem;
  struct type **types;
  unsigned int length;

  types = GDBARCH_OBSTACK_CALLOC (gdbarch, nr_rust_primitive_types + 1,
				  struct type *);

  types[rust_primitive_bool] = arch_boolean_type (gdbarch, 8, 1, "bool");
  types[rust_primitive_char] = arch_character_type (gdbarch, 32, 1, "char");
  types[rust_primitive_i8] = arch_integer_type (gdbarch, 8, 0, "i8");
  types[rust_primitive_u8] = arch_integer_type (gdbarch, 8, 1, "u8");
  types[rust_primitive_i16] = arch_integer_type (gdbarch, 16, 0, "i16");
  types[rust_primitive_u16] = arch_integer_type (gdbarch, 16, 1, "u16");
  types[rust_primitive_i32] = arch_integer_type (gdbarch, 32, 0, "i32");
  types[rust_primitive_u32] = arch_integer_type (gdbarch, 32, 1, "u32");
  types[rust_primitive_i64] = arch_integer_type (gdbarch, 64, 0, "i64");
  types[rust_primitive_u64] = arch_integer_type (gdbarch, 64, 1, "u64");

  length = 8 * TYPE_LENGTH (builtin->builtin_data_ptr);
  types[rust_primitive_isize] = arch_integer_type (gdbarch, length, 0, "isize");
  types[rust_primitive_usize] = arch_integer_type (gdbarch, length, 1, "usize");

  types[rust_primitive_f32] = arch_float_type (gdbarch, 32, "f32", NULL);
  types[rust_primitive_f64] = arch_float_type (gdbarch, 64, "f64", NULL);

  types[rust_primitive_unit] = arch_integer_type (gdbarch, 0, 1, "()");

  tem = make_cv_type (1, 0, types[rust_primitive_u8], NULL);
  types[rust_primitive_str] = rust_slice_type ("&str", tem,
					       types[rust_primitive_usize]);

  lai->primitive_type_vector = types;
  lai->bool_type_default = types[rust_primitive_bool];
  lai->string_char_type = types[rust_primitive_u8];
}



/* A helper for rust_evaluate_subexp that handles OP_FUNCALL.  */

static struct value *
rust_evaluate_funcall (struct expression *exp, int *pos, enum noside noside)
{
  int i;
  int num_args = exp->elts[*pos + 1].longconst;
  const char *method;
  char *name;
  struct value *function, *result, *arg0;
  struct value **args;
  struct cleanup *cleanup;
  struct type *type, *fn_type;
  const struct block *block;
  struct block_symbol sym;

  /* For an ordinary function call we can simply defer to the
     generic implementation.  */
  if (exp->elts[*pos + 3].opcode != STRUCTOP_STRUCT)
    return evaluate_subexp_standard (NULL, exp, pos, noside);

  /* Skip over the OP_FUNCALL and the STRUCTOP_STRUCT.  */
  *pos += 4;
  method = &exp->elts[*pos + 1].string;
  *pos += 3 + BYTES_TO_EXP_ELEM (exp->elts[*pos].longconst + 1);

  /* Evaluate the argument to STRUCTOP_STRUCT, then find its
     type in order to look up the method.  */
  arg0 = evaluate_subexp (NULL_TYPE, exp, pos, noside);

  if (noside == EVAL_SKIP)
    {
      for (i = 0; i < num_args; ++i)
	evaluate_subexp (NULL_TYPE, exp, pos, noside);
      return arg0;
    }

  args = XNEWVEC (struct value *, num_args + 1);
  cleanup = make_cleanup (xfree, args);
  args[0] = arg0;

  /* We don't yet implement real Deref semantics.  */
  while (TYPE_CODE (value_type (args[0])) == TYPE_CODE_PTR)
    args[0] = value_ind (args[0]);

  type = value_type (args[0]);
  if ((TYPE_CODE (type) != TYPE_CODE_STRUCT
       && TYPE_CODE (type) != TYPE_CODE_UNION
       && TYPE_CODE (type) != TYPE_CODE_ENUM)
      || rust_tuple_type_p (type))
    error (_("Method calls only supported on struct or enum types"));
  if (TYPE_TAG_NAME (type) == NULL)
    error (_("Method call on nameless type"));

  name = concat (TYPE_TAG_NAME (type), "::", method, (char *) NULL);
  make_cleanup (xfree, name);

  block = get_selected_block (0);
  sym = lookup_symbol (name, block, VAR_DOMAIN, NULL);
  if (sym.symbol == NULL)
    error (_("Could not find function named '%s'"), name);

  fn_type = SYMBOL_TYPE (sym.symbol);
  if (TYPE_NFIELDS (fn_type) == 0)
    error (_("Function '%s' takes no arguments"), name);

  if (TYPE_CODE (TYPE_FIELD_TYPE (fn_type, 0)) == TYPE_CODE_PTR)
    args[0] = value_addr (args[0]);

  function = address_of_variable (sym.symbol, block);

  for (i = 0; i < num_args; ++i)
    args[i + 1] = evaluate_subexp (NULL_TYPE, exp, pos, noside);

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    result = value_zero (TYPE_TARGET_TYPE (fn_type), not_lval);
  else
    result = call_function_by_hand (function, num_args + 1, args);
  do_cleanups (cleanup);
  return result;
}

/* A helper for rust_evaluate_subexp that handles OP_RANGE.  */

static struct value *
rust_range (struct expression *exp, int *pos, enum noside noside)
{
  enum range_type kind;
  struct value *low = NULL, *high = NULL;
  struct value *addrval, *result;
  CORE_ADDR addr;
  struct type *range_type;
  struct type *index_type;
  struct type *temp_type;
  const char *name;

  kind = (enum range_type) longest_to_int (exp->elts[*pos + 1].longconst);
  *pos += 3;

  if (kind == HIGH_BOUND_DEFAULT || kind == NONE_BOUND_DEFAULT)
    low = evaluate_subexp (NULL_TYPE, exp, pos, noside);
  if (kind == LOW_BOUND_DEFAULT || kind == NONE_BOUND_DEFAULT)
    high = evaluate_subexp (NULL_TYPE, exp, pos, noside);

  if (noside == EVAL_SKIP)
    return value_from_longest (builtin_type (exp->gdbarch)->builtin_int, 1);

  if (low == NULL)
    {
      if (high == NULL)
	{
	  index_type = NULL;
	  name = "std::ops::RangeFull";
	}
      else
	{
	  index_type = value_type (high);
	  name = "std::ops::RangeTo";
	}
    }
  else
    {
      if (high == NULL)
	{
	  index_type = value_type (low);
	  name = "std::ops::RangeFrom";
	}
      else
	{
	  if (!types_equal (value_type (low), value_type (high)))
	    error (_("Range expression with different types"));
	  index_type = value_type (low);
	  name = "std::ops::Range";
	}
    }

  /* If we don't have an index type, just allocate this on the
     arch.  Here any type will do.  */
  temp_type = (index_type == NULL
	       ? language_bool_type (exp->language_defn, exp->gdbarch)
	       : index_type);
  /* It would be nicer to cache the range type.  */
  range_type = rust_composite_type (temp_type, name,
				    low == NULL ? NULL : "start", index_type,
				    high == NULL ? NULL : "end", index_type);

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value_zero (range_type, lval_memory);

  addrval = value_allocate_space_in_inferior (TYPE_LENGTH (range_type));
  addr = value_as_long (addrval);
  result = value_at_lazy (range_type, addr);

  if (low != NULL)
    {
      struct value *start = value_struct_elt (&result, NULL, "start", NULL,
					      "range");

      value_assign (start, low);
    }

  if (high != NULL)
    {
      struct value *end = value_struct_elt (&result, NULL, "end", NULL,
					    "range");

      value_assign (end, high);
    }

  result = value_at_lazy (range_type, addr);
  return result;
}

/* A helper function to compute the range and kind given a range
   value.  TYPE is the type of the range value.  RANGE is the range
   value.  LOW, HIGH, and KIND are out parameters.  The LOW and HIGH
   parameters might be filled in, or might not be, depending on the
   kind of range this is.  KIND will always be set to the appropriate
   value describing the kind of range, and this can be used to
   determine whether LOW or HIGH are valid.  */

static void
rust_compute_range (struct type *type, struct value *range,
		    LONGEST *low, LONGEST *high,
		    enum range_type *kind)
{
  int i;

  *low = 0;
  *high = 0;
  *kind = BOTH_BOUND_DEFAULT;

  if (TYPE_NFIELDS (type) == 0)
    return;

  i = 0;
  if (strcmp (TYPE_FIELD_NAME (type, 0), "start") == 0)
    {
      *kind = HIGH_BOUND_DEFAULT;
      *low = value_as_long (value_field (range, 0));
      ++i;
    }
  if (TYPE_NFIELDS (type) > i
      && strcmp (TYPE_FIELD_NAME (type, i), "end") == 0)
    {
      *kind = (*kind == BOTH_BOUND_DEFAULT
	       ? LOW_BOUND_DEFAULT : NONE_BOUND_DEFAULT);
      *high = value_as_long (value_field (range, i));
    }
}

/* A helper for rust_evaluate_subexp that handles BINOP_SUBSCRIPT.  */

static struct value *
rust_subscript (struct expression *exp, int *pos, enum noside noside,
		int for_addr)
{
  struct value *lhs, *rhs, *result;
  struct type *rhstype;
  LONGEST low, high_bound;
  /* Initialized to appease the compiler.  */
  enum range_type kind = BOTH_BOUND_DEFAULT;
  LONGEST high = 0;
  int want_slice = 0;

  ++*pos;
  lhs = evaluate_subexp (NULL_TYPE, exp, pos, noside);
  rhs = evaluate_subexp (NULL_TYPE, exp, pos, noside);

  if (noside == EVAL_SKIP)
    return lhs;

  rhstype = check_typedef (value_type (rhs));
  if (rust_range_type_p (rhstype))
    {
      if (!for_addr)
	error (_("Can't take slice of array without '&'"));
      rust_compute_range (rhstype, rhs, &low, &high, &kind);
      want_slice = 1;
    }
  else
    low = value_as_long (rhs);

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      struct type *type = check_typedef (value_type (lhs));

      result = value_zero (TYPE_TARGET_TYPE (type), VALUE_LVAL (lhs));
    }
  else
    {
      LONGEST low_bound;
      struct value *base;
      struct type *type = check_typedef (value_type (lhs));

      if (TYPE_CODE (type) == TYPE_CODE_ARRAY)
	{
	  base = lhs;
	  if (!get_array_bounds (type, &low_bound, &high_bound))
	    error (_("Can't compute array bounds"));
	  if (low_bound != 0)
	    error (_("Found array with non-zero lower bound"));
	  ++high_bound;
	}
      else if (rust_slice_type_p (type))
	{
	  struct value *len;

	  base = value_struct_elt (&lhs, NULL, "data_ptr", NULL, "slice");
	  len = value_struct_elt (&lhs, NULL, "length", NULL, "slice");
	  low_bound = 0;
	  high_bound = value_as_long (len);
	}
      else if (TYPE_CODE (type) == TYPE_CODE_PTR)
	{
	  base = lhs;
	  low_bound = 0;
	  high_bound = LONGEST_MAX;
	}
      else
	error (_("Cannot subscript non-array type"));

      if (want_slice
	  && (kind == BOTH_BOUND_DEFAULT || kind == LOW_BOUND_DEFAULT))
	low = low_bound;
      if (low < 0)
	error (_("Index less than zero"));
      if (low > high_bound)
	error (_("Index greater than length"));

      result = value_subscript (base, low);
    }

  if (for_addr)
    {
      if (want_slice)
	{
	  struct type *usize, *slice;
	  CORE_ADDR addr;
	  struct value *addrval, *tem;

	  if (kind == BOTH_BOUND_DEFAULT || kind == HIGH_BOUND_DEFAULT)
	    high = high_bound;
	  if (high < 0)
	    error (_("High index less than zero"));
	  if (low > high)
	    error (_("Low index greater than high index"));
	  if (high > high_bound)
	    error (_("High index greater than length"));

	  usize = language_lookup_primitive_type (exp->language_defn,
						  exp->gdbarch,
						  "usize");
	  slice = rust_slice_type ("&[*gdb*]", value_type (result),
				   usize);

	  addrval = value_allocate_space_in_inferior (TYPE_LENGTH (slice));
	  addr = value_as_long (addrval);
	  tem = value_at_lazy (slice, addr);

	  value_assign (value_field (tem, 0), value_addr (result));
	  value_assign (value_field (tem, 1),
			value_from_longest (usize, high - low));

	  result = value_at_lazy (slice, addr);
	}
      else
	result = value_addr (result);
    }

  return result;
}

/* evaluate_exp implementation for Rust.  */

static struct value *
rust_evaluate_subexp (struct type *expect_type, struct expression *exp,
		      int *pos, enum noside noside)
{
  struct value *result;

  switch (exp->elts[*pos].opcode)
    {
    case UNOP_COMPLEMENT:
      {
	struct value *value;

	++*pos;
	value = evaluate_subexp (NULL_TYPE, exp, pos, noside);
	if (noside == EVAL_SKIP)
	  {
	    /* Preserving the type is enough.  */
	    return value;
	  }
	if (TYPE_CODE (value_type (value)) == TYPE_CODE_BOOL)
	  result = value_from_longest (value_type (value),
				       value_logical_not (value));
	else
	  result = value_complement (value);
      }
      break;

    case BINOP_SUBSCRIPT:
      result = rust_subscript (exp, pos, noside, 0);
      break;

    case OP_FUNCALL:
      result = rust_evaluate_funcall (exp, pos, noside);
      break;

    case OP_AGGREGATE:
      {
	int pc = (*pos)++;
	struct type *type = exp->elts[pc + 1].type;
	int arglen = longest_to_int (exp->elts[pc + 2].longconst);
	int i;
	CORE_ADDR addr = 0;
	struct value *addrval = NULL;

	*pos += 3;

	if (noside == EVAL_NORMAL)
	  {
	    addrval = value_allocate_space_in_inferior (TYPE_LENGTH (type));
	    addr = value_as_long (addrval);
	    result = value_at_lazy (type, addr);
	  }

	if (arglen > 0 && exp->elts[*pos].opcode == OP_OTHERS)
	  {
	    struct value *init;

	    ++*pos;
	    init = rust_evaluate_subexp (NULL, exp, pos, noside);
	    if (noside == EVAL_NORMAL)
	      {
		/* This isn't quite right but will do for the time
		   being, seeing that we can't implement the Copy
		   trait anyway.  */
		value_assign (result, init);
	      }

	    --arglen;
	  }

	gdb_assert (arglen % 2 == 0);
	for (i = 0; i < arglen; i += 2)
	  {
	    int len;
	    const char *fieldname;
	    struct value *value, *field;

	    gdb_assert (exp->elts[*pos].opcode == OP_NAME);
	    ++*pos;
	    len = longest_to_int (exp->elts[*pos].longconst);
	    ++*pos;
	    fieldname = &exp->elts[*pos].string;
	    *pos += 2 + BYTES_TO_EXP_ELEM (len + 1);

	    value = rust_evaluate_subexp (NULL, exp, pos, noside);
	    if (noside == EVAL_NORMAL)
	      {
		field = value_struct_elt (&result, NULL, fieldname, NULL,
					  "structure");
		value_assign (field, value);
	      }
	  }

	if (noside == EVAL_SKIP)
	  return value_from_longest (builtin_type (exp->gdbarch)->builtin_int,
				     1);
	else if (noside == EVAL_AVOID_SIDE_EFFECTS)
	  result = allocate_value (type);
	else
	  result = value_at_lazy (type, addr);
      }
      break;

    case OP_RUST_ARRAY:
      {
	int pc = (*pos)++;
	int copies;
	struct value *elt;
	struct value *ncopies;

	elt = rust_evaluate_subexp (NULL, exp, pos, noside);
	ncopies = rust_evaluate_subexp (NULL, exp, pos, noside);
	copies = value_as_long (ncopies);
	if (copies < 0)
	  error (_("Array with negative number of elements"));

	if (noside == EVAL_NORMAL)
	  {
	    CORE_ADDR addr;
	    int i;
	    struct value **eltvec = XNEWVEC (struct value *, copies);
	    struct cleanup *cleanup = make_cleanup (xfree, eltvec);

	    for (i = 0; i < copies; ++i)
	      eltvec[i] = elt;
	    result = value_array (0, copies - 1, eltvec);

	    do_cleanups (cleanup);
	  }
	else
	  {
	    struct type *arraytype
	      = lookup_array_range_type (value_type (elt), 0, copies - 1);
	    result = allocate_value (arraytype);
	  }
      }
      break;

    case STRUCTOP_ANONYMOUS:
      {
        /* Anonymous field access, i.e. foo.1.  */
        struct value *lhs;
        int pc, field_number, nfields;
        struct type *type, *variant_type;
        struct disr_info disr;

        pc = (*pos)++;
        field_number = longest_to_int (exp->elts[pc + 1].longconst);
        (*pos) += 2;
        lhs = evaluate_subexp (NULL_TYPE, exp, pos, noside);

        type = value_type (lhs);
        if (TYPE_CODE (type) == TYPE_CODE_UNION)
	  {
	    struct cleanup *cleanup;

	    disr = rust_get_disr_info (type, value_contents (lhs),
				       value_embedded_offset (lhs),
				       value_address (lhs), lhs);

	    cleanup = make_cleanup (xfree, disr.name);

	    if (disr.is_encoded && disr.field_no == RUST_ENCODED_ENUM_HIDDEN)
	      {
		variant_type = NULL;
		nfields = 0;
	      }
	    else
	      {
		variant_type = TYPE_FIELD_TYPE (type, disr.field_no);
		nfields = TYPE_NFIELDS (variant_type);
	      }

	    if (!disr.is_encoded)
	      ++field_number;

	    if (field_number >= nfields || field_number < 0)
	      error(_("Cannot access field %d of variant %s, \
there are only %d fields"),
		    disr.is_encoded ? field_number : field_number - 1,
		    disr.name,
		    disr.is_encoded ? nfields : nfields - 1);

	    if (!(disr.is_encoded
		  ? rust_tuple_struct_type_p (variant_type)
		  : rust_tuple_variant_type_p (variant_type)))
	      error(_("Variant %s is not a tuple variant"), disr.name);

	    result = value_primitive_field (lhs, 0, field_number,
					    variant_type);
	    do_cleanups (cleanup);
	  }
	else if (TYPE_CODE (type) == TYPE_CODE_STRUCT)
	  {
	    /* Tuples and tuple structs */
	    nfields = TYPE_NFIELDS(type);

	    if (field_number >= nfields || field_number < 0)
	      error(_("Cannot access field %d of %s, there are only %d fields"),
		    field_number, TYPE_TAG_NAME (type), nfields);

	    /* Tuples are tuple structs too.  */
	    if (!rust_tuple_struct_type_p (type))
	      error(_("Attempting to access anonymous field %d of %s, which is \
not a tuple, tuple struct, or tuple-like variant"),
		    field_number, TYPE_TAG_NAME (type));

	    result = value_primitive_field (lhs, 0, field_number, type);
	  }
	else
	  error(_("Anonymous field access is only allowed on tuples, \
tuple structs, and tuple-like enum variants"));
      }
      break;

    case STRUCTOP_STRUCT:
      {
        struct value* lhs;
        struct type *type;
        int tem, pc;

        pc = (*pos)++;
        tem = longest_to_int (exp->elts[pc + 1].longconst);
        (*pos) += 3 + BYTES_TO_EXP_ELEM (tem + 1);
        lhs = evaluate_subexp (NULL_TYPE, exp, pos, noside);

        type = value_type (lhs);

        if (TYPE_CODE (type) == TYPE_CODE_UNION)
	  {
	    int i, start;
	    struct disr_info disr;
	    struct cleanup* cleanup;
	    struct type* variant_type;
	    char* field_name;

	    field_name = &exp->elts[pc + 2].string;

	    disr = rust_get_disr_info (type, value_contents (lhs),
				       value_embedded_offset (lhs),
				       value_address (lhs), lhs);

	    cleanup = make_cleanup (xfree, disr.name);

	    if (disr.is_encoded && disr.field_no == RUST_ENCODED_ENUM_HIDDEN)
	      error(_("Could not find field %s of struct variant %s"),
		    field_name, disr.name);

	    variant_type = TYPE_FIELD_TYPE (type, disr.field_no);

	    if (variant_type == NULL
		|| rust_tuple_variant_type_p (variant_type))
	      error(_("Attempting to access named field %s of tuple variant %s, \
which has only anonymous fields"),
		    field_name, disr.name);

	    start = disr.is_encoded ? 0 : 1;
	    for (i = start; i < TYPE_NFIELDS (variant_type); i++)
	      {
		if (strcmp (TYPE_FIELD_NAME (variant_type, i),
			    field_name) == 0) {
		  result = value_primitive_field (lhs, 0, i, variant_type);
		  break;
		}
	      }

	    if (i == TYPE_NFIELDS (variant_type))
	      /* We didn't find it.  */
	      error(_("Could not find field %s of struct variant %s"),
		    field_name, disr.name);

	    do_cleanups (cleanup);
	  }
	else
	  {
	    *pos = pc;
	    result = evaluate_subexp_standard (expect_type, exp, pos, noside);
	  }
      }
      break;

    case OP_RANGE:
      result = rust_range (exp, pos, noside);
      break;

    case UNOP_ADDR:
      /* We might have &array[range], in which case we need to make a
	 slice.  */
      if (exp->elts[*pos + 1].opcode == BINOP_SUBSCRIPT)
	{
	  ++*pos;
	  result = rust_subscript (exp, pos, noside, 1);
	  break;
	}
      /* Fall through.  */
    default:
      result = evaluate_subexp_standard (expect_type, exp, pos, noside);
      break;
    }

  return result;
}

/* operator_length implementation for Rust.  */

static void
rust_operator_length (const struct expression *exp, int pc, int *oplenp,
		      int *argsp)
{
  int oplen = 1;
  int args = 0;

  switch (exp->elts[pc - 1].opcode)
    {
    case OP_AGGREGATE:
      /* We handle aggregate as a type and argument count.  The first
	 argument might be OP_OTHERS.  After that the arguments
	 alternate: first an OP_NAME, then an expression.  */
      oplen = 4;
      args = longest_to_int (exp->elts[pc - 2].longconst);
      break;

    case OP_OTHERS:
      oplen = 1;
      args = 1;
      break;

    case STRUCTOP_ANONYMOUS:
      oplen = 3;
      args = 1;
      break;

    case OP_RUST_ARRAY:
      oplen = 1;
      args = 2;
      break;

    default:
      operator_length_standard (exp, pc, oplenp, argsp);
      return;
    }

  *oplenp = oplen;
  *argsp = args;
}

/* op_name implementation for Rust.  */

static char *
rust_op_name (enum exp_opcode opcode)
{
  switch (opcode)
    {
    case OP_AGGREGATE:
      return "OP_AGGREGATE";
    case OP_OTHERS:
      return "OP_OTHERS";
    default:
      return op_name_standard (opcode);
    }
}

/* dump_subexp_body implementation for Rust.  */

static int
rust_dump_subexp_body (struct expression *exp, struct ui_file *stream,
		       int elt)
{
  switch (exp->elts[elt].opcode)
    {
    case OP_AGGREGATE:
      {
	int length = longest_to_int (exp->elts[elt + 2].longconst);
	int i;

	fprintf_filtered (stream, "Type @");
	gdb_print_host_address (exp->elts[elt + 1].type, stream);
	fprintf_filtered (stream, " (");
	type_print (exp->elts[elt + 1].type, NULL, stream, 0);
	fprintf_filtered (stream, "), length %d", length);

	elt += 4;
	for (i = 0; i < length; ++i)
	  elt = dump_subexp (exp, stream, elt);
      }
      break;

    case OP_STRING:
    case OP_NAME:
      {
	LONGEST len = exp->elts[elt + 1].longconst;

	fprintf_filtered (stream, "%s: %s",
			  (exp->elts[elt].opcode == OP_STRING
			   ? "string" : "name"),
			  &exp->elts[elt + 2].string);
	elt += 4 + BYTES_TO_EXP_ELEM (len + 1);
      }
      break;

    case OP_OTHERS:
      elt = dump_subexp (exp, stream, elt + 1);
      break;

    case STRUCTOP_ANONYMOUS:
      {
	int field_number;

	field_number = longest_to_int (exp->elts[elt].longconst);

	fprintf_filtered (stream, "Field number: %d", field_number);
	elt = dump_subexp (exp, stream, elt + 2);
      }
      break;

    case OP_RUST_ARRAY:
      break;

    default:
      elt = dump_subexp_body_standard (exp, stream, elt);
      break;
    }

  return elt;
}

/* print_subexp implementation for Rust.  */

static void
rust_print_subexp (struct expression *exp, int *pos, struct ui_file *stream,
		   enum precedence prec)
{
  switch (exp->elts[*pos].opcode)
    {
    case OP_AGGREGATE:
      {
	int length = longest_to_int (exp->elts[*pos + 2].longconst);
	int i;

	type_print (exp->elts[*pos + 1].type, "", stream, 0);
	fputs_filtered (" { ", stream);

	*pos += 4;
	for (i = 0; i < length; ++i)
	  {
	    rust_print_subexp (exp, pos, stream, prec);
	    fputs_filtered (", ", stream);
	  }
	fputs_filtered (" }", stream);
      }
      break;

    case OP_NAME:
      {
	LONGEST len = exp->elts[*pos + 1].longconst;

	fputs_filtered (&exp->elts[*pos + 2].string, stream);
	*pos += 4 + BYTES_TO_EXP_ELEM (len + 1);
      }
      break;

    case OP_OTHERS:
      {
	fputs_filtered ("<<others>> (", stream);
	++*pos;
	rust_print_subexp (exp, pos, stream, prec);
	fputs_filtered (")", stream);
      }
      break;

    case STRUCTOP_ANONYMOUS:
      {
	int tem = longest_to_int (exp->elts[*pos + 1].longconst);

	(*pos) += 3;
	print_subexp (exp, pos, stream, PREC_SUFFIX);
	fprintf_filtered (stream, ".%d", tem);
      }
      return;

    case OP_RUST_ARRAY:
      ++*pos;
      fprintf_filtered (stream, "[");
      rust_print_subexp (exp, pos, stream, prec);
      fprintf_filtered (stream, "; ");
      rust_print_subexp (exp, pos, stream, prec);
      fprintf_filtered (stream, "]");
      break;

    default:
      print_subexp_standard (exp, pos, stream, prec);
      break;
    }
}

/* operator_check implementation for Rust.  */

static int
rust_operator_check (struct expression *exp, int pos,
		     int (*objfile_func) (struct objfile *objfile,
					  void *data),
		     void *data)
{
  switch (exp->elts[pos].opcode)
    {
    case OP_AGGREGATE:
      {
	struct type *type = exp->elts[pos + 1].type;
	struct objfile *objfile = TYPE_OBJFILE (type);

	if (objfile != NULL && (*objfile_func) (objfile, data))
	  return 1;
      }
      break;

    case OP_OTHERS:
    case OP_NAME:
    case OP_RUST_ARRAY:
      break;

    default:
      return operator_check_standard (exp, pos, objfile_func, data);
    }

  return 0;
}



/* Implementation of la_lookup_symbol_nonlocal for Rust.  */

static struct block_symbol
rust_lookup_symbol_nonlocal (const struct language_defn *langdef,
			     const char *name,
			     const struct block *block,
			     const domain_enum domain)
{
  struct block_symbol result = {NULL, NULL};

  if (symbol_lookup_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "rust_lookup_symbol_non_local"
			  " (%s, %s (scope %s), %s)\n",
			  name, host_address_to_string (block),
			  block_scope (block), domain_name (domain));
    }

  /* Look up bare names in the block's scope.  */
  if (name[cp_find_first_component (name)] == '\0')
    {
      const char *scope = block_scope (block);

      if (scope[0] != '\0')
	{
	  char *scopedname = concat (scope, "::", name, (char *) NULL);
	  struct cleanup *cleanup = make_cleanup (xfree, scopedname);

	  result = lookup_symbol_in_static_block (scopedname, block,
						  domain);
	  if (result.symbol == NULL)
	    result = lookup_global_symbol (scopedname, block, domain);
	  do_cleanups (cleanup);
	}
    }
  return result;
}



/* la_sniff_from_mangled_name for Rust.  */

static int
rust_sniff_from_mangled_name (const char *mangled, char **demangled)
{
  *demangled = gdb_demangle (mangled, DMGL_PARAMS | DMGL_ANSI);
  return *demangled != NULL;
}



static const struct exp_descriptor exp_descriptor_rust = 
{
  rust_print_subexp,
  rust_operator_length,
  rust_operator_check,
  rust_op_name,
  rust_dump_subexp_body,
  rust_evaluate_subexp
};

static const char *rust_extensions[] =
{
  ".rs", NULL
};

static const struct language_defn rust_language_defn =
{
  "rust",
  "Rust",
  language_rust,
  range_check_on,
  case_sensitive_on,
  array_row_major,
  macro_expansion_no,
  rust_extensions,
  &exp_descriptor_rust,
  rust_parse,
  rustyyerror,
  null_post_parser,
  rust_printchar,		/* Print a character constant */
  rust_printstr,		/* Function to print string constant */
  rust_emitchar,		/* Print a single char */
  rust_print_type,		/* Print a type using appropriate syntax */
  rust_print_typedef,		/* Print a typedef using appropriate syntax */
  rust_val_print,		/* Print a value using appropriate syntax */
  c_value_print,		/* Print a top-level value */
  default_read_var_value,	/* la_read_var_value */
  NULL,				/* Language specific skip_trampoline */
  NULL,				/* name_of_this */
  rust_lookup_symbol_nonlocal,	/* lookup_symbol_nonlocal */
  basic_lookup_transparent_type,/* lookup_transparent_type */
  gdb_demangle,			/* Language specific symbol demangler */
  rust_sniff_from_mangled_name,
  NULL,				/* Language specific
				   class_name_from_physname */
  c_op_print_tab,		/* expression operators for printing */
  1,				/* c-style arrays */
  0,				/* String lower bound */
  default_word_break_characters,
  default_make_symbol_completion_list,
  rust_language_arch_info,
  default_print_array_index,
  default_pass_by_reference,
  c_get_string,
  NULL,				/* la_get_symbol_name_cmp */
  iterate_over_symbols,
  &default_varobj_ops,
  NULL,
  NULL,
  LANG_MAGIC
};

void
_initialize_rust_language (void)
{
  add_language (&rust_language_defn);
}
