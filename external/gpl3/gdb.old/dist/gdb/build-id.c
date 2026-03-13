/* build-id-related functions.

   Copyright (C) 1991-2024 Free Software Foundation, Inc.

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

#include "bfd.h"
#include "gdb_bfd.h"
#include "build-id.h"
#include "gdbsupport/gdb_vecs.h"
#include "symfile.h"
#include "objfiles.h"
#include "filenames.h"
#include "gdbcore.h"
#include "cli/cli-style.h"
#include "gdbsupport/scoped_fd.h"
#include "debuginfod-support.h"
#include "extension.h"

/* See build-id.h.  */

const struct bfd_build_id *
build_id_bfd_get (bfd *abfd)
{
  /* Dynamic objfiles such as ones created by JIT reader API
     have no underlying bfd structure (that is, objfile->obfd
     is NULL).  */
  if (abfd == nullptr)
    return nullptr;

  if (!bfd_check_format (abfd, bfd_object)
      && !bfd_check_format (abfd, bfd_core))
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
    warning (_("File \"%ps\" has no build-id, file skipped"),
	     styled_string (file_name_style.style (),
			    bfd_get_filename (abfd)));
  else if (!build_id_equal (found, check_len, check))
    warning (_("File \"%ps\" has a different build-id, file skipped"),
	     styled_string (file_name_style.style (),
			    bfd_get_filename (abfd)));
  else
    retval = 1;

  return retval;
}

/* Helper for build_id_to_debug_bfd.  ORIGINAL_LINK with SUFFIX appended is
   a path to a potential build-id-based separate debug file, potentially a
   symlink to the real file.  If the file exists and matches BUILD_ID,
   return a BFD reference to it.  */

static gdb_bfd_ref_ptr
build_id_to_debug_bfd_1 (const std::string &original_link,
			 size_t build_id_len, const bfd_byte *build_id,
			 const char *suffix)
{
  tribool supports_target_stat = TRIBOOL_UNKNOWN;

  /* Drop the 'target:' prefix if the target filesystem is local.  */
  std::string_view original_link_view (original_link);
  if (is_target_filename (original_link) && target_filesystem_is_local ())
    original_link_view
      = original_link_view.substr (strlen (TARGET_SYSROOT_PREFIX));

  /* The upper bound of '10' here is completely arbitrary.  The loop should
     terminate via 'break' when either (a) a readable symlink is found, or
     (b) a non-existing entry is found.

     However, for remote targets, we rely on the remote returning sane
     error codes.  If a remote sends back the wrong error code then it
     might trick GDB into thinking that the symlink exists, but points to a
     missing file, in which case GDB will try the next seqno.  We don't
     want a broken remote to cause GDB to spin here forever, hence a fixed
     upper bound.  */

  for (unsigned seqno = 0; seqno < 10; seqno++)
    {
      std::string link (original_link_view);

      if (seqno > 0)
	string_appendf (link, ".%u", seqno);

      link += suffix;

      separate_debug_file_debug_printf ("Trying %s...", link.c_str ());

      gdb::unique_xmalloc_ptr<char> filename_holder;
      const char *filename = nullptr;
      if (is_target_filename (link))
	{
	  gdb_assert (link.length () >= strlen (TARGET_SYSROOT_PREFIX));
	  const char *link_on_target
	    = link.c_str () + strlen (TARGET_SYSROOT_PREFIX);

	  fileio_error target_errno;
	  if (supports_target_stat != TRIBOOL_FALSE)
	    {
	      struct stat sb;
	      int res = target_fileio_stat (nullptr, link_on_target, &sb,
					    &target_errno);

	      if (res != 0 && target_errno != FILEIO_ENOSYS)
		{
		  separate_debug_file_debug_printf ("path doesn't exist");
		  break;
		}
	      else if (res != 0 && target_errno == FILEIO_ENOSYS)
		supports_target_stat = TRIBOOL_FALSE;
	      else
		{
		  supports_target_stat = TRIBOOL_TRUE;
		  filename = link.c_str ();
		}
	    }

	  if (supports_target_stat == TRIBOOL_FALSE)
	    {
	      gdb_assert (filename == nullptr);

	      /* Connecting to a target that doesn't support 'stat'.  Try
		 'readlink' as an alternative.  This isn't ideal, but is
		 maybe better than nothing.  Returns EINVAL if the path
		 isn't a symbolic link, which hints that the path is
		 available -- there are other errors e.g. ENOENT for when
		 the path doesn't exist, but we just assume that anything
		 other than EINVAL indicates the path doesn't exist.  */
	      std::optional<std::string> link_target
		= target_fileio_readlink (nullptr, link_on_target,
					  &target_errno);
	      if (link_target.has_value ()
		  || target_errno == FILEIO_EINVAL)
		filename = link.c_str ();
	      else
		{
		  separate_debug_file_debug_printf ("path doesn't exist");
		  break;
		}
	    }
	}
      else
	{
	  struct stat buf;

	  /* The `access' call below automatically dereferences LINK, but
	     we want to stop incrementing SEQNO once we find a symlink
	     that doesn't exist.  */
	  if (lstat (link.c_str (), &buf) != 0)
	    {
	      separate_debug_file_debug_printf ("path doesn't exist");
	      break;
	    }

	  /* Can LINK be accessed, or if LINK is a symlink, can the file
	     pointed too be accessed?  Do this as lrealpath() is
	     expensive, even for the usually non-existent files.  */
	  if (access (link.c_str (), F_OK) == 0)
	    {
	      filename_holder.reset (lrealpath (link.c_str ()));
	      filename = filename_holder.get ();
	    }
	}

      if (filename == nullptr)
	{
	  separate_debug_file_debug_printf ("unable to compute real path");
	  continue;
	}

      /* We expect to be silent on the non-existing files.  */
      gdb_bfd_ref_ptr debug_bfd = gdb_bfd_open (filename, gnutarget);

      if (debug_bfd == NULL)
	{
	  separate_debug_file_debug_printf ("unable to open `%s`", filename);
	  continue;
	}

      if (!build_id_verify (debug_bfd.get(), build_id_len, build_id))
	{
	  separate_debug_file_debug_printf ("build-id does not match");
	  continue;
	}

      separate_debug_file_debug_printf ("found a match");
      return debug_bfd;
    }

  separate_debug_file_debug_printf ("no suitable file found");
  return {};
}

/* Common code for finding BFDs of a given build-id.  This function
   works with both debuginfo files (SUFFIX == ".debug") and executable
   files (SUFFIX == "").

   The build-id will be split into a single byte sub-directory, followed by
   the remaining build-id bytes as the filename, i.e. we use the lookup
   format: `.build-id/xx/yy....zz`.  As a consequence, if BUILD_ID_LEN is
   less than 2 (bytes), no results will be found as there are not enough
   bytes to form the `yy....zz` part of the lookup filename.  */

static gdb_bfd_ref_ptr
build_id_to_bfd_suffix (size_t build_id_len, const bfd_byte *build_id,
			const char *suffix)
{
  SEPARATE_DEBUG_FILE_SCOPED_DEBUG_ENTER_EXIT;

  if (build_id_len < 2)
    {
      /* Zero length build-ids are ignored by bfd.  */
      gdb_assert (build_id_len > 0);
      separate_debug_file_debug_printf
	("Ignoring short build-id `%s' for build-id based lookup",
	 bin2hex (build_id, build_id_len).c_str ());
      return {};
    }

  /* Keep backward compatibility so that DEBUG_FILE_DIRECTORY being "" will
     cause "/.build-id/..." lookups.  */

  std::vector<gdb::unique_xmalloc_ptr<char>> debugdir_vec
    = dirnames_to_char_ptr_vec (debug_file_directory.c_str ());

  for (const gdb::unique_xmalloc_ptr<char> &debugdir : debugdir_vec)
    {
      const gdb_byte *data = build_id;
      size_t size = build_id_len;

      /* Compute where the file named after the build-id would be.

	 If debugdir is "/usr/lib/debug" and the build-id is abcdef, this will
	 give "/usr/lib/debug/.build-id/ab/cdef.debug".  */
      std::string link = debugdir.get ();
      link += "/.build-id/";

      gdb_assert (size > 1);
      size--;
      string_appendf (link, "%02x/", (unsigned) *data++);

      while (size-- > 0)
	string_appendf (link, "%02x", (unsigned) *data++);

      gdb_bfd_ref_ptr debug_bfd
	= build_id_to_debug_bfd_1 (link, build_id_len, build_id, suffix);
      if (debug_bfd != NULL)
	return debug_bfd;

      /* Try to look under the sysroot as well.  If the sysroot is
	 "/the/sysroot", it will give
	 "/the/sysroot/usr/lib/debug/.build-id/ab/cdef.debug".

	 If the sysroot is 'target:' and the target filesystem is local to
	 GDB then 'target:/path/to/check' becomes '/path/to/check' which
	 we just checked above.  */

      if (!gdb_sysroot.empty ()
	  && (gdb_sysroot != TARGET_SYSROOT_PREFIX
	      || !target_filesystem_is_local ()))
	{
	  link = gdb_sysroot + link;
	  debug_bfd = build_id_to_debug_bfd_1 (link, build_id_len, build_id,
					       suffix);
	  if (debug_bfd != NULL)
	    return debug_bfd;
	}
    }

  return {};
}

/* See build-id.h.  */

gdb_bfd_ref_ptr
build_id_to_debug_bfd (size_t build_id_len, const bfd_byte *build_id)
{
  return build_id_to_bfd_suffix (build_id_len, build_id, ".debug");
}

/* Find and open a BFD for an executable file given a build-id.  If no BFD
   can be found, return NULL.  The returned reference to the BFD must be
   released by the caller.  */

static gdb_bfd_ref_ptr
build_id_to_exec_bfd (size_t build_id_len, const bfd_byte *build_id)
{
  return build_id_to_bfd_suffix (build_id_len, build_id, "");
}

/* See build-id.h.  */

std::string
find_separate_debug_file_by_buildid (struct objfile *objfile,
				     deferred_warnings *warnings)
{
  const struct bfd_build_id *build_id;

  build_id = build_id_bfd_get (objfile->obfd.get ());
  if (build_id != NULL)
    {
      SEPARATE_DEBUG_FILE_SCOPED_DEBUG_START_END
	("looking for separate debug info (build-id) for %s",
	 objfile_name (objfile));

      gdb_bfd_ref_ptr abfd (build_id_to_debug_bfd (build_id->size,
						   build_id->data));
      /* Prevent looping on a stripped .debug file.  */
      if (abfd != NULL
	  && filename_cmp (bfd_get_filename (abfd.get ()),
			   objfile_name (objfile)) == 0)
	{
	  separate_debug_file_debug_printf
	    ("\"%s\": separate debug info file has no debug info",
	     bfd_get_filename (abfd.get ()));
	  warnings->warn (_("\"%ps\": separate debug info file has no "
			    "debug info"),
			  styled_string (file_name_style.style (),
					 bfd_get_filename (abfd.get ())));
	}
      else if (abfd != NULL)
	return std::string (bfd_get_filename (abfd.get ()));
    }

  return std::string ();
}

/* See build-id.h.  */

gdb_bfd_ref_ptr
find_objfile_by_build_id (program_space *pspace,
			  const bfd_build_id *build_id,
			  const char *expected_filename)
{
  gdb_bfd_ref_ptr abfd;

  for (unsigned attempt = 0, max_attempts = 1;
       attempt < max_attempts && abfd == nullptr;
       ++attempt)
    {
      /* Try to find the executable (or shared object) by looking for a
	 (sym)link on disk from the build-id to the object file.  */
      abfd = build_id_to_exec_bfd (build_id->size, build_id->data);

      if (abfd != nullptr || attempt > 0)
	break;

      /* Attempt to query debuginfod for the executable.  This will only
	 get run during the first attempt, if an extension language hook
	 (see below) asked for a second attempt then we will have already
	 broken out of the loop above.  */
      gdb::unique_xmalloc_ptr<char> path;
      scoped_fd fd = debuginfod_exec_query (build_id->data, build_id->size,
					    expected_filename, &path);
      if (fd.get () >= 0)
	{
	  abfd = gdb_bfd_open (path.get (), gnutarget);

	  if (abfd == nullptr)
	    warning (_("\"%ps\" from debuginfod cannot be opened as bfd: %s"),
		     styled_string (file_name_style.style (), path.get ()),
		     gdb_bfd_errmsg (bfd_get_error (), nullptr).c_str ());
	  else if (!build_id_verify (abfd.get (), build_id->size,
				     build_id->data))
	    abfd = nullptr;
	}

      if (abfd != nullptr)
	break;

      ext_lang_missing_file_result ext_result
	= ext_lang_find_objfile_from_buildid (pspace, build_id,
					      expected_filename);
      if (!ext_result.filename ().empty ())
	{
	  /* The extension identified the file for us.  */
	  abfd = gdb_bfd_open (ext_result.filename ().c_str (), gnutarget);
	  if (abfd == nullptr)
	    {
	      warning (_("\"%ps\" from extension cannot be opened as bfd: %s"),
		       styled_string (file_name_style.style (),
				      ext_result.filename ().c_str ()),
		       gdb_bfd_errmsg (bfd_get_error (), nullptr).c_str ());
	      break;
	    }

	  /* If the extension gave us a path to a file then we always
	     assume that it is the correct file, we do no additional check
	     of its build-id.  */
	}
      else if (ext_result.try_again ())
	{
	  /* The extension might have installed the file in the expected
	     location, we should try again.  */
	  max_attempts = 2;
	  continue;
	}
    }

  return abfd;
}
