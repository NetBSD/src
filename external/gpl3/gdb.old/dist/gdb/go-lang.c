/* Go language support routines for GDB, the GNU debugger.

   Copyright (C) 2012-2014 Free Software Foundation, Inc.

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

/* TODO:
   - split stacks
   - printing of native types
   - goroutines
   - lots more
   - gccgo mangling needs redoing
     It's too hard, for example, to know whether one is looking at a mangled
     Go symbol or not, and their are ambiguities, e.g., the demangler may
     get passed *any* symbol, including symbols from other languages
     and including symbols that are already demangled.
     One thought is to at least add an _G prefix.
   - 6g mangling isn't supported yet
*/

#include "defs.h"
#include "gdb_assert.h"
#include "gdb_obstack.h"
#include <string.h>
#include "block.h"
#include "symtab.h"
#include "language.h"
#include "varobj.h"
#include "go-lang.h"
#include "c-lang.h"
#include "parser-defs.h"

#include <ctype.h>

/* The main function in the main package.  */
static const char GO_MAIN_MAIN[] = "main.main";

/* Function returning the special symbol name used by Go for the main
   procedure in the main program if it is found in minimal symbol list.
   This function tries to find minimal symbols so that it finds them even
   if the program was compiled without debugging information.  */

const char *
go_main_name (void)
{
  struct minimal_symbol *msym;

  msym = lookup_minimal_symbol (GO_MAIN_MAIN, NULL, NULL);
  if (msym != NULL)
    return GO_MAIN_MAIN;

  /* No known entry procedure found, the main program is probably not Go.  */
  return NULL;
}

/* Return non-zero if TYPE is a gccgo string.
   We assume CHECK_TYPEDEF has already been done.  */

static int
gccgo_string_p (struct type *type)
{
  /* gccgo strings don't necessarily have a name we can use.  */

  if (TYPE_NFIELDS (type) == 2)
    {
      struct type *type0 = TYPE_FIELD_TYPE (type, 0);
      struct type *type1 = TYPE_FIELD_TYPE (type, 1);

      CHECK_TYPEDEF (type0);
      CHECK_TYPEDEF (type1);

      if (TYPE_CODE (type0) == TYPE_CODE_PTR
	  && strcmp (TYPE_FIELD_NAME (type, 0), "__data") == 0
	  && TYPE_CODE (type1) == TYPE_CODE_INT
	  && strcmp (TYPE_FIELD_NAME (type, 1), "__length") == 0)
	{
	  struct type *target_type = TYPE_TARGET_TYPE (type0);

	  CHECK_TYPEDEF (target_type);

	  if (TYPE_CODE (target_type) == TYPE_CODE_INT
	      && TYPE_LENGTH (target_type) == 1
	      && strcmp (TYPE_NAME (target_type), "uint8") == 0)
	    return 1;
	}
    }

  return 0;
}

/* Return non-zero if TYPE is a 6g string.
   We assume CHECK_TYPEDEF has already been done.  */

static int
sixg_string_p (struct type *type)
{
  if (TYPE_NFIELDS (type) == 2
      && TYPE_TAG_NAME (type) != NULL
      && strcmp (TYPE_TAG_NAME (type), "string") == 0)
    return 1;

  return 0;
}

/* Classify the kind of Go object that TYPE is.
   TYPE is a TYPE_CODE_STRUCT, used to represent a Go object.  */

enum go_type
go_classify_struct_type (struct type *type)
{
  CHECK_TYPEDEF (type);

  /* Recognize strings as they're useful to be able to print without
     pretty-printers.  */
  if (gccgo_string_p (type)
      || sixg_string_p (type))
    return GO_TYPE_STRING;

  return GO_TYPE_NONE;
}

/* Subroutine of unpack_mangled_go_symbol to simplify it.
   Given "[foo.]bar.baz", store "bar" in *PACKAGEP and "baz" in *OBJECTP.
   We stomp on the last '.' to nul-terminate "bar".
   The caller is responsible for memory management.  */

static void
unpack_package_and_object (char *buf,
			   const char **packagep, const char **objectp)
{
  char *last_dot;

  last_dot = strrchr (buf, '.');
  gdb_assert (last_dot != NULL);
  *objectp = last_dot + 1;
  *last_dot = '\0';
  last_dot = strrchr (buf, '.');
  if (last_dot != NULL)
    *packagep = last_dot + 1;
  else
    *packagep = buf;
}

/* Given a mangled Go symbol, find its package name, object name, and
   method type (if present).
   E.g., for "libgo_net.textproto.String.N33_libgo_net.textproto.ProtocolError"
   *PACKAGEP = "textproto"
   *OBJECTP = "String"
   *METHOD_TYPE_PACKAGEP = "textproto"
   *METHOD_TYPE_OBJECTP = "ProtocolError"

   Space for the resulting strings is malloc'd in one buffer.
   PACKAGEP,OBJECTP,METHOD_TYPE* will (typically) point into this buffer.
   [There are a few exceptions, but the caller is still responsible for
   freeing the resulting pointer.]
   A pointer to this buffer is returned, or NULL if symbol isn't a
   mangled Go symbol.
   The caller is responsible for freeing the result.

   *METHOD_TYPE_IS_POINTERP is set to a boolean indicating if
   the method type is a pointer.

   There may be value in returning the outer container,
   i.e., "net" in the above example, but for now it's not needed.
   Plus it's currently not straightforward to compute,
   it comes from -fgo-prefix, and there's no algorithm to compute it.

   If we ever need to unpack the method type, this routine should work
   for that too.  */

static char *
unpack_mangled_go_symbol (const char *mangled_name,
			  const char **packagep,
			  const char **objectp,
			  const char **method_type_packagep,
			  const char **method_type_objectp,
			  int *method_type_is_pointerp)
{
  char *buf;
  char *p;
  int len = strlen (mangled_name);
  /* Pointer to last digit in "N<digit(s)>_".  */
  char *saw_digit;
  /* Pointer to "N" if valid "N<digit(s)>_" found.  */
  char *method_type;
  /* Pointer to the first '.'.  */
  char *first_dot;
  /* Pointer to the last '.'.  */
  char *last_dot;
  /* Non-zero if we saw a pointer indicator.  */
  int saw_pointer;

  *packagep = *objectp = NULL;
  *method_type_packagep = *method_type_objectp = NULL;
  *method_type_is_pointerp = 0;

  /* main.init is mangled specially.  */
  if (strcmp (mangled_name, "__go_init_main") == 0)
    {
      char *package = xstrdup ("main");

      *packagep = package;
      *objectp = "init";
      return package;
    }

  /* main.main is mangled specially (missing prefix).  */
  if (strcmp (mangled_name, "main.main") == 0)
    {
      char *package = xstrdup ("main");

      *packagep = package;
      *objectp = "main";
      return package;
    }

  /* We may get passed, e.g., "main.T.Foo", which is *not* mangled.
     Alas it looks exactly like "prefix.package.object."
     To cope for now we only recognize the following prefixes:

     go: the default
     libgo_.*: used by gccgo's runtime

     Thus we don't support -fgo-prefix (except as used by the runtime).  */
  if (strncmp (mangled_name, "go.", 3) != 0
      && strncmp (mangled_name, "libgo_", 6) != 0)
    return NULL;

  /* Quick check for whether a search may be fruitful.  */
  /* Ignore anything with @plt, etc. in it.  */
  if (strchr (mangled_name, '@') != NULL)
    return NULL;
  /* It must have at least two dots.  */
  first_dot = strchr (mangled_name, '.');
  if (first_dot == NULL)
    return NULL;
  /* Treat "foo.bar" as unmangled.  It can collide with lots of other
     languages and it's not clear what the consequences are.
     And except for main.main, all gccgo symbols are at least
     prefix.package.object.  */
  last_dot = strrchr (mangled_name, '.');
  if (last_dot == first_dot)
    return NULL;

  /* More quick checks.  */
  if (last_dot[1] == '\0' /* foo. */
      || last_dot[-1] == '.') /* foo..bar */
    return NULL;

  /* At this point we've decided we have a mangled Go symbol.  */

  buf = xstrdup (mangled_name);

  /* Search backwards looking for "N<digit(s)>".  */
  p = buf + len;
  saw_digit = method_type = NULL;
  saw_pointer = 0;
  while (p > buf)
    {
      int current = *(const unsigned char *) --p;
      int current_is_digit = isdigit (current);

      if (saw_digit)
	{
	  if (current_is_digit)
	    continue;
	  if (current == 'N'
	      && ((p > buf && p[-1] == '.')
		  || (p > buf + 1 && p[-1] == 'p' && p[-2] == '.')))
	    {
	      if (atoi (p + 1) == strlen (saw_digit + 2))
		{
		  if (p[-1] == '.')
		    method_type = p - 1;
		  else
		    {
		      gdb_assert (p[-1] == 'p');
		      saw_pointer = 1;
		      method_type = p - 2;
		    }
		  break;
		}
	    }
	  /* Not what we're looking for, reset and keep looking.  */
	  saw_digit = NULL;
	  saw_pointer = 0;
	  continue;
	}
      if (current_is_digit && p[1] == '_')
	{
	  /* Possible start of method "this" [sic] type.  */
	  saw_digit = p;
	  continue;
	}
    }

  if (method_type != NULL
      /* Ensure not something like "..foo".  */
      && (method_type > buf && method_type[-1] != '.'))
    {
      unpack_package_and_object (saw_digit + 2,
				 method_type_packagep, method_type_objectp);
      *method_type = '\0';
      *method_type_is_pointerp = saw_pointer;
    }

  unpack_package_and_object (buf, packagep, objectp);
  return buf;
}

/* Implements the la_demangle language_defn routine for language Go.

   N.B. This may get passed *any* symbol, including symbols from other
   languages and including symbols that are already demangled.
   Both of these situations are kinda unfortunate, but that's how things
   are today.

   N.B. This currently only supports gccgo's mangling.

   N.B. gccgo's mangling needs, I think, changing.
   This demangler can't work in all situations,
   thus not too much effort is currently put into it.  */

char *
go_demangle (const char *mangled_name, int options)
{
  struct obstack tempbuf;
  char *result;
  char *name_buf;
  const char *package_name;
  const char *object_name;
  const char *method_type_package_name;
  const char *method_type_object_name;
  int method_type_is_pointer;

  if (mangled_name == NULL)
    return NULL;

  name_buf = unpack_mangled_go_symbol (mangled_name,
				       &package_name, &object_name,
				       &method_type_package_name,
				       &method_type_object_name,
				       &method_type_is_pointer);
  if (name_buf == NULL)
    return NULL;

  obstack_init (&tempbuf);

  /* Print methods as they appear in "method expressions".  */
  if (method_type_package_name != NULL)
    {
      /* FIXME: Seems like we should include package_name here somewhere.  */
      if (method_type_is_pointer)
	  obstack_grow_str (&tempbuf, "(*");
      obstack_grow_str (&tempbuf, method_type_package_name);
      obstack_grow_str (&tempbuf, ".");
      obstack_grow_str (&tempbuf, method_type_object_name);
      if (method_type_is_pointer)
	obstack_grow_str (&tempbuf, ")");
      obstack_grow_str (&tempbuf, ".");
      obstack_grow_str (&tempbuf, object_name);
    }
  else
    {
      obstack_grow_str (&tempbuf, package_name);
      obstack_grow_str (&tempbuf, ".");
      obstack_grow_str (&tempbuf, object_name);
    }
  obstack_grow_str0 (&tempbuf, "");

  result = xstrdup (obstack_finish (&tempbuf));
  obstack_free (&tempbuf, NULL);
  xfree (name_buf);
  return result;
}

/* Given a Go symbol, return its package or NULL if unknown.
   Space for the result is malloc'd, caller must free.  */

char *
go_symbol_package_name (const struct symbol *sym)
{
  const char *mangled_name = SYMBOL_LINKAGE_NAME (sym);
  const char *package_name;
  const char *object_name;
  const char *method_type_package_name;
  const char *method_type_object_name;
  int method_type_is_pointer;
  char *name_buf;
  char *result;

  gdb_assert (SYMBOL_LANGUAGE (sym) == language_go);
  name_buf = unpack_mangled_go_symbol (mangled_name,
				       &package_name, &object_name,
				       &method_type_package_name,
				       &method_type_object_name,
				       &method_type_is_pointer);
  /* Some Go symbols don't have mangled form we interpret (yet).  */
  if (name_buf == NULL)
    return NULL;
  result = xstrdup (package_name);
  xfree (name_buf);
  return result;
}

/* Return the package that BLOCK is in, or NULL if there isn't one.
   Space for the result is malloc'd, caller must free.  */

char *
go_block_package_name (const struct block *block)
{
  while (block != NULL)
    {
      struct symbol *function = BLOCK_FUNCTION (block);

      if (function != NULL)
	{
	  char *package_name = go_symbol_package_name (function);

	  if (package_name != NULL)
	    return package_name;

	  /* Stop looking if we find a function without a package name.
	     We're most likely outside of Go and thus the concept of the
	     "current" package is gone.  */
	  return NULL;
	}

      block = BLOCK_SUPERBLOCK (block);
    }

  return NULL;
}

/* Table mapping opcodes into strings for printing operators
   and precedences of the operators.
   TODO(dje): &^ ?  */

static const struct op_print go_op_print_tab[] =
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
  {"^", UNOP_COMPLEMENT, PREC_PREFIX, 0},
  {"*", UNOP_IND, PREC_PREFIX, 0},
  {"&", UNOP_ADDR, PREC_PREFIX, 0},
  {"unsafe.Sizeof ", UNOP_SIZEOF, PREC_PREFIX, 0},
  {"++", UNOP_POSTINCREMENT, PREC_SUFFIX, 0},
  {"--", UNOP_POSTDECREMENT, PREC_SUFFIX, 0},
  {NULL, 0, 0, 0}
};

enum go_primitive_types {
  go_primitive_type_void,
  go_primitive_type_char,
  go_primitive_type_bool,
  go_primitive_type_int,
  go_primitive_type_uint,
  go_primitive_type_uintptr,
  go_primitive_type_int8,
  go_primitive_type_int16,
  go_primitive_type_int32,
  go_primitive_type_int64,
  go_primitive_type_uint8,
  go_primitive_type_uint16,
  go_primitive_type_uint32,
  go_primitive_type_uint64,
  go_primitive_type_float32,
  go_primitive_type_float64,
  go_primitive_type_complex64,
  go_primitive_type_complex128,
  nr_go_primitive_types
};

static void
go_language_arch_info (struct gdbarch *gdbarch,
		       struct language_arch_info *lai)
{
  const struct builtin_go_type *builtin = builtin_go_type (gdbarch);

  lai->string_char_type = builtin->builtin_char;

  lai->primitive_type_vector
    = GDBARCH_OBSTACK_CALLOC (gdbarch, nr_go_primitive_types + 1,
			      struct type *);

  lai->primitive_type_vector [go_primitive_type_void]
    = builtin->builtin_void;
  lai->primitive_type_vector [go_primitive_type_char]
    = builtin->builtin_char;
  lai->primitive_type_vector [go_primitive_type_bool]
    = builtin->builtin_bool;
  lai->primitive_type_vector [go_primitive_type_int]
    = builtin->builtin_int;
  lai->primitive_type_vector [go_primitive_type_uint]
    = builtin->builtin_uint;
  lai->primitive_type_vector [go_primitive_type_uintptr]
    = builtin->builtin_uintptr;
  lai->primitive_type_vector [go_primitive_type_int8]
    = builtin->builtin_int8;
  lai->primitive_type_vector [go_primitive_type_int16]
    = builtin->builtin_int16;
  lai->primitive_type_vector [go_primitive_type_int32]
    = builtin->builtin_int32;
  lai->primitive_type_vector [go_primitive_type_int64]
    = builtin->builtin_int64;
  lai->primitive_type_vector [go_primitive_type_uint8]
    = builtin->builtin_uint8;
  lai->primitive_type_vector [go_primitive_type_uint16]
    = builtin->builtin_uint16;
  lai->primitive_type_vector [go_primitive_type_uint32]
    = builtin->builtin_uint32;
  lai->primitive_type_vector [go_primitive_type_uint64]
    = builtin->builtin_uint64;
  lai->primitive_type_vector [go_primitive_type_float32]
    = builtin->builtin_float32;
  lai->primitive_type_vector [go_primitive_type_float64]
    = builtin->builtin_float64;
  lai->primitive_type_vector [go_primitive_type_complex64]
    = builtin->builtin_complex64;
  lai->primitive_type_vector [go_primitive_type_complex128]
    = builtin->builtin_complex128;

  lai->bool_type_symbol = "bool";
  lai->bool_type_default = builtin->builtin_bool;
}

static const struct language_defn go_language_defn =
{
  "go",
  "Go",
  language_go,
  range_check_off,
  case_sensitive_on,
  array_row_major,
  macro_expansion_no,
  &exp_descriptor_c,
  go_parse,
  go_error,
  null_post_parser,
  c_printchar,			/* Print a character constant.  */
  c_printstr,			/* Function to print string constant.  */
  c_emit_char,			/* Print a single char.  */
  go_print_type,		/* Print a type using appropriate syntax.  */
  c_print_typedef,		/* Print a typedef using appropriate
				   syntax.  */
  go_val_print,			/* Print a value using appropriate syntax.  */
  c_value_print,		/* Print a top-level value.  */
  default_read_var_value,	/* la_read_var_value */
  NULL,				/* Language specific skip_trampoline.  */
  NULL,				/* name_of_this */
  basic_lookup_symbol_nonlocal, 
  basic_lookup_transparent_type,
  go_demangle,			/* Language specific symbol demangler.  */
  NULL,				/* Language specific
				   class_name_from_physname.  */
  go_op_print_tab,		/* Expression operators for printing.  */
  1,				/* C-style arrays.  */
  0,				/* String lower bound.  */
  default_word_break_characters,
  default_make_symbol_completion_list,
  go_language_arch_info,
  default_print_array_index,
  default_pass_by_reference,
  c_get_string,
  NULL,
  iterate_over_symbols,
  &default_varobj_ops,
  LANG_MAGIC
};

static void *
build_go_types (struct gdbarch *gdbarch)
{
  struct builtin_go_type *builtin_go_type
    = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct builtin_go_type);

  builtin_go_type->builtin_void
    = arch_type (gdbarch, TYPE_CODE_VOID, 1, "void");
  builtin_go_type->builtin_char
    = arch_character_type (gdbarch, 8, 1, "char");
  builtin_go_type->builtin_bool
    = arch_boolean_type (gdbarch, 8, 0, "bool");
  builtin_go_type->builtin_int
    = arch_integer_type (gdbarch, gdbarch_int_bit (gdbarch), 0, "int");
  builtin_go_type->builtin_uint
    = arch_integer_type (gdbarch, gdbarch_int_bit (gdbarch), 1, "uint");
  builtin_go_type->builtin_uintptr
    = arch_integer_type (gdbarch, gdbarch_ptr_bit (gdbarch), 1, "uintptr");
  builtin_go_type->builtin_int8
    = arch_integer_type (gdbarch, 8, 0, "int8");
  builtin_go_type->builtin_int16
    = arch_integer_type (gdbarch, 16, 0, "int16");
  builtin_go_type->builtin_int32
    = arch_integer_type (gdbarch, 32, 0, "int32");
  builtin_go_type->builtin_int64
    = arch_integer_type (gdbarch, 64, 0, "int64");
  builtin_go_type->builtin_uint8
    = arch_integer_type (gdbarch, 8, 1, "uint8");
  builtin_go_type->builtin_uint16
    = arch_integer_type (gdbarch, 16, 1, "uint16");
  builtin_go_type->builtin_uint32
    = arch_integer_type (gdbarch, 32, 1, "uint32");
  builtin_go_type->builtin_uint64
    = arch_integer_type (gdbarch, 64, 1, "uint64");
  builtin_go_type->builtin_float32
    = arch_float_type (gdbarch, 32, "float32", NULL);
  builtin_go_type->builtin_float64
    = arch_float_type (gdbarch, 64, "float64", NULL);
  builtin_go_type->builtin_complex64
    = arch_complex_type (gdbarch, "complex64",
			 builtin_go_type->builtin_float32);
  builtin_go_type->builtin_complex128
    = arch_complex_type (gdbarch, "complex128",
			 builtin_go_type->builtin_float64);

  return builtin_go_type;
}

static struct gdbarch_data *go_type_data;

const struct builtin_go_type *
builtin_go_type (struct gdbarch *gdbarch)
{
  return gdbarch_data (gdbarch, go_type_data);
}

extern initialize_file_ftype _initialize_go_language;

void
_initialize_go_language (void)
{
  go_type_data = gdbarch_data_register_post_init (build_go_types);

  add_language (&go_language_defn);
}
