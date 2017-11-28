/* Rust language support definitions for GDB, the GNU debugger.

   Copyright (C) 2016-2017 Free Software Foundation, Inc.

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

#ifndef RUST_LANG_H
#define RUST_LANG_H

struct parser_state;
struct type;

/* The la_parser implementation for Rust.  */
extern int rust_parse (struct parser_state *);

/* The la_error implementation for Rust.  */
extern void rustyyerror (const char *);

/* Return true if TYPE is a tuple type; otherwise false.  */
extern bool rust_tuple_type_p (struct type *type);

/* Return true if TYPE is a tuple struct type; otherwise false.  */
extern bool rust_tuple_struct_type_p (struct type *type);

/* Given a block, find the name of the block's crate. Returns an empty
   stringif no crate name can be found.  */
extern std::string rust_crate_for_block (const struct block *block);

/* Create a new slice type.  NAME is the name of the type.  ELT_TYPE
   is the type of the elements of the slice.  USIZE_TYPE is the Rust
   "usize" type to use.  The new type is allocated whereever ELT_TYPE
   is allocated.  */
struct type *rust_slice_type (const char *name, struct type *elt_type,
			      struct type *usize_type);

#endif /* RUST_LANG_H */
