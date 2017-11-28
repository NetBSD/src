/* Fortran language support routines for GDB, the GNU debugger.

   Copyright (C) 1993-2016 Free Software Foundation, Inc.

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
#include "f-lang.h"
#include "valprint.h"
#include "value.h"
#include "cp-support.h"
#include "charset.h"
#include "c-lang.h"


/* Local functions */

extern void _initialize_f_language (void);

static void f_printchar (int c, struct type *type, struct ui_file * stream);
static void f_emit_char (int c, struct type *type,
			 struct ui_file * stream, int quoter);

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
      if (gdbarch_byte_order (get_type_arch (type)) == BFD_ENDIAN_BIG)
	encoding = "UTF-32BE";
      else
	encoding = "UTF-32LE";
      break;

    default:
      error (_("unrecognized character type"));
    }

  return encoding;
}

/* Print the character C on STREAM as part of the contents of a literal
   string whose delimiter is QUOTER.  Note that that format for printing
   characters and strings is language specific.
   FIXME:  This is a copy of the same function from c-exp.y.  It should
   be replaced with a true F77 version.  */

static void
f_emit_char (int c, struct type *type, struct ui_file *stream, int quoter)
{
  const char *encoding = f_get_encoding (type);

  generic_emit_char (c, type, stream, quoter, encoding);
}

/* Implementation of la_printchar.  */

static void
f_printchar (int c, struct type *type, struct ui_file *stream)
{
  fputs_filtered ("'", stream);
  LA_EMIT_CHAR (c, type, stream, '\'');
  fputs_filtered ("'", stream);
}

/* Print the character string STRING, printing at most LENGTH characters.
   Printing stops early if the number hits print_max; repeat counts
   are printed as appropriate.  Print ellipses at the end if we
   had to stop before printing LENGTH characters, or if FORCE_ELLIPSES.
   FIXME:  This is a copy of the same function from c-exp.y.  It should
   be replaced with a true F77 version.  */

static void
f_printstr (struct ui_file *stream, struct type *type, const gdb_byte *string,
	    unsigned int length, const char *encoding, int force_ellipses,
	    const struct value_print_options *options)
{
  const char *type_encoding = f_get_encoding (type);

  if (TYPE_LENGTH (type) == 4)
    fputs_filtered ("4_", stream);

  if (!encoding || !*encoding)
    encoding = type_encoding;

  generic_printstr (stream, type, string, length, encoding,
		    force_ellipses, '\'', 0, options);
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

static void
f_language_arch_info (struct gdbarch *gdbarch,
		      struct language_arch_info *lai)
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

/* Remove the modules separator :: from the default break list.  */

static char *
f_word_break_characters (void)
{
  static char *retval;

  if (!retval)
    {
      char *s;

      retval = xstrdup (default_word_break_characters ());
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

/* Consider the modules separator :: as a valid symbol name character
   class.  */

static VEC (char_ptr) *
f_make_symbol_completion_list (const char *text, const char *word,
			       enum type_code code)
{
  return default_make_symbol_completion_list_break_on (text, word, ":", code);
}

static const char *f_extensions[] =
{
  ".f", ".F", ".for", ".FOR", ".ftn", ".FTN", ".fpp", ".FPP",
  ".f90", ".F90", ".f95", ".F95", ".f03", ".F03", ".f08", ".F08",
  NULL
};

const struct language_defn f_language_defn =
{
  "fortran",
  "Fortran",
  language_fortran,
  range_check_on,
  case_sensitive_off,
  array_column_major,
  macro_expansion_no,
  f_extensions,
  &exp_descriptor_standard,
  f_parse,			/* parser */
  f_yyerror,			/* parser error function */
  null_post_parser,
  f_printchar,			/* Print character constant */
  f_printstr,			/* function to print string constant */
  f_emit_char,			/* Function to print a single character */
  f_print_type,			/* Print a type using appropriate syntax */
  default_print_typedef,	/* Print a typedef using appropriate syntax */
  f_val_print,			/* Print a value using appropriate syntax */
  c_value_print,		/* FIXME */
  default_read_var_value,	/* la_read_var_value */
  NULL,				/* Language specific skip_trampoline */
  NULL,                    	/* name_of_this */
  cp_lookup_symbol_nonlocal,	/* lookup_symbol_nonlocal */
  basic_lookup_transparent_type,/* lookup_transparent_type */

  /* We could support demangling here to provide module namespaces
     also for inferiors with only minimal symbol table (ELF symbols).
     Just the mangling standard is not standardized across compilers
     and there is no DW_AT_producer available for inferiors with only
     the ELF symbols to check the mangling kind.  */
  NULL,				/* Language specific symbol demangler */
  NULL,
  NULL,				/* Language specific
				   class_name_from_physname */
  f_op_print_tab,		/* expression operators for printing */
  0,				/* arrays are first-class (not c-style) */
  1,				/* String lower bound */
  f_word_break_characters,
  f_make_symbol_completion_list,
  f_language_arch_info,
  default_print_array_index,
  default_pass_by_reference,
  default_get_string,
  NULL,				/* la_get_symbol_name_cmp */
  iterate_over_symbols,
  &default_varobj_ops,
  NULL,
  NULL,
  LANG_MAGIC
};

static void *
build_fortran_types (struct gdbarch *gdbarch)
{
  struct builtin_f_type *builtin_f_type
    = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct builtin_f_type);

  builtin_f_type->builtin_void
    = arch_type (gdbarch, TYPE_CODE_VOID, 1, "VOID");

  builtin_f_type->builtin_character
    = arch_integer_type (gdbarch, TARGET_CHAR_BIT, 0, "character");

  builtin_f_type->builtin_logical_s1
    = arch_boolean_type (gdbarch, TARGET_CHAR_BIT, 1, "logical*1");

  builtin_f_type->builtin_integer_s2
    = arch_integer_type (gdbarch, gdbarch_short_bit (gdbarch), 0,
			 "integer*2");

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
		       "real", NULL);
  builtin_f_type->builtin_real_s8
    = arch_float_type (gdbarch, gdbarch_double_bit (gdbarch),
		       "real*8", NULL);
  builtin_f_type->builtin_real_s16
    = arch_float_type (gdbarch, gdbarch_long_double_bit (gdbarch),
		       "real*16", NULL);

  builtin_f_type->builtin_complex_s8
    = arch_complex_type (gdbarch, "complex*8",
			 builtin_f_type->builtin_real);
  builtin_f_type->builtin_complex_s16
    = arch_complex_type (gdbarch, "complex*16",
			 builtin_f_type->builtin_real_s8);
  builtin_f_type->builtin_complex_s32
    = arch_complex_type (gdbarch, "complex*32",
			 builtin_f_type->builtin_real_s16);

  return builtin_f_type;
}

static struct gdbarch_data *f_type_data;

const struct builtin_f_type *
builtin_f_type (struct gdbarch *gdbarch)
{
  return (const struct builtin_f_type *) gdbarch_data (gdbarch, f_type_data);
}

void
_initialize_f_language (void)
{
  f_type_data = gdbarch_data_register_post_init (build_fortran_types);

  add_language (&f_language_defn);
}
