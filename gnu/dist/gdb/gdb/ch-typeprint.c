// OBSOLETE /* Support for printing Chill types for GDB, the GNU debugger.
// OBSOLETE    Copyright 1986, 1988, 1989, 1991, 1992, 1993, 1994, 1995, 2000
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
// OBSOLETE #include "bfd.h"		/* Binary File Description */
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "gdbtypes.h"
// OBSOLETE #include "expression.h"
// OBSOLETE #include "value.h"
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include "target.h"
// OBSOLETE #include "language.h"
// OBSOLETE #include "ch-lang.h"
// OBSOLETE #include "typeprint.h"
// OBSOLETE 
// OBSOLETE #include "gdb_string.h"
// OBSOLETE #include <errno.h>
// OBSOLETE 
// OBSOLETE static void chill_type_print_base (struct type *, struct ui_file *, int, int);
// OBSOLETE 
// OBSOLETE void
// OBSOLETE chill_print_type (struct type *type, char *varstring, struct ui_file *stream,
// OBSOLETE 		  int show, int level)
// OBSOLETE {
// OBSOLETE   if (varstring != NULL && *varstring != '\0')
// OBSOLETE     {
// OBSOLETE       fputs_filtered (varstring, stream);
// OBSOLETE       fputs_filtered (" ", stream);
// OBSOLETE     }
// OBSOLETE   chill_type_print_base (type, stream, show, level);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Print the name of the type (or the ultimate pointer target,
// OBSOLETE    function value or array element).
// OBSOLETE 
// OBSOLETE    SHOW nonzero means don't print this type as just its name;
// OBSOLETE    show its real definition even if it has a name.
// OBSOLETE    SHOW zero means print just typename or tag if there is one
// OBSOLETE    SHOW negative means abbreviate structure elements.
// OBSOLETE    SHOW is decremented for printing of structure elements.
// OBSOLETE 
// OBSOLETE    LEVEL is the depth to indent by.
// OBSOLETE    We increase it for some recursive calls.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE chill_type_print_base (struct type *type, struct ui_file *stream, int show,
// OBSOLETE 		       int level)
// OBSOLETE {
// OBSOLETE   register int len;
// OBSOLETE   register int i;
// OBSOLETE   struct type *index_type;
// OBSOLETE   struct type *range_type;
// OBSOLETE   LONGEST low_bound;
// OBSOLETE   LONGEST high_bound;
// OBSOLETE 
// OBSOLETE   QUIT;
// OBSOLETE 
// OBSOLETE   wrap_here ("    ");
// OBSOLETE   if (type == NULL)
// OBSOLETE     {
// OBSOLETE       fputs_filtered ("<type unknown>", stream);
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* When SHOW is zero or less, and there is a valid type name, then always
// OBSOLETE      just print the type name directly from the type. */
// OBSOLETE 
// OBSOLETE   if ((show <= 0) && (TYPE_NAME (type) != NULL))
// OBSOLETE     {
// OBSOLETE       fputs_filtered (TYPE_NAME (type), stream);
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (TYPE_CODE (type) != TYPE_CODE_TYPEDEF)
// OBSOLETE     CHECK_TYPEDEF (type);
// OBSOLETE 
// OBSOLETE   switch (TYPE_CODE (type))
// OBSOLETE     {
// OBSOLETE     case TYPE_CODE_TYPEDEF:
// OBSOLETE       chill_type_print_base (TYPE_TARGET_TYPE (type), stream, 0, level);
// OBSOLETE       break;
// OBSOLETE     case TYPE_CODE_PTR:
// OBSOLETE       if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_VOID)
// OBSOLETE 	{
// OBSOLETE 	  fprintf_filtered (stream,
// OBSOLETE 			    TYPE_NAME (type) ? TYPE_NAME (type) : "PTR");
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE       fprintf_filtered (stream, "REF ");
// OBSOLETE       chill_type_print_base (TYPE_TARGET_TYPE (type), stream, 0, level);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_BOOL:
// OBSOLETE       /* FIXME: we should probably just print the TYPE_NAME, in case
// OBSOLETE          anyone ever fixes the compiler to give us the real names
// OBSOLETE          in the presence of the chill equivalent of typedef (assuming
// OBSOLETE          there is one).  */
// OBSOLETE       fprintf_filtered (stream,
// OBSOLETE 			TYPE_NAME (type) ? TYPE_NAME (type) : "BOOL");
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_ARRAY:
// OBSOLETE       fputs_filtered ("ARRAY (", stream);
// OBSOLETE       range_type = TYPE_FIELD_TYPE (type, 0);
// OBSOLETE       if (TYPE_CODE (range_type) != TYPE_CODE_RANGE)
// OBSOLETE 	chill_print_type (range_type, "", stream, 0, level);
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  index_type = TYPE_TARGET_TYPE (range_type);
// OBSOLETE 	  low_bound = TYPE_FIELD_BITPOS (range_type, 0);
// OBSOLETE 	  high_bound = TYPE_FIELD_BITPOS (range_type, 1);
// OBSOLETE 	  print_type_scalar (index_type, low_bound, stream);
// OBSOLETE 	  fputs_filtered (":", stream);
// OBSOLETE 	  print_type_scalar (index_type, high_bound, stream);
// OBSOLETE 	}
// OBSOLETE       fputs_filtered (") ", stream);
// OBSOLETE       chill_print_type (TYPE_TARGET_TYPE (type), "", stream, 0, level);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_BITSTRING:
// OBSOLETE       fprintf_filtered (stream, "BOOLS (%d)",
// OBSOLETE 		      TYPE_FIELD_BITPOS (TYPE_FIELD_TYPE (type, 0), 1) + 1);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_SET:
// OBSOLETE       fputs_filtered ("POWERSET ", stream);
// OBSOLETE       chill_print_type (TYPE_INDEX_TYPE (type), "", stream,
// OBSOLETE 			show - 1, level);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_STRING:
// OBSOLETE       range_type = TYPE_FIELD_TYPE (type, 0);
// OBSOLETE       index_type = TYPE_TARGET_TYPE (range_type);
// OBSOLETE       high_bound = TYPE_FIELD_BITPOS (range_type, 1);
// OBSOLETE       fputs_filtered ("CHARS (", stream);
// OBSOLETE       print_type_scalar (index_type, high_bound + 1, stream);
// OBSOLETE       fputs_filtered (")", stream);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_MEMBER:
// OBSOLETE       fprintf_filtered (stream, "MEMBER ");
// OBSOLETE       chill_type_print_base (TYPE_TARGET_TYPE (type), stream, 0, level);
// OBSOLETE       break;
// OBSOLETE     case TYPE_CODE_REF:
// OBSOLETE       fprintf_filtered (stream, "/*LOC*/ ");
// OBSOLETE       chill_type_print_base (TYPE_TARGET_TYPE (type), stream, show, level);
// OBSOLETE       break;
// OBSOLETE     case TYPE_CODE_FUNC:
// OBSOLETE       fprintf_filtered (stream, "PROC (");
// OBSOLETE       len = TYPE_NFIELDS (type);
// OBSOLETE       for (i = 0; i < len; i++)
// OBSOLETE 	{
// OBSOLETE 	  struct type *param_type = TYPE_FIELD_TYPE (type, i);
// OBSOLETE 	  if (i > 0)
// OBSOLETE 	    {
// OBSOLETE 	      fputs_filtered (", ", stream);
// OBSOLETE 	      wrap_here ("    ");
// OBSOLETE 	    }
// OBSOLETE 	  if (TYPE_CODE (param_type) == TYPE_CODE_REF)
// OBSOLETE 	    {
// OBSOLETE 	      chill_type_print_base (TYPE_TARGET_TYPE (param_type),
// OBSOLETE 				     stream, 0, level);
// OBSOLETE 	      fputs_filtered (" LOC", stream);
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    chill_type_print_base (param_type, stream, show, level);
// OBSOLETE 	}
// OBSOLETE       fprintf_filtered (stream, ")");
// OBSOLETE       if (TYPE_CODE (TYPE_TARGET_TYPE (type)) != TYPE_CODE_VOID)
// OBSOLETE 	{
// OBSOLETE 	  fputs_filtered (" RETURNS (", stream);
// OBSOLETE 	  chill_type_print_base (TYPE_TARGET_TYPE (type), stream, 0, level);
// OBSOLETE 	  fputs_filtered (")", stream);
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_STRUCT:
// OBSOLETE       if (chill_varying_type (type))
// OBSOLETE 	{
// OBSOLETE 	  chill_type_print_base (TYPE_FIELD_TYPE (type, 1),
// OBSOLETE 				 stream, 0, level);
// OBSOLETE 	  fputs_filtered (" VARYING", stream);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  fprintf_filtered (stream, "STRUCT ");
// OBSOLETE 
// OBSOLETE 	  fprintf_filtered (stream, "(\n");
// OBSOLETE 	  if ((TYPE_NFIELDS (type) == 0) && (TYPE_NFN_FIELDS (type) == 0))
// OBSOLETE 	    {
// OBSOLETE 	      if (TYPE_STUB (type))
// OBSOLETE 		{
// OBSOLETE 		  fprintfi_filtered (level + 4, stream, "<incomplete type>\n");
// OBSOLETE 		}
// OBSOLETE 	      else
// OBSOLETE 		{
// OBSOLETE 		  fprintfi_filtered (level + 4, stream, "<no data fields>\n");
// OBSOLETE 		}
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    {
// OBSOLETE 	      len = TYPE_NFIELDS (type);
// OBSOLETE 	      for (i = TYPE_N_BASECLASSES (type); i < len; i++)
// OBSOLETE 		{
// OBSOLETE 		  struct type *field_type = TYPE_FIELD_TYPE (type, i);
// OBSOLETE 		  QUIT;
// OBSOLETE 		  print_spaces_filtered (level + 4, stream);
// OBSOLETE 		  if (TYPE_CODE (field_type) == TYPE_CODE_UNION)
// OBSOLETE 		    {
// OBSOLETE 		      int j;	/* variant number */
// OBSOLETE 		      fputs_filtered ("CASE OF\n", stream);
// OBSOLETE 		      for (j = 0; j < TYPE_NFIELDS (field_type); j++)
// OBSOLETE 			{
// OBSOLETE 			  int k;	/* variant field index */
// OBSOLETE 			  struct type *variant_type
// OBSOLETE 			  = TYPE_FIELD_TYPE (field_type, j);
// OBSOLETE 			  int var_len = TYPE_NFIELDS (variant_type);
// OBSOLETE 			  print_spaces_filtered (level + 4, stream);
// OBSOLETE 			  if (strcmp (TYPE_FIELD_NAME (field_type, j),
// OBSOLETE 				      "else") == 0)
// OBSOLETE 			    fputs_filtered ("ELSE\n", stream);
// OBSOLETE 			  else
// OBSOLETE 			    fputs_filtered (":\n", stream);
// OBSOLETE 			  if (TYPE_CODE (variant_type) != TYPE_CODE_STRUCT)
// OBSOLETE 			    error ("variant record confusion");
// OBSOLETE 			  for (k = 0; k < var_len; k++)
// OBSOLETE 			    {
// OBSOLETE 			      print_spaces_filtered (level + 8, stream);
// OBSOLETE 			      chill_print_type (TYPE_FIELD_TYPE (variant_type, k),
// OBSOLETE 					  TYPE_FIELD_NAME (variant_type, k),
// OBSOLETE 						stream, show - 1, level + 8);
// OBSOLETE 			      if (k < (var_len - 1))
// OBSOLETE 				fputs_filtered (",", stream);
// OBSOLETE 			      fputs_filtered ("\n", stream);
// OBSOLETE 			    }
// OBSOLETE 			}
// OBSOLETE 		      print_spaces_filtered (level + 4, stream);
// OBSOLETE 		      fputs_filtered ("ESAC", stream);
// OBSOLETE 		    }
// OBSOLETE 		  else
// OBSOLETE 		    chill_print_type (field_type,
// OBSOLETE 				      TYPE_FIELD_NAME (type, i),
// OBSOLETE 				      stream, show - 1, level + 4);
// OBSOLETE 		  if (i < (len - 1))
// OBSOLETE 		    {
// OBSOLETE 		      fputs_filtered (",", stream);
// OBSOLETE 		    }
// OBSOLETE 		  fputs_filtered ("\n", stream);
// OBSOLETE 		}
// OBSOLETE 	    }
// OBSOLETE 	  fprintfi_filtered (level, stream, ")");
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_RANGE:
// OBSOLETE       {
// OBSOLETE 	struct type *target = TYPE_TARGET_TYPE (type);
// OBSOLETE 	if (target && TYPE_NAME (target))
// OBSOLETE 	  fputs_filtered (TYPE_NAME (target), stream);
// OBSOLETE 	else
// OBSOLETE 	  fputs_filtered ("RANGE", stream);
// OBSOLETE 	if (target == NULL)
// OBSOLETE 	  target = builtin_type_long;
// OBSOLETE 	fputs_filtered (" (", stream);
// OBSOLETE 	print_type_scalar (target, TYPE_LOW_BOUND (type), stream);
// OBSOLETE 	fputs_filtered (":", stream);
// OBSOLETE 	print_type_scalar (target, TYPE_HIGH_BOUND (type), stream);
// OBSOLETE 	fputs_filtered (")", stream);
// OBSOLETE       }
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_ENUM:
// OBSOLETE       {
// OBSOLETE 	register int lastval = 0;
// OBSOLETE 	fprintf_filtered (stream, "SET (");
// OBSOLETE 	len = TYPE_NFIELDS (type);
// OBSOLETE 	for (i = 0; i < len; i++)
// OBSOLETE 	  {
// OBSOLETE 	    QUIT;
// OBSOLETE 	    if (i)
// OBSOLETE 	      fprintf_filtered (stream, ", ");
// OBSOLETE 	    wrap_here ("    ");
// OBSOLETE 	    fputs_filtered (TYPE_FIELD_NAME (type, i), stream);
// OBSOLETE 	    if (lastval != TYPE_FIELD_BITPOS (type, i))
// OBSOLETE 	      {
// OBSOLETE 		fprintf_filtered (stream, " = %d", TYPE_FIELD_BITPOS (type, i));
// OBSOLETE 		lastval = TYPE_FIELD_BITPOS (type, i);
// OBSOLETE 	      }
// OBSOLETE 	    lastval++;
// OBSOLETE 	  }
// OBSOLETE 	fprintf_filtered (stream, ")");
// OBSOLETE       }
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_VOID:
// OBSOLETE     case TYPE_CODE_UNDEF:
// OBSOLETE     case TYPE_CODE_ERROR:
// OBSOLETE     case TYPE_CODE_UNION:
// OBSOLETE     case TYPE_CODE_METHOD:
// OBSOLETE       error ("missing language support in chill_type_print_base");
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     default:
// OBSOLETE 
// OBSOLETE       /* Handle types not explicitly handled by the other cases,
// OBSOLETE          such as fundamental types.  For these, just print whatever
// OBSOLETE          the type name is, as recorded in the type itself.  If there
// OBSOLETE          is no type name, then complain. */
// OBSOLETE 
// OBSOLETE       if (TYPE_NAME (type) != NULL)
// OBSOLETE 	{
// OBSOLETE 	  fputs_filtered (TYPE_NAME (type), stream);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  error ("Unrecognized type code (%d) in symbol table.",
// OBSOLETE 		 TYPE_CODE (type));
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE }
