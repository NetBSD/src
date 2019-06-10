/* Work with executable files, for GDB. 

   Copyright (C) 1988-2017 Free Software Foundation, Inc.

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
#include "observer.h"
#include "arch-utils.h"
#include "gdbthread.h"
#include "progspace.h"
#include "gdb_bfd.h"
#include "gcore.h"

#include <fcntl.h>
#include "readline/readline.h"
#include "gdbcore.h"

#include <ctype.h>
#include <sys/stat.h>
#include "solist.h"
#include <algorithm>

void (*deprecated_file_changed_hook) (char *);

/* Prototypes for local functions */

static void file_command (char *, int);

static void set_section_command (char *, int);

static void exec_files_info (struct target_ops *);

static void init_exec_ops (void);

void _initialize_exec (void);

/* The target vector for executable files.  */

static struct target_ops exec_ops;

/* Whether to open exec and core files read-only or read-write.  */

int write_files = 0;
static void
show_write_files (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Writing into executable and core files is %s.\n"),
		    value);
}


static void
exec_open (const char *args, int from_tty)
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

static void
exec_close_1 (struct target_ops *self)
{
  struct program_space *ss;
  struct cleanup *old_chain;

  old_chain = save_current_program_space ();
  ALL_PSPACES (ss)
  {
    set_current_program_space (ss);
    clear_section_table (current_target_sections);
    exec_close ();
  }

  do_cleanups (old_chain);
}

void
exec_file_clear (int from_tty)
{
  /* Remove exec file.  */
  exec_close ();

  if (from_tty)
    printf_unfiltered (_("No executable file now.\n"));
}

/* See exec.h.  */

void
try_open_exec_file (const char *exec_file_host, struct inferior *inf,
		    symfile_add_flags add_flags)
{
  struct cleanup *old_chain;
  struct gdb_exception prev_err = exception_none;

  old_chain = make_cleanup (free_current_contents, &prev_err.message);

  /* exec_file_attach and symbol_file_add_main may throw an error if the file
     cannot be opened either locally or remotely.

     This happens for example, when the file is first found in the local
     sysroot (above), and then disappears (a TOCTOU race), or when it doesn't
     exist in the target filesystem, or when the file does exist, but
     is not readable.

     Even without a symbol file, the remote-based debugging session should
     continue normally instead of ending abruptly.  Hence we catch thrown
     errors/exceptions in the following code.  */
  TRY
    {
      /* We must do this step even if exec_file_host is NULL, so that
	 exec_file_attach will clear state.  */
      exec_file_attach (exec_file_host, add_flags & SYMFILE_VERBOSE);
    }
  CATCH (err, RETURN_MASK_ERROR)
    {
      if (err.message != NULL)
	warning ("%s", err.message);

      prev_err = err;

      /* Save message so it doesn't get trashed by the catch below.  */
      if (err.message != NULL)
	prev_err.message = xstrdup (err.message);
    }
  END_CATCH

  if (exec_file_host != NULL)
    {
      TRY
	{
	  symbol_file_add_main (exec_file_host, add_flags);
	}
      CATCH (err, RETURN_MASK_ERROR)
	{
	  if (!exception_print_same (prev_err, err))
	    warning ("%s", err.message);
	}
      END_CATCH
    }

  do_cleanups (old_chain);
}

/* See gdbcore.h.  */

void
exec_file_locate_attach (int pid, int defer_bp_reset, int from_tty)
{
  char *exec_file_target, *exec_file_host;
  struct cleanup *old_chain;
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

  exec_file_host = exec_file_find (exec_file_target, NULL);
  old_chain = make_cleanup (xfree, exec_file_host);

  if (defer_bp_reset)
    add_flags |= SYMFILE_DEFER_BP_RESET;

  if (from_tty)
    add_flags |= SYMFILE_VERBOSE;

  /* Attempt to open the exec file.  */
  try_open_exec_file (exec_file_host, current_inferior (), add_flags);
  do_cleanups (old_chain);
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
  struct cleanup *cleanups;

  /* First, acquire a reference to the current exec_bfd.  We release
     this at the end of the function; but acquiring it now lets the
     BFD cache return it if this call refers to the same file.  */
  gdb_bfd_ref (exec_bfd);
  gdb_bfd_ref_ptr exec_bfd_holder (exec_bfd);

  cleanups = make_cleanup (null_cleanup, NULL);

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
      char *scratch_pathname, *canonical_pathname;
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

      if (load_via_target)
	{
	  /* gdb_bfd_fopen does not support "target:" filenames.  */
	  if (write_files)
	    warning (_("writing into executable files is "
		       "not supported for %s sysroots"),
		     TARGET_SYSROOT_PREFIX);

	  scratch_pathname = xstrdup (filename);
	  make_cleanup (xfree, scratch_pathname);

	  scratch_chan = -1;

	  canonical_pathname = scratch_pathname;
	}
      else
	{
	  scratch_chan = openp (getenv ("PATH"), OPF_TRY_CWD_FIRST,
				filename, write_files ?
				O_RDWR | O_BINARY : O_RDONLY | O_BINARY,
				&scratch_pathname);
#if defined(__GO32__) || defined(_WIN32) || defined(__CYGWIN__)
	  if (scratch_chan < 0)
	    {
	      char *exename = (char *) alloca (strlen (filename) + 5);

	      strcat (strcpy (exename, filename), ".exe");
	      scratch_chan = openp (getenv ("PATH"), OPF_TRY_CWD_FIRST,
				    exename, write_files ?
				    O_RDWR | O_BINARY
				    : O_RDONLY | O_BINARY,
				    &scratch_pathname);
	    }
#endif
	  if (scratch_chan < 0)
	    perror_with_name (filename);

	  make_cleanup (xfree, scratch_pathname);

	  /* gdb_bfd_open (and its variants) prefers canonicalized
	     pathname for better BFD caching.  */
	  canonical_pathname = gdb_realpath (scratch_pathname);
	  make_cleanup (xfree, canonical_pathname);
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
	  error (_("\"%s\": could not open as an executable file: %s."),
		 scratch_pathname, bfd_errmsg (bfd_get_error ()));
	}

      /* gdb_realpath_keepfile resolves symlinks on the local
	 filesystem and so cannot be used for "target:" files.  */
      gdb_assert (exec_filename == NULL);
      if (load_via_target)
	exec_filename = xstrdup (bfd_get_filename (exec_bfd));
      else
	exec_filename = gdb_realpath_keepfile (scratch_pathname);

      if (!bfd_check_format_matches (exec_bfd, bfd_object, &matching))
	{
	  /* Make sure to close exec_bfd, or else "run" might try to use
	     it.  */
	  exec_close ();
	  error (_("\"%s\": not in executable format: %s"),
		 scratch_pathname,
		 gdb_bfd_errmsg (bfd_get_error (), matching));
	}

      if (build_section_table (exec_bfd, &sections, &sections_end))
	{
	  /* Make sure to close exec_bfd, or else "run" might try to use
	     it.  */
	  exec_close ();
	  error (_("\"%s\": can't find the file sections: %s"),
		 scratch_pathname, bfd_errmsg (bfd_get_error ()));
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

  do_cleanups (cleanups);

  bfd_cache_close_all ();
  observer_notify_executable_changed ();
}

/*  Process the first arg in ARGS as the new exec file.

   Note that we have to explicitly ignore additional args, since we can
   be called from file_command(), which also calls symbol_file_command()
   which can take multiple args.
   
   If ARGS is NULL, we just want to close the exec file.  */

static void
exec_file_command (char *args, int from_tty)
{
  char **argv;
  char *filename;

  if (from_tty && target_has_execution
      && !query (_("A program is being debugged already.\n"
		   "Are you sure you want to change the file? ")))
    error (_("File not changed."));

  if (args)
    {
      struct cleanup *cleanups;

      /* Scan through the args and pick up the first non option arg
         as the filename.  */

      argv = gdb_buildargv (args);
      cleanups = make_cleanup_freeargv (argv);

      for (; (*argv != NULL) && (**argv == '-'); argv++)
        {;
        }
      if (*argv == NULL)
        error (_("No executable file name was specified"));

      filename = tilde_expand (*argv);
      make_cleanup (xfree, filename);
      exec_file_attach (filename, from_tty);

      do_cleanups (cleanups);
    }
  else
    exec_file_attach (NULL, from_tty);
}

/* Set both the exec file and the symbol file, in one command.
   What a novelty.  Why did GDB go through four major releases before this
   command was added?  */

static void
file_command (char *arg, int from_tty)
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
  aflag = bfd_get_section_flags (abfd, asect);
  if (!(aflag & SEC_ALLOC))
    return;

  (*table_pp)->owner = NULL;
  (*table_pp)->the_bfd_section = asect;
  (*table_pp)->addr = bfd_section_vma (abfd, asect);
  (*table_pp)->endaddr = (*table_pp)->addr + bfd_section_size (abfd, asect);
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
  if (*start)
    xfree (* start);
  *start = XNEWVEC (struct target_section, count);
  *end = *start;
  bfd_map_over_sections (some_bfd, add_to_section_table, (char *) end);
  if (*end > *start + count)
    internal_error (__FILE__, __LINE__,
		    _("failed internal consistency check"));
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

      /* If these are the first file sections we can provide memory
	 from, push the file_stratum target.  */
      if (!target_is_pushed (&exec_ops))
	push_target (&exec_ops);
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
      if (bfd_get_section_size (osect->the_bfd_section) == 0)
	continue;
      count++;
    }

  if (count == 0)
    return;

  space = resize_section_table (table, count);

  ts = table->sections + space;

  ALL_OBJFILE_OSECTIONS (objfile, osect)
    {
      if (bfd_get_section_size (osect->the_bfd_section) == 0)
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
	 remove the file_stratum target from the stack.  */
      if (old_count + (dest - src) == 0)
	{
	  struct program_space *pspace;

	  ALL_PSPACES (pspace)
	    if (pspace->target_sections.sections
		!= pspace->target_sections.sections_end)
	      return;

	  unpush_target (&exec_ops);
	}
    }
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
	  size = bfd_get_section_size (s);
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

/* Appends all read-only memory ranges found in the target section
   table defined by SECTIONS and SECTIONS_END, starting at (and
   intersected with) MEMADDR for LEN bytes.  Returns the augmented
   VEC.  */

static VEC(mem_range_s) *
section_table_available_memory (VEC(mem_range_s) *memory,
				CORE_ADDR memaddr, ULONGEST len,
				struct target_section *sections,
				struct target_section *sections_end)
{
  struct target_section *p;

  for (p = sections; p < sections_end; p++)
    {
      if ((bfd_get_section_flags (p->the_bfd_section->owner,
				  p->the_bfd_section)
	   & SEC_READONLY) == 0)
	continue;

      /* Copy the meta-data, adjusted.  */
      if (mem_ranges_overlap (p->addr, p->endaddr - p->addr, memaddr, len))
	{
	  ULONGEST lo1, hi1, lo2, hi2;
	  struct mem_range *r;

	  lo1 = memaddr;
	  hi1 = memaddr + len;

	  lo2 = p->addr;
	  hi2 = p->endaddr;

	  r = VEC_safe_push (mem_range_s, memory, NULL);

	  r->start = std::max (lo1, lo2);
	  r->length = std::min (hi1, hi2) - r->start;
	}
    }

  return memory;
}

enum target_xfer_status
section_table_read_available_memory (gdb_byte *readbuf, ULONGEST offset,
				     ULONGEST len, ULONGEST *xfered_len)
{
  VEC(mem_range_s) *available_memory = NULL;
  struct target_section_table *table;
  struct cleanup *old_chain;
  mem_range_s *r;
  int i;

  table = target_get_section_table (&exec_ops);
  available_memory = section_table_available_memory (available_memory,
						     offset, len,
						     table->sections,
						     table->sections_end);

  old_chain = make_cleanup (VEC_cleanup(mem_range_s),
			    &available_memory);

  normalize_mem_ranges (available_memory);

  for (i = 0;
       VEC_iterate (mem_range_s, available_memory, i, r);
       i++)
    {
      if (mem_ranges_overlap (r->start, r->length, offset, len))
	{
	  CORE_ADDR end;
	  enum target_xfer_status status;

	  /* Get the intersection window.  */
	  end = std::min<CORE_ADDR> (offset + len, r->start + r->length);

	  gdb_assert (end - offset <= len);

	  if (offset >= r->start)
	    status = exec_read_partial_read_only (readbuf, offset,
						  end - offset,
						  xfered_len);
	  else
	    {
	      *xfered_len = r->start - offset;
	      status = TARGET_XFER_UNAVAILABLE;
	    }
	  do_cleanups (old_chain);
	  return status;
	}
    }
  do_cleanups (old_chain);

  *xfered_len = len;
  return TARGET_XFER_UNAVAILABLE;
}

enum target_xfer_status
section_table_xfer_memory_partial (gdb_byte *readbuf, const gdb_byte *writebuf,
				   ULONGEST offset, ULONGEST len,
				   ULONGEST *xfered_len,
				   struct target_section *sections,
				   struct target_section *sections_end,
				   const char *section_name)
{
  int res;
  struct target_section *p;
  ULONGEST memaddr = offset;
  ULONGEST memend = memaddr + len;

  if (len == 0)
    internal_error (__FILE__, __LINE__,
		    _("failed internal consistency check"));

  for (p = sections; p < sections_end; p++)
    {
      struct bfd_section *asect = p->the_bfd_section;
      bfd *abfd = asect->owner;

      if (section_name && strcmp (section_name, asect->name) != 0)
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

static struct target_section_table *
exec_get_section_table (struct target_ops *ops)
{
  return current_target_sections;
}

static enum target_xfer_status
exec_xfer_partial (struct target_ops *ops, enum target_object object,
		   const char *annex, gdb_byte *readbuf,
		   const gdb_byte *writebuf,
		   ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  struct target_section_table *table = target_get_section_table (ops);

  if (object == TARGET_OBJECT_MEMORY)
    return section_table_xfer_memory_partial (readbuf, writebuf,
					      offset, len, xfered_len,
					      table->sections,
					      table->sections_end,
					      NULL);
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

  printf_filtered ("\t`%s', ", bfd_get_filename (abfd));
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
	  bfd *pbfd = psect->owner;

	  if ((bfd_get_section_flags (pbfd, psect) & (SEC_ALLOC | SEC_LOAD))
	      != (SEC_ALLOC | SEC_LOAD))
	    continue;

	  if (bfd_get_section_vma (pbfd, psect) <= abfd->start_address
	      && abfd->start_address < (bfd_get_section_vma (pbfd, psect)
					+ bfd_get_section_size (psect)))
	    {
	      displacement = p->addr - bfd_get_section_vma (pbfd, psect);
	      break;
	    }
	}
      if (p == t->sections_end)
	warning (_("Cannot find section for the entry point of %s."),
		 bfd_get_filename (abfd));

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
      printf_filtered (" is %s", bfd_section_name (pbfd, psect));
      if (pbfd != abfd)
	printf_filtered (" in %s", bfd_get_filename (pbfd));
      printf_filtered ("\n");
    }
}

static void
exec_files_info (struct target_ops *t)
{
  if (exec_bfd)
    print_section_info (current_target_sections, exec_bfd);
  else
    puts_filtered (_("\t<no file loaded>\n"));
}

static void
set_section_command (char *args, int from_tty)
{
  struct target_section *p;
  char *secname;
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
      if (!strncmp (secname, bfd_section_name (p->bfd,
					       p->the_bfd_section), seclen)
	  && bfd_section_name (p->bfd, p->the_bfd_section)[seclen] == '\0')
	{
	  offset = secaddr - p->addr;
	  p->addr += offset;
	  p->endaddr += offset;
	  if (from_tty)
	    exec_files_info (&exec_ops);
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
      if (filename_cmp (filename, p->the_bfd_section->owner->filename) == 0
	  && index == p->the_bfd_section->index)
	{
	  p->endaddr += address - p->addr;
	  p->addr = address;
	}
    }
}

/* If mourn is being called in all the right places, this could be say
   `gdb internal error' (since generic_mourn calls
   breakpoint_init_inferior).  */

static int
ignore (struct target_ops *ops, struct gdbarch *gdbarch,
	struct bp_target_info *bp_tgt)
{
  return 0;
}

/* Implement the to_remove_breakpoint method.  */

static int
exec_remove_breakpoint (struct target_ops *ops, struct gdbarch *gdbarch,
			struct bp_target_info *bp_tgt,
			enum remove_bp_reason reason)
{
  return 0;
}

static int
exec_has_memory (struct target_ops *ops)
{
  /* We can provide memory if we have any file/target sections to read
     from.  */
  return (current_target_sections->sections
	  != current_target_sections->sections_end);
}

static char *
exec_make_note_section (struct target_ops *self, bfd *obfd, int *note_size)
{
  error (_("Can't create a corefile"));
}

/* Fill in the exec file target vector.  Very few entries need to be
   defined.  */

static void
init_exec_ops (void)
{
  exec_ops.to_shortname = "exec";
  exec_ops.to_longname = "Local exec file";
  exec_ops.to_doc = "Use an executable file as a target.\n\
Specify the filename of the executable file.";
  exec_ops.to_open = exec_open;
  exec_ops.to_close = exec_close_1;
  exec_ops.to_xfer_partial = exec_xfer_partial;
  exec_ops.to_get_section_table = exec_get_section_table;
  exec_ops.to_files_info = exec_files_info;
  exec_ops.to_insert_breakpoint = ignore;
  exec_ops.to_remove_breakpoint = exec_remove_breakpoint;
  exec_ops.to_stratum = file_stratum;
  exec_ops.to_has_memory = exec_has_memory;
  exec_ops.to_make_corefile_notes = exec_make_note_section;
  exec_ops.to_find_memory_regions = objfile_find_memory_regions;
  exec_ops.to_magic = OPS_MAGIC;
}

void
_initialize_exec (void)
{
  struct cmd_list_element *c;

  init_exec_ops ();

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

  add_target_with_completer (&exec_ops, filename_completer);
}
