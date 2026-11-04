/* DWARF DWZ handling for GDB.

   Copyright (C) 2003-2025 Free Software Foundation, Inc.

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

#include "dwarf2/dwz.h"

#include "build-id.h"
#include "debuginfod-support.h"
#include "dwarf2/leb.h"
#include "dwarf2/read.h"
#include "dwarf2/sect-names.h"
#include "filenames.h"
#include "gdb_bfd.h"
#include "gdbcore.h"
#include "gdbsupport/gdb_vecs.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/scoped_fd.h"
#include "run-on-main-thread.h"

const char *
dwz_file::read_string (struct objfile *objfile, LONGEST str_offset)
{
  /* This must be true because the sections are read in when the
     dwz_file is created.  */
  gdb_assert (str.readin);

  if (str.buffer == NULL)
    error (_("supplementary DWARF file missing .debug_str "
	     "section [in module %s]"),
	   this->filename ());
  if (str_offset >= str.size)
    error (_("invalid string reference to supplementary DWARF file "
	     "[in module %s]"),
	   this->filename ());
  gdb_assert (HOST_CHAR_BIT == 8);
  if (str.buffer[str_offset] == '\0')
    return NULL;
  return (const char *) (str.buffer + str_offset);
}

/* A helper function to find the sections for a .dwz file.  */

static void
locate_dwz_sections (objfile *objfile, dwz_file &dwz_file)
{
  for (asection *sec : gdb_bfd_sections (dwz_file.dwz_bfd))
    {
      dwarf2_section_info *sect = nullptr;

      /* Note that we only support the standard ELF names, because .dwz
     is ELF-only (at the time of writing).  */
      if (dwarf2_elf_names.abbrev.matches (sec->name))
	sect = &dwz_file.abbrev;
      else if (dwarf2_elf_names.info.matches (sec->name))
	sect = &dwz_file.info;
      else if (dwarf2_elf_names.str.matches (sec->name))
	sect = &dwz_file.str;
      else if (dwarf2_elf_names.line.matches (sec->name))
	sect = &dwz_file.line;
      else if (dwarf2_elf_names.macro.matches (sec->name))
	sect = &dwz_file.macro;
      else if (dwarf2_elf_names.gdb_index.matches (sec->name))
	sect = &dwz_file.gdb_index;
      else if (dwarf2_elf_names.debug_names.matches (sec->name))
	sect = &dwz_file.debug_names;
      else if (dwarf2_elf_names.types.matches (sec->name))
	sect = &dwz_file.types;

      if (sect != nullptr)
	{
	  sect->s.section = sec;
	  sect->size = bfd_section_size (sec);
	  sect->read (objfile);
	}
    }
}

/* Helper that throws an exception when reading the .debug_sup
   section.  */

static void
debug_sup_failure (const char *text, bfd *abfd)
{
  error (_("%s [in modules %s]"), text, bfd_get_filename (abfd));
}

/* Look for the .debug_sup section and read it.  If the section does
   not exist, this returns false.  If the section does exist but fails
   to parse for some reason, an exception is thrown.  Otherwise, if
   everything goes well, this returns true and fills in the out
   parameters.  */

static bool
get_debug_sup_info (bfd *abfd,
		    std::string *filename,
		    size_t *buildid_len,
		    gdb::unique_xmalloc_ptr<bfd_byte> *buildid)
{
  asection *sect = bfd_get_section_by_name (abfd, ".debug_sup");
  if (sect == nullptr)
    return false;

  bfd_byte *contents;
  if (!bfd_malloc_and_get_section (abfd, sect, &contents))
    debug_sup_failure (_("could not read .debug_sup section"), abfd);

  gdb::unique_xmalloc_ptr<bfd_byte> content_holder (contents);
  bfd_size_type size = bfd_section_size (sect);

  /* Version of this section.  */
  if (size < 4)
    debug_sup_failure (_(".debug_sup section too short"), abfd);
  unsigned int version = read_2_bytes (abfd, contents);
  contents += 2;
  size -= 2;
  if (version != 5)
    debug_sup_failure (_(".debug_sup has wrong version number"), abfd);

  /* Skip the is_supplementary value.  We already ensured there were
     enough bytes, above.  */
  ++contents;
  --size;

  /* The spec says that in the supplementary file, this must be \0,
     but it doesn't seem very important.  */
  const char *fname = (const char *) contents;
  size_t len = strlen (fname) + 1;
  if (filename != nullptr)
    *filename = fname;
  contents += len;
  size -= len;

  if (size == 0)
    debug_sup_failure (_(".debug_sup section missing ID"), abfd);

  unsigned int bytes_read;
  *buildid_len = read_unsigned_leb128 (abfd, contents, &bytes_read);
  contents += bytes_read;
  size -= bytes_read;

  if (size < *buildid_len)
    debug_sup_failure (_("extra data after .debug_sup section ID"), abfd);

  if (*buildid_len != 0)
    buildid->reset ((bfd_byte *) xmemdup (contents, *buildid_len,
					  *buildid_len));

  return true;
}

/* Validate that ABFD matches the given BUILDID.  If DWARF5 is true,
   then this is done by examining the .debug_sup data.  */

static bool
verify_id (bfd *abfd, size_t len, const bfd_byte *buildid, bool dwarf5)
{
  if (!bfd_check_format (abfd, bfd_object))
    return false;

  if (dwarf5)
    {
      size_t new_len;
      gdb::unique_xmalloc_ptr<bfd_byte> new_id;

      if (!get_debug_sup_info (abfd, nullptr, &new_len, &new_id))
	return false;
      return (len == new_len
	      && memcmp (buildid, new_id.get (), len) == 0);
    }
  else
    return build_id_verify (abfd, len, buildid);
}

/* Find either the .debug_sup or .gnu_debugaltlink section and return
   its contents.  Returns true on success and sets out parameters, or
   false if nothing is found.  */

static bool
read_alt_info (bfd *abfd, std::string *filename,
	       size_t *buildid_len,
	       gdb::unique_xmalloc_ptr<bfd_byte> *buildid,
	       bool *dwarf5)
{
  if (get_debug_sup_info (abfd, filename, buildid_len, buildid))
    {
      *dwarf5 = true;
      return true;
    }

  bfd_size_type buildid_len_arg;
  bfd_set_error (bfd_error_no_error);
  bfd_byte *buildid_out;
  gdb::unique_xmalloc_ptr<char> new_filename
    (bfd_get_alt_debug_link_info (abfd, &buildid_len_arg,
				  &buildid_out));
  if (new_filename == nullptr)
    {
      if (bfd_get_error () == bfd_error_no_error)
	return false;
      error (_("could not read '.gnu_debugaltlink' section: %s"),
	     bfd_errmsg (bfd_get_error ()));
    }
  *filename = new_filename.get ();

  *buildid_len = buildid_len_arg;
  buildid->reset (buildid_out);
  *dwarf5 = false;
  return true;
}

/* Attempt to find a .dwz file (whose full path is represented by
   FILENAME) in all of the specified debug file directories provided.

   Return the equivalent gdb_bfd_ref_ptr of the .dwz file found, or
   nullptr if it could not find anything.  */

static gdb_bfd_ref_ptr
dwz_search_other_debugdirs (std::string &filename, bfd_byte *buildid,
			    size_t buildid_len, bool dwarf5)
{
  /* Let's assume that the path represented by FILENAME has the
     "/.dwz/" subpath in it.  This is what (most) GNU/Linux
     distributions do, anyway.  */
  size_t dwz_pos = filename.find ("/.dwz/");

  if (dwz_pos == std::string::npos)
    return nullptr;

  /* This is an obvious assertion, but it's here more to educate
     future readers of this code that FILENAME at DWZ_POS *must*
     contain a directory separator.  */
  gdb_assert (IS_DIR_SEPARATOR (filename[dwz_pos]));

  gdb_bfd_ref_ptr dwz_bfd;
  std::vector<gdb::unique_xmalloc_ptr<char>> debugdir_vec
    = dirnames_to_char_ptr_vec (debug_file_directory.c_str ());

  for (const gdb::unique_xmalloc_ptr<char> &debugdir : debugdir_vec)
    {
      /* The idea is to iterate over the
	 debug file directories provided by the user and
	 replace the hard-coded path in the "filename" by each
	 debug-file-directory.

	 For example, suppose that filename is:

	   /usr/lib/debug/.dwz/foo.dwz

	 And suppose that we have "$HOME/bar" as the
	 debug-file-directory.  We would then adjust filename
	 to look like:

	   $HOME/bar/.dwz/foo.dwz

	 which would hopefully allow us to find the alt debug
	 file.  */
      std::string ddir = debugdir.get ();

      if (ddir.empty ())
	continue;

      /* Make sure the current debug-file-directory ends with a
	 directory separator.  This is needed because, if FILENAME
	 contains something like "/usr/lib/abcde/.dwz/foo.dwz" and
	 DDIR is "/usr/lib/abc", then could wrongfully skip it
	 below.  */
      if (!IS_DIR_SEPARATOR (ddir.back ()))
	ddir += SLASH_STRING;

      /* Check whether the beginning of FILENAME is DDIR.  If it is,
	 then we are dealing with a file which we already attempted to
	 open before, so we just skip it and continue processing the
	 remaining debug file directories.  */
      if (filename.size () > ddir.size ()
	  && filename.compare (0, ddir.size (), ddir) == 0)
	continue;

      /* Replace FILENAME's default debug-file-directory with
	 DDIR.  */
      std::string new_filename = ddir + &filename[dwz_pos + 1];

      dwz_bfd = gdb_bfd_open (new_filename.c_str (), gnutarget);

      if (dwz_bfd == nullptr)
	continue;

      if (!verify_id (dwz_bfd.get (), buildid_len, buildid, dwarf5))
	{
	  dwz_bfd.reset (nullptr);
	  continue;
	}

      /* Found it.  */
      break;
    }

  return dwz_bfd;
}

/* See dwz.h.  */

void
dwz_file::read_dwz_file (dwarf2_per_objfile *per_objfile)
{
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  /* This may query the user via the debuginfod support, so it may
     only be run in the main thread.  */
  gdb_assert (is_main_thread ());

  /* This should only be called once.  */
  gdb_assert (!per_bfd->dwz_file.has_value ());
  /* Set this early, so that on error it remains NULL.  */
  per_bfd->dwz_file.emplace (nullptr);

  size_t buildid_len;
  gdb::unique_xmalloc_ptr<bfd_byte> buildid;
  std::string filename;
  bool dwarf5;
  if (!read_alt_info (per_bfd->obfd, &filename, &buildid_len, &buildid,
		      &dwarf5))
    {
      /* Nothing found, nothing to do.  */
      return;
    }

  if (!IS_ABSOLUTE_PATH (filename.c_str ()))
    {
      gdb::unique_xmalloc_ptr<char> abs = gdb_realpath (per_bfd->filename ());

      filename = gdb_ldirname (abs.get ()) + SLASH_STRING + filename;
    }

  /* First try the file name given in the section.  If that doesn't
     work, try to use the build-id instead.  */
  gdb_bfd_ref_ptr dwz_bfd (gdb_bfd_open (filename.c_str (), gnutarget));
  if (dwz_bfd != NULL)
    {
      if (!verify_id (dwz_bfd.get (), buildid_len, buildid.get (), dwarf5))
	dwz_bfd.reset (nullptr);
    }

  if (dwz_bfd == NULL)
    dwz_bfd = build_id_to_debug_bfd (buildid_len, buildid.get ());

  if (dwz_bfd == nullptr)
    {
      /* If the user has provided us with different
	 debug file directories, we can try them in order.  */
      dwz_bfd = dwz_search_other_debugdirs (filename, buildid.get (),
					    buildid_len, dwarf5);
    }

  if (dwz_bfd == nullptr)
    {
      gdb::unique_xmalloc_ptr<char> alt_filename;
      scoped_fd fd
	= debuginfod_debuginfo_query (buildid.get (), buildid_len,
				      per_bfd->filename (), &alt_filename);

      if (fd.get () >= 0)
	{
	  /* File successfully retrieved from server.  */
	  dwz_bfd = gdb_bfd_open (alt_filename.get (), gnutarget);

	  if (dwz_bfd == nullptr)
	    warning (_("File \"%s\" from debuginfod cannot be opened as bfd"),
		     alt_filename.get ());
	  else if (!verify_id (dwz_bfd.get (), buildid_len, buildid.get (),
			       dwarf5))
	    dwz_bfd.reset (nullptr);
	}
    }

  if (dwz_bfd == NULL)
    error (_("could not find supplementary DWARF file (%s) for %s"),
	   filename.c_str (),
	   per_bfd->filename ());

  dwz_file_up result (new dwz_file (std::move (dwz_bfd)));

  locate_dwz_sections (per_objfile->objfile, *result);

  gdb_bfd_record_inclusion (per_bfd->obfd, result->dwz_bfd.get ());
  bfd_cache_close (result->dwz_bfd.get ());

  per_bfd->dwz_file = std::move (result);
}
