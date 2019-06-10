/* Fork a Unix child process, and set up to debug it, for GDB.

   Copyright (C) 1990-2017 Free Software Foundation, Inc.

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
#include "terminal.h"
#include "target.h"
#include "gdb_wait.h"
#include "gdb_vfork.h"
#include "gdbcore.h"
#include "gdbthread.h"
#include "command.h" /* for dont_repeat () */
#include "gdbcmd.h"
#include "solib.h"
#include "filestuff.h"
#include "top.h"
#include "signals-state-save-restore.h"
#include <signal.h>
#include <vector>

/* This just gets used as a default if we can't find SHELL.  */
#define SHELL_FILE "/bin/sh"

extern char **environ;

static char *exec_wrapper;

/* Build the argument vector for execv(3).  */

class execv_argv
{
public:
  /* EXEC_FILE is the file to run.  ALLARGS is a string containing the
     arguments to the program.  If starting with a shell, SHELL_FILE
     is the shell to run.  Otherwise, SHELL_FILE is NULL.  */
  execv_argv (const char *exec_file, const std::string &allargs,
	      const char *shell_file);

  /* Return a pointer to the built argv, in the type expected by
     execv.  The result is (only) valid for as long as this execv_argv
     object is live.  We return a "char **" because that's the type
     that the execv functions expect.  Note that it is guaranteed that
     the execv functions do not modify the argv[] array nor the
     strings to which the array point.  */
  char **argv ()
  {
    return const_cast<char **> (&m_argv[0]);
  }

private:
  /* Disable copying.  */
  execv_argv (const execv_argv &) = delete;
  void operator= (const execv_argv &) = delete;

  /* Helper methods for constructing the argument vector.  */

  /* Used when building an argv for a straight execv call, without
     going via the shell.  */
  void init_for_no_shell (const char *exec_file,
			  const std::string &allargs);

  /* Used when building an argv for execing a shell that execs the
     child program.  */
  void init_for_shell (const char *exec_file,
		       const std::string &allargs,
		       const char *shell_file);

  /* The argument vector built.  Holds non-owning pointers.  Elements
     either point to the strings passed to the execv_argv ctor, or
     inside M_STORAGE.  */
  std::vector<const char *> m_argv;

  /* Storage.  In the no-shell case, this contains a copy of the
     arguments passed to the ctor, split by '\0'.  In the shell case,
     this contains the quoted shell command.  I.e., SHELL_COMMAND in
     {"$SHELL" "-c", SHELL_COMMAND, NULL}.  */
  std::string m_storage;
};

/* Create argument vector for straight call to execvp.  Breaks up
   ALLARGS into an argument vector suitable for passing to execvp and
   stores it in M_ARGV.  E.g., on "run a b c d" this routine would get
   as input the string "a b c d", and as output it would fill in
   M_ARGV with the four arguments "a", "b", "c", "d".  Each argument
   in M_ARGV points to a substring of a copy of ALLARGS stored in
   M_STORAGE.  */

void
execv_argv::init_for_no_shell (const char *exec_file,
			       const std::string &allargs)
{

  /* Save/work with a copy stored in our storage.  The pointers pushed
     to M_ARGV point directly into M_STORAGE, which is modified in
     place with the necessary NULL terminators.  This avoids N heap
     allocations and string dups when 1 is sufficient.  */
  std::string &args_copy = m_storage = allargs;

  m_argv.push_back (exec_file);

  for (size_t cur_pos = 0; cur_pos < args_copy.size ();)
    {
      /* Skip whitespace-like chars.  */
      std::size_t pos = args_copy.find_first_not_of (" \t\n", cur_pos);

      if (pos != std::string::npos)
	cur_pos = pos;

      /* Find the position of the next separator.  */
      std::size_t next_sep = args_copy.find_first_of (" \t\n", cur_pos);

      if (next_sep == std::string::npos)
	{
	  /* No separator found, which means this is the last
	     argument.  */
	  next_sep = args_copy.size ();
	}
      else
	{
	  /* Replace the separator with a terminator.  */
	  args_copy[next_sep++] = '\0';
	}

      m_argv.push_back (&args_copy[cur_pos]);

      cur_pos = next_sep;
    }

  /* NULL-terminate the vector.  */
  m_argv.push_back (NULL);
}

/* When executing a command under the given shell, return true if the
   '!' character should be escaped when embedded in a quoted
   command-line argument.  */

static bool
escape_bang_in_quoted_argument (const char *shell_file)
{
  size_t shell_file_len = strlen (shell_file);

  /* Bang should be escaped only in C Shells.  For now, simply check
     that the shell name ends with 'csh', which covers at least csh
     and tcsh.  This should be good enough for now.  */

  if (shell_file_len < 3)
    return false;

  if (shell_file[shell_file_len - 3] == 'c'
      && shell_file[shell_file_len - 2] == 's'
      && shell_file[shell_file_len - 1] == 'h')
    return true;

  return false;
}

/* See declaration.  */

execv_argv::execv_argv (const char *exec_file,
			const std::string &allargs,
			const char *shell_file)
{
  if (shell_file == NULL)
    init_for_no_shell (exec_file, allargs);
  else
    init_for_shell (exec_file, allargs, shell_file);
}

/* See declaration.  */

void
execv_argv::init_for_shell (const char *exec_file,
			    const std::string &allargs,
			    const char *shell_file)
{
  /* We're going to call a shell.  */
  bool escape_bang = escape_bang_in_quoted_argument (shell_file);

  /* We need to build a new shell command string, and make argv point
     to it.  So build it in the storage.  */
  std::string &shell_command = m_storage;

  shell_command = "exec ";

  /* Add any exec wrapper.  That may be a program name with arguments,
     so the user must handle quoting.  */
  if (exec_wrapper)
    {
      shell_command += exec_wrapper;
      shell_command += ' ';
    }

  /* Now add exec_file, quoting as necessary.  */

  /* Quoting in this style is said to work with all shells.  But csh
     on IRIX 4.0.1 can't deal with it.  So we only quote it if we need
     to.  */
  bool need_to_quote;
  const char *p = exec_file;
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
	  need_to_quote = true;
	  goto end_scan;

	case '\0':
	  need_to_quote = false;
	  goto end_scan;

	default:
	  break;
	}
      ++p;
    }
 end_scan:
  if (need_to_quote)
    {
      shell_command += '\'';
      for (p = exec_file; *p != '\0'; ++p)
	{
	  if (*p == '\'')
	    shell_command += "'\\''";
	  else if (*p == '!' && escape_bang)
	    shell_command += "\\!";
	  else
	    shell_command += *p;
	}
      shell_command += '\'';
    }
  else
    shell_command += exec_file;

  shell_command += ' ' + allargs;

  /* If we decided above to start up with a shell, we exec the shell.
     "-c" says to interpret the next arg as a shell command to
     execute, and this command is "exec <target-program> <args>".  */
  m_argv.reserve (4);
  m_argv.push_back (shell_file);
  m_argv.push_back ("-c");
  m_argv.push_back (shell_command.c_str ());
  m_argv.push_back (NULL);
}

/* See inferior.h.  */

void
trace_start_error (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  fprintf_unfiltered (gdb_stderr, "Could not trace the inferior "
		                  "process.\nError: ");
  vfprintf_unfiltered (gdb_stderr, fmt, ap);
  va_end (ap);

  gdb_flush (gdb_stderr);
  _exit (0177);
}

/* See inferior.h.  */

void
trace_start_error_with_name (const char *string)
{
  trace_start_error ("%s: %s", string, safe_strerror (errno));
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
fork_inferior (const char *exec_file_arg, const std::string &allargs,
	       char **env, void (*traceme_fun) (void),
	       void (*init_trace_fun) (int), void (*pre_trace_fun) (void),
	       char *shell_file_arg,
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
  static const char *exec_file;
  char **save_our_env;
  const char *inferior_io_terminal = get_inferior_io_terminal ();
  struct inferior *inf;
  int i;
  int save_errno;
  struct ui *save_ui;

  /* If no exec file handed to us, get it from the exec-file command
     -- with a good, common error message if none is specified.  */
  if (exec_file_arg == NULL)
    exec_file = get_exec_file (1);
  else
    exec_file = exec_file_arg;

  /* 'startup_with_shell' is declared in inferior.h and bound to the
     "set startup-with-shell" option.  If 0, we'll just do a
     fork/exec, no shell, so don't bother figuring out what shell.  */
  if (startup_with_shell)
    {
      shell_file = shell_file_arg;
      /* Figure out what shell to start up the user program under.  */
      if (shell_file == NULL)
	shell_file = getenv ("SHELL");
      if (shell_file == NULL)
	shell_file = default_shell_file;
    }
  else
    shell_file = NULL;

  /* Build the argument vector.  */
  execv_argv child_argv (exec_file, allargs, shell_file);

  /* Retain a copy of our environment variables, since the child will
     replace the value of environ and if we're vforked, we have to
     restore it.  */
  save_our_env = environ;

  /* Likewise the current UI.  */
  save_ui = current_ui;

  /* Tell the terminal handling subsystem what tty we plan to run on;
     it will just record the information for later.  */
  new_tty_prefork (inferior_io_terminal);

  /* It is generally good practice to flush any possible pending stdio
     output prior to doing a fork, to avoid the possibility of both
     the parent and child flushing the same data after the fork.  */
  gdb_flush (main_ui->m_gdb_stdout);
  gdb_flush (main_ui->m_gdb_stderr);

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
      /* Switch to the main UI, so that gdb_std{in/out/err} in the
	 child are mapped to std{in/out/err}.  This makes it possible
	 to use fprintf_unfiltered/warning/error/etc. in the child
	 from here on.  */
      current_ui = main_ui;

      /* Close all file descriptors except those that gdb inherited
	 (usually 0/1/2), so they don't leak to the inferior.  Note
	 that this closes the file descriptors of all secondary
	 UIs.  */
      close_most_fds ();

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

      restore_original_signals_state ();

      /* There is no execlpe call, so we have to set the environment
         for our child in the global variable.  If we've vforked, this
         clobbers the parent, but environ is restored a few lines down
         in the parent.  By the way, yes we do need to look down the
         path to find $SHELL.  Rich Pixley says so, and I agree.  */
      environ = env;

      char **argv = child_argv.argv ();

      if (exec_fun != NULL)
        (*exec_fun) (argv[0], &argv[0], env);
      else
        execvp (argv[0], &argv[0]);

      /* If we get here, it's an error.  */
      save_errno = errno;
      fprintf_unfiltered (gdb_stderr, "Cannot exec %s", argv[0]);
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

  /* Likewise the current UI.  */
  current_ui = save_ui;

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

  if (startup_with_shell)
    {
      /* One trap extra for exec'ing the shell.  */
      pending_execs++;
    }

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
	    target_mourn_inferior (event_ptid);
	    error (_("During startup program terminated with signal %s, %s."),
		   gdb_signal_to_name (ws.value.sig),
		   gdb_signal_to_string (ws.value.sig));
	    return;

	  case TARGET_WAITKIND_EXITED:
	    target_terminal_ours ();
	    target_mourn_inferior (event_ptid);
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
	  target_continue (resume_ptid, resume_signal);
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
	  target_continue_no_signal (resume_ptid);
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

static void
show_startup_with_shell (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Use of shell to start subprocesses is %s.\n"),
		    value);
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

  add_setshow_boolean_cmd ("startup-with-shell", class_support,
			   &startup_with_shell, _("\
Set use of shell to start subprocesses.  The default is on."), _("\
Show use of shell to start subprocesses."), NULL,
			   NULL,
			   show_startup_with_shell,
			   &setlist, &showlist);
}
