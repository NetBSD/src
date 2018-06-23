/* Native-dependent code for NetBSD

   Copyright (C) 2002-2017 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "inferior.h"
#include "regcache.h"
#include "regset.h"
#include "gdbcmd.h"
#include "gdbthread.h"
#include "gdb_wait.h"
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
#ifdef HAVE_KINFO_GETVMMAP
#include <util.h>
#endif

#include "elf-bfd.h"
#include "nbsd-nat.h"

/* Return the name of a file that can be opened to get the symbols for
   the child process identified by PID.  */

static char *
nbsd_pid_to_exec_file (struct target_ops *self, int pid)
{
  ssize_t len;
  static char buf[PATH_MAX];
  char name[PATH_MAX];

#ifdef KERN_PROC_PATHNAME
  size_t buflen;
  int mib[4];

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC_ARGS;
  mib[2] = pid;
  mib[3] = KERN_PROC_PATHNAME;
  buflen = sizeof buf;
  if (sysctl (mib, 4, buf, &buflen, NULL, 0) == 0)
    return buf;
#endif

  xsnprintf (name, PATH_MAX, "/proc/%d/exe", pid);
  len = readlink (name, buf, PATH_MAX - 1);
  if (len != -1)
    {
      buf[len] = '\0';
      return buf;
    }

  return NULL;
}

/* Iterate over all the memory regions in the current inferior,
   calling FUNC for each memory region.  OBFD is passed as the last
   argument to FUNC.  */

static int
nbsd_find_memory_regions (struct target_ops *self,
			  find_memory_region_ftype func, void *obfd)
{
  pid_t pid = ptid_get_pid (inferior_ptid);
  struct kinfo_vmentry *vmentl, *kve;
  uint64_t size;
  struct cleanup *cleanup;
  int i;
  size_t nitems;

  vmentl = kinfo_getvmmap (pid, &nitems);
  if (vmentl == NULL)
    perror_with_name (_("Couldn't fetch VM map entries."));
  cleanup = make_cleanup (free, vmentl);

  for (i = 0; i < nitems; i++)
    {
      kve = &vmentl[i];

      /* Skip unreadable segments and those where MAP_NOCORE has been set.  */
      if (!(kve->kve_protection & KVME_PROT_READ)
	  || kve->kve_flags & KVME_FLAG_NOCOREDUMP)
	continue;

      /* Skip segments with an invalid type.  */
      switch (kve->kve_type) {
	case KVME_TYPE_VNODE:
	case KVME_TYPE_ANON:
	case KVME_TYPE_SUBMAP:
	case KVME_TYPE_OBJECT:
	  break;
	default:
	  continue;
      }

      size = kve->kve_end - kve->kve_start;
      if (info_verbose)
	{
	  fprintf_filtered (gdb_stdout, 
			    "Save segment, %ld bytes at %s (%c%c%c)\n",
			    (long) size,
			    paddress (target_gdbarch (), kve->kve_start),
			    kve->kve_protection & KVME_PROT_READ ? 'r' : '-',
			    kve->kve_protection & KVME_PROT_WRITE ? 'w' : '-',
			    kve->kve_protection & KVME_PROT_EXEC ? 'x' : '-');
	}

      /* Invoke the callback function to create the corefile segment.
	 Pass MODIFIED as true, we do not know the real modification state.  */
      func (kve->kve_start, size, kve->kve_protection & KVME_PROT_READ,
	    kve->kve_protection & KVME_PROT_WRITE,
	    kve->kve_protection & KVME_PROT_EXEC, 1, obfd);
    }
  do_cleanups (cleanup);
  return 0;
}

#ifdef KERN_PROC_AUXV
static enum target_xfer_status (*super_xfer_partial) (struct target_ops *ops,
						      enum target_object object,
						      const char *annex,
						      gdb_byte *readbuf,
						      const gdb_byte *writebuf,
						      ULONGEST offset,
						      ULONGEST len,
						      ULONGEST *xfered_len);

/* Implement the "to_xfer_partial target_ops" method.  */

static enum target_xfer_status
nbsd_xfer_partial (struct target_ops *ops, enum target_object object,
		   const char *annex, gdb_byte *readbuf,
		   const gdb_byte *writebuf,
		   ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  pid_t pid = ptid_get_pid (inferior_ptid);

  switch (object)
    {
    case TARGET_OBJECT_AUXV:
      {
	struct cleanup *cleanup = make_cleanup (null_cleanup, NULL);
	unsigned char *buf;
	size_t buflen;
	int mib[4];

	if (writebuf != NULL)
	  return TARGET_XFER_E_IO;
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_AUXV;
	mib[3] = pid;
	if (offset == 0)
	  {
	    buf = readbuf;
	    buflen = len;
	  }
	else
	  {
	    buflen = offset + len;
	    buf = XCNEWVEC (unsigned char, buflen);
	    cleanup = make_cleanup (xfree, buf);
	  }
	if (sysctl (mib, 4, buf, &buflen, NULL, 0) == 0)
	  {
	    if (offset != 0)
	      {
		if (buflen > offset)
		  {
		    buflen -= offset;
		    memcpy (readbuf, buf + offset, buflen);
		  }
		else
		  buflen = 0;
	      }
	    do_cleanups (cleanup);
	    *xfered_len = buflen;
	    return (buflen == 0) ? TARGET_XFER_EOF : TARGET_XFER_OK;
	  }
	do_cleanups (cleanup);
	return TARGET_XFER_E_IO;
      }
    default:
      return super_xfer_partial (ops, object, annex, readbuf, writebuf, offset,
				 len, xfered_len);
    }
}
#endif

#ifdef PT_LWPINFO
static int debug_nbsd_lwp;

static void (*super_resume) (struct target_ops *,
			     ptid_t,
			     int,
			     enum gdb_signal);
static ptid_t (*super_wait) (struct target_ops *,
			     ptid_t,
			     struct target_waitstatus *,
			     int);

static void
show_nbsd_lwp_debug (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Debugging of NetBSD lwp module is %s.\n"), value);
}

#if defined(TDP_RFPPWAIT) || defined(HAVE_STRUCT_PTRACE_LWPINFO_PL_TDNAME)
/* Fetch the external variant of the kernel's internal process
   structure for the process PID into KP.  */

static void
nbsd_fetch_kinfo_proc (pid_t pid, struct kinfo_proc *kp)
{
  size_t len;
  int mib[4];

  len = sizeof *kp;
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = pid;
  if (sysctl (mib, 4, kp, &len, NULL, 0) == -1)
    perror_with_name (("sysctl"));
}
#endif


/* Return true if PTID is still active in the inferior.  */

static int
nbsd_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  if (ptid_lwp_p (ptid))
    {
      struct ptrace_lwpinfo pl;

      pl.pl_lwpid = ptid_get_lwp (ptid);
      if (ptrace (PT_LWPINFO, ptid_get_pid (ptid), (caddr_t) &pl, sizeof pl)
	  == -1)
	return 0;
#ifdef PL_FLAG_EXITED
      if (pl.pl_flags & PL_FLAG_EXITED)
	return 0;
#endif
    }

  return 1;
}

/* Convert PTID to a string.  Returns the string in a static
   buffer.  */

static const char *
nbsd_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  lwpid_t lwp;

  lwp = ptid_get_lwp (ptid);
  if (lwp != 0)
    {
      static char buf[64];
      int pid = ptid_get_pid (ptid);

      xsnprintf (buf, sizeof buf, "LWP %d of process %d", lwp, pid);
      return buf;
    }

  return normal_pid_to_str (ptid);
}

#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_TDNAME
/* Return the name assigned to a thread by an application.  Returns
   the string in a static buffer.  */

static const char *
nbsd_thread_name (struct target_ops *self, struct thread_info *thr)
{
  struct ptrace_lwpinfo pl;
  struct kinfo_proc kp;
  pid_t pid = ptid_get_pid (thr->ptid);
  lwpid_t lwp = ptid_get_lwp (thr->ptid);
  static char buf[sizeof pl.pl_tdname + 1];

  /* Note that ptrace_lwpinfo returns the process command in pl_tdname
     if a name has not been set explicitly.  Return a NULL name in
     that case.  */
  nbsd_fetch_kinfo_proc (pid, &kp);
  pl.pl_lwpid = lwp;
  if (ptrace (PT_LWPINFO, pid, (caddr_t) &pl, sizeof pl) == -1)
    perror_with_name (("ptrace"));
  if (strcmp (kp.ki_comm, pl.pl_tdname) == 0)
    return NULL;
  xsnprintf (buf, sizeof buf, "%s", pl.pl_tdname);
  return buf;
}
#endif

/* Enable additional event reporting on new processes.

   To catch fork events, PTRACE_FORK is set on every traced process
   to enable stops on returns from fork or vfork.  Note that both the
   parent and child will always stop, even if system call stops are
   not enabled.

   To catch LWP events, PTRACE_EVENTS is set on every traced process.
   This enables stops on the birth for new LWPs (excluding the "main" LWP)
   and the death of LWPs (excluding the last LWP in a process).  Note
   that unlike fork events, the LWP that creates a new LWP does not
   report an event.  */

static void
nbsd_enable_proc_events (pid_t pid)
{
#ifdef PT_GET_EVENT_MASK
  int events;

  if (ptrace (PT_GET_EVENT_MASK, pid, (PTRACE_TYPE_ARG3)&events,
	      sizeof (events)) == -1)
    perror_with_name (("ptrace"));
  events |= PTRACE_FORK;
#ifdef PTRACE_LWP
  events |= PTRACE_LWP;
#endif
#ifdef notyet
#ifdef PTRACE_VFORK
  events |= PTRACE_VFORK;
#endif
#endif
  if (ptrace (PT_SET_EVENT_MASK, pid, (PTRACE_TYPE_ARG3)&events,
	      sizeof (events)) == -1)
    perror_with_name (("ptrace"));
#else
#ifdef TDP_RFPPWAIT
  if (ptrace (PT_FOLLOW_FORK, pid, (PTRACE_TYPE_ARG3)0, 1) == -1)
    perror_with_name (("ptrace"));
#endif
#ifdef PT_LWP_EVENTS
  if (ptrace (PT_LWP_EVENTS, pid, (PTRACE_TYPE_ARG3)0, 1) == -1)
    perror_with_name (("ptrace"));
#endif
#endif
}

/* Add threads for any new LWPs in a process.

   When LWP events are used, this function is only used to detect existing
   threads when attaching to a process.  On older systems, this function is
   called to discover new threads each time the thread list is updated.  */

static void
nbsd_add_threads (pid_t pid)
{
  int val;
  struct ptrace_lwpinfo pl;

//  gdb_assert (!in_thread_list (pid_to_ptid (pid)));
  pl.pl_lwpid = 0;
  while ((val = ptrace (PT_LWPINFO, pid, (void *)&pl, sizeof(pl))) != -1
    && pl.pl_lwpid != 0)
    {
      ptid_t ptid = ptid_build (pid, pl.pl_lwpid, 0);
      if (!in_thread_list (ptid))
	add_thread (ptid);
    }
}

/* Implement the "to_update_thread_list" target_ops method.  */

static void
nbsd_update_thread_list (struct target_ops *ops)
{
#ifdef PT_LWP_EVENTS
  /* With support for thread events, threads are added/deleted from the
     list as events are reported, so just try deleting exited threads.  */
  delete_exited_threads ();
#else
  prune_threads ();

  nbsd_add_threads (ptid_get_pid (inferior_ptid));
#endif
}

#ifdef TDP_RFPPWAIT
/*
  To catch fork events, PT_FOLLOW_FORK is set on every traced process
  to enable stops on returns from fork or vfork.  Note that both the
  parent and child will always stop, even if system call stops are not
  enabled.

  After a fork, both the child and parent process will stop and report
  an event.  However, there is no guarantee of order.  If the parent
  reports its stop first, then nbsd_wait explicitly waits for the new
  child before returning.  If the child reports its stop first, then
  the event is saved on a list and ignored until the parent's stop is
  reported.  nbsd_wait could have been changed to fetch the parent PID
  of the new child and used that to wait for the parent explicitly.
  However, if two threads in the parent fork at the same time, then
  the wait on the parent might return the "wrong" fork event.

  The initial version of PT_FOLLOW_FORK did not set PL_FLAG_CHILD for
  the new child process.  This flag could be inferred by treating any
  events for an unknown pid as a new child.

  In addition, the initial version of PT_FOLLOW_FORK did not report a
  stop event for the parent process of a vfork until after the child
  process executed a new program or exited.  The kernel was changed to
  defer the wait for exit or exec of the child until after posting the
  stop event shortly after the change to introduce PL_FLAG_CHILD.
  This could be worked around by reporting a vfork event when the
  child event posted and ignoring the subsequent event from the
  parent.

  This implementation requires both of these fixes for simplicity's
  sake.  FreeBSD versions newer than 9.1 contain both fixes.
*/

struct nbsd_fork_info
{
  struct nbsd_fork_info *next;
  ptid_t ptid;
};

static struct nbsd_fork_info *nbsd_pending_children;

/* Record a new child process event that is reported before the
   corresponding fork event in the parent.  */

static void
nbsd_remember_child (ptid_t pid)
{
  struct nbsd_fork_info *info = XCNEW (struct nbsd_fork_info);

  info->ptid = pid;
  info->next = nbsd_pending_children;
  nbsd_pending_children = info;
}

/* Check for a previously-recorded new child process event for PID.
   If one is found, remove it from the list and return the PTID.  */

static ptid_t
nbsd_is_child_pending (pid_t pid)
{
  struct nbsd_fork_info *info, *prev;
  ptid_t ptid;

  prev = NULL;
  for (info = nbsd_pending_children; info; prev = info, info = info->next)
    {
      if (ptid_get_pid (info->ptid) == pid)
	{
	  if (prev == NULL)
	    nbsd_pending_children = info->next;
	  else
	    prev->next = info->next;
	  ptid = info->ptid;
	  xfree (info);
	  return ptid;
	}
    }
  return null_ptid;
}

#ifndef PTRACE_VFORK
static struct nbsd_fork_info *nbsd_pending_vfork_done;

/* Record a pending vfork done event.  */

static void
nbsd_add_vfork_done (ptid_t pid)
{
  struct nbsd_fork_info *info = XCNEW (struct nbsd_fork_info);

  info->ptid = pid;
  info->next = nbsd_pending_vfork_done;
  nbsd_pending_vfork_done = info;
}

/* Check for a pending vfork done event for a specific PID.  */

static int
nbsd_is_vfork_done_pending (pid_t pid)
{
  struct nbsd_fork_info *info;

  for (info = nbsd_pending_vfork_done; info != NULL; info = info->next)
    {
      if (ptid_get_pid (info->ptid) == pid)
	return 1;
    }
  return 0;
}

/* Check for a pending vfork done event.  If one is found, remove it
   from the list and return the PTID.  */

static ptid_t
nbsd_next_vfork_done (void)
{
  struct nbsd_fork_info *info;
  ptid_t ptid;

  if (nbsd_pending_vfork_done != NULL)
    {
      info = nbsd_pending_vfork_done;
      nbsd_pending_vfork_done = info->next;
      ptid = info->ptid;
      xfree (info);
      return ptid;
    }
  return null_ptid;
}
#endif
#endif

/* Implement the "to_resume" target_ops method.  */

static void
nbsd_resume (struct target_ops *ops,
	     ptid_t ptid, int step, enum gdb_signal signo)
{
  if (debug_nbsd_lwp)
    fprintf_unfiltered (gdb_stdlog,
			"NLWP: nbsd_resume for ptid (%d, %ld, %ld)\n",
			ptid_get_pid (ptid), ptid_get_lwp (ptid),
			ptid_get_tid (ptid));
  if (ptid_get_pid(ptid) == -1)
    ptid = inferior_ptid;
  super_resume (ops, ptid, step, signo);
}

/* Wait for the child specified by PTID to do something.  Return the
   process ID of the child, or MINUS_ONE_PTID in case of error; store
   the status in *OURSTATUS.  */

static ptid_t
nbsd_wait (struct target_ops *ops,
	   ptid_t ptid, struct target_waitstatus *ourstatus,
	   int target_options)
{
  ptid_t wptid;

  wptid = super_wait (ops, ptid, ourstatus, target_options);
  if (ourstatus->kind == TARGET_WAITKIND_STOPPED)
    {
      struct ptrace_lwpinfo pl;
      pid_t pid;
      int status;
      pid = ptid_get_pid (wptid);
      // Find the lwp that caused the wait status change
      pl.pl_lwpid = 0;
      do {
	if (ptrace (PT_LWPINFO, pid, (caddr_t) &pl, sizeof pl) == -1)
	  perror_with_name (("ptrace"));
	if (pl.pl_event == PL_EVENT_SIGNAL)
	  break;
      } while (pl.pl_lwpid != 0);
      if (pl.pl_lwpid != 0)
	wptid = ptid_build (pid, pl.pl_lwpid, 0);
      if (!in_thread_list (wptid))
	add_thread (wptid);

      if (debug_nbsd_lwp)
	fprintf_unfiltered (gdb_stdlog,
		"NLWP: nbsd_wait returned (%d, %ld, %ld)\n",
		ptid_get_pid (wptid), ptid_get_lwp (wptid),
		ptid_get_tid (wptid));
      inferior_ptid = wptid;

    }
    return wptid;
}

#ifdef TDP_RFPPWAIT
/* Target hook for follow_fork.  On entry and at return inferior_ptid is
   the ptid of the followed inferior.  */

static int
nbsd_follow_fork (struct target_ops *ops, int follow_child,
			int detach_fork)
{
  if (!follow_child && detach_fork)
    {
      struct thread_info *tp = inferior_thread ();
      pid_t child_pid = ptid_get_pid (tp->pending_follow.value.related_pid);

      /* Breakpoints have already been detached from the child by
	 infrun.c.  */

      if (ptrace (PT_DETACH, child_pid, (PTRACE_TYPE_ARG3)1, 0) == -1)
	perror_with_name (("ptrace"));
    }

  return 0;
}

static int
nbsd_insert_fork_catchpoint (struct target_ops *self, int pid)
{
  return 0;
}

static int
nbsd_remove_fork_catchpoint (struct target_ops *self, int pid)
{
  return 0;
}

static int
nbsd_insert_vfork_catchpoint (struct target_ops *self, int pid)
{
  return 0;
}

static int
nbsd_remove_vfork_catchpoint (struct target_ops *self, int pid)
{
  return 0;
}
#endif

/* Implement the "to_post_startup_inferior" target_ops method.  */

static void
nbsd_post_startup_inferior (struct target_ops *self, ptid_t pid)
{
  nbsd_enable_proc_events (ptid_get_pid (pid));
}

/* Implement the "to_post_attach" target_ops method.  */

static void
nbsd_post_attach (struct target_ops *self, int pid)
{
  nbsd_enable_proc_events (pid);
  nbsd_add_threads (pid);
}

#ifdef PL_FLAG_EXEC
/* If the FreeBSD kernel supports PL_FLAG_EXEC, then traced processes
   will always stop after exec.  */

static int
nbsd_insert_exec_catchpoint (struct target_ops *self, int pid)
{
  return 0;
}

static int
nbsd_remove_exec_catchpoint (struct target_ops *self, int pid)
{
  return 0;
}
#endif

#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_SYSCALL_CODE
static int
nbsd_set_syscall_catchpoint (struct target_ops *self, int pid, int needed,
			     int any_count, int table_size, int *table)
{

  /* Ignore the arguments.  inf-ptrace.c will use PT_SYSCALL which
     will catch all system call entries and exits.  The system calls
     are filtered by GDB rather than the kernel.  */
  return 0;
}
#endif
#endif

void
nbsd_nat_add_target (struct target_ops *t)
{
  t->to_pid_to_exec_file = nbsd_pid_to_exec_file;
  t->to_find_memory_regions = nbsd_find_memory_regions;
#ifdef KERN_PROC_AUXV
  super_xfer_partial = t->to_xfer_partial;
  t->to_xfer_partial = nbsd_xfer_partial;
#endif
#ifdef PT_LWPINFO
  t->to_thread_alive = nbsd_thread_alive;
  t->to_pid_to_str = nbsd_pid_to_str;
#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_TDNAME
  t->to_thread_name = nbsd_thread_name;
#endif
  t->to_update_thread_list = nbsd_update_thread_list;
  t->to_has_thread_control = tc_schedlock;
  super_resume = t->to_resume;
  t->to_resume = nbsd_resume;
  super_wait = t->to_wait;
  t->to_wait = nbsd_wait;
  t->to_post_startup_inferior = nbsd_post_startup_inferior;
  t->to_post_attach = nbsd_post_attach;
#ifdef TDP_RFPPWAIT
  t->to_follow_fork = nbsd_follow_fork;
  t->to_insert_fork_catchpoint = nbsd_insert_fork_catchpoint;
  t->to_remove_fork_catchpoint = nbsd_remove_fork_catchpoint;
  t->to_insert_vfork_catchpoint = nbsd_insert_vfork_catchpoint;
  t->to_remove_vfork_catchpoint = nbsd_remove_vfork_catchpoint;
#endif
#ifdef PL_FLAG_EXEC
  t->to_insert_exec_catchpoint = nbsd_insert_exec_catchpoint;
  t->to_remove_exec_catchpoint = nbsd_remove_exec_catchpoint;
#endif
#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_SYSCALL_CODE
  t->to_set_syscall_catchpoint = nbsd_set_syscall_catchpoint;
#endif
#endif
  add_target (t);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_nbsd_nat;

void
_initialize_nbsd_nat (void)
{
#ifdef PT_LWPINFO
  add_setshow_boolean_cmd ("nbsd-lwp", class_maintenance,
			   &debug_nbsd_lwp, _("\
Set debugging of NetBSD lwp module."), _("\
Show debugging of NetBSD lwp module."), _("\
Enables printf debugging output."),
			   NULL,
			   &show_nbsd_lwp_debug,
			   &setdebuglist, &showdebuglist);
#endif
}
