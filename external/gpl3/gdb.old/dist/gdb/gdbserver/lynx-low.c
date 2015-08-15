/* Copyright (C) 2009-2014 Free Software Foundation, Inc.

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
#include "target.h"
#include "lynx-low.h"

#include <limits.h>
#include <sys/ptrace.h>
#include <sys/piddef.h> /* Provides PIDGET, TIDGET, BUILDPID, etc.  */
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "gdb_wait.h"
#include <signal.h>
#include "filestuff.h"

int using_threads = 1;

const struct target_desc *lynx_tdesc;

/* Per-process private data.  */

struct process_info_private
{
  /* The PTID obtained from the last wait performed on this process.
     Initialized to null_ptid until the first wait is performed.  */
  ptid_t last_wait_event_ptid;
};

/* Print a debug trace on standard output if debug_threads is set.  */

static void
lynx_debug (char *string, ...)
{
  va_list args;

  if (!debug_threads)
    return;

  va_start (args, string);
  fprintf (stderr, "DEBUG(lynx): ");
  vfprintf (stderr, string, args);
  fprintf (stderr, "\n");
  va_end (args);
}

/* Build a ptid_t given a PID and a LynxOS TID.  */

static ptid_t
lynx_ptid_build (int pid, long tid)
{
  /* brobecker/2010-06-21: It looks like the LWP field in ptids
     should be distinct for each thread (see write_ptid where it
     writes the thread ID from the LWP).  So instead of storing
     the LynxOS tid in the tid field of the ptid, we store it in
     the lwp field.  */
  return ptid_build (pid, tid, 0);
}

/* Return the process ID of the given PTID.

   This function has little reason to exist, it's just a wrapper around
   ptid_get_pid.  But since we have a getter function for the lynxos
   ptid, it feels cleaner to have a getter for the pid as well.  */

static int
lynx_ptid_get_pid (ptid_t ptid)
{
  return ptid_get_pid (ptid);
}

/* Return the LynxOS tid of the given PTID.  */

static long
lynx_ptid_get_tid (ptid_t ptid)
{
  /* See lynx_ptid_build: The LynxOS tid is stored inside the lwp field
     of the ptid.  */
  return ptid_get_lwp (ptid);
}

/* For a given PTID, return the associated PID as known by the LynxOS
   ptrace layer.  */

static int
lynx_ptrace_pid_from_ptid (ptid_t ptid)
{
  return BUILDPID (lynx_ptid_get_pid (ptid), lynx_ptid_get_tid (ptid));
}

/* Return a string image of the ptrace REQUEST number.  */

static char *
ptrace_request_to_str (int request)
{
#define CASE(X) case X: return #X
  switch (request)
    {
      CASE(PTRACE_TRACEME);
      CASE(PTRACE_PEEKTEXT);
      CASE(PTRACE_PEEKDATA);
      CASE(PTRACE_PEEKUSER);
      CASE(PTRACE_POKETEXT);
      CASE(PTRACE_POKEDATA);
      CASE(PTRACE_POKEUSER);
      CASE(PTRACE_CONT);
      CASE(PTRACE_KILL);
      CASE(PTRACE_SINGLESTEP);
      CASE(PTRACE_ATTACH);
      CASE(PTRACE_DETACH);
      CASE(PTRACE_GETREGS);
      CASE(PTRACE_SETREGS);
      CASE(PTRACE_GETFPREGS);
      CASE(PTRACE_SETFPREGS);
      CASE(PTRACE_READDATA);
      CASE(PTRACE_WRITEDATA);
      CASE(PTRACE_READTEXT);
      CASE(PTRACE_WRITETEXT);
      CASE(PTRACE_GETFPAREGS);
      CASE(PTRACE_SETFPAREGS);
      CASE(PTRACE_GETWINDOW);
      CASE(PTRACE_SETWINDOW);
      CASE(PTRACE_SYSCALL);
      CASE(PTRACE_DUMPCORE);
      CASE(PTRACE_SETWRBKPT);
      CASE(PTRACE_SETACBKPT);
      CASE(PTRACE_CLRBKPT);
      CASE(PTRACE_GET_UCODE);
#ifdef PT_READ_GPR
      CASE(PT_READ_GPR);
#endif
#ifdef PT_WRITE_GPR
      CASE(PT_WRITE_GPR);
#endif
#ifdef PT_READ_FPR
      CASE(PT_READ_FPR);
#endif
#ifdef PT_WRITE_FPR
      CASE(PT_WRITE_FPR);
#endif
#ifdef PT_READ_VPR
      CASE(PT_READ_VPR);
#endif
#ifdef PT_WRITE_VPR
      CASE(PT_WRITE_VPR);
#endif
#ifdef PTRACE_PEEKUSP
      CASE(PTRACE_PEEKUSP);
#endif
#ifdef PTRACE_POKEUSP
      CASE(PTRACE_POKEUSP);
#endif
      CASE(PTRACE_PEEKTHREAD);
      CASE(PTRACE_THREADUSER);
      CASE(PTRACE_FPREAD);
      CASE(PTRACE_FPWRITE);
      CASE(PTRACE_SETSIG);
      CASE(PTRACE_CONT_ONE);
      CASE(PTRACE_KILL_ONE);
      CASE(PTRACE_SINGLESTEP_ONE);
      CASE(PTRACE_GETLOADINFO);
      CASE(PTRACE_GETTRACESIG);
#ifdef PTRACE_GETTHREADLIST
      CASE(PTRACE_GETTHREADLIST);
#endif
    }
#undef CASE

  return "<unknown-request>";
}

/* A wrapper around ptrace that allows us to print debug traces of
   ptrace calls if debug traces are activated.  */

static int
lynx_ptrace (int request, ptid_t ptid, int addr, int data, int addr2)
{
  int result;
  const int pid = lynx_ptrace_pid_from_ptid (ptid);
  int saved_errno;

  if (debug_threads)
    fprintf (stderr, "PTRACE (%s, pid=%d(pid=%d, tid=%d), addr=0x%x, "
             "data=0x%x, addr2=0x%x)",
             ptrace_request_to_str (request), pid, PIDGET (pid), TIDGET (pid),
             addr, data, addr2);
  result = ptrace (request, pid, addr, data, addr2);
  saved_errno = errno;
  if (debug_threads)
    fprintf (stderr, " -> %d (=0x%x)\n", result, result);

  errno = saved_errno;
  return result;
}

/* Call add_process with the given parameters, and initializes
   the process' private data.  */

static struct process_info *
lynx_add_process (int pid, int attached)
{
  struct process_info *proc;

  proc = add_process (pid, attached);
  proc->tdesc = lynx_tdesc;
  proc->private = xcalloc (1, sizeof (*proc->private));
  proc->private->last_wait_event_ptid = null_ptid;

  return proc;
}

/* Implement the create_inferior method of the target_ops vector.  */

static int
lynx_create_inferior (char *program, char **allargs)
{
  int pid;

  lynx_debug ("lynx_create_inferior ()");

  pid = fork ();
  if (pid < 0)
    perror_with_name ("fork");

  if (pid == 0)
    {
      int pgrp;

      close_most_fds ();

      /* Switch child to its own process group so that signals won't
         directly affect gdbserver. */
      pgrp = getpid();
      setpgid (0, pgrp);
      ioctl (0, TIOCSPGRP, &pgrp);
      lynx_ptrace (PTRACE_TRACEME, null_ptid, 0, 0, 0);
      execv (program, allargs);
      fprintf (stderr, "Cannot exec %s: %s.\n", program, strerror (errno));
      fflush (stderr);
      _exit (0177);
    }

  lynx_add_process (pid, 0);
  /* Do not add the process thread just yet, as we do not know its tid.
     We will add it later, during the wait for the STOP event corresponding
     to the lynx_ptrace (PTRACE_TRACEME) call above.  */
  return pid;
}

/* Assuming we've just attached to a running inferior whose pid is PID,
   add all threads running in that process.  */

static void
lynx_add_threads_after_attach (int pid)
{
  /* Ugh!  There appears to be no way to get the list of threads
     in the program we just attached to.  So get the list by calling
     the "ps" command.  This is only needed now, as we will then
     keep the thread list up to date thanks to thread creation and
     exit notifications.  */
  FILE *f;
  char buf[256];
  int thread_pid, thread_tid;

  f = popen ("ps atx", "r");
  if (f == NULL)
    perror_with_name ("Cannot get thread list");

  while (fgets (buf, sizeof (buf), f) != NULL)
    if ((sscanf (buf, "%d %d", &thread_pid, &thread_tid) == 2
	 && thread_pid == pid))
    {
      ptid_t thread_ptid = lynx_ptid_build (pid, thread_tid);

      if (!find_thread_ptid (thread_ptid))
	{
	  lynx_debug ("New thread: (pid = %d, tid = %d)",
		      pid, thread_tid);
	  add_thread (thread_ptid, NULL);
	}
    }

  pclose (f);
}

/* Implement the attach target_ops method.  */

static int
lynx_attach (unsigned long pid)
{
  ptid_t ptid = lynx_ptid_build (pid, 0);

  if (lynx_ptrace (PTRACE_ATTACH, ptid, 0, 0, 0) != 0)
    error ("Cannot attach to process %lu: %s (%d)\n", pid,
	   strerror (errno), errno);

  lynx_add_process (pid, 1);
  lynx_add_threads_after_attach (pid);

  return 0;
}

/* Implement the resume target_ops method.  */

static void
lynx_resume (struct thread_resume *resume_info, size_t n)
{
  /* FIXME: Assume for now that n == 1.  */
  ptid_t ptid = resume_info[0].thread;
  const int request = (resume_info[0].kind == resume_step
                       ? PTRACE_SINGLESTEP : PTRACE_CONT);
  const int signal = resume_info[0].sig;

  /* If given a minus_one_ptid, then try using the current_process'
     private->last_wait_event_ptid.  On most LynxOS versions,
     using any of the process' thread works well enough, but
     LynxOS 178 is a little more sensitive, and triggers some
     unexpected signals (Eg SIG61) when we resume the inferior
     using a different thread.  */
  if (ptid_equal (ptid, minus_one_ptid))
    ptid = current_process()->private->last_wait_event_ptid;

  /* The ptid might still be minus_one_ptid; this can happen between
     the moment we create the inferior or attach to a process, and
     the moment we resume its execution for the first time.  It is
     fine to use the current_inferior's ptid in those cases.  */
  if (ptid_equal (ptid, minus_one_ptid))
    ptid = thread_to_gdb_id (current_inferior);

  regcache_invalidate ();

  errno = 0;
  lynx_ptrace (request, ptid, 1, signal, 0);
  if (errno)
    perror_with_name ("ptrace");
}

/* Resume the execution of the given PTID.  */

static void
lynx_continue (ptid_t ptid)
{
  struct thread_resume resume_info;

  resume_info.thread = ptid;
  resume_info.kind = resume_continue;
  resume_info.sig = 0;

  lynx_resume (&resume_info, 1);
}

/* A wrapper around waitpid that handles the various idiosyncrasies
   of LynxOS' waitpid.  */

static int
lynx_waitpid (int pid, int *stat_loc)
{
  int ret = 0;

  while (1)
    {
      ret = waitpid (pid, stat_loc, WNOHANG);
      if (ret < 0)
        {
	  /* An ECHILD error is not indicative of a real problem.
	     It happens for instance while waiting for the inferior
	     to stop after attaching to it.  */
	  if (errno != ECHILD)
	    perror_with_name ("waitpid (WNOHANG)");
	}
      if (ret > 0)
        break;
      /* No event with WNOHANG.  See if there is one with WUNTRACED.  */
      ret = waitpid (pid, stat_loc, WNOHANG | WUNTRACED);
      if (ret < 0)
        {
	  /* An ECHILD error is not indicative of a real problem.
	     It happens for instance while waiting for the inferior
	     to stop after attaching to it.  */
	  if (errno != ECHILD)
	    perror_with_name ("waitpid (WNOHANG|WUNTRACED)");
	}
      if (ret > 0)
        break;
      usleep (1000);
    }
  return ret;
}

/* Implement the wait target_ops method.  */

static ptid_t
lynx_wait_1 (ptid_t ptid, struct target_waitstatus *status, int options)
{
  int pid;
  int ret;
  int wstat;
  ptid_t new_ptid;

  if (ptid_equal (ptid, minus_one_ptid))
    pid = lynx_ptid_get_pid (thread_to_gdb_id (current_inferior));
  else
    pid = BUILDPID (lynx_ptid_get_pid (ptid), lynx_ptid_get_tid (ptid));

retry:

  ret = lynx_waitpid (pid, &wstat);
  new_ptid = lynx_ptid_build (ret, ((union wait *) &wstat)->w_tid);
  find_process_pid (ret)->private->last_wait_event_ptid = new_ptid;

  /* If this is a new thread, then add it now.  The reason why we do
     this here instead of when handling new-thread events is because
     we need to add the thread associated to the "main" thread - even
     for non-threaded applications where the new-thread events are not
     generated.  */
  if (!find_thread_ptid (new_ptid))
    {
      lynx_debug ("New thread: (pid = %d, tid = %d)",
		  lynx_ptid_get_pid (new_ptid), lynx_ptid_get_tid (new_ptid));
      add_thread (new_ptid, NULL);
    }

  if (WIFSTOPPED (wstat))
    {
      status->kind = TARGET_WAITKIND_STOPPED;
      status->value.integer = gdb_signal_from_host (WSTOPSIG (wstat));
      lynx_debug ("process stopped with signal: %d",
                  status->value.integer);
    }
  else if (WIFEXITED (wstat))
    {
      status->kind = TARGET_WAITKIND_EXITED;
      status->value.integer = WEXITSTATUS (wstat);
      lynx_debug ("process exited with code: %d", status->value.integer);
    }
  else if (WIFSIGNALED (wstat))
    {
      status->kind = TARGET_WAITKIND_SIGNALLED;
      status->value.integer = gdb_signal_from_host (WTERMSIG (wstat));
      lynx_debug ("process terminated with code: %d",
                  status->value.integer);
    }
  else
    {
      /* Not sure what happened if we get here, or whether we can
	 in fact get here.  But if we do, handle the event the best
	 we can.  */
      status->kind = TARGET_WAITKIND_STOPPED;
      status->value.integer = gdb_signal_from_host (0);
      lynx_debug ("unknown event ????");
    }

  /* SIGTRAP events are generated for situations other than single-step/
     breakpoint events (Eg. new-thread events).  Handle those other types
     of events, and resume the execution if necessary.  */
  if (status->kind == TARGET_WAITKIND_STOPPED
      && status->value.integer == GDB_SIGNAL_TRAP)
    {
      const int realsig = lynx_ptrace (PTRACE_GETTRACESIG, new_ptid, 0, 0, 0);

      lynx_debug ("(realsig = %d)", realsig);
      switch (realsig)
	{
	  case SIGNEWTHREAD:
	    /* We just added the new thread above.  No need to do anything
	       further.  Just resume the execution again.  */
	    lynx_continue (new_ptid);
	    goto retry;

	  case SIGTHREADEXIT:
	    remove_thread (find_thread_ptid (new_ptid));
	    lynx_continue (new_ptid);
	    goto retry;
	}
    }

  return new_ptid;
}

/* A wrapper around lynx_wait_1 that also prints debug traces when
   such debug traces have been activated.  */

static ptid_t
lynx_wait (ptid_t ptid, struct target_waitstatus *status, int options)
{
  ptid_t new_ptid;

  lynx_debug ("lynx_wait (pid = %d, tid = %ld)",
              lynx_ptid_get_pid (ptid), lynx_ptid_get_tid (ptid));
  new_ptid = lynx_wait_1 (ptid, status, options);
  lynx_debug ("          -> (pid=%d, tid=%ld, status->kind = %d)",
	      lynx_ptid_get_pid (new_ptid), lynx_ptid_get_tid (new_ptid),
	      status->kind);
  return new_ptid;
}

/* Implement the kill target_ops method.  */

static int
lynx_kill (int pid)
{
  ptid_t ptid = lynx_ptid_build (pid, 0);
  struct target_waitstatus status;
  struct process_info *process;

  process = find_process_pid (pid);
  if (process == NULL)
    return -1;

  lynx_ptrace (PTRACE_KILL, ptid, 0, 0, 0);
  lynx_wait (ptid, &status, 0);
  the_target->mourn (process);
  return 0;
}

/* Implement the detach target_ops method.  */

static int
lynx_detach (int pid)
{
  ptid_t ptid = lynx_ptid_build (pid, 0);
  struct process_info *process;

  process = find_process_pid (pid);
  if (process == NULL)
    return -1;

  lynx_ptrace (PTRACE_DETACH, ptid, 0, 0, 0);
  the_target->mourn (process);
  return 0;
}

/* Implement the mourn target_ops method.  */

static void
lynx_mourn (struct process_info *proc)
{
  /* Free our private data.  */
  free (proc->private);
  proc->private = NULL;

  clear_inferiors ();
}

/* Implement the join target_ops method.  */

static void
lynx_join (int pid)
{
  /* The PTRACE_DETACH is sufficient to detach from the process.
     So no need to do anything extra.  */
}

/* Implement the thread_alive target_ops method.  */

static int
lynx_thread_alive (ptid_t ptid)
{
  /* The list of threads is updated at the end of each wait, so it
     should be up to date.  No need to re-fetch it.  */
  return (find_thread_ptid (ptid) != NULL);
}

/* Implement the fetch_registers target_ops method.  */

static void
lynx_fetch_registers (struct regcache *regcache, int regno)
{
  struct lynx_regset_info *regset = lynx_target_regsets;
  ptid_t inferior_ptid = thread_to_gdb_id (current_inferior);

  lynx_debug ("lynx_fetch_registers (regno = %d)", regno);

  while (regset->size >= 0)
    {
      char *buf;
      int res;

      buf = xmalloc (regset->size);
      res = lynx_ptrace (regset->get_request, inferior_ptid, (int) buf, 0, 0);
      if (res < 0)
        perror ("ptrace");
      regset->store_function (regcache, buf);
      free (buf);
      regset++;
    }
}

/* Implement the store_registers target_ops method.  */

static void
lynx_store_registers (struct regcache *regcache, int regno)
{
  struct lynx_regset_info *regset = lynx_target_regsets;
  ptid_t inferior_ptid = thread_to_gdb_id (current_inferior);

  lynx_debug ("lynx_store_registers (regno = %d)", regno);

  while (regset->size >= 0)
    {
      char *buf;
      int res;

      buf = xmalloc (regset->size);
      res = lynx_ptrace (regset->get_request, inferior_ptid, (int) buf, 0, 0);
      if (res == 0)
        {
	  /* Then overlay our cached registers on that.  */
	  regset->fill_function (regcache, buf);
	  /* Only now do we write the register set.  */
	  res = lynx_ptrace (regset->set_request, inferior_ptid, (int) buf,
			     0, 0);
        }
      if (res < 0)
        perror ("ptrace");
      free (buf);
      regset++;
    }
}

/* Implement the read_memory target_ops method.  */

static int
lynx_read_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
{
  /* On LynxOS, memory reads needs to be performed in chunks the size
     of int types, and they should also be aligned accordingly.  */
  int buf;
  const int xfer_size = sizeof (buf);
  CORE_ADDR addr = memaddr & -(CORE_ADDR) xfer_size;
  ptid_t inferior_ptid = thread_to_gdb_id (current_inferior);

  while (addr < memaddr + len)
    {
      int skip = 0;
      int truncate = 0;

      errno = 0;
      if (addr < memaddr)
        skip = memaddr - addr;
      if (addr + xfer_size > memaddr + len)
        truncate = addr + xfer_size - memaddr - len;
      buf = lynx_ptrace (PTRACE_PEEKTEXT, inferior_ptid, addr, 0, 0);
      if (errno)
        return errno;
      memcpy (myaddr + (addr - memaddr) + skip, (gdb_byte *) &buf + skip,
              xfer_size - skip - truncate);
      addr += xfer_size;
    }

  return 0;
}

/* Implement the write_memory target_ops method.  */

static int
lynx_write_memory (CORE_ADDR memaddr, const unsigned char *myaddr, int len)
{
  /* On LynxOS, memory writes needs to be performed in chunks the size
     of int types, and they should also be aligned accordingly.  */
  int buf;
  const int xfer_size = sizeof (buf);
  CORE_ADDR addr = memaddr & -(CORE_ADDR) xfer_size;
  ptid_t inferior_ptid = thread_to_gdb_id (current_inferior);

  while (addr < memaddr + len)
    {
      int skip = 0;
      int truncate = 0;

      if (addr < memaddr)
        skip = memaddr - addr;
      if (addr + xfer_size > memaddr + len)
        truncate = addr + xfer_size - memaddr - len;
      if (skip > 0 || truncate > 0)
        /* We need to read the memory at this address in order to preserve
           the data that we are not overwriting.  */
        lynx_read_memory (addr, (unsigned char *) &buf, xfer_size);
        if (errno)
          return errno;
      memcpy ((gdb_byte *) &buf + skip, myaddr + (addr - memaddr) + skip,
              xfer_size - skip - truncate);
      errno = 0;
      lynx_ptrace (PTRACE_POKETEXT, inferior_ptid, addr, buf, 0);
      if (errno)
        return errno;
      addr += xfer_size;
    }

  return 0;
}

/* Implement the kill_request target_ops method.  */

static void
lynx_request_interrupt (void)
{
  ptid_t inferior_ptid = thread_to_gdb_id (current_inferior);

  kill (lynx_ptid_get_pid (inferior_ptid), SIGINT);
}

/* The LynxOS target_ops vector.  */

static struct target_ops lynx_target_ops = {
  lynx_create_inferior,
  lynx_attach,
  lynx_kill,
  lynx_detach,
  lynx_mourn,
  lynx_join,
  lynx_thread_alive,
  lynx_resume,
  lynx_wait,
  lynx_fetch_registers,
  lynx_store_registers,
  NULL,  /* prepare_to_access_memory */
  NULL,  /* done_accessing_memory */
  lynx_read_memory,
  lynx_write_memory,
  NULL,  /* look_up_symbols */
  lynx_request_interrupt,
  NULL,  /* read_auxv */
  NULL,  /* insert_point */
  NULL,  /* remove_point */
  NULL,  /* stopped_by_watchpoint */
  NULL,  /* stopped_data_address */
  NULL,  /* read_offsets */
  NULL,  /* get_tls_address */
  NULL,  /* qxfer_spu */
  NULL,  /* hostio_last_error */
  NULL,  /* qxfer_osdata */
  NULL,  /* qxfer_siginfo */
  NULL,  /* supports_non_stop */
  NULL,  /* async */
  NULL,  /* start_non_stop */
  NULL,  /* supports_multi_process */
  NULL,  /* handle_monitor_command */
};

void
initialize_low (void)
{
  set_target_ops (&lynx_target_ops);
  the_low_target.arch_setup ();
}

