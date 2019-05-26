/* Core dump and executable file functions below target vector, for GDB.

   Copyright (C) 1986-2019 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include <signal.h>
#include <fcntl.h>
#include "frame.h"		/* required by inferior.h */
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
#include "readline/readline.h"
#include "solib.h"
#include "filenames.h"
#include "progspace.h"
#include "objfiles.h"
#include "gdb_bfd.h"
#include "completer.h"
#include "common/filestuff.h"

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

static core_fns *sniff_core_bfd (gdbarch *core_gdbarch,
				 bfd *abfd);

/* The core file target.  */

static const target_info core_target_info = {
  "core",
  N_("Local core dump file"),
  N_("Use a core file as a target.  Specify the filename of the core file.")
};

class core_target final : public process_stratum_target
{
public:
  core_target ();
  ~core_target () override;

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

  const char *pid_to_str (ptid_t) override;

  const char *thread_name (struct thread_info *) override;

  bool has_all_memory () override { return false; }
  bool has_memory () override;
  bool has_stack () override;
  bool has_registers () override;
  bool has_execution (ptid_t) override { return false; }

  bool info_proc (const char *, enum info_proc_what) override;

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
				  int which,
				  const char *human_name,
				  bool required);

private: /* per-core data */

  /* The core's section table.  Note that these target sections are
     *not* mapped in the current address spaces' set of target
     sections --- those should come only from pure executable or
     shared library bfds.  The core bfd sections are an implementation
     detail of the core target, just like ptrace is for unix child
     targets.  */
  target_section_table m_core_section_table {};

  /* The core_fns for a core file handler that is prepared to read the
     core file currently open on core_bfd.  */
  core_fns *m_core_vec = NULL;

  /* FIXME: kettenis/20031023: Eventually this field should
     disappear.  */
  struct gdbarch *m_core_gdbarch = NULL;
};

core_target::core_target ()
{
  m_core_gdbarch = gdbarch_from_bfd (core_bfd);

  /* Find a suitable core file handler to munch on core_bfd */
  m_core_vec = sniff_core_bfd (m_core_gdbarch, core_bfd);

  /* Find the data section */
  if (build_section_table (core_bfd,
			   &m_core_section_table.sections,
			   &m_core_section_table.sections_end))
    error (_("\"%s\": Can't find sections: %s"),
	   bfd_get_filename (core_bfd), bfd_errmsg (bfd_get_error ()));
}

core_target::~core_target ()
{
  xfree (m_core_section_table.sections);
}

/* List of all available core_fns.  On gdb startup, each core file
   register reader calls deprecated_add_core_fns() to register
   information on each core format it is prepared to read.  */

static struct core_fns *core_file_fns = NULL;

static int gdb_check_format (bfd *);

static void add_to_thread_list (bfd *, asection *, void *);

/* An arbitrary identifier for the core inferior.  */
#define CORELOW_PID 1

/* Link a new core_fns into the global core_file_fns list.  Called on
   gdb startup by the _initialize routine in each core file register
   reader, to register information about each format the reader is
   prepared to handle.  */

void
deprecated_add_core_fns (struct core_fns *cf)
{
  cf->next = core_file_fns;
  core_file_fns = cf;
}

/* The default function that core file handlers can use to examine a
   core file BFD and decide whether or not to accept the job of
   reading the core file.  */

int
default_core_sniffer (struct core_fns *our_fns, bfd *abfd)
{
  int result;

  result = (bfd_get_flavour (abfd) == our_fns -> core_flavour);
  return (result);
}

/* Walk through the list of core functions to find a set that can
   handle the core file open on ABFD.  Returns pointer to set that is
   selected.  */

static struct core_fns *
sniff_core_bfd (struct gdbarch *core_gdbarch, bfd *abfd)
{
  struct core_fns *cf;
  struct core_fns *yummy = NULL;
  int matches = 0;

  /* Don't sniff if we have support for register sets in
     CORE_GDBARCH.  */
  if (core_gdbarch && gdbarch_iterate_over_regset_sections_p (core_gdbarch))
    return NULL;

  for (cf = core_file_fns; cf != NULL; cf = cf->next)
    {
      if (cf->core_sniffer (cf, abfd))
	{
	  yummy = cf;
	  matches++;
	}
    }
  if (matches > 1)
    {
      warning (_("\"%s\": ambiguous core format, %d handlers match"),
	       bfd_get_filename (abfd), matches);
    }
  else if (matches == 0)
    error (_("\"%s\": no core file handler recognizes format"),
	   bfd_get_filename (abfd));

  return (yummy);
}

/* The default is to reject every core file format we see.  Either
   BFD has to recognize it, or we have to provide a function in the
   core file handler that recognizes it.  */

int
default_check_format (bfd *abfd)
{
  return (0);
}

/* Attempt to recognize core file formats that BFD rejects.  */

static int
gdb_check_format (bfd *abfd)
{
  struct core_fns *cf;

  for (cf = core_file_fns; cf != NULL; cf = cf->next)
    {
      if (cf->check_format (abfd))
	{
	  return (1);
	}
    }
  return (0);
}

/* Close the core target.  */

void
core_target::close ()
{
  if (core_bfd)
    {
      inferior_ptid = null_ptid;    /* Avoid confusion from thread
				       stuff.  */
      exit_inferior_silent (current_inferior ());

      /* Clear out solib state while the bfd is still open.  See
         comments in clear_solib in solib.c.  */
      clear_solib ();

      current_program_space->cbfd.reset (nullptr);
    }

  /* Core targets are heap-allocated (see core_target_open), so here
     we delete ourselves.  */
  delete this;
}

/* Look for sections whose names start with `.reg/' so that we can
   extract the list of threads in a core file.  */

static void
add_to_thread_list (bfd *abfd, asection *asect, void *reg_sect_arg)
{
  ptid_t ptid;
  int core_tid;
  int pid, lwpid;
  asection *reg_sect = (asection *) reg_sect_arg;
  int fake_pid_p = 0;
  struct inferior *inf;

  if (!startswith (bfd_section_name (abfd, asect), ".reg/"))
    return;

  core_tid = atoi (bfd_section_name (abfd, asect) + 5);

  pid = bfd_core_file_pid (core_bfd);
  if (pid == 0)
    {
      fake_pid_p = 1;
      pid = CORELOW_PID;
    }

  lwpid = core_tid;

  inf = current_inferior ();
  if (inf->pid == 0)
    {
      inferior_appeared (inf, pid);
      inf->fake_pid_p = fake_pid_p;
    }

  ptid = ptid_t (pid, lwpid, 0);

  add_thread (ptid);

/* Warning, Will Robinson, looking at BFD private data! */

  if (reg_sect != NULL
      && asect->filepos == reg_sect->filepos)	/* Did we find .reg?  */
    inferior_ptid = ptid;			/* Yes, make it current.  */
}

/* Issue a message saying we have no core to debug, if FROM_TTY.  */

static void
maybe_say_no_core_file_now (int from_tty)
{
  if (from_tty)
    printf_filtered (_("No core file now.\n"));
}

/* Backward compatability with old way of specifying core files.  */

void
core_file_command (const char *filename, int from_tty)
{
  dont_repeat ();		/* Either way, seems bogus.  */

  if (filename == NULL)
    {
      if (core_bfd != NULL)
	{
	  target_detach (current_inferior (), from_tty);
	  gdb_assert (core_bfd == NULL);
	}
      else
	maybe_say_no_core_file_now (from_tty);
    }
  else
    core_target_open (filename, from_tty);
}

/* See gdbcore.h.  */

void
core_target_open (const char *arg, int from_tty)
{
  const char *p;
  int siggy;
  int scratch_chan;
  int flags;

  target_preopen (from_tty);
  if (!arg)
    {
      if (core_bfd)
	error (_("No core file specified.  (Use `detach' "
		 "to stop debugging a core file.)"));
      else
	error (_("No core file specified."));
    }

  gdb::unique_xmalloc_ptr<char> filename (tilde_expand (arg));
  if (!IS_ABSOLUTE_PATH (filename.get ()))
    filename.reset (concat (current_directory, "/",
			    filename.get (), (char *) NULL));

  flags = O_BINARY | O_LARGEFILE;
  if (write_files)
    flags |= O_RDWR;
  else
    flags |= O_RDONLY;
  scratch_chan = gdb_open_cloexec (filename.get (), flags, 0);
  if (scratch_chan < 0)
    perror_with_name (filename.get ());

  gdb_bfd_ref_ptr temp_bfd (gdb_bfd_fopen (filename.get (), gnutarget,
					   write_files ? FOPEN_RUB : FOPEN_RB,
					   scratch_chan));
  if (temp_bfd == NULL)
    perror_with_name (filename.get ());

  if (!bfd_check_format (temp_bfd.get (), bfd_core)
      && !gdb_check_format (temp_bfd.get ()))
    {
      /* Do it after the err msg */
      /* FIXME: should be checking for errors from bfd_close (for one
         thing, on error it does not free all the storage associated
         with the bfd).  */
      error (_("\"%s\" is not a core dump: %s"),
	     filename.get (), bfd_errmsg (bfd_get_error ()));
    }

  current_program_space->cbfd = std::move (temp_bfd);

  core_target *target = new core_target ();

  /* Own the target until it is successfully pushed.  */
  target_ops_up target_holder (target);

  validate_files ();

  /* If we have no exec file, try to set the architecture from the
     core file.  We don't do this unconditionally since an exec file
     typically contains more information that helps us determine the
     architecture than a core file.  */
  if (!exec_bfd)
    set_gdbarch_from_file (core_bfd);

  push_target (std::move (target_holder));

  inferior_ptid = null_ptid;

  /* Need to flush the register cache (and the frame cache) from a
     previous debug session.  If inferior_ptid ends up the same as the
     last debug session --- e.g., b foo; run; gcore core1; step; gcore
     core2; core core1; core core2 --- then there's potential for
     get_current_regcache to return the cached regcache of the
     previous session, and the frame cache being stale.  */
  registers_changed ();

  /* Build up thread list from BFD sections, and possibly set the
     current thread to the .reg/NN section matching the .reg
     section.  */
  bfd_map_over_sections (core_bfd, add_to_thread_list,
			 bfd_get_section_by_name (core_bfd, ".reg"));

  if (inferior_ptid == null_ptid)
    {
      /* Either we found no .reg/NN section, and hence we have a
	 non-threaded core (single-threaded, from gdb's perspective),
	 or for some reason add_to_thread_list couldn't determine
	 which was the "main" thread.  The latter case shouldn't
	 usually happen, but we're dealing with input here, which can
	 always be broken in different ways.  */
      thread_info *thread = first_thread_of_inferior (current_inferior ());

      if (thread == NULL)
	{
	  inferior_appeared (current_inferior (), CORELOW_PID);
	  inferior_ptid = ptid_t (CORELOW_PID);
	  add_thread_silent (inferior_ptid);
	}
      else
	switch_to_thread (thread);
    }

  post_create_inferior (target, from_tty);

  /* Now go through the target stack looking for threads since there
     may be a thread_stratum target loaded on top of target core by
     now.  The layer above should claim threads found in the BFD
     sections.  */
  TRY
    {
      target_update_thread_list ();
    }

  CATCH (except, RETURN_MASK_ERROR)
    {
      exception_print (gdb_stderr, except);
    }
  END_CATCH

  p = bfd_core_file_failing_command (core_bfd);
  if (p)
    printf_filtered (_("Core was generated by `%s'.\n"), p);

  /* Clearing any previous state of convenience variables.  */
  clear_exit_convenience_vars ();

  siggy = bfd_core_file_failing_signal (core_bfd);
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

      printf_filtered (_("Program terminated with signal %s, %s.\n"),
		       gdb_signal_to_name (sig), gdb_signal_to_string (sig));

      /* Set the value of the internal variable $_exitsignal,
	 which holds the signal uncaught by the inferior.  */
      set_internalvar_integer (lookup_internalvar ("_exitsignal"),
			       siggy);
    }

  /* Fetch all registers from core file.  */
  target_fetch_registers (get_current_regcache (), -1);

  /* Now, set up the frame cache, and print the top of stack.  */
  reinit_frame_cache ();
  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);

  /* Current thread should be NUM 1 but the user does not know that.
     If a program is single threaded gdb in general does not mention
     anything about threads.  That is why the test is >= 2.  */
  if (thread_count () >= 2)
    {
      TRY
	{
	  thread_command (NULL, from_tty);
	}
      CATCH (except, RETURN_MASK_ERROR)
	{
	  exception_print (gdb_stderr, except);
	}
      END_CATCH
    }
}

void
core_target::detach (inferior *inf, int from_tty)
{
  /* Note that 'this' is dangling after this call.  unpush_target
     closes the target, and our close implementation deletes
     'this'.  */
  unpush_target (this);

  reinit_frame_cache ();
  maybe_say_no_core_file_now (from_tty);
}

/* Try to retrieve registers from a section in core_bfd, and supply
   them to m_core_vec->core_read_registers, as the register set
   numbered WHICH.

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
					int which,
					const char *human_name,
					bool required)
{
  struct bfd_section *section;
  bfd_size_type size;
  char *contents;
  bool variable_size_section = (regset != NULL
				&& regset->flags & REGSET_VARIABLE_SIZE);

  thread_section_name section_name (name, regcache->ptid ());

  section = bfd_get_section_by_name (core_bfd, section_name.c_str ());
  if (! section)
    {
      if (required)
	warning (_("Couldn't find %s registers in core file."),
		 human_name);
      return;
    }

  size = bfd_section_size (core_bfd, section);
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

  contents = (char *) alloca (size);
  if (! bfd_get_section_contents (core_bfd, section, contents,
				  (file_ptr) 0, size))
    {
      warning (_("Couldn't read %s registers from `%s' section in core file."),
	       human_name, section_name.c_str ());
      return;
    }

  if (regset != NULL)
    {
      regset->supply_regset (regset, regcache, -1, contents, size);
      return;
    }

  gdb_assert (m_core_vec != nullptr);
  m_core_vec->core_read_registers (regcache, contents, size, which,
				   ((CORE_ADDR)
				    bfd_section_vma (core_bfd, section)));
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
  auto *data = (get_core_registers_cb_data *) cb_data;
  bool required = false;
  bool variable_size_section = (regset != NULL
				&& regset->flags & REGSET_VARIABLE_SIZE);

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

  /* The 'which' parameter is only used when no regset is provided.
     Thus we just set it to -1. */
  data->target->get_core_register_section (data->regcache, regset, sect_name,
					   supply_size, -1, human_name,
					   required);
}

/* Get the registers out of a core file.  This is the machine-
   independent part.  Fetch_core_registers is the machine-dependent
   part, typically implemented in the xm-file for each
   architecture.  */

/* We just get all the registers, so we don't use regno.  */

void
core_target::fetch_registers (struct regcache *regcache, int regno)
{
  int i;
  struct gdbarch *gdbarch;

  if (!(m_core_gdbarch != nullptr
	&& gdbarch_iterate_over_regset_sections_p (m_core_gdbarch))
      && (m_core_vec == NULL || m_core_vec->core_read_registers == NULL))
    {
      fprintf_filtered (gdb_stderr,
		     "Can't fetch registers from this type of core file\n");
      return;
    }

  gdbarch = regcache->arch ();
  if (gdbarch_iterate_over_regset_sections_p (gdbarch))
    {
      get_core_registers_cb_data data = { this, regcache };
      gdbarch_iterate_over_regset_sections (gdbarch,
					    get_core_registers_cb,
					    (void *) &data, NULL);
    }
  else
    {
      get_core_register_section (regcache, NULL,
				 ".reg", 0, 0, "general-purpose", 1);
      get_core_register_section (regcache, NULL,
				 ".reg2", 0, 2, "floating-point", 0);
    }

  /* Mark all registers not found in the core as unavailable.  */
  for (i = 0; i < gdbarch_num_regs (regcache->arch ()); i++)
    if (regcache->get_register_status (i) == REG_UNKNOWN)
      regcache->raw_supply (i, NULL);
}

void
core_target::files_info ()
{
  print_section_info (&m_core_section_table, core_bfd);
}

struct spuid_list
{
  gdb_byte *buf;
  ULONGEST offset;
  LONGEST len;
  ULONGEST pos;
  ULONGEST written;
};

static void
add_to_spuid_list (bfd *abfd, asection *asect, void *list_p)
{
  struct spuid_list *list = (struct spuid_list *) list_p;
  enum bfd_endian byte_order
    = bfd_big_endian (abfd) ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE;
  int fd, pos = 0;

  sscanf (bfd_section_name (abfd, asect), "SPU/%d/regs%n", &fd, &pos);
  if (pos == 0)
    return;

  if (list->pos >= list->offset && list->pos + 4 <= list->offset + list->len)
    {
      store_unsigned_integer (list->buf + list->pos - list->offset,
			      4, byte_order, fd);
      list->written += 4;
    }
  list->pos += 4;
}

enum target_xfer_status
core_target::xfer_partial (enum target_object object, const char *annex,
			   gdb_byte *readbuf, const gdb_byte *writebuf,
			   ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
      return (section_table_xfer_memory_partial
	      (readbuf, writebuf,
	       offset, len, xfered_len,
	       m_core_section_table.sections,
	       m_core_section_table.sections_end,
	       NULL));

    case TARGET_OBJECT_AUXV:
      if (readbuf)
	{
	  /* When the aux vector is stored in core file, BFD
	     represents this with a fake section called ".auxv".  */

	  struct bfd_section *section;
	  bfd_size_type size;

	  section = bfd_get_section_by_name (core_bfd, ".auxv");
	  if (section == NULL)
	    return TARGET_XFER_E_IO;

	  size = bfd_section_size (core_bfd, section);
	  if (offset >= size)
	    return TARGET_XFER_EOF;
	  size -= offset;
	  if (size > len)
	    size = len;

	  if (size == 0)
	    return TARGET_XFER_EOF;
	  if (!bfd_get_section_contents (core_bfd, section, readbuf,
					 (file_ptr) offset, size))
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

	  section = bfd_get_section_by_name (core_bfd, ".wcookie");
	  if (section == NULL)
	    return TARGET_XFER_E_IO;

	  size = bfd_section_size (core_bfd, section);
	  if (offset >= size)
	    return TARGET_XFER_EOF;
	  size -= offset;
	  if (size > len)
	    size = len;

	  if (size == 0)
	    return TARGET_XFER_EOF;
	  if (!bfd_get_section_contents (core_bfd, section, readbuf,
					 (file_ptr) offset, size))
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
      /* FALL THROUGH */

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
      /* FALL THROUGH */

    case TARGET_OBJECT_SPU:
      if (readbuf && annex)
	{
	  /* When the SPU contexts are stored in a core file, BFD
	     represents this with a fake section called
	     "SPU/<annex>".  */

	  struct bfd_section *section;
	  bfd_size_type size;
	  char sectionstr[100];

	  xsnprintf (sectionstr, sizeof sectionstr, "SPU/%s", annex);

	  section = bfd_get_section_by_name (core_bfd, sectionstr);
	  if (section == NULL)
	    return TARGET_XFER_E_IO;

	  size = bfd_section_size (core_bfd, section);
	  if (offset >= size)
	    return TARGET_XFER_EOF;
	  size -= offset;
	  if (size > len)
	    size = len;

	  if (size == 0)
	    return TARGET_XFER_EOF;
	  if (!bfd_get_section_contents (core_bfd, section, readbuf,
					 (file_ptr) offset, size))
	    {
	      warning (_("Couldn't read SPU section in core file."));
	      return TARGET_XFER_E_IO;
	    }

	  *xfered_len = (ULONGEST) size;
	  return TARGET_XFER_OK;
	}
      else if (readbuf)
	{
	  /* NULL annex requests list of all present spuids.  */
	  struct spuid_list list;

	  list.buf = readbuf;
	  list.offset = offset;
	  list.len = len;
	  list.pos = 0;
	  list.written = 0;
	  bfd_map_over_sections (core_bfd, add_to_spuid_list, &list);

	  if (list.written == 0)
	    return TARGET_XFER_EOF;
	  else
	    {
	      *xfered_len = (ULONGEST) list.written;
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
   behaviour.
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
  if (m_core_gdbarch && gdbarch_core_read_description_p (m_core_gdbarch))
    {
      const struct target_desc *result;

      result = gdbarch_core_read_description (m_core_gdbarch, this, core_bfd);
      if (result != NULL)
	return result;
    }

  return this->beneath ()->read_description ();
}

const char *
core_target::pid_to_str (ptid_t ptid)
{
  static char buf[64];
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
  inf = find_inferior_ptid (ptid);
  if (inf != NULL && !inf->fake_pid_p)
    return normal_pid_to_str (ptid);

  /* No luck.  We simply don't have a valid PID to print.  */
  xsnprintf (buf, sizeof buf, "<main task>");
  return buf;
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
  return (core_bfd != NULL);
}

bool
core_target::has_stack ()
{
  return (core_bfd != NULL);
}

bool
core_target::has_registers ()
{
  return (core_bfd != NULL);
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

void
_initialize_corelow (void)
{
  add_target (core_target_info, core_target_open, filename_completer);
}
