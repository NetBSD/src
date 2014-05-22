/* Shared library declarations for GDB, the GNU Debugger.
   Copyright (C) 1990-2013 Free Software Foundation, Inc.

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

#ifndef SOLIST_H
#define SOLIST_H

#define SO_NAME_MAX_PATH_SIZE 512	/* FIXME: Should be dynamic */
/* For domain_enum domain.  */
#include "symtab.h"

/* Forward declaration for target specific link map information.  This
   struct is opaque to all but the target specific file.  */
struct lm_info;

struct so_list
  {
    /* The following fields of the structure come directly from the
       dynamic linker's tables in the inferior, and are initialized by
       current_sos.  */

    struct so_list *next;	/* next structure in linked list */

    /* A pointer to target specific link map information.  Often this
       will be a copy of struct link_map from the user process, but
       it need not be; it can be any collection of data needed to
       traverse the dynamic linker's data structures.  */
    struct lm_info *lm_info;

    /* Shared object file name, exactly as it appears in the
       inferior's link map.  This may be a relative path, or something
       which needs to be looked up in LD_LIBRARY_PATH, etc.  We use it
       to tell which entries in the inferior's dynamic linker's link
       map we've already loaded.  */
    char so_original_name[SO_NAME_MAX_PATH_SIZE];

    /* Shared object file name, expanded to something GDB can open.  */
    char so_name[SO_NAME_MAX_PATH_SIZE];

    /* Program space this shared library belongs to.  */
    struct program_space *pspace;

    /* The following fields of the structure are built from
       information gathered from the shared object file itself, and
       are set when we actually add it to our symbol tables.

       current_sos must initialize these fields to 0.  */

    bfd *abfd;
    char symbols_loaded;	/* flag: symbols read in yet?  */

    /* objfile with symbols for a loaded library.  Target memory is read from
       ABFD.  OBJFILE may be NULL either before symbols have been loaded, if
       the file cannot be found or after the command "nosharedlibrary".  */
    struct objfile *objfile;

    struct target_section *sections;
    struct target_section *sections_end;

    /* Record the range of addresses belonging to this shared library.
       There may not be just one (e.g. if two segments are relocated
       differently); but this is only used for "info sharedlibrary".  */
    CORE_ADDR addr_low, addr_high;
  };

struct target_so_ops
  {
    /* Adjust the section binding addresses by the base address at
       which the object was actually mapped.  */
    void (*relocate_section_addresses) (struct so_list *so,
                                        struct target_section *);

    /* Free the link map info and any other private data structures
       associated with a so_list entry.  */
    void (*free_so) (struct so_list *so);

    /* Reset or free private data structures not associated with
       so_list entries.  */
    void (*clear_solib) (void);

    /* Target dependent code to run after child process fork.  */
    void (*solib_create_inferior_hook) (int from_tty);

    /* Do additional symbol handling, lookup, etc. after symbols for a
       shared object have been loaded in the usual way.  This is
       called to do any system specific symbol handling that might be
       needed.  */
    void (*special_symbol_handling) (void);

    /* Construct a list of the currently loaded shared objects.  This
       list does not include an entry for the main executable file.

       Note that we only gather information directly available from the
       inferior --- we don't examine any of the shared library files
       themselves.  The declaration of `struct so_list' says which fields
       we provide values for.  */
    struct so_list *(*current_sos) (void);

    /* Find, open, and read the symbols for the main executable.  If
       FROM_TTYP dereferences to a non-zero integer, allow messages to
       be printed.  This parameter is a pointer rather than an int
       because open_symbol_file_object is called via catch_errors and
       catch_errors requires a pointer argument.  */
    int (*open_symbol_file_object) (void *from_ttyp);

    /* Determine if PC lies in the dynamic symbol resolution code of
       the run time loader.  */
    int (*in_dynsym_resolve_code) (CORE_ADDR pc);

    /* Find and open shared library binary file.  */
    bfd *(*bfd_open) (char *pathname);

    /* Extra hook for finding and opening a solib.
       Convenience function for remote debuggers finding host libs.  */
    int (*find_and_open_solib) (char *soname,
        unsigned o_flags, char **temp_pathname);

    /* Hook for looking up global symbols in a library-specific way.  */
    struct symbol * (*lookup_lib_global_symbol) (const struct objfile *objfile,
						 const char *name,
						 const domain_enum domain);

    /* Given two so_list objects, one from the GDB thread list
       and another from the list returned by current_sos, return 1
       if they represent the same library.
       Falls back to using strcmp on so_original_name field when set
       to NULL.  */
    int (*same) (struct so_list *gdb, struct so_list *inferior);

    /* Return whether a region of memory must be kept in a core file
       for shared libraries loaded before "gcore" is used to be
       handled correctly when the core file is loaded.  This only
       applies when the section would otherwise not be kept in the
       core file (in particular, for readonly sections).  */
    int (*keep_data_in_core) (CORE_ADDR vaddr,
			      unsigned long size);
  };

/* Free the memory associated with a (so_list *).  */
void free_so (struct so_list *so);

/* Return address of first so_list entry in master shared object list.  */
struct so_list *master_so_list (void);

/* Find shared library binary file.  */
extern char *solib_find (char *in_pathname, int *fd);

/* Open BFD for shared library file.  */
extern bfd *solib_bfd_fopen (char *pathname, int fd);

/* Find solib binary file and open it.  */
extern bfd *solib_bfd_open (char *in_pathname);

/* FIXME: gdbarch needs to control this variable.  */
extern struct target_so_ops *current_target_so_ops;

/* Handler for library-specific global symbol lookup in solib.c.  */
struct symbol *solib_global_lookup (const struct objfile *objfile,
				    const char *name,
				    const domain_enum domain);

#endif
