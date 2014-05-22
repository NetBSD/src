/* Support for printing Ada types for GDB, the GNU debugger.
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
#include "gdb_obstack.h"
#include "bfd.h"		/* Binary File Description */
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "gdbcore.h"
#include "target.h"
#include "command.h"
#include "gdbcmd.h"
#include "language.h"
#include "demangle.h"
#include "c-lang.h"
#include "typeprint.h"
#include "ada-lang.h"

#include <ctype.h>
#include "gdb_string.h"
#include <errno.h>

static int print_selected_record_field_types (struct type *, struct type *,
					      int, int,
					      struct ui_file *, int, int,
					      const struct type_print_options *);
   
static int print_record_field_types (struct type *, struct type *,
				     struct ui_file *, int, int,
				     const struct type_print_options *);

static void print_array_type (struct type *, struct ui_file *, int, int,
			      const struct type_print_options *);

static int print_choices (struct type *, int, struct ui_file *,
			  struct type *);

static void print_range (struct type *, struct ui_file *);

static void print_range_bound (struct type *, char *, int *,
			       struct ui_file *);

static void
print_dynamic_range_bound (struct type *, const char *, int,
			   const char *, struct ui_file *);

static void print_range_type (struct type *, struct ui_file *);



static char *name_buffer;
static int name_buffer_len;

/* The (decoded) Ada name of TYPE.  This value persists until the
   next call.  */

static char *
decoded_type_name (struct type *type)
{
  if (ada_type_name (type) == NULL)
    return NULL;
  else
    {
      const char *raw_name = ada_type_name (type);
      char *s, *q;

      if (name_buffer == NULL || name_buffer_len <= strlen (raw_name))
	{
	  name_buffer_len = 16 + 2 * strlen (raw_name);
	  name_buffer = xrealloc (name_buffer, name_buffer_len);
	}
      strcpy (name_buffer, raw_name);

      s = (char *) strstr (name_buffer, "___");
      if (s != NULL)
	*s = '\0';

      s = name_buffer + strlen (name_buffer) - 1;
      while (s > name_buffer && (s[0] != '_' || s[-1] != '_'))
	s -= 1;

      if (s == name_buffer)
	return name_buffer;

      if (!islower (s[1]))
	return NULL;

      for (s = q = name_buffer; *s != '\0'; q += 1)
	{
	  if (s[0] == '_' && s[1] == '_')
	    {
	      *q = '.';
	      s += 2;
	    }
	  else
	    {
	      *q = *s;
	      s += 1;
	    }
	}
      *q = '\0';
      return name_buffer;
    }
}

/* Print TYPE on STREAM, preferably as a range.  */

static void
print_range (struct type *type, struct ui_file *stream)
{
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_RANGE:
    case TYPE_CODE_ENUM:
      {
	struct type *target_type;
	target_type = TYPE_TARGET_TYPE (type);
	if (target_type == NULL)
	  target_type = type;
	ada_print_scalar (target_type, ada_discrete_type_low_bound (type),
			  stream);
	fprintf_filtered (stream, " .. ");
	ada_print_scalar (target_type, ada_discrete_type_high_bound (type),
			  stream);
      }
      break;
    default:
      fprintf_filtered (stream, "%.*s",
			ada_name_prefix_len (TYPE_NAME (type)),
			TYPE_NAME (type));
      break;
    }
}

/* Print the number or discriminant bound at BOUNDS+*N on STREAM, and
   set *N past the bound and its delimiter, if any.  */

static void
print_range_bound (struct type *type, char *bounds, int *n,
		   struct ui_file *stream)
{
  LONGEST B;

  if (ada_scan_number (bounds, *n, &B, n))
    {
      /* STABS decodes all range types which bounds are 0 .. -1 as
         unsigned integers (ie. the type code is TYPE_CODE_INT, not
         TYPE_CODE_RANGE).  Unfortunately, ada_print_scalar() relies
         on the unsigned flag to determine whether the bound should
         be printed as a signed or an unsigned value.  This causes
         the upper bound of the 0 .. -1 range types to be printed as
         a very large unsigned number instead of -1.
         To workaround this stabs deficiency, we replace the TYPE by NULL
         to indicate default output when we detect that the bound is negative,
         and the type is a TYPE_CODE_INT.  The bound is negative when
         'm' is the last character of the number scanned in BOUNDS.  */
      if (bounds[*n - 1] == 'm' && TYPE_CODE (type) == TYPE_CODE_INT)
	type = NULL;
      ada_print_scalar (type, B, stream);
      if (bounds[*n] == '_')
	*n += 2;
    }
  else
    {
      int bound_len;
      char *bound = bounds + *n;
      char *pend;

      pend = strstr (bound, "__");
      if (pend == NULL)
	*n += bound_len = strlen (bound);
      else
	{
	  bound_len = pend - bound;
	  *n += bound_len + 2;
	}
      fprintf_filtered (stream, "%.*s", bound_len, bound);
    }
}

/* Assuming NAME[0 .. NAME_LEN-1] is the name of a range type, print
   the value (if found) of the bound indicated by SUFFIX ("___L" or
   "___U") according to the ___XD conventions.  */

static void
print_dynamic_range_bound (struct type *type, const char *name, int name_len,
			   const char *suffix, struct ui_file *stream)
{
  static char *name_buf = NULL;
  static size_t name_buf_len = 0;
  LONGEST B;
  int OK;

  GROW_VECT (name_buf, name_buf_len, name_len + strlen (suffix) + 1);
  strncpy (name_buf, name, name_len);
  strcpy (name_buf + name_len, suffix);

  B = get_int_var_value (name_buf, &OK);
  if (OK)
    ada_print_scalar (type, B, stream);
  else
    fprintf_filtered (stream, "?");
}

/* Print RAW_TYPE as a range type, using any bound information
   following the GNAT encoding (if available).  */

static void
print_range_type (struct type *raw_type, struct ui_file *stream)
{
  const char *name;
  struct type *base_type;
  const char *subtype_info;

  gdb_assert (raw_type != NULL);
  name = TYPE_NAME (raw_type);
  gdb_assert (name != NULL);

  if (TYPE_CODE (raw_type) == TYPE_CODE_RANGE)
    base_type = TYPE_TARGET_TYPE (raw_type);
  else
    base_type = raw_type;

  subtype_info = strstr (name, "___XD");
  if (subtype_info == NULL)
    print_range (raw_type, stream);
  else
    {
      int prefix_len = subtype_info - name;
      char *bounds_str;
      int n;

      subtype_info += 5;
      bounds_str = strchr (subtype_info, '_');
      n = 1;

      if (*subtype_info == 'L')
	{
	  print_range_bound (base_type, bounds_str, &n, stream);
	  subtype_info += 1;
	}
      else
	print_dynamic_range_bound (base_type, name, prefix_len, "___L",
				   stream);

      fprintf_filtered (stream, " .. ");

      if (*subtype_info == 'U')
	print_range_bound (base_type, bounds_str, &n, stream);
      else
	print_dynamic_range_bound (base_type, name, prefix_len, "___U",
				   stream);
    }
}

/* Print enumerated type TYPE on STREAM.  */

static void
print_enum_type (struct type *type, struct ui_file *stream)
{
  int len = TYPE_NFIELDS (type);
  int i;
  LONGEST lastval;

  fprintf_filtered (stream, "(");
  wrap_here (" ");

  lastval = 0;
  for (i = 0; i < len; i++)
    {
      QUIT;
      if (i)
	fprintf_filtered (stream, ", ");
      wrap_here ("    ");
      fputs_filtered (ada_enum_name (TYPE_FIELD_NAME (type, i)), stream);
      if (lastval != TYPE_FIELD_ENUMVAL (type, i))
	{
	  fprintf_filtered (stream, " => %s",
			    plongest (TYPE_FIELD_ENUMVAL (type, i)));
	  lastval = TYPE_FIELD_ENUMVAL (type, i);
	}
      lastval += 1;
    }
  fprintf_filtered (stream, ")");
}

/* Print representation of Ada fixed-point type TYPE on STREAM.  */

static void
print_fixed_point_type (struct type *type, struct ui_file *stream)
{
  DOUBLEST delta = ada_delta (type);
  DOUBLEST small = ada_fixed_to_float (type, 1.0);

  if (delta < 0.0)
    fprintf_filtered (stream, "delta ??");
  else
    {
      fprintf_filtered (stream, "delta %g", (double) delta);
      if (delta != small)
	fprintf_filtered (stream, " <'small = %g>", (double) small);
    }
}

/* Print simple (constrained) array type TYPE on STREAM.  LEVEL is the
   recursion (indentation) level, in case the element type itself has
   nested structure, and SHOW is the number of levels of internal
   structure to show (see ada_print_type).  */

static void
print_array_type (struct type *type, struct ui_file *stream, int show,
		  int level, const struct type_print_options *flags)
{
  int bitsize;
  int n_indices;

  if (ada_is_constrained_packed_array_type (type))
    type = ada_coerce_to_simple_array_type (type);

  bitsize = 0;
  fprintf_filtered (stream, "array (");

  if (type == NULL)
    {
      fprintf_filtered (stream, _("<undecipherable array type>"));
      return;
    }

  n_indices = -1;
  if (ada_is_simple_array_type (type))
    {
      struct type *range_desc_type;
      struct type *arr_type;

      range_desc_type = ada_find_parallel_type (type, "___XA");
      ada_fixup_array_indexes_type (range_desc_type);

      bitsize = 0;
      if (range_desc_type == NULL)
	{
	  for (arr_type = type; TYPE_CODE (arr_type) == TYPE_CODE_ARRAY;
	       arr_type = TYPE_TARGET_TYPE (arr_type))
	    {
	      if (arr_type != type)
		fprintf_filtered (stream, ", ");
	      print_range (TYPE_INDEX_TYPE (arr_type), stream);
	      if (TYPE_FIELD_BITSIZE (arr_type, 0) > 0)
		bitsize = TYPE_FIELD_BITSIZE (arr_type, 0);
	    }
	}
      else
	{
	  int k;

	  n_indices = TYPE_NFIELDS (range_desc_type);
	  for (k = 0, arr_type = type;
	       k < n_indices;
	       k += 1, arr_type = TYPE_TARGET_TYPE (arr_type))
	    {
	      if (k > 0)
		fprintf_filtered (stream, ", ");
	      print_range_type (TYPE_FIELD_TYPE (range_desc_type, k),
				stream);
	      if (TYPE_FIELD_BITSIZE (arr_type, 0) > 0)
		bitsize = TYPE_FIELD_BITSIZE (arr_type, 0);
	    }
	}
    }
  else
    {
      int i, i0;

      for (i = i0 = ada_array_arity (type); i > 0; i -= 1)
	fprintf_filtered (stream, "%s<>", i == i0 ? "" : ", ");
    }

  fprintf_filtered (stream, ") of ");
  wrap_here ("");
  ada_print_type (ada_array_element_type (type, n_indices), "", stream,
		  show == 0 ? 0 : show - 1, level + 1, flags);
  if (bitsize > 0)
    fprintf_filtered (stream, " <packed: %d-bit elements>", bitsize);
}

/* Print the choices encoded by field FIELD_NUM of variant-part TYPE on
   STREAM, assuming that VAL_TYPE (if non-NULL) is the type of the
   values.  Return non-zero if the field is an encoding of
   discriminant values, as in a standard variant record, and 0 if the
   field is not so encoded (as happens with single-component variants
   in types annotated with pragma Unchecked_Variant).  */

static int
print_choices (struct type *type, int field_num, struct ui_file *stream,
	       struct type *val_type)
{
  int have_output;
  int p;
  const char *name = TYPE_FIELD_NAME (type, field_num);

  have_output = 0;

  /* Skip over leading 'V': NOTE soon to be obsolete.  */
  if (name[0] == 'V')
    {
      if (!ada_scan_number (name, 1, NULL, &p))
	goto Huh;
    }
  else
    p = 0;

  while (1)
    {
      switch (name[p])
	{
	default:
	  goto Huh;
	case '_':
	case '\0':
	  fprintf_filtered (stream, " =>");
	  return 1;
	case 'S':
	case 'R':
	case 'O':
	  if (have_output)
	    fprintf_filtered (stream, " | ");
	  have_output = 1;
	  break;
	}

      switch (name[p])
	{
	case 'S':
	  {
	    LONGEST W;

	    if (!ada_scan_number (name, p + 1, &W, &p))
	      goto Huh;
	    ada_print_scalar (val_type, W, stream);
	    break;
	  }
	case 'R':
	  {
	    LONGEST L, U;

	    if (!ada_scan_number (name, p + 1, &L, &p)
		|| name[p] != 'T' || !ada_scan_number (name, p + 1, &U, &p))
	      goto Huh;
	    ada_print_scalar (val_type, L, stream);
	    fprintf_filtered (stream, " .. ");
	    ada_print_scalar (val_type, U, stream);
	    break;
	  }
	case 'O':
	  fprintf_filtered (stream, "others");
	  p += 1;
	  break;
	}
    }

Huh:
  fprintf_filtered (stream, "?? =>");
  return 0;
}

/* Assuming that field FIELD_NUM of TYPE represents variants whose
   discriminant is contained in OUTER_TYPE, print its components on STREAM.
   LEVEL is the recursion (indentation) level, in case any of the fields
   themselves have nested structure, and SHOW is the number of levels of 
   internal structure to show (see ada_print_type).  For this purpose,
   fields nested in a variant part are taken to be at the same level as
   the fields immediately outside the variant part.  */

static void
print_variant_clauses (struct type *type, int field_num,
		       struct type *outer_type, struct ui_file *stream,
		       int show, int level,
		       const struct type_print_options *flags)
{
  int i;
  struct type *var_type, *par_type;
  struct type *discr_type;

  var_type = TYPE_FIELD_TYPE (type, field_num);
  discr_type = ada_variant_discrim_type (var_type, outer_type);

  if (TYPE_CODE (var_type) == TYPE_CODE_PTR)
    {
      var_type = TYPE_TARGET_TYPE (var_type);
      if (var_type == NULL || TYPE_CODE (var_type) != TYPE_CODE_UNION)
	return;
    }

  par_type = ada_find_parallel_type (var_type, "___XVU");
  if (par_type != NULL)
    var_type = par_type;

  for (i = 0; i < TYPE_NFIELDS (var_type); i += 1)
    {
      fprintf_filtered (stream, "\n%*swhen ", level + 4, "");
      if (print_choices (var_type, i, stream, discr_type))
	{
	  if (print_record_field_types (TYPE_FIELD_TYPE (var_type, i),
					outer_type, stream, show, level + 4,
					flags)
	      <= 0)
	    fprintf_filtered (stream, " null;");
	}
      else
	print_selected_record_field_types (var_type, outer_type, i, i,
					   stream, show, level + 4, flags);
    }
}

/* Assuming that field FIELD_NUM of TYPE is a variant part whose
   discriminants are contained in OUTER_TYPE, print a description of it
   on STREAM.  LEVEL is the recursion (indentation) level, in case any of
   the fields themselves have nested structure, and SHOW is the number of
   levels of internal structure to show (see ada_print_type).  For this
   purpose, fields nested in a variant part are taken to be at the same
   level as the fields immediately outside the variant part.  */

static void
print_variant_part (struct type *type, int field_num, struct type *outer_type,
		    struct ui_file *stream, int show, int level,
		    const struct type_print_options *flags)
{
  fprintf_filtered (stream, "\n%*scase %s is", level + 4, "",
		    ada_variant_discrim_name
		    (TYPE_FIELD_TYPE (type, field_num)));
  print_variant_clauses (type, field_num, outer_type, stream, show,
			 level + 4, flags);
  fprintf_filtered (stream, "\n%*send case;", level + 4, "");
}

/* Print a description on STREAM of the fields FLD0 through FLD1 in
   record or union type TYPE, whose discriminants are in OUTER_TYPE.
   LEVEL is the recursion (indentation) level, in case any of the
   fields themselves have nested structure, and SHOW is the number of
   levels of internal structure to show (see ada_print_type).  Does
   not print parent type information of TYPE.  Returns 0 if no fields
   printed, -1 for an incomplete type, else > 0.  Prints each field
   beginning on a new line, but does not put a new line at end.  */

static int
print_selected_record_field_types (struct type *type, struct type *outer_type,
				   int fld0, int fld1,
				   struct ui_file *stream, int show, int level,
				   const struct type_print_options *flags)
{
  int i, flds;

  flds = 0;

  if (fld0 > fld1 && TYPE_STUB (type))
    return -1;

  for (i = fld0; i <= fld1; i += 1)
    {
      QUIT;

      if (ada_is_parent_field (type, i) || ada_is_ignored_field (type, i))
	;
      else if (ada_is_wrapper_field (type, i))
	flds += print_record_field_types (TYPE_FIELD_TYPE (type, i), type,
					  stream, show, level, flags);
      else if (ada_is_variant_part (type, i))
	{
	  print_variant_part (type, i, outer_type, stream, show, level, flags);
	  flds = 1;
	}
      else
	{
	  flds += 1;
	  fprintf_filtered (stream, "\n%*s", level + 4, "");
	  ada_print_type (TYPE_FIELD_TYPE (type, i),
			  TYPE_FIELD_NAME (type, i),
			  stream, show - 1, level + 4, flags);
	  fprintf_filtered (stream, ";");
	}
    }

  return flds;
}

/* Print a description on STREAM of all fields of record or union type
   TYPE, as for print_selected_record_field_types, above.  */

static int
print_record_field_types (struct type *type, struct type *outer_type,
			  struct ui_file *stream, int show, int level,
			  const struct type_print_options *flags)
{
  return print_selected_record_field_types (type, outer_type,
					    0, TYPE_NFIELDS (type) - 1,
					    stream, show, level, flags);
}
   

/* Print record type TYPE on STREAM.  LEVEL is the recursion (indentation)
   level, in case the element type itself has nested structure, and SHOW is
   the number of levels of internal structure to show (see ada_print_type).  */

static void
print_record_type (struct type *type0, struct ui_file *stream, int show,
		   int level, const struct type_print_options *flags)
{
  struct type *parent_type;
  struct type *type;

  type = ada_find_parallel_type (type0, "___XVE");
  if (type == NULL)
    type = type0;

  parent_type = ada_parent_type (type);
  if (ada_type_name (parent_type) != NULL)
    {
      const char *parent_name = decoded_type_name (parent_type);

      /* If we fail to decode the parent type name, then use the parent
	 type name as is.  Not pretty, but should never happen except
	 when the debugging info is incomplete or incorrect.  This
	 prevents a crash trying to print a NULL pointer.  */
      if (parent_name == NULL)
	parent_name = ada_type_name (parent_type);
      fprintf_filtered (stream, "new %s with record", parent_name);
    }
  else if (parent_type == NULL && ada_is_tagged_type (type, 0))
    fprintf_filtered (stream, "tagged record");
  else
    fprintf_filtered (stream, "record");

  if (show < 0)
    fprintf_filtered (stream, " ... end record");
  else
    {
      int flds;

      flds = 0;
      if (parent_type != NULL && ada_type_name (parent_type) == NULL)
	flds += print_record_field_types (parent_type, parent_type,
					  stream, show, level, flags);
      flds += print_record_field_types (type, type, stream, show, level,
					flags);

      if (flds > 0)
	fprintf_filtered (stream, "\n%*send record", level, "");
      else if (flds < 0)
	fprintf_filtered (stream, _(" <incomplete type> end record"));
      else
	fprintf_filtered (stream, " null; end record");
    }
}

/* Print the unchecked union type TYPE in something resembling Ada
   format on STREAM.  LEVEL is the recursion (indentation) level
   in case the element type itself has nested structure, and SHOW is the
   number of levels of internal structure to show (see ada_print_type).  */
static void
print_unchecked_union_type (struct type *type, struct ui_file *stream,
			    int show, int level,
			    const struct type_print_options *flags)
{
  if (show < 0)
    fprintf_filtered (stream, "record (?) is ... end record");
  else if (TYPE_NFIELDS (type) == 0)
    fprintf_filtered (stream, "record (?) is null; end record");
  else
    {
      int i;

      fprintf_filtered (stream, "record (?) is\n%*scase ? is", level + 4, "");

      for (i = 0; i < TYPE_NFIELDS (type); i += 1)
	{
	  fprintf_filtered (stream, "\n%*swhen ? =>\n%*s", level + 8, "",
			    level + 12, "");
	  ada_print_type (TYPE_FIELD_TYPE (type, i),
			  TYPE_FIELD_NAME (type, i),
			  stream, show - 1, level + 12, flags);
	  fprintf_filtered (stream, ";");
	}

      fprintf_filtered (stream, "\n%*send case;\n%*send record",
			level + 4, "", level, "");
    }
}



/* Print function or procedure type TYPE on STREAM.  Make it a header
   for function or procedure NAME if NAME is not null.  */

static void
print_func_type (struct type *type, struct ui_file *stream, const char *name,
		 const struct type_print_options *flags)
{
  int i, len = TYPE_NFIELDS (type);

  if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_VOID)
    fprintf_filtered (stream, "procedure");
  else
    fprintf_filtered (stream, "function");

  if (name != NULL && name[0] != '\0')
    fprintf_filtered (stream, " %s", name);

  if (len > 0)
    {
      fprintf_filtered (stream, " (");
      for (i = 0; i < len; i += 1)
	{
	  if (i > 0)
	    {
	      fputs_filtered ("; ", stream);
	      wrap_here ("    ");
	    }
	  fprintf_filtered (stream, "a%d: ", i + 1);
	  ada_print_type (TYPE_FIELD_TYPE (type, i), "", stream, -1, 0,
			  flags);
	}
      fprintf_filtered (stream, ")");
    }

  if (TYPE_CODE (TYPE_TARGET_TYPE (type)) != TYPE_CODE_VOID)
    {
      fprintf_filtered (stream, " return ");
      ada_print_type (TYPE_TARGET_TYPE (type), "", stream, 0, 0, flags);
    }
}


/* Print a description of a type TYPE0.
   Output goes to STREAM (via stdio).
   If VARSTRING is a non-empty string, print as an Ada variable/field
       declaration.
   SHOW+1 is the maximum number of levels of internal type structure
      to show (this applies to record types, enumerated types, and
      array types).
   SHOW is the number of levels of internal type structure to show
      when there is a type name for the SHOWth deepest level (0th is
      outer level).
   When SHOW<0, no inner structure is shown.
   LEVEL indicates level of recursion (for nested definitions).  */

void
ada_print_type (struct type *type0, const char *varstring,
		struct ui_file *stream, int show, int level,
		const struct type_print_options *flags)
{
  struct type *type = ada_check_typedef (ada_get_base_type (type0));
  char *type_name = decoded_type_name (type0);
  int is_var_decl = (varstring != NULL && varstring[0] != '\0');

  if (type == NULL)
    {
      if (is_var_decl)
	fprintf_filtered (stream, "%.*s: ",
			  ada_name_prefix_len (varstring), varstring);
      fprintf_filtered (stream, "<null type?>");
      return;
    }

  if (show > 0)
    type = ada_check_typedef (type);

  if (is_var_decl && TYPE_CODE (type) != TYPE_CODE_FUNC)
    fprintf_filtered (stream, "%.*s: ",
		      ada_name_prefix_len (varstring), varstring);

  if (type_name != NULL && show <= 0 && !ada_is_aligner_type (type))
    {
      fprintf_filtered (stream, "%.*s",
			ada_name_prefix_len (type_name), type_name);
      return;
    }

  if (ada_is_aligner_type (type))
    ada_print_type (ada_aligned_type (type), "", stream, show, level, flags);
  else if (ada_is_constrained_packed_array_type (type)
	   && TYPE_CODE (type) != TYPE_CODE_PTR)
    print_array_type (type, stream, show, level, flags);
  else
    switch (TYPE_CODE (type))
      {
      default:
	fprintf_filtered (stream, "<");
	c_print_type (type, "", stream, show, level, flags);
	fprintf_filtered (stream, ">");
	break;
      case TYPE_CODE_PTR:
      case TYPE_CODE_TYPEDEF:
	fprintf_filtered (stream, "access ");
	ada_print_type (TYPE_TARGET_TYPE (type), "", stream, show, level,
			flags);
	break;
      case TYPE_CODE_REF:
	fprintf_filtered (stream, "<ref> ");
	ada_print_type (TYPE_TARGET_TYPE (type), "", stream, show, level,
			flags);
	break;
      case TYPE_CODE_ARRAY:
	print_array_type (type, stream, show, level, flags);
	break;
      case TYPE_CODE_BOOL:
	fprintf_filtered (stream, "(false, true)");
	break;
      case TYPE_CODE_INT:
	if (ada_is_fixed_point_type (type))
	  print_fixed_point_type (type, stream);
	else
	  {
	    const char *name = ada_type_name (type);

	    if (!ada_is_range_type_name (name))
	      fprintf_filtered (stream, _("<%d-byte integer>"),
				TYPE_LENGTH (type));
	    else
	      {
		fprintf_filtered (stream, "range ");
		print_range_type (type, stream);
	      }
	  }
	break;
      case TYPE_CODE_RANGE:
	if (ada_is_fixed_point_type (type))
	  print_fixed_point_type (type, stream);
	else if (ada_is_modular_type (type))
	  fprintf_filtered (stream, "mod %s", 
			    int_string (ada_modulus (type), 10, 0, 0, 1));
	else
	  {
	    fprintf_filtered (stream, "range ");
	    print_range (type, stream);
	  }
	break;
      case TYPE_CODE_FLT:
	fprintf_filtered (stream, _("<%d-byte float>"), TYPE_LENGTH (type));
	break;
      case TYPE_CODE_ENUM:
	if (show < 0)
	  fprintf_filtered (stream, "(...)");
	else
	  print_enum_type (type, stream);
	break;
      case TYPE_CODE_STRUCT:
	if (ada_is_array_descriptor_type (type))
	  print_array_type (type, stream, show, level, flags);
	else if (ada_is_bogus_array_descriptor (type))
	  fprintf_filtered (stream,
			    _("array (?) of ? (<mal-formed descriptor>)"));
	else
	  print_record_type (type, stream, show, level, flags);
	break;
      case TYPE_CODE_UNION:
	print_unchecked_union_type (type, stream, show, level, flags);
	break;
      case TYPE_CODE_FUNC:
	print_func_type (type, stream, varstring, flags);
	break;
      }
}

/* Implement the la_print_typedef language method for Ada.  */

void
ada_print_typedef (struct type *type, struct symbol *new_symbol,
                   struct ui_file *stream)
{
  type = ada_check_typedef (type);
  ada_print_type (type, "", stream, 0, 0, &type_print_raw_options);
  fprintf_filtered (stream, "\n");
}
