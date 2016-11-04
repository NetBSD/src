/* Native-dependent code for OpenBSD.

   Copyright (C) 2012-2016 Free Software Foundation, Inc.

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
#include "gdb_wait.h"

#include "inf-child.h"
#include "obsd-nat.h"

/* OpenBSD 5.2 and later include rthreads which uses a thread model
   that maps userland threads directly onto kernel threads in a 1:1
   fashion.  */

#ifdef PT_GET_THREAD_FIRST

static char *
obsd_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  if (ptid_get_lwp (ptid) != 0)
    {
      static char buf[64];

      xsnprintf (buf, sizeof buf, "thread %ld", ptid_get_lwp (ptid));
      return buf;
    }

  return normal_pid_to_str (ptid);
}

static void
obsd_update_thread_list (struct target_ops *ops)
{
  pid_t pid = ptid_get_pid (inferior_ptid);
  struct ptrace_thread_state pts;

  prune_threads ();

  if (ptrace (PT_GET_THREAD_FIRST, pid, (caddr_t)&pts, sizeof pts) == -1)
    perror_with_name (("ptrace"));

  while (pts.pts_tid != -1)
    {
      ptid_t ptid = ptid_build (pid, pts.pts_tid, 0);

      if (!in_thread_list (ptid))
	{
	  if (ptid_get_lwp (inferior_ptid) == 0)
	    thread_change_ptid (inferior_ptid, ptid);
	  else
	    add_thread (ptid);
	}

      if (ptrace (PT_GET_THREAD_NEXT, pid, (caddr_t)&pts, sizeof pts) == -1)
	perror_with_name (("ptrace"));
    }
}

static ptid_t
obsd_wait (struct target_ops *ops,
	   ptid_t ptid, struct target_waitstatus *ourstatus, int options)
{
  pid_t pid;
  int status, save_errno;

  do
    {
      set_sigint_trap ();

      do
	{
	  pid = waitpid (ptid_get_pid (ptid), &status, 0);
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
      if (!WIFSTOPPED (status) && pid != ptid_get_pid (inferior_ptid))
	pid = -1;
    }
  while (pid == -1);

  ptid = pid_to_ptid (pid);

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
	  ourstatus->value.related_pid = pid_to_ptid (pe.pe_other_pid);

	  /* Make sure the other end of the fork is stopped too.  */
	  fpid = waitpid (pe.pe_other_pid, &status, 0);
	  if (fpid == -1)
	    perror_with_name (("waitpid"));

	  if (ptrace (PT_GET_PROCESS_STATE, fpid,
		      (caddr_t)&pe, sizeof pe) == -1)
	    perror_with_name (("ptrace"));

	  gdb_assert (pe.pe_report_event == PTRACE_FORK);
	  gdb_assert (pe.pe_other_pid == pid);
	  if (fpid == ptid_get_pid (inferior_ptid))
	    {
	      ourstatus->value.related_pid = pid_to_ptid (pe.pe_other_pid);
	      return pid_to_ptid (fpid);
	    }

	  return pid_to_ptid (pid);
	}

      ptid = ptid_build (pid, pe.pe_tid, 0);
      if (!in_thread_list (ptid))
	{
	  if (ptid_get_lwp (inferior_ptid) == 0)
	    thread_change_ptid (inferior_ptid, ptid);
	  else
	    add_thread (ptid);
	}
    }

  store_waitstatus (ourstatus, status);
  return ptid;
}

void
obsd_add_target (struct target_ops *t)
{
  /* Override some methods to support threads.  */
  t->to_pid_to_str = obsd_pid_to_str;
  t->to_update_thread_list = obsd_update_thread_list;
  t->to_wait = obsd_wait;
  add_target (t);
}

#else

void
obsd_add_target (struct target_ops *t)
{
  add_target (t);
}

#endif /* PT_GET_THREAD_FIRST */
