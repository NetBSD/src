/* Fork a Unix child process, and set up to debug it, for GDB.

   Copyright (C) 1990-2013 Free Software Foundation, Inc.

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
#include "gdb_string.h"
#include "inferior.h"
#include "terminal.h"
#include "target.h"
#include "gdb_wait.h"
#include "gdb_vfork.h"
#include "gdbcore.h"
#include "terminal.h"
#include "gdbthread.h"
#include "command.h" /* for dont_repeat () */
#include "gdbcmd.h"
#include "solib.h"

#include <signal.h>

/* This just gets used as a default if we can't find SHELL.  */
#define SHELL_FILE "/bin/sh"

extern char **environ;

static char *exec_wrapper;

/* Break up SCRATCH into an argument vector suitable for passing to
   execvp and store it in ARGV.  E.g., on "run a b c d" this routine
   would get as input the string "a b c d", and as output it would
   fill in ARGV with the four arguments "a", "b", "c", "d".  */

static void
breakup_args (char *scratch, char **argv)
{
  char *cp = scratch, *tmp;

  for (;;)
    {
      /* Scan past leading separators */
      while (*cp == ' ' || *cp == '\t' || *cp == '\n')
	cp++;

      /* Break if at end of string.  */
      if (*cp == '\0')
	break;

      /* Take an arg.  */
      *argv++ = cp;

      /* Scan for next arg separator.  */
      tmp = strchr (cp, ' ');
      if (tmp == NULL)
	tmp = strchr (cp, '\t');
      if (tmp == NULL)
	tmp = strchr (cp, '\n');

      /* No separators => end of string => break.  */
      if (tmp == NULL)
	break;
      cp = tmp;

      /* Replace the separator with a terminator.  */
      *cp++ = '\0';
    }

  /* Null-terminate the vector.  */
  *argv = NULL;
}

/* When executing a command under the given shell, return non-zero if
   the '!' character should be escaped when embedded in a quoted
   command-line argument.  */

static int
escape_bang_in_quoted_argument (const char *shell_file)
{
  const int shell_file_len = strlen (shell_file);

  /* Bang should be escaped only in C Shells.  For now, simply check
     that the shell name ends with 'csh', which covers at least csh
     and tcsh.  This should be good enough for now.  */

  if (shell_file_len < 3)
    return 0;

  if (shell_file[shell_file_len - 3] == 'c'
      && shell_file[shell_file_len - 2] == 's'
      && shell_file[shell_file_len - 1] == 'h')
    return 1;

  return 0;
}

/* Start an inferior Unix child process and sets inferior_ptid to its
   pid.  EXEC_FILE is the file to run.  ALLARGS is a string containing
   the arguments to the program.  ENV is the environment vector to
   pass.  SHELL_FILE is the shell file, or NULL if we should pick
   one.  EXEC_FUN is the exec(2) function to use, or NULL for the default
   one.  */

/* This function is NOT reentrant.  Some of the variables have been
   made static to ensure that they survive the vfork call.  */

int
fork_inferior (char *exec_file_arg, char *allargs, char **env,
	       void (*traceme_fun) (void), void (*init_trace_fun) (int),
	       void (*pre_trace_fun) (void), char *shell_file_arg,
               void (*exec_fun)(const char *file, char * const *argv,
                                char * const *env))
{
  int pid;
  static char default_shell_file[] = SHELL_FILE;
  /* Set debug_fork then attach to the child while it sleeps, to debug.  */
  static int debug_fork = 0;
  /* This is set to the result of setpgrp, which if vforked, will be visible
     to you in the parent process.  It's only used by humans for debugging.  */
  static int debug_setpgrp = 657473;
  static char *shell_file;
  static char *exec_file;
  char **save_our_env;
  int shell = 0;
  static char **argv;
  const char *inferior_io_terminal = get_inferior_io_terminal ();
  struct inferior *inf;
  int i;
  int save_errno;

  /* If no exec file handed to us, get it from the exec-file command
     -- with a good, common error message if none is specified.  */
  exec_file = exec_file_arg;
  if (exec_file == 0)
    exec_file = get_exec_file (1);

  /* STARTUP_WITH_SHELL is defined in inferior.h.  If 0,e we'll just
    do a fork/exec, no shell, so don't bother figuring out what
    shell.  */
  shell_file = shell_file_arg;
  if (STARTUP_WITH_SHELL)
    {
      /* Figure out what shell to start up the user program under.  */
      if (shell_file == NULL)
	shell_file = getenv ("SHELL");
      if (shell_file == NULL)
	shell_file = default_shell_file;
      shell = 1;
    }

  if (!shell)
    {
      /* We're going to call execvp.  Create argument vector.
	 Calculate an upper bound on the length of the vector by
	 assuming that every other character is a separate
	 argument.  */
      int argc = (strlen (allargs) + 1) / 2 + 2;

      argv = (char **) alloca (argc * sizeof (*argv));
      argv[0] = exec_file;
      breakup_args (allargs, &argv[1]);
    }
  else
    {
      /* We're going to call a shell.  */
      char *shell_command;
      int len;
      char *p;
      int need_to_quote;
      const int escape_bang = escape_bang_in_quoted_argument (shell_file);

      /* Multiplying the length of exec_file by 4 is to account for the
         fact that it may expand when quoted; it is a worst-case number
         based on every character being '.  */
      len = 5 + 4 * strlen (exec_file) + 1 + strlen (allargs) + 1 + /*slop */ 12;
      if (exec_wrapper)
        len += strlen (exec_wrapper) + 1;

      shell_command = (char *) alloca (len);
      shell_command[0] = '\0';

      strcat (shell_command, "exec ");

      /* Add any exec wrapper.  That may be a program name with arguments, so
	 the user must handle quoting.  */
      if (exec_wrapper)
	{
	  strcat (shell_command, exec_wrapper);
	  strcat (shell_command, " ");
	}

      /* Now add exec_file, quoting as necessary.  */

      /* Quoting in this style is said to work with all shells.  But
         csh on IRIX 4.0.1 can't deal with it.  So we only quote it if
         we need to.  */
      p = exec_file;
      while (1)
	{
	  switch (*p)
	    {
	    case '\'':
	    case '!':
	    case '"':
	    case '(':
	    case ')':
	    case '$':
	    case '&':
	    case ';':
	    case '<':
	    case '>':
	    case ' ':
	    case '\n':
	    case '\t':
	      need_to_quote = 1;
	      goto end_scan;

	    case '\0':
	      need_to_quote = 0;
	      goto end_scan;

	    default:
	      break;
	    }
	  ++p;
	}
    end_scan:
      if (need_to_quote)
	{
	  strcat (shell_command, "'");
	  for (p = exec_file; *p != '\0'; ++p)
	    {
	      if (*p == '\'')
		strcat (shell_command, "'\\''");
	      else if (*p == '!' && escape_bang)
		strcat (shell_command, "\\!");
	      else
		strncat (shell_command, p, 1);
	    }
	  strcat (shell_command, "'");
	}
      else
	strcat (shell_command, exec_file);

      strcat (shell_command, " ");
      strcat (shell_command, allargs);

      /* If we decided above to start up with a shell, we exec the
	 shell, "-c" says to interpret the next arg as a shell command
	 to execute, and this command is "exec <target-program>
	 <args>".  */
      argv = (char **) alloca (4 * sizeof (char *));
      argv[0] = shell_file;
      argv[1] = "-c";
      argv[2] = shell_command;
      argv[3] = (char *) 0;
    }

  /* Retain a copy of our environment variables, since the child will
     replace the value of environ and if we're vforked, we have to
     restore it.  */
  save_our_env = environ;

  /* Tell the terminal handling subsystem what tty we plan to run on;
     it will just record the information for later.  */
  new_tty_prefork (inferior_io_terminal);

  /* It is generally good practice to flush any possible pending stdio
     output prior to doing a fork, to avoid the possibility of both
     the parent and child flushing the same data after the fork.  */
  gdb_flush (gdb_stdout);
  gdb_flush (gdb_stderr);

  /* If there's any initialization of the target layers that must
     happen to prepare to handle the child we're about fork, do it
     now...  */
  if (pre_trace_fun != NULL)
    (*pre_trace_fun) ();

  /* Create the child process.  Since the child process is going to
     exec(3) shortly afterwards, try to reduce the overhead by
     calling vfork(2).  However, if PRE_TRACE_FUN is non-null, it's
     likely that this optimization won't work since there's too much
     work to do between the vfork(2) and the exec(3).  This is known
     to be the case on ttrace(2)-based HP-UX, where some handshaking
     between parent and child needs to happen between fork(2) and
     exec(2).  However, since the parent is suspended in the vforked
     state, this doesn't work.  Also note that the vfork(2) call might
     actually be a call to fork(2) due to the fact that autoconf will
     ``#define vfork fork'' on certain platforms.  */
  if (pre_trace_fun || debug_fork)
    pid = fork ();
  else
    pid = vfork ();

  if (pid < 0)
    perror_with_name (("vfork"));

  if (pid == 0)
    {
      if (debug_fork)
	sleep (debug_fork);

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

      /* Changing the signal handlers for the inferior after
         a vfork can also change them for the superior, so we don't mess
         with signals here.  See comments in
         initialize_signals for how we get the right signal handlers
         for the inferior.  */

      /* "Trace me, Dr. Memory!"  */
      (*traceme_fun) ();

      /* The call above set this process (the "child") as debuggable
        by the original gdb process (the "parent").  Since processes
        (unlike people) can have only one parent, if you are debugging
        gdb itself (and your debugger is thus _already_ the
        controller/parent for this child), code from here on out is
        undebuggable.  Indeed, you probably got an error message
        saying "not parent".  Sorry; you'll have to use print
        statements!  */

      /* There is no execlpe call, so we have to set the environment
         for our child in the global variable.  If we've vforked, this
         clobbers the parent, but environ is restored a few lines down
         in the parent.  By the way, yes we do need to look down the
         path to find $SHELL.  Rich Pixley says so, and I agree.  */
      environ = env;

      if (exec_fun != NULL)
        (*exec_fun) (argv[0], argv, env);
      else
        execvp (argv[0], argv);

      /* If we get here, it's an error.  */
      save_errno = errno;
      fprintf_unfiltered (gdb_stderr, "Cannot exec %s", exec_file);
      for (i = 1; argv[i] != NULL; i++)
	fprintf_unfiltered (gdb_stderr, " %s", argv[i]);
      fprintf_unfiltered (gdb_stderr, ".\n");
      fprintf_unfiltered (gdb_stderr, "Error: %s\n",
			  safe_strerror (save_errno));
      gdb_flush (gdb_stderr);
      _exit (0177);
    }

  /* Restore our environment in case a vforked child clob'd it.  */
  environ = save_our_env;

  if (!have_inferiors ())
    init_thread_list ();

  inf = current_inferior ();

  inferior_appeared (inf, pid);

  /* Needed for wait_for_inferior stuff below.  */
  inferior_ptid = pid_to_ptid (pid);

  new_tty_postfork ();

  /* We have something that executes now.  We'll be running through
     the shell at this point, but the pid shouldn't change.  Targets
     supporting MT should fill this task's ptid with more data as soon
     as they can.  */
  add_thread_silent (inferior_ptid);

  /* Now that we have a child process, make it our target, and
     initialize anything target-vector-specific that needs
     initializing.  */
  if (init_trace_fun)
    (*init_trace_fun) (pid);

  /* We are now in the child process of interest, having exec'd the
     correct program, and are poised at the first instruction of the
     new program.  */
  return pid;
}

/* Accept NTRAPS traps from the inferior.  */

void
startup_inferior (int ntraps)
{
  int pending_execs = ntraps;
  int terminal_initted = 0;
  ptid_t resume_ptid;

  if (target_supports_multi_process ())
    resume_ptid = pid_to_ptid (ptid_get_pid (inferior_ptid));
  else
    resume_ptid = minus_one_ptid;

  /* The process was started by the fork that created it, but it will
     have stopped one instruction after execing the shell.  Here we
     must get it up to actual execution of the real program.  */

  if (exec_wrapper)
    pending_execs++;

  while (1)
    {
      enum gdb_signal resume_signal = GDB_SIGNAL_0;
      ptid_t event_ptid;

      struct target_waitstatus ws;
      memset (&ws, 0, sizeof (ws));
      event_ptid = target_wait (resume_ptid, &ws, 0);

      if (ws.kind == TARGET_WAITKIND_IGNORE)
	/* The inferior didn't really stop, keep waiting.  */
	continue;

      switch (ws.kind)
	{
	  case TARGET_WAITKIND_SPURIOUS:
	  case TARGET_WAITKIND_LOADED:
	  case TARGET_WAITKIND_FORKED:
	  case TARGET_WAITKIND_VFORKED:
	  case TARGET_WAITKIND_SYSCALL_ENTRY:
	  case TARGET_WAITKIND_SYSCALL_RETURN:
	    /* Ignore gracefully during startup of the inferior.  */
	    switch_to_thread (event_ptid);
	    break;

	  case TARGET_WAITKIND_SIGNALLED:
	    target_terminal_ours ();
	    target_mourn_inferior ();
	    error (_("During startup program terminated with signal %s, %s."),
		   gdb_signal_to_name (ws.value.sig),
		   gdb_signal_to_string (ws.value.sig));
	    return;

	  case TARGET_WAITKIND_EXITED:
	    target_terminal_ours ();
	    target_mourn_inferior ();
	    if (ws.value.integer)
	      error (_("During startup program exited with code %d."),
		     ws.value.integer);
	    else
	      error (_("During startup program exited normally."));
	    return;

	  case TARGET_WAITKIND_EXECD:
	    /* Handle EXEC signals as if they were SIGTRAP signals.  */
	    xfree (ws.value.execd_pathname);
	    resume_signal = GDB_SIGNAL_TRAP;
	    switch_to_thread (event_ptid);
	    break;

	  case TARGET_WAITKIND_STOPPED:
	    resume_signal = ws.value.sig;
	    switch_to_thread (event_ptid);
	    break;
	}

      if (resume_signal != GDB_SIGNAL_TRAP)
	{
	  /* Let shell child handle its own signals in its own way.  */
	  target_resume (resume_ptid, 0, resume_signal);
	}
      else
	{
	  /* We handle SIGTRAP, however; it means child did an exec.  */
	  if (!terminal_initted)
	    {
	      /* Now that the child has exec'd we know it has already
	         set its process group.  On POSIX systems, tcsetpgrp
	         will fail with EPERM if we try it before the child's
	         setpgid.  */

	      /* Set up the "saved terminal modes" of the inferior
	         based on what modes we are starting it with.  */
	      target_terminal_init ();

	      /* Install inferior's terminal modes.  */
	      target_terminal_inferior ();

	      terminal_initted = 1;
	    }

	  if (--pending_execs == 0)
	    break;

	  /* Just make it go on.  */
	  target_resume (resume_ptid, 0, GDB_SIGNAL_0);
	}
    }

  /* Mark all threads non-executing.  */
  set_executing (resume_ptid, 0);
}

/* Implement the "unset exec-wrapper" command.  */

static void
unset_exec_wrapper_command (char *args, int from_tty)
{
  xfree (exec_wrapper);
  exec_wrapper = NULL;
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_fork_child;

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
}
