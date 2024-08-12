/* build-id-related functions.

   Copyright (C) 1991-2023 Free Software Foundation, Inc.

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

#ifndef BUILD_ID_H
#define BUILD_ID_H

#include "gdb_bfd.h"
#include "gdbsupport/rsp-low.h"

/* Locate NT_GNU_BUILD_ID from ABFD and return its content.  */

extern const struct bfd_build_id *build_id_bfd_get (bfd *abfd);

/* Return true if ABFD has NT_GNU_BUILD_ID matching the CHECK value.
   Otherwise, issue a warning and return false.  */

extern int build_id_verify (bfd *abfd,
			    size_t check_len, const bfd_byte *check);


/* Find and open a BFD for a debuginfo file  given a build-id.  If no BFD
   can be found, return NULL.  */

extern gdb_bfd_ref_ptr build_id_to_debug_bfd (size_t build_id_len,
					      const bfd_byte *build_id);

/* Find and open a BFD for an executable file given a build-id.  If no BFD
   can be found, return NULL.  The returned reference to the BFD must be
   released by the caller.  */

extern gdb_bfd_ref_ptr build_id_to_exec_bfd (size_t build_id_len,
					     const bfd_byte *build_id);

/* Find the separate debug file for OBJFILE, by using the build-id
   associated with OBJFILE's BFD.  If successful, returns the file name for the
   separate debug file, otherwise, return an empty string.  */

extern std::string find_separate_debug_file_by_buildid
  (struct objfile *objfile);

/* Return an hex-string representation of BUILD_ID.  */

static inline std::string
build_id_to_string (const bfd_build_id *build_id)
{
  gdb_assert (build_id != NULL);

  return bin2hex (build_id->data, build_id->size);
}

#endif /* BUILD_ID_H */
