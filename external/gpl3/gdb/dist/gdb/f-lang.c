/* Fortran language support routines for GDB, the GNU debugger.

   Copyright (C) 1993-2020 Free Software Foundation, Inc.

   Contributed by Motorola.  Adapted from the C parser by Farooq Butt
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
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "parser-defs.h"
#include "language.h"
#include "varobj.h"
#include "gdbcore.h"
#include "f-lang.h"
#include "valprint.h"
#include "value.h"
#include "cp-support.h"
#include "charset.h"
#include "c-lang.h"
#include "target-float.h"
#include "gdbarch.h"

#include <math.h>

/* Local functions */

/* Return the encoding that should be used for the character type
   TYPE.  */

static const char *
f_get_encoding (struct type *type)
{
  const char *encoding;

  switch (TYPE_LENGTH (type))
    {
    case 1:
      encoding = target_charset (get_type_arch (type));
      break;
    case 4:
      if (type_byte_order (type) == BFD_ENDIAN_BIG)
	encoding = "UTF-32BE";
      else
	encoding = "UTF-32LE";
      break;

    default:
      error (_("unrecognized character type"));
    }

  return encoding;
}



/* Table of operators and their precedences for printing expressions.  */

static const struct op_print f_op_print_tab[] =
{
  {"+", BINOP_ADD, PREC_ADD, 0},
  {"+", UNOP_PLUS, PREC_PREFIX, 0},
  {"-", BINOP_SUB, PREC_ADD, 0},
  {"-", UNOP_NEG, PREC_PREFIX, 0},
  {"*", BINOP_MUL, PREC_MUL, 0},
  {"/", BINOP_DIV, PREC_MUL, 0},
  {"DIV", BINOP_INTDIV, PREC_MUL, 0},
  {"MOD", BINOP_REM, PREC_MUL, 0},
  {"=", BINOP_ASSIGN, PREC_ASSIGN, 1},
  {".OR.", BINOP_LOGICAL_OR, PREC_LOGICAL_OR, 0},
  {".AND.", BINOP_LOGICAL_AND, PREC_LOGICAL_AND, 0},
  {".NOT.", UNOP_LOGICAL_NOT, PREC_PREFIX, 0},
  {".EQ.", BINOP_EQUAL, PREC_EQUAL, 0},
  {".NE.", BINOP_NOTEQUAL, PREC_EQUAL, 0},
  {".LE.", BINOP_LEQ, PREC_ORDER, 0},
  {".GE.", BINOP_GEQ, PREC_ORDER, 0},
  {".GT.", BINOP_GTR, PREC_ORDER, 0},
  {".LT.", BINOP_LESS, PREC_ORDER, 0},
  {"**", UNOP_IND, PREC_PREFIX, 0},
  {"@", BINOP_REPEAT, PREC_REPEAT, 0},
  {NULL, OP_NULL, PREC_REPEAT, 0}
};

enum f_primitive_types {
  f_primitive_type_character,
  f_primitive_type_logical,
  f_primitive_type_logical_s1,
  f_primitive_type_logical_s2,
  f_primitive_type_logical_s8,
  f_primitive_type_integer,
  f_primitive_type_integer_s2,
  f_primitive_type_real,
  f_primitive_type_real_s8,
  f_primitive_type_real_s16,
  f_primitive_type_complex_s8,
  f_primitive_type_complex_s16,
  f_primitive_type_void,
  nr_f_primitive_types
};

/* Special expression evaluation cases for Fortran.  */

static struct value *
evaluate_subexp_f (struct type *expect_type, struct expression *exp,
		   int *pos, enum noside noside)
{
  struct value *arg1 = NULL, *arg2 = NULL;
  enum exp_opcode op;
  int pc;
  struct type *type;

  pc = *pos;
  *pos += 1;
  op = exp->elts[pc].opcode;

  switch (op)
    {
    default:
      *pos -= 1;
      return evaluate_subexp_standard (expect_type, exp, pos, noside);

    case UNOP_ABS:
      arg1 = evaluate_subexp (nullptr, exp, pos, noside);
      if (noside == EVAL_SKIP)
	return eval_skip_value (exp);
      type = value_type (arg1);
      switch (type->code ())
	{
	case TYPE_CODE_FLT:
	  {
	    double d
	      = fabs (target_float_to_host_double (value_contents (arg1),
						   value_type (arg1)));
	    return value_from_host_double (type, d);
	  }
	case TYPE_CODE_INT:
	  {
	    LONGEST l = value_as_long (arg1);
	    l = llabs (l);
	    return value_from_longest (type, l);
	  }
	}
      error (_("ABS of type %s not supported"), TYPE_SAFE_NAME (type));

    case BINOP_MOD:
      arg1 = evaluate_subexp (nullptr, exp, pos, noside);
      arg2 = evaluate_subexp (value_type (arg1), exp, pos, noside);
      if (noside == EVAL_SKIP)
	return eval_skip_value (exp);
      type = value_type (arg1);
      if (type->code () != value_type (arg2)->code ())
	error (_("non-matching types for parameters to MOD ()"));
      switch (type->code ())
	{
	case TYPE_CODE_FLT:
	  {
	    double d1
	      = target_float_to_host_double (value_contents (arg1),
					     value_type (arg1));
	    double d2
	      = target_float_to_host_double (value_contents (arg2),
					     value_type (arg2));
	    double d3 = fmod (d1, d2);
	    return value_from_host_double (type, d3);
	  }
	case TYPE_CODE_INT:
	  {
	    LONGEST v1 = value_as_long (arg1);
	    LONGEST v2 = value_as_long (arg2);
	    if (v2 == 0)
	      error (_("calling MOD (N, 0) is undefined"));
	    LONGEST v3 = v1 - (v1 / v2) * v2;
	    return value_from_longest (value_type (arg1), v3);
	  }
	}
      error (_("MOD of type %s not supported"), TYPE_SAFE_NAME (type));

    case UNOP_FORTRAN_CEILING:
      {
	arg1 = evaluate_subexp (nullptr, exp, pos, noside);
	if (noside == EVAL_SKIP)
	  return eval_skip_value (exp);
	type = value_type (arg1);
	if (type->code () != TYPE_CODE_FLT)
	  error (_("argument to CEILING must be of type float"));
	double val
	  = target_float_to_host_double (value_contents (arg1),
					 value_type (arg1));
	val = ceil (val);
	return value_from_host_double (type, val);
      }

    case UNOP_FORTRAN_FLOOR:
      {
	arg1 = evaluate_subexp (nullptr, exp, pos, noside);
	if (noside == EVAL_SKIP)
	  return eval_skip_value (exp);
	type = value_type (arg1);
	if (type->code () != TYPE_CODE_FLT)
	  error (_("argument to FLOOR must be of type float"));
	double val
	  = target_float_to_host_double (value_contents (arg1),
					 value_type (arg1));
	val = floor (val);
	return value_from_host_double (type, val);
      }

    case BINOP_FORTRAN_MODULO:
      {
	arg1 = evaluate_subexp (nullptr, exp, pos, noside);
	arg2 = evaluate_subexp (value_type (arg1), exp, pos, noside);
	if (noside == EVAL_SKIP)
	  return eval_skip_value (exp);
	type = value_type (arg1);
	if (type->code () != value_type (arg2)->code ())
	  error (_("non-matching types for parameters to MODULO ()"));
        /* MODULO(A, P) = A - FLOOR (A / P) * P */
	switch (type->code ())
	  {
	  case TYPE_CODE_INT:
	    {
	      LONGEST a = value_as_long (arg1);
	      LONGEST p = value_as_long (arg2);
	      LONGEST result = a - (a / p) * p;
	      if (result != 0 && (a < 0) != (p < 0))
		result += p;
	      return value_from_longest (value_type (arg1), result);
	    }
	  case TYPE_CODE_FLT:
	    {
	      double a
		= target_float_to_host_double (value_contents (arg1),
					       value_type (arg1));
	      double p
		= target_float_to_host_double (value_contents (arg2),
					       value_type (arg2));
	      double result = fmod (a, p);
	      if (result != 0 && (a < 0.0) != (p < 0.0))
		result += p;
	      return value_from_host_double (type, result);
	    }
	  }
	error (_("MODULO of type %s not supported"), TYPE_SAFE_NAME (type));
      }

    case BINOP_FORTRAN_CMPLX:
      arg1 = evaluate_subexp (nullptr, exp, pos, noside);
      arg2 = evaluate_subexp (value_type (arg1), exp, pos, noside);
      if (noside == EVAL_SKIP)
	return eval_skip_value (exp);
      type = builtin_f_type(exp->gdbarch)->builtin_complex_s16;
      return value_literal_complex (arg1, arg2, type);

    case UNOP_FORTRAN_KIND:
      arg1 = evaluate_subexp (NULL, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
      type = value_type (arg1);

      switch (type->code ())
        {
          case TYPE_CODE_STRUCT:
          case TYPE_CODE_UNION:
          case TYPE_CODE_MODULE:
          case TYPE_CODE_FUNC:
            error (_("argument to kind must be an intrinsic type"));
        }

      if (!TYPE_TARGET_TYPE (type))
        return value_from_longest (builtin_type (exp->gdbarch)->builtin_int,
				   TYPE_LENGTH (type));
      return value_from_longest (builtin_type (exp->gdbarch)->builtin_int,
				 TYPE_LENGTH (TYPE_TARGET_TYPE (type)));
    }

  /* Should be unreachable.  */
  return nullptr;
}

/* Special expression lengths for Fortran.  */

static void
operator_length_f (const struct expression *exp, int pc, int *oplenp,
		   int *argsp)
{
  int oplen = 1;
  int args = 0;

  switch (exp->elts[pc - 1].opcode)
    {
    default:
      operator_length_standard (exp, pc, oplenp, argsp);
      return;

    case UNOP_FORTRAN_KIND:
    case UNOP_FORTRAN_FLOOR:
    case UNOP_FORTRAN_CEILING:
      oplen = 1;
      args = 1;
      break;

    case BINOP_FORTRAN_CMPLX:
    case BINOP_FORTRAN_MODULO:
      oplen = 1;
      args = 2;
      break;
    }

  *oplenp = oplen;
  *argsp = args;
}

/* Helper for PRINT_SUBEXP_F.  Arguments are as for PRINT_SUBEXP_F, except
   the extra argument NAME which is the text that should be printed as the
   name of this operation.  */

static void
print_unop_subexp_f (struct expression *exp, int *pos,
		     struct ui_file *stream, enum precedence prec,
		     const char *name)
{
  (*pos)++;
  fprintf_filtered (stream, "%s(", name);
  print_subexp (exp, pos, stream, PREC_SUFFIX);
  fputs_filtered (")", stream);
}

/* Helper for PRINT_SUBEXP_F.  Arguments are as for PRINT_SUBEXP_F, except
   the extra argument NAME which is the text that should be printed as the
   name of this operation.  */

static void
print_binop_subexp_f (struct expression *exp, int *pos,
		      struct ui_file *stream, enum precedence prec,
		      const char *name)
{
  (*pos)++;
  fprintf_filtered (stream, "%s(", name);
  print_subexp (exp, pos, stream, PREC_SUFFIX);
  fputs_filtered (",", stream);
  print_subexp (exp, pos, stream, PREC_SUFFIX);
  fputs_filtered (")", stream);
}

/* Special expression printing for Fortran.  */

static void
print_subexp_f (struct expression *exp, int *pos,
		struct ui_file *stream, enum precedence prec)
{
  int pc = *pos;
  enum exp_opcode op = exp->elts[pc].opcode;

  switch (op)
    {
    default:
      print_subexp_standard (exp, pos, stream, prec);
      return;

    case UNOP_FORTRAN_KIND:
      print_unop_subexp_f (exp, pos, stream, prec, "KIND");
      return;

    case UNOP_FORTRAN_FLOOR:
      print_unop_subexp_f (exp, pos, stream, prec, "FLOOR");
      return;

    case UNOP_FORTRAN_CEILING:
      print_unop_subexp_f (exp, pos, stream, prec, "CEILING");
      return;

    case BINOP_FORTRAN_CMPLX:
      print_binop_subexp_f (exp, pos, stream, prec, "CMPLX");
      return;

    case BINOP_FORTRAN_MODULO:
      print_binop_subexp_f (exp, pos, stream, prec, "MODULO");
      return;
    }
}

/* Special expression names for Fortran.  */

static const char *
op_name_f (enum exp_opcode opcode)
{
  switch (opcode)
    {
    default:
      return op_name_standard (opcode);

#define OP(name)	\
    case name:		\
      return #name ;
#include "fortran-operator.def"
#undef OP
    }
}

/* Special expression dumping for Fortran.  */

static int
dump_subexp_body_f (struct expression *exp,
		    struct ui_file *stream, int elt)
{
  int opcode = exp->elts[elt].opcode;
  int oplen, nargs, i;

  switch (opcode)
    {
    default:
      return dump_subexp_body_standard (exp, stream, elt);

    case UNOP_FORTRAN_KIND:
    case UNOP_FORTRAN_FLOOR:
    case UNOP_FORTRAN_CEILING:
    case BINOP_FORTRAN_CMPLX:
    case BINOP_FORTRAN_MODULO:
      operator_length_f (exp, (elt + 1), &oplen, &nargs);
      break;
    }

  elt += oplen;
  for (i = 0; i < nargs; i += 1)
    elt = dump_subexp (exp, stream, elt);

  return elt;
}

/* Special expression checking for Fortran.  */

static int
operator_check_f (struct expression *exp, int pos,
		  int (*objfile_func) (struct objfile *objfile,
				       void *data),
		  void *data)
{
  const union exp_element *const elts = exp->elts;

  switch (elts[pos].opcode)
    {
    case UNOP_FORTRAN_KIND:
    case UNOP_FORTRAN_FLOOR:
    case UNOP_FORTRAN_CEILING:
    case BINOP_FORTRAN_CMPLX:
    case BINOP_FORTRAN_MODULO:
      /* Any references to objfiles are held in the arguments to this
	 expression, not within the expression itself, so no additional
	 checking is required here, the outer expression iteration code
	 will take care of checking each argument.  */
      break;

    default:
      return operator_check_standard (exp, pos, objfile_func, data);
    }

  return 0;
}

static const char *f_extensions[] =
{
  ".f", ".F", ".for", ".FOR", ".ftn", ".FTN", ".fpp", ".FPP",
  ".f90", ".F90", ".f95", ".F95", ".f03", ".F03", ".f08", ".F08",
  NULL
};

/* Expression processing for Fortran.  */
static const struct exp_descriptor exp_descriptor_f =
{
  print_subexp_f,
  operator_length_f,
  operator_check_f,
  op_name_f,
  dump_subexp_body_f,
  evaluate_subexp_f
};

/* Constant data that describes the Fortran language.  */

extern const struct language_data f_language_data =
{
  "fortran",
  "Fortran",
  language_fortran,
  range_check_on,
  case_sensitive_off,
  array_column_major,
  macro_expansion_no,
  f_extensions,
  &exp_descriptor_f,
  NULL,                    	/* name_of_this */
  false,			/* la_store_sym_names_in_linkage_form_p */
  f_op_print_tab,		/* expression operators for printing */
  0,				/* arrays are first-class (not c-style) */
  1,				/* String lower bound */
  &default_varobj_ops,
  "(...)"			/* la_struct_too_deep_ellipsis */
};

/* Class representing the Fortran language.  */

class f_language : public language_defn
{
public:
  f_language ()
    : language_defn (language_fortran, f_language_data)
  { /* Nothing.  */ }

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    const struct builtin_f_type *builtin = builtin_f_type (gdbarch);

    lai->string_char_type = builtin->builtin_character;
    lai->primitive_type_vector
      = GDBARCH_OBSTACK_CALLOC (gdbarch, nr_f_primitive_types + 1,
				struct type *);

    lai->primitive_type_vector [f_primitive_type_character]
      = builtin->builtin_character;
    lai->primitive_type_vector [f_primitive_type_logical]
      = builtin->builtin_logical;
    lai->primitive_type_vector [f_primitive_type_logical_s1]
      = builtin->builtin_logical_s1;
    lai->primitive_type_vector [f_primitive_type_logical_s2]
      = builtin->builtin_logical_s2;
    lai->primitive_type_vector [f_primitive_type_logical_s8]
      = builtin->builtin_logical_s8;
    lai->primitive_type_vector [f_primitive_type_real]
      = builtin->builtin_real;
    lai->primitive_type_vector [f_primitive_type_real_s8]
      = builtin->builtin_real_s8;
    lai->primitive_type_vector [f_primitive_type_real_s16]
      = builtin->builtin_real_s16;
    lai->primitive_type_vector [f_primitive_type_complex_s8]
      = builtin->builtin_complex_s8;
    lai->primitive_type_vector [f_primitive_type_complex_s16]
      = builtin->builtin_complex_s16;
    lai->primitive_type_vector [f_primitive_type_void]
      = builtin->builtin_void;

    lai->bool_type_symbol = "logical";
    lai->bool_type_default = builtin->builtin_logical_s2;
  }

  /* See language.h.  */
  unsigned int search_name_hash (const char *name) const override
  {
    return cp_search_name_hash (name);
  }

  /* See language.h.  */

  char *demangle (const char *mangled, int options) const override
  {
      /* We could support demangling here to provide module namespaces
	 also for inferiors with only minimal symbol table (ELF symbols).
	 Just the mangling standard is not standardized across compilers
	 and there is no DW_AT_producer available for inferiors with only
	 the ELF symbols to check the mangling kind.  */
    return nullptr;
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override
  {
    f_print_type (type, varstring, stream, show, level, flags);
  }

  /* See language.h.  This just returns default set of word break
     characters but with the modules separator `::' removed.  */

  const char *word_break_characters (void) const override
  {
    static char *retval;

    if (!retval)
      {
	char *s;

	retval = xstrdup (language_defn::word_break_characters ());
	s = strchr (retval, ':');
	if (s)
	  {
	    char *last_char = &s[strlen (s) - 1];

	    *s = *last_char;
	    *last_char = 0;
	  }
      }
    return retval;
  }


  /* See language.h.  */

  void collect_symbol_completion_matches (completion_tracker &tracker,
					  complete_symbol_mode mode,
					  symbol_name_match_type name_match_type,
					  const char *text, const char *word,
					  enum type_code code) const override
  {
    /* Consider the modules separator :: as a valid symbol name character
       class.  */
    default_collect_symbol_completion_matches_break_on (tracker, mode,
							name_match_type,
							text, word, ":",
							code);
  }

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override
  {
    return f_value_print_inner (val, stream, recurse, options);
  }

  /* See language.h.  */

  struct block_symbol lookup_symbol_nonlocal
	(const char *name, const struct block *block,
	 const domain_enum domain) const override
  {
    return cp_lookup_symbol_nonlocal (this, name, block, domain);
  }

  /* See language.h.  */

  int parser (struct parser_state *ps) const override
  {
    return f_parse (ps);
  }

  /* See language.h.  */

  void emitchar (int ch, struct type *chtype,
		 struct ui_file *stream, int quoter) const override
  {
    const char *encoding = f_get_encoding (chtype);
    generic_emit_char (ch, chtype, stream, quoter, encoding);
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
    const char *type_encoding = f_get_encoding (elttype);

    if (TYPE_LENGTH (elttype) == 4)
      fputs_filtered ("4_", stream);

    if (!encoding || !*encoding)
      encoding = type_encoding;

    generic_printstr (stream, elttype, string, length, encoding,
		      force_ellipses, '\'', 0, options);
  }

  /* See language.h.  */

  void print_typedef (struct type *type, struct symbol *new_symbol,
		      struct ui_file *stream) const override
  {
    f_print_typedef (type, new_symbol, stream);
  }

  /* See language.h.  */

  bool is_string_type_p (struct type *type) const override
  {
    type = check_typedef (type);
    return (type->code () == TYPE_CODE_STRING
	    || (type->code () == TYPE_CODE_ARRAY
		&& TYPE_TARGET_TYPE (type)->code () == TYPE_CODE_CHAR));
  }

protected:

  /* See language.h.  */

  symbol_name_matcher_ftype *get_symbol_name_matcher_inner
	(const lookup_name_info &lookup_name) const override
  {
    return cp_get_symbol_name_matcher (lookup_name);
  }
};

/* Single instance of the Fortran language class.  */

static f_language f_language_defn;

static void *
build_fortran_types (struct gdbarch *gdbarch)
{
  struct builtin_f_type *builtin_f_type
    = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct builtin_f_type);

  builtin_f_type->builtin_void
    = arch_type (gdbarch, TYPE_CODE_VOID, TARGET_CHAR_BIT, "void");

  builtin_f_type->builtin_character
    = arch_type (gdbarch, TYPE_CODE_CHAR, TARGET_CHAR_BIT, "character");

  builtin_f_type->builtin_logical_s1
    = arch_boolean_type (gdbarch, TARGET_CHAR_BIT, 1, "logical*1");

  builtin_f_type->builtin_integer_s2
    = arch_integer_type (gdbarch, gdbarch_short_bit (gdbarch), 0,
			 "integer*2");

  builtin_f_type->builtin_integer_s8
    = arch_integer_type (gdbarch, gdbarch_long_long_bit (gdbarch), 0,
			 "integer*8");

  builtin_f_type->builtin_logical_s2
    = arch_boolean_type (gdbarch, gdbarch_short_bit (gdbarch), 1,
			 "logical*2");

  builtin_f_type->builtin_logical_s8
    = arch_boolean_type (gdbarch, gdbarch_long_long_bit (gdbarch), 1,
			 "logical*8");

  builtin_f_type->builtin_integer
    = arch_integer_type (gdbarch, gdbarch_int_bit (gdbarch), 0,
			 "integer");

  builtin_f_type->builtin_logical
    = arch_boolean_type (gdbarch, gdbarch_int_bit (gdbarch), 1,
			 "logical*4");

  builtin_f_type->builtin_real
    = arch_float_type (gdbarch, gdbarch_float_bit (gdbarch),
		       "real", gdbarch_float_format (gdbarch));
  builtin_f_type->builtin_real_s8
    = arch_float_type (gdbarch, gdbarch_double_bit (gdbarch),
		       "real*8", gdbarch_double_format (gdbarch));
  auto fmt = gdbarch_floatformat_for_type (gdbarch, "real(kind=16)", 128);
  if (fmt != nullptr)
    builtin_f_type->builtin_real_s16
      = arch_float_type (gdbarch, 128, "real*16", fmt);
  else if (gdbarch_long_double_bit (gdbarch) == 128)
    builtin_f_type->builtin_real_s16
      = arch_float_type (gdbarch, gdbarch_long_double_bit (gdbarch),
			 "real*16", gdbarch_long_double_format (gdbarch));
  else
    builtin_f_type->builtin_real_s16
      = arch_type (gdbarch, TYPE_CODE_ERROR, 128, "real*16");

  builtin_f_type->builtin_complex_s8
    = init_complex_type ("complex*8", builtin_f_type->builtin_real);
  builtin_f_type->builtin_complex_s16
    = init_complex_type ("complex*16", builtin_f_type->builtin_real_s8);

  if (builtin_f_type->builtin_real_s16->code () == TYPE_CODE_ERROR)
    builtin_f_type->builtin_complex_s32
      = arch_type (gdbarch, TYPE_CODE_ERROR, 256, "complex*32");
  else
    builtin_f_type->builtin_complex_s32
      = init_complex_type ("complex*32", builtin_f_type->builtin_real_s16);

  return builtin_f_type;
}

static struct gdbarch_data *f_type_data;

const struct builtin_f_type *
builtin_f_type (struct gdbarch *gdbarch)
{
  return (const struct builtin_f_type *) gdbarch_data (gdbarch, f_type_data);
}

void _initialize_f_language ();
void
_initialize_f_language ()
{
  f_type_data = gdbarch_data_register_post_init (build_fortran_types);
}

/* See f-lang.h.  */

struct value *
fortran_argument_convert (struct value *value, bool is_artificial)
{
  if (!is_artificial)
    {
      /* If the value is not in the inferior e.g. registers values,
	 convenience variables and user input.  */
      if (VALUE_LVAL (value) != lval_memory)
	{
	  struct type *type = value_type (value);
	  const int length = TYPE_LENGTH (type);
	  const CORE_ADDR addr
	    = value_as_long (value_allocate_space_in_inferior (length));
	  write_memory (addr, value_contents (value), length);
	  struct value *val
	    = value_from_contents_and_address (type, value_contents (value),
					       addr);
	  return value_addr (val);
	}
      else
	return value_addr (value); /* Program variables, e.g. arrays.  */
    }
    return value;
}

/* See f-lang.h.  */

struct type *
fortran_preserve_arg_pointer (struct value *arg, struct type *type)
{
  if (value_type (arg)->code () == TYPE_CODE_PTR)
    return value_type (arg);
  return type;
}
