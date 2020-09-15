/* Rust language support routines for GDB, the GNU debugger.

   Copyright (C) 2016-2020 Free Software Foundation, Inc.

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
#include "psymtab.h"
#include "rust-lang.h"
#include "typeprint.h"
#include "valprint.h"
#include "varobj.h"
#include <algorithm>
#include <string>
#include <vector>
#include "cli/cli-style.h"

/* See rust-lang.h.  */

const char *
rust_last_path_segment (const char *path)
{
  const char *result = strrchr (path, ':');

  if (result == NULL)
    return path;
  return result + 1;
}

/* See rust-lang.h.  */

std::string
rust_crate_for_block (const struct block *block)
{
  const char *scope = block_scope (block);

  if (scope[0] == '\0')
    return std::string ();

  return std::string (scope, cp_find_first_component (scope));
}

/* Return true if TYPE, which must be a struct type, represents a Rust
   enum.  */

static bool
rust_enum_p (struct type *type)
{
  /* is_dynamic_type will return true if any field has a dynamic
     attribute -- but we only want to check the top level.  */
  return TYPE_HAS_VARIANT_PARTS (type);
}

/* Return true if TYPE, which must be an already-resolved enum type,
   has no variants.  */

static bool
rust_empty_enum_p (const struct type *type)
{
  return type->num_fields () == 0;
}

/* Given an already-resolved enum type and contents, find which
   variant is active.  */

static int
rust_enum_variant (struct type *type)
{
  /* The active variant is simply the first non-artificial field.  */
  for (int i = 0; i < type->num_fields (); ++i)
    if (!TYPE_FIELD_ARTIFICIAL (type, i))
      return i;

  /* Perhaps we could get here by trying to print an Ada variant
     record in Rust mode.  Unlikely, but an error is safer than an
     assert.  */
  error (_("Could not find active enum variant"));
}

/* See rust-lang.h.  */

bool
rust_tuple_type_p (struct type *type)
{
  /* The current implementation is a bit of a hack, but there's
     nothing else in the debuginfo to distinguish a tuple from a
     struct.  */
  return (type->code () == TYPE_CODE_STRUCT
	  && type->name () != NULL
	  && type->name ()[0] == '(');
}

/* Return true if all non-static fields of a structlike type are in a
   sequence like __0, __1, __2.  */

static bool
rust_underscore_fields (struct type *type)
{
  int i, field_number;

  field_number = 0;

  if (type->code () != TYPE_CODE_STRUCT)
    return false;
  for (i = 0; i < type->num_fields (); ++i)
    {
      if (!field_is_static (&type->field (i)))
	{
	  char buf[20];

	  xsnprintf (buf, sizeof (buf), "__%d", field_number);
	  if (strcmp (buf, TYPE_FIELD_NAME (type, i)) != 0)
	    return false;
	  field_number++;
	}
    }
  return true;
}

/* See rust-lang.h.  */

bool
rust_tuple_struct_type_p (struct type *type)
{
  /* This is just an approximation until DWARF can represent Rust more
     precisely.  We exclude zero-length structs because they may not
     be tuple structs, and there's no way to tell.  */
  return type->num_fields () > 0 && rust_underscore_fields (type);
}

/* Return true if TYPE is a slice type, otherwise false.  */

static bool
rust_slice_type_p (struct type *type)
{
  return (type->code () == TYPE_CODE_STRUCT
	  && type->name () != NULL
	  && (strncmp (type->name (), "&[", 2) == 0
	      || strcmp (type->name (), "&str") == 0));
}

/* Return true if TYPE is a range type, otherwise false.  */

static bool
rust_range_type_p (struct type *type)
{
  int i;

  if (type->code () != TYPE_CODE_STRUCT
      || type->num_fields () > 2
      || type->name () == NULL
      || strstr (type->name (), "::Range") == NULL)
    return false;

  if (type->num_fields () == 0)
    return true;

  i = 0;
  if (strcmp (TYPE_FIELD_NAME (type, 0), "start") == 0)
    {
      if (type->num_fields () == 1)
	return true;
      i = 1;
    }
  else if (type->num_fields () == 2)
    {
      /* First field had to be "start".  */
      return false;
    }

  return strcmp (TYPE_FIELD_NAME (type, i), "end") == 0;
}

/* Return true if TYPE is an inclusive range type, otherwise false.
   This is only valid for types which are already known to be range
   types.  */

static bool
rust_inclusive_range_type_p (struct type *type)
{
  return (strstr (type->name (), "::RangeInclusive") != NULL
	  || strstr (type->name (), "::RangeToInclusive") != NULL);
}

/* Return true if TYPE seems to be the type "u8", otherwise false.  */

static bool
rust_u8_type_p (struct type *type)
{
  return (type->code () == TYPE_CODE_INT
	  && TYPE_UNSIGNED (type)
	  && TYPE_LENGTH (type) == 1);
}

/* Return true if TYPE is a Rust character type.  */

static bool
rust_chartype_p (struct type *type)
{
  return (type->code () == TYPE_CODE_CHAR
	  && TYPE_LENGTH (type) == 4
	  && TYPE_UNSIGNED (type));
}

/* If VALUE represents a trait object pointer, return the underlying
   pointer with the correct (i.e., runtime) type.  Otherwise, return
   NULL.  */

static struct value *
rust_get_trait_object_pointer (struct value *value)
{
  struct type *type = check_typedef (value_type (value));

  if (type->code () != TYPE_CODE_STRUCT || type->num_fields () != 2)
    return NULL;

  /* Try to be a bit resilient if the ABI changes.  */
  int vtable_field = 0;
  for (int i = 0; i < 2; ++i)
    {
      if (strcmp (TYPE_FIELD_NAME (type, i), "vtable") == 0)
	vtable_field = i;
      else if (strcmp (TYPE_FIELD_NAME (type, i), "pointer") != 0)
	return NULL;
    }

  CORE_ADDR vtable = value_as_address (value_field (value, vtable_field));
  struct symbol *symbol = find_symbol_at_address (vtable);
  if (symbol == NULL || symbol->subclass != SYMBOL_RUST_VTABLE)
    return NULL;

  struct rust_vtable_symbol *vtable_sym
    = static_cast<struct rust_vtable_symbol *> (symbol);
  struct type *pointer_type = lookup_pointer_type (vtable_sym->concrete_type);
  return value_cast (pointer_type, value_field (value, 1 - vtable_field));
}



/* language_defn::printstr implementation for Rust.  */

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



static void rust_value_print_inner (struct value *val, struct ui_file *stream,
				    int recurse,
				    const struct value_print_options *options);

/* Helper function to print a string slice.  */

static void
rust_val_print_str (struct ui_file *stream, struct value *val,
		    const struct value_print_options *options)
{
  struct value *base = value_struct_elt (&val, NULL, "data_ptr", NULL,
					 "slice");
  struct value *len = value_struct_elt (&val, NULL, "length", NULL, "slice");

  val_print_string (TYPE_TARGET_TYPE (value_type (base)), "UTF-8",
		    value_as_address (base), value_as_long (len), stream,
		    options);
}

/* rust_val_print helper for structs and untagged unions.  */

static void
val_print_struct (struct value *val, struct ui_file *stream, int recurse,
		  const struct value_print_options *options)
{
  int i;
  int first_field;
  struct type *type = check_typedef (value_type (val));

  if (rust_slice_type_p (type) && strcmp (type->name (), "&str") == 0)
    {
      /* If what we are printing here is actually a string within a
	 structure then VAL will be the original parent value, while TYPE
	 will be the type of the structure representing the string we want
	 to print.
	 However, RUST_VAL_PRINT_STR looks up the fields of the string
	 inside VAL, assuming that VAL is the string.
	 So, recreate VAL as a value representing just the string.  */
      val = value_at_lazy (type, value_address (val));
      rust_val_print_str (stream, val, options);
      return;
    }

  bool is_tuple = rust_tuple_type_p (type);
  bool is_tuple_struct = !is_tuple && rust_tuple_struct_type_p (type);
  struct value_print_options opts;

  if (!is_tuple)
    {
      if (type->name () != NULL)
        fprintf_filtered (stream, "%s", type->name ());

      if (type->num_fields () == 0)
        return;

      if (type->name () != NULL)
        fputs_filtered (" ", stream);
    }

  if (is_tuple || is_tuple_struct)
    fputs_filtered ("(", stream);
  else
    fputs_filtered ("{", stream);

  opts = *options;
  opts.deref_ref = 0;

  first_field = 1;
  for (i = 0; i < type->num_fields (); ++i)
    {
      if (field_is_static (&type->field (i)))
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
	  fputs_styled (TYPE_FIELD_NAME (type, i),
			variable_name_style.style (), stream);
	  fputs_filtered (": ", stream);
        }

      rust_value_print_inner (value_field (val, i), stream, recurse + 1,
			      &opts);
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

/* rust_val_print helper for discriminated unions (Rust enums).  */

static void
rust_print_enum (struct value *val, struct ui_file *stream, int recurse,
		 const struct value_print_options *options)
{
  struct value_print_options opts = *options;
  struct type *type = check_typedef (value_type (val));

  opts.deref_ref = 0;

  gdb_assert (rust_enum_p (type));
  gdb::array_view<const gdb_byte> view (value_contents_for_printing (val),
					TYPE_LENGTH (value_type (val)));
  type = resolve_dynamic_type (type, view, value_address (val));

  if (rust_empty_enum_p (type))
    {
      /* Print the enum type name here to be more clear.  */
      fprintf_filtered (stream, _("%s {%p[<No data fields>%p]}"),
			type->name (),
			metadata_style.style ().ptr (), nullptr);
      return;
    }

  int variant_fieldno = rust_enum_variant (type);
  val = value_field (val, variant_fieldno);
  struct type *variant_type = type->field (variant_fieldno).type ();

  int nfields = variant_type->num_fields ();

  bool is_tuple = rust_tuple_struct_type_p (variant_type);

  fprintf_filtered (stream, "%s", variant_type->name ());
  if (nfields == 0)
    {
      /* In case of a nullary variant like 'None', just output
	 the name. */
      return;
    }

  /* In case of a non-nullary variant, we output 'Foo(x,y,z)'. */
  if (is_tuple)
    fprintf_filtered (stream, "(");
  else
    {
      /* struct variant.  */
      fprintf_filtered (stream, "{");
    }

  bool first_field = true;
  for (int j = 0; j < variant_type->num_fields (); j++)
    {
      if (!first_field)
	fputs_filtered (", ", stream);
      first_field = false;

      if (!is_tuple)
	fprintf_filtered (stream, "%ps: ",
			  styled_string (variable_name_style.style (),
					 TYPE_FIELD_NAME (variant_type, j)));

      rust_value_print_inner (value_field (val, j), stream, recurse + 1,
			      &opts);
    }

  if (is_tuple)
    fputs_filtered (")", stream);
  else
    fputs_filtered ("}", stream);
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

/* la_value_print_inner implementation for Rust.  */
static void
rust_value_print_inner (struct value *val, struct ui_file *stream,
			int recurse,
			const struct value_print_options *options)
{
  struct value_print_options opts = *options;
  opts.deref_ref = 1;

  if (opts.prettyformat == Val_prettyformat_default)
    opts.prettyformat = (opts.prettyformat_structs
			 ? Val_prettyformat : Val_no_prettyformat);

  struct type *type = check_typedef (value_type (val));
  switch (type->code ())
    {
    case TYPE_CODE_PTR:
      {
	LONGEST low_bound, high_bound;
	
	if (TYPE_TARGET_TYPE (type)->code () == TYPE_CODE_ARRAY
	    && rust_u8_type_p (TYPE_TARGET_TYPE (TYPE_TARGET_TYPE (type)))
	    && get_array_bounds (TYPE_TARGET_TYPE (type), &low_bound,
				 &high_bound))
	  {
	    /* We have a pointer to a byte string, so just print
	       that.  */
	    struct type *elttype = check_typedef (TYPE_TARGET_TYPE (type));
	    CORE_ADDR addr = value_as_address (val);
	    struct gdbarch *arch = get_type_arch (type);

	    if (opts.addressprint)
	      {
		fputs_filtered (paddress (arch, addr), stream);
		fputs_filtered (" ", stream);
	      }

	    fputs_filtered ("b", stream);
	    val_print_string (TYPE_TARGET_TYPE (elttype), "ASCII", addr,
			      high_bound - low_bound + 1, stream,
			      &opts);
	    break;
	  }
      }
      goto generic_print;

    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_MEMBERPTR:
      c_value_print_inner (val, stream, recurse, &opts);
      break;

    case TYPE_CODE_INT:
      /* Recognize the unit type.  */
      if (TYPE_UNSIGNED (type) && TYPE_LENGTH (type) == 0
	  && type->name () != NULL && strcmp (type->name (), "()") == 0)
	{
	  fputs_filtered ("()", stream);
	  break;
	}
      goto generic_print;

    case TYPE_CODE_STRING:
      {
	LONGEST low_bound, high_bound;

	if (!get_array_bounds (type, &low_bound, &high_bound))
	  error (_("Could not determine the array bounds"));

	/* If we see a plain TYPE_CODE_STRING, then we're printing a
	   byte string, hence the choice of "ASCII" as the
	   encoding.  */
	fputs_filtered ("b", stream);
	rust_printstr (stream, TYPE_TARGET_TYPE (type),
		       value_contents_for_printing (val),
		       high_bound - low_bound + 1, "ASCII", 0, &opts);
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
      /* Untagged unions are printed as if they are structs.  Since
	 the field bit positions overlap in the debuginfo, the code
	 for printing a union is same as that for a struct, the only
	 difference is that the input type will have overlapping
	 fields.  */
      val_print_struct (val, stream, recurse, &opts);
      break;

    case TYPE_CODE_STRUCT:
      if (rust_enum_p (type))
	rust_print_enum (val, stream, recurse, &opts);
      else
	val_print_struct (val, stream, recurse, &opts);
      break;

    default:
    generic_print:
      /* Nothing special yet.  */
      generic_value_print (val, stream, recurse, &opts, &rust_decorations);
    }
}



static void
rust_internal_print_type (struct type *type, const char *varstring,
			  struct ui_file *stream, int show, int level,
			  const struct type_print_options *flags,
			  bool for_rust_enum, print_offset_data *podata);

/* Print a struct or union typedef.  */
static void
rust_print_struct_def (struct type *type, const char *varstring,
		       struct ui_file *stream, int show, int level,
		       const struct type_print_options *flags,
		       bool for_rust_enum, print_offset_data *podata)
{
  /* Print a tuple type simply.  */
  if (rust_tuple_type_p (type))
    {
      fputs_filtered (type->name (), stream);
      return;
    }

  /* If we see a base class, delegate to C.  */
  if (TYPE_N_BASECLASSES (type) > 0)
    c_print_type (type, varstring, stream, show, level, flags);

  if (flags->print_offsets)
    {
      /* Temporarily bump the level so that the output lines up
	 correctly.  */
      level += 2;
    }

  /* Compute properties of TYPE here because, in the enum case, the
     rest of the code ends up looking only at the variant part.  */
  const char *tagname = type->name ();
  bool is_tuple_struct = rust_tuple_struct_type_p (type);
  bool is_tuple = rust_tuple_type_p (type);
  bool is_enum = rust_enum_p (type);

  if (for_rust_enum)
    {
      /* Already printing an outer enum, so nothing to print here.  */
    }
  else
    {
      /* This code path is also used by unions and enums.  */
      if (is_enum)
	{
	  fputs_filtered ("enum ", stream);
	  dynamic_prop *prop = type->dyn_prop (DYN_PROP_VARIANT_PARTS);
	  if (prop != nullptr && prop->kind () == PROP_TYPE)
	    type = prop->original_type ();
	}
      else if (type->code () == TYPE_CODE_STRUCT)
	fputs_filtered ("struct ", stream);
      else
	fputs_filtered ("union ", stream);

      if (tagname != NULL)
	fputs_filtered (tagname, stream);
    }

  if (type->num_fields () == 0 && !is_tuple)
    return;
  if (for_rust_enum && !flags->print_offsets)
    fputs_filtered (is_tuple_struct ? "(" : "{", stream);
  else
    fputs_filtered (is_tuple_struct ? " (\n" : " {\n", stream);

  /* When printing offsets, we rearrange the fields into storage
     order.  This lets us show holes more clearly.  We work using
     field indices here because it simplifies calls to
     print_offset_data::update below.  */
  std::vector<int> fields;
  for (int i = 0; i < type->num_fields (); ++i)
    {
      if (field_is_static (&type->field (i)))
	continue;
      if (is_enum && TYPE_FIELD_ARTIFICIAL (type, i))
	continue;
      fields.push_back (i);
    }
  if (flags->print_offsets)
    std::sort (fields.begin (), fields.end (),
	       [&] (int a, int b)
	       {
		 return (TYPE_FIELD_BITPOS (type, a)
			 < TYPE_FIELD_BITPOS (type, b));
	       });

  for (int i : fields)
    {
      QUIT;

      gdb_assert (!field_is_static (&type->field (i)));
      gdb_assert (! (is_enum && TYPE_FIELD_ARTIFICIAL (type, i)));

      if (flags->print_offsets)
	podata->update (type, i, stream);

      /* We'd like to print "pub" here as needed, but rustc
	 doesn't emit the debuginfo, and our types don't have
	 cplus_struct_type attached.  */

      /* For a tuple struct we print the type but nothing
	 else.  */
      if (!for_rust_enum || flags->print_offsets)
	print_spaces_filtered (level + 2, stream);
      if (is_enum)
	fputs_styled (TYPE_FIELD_NAME (type, i), variable_name_style.style (),
		      stream);
      else if (!is_tuple_struct)
	fprintf_filtered (stream, "%ps: ",
			  styled_string (variable_name_style.style (),
					 TYPE_FIELD_NAME (type, i)));

      rust_internal_print_type (type->field (i).type (), NULL,
				stream, (is_enum ? show : show - 1),
				level + 2, flags, is_enum, podata);
      if (!for_rust_enum || flags->print_offsets)
	fputs_filtered (",\n", stream);
      /* Note that this check of "I" is ok because we only sorted the
	 fields by offset when print_offsets was set, so we won't take
	 this branch in that case.  */
      else if (i + 1 < type->num_fields ())
	fputs_filtered (", ", stream);
    }

  if (flags->print_offsets)
    {
      /* Undo the temporary level increase we did above.  */
      level -= 2;
      podata->finish (type, level, stream);
      print_spaces_filtered (print_offset_data::indentation, stream);
      if (level == 0)
	print_spaces_filtered (2, stream);
    }
  if (!for_rust_enum || flags->print_offsets)
    print_spaces_filtered (level, stream);
  fputs_filtered (is_tuple_struct ? ")" : "}", stream);
}

/* la_print_type implementation for Rust.  */

static void
rust_internal_print_type (struct type *type, const char *varstring,
			  struct ui_file *stream, int show, int level,
			  const struct type_print_options *flags,
			  bool for_rust_enum, print_offset_data *podata)
{
  QUIT;
  if (show <= 0
      && type->name () != NULL)
    {
      /* Rust calls the unit type "void" in its debuginfo,
         but we don't want to print it as that.  */
      if (type->code () == TYPE_CODE_VOID)
        fputs_filtered ("()", stream);
      else
        fputs_filtered (type->name (), stream);
      return;
    }

  type = check_typedef (type);
  switch (type->code ())
    {
    case TYPE_CODE_VOID:
      /* If we have an enum, we've already printed the type's
	 unqualified name, and there is nothing else to print
	 here.  */
      if (!for_rust_enum)
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
      for (int i = 0; i < type->num_fields (); ++i)
	{
	  QUIT;
	  if (i > 0)
	    fputs_filtered (", ", stream);
	  rust_internal_print_type (type->field (i).type (), "", stream,
				    -1, 0, flags, false, podata);
	}
      fputs_filtered (")", stream);
      /* If it returns unit, we can omit the return type.  */
      if (TYPE_TARGET_TYPE (type)->code () != TYPE_CODE_VOID)
        {
          fputs_filtered (" -> ", stream);
          rust_internal_print_type (TYPE_TARGET_TYPE (type), "", stream,
				    -1, 0, flags, false, podata);
        }
      break;

    case TYPE_CODE_ARRAY:
      {
	LONGEST low_bound, high_bound;

	fputs_filtered ("[", stream);
	rust_internal_print_type (TYPE_TARGET_TYPE (type), NULL,
				  stream, show - 1, level, flags, false,
				  podata);

	if (type->bounds ()->high.kind () == PROP_LOCEXPR
	    || type->bounds ()->high.kind () == PROP_LOCLIST)
	  fprintf_filtered (stream, "; variable length");
	else if (get_array_bounds (type, &low_bound, &high_bound))
	  fprintf_filtered (stream, "; %s",
			    plongest (high_bound - low_bound + 1));
	fputs_filtered ("]", stream);
      }
      break;

    case TYPE_CODE_UNION:
    case TYPE_CODE_STRUCT:
      rust_print_struct_def (type, varstring, stream, show, level, flags,
			     for_rust_enum, podata);
      break;

    case TYPE_CODE_ENUM:
      {
	int len = 0;

	fputs_filtered ("enum ", stream);
	if (type->name () != NULL)
	  {
	    fputs_filtered (type->name (), stream);
	    fputs_filtered (" ", stream);
	    len = strlen (type->name ());
	  }
	fputs_filtered ("{\n", stream);

	for (int i = 0; i < type->num_fields (); ++i)
	  {
	    const char *name = TYPE_FIELD_NAME (type, i);

	    QUIT;

	    if (len > 0
		&& strncmp (name, type->name (), len) == 0
		&& name[len] == ':'
		&& name[len + 1] == ':')
	      name += len + 2;
	    fprintfi_filtered (level + 2, stream, "%ps,\n",
			       styled_string (variable_name_style.style (),
					      name));
	  }

	fputs_filtered ("}", stream);
      }
      break;

    case TYPE_CODE_PTR:
      {
	if (type->name () != nullptr)
	  fputs_filtered (type->name (), stream);
	else
	  {
	    /* We currently can't distinguish between pointers and
	       references.  */
	    fputs_filtered ("*mut ", stream);
	    type_print (TYPE_TARGET_TYPE (type), "", stream, 0);
	  }
      }
      break;

    default:
    c_printer:
      c_print_type (type, varstring, stream, show, level, flags);
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

  result->set_code (TYPE_CODE_STRUCT);
  result->set_name (name);

  result->set_num_fields (nfields);
  result->set_fields
    ((struct field *) TYPE_ZALLOC (result, nfields * sizeof (struct field)));

  i = 0;
  bitpos = 0;
  if (field1 != NULL)
    {
      struct field *field = &result->field (i);

      SET_FIELD_BITPOS (*field, bitpos);
      bitpos += TYPE_LENGTH (type1) * TARGET_CHAR_BIT;

      FIELD_NAME (*field) = field1;
      field->set_type (type1);
      ++i;
    }
  if (field2 != NULL)
    {
      struct field *field = &result->field (i);
      unsigned align = type_align (type2);

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
      field->set_type (type2);
      ++i;
    }

  if (i > 0)
    TYPE_LENGTH (result)
      = (TYPE_FIELD_BITPOS (result, i - 1) / TARGET_CHAR_BIT +
	 TYPE_LENGTH (result->field (i - 1).type ()));
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



/* A helper for rust_evaluate_subexp that handles OP_FUNCALL.  */

static struct value *
rust_evaluate_funcall (struct expression *exp, int *pos, enum noside noside)
{
  int i;
  int num_args = exp->elts[*pos + 1].longconst;
  const char *method;
  struct value *function, *result, *arg0;
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
  arg0 = evaluate_subexp (nullptr, exp, pos, noside);

  if (noside == EVAL_SKIP)
    {
      for (i = 0; i < num_args; ++i)
	evaluate_subexp (nullptr, exp, pos, noside);
      return arg0;
    }

  std::vector<struct value *> args (num_args + 1);
  args[0] = arg0;

  /* We don't yet implement real Deref semantics.  */
  while (value_type (args[0])->code () == TYPE_CODE_PTR)
    args[0] = value_ind (args[0]);

  type = value_type (args[0]);
  if ((type->code () != TYPE_CODE_STRUCT
       && type->code () != TYPE_CODE_UNION
       && type->code () != TYPE_CODE_ENUM)
      || rust_tuple_type_p (type))
    error (_("Method calls only supported on struct or enum types"));
  if (type->name () == NULL)
    error (_("Method call on nameless type"));

  std::string name = std::string (type->name ()) + "::" + method;

  block = get_selected_block (0);
  sym = lookup_symbol (name.c_str (), block, VAR_DOMAIN, NULL);
  if (sym.symbol == NULL)
    error (_("Could not find function named '%s'"), name.c_str ());

  fn_type = SYMBOL_TYPE (sym.symbol);
  if (fn_type->num_fields () == 0)
    error (_("Function '%s' takes no arguments"), name.c_str ());

  if (fn_type->field (0).type ()->code () == TYPE_CODE_PTR)
    args[0] = value_addr (args[0]);

  function = address_of_variable (sym.symbol, block);

  for (i = 0; i < num_args; ++i)
    args[i + 1] = evaluate_subexp (nullptr, exp, pos, noside);

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    result = value_zero (TYPE_TARGET_TYPE (fn_type), not_lval);
  else
    result = call_function_by_hand (function, NULL, args);
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

  if (kind == HIGH_BOUND_DEFAULT || kind == NONE_BOUND_DEFAULT
      || kind == NONE_BOUND_DEFAULT_EXCLUSIVE)
    low = evaluate_subexp (nullptr, exp, pos, noside);
  if (kind == LOW_BOUND_DEFAULT || kind == LOW_BOUND_DEFAULT_EXCLUSIVE
      || kind == NONE_BOUND_DEFAULT || kind == NONE_BOUND_DEFAULT_EXCLUSIVE)
    high = evaluate_subexp (nullptr, exp, pos, noside);
  bool inclusive = (kind == NONE_BOUND_DEFAULT || kind == LOW_BOUND_DEFAULT);

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
	  name = (inclusive
		  ? "std::ops::RangeToInclusive" : "std::ops::RangeTo");
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
	  name = inclusive ? "std::ops::RangeInclusive" : "std::ops::Range";
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

  if (type->num_fields () == 0)
    return;

  i = 0;
  if (strcmp (TYPE_FIELD_NAME (type, 0), "start") == 0)
    {
      *kind = HIGH_BOUND_DEFAULT;
      *low = value_as_long (value_field (range, 0));
      ++i;
    }
  if (type->num_fields () > i
      && strcmp (TYPE_FIELD_NAME (type, i), "end") == 0)
    {
      *kind = (*kind == BOTH_BOUND_DEFAULT
	       ? LOW_BOUND_DEFAULT : NONE_BOUND_DEFAULT);
      *high = value_as_long (value_field (range, i));

      if (rust_inclusive_range_type_p (type))
	++*high;
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
  lhs = evaluate_subexp (nullptr, exp, pos, noside);
  rhs = evaluate_subexp (nullptr, exp, pos, noside);

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

  struct type *type = check_typedef (value_type (lhs));
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      struct type *base_type = nullptr;
      if (type->code () == TYPE_CODE_ARRAY)
	base_type = TYPE_TARGET_TYPE (type);
      else if (rust_slice_type_p (type))
	{
	  for (int i = 0; i < type->num_fields (); ++i)
	    {
	      if (strcmp (TYPE_FIELD_NAME (type, i), "data_ptr") == 0)
		{
		  base_type = TYPE_TARGET_TYPE (type->field (i).type ());
		  break;
		}
	    }
	  if (base_type == nullptr)
	    error (_("Could not find 'data_ptr' in slice type"));
	}
      else if (type->code () == TYPE_CODE_PTR)
	base_type = TYPE_TARGET_TYPE (type);
      else
	error (_("Cannot subscript non-array type"));

      struct type *new_type;
      if (want_slice)
	{
	  if (rust_slice_type_p (type))
	    new_type = type;
	  else
	    {
	      struct type *usize
		= language_lookup_primitive_type (exp->language_defn,
						  exp->gdbarch,
						  "usize");
	      new_type = rust_slice_type ("&[*gdb*]", base_type, usize);
	    }
	}
      else
	new_type = base_type;

      return value_zero (new_type, VALUE_LVAL (lhs));
    }
  else
    {
      LONGEST low_bound;
      struct value *base;

      if (type->code () == TYPE_CODE_ARRAY)
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
      else if (type->code () == TYPE_CODE_PTR)
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
	  const char *new_name = ((type != nullptr
				   && rust_slice_type_p (type))
				  ? type->name () : "&[*gdb*]");

	  slice = rust_slice_type (new_name, value_type (result), usize);

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
    case UNOP_IND:
      {
	if (noside != EVAL_NORMAL)
	  result = evaluate_subexp_standard (expect_type, exp, pos, noside);
	else
	  {
	    ++*pos;
	    struct value *value = evaluate_subexp (expect_type, exp, pos,
						   noside);

	    struct value *trait_ptr = rust_get_trait_object_pointer (value);
	    if (trait_ptr != NULL)
	      value = trait_ptr;

	    result = value_ind (value);
	  }
      }
      break;

    case UNOP_COMPLEMENT:
      {
	struct value *value;

	++*pos;
	value = evaluate_subexp (nullptr, exp, pos, noside);
	if (noside == EVAL_SKIP)
	  {
	    /* Preserving the type is enough.  */
	    return value;
	  }
	if (value_type (value)->code () == TYPE_CODE_BOOL)
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
	(*pos)++;
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
	    int i;
	    std::vector<struct value *> eltvec (copies);

	    for (i = 0; i < copies; ++i)
	      eltvec[i] = elt;
	    result = value_array (0, copies - 1, eltvec.data ());
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
        struct type *type;

        pc = (*pos)++;
        field_number = longest_to_int (exp->elts[pc + 1].longconst);
        (*pos) += 2;
	lhs = evaluate_subexp (nullptr, exp, pos, noside);

	type = value_type (lhs);

	if (type->code () == TYPE_CODE_STRUCT)
	  {
	    struct type *outer_type = NULL;

	    if (rust_enum_p (type))
	      {
		gdb::array_view<const gdb_byte> view (value_contents (lhs),
						      TYPE_LENGTH (type));
		type = resolve_dynamic_type (type, view, value_address (lhs));

		if (rust_empty_enum_p (type))
		  error (_("Cannot access field %d of empty enum %s"),
			 field_number, type->name ());

		int fieldno = rust_enum_variant (type);
		lhs = value_primitive_field (lhs, 0, fieldno, type);
		outer_type = type;
		type = value_type (lhs);
	      }

	    /* Tuples and tuple structs */
	    nfields = type->num_fields ();

	    if (field_number >= nfields || field_number < 0)
	      {
		if (outer_type != NULL)
		  error(_("Cannot access field %d of variant %s::%s, "
			  "there are only %d fields"),
			field_number, outer_type->name (),
			rust_last_path_segment (type->name ()),
			nfields);
		else
		  error(_("Cannot access field %d of %s, "
			  "there are only %d fields"),
			field_number, type->name (), nfields);
	      }

	    /* Tuples are tuple structs too.  */
	    if (!rust_tuple_struct_type_p (type))
	      {
		if (outer_type != NULL)
		  error(_("Variant %s::%s is not a tuple variant"),
			outer_type->name (),
			rust_last_path_segment (type->name ()));
		else
		  error(_("Attempting to access anonymous field %d "
			  "of %s, which is not a tuple, tuple struct, or "
			  "tuple-like variant"),
		      field_number, type->name ());
	      }

	    result = value_primitive_field (lhs, 0, field_number, type);
	  }
	else
	  error(_("Anonymous field access is only allowed on tuples, \
tuple structs, and tuple-like enum variants"));
      }
      break;

    case STRUCTOP_STRUCT:
      {
        struct value *lhs;
        struct type *type;
        int tem, pc;

        pc = (*pos)++;
        tem = longest_to_int (exp->elts[pc + 1].longconst);
        (*pos) += 3 + BYTES_TO_EXP_ELEM (tem + 1);
	lhs = evaluate_subexp (nullptr, exp, pos, noside);

	const char *field_name = &exp->elts[pc + 2].string;
        type = value_type (lhs);
        if (type->code () == TYPE_CODE_STRUCT && rust_enum_p (type))
	  {
	    gdb::array_view<const gdb_byte> view (value_contents (lhs),
						  TYPE_LENGTH (type));
	    type = resolve_dynamic_type (type, view, value_address (lhs));

	    if (rust_empty_enum_p (type))
	      error (_("Cannot access field %s of empty enum %s"),
		     field_name, type->name ());

	    int fieldno = rust_enum_variant (type);
	    lhs = value_primitive_field (lhs, 0, fieldno, type);

	    struct type *outer_type = type;
	    type = value_type (lhs);
	    if (rust_tuple_type_p (type) || rust_tuple_struct_type_p (type))
		error (_("Attempting to access named field %s of tuple "
			 "variant %s::%s, which has only anonymous fields"),
		       field_name, outer_type->name (),
		       rust_last_path_segment (type->name ()));

	    try
	      {
		result = value_struct_elt (&lhs, NULL, field_name,
					   NULL, "structure");
	      }
	    catch (const gdb_exception_error &except)
	      {
		error (_("Could not find field %s of struct variant %s::%s"),
		       field_name, outer_type->name (),
		       rust_last_path_segment (type->name ()));
	      }
	  }
	else
	  result = value_struct_elt (&lhs, NULL, field_name, NULL, "structure");
	if (noside == EVAL_AVOID_SIDE_EFFECTS)
	  result = value_zero (value_type (result), VALUE_LVAL (result));
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

static const char *
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

	field_number = longest_to_int (exp->elts[elt + 1].longconst);

	fprintf_filtered (stream, "Field number: %d", field_number);
	elt = dump_subexp (exp, stream, elt + 3);
      }
      break;

    case OP_RUST_ARRAY:
      ++elt;
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
      break;

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

/* Constant data representing the Rust language.  */

extern const struct language_data rust_language_data =
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
  NULL,				/* name_of_this */
  false,			/* la_store_sym_names_in_linkage_form_p */
  c_op_print_tab,		/* expression operators for printing */
  1,				/* c-style arrays */
  0,				/* String lower bound */
  &default_varobj_ops,
  "{...}"			/* la_struct_too_deep_ellipsis */
};

/* Class representing the Rust language.  */

class rust_language : public language_defn
{
public:
  rust_language ()
    : language_defn (language_rust, rust_language_data)
  { /* Nothing.  */ }

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    const struct builtin_type *builtin = builtin_type (gdbarch);

    struct type **types
      = GDBARCH_OBSTACK_CALLOC (gdbarch, nr_rust_primitive_types + 1,
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

    unsigned int length = 8 * TYPE_LENGTH (builtin->builtin_data_ptr);
    types[rust_primitive_isize] = arch_integer_type (gdbarch, length, 0, "isize");
    types[rust_primitive_usize] = arch_integer_type (gdbarch, length, 1, "usize");

    types[rust_primitive_f32] = arch_float_type (gdbarch, 32, "f32",
						 floatformats_ieee_single);
    types[rust_primitive_f64] = arch_float_type (gdbarch, 64, "f64",
						 floatformats_ieee_double);

    types[rust_primitive_unit] = arch_integer_type (gdbarch, 0, 1, "()");

    struct type *tem = make_cv_type (1, 0, types[rust_primitive_u8], NULL);
    types[rust_primitive_str] = rust_slice_type ("&str", tem,
						 types[rust_primitive_usize]);

    lai->primitive_type_vector = types;
    lai->bool_type_default = types[rust_primitive_bool];
    lai->string_char_type = types[rust_primitive_u8];
  }

  /* See language.h.  */
  bool sniff_from_mangled_name (const char *mangled,
				char **demangled) const override
  {
    *demangled = gdb_demangle (mangled, DMGL_PARAMS | DMGL_ANSI);
    return *demangled != NULL;
  }

  /* See language.h.  */

  char *demangle (const char *mangled, int options) const override
  {
    return gdb_demangle (mangled, options);
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override
  {
    print_offset_data podata;
    rust_internal_print_type (type, varstring, stream, show, level,
			      flags, false, &podata);
  }

  /* See language.h.  */

  gdb::unique_xmalloc_ptr<char> watch_location_expression
	(struct type *type, CORE_ADDR addr) const override
  {
    type = check_typedef (TYPE_TARGET_TYPE (check_typedef (type)));
    std::string name = type_to_string (type);
    return gdb::unique_xmalloc_ptr<char>
      (xstrprintf ("*(%s as *mut %s)", core_addr_to_string (addr),
		   name.c_str ()));
  }

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override
  {
    return rust_value_print_inner (val, stream, recurse, options);
  }

  /* See language.h.  */

  struct block_symbol lookup_symbol_nonlocal
	(const char *name, const struct block *block,
	 const domain_enum domain) const override
  {
    struct block_symbol result = {};

    if (symbol_lookup_debug)
      {
	fprintf_unfiltered (gdb_stdlog,
			    "rust_lookup_symbol_non_local"
			    " (%s, %s (scope %s), %s)\n",
			    name, host_address_to_string (block),
			    block_scope (block), domain_name (domain));
      }

    /* Look up bare names in the block's scope.  */
    std::string scopedname;
    if (name[cp_find_first_component (name)] == '\0')
      {
	const char *scope = block_scope (block);

	if (scope[0] != '\0')
	  {
	    scopedname = std::string (scope) + "::" + name;
	    name = scopedname.c_str ();
	  }
	else
	  name = NULL;
      }

    if (name != NULL)
      {
	result = lookup_symbol_in_static_block (name, block, domain);
	if (result.symbol == NULL)
	  result = lookup_global_symbol (name, block, domain);
      }
    return result;
  }

  /* See language.h.  */

  int parser (struct parser_state *ps) const override
  {
    return rust_parse (ps);
  }

  /* See language.h.  */

  void emitchar (int ch, struct type *chtype,
		 struct ui_file *stream, int quoter) const override
  {
    if (!rust_chartype_p (chtype))
      generic_emit_char (ch, chtype, stream, quoter,
			 target_charset (get_type_arch (chtype)));
    else if (ch == '\\' || ch == quoter)
      fprintf_filtered (stream, "\\%c", ch);
    else if (ch == '\n')
      fputs_filtered ("\\n", stream);
    else if (ch == '\r')
      fputs_filtered ("\\r", stream);
    else if (ch == '\t')
      fputs_filtered ("\\t", stream);
    else if (ch == '\0')
      fputs_filtered ("\\0", stream);
    else if (ch >= 32 && ch <= 127 && isprint (ch))
      fputc_filtered (ch, stream);
    else if (ch <= 255)
      fprintf_filtered (stream, "\\x%02x", ch);
    else
      fprintf_filtered (stream, "\\u{%06x}", ch);
  }

  /* See language.h.  */

  void printchar (int ch, struct type *chtype,
		  struct ui_file *stream) const override
  {
    fputs_filtered ("'", stream);
    LA_EMIT_CHAR (ch, chtype, stream, '\'');
    fputs_filtered ("'", stream);
  }

  /* See language.h.  */

  void printstr (struct ui_file *stream, struct type *elttype,
		 const gdb_byte *string, unsigned int length,
		 const char *encoding, int force_ellipses,
		 const struct value_print_options *options) const override
  {
    rust_printstr (stream, elttype, string, length, encoding,
		   force_ellipses, options);
  }

  /* See language.h.  */

  void print_typedef (struct type *type, struct symbol *new_symbol,
		      struct ui_file *stream) const override
  {
    type = check_typedef (type);
    fprintf_filtered (stream, "type %s = ", new_symbol->print_name ());
    type_print (type, "", stream, 0);
    fprintf_filtered (stream, ";");
  }

  /* See language.h.  */

  bool is_string_type_p (struct type *type) const override
  {
    LONGEST low_bound, high_bound;

    type = check_typedef (type);
    return ((type->code () == TYPE_CODE_STRING)
	    || (type->code () == TYPE_CODE_PTR
		&& (TYPE_TARGET_TYPE (type)->code () == TYPE_CODE_ARRAY
		    && rust_u8_type_p (TYPE_TARGET_TYPE (TYPE_TARGET_TYPE (type)))
		    && get_array_bounds (TYPE_TARGET_TYPE (type), &low_bound,
					 &high_bound)))
	    || (type->code () == TYPE_CODE_STRUCT
		&& !rust_enum_p (type)
		&& rust_slice_type_p (type)
		&& strcmp (type->name (), "&str") == 0));
  }
};

/* Single instance of the Rust language class.  */

static rust_language rust_language_defn;
