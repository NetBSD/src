/* Core dump and executable file functions below target vector, for GDB.
   Copyright 1986, 87, 89, 91, 92, 93, 94, 95, 96, 97, 1998
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "gdb_string.h"
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include "frame.h"		/* required by inferior.h */
#include "inferior.h"
#include "symtab.h"
#include "command.h"
#include "bfd.h"
#include "target.h"
#include "gdbcore.h"
#include "gdbthread.h"

/* List of all available core_fns.  On gdb startup, each core file register
   reader calls add_core_fns() to register information on each core format it
   is prepared to read. */

static struct core_fns *core_file_fns = NULL;

/* The core_fns for a core file handler that is prepared to read the core
   file currently open on core_bfd. */

static struct core_fns *core_vec = NULL;

static void core_files_info PARAMS ((struct target_ops *));

#ifdef SOLIB_ADD
static int solib_add_stub PARAMS ((PTR));
#endif

static struct core_fns *sniff_core_bfd PARAMS ((bfd *));

static boolean gdb_check_format PARAMS ((bfd *));

static void core_open PARAMS ((char *, int));

static void core_detach PARAMS ((char *, int));

static void core_close PARAMS ((int));

static void get_core_registers PARAMS ((int));

static void add_to_thread_list PARAMS ((bfd *, asection *, PTR));

static int ignore PARAMS ((CORE_ADDR, char *));

static char *core_file_to_sym_file PARAMS ((char *));

static int core_file_thread_alive PARAMS ((int tid));

static void init_core_ops PARAMS ((void));

void _initialize_corelow PARAMS ((void));

struct target_ops core_ops;

/* Link a new core_fns into the global core_file_fns list.  Called on gdb
   startup by the _initialize routine in each core file register reader, to
   register information about each format the the reader is prepared to
   handle. */

void
add_core_fns (cf)
     struct core_fns *cf;
{
  cf->next = core_file_fns;
  core_file_fns = cf;
}

/* The default function that core file handlers can use to examine a
   core file BFD and decide whether or not to accept the job of
   reading the core file. */

int
default_core_sniffer (our_fns, abfd)
     struct core_fns *our_fns;
     bfd *abfd;
{
  int result;

  result = (bfd_get_flavour (abfd) == our_fns -> core_flavour);
  return (result);
}

/* Walk through the list of core functions to find a set that can
   handle the core file open on ABFD.  Default to the first one in the
   list of nothing matches.  Returns pointer to set that is
   selected. */

static struct core_fns *
sniff_core_bfd (abfd)
     bfd *abfd;
{
  struct core_fns *cf;
  struct core_fns *yummy = NULL;
  int matches = 0;;

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
      warning ("\"%s\": ambiguous core format, %d handlers match",
	       bfd_get_filename (abfd), matches);
    }
  else if (matches == 0)
    {
      warning ("\"%s\": no core file handler recognizes format, using default",
	       bfd_get_filename (abfd));
    }
  if (yummy == NULL)
    {
      yummy = core_file_fns;
    }
  return (yummy);
}

/* The default is to reject every core file format we see.  Either
   BFD has to recognize it, or we have to provide a function in the
   core file handler that recognizes it. */

int
default_check_format (abfd)
     bfd *abfd;
{
  return (0);
}

/* Attempt to recognize core file formats that BFD rejects. */

static boolean 
gdb_check_format (abfd)
     bfd *abfd;
{
  struct core_fns *cf;

  for (cf = core_file_fns; cf != NULL; cf = cf->next)
    {
      if (cf->check_format (abfd))
	{
	  return (true);
	}
    }
  return (false);
}

/* Discard all vestiges of any previous core file and mark data and stack
   spaces as empty.  */

/* ARGSUSED */
static void
core_close (quitting)
     int quitting;
{
  char *name;

  if (core_bfd)
    {
      inferior_pid = 0;		/* Avoid confusion from thread stuff */

      /* Clear out solib state while the bfd is still open. See
         comments in clear_solib in solib.c. */
#ifdef CLEAR_SOLIB
      CLEAR_SOLIB ();
#endif

      name = bfd_get_filename (core_bfd);
      if (!bfd_close (core_bfd))
	warning ("cannot close \"%s\": %s",
		 name, bfd_errmsg (bfd_get_error ()));
      free (name);
      core_bfd = NULL;
      if (core_ops.to_sections)
	{
	  free ((PTR) core_ops.to_sections);
	  core_ops.to_sections = NULL;
	  core_ops.to_sections_end = NULL;
	}
    }
  core_vec = NULL;
}

#ifdef SOLIB_ADD
/* Stub function for catch_errors around shared library hacking.  FROM_TTYP
   is really an int * which points to from_tty.  */

static int
solib_add_stub (from_ttyp)
     PTR from_ttyp;
{
  SOLIB_ADD (NULL, *(int *) from_ttyp, &current_target);
  re_enable_breakpoints_in_shlibs ();
  return 0;
}
#endif /* SOLIB_ADD */

/* Look for sections whose names start with `.reg/' so that we can extract the
   list of threads in a core file.  */

static void
add_to_thread_list (abfd, asect, reg_sect_arg)
     bfd *abfd;
     asection *asect;
     PTR reg_sect_arg;
{
  int thread_id;
  asection *reg_sect = (asection *) reg_sect_arg;

  if (strncmp (bfd_section_name (abfd, asect), ".reg/", 5) != 0)
    return;

  thread_id = atoi (bfd_section_name (abfd, asect) + 5);

  add_thread (thread_id);

/* Warning, Will Robinson, looking at BFD private data! */

  if (reg_sect != NULL
      && asect->filepos == reg_sect->filepos)	/* Did we find .reg? */
    inferior_pid = thread_id;	/* Yes, make it current */
}

/* This routine opens and sets up the core file bfd.  */

static void
core_open (filename, from_tty)
     char *filename;
     int from_tty;
{
  const char *p;
  int siggy;
  struct cleanup *old_chain;
  char *temp;
  bfd *temp_bfd;
  int ontop;
  int scratch_chan;

  target_preopen (from_tty);
  if (!filename)
    {
      error (core_bfd ?
	     "No core file specified.  (Use `detach' to stop debugging a core file.)"
	     : "No core file specified.");
    }

  filename = tilde_expand (filename);
  if (filename[0] != '/')
    {
      temp = concat (current_directory, "/", filename, NULL);
      free (filename);
      filename = temp;
    }

  old_chain = make_cleanup (free, filename);

  scratch_chan = open (filename, write_files ? O_RDWR : O_RDONLY, 0);
  if (scratch_chan < 0)
    perror_with_name (filename);

  temp_bfd = bfd_fdopenr (filename, gnutarget, scratch_chan);
  if (temp_bfd == NULL)
    perror_with_name (filename);

  if (!bfd_check_format (temp_bfd, bfd_core) &&
      !gdb_check_format (temp_bfd))
    {
      /* Do it after the err msg */
      /* FIXME: should be checking for errors from bfd_close (for one thing,
         on error it does not free all the storage associated with the
         bfd).  */
      make_cleanup ((make_cleanup_func) bfd_close, temp_bfd);
      error ("\"%s\" is not a core dump: %s",
	     filename, bfd_errmsg (bfd_get_error ()));
    }

  /* Looks semi-reasonable.  Toss the old core file and work on the new.  */

  discard_cleanups (old_chain);	/* Don't free filename any more */
  unpush_target (&core_ops);
  core_bfd = temp_bfd;
  old_chain = make_cleanup ((make_cleanup_func) core_close, core_bfd);

  /* Find a suitable core file handler to munch on core_bfd */
  core_vec = sniff_core_bfd (core_bfd);

  validate_files ();

  /* Find the data section */
  if (build_section_table (core_bfd, &core_ops.to_sections,
			   &core_ops.to_sections_end))
    error ("\"%s\": Can't find sections: %s",
	   bfd_get_filename (core_bfd), bfd_errmsg (bfd_get_error ()));

  ontop = !push_target (&core_ops);
  discard_cleanups (old_chain);

  p = bfd_core_file_failing_command (core_bfd);
  if (p)
    printf_filtered ("Core was generated by `%s'.\n", p);

  siggy = bfd_core_file_failing_signal (core_bfd);
  if (siggy > 0)
    printf_filtered ("Program terminated with signal %d, %s.\n", siggy,
		     safe_strsignal (siggy));

  /* Build up thread list from BFD sections. */

  init_thread_list ();
  bfd_map_over_sections (core_bfd, add_to_thread_list,
			 bfd_get_section_by_name (core_bfd, ".reg"));

  if (ontop)
    {
      /* Fetch all registers from core file.  */
      target_fetch_registers (-1);

      /* Add symbols and section mappings for any shared libraries.  */
#ifdef SOLIB_ADD
      catch_errors (solib_add_stub, &from_tty, (char *) 0,
		    RETURN_MASK_ALL);
#endif

      /* Now, set up the frame cache, and print the top of stack.  */
      flush_cached_frames ();
      select_frame (get_current_frame (), 0);
      print_stack_frame (selected_frame, selected_frame_level, 1);
    }
  else
    {
      warning (
		"you won't be able to access this core file until you terminate\n\
your %s; do ``info files''", target_longname);
    }
}

static void
core_detach (args, from_tty)
     char *args;
     int from_tty;
{
  if (args)
    error ("Too many arguments");
  unpush_target (&core_ops);
  reinit_frame_cache ();
  if (from_tty)
    printf_filtered ("No core file now.\n");
}


/* Try to retrieve registers from a section in core_bfd, and supply
   them to core_vec->core_read_registers, as the register set numbered
   WHICH.

   If inferior_pid is zero, do the single-threaded thing: look for a
   section named NAME.  If inferior_pid is non-zero, do the
   multi-threaded thing: look for a section named "NAME/PID", where
   PID is the shortest ASCII decimal representation of inferior_pid.

   HUMAN_NAME is a human-readable name for the kind of registers the
   NAME section contains, for use in error messages.

   If REQUIRED is non-zero, print an error if the core file doesn't
   have a section by the appropriate name.  Otherwise, just do nothing.  */

static void
get_core_register_section (char *name,
			   int which,
			   char *human_name,
			   int required)
{
  char section_name[100];
  sec_ptr section;
  bfd_size_type size;
  char *contents;

  if (inferior_pid)
    sprintf (section_name, "%s/%d", name, inferior_pid);
  else
    strcpy (section_name, name);

  section = bfd_get_section_by_name (core_bfd, section_name);
  if (! section)
    {
      if (required)
	warning ("Couldn't find %s registers in core file.\n", human_name);
      return;
    }

  size = bfd_section_size (core_bfd, section);
  contents = alloca (size);
  if (! bfd_get_section_contents (core_bfd, section, contents,
				  (file_ptr) 0, size))
    {
      warning ("Couldn't read %s registers from `%s' section in core file.\n",
	       human_name, name);
      return;
    }

  core_vec->core_read_registers (contents, size, which, 
				 ((CORE_ADDR)
				  bfd_section_vma (core_bfd, section)));
}


/* Get the registers out of a core file.  This is the machine-
   independent part.  Fetch_core_registers is the machine-dependent
   part, typically implemented in the xm-file for each architecture.  */

/* We just get all the registers, so we don't use regno.  */

/* ARGSUSED */
static void
get_core_registers (regno)
     int regno;
{
  int status;

  if (core_vec == NULL
      || core_vec->core_read_registers == NULL)
    {
      fprintf_filtered (gdb_stderr,
		     "Can't fetch registers from this type of core file\n");
      return;
    }

  get_core_register_section (".reg", 0, "general-purpose", 1);
  get_core_register_section (".reg2", 2, "floating-point", 0);
  get_core_register_section (".reg-xfp", 3, "extended floating-point", 0);

  registers_fetched ();
}

static char *
core_file_to_sym_file (core)
     char *core;
{
  CONST char *failing_command;
  char *p;
  char *temp;
  bfd *temp_bfd;
  int scratch_chan;

  if (!core)
    error ("No core file specified.");

  core = tilde_expand (core);
  if (core[0] != '/')
    {
      temp = concat (current_directory, "/", core, NULL);
      core = temp;
    }

  scratch_chan = open (core, write_files ? O_RDWR : O_RDONLY, 0);
  if (scratch_chan < 0)
    perror_with_name (core);

  temp_bfd = bfd_fdopenr (core, gnutarget, scratch_chan);
  if (temp_bfd == NULL)
    perror_with_name (core);

  if (!bfd_check_format (temp_bfd, bfd_core))
    {
      /* Do it after the err msg */
      /* FIXME: should be checking for errors from bfd_close (for one thing,
         on error it does not free all the storage associated with the
         bfd).  */
      make_cleanup ((make_cleanup_func) bfd_close, temp_bfd);
      error ("\"%s\" is not a core dump: %s",
	     core, bfd_errmsg (bfd_get_error ()));
    }

  /* Find the data section */
  if (build_section_table (temp_bfd, &core_ops.to_sections,
			   &core_ops.to_sections_end))
    error ("\"%s\": Can't find sections: %s",
	   bfd_get_filename (temp_bfd), bfd_errmsg (bfd_get_error ()));

  failing_command = bfd_core_file_failing_command (temp_bfd);

  bfd_close (temp_bfd);

  /* If we found a filename, remember that it is probably saved
     relative to the executable that created it.  If working directory
     isn't there now, we may not be able to find the executable.  Rather
     than trying to be sauve about finding it, just check if the file
     exists where we are now.  If not, then punt and tell our client
     we couldn't find the sym file.
   */
  p = (char *) failing_command;
  if ((p != NULL) && (access (p, F_OK) != 0))
    p = NULL;

  return p;
}

static void
core_files_info (t)
     struct target_ops *t;
{
  print_section_info (t, core_bfd);
}

/* If mourn is being called in all the right places, this could be say
   `gdb internal error' (since generic_mourn calls breakpoint_init_inferior).  */

static int
ignore (addr, contents)
     CORE_ADDR addr;
     char *contents;
{
  return 0;
}


/* Okay, let's be honest: threads gleaned from a core file aren't
   exactly lively, are they?  On the other hand, if we don't claim
   that each & every one is alive, then we don't get any of them
   to appear in an "info thread" command, which is quite a useful
   behaviour.
 */
static int
core_file_thread_alive (tid)
     int tid;
{
  return 1;
}

/* Fill in core_ops with its defined operations and properties.  */

static void
init_core_ops ()
{
  core_ops.to_shortname = "core";
  core_ops.to_longname = "Local core dump file";
  core_ops.to_doc =
    "Use a core file as a target.  Specify the filename of the core file.";
  core_ops.to_open = core_open;
  core_ops.to_close = core_close;
  core_ops.to_attach = find_default_attach;
  core_ops.to_require_attach = find_default_require_attach;
  core_ops.to_detach = core_detach;
  core_ops.to_require_detach = find_default_require_detach;
  core_ops.to_fetch_registers = get_core_registers;
  core_ops.to_xfer_memory = xfer_memory;
  core_ops.to_files_info = core_files_info;
  core_ops.to_insert_breakpoint = ignore;
  core_ops.to_remove_breakpoint = ignore;
  core_ops.to_create_inferior = find_default_create_inferior;
  core_ops.to_clone_and_follow_inferior = find_default_clone_and_follow_inferior;
  core_ops.to_thread_alive = core_file_thread_alive;
  core_ops.to_core_file_to_sym_file = core_file_to_sym_file;
  core_ops.to_stratum = core_stratum;
  core_ops.to_has_memory = 1;
  core_ops.to_has_stack = 1;
  core_ops.to_has_registers = 1;
  core_ops.to_magic = OPS_MAGIC;
}

/* non-zero if we should not do the add_target call in
   _initialize_corelow; not initialized (i.e., bss) so that
   the target can initialize it (i.e., data) if appropriate.
   This needs to be set at compile time because we don't know
   for sure whether the target's initialize routine is called
   before us or after us. */
int coreops_suppress_target;

void
_initialize_corelow ()
{
  init_core_ops ();

  if (!coreops_suppress_target)
    add_target (&core_ops);
}
