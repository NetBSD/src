/* Support for printing Fortran values for GDB, the GNU debugger.

   Copyright (C) 1993-2016 Free Software Foundation, Inc.

   Contributed by Motorola.  Adapted from the C definitions by Farooq Butt
   (fmbutt@engage.sps.mot.com), additionally worked over by Stan Shebs.

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
#include "f-lang.h"
#include "frame.h"
#include "gdbcore.h"
#include "command.h"
#include "block.h"
#include "dictionary.h"

extern void _initialize_f_valprint (void);
static void info_common_command (char *, int);
static void f77_get_dynamic_length_of_aggregate (struct type *);

int f77_array_offset_tbl[MAX_FORTRAN_DIMS + 1][2];

/* Array which holds offsets to be applied to get a row's elements
   for a given array.  Array also holds the size of each subarray.  */

int
f77_get_lowerbound (struct type *type)
{
  if (TYPE_ARRAY_LOWER_BOUND_IS_UNDEFINED (type))
    error (_("Lower bound may not be '*' in F77"));

  return TYPE_ARRAY_LOWER_BOUND_VALUE (type);
}

int
f77_get_upperbound (struct type *type)
{
  if (TYPE_ARRAY_UPPER_BOUND_IS_UNDEFINED (type))
    {
      /* We have an assumed size array on our hands.  Assume that
	 upper_bound == lower_bound so that we show at least 1 element.
	 If the user wants to see more elements, let him manually ask for 'em
	 and we'll subscript the array and show him.  */

      return f77_get_lowerbound (type);
    }

  return TYPE_ARRAY_UPPER_BOUND_VALUE (type);
}

/* Obtain F77 adjustable array dimensions.  */

static void
f77_get_dynamic_length_of_aggregate (struct type *type)
{
  int upper_bound = -1;
  int lower_bound = 1;

  /* Recursively go all the way down into a possibly multi-dimensional
     F77 array and get the bounds.  For simple arrays, this is pretty
     easy but when the bounds are dynamic, we must be very careful 
     to add up all the lengths correctly.  Not doing this right 
     will lead to horrendous-looking arrays in parameter lists.

     This function also works for strings which behave very 
     similarly to arrays.  */

  if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_ARRAY
      || TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_STRING)
    f77_get_dynamic_length_of_aggregate (TYPE_TARGET_TYPE (type));

  /* Recursion ends here, start setting up lengths.  */
  lower_bound = f77_get_lowerbound (type);
  upper_bound = f77_get_upperbound (type);

  /* Patch in a valid length value.  */

  TYPE_LENGTH (type) =
    (upper_bound - lower_bound + 1)
    * TYPE_LENGTH (check_typedef (TYPE_TARGET_TYPE (type)));
}

/* Actual function which prints out F77 arrays, Valaddr == address in 
   the superior.  Address == the address in the inferior.  */

static void
f77_print_array_1 (int nss, int ndimensions, struct type *type,
		   const gdb_byte *valaddr,
		   int embedded_offset, CORE_ADDR address,
		   struct ui_file *stream, int recurse,
		   const struct value *val,
		   const struct value_print_options *options,
		   int *elts)
{
  struct type *range_type = TYPE_INDEX_TYPE (check_typedef (type));
  CORE_ADDR addr = address + embedded_offset;
  LONGEST lowerbound, upperbound;
  int i;

  get_discrete_bounds (range_type, &lowerbound, &upperbound);

  if (nss != ndimensions)
    {
      size_t dim_size = TYPE_LENGTH (TYPE_TARGET_TYPE (type));
      size_t offs = 0;

      for (i = lowerbound;
	   (i < upperbound + 1 && (*elts) < options->print_max);
	   i++)
	{
	  struct value *subarray = value_from_contents_and_address
	    (TYPE_TARGET_TYPE (type), value_contents_for_printing_const (val)
	     + offs, addr + offs);

	  fprintf_filtered (stream, "( ");
	  f77_print_array_1 (nss + 1, ndimensions, value_type (subarray),
			     value_contents_for_printing (subarray),
			     value_embedded_offset (subarray),
			     value_address (subarray),
			     stream, recurse, subarray, options, elts);
	  offs += dim_size;
	  fprintf_filtered (stream, ") ");
	}
      if (*elts >= options->print_max && i < upperbound)
	fprintf_filtered (stream, "...");
    }
  else
    {
      for (i = lowerbound; i < upperbound + 1 && (*elts) < options->print_max;
	   i++, (*elts)++)
	{
	  struct value *elt = value_subscript ((struct value *)val, i);

	  val_print (value_type (elt),
		     value_contents_for_printing (elt),
		     value_embedded_offset (elt),
		     value_address (elt), stream, recurse,
		     elt, options, current_language);

	  if (i != upperbound)
	    fprintf_filtered (stream, ", ");

	  if ((*elts == options->print_max - 1)
	      && (i != upperbound))
	    fprintf_filtered (stream, "...");
	}
    }
}

/* This function gets called to print an F77 array, we set up some 
   stuff and then immediately call f77_print_array_1().  */

static void
f77_print_array (struct type *type, const gdb_byte *valaddr,
		 int embedded_offset,
		 CORE_ADDR address, struct ui_file *stream,
		 int recurse,
		 const struct value *val,
		 const struct value_print_options *options)
{
  int ndimensions;
  int elts = 0;

  ndimensions = calc_f77_array_dims (type);

  if (ndimensions > MAX_FORTRAN_DIMS || ndimensions < 0)
    error (_("\
Type node corrupt! F77 arrays cannot have %d subscripts (%d Max)"),
	   ndimensions, MAX_FORTRAN_DIMS);

  f77_print_array_1 (1, ndimensions, type, valaddr, embedded_offset,
		     address, stream, recurse, val, options, &elts);
}


/* Decorations for Fortran.  */

static const struct generic_val_print_decorations f_decorations =
{
  "(",
  ",",
  ")",
  ".TRUE.",
  ".FALSE.",
  "VOID",
  "{",
  "}"
};

/* See val_print for a description of the various parameters of this
   function; they are identical.  */

void
f_val_print (struct type *type, const gdb_byte *valaddr, int embedded_offset,
	     CORE_ADDR address, struct ui_file *stream, int recurse,
	     const struct value *original_value,
	     const struct value_print_options *options)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int printed_field = 0; /* Number of fields printed.  */
  struct type *elttype;
  CORE_ADDR addr;
  int index;

  type = check_typedef (type);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_STRING:
      f77_get_dynamic_length_of_aggregate (type);
      LA_PRINT_STRING (stream, builtin_type (gdbarch)->builtin_char,
		       valaddr + embedded_offset,
		       TYPE_LENGTH (type), NULL, 0, options);
      break;

    case TYPE_CODE_ARRAY:
      if (TYPE_CODE (TYPE_TARGET_TYPE (type)) != TYPE_CODE_CHAR)
	{
	  fprintf_filtered (stream, "(");
	  f77_print_array (type, valaddr, embedded_offset,
			   address, stream, recurse, original_value, options);
	  fprintf_filtered (stream, ")");
	}
      else
	{
	  struct type *ch_type = TYPE_TARGET_TYPE (type);

	  f77_get_dynamic_length_of_aggregate (type);
	  LA_PRINT_STRING (stream, ch_type,
			   valaddr + embedded_offset,
			   TYPE_LENGTH (type) / TYPE_LENGTH (ch_type),
			   NULL, 0, options);
	}
      break;

    case TYPE_CODE_PTR:
      if (options->format && options->format != 's')
	{
	  val_print_scalar_formatted (type, valaddr, embedded_offset,
				      original_value, options, 0, stream);
	  break;
	}
      else
	{
	  int want_space = 0;

	  addr = unpack_pointer (type, valaddr + embedded_offset);
	  elttype = check_typedef (TYPE_TARGET_TYPE (type));

	  if (TYPE_CODE (elttype) == TYPE_CODE_FUNC)
	    {
	      /* Try to print what function it points to.  */
	      print_function_pointer_address (options, gdbarch, addr, stream);
	      return;
	    }

	  if (options->symbol_print)
	    want_space = print_address_demangle (options, gdbarch, addr,
						 stream, demangle);
	  else if (options->addressprint && options->format != 's')
	    {
	      fputs_filtered (paddress (gdbarch, addr), stream);
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
	      val_print_string (TYPE_TARGET_TYPE (type), NULL, addr, -1,
				stream, options);
	    }
	  return;
	}
      break;

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
	{
	  val_print_type_code_int (type, valaddr + embedded_offset, stream);
	  /* C and C++ has no single byte int type, char is used instead.
	     Since we don't know whether the value is really intended to
	     be used as an integer or a character, print the character
	     equivalent as well.  */
	  if (TYPE_LENGTH (type) == 1)
	    {
	      LONGEST c;

	      fputs_filtered (" ", stream);
	      c = unpack_long (type, valaddr + embedded_offset);
	      LA_PRINT_CHAR ((unsigned char) c, type, stream);
	    }
	}
      break;

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      /* Starting from the Fortran 90 standard, Fortran supports derived
         types.  */
      fprintf_filtered (stream, "( ");
      for (index = 0; index < TYPE_NFIELDS (type); index++)
        {
	  struct value *field = value_field
	    ((struct value *)original_value, index);

	  struct type *field_type = check_typedef (TYPE_FIELD_TYPE (type, index));


	  if (TYPE_CODE (field_type) != TYPE_CODE_FUNC)
	    {
	      const char *field_name;

	      if (printed_field > 0)
		fputs_filtered (", ", stream);

	      field_name = TYPE_FIELD_NAME (type, index);
	      if (field_name != NULL)
		{
		  fputs_filtered (field_name, stream);
		  fputs_filtered (" = ", stream);
		}

	      val_print (value_type (field),
			 value_contents_for_printing (field),
			 value_embedded_offset (field),
			 value_address (field), stream, recurse + 1,
			 field, options, current_language);

	      ++printed_field;
	    }
	 }
      fprintf_filtered (stream, " )");
      break;     

    case TYPE_CODE_REF:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_UNDEF:
    case TYPE_CODE_COMPLEX:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
    default:
      generic_val_print (type, valaddr, embedded_offset, address,
			 stream, recurse, original_value, options,
			 &f_decorations);
      break;
    }
  gdb_flush (stream);
}

static void
info_common_command_for_block (const struct block *block, const char *comname,
			       int *any_printed)
{
  struct block_iterator iter;
  struct symbol *sym;
  const char *name;
  struct value_print_options opts;

  get_user_print_options (&opts);

  ALL_BLOCK_SYMBOLS (block, iter, sym)
    if (SYMBOL_DOMAIN (sym) == COMMON_BLOCK_DOMAIN)
      {
	const struct common_block *common = SYMBOL_VALUE_COMMON_BLOCK (sym);
	size_t index;

	gdb_assert (SYMBOL_CLASS (sym) == LOC_COMMON_BLOCK);

	if (comname && (!SYMBOL_LINKAGE_NAME (sym)
	                || strcmp (comname, SYMBOL_LINKAGE_NAME (sym)) != 0))
	  continue;

	if (*any_printed)
	  putchar_filtered ('\n');
	else
	  *any_printed = 1;
	if (SYMBOL_PRINT_NAME (sym))
	  printf_filtered (_("Contents of F77 COMMON block '%s':\n"),
			   SYMBOL_PRINT_NAME (sym));
	else
	  printf_filtered (_("Contents of blank COMMON block:\n"));
	
	for (index = 0; index < common->n_entries; index++)
	  {
	    struct value *val = NULL;

	    printf_filtered ("%s = ",
			     SYMBOL_PRINT_NAME (common->contents[index]));

	    TRY
	      {
		val = value_of_variable (common->contents[index], block);
		value_print (val, gdb_stdout, &opts);
	      }

	    CATCH (except, RETURN_MASK_ERROR)
	      {
		printf_filtered ("<error reading variable: %s>", except.message);
	      }
	    END_CATCH

	    putchar_filtered ('\n');
	  }
      }
}

/* This function is used to print out the values in a given COMMON 
   block.  It will always use the most local common block of the 
   given name.  */

static void
info_common_command (char *comname, int from_tty)
{
  struct frame_info *fi;
  const struct block *block;
  int values_printed = 0;

  /* We have been told to display the contents of F77 COMMON 
     block supposedly visible in this function.  Let us 
     first make sure that it is visible and if so, let 
     us display its contents.  */

  fi = get_selected_frame (_("No frame selected"));

  /* The following is generally ripped off from stack.c's routine 
     print_frame_info().  */

  block = get_frame_block (fi, 0);
  if (block == NULL)
    {
      printf_filtered (_("No symbol table info available.\n"));
      return;
    }

  while (block)
    {
      info_common_command_for_block (block, comname, &values_printed);
      /* After handling the function's top-level block, stop.  Don't
         continue to its superblock, the block of per-file symbols.  */
      if (BLOCK_FUNCTION (block))
	break;
      block = BLOCK_SUPERBLOCK (block);
    }

  if (!values_printed)
    {
      if (comname)
	printf_filtered (_("No common block '%s'.\n"), comname);
      else
	printf_filtered (_("No common blocks.\n"));
    }
}

void
_initialize_f_valprint (void)
{
  add_info ("common", info_common_command,
	    _("Print out the values contained in a Fortran COMMON block."));
}
