/* Low level interface for debugging GNU/Linux threads for GDB,
   the GNU debugger.
   Copyright 1998, 1999 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* This module implements the debugging interface of the linuxthreads package
   of the glibc. This package implements a simple clone()-based implementation
   of Posix threads for Linux. To use this module, be sure that you have at
   least the version of the linuxthreads package that holds the support of
   GDB (currently 0.8 included in the glibc-2.0.7).

   Right now, the linuxthreads package does not care of priority scheduling,
   so, neither this module does; In particular, the threads are resumed
   in any order, which could lead to different scheduling than the one
   happening when GDB does not control the execution.

   The latest point is that ptrace(PT_ATTACH, ...) is intrusive in Linux:
   When a process is attached, then the attaching process becomes the current
   parent of the attached process, and the old parent has lost this child.
   If the old parent does a wait[...](), then this child is no longer
   considered by the kernel as a child of the old parent, thus leading to
   results of the call different when the child is attached and when it's not.

   A fix has been submitted to the Linux community to solve this problem,
   which consequences are not visible to the application itself, but on the
   process which may wait() for the completion of the application (mostly,
   it may consider that the application no longer exists (errno == ECHILD),
   although it does, and thus being unable to get the exit status and resource
   usage of the child. If by chance, it is able to wait() for the application
   after it has died (by receiving first a SIGCHILD, and then doing a wait(),
   then the exit status and resource usage may be wrong, because the
   linuxthreads package heavily relies on wait() synchronization to keep
   them correct.  */

#include "defs.h"
#include <sys/types.h> /* for pid_t */
#include <sys/ptrace.h> /* for PT_* flags */
#include "gdb_wait.h" /* for WUNTRACED and __WCLONE flags */
#include <signal.h> /* for struct sigaction and NSIG */
#include <sys/utsname.h>

#include "target.h"
#include "inferior.h"
#include "gdbcore.h"
#include "gdbthread.h"
#include "gdbcmd.h"
#include "breakpoint.h"

#ifndef PT_ATTACH
#define PT_ATTACH	PTRACE_ATTACH
#endif
#ifndef PT_KILL
#define PT_KILL		PTRACE_KILL
#endif
#ifndef PT_READ_U
#define PT_READ_U	PTRACE_PEEKUSR
#endif

#ifdef NSIG
#define LINUXTHREAD_NSIG NSIG
#else
#ifdef _NSIG
#define LINUXTHREAD_NSIG _NSIG
#endif
#endif

extern int child_suppress_run;		/* make inftarg.c non-runnable */
struct target_ops linuxthreads_ops;	/* Forward declaration */
extern struct target_ops child_ops;	/* target vector for inftarg.c */

static CORE_ADDR linuxthreads_handles;	/* array of linuxthreads handles */
static CORE_ADDR linuxthreads_manager;	/* pid of linuxthreads manager thread */
static CORE_ADDR linuxthreads_initial;	/* pid of linuxthreads initial thread */
static CORE_ADDR linuxthreads_debug;	/* linuxthreads internal debug flag */
static CORE_ADDR linuxthreads_num;	/* number of valid handle entries */

static int linuxthreads_max;		/* Maximum number of linuxthreads.
					   Zero if this executable doesn't use
					   threads, or wasn't linked with a
					   debugger-friendly version of the
					   linuxthreads library.  */

static int linuxthreads_sizeof_handle;	/* size of a linuxthreads handle */
static int linuxthreads_offset_descr;	/* h_descr offset of the linuxthreads
					   handle */
static int linuxthreads_offset_pid;	/* p_pid offset of the linuxthreads
					   descr */

static int linuxthreads_manager_pid;	/* manager pid */
static int linuxthreads_initial_pid;	/* initial pid */

/* These variables form a bag of threads with interesting status.  If
   wait_thread (PID) finds that PID stopped for some interesting
   reason (i.e. anything other than stopped with SIGSTOP), then it
   records its status in this queue.  linuxthreads_wait and
   linuxthreads_find_trap extract processes from here.  */
static int *linuxthreads_wait_pid;	/* wait array of pid */
static int *linuxthreads_wait_status;	/* wait array of status */
static int linuxthreads_wait_last;	/* index of last valid elt in
					   linuxthreads_wait_{pid,status} */

static sigset_t linuxthreads_block_mask;  /* sigset without SIGCHLD */

static int linuxthreads_step_pid;	/* current stepped pid */
static int linuxthreads_step_signo;	/* current stepped target signal */
static int linuxthreads_exit_status;	/* exit status of initial thread */

static int linuxthreads_inferior_pid;	/* temporary internal inferior pid */
static int linuxthreads_breakpoint_pid;	/* last pid that hit a breakpoint */
static int linuxthreads_attach_pending;	/* attach command without wait */

static int linuxthreads_breakpoints_inserted;	/* any breakpoints inserted */

/* LinuxThreads uses certain signals for communication between
   processes; we need to tell GDB to pass them through silently to the
   inferior.  The LinuxThreads library has global variables we can
   read containing the relevant signal numbers, but since the signal
   numbers are chosen at run-time, those variables aren't initialized
   until the shared library's constructors have had a chance to run.  */

struct linuxthreads_signal {

  /* The name of the LinuxThreads library variable that contains
     the signal number.  */
  char *var;

  /* True if this variable must exist for us to debug properly.  */
  int required;
  
  /* The variable's address in the inferior, or zero if the
     LinuxThreads library hasn't been loaded into this inferior yet.  */
  CORE_ADDR addr;

  /* The signal number, or zero if we don't know yet (either because
     we haven't found the variable, or it hasn't been initialized).
     This is an actual target signal number that you could pass to
     `kill', not a GDB signal number.  */
  int signal;

  /* GDB's original settings for `stop' and `print' for this signal.
     We restore them when the user selects a different executable.
     Invariant: if sig->signal != 0, then sig->{stop,print} contain
     the original settings.  */
  int stop, print;
};

struct linuxthreads_signal linuxthreads_sig_restart = {
  "__pthread_sig_restart", 1, 0, 0, 0, 0
};
struct linuxthreads_signal linuxthreads_sig_cancel = {
  "__pthread_sig_cancel", 1, 0, 0, 0, 0
};
struct linuxthreads_signal linuxthreads_sig_debug = {
  "__pthread_sig_debug", 0, 0, 0, 0, 0
};

/* Set by thread_db module when it takes over the thread_stratum. 
   In that case we must:
   a) refrain from turning on the debug signal, and
   b) refrain from calling add_thread.  */

int using_thread_db = 0;

/* A table of breakpoint locations, one per PID.  */
static struct linuxthreads_breakpoint {
  CORE_ADDR	pc;	/* PC of breakpoint */
  int		pid;	/* pid of breakpoint */
  int		step;	/* whether the pc has been reached after sstep */
} *linuxthreads_breakpoint_zombie;		/* Zombie breakpoints array */
static int linuxthreads_breakpoint_last;	/* Last zombie breakpoint */

/* linuxthreads_{insert,remove}_breakpoint pass the breakpoint address
   to {insert,remove}_breakpoint via this variable, since
   iterate_active_threads doesn't provide any way to pass values
   through to the worker function.  */
static CORE_ADDR linuxthreads_breakpoint_addr;

#define	REMOVE_BREAKPOINT_ZOMBIE(_i) \
{ \
  if ((_i) < linuxthreads_breakpoint_last) \
    linuxthreads_breakpoint_zombie[(_i)] = \
      linuxthreads_breakpoint_zombie[linuxthreads_breakpoint_last]; \
  linuxthreads_breakpoint_last--; \
}



#ifndef PTRACE_XFER_TYPE
#define PTRACE_XFER_TYPE int
#endif
/* Check to see if the given thread is alive.  */
static int
linuxthreads_thread_alive (pid)
     int pid;
{
  errno = 0;
  return ptrace (PT_READ_U, pid, (PTRACE_ARG3_TYPE)0, 0) >= 0 || errno == 0;
}

/* On detach(), find a SIGTRAP status.  If stop is non-zero, find a
   SIGSTOP one, too.

   Make sure PID is ready to run, and free of interference from our
   efforts to debug it (e.g., pending SIGSTOP or SIGTRAP signals).  If
   STOP is zero, just look for a SIGTRAP.  If STOP is non-zero, look
   for a SIGSTOP, too.  Return non-zero if PID is alive and ready to
   run; return zero if PID is dead.

   PID may or may not be stopped at the moment, and we may or may not
   have waited for it already.  We check the linuxthreads_wait bag in
   case we've already got a status for it.  We may possibly wait for
   it ourselves.

   PID may have signals waiting to be delivered.  If they're caused by
   our efforts to debug it, accept them with wait, but don't pass them
   through to PID.  Do pass all other signals through.  */   
static int
linuxthreads_find_trap (pid, stop)
    int pid;
    int stop;
{
  int i;
  int rpid;
  int status;
  int found_stop = 0;
  int found_trap = 0;

  /* PID may have any number of signals pending.  The kernel will
     report each of them to us via wait, and then it's up to us to
     pass them along to the process via ptrace, if we so choose.

     We need to paw through the whole set until we've found a SIGTRAP
     (or a SIGSTOP, if `stop' is set).  We don't pass the SIGTRAP (or
     SIGSTOP) through, but we do re-send all the others, so PID will
     receive them when we resume it.  */
  int *wstatus = alloca (LINUXTHREAD_NSIG * sizeof (int));
  int last = 0;

  /* Look at the pending status */
  for (i = linuxthreads_wait_last; i >= 0; i--)
    if (linuxthreads_wait_pid[i] == pid)
      {
	status = linuxthreads_wait_status[i];

	/* Delete the i'th member of the table.  Since the table is
	   unordered, we can do this simply by copying the table's
	   last element to the i'th position, and shrinking the table
	   by one element.  */
	if (i < linuxthreads_wait_last)
	  {
	    linuxthreads_wait_status[i] =
	      linuxthreads_wait_status[linuxthreads_wait_last];
	    linuxthreads_wait_pid[i] =
	      linuxthreads_wait_pid[linuxthreads_wait_last];
	  }
	linuxthreads_wait_last--;

	if (!WIFSTOPPED(status)) /* Thread has died */
	  return 0;

	if (WSTOPSIG(status) == SIGTRAP)
	  {
	    if (stop)
	      found_trap = 1;
	    else
	      return 1;
	  }
	else if (WSTOPSIG(status) == SIGSTOP)
	  {
	    if (stop)
	      found_stop = 1;
	  }
	else
	  {
	    wstatus[0] = status;
	    last = 1;
	  }

	break;
      }

  if (stop)
    {
      /* Make sure that we'll find what we're looking for.  */
      if (!found_trap)
	{
	  kill (pid, SIGTRAP);
	}
      if (!found_stop)
	{
	  kill (pid, SIGSTOP);
	}
    }
		      
  /* Catch all status until SIGTRAP and optionally SIGSTOP show up.  */
  for (;;)
    {
      /* resume the child every time... */
      child_resume (pid, 1, TARGET_SIGNAL_0);

      /* loop as long as errno == EINTR:
	 waitpid syscall may be aborted due to GDB receiving a signal. 
	 FIXME: EINTR handling should no longer be necessary here, since
	 we now block SIGCHLD except in an explicit sigsuspend call.  */
      
      for (;;)
	{
	  rpid = waitpid (pid, &status, __WCLONE);
	  if (rpid > 0)
	    {
	      break;
	    }
	  if (errno == EINTR)
	    {
	      continue;
	    }

	  /* There are a few reasons the wait call above may have
	     failed.  If the thread manager dies, its children get
	     reparented, and this interferes with GDB waiting for
	     them, in some cases.  Another possibility is that the
	     initial thread was not cloned, so calling wait with
	     __WCLONE won't find it.  I think neither of these should
	     occur in modern Linux kernels --- they don't seem to in
	     2.0.36.  */
	  rpid = waitpid (pid, &status, 0);
	  if (rpid > 0)
	    {
	      break;
	    }
	  if (errno != EINTR)
	    perror_with_name ("find_trap/waitpid");
	}

      if (!WIFSTOPPED(status)) /* Thread has died */
	return 0;

      if (WSTOPSIG(status) == SIGTRAP)
	if (!stop || found_stop)
	  break;
	else
	  found_trap = 1;
      else if (WSTOPSIG(status) != SIGSTOP)
	wstatus[last++] = status;
      else if (stop)
	{
	  if (found_trap)
	    break;
	  else
	    found_stop = 1;
	}
    }

  /* Resend any other signals we noticed to the thread, to be received
     when we continue it.  */
  while (--last >= 0)
    {
      kill (pid, WSTOPSIG(wstatus[last]));
    }

  return 1;
}

/* Cleanup stub for save_inferior_pid.  */
static void
restore_inferior_pid (void *arg)
{
  int *saved_pid_ptr = arg;
  inferior_pid = *saved_pid_ptr;
  free (arg);
}

/* Register a cleanup to restore the value of inferior_pid.  */
static struct cleanup *
save_inferior_pid (void)
{
  int *saved_pid_ptr;
  
  saved_pid_ptr = xmalloc (sizeof (int));
  *saved_pid_ptr = inferior_pid;
  return make_cleanup (restore_inferior_pid, saved_pid_ptr);
}

static void
sigchld_handler (signo)
    int signo;
{
  /* This handler is used to get an EINTR while doing waitpid()
     when an event is received */
}

/* Have we already collected a wait status for PID in the
   linuxthreads_wait bag?  */
static int
linuxthreads_pending_status (pid)
    int pid;
{
  int i;
  for (i = linuxthreads_wait_last; i >= 0; i--)
    if (linuxthreads_wait_pid[i] == pid)
      return 1;
  return 0;
}


/* Internal linuxthreads signal management */

/* Check in OBJFILE for the variable that holds the number for signal SIG.
   We assume that we've already found other LinuxThreads-ish variables
   in OBJFILE, so we complain if it's required, but not there.
   Return true iff things are okay.  */
static int
find_signal_var (sig, objfile)
     struct linuxthreads_signal *sig;
     struct objfile *objfile;
{
  struct minimal_symbol *ms = lookup_minimal_symbol (sig->var, NULL, objfile);

  if (! ms)
    {
      if (sig->required)
	{
	  fprintf_unfiltered (gdb_stderr,
			      "Unable to find linuxthreads symbol \"%s\"\n",
			      sig->var);
	  return 0;
	}
      else
	{
	  sig->addr = 0;
	  return 1;
	}
    }

  sig->addr = SYMBOL_VALUE_ADDRESS (ms);

  return 1;
}

static int
find_all_signal_vars (objfile)
     struct objfile *objfile;
{
  return (   find_signal_var (&linuxthreads_sig_restart, objfile)
	  && find_signal_var (&linuxthreads_sig_cancel,  objfile)
	  && find_signal_var (&linuxthreads_sig_debug,   objfile));
}

/* A struct complaint isn't appropriate here.  */
static int complained_cannot_determine_thread_signal_number = 0;

/* Check to see if the variable holding the signal number for SIG has
   been initialized yet.  If it has, tell GDB to pass that signal
   through to the inferior silently.  */
static void
check_signal_number (sig)
     struct linuxthreads_signal *sig;
{
  int num;

  if (sig->signal)
    /* We already know this signal number.  */
    return;

  if (! sig->addr)
    /* We don't know the variable's address yet.  */
    return;

  if (target_read_memory (sig->addr, (char *)&num, sizeof (num))
      != 0)
    {
      /* If this happens once, it'll probably happen for all the
	 signals, so only complain once.  */
      if (! complained_cannot_determine_thread_signal_number)
	warning ("Cannot determine thread signal number; "
		 "GDB may report spurious signals.");
      complained_cannot_determine_thread_signal_number = 1;
      return;
    }
  
  if (num == 0)
    /* It hasn't been initialized yet.  */
    return;

  /* We know sig->signal was zero, and is becoming non-zero, so it's
     okay to sample GDB's original settings.  */
  sig->signal = num;
  sig->stop  = signal_stop_update  (target_signal_from_host (num), 0);
  sig->print = signal_print_update (target_signal_from_host (num), 0);
}

void
check_all_signal_numbers ()
{
  /* If this isn't a LinuxThreads program, quit early.  */
  if (! linuxthreads_max)
    return;

  check_signal_number (&linuxthreads_sig_restart);
  check_signal_number (&linuxthreads_sig_cancel);
  check_signal_number (&linuxthreads_sig_debug);

  /* handle linuxthread exit */
  if (linuxthreads_sig_debug.signal
      || linuxthreads_sig_restart.signal)
    {
      struct sigaction sact;

      sact.sa_handler = sigchld_handler;
      sigemptyset(&sact.sa_mask);
      sact.sa_flags = 0;

      if (linuxthreads_sig_debug.signal > 0)
	sigaction(linuxthreads_sig_cancel.signal, &sact, NULL);
      else
	sigaction(linuxthreads_sig_restart.signal, &sact, NULL);
    }
}


/* Restore GDB's original settings for SIG.
   This should only be called when we're no longer sure if we're
   talking to an executable that uses LinuxThreads, so we clear the
   signal number and variable address too.  */
static void
restore_signal (sig)
     struct linuxthreads_signal *sig;
{
  if (! sig->signal)
    return;

  /* We know sig->signal was non-zero, and is becoming zero, so it's
     okay to restore GDB's original settings.  */
  signal_stop_update  (target_signal_from_host (sig->signal), sig->stop);
  signal_print_update (target_signal_from_host (sig->signal), sig->print);

  sig->signal = 0;
  sig->addr = 0;
}


/* Restore GDB's original settings for all LinuxThreads signals.
   This should only be called when we're no longer sure if we're
   talking to an executable that uses LinuxThreads, so we clear the
   signal number and variable address too.  */
static void
restore_all_signals ()
{
  restore_signal (&linuxthreads_sig_restart);
  restore_signal (&linuxthreads_sig_cancel);
  restore_signal (&linuxthreads_sig_debug);

  /* If it happens again, we should complain again.  */
  complained_cannot_determine_thread_signal_number = 0;
}




/* Apply FUNC to the pid of each active thread.  This consults the
   inferior's handle table to find active threads.

   If ALL is non-zero, process all threads.
   If ALL is zero, skip threads with pending status.  */
static void
iterate_active_threads (func, all)
    void (*func)(int);
    int all;
{
  CORE_ADDR descr;
  int pid;
  int i;
  int num;

  read_memory (linuxthreads_num, (char *)&num, sizeof (int));

  for (i = 0; i < linuxthreads_max && num > 0; i++)
    {
      read_memory (linuxthreads_handles +
		   linuxthreads_sizeof_handle * i + linuxthreads_offset_descr,
		   (char *)&descr, sizeof (void *));
      if (descr)
	{
	  num--;
	  read_memory (descr + linuxthreads_offset_pid,
		       (char *)&pid, sizeof (pid_t));
	  if (pid > 0 && pid != linuxthreads_manager_pid
	      && (all || (!linuxthreads_pending_status (pid))))
	    (*func)(pid);
	}
    }
}

/* Insert a thread breakpoint at linuxthreads_breakpoint_addr.
   This is the worker function for linuxthreads_insert_breakpoint,
   which passes it to iterate_active_threads.  */
static void
insert_breakpoint (pid)
    int pid;
{
  int j;

  /* Remove (if any) the positive zombie breakpoint.  */
  for (j = linuxthreads_breakpoint_last; j >= 0; j--)
    if (linuxthreads_breakpoint_zombie[j].pid == pid)
      {
	if ((linuxthreads_breakpoint_zombie[j].pc - DECR_PC_AFTER_BREAK
	     == linuxthreads_breakpoint_addr)
	    && !linuxthreads_breakpoint_zombie[j].step)
	  REMOVE_BREAKPOINT_ZOMBIE(j);
	break;
      }
}

/* Note that we're about to remove a thread breakpoint at
   linuxthreads_breakpoint_addr.

   This is the worker function for linuxthreads_remove_breakpoint,
   which passes it to iterate_active_threads.  The actual work of
   overwriting the breakpoint instruction is done by
   child_ops.to_remove_breakpoint; here, we simply create a zombie
   breakpoint if the thread's PC is pointing at the breakpoint being
   removed.  */
static void
remove_breakpoint (pid)
    int pid;
{
  int j;

  /* Insert a positive zombie breakpoint (if needed).  */
  for (j = 0; j <= linuxthreads_breakpoint_last; j++)
    if (linuxthreads_breakpoint_zombie[j].pid == pid)
      break;

  if (in_thread_list (pid) && linuxthreads_thread_alive (pid))
    {
      CORE_ADDR pc = read_pc_pid (pid);
      if (linuxthreads_breakpoint_addr == pc - DECR_PC_AFTER_BREAK
	  && j > linuxthreads_breakpoint_last)
	{
	  linuxthreads_breakpoint_zombie[j].pid = pid;
	  linuxthreads_breakpoint_zombie[j].pc = pc;
	  linuxthreads_breakpoint_zombie[j].step = 0;
	  linuxthreads_breakpoint_last++;
	}
    }
}

/* Kill a thread */
static void
kill_thread (pid)
    int pid;
{
  if (in_thread_list (pid))
    {
      ptrace (PT_KILL, pid, (PTRACE_ARG3_TYPE) 0, 0);
    }
  else
    {
      kill (pid, SIGKILL);
    }
}

/* Resume a thread */
static void
resume_thread (pid)
    int pid;
{
  if (pid != inferior_pid
      && in_thread_list (pid)
      && linuxthreads_thread_alive (pid))
    {
      if (pid == linuxthreads_step_pid)
	{
	  child_resume (pid, 1, linuxthreads_step_signo);
	}
      else
	{
	  child_resume (pid, 0, TARGET_SIGNAL_0);
	}
    }
}

/* Detach a thread */
static void
detach_thread (pid)
    int pid;
{
  if (in_thread_list (pid) && linuxthreads_thread_alive (pid))
    {
      /* Remove pending SIGTRAP and SIGSTOP */
      linuxthreads_find_trap (pid, 1);

      inferior_pid = pid;
      detach (TARGET_SIGNAL_0);
      inferior_pid = linuxthreads_manager_pid;
    }
}

/* Attach a thread */
void
attach_thread (pid)
     int pid;
{
  if (ptrace (PT_ATTACH, pid, (PTRACE_ARG3_TYPE) 0, 0) != 0)
    perror_with_name ("attach_thread");
}

/* Stop a thread */
static void
stop_thread (pid)
    int pid;
{
  if (pid != inferior_pid)
    {
      if (in_thread_list (pid))
	{
	  kill (pid, SIGSTOP);
	}
      else if (ptrace (PT_ATTACH, pid, (PTRACE_ARG3_TYPE) 0, 0) == 0)
	{
	  if (!linuxthreads_attach_pending)
	    printf_filtered ("[New %s]\n", target_pid_to_str (pid));
	  add_thread (pid);
	  if (linuxthreads_sig_debug.signal)
	    {
	      /* After a new thread in glibc 2.1 signals gdb its existence,
		 it suspends itself and wait for linuxthreads_sig_restart,
		 now we can wake it up. */
	      kill (pid, linuxthreads_sig_restart.signal);
	    }
	}
      else
	perror_with_name ("ptrace in stop_thread");
    }
}

/* Wait for a thread */
static void
wait_thread (pid)
    int pid;
{
  int status;
  int rpid;

  if (pid != inferior_pid && in_thread_list (pid))
    {
      /* loop as long as errno == EINTR:
	 waitpid syscall may be aborted if GDB receives a signal. 
	 FIXME: EINTR handling should no longer be necessary here, since
	 we now block SIGCHLD except during an explicit sigsuspend call. */
      for (;;)
	{
	  /* Get first pid status.  */
	  rpid = waitpid(pid, &status, __WCLONE);
	  if (rpid > 0)
	    {
	      break;
	    }
	  if (errno == EINTR)
	    {
	      continue;
	    }

	  /* There are two reasons this might have failed:

	     1) PID is the initial thread, which wasn't cloned, so
	     passing the __WCLONE flag to waitpid prevented us from
	     finding it.

	     2) The manager thread is the parent of all but the
	     initial thread; if it dies, the children will all be
	     reparented to init, which will wait for them.  This means
	     our call to waitpid won't find them.

	     Actually, based on a casual look at the 2.0.36 kernel
	     code, I don't think either of these cases happen.  But I
	     don't have things set up for remotely debugging the
	     kernel, so I'm not sure.  And perhaps older kernels
	     didn't work.  */
	  rpid = waitpid(pid, &status, 0);
	  if (rpid > 0)
	    {
	      break;
	    }
	  if (errno != EINTR && linuxthreads_thread_alive (pid))
	    perror_with_name ("wait_thread/waitpid");

	  /* the thread is dead.  */
	  return;
	}
      if (!WIFSTOPPED(status) || WSTOPSIG(status) != SIGSTOP)
	{
	  linuxthreads_wait_pid[++linuxthreads_wait_last] = pid;
	  linuxthreads_wait_status[linuxthreads_wait_last] = status;
	}
    }
}

/* Walk through the linuxthreads handles in order to detect all
   threads and stop them */
static void
update_stop_threads (test_pid)
    int test_pid;
{
  struct cleanup *old_chain = NULL;

  check_all_signal_numbers ();

  if (linuxthreads_manager_pid == 0)
    {
      if (linuxthreads_manager)
	{
	  if (test_pid > 0 && test_pid != inferior_pid)
	    {
	      old_chain = save_inferior_pid ();
	      inferior_pid = test_pid;
	    }
	  read_memory (linuxthreads_manager,
		       (char *)&linuxthreads_manager_pid, sizeof (pid_t));
	}
      if (linuxthreads_initial)
	{
	  if (test_pid > 0 && test_pid != inferior_pid)
	    {
	      old_chain = save_inferior_pid ();
	      inferior_pid = test_pid;
	    }
	  read_memory(linuxthreads_initial,
		      (char *)&linuxthreads_initial_pid, sizeof (pid_t));
	}
    }

  if (linuxthreads_manager_pid != 0)
    {
      if (old_chain == NULL && test_pid > 0 &&
	  test_pid != inferior_pid && linuxthreads_thread_alive (test_pid))
	{
	  old_chain = save_inferior_pid ();
	  inferior_pid = test_pid;
	}

      if (linuxthreads_thread_alive (inferior_pid))
	{
	  if (test_pid > 0)
	    {
	      if (test_pid != linuxthreads_manager_pid
		  && !linuxthreads_pending_status (linuxthreads_manager_pid))
		{
		  stop_thread (linuxthreads_manager_pid);
		  wait_thread (linuxthreads_manager_pid);
		}
	      if (!in_thread_list (test_pid))
	        {
		  if (!linuxthreads_attach_pending)
		    printf_filtered ("[New %s]\n",
				     target_pid_to_str (test_pid));
		  add_thread (test_pid);
		  if (linuxthreads_sig_debug.signal
		      && inferior_pid == test_pid)
		    {
		      /* After a new thread in glibc 2.1 signals gdb its
			 existence, it suspends itself and wait for
			 linuxthreads_sig_restart, now we can wake it up. */
		      kill (test_pid, linuxthreads_sig_restart.signal);
		    }
		}
	    }
	  iterate_active_threads (stop_thread, 0);
	  iterate_active_threads (wait_thread, 0);
	}
    }

  if (old_chain != NULL)
    do_cleanups (old_chain);
}

/* This routine is called whenever a new symbol table is read in, or
   when all symbol tables are removed.  linux-thread event handling
   can only be initialized when we find the right variables in
   libpthread.so.  Since it's a shared library, those variables don't
   show up until the library gets mapped and the symbol table is read
   in.  */

/* This new_objfile event is now managed by a chained function pointer. 
 * It is the callee's responsability to call the next client on the chain.
 */

/* Saved pointer to previous owner of the new_objfile event. */
static void (*target_new_objfile_chain) PARAMS ((struct objfile *));

void
linuxthreads_new_objfile (objfile)
    struct objfile *objfile;
{
  struct minimal_symbol *ms;

  /* Call predecessor on chain, if any.
     Calling the new module first allows it to dominate, 
     if it finds its compatible libraries.  */

  if (target_new_objfile_chain)
    target_new_objfile_chain (objfile);

  if (!objfile)
    {
      /* We're starting an entirely new executable, so we can no
	 longer be sure that it uses LinuxThreads.  Restore the signal
	 flags to their original states.  */
      restore_all_signals ();

      /* Indicate that we don't know anything's address any more.  */
      linuxthreads_max = 0;

      goto quit;
    }

  /* If we've already found our variables in another objfile, don't
     bother looking for them again.  */
  if (linuxthreads_max)
    goto quit;

  if (! lookup_minimal_symbol ("__pthread_initial_thread", NULL, objfile))
    /* This object file isn't the pthreads library.  */
    goto quit;

  if ((ms = lookup_minimal_symbol ("__pthread_threads_debug",
				   NULL, objfile)) == NULL)
    {
      /* The debugging-aware libpthreads is not present in this objfile */
      warning ("\
This program seems to use POSIX threads, but the thread library used\n\
does not support debugging.  This may make using GDB difficult.  Don't\n\
set breakpoints or single-step through code that might be executed by\n\
any thread other than the main thread.");
      goto quit;
    }
  linuxthreads_debug = SYMBOL_VALUE_ADDRESS (ms);

  /* Read internal structures configuration */
  if ((ms = lookup_minimal_symbol ("__pthread_sizeof_handle",
				   NULL, objfile)) == NULL
      || target_read_memory (SYMBOL_VALUE_ADDRESS (ms),
			     (char *)&linuxthreads_sizeof_handle,
			     sizeof (linuxthreads_sizeof_handle)) != 0)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Unable to find linuxthreads symbol \"%s\"\n",
			  "__pthread_sizeof_handle");
      goto quit;
    }

  if ((ms = lookup_minimal_symbol ("__pthread_offsetof_descr",
				   NULL, objfile)) == NULL
      || target_read_memory (SYMBOL_VALUE_ADDRESS (ms),
			     (char *)&linuxthreads_offset_descr,
			     sizeof (linuxthreads_offset_descr)) != 0)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Unable to find linuxthreads symbol \"%s\"\n",
			  "__pthread_offsetof_descr");
      goto quit;
    }
	 
  if ((ms = lookup_minimal_symbol ("__pthread_offsetof_pid",
				   NULL, objfile)) == NULL
      || target_read_memory (SYMBOL_VALUE_ADDRESS (ms),
			     (char *)&linuxthreads_offset_pid,
			     sizeof (linuxthreads_offset_pid)) != 0)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Unable to find linuxthreads symbol \"%s\"\n",
			  "__pthread_offsetof_pid");
      goto quit;
    }

  if (! find_all_signal_vars (objfile))
    goto quit;

  /* Read adresses of internal structures to access */
  if ((ms = lookup_minimal_symbol ("__pthread_handles",
				   NULL, objfile)) == NULL)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Unable to find linuxthreads symbol \"%s\"\n",
			  "__pthread_handles");
      goto quit;
    }
  linuxthreads_handles = SYMBOL_VALUE_ADDRESS (ms);

  if ((ms = lookup_minimal_symbol ("__pthread_handles_num",
				   NULL, objfile)) == NULL)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Unable to find linuxthreads symbol \"%s\"\n",
			  "__pthread_handles_num");
      goto quit;
    }
  linuxthreads_num = SYMBOL_VALUE_ADDRESS (ms);

  if ((ms = lookup_minimal_symbol ("__pthread_manager_thread",
				   NULL, objfile)) == NULL)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Unable to find linuxthreads symbol \"%s\"\n",
			  "__pthread_manager_thread");
      goto quit;
    }
  linuxthreads_manager = SYMBOL_VALUE_ADDRESS (ms) + linuxthreads_offset_pid;

  if ((ms = lookup_minimal_symbol ("__pthread_initial_thread",
				   NULL, objfile)) == NULL)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Unable to find linuxthreads symbol \"%s\"\n",
			  "__pthread_initial_thread");
      goto quit;
    }
  linuxthreads_initial = SYMBOL_VALUE_ADDRESS (ms) + linuxthreads_offset_pid;

  /* Search for this last, so it won't be set to a non-zero value unless
     we successfully found all the symbols above.  */
  if ((ms = lookup_minimal_symbol ("__pthread_threads_max",
				   NULL, objfile)) == NULL
      || target_read_memory (SYMBOL_VALUE_ADDRESS (ms),
			     (char *)&linuxthreads_max,
			     sizeof (linuxthreads_max)) != 0)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Unable to find linuxthreads symbol \"%s\"\n",
			  "__pthread_threads_max");
      goto quit;
    }

  /* Allocate gdb internal structures */
  linuxthreads_wait_pid = 
    (int *) xmalloc (sizeof (int) * (linuxthreads_max + 1));
  linuxthreads_wait_status =
    (int *) xmalloc (sizeof (int) * (linuxthreads_max + 1));
  linuxthreads_breakpoint_zombie = (struct linuxthreads_breakpoint *)
    xmalloc (sizeof (struct linuxthreads_breakpoint) * (linuxthreads_max + 1));

  if (inferior_pid && 
      !linuxthreads_attach_pending && 
      !using_thread_db)		/* suppressed by thread_db module */
    {
      int on = 1;

      target_write_memory (linuxthreads_debug, (char *)&on, sizeof (on));
      linuxthreads_attach_pending = 1;
      update_stop_threads (inferior_pid);
      linuxthreads_attach_pending = 0;
    }

  check_all_signal_numbers ();

quit:
}

/* If we have switched threads from a one that stopped at breakpoint,
   return 1 otherwise 0.  */

int
linuxthreads_prepare_to_proceed (step)
    int step;
{
  if (!linuxthreads_max
      || !linuxthreads_manager_pid
      || !linuxthreads_breakpoint_pid
      || !breakpoint_here_p (read_pc_pid (linuxthreads_breakpoint_pid)))
    return 0;

  if (step)
    {
      /* Mark the current inferior as single stepping process.  */
      linuxthreads_step_pid = inferior_pid;
    }

  linuxthreads_inferior_pid = linuxthreads_breakpoint_pid;
  return linuxthreads_breakpoint_pid;
}

/* Convert a pid to printable form. */

char *
linuxthreads_pid_to_str (pid)
    int pid;
{
  static char buf[100];

  sprintf (buf, "%s %d%s", linuxthreads_max ? "Thread" : "Pid", pid,
	   (pid == linuxthreads_manager_pid) ? " (manager thread)"
	   : (pid == linuxthreads_initial_pid) ? " (initial thread)"
	   : "");

  return buf;
}

/* Attach to process PID, then initialize for debugging it
   and wait for the trace-trap that results from attaching.  */

static void
linuxthreads_attach (args, from_tty)
    char *args;
    int from_tty;
{
  if (!args)
    error_no_arg ("process-id to attach");

  push_target (&linuxthreads_ops);
  linuxthreads_breakpoints_inserted = 1;
  linuxthreads_breakpoint_last = -1;
  linuxthreads_wait_last = -1;
  WSETSTOP (linuxthreads_exit_status, 0);

  child_ops.to_attach (args, from_tty);

  if (linuxthreads_max)
    linuxthreads_attach_pending = 1;
}

/* Take a program previously attached to and detaches it.
   The program resumes execution and will no longer stop
   on signals, etc.  We'd better not have left any breakpoints
   in the program or it'll die when it hits one.  For this
   to work, it may be necessary for the process to have been
   previously attached.  It *might* work if the program was
   started via the normal ptrace (PTRACE_TRACEME).  */

static void
linuxthreads_detach (args, from_tty)
    char *args;
    int from_tty;
{
  if (linuxthreads_max)
    {
      int i;
      int pid;
      int off = 0;
      target_write_memory (linuxthreads_debug, (char *)&off, sizeof (off));

      /* Walk through linuxthreads array in order to detach known threads.  */
      if (linuxthreads_manager_pid != 0)
	{
	  /* Get rid of all positive zombie breakpoints.  */
	  for (i = 0; i <= linuxthreads_breakpoint_last; i++)
	    {
	      if (linuxthreads_breakpoint_zombie[i].step)
		continue;

	      pid = linuxthreads_breakpoint_zombie[i].pid;
	      if (!linuxthreads_thread_alive (pid))
		continue;

	      if (linuxthreads_breakpoint_zombie[i].pc != read_pc_pid (pid))
		continue;

	      /* Continue in STEP mode until the thread pc has moved or
		 until SIGTRAP is found on the same PC.  */
	      if (linuxthreads_find_trap (pid, 0)
		  && linuxthreads_breakpoint_zombie[i].pc == read_pc_pid (pid))
		write_pc_pid (linuxthreads_breakpoint_zombie[i].pc
			      - DECR_PC_AFTER_BREAK, pid);
	    }

	  /* Detach thread after thread.  */
	  inferior_pid = linuxthreads_manager_pid;
	  iterate_active_threads (detach_thread, 1);

	  /* Remove pending SIGTRAP and SIGSTOP */
	  linuxthreads_find_trap (inferior_pid, 1);

	  linuxthreads_wait_last = -1;
	  WSETSTOP (linuxthreads_exit_status, 0);
	}

      linuxthreads_inferior_pid = 0;
      linuxthreads_breakpoint_pid = 0;
      linuxthreads_step_pid = 0;
      linuxthreads_step_signo = TARGET_SIGNAL_0;
      linuxthreads_manager_pid = 0;
      linuxthreads_initial_pid = 0;
      linuxthreads_attach_pending = 0;
      init_thread_list ();           /* Destroy thread info */
    }

  child_ops.to_detach (args, from_tty);

  unpush_target (&linuxthreads_ops);
}

/* Resume execution of process PID.  If STEP is nozero, then
   just single step it.  If SIGNAL is nonzero, restart it with that
   signal activated.  */

static void
linuxthreads_resume (pid, step, signo)
    int pid;
    int step;
    enum target_signal signo;
{
  if (!linuxthreads_max || stop_soon_quietly || linuxthreads_manager_pid == 0)
    {
      child_ops.to_resume (pid, step, signo);
    }
  else
    {
      int rpid;
      if (linuxthreads_inferior_pid)
	{
	  /* Prepare resume of the last thread that hit a breakpoint */
	  linuxthreads_breakpoints_inserted = 0;
	  rpid = linuxthreads_inferior_pid;
	  linuxthreads_step_signo = signo;
	}
      else
        {
	  struct cleanup *old_chain = NULL;
	  int i;

	  if (pid < 0)
	    {
	      linuxthreads_step_pid = step ? inferior_pid : 0;
	      linuxthreads_step_signo = signo;
	      rpid = inferior_pid;
	    }
	  else
	    rpid = pid;

	  if (pid < 0 || !step)
	    {
	      linuxthreads_breakpoints_inserted = 1;

	      /* Walk through linuxthreads array in order to resume threads */
	      if (pid >= 0 && inferior_pid != pid)
		{
		  old_chain = save_inferior_pid ();
		  inferior_pid = pid;
		}

	      iterate_active_threads (resume_thread, 0);
	      if (linuxthreads_manager_pid != inferior_pid
		  && !linuxthreads_pending_status (linuxthreads_manager_pid))
		resume_thread (linuxthreads_manager_pid);
	    }
	  else
	    linuxthreads_breakpoints_inserted = 0;

	  /* Deal with zombie breakpoint */
	  for (i = 0; i <= linuxthreads_breakpoint_last; i++)
	    if (linuxthreads_breakpoint_zombie[i].pid == rpid)
	      {
		if (linuxthreads_breakpoint_zombie[i].pc != read_pc_pid (rpid))
		  {
		    /* The current pc is out of zombie breakpoint.  */
		    REMOVE_BREAKPOINT_ZOMBIE(i);
		  }
		break;
	      }

	  if (old_chain != NULL)
	    do_cleanups (old_chain);
	}

      /* Resume initial thread. */
      /* [unles it has a wait event pending] */
      if (!linuxthreads_pending_status (rpid))
	{
	  child_ops.to_resume (rpid, step, signo);
	}
    }
}

/* Abstract out the child_wait functionality.  */
int
linux_child_wait (pid, rpid, status)
     int pid;
     int *rpid;
     int *status;
{
  int save_errno;

  /* Note: inftarg has these inside the loop. */
  set_sigint_trap ();	/* Causes SIGINT to be passed on to the
			   attached process. */
  set_sigio_trap  ();

  errno = save_errno = 0;
  for (;;)
    {
      errno = 0;
      *rpid = waitpid (pid, status, __WCLONE | WNOHANG);
      save_errno = errno;

      if (*rpid > 0)
	{
	  /* Got an event -- break out */
	  break;
	}
      if (errno == EINTR)	/* interrupted by signal, try again */
	{
	  continue;
	}

      errno = 0;
      *rpid = waitpid (pid, status, WNOHANG);
      if (*rpid > 0)
	{
	  /* Got an event -- break out */
	  break;
	}
      if (errno == EINTR)
	{
	  continue;
	}
      if (errno != 0 && save_errno != 0)
	{
	  break;
	}
      sigsuspend(&linuxthreads_block_mask);
    }
  clear_sigio_trap  ();
  clear_sigint_trap ();

  return errno ? errno : save_errno;
}


/* Wait for any threads to stop.  We may have to convert PID from a thread id
   to a LWP id, and vice versa on the way out.  */

static int
linuxthreads_wait (pid, ourstatus)
     int pid;
     struct target_waitstatus *ourstatus;
{
  int status;
  int rpid;
  int i;
  int last;
  int *wstatus;

  if (linuxthreads_max && !linuxthreads_breakpoints_inserted)
    wstatus = alloca (LINUXTHREAD_NSIG * sizeof (int));

  /* See if the inferior has chosen values for its signals yet.  By
     checking for them here, we can be sure we've updated GDB's signal
     handling table before the inferior ever gets one of them.  (Well,
     before we notice, anyway.)  */
  check_all_signal_numbers ();

  for (;;)
    {
      if (!linuxthreads_max)
	  rpid = 0;
      else if (!linuxthreads_breakpoints_inserted)
	{
	  if (linuxthreads_inferior_pid)
	    pid = linuxthreads_inferior_pid;
	  else if (pid < 0)
	    pid = inferior_pid;
	  last = rpid = 0;
	}
      else if (pid < 0 && linuxthreads_wait_last >= 0)
	{
	  status = linuxthreads_wait_status[linuxthreads_wait_last];
	  rpid = linuxthreads_wait_pid[linuxthreads_wait_last--];
        }
      else if (pid > 0 && linuxthreads_pending_status (pid))
	{
	  for (i = linuxthreads_wait_last; i >= 0; i--)
	    if (linuxthreads_wait_pid[i] == pid)
		break;
	  if (i < 0)
	    rpid = 0;
	  else
	    {
	      status = linuxthreads_wait_status[i];
	      rpid = pid;
	      if (i < linuxthreads_wait_last)
		{
		  linuxthreads_wait_status[i] =
		    linuxthreads_wait_status[linuxthreads_wait_last];
		  linuxthreads_wait_pid[i] =
		    linuxthreads_wait_pid[linuxthreads_wait_last];
		}
	      linuxthreads_wait_last--;
	    }
	}
      else
	  rpid = 0;

      if (rpid == 0)
	{
	  int save_errno;

	  save_errno = linux_child_wait (pid, &rpid, &status);

	  if (rpid == -1)
	    {
	      if (WIFEXITED(linuxthreads_exit_status))
		{
		  store_waitstatus (ourstatus, linuxthreads_exit_status);
		  return inferior_pid;
		}
	      else
		{
		  fprintf_unfiltered
		      (gdb_stderr, "Child process unexpectedly missing: %s.\n",
		       safe_strerror (save_errno));
		  /* Claim it exited with unknown signal.  */
		  ourstatus->kind = TARGET_WAITKIND_SIGNALLED;
		  ourstatus->value.sig = TARGET_SIGNAL_UNKNOWN;
		  return -1;
		}
	    }

	  /* We have now gotten a new event from waitpid above. */

	  /* Signals arrive in any order.  So get all signals until
	     SIGTRAP and resend previous ones to be held after.  */
	  if (linuxthreads_max
	      && !linuxthreads_breakpoints_inserted
	      && WIFSTOPPED(status))
	    if (WSTOPSIG(status) == SIGTRAP)
	      {
		while (--last >= 0)
		  {
		    kill (rpid, WSTOPSIG(wstatus[last]));
		  }

		/* insert negative zombie breakpoint */
		for (i = 0; i <= linuxthreads_breakpoint_last; i++)
		  if (linuxthreads_breakpoint_zombie[i].pid == rpid)
		      break;
		if (i > linuxthreads_breakpoint_last)
		  {
		    linuxthreads_breakpoint_zombie[i].pid = rpid;
		    linuxthreads_breakpoint_last++;
		  }
		linuxthreads_breakpoint_zombie[i].pc = read_pc_pid (rpid);
		linuxthreads_breakpoint_zombie[i].step = 1;
	      }
	    else
	      {
		if (WSTOPSIG(status) != SIGSTOP)
		  {
		    for (i = 0; i < last; i++)
		      if (wstatus[i] == status)
			break;
		    if (i >= last)
		      {
			wstatus[last++] = status;
		      }
		  }
		child_resume (rpid, 1, TARGET_SIGNAL_0);
		continue;
	      }
	  if (linuxthreads_inferior_pid)
	    linuxthreads_inferior_pid = 0;
	}

      if (linuxthreads_max && !stop_soon_quietly)
	{
	  if (linuxthreads_max
	      && WIFSTOPPED(status)
	      && WSTOPSIG(status) == SIGSTOP)
	    {
	      /* Skip SIGSTOP signals.  */
	      if (!linuxthreads_pending_status (rpid))
		{
		  if (linuxthreads_step_pid == rpid)
		    {
		      child_resume (rpid, 1, linuxthreads_step_signo);
		    }
		  else
		    {
		      child_resume (rpid, 0, TARGET_SIGNAL_0);
		    }
		}
	      continue;
	    }

	  /* Do no report exit status of cloned threads.  */
	  if (WIFEXITED(status))
	    {
	      if (rpid == linuxthreads_initial_pid)
		linuxthreads_exit_status = status;

	      /* Remove any zombie breakpoint.  */
	      for (i = 0; i <= linuxthreads_breakpoint_last; i++)
		if (linuxthreads_breakpoint_zombie[i].pid == rpid)
		  {
		    REMOVE_BREAKPOINT_ZOMBIE(i);
		    break;
		  }
	      if (pid > 0)
		pid = -1;
	      continue;
	    }

	  /* Deal with zombie breakpoint */
	  for (i = 0; i <= linuxthreads_breakpoint_last; i++)
	    if (linuxthreads_breakpoint_zombie[i].pid == rpid)
	      break;

	  if (i <= linuxthreads_breakpoint_last)
	    {
	      /* There is a potential zombie breakpoint */
	      if (WIFEXITED(status)
		  || linuxthreads_breakpoint_zombie[i].pc != read_pc_pid (rpid))
	        {
		  /* The current pc is out of zombie breakpoint.  */
		  REMOVE_BREAKPOINT_ZOMBIE(i);
		}
	      else if (!linuxthreads_breakpoint_zombie[i].step
		       && WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP)
	        {
		  /* This is a real one ==> decrement PC and restart.  */
		  write_pc_pid (linuxthreads_breakpoint_zombie[i].pc
				- DECR_PC_AFTER_BREAK, rpid);
		  if (linuxthreads_step_pid == rpid)
		    {
		      child_resume (rpid, 1, linuxthreads_step_signo);
		    }
		  else
		    {
		      child_resume (rpid, 0, TARGET_SIGNAL_0);
		    }
		  continue;
		}
	    }

	  /* Walk through linuxthreads array in order to stop them */
	  if (linuxthreads_breakpoints_inserted)
	    update_stop_threads (rpid);

	}
      else if (rpid != inferior_pid)
	continue;

      store_waitstatus (ourstatus, status);

      if (linuxthreads_attach_pending && !stop_soon_quietly)
        {
	  int on = 1;
	  if (!using_thread_db)
	    {
	      target_write_memory (linuxthreads_debug, 
				   (char *) &on, sizeof (on));
	      update_stop_threads (rpid);
	    }
	  linuxthreads_attach_pending = 0;
        }

      if (linuxthreads_breakpoints_inserted
	  && WIFSTOPPED(status)
	  && WSTOPSIG(status) == SIGTRAP)
	linuxthreads_breakpoint_pid = rpid;
      else if (linuxthreads_breakpoint_pid)
	linuxthreads_breakpoint_pid = 0;

      return rpid;
    }
}

/* Fork an inferior process, and start debugging it with ptrace.  */

static void
linuxthreads_create_inferior (exec_file, allargs, env)
    char *exec_file;
    char *allargs;
    char **env;
{
  if (!exec_file && !exec_bfd)
    {
      error ("No executable file specified.\n\
Use the \"file\" or \"exec-file\" command.");
      return;
    }

  push_target (&linuxthreads_ops);
  linuxthreads_breakpoints_inserted = 1;
  linuxthreads_breakpoint_last = -1;
  linuxthreads_wait_last = -1;
  WSETSTOP (linuxthreads_exit_status, 0);
  
  if (linuxthreads_max)
    linuxthreads_attach_pending = 1;

  child_ops.to_create_inferior (exec_file, allargs, env);
}

void
linuxthreads_discard_global_state ()
{
  linuxthreads_inferior_pid = 0;
  linuxthreads_breakpoint_pid = 0;
  linuxthreads_step_pid = 0;
  linuxthreads_step_signo = TARGET_SIGNAL_0;
  linuxthreads_manager_pid = 0;
  linuxthreads_initial_pid = 0;
  linuxthreads_attach_pending = 0;
  linuxthreads_max = 0;
}

/* Clean up after the inferior dies.  */

static void
linuxthreads_mourn_inferior ()
{
  if (linuxthreads_max)
    {
      int off = 0;
      target_write_memory (linuxthreads_debug, (char *)&off, sizeof (off));

      linuxthreads_discard_global_state ();
      init_thread_list();           /* Destroy thread info */
    }

  child_ops.to_mourn_inferior ();

  unpush_target (&linuxthreads_ops);
}

/* Kill the inferior process */

static void
linuxthreads_kill ()
{
  int rpid;
  int status;

  if (inferior_pid == 0)
    return;

  if (linuxthreads_max && linuxthreads_manager_pid != 0)
    {
      /* Remove all threads status.  */
      inferior_pid = linuxthreads_manager_pid;
      iterate_active_threads (kill_thread, 1);
    }

  kill_thread (inferior_pid);

#if 0
  /* doing_quit_force solves a real problem, but I think a properly
     placed call to catch_errors would do the trick much more cleanly.  */
  if (doing_quit_force >= 0)
    {
      if (linuxthreads_max && linuxthreads_manager_pid != 0)
	{
	  /* Wait for thread to complete */
	  while ((rpid = waitpid (-1, &status, __WCLONE)) > 0)
	    if (!WIFEXITED(status))
	      kill_thread (rpid);

	  while ((rpid = waitpid (-1, &status, 0)) > 0)
	    if (!WIFEXITED(status))
	      kill_thread (rpid);
	}
      else
	while ((rpid = waitpid (inferior_pid, &status, 0)) > 0)
	  if (!WIFEXITED(status))
	    ptrace (PT_KILL, inferior_pid, (PTRACE_ARG3_TYPE) 0, 0);
    }
#endif

  /* Wait for all threads. */
  do
    {
      rpid = waitpid (-1, &status, __WCLONE | WNOHANG);
    }
  while (rpid > 0 || errno == EINTR);
  /* FIXME: should no longer need to handle EINTR here. */

  do
    {
      rpid = waitpid (-1, &status, WNOHANG);
    }
  while (rpid > 0 || errno == EINTR);
  /* FIXME: should no longer need to handle EINTR here. */

  linuxthreads_mourn_inferior ();
}

/* Insert a breakpoint */

static int
linuxthreads_insert_breakpoint (addr, contents_cache)
    CORE_ADDR addr;
    char *contents_cache;
{
  if (linuxthreads_max && linuxthreads_manager_pid != 0)
    {
      linuxthreads_breakpoint_addr = addr;
      iterate_active_threads (insert_breakpoint, 1);
      insert_breakpoint (linuxthreads_manager_pid);
    }

  return child_ops.to_insert_breakpoint (addr, contents_cache);
}

/* Remove a breakpoint */

static int
linuxthreads_remove_breakpoint (addr, contents_cache)
    CORE_ADDR addr;
    char *contents_cache;
{
  if (linuxthreads_max && linuxthreads_manager_pid != 0)
    {
      linuxthreads_breakpoint_addr = addr;
      iterate_active_threads (remove_breakpoint, 1);
      remove_breakpoint (linuxthreads_manager_pid);
    }

  return child_ops.to_remove_breakpoint (addr, contents_cache);
}

/* Mark our target-struct as eligible for stray "run" and "attach" commands.  */

static int
linuxthreads_can_run ()
{
  return child_suppress_run;
}


static void
init_linuxthreads_ops ()
{
  linuxthreads_ops.to_shortname = "linuxthreads";
  linuxthreads_ops.to_longname  = "LINUX threads and pthread.";
  linuxthreads_ops.to_doc       = "LINUX threads and pthread support.";
  linuxthreads_ops.to_attach    = linuxthreads_attach;
  linuxthreads_ops.to_detach    = linuxthreads_detach;
  linuxthreads_ops.to_resume    = linuxthreads_resume;
  linuxthreads_ops.to_wait      = linuxthreads_wait;
  linuxthreads_ops.to_kill      = linuxthreads_kill;
  linuxthreads_ops.to_can_run   = linuxthreads_can_run;
  linuxthreads_ops.to_stratum   = thread_stratum;
  linuxthreads_ops.to_insert_breakpoint = linuxthreads_insert_breakpoint;
  linuxthreads_ops.to_remove_breakpoint = linuxthreads_remove_breakpoint;
  linuxthreads_ops.to_create_inferior   = linuxthreads_create_inferior;
  linuxthreads_ops.to_mourn_inferior    = linuxthreads_mourn_inferior;
  linuxthreads_ops.to_thread_alive      = linuxthreads_thread_alive;
  linuxthreads_ops.to_pid_to_str        = linuxthreads_pid_to_str;
  linuxthreads_ops.to_magic             = OPS_MAGIC;
}

void
_initialize_linuxthreads ()
{
  struct sigaction sact;
  sigset_t linuxthreads_wait_mask;	  /* sigset with SIGCHLD */

  init_linuxthreads_ops ();
  add_target (&linuxthreads_ops);
  child_suppress_run = 1;

  /* Hook onto the "new_objfile" event.
   * If someone else is already hooked onto the event, 
   * then make sure he will be called after we are.
   */
  target_new_objfile_chain = target_new_objfile_hook;
  target_new_objfile_hook  = linuxthreads_new_objfile;

  /* Attach SIGCHLD handler */
  sact.sa_handler = sigchld_handler;
  sigemptyset (&sact.sa_mask);
  sact.sa_flags = 0;
  sigaction (SIGCHLD, &sact, NULL);

  /* initialize SIGCHLD mask */
  sigemptyset (&linuxthreads_wait_mask);
  sigaddset (&linuxthreads_wait_mask, SIGCHLD);

  /* Use SIG_BLOCK to block receipt of SIGCHLD.
     The block_mask will allow us to wait for this signal explicitly.  */
  sigprocmask(SIG_BLOCK, 
	      &linuxthreads_wait_mask, 
	      &linuxthreads_block_mask);
  /* Make sure that linuxthreads_block_mask is not blocking SIGCHLD */
  sigdelset (&linuxthreads_block_mask, SIGCHLD);
}
