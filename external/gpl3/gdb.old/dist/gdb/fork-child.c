/* Fork a Unix child process, and set up to debug it, for GDB.

   Copyright (C) 1990-2019 Free Software Foundation, Inc.

   Contributed by Cygnus Support.

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
#include "inferior.h"
#include "gdbcmd.h"
#include "terminal.h"
#include "gdbthread.h"
#include "top.h"
#include "common/job-control.h"
#include "common/filestuff.h"
#include "nat/fork-inferior.h"
#include "common/common-inferior.h"

/* The exec-wrapper, if any, that will be used when starting the
   inferior.  */

static char *exec_wrapper = NULL;

/* See common/common-inferior.h.  */

const char *
get_exec_wrapper ()
{
  return exec_wrapper;
}

/* See nat/fork-inferior.h.  */

void
gdb_flush_out_err ()
{
  gdb_flush (main_ui->m_gdb_stdout);
  gdb_flush (main_ui->m_gdb_stderr);
}

/* The ui structure that will be saved on 'prefork_hook' and
   restored on 'postfork_hook'.  */
static struct ui *saved_ui = NULL;

/* See nat/fork-inferior.h.  */

void
prefork_hook (const char *args)
{
  const char *inferior_io_terminal = get_inferior_io_terminal ();

  gdb_assert (saved_ui == NULL);
  /* Retain a copy of our UI, since the child will replace this value
     and if we're vforked, we have to restore it.  */
  saved_ui = current_ui;

  /* Tell the terminal handling subsystem what tty we plan to run on;
     it will just record the information for later.  */
  new_tty_prefork (inferior_io_terminal);
}

/* See nat/fork-inferior.h.  */

void
postfork_hook (pid_t pid)
{
  inferior *inf = current_inferior ();

  inferior_appeared (inf, pid);

  /* Needed for wait_for_inferior stuff.  */
  inferior_ptid = ptid_t (pid);

  gdb_assert (saved_ui != NULL);
  current_ui = saved_ui;
  saved_ui = NULL;

  new_tty_postfork ();
}

/* See nat/fork-inferior.h.  */

void
postfork_child_hook ()
{
  /* This is set to the result of setpgrp, which if vforked, will be
     visible to you in the parent process.  It's only used by humans
     for debugging.  */
  static int debug_setpgrp = 657473;

  /* Make sure we switch to main_ui here in order to be able to
     use the fprintf_unfiltered/warning/error functions.  */
  current_ui = main_ui;

  /* Create a new session for the inferior process, if necessary.
     It will also place the inferior in a separate process group.  */
  if (create_tty_session () <= 0)
    {
      /* No session was created, but we still want to run the inferior
	 in a separate process group.  */
      debug_setpgrp = gdb_setpgid ();
      if (debug_setpgrp == -1)
	perror (_("setpgrp failed in child"));
    }

  /* Ask the tty subsystem to switch to the one we specified
     earlier (or to share the current terminal, if none was
     specified).  */
  new_tty ();
}

/* See inferior.h.  */

ptid_t
gdb_startup_inferior (pid_t pid, int num_traps)
{
  ptid_t ptid = startup_inferior (pid, num_traps, NULL, NULL);

  /* Mark all threads non-executing.  */
  set_executing (ptid, 0);

  return ptid;
}

/* Implement the "unset exec-wrapper" command.  */

static void
unset_exec_wrapper_command (const char *args, int from_tty)
{
  xfree (exec_wrapper);
  exec_wrapper = NULL;
}

static void
show_startup_with_shell (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Use of shell to start subprocesses is %s.\n"),
		    value);
}

void
_initialize_fork_child (void)
{
  add_setshow_filename_cmd ("exec-wrapper", class_run, &exec_wrapper, _("\
Set a wrapper for running programs.\n\
The wrapper prepares the system and environment for the new program."),
			    _("\
Show the wrapper for running programs."), NULL,
			    NULL, NULL,
			    &setlist, &showlist);

  add_cmd ("exec-wrapper", class_run, unset_exec_wrapper_command,
           _("Disable use of an execution wrapper."),
           &unsetlist);

  add_setshow_boolean_cmd ("startup-with-shell", class_support,
			   &startup_with_shell, _("\
Set use of shell to start subprocesses.  The default is on."), _("\
Show use of shell to start subprocesses."), NULL,
			   NULL,
			   show_startup_with_shell,
			   &setlist, &showlist);
}
