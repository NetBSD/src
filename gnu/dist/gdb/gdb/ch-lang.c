// OBSOLETE /* Chill language support routines for GDB, the GNU debugger.
// OBSOLETE    Copyright 1992, 1993, 1994, 1995, 1996, 2000, 2001, 2002
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
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "gdbtypes.h"
// OBSOLETE #include "value.h"
// OBSOLETE #include "expression.h"
// OBSOLETE #include "parser-defs.h"
// OBSOLETE #include "language.h"
// OBSOLETE #include "ch-lang.h"
// OBSOLETE #include "valprint.h"
// OBSOLETE 
// OBSOLETE extern void _initialize_chill_language (void);
// OBSOLETE 
// OBSOLETE static struct value *evaluate_subexp_chill (struct type *, struct expression *,
// OBSOLETE 					    int *, enum noside);
// OBSOLETE 
// OBSOLETE static struct value *value_chill_max_min (enum exp_opcode, struct value *);
// OBSOLETE 
// OBSOLETE static struct value *value_chill_card (struct value *);
// OBSOLETE 
// OBSOLETE static struct value *value_chill_length (struct value *);
// OBSOLETE 
// OBSOLETE static struct type *chill_create_fundamental_type (struct objfile *, int);
// OBSOLETE 
// OBSOLETE static void chill_printstr (struct ui_file * stream, char *string,
// OBSOLETE 			    unsigned int length, int width,
// OBSOLETE 			    int force_ellipses);
// OBSOLETE 
// OBSOLETE static void chill_printchar (int, struct ui_file *);
// OBSOLETE 
// OBSOLETE /* For now, Chill uses a simple mangling algorithm whereby you simply
// OBSOLETE    discard everything after the occurance of two successive CPLUS_MARKER
// OBSOLETE    characters to derive the demangled form. */
// OBSOLETE 
// OBSOLETE char *
// OBSOLETE chill_demangle (const char *mangled)
// OBSOLETE {
// OBSOLETE   const char *joiner = NULL;
// OBSOLETE   char *demangled;
// OBSOLETE   const char *cp = mangled;
// OBSOLETE 
// OBSOLETE   while (*cp)
// OBSOLETE     {
// OBSOLETE       if (is_cplus_marker (*cp))
// OBSOLETE 	{
// OBSOLETE 	  joiner = cp;
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE       cp++;
// OBSOLETE     }
// OBSOLETE   if (joiner != NULL && *(joiner + 1) == *joiner)
// OBSOLETE     {
// OBSOLETE       demangled = savestring (mangled, joiner - mangled);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       demangled = NULL;
// OBSOLETE     }
// OBSOLETE   return (demangled);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE chill_printchar (register int c, struct ui_file *stream)
// OBSOLETE {
// OBSOLETE   c &= 0xFF;			/* Avoid sign bit follies */
// OBSOLETE 
// OBSOLETE   if (PRINT_LITERAL_FORM (c))
// OBSOLETE     {
// OBSOLETE       if (c == '\'' || c == '^')
// OBSOLETE 	fprintf_filtered (stream, "'%c%c'", c, c);
// OBSOLETE       else
// OBSOLETE 	fprintf_filtered (stream, "'%c'", c);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       fprintf_filtered (stream, "'^(%u)'", (unsigned int) c);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Print the character string STRING, printing at most LENGTH characters.
// OBSOLETE    Printing stops early if the number hits print_max; repeat counts
// OBSOLETE    are printed as appropriate.  Print ellipses at the end if we
// OBSOLETE    had to stop before printing LENGTH characters, or if FORCE_ELLIPSES.
// OBSOLETE    Note that gdb maintains the length of strings without counting the
// OBSOLETE    terminating null byte, while chill strings are typically written with
// OBSOLETE    an explicit null byte.  So we always assume an implied null byte
// OBSOLETE    until gdb is able to maintain non-null terminated strings as well
// OBSOLETE    as null terminated strings (FIXME).
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE chill_printstr (struct ui_file *stream, char *string, unsigned int length,
// OBSOLETE 		int width, int force_ellipses)
// OBSOLETE {
// OBSOLETE   register unsigned int i;
// OBSOLETE   unsigned int things_printed = 0;
// OBSOLETE   int in_literal_form = 0;
// OBSOLETE   int in_control_form = 0;
// OBSOLETE   int need_slashslash = 0;
// OBSOLETE   unsigned int c;
// OBSOLETE 
// OBSOLETE   if (length == 0)
// OBSOLETE     {
// OBSOLETE       fputs_filtered ("\"\"", stream);
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   for (i = 0; i < length && things_printed < print_max; ++i)
// OBSOLETE     {
// OBSOLETE       /* Position of the character we are examining
// OBSOLETE          to see whether it is repeated.  */
// OBSOLETE       unsigned int rep1;
// OBSOLETE       /* Number of repetitions we have detected so far.  */
// OBSOLETE       unsigned int reps;
// OBSOLETE 
// OBSOLETE       QUIT;
// OBSOLETE 
// OBSOLETE       if (need_slashslash)
// OBSOLETE 	{
// OBSOLETE 	  fputs_filtered ("//", stream);
// OBSOLETE 	  need_slashslash = 0;
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       rep1 = i + 1;
// OBSOLETE       reps = 1;
// OBSOLETE       while (rep1 < length && string[rep1] == string[i])
// OBSOLETE 	{
// OBSOLETE 	  ++rep1;
// OBSOLETE 	  ++reps;
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       c = string[i];
// OBSOLETE       if (reps > repeat_count_threshold)
// OBSOLETE 	{
// OBSOLETE 	  if (in_control_form || in_literal_form)
// OBSOLETE 	    {
// OBSOLETE 	      if (in_control_form)
// OBSOLETE 		fputs_filtered (")", stream);
// OBSOLETE 	      fputs_filtered ("\"//", stream);
// OBSOLETE 	      in_control_form = in_literal_form = 0;
// OBSOLETE 	    }
// OBSOLETE 	  chill_printchar (c, stream);
// OBSOLETE 	  fprintf_filtered (stream, "<repeats %u times>", reps);
// OBSOLETE 	  i = rep1 - 1;
// OBSOLETE 	  things_printed += repeat_count_threshold;
// OBSOLETE 	  need_slashslash = 1;
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  if (!in_literal_form && !in_control_form)
// OBSOLETE 	    fputs_filtered ("\"", stream);
// OBSOLETE 	  if (PRINT_LITERAL_FORM (c))
// OBSOLETE 	    {
// OBSOLETE 	      if (!in_literal_form)
// OBSOLETE 		{
// OBSOLETE 		  if (in_control_form)
// OBSOLETE 		    {
// OBSOLETE 		      fputs_filtered (")", stream);
// OBSOLETE 		      in_control_form = 0;
// OBSOLETE 		    }
// OBSOLETE 		  in_literal_form = 1;
// OBSOLETE 		}
// OBSOLETE 	      fprintf_filtered (stream, "%c", c);
// OBSOLETE 	      if (c == '"' || c == '^')
// OBSOLETE 		/* duplicate this one as must be done at input */
// OBSOLETE 		fprintf_filtered (stream, "%c", c);
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    {
// OBSOLETE 	      if (!in_control_form)
// OBSOLETE 		{
// OBSOLETE 		  if (in_literal_form)
// OBSOLETE 		    {
// OBSOLETE 		      in_literal_form = 0;
// OBSOLETE 		    }
// OBSOLETE 		  fputs_filtered ("^(", stream);
// OBSOLETE 		  in_control_form = 1;
// OBSOLETE 		}
// OBSOLETE 	      else
// OBSOLETE 		fprintf_filtered (stream, ",");
// OBSOLETE 	      c = c & 0xff;
// OBSOLETE 	      fprintf_filtered (stream, "%u", (unsigned int) c);
// OBSOLETE 	    }
// OBSOLETE 	  ++things_printed;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Terminate the quotes if necessary.  */
// OBSOLETE   if (in_control_form)
// OBSOLETE     {
// OBSOLETE       fputs_filtered (")", stream);
// OBSOLETE     }
// OBSOLETE   if (in_literal_form || in_control_form)
// OBSOLETE     {
// OBSOLETE       fputs_filtered ("\"", stream);
// OBSOLETE     }
// OBSOLETE   if (force_ellipses || (i < length))
// OBSOLETE     {
// OBSOLETE       fputs_filtered ("...", stream);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct type *
// OBSOLETE chill_create_fundamental_type (struct objfile *objfile, int typeid)
// OBSOLETE {
// OBSOLETE   register struct type *type = NULL;
// OBSOLETE 
// OBSOLETE   switch (typeid)
// OBSOLETE     {
// OBSOLETE     default:
// OBSOLETE       /* FIXME:  For now, if we are asked to produce a type not in this
// OBSOLETE          language, create the equivalent of a C integer type with the
// OBSOLETE          name "<?type?>".  When all the dust settles from the type
// OBSOLETE          reconstruction work, this should probably become an error. */
// OBSOLETE       type = init_type (TYPE_CODE_INT, 2, 0, "<?type?>", objfile);
// OBSOLETE       warning ("internal error: no chill fundamental type %d", typeid);
// OBSOLETE       break;
// OBSOLETE     case FT_VOID:
// OBSOLETE       /* FIXME:  Currently the GNU Chill compiler emits some DWARF entries for
// OBSOLETE          typedefs, unrelated to anything directly in the code being compiled,
// OBSOLETE          that have some FT_VOID types.  Just fake it for now. */
// OBSOLETE       type = init_type (TYPE_CODE_VOID, 0, 0, "<?VOID?>", objfile);
// OBSOLETE       break;
// OBSOLETE     case FT_BOOLEAN:
// OBSOLETE       type = init_type (TYPE_CODE_BOOL, 1, TYPE_FLAG_UNSIGNED, "BOOL", objfile);
// OBSOLETE       break;
// OBSOLETE     case FT_CHAR:
// OBSOLETE       type = init_type (TYPE_CODE_CHAR, 1, TYPE_FLAG_UNSIGNED, "CHAR", objfile);
// OBSOLETE       break;
// OBSOLETE     case FT_SIGNED_CHAR:
// OBSOLETE       type = init_type (TYPE_CODE_INT, 1, 0, "BYTE", objfile);
// OBSOLETE       break;
// OBSOLETE     case FT_UNSIGNED_CHAR:
// OBSOLETE       type = init_type (TYPE_CODE_INT, 1, TYPE_FLAG_UNSIGNED, "UBYTE", objfile);
// OBSOLETE       break;
// OBSOLETE     case FT_SHORT:		/* Chill ints are 2 bytes */
// OBSOLETE       type = init_type (TYPE_CODE_INT, 2, 0, "INT", objfile);
// OBSOLETE       break;
// OBSOLETE     case FT_UNSIGNED_SHORT:	/* Chill ints are 2 bytes */
// OBSOLETE       type = init_type (TYPE_CODE_INT, 2, TYPE_FLAG_UNSIGNED, "UINT", objfile);
// OBSOLETE       break;
// OBSOLETE     case FT_INTEGER:		/* FIXME? */
// OBSOLETE     case FT_SIGNED_INTEGER:	/* FIXME? */
// OBSOLETE     case FT_LONG:		/* Chill longs are 4 bytes */
// OBSOLETE     case FT_SIGNED_LONG:	/* Chill longs are 4 bytes */
// OBSOLETE       type = init_type (TYPE_CODE_INT, 4, 0, "LONG", objfile);
// OBSOLETE       break;
// OBSOLETE     case FT_UNSIGNED_INTEGER:	/* FIXME? */
// OBSOLETE     case FT_UNSIGNED_LONG:	/* Chill longs are 4 bytes */
// OBSOLETE       type = init_type (TYPE_CODE_INT, 4, TYPE_FLAG_UNSIGNED, "ULONG", objfile);
// OBSOLETE       break;
// OBSOLETE     case FT_FLOAT:
// OBSOLETE       type = init_type (TYPE_CODE_FLT, 4, 0, "REAL", objfile);
// OBSOLETE       break;
// OBSOLETE     case FT_DBL_PREC_FLOAT:
// OBSOLETE       type = init_type (TYPE_CODE_FLT, 8, 0, "LONG_REAL", objfile);
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE   return (type);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Table of operators and their precedences for printing expressions.  */
// OBSOLETE 
// OBSOLETE static const struct op_print chill_op_print_tab[] =
// OBSOLETE {
// OBSOLETE   {"AND", BINOP_LOGICAL_AND, PREC_LOGICAL_AND, 0},
// OBSOLETE   {"OR", BINOP_LOGICAL_OR, PREC_LOGICAL_OR, 0},
// OBSOLETE   {"NOT", UNOP_LOGICAL_NOT, PREC_PREFIX, 0},
// OBSOLETE   {"MOD", BINOP_MOD, PREC_MUL, 0},
// OBSOLETE   {"REM", BINOP_REM, PREC_MUL, 0},
// OBSOLETE   {"SIZE", UNOP_SIZEOF, PREC_BUILTIN_FUNCTION, 0},
// OBSOLETE   {"LOWER", UNOP_LOWER, PREC_BUILTIN_FUNCTION, 0},
// OBSOLETE   {"UPPER", UNOP_UPPER, PREC_BUILTIN_FUNCTION, 0},
// OBSOLETE   {"CARD", UNOP_CARD, PREC_BUILTIN_FUNCTION, 0},
// OBSOLETE   {"MAX", UNOP_CHMAX, PREC_BUILTIN_FUNCTION, 0},
// OBSOLETE   {"MIN", UNOP_CHMIN, PREC_BUILTIN_FUNCTION, 0},
// OBSOLETE   {":=", BINOP_ASSIGN, PREC_ASSIGN, 1},
// OBSOLETE   {"=", BINOP_EQUAL, PREC_EQUAL, 0},
// OBSOLETE   {"/=", BINOP_NOTEQUAL, PREC_EQUAL, 0},
// OBSOLETE   {"<=", BINOP_LEQ, PREC_ORDER, 0},
// OBSOLETE   {">=", BINOP_GEQ, PREC_ORDER, 0},
// OBSOLETE   {">", BINOP_GTR, PREC_ORDER, 0},
// OBSOLETE   {"<", BINOP_LESS, PREC_ORDER, 0},
// OBSOLETE   {"+", BINOP_ADD, PREC_ADD, 0},
// OBSOLETE   {"-", BINOP_SUB, PREC_ADD, 0},
// OBSOLETE   {"*", BINOP_MUL, PREC_MUL, 0},
// OBSOLETE   {"/", BINOP_DIV, PREC_MUL, 0},
// OBSOLETE   {"//", BINOP_CONCAT, PREC_PREFIX, 0},		/* FIXME: precedence? */
// OBSOLETE   {"-", UNOP_NEG, PREC_PREFIX, 0},
// OBSOLETE   {"->", UNOP_IND, PREC_SUFFIX, 1},
// OBSOLETE   {"->", UNOP_ADDR, PREC_PREFIX, 0},
// OBSOLETE   {":", BINOP_RANGE, PREC_ASSIGN, 0},
// OBSOLETE   {NULL, 0, 0, 0}
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* The built-in types of Chill.  */
// OBSOLETE 
// OBSOLETE struct type *builtin_type_chill_bool;
// OBSOLETE struct type *builtin_type_chill_char;
// OBSOLETE struct type *builtin_type_chill_long;
// OBSOLETE struct type *builtin_type_chill_ulong;
// OBSOLETE struct type *builtin_type_chill_real;
// OBSOLETE 
// OBSOLETE struct type **const (chill_builtin_types[]) =
// OBSOLETE {
// OBSOLETE   &builtin_type_chill_bool,
// OBSOLETE     &builtin_type_chill_char,
// OBSOLETE     &builtin_type_chill_long,
// OBSOLETE     &builtin_type_chill_ulong,
// OBSOLETE     &builtin_type_chill_real,
// OBSOLETE     0
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* Calculate LOWER or UPPER of TYPE.
// OBSOLETE    Returns the result as an integer.
// OBSOLETE    *RESULT_TYPE is the appropriate type for the result. */
// OBSOLETE 
// OBSOLETE LONGEST
// OBSOLETE type_lower_upper (enum exp_opcode op,	/* Either UNOP_LOWER or UNOP_UPPER */
// OBSOLETE 		  struct type *type, struct type **result_type)
// OBSOLETE {
// OBSOLETE   LONGEST low, high;
// OBSOLETE   *result_type = type;
// OBSOLETE   CHECK_TYPEDEF (type);
// OBSOLETE   switch (TYPE_CODE (type))
// OBSOLETE     {
// OBSOLETE     case TYPE_CODE_STRUCT:
// OBSOLETE       *result_type = builtin_type_int;
// OBSOLETE       if (chill_varying_type (type))
// OBSOLETE 	return type_lower_upper (op, TYPE_FIELD_TYPE (type, 1), result_type);
// OBSOLETE       break;
// OBSOLETE     case TYPE_CODE_ARRAY:
// OBSOLETE     case TYPE_CODE_BITSTRING:
// OBSOLETE     case TYPE_CODE_STRING:
// OBSOLETE       type = TYPE_FIELD_TYPE (type, 0);		/* Get index type */
// OBSOLETE 
// OBSOLETE       /* ... fall through ... */
// OBSOLETE     case TYPE_CODE_RANGE:
// OBSOLETE       *result_type = TYPE_TARGET_TYPE (type);
// OBSOLETE       return op == UNOP_LOWER ? TYPE_LOW_BOUND (type) : TYPE_HIGH_BOUND (type);
// OBSOLETE 
// OBSOLETE     case TYPE_CODE_ENUM:
// OBSOLETE     case TYPE_CODE_BOOL:
// OBSOLETE     case TYPE_CODE_INT:
// OBSOLETE     case TYPE_CODE_CHAR:
// OBSOLETE       if (get_discrete_bounds (type, &low, &high) >= 0)
// OBSOLETE 	{
// OBSOLETE 	  *result_type = type;
// OBSOLETE 	  return op == UNOP_LOWER ? low : high;
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE     case TYPE_CODE_UNDEF:
// OBSOLETE     case TYPE_CODE_PTR:
// OBSOLETE     case TYPE_CODE_UNION:
// OBSOLETE     case TYPE_CODE_FUNC:
// OBSOLETE     case TYPE_CODE_FLT:
// OBSOLETE     case TYPE_CODE_VOID:
// OBSOLETE     case TYPE_CODE_SET:
// OBSOLETE     case TYPE_CODE_ERROR:
// OBSOLETE     case TYPE_CODE_MEMBER:
// OBSOLETE     case TYPE_CODE_METHOD:
// OBSOLETE     case TYPE_CODE_REF:
// OBSOLETE     case TYPE_CODE_COMPLEX:
// OBSOLETE     default:
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE   error ("unknown mode for LOWER/UPPER builtin");
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct value *
// OBSOLETE value_chill_length (struct value *val)
// OBSOLETE {
// OBSOLETE   LONGEST tmp;
// OBSOLETE   struct type *type = VALUE_TYPE (val);
// OBSOLETE   struct type *ttype;
// OBSOLETE   CHECK_TYPEDEF (type);
// OBSOLETE   switch (TYPE_CODE (type))
// OBSOLETE     {
// OBSOLETE     case TYPE_CODE_ARRAY:
// OBSOLETE     case TYPE_CODE_BITSTRING:
// OBSOLETE     case TYPE_CODE_STRING:
// OBSOLETE       tmp = type_lower_upper (UNOP_UPPER, type, &ttype)
// OBSOLETE 	- type_lower_upper (UNOP_LOWER, type, &ttype) + 1;
// OBSOLETE       break;
// OBSOLETE     case TYPE_CODE_STRUCT:
// OBSOLETE       if (chill_varying_type (type))
// OBSOLETE 	{
// OBSOLETE 	  tmp = unpack_long (TYPE_FIELD_TYPE (type, 0), VALUE_CONTENTS (val));
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE       /* ... else fall through ... */
// OBSOLETE     default:
// OBSOLETE       error ("bad argument to LENGTH builtin");
// OBSOLETE     }
// OBSOLETE   return value_from_longest (builtin_type_int, tmp);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct value *
// OBSOLETE value_chill_card (struct value *val)
// OBSOLETE {
// OBSOLETE   LONGEST tmp = 0;
// OBSOLETE   struct type *type = VALUE_TYPE (val);
// OBSOLETE   CHECK_TYPEDEF (type);
// OBSOLETE 
// OBSOLETE   if (TYPE_CODE (type) == TYPE_CODE_SET)
// OBSOLETE     {
// OBSOLETE       struct type *range_type = TYPE_INDEX_TYPE (type);
// OBSOLETE       LONGEST lower_bound, upper_bound;
// OBSOLETE       int i;
// OBSOLETE 
// OBSOLETE       get_discrete_bounds (range_type, &lower_bound, &upper_bound);
// OBSOLETE       for (i = lower_bound; i <= upper_bound; i++)
// OBSOLETE 	if (value_bit_index (type, VALUE_CONTENTS (val), i) > 0)
// OBSOLETE 	  tmp++;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     error ("bad argument to CARD builtin");
// OBSOLETE 
// OBSOLETE   return value_from_longest (builtin_type_int, tmp);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct value *
// OBSOLETE value_chill_max_min (enum exp_opcode op, struct value *val)
// OBSOLETE {
// OBSOLETE   LONGEST tmp = 0;
// OBSOLETE   struct type *type = VALUE_TYPE (val);
// OBSOLETE   struct type *elttype;
// OBSOLETE   CHECK_TYPEDEF (type);
// OBSOLETE 
// OBSOLETE   if (TYPE_CODE (type) == TYPE_CODE_SET)
// OBSOLETE     {
// OBSOLETE       LONGEST lower_bound, upper_bound;
// OBSOLETE       int i, empty = 1;
// OBSOLETE 
// OBSOLETE       elttype = TYPE_INDEX_TYPE (type);
// OBSOLETE       CHECK_TYPEDEF (elttype);
// OBSOLETE       get_discrete_bounds (elttype, &lower_bound, &upper_bound);
// OBSOLETE 
// OBSOLETE       if (op == UNOP_CHMAX)
// OBSOLETE 	{
// OBSOLETE 	  for (i = upper_bound; i >= lower_bound; i--)
// OBSOLETE 	    {
// OBSOLETE 	      if (value_bit_index (type, VALUE_CONTENTS (val), i) > 0)
// OBSOLETE 		{
// OBSOLETE 		  tmp = i;
// OBSOLETE 		  empty = 0;
// OBSOLETE 		  break;
// OBSOLETE 		}
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  for (i = lower_bound; i <= upper_bound; i++)
// OBSOLETE 	    {
// OBSOLETE 	      if (value_bit_index (type, VALUE_CONTENTS (val), i) > 0)
// OBSOLETE 		{
// OBSOLETE 		  tmp = i;
// OBSOLETE 		  empty = 0;
// OBSOLETE 		  break;
// OBSOLETE 		}
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       if (empty)
// OBSOLETE 	error ("%s for empty powerset", op == UNOP_CHMAX ? "MAX" : "MIN");
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     error ("bad argument to %s builtin", op == UNOP_CHMAX ? "MAX" : "MIN");
// OBSOLETE 
// OBSOLETE   return value_from_longest (TYPE_CODE (elttype) == TYPE_CODE_RANGE
// OBSOLETE 			     ? TYPE_TARGET_TYPE (elttype)
// OBSOLETE 			     : elttype,
// OBSOLETE 			     tmp);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct value *
// OBSOLETE evaluate_subexp_chill (struct type *expect_type,
// OBSOLETE 		       register struct expression *exp, register int *pos,
// OBSOLETE 		       enum noside noside)
// OBSOLETE {
// OBSOLETE   int pc = *pos;
// OBSOLETE   struct type *type;
// OBSOLETE   int tem, nargs;
// OBSOLETE   struct value *arg1;
// OBSOLETE   struct value **argvec;
// OBSOLETE   enum exp_opcode op = exp->elts[*pos].opcode;
// OBSOLETE   switch (op)
// OBSOLETE     {
// OBSOLETE     case MULTI_SUBSCRIPT:
// OBSOLETE       if (noside == EVAL_SKIP)
// OBSOLETE 	break;
// OBSOLETE       (*pos) += 3;
// OBSOLETE       nargs = longest_to_int (exp->elts[pc + 1].longconst);
// OBSOLETE       arg1 = evaluate_subexp_with_coercion (exp, pos, noside);
// OBSOLETE       type = check_typedef (VALUE_TYPE (arg1));
// OBSOLETE 
// OBSOLETE       if (nargs == 1 && TYPE_CODE (type) == TYPE_CODE_INT)
// OBSOLETE 	{
// OBSOLETE 	  /* Looks like string repetition. */
// OBSOLETE 	  struct value *string = evaluate_subexp_with_coercion (exp, pos,
// OBSOLETE 								noside);
// OBSOLETE 	  return value_concat (arg1, string);
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       switch (TYPE_CODE (type))
// OBSOLETE 	{
// OBSOLETE 	case TYPE_CODE_PTR:
// OBSOLETE 	  type = check_typedef (TYPE_TARGET_TYPE (type));
// OBSOLETE 	  if (!type || TYPE_CODE (type) != TYPE_CODE_FUNC)
// OBSOLETE 	    error ("reference value used as function");
// OBSOLETE 	  /* ... fall through ... */
// OBSOLETE 	case TYPE_CODE_FUNC:
// OBSOLETE 	  /* It's a function call. */
// OBSOLETE 	  if (noside == EVAL_AVOID_SIDE_EFFECTS)
// OBSOLETE 	    break;
// OBSOLETE 
// OBSOLETE 	  /* Allocate arg vector, including space for the function to be
// OBSOLETE 	     called in argvec[0] and a terminating NULL */
// OBSOLETE 	  argvec = (struct value **) alloca (sizeof (struct value *)
// OBSOLETE 					     * (nargs + 2));
// OBSOLETE 	  argvec[0] = arg1;
// OBSOLETE 	  tem = 1;
// OBSOLETE 	  for (; tem <= nargs && tem <= TYPE_NFIELDS (type); tem++)
// OBSOLETE 	    {
// OBSOLETE 	      argvec[tem]
// OBSOLETE 		= evaluate_subexp_chill (TYPE_FIELD_TYPE (type, tem - 1),
// OBSOLETE 					 exp, pos, noside);
// OBSOLETE 	    }
// OBSOLETE 	  for (; tem <= nargs; tem++)
// OBSOLETE 	    argvec[tem] = evaluate_subexp_with_coercion (exp, pos, noside);
// OBSOLETE 	  argvec[tem] = 0;	/* signal end of arglist */
// OBSOLETE 
// OBSOLETE 	  return call_function_by_hand (argvec[0], nargs, argvec + 1);
// OBSOLETE 	default:
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       while (nargs-- > 0)
// OBSOLETE 	{
// OBSOLETE 	  struct value *index = evaluate_subexp_with_coercion (exp, pos,
// OBSOLETE 							       noside);
// OBSOLETE 	  arg1 = value_subscript (arg1, index);
// OBSOLETE 	}
// OBSOLETE       return (arg1);
// OBSOLETE 
// OBSOLETE     case UNOP_LOWER:
// OBSOLETE     case UNOP_UPPER:
// OBSOLETE       (*pos)++;
// OBSOLETE       if (noside == EVAL_SKIP)
// OBSOLETE 	{
// OBSOLETE 	  (*exp->language_defn->evaluate_exp) (NULL_TYPE, exp, pos, EVAL_SKIP);
// OBSOLETE 	  goto nosideret;
// OBSOLETE 	}
// OBSOLETE       arg1 = (*exp->language_defn->evaluate_exp) (NULL_TYPE, exp, pos,
// OBSOLETE 						  EVAL_AVOID_SIDE_EFFECTS);
// OBSOLETE       tem = type_lower_upper (op, VALUE_TYPE (arg1), &type);
// OBSOLETE       return value_from_longest (type, tem);
// OBSOLETE 
// OBSOLETE     case UNOP_LENGTH:
// OBSOLETE       (*pos)++;
// OBSOLETE       arg1 = (*exp->language_defn->evaluate_exp) (NULL_TYPE, exp, pos, noside);
// OBSOLETE       return value_chill_length (arg1);
// OBSOLETE 
// OBSOLETE     case UNOP_CARD:
// OBSOLETE       (*pos)++;
// OBSOLETE       arg1 = (*exp->language_defn->evaluate_exp) (NULL_TYPE, exp, pos, noside);
// OBSOLETE       return value_chill_card (arg1);
// OBSOLETE 
// OBSOLETE     case UNOP_CHMAX:
// OBSOLETE     case UNOP_CHMIN:
// OBSOLETE       (*pos)++;
// OBSOLETE       arg1 = (*exp->language_defn->evaluate_exp) (NULL_TYPE, exp, pos, noside);
// OBSOLETE       return value_chill_max_min (op, arg1);
// OBSOLETE 
// OBSOLETE     case BINOP_COMMA:
// OBSOLETE       error ("',' operator used in invalid context");
// OBSOLETE 
// OBSOLETE     default:
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return evaluate_subexp_standard (expect_type, exp, pos, noside);
// OBSOLETE nosideret:
// OBSOLETE   return value_from_longest (builtin_type_long, (LONGEST) 1);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE const struct language_defn chill_language_defn =
// OBSOLETE {
// OBSOLETE   "chill",
// OBSOLETE   language_chill,
// OBSOLETE   chill_builtin_types,
// OBSOLETE   range_check_on,
// OBSOLETE   type_check_on,
// OBSOLETE   case_sensitive_on,
// OBSOLETE   chill_parse,			/* parser */
// OBSOLETE   chill_error,			/* parser error function */
// OBSOLETE   evaluate_subexp_chill,
// OBSOLETE   chill_printchar,		/* print a character constant */
// OBSOLETE   chill_printstr,		/* function to print a string constant */
// OBSOLETE   NULL,				/* Function to print a single char */
// OBSOLETE   chill_create_fundamental_type,	/* Create fundamental type in this language */
// OBSOLETE   chill_print_type,		/* Print a type using appropriate syntax */
// OBSOLETE   chill_val_print,		/* Print a value using appropriate syntax */
// OBSOLETE   chill_value_print,		/* Print a top-levl value */
// OBSOLETE   {"", "B'", "", ""},		/* Binary format info */
// OBSOLETE   {"O'%lo", "O'", "o", ""},	/* Octal format info */
// OBSOLETE   {"D'%ld", "D'", "d", ""},	/* Decimal format info */
// OBSOLETE   {"H'%lx", "H'", "x", ""},	/* Hex format info */
// OBSOLETE   chill_op_print_tab,		/* expression operators for printing */
// OBSOLETE   0,				/* arrays are first-class (not c-style) */
// OBSOLETE   0,				/* String lower bound */
// OBSOLETE   &builtin_type_chill_char,	/* Type of string elements */
// OBSOLETE   LANG_MAGIC
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* Initialization for Chill */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_chill_language (void)
// OBSOLETE {
// OBSOLETE   builtin_type_chill_bool =
// OBSOLETE     init_type (TYPE_CODE_BOOL, TARGET_CHAR_BIT / TARGET_CHAR_BIT,
// OBSOLETE 	       TYPE_FLAG_UNSIGNED,
// OBSOLETE 	       "BOOL", (struct objfile *) NULL);
// OBSOLETE   builtin_type_chill_char =
// OBSOLETE     init_type (TYPE_CODE_CHAR, TARGET_CHAR_BIT / TARGET_CHAR_BIT,
// OBSOLETE 	       TYPE_FLAG_UNSIGNED,
// OBSOLETE 	       "CHAR", (struct objfile *) NULL);
// OBSOLETE   builtin_type_chill_long =
// OBSOLETE     init_type (TYPE_CODE_INT, TARGET_LONG_BIT / TARGET_CHAR_BIT,
// OBSOLETE 	       0,
// OBSOLETE 	       "LONG", (struct objfile *) NULL);
// OBSOLETE   builtin_type_chill_ulong =
// OBSOLETE     init_type (TYPE_CODE_INT, TARGET_LONG_BIT / TARGET_CHAR_BIT,
// OBSOLETE 	       TYPE_FLAG_UNSIGNED,
// OBSOLETE 	       "ULONG", (struct objfile *) NULL);
// OBSOLETE   builtin_type_chill_real =
// OBSOLETE     init_type (TYPE_CODE_FLT, TARGET_DOUBLE_BIT / TARGET_CHAR_BIT,
// OBSOLETE 	       0,
// OBSOLETE 	       "LONG_REAL", (struct objfile *) NULL);
// OBSOLETE 
// OBSOLETE   add_language (&chill_language_defn);
// OBSOLETE }
