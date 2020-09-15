/* Fortran language support definitions for GDB, the GNU debugger.

   Copyright (C) 1992-2020 Free Software Foundation, Inc.

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

#ifndef F_LANG_H
#define F_LANG_H

struct type_print_options;
struct parser_state;

extern int f_parse (struct parser_state *);

/* Implement the la_print_typedef language method for Fortran.  */

extern void f_print_typedef (struct type *type, struct symbol *new_symbol,
			     struct ui_file *stream);

extern void f_print_type (struct type *, const char *, struct ui_file *, int,
			  int, const struct type_print_options *);

/* Implement la_value_print_inner for Fortran.  */

extern void f_value_print_inner (struct value *val, struct ui_file *stream,
				  int recurse,
				  const struct value_print_options *options);

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

extern LONGEST f77_get_upperbound (struct type *);

extern LONGEST f77_get_lowerbound (struct type *);

extern void f77_get_dynamic_array_length (struct type *);

extern int calc_f77_array_dims (struct type *);


/* Fortran (F77) types */

struct builtin_f_type
{
  struct type *builtin_character;
  struct type *builtin_integer;
  struct type *builtin_integer_s2;
  struct type *builtin_integer_s8;
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

/* Ensures that function argument VALUE is in the appropriate form to
   pass to a Fortran function.  Returns a possibly new value that should
   be used instead of VALUE.

   When IS_ARTIFICIAL is true this indicates an artificial argument,
   e.g. hidden string lengths which the GNU Fortran argument passing
   convention specifies as being passed by value.

   When IS_ARTIFICIAL is false, the argument is passed by pointer.  If the
   value is already in target memory then return a value that is a pointer
   to VALUE.  If VALUE is not in memory (e.g. an integer literal), allocate
   space in the target, copy VALUE in, and return a pointer to the in
   memory copy.  */

extern struct value *fortran_argument_convert (struct value *value,
					       bool is_artificial);

/* Ensures that function argument TYPE is appropriate to inform the debugger
   that ARG should be passed as a pointer.  Returns the potentially updated
   argument type.

   If ARG is of type pointer then the type of ARG is returned, otherwise
   TYPE is returned untouched.

   This function exists to augment the types of Fortran function call
   parameters to be pointers to the reported value, when the corresponding ARG
   has also been wrapped in a pointer (by fortran_argument_convert).  This
   informs the debugger that these arguments should be passed as a pointer
   rather than as the pointed to type.  */

extern struct type *fortran_preserve_arg_pointer (struct value *arg,
						  struct type *type);

#endif /* F_LANG_H */
