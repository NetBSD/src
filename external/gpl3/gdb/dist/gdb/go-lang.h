/* Go language support definitions for GDB, the GNU debugger.

   Copyright (C) 2012-2016 Free Software Foundation, Inc.

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

#if !defined (GO_LANG_H)
#define GO_LANG_H 1

struct type_print_options;

#include "gdbtypes.h"
#include "symtab.h"
#include "value.h"

struct parser_state;

struct builtin_go_type
{
  struct type *builtin_void;
  struct type *builtin_char;
  struct type *builtin_bool;
  struct type *builtin_int;
  struct type *builtin_uint;
  struct type *builtin_uintptr;
  struct type *builtin_int8;
  struct type *builtin_int16;
  struct type *builtin_int32;
  struct type *builtin_int64;
  struct type *builtin_uint8;
  struct type *builtin_uint16;
  struct type *builtin_uint32;
  struct type *builtin_uint64;
  struct type *builtin_float32;
  struct type *builtin_float64;
  struct type *builtin_complex64;
  struct type *builtin_complex128;
};

enum go_type
{
  GO_TYPE_NONE, /* Not a Go object.  */
  GO_TYPE_STRING
};

/* Defined in go-exp.y.  */

extern int go_parse (struct parser_state *);

extern void go_yyerror (char *);

/* Defined in go-lang.c.  */

extern const char *go_main_name (void);

extern enum go_type go_classify_struct_type (struct type *type);

extern char *go_demangle (const char *mangled, int options);

extern char *go_symbol_package_name (const struct symbol *sym);

extern char *go_block_package_name (const struct block *block);

extern const struct builtin_go_type *builtin_go_type (struct gdbarch *);

/* Defined in go-typeprint.c.  */

extern void go_print_type (struct type *type, const char *varstring,
			   struct ui_file *stream, int show, int level,
			   const struct type_print_options *flags);

/* Defined in go-valprint.c.  */

extern void go_val_print (struct type *type, const gdb_byte *valaddr,
			  int embedded_offset, CORE_ADDR address,
			  struct ui_file *stream, int recurse,
			  const struct value *val,
			  const struct value_print_options *options);

#endif /* !defined (GO_LANG_H) */
