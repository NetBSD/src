/* Fortran language support definitions for GDB, the GNU debugger.

   Copyright (C) 1992-2017 Free Software Foundation, Inc.

   Contributed by Motorola.  Adapted from the C definitions by Farooq Butt
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

struct type_print_options;
struct parser_state;

extern int f_parse (struct parser_state *);

extern void f_yyerror (const char *);	/* Defined in f-exp.y */

extern void f_print_type (struct type *, const char *, struct ui_file *, int,
			  int, const struct type_print_options *);

extern void f_val_print (struct type *, int, CORE_ADDR,
			 struct ui_file *, int,
			 struct value *,
			 const struct value_print_options *);

/* Language-specific data structures */

/* A common block.  */

struct common_block
{
  /* The number of entries in the block.  */
  size_t n_entries;

  /* The contents of the block, allocated using the struct hack.  All
     pointers in the array are non-NULL.  */
  struct symbol *contents[1];
};

extern int f77_get_upperbound (struct type *);

extern int f77_get_lowerbound (struct type *);

extern void f77_get_dynamic_array_length (struct type *);

extern int calc_f77_array_dims (struct type *);


/* Fortran (F77) types */

struct builtin_f_type
{
  struct type *builtin_character;
  struct type *builtin_integer;
  struct type *builtin_integer_s2;
  struct type *builtin_logical;
  struct type *builtin_logical_s1;
  struct type *builtin_logical_s2;
  struct type *builtin_logical_s8;
  struct type *builtin_real;
  struct type *builtin_real_s8;
  struct type *builtin_real_s16;
  struct type *builtin_complex_s8;
  struct type *builtin_complex_s16;
  struct type *builtin_complex_s32;
  struct type *builtin_void;
};

/* Return the Fortran type table for the specified architecture.  */
extern const struct builtin_f_type *builtin_f_type (struct gdbarch *gdbarch);

