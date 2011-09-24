/* GNU/Linux native-dependent code common to multiple platforms.

   Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
   2011 Free Software Foundation, Inc.

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
#include "target.h"
#include "gdb_string.h"
#include "gdb_wait.h"
#include "gdb_assert.h"
#ifdef HAVE_TKILL_SYSCALL
#include <unistd.h>
#include <sys/syscall.h>
#endif
#include <sys/ptrace.h>
#include "linux-nat.h"
#include "linux-fork.h"
#include "gdbthread.h"
#include "gdbcmd.h"
#include "regcache.h"
#include "regset.h"
#include "inf-ptrace.h"
#include "auxv.h"
#include <sys/param.h>		/* for MAXPATHLEN */
#include <sys/procfs.h>		/* for elf_gregset etc.  */
#include "elf-bfd.h"		/* for elfcore_write_* */
#include "gregset.h"		/* for gregset */
#include "gdbcore.h"		/* for get_exec_file */
#include <ctype.h>		/* for isdigit */
#include "gdbthread.h"		/* for struct thread_info etc.  */
#include "gdb_stat.h"		/* for struct stat */
#include <fcntl.h>		/* for O_RDONLY */
#include "inf-loop.h"
#include "event-loop.h"
#include "event-top.h"
#include <pwd.h>
#include <sys/types.h>
#include "gdb_dirent.h"
#include "xml-support.h"
#include "terminal.h"
#include <sys/vfs.h>
#include "solib.h"

#ifndef SPUFS_MAGIC
#define SPUFS_MAGIC 0x23c9b64e
#endif

#ifdef HAVE_PERSONALITY
# include <sys/personality.h>
# if !HAVE_DECL_ADDR_NO_RANDOMIZE
#  define ADDR_NO_RANDOMIZE 0x0040000
# endif
#endif /* HAVE_PERSONALITY */

/* This comment documents high-level logic of this file.

Waiting for events in sync mode
===============================

When waiting for an event in a specific thread, we just use waitpid, passing
the specific pid, and not passing WNOHANG.

When waiting for an event in all threads, waitpid is not quite good.  Prior to
version 2.4, Linux can either wait for event in main thread, or in secondary
threads.  (2.4 has the __WALL flag).  So, if we use blocking waitpid, we might
miss an event.  The solution is to use non-blocking waitpid, together with
sigsuspend.  First, we use non-blocking waitpid to get an event in the main 
process, if any.  Second, we use non-blocking waitpid with the __WCLONED
flag to check for events in cloned processes.  If nothing is found, we use
sigsuspend to wait for SIGCHLD.  When SIGCHLD arrives, it means something
happened to a child process -- and SIGCHLD will be delivered both for events
in main debugged process and in cloned processes.  As soon as we know there's
an event, we get back to calling nonblocking waitpid with and without 
__WCLONED.

Note that SIGCHLD should be blocked between waitpid and sigsuspend calls,
so that we don't miss a signal.  If SIGCHLD arrives in between, when it's
blocked, the signal becomes pending and sigsuspend immediately
notices it and returns.

Waiting for events in async mode
================================

In async mode, GDB should always be ready to handle both user input
and target events, so neither blocking waitpid nor sigsuspend are
viable options.  Instead, we should asynchronously notify the GDB main
event loop whenever there's an unprocessed event from the target.  We
detect asynchronous target events by handling SIGCHLD signals.  To
notify the event loop about target events, the self-pipe trick is used
--- a pipe is registered as waitable event source in the event loop,
the event loop select/poll's on the read end of this pipe (as well on
other event sources, e.g., stdin), and the SIGCHLD handler writes a
byte to this pipe.  This is more portable than relying on
pselect/ppoll, since on kernels that lack those syscalls, libc
emulates them with select/poll+sigprocmask, and that is racy
(a.k.a. plain broken).

Obviously, if we fail to notify the event loop if there's a target
event, it's bad.  OTOH, if we notify the event loop when there's no
event from the target, linux_nat_wait will detect that there's no real
event to report, and return event of type TARGET_WAITKIND_IGNORE.
This is mostly harmless, but it will waste time and is better avoided.

The main design point is that every time GDB is outside linux-nat.c,
we have a SIGCHLD handler installed that is called when something
happens to the target and notifies the GDB event loop.  Whenever GDB
core decides to handle the event, and calls into linux-nat.c, we
process things as in sync mode, except that the we never block in
sigsuspend.

While processing an event, we may end up momentarily blocked in
waitpid calls.  Those waitpid calls, while blocking, are guarantied to
return quickly.  E.g., in all-stop mode, before reporting to the core
that an LWP hit a breakpoint, all LWPs are stopped by sending them
SIGSTOP, and synchronously waiting for the SIGSTOP to be reported.
Note that this is different from blocking indefinitely waiting for the
next event --- here, we're already handling an event.

Use of signals
==============

We stop threads by sending a SIGSTOP.  The use of SIGSTOP instead of another
signal is not entirely significant; we just need for a signal to be delivered,
so that we can intercept it.  SIGSTOP's advantage is that it can not be
blocked.  A disadvantage is that it is not a real-time signal, so it can only
be queued once; we do not keep track of other sources of SIGSTOP.

Two other signals that can't be blocked are SIGCONT and SIGKILL.  But we can't
use them, because they have special behavior when the signal is generated -
not when it is delivered.  SIGCONT resumes the entire thread group and SIGKILL
kills the entire thread group.

A delivered SIGSTOP would stop the entire thread group, not just the thread we
tkill'd.  But we never let the SIGSTOP be delivered; we always intercept and 
cancel it (by PTRACE_CONT without passing SIGSTOP).

We could use a real-time signal instead.  This would solve those problems; we
could use PTRACE_GETSIGINFO to locate the specific stop signals sent by GDB.
But we would still have to have some support for SIGSTOP, since PTRACE_ATTACH
generates it, and there are races with trying to find a signal that is not
blocked.  */

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

/* If the system headers did not provide the constants, hard-code the normal
   values.  */
#ifndef PTRACE_EVENT_FORK

#define PTRACE_SETOPTIONS	0x4200
#define PTRACE_GETEVENTMSG	0x4201

/* Options set using PTRACE_SETOPTIONS.  */
#define PTRACE_O_TRACESYSGOOD	0x00000001
#define PTRACE_O_TRACEFORK	0x00000002
#define PTRACE_O_TRACEVFORK	0x00000004
#define PTRACE_O_TRACECLONE	0x00000008
#define PTRACE_O_TRACEEXEC	0x00000010
#define PTRACE_O_TRACEVFORKDONE	0x00000020
#define PTRACE_O_TRACEEXIT	0x00000040

/* Wait extended result codes for the above trace options.  */
#define PTRACE_EVENT_FORK	1
#define PTRACE_EVENT_VFORK	2
#define PTRACE_EVENT_CLONE	3
#define PTRACE_EVENT_EXEC	4
#define PTRACE_EVENT_VFORK_DONE	5
#define PTRACE_EVENT_EXIT	6

#endif /* PTRACE_EVENT_FORK */

/* Unlike other extended result codes, WSTOPSIG (status) on
   PTRACE_O_TRACESYSGOOD syscall events doesn't return SIGTRAP, but
   instead SIGTRAP with bit 7 set.  */
#define SYSCALL_SIGTRAP (SIGTRAP | 0x80)

/* We can't always assume that this flag is available, but all systems
   with the ptrace event handlers also have __WALL, so it's safe to use
   here.  */
#ifndef __WALL
#define __WALL          0x40000000 /* Wait for any child.  */
#endif

#ifndef PTRACE_GETSIGINFO
# define PTRACE_GETSIGINFO    0x4202
# define PTRACE_SETSIGINFO    0x4203
#endif

/* The single-threaded native GNU/Linux target_ops.  We save a pointer for
   the use of the multi-threaded target.  */
static struct target_ops *linux_ops;
static struct target_ops linux_ops_saved;

/* The method to call, if any, when a new thread is attached.  */
static void (*linux_nat_new_thread) (ptid_t);

/* The method to call, if any, when the siginfo object needs to be
   converted between the layout returned by ptrace, and the layout in
   the architecture of the inferior.  */
static int (*linux_nat_siginfo_fixup) (struct siginfo *,
				       gdb_byte *,
				       int);

/* The saved to_xfer_partial method, inherited from inf-ptrace.c.
   Called by our to_xfer_partial.  */
static LONGEST (*super_xfer_partial) (struct target_ops *, 
				      enum target_object,
				      const char *, gdb_byte *, 
				      const gdb_byte *,
				      ULONGEST, LONGEST);

static int debug_linux_nat;
static void
show_debug_linux_nat (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Debugging of GNU/Linux lwp module is %s.\n"),
		    value);
}

static int debug_linux_nat_async = 0;
static void
show_debug_linux_nat_async (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Debugging of GNU/Linux async lwp module is %s.\n"),
		    value);
}

static int disable_randomization = 1;

static void
show_disable_randomization (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c, const char *value)
{
#ifdef HAVE_PERSONALITY
  fprintf_filtered (file,
		    _("Disabling randomization of debuggee's "
		      "virtual address space is %s.\n"),
		    value);
#else /* !HAVE_PERSONALITY */
  fputs_filtered (_("Disabling randomization of debuggee's "
		    "virtual address space is unsupported on\n"
		    "this platform.\n"), file);
#endif /* !HAVE_PERSONALITY */
}

static void
set_disable_randomization (char *args, int from_tty,
			   struct cmd_list_element *c)
{
#ifndef HAVE_PERSONALITY
  error (_("Disabling randomization of debuggee's "
	   "virtual address space is unsupported on\n"
	   "this platform."));
#endif /* !HAVE_PERSONALITY */
}

struct simple_pid_list
{
  int pid;
  int status;
  struct simple_pid_list *next;
};
struct simple_pid_list *stopped_pids;

/* This variable is a tri-state flag: -1 for unknown, 0 if PTRACE_O_TRACEFORK
   can not be used, 1 if it can.  */

static int linux_supports_tracefork_flag = -1;

/* This variable is a tri-state flag: -1 for unknown, 0 if
   PTRACE_O_TRACESYSGOOD can not be used, 1 if it can.  */

static int linux_supports_tracesysgood_flag = -1;

/* If we have PTRACE_O_TRACEFORK, this flag indicates whether we also have
   PTRACE_O_TRACEVFORKDONE.  */

static int linux_supports_tracevforkdone_flag = -1;

/* Async mode support.  */

/* Zero if the async mode, although enabled, is masked, which means
   linux_nat_wait should behave as if async mode was off.  */
static int linux_nat_async_mask_value = 1;

/* Stores the current used ptrace() options.  */
static int current_ptrace_options = 0;

/* The read/write ends of the pipe registered as waitable file in the
   event loop.  */
static int linux_nat_event_pipe[2] = { -1, -1 };

/* Flush the event pipe.  */

static void
async_file_flush (void)
{
  int ret;
  char buf;

  do
    {
      ret = read (linux_nat_event_pipe[0], &buf, 1);
    }
  while (ret >= 0 || (ret == -1 && errno == EINTR));
}

/* Put something (anything, doesn't matter what, or how much) in event
   pipe, so that the select/poll in the event-loop realizes we have
   something to process.  */

static void
async_file_mark (void)
{
  int ret;

  /* It doesn't really matter what the pipe contains, as long we end
     up with something in it.  Might as well flush the previous
     left-overs.  */
  async_file_flush ();

  do
    {
      ret = write (linux_nat_event_pipe[1], "+", 1);
    }
  while (ret == -1 && errno == EINTR);

  /* Ignore EAGAIN.  If the pipe is full, the event loop will already
     be awakened anyway.  */
}

static void linux_nat_async (void (*callback)
			     (enum inferior_event_type event_type,
			      void *context),
			     void *context);
static int linux_nat_async_mask (int mask);
static int kill_lwp (int lwpid, int signo);

static int stop_callback (struct lwp_info *lp, void *data);

static void block_child_signals (sigset_t *prev_mask);
static void restore_child_signals_mask (sigset_t *prev_mask);

struct lwp_info;
static struct lwp_info *add_lwp (ptid_t ptid);
static void purge_lwp_list (int pid);
static struct lwp_info *find_lwp_pid (ptid_t ptid);


/* Trivial list manipulation functions to keep track of a list of
   new stopped processes.  */
static void
add_to_pid_list (struct simple_pid_list **listp, int pid, int status)
{
  struct simple_pid_list *new_pid = xmalloc (sizeof (struct simple_pid_list));

  new_pid->pid = pid;
  new_pid->status = status;
  new_pid->next = *listp;
  *listp = new_pid;
}

static int
pull_pid_from_list (struct simple_pid_list **listp, int pid, int *statusp)
{
  struct simple_pid_list **p;

  for (p = listp; *p != NULL; p = &(*p)->next)
    if ((*p)->pid == pid)
      {
	struct simple_pid_list *next = (*p)->next;

	*statusp = (*p)->status;
	xfree (*p);
	*p = next;
	return 1;
      }
  return 0;
}

static void
linux_record_stopped_pid (int pid, int status)
{
  add_to_pid_list (&stopped_pids, pid, status);
}


/* A helper function for linux_test_for_tracefork, called after fork ().  */

static void
linux_tracefork_child (void)
{
  ptrace (PTRACE_TRACEME, 0, 0, 0);
  kill (getpid (), SIGSTOP);
  fork ();
  _exit (0);
}

/* Wrapper function for waitpid which handles EINTR.  */

static int
my_waitpid (int pid, int *statusp, int flags)
{
  int ret;

  do
    {
      ret = waitpid (pid, statusp, flags);
    }
  while (ret == -1 && errno == EINTR);

  return ret;
}

/* Determine if PTRACE_O_TRACEFORK can be used to follow fork events.

   First, we try to enable fork tracing on ORIGINAL_PID.  If this fails,
   we know that the feature is not available.  This may change the tracing
   options for ORIGINAL_PID, but we'll be setting them shortly anyway.

   However, if it succeeds, we don't know for sure that the feature is
   available; old versions of PTRACE_SETOPTIONS ignored unknown options.  We
   create a child process, attach to it, use PTRACE_SETOPTIONS to enable
   fork tracing, and let it fork.  If the process exits, we assume that we
   can't use TRACEFORK; if we get the fork notification, and we can extract
   the new child's PID, then we assume that we can.  */

static void
linux_test_for_tracefork (int original_pid)
{
  int child_pid, ret, status;
  long second_pid;
  sigset_t prev_mask;

  /* We don't want those ptrace calls to be interrupted.  */
  block_child_signals (&prev_mask);

  linux_supports_tracefork_flag = 0;
  linux_supports_tracevforkdone_flag = 0;

  ret = ptrace (PTRACE_SETOPTIONS, original_pid, 0, PTRACE_O_TRACEFORK);
  if (ret != 0)
    {
      restore_child_signals_mask (&prev_mask);
      return;
    }

  child_pid = fork ();
  if (child_pid == -1)
    perror_with_name (("fork"));

  if (child_pid == 0)
    linux_tracefork_child ();

  ret = my_waitpid (child_pid, &status, 0);
  if (ret == -1)
    perror_with_name (("waitpid"));
  else if (ret != child_pid)
    error (_("linux_test_for_tracefork: waitpid: unexpected result %d."), ret);
  if (! WIFSTOPPED (status))
    error (_("linux_test_for_tracefork: waitpid: unexpected status %d."),
	   status);

  ret = ptrace (PTRACE_SETOPTIONS, child_pid, 0, PTRACE_O_TRACEFORK);
  if (ret != 0)
    {
      ret = ptrace (PTRACE_KILL, child_pid, 0, 0);
      if (ret != 0)
	{
	  warning (_("linux_test_for_tracefork: failed to kill child"));
	  restore_child_signals_mask (&prev_mask);
	  return;
	}

      ret = my_waitpid (child_pid, &status, 0);
      if (ret != child_pid)
	warning (_("linux_test_for_tracefork: failed "
		   "to wait for killed child"));
      else if (!WIFSIGNALED (status))
	warning (_("linux_test_for_tracefork: unexpected "
		   "wait status 0x%x from killed child"), status);

      restore_child_signals_mask (&prev_mask);
      return;
    }

  /* Check whether PTRACE_O_TRACEVFORKDONE is available.  */
  ret = ptrace (PTRACE_SETOPTIONS, child_pid, 0,
		PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORKDONE);
  linux_supports_tracevforkdone_flag = (ret == 0);

  ret = ptrace (PTRACE_CONT, child_pid, 0, 0);
  if (ret != 0)
    warning (_("linux_test_for_tracefork: failed to resume child"));

  ret = my_waitpid (child_pid, &status, 0);

  if (ret == child_pid && WIFSTOPPED (status)
      && status >> 16 == PTRACE_EVENT_FORK)
    {
      second_pid = 0;
      ret = ptrace (PTRACE_GETEVENTMSG, child_pid, 0, &second_pid);
      if (ret == 0 && second_pid != 0)
	{
	  int second_status;

	  linux_supports_tracefork_flag = 1;
	  my_waitpid (second_pid, &second_status, 0);
	  ret = ptrace (PTRACE_KILL, second_pid, 0, 0);
	  if (ret != 0)
	    warning (_("linux_test_for_tracefork: "
		       "failed to kill second child"));
	  my_waitpid (second_pid, &status, 0);
	}
    }
  else
    warning (_("linux_test_for_tracefork: unexpected result from waitpid "
	     "(%d, status 0x%x)"), ret, status);

  ret = ptrace (PTRACE_KILL, child_pid, 0, 0);
  if (ret != 0)
    warning (_("linux_test_for_tracefork: failed to kill child"));
  my_waitpid (child_pid, &status, 0);

  restore_child_signals_mask (&prev_mask);
}

/* Determine if PTRACE_O_TRACESYSGOOD can be used to follow syscalls.

   We try to enable syscall tracing on ORIGINAL_PID.  If this fails,
   we know that the feature is not available.  This may change the tracing
   options for ORIGINAL_PID, but we'll be setting them shortly anyway.  */

static void
linux_test_for_tracesysgood (int original_pid)
{
  int ret;
  sigset_t prev_mask;

  /* We don't want those ptrace calls to be interrupted.  */
  block_child_signals (&prev_mask);

  linux_supports_tracesysgood_flag = 0;

  ret = ptrace (PTRACE_SETOPTIONS, original_pid, 0, PTRACE_O_TRACESYSGOOD);
  if (ret != 0)
    goto out;

  linux_supports_tracesysgood_flag = 1;
out:
  restore_child_signals_mask (&prev_mask);
}

/* Determine wether we support PTRACE_O_TRACESYSGOOD option available.
   This function also sets linux_supports_tracesysgood_flag.  */

static int
linux_supports_tracesysgood (int pid)
{
  if (linux_supports_tracesysgood_flag == -1)
    linux_test_for_tracesysgood (pid);
  return linux_supports_tracesysgood_flag;
}

/* Return non-zero iff we have tracefork functionality available.
   This function also sets linux_supports_tracefork_flag.  */

static int
linux_supports_tracefork (int pid)
{
  if (linux_supports_tracefork_flag == -1)
    linux_test_for_tracefork (pid);
  return linux_supports_tracefork_flag;
}

static int
linux_supports_tracevforkdone (int pid)
{
  if (linux_supports_tracefork_flag == -1)
    linux_test_for_tracefork (pid);
  return linux_supports_tracevforkdone_flag;
}

static void
linux_enable_tracesysgood (ptid_t ptid)
{
  int pid = ptid_get_lwp (ptid);

  if (pid == 0)
    pid = ptid_get_pid (ptid);

  if (linux_supports_tracesysgood (pid) == 0)
    return;

  current_ptrace_options |= PTRACE_O_TRACESYSGOOD;

  ptrace (PTRACE_SETOPTIONS, pid, 0, current_ptrace_options);
}


void
linux_enable_event_reporting (ptid_t ptid)
{
  int pid = ptid_get_lwp (ptid);

  if (pid == 0)
    pid = ptid_get_pid (ptid);

  if (! linux_supports_tracefork (pid))
    return;

  current_ptrace_options |= PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK
    | PTRACE_O_TRACEEXEC | PTRACE_O_TRACECLONE;

  if (linux_supports_tracevforkdone (pid))
    current_ptrace_options |= PTRACE_O_TRACEVFORKDONE;

  /* Do not enable PTRACE_O_TRACEEXIT until GDB is more prepared to support
     read-only process state.  */

  ptrace (PTRACE_SETOPTIONS, pid, 0, current_ptrace_options);
}

static void
linux_child_post_attach (int pid)
{
  linux_enable_event_reporting (pid_to_ptid (pid));
  check_for_thread_db ();
  linux_enable_tracesysgood (pid_to_ptid (pid));
}

static void
linux_child_post_startup_inferior (ptid_t ptid)
{
  linux_enable_event_reporting (ptid);
  check_for_thread_db ();
  linux_enable_tracesysgood (ptid);
}

static int
linux_child_follow_fork (struct target_ops *ops, int follow_child)
{
  sigset_t prev_mask;
  int has_vforked;
  int parent_pid, child_pid;

  block_child_signals (&prev_mask);

  has_vforked = (inferior_thread ()->pending_follow.kind
		 == TARGET_WAITKIND_VFORKED);
  parent_pid = ptid_get_lwp (inferior_ptid);
  if (parent_pid == 0)
    parent_pid = ptid_get_pid (inferior_ptid);
  child_pid = PIDGET (inferior_thread ()->pending_follow.value.related_pid);

  if (!detach_fork)
    linux_enable_event_reporting (pid_to_ptid (child_pid));

  if (has_vforked
      && !non_stop /* Non-stop always resumes both branches.  */
      && (!target_is_async_p () || sync_execution)
      && !(follow_child || detach_fork || sched_multi))
    {
      /* The parent stays blocked inside the vfork syscall until the
	 child execs or exits.  If we don't let the child run, then
	 the parent stays blocked.  If we're telling the parent to run
	 in the foreground, the user will not be able to ctrl-c to get
	 back the terminal, effectively hanging the debug session.  */
      fprintf_filtered (gdb_stderr, _("\
Can not resume the parent process over vfork in the foreground while\n\
holding the child stopped.  Try \"set detach-on-fork\" or \
\"set schedule-multiple\".\n"));
      /* FIXME output string > 80 columns.  */
      return 1;
    }

  if (! follow_child)
    {
      struct lwp_info *child_lp = NULL;

      /* We're already attached to the parent, by default.  */

      /* Detach new forked process?  */
      if (detach_fork)
	{
	  /* Before detaching from the child, remove all breakpoints
	     from it.  If we forked, then this has already been taken
	     care of by infrun.c.  If we vforked however, any
	     breakpoint inserted in the parent is visible in the
	     child, even those added while stopped in a vfork
	     catchpoint.  This will remove the breakpoints from the
	     parent also, but they'll be reinserted below.  */
	  if (has_vforked)
	    {
	      /* keep breakpoints list in sync.  */
	      remove_breakpoints_pid (GET_PID (inferior_ptid));
	    }

	  if (info_verbose || debug_linux_nat)
	    {
	      target_terminal_ours ();
	      fprintf_filtered (gdb_stdlog,
				"Detaching after fork from "
				"child process %d.\n",
				child_pid);
	    }

	  ptrace (PTRACE_DETACH, child_pid, 0, 0);
	}
      else
	{
	  struct inferior *parent_inf, *child_inf;
	  struct cleanup *old_chain;

	  /* Add process to GDB's tables.  */
	  child_inf = add_inferior (child_pid);

	  parent_inf = current_inferior ();
	  child_inf->attach_flag = parent_inf->attach_flag;
	  copy_terminal_info (child_inf, parent_inf);

	  old_chain = save_inferior_ptid ();
	  save_current_program_space ();

	  inferior_ptid = ptid_build (child_pid, child_pid, 0);
	  add_thread (inferior_ptid);
	  child_lp = add_lwp (inferior_ptid);
	  child_lp->stopped = 1;
	  child_lp->resumed = 1;

	  /* If this is a vfork child, then the address-space is
	     shared with the parent.  */
	  if (has_vforked)
	    {
	      child_inf->pspace = parent_inf->pspace;
	      child_inf->aspace = parent_inf->aspace;

	      /* The parent will be frozen until the child is done
		 with the shared region.  Keep track of the
		 parent.  */
	      child_inf->vfork_parent = parent_inf;
	      child_inf->pending_detach = 0;
	      parent_inf->vfork_child = child_inf;
	      parent_inf->pending_detach = 0;
	    }
	  else
	    {
	      child_inf->aspace = new_address_space ();
	      child_inf->pspace = add_program_space (child_inf->aspace);
	      child_inf->removable = 1;
	      set_current_program_space (child_inf->pspace);
	      clone_program_space (child_inf->pspace, parent_inf->pspace);

	      /* Let the shared library layer (solib-svr4) learn about
		 this new process, relocate the cloned exec, pull in
		 shared libraries, and install the solib event
		 breakpoint.  If a "cloned-VM" event was propagated
		 better throughout the core, this wouldn't be
		 required.  */
	      solib_create_inferior_hook (0);
	    }

	  /* Let the thread_db layer learn about this new process.  */
	  check_for_thread_db ();

	  do_cleanups (old_chain);
	}

      if (has_vforked)
	{
	  struct lwp_info *lp;
	  struct inferior *parent_inf;

	  parent_inf = current_inferior ();

	  /* If we detached from the child, then we have to be careful
	     to not insert breakpoints in the parent until the child
	     is done with the shared memory region.  However, if we're
	     staying attached to the child, then we can and should
	     insert breakpoints, so that we can debug it.  A
	     subsequent child exec or exit is enough to know when does
	     the child stops using the parent's address space.  */
	  parent_inf->waiting_for_vfork_done = detach_fork;
	  parent_inf->pspace->breakpoints_not_allowed = detach_fork;

	  lp = find_lwp_pid (pid_to_ptid (parent_pid));
	  gdb_assert (linux_supports_tracefork_flag >= 0);
	  if (linux_supports_tracevforkdone (0))
	    {
  	      if (debug_linux_nat)
  		fprintf_unfiltered (gdb_stdlog,
  				    "LCFF: waiting for VFORK_DONE on %d\n",
  				    parent_pid);

	      lp->stopped = 1;
	      lp->resumed = 1;

	      /* We'll handle the VFORK_DONE event like any other
		 event, in target_wait.  */
	    }
	  else
	    {
	      /* We can't insert breakpoints until the child has
		 finished with the shared memory region.  We need to
		 wait until that happens.  Ideal would be to just
		 call:
		 - ptrace (PTRACE_SYSCALL, parent_pid, 0, 0);
		 - waitpid (parent_pid, &status, __WALL);
		 However, most architectures can't handle a syscall
		 being traced on the way out if it wasn't traced on
		 the way in.

		 We might also think to loop, continuing the child
		 until it exits or gets a SIGTRAP.  One problem is
		 that the child might call ptrace with PTRACE_TRACEME.

		 There's no simple and reliable way to figure out when
		 the vforked child will be done with its copy of the
		 shared memory.  We could step it out of the syscall,
		 two instructions, let it go, and then single-step the
		 parent once.  When we have hardware single-step, this
		 would work; with software single-step it could still
		 be made to work but we'd have to be able to insert
		 single-step breakpoints in the child, and we'd have
		 to insert -just- the single-step breakpoint in the
		 parent.  Very awkward.

		 In the end, the best we can do is to make sure it
		 runs for a little while.  Hopefully it will be out of
		 range of any breakpoints we reinsert.  Usually this
		 is only the single-step breakpoint at vfork's return
		 point.  */

  	      if (debug_linux_nat)
  		fprintf_unfiltered (gdb_stdlog,
				    "LCFF: no VFORK_DONE "
				    "support, sleeping a bit\n");

	      usleep (10000);

	      /* Pretend we've seen a PTRACE_EVENT_VFORK_DONE event,
		 and leave it pending.  The next linux_nat_resume call
		 will notice a pending event, and bypasses actually
		 resuming the inferior.  */
	      lp->status = 0;
	      lp->waitstatus.kind = TARGET_WAITKIND_VFORK_DONE;
	      lp->stopped = 0;
	      lp->resumed = 1;

	      /* If we're in async mode, need to tell the event loop
		 there's something here to process.  */
	      if (target_can_async_p ())
		async_file_mark ();
	    }
	}
    }
  else
    {
      struct inferior *parent_inf, *child_inf;
      struct lwp_info *lp;
      struct program_space *parent_pspace;

      if (info_verbose || debug_linux_nat)
	{
	  target_terminal_ours ();
	  if (has_vforked)
	    fprintf_filtered (gdb_stdlog,
			      _("Attaching after process %d "
				"vfork to child process %d.\n"),
			      parent_pid, child_pid);
	  else
	    fprintf_filtered (gdb_stdlog,
			      _("Attaching after process %d "
				"fork to child process %d.\n"),
			      parent_pid, child_pid);
	}

      /* Add the new inferior first, so that the target_detach below
	 doesn't unpush the target.  */

      child_inf = add_inferior (child_pid);

      parent_inf = current_inferior ();
      child_inf->attach_flag = parent_inf->attach_flag;
      copy_terminal_info (child_inf, parent_inf);

      parent_pspace = parent_inf->pspace;

      /* If we're vforking, we want to hold on to the parent until the
	 child exits or execs.  At child exec or exit time we can
	 remove the old breakpoints from the parent and detach or
	 resume debugging it.  Otherwise, detach the parent now; we'll
	 want to reuse it's program/address spaces, but we can't set
	 them to the child before removing breakpoints from the
	 parent, otherwise, the breakpoints module could decide to
	 remove breakpoints from the wrong process (since they'd be
	 assigned to the same address space).  */

      if (has_vforked)
	{
	  gdb_assert (child_inf->vfork_parent == NULL);
	  gdb_assert (parent_inf->vfork_child == NULL);
	  child_inf->vfork_parent = parent_inf;
	  child_inf->pending_detach = 0;
	  parent_inf->vfork_child = child_inf;
	  parent_inf->pending_detach = detach_fork;
	  parent_inf->waiting_for_vfork_done = 0;
	}
      else if (detach_fork)
	target_detach (NULL, 0);

      /* Note that the detach above makes PARENT_INF dangling.  */

      /* Add the child thread to the appropriate lists, and switch to
	 this new thread, before cloning the program space, and
	 informing the solib layer about this new process.  */

      inferior_ptid = ptid_build (child_pid, child_pid, 0);
      add_thread (inferior_ptid);
      lp = add_lwp (inferior_ptid);
      lp->stopped = 1;
      lp->resumed = 1;

      /* If this is a vfork child, then the address-space is shared
	 with the parent.  If we detached from the parent, then we can
	 reuse the parent's program/address spaces.  */
      if (has_vforked || detach_fork)
	{
	  child_inf->pspace = parent_pspace;
	  child_inf->aspace = child_inf->pspace->aspace;
	}
      else
	{
	  child_inf->aspace = new_address_space ();
	  child_inf->pspace = add_program_space (child_inf->aspace);
	  child_inf->removable = 1;
	  set_current_program_space (child_inf->pspace);
	  clone_program_space (child_inf->pspace, parent_pspace);

	  /* Let the shared library layer (solib-svr4) learn about
	     this new process, relocate the cloned exec, pull in
	     shared libraries, and install the solib event breakpoint.
	     If a "cloned-VM" event was propagated better throughout
	     the core, this wouldn't be required.  */
	  solib_create_inferior_hook (0);
	}

      /* Let the thread_db layer learn about this new process.  */
      check_for_thread_db ();
    }

  restore_child_signals_mask (&prev_mask);
  return 0;
}


static int
linux_child_insert_fork_catchpoint (int pid)
{
  return !linux_supports_tracefork (pid);
}

static int
linux_child_remove_fork_catchpoint (int pid)
{
  return 0;
}

static int
linux_child_insert_vfork_catchpoint (int pid)
{
  return !linux_supports_tracefork (pid);
}

static int
linux_child_remove_vfork_catchpoint (int pid)
{
  return 0;
}

static int
linux_child_insert_exec_catchpoint (int pid)
{
  return !linux_supports_tracefork (pid);
}

static int
linux_child_remove_exec_catchpoint (int pid)
{
  return 0;
}

static int
linux_child_set_syscall_catchpoint (int pid, int needed, int any_count,
				    int table_size, int *table)
{
  if (!linux_supports_tracesysgood (pid))
    return 1;

  /* On GNU/Linux, we ignore the arguments.  It means that we only
     enable the syscall catchpoints, but do not disable them.

     Also, we do not use the `table' information because we do not
     filter system calls here.  We let GDB do the logic for us.  */
  return 0;
}

/* On GNU/Linux there are no real LWP's.  The closest thing to LWP's
   are processes sharing the same VM space.  A multi-threaded process
   is basically a group of such processes.  However, such a grouping
   is almost entirely a user-space issue; the kernel doesn't enforce
   such a grouping at all (this might change in the future).  In
   general, we'll rely on the threads library (i.e. the GNU/Linux
   Threads library) to provide such a grouping.

   It is perfectly well possible to write a multi-threaded application
   without the assistance of a threads library, by using the clone
   system call directly.  This module should be able to give some
   rudimentary support for debugging such applications if developers
   specify the CLONE_PTRACE flag in the clone system call, and are
   using the Linux kernel 2.4 or above.

   Note that there are some peculiarities in GNU/Linux that affect
   this code:

   - In general one should specify the __WCLONE flag to waitpid in
     order to make it report events for any of the cloned processes
     (and leave it out for the initial process).  However, if a cloned
     process has exited the exit status is only reported if the
     __WCLONE flag is absent.  Linux kernel 2.4 has a __WALL flag, but
     we cannot use it since GDB must work on older systems too.

   - When a traced, cloned process exits and is waited for by the
     debugger, the kernel reassigns it to the original parent and
     keeps it around as a "zombie".  Somehow, the GNU/Linux Threads
     library doesn't notice this, which leads to the "zombie problem":
     When debugged a multi-threaded process that spawns a lot of
     threads will run out of processes, even if the threads exit,
     because the "zombies" stay around.  */

/* List of known LWPs.  */
struct lwp_info *lwp_list;


/* Original signal mask.  */
static sigset_t normal_mask;

/* Signal mask for use with sigsuspend in linux_nat_wait, initialized in
   _initialize_linux_nat.  */
static sigset_t suspend_mask;

/* Signals to block to make that sigsuspend work.  */
static sigset_t blocked_mask;

/* SIGCHLD action.  */
struct sigaction sigchld_action;

/* Block child signals (SIGCHLD and linux threads signals), and store
   the previous mask in PREV_MASK.  */

static void
block_child_signals (sigset_t *prev_mask)
{
  /* Make sure SIGCHLD is blocked.  */
  if (!sigismember (&blocked_mask, SIGCHLD))
    sigaddset (&blocked_mask, SIGCHLD);

  sigprocmask (SIG_BLOCK, &blocked_mask, prev_mask);
}

/* Restore child signals mask, previously returned by
   block_child_signals.  */

static void
restore_child_signals_mask (sigset_t *prev_mask)
{
  sigprocmask (SIG_SETMASK, prev_mask, NULL);
}


/* Prototypes for local functions.  */
static int stop_wait_callback (struct lwp_info *lp, void *data);
static int linux_thread_alive (ptid_t ptid);
static char *linux_child_pid_to_exec_file (int pid);


/* Convert wait status STATUS to a string.  Used for printing debug
   messages only.  */

static char *
status_to_str (int status)
{
  static char buf[64];

  if (WIFSTOPPED (status))
    {
      if (WSTOPSIG (status) == SYSCALL_SIGTRAP)
	snprintf (buf, sizeof (buf), "%s (stopped at syscall)",
		  strsignal (SIGTRAP));
      else
	snprintf (buf, sizeof (buf), "%s (stopped)",
		  strsignal (WSTOPSIG (status)));
    }
  else if (WIFSIGNALED (status))
    snprintf (buf, sizeof (buf), "%s (terminated)",
	      strsignal (WTERMSIG (status)));
  else
    snprintf (buf, sizeof (buf), "%d (exited)", WEXITSTATUS (status));

  return buf;
}

/* Remove all LWPs belong to PID from the lwp list.  */

static void
purge_lwp_list (int pid)
{
  struct lwp_info *lp, *lpprev, *lpnext;

  lpprev = NULL;

  for (lp = lwp_list; lp; lp = lpnext)
    {
      lpnext = lp->next;

      if (ptid_get_pid (lp->ptid) == pid)
	{
	  if (lp == lwp_list)
	    lwp_list = lp->next;
	  else
	    lpprev->next = lp->next;

	  xfree (lp);
	}
      else
	lpprev = lp;
    }
}

/* Return the number of known LWPs in the tgid given by PID.  */

static int
num_lwps (int pid)
{
  int count = 0;
  struct lwp_info *lp;

  for (lp = lwp_list; lp; lp = lp->next)
    if (ptid_get_pid (lp->ptid) == pid)
      count++;

  return count;
}

/* Add the LWP specified by PID to the list.  Return a pointer to the
   structure describing the new LWP.  The LWP should already be stopped
   (with an exception for the very first LWP).  */

static struct lwp_info *
add_lwp (ptid_t ptid)
{
  struct lwp_info *lp;

  gdb_assert (is_lwp (ptid));

  lp = (struct lwp_info *) xmalloc (sizeof (struct lwp_info));

  memset (lp, 0, sizeof (struct lwp_info));

  lp->waitstatus.kind = TARGET_WAITKIND_IGNORE;

  lp->ptid = ptid;
  lp->core = -1;

  lp->next = lwp_list;
  lwp_list = lp;

  if (num_lwps (GET_PID (ptid)) > 1 && linux_nat_new_thread != NULL)
    linux_nat_new_thread (ptid);

  return lp;
}

/* Remove the LWP specified by PID from the list.  */

static void
delete_lwp (ptid_t ptid)
{
  struct lwp_info *lp, *lpprev;

  lpprev = NULL;

  for (lp = lwp_list; lp; lpprev = lp, lp = lp->next)
    if (ptid_equal (lp->ptid, ptid))
      break;

  if (!lp)
    return;

  if (lpprev)
    lpprev->next = lp->next;
  else
    lwp_list = lp->next;

  xfree (lp);
}

/* Return a pointer to the structure describing the LWP corresponding
   to PID.  If no corresponding LWP could be found, return NULL.  */

static struct lwp_info *
find_lwp_pid (ptid_t ptid)
{
  struct lwp_info *lp;
  int lwp;

  if (is_lwp (ptid))
    lwp = GET_LWP (ptid);
  else
    lwp = GET_PID (ptid);

  for (lp = lwp_list; lp; lp = lp->next)
    if (lwp == GET_LWP (lp->ptid))
      return lp;

  return NULL;
}

/* Call CALLBACK with its second argument set to DATA for every LWP in
   the list.  If CALLBACK returns 1 for a particular LWP, return a
   pointer to the structure describing that LWP immediately.
   Otherwise return NULL.  */

struct lwp_info *
iterate_over_lwps (ptid_t filter,
		   int (*callback) (struct lwp_info *, void *),
		   void *data)
{
  struct lwp_info *lp, *lpnext;

  for (lp = lwp_list; lp; lp = lpnext)
    {
      lpnext = lp->next;

      if (ptid_match (lp->ptid, filter))
	{
	  if ((*callback) (lp, data))
	    return lp;
	}
    }

  return NULL;
}

/* Update our internal state when changing from one checkpoint to
   another indicated by NEW_PTID.  We can only switch single-threaded
   applications, so we only create one new LWP, and the previous list
   is discarded.  */

void
linux_nat_switch_fork (ptid_t new_ptid)
{
  struct lwp_info *lp;

  purge_lwp_list (GET_PID (inferior_ptid));

  lp = add_lwp (new_ptid);
  lp->stopped = 1;

  /* This changes the thread's ptid while preserving the gdb thread
     num.  Also changes the inferior pid, while preserving the
     inferior num.  */
  thread_change_ptid (inferior_ptid, new_ptid);

  /* We've just told GDB core that the thread changed target id, but,
     in fact, it really is a different thread, with different register
     contents.  */
  registers_changed ();
}

/* Handle the exit of a single thread LP.  */

static void
exit_lwp (struct lwp_info *lp)
{
  struct thread_info *th = find_thread_ptid (lp->ptid);

  if (th)
    {
      if (print_thread_events)
	printf_unfiltered (_("[%s exited]\n"), target_pid_to_str (lp->ptid));

      delete_thread (lp->ptid);
    }

  delete_lwp (lp->ptid);
}

/* Return an lwp's tgid, found in `/proc/PID/status'.  */

int
linux_proc_get_tgid (int lwpid)
{
  FILE *status_file;
  char buf[100];
  int tgid = -1;

  snprintf (buf, sizeof (buf), "/proc/%d/status", (int) lwpid);
  status_file = fopen (buf, "r");
  if (status_file != NULL)
    {
      while (fgets (buf, sizeof (buf), status_file))
	{
	  if (strncmp (buf, "Tgid:", 5) == 0)
	    {
	      tgid = strtoul (buf + strlen ("Tgid:"), NULL, 10);
	      break;
	    }
	}

      fclose (status_file);
    }

  return tgid;
}

/* Detect `T (stopped)' in `/proc/PID/status'.
   Other states including `T (tracing stop)' are reported as false.  */

static int
pid_is_stopped (pid_t pid)
{
  FILE *status_file;
  char buf[100];
  int retval = 0;

  snprintf (buf, sizeof (buf), "/proc/%d/status", (int) pid);
  status_file = fopen (buf, "r");
  if (status_file != NULL)
    {
      int have_state = 0;

      while (fgets (buf, sizeof (buf), status_file))
	{
	  if (strncmp (buf, "State:", 6) == 0)
	    {
	      have_state = 1;
	      break;
	    }
	}
      if (have_state && strstr (buf, "T (stopped)") != NULL)
	retval = 1;
      fclose (status_file);
    }
  return retval;
}

/* Wait for the LWP specified by LP, which we have just attached to.
   Returns a wait status for that LWP, to cache.  */

static int
linux_nat_post_attach_wait (ptid_t ptid, int first, int *cloned,
			    int *signalled)
{
  pid_t new_pid, pid = GET_LWP (ptid);
  int status;

  if (pid_is_stopped (pid))
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LNPAW: Attaching to a stopped process\n");

      /* The process is definitely stopped.  It is in a job control
	 stop, unless the kernel predates the TASK_STOPPED /
	 TASK_TRACED distinction, in which case it might be in a
	 ptrace stop.  Make sure it is in a ptrace stop; from there we
	 can kill it, signal it, et cetera.

         First make sure there is a pending SIGSTOP.  Since we are
	 already attached, the process can not transition from stopped
	 to running without a PTRACE_CONT; so we know this signal will
	 go into the queue.  The SIGSTOP generated by PTRACE_ATTACH is
	 probably already in the queue (unless this kernel is old
	 enough to use TASK_STOPPED for ptrace stops); but since SIGSTOP
	 is not an RT signal, it can only be queued once.  */
      kill_lwp (pid, SIGSTOP);

      /* Finally, resume the stopped process.  This will deliver the SIGSTOP
	 (or a higher priority signal, just like normal PTRACE_ATTACH).  */
      ptrace (PTRACE_CONT, pid, 0, 0);
    }

  /* Make sure the initial process is stopped.  The user-level threads
     layer might want to poke around in the inferior, and that won't
     work if things haven't stabilized yet.  */
  new_pid = my_waitpid (pid, &status, 0);
  if (new_pid == -1 && errno == ECHILD)
    {
      if (first)
	warning (_("%s is a cloned process"), target_pid_to_str (ptid));

      /* Try again with __WCLONE to check cloned processes.  */
      new_pid = my_waitpid (pid, &status, __WCLONE);
      *cloned = 1;
    }

  gdb_assert (pid == new_pid);

  if (!WIFSTOPPED (status))
    {
      /* The pid we tried to attach has apparently just exited.  */
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog, "LNPAW: Failed to stop %d: %s",
			    pid, status_to_str (status));
      return status;
    }

  if (WSTOPSIG (status) != SIGSTOP)
    {
      *signalled = 1;
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LNPAW: Received %s after attaching\n",
			    status_to_str (status));
    }

  return status;
}

/* Attach to the LWP specified by PID.  Return 0 if successful or -1
   if the new LWP could not be attached.  */

int
lin_lwp_attach_lwp (ptid_t ptid)
{
  struct lwp_info *lp;
  sigset_t prev_mask;

  gdb_assert (is_lwp (ptid));

  block_child_signals (&prev_mask);

  lp = find_lwp_pid (ptid);

  /* We assume that we're already attached to any LWP that has an id
     equal to the overall process id, and to any LWP that is already
     in our list of LWPs.  If we're not seeing exit events from threads
     and we've had PID wraparound since we last tried to stop all threads,
     this assumption might be wrong; fortunately, this is very unlikely
     to happen.  */
  if (GET_LWP (ptid) != GET_PID (ptid) && lp == NULL)
    {
      int status, cloned = 0, signalled = 0;

      if (ptrace (PTRACE_ATTACH, GET_LWP (ptid), 0, 0) < 0)
	{
	  /* If we fail to attach to the thread, issue a warning,
	     but continue.  One way this can happen is if thread
	     creation is interrupted; as of Linux kernel 2.6.19, a
	     bug may place threads in the thread list and then fail
	     to create them.  */
	  warning (_("Can't attach %s: %s"), target_pid_to_str (ptid),
		   safe_strerror (errno));
	  restore_child_signals_mask (&prev_mask);
	  return -1;
	}

      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LLAL: PTRACE_ATTACH %s, 0, 0 (OK)\n",
			    target_pid_to_str (ptid));

      status = linux_nat_post_attach_wait (ptid, 0, &cloned, &signalled);
      if (!WIFSTOPPED (status))
	return -1;

      lp = add_lwp (ptid);
      lp->stopped = 1;
      lp->cloned = cloned;
      lp->signalled = signalled;
      if (WSTOPSIG (status) != SIGSTOP)
	{
	  lp->resumed = 1;
	  lp->status = status;
	}

      target_post_attach (GET_LWP (lp->ptid));

      if (debug_linux_nat)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "LLAL: waitpid %s received %s\n",
			      target_pid_to_str (ptid),
			      status_to_str (status));
	}
    }
  else
    {
      /* We assume that the LWP representing the original process is
         already stopped.  Mark it as stopped in the data structure
         that the GNU/linux ptrace layer uses to keep track of
         threads.  Note that this won't have already been done since
         the main thread will have, we assume, been stopped by an
         attach from a different layer.  */
      if (lp == NULL)
	lp = add_lwp (ptid);
      lp->stopped = 1;
    }

  restore_child_signals_mask (&prev_mask);
  return 0;
}

static void
linux_nat_create_inferior (struct target_ops *ops, 
			   char *exec_file, char *allargs, char **env,
			   int from_tty)
{
#ifdef HAVE_PERSONALITY
  int personality_orig = 0, personality_set = 0;
#endif /* HAVE_PERSONALITY */

  /* The fork_child mechanism is synchronous and calls target_wait, so
     we have to mask the async mode.  */

#ifdef HAVE_PERSONALITY
  if (disable_randomization)
    {
      errno = 0;
      personality_orig = personality (0xffffffff);
      if (errno == 0 && !(personality_orig & ADDR_NO_RANDOMIZE))
	{
	  personality_set = 1;
	  personality (personality_orig | ADDR_NO_RANDOMIZE);
	}
      if (errno != 0 || (personality_set
			 && !(personality (0xffffffff) & ADDR_NO_RANDOMIZE)))
	warning (_("Error disabling address space randomization: %s"),
		 safe_strerror (errno));
    }
#endif /* HAVE_PERSONALITY */

  linux_ops->to_create_inferior (ops, exec_file, allargs, env, from_tty);

#ifdef HAVE_PERSONALITY
  if (personality_set)
    {
      errno = 0;
      personality (personality_orig);
      if (errno != 0)
	warning (_("Error restoring address space randomization: %s"),
		 safe_strerror (errno));
    }
#endif /* HAVE_PERSONALITY */
}

static void
linux_nat_attach (struct target_ops *ops, char *args, int from_tty)
{
  struct lwp_info *lp;
  int status;
  ptid_t ptid;

  linux_ops->to_attach (ops, args, from_tty);

  /* The ptrace base target adds the main thread with (pid,0,0)
     format.  Decorate it with lwp info.  */
  ptid = BUILD_LWP (GET_PID (inferior_ptid), GET_PID (inferior_ptid));
  thread_change_ptid (inferior_ptid, ptid);

  /* Add the initial process as the first LWP to the list.  */
  lp = add_lwp (ptid);

  status = linux_nat_post_attach_wait (lp->ptid, 1, &lp->cloned,
				       &lp->signalled);
  if (!WIFSTOPPED (status))
    {
      if (WIFEXITED (status))
	{
	  int exit_code = WEXITSTATUS (status);

	  target_terminal_ours ();
	  target_mourn_inferior ();
	  if (exit_code == 0)
	    error (_("Unable to attach: program exited normally."));
	  else
	    error (_("Unable to attach: program exited with code %d."),
		   exit_code);
	}
      else if (WIFSIGNALED (status))
	{
	  enum target_signal signo;

	  target_terminal_ours ();
	  target_mourn_inferior ();

	  signo = target_signal_from_host (WTERMSIG (status));
	  error (_("Unable to attach: program terminated with signal "
		   "%s, %s."),
		 target_signal_to_name (signo),
		 target_signal_to_string (signo));
	}

      internal_error (__FILE__, __LINE__,
		      _("unexpected status %d for PID %ld"),
		      status, (long) GET_LWP (ptid));
    }

  lp->stopped = 1;

  /* Save the wait status to report later.  */
  lp->resumed = 1;
  if (debug_linux_nat)
    fprintf_unfiltered (gdb_stdlog,
			"LNA: waitpid %ld, saving status %s\n",
			(long) GET_PID (lp->ptid), status_to_str (status));

  lp->status = status;

  if (target_can_async_p ())
    target_async (inferior_event_handler, 0);
}

/* Get pending status of LP.  */
static int
get_pending_status (struct lwp_info *lp, int *status)
{
  enum target_signal signo = TARGET_SIGNAL_0;

  /* If we paused threads momentarily, we may have stored pending
     events in lp->status or lp->waitstatus (see stop_wait_callback),
     and GDB core hasn't seen any signal for those threads.
     Otherwise, the last signal reported to the core is found in the
     thread object's stop_signal.

     There's a corner case that isn't handled here at present.  Only
     if the thread stopped with a TARGET_WAITKIND_STOPPED does
     stop_signal make sense as a real signal to pass to the inferior.
     Some catchpoint related events, like
     TARGET_WAITKIND_(V)FORK|EXEC|SYSCALL, have their stop_signal set
     to TARGET_SIGNAL_SIGTRAP when the catchpoint triggers.  But,
     those traps are debug API (ptrace in our case) related and
     induced; the inferior wouldn't see them if it wasn't being
     traced.  Hence, we should never pass them to the inferior, even
     when set to pass state.  Since this corner case isn't handled by
     infrun.c when proceeding with a signal, for consistency, neither
     do we handle it here (or elsewhere in the file we check for
     signal pass state).  Normally SIGTRAP isn't set to pass state, so
     this is really a corner case.  */

  if (lp->waitstatus.kind != TARGET_WAITKIND_IGNORE)
    signo = TARGET_SIGNAL_0; /* a pending ptrace event, not a real signal.  */
  else if (lp->status)
    signo = target_signal_from_host (WSTOPSIG (lp->status));
  else if (non_stop && !is_executing (lp->ptid))
    {
      struct thread_info *tp = find_thread_ptid (lp->ptid);

      signo = tp->suspend.stop_signal;
    }
  else if (!non_stop)
    {
      struct target_waitstatus last;
      ptid_t last_ptid;

      get_last_target_status (&last_ptid, &last);

      if (GET_LWP (lp->ptid) == GET_LWP (last_ptid))
	{
	  struct thread_info *tp = find_thread_ptid (lp->ptid);

	  signo = tp->suspend.stop_signal;
	}
    }

  *status = 0;

  if (signo == TARGET_SIGNAL_0)
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "GPT: lwp %s has no pending signal\n",
			    target_pid_to_str (lp->ptid));
    }
  else if (!signal_pass_state (signo))
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "GPT: lwp %s had signal %s, "
			    "but it is in no pass state\n",
			    target_pid_to_str (lp->ptid),
			    target_signal_to_string (signo));
    }
  else
    {
      *status = W_STOPCODE (target_signal_to_host (signo));

      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "GPT: lwp %s has pending signal %s\n",
			    target_pid_to_str (lp->ptid),
			    target_signal_to_string (signo));
    }

  return 0;
}

static int
detach_callback (struct lwp_info *lp, void *data)
{
  gdb_assert (lp->status == 0 || WIFSTOPPED (lp->status));

  if (debug_linux_nat && lp->status)
    fprintf_unfiltered (gdb_stdlog, "DC:  Pending %s for %s on detach.\n",
			strsignal (WSTOPSIG (lp->status)),
			target_pid_to_str (lp->ptid));

  /* If there is a pending SIGSTOP, get rid of it.  */
  if (lp->signalled)
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "DC: Sending SIGCONT to %s\n",
			    target_pid_to_str (lp->ptid));

      kill_lwp (GET_LWP (lp->ptid), SIGCONT);
      lp->signalled = 0;
    }

  /* We don't actually detach from the LWP that has an id equal to the
     overall process id just yet.  */
  if (GET_LWP (lp->ptid) != GET_PID (lp->ptid))
    {
      int status = 0;

      /* Pass on any pending signal for this LWP.  */
      get_pending_status (lp, &status);

      errno = 0;
      if (ptrace (PTRACE_DETACH, GET_LWP (lp->ptid), 0,
		  WSTOPSIG (status)) < 0)
	error (_("Can't detach %s: %s"), target_pid_to_str (lp->ptid),
	       safe_strerror (errno));

      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "PTRACE_DETACH (%s, %s, 0) (OK)\n",
			    target_pid_to_str (lp->ptid),
			    strsignal (WSTOPSIG (status)));

      delete_lwp (lp->ptid);
    }

  return 0;
}

static void
linux_nat_detach (struct target_ops *ops, char *args, int from_tty)
{
  int pid;
  int status;
  struct lwp_info *main_lwp;

  pid = GET_PID (inferior_ptid);

  if (target_can_async_p ())
    linux_nat_async (NULL, 0);

  /* Stop all threads before detaching.  ptrace requires that the
     thread is stopped to sucessfully detach.  */
  iterate_over_lwps (pid_to_ptid (pid), stop_callback, NULL);
  /* ... and wait until all of them have reported back that
     they're no longer running.  */
  iterate_over_lwps (pid_to_ptid (pid), stop_wait_callback, NULL);

  iterate_over_lwps (pid_to_ptid (pid), detach_callback, NULL);

  /* Only the initial process should be left right now.  */
  gdb_assert (num_lwps (GET_PID (inferior_ptid)) == 1);

  main_lwp = find_lwp_pid (pid_to_ptid (pid));

  /* Pass on any pending signal for the last LWP.  */
  if ((args == NULL || *args == '\0')
      && get_pending_status (main_lwp, &status) != -1
      && WIFSTOPPED (status))
    {
      /* Put the signal number in ARGS so that inf_ptrace_detach will
	 pass it along with PTRACE_DETACH.  */
      args = alloca (8);
      sprintf (args, "%d", (int) WSTOPSIG (status));
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LND: Sending signal %s to %s\n",
			    args,
			    target_pid_to_str (main_lwp->ptid));
    }

  delete_lwp (main_lwp->ptid);

  if (forks_exist_p ())
    {
      /* Multi-fork case.  The current inferior_ptid is being detached
	 from, but there are other viable forks to debug.  Detach from
	 the current fork, and context-switch to the first
	 available.  */
      linux_fork_detach (args, from_tty);

      if (non_stop && target_can_async_p ())
 	target_async (inferior_event_handler, 0);
    }
  else
    linux_ops->to_detach (ops, args, from_tty);
}

/* Resume LP.  */

static int
resume_callback (struct lwp_info *lp, void *data)
{
  struct inferior *inf = find_inferior_pid (GET_PID (lp->ptid));

  if (lp->stopped && inf->vfork_child != NULL)
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "RC: Not resuming %s (vfork parent)\n",
			    target_pid_to_str (lp->ptid));
    }
  else if (lp->stopped && lp->status == 0)
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "RC:  PTRACE_CONT %s, 0, 0 (resuming sibling)\n",
			    target_pid_to_str (lp->ptid));

      linux_ops->to_resume (linux_ops,
			    pid_to_ptid (GET_LWP (lp->ptid)),
			    0, TARGET_SIGNAL_0);
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "RC:  PTRACE_CONT %s, 0, 0 (resume sibling)\n",
			    target_pid_to_str (lp->ptid));
      lp->stopped = 0;
      lp->step = 0;
      memset (&lp->siginfo, 0, sizeof (lp->siginfo));
      lp->stopped_by_watchpoint = 0;
    }
  else if (lp->stopped && debug_linux_nat)
    fprintf_unfiltered (gdb_stdlog,
			"RC: Not resuming sibling %s (has pending)\n",
			target_pid_to_str (lp->ptid));
  else if (debug_linux_nat)
    fprintf_unfiltered (gdb_stdlog,
			"RC: Not resuming sibling %s (not stopped)\n",
			target_pid_to_str (lp->ptid));

  return 0;
}

static int
resume_clear_callback (struct lwp_info *lp, void *data)
{
  lp->resumed = 0;
  return 0;
}

static int
resume_set_callback (struct lwp_info *lp, void *data)
{
  lp->resumed = 1;
  return 0;
}

static void
linux_nat_resume (struct target_ops *ops,
		  ptid_t ptid, int step, enum target_signal signo)
{
  sigset_t prev_mask;
  struct lwp_info *lp;
  int resume_many;

  if (debug_linux_nat)
    fprintf_unfiltered (gdb_stdlog,
			"LLR: Preparing to %s %s, %s, inferior_ptid %s\n",
			step ? "step" : "resume",
			target_pid_to_str (ptid),
			(signo != TARGET_SIGNAL_0
			 ? strsignal (target_signal_to_host (signo)) : "0"),
			target_pid_to_str (inferior_ptid));

  block_child_signals (&prev_mask);

  /* A specific PTID means `step only this process id'.  */
  resume_many = (ptid_equal (minus_one_ptid, ptid)
		 || ptid_is_pid (ptid));

  /* Mark the lwps we're resuming as resumed.  */
  iterate_over_lwps (ptid, resume_set_callback, NULL);

  /* See if it's the current inferior that should be handled
     specially.  */
  if (resume_many)
    lp = find_lwp_pid (inferior_ptid);
  else
    lp = find_lwp_pid (ptid);
  gdb_assert (lp != NULL);

  /* Remember if we're stepping.  */
  lp->step = step;

  /* If we have a pending wait status for this thread, there is no
     point in resuming the process.  But first make sure that
     linux_nat_wait won't preemptively handle the event - we
     should never take this short-circuit if we are going to
     leave LP running, since we have skipped resuming all the
     other threads.  This bit of code needs to be synchronized
     with linux_nat_wait.  */

  if (lp->status && WIFSTOPPED (lp->status))
    {
      enum target_signal saved_signo;
      struct inferior *inf;

      inf = find_inferior_pid (ptid_get_pid (lp->ptid));
      gdb_assert (inf);
      saved_signo = target_signal_from_host (WSTOPSIG (lp->status));

      /* Defer to common code if we're gaining control of the
	 inferior.  */
      if (inf->control.stop_soon == NO_STOP_QUIETLY
	  && signal_stop_state (saved_signo) == 0
	  && signal_print_state (saved_signo) == 0
	  && signal_pass_state (saved_signo) == 1)
	{
	  if (debug_linux_nat)
	    fprintf_unfiltered (gdb_stdlog,
				"LLR: Not short circuiting for ignored "
				"status 0x%x\n", lp->status);

	  /* FIXME: What should we do if we are supposed to continue
	     this thread with a signal?  */
	  gdb_assert (signo == TARGET_SIGNAL_0);
	  signo = saved_signo;
	  lp->status = 0;
	}
    }

  if (lp->status || lp->waitstatus.kind != TARGET_WAITKIND_IGNORE)
    {
      /* FIXME: What should we do if we are supposed to continue
	 this thread with a signal?  */
      gdb_assert (signo == TARGET_SIGNAL_0);

      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LLR: Short circuiting for status 0x%x\n",
			    lp->status);

      restore_child_signals_mask (&prev_mask);
      if (target_can_async_p ())
	{
	  target_async (inferior_event_handler, 0);
	  /* Tell the event loop we have something to process.  */
	  async_file_mark ();
	}
      return;
    }

  /* Mark LWP as not stopped to prevent it from being continued by
     resume_callback.  */
  lp->stopped = 0;

  if (resume_many)
    iterate_over_lwps (ptid, resume_callback, NULL);

  /* Convert to something the lower layer understands.  */
  ptid = pid_to_ptid (GET_LWP (lp->ptid));

  linux_ops->to_resume (linux_ops, ptid, step, signo);
  memset (&lp->siginfo, 0, sizeof (lp->siginfo));
  lp->stopped_by_watchpoint = 0;

  if (debug_linux_nat)
    fprintf_unfiltered (gdb_stdlog,
			"LLR: %s %s, %s (resume event thread)\n",
			step ? "PTRACE_SINGLESTEP" : "PTRACE_CONT",
			target_pid_to_str (ptid),
			(signo != TARGET_SIGNAL_0
			 ? strsignal (target_signal_to_host (signo)) : "0"));

  restore_child_signals_mask (&prev_mask);
  if (target_can_async_p ())
    target_async (inferior_event_handler, 0);
}

/* Send a signal to an LWP.  */

static int
kill_lwp (int lwpid, int signo)
{
  /* Use tkill, if possible, in case we are using nptl threads.  If tkill
     fails, then we are not using nptl threads and we should be using kill.  */

#ifdef HAVE_TKILL_SYSCALL
  {
    static int tkill_failed;

    if (!tkill_failed)
      {
	int ret;

	errno = 0;
	ret = syscall (__NR_tkill, lwpid, signo);
	if (errno != ENOSYS)
	  return ret;
	tkill_failed = 1;
      }
  }
#endif

  return kill (lwpid, signo);
}

/* Handle a GNU/Linux syscall trap wait response.  If we see a syscall
   event, check if the core is interested in it: if not, ignore the
   event, and keep waiting; otherwise, we need to toggle the LWP's
   syscall entry/exit status, since the ptrace event itself doesn't
   indicate it, and report the trap to higher layers.  */

static int
linux_handle_syscall_trap (struct lwp_info *lp, int stopping)
{
  struct target_waitstatus *ourstatus = &lp->waitstatus;
  struct gdbarch *gdbarch = target_thread_architecture (lp->ptid);
  int syscall_number = (int) gdbarch_get_syscall_number (gdbarch, lp->ptid);

  if (stopping)
    {
      /* If we're stopping threads, there's a SIGSTOP pending, which
	 makes it so that the LWP reports an immediate syscall return,
	 followed by the SIGSTOP.  Skip seeing that "return" using
	 PTRACE_CONT directly, and let stop_wait_callback collect the
	 SIGSTOP.  Later when the thread is resumed, a new syscall
	 entry event.  If we didn't do this (and returned 0), we'd
	 leave a syscall entry pending, and our caller, by using
	 PTRACE_CONT to collect the SIGSTOP, skips the syscall return
	 itself.  Later, when the user re-resumes this LWP, we'd see
	 another syscall entry event and we'd mistake it for a return.

	 If stop_wait_callback didn't force the SIGSTOP out of the LWP
	 (leaving immediately with LWP->signalled set, without issuing
	 a PTRACE_CONT), it would still be problematic to leave this
	 syscall enter pending, as later when the thread is resumed,
	 it would then see the same syscall exit mentioned above,
	 followed by the delayed SIGSTOP, while the syscall didn't
	 actually get to execute.  It seems it would be even more
	 confusing to the user.  */

      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LHST: ignoring syscall %d "
			    "for LWP %ld (stopping threads), "
			    "resuming with PTRACE_CONT for SIGSTOP\n",
			    syscall_number,
			    GET_LWP (lp->ptid));

      lp->syscall_state = TARGET_WAITKIND_IGNORE;
      ptrace (PTRACE_CONT, GET_LWP (lp->ptid), 0, 0);
      return 1;
    }

  if (catch_syscall_enabled ())
    {
      /* Always update the entry/return state, even if this particular
	 syscall isn't interesting to the core now.  In async mode,
	 the user could install a new catchpoint for this syscall
	 between syscall enter/return, and we'll need to know to
	 report a syscall return if that happens.  */
      lp->syscall_state = (lp->syscall_state == TARGET_WAITKIND_SYSCALL_ENTRY
			   ? TARGET_WAITKIND_SYSCALL_RETURN
			   : TARGET_WAITKIND_SYSCALL_ENTRY);

      if (catching_syscall_number (syscall_number))
	{
	  /* Alright, an event to report.  */
	  ourstatus->kind = lp->syscall_state;
	  ourstatus->value.syscall_number = syscall_number;

	  if (debug_linux_nat)
	    fprintf_unfiltered (gdb_stdlog,
				"LHST: stopping for %s of syscall %d"
				" for LWP %ld\n",
				lp->syscall_state
				== TARGET_WAITKIND_SYSCALL_ENTRY
				? "entry" : "return",
				syscall_number,
				GET_LWP (lp->ptid));
	  return 0;
	}

      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LHST: ignoring %s of syscall %d "
			    "for LWP %ld\n",
			    lp->syscall_state == TARGET_WAITKIND_SYSCALL_ENTRY
			    ? "entry" : "return",
			    syscall_number,
			    GET_LWP (lp->ptid));
    }
  else
    {
      /* If we had been syscall tracing, and hence used PT_SYSCALL
	 before on this LWP, it could happen that the user removes all
	 syscall catchpoints before we get to process this event.
	 There are two noteworthy issues here:

	 - When stopped at a syscall entry event, resuming with
	   PT_STEP still resumes executing the syscall and reports a
	   syscall return.

	 - Only PT_SYSCALL catches syscall enters.  If we last
	   single-stepped this thread, then this event can't be a
	   syscall enter.  If we last single-stepped this thread, this
	   has to be a syscall exit.

	 The points above mean that the next resume, be it PT_STEP or
	 PT_CONTINUE, can not trigger a syscall trace event.  */
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LHST: caught syscall event "
			    "with no syscall catchpoints."
			    " %d for LWP %ld, ignoring\n",
			    syscall_number,
			    GET_LWP (lp->ptid));
      lp->syscall_state = TARGET_WAITKIND_IGNORE;
    }

  /* The core isn't interested in this event.  For efficiency, avoid
     stopping all threads only to have the core resume them all again.
     Since we're not stopping threads, if we're still syscall tracing
     and not stepping, we can't use PTRACE_CONT here, as we'd miss any
     subsequent syscall.  Simply resume using the inf-ptrace layer,
     which knows when to use PT_SYSCALL or PT_CONTINUE.  */

  /* Note that gdbarch_get_syscall_number may access registers, hence
     fill a regcache.  */
  registers_changed ();
  linux_ops->to_resume (linux_ops, pid_to_ptid (GET_LWP (lp->ptid)),
			lp->step, TARGET_SIGNAL_0);
  return 1;
}

/* Handle a GNU/Linux extended wait response.  If we see a clone
   event, we need to add the new LWP to our list (and not report the
   trap to higher layers).  This function returns non-zero if the
   event should be ignored and we should wait again.  If STOPPING is
   true, the new LWP remains stopped, otherwise it is continued.  */

static int
linux_handle_extended_wait (struct lwp_info *lp, int status,
			    int stopping)
{
  int pid = GET_LWP (lp->ptid);
  struct target_waitstatus *ourstatus = &lp->waitstatus;
  int event = status >> 16;

  if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK
      || event == PTRACE_EVENT_CLONE)
    {
      unsigned long new_pid;
      int ret;

      ptrace (PTRACE_GETEVENTMSG, pid, 0, &new_pid);

      /* If we haven't already seen the new PID stop, wait for it now.  */
      if (! pull_pid_from_list (&stopped_pids, new_pid, &status))
	{
	  /* The new child has a pending SIGSTOP.  We can't affect it until it
	     hits the SIGSTOP, but we're already attached.  */
	  ret = my_waitpid (new_pid, &status,
			    (event == PTRACE_EVENT_CLONE) ? __WCLONE : 0);
	  if (ret == -1)
	    perror_with_name (_("waiting for new child"));
	  else if (ret != new_pid)
	    internal_error (__FILE__, __LINE__,
			    _("wait returned unexpected PID %d"), ret);
	  else if (!WIFSTOPPED (status))
	    internal_error (__FILE__, __LINE__,
			    _("wait returned unexpected status 0x%x"), status);
	}

      ourstatus->value.related_pid = ptid_build (new_pid, new_pid, 0);

      if (event == PTRACE_EVENT_FORK
	  && linux_fork_checkpointing_p (GET_PID (lp->ptid)))
	{
	  /* Handle checkpointing by linux-fork.c here as a special
	     case.  We don't want the follow-fork-mode or 'catch fork'
	     to interfere with this.  */

	  /* This won't actually modify the breakpoint list, but will
	     physically remove the breakpoints from the child.  */
	  detach_breakpoints (new_pid);

	  /* Retain child fork in ptrace (stopped) state.  */
	  if (!find_fork_pid (new_pid))
	    add_fork (new_pid);

	  /* Report as spurious, so that infrun doesn't want to follow
	     this fork.  We're actually doing an infcall in
	     linux-fork.c.  */
	  ourstatus->kind = TARGET_WAITKIND_SPURIOUS;
	  linux_enable_event_reporting (pid_to_ptid (new_pid));

	  /* Report the stop to the core.  */
	  return 0;
	}

      if (event == PTRACE_EVENT_FORK)
	ourstatus->kind = TARGET_WAITKIND_FORKED;
      else if (event == PTRACE_EVENT_VFORK)
	ourstatus->kind = TARGET_WAITKIND_VFORKED;
      else
	{
	  struct lwp_info *new_lp;

	  ourstatus->kind = TARGET_WAITKIND_IGNORE;

	  new_lp = add_lwp (BUILD_LWP (new_pid, GET_PID (lp->ptid)));
	  new_lp->cloned = 1;
	  new_lp->stopped = 1;

	  if (WSTOPSIG (status) != SIGSTOP)
	    {
	      /* This can happen if someone starts sending signals to
		 the new thread before it gets a chance to run, which
		 have a lower number than SIGSTOP (e.g. SIGUSR1).
		 This is an unlikely case, and harder to handle for
		 fork / vfork than for clone, so we do not try - but
		 we handle it for clone events here.  We'll send
		 the other signal on to the thread below.  */

	      new_lp->signalled = 1;
	    }
	  else
	    status = 0;

	  if (non_stop)
	    {
	      /* Add the new thread to GDB's lists as soon as possible
		 so that:

		 1) the frontend doesn't have to wait for a stop to
		 display them, and,

		 2) we tag it with the correct running state.  */

	      /* If the thread_db layer is active, let it know about
		 this new thread, and add it to GDB's list.  */
	      if (!thread_db_attach_lwp (new_lp->ptid))
		{
		  /* We're not using thread_db.  Add it to GDB's
		     list.  */
		  target_post_attach (GET_LWP (new_lp->ptid));
		  add_thread (new_lp->ptid);
		}

	      if (!stopping)
		{
		  set_running (new_lp->ptid, 1);
		  set_executing (new_lp->ptid, 1);
		}
	    }

	  /* Note the need to use the low target ops to resume, to
	     handle resuming with PT_SYSCALL if we have syscall
	     catchpoints.  */
	  if (!stopping)
	    {
	      enum target_signal signo;

	      new_lp->stopped = 0;
	      new_lp->resumed = 1;

	      signo = (status
		       ? target_signal_from_host (WSTOPSIG (status))
		       : TARGET_SIGNAL_0);

	      linux_ops->to_resume (linux_ops, pid_to_ptid (new_pid),
				    0, signo);
	    }
	  else
	    {
	      if (status != 0)
		{
		  /* We created NEW_LP so it cannot yet contain STATUS.  */
		  gdb_assert (new_lp->status == 0);

		  /* Save the wait status to report later.  */
		  if (debug_linux_nat)
		    fprintf_unfiltered (gdb_stdlog,
					"LHEW: waitpid of new LWP %ld, "
					"saving status %s\n",
					(long) GET_LWP (new_lp->ptid),
					status_to_str (status));
		  new_lp->status = status;
		}
	    }

	  if (debug_linux_nat)
	    fprintf_unfiltered (gdb_stdlog,
				"LHEW: Got clone event "
				"from LWP %ld, resuming\n",
				GET_LWP (lp->ptid));
	  linux_ops->to_resume (linux_ops, pid_to_ptid (GET_LWP (lp->ptid)),
				0, TARGET_SIGNAL_0);

	  return 1;
	}

      return 0;
    }

  if (event == PTRACE_EVENT_EXEC)
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LHEW: Got exec event from LWP %ld\n",
			    GET_LWP (lp->ptid));

      ourstatus->kind = TARGET_WAITKIND_EXECD;
      ourstatus->value.execd_pathname
	= xstrdup (linux_child_pid_to_exec_file (pid));

      return 0;
    }

  if (event == PTRACE_EVENT_VFORK_DONE)
    {
      if (current_inferior ()->waiting_for_vfork_done)
	{
	  if (debug_linux_nat)
	    fprintf_unfiltered (gdb_stdlog,
				"LHEW: Got expected PTRACE_EVENT_"
				"VFORK_DONE from LWP %ld: stopping\n",
				GET_LWP (lp->ptid));

	  ourstatus->kind = TARGET_WAITKIND_VFORK_DONE;
	  return 0;
	}

      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LHEW: Got PTRACE_EVENT_VFORK_DONE "
			    "from LWP %ld: resuming\n",
			    GET_LWP (lp->ptid));
      ptrace (PTRACE_CONT, GET_LWP (lp->ptid), 0, 0);
      return 1;
    }

  internal_error (__FILE__, __LINE__,
		  _("unknown ptrace event %d"), event);
}

/* Wait for LP to stop.  Returns the wait status, or 0 if the LWP has
   exited.  */

static int
wait_lwp (struct lwp_info *lp)
{
  pid_t pid;
  int status;
  int thread_dead = 0;

  gdb_assert (!lp->stopped);
  gdb_assert (lp->status == 0);

  pid = my_waitpid (GET_LWP (lp->ptid), &status, 0);
  if (pid == -1 && errno == ECHILD)
    {
      pid = my_waitpid (GET_LWP (lp->ptid), &status, __WCLONE);
      if (pid == -1 && errno == ECHILD)
	{
	  /* The thread has previously exited.  We need to delete it
	     now because, for some vendor 2.4 kernels with NPTL
	     support backported, there won't be an exit event unless
	     it is the main thread.  2.6 kernels will report an exit
	     event for each thread that exits, as expected.  */
	  thread_dead = 1;
	  if (debug_linux_nat)
	    fprintf_unfiltered (gdb_stdlog, "WL: %s vanished.\n",
				target_pid_to_str (lp->ptid));
	}
    }

  if (!thread_dead)
    {
      gdb_assert (pid == GET_LWP (lp->ptid));

      if (debug_linux_nat)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "WL: waitpid %s received %s\n",
			      target_pid_to_str (lp->ptid),
			      status_to_str (status));
	}
    }

  /* Check if the thread has exited.  */
  if (WIFEXITED (status) || WIFSIGNALED (status))
    {
      thread_dead = 1;
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog, "WL: %s exited.\n",
			    target_pid_to_str (lp->ptid));
    }

  if (thread_dead)
    {
      exit_lwp (lp);
      return 0;
    }

  gdb_assert (WIFSTOPPED (status));

  /* Handle GNU/Linux's syscall SIGTRAPs.  */
  if (WIFSTOPPED (status) && WSTOPSIG (status) == SYSCALL_SIGTRAP)
    {
      /* No longer need the sysgood bit.  The ptrace event ends up
	 recorded in lp->waitstatus if we care for it.  We can carry
	 on handling the event like a regular SIGTRAP from here
	 on.  */
      status = W_STOPCODE (SIGTRAP);
      if (linux_handle_syscall_trap (lp, 1))
	return wait_lwp (lp);
    }

  /* Handle GNU/Linux's extended waitstatus for trace events.  */
  if (WIFSTOPPED (status) && WSTOPSIG (status) == SIGTRAP && status >> 16 != 0)
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "WL: Handling extended status 0x%06x\n",
			    status);
      if (linux_handle_extended_wait (lp, status, 1))
	return wait_lwp (lp);
    }

  return status;
}

/* Save the most recent siginfo for LP.  This is currently only called
   for SIGTRAP; some ports use the si_addr field for
   target_stopped_data_address.  In the future, it may also be used to
   restore the siginfo of requeued signals.  */

static void
save_siginfo (struct lwp_info *lp)
{
  errno = 0;
  ptrace (PTRACE_GETSIGINFO, GET_LWP (lp->ptid),
	  (PTRACE_TYPE_ARG3) 0, &lp->siginfo);

  if (errno != 0)
    memset (&lp->siginfo, 0, sizeof (lp->siginfo));
}

/* Send a SIGSTOP to LP.  */

static int
stop_callback (struct lwp_info *lp, void *data)
{
  if (!lp->stopped && !lp->signalled)
    {
      int ret;

      if (debug_linux_nat)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "SC:  kill %s **<SIGSTOP>**\n",
			      target_pid_to_str (lp->ptid));
	}
      errno = 0;
      ret = kill_lwp (GET_LWP (lp->ptid), SIGSTOP);
      if (debug_linux_nat)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "SC:  lwp kill %d %s\n",
			      ret,
			      errno ? safe_strerror (errno) : "ERRNO-OK");
	}

      lp->signalled = 1;
      gdb_assert (lp->status == 0);
    }

  return 0;
}

/* Return non-zero if LWP PID has a pending SIGINT.  */

static int
linux_nat_has_pending_sigint (int pid)
{
  sigset_t pending, blocked, ignored;

  linux_proc_pending_signals (pid, &pending, &blocked, &ignored);

  if (sigismember (&pending, SIGINT)
      && !sigismember (&ignored, SIGINT))
    return 1;

  return 0;
}

/* Set a flag in LP indicating that we should ignore its next SIGINT.  */

static int
set_ignore_sigint (struct lwp_info *lp, void *data)
{
  /* If a thread has a pending SIGINT, consume it; otherwise, set a
     flag to consume the next one.  */
  if (lp->stopped && lp->status != 0 && WIFSTOPPED (lp->status)
      && WSTOPSIG (lp->status) == SIGINT)
    lp->status = 0;
  else
    lp->ignore_sigint = 1;

  return 0;
}

/* If LP does not have a SIGINT pending, then clear the ignore_sigint flag.
   This function is called after we know the LWP has stopped; if the LWP
   stopped before the expected SIGINT was delivered, then it will never have
   arrived.  Also, if the signal was delivered to a shared queue and consumed
   by a different thread, it will never be delivered to this LWP.  */

static void
maybe_clear_ignore_sigint (struct lwp_info *lp)
{
  if (!lp->ignore_sigint)
    return;

  if (!linux_nat_has_pending_sigint (GET_LWP (lp->ptid)))
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "MCIS: Clearing bogus flag for %s\n",
			    target_pid_to_str (lp->ptid));
      lp->ignore_sigint = 0;
    }
}

/* Fetch the possible triggered data watchpoint info and store it in
   LP.

   On some archs, like x86, that use debug registers to set
   watchpoints, it's possible that the way to know which watched
   address trapped, is to check the register that is used to select
   which address to watch.  Problem is, between setting the watchpoint
   and reading back which data address trapped, the user may change
   the set of watchpoints, and, as a consequence, GDB changes the
   debug registers in the inferior.  To avoid reading back a stale
   stopped-data-address when that happens, we cache in LP the fact
   that a watchpoint trapped, and the corresponding data address, as
   soon as we see LP stop with a SIGTRAP.  If GDB changes the debug
   registers meanwhile, we have the cached data we can rely on.  */

static void
save_sigtrap (struct lwp_info *lp)
{
  struct cleanup *old_chain;

  if (linux_ops->to_stopped_by_watchpoint == NULL)
    {
      lp->stopped_by_watchpoint = 0;
      return;
    }

  old_chain = save_inferior_ptid ();
  inferior_ptid = lp->ptid;

  lp->stopped_by_watchpoint = linux_ops->to_stopped_by_watchpoint ();

  if (lp->stopped_by_watchpoint)
    {
      if (linux_ops->to_stopped_data_address != NULL)
	lp->stopped_data_address_p =
	  linux_ops->to_stopped_data_address (&current_target,
					      &lp->stopped_data_address);
      else
	lp->stopped_data_address_p = 0;
    }

  do_cleanups (old_chain);
}

/* See save_sigtrap.  */

static int
linux_nat_stopped_by_watchpoint (void)
{
  struct lwp_info *lp = find_lwp_pid (inferior_ptid);

  gdb_assert (lp != NULL);

  return lp->stopped_by_watchpoint;
}

static int
linux_nat_stopped_data_address (struct target_ops *ops, CORE_ADDR *addr_p)
{
  struct lwp_info *lp = find_lwp_pid (inferior_ptid);

  gdb_assert (lp != NULL);

  *addr_p = lp->stopped_data_address;

  return lp->stopped_data_address_p;
}

/* Commonly any breakpoint / watchpoint generate only SIGTRAP.  */

static int
sigtrap_is_event (int status)
{
  return WIFSTOPPED (status) && WSTOPSIG (status) == SIGTRAP;
}

/* SIGTRAP-like events recognizer.  */

static int (*linux_nat_status_is_event) (int status) = sigtrap_is_event;

/* Check for SIGTRAP-like events in LP.  */

static int
linux_nat_lp_status_is_event (struct lwp_info *lp)
{
  /* We check for lp->waitstatus in addition to lp->status, because we can
     have pending process exits recorded in lp->status
     and W_EXITCODE(0,0) == 0.  We should probably have an additional
     lp->status_p flag.  */

  return (lp->waitstatus.kind == TARGET_WAITKIND_IGNORE
	  && linux_nat_status_is_event (lp->status));
}

/* Set alternative SIGTRAP-like events recognizer.  If
   breakpoint_inserted_here_p there then gdbarch_decr_pc_after_break will be
   applied.  */

void
linux_nat_set_status_is_event (struct target_ops *t,
			       int (*status_is_event) (int status))
{
  linux_nat_status_is_event = status_is_event;
}

/* Wait until LP is stopped.  */

static int
stop_wait_callback (struct lwp_info *lp, void *data)
{
  struct inferior *inf = find_inferior_pid (GET_PID (lp->ptid));

  /* If this is a vfork parent, bail out, it is not going to report
     any SIGSTOP until the vfork is done with.  */
  if (inf->vfork_child != NULL)
    return 0;

  if (!lp->stopped)
    {
      int status;

      status = wait_lwp (lp);
      if (status == 0)
	return 0;

      if (lp->ignore_sigint && WIFSTOPPED (status)
	  && WSTOPSIG (status) == SIGINT)
	{
	  lp->ignore_sigint = 0;

	  errno = 0;
	  ptrace (PTRACE_CONT, GET_LWP (lp->ptid), 0, 0);
	  if (debug_linux_nat)
	    fprintf_unfiltered (gdb_stdlog,
				"PTRACE_CONT %s, 0, 0 (%s) "
				"(discarding SIGINT)\n",
				target_pid_to_str (lp->ptid),
				errno ? safe_strerror (errno) : "OK");

	  return stop_wait_callback (lp, NULL);
	}

      maybe_clear_ignore_sigint (lp);

      if (WSTOPSIG (status) != SIGSTOP)
	{
	  if (linux_nat_status_is_event (status))
	    {
	      /* If a LWP other than the LWP that we're reporting an
	         event for has hit a GDB breakpoint (as opposed to
	         some random trap signal), then just arrange for it to
	         hit it again later.  We don't keep the SIGTRAP status
	         and don't forward the SIGTRAP signal to the LWP.  We
	         will handle the current event, eventually we will
	         resume all LWPs, and this one will get its breakpoint
	         trap again.

	         If we do not do this, then we run the risk that the
	         user will delete or disable the breakpoint, but the
	         thread will have already tripped on it.  */

	      /* Save the trap's siginfo in case we need it later.  */
	      save_siginfo (lp);

	      save_sigtrap (lp);

	      /* Now resume this LWP and get the SIGSTOP event.  */
	      errno = 0;
	      ptrace (PTRACE_CONT, GET_LWP (lp->ptid), 0, 0);
	      if (debug_linux_nat)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "PTRACE_CONT %s, 0, 0 (%s)\n",
				      target_pid_to_str (lp->ptid),
				      errno ? safe_strerror (errno) : "OK");

		  fprintf_unfiltered (gdb_stdlog,
				      "SWC: Candidate SIGTRAP event in %s\n",
				      target_pid_to_str (lp->ptid));
		}
	      /* Hold this event/waitstatus while we check to see if
		 there are any more (we still want to get that SIGSTOP).  */
	      stop_wait_callback (lp, NULL);

	      /* Hold the SIGTRAP for handling by linux_nat_wait.  If
		 there's another event, throw it back into the
		 queue.  */
	      if (lp->status)
		{
		  if (debug_linux_nat)
		    fprintf_unfiltered (gdb_stdlog,
					"SWC: kill %s, %s\n",
					target_pid_to_str (lp->ptid),
					status_to_str ((int) status));
		  kill_lwp (GET_LWP (lp->ptid), WSTOPSIG (lp->status));
		}

	      /* Save the sigtrap event.  */
	      lp->status = status;
	      return 0;
	    }
	  else
	    {
	      /* The thread was stopped with a signal other than
	         SIGSTOP, and didn't accidentally trip a breakpoint.  */

	      if (debug_linux_nat)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "SWC: Pending event %s in %s\n",
				      status_to_str ((int) status),
				      target_pid_to_str (lp->ptid));
		}
	      /* Now resume this LWP and get the SIGSTOP event.  */
	      errno = 0;
	      ptrace (PTRACE_CONT, GET_LWP (lp->ptid), 0, 0);
	      if (debug_linux_nat)
		fprintf_unfiltered (gdb_stdlog,
				    "SWC: PTRACE_CONT %s, 0, 0 (%s)\n",
				    target_pid_to_str (lp->ptid),
				    errno ? safe_strerror (errno) : "OK");

	      /* Hold this event/waitstatus while we check to see if
	         there are any more (we still want to get that SIGSTOP).  */
	      stop_wait_callback (lp, NULL);

	      /* If the lp->status field is still empty, use it to
		 hold this event.  If not, then this event must be
		 returned to the event queue of the LWP.  */
	      if (lp->status)
		{
		  if (debug_linux_nat)
		    {
		      fprintf_unfiltered (gdb_stdlog,
					  "SWC: kill %s, %s\n",
					  target_pid_to_str (lp->ptid),
					  status_to_str ((int) status));
		    }
		  kill_lwp (GET_LWP (lp->ptid), WSTOPSIG (status));
		}
	      else
		lp->status = status;
	      return 0;
	    }
	}
      else
	{
	  /* We caught the SIGSTOP that we intended to catch, so
	     there's no SIGSTOP pending.  */
	  lp->stopped = 1;
	  lp->signalled = 0;
	}
    }

  return 0;
}

/* Return non-zero if LP has a wait status pending.  */

static int
status_callback (struct lwp_info *lp, void *data)
{
  /* Only report a pending wait status if we pretend that this has
     indeed been resumed.  */
  if (!lp->resumed)
    return 0;

  if (lp->waitstatus.kind != TARGET_WAITKIND_IGNORE)
    {
      /* A ptrace event, like PTRACE_FORK|VFORK|EXEC, syscall event,
	 or a pending process exit.  Note that `W_EXITCODE(0,0) ==
	 0', so a clean process exit can not be stored pending in
	 lp->status, it is indistinguishable from
	 no-pending-status.  */
      return 1;
    }

  if (lp->status != 0)
    return 1;

  return 0;
}

/* Return non-zero if LP isn't stopped.  */

static int
running_callback (struct lwp_info *lp, void *data)
{
  return (lp->stopped == 0 || (lp->status != 0 && lp->resumed));
}

/* Count the LWP's that have had events.  */

static int
count_events_callback (struct lwp_info *lp, void *data)
{
  int *count = data;

  gdb_assert (count != NULL);

  /* Count only resumed LWPs that have a SIGTRAP event pending.  */
  if (lp->resumed && linux_nat_lp_status_is_event (lp))
    (*count)++;

  return 0;
}

/* Select the LWP (if any) that is currently being single-stepped.  */

static int
select_singlestep_lwp_callback (struct lwp_info *lp, void *data)
{
  if (lp->step && lp->status != 0)
    return 1;
  else
    return 0;
}

/* Select the Nth LWP that has had a SIGTRAP event.  */

static int
select_event_lwp_callback (struct lwp_info *lp, void *data)
{
  int *selector = data;

  gdb_assert (selector != NULL);

  /* Select only resumed LWPs that have a SIGTRAP event pending.  */
  if (lp->resumed && linux_nat_lp_status_is_event (lp))
    if ((*selector)-- == 0)
      return 1;

  return 0;
}

static int
cancel_breakpoint (struct lwp_info *lp)
{
  /* Arrange for a breakpoint to be hit again later.  We don't keep
     the SIGTRAP status and don't forward the SIGTRAP signal to the
     LWP.  We will handle the current event, eventually we will resume
     this LWP, and this breakpoint will trap again.

     If we do not do this, then we run the risk that the user will
     delete or disable the breakpoint, but the LWP will have already
     tripped on it.  */

  struct regcache *regcache = get_thread_regcache (lp->ptid);
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  CORE_ADDR pc;

  pc = regcache_read_pc (regcache) - gdbarch_decr_pc_after_break (gdbarch);
  if (breakpoint_inserted_here_p (get_regcache_aspace (regcache), pc))
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "CB: Push back breakpoint for %s\n",
			    target_pid_to_str (lp->ptid));

      /* Back up the PC if necessary.  */
      if (gdbarch_decr_pc_after_break (gdbarch))
	regcache_write_pc (regcache, pc);

      return 1;
    }
  return 0;
}

static int
cancel_breakpoints_callback (struct lwp_info *lp, void *data)
{
  struct lwp_info *event_lp = data;

  /* Leave the LWP that has been elected to receive a SIGTRAP alone.  */
  if (lp == event_lp)
    return 0;

  /* If a LWP other than the LWP that we're reporting an event for has
     hit a GDB breakpoint (as opposed to some random trap signal),
     then just arrange for it to hit it again later.  We don't keep
     the SIGTRAP status and don't forward the SIGTRAP signal to the
     LWP.  We will handle the current event, eventually we will resume
     all LWPs, and this one will get its breakpoint trap again.

     If we do not do this, then we run the risk that the user will
     delete or disable the breakpoint, but the LWP will have already
     tripped on it.  */

  if (linux_nat_lp_status_is_event (lp)
      && cancel_breakpoint (lp))
    /* Throw away the SIGTRAP.  */
    lp->status = 0;

  return 0;
}

/* Select one LWP out of those that have events pending.  */

static void
select_event_lwp (ptid_t filter, struct lwp_info **orig_lp, int *status)
{
  int num_events = 0;
  int random_selector;
  struct lwp_info *event_lp;

  /* Record the wait status for the original LWP.  */
  (*orig_lp)->status = *status;

  /* Give preference to any LWP that is being single-stepped.  */
  event_lp = iterate_over_lwps (filter,
				select_singlestep_lwp_callback, NULL);
  if (event_lp != NULL)
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "SEL: Select single-step %s\n",
			    target_pid_to_str (event_lp->ptid));
    }
  else
    {
      /* No single-stepping LWP.  Select one at random, out of those
         which have had SIGTRAP events.  */

      /* First see how many SIGTRAP events we have.  */
      iterate_over_lwps (filter, count_events_callback, &num_events);

      /* Now randomly pick a LWP out of those that have had a SIGTRAP.  */
      random_selector = (int)
	((num_events * (double) rand ()) / (RAND_MAX + 1.0));

      if (debug_linux_nat && num_events > 1)
	fprintf_unfiltered (gdb_stdlog,
			    "SEL: Found %d SIGTRAP events, selecting #%d\n",
			    num_events, random_selector);

      event_lp = iterate_over_lwps (filter,
				    select_event_lwp_callback,
				    &random_selector);
    }

  if (event_lp != NULL)
    {
      /* Switch the event LWP.  */
      *orig_lp = event_lp;
      *status = event_lp->status;
    }

  /* Flush the wait status for the event LWP.  */
  (*orig_lp)->status = 0;
}

/* Return non-zero if LP has been resumed.  */

static int
resumed_callback (struct lwp_info *lp, void *data)
{
  return lp->resumed;
}

/* Stop an active thread, verify it still exists, then resume it.  */

static int
stop_and_resume_callback (struct lwp_info *lp, void *data)
{
  struct lwp_info *ptr;

  if (!lp->stopped && !lp->signalled)
    {
      stop_callback (lp, NULL);
      stop_wait_callback (lp, NULL);
      /* Resume if the lwp still exists.  */
      for (ptr = lwp_list; ptr; ptr = ptr->next)
	if (lp == ptr)
	  {
	    resume_callback (lp, NULL);
	    resume_set_callback (lp, NULL);
	  }
    }
  return 0;
}

/* Check if we should go on and pass this event to common code.
   Return the affected lwp if we are, or NULL otherwise.  */
static struct lwp_info *
linux_nat_filter_event (int lwpid, int status, int options)
{
  struct lwp_info *lp;

  lp = find_lwp_pid (pid_to_ptid (lwpid));

  /* Check for stop events reported by a process we didn't already
     know about - anything not already in our LWP list.

     If we're expecting to receive stopped processes after
     fork, vfork, and clone events, then we'll just add the
     new one to our list and go back to waiting for the event
     to be reported - the stopped process might be returned
     from waitpid before or after the event is.  */
  if (WIFSTOPPED (status) && !lp)
    {
      linux_record_stopped_pid (lwpid, status);
      return NULL;
    }

  /* Make sure we don't report an event for the exit of an LWP not in
     our list, i.e. not part of the current process.  This can happen
     if we detach from a program we originally forked and then it
     exits.  */
  if (!WIFSTOPPED (status) && !lp)
    return NULL;

  /* NOTE drow/2003-06-17: This code seems to be meant for debugging
     CLONE_PTRACE processes which do not use the thread library -
     otherwise we wouldn't find the new LWP this way.  That doesn't
     currently work, and the following code is currently unreachable
     due to the two blocks above.  If it's fixed some day, this code
     should be broken out into a function so that we can also pick up
     LWPs from the new interface.  */
  if (!lp)
    {
      lp = add_lwp (BUILD_LWP (lwpid, GET_PID (inferior_ptid)));
      if (options & __WCLONE)
	lp->cloned = 1;

      gdb_assert (WIFSTOPPED (status)
		  && WSTOPSIG (status) == SIGSTOP);
      lp->signalled = 1;

      if (!in_thread_list (inferior_ptid))
	{
	  inferior_ptid = BUILD_LWP (GET_PID (inferior_ptid),
				     GET_PID (inferior_ptid));
	  add_thread (inferior_ptid);
	}

      add_thread (lp->ptid);
    }

  /* Handle GNU/Linux's syscall SIGTRAPs.  */
  if (WIFSTOPPED (status) && WSTOPSIG (status) == SYSCALL_SIGTRAP)
    {
      /* No longer need the sysgood bit.  The ptrace event ends up
	 recorded in lp->waitstatus if we care for it.  We can carry
	 on handling the event like a regular SIGTRAP from here
	 on.  */
      status = W_STOPCODE (SIGTRAP);
      if (linux_handle_syscall_trap (lp, 0))
	return NULL;
    }

  /* Handle GNU/Linux's extended waitstatus for trace events.  */
  if (WIFSTOPPED (status) && WSTOPSIG (status) == SIGTRAP && status >> 16 != 0)
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LLW: Handling extended status 0x%06x\n",
			    status);
      if (linux_handle_extended_wait (lp, status, 0))
	return NULL;
    }

  if (linux_nat_status_is_event (status))
    {
      /* Save the trap's siginfo in case we need it later.  */
      save_siginfo (lp);

      save_sigtrap (lp);
    }

  /* Check if the thread has exited.  */
  if ((WIFEXITED (status) || WIFSIGNALED (status))
      && num_lwps (GET_PID (lp->ptid)) > 1)
    {
      /* If this is the main thread, we must stop all threads and verify
	 if they are still alive.  This is because in the nptl thread model
	 on Linux 2.4, there is no signal issued for exiting LWPs
	 other than the main thread.  We only get the main thread exit
	 signal once all child threads have already exited.  If we
	 stop all the threads and use the stop_wait_callback to check
	 if they have exited we can determine whether this signal
	 should be ignored or whether it means the end of the debugged
	 application, regardless of which threading model is being
	 used.  */
      if (GET_PID (lp->ptid) == GET_LWP (lp->ptid))
	{
	  lp->stopped = 1;
	  iterate_over_lwps (pid_to_ptid (GET_PID (lp->ptid)),
			     stop_and_resume_callback, NULL);
	}

      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LLW: %s exited.\n",
			    target_pid_to_str (lp->ptid));

      if (num_lwps (GET_PID (lp->ptid)) > 1)
       {
	 /* If there is at least one more LWP, then the exit signal
	    was not the end of the debugged application and should be
	    ignored.  */
	 exit_lwp (lp);
	 return NULL;
       }
    }

  /* Check if the current LWP has previously exited.  In the nptl
     thread model, LWPs other than the main thread do not issue
     signals when they exit so we must check whenever the thread has
     stopped.  A similar check is made in stop_wait_callback().  */
  if (num_lwps (GET_PID (lp->ptid)) > 1 && !linux_thread_alive (lp->ptid))
    {
      ptid_t ptid = pid_to_ptid (GET_PID (lp->ptid));

      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LLW: %s exited.\n",
			    target_pid_to_str (lp->ptid));

      exit_lwp (lp);

      /* Make sure there is at least one thread running.  */
      gdb_assert (iterate_over_lwps (ptid, running_callback, NULL));

      /* Discard the event.  */
      return NULL;
    }

  /* Make sure we don't report a SIGSTOP that we sent ourselves in
     an attempt to stop an LWP.  */
  if (lp->signalled
      && WIFSTOPPED (status) && WSTOPSIG (status) == SIGSTOP)
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LLW: Delayed SIGSTOP caught for %s.\n",
			    target_pid_to_str (lp->ptid));

      /* This is a delayed SIGSTOP.  */
      lp->signalled = 0;

      registers_changed ();

      linux_ops->to_resume (linux_ops, pid_to_ptid (GET_LWP (lp->ptid)),
			    lp->step, TARGET_SIGNAL_0);
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LLW: %s %s, 0, 0 (discard SIGSTOP)\n",
			    lp->step ?
			    "PTRACE_SINGLESTEP" : "PTRACE_CONT",
			    target_pid_to_str (lp->ptid));

      lp->stopped = 0;
      gdb_assert (lp->resumed);

      /* Discard the event.  */
      return NULL;
    }

  /* Make sure we don't report a SIGINT that we have already displayed
     for another thread.  */
  if (lp->ignore_sigint
      && WIFSTOPPED (status) && WSTOPSIG (status) == SIGINT)
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LLW: Delayed SIGINT caught for %s.\n",
			    target_pid_to_str (lp->ptid));

      /* This is a delayed SIGINT.  */
      lp->ignore_sigint = 0;

      registers_changed ();
      linux_ops->to_resume (linux_ops, pid_to_ptid (GET_LWP (lp->ptid)),
			    lp->step, TARGET_SIGNAL_0);
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LLW: %s %s, 0, 0 (discard SIGINT)\n",
			    lp->step ?
			    "PTRACE_SINGLESTEP" : "PTRACE_CONT",
			    target_pid_to_str (lp->ptid));

      lp->stopped = 0;
      gdb_assert (lp->resumed);

      /* Discard the event.  */
      return NULL;
    }

  /* An interesting event.  */
  gdb_assert (lp);
  lp->status = status;
  return lp;
}

static ptid_t
linux_nat_wait_1 (struct target_ops *ops,
		  ptid_t ptid, struct target_waitstatus *ourstatus,
		  int target_options)
{
  static sigset_t prev_mask;
  struct lwp_info *lp = NULL;
  int options = 0;
  int status = 0;
  pid_t pid;

  if (debug_linux_nat_async)
    fprintf_unfiltered (gdb_stdlog, "LLW: enter\n");

  /* The first time we get here after starting a new inferior, we may
     not have added it to the LWP list yet - this is the earliest
     moment at which we know its PID.  */
  if (ptid_is_pid (inferior_ptid))
    {
      /* Upgrade the main thread's ptid.  */
      thread_change_ptid (inferior_ptid,
			  BUILD_LWP (GET_PID (inferior_ptid),
				     GET_PID (inferior_ptid)));

      lp = add_lwp (inferior_ptid);
      lp->resumed = 1;
    }

  /* Make sure SIGCHLD is blocked.  */
  block_child_signals (&prev_mask);

  if (ptid_equal (ptid, minus_one_ptid))
    pid = -1;
  else if (ptid_is_pid (ptid))
    /* A request to wait for a specific tgid.  This is not possible
       with waitpid, so instead, we wait for any child, and leave
       children we're not interested in right now with a pending
       status to report later.  */
    pid = -1;
  else
    pid = GET_LWP (ptid);

retry:
  lp = NULL;
  status = 0;

  /* Make sure that of those LWPs we want to get an event from, there
     is at least one LWP that has been resumed.  If there's none, just
     bail out.  The core may just be flushing asynchronously all
     events.  */
  if (iterate_over_lwps (ptid, resumed_callback, NULL) == NULL)
    {
      ourstatus->kind = TARGET_WAITKIND_IGNORE;

      if (debug_linux_nat_async)
	fprintf_unfiltered (gdb_stdlog, "LLW: exit (no resumed LWP)\n");

      restore_child_signals_mask (&prev_mask);
      return minus_one_ptid;
    }

  /* First check if there is a LWP with a wait status pending.  */
  if (pid == -1)
    {
      /* Any LWP that's been resumed will do.  */
      lp = iterate_over_lwps (ptid, status_callback, NULL);
      if (lp)
	{
	  if (debug_linux_nat && lp->status)
	    fprintf_unfiltered (gdb_stdlog,
				"LLW: Using pending wait status %s for %s.\n",
				status_to_str (lp->status),
				target_pid_to_str (lp->ptid));
	}

      /* But if we don't find one, we'll have to wait, and check both
	 cloned and uncloned processes.  We start with the cloned
	 processes.  */
      options = __WCLONE | WNOHANG;
    }
  else if (is_lwp (ptid))
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LLW: Waiting for specific LWP %s.\n",
			    target_pid_to_str (ptid));

      /* We have a specific LWP to check.  */
      lp = find_lwp_pid (ptid);
      gdb_assert (lp);

      if (debug_linux_nat && lp->status)
	fprintf_unfiltered (gdb_stdlog,
			    "LLW: Using pending wait status %s for %s.\n",
			    status_to_str (lp->status),
			    target_pid_to_str (lp->ptid));

      /* If we have to wait, take into account whether PID is a cloned
         process or not.  And we have to convert it to something that
         the layer beneath us can understand.  */
      options = lp->cloned ? __WCLONE : 0;
      pid = GET_LWP (ptid);

      /* We check for lp->waitstatus in addition to lp->status,
	 because we can have pending process exits recorded in
	 lp->status and W_EXITCODE(0,0) == 0.  We should probably have
	 an additional lp->status_p flag.  */
      if (lp->status == 0 && lp->waitstatus.kind == TARGET_WAITKIND_IGNORE)
	lp = NULL;
    }

  if (lp && lp->signalled)
    {
      /* A pending SIGSTOP may interfere with the normal stream of
         events.  In a typical case where interference is a problem,
         we have a SIGSTOP signal pending for LWP A while
         single-stepping it, encounter an event in LWP B, and take the
         pending SIGSTOP while trying to stop LWP A.  After processing
         the event in LWP B, LWP A is continued, and we'll never see
         the SIGTRAP associated with the last time we were
         single-stepping LWP A.  */

      /* Resume the thread.  It should halt immediately returning the
         pending SIGSTOP.  */
      registers_changed ();
      linux_ops->to_resume (linux_ops, pid_to_ptid (GET_LWP (lp->ptid)),
			    lp->step, TARGET_SIGNAL_0);
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LLW: %s %s, 0, 0 (expect SIGSTOP)\n",
			    lp->step ? "PTRACE_SINGLESTEP" : "PTRACE_CONT",
			    target_pid_to_str (lp->ptid));
      lp->stopped = 0;
      gdb_assert (lp->resumed);

      /* Catch the pending SIGSTOP.  */
      status = lp->status;
      lp->status = 0;

      stop_wait_callback (lp, NULL);

      /* If the lp->status field isn't empty, we caught another signal
	 while flushing the SIGSTOP.  Return it back to the event
	 queue of the LWP, as we already have an event to handle.  */
      if (lp->status)
	{
	  if (debug_linux_nat)
	    fprintf_unfiltered (gdb_stdlog,
				"LLW: kill %s, %s\n",
				target_pid_to_str (lp->ptid),
				status_to_str (lp->status));
	  kill_lwp (GET_LWP (lp->ptid), WSTOPSIG (lp->status));
	}

      lp->status = status;
    }

  if (!target_can_async_p ())
    {
      /* Causes SIGINT to be passed on to the attached process.  */
      set_sigint_trap ();
    }

  /* Translate generic target_wait options into waitpid options.  */
  if (target_options & TARGET_WNOHANG)
    options |= WNOHANG;

  while (lp == NULL)
    {
      pid_t lwpid;

      lwpid = my_waitpid (pid, &status, options);

      if (lwpid > 0)
	{
	  gdb_assert (pid == -1 || lwpid == pid);

	  if (debug_linux_nat)
	    {
	      fprintf_unfiltered (gdb_stdlog,
				  "LLW: waitpid %ld received %s\n",
				  (long) lwpid, status_to_str (status));
	    }

	  lp = linux_nat_filter_event (lwpid, status, options);

	  /* STATUS is now no longer valid, use LP->STATUS instead.  */
	  status = 0;

	  if (lp
	      && ptid_is_pid (ptid)
	      && ptid_get_pid (lp->ptid) != ptid_get_pid (ptid))
	    {
	      gdb_assert (lp->resumed);

	      if (debug_linux_nat)
		fprintf (stderr,
			 "LWP %ld got an event %06x, leaving pending.\n",
			 ptid_get_lwp (lp->ptid), lp->status);

	      if (WIFSTOPPED (lp->status))
		{
		  if (WSTOPSIG (lp->status) != SIGSTOP)
		    {
		      /* Cancel breakpoint hits.  The breakpoint may
			 be removed before we fetch events from this
			 process to report to the core.  It is best
			 not to assume the moribund breakpoints
			 heuristic always handles these cases --- it
			 could be too many events go through to the
			 core before this one is handled.  All-stop
			 always cancels breakpoint hits in all
			 threads.  */
		      if (non_stop
			  && linux_nat_lp_status_is_event (lp)
			  && cancel_breakpoint (lp))
			{
			  /* Throw away the SIGTRAP.  */
			  lp->status = 0;

			  if (debug_linux_nat)
			    fprintf (stderr,
				     "LLW: LWP %ld hit a breakpoint while"
				     " waiting for another process;"
				     " cancelled it\n",
				     ptid_get_lwp (lp->ptid));
			}
		      lp->stopped = 1;
		    }
		  else
		    {
		      lp->stopped = 1;
		      lp->signalled = 0;
		    }
		}
	      else if (WIFEXITED (lp->status) || WIFSIGNALED (lp->status))
		{
		  if (debug_linux_nat)
		    fprintf (stderr,
			     "Process %ld exited while stopping LWPs\n",
			     ptid_get_lwp (lp->ptid));

		  /* This was the last lwp in the process.  Since
		     events are serialized to GDB core, and we can't
		     report this one right now, but GDB core and the
		     other target layers will want to be notified
		     about the exit code/signal, leave the status
		     pending for the next time we're able to report
		     it.  */

		  /* Prevent trying to stop this thread again.  We'll
		     never try to resume it because it has a pending
		     status.  */
		  lp->stopped = 1;

		  /* Dead LWP's aren't expected to reported a pending
		     sigstop.  */
		  lp->signalled = 0;

		  /* Store the pending event in the waitstatus as
		     well, because W_EXITCODE(0,0) == 0.  */
		  store_waitstatus (&lp->waitstatus, lp->status);
		}

	      /* Keep looking.  */
	      lp = NULL;
	      continue;
	    }

	  if (lp)
	    break;
	  else
	    {
	      if (pid == -1)
		{
		  /* waitpid did return something.  Restart over.  */
		  options |= __WCLONE;
		}
	      continue;
	    }
	}

      if (pid == -1)
	{
	  /* Alternate between checking cloned and uncloned processes.  */
	  options ^= __WCLONE;

	  /* And every time we have checked both:
	     In async mode, return to event loop;
	     In sync mode, suspend waiting for a SIGCHLD signal.  */
	  if (options & __WCLONE)
	    {
	      if (target_options & TARGET_WNOHANG)
		{
		  /* No interesting event.  */
		  ourstatus->kind = TARGET_WAITKIND_IGNORE;

		  if (debug_linux_nat_async)
		    fprintf_unfiltered (gdb_stdlog, "LLW: exit (ignore)\n");

		  restore_child_signals_mask (&prev_mask);
		  return minus_one_ptid;
		}

	      sigsuspend (&suspend_mask);
	    }
	}
      else if (target_options & TARGET_WNOHANG)
	{
	  /* No interesting event for PID yet.  */
	  ourstatus->kind = TARGET_WAITKIND_IGNORE;

	  if (debug_linux_nat_async)
	    fprintf_unfiltered (gdb_stdlog, "LLW: exit (ignore)\n");

	  restore_child_signals_mask (&prev_mask);
	  return minus_one_ptid;
	}

      /* We shouldn't end up here unless we want to try again.  */
      gdb_assert (lp == NULL);
    }

  if (!target_can_async_p ())
    clear_sigint_trap ();

  gdb_assert (lp);

  status = lp->status;
  lp->status = 0;

  /* Don't report signals that GDB isn't interested in, such as
     signals that are neither printed nor stopped upon.  Stopping all
     threads can be a bit time-consuming so if we want decent
     performance with heavily multi-threaded programs, especially when
     they're using a high frequency timer, we'd better avoid it if we
     can.  */

  if (WIFSTOPPED (status))
    {
      enum target_signal signo = target_signal_from_host (WSTOPSIG (status));
      struct inferior *inf;

      inf = find_inferior_pid (ptid_get_pid (lp->ptid));
      gdb_assert (inf);

      /* Defer to common code if we get a signal while
	 single-stepping, since that may need special care, e.g. to
	 skip the signal handler, or, if we're gaining control of the
	 inferior.  */
      if (!lp->step
	  && inf->control.stop_soon == NO_STOP_QUIETLY
	  && signal_stop_state (signo) == 0
	  && signal_print_state (signo) == 0
	  && signal_pass_state (signo) == 1)
	{
	  /* FIMXE: kettenis/2001-06-06: Should we resume all threads
	     here?  It is not clear we should.  GDB may not expect
	     other threads to run.  On the other hand, not resuming
	     newly attached threads may cause an unwanted delay in
	     getting them running.  */
	  registers_changed ();
	  linux_ops->to_resume (linux_ops, pid_to_ptid (GET_LWP (lp->ptid)),
				lp->step, signo);
	  if (debug_linux_nat)
	    fprintf_unfiltered (gdb_stdlog,
				"LLW: %s %s, %s (preempt 'handle')\n",
				lp->step ?
				"PTRACE_SINGLESTEP" : "PTRACE_CONT",
				target_pid_to_str (lp->ptid),
				(signo != TARGET_SIGNAL_0
				 ? strsignal (target_signal_to_host (signo))
				 : "0"));
	  lp->stopped = 0;
	  goto retry;
	}

      if (!non_stop)
	{
	  /* Only do the below in all-stop, as we currently use SIGINT
	     to implement target_stop (see linux_nat_stop) in
	     non-stop.  */
	  if (signo == TARGET_SIGNAL_INT && signal_pass_state (signo) == 0)
	    {
	      /* If ^C/BREAK is typed at the tty/console, SIGINT gets
		 forwarded to the entire process group, that is, all LWPs
		 will receive it - unless they're using CLONE_THREAD to
		 share signals.  Since we only want to report it once, we
		 mark it as ignored for all LWPs except this one.  */
	      iterate_over_lwps (pid_to_ptid (ptid_get_pid (ptid)),
					      set_ignore_sigint, NULL);
	      lp->ignore_sigint = 0;
	    }
	  else
	    maybe_clear_ignore_sigint (lp);
	}
    }

  /* This LWP is stopped now.  */
  lp->stopped = 1;

  if (debug_linux_nat)
    fprintf_unfiltered (gdb_stdlog, "LLW: Candidate event %s in %s.\n",
			status_to_str (status), target_pid_to_str (lp->ptid));

  if (!non_stop)
    {
      /* Now stop all other LWP's ...  */
      iterate_over_lwps (minus_one_ptid, stop_callback, NULL);

      /* ... and wait until all of them have reported back that
	 they're no longer running.  */
      iterate_over_lwps (minus_one_ptid, stop_wait_callback, NULL);

      /* If we're not waiting for a specific LWP, choose an event LWP
	 from among those that have had events.  Giving equal priority
	 to all LWPs that have had events helps prevent
	 starvation.  */
      if (pid == -1)
	select_event_lwp (ptid, &lp, &status);

      /* Now that we've selected our final event LWP, cancel any
	 breakpoints in other LWPs that have hit a GDB breakpoint.
	 See the comment in cancel_breakpoints_callback to find out
	 why.  */
      iterate_over_lwps (minus_one_ptid, cancel_breakpoints_callback, lp);

      /* In all-stop, from the core's perspective, all LWPs are now
	 stopped until a new resume action is sent over.  */
      iterate_over_lwps (minus_one_ptid, resume_clear_callback, NULL);
    }
  else
    lp->resumed = 0;

  if (linux_nat_status_is_event (status))
    {
      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LLW: trap ptid is %s.\n",
			    target_pid_to_str (lp->ptid));
    }

  if (lp->waitstatus.kind != TARGET_WAITKIND_IGNORE)
    {
      *ourstatus = lp->waitstatus;
      lp->waitstatus.kind = TARGET_WAITKIND_IGNORE;
    }
  else
    store_waitstatus (ourstatus, status);

  if (debug_linux_nat_async)
    fprintf_unfiltered (gdb_stdlog, "LLW: exit\n");

  restore_child_signals_mask (&prev_mask);

  if (ourstatus->kind == TARGET_WAITKIND_EXITED
      || ourstatus->kind == TARGET_WAITKIND_SIGNALLED)
    lp->core = -1;
  else
    lp->core = linux_nat_core_of_thread_1 (lp->ptid);

  return lp->ptid;
}

/* Resume LWPs that are currently stopped without any pending status
   to report, but are resumed from the core's perspective.  */

static int
resume_stopped_resumed_lwps (struct lwp_info *lp, void *data)
{
  ptid_t *wait_ptid_p = data;

  if (lp->stopped
      && lp->resumed
      && lp->status == 0
      && lp->waitstatus.kind == TARGET_WAITKIND_IGNORE)
    {
      gdb_assert (is_executing (lp->ptid));

      /* Don't bother if there's a breakpoint at PC that we'd hit
	 immediately, and we're not waiting for this LWP.  */
      if (!ptid_match (lp->ptid, *wait_ptid_p))
	{
	  struct regcache *regcache = get_thread_regcache (lp->ptid);
	  CORE_ADDR pc = regcache_read_pc (regcache);

	  if (breakpoint_inserted_here_p (get_regcache_aspace (regcache), pc))
	    return 0;
	}

      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "RSRL: resuming stopped-resumed LWP %s\n",
			    target_pid_to_str (lp->ptid));

      linux_ops->to_resume (linux_ops, pid_to_ptid (GET_LWP (lp->ptid)),
			    lp->step, TARGET_SIGNAL_0);
      lp->stopped = 0;
      memset (&lp->siginfo, 0, sizeof (lp->siginfo));
      lp->stopped_by_watchpoint = 0;
    }

  return 0;
}

static ptid_t
linux_nat_wait (struct target_ops *ops,
		ptid_t ptid, struct target_waitstatus *ourstatus,
		int target_options)
{
  ptid_t event_ptid;

  if (debug_linux_nat)
    fprintf_unfiltered (gdb_stdlog,
			"linux_nat_wait: [%s]\n", target_pid_to_str (ptid));

  /* Flush the async file first.  */
  if (target_can_async_p ())
    async_file_flush ();

  /* Resume LWPs that are currently stopped without any pending status
     to report, but are resumed from the core's perspective.  LWPs get
     in this state if we find them stopping at a time we're not
     interested in reporting the event (target_wait on a
     specific_process, for example, see linux_nat_wait_1), and
     meanwhile the event became uninteresting.  Don't bother resuming
     LWPs we're not going to wait for if they'd stop immediately.  */
  if (non_stop)
    iterate_over_lwps (minus_one_ptid, resume_stopped_resumed_lwps, &ptid);

  event_ptid = linux_nat_wait_1 (ops, ptid, ourstatus, target_options);

  /* If we requested any event, and something came out, assume there
     may be more.  If we requested a specific lwp or process, also
     assume there may be more.  */
  if (target_can_async_p ()
      && (ourstatus->kind != TARGET_WAITKIND_IGNORE
	  || !ptid_equal (ptid, minus_one_ptid)))
    async_file_mark ();

  /* Get ready for the next event.  */
  if (target_can_async_p ())
    target_async (inferior_event_handler, 0);

  return event_ptid;
}

static int
kill_callback (struct lwp_info *lp, void *data)
{
  errno = 0;
  ptrace (PTRACE_KILL, GET_LWP (lp->ptid), 0, 0);
  if (debug_linux_nat)
    fprintf_unfiltered (gdb_stdlog,
			"KC:  PTRACE_KILL %s, 0, 0 (%s)\n",
			target_pid_to_str (lp->ptid),
			errno ? safe_strerror (errno) : "OK");

  return 0;
}

static int
kill_wait_callback (struct lwp_info *lp, void *data)
{
  pid_t pid;

  /* We must make sure that there are no pending events (delayed
     SIGSTOPs, pending SIGTRAPs, etc.) to make sure the current
     program doesn't interfere with any following debugging session.  */

  /* For cloned processes we must check both with __WCLONE and
     without, since the exit status of a cloned process isn't reported
     with __WCLONE.  */
  if (lp->cloned)
    {
      do
	{
	  pid = my_waitpid (GET_LWP (lp->ptid), NULL, __WCLONE);
	  if (pid != (pid_t) -1)
	    {
	      if (debug_linux_nat)
		fprintf_unfiltered (gdb_stdlog,
				    "KWC: wait %s received unknown.\n",
				    target_pid_to_str (lp->ptid));
	      /* The Linux kernel sometimes fails to kill a thread
		 completely after PTRACE_KILL; that goes from the stop
		 point in do_fork out to the one in
		 get_signal_to_deliever and waits again.  So kill it
		 again.  */
	      kill_callback (lp, NULL);
	    }
	}
      while (pid == GET_LWP (lp->ptid));

      gdb_assert (pid == -1 && errno == ECHILD);
    }

  do
    {
      pid = my_waitpid (GET_LWP (lp->ptid), NULL, 0);
      if (pid != (pid_t) -1)
	{
	  if (debug_linux_nat)
	    fprintf_unfiltered (gdb_stdlog,
				"KWC: wait %s received unk.\n",
				target_pid_to_str (lp->ptid));
	  /* See the call to kill_callback above.  */
	  kill_callback (lp, NULL);
	}
    }
  while (pid == GET_LWP (lp->ptid));

  gdb_assert (pid == -1 && errno == ECHILD);
  return 0;
}

static void
linux_nat_kill (struct target_ops *ops)
{
  struct target_waitstatus last;
  ptid_t last_ptid;
  int status;

  /* If we're stopped while forking and we haven't followed yet,
     kill the other task.  We need to do this first because the
     parent will be sleeping if this is a vfork.  */

  get_last_target_status (&last_ptid, &last);

  if (last.kind == TARGET_WAITKIND_FORKED
      || last.kind == TARGET_WAITKIND_VFORKED)
    {
      ptrace (PT_KILL, PIDGET (last.value.related_pid), 0, 0);
      wait (&status);
    }

  if (forks_exist_p ())
    linux_fork_killall ();
  else
    {
      ptid_t ptid = pid_to_ptid (ptid_get_pid (inferior_ptid));

      /* Stop all threads before killing them, since ptrace requires
	 that the thread is stopped to sucessfully PTRACE_KILL.  */
      iterate_over_lwps (ptid, stop_callback, NULL);
      /* ... and wait until all of them have reported back that
	 they're no longer running.  */
      iterate_over_lwps (ptid, stop_wait_callback, NULL);

      /* Kill all LWP's ...  */
      iterate_over_lwps (ptid, kill_callback, NULL);

      /* ... and wait until we've flushed all events.  */
      iterate_over_lwps (ptid, kill_wait_callback, NULL);
    }

  target_mourn_inferior ();
}

static void
linux_nat_mourn_inferior (struct target_ops *ops)
{
  purge_lwp_list (ptid_get_pid (inferior_ptid));

  if (! forks_exist_p ())
    /* Normal case, no other forks available.  */
    linux_ops->to_mourn_inferior (ops);
  else
    /* Multi-fork case.  The current inferior_ptid has exited, but
       there are other viable forks to debug.  Delete the exiting
       one and context-switch to the first available.  */
    linux_fork_mourn_inferior ();
}

/* Convert a native/host siginfo object, into/from the siginfo in the
   layout of the inferiors' architecture.  */

static void
siginfo_fixup (struct siginfo *siginfo, gdb_byte *inf_siginfo, int direction)
{
  int done = 0;

  if (linux_nat_siginfo_fixup != NULL)
    done = linux_nat_siginfo_fixup (siginfo, inf_siginfo, direction);

  /* If there was no callback, or the callback didn't do anything,
     then just do a straight memcpy.  */
  if (!done)
    {
      if (direction == 1)
	memcpy (siginfo, inf_siginfo, sizeof (struct siginfo));
      else
	memcpy (inf_siginfo, siginfo, sizeof (struct siginfo));
    }
}

static LONGEST
linux_xfer_siginfo (struct target_ops *ops, enum target_object object,
                    const char *annex, gdb_byte *readbuf,
		    const gdb_byte *writebuf, ULONGEST offset, LONGEST len)
{
  int pid;
  struct siginfo siginfo;
  gdb_byte inf_siginfo[sizeof (struct siginfo)];

  gdb_assert (object == TARGET_OBJECT_SIGNAL_INFO);
  gdb_assert (readbuf || writebuf);

  pid = GET_LWP (inferior_ptid);
  if (pid == 0)
    pid = GET_PID (inferior_ptid);

  if (offset > sizeof (siginfo))
    return -1;

  errno = 0;
  ptrace (PTRACE_GETSIGINFO, pid, (PTRACE_TYPE_ARG3) 0, &siginfo);
  if (errno != 0)
    return -1;

  /* When GDB is built as a 64-bit application, ptrace writes into
     SIGINFO an object with 64-bit layout.  Since debugging a 32-bit
     inferior with a 64-bit GDB should look the same as debugging it
     with a 32-bit GDB, we need to convert it.  GDB core always sees
     the converted layout, so any read/write will have to be done
     post-conversion.  */
  siginfo_fixup (&siginfo, inf_siginfo, 0);

  if (offset + len > sizeof (siginfo))
    len = sizeof (siginfo) - offset;

  if (readbuf != NULL)
    memcpy (readbuf, inf_siginfo + offset, len);
  else
    {
      memcpy (inf_siginfo + offset, writebuf, len);

      /* Convert back to ptrace layout before flushing it out.  */
      siginfo_fixup (&siginfo, inf_siginfo, 1);

      errno = 0;
      ptrace (PTRACE_SETSIGINFO, pid, (PTRACE_TYPE_ARG3) 0, &siginfo);
      if (errno != 0)
	return -1;
    }

  return len;
}

static LONGEST
linux_nat_xfer_partial (struct target_ops *ops, enum target_object object,
			const char *annex, gdb_byte *readbuf,
			const gdb_byte *writebuf,
			ULONGEST offset, LONGEST len)
{
  struct cleanup *old_chain;
  LONGEST xfer;

  if (object == TARGET_OBJECT_SIGNAL_INFO)
    return linux_xfer_siginfo (ops, object, annex, readbuf, writebuf,
			       offset, len);

  /* The target is connected but no live inferior is selected.  Pass
     this request down to a lower stratum (e.g., the executable
     file).  */
  if (object == TARGET_OBJECT_MEMORY && ptid_equal (inferior_ptid, null_ptid))
    return 0;

  old_chain = save_inferior_ptid ();

  if (is_lwp (inferior_ptid))
    inferior_ptid = pid_to_ptid (GET_LWP (inferior_ptid));

  xfer = linux_ops->to_xfer_partial (ops, object, annex, readbuf, writebuf,
				     offset, len);

  do_cleanups (old_chain);
  return xfer;
}

static int
linux_thread_alive (ptid_t ptid)
{
  int err, tmp_errno;

  gdb_assert (is_lwp (ptid));

  /* Send signal 0 instead of anything ptrace, because ptracing a
     running thread errors out claiming that the thread doesn't
     exist.  */
  err = kill_lwp (GET_LWP (ptid), 0);
  tmp_errno = errno;
  if (debug_linux_nat)
    fprintf_unfiltered (gdb_stdlog,
			"LLTA: KILL(SIG0) %s (%s)\n",
			target_pid_to_str (ptid),
			err ? safe_strerror (tmp_errno) : "OK");

  if (err != 0)
    return 0;

  return 1;
}

static int
linux_nat_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  return linux_thread_alive (ptid);
}

static char *
linux_nat_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  static char buf[64];

  if (is_lwp (ptid)
      && (GET_PID (ptid) != GET_LWP (ptid)
	  || num_lwps (GET_PID (ptid)) > 1))
    {
      snprintf (buf, sizeof (buf), "LWP %ld", GET_LWP (ptid));
      return buf;
    }

  return normal_pid_to_str (ptid);
}

static char *
linux_nat_thread_name (struct thread_info *thr)
{
  int pid = ptid_get_pid (thr->ptid);
  long lwp = ptid_get_lwp (thr->ptid);
#define FORMAT "/proc/%d/task/%ld/comm"
  char buf[sizeof (FORMAT) + 30];
  FILE *comm_file;
  char *result = NULL;

  snprintf (buf, sizeof (buf), FORMAT, pid, lwp);
  comm_file = fopen (buf, "r");
  if (comm_file)
    {
      /* Not exported by the kernel, so we define it here.  */
#define COMM_LEN 16
      static char line[COMM_LEN + 1];

      if (fgets (line, sizeof (line), comm_file))
	{
	  char *nl = strchr (line, '\n');

	  if (nl)
	    *nl = '\0';
	  if (*line != '\0')
	    result = line;
	}

      fclose (comm_file);
    }

#undef COMM_LEN
#undef FORMAT

  return result;
}

/* Accepts an integer PID; Returns a string representing a file that
   can be opened to get the symbols for the child process.  */

static char *
linux_child_pid_to_exec_file (int pid)
{
  char *name1, *name2;

  name1 = xmalloc (MAXPATHLEN);
  name2 = xmalloc (MAXPATHLEN);
  make_cleanup (xfree, name1);
  make_cleanup (xfree, name2);
  memset (name2, 0, MAXPATHLEN);

  sprintf (name1, "/proc/%d/exe", pid);
  if (readlink (name1, name2, MAXPATHLEN) > 0)
    return name2;
  else
    return name1;
}

/* Service function for corefiles and info proc.  */

static int
read_mapping (FILE *mapfile,
	      long long *addr,
	      long long *endaddr,
	      char *permissions,
	      long long *offset,
	      char *device, long long *inode, char *filename)
{
  int ret = fscanf (mapfile, "%llx-%llx %s %llx %s %llx",
		    addr, endaddr, permissions, offset, device, inode);

  filename[0] = '\0';
  if (ret > 0 && ret != EOF)
    {
      /* Eat everything up to EOL for the filename.  This will prevent
         weird filenames (such as one with embedded whitespace) from
         confusing this code.  It also makes this code more robust in
         respect to annotations the kernel may add after the filename.

         Note the filename is used for informational purposes
         only.  */
      ret += fscanf (mapfile, "%[^\n]\n", filename);
    }

  return (ret != 0 && ret != EOF);
}

/* Fills the "to_find_memory_regions" target vector.  Lists the memory
   regions in the inferior for a corefile.  */

static int
linux_nat_find_memory_regions (find_memory_region_ftype func, void *obfd)
{
  int pid = PIDGET (inferior_ptid);
  char mapsfilename[MAXPATHLEN];
  FILE *mapsfile;
  long long addr, endaddr, size, offset, inode;
  char permissions[8], device[8], filename[MAXPATHLEN];
  int read, write, exec;
  struct cleanup *cleanup;

  /* Compose the filename for the /proc memory map, and open it.  */
  sprintf (mapsfilename, "/proc/%d/maps", pid);
  if ((mapsfile = fopen (mapsfilename, "r")) == NULL)
    error (_("Could not open %s."), mapsfilename);
  cleanup = make_cleanup_fclose (mapsfile);

  if (info_verbose)
    fprintf_filtered (gdb_stdout,
		      "Reading memory regions from %s\n", mapsfilename);

  /* Now iterate until end-of-file.  */
  while (read_mapping (mapsfile, &addr, &endaddr, &permissions[0],
		       &offset, &device[0], &inode, &filename[0]))
    {
      size = endaddr - addr;

      /* Get the segment's permissions.  */
      read = (strchr (permissions, 'r') != 0);
      write = (strchr (permissions, 'w') != 0);
      exec = (strchr (permissions, 'x') != 0);

      if (info_verbose)
	{
	  fprintf_filtered (gdb_stdout,
			    "Save segment, %s bytes at %s (%c%c%c)",
			    plongest (size), paddress (target_gdbarch, addr),
			    read ? 'r' : ' ',
			    write ? 'w' : ' ', exec ? 'x' : ' ');
	  if (filename[0])
	    fprintf_filtered (gdb_stdout, " for %s", filename);
	  fprintf_filtered (gdb_stdout, "\n");
	}

      /* Invoke the callback function to create the corefile
	 segment.  */
      func (addr, size, read, write, exec, obfd);
    }
  do_cleanups (cleanup);
  return 0;
}

static int
find_signalled_thread (struct thread_info *info, void *data)
{
  if (info->suspend.stop_signal != TARGET_SIGNAL_0
      && ptid_get_pid (info->ptid) == ptid_get_pid (inferior_ptid))
    return 1;

  return 0;
}

static enum target_signal
find_stop_signal (void)
{
  struct thread_info *info =
    iterate_over_threads (find_signalled_thread, NULL);

  if (info)
    return info->suspend.stop_signal;
  else
    return TARGET_SIGNAL_0;
}

/* Records the thread's register state for the corefile note
   section.  */

static char *
linux_nat_do_thread_registers (bfd *obfd, ptid_t ptid,
			       char *note_data, int *note_size,
			       enum target_signal stop_signal)
{
  unsigned long lwp = ptid_get_lwp (ptid);
  struct gdbarch *gdbarch = target_gdbarch;
  struct regcache *regcache = get_thread_arch_regcache (ptid, gdbarch);
  const struct regset *regset;
  int core_regset_p;
  struct cleanup *old_chain;
  struct core_regset_section *sect_list;
  char *gdb_regset;

  old_chain = save_inferior_ptid ();
  inferior_ptid = ptid;
  target_fetch_registers (regcache, -1);
  do_cleanups (old_chain);

  core_regset_p = gdbarch_regset_from_core_section_p (gdbarch);
  sect_list = gdbarch_core_regset_sections (gdbarch);

  /* The loop below uses the new struct core_regset_section, which stores
     the supported section names and sizes for the core file.  Note that
     note PRSTATUS needs to be treated specially.  But the other notes are
     structurally the same, so they can benefit from the new struct.  */
  if (core_regset_p && sect_list != NULL)
    while (sect_list->sect_name != NULL)
      {
	regset = gdbarch_regset_from_core_section (gdbarch,
						   sect_list->sect_name,
						   sect_list->size);
	gdb_assert (regset && regset->collect_regset);
	gdb_regset = xmalloc (sect_list->size);
	regset->collect_regset (regset, regcache, -1,
				gdb_regset, sect_list->size);

	if (strcmp (sect_list->sect_name, ".reg") == 0)
	  note_data = (char *) elfcore_write_prstatus
				(obfd, note_data, note_size,
				 lwp, target_signal_to_host (stop_signal),
				 gdb_regset);
	else
	  note_data = (char *) elfcore_write_register_note
				(obfd, note_data, note_size,
				 sect_list->sect_name, gdb_regset,
				 sect_list->size);
	xfree (gdb_regset);
	sect_list++;
      }

  /* For architectures that does not have the struct core_regset_section
     implemented, we use the old method.  When all the architectures have
     the new support, the code below should be deleted.  */
  else
    {
      gdb_gregset_t gregs;
      gdb_fpregset_t fpregs;

      if (core_regset_p
	  && (regset = gdbarch_regset_from_core_section (gdbarch, ".reg",
							 sizeof (gregs)))
	  != NULL && regset->collect_regset != NULL)
	regset->collect_regset (regset, regcache, -1,
				&gregs, sizeof (gregs));
      else
	fill_gregset (regcache, &gregs, -1);

      note_data = (char *) elfcore_write_prstatus
	(obfd, note_data, note_size, lwp, target_signal_to_host (stop_signal),
	 &gregs);

      if (core_regset_p
          && (regset = gdbarch_regset_from_core_section (gdbarch, ".reg2",
							 sizeof (fpregs)))
	  != NULL && regset->collect_regset != NULL)
	regset->collect_regset (regset, regcache, -1,
				&fpregs, sizeof (fpregs));
      else
	fill_fpregset (regcache, &fpregs, -1);

      note_data = (char *) elfcore_write_prfpreg (obfd,
						  note_data,
						  note_size,
						  &fpregs, sizeof (fpregs));
    }

  return note_data;
}

struct linux_nat_corefile_thread_data
{
  bfd *obfd;
  char *note_data;
  int *note_size;
  int num_notes;
  enum target_signal stop_signal;
};

/* Called by gdbthread.c once per thread.  Records the thread's
   register state for the corefile note section.  */

static int
linux_nat_corefile_thread_callback (struct lwp_info *ti, void *data)
{
  struct linux_nat_corefile_thread_data *args = data;

  args->note_data = linux_nat_do_thread_registers (args->obfd,
						   ti->ptid,
						   args->note_data,
						   args->note_size,
						   args->stop_signal);
  args->num_notes++;

  return 0;
}

/* Enumerate spufs IDs for process PID.  */

static void
iterate_over_spus (int pid, void (*callback) (void *, int), void *data)
{
  char path[128];
  DIR *dir;
  struct dirent *entry;

  xsnprintf (path, sizeof path, "/proc/%d/fd", pid);
  dir = opendir (path);
  if (!dir)
    return;

  rewinddir (dir);
  while ((entry = readdir (dir)) != NULL)
    {
      struct stat st;
      struct statfs stfs;
      int fd;

      fd = atoi (entry->d_name);
      if (!fd)
	continue;

      xsnprintf (path, sizeof path, "/proc/%d/fd/%d", pid, fd);
      if (stat (path, &st) != 0)
	continue;
      if (!S_ISDIR (st.st_mode))
	continue;

      if (statfs (path, &stfs) != 0)
	continue;
      if (stfs.f_type != SPUFS_MAGIC)
	continue;

      callback (data, fd);
    }

  closedir (dir);
}

/* Generate corefile notes for SPU contexts.  */

struct linux_spu_corefile_data
{
  bfd *obfd;
  char *note_data;
  int *note_size;
};

static void
linux_spu_corefile_callback (void *data, int fd)
{
  struct linux_spu_corefile_data *args = data;
  int i;

  static const char *spu_files[] =
    {
      "object-id",
      "mem",
      "regs",
      "fpcr",
      "lslr",
      "decr",
      "decr_status",
      "signal1",
      "signal1_type",
      "signal2",
      "signal2_type",
      "event_mask",
      "event_status",
      "mbox_info",
      "ibox_info",
      "wbox_info",
      "dma_info",
      "proxydma_info",
   };

  for (i = 0; i < sizeof (spu_files) / sizeof (spu_files[0]); i++)
    {
      char annex[32], note_name[32];
      gdb_byte *spu_data;
      LONGEST spu_len;

      xsnprintf (annex, sizeof annex, "%d/%s", fd, spu_files[i]);
      spu_len = target_read_alloc (&current_target, TARGET_OBJECT_SPU,
				   annex, &spu_data);
      if (spu_len > 0)
	{
	  xsnprintf (note_name, sizeof note_name, "SPU/%s", annex);
	  args->note_data = elfcore_write_note (args->obfd, args->note_data,
						args->note_size, note_name,
						NT_SPU, spu_data, spu_len);
	  xfree (spu_data);
	}
    }
}

static char *
linux_spu_make_corefile_notes (bfd *obfd, char *note_data, int *note_size)
{
  struct linux_spu_corefile_data args;

  args.obfd = obfd;
  args.note_data = note_data;
  args.note_size = note_size;

  iterate_over_spus (PIDGET (inferior_ptid),
		     linux_spu_corefile_callback, &args);

  return args.note_data;
}

/* Fills the "to_make_corefile_note" target vector.  Builds the note
   section for a corefile, and returns it in a malloc buffer.  */

static char *
linux_nat_make_corefile_notes (bfd *obfd, int *note_size)
{
  struct linux_nat_corefile_thread_data thread_args;
  /* The variable size must be >= sizeof (prpsinfo_t.pr_fname).  */
  char fname[16] = { '\0' };
  /* The variable size must be >= sizeof (prpsinfo_t.pr_psargs).  */
  char psargs[80] = { '\0' };
  char *note_data = NULL;
  ptid_t filter = pid_to_ptid (ptid_get_pid (inferior_ptid));
  gdb_byte *auxv;
  int auxv_len;

  if (get_exec_file (0))
    {
      strncpy (fname, lbasename (get_exec_file (0)), sizeof (fname));
      strncpy (psargs, get_exec_file (0), sizeof (psargs));
      if (get_inferior_args ())
	{
	  char *string_end;
	  char *psargs_end = psargs + sizeof (psargs);

	  /* linux_elfcore_write_prpsinfo () handles zero unterminated
	     strings fine.  */
	  string_end = memchr (psargs, 0, sizeof (psargs));
	  if (string_end != NULL)
	    {
	      *string_end++ = ' ';
	      strncpy (string_end, get_inferior_args (),
		       psargs_end - string_end);
	    }
	}
      note_data = (char *) elfcore_write_prpsinfo (obfd,
						   note_data,
						   note_size, fname, psargs);
    }

  /* Dump information for threads.  */
  thread_args.obfd = obfd;
  thread_args.note_data = note_data;
  thread_args.note_size = note_size;
  thread_args.num_notes = 0;
  thread_args.stop_signal = find_stop_signal ();
  iterate_over_lwps (filter, linux_nat_corefile_thread_callback, &thread_args);
  gdb_assert (thread_args.num_notes != 0);
  note_data = thread_args.note_data;

  auxv_len = target_read_alloc (&current_target, TARGET_OBJECT_AUXV,
				NULL, &auxv);
  if (auxv_len > 0)
    {
      note_data = elfcore_write_note (obfd, note_data, note_size,
				      "CORE", NT_AUXV, auxv, auxv_len);
      xfree (auxv);
    }

  note_data = linux_spu_make_corefile_notes (obfd, note_data, note_size);

  make_cleanup (xfree, note_data);
  return note_data;
}

/* Implement the "info proc" command.  */

static void
linux_nat_info_proc_cmd (char *args, int from_tty)
{
  /* A long is used for pid instead of an int to avoid a loss of precision
     compiler warning from the output of strtoul.  */
  long pid = PIDGET (inferior_ptid);
  FILE *procfile;
  char **argv = NULL;
  char buffer[MAXPATHLEN];
  char fname1[MAXPATHLEN], fname2[MAXPATHLEN];
  int cmdline_f = 1;
  int cwd_f = 1;
  int exe_f = 1;
  int mappings_f = 0;
  int status_f = 0;
  int stat_f = 0;
  int all = 0;
  struct stat dummy;

  if (args)
    {
      /* Break up 'args' into an argv array.  */
      argv = gdb_buildargv (args);
      make_cleanup_freeargv (argv);
    }
  while (argv != NULL && *argv != NULL)
    {
      if (isdigit (argv[0][0]))
	{
	  pid = strtoul (argv[0], NULL, 10);
	}
      else if (strncmp (argv[0], "mappings", strlen (argv[0])) == 0)
	{
	  mappings_f = 1;
	}
      else if (strcmp (argv[0], "status") == 0)
	{
	  status_f = 1;
	}
      else if (strcmp (argv[0], "stat") == 0)
	{
	  stat_f = 1;
	}
      else if (strcmp (argv[0], "cmd") == 0)
	{
	  cmdline_f = 1;
	}
      else if (strncmp (argv[0], "exe", strlen (argv[0])) == 0)
	{
	  exe_f = 1;
	}
      else if (strcmp (argv[0], "cwd") == 0)
	{
	  cwd_f = 1;
	}
      else if (strncmp (argv[0], "all", strlen (argv[0])) == 0)
	{
	  all = 1;
	}
      else
	{
	  /* [...] (future options here).  */
	}
      argv++;
    }
  if (pid == 0)
    error (_("No current process: you must name one."));

  sprintf (fname1, "/proc/%ld", pid);
  if (stat (fname1, &dummy) != 0)
    error (_("No /proc directory: '%s'"), fname1);

  printf_filtered (_("process %ld\n"), pid);
  if (cmdline_f || all)
    {
      sprintf (fname1, "/proc/%ld/cmdline", pid);
      if ((procfile = fopen (fname1, "r")) != NULL)
	{
	  struct cleanup *cleanup = make_cleanup_fclose (procfile);

          if (fgets (buffer, sizeof (buffer), procfile))
            printf_filtered ("cmdline = '%s'\n", buffer);
          else
            warning (_("unable to read '%s'"), fname1);
	  do_cleanups (cleanup);
	}
      else
	warning (_("unable to open /proc file '%s'"), fname1);
    }
  if (cwd_f || all)
    {
      sprintf (fname1, "/proc/%ld/cwd", pid);
      memset (fname2, 0, sizeof (fname2));
      if (readlink (fname1, fname2, sizeof (fname2)) > 0)
	printf_filtered ("cwd = '%s'\n", fname2);
      else
	warning (_("unable to read link '%s'"), fname1);
    }
  if (exe_f || all)
    {
      sprintf (fname1, "/proc/%ld/exe", pid);
      memset (fname2, 0, sizeof (fname2));
      if (readlink (fname1, fname2, sizeof (fname2)) > 0)
	printf_filtered ("exe = '%s'\n", fname2);
      else
	warning (_("unable to read link '%s'"), fname1);
    }
  if (mappings_f || all)
    {
      sprintf (fname1, "/proc/%ld/maps", pid);
      if ((procfile = fopen (fname1, "r")) != NULL)
	{
	  long long addr, endaddr, size, offset, inode;
	  char permissions[8], device[8], filename[MAXPATHLEN];
	  struct cleanup *cleanup;

	  cleanup = make_cleanup_fclose (procfile);
	  printf_filtered (_("Mapped address spaces:\n\n"));
	  if (gdbarch_addr_bit (target_gdbarch) == 32)
	    {
	      printf_filtered ("\t%10s %10s %10s %10s %7s\n",
			   "Start Addr",
			   "  End Addr",
			   "      Size", "    Offset", "objfile");
            }
	  else
            {
	      printf_filtered ("  %18s %18s %10s %10s %7s\n",
			   "Start Addr",
			   "  End Addr",
			   "      Size", "    Offset", "objfile");
	    }

	  while (read_mapping (procfile, &addr, &endaddr, &permissions[0],
			       &offset, &device[0], &inode, &filename[0]))
	    {
	      size = endaddr - addr;

	      /* FIXME: carlton/2003-08-27: Maybe the printf_filtered
		 calls here (and possibly above) should be abstracted
		 out into their own functions?  Andrew suggests using
		 a generic local_address_string instead to print out
		 the addresses; that makes sense to me, too.  */

	      if (gdbarch_addr_bit (target_gdbarch) == 32)
	        {
	          printf_filtered ("\t%#10lx %#10lx %#10x %#10x %7s\n",
			       (unsigned long) addr,	/* FIXME: pr_addr */
			       (unsigned long) endaddr,
			       (int) size,
			       (unsigned int) offset,
			       filename[0] ? filename : "");
		}
	      else
	        {
	          printf_filtered ("  %#18lx %#18lx %#10x %#10x %7s\n",
			       (unsigned long) addr,	/* FIXME: pr_addr */
			       (unsigned long) endaddr,
			       (int) size,
			       (unsigned int) offset,
			       filename[0] ? filename : "");
	        }
	    }

	  do_cleanups (cleanup);
	}
      else
	warning (_("unable to open /proc file '%s'"), fname1);
    }
  if (status_f || all)
    {
      sprintf (fname1, "/proc/%ld/status", pid);
      if ((procfile = fopen (fname1, "r")) != NULL)
	{
	  struct cleanup *cleanup = make_cleanup_fclose (procfile);

	  while (fgets (buffer, sizeof (buffer), procfile) != NULL)
	    puts_filtered (buffer);
	  do_cleanups (cleanup);
	}
      else
	warning (_("unable to open /proc file '%s'"), fname1);
    }
  if (stat_f || all)
    {
      sprintf (fname1, "/proc/%ld/stat", pid);
      if ((procfile = fopen (fname1, "r")) != NULL)
	{
	  int itmp;
	  char ctmp;
	  long ltmp;
	  struct cleanup *cleanup = make_cleanup_fclose (procfile);

	  if (fscanf (procfile, "%d ", &itmp) > 0)
	    printf_filtered (_("Process: %d\n"), itmp);
	  if (fscanf (procfile, "(%[^)]) ", &buffer[0]) > 0)
	    printf_filtered (_("Exec file: %s\n"), buffer);
	  if (fscanf (procfile, "%c ", &ctmp) > 0)
	    printf_filtered (_("State: %c\n"), ctmp);
	  if (fscanf (procfile, "%d ", &itmp) > 0)
	    printf_filtered (_("Parent process: %d\n"), itmp);
	  if (fscanf (procfile, "%d ", &itmp) > 0)
	    printf_filtered (_("Process group: %d\n"), itmp);
	  if (fscanf (procfile, "%d ", &itmp) > 0)
	    printf_filtered (_("Session id: %d\n"), itmp);
	  if (fscanf (procfile, "%d ", &itmp) > 0)
	    printf_filtered (_("TTY: %d\n"), itmp);
	  if (fscanf (procfile, "%d ", &itmp) > 0)
	    printf_filtered (_("TTY owner process group: %d\n"), itmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("Flags: 0x%lx\n"), ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("Minor faults (no memory page): %lu\n"),
			     (unsigned long) ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("Minor faults, children: %lu\n"),
			     (unsigned long) ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("Major faults (memory page faults): %lu\n"),
			     (unsigned long) ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("Major faults, children: %lu\n"),
			     (unsigned long) ltmp);
	  if (fscanf (procfile, "%ld ", &ltmp) > 0)
	    printf_filtered (_("utime: %ld\n"), ltmp);
	  if (fscanf (procfile, "%ld ", &ltmp) > 0)
	    printf_filtered (_("stime: %ld\n"), ltmp);
	  if (fscanf (procfile, "%ld ", &ltmp) > 0)
	    printf_filtered (_("utime, children: %ld\n"), ltmp);
	  if (fscanf (procfile, "%ld ", &ltmp) > 0)
	    printf_filtered (_("stime, children: %ld\n"), ltmp);
	  if (fscanf (procfile, "%ld ", &ltmp) > 0)
	    printf_filtered (_("jiffies remaining in current "
			       "time slice: %ld\n"), ltmp);
	  if (fscanf (procfile, "%ld ", &ltmp) > 0)
	    printf_filtered (_("'nice' value: %ld\n"), ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("jiffies until next timeout: %lu\n"),
			     (unsigned long) ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("jiffies until next SIGALRM: %lu\n"),
			     (unsigned long) ltmp);
	  if (fscanf (procfile, "%ld ", &ltmp) > 0)
	    printf_filtered (_("start time (jiffies since "
			       "system boot): %ld\n"), ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("Virtual memory size: %lu\n"),
			     (unsigned long) ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("Resident set size: %lu\n"),
			     (unsigned long) ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("rlim: %lu\n"), (unsigned long) ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("Start of text: 0x%lx\n"), ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("End of text: 0x%lx\n"), ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)
	    printf_filtered (_("Start of stack: 0x%lx\n"), ltmp);
#if 0	/* Don't know how architecture-dependent the rest is...
	   Anyway the signal bitmap info is available from "status".  */
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)	/* FIXME arch?  */
	    printf_filtered (_("Kernel stack pointer: 0x%lx\n"), ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)	/* FIXME arch?  */
	    printf_filtered (_("Kernel instr pointer: 0x%lx\n"), ltmp);
	  if (fscanf (procfile, "%ld ", &ltmp) > 0)
	    printf_filtered (_("Pending signals bitmap: 0x%lx\n"), ltmp);
	  if (fscanf (procfile, "%ld ", &ltmp) > 0)
	    printf_filtered (_("Blocked signals bitmap: 0x%lx\n"), ltmp);
	  if (fscanf (procfile, "%ld ", &ltmp) > 0)
	    printf_filtered (_("Ignored signals bitmap: 0x%lx\n"), ltmp);
	  if (fscanf (procfile, "%ld ", &ltmp) > 0)
	    printf_filtered (_("Catched signals bitmap: 0x%lx\n"), ltmp);
	  if (fscanf (procfile, "%lu ", &ltmp) > 0)	/* FIXME arch?  */
	    printf_filtered (_("wchan (system call): 0x%lx\n"), ltmp);
#endif
	  do_cleanups (cleanup);
	}
      else
	warning (_("unable to open /proc file '%s'"), fname1);
    }
}

/* Implement the to_xfer_partial interface for memory reads using the /proc
   filesystem.  Because we can use a single read() call for /proc, this
   can be much more efficient than banging away at PTRACE_PEEKTEXT,
   but it doesn't support writes.  */

static LONGEST
linux_proc_xfer_partial (struct target_ops *ops, enum target_object object,
			 const char *annex, gdb_byte *readbuf,
			 const gdb_byte *writebuf,
			 ULONGEST offset, LONGEST len)
{
  LONGEST ret;
  int fd;
  char filename[64];

  if (object != TARGET_OBJECT_MEMORY || !readbuf)
    return 0;

  /* Don't bother for one word.  */
  if (len < 3 * sizeof (long))
    return 0;

  /* We could keep this file open and cache it - possibly one per
     thread.  That requires some juggling, but is even faster.  */
  sprintf (filename, "/proc/%d/mem", PIDGET (inferior_ptid));
  fd = open (filename, O_RDONLY | O_LARGEFILE);
  if (fd == -1)
    return 0;

  /* If pread64 is available, use it.  It's faster if the kernel
     supports it (only one syscall), and it's 64-bit safe even on
     32-bit platforms (for instance, SPARC debugging a SPARC64
     application).  */
#ifdef HAVE_PREAD64
  if (pread64 (fd, readbuf, len, offset) != len)
#else
  if (lseek (fd, offset, SEEK_SET) == -1 || read (fd, readbuf, len) != len)
#endif
    ret = 0;
  else
    ret = len;

  close (fd);
  return ret;
}


/* Enumerate spufs IDs for process PID.  */
static LONGEST
spu_enumerate_spu_ids (int pid, gdb_byte *buf, ULONGEST offset, LONGEST len)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch);
  LONGEST pos = 0;
  LONGEST written = 0;
  char path[128];
  DIR *dir;
  struct dirent *entry;

  xsnprintf (path, sizeof path, "/proc/%d/fd", pid);
  dir = opendir (path);
  if (!dir)
    return -1;

  rewinddir (dir);
  while ((entry = readdir (dir)) != NULL)
    {
      struct stat st;
      struct statfs stfs;
      int fd;

      fd = atoi (entry->d_name);
      if (!fd)
	continue;

      xsnprintf (path, sizeof path, "/proc/%d/fd/%d", pid, fd);
      if (stat (path, &st) != 0)
	continue;
      if (!S_ISDIR (st.st_mode))
	continue;

      if (statfs (path, &stfs) != 0)
	continue;
      if (stfs.f_type != SPUFS_MAGIC)
	continue;

      if (pos >= offset && pos + 4 <= offset + len)
	{
	  store_unsigned_integer (buf + pos - offset, 4, byte_order, fd);
	  written += 4;
	}
      pos += 4;
    }

  closedir (dir);
  return written;
}

/* Implement the to_xfer_partial interface for the TARGET_OBJECT_SPU
   object type, using the /proc file system.  */
static LONGEST
linux_proc_xfer_spu (struct target_ops *ops, enum target_object object,
		     const char *annex, gdb_byte *readbuf,
		     const gdb_byte *writebuf,
		     ULONGEST offset, LONGEST len)
{
  char buf[128];
  int fd = 0;
  int ret = -1;
  int pid = PIDGET (inferior_ptid);

  if (!annex)
    {
      if (!readbuf)
	return -1;
      else
	return spu_enumerate_spu_ids (pid, readbuf, offset, len);
    }

  xsnprintf (buf, sizeof buf, "/proc/%d/fd/%s", pid, annex);
  fd = open (buf, writebuf? O_WRONLY : O_RDONLY);
  if (fd <= 0)
    return -1;

  if (offset != 0
      && lseek (fd, (off_t) offset, SEEK_SET) != (off_t) offset)
    {
      close (fd);
      return 0;
    }

  if (writebuf)
    ret = write (fd, writebuf, (size_t) len);
  else if (readbuf)
    ret = read (fd, readbuf, (size_t) len);

  close (fd);
  return ret;
}


/* Parse LINE as a signal set and add its set bits to SIGS.  */

static void
add_line_to_sigset (const char *line, sigset_t *sigs)
{
  int len = strlen (line) - 1;
  const char *p;
  int signum;

  if (line[len] != '\n')
    error (_("Could not parse signal set: %s"), line);

  p = line;
  signum = len * 4;
  while (len-- > 0)
    {
      int digit;

      if (*p >= '0' && *p <= '9')
	digit = *p - '0';
      else if (*p >= 'a' && *p <= 'f')
	digit = *p - 'a' + 10;
      else
	error (_("Could not parse signal set: %s"), line);

      signum -= 4;

      if (digit & 1)
	sigaddset (sigs, signum + 1);
      if (digit & 2)
	sigaddset (sigs, signum + 2);
      if (digit & 4)
	sigaddset (sigs, signum + 3);
      if (digit & 8)
	sigaddset (sigs, signum + 4);

      p++;
    }
}

/* Find process PID's pending signals from /proc/pid/status and set
   SIGS to match.  */

void
linux_proc_pending_signals (int pid, sigset_t *pending,
			    sigset_t *blocked, sigset_t *ignored)
{
  FILE *procfile;
  char buffer[MAXPATHLEN], fname[MAXPATHLEN];
  struct cleanup *cleanup;

  sigemptyset (pending);
  sigemptyset (blocked);
  sigemptyset (ignored);
  sprintf (fname, "/proc/%d/status", pid);
  procfile = fopen (fname, "r");
  if (procfile == NULL)
    error (_("Could not open %s"), fname);
  cleanup = make_cleanup_fclose (procfile);

  while (fgets (buffer, MAXPATHLEN, procfile) != NULL)
    {
      /* Normal queued signals are on the SigPnd line in the status
	 file.  However, 2.6 kernels also have a "shared" pending
	 queue for delivering signals to a thread group, so check for
	 a ShdPnd line also.

	 Unfortunately some Red Hat kernels include the shared pending
	 queue but not the ShdPnd status field.  */

      if (strncmp (buffer, "SigPnd:\t", 8) == 0)
	add_line_to_sigset (buffer + 8, pending);
      else if (strncmp (buffer, "ShdPnd:\t", 8) == 0)
	add_line_to_sigset (buffer + 8, pending);
      else if (strncmp (buffer, "SigBlk:\t", 8) == 0)
	add_line_to_sigset (buffer + 8, blocked);
      else if (strncmp (buffer, "SigIgn:\t", 8) == 0)
	add_line_to_sigset (buffer + 8, ignored);
    }

  do_cleanups (cleanup);
}

static LONGEST
linux_nat_xfer_osdata (struct target_ops *ops, enum target_object object,
		       const char *annex, gdb_byte *readbuf,
		       const gdb_byte *writebuf, ULONGEST offset, LONGEST len)
{
  /* We make the process list snapshot when the object starts to be
     read.  */
  static const char *buf;
  static LONGEST len_avail = -1;
  static struct obstack obstack;

  DIR *dirp;

  gdb_assert (object == TARGET_OBJECT_OSDATA);

  if (!annex)
    {
      if (offset == 0)
	{
	  if (len_avail != -1 && len_avail != 0)
	    obstack_free (&obstack, NULL);
	  len_avail = 0;
	  buf = NULL;
	  obstack_init (&obstack);
	  obstack_grow_str (&obstack, "<osdata type=\"types\">\n");

	  obstack_xml_printf (&obstack,
			      "<item>"
			      "<column name=\"Type\">processes</column>"
			      "<column name=\"Description\">"
			      "Listing of all processes</column>"
			      "</item>");

	  obstack_grow_str0 (&obstack, "</osdata>\n");
	  buf = obstack_finish (&obstack);
	  len_avail = strlen (buf);
	}

      if (offset >= len_avail)
	{
	  /* Done.  Get rid of the obstack.  */
	  obstack_free (&obstack, NULL);
	  buf = NULL;
	  len_avail = 0;
	  return 0;
	}

      if (len > len_avail - offset)
	len = len_avail - offset;
      memcpy (readbuf, buf + offset, len);

      return len;
    }

  if (strcmp (annex, "processes") != 0)
    return 0;

  gdb_assert (readbuf && !writebuf);

  if (offset == 0)
    {
      if (len_avail != -1 && len_avail != 0)
	obstack_free (&obstack, NULL);
      len_avail = 0;
      buf = NULL;
      obstack_init (&obstack);
      obstack_grow_str (&obstack, "<osdata type=\"processes\">\n");

      dirp = opendir ("/proc");
      if (dirp)
	{
	  struct dirent *dp;

	  while ((dp = readdir (dirp)) != NULL)
	    {
	      struct stat statbuf;
	      char procentry[sizeof ("/proc/4294967295")];

	      if (!isdigit (dp->d_name[0])
		  || NAMELEN (dp) > sizeof ("4294967295") - 1)
		continue;

	      sprintf (procentry, "/proc/%s", dp->d_name);
	      if (stat (procentry, &statbuf) == 0
		  && S_ISDIR (statbuf.st_mode))
		{
		  char *pathname;
		  FILE *f;
		  char cmd[MAXPATHLEN + 1];
		  struct passwd *entry;

		  pathname = xstrprintf ("/proc/%s/cmdline", dp->d_name);
		  entry = getpwuid (statbuf.st_uid);

		  if ((f = fopen (pathname, "r")) != NULL)
		    {
		      size_t length = fread (cmd, 1, sizeof (cmd) - 1, f);

		      if (length > 0)
			{
			  int i;

			  for (i = 0; i < length; i++)
			    if (cmd[i] == '\0')
			      cmd[i] = ' ';
			  cmd[length] = '\0';

			  obstack_xml_printf (
			    &obstack,
			    "<item>"
			    "<column name=\"pid\">%s</column>"
			    "<column name=\"user\">%s</column>"
			    "<column name=\"command\">%s</column>"
			    "</item>",
			    dp->d_name,
			    entry ? entry->pw_name : "?",
			    cmd);
			}
		      fclose (f);
		    }

		  xfree (pathname);
		}
	    }

	  closedir (dirp);
	}

      obstack_grow_str0 (&obstack, "</osdata>\n");
      buf = obstack_finish (&obstack);
      len_avail = strlen (buf);
    }

  if (offset >= len_avail)
    {
      /* Done.  Get rid of the obstack.  */
      obstack_free (&obstack, NULL);
      buf = NULL;
      len_avail = 0;
      return 0;
    }

  if (len > len_avail - offset)
    len = len_avail - offset;
  memcpy (readbuf, buf + offset, len);

  return len;
}

static LONGEST
linux_xfer_partial (struct target_ops *ops, enum target_object object,
                    const char *annex, gdb_byte *readbuf,
		    const gdb_byte *writebuf, ULONGEST offset, LONGEST len)
{
  LONGEST xfer;

  if (object == TARGET_OBJECT_AUXV)
    return memory_xfer_auxv (ops, object, annex, readbuf, writebuf,
			     offset, len);

  if (object == TARGET_OBJECT_OSDATA)
    return linux_nat_xfer_osdata (ops, object, annex, readbuf, writebuf,
                               offset, len);

  if (object == TARGET_OBJECT_SPU)
    return linux_proc_xfer_spu (ops, object, annex, readbuf, writebuf,
				offset, len);

  /* GDB calculates all the addresses in possibly larget width of the address.
     Address width needs to be masked before its final use - either by
     linux_proc_xfer_partial or inf_ptrace_xfer_partial.

     Compare ADDR_BIT first to avoid a compiler warning on shift overflow.  */

  if (object == TARGET_OBJECT_MEMORY)
    {
      int addr_bit = gdbarch_addr_bit (target_gdbarch);

      if (addr_bit < (sizeof (ULONGEST) * HOST_CHAR_BIT))
	offset &= ((ULONGEST) 1 << addr_bit) - 1;
    }

  xfer = linux_proc_xfer_partial (ops, object, annex, readbuf, writebuf,
				  offset, len);
  if (xfer != 0)
    return xfer;

  return super_xfer_partial (ops, object, annex, readbuf, writebuf,
			     offset, len);
}

/* Create a prototype generic GNU/Linux target.  The client can override
   it with local methods.  */

static void
linux_target_install_ops (struct target_ops *t)
{
  t->to_insert_fork_catchpoint = linux_child_insert_fork_catchpoint;
  t->to_remove_fork_catchpoint = linux_child_remove_fork_catchpoint;
  t->to_insert_vfork_catchpoint = linux_child_insert_vfork_catchpoint;
  t->to_remove_vfork_catchpoint = linux_child_remove_vfork_catchpoint;
  t->to_insert_exec_catchpoint = linux_child_insert_exec_catchpoint;
  t->to_remove_exec_catchpoint = linux_child_remove_exec_catchpoint;
  t->to_set_syscall_catchpoint = linux_child_set_syscall_catchpoint;
  t->to_pid_to_exec_file = linux_child_pid_to_exec_file;
  t->to_post_startup_inferior = linux_child_post_startup_inferior;
  t->to_post_attach = linux_child_post_attach;
  t->to_follow_fork = linux_child_follow_fork;
  t->to_find_memory_regions = linux_nat_find_memory_regions;
  t->to_make_corefile_notes = linux_nat_make_corefile_notes;

  super_xfer_partial = t->to_xfer_partial;
  t->to_xfer_partial = linux_xfer_partial;
}

struct target_ops *
linux_target (void)
{
  struct target_ops *t;

  t = inf_ptrace_target ();
  linux_target_install_ops (t);

  return t;
}

struct target_ops *
linux_trad_target (CORE_ADDR (*register_u_offset)(struct gdbarch *, int, int))
{
  struct target_ops *t;

  t = inf_ptrace_trad_target (register_u_offset);
  linux_target_install_ops (t);

  return t;
}

/* target_is_async_p implementation.  */

static int
linux_nat_is_async_p (void)
{
  /* NOTE: palves 2008-03-21: We're only async when the user requests
     it explicitly with the "set target-async" command.
     Someday, linux will always be async.  */
  if (!target_async_permitted)
    return 0;

  /* See target.h/target_async_mask.  */
  return linux_nat_async_mask_value;
}

/* target_can_async_p implementation.  */

static int
linux_nat_can_async_p (void)
{
  /* NOTE: palves 2008-03-21: We're only async when the user requests
     it explicitly with the "set target-async" command.
     Someday, linux will always be async.  */
  if (!target_async_permitted)
    return 0;

  /* See target.h/target_async_mask.  */
  return linux_nat_async_mask_value;
}

static int
linux_nat_supports_non_stop (void)
{
  return 1;
}

/* True if we want to support multi-process.  To be removed when GDB
   supports multi-exec.  */

int linux_multi_process = 1;

static int
linux_nat_supports_multi_process (void)
{
  return linux_multi_process;
}

/* target_async_mask implementation.  */

static int
linux_nat_async_mask (int new_mask)
{
  int curr_mask = linux_nat_async_mask_value;

  if (curr_mask != new_mask)
    {
      if (new_mask == 0)
	{
	  linux_nat_async (NULL, 0);
	  linux_nat_async_mask_value = new_mask;
	}
      else
	{
	  linux_nat_async_mask_value = new_mask;

	  /* If we're going out of async-mask in all-stop, then the
	     inferior is stopped.  The next resume will call
	     target_async.  In non-stop, the target event source
	     should be always registered in the event loop.  Do so
	     now.  */
	  if (non_stop)
	    linux_nat_async (inferior_event_handler, 0);
	}
    }

  return curr_mask;
}

static int async_terminal_is_ours = 1;

/* target_terminal_inferior implementation.  */

static void
linux_nat_terminal_inferior (void)
{
  if (!target_is_async_p ())
    {
      /* Async mode is disabled.  */
      terminal_inferior ();
      return;
    }

  terminal_inferior ();

  /* Calls to target_terminal_*() are meant to be idempotent.  */
  if (!async_terminal_is_ours)
    return;

  delete_file_handler (input_fd);
  async_terminal_is_ours = 0;
  set_sigint_trap ();
}

/* target_terminal_ours implementation.  */

static void
linux_nat_terminal_ours (void)
{
  if (!target_is_async_p ())
    {
      /* Async mode is disabled.  */
      terminal_ours ();
      return;
    }

  /* GDB should never give the terminal to the inferior if the
     inferior is running in the background (run&, continue&, etc.),
     but claiming it sure should.  */
  terminal_ours ();

  if (async_terminal_is_ours)
    return;

  clear_sigint_trap ();
  add_file_handler (input_fd, stdin_event_handler, 0);
  async_terminal_is_ours = 1;
}

static void (*async_client_callback) (enum inferior_event_type event_type,
				      void *context);
static void *async_client_context;

/* SIGCHLD handler that serves two purposes: In non-stop/async mode,
   so we notice when any child changes state, and notify the
   event-loop; it allows us to use sigsuspend in linux_nat_wait_1
   above to wait for the arrival of a SIGCHLD.  */

static void
sigchld_handler (int signo)
{
  int old_errno = errno;

  if (debug_linux_nat_async)
    fprintf_unfiltered (gdb_stdlog, "sigchld\n");

  if (signo == SIGCHLD
      && linux_nat_event_pipe[0] != -1)
    async_file_mark (); /* Let the event loop know that there are
			   events to handle.  */

  errno = old_errno;
}

/* Callback registered with the target events file descriptor.  */

static void
handle_target_event (int error, gdb_client_data client_data)
{
  (*async_client_callback) (INF_REG_EVENT, async_client_context);
}

/* Create/destroy the target events pipe.  Returns previous state.  */

static int
linux_async_pipe (int enable)
{
  int previous = (linux_nat_event_pipe[0] != -1);

  if (previous != enable)
    {
      sigset_t prev_mask;

      block_child_signals (&prev_mask);

      if (enable)
	{
	  if (pipe (linux_nat_event_pipe) == -1)
	    internal_error (__FILE__, __LINE__,
			    "creating event pipe failed.");

	  fcntl (linux_nat_event_pipe[0], F_SETFL, O_NONBLOCK);
	  fcntl (linux_nat_event_pipe[1], F_SETFL, O_NONBLOCK);
	}
      else
	{
	  close (linux_nat_event_pipe[0]);
	  close (linux_nat_event_pipe[1]);
	  linux_nat_event_pipe[0] = -1;
	  linux_nat_event_pipe[1] = -1;
	}

      restore_child_signals_mask (&prev_mask);
    }

  return previous;
}

/* target_async implementation.  */

static void
linux_nat_async (void (*callback) (enum inferior_event_type event_type,
				   void *context), void *context)
{
  if (linux_nat_async_mask_value == 0 || !target_async_permitted)
    internal_error (__FILE__, __LINE__,
		    "Calling target_async when async is masked");

  if (callback != NULL)
    {
      async_client_callback = callback;
      async_client_context = context;
      if (!linux_async_pipe (1))
	{
	  add_file_handler (linux_nat_event_pipe[0],
			    handle_target_event, NULL);
	  /* There may be pending events to handle.  Tell the event loop
	     to poll them.  */
	  async_file_mark ();
	}
    }
  else
    {
      async_client_callback = callback;
      async_client_context = context;
      delete_file_handler (linux_nat_event_pipe[0]);
      linux_async_pipe (0);
    }
  return;
}

/* Stop an LWP, and push a TARGET_SIGNAL_0 stop status if no other
   event came out.  */

static int
linux_nat_stop_lwp (struct lwp_info *lwp, void *data)
{
  if (!lwp->stopped)
    {
      ptid_t ptid = lwp->ptid;

      if (debug_linux_nat)
	fprintf_unfiltered (gdb_stdlog,
			    "LNSL: running -> suspending %s\n",
			    target_pid_to_str (lwp->ptid));


      stop_callback (lwp, NULL);
      stop_wait_callback (lwp, NULL);

      /* If the lwp exits while we try to stop it, there's nothing
	 else to do.  */
      lwp = find_lwp_pid (ptid);
      if (lwp == NULL)
	return 0;

      /* If we didn't collect any signal other than SIGSTOP while
	 stopping the LWP, push a SIGNAL_0 event.  In either case, the
	 event-loop will end up calling target_wait which will collect
	 these.  */
      if (lwp->status == 0)
	lwp->status = W_STOPCODE (0);
      async_file_mark ();
    }
  else
    {
      /* Already known to be stopped; do nothing.  */

      if (debug_linux_nat)
	{
	  if (find_thread_ptid (lwp->ptid)->stop_requested)
	    fprintf_unfiltered (gdb_stdlog,
				"LNSL: already stopped/stop_requested %s\n",
				target_pid_to_str (lwp->ptid));
	  else
	    fprintf_unfiltered (gdb_stdlog,
				"LNSL: already stopped/no "
				"stop_requested yet %s\n",
				target_pid_to_str (lwp->ptid));
	}
    }
  return 0;
}

static void
linux_nat_stop (ptid_t ptid)
{
  if (non_stop)
    iterate_over_lwps (ptid, linux_nat_stop_lwp, NULL);
  else
    linux_ops->to_stop (ptid);
}

static void
linux_nat_close (int quitting)
{
  /* Unregister from the event loop.  */
  if (target_is_async_p ())
    target_async (NULL, 0);

  /* Reset the async_masking.  */
  linux_nat_async_mask_value = 1;

  if (linux_ops->to_close)
    linux_ops->to_close (quitting);
}

/* When requests are passed down from the linux-nat layer to the
   single threaded inf-ptrace layer, ptids of (lwpid,0,0) form are
   used.  The address space pointer is stored in the inferior object,
   but the common code that is passed such ptid can't tell whether
   lwpid is a "main" process id or not (it assumes so).  We reverse
   look up the "main" process id from the lwp here.  */

struct address_space *
linux_nat_thread_address_space (struct target_ops *t, ptid_t ptid)
{
  struct lwp_info *lwp;
  struct inferior *inf;
  int pid;

  pid = GET_LWP (ptid);
  if (GET_LWP (ptid) == 0)
    {
      /* An (lwpid,0,0) ptid.  Look up the lwp object to get at the
	 tgid.  */
      lwp = find_lwp_pid (ptid);
      pid = GET_PID (lwp->ptid);
    }
  else
    {
      /* A (pid,lwpid,0) ptid.  */
      pid = GET_PID (ptid);
    }

  inf = find_inferior_pid (pid);
  gdb_assert (inf != NULL);
  return inf->aspace;
}

int
linux_nat_core_of_thread_1 (ptid_t ptid)
{
  struct cleanup *back_to;
  char *filename;
  FILE *f;
  char *content = NULL;
  char *p;
  char *ts = 0;
  int content_read = 0;
  int i;
  int core;

  filename = xstrprintf ("/proc/%d/task/%ld/stat",
			 GET_PID (ptid), GET_LWP (ptid));
  back_to = make_cleanup (xfree, filename);

  f = fopen (filename, "r");
  if (!f)
    {
      do_cleanups (back_to);
      return -1;
    }

  make_cleanup_fclose (f);

  for (;;)
    {
      int n;

      content = xrealloc (content, content_read + 1024);
      n = fread (content + content_read, 1, 1024, f);
      content_read += n;
      if (n < 1024)
	{
	  content[content_read] = '\0';
	  break;
	}
    }

  make_cleanup (xfree, content);

  p = strchr (content, '(');

  /* Skip ")".  */
  if (p != NULL)
    p = strchr (p, ')');
  if (p != NULL)
    p++;

  /* If the first field after program name has index 0, then core number is
     the field with index 36.  There's no constant for that anywhere.  */
  if (p != NULL)
    p = strtok_r (p, " ", &ts);
  for (i = 0; p != NULL && i != 36; ++i)
    p = strtok_r (NULL, " ", &ts);

  if (p == NULL || sscanf (p, "%d", &core) == 0)
    core = -1;

  do_cleanups (back_to);

  return core;
}

/* Return the cached value of the processor core for thread PTID.  */

int
linux_nat_core_of_thread (struct target_ops *ops, ptid_t ptid)
{
  struct lwp_info *info = find_lwp_pid (ptid);

  if (info)
    return info->core;
  return -1;
}

void
linux_nat_add_target (struct target_ops *t)
{
  /* Save the provided single-threaded target.  We save this in a separate
     variable because another target we've inherited from (e.g. inf-ptrace)
     may have saved a pointer to T; we want to use it for the final
     process stratum target.  */
  linux_ops_saved = *t;
  linux_ops = &linux_ops_saved;

  /* Override some methods for multithreading.  */
  t->to_create_inferior = linux_nat_create_inferior;
  t->to_attach = linux_nat_attach;
  t->to_detach = linux_nat_detach;
  t->to_resume = linux_nat_resume;
  t->to_wait = linux_nat_wait;
  t->to_xfer_partial = linux_nat_xfer_partial;
  t->to_kill = linux_nat_kill;
  t->to_mourn_inferior = linux_nat_mourn_inferior;
  t->to_thread_alive = linux_nat_thread_alive;
  t->to_pid_to_str = linux_nat_pid_to_str;
  t->to_thread_name = linux_nat_thread_name;
  t->to_has_thread_control = tc_schedlock;
  t->to_thread_address_space = linux_nat_thread_address_space;
  t->to_stopped_by_watchpoint = linux_nat_stopped_by_watchpoint;
  t->to_stopped_data_address = linux_nat_stopped_data_address;

  t->to_can_async_p = linux_nat_can_async_p;
  t->to_is_async_p = linux_nat_is_async_p;
  t->to_supports_non_stop = linux_nat_supports_non_stop;
  t->to_async = linux_nat_async;
  t->to_async_mask = linux_nat_async_mask;
  t->to_terminal_inferior = linux_nat_terminal_inferior;
  t->to_terminal_ours = linux_nat_terminal_ours;
  t->to_close = linux_nat_close;

  /* Methods for non-stop support.  */
  t->to_stop = linux_nat_stop;

  t->to_supports_multi_process = linux_nat_supports_multi_process;

  t->to_core_of_thread = linux_nat_core_of_thread;

  /* We don't change the stratum; this target will sit at
     process_stratum and thread_db will set at thread_stratum.  This
     is a little strange, since this is a multi-threaded-capable
     target, but we want to be on the stack below thread_db, and we
     also want to be used for single-threaded processes.  */

  add_target (t);
}

/* Register a method to call whenever a new thread is attached.  */
void
linux_nat_set_new_thread (struct target_ops *t, void (*new_thread) (ptid_t))
{
  /* Save the pointer.  We only support a single registered instance
     of the GNU/Linux native target, so we do not need to map this to
     T.  */
  linux_nat_new_thread = new_thread;
}

/* Register a method that converts a siginfo object between the layout
   that ptrace returns, and the layout in the architecture of the
   inferior.  */
void
linux_nat_set_siginfo_fixup (struct target_ops *t,
			     int (*siginfo_fixup) (struct siginfo *,
						   gdb_byte *,
						   int))
{
  /* Save the pointer.  */
  linux_nat_siginfo_fixup = siginfo_fixup;
}

/* Return the saved siginfo associated with PTID.  */
struct siginfo *
linux_nat_get_siginfo (ptid_t ptid)
{
  struct lwp_info *lp = find_lwp_pid (ptid);

  gdb_assert (lp != NULL);

  return &lp->siginfo;
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_linux_nat;

void
_initialize_linux_nat (void)
{
  add_info ("proc", linux_nat_info_proc_cmd, _("\
Show /proc process information about any running process.\n\
Specify any process id, or use the program being debugged by default.\n\
Specify any of the following keywords for detailed info:\n\
  mappings -- list of mapped memory regions.\n\
  stat     -- list a bunch of random process info.\n\
  status   -- list a different bunch of random process info.\n\
  all      -- list all available /proc info."));

  add_setshow_zinteger_cmd ("lin-lwp", class_maintenance,
			    &debug_linux_nat, _("\
Set debugging of GNU/Linux lwp module."), _("\
Show debugging of GNU/Linux lwp module."), _("\
Enables printf debugging output."),
			    NULL,
			    show_debug_linux_nat,
			    &setdebuglist, &showdebuglist);

  add_setshow_zinteger_cmd ("lin-lwp-async", class_maintenance,
			    &debug_linux_nat_async, _("\
Set debugging of GNU/Linux async lwp module."), _("\
Show debugging of GNU/Linux async lwp module."), _("\
Enables printf debugging output."),
			    NULL,
			    show_debug_linux_nat_async,
			    &setdebuglist, &showdebuglist);

  /* Save this mask as the default.  */
  sigprocmask (SIG_SETMASK, NULL, &normal_mask);

  /* Install a SIGCHLD handler.  */
  sigchld_action.sa_handler = sigchld_handler;
  sigemptyset (&sigchld_action.sa_mask);
  sigchld_action.sa_flags = SA_RESTART;

  /* Make it the default.  */
  sigaction (SIGCHLD, &sigchld_action, NULL);

  /* Make sure we don't block SIGCHLD during a sigsuspend.  */
  sigprocmask (SIG_SETMASK, NULL, &suspend_mask);
  sigdelset (&suspend_mask, SIGCHLD);

  sigemptyset (&blocked_mask);

  add_setshow_boolean_cmd ("disable-randomization", class_support,
			   &disable_randomization, _("\
Set disabling of debuggee's virtual address space randomization."), _("\
Show disabling of debuggee's virtual address space randomization."), _("\
When this mode is on (which is the default), randomization of the virtual\n\
address space is disabled.  Standalone programs run with the randomization\n\
enabled by default on some platforms."),
			   &set_disable_randomization,
			   &show_disable_randomization,
			   &setlist, &showlist);
}


/* FIXME: kettenis/2000-08-26: The stuff on this page is specific to
   the GNU/Linux Threads library and therefore doesn't really belong
   here.  */

/* Read variable NAME in the target and return its value if found.
   Otherwise return zero.  It is assumed that the type of the variable
   is `int'.  */

static int
get_signo (const char *name)
{
  struct minimal_symbol *ms;
  int signo;

  ms = lookup_minimal_symbol (name, NULL, NULL);
  if (ms == NULL)
    return 0;

  if (target_read_memory (SYMBOL_VALUE_ADDRESS (ms), (gdb_byte *) &signo,
			  sizeof (signo)) != 0)
    return 0;

  return signo;
}

/* Return the set of signals used by the threads library in *SET.  */

void
lin_thread_get_thread_signals (sigset_t *set)
{
  struct sigaction action;
  int restart, cancel;

  sigemptyset (&blocked_mask);
  sigemptyset (set);

  restart = get_signo ("__pthread_sig_restart");
  cancel = get_signo ("__pthread_sig_cancel");

  /* LinuxThreads normally uses the first two RT signals, but in some legacy
     cases may use SIGUSR1/SIGUSR2.  NPTL always uses RT signals, but does
     not provide any way for the debugger to query the signal numbers -
     fortunately they don't change!  */

  if (restart == 0)
    restart = __SIGRTMIN;

  if (cancel == 0)
    cancel = __SIGRTMIN + 1;

  sigaddset (set, restart);
  sigaddset (set, cancel);

  /* The GNU/Linux Threads library makes terminating threads send a
     special "cancel" signal instead of SIGCHLD.  Make sure we catch
     those (to prevent them from terminating GDB itself, which is
     likely to be their default action) and treat them the same way as
     SIGCHLD.  */

  action.sa_handler = sigchld_handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = SA_RESTART;
  sigaction (cancel, &action, NULL);

  /* We block the "cancel" signal throughout this code ...  */
  sigaddset (&blocked_mask, cancel);
  sigprocmask (SIG_BLOCK, &blocked_mask, NULL);

  /* ... except during a sigsuspend.  */
  sigdelset (&suspend_mask, cancel);
}
