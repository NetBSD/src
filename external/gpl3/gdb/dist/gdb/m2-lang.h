/* Modula 2 language support definitions for GDB, the GNU debugger.

   Copyright (C) 1992-2016 Free Software Foundation, Inc.

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

extern int m2_parse (struct parser_state *); /* Defined in m2-exp.y */

extern void m2_yyerror (char *);	/* Defined in m2-exp.y */

/* Defined in m2-typeprint.c */
extern void m2_print_type (struct type *, const char *, struct ui_file *, int,
			   int, const struct type_print_options *);

extern void m2_print_typedef (struct type *, struct symbol *,
			      struct ui_file *);

extern int m2_is_long_set (struct type *type);
extern int m2_is_unbounded_array (struct type *type);

extern void m2_val_print (struct type *, const gdb_byte *, int, CORE_ADDR,
			  struct ui_file *, int,
			  const struct value *,
			  const struct value_print_options *);

extern int get_long_set_bounds (struct type *type, LONGEST *low,
				LONGEST *high);

/* Modula-2 types */

struct builtin_m2_type
{
  struct type *builtin_char;
  struct type *builtin_int;
  struct type *builtin_card;
  struct type *builtin_real;
  struct type *builtin_bool;
};

/* Return the Modula-2 type table for the specified architecture.  */
extern const struct builtin_m2_type *builtin_m2_type (struct gdbarch *gdbarch);

