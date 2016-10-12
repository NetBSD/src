/* Shared library declarations for GDB, the GNU Debugger.
   
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

#ifndef SOLIB_H
#define SOLIB_H

/* Forward decl's for prototypes */
struct so_list;
struct target_ops;
struct target_so_ops;
struct program_space;

/* Called when we free all symtabs, to free the shared library information
   as well.  */

extern void clear_solib (void);

/* Called to add symbols from a shared library to gdb's symbol table.  */

extern void solib_add (const char *, int, struct target_ops *, int);
extern int solib_read_symbols (struct so_list *, int);

/* Function to be called when the inferior starts up, to discover the
   names of shared libraries that are dynamically linked, the base
   addresses to which they are linked, and sufficient information to
   read in their symbols at a later time.  */

extern void solib_create_inferior_hook (int from_tty);

/* If ADDR lies in a shared library, return its name.  */

extern char *solib_name_from_address (struct program_space *, CORE_ADDR);

/* Return 1 if ADDR lies within SOLIB.  */

extern int solib_contains_address_p (const struct so_list *, CORE_ADDR);

/* Return whether the data starting at VADDR, size SIZE, must be kept
   in a core file for shared libraries loaded before "gcore" is used
   to be handled correctly when the core file is loaded.  This only
   applies when the section would otherwise not be kept in the core
   file (in particular, for readonly sections).  */

extern int solib_keep_data_in_core (CORE_ADDR vaddr, unsigned long size);

/* Return 1 if PC lies in the dynamic symbol resolution code of the
   run time loader.  */

extern int in_solib_dynsym_resolve_code (CORE_ADDR);

/* Discard symbols that were auto-loaded from shared libraries.  */

extern void no_shared_libraries (char *ignored, int from_tty);

/* Set the solib operations for GDBARCH to NEW_OPS.  */

extern void set_solib_ops (struct gdbarch *gdbarch,
			   const struct target_so_ops *new_ops);

/* Return non-zero if NAME is the libpthread shared library.  */

extern int libpthread_name_p (const char *name);

/* Look up symbol from both symbol table and dynamic string table.  */

extern CORE_ADDR gdb_bfd_lookup_symbol (bfd *abfd,
					int (*match_sym) (const asymbol *,
							  const void *),
					const void *data);

/* Look up symbol from symbol table.  */

extern CORE_ADDR gdb_bfd_lookup_symbol_from_symtab (bfd *abfd,
						    int (*match_sym)
						      (const asymbol *,
						       const void *),
						    const void *data);

/* Enable or disable optional solib event breakpoints as appropriate.  */

extern void update_solib_breakpoints (void);

/* Handle an solib event by calling solib_add.  */

extern void handle_solib_event (void);

#endif /* SOLIB_H */
