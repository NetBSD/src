/* build-id-related functions.

   Copyright (C) 1991-2017 Free Software Foundation, Inc.

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

#include "defs.h"
#include "bfd.h"
#include "gdb_bfd.h"
#include "build-id.h"
#include "gdb_vecs.h"
#include "symfile.h"
#include "objfiles.h"
#include "filenames.h"
#include "gdbcore.h"

/* See build-id.h.  */

const struct bfd_build_id *
build_id_bfd_get (bfd *abfd)
{
  if (!bfd_check_format (abfd, bfd_object))
    return NULL;

  if (abfd->build_id != NULL)
    return abfd->build_id;

  /* No build-id */
  return NULL;
}

/* See build-id.h.  */

int
build_id_verify (bfd *abfd, size_t check_len, const bfd_byte *check)
{
  const struct bfd_build_id *found;
  int retval = 0;

  found = build_id_bfd_get (abfd);

  if (found == NULL)
    warning (_("File \"%s\" has no build-id, file skipped"),
	     bfd_get_filename (abfd));
  else if (found->size != check_len
           || memcmp (found->data, check, found->size) != 0)
    warning (_("File \"%s\" has a different build-id, file skipped"),
	     bfd_get_filename (abfd));
  else
    retval = 1;

  return retval;
}

/* See build-id.h.  */

gdb_bfd_ref_ptr
build_id_to_debug_bfd (size_t build_id_len, const bfd_byte *build_id)
{
  char *link, *debugdir;
  VEC (char_ptr) *debugdir_vec;
  struct cleanup *back_to;
  int ix;
  gdb_bfd_ref_ptr abfd;
  int alloc_len;

  /* DEBUG_FILE_DIRECTORY/.build-id/ab/cdef */
  alloc_len = (strlen (debug_file_directory)
	       + (sizeof "/.build-id/" - 1) + 1
	       + 2 * build_id_len + (sizeof ".debug" - 1) + 1);
  link = (char *) alloca (alloc_len);

  /* Keep backward compatibility so that DEBUG_FILE_DIRECTORY being "" will
     cause "/.build-id/..." lookups.  */

  debugdir_vec = dirnames_to_char_ptr_vec (debug_file_directory);
  back_to = make_cleanup_free_char_ptr_vec (debugdir_vec);

  for (ix = 0; VEC_iterate (char_ptr, debugdir_vec, ix, debugdir); ++ix)
    {
      size_t debugdir_len = strlen (debugdir);
      const gdb_byte *data = build_id;
      size_t size = build_id_len;
      char *s;
      char *filename = NULL;
      struct cleanup *inner;

      memcpy (link, debugdir, debugdir_len);
      s = &link[debugdir_len];
      s += sprintf (s, "/.build-id/");
      if (size > 0)
	{
	  size--;
	  s += sprintf (s, "%02x", (unsigned) *data++);
	}
      if (size > 0)
	*s++ = '/';
      while (size-- > 0)
	s += sprintf (s, "%02x", (unsigned) *data++);
      strcpy (s, ".debug");

      /* lrealpath() is expensive even for the usually non-existent files.  */
      if (access (link, F_OK) == 0)
	filename = lrealpath (link);

      if (filename == NULL)
	continue;

      /* We expect to be silent on the non-existing files.  */
      inner = make_cleanup (xfree, filename);
      abfd = gdb_bfd_open (filename, gnutarget, -1);
      do_cleanups (inner);

      if (abfd == NULL)
	continue;

      if (build_id_verify (abfd.get(), build_id_len, build_id))
	break;

      abfd.release ();
    }

  do_cleanups (back_to);
  return abfd;
}

/* See build-id.h.  */

char *
find_separate_debug_file_by_buildid (struct objfile *objfile)
{
  const struct bfd_build_id *build_id;

  build_id = build_id_bfd_get (objfile->obfd);
  if (build_id != NULL)
    {
      gdb_bfd_ref_ptr abfd (build_id_to_debug_bfd (build_id->size,
						   build_id->data));
      /* Prevent looping on a stripped .debug file.  */
      if (abfd != NULL
	  && filename_cmp (bfd_get_filename (abfd.get ()),
			   objfile_name (objfile)) == 0)
	warning (_("\"%s\": separate debug info file has no debug info"),
		 bfd_get_filename (abfd.get ()));
      else if (abfd != NULL)
	return xstrdup (bfd_get_filename (abfd.get ()));
    }
  return NULL;
}
