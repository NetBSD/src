/* Work with executable files, for GDB. 

   Copyright (C) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
   1998, 1999, 2000, 2001, 2002, 2003, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

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

#include <fcntl.h>
#include "readline/readline.h"
#include "gdb_string.h"

#include "gdbcore.h"

#include <ctype.h>
#include "gdb_stat.h"

#include "xcoffsolib.h"

struct vmap *map_vmap (bfd *, bfd *);

void (*deprecated_file_changed_hook) (char *);

/* Prototypes for local functions */

static void file_command (char *, int);

static void set_section_command (char *, int);

static void exec_files_info (struct target_ops *);

static void init_exec_ops (void);

void _initialize_exec (void);

/* The target vector for executable files.  */

struct target_ops exec_ops;

/* True if the exec target is pushed on the stack.  */
static int using_exec_ops;

/* Whether to open exec and core files read-only or read-write.  */

int write_files = 0;
static void
show_write_files (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Writing into executable and core files is %s.\n"),
		    value);
}


struct vmap *vmap;

static void
exec_open (char *args, int from_tty)
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
      char *name = bfd_get_filename (abfd);

      gdb_bfd_close_or_warn (abfd);
      xfree (name);

      /* Removing target sections may close the exec_ops target.
	 Clear exec_bfd before doing so to prevent recursion.  */
      exec_bfd = NULL;
      exec_bfd_mtime = 0;

      remove_target_sections (abfd);
    }
}

/* This is the target_close implementation.  Clears all target
   sections and closes all executable bfds from all program spaces.  */

static void
exec_close_1 (int quitting)
{
  int need_symtab_cleanup = 0;
  struct vmap *vp, *nxt;

  using_exec_ops = 0;

  for (nxt = vmap; nxt != NULL;)
    {
      vp = nxt;
      nxt = vp->nxt;

      /* if there is an objfile associated with this bfd,
         free_objfile() will do proper cleanup of objfile *and* bfd.  */

      if (vp->objfile)
	{
	  free_objfile (vp->objfile);
	  need_symtab_cleanup = 1;
	}
      else if (vp->bfd != exec_bfd)
	/* FIXME-leak: We should be freeing vp->name too, I think.  */
	gdb_bfd_close_or_warn (vp->bfd);

      xfree (vp);
    }

  vmap = NULL;

  {
    struct program_space *ss;
    struct cleanup *old_chain;

    old_chain = save_current_program_space ();
    ALL_PSPACES (ss)
    {
      set_current_program_space (ss);

      /* Delete all target sections.  */
      resize_section_table
	(current_target_sections,
	 -resize_section_table (current_target_sections, 0));

      exec_close ();
    }

    do_cleanups (old_chain);
  }
}

void
exec_file_clear (int from_tty)
{
  /* Remove exec file.  */
  exec_close ();

  if (from_tty)
    printf_unfiltered (_("No executable file now.\n"));
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
exec_file_attach (char *filename, int from_tty)
{
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
      struct cleanup *cleanups;
      char *scratch_pathname;
      int scratch_chan;
      struct target_section *sections = NULL, *sections_end = NULL;
      char **matching;

      scratch_chan = openp (getenv ("PATH"), OPF_TRY_CWD_FIRST, filename,
		   write_files ? O_RDWR | O_BINARY : O_RDONLY | O_BINARY,
			    &scratch_pathname);
#if defined(__GO32__) || defined(_WIN32) || defined(__CYGWIN__)
      if (scratch_chan < 0)
	{
	  char *exename = alloca (strlen (filename) + 5);

	  strcat (strcpy (exename, filename), ".exe");
	  scratch_chan = openp (getenv ("PATH"), OPF_TRY_CWD_FIRST, exename,
	     write_files ? O_RDWR | O_BINARY : O_RDONLY | O_BINARY,
	     &scratch_pathname);
	}
#endif
      if (scratch_chan < 0)
	perror_with_name (filename);
      exec_bfd = bfd_fopen (scratch_pathname, gnutarget,
			    write_files ? FOPEN_RUB : FOPEN_RB,
			    scratch_chan);

      if (!exec_bfd)
	{
	  close (scratch_chan);
	  error (_("\"%s\": could not open as an executable file: %s"),
		 scratch_pathname, bfd_errmsg (bfd_get_error ()));
	}

      /* At this point, scratch_pathname and exec_bfd->name both point to the
         same malloc'd string.  However exec_close() will attempt to free it
         via the exec_bfd->name pointer, so we need to make another copy and
         leave exec_bfd as the new owner of the original copy.  */
      scratch_pathname = xstrdup (scratch_pathname);
      cleanups = make_cleanup (xfree, scratch_pathname);

      if (!bfd_check_format_matches (exec_bfd, bfd_object, &matching))
	{
	  /* Make sure to close exec_bfd, or else "run" might try to use
	     it.  */
	  exec_close ();
	  error (_("\"%s\": not in executable format: %s"),
		 scratch_pathname,
		 gdb_bfd_errmsg (bfd_get_error (), matching));
	}

      /* FIXME - This should only be run for RS6000, but the ifdef is a poor
         way to accomplish.  */
#ifdef DEPRECATED_IBM6000_TARGET
      /* Setup initial vmap.  */

      map_vmap (exec_bfd, 0);
      if (vmap == NULL)
	{
	  /* Make sure to close exec_bfd, or else "run" might try to use
	     it.  */
	  exec_close ();
	  error (_("\"%s\": can't find the file sections: %s"),
		 scratch_pathname, bfd_errmsg (bfd_get_error ()));
	}
#endif /* DEPRECATED_IBM6000_TARGET */

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
      add_target_sections (sections, sections_end);
      xfree (sections);

      /* Tell display code (if any) about the changed file name.  */
      if (deprecated_exec_file_display_hook)
	(*deprecated_exec_file_display_hook) (filename);

      do_cleanups (cleanups);
    }
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

  /* Check the section flags, but do not discard zero-length sections, since
     some symbols may still be attached to this section.  For instance, we
     encountered on sparc-solaris 2.10 a shared library with an empty .bss
     section to which a symbol named "_end" was attached.  The address
     of this symbol still needs to be relocated.  */
  aflag = bfd_get_section_flags (abfd, asect);
  if (!(aflag & SEC_ALLOC))
    return;

  (*table_pp)->bfd = abfd;
  (*table_pp)->the_bfd_section = asect;
  (*table_pp)->addr = bfd_section_vma (abfd, asect);
  (*table_pp)->endaddr = (*table_pp)->addr + bfd_section_size (abfd, asect);
  (*table_pp)++;
}

int
resize_section_table (struct target_section_table *table, int num_added)
{
  struct target_section *old_value;
  int old_count;
  int new_count;

  old_value = table->sections;
  old_count = table->sections_end - table->sections;

  new_count = num_added + old_count;

  if (new_count)
    {
      table->sections = xrealloc (table->sections,
				  sizeof (struct target_section) * new_count);
      table->sections_end = table->sections + new_count;
    }
  else
    {
      xfree (table->sections);
      table->sections = table->sections_end = NULL;
    }

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
  *start = (struct target_section *) xmalloc (count * sizeof (**start));
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
add_target_sections (struct target_section *sections,
		     struct target_section *sections_end)
{
  int count;
  struct target_section_table *table = current_target_sections;

  count = sections_end - sections;

  if (count > 0)
    {
      int space = resize_section_table (table, count);

      memcpy (table->sections + space,
	      sections, count * sizeof (sections[0]));

      /* If these are the first file sections we can provide memory
	 from, push the file_stratum target.  */
      if (!using_exec_ops)
	{
	  using_exec_ops = 1;
	  push_target (&exec_ops);
	}
    }
}

/* Remove all target sections taken from ABFD.  */

void
remove_target_sections (bfd *abfd)
{
  struct target_section *src, *dest;
  struct target_section_table *table = current_target_sections;

  dest = table->sections;
  for (src = table->sections; src < table->sections_end; src++)
    if (src->bfd != abfd)
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


static void
bfdsec_to_vmap (struct bfd *abfd, struct bfd_section *sect, void *arg3)
{
  struct vmap_and_bfd *vmap_bfd = (struct vmap_and_bfd *) arg3;
  struct vmap *vp;

  vp = vmap_bfd->pvmap;

  if ((bfd_get_section_flags (abfd, sect) & SEC_LOAD) == 0)
    return;

  if (strcmp (bfd_section_name (abfd, sect), ".text") == 0)
    {
      vp->tstart = bfd_section_vma (abfd, sect);
      vp->tend = vp->tstart + bfd_section_size (abfd, sect);
      vp->tvma = bfd_section_vma (abfd, sect);
      vp->toffs = sect->filepos;
    }
  else if (strcmp (bfd_section_name (abfd, sect), ".data") == 0)
    {
      vp->dstart = bfd_section_vma (abfd, sect);
      vp->dend = vp->dstart + bfd_section_size (abfd, sect);
      vp->dvma = bfd_section_vma (abfd, sect);
    }
  /* Silently ignore other types of sections.  (FIXME?)  */
}

/* Make a vmap for ABFD which might be a member of the archive ARCH.
   Return the new vmap.  */

struct vmap *
map_vmap (bfd *abfd, bfd *arch)
{
  struct vmap_and_bfd vmap_bfd;
  struct vmap *vp, **vpp;

  vp = (struct vmap *) xmalloc (sizeof (*vp));
  memset ((char *) vp, '\0', sizeof (*vp));
  vp->nxt = 0;
  vp->bfd = abfd;
  vp->name = bfd_get_filename (arch ? arch : abfd);
  vp->member = arch ? bfd_get_filename (abfd) : "";

  vmap_bfd.pbfd = arch;
  vmap_bfd.pvmap = vp;
  bfd_map_over_sections (abfd, bfdsec_to_vmap, &vmap_bfd);

  /* Find the end of the list and append.  */
  for (vpp = &vmap; *vpp; vpp = &(*vpp)->nxt)
    ;
  *vpp = vp;

  return vp;
}


VEC(mem_range_s) *
section_table_available_memory (VEC(mem_range_s) *memory,
				CORE_ADDR memaddr, ULONGEST len,
				struct target_section *sections,
				struct target_section *sections_end)
{
  struct target_section *p;

  for (p = sections; p < sections_end; p++)
    {
      if ((bfd_get_section_flags (p->bfd, p->the_bfd_section)
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

	  r->start = max (lo1, lo2);
	  r->length = min (hi1, hi2) - r->start;
	}
    }

  return memory;
}

int
section_table_xfer_memory_partial (gdb_byte *readbuf, const gdb_byte *writebuf,
				   ULONGEST offset, LONGEST len,
				   struct target_section *sections,
				   struct target_section *sections_end,
				   const char *section_name)
{
  int res;
  struct target_section *p;
  ULONGEST memaddr = offset;
  ULONGEST memend = memaddr + len;

  if (len <= 0)
    internal_error (__FILE__, __LINE__,
		    _("failed internal consistency check"));

  for (p = sections; p < sections_end; p++)
    {
      if (section_name && strcmp (section_name, p->the_bfd_section->name) != 0)
	continue;		/* not the section we need.  */
      if (memaddr >= p->addr)
        {
	  if (memend <= p->endaddr)
	    {
	      /* Entire transfer is within this section.  */
	      if (writebuf)
		res = bfd_set_section_contents (p->bfd, p->the_bfd_section,
						writebuf, memaddr - p->addr,
						len);
	      else
		res = bfd_get_section_contents (p->bfd, p->the_bfd_section,
						readbuf, memaddr - p->addr,
						len);
	      return (res != 0) ? len : 0;
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
		res = bfd_set_section_contents (p->bfd, p->the_bfd_section,
						writebuf, memaddr - p->addr,
						len);
	      else
		res = bfd_get_section_contents (p->bfd, p->the_bfd_section,
						readbuf, memaddr - p->addr,
						len);
	      return (res != 0) ? len : 0;
	    }
        }
    }

  return 0;			/* We can't help.  */
}

struct target_section_table *
exec_get_section_table (struct target_ops *ops)
{
  return current_target_sections;
}

static LONGEST
exec_xfer_partial (struct target_ops *ops, enum target_object object,
		   const char *annex, gdb_byte *readbuf,
		   const gdb_byte *writebuf,
		   ULONGEST offset, LONGEST len)
{
  struct target_section_table *table = target_get_section_table (ops);

  if (object == TARGET_OBJECT_MEMORY)
    return section_table_xfer_memory_partial (readbuf, writebuf,
					      offset, len,
					      table->sections,
					      table->sections_end,
					      NULL);
  else
    return -1;
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
	  asection *asect = p->the_bfd_section;

	  if ((bfd_get_section_flags (abfd, asect) & (SEC_ALLOC | SEC_LOAD))
	      != (SEC_ALLOC | SEC_LOAD))
	    continue;

	  if (bfd_get_section_vma (abfd, asect) <= abfd->start_address
	      && abfd->start_address < (bfd_get_section_vma (abfd, asect)
					+ bfd_get_section_size (asect)))
	    {
	      displacement = p->addr - bfd_get_section_vma (abfd, asect);
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
			 hex_string_custom (p->the_bfd_section->filepos, 8));
      printf_filtered (" is %s", bfd_section_name (p->bfd,
						   p->the_bfd_section));
      if (p->bfd != abfd)
	printf_filtered (" in %s", bfd_get_filename (p->bfd));
      printf_filtered ("\n");
    }
}

static void
exec_files_info (struct target_ops *t)
{
  print_section_info (current_target_sections, exec_bfd);

  if (vmap)
    {
      int addr_size = gdbarch_addr_bit (target_gdbarch) / 8;
      struct vmap *vp;

      printf_unfiltered (_("\tMapping info for file `%s'.\n"), vmap->name);
      printf_unfiltered ("\t  %*s   %*s   %*s   %*s %8.8s %s\n",
			 addr_size * 2, "tstart",
			 addr_size * 2, "tend",
			 addr_size * 2, "dstart",
			 addr_size * 2, "dend",
			 "section",
			 "file(member)");

      for (vp = vmap; vp; vp = vp->nxt)
	printf_unfiltered ("\t0x%s 0x%s 0x%s 0x%s %s%s%s%s\n",
			   phex (vp->tstart, addr_size),
			   phex (vp->tend, addr_size),
			   phex (vp->dstart, addr_size),
			   phex (vp->dend, addr_size),
			   vp->name,
			   *vp->member ? "(" : "", vp->member,
			   *vp->member ? ")" : "");
    }
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
      if (!strncmp (secname, bfd_section_name (exec_bfd,
					       p->the_bfd_section), seclen)
	  && bfd_section_name (exec_bfd, p->the_bfd_section)[seclen] == '\0')
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
      if (filename_cmp (filename, p->bfd->filename) == 0
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
ignore (struct gdbarch *gdbarch, struct bp_target_info *bp_tgt)
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

/* Find mapped memory.  */

extern void
exec_set_find_memory_regions (int (*func) (find_memory_region_ftype, void *))
{
  exec_ops.to_find_memory_regions = func;
}

static char *exec_make_note_section (bfd *, int *);

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
  exec_ops.to_attach = find_default_attach;
  exec_ops.to_xfer_partial = exec_xfer_partial;
  exec_ops.to_get_section_table = exec_get_section_table;
  exec_ops.to_files_info = exec_files_info;
  exec_ops.to_insert_breakpoint = ignore;
  exec_ops.to_remove_breakpoint = ignore;
  exec_ops.to_create_inferior = find_default_create_inferior;
  exec_ops.to_stratum = file_stratum;
  exec_ops.to_has_memory = exec_has_memory;
  exec_ops.to_make_corefile_notes = exec_make_note_section;
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

  add_target (&exec_ops);
}

static char *
exec_make_note_section (bfd *obfd, int *note_size)
{
  error (_("Can't create a corefile"));
}
