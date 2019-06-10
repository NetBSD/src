/* Work with executable files, for GDB, the GNU debugger.

   Copyright (C) 2003-2017 Free Software Foundation, Inc.

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

#ifndef EXEC_H
#define EXEC_H

#include "target.h"
#include "progspace.h"
#include "memrange.h"
#include "symfile-add-flags.h"

struct target_section;
struct target_ops;
struct bfd;
struct objfile;

#define exec_bfd current_program_space->ebfd
#define exec_bfd_mtime current_program_space->ebfd_mtime
#define exec_filename current_program_space->pspace_exec_filename

/* Builds a section table, given args BFD, SECTABLE_PTR, SECEND_PTR.
   Returns 0 if OK, 1 on error.  */

extern int build_section_table (struct bfd *, struct target_section **,
				struct target_section **);

/* Remove all entries from TABLE.  */

extern void clear_section_table (struct target_section_table *table);

/* Read from mappable read-only sections of BFD executable files.
   Return TARGET_XFER_OK, if read is successful.  Return
   TARGET_XFER_EOF if read is done.  Return TARGET_XFER_E_IO
   otherwise.  */

extern enum target_xfer_status
  exec_read_partial_read_only (gdb_byte *readbuf, ULONGEST offset,
			       ULONGEST len, ULONGEST *xfered_len);

/* Read or write from mappable sections of BFD executable files.

   Request to transfer up to LEN 8-bit bytes of the target sections
   defined by SECTIONS and SECTIONS_END.  The OFFSET specifies the
   starting address.
   If SECTION_NAME is not NULL, only access sections with that same
   name.

   Return the number of bytes actually transfered, or zero when no
   data is available for the requested range.

   This function is intended to be used from target_xfer_partial
   implementations.  See target_read and target_write for more
   information.

   One, and only one, of readbuf or writebuf must be non-NULL.  */

extern enum target_xfer_status
  section_table_xfer_memory_partial (gdb_byte *,
				     const gdb_byte *,
				     ULONGEST, ULONGEST, ULONGEST *,
				     struct target_section *,
				     struct target_section *,
				     const char *);

/* Read from mappable read-only sections of BFD executable files.
   Similar to exec_read_partial_read_only, but return
   TARGET_XFER_UNAVAILABLE if data is unavailable.  */

extern enum target_xfer_status
  section_table_read_available_memory (gdb_byte *readbuf, ULONGEST offset,
				       ULONGEST len, ULONGEST *xfered_len);

/* Set the loaded address of a section.  */
extern void exec_set_section_address (const char *, int, CORE_ADDR);

/* Remove all target sections owned by OWNER.  */

extern void remove_target_sections (void *owner);

/* Add the sections array defined by [SECTIONS..SECTIONS_END[ to the
   current set of target sections.  */

extern void add_target_sections (void *owner,
				 struct target_section *sections,
				 struct target_section *sections_end);

/* Add the sections of OBJFILE to the current set of target sections.
 * OBJFILE owns the new target sections.  */

extern void add_target_sections_of_objfile (struct objfile *objfile);

/* Prints info about all sections defined in the TABLE.  ABFD is
   special cased --- it's filename is omitted; if it is the executable
   file, its entry point is printed.  */

extern void print_section_info (struct target_section_table *table,
				bfd *abfd);

extern void exec_close (void);

/* Helper function that attempts to open the symbol file at EXEC_FILE_HOST.
   If successful, it proceeds to add the symbol file as the main symbol file.

   ADD_FLAGS is passed on to the function adding the symbol file.  */
extern void try_open_exec_file (const char *exec_file_host,
				struct inferior *inf,
				symfile_add_flags add_flags);
#endif
