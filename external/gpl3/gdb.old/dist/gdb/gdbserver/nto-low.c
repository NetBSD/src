/* QNX Neutrino specific low level interface, for the remote server
   for GDB.
   Copyright (C) 2009-2017 Free Software Foundation, Inc.

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
#include "gdbthread.h"
#include "nto-low.h"
#include "hostio.h"

#include <limits.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/procfs.h>
#include <sys/auxv.h>
#include <sys/iomgr.h>
#include <sys/neutrino.h>


extern int using_threads;
int using_threads = 1;

const struct target_desc *nto_tdesc;

static void
nto_trace (const char *fmt, ...)
{
  va_list arg_list;

  if (debug_threads == 0)
    return;
  fprintf (stderr, "nto:");
  va_start (arg_list, fmt);
  vfprintf (stderr, fmt, arg_list);
  va_end (arg_list);
}

#define TRACE nto_trace

/* Structure holding neutrino specific information about
   inferior.  */

struct nto_inferior
{
  char nto_procfs_path[PATH_MAX];
  int ctl_fd;
  pid_t pid;
  int exit_signo; /* For tracking exit status.  */
};

static struct nto_inferior nto_inferior;

static void
init_nto_inferior (struct nto_inferior *nto_inferior)
{
  memset (nto_inferior, 0, sizeof (struct nto_inferior));
  nto_inferior->ctl_fd = -1;
  nto_inferior->pid = -1;
}

static void
do_detach (void)
{
  if (nto_inferior.ctl_fd != -1)
    {
      nto_trace ("Closing fd\n");
      close (nto_inferior.ctl_fd);
      init_nto_inferior (&nto_inferior);
    }
}

/* Set current thread. Return 1 on success, 0 otherwise.  */

static int
nto_set_thread (ptid_t ptid)
{
  int res = 0;

  TRACE ("%s pid: %d tid: %ld\n", __func__, ptid_get_pid (ptid),
	 ptid_get_lwp (ptid));
  if (nto_inferior.ctl_fd != -1
      && !ptid_equal (ptid, null_ptid)
      && !ptid_equal (ptid, minus_one_ptid))
    {
      pthread_t tid = ptid_get_lwp (ptid);

      if (EOK == devctl (nto_inferior.ctl_fd, DCMD_PROC_CURTHREAD, &tid,
	  sizeof (tid), 0))
	res = 1;
      else
	TRACE ("%s: Error: failed to set current thread\n", __func__);
    }
  return res;
}

/* This function will determine all alive threads.  Note that we do not list
   dead but unjoined threads even though they are still in the process' thread
   list.  

   NTO_INFERIOR must not be NULL.  */

static void
nto_find_new_threads (struct nto_inferior *nto_inferior)
{
  pthread_t tid;

  TRACE ("%s pid:%d\n", __func__, nto_inferior->pid);

  if (nto_inferior->ctl_fd == -1)
    return;

  for (tid = 1;; ++tid)
    {
      procfs_status status;
      ptid_t ptid;
      int err;

      status.tid = tid;
      err = devctl (nto_inferior->ctl_fd, DCMD_PROC_TIDSTATUS, &status,
		    sizeof (status), 0);

      if (err != EOK || status.tid == 0)
	break;

      /* All threads in between are gone.  */
      while (tid != status.tid || status.state == STATE_DEAD)
	{
	  struct thread_info *ti;

	  ptid = ptid_build (nto_inferior->pid, tid, 0);
	  ti = find_thread_ptid (ptid);
	  if (ti != NULL)
	    {
	      TRACE ("Removing thread %d\n", tid);
	      remove_thread (ti);
	    }
	  if (tid == status.tid)
	    break;
	  ++tid;
	}

      if (status.state != STATE_DEAD)
	{
	  TRACE ("Adding thread %d\n", tid);
	  ptid = ptid_build (nto_inferior->pid, tid, 0);
	  if (!find_thread_ptid (ptid))
	    add_thread (ptid, NULL);
	}
    }
}

/* Given pid, open procfs path.  */

static pid_t
do_attach (pid_t pid)
{
  procfs_status status;
  struct sigevent event;

  if (nto_inferior.ctl_fd != -1)
    {
      close (nto_inferior.ctl_fd);
      init_nto_inferior (&nto_inferior);
    }
  xsnprintf (nto_inferior.nto_procfs_path, PATH_MAX - 1, "/proc/%d/as", pid);
  nto_inferior.ctl_fd = open (nto_inferior.nto_procfs_path, O_RDWR);
  if (nto_inferior.ctl_fd == -1)
    {
      TRACE ("Failed to open %s\n", nto_inferior.nto_procfs_path);
      init_nto_inferior (&nto_inferior);
      return -1;
    }
  if (devctl (nto_inferior.ctl_fd, DCMD_PROC_STOP, &status, sizeof (status), 0)
      != EOK)
    {
      do_detach ();
      return -1;
    }
  nto_inferior.pid = pid;
  /* Define a sigevent for process stopped notification.  */
  event.sigev_notify = SIGEV_SIGNAL_THREAD;
  event.sigev_signo = SIGUSR1;
  event.sigev_code = 0;
  event.sigev_value.sival_ptr = NULL;
  event.sigev_priority = -1;
  devctl (nto_inferior.ctl_fd, DCMD_PROC_EVENT, &event, sizeof (event), 0);

  if (devctl (nto_inferior.ctl_fd, DCMD_PROC_STATUS, &status, sizeof (status),
	      0) == EOK
      && (status.flags & _DEBUG_FLAG_STOPPED))
    {
      ptid_t ptid;
      struct process_info *proc;

      kill (pid, SIGCONT);
      ptid = ptid_build (status.pid, status.tid, 0);
      the_low_target.arch_setup ();
      proc = add_process (status.pid, 1);
      proc->tdesc = nto_tdesc;
      TRACE ("Adding thread: pid=%d tid=%ld\n", status.pid,
	     ptid_get_lwp (ptid));
      nto_find_new_threads (&nto_inferior);
    }
  else
    {
      do_detach ();
      return -1;
    }

  return pid;
}

/* Read or write LEN bytes from/to inferior's MEMADDR memory address
   into gdbservers's MYADDR buffer.  Return number of bytes actually
   transfered.  */

static int
nto_xfer_memory (off_t memaddr, unsigned char *myaddr, int len,
		 int dowrite)
{
  int nbytes = 0;

  if (lseek (nto_inferior.ctl_fd, memaddr, SEEK_SET) == memaddr)
    {
      if (dowrite)
	nbytes = write (nto_inferior.ctl_fd, myaddr, len);
      else
	nbytes = read (nto_inferior.ctl_fd, myaddr, len);
      if (nbytes < 0)
	nbytes = 0;
    }
  if (nbytes == 0)
    {
      int e = errno;
      TRACE ("Error in %s : errno=%d (%s)\n", __func__, e, strerror (e));
    }
  return nbytes;
}

/* Insert or remove breakpoint or watchpoint at address ADDR.
   TYPE can be one of Neutrino breakpoint types.  SIZE must be 0 for
   inserting the point, -1 for removing it.  

   Return 0 on success, 1 otherwise.  */

static int
nto_breakpoint (CORE_ADDR addr, int type, int size)
{
  procfs_break brk;

  brk.type = type;
  brk.addr = addr;
  brk.size = size;
  if (devctl (nto_inferior.ctl_fd, DCMD_PROC_BREAK, &brk, sizeof (brk), 0)
      != EOK)
    return 1;
  return 0;
}

/* Read auxiliary vector from inferior's initial stack into gdbserver's
   MYADDR buffer, up to LEN bytes.  

   Return number of bytes read.  */

static int
nto_read_auxv_from_initial_stack (CORE_ADDR initial_stack,
				  unsigned char *myaddr,
				  unsigned int len)
{
  int data_ofs = 0;
  int anint;
  unsigned int len_read = 0;

  /* Skip over argc, argv and envp... Comment from ldd.c:

     The startup frame is set-up so that we have:
     auxv
     NULL
     ...
     envp2
     envp1 <----- void *frame + (argc + 2) * sizeof(char *)
     NULL
     ...
     argv2
     argv1
     argc  <------ void * frame

     On entry to ldd, frame gives the address of argc on the stack.  */
  if (nto_xfer_memory (initial_stack, (unsigned char *)&anint,
		       sizeof (anint), 0) != sizeof (anint))
    return 0;

  /* Size of pointer is assumed to be 4 bytes (32 bit arch. ) */
  data_ofs += (anint + 2) * sizeof (void *); /* + 2 comes from argc itself and
						NULL terminating pointer in
						argv.  */

  /* Now loop over env table:  */
  while (nto_xfer_memory (initial_stack + data_ofs,
			  (unsigned char *)&anint, sizeof (anint), 0)
	 == sizeof (anint))
    {
      data_ofs += sizeof (anint);
      if (anint == 0)
	break;
    }
  initial_stack += data_ofs;

  memset (myaddr, 0, len);
  while (len_read <= len - sizeof (auxv_t))
    {
      auxv_t *auxv = (auxv_t *)myaddr;

      /* Search backwards until we have read AT_PHDR (num. 3),
	 AT_PHENT (num 4), AT_PHNUM (num 5)  */
      if (nto_xfer_memory (initial_stack, (unsigned char *)auxv,
			   sizeof (auxv_t), 0) == sizeof (auxv_t))
	{
	  if (auxv->a_type != AT_NULL)
	    {
	      auxv++;
	      len_read += sizeof (auxv_t);
	    }
	  if (auxv->a_type == AT_PHNUM) /* That's all we need.  */
	    break;
	  initial_stack += sizeof (auxv_t);
	}
      else
	break;
    }
  TRACE ("auxv: len_read: %d\n", len_read);
  return len_read;
}

/* Start inferior specified by PROGRAM passing arguments ALLARGS.  */

static int
nto_create_inferior (char *program, char **allargs)
{
  struct inheritance inherit;
  pid_t pid;
  sigset_t set;

  TRACE ("%s %s\n", __func__, program);
  /* Clear any pending SIGUSR1's but keep the behavior the same.  */
  signal (SIGUSR1, signal (SIGUSR1, SIG_IGN));

  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);
  sigprocmask (SIG_UNBLOCK, &set, NULL);

  memset (&inherit, 0, sizeof (inherit));
  inherit.flags |= SPAWN_SETGROUP | SPAWN_HOLD;
  inherit.pgroup = SPAWN_NEWPGROUP;
  pid = spawnp (program, 0, NULL, &inherit, allargs, 0);
  sigprocmask (SIG_BLOCK, &set, NULL);

  if (pid == -1)
    return -1;

  if (do_attach (pid) != pid)
    return -1;

  return pid;
}

/* Attach to process PID.  */

static int
nto_attach (unsigned long pid)
{
  TRACE ("%s %ld\n", __func__, pid);
  if (do_attach (pid) != pid)
    error ("Unable to attach to %ld\n", pid);
  return 0;
}

/* Send signal to process PID.  */

static int
nto_kill (int pid)
{
  TRACE ("%s %d\n", __func__, pid);
  kill (pid, SIGKILL);
  do_detach ();
  return 0;
}

/* Detach from process PID.  */

static int
nto_detach (int pid)
{
  TRACE ("%s %d\n", __func__, pid);
  do_detach ();
  return 0;
}

static void
nto_mourn (struct process_info *process)
{
  remove_process (process);
}

/* Check if the given thread is alive.  

   Return 1 if alive, 0 otherwise.  */

static int
nto_thread_alive (ptid_t ptid)
{
  int res;

  TRACE ("%s pid:%d tid:%d\n", __func__, ptid_get_pid (ptid),
	 ptid_get_lwp (ptid));
  if (SignalKill (0, ptid_get_pid (ptid), ptid_get_lwp (ptid),
		  0, 0, 0) == -1)
    res = 0;
  else
    res = 1;
  TRACE ("%s: %s\n", __func__, res ? "yes" : "no");
  return res;
}

/* Resume inferior's execution.  */

static void
nto_resume (struct thread_resume *resume_info, size_t n)
{
  /* We can only work in all-stop mode.  */
  procfs_status status;
  procfs_run run;
  int err;

  TRACE ("%s\n", __func__);
  /* Workaround for aliasing rules violation. */
  sigset_t *run_fault = (sigset_t *) (void *) &run.fault;

  nto_set_thread (resume_info->thread);

  run.flags = _DEBUG_RUN_FAULT | _DEBUG_RUN_TRACE;
  if (resume_info->kind == resume_step)
    run.flags |= _DEBUG_RUN_STEP;
  run.flags |= _DEBUG_RUN_ARM;

  sigemptyset (run_fault);
  sigaddset (run_fault, FLTBPT);
  sigaddset (run_fault, FLTTRACE);
  sigaddset (run_fault, FLTILL);
  sigaddset (run_fault, FLTPRIV);
  sigaddset (run_fault, FLTBOUNDS);
  sigaddset (run_fault, FLTIOVF);
  sigaddset (run_fault, FLTIZDIV);
  sigaddset (run_fault, FLTFPE);
  sigaddset (run_fault, FLTPAGE);
  sigaddset (run_fault, FLTSTACK);
  sigaddset (run_fault, FLTACCESS);

  sigemptyset (&run.trace);
  if (resume_info->sig)
    {
      int signal_to_pass;

      devctl (nto_inferior.ctl_fd, DCMD_PROC_STATUS, &status, sizeof (status),
	      0);
      signal_to_pass = resume_info->sig;
      if (status.why & (_DEBUG_WHY_SIGNALLED | _DEBUG_WHY_FAULTED))
	{
	  if (signal_to_pass != status.info.si_signo)
	    {
	      kill (status.pid, signal_to_pass);
	      run.flags |= _DEBUG_RUN_CLRFLT | _DEBUG_RUN_CLRSIG;
	    }
	  else		/* Let it kill the program without telling us.  */
	    sigdelset (&run.trace, signal_to_pass);
	}
    }
  else
    run.flags |= _DEBUG_RUN_CLRSIG | _DEBUG_RUN_CLRFLT;

  sigfillset (&run.trace);

  regcache_invalidate ();

  err = devctl (nto_inferior.ctl_fd, DCMD_PROC_RUN, &run, sizeof (run), 0);
  if (err != EOK)
    TRACE ("Error: %d \"%s\"\n", err, strerror (err));
}

/* Wait for inferior's event.  

   Return ptid of thread that caused the event.  */

static ptid_t
nto_wait (ptid_t ptid,
	  struct target_waitstatus *ourstatus, int target_options)
{
  sigset_t set;
  siginfo_t info;
  procfs_status status;
  const int trace_mask = (_DEBUG_FLAG_TRACE_EXEC | _DEBUG_FLAG_TRACE_RD
			  | _DEBUG_FLAG_TRACE_WR | _DEBUG_FLAG_TRACE_MODIFY);

  TRACE ("%s\n", __func__);

  ourstatus->kind = TARGET_WAITKIND_SPURIOUS;

  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);

  devctl (nto_inferior.ctl_fd, DCMD_PROC_STATUS, &status, sizeof (status), 0);
  while (!(status.flags & _DEBUG_FLAG_ISTOP))
    {
      sigwaitinfo (&set, &info);
      devctl (nto_inferior.ctl_fd, DCMD_PROC_STATUS, &status, sizeof (status),
	      0);
    }
  nto_find_new_threads (&nto_inferior);

  if (status.flags & _DEBUG_FLAG_SSTEP)
    {
      TRACE ("SSTEP\n");
      ourstatus->kind = TARGET_WAITKIND_STOPPED;
      ourstatus->value.sig = GDB_SIGNAL_TRAP;
    }
  /* Was it a breakpoint?  */
  else if (status.flags & trace_mask)
    {
      TRACE ("STOPPED\n");
      ourstatus->kind = TARGET_WAITKIND_STOPPED;
      ourstatus->value.sig = GDB_SIGNAL_TRAP;
    }
  else if (status.flags & _DEBUG_FLAG_ISTOP)
    {
      TRACE ("ISTOP\n");
      switch (status.why)
	{
	case _DEBUG_WHY_SIGNALLED:
	  TRACE ("  SIGNALLED\n");
	  ourstatus->kind = TARGET_WAITKIND_STOPPED;
	  ourstatus->value.sig =
	    gdb_signal_from_host (status.info.si_signo);
	  nto_inferior.exit_signo = ourstatus->value.sig;
	  break;
	case _DEBUG_WHY_FAULTED:
	  TRACE ("  FAULTED\n");
	  ourstatus->kind = TARGET_WAITKIND_STOPPED;
	  if (status.info.si_signo == SIGTRAP)
	    {
	      ourstatus->value.sig = 0;
	      nto_inferior.exit_signo = 0;
	    }
	  else
	    {
	      ourstatus->value.sig =
		gdb_signal_from_host (status.info.si_signo);
	      nto_inferior.exit_signo = ourstatus->value.sig;
	    }
	  break;

	case _DEBUG_WHY_TERMINATED:
	  {
	    int waitval = 0;

	    TRACE ("  TERMINATED\n");
	    waitpid (ptid_get_pid (ptid), &waitval, WNOHANG);
	    if (nto_inferior.exit_signo)
	      {
		/* Abnormal death.  */
		ourstatus->kind = TARGET_WAITKIND_SIGNALLED;
		ourstatus->value.sig = nto_inferior.exit_signo;
	      }
	    else
	      {
		/* Normal death.  */
		ourstatus->kind = TARGET_WAITKIND_EXITED;
		ourstatus->value.integer = WEXITSTATUS (waitval);
	      }
	    nto_inferior.exit_signo = 0;
	    break;
	  }

	case _DEBUG_WHY_REQUESTED:
	  TRACE ("REQUESTED\n");
	  /* We are assuming a requested stop is due to a SIGINT.  */
	  ourstatus->kind = TARGET_WAITKIND_STOPPED;
	  ourstatus->value.sig = GDB_SIGNAL_INT;
	  nto_inferior.exit_signo = 0;
	  break;
	}
    }

  return ptid_build (status.pid, status.tid, 0);
}

/* Fetch inferior's registers for currently selected thread (CURRENT_INFERIOR).
   If REGNO is -1, fetch all registers, or REGNO register only otherwise.  */

static void
nto_fetch_registers (struct regcache *regcache, int regno)
{
  int regsize;
  procfs_greg greg;
  ptid_t ptid;

  TRACE ("%s (regno=%d)\n", __func__, regno);
  if (regno >= the_low_target.num_regs)
    return;

  if (current_thread == NULL)
    {
      TRACE ("current_thread is NULL\n");
      return;
    }
  ptid = thread_to_gdb_id (current_thread);
  if (!nto_set_thread (ptid))
    return;

  if (devctl (nto_inferior.ctl_fd, DCMD_PROC_GETGREG, &greg, sizeof (greg),
	      &regsize) == EOK)
    {
      if (regno == -1) /* All registers. */
	{
	  for (regno = 0; regno != the_low_target.num_regs; ++regno)
	    {
	      const unsigned int registeroffset
		= the_low_target.register_offset (regno);
	      supply_register (regcache, regno,
			       ((char *)&greg) + registeroffset);
	    }
	}
      else
	{
	  const unsigned int registeroffset
	    = the_low_target.register_offset (regno);
	  if (registeroffset == -1)
	    return;
	  supply_register (regcache, regno, ((char *)&greg) + registeroffset);
	}
    }
  else
    TRACE ("ERROR reading registers from inferior.\n");
}

/* Store registers for currently selected thread (CURRENT_INFERIOR).  
   We always store all registers, regardless of REGNO.  */

static void
nto_store_registers (struct regcache *regcache, int regno)
{
  procfs_greg greg;
  int err;
  ptid_t ptid;

  TRACE ("%s (regno:%d)\n", __func__, regno);

  if (current_thread == NULL)
    {
      TRACE ("current_thread is NULL\n");
      return;
    }
  ptid = thread_to_gdb_id (current_thread);
  if (!nto_set_thread (ptid))
    return;

  memset (&greg, 0, sizeof (greg));
  for  (regno = 0; regno != the_low_target.num_regs; ++regno)
    {
      const unsigned int regoffset
	= the_low_target.register_offset (regno);
      collect_register (regcache, regno, ((char *)&greg) + regoffset);
    }
  err = devctl (nto_inferior.ctl_fd, DCMD_PROC_SETGREG, &greg, sizeof (greg),
		0);
  if (err != EOK)
    TRACE ("Error: setting registers.\n");
}

/* Read LEN bytes from inferior's memory address MEMADDR into
   gdbserver's MYADDR buffer.  

   Return 0 on success -1 otherwise.  */

static int
nto_read_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
{
  TRACE ("%s memaddr:0x%08lx, len:%d\n", __func__, memaddr, len);

  if (nto_xfer_memory (memaddr, myaddr, len, 0) != len)
    {
      TRACE ("Failed to read memory\n");
      return -1;
    }

  return 0;
}

/* Write LEN bytes from gdbserver's buffer MYADDR into inferior's
   memory at address MEMADDR.  

   Return 0 on success -1 otherwise.  */

static int
nto_write_memory (CORE_ADDR memaddr, const unsigned char *myaddr, int len)
{
  int len_written;

  TRACE ("%s memaddr: 0x%08llx len: %d\n", __func__, memaddr, len);
  if ((len_written = nto_xfer_memory (memaddr, (unsigned char *)myaddr, len,
				      1))
      != len)
    {
      TRACE ("Wanted to write: %d but written: %d\n", len, len_written);
      return -1;
    }

  return 0;
}

/* Stop inferior.  We always stop all threads.  */

static void
nto_request_interrupt (void)
{
  TRACE ("%s\n", __func__);
  nto_set_thread (ptid_build (nto_inferior.pid, 1, 0));
  if (EOK != devctl (nto_inferior.ctl_fd, DCMD_PROC_STOP, NULL, 0, 0))
    TRACE ("Error stopping inferior.\n");
}

/* Read auxiliary vector from inferior's memory into gdbserver's buffer
   MYADDR.  We always read whole auxv.  
   
   Return number of bytes stored in MYADDR buffer, 0 if OFFSET > 0
   or -1 on error.  */

static int
nto_read_auxv (CORE_ADDR offset, unsigned char *myaddr, unsigned int len)
{
  int err;
  CORE_ADDR initial_stack;
  procfs_info procinfo;

  TRACE ("%s\n", __func__);
  if (offset > 0)
    return 0;

  err = devctl (nto_inferior.ctl_fd, DCMD_PROC_INFO, &procinfo,
		sizeof procinfo, 0);
  if (err != EOK)
    return -1;

  initial_stack = procinfo.initial_stack;

  return nto_read_auxv_from_initial_stack (initial_stack, myaddr, len);
}

static int
nto_supports_z_point_type (char z_type)
{
  switch (z_type)
    {
    case Z_PACKET_SW_BP:
    case Z_PACKET_HW_BP:
    case Z_PACKET_WRITE_WP:
    case Z_PACKET_READ_WP:
    case Z_PACKET_ACCESS_WP:
      return 1;
    default:
      return 0;
    }
}

/* Insert {break/watch}point at address ADDR.  SIZE is not used.  */

static int
nto_insert_point (enum raw_bkpt_type type, CORE_ADDR addr,
		  int size, struct raw_breakpoint *bp)
{
  int wtype = _DEBUG_BREAK_HW; /* Always request HW.  */

  TRACE ("%s type:%c addr: 0x%08lx len:%d\n", __func__, (int)type, addr, size);
  switch (type)
    {
    case raw_bkpt_type_sw:
      wtype = _DEBUG_BREAK_EXEC;
      break;
    case raw_bkpt_type_hw:
      wtype |= _DEBUG_BREAK_EXEC;
      break;
    case raw_bkpt_type_write_wp:
      wtype |= _DEBUG_BREAK_RW;
      break;
    case raw_bkpt_type_read_wp:
      wtype |= _DEBUG_BREAK_RD;
      break;
    case raw_bkpt_type_access_wp:
      wtype |= _DEBUG_BREAK_RW;
      break;
    default:
      return 1; /* Not supported.  */
    }
  return nto_breakpoint (addr, wtype, 0);
}

/* Remove {break/watch}point at address ADDR.  SIZE is not used.  */

static int
nto_remove_point (enum raw_bkpt_type type, CORE_ADDR addr,
		  int size, struct raw_breakpoint *bp)
{
  int wtype = _DEBUG_BREAK_HW; /* Always request HW.  */

  TRACE ("%s type:%c addr: 0x%08lx len:%d\n", __func__, (int)type, addr, size);
  switch (type)
    {
    case raw_bkpt_type_sw:
      wtype = _DEBUG_BREAK_EXEC;
      break;
    case raw_bkpt_type_hw:
      wtype |= _DEBUG_BREAK_EXEC;
      break;
    case raw_bkpt_type_write_wp:
      wtype |= _DEBUG_BREAK_RW;
      break;
    case raw_bkpt_type_read_wp:
      wtype |= _DEBUG_BREAK_RD;
      break;
    case raw_bkpt_type_access_wp:
      wtype |= _DEBUG_BREAK_RW;
      break;
    default:
      return 1; /* Not supported.  */
    }
  return nto_breakpoint (addr, wtype, -1);
}

/* Check if the reason of stop for current thread (CURRENT_INFERIOR) is
   a watchpoint.

   Return 1 if stopped by watchpoint, 0 otherwise.  */

static int
nto_stopped_by_watchpoint (void)
{
  int ret = 0;

  TRACE ("%s\n", __func__);
  if (nto_inferior.ctl_fd != -1 && current_thread != NULL)
    {
      ptid_t ptid;

      ptid = thread_to_gdb_id (current_thread);
      if (nto_set_thread (ptid))
	{
	  const int watchmask = _DEBUG_FLAG_TRACE_RD | _DEBUG_FLAG_TRACE_WR
				| _DEBUG_FLAG_TRACE_MODIFY;
	  procfs_status status;
	  int err;

	  err = devctl (nto_inferior.ctl_fd, DCMD_PROC_STATUS, &status,
			sizeof (status), 0);
	  if (err == EOK && (status.flags & watchmask))
	    ret = 1;
	}
    }
  TRACE ("%s: %s\n", __func__, ret ? "yes" : "no");
  return ret;
}

/* Get instruction pointer for CURRENT_INFERIOR thread.  

   Return inferior's instruction pointer value, or 0 on error.  */ 

static CORE_ADDR
nto_stopped_data_address (void)
{
  CORE_ADDR ret = (CORE_ADDR)0;

  TRACE ("%s\n", __func__);
  if (nto_inferior.ctl_fd != -1 && current_thread != NULL)
    {
      ptid_t ptid;

      ptid = thread_to_gdb_id (current_thread);

      if (nto_set_thread (ptid))
	{
	  procfs_status status;

	  if (devctl (nto_inferior.ctl_fd, DCMD_PROC_STATUS, &status,
		      sizeof (status), 0) == EOK)
	    ret = status.ip;
	}
    }
  TRACE ("%s: 0x%08lx\n", __func__, ret);
  return ret;
}

/* We do not currently support non-stop.  */

static int
nto_supports_non_stop (void)
{
  TRACE ("%s\n", __func__);
  return 0;
}

/* Implementation of the target_ops method "sw_breakpoint_from_kind".  */

static const gdb_byte *
nto_sw_breakpoint_from_kind (int kind, int *size)
{
  *size = the_low_target.breakpoint_len;
  return the_low_target.breakpoint;
}


static struct target_ops nto_target_ops = {
  nto_create_inferior,
  NULL,  /* post_create_inferior */
  nto_attach,
  nto_kill,
  nto_detach,
  nto_mourn,
  NULL, /* nto_join */
  nto_thread_alive,
  nto_resume,
  nto_wait,
  nto_fetch_registers,
  nto_store_registers,
  NULL, /* prepare_to_access_memory */
  NULL, /* done_accessing_memory */
  nto_read_memory,
  nto_write_memory,
  NULL, /* nto_look_up_symbols */
  nto_request_interrupt,
  nto_read_auxv,
  nto_supports_z_point_type,
  nto_insert_point,
  nto_remove_point,
  NULL, /* stopped_by_sw_breakpoint */
  NULL, /* supports_stopped_by_sw_breakpoint */
  NULL, /* stopped_by_hw_breakpoint */
  NULL, /* supports_stopped_by_hw_breakpoint */
  target_can_do_hardware_single_step,
  nto_stopped_by_watchpoint,
  nto_stopped_data_address,
  NULL, /* nto_read_offsets */
  NULL, /* thread_db_set_tls_address */
  NULL,
  hostio_last_error_from_errno,
  NULL, /* nto_qxfer_osdata */
  NULL, /* xfer_siginfo */
  nto_supports_non_stop,
  NULL, /* async */
  NULL, /* start_non_stop */
  NULL, /* supports_multi_process */
  NULL, /* supports_fork_events */
  NULL, /* supports_vfork_events */
  NULL, /* supports_exec_events */
  NULL, /* handle_new_gdb_connection */
  NULL, /* handle_monitor_command */
  NULL, /* core_of_thread */
  NULL, /* read_loadmap */
  NULL, /* process_qsupported */
  NULL, /* supports_tracepoints */
  NULL, /* read_pc */
  NULL, /* write_pc */
  NULL, /* thread_stopped */
  NULL, /* get_tib_address */
  NULL, /* pause_all */
  NULL, /* unpause_all */
  NULL, /* stabilize_threads */
  NULL, /* install_fast_tracepoint_jump_pad */
  NULL, /* emit_ops */
  NULL, /* supports_disable_randomization */
  NULL, /* get_min_fast_tracepoint_insn_len */
  NULL, /* qxfer_libraries_svr4 */
  NULL, /* support_agent */
  NULL, /* support_btrace */
  NULL, /* enable_btrace */
  NULL, /* disable_btrace */
  NULL, /* read_btrace */
  NULL, /* read_btrace_conf */
  NULL, /* supports_range_stepping */
  NULL, /* pid_to_exec_file */
  NULL, /* multifs_open */
  NULL, /* multifs_unlink */
  NULL, /* multifs_readlink */
  NULL, /* breakpoint_kind_from_pc */
  nto_sw_breakpoint_from_kind,
};


/* Global function called by server.c.  Initializes QNX Neutrino
   gdbserver.  */

void
initialize_low (void)
{
  sigset_t set;

  TRACE ("%s\n", __func__);
  set_target_ops (&nto_target_ops);

  /* We use SIGUSR1 to gain control after we block waiting for a process.
     We use sigwaitevent to wait.  */
  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);
  sigprocmask (SIG_BLOCK, &set, NULL);
}

