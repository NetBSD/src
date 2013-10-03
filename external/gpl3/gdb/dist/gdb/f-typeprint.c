/* Support for printing Fortran types for GDB, the GNU debugger.

   Copyright (C) 1986-2013 Free Software Foundation, Inc.

   Contributed by Motorola.  Adapted from the C version by Farooq Butt
   (fmbutt@engage.sps.mot.com).

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
#include "bfd.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "gdbcore.h"
#include "target.h"
#include "f-lang.h"

#include "gdb_string.h"
#include <errno.h>

#if 0				/* Currently unused.  */
static void f_type_print_args (struct type *, struct ui_file *);
#endif

static void f_type_print_varspec_suffix (struct type *, struct ui_file *, int,
					 int, int, int);

void f_type_print_varspec_prefix (struct type *, struct ui_file *,
				  int, int);

void f_type_print_base (struct type *, struct ui_file *, int, int);


/* LEVEL is the depth to indent lines by.  */

void
f_print_type (struct type *type, const char *varstring, struct ui_file *stream,
	      int show, int level, const struct type_print_options *flags)
{
  enum type_code code;
  int demangled_args;

  f_type_print_base (type, stream, show, level);
  code = TYPE_CODE (type);
  if ((varstring != NULL && *varstring != '\0')
  /* Need a space if going to print stars or brackets;
     but not if we will print just a type name.  */
      || ((show > 0 || TYPE_NAME (type) == 0)
          && (code == TYPE_CODE_PTR || code == TYPE_CODE_FUNC
	      || code == TYPE_CODE_METHOD
	      || code == TYPE_CODE_ARRAY
	      || code == TYPE_CODE_REF)))
    fputs_filtered (" ", stream);
  f_type_print_varspec_prefix (type, stream, show, 0);

  if (varstring != NULL)
    {
      fputs_filtered (varstring, stream);

      /* For demangled function names, we have the arglist as part of the name,
         so don't print an additional pair of ()'s.  */

      demangled_args = varstring[strlen (varstring) - 1] == ')'; 
      f_type_print_varspec_suffix (type, stream, show, 0, demangled_args, 0);
   }
}

/* Print any asterisks or open-parentheses needed before the
   variable name (to describe its type).

   On outermost call, pass 0 for PASSED_A_PTR.
   On outermost call, SHOW > 0 means should ignore
   any typename for TYPE and show its details.
   SHOW is always zero on recursive calls.  */

void
f_type_print_varspec_prefix (struct type *type, struct ui_file *stream,
			     int show, int passed_a_ptr)
{
  if (type == 0)
    return;

  if (TYPE_NAME (type) && show <= 0)
    return;

  QUIT;

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_PTR:
      f_type_print_varspec_prefix (TYPE_TARGET_TYPE (type), stream, 0, 1);
      break;

    case TYPE_CODE_FUNC:
      f_type_print_varspec_prefix (TYPE_TARGET_TYPE (type), stream, 0, 0);
      if (passed_a_ptr)
	fprintf_filtered (stream, "(");
      break;

    case TYPE_CODE_ARRAY:
      f_type_print_varspec_prefix (TYPE_TARGET_TYPE (type), stream, 0, 0);
      break;

    case TYPE_CODE_UNDEF:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_SET:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_STRING:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_REF:
    case TYPE_CODE_COMPLEX:
    case TYPE_CODE_TYPEDEF:
      /* These types need no prefix.  They are listed here so that
         gcc -Wall will reveal any types that haven't been handled.  */
      break;
    }
}

/* Print any array sizes, function arguments or close parentheses
   needed after the variable name (to describe its type).
   Args work like c_type_print_varspec_prefix.  */

static void
f_type_print_varspec_suffix (struct type *type, struct ui_file *stream,
			     int show, int passed_a_ptr, int demangled_args,
			     int arrayprint_recurse_level)
{
  int upper_bound, lower_bound;

  /* No static variables are permitted as an error call may occur during
     execution of this function.  */

  if (type == 0)
    return;

  if (TYPE_NAME (type) && show <= 0)
    return;

  QUIT;

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_ARRAY:
      arrayprint_recurse_level++;

      if (arrayprint_recurse_level == 1)
	fprintf_filtered (stream, "(");

      if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_ARRAY)
	f_type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream, 0, 0, 0,
				     arrayprint_recurse_level);

      lower_bound = f77_get_lowerbound (type);
      if (lower_bound != 1)	/* Not the default.  */
	fprintf_filtered (stream, "%d:", lower_bound);

      /* Make sure that, if we have an assumed size array, we
         print out a warning and print the upperbound as '*'.  */

      if (TYPE_ARRAY_UPPER_BOUND_IS_UNDEFINED (type))
	fprintf_filtered (stream, "*");
      else
	{
	  upper_bound = f77_get_upperbound (type);
	  fprintf_filtered (stream, "%d", upper_bound);
	}

      if (TYPE_CODE (TYPE_TARGET_TYPE (type)) != TYPE_CODE_ARRAY)
	f_type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream, 0, 0, 0,
				     arrayprint_recurse_level);
      if (arrayprint_recurse_level == 1)
	fprintf_filtered (stream, ")");
      else
	fprintf_filtered (stream, ",");
      arrayprint_recurse_level--;
      break;

    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
      f_type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream, 0, 1, 0,
				   arrayprint_recurse_level);
      fprintf_filtered (stream, ")");
      break;

    case TYPE_CODE_FUNC:
      f_type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream, 0,
				   passed_a_ptr, 0, arrayprint_recurse_level);
      if (passed_a_ptr)
	fprintf_filtered (stream, ")");

      fprintf_filtered (stream, "()");
      break;

    case TYPE_CODE_UNDEF:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_SET:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_STRING:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_COMPLEX:
    case TYPE_CODE_TYPEDEF:
      /* These types do not need a suffix.  They are listed so that
         gcc -Wall will report types that may not have been considered.  */
      break;
    }
}

/* Print the name of the type (or the ultimate pointer target,
   function value or array element), or the description of a
   structure or union.

   SHOW nonzero means don't print this type as just its name;
   show its real definition even if it has a name.
   SHOW zero means print just typename or struct tag if there is one
   SHOW negative means abbreviate structure elements.
   SHOW is decremented for printing of structure elements.

   LEVEL is the depth to indent by.
   We increase it for some recursive calls.  */

void
f_type_print_base (struct type *type, struct ui_file *stream, int show,
		   int level)
{
  int upper_bound;
  int index;

  QUIT;

  wrap_here ("    ");
  if (type == NULL)
    {
      fputs_filtered ("<type unknown>", stream);
      return;
    }

  /* When SHOW is zero or less, and there is a valid type name, then always
     just print the type name directly from the type.  */

  if ((show <= 0) && (TYPE_NAME (type) != NULL))
    {
      fputs_filtered (TYPE_NAME (type), stream);
      return;
    }

  if (TYPE_CODE (type) != TYPE_CODE_TYPEDEF)
    CHECK_TYPEDEF (type);

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_TYPEDEF:
      f_type_print_base (TYPE_TARGET_TYPE (type), stream, 0, level);
      break;

    case TYPE_CODE_ARRAY:
    case TYPE_CODE_FUNC:
      f_type_print_base (TYPE_TARGET_TYPE (type), stream, show, level);
      break;

    case TYPE_CODE_PTR:
      fprintf_filtered (stream, "PTR TO -> ( ");
      f_type_print_base (TYPE_TARGET_TYPE (type), stream, 0, level);
      break;

    case TYPE_CODE_REF:
      fprintf_filtered (stream, "REF TO -> ( ");
      f_type_print_base (TYPE_TARGET_TYPE (type), stream, 0, level);
      break;

    case TYPE_CODE_VOID:
      fprintfi_filtered (level, stream, "VOID");
      break;

    case TYPE_CODE_UNDEF:
      fprintfi_filtered (level, stream, "struct <unknown>");
      break;

    case TYPE_CODE_ERROR:
      fprintfi_filtered (level, stream, "%s", TYPE_ERROR_NAME (type));
      break;

    case TYPE_CODE_RANGE:
      /* This should not occur.  */
      fprintfi_filtered (level, stream, "<range type>");
      break;

    case TYPE_CODE_CHAR:
    case TYPE_CODE_INT:
      /* There may be some character types that attempt to come
         through as TYPE_CODE_INT since dbxstclass.h is so
         C-oriented, we must change these to "character" from "char".  */

      if (strcmp (TYPE_NAME (type), "char") == 0)
	fprintfi_filtered (level, stream, "character");
      else
	goto default_case;
      break;

    case TYPE_CODE_STRING:
      /* Strings may have dynamic upperbounds (lengths) like arrays.  */

      if (TYPE_ARRAY_UPPER_BOUND_IS_UNDEFINED (type))
	fprintfi_filtered (level, stream, "character*(*)");
      else
	{
	  upper_bound = f77_get_upperbound (type);
	  fprintf_filtered (stream, "character*%d", upper_bound);
	}
      break;

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      if (TYPE_CODE (type) == TYPE_CODE_UNION)
	fprintfi_filtered (level, stream, "Type, C_Union :: ");
      else
	fprintfi_filtered (level, stream, "Type ");
      fputs_filtered (TYPE_TAG_NAME (type), stream);
      fputs_filtered ("\n", stream);
      for (index = 0; index < TYPE_NFIELDS (type); index++)
	{
	  f_type_print_base (TYPE_FIELD_TYPE (type, index), stream, show,
			     level + 4);
	  fputs_filtered (" :: ", stream);
	  fputs_filtered (TYPE_FIELD_NAME (type, index), stream);
	  f_type_print_varspec_suffix (TYPE_FIELD_TYPE (type, index),
				       stream, 0, 0, 0, 0);
	  fputs_filtered ("\n", stream);
	} 
      fprintfi_filtered (level, stream, "End Type ");
      fputs_filtered (TYPE_TAG_NAME (type), stream);
      break;

    case TYPE_CODE_MODULE:
      fprintfi_filtered (level, stream, "module %s", TYPE_TAG_NAME (type));
      break;

    default_case:
    default:
      /* Handle types not explicitly handled by the other cases,
         such as fundamental types.  For these, just print whatever
         the type name is, as recorded in the type itself.  If there
         is no type name, then complain.  */
      if (TYPE_NAME (type) != NULL)
	fprintfi_filtered (level, stream, "%s", TYPE_NAME (type));
      else
	error (_("Invalid type code (%d) in symbol table."), TYPE_CODE (type));
      break;
    }
}
