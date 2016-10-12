/* Low level interface to ptrace, for the remote server for GDB.
   Copyright (C) 1995-2016 Free Software Foundation, Inc.

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

#include "server.h"
#include "linux-low.h"
#include "nat/linux-osdata.h"
#include "agent.h"
#include "tdesc.h"
#include "rsp-low.h"
#include "signals-state-save-restore.h"
#include "nat/linux-nat.h"
#include "nat/linux-waitpid.h"
#include "gdb_wait.h"
#include "nat/gdb_ptrace.h"
#include "nat/linux-ptrace.h"
#include "nat/linux-procfs.h"
#include "nat/linux-personality.h"
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/uio.h>
#include "filestuff.h"
#include "tracepoint.h"
#include "hostio.h"
#include <inttypes.h>
#ifndef ELFMAG0
/* Don't include <linux/elf.h> here.  If it got included by gdb_proc_service.h
   then ELFMAG0 will have been defined.  If it didn't get included by
   gdb_proc_service.h then including it will likely introduce a duplicate
   definition of elf_fpregset_t.  */
#include <elf.h>
#endif
#include "nat/linux-namespaces.h"

#ifndef SPUFS_MAGIC
#define SPUFS_MAGIC 0x23c9b64e
#endif

#ifdef HAVE_PERSONALITY
# include <sys/personality.h>
# if !HAVE_DECL_ADDR_NO_RANDOMIZE
#  define ADDR_NO_RANDOMIZE 0x0040000
# endif
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

/* Some targets did not define these ptrace constants from the start,
   so gdbserver defines them locally here.  In the future, these may
   be removed after they are added to asm/ptrace.h.  */
#if !(defined(PT_TEXT_ADDR) \
      || defined(PT_DATA_ADDR) \
      || defined(PT_TEXT_END_ADDR))
#if defined(__mcoldfire__)
/* These are still undefined in 3.10 kernels.  */
#define PT_TEXT_ADDR 49*4
#define PT_DATA_ADDR 50*4
#define PT_TEXT_END_ADDR  51*4
/* BFIN already defines these since at least 2.6.32 kernels.  */
#elif defined(BFIN)
#define PT_TEXT_ADDR 220
#define PT_TEXT_END_ADDR 224
#define PT_DATA_ADDR 228
/* These are still undefined in 3.10 kernels.  */
#elif defined(__TMS320C6X__)
#define PT_TEXT_ADDR     (0x10000*4)
#define PT_DATA_ADDR     (0x10004*4)
#define PT_TEXT_END_ADDR (0x10008*4)
#endif
#endif

#ifdef HAVE_LINUX_BTRACE
# include "nat/linux-btrace.h"
# include "btrace-common.h"
#endif

#ifndef HAVE_ELF32_AUXV_T
/* Copied from glibc's elf.h.  */
typedef struct
{
  uint32_t a_type;		/* Entry type */
  union
    {
      uint32_t a_val;		/* Integer value */
      /* We use to have pointer elements added here.  We cannot do that,
	 though, since it does not work when using 32-bit definitions
	 on 64-bit platforms and vice versa.  */
    } a_un;
} Elf32_auxv_t;
#endif

#ifndef HAVE_ELF64_AUXV_T
/* Copied from glibc's elf.h.  */
typedef struct
{
  uint64_t a_type;		/* Entry type */
  union
    {
      uint64_t a_val;		/* Integer value */
      /* We use to have pointer elements added here.  We cannot do that,
	 though, since it does not work when using 32-bit definitions
	 on 64-bit platforms and vice versa.  */
    } a_un;
} Elf64_auxv_t;
#endif

/* Does the current host support PTRACE_GETREGSET?  */
int have_ptrace_getregset = -1;

/* LWP accessors.  */

/* See nat/linux-nat.h.  */

ptid_t
ptid_of_lwp (struct lwp_info *lwp)
{
  return ptid_of (get_lwp_thread (lwp));
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

/* A list of all unknown processes which receive stop signals.  Some
   other process will presumably claim each of these as forked
   children momentarily.  */

struct simple_pid_list
{
  /* The process ID.  */
  int pid;

  /* The status as reported by waitpid.  */
  int status;

  /* Next in chain.  */
  struct simple_pid_list *next;
};
struct simple_pid_list *stopped_pids;

/* Trivial list manipulation functions to keep track of a list of new
   stopped processes.  */

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

enum stopping_threads_kind
  {
    /* Not stopping threads presently.  */
    NOT_STOPPING_THREADS,

    /* Stopping threads.  */
    STOPPING_THREADS,

    /* Stopping and suspending threads.  */
    STOPPING_AND_SUSPENDING_THREADS
  };

/* This is set while stop_all_lwps is in effect.  */
enum stopping_threads_kind stopping_threads = NOT_STOPPING_THREADS;

/* FIXME make into a target method?  */
int using_threads = 1;

/* True if we're presently stabilizing threads (moving them out of
   jump pads).  */
static int stabilizing_threads;

static void linux_resume_one_lwp (struct lwp_info *lwp,
				  int step, int signal, siginfo_t *info);
static void linux_resume (struct thread_resume *resume_info, size_t n);
static void stop_all_lwps (int suspend, struct lwp_info *except);
static void unstop_all_lwps (int unsuspend, struct lwp_info *except);
static void unsuspend_all_lwps (struct lwp_info *except);
static int linux_wait_for_event_filtered (ptid_t wait_ptid, ptid_t filter_ptid,
					  int *wstat, int options);
static int linux_wait_for_event (ptid_t ptid, int *wstat, int options);
static struct lwp_info *add_lwp (ptid_t ptid);
static void linux_mourn (struct process_info *process);
static int linux_stopped_by_watchpoint (void);
static void mark_lwp_dead (struct lwp_info *lwp, int wstat);
static int lwp_is_marked_dead (struct lwp_info *lwp);
static void proceed_all_lwps (void);
static int finish_step_over (struct lwp_info *lwp);
static int kill_lwp (unsigned long lwpid, int signo);
static void enqueue_pending_signal (struct lwp_info *lwp, int signal, siginfo_t *info);
static void complete_ongoing_step_over (void);
static int linux_low_ptrace_options (int attached);
static int check_ptrace_stopped_lwp_gone (struct lwp_info *lp);
static int proceed_one_lwp (struct inferior_list_entry *entry, void *except);

/* When the event-loop is doing a step-over, this points at the thread
   being stepped.  */
ptid_t step_over_bkpt;

/* True if the low target can hardware single-step.  */

static int
can_hardware_single_step (void)
{
  if (the_low_target.supports_hardware_single_step != NULL)
    return the_low_target.supports_hardware_single_step ();
  else
    return 0;
}

/* True if the low target can software single-step.  Such targets
   implement the GET_NEXT_PCS callback.  */

static int
can_software_single_step (void)
{
  return (the_low_target.get_next_pcs != NULL);
}

/* True if the low target supports memory breakpoints.  If so, we'll
   have a GET_PC implementation.  */

static int
supports_breakpoints (void)
{
  return (the_low_target.get_pc != NULL);
}

/* Returns true if this target can support fast tracepoints.  This
   does not mean that the in-process agent has been loaded in the
   inferior.  */

static int
supports_fast_tracepoints (void)
{
  return the_low_target.install_fast_tracepoint_jump_pad != NULL;
}

/* True if LWP is stopped in its stepping range.  */

static int
lwp_in_step_range (struct lwp_info *lwp)
{
  CORE_ADDR pc = lwp->stop_pc;

  return (pc >= lwp->step_range_start && pc < lwp->step_range_end);
}

struct pending_signals
{
  int signal;
  siginfo_t info;
  struct pending_signals *prev;
};

/* The read/write ends of the pipe registered as waitable file in the
   event loop.  */
static int linux_event_pipe[2] = { -1, -1 };

/* True if we're currently in async mode.  */
#define target_is_async_p() (linux_event_pipe[0] != -1)

static void send_sigstop (struct lwp_info *lwp);
static void wait_for_sigstop (void);

/* Return non-zero if HEADER is a 64-bit ELF file.  */

static int
elf_64_header_p (const Elf64_Ehdr *header, unsigned int *machine)
{
  if (header->e_ident[EI_MAG0] == ELFMAG0
      && header->e_ident[EI_MAG1] == ELFMAG1
      && header->e_ident[EI_MAG2] == ELFMAG2
      && header->e_ident[EI_MAG3] == ELFMAG3)
    {
      *machine = header->e_machine;
      return header->e_ident[EI_CLASS] == ELFCLASS64;

    }
  *machine = EM_NONE;
  return -1;
}

/* Return non-zero if FILE is a 64-bit ELF file,
   zero if the file is not a 64-bit ELF file,
   and -1 if the file is not accessible or doesn't exist.  */

static int
elf_64_file_p (const char *file, unsigned int *machine)
{
  Elf64_Ehdr header;
  int fd;

  fd = open (file, O_RDONLY);
  if (fd < 0)
    return -1;

  if (read (fd, &header, sizeof (header)) != sizeof (header))
    {
      close (fd);
      return 0;
    }
  close (fd);

  return elf_64_header_p (&header, machine);
}

/* Accepts an integer PID; Returns true if the executable PID is
   running is a 64-bit ELF file..  */

int
linux_pid_exe_is_elf_64_file (int pid, unsigned int *machine)
{
  char file[PATH_MAX];

  sprintf (file, "/proc/%d/exe", pid);
  return elf_64_file_p (file, machine);
}

static void
delete_lwp (struct lwp_info *lwp)
{
  struct thread_info *thr = get_lwp_thread (lwp);

  if (debug_threads)
    debug_printf ("deleting %ld\n", lwpid_of (thr));

  remove_thread (thr);
  free (lwp->arch_private);
  free (lwp);
}

/* Add a process to the common process list, and set its private
   data.  */

static struct process_info *
linux_add_process (int pid, int attached)
{
  struct process_info *proc;

  proc = add_process (pid, attached);
  proc->priv = XCNEW (struct process_info_private);

  if (the_low_target.new_process != NULL)
    proc->priv->arch_private = the_low_target.new_process ();

  return proc;
}

static CORE_ADDR get_pc (struct lwp_info *lwp);

/* Call the target arch_setup function on the current thread.  */

static void
linux_arch_setup (void)
{
  the_low_target.arch_setup ();
}

/* Call the target arch_setup function on THREAD.  */

static void
linux_arch_setup_thread (struct thread_info *thread)
{
  struct thread_info *saved_thread;

  saved_thread = current_thread;
  current_thread = thread;

  linux_arch_setup ();

  current_thread = saved_thread;
}

/* Handle a GNU/Linux extended wait response.  If we see a clone,
   fork, or vfork event, we need to add the new LWP to our list
   (and return 0 so as not to report the trap to higher layers).
   If we see an exec event, we will modify ORIG_EVENT_LWP to point
   to a new LWP representing the new program.  */

static int
handle_extended_wait (struct lwp_info **orig_event_lwp, int wstat)
{
  struct lwp_info *event_lwp = *orig_event_lwp;
  int event = linux_ptrace_get_extended_event (wstat);
  struct thread_info *event_thr = get_lwp_thread (event_lwp);
  struct lwp_info *new_lwp;

  gdb_assert (event_lwp->waitstatus.kind == TARGET_WAITKIND_IGNORE);

  /* All extended events we currently use are mid-syscall.  Only
     PTRACE_EVENT_STOP is delivered more like a signal-stop, but
     you have to be using PTRACE_SEIZE to get that.  */
  event_lwp->syscall_state = TARGET_WAITKIND_SYSCALL_ENTRY;

  if ((event == PTRACE_EVENT_FORK) || (event == PTRACE_EVENT_VFORK)
      || (event == PTRACE_EVENT_CLONE))
    {
      ptid_t ptid;
      unsigned long new_pid;
      int ret, status;

      /* Get the pid of the new lwp.  */
      ptrace (PTRACE_GETEVENTMSG, lwpid_of (event_thr), (PTRACE_TYPE_ARG3) 0,
	      &new_pid);

      /* If we haven't already seen the new PID stop, wait for it now.  */
      if (!pull_pid_from_list (&stopped_pids, new_pid, &status))
	{
	  /* The new child has a pending SIGSTOP.  We can't affect it until it
	     hits the SIGSTOP, but we're already attached.  */

	  ret = my_waitpid (new_pid, &status, __WALL);

	  if (ret == -1)
	    perror_with_name ("waiting for new child");
	  else if (ret != new_pid)
	    warning ("wait returned unexpected PID %d", ret);
	  else if (!WIFSTOPPED (status))
	    warning ("wait returned unexpected status 0x%x", status);
	}

      if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK)
	{
	  struct process_info *parent_proc;
	  struct process_info *child_proc;
	  struct lwp_info *child_lwp;
	  struct thread_info *child_thr;
	  struct target_desc *tdesc;

	  ptid = ptid_build (new_pid, new_pid, 0);

	  if (debug_threads)
	    {
	      debug_printf ("HEW: Got fork event from LWP %ld, "
			    "new child is %d\n",
			    ptid_get_lwp (ptid_of (event_thr)),
			    ptid_get_pid (ptid));
	    }

	  /* Add the new process to the tables and clone the breakpoint
	     lists of the parent.  We need to do this even if the new process
	     will be detached, since we will need the process object and the
	     breakpoints to remove any breakpoints from memory when we
	     detach, and the client side will access registers.  */
	  child_proc = linux_add_process (new_pid, 0);
	  gdb_assert (child_proc != NULL);
	  child_lwp = add_lwp (ptid);
	  gdb_assert (child_lwp != NULL);
	  child_lwp->stopped = 1;
	  child_lwp->must_set_ptrace_flags = 1;
	  child_lwp->status_pending_p = 0;
	  child_thr = get_lwp_thread (child_lwp);
	  child_thr->last_resume_kind = resume_stop;
	  child_thr->last_status.kind = TARGET_WAITKIND_STOPPED;

	  /* If we're suspending all threads, leave this one suspended
	     too.  If the fork/clone parent is stepping over a breakpoint,
	     all other threads have been suspended already.  Leave the
	     child suspended too.  */
	  if (stopping_threads == STOPPING_AND_SUSPENDING_THREADS
	      || event_lwp->bp_reinsert != 0)
	    {
	      if (debug_threads)
		debug_printf ("HEW: leaving child suspended\n");
	      child_lwp->suspended = 1;
	    }

	  parent_proc = get_thread_process (event_thr);
	  child_proc->attached = parent_proc->attached;

	  if (event_lwp->bp_reinsert != 0
	      && can_software_single_step ()
	      && event == PTRACE_EVENT_VFORK)
	    {
	      /* If we leave reinsert breakpoints there, child will
		 hit it, so uninsert reinsert breakpoints from parent
		 (and child).  Once vfork child is done, reinsert
		 them back to parent.  */
	      uninsert_reinsert_breakpoints (event_thr);
	    }

	  clone_all_breakpoints (child_thr, event_thr);

	  tdesc = XNEW (struct target_desc);
	  copy_target_description (tdesc, parent_proc->tdesc);
	  child_proc->tdesc = tdesc;

	  /* Clone arch-specific process data.  */
	  if (the_low_target.new_fork != NULL)
	    the_low_target.new_fork (parent_proc, child_proc);

	  /* Save fork info in the parent thread.  */
	  if (event == PTRACE_EVENT_FORK)
	    event_lwp->waitstatus.kind = TARGET_WAITKIND_FORKED;
	  else if (event == PTRACE_EVENT_VFORK)
	    event_lwp->waitstatus.kind = TARGET_WAITKIND_VFORKED;

	  event_lwp->waitstatus.value.related_pid = ptid;

	  /* The status_pending field contains bits denoting the
	     extended event, so when the pending event is handled,
	     the handler will look at lwp->waitstatus.  */
	  event_lwp->status_pending_p = 1;
	  event_lwp->status_pending = wstat;

	  /* If the parent thread is doing step-over with reinsert
	     breakpoints, the list of reinsert breakpoints are cloned
	     from the parent's.  Remove them from the child process.
	     In case of vfork, we'll reinsert them back once vforked
	     child is done.  */
	  if (event_lwp->bp_reinsert != 0
	      && can_software_single_step ())
	    {
	      /* The child process is forked and stopped, so it is safe
		 to access its memory without stopping all other threads
		 from other processes.  */
	      delete_reinsert_breakpoints (child_thr);

	      gdb_assert (has_reinsert_breakpoints (event_thr));
	      gdb_assert (!has_reinsert_breakpoints (child_thr));
	    }

	  /* Report the event.  */
	  return 0;
	}

      if (debug_threads)
	debug_printf ("HEW: Got clone event "
		      "from LWP %ld, new child is LWP %ld\n",
		      lwpid_of (event_thr), new_pid);

      ptid = ptid_build (pid_of (event_thr), new_pid, 0);
      new_lwp = add_lwp (ptid);

      /* Either we're going to immediately resume the new thread
	 or leave it stopped.  linux_resume_one_lwp is a nop if it
	 thinks the thread is currently running, so set this first
	 before calling linux_resume_one_lwp.  */
      new_lwp->stopped = 1;

      /* If we're suspending all threads, leave this one suspended
	 too.  If the fork/clone parent is stepping over a breakpoint,
	 all other threads have been suspended already.  Leave the
	 child suspended too.  */
      if (stopping_threads == STOPPING_AND_SUSPENDING_THREADS
	  || event_lwp->bp_reinsert != 0)
	new_lwp->suspended = 1;

      /* Normally we will get the pending SIGSTOP.  But in some cases
	 we might get another signal delivered to the group first.
	 If we do get another signal, be sure not to lose it.  */
      if (WSTOPSIG (status) != SIGSTOP)
	{
	  new_lwp->stop_expected = 1;
	  new_lwp->status_pending_p = 1;
	  new_lwp->status_pending = status;
	}
      else if (report_thread_events)
	{
	  new_lwp->waitstatus.kind = TARGET_WAITKIND_THREAD_CREATED;
	  new_lwp->status_pending_p = 1;
	  new_lwp->status_pending = status;
	}

      /* Don't report the event.  */
      return 1;
    }
  else if (event == PTRACE_EVENT_VFORK_DONE)
    {
      event_lwp->waitstatus.kind = TARGET_WAITKIND_VFORK_DONE;

      if (event_lwp->bp_reinsert != 0 && can_software_single_step ())
	{
	  reinsert_reinsert_breakpoints (event_thr);

	  gdb_assert (has_reinsert_breakpoints (event_thr));
	}

      /* Report the event.  */
      return 0;
    }
  else if (event == PTRACE_EVENT_EXEC && report_exec_events)
    {
      struct process_info *proc;
      VEC (int) *syscalls_to_catch;
      ptid_t event_ptid;
      pid_t event_pid;

      if (debug_threads)
	{
	  debug_printf ("HEW: Got exec event from LWP %ld\n",
			lwpid_of (event_thr));
	}

      /* Get the event ptid.  */
      event_ptid = ptid_of (event_thr);
      event_pid = ptid_get_pid (event_ptid);

      /* Save the syscall list from the execing process.  */
      proc = get_thread_process (event_thr);
      syscalls_to_catch = proc->syscalls_to_catch;
      proc->syscalls_to_catch = NULL;

      /* Delete the execing process and all its threads.  */
      linux_mourn (proc);
      current_thread = NULL;

      /* Create a new process/lwp/thread.  */
      proc = linux_add_process (event_pid, 0);
      event_lwp = add_lwp (event_ptid);
      event_thr = get_lwp_thread (event_lwp);
      gdb_assert (current_thread == event_thr);
      linux_arch_setup_thread (event_thr);

      /* Set the event status.  */
      event_lwp->waitstatus.kind = TARGET_WAITKIND_EXECD;
      event_lwp->waitstatus.value.execd_pathname
	= xstrdup (linux_proc_pid_to_exec_file (lwpid_of (event_thr)));

      /* Mark the exec status as pending.  */
      event_lwp->stopped = 1;
      event_lwp->status_pending_p = 1;
      event_lwp->status_pending = wstat;
      event_thr->last_resume_kind = resume_continue;
      event_thr->last_status.kind = TARGET_WAITKIND_IGNORE;

      /* Update syscall state in the new lwp, effectively mid-syscall too.  */
      event_lwp->syscall_state = TARGET_WAITKIND_SYSCALL_ENTRY;

      /* Restore the list to catch.  Don't rely on the client, which is free
	 to avoid sending a new list when the architecture doesn't change.
	 Also, for ANY_SYSCALL, the architecture doesn't really matter.  */
      proc->syscalls_to_catch = syscalls_to_catch;

      /* Report the event.  */
      *orig_event_lwp = event_lwp;
      return 0;
    }

  internal_error (__FILE__, __LINE__, _("unknown ptrace event %d"), event);
}

/* Return the PC as read from the regcache of LWP, without any
   adjustment.  */

static CORE_ADDR
get_pc (struct lwp_info *lwp)
{
  struct thread_info *saved_thread;
  struct regcache *regcache;
  CORE_ADDR pc;

  if (the_low_target.get_pc == NULL)
    return 0;

  saved_thread = current_thread;
  current_thread = get_lwp_thread (lwp);

  regcache = get_thread_regcache (current_thread, 1);
  pc = (*the_low_target.get_pc) (regcache);

  if (debug_threads)
    debug_printf ("pc is 0x%lx\n", (long) pc);

  current_thread = saved_thread;
  return pc;
}

/* This function should only be called if LWP got a SYSCALL_SIGTRAP.
   Fill *SYSNO with the syscall nr trapped.  */

static void
get_syscall_trapinfo (struct lwp_info *lwp, int *sysno)
{
  struct thread_info *saved_thread;
  struct regcache *regcache;

  if (the_low_target.get_syscall_trapinfo == NULL)
    {
      /* If we cannot get the syscall trapinfo, report an unknown
	 system call number.  */
      *sysno = UNKNOWN_SYSCALL;
      return;
    }

  saved_thread = current_thread;
  current_thread = get_lwp_thread (lwp);

  regcache = get_thread_regcache (current_thread, 1);
  (*the_low_target.get_syscall_trapinfo) (regcache, sysno);

  if (debug_threads)
    debug_printf ("get_syscall_trapinfo sysno %d\n", *sysno);

  current_thread = saved_thread;
}

static int check_stopped_by_watchpoint (struct lwp_info *child);

/* Called when the LWP stopped for a signal/trap.  If it stopped for a
   trap check what caused it (breakpoint, watchpoint, trace, etc.),
   and save the result in the LWP's stop_reason field.  If it stopped
   for a breakpoint, decrement the PC if necessary on the lwp's
   architecture.  Returns true if we now have the LWP's stop PC.  */

static int
save_stop_reason (struct lwp_info *lwp)
{
  CORE_ADDR pc;
  CORE_ADDR sw_breakpoint_pc;
  struct thread_info *saved_thread;
#if USE_SIGTRAP_SIGINFO
  siginfo_t siginfo;
#endif

  if (the_low_target.get_pc == NULL)
    return 0;

  pc = get_pc (lwp);
  sw_breakpoint_pc = pc - the_low_target.decr_pc_after_break;

  /* breakpoint_at reads from the current thread.  */
  saved_thread = current_thread;
  current_thread = get_lwp_thread (lwp);

#if USE_SIGTRAP_SIGINFO
  if (ptrace (PTRACE_GETSIGINFO, lwpid_of (current_thread),
	      (PTRACE_TYPE_ARG3) 0, &siginfo) == 0)
    {
      if (siginfo.si_signo == SIGTRAP)
	{
	  if (GDB_ARCH_IS_TRAP_BRKPT (siginfo.si_code)
	      && GDB_ARCH_IS_TRAP_HWBKPT (siginfo.si_code))
	    {
	      /* The si_code is ambiguous on this arch -- check debug
		 registers.  */
	      if (!check_stopped_by_watchpoint (lwp))
		lwp->stop_reason = TARGET_STOPPED_BY_SW_BREAKPOINT;
	    }
	  else if (GDB_ARCH_IS_TRAP_BRKPT (siginfo.si_code))
	    {
	      /* If we determine the LWP stopped for a SW breakpoint,
		 trust it.  Particularly don't check watchpoint
		 registers, because at least on s390, we'd find
		 stopped-by-watchpoint as long as there's a watchpoint
		 set.  */
	      lwp->stop_reason = TARGET_STOPPED_BY_SW_BREAKPOINT;
	    }
	  else if (GDB_ARCH_IS_TRAP_HWBKPT (siginfo.si_code))
	    {
	      /* This can indicate either a hardware breakpoint or
		 hardware watchpoint.  Check debug registers.  */
	      if (!check_stopped_by_watchpoint (lwp))
		lwp->stop_reason = TARGET_STOPPED_BY_HW_BREAKPOINT;
	    }
	  else if (siginfo.si_code == TRAP_TRACE)
	    {
	      /* We may have single stepped an instruction that
		 triggered a watchpoint.  In that case, on some
		 architectures (such as x86), instead of TRAP_HWBKPT,
		 si_code indicates TRAP_TRACE, and we need to check
		 the debug registers separately.  */
	      if (!check_stopped_by_watchpoint (lwp))
		lwp->stop_reason = TARGET_STOPPED_BY_SINGLE_STEP;
	    }
	}
    }
#else
  /* We may have just stepped a breakpoint instruction.  E.g., in
     non-stop mode, GDB first tells the thread A to step a range, and
     then the user inserts a breakpoint inside the range.  In that
     case we need to report the breakpoint PC.  */
  if ((!lwp->stepping || lwp->stop_pc == sw_breakpoint_pc)
      && (*the_low_target.breakpoint_at) (sw_breakpoint_pc))
    lwp->stop_reason = TARGET_STOPPED_BY_SW_BREAKPOINT;

  if (hardware_breakpoint_inserted_here (pc))
    lwp->stop_reason = TARGET_STOPPED_BY_HW_BREAKPOINT;

  if (lwp->stop_reason == TARGET_STOPPED_BY_NO_REASON)
    check_stopped_by_watchpoint (lwp);
#endif

  if (lwp->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT)
    {
      if (debug_threads)
	{
	  struct thread_info *thr = get_lwp_thread (lwp);

	  debug_printf ("CSBB: %s stopped by software breakpoint\n",
			target_pid_to_str (ptid_of (thr)));
	}

      /* Back up the PC if necessary.  */
      if (pc != sw_breakpoint_pc)
	{
	  struct regcache *regcache
	    = get_thread_regcache (current_thread, 1);
	  (*the_low_target.set_pc) (regcache, sw_breakpoint_pc);
	}

      /* Update this so we record the correct stop PC below.  */
      pc = sw_breakpoint_pc;
    }
  else if (lwp->stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT)
    {
      if (debug_threads)
	{
	  struct thread_info *thr = get_lwp_thread (lwp);

	  debug_printf ("CSBB: %s stopped by hardware breakpoint\n",
			target_pid_to_str (ptid_of (thr)));
	}
    }
  else if (lwp->stop_reason == TARGET_STOPPED_BY_WATCHPOINT)
    {
      if (debug_threads)
	{
	  struct thread_info *thr = get_lwp_thread (lwp);

	  debug_printf ("CSBB: %s stopped by hardware watchpoint\n",
			target_pid_to_str (ptid_of (thr)));
	}
    }
  else if (lwp->stop_reason == TARGET_STOPPED_BY_SINGLE_STEP)
    {
      if (debug_threads)
	{
	  struct thread_info *thr = get_lwp_thread (lwp);

	  debug_printf ("CSBB: %s stopped by trace\n",
			target_pid_to_str (ptid_of (thr)));
	}
    }

  lwp->stop_pc = pc;
  current_thread = saved_thread;
  return 1;
}

static struct lwp_info *
add_lwp (ptid_t ptid)
{
  struct lwp_info *lwp;

  lwp = XCNEW (struct lwp_info);

  lwp->waitstatus.kind = TARGET_WAITKIND_IGNORE;

  if (the_low_target.new_thread != NULL)
    the_low_target.new_thread (lwp);

  lwp->thread = add_thread (ptid, lwp);

  return lwp;
}

/* Start an inferior process and returns its pid.
   ALLARGS is a vector of program-name and args. */

static int
linux_create_inferior (char *program, char **allargs)
{
  struct lwp_info *new_lwp;
  int pid;
  ptid_t ptid;
  struct cleanup *restore_personality
    = maybe_disable_address_space_randomization (disable_randomization);

#if defined(__UCLIBC__) && defined(HAS_NOMMU)
  pid = vfork ();
#else
  pid = fork ();
#endif
  if (pid < 0)
    perror_with_name ("fork");

  if (pid == 0)
    {
      close_most_fds ();
      ptrace (PTRACE_TRACEME, 0, (PTRACE_TYPE_ARG3) 0, (PTRACE_TYPE_ARG4) 0);

      setpgid (0, 0);

      /* If gdbserver is connected to gdb via stdio, redirect the inferior's
	 stdout to stderr so that inferior i/o doesn't corrupt the connection.
	 Also, redirect stdin to /dev/null.  */
      if (remote_connection_is_stdio ())
	{
	  close (0);
	  open ("/dev/null", O_RDONLY);
	  dup2 (2, 1);
	  if (write (2, "stdin/stdout redirected\n",
		     sizeof ("stdin/stdout redirected\n") - 1) < 0)
	    {
	      /* Errors ignored.  */;
	    }
	}

      restore_original_signals_state ();

      execv (program, allargs);
      if (errno == ENOENT)
	execvp (program, allargs);

      fprintf (stderr, "Cannot exec %s: %s.\n", program,
	       strerror (errno));
      fflush (stderr);
      _exit (0177);
    }

  do_cleanups (restore_personality);

  linux_add_process (pid, 0);

  ptid = ptid_build (pid, pid, 0);
  new_lwp = add_lwp (ptid);
  new_lwp->must_set_ptrace_flags = 1;

  return pid;
}

/* Implement the post_create_inferior target_ops method.  */

static void
linux_post_create_inferior (void)
{
  struct lwp_info *lwp = get_thread_lwp (current_thread);

  linux_arch_setup ();

  if (lwp->must_set_ptrace_flags)
    {
      struct process_info *proc = current_process ();
      int options = linux_low_ptrace_options (proc->attached);

      linux_enable_event_reporting (lwpid_of (current_thread), options);
      lwp->must_set_ptrace_flags = 0;
    }
}

/* Attach to an inferior process.  Returns 0 on success, ERRNO on
   error.  */

int
linux_attach_lwp (ptid_t ptid)
{
  struct lwp_info *new_lwp;
  int lwpid = ptid_get_lwp (ptid);

  if (ptrace (PTRACE_ATTACH, lwpid, (PTRACE_TYPE_ARG3) 0, (PTRACE_TYPE_ARG4) 0)
      != 0)
    return errno;

  new_lwp = add_lwp (ptid);

  /* We need to wait for SIGSTOP before being able to make the next
     ptrace call on this LWP.  */
  new_lwp->must_set_ptrace_flags = 1;

  if (linux_proc_pid_is_stopped (lwpid))
    {
      if (debug_threads)
	debug_printf ("Attached to a stopped process\n");

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
	 enough to use TASK_STOPPED for ptrace stops); but since
	 SIGSTOP is not an RT signal, it can only be queued once.  */
      kill_lwp (lwpid, SIGSTOP);

      /* Finally, resume the stopped process.  This will deliver the
	 SIGSTOP (or a higher priority signal, just like normal
	 PTRACE_ATTACH), which we'll catch later on.  */
      ptrace (PTRACE_CONT, lwpid, (PTRACE_TYPE_ARG3) 0, (PTRACE_TYPE_ARG4) 0);
    }

  /* The next time we wait for this LWP we'll see a SIGSTOP as PTRACE_ATTACH
     brings it to a halt.

     There are several cases to consider here:

     1) gdbserver has already attached to the process and is being notified
	of a new thread that is being created.
	In this case we should ignore that SIGSTOP and resume the
	process.  This is handled below by setting stop_expected = 1,
	and the fact that add_thread sets last_resume_kind ==
	resume_continue.

     2) This is the first thread (the process thread), and we're attaching
	to it via attach_inferior.
	In this case we want the process thread to stop.
	This is handled by having linux_attach set last_resume_kind ==
	resume_stop after we return.

	If the pid we are attaching to is also the tgid, we attach to and
	stop all the existing threads.  Otherwise, we attach to pid and
	ignore any other threads in the same group as this pid.

     3) GDB is connecting to gdbserver and is requesting an enumeration of all
	existing threads.
	In this case we want the thread to stop.
	FIXME: This case is currently not properly handled.
	We should wait for the SIGSTOP but don't.  Things work apparently
	because enough time passes between when we ptrace (ATTACH) and when
	gdb makes the next ptrace call on the thread.

     On the other hand, if we are currently trying to stop all threads, we
     should treat the new thread as if we had sent it a SIGSTOP.  This works
     because we are guaranteed that the add_lwp call above added us to the
     end of the list, and so the new thread has not yet reached
     wait_for_sigstop (but will).  */
  new_lwp->stop_expected = 1;

  return 0;
}

/* Callback for linux_proc_attach_tgid_threads.  Attach to PTID if not
   already attached.  Returns true if a new LWP is found, false
   otherwise.  */

static int
attach_proc_task_lwp_callback (ptid_t ptid)
{
  /* Is this a new thread?  */
  if (find_thread_ptid (ptid) == NULL)
    {
      int lwpid = ptid_get_lwp (ptid);
      int err;

      if (debug_threads)
	debug_printf ("Found new lwp %d\n", lwpid);

      err = linux_attach_lwp (ptid);

      /* Be quiet if we simply raced with the thread exiting.  EPERM
	 is returned if the thread's task still exists, and is marked
	 as exited or zombie, as well as other conditions, so in that
	 case, confirm the status in /proc/PID/status.  */
      if (err == ESRCH
	  || (err == EPERM && linux_proc_pid_is_gone (lwpid)))
	{
	  if (debug_threads)
	    {
	      debug_printf ("Cannot attach to lwp %d: "
			    "thread is gone (%d: %s)\n",
			    lwpid, err, strerror (err));
	    }
	}
      else if (err != 0)
	{
	  warning (_("Cannot attach to lwp %d: %s"),
		   lwpid,
		   linux_ptrace_attach_fail_reason_string (ptid, err));
	}

      return 1;
    }
  return 0;
}

static void async_file_mark (void);

/* Attach to PID.  If PID is the tgid, attach to it and all
   of its threads.  */

static int
linux_attach (unsigned long pid)
{
  struct process_info *proc;
  struct thread_info *initial_thread;
  ptid_t ptid = ptid_build (pid, pid, 0);
  int err;

  /* Attach to PID.  We will check for other threads
     soon.  */
  err = linux_attach_lwp (ptid);
  if (err != 0)
    error ("Cannot attach to process %ld: %s",
	   pid, linux_ptrace_attach_fail_reason_string (ptid, err));

  proc = linux_add_process (pid, 1);

  /* Don't ignore the initial SIGSTOP if we just attached to this
     process.  It will be collected by wait shortly.  */
  initial_thread = find_thread_ptid (ptid_build (pid, pid, 0));
  initial_thread->last_resume_kind = resume_stop;

  /* We must attach to every LWP.  If /proc is mounted, use that to
     find them now.  On the one hand, the inferior may be using raw
     clone instead of using pthreads.  On the other hand, even if it
     is using pthreads, GDB may not be connected yet (thread_db needs
     to do symbol lookups, through qSymbol).  Also, thread_db walks
     structures in the inferior's address space to find the list of
     threads/LWPs, and those structures may well be corrupted.  Note
     that once thread_db is loaded, we'll still use it to list threads
     and associate pthread info with each LWP.  */
  linux_proc_attach_tgid_threads (pid, attach_proc_task_lwp_callback);

  /* GDB will shortly read the xml target description for this
     process, to figure out the process' architecture.  But the target
     description is only filled in when the first process/thread in
     the thread group reports its initial PTRACE_ATTACH SIGSTOP.  Do
     that now, otherwise, if GDB is fast enough, it could read the
     target description _before_ that initial stop.  */
  if (non_stop)
    {
      struct lwp_info *lwp;
      int wstat, lwpid;
      ptid_t pid_ptid = pid_to_ptid (pid);

      lwpid = linux_wait_for_event_filtered (pid_ptid, pid_ptid,
					     &wstat, __WALL);
      gdb_assert (lwpid > 0);

      lwp = find_lwp_pid (pid_to_ptid (lwpid));

      if (!WIFSTOPPED (wstat) || WSTOPSIG (wstat) != SIGSTOP)
	{
	  lwp->status_pending_p = 1;
	  lwp->status_pending = wstat;
	}

      initial_thread->last_resume_kind = resume_continue;

      async_file_mark ();

      gdb_assert (proc->tdesc != NULL);
    }

  return 0;
}

struct counter
{
  int pid;
  int count;
};

static int
second_thread_of_pid_p (struct inferior_list_entry *entry, void *args)
{
  struct counter *counter = (struct counter *) args;

  if (ptid_get_pid (entry->id) == counter->pid)
    {
      if (++counter->count > 1)
	return 1;
    }

  return 0;
}

static int
last_thread_of_process_p (int pid)
{
  struct counter counter = { pid , 0 };

  return (find_inferior (&all_threads,
			 second_thread_of_pid_p, &counter) == NULL);
}

/* Kill LWP.  */

static void
linux_kill_one_lwp (struct lwp_info *lwp)
{
  struct thread_info *thr = get_lwp_thread (lwp);
  int pid = lwpid_of (thr);

  /* PTRACE_KILL is unreliable.  After stepping into a signal handler,
     there is no signal context, and ptrace(PTRACE_KILL) (or
     ptrace(PTRACE_CONT, SIGKILL), pretty much the same) acts like
     ptrace(CONT, pid, 0,0) and just resumes the tracee.  A better
     alternative is to kill with SIGKILL.  We only need one SIGKILL
     per process, not one for each thread.  But since we still support
     support debugging programs using raw clone without CLONE_THREAD,
     we send one for each thread.  For years, we used PTRACE_KILL
     only, so we're being a bit paranoid about some old kernels where
     PTRACE_KILL might work better (dubious if there are any such, but
     that's why it's paranoia), so we try SIGKILL first, PTRACE_KILL
     second, and so we're fine everywhere.  */

  errno = 0;
  kill_lwp (pid, SIGKILL);
  if (debug_threads)
    {
      int save_errno = errno;

      debug_printf ("LKL:  kill_lwp (SIGKILL) %s, 0, 0 (%s)\n",
		    target_pid_to_str (ptid_of (thr)),
		    save_errno ? strerror (save_errno) : "OK");
    }

  errno = 0;
  ptrace (PTRACE_KILL, pid, (PTRACE_TYPE_ARG3) 0, (PTRACE_TYPE_ARG4) 0);
  if (debug_threads)
    {
      int save_errno = errno;

      debug_printf ("LKL:  PTRACE_KILL %s, 0, 0 (%s)\n",
		    target_pid_to_str (ptid_of (thr)),
		    save_errno ? strerror (save_errno) : "OK");
    }
}

/* Kill LWP and wait for it to die.  */

static void
kill_wait_lwp (struct lwp_info *lwp)
{
  struct thread_info *thr = get_lwp_thread (lwp);
  int pid = ptid_get_pid (ptid_of (thr));
  int lwpid = ptid_get_lwp (ptid_of (thr));
  int wstat;
  int res;

  if (debug_threads)
    debug_printf ("kwl: killing lwp %d, for pid: %d\n", lwpid, pid);

  do
    {
      linux_kill_one_lwp (lwp);

      /* Make sure it died.  Notes:

	 - The loop is most likely unnecessary.

	 - We don't use linux_wait_for_event as that could delete lwps
	   while we're iterating over them.  We're not interested in
	   any pending status at this point, only in making sure all
	   wait status on the kernel side are collected until the
	   process is reaped.

	 - We don't use __WALL here as the __WALL emulation relies on
	   SIGCHLD, and killing a stopped process doesn't generate
	   one, nor an exit status.
      */
      res = my_waitpid (lwpid, &wstat, 0);
      if (res == -1 && errno == ECHILD)
	res = my_waitpid (lwpid, &wstat, __WCLONE);
    } while (res > 0 && WIFSTOPPED (wstat));

  /* Even if it was stopped, the child may have already disappeared.
     E.g., if it was killed by SIGKILL.  */
  if (res < 0 && errno != ECHILD)
    perror_with_name ("kill_wait_lwp");
}

/* Callback for `find_inferior'.  Kills an lwp of a given process,
   except the leader.  */

static int
kill_one_lwp_callback (struct inferior_list_entry *entry, void *args)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);
  int pid = * (int *) args;

  if (ptid_get_pid (entry->id) != pid)
    return 0;

  /* We avoid killing the first thread here, because of a Linux kernel (at
     least 2.6.0-test7 through 2.6.8-rc4) bug; if we kill the parent before
     the children get a chance to be reaped, it will remain a zombie
     forever.  */

  if (lwpid_of (thread) == pid)
    {
      if (debug_threads)
	debug_printf ("lkop: is last of process %s\n",
		      target_pid_to_str (entry->id));
      return 0;
    }

  kill_wait_lwp (lwp);
  return 0;
}

static int
linux_kill (int pid)
{
  struct process_info *process;
  struct lwp_info *lwp;

  process = find_process_pid (pid);
  if (process == NULL)
    return -1;

  /* If we're killing a running inferior, make sure it is stopped
     first, as PTRACE_KILL will not work otherwise.  */
  stop_all_lwps (0, NULL);

  find_inferior (&all_threads, kill_one_lwp_callback , &pid);

  /* See the comment in linux_kill_one_lwp.  We did not kill the first
     thread in the list, so do so now.  */
  lwp = find_lwp_pid (pid_to_ptid (pid));

  if (lwp == NULL)
    {
      if (debug_threads)
	debug_printf ("lk_1: cannot find lwp for pid: %d\n",
		      pid);
    }
  else
    kill_wait_lwp (lwp);

  the_target->mourn (process);

  /* Since we presently can only stop all lwps of all processes, we
     need to unstop lwps of other processes.  */
  unstop_all_lwps (0, NULL);
  return 0;
}

/* Get pending signal of THREAD, for detaching purposes.  This is the
   signal the thread last stopped for, which we need to deliver to the
   thread when detaching, otherwise, it'd be suppressed/lost.  */

static int
get_detach_signal (struct thread_info *thread)
{
  enum gdb_signal signo = GDB_SIGNAL_0;
  int status;
  struct lwp_info *lp = get_thread_lwp (thread);

  if (lp->status_pending_p)
    status = lp->status_pending;
  else
    {
      /* If the thread had been suspended by gdbserver, and it stopped
	 cleanly, then it'll have stopped with SIGSTOP.  But we don't
	 want to deliver that SIGSTOP.  */
      if (thread->last_status.kind != TARGET_WAITKIND_STOPPED
	  || thread->last_status.value.sig == GDB_SIGNAL_0)
	return 0;

      /* Otherwise, we may need to deliver the signal we
	 intercepted.  */
      status = lp->last_status;
    }

  if (!WIFSTOPPED (status))
    {
      if (debug_threads)
	debug_printf ("GPS: lwp %s hasn't stopped: no pending signal\n",
		      target_pid_to_str (ptid_of (thread)));
      return 0;
    }

  /* Extended wait statuses aren't real SIGTRAPs.  */
  if (WSTOPSIG (status) == SIGTRAP && linux_is_extended_waitstatus (status))
    {
      if (debug_threads)
	debug_printf ("GPS: lwp %s had stopped with extended "
		      "status: no pending signal\n",
		      target_pid_to_str (ptid_of (thread)));
      return 0;
    }

  signo = gdb_signal_from_host (WSTOPSIG (status));

  if (program_signals_p && !program_signals[signo])
    {
      if (debug_threads)
	debug_printf ("GPS: lwp %s had signal %s, but it is in nopass state\n",
		      target_pid_to_str (ptid_of (thread)),
		      gdb_signal_to_string (signo));
      return 0;
    }
  else if (!program_signals_p
	   /* If we have no way to know which signals GDB does not
	      want to have passed to the program, assume
	      SIGTRAP/SIGINT, which is GDB's default.  */
	   && (signo == GDB_SIGNAL_TRAP || signo == GDB_SIGNAL_INT))
    {
      if (debug_threads)
	debug_printf ("GPS: lwp %s had signal %s, "
		      "but we don't know if we should pass it. "
		      "Default to not.\n",
		      target_pid_to_str (ptid_of (thread)),
		      gdb_signal_to_string (signo));
      return 0;
    }
  else
    {
      if (debug_threads)
	debug_printf ("GPS: lwp %s has pending signal %s: delivering it.\n",
		      target_pid_to_str (ptid_of (thread)),
		      gdb_signal_to_string (signo));

      return WSTOPSIG (status);
    }
}

/* Detach from LWP.  */

static void
linux_detach_one_lwp (struct lwp_info *lwp)
{
  struct thread_info *thread = get_lwp_thread (lwp);
  int sig;
  int lwpid;

  /* If there is a pending SIGSTOP, get rid of it.  */
  if (lwp->stop_expected)
    {
      if (debug_threads)
	debug_printf ("Sending SIGCONT to %s\n",
		      target_pid_to_str (ptid_of (thread)));

      kill_lwp (lwpid_of (thread), SIGCONT);
      lwp->stop_expected = 0;
    }

  /* Pass on any pending signal for this thread.  */
  sig = get_detach_signal (thread);

  /* Preparing to resume may try to write registers, and fail if the
     lwp is zombie.  If that happens, ignore the error.  We'll handle
     it below, when detach fails with ESRCH.  */
  TRY
    {
      /* Flush any pending changes to the process's registers.  */
      regcache_invalidate_thread (thread);

      /* Finally, let it resume.  */
      if (the_low_target.prepare_to_resume != NULL)
	the_low_target.prepare_to_resume (lwp);
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      if (!check_ptrace_stopped_lwp_gone (lwp))
	throw_exception (ex);
    }
  END_CATCH

  lwpid = lwpid_of (thread);
  if (ptrace (PTRACE_DETACH, lwpid, (PTRACE_TYPE_ARG3) 0,
	      (PTRACE_TYPE_ARG4) (long) sig) < 0)
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
		       lwpid, strerror (errno));
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
		 target_pid_to_str (ptid_of (thread)),
		 strerror (save_errno));
	}
    }
  else if (debug_threads)
    {
      debug_printf ("PTRACE_DETACH (%s, %s, 0) (OK)\n",
		    target_pid_to_str (ptid_of (thread)),
		    strsignal (sig));
    }

  delete_lwp (lwp);
}

/* Callback for find_inferior.  Detaches from non-leader threads of a
   given process.  */

static int
linux_detach_lwp_callback (struct inferior_list_entry *entry, void *args)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);
  int pid = *(int *) args;
  int lwpid = lwpid_of (thread);

  /* Skip other processes.  */
  if (ptid_get_pid (entry->id) != pid)
    return 0;

  /* We don't actually detach from the thread group leader just yet.
     If the thread group exits, we must reap the zombie clone lwps
     before we're able to reap the leader.  */
  if (ptid_get_pid (entry->id) == lwpid)
    return 0;

  linux_detach_one_lwp (lwp);
  return 0;
}

static int
linux_detach (int pid)
{
  struct process_info *process;
  struct lwp_info *main_lwp;

  process = find_process_pid (pid);
  if (process == NULL)
    return -1;

  /* As there's a step over already in progress, let it finish first,
     otherwise nesting a stabilize_threads operation on top gets real
     messy.  */
  complete_ongoing_step_over ();

  /* Stop all threads before detaching.  First, ptrace requires that
     the thread is stopped to sucessfully detach.  Second, thread_db
     may need to uninstall thread event breakpoints from memory, which
     only works with a stopped process anyway.  */
  stop_all_lwps (0, NULL);

#ifdef USE_THREAD_DB
  thread_db_detach (process);
#endif

  /* Stabilize threads (move out of jump pads).  */
  stabilize_threads ();

  /* Detach from the clone lwps first.  If the thread group exits just
     while we're detaching, we must reap the clone lwps before we're
     able to reap the leader.  */
  find_inferior (&all_threads, linux_detach_lwp_callback, &pid);

  main_lwp = find_lwp_pid (pid_to_ptid (pid));
  linux_detach_one_lwp (main_lwp);

  the_target->mourn (process);

  /* Since we presently can only stop all lwps of all processes, we
     need to unstop lwps of other processes.  */
  unstop_all_lwps (0, NULL);
  return 0;
}

/* Remove all LWPs that belong to process PROC from the lwp list.  */

static int
delete_lwp_callback (struct inferior_list_entry *entry, void *proc)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);
  struct process_info *process = (struct process_info *) proc;

  if (pid_of (thread) == pid_of (process))
    delete_lwp (lwp);

  return 0;
}

static void
linux_mourn (struct process_info *process)
{
  struct process_info_private *priv;

#ifdef USE_THREAD_DB
  thread_db_mourn (process);
#endif

  find_inferior (&all_threads, delete_lwp_callback, process);

  /* Freeing all private data.  */
  priv = process->priv;
  free (priv->arch_private);
  free (priv);
  process->priv = NULL;

  remove_process (process);
}

static void
linux_join (int pid)
{
  int status, ret;

  do {
    ret = my_waitpid (pid, &status, 0);
    if (WIFEXITED (status) || WIFSIGNALED (status))
      break;
  } while (ret != -1 || errno != ECHILD);
}

/* Return nonzero if the given thread is still alive.  */
static int
linux_thread_alive (ptid_t ptid)
{
  struct lwp_info *lwp = find_lwp_pid (ptid);

  /* We assume we always know if a thread exits.  If a whole process
     exited but we still haven't been able to report it to GDB, we'll
     hold on to the last lwp of the dead process.  */
  if (lwp != NULL)
    return !lwp_is_marked_dead (lwp);
  else
    return 0;
}

/* Return 1 if this lwp still has an interesting status pending.  If
   not (e.g., it had stopped for a breakpoint that is gone), return
   false.  */

static int
thread_still_has_status_pending_p (struct thread_info *thread)
{
  struct lwp_info *lp = get_thread_lwp (thread);

  if (!lp->status_pending_p)
    return 0;

  if (thread->last_resume_kind != resume_stop
      && (lp->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT
	  || lp->stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT))
    {
      struct thread_info *saved_thread;
      CORE_ADDR pc;
      int discard = 0;

      gdb_assert (lp->last_status != 0);

      pc = get_pc (lp);

      saved_thread = current_thread;
      current_thread = thread;

      if (pc != lp->stop_pc)
	{
	  if (debug_threads)
	    debug_printf ("PC of %ld changed\n",
			  lwpid_of (thread));
	  discard = 1;
	}

#if !USE_SIGTRAP_SIGINFO
      else if (lp->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT
	       && !(*the_low_target.breakpoint_at) (pc))
	{
	  if (debug_threads)
	    debug_printf ("previous SW breakpoint of %ld gone\n",
			  lwpid_of (thread));
	  discard = 1;
	}
      else if (lp->stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT
	       && !hardware_breakpoint_inserted_here (pc))
	{
	  if (debug_threads)
	    debug_printf ("previous HW breakpoint of %ld gone\n",
			  lwpid_of (thread));
	  discard = 1;
	}
#endif

      current_thread = saved_thread;

      if (discard)
	{
	  if (debug_threads)
	    debug_printf ("discarding pending breakpoint status\n");
	  lp->status_pending_p = 0;
	  return 0;
	}
    }

  return 1;
}

/* Returns true if LWP is resumed from the client's perspective.  */

static int
lwp_resumed (struct lwp_info *lwp)
{
  struct thread_info *thread = get_lwp_thread (lwp);

  if (thread->last_resume_kind != resume_stop)
    return 1;

  /* Did gdb send us a `vCont;t', but we haven't reported the
     corresponding stop to gdb yet?  If so, the thread is still
     resumed/running from gdb's perspective.  */
  if (thread->last_resume_kind == resume_stop
      && thread->last_status.kind == TARGET_WAITKIND_IGNORE)
    return 1;

  return 0;
}

/* Return 1 if this lwp has an interesting status pending.  */
static int
status_pending_p_callback (struct inferior_list_entry *entry, void *arg)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lp = get_thread_lwp (thread);
  ptid_t ptid = * (ptid_t *) arg;

  /* Check if we're only interested in events from a specific process
     or a specific LWP.  */
  if (!ptid_match (ptid_of (thread), ptid))
    return 0;

  if (!lwp_resumed (lp))
    return 0;

  if (lp->status_pending_p
      && !thread_still_has_status_pending_p (thread))
    {
      linux_resume_one_lwp (lp, lp->stepping, GDB_SIGNAL_0, NULL);
      return 0;
    }

  return lp->status_pending_p;
}

static int
same_lwp (struct inferior_list_entry *entry, void *data)
{
  ptid_t ptid = *(ptid_t *) data;
  int lwp;

  if (ptid_get_lwp (ptid) != 0)
    lwp = ptid_get_lwp (ptid);
  else
    lwp = ptid_get_pid (ptid);

  if (ptid_get_lwp (entry->id) == lwp)
    return 1;

  return 0;
}

struct lwp_info *
find_lwp_pid (ptid_t ptid)
{
  struct inferior_list_entry *thread
    = find_inferior (&all_threads, same_lwp, &ptid);

  if (thread == NULL)
    return NULL;

  return get_thread_lwp ((struct thread_info *) thread);
}

/* Return the number of known LWPs in the tgid given by PID.  */

static int
num_lwps (int pid)
{
  struct inferior_list_entry *inf, *tmp;
  int count = 0;

  ALL_INFERIORS (&all_threads, inf, tmp)
    {
      if (ptid_get_pid (inf->id) == pid)
	count++;
    }

  return count;
}

/* The arguments passed to iterate_over_lwps.  */

struct iterate_over_lwps_args
{
  /* The FILTER argument passed to iterate_over_lwps.  */
  ptid_t filter;

  /* The CALLBACK argument passed to iterate_over_lwps.  */
  iterate_over_lwps_ftype *callback;

  /* The DATA argument passed to iterate_over_lwps.  */
  void *data;
};

/* Callback for find_inferior used by iterate_over_lwps to filter
   calls to the callback supplied to that function.  Returning a
   nonzero value causes find_inferiors to stop iterating and return
   the current inferior_list_entry.  Returning zero indicates that
   find_inferiors should continue iterating.  */

static int
iterate_over_lwps_filter (struct inferior_list_entry *entry, void *args_p)
{
  struct iterate_over_lwps_args *args
    = (struct iterate_over_lwps_args *) args_p;

  if (ptid_match (entry->id, args->filter))
    {
      struct thread_info *thr = (struct thread_info *) entry;
      struct lwp_info *lwp = get_thread_lwp (thr);

      return (*args->callback) (lwp, args->data);
    }

  return 0;
}

/* See nat/linux-nat.h.  */

struct lwp_info *
iterate_over_lwps (ptid_t filter,
		   iterate_over_lwps_ftype callback,
		   void *data)
{
  struct iterate_over_lwps_args args = {filter, callback, data};
  struct inferior_list_entry *entry;

  entry = find_inferior (&all_threads, iterate_over_lwps_filter, &args);
  if (entry == NULL)
    return NULL;

  return get_thread_lwp ((struct thread_info *) entry);
}

/* Detect zombie thread group leaders, and "exit" them.  We can't reap
   their exits until all other threads in the group have exited.  */

static void
check_zombie_leaders (void)
{
  struct process_info *proc, *tmp;

  ALL_PROCESSES (proc, tmp)
    {
      pid_t leader_pid = pid_of (proc);
      struct lwp_info *leader_lp;

      leader_lp = find_lwp_pid (pid_to_ptid (leader_pid));

      if (debug_threads)
	debug_printf ("leader_pid=%d, leader_lp!=NULL=%d, "
		      "num_lwps=%d, zombie=%d\n",
		      leader_pid, leader_lp!= NULL, num_lwps (leader_pid),
		      linux_proc_pid_is_zombie (leader_pid));

      if (leader_lp != NULL && !leader_lp->stopped
	  /* Check if there are other threads in the group, as we may
	     have raced with the inferior simply exiting.  */
	  && !last_thread_of_process_p (leader_pid)
	  && linux_proc_pid_is_zombie (leader_pid))
	{
	  /* A leader zombie can mean one of two things:

	     - It exited, and there's an exit status pending
	     available, or only the leader exited (not the whole
	     program).  In the latter case, we can't waitpid the
	     leader's exit status until all other threads are gone.

	     - There are 3 or more threads in the group, and a thread
	     other than the leader exec'd.  On an exec, the Linux
	     kernel destroys all other threads (except the execing
	     one) in the thread group, and resets the execing thread's
	     tid to the tgid.  No exit notification is sent for the
	     execing thread -- from the ptracer's perspective, it
	     appears as though the execing thread just vanishes.
	     Until we reap all other threads except the leader and the
	     execing thread, the leader will be zombie, and the
	     execing thread will be in `D (disc sleep)'.  As soon as
	     all other threads are reaped, the execing thread changes
	     it's tid to the tgid, and the previous (zombie) leader
	     vanishes, giving place to the "new" leader.  We could try
	     distinguishing the exit and exec cases, by waiting once
	     more, and seeing if something comes out, but it doesn't
	     sound useful.  The previous leader _does_ go away, and
	     we'll re-add the new one once we see the exec event
	     (which is just the same as what would happen if the
	     previous leader did exit voluntarily before some other
	     thread execs).  */

	  if (debug_threads)
	    fprintf (stderr,
		     "CZL: Thread group leader %d zombie "
		     "(it exited, or another thread execd).\n",
		     leader_pid);

	  delete_lwp (leader_lp);
	}
    }
}

/* Callback for `find_inferior'.  Returns the first LWP that is not
   stopped.  ARG is a PTID filter.  */

static int
not_stopped_callback (struct inferior_list_entry *entry, void *arg)
{
  struct thread_info *thr = (struct thread_info *) entry;
  struct lwp_info *lwp;
  ptid_t filter = *(ptid_t *) arg;

  if (!ptid_match (ptid_of (thr), filter))
    return 0;

  lwp = get_thread_lwp (thr);
  if (!lwp->stopped)
    return 1;

  return 0;
}

/* Increment LWP's suspend count.  */

static void
lwp_suspended_inc (struct lwp_info *lwp)
{
  lwp->suspended++;

  if (debug_threads && lwp->suspended > 4)
    {
      struct thread_info *thread = get_lwp_thread (lwp);

      debug_printf ("LWP %ld has a suspiciously high suspend count,"
		    " suspended=%d\n", lwpid_of (thread), lwp->suspended);
    }
}

/* Decrement LWP's suspend count.  */

static void
lwp_suspended_decr (struct lwp_info *lwp)
{
  lwp->suspended--;

  if (lwp->suspended < 0)
    {
      struct thread_info *thread = get_lwp_thread (lwp);

      internal_error (__FILE__, __LINE__,
		      "unsuspend LWP %ld, suspended=%d\n", lwpid_of (thread),
		      lwp->suspended);
    }
}

/* This function should only be called if the LWP got a SIGTRAP.

   Handle any tracepoint steps or hits.  Return true if a tracepoint
   event was handled, 0 otherwise.  */

static int
handle_tracepoints (struct lwp_info *lwp)
{
  struct thread_info *tinfo = get_lwp_thread (lwp);
  int tpoint_related_event = 0;

  gdb_assert (lwp->suspended == 0);

  /* If this tracepoint hit causes a tracing stop, we'll immediately
     uninsert tracepoints.  To do this, we temporarily pause all
     threads, unpatch away, and then unpause threads.  We need to make
     sure the unpausing doesn't resume LWP too.  */
  lwp_suspended_inc (lwp);

  /* And we need to be sure that any all-threads-stopping doesn't try
     to move threads out of the jump pads, as it could deadlock the
     inferior (LWP could be in the jump pad, maybe even holding the
     lock.)  */

  /* Do any necessary step collect actions.  */
  tpoint_related_event |= tracepoint_finished_step (tinfo, lwp->stop_pc);

  tpoint_related_event |= handle_tracepoint_bkpts (tinfo, lwp->stop_pc);

  /* See if we just hit a tracepoint and do its main collect
     actions.  */
  tpoint_related_event |= tracepoint_was_hit (tinfo, lwp->stop_pc);

  lwp_suspended_decr (lwp);

  gdb_assert (lwp->suspended == 0);
  gdb_assert (!stabilizing_threads || lwp->collecting_fast_tracepoint);

  if (tpoint_related_event)
    {
      if (debug_threads)
	debug_printf ("got a tracepoint event\n");
      return 1;
    }

  return 0;
}

/* Convenience wrapper.  Returns true if LWP is presently collecting a
   fast tracepoint.  */

static int
linux_fast_tracepoint_collecting (struct lwp_info *lwp,
				  struct fast_tpoint_collect_status *status)
{
  CORE_ADDR thread_area;
  struct thread_info *thread = get_lwp_thread (lwp);

  if (the_low_target.get_thread_area == NULL)
    return 0;

  /* Get the thread area address.  This is used to recognize which
     thread is which when tracing with the in-process agent library.
     We don't read anything from the address, and treat it as opaque;
     it's the address itself that we assume is unique per-thread.  */
  if ((*the_low_target.get_thread_area) (lwpid_of (thread), &thread_area) == -1)
    return 0;

  return fast_tracepoint_collecting (thread_area, lwp->stop_pc, status);
}

/* The reason we resume in the caller, is because we want to be able
   to pass lwp->status_pending as WSTAT, and we need to clear
   status_pending_p before resuming, otherwise, linux_resume_one_lwp
   refuses to resume.  */

static int
maybe_move_out_of_jump_pad (struct lwp_info *lwp, int *wstat)
{
  struct thread_info *saved_thread;

  saved_thread = current_thread;
  current_thread = get_lwp_thread (lwp);

  if ((wstat == NULL
       || (WIFSTOPPED (*wstat) && WSTOPSIG (*wstat) != SIGTRAP))
      && supports_fast_tracepoints ()
      && agent_loaded_p ())
    {
      struct fast_tpoint_collect_status status;
      int r;

      if (debug_threads)
	debug_printf ("Checking whether LWP %ld needs to move out of the "
		      "jump pad.\n",
		      lwpid_of (current_thread));

      r = linux_fast_tracepoint_collecting (lwp, &status);

      if (wstat == NULL
	  || (WSTOPSIG (*wstat) != SIGILL
	      && WSTOPSIG (*wstat) != SIGFPE
	      && WSTOPSIG (*wstat) != SIGSEGV
	      && WSTOPSIG (*wstat) != SIGBUS))
	{
	  lwp->collecting_fast_tracepoint = r;

	  if (r != 0)
	    {
	      if (r == 1 && lwp->exit_jump_pad_bkpt == NULL)
		{
		  /* Haven't executed the original instruction yet.
		     Set breakpoint there, and wait till it's hit,
		     then single-step until exiting the jump pad.  */
		  lwp->exit_jump_pad_bkpt
		    = set_breakpoint_at (status.adjusted_insn_addr, NULL);
		}

	      if (debug_threads)
		debug_printf ("Checking whether LWP %ld needs to move out of "
			      "the jump pad...it does\n",
			      lwpid_of (current_thread));
	      current_thread = saved_thread;

	      return 1;
	    }
	}
      else
	{
	  /* If we get a synchronous signal while collecting, *and*
	     while executing the (relocated) original instruction,
	     reset the PC to point at the tpoint address, before
	     reporting to GDB.  Otherwise, it's an IPA lib bug: just
	     report the signal to GDB, and pray for the best.  */

	  lwp->collecting_fast_tracepoint = 0;

	  if (r != 0
	      && (status.adjusted_insn_addr <= lwp->stop_pc
		  && lwp->stop_pc < status.adjusted_insn_addr_end))
	    {
	      siginfo_t info;
	      struct regcache *regcache;

	      /* The si_addr on a few signals references the address
		 of the faulting instruction.  Adjust that as
		 well.  */
	      if ((WSTOPSIG (*wstat) == SIGILL
		   || WSTOPSIG (*wstat) == SIGFPE
		   || WSTOPSIG (*wstat) == SIGBUS
		   || WSTOPSIG (*wstat) == SIGSEGV)
		  && ptrace (PTRACE_GETSIGINFO, lwpid_of (current_thread),
			     (PTRACE_TYPE_ARG3) 0, &info) == 0
		  /* Final check just to make sure we don't clobber
		     the siginfo of non-kernel-sent signals.  */
		  && (uintptr_t) info.si_addr == lwp->stop_pc)
		{
		  info.si_addr = (void *) (uintptr_t) status.tpoint_addr;
		  ptrace (PTRACE_SETSIGINFO, lwpid_of (current_thread),
			  (PTRACE_TYPE_ARG3) 0, &info);
		}

	      regcache = get_thread_regcache (current_thread, 1);
	      (*the_low_target.set_pc) (regcache, status.tpoint_addr);
	      lwp->stop_pc = status.tpoint_addr;

	      /* Cancel any fast tracepoint lock this thread was
		 holding.  */
	      force_unlock_trace_buffer ();
	    }

	  if (lwp->exit_jump_pad_bkpt != NULL)
	    {
	      if (debug_threads)
		debug_printf ("Cancelling fast exit-jump-pad: removing bkpt. "
			      "stopping all threads momentarily.\n");

	      stop_all_lwps (1, lwp);

	      delete_breakpoint (lwp->exit_jump_pad_bkpt);
	      lwp->exit_jump_pad_bkpt = NULL;

	      unstop_all_lwps (1, lwp);

	      gdb_assert (lwp->suspended >= 0);
	    }
	}
    }

  if (debug_threads)
    debug_printf ("Checking whether LWP %ld needs to move out of the "
		  "jump pad...no\n",
		  lwpid_of (current_thread));

  current_thread = saved_thread;
  return 0;
}

/* Enqueue one signal in the "signals to report later when out of the
   jump pad" list.  */

static void
enqueue_one_deferred_signal (struct lwp_info *lwp, int *wstat)
{
  struct pending_signals *p_sig;
  struct thread_info *thread = get_lwp_thread (lwp);

  if (debug_threads)
    debug_printf ("Deferring signal %d for LWP %ld.\n",
		  WSTOPSIG (*wstat), lwpid_of (thread));

  if (debug_threads)
    {
      struct pending_signals *sig;

      for (sig = lwp->pending_signals_to_report;
	   sig != NULL;
	   sig = sig->prev)
	debug_printf ("   Already queued %d\n",
		      sig->signal);

      debug_printf ("   (no more currently queued signals)\n");
    }

  /* Don't enqueue non-RT signals if they are already in the deferred
     queue.  (SIGSTOP being the easiest signal to see ending up here
     twice)  */
  if (WSTOPSIG (*wstat) < __SIGRTMIN)
    {
      struct pending_signals *sig;

      for (sig = lwp->pending_signals_to_report;
	   sig != NULL;
	   sig = sig->prev)
	{
	  if (sig->signal == WSTOPSIG (*wstat))
	    {
	      if (debug_threads)
		debug_printf ("Not requeuing already queued non-RT signal %d"
			      " for LWP %ld\n",
			      sig->signal,
			      lwpid_of (thread));
	      return;
	    }
	}
    }

  p_sig = XCNEW (struct pending_signals);
  p_sig->prev = lwp->pending_signals_to_report;
  p_sig->signal = WSTOPSIG (*wstat);

  ptrace (PTRACE_GETSIGINFO, lwpid_of (thread), (PTRACE_TYPE_ARG3) 0,
	  &p_sig->info);

  lwp->pending_signals_to_report = p_sig;
}

/* Dequeue one signal from the "signals to report later when out of
   the jump pad" list.  */

static int
dequeue_one_deferred_signal (struct lwp_info *lwp, int *wstat)
{
  struct thread_info *thread = get_lwp_thread (lwp);

  if (lwp->pending_signals_to_report != NULL)
    {
      struct pending_signals **p_sig;

      p_sig = &lwp->pending_signals_to_report;
      while ((*p_sig)->prev != NULL)
	p_sig = &(*p_sig)->prev;

      *wstat = W_STOPCODE ((*p_sig)->signal);
      if ((*p_sig)->info.si_signo != 0)
	ptrace (PTRACE_SETSIGINFO, lwpid_of (thread), (PTRACE_TYPE_ARG3) 0,
		&(*p_sig)->info);
      free (*p_sig);
      *p_sig = NULL;

      if (debug_threads)
	debug_printf ("Reporting deferred signal %d for LWP %ld.\n",
		      WSTOPSIG (*wstat), lwpid_of (thread));

      if (debug_threads)
	{
	  struct pending_signals *sig;

	  for (sig = lwp->pending_signals_to_report;
	       sig != NULL;
	       sig = sig->prev)
	    debug_printf ("   Still queued %d\n",
			  sig->signal);

	  debug_printf ("   (no more queued signals)\n");
	}

      return 1;
    }

  return 0;
}

/* Fetch the possibly triggered data watchpoint info and store it in
   CHILD.

   On some archs, like x86, that use debug registers to set
   watchpoints, it's possible that the way to know which watched
   address trapped, is to check the register that is used to select
   which address to watch.  Problem is, between setting the watchpoint
   and reading back which data address trapped, the user may change
   the set of watchpoints, and, as a consequence, GDB changes the
   debug registers in the inferior.  To avoid reading back a stale
   stopped-data-address when that happens, we cache in LP the fact
   that a watchpoint trapped, and the corresponding data address, as
   soon as we see CHILD stop with a SIGTRAP.  If GDB changes the debug
   registers meanwhile, we have the cached data we can rely on.  */

static int
check_stopped_by_watchpoint (struct lwp_info *child)
{
  if (the_low_target.stopped_by_watchpoint != NULL)
    {
      struct thread_info *saved_thread;

      saved_thread = current_thread;
      current_thread = get_lwp_thread (child);

      if (the_low_target.stopped_by_watchpoint ())
	{
	  child->stop_reason = TARGET_STOPPED_BY_WATCHPOINT;

	  if (the_low_target.stopped_data_address != NULL)
	    child->stopped_data_address
	      = the_low_target.stopped_data_address ();
	  else
	    child->stopped_data_address = 0;
	}

      current_thread = saved_thread;
    }

  return child->stop_reason == TARGET_STOPPED_BY_WATCHPOINT;
}

/* Return the ptrace options that we want to try to enable.  */

static int
linux_low_ptrace_options (int attached)
{
  int options = 0;

  if (!attached)
    options |= PTRACE_O_EXITKILL;

  if (report_fork_events)
    options |= PTRACE_O_TRACEFORK;

  if (report_vfork_events)
    options |= (PTRACE_O_TRACEVFORK | PTRACE_O_TRACEVFORKDONE);

  if (report_exec_events)
    options |= PTRACE_O_TRACEEXEC;

  options |= PTRACE_O_TRACESYSGOOD;

  return options;
}

/* Do low-level handling of the event, and check if we should go on
   and pass it to caller code.  Return the affected lwp if we are, or
   NULL otherwise.  */

static struct lwp_info *
linux_low_filter_event (int lwpid, int wstat)
{
  struct lwp_info *child;
  struct thread_info *thread;
  int have_stop_pc = 0;

  child = find_lwp_pid (pid_to_ptid (lwpid));

  /* Check for stop events reported by a process we didn't already
     know about - anything not already in our LWP list.

     If we're expecting to receive stopped processes after
     fork, vfork, and clone events, then we'll just add the
     new one to our list and go back to waiting for the event
     to be reported - the stopped process might be returned
     from waitpid before or after the event is.

     But note the case of a non-leader thread exec'ing after the
     leader having exited, and gone from our lists (because
     check_zombie_leaders deleted it).  The non-leader thread
     changes its tid to the tgid.  */

  if (WIFSTOPPED (wstat) && child == NULL && WSTOPSIG (wstat) == SIGTRAP
      && linux_ptrace_get_extended_event (wstat) == PTRACE_EVENT_EXEC)
    {
      ptid_t child_ptid;

      /* A multi-thread exec after we had seen the leader exiting.  */
      if (debug_threads)
	{
	  debug_printf ("LLW: Re-adding thread group leader LWP %d"
			"after exec.\n", lwpid);
	}

      child_ptid = ptid_build (lwpid, lwpid, 0);
      child = add_lwp (child_ptid);
      child->stopped = 1;
      current_thread = child->thread;
    }

  /* If we didn't find a process, one of two things presumably happened:
     - A process we started and then detached from has exited.  Ignore it.
     - A process we are controlling has forked and the new child's stop
     was reported to us by the kernel.  Save its PID.  */
  if (child == NULL && WIFSTOPPED (wstat))
    {
      add_to_pid_list (&stopped_pids, lwpid, wstat);
      return NULL;
    }
  else if (child == NULL)
    return NULL;

  thread = get_lwp_thread (child);

  child->stopped = 1;

  child->last_status = wstat;

  /* Check if the thread has exited.  */
  if ((WIFEXITED (wstat) || WIFSIGNALED (wstat)))
    {
      if (debug_threads)
	debug_printf ("LLFE: %d exited.\n", lwpid);

      if (finish_step_over (child))
	{
	  /* Unsuspend all other LWPs, and set them back running again.  */
	  unsuspend_all_lwps (child);
	}

      /* If there is at least one more LWP, then the exit signal was
	 not the end of the debugged application and should be
	 ignored, unless GDB wants to hear about thread exits.  */
      if (report_thread_events
	  || last_thread_of_process_p (pid_of (thread)))
	{
	  /* Since events are serialized to GDB core, and we can't
	     report this one right now.  Leave the status pending for
	     the next time we're able to report it.  */
	  mark_lwp_dead (child, wstat);
	  return child;
	}
      else
	{
	  delete_lwp (child);
	  return NULL;
	}
    }

  gdb_assert (WIFSTOPPED (wstat));

  if (WIFSTOPPED (wstat))
    {
      struct process_info *proc;

      /* Architecture-specific setup after inferior is running.  */
      proc = find_process_pid (pid_of (thread));
      if (proc->tdesc == NULL)
	{
	  if (proc->attached)
	    {
	      /* This needs to happen after we have attached to the
		 inferior and it is stopped for the first time, but
		 before we access any inferior registers.  */
	      linux_arch_setup_thread (thread);
	    }
	  else
	    {
	      /* The process is started, but GDBserver will do
		 architecture-specific setup after the program stops at
		 the first instruction.  */
	      child->status_pending_p = 1;
	      child->status_pending = wstat;
	      return child;
	    }
	}
    }

  if (WIFSTOPPED (wstat) && child->must_set_ptrace_flags)
    {
      struct process_info *proc = find_process_pid (pid_of (thread));
      int options = linux_low_ptrace_options (proc->attached);

      linux_enable_event_reporting (lwpid, options);
      child->must_set_ptrace_flags = 0;
    }

  /* Always update syscall_state, even if it will be filtered later.  */
  if (WIFSTOPPED (wstat) && WSTOPSIG (wstat) == SYSCALL_SIGTRAP)
    {
      child->syscall_state
	= (child->syscall_state == TARGET_WAITKIND_SYSCALL_ENTRY
	   ? TARGET_WAITKIND_SYSCALL_RETURN
	   : TARGET_WAITKIND_SYSCALL_ENTRY);
    }
  else
    {
      /* Almost all other ptrace-stops are known to be outside of system
	 calls, with further exceptions in handle_extended_wait.  */
      child->syscall_state = TARGET_WAITKIND_IGNORE;
    }

  /* Be careful to not overwrite stop_pc until save_stop_reason is
     called.  */
  if (WIFSTOPPED (wstat) && WSTOPSIG (wstat) == SIGTRAP
      && linux_is_extended_waitstatus (wstat))
    {
      child->stop_pc = get_pc (child);
      if (handle_extended_wait (&child, wstat))
	{
	  /* The event has been handled, so just return without
	     reporting it.  */
	  return NULL;
	}
    }

  if (linux_wstatus_maybe_breakpoint (wstat))
    {
      if (save_stop_reason (child))
	have_stop_pc = 1;
    }

  if (!have_stop_pc)
    child->stop_pc = get_pc (child);

  if (WIFSTOPPED (wstat) && WSTOPSIG (wstat) == SIGSTOP
      && child->stop_expected)
    {
      if (debug_threads)
	debug_printf ("Expected stop.\n");
      child->stop_expected = 0;

      if (thread->last_resume_kind == resume_stop)
	{
	  /* We want to report the stop to the core.  Treat the
	     SIGSTOP as a normal event.  */
	  if (debug_threads)
	    debug_printf ("LLW: resume_stop SIGSTOP caught for %s.\n",
			  target_pid_to_str (ptid_of (thread)));
	}
      else if (stopping_threads != NOT_STOPPING_THREADS)
	{
	  /* Stopping threads.  We don't want this SIGSTOP to end up
	     pending.  */
	  if (debug_threads)
	    debug_printf ("LLW: SIGSTOP caught for %s "
			  "while stopping threads.\n",
			  target_pid_to_str (ptid_of (thread)));
	  return NULL;
	}
      else
	{
	  /* This is a delayed SIGSTOP.  Filter out the event.  */
	  if (debug_threads)
	    debug_printf ("LLW: %s %s, 0, 0 (discard delayed SIGSTOP)\n",
			  child->stepping ? "step" : "continue",
			  target_pid_to_str (ptid_of (thread)));

	  linux_resume_one_lwp (child, child->stepping, 0, NULL);
	  return NULL;
	}
    }

  child->status_pending_p = 1;
  child->status_pending = wstat;
  return child;
}

/* Return true if THREAD is doing hardware single step.  */

static int
maybe_hw_step (struct thread_info *thread)
{
  if (can_hardware_single_step ())
    return 1;
  else
    {
      /* GDBserver must insert reinsert breakpoint for software
	 single step.  */
      gdb_assert (has_reinsert_breakpoints (thread));
      return 0;
    }
}

/* Resume LWPs that are currently stopped without any pending status
   to report, but are resumed from the core's perspective.  */

static void
resume_stopped_resumed_lwps (struct inferior_list_entry *entry)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lp = get_thread_lwp (thread);

  if (lp->stopped
      && !lp->suspended
      && !lp->status_pending_p
      && thread->last_status.kind == TARGET_WAITKIND_IGNORE)
    {
      int step = 0;

      if (thread->last_resume_kind == resume_step)
	step = maybe_hw_step (thread);

      if (debug_threads)
	debug_printf ("RSRL: resuming stopped-resumed LWP %s at %s: step=%d\n",
		      target_pid_to_str (ptid_of (thread)),
		      paddress (lp->stop_pc),
		      step);

      linux_resume_one_lwp (lp, step, GDB_SIGNAL_0, NULL);
    }
}

/* Wait for an event from child(ren) WAIT_PTID, and return any that
   match FILTER_PTID (leaving others pending).  The PTIDs can be:
   minus_one_ptid, to specify any child; a pid PTID, specifying all
   lwps of a thread group; or a PTID representing a single lwp.  Store
   the stop status through the status pointer WSTAT.  OPTIONS is
   passed to the waitpid call.  Return 0 if no event was found and
   OPTIONS contains WNOHANG.  Return -1 if no unwaited-for children
   was found.  Return the PID of the stopped child otherwise.  */

static int
linux_wait_for_event_filtered (ptid_t wait_ptid, ptid_t filter_ptid,
			       int *wstatp, int options)
{
  struct thread_info *event_thread;
  struct lwp_info *event_child, *requested_child;
  sigset_t block_mask, prev_mask;

 retry:
  /* N.B. event_thread points to the thread_info struct that contains
     event_child.  Keep them in sync.  */
  event_thread = NULL;
  event_child = NULL;
  requested_child = NULL;

  /* Check for a lwp with a pending status.  */

  if (ptid_equal (filter_ptid, minus_one_ptid) || ptid_is_pid (filter_ptid))
    {
      event_thread = (struct thread_info *)
	find_inferior (&all_threads, status_pending_p_callback, &filter_ptid);
      if (event_thread != NULL)
	event_child = get_thread_lwp (event_thread);
      if (debug_threads && event_thread)
	debug_printf ("Got a pending child %ld\n", lwpid_of (event_thread));
    }
  else if (!ptid_equal (filter_ptid, null_ptid))
    {
      requested_child = find_lwp_pid (filter_ptid);

      if (stopping_threads == NOT_STOPPING_THREADS
	  && requested_child->status_pending_p
	  && requested_child->collecting_fast_tracepoint)
	{
	  enqueue_one_deferred_signal (requested_child,
				       &requested_child->status_pending);
	  requested_child->status_pending_p = 0;
	  requested_child->status_pending = 0;
	  linux_resume_one_lwp (requested_child, 0, 0, NULL);
	}

      if (requested_child->suspended
	  && requested_child->status_pending_p)
	{
	  internal_error (__FILE__, __LINE__,
			  "requesting an event out of a"
			  " suspended child?");
	}

      if (requested_child->status_pending_p)
	{
	  event_child = requested_child;
	  event_thread = get_lwp_thread (event_child);
	}
    }

  if (event_child != NULL)
    {
      if (debug_threads)
	debug_printf ("Got an event from pending child %ld (%04x)\n",
		      lwpid_of (event_thread), event_child->status_pending);
      *wstatp = event_child->status_pending;
      event_child->status_pending_p = 0;
      event_child->status_pending = 0;
      current_thread = event_thread;
      return lwpid_of (event_thread);
    }

  /* But if we don't find a pending event, we'll have to wait.

     We only enter this loop if no process has a pending wait status.
     Thus any action taken in response to a wait status inside this
     loop is responding as soon as we detect the status, not after any
     pending events.  */

  /* Make sure SIGCHLD is blocked until the sigsuspend below.  Block
     all signals while here.  */
  sigfillset (&block_mask);
  sigprocmask (SIG_BLOCK, &block_mask, &prev_mask);

  /* Always pull all events out of the kernel.  We'll randomly select
     an event LWP out of all that have events, to prevent
     starvation.  */
  while (event_child == NULL)
    {
      pid_t ret = 0;

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
      ret = my_waitpid (-1, wstatp, options | WNOHANG);

      if (debug_threads)
	debug_printf ("LWFE: waitpid(-1, ...) returned %d, %s\n",
		      ret, errno ? strerror (errno) : "ERRNO-OK");

      if (ret > 0)
	{
	  if (debug_threads)
	    {
	      debug_printf ("LLW: waitpid %ld received %s\n",
			    (long) ret, status_to_str (*wstatp));
	    }

	  /* Filter all events.  IOW, leave all events pending.  We'll
	     randomly select an event LWP out of all that have events
	     below.  */
	  linux_low_filter_event (ret, *wstatp);
	  /* Retry until nothing comes out of waitpid.  A single
	     SIGCHLD can indicate more than one child stopped.  */
	  continue;
	}

      /* Now that we've pulled all events out of the kernel, resume
	 LWPs that don't have an interesting event to report.  */
      if (stopping_threads == NOT_STOPPING_THREADS)
	for_each_inferior (&all_threads, resume_stopped_resumed_lwps);

      /* ... and find an LWP with a status to report to the core, if
	 any.  */
      event_thread = (struct thread_info *)
	find_inferior (&all_threads, status_pending_p_callback, &filter_ptid);
      if (event_thread != NULL)
	{
	  event_child = get_thread_lwp (event_thread);
	  *wstatp = event_child->status_pending;
	  event_child->status_pending_p = 0;
	  event_child->status_pending = 0;
	  break;
	}

      /* Check for zombie thread group leaders.  Those can't be reaped
	 until all other threads in the thread group are.  */
      check_zombie_leaders ();

      /* If there are no resumed children left in the set of LWPs we
	 want to wait for, bail.  We can't just block in
	 waitpid/sigsuspend, because lwps might have been left stopped
	 in trace-stop state, and we'd be stuck forever waiting for
	 their status to change (which would only happen if we resumed
	 them).  Even if WNOHANG is set, this return code is preferred
	 over 0 (below), as it is more detailed.  */
      if ((find_inferior (&all_threads,
			  not_stopped_callback,
			  &wait_ptid) == NULL))
	{
	  if (debug_threads)
	    debug_printf ("LLW: exit (no unwaited-for LWP)\n");
	  sigprocmask (SIG_SETMASK, &prev_mask, NULL);
	  return -1;
	}

      /* No interesting event to report to the caller.  */
      if ((options & WNOHANG))
	{
	  if (debug_threads)
	    debug_printf ("WNOHANG set, no event found\n");

	  sigprocmask (SIG_SETMASK, &prev_mask, NULL);
	  return 0;
	}

      /* Block until we get an event reported with SIGCHLD.  */
      if (debug_threads)
	debug_printf ("sigsuspend'ing\n");

      sigsuspend (&prev_mask);
      sigprocmask (SIG_SETMASK, &prev_mask, NULL);
      goto retry;
    }

  sigprocmask (SIG_SETMASK, &prev_mask, NULL);

  current_thread = event_thread;

  return lwpid_of (event_thread);
}

/* Wait for an event from child(ren) PTID.  PTIDs can be:
   minus_one_ptid, to specify any child; a pid PTID, specifying all
   lwps of a thread group; or a PTID representing a single lwp.  Store
   the stop status through the status pointer WSTAT.  OPTIONS is
   passed to the waitpid call.  Return 0 if no event was found and
   OPTIONS contains WNOHANG.  Return -1 if no unwaited-for children
   was found.  Return the PID of the stopped child otherwise.  */

static int
linux_wait_for_event (ptid_t ptid, int *wstatp, int options)
{
  return linux_wait_for_event_filtered (ptid, ptid, wstatp, options);
}

/* Count the LWP's that have had events.  */

static int
count_events_callback (struct inferior_list_entry *entry, void *data)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lp = get_thread_lwp (thread);
  int *count = (int *) data;

  gdb_assert (count != NULL);

  /* Count only resumed LWPs that have an event pending. */
  if (thread->last_status.kind == TARGET_WAITKIND_IGNORE
      && lp->status_pending_p)
    (*count)++;

  return 0;
}

/* Select the LWP (if any) that is currently being single-stepped.  */

static int
select_singlestep_lwp_callback (struct inferior_list_entry *entry, void *data)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lp = get_thread_lwp (thread);

  if (thread->last_status.kind == TARGET_WAITKIND_IGNORE
      && thread->last_resume_kind == resume_step
      && lp->status_pending_p)
    return 1;
  else
    return 0;
}

/* Select the Nth LWP that has had an event.  */

static int
select_event_lwp_callback (struct inferior_list_entry *entry, void *data)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lp = get_thread_lwp (thread);
  int *selector = (int *) data;

  gdb_assert (selector != NULL);

  /* Select only resumed LWPs that have an event pending. */
  if (thread->last_status.kind == TARGET_WAITKIND_IGNORE
      && lp->status_pending_p)
    if ((*selector)-- == 0)
      return 1;

  return 0;
}

/* Select one LWP out of those that have events pending.  */

static void
select_event_lwp (struct lwp_info **orig_lp)
{
  int num_events = 0;
  int random_selector;
  struct thread_info *event_thread = NULL;

  /* In all-stop, give preference to the LWP that is being
     single-stepped.  There will be at most one, and it's the LWP that
     the core is most interested in.  If we didn't do this, then we'd
     have to handle pending step SIGTRAPs somehow in case the core
     later continues the previously-stepped thread, otherwise we'd
     report the pending SIGTRAP, and the core, not having stepped the
     thread, wouldn't understand what the trap was for, and therefore
     would report it to the user as a random signal.  */
  if (!non_stop)
    {
      event_thread
	= (struct thread_info *) find_inferior (&all_threads,
						select_singlestep_lwp_callback,
						NULL);
      if (event_thread != NULL)
	{
	  if (debug_threads)
	    debug_printf ("SEL: Select single-step %s\n",
			  target_pid_to_str (ptid_of (event_thread)));
	}
    }
  if (event_thread == NULL)
    {
      /* No single-stepping LWP.  Select one at random, out of those
         which have had events.  */

      /* First see how many events we have.  */
      find_inferior (&all_threads, count_events_callback, &num_events);
      gdb_assert (num_events > 0);

      /* Now randomly pick a LWP out of those that have had
	 events.  */
      random_selector = (int)
	((num_events * (double) rand ()) / (RAND_MAX + 1.0));

      if (debug_threads && num_events > 1)
	debug_printf ("SEL: Found %d SIGTRAP events, selecting #%d\n",
		      num_events, random_selector);

      event_thread
	= (struct thread_info *) find_inferior (&all_threads,
						select_event_lwp_callback,
						&random_selector);
    }

  if (event_thread != NULL)
    {
      struct lwp_info *event_lp = get_thread_lwp (event_thread);

      /* Switch the event LWP.  */
      *orig_lp = event_lp;
    }
}

/* Decrement the suspend count of an LWP.  */

static int
unsuspend_one_lwp (struct inferior_list_entry *entry, void *except)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);

  /* Ignore EXCEPT.  */
  if (lwp == except)
    return 0;

  lwp_suspended_decr (lwp);
  return 0;
}

/* Decrement the suspend count of all LWPs, except EXCEPT, if non
   NULL.  */

static void
unsuspend_all_lwps (struct lwp_info *except)
{
  find_inferior (&all_threads, unsuspend_one_lwp, except);
}

static void move_out_of_jump_pad_callback (struct inferior_list_entry *entry);
static int stuck_in_jump_pad_callback (struct inferior_list_entry *entry,
				       void *data);
static int lwp_running (struct inferior_list_entry *entry, void *data);
static ptid_t linux_wait_1 (ptid_t ptid,
			    struct target_waitstatus *ourstatus,
			    int target_options);

/* Stabilize threads (move out of jump pads).

   If a thread is midway collecting a fast tracepoint, we need to
   finish the collection and move it out of the jump pad before
   reporting the signal.

   This avoids recursion while collecting (when a signal arrives
   midway, and the signal handler itself collects), which would trash
   the trace buffer.  In case the user set a breakpoint in a signal
   handler, this avoids the backtrace showing the jump pad, etc..
   Most importantly, there are certain things we can't do safely if
   threads are stopped in a jump pad (or in its callee's).  For
   example:

     - starting a new trace run.  A thread still collecting the
   previous run, could trash the trace buffer when resumed.  The trace
   buffer control structures would have been reset but the thread had
   no way to tell.  The thread could even midway memcpy'ing to the
   buffer, which would mean that when resumed, it would clobber the
   trace buffer that had been set for a new run.

     - we can't rewrite/reuse the jump pads for new tracepoints
   safely.  Say you do tstart while a thread is stopped midway while
   collecting.  When the thread is later resumed, it finishes the
   collection, and returns to the jump pad, to execute the original
   instruction that was under the tracepoint jump at the time the
   older run had been started.  If the jump pad had been rewritten
   since for something else in the new run, the thread would now
   execute the wrong / random instructions.  */

static void
linux_stabilize_threads (void)
{
  struct thread_info *saved_thread;
  struct thread_info *thread_stuck;

  thread_stuck
    = (struct thread_info *) find_inferior (&all_threads,
					    stuck_in_jump_pad_callback,
					    NULL);
  if (thread_stuck != NULL)
    {
      if (debug_threads)
	debug_printf ("can't stabilize, LWP %ld is stuck in jump pad\n",
		      lwpid_of (thread_stuck));
      return;
    }

  saved_thread = current_thread;

  stabilizing_threads = 1;

  /* Kick 'em all.  */
  for_each_inferior (&all_threads, move_out_of_jump_pad_callback);

  /* Loop until all are stopped out of the jump pads.  */
  while (find_inferior (&all_threads, lwp_running, NULL) != NULL)
    {
      struct target_waitstatus ourstatus;
      struct lwp_info *lwp;
      int wstat;

      /* Note that we go through the full wait even loop.  While
	 moving threads out of jump pad, we need to be able to step
	 over internal breakpoints and such.  */
      linux_wait_1 (minus_one_ptid, &ourstatus, 0);

      if (ourstatus.kind == TARGET_WAITKIND_STOPPED)
	{
	  lwp = get_thread_lwp (current_thread);

	  /* Lock it.  */
	  lwp_suspended_inc (lwp);

	  if (ourstatus.value.sig != GDB_SIGNAL_0
	      || current_thread->last_resume_kind == resume_stop)
	    {
	      wstat = W_STOPCODE (gdb_signal_to_host (ourstatus.value.sig));
	      enqueue_one_deferred_signal (lwp, &wstat);
	    }
	}
    }

  unsuspend_all_lwps (NULL);

  stabilizing_threads = 0;

  current_thread = saved_thread;

  if (debug_threads)
    {
      thread_stuck
	= (struct thread_info *) find_inferior (&all_threads,
						stuck_in_jump_pad_callback,
						NULL);
      if (thread_stuck != NULL)
	debug_printf ("couldn't stabilize, LWP %ld got stuck in jump pad\n",
		      lwpid_of (thread_stuck));
    }
}

/* Convenience function that is called when the kernel reports an
   event that is not passed out to GDB.  */

static ptid_t
ignore_event (struct target_waitstatus *ourstatus)
{
  /* If we got an event, there may still be others, as a single
     SIGCHLD can indicate more than one child stopped.  This forces
     another target_wait call.  */
  async_file_mark ();

  ourstatus->kind = TARGET_WAITKIND_IGNORE;
  return null_ptid;
}

/* Convenience function that is called when the kernel reports an exit
   event.  This decides whether to report the event to GDB as a
   process exit event, a thread exit event, or to suppress the
   event.  */

static ptid_t
filter_exit_event (struct lwp_info *event_child,
		   struct target_waitstatus *ourstatus)
{
  struct thread_info *thread = get_lwp_thread (event_child);
  ptid_t ptid = ptid_of (thread);

  if (!last_thread_of_process_p (pid_of (thread)))
    {
      if (report_thread_events)
	ourstatus->kind = TARGET_WAITKIND_THREAD_EXITED;
      else
	ourstatus->kind = TARGET_WAITKIND_IGNORE;

      delete_lwp (event_child);
    }
  return ptid;
}

/* Returns 1 if GDB is interested in any event_child syscalls.  */

static int
gdb_catching_syscalls_p (struct lwp_info *event_child)
{
  struct thread_info *thread = get_lwp_thread (event_child);
  struct process_info *proc = get_thread_process (thread);

  return !VEC_empty (int, proc->syscalls_to_catch);
}

/* Returns 1 if GDB is interested in the event_child syscall.
   Only to be called when stopped reason is SYSCALL_SIGTRAP.  */

static int
gdb_catch_this_syscall_p (struct lwp_info *event_child)
{
  int i, iter;
  int sysno;
  struct thread_info *thread = get_lwp_thread (event_child);
  struct process_info *proc = get_thread_process (thread);

  if (VEC_empty (int, proc->syscalls_to_catch))
    return 0;

  if (VEC_index (int, proc->syscalls_to_catch, 0) == ANY_SYSCALL)
    return 1;

  get_syscall_trapinfo (event_child, &sysno);
  for (i = 0;
       VEC_iterate (int, proc->syscalls_to_catch, i, iter);
       i++)
    if (iter == sysno)
      return 1;

  return 0;
}

/* Wait for process, returns status.  */

static ptid_t
linux_wait_1 (ptid_t ptid,
	      struct target_waitstatus *ourstatus, int target_options)
{
  int w;
  struct lwp_info *event_child;
  int options;
  int pid;
  int step_over_finished;
  int bp_explains_trap;
  int maybe_internal_trap;
  int report_to_gdb;
  int trace_event;
  int in_step_range;
  int any_resumed;

  if (debug_threads)
    {
      debug_enter ();
      debug_printf ("linux_wait_1: [%s]\n", target_pid_to_str (ptid));
    }

  /* Translate generic target options into linux options.  */
  options = __WALL;
  if (target_options & TARGET_WNOHANG)
    options |= WNOHANG;

  bp_explains_trap = 0;
  trace_event = 0;
  in_step_range = 0;
  ourstatus->kind = TARGET_WAITKIND_IGNORE;

  /* Find a resumed LWP, if any.  */
  if (find_inferior (&all_threads,
		     status_pending_p_callback,
		     &minus_one_ptid) != NULL)
    any_resumed = 1;
  else if ((find_inferior (&all_threads,
			   not_stopped_callback,
			   &minus_one_ptid) != NULL))
    any_resumed = 1;
  else
    any_resumed = 0;

  if (ptid_equal (step_over_bkpt, null_ptid))
    pid = linux_wait_for_event (ptid, &w, options);
  else
    {
      if (debug_threads)
	debug_printf ("step_over_bkpt set [%s], doing a blocking wait\n",
		      target_pid_to_str (step_over_bkpt));
      pid = linux_wait_for_event (step_over_bkpt, &w, options & ~WNOHANG);
    }

  if (pid == 0 || (pid == -1 && !any_resumed))
    {
      gdb_assert (target_options & TARGET_WNOHANG);

      if (debug_threads)
	{
	  debug_printf ("linux_wait_1 ret = null_ptid, "
			"TARGET_WAITKIND_IGNORE\n");
	  debug_exit ();
	}

      ourstatus->kind = TARGET_WAITKIND_IGNORE;
      return null_ptid;
    }
  else if (pid == -1)
    {
      if (debug_threads)
	{
	  debug_printf ("linux_wait_1 ret = null_ptid, "
			"TARGET_WAITKIND_NO_RESUMED\n");
	  debug_exit ();
	}

      ourstatus->kind = TARGET_WAITKIND_NO_RESUMED;
      return null_ptid;
    }

  event_child = get_thread_lwp (current_thread);

  /* linux_wait_for_event only returns an exit status for the last
     child of a process.  Report it.  */
  if (WIFEXITED (w) || WIFSIGNALED (w))
    {
      if (WIFEXITED (w))
	{
	  ourstatus->kind = TARGET_WAITKIND_EXITED;
	  ourstatus->value.integer = WEXITSTATUS (w);

	  if (debug_threads)
	    {
	      debug_printf ("linux_wait_1 ret = %s, exited with "
			    "retcode %d\n",
			    target_pid_to_str (ptid_of (current_thread)),
			    WEXITSTATUS (w));
	      debug_exit ();
	    }
	}
      else
	{
	  ourstatus->kind = TARGET_WAITKIND_SIGNALLED;
	  ourstatus->value.sig = gdb_signal_from_host (WTERMSIG (w));

	  if (debug_threads)
	    {
	      debug_printf ("linux_wait_1 ret = %s, terminated with "
			    "signal %d\n",
			    target_pid_to_str (ptid_of (current_thread)),
			    WTERMSIG (w));
	      debug_exit ();
	    }
	}

      if (ourstatus->kind == TARGET_WAITKIND_EXITED)
	return filter_exit_event (event_child, ourstatus);

      return ptid_of (current_thread);
    }

  /* If step-over executes a breakpoint instruction, in the case of a
     hardware single step it means a gdb/gdbserver breakpoint had been
     planted on top of a permanent breakpoint, in the case of a software
     single step it may just mean that gdbserver hit the reinsert breakpoint.
     The PC has been adjusted by save_stop_reason to point at
     the breakpoint address.
     So in the case of the hardware single step advance the PC manually
     past the breakpoint and in the case of software single step advance only
     if it's not the reinsert_breakpoint we are hitting.
     This avoids that a program would keep trapping a permanent breakpoint
     forever.  */
  if (!ptid_equal (step_over_bkpt, null_ptid)
      && event_child->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT
      && (event_child->stepping
	  || !reinsert_breakpoint_inserted_here (event_child->stop_pc)))
    {
      int increment_pc = 0;
      int breakpoint_kind = 0;
      CORE_ADDR stop_pc = event_child->stop_pc;

      breakpoint_kind =
	the_target->breakpoint_kind_from_current_state (&stop_pc);
      the_target->sw_breakpoint_from_kind (breakpoint_kind, &increment_pc);

      if (debug_threads)
	{
	  debug_printf ("step-over for %s executed software breakpoint\n",
			target_pid_to_str (ptid_of (current_thread)));
	}

      if (increment_pc != 0)
	{
	  struct regcache *regcache
	    = get_thread_regcache (current_thread, 1);

	  event_child->stop_pc += increment_pc;
	  (*the_low_target.set_pc) (regcache, event_child->stop_pc);

	  if (!(*the_low_target.breakpoint_at) (event_child->stop_pc))
	    event_child->stop_reason = TARGET_STOPPED_BY_NO_REASON;
	}
    }

  /* If this event was not handled before, and is not a SIGTRAP, we
     report it.  SIGILL and SIGSEGV are also treated as traps in case
     a breakpoint is inserted at the current PC.  If this target does
     not support internal breakpoints at all, we also report the
     SIGTRAP without further processing; it's of no concern to us.  */
  maybe_internal_trap
    = (supports_breakpoints ()
       && (WSTOPSIG (w) == SIGTRAP
	   || ((WSTOPSIG (w) == SIGILL
		|| WSTOPSIG (w) == SIGSEGV)
	       && (*the_low_target.breakpoint_at) (event_child->stop_pc))));

  if (maybe_internal_trap)
    {
      /* Handle anything that requires bookkeeping before deciding to
	 report the event or continue waiting.  */

      /* First check if we can explain the SIGTRAP with an internal
	 breakpoint, or if we should possibly report the event to GDB.
	 Do this before anything that may remove or insert a
	 breakpoint.  */
      bp_explains_trap = breakpoint_inserted_here (event_child->stop_pc);

      /* We have a SIGTRAP, possibly a step-over dance has just
	 finished.  If so, tweak the state machine accordingly,
	 reinsert breakpoints and delete any reinsert (software
	 single-step) breakpoints.  */
      step_over_finished = finish_step_over (event_child);

      /* Now invoke the callbacks of any internal breakpoints there.  */
      check_breakpoints (event_child->stop_pc);

      /* Handle tracepoint data collecting.  This may overflow the
	 trace buffer, and cause a tracing stop, removing
	 breakpoints.  */
      trace_event = handle_tracepoints (event_child);

      if (bp_explains_trap)
	{
	  if (debug_threads)
	    debug_printf ("Hit a gdbserver breakpoint.\n");
	}
    }
  else
    {
      /* We have some other signal, possibly a step-over dance was in
	 progress, and it should be cancelled too.  */
      step_over_finished = finish_step_over (event_child);
    }

  /* We have all the data we need.  Either report the event to GDB, or
     resume threads and keep waiting for more.  */

  /* If we're collecting a fast tracepoint, finish the collection and
     move out of the jump pad before delivering a signal.  See
     linux_stabilize_threads.  */

  if (WIFSTOPPED (w)
      && WSTOPSIG (w) != SIGTRAP
      && supports_fast_tracepoints ()
      && agent_loaded_p ())
    {
      if (debug_threads)
	debug_printf ("Got signal %d for LWP %ld.  Check if we need "
		      "to defer or adjust it.\n",
		      WSTOPSIG (w), lwpid_of (current_thread));

      /* Allow debugging the jump pad itself.  */
      if (current_thread->last_resume_kind != resume_step
	  && maybe_move_out_of_jump_pad (event_child, &w))
	{
	  enqueue_one_deferred_signal (event_child, &w);

	  if (debug_threads)
	    debug_printf ("Signal %d for LWP %ld deferred (in jump pad)\n",
			  WSTOPSIG (w), lwpid_of (current_thread));

	  linux_resume_one_lwp (event_child, 0, 0, NULL);

	  return ignore_event (ourstatus);
	}
    }

  if (event_child->collecting_fast_tracepoint)
    {
      if (debug_threads)
	debug_printf ("LWP %ld was trying to move out of the jump pad (%d). "
		      "Check if we're already there.\n",
		      lwpid_of (current_thread),
		      event_child->collecting_fast_tracepoint);

      trace_event = 1;

      event_child->collecting_fast_tracepoint
	= linux_fast_tracepoint_collecting (event_child, NULL);

      if (event_child->collecting_fast_tracepoint != 1)
	{
	  /* No longer need this breakpoint.  */
	  if (event_child->exit_jump_pad_bkpt != NULL)
	    {
	      if (debug_threads)
		debug_printf ("No longer need exit-jump-pad bkpt; removing it."
			      "stopping all threads momentarily.\n");

	      /* Other running threads could hit this breakpoint.
		 We don't handle moribund locations like GDB does,
		 instead we always pause all threads when removing
		 breakpoints, so that any step-over or
		 decr_pc_after_break adjustment is always taken
		 care of while the breakpoint is still
		 inserted.  */
	      stop_all_lwps (1, event_child);

	      delete_breakpoint (event_child->exit_jump_pad_bkpt);
	      event_child->exit_jump_pad_bkpt = NULL;

	      unstop_all_lwps (1, event_child);

	      gdb_assert (event_child->suspended >= 0);
	    }
	}

      if (event_child->collecting_fast_tracepoint == 0)
	{
	  if (debug_threads)
	    debug_printf ("fast tracepoint finished "
			  "collecting successfully.\n");

	  /* We may have a deferred signal to report.  */
	  if (dequeue_one_deferred_signal (event_child, &w))
	    {
	      if (debug_threads)
		debug_printf ("dequeued one signal.\n");
	    }
	  else
	    {
	      if (debug_threads)
		debug_printf ("no deferred signals.\n");

	      if (stabilizing_threads)
		{
		  ourstatus->kind = TARGET_WAITKIND_STOPPED;
		  ourstatus->value.sig = GDB_SIGNAL_0;

		  if (debug_threads)
		    {
		      debug_printf ("linux_wait_1 ret = %s, stopped "
				    "while stabilizing threads\n",
				    target_pid_to_str (ptid_of (current_thread)));
		      debug_exit ();
		    }

		  return ptid_of (current_thread);
		}
	    }
	}
    }

  /* Check whether GDB would be interested in this event.  */

  /* Check if GDB is interested in this syscall.  */
  if (WIFSTOPPED (w)
      && WSTOPSIG (w) == SYSCALL_SIGTRAP
      && !gdb_catch_this_syscall_p (event_child))
    {
      if (debug_threads)
	{
	  debug_printf ("Ignored syscall for LWP %ld.\n",
			lwpid_of (current_thread));
	}

      linux_resume_one_lwp (event_child, event_child->stepping,
			    0, NULL);
      return ignore_event (ourstatus);
    }

  /* If GDB is not interested in this signal, don't stop other
     threads, and don't report it to GDB.  Just resume the inferior
     right away.  We do this for threading-related signals as well as
     any that GDB specifically requested we ignore.  But never ignore
     SIGSTOP if we sent it ourselves, and do not ignore signals when
     stepping - they may require special handling to skip the signal
     handler. Also never ignore signals that could be caused by a
     breakpoint.  */
  if (WIFSTOPPED (w)
      && current_thread->last_resume_kind != resume_step
      && (
#if defined (USE_THREAD_DB) && !defined (__ANDROID__)
	  (current_process ()->priv->thread_db != NULL
	   && (WSTOPSIG (w) == __SIGRTMIN
	       || WSTOPSIG (w) == __SIGRTMIN + 1))
	  ||
#endif
	  (pass_signals[gdb_signal_from_host (WSTOPSIG (w))]
	   && !(WSTOPSIG (w) == SIGSTOP
		&& current_thread->last_resume_kind == resume_stop)
	   && !linux_wstatus_maybe_breakpoint (w))))
    {
      siginfo_t info, *info_p;

      if (debug_threads)
	debug_printf ("Ignored signal %d for LWP %ld.\n",
		      WSTOPSIG (w), lwpid_of (current_thread));

      if (ptrace (PTRACE_GETSIGINFO, lwpid_of (current_thread),
		  (PTRACE_TYPE_ARG3) 0, &info) == 0)
	info_p = &info;
      else
	info_p = NULL;

      if (step_over_finished)
	{
	  /* We cancelled this thread's step-over above.  We still
	     need to unsuspend all other LWPs, and set them back
	     running again while the signal handler runs.  */
	  unsuspend_all_lwps (event_child);

	  /* Enqueue the pending signal info so that proceed_all_lwps
	     doesn't lose it.  */
	  enqueue_pending_signal (event_child, WSTOPSIG (w), info_p);

	  proceed_all_lwps ();
	}
      else
	{
	  linux_resume_one_lwp (event_child, event_child->stepping,
				WSTOPSIG (w), info_p);
	}
      return ignore_event (ourstatus);
    }

  /* Note that all addresses are always "out of the step range" when
     there's no range to begin with.  */
  in_step_range = lwp_in_step_range (event_child);

  /* If GDB wanted this thread to single step, and the thread is out
     of the step range, we always want to report the SIGTRAP, and let
     GDB handle it.  Watchpoints should always be reported.  So should
     signals we can't explain.  A SIGTRAP we can't explain could be a
     GDB breakpoint --- we may or not support Z0 breakpoints.  If we
     do, we're be able to handle GDB breakpoints on top of internal
     breakpoints, by handling the internal breakpoint and still
     reporting the event to GDB.  If we don't, we're out of luck, GDB
     won't see the breakpoint hit.  If we see a single-step event but
     the thread should be continuing, don't pass the trap to gdb.
     That indicates that we had previously finished a single-step but
     left the single-step pending -- see
     complete_ongoing_step_over.  */
  report_to_gdb = (!maybe_internal_trap
		   || (current_thread->last_resume_kind == resume_step
		       && !in_step_range)
		   || event_child->stop_reason == TARGET_STOPPED_BY_WATCHPOINT
		   || (!in_step_range
		       && !bp_explains_trap
		       && !trace_event
		       && !step_over_finished
		       && !(current_thread->last_resume_kind == resume_continue
			    && event_child->stop_reason == TARGET_STOPPED_BY_SINGLE_STEP))
		   || (gdb_breakpoint_here (event_child->stop_pc)
		       && gdb_condition_true_at_breakpoint (event_child->stop_pc)
		       && gdb_no_commands_at_breakpoint (event_child->stop_pc))
		   || event_child->waitstatus.kind != TARGET_WAITKIND_IGNORE);

  run_breakpoint_commands (event_child->stop_pc);

  /* We found no reason GDB would want us to stop.  We either hit one
     of our own breakpoints, or finished an internal step GDB
     shouldn't know about.  */
  if (!report_to_gdb)
    {
      if (debug_threads)
	{
	  if (bp_explains_trap)
	    debug_printf ("Hit a gdbserver breakpoint.\n");
	  if (step_over_finished)
	    debug_printf ("Step-over finished.\n");
	  if (trace_event)
	    debug_printf ("Tracepoint event.\n");
	  if (lwp_in_step_range (event_child))
	    debug_printf ("Range stepping pc 0x%s [0x%s, 0x%s).\n",
			  paddress (event_child->stop_pc),
			  paddress (event_child->step_range_start),
			  paddress (event_child->step_range_end));
	}

      /* We're not reporting this breakpoint to GDB, so apply the
	 decr_pc_after_break adjustment to the inferior's regcache
	 ourselves.  */

      if (the_low_target.set_pc != NULL)
	{
	  struct regcache *regcache
	    = get_thread_regcache (current_thread, 1);
	  (*the_low_target.set_pc) (regcache, event_child->stop_pc);
	}

      /* We may have finished stepping over a breakpoint.  If so,
	 we've stopped and suspended all LWPs momentarily except the
	 stepping one.  This is where we resume them all again.  We're
	 going to keep waiting, so use proceed, which handles stepping
	 over the next breakpoint.  */
      if (debug_threads)
	debug_printf ("proceeding all threads.\n");

      if (step_over_finished)
	unsuspend_all_lwps (event_child);

      proceed_all_lwps ();
      return ignore_event (ourstatus);
    }

  if (debug_threads)
    {
      if (event_child->waitstatus.kind != TARGET_WAITKIND_IGNORE)
	{
	  char *str;

	  str = target_waitstatus_to_string (&event_child->waitstatus);
	  debug_printf ("LWP %ld: extended event with waitstatus %s\n",
			lwpid_of (get_lwp_thread (event_child)), str);
	  xfree (str);
	}
      if (current_thread->last_resume_kind == resume_step)
	{
	  if (event_child->step_range_start == event_child->step_range_end)
	    debug_printf ("GDB wanted to single-step, reporting event.\n");
	  else if (!lwp_in_step_range (event_child))
	    debug_printf ("Out of step range, reporting event.\n");
	}
      if (event_child->stop_reason == TARGET_STOPPED_BY_WATCHPOINT)
	debug_printf ("Stopped by watchpoint.\n");
      else if (gdb_breakpoint_here (event_child->stop_pc))
	debug_printf ("Stopped by GDB breakpoint.\n");
      if (debug_threads)
	debug_printf ("Hit a non-gdbserver trap event.\n");
    }

  /* Alright, we're going to report a stop.  */

  /* Remove reinsert breakpoints.  */
  if (can_software_single_step ())
    {
      /* Remove reinsert breakpoints or not.  It it is true, stop all
	 lwps, so that other threads won't hit the breakpoint in the
	 staled memory.  */
      int remove_reinsert_breakpoints_p = 0;

      if (non_stop)
	{
	  remove_reinsert_breakpoints_p
	    = has_reinsert_breakpoints (current_thread);
	}
      else
	{
	  /* In all-stop, a stop reply cancels all previous resume
	     requests.  Delete all reinsert breakpoints.  */
	  struct inferior_list_entry *inf, *tmp;

	  ALL_INFERIORS (&all_threads, inf, tmp)
	    {
	      struct thread_info *thread = (struct thread_info *) inf;

	      if (has_reinsert_breakpoints (thread))
		{
		  remove_reinsert_breakpoints_p = 1;
		  break;
		}
	    }
	}

      if (remove_reinsert_breakpoints_p)
	{
	  /* If we remove reinsert breakpoints from memory, stop all lwps,
	     so that other threads won't hit the breakpoint in the staled
	     memory.  */
	  stop_all_lwps (0, event_child);

	  if (non_stop)
	    {
	      gdb_assert (has_reinsert_breakpoints (current_thread));
	      delete_reinsert_breakpoints (current_thread);
	    }
	  else
	    {
	      struct inferior_list_entry *inf, *tmp;

	      ALL_INFERIORS (&all_threads, inf, tmp)
		{
		  struct thread_info *thread = (struct thread_info *) inf;

		  if (has_reinsert_breakpoints (thread))
		    delete_reinsert_breakpoints (thread);
		}
	    }

	  unstop_all_lwps (0, event_child);
	}
    }

  if (!stabilizing_threads)
    {
      /* In all-stop, stop all threads.  */
      if (!non_stop)
	stop_all_lwps (0, NULL);

      if (step_over_finished)
	{
	  if (!non_stop)
	    {
	      /* If we were doing a step-over, all other threads but
		 the stepping one had been paused in start_step_over,
		 with their suspend counts incremented.  We don't want
		 to do a full unstop/unpause, because we're in
		 all-stop mode (so we want threads stopped), but we
		 still need to unsuspend the other threads, to
		 decrement their `suspended' count back.  */
	      unsuspend_all_lwps (event_child);
	    }
	  else
	    {
	      /* If we just finished a step-over, then all threads had
		 been momentarily paused.  In all-stop, that's fine,
		 we want threads stopped by now anyway.  In non-stop,
		 we need to re-resume threads that GDB wanted to be
		 running.  */
	      unstop_all_lwps (1, event_child);
	    }
	}

      /* If we're not waiting for a specific LWP, choose an event LWP
	 from among those that have had events.  Giving equal priority
	 to all LWPs that have had events helps prevent
	 starvation.  */
      if (ptid_equal (ptid, minus_one_ptid))
	{
	  event_child->status_pending_p = 1;
	  event_child->status_pending = w;

	  select_event_lwp (&event_child);

	  /* current_thread and event_child must stay in sync.  */
	  current_thread = get_lwp_thread (event_child);

	  event_child->status_pending_p = 0;
	  w = event_child->status_pending;
	}


      /* Stabilize threads (move out of jump pads).  */
      if (!non_stop)
	stabilize_threads ();
    }
  else
    {
      /* If we just finished a step-over, then all threads had been
	 momentarily paused.  In all-stop, that's fine, we want
	 threads stopped by now anyway.  In non-stop, we need to
	 re-resume threads that GDB wanted to be running.  */
      if (step_over_finished)
	unstop_all_lwps (1, event_child);
    }

  if (event_child->waitstatus.kind != TARGET_WAITKIND_IGNORE)
    {
      /* If the reported event is an exit, fork, vfork or exec, let
	 GDB know.  */
      *ourstatus = event_child->waitstatus;
      /* Clear the event lwp's waitstatus since we handled it already.  */
      event_child->waitstatus.kind = TARGET_WAITKIND_IGNORE;
    }
  else
    ourstatus->kind = TARGET_WAITKIND_STOPPED;

  /* Now that we've selected our final event LWP, un-adjust its PC if
     it was a software breakpoint, and the client doesn't know we can
     adjust the breakpoint ourselves.  */
  if (event_child->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT
      && !swbreak_feature)
    {
      int decr_pc = the_low_target.decr_pc_after_break;

      if (decr_pc != 0)
	{
	  struct regcache *regcache
	    = get_thread_regcache (current_thread, 1);
	  (*the_low_target.set_pc) (regcache, event_child->stop_pc + decr_pc);
	}
    }

  if (WSTOPSIG (w) == SYSCALL_SIGTRAP)
    {
      get_syscall_trapinfo (event_child,
			    &ourstatus->value.syscall_number);
      ourstatus->kind = event_child->syscall_state;
    }
  else if (current_thread->last_resume_kind == resume_stop
	   && WSTOPSIG (w) == SIGSTOP)
    {
      /* A thread that has been requested to stop by GDB with vCont;t,
	 and it stopped cleanly, so report as SIG0.  The use of
	 SIGSTOP is an implementation detail.  */
      ourstatus->value.sig = GDB_SIGNAL_0;
    }
  else if (current_thread->last_resume_kind == resume_stop
	   && WSTOPSIG (w) != SIGSTOP)
    {
      /* A thread that has been requested to stop by GDB with vCont;t,
	 but, it stopped for other reasons.  */
      ourstatus->value.sig = gdb_signal_from_host (WSTOPSIG (w));
    }
  else if (ourstatus->kind == TARGET_WAITKIND_STOPPED)
    {
      ourstatus->value.sig = gdb_signal_from_host (WSTOPSIG (w));
    }

  gdb_assert (ptid_equal (step_over_bkpt, null_ptid));

  if (debug_threads)
    {
      debug_printf ("linux_wait_1 ret = %s, %d, %d\n",
		    target_pid_to_str (ptid_of (current_thread)),
		    ourstatus->kind, ourstatus->value.sig);
      debug_exit ();
    }

  if (ourstatus->kind == TARGET_WAITKIND_EXITED)
    return filter_exit_event (event_child, ourstatus);

  return ptid_of (current_thread);
}

/* Get rid of any pending event in the pipe.  */
static void
async_file_flush (void)
{
  int ret;
  char buf;

  do
    ret = read (linux_event_pipe[0], &buf, 1);
  while (ret >= 0 || (ret == -1 && errno == EINTR));
}

/* Put something in the pipe, so the event loop wakes up.  */
static void
async_file_mark (void)
{
  int ret;

  async_file_flush ();

  do
    ret = write (linux_event_pipe[1], "+", 1);
  while (ret == 0 || (ret == -1 && errno == EINTR));

  /* Ignore EAGAIN.  If the pipe is full, the event loop will already
     be awakened anyway.  */
}

static ptid_t
linux_wait (ptid_t ptid,
	    struct target_waitstatus *ourstatus, int target_options)
{
  ptid_t event_ptid;

  /* Flush the async file first.  */
  if (target_is_async_p ())
    async_file_flush ();

  do
    {
      event_ptid = linux_wait_1 (ptid, ourstatus, target_options);
    }
  while ((target_options & TARGET_WNOHANG) == 0
	 && ptid_equal (event_ptid, null_ptid)
	 && ourstatus->kind == TARGET_WAITKIND_IGNORE);

  /* If at least one stop was reported, there may be more.  A single
     SIGCHLD can signal more than one child stop.  */
  if (target_is_async_p ()
      && (target_options & TARGET_WNOHANG) != 0
      && !ptid_equal (event_ptid, null_ptid))
    async_file_mark ();

  return event_ptid;
}

/* Send a signal to an LWP.  */

static int
kill_lwp (unsigned long lwpid, int signo)
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

void
linux_stop_lwp (struct lwp_info *lwp)
{
  send_sigstop (lwp);
}

static void
send_sigstop (struct lwp_info *lwp)
{
  int pid;

  pid = lwpid_of (get_lwp_thread (lwp));

  /* If we already have a pending stop signal for this process, don't
     send another.  */
  if (lwp->stop_expected)
    {
      if (debug_threads)
	debug_printf ("Have pending sigstop for lwp %d\n", pid);

      return;
    }

  if (debug_threads)
    debug_printf ("Sending sigstop to lwp %d\n", pid);

  lwp->stop_expected = 1;
  kill_lwp (pid, SIGSTOP);
}

static int
send_sigstop_callback (struct inferior_list_entry *entry, void *except)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);

  /* Ignore EXCEPT.  */
  if (lwp == except)
    return 0;

  if (lwp->stopped)
    return 0;

  send_sigstop (lwp);
  return 0;
}

/* Increment the suspend count of an LWP, and stop it, if not stopped
   yet.  */
static int
suspend_and_send_sigstop_callback (struct inferior_list_entry *entry,
				   void *except)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);

  /* Ignore EXCEPT.  */
  if (lwp == except)
    return 0;

  lwp_suspended_inc (lwp);

  return send_sigstop_callback (entry, except);
}

static void
mark_lwp_dead (struct lwp_info *lwp, int wstat)
{
  /* Store the exit status for later.  */
  lwp->status_pending_p = 1;
  lwp->status_pending = wstat;

  /* Store in waitstatus as well, as there's nothing else to process
     for this event.  */
  if (WIFEXITED (wstat))
    {
      lwp->waitstatus.kind = TARGET_WAITKIND_EXITED;
      lwp->waitstatus.value.integer = WEXITSTATUS (wstat);
    }
  else if (WIFSIGNALED (wstat))
    {
      lwp->waitstatus.kind = TARGET_WAITKIND_SIGNALLED;
      lwp->waitstatus.value.sig = gdb_signal_from_host (WTERMSIG (wstat));
    }

  /* Prevent trying to stop it.  */
  lwp->stopped = 1;

  /* No further stops are expected from a dead lwp.  */
  lwp->stop_expected = 0;
}

/* Return true if LWP has exited already, and has a pending exit event
   to report to GDB.  */

static int
lwp_is_marked_dead (struct lwp_info *lwp)
{
  return (lwp->status_pending_p
	  && (WIFEXITED (lwp->status_pending)
	      || WIFSIGNALED (lwp->status_pending)));
}

/* Wait for all children to stop for the SIGSTOPs we just queued.  */

static void
wait_for_sigstop (void)
{
  struct thread_info *saved_thread;
  ptid_t saved_tid;
  int wstat;
  int ret;

  saved_thread = current_thread;
  if (saved_thread != NULL)
    saved_tid = saved_thread->entry.id;
  else
    saved_tid = null_ptid; /* avoid bogus unused warning */

  if (debug_threads)
    debug_printf ("wait_for_sigstop: pulling events\n");

  /* Passing NULL_PTID as filter indicates we want all events to be
     left pending.  Eventually this returns when there are no
     unwaited-for children left.  */
  ret = linux_wait_for_event_filtered (minus_one_ptid, null_ptid,
				       &wstat, __WALL);
  gdb_assert (ret == -1);

  if (saved_thread == NULL || linux_thread_alive (saved_tid))
    current_thread = saved_thread;
  else
    {
      if (debug_threads)
	debug_printf ("Previously current thread died.\n");

      /* We can't change the current inferior behind GDB's back,
	 otherwise, a subsequent command may apply to the wrong
	 process.  */
      current_thread = NULL;
    }
}

/* Returns true if LWP ENTRY is stopped in a jump pad, and we can't
   move it out, because we need to report the stop event to GDB.  For
   example, if the user puts a breakpoint in the jump pad, it's
   because she wants to debug it.  */

static int
stuck_in_jump_pad_callback (struct inferior_list_entry *entry, void *data)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);

  if (lwp->suspended != 0)
    {
      internal_error (__FILE__, __LINE__,
		      "LWP %ld is suspended, suspended=%d\n",
		      lwpid_of (thread), lwp->suspended);
    }
  gdb_assert (lwp->stopped);

  /* Allow debugging the jump pad, gdb_collect, etc..  */
  return (supports_fast_tracepoints ()
	  && agent_loaded_p ()
	  && (gdb_breakpoint_here (lwp->stop_pc)
	      || lwp->stop_reason == TARGET_STOPPED_BY_WATCHPOINT
	      || thread->last_resume_kind == resume_step)
	  && linux_fast_tracepoint_collecting (lwp, NULL));
}

static void
move_out_of_jump_pad_callback (struct inferior_list_entry *entry)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct thread_info *saved_thread;
  struct lwp_info *lwp = get_thread_lwp (thread);
  int *wstat;

  if (lwp->suspended != 0)
    {
      internal_error (__FILE__, __LINE__,
		      "LWP %ld is suspended, suspended=%d\n",
		      lwpid_of (thread), lwp->suspended);
    }
  gdb_assert (lwp->stopped);

  /* For gdb_breakpoint_here.  */
  saved_thread = current_thread;
  current_thread = thread;

  wstat = lwp->status_pending_p ? &lwp->status_pending : NULL;

  /* Allow debugging the jump pad, gdb_collect, etc.  */
  if (!gdb_breakpoint_here (lwp->stop_pc)
      && lwp->stop_reason != TARGET_STOPPED_BY_WATCHPOINT
      && thread->last_resume_kind != resume_step
      && maybe_move_out_of_jump_pad (lwp, wstat))
    {
      if (debug_threads)
	debug_printf ("LWP %ld needs stabilizing (in jump pad)\n",
		      lwpid_of (thread));

      if (wstat)
	{
	  lwp->status_pending_p = 0;
	  enqueue_one_deferred_signal (lwp, wstat);

	  if (debug_threads)
	    debug_printf ("Signal %d for LWP %ld deferred "
			  "(in jump pad)\n",
			  WSTOPSIG (*wstat), lwpid_of (thread));
	}

      linux_resume_one_lwp (lwp, 0, 0, NULL);
    }
  else
    lwp_suspended_inc (lwp);

  current_thread = saved_thread;
}

static int
lwp_running (struct inferior_list_entry *entry, void *data)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);

  if (lwp_is_marked_dead (lwp))
    return 0;
  if (lwp->stopped)
    return 0;
  return 1;
}

/* Stop all lwps that aren't stopped yet, except EXCEPT, if not NULL.
   If SUSPEND, then also increase the suspend count of every LWP,
   except EXCEPT.  */

static void
stop_all_lwps (int suspend, struct lwp_info *except)
{
  /* Should not be called recursively.  */
  gdb_assert (stopping_threads == NOT_STOPPING_THREADS);

  if (debug_threads)
    {
      debug_enter ();
      debug_printf ("stop_all_lwps (%s, except=%s)\n",
		    suspend ? "stop-and-suspend" : "stop",
		    except != NULL
		    ? target_pid_to_str (ptid_of (get_lwp_thread (except)))
		    : "none");
    }

  stopping_threads = (suspend
		      ? STOPPING_AND_SUSPENDING_THREADS
		      : STOPPING_THREADS);

  if (suspend)
    find_inferior (&all_threads, suspend_and_send_sigstop_callback, except);
  else
    find_inferior (&all_threads, send_sigstop_callback, except);
  wait_for_sigstop ();
  stopping_threads = NOT_STOPPING_THREADS;

  if (debug_threads)
    {
      debug_printf ("stop_all_lwps done, setting stopping_threads "
		    "back to !stopping\n");
      debug_exit ();
    }
}

/* Enqueue one signal in the chain of signals which need to be
   delivered to this process on next resume.  */

static void
enqueue_pending_signal (struct lwp_info *lwp, int signal, siginfo_t *info)
{
  struct pending_signals *p_sig = XNEW (struct pending_signals);

  p_sig->prev = lwp->pending_signals;
  p_sig->signal = signal;
  if (info == NULL)
    memset (&p_sig->info, 0, sizeof (siginfo_t));
  else
    memcpy (&p_sig->info, info, sizeof (siginfo_t));
  lwp->pending_signals = p_sig;
}

/* Install breakpoints for software single stepping.  */

static void
install_software_single_step_breakpoints (struct lwp_info *lwp)
{
  int i;
  CORE_ADDR pc;
  struct thread_info *thread = get_lwp_thread (lwp);
  struct regcache *regcache = get_thread_regcache (thread, 1);
  VEC (CORE_ADDR) *next_pcs = NULL;
  struct cleanup *old_chain = make_cleanup_restore_current_thread ();

  make_cleanup (VEC_cleanup (CORE_ADDR), &next_pcs);

  current_thread = thread;
  next_pcs = (*the_low_target.get_next_pcs) (regcache);

  for (i = 0; VEC_iterate (CORE_ADDR, next_pcs, i, pc); ++i)
    set_reinsert_breakpoint (pc, current_ptid);

  do_cleanups (old_chain);
}

/* Single step via hardware or software single step.
   Return 1 if hardware single stepping, 0 if software single stepping
   or can't single step.  */

static int
single_step (struct lwp_info* lwp)
{
  int step = 0;

  if (can_hardware_single_step ())
    {
      step = 1;
    }
  else if (can_software_single_step ())
    {
      install_software_single_step_breakpoints (lwp);
      step = 0;
    }
  else
    {
      if (debug_threads)
	debug_printf ("stepping is not implemented on this target");
    }

  return step;
}

/* The signal can be delivered to the inferior if we are not trying to
   finish a fast tracepoint collect.  Since signal can be delivered in
   the step-over, the program may go to signal handler and trap again
   after return from the signal handler.  We can live with the spurious
   double traps.  */

static int
lwp_signal_can_be_delivered (struct lwp_info *lwp)
{
  return !lwp->collecting_fast_tracepoint;
}

/* Resume execution of LWP.  If STEP is nonzero, single-step it.  If
   SIGNAL is nonzero, give it that signal.  */

static void
linux_resume_one_lwp_throw (struct lwp_info *lwp,
			    int step, int signal, siginfo_t *info)
{
  struct thread_info *thread = get_lwp_thread (lwp);
  struct thread_info *saved_thread;
  int fast_tp_collecting;
  int ptrace_request;
  struct process_info *proc = get_thread_process (thread);

  /* Note that target description may not be initialised
     (proc->tdesc == NULL) at this point because the program hasn't
     stopped at the first instruction yet.  It means GDBserver skips
     the extra traps from the wrapper program (see option --wrapper).
     Code in this function that requires register access should be
     guarded by proc->tdesc == NULL or something else.  */

  if (lwp->stopped == 0)
    return;

  gdb_assert (lwp->waitstatus.kind == TARGET_WAITKIND_IGNORE);

  fast_tp_collecting = lwp->collecting_fast_tracepoint;

  gdb_assert (!stabilizing_threads || fast_tp_collecting);

  /* Cancel actions that rely on GDB not changing the PC (e.g., the
     user used the "jump" command, or "set $pc = foo").  */
  if (thread->while_stepping != NULL && lwp->stop_pc != get_pc (lwp))
    {
      /* Collecting 'while-stepping' actions doesn't make sense
	 anymore.  */
      release_while_stepping_state_list (thread);
    }

  /* If we have pending signals or status, and a new signal, enqueue the
     signal.  Also enqueue the signal if it can't be delivered to the
     inferior right now.  */
  if (signal != 0
      && (lwp->status_pending_p
	  || lwp->pending_signals != NULL
	  || !lwp_signal_can_be_delivered (lwp)))
    {
      enqueue_pending_signal (lwp, signal, info);

      /* Postpone any pending signal.  It was enqueued above.  */
      signal = 0;
    }

  if (lwp->status_pending_p)
    {
      if (debug_threads)
	debug_printf ("Not resuming lwp %ld (%s, stop %s);"
		      " has pending status\n",
		      lwpid_of (thread), step ? "step" : "continue",
		      lwp->stop_expected ? "expected" : "not expected");
      return;
    }

  saved_thread = current_thread;
  current_thread = thread;

  /* This bit needs some thinking about.  If we get a signal that
     we must report while a single-step reinsert is still pending,
     we often end up resuming the thread.  It might be better to
     (ew) allow a stack of pending events; then we could be sure that
     the reinsert happened right away and not lose any signals.

     Making this stack would also shrink the window in which breakpoints are
     uninserted (see comment in linux_wait_for_lwp) but not enough for
     complete correctness, so it won't solve that problem.  It may be
     worthwhile just to solve this one, however.  */
  if (lwp->bp_reinsert != 0)
    {
      if (debug_threads)
	debug_printf ("  pending reinsert at 0x%s\n",
		      paddress (lwp->bp_reinsert));

      if (can_hardware_single_step ())
	{
	  if (fast_tp_collecting == 0)
	    {
	      if (step == 0)
		fprintf (stderr, "BAD - reinserting but not stepping.\n");
	      if (lwp->suspended)
		fprintf (stderr, "BAD - reinserting and suspended(%d).\n",
			 lwp->suspended);
	    }
	}

      step = maybe_hw_step (thread);
    }

  if (fast_tp_collecting == 1)
    {
      if (debug_threads)
	debug_printf ("lwp %ld wants to get out of fast tracepoint jump pad"
		      " (exit-jump-pad-bkpt)\n",
		      lwpid_of (thread));
    }
  else if (fast_tp_collecting == 2)
    {
      if (debug_threads)
	debug_printf ("lwp %ld wants to get out of fast tracepoint jump pad"
		      " single-stepping\n",
		      lwpid_of (thread));

      if (can_hardware_single_step ())
	step = 1;
      else
	{
	  internal_error (__FILE__, __LINE__,
			  "moving out of jump pad single-stepping"
			  " not implemented on this target");
	}
    }

  /* If we have while-stepping actions in this thread set it stepping.
     If we have a signal to deliver, it may or may not be set to
     SIG_IGN, we don't know.  Assume so, and allow collecting
     while-stepping into a signal handler.  A possible smart thing to
     do would be to set an internal breakpoint at the signal return
     address, continue, and carry on catching this while-stepping
     action only when that breakpoint is hit.  A future
     enhancement.  */
  if (thread->while_stepping != NULL)
    {
      if (debug_threads)
	debug_printf ("lwp %ld has a while-stepping action -> forcing step.\n",
		      lwpid_of (thread));

      step = single_step (lwp);
    }

  if (proc->tdesc != NULL && the_low_target.get_pc != NULL)
    {
      struct regcache *regcache = get_thread_regcache (current_thread, 1);

      lwp->stop_pc = (*the_low_target.get_pc) (regcache);

      if (debug_threads)
	{
	  debug_printf ("  %s from pc 0x%lx\n", step ? "step" : "continue",
			(long) lwp->stop_pc);
	}
    }

  /* If we have pending signals, consume one if it can be delivered to
     the inferior.  */
  if (lwp->pending_signals != NULL && lwp_signal_can_be_delivered (lwp))
    {
      struct pending_signals **p_sig;

      p_sig = &lwp->pending_signals;
      while ((*p_sig)->prev != NULL)
	p_sig = &(*p_sig)->prev;

      signal = (*p_sig)->signal;
      if ((*p_sig)->info.si_signo != 0)
	ptrace (PTRACE_SETSIGINFO, lwpid_of (thread), (PTRACE_TYPE_ARG3) 0,
		&(*p_sig)->info);

      free (*p_sig);
      *p_sig = NULL;
    }

  if (debug_threads)
    debug_printf ("Resuming lwp %ld (%s, signal %d, stop %s)\n",
		  lwpid_of (thread), step ? "step" : "continue", signal,
		  lwp->stop_expected ? "expected" : "not expected");

  if (the_low_target.prepare_to_resume != NULL)
    the_low_target.prepare_to_resume (lwp);

  regcache_invalidate_thread (thread);
  errno = 0;
  lwp->stepping = step;
  if (step)
    ptrace_request = PTRACE_SINGLESTEP;
  else if (gdb_catching_syscalls_p (lwp))
    ptrace_request = PTRACE_SYSCALL;
  else
    ptrace_request = PTRACE_CONT;
  ptrace (ptrace_request,
	  lwpid_of (thread),
	  (PTRACE_TYPE_ARG3) 0,
	  /* Coerce to a uintptr_t first to avoid potential gcc warning
	     of coercing an 8 byte integer to a 4 byte pointer.  */
	  (PTRACE_TYPE_ARG4) (uintptr_t) signal);

  current_thread = saved_thread;
  if (errno)
    perror_with_name ("resuming thread");

  /* Successfully resumed.  Clear state that no longer makes sense,
     and mark the LWP as running.  Must not do this before resuming
     otherwise if that fails other code will be confused.  E.g., we'd
     later try to stop the LWP and hang forever waiting for a stop
     status.  Note that we must not throw after this is cleared,
     otherwise handle_zombie_lwp_error would get confused.  */
  lwp->stopped = 0;
  lwp->stop_reason = TARGET_STOPPED_BY_NO_REASON;
}

/* Called when we try to resume a stopped LWP and that errors out.  If
   the LWP is no longer in ptrace-stopped state (meaning it's zombie,
   or about to become), discard the error, clear any pending status
   the LWP may have, and return true (we'll collect the exit status
   soon enough).  Otherwise, return false.  */

static int
check_ptrace_stopped_lwp_gone (struct lwp_info *lp)
{
  struct thread_info *thread = get_lwp_thread (lp);

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
  if (linux_proc_pid_is_trace_stopped_nowarn (lwpid_of (thread)) == 0)
    {
      lp->stop_reason = TARGET_STOPPED_BY_NO_REASON;
      lp->status_pending_p = 0;
      return 1;
    }
  return 0;
}

/* Like linux_resume_one_lwp_throw, but no error is thrown if the LWP
   disappears while we try to resume it.  */

static void
linux_resume_one_lwp (struct lwp_info *lwp,
		      int step, int signal, siginfo_t *info)
{
  TRY
    {
      linux_resume_one_lwp_throw (lwp, step, signal, info);
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      if (!check_ptrace_stopped_lwp_gone (lwp))
	throw_exception (ex);
    }
  END_CATCH
}

struct thread_resume_array
{
  struct thread_resume *resume;
  size_t n;
};

/* This function is called once per thread via find_inferior.
   ARG is a pointer to a thread_resume_array struct.
   We look up the thread specified by ENTRY in ARG, and mark the thread
   with a pointer to the appropriate resume request.

   This algorithm is O(threads * resume elements), but resume elements
   is small (and will remain small at least until GDB supports thread
   suspension).  */

static int
linux_set_resume_request (struct inferior_list_entry *entry, void *arg)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);
  int ndx;
  struct thread_resume_array *r;

  r = (struct thread_resume_array *) arg;

  for (ndx = 0; ndx < r->n; ndx++)
    {
      ptid_t ptid = r->resume[ndx].thread;
      if (ptid_equal (ptid, minus_one_ptid)
	  || ptid_equal (ptid, entry->id)
	  /* Handle both 'pPID' and 'pPID.-1' as meaning 'all threads
	     of PID'.  */
	  || (ptid_get_pid (ptid) == pid_of (thread)
	      && (ptid_is_pid (ptid)
		  || ptid_get_lwp (ptid) == -1)))
	{
	  if (r->resume[ndx].kind == resume_stop
	      && thread->last_resume_kind == resume_stop)
	    {
	      if (debug_threads)
		debug_printf ("already %s LWP %ld at GDB's request\n",
			      (thread->last_status.kind
			       == TARGET_WAITKIND_STOPPED)
			      ? "stopped"
			      : "stopping",
			      lwpid_of (thread));

	      continue;
	    }

	  lwp->resume = &r->resume[ndx];
	  thread->last_resume_kind = lwp->resume->kind;

	  lwp->step_range_start = lwp->resume->step_range_start;
	  lwp->step_range_end = lwp->resume->step_range_end;

	  /* If we had a deferred signal to report, dequeue one now.
	     This can happen if LWP gets more than one signal while
	     trying to get out of a jump pad.  */
	  if (lwp->stopped
	      && !lwp->status_pending_p
	      && dequeue_one_deferred_signal (lwp, &lwp->status_pending))
	    {
	      lwp->status_pending_p = 1;

	      if (debug_threads)
		debug_printf ("Dequeueing deferred signal %d for LWP %ld, "
			      "leaving status pending.\n",
			      WSTOPSIG (lwp->status_pending),
			      lwpid_of (thread));
	    }

	  return 0;
	}
    }

  /* No resume action for this thread.  */
  lwp->resume = NULL;

  return 0;
}

/* find_inferior callback for linux_resume.
   Set *FLAG_P if this lwp has an interesting status pending.  */

static int
resume_status_pending_p (struct inferior_list_entry *entry, void *flag_p)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);

  /* LWPs which will not be resumed are not interesting, because
     we might not wait for them next time through linux_wait.  */
  if (lwp->resume == NULL)
    return 0;

  if (thread_still_has_status_pending_p (thread))
    * (int *) flag_p = 1;

  return 0;
}

/* Return 1 if this lwp that GDB wants running is stopped at an
   internal breakpoint that we need to step over.  It assumes that any
   required STOP_PC adjustment has already been propagated to the
   inferior's regcache.  */

static int
need_step_over_p (struct inferior_list_entry *entry, void *dummy)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);
  struct thread_info *saved_thread;
  CORE_ADDR pc;
  struct process_info *proc = get_thread_process (thread);

  /* GDBserver is skipping the extra traps from the wrapper program,
     don't have to do step over.  */
  if (proc->tdesc == NULL)
    return 0;

  /* LWPs which will not be resumed are not interesting, because we
     might not wait for them next time through linux_wait.  */

  if (!lwp->stopped)
    {
      if (debug_threads)
	debug_printf ("Need step over [LWP %ld]? Ignoring, not stopped\n",
		      lwpid_of (thread));
      return 0;
    }

  if (thread->last_resume_kind == resume_stop)
    {
      if (debug_threads)
	debug_printf ("Need step over [LWP %ld]? Ignoring, should remain"
		      " stopped\n",
		      lwpid_of (thread));
      return 0;
    }

  gdb_assert (lwp->suspended >= 0);

  if (lwp->suspended)
    {
      if (debug_threads)
	debug_printf ("Need step over [LWP %ld]? Ignoring, suspended\n",
		      lwpid_of (thread));
      return 0;
    }

  if (lwp->status_pending_p)
    {
      if (debug_threads)
	debug_printf ("Need step over [LWP %ld]? Ignoring, has pending"
		      " status.\n",
		      lwpid_of (thread));
      return 0;
    }

  /* Note: PC, not STOP_PC.  Either GDB has adjusted the PC already,
     or we have.  */
  pc = get_pc (lwp);

  /* If the PC has changed since we stopped, then don't do anything,
     and let the breakpoint/tracepoint be hit.  This happens if, for
     instance, GDB handled the decr_pc_after_break subtraction itself,
     GDB is OOL stepping this thread, or the user has issued a "jump"
     command, or poked thread's registers herself.  */
  if (pc != lwp->stop_pc)
    {
      if (debug_threads)
	debug_printf ("Need step over [LWP %ld]? Cancelling, PC was changed. "
		      "Old stop_pc was 0x%s, PC is now 0x%s\n",
		      lwpid_of (thread),
		      paddress (lwp->stop_pc), paddress (pc));
      return 0;
    }

  /* On software single step target, resume the inferior with signal
     rather than stepping over.  */
  if (can_software_single_step ()
      && lwp->pending_signals != NULL
      && lwp_signal_can_be_delivered (lwp))
    {
      if (debug_threads)
	debug_printf ("Need step over [LWP %ld]? Ignoring, has pending"
		      " signals.\n",
		      lwpid_of (thread));

      return 0;
    }

  saved_thread = current_thread;
  current_thread = thread;

  /* We can only step over breakpoints we know about.  */
  if (breakpoint_here (pc) || fast_tracepoint_jump_here (pc))
    {
      /* Don't step over a breakpoint that GDB expects to hit
	 though.  If the condition is being evaluated on the target's side
	 and it evaluate to false, step over this breakpoint as well.  */
      if (gdb_breakpoint_here (pc)
	  && gdb_condition_true_at_breakpoint (pc)
	  && gdb_no_commands_at_breakpoint (pc))
	{
	  if (debug_threads)
	    debug_printf ("Need step over [LWP %ld]? yes, but found"
			  " GDB breakpoint at 0x%s; skipping step over\n",
			  lwpid_of (thread), paddress (pc));

	  current_thread = saved_thread;
	  return 0;
	}
      else
	{
	  if (debug_threads)
	    debug_printf ("Need step over [LWP %ld]? yes, "
			  "found breakpoint at 0x%s\n",
			  lwpid_of (thread), paddress (pc));

	  /* We've found an lwp that needs stepping over --- return 1 so
	     that find_inferior stops looking.  */
	  current_thread = saved_thread;

	  return 1;
	}
    }

  current_thread = saved_thread;

  if (debug_threads)
    debug_printf ("Need step over [LWP %ld]? No, no breakpoint found"
		  " at 0x%s\n",
		  lwpid_of (thread), paddress (pc));

  return 0;
}

/* Start a step-over operation on LWP.  When LWP stopped at a
   breakpoint, to make progress, we need to remove the breakpoint out
   of the way.  If we let other threads run while we do that, they may
   pass by the breakpoint location and miss hitting it.  To avoid
   that, a step-over momentarily stops all threads while LWP is
   single-stepped by either hardware or software while the breakpoint
   is temporarily uninserted from the inferior.  When the single-step
   finishes, we reinsert the breakpoint, and let all threads that are
   supposed to be running, run again.  */

static int
start_step_over (struct lwp_info *lwp)
{
  struct thread_info *thread = get_lwp_thread (lwp);
  struct thread_info *saved_thread;
  CORE_ADDR pc;
  int step;

  if (debug_threads)
    debug_printf ("Starting step-over on LWP %ld.  Stopping all threads\n",
		  lwpid_of (thread));

  stop_all_lwps (1, lwp);

  if (lwp->suspended != 0)
    {
      internal_error (__FILE__, __LINE__,
		      "LWP %ld suspended=%d\n", lwpid_of (thread),
		      lwp->suspended);
    }

  if (debug_threads)
    debug_printf ("Done stopping all threads for step-over.\n");

  /* Note, we should always reach here with an already adjusted PC,
     either by GDB (if we're resuming due to GDB's request), or by our
     caller, if we just finished handling an internal breakpoint GDB
     shouldn't care about.  */
  pc = get_pc (lwp);

  saved_thread = current_thread;
  current_thread = thread;

  lwp->bp_reinsert = pc;
  uninsert_breakpoints_at (pc);
  uninsert_fast_tracepoint_jumps_at (pc);

  step = single_step (lwp);

  current_thread = saved_thread;

  linux_resume_one_lwp (lwp, step, 0, NULL);

  /* Require next event from this LWP.  */
  step_over_bkpt = thread->entry.id;
  return 1;
}

/* Finish a step-over.  Reinsert the breakpoint we had uninserted in
   start_step_over, if still there, and delete any reinsert
   breakpoints we've set, on non hardware single-step targets.  */

static int
finish_step_over (struct lwp_info *lwp)
{
  if (lwp->bp_reinsert != 0)
    {
      struct thread_info *saved_thread = current_thread;

      if (debug_threads)
	debug_printf ("Finished step over.\n");

      current_thread = get_lwp_thread (lwp);

      /* Reinsert any breakpoint at LWP->BP_REINSERT.  Note that there
	 may be no breakpoint to reinsert there by now.  */
      reinsert_breakpoints_at (lwp->bp_reinsert);
      reinsert_fast_tracepoint_jumps_at (lwp->bp_reinsert);

      lwp->bp_reinsert = 0;

      /* Delete any software-single-step reinsert breakpoints.  No
	 longer needed.  We don't have to worry about other threads
	 hitting this trap, and later not being able to explain it,
	 because we were stepping over a breakpoint, and we hold all
	 threads but LWP stopped while doing that.  */
      if (!can_hardware_single_step ())
	{
	  gdb_assert (has_reinsert_breakpoints (current_thread));
	  delete_reinsert_breakpoints (current_thread);
	}

      step_over_bkpt = null_ptid;
      current_thread = saved_thread;
      return 1;
    }
  else
    return 0;
}

/* If there's a step over in progress, wait until all threads stop
   (that is, until the stepping thread finishes its step), and
   unsuspend all lwps.  The stepping thread ends with its status
   pending, which is processed later when we get back to processing
   events.  */

static void
complete_ongoing_step_over (void)
{
  if (!ptid_equal (step_over_bkpt, null_ptid))
    {
      struct lwp_info *lwp;
      int wstat;
      int ret;

      if (debug_threads)
	debug_printf ("detach: step over in progress, finish it first\n");

      /* Passing NULL_PTID as filter indicates we want all events to
	 be left pending.  Eventually this returns when there are no
	 unwaited-for children left.  */
      ret = linux_wait_for_event_filtered (minus_one_ptid, null_ptid,
					   &wstat, __WALL);
      gdb_assert (ret == -1);

      lwp = find_lwp_pid (step_over_bkpt);
      if (lwp != NULL)
	finish_step_over (lwp);
      step_over_bkpt = null_ptid;
      unsuspend_all_lwps (lwp);
    }
}

/* This function is called once per thread.  We check the thread's resume
   request, which will tell us whether to resume, step, or leave the thread
   stopped; and what signal, if any, it should be sent.

   For threads which we aren't explicitly told otherwise, we preserve
   the stepping flag; this is used for stepping over gdbserver-placed
   breakpoints.

   If pending_flags was set in any thread, we queue any needed
   signals, since we won't actually resume.  We already have a pending
   event to report, so we don't need to preserve any step requests;
   they should be re-issued if necessary.  */

static int
linux_resume_one_thread (struct inferior_list_entry *entry, void *arg)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);
  int leave_all_stopped = * (int *) arg;
  int leave_pending;

  if (lwp->resume == NULL)
    return 0;

  if (lwp->resume->kind == resume_stop)
    {
      if (debug_threads)
	debug_printf ("resume_stop request for LWP %ld\n", lwpid_of (thread));

      if (!lwp->stopped)
	{
	  if (debug_threads)
	    debug_printf ("stopping LWP %ld\n", lwpid_of (thread));

	  /* Stop the thread, and wait for the event asynchronously,
	     through the event loop.  */
	  send_sigstop (lwp);
	}
      else
	{
	  if (debug_threads)
	    debug_printf ("already stopped LWP %ld\n",
			  lwpid_of (thread));

	  /* The LWP may have been stopped in an internal event that
	     was not meant to be notified back to GDB (e.g., gdbserver
	     breakpoint), so we should be reporting a stop event in
	     this case too.  */

	  /* If the thread already has a pending SIGSTOP, this is a
	     no-op.  Otherwise, something later will presumably resume
	     the thread and this will cause it to cancel any pending
	     operation, due to last_resume_kind == resume_stop.  If
	     the thread already has a pending status to report, we
	     will still report it the next time we wait - see
	     status_pending_p_callback.  */

	  /* If we already have a pending signal to report, then
	     there's no need to queue a SIGSTOP, as this means we're
	     midway through moving the LWP out of the jumppad, and we
	     will report the pending signal as soon as that is
	     finished.  */
	  if (lwp->pending_signals_to_report == NULL)
	    send_sigstop (lwp);
	}

      /* For stop requests, we're done.  */
      lwp->resume = NULL;
      thread->last_status.kind = TARGET_WAITKIND_IGNORE;
      return 0;
    }

  /* If this thread which is about to be resumed has a pending status,
     then don't resume it - we can just report the pending status.
     Likewise if it is suspended, because e.g., another thread is
     stepping past a breakpoint.  Make sure to queue any signals that
     would otherwise be sent.  In all-stop mode, we do this decision
     based on if *any* thread has a pending status.  If there's a
     thread that needs the step-over-breakpoint dance, then don't
     resume any other thread but that particular one.  */
  leave_pending = (lwp->suspended
		   || lwp->status_pending_p
		   || leave_all_stopped);

  /* If we have a new signal, enqueue the signal.  */
  if (lwp->resume->sig != 0)
    {
      siginfo_t info, *info_p;

      /* If this is the same signal we were previously stopped by,
	 make sure to queue its siginfo.  */
      if (WIFSTOPPED (lwp->last_status)
	  && WSTOPSIG (lwp->last_status) == lwp->resume->sig
	  && ptrace (PTRACE_GETSIGINFO, lwpid_of (thread),
		     (PTRACE_TYPE_ARG3) 0, &info) == 0)
	info_p = &info;
      else
	info_p = NULL;

      enqueue_pending_signal (lwp, lwp->resume->sig, info_p);
    }

  if (!leave_pending)
    {
      if (debug_threads)
	debug_printf ("resuming LWP %ld\n", lwpid_of (thread));

      proceed_one_lwp (entry, NULL);
    }
  else
    {
      if (debug_threads)
	debug_printf ("leaving LWP %ld stopped\n", lwpid_of (thread));
    }

  thread->last_status.kind = TARGET_WAITKIND_IGNORE;
  lwp->resume = NULL;
  return 0;
}

static void
linux_resume (struct thread_resume *resume_info, size_t n)
{
  struct thread_resume_array array = { resume_info, n };
  struct thread_info *need_step_over = NULL;
  int any_pending;
  int leave_all_stopped;

  if (debug_threads)
    {
      debug_enter ();
      debug_printf ("linux_resume:\n");
    }

  find_inferior (&all_threads, linux_set_resume_request, &array);

  /* If there is a thread which would otherwise be resumed, which has
     a pending status, then don't resume any threads - we can just
     report the pending status.  Make sure to queue any signals that
     would otherwise be sent.  In non-stop mode, we'll apply this
     logic to each thread individually.  We consume all pending events
     before considering to start a step-over (in all-stop).  */
  any_pending = 0;
  if (!non_stop)
    find_inferior (&all_threads, resume_status_pending_p, &any_pending);

  /* If there is a thread which would otherwise be resumed, which is
     stopped at a breakpoint that needs stepping over, then don't
     resume any threads - have it step over the breakpoint with all
     other threads stopped, then resume all threads again.  Make sure
     to queue any signals that would otherwise be delivered or
     queued.  */
  if (!any_pending && supports_breakpoints ())
    need_step_over
      = (struct thread_info *) find_inferior (&all_threads,
					      need_step_over_p, NULL);

  leave_all_stopped = (need_step_over != NULL || any_pending);

  if (debug_threads)
    {
      if (need_step_over != NULL)
	debug_printf ("Not resuming all, need step over\n");
      else if (any_pending)
	debug_printf ("Not resuming, all-stop and found "
		      "an LWP with pending status\n");
      else
	debug_printf ("Resuming, no pending status or step over needed\n");
    }

  /* Even if we're leaving threads stopped, queue all signals we'd
     otherwise deliver.  */
  find_inferior (&all_threads, linux_resume_one_thread, &leave_all_stopped);

  if (need_step_over)
    start_step_over (get_thread_lwp (need_step_over));

  if (debug_threads)
    {
      debug_printf ("linux_resume done\n");
      debug_exit ();
    }

  /* We may have events that were pending that can/should be sent to
     the client now.  Trigger a linux_wait call.  */
  if (target_is_async_p ())
    async_file_mark ();
}

/* This function is called once per thread.  We check the thread's
   last resume request, which will tell us whether to resume, step, or
   leave the thread stopped.  Any signal the client requested to be
   delivered has already been enqueued at this point.

   If any thread that GDB wants running is stopped at an internal
   breakpoint that needs stepping over, we start a step-over operation
   on that particular thread, and leave all others stopped.  */

static int
proceed_one_lwp (struct inferior_list_entry *entry, void *except)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);
  int step;

  if (lwp == except)
    return 0;

  if (debug_threads)
    debug_printf ("proceed_one_lwp: lwp %ld\n", lwpid_of (thread));

  if (!lwp->stopped)
    {
      if (debug_threads)
	debug_printf ("   LWP %ld already running\n", lwpid_of (thread));
      return 0;
    }

  if (thread->last_resume_kind == resume_stop
      && thread->last_status.kind != TARGET_WAITKIND_IGNORE)
    {
      if (debug_threads)
	debug_printf ("   client wants LWP to remain %ld stopped\n",
		      lwpid_of (thread));
      return 0;
    }

  if (lwp->status_pending_p)
    {
      if (debug_threads)
	debug_printf ("   LWP %ld has pending status, leaving stopped\n",
		      lwpid_of (thread));
      return 0;
    }

  gdb_assert (lwp->suspended >= 0);

  if (lwp->suspended)
    {
      if (debug_threads)
	debug_printf ("   LWP %ld is suspended\n", lwpid_of (thread));
      return 0;
    }

  if (thread->last_resume_kind == resume_stop
      && lwp->pending_signals_to_report == NULL
      && lwp->collecting_fast_tracepoint == 0)
    {
      /* We haven't reported this LWP as stopped yet (otherwise, the
	 last_status.kind check above would catch it, and we wouldn't
	 reach here.  This LWP may have been momentarily paused by a
	 stop_all_lwps call while handling for example, another LWP's
	 step-over.  In that case, the pending expected SIGSTOP signal
	 that was queued at vCont;t handling time will have already
	 been consumed by wait_for_sigstop, and so we need to requeue
	 another one here.  Note that if the LWP already has a SIGSTOP
	 pending, this is a no-op.  */

      if (debug_threads)
	debug_printf ("Client wants LWP %ld to stop. "
		      "Making sure it has a SIGSTOP pending\n",
		      lwpid_of (thread));

      send_sigstop (lwp);
    }

  if (thread->last_resume_kind == resume_step)
    {
      if (debug_threads)
	debug_printf ("   stepping LWP %ld, client wants it stepping\n",
		      lwpid_of (thread));

      /* If resume_step is requested by GDB, install reinsert
	 breakpoints when the thread is about to be actually resumed if
	 the reinsert breakpoints weren't removed.  */
      if (can_software_single_step () && !has_reinsert_breakpoints (thread))
	install_software_single_step_breakpoints (lwp);

      step = maybe_hw_step (thread);
    }
  else if (lwp->bp_reinsert != 0)
    {
      if (debug_threads)
	debug_printf ("   stepping LWP %ld, reinsert set\n",
		      lwpid_of (thread));

      step = maybe_hw_step (thread);
    }
  else
    step = 0;

  linux_resume_one_lwp (lwp, step, 0, NULL);
  return 0;
}

static int
unsuspend_and_proceed_one_lwp (struct inferior_list_entry *entry, void *except)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);

  if (lwp == except)
    return 0;

  lwp_suspended_decr (lwp);

  return proceed_one_lwp (entry, except);
}

/* When we finish a step-over, set threads running again.  If there's
   another thread that may need a step-over, now's the time to start
   it.  Eventually, we'll move all threads past their breakpoints.  */

static void
proceed_all_lwps (void)
{
  struct thread_info *need_step_over;

  /* If there is a thread which would otherwise be resumed, which is
     stopped at a breakpoint that needs stepping over, then don't
     resume any threads - have it step over the breakpoint with all
     other threads stopped, then resume all threads again.  */

  if (supports_breakpoints ())
    {
      need_step_over
	= (struct thread_info *) find_inferior (&all_threads,
						need_step_over_p, NULL);

      if (need_step_over != NULL)
	{
	  if (debug_threads)
	    debug_printf ("proceed_all_lwps: found "
			  "thread %ld needing a step-over\n",
			  lwpid_of (need_step_over));

	  start_step_over (get_thread_lwp (need_step_over));
	  return;
	}
    }

  if (debug_threads)
    debug_printf ("Proceeding, no step-over needed\n");

  find_inferior (&all_threads, proceed_one_lwp, NULL);
}

/* Stopped LWPs that the client wanted to be running, that don't have
   pending statuses, are set to run again, except for EXCEPT, if not
   NULL.  This undoes a stop_all_lwps call.  */

static void
unstop_all_lwps (int unsuspend, struct lwp_info *except)
{
  if (debug_threads)
    {
      debug_enter ();
      if (except)
	debug_printf ("unstopping all lwps, except=(LWP %ld)\n",
		      lwpid_of (get_lwp_thread (except)));
      else
	debug_printf ("unstopping all lwps\n");
    }

  if (unsuspend)
    find_inferior (&all_threads, unsuspend_and_proceed_one_lwp, except);
  else
    find_inferior (&all_threads, proceed_one_lwp, except);

  if (debug_threads)
    {
      debug_printf ("unstop_all_lwps done\n");
      debug_exit ();
    }
}


#ifdef HAVE_LINUX_REGSETS

#define use_linux_regsets 1

/* Returns true if REGSET has been disabled.  */

static int
regset_disabled (struct regsets_info *info, struct regset_info *regset)
{
  return (info->disabled_regsets != NULL
	  && info->disabled_regsets[regset - info->regsets]);
}

/* Disable REGSET.  */

static void
disable_regset (struct regsets_info *info, struct regset_info *regset)
{
  int dr_offset;

  dr_offset = regset - info->regsets;
  if (info->disabled_regsets == NULL)
    info->disabled_regsets = (char *) xcalloc (1, info->num_regsets);
  info->disabled_regsets[dr_offset] = 1;
}

static int
regsets_fetch_inferior_registers (struct regsets_info *regsets_info,
				  struct regcache *regcache)
{
  struct regset_info *regset;
  int saw_general_regs = 0;
  int pid;
  struct iovec iov;

  pid = lwpid_of (current_thread);
  for (regset = regsets_info->regsets; regset->size >= 0; regset++)
    {
      void *buf, *data;
      int nt_type, res;

      if (regset->size == 0 || regset_disabled (regsets_info, regset))
	continue;

      buf = xmalloc (regset->size);

      nt_type = regset->nt_type;
      if (nt_type)
	{
	  iov.iov_base = buf;
	  iov.iov_len = regset->size;
	  data = (void *) &iov;
	}
      else
	data = buf;

#ifndef __sparc__
      res = ptrace (regset->get_request, pid,
		    (PTRACE_TYPE_ARG3) (long) nt_type, data);
#else
      res = ptrace (regset->get_request, pid, data, nt_type);
#endif
      if (res < 0)
	{
	  if (errno == EIO)
	    {
	      /* If we get EIO on a regset, do not try it again for
		 this process mode.  */
	      disable_regset (regsets_info, regset);
	    }
	  else if (errno == ENODATA)
	    {
	      /* ENODATA may be returned if the regset is currently
		 not "active".  This can happen in normal operation,
		 so suppress the warning in this case.  */
	    }
	  else if (errno == ESRCH)
	    {
	      /* At this point, ESRCH should mean the process is
		 already gone, in which case we simply ignore attempts
		 to read its registers.  */
	    }
	  else
	    {
	      char s[256];
	      sprintf (s, "ptrace(regsets_fetch_inferior_registers) PID=%d",
		       pid);
	      perror (s);
	    }
	}
      else
	{
	  if (regset->type == GENERAL_REGS)
	    saw_general_regs = 1;
	  regset->store_function (regcache, buf);
	}
      free (buf);
    }
  if (saw_general_regs)
    return 0;
  else
    return 1;
}

static int
regsets_store_inferior_registers (struct regsets_info *regsets_info,
				  struct regcache *regcache)
{
  struct regset_info *regset;
  int saw_general_regs = 0;
  int pid;
  struct iovec iov;

  pid = lwpid_of (current_thread);
  for (regset = regsets_info->regsets; regset->size >= 0; regset++)
    {
      void *buf, *data;
      int nt_type, res;

      if (regset->size == 0 || regset_disabled (regsets_info, regset)
	  || regset->fill_function == NULL)
	continue;

      buf = xmalloc (regset->size);

      /* First fill the buffer with the current register set contents,
	 in case there are any items in the kernel's regset that are
	 not in gdbserver's regcache.  */

      nt_type = regset->nt_type;
      if (nt_type)
	{
	  iov.iov_base = buf;
	  iov.iov_len = regset->size;
	  data = (void *) &iov;
	}
      else
	data = buf;

#ifndef __sparc__
      res = ptrace (regset->get_request, pid,
		    (PTRACE_TYPE_ARG3) (long) nt_type, data);
#else
      res = ptrace (regset->get_request, pid, data, nt_type);
#endif

      if (res == 0)
	{
	  /* Then overlay our cached registers on that.  */
	  regset->fill_function (regcache, buf);

	  /* Only now do we write the register set.  */
#ifndef __sparc__
	  res = ptrace (regset->set_request, pid,
			(PTRACE_TYPE_ARG3) (long) nt_type, data);
#else
	  res = ptrace (regset->set_request, pid, data, nt_type);
#endif
	}

      if (res < 0)
	{
	  if (errno == EIO)
	    {
	      /* If we get EIO on a regset, do not try it again for
		 this process mode.  */
	      disable_regset (regsets_info, regset);
	    }
	  else if (errno == ESRCH)
	    {
	      /* At this point, ESRCH should mean the process is
		 already gone, in which case we simply ignore attempts
		 to change its registers.  See also the related
		 comment in linux_resume_one_lwp.  */
	      free (buf);
	      return 0;
	    }
	  else
	    {
	      perror ("Warning: ptrace(regsets_store_inferior_registers)");
	    }
	}
      else if (regset->type == GENERAL_REGS)
	saw_general_regs = 1;
      free (buf);
    }
  if (saw_general_regs)
    return 0;
  else
    return 1;
}

#else /* !HAVE_LINUX_REGSETS */

#define use_linux_regsets 0
#define regsets_fetch_inferior_registers(regsets_info, regcache) 1
#define regsets_store_inferior_registers(regsets_info, regcache) 1

#endif

/* Return 1 if register REGNO is supported by one of the regset ptrace
   calls or 0 if it has to be transferred individually.  */

static int
linux_register_in_regsets (const struct regs_info *regs_info, int regno)
{
  unsigned char mask = 1 << (regno % 8);
  size_t index = regno / 8;

  return (use_linux_regsets
	  && (regs_info->regset_bitmap == NULL
	      || (regs_info->regset_bitmap[index] & mask) != 0));
}

#ifdef HAVE_LINUX_USRREGS

static int
register_addr (const struct usrregs_info *usrregs, int regnum)
{
  int addr;

  if (regnum < 0 || regnum >= usrregs->num_regs)
    error ("Invalid register number %d.", regnum);

  addr = usrregs->regmap[regnum];

  return addr;
}

/* Fetch one register.  */
static void
fetch_register (const struct usrregs_info *usrregs,
		struct regcache *regcache, int regno)
{
  CORE_ADDR regaddr;
  int i, size;
  char *buf;
  int pid;

  if (regno >= usrregs->num_regs)
    return;
  if ((*the_low_target.cannot_fetch_register) (regno))
    return;

  regaddr = register_addr (usrregs, regno);
  if (regaddr == -1)
    return;

  size = ((register_size (regcache->tdesc, regno)
	   + sizeof (PTRACE_XFER_TYPE) - 1)
	  & -sizeof (PTRACE_XFER_TYPE));
  buf = (char *) alloca (size);

  pid = lwpid_of (current_thread);
  for (i = 0; i < size; i += sizeof (PTRACE_XFER_TYPE))
    {
      errno = 0;
      *(PTRACE_XFER_TYPE *) (buf + i) =
	ptrace (PTRACE_PEEKUSER, pid,
		/* Coerce to a uintptr_t first to avoid potential gcc warning
		   of coercing an 8 byte integer to a 4 byte pointer.  */
		(PTRACE_TYPE_ARG3) (uintptr_t) regaddr, (PTRACE_TYPE_ARG4) 0);
      regaddr += sizeof (PTRACE_XFER_TYPE);
      if (errno != 0)
	error ("reading register %d: %s", regno, strerror (errno));
    }

  if (the_low_target.supply_ptrace_register)
    the_low_target.supply_ptrace_register (regcache, regno, buf);
  else
    supply_register (regcache, regno, buf);
}

/* Store one register.  */
static void
store_register (const struct usrregs_info *usrregs,
		struct regcache *regcache, int regno)
{
  CORE_ADDR regaddr;
  int i, size;
  char *buf;
  int pid;

  if (regno >= usrregs->num_regs)
    return;
  if ((*the_low_target.cannot_store_register) (regno))
    return;

  regaddr = register_addr (usrregs, regno);
  if (regaddr == -1)
    return;

  size = ((register_size (regcache->tdesc, regno)
	   + sizeof (PTRACE_XFER_TYPE) - 1)
	  & -sizeof (PTRACE_XFER_TYPE));
  buf = (char *) alloca (size);
  memset (buf, 0, size);

  if (the_low_target.collect_ptrace_register)
    the_low_target.collect_ptrace_register (regcache, regno, buf);
  else
    collect_register (regcache, regno, buf);

  pid = lwpid_of (current_thread);
  for (i = 0; i < size; i += sizeof (PTRACE_XFER_TYPE))
    {
      errno = 0;
      ptrace (PTRACE_POKEUSER, pid,
	    /* Coerce to a uintptr_t first to avoid potential gcc warning
	       about coercing an 8 byte integer to a 4 byte pointer.  */
	      (PTRACE_TYPE_ARG3) (uintptr_t) regaddr,
	      (PTRACE_TYPE_ARG4) *(PTRACE_XFER_TYPE *) (buf + i));
      if (errno != 0)
	{
	  /* At this point, ESRCH should mean the process is
	     already gone, in which case we simply ignore attempts
	     to change its registers.  See also the related
	     comment in linux_resume_one_lwp.  */
	  if (errno == ESRCH)
	    return;

	  if ((*the_low_target.cannot_store_register) (regno) == 0)
	    error ("writing register %d: %s", regno, strerror (errno));
	}
      regaddr += sizeof (PTRACE_XFER_TYPE);
    }
}

/* Fetch all registers, or just one, from the child process.
   If REGNO is -1, do this for all registers, skipping any that are
   assumed to have been retrieved by regsets_fetch_inferior_registers,
   unless ALL is non-zero.
   Otherwise, REGNO specifies which register (so we can save time).  */
static void
usr_fetch_inferior_registers (const struct regs_info *regs_info,
			      struct regcache *regcache, int regno, int all)
{
  struct usrregs_info *usr = regs_info->usrregs;

  if (regno == -1)
    {
      for (regno = 0; regno < usr->num_regs; regno++)
	if (all || !linux_register_in_regsets (regs_info, regno))
	  fetch_register (usr, regcache, regno);
    }
  else
    fetch_register (usr, regcache, regno);
}

/* Store our register values back into the inferior.
   If REGNO is -1, do this for all registers, skipping any that are
   assumed to have been saved by regsets_store_inferior_registers,
   unless ALL is non-zero.
   Otherwise, REGNO specifies which register (so we can save time).  */
static void
usr_store_inferior_registers (const struct regs_info *regs_info,
			      struct regcache *regcache, int regno, int all)
{
  struct usrregs_info *usr = regs_info->usrregs;

  if (regno == -1)
    {
      for (regno = 0; regno < usr->num_regs; regno++)
	if (all || !linux_register_in_regsets (regs_info, regno))
	  store_register (usr, regcache, regno);
    }
  else
    store_register (usr, regcache, regno);
}

#else /* !HAVE_LINUX_USRREGS */

#define usr_fetch_inferior_registers(regs_info, regcache, regno, all) do {} while (0)
#define usr_store_inferior_registers(regs_info, regcache, regno, all) do {} while (0)

#endif


static void
linux_fetch_registers (struct regcache *regcache, int regno)
{
  int use_regsets;
  int all = 0;
  const struct regs_info *regs_info = (*the_low_target.regs_info) ();

  if (regno == -1)
    {
      if (the_low_target.fetch_register != NULL
	  && regs_info->usrregs != NULL)
	for (regno = 0; regno < regs_info->usrregs->num_regs; regno++)
	  (*the_low_target.fetch_register) (regcache, regno);

      all = regsets_fetch_inferior_registers (regs_info->regsets_info, regcache);
      if (regs_info->usrregs != NULL)
	usr_fetch_inferior_registers (regs_info, regcache, -1, all);
    }
  else
    {
      if (the_low_target.fetch_register != NULL
	  && (*the_low_target.fetch_register) (regcache, regno))
	return;

      use_regsets = linux_register_in_regsets (regs_info, regno);
      if (use_regsets)
	all = regsets_fetch_inferior_registers (regs_info->regsets_info,
						regcache);
      if ((!use_regsets || all) && regs_info->usrregs != NULL)
	usr_fetch_inferior_registers (regs_info, regcache, regno, 1);
    }
}

static void
linux_store_registers (struct regcache *regcache, int regno)
{
  int use_regsets;
  int all = 0;
  const struct regs_info *regs_info = (*the_low_target.regs_info) ();

  if (regno == -1)
    {
      all = regsets_store_inferior_registers (regs_info->regsets_info,
					      regcache);
      if (regs_info->usrregs != NULL)
	usr_store_inferior_registers (regs_info, regcache, regno, all);
    }
  else
    {
      use_regsets = linux_register_in_regsets (regs_info, regno);
      if (use_regsets)
	all = regsets_store_inferior_registers (regs_info->regsets_info,
						regcache);
      if ((!use_regsets || all) && regs_info->usrregs != NULL)
	usr_store_inferior_registers (regs_info, regcache, regno, 1);
    }
}


/* Copy LEN bytes from inferior's memory starting at MEMADDR
   to debugger memory starting at MYADDR.  */

static int
linux_read_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
{
  int pid = lwpid_of (current_thread);
  register PTRACE_XFER_TYPE *buffer;
  register CORE_ADDR addr;
  register int count;
  char filename[64];
  register int i;
  int ret;
  int fd;

  /* Try using /proc.  Don't bother for one word.  */
  if (len >= 3 * sizeof (long))
    {
      int bytes;

      /* We could keep this file open and cache it - possibly one per
	 thread.  That requires some juggling, but is even faster.  */
      sprintf (filename, "/proc/%d/mem", pid);
      fd = open (filename, O_RDONLY | O_LARGEFILE);
      if (fd == -1)
	goto no_proc;

      /* If pread64 is available, use it.  It's faster if the kernel
	 supports it (only one syscall), and it's 64-bit safe even on
	 32-bit platforms (for instance, SPARC debugging a SPARC64
	 application).  */
#ifdef HAVE_PREAD64
      bytes = pread64 (fd, myaddr, len, memaddr);
#else
      bytes = -1;
      if (lseek (fd, memaddr, SEEK_SET) != -1)
	bytes = read (fd, myaddr, len);
#endif

      close (fd);
      if (bytes == len)
	return 0;

      /* Some data was read, we'll try to get the rest with ptrace.  */
      if (bytes > 0)
	{
	  memaddr += bytes;
	  myaddr += bytes;
	  len -= bytes;
	}
    }

 no_proc:
  /* Round starting address down to longword boundary.  */
  addr = memaddr & -(CORE_ADDR) sizeof (PTRACE_XFER_TYPE);
  /* Round ending address up; get number of longwords that makes.  */
  count = ((((memaddr + len) - addr) + sizeof (PTRACE_XFER_TYPE) - 1)
	   / sizeof (PTRACE_XFER_TYPE));
  /* Allocate buffer of that many longwords.  */
  buffer = XALLOCAVEC (PTRACE_XFER_TYPE, count);

  /* Read all the longwords */
  errno = 0;
  for (i = 0; i < count; i++, addr += sizeof (PTRACE_XFER_TYPE))
    {
      /* Coerce the 3rd arg to a uintptr_t first to avoid potential gcc warning
	 about coercing an 8 byte integer to a 4 byte pointer.  */
      buffer[i] = ptrace (PTRACE_PEEKTEXT, pid,
			  (PTRACE_TYPE_ARG3) (uintptr_t) addr,
			  (PTRACE_TYPE_ARG4) 0);
      if (errno)
	break;
    }
  ret = errno;

  /* Copy appropriate bytes out of the buffer.  */
  if (i > 0)
    {
      i *= sizeof (PTRACE_XFER_TYPE);
      i -= memaddr & (sizeof (PTRACE_XFER_TYPE) - 1);
      memcpy (myaddr,
	      (char *) buffer + (memaddr & (sizeof (PTRACE_XFER_TYPE) - 1)),
	      i < len ? i : len);
    }

  return ret;
}

/* Copy LEN bytes of data from debugger memory at MYADDR to inferior's
   memory at MEMADDR.  On failure (cannot write to the inferior)
   returns the value of errno.  Always succeeds if LEN is zero.  */

static int
linux_write_memory (CORE_ADDR memaddr, const unsigned char *myaddr, int len)
{
  register int i;
  /* Round starting address down to longword boundary.  */
  register CORE_ADDR addr = memaddr & -(CORE_ADDR) sizeof (PTRACE_XFER_TYPE);
  /* Round ending address up; get number of longwords that makes.  */
  register int count
    = (((memaddr + len) - addr) + sizeof (PTRACE_XFER_TYPE) - 1)
    / sizeof (PTRACE_XFER_TYPE);

  /* Allocate buffer of that many longwords.  */
  register PTRACE_XFER_TYPE *buffer = XALLOCAVEC (PTRACE_XFER_TYPE, count);

  int pid = lwpid_of (current_thread);

  if (len == 0)
    {
      /* Zero length write always succeeds.  */
      return 0;
    }

  if (debug_threads)
    {
      /* Dump up to four bytes.  */
      char str[4 * 2 + 1];
      char *p = str;
      int dump = len < 4 ? len : 4;

      for (i = 0; i < dump; i++)
	{
	  sprintf (p, "%02x", myaddr[i]);
	  p += 2;
	}
      *p = '\0';

      debug_printf ("Writing %s to 0x%08lx in process %d\n",
		    str, (long) memaddr, pid);
    }

  /* Fill start and end extra bytes of buffer with existing memory data.  */

  errno = 0;
  /* Coerce the 3rd arg to a uintptr_t first to avoid potential gcc warning
     about coercing an 8 byte integer to a 4 byte pointer.  */
  buffer[0] = ptrace (PTRACE_PEEKTEXT, pid,
		      (PTRACE_TYPE_ARG3) (uintptr_t) addr,
		      (PTRACE_TYPE_ARG4) 0);
  if (errno)
    return errno;

  if (count > 1)
    {
      errno = 0;
      buffer[count - 1]
	= ptrace (PTRACE_PEEKTEXT, pid,
		  /* Coerce to a uintptr_t first to avoid potential gcc warning
		     about coercing an 8 byte integer to a 4 byte pointer.  */
		  (PTRACE_TYPE_ARG3) (uintptr_t) (addr + (count - 1)
						  * sizeof (PTRACE_XFER_TYPE)),
		  (PTRACE_TYPE_ARG4) 0);
      if (errno)
	return errno;
    }

  /* Copy data to be written over corresponding part of buffer.  */

  memcpy ((char *) buffer + (memaddr & (sizeof (PTRACE_XFER_TYPE) - 1)),
	  myaddr, len);

  /* Write the entire buffer.  */

  for (i = 0; i < count; i++, addr += sizeof (PTRACE_XFER_TYPE))
    {
      errno = 0;
      ptrace (PTRACE_POKETEXT, pid,
	      /* Coerce to a uintptr_t first to avoid potential gcc warning
		 about coercing an 8 byte integer to a 4 byte pointer.  */
	      (PTRACE_TYPE_ARG3) (uintptr_t) addr,
	      (PTRACE_TYPE_ARG4) buffer[i]);
      if (errno)
	return errno;
    }

  return 0;
}

static void
linux_look_up_symbols (void)
{
#ifdef USE_THREAD_DB
  struct process_info *proc = current_process ();

  if (proc->priv->thread_db != NULL)
    return;

  thread_db_init ();
#endif
}

static void
linux_request_interrupt (void)
{
  extern unsigned long signal_pid;

  /* Send a SIGINT to the process group.  This acts just like the user
     typed a ^C on the controlling terminal.  */
  kill (-signal_pid, SIGINT);
}

/* Copy LEN bytes from inferior's auxiliary vector starting at OFFSET
   to debugger memory starting at MYADDR.  */

static int
linux_read_auxv (CORE_ADDR offset, unsigned char *myaddr, unsigned int len)
{
  char filename[PATH_MAX];
  int fd, n;
  int pid = lwpid_of (current_thread);

  xsnprintf (filename, sizeof filename, "/proc/%d/auxv", pid);

  fd = open (filename, O_RDONLY);
  if (fd < 0)
    return -1;

  if (offset != (CORE_ADDR) 0
      && lseek (fd, (off_t) offset, SEEK_SET) != (off_t) offset)
    n = -1;
  else
    n = read (fd, myaddr, len);

  close (fd);

  return n;
}

/* These breakpoint and watchpoint related wrapper functions simply
   pass on the function call if the target has registered a
   corresponding function.  */

static int
linux_supports_z_point_type (char z_type)
{
  return (the_low_target.supports_z_point_type != NULL
	  && the_low_target.supports_z_point_type (z_type));
}

static int
linux_insert_point (enum raw_bkpt_type type, CORE_ADDR addr,
		    int size, struct raw_breakpoint *bp)
{
  if (type == raw_bkpt_type_sw)
    return insert_memory_breakpoint (bp);
  else if (the_low_target.insert_point != NULL)
    return the_low_target.insert_point (type, addr, size, bp);
  else
    /* Unsupported (see target.h).  */
    return 1;
}

static int
linux_remove_point (enum raw_bkpt_type type, CORE_ADDR addr,
		    int size, struct raw_breakpoint *bp)
{
  if (type == raw_bkpt_type_sw)
    return remove_memory_breakpoint (bp);
  else if (the_low_target.remove_point != NULL)
    return the_low_target.remove_point (type, addr, size, bp);
  else
    /* Unsupported (see target.h).  */
    return 1;
}

/* Implement the to_stopped_by_sw_breakpoint target_ops
   method.  */

static int
linux_stopped_by_sw_breakpoint (void)
{
  struct lwp_info *lwp = get_thread_lwp (current_thread);

  return (lwp->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT);
}

/* Implement the to_supports_stopped_by_sw_breakpoint target_ops
   method.  */

static int
linux_supports_stopped_by_sw_breakpoint (void)
{
  return USE_SIGTRAP_SIGINFO;
}

/* Implement the to_stopped_by_hw_breakpoint target_ops
   method.  */

static int
linux_stopped_by_hw_breakpoint (void)
{
  struct lwp_info *lwp = get_thread_lwp (current_thread);

  return (lwp->stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT);
}

/* Implement the to_supports_stopped_by_hw_breakpoint target_ops
   method.  */

static int
linux_supports_stopped_by_hw_breakpoint (void)
{
  return USE_SIGTRAP_SIGINFO;
}

/* Implement the supports_hardware_single_step target_ops method.  */

static int
linux_supports_hardware_single_step (void)
{
  return can_hardware_single_step ();
}

static int
linux_supports_software_single_step (void)
{
  return can_software_single_step ();
}

static int
linux_stopped_by_watchpoint (void)
{
  struct lwp_info *lwp = get_thread_lwp (current_thread);

  return lwp->stop_reason == TARGET_STOPPED_BY_WATCHPOINT;
}

static CORE_ADDR
linux_stopped_data_address (void)
{
  struct lwp_info *lwp = get_thread_lwp (current_thread);

  return lwp->stopped_data_address;
}

#if defined(__UCLIBC__) && defined(HAS_NOMMU)	      \
    && defined(PT_TEXT_ADDR) && defined(PT_DATA_ADDR) \
    && defined(PT_TEXT_END_ADDR)

/* This is only used for targets that define PT_TEXT_ADDR,
   PT_DATA_ADDR and PT_TEXT_END_ADDR.  If those are not defined, supposedly
   the target has different ways of acquiring this information, like
   loadmaps.  */

/* Under uClinux, programs are loaded at non-zero offsets, which we need
   to tell gdb about.  */

static int
linux_read_offsets (CORE_ADDR *text_p, CORE_ADDR *data_p)
{
  unsigned long text, text_end, data;
  int pid = lwpid_of (current_thread);

  errno = 0;

  text = ptrace (PTRACE_PEEKUSER, pid, (PTRACE_TYPE_ARG3) PT_TEXT_ADDR,
		 (PTRACE_TYPE_ARG4) 0);
  text_end = ptrace (PTRACE_PEEKUSER, pid, (PTRACE_TYPE_ARG3) PT_TEXT_END_ADDR,
		     (PTRACE_TYPE_ARG4) 0);
  data = ptrace (PTRACE_PEEKUSER, pid, (PTRACE_TYPE_ARG3) PT_DATA_ADDR,
		 (PTRACE_TYPE_ARG4) 0);

  if (errno == 0)
    {
      /* Both text and data offsets produced at compile-time (and so
	 used by gdb) are relative to the beginning of the program,
	 with the data segment immediately following the text segment.
	 However, the actual runtime layout in memory may put the data
	 somewhere else, so when we send gdb a data base-address, we
	 use the real data base address and subtract the compile-time
	 data base-address from it (which is just the length of the
	 text segment).  BSS immediately follows data in both
	 cases.  */
      *text_p = text;
      *data_p = data - (text_end - text);

      return 1;
    }
 return 0;
}
#endif

static int
linux_qxfer_osdata (const char *annex,
		    unsigned char *readbuf, unsigned const char *writebuf,
		    CORE_ADDR offset, int len)
{
  return linux_common_xfer_osdata (annex, readbuf, offset, len);
}

/* Convert a native/host siginfo object, into/from the siginfo in the
   layout of the inferiors' architecture.  */

static void
siginfo_fixup (siginfo_t *siginfo, gdb_byte *inf_siginfo, int direction)
{
  int done = 0;

  if (the_low_target.siginfo_fixup != NULL)
    done = the_low_target.siginfo_fixup (siginfo, inf_siginfo, direction);

  /* If there was no callback, or the callback didn't do anything,
     then just do a straight memcpy.  */
  if (!done)
    {
      if (direction == 1)
	memcpy (siginfo, inf_siginfo, sizeof (siginfo_t));
      else
	memcpy (inf_siginfo, siginfo, sizeof (siginfo_t));
    }
}

static int
linux_xfer_siginfo (const char *annex, unsigned char *readbuf,
		    unsigned const char *writebuf, CORE_ADDR offset, int len)
{
  int pid;
  siginfo_t siginfo;
  gdb_byte inf_siginfo[sizeof (siginfo_t)];

  if (current_thread == NULL)
    return -1;

  pid = lwpid_of (current_thread);

  if (debug_threads)
    debug_printf ("%s siginfo for lwp %d.\n",
		  readbuf != NULL ? "Reading" : "Writing",
		  pid);

  if (offset >= sizeof (siginfo))
    return -1;

  if (ptrace (PTRACE_GETSIGINFO, pid, (PTRACE_TYPE_ARG3) 0, &siginfo) != 0)
    return -1;

  /* When GDBSERVER is built as a 64-bit application, ptrace writes into
     SIGINFO an object with 64-bit layout.  Since debugging a 32-bit
     inferior with a 64-bit GDBSERVER should look the same as debugging it
     with a 32-bit GDBSERVER, we need to convert it.  */
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

      if (ptrace (PTRACE_SETSIGINFO, pid, (PTRACE_TYPE_ARG3) 0, &siginfo) != 0)
	return -1;
    }

  return len;
}

/* SIGCHLD handler that serves two purposes: In non-stop/async mode,
   so we notice when children change state; as the handler for the
   sigsuspend in my_waitpid.  */

static void
sigchld_handler (int signo)
{
  int old_errno = errno;

  if (debug_threads)
    {
      do
	{
	  /* fprintf is not async-signal-safe, so call write
	     directly.  */
	  if (write (2, "sigchld_handler\n",
		     sizeof ("sigchld_handler\n") - 1) < 0)
	    break; /* just ignore */
	} while (0);
    }

  if (target_is_async_p ())
    async_file_mark (); /* trigger a linux_wait */

  errno = old_errno;
}

static int
linux_supports_non_stop (void)
{
  return 1;
}

static int
linux_async (int enable)
{
  int previous = target_is_async_p ();

  if (debug_threads)
    debug_printf ("linux_async (%d), previous=%d\n",
		  enable, previous);

  if (previous != enable)
    {
      sigset_t mask;
      sigemptyset (&mask);
      sigaddset (&mask, SIGCHLD);

      sigprocmask (SIG_BLOCK, &mask, NULL);

      if (enable)
	{
	  if (pipe (linux_event_pipe) == -1)
	    {
	      linux_event_pipe[0] = -1;
	      linux_event_pipe[1] = -1;
	      sigprocmask (SIG_UNBLOCK, &mask, NULL);

	      warning ("creating event pipe failed.");
	      return previous;
	    }

	  fcntl (linux_event_pipe[0], F_SETFL, O_NONBLOCK);
	  fcntl (linux_event_pipe[1], F_SETFL, O_NONBLOCK);

	  /* Register the event loop handler.  */
	  add_file_handler (linux_event_pipe[0],
			    handle_target_event, NULL);

	  /* Always trigger a linux_wait.  */
	  async_file_mark ();
	}
      else
	{
	  delete_file_handler (linux_event_pipe[0]);

	  close (linux_event_pipe[0]);
	  close (linux_event_pipe[1]);
	  linux_event_pipe[0] = -1;
	  linux_event_pipe[1] = -1;
	}

      sigprocmask (SIG_UNBLOCK, &mask, NULL);
    }

  return previous;
}

static int
linux_start_non_stop (int nonstop)
{
  /* Register or unregister from event-loop accordingly.  */
  linux_async (nonstop);

  if (target_is_async_p () != (nonstop != 0))
    return -1;

  return 0;
}

static int
linux_supports_multi_process (void)
{
  return 1;
}

/* Check if fork events are supported.  */

static int
linux_supports_fork_events (void)
{
  return linux_supports_tracefork ();
}

/* Check if vfork events are supported.  */

static int
linux_supports_vfork_events (void)
{
  return linux_supports_tracefork ();
}

/* Check if exec events are supported.  */

static int
linux_supports_exec_events (void)
{
  return linux_supports_traceexec ();
}

/* Callback for 'find_inferior'.  Set the (possibly changed) ptrace
   options for the specified lwp.  */

static int
reset_lwp_ptrace_options_callback (struct inferior_list_entry *entry,
				   void *args)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct lwp_info *lwp = get_thread_lwp (thread);

  if (!lwp->stopped)
    {
      /* Stop the lwp so we can modify its ptrace options.  */
      lwp->must_set_ptrace_flags = 1;
      linux_stop_lwp (lwp);
    }
  else
    {
      /* Already stopped; go ahead and set the ptrace options.  */
      struct process_info *proc = find_process_pid (pid_of (thread));
      int options = linux_low_ptrace_options (proc->attached);

      linux_enable_event_reporting (lwpid_of (thread), options);
      lwp->must_set_ptrace_flags = 0;
    }

  return 0;
}

/* Target hook for 'handle_new_gdb_connection'.  Causes a reset of the
   ptrace flags for all inferiors.  This is in case the new GDB connection
   doesn't support the same set of events that the previous one did.  */

static void
linux_handle_new_gdb_connection (void)
{
  pid_t pid;

  /* Request that all the lwps reset their ptrace options.  */
  find_inferior (&all_threads, reset_lwp_ptrace_options_callback , &pid);
}

static int
linux_supports_disable_randomization (void)
{
#ifdef HAVE_PERSONALITY
  return 1;
#else
  return 0;
#endif
}

static int
linux_supports_agent (void)
{
  return 1;
}

static int
linux_supports_range_stepping (void)
{
  if (*the_low_target.supports_range_stepping == NULL)
    return 0;

  return (*the_low_target.supports_range_stepping) ();
}

/* Enumerate spufs IDs for process PID.  */
static int
spu_enumerate_spu_ids (long pid, unsigned char *buf, CORE_ADDR offset, int len)
{
  int pos = 0;
  int written = 0;
  char path[128];
  DIR *dir;
  struct dirent *entry;

  sprintf (path, "/proc/%ld/fd", pid);
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

      sprintf (path, "/proc/%ld/fd/%d", pid, fd);
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
          *(unsigned int *)(buf + pos - offset) = fd;
          written += 4;
        }
      pos += 4;
    }

  closedir (dir);
  return written;
}

/* Implements the to_xfer_partial interface for the TARGET_OBJECT_SPU
   object type, using the /proc file system.  */
static int
linux_qxfer_spu (const char *annex, unsigned char *readbuf,
		 unsigned const char *writebuf,
		 CORE_ADDR offset, int len)
{
  long pid = lwpid_of (current_thread);
  char buf[128];
  int fd = 0;
  int ret = 0;

  if (!writebuf && !readbuf)
    return -1;

  if (!*annex)
    {
      if (!readbuf)
	return -1;
      else
	return spu_enumerate_spu_ids (pid, readbuf, offset, len);
    }

  sprintf (buf, "/proc/%ld/fd/%s", pid, annex);
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
  else
    ret = read (fd, readbuf, (size_t) len);

  close (fd);
  return ret;
}

#if defined PT_GETDSBT || defined PTRACE_GETFDPIC
struct target_loadseg
{
  /* Core address to which the segment is mapped.  */
  Elf32_Addr addr;
  /* VMA recorded in the program header.  */
  Elf32_Addr p_vaddr;
  /* Size of this segment in memory.  */
  Elf32_Word p_memsz;
};

# if defined PT_GETDSBT
struct target_loadmap
{
  /* Protocol version number, must be zero.  */
  Elf32_Word version;
  /* Pointer to the DSBT table, its size, and the DSBT index.  */
  unsigned *dsbt_table;
  unsigned dsbt_size, dsbt_index;
  /* Number of segments in this map.  */
  Elf32_Word nsegs;
  /* The actual memory map.  */
  struct target_loadseg segs[/*nsegs*/];
};
#  define LINUX_LOADMAP		PT_GETDSBT
#  define LINUX_LOADMAP_EXEC	PTRACE_GETDSBT_EXEC
#  define LINUX_LOADMAP_INTERP	PTRACE_GETDSBT_INTERP
# else
struct target_loadmap
{
  /* Protocol version number, must be zero.  */
  Elf32_Half version;
  /* Number of segments in this map.  */
  Elf32_Half nsegs;
  /* The actual memory map.  */
  struct target_loadseg segs[/*nsegs*/];
};
#  define LINUX_LOADMAP		PTRACE_GETFDPIC
#  define LINUX_LOADMAP_EXEC	PTRACE_GETFDPIC_EXEC
#  define LINUX_LOADMAP_INTERP	PTRACE_GETFDPIC_INTERP
# endif

static int
linux_read_loadmap (const char *annex, CORE_ADDR offset,
		    unsigned char *myaddr, unsigned int len)
{
  int pid = lwpid_of (current_thread);
  int addr = -1;
  struct target_loadmap *data = NULL;
  unsigned int actual_length, copy_length;

  if (strcmp (annex, "exec") == 0)
    addr = (int) LINUX_LOADMAP_EXEC;
  else if (strcmp (annex, "interp") == 0)
    addr = (int) LINUX_LOADMAP_INTERP;
  else
    return -1;

  if (ptrace (LINUX_LOADMAP, pid, addr, &data) != 0)
    return -1;

  if (data == NULL)
    return -1;

  actual_length = sizeof (struct target_loadmap)
    + sizeof (struct target_loadseg) * data->nsegs;

  if (offset < 0 || offset > actual_length)
    return -1;

  copy_length = actual_length - offset < len ? actual_length - offset : len;
  memcpy (myaddr, (char *) data + offset, copy_length);
  return copy_length;
}
#else
# define linux_read_loadmap NULL
#endif /* defined PT_GETDSBT || defined PTRACE_GETFDPIC */

static void
linux_process_qsupported (char **features, int count)
{
  if (the_low_target.process_qsupported != NULL)
    the_low_target.process_qsupported (features, count);
}

static int
linux_supports_catch_syscall (void)
{
  return (the_low_target.get_syscall_trapinfo != NULL
	  && linux_supports_tracesysgood ());
}

static int
linux_get_ipa_tdesc_idx (void)
{
  if (the_low_target.get_ipa_tdesc_idx == NULL)
    return 0;

  return (*the_low_target.get_ipa_tdesc_idx) ();
}

static int
linux_supports_tracepoints (void)
{
  if (*the_low_target.supports_tracepoints == NULL)
    return 0;

  return (*the_low_target.supports_tracepoints) ();
}

static CORE_ADDR
linux_read_pc (struct regcache *regcache)
{
  if (the_low_target.get_pc == NULL)
    return 0;

  return (*the_low_target.get_pc) (regcache);
}

static void
linux_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  gdb_assert (the_low_target.set_pc != NULL);

  (*the_low_target.set_pc) (regcache, pc);
}

static int
linux_thread_stopped (struct thread_info *thread)
{
  return get_thread_lwp (thread)->stopped;
}

/* This exposes stop-all-threads functionality to other modules.  */

static void
linux_pause_all (int freeze)
{
  stop_all_lwps (freeze, NULL);
}

/* This exposes unstop-all-threads functionality to other gdbserver
   modules.  */

static void
linux_unpause_all (int unfreeze)
{
  unstop_all_lwps (unfreeze, NULL);
}

static int
linux_prepare_to_access_memory (void)
{
  /* Neither ptrace nor /proc/PID/mem allow accessing memory through a
     running LWP.  */
  if (non_stop)
    linux_pause_all (1);
  return 0;
}

static void
linux_done_accessing_memory (void)
{
  /* Neither ptrace nor /proc/PID/mem allow accessing memory through a
     running LWP.  */
  if (non_stop)
    linux_unpause_all (1);
}

static int
linux_install_fast_tracepoint_jump_pad (CORE_ADDR tpoint, CORE_ADDR tpaddr,
					CORE_ADDR collector,
					CORE_ADDR lockaddr,
					ULONGEST orig_size,
					CORE_ADDR *jump_entry,
					CORE_ADDR *trampoline,
					ULONGEST *trampoline_size,
					unsigned char *jjump_pad_insn,
					ULONGEST *jjump_pad_insn_size,
					CORE_ADDR *adjusted_insn_addr,
					CORE_ADDR *adjusted_insn_addr_end,
					char *err)
{
  return (*the_low_target.install_fast_tracepoint_jump_pad)
    (tpoint, tpaddr, collector, lockaddr, orig_size,
     jump_entry, trampoline, trampoline_size,
     jjump_pad_insn, jjump_pad_insn_size,
     adjusted_insn_addr, adjusted_insn_addr_end,
     err);
}

static struct emit_ops *
linux_emit_ops (void)
{
  if (the_low_target.emit_ops != NULL)
    return (*the_low_target.emit_ops) ();
  else
    return NULL;
}

static int
linux_get_min_fast_tracepoint_insn_len (void)
{
  return (*the_low_target.get_min_fast_tracepoint_insn_len) ();
}

/* Extract &phdr and num_phdr in the inferior.  Return 0 on success.  */

static int
get_phdr_phnum_from_proc_auxv (const int pid, const int is_elf64,
			       CORE_ADDR *phdr_memaddr, int *num_phdr)
{
  char filename[PATH_MAX];
  int fd;
  const int auxv_size = is_elf64
    ? sizeof (Elf64_auxv_t) : sizeof (Elf32_auxv_t);
  char buf[sizeof (Elf64_auxv_t)];  /* The larger of the two.  */

  xsnprintf (filename, sizeof filename, "/proc/%d/auxv", pid);

  fd = open (filename, O_RDONLY);
  if (fd < 0)
    return 1;

  *phdr_memaddr = 0;
  *num_phdr = 0;
  while (read (fd, buf, auxv_size) == auxv_size
	 && (*phdr_memaddr == 0 || *num_phdr == 0))
    {
      if (is_elf64)
	{
	  Elf64_auxv_t *const aux = (Elf64_auxv_t *) buf;

	  switch (aux->a_type)
	    {
	    case AT_PHDR:
	      *phdr_memaddr = aux->a_un.a_val;
	      break;
	    case AT_PHNUM:
	      *num_phdr = aux->a_un.a_val;
	      break;
	    }
	}
      else
	{
	  Elf32_auxv_t *const aux = (Elf32_auxv_t *) buf;

	  switch (aux->a_type)
	    {
	    case AT_PHDR:
	      *phdr_memaddr = aux->a_un.a_val;
	      break;
	    case AT_PHNUM:
	      *num_phdr = aux->a_un.a_val;
	      break;
	    }
	}
    }

  close (fd);

  if (*phdr_memaddr == 0 || *num_phdr == 0)
    {
      warning ("Unexpected missing AT_PHDR and/or AT_PHNUM: "
	       "phdr_memaddr = %ld, phdr_num = %d",
	       (long) *phdr_memaddr, *num_phdr);
      return 2;
    }

  return 0;
}

/* Return &_DYNAMIC (via PT_DYNAMIC) in the inferior, or 0 if not present.  */

static CORE_ADDR
get_dynamic (const int pid, const int is_elf64)
{
  CORE_ADDR phdr_memaddr, relocation;
  int num_phdr, i;
  unsigned char *phdr_buf;
  const int phdr_size = is_elf64 ? sizeof (Elf64_Phdr) : sizeof (Elf32_Phdr);

  if (get_phdr_phnum_from_proc_auxv (pid, is_elf64, &phdr_memaddr, &num_phdr))
    return 0;

  gdb_assert (num_phdr < 100);  /* Basic sanity check.  */
  phdr_buf = (unsigned char *) alloca (num_phdr * phdr_size);

  if (linux_read_memory (phdr_memaddr, phdr_buf, num_phdr * phdr_size))
    return 0;

  /* Compute relocation: it is expected to be 0 for "regular" executables,
     non-zero for PIE ones.  */
  relocation = -1;
  for (i = 0; relocation == -1 && i < num_phdr; i++)
    if (is_elf64)
      {
	Elf64_Phdr *const p = (Elf64_Phdr *) (phdr_buf + i * phdr_size);

	if (p->p_type == PT_PHDR)
	  relocation = phdr_memaddr - p->p_vaddr;
      }
    else
      {
	Elf32_Phdr *const p = (Elf32_Phdr *) (phdr_buf + i * phdr_size);

	if (p->p_type == PT_PHDR)
	  relocation = phdr_memaddr - p->p_vaddr;
      }

  if (relocation == -1)
    {
      /* PT_PHDR is optional, but necessary for PIE in general.  Fortunately
	 any real world executables, including PIE executables, have always
	 PT_PHDR present.  PT_PHDR is not present in some shared libraries or
	 in fpc (Free Pascal 2.4) binaries but neither of those have a need for
	 or present DT_DEBUG anyway (fpc binaries are statically linked).

	 Therefore if there exists DT_DEBUG there is always also PT_PHDR.

	 GDB could find RELOCATION also from AT_ENTRY - e_entry.  */

      return 0;
    }

  for (i = 0; i < num_phdr; i++)
    {
      if (is_elf64)
	{
	  Elf64_Phdr *const p = (Elf64_Phdr *) (phdr_buf + i * phdr_size);

	  if (p->p_type == PT_DYNAMIC)
	    return p->p_vaddr + relocation;
	}
      else
	{
	  Elf32_Phdr *const p = (Elf32_Phdr *) (phdr_buf + i * phdr_size);

	  if (p->p_type == PT_DYNAMIC)
	    return p->p_vaddr + relocation;
	}
    }

  return 0;
}

/* Return &_r_debug in the inferior, or -1 if not present.  Return value
   can be 0 if the inferior does not yet have the library list initialized.
   We look for DT_MIPS_RLD_MAP first.  MIPS executables use this instead of
   DT_DEBUG, although they sometimes contain an unused DT_DEBUG entry too.  */

static CORE_ADDR
get_r_debug (const int pid, const int is_elf64)
{
  CORE_ADDR dynamic_memaddr;
  const int dyn_size = is_elf64 ? sizeof (Elf64_Dyn) : sizeof (Elf32_Dyn);
  unsigned char buf[sizeof (Elf64_Dyn)];  /* The larger of the two.  */
  CORE_ADDR map = -1;

  dynamic_memaddr = get_dynamic (pid, is_elf64);
  if (dynamic_memaddr == 0)
    return map;

  while (linux_read_memory (dynamic_memaddr, buf, dyn_size) == 0)
    {
      if (is_elf64)
	{
	  Elf64_Dyn *const dyn = (Elf64_Dyn *) buf;
#if defined DT_MIPS_RLD_MAP || defined DT_MIPS_RLD_MAP_REL
	  union
	    {
	      Elf64_Xword map;
	      unsigned char buf[sizeof (Elf64_Xword)];
	    }
	  rld_map;
#endif
#ifdef DT_MIPS_RLD_MAP
	  if (dyn->d_tag == DT_MIPS_RLD_MAP)
	    {
	      if (linux_read_memory (dyn->d_un.d_val,
				     rld_map.buf, sizeof (rld_map.buf)) == 0)
		return rld_map.map;
	      else
		break;
	    }
#endif	/* DT_MIPS_RLD_MAP */
#ifdef DT_MIPS_RLD_MAP_REL
	  if (dyn->d_tag == DT_MIPS_RLD_MAP_REL)
	    {
	      if (linux_read_memory (dyn->d_un.d_val + dynamic_memaddr,
				     rld_map.buf, sizeof (rld_map.buf)) == 0)
		return rld_map.map;
	      else
		break;
	    }
#endif	/* DT_MIPS_RLD_MAP_REL */

	  if (dyn->d_tag == DT_DEBUG && map == -1)
	    map = dyn->d_un.d_val;

	  if (dyn->d_tag == DT_NULL)
	    break;
	}
      else
	{
	  Elf32_Dyn *const dyn = (Elf32_Dyn *) buf;
#if defined DT_MIPS_RLD_MAP || defined DT_MIPS_RLD_MAP_REL
	  union
	    {
	      Elf32_Word map;
	      unsigned char buf[sizeof (Elf32_Word)];
	    }
	  rld_map;
#endif
#ifdef DT_MIPS_RLD_MAP
	  if (dyn->d_tag == DT_MIPS_RLD_MAP)
	    {
	      if (linux_read_memory (dyn->d_un.d_val,
				     rld_map.buf, sizeof (rld_map.buf)) == 0)
		return rld_map.map;
	      else
		break;
	    }
#endif	/* DT_MIPS_RLD_MAP */
#ifdef DT_MIPS_RLD_MAP_REL
	  if (dyn->d_tag == DT_MIPS_RLD_MAP_REL)
	    {
	      if (linux_read_memory (dyn->d_un.d_val + dynamic_memaddr,
				     rld_map.buf, sizeof (rld_map.buf)) == 0)
		return rld_map.map;
	      else
		break;
	    }
#endif	/* DT_MIPS_RLD_MAP_REL */

	  if (dyn->d_tag == DT_DEBUG && map == -1)
	    map = dyn->d_un.d_val;

	  if (dyn->d_tag == DT_NULL)
	    break;
	}

      dynamic_memaddr += dyn_size;
    }

  return map;
}

/* Read one pointer from MEMADDR in the inferior.  */

static int
read_one_ptr (CORE_ADDR memaddr, CORE_ADDR *ptr, int ptr_size)
{
  int ret;

  /* Go through a union so this works on either big or little endian
     hosts, when the inferior's pointer size is smaller than the size
     of CORE_ADDR.  It is assumed the inferior's endianness is the
     same of the superior's.  */
  union
  {
    CORE_ADDR core_addr;
    unsigned int ui;
    unsigned char uc;
  } addr;

  ret = linux_read_memory (memaddr, &addr.uc, ptr_size);
  if (ret == 0)
    {
      if (ptr_size == sizeof (CORE_ADDR))
	*ptr = addr.core_addr;
      else if (ptr_size == sizeof (unsigned int))
	*ptr = addr.ui;
      else
	gdb_assert_not_reached ("unhandled pointer size");
    }
  return ret;
}

struct link_map_offsets
  {
    /* Offset and size of r_debug.r_version.  */
    int r_version_offset;

    /* Offset and size of r_debug.r_map.  */
    int r_map_offset;

    /* Offset to l_addr field in struct link_map.  */
    int l_addr_offset;

    /* Offset to l_name field in struct link_map.  */
    int l_name_offset;

    /* Offset to l_ld field in struct link_map.  */
    int l_ld_offset;

    /* Offset to l_next field in struct link_map.  */
    int l_next_offset;

    /* Offset to l_prev field in struct link_map.  */
    int l_prev_offset;
  };

/* Construct qXfer:libraries-svr4:read reply.  */

static int
linux_qxfer_libraries_svr4 (const char *annex, unsigned char *readbuf,
			    unsigned const char *writebuf,
			    CORE_ADDR offset, int len)
{
  char *document;
  unsigned document_len;
  struct process_info_private *const priv = current_process ()->priv;
  char filename[PATH_MAX];
  int pid, is_elf64;

  static const struct link_map_offsets lmo_32bit_offsets =
    {
      0,     /* r_version offset. */
      4,     /* r_debug.r_map offset.  */
      0,     /* l_addr offset in link_map.  */
      4,     /* l_name offset in link_map.  */
      8,     /* l_ld offset in link_map.  */
      12,    /* l_next offset in link_map.  */
      16     /* l_prev offset in link_map.  */
    };

  static const struct link_map_offsets lmo_64bit_offsets =
    {
      0,     /* r_version offset. */
      8,     /* r_debug.r_map offset.  */
      0,     /* l_addr offset in link_map.  */
      8,     /* l_name offset in link_map.  */
      16,    /* l_ld offset in link_map.  */
      24,    /* l_next offset in link_map.  */
      32     /* l_prev offset in link_map.  */
    };
  const struct link_map_offsets *lmo;
  unsigned int machine;
  int ptr_size;
  CORE_ADDR lm_addr = 0, lm_prev = 0;
  int allocated = 1024;
  char *p;
  CORE_ADDR l_name, l_addr, l_ld, l_next, l_prev;
  int header_done = 0;

  if (writebuf != NULL)
    return -2;
  if (readbuf == NULL)
    return -1;

  pid = lwpid_of (current_thread);
  xsnprintf (filename, sizeof filename, "/proc/%d/exe", pid);
  is_elf64 = elf_64_file_p (filename, &machine);
  lmo = is_elf64 ? &lmo_64bit_offsets : &lmo_32bit_offsets;
  ptr_size = is_elf64 ? 8 : 4;

  while (annex[0] != '\0')
    {
      const char *sep;
      CORE_ADDR *addrp;
      int len;

      sep = strchr (annex, '=');
      if (sep == NULL)
	break;

      len = sep - annex;
      if (len == 5 && startswith (annex, "start"))
	addrp = &lm_addr;
      else if (len == 4 && startswith (annex, "prev"))
	addrp = &lm_prev;
      else
	{
	  annex = strchr (sep, ';');
	  if (annex == NULL)
	    break;
	  annex++;
	  continue;
	}

      annex = decode_address_to_semicolon (addrp, sep + 1);
    }

  if (lm_addr == 0)
    {
      int r_version = 0;

      if (priv->r_debug == 0)
	priv->r_debug = get_r_debug (pid, is_elf64);

      /* We failed to find DT_DEBUG.  Such situation will not change
	 for this inferior - do not retry it.  Report it to GDB as
	 E01, see for the reasons at the GDB solib-svr4.c side.  */
      if (priv->r_debug == (CORE_ADDR) -1)
	return -1;

      if (priv->r_debug != 0)
	{
	  if (linux_read_memory (priv->r_debug + lmo->r_version_offset,
				 (unsigned char *) &r_version,
				 sizeof (r_version)) != 0
	      || r_version != 1)
	    {
	      warning ("unexpected r_debug version %d", r_version);
	    }
	  else if (read_one_ptr (priv->r_debug + lmo->r_map_offset,
				 &lm_addr, ptr_size) != 0)
	    {
	      warning ("unable to read r_map from 0x%lx",
		       (long) priv->r_debug + lmo->r_map_offset);
	    }
	}
    }

  document = (char *) xmalloc (allocated);
  strcpy (document, "<library-list-svr4 version=\"1.0\"");
  p = document + strlen (document);

  while (lm_addr
	 && read_one_ptr (lm_addr + lmo->l_name_offset,
			  &l_name, ptr_size) == 0
	 && read_one_ptr (lm_addr + lmo->l_addr_offset,
			  &l_addr, ptr_size) == 0
	 && read_one_ptr (lm_addr + lmo->l_ld_offset,
			  &l_ld, ptr_size) == 0
	 && read_one_ptr (lm_addr + lmo->l_prev_offset,
			  &l_prev, ptr_size) == 0
	 && read_one_ptr (lm_addr + lmo->l_next_offset,
			  &l_next, ptr_size) == 0)
    {
      unsigned char libname[PATH_MAX];

      if (lm_prev != l_prev)
	{
	  warning ("Corrupted shared library list: 0x%lx != 0x%lx",
		   (long) lm_prev, (long) l_prev);
	  break;
	}

      /* Ignore the first entry even if it has valid name as the first entry
	 corresponds to the main executable.  The first entry should not be
	 skipped if the dynamic loader was loaded late by a static executable
	 (see solib-svr4.c parameter ignore_first).  But in such case the main
	 executable does not have PT_DYNAMIC present and this function already
	 exited above due to failed get_r_debug.  */
      if (lm_prev == 0)
	{
	  sprintf (p, " main-lm=\"0x%lx\"", (unsigned long) lm_addr);
	  p = p + strlen (p);
	}
      else
	{
	  /* Not checking for error because reading may stop before
	     we've got PATH_MAX worth of characters.  */
	  libname[0] = '\0';
	  linux_read_memory (l_name, libname, sizeof (libname) - 1);
	  libname[sizeof (libname) - 1] = '\0';
	  if (libname[0] != '\0')
	    {
	      /* 6x the size for xml_escape_text below.  */
	      size_t len = 6 * strlen ((char *) libname);
	      char *name;

	      if (!header_done)
		{
		  /* Terminate `<library-list-svr4'.  */
		  *p++ = '>';
		  header_done = 1;
		}

	      while (allocated < p - document + len + 200)
		{
		  /* Expand to guarantee sufficient storage.  */
		  uintptr_t document_len = p - document;

		  document = (char *) xrealloc (document, 2 * allocated);
		  allocated *= 2;
		  p = document + document_len;
		}

	      name = xml_escape_text ((char *) libname);
	      p += sprintf (p, "<library name=\"%s\" lm=\"0x%lx\" "
			    "l_addr=\"0x%lx\" l_ld=\"0x%lx\"/>",
			    name, (unsigned long) lm_addr,
			    (unsigned long) l_addr, (unsigned long) l_ld);
	      free (name);
	    }
	}

      lm_prev = lm_addr;
      lm_addr = l_next;
    }

  if (!header_done)
    {
      /* Empty list; terminate `<library-list-svr4'.  */
      strcpy (p, "/>");
    }
  else
    strcpy (p, "</library-list-svr4>");

  document_len = strlen (document);
  if (offset < document_len)
    document_len -= offset;
  else
    document_len = 0;
  if (len > document_len)
    len = document_len;

  memcpy (readbuf, document + offset, len);
  xfree (document);

  return len;
}

#ifdef HAVE_LINUX_BTRACE

/* See to_disable_btrace target method.  */

static int
linux_low_disable_btrace (struct btrace_target_info *tinfo)
{
  enum btrace_error err;

  err = linux_disable_btrace (tinfo);
  return (err == BTRACE_ERR_NONE ? 0 : -1);
}

/* Encode an Intel Processor Trace configuration.  */

static void
linux_low_encode_pt_config (struct buffer *buffer,
			    const struct btrace_data_pt_config *config)
{
  buffer_grow_str (buffer, "<pt-config>\n");

  switch (config->cpu.vendor)
    {
    case CV_INTEL:
      buffer_xml_printf (buffer, "<cpu vendor=\"GenuineIntel\" family=\"%u\" "
			 "model=\"%u\" stepping=\"%u\"/>\n",
			 config->cpu.family, config->cpu.model,
			 config->cpu.stepping);
      break;

    default:
      break;
    }

  buffer_grow_str (buffer, "</pt-config>\n");
}

/* Encode a raw buffer.  */

static void
linux_low_encode_raw (struct buffer *buffer, const gdb_byte *data,
		      unsigned int size)
{
  if (size == 0)
    return;

  /* We use hex encoding - see common/rsp-low.h.  */
  buffer_grow_str (buffer, "<raw>\n");

  while (size-- > 0)
    {
      char elem[2];

      elem[0] = tohex ((*data >> 4) & 0xf);
      elem[1] = tohex (*data++ & 0xf);

      buffer_grow (buffer, elem, 2);
    }

  buffer_grow_str (buffer, "</raw>\n");
}

/* See to_read_btrace target method.  */

static int
linux_low_read_btrace (struct btrace_target_info *tinfo, struct buffer *buffer,
		       enum btrace_read_type type)
{
  struct btrace_data btrace;
  struct btrace_block *block;
  enum btrace_error err;
  int i;

  btrace_data_init (&btrace);

  err = linux_read_btrace (&btrace, tinfo, type);
  if (err != BTRACE_ERR_NONE)
    {
      if (err == BTRACE_ERR_OVERFLOW)
	buffer_grow_str0 (buffer, "E.Overflow.");
      else
	buffer_grow_str0 (buffer, "E.Generic Error.");

      goto err;
    }

  switch (btrace.format)
    {
    case BTRACE_FORMAT_NONE:
      buffer_grow_str0 (buffer, "E.No Trace.");
      goto err;

    case BTRACE_FORMAT_BTS:
      buffer_grow_str (buffer, "<!DOCTYPE btrace SYSTEM \"btrace.dtd\">\n");
      buffer_grow_str (buffer, "<btrace version=\"1.0\">\n");

      for (i = 0;
	   VEC_iterate (btrace_block_s, btrace.variant.bts.blocks, i, block);
	   i++)
	buffer_xml_printf (buffer, "<block begin=\"0x%s\" end=\"0x%s\"/>\n",
			   paddress (block->begin), paddress (block->end));

      buffer_grow_str0 (buffer, "</btrace>\n");
      break;

    case BTRACE_FORMAT_PT:
      buffer_grow_str (buffer, "<!DOCTYPE btrace SYSTEM \"btrace.dtd\">\n");
      buffer_grow_str (buffer, "<btrace version=\"1.0\">\n");
      buffer_grow_str (buffer, "<pt>\n");

      linux_low_encode_pt_config (buffer, &btrace.variant.pt.config);

      linux_low_encode_raw (buffer, btrace.variant.pt.data,
			    btrace.variant.pt.size);

      buffer_grow_str (buffer, "</pt>\n");
      buffer_grow_str0 (buffer, "</btrace>\n");
      break;

    default:
      buffer_grow_str0 (buffer, "E.Unsupported Trace Format.");
      goto err;
    }

  btrace_data_fini (&btrace);
  return 0;

err:
  btrace_data_fini (&btrace);
  return -1;
}

/* See to_btrace_conf target method.  */

static int
linux_low_btrace_conf (const struct btrace_target_info *tinfo,
		       struct buffer *buffer)
{
  const struct btrace_config *conf;

  buffer_grow_str (buffer, "<!DOCTYPE btrace-conf SYSTEM \"btrace-conf.dtd\">\n");
  buffer_grow_str (buffer, "<btrace-conf version=\"1.0\">\n");

  conf = linux_btrace_conf (tinfo);
  if (conf != NULL)
    {
      switch (conf->format)
	{
	case BTRACE_FORMAT_NONE:
	  break;

	case BTRACE_FORMAT_BTS:
	  buffer_xml_printf (buffer, "<bts");
	  buffer_xml_printf (buffer, " size=\"0x%x\"", conf->bts.size);
	  buffer_xml_printf (buffer, " />\n");
	  break;

	case BTRACE_FORMAT_PT:
	  buffer_xml_printf (buffer, "<pt");
	  buffer_xml_printf (buffer, " size=\"0x%x\"", conf->pt.size);
	  buffer_xml_printf (buffer, "/>\n");
	  break;
	}
    }

  buffer_grow_str0 (buffer, "</btrace-conf>\n");
  return 0;
}
#endif /* HAVE_LINUX_BTRACE */

/* See nat/linux-nat.h.  */

ptid_t
current_lwp_ptid (void)
{
  return ptid_of (current_thread);
}

/* Implementation of the target_ops method "breakpoint_kind_from_pc".  */

static int
linux_breakpoint_kind_from_pc (CORE_ADDR *pcptr)
{
  if (the_low_target.breakpoint_kind_from_pc != NULL)
    return (*the_low_target.breakpoint_kind_from_pc) (pcptr);
  else
    return default_breakpoint_kind_from_pc (pcptr);
}

/* Implementation of the target_ops method "sw_breakpoint_from_kind".  */

static const gdb_byte *
linux_sw_breakpoint_from_kind (int kind, int *size)
{
  gdb_assert (the_low_target.sw_breakpoint_from_kind != NULL);

  return (*the_low_target.sw_breakpoint_from_kind) (kind, size);
}

/* Implementation of the target_ops method
   "breakpoint_kind_from_current_state".  */

static int
linux_breakpoint_kind_from_current_state (CORE_ADDR *pcptr)
{
  if (the_low_target.breakpoint_kind_from_current_state != NULL)
    return (*the_low_target.breakpoint_kind_from_current_state) (pcptr);
  else
    return linux_breakpoint_kind_from_pc (pcptr);
}

/* Default implementation of linux_target_ops method "set_pc" for
   32-bit pc register which is literally named "pc".  */

void
linux_set_pc_32bit (struct regcache *regcache, CORE_ADDR pc)
{
  uint32_t newpc = pc;

  supply_register_by_name (regcache, "pc", &newpc);
}

/* Default implementation of linux_target_ops method "get_pc" for
   32-bit pc register which is literally named "pc".  */

CORE_ADDR
linux_get_pc_32bit (struct regcache *regcache)
{
  uint32_t pc;

  collect_register_by_name (regcache, "pc", &pc);
  if (debug_threads)
    debug_printf ("stop pc is 0x%" PRIx32 "\n", pc);
  return pc;
}

/* Default implementation of linux_target_ops method "set_pc" for
   64-bit pc register which is literally named "pc".  */

void
linux_set_pc_64bit (struct regcache *regcache, CORE_ADDR pc)
{
  uint64_t newpc = pc;

  supply_register_by_name (regcache, "pc", &newpc);
}

/* Default implementation of linux_target_ops method "get_pc" for
   64-bit pc register which is literally named "pc".  */

CORE_ADDR
linux_get_pc_64bit (struct regcache *regcache)
{
  uint64_t pc;

  collect_register_by_name (regcache, "pc", &pc);
  if (debug_threads)
    debug_printf ("stop pc is 0x%" PRIx64 "\n", pc);
  return pc;
}


static struct target_ops linux_target_ops = {
  linux_create_inferior,
  linux_post_create_inferior,
  linux_attach,
  linux_kill,
  linux_detach,
  linux_mourn,
  linux_join,
  linux_thread_alive,
  linux_resume,
  linux_wait,
  linux_fetch_registers,
  linux_store_registers,
  linux_prepare_to_access_memory,
  linux_done_accessing_memory,
  linux_read_memory,
  linux_write_memory,
  linux_look_up_symbols,
  linux_request_interrupt,
  linux_read_auxv,
  linux_supports_z_point_type,
  linux_insert_point,
  linux_remove_point,
  linux_stopped_by_sw_breakpoint,
  linux_supports_stopped_by_sw_breakpoint,
  linux_stopped_by_hw_breakpoint,
  linux_supports_stopped_by_hw_breakpoint,
  linux_supports_hardware_single_step,
  linux_stopped_by_watchpoint,
  linux_stopped_data_address,
#if defined(__UCLIBC__) && defined(HAS_NOMMU)	      \
    && defined(PT_TEXT_ADDR) && defined(PT_DATA_ADDR) \
    && defined(PT_TEXT_END_ADDR)
  linux_read_offsets,
#else
  NULL,
#endif
#ifdef USE_THREAD_DB
  thread_db_get_tls_address,
#else
  NULL,
#endif
  linux_qxfer_spu,
  hostio_last_error_from_errno,
  linux_qxfer_osdata,
  linux_xfer_siginfo,
  linux_supports_non_stop,
  linux_async,
  linux_start_non_stop,
  linux_supports_multi_process,
  linux_supports_fork_events,
  linux_supports_vfork_events,
  linux_supports_exec_events,
  linux_handle_new_gdb_connection,
#ifdef USE_THREAD_DB
  thread_db_handle_monitor_command,
#else
  NULL,
#endif
  linux_common_core_of_thread,
  linux_read_loadmap,
  linux_process_qsupported,
  linux_supports_tracepoints,
  linux_read_pc,
  linux_write_pc,
  linux_thread_stopped,
  NULL,
  linux_pause_all,
  linux_unpause_all,
  linux_stabilize_threads,
  linux_install_fast_tracepoint_jump_pad,
  linux_emit_ops,
  linux_supports_disable_randomization,
  linux_get_min_fast_tracepoint_insn_len,
  linux_qxfer_libraries_svr4,
  linux_supports_agent,
#ifdef HAVE_LINUX_BTRACE
  linux_supports_btrace,
  linux_enable_btrace,
  linux_low_disable_btrace,
  linux_low_read_btrace,
  linux_low_btrace_conf,
#else
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
#endif
  linux_supports_range_stepping,
  linux_proc_pid_to_exec_file,
  linux_mntns_open_cloexec,
  linux_mntns_unlink,
  linux_mntns_readlink,
  linux_breakpoint_kind_from_pc,
  linux_sw_breakpoint_from_kind,
  linux_proc_tid_get_name,
  linux_breakpoint_kind_from_current_state,
  linux_supports_software_single_step,
  linux_supports_catch_syscall,
  linux_get_ipa_tdesc_idx,
};

#ifdef HAVE_LINUX_REGSETS
void
initialize_regsets_info (struct regsets_info *info)
{
  for (info->num_regsets = 0;
       info->regsets[info->num_regsets].size >= 0;
       info->num_regsets++)
    ;
}
#endif

void
initialize_low (void)
{
  struct sigaction sigchld_action;

  memset (&sigchld_action, 0, sizeof (sigchld_action));
  set_target_ops (&linux_target_ops);

  linux_ptrace_init_warnings ();

  sigchld_action.sa_handler = sigchld_handler;
  sigemptyset (&sigchld_action.sa_mask);
  sigchld_action.sa_flags = SA_RESTART;
  sigaction (SIGCHLD, &sigchld_action, NULL);

  initialize_low_arch ();

  linux_check_ptrace_features ();
}
