/* Native debugging support for GNU/Linux (LWP layer).

   Copyright (C) 2000-2013 Free Software Foundation, Inc.

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

#include "target.h"

#include <signal.h>

struct arch_lwp_info;

/* Ways to "resume" a thread.  */

enum resume_kind
{
  /* Thread should continue.  */
  resume_continue,

  /* Thread should single-step.  */
  resume_step,

  /* Thread should be stopped.  */
  resume_stop
};

/* Structure describing an LWP.  This is public only for the purposes
   of ALL_LWPS; target-specific code should generally not access it
   directly.  */

struct lwp_info
{
  /* The process id of the LWP.  This is a combination of the LWP id
     and overall process id.  */
  ptid_t ptid;

  /* Non-zero if this LWP is cloned.  In this context "cloned" means
     that the LWP is reporting to its parent using a signal other than
     SIGCHLD.  */
  int cloned;

  /* Non-zero if we sent this LWP a SIGSTOP (but the LWP didn't report
     it back yet).  */
  int signalled;

  /* Non-zero if this LWP is stopped.  */
  int stopped;

  /* Non-zero if this LWP will be/has been resumed.  Note that an LWP
     can be marked both as stopped and resumed at the same time.  This
     happens if we try to resume an LWP that has a wait status
     pending.  We shouldn't let the LWP run until that wait status has
     been processed, but we should not report that wait status if GDB
     didn't try to let the LWP run.  */
  int resumed;

  /* The last resume GDB requested on this thread.  */
  enum resume_kind last_resume_kind;

  /* If non-zero, a pending wait status.  */
  int status;

  /* Non-zero if we were stepping this LWP.  */
  int step;

  /* STOPPED_BY_WATCHPOINT is non-zero if this LWP stopped with a data
     watchpoint trap.  */
  int stopped_by_watchpoint;

  /* On architectures where it is possible to know the data address of
     a triggered watchpoint, STOPPED_DATA_ADDRESS_P is non-zero, and
     STOPPED_DATA_ADDRESS contains such data address.  Otherwise,
     STOPPED_DATA_ADDRESS_P is false, and STOPPED_DATA_ADDRESS is
     undefined.  Only valid if STOPPED_BY_WATCHPOINT is true.  */
  int stopped_data_address_p;
  CORE_ADDR stopped_data_address;

  /* Non-zero if we expect a duplicated SIGINT.  */
  int ignore_sigint;

  /* If WAITSTATUS->KIND != TARGET_WAITKIND_SPURIOUS, the waitstatus
     for this LWP's last event.  This may correspond to STATUS above,
     or to a local variable in lin_lwp_wait.  */
  struct target_waitstatus waitstatus;

  /* Signal wether we are in a SYSCALL_ENTRY or
     in a SYSCALL_RETURN event.
     Values:
     - TARGET_WAITKIND_SYSCALL_ENTRY
     - TARGET_WAITKIND_SYSCALL_RETURN */
  int syscall_state;

  /* The processor core this LWP was last seen on.  */
  int core;

  /* Arch-specific additions.  */
  struct arch_lwp_info *arch_private;

  /* Next LWP in list.  */
  struct lwp_info *next;
};

/* The global list of LWPs, for ALL_LWPS.  Unlike the threads list,
   there is always at least one LWP on the list while the GNU/Linux
   native target is active.  */
extern struct lwp_info *lwp_list;

/* Iterate over each active thread (light-weight process).  */
#define ALL_LWPS(LP)							\
  for ((LP) = lwp_list;							\
       (LP) != NULL;							\
       (LP) = (LP)->next)

#define GET_LWP(ptid)		ptid_get_lwp (ptid)
#define GET_PID(ptid)		ptid_get_pid (ptid)
#define is_lwp(ptid)		(GET_LWP (ptid) != 0)
#define BUILD_LWP(lwp, pid)	ptid_build (pid, lwp, 0)

/* Attempt to initialize libthread_db.  */
void check_for_thread_db (void);

int thread_db_attach_lwp (ptid_t ptid);

/* Return the set of signals used by the threads library.  */
extern void lin_thread_get_thread_signals (sigset_t *mask);

/* Find process PID's pending signal set from /proc/pid/status.  */
void linux_proc_pending_signals (int pid, sigset_t *pending,
				 sigset_t *blocked, sigset_t *ignored);

/* linux-nat functions for handling fork events.  */
extern void linux_enable_event_reporting (ptid_t ptid);

extern int lin_lwp_attach_lwp (ptid_t ptid);

extern void linux_stop_lwp (struct lwp_info *lwp);

/* Iterator function for lin-lwp's lwp list.  */
struct lwp_info *iterate_over_lwps (ptid_t filter,
				    int (*callback) (struct lwp_info *,
						     void *), 
				    void *data);

/* Create a prototype generic GNU/Linux target.  The client can
   override it with local methods.  */
struct target_ops * linux_target (void);

/* Create a generic GNU/Linux target using traditional 
   ptrace register access.  */
struct target_ops *
linux_trad_target (CORE_ADDR (*register_u_offset)(struct gdbarch *, int, int));

/* Register the customized GNU/Linux target.  This should be used
   instead of calling add_target directly.  */
void linux_nat_add_target (struct target_ops *);

/* Register a method to call whenever a new thread is attached.  */
void linux_nat_set_new_thread (struct target_ops *, void (*) (struct lwp_info *));


/* Register a method to call whenever a new fork is attached.  */
typedef void (linux_nat_new_fork_ftype) (struct lwp_info *parent,
					 pid_t child_pid);
void linux_nat_set_new_fork (struct target_ops *ops,
			     linux_nat_new_fork_ftype *fn);

/* Register a method to call whenever a process is killed or
   detached.  */
typedef void (linux_nat_forget_process_ftype) (pid_t pid);
void linux_nat_set_forget_process (struct target_ops *ops,
				   linux_nat_forget_process_ftype *fn);

/* Call the method registered with the function above.  PID is the
   process to forget about.  */
void linux_nat_forget_process (pid_t pid);

/* Register a method that converts a siginfo object between the layout
   that ptrace returns, and the layout in the architecture of the
   inferior.  */
void linux_nat_set_siginfo_fixup (struct target_ops *,
				  int (*) (siginfo_t *,
					   gdb_byte *,
					   int));

/* Register a method to call prior to resuming a thread.  */

void linux_nat_set_prepare_to_resume (struct target_ops *,
				      void (*) (struct lwp_info *));

/* Update linux-nat internal state when changing from one fork
   to another.  */
void linux_nat_switch_fork (ptid_t new_ptid);

/* Store the saved siginfo associated with PTID in *SIGINFO.
   Return 1 if it was retrieved successfully, 0 otherwise (*SIGINFO is
   uninitialized in such case).  */
int linux_nat_get_siginfo (ptid_t ptid, siginfo_t *siginfo);

/* Set alternative SIGTRAP-like events recognizer.  */
void linux_nat_set_status_is_event (struct target_ops *t,
				    int (*status_is_event) (int status));
