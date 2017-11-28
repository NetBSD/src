/* Multi-process/thread control defs for GDB, the GNU debugger.
   Copyright (C) 1987-2017 Free Software Foundation, Inc.
   Contributed by Lynx Real-Time Systems, Inc.  Los Gatos, CA.
   

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

#ifndef GDBTHREAD_H
#define GDBTHREAD_H

struct symtab;

#include "breakpoint.h"
#include "frame.h"
#include "ui-out.h"
#include "inferior.h"
#include "btrace.h"
#include "common/vec.h"
#include "target/waitstatus.h"
#include "cli/cli-utils.h"

/* Frontend view of the thread state.  Possible extensions: stepping,
   finishing, until(ling),...  */
enum thread_state
{
  THREAD_STOPPED,
  THREAD_RUNNING,
  THREAD_EXITED,
};

/* Inferior thread specific part of `struct infcall_control_state'.

   Inferior process counterpart is `struct inferior_control_state'.  */

struct thread_control_state
{
  /* User/external stepping state.  */

  /* Step-resume or longjmp-resume breakpoint.  */
  struct breakpoint *step_resume_breakpoint;

  /* Exception-resume breakpoint.  */
  struct breakpoint *exception_resume_breakpoint;

  /* Breakpoints used for software single stepping.  Plural, because
     it may have multiple locations.  E.g., if stepping over a
     conditional branch instruction we can't decode the condition for,
     we'll need to put a breakpoint at the branch destination, and
     another at the instruction after the branch.  */
  struct breakpoint *single_step_breakpoints;

  /* Range to single step within.

     If this is nonzero, respond to a single-step signal by continuing
     to step if the pc is in this range.

     If step_range_start and step_range_end are both 1, it means to
     step for a single instruction (FIXME: it might clean up
     wait_for_inferior in a minor way if this were changed to the
     address of the instruction and that address plus one.  But maybe
     not).  */
  CORE_ADDR step_range_start;	/* Inclusive */
  CORE_ADDR step_range_end;	/* Exclusive */

  /* Function the thread was in as of last it started stepping.  */
  struct symbol *step_start_function;

  /* If GDB issues a target step request, and this is nonzero, the
     target should single-step this thread once, and then continue
     single-stepping it without GDB core involvement as long as the
     thread stops in the step range above.  If this is zero, the
     target should ignore the step range, and only issue one single
     step.  */
  int may_range_step;

  /* Stack frame address as of when stepping command was issued.
     This is how we know when we step into a subroutine call, and how
     to set the frame for the breakpoint used to step out.  */
  struct frame_id step_frame_id;

  /* Similarly, the frame ID of the underlying stack frame (skipping
     any inlined frames).  */
  struct frame_id step_stack_frame_id;

  /* Nonzero if we are presently stepping over a breakpoint.

     If we hit a breakpoint or watchpoint, and then continue, we need
     to single step the current thread with breakpoints disabled, to
     avoid hitting the same breakpoint or watchpoint again.  And we
     should step just a single thread and keep other threads stopped,
     so that other threads don't miss breakpoints while they are
     removed.

     So, this variable simultaneously means that we need to single
     step the current thread, keep other threads stopped, and that
     breakpoints should be removed while we step.

     This variable is set either:
     - in proceed, when we resume inferior on user's explicit request
     - in keep_going, if handle_inferior_event decides we need to
     step over breakpoint.

     The variable is cleared in normal_stop.  The proceed calls
     wait_for_inferior, which calls handle_inferior_event in a loop,
     and until wait_for_inferior exits, this variable is changed only
     by keep_going.  */
  int trap_expected;

  /* Nonzero if the thread is being proceeded for a "finish" command
     or a similar situation when return value should be printed.  */
  int proceed_to_finish;

  /* Nonzero if the thread is being proceeded for an inferior function
     call.  */
  int in_infcall;

  enum step_over_calls_kind step_over_calls;

  /* Nonzero if stopped due to a step command.  */
  int stop_step;

  /* Chain containing status of breakpoint(s) the thread stopped
     at.  */
  bpstat stop_bpstat;

  /* Whether the command that started the thread was a stepping
     command.  This is used to decide whether "set scheduler-locking
     step" behaves like "on" or "off".  */
  int stepping_command;
};

/* Inferior thread specific part of `struct infcall_suspend_state'.  */

struct thread_suspend_state
{
  /* Last signal that the inferior received (why it stopped).  When
     the thread is resumed, this signal is delivered.  Note: the
     target should not check whether the signal is in pass state,
     because the signal may have been explicitly passed with the
     "signal" command, which overrides "handle nopass".  If the signal
     should be suppressed, the core will take care of clearing this
     before the target is resumed.  */
  enum gdb_signal stop_signal;

  /* The reason the thread last stopped, if we need to track it
     (breakpoint, watchpoint, etc.)  */
  enum target_stop_reason stop_reason;

  /* The waitstatus for this thread's last event.  */
  struct target_waitstatus waitstatus;
  /* If true WAITSTATUS hasn't been handled yet.  */
  int waitstatus_pending_p;

  /* Record the pc of the thread the last time it stopped.  (This is
     not the current thread's PC as that may have changed since the
     last stop, e.g., "return" command, or "p $pc = 0xf000").  This is
     used in coordination with stop_reason and waitstatus_pending_p:
     if the thread's PC is changed since it last stopped, a pending
     breakpoint waitstatus is discarded.  */
  CORE_ADDR stop_pc;
};

typedef struct value *value_ptr;
DEF_VEC_P (value_ptr);
typedef VEC (value_ptr) value_vec;

struct thread_info
{
public:
  explicit thread_info (inferior *inf, ptid_t ptid);
  ~thread_info ();

  bool deletable () const
  {
    /* If this is the current thread, or there's code out there that
       relies on it existing (m_refcount > 0) we can't delete yet.  */
    return (m_refcount == 0 && !ptid_equal (ptid, inferior_ptid));
  }

  /* Increase the refcount.  */
  void incref ()
  {
    gdb_assert (m_refcount >= 0);
    m_refcount++;
  }

  /* Decrease the refcount.  */
  void decref ()
  {
    m_refcount--;
    gdb_assert (m_refcount >= 0);
  }

  struct thread_info *next = NULL;
  ptid_t ptid;			/* "Actual process id";
				    In fact, this may be overloaded with 
				    kernel thread id, etc.  */

  /* Each thread has two GDB IDs.

     a) The thread ID (Id).  This consists of the pair of:

        - the number of the thread's inferior and,

        - the thread's thread number in its inferior, aka, the
          per-inferior thread number.  This number is unique in the
          inferior but not unique between inferiors.

     b) The global ID (GId).  This is a a single integer unique
        between all inferiors.

     E.g.:

      (gdb) info threads -gid
	Id    GId   Target Id   Frame
      * 1.1   1     Thread A    0x16a09237 in foo () at foo.c:10
	1.2   3     Thread B    0x15ebc6ed in bar () at foo.c:20
	1.3   5     Thread C    0x15ebc6ed in bar () at foo.c:20
	2.1   2     Thread A    0x16a09237 in foo () at foo.c:10
	2.2   4     Thread B    0x15ebc6ed in bar () at foo.c:20
	2.3   6     Thread C    0x15ebc6ed in bar () at foo.c:20

     Above, both inferiors 1 and 2 have threads numbered 1-3, but each
     thread has its own unique global ID.  */

  /* The thread's global GDB thread number.  This is exposed to MI,
     Python/Scheme, visible with "info threads -gid", and is also what
     the $_gthread convenience variable is bound to.  */
  int global_num;

  /* The per-inferior thread number.  This is unique in the inferior
     the thread belongs to, but not unique between inferiors.  This is
     what the $_thread convenience variable is bound to.  */
  int per_inf_num;

  /* The inferior this thread belongs to.  */
  struct inferior *inf;

  /* The name of the thread, as specified by the user.  This is NULL
     if the thread does not have a user-given name.  */
  char *name = NULL;

  /* Non-zero means the thread is executing.  Note: this is different
     from saying that there is an active target and we are stopped at
     a breakpoint, for instance.  This is a real indicator whether the
     thread is off and running.  */
  int executing = 0;

  /* Non-zero if this thread is resumed from infrun's perspective.
     Note that a thread can be marked both as not-executing and
     resumed at the same time.  This happens if we try to resume a
     thread that has a wait status pending.  We shouldn't let the
     thread really run until that wait status has been processed, but
     we should not process that wait status if we didn't try to let
     the thread run.  */
  int resumed = 0;

  /* Frontend view of the thread state.  Note that the THREAD_RUNNING/
     THREAD_STOPPED states are different from EXECUTING.  When the
     thread is stopped internally while handling an internal event,
     like a software single-step breakpoint, EXECUTING will be false,
     but STATE will still be THREAD_RUNNING.  */
  enum thread_state state = THREAD_STOPPED;

  /* State of GDB control of inferior thread execution.
     See `struct thread_control_state'.  */
  thread_control_state control {};

  /* State of inferior thread to restore after GDB is done with an inferior
     call.  See `struct thread_suspend_state'.  */
  thread_suspend_state suspend {};

  int current_line = 0;
  struct symtab *current_symtab = NULL;

  /* Internal stepping state.  */

  /* Record the pc of the thread the last time it was resumed.  (It
     can't be done on stop as the PC may change since the last stop,
     e.g., "return" command, or "p $pc = 0xf000").  This is maintained
     by proceed and keep_going, and among other things, it's used in
     adjust_pc_after_break to distinguish a hardware single-step
     SIGTRAP from a breakpoint SIGTRAP.  */
  CORE_ADDR prev_pc = 0;

  /* Did we set the thread stepping a breakpoint instruction?  This is
     used in conjunction with PREV_PC to decide whether to adjust the
     PC.  */
  int stepped_breakpoint = 0;

  /* Should we step over breakpoint next time keep_going is called?  */
  int stepping_over_breakpoint = 0;

  /* Should we step over a watchpoint next time keep_going is called?
     This is needed on targets with non-continuable, non-steppable
     watchpoints.  */
  int stepping_over_watchpoint = 0;

  /* Set to TRUE if we should finish single-stepping over a breakpoint
     after hitting the current step-resume breakpoint.  The context here
     is that GDB is to do `next' or `step' while signal arrives.
     When stepping over a breakpoint and signal arrives, GDB will attempt
     to skip signal handler, so it inserts a step_resume_breakpoint at the
     signal return address, and resume inferior.
     step_after_step_resume_breakpoint is set to TRUE at this moment in
     order to keep GDB in mind that there is still a breakpoint to step over
     when GDB gets back SIGTRAP from step_resume_breakpoint.  */
  int step_after_step_resume_breakpoint = 0;

  /* Pointer to the state machine manager object that handles what is
     left to do for the thread's execution command after the target
     stops.  Several execution commands use it.  */
  struct thread_fsm *thread_fsm = NULL;

  /* This is used to remember when a fork or vfork event was caught by
     a catchpoint, and thus the event is to be followed at the next
     resume of the thread, and not immediately.  */
  struct target_waitstatus pending_follow;

  /* True if this thread has been explicitly requested to stop.  */
  int stop_requested = 0;

  /* The initiating frame of a nexting operation, used for deciding
     which exceptions to intercept.  If it is null_frame_id no
     bp_longjmp or bp_exception but longjmp has been caught just for
     bp_longjmp_call_dummy.  */
  struct frame_id initiating_frame = null_frame_id;

  /* Private data used by the target vector implementation.  */
  struct private_thread_info *priv = NULL;

  /* Function that is called to free PRIVATE.  If this is NULL, then
     xfree will be called on PRIVATE.  */
  void (*private_dtor) (struct private_thread_info *) = NULL;

  /* Branch trace information for this thread.  */
  struct btrace_thread_info btrace {};

  /* Flag which indicates that the stack temporaries should be stored while
     evaluating expressions.  */
  int stack_temporaries_enabled = 0;

  /* Values that are stored as temporaries on stack while evaluating
     expressions.  */
  value_vec *stack_temporaries = NULL;

  /* Step-over chain.  A thread is in the step-over queue if these are
     non-NULL.  If only a single thread is in the chain, then these
     fields point to self.  */
  struct thread_info *step_over_prev = NULL;
  struct thread_info *step_over_next = NULL;

private:

  /* If this is > 0, then it means there's code out there that relies
     on this thread being listed.  Don't delete it from the lists even
     if we detect it exiting.  */
  int m_refcount = 0;
};

/* Create an empty thread list, or empty the existing one.  */
extern void init_thread_list (void);

/* Add a thread to the thread list, print a message
   that a new thread is found, and return the pointer to
   the new thread.  Caller my use this pointer to 
   initialize the private thread data.  */
extern struct thread_info *add_thread (ptid_t ptid);

/* Same as add_thread, but does not print a message
   about new thread.  */
extern struct thread_info *add_thread_silent (ptid_t ptid);

/* Same as add_thread, and sets the private info.  */
extern struct thread_info *add_thread_with_info (ptid_t ptid,
						 struct private_thread_info *);

/* Delete an existing thread list entry.  */
extern void delete_thread (ptid_t);

/* Delete an existing thread list entry, and be quiet about it.  Used
   after the process this thread having belonged to having already
   exited, for example.  */
extern void delete_thread_silent (ptid_t);

/* Delete a step_resume_breakpoint from the thread database.  */
extern void delete_step_resume_breakpoint (struct thread_info *);

/* Delete an exception_resume_breakpoint from the thread database.  */
extern void delete_exception_resume_breakpoint (struct thread_info *);

/* Delete the single-step breakpoints of thread TP, if any.  */
extern void delete_single_step_breakpoints (struct thread_info *tp);

/* Check if the thread has software single stepping breakpoints
   set.  */
extern int thread_has_single_step_breakpoints_set (struct thread_info *tp);

/* Check whether the thread has software single stepping breakpoints
   set at PC.  */
extern int thread_has_single_step_breakpoint_here (struct thread_info *tp,
						   struct address_space *aspace,
						   CORE_ADDR addr);

/* Translate the global integer thread id (GDB's homegrown id, not the
   system's) into a "pid" (which may be overloaded with extra thread
   information).  */
extern ptid_t global_thread_id_to_ptid (int num);

/* Translate a 'pid' (which may be overloaded with extra thread
   information) into the global integer thread id (GDB's homegrown id,
   not the system's).  */
extern int ptid_to_global_thread_id (ptid_t ptid);

/* Returns whether to show inferior-qualified thread IDs, or plain
   thread numbers.  Inferior-qualified IDs are shown whenever we have
   multiple inferiors, or the only inferior left has number > 1.  */
extern int show_inferior_qualified_tids (void);

/* Return a string version of THR's thread ID.  If there are multiple
   inferiors, then this prints the inferior-qualifier form, otherwise
   it only prints the thread number.  The result is stored in a
   circular static buffer, NUMCELLS deep.  */
const char *print_thread_id (struct thread_info *thr);

/* Boolean test for an already-known pid (which may be overloaded with
   extra thread information).  */
extern int in_thread_list (ptid_t ptid);

/* Boolean test for an already-known global thread id (GDB's homegrown
   global id, not the system's).  */
extern int valid_global_thread_id (int global_id);

/* Search function to lookup a thread by 'pid'.  */
extern struct thread_info *find_thread_ptid (ptid_t ptid);

/* Find thread by GDB global thread ID.  */
struct thread_info *find_thread_global_id (int global_id);

/* Finds the first thread of the inferior given by PID.  If PID is -1,
   returns the first thread in the list.  */
struct thread_info *first_thread_of_process (int pid);

/* Returns any thread of process PID, giving preference to the current
   thread.  */
extern struct thread_info *any_thread_of_process (int pid);

/* Returns any non-exited thread of process PID, giving preference to
   the current thread, and to not executing threads.  */
extern struct thread_info *any_live_thread_of_process (int pid);

/* Change the ptid of thread OLD_PTID to NEW_PTID.  */
void thread_change_ptid (ptid_t old_ptid, ptid_t new_ptid);

/* Iterator function to call a user-provided callback function
   once for each known thread.  */
typedef int (*thread_callback_func) (struct thread_info *, void *);
extern struct thread_info *iterate_over_threads (thread_callback_func, void *);

/* Traverse all threads.  */
#define ALL_THREADS(T)				\
  for (T = thread_list; T; T = T->next)		\

/* Traverse over all threads, sorted by inferior.  */
#define ALL_THREADS_BY_INFERIOR(inf, tp) \
  ALL_INFERIORS (inf) \
    ALL_THREADS (tp) \
      if (inf == tp->inf)

/* Traverse all threads, except those that have THREAD_EXITED
   state.  */

#define ALL_NON_EXITED_THREADS(T)				\
  for (T = thread_list; T; T = T->next) \
    if ((T)->state != THREAD_EXITED)

/* Traverse all threads, including those that have THREAD_EXITED
   state.  Allows deleting the currently iterated thread.  */
#define ALL_THREADS_SAFE(T, TMP)	\
  for ((T) = thread_list;			\
       (T) != NULL ? ((TMP) = (T)->next, 1): 0;	\
       (T) = (TMP))

extern int thread_count (void);

/* Switch from one thread to another.  Also sets the STOP_PC
   global.  */
extern void switch_to_thread (ptid_t ptid);

/* Switch from one thread to another.  Does not read registers and
   sets STOP_PC to -1.  */
extern void switch_to_thread_no_regs (struct thread_info *thread);

/* Marks or clears thread(s) PTID as resumed.  If PTID is
   MINUS_ONE_PTID, applies to all threads.  If ptid_is_pid(PTID) is
   true, applies to all threads of the process pointed at by PTID.  */
extern void set_resumed (ptid_t ptid, int resumed);

/* Marks thread PTID is running, or stopped. 
   If PTID is minus_one_ptid, marks all threads.  */
extern void set_running (ptid_t ptid, int running);

/* Marks or clears thread(s) PTID as having been requested to stop.
   If PTID is MINUS_ONE_PTID, applies to all threads.  If
   ptid_is_pid(PTID) is true, applies to all threads of the process
   pointed at by PTID.  If STOP, then the THREAD_STOP_REQUESTED
   observer is called with PTID as argument.  */
extern void set_stop_requested (ptid_t ptid, int stop);

/* NOTE: Since the thread state is not a boolean, most times, you do
   not want to check it with negation.  If you really want to check if
   the thread is stopped,

    use (good):

     if (is_stopped (ptid))

    instead of (bad):

     if (!is_running (ptid))

   The latter also returns true on exited threads, most likelly not
   what you want.  */

/* Reports if in the frontend's perpective, thread PTID is running.  */
extern int is_running (ptid_t ptid);

/* Is this thread listed, but known to have exited?  We keep it listed
   (but not visible) until it's safe to delete.  */
extern int is_exited (ptid_t ptid);

/* In the frontend's perpective, is this thread stopped?  */
extern int is_stopped (ptid_t ptid);

/* Marks thread PTID as executing, or not.  If PTID is minus_one_ptid,
   marks all threads.

   Note that this is different from the running state.  See the
   description of state and executing fields of struct
   thread_info.  */
extern void set_executing (ptid_t ptid, int executing);

/* Reports if thread PTID is executing.  */
extern int is_executing (ptid_t ptid);

/* True if any (known or unknown) thread is or may be executing.  */
extern int threads_are_executing (void);

/* Merge the executing property of thread PTID over to its thread
   state property (frontend running/stopped view).

   "not executing" -> "stopped"
   "executing"     -> "running"
   "exited"        -> "exited"

   If PTID is minus_one_ptid, go over all threads.

   Notifications are only emitted if the thread state did change.  */
extern void finish_thread_state (ptid_t ptid);

/* Same as FINISH_THREAD_STATE, but with an interface suitable to be
   registered as a cleanup.  PTID_P points to the ptid_t that is
   passed to FINISH_THREAD_STATE.  */
extern void finish_thread_state_cleanup (void *ptid_p);

/* Commands with a prefix of `thread'.  */
extern struct cmd_list_element *thread_cmd_list;

extern void thread_command (char *tidstr, int from_tty);

/* Print notices on thread events (attach, detach, etc.), set with
   `set print thread-events'.  */
extern int print_thread_events;

/* Prints the list of threads and their details on UIOUT.  If
   REQUESTED_THREADS, a list of GDB ids/ranges, is not NULL, only
   print threads whose ID is included in the list.  If PID is not -1,
   only print threads from the process PID.  Otherwise, threads from
   all attached PIDs are printed.  If both REQUESTED_THREADS is not
   NULL and PID is not -1, then the thread is printed if it belongs to
   the specified process.  Otherwise, an error is raised.  */
extern void print_thread_info (struct ui_out *uiout, char *requested_threads,
			       int pid);

extern struct cleanup *make_cleanup_restore_current_thread (void);

/* Returns a pointer into the thread_info corresponding to
   INFERIOR_PTID.  INFERIOR_PTID *must* be in the thread list.  */
extern struct thread_info* inferior_thread (void);

extern void update_thread_list (void);

/* Delete any thread the target says is no longer alive.  */

extern void prune_threads (void);

/* Delete threads marked THREAD_EXITED.  Unlike prune_threads, this
   does not consult the target about whether the thread is alive right
   now.  */
extern void delete_exited_threads (void);

/* Return true if PC is in the stepping range of THREAD.  */

int pc_in_thread_step_range (CORE_ADDR pc, struct thread_info *thread);

extern struct cleanup *enable_thread_stack_temporaries (ptid_t ptid);

extern int thread_stack_temporaries_enabled_p (ptid_t ptid);

extern void push_thread_stack_temporary (ptid_t ptid, struct value *v);

extern struct value *get_last_thread_stack_temporary (ptid_t);

extern int value_in_thread_stack_temporaries (struct value *, ptid_t);

/* Add TP to the end of its inferior's pending step-over chain.  */

extern void thread_step_over_chain_enqueue (struct thread_info *tp);

/* Remove TP from its inferior's pending step-over chain.  */

extern void thread_step_over_chain_remove (struct thread_info *tp);

/* Return the next thread in the step-over chain starting at TP.  NULL
   if TP is the last entry in the chain.  */

extern struct thread_info *thread_step_over_chain_next (struct thread_info *tp);

/* Return true if TP is in the step-over chain.  */

extern int thread_is_in_step_over_chain (struct thread_info *tp);

/* Cancel any ongoing execution command.  */

extern void thread_cancel_execution_command (struct thread_info *thr);

/* Check whether it makes sense to access a register of the current
   thread at this point.  If not, throw an error (e.g., the thread is
   executing).  */
extern void validate_registers_access (void);

/* Check whether it makes sense to access a register of PTID at this point.
   Returns true if registers may be accessed; false otherwise.  */
extern bool can_access_registers_ptid (ptid_t ptid);

/* Returns whether to show which thread hit the breakpoint, received a
   signal, etc. and ended up causing a user-visible stop.  This is
   true iff we ever detected multiple threads.  */
extern int show_thread_that_caused_stop (void);

/* Print the message for a thread or/and frame selected.  */
extern void print_selected_thread_frame (struct ui_out *uiout,
					 user_selected_what selection);

extern struct thread_info *thread_list;

#endif /* GDBTHREAD_H */
