/* D language support definitions for GDB, the GNU debugger.

   Copyright (C) 2005-2014 Free Software Foundation, Inc.

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

extern char *d_demangle (const char *mangled, int options);

extern void d_val_print (struct type *type, const gdb_byte *valaddr,
			 int embedded_offset, CORE_ADDR address,
			 struct ui_file *stream, int recurse,
			 const struct value *val,
			 const struct value_print_options *options);

#endif /* !defined (D_LANG_H) */
