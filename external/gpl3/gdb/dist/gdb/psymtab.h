/* Public partial symbol table definitions.

   Copyright (C) 2009-2014 Free Software Foundation, Inc.

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

#ifndef PSYMTAB_H
#define PSYMTAB_H

#include "symfile.h"

/* A bcache for partial symbols.  */

struct psymbol_bcache;

extern struct psymbol_bcache *psymbol_bcache_init (void);
extern void psymbol_bcache_free (struct psymbol_bcache *);
extern struct bcache *psymbol_bcache_get_bcache (struct psymbol_bcache *);

void expand_partial_symbol_names (int (*fun) (const char *, void *),
				  void *data);

void map_partial_symbol_filenames (symbol_filename_ftype *fun, void *data,
				   int need_fullname);

extern const struct quick_symbol_functions psym_functions;

extern const struct quick_symbol_functions dwarf2_gdb_index_functions;

/* Ensure that the partial symbols for OBJFILE have been loaded.  If
   VERBOSE is non-zero, then this will print a message when symbols
   are loaded.  This function always returns its argument, as a
   convenience.  */

extern struct objfile *require_partial_symbols (struct objfile *objfile,
						int verbose);

#endif /* PSYMTAB_H */
