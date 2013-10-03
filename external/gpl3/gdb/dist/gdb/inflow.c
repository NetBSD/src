/* Low level interface to ptrace, for GDB when running under Unix.
   Copyright (C) 1986-2013 Free Software Foundation, Inc.

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
#include "command.h"
#include "serial.h"
#include "terminal.h"
#include "target.h"
#include "gdbthread.h"
#include "observer.h"

#include "gdb_string.h"
#include <signal.h>
#include <fcntl.h>
#include "gdb_select.h"

#include "inflow.h"
#include "gdbcmd.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifndef O_NOCTTY
#define O_NOCTTY 0
#endif

extern void _initialize_inflow (void);

static void pass_signal (int);

static void terminal_ours_1 (int);

/* Record terminal status separately for debugger and inferior.  */

static struct serial *stdin_serial;

/* Terminal related info we need to keep track of.  Each inferior
   holds an instance of this structure --- we save it whenever the
   corresponding inferior stops, and restore it to the foreground
   inferior when it resumes.  */
struct terminal_info
{
  /* The name of the tty (from the `tty' command) that we gave to the
     inferior when it was started.  */
  char *run_terminal;

  /* TTY state.  We save it whenever the inferior stops, and restore
     it when it resumes.  */
  serial_ttystate ttystate;

#ifdef PROCESS_GROUP_TYPE
  /* Process group.  Saved and restored just like ttystate.  */
  PROCESS_GROUP_TYPE process_group;
#endif

  /* fcntl flags.  Saved and restored just like ttystate.  */
  int tflags;
};

/* Our own tty state, which we restore every time we need to deal with
   the terminal.  This is only set once, when GDB first starts.  The
   settings of flags which readline saves and restores and
   unimportant.  */
static struct terminal_info our_terminal_info;

static struct terminal_info *get_inflow_inferior_data (struct inferior *);

#ifdef PROCESS_GROUP_TYPE

/* Return the process group of the current inferior.  */

PROCESS_GROUP_TYPE
inferior_process_group (void)
{
  return get_inflow_inferior_data (current_inferior ())->process_group;
}
#endif

/* While the inferior is running, we want SIGINT and SIGQUIT to go to the
   inferior only.  If we have job control, that takes care of it.  If not,
   we save our handlers in these two variables and set SIGINT and SIGQUIT
   to SIG_IGN.  */

static void (*sigint_ours) ();
static void (*sigquit_ours) ();

/* The name of the tty (from the `tty' command) that we're giving to
   the inferior when starting it up.  This is only (and should only
   be) used as a transient global by new_tty_prefork,
   create_tty_session, new_tty and new_tty_postfork, all called from
   fork_inferior, while forking a new child.  */
static const char *inferior_thisrun_terminal;

/* Nonzero if our terminal settings are in effect.  Zero if the
   inferior's settings are in effect.  Ignored if !gdb_has_a_terminal
   ().  */

int terminal_is_ours;

#ifdef PROCESS_GROUP_TYPE
static PROCESS_GROUP_TYPE
gdb_getpgrp (void)
{
  int process_group = -1;

#ifdef HAVE_TERMIOS
  process_group = tcgetpgrp (0);
#endif
#ifdef HAVE_TERMIO
  process_group = getpgrp ();
#endif
#ifdef HAVE_SGTTY
  ioctl (0, TIOCGPGRP, &process_group);
#endif
  return process_group;
}
#endif

enum
  {
    yes, no, have_not_checked
  }
gdb_has_a_terminal_flag = have_not_checked;

/* The value of the "interactive-mode" setting.  */
static enum auto_boolean interactive_mode = AUTO_BOOLEAN_AUTO;

/* Implement the "show interactive-mode" option.  */

static void
show_interactive_mode (struct ui_file *file, int from_tty,
                       struct cmd_list_element *c,
                       const char *value)
{
  if (interactive_mode == AUTO_BOOLEAN_AUTO)
    fprintf_filtered (file, "Debugger's interactive mode "
		            "is %s (currently %s).\n",
                      value, gdb_has_a_terminal () ? "on" : "off");
  else
    fprintf_filtered (file, "Debugger's interactive mode is %s.\n", value);
}

/* Does GDB have a terminal (on stdin)?  */
int
gdb_has_a_terminal (void)
{
  if (interactive_mode != AUTO_BOOLEAN_AUTO)
    return interactive_mode == AUTO_BOOLEAN_TRUE;

  switch (gdb_has_a_terminal_flag)
    {
    case yes:
      return 1;
    case no:
      return 0;
    case have_not_checked:
      /* Get all the current tty settings (including whether we have a
         tty at all!).  Can't do this in _initialize_inflow because
         serial_fdopen() won't work until the serial_ops_list is
         initialized.  */

#ifdef F_GETFL
      our_terminal_info.tflags = fcntl (0, F_GETFL, 0);
#endif

      gdb_has_a_terminal_flag = no;
      if (stdin_serial != NULL)
	{
	  our_terminal_info.ttystate = serial_get_tty_state (stdin_serial);

	  if (our_terminal_info.ttystate != NULL)
	    {
	      gdb_has_a_terminal_flag = yes;
#ifdef PROCESS_GROUP_TYPE
	      our_terminal_info.process_group = gdb_getpgrp ();
#endif
	    }
	}

      return gdb_has_a_terminal_flag == yes;
    default:
      /* "Can't happen".  */
      return 0;
    }
}

/* Macro for printing errors from ioctl operations */

#define	OOPSY(what)	\
  if (result == -1)	\
    fprintf_unfiltered(gdb_stderr, "[%s failed in terminal_inferior: %s]\n", \
	    what, safe_strerror (errno))

static void terminal_ours_1 (int);

/* Initialize the terminal settings we record for the inferior,
   before we actually run the inferior.  */

void
terminal_init_inferior_with_pgrp (int pgrp)
{
  if (gdb_has_a_terminal ())
    {
      struct inferior *inf = current_inferior ();
      struct terminal_info *tinfo = get_inflow_inferior_data (inf);

      xfree (tinfo->ttystate);
      tinfo->ttystate = serial_copy_tty_state (stdin_serial,
					       our_terminal_info.ttystate);

#ifdef PROCESS_GROUP_TYPE
      tinfo->process_group = pgrp;
#endif

      /* Make sure that next time we call terminal_inferior (which will be
         before the program runs, as it needs to be), we install the new
         process group.  */
      terminal_is_ours = 1;
    }
}

/* Save the terminal settings again.  This is necessary for the TUI
   when it switches to TUI or non-TUI mode;  curses changes the terminal
   and gdb must be able to restore it correctly.  */

void
terminal_save_ours (void)
{
  if (gdb_has_a_terminal ())
    {
      xfree (our_terminal_info.ttystate);
      our_terminal_info.ttystate = serial_get_tty_state (stdin_serial);
    }
}

void
terminal_init_inferior (void)
{
#ifdef PROCESS_GROUP_TYPE
  /* This is for Lynx, and should be cleaned up by having Lynx be a separate
     debugging target with a version of target_terminal_init_inferior which
     passes in the process group to a generic routine which does all the work
     (and the non-threaded child_terminal_init_inferior can just pass in
     inferior_ptid to the same routine).  */
  /* We assume INFERIOR_PID is also the child's process group.  */
  terminal_init_inferior_with_pgrp (PIDGET (inferior_ptid));
#endif /* PROCESS_GROUP_TYPE */
}

/* Put the inferior's terminal settings into effect.
   This is preparation for starting or resuming the inferior.  */

void
terminal_inferior (void)
{
  struct inferior *inf;
  struct terminal_info *tinfo;

  if (!terminal_is_ours)
    return;

  inf = current_inferior ();
  tinfo = get_inflow_inferior_data (inf);

  if (gdb_has_a_terminal ()
      && tinfo->ttystate != NULL
      && tinfo->run_terminal == NULL)
    {
      int result;

#ifdef F_GETFL
      /* Is there a reason this is being done twice?  It happens both
         places we use F_SETFL, so I'm inclined to think perhaps there
         is some reason, however perverse.  Perhaps not though...  */
      result = fcntl (0, F_SETFL, tinfo->tflags);
      result = fcntl (0, F_SETFL, tinfo->tflags);
      OOPSY ("fcntl F_SETFL");
#endif

      /* Because we were careful to not change in or out of raw mode in
         terminal_ours, we will not change in our out of raw mode with
         this call, so we don't flush any input.  */
      result = serial_set_tty_state (stdin_serial,
				     tinfo->ttystate);
      OOPSY ("setting tty state");

      if (!job_control)
	{
	  sigint_ours = (void (*)()) signal (SIGINT, SIG_IGN);
#ifdef SIGQUIT
	  sigquit_ours = (void (*)()) signal (SIGQUIT, SIG_IGN);
#endif
	}

      /* If attach_flag is set, we don't know whether we are sharing a
         terminal with the inferior or not.  (attaching a process
         without a terminal is one case where we do not; attaching a
         process which we ran from the same shell as GDB via `&' is
         one case where we do, I think (but perhaps this is not
         `sharing' in the sense that we need to save and restore tty
         state)).  I don't know if there is any way to tell whether we
         are sharing a terminal.  So what we do is to go through all
         the saving and restoring of the tty state, but ignore errors
         setting the process group, which will happen if we are not
         sharing a terminal).  */

      if (job_control)
	{
#ifdef HAVE_TERMIOS
	  result = tcsetpgrp (0, tinfo->process_group);
	  if (!inf->attach_flag)
	    OOPSY ("tcsetpgrp");
#endif

#ifdef HAVE_SGTTY
	  result = ioctl (0, TIOCSPGRP, &tinfo->process_group);
	  if (!inf->attach_flag)
	    OOPSY ("TIOCSPGRP");
#endif
	}

    }
  terminal_is_ours = 0;
}

/* Put some of our terminal settings into effect,
   enough to get proper results from our output,
   but do not change into or out of RAW mode
   so that no input is discarded.

   After doing this, either terminal_ours or terminal_inferior
   should be called to get back to a normal state of affairs.  */

void
terminal_ours_for_output (void)
{
  terminal_ours_1 (1);
}

/* Put our terminal settings into effect.
   First record the inferior's terminal settings
   so they can be restored properly later.  */

void
terminal_ours (void)
{
  terminal_ours_1 (0);
}

/* output_only is not used, and should not be used unless we introduce
   separate terminal_is_ours and terminal_is_ours_for_output
   flags.  */

static void
terminal_ours_1 (int output_only)
{
  struct inferior *inf;
  struct terminal_info *tinfo;

  if (terminal_is_ours)
    return;

  terminal_is_ours = 1;

  /* Checking inferior->run_terminal is necessary so that
     if GDB is running in the background, it won't block trying
     to do the ioctl()'s below.  Checking gdb_has_a_terminal
     avoids attempting all the ioctl's when running in batch.  */

  inf = current_inferior ();
  tinfo = get_inflow_inferior_data (inf);

  if (tinfo->run_terminal != NULL || gdb_has_a_terminal () == 0)
    return;

    {
#ifdef SIGTTOU
      /* Ignore this signal since it will happen when we try to set the
         pgrp.  */
      void (*osigttou) () = NULL;
#endif
      int result;

#ifdef SIGTTOU
      if (job_control)
	osigttou = (void (*)()) signal (SIGTTOU, SIG_IGN);
#endif

      xfree (tinfo->ttystate);
      tinfo->ttystate = serial_get_tty_state (stdin_serial);

#ifdef PROCESS_GROUP_TYPE
      if (!inf->attach_flag)
	/* If setpgrp failed in terminal_inferior, this would give us
	   our process group instead of the inferior's.  See
	   terminal_inferior for details.  */
	tinfo->process_group = gdb_getpgrp ();
#endif

      /* Here we used to set ICANON in our ttystate, but I believe this
         was an artifact from before when we used readline.  Readline sets
         the tty state when it needs to.
         FIXME-maybe: However, query() expects non-raw mode and doesn't
         use readline.  Maybe query should use readline (on the other hand,
         this only matters for HAVE_SGTTY, not termio or termios, I think).  */

      /* Set tty state to our_ttystate.  We don't change in our out of raw
         mode, to avoid flushing input.  We need to do the same thing
         regardless of output_only, because we don't have separate
         terminal_is_ours and terminal_is_ours_for_output flags.  It's OK,
         though, since readline will deal with raw mode when/if it needs
         to.  */

      serial_noflush_set_tty_state (stdin_serial, our_terminal_info.ttystate,
				    tinfo->ttystate);

      if (job_control)
	{
#ifdef HAVE_TERMIOS
	  result = tcsetpgrp (0, our_terminal_info.process_group);
#if 0
	  /* This fails on Ultrix with EINVAL if you run the testsuite
	     in the background with nohup, and then log out.  GDB never
	     used to check for an error here, so perhaps there are other
	     such situations as well.  */
	  if (result == -1)
	    fprintf_unfiltered (gdb_stderr,
				"[tcsetpgrp failed in terminal_ours: %s]\n",
				safe_strerror (errno));
#endif
#endif /* termios */

#ifdef HAVE_SGTTY
	  result = ioctl (0, TIOCSPGRP, &our_terminal_info.process_group);
#endif
	}

#ifdef SIGTTOU
      if (job_control)
	signal (SIGTTOU, osigttou);
#endif

      if (!job_control)
	{
	  signal (SIGINT, sigint_ours);
#ifdef SIGQUIT
	  signal (SIGQUIT, sigquit_ours);
#endif
	}

#ifdef F_GETFL
      tinfo->tflags = fcntl (0, F_GETFL, 0);

      /* Is there a reason this is being done twice?  It happens both
         places we use F_SETFL, so I'm inclined to think perhaps there
         is some reason, however perverse.  Perhaps not though...  */
      result = fcntl (0, F_SETFL, our_terminal_info.tflags);
      result = fcntl (0, F_SETFL, our_terminal_info.tflags);
#endif
    }
}

/* Per-inferior data key.  */
static const struct inferior_data *inflow_inferior_data;

static void
inflow_inferior_data_cleanup (struct inferior *inf, void *arg)
{
  struct terminal_info *info;

  info = inferior_data (inf, inflow_inferior_data);
  if (info != NULL)
    {
      xfree (info->run_terminal);
      xfree (info->ttystate);
      xfree (info);
    }
}

/* Get the current svr4 data.  If none is found yet, add it now.  This
   function always returns a valid object.  */

static struct terminal_info *
get_inflow_inferior_data (struct inferior *inf)
{
  struct terminal_info *info;

  info = inferior_data (inf, inflow_inferior_data);
  if (info == NULL)
    {
      info = XZALLOC (struct terminal_info);
      set_inferior_data (inf, inflow_inferior_data, info);
    }

  return info;
}

/* This is a "inferior_exit" observer.  Releases the TERMINAL_INFO member
   of the inferior structure.  This field is private to inflow.c, and
   its type is opaque to the rest of GDB.  PID is the target pid of
   the inferior that is about to be removed from the inferior
   list.  */

static void
inflow_inferior_exit (struct inferior *inf)
{
  struct terminal_info *info;

  info = inferior_data (inf, inflow_inferior_data);
  if (info != NULL)
    {
      xfree (info->run_terminal);
      xfree (info->ttystate);
      xfree (info);
      set_inferior_data (inf, inflow_inferior_data, NULL);
    }
}

void
copy_terminal_info (struct inferior *to, struct inferior *from)
{
  struct terminal_info *tinfo_to, *tinfo_from;

  tinfo_to = get_inflow_inferior_data (to);
  tinfo_from = get_inflow_inferior_data (from);

  xfree (tinfo_to->run_terminal);
  xfree (tinfo_to->ttystate);

  *tinfo_to = *tinfo_from;

  if (tinfo_from->run_terminal)
    tinfo_to->run_terminal
      = xstrdup (tinfo_from->run_terminal);

  if (tinfo_from->ttystate)
    tinfo_to->ttystate
      = serial_copy_tty_state (stdin_serial, tinfo_from->ttystate);
}

void
term_info (char *arg, int from_tty)
{
  target_terminal_info (arg, from_tty);
}

void
child_terminal_info (char *args, int from_tty)
{
  struct inferior *inf;
  struct terminal_info *tinfo;

  if (!gdb_has_a_terminal ())
    {
      printf_filtered (_("This GDB does not control a terminal.\n"));
      return;
    }

  if (ptid_equal (inferior_ptid, null_ptid))
    return;

  inf = current_inferior ();
  tinfo = get_inflow_inferior_data (inf);

  printf_filtered (_("Inferior's terminal status "
		     "(currently saved by GDB):\n"));

  /* First the fcntl flags.  */
  {
    int flags;

    flags = tinfo->tflags;

    printf_filtered ("File descriptor flags = ");

#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif
    /* (O_ACCMODE) parens are to avoid Ultrix header file bug.  */
    switch (flags & (O_ACCMODE))
      {
      case O_RDONLY:
	printf_filtered ("O_RDONLY");
	break;
      case O_WRONLY:
	printf_filtered ("O_WRONLY");
	break;
      case O_RDWR:
	printf_filtered ("O_RDWR");
	break;
      }
    flags &= ~(O_ACCMODE);

#ifdef O_NONBLOCK
    if (flags & O_NONBLOCK)
      printf_filtered (" | O_NONBLOCK");
    flags &= ~O_NONBLOCK;
#endif

#if defined (O_NDELAY)
    /* If O_NDELAY and O_NONBLOCK are defined to the same thing, we will
       print it as O_NONBLOCK, which is good cause that is what POSIX
       has, and the flag will already be cleared by the time we get here.  */
    if (flags & O_NDELAY)
      printf_filtered (" | O_NDELAY");
    flags &= ~O_NDELAY;
#endif

    if (flags & O_APPEND)
      printf_filtered (" | O_APPEND");
    flags &= ~O_APPEND;

#if defined (O_BINARY)
    if (flags & O_BINARY)
      printf_filtered (" | O_BINARY");
    flags &= ~O_BINARY;
#endif

    if (flags)
      printf_filtered (" | 0x%x", flags);
    printf_filtered ("\n");
  }

#ifdef PROCESS_GROUP_TYPE
  printf_filtered ("Process group = %d\n", (int) tinfo->process_group);
#endif

  serial_print_tty_state (stdin_serial, tinfo->ttystate, gdb_stdout);
}

/* NEW_TTY_PREFORK is called before forking a new child process,
   so we can record the state of ttys in the child to be formed.
   TTYNAME is null if we are to share the terminal with gdb;
   or points to a string containing the name of the desired tty.

   NEW_TTY is called in new child processes under Unix, which will
   become debugger target processes.  This actually switches to
   the terminal specified in the NEW_TTY_PREFORK call.  */

void
new_tty_prefork (const char *ttyname)
{
  /* Save the name for later, for determining whether we and the child
     are sharing a tty.  */
  inferior_thisrun_terminal = ttyname;
}

#if !defined(__GO32__) && !defined(_WIN32)
/* If RESULT, assumed to be the return value from a system call, is
   negative, print the error message indicated by errno and exit.
   MSG should identify the operation that failed.  */
static void
check_syscall (const char *msg, int result)
{
  if (result < 0)
    {
      print_sys_errmsg (msg, errno);
      _exit (1);
    }
}
#endif

void
new_tty (void)
{
  int tty;

  if (inferior_thisrun_terminal == 0)
    return;
#if !defined(__GO32__) && !defined(_WIN32)
#ifdef TIOCNOTTY
  /* Disconnect the child process from our controlling terminal.  On some
     systems (SVR4 for example), this may cause a SIGTTOU, so temporarily
     ignore SIGTTOU.  */
  tty = open ("/dev/tty", O_RDWR);
  if (tty > 0)
    {
      void (*osigttou) ();

      osigttou = (void (*)()) signal (SIGTTOU, SIG_IGN);
      ioctl (tty, TIOCNOTTY, 0);
      close (tty);
      signal (SIGTTOU, osigttou);
    }
#endif

  /* Now open the specified new terminal.  */
  tty = open (inferior_thisrun_terminal, O_RDWR | O_NOCTTY);
  check_syscall (inferior_thisrun_terminal, tty);

  /* Avoid use of dup2; doesn't exist on all systems.  */
  if (tty != 0)
    {
      close (0);
      check_syscall ("dup'ing tty into fd 0", dup (tty));
    }
  if (tty != 1)
    {
      close (1);
      check_syscall ("dup'ing tty into fd 1", dup (tty));
    }
  if (tty != 2)
    {
      close (2);
      check_syscall ("dup'ing tty into fd 2", dup (tty));
    }

#ifdef TIOCSCTTY
  /* Make tty our new controlling terminal.  */
  if (ioctl (tty, TIOCSCTTY, 0) == -1)
    /* Mention GDB in warning because it will appear in the inferior's
       terminal instead of GDB's.  */
    warning (_("GDB: Failed to set controlling terminal: %s"),
	     safe_strerror (errno));
#endif

  if (tty > 2)
    close (tty);
#endif /* !go32 && !win32 */
}

/* NEW_TTY_POSTFORK is called after forking a new child process, and
   adding it to the inferior table, to store the TTYNAME being used by
   the child, or null if it sharing the terminal with gdb.  */

void
new_tty_postfork (void)
{
  /* Save the name for later, for determining whether we and the child
     are sharing a tty.  */

  if (inferior_thisrun_terminal)
    {
      struct inferior *inf = current_inferior ();
      struct terminal_info *tinfo = get_inflow_inferior_data (inf);

      tinfo->run_terminal = xstrdup (inferior_thisrun_terminal);
    }

  inferior_thisrun_terminal = NULL;
}


/* Call set_sigint_trap when you need to pass a signal on to an attached
   process when handling SIGINT.  */

static void
pass_signal (int signo)
{
#ifndef _WIN32
  kill (PIDGET (inferior_ptid), SIGINT);
#endif
}

static void (*osig) ();
static int osig_set;

void
set_sigint_trap (void)
{
  struct inferior *inf = current_inferior ();
  struct terminal_info *tinfo = get_inflow_inferior_data (inf);

  if (inf->attach_flag || tinfo->run_terminal)
    {
      osig = (void (*)()) signal (SIGINT, pass_signal);
      osig_set = 1;
    }
  else
    osig_set = 0;
}

void
clear_sigint_trap (void)
{
  if (osig_set)
    {
      signal (SIGINT, osig);
      osig_set = 0;
    }
}


/* Create a new session if the inferior will run in a different tty.
   A session is UNIX's way of grouping processes that share a controlling
   terminal, so a new one is needed if the inferior terminal will be
   different from GDB's.

   Returns the session id of the new session, 0 if no session was created
   or -1 if an error occurred.  */
pid_t
create_tty_session (void)
{
#ifdef HAVE_SETSID
  pid_t ret;

  if (!job_control || inferior_thisrun_terminal == 0)
    return 0;

  ret = setsid ();
  if (ret == -1)
    warning (_("Failed to create new terminal session: setsid: %s"),
	     safe_strerror (errno));

  return ret;
#else
  return 0;
#endif /* HAVE_SETSID */
}

/* This is here because this is where we figure out whether we (probably)
   have job control.  Just using job_control only does part of it because
   setpgid or setpgrp might not exist on a system without job control.
   It might be considered misplaced (on the other hand, process groups and
   job control are closely related to ttys).

   For a more clean implementation, in libiberty, put a setpgid which merely
   calls setpgrp and a setpgrp which does nothing (any system with job control
   will have one or the other).  */
int
gdb_setpgid (void)
{
  int retval = 0;

  if (job_control)
    {
#if defined (HAVE_TERMIOS) || defined (TIOCGPGRP)
#ifdef HAVE_SETPGID
      /* The call setpgid (0, 0) is supposed to work and mean the same
         thing as this, but on Ultrix 4.2A it fails with EPERM (and
         setpgid (getpid (), getpid ()) succeeds).  */
      retval = setpgid (getpid (), getpid ());
#else
#ifdef HAVE_SETPGRP
#ifdef SETPGRP_VOID 
      retval = setpgrp ();
#else
      retval = setpgrp (getpid (), getpid ());
#endif
#endif /* HAVE_SETPGRP */
#endif /* HAVE_SETPGID */
#endif /* defined (HAVE_TERMIOS) || defined (TIOCGPGRP) */
    }

  return retval;
}

/* Get all the current tty settings (including whether we have a
   tty at all!).  We can't do this in _initialize_inflow because
   serial_fdopen() won't work until the serial_ops_list is
   initialized, but we don't want to do it lazily either, so
   that we can guarantee stdin_serial is opened if there is
   a terminal.  */
void
initialize_stdin_serial (void)
{
  stdin_serial = serial_fdopen (0);
}

void
_initialize_inflow (void)
{
  add_info ("terminal", term_info,
	    _("Print inferior's saved terminal status."));

  add_setshow_auto_boolean_cmd ("interactive-mode", class_support,
                                &interactive_mode, _("\
Set whether GDB's standard input is a terminal."), _("\
Show whether GDB's standard input is a terminal."), _("\
If on, GDB assumes that standard input is a terminal.  In practice, it\n\
means that GDB should wait for the user to answer queries associated to\n\
commands entered at the command prompt.  If off, GDB assumes that standard\n\
input is not a terminal, and uses the default answer to all queries.\n\
If auto (the default), determine which mode to use based on the standard\n\
input settings."),
                        NULL,
                        show_interactive_mode,
                        &setlist, &showlist);

  terminal_is_ours = 1;

  /* OK, figure out whether we have job control.  If neither termios nor
     sgtty (i.e. termio or go32), leave job_control 0.  */

#if defined (HAVE_TERMIOS)
  /* Do all systems with termios have the POSIX way of identifying job
     control?  I hope so.  */
#ifdef _POSIX_JOB_CONTROL
  job_control = 1;
#else
#ifdef _SC_JOB_CONTROL
  job_control = sysconf (_SC_JOB_CONTROL);
#else
  job_control = 0;		/* Have to assume the worst.  */
#endif /* _SC_JOB_CONTROL */
#endif /* _POSIX_JOB_CONTROL */
#endif /* HAVE_TERMIOS */

#ifdef HAVE_SGTTY
#ifdef TIOCGPGRP
  job_control = 1;
#else
  job_control = 0;
#endif /* TIOCGPGRP */
#endif /* sgtty */

  observer_attach_inferior_exit (inflow_inferior_exit);

  inflow_inferior_data
    = register_inferior_data_with_cleanup (NULL, inflow_inferior_data_cleanup);
}
