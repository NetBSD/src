/* D language support routines for GDB, the GNU debugger.

   Copyright (C) 2005-2013 Free Software Foundation, Inc.

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
#include "language.h"
#include "d-lang.h"
#include "c-lang.h"
#include "gdb_string.h"
#include "parser-defs.h"
#include "gdb_obstack.h"

#include <ctype.h>

/* Extract identifiers from MANGLED_STR and append it to TEMPBUF.
   Return 1 on success or 0 on failure.  */
static int
extract_identifiers (const char *mangled_str, struct obstack *tempbuf)
{
  long i = 0;

  while (isdigit (*mangled_str))
    {
      char *end_ptr;

      i = strtol (mangled_str, &end_ptr, 10);
      mangled_str = end_ptr;
      if (i <= 0 || strlen (mangled_str) < i)
        return 0;
      obstack_grow (tempbuf, mangled_str, i);
      mangled_str += i;
      obstack_grow_str (tempbuf, ".");
    }
  if (*mangled_str == '\0' || i == 0)
    return 0;
  obstack_blank (tempbuf, -1);
  return 1;
}

/* Extract and demangle type from MANGLED_STR and append it to TEMPBUF.
   Return 1 on success or 0 on failure.  */
static int
extract_type_info (const char *mangled_str, struct obstack *tempbuf)
{
  if (*mangled_str == '\0')
    return 0;
  switch (*mangled_str++)
    {
      case 'A': /* dynamic array */
      case 'G': /* static array */
      case 'H': /* associative array */
	if (!extract_type_info (mangled_str, tempbuf))
	  return 0;
	obstack_grow_str (tempbuf, "[]");
	return 1;
      case 'P': /* pointer */
	if (!extract_type_info (mangled_str, tempbuf))
	  return 0;
	obstack_grow_str (tempbuf, "*");
	return 1;
      case 'R': /* reference */
	if (!extract_type_info (mangled_str, tempbuf))
	  return 0;
	obstack_grow_str (tempbuf, "&");
	return 1;
      case 'Z': /* return value */
	return extract_type_info (mangled_str, tempbuf);
      case 'J': /* out */
	obstack_grow_str (tempbuf, "out ");
	return extract_type_info (mangled_str, tempbuf);
      case 'K': /* inout */
	obstack_grow_str (tempbuf, "inout ");
	return extract_type_info (mangled_str, tempbuf);
      case 'E': /* enum */
      case 'T': /* typedef */
      case 'D': /* delegate */
      case 'C': /* class */
      case 'S': /* struct */
	return extract_identifiers (mangled_str, tempbuf);

      /* basic types: */
      case 'n': obstack_grow_str (tempbuf, "none"); return 1;
      case 'v': obstack_grow_str (tempbuf, "void"); return 1;
      case 'g': obstack_grow_str (tempbuf, "byte"); return 1;
      case 'h': obstack_grow_str (tempbuf, "ubyte"); return 1;
      case 's': obstack_grow_str (tempbuf, "short"); return 1;
      case 't': obstack_grow_str (tempbuf, "ushort"); return 1;
      case 'i': obstack_grow_str (tempbuf, "int"); return 1;
      case 'k': obstack_grow_str (tempbuf, "uint"); return 1;
      case 'l': obstack_grow_str (tempbuf, "long"); return 1;
      case 'm': obstack_grow_str (tempbuf, "ulong"); return 1;
      case 'f': obstack_grow_str (tempbuf, "float"); return 1;
      case 'd': obstack_grow_str (tempbuf, "double"); return 1;
      case 'e': obstack_grow_str (tempbuf, "real"); return 1;

      /* imaginary and complex: */
      case 'o': obstack_grow_str (tempbuf, "ifloat"); return 1;
      case 'p': obstack_grow_str (tempbuf, "idouble"); return 1;
      case 'j': obstack_grow_str (tempbuf, "ireal"); return 1;
      case 'q': obstack_grow_str (tempbuf, "cfloat"); return 1;
      case 'r': obstack_grow_str (tempbuf, "cdouble"); return 1;
      case 'c': obstack_grow_str (tempbuf, "creal"); return 1;

      /* other types: */
      case 'b': obstack_grow_str (tempbuf, "bit"); return 1;
      case 'a': obstack_grow_str (tempbuf, "char"); return 1;
      case 'u': obstack_grow_str (tempbuf, "wchar"); return 1;
      case 'w': obstack_grow_str (tempbuf, "dchar"); return 1;

      default:
	obstack_grow_str (tempbuf, "unknown");
	return 1;
    }
}

/* Implements the la_demangle language_defn routine for language D.  */
char *
d_demangle (const char *symbol, int options)
{
  struct obstack tempbuf;
  char *out_str;
  unsigned char is_func = 0;

  if (symbol == NULL)
    return NULL;
  else if (strcmp (symbol, "_Dmain") == 0)
    return xstrdup ("D main");

  obstack_init (&tempbuf);
  
  if (symbol[0] == '_' && symbol[1] == 'D')
    {
      symbol += 2;
      is_func = 1;
    }
  else if (strncmp (symbol, "__Class_", 8) == 0)
    symbol += 8;
  else if (strncmp (symbol, "__init_", 7) == 0)
    symbol += 7;
  else if (strncmp (symbol, "__vtbl_", 7) == 0)
    symbol += 7;
  else if (strncmp (symbol, "__modctor_", 10) == 0)
    symbol += 10;
  else if (strncmp (symbol, "__moddtor_", 10) == 0)
    symbol += 10;
  else if (strncmp (symbol, "__ModuleInfo_", 13) == 0)
    symbol += 13;
  else
    {
      obstack_free (&tempbuf, NULL);
      return NULL;
    }
  
  if (!extract_identifiers (symbol, &tempbuf))
    {
      obstack_free (&tempbuf, NULL);
      return NULL;
    }

  obstack_grow_str (&tempbuf, "(");
  if (is_func == 1 && *symbol == 'F')
    {
      symbol++;
      while (*symbol != '\0' && *symbol != 'Z')
	{
	  if (is_func == 1)
	    is_func++;
	  else
	    obstack_grow_str (&tempbuf, ", ");
	  if (!extract_type_info (symbol, &tempbuf))
	    {
	      obstack_free (&tempbuf, NULL);
	      return NULL;
	   }
	}
     }
  obstack_grow_str0 (&tempbuf, ")");

  /* Doesn't display the return type, but wouldn't be too hard to do.  */

  out_str = xstrdup (obstack_finish (&tempbuf));
  obstack_free (&tempbuf, NULL);
  return out_str;
}

/* Table mapping opcodes into strings for printing operators
   and precedences of the operators.  */
static const struct op_print d_op_print_tab[] =
{
  {",", BINOP_COMMA, PREC_COMMA, 0},
  {"=", BINOP_ASSIGN, PREC_ASSIGN, 1},
  {"||", BINOP_LOGICAL_OR, PREC_LOGICAL_OR, 0},
  {"&&", BINOP_LOGICAL_AND, PREC_LOGICAL_AND, 0},
  {"|", BINOP_BITWISE_IOR, PREC_BITWISE_IOR, 0},
  {"^", BINOP_BITWISE_XOR, PREC_BITWISE_XOR, 0},
  {"&", BINOP_BITWISE_AND, PREC_BITWISE_AND, 0},
  {"==", BINOP_EQUAL, PREC_EQUAL, 0},
  {"!=", BINOP_NOTEQUAL, PREC_EQUAL, 0},
  {"<=", BINOP_LEQ, PREC_ORDER, 0},
  {">=", BINOP_GEQ, PREC_ORDER, 0},
  {">", BINOP_GTR, PREC_ORDER, 0},
  {"<", BINOP_LESS, PREC_ORDER, 0},
  {">>", BINOP_RSH, PREC_SHIFT, 0},
  {"<<", BINOP_LSH, PREC_SHIFT, 0},
  {"+", BINOP_ADD, PREC_ADD, 0},
  {"-", BINOP_SUB, PREC_ADD, 0},
  {"*", BINOP_MUL, PREC_MUL, 0},
  {"/", BINOP_DIV, PREC_MUL, 0},
  {"%", BINOP_REM, PREC_MUL, 0},
  {"@", BINOP_REPEAT, PREC_REPEAT, 0},
  {"-", UNOP_NEG, PREC_PREFIX, 0},
  {"!", UNOP_LOGICAL_NOT, PREC_PREFIX, 0},
  {"~", UNOP_COMPLEMENT, PREC_PREFIX, 0},
  {"*", UNOP_IND, PREC_PREFIX, 0},
  {"&", UNOP_ADDR, PREC_PREFIX, 0},
  {"sizeof ", UNOP_SIZEOF, PREC_PREFIX, 0},
  {"++", UNOP_PREINCREMENT, PREC_PREFIX, 0},
  {"--", UNOP_PREDECREMENT, PREC_PREFIX, 0},
  {NULL, 0, 0, 0}
};

static const struct language_defn d_language_defn =
{
  "d",
  language_d,
  range_check_off,
  case_sensitive_on,
  array_row_major,
  macro_expansion_c,
  &exp_descriptor_c,
  c_parse,
  c_error,
  null_post_parser,
  c_printchar,			/* Print a character constant.  */
  c_printstr,			/* Function to print string constant.  */
  c_emit_char,			/* Print a single char.  */
  c_print_type,			/* Print a type using appropriate syntax.  */
  c_print_typedef,		/* Print a typedef using appropriate
				   syntax.  */
  d_val_print,			/* Print a value using appropriate syntax.  */
  c_value_print,		/* Print a top-level value.  */
  default_read_var_value,	/* la_read_var_value */
  NULL,				/* Language specific skip_trampoline.  */
  "this",
  basic_lookup_symbol_nonlocal, 
  basic_lookup_transparent_type,
  d_demangle,			/* Language specific symbol demangler.  */
  NULL,				/* Language specific
				   class_name_from_physname.  */
  d_op_print_tab,		/* Expression operators for printing.  */
  1,				/* C-style arrays.  */
  0,				/* String lower bound.  */
  default_word_break_characters,
  default_make_symbol_completion_list,
  c_language_arch_info,
  default_print_array_index,
  default_pass_by_reference,
  c_get_string,
  NULL,				/* la_get_symbol_name_cmp */
  iterate_over_symbols,
  LANG_MAGIC
};

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_d_language;

void
_initialize_d_language (void)
{
  add_language (&d_language_defn);
}
