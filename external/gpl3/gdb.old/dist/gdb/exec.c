/* Work with executable files, for GDB. 

   Copyright (C) 1988-2020 Free Software Foundation, Inc.

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
#include "frame.h"
#include "inferior.h"
#include "target.h"
#include "gdbcmd.h"
#include "language.h"
#include "filenames.h"
#include "symfile.h"
#include "objfiles.h"
#include "completer.h"
#include "value.h"
#include "exec.h"
#include "observable.h"
#include "arch-utils.h"
#include "gdbthread.h"
#include "progspace.h"
#include "progspace-and-thread.h"
#include "gdb_bfd.h"
#include "gcore.h"
#include "source.h"
#include "build-id.h"

#include <fcntl.h>
#include "readline/tilde.h"
#include "gdbcore.h"

#include <ctype.h>
#include <sys/stat.h>
#include "solist.h"
#include <algorithm>
#include "gdbsupport/pathstuff.h"
#include "cli/cli-style.h"

void (*deprecated_file_changed_hook) (const char *);

static const target_info exec_target_info = {
  "exec",
  N_("Local exec file"),
  N_("Use an executable file as a target.\n\
Specify the filename of the executable file.")
};

/* The target vector for executable files.  */

struct exec_target final : public target_ops
{
  const target_info &info () const override
  { return exec_target_info; }

  strata stratum () const override { return file_stratum; }

  void close () override;
  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;
  struct target_section_table *get_section_table () override;
  void files_info () override;

  bool has_memory () override;
  char *make_corefile_notes (bfd *, int *) override;
  int find_memory_regions (find_memory_region_ftype func, void *data) override;
};

static exec_target exec_ops;

/* How to handle a mismatch between the current exec file and the exec
   file determined from target.  */

static const char *const exec_file_mismatch_names[]
  = {"ask", "warn", "off", NULL };
enum exec_file_mismatch_mode
  {
    exec_file_mismatch_ask, exec_file_mismatch_warn, exec_file_mismatch_off
  };
static const char *exec_file_mismatch = exec_file_mismatch_names[0];
static enum exec_file_mismatch_mode exec_file_mismatch_mode
  = exec_file_mismatch_ask;

/* Show command.  */
static void
show_exec_file_mismatch_command (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (gdb_stdout,
		    _("exec-file-mismatch handling is currently \"%s\".\n"),
		    exec_file_mismatch_names[exec_file_mismatch_mode]);
}

/* Set command.  Change the setting for range checking.  */
static void
set_exec_file_mismatch_command (const char *ignore,
				int from_tty, struct cmd_list_element *c)
{
  for (enum exec_file_mismatch_mode mode = exec_file_mismatch_ask;
       ;
       mode = static_cast<enum exec_file_mismatch_mode>(1 + (int) mode))
    {
      if (strcmp (exec_file_mismatch, exec_file_mismatch_names[mode]) == 0)
	{
	  exec_file_mismatch_mode = mode;
	  return;
	}
      if (mode == exec_file_mismatch_off)
	internal_error (__FILE__, __LINE__,
			_("Unrecognized exec-file-mismatch setting: \"%s\""),
			exec_file_mismatch);
    }
}

/* Whether to open exec and core files read-only or read-write.  */

bool write_files = false;
static void
show_write_files (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Writing into executable and core files is %s.\n"),
		    value);
}


static void
exec_target_open (const char *args, int from_tty)
{
  target_preopen (from_tty);
  exec_file_attach (args, from_tty);
}

/* Close and clear exec_bfd.  If we end up with no target sections to
   read memory from, this unpushes the exec_ops target.  */

void
exec_close (void)
{
  if (exec_bfd)
    {
      bfd *abfd = exec_bfd;

      gdb_bfd_unref (abfd);

      /* Removing target sections may close the exec_ops target.
	 Clear exec_bfd before doing so to prevent recursion.  */
      exec_bfd = NULL;
      exec_bfd_mtime = 0;

      remove_target_sections (&exec_bfd);

      xfree (exec_filename);
      exec_filename = NULL;
    }
}

/* This is the target_close implementation.  Clears all target
   sections and closes all executable bfds from all program spaces.  */

void
exec_target::close ()
{
  scoped_restore_current_program_space restore_pspace;

  for (struct program_space *ss : program_spaces)
    {
      set_current_program_space (ss);
      clear_section_table (current_target_sections);
      exec_close ();
    }
}

/* See gdbcore.h.  */

void
try_open_exec_file (const char *exec_file_host, struct inferior *inf,
		    symfile_add_flags add_flags)
{
  struct gdb_exception prev_err;

  /* exec_file_attach and symbol_file_add_main may throw an error if the file
     cannot be opened either locally or remotely.

     This happens for example, when the file is first found in the local
     sysroot (above), and then disappears (a TOCTOU race), or when it doesn't
     exist in the target filesystem, or when the file does exist, but
     is not readable.

     Even without a symbol file, the remote-based debugging session should
     continue normally instead of ending abruptly.  Hence we catch thrown
     errors/exceptions in the following code.  */
  try
    {
      /* We must do this step even if exec_file_host is NULL, so that
	 exec_file_attach will clear state.  */
      exec_file_attach (exec_file_host, add_flags & SYMFILE_VERBOSE);
    }
  catch (gdb_exception_error &err)
    {
      if (err.message != NULL)
	warning ("%s", err.what ());

      prev_err = std::move (err);
    }

  if (exec_file_host != NULL)
    {
      try
	{
	  symbol_file_add_main (exec_file_host, add_flags);
	}
      catch (const gdb_exception_error &err)
	{
	  if (!exception_print_same (prev_err, err))
	    warning ("%s", err.what ());
	}
    }
}

/* See gdbcore.h.  */

void
validate_exec_file (int from_tty)
{
  /* If user asked to ignore the mismatch, do nothing.  */
  if (exec_file_mismatch_mode == exec_file_mismatch_off)
    return;

  const char *current_exec_file = get_exec_file (0);
  struct inferior *inf = current_inferior ();
  /* Try to determine a filename from the process itself.  */
  const char *pid_exec_file = target_pid_to_exec_file (inf->pid);
  bool build_id_mismatch = false;

  /* If we cannot validate the exec file, return.  */
  if (current_exec_file == NULL || pid_exec_file == NULL)
    return;

  /* Try validating via build-id, if available.  This is the most
     reliable check.  */

  /* In case current_exec_file was changed, reopen_exec_file ensures
     an up to date build_id (will do nothing if the file timestamp
     did not change).  If exec file changed, reopen_exec_file has
     allocated another file name, so get_exec_file again.  */
  reopen_exec_file ();
  current_exec_file = get_exec_file (0);

  const bfd_build_id *exec_file_build_id = build_id_bfd_get (exec_bfd);
  if (exec_file_build_id != nullptr)
    {
      /* Prepend the target prefix, to force gdb_bfd_open to open the
	 file on the remote file system (if indeed remote).  */
      std::string target_pid_exec_file
	= std::string (TARGET_SYSROOT_PREFIX) + pid_exec_file;

      gdb_bfd_ref_ptr abfd (gdb_bfd_open (target_pid_exec_file.c_str (),
					  gnutarget, -1, false));
      if (abfd != nullptr)
	{
	  const bfd_build_id *target_exec_file_build_id
	    = build_id_bfd_get (abfd.get ());

	  if (target_exec_file_build_id != nullptr)
	    {
	      if (exec_file_build_id->size == target_exec_file_build_id->size
		  && memcmp (exec_file_build_id->data,
			     target_exec_file_build_id->data,
			     exec_file_build_id->size) == 0)
		{
		  /* Match.  */
		  return;
		}
	      else
		build_id_mismatch = true;
	    }
	}
    }

  if (build_id_mismatch)
    {
      std::string exec_file_target (pid_exec_file);

      /* In case the exec file is not local, exec_file_target has to point at
	 the target file system.  */
      if (is_target_filename (current_exec_file) && !target_filesystem_is_local ())
	exec_file_target = TARGET_SYSROOT_PREFIX + exec_file_target;

      warning
	(_("Build ID mismatch between current exec-file %ps\n"
	   "and automatically determined exec-file %ps\n"
	   "exec-file-mismatch handling is currently \"%s\""),
	 styled_string (file_name_style.style (), current_exec_file),
	 styled_string (file_name_style.style (), exec_file_target.c_str ()),
	 exec_file_mismatch_names[exec_file_mismatch_mode]);
      if (exec_file_mismatch_mode == exec_file_mismatch_ask)
	{
	  symfile_add_flags add_flags = SYMFILE_MAINLINE;
	  if (from_tty)
	    {
	      add_flags |= SYMFILE_VERBOSE;
	      add_flags |= SYMFILE_ALWAYS_CONFIRM;
	    }
	  try
	    {
	      symbol_file_add_main (exec_file_target.c_str (), add_flags);
	      exec_file_attach (exec_file_target.c_str (), from_tty);
	    }
	  catch (gdb_exception_error &err)
	    {
	      warning (_("loading %ps %s"),
		       styled_string (file_name_style.style (),
				      exec_file_target.c_str ()),
		       err.message != NULL ? err.what () : "error");
	    }
	}
    }
}

/* See gdbcore.h.  */

void
exec_file_locate_attach (int pid, int defer_bp_reset, int from_tty)
{
  char *exec_file_target;
  symfile_add_flags add_flags = 0;

  /* Do nothing if we already have an executable filename.  */
  if (get_exec_file (0) != NULL)
    return;

  /* Try to determine a filename from the process itself.  */
  exec_file_target = target_pid_to_exec_file (pid);
  if (exec_file_target == NULL)
    {
      warning (_("No executable has been specified and target does not "
		 "support\n"
		 "determining executable automatically.  "
		 "Try using the \"file\" command."));
      return;
    }

  gdb::unique_xmalloc_ptr<char> exec_file_host
    = exec_file_find (exec_file_target, NULL);

  if (defer_bp_reset)
    add_flags |= SYMFILE_DEFER_BP_RESET;

  if (from_tty)
    add_flags |= SYMFILE_VERBOSE;

  /* Attempt to open the exec file.  */
  try_open_exec_file (exec_file_host.get (), current_inferior (), add_flags);
}

/* Set FILENAME as the new exec file.

   This function is intended to be behave essentially the same
   as exec_file_command, except that the latter will detect when
   a target is being debugged, and will ask the user whether it
   should be shut down first.  (If the answer is "no", then the
   new file is ignored.)

   This file is used by exec_file_command, to do the work of opening
   and processing the exec file after any prompting has happened.

   And, it is used by child_attach, when the attach command was
   given a pid but not a exec pathname, and the attach command could
   figure out the pathname from the pid.  (In this case, we shouldn't
   ask the user whether the current target should be shut down --
   we're supplying the exec pathname late for good reason.)  */

void
exec_file_attach (const char *filename, int from_tty)
{
  /* First, acquire a reference to the current exec_bfd.  We release
     this at the end of the function; but acquiring it now lets the
     BFD cache return it if this call refers to the same file.  */
  gdb_bfd_ref_ptr exec_bfd_holder = gdb_bfd_ref_ptr::new_reference (exec_bfd);

  /* Remove any previous exec file.  */
  exec_close ();

  /* Now open and digest the file the user requested, if any.  */

  if (!filename)
    {
      if (from_tty)
        printf_unfiltered (_("No executable file now.\n"));

      set_gdbarch_from_file (NULL);
    }
  else
    {
      int load_via_target = 0;
      const char *scratch_pathname, *canonical_pathname;
      int scratch_chan;
      struct target_section *sections = NULL, *sections_end = NULL;
      char **matching;

      if (is_target_filename (filename))
	{
	  if (target_filesystem_is_local ())
	    filename += strlen (TARGET_SYSROOT_PREFIX);
	  else
	    load_via_target = 1;
	}

      gdb::unique_xmalloc_ptr<char> canonical_storage, scratch_storage;
      if (load_via_target)
	{
	  /* gdb_bfd_fopen does not support "target:" filenames.  */
	  if (write_files)
	    warning (_("writing into executable files is "
		       "not supported for %s sysroots"),
		     TARGET_SYSROOT_PREFIX);

	  scratch_pathname = filename;
	  scratch_chan = -1;
	  canonical_pathname = scratch_pathname;
	}
      else
	{
	  scratch_chan = openp (getenv ("PATH"), OPF_TRY_CWD_FIRST,
				filename, write_files ?
				O_RDWR | O_BINARY : O_RDONLY | O_BINARY,
				&scratch_storage);
#if defined(__GO32__) || defined(_WIN32) || defined(__CYGWIN__)
	  if (scratch_chan < 0)
	    {
	      int first_errno = errno;
	      char *exename = (char *) alloca (strlen (filename) + 5);

	      strcat (strcpy (exename, filename), ".exe");
	      scratch_chan = openp (getenv ("PATH"), OPF_TRY_CWD_FIRST,
				    exename, write_files ?
				    O_RDWR | O_BINARY
				    : O_RDONLY | O_BINARY,
				    &scratch_storage);
	      if (scratch_chan < 0)
		errno = first_errno;
	    }
#endif
	  if (scratch_chan < 0)
	    perror_with_name (filename);

	  scratch_pathname = scratch_storage.get ();

	  /* gdb_bfd_open (and its variants) prefers canonicalized
	     pathname for better BFD caching.  */
	  canonical_storage = gdb_realpath (scratch_pathname);
	  canonical_pathname = canonical_storage.get ();
	}

      gdb_bfd_ref_ptr temp;
      if (write_files && !load_via_target)
	temp = gdb_bfd_fopen (canonical_pathname, gnutarget,
			      FOPEN_RUB, scratch_chan);
      else
	temp = gdb_bfd_open (canonical_pathname, gnutarget, scratch_chan);
      exec_bfd = temp.release ();

      if (!exec_bfd)
	{
	  error (_("\"%ps\": could not open as an executable file: %s."),
		 styled_string (file_name_style.style (), scratch_pathname),
		 bfd_errmsg (bfd_get_error ()));
	}

      /* gdb_realpath_keepfile resolves symlinks on the local
	 filesystem and so cannot be used for "target:" files.  */
      gdb_assert (exec_filename == NULL);
      if (load_via_target)
	exec_filename = xstrdup (bfd_get_filename (exec_bfd));
      else
	exec_filename = gdb_realpath_keepfile (scratch_pathname).release ();

      if (!bfd_check_format_matches (exec_bfd, bfd_object, &matching))
	{
	  /* Make sure to close exec_bfd, or else "run" might try to use
	     it.  */
	  exec_close ();
	  error (_("\"%ps\": not in executable format: %s"),
		 styled_string (file_name_style.style (), scratch_pathname),
		 gdb_bfd_errmsg (bfd_get_error (), matching).c_str ());
	}

      if (build_section_table (exec_bfd, &sections, &sections_end))
	{
	  /* Make sure to close exec_bfd, or else "run" might try to use
	     it.  */
	  exec_close ();
	  error (_("\"%ps\": can't find the file sections: %s"),
		 styled_string (file_name_style.style (), scratch_pathname),
		 bfd_errmsg (bfd_get_error ()));
	}

      exec_bfd_mtime = bfd_get_mtime (exec_bfd);

      validate_files ();

      set_gdbarch_from_file (exec_bfd);

      /* Add the executable's sections to the current address spaces'
	 list of sections.  This possibly pushes the exec_ops
	 target.  */
      add_target_sections (&exec_bfd, sections, sections_end);
      xfree (sections);

      /* Tell display code (if any) about the changed file name.  */
      if (deprecated_exec_file_display_hook)
	(*deprecated_exec_file_display_hook) (filename);
    }

  bfd_cache_close_all ();
  gdb::observers::executable_changed.notify ();
}

/*  Process the first arg in ARGS as the new exec file.

   Note that we have to explicitly ignore additional args, since we can
   be called from file_command(), which also calls symbol_file_command()
   which can take multiple args.
   
   If ARGS is NULL, we just want to close the exec file.  */

static void
exec_file_command (const char *args, int from_tty)
{
  if (from_tty && target_has_execution
      && !query (_("A program is being debugged already.\n"
		   "Are you sure you want to change the file? ")))
    error (_("File not changed."));

  if (args)
    {
      /* Scan through the args and pick up the first non option arg
         as the filename.  */

      gdb_argv built_argv (args);
      char **argv = built_argv.get ();

      for (; (*argv != NULL) && (**argv == '-'); argv++)
        {;
        }
      if (*argv == NULL)
        error (_("No executable file name was specified"));

      gdb::unique_xmalloc_ptr<char> filename (tilde_expand (*argv));
      exec_file_attach (filename.get (), from_tty);
    }
  else
    exec_file_attach (NULL, from_tty);
}

/* Set both the exec file and the symbol file, in one command.
   What a novelty.  Why did GDB go through four major releases before this
   command was added?  */

static void
file_command (const char *arg, int from_tty)
{
  /* FIXME, if we lose on reading the symbol file, we should revert
     the exec file, but that's rough.  */
  exec_file_command (arg, from_tty);
  symbol_file_command (arg, from_tty);
  if (deprecated_file_changed_hook)
    deprecated_file_changed_hook (arg);
}


/* Locate all mappable sections of a BFD file.
   table_pp_char is a char * to get it through bfd_map_over_sections;
   we cast it back to its proper type.  */

static void
add_to_section_table (bfd *abfd, struct bfd_section *asect,
		      void *table_pp_char)
{
  struct target_section **table_pp = (struct target_section **) table_pp_char;
  flagword aflag;

  gdb_assert (abfd == asect->owner);

  /* Check the section flags, but do not discard zero-length sections, since
     some symbols may still be attached to this section.  For instance, we
     encountered on sparc-solaris 2.10 a shared library with an empty .bss
     section to which a symbol named "_end" was attached.  The address
     of this symbol still needs to be relocated.  */
  aflag = bfd_section_flags (asect);
  if (!(aflag & SEC_ALLOC))
    return;

  (*table_pp)->owner = NULL;
  (*table_pp)->the_bfd_section = asect;
  (*table_pp)->addr = bfd_section_vma (asect);
  (*table_pp)->endaddr = (*table_pp)->addr + bfd_section_size (asect);
  (*table_pp)++;
}

/* See exec.h.  */

void
clear_section_table (struct target_section_table *table)
{
  xfree (table->sections);
  table->sections = table->sections_end = NULL;
}

/* Resize section table TABLE by ADJUSTMENT.
   ADJUSTMENT may be negative, in which case the caller must have already
   removed the sections being deleted.
   Returns the old size.  */

static int
resize_section_table (struct target_section_table *table, int adjustment)
{
  int old_count;
  int new_count;

  old_count = table->sections_end - table->sections;

  new_count = adjustment + old_count;

  if (new_count)
    {
      table->sections = XRESIZEVEC (struct target_section, table->sections,
				    new_count);
      table->sections_end = table->sections + new_count;
    }
  else
    clear_section_table (table);

  return old_count;
}

/* Builds a section table, given args BFD, SECTABLE_PTR, SECEND_PTR.
   Returns 0 if OK, 1 on error.  */

int
build_section_table (struct bfd *some_bfd, struct target_section **start,
		     struct target_section **end)
{
  unsigned count;

  count = bfd_count_sections (some_bfd);
  xfree (*start);
  *start = XNEWVEC (struct target_section, count);
  *end = *start;
  bfd_map_over_sections (some_bfd, add_to_section_table, (char *) end);

  gdb_assert (*end <= *start + count);

  /* We could realloc the table, but it probably loses for most files.  */
  return 0;
}

/* Add the sections array defined by [SECTIONS..SECTIONS_END[ to the
   current set of target sections.  */

void
add_target_sections (void *owner,
		     struct target_section *sections,
		     struct target_section *sections_end)
{
  int count;
  struct target_section_table *table = current_target_sections;

  count = sections_end - sections;

  if (count > 0)
    {
      int space = resize_section_table (table, count);
      int i;

      for (i = 0; i < count; ++i)
	{
	  table->sections[space + i] = sections[i];
	  table->sections[space + i].owner = owner;
	}

      scoped_restore_current_pspace_and_thread restore_pspace_thread;
      program_space *curr_pspace = current_program_space;

      /* If these are the first file sections we can provide memory
	 from, push the file_stratum target.  Must do this in all
	 inferiors sharing the program space.  */
      for (inferior *inf : all_inferiors ())
	{
	  if (inf->pspace != curr_pspace)
	    continue;

	  if (inf->target_is_pushed (&exec_ops))
	    continue;

	  switch_to_inferior_no_thread (inf);
	  push_target (&exec_ops);
	}
    }
}

/* Add the sections of OBJFILE to the current set of target sections.  */

void
add_target_sections_of_objfile (struct objfile *objfile)
{
  struct target_section_table *table = current_target_sections;
  struct obj_section *osect;
  int space;
  unsigned count = 0;
  struct target_section *ts;

  if (objfile == NULL)
    return;

  /* Compute the number of sections to add.  */
  ALL_OBJFILE_OSECTIONS (objfile, osect)
    {
      if (bfd_section_size (osect->the_bfd_section) == 0)
	continue;
      count++;
    }

  if (count == 0)
    return;

  space = resize_section_table (table, count);

  ts = table->sections + space;

  ALL_OBJFILE_OSECTIONS (objfile, osect)
    {
      if (bfd_section_size (osect->the_bfd_section) == 0)
	continue;

      gdb_assert (ts < table->sections + space + count);

      ts->addr = obj_section_addr (osect);
      ts->endaddr = obj_section_endaddr (osect);
      ts->the_bfd_section = osect->the_bfd_section;
      ts->owner = (void *) objfile;

      ts++;
    }
}

/* Remove all target sections owned by OWNER.
   OWNER must be the same value passed to add_target_sections.  */

void
remove_target_sections (void *owner)
{
  struct target_section *src, *dest;
  struct target_section_table *table = current_target_sections;

  gdb_assert (owner != NULL);

  dest = table->sections;
  for (src = table->sections; src < table->sections_end; src++)
    if (src->owner != owner)
      {
	/* Keep this section.  */
	if (dest < src)
	  *dest = *src;
	dest++;
      }

  /* If we've dropped any sections, resize the section table.  */
  if (dest < src)
    {
      int old_count;

      old_count = resize_section_table (table, dest - src);

      /* If we don't have any more sections to read memory from,
	 remove the file_stratum target from the stack of each
	 inferior sharing the program space.  */
      if (old_count + (dest - src) == 0)
	{
	  scoped_restore_current_pspace_and_thread restore_pspace_thread;
	  program_space *curr_pspace = current_program_space;

	  for (inferior *inf : all_inferiors ())
	    {
	      if (inf->pspace != curr_pspace)
		continue;

	      if (inf->pspace->target_sections.sections
		  != inf->pspace->target_sections.sections_end)
		continue;

	      switch_to_inferior_no_thread (inf);
	      unpush_target (&exec_ops);
	    }
	}
    }
}

/* See exec.h.  */

void
exec_on_vfork ()
{
  if (current_program_space->target_sections.sections
      != current_program_space->target_sections.sections_end)
    push_target (&exec_ops);
}



enum target_xfer_status
exec_read_partial_read_only (gdb_byte *readbuf, ULONGEST offset,
			     ULONGEST len, ULONGEST *xfered_len)
{
  /* It's unduly pedantic to refuse to look at the executable for
     read-only pieces; so do the equivalent of readonly regions aka
     QTro packet.  */
  if (exec_bfd != NULL)
    {
      asection *s;
      bfd_size_type size;
      bfd_vma vma;

      for (s = exec_bfd->sections; s; s = s->next)
	{
	  if ((s->flags & SEC_LOAD) == 0
	      || (s->flags & SEC_READONLY) == 0)
	    continue;

	  vma = s->vma;
	  size = bfd_section_size (s);
	  if (vma <= offset && offset < (vma + size))
	    {
	      ULONGEST amt;

	      amt = (vma + size) - offset;
	      if (amt > len)
		amt = len;

	      amt = bfd_get_section_contents (exec_bfd, s,
					      readbuf, offset - vma, amt);

	      if (amt == 0)
		return TARGET_XFER_EOF;
	      else
		{
		  *xfered_len = amt;
		  return TARGET_XFER_OK;
		}
	    }
	}
    }

  /* Indicate failure to find the requested memory block.  */
  return TARGET_XFER_E_IO;
}

/* Return all read-only memory ranges found in the target section
   table defined by SECTIONS and SECTIONS_END, starting at (and
   intersected with) MEMADDR for LEN bytes.  */

static std::vector<mem_range>
section_table_available_memory (CORE_ADDR memaddr, ULONGEST len,
				struct target_section *sections,
				struct target_section *sections_end)
{
  std::vector<mem_range> memory;

  for (target_section *p = sections; p < sections_end; p++)
    {
      if ((bfd_section_flags (p->the_bfd_section) & SEC_READONLY) == 0)
	continue;

      /* Copy the meta-data, adjusted.  */
      if (mem_ranges_overlap (p->addr, p->endaddr - p->addr, memaddr, len))
	{
	  ULONGEST lo1, hi1, lo2, hi2;

	  lo1 = memaddr;
	  hi1 = memaddr + len;

	  lo2 = p->addr;
	  hi2 = p->endaddr;

	  CORE_ADDR start = std::max (lo1, lo2);
	  int length = std::min (hi1, hi2) - start;

	  memory.emplace_back (start, length);
	}
    }

  return memory;
}

enum target_xfer_status
section_table_read_available_memory (gdb_byte *readbuf, ULONGEST offset,
				     ULONGEST len, ULONGEST *xfered_len)
{
  target_section_table *table = target_get_section_table (&exec_ops);
  std::vector<mem_range> available_memory
    = section_table_available_memory (offset, len,
				      table->sections, table->sections_end);

  normalize_mem_ranges (&available_memory);

  for (const mem_range &r : available_memory)
    {
      if (mem_ranges_overlap (r.start, r.length, offset, len))
	{
	  CORE_ADDR end;
	  enum target_xfer_status status;

	  /* Get the intersection window.  */
	  end = std::min<CORE_ADDR> (offset + len, r.start + r.length);

	  gdb_assert (end - offset <= len);

	  if (offset >= r.start)
	    status = exec_read_partial_read_only (readbuf, offset,
						  end - offset,
						  xfered_len);
	  else
	    {
	      *xfered_len = r.start - offset;
	      status = TARGET_XFER_UNAVAILABLE;
	    }
	  return status;
	}
    }

  *xfered_len = len;
  return TARGET_XFER_UNAVAILABLE;
}

enum target_xfer_status
section_table_xfer_memory_partial (gdb_byte *readbuf, const gdb_byte *writebuf,
				   ULONGEST offset, ULONGEST len,
				   ULONGEST *xfered_len,
				   struct target_section *sections,
				   struct target_section *sections_end,
				   gdb::function_view<bool
				     (const struct target_section *)> match_cb)
{
  int res;
  struct target_section *p;
  ULONGEST memaddr = offset;
  ULONGEST memend = memaddr + len;

  gdb_assert (len != 0);

  for (p = sections; p < sections_end; p++)
    {
      struct bfd_section *asect = p->the_bfd_section;
      bfd *abfd = asect->owner;

      if (match_cb != nullptr && !match_cb (p))
	continue;		/* not the section we need.  */
      if (memaddr >= p->addr)
        {
	  if (memend <= p->endaddr)
	    {
	      /* Entire transfer is within this section.  */
	      if (writebuf)
		res = bfd_set_section_contents (abfd, asect,
						writebuf, memaddr - p->addr,
						len);
	      else
		res = bfd_get_section_contents (abfd, asect,
						readbuf, memaddr - p->addr,
						len);

	      if (res != 0)
		{
		  *xfered_len = len;
		  return TARGET_XFER_OK;
		}
	      else
		return TARGET_XFER_EOF;
	    }
	  else if (memaddr >= p->endaddr)
	    {
	      /* This section ends before the transfer starts.  */
	      continue;
	    }
	  else
	    {
	      /* This section overlaps the transfer.  Just do half.  */
	      len = p->endaddr - memaddr;
	      if (writebuf)
		res = bfd_set_section_contents (abfd, asect,
						writebuf, memaddr - p->addr,
						len);
	      else
		res = bfd_get_section_contents (abfd, asect,
						readbuf, memaddr - p->addr,
						len);
	      if (res != 0)
		{
		  *xfered_len = len;
		  return TARGET_XFER_OK;
		}
	      else
		return TARGET_XFER_EOF;
	    }
        }
    }

  return TARGET_XFER_EOF;		/* We can't help.  */
}

struct target_section_table *
exec_target::get_section_table ()
{
  return current_target_sections;
}

enum target_xfer_status
exec_target::xfer_partial (enum target_object object,
			   const char *annex, gdb_byte *readbuf,
			   const gdb_byte *writebuf,
			   ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  struct target_section_table *table = get_section_table ();

  if (object == TARGET_OBJECT_MEMORY)
    return section_table_xfer_memory_partial (readbuf, writebuf,
					      offset, len, xfered_len,
					      table->sections,
					      table->sections_end);
  else
    return TARGET_XFER_E_IO;
}


void
print_section_info (struct target_section_table *t, bfd *abfd)
{
  struct gdbarch *gdbarch = gdbarch_from_bfd (abfd);
  struct target_section *p;
  /* FIXME: 16 is not wide enough when gdbarch_addr_bit > 64.  */
  int wid = gdbarch_addr_bit (gdbarch) <= 32 ? 8 : 16;

  printf_filtered ("\t`%ps', ",
		   styled_string (file_name_style.style (),
				  bfd_get_filename (abfd)));
  wrap_here ("        ");
  printf_filtered (_("file type %s.\n"), bfd_get_target (abfd));
  if (abfd == exec_bfd)
    {
      /* gcc-3.4 does not like the initialization in
	 <p == t->sections_end>.  */
      bfd_vma displacement = 0;
      bfd_vma entry_point;

      for (p = t->sections; p < t->sections_end; p++)
	{
	  struct bfd_section *psect = p->the_bfd_section;

	  if ((bfd_section_flags (psect) & (SEC_ALLOC | SEC_LOAD))
	      != (SEC_ALLOC | SEC_LOAD))
	    continue;

	  if (bfd_section_vma (psect) <= abfd->start_address
	      && abfd->start_address < (bfd_section_vma (psect)
					+ bfd_section_size (psect)))
	    {
	      displacement = p->addr - bfd_section_vma (psect);
	      break;
	    }
	}
      if (p == t->sections_end)
	warning (_("Cannot find section for the entry point of %ps."),
		 styled_string (file_name_style.style (),
				bfd_get_filename (abfd)));

      entry_point = gdbarch_addr_bits_remove (gdbarch, 
					      bfd_get_start_address (abfd) 
						+ displacement);
      printf_filtered (_("\tEntry point: %s\n"),
		       paddress (gdbarch, entry_point));
    }
  for (p = t->sections; p < t->sections_end; p++)
    {
      struct bfd_section *psect = p->the_bfd_section;
      bfd *pbfd = psect->owner;

      printf_filtered ("\t%s", hex_string_custom (p->addr, wid));
      printf_filtered (" - %s", hex_string_custom (p->endaddr, wid));

      /* FIXME: A format of "08l" is not wide enough for file offsets
	 larger than 4GB.  OTOH, making it "016l" isn't desirable either
	 since most output will then be much wider than necessary.  It
	 may make sense to test the size of the file and choose the
	 format string accordingly.  */
      /* FIXME: i18n: Need to rewrite this sentence.  */
      if (info_verbose)
	printf_filtered (" @ %s",
			 hex_string_custom (psect->filepos, 8));
      printf_filtered (" is %s", bfd_section_name (psect));
      if (pbfd != abfd)
	printf_filtered (" in %ps",
			 styled_string (file_name_style.style (),
					bfd_get_filename (pbfd)));
      printf_filtered ("\n");
    }
}

void
exec_target::files_info ()
{
  if (exec_bfd)
    print_section_info (current_target_sections, exec_bfd);
  else
    puts_filtered (_("\t<no file loaded>\n"));
}

static void
set_section_command (const char *args, int from_tty)
{
  struct target_section *p;
  const char *secname;
  unsigned seclen;
  unsigned long secaddr;
  char secprint[100];
  long offset;
  struct target_section_table *table;

  if (args == 0)
    error (_("Must specify section name and its virtual address"));

  /* Parse out section name.  */
  for (secname = args; !isspace (*args); args++);
  seclen = args - secname;

  /* Parse out new virtual address.  */
  secaddr = parse_and_eval_address (args);

  table = current_target_sections;
  for (p = table->sections; p < table->sections_end; p++)
    {
      if (!strncmp (secname, bfd_section_name (p->the_bfd_section), seclen)
	  && bfd_section_name (p->the_bfd_section)[seclen] == '\0')
	{
	  offset = secaddr - p->addr;
	  p->addr += offset;
	  p->endaddr += offset;
	  if (from_tty)
	    exec_ops.files_info ();
	  return;
	}
    }
  if (seclen >= sizeof (secprint))
    seclen = sizeof (secprint) - 1;
  strncpy (secprint, secname, seclen);
  secprint[seclen] = '\0';
  error (_("Section %s not found"), secprint);
}

/* If we can find a section in FILENAME with BFD index INDEX, adjust
   it to ADDRESS.  */

void
exec_set_section_address (const char *filename, int index, CORE_ADDR address)
{
  struct target_section *p;
  struct target_section_table *table;

  table = current_target_sections;
  for (p = table->sections; p < table->sections_end; p++)
    {
      if (filename_cmp (filename,
			bfd_get_filename (p->the_bfd_section->owner)) == 0
	  && index == p->the_bfd_section->index)
	{
	  p->endaddr += address - p->addr;
	  p->addr = address;
	}
    }
}

bool
exec_target::has_memory ()
{
  /* We can provide memory if we have any file/target sections to read
     from.  */
  return (current_target_sections->sections
	  != current_target_sections->sections_end);
}

char *
exec_target::make_corefile_notes (bfd *obfd, int *note_size)
{
  error (_("Can't create a corefile"));
}

int
exec_target::find_memory_regions (find_memory_region_ftype func, void *data)
{
  return objfile_find_memory_regions (this, func, data);
}

void _initialize_exec ();
void
_initialize_exec ()
{
  struct cmd_list_element *c;

  if (!dbx_commands)
    {
      c = add_cmd ("file", class_files, file_command, _("\
Use FILE as program to be debugged.\n\
It is read for its symbols, for getting the contents of pure memory,\n\
and it is the program executed when you use the `run' command.\n\
If FILE cannot be found as specified, your execution directory path\n\
($PATH) is searched for a command of that name.\n\
No arg means to have no executable file and no symbols."), &cmdlist);
      set_cmd_completer (c, filename_completer);
    }

  c = add_cmd ("exec-file", class_files, exec_file_command, _("\
Use FILE as program for getting contents of pure memory.\n\
If FILE cannot be found as specified, your execution directory path\n\
is searched for a command of that name.\n\
No arg means have no executable file."), &cmdlist);
  set_cmd_completer (c, filename_completer);

  add_com ("section", class_files, set_section_command, _("\
Change the base address of section SECTION of the exec file to ADDR.\n\
This can be used if the exec file does not contain section addresses,\n\
(such as in the a.out format), or when the addresses specified in the\n\
file itself are wrong.  Each section must be changed separately.  The\n\
``info files'' command lists all the sections and their addresses."));

  add_setshow_boolean_cmd ("write", class_support, &write_files, _("\
Set writing into executable and core files."), _("\
Show writing into executable and core files."), NULL,
			   NULL,
			   show_write_files,
			   &setlist, &showlist);

  add_setshow_enum_cmd ("exec-file-mismatch", class_support,
			exec_file_mismatch_names,
			&exec_file_mismatch,
			_("\
Set exec-file-mismatch handling (ask|warn|off)."),
			_("\
Show exec-file-mismatch handling (ask|warn|off)."),
			_("\
Specifies how to handle a mismatch between the current exec-file\n\
loaded by GDB and the exec-file automatically determined when attaching\n\
to a process:\n\n\
 ask  - warn the user and ask whether to load the determined exec-file.\n\
 warn - warn the user, but do not change the exec-file.\n\
 off  - do not check for mismatch.\n\
\n\
GDB detects a mismatch by comparing the build IDs of the files.\n\
If the user confirms loading the determined exec-file, then its symbols\n\
will be loaded as well."),
			set_exec_file_mismatch_command,
			show_exec_file_mismatch_command,
			&setlist, &showlist);

  add_target (exec_target_info, exec_target_open, filename_completer);
}
