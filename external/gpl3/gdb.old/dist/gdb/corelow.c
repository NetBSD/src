/* Core dump and executable file functions below target vector, for GDB.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#include "arch-utils.h"
#include <signal.h>
#include <fcntl.h>
#include "exceptions.h"
#include "frame.h"
#include "inferior.h"
#include "infrun.h"
#include "symtab.h"
#include "command.h"
#include "bfd.h"
#include "target.h"
#include "process-stratum-target.h"
#include "gdbcore.h"
#include "gdbthread.h"
#include "regcache.h"
#include "regset.h"
#include "symfile.h"
#include "exec.h"
#include "readline/tilde.h"
#include "solib.h"
#include "solist.h"
#include "filenames.h"
#include "progspace.h"
#include "objfiles.h"
#include "gdb_bfd.h"
#include "completer.h"
#include "gdbsupport/filestuff.h"
#include "build-id.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/scoped_fd.h"
#include "gdbsupport/x86-xstate.h"
#include <unordered_map>
#include <unordered_set>
#include "cli/cli-cmds.h"
#include "xml-tdesc.h"
#include "memtag.h"
#include "cli/cli-style.h"

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

/* Forward declarations.  */

static void core_target_open (const char *arg, int from_tty);

/* A mem_range and the build-id associated with the file mapped into the
   given range.  */

struct mem_range_and_build_id
{
  mem_range_and_build_id (mem_range &&r, const bfd_build_id *id)
    : range (r),
      build_id (id)
  { /* Nothing.  */ }

  /* A range of memory addresses.  */
  mem_range range;

  /* The build-id of the file mapped into RANGE.  */
  const bfd_build_id *build_id;
};

/* An instance of this class is created within the core_target and is used
   to hold all the information that relating to mapped files, their address
   ranges, and their corresponding build-ids.  */

struct mapped_file_info
{
  /* See comment on function definition.  */

  void add (const char *soname, const char *expected_filename,
	    const char *actual_filename, std::vector<mem_range> &&ranges,
	    const bfd_build_id *build_id);

  /* See comment on function definition.  */

  std::optional <core_target_mapped_file_info>
  lookup (const char *filename, const std::optional<CORE_ADDR> &addr);

private:

  /* Helper for ::lookup.  BUILD_ID is a build-id that was found in
     one of the data structures within this class.  Lookup the
     corresponding filename in m_build_id_to_filename_map and return a pair
     containing the build-id and filename.

     If no corresponding filename is found in m_build_id_to_filename_map
     then the returned pair contains BUILD_ID and an empty string.

     If BUILD_ID is nullptr then the returned pair contains nullptr and an
     empty string.  */

  struct core_target_mapped_file_info
  make_result (const bfd_build_id *build_id)
  {
    if (build_id != nullptr)
      {
      auto it = m_build_id_to_filename_map.find (build_id);
      if (it != m_build_id_to_filename_map.end ())
	return { build_id, it->second };
    }

    return { build_id, {} };
  }

  /* A type that maps a string to a build-id.  */
  using string_to_build_id_map
    = std::unordered_map<std::string, const bfd_build_id *>;

  /* A type that maps a build-id to a string.  */
  using build_id_to_string_map
    = std::unordered_map<const bfd_build_id *, std::string>;

  /* When loading a core file, the build-ids are extracted based on the
     file backed mappings.  This map associates the name of a file that was
     mapped into the core file with the corresponding build-id.  The
     build-id pointers in this map will never be nullptr as we only record
     files if they have a build-id.  */

  string_to_build_id_map m_filename_to_build_id_map;

  /* Map a build-id pointer back to the name of the file that was mapped
     into the inferior's address space.  If we lookup a matching build-id
     using either a soname or an address then this map allows us to also
     provide a full path to a file with a matching build-id.  */

  build_id_to_string_map m_build_id_to_filename_map;

  /* If the file that was mapped into the core file was a shared library
     then it might have a DT_SONAME tag in its .dynamic section, this tag
     contains the name of a shared object.  When opening a shared library,
     if it's basename appears in this map then we can use the corresponding
     build-id.

     In the rare case that two different files have the same DT_SONAME
     value then the build-id pointer in this map will be nullptr, this
     indicates that it's not possible to find a build-id based on the given
     DT_SONAME value.  */

  string_to_build_id_map m_soname_to_build_id_map;

  /* This vector maps memory ranges onto an associated build-id.  The
     ranges are those of the files mapped into the core file.

     Entries in this vector must not overlap, and are sorted be increasing
     memory address.  Within each entry the build-id pointer will not be
     nullptr.

     While building this vector the entries are not sorted, they are
     sorted once after the table has finished being built.  */

  std::vector<mem_range_and_build_id> m_address_to_build_id_list;

  /* False if address_to_build_id_list is unsorted, otherwise true.  */

  bool m_address_to_build_id_list_sorted = false;
};

/* The core file target.  */

static const target_info core_target_info = {
  "core",
  N_("Local core dump file"),
  N_("Use a core file as a target.\n\
Specify the filename of the core file.")
};

class core_target final : public process_stratum_target
{
public:
  core_target ();

  const target_info &info () const override
  { return core_target_info; }

  void close () override;
  void detach (inferior *, int) override;
  void fetch_registers (struct regcache *, int) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;
  void files_info () override;

  bool thread_alive (ptid_t ptid) override;
  const struct target_desc *read_description () override;

  std::string pid_to_str (ptid_t) override;

  const char *thread_name (struct thread_info *) override;

  bool has_all_memory () override { return true; }
  bool has_memory () override;
  bool has_stack () override;
  bool has_registers () override;
  bool has_execution (inferior *inf) override { return false; }

  bool info_proc (const char *, enum info_proc_what) override;

  bool supports_memory_tagging () override;

  /* Core file implementation of fetch_memtags.  Fetch the memory tags from
     core file notes.  */
  bool fetch_memtags (CORE_ADDR address, size_t len,
		      gdb::byte_vector &tags, int type) override;

  /* If the architecture supports it, check if ADDRESS is within a memory range
     mapped with tags.  For example,  MTE tags for AArch64.  */
  bool is_address_tagged (gdbarch *gdbarch, CORE_ADDR address) override;

  x86_xsave_layout fetch_x86_xsave_layout () override;

  /* A few helpers.  */

  /* Getter, see variable definition.  */
  struct gdbarch *core_gdbarch ()
  {
    return m_core_gdbarch;
  }

  /* See definition.  */
  void get_core_register_section (struct regcache *regcache,
				  const struct regset *regset,
				  const char *name,
				  int section_min_size,
				  const char *human_name,
				  bool required);

  /* See definition.  */
  void info_proc_mappings (struct gdbarch *gdbarch);

  std::optional <core_target_mapped_file_info>
  lookup_mapped_file_info (const char *filename,
			   const std::optional<CORE_ADDR> &addr)
  {
    return m_mapped_file_info.lookup (filename, addr);
  }

  /* Return a string containing the expected executable filename obtained
     from the mapped file information within the core file.  The filename
     returned will be for the mapped file whose ELF headers are mapped at
     the lowest address (i.e. which GDB encounters first).

     If no suitable filename can be found then the returned string will be
     empty.

     If there are no build-ids embedded into the core file then the
     returned string will be empty.

     If a non-empty string is returned then there is no guarantee that the
     named file exists on disk, or if it does exist on disk, then the
     on-disk file might have a different build-id to the desired
     build-id.  */
  const std::string &
  expected_exec_filename () const
  {
    return m_expected_exec_filename;
  }

private: /* per-core data */

  /* Get rid of the core inferior.  */
  void clear_core ();

  /* The core's section table.  Note that these target sections are
     *not* mapped in the current address spaces' set of target
     sections --- those should come only from pure executable or
     shared library bfds.  The core bfd sections are an implementation
     detail of the core target, just like ptrace is for unix child
     targets.  */
  std::vector<target_section> m_core_section_table;

  /* File-backed address space mappings: some core files include
     information about memory mapped files.  */
  std::vector<target_section> m_core_file_mappings;

  /* Unavailable mappings.  These correspond to pathnames which either
     weren't found or could not be opened.  Knowing these addresses can
     still be useful.  */
  std::vector<mem_range> m_core_unavailable_mappings;

  /* Data structure that holds information mapping filenames and address
     ranges to the corresponding build-ids as well as the reverse build-id
     to filename mapping.  */
  mapped_file_info m_mapped_file_info;

  /* Build m_core_file_mappings and m_mapped_file_info.  Called from the
     constructor.  */
  void build_file_mappings ();

  /* FIXME: kettenis/20031023: Eventually this field should
     disappear.  */
  struct gdbarch *m_core_gdbarch = NULL;

  /* If not empty then this contains the name of the executable discovered
     when processing the memory-mapped file information.  This will only
     be set if we find a mapped with a suitable build-id.  */
  std::string m_expected_exec_filename;
};

core_target::core_target ()
{
  /* Find a first arch based on the BFD.  We need the initial gdbarch so
     we can setup the hooks to find a target description.  */
  m_core_gdbarch = gdbarch_from_bfd (current_program_space->core_bfd ());

  /* If the arch is able to read a target description from the core, it
     could yield a more specific gdbarch.  */
  const struct target_desc *tdesc = read_description ();

  if (tdesc != nullptr)
    {
      struct gdbarch_info info;
      info.abfd = current_program_space->core_bfd ();
      info.target_desc = tdesc;
      m_core_gdbarch = gdbarch_find_by_info (info);
    }

  if (!m_core_gdbarch
      || !gdbarch_iterate_over_regset_sections_p (m_core_gdbarch))
    error (_("\"%s\": Core file format not supported"),
	   bfd_get_filename (current_program_space->core_bfd ()));

  /* Find the data section */
  m_core_section_table = build_section_table (current_program_space->core_bfd ());

  build_file_mappings ();
}

/* Construct the table for file-backed mappings if they exist.

   For each unique path in the note, we'll open a BFD with a bfd
   target of "binary".  This is an unstructured bfd target upon which
   we'll impose a structure from the mappings in the architecture-specific
   mappings note.  A BFD section is allocated and initialized for each
   file-backed mapping.

   We take care to not share already open bfds with other parts of
   GDB; in particular, we don't want to add new sections to existing
   BFDs.  We do, however, ensure that the BFDs that we allocate here
   will go away (be deallocated) when the core target is detached.  */

void
core_target::build_file_mappings ()
{
  /* Type holding information about a single file mapped into the inferior
     at the point when the core file was created.  Associates a build-id
     with the list of regions the file is mapped into.  */
  struct mapped_file
  {
    /* Type for a region of a file that was mapped into the inferior when
       the core file was generated.  */
    struct region
    {
      /* Constructor.   See member variables for argument descriptions.  */
      region (CORE_ADDR start_, CORE_ADDR end_, CORE_ADDR file_ofs_)
	: start (start_),
	  end (end_),
	  file_ofs (file_ofs_)
      { /* Nothing.  */ }

      /* The inferior address for the start of the mapped region.  */
      CORE_ADDR start;

      /* The inferior address immediately after the mapped region.  */
      CORE_ADDR end;

      /* The offset within the mapped file for this content.  */
      CORE_ADDR file_ofs;
    };

    /* If not nullptr, then this is the build-id associated with this
       file.  */
    const bfd_build_id *build_id = nullptr;

    /* If true then we have seen multiple different build-ids associated
       with the same filename.  The build_id field will have been set back
       to nullptr, and we should not set build_id in future.  */
    bool ignore_build_id_p = false;

    /* All the mapped regions of this file.  */
    std::vector<region> regions;
  };

  std::unordered_map<std::string, struct bfd *> bfd_map;
  std::unordered_set<std::string> unavailable_paths;

  /* All files mapped into the core file.  The key is the filename.  */
  std::unordered_map<std::string, mapped_file> mapped_files;

  /* See linux_read_core_file_mappings() in linux-tdep.c for an example
     read_core_file_mappings method.  */
  gdbarch_read_core_file_mappings (m_core_gdbarch,
				   current_program_space->core_bfd (),

    /* After determining the number of mappings, read_core_file_mappings
       will invoke this lambda.  */
    [&] (ULONGEST)
      {
      },

    /* read_core_file_mappings will invoke this lambda for each mapping
       that it finds.  */
    [&] (int num, ULONGEST start, ULONGEST end, ULONGEST file_ofs,
	 const char *filename, const bfd_build_id *build_id)
      {
	/* Architecture-specific read_core_mapping methods are expected to
	   weed out non-file-backed mappings.  */
	gdb_assert (filename != nullptr);

	/* Add this mapped region to the data for FILENAME.  */
	mapped_file &file_data = mapped_files[filename];
	file_data.regions.emplace_back (start, end, file_ofs);
	if (build_id != nullptr && !file_data.ignore_build_id_p)
	  {
	    if (file_data.build_id == nullptr)
	      file_data.build_id = build_id;
	    else if (!build_id_equal (build_id, file_data.build_id))
	      {
		warning (_("Multiple build-ids found for %ps"),
			 styled_string (file_name_style.style (), filename));
		file_data.build_id = nullptr;
		file_data.ignore_build_id_p = true;
	      }
	  }
      });

  /* Get the build-id of the core file.  */
  const bfd_build_id *core_build_id
    = build_id_bfd_get (current_program_space->core_bfd ());

  for (const auto &iter : mapped_files)
    {
      const std::string &filename = iter.first;
      const mapped_file &file_data = iter.second;

      /* If this mapped file has the same build-id as was discovered for
	 the core-file itself, then we assume this is the main
	 executable.  Record the filename as we can use this later.  */
      if (file_data.build_id != nullptr
	  && m_expected_exec_filename.empty ()
	  && build_id_equal (file_data.build_id, core_build_id))
	m_expected_exec_filename = filename;

      /* Use exec_file_find() to do sysroot expansion.  It'll
	 also strip the potential sysroot "target:" prefix.  If
	 there is no sysroot, an equivalent (possibly more
	 canonical) pathname will be provided.  */
      gdb::unique_xmalloc_ptr<char> expanded_fname
	= exec_file_find (filename.c_str (), nullptr);

      bool build_id_mismatch = false;
      if (expanded_fname != nullptr && file_data.build_id != nullptr)
	{
	  /* We temporarily open the bfd as a structured target, this
	     allows us to read the build-id from the bfd if there is one.
	     For this task it's OK if we reuse an already open bfd object,
	     so we make this call through GDB's bfd cache.  Once we've
	     checked the build-id (if there is one) we'll drop this
	     reference and re-open the bfd using the "binary" target.  */
	  gdb_bfd_ref_ptr tmp_bfd
	    = gdb_bfd_open (expanded_fname.get (), gnutarget);

	  if (tmp_bfd != nullptr
	      && bfd_check_format (tmp_bfd.get (), bfd_object)
	      && build_id_bfd_get (tmp_bfd.get ()) != nullptr)
	    {
	      /* The newly opened TMP_BFD has a build-id, and this mapped
		 file has a build-id extracted from the core-file.  Check
		 the build-id's match, and if not, reject TMP_BFD.  */
	      const struct bfd_build_id *found
		= build_id_bfd_get (tmp_bfd.get ());
	      if (!build_id_equal (found, file_data.build_id))
		build_id_mismatch = true;
	    }
	}

      gdb_bfd_ref_ptr abfd;
      if (expanded_fname != nullptr && !build_id_mismatch)
	{
	  struct bfd *b = bfd_openr (expanded_fname.get (), "binary");
	  abfd = gdb_bfd_ref_ptr::new_reference (b);
	}

      if ((expanded_fname == nullptr
	   || abfd == nullptr
	   || !bfd_check_format (abfd.get (), bfd_object))
	  && file_data.build_id != nullptr)
	{
	  abfd = find_objfile_by_build_id (current_program_space,
					   file_data.build_id,
					   filename.c_str ());

	  if (abfd != nullptr)
	    {
	      /* The find_objfile_by_build_id will have opened ABFD using
		 the GNUTARGET global bfd type, however, we need the bfd
		 opened as the binary type (see the function's header
		 comment), so now we reopen ABFD with the desired binary
		 type.  */
	      expanded_fname
		= make_unique_xstrdup (bfd_get_filename (abfd.get ()));
	      struct bfd *b = bfd_openr (expanded_fname.get (), "binary");
	      gdb_assert (b != nullptr);
	      abfd = gdb_bfd_ref_ptr::new_reference (b);
	    }
	}

      std::vector<mem_range> ranges;
      for (const mapped_file::region &region : file_data.regions)
	ranges.emplace_back (region.start, region.end - region.start);

      if (expanded_fname == nullptr
	  || abfd == nullptr
	  || !bfd_check_format (abfd.get (), bfd_object))
	{
	  /* If ABFD was opened, but the wrong format, close it now.  */
	  abfd = nullptr;

	  /* Record all regions for this file as unavailable.  */
	  for (const mapped_file::region &region : file_data.regions)
	    m_core_unavailable_mappings.emplace_back (region.start,
						      region.end
						      - region.start);

	  /* And give the user an appropriate warning.  */
	  if (build_id_mismatch)
	    {
	      if (expanded_fname == nullptr
		  || filename == expanded_fname.get ())
		warning (_("File %ps doesn't match build-id from core-file "
			   "during file-backed mapping processing"),
			 styled_string (file_name_style.style (),
					filename.c_str ()));
	      else
		warning (_("File %ps which was expanded to %ps, doesn't match "
			   "build-id from core-file during file-backed "
			   "mapping processing"),
			 styled_string (file_name_style.style (),
					filename.c_str ()),
			 styled_string (file_name_style.style (),
					expanded_fname.get ()));
	    }
	  else
	    {
	      if (expanded_fname == nullptr
		  || filename == expanded_fname.get ())
		warning (_("Can't open file %ps during file-backed mapping "
			   "note processing"),
			 styled_string (file_name_style.style (),
					filename.c_str ()));
	      else
		warning (_("Can't open file %ps which was expanded to %ps "
			   "during file-backed mapping note processing"),
			 styled_string (file_name_style.style (),
					filename.c_str ()),
			 styled_string (file_name_style.style (),
					expanded_fname.get ()));
	    }
	}
      else
	{
	  /* Ensure that the bfd will be closed when core_bfd is closed.
	     This can be checked before/after a core file detach via "maint
	     info bfds".  */
	  gdb_bfd_record_inclusion (current_program_space->core_bfd (),
				    abfd.get ());

	  /* Create sections for each mapped region.  */
	  for (const mapped_file::region &region : file_data.regions)
	    {
	      /* Make new BFD section.  All sections have the same name,
		 which is permitted by bfd_make_section_anyway().  */
	      asection *sec = bfd_make_section_anyway (abfd.get (), "load");
	      if (sec == nullptr)
		error (_("Can't make section"));
	      sec->filepos = region.file_ofs;
	      bfd_set_section_flags (sec, SEC_READONLY | SEC_HAS_CONTENTS);
	      bfd_set_section_size (sec, region.end - region.start);
	      bfd_set_section_vma (sec, region.start);
	      bfd_set_section_lma (sec, region.start);
	      bfd_set_section_alignment (sec, 2);

	      /* Set target_section fields.  */
	      m_core_file_mappings.emplace_back (region.start, region.end, sec);
	    }
	}

      /* If this is a bfd with a build-id then record the filename,
	 optional soname (DT_SONAME .dynamic attribute), and the range of
	 addresses at which this bfd is mapped.  This information can be
	 used to perform build-id checking when loading the shared
	 libraries.  */
      if (file_data.build_id != nullptr)
	{
	  normalize_mem_ranges (&ranges);

	  const char *actual_filename = nullptr;
	  gdb::unique_xmalloc_ptr<char> soname;
	  if (abfd != nullptr)
	    {
	      actual_filename = bfd_get_filename (abfd.get ());
	      soname = gdb_bfd_read_elf_soname (actual_filename);
	    }

	  m_mapped_file_info.add (soname.get (), filename.c_str (),
				  actual_filename, std::move (ranges),
				  file_data.build_id);
	}
    }

  normalize_mem_ranges (&m_core_unavailable_mappings);
}

/* An arbitrary identifier for the core inferior.  */
#define CORELOW_PID 1

void
core_target::clear_core ()
{
  if (current_program_space->core_bfd () != nullptr)
    {
      switch_to_no_thread ();    /* Avoid confusion from thread
				    stuff.  */
      exit_inferior (current_inferior ());

      /* Clear out solib state while the bfd is still open.  See
	 comments in clear_solib in solib.c.  */
      clear_solib (current_program_space);

      current_program_space->cbfd.reset (nullptr);
    }
}

/* Close the core target.  */

void
core_target::close ()
{
  clear_core ();

  /* Core targets are heap-allocated (see core_target_open), so here
     we delete ourselves.  */
  delete this;
}

/* Look for sections whose names start with `.reg/' so that we can
   extract the list of threads in a core file.  */

/* If ASECT is a section whose name begins with '.reg/' then extract the
   lwpid after the '/' and create a new thread in INF.

   If REG_SECT is not nullptr, and the both ASECT and REG_SECT point at the
   same position in the parent bfd object then switch to the newly created
   thread, otherwise, the selected thread is left unchanged.  */

static void
add_to_thread_list (asection *asect, asection *reg_sect, inferior *inf)
{
  if (!startswith (bfd_section_name (asect), ".reg/"))
    return;

  int lwpid = atoi (bfd_section_name (asect) + 5);
  ptid_t ptid (inf->pid, lwpid);
  thread_info *thr = add_thread (inf->process_target (), ptid);

  /* Warning, Will Robinson, looking at BFD private data! */

  if (reg_sect != NULL
      && asect->filepos == reg_sect->filepos)	/* Did we find .reg?  */
    switch_to_thread (thr);			/* Yes, make it current.  */
}

/* Issue a message saying we have no core to debug, if FROM_TTY.  */

static void
maybe_say_no_core_file_now (int from_tty)
{
  if (from_tty)
    gdb_printf (_("No core file now.\n"));
}

/* Backward compatibility with old way of specifying core files.  */

void
core_file_command (const char *filename, int from_tty)
{
  dont_repeat ();		/* Either way, seems bogus.  */

  if (filename == NULL)
    {
      if (current_program_space->core_bfd () != nullptr)
	{
	  target_detach (current_inferior (), from_tty);
	  gdb_assert (current_program_space->core_bfd () == nullptr);
	}
      else
	maybe_say_no_core_file_now (from_tty);
    }
  else
    core_target_open (filename, from_tty);
}

/* A vmcore file is a core file created by the Linux kernel at the point of
   a crash.  Each thread in the core file represents a real CPU core, and
   the lwpid for each thread is the pid of the process that was running on
   that core at the moment of the crash.

   However, not every CPU core will have been running a process, some cores
   will be idle.  For these idle cores the CPU writes an lwpid of 0.  And
   of course, multiple cores might be idle, so there could be multiple
   threads with an lwpid of 0.

   The problem is GDB doesn't really like threads with an lwpid of 0; GDB
   presents such a thread as a process rather than a thread.  And GDB
   certainly doesn't like multiple threads having the same lwpid, each time
   a new thread is seen with the same lwpid the earlier thread (with the
   same lwpid) will be deleted.

   This function addresses both of these problems by assigning a fake lwpid
   to any thread with an lwpid of 0.

   GDB finds the lwpid information by looking at the bfd section names
   which include the lwpid, e.g. .reg/NN where NN is the lwpid.  This
   function looks though all the section names looking for sections named
   .reg/NN.  If any sections are found where NN == 0, then we assign a new
   unique value of NN.  Then, in a second pass, any sections ending /0 are
   assigned their new number.

   Remember, a core file may contain multiple register sections for
   different register sets, but the sets are always grouped by thread, so
   we can figure out which registers should be assigned the same new
   lwpid.  For example, consider a core file containing:

     .reg/0, .reg2/0, .reg/0, .reg2/0

   This represents two threads, each thread contains a .reg and .reg2
   register set.  The .reg represents the start of each thread.  After
   renaming the sections will now look like this:

     .reg/1, .reg2/1, .reg/2, .reg2/2

   After calling this function the rest of the core file handling code can
   treat this core file just like any other core file.  */

static void
rename_vmcore_idle_reg_sections (bfd *abfd, inferior *inf)
{
  /* Map from the bfd section to its lwpid (the /NN number).  */
  std::vector<std::pair<asection *, int>> sections_and_lwpids;

  /* The set of all /NN numbers found.  Needed so we can easily find unused
     numbers in the case that we need to rename some sections.  */
  std::unordered_set<int> all_lwpids;

  /* A count of how many sections called .reg/0 we have found.  */
  unsigned zero_lwpid_count = 0;

  /* Look for all the .reg sections.  Record the section object and the
     lwpid which is extracted from the section name.  Spot if any have an
     lwpid of zero.  */
  for (asection *sect : gdb_bfd_sections (current_program_space->core_bfd ()))
    {
      if (startswith (bfd_section_name (sect), ".reg/"))
	{
	  int lwpid = atoi (bfd_section_name (sect) + 5);
	  sections_and_lwpids.emplace_back (sect, lwpid);
	  all_lwpids.insert (lwpid);
	  if (lwpid == 0)
	    zero_lwpid_count++;
	}
    }

  /* If every ".reg/NN" section has a non-zero lwpid then we don't need to
     do any renaming.  */
  if (zero_lwpid_count == 0)
    return;

  /* Assign a new number to any .reg sections with an lwpid of 0.  */
  int new_lwpid = 1;
  for (auto &sect_and_lwpid : sections_and_lwpids)
    if (sect_and_lwpid.second == 0)
      {
	while (all_lwpids.find (new_lwpid) != all_lwpids.end ())
	  new_lwpid++;
	sect_and_lwpid.second = new_lwpid;
	new_lwpid++;
      }

  /* Now update the names of any sections with an lwpid of 0.  This is
     more than just the .reg sections we originally found.  */
  std::string replacement_lwpid_str;
  auto iter = sections_and_lwpids.begin ();
  int replacement_lwpid = 0;
  for (asection *sect : gdb_bfd_sections (current_program_space->core_bfd ()))
    {
      if (iter != sections_and_lwpids.end () && sect == iter->first)
	{
	  gdb_assert (startswith (bfd_section_name (sect), ".reg/"));

	  int lwpid = atoi (bfd_section_name (sect) + 5);
	  if (lwpid == iter->second)
	    {
	      /* This section was not given a new number.  */
	      gdb_assert (lwpid != 0);
	      replacement_lwpid = 0;
	    }
	  else
	    {
	      replacement_lwpid = iter->second;
	      ptid_t ptid (inf->pid, replacement_lwpid);
	      if (!replacement_lwpid_str.empty ())
		replacement_lwpid_str += ", ";
	      replacement_lwpid_str += target_pid_to_str (ptid);
	    }

	  iter++;
	}

      if (replacement_lwpid != 0)
	{
	  const char *name = bfd_section_name (sect);
	  size_t len = strlen (name);

	  if (strncmp (name + len - 2, "/0", 2) == 0)
	    {
	      /* This section needs a new name.  */
	      std::string name_str
		= string_printf ("%.*s/%d",
				 static_cast<int> (len - 2),
				 name, replacement_lwpid);
	      char *name_buf
		= static_cast<char *> (bfd_alloc (abfd, name_str.size () + 1));
	      if (name_buf == nullptr)
		error (_("failed to allocate space for section name '%s'"),
		       name_str.c_str ());
	      memcpy (name_buf, name_str.c_str(), name_str.size () + 1);
	      bfd_rename_section (sect, name_buf);
	    }
	}
    }

  if (zero_lwpid_count == 1)
    warning (_("found thread with pid 0, assigned replacement Target Id: %s"),
	     replacement_lwpid_str.c_str ());
  else
    warning (_("found threads with pid 0, assigned replacement Target Ids: %s"),
	     replacement_lwpid_str.c_str ());
}

/* Use CTX to try and find (and open) the executable file for the core file
   CBFD.  BUILD_ID is the build-id for CBFD which was already extracted by
   our caller.

   Will return the opened executable or nullptr if the executable couldn't
   be found.  */

static gdb_bfd_ref_ptr
locate_exec_from_corefile_exec_context (bfd *cbfd,
					const bfd_build_id *build_id,
					const core_file_exec_context &ctx)
{
  /* CTX must be valid, and a valid context has an execfn() string.  */
  gdb_assert (ctx.valid ());
  gdb_assert (ctx.execfn () != nullptr);

  /* EXEC_NAME will be the command used to start the inferior.  This might
     not be an absolute path (but could be).  */
  const char *exec_name = ctx.execfn ();

  /* Function to open FILENAME and check if its build-id matches BUILD_ID
     from this enclosing scope.  Returns the open BFD for filename if the
     FILENAME has a matching build-id, otherwise, returns nullptr.  */
  const auto open_and_check_build_id
    = [&build_id] (const char *filename) -> gdb_bfd_ref_ptr
  {
    /* Try to open a file.  If this succeeds then we still need to perform
       a build-id check.  */
    gdb_bfd_ref_ptr execbfd = gdb_bfd_open (filename, gnutarget);

    /* We managed to open a file, but if it's build-id doesn't match
       BUILD_ID then we just cannot trust it's the right file.  */
    if (execbfd != nullptr)
      {
	const bfd_build_id *other_build_id = build_id_bfd_get (execbfd.get ());

	if (other_build_id == nullptr
	    || !build_id_equal (other_build_id, build_id))
	  execbfd = nullptr;
      }

    return execbfd;
  };

  gdb_bfd_ref_ptr execbfd;

  /* If EXEC_NAME is absolute then try to open it now.  Otherwise, see if
     EXEC_NAME is a relative path from the location of the core file.  This
     is just a guess, the executable might not be here, but we still rely
     on a build-id match in order to accept any executable we find; we
     don't accept something just because it happens to be in the right
     location.  */
  if (IS_ABSOLUTE_PATH (exec_name))
    execbfd = open_and_check_build_id (exec_name);
  else
    {
      std::string p = (ldirname (bfd_get_filename (cbfd))
		       + '/'
		       + exec_name);
      execbfd = open_and_check_build_id (p.c_str ());
    }

  /* If we haven't found the executable yet, then try checking to see if
     the executable is in the same directory as the core file.  Again,
     there's no reason why this should be the case, but it's worth a try,
     and the build-id check should ensure we don't use an invalid file if
     we happen to find one.  */
  if (execbfd == nullptr)
    {
      const char *base_name = lbasename (exec_name);
      std::string p = (ldirname (bfd_get_filename (cbfd))
		       + '/'
		       + base_name);
      execbfd = open_and_check_build_id (p.c_str ());
    }

  /* If the above didn't provide EXECBFD then try the exec_filename from
     the context.  This will be an absolute filename which the gdbarch code
     figured out from the core file.  In some cases the gdbarch code might
     not be able to figure out a suitable absolute filename though.  */
  if (execbfd == nullptr && ctx.exec_filename () != nullptr)
    {
      gdb_assert (IS_ABSOLUTE_PATH (ctx.exec_filename ()));

      /* Try to open a file.  If this succeeds then we still need to
	 perform a build-id check.  */
      execbfd = open_and_check_build_id (ctx.exec_filename ());
    }

  return execbfd;
}

/* Locate (and load) an executable file (and symbols) given the core file
   BFD ABFD.  */

static void
locate_exec_from_corefile_build_id (bfd *abfd,
				    core_target *target,
				    const core_file_exec_context &ctx,
				    int from_tty)
{
  const bfd_build_id *build_id = build_id_bfd_get (abfd);
  if (build_id == nullptr)
    return;

  gdb_bfd_ref_ptr execbfd;

  if (ctx.valid ())
    execbfd = locate_exec_from_corefile_exec_context (abfd, build_id, ctx);

  if (execbfd == nullptr)
    {
      /* The filename used for the find_objfile_by_build_id call.  */
      std::string filename;

      if (!target->expected_exec_filename ().empty ())
	filename = target->expected_exec_filename ();
      else
	{
	  /* We didn't find an executable name from the mapped file
	     information, so as a stand-in build a string based on the
	     build-id.  */
	  std::string build_id_hex_str
	    = bin2hex (build_id->data, build_id->size);
	  filename
	    = string_printf ("with build-id %s", build_id_hex_str.c_str ());
	}

      execbfd
	= find_objfile_by_build_id (current_program_space, build_id,
				    filename.c_str ());
    }

  if (execbfd != nullptr)
    {
      exec_file_attach (bfd_get_filename (execbfd.get ()), from_tty);
      symbol_file_add_main (bfd_get_filename (execbfd.get ()),
			    symfile_add_flag (from_tty ? SYMFILE_VERBOSE : 0));
    }
}

/* Open and set up the core file bfd.  */

static void
core_target_open (const char *arg, int from_tty)
{
  int siggy;
  int scratch_chan;
  int flags;

  target_preopen (from_tty);

  std::string filename = extract_single_filename_arg (arg);

  if (filename.empty ())
    {
      if (current_program_space->core_bfd ())
	error (_("No core file specified.  (Use `detach' "
		 "to stop debugging a core file.)"));
      else
	error (_("No core file specified."));
    }

  if (!IS_ABSOLUTE_PATH (filename.c_str ()))
    filename = gdb_abspath (filename);

  flags = O_BINARY | O_LARGEFILE;
  if (write_files)
    flags |= O_RDWR;
  else
    flags |= O_RDONLY;
  scratch_chan = gdb_open_cloexec (filename.c_str (), flags, 0).release ();
  if (scratch_chan < 0)
    perror_with_name (filename.c_str ());

  gdb_bfd_ref_ptr temp_bfd (gdb_bfd_fopen (filename.c_str (), gnutarget,
					   write_files ? FOPEN_RUB : FOPEN_RB,
					   scratch_chan));
  if (temp_bfd == NULL)
    perror_with_name (filename.c_str ());

  if (!bfd_check_format (temp_bfd.get (), bfd_core))
    {
      /* Do it after the err msg */
      /* FIXME: should be checking for errors from bfd_close (for one
	 thing, on error it does not free all the storage associated
	 with the bfd).  */
      error (_("\"%s\" is not a core dump: %s"),
	     filename.c_str (), bfd_errmsg (bfd_get_error ()));
    }

  current_program_space->cbfd = std::move (temp_bfd);

  core_target *target = new core_target ();

  /* Own the target until it is successfully pushed.  */
  target_ops_up target_holder (target);

  validate_files ();

  current_inferior ()->push_target (std::move (target_holder));

  switch_to_no_thread ();

  /* Need to flush the register cache (and the frame cache) from a
     previous debug session.  If inferior_ptid ends up the same as the
     last debug session --- e.g., b foo; run; gcore core1; step; gcore
     core2; core core1; core core2 --- then there's potential for
     get_current_regcache to return the cached regcache of the
     previous session, and the frame cache being stale.  */
  registers_changed ();

  /* Find (or fake) the pid for the process in this core file, and
     initialise the current inferior with that pid.  */
  bool fake_pid_p = false;
  int pid = bfd_core_file_pid (current_program_space->core_bfd ());
  if (pid == 0)
    {
      fake_pid_p = true;
      pid = CORELOW_PID;
    }

  inferior *inf = current_inferior ();
  gdb_assert (inf->pid == 0);
  inferior_appeared (inf, pid);
  inf->fake_pid_p = fake_pid_p;

  /* Rename any .reg/0 sections, giving them each a fake lwpid.  */
  rename_vmcore_idle_reg_sections (current_program_space->core_bfd (), inf);

  /* Build up thread list from BFD sections, and possibly set the
     current thread to the .reg/NN section matching the .reg
     section.  */
  asection *reg_sect
    = bfd_get_section_by_name (current_program_space->core_bfd (), ".reg");
  for (asection *sect : gdb_bfd_sections (current_program_space->core_bfd ()))
    add_to_thread_list (sect, reg_sect, inf);

  if (inferior_ptid == null_ptid)
    {
      /* Either we found no .reg/NN section, and hence we have a
	 non-threaded core (single-threaded, from gdb's perspective),
	 or for some reason add_to_thread_list couldn't determine
	 which was the "main" thread.  The latter case shouldn't
	 usually happen, but we're dealing with input here, which can
	 always be broken in different ways.  */
      thread_info *thread = first_thread_of_inferior (inf);

      if (thread == NULL)
	thread = add_thread_silent (target, ptid_t (CORELOW_PID));

      switch_to_thread (thread);
    }

  /* In order to parse the exec context from the core file the current
     inferior needs to have a suitable gdbarch set.  If an exec file is
     loaded then the gdbarch will have been set based on the exec file, but
     if not, ensure we have a suitable gdbarch in place now.  */
  if (current_program_space->exec_bfd () == nullptr)
      current_inferior ()->set_arch (target->core_gdbarch ());

  /* See if the gdbarch can find the executable name and argument list from
     the core file.  */
  core_file_exec_context ctx
    = gdbarch_core_parse_exec_context (target->core_gdbarch (),
				       current_program_space->core_bfd ());

  /* If we don't have an executable loaded then see if we can locate one
     based on the core file.  */
  if (current_program_space->exec_bfd () == nullptr)
    locate_exec_from_corefile_build_id (current_program_space->core_bfd (),
					target, ctx, from_tty);

  /* If we have no exec file, try to set the architecture from the
     core file.  We don't do this unconditionally since an exec file
     typically contains more information that helps us determine the
     architecture than a core file.  */
  if (current_program_space->exec_bfd () == nullptr)
    set_gdbarch_from_file (current_program_space->core_bfd ());

  post_create_inferior (from_tty);

  /* Now go through the target stack looking for threads since there
     may be a thread_stratum target loaded on top of target core by
     now.  The layer above should claim threads found in the BFD
     sections.  */
  try
    {
      target_update_thread_list ();
    }

  catch (const gdb_exception_error &except)
    {
      exception_print (gdb_stderr, except);
    }

  if (ctx.valid ())
    {
      /* Copy the arguments into the inferior.  */
      std::vector<char *> argv;
      for (const gdb::unique_xmalloc_ptr<char> &a : ctx.args ())
	argv.push_back (a.get ());
      gdb::array_view<char * const> view (argv.data (), argv.size ());
      current_inferior ()->set_args (view);

      /* And now copy the environment.  */
      current_inferior ()->environment = ctx.environment ();

      /* Inform the user of executable and arguments.  */
      const std::string &args = current_inferior ()->args ();
      gdb_printf (_("Core was generated by `%ps%s%s'.\n"),
		  styled_string (file_name_style.style (),
				 ctx.execfn ()),
		  (args.length () > 0 ? " " : ""), args.c_str ());
    }
  else
    {
      const char *failing_command
	= bfd_core_file_failing_command (current_program_space->core_bfd ());
      if (failing_command != nullptr)
	gdb_printf (_("Core was generated by `%s'.\n"),
		    failing_command);
    }

  /* Clearing any previous state of convenience variables.  */
  clear_exit_convenience_vars ();

  siggy = bfd_core_file_failing_signal (current_program_space->core_bfd ());
  if (siggy > 0)
    {
      gdbarch *core_gdbarch = target->core_gdbarch ();

      /* If we don't have a CORE_GDBARCH to work with, assume a native
	 core (map gdb_signal from host signals).  If we do have
	 CORE_GDBARCH to work with, but no gdb_signal_from_target
	 implementation for that gdbarch, as a fallback measure,
	 assume the host signal mapping.  It'll be correct for native
	 cores, but most likely incorrect for cross-cores.  */
      enum gdb_signal sig = (core_gdbarch != NULL
			     && gdbarch_gdb_signal_from_target_p (core_gdbarch)
			     ? gdbarch_gdb_signal_from_target (core_gdbarch,
							       siggy)
			     : gdb_signal_from_host (siggy));

      gdb_printf (_("Program terminated with signal %s, %s"),
		  gdb_signal_to_name (sig), gdb_signal_to_string (sig));
      if (gdbarch_report_signal_info_p (core_gdbarch))
	gdbarch_report_signal_info (core_gdbarch, current_uiout, sig);
      gdb_printf (_(".\n"));

      /* Set the value of the internal variable $_exitsignal,
	 which holds the signal uncaught by the inferior.  */
      set_internalvar_integer (lookup_internalvar ("_exitsignal"),
			       siggy);
    }

  /* Fetch all registers from core file.  */
  target_fetch_registers (get_thread_regcache (inferior_thread ()), -1);

  /* Now, set up the frame cache, and print the top of stack.  */
  reinit_frame_cache ();
  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);

  /* Current thread should be NUM 1 but the user does not know that.
     If a program is single threaded gdb in general does not mention
     anything about threads.  That is why the test is >= 2.  */
  if (thread_count (target) >= 2)
    {
      try
	{
	  thread_command (NULL, from_tty);
	}
      catch (const gdb_exception_error &except)
	{
	  exception_print (gdb_stderr, except);
	}
    }
}

void
core_target::detach (inferior *inf, int from_tty)
{
  /* Get rid of the core.  Don't rely on core_target::close doing it,
     because target_detach may be called with core_target's refcount > 1,
     meaning core_target::close may not be called yet by the
     unpush_target call below.  */
  clear_core ();

  /* Note that 'this' may be dangling after this call.  unpush_target
     closes the target if the refcount reaches 0, and our close
     implementation deletes 'this'.  */
  inf->unpush_target (this);

  /* Clear the register cache and the frame cache.  */
  registers_changed ();
  reinit_frame_cache ();
  maybe_say_no_core_file_now (from_tty);
}

/* Try to retrieve registers from a section in core_bfd, and supply
   them to REGSET.

   If ptid's lwp member is zero, do the single-threaded
   thing: look for a section named NAME.  If ptid's lwp
   member is non-zero, do the multi-threaded thing: look for a section
   named "NAME/LWP", where LWP is the shortest ASCII decimal
   representation of ptid's lwp member.

   HUMAN_NAME is a human-readable name for the kind of registers the
   NAME section contains, for use in error messages.

   If REQUIRED is true, print an error if the core file doesn't have a
   section by the appropriate name.  Otherwise, just do nothing.  */

void
core_target::get_core_register_section (struct regcache *regcache,
					const struct regset *regset,
					const char *name,
					int section_min_size,
					const char *human_name,
					bool required)
{
  gdb_assert (regset != nullptr);

  struct bfd_section *section;
  bfd_size_type size;
  bool variable_size_section = (regset->flags & REGSET_VARIABLE_SIZE);

  thread_section_name section_name (name, regcache->ptid ());

  section = bfd_get_section_by_name (current_program_space->core_bfd (),
				     section_name.c_str ());
  if (! section)
    {
      if (required)
	warning (_("Couldn't find %s registers in core file."),
		 human_name);
      return;
    }

  size = bfd_section_size (section);
  if (size < section_min_size)
    {
      warning (_("Section `%s' in core file too small."),
	       section_name.c_str ());
      return;
    }
  if (size != section_min_size && !variable_size_section)
    {
      warning (_("Unexpected size of section `%s' in core file."),
	       section_name.c_str ());
    }

  gdb::byte_vector contents (size);
  if (!bfd_get_section_contents (current_program_space->core_bfd (), section,
				 contents.data (), (file_ptr) 0, size))
    {
      warning (_("Couldn't read %s registers from `%s' section in core file."),
	       human_name, section_name.c_str ());
      return;
    }

  regset->supply_regset (regset, regcache, -1, contents.data (), size);
}

/* Data passed to gdbarch_iterate_over_regset_sections's callback.  */
struct get_core_registers_cb_data
{
  core_target *target;
  struct regcache *regcache;
};

/* Callback for get_core_registers that handles a single core file
   register note section. */

static void
get_core_registers_cb (const char *sect_name, int supply_size, int collect_size,
		       const struct regset *regset,
		       const char *human_name, void *cb_data)
{
  gdb_assert (regset != nullptr);

  auto *data = (get_core_registers_cb_data *) cb_data;
  bool required = false;
  bool variable_size_section = (regset->flags & REGSET_VARIABLE_SIZE);

  if (!variable_size_section)
    gdb_assert (supply_size == collect_size);

  if (strcmp (sect_name, ".reg") == 0)
    {
      required = true;
      if (human_name == NULL)
	human_name = "general-purpose";
    }
  else if (strcmp (sect_name, ".reg2") == 0)
    {
      if (human_name == NULL)
	human_name = "floating-point";
    }

  data->target->get_core_register_section (data->regcache, regset, sect_name,
					   supply_size, human_name, required);
}

/* Get the registers out of a core file.  This is the machine-
   independent part.  Fetch_core_registers is the machine-dependent
   part, typically implemented in the xm-file for each
   architecture.  */

/* We just get all the registers, so we don't use regno.  */

void
core_target::fetch_registers (struct regcache *regcache, int regno)
{
  if (!(m_core_gdbarch != nullptr
	&& gdbarch_iterate_over_regset_sections_p (m_core_gdbarch)))
    {
      gdb_printf (gdb_stderr,
		  "Can't fetch registers from this type of core file\n");
      return;
    }

  struct gdbarch *gdbarch = regcache->arch ();
  get_core_registers_cb_data data = { this, regcache };
  gdbarch_iterate_over_regset_sections (gdbarch,
					get_core_registers_cb,
					(void *) &data, NULL);

  /* Mark all registers not found in the core as unavailable.  */
  for (int i = 0; i < gdbarch_num_regs (regcache->arch ()); i++)
    if (regcache->get_register_status (i) == REG_UNKNOWN)
      regcache->raw_supply (i, NULL);
}

void
core_target::files_info ()
{
  print_section_info (&m_core_section_table, current_program_space->core_bfd ());
}


enum target_xfer_status
core_target::xfer_partial (enum target_object object, const char *annex,
			   gdb_byte *readbuf, const gdb_byte *writebuf,
			   ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
      {
	enum target_xfer_status xfer_status;

	/* Try accessing memory contents from core file data,
	   restricting consideration to those sections for which
	   the BFD section flag SEC_HAS_CONTENTS is set.  */
	auto has_contents_cb = [] (const struct target_section *s)
	  {
	    return ((s->the_bfd_section->flags & SEC_HAS_CONTENTS) != 0);
	  };
	xfer_status = section_table_xfer_memory_partial
			(readbuf, writebuf,
			 offset, len, xfered_len,
			 m_core_section_table,
			 has_contents_cb);
	if (xfer_status == TARGET_XFER_OK)
	  return TARGET_XFER_OK;

	/* Check file backed mappings.  If they're available, use core file
	   provided mappings (e.g. from .note.linuxcore.file or the like)
	   as this should provide a more accurate result.  */
	if (!m_core_file_mappings.empty ())
	  {
	    xfer_status = section_table_xfer_memory_partial
			    (readbuf, writebuf, offset, len, xfered_len,
			     m_core_file_mappings);
	    if (xfer_status == TARGET_XFER_OK)
	      return xfer_status;
	  }

	/* If the access is within an unavailable file mapping then we try
	   to check in the stratum below (the executable stratum).  The
	   thinking here is that if the mapping was read/write then the
	   contents would have been written into the core file and the
	   access would have been satisfied by m_core_section_table.

	   But if the access has not yet been resolved then we can assume
	   the access is read-only.  If the executable was not found
	   during the mapped file check then we'll have an unavailable
	   mapping entry, however, if the user has provided the executable
	   (maybe in a different location) then we might be able to
	   resolve the access from there.

	   If that fails, but the access is within an unavailable region,
	   then the access itself should fail.  */
	for (const auto &mr : m_core_unavailable_mappings)
	  {
	    if (mr.contains (offset))
	      {
		if (!mr.contains (offset + len))
		  len = mr.start + mr.length - offset;

		xfer_status
		  = this->beneath ()->xfer_partial (TARGET_OBJECT_MEMORY,
						    nullptr, readbuf,
						    writebuf, offset,
						    len, xfered_len);
		if (xfer_status == TARGET_XFER_OK)
		  return TARGET_XFER_OK;

		return TARGET_XFER_E_IO;
	      }
	  }

	/* The following is acting as a fallback in case we encounter a
	   situation where the core file is lacking and mapped file
	   information.  Here we query the exec file stratum to see if it
	   can resolve the access.  Doing this when we are missing mapped
	   file information might be the best we can do, but there are
	   certainly cases this will get wrong, e.g. if an inferior created
	   a zero initialised mapping over the top of some data that exists
	   within the executable then this will return the executable data
	   rather than the zero data.  Maybe we should just drop this
	   block?  */
	if (m_core_file_mappings.empty ()
	    && m_core_unavailable_mappings.empty ())
	  {
	    xfer_status
	      = this->beneath ()->xfer_partial (object, annex, readbuf,
						writebuf, offset, len,
						xfered_len);
	    if (xfer_status == TARGET_XFER_OK)
	      return TARGET_XFER_OK;
	  }

#ifndef __NetBSD__
	/* Finally, attempt to access data in core file sections with
	   no contents.  These will typically read as all zero.  */
	auto no_contents_cb = [&] (const struct target_section *s)
	  {
	    return !has_contents_cb (s);
	  };
	xfer_status = section_table_xfer_memory_partial
			(readbuf, writebuf,
			 offset, len, xfered_len,
			 m_core_section_table,
			 no_contents_cb);
#endif

	return xfer_status;
      }
    case TARGET_OBJECT_AUXV:
      if (readbuf)
	{
	  /* When the aux vector is stored in core file, BFD
	     represents this with a fake section called ".auxv".  */

	  struct bfd_section *section;
	  bfd_size_type size;

	  section = bfd_get_section_by_name (current_program_space->core_bfd (),
					     ".auxv");
	  if (section == NULL)
	    return TARGET_XFER_E_IO;

	  size = bfd_section_size (section);
	  if (offset >= size)
	    return TARGET_XFER_EOF;
	  size -= offset;
	  if (size > len)
	    size = len;

	  if (size == 0)
	    return TARGET_XFER_EOF;
	  if (!bfd_get_section_contents (current_program_space->core_bfd (),
					 section, readbuf, (file_ptr) offset,
					 size))
	    {
	      warning (_("Couldn't read NT_AUXV note in core file."));
	      return TARGET_XFER_E_IO;
	    }

	  *xfered_len = (ULONGEST) size;
	  return TARGET_XFER_OK;
	}
      return TARGET_XFER_E_IO;

    case TARGET_OBJECT_WCOOKIE:
      if (readbuf)
	{
	  /* When the StackGhost cookie is stored in core file, BFD
	     represents this with a fake section called
	     ".wcookie".  */

	  struct bfd_section *section;
	  bfd_size_type size;

	  section = bfd_get_section_by_name (current_program_space->core_bfd (),
					     ".wcookie");
	  if (section == NULL)
	    return TARGET_XFER_E_IO;

	  size = bfd_section_size (section);
	  if (offset >= size)
	    return TARGET_XFER_EOF;
	  size -= offset;
	  if (size > len)
	    size = len;

	  if (size == 0)
	    return TARGET_XFER_EOF;
	  if (!bfd_get_section_contents (current_program_space->core_bfd (),
					 section, readbuf, (file_ptr) offset,
					 size))
	    {
	      warning (_("Couldn't read StackGhost cookie in core file."));
	      return TARGET_XFER_E_IO;
	    }

	  *xfered_len = (ULONGEST) size;
	  return TARGET_XFER_OK;

	}
      return TARGET_XFER_E_IO;

    case TARGET_OBJECT_LIBRARIES:
      if (m_core_gdbarch != nullptr
	  && gdbarch_core_xfer_shared_libraries_p (m_core_gdbarch))
	{
	  if (writebuf)
	    return TARGET_XFER_E_IO;
	  else
	    {
	      *xfered_len = gdbarch_core_xfer_shared_libraries (m_core_gdbarch,
								readbuf,
								offset, len);

	      if (*xfered_len == 0)
		return TARGET_XFER_EOF;
	      else
		return TARGET_XFER_OK;
	    }
	}
      return TARGET_XFER_E_IO;

    case TARGET_OBJECT_LIBRARIES_AIX:
      if (m_core_gdbarch != nullptr
	  && gdbarch_core_xfer_shared_libraries_aix_p (m_core_gdbarch))
	{
	  if (writebuf)
	    return TARGET_XFER_E_IO;
	  else
	    {
	      *xfered_len
		= gdbarch_core_xfer_shared_libraries_aix (m_core_gdbarch,
							  readbuf, offset,
							  len);

	      if (*xfered_len == 0)
		return TARGET_XFER_EOF;
	      else
		return TARGET_XFER_OK;
	    }
	}
      return TARGET_XFER_E_IO;

    case TARGET_OBJECT_SIGNAL_INFO:
      if (readbuf)
	{
	  if (m_core_gdbarch != nullptr
	      && gdbarch_core_xfer_siginfo_p (m_core_gdbarch))
	    {
	      LONGEST l = gdbarch_core_xfer_siginfo  (m_core_gdbarch, readbuf,
						      offset, len);

	      if (l >= 0)
		{
		  *xfered_len = l;
		  if (l == 0)
		    return TARGET_XFER_EOF;
		  else
		    return TARGET_XFER_OK;
		}
	    }
	}
      return TARGET_XFER_E_IO;

    default:
      return this->beneath ()->xfer_partial (object, annex, readbuf,
					     writebuf, offset, len,
					     xfered_len);
    }
}



/* Okay, let's be honest: threads gleaned from a core file aren't
   exactly lively, are they?  On the other hand, if we don't claim
   that each & every one is alive, then we don't get any of them
   to appear in an "info thread" command, which is quite a useful
   behavior.
 */
bool
core_target::thread_alive (ptid_t ptid)
{
  return true;
}

/* Ask the current architecture what it knows about this core file.
   That will be used, in turn, to pick a better architecture.  This
   wrapper could be avoided if targets got a chance to specialize
   core_target.  */

const struct target_desc *
core_target::read_description ()
{
  /* First check whether the target wants us to use the corefile target
     description notes.  */
  if (gdbarch_use_target_description_from_corefile_notes
	(m_core_gdbarch, current_program_space->core_bfd ()))
    {
      /* If the core file contains a target description note then go ahead and
	 use that.  */
      bfd_size_type tdesc_note_size = 0;
      struct bfd_section *tdesc_note_section
	= bfd_get_section_by_name (current_program_space->core_bfd (), ".gdb-tdesc");
      if (tdesc_note_section != nullptr)
	tdesc_note_size = bfd_section_size (tdesc_note_section);
      if (tdesc_note_size > 0)
	{
	  gdb::char_vector contents (tdesc_note_size + 1);
	  if (bfd_get_section_contents (current_program_space->core_bfd (),
					tdesc_note_section, contents.data (),
					(file_ptr) 0, tdesc_note_size))
	    {
	      /* Ensure we have a null terminator.  */
	      contents[tdesc_note_size] = '\0';
	      const struct target_desc *result
		= string_read_description_xml (contents.data ());
	      if (result != nullptr)
		return result;
	    }
	}
    }

  /* If the architecture provides a corefile target description hook, use
     it now.  Even if the core file contains a target description in a note
     section, it is not useful for targets that can potentially have distinct
     descriptions for each thread.  One example is AArch64's SVE/SME
     extensions that allow per-thread vector length changes, resulting in
     registers with different sizes.  */
  if (m_core_gdbarch && gdbarch_core_read_description_p (m_core_gdbarch))
    {
      const struct target_desc *result;

      result = gdbarch_core_read_description
		 (m_core_gdbarch, this, current_program_space->core_bfd ());
      if (result != nullptr)
	return result;
    }

  return this->beneath ()->read_description ();
}

std::string
core_target::pid_to_str (ptid_t ptid)
{
  struct inferior *inf;
  int pid;

  /* The preferred way is to have a gdbarch/OS specific
     implementation.  */
  if (m_core_gdbarch != nullptr
      && gdbarch_core_pid_to_str_p (m_core_gdbarch))
    return gdbarch_core_pid_to_str (m_core_gdbarch, ptid);

  /* Otherwise, if we don't have one, we'll just fallback to
     "process", with normal_pid_to_str.  */

  /* Try the LWPID field first.  */
  pid = ptid.lwp ();
  if (pid != 0)
    return normal_pid_to_str (ptid_t (pid));

  /* Otherwise, this isn't a "threaded" core -- use the PID field, but
     only if it isn't a fake PID.  */
  inf = find_inferior_ptid (this, ptid);
  if (inf != NULL && !inf->fake_pid_p)
    return normal_pid_to_str (ptid);

  /* No luck.  We simply don't have a valid PID to print.  */
  return "<main task>";
}

const char *
core_target::thread_name (struct thread_info *thr)
{
  if (m_core_gdbarch != nullptr
      && gdbarch_core_thread_name_p (m_core_gdbarch))
    return gdbarch_core_thread_name (m_core_gdbarch, thr);
  return NULL;
}

bool
core_target::has_memory ()
{
  return current_program_space->core_bfd () != nullptr;
}

bool
core_target::has_stack ()
{
  return current_program_space->core_bfd () != nullptr;
}

bool
core_target::has_registers ()
{
  return current_program_space->core_bfd () != nullptr;
}

/* Implement the to_info_proc method.  */

bool
core_target::info_proc (const char *args, enum info_proc_what request)
{
  struct gdbarch *gdbarch = get_current_arch ();

  /* Since this is the core file target, call the 'core_info_proc'
     method on gdbarch, not 'info_proc'.  */
  if (gdbarch_core_info_proc_p (gdbarch))
    gdbarch_core_info_proc (gdbarch, args, request);

  return true;
}

/* Implementation of the "supports_memory_tagging" target_ops method.  */

bool
core_target::supports_memory_tagging ()
{
  /* Look for memory tag sections.  If they exist, that means this core file
     supports memory tagging.  */

  return (bfd_get_section_by_name (current_program_space->core_bfd (), "memtag")
	  != nullptr);
}

/* Implementation of the "fetch_memtags" target_ops method.  */

bool
core_target::fetch_memtags (CORE_ADDR address, size_t len,
			    gdb::byte_vector &tags, int type)
{
  gdbarch *gdbarch = current_inferior ()->arch ();

  /* Make sure we have a way to decode the memory tag notes.  */
  if (!gdbarch_decode_memtag_section_p (gdbarch))
    error (_("gdbarch_decode_memtag_section not implemented for this "
	     "architecture."));

  memtag_section_info info;
  info.memtag_section = nullptr;

  while (get_next_core_memtag_section (current_program_space->core_bfd (),
				       info.memtag_section, address, info))
  {
    size_t adjusted_length
      = (address + len < info.end_address) ? len : (info.end_address - address);

    /* Decode the memory tag note and return the tags.  */
    gdb::byte_vector tags_read
      = gdbarch_decode_memtag_section (gdbarch, info.memtag_section, type,
				       address, adjusted_length);

    /* Transfer over the tags that have been read.  */
    tags.insert (tags.end (), tags_read.begin (), tags_read.end ());

    /* ADDRESS + LEN may cross the boundaries of a particular memory tag
       segment.  Check if we need to fetch tags from a different section.  */
    if (!tags_read.empty () && (address + len) < info.end_address)
      return true;

    /* There are more tags to fetch.  Update ADDRESS and LEN.  */
    len -= (info.end_address - address);
    address = info.end_address;
  }

  return false;
}

bool
core_target::is_address_tagged (gdbarch *gdbarch, CORE_ADDR address)
{
  return gdbarch_tagged_address_p (gdbarch, address);
}

/* Implementation of the "fetch_x86_xsave_layout" target_ops method.  */

x86_xsave_layout
core_target::fetch_x86_xsave_layout ()
{
  if (m_core_gdbarch != nullptr &&
      gdbarch_core_read_x86_xsave_layout_p (m_core_gdbarch))
    {
      x86_xsave_layout layout;
      if (!gdbarch_core_read_x86_xsave_layout (m_core_gdbarch, layout))
	return {};

      return layout;
    }

  return {};
}

/* Get a pointer to the current core target.  If not connected to a
   core target, return NULL.  */

static core_target *
get_current_core_target ()
{
  target_ops *proc_target = current_inferior ()->process_target ();
  return dynamic_cast<core_target *> (proc_target);
}

/* Display file backed mappings from core file.  */

void
core_target::info_proc_mappings (struct gdbarch *gdbarch)
{
  if (m_core_file_mappings.empty ())
    return;

  gdb_printf (_("Mapped address spaces:\n\n"));
  ui_out_emit_table emitter (current_uiout, 5, -1, "ProcMappings");

  int width = gdbarch_addr_bit (gdbarch) == 32 ? 10 : 18;
  current_uiout->table_header (width, ui_left, "start", "Start Addr");
  current_uiout->table_header (width, ui_left, "end", "End Addr");
  current_uiout->table_header (width, ui_left, "size", "Size");
  current_uiout->table_header (width, ui_left, "offset", "Offset");
  current_uiout->table_header (0, ui_left, "objfile", "File");
  current_uiout->table_body ();

  for (const target_section &tsp : m_core_file_mappings)
    {
      ULONGEST start = tsp.addr;
      ULONGEST end = tsp.endaddr;
      ULONGEST file_ofs = tsp.the_bfd_section->filepos;
      const char *filename = bfd_get_filename (tsp.the_bfd_section->owner);

      ui_out_emit_tuple tuple_emitter (current_uiout, nullptr);
      current_uiout->field_core_addr ("start", gdbarch, start);
      current_uiout->field_core_addr ("end", gdbarch, end);
      /* These next two aren't really addresses and so shouldn't be
	 styled as such.  */
      current_uiout->field_string ("size", paddress (gdbarch, end - start));
      current_uiout->field_string ("offset", paddress (gdbarch, file_ofs));
      current_uiout->field_string ("objfile", filename,
				   file_name_style.style ());
      current_uiout->text ("\n");
    }
}

/* Implement "maintenance print core-file-backed-mappings" command.  

   If mappings are loaded, the results should be similar to the
   mappings shown by "info proc mappings".  This command is mainly a
   debugging tool for GDB developers to make sure that the expected
   mappings are present after loading a core file.  For Linux, the
   output provided by this command will be very similar (if not
   identical) to that provided by "info proc mappings".  This is not
   necessarily the case for other OSes which might provide
   more/different information in the "info proc mappings" output.  */

static void
maintenance_print_core_file_backed_mappings (const char *args, int from_tty)
{
  core_target *targ = get_current_core_target ();
  if (targ != nullptr)
    targ->info_proc_mappings (targ->core_gdbarch ());
}

/* Add more details discovered while processing the core-file's mapped file
   information, we're building maps between filenames and the corresponding
   build-ids, between address ranges and the corresponding build-ids, and
   also a reverse map between build-id and the corresponding filename.

   SONAME is the DT_SONAME attribute extracted from the .dynamic section of
   a shared library that was mapped into the core file.  This can be
   nullptr if the mapped files was not a shared library, or didn't have a
   DT_SONAME attribute.

   EXPECTED_FILENAME is the name of the file that was mapped into the
   inferior as extracted from the core file, this should never be nullptr.

   ACTUAL_FILENAME is the name of the actual file GDB found to provide the
   mapped file information, this can be nullptr if GDB failed to find a
   suitable file.  This might be different to EXPECTED_FILENAME, e.g. GDB
   might have downloaded the file from debuginfod and so ACTUAL_FILENAME
   will be a file in the debuginfod client cache.

   RANGES is the list of memory ranges at which this file was mapped into
   the inferior.

   BUILD_ID is the build-id for this mapped file, this will never be
   nullptr.  Not every mapped file will have a build-id, but there's no
   point calling this function if we failed to find a build-id; this
   structure only exists so we can lookup files based on their build-id.  */

void
mapped_file_info::add (const char *soname,
		       const char *expected_filename,
		       const char *actual_filename,
		       std::vector<mem_range> &&ranges,
		       const bfd_build_id *build_id)
{
  gdb_assert (build_id != nullptr);
  gdb_assert (expected_filename != nullptr);

  if (soname != nullptr)
    {
      /* If we already have an entry with this SONAME then this indicates
	 that the inferior has two files mapped into memory with different
	 file names (and most likely different build-ids), but with the
	 same DT_SONAME attribute.  In this case we can't use the
	 DT_SONAME to figure out the expected build-id of a shared
	 library, so poison the entry for this SONAME by setting the entry
	 to nullptr.  */
      auto it = m_soname_to_build_id_map.find (soname);
      if (it != m_soname_to_build_id_map.end ()
	  && it->second != nullptr
	  && !build_id_equal (it->second, build_id))
	m_soname_to_build_id_map[soname] = nullptr;
      else
	m_soname_to_build_id_map[soname] = build_id;
    }

  /* When the core file is initially opened and the mapped files are
     parsed, we group the build-id information based on the file name.  As
     a consequence, we should see each EXPECTED_FILENAME value exactly
     once.  This means that each insertion should always succeed.  */
  const auto inserted
    = m_filename_to_build_id_map.emplace (expected_filename, build_id).second;
  gdb_assert (inserted);

  /* Setup the reverse build-id to file name map.  */
  if (actual_filename != nullptr)
    m_build_id_to_filename_map.emplace (build_id, actual_filename);

  /* Setup the list of memory range to build-id objects.  */
  for (mem_range &r : ranges)
    m_address_to_build_id_list.emplace_back (std::move (r), build_id);

  /* At this point the m_address_to_build_id_list is unsorted (we just
     added some entries to the end of the list).  All entries should be
     added before any look-ups are performed, and the list is only sorted
     when the first look-up is performed.  */
  gdb_assert (!m_address_to_build_id_list_sorted);
}

/* FILENAME is the name of a file GDB is trying to load, and ADDR is
   (optionally) an address within the file in the inferior's address space.

   Search through the information gathered from the core-file's mapped file
   information looking for a file named FILENAME, or for a file that covers
   ADDR.  If a match is found then return the build-id for the file along
   with the location where GDB found the mapped file.

   The location of the mapped file might be the empty string if GDB was
   unable to find the mapped file.

   If no build-id can be found for FILENAME then GDB will return a pair
   containing nullptr (for the build-id) and an empty string for the file
   name.  */

std::optional <core_target_mapped_file_info>
mapped_file_info::lookup (const char *filename,
			  const std::optional<CORE_ADDR> &addr)
{
  if (filename != nullptr)
    {
      /* If there's a matching entry in m_filename_to_build_id_map then the
	 associated build-id will not be nullptr, and can be used to
	 validate that FILENAME is correct.  */
      auto it = m_filename_to_build_id_map.find (filename);
      if (it != m_filename_to_build_id_map.end ())
	return make_result (it->second);
    }

  if (addr.has_value ())
    {
      /* On the first lookup, sort the address_to_build_id_list.  */
      if (!m_address_to_build_id_list_sorted)
	{
	  std::sort (m_address_to_build_id_list.begin (),
		     m_address_to_build_id_list.end (),
		     [] (const mem_range_and_build_id &a,
			 const mem_range_and_build_id &b) {
		       return a.range < b.range;
		     });
	  m_address_to_build_id_list_sorted = true;
	}

      /* Look for the first entry whose range's start address is not less
	 than, or equal too, the address ADDR.  If we find such an entry,
	 then the previous entry's range might contain ADDR.  If it does
	 then that previous entry's build-id can be used.  */
      auto it = std::lower_bound
	(m_address_to_build_id_list.begin (),
	 m_address_to_build_id_list.end (),
	 *addr,
	 [] (const mem_range_and_build_id &a,
	     const CORE_ADDR &b) {
	  return a.range.start <= b;
	});

      if (it != m_address_to_build_id_list.begin ())
	{
	  --it;

	  if (it->range.contains (*addr))
	    return make_result (it->build_id);
	}
    }

  if (filename != nullptr)
    {
      /* If the basename of FILENAME appears in m_soname_to_build_id_map
	 then when the mapped files were processed, we saw a file with a
	 DT_SONAME attribute corresponding to FILENAME, use that build-id
	 to validate FILENAME.

	 However, the build-id in this map might be nullptr if we saw
	 multiple mapped files with the same DT_SONAME attribute (though
	 this should be pretty rare).  */
      auto it
	= m_soname_to_build_id_map.find (lbasename (filename));
      if (it != m_soname_to_build_id_map.end ()
	  && it->second != nullptr)
	return make_result (it->second);
    }

  return {};
}

/* See gdbcore.h.  */

std::optional <core_target_mapped_file_info>
core_target_find_mapped_file (const char *filename,
			      std::optional<CORE_ADDR> addr)
{
  core_target *targ = get_current_core_target ();
  if (targ == nullptr || current_program_space->cbfd.get () == nullptr)
    return {};

  return targ->lookup_mapped_file_info (filename, addr);
}

void _initialize_corelow ();
void
_initialize_corelow ()
{
  add_target (core_target_info, core_target_open,
	      filename_maybe_quoted_completer);
  add_cmd ("core-file-backed-mappings", class_maintenance,
	   maintenance_print_core_file_backed_mappings,
	   _("Print core file's file-backed mappings."),
	   &maintenanceprintlist);
}
