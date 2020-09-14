/* Native-dependent code for NetBSD

   Copyright (C) 2006-2019 Free Software Foundation, Inc.

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
#include "common/gdb_wait.h"
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

char *
nbsd_nat_target::pid_to_exec_file (int pid)
{
  ssize_t len;
  static char buf[PATH_MAX];
  char name[PATH_MAX];

  size_t buflen;
  int mib[4];

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC_ARGS;
  mib[2] = pid;
  mib[3] = KERN_PROC_PATHNAME;
  buflen = sizeof buf;
  if (sysctl (mib, 4, buf, &buflen, NULL, 0) == 0)
    return buf;

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

int
nbsd_nat_target::find_memory_regions (find_memory_region_ftype func,
					   void *obfd)
{
  pid_t pid = inferior_ptid.pid ();
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

static int debug_nbsd_lwp;

static void
show_nbsd_lwp_debug (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Debugging of NetBSD lwp module is %s.\n"), value);
}

/* Return true if PTID is still active in the inferior.  */

bool
nbsd_nat_target::thread_alive (ptid_t ptid)
{
  if (ptid.lwp_p ())
    {
      struct ptrace_lwpstatus pl;

      pl.pl_lwpid = ptid.lwp ();
      if (ptrace (PT_LWPSTATUS, ptid.pid (), (caddr_t) &pl, sizeof pl)
	  == -1)
	return 0;
    }

  return 1;
}

/* Convert PTID to a string.  Returns the string in a static
   buffer.  */

const char *
nbsd_nat_target::pid_to_str (ptid_t ptid)
{
  lwpid_t lwp;

  lwp = ptid.lwp ();
  if (lwp != 0)
    {
      static char buf[64];
      int pid = ptid.pid ();

      xsnprintf (buf, sizeof buf, "LWP %d of process %d", lwp, pid);
      return buf;
    }

  return normal_pid_to_str (ptid);
}

/* Return the name assigned to a thread by an application.  Returns
   the string in a static buffer.  */

const char *
nbsd_nat_target::thread_name (struct thread_info *thr)
{
  struct kinfo_lwp *kl;
  pid_t pid = thr->ptid.pid ();
  lwpid_t lwp = thr->ptid.lwp ();
  static char buf[KI_LNAMELEN];
  int mib[5];
  size_t i, nlwps;
  size_t size;

  mib[0] = CTL_KERN;
  mib[1] = KERN_LWP;
  mib[2] = pid;
  mib[3] = sizeof(struct kinfo_lwp);
  mib[4] = 0;

  if (sysctl(mib, 5, NULL, &size, NULL, 0) == -1 || size == 0)
    perror_with_name (("sysctl"));

  mib[4] = size / sizeof(size_t);

  kl = (struct kinfo_lwp *) xmalloc (size);
  if (kl == NULL)
    perror_with_name (("malloc"));

  if (sysctl(mib, 5, kl, &size, NULL, 0) == -1 || size == 0)
    perror_with_name (("sysctl"));

  nlwps = size / sizeof(struct kinfo_lwp);
  buf[0] = '\0';
  for (i = 0; i < nlwps; i++) {
    if (kl[i].l_lid == lwp) {
      xsnprintf (buf, sizeof buf, "%s", kl[i].l_name);
      break;
    }
  }
  xfree(kl);

  return buf;
}

/* Enable additional event reporting on new processes. */

static void
nbsd_enable_proc_events (pid_t pid)
{
  int events;

  if (ptrace (PT_GET_EVENT_MASK, pid, (PTRACE_TYPE_ARG3)&events,
	      sizeof (events)) == -1)
    perror_with_name (("ptrace"));
  events |= PTRACE_FORK;
  events |= PTRACE_VFORK;
  events |= PTRACE_VFORK_DONE;
  events |= PTRACE_LWP_CREATE;
  events |= PTRACE_LWP_EXIT;
#if notyet
  events |= PTRACE_POSIX_SPAWN;
#endif
  if (ptrace (PT_SET_EVENT_MASK, pid, (PTRACE_TYPE_ARG3)&events,
	      sizeof (events)) == -1)
    perror_with_name (("ptrace"));
}

/* Add threads for any new LWPs in a process.

   When LWP events are used, this function is only used to detect existing
   threads when attaching to a process.  On older systems, this function is
   called to discover new threads each time the thread list is updated.  */

static void
nbsd_add_threads (pid_t pid)
{
  int val;
  struct ptrace_lwpstatus pl;

  pl.pl_lwpid = 0;
  while ((val = ptrace (PT_LWPNEXT, pid, (void *)&pl, sizeof(pl))) != -1
    && pl.pl_lwpid != 0)
    {
      ptid_t ptid = ptid_t (pid, pl.pl_lwpid, 0);
      if (!in_thread_list (ptid))
	{
	  if (inferior_ptid.lwp () == 0)
	    thread_change_ptid (inferior_ptid, ptid);
	  else
	    add_thread (ptid);
	}
    }
}

/* Implement the "to_update_thread_list" target_ops method.  */

void
nbsd_nat_target::update_thread_list ()
{
  prune_threads ();

  nbsd_add_threads (inferior_ptid.pid ());
}


struct nbsd_fork_info
{
  struct nbsd_fork_info *next;
  ptid_t ptid;
};

void
nbsd_nat_target::resume (ptid_t ptid, int step, enum gdb_signal signo)
{
  if (debug_nbsd_lwp)
    fprintf_unfiltered (gdb_stdlog,
			"NLWP: nbsd_resume for ptid (%d, %ld, %ld)\n",
			ptid.pid (), ptid.lwp (), ptid.tid ());
  if (ptid.pid () == -1)
    ptid = inferior_ptid;
  inf_ptrace_target::resume (ptid, step, signo);
}

/* Wait for the child specified by PTID to do something.  Return the
   process ID of the child, or MINUS_ONE_PTID in case of error; store
   the status in *OURSTATUS.  */

ptid_t
nbsd_nat_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
		       int target_options)
{
  ptid_t wptid;

  /*
   * Always perform polling on exact PID, overwrite the default polling on
   * WAIT_ANY.
   *
   * This avoids events reported in random order reported for FORK / VFORK.
   *
   * Polling on traced parent always simplifies the code.
   */
  ptid = inferior_ptid;

  if (debug_nbsd_lwp)
    fprintf_unfiltered (gdb_stdlog, "NLWP: calling super_wait (%d, %ld, %ld) target_options=%#x\n",
                        ptid.pid (), ptid.lwp (), ptid.tid (), target_options);

  wptid = inf_ptrace_target::wait (ptid, ourstatus, target_options);

  if (debug_nbsd_lwp)
    fprintf_unfiltered (gdb_stdlog, "NLWP: returned from super_wait (%d, %ld, %ld) target_options=%#x with ourstatus->kind=%d\n",
                        ptid.pid (), ptid.lwp (), ptid.tid (),
			target_options, ourstatus->kind);

  if (ourstatus->kind == TARGET_WAITKIND_STOPPED)
    {
      ptrace_state_t pst;
      ptrace_siginfo_t psi, child_psi;
      int status;
      pid_t pid, child, wchild;
      ptid_t child_ptid;
      lwpid_t lwp;

      pid = wptid.pid ();
      // Find the lwp that caused the wait status change
      if (ptrace(PT_GET_SIGINFO, pid, &psi, sizeof(psi)) == -1)
        perror_with_name (("ptrace"));

      /* For whole-process signals pick random thread */
      if (psi.psi_lwpid == 0) {
        // XXX: Is this always valid?
        lwp = inferior_ptid.lwp ();
      } else {
        lwp = psi.psi_lwpid;
      }

      wptid = ptid_t (pid, lwp, 0);

      /* Set LWP in the process */
      if (in_thread_list (ptid_t (pid))) {
          if (debug_nbsd_lwp)
            fprintf_unfiltered (gdb_stdlog,
                                "NLWP: using LWP %d for first thread\n",
                                lwp);
          thread_change_ptid (ptid_t (pid), wptid);                                                                                                 
      }

      if (debug_nbsd_lwp)
         fprintf_unfiltered (gdb_stdlog,
                             "NLWP: received signal=%d si_code=%d in process=%d lwp=%d\n",
                             psi.psi_siginfo.si_signo, psi.psi_siginfo.si_code, pid, lwp);

      switch (psi.psi_siginfo.si_signo) {
      case SIGTRAP:
        switch (psi.psi_siginfo.si_code) {
        case TRAP_BRKPT:
//          lp->stop_reason = TARGET_STOPPED_BY_SW_BREAKPOINT;
          break;
        case TRAP_DBREG:
//          if (hardware_breakpoint_inserted_here_p (get_regcache_aspace (regcache), pc))
//            lp->stop_reason = TARGET_STOPPED_BY_HW_BREAKPOINT;
//          else
//            lp->stop_reason = TARGET_STOPPED_BY_WATCHPOINT;
          break;
        case TRAP_TRACE:
//          lp->stop_reason = TARGET_STOPPED_BY_SINGLE_STEP;
          break;
        case TRAP_SCE:
          ourstatus->kind = TARGET_WAITKIND_SYSCALL_ENTRY;
          ourstatus->value.syscall_number = psi.psi_siginfo.si_sysnum;
          break;
        case TRAP_SCX:
          ourstatus->kind = TARGET_WAITKIND_SYSCALL_RETURN;
          ourstatus->value.syscall_number = psi.psi_siginfo.si_sysnum;
          break;
        case TRAP_EXEC:
          ourstatus->kind = TARGET_WAITKIND_EXECD;
          ourstatus->value.execd_pathname = xstrdup(pid_to_exec_file (pid));
          break;
        case TRAP_LWP:
        case TRAP_CHLD:
          if (ptrace(PT_GET_PROCESS_STATE, pid, &pst, sizeof(pst)) == -1)
            perror_with_name (("ptrace"));
          switch (pst.pe_report_event) {
          case PTRACE_FORK:
          case PTRACE_VFORK:
            if (pst.pe_report_event == PTRACE_FORK)
              ourstatus->kind = TARGET_WAITKIND_FORKED;
            else
              ourstatus->kind = TARGET_WAITKIND_VFORKED;
            child = pst.pe_other_pid;

            if (debug_nbsd_lwp)
              fprintf_unfiltered (gdb_stdlog,
                                  "NLWP: registered %s event for PID %d\n",
                                  (pst.pe_report_event == PTRACE_FORK) ? "FORK" : "VFORK", child);

            wchild = waitpid (child, &status, 0);

            if (wchild == -1)
              perror_with_name (("waitpid"));

            gdb_assert (wchild == child);

            if (!WIFSTOPPED(status)) {
              /* Abnormal situation (SIGKILLed?).. bail out */
              ourstatus->kind = TARGET_WAITKIND_SPURIOUS;
              return wptid;
            }

            if (ptrace(PT_GET_SIGINFO, child, &child_psi, sizeof(child_psi)) == -1)
              perror_with_name (("ptrace"));

            if (child_psi.psi_siginfo.si_signo != SIGTRAP) {
              /* Abnormal situation.. bail out */
              ourstatus->kind = TARGET_WAITKIND_SPURIOUS;
              return wptid;
            }

            if (child_psi.psi_siginfo.si_code != TRAP_CHLD) {
              /* Abnormal situation.. bail out */
              ourstatus->kind = TARGET_WAITKIND_SPURIOUS;
              return wptid;
            }

            child_ptid = ptid_t (child, child_psi.psi_lwpid, 0);
            nbsd_enable_proc_events (child_ptid.pid ());
            ourstatus->value.related_pid = child_ptid;
            break;
          case PTRACE_VFORK_DONE:
            ourstatus->kind = TARGET_WAITKIND_VFORK_DONE;
            if (debug_nbsd_lwp)
              fprintf_unfiltered (gdb_stdlog, "NLWP: reported VFORK_DONE parent=%d child=%d\n", pid, pst.pe_other_pid);
            break;
          case PTRACE_LWP_CREATE:
            wptid = ptid_t (pid, pst.pe_lwp, 0);
            if (in_thread_list (wptid)) {
              /* Newborn reported after attach? */
              ourstatus->kind = TARGET_WAITKIND_SPURIOUS;
              return wptid;
            }
	    if (inferior_ptid.lwp () == 0)
	      thread_change_ptid (inferior_ptid, wptid);
	    else
	      add_thread (wptid);
            ourstatus->kind = TARGET_WAITKIND_THREAD_CREATED;
            if (debug_nbsd_lwp)
              fprintf_unfiltered (gdb_stdlog, "NLWP: created LWP %d\n", pst.pe_lwp);
            break;
          case PTRACE_LWP_EXIT:
            wptid = ptid_t (pid, pst.pe_lwp, 0);
            if (!in_thread_list (wptid)) {
              /* Dead child reported after attach? */
              ourstatus->kind = TARGET_WAITKIND_SPURIOUS;
              return wptid;
            }
            delete_thread (find_thread_ptid (wptid));
            ourstatus->kind = TARGET_WAITKIND_THREAD_EXITED;
            if (debug_nbsd_lwp)
              fprintf_unfiltered (gdb_stdlog, "NLWP: exited LWP %d\n", pst.pe_lwp);
            if (ptrace (PT_CONTINUE, pid, (void *)1, 0) == -1)
              perror_with_name (("ptrace"));
            break;
          }
          break;
        }
        break;
      }

      if (debug_nbsd_lwp)
	fprintf_unfiltered (gdb_stdlog,
		"NLWP: nbsd_wait returned (%d, %ld, %ld)\n",
		wptid.pid (), wptid.lwp (),
		wptid.tid ());
      inferior_ptid = wptid;

    }
    return wptid;
}

/* Target hook for follow_fork.  On entry and at return inferior_ptid is
   the ptid of the followed inferior.  */

int
nbsd_nat_target::follow_fork (int follow_child, int detach_fork)
{
  if (!follow_child && detach_fork)
    {
      struct thread_info *tp = inferior_thread ();
      pid_t child_pid = tp->pending_follow.value.related_pid.pid ();

      /* Breakpoints have already been detached from the child by
	 infrun.c.  */

      if (ptrace (PT_DETACH, child_pid, (PTRACE_TYPE_ARG3)1, 0) == -1)
	perror_with_name (("ptrace"));
    }

  return 0;
}

void
nbsd_nat_target::post_startup_inferior (ptid_t pid)
{
  nbsd_enable_proc_events (pid.pid ());
}

void
nbsd_nat_target::post_attach (int pid)
{
  nbsd_enable_proc_events (pid);
  nbsd_add_threads (pid);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_nbsd_nat;

void
_initialize_nbsd_nat (void)
{
  add_setshow_boolean_cmd ("nbsd-lwp", class_maintenance,
			   &debug_nbsd_lwp, _("\
Set debugging of NetBSD lwp module."), _("\
Show debugging of NetBSD lwp module."), _("\
Enables printf debugging output."),
			   NULL,
			   &show_nbsd_lwp_debug,
			   &setdebuglist, &showdebuglist);
}
