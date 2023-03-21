/* GNU/Linux native-dependent code common to multiple platforms.

   Copyright (C) 2001-2020 Free Software Foundation, Inc.

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
#include "infrun.h"
#include "target.h"
#include "nat/linux-nat.h"
#include "nat/linux-waitpid.h"
#include "gdbsupport/gdb_wait.h"
#include <unistd.h>
#include <sys/syscall.h>
#include "nat/gdb_ptrace.h"
#include "linux-nat.h"
#include "nat/linux-ptrace.h"
#include "nat/linux-procfs.h"
#include "nat/linux-personality.h"
#include "linux-fork.h"
#include "gdbthread.h"
#include "gdbcmd.h"
#include "regcache.h"
#include "regset.h"
#include "inf-child.h"
#include "inf-ptrace.h"
#include "auxv.h"
#include <sys/procfs.h>		/* for elf_gregset etc.  */
#include "elf-bfd.h"		/* for elfcore_write_* */
#include "gregset.h"		/* for gregset */
#include "gdbcore.h"		/* for get_exec_file */
#include <ctype.h>		/* for isdigit */
#include <sys/stat.h>		/* for struct stat */
#include <fcntl.h>		/* for O_RDONLY */
#include "inf-loop.h"
#include "gdbsupport/event-loop.h"
#include "event-top.h"
#include <pwd.h>
#include <sys/types.h>
#include <dirent.h>
#include "xml-support.h"
#include <sys/vfs.h>
#include "solib.h"
#include "nat/linux-osdata.h"
#include "linux-tdep.h"
#include "symfile.h"
#include "gdbsupport/agent.h"
#include "tracepoint.h"
#include "gdbsupport/buffer.h"
#include "target-descriptions.h"
#include "gdbsupport/filestuff.h"
#include "objfiles.h"
#include "nat/linux-namespaces.h"
#include "gdbsupport/fileio.h"
#include "gdbsupport/scope-exit.h"
#include "gdbsupport/gdb-sigmask.h"
#include "debug.h"

/* This comment documents high-level logic of this file.

Waiting for events in sync mode
===============================

When waiting for an event in a specific thread, we just use waitpid,
passing the specific pid, and not passing WNOHANG.

When waiting for an event in all threads, waitpid is not quite good:

- If the thread group leader exits while other threads in the thread
  group still exist, waitpid(TGID, ...) hangs.  That waitpid won't
  return an exit status until the other threads in the group are
  reaped.

- When a non-leader thread execs, that thread just vanishes without
  reporting an exit (so we'd hang if we waited for it explicitly in
  that case).  The exec event is instead reported to the TGID pid.

The solution is to always use -1 and WNOHANG, together with
sigsuspend.

First, we use non-blocking waitpid to check for events.  If nothing is
found, we use sigsuspend to wait for SIGCHLD.  When SIGCHLD arrives,
it means something happened to a child process.  As soon as we know
there's an event, we get back to calling nonblocking waitpid.

Note that SIGCHLD should be blocked between waitpid and sigsuspend
calls, so that we don't miss a signal.  If SIGCHLD arrives in between,
when it's blocked, the signal becomes pending and sigsuspend
immediately notices it and returns.

Waiting for events in async mode (TARGET_WNOHANG)
=================================================

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
blocked.

Exec events
===========

The case of a thread group (process) with 3 or more threads, and a
thread other than the leader execs is worth detailing:

On an exec, the Linux kernel destroys all threads except the execing
one in the thread group, and resets the execing thread's tid to the
tgid.  No exit notification is sent for the execing thread -- from the
ptracer's perspective, it appears as though the execing thread just
vanishes.  Until we reap all other threads except the leader and the
execing thread, the leader will be zombie, and the execing thread will
be in `D (disc sleep)' state.  As soon as all other threads are
reaped, the execing thread changes its tid to the tgid, and the
previous (zombie) leader vanishes, giving place to the "new"
leader.  */

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

struct linux_nat_target *linux_target;

/* Does the current host support PTRACE_GETREGSET?  */
enum tribool have_ptrace_getregset = TRIBOOL_UNKNOWN;

static unsigned int debug_linux_nat;
static void
show_debug_linux_nat (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Debugging of GNU/Linux lwp module is %s.\n"),
		    value);
}

/* Print a debug statement.  Should be used through linux_nat_debug_printf.  */

static void ATTRIBUTE_PRINTF (2, 3)
linux_nat_debug_printf_1 (const char *func_name, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  debug_prefixed_vprintf ("linux-nat", func_name, fmt, ap);
  va_end (ap);
}

#define linux_nat_debug_printf(fmt, ...) \
  do { \
    if (debug_linux_nat) \
      linux_nat_debug_printf_1 (__func__, fmt, ##__VA_ARGS__); \
  } while (0)

struct simple_pid_list
{
  int pid;
  int status;
  struct simple_pid_list *next;
};
static struct simple_pid_list *stopped_pids;

/* Whether target_thread_events is in effect.  */
static int report_thread_events;

/* Async mode support.  */

/* The read/write ends of the pipe registered as waitable file in the
   event loop.  */
static int linux_nat_event_pipe[2] = { -1, -1 };

/* True if we're currently in async mode.  */
#define linux_is_async_p() (linux_nat_event_pipe[0] != -1)

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

static int kill_lwp (int lwpid, int signo);

static int stop_callback (struct lwp_info *lp);

static void block_child_signals (sigset_t *prev_mask);
static void restore_child_signals_mask (sigset_t *prev_mask);

struct lwp_info;
static struct lwp_info *add_lwp (ptid_t ptid);
static void purge_lwp_list (int pid);
static void delete_lwp (ptid_t ptid);
static struct lwp_info *find_lwp_pid (ptid_t ptid);

static int lwp_status_pending_p (struct lwp_info *lp);

static void save_stop_reason (struct lwp_info *lp);


/* LWP accessors.  */

/* See nat/linux-nat.h.  */

ptid_t
ptid_of_lwp (struct lwp_info *lwp)
{
  return lwp->ptid;
}

/* See nat/linux-nat.h.  */

void
lwp_set_arch_private_info (struct lwp_info *lwp,
			   struct arch_lwp_info *info)
{
  lwp->arch_private = info;
}

/* See nat/linux-nat.h.  */

struct arch_lwp_info *
lwp_arch_private_info (struct lwp_info *lwp)
{
  return lwp->arch_private;
}

/* See nat/linux-nat.h.  */

int
lwp_is_stopped (struct lwp_info *lwp)
{
  return lwp->stopped;
}

/* See nat/linux-nat.h.  */

enum target_stop_reason
lwp_stop_reason (struct lwp_info *lwp)
{
  return lwp->stop_reason;
}

/* See nat/linux-nat.h.  */

int
lwp_is_stepping (struct lwp_info *lwp)
{
  return lwp->step;
}


/* Trivial list manipulation functions to keep track of a list of
   new stopped processes.  */
static void
add_to_pid_list (struct simple_pid_list **listp, int pid, int status)
{
  struct simple_pid_list *new_pid = XNEW (struct simple_pid_list);

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

/* Return the ptrace options that we want to try to enable.  */

static int
linux_nat_ptrace_options (int attached)
{
  int options = 0;

  if (!attached)
    options |= PTRACE_O_EXITKILL;

  options |= (PTRACE_O_TRACESYSGOOD
	      | PTRACE_O_TRACEVFORKDONE
	      | PTRACE_O_TRACEVFORK
	      | PTRACE_O_TRACEFORK
	      | PTRACE_O_TRACEEXEC);

  return options;
}

/* Initialize ptrace and procfs warnings and check for supported
   ptrace features given PID.

   ATTACHED should be nonzero iff we attached to the inferior.  */

static void
linux_init_ptrace_procfs (pid_t pid, int attached)
{
  int options = linux_nat_ptrace_options (attached);

  linux_enable_event_reporting (pid, options);
  linux_ptrace_init_warnings ();
  linux_proc_init_warnings ();
}

linux_nat_target::~linux_nat_target ()
{}

void
linux_nat_target::post_attach (int pid)
{
  linux_init_ptrace_procfs (pid, 1);
}

void
linux_nat_target::post_startup_inferior (ptid_t ptid)
{
  linux_init_ptrace_procfs (ptid.pid (), 0);
}

/* Return the number of known LWPs in the tgid given by PID.  */

static int
num_lwps (int pid)
{
  int count = 0;
  struct lwp_info *lp;

  for (lp = lwp_list; lp; lp = lp->next)
    if (lp->ptid.pid () == pid)
      count++;

  return count;
}

/* Deleter for lwp_info unique_ptr specialisation.  */

struct lwp_deleter
{
  void operator() (struct lwp_info *lwp) const
  {
    delete_lwp (lwp->ptid);
  }
};

/* A unique_ptr specialisation for lwp_info.  */

typedef std::unique_ptr<struct lwp_info, lwp_deleter> lwp_info_up;

/* Target hook for follow_fork.  On entry inferior_ptid must be the
   ptid of the followed inferior.  At return, inferior_ptid will be
   unchanged.  */

bool
linux_nat_target::follow_fork (bool follow_child, bool detach_fork)
{
  if (!follow_child)
    {
      struct lwp_info *child_lp = NULL;
      int has_vforked;
      ptid_t parent_ptid, child_ptid;
      int parent_pid, child_pid;

      has_vforked = (inferior_thread ()->pending_follow.kind
		     == TARGET_WAITKIND_VFORKED);
      parent_ptid = inferior_ptid;
      child_ptid = inferior_thread ()->pending_follow.value.related_pid;
      parent_pid = parent_ptid.lwp ();
      child_pid = child_ptid.lwp ();

      /* We're already attached to the parent, by default.  */
      child_lp = add_lwp (child_ptid);
      child_lp->stopped = 1;
      child_lp->last_resume_kind = resume_stop;

      /* Detach new forked process?  */
      if (detach_fork)
	{
	  int child_stop_signal = 0;
	  bool detach_child = true;

	  /* Move CHILD_LP into a unique_ptr and clear the source pointer
	     to prevent us doing anything stupid with it.  */
	  lwp_info_up child_lp_ptr (child_lp);
	  child_lp = nullptr;

	  linux_target->low_prepare_to_resume (child_lp_ptr.get ());

	  /* When debugging an inferior in an architecture that supports
	     hardware single stepping on a kernel without commit
	     6580807da14c423f0d0a708108e6df6ebc8bc83d, the vfork child
	     process starts with the TIF_SINGLESTEP/X86_EFLAGS_TF bits
	     set if the parent process had them set.
	     To work around this, single step the child process
	     once before detaching to clear the flags.  */

	  /* Note that we consult the parent's architecture instead of
	     the child's because there's no inferior for the child at
	     this point.  */
	  if (!gdbarch_software_single_step_p (target_thread_architecture
					       (parent_ptid)))
	    {
	      int status;

	      linux_disable_event_reporting (child_pid);
	      if (ptrace (PTRACE_SINGLESTEP, child_pid, 0, 0) < 0)
		perror_with_name (_("Couldn't do single step"));
	      if (my_waitpid (child_pid, &status, 0) < 0)
		perror_with_name (_("Couldn't wait vfork process"));
	      else
		{
		  detach_child = WIFSTOPPED (status);
		  child_stop_signal = WSTOPSIG (status);
		}
	    }

	  if (detach_child)
	    {
	      int signo = child_stop_signal;

	      if (signo != 0
		  && !signal_pass_state (gdb_signal_from_host (signo)))
		signo = 0;
	      ptrace (PTRACE_DETACH, child_pid, 0, signo);
	    }
	}
      else
	{
	  /* Switching inferior_ptid is not enough, because then
	     inferior_thread () would crash by not finding the thread
	     in the current inferior.  */
	  scoped_restore_current_thread restore_current_thread;
	  thread_info *child = find_thread_ptid (this, child_ptid);
	  switch_to_thread (child);

	  /* Let the thread_db layer learn about this new process.  */
	  check_for_thread_db ();
	}

      if (has_vforked)
	{
	  struct lwp_info *parent_lp;

	  parent_lp = find_lwp_pid (parent_ptid);
	  gdb_assert (linux_supports_tracefork () >= 0);

	  if (linux_supports_tracevforkdone ())
	    {
	      linux_nat_debug_printf ("waiting for VFORK_DONE on %d",
				      parent_pid);
	      parent_lp->stopped = 1;

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

	      linux_nat_debug_printf ("no VFORK_DONE support, sleeping a bit");

	      usleep (10000);

	      /* Pretend we've seen a PTRACE_EVENT_VFORK_DONE event,
		 and leave it pending.  The next linux_nat_resume call
		 will notice a pending event, and bypasses actually
		 resuming the inferior.  */
	      parent_lp->status = 0;
	      parent_lp->waitstatus.kind = TARGET_WAITKIND_VFORK_DONE;
	      parent_lp->stopped = 1;

	      /* If we're in async mode, need to tell the event loop
		 there's something here to process.  */
	      if (target_is_async_p ())
		async_file_mark ();
	    }
	}
    }
  else
    {
      struct lwp_info *child_lp;

      child_lp = add_lwp (inferior_ptid);
      child_lp->stopped = 1;
      child_lp->last_resume_kind = resume_stop;

      /* Let the thread_db layer learn about this new process.  */
      check_for_thread_db ();
    }

  return false;
}


int
linux_nat_target::insert_fork_catchpoint (int pid)
{
  return !linux_supports_tracefork ();
}

int
linux_nat_target::remove_fork_catchpoint (int pid)
{
  return 0;
}

int
linux_nat_target::insert_vfork_catchpoint (int pid)
{
  return !linux_supports_tracefork ();
}

int
linux_nat_target::remove_vfork_catchpoint (int pid)
{
  return 0;
}

int
linux_nat_target::insert_exec_catchpoint (int pid)
{
  return !linux_supports_tracefork ();
}

int
linux_nat_target::remove_exec_catchpoint (int pid)
{
  return 0;
}

int
linux_nat_target::set_syscall_catchpoint (int pid, bool needed, int any_count,
					  gdb::array_view<const int> syscall_counts)
{
  if (!linux_supports_tracesysgood ())
    return 1;

  /* On GNU/Linux, we ignore the arguments.  It means that we only
     enable the syscall catchpoints, but do not disable them.

     Also, we do not use the `syscall_counts' information because we do not
     filter system calls here.  We let GDB do the logic for us.  */
  return 0;
}

/* List of known LWPs, keyed by LWP PID.  This speeds up the common
   case of mapping a PID returned from the kernel to our corresponding
   lwp_info data structure.  */
static htab_t lwp_lwpid_htab;

/* Calculate a hash from a lwp_info's LWP PID.  */

static hashval_t
lwp_info_hash (const void *ap)
{
  const struct lwp_info *lp = (struct lwp_info *) ap;
  pid_t pid = lp->ptid.lwp ();

  return iterative_hash_object (pid, 0);
}

/* Equality function for the lwp_info hash table.  Compares the LWP's
   PID.  */

static int
lwp_lwpid_htab_eq (const void *a, const void *b)
{
  const struct lwp_info *entry = (const struct lwp_info *) a;
  const struct lwp_info *element = (const struct lwp_info *) b;

  return entry->ptid.lwp () == element->ptid.lwp ();
}

/* Create the lwp_lwpid_htab hash table.  */

static void
lwp_lwpid_htab_create (void)
{
  lwp_lwpid_htab = htab_create (100, lwp_info_hash, lwp_lwpid_htab_eq, NULL);
}

/* Add LP to the hash table.  */

static void
lwp_lwpid_htab_add_lwp (struct lwp_info *lp)
{
  void **slot;

  slot = htab_find_slot (lwp_lwpid_htab, lp, INSERT);
  gdb_assert (slot != NULL && *slot == NULL);
  *slot = lp;
}

/* Head of doubly-linked list of known LWPs.  Sorted by reverse
   creation order.  This order is assumed in some cases.  E.g.,
   reaping status after killing alls lwps of a process: the leader LWP
   must be reaped last.  */
struct lwp_info *lwp_list;

/* Add LP to sorted-by-reverse-creation-order doubly-linked list.  */

static void
lwp_list_add (struct lwp_info *lp)
{
  lp->next = lwp_list;
  if (lwp_list != NULL)
    lwp_list->prev = lp;
  lwp_list = lp;
}

/* Remove LP from sorted-by-reverse-creation-order doubly-linked
   list.  */

static void
lwp_list_remove (struct lwp_info *lp)
{
  /* Remove from sorted-by-creation-order list.  */
  if (lp->next != NULL)
    lp->next->prev = lp->prev;
  if (lp->prev != NULL)
    lp->prev->next = lp->next;
  if (lp == lwp_list)
    lwp_list = lp->next;
}



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

  gdb_sigmask (SIG_BLOCK, &blocked_mask, prev_mask);
}

/* Restore child signals mask, previously returned by
   block_child_signals.  */

static void
restore_child_signals_mask (sigset_t *prev_mask)
{
  gdb_sigmask (SIG_SETMASK, prev_mask, NULL);
}

/* Mask of signals to pass directly to the inferior.  */
static sigset_t pass_mask;

/* Update signals to pass to the inferior.  */
void
linux_nat_target::pass_signals
  (gdb::array_view<const unsigned char> pass_signals)
{
  int signo;

  sigemptyset (&pass_mask);

  for (signo = 1; signo < NSIG; signo++)
    {
      int target_signo = gdb_signal_from_host (signo);
      if (target_signo < pass_signals.size () && pass_signals[target_signo])
        sigaddset (&pass_mask, signo);
    }
}



/* Prototypes for local functions.  */
static int stop_wait_callback (struct lwp_info *lp);
static int resume_stopped_resumed_lwps (struct lwp_info *lp, const ptid_t wait_ptid);
static int check_ptrace_stopped_lwp_gone (struct lwp_info *lp);



/* Destroy and free LP.  */

static void
lwp_free (struct lwp_info *lp)
{
  /* Let the arch specific bits release arch_lwp_info.  */
  linux_target->low_delete_thread (lp->arch_private);

  xfree (lp);
}

/* Traversal function for purge_lwp_list.  */

static int
lwp_lwpid_htab_remove_pid (void **slot, void *info)
{
  struct lwp_info *lp = (struct lwp_info *) *slot;
  int pid = *(int *) info;

  if (lp->ptid.pid () == pid)
    {
      htab_clear_slot (lwp_lwpid_htab, slot);
      lwp_list_remove (lp);
      lwp_free (lp);
    }

  return 1;
}

/* Remove all LWPs belong to PID from the lwp list.  */

static void
purge_lwp_list (int pid)
{
  htab_traverse_noresize (lwp_lwpid_htab, lwp_lwpid_htab_remove_pid, &pid);
}

/* Add the LWP specified by PTID to the list.  PTID is the first LWP
   in the process.  Return a pointer to the structure describing the
   new LWP.

   This differs from add_lwp in that we don't let the arch specific
   bits know about this new thread.  Current clients of this callback
   take the opportunity to install watchpoints in the new thread, and
   we shouldn't do that for the first thread.  If we're spawning a
   child ("run"), the thread executes the shell wrapper first, and we
   shouldn't touch it until it execs the program we want to debug.
   For "attach", it'd be okay to call the callback, but it's not
   necessary, because watchpoints can't yet have been inserted into
   the inferior.  */

static struct lwp_info *
add_initial_lwp (ptid_t ptid)
{
  struct lwp_info *lp;

  gdb_assert (ptid.lwp_p ());

  lp = XNEW (struct lwp_info);

  memset (lp, 0, sizeof (struct lwp_info));

  lp->last_resume_kind = resume_continue;
  lp->waitstatus.kind = TARGET_WAITKIND_IGNORE;

  lp->ptid = ptid;
  lp->core = -1;

  /* Add to sorted-by-reverse-creation-order list.  */
  lwp_list_add (lp);

  /* Add to keyed-by-pid htab.  */
  lwp_lwpid_htab_add_lwp (lp);

  return lp;
}

/* Add the LWP specified by PID to the list.  Return a pointer to the
   structure describing the new LWP.  The LWP should already be
   stopped.  */

static struct lwp_info *
add_lwp (ptid_t ptid)
{
  struct lwp_info *lp;

  lp = add_initial_lwp (ptid);

  /* Let the arch specific bits know about this new thread.  Current
     clients of this callback take the opportunity to install
     watchpoints in the new thread.  We don't do this for the first
     thread though.  See add_initial_lwp.  */
  linux_target->low_new_thread (lp);

  return lp;
}

/* Remove the LWP specified by PID from the list.  */

static void
delete_lwp (ptid_t ptid)
{
  struct lwp_info *lp;
  void **slot;
  struct lwp_info dummy;

  dummy.ptid = ptid;
  slot = htab_find_slot (lwp_lwpid_htab, &dummy, NO_INSERT);
  if (slot == NULL)
    return;

  lp = *(struct lwp_info **) slot;
  gdb_assert (lp != NULL);

  htab_clear_slot (lwp_lwpid_htab, slot);

  /* Remove from sorted-by-creation-order list.  */
  lwp_list_remove (lp);

  /* Release.  */
  lwp_free (lp);
}

/* Return a pointer to the structure describing the LWP corresponding
   to PID.  If no corresponding LWP could be found, return NULL.  */

static struct lwp_info *
find_lwp_pid (ptid_t ptid)
{
  struct lwp_info *lp;
  int lwp;
  struct lwp_info dummy;

  if (ptid.lwp_p ())
    lwp = ptid.lwp ();
  else
    lwp = ptid.pid ();

  dummy.ptid = ptid_t (0, lwp, 0);
  lp = (struct lwp_info *) htab_find (lwp_lwpid_htab, &dummy);
  return lp;
}

/* See nat/linux-nat.h.  */

struct lwp_info *
iterate_over_lwps (ptid_t filter,
		   gdb::function_view<iterate_over_lwps_ftype> callback)
{
  struct lwp_info *lp, *lpnext;

  for (lp = lwp_list; lp; lp = lpnext)
    {
      lpnext = lp->next;

      if (lp->ptid.matches (filter))
	{
	  if (callback (lp) != 0)
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

  purge_lwp_list (inferior_ptid.pid ());

  lp = add_lwp (new_ptid);
  lp->stopped = 1;

  /* This changes the thread's ptid while preserving the gdb thread
     num.  Also changes the inferior pid, while preserving the
     inferior num.  */
  thread_change_ptid (linux_target, inferior_ptid, new_ptid);

  /* We've just told GDB core that the thread changed target id, but,
     in fact, it really is a different thread, with different register
     contents.  */
  registers_changed ();
}

/* Handle the exit of a single thread LP.  */

static void
exit_lwp (struct lwp_info *lp)
{
  struct thread_info *th = find_thread_ptid (linux_target, lp->ptid);

  if (th)
    {
      if (print_thread_events)
	printf_unfiltered (_("[%s exited]\n"),
			   target_pid_to_str (lp->ptid).c_str ());

      delete_thread (th);
    }

  delete_lwp (lp->ptid);
}

/* Wait for the LWP specified by LP, which we have just attached to.
   Returns a wait status for that LWP, to cache.  */

static int
linux_nat_post_attach_wait (ptid_t ptid, int *signalled)
{
  pid_t new_pid, pid = ptid.lwp ();
  int status;

  if (linux_proc_pid_is_stopped (pid))
    {
      linux_nat_debug_printf ("Attaching to a stopped process");

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
  new_pid = my_waitpid (pid, &status, __WALL);
  gdb_assert (pid == new_pid);

  if (!WIFSTOPPED (status))
    {
      /* The pid we tried to attach has apparently just exited.  */
      linux_nat_debug_printf ("Failed to stop %d: %s", pid,
			      status_to_str (status));
      return status;
    }

  if (WSTOPSIG (status) != SIGSTOP)
    {
      *signalled = 1;
      linux_nat_debug_printf ("Received %s after attaching",
			      status_to_str (status));
    }

  return status;
}

void
linux_nat_target::create_inferior (const char *exec_file,
				   const std::string &allargs,
				   char **env, int from_tty)
{
  maybe_disable_address_space_randomization restore_personality
    (disable_randomization);

  /* The fork_child mechanism is synchronous and calls target_wait, so
     we have to mask the async mode.  */

  /* Make sure we report all signals during startup.  */
  pass_signals ({});

  inf_ptrace_target::create_inferior (exec_file, allargs, env, from_tty);
}

/* Callback for linux_proc_attach_tgid_threads.  Attach to PTID if not
   already attached.  Returns true if a new LWP is found, false
   otherwise.  */

static int
attach_proc_task_lwp_callback (ptid_t ptid)
{
  struct lwp_info *lp;

  /* Ignore LWPs we're already attached to.  */
  lp = find_lwp_pid (ptid);
  if (lp == NULL)
    {
      int lwpid = ptid.lwp ();

      if (ptrace (PTRACE_ATTACH, lwpid, 0, 0) < 0)
	{
	  int err = errno;

	  /* Be quiet if we simply raced with the thread exiting.
	     EPERM is returned if the thread's task still exists, and
	     is marked as exited or zombie, as well as other
	     conditions, so in that case, confirm the status in
	     /proc/PID/status.  */
	  if (err == ESRCH
	      || (err == EPERM && linux_proc_pid_is_gone (lwpid)))
	    {
	      linux_nat_debug_printf
		("Cannot attach to lwp %d: thread is gone (%d: %s)",
		 lwpid, err, safe_strerror (err));

	    }
	  else
	    {
	      std::string reason
		= linux_ptrace_attach_fail_reason_string (ptid, err);

	      warning (_("Cannot attach to lwp %d: %s"),
		       lwpid, reason.c_str ());
	    }
	}
      else
	{
	  linux_nat_debug_printf ("PTRACE_ATTACH %s, 0, 0 (OK)",
				  target_pid_to_str (ptid).c_str ());

	  lp = add_lwp (ptid);

	  /* The next time we wait for this LWP we'll see a SIGSTOP as
	     PTRACE_ATTACH brings it to a halt.  */
	  lp->signalled = 1;

	  /* We need to wait for a stop before being able to make the
	     next ptrace call on this LWP.  */
	  lp->must_set_ptrace_flags = 1;

	  /* So that wait collects the SIGSTOP.  */
	  lp->resumed = 1;

	  /* Also add the LWP to gdb's thread list, in case a
	     matching libthread_db is not found (or the process uses
	     raw clone).  */
	  add_thread (linux_target, lp->ptid);
	  set_running (linux_target, lp->ptid, true);
	  set_executing (linux_target, lp->ptid, true);
	}

      return 1;
    }
  return 0;
}

void
linux_nat_target::attach (const char *args, int from_tty)
{
  struct lwp_info *lp;
  int status;
  ptid_t ptid;

  /* Make sure we report all signals during attach.  */
  pass_signals ({});

  try
    {
      inf_ptrace_target::attach (args, from_tty);
    }
  catch (const gdb_exception_error &ex)
    {
      pid_t pid = parse_pid_to_attach (args);
      std::string reason = linux_ptrace_attach_fail_reason (pid);

      if (!reason.empty ())
	throw_error (ex.error, "warning: %s\n%s", reason.c_str (),
		     ex.what ());
      else
	throw_error (ex.error, "%s", ex.what ());
    }

  /* The ptrace base target adds the main thread with (pid,0,0)
     format.  Decorate it with lwp info.  */
  ptid = ptid_t (inferior_ptid.pid (),
		 inferior_ptid.pid (),
		 0);
  thread_change_ptid (linux_target, inferior_ptid, ptid);

  /* Add the initial process as the first LWP to the list.  */
  lp = add_initial_lwp (ptid);

  status = linux_nat_post_attach_wait (lp->ptid, &lp->signalled);
  if (!WIFSTOPPED (status))
    {
      if (WIFEXITED (status))
	{
	  int exit_code = WEXITSTATUS (status);

	  target_terminal::ours ();
	  target_mourn_inferior (inferior_ptid);
	  if (exit_code == 0)
	    error (_("Unable to attach: program exited normally."));
	  else
	    error (_("Unable to attach: program exited with code %d."),
		   exit_code);
	}
      else if (WIFSIGNALED (status))
	{
	  enum gdb_signal signo;

	  target_terminal::ours ();
	  target_mourn_inferior (inferior_ptid);

	  signo = gdb_signal_from_host (WTERMSIG (status));
	  error (_("Unable to attach: program terminated with signal "
		   "%s, %s."),
		 gdb_signal_to_name (signo),
		 gdb_signal_to_string (signo));
	}

      internal_error (__FILE__, __LINE__,
		      _("unexpected status %d for PID %ld"),
		      status, (long) ptid.lwp ());
    }

  lp->stopped = 1;

  /* Save the wait status to report later.  */
  lp->resumed = 1;
  linux_nat_debug_printf ("waitpid %ld, saving status %s",
			  (long) lp->ptid.pid (), status_to_str (status));

  lp->status = status;

  /* We must attach to every LWP.  If /proc is mounted, use that to
     find them now.  The inferior may be using raw clone instead of
     using pthreads.  But even if it is using pthreads, thread_db
     walks structures in the inferior's address space to find the list
     of threads/LWPs, and those structures may well be corrupted.
     Note that once thread_db is loaded, we'll still use it to list
     threads and associate pthread info with each LWP.  */
  linux_proc_attach_tgid_threads (lp->ptid.pid (),
				  attach_proc_task_lwp_callback);

  if (target_can_async_p ())
    target_async (1);
}

/* Get pending signal of THREAD as a host signal number, for detaching
   purposes.  This is the signal the thread last stopped for, which we
   need to deliver to the thread when detaching, otherwise, it'd be
   suppressed/lost.  */

static int
get_detach_signal (struct lwp_info *lp)
{
  enum gdb_signal signo = GDB_SIGNAL_0;

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
     to GDB_SIGNAL_SIGTRAP when the catchpoint triggers.  But,
     those traps are debug API (ptrace in our case) related and
     induced; the inferior wouldn't see them if it wasn't being
     traced.  Hence, we should never pass them to the inferior, even
     when set to pass state.  Since this corner case isn't handled by
     infrun.c when proceeding with a signal, for consistency, neither
     do we handle it here (or elsewhere in the file we check for
     signal pass state).  Normally SIGTRAP isn't set to pass state, so
     this is really a corner case.  */

  if (lp->waitstatus.kind != TARGET_WAITKIND_IGNORE)
    signo = GDB_SIGNAL_0; /* a pending ptrace event, not a real signal.  */
  else if (lp->status)
    signo = gdb_signal_from_host (WSTOPSIG (lp->status));
  else
    {
      struct thread_info *tp = find_thread_ptid (linux_target, lp->ptid);

      if (target_is_non_stop_p () && !tp->executing)
	{
	  if (tp->suspend.waitstatus_pending_p)
	    signo = tp->suspend.waitstatus.value.sig;
	  else
	    signo = tp->suspend.stop_signal;
	}
      else if (!target_is_non_stop_p ())
	{
	  ptid_t last_ptid;
	  process_stratum_target *last_target;

	  get_last_target_status (&last_target, &last_ptid, nullptr);

	  if (last_target == linux_target
	      && lp->ptid.lwp () == last_ptid.lwp ())
	    signo = tp->suspend.stop_signal;
	}
    }

  if (signo == GDB_SIGNAL_0)
    {
      linux_nat_debug_printf ("lwp %s has no pending signal",
			      target_pid_to_str (lp->ptid).c_str ());
    }
  else if (!signal_pass_state (signo))
    {
      linux_nat_debug_printf
	("lwp %s had signal %s but it is in no pass state",
	 target_pid_to_str (lp->ptid).c_str (), gdb_signal_to_string (signo));
    }
  else
    {
      linux_nat_debug_printf ("lwp %s has pending signal %s",
			      target_pid_to_str (lp->ptid).c_str (),
			      gdb_signal_to_string (signo));

      return gdb_signal_to_host (signo);
    }

  return 0;
}

/* Detach from LP.  If SIGNO_P is non-NULL, then it points to the
   signal number that should be passed to the LWP when detaching.
   Otherwise pass any pending signal the LWP may have, if any.  */

static void
detach_one_lwp (struct lwp_info *lp, int *signo_p)
{
  int lwpid = lp->ptid.lwp ();
  int signo;

  gdb_assert (lp->status == 0 || WIFSTOPPED (lp->status));

  if (lp->status != 0)
    linux_nat_debug_printf ("Pending %s for %s on detach.",
			    strsignal (WSTOPSIG (lp->status)),
			    target_pid_to_str (lp->ptid).c_str ());

  /* If there is a pending SIGSTOP, get rid of it.  */
  if (lp->signalled)
    {
      linux_nat_debug_printf ("Sending SIGCONT to %s",
			      target_pid_to_str (lp->ptid).c_str ());

      kill_lwp (lwpid, SIGCONT);
      lp->signalled = 0;
    }

  if (signo_p == NULL)
    {
      /* Pass on any pending signal for this LWP.  */
      signo = get_detach_signal (lp);
    }
  else
    signo = *signo_p;

  /* Preparing to resume may try to write registers, and fail if the
     lwp is zombie.  If that happens, ignore the error.  We'll handle
     it below, when detach fails with ESRCH.  */
  try
    {
      linux_target->low_prepare_to_resume (lp);
    }
  catch (const gdb_exception_error &ex)
    {
      if (!check_ptrace_stopped_lwp_gone (lp))
	throw;
    }

  if (ptrace (PTRACE_DETACH, lwpid, 0, signo) < 0)
    {
      int save_errno = errno;

      /* We know the thread exists, so ESRCH must mean the lwp is
	 zombie.  This can happen if one of the already-detached
	 threads exits the whole thread group.  In that case we're
	 still attached, and must reap the lwp.  */
      if (save_errno == ESRCH)
	{
	  int ret, status;

	  ret = my_waitpid (lwpid, &status, __WALL);
	  if (ret == -1)
	    {
	      warning (_("Couldn't reap LWP %d while detaching: %s"),
		       lwpid, safe_strerror (errno));
	    }
	  else if (!WIFEXITED (status) && !WIFSIGNALED (status))
	    {
	      warning (_("Reaping LWP %d while detaching "
			 "returned unexpected status 0x%x"),
		       lwpid, status);
	    }
	}
      else
	{
	  error (_("Can't detach %s: %s"),
		 target_pid_to_str (lp->ptid).c_str (),
		 safe_strerror (save_errno));
	}
    }
  else
    linux_nat_debug_printf ("PTRACE_DETACH (%s, %s, 0) (OK)",
			    target_pid_to_str (lp->ptid).c_str (),
			    strsignal (signo));

  delete_lwp (lp->ptid);
}

static int
detach_callback (struct lwp_info *lp)
{
  /* We don't actually detach from the thread group leader just yet.
     If the thread group exits, we must reap the zombie clone lwps
     before we're able to reap the leader.  */
  if (lp->ptid.lwp () != lp->ptid.pid ())
    detach_one_lwp (lp, NULL);
  return 0;
}

void
linux_nat_target::detach (inferior *inf, int from_tty)
{
  struct lwp_info *main_lwp;
  int pid = inf->pid;

  /* Don't unregister from the event loop, as there may be other
     inferiors running. */

  /* Stop all threads before detaching.  ptrace requires that the
     thread is stopped to successfully detach.  */
  iterate_over_lwps (ptid_t (pid), stop_callback);
  /* ... and wait until all of them have reported back that
     they're no longer running.  */
  iterate_over_lwps (ptid_t (pid), stop_wait_callback);

  iterate_over_lwps (ptid_t (pid), detach_callback);

  /* Only the initial process should be left right now.  */
  gdb_assert (num_lwps (pid) == 1);

  main_lwp = find_lwp_pid (ptid_t (pid));

  if (forks_exist_p ())
    {
      /* Multi-fork case.  The current inferior_ptid is being detached
	 from, but there are other viable forks to debug.  Detach from
	 the current fork, and context-switch to the first
	 available.  */
      linux_fork_detach (from_tty);
    }
  else
    {
      target_announce_detach (from_tty);

      /* Pass on any pending signal for the last LWP.  */
      int signo = get_detach_signal (main_lwp);

      detach_one_lwp (main_lwp, &signo);

      detach_success (inf);
    }
}

/* Resume execution of the inferior process.  If STEP is nonzero,
   single-step it.  If SIGNAL is nonzero, give it that signal.  */

static void
linux_resume_one_lwp_throw (struct lwp_info *lp, int step,
			    enum gdb_signal signo)
{
  lp->step = step;

  /* stop_pc doubles as the PC the LWP had when it was last resumed.
     We only presently need that if the LWP is stepped though (to
     handle the case of stepping a breakpoint instruction).  */
  if (step)
    {
      struct regcache *regcache = get_thread_regcache (linux_target, lp->ptid);

      lp->stop_pc = regcache_read_pc (regcache);
    }
  else
    lp->stop_pc = 0;

  linux_target->low_prepare_to_resume (lp);
  linux_target->low_resume (lp->ptid, step, signo);

  /* Successfully resumed.  Clear state that no longer makes sense,
     and mark the LWP as running.  Must not do this before resuming
     otherwise if that fails other code will be confused.  E.g., we'd
     later try to stop the LWP and hang forever waiting for a stop
     status.  Note that we must not throw after this is cleared,
     otherwise handle_zombie_lwp_error would get confused.  */
  lp->stopped = 0;
  lp->core = -1;
  lp->stop_reason = TARGET_STOPPED_BY_NO_REASON;
  registers_changed_ptid (linux_target, lp->ptid);
}

/* Called when we try to resume a stopped LWP and that errors out.  If
   the LWP is no longer in ptrace-stopped state (meaning it's zombie,
   or about to become), discard the error, clear any pending status
   the LWP may have, and return true (we'll collect the exit status
   soon enough).  Otherwise, return false.  */

static int
check_ptrace_stopped_lwp_gone (struct lwp_info *lp)
{
  /* If we get an error after resuming the LWP successfully, we'd
     confuse !T state for the LWP being gone.  */
  gdb_assert (lp->stopped);

  /* We can't just check whether the LWP is in 'Z (Zombie)' state,
     because even if ptrace failed with ESRCH, the tracee may be "not
     yet fully dead", but already refusing ptrace requests.  In that
     case the tracee has 'R (Running)' state for a little bit
     (observed in Linux 3.18).  See also the note on ESRCH in the
     ptrace(2) man page.  Instead, check whether the LWP has any state
     other than ptrace-stopped.  */

  /* Don't assume anything if /proc/PID/status can't be read.  */
  if (linux_proc_pid_is_trace_stopped_nowarn (lp->ptid.lwp ()) == 0)
    {
      lp->stop_reason = TARGET_STOPPED_BY_NO_REASON;
      lp->status = 0;
      lp->waitstatus.kind = TARGET_WAITKIND_IGNORE;
      return 1;
    }
  return 0;
}

/* Like linux_resume_one_lwp_throw, but no error is thrown if the LWP
   disappears while we try to resume it.  */

static void
linux_resume_one_lwp (struct lwp_info *lp, int step, enum gdb_signal signo)
{
  try
    {
      linux_resume_one_lwp_throw (lp, step, signo);
    }
  catch (const gdb_exception_error &ex)
    {
      if (!check_ptrace_stopped_lwp_gone (lp))
	throw;
    }
}

/* Resume LP.  */

static void
resume_lwp (struct lwp_info *lp, int step, enum gdb_signal signo)
{
  if (lp->stopped)
    {
      struct inferior *inf = find_inferior_ptid (linux_target, lp->ptid);

      if (inf->vfork_child != NULL)
	{
	  linux_nat_debug_printf ("Not resuming %s (vfork parent)",
				  target_pid_to_str (lp->ptid).c_str ());
	}
      else if (!lwp_status_pending_p (lp))
	{
	  linux_nat_debug_printf ("Resuming sibling %s, %s, %s",
				  target_pid_to_str (lp->ptid).c_str (),
				  (signo != GDB_SIGNAL_0
				   ? strsignal (gdb_signal_to_host (signo))
				   : "0"),
				  step ? "step" : "resume");

	  linux_resume_one_lwp (lp, step, signo);
	}
      else
	{
	  linux_nat_debug_printf ("Not resuming sibling %s (has pending)",
				  target_pid_to_str (lp->ptid).c_str ());
	}
    }
  else
    linux_nat_debug_printf ("Not resuming sibling %s (not stopped)",
			    target_pid_to_str (lp->ptid).c_str ());
}

/* Callback for iterate_over_lwps.  If LWP is EXCEPT, do nothing.
   Resume LWP with the last stop signal, if it is in pass state.  */

static int
linux_nat_resume_callback (struct lwp_info *lp, struct lwp_info *except)
{
  enum gdb_signal signo = GDB_SIGNAL_0;

  if (lp == except)
    return 0;

  if (lp->stopped)
    {
      struct thread_info *thread;

      thread = find_thread_ptid (linux_target, lp->ptid);
      if (thread != NULL)
	{
	  signo = thread->suspend.stop_signal;
	  thread->suspend.stop_signal = GDB_SIGNAL_0;
	}
    }

  resume_lwp (lp, 0, signo);
  return 0;
}

static int
resume_clear_callback (struct lwp_info *lp)
{
  lp->resumed = 0;
  lp->last_resume_kind = resume_stop;
  return 0;
}

static int
resume_set_callback (struct lwp_info *lp)
{
  lp->resumed = 1;
  lp->last_resume_kind = resume_continue;
  return 0;
}

void
linux_nat_target::resume (ptid_t ptid, int step, enum gdb_signal signo)
{
  struct lwp_info *lp;
  int resume_many;

  linux_nat_debug_printf ("Preparing to %s %s, %s, inferior_ptid %s",
			  step ? "step" : "resume",
			  target_pid_to_str (ptid).c_str (),
			  (signo != GDB_SIGNAL_0
			   ? strsignal (gdb_signal_to_host (signo)) : "0"),
			  target_pid_to_str (inferior_ptid).c_str ());

  /* A specific PTID means `step only this process id'.  */
  resume_many = (minus_one_ptid == ptid
		 || ptid.is_pid ());

  /* Mark the lwps we're resuming as resumed and update their
     last_resume_kind to resume_continue.  */
  iterate_over_lwps (ptid, resume_set_callback);

  /* See if it's the current inferior that should be handled
     specially.  */
  if (resume_many)
    lp = find_lwp_pid (inferior_ptid);
  else
    lp = find_lwp_pid (ptid);
  gdb_assert (lp != NULL);

  /* Remember if we're stepping.  */
  lp->last_resume_kind = step ? resume_step : resume_continue;

  /* If we have a pending wait status for this thread, there is no
     point in resuming the process.  But first make sure that
     linux_nat_wait won't preemptively handle the event - we
     should never take this short-circuit if we are going to
     leave LP running, since we have skipped resuming all the
     other threads.  This bit of code needs to be synchronized
     with linux_nat_wait.  */

  if (lp->status && WIFSTOPPED (lp->status))
    {
      if (!lp->step
	  && WSTOPSIG (lp->status)
	  && sigismember (&pass_mask, WSTOPSIG (lp->status)))
	{
	  linux_nat_debug_printf
	    ("Not short circuiting for ignored status 0x%x", lp->status);

	  /* FIXME: What should we do if we are supposed to continue
	     this thread with a signal?  */
	  gdb_assert (signo == GDB_SIGNAL_0);
	  signo = gdb_signal_from_host (WSTOPSIG (lp->status));
	  lp->status = 0;
	}
    }

  if (lwp_status_pending_p (lp))
    {
      /* FIXME: What should we do if we are supposed to continue
	 this thread with a signal?  */
      gdb_assert (signo == GDB_SIGNAL_0);

      linux_nat_debug_printf ("Short circuiting for status 0x%x",
			      lp->status);

      if (target_can_async_p ())
	{
	  target_async (1);
	  /* Tell the event loop we have something to process.  */
	  async_file_mark ();
	}
      return;
    }

  if (resume_many)
    iterate_over_lwps (ptid, [=] (struct lwp_info *info)
			     {
			       return linux_nat_resume_callback (info, lp);
			     });

  linux_nat_debug_printf ("%s %s, %s (resume event thread)",
			  step ? "PTRACE_SINGLESTEP" : "PTRACE_CONT",
			  target_pid_to_str (lp->ptid).c_str (),
			  (signo != GDB_SIGNAL_0
			   ? strsignal (gdb_signal_to_host (signo)) : "0"));

  linux_resume_one_lwp (lp, step, signo);

  if (target_can_async_p ())
    target_async (1);
}

/* Send a signal to an LWP.  */

static int
kill_lwp (int lwpid, int signo)
{
  int ret;

  errno = 0;
  ret = syscall (__NR_tkill, lwpid, signo);
  if (errno == ENOSYS)
    {
      /* If tkill fails, then we are not using nptl threads, a
	 configuration we no longer support.  */
      perror_with_name (("tkill"));
    }
  return ret;
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
  thread_info *thread = find_thread_ptid (linux_target, lp->ptid);
  int syscall_number = (int) gdbarch_get_syscall_number (gdbarch, thread);

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

      linux_nat_debug_printf
	("ignoring syscall %d for LWP %ld (stopping threads), resuming with "
	 "PTRACE_CONT for SIGSTOP", syscall_number, lp->ptid.lwp ());

      lp->syscall_state = TARGET_WAITKIND_IGNORE;
      ptrace (PTRACE_CONT, lp->ptid.lwp (), 0, 0);
      lp->stopped = 0;
      return 1;
    }

  /* Always update the entry/return state, even if this particular
     syscall isn't interesting to the core now.  In async mode,
     the user could install a new catchpoint for this syscall
     between syscall enter/return, and we'll need to know to
     report a syscall return if that happens.  */
  lp->syscall_state = (lp->syscall_state == TARGET_WAITKIND_SYSCALL_ENTRY
		       ? TARGET_WAITKIND_SYSCALL_RETURN
		       : TARGET_WAITKIND_SYSCALL_ENTRY);

  if (catch_syscall_enabled ())
    {
      if (catching_syscall_number (syscall_number))
	{
	  /* Alright, an event to report.  */
	  ourstatus->kind = lp->syscall_state;
	  ourstatus->value.syscall_number = syscall_number;

	  linux_nat_debug_printf
	    ("stopping for %s of syscall %d for LWP %ld",
	     (lp->syscall_state == TARGET_WAITKIND_SYSCALL_ENTRY
	      ? "entry" : "return"), syscall_number, lp->ptid.lwp ());

	  return 0;
	}

      linux_nat_debug_printf
	("ignoring %s of syscall %d for LWP %ld",
	 (lp->syscall_state == TARGET_WAITKIND_SYSCALL_ENTRY
	  ? "entry" : "return"), syscall_number, lp->ptid.lwp ());
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
      linux_nat_debug_printf
	("caught syscall event with no syscall catchpoints. %d for LWP %ld, "
	 "ignoring", syscall_number, lp->ptid.lwp ());
      lp->syscall_state = TARGET_WAITKIND_IGNORE;
    }

  /* The core isn't interested in this event.  For efficiency, avoid
     stopping all threads only to have the core resume them all again.
     Since we're not stopping threads, if we're still syscall tracing
     and not stepping, we can't use PTRACE_CONT here, as we'd miss any
     subsequent syscall.  Simply resume using the inf-ptrace layer,
     which knows when to use PT_SYSCALL or PT_CONTINUE.  */

  linux_resume_one_lwp (lp, lp->step, GDB_SIGNAL_0);
  return 1;
}

/* Handle a GNU/Linux extended wait response.  If we see a clone
   event, we need to add the new LWP to our list (and not report the
   trap to higher layers).  This function returns non-zero if the
   event should be ignored and we should wait again.  If STOPPING is
   true, the new LWP remains stopped, otherwise it is continued.  */

static int
linux_handle_extended_wait (struct lwp_info *lp, int status)
{
  int pid = lp->ptid.lwp ();
  struct target_waitstatus *ourstatus = &lp->waitstatus;
  int event = linux_ptrace_get_extended_event (status);

  /* All extended events we currently use are mid-syscall.  Only
     PTRACE_EVENT_STOP is delivered more like a signal-stop, but
     you have to be using PTRACE_SEIZE to get that.  */
  lp->syscall_state = TARGET_WAITKIND_SYSCALL_ENTRY;

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
	  ret = my_waitpid (new_pid, &status, __WALL);
	  if (ret == -1)
	    perror_with_name (_("waiting for new child"));
	  else if (ret != new_pid)
	    internal_error (__FILE__, __LINE__,
			    _("wait returned unexpected PID %d"), ret);
	  else if (!WIFSTOPPED (status))
	    internal_error (__FILE__, __LINE__,
			    _("wait returned unexpected status 0x%x"), status);
	}

      ourstatus->value.related_pid = ptid_t (new_pid, new_pid, 0);

      if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK)
	{
	  /* The arch-specific native code may need to know about new
	     forks even if those end up never mapped to an
	     inferior.  */
	  linux_target->low_new_fork (lp, new_pid);
	}
      else if (event == PTRACE_EVENT_CLONE)
	{
	  linux_target->low_new_clone (lp, new_pid);
	}

      if (event == PTRACE_EVENT_FORK
	  && linux_fork_checkpointing_p (lp->ptid.pid ()))
	{
	  /* Handle checkpointing by linux-fork.c here as a special
	     case.  We don't want the follow-fork-mode or 'catch fork'
	     to interfere with this.  */

	  /* This won't actually modify the breakpoint list, but will
	     physically remove the breakpoints from the child.  */
	  detach_breakpoints (ptid_t (new_pid, new_pid, 0));

	  /* Retain child fork in ptrace (stopped) state.  */
	  if (!find_fork_pid (new_pid))
	    add_fork (new_pid);

	  /* Report as spurious, so that infrun doesn't want to follow
	     this fork.  We're actually doing an infcall in
	     linux-fork.c.  */
	  ourstatus->kind = TARGET_WAITKIND_SPURIOUS;

	  /* Report the stop to the core.  */
	  return 0;
	}

      if (event == PTRACE_EVENT_FORK)
	ourstatus->kind = TARGET_WAITKIND_FORKED;
      else if (event == PTRACE_EVENT_VFORK)
	ourstatus->kind = TARGET_WAITKIND_VFORKED;
      else if (event == PTRACE_EVENT_CLONE)
	{
	  struct lwp_info *new_lp;

	  ourstatus->kind = TARGET_WAITKIND_IGNORE;

	  linux_nat_debug_printf
	    ("Got clone event from LWP %d, new child is LWP %ld", pid, new_pid);

	  new_lp = add_lwp (ptid_t (lp->ptid.pid (), new_pid, 0));
	  new_lp->stopped = 1;
	  new_lp->resumed = 1;

	  /* If the thread_db layer is active, let it record the user
	     level thread id and status, and add the thread to GDB's
	     list.  */
	  if (!thread_db_notice_clone (lp->ptid, new_lp->ptid))
	    {
	      /* The process is not using thread_db.  Add the LWP to
		 GDB's list.  */
	      target_post_attach (new_lp->ptid.lwp ());
	      add_thread (linux_target, new_lp->ptid);
	    }

	  /* Even if we're stopping the thread for some reason
	     internal to this module, from the perspective of infrun
	     and the user/frontend, this new thread is running until
	     it next reports a stop.  */
	  set_running (linux_target, new_lp->ptid, true);
	  set_executing (linux_target, new_lp->ptid, true);

	  if (WSTOPSIG (status) != SIGSTOP)
	    {
	      /* This can happen if someone starts sending signals to
		 the new thread before it gets a chance to run, which
		 have a lower number than SIGSTOP (e.g. SIGUSR1).
		 This is an unlikely case, and harder to handle for
		 fork / vfork than for clone, so we do not try - but
		 we handle it for clone events here.  */

	      new_lp->signalled = 1;

	      /* We created NEW_LP so it cannot yet contain STATUS.  */
	      gdb_assert (new_lp->status == 0);

	      /* Save the wait status to report later.  */
	      linux_nat_debug_printf
		("waitpid of new LWP %ld, saving status %s",
		 (long) new_lp->ptid.lwp (), status_to_str (status));
	      new_lp->status = status;
	    }
	  else if (report_thread_events)
	    {
	      new_lp->waitstatus.kind = TARGET_WAITKIND_THREAD_CREATED;
	      new_lp->status = status;
	    }

	  return 1;
	}

      return 0;
    }

  if (event == PTRACE_EVENT_EXEC)
    {
      linux_nat_debug_printf ("Got exec event from LWP %ld", lp->ptid.lwp ());

      ourstatus->kind = TARGET_WAITKIND_EXECD;
      ourstatus->value.execd_pathname
	= xstrdup (linux_proc_pid_to_exec_file (pid));

      /* The thread that execed must have been resumed, but, when a
	 thread execs, it changes its tid to the tgid, and the old
	 tgid thread might have not been resumed.  */
      lp->resumed = 1;
      return 0;
    }

  if (event == PTRACE_EVENT_VFORK_DONE)
    {
      if (current_inferior ()->waiting_for_vfork_done)
	{
	  linux_nat_debug_printf
	    ("Got expected PTRACE_EVENT_VFORK_DONE from LWP %ld: stopping",
	     lp->ptid.lwp ());

	  ourstatus->kind = TARGET_WAITKIND_VFORK_DONE;
	  return 0;
	}

      linux_nat_debug_printf
	("Got PTRACE_EVENT_VFORK_DONE from LWP %ld: ignoring", lp->ptid.lwp ());

      return 1;
    }

  internal_error (__FILE__, __LINE__,
		  _("unknown ptrace event %d"), event);
}

/* Suspend waiting for a signal.  We're mostly interested in
   SIGCHLD/SIGINT.  */

static void
wait_for_signal ()
{
  linux_nat_debug_printf ("about to sigsuspend");
  sigsuspend (&suspend_mask);

  /* If the quit flag is set, it means that the user pressed Ctrl-C
     and we're debugging a process that is running on a separate
     terminal, so we must forward the Ctrl-C to the inferior.  (If the
     inferior is sharing GDB's terminal, then the Ctrl-C reaches the
     inferior directly.)  We must do this here because functions that
     need to block waiting for a signal loop forever until there's an
     event to report before returning back to the event loop.  */
  if (!target_terminal::is_ours ())
    {
      if (check_quit_flag ())
	target_pass_ctrlc ();
    }
}

/* Wait for LP to stop.  Returns the wait status, or 0 if the LWP has
   exited.  */

static int
wait_lwp (struct lwp_info *lp)
{
  pid_t pid;
  int status = 0;
  int thread_dead = 0;
  sigset_t prev_mask;

  gdb_assert (!lp->stopped);
  gdb_assert (lp->status == 0);

  /* Make sure SIGCHLD is blocked for sigsuspend avoiding a race below.  */
  block_child_signals (&prev_mask);

  for (;;)
    {
      pid = my_waitpid (lp->ptid.lwp (), &status, __WALL | WNOHANG);
      if (pid == -1 && errno == ECHILD)
	{
	  /* The thread has previously exited.  We need to delete it
	     now because if this was a non-leader thread execing, we
	     won't get an exit event.  See comments on exec events at
	     the top of the file.  */
	  thread_dead = 1;
	  linux_nat_debug_printf ("%s vanished.",
				  target_pid_to_str (lp->ptid).c_str ());
	}
      if (pid != 0)
	break;

      /* Bugs 10970, 12702.
	 Thread group leader may have exited in which case we'll lock up in
	 waitpid if there are other threads, even if they are all zombies too.
	 Basically, we're not supposed to use waitpid this way.
	  tkill(pid,0) cannot be used here as it gets ESRCH for both
	 for zombie and running processes.

	 As a workaround, check if we're waiting for the thread group leader and
	 if it's a zombie, and avoid calling waitpid if it is.

	 This is racy, what if the tgl becomes a zombie right after we check?
	 Therefore always use WNOHANG with sigsuspend - it is equivalent to
	 waiting waitpid but linux_proc_pid_is_zombie is safe this way.  */

      if (lp->ptid.pid () == lp->ptid.lwp ()
	  && linux_proc_pid_is_zombie (lp->ptid.lwp ()))
	{
	  thread_dead = 1;
	  linux_nat_debug_printf ("Thread group leader %s vanished.",
				  target_pid_to_str (lp->ptid).c_str ());
	  break;
	}

      /* Wait for next SIGCHLD and try again.  This may let SIGCHLD handlers
	 get invoked despite our caller had them intentionally blocked by
	 block_child_signals.  This is sensitive only to the loop of
	 linux_nat_wait_1 and there if we get called my_waitpid gets called
	 again before it gets to sigsuspend so we can safely let the handlers
	 get executed here.  */
      wait_for_signal ();
    }

  restore_child_signals_mask (&prev_mask);

  if (!thread_dead)
    {
      gdb_assert (pid == lp->ptid.lwp ());

      linux_nat_debug_printf ("waitpid %s received %s",
			      target_pid_to_str (lp->ptid).c_str (),
			      status_to_str (status));

      /* Check if the thread has exited.  */
      if (WIFEXITED (status) || WIFSIGNALED (status))
	{
	  if (report_thread_events
	      || lp->ptid.pid () == lp->ptid.lwp ())
	    {
	      linux_nat_debug_printf ("LWP %d exited.", lp->ptid.pid ());

	      /* If this is the leader exiting, it means the whole
		 process is gone.  Store the status to report to the
		 core.  Store it in lp->waitstatus, because lp->status
		 would be ambiguous (W_EXITCODE(0,0) == 0).  */
	      store_waitstatus (&lp->waitstatus, status);
	      return 0;
	    }

	  thread_dead = 1;
	  linux_nat_debug_printf ("%s exited.",
				  target_pid_to_str (lp->ptid).c_str ());
	}
    }

  if (thread_dead)
    {
      exit_lwp (lp);
      return 0;
    }

  gdb_assert (WIFSTOPPED (status));
  lp->stopped = 1;

  if (lp->must_set_ptrace_flags)
    {
      inferior *inf = find_inferior_pid (linux_target, lp->ptid.pid ());
      int options = linux_nat_ptrace_options (inf->attach_flag);

      linux_enable_event_reporting (lp->ptid.lwp (), options);
      lp->must_set_ptrace_flags = 0;
    }

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
  else
    {
      /* Almost all other ptrace-stops are known to be outside of system
	 calls, with further exceptions in linux_handle_extended_wait.  */
      lp->syscall_state = TARGET_WAITKIND_IGNORE;
    }

  /* Handle GNU/Linux's extended waitstatus for trace events.  */
  if (WIFSTOPPED (status) && WSTOPSIG (status) == SIGTRAP
      && linux_is_extended_waitstatus (status))
    {
      linux_nat_debug_printf ("Handling extended status 0x%06x", status);
      linux_handle_extended_wait (lp, status);
      return 0;
    }

  return status;
}

/* Send a SIGSTOP to LP.  */

static int
stop_callback (struct lwp_info *lp)
{
  if (!lp->stopped && !lp->signalled)
    {
      int ret;

      linux_nat_debug_printf ("kill %s **<SIGSTOP>**",
			      target_pid_to_str (lp->ptid).c_str ());

      errno = 0;
      ret = kill_lwp (lp->ptid.lwp (), SIGSTOP);
      linux_nat_debug_printf ("lwp kill %d %s", ret,
			      errno ? safe_strerror (errno) : "ERRNO-OK");

      lp->signalled = 1;
      gdb_assert (lp->status == 0);
    }

  return 0;
}

/* Request a stop on LWP.  */

void
linux_stop_lwp (struct lwp_info *lwp)
{
  stop_callback (lwp);
}

/* See linux-nat.h  */

void
linux_stop_and_wait_all_lwps (void)
{
  /* Stop all LWP's ...  */
  iterate_over_lwps (minus_one_ptid, stop_callback);

  /* ... and wait until all of them have reported back that
     they're no longer running.  */
  iterate_over_lwps (minus_one_ptid, stop_wait_callback);
}

/* See linux-nat.h  */

void
linux_unstop_all_lwps (void)
{
  iterate_over_lwps (minus_one_ptid,
		     [] (struct lwp_info *info)
		     {
		       return resume_stopped_resumed_lwps (info, minus_one_ptid);
		     });
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
set_ignore_sigint (struct lwp_info *lp)
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

  if (!linux_nat_has_pending_sigint (lp->ptid.lwp ()))
    {
      linux_nat_debug_printf ("Clearing bogus flag for %s",
			      target_pid_to_str (lp->ptid).c_str ());
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

static int
check_stopped_by_watchpoint (struct lwp_info *lp)
{
  scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);
  inferior_ptid = lp->ptid;

  if (linux_target->low_stopped_by_watchpoint ())
    {
      lp->stop_reason = TARGET_STOPPED_BY_WATCHPOINT;
      lp->stopped_data_address_p
	= linux_target->low_stopped_data_address (&lp->stopped_data_address);
    }

  return lp->stop_reason == TARGET_STOPPED_BY_WATCHPOINT;
}

/* Returns true if the LWP had stopped for a watchpoint.  */

bool
linux_nat_target::stopped_by_watchpoint ()
{
  struct lwp_info *lp = find_lwp_pid (inferior_ptid);

  gdb_assert (lp != NULL);

  return lp->stop_reason == TARGET_STOPPED_BY_WATCHPOINT;
}

bool
linux_nat_target::stopped_data_address (CORE_ADDR *addr_p)
{
  struct lwp_info *lp = find_lwp_pid (inferior_ptid);

  gdb_assert (lp != NULL);

  *addr_p = lp->stopped_data_address;

  return lp->stopped_data_address_p;
}

/* Commonly any breakpoint / watchpoint generate only SIGTRAP.  */

bool
linux_nat_target::low_status_is_event (int status)
{
  return WIFSTOPPED (status) && WSTOPSIG (status) == SIGTRAP;
}

/* Wait until LP is stopped.  */

static int
stop_wait_callback (struct lwp_info *lp)
{
  inferior *inf = find_inferior_ptid (linux_target, lp->ptid);

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
	  ptrace (PTRACE_CONT, lp->ptid.lwp (), 0, 0);
	  lp->stopped = 0;
	  linux_nat_debug_printf
	    ("PTRACE_CONT %s, 0, 0 (%s) (discarding SIGINT)",
	     target_pid_to_str (lp->ptid).c_str (),
	     errno ? safe_strerror (errno) : "OK");

	  return stop_wait_callback (lp);
	}

      maybe_clear_ignore_sigint (lp);

      if (WSTOPSIG (status) != SIGSTOP)
	{
	  /* The thread was stopped with a signal other than SIGSTOP.  */

	  linux_nat_debug_printf ("Pending event %s in %s",
				  status_to_str ((int) status),
				  target_pid_to_str (lp->ptid).c_str ());

	  /* Save the sigtrap event.  */
	  lp->status = status;
	  gdb_assert (lp->signalled);
	  save_stop_reason (lp);
	}
      else
	{
	  /* We caught the SIGSTOP that we intended to catch.  */

	  linux_nat_debug_printf ("Expected SIGSTOP caught for %s.",
				  target_pid_to_str (lp->ptid).c_str ());

	  lp->signalled = 0;

	  /* If we are waiting for this stop so we can report the thread
	     stopped then we need to record this status.  Otherwise, we can
	     now discard this stop event.  */
	  if (lp->last_resume_kind == resume_stop)
	    {
	      lp->status = status;
	      save_stop_reason (lp);
	    }
	}
    }

  return 0;
}

/* Return non-zero if LP has a wait status pending.  Discard the
   pending event and resume the LWP if the event that originally
   caused the stop became uninteresting.  */

static int
status_callback (struct lwp_info *lp)
{
  /* Only report a pending wait status if we pretend that this has
     indeed been resumed.  */
  if (!lp->resumed)
    return 0;

  if (!lwp_status_pending_p (lp))
    return 0;

  if (lp->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT
      || lp->stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT)
    {
      struct regcache *regcache = get_thread_regcache (linux_target, lp->ptid);
      CORE_ADDR pc;
      int discard = 0;

      pc = regcache_read_pc (regcache);

      if (pc != lp->stop_pc)
	{
	  linux_nat_debug_printf ("PC of %s changed.  was=%s, now=%s",
				  target_pid_to_str (lp->ptid).c_str (),
				  paddress (target_gdbarch (), lp->stop_pc),
				  paddress (target_gdbarch (), pc));
	  discard = 1;
	}

#if !USE_SIGTRAP_SIGINFO
      else if (!breakpoint_inserted_here_p (regcache->aspace (), pc))
	{
	  linux_nat_debug_printf ("previous breakpoint of %s, at %s gone",
				  target_pid_to_str (lp->ptid).c_str (),
				  paddress (target_gdbarch (), lp->stop_pc));

	  discard = 1;
	}
#endif

      if (discard)
	{
	  linux_nat_debug_printf ("pending event of %s cancelled.",
				  target_pid_to_str (lp->ptid).c_str ());

	  lp->status = 0;
	  linux_resume_one_lwp (lp, lp->step, GDB_SIGNAL_0);
	  return 0;
	}
    }

  return 1;
}

/* Count the LWP's that have had events.  */

static int
count_events_callback (struct lwp_info *lp, int *count)
{
  gdb_assert (count != NULL);

  /* Select only resumed LWPs that have an event pending.  */
  if (lp->resumed && lwp_status_pending_p (lp))
    (*count)++;

  return 0;
}

/* Select the LWP (if any) that is currently being single-stepped.  */

static int
select_singlestep_lwp_callback (struct lwp_info *lp)
{
  if (lp->last_resume_kind == resume_step
      && lp->status != 0)
    return 1;
  else
    return 0;
}

/* Returns true if LP has a status pending.  */

static int
lwp_status_pending_p (struct lwp_info *lp)
{
  /* We check for lp->waitstatus in addition to lp->status, because we
     can have pending process exits recorded in lp->status and
     W_EXITCODE(0,0) happens to be 0.  */
  return lp->status != 0 || lp->waitstatus.kind != TARGET_WAITKIND_IGNORE;
}

/* Select the Nth LWP that has had an event.  */

static int
select_event_lwp_callback (struct lwp_info *lp, int *selector)
{
  gdb_assert (selector != NULL);

  /* Select only resumed LWPs that have an event pending.  */
  if (lp->resumed && lwp_status_pending_p (lp))
    if ((*selector)-- == 0)
      return 1;

  return 0;
}

/* Called when the LWP stopped for a signal/trap.  If it stopped for a
   trap check what caused it (breakpoint, watchpoint, trace, etc.),
   and save the result in the LWP's stop_reason field.  If it stopped
   for a breakpoint, decrement the PC if necessary on the lwp's
   architecture.  */

static void
save_stop_reason (struct lwp_info *lp)
{
  struct regcache *regcache;
  struct gdbarch *gdbarch;
  CORE_ADDR pc;
  CORE_ADDR sw_bp_pc;
#if USE_SIGTRAP_SIGINFO
  siginfo_t siginfo;
#endif

  gdb_assert (lp->stop_reason == TARGET_STOPPED_BY_NO_REASON);
  gdb_assert (lp->status != 0);

  if (!linux_target->low_status_is_event (lp->status))
    return;

  regcache = get_thread_regcache (linux_target, lp->ptid);
  gdbarch = regcache->arch ();

  pc = regcache_read_pc (regcache);
  sw_bp_pc = pc - gdbarch_decr_pc_after_break (gdbarch);

#if USE_SIGTRAP_SIGINFO
  if (linux_nat_get_siginfo (lp->ptid, &siginfo))
    {
      if (siginfo.si_signo == SIGTRAP)
	{
	  if (GDB_ARCH_IS_TRAP_BRKPT (siginfo.si_code)
	      && GDB_ARCH_IS_TRAP_HWBKPT (siginfo.si_code))
	    {
	      /* The si_code is ambiguous on this arch -- check debug
		 registers.  */
	      if (!check_stopped_by_watchpoint (lp))
		lp->stop_reason = TARGET_STOPPED_BY_SW_BREAKPOINT;
	    }
	  else if (GDB_ARCH_IS_TRAP_BRKPT (siginfo.si_code))
	    {
	      /* If we determine the LWP stopped for a SW breakpoint,
		 trust it.  Particularly don't check watchpoint
		 registers, because, at least on s390, we'd find
		 stopped-by-watchpoint as long as there's a watchpoint
		 set.  */
	      lp->stop_reason = TARGET_STOPPED_BY_SW_BREAKPOINT;
	    }
	  else if (GDB_ARCH_IS_TRAP_HWBKPT (siginfo.si_code))
	    {
	      /* This can indicate either a hardware breakpoint or
		 hardware watchpoint.  Check debug registers.  */
	      if (!check_stopped_by_watchpoint (lp))
		lp->stop_reason = TARGET_STOPPED_BY_HW_BREAKPOINT;
	    }
	  else if (siginfo.si_code == TRAP_TRACE)
	    {
	      linux_nat_debug_printf ("%s stopped by trace",
				      target_pid_to_str (lp->ptid).c_str ());

	      /* We may have single stepped an instruction that
		 triggered a watchpoint.  In that case, on some
		 architectures (such as x86), instead of TRAP_HWBKPT,
		 si_code indicates TRAP_TRACE, and we need to check
		 the debug registers separately.  */
	      check_stopped_by_watchpoint (lp);
	    }
	}
    }
#else
  if ((!lp->step || lp->stop_pc == sw_bp_pc)
      && software_breakpoint_inserted_here_p (regcache->aspace (),
					      sw_bp_pc))
    {
      /* The LWP was either continued, or stepped a software
	 breakpoint instruction.  */
      lp->stop_reason = TARGET_STOPPED_BY_SW_BREAKPOINT;
    }

  if (hardware_breakpoint_inserted_here_p (regcache->aspace (), pc))
    lp->stop_reason = TARGET_STOPPED_BY_HW_BREAKPOINT;

  if (lp->stop_reason == TARGET_STOPPED_BY_NO_REASON)
    check_stopped_by_watchpoint (lp);
#endif

  if (lp->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT)
    {
      linux_nat_debug_printf ("%s stopped by software breakpoint",
			      target_pid_to_str (lp->ptid).c_str ());

      /* Back up the PC if necessary.  */
      if (pc != sw_bp_pc)
	regcache_write_pc (regcache, sw_bp_pc);

      /* Update this so we record the correct stop PC below.  */
      pc = sw_bp_pc;
    }
  else if (lp->stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT)
    {
      linux_nat_debug_printf ("%s stopped by hardware breakpoint",
			      target_pid_to_str (lp->ptid).c_str ());
    }
  else if (lp->stop_reason == TARGET_STOPPED_BY_WATCHPOINT)
    {
      linux_nat_debug_printf ("%s stopped by hardware watchpoint",
			      target_pid_to_str (lp->ptid).c_str ());
    }

  lp->stop_pc = pc;
}


/* Returns true if the LWP had stopped for a software breakpoint.  */

bool
linux_nat_target::stopped_by_sw_breakpoint ()
{
  struct lwp_info *lp = find_lwp_pid (inferior_ptid);

  gdb_assert (lp != NULL);

  return lp->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT;
}

/* Implement the supports_stopped_by_sw_breakpoint method.  */

bool
linux_nat_target::supports_stopped_by_sw_breakpoint ()
{
  return USE_SIGTRAP_SIGINFO;
}

/* Returns true if the LWP had stopped for a hardware
   breakpoint/watchpoint.  */

bool
linux_nat_target::stopped_by_hw_breakpoint ()
{
  struct lwp_info *lp = find_lwp_pid (inferior_ptid);

  gdb_assert (lp != NULL);

  return lp->stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT;
}

/* Implement the supports_stopped_by_hw_breakpoint method.  */

bool
linux_nat_target::supports_stopped_by_hw_breakpoint ()
{
  return USE_SIGTRAP_SIGINFO;
}

/* Select one LWP out of those that have events pending.  */

static void
select_event_lwp (ptid_t filter, struct lwp_info **orig_lp, int *status)
{
  int num_events = 0;
  int random_selector;
  struct lwp_info *event_lp = NULL;

  /* Record the wait status for the original LWP.  */
  (*orig_lp)->status = *status;

  /* In all-stop, give preference to the LWP that is being
     single-stepped.  There will be at most one, and it will be the
     LWP that the core is most interested in.  If we didn't do this,
     then we'd have to handle pending step SIGTRAPs somehow in case
     the core later continues the previously-stepped thread, as
     otherwise we'd report the pending SIGTRAP then, and the core, not
     having stepped the thread, wouldn't understand what the trap was
     for, and therefore would report it to the user as a random
     signal.  */
  if (!target_is_non_stop_p ())
    {
      event_lp = iterate_over_lwps (filter, select_singlestep_lwp_callback);
      if (event_lp != NULL)
	{
	  linux_nat_debug_printf ("Select single-step %s",
				  target_pid_to_str (event_lp->ptid).c_str ());
	}
    }

  if (event_lp == NULL)
    {
      /* Pick one at random, out of those which have had events.  */

      /* First see how many events we have.  */
      iterate_over_lwps (filter,
			 [&] (struct lwp_info *info)
			 {
			   return count_events_callback (info, &num_events);
			 });
      gdb_assert (num_events > 0);

      /* Now randomly pick a LWP out of those that have had
	 events.  */
      random_selector = (int)
	((num_events * (double) rand ()) / (RAND_MAX + 1.0));

      if (num_events > 1)
	linux_nat_debug_printf ("Found %d events, selecting #%d",
				num_events, random_selector);

      event_lp
	= (iterate_over_lwps
	   (filter,
	    [&] (struct lwp_info *info)
	    {
	      return select_event_lwp_callback (info,
						&random_selector);
	    }));
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
resumed_callback (struct lwp_info *lp)
{
  return lp->resumed;
}

/* Check if we should go on and pass this event to common code.
   Return the affected lwp if we should, or NULL otherwise.  */

static struct lwp_info *
linux_nat_filter_event (int lwpid, int status)
{
  struct lwp_info *lp;
  int event = linux_ptrace_get_extended_event (status);

  lp = find_lwp_pid (ptid_t (lwpid));

  /* Check for stop events reported by a process we didn't already
     know about - anything not already in our LWP list.

     If we're expecting to receive stopped processes after
     fork, vfork, and clone events, then we'll just add the
     new one to our list and go back to waiting for the event
     to be reported - the stopped process might be returned
     from waitpid before or after the event is.

     But note the case of a non-leader thread exec'ing after the
     leader having exited, and gone from our lists.  The non-leader
     thread changes its tid to the tgid.  */

  if (WIFSTOPPED (status) && lp == NULL
      && (WSTOPSIG (status) == SIGTRAP && event == PTRACE_EVENT_EXEC))
    {
      /* A multi-thread exec after we had seen the leader exiting.  */
      linux_nat_debug_printf ("Re-adding thread group leader LWP %d.", lwpid);

      lp = add_lwp (ptid_t (lwpid, lwpid, 0));
      lp->stopped = 1;
      lp->resumed = 1;
      add_thread (linux_target, lp->ptid);
    }

  if (WIFSTOPPED (status) && !lp)
    {
      linux_nat_debug_printf ("saving LWP %ld status %s in stopped_pids list",
			      (long) lwpid, status_to_str (status));
      add_to_pid_list (&stopped_pids, lwpid, status);
      return NULL;
    }

  /* Make sure we don't report an event for the exit of an LWP not in
     our list, i.e. not part of the current process.  This can happen
     if we detach from a program we originally forked and then it
     exits.  */
  if (!WIFSTOPPED (status) && !lp)
    return NULL;

  /* This LWP is stopped now.  (And if dead, this prevents it from
     ever being continued.)  */
  lp->stopped = 1;

  if (WIFSTOPPED (status) && lp->must_set_ptrace_flags)
    {
      inferior *inf = find_inferior_pid (linux_target, lp->ptid.pid ());
      int options = linux_nat_ptrace_options (inf->attach_flag);

      linux_enable_event_reporting (lp->ptid.lwp (), options);
      lp->must_set_ptrace_flags = 0;
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
  else
    {
      /* Almost all other ptrace-stops are known to be outside of system
	 calls, with further exceptions in linux_handle_extended_wait.  */
      lp->syscall_state = TARGET_WAITKIND_IGNORE;
    }

  /* Handle GNU/Linux's extended waitstatus for trace events.  */
  if (WIFSTOPPED (status) && WSTOPSIG (status) == SIGTRAP
      && linux_is_extended_waitstatus (status))
    {
      linux_nat_debug_printf ("Handling extended status 0x%06x", status);

      if (linux_handle_extended_wait (lp, status))
	return NULL;
    }

  /* Check if the thread has exited.  */
  if (WIFEXITED (status) || WIFSIGNALED (status))
    {
      if (!report_thread_events
	  && num_lwps (lp->ptid.pid ()) > 1)
	{
	  linux_nat_debug_printf ("%s exited.",
				  target_pid_to_str (lp->ptid).c_str ());

	  /* If there is at least one more LWP, then the exit signal
	     was not the end of the debugged application and should be
	     ignored.  */
	  exit_lwp (lp);
	  return NULL;
	}

      /* Note that even if the leader was ptrace-stopped, it can still
	 exit, if e.g., some other thread brings down the whole
	 process (calls `exit').  So don't assert that the lwp is
	 resumed.  */
      linux_nat_debug_printf ("LWP %ld exited (resumed=%d)",
			      lp->ptid.lwp (), lp->resumed);

      /* Dead LWP's aren't expected to reported a pending sigstop.  */
      lp->signalled = 0;

      /* Store the pending event in the waitstatus, because
	 W_EXITCODE(0,0) == 0.  */
      store_waitstatus (&lp->waitstatus, status);
      return lp;
    }

  /* Make sure we don't report a SIGSTOP that we sent ourselves in
     an attempt to stop an LWP.  */
  if (lp->signalled
      && WIFSTOPPED (status) && WSTOPSIG (status) == SIGSTOP)
    {
      lp->signalled = 0;

      if (lp->last_resume_kind == resume_stop)
	{
	  linux_nat_debug_printf ("resume_stop SIGSTOP caught for %s.",
				  target_pid_to_str (lp->ptid).c_str ());
	}
      else
	{
	  /* This is a delayed SIGSTOP.  Filter out the event.  */

	  linux_nat_debug_printf
	    ("%s %s, 0, 0 (discard delayed SIGSTOP)",
	     lp->step ? "PTRACE_SINGLESTEP" : "PTRACE_CONT",
	     target_pid_to_str (lp->ptid).c_str ());

	  linux_resume_one_lwp (lp, lp->step, GDB_SIGNAL_0);
	  gdb_assert (lp->resumed);
	  return NULL;
	}
    }

  /* Make sure we don't report a SIGINT that we have already displayed
     for another thread.  */
  if (lp->ignore_sigint
      && WIFSTOPPED (status) && WSTOPSIG (status) == SIGINT)
    {
      linux_nat_debug_printf ("Delayed SIGINT caught for %s.",
			      target_pid_to_str (lp->ptid).c_str ());

      /* This is a delayed SIGINT.  */
      lp->ignore_sigint = 0;

      linux_resume_one_lwp (lp, lp->step, GDB_SIGNAL_0);
      linux_nat_debug_printf ("%s %s, 0, 0 (discard SIGINT)",
			      lp->step ? "PTRACE_SINGLESTEP" : "PTRACE_CONT",
			      target_pid_to_str (lp->ptid).c_str ());
      gdb_assert (lp->resumed);

      /* Discard the event.  */
      return NULL;
    }

  /* Don't report signals that GDB isn't interested in, such as
     signals that are neither printed nor stopped upon.  Stopping all
     threads can be a bit time-consuming, so if we want decent
     performance with heavily multi-threaded programs, especially when
     they're using a high frequency timer, we'd better avoid it if we
     can.  */
  if (WIFSTOPPED (status))
    {
      enum gdb_signal signo = gdb_signal_from_host (WSTOPSIG (status));

      if (!target_is_non_stop_p ())
	{
	  /* Only do the below in all-stop, as we currently use SIGSTOP
	     to implement target_stop (see linux_nat_stop) in
	     non-stop.  */
	  if (signo == GDB_SIGNAL_INT && signal_pass_state (signo) == 0)
	    {
	      /* If ^C/BREAK is typed at the tty/console, SIGINT gets
		 forwarded to the entire process group, that is, all LWPs
		 will receive it - unless they're using CLONE_THREAD to
		 share signals.  Since we only want to report it once, we
		 mark it as ignored for all LWPs except this one.  */
	      iterate_over_lwps (ptid_t (lp->ptid.pid ()), set_ignore_sigint);
	      lp->ignore_sigint = 0;
	    }
	  else
	    maybe_clear_ignore_sigint (lp);
	}

      /* When using hardware single-step, we need to report every signal.
	 Otherwise, signals in pass_mask may be short-circuited
	 except signals that might be caused by a breakpoint, or SIGSTOP
	 if we sent the SIGSTOP and are waiting for it to arrive.  */
      if (!lp->step
	  && WSTOPSIG (status) && sigismember (&pass_mask, WSTOPSIG (status))
	  && (WSTOPSIG (status) != SIGSTOP
	      || !find_thread_ptid (linux_target, lp->ptid)->stop_requested)
	  && !linux_wstatus_maybe_breakpoint (status))
	{
	  linux_resume_one_lwp (lp, lp->step, signo);
	  linux_nat_debug_printf
	    ("%s %s, %s (preempt 'handle')",
	     lp->step ? "PTRACE_SINGLESTEP" : "PTRACE_CONT",
	     target_pid_to_str (lp->ptid).c_str (),
	     (signo != GDB_SIGNAL_0
	      ? strsignal (gdb_signal_to_host (signo)) : "0"));
	  return NULL;
	}
    }

  /* An interesting event.  */
  gdb_assert (lp);
  lp->status = status;
  save_stop_reason (lp);
  return lp;
}

/* Detect zombie thread group leaders, and "exit" them.  We can't reap
   their exits until all other threads in the group have exited.  */

static void
check_zombie_leaders (void)
{
  for (inferior *inf : all_inferiors ())
    {
      struct lwp_info *leader_lp;

      if (inf->pid == 0)
	continue;

      leader_lp = find_lwp_pid (ptid_t (inf->pid));
      if (leader_lp != NULL
	  /* Check if there are other threads in the group, as we may
	     have raced with the inferior simply exiting.  */
	  && num_lwps (inf->pid) > 1
	  && linux_proc_pid_is_zombie (inf->pid))
	{
	  linux_nat_debug_printf ("Thread group leader %d zombie "
				  "(it exited, or another thread execd).",
				  inf->pid);

	  /* A leader zombie can mean one of two things:

	     - It exited, and there's an exit status pending
	     available, or only the leader exited (not the whole
	     program).  In the latter case, we can't waitpid the
	     leader's exit status until all other threads are gone.

	     - There are 3 or more threads in the group, and a thread
	     other than the leader exec'd.  See comments on exec
	     events at the top of the file.  We could try
	     distinguishing the exit and exec cases, by waiting once
	     more, and seeing if something comes out, but it doesn't
	     sound useful.  The previous leader _does_ go away, and
	     we'll re-add the new one once we see the exec event
	     (which is just the same as what would happen if the
	     previous leader did exit voluntarily before some other
	     thread execs).  */

	  linux_nat_debug_printf ("Thread group leader %d vanished.", inf->pid);
	  exit_lwp (leader_lp);
	}
    }
}

/* Convenience function that is called when the kernel reports an exit
   event.  This decides whether to report the event to GDB as a
   process exit event, a thread exit event, or to suppress the
   event.  */

static ptid_t
filter_exit_event (struct lwp_info *event_child,
		   struct target_waitstatus *ourstatus)
{
  ptid_t ptid = event_child->ptid;

  if (num_lwps (ptid.pid ()) > 1)
    {
      if (report_thread_events)
	ourstatus->kind = TARGET_WAITKIND_THREAD_EXITED;
      else
	ourstatus->kind = TARGET_WAITKIND_IGNORE;

      exit_lwp (event_child);
    }

  return ptid;
}

static ptid_t
linux_nat_wait_1 (ptid_t ptid, struct target_waitstatus *ourstatus,
		  int target_options)
{
  sigset_t prev_mask;
  enum resume_kind last_resume_kind;
  struct lwp_info *lp;
  int status;

  linux_nat_debug_printf ("enter");

  /* The first time we get here after starting a new inferior, we may
     not have added it to the LWP list yet - this is the earliest
     moment at which we know its PID.  */
  if (inferior_ptid.is_pid ())
    {
      /* Upgrade the main thread's ptid.  */
      thread_change_ptid (linux_target, inferior_ptid,
			  ptid_t (inferior_ptid.pid (),
				  inferior_ptid.pid (), 0));

      lp = add_initial_lwp (inferior_ptid);
      lp->resumed = 1;
    }

  /* Make sure SIGCHLD is blocked until the sigsuspend below.  */
  block_child_signals (&prev_mask);

  /* First check if there is a LWP with a wait status pending.  */
  lp = iterate_over_lwps (ptid, status_callback);
  if (lp != NULL)
    {
      linux_nat_debug_printf ("Using pending wait status %s for %s.",
			      status_to_str (lp->status),
			      target_pid_to_str (lp->ptid).c_str ());
    }

  /* But if we don't find a pending event, we'll have to wait.  Always
     pull all events out of the kernel.  We'll randomly select an
     event LWP out of all that have events, to prevent starvation.  */

  while (lp == NULL)
    {
      pid_t lwpid;

      /* Always use -1 and WNOHANG, due to couple of a kernel/ptrace
	 quirks:

	 - If the thread group leader exits while other threads in the
	   thread group still exist, waitpid(TGID, ...) hangs.  That
	   waitpid won't return an exit status until the other threads
	   in the group are reaped.

	 - When a non-leader thread execs, that thread just vanishes
	   without reporting an exit (so we'd hang if we waited for it
	   explicitly in that case).  The exec event is reported to
	   the TGID pid.  */

      errno = 0;
      lwpid = my_waitpid (-1, &status,  __WALL | WNOHANG);

      linux_nat_debug_printf ("waitpid(-1, ...) returned %d, %s",
			      lwpid,
			      errno ? safe_strerror (errno) : "ERRNO-OK");

      if (lwpid > 0)
	{
	  linux_nat_debug_printf ("waitpid %ld received %s",
				  (long) lwpid, status_to_str (status));

	  linux_nat_filter_event (lwpid, status);
	  /* Retry until nothing comes out of waitpid.  A single
	     SIGCHLD can indicate more than one child stopped.  */
	  continue;
	}

      /* Now that we've pulled all events out of the kernel, resume
	 LWPs that don't have an interesting event to report.  */
      iterate_over_lwps (minus_one_ptid,
			 [] (struct lwp_info *info)
			 {
			   return resume_stopped_resumed_lwps (info, minus_one_ptid);
			 });

      /* ... and find an LWP with a status to report to the core, if
	 any.  */
      lp = iterate_over_lwps (ptid, status_callback);
      if (lp != NULL)
	break;

      /* Check for zombie thread group leaders.  Those can't be reaped
	 until all other threads in the thread group are.  */
      check_zombie_leaders ();

      /* If there are no resumed children left, bail.  We'd be stuck
	 forever in the sigsuspend call below otherwise.  */
      if (iterate_over_lwps (ptid, resumed_callback) == NULL)
	{
	  linux_nat_debug_printf ("exit (no resumed LWP)");

	  ourstatus->kind = TARGET_WAITKIND_NO_RESUMED;

	  restore_child_signals_mask (&prev_mask);
	  return minus_one_ptid;
	}

      /* No interesting event to report to the core.  */

      if (target_options & TARGET_WNOHANG)
	{
	  linux_nat_debug_printf ("exit (ignore)");

	  ourstatus->kind = TARGET_WAITKIND_IGNORE;
	  restore_child_signals_mask (&prev_mask);
	  return minus_one_ptid;
	}

      /* We shouldn't end up here unless we want to try again.  */
      gdb_assert (lp == NULL);

      /* Block until we get an event reported with SIGCHLD.  */
      wait_for_signal ();
    }

  gdb_assert (lp);

  status = lp->status;
  lp->status = 0;

  if (!target_is_non_stop_p ())
    {
      /* Now stop all other LWP's ...  */
      iterate_over_lwps (minus_one_ptid, stop_callback);

      /* ... and wait until all of them have reported back that
	 they're no longer running.  */
      iterate_over_lwps (minus_one_ptid, stop_wait_callback);
    }

  /* If we're not waiting for a specific LWP, choose an event LWP from
     among those that have had events.  Giving equal priority to all
     LWPs that have had events helps prevent starvation.  */
  if (ptid == minus_one_ptid || ptid.is_pid ())
    select_event_lwp (ptid, &lp, &status);

  gdb_assert (lp != NULL);

  /* Now that we've selected our final event LWP, un-adjust its PC if
     it was a software breakpoint, and we can't reliably support the
     "stopped by software breakpoint" stop reason.  */
  if (lp->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT
      && !USE_SIGTRAP_SIGINFO)
    {
      struct regcache *regcache = get_thread_regcache (linux_target, lp->ptid);
      struct gdbarch *gdbarch = regcache->arch ();
      int decr_pc = gdbarch_decr_pc_after_break (gdbarch);

      if (decr_pc != 0)
	{
	  CORE_ADDR pc;

	  pc = regcache_read_pc (regcache);
	  regcache_write_pc (regcache, pc + decr_pc);
	}
    }

  /* We'll need this to determine whether to report a SIGSTOP as
     GDB_SIGNAL_0.  Need to take a copy because resume_clear_callback
     clears it.  */
  last_resume_kind = lp->last_resume_kind;

  if (!target_is_non_stop_p ())
    {
      /* In all-stop, from the core's perspective, all LWPs are now
	 stopped until a new resume action is sent over.  */
      iterate_over_lwps (minus_one_ptid, resume_clear_callback);
    }
  else
    {
      resume_clear_callback (lp);
    }

  if (linux_target->low_status_is_event (status))
    {
      linux_nat_debug_printf ("trap ptid is %s.",
			      target_pid_to_str (lp->ptid).c_str ());
    }

  if (lp->waitstatus.kind != TARGET_WAITKIND_IGNORE)
    {
      *ourstatus = lp->waitstatus;
      lp->waitstatus.kind = TARGET_WAITKIND_IGNORE;
    }
  else
    store_waitstatus (ourstatus, status);

  linux_nat_debug_printf ("exit");

  restore_child_signals_mask (&prev_mask);

  if (last_resume_kind == resume_stop
      && ourstatus->kind == TARGET_WAITKIND_STOPPED
      && WSTOPSIG (status) == SIGSTOP)
    {
      /* A thread that has been requested to stop by GDB with
	 target_stop, and it stopped cleanly, so report as SIG0.  The
	 use of SIGSTOP is an implementation detail.  */
      ourstatus->value.sig = GDB_SIGNAL_0;
    }

  if (ourstatus->kind == TARGET_WAITKIND_EXITED
      || ourstatus->kind == TARGET_WAITKIND_SIGNALLED)
    lp->core = -1;
  else
    lp->core = linux_common_core_of_thread (lp->ptid);

  if (ourstatus->kind == TARGET_WAITKIND_EXITED)
    return filter_exit_event (lp, ourstatus);

  return lp->ptid;
}

/* Resume LWPs that are currently stopped without any pending status
   to report, but are resumed from the core's perspective.  */

static int
resume_stopped_resumed_lwps (struct lwp_info *lp, const ptid_t wait_ptid)
{
  if (!lp->stopped)
    {
      linux_nat_debug_printf ("NOT resuming LWP %s, not stopped",
			      target_pid_to_str (lp->ptid).c_str ());
    }
  else if (!lp->resumed)
    {
      linux_nat_debug_printf ("NOT resuming LWP %s, not resumed",
			      target_pid_to_str (lp->ptid).c_str ());
    }
  else if (lwp_status_pending_p (lp))
    {
      linux_nat_debug_printf ("NOT resuming LWP %s, has pending status",
			      target_pid_to_str (lp->ptid).c_str ());
    }
  else
    {
      struct regcache *regcache = get_thread_regcache (linux_target, lp->ptid);
      struct gdbarch *gdbarch = regcache->arch ();

      try
	{
	  CORE_ADDR pc = regcache_read_pc (regcache);
	  int leave_stopped = 0;

	  /* Don't bother if there's a breakpoint at PC that we'd hit
	     immediately, and we're not waiting for this LWP.  */
	  if (!lp->ptid.matches (wait_ptid))
	    {
	      if (breakpoint_inserted_here_p (regcache->aspace (), pc))
		leave_stopped = 1;
	    }

	  if (!leave_stopped)
	    {
	      linux_nat_debug_printf
		("resuming stopped-resumed LWP %s at %s: step=%d",
		 target_pid_to_str (lp->ptid).c_str (), paddress (gdbarch, pc),
		 lp->step);

	      linux_resume_one_lwp_throw (lp, lp->step, GDB_SIGNAL_0);
	    }
	}
      catch (const gdb_exception_error &ex)
	{
	  if (!check_ptrace_stopped_lwp_gone (lp))
	    throw;
	}
    }

  return 0;
}

ptid_t
linux_nat_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
			int target_options)
{
  ptid_t event_ptid;

  linux_nat_debug_printf ("[%s], [%s]", target_pid_to_str (ptid).c_str (),
			  target_options_to_string (target_options).c_str ());

  /* Flush the async file first.  */
  if (target_is_async_p ())
    async_file_flush ();

  /* Resume LWPs that are currently stopped without any pending status
     to report, but are resumed from the core's perspective.  LWPs get
     in this state if we find them stopping at a time we're not
     interested in reporting the event (target_wait on a
     specific_process, for example, see linux_nat_wait_1), and
     meanwhile the event became uninteresting.  Don't bother resuming
     LWPs we're not going to wait for if they'd stop immediately.  */
  if (target_is_non_stop_p ())
    iterate_over_lwps (minus_one_ptid,
		       [=] (struct lwp_info *info)
		       {
			 return resume_stopped_resumed_lwps (info, ptid);
		       });

  event_ptid = linux_nat_wait_1 (ptid, ourstatus, target_options);

  /* If we requested any event, and something came out, assume there
     may be more.  If we requested a specific lwp or process, also
     assume there may be more.  */
  if (target_is_async_p ()
      && ((ourstatus->kind != TARGET_WAITKIND_IGNORE
	   && ourstatus->kind != TARGET_WAITKIND_NO_RESUMED)
	  || ptid != minus_one_ptid))
    async_file_mark ();

  return event_ptid;
}

/* Kill one LWP.  */

static void
kill_one_lwp (pid_t pid)
{
  /* PTRACE_KILL may resume the inferior.  Send SIGKILL first.  */

  errno = 0;
  kill_lwp (pid, SIGKILL);

  if (debug_linux_nat)
    {
      int save_errno = errno;

      linux_nat_debug_printf
	("kill (SIGKILL) %ld, 0, 0 (%s)", (long) pid,
	 save_errno != 0 ? safe_strerror (save_errno) : "OK");
    }

  /* Some kernels ignore even SIGKILL for processes under ptrace.  */

  errno = 0;
  ptrace (PTRACE_KILL, pid, 0, 0);
  if (debug_linux_nat)
    {
      int save_errno = errno;

      linux_nat_debug_printf
	("PTRACE_KILL %ld, 0, 0 (%s)", (long) pid,
	 save_errno ? safe_strerror (save_errno) : "OK");
    }
}

/* Wait for an LWP to die.  */

static void
kill_wait_one_lwp (pid_t pid)
{
  pid_t res;

  /* We must make sure that there are no pending events (delayed
     SIGSTOPs, pending SIGTRAPs, etc.) to make sure the current
     program doesn't interfere with any following debugging session.  */

  do
    {
      res = my_waitpid (pid, NULL, __WALL);
      if (res != (pid_t) -1)
	{
	  linux_nat_debug_printf ("wait %ld received unknown.", (long) pid);

	  /* The Linux kernel sometimes fails to kill a thread
	     completely after PTRACE_KILL; that goes from the stop
	     point in do_fork out to the one in get_signal_to_deliver
	     and waits again.  So kill it again.  */
	  kill_one_lwp (pid);
	}
    }
  while (res == pid);

  gdb_assert (res == -1 && errno == ECHILD);
}

/* Callback for iterate_over_lwps.  */

static int
kill_callback (struct lwp_info *lp)
{
  kill_one_lwp (lp->ptid.lwp ());
  return 0;
}

/* Callback for iterate_over_lwps.  */

static int
kill_wait_callback (struct lwp_info *lp)
{
  kill_wait_one_lwp (lp->ptid.lwp ());
  return 0;
}

/* Kill the fork children of any threads of inferior INF that are
   stopped at a fork event.  */

static void
kill_unfollowed_fork_children (struct inferior *inf)
{
  for (thread_info *thread : inf->non_exited_threads ())
    {
      struct target_waitstatus *ws = &thread->pending_follow;

      if (ws->kind == TARGET_WAITKIND_FORKED
	  || ws->kind == TARGET_WAITKIND_VFORKED)
	{
	  ptid_t child_ptid = ws->value.related_pid;
	  int child_pid = child_ptid.pid ();
	  int child_lwp = child_ptid.lwp ();

	  kill_one_lwp (child_lwp);
	  kill_wait_one_lwp (child_lwp);

	  /* Let the arch-specific native code know this process is
	     gone.  */
	  linux_target->low_forget_process (child_pid);
	}
    }
}

void
linux_nat_target::kill ()
{
  /* If we're stopped while forking and we haven't followed yet,
     kill the other task.  We need to do this first because the
     parent will be sleeping if this is a vfork.  */
  kill_unfollowed_fork_children (current_inferior ());

  if (forks_exist_p ())
    linux_fork_killall ();
  else
    {
      ptid_t ptid = ptid_t (inferior_ptid.pid ());

      /* Stop all threads before killing them, since ptrace requires
	 that the thread is stopped to successfully PTRACE_KILL.  */
      iterate_over_lwps (ptid, stop_callback);
      /* ... and wait until all of them have reported back that
	 they're no longer running.  */
      iterate_over_lwps (ptid, stop_wait_callback);

      /* Kill all LWP's ...  */
      iterate_over_lwps (ptid, kill_callback);

      /* ... and wait until we've flushed all events.  */
      iterate_over_lwps (ptid, kill_wait_callback);
    }

  target_mourn_inferior (inferior_ptid);
}

void
linux_nat_target::mourn_inferior ()
{
  int pid = inferior_ptid.pid ();

  purge_lwp_list (pid);

  if (! forks_exist_p ())
    /* Normal case, no other forks available.  */
    inf_ptrace_target::mourn_inferior ();
  else
    /* Multi-fork case.  The current inferior_ptid has exited, but
       there are other viable forks to debug.  Delete the exiting
       one and context-switch to the first available.  */
    linux_fork_mourn_inferior ();

  /* Let the arch-specific native code know this process is gone.  */
  linux_target->low_forget_process (pid);
}

/* Convert a native/host siginfo object, into/from the siginfo in the
   layout of the inferiors' architecture.  */

static void
siginfo_fixup (siginfo_t *siginfo, gdb_byte *inf_siginfo, int direction)
{
  /* If the low target didn't do anything, then just do a straight
     memcpy.  */
  if (!linux_target->low_siginfo_fixup (siginfo, inf_siginfo, direction))
    {
      if (direction == 1)
	memcpy (siginfo, inf_siginfo, sizeof (siginfo_t));
      else
	memcpy (inf_siginfo, siginfo, sizeof (siginfo_t));
    }
}

static enum target_xfer_status
linux_xfer_siginfo (enum target_object object,
                    const char *annex, gdb_byte *readbuf,
		    const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
		    ULONGEST *xfered_len)
{
  int pid;
  siginfo_t siginfo;
  gdb_byte inf_siginfo[sizeof (siginfo_t)];

  gdb_assert (object == TARGET_OBJECT_SIGNAL_INFO);
  gdb_assert (readbuf || writebuf);

  pid = inferior_ptid.lwp ();
  if (pid == 0)
    pid = inferior_ptid.pid ();

  if (offset > sizeof (siginfo))
    return TARGET_XFER_E_IO;

  errno = 0;
  ptrace (PTRACE_GETSIGINFO, pid, (PTRACE_TYPE_ARG3) 0, &siginfo);
  if (errno != 0)
    return TARGET_XFER_E_IO;

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
	return TARGET_XFER_E_IO;
    }

  *xfered_len = len;
  return TARGET_XFER_OK;
}

static enum target_xfer_status
linux_nat_xfer_osdata (enum target_object object,
		       const char *annex, gdb_byte *readbuf,
		       const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
		       ULONGEST *xfered_len);

static enum target_xfer_status
linux_proc_xfer_partial (enum target_object object,
			 const char *annex, gdb_byte *readbuf,
			 const gdb_byte *writebuf,
			 ULONGEST offset, LONGEST len, ULONGEST *xfered_len);

enum target_xfer_status
linux_nat_target::xfer_partial (enum target_object object,
				const char *annex, gdb_byte *readbuf,
				const gdb_byte *writebuf,
				ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  enum target_xfer_status xfer;

  if (object == TARGET_OBJECT_SIGNAL_INFO)
    return linux_xfer_siginfo (object, annex, readbuf, writebuf,
			       offset, len, xfered_len);

  /* The target is connected but no live inferior is selected.  Pass
     this request down to a lower stratum (e.g., the executable
     file).  */
  if (object == TARGET_OBJECT_MEMORY && inferior_ptid == null_ptid)
    return TARGET_XFER_EOF;

  if (object == TARGET_OBJECT_AUXV)
    return memory_xfer_auxv (this, object, annex, readbuf, writebuf,
			     offset, len, xfered_len);

  if (object == TARGET_OBJECT_OSDATA)
    return linux_nat_xfer_osdata (object, annex, readbuf, writebuf,
				  offset, len, xfered_len);

  /* GDB calculates all addresses in the largest possible address
     width.
     The address width must be masked before its final use - either by
     linux_proc_xfer_partial or inf_ptrace_target::xfer_partial.

     Compare ADDR_BIT first to avoid a compiler warning on shift overflow.  */

  if (object == TARGET_OBJECT_MEMORY)
    {
      int addr_bit = gdbarch_addr_bit (target_gdbarch ());

      if (addr_bit < (sizeof (ULONGEST) * HOST_CHAR_BIT))
	offset &= ((ULONGEST) 1 << addr_bit) - 1;
    }

  xfer = linux_proc_xfer_partial (object, annex, readbuf, writebuf,
				  offset, len, xfered_len);
  if (xfer != TARGET_XFER_EOF)
    return xfer;

  return inf_ptrace_target::xfer_partial (object, annex, readbuf, writebuf,
					  offset, len, xfered_len);
}

bool
linux_nat_target::thread_alive (ptid_t ptid)
{
  /* As long as a PTID is in lwp list, consider it alive.  */
  return find_lwp_pid (ptid) != NULL;
}

/* Implement the to_update_thread_list target method for this
   target.  */

void
linux_nat_target::update_thread_list ()
{
  struct lwp_info *lwp;

  /* We add/delete threads from the list as clone/exit events are
     processed, so just try deleting exited threads still in the
     thread list.  */
  delete_exited_threads ();

  /* Update the processor core that each lwp/thread was last seen
     running on.  */
  ALL_LWPS (lwp)
    {
      /* Avoid accessing /proc if the thread hasn't run since we last
	 time we fetched the thread's core.  Accessing /proc becomes
	 noticeably expensive when we have thousands of LWPs.  */
      if (lwp->core == -1)
	lwp->core = linux_common_core_of_thread (lwp->ptid);
    }
}

std::string
linux_nat_target::pid_to_str (ptid_t ptid)
{
  if (ptid.lwp_p ()
      && (ptid.pid () != ptid.lwp ()
	  || num_lwps (ptid.pid ()) > 1))
    return string_printf ("LWP %ld", ptid.lwp ());

  return normal_pid_to_str (ptid);
}

const char *
linux_nat_target::thread_name (struct thread_info *thr)
{
  return linux_proc_tid_get_name (thr->ptid);
}

/* Accepts an integer PID; Returns a string representing a file that
   can be opened to get the symbols for the child process.  */

char *
linux_nat_target::pid_to_exec_file (int pid)
{
  return linux_proc_pid_to_exec_file (pid);
}

/* Implement the to_xfer_partial target method using /proc/<pid>/mem.
   Because we can use a single read/write call, this can be much more
   efficient than banging away at PTRACE_PEEKTEXT.  */

static enum target_xfer_status
linux_proc_xfer_partial (enum target_object object,
			 const char *annex, gdb_byte *readbuf,
			 const gdb_byte *writebuf,
			 ULONGEST offset, LONGEST len, ULONGEST *xfered_len)
{
  LONGEST ret;
  int fd;
  char filename[64];

  if (object != TARGET_OBJECT_MEMORY)
    return TARGET_XFER_EOF;

  /* Don't bother for one word.  */
  if (len < 3 * sizeof (long))
    return TARGET_XFER_EOF;

  /* We could keep this file open and cache it - possibly one per
     thread.  That requires some juggling, but is even faster.  */
  xsnprintf (filename, sizeof filename, "/proc/%ld/mem",
	     inferior_ptid.lwp ());
  fd = gdb_open_cloexec (filename, ((readbuf ? O_RDONLY : O_WRONLY)
				    | O_LARGEFILE), 0);
  if (fd == -1)
    return TARGET_XFER_EOF;

  /* Use pread64/pwrite64 if available, since they save a syscall and can
     handle 64-bit offsets even on 32-bit platforms (for instance, SPARC
     debugging a SPARC64 application).  */
#ifdef HAVE_PREAD64
  ret = (readbuf ? pread64 (fd, readbuf, len, offset)
	 : pwrite64 (fd, writebuf, len, offset));
#else
  ret = lseek (fd, offset, SEEK_SET);
  if (ret != -1)
    ret = (readbuf ? read (fd, readbuf, len)
	   : write (fd, writebuf, len));
#endif

  close (fd);

  if (ret == -1 || ret == 0)
    return TARGET_XFER_EOF;
  else
    {
      *xfered_len = ret;
      return TARGET_XFER_OK;
    }
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
  char buffer[PATH_MAX], fname[PATH_MAX];

  sigemptyset (pending);
  sigemptyset (blocked);
  sigemptyset (ignored);
  xsnprintf (fname, sizeof fname, "/proc/%d/status", pid);
  gdb_file_up procfile = gdb_fopen_cloexec (fname, "r");
  if (procfile == NULL)
    error (_("Could not open %s"), fname);

  while (fgets (buffer, PATH_MAX, procfile.get ()) != NULL)
    {
      /* Normal queued signals are on the SigPnd line in the status
	 file.  However, 2.6 kernels also have a "shared" pending
	 queue for delivering signals to a thread group, so check for
	 a ShdPnd line also.

	 Unfortunately some Red Hat kernels include the shared pending
	 queue but not the ShdPnd status field.  */

      if (startswith (buffer, "SigPnd:\t"))
	add_line_to_sigset (buffer + 8, pending);
      else if (startswith (buffer, "ShdPnd:\t"))
	add_line_to_sigset (buffer + 8, pending);
      else if (startswith (buffer, "SigBlk:\t"))
	add_line_to_sigset (buffer + 8, blocked);
      else if (startswith (buffer, "SigIgn:\t"))
	add_line_to_sigset (buffer + 8, ignored);
    }
}

static enum target_xfer_status
linux_nat_xfer_osdata (enum target_object object,
		       const char *annex, gdb_byte *readbuf,
		       const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
		       ULONGEST *xfered_len)
{
  gdb_assert (object == TARGET_OBJECT_OSDATA);

  *xfered_len = linux_common_xfer_osdata (annex, readbuf, offset, len);
  if (*xfered_len == 0)
    return TARGET_XFER_EOF;
  else
    return TARGET_XFER_OK;
}

std::vector<static_tracepoint_marker>
linux_nat_target::static_tracepoint_markers_by_strid (const char *strid)
{
  char s[IPA_CMD_BUF_SIZE];
  int pid = inferior_ptid.pid ();
  std::vector<static_tracepoint_marker> markers;
  const char *p = s;
  ptid_t ptid = ptid_t (pid, 0, 0);
  static_tracepoint_marker marker;

  /* Pause all */
  target_stop (ptid);

  memcpy (s, "qTfSTM", sizeof ("qTfSTM"));
  s[sizeof ("qTfSTM")] = 0;

  agent_run_command (pid, s, strlen (s) + 1);

  /* Unpause all.  */
  SCOPE_EXIT { target_continue_no_signal (ptid); };

  while (*p++ == 'm')
    {
      do
	{
	  parse_static_tracepoint_marker_definition (p, &p, &marker);

	  if (strid == NULL || marker.str_id == strid)
	    markers.push_back (std::move (marker));
	}
      while (*p++ == ',');	/* comma-separated list */

      memcpy (s, "qTsSTM", sizeof ("qTsSTM"));
      s[sizeof ("qTsSTM")] = 0;
      agent_run_command (pid, s, strlen (s) + 1);
      p = s;
    }

  return markers;
}

/* target_is_async_p implementation.  */

bool
linux_nat_target::is_async_p ()
{
  return linux_is_async_p ();
}

/* target_can_async_p implementation.  */

bool
linux_nat_target::can_async_p ()
{
  /* We're always async, unless the user explicitly prevented it with the
     "maint set target-async" command.  */
  return target_async_permitted;
}

bool
linux_nat_target::supports_non_stop ()
{
  return true;
}

/* to_always_non_stop_p implementation.  */

bool
linux_nat_target::always_non_stop_p ()
{
  return true;
}

bool
linux_nat_target::supports_multi_process ()
{
  return true;
}

bool
linux_nat_target::supports_disable_randomization ()
{
#ifdef HAVE_PERSONALITY
  return true;
#else
  return false;
#endif
}

/* SIGCHLD handler that serves two purposes: In non-stop/async mode,
   so we notice when any child changes state, and notify the
   event-loop; it allows us to use sigsuspend in linux_nat_wait_1
   above to wait for the arrival of a SIGCHLD.  */

static void
sigchld_handler (int signo)
{
  int old_errno = errno;

  if (debug_linux_nat)
    gdb_stdlog->write_async_safe ("sigchld\n", sizeof ("sigchld\n") - 1);

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
  inferior_event_handler (INF_REG_EVENT);
}

/* Create/destroy the target events pipe.  Returns previous state.  */

static int
linux_async_pipe (int enable)
{
  int previous = linux_is_async_p ();

  if (previous != enable)
    {
      sigset_t prev_mask;

      /* Block child signals while we create/destroy the pipe, as
	 their handler writes to it.  */
      block_child_signals (&prev_mask);

      if (enable)
	{
	  if (gdb_pipe_cloexec (linux_nat_event_pipe) == -1)
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

int
linux_nat_target::async_wait_fd ()
{
  return linux_nat_event_pipe[0];
}

/* target_async implementation.  */

void
linux_nat_target::async (int enable)
{
  if (enable)
    {
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
      delete_file_handler (linux_nat_event_pipe[0]);
      linux_async_pipe (0);
    }
  return;
}

/* Stop an LWP, and push a GDB_SIGNAL_0 stop status if no other
   event came out.  */

static int
linux_nat_stop_lwp (struct lwp_info *lwp)
{
  if (!lwp->stopped)
    {
      linux_nat_debug_printf ("running -> suspending %s",
			      target_pid_to_str (lwp->ptid).c_str ());


      if (lwp->last_resume_kind == resume_stop)
	{
	  linux_nat_debug_printf ("already stopping LWP %ld at GDB's request",
				  lwp->ptid.lwp ());
	  return 0;
	}

      stop_callback (lwp);
      lwp->last_resume_kind = resume_stop;
    }
  else
    {
      /* Already known to be stopped; do nothing.  */

      if (debug_linux_nat)
	{
	  if (find_thread_ptid (linux_target, lwp->ptid)->stop_requested)
	    linux_nat_debug_printf ("already stopped/stop_requested %s",
				    target_pid_to_str (lwp->ptid).c_str ());
	  else
	    linux_nat_debug_printf ("already stopped/no stop_requested yet %s",
				    target_pid_to_str (lwp->ptid).c_str ());
	}
    }
  return 0;
}

void
linux_nat_target::stop (ptid_t ptid)
{
  iterate_over_lwps (ptid, linux_nat_stop_lwp);
}

void
linux_nat_target::close ()
{
  /* Unregister from the event loop.  */
  if (is_async_p ())
    async (0);

  inf_ptrace_target::close ();
}

/* When requests are passed down from the linux-nat layer to the
   single threaded inf-ptrace layer, ptids of (lwpid,0,0) form are
   used.  The address space pointer is stored in the inferior object,
   but the common code that is passed such ptid can't tell whether
   lwpid is a "main" process id or not (it assumes so).  We reverse
   look up the "main" process id from the lwp here.  */

struct address_space *
linux_nat_target::thread_address_space (ptid_t ptid)
{
  struct lwp_info *lwp;
  struct inferior *inf;
  int pid;

  if (ptid.lwp () == 0)
    {
      /* An (lwpid,0,0) ptid.  Look up the lwp object to get at the
	 tgid.  */
      lwp = find_lwp_pid (ptid);
      pid = lwp->ptid.pid ();
    }
  else
    {
      /* A (pid,lwpid,0) ptid.  */
      pid = ptid.pid ();
    }

  inf = find_inferior_pid (this, pid);
  gdb_assert (inf != NULL);
  return inf->aspace;
}

/* Return the cached value of the processor core for thread PTID.  */

int
linux_nat_target::core_of_thread (ptid_t ptid)
{
  struct lwp_info *info = find_lwp_pid (ptid);

  if (info)
    return info->core;
  return -1;
}

/* Implementation of to_filesystem_is_local.  */

bool
linux_nat_target::filesystem_is_local ()
{
  struct inferior *inf = current_inferior ();

  if (inf->fake_pid_p || inf->pid == 0)
    return true;

  return linux_ns_same (inf->pid, LINUX_NS_MNT);
}

/* Convert the INF argument passed to a to_fileio_* method
   to a process ID suitable for passing to its corresponding
   linux_mntns_* function.  If INF is non-NULL then the
   caller is requesting the filesystem seen by INF.  If INF
   is NULL then the caller is requesting the filesystem seen
   by the GDB.  We fall back to GDB's filesystem in the case
   that INF is non-NULL but its PID is unknown.  */

static pid_t
linux_nat_fileio_pid_of (struct inferior *inf)
{
  if (inf == NULL || inf->fake_pid_p || inf->pid == 0)
    return getpid ();
  else
    return inf->pid;
}

/* Implementation of to_fileio_open.  */

int
linux_nat_target::fileio_open (struct inferior *inf, const char *filename,
			       int flags, int mode, int warn_if_slow,
			       int *target_errno)
{
  int nat_flags;
  mode_t nat_mode;
  int fd;

  if (fileio_to_host_openflags (flags, &nat_flags) == -1
      || fileio_to_host_mode (mode, &nat_mode) == -1)
    {
      *target_errno = FILEIO_EINVAL;
      return -1;
    }

  fd = linux_mntns_open_cloexec (linux_nat_fileio_pid_of (inf),
				 filename, nat_flags, nat_mode);
  if (fd == -1)
    *target_errno = host_to_fileio_error (errno);

  return fd;
}

/* Implementation of to_fileio_readlink.  */

gdb::optional<std::string>
linux_nat_target::fileio_readlink (struct inferior *inf, const char *filename,
				   int *target_errno)
{
  char buf[PATH_MAX];
  int len;

  len = linux_mntns_readlink (linux_nat_fileio_pid_of (inf),
			      filename, buf, sizeof (buf));
  if (len < 0)
    {
      *target_errno = host_to_fileio_error (errno);
      return {};
    }

  return std::string (buf, len);
}

/* Implementation of to_fileio_unlink.  */

int
linux_nat_target::fileio_unlink (struct inferior *inf, const char *filename,
				 int *target_errno)
{
  int ret;

  ret = linux_mntns_unlink (linux_nat_fileio_pid_of (inf),
			    filename);
  if (ret == -1)
    *target_errno = host_to_fileio_error (errno);

  return ret;
}

/* Implementation of the to_thread_events method.  */

void
linux_nat_target::thread_events (int enable)
{
  report_thread_events = enable;
}

linux_nat_target::linux_nat_target ()
{
  /* We don't change the stratum; this target will sit at
     process_stratum and thread_db will set at thread_stratum.  This
     is a little strange, since this is a multi-threaded-capable
     target, but we want to be on the stack below thread_db, and we
     also want to be used for single-threaded processes.  */
}

/* See linux-nat.h.  */

int
linux_nat_get_siginfo (ptid_t ptid, siginfo_t *siginfo)
{
  int pid;

  pid = ptid.lwp ();
  if (pid == 0)
    pid = ptid.pid ();

  errno = 0;
  ptrace (PTRACE_GETSIGINFO, pid, (PTRACE_TYPE_ARG3) 0, siginfo);
  if (errno != 0)
    {
      memset (siginfo, 0, sizeof (*siginfo));
      return 0;
    }
  return 1;
}

/* See nat/linux-nat.h.  */

ptid_t
current_lwp_ptid (void)
{
  gdb_assert (inferior_ptid.lwp_p ());
  return inferior_ptid;
}

void _initialize_linux_nat ();
void
_initialize_linux_nat ()
{
  add_setshow_zuinteger_cmd ("lin-lwp", class_maintenance,
			     &debug_linux_nat, _("\
Set debugging of GNU/Linux lwp module."), _("\
Show debugging of GNU/Linux lwp module."), _("\
Enables printf debugging output."),
			     NULL,
			     show_debug_linux_nat,
			     &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("linux-namespaces", class_maintenance,
			   &debug_linux_namespaces, _("\
Set debugging of GNU/Linux namespaces module."), _("\
Show debugging of GNU/Linux namespaces module."), _("\
Enables printf debugging output."),
			   NULL,
			   NULL,
			   &setdebuglist, &showdebuglist);

  /* Install a SIGCHLD handler.  */
  sigchld_action.sa_handler = sigchld_handler;
  sigemptyset (&sigchld_action.sa_mask);
  sigchld_action.sa_flags = SA_RESTART;

  /* Make it the default.  */
  sigaction (SIGCHLD, &sigchld_action, NULL);

  /* Make sure we don't block SIGCHLD during a sigsuspend.  */
  gdb_sigmask (SIG_SETMASK, NULL, &suspend_mask);
  sigdelset (&suspend_mask, SIGCHLD);

  sigemptyset (&blocked_mask);

  lwp_lwpid_htab_create ();
}


/* FIXME: kettenis/2000-08-26: The stuff on this page is specific to
   the GNU/Linux Threads library and therefore doesn't really belong
   here.  */

/* Return the set of signals used by the threads library in *SET.  */

void
lin_thread_get_thread_signals (sigset_t *set)
{
  sigemptyset (set);

  /* NPTL reserves the first two RT signals, but does not provide any
     way for the debugger to query the signal numbers - fortunately
     they don't change.  */
  sigaddset (set, __SIGRTMIN);
  sigaddset (set, __SIGRTMIN + 1);
}
