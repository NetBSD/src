/* Java language support definitions for GDB, the GNU debugger.

   Copyright (C) 1997-2016 Free Software Foundation, Inc.

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

#ifndef JV_LANG_H
#define JV_LANG_H

struct value;
struct type_print_options;
struct parser_state;

extern int java_parse (struct parser_state *); /* Defined in jv-exp.y */

extern void java_yyerror (char *);	/* Defined in jv-exp.y */

struct builtin_java_type
{
  struct type *builtin_int;
  struct type *builtin_byte;
  struct type *builtin_short;
  struct type *builtin_long;
  struct type *builtin_boolean;
  struct type *builtin_char;
  struct type *builtin_float;
  struct type *builtin_double;
  struct type *builtin_void;
};

extern const struct builtin_java_type *builtin_java_type (struct gdbarch *);

extern void java_val_print (struct type *, const gdb_byte *, int, CORE_ADDR,
			    struct ui_file *, int,
			    const struct value *,
			    const struct value_print_options *);

extern void java_value_print (struct value *, struct ui_file *,
			      const struct value_print_options *);

extern struct value *java_class_from_object (struct value *);

extern struct type *type_from_class (struct gdbarch *, struct value *);

extern struct type *java_primitive_type (struct gdbarch *, int signature);

extern struct type *java_primitive_type_from_name (struct gdbarch *,
						   const char *, int);

extern struct type *java_array_type (struct type *, int);

extern struct type *get_java_object_type (void);
extern int get_java_object_header_size (struct gdbarch *);

extern struct type *java_lookup_class (char *);

extern int is_object_type (struct type *);

/* Defined in jv-typeprint.c */
extern void java_print_type (struct type *, const char *,
			     struct ui_file *, int, int,
			     const struct type_print_options *);

extern char *java_demangle_type_signature (const char *);

#endif
