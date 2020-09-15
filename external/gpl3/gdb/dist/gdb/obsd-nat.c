/* Native-dependent code for OpenBSD.

   Copyright (C) 2012-2020 Free Software Foundation, Inc.

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
#include "gdbthread.h"
#include "inferior.h"
#include "target.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include "gdbsupport/gdb_wait.h"

#include "inf-child.h"
#include "obsd-nat.h"

/* OpenBSD 5.2 and later include rthreads which uses a thread model
   that maps userland threads directly onto kernel threads in a 1:1
   fashion.  */

#ifdef PT_GET_THREAD_FIRST

std::string
obsd_nat_target::pid_to_str (ptid_t ptid)
{
  if (ptid.lwp () != 0)
    return string_printf ("thread %ld", ptid.lwp ());

  return normal_pid_to_str (ptid);
}

void
obsd_nat_target::update_thread_list ()
{
  pid_t pid = inferior_ptid.pid ();
  struct ptrace_thread_state pts;

  prune_threads ();

  if (ptrace (PT_GET_THREAD_FIRST, pid, (caddr_t)&pts, sizeof pts) == -1)
    perror_with_name (("ptrace"));

  while (pts.pts_tid != -1)
    {
      ptid_t ptid = ptid_t (pid, pts.pts_tid, 0);

      if (!in_thread_list (this, ptid))
	{
	  if (inferior_ptid.lwp () == 0)
	    thread_change_ptid (this, inferior_ptid, ptid);
	  else
	    add_thread (this, ptid);
	}

      if (ptrace (PT_GET_THREAD_NEXT, pid, (caddr_t)&pts, sizeof pts) == -1)
	perror_with_name (("ptrace"));
    }
}

ptid_t
obsd_nat_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
		       int options)
{
  pid_t pid;
  int status, save_errno;

  do
    {
      set_sigint_trap ();

      do
	{
	  pid = waitpid (ptid.pid (), &status, 0);
	  save_errno = errno;
	}
      while (pid == -1 && errno == EINTR);

      clear_sigint_trap ();

      if (pid == -1)
	{
	  fprintf_unfiltered (gdb_stderr,
			      _("Child process unexpectedly missing: %s.\n"),
			      safe_strerror (save_errno));

	  /* Claim it exited with unknown signal.  */
	  ourstatus->kind = TARGET_WAITKIND_SIGNALLED;
	  ourstatus->value.sig = GDB_SIGNAL_UNKNOWN;
	  return inferior_ptid;
	}

      /* Ignore terminated detached child processes.  */
      if (!WIFSTOPPED (status) && pid != inferior_ptid.pid ())
	pid = -1;
    }
  while (pid == -1);

  ptid = ptid_t (pid);

  if (WIFSTOPPED (status))
    {
      ptrace_state_t pe;
      pid_t fpid;

      if (ptrace (PT_GET_PROCESS_STATE, pid, (caddr_t)&pe, sizeof pe) == -1)
	perror_with_name (("ptrace"));

      switch (pe.pe_report_event)
	{
	case PTRACE_FORK:
	  ourstatus->kind = TARGET_WAITKIND_FORKED;
	  ourstatus->value.related_pid = ptid_t (pe.pe_other_pid);

	  /* Make sure the other end of the fork is stopped too.  */
	  fpid = waitpid (pe.pe_other_pid, &status, 0);
	  if (fpid == -1)
	    perror_with_name (("waitpid"));

	  if (ptrace (PT_GET_PROCESS_STATE, fpid,
		      (caddr_t)&pe, sizeof pe) == -1)
	    perror_with_name (("ptrace"));

	  gdb_assert (pe.pe_report_event == PTRACE_FORK);
	  gdb_assert (pe.pe_other_pid == pid);
	  if (fpid == inferior_ptid.pid ())
	    {
	      ourstatus->value.related_pid = ptid_t (pe.pe_other_pid);
	      return ptid_t (fpid);
	    }

	  return ptid_t (pid);
	}

      ptid = ptid_t (pid, pe.pe_tid, 0);
      if (!in_thread_list (this, ptid))
	{
	  if (inferior_ptid.lwp () == 0)
	    thread_change_ptid (this, inferior_ptid, ptid);
	  else
	    add_thread (this, ptid);
	}
    }

  store_waitstatus (ourstatus, status);
  return ptid;
}

#endif /* PT_GET_THREAD_FIRST */

#ifdef PT_GET_PROCESS_STATE

void
obsd_nat_target::post_attach (int pid)
{
  ptrace_event_t pe;

  /* Set the initial event mask.  */
  memset (&pe, 0, sizeof pe);
  pe.pe_set_event |= PTRACE_FORK;
  if (ptrace (PT_SET_EVENT_MASK, pid,
	      (PTRACE_TYPE_ARG3)&pe, sizeof pe) == -1)
    perror_with_name (("ptrace"));
}

void
obsd_nat_target::post_startup_inferior (ptid_t pid)
{
  ptrace_event_t pe;

  /* Set the initial event mask.  */
  memset (&pe, 0, sizeof pe);
  pe.pe_set_event |= PTRACE_FORK;
  if (ptrace (PT_SET_EVENT_MASK, pid.pid (),
	      (PTRACE_TYPE_ARG3)&pe, sizeof pe) == -1)
    perror_with_name (("ptrace"));
}

/* Target hook for follow_fork.  On entry and at return inferior_ptid is
   the ptid of the followed inferior.  */

bool
obsd_nat_target::follow_fork (bool follow_child, bool detach_fork)
{
  if (!follow_child)
    {
      struct thread_info *tp = inferior_thread ();
      pid_t child_pid = tp->pending_follow.value.related_pid.pid ();

      /* Breakpoints have already been detached from the child by
	 infrun.c.  */

      if (ptrace (PT_DETACH, child_pid, (PTRACE_TYPE_ARG3)1, 0) == -1)
	perror_with_name (("ptrace"));
    }

  return false;
}

int
obsd_nat_target::insert_fork_catchpoint (int pid)
{
  return 0;
}

int
obsd_nat_target::remove_fork_catchpoint (int pid)
{
  return 0;
}

#endif /* PT_GET_PROCESS_STATE */
