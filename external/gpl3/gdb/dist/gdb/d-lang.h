/* D language support definitions for GDB, the GNU debugger.

   Copyright (C) 2005-2016 Free Software Foundation, Inc.

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

#if !defined (D_LANG_H)
#define D_LANG_H 1

#include "symtab.h"

/* Language specific builtin types for D.  Any additional types added
   should be kept in sync with enum d_primitive_types, where these
   types are documented.  */

struct builtin_d_type
{
  struct type *builtin_void;
  struct type *builtin_bool;
  struct type *builtin_byte;
  struct type *builtin_ubyte;
  struct type *builtin_short;
  struct type *builtin_ushort;
  struct type *builtin_int;
  struct type *builtin_uint;
  struct type *builtin_long;
  struct type *builtin_ulong;
  struct type *builtin_cent;
  struct type *builtin_ucent;
  struct type *builtin_float;
  struct type *builtin_double;
  struct type *builtin_real;
  struct type *builtin_ifloat;
  struct type *builtin_idouble;
  struct type *builtin_ireal;
  struct type *builtin_cfloat;
  struct type *builtin_cdouble;
  struct type *builtin_creal;
  struct type *builtin_char;
  struct type *builtin_wchar;
  struct type *builtin_dchar;
};

/* Defined in d-exp.y.  */

extern int d_parse (struct parser_state *);

extern void d_yyerror (char *);

/* Defined in d-lang.c  */

extern const char *d_main_name (void);

extern char *d_demangle (const char *mangled, int options);

extern const struct builtin_d_type *builtin_d_type (struct gdbarch *);

/* Defined in d-namespace.c  */

extern struct block_symbol d_lookup_symbol_nonlocal (const struct language_defn *,
						     const char *,
						     const struct block *,
						     const domain_enum);

extern struct block_symbol d_lookup_nested_symbol (struct type *, const char *,
						   const struct block *);

/* Defined in d-valprint.c  */

extern void d_val_print (struct type *type, const gdb_byte *valaddr,
			 int embedded_offset, CORE_ADDR address,
			 struct ui_file *stream, int recurse,
			 const struct value *val,
			 const struct value_print_options *options);

#endif /* !defined (D_LANG_H) */
