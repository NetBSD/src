// OBSOLETE /* Support for printing Chill values for GDB, the GNU debugger.
// OBSOLETE    Copyright 1986, 1988, 1989, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
// OBSOLETE    1998, 2000, 2001
// OBSOLETE    Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    This file is part of GDB.
// OBSOLETE 
// OBSOLETE    This program is free software; you can redistribute it and/or modify
// OBSOLETE    it under the terms of the GNU General Public License as published by
// OBSOLETE    the Free Software Foundation; either version 2 of the License, or
// OBSOLETE    (at your option) any later version.
// OBSOLETE 
// OBSOLETE    This program is distributed in the hope that it will be useful,
// OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE    GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE    You should have received a copy of the GNU General Public License
// OBSOLETE    along with this program; if not, write to the Free Software
// OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330,
// OBSOLETE    Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE #include "defs.h"
// OBSOLETE #include "gdb_obstack.h"
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "gdbtypes.h"
// OBSOLETE #include "valprint.h"
// OBSOLETE #include "expression.h"
// OBSOLETE #include "value.h"
// OBSOLETE #include "language.h"
// OBSOLETE #include "demangle.h"
// OBSOLETE #include "c-lang.h"		/* For c_val_print */
// OBSOLETE #include "typeprint.h"
// OBSOLETE #include "ch-lang.h"
// OBSOLETE #include "annotate.h"
// OBSOLETE 
// OBSOLETE static void chill_print_value_fields (struct type *, char *,
// OBSOLETE 				      struct ui_file *, int, int,
// OBSOLETE 				      enum val_prettyprint, struct type **);
// OBSOLETE 
// OBSOLETE static void chill_print_type_scalar (struct type *, LONGEST,
// OBSOLETE 				     struct ui_file *);
// OBSOLETE 
// OBSOLETE static void chill_val_print_array_elements (struct type *, char *,
// OBSOLETE 					    CORE_ADDR, struct ui_file *,
// OBSOLETE 					    int, int, int,
// OBSOLETE 					    enum val_prettyprint);
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Print integral scalar data VAL, of type TYPE, onto stdio stream STREAM.
// OBSOLETE    Used to print data from type structures in a specified type.  For example,
// OBSOLETE    array bounds may be characters or booleans in some languages, and this
// OBSOLETE    allows the ranges to be printed in their "natural" form rather than as
// OBSOLETE    decimal integer values. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE chill_print_type_scalar (struct type *type, LONGEST val, struct ui_file *stream)
// OBSOLETE {
// OBSOLETE   switch (TYPE_CODE (type))
// OBSOLETE     {
// OBSOLETE     case TYPE_CODE_RANGE:
// OBSOLETE       if (TYPE_TARGET_TYPE (type))
// OBSOLETE 	{
// OBSOLETE 	  chill_print_type_scalar (TYPE_TARGET_TYPE (type), val, stream);
// OBSOLETE 	  return;
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE     case TYPE_CODE_UNDEF:
// OBSOLETE     case TYPE_CODE_PTR:
// OBSOLETE     case TYPE_CODE_ARRAY:
// OBSOLETE     case TYPE_CODE_STRUCT:
// OBSOLETE     case TYPE_CODE_UNION:
// OBSOLETE     case TYPE_CODE_ENUM:
// OBSOLETE     case TYPE_CODE_FUNC:
// OBSOLETE     case TYPE_CODE_INT:
// OBSOLETE     case TYPE_CODE_FLT:
// OBSOLETE     case TYPE_CODE_VOID:
// OBSOLETE     case TYPE_CODE_SET:
// OBSOLETE     case TYPE_CODE_STRING:
// OBSOLETE     case TYPE_CODE_BITSTRING:
// OBSOLETE     case TYPE_CODE_ERROR:
// OBSOLETE     case TYPE_CODE_MEMBER:
// OBSOLETE     case TYPE_CODE_METHOD:
// OBSOLETE     case TYPE_CODE_REF:
// OBSOLETE     case TYPE_CODE_CHAR:
// OBSOLETE     case TYPE_CODE_BOOL:
// OBSOLETE     case TYPE_CODE_COMPLEX:
// OBSOLETE     case TYPE_CODE_TYPEDEF:
// OBSOLETE     default:
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE   print_type_scalar (type, val, stream);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Print the elements of an array.
// OBSOLETE    Similar to val_print_array_elements, but prints
// OBSOLETE    element indexes (in Chill syntax). */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE chill_val_print_array_elements (struct type *type, char *valaddr,
// OBSOLETE 				CORE_ADDR address, struct ui_file *stream,
// OBSOLETE 				int format, int deref_ref, int recurse,
// OBSOLETE 				enum val_prettyprint pretty)
// OBSOLETE {
// OBSOLETE   unsigned int i = 0;
// OBSOLETE   unsigned int things_printed = 0;
// OBSOLETE   unsigned len;
// OBSOLETE   struct type *elttype;
// OBSOLETE   struct type *range_type = TYPE_FIELD_TYPE (type, 0);
// OBSOLETE   struct type *index_type = TYPE_TARGET_TYPE (range_type);
// OBSOLETE   unsigned eltlen;
// OBSOLETE   /* Position of the array element we are examining to see
// OBSOLETE      whether it is repeated.  */
// OBSOLETE   unsigned int rep1;
// OBSOLETE   /* Number of repetitions we have detected so far.  */
// OBSOLETE   unsigned int reps;
// OBSOLETE   LONGEST low_bound = TYPE_FIELD_BITPOS (range_type, 0);
// OBSOLETE 
// OBSOLETE   elttype = check_typedef (TYPE_TARGET_TYPE (type));
// OBSOLETE   eltlen = TYPE_LENGTH (elttype);
// OBSOLETE   len = TYPE_LENGTH (type) / eltlen;
// OBSOLETE 
// OBSOLETE   annotate_array_section_begin (i, elttype);
// OBSOLETE 
// OBSOLETE   for (; i < len && things_printed < print_max; i++)
// OBSOLETE     {
// OBSOLETE       if (i != 0)
// OBSOLETE 	{
// OBSOLETE 	  if (prettyprint_arrays)
// OBSOLETE 	    {
// OBSOLETE 	      fprintf_filtered (stream, ",\n");
// OBSOLETE 	      print_spaces_filtered (2 + 2 * recurse, stream);
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    {
// OBSOLETE 	      fprintf_filtered (stream, ", ");
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       wrap_here (n_spaces (2 + 2 * recurse));
// OBSOLETE 
// OBSOLETE       rep1 = i + 1;
// OBSOLETE       reps = 1;
// OBSOLETE       while ((rep1 < len) &&
// OBSOLETE 	     !memcmp (valaddr + i * eltlen, valaddr + rep1 * eltlen, eltlen))
// OBSOLETE 	{
// OBSOLETE 	  ++reps;
// OBSOLETE 	  ++rep1;
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       fputs_filtered ("(", stream);
// OBSOLETE       chill_print_type_scalar (index_type, low_bound + i, stream);
// OBSOLETE       if (reps > 1)
// OBSOLETE 	{
// OBSOLETE 	  fputs_filtered (":", stream);
// OBSOLETE 	  chill_print_type_scalar (index_type, low_bound + i + reps - 1,
// OBSOLETE 				   stream);
// OBSOLETE 	  fputs_filtered ("): ", stream);
// OBSOLETE 	  val_print (elttype, valaddr + i * eltlen, 0, 0, stream, format,
// OBSOLETE 		     deref_ref, recurse + 1, pretty);
// OBSOLETE 
// OBSOLETE 	  i = rep1 - 1;
// OBSOLETE 	  things_printed += 1;
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  fputs_filtered ("): ", stream);
// OBSOLETE 	  val_print (elttype, valaddr + i * eltlen, 0, 0, stream, format,
// OBSOLETE 		     deref_ref, recurse + 1, pretty);
// OBSOLETE 	  annotate_elt ();
// OBSOLETE 	  things_printed++;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   annotate_array_section_end ();
// OBSOLETE   if (i < len)
// OBSOLETE     {
// OBSOLETE       fprintf_filtered (stream, "...");
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Print data of type TYPE located at VALADDR (within GDB), which came from
// OBSOLETE    the inferior at address ADDRESS, onto stdio stream STREAM according to
// OBSOLETE    FORMAT (a letter or 0 for natural format).  The data at VALADDR is in
// OBSOLETE    target byte order.
// OBSOLETE 
// OBSOLETE    If the data are a string pointer, returns the number of string characters
// OBSOLETE    printed.
// OBSOLETE 
// OBSOLETE    If DEREF_REF is nonzero, then dereference references, otherwise just print
// OBSOLETE    them like pointers.
// OBSOLETE 
// OBSOLETE    The PRETTY parameter controls prettyprinting.  */
// OBSOLETE 
// OBSOLETE int
// OBSOLETE chill_val_print (struct type *type, char *valaddr, int embedded_offset,
// OBSOLETE 		 CORE_ADDR address, struct ui_file *stream, int format,
// OBSOLETE 		 int deref_ref, int recurse, enum val_prettyprint pretty)
// OBSOLETE {
// OBSOLETE   LONGEST val;
// OBSOLETE   unsigned int i = 0;		/* Number of characters printed.  */
// OBSOLETE   struct type *elttype;
// OBSOLETE   CORE_ADDR addr;
// OBSOLETE 
// OBSOLETE   CHECK_TYPEDEF (type);
// OBSOLETE 
// OBSOLETE   switch (TYPE_CODE (type))
// OBSOLETE     {
// OBSOLETE     case TYPE_CODE_ARRAY:
// OBSOLETE       if (TYPE_LENGTH (type) > 0 && TYPE_LENGTH (TYPE_TARGET_TYPE (type)) > 0)
// OBSOLETE 	{
// OBSOLETE 	  if (prettyprint_arrays)
// OBSOLETE 	    {
// OBSOLETE 	      print_spaces_filtered (2 + 2 * recurse, stream);
// OBSOLETE 	    }
// OBSOLETE 	  fprintf_filtered (stream, "[");
// OBSOLETE 	  chill_val_print_array_elements (type, valaddr, address, stream,
// OBSOLETE 					format, deref_ref, recurse, pretty);
// OBSOLETE 	  fprintf_filtered (stream, "]");
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  error ("unimplemented in chill_val_print; unspecified array length");
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_INT:
// OBSOLETE       format = format ? format : output_format;
// OBSOLETE       if (format)
// OBSOLETE 	{
// OBSOLETE 	  print_scalar_formatted (valaddr, type, format, 0, stream);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  val_print_type_code_int (type, valaddr, stream);
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_CHAR:
// OBSOLETE       format = format ? format : output_format;
// OBSOLETE       if (format)
// OBSOLETE 	{
// OBSOLETE 	  print_scalar_formatted (valaddr, type, format, 0, stream);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  LA_PRINT_CHAR ((unsigned char) unpack_long (type, valaddr),
// OBSOLETE 			 stream);
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_FLT:
// OBSOLETE       if (format)
// OBSOLETE 	{
// OBSOLETE 	  print_scalar_formatted (valaddr, type, format, 0, stream);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  print_floating (valaddr, type, stream);
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_BOOL:
// OBSOLETE       format = format ? format : output_format;
// OBSOLETE       if (format)
// OBSOLETE 	{
// OBSOLETE 	  print_scalar_formatted (valaddr, type, format, 0, stream);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  /* FIXME: Why is this using builtin_type_chill_bool not type?  */
// OBSOLETE 	  val = unpack_long (builtin_type_chill_bool, valaddr);
// OBSOLETE 	  fprintf_filtered (stream, val ? "TRUE" : "FALSE");
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_UNDEF:
// OBSOLETE       /* This happens (without TYPE_FLAG_STUB set) on systems which don't use
// OBSOLETE          dbx xrefs (NO_DBX_XREFS in gcc) if a file has a "struct foo *bar"
// OBSOLETE          and no complete type for struct foo in that file.  */
// OBSOLETE       fprintf_filtered (stream, "<incomplete type>");
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_PTR:
// OBSOLETE       if (format && format != 's')
// OBSOLETE 	{
// OBSOLETE 	  print_scalar_formatted (valaddr, type, format, 0, stream);
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE       addr = unpack_pointer (type, valaddr);
// OBSOLETE       elttype = check_typedef (TYPE_TARGET_TYPE (type));
// OBSOLETE 
// OBSOLETE       /* We assume a NULL pointer is all zeros ... */
// OBSOLETE       if (addr == 0)
// OBSOLETE 	{
// OBSOLETE 	  fputs_filtered ("NULL", stream);
// OBSOLETE 	  return 0;
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       if (TYPE_CODE (elttype) == TYPE_CODE_FUNC)
// OBSOLETE 	{
// OBSOLETE 	  /* Try to print what function it points to.  */
// OBSOLETE 	  print_address_demangle (addr, stream, demangle);
// OBSOLETE 	  /* Return value is irrelevant except for string pointers.  */
// OBSOLETE 	  return (0);
// OBSOLETE 	}
// OBSOLETE       if (addressprint && format != 's')
// OBSOLETE 	{
// OBSOLETE 	  print_address_numeric (addr, 1, stream);
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       /* For a pointer to char or unsigned char, also print the string
// OBSOLETE          pointed to, unless pointer is null.  */
// OBSOLETE       if (TYPE_LENGTH (elttype) == 1
// OBSOLETE 	  && TYPE_CODE (elttype) == TYPE_CODE_CHAR
// OBSOLETE 	  && (format == 0 || format == 's')
// OBSOLETE 	  && addr != 0
// OBSOLETE 	  &&			/* If print_max is UINT_MAX, the alloca below will fail.
// OBSOLETE 				   In that case don't try to print the string.  */
// OBSOLETE 	  print_max < UINT_MAX)
// OBSOLETE 	i = val_print_string (addr, -1, TYPE_LENGTH (elttype), stream);
// OBSOLETE 
// OBSOLETE       /* Return number of characters printed, plus one for the
// OBSOLETE          terminating null if we have "reached the end".  */
// OBSOLETE       return (i + (print_max && i != print_max));
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_STRING:
// OBSOLETE       i = TYPE_LENGTH (type);
// OBSOLETE       LA_PRINT_STRING (stream, valaddr, i, 1, 0);
// OBSOLETE       /* Return number of characters printed, plus one for the terminating
// OBSOLETE          null if we have "reached the end".  */
// OBSOLETE       return (i + (print_max && i != print_max));
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_BITSTRING:
// OBSOLETE     case TYPE_CODE_SET:
// OBSOLETE       elttype = TYPE_INDEX_TYPE (type);
// OBSOLETE       CHECK_TYPEDEF (elttype);
// OBSOLETE       if (TYPE_STUB (elttype))
// OBSOLETE 	{
// OBSOLETE 	  fprintf_filtered (stream, "<incomplete type>");
// OBSOLETE 	  gdb_flush (stream);
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE       {
// OBSOLETE 	struct type *range = elttype;
// OBSOLETE 	LONGEST low_bound, high_bound;
// OBSOLETE 	int i;
// OBSOLETE 	int is_bitstring = TYPE_CODE (type) == TYPE_CODE_BITSTRING;
// OBSOLETE 	int need_comma = 0;
// OBSOLETE 
// OBSOLETE 	if (is_bitstring)
// OBSOLETE 	  fputs_filtered ("B'", stream);
// OBSOLETE 	else
// OBSOLETE 	  fputs_filtered ("[", stream);
// OBSOLETE 
// OBSOLETE 	i = get_discrete_bounds (range, &low_bound, &high_bound);
// OBSOLETE       maybe_bad_bstring:
// OBSOLETE 	if (i < 0)
// OBSOLETE 	  {
// OBSOLETE 	    fputs_filtered ("<error value>", stream);
// OBSOLETE 	    goto done;
// OBSOLETE 	  }
// OBSOLETE 
// OBSOLETE 	for (i = low_bound; i <= high_bound; i++)
// OBSOLETE 	  {
// OBSOLETE 	    int element = value_bit_index (type, valaddr, i);
// OBSOLETE 	    if (element < 0)
// OBSOLETE 	      {
// OBSOLETE 		i = element;
// OBSOLETE 		goto maybe_bad_bstring;
// OBSOLETE 	      }
// OBSOLETE 	    if (is_bitstring)
// OBSOLETE 	      fprintf_filtered (stream, "%d", element);
// OBSOLETE 	    else if (element)
// OBSOLETE 	      {
// OBSOLETE 		if (need_comma)
// OBSOLETE 		  fputs_filtered (", ", stream);
// OBSOLETE 		chill_print_type_scalar (range, (LONGEST) i, stream);
// OBSOLETE 		need_comma = 1;
// OBSOLETE 
// OBSOLETE 		/* Look for a continuous range of true elements. */
// OBSOLETE 		if (i + 1 <= high_bound && value_bit_index (type, valaddr, ++i))
// OBSOLETE 		  {
// OBSOLETE 		    int j = i;	/* j is the upper bound so far of the range */
// OBSOLETE 		    fputs_filtered (":", stream);
// OBSOLETE 		    while (i + 1 <= high_bound
// OBSOLETE 			   && value_bit_index (type, valaddr, ++i))
// OBSOLETE 		      j = i;
// OBSOLETE 		    chill_print_type_scalar (range, (LONGEST) j, stream);
// OBSOLETE 		  }
// OBSOLETE 	      }
// OBSOLETE 	  }
// OBSOLETE       done:
// OBSOLETE 	if (is_bitstring)
// OBSOLETE 	  fputs_filtered ("'", stream);
// OBSOLETE 	else
// OBSOLETE 	  fputs_filtered ("]", stream);
// OBSOLETE       }
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_STRUCT:
// OBSOLETE       if (chill_varying_type (type))
// OBSOLETE 	{
// OBSOLETE 	  struct type *inner = check_typedef (TYPE_FIELD_TYPE (type, 1));
// OBSOLETE 	  long length = unpack_long (TYPE_FIELD_TYPE (type, 0), valaddr);
// OBSOLETE 	  char *data_addr = valaddr + TYPE_FIELD_BITPOS (type, 1) / 8;
// OBSOLETE 
// OBSOLETE 	  switch (TYPE_CODE (inner))
// OBSOLETE 	    {
// OBSOLETE 	    case TYPE_CODE_STRING:
// OBSOLETE 	      if (length > TYPE_LENGTH (type) - 2)
// OBSOLETE 		{
// OBSOLETE 		  fprintf_filtered (stream,
// OBSOLETE 			"<dynamic length %ld > static length %d> *invalid*",
// OBSOLETE 				    length, TYPE_LENGTH (type));
// OBSOLETE 
// OBSOLETE 		  /* Don't print the string; doing so might produce a
// OBSOLETE 		     segfault.  */
// OBSOLETE 		  return length;
// OBSOLETE 		}
// OBSOLETE 	      LA_PRINT_STRING (stream, data_addr, length, 1, 0);
// OBSOLETE 	      return length;
// OBSOLETE 	    default:
// OBSOLETE 	      break;
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       chill_print_value_fields (type, valaddr, stream, format, recurse, pretty,
// OBSOLETE 				0);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_REF:
// OBSOLETE       if (addressprint)
// OBSOLETE 	{
// OBSOLETE 	  fprintf_filtered (stream, "LOC(");
// OBSOLETE 	  print_address_numeric
// OBSOLETE 	    (extract_address (valaddr, TARGET_PTR_BIT / HOST_CHAR_BIT),
// OBSOLETE 	     1,
// OBSOLETE 	     stream);
// OBSOLETE 	  fprintf_filtered (stream, ")");
// OBSOLETE 	  if (deref_ref)
// OBSOLETE 	    fputs_filtered (": ", stream);
// OBSOLETE 	}
// OBSOLETE       /* De-reference the reference.  */
// OBSOLETE       if (deref_ref)
// OBSOLETE 	{
// OBSOLETE 	  if (TYPE_CODE (TYPE_TARGET_TYPE (type)) != TYPE_CODE_UNDEF)
// OBSOLETE 	    {
// OBSOLETE 	      struct value *deref_val =
// OBSOLETE 	      value_at
// OBSOLETE 	      (TYPE_TARGET_TYPE (type),
// OBSOLETE 	       unpack_pointer (lookup_pointer_type (builtin_type_void),
// OBSOLETE 			       valaddr),
// OBSOLETE 	       NULL);
// OBSOLETE 	      val_print (VALUE_TYPE (deref_val),
// OBSOLETE 			 VALUE_CONTENTS (deref_val),
// OBSOLETE 			 0,
// OBSOLETE 			 VALUE_ADDRESS (deref_val), stream, format,
// OBSOLETE 			 deref_ref, recurse + 1, pretty);
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    fputs_filtered ("???", stream);
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_ENUM:
// OBSOLETE       c_val_print (type, valaddr, 0, address, stream, format,
// OBSOLETE 		   deref_ref, recurse, pretty);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_RANGE:
// OBSOLETE       if (TYPE_TARGET_TYPE (type))
// OBSOLETE 	chill_val_print (TYPE_TARGET_TYPE (type), valaddr, 0, address, stream,
// OBSOLETE 			 format, deref_ref, recurse, pretty);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_MEMBER:
// OBSOLETE     case TYPE_CODE_UNION:
// OBSOLETE     case TYPE_CODE_FUNC:
// OBSOLETE     case TYPE_CODE_VOID:
// OBSOLETE     case TYPE_CODE_ERROR:
// OBSOLETE     default:
// OBSOLETE       /* Let's defer printing to the C printer, rather than
// OBSOLETE          print an error message.  FIXME! */
// OBSOLETE       c_val_print (type, valaddr, 0, address, stream, format,
// OBSOLETE 		   deref_ref, recurse, pretty);
// OBSOLETE     }
// OBSOLETE   gdb_flush (stream);
// OBSOLETE   return (0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Mutually recursive subroutines of cplus_print_value and c_val_print to
// OBSOLETE    print out a structure's fields: cp_print_value_fields and cplus_print_value.
// OBSOLETE 
// OBSOLETE    TYPE, VALADDR, STREAM, RECURSE, and PRETTY have the
// OBSOLETE    same meanings as in cplus_print_value and c_val_print.
// OBSOLETE 
// OBSOLETE    DONT_PRINT is an array of baseclass types that we
// OBSOLETE    should not print, or zero if called from top level.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE chill_print_value_fields (struct type *type, char *valaddr,
// OBSOLETE 			  struct ui_file *stream, int format, int recurse,
// OBSOLETE 			  enum val_prettyprint pretty, struct type **dont_print)
// OBSOLETE {
// OBSOLETE   int i, len;
// OBSOLETE   int fields_seen = 0;
// OBSOLETE 
// OBSOLETE   CHECK_TYPEDEF (type);
// OBSOLETE 
// OBSOLETE   fprintf_filtered (stream, "[");
// OBSOLETE   len = TYPE_NFIELDS (type);
// OBSOLETE   if (len == 0)
// OBSOLETE     {
// OBSOLETE       fprintf_filtered (stream, "<No data fields>");
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       for (i = 0; i < len; i++)
// OBSOLETE 	{
// OBSOLETE 	  if (fields_seen)
// OBSOLETE 	    {
// OBSOLETE 	      fprintf_filtered (stream, ", ");
// OBSOLETE 	    }
// OBSOLETE 	  fields_seen = 1;
// OBSOLETE 	  if (pretty)
// OBSOLETE 	    {
// OBSOLETE 	      fprintf_filtered (stream, "\n");
// OBSOLETE 	      print_spaces_filtered (2 + 2 * recurse, stream);
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    {
// OBSOLETE 	      wrap_here (n_spaces (2 + 2 * recurse));
// OBSOLETE 	    }
// OBSOLETE 	  fputs_filtered (".", stream);
// OBSOLETE 	  fprintf_symbol_filtered (stream, TYPE_FIELD_NAME (type, i),
// OBSOLETE 				   language_chill, DMGL_NO_OPTS);
// OBSOLETE 	  fputs_filtered (": ", stream);
// OBSOLETE 	  if (TYPE_FIELD_PACKED (type, i))
// OBSOLETE 	    {
// OBSOLETE 	      struct value *v;
// OBSOLETE 
// OBSOLETE 	      /* Bitfields require special handling, especially due to byte
// OBSOLETE 	         order problems.  */
// OBSOLETE 	      v = value_from_longest (TYPE_FIELD_TYPE (type, i),
// OBSOLETE 				   unpack_field_as_long (type, valaddr, i));
// OBSOLETE 
// OBSOLETE 	      chill_val_print (TYPE_FIELD_TYPE (type, i), VALUE_CONTENTS (v), 0, 0,
// OBSOLETE 			       stream, format, 0, recurse + 1, pretty);
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    {
// OBSOLETE 	      chill_val_print (TYPE_FIELD_TYPE (type, i),
// OBSOLETE 			       valaddr + TYPE_FIELD_BITPOS (type, i) / 8, 0,
// OBSOLETE 			       0, stream, format, 0, recurse + 1, pretty);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       if (pretty)
// OBSOLETE 	{
// OBSOLETE 	  fprintf_filtered (stream, "\n");
// OBSOLETE 	  print_spaces_filtered (2 * recurse, stream);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   fprintf_filtered (stream, "]");
// OBSOLETE }
// OBSOLETE 
// OBSOLETE int
// OBSOLETE chill_value_print (struct value *val, struct ui_file *stream, int format,
// OBSOLETE 		   enum val_prettyprint pretty)
// OBSOLETE {
// OBSOLETE   struct type *type = VALUE_TYPE (val);
// OBSOLETE   struct type *real_type = check_typedef (type);
// OBSOLETE 
// OBSOLETE   /* If it is a pointer, indicate what it points to.
// OBSOLETE 
// OBSOLETE      Print type also if it is a reference. */
// OBSOLETE 
// OBSOLETE   if (TYPE_CODE (real_type) == TYPE_CODE_PTR ||
// OBSOLETE       TYPE_CODE (real_type) == TYPE_CODE_REF)
// OBSOLETE     {
// OBSOLETE       char *valaddr = VALUE_CONTENTS (val);
// OBSOLETE       CORE_ADDR addr = unpack_pointer (type, valaddr);
// OBSOLETE       if (TYPE_CODE (type) != TYPE_CODE_PTR || addr != 0)
// OBSOLETE 	{
// OBSOLETE 	  int i;
// OBSOLETE 	  char *name = TYPE_NAME (type);
// OBSOLETE 	  if (name)
// OBSOLETE 	    fputs_filtered (name, stream);
// OBSOLETE 	  else if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_VOID)
// OBSOLETE 	    fputs_filtered ("PTR", stream);
// OBSOLETE 	  else
// OBSOLETE 	    {
// OBSOLETE 	      fprintf_filtered (stream, "(");
// OBSOLETE 	      type_print (type, "", stream, -1);
// OBSOLETE 	      fprintf_filtered (stream, ")");
// OBSOLETE 	    }
// OBSOLETE 	  fprintf_filtered (stream, "(");
// OBSOLETE 	  i = val_print (type, valaddr, 0, VALUE_ADDRESS (val),
// OBSOLETE 			 stream, format, 1, 0, pretty);
// OBSOLETE 	  fprintf_filtered (stream, ")");
// OBSOLETE 	  return i;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   return (val_print (type, VALUE_CONTENTS (val), 0,
// OBSOLETE 		     VALUE_ADDRESS (val), stream, format, 1, 0, pretty));
// OBSOLETE }
