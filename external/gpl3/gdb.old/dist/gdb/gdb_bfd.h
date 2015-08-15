/* Definitions for BFD wrappers used by GDB.

   Copyright (C) 2011-2014 Free Software Foundation, Inc.

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

#ifndef GDB_BFD_H
#define GDB_BFD_H

#include "registry.h"

DECLARE_REGISTRY (bfd);

/* Open a read-only (FOPEN_RB) BFD given arguments like bfd_fopen.
   Returns NULL on error.  On success, returns a new reference to the
   BFD, which must be freed with gdb_bfd_unref.  BFDs returned by this
   call are shared among all callers opening the same file.  If FD is
   not -1, then after this call it is owned by BFD.  */

struct bfd *gdb_bfd_open (const char *name, const char *target, int fd);

/* Increment the reference count of ABFD.  It is fine for ABFD to be
   NULL; in this case the function does nothing.  */

void gdb_bfd_ref (struct bfd *abfd);

/* Decrement the reference count of ABFD.  If this is the last
   reference, ABFD will be freed.  If ABFD is NULL, this function does
   nothing.  */

void gdb_bfd_unref (struct bfd *abfd);

/* Mark the CHILD BFD as being a member of PARENT.  Also, increment
   the reference count of CHILD.  Calling this function ensures that
   as along as CHILD remains alive, PARENT will as well.  Both CHILD
   and PARENT must be non-NULL.  This can be called more than once
   with the same arguments; but it is not allowed to call it for a
   single CHILD with different values for PARENT.  */

void gdb_bfd_mark_parent (bfd *child, bfd *parent);

/* Mark INCLUDEE as being included by INCLUDER.
   This is used to associate the life time of INCLUDEE with INCLUDER.
   For example, with Fission, one file can refer to debug info in another
   file, and internal tables we build for the main file (INCLUDER) may refer
   to data contained in INCLUDEE.  Therefore we want to keep INCLUDEE around
   at least as long as INCLUDER exists.

   Note that this is different than gdb_bfd_mark_parent because in our case
   lifetime tracking is based on the "parent" whereas in gdb_bfd_mark_parent
   lifetime tracking is based on the "child".  Plus in our case INCLUDEE could
   have multiple different "parents".  */

void gdb_bfd_record_inclusion (bfd *includer, bfd *includee);

/* Try to read or map the contents of the section SECT.  If
   successful, the section data is returned and *SIZE is set to the
   size of the section data; this may not be the same as the size
   according to bfd_get_section_size if the section was compressed.
   The returned section data is associated with the BFD and will be
   destroyed when the BFD is destroyed.  There is no other way to free
   it; for temporary uses of section data, see
   bfd_malloc_and_get_section.  SECT may not have relocations.  This
   function will throw on error.  */

const gdb_byte *gdb_bfd_map_section (asection *section, bfd_size_type *size);

/* Compute the CRC for ABFD.  The CRC is used to find and verify
   separate debug files.  When successful, this fills in *CRC_OUT and
   returns 1.  Otherwise, this issues a warning and returns 0.  */

int gdb_bfd_crc (struct bfd *abfd, unsigned long *crc_out);



/* A wrapper for bfd_fopen that initializes the gdb-specific reference
   count.  */

bfd *gdb_bfd_fopen (const char *, const char *, const char *, int);

/* A wrapper for bfd_openr that initializes the gdb-specific reference
   count.  */

bfd *gdb_bfd_openr (const char *, const char *);

/* A wrapper for bfd_openw that initializes the gdb-specific reference
   count.  */

bfd *gdb_bfd_openw (const char *, const char *);

/* A wrapper for bfd_openr_iovec that initializes the gdb-specific
   reference count.  */

bfd *gdb_bfd_openr_iovec (const char *filename, const char *target,
			  void *(*open_func) (struct bfd *nbfd,
					      void *open_closure),
			  void *open_closure,
			  file_ptr (*pread_func) (struct bfd *nbfd,
						  void *stream,
						  void *buf,
						  file_ptr nbytes,
						  file_ptr offset),
			  int (*close_func) (struct bfd *nbfd,
					     void *stream),
			  int (*stat_func) (struct bfd *abfd,
					    void *stream,
					    struct stat *sb));

/* A wrapper for bfd_openr_next_archived_file that initializes the
   gdb-specific reference count.  */

bfd *gdb_bfd_openr_next_archived_file (bfd *archive, bfd *previous);

/* A wrapper for bfd_fdopenr that initializes the gdb-specific
   reference count.  */

bfd *gdb_bfd_fdopenr (const char *filename, const char *target, int fd);



/* Return the index of the BFD section SECTION.  Ordinarily this is
   just the section's index, but for some special sections, like
   bfd_com_section_ptr, it will be a synthesized value.  */

int gdb_bfd_section_index (bfd *abfd, asection *section);


/* Like bfd_count_sections, but include any possible global sections,
   like bfd_com_section_ptr.  */

int gdb_bfd_count_sections (bfd *abfd);

/* Return true if any section requires relocations, false
   otherwise.  */

int gdb_bfd_requires_relocations (bfd *abfd);

#endif /* GDB_BFD_H */
