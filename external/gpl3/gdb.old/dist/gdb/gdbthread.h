/* Multi-process/thread control defs for GDB, the GNU debugger.
   Copyright (C) 1987-2020 Free Software Foundation, Inc.
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
#include "btrace.h"
#include "target/waitstatus.h"
#include "cli/cli-utils.h"
#include "gdbsupport/refcounted-object.h"
#include "gdbsupport/common-gdbthread.h"
#include "gdbsupport/forward-scope-exit.h"

struct inferior;
struct process_stratum_target;

/* Frontend view of the thread state.  Possible extensions: stepping,
   finishing, until(ling),...

   NOTE: Since the thread state is not a boolean, most times, you do
   not want to check it with negation.  If you really want to check if
   the thread is stopped,

    use (good):

     if (tp->state == THREAD_STOPPED)

    instead of (bad):

     if (tp->state != THREAD_RUNNING)

   The latter is also true for exited threads, most likely not what
   you want.  */
enum thread_state
{
  /* In the frontend's perpective, the thread is stopped.  */
  THREAD_STOPPED,

  /* In the frontend's perpective, the thread is running.  */
  THREAD_RUNNING,

  /* The thread is listed, but known to have exited.  We keep it
     listed (but not visible) until it's safe to delete it.  */
  THREAD_EXITED,
};

/* STEP_OVER_ALL means step over all subroutine calls.
   STEP_OVER_UNDEBUGGABLE means step over calls to undebuggable functions.
   STEP_OVER_NONE means don't step over any subroutine calls.  */

enum step_over_calls_kind
  {
    STEP_OVER_NONE,
    STEP_OVER_ALL,
    STEP_OVER_UNDEBUGGABLE
  };

/* Inferior thread specific part of `struct infcall_control_state'.

   Inferior process counterpart is `struct inferior_control_state'.  */

struct thread_control_state
{
  /* User/external stepping state.  */

  /* Step-resume or longjmp-resume breakpoint.  */
  struct breakpoint *step_resume_breakpoint = nullptr;

  /* Exception-resume breakpoint.  */
  struct breakpoint *exception_resume_breakpoint = nullptr;

  /* Breakpoints used for software single stepping.  Plural, because
     it may have multiple locations.  E.g., if stepping over a
     conditional branch instruction we can't decode the condition for,
     we'll need to put a breakpoint at the branch destination, and
     another at the instruction after the branch.  */
  struct breakpoint *single_step_breakpoints = nullptr;

  /* Range to single step within.

     If this is nonzero, respond to a single-step signal by continuing
     to step if the pc is in this range.

     If step_range_start and step_range_end are both 1, it means to
     step for a single instruction (FIXME: it might clean up
     wait_for_inferior in a minor way if this were changed to the
     address of the instruction and that address plus one.  But maybe
     not).  */
  CORE_ADDR step_range_start = 0;	/* Inclusive */
  CORE_ADDR step_range_end = 0;		/* Exclusive */

  /* Function the thread was in as of last it started stepping.  */
  struct symbol *step_start_function = nullptr;

  /* If GDB issues a target step request, and this is nonzero, the
     target should single-step this thread once, and then continue
     single-stepping it without GDB core involvement as long as the
     thread stops in the step range above.  If this is zero, the
     target should ignore the step range, and only issue one single
     step.  */
  int may_range_step = 0;

  /* Stack frame address as of when stepping command was issued.
     This is how we know when we step into a subroutine call, and how
     to set the frame for the breakpoint used to step out.  */
  struct frame_id step_frame_id {};

  /* Similarly, the frame ID of the underlying stack frame (skipping
     any inlined frames).  */
  struct frame_id step_stack_frame_id {};

  /* True if the the thread is presently stepping over a breakpoint or
     a watchpoint, either with an inline step over or a displaced (out
     of line) step, and we're now expecting it to report a trap for
     the finished single step.  */
  int trap_expected = 0;

  /* Nonzero if the thread is being proceeded for a "finish" command
     or a similar situation when return value should be printed.  */
  int proceed_to_finish = 0;

  /* Nonzero if the thread is being proceeded for an inferior function
     call.  */
  int in_infcall = 0;

  enum step_over_calls_kind step_over_calls = STEP_OVER_NONE;

  /* Nonzero if stopped due to a step command.  */
  int stop_step = 0;

  /* Chain containing status of breakpoint(s) the thread stopped
     at.  */
  bpstat stop_bpstat = nullptr;

  /* Whether the command that started the thread was a stepping
     command.  This is used to decide whether "set scheduler-locking
     step" behaves like "on" or "off".  */
  int stepping_command = 0;
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
  enum gdb_signal stop_signal = GDB_SIGNAL_0;

  /* The reason the thread last stopped, if we need to track it
     (breakpoint, watchpoint, etc.)  */
  enum target_stop_reason stop_reason = TARGET_STOPPED_BY_NO_REASON;

  /* The waitstatus for this thread's last event.  */
  struct target_waitstatus waitstatus {};
  /* If true WAITSTATUS hasn't been handled yet.  */
  int waitstatus_pending_p = 0;

  /* Record the pc of the thread the last time it stopped.  (This is
     not the current thread's PC as that may have changed since the
     last stop, e.g., "return" command, or "p $pc = 0xf000").

     - If the thread's PC has not changed since the thread last
       stopped, then proceed skips a breakpoint at the current PC,
       otherwise we let the thread run into the breakpoint.

     - If the thread has an unprocessed event pending, as indicated by
       waitstatus_pending_p, this is used in coordination with
       stop_reason: if the thread's PC has changed since the thread
       last stopped, a pending breakpoint waitstatus is discarded.

     - If the thread is running, this is set to -1, to avoid leaving
       it with a stale value, to make it easier to catch bugs.  */
  CORE_ADDR stop_pc = 0;
};

/* Base class for target-specific thread data.  */
struct private_thread_info
{
  virtual ~private_thread_info () = 0;
};

/* Threads are intrusively refcounted objects.  Being the
   user-selected thread is normally considered an implicit strong
   reference and is thus not accounted in the refcount, unlike
   inferior objects.  This is necessary, because there's no "current
   thread" pointer.  Instead the current thread is inferred from the
   inferior_ptid global.  However, when GDB needs to remember the
   selected thread to later restore it, GDB bumps the thread object's
   refcount, to prevent something deleting the thread object before
   reverting back (e.g., due to a "kill" command).  If the thread
   meanwhile exits before being re-selected, then the thread object is
   left listed in the thread list, but marked with state
   THREAD_EXITED.  (See scoped_restore_current_thread and
   delete_thread).  All other thread references are considered weak
   references.  Placing a thread in the thread list is an implicit
   strong reference, and is thus not accounted for in the thread's
   refcount.  */

class thread_info : public refcounted_object
{
public:
  explicit thread_info (inferior *inf, ptid_t ptid);
  ~thread_info ();

  bool deletable () const;

  /* Mark this thread as running and notify observers.  */
  void set_running (bool running);

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

  /* True means the thread is executing.  Note: this is different
     from saying that there is an active target and we are stopped at
     a breakpoint, for instance.  This is a real indicator whether the
     thread is off and running.  */
  bool executing = false;

  /* True if this thread is resumed from infrun's perspective.
     Note that a thread can be marked both as not-executing and
     resumed at the same time.  This happens if we try to resume a
     thread that has a wait status pending.  We shouldn't let the
     thread really run until that wait status has been processed, but
     we should not process that wait status if we didn't try to let
     the thread run.  */
  bool resumed = false;

  /* Frontend view of the thread state.  Note that the THREAD_RUNNING/
     THREAD_STOPPED states are different from EXECUTING.  When the
     thread is stopped internally while handling an internal event,
     like a software single-step breakpoint, EXECUTING will be false,
     but STATE will still be THREAD_RUNNING.  */
  enum thread_state state = THREAD_STOPPED;

  /* State of GDB control of inferior thread execution.
     See `struct thread_control_state'.  */
  thread_control_state control;

  /* State of inferior thread to restore after GDB is done with an inferior
     call.  See `struct thread_suspend_state'.  */
  thread_suspend_state suspend;

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
  std::unique_ptr<private_thread_info> priv;

  /* Branch trace information for this thread.  */
  struct btrace_thread_info btrace {};

  /* Flag which indicates that the stack temporaries should be stored while
     evaluating expressions.  */
  bool stack_temporaries_enabled = false;

  /* Values that are stored as temporaries on stack while evaluating
     expressions.  */
  std::vector<struct value *> stack_temporaries;

  /* Step-over chain.  A thread is in the step-over queue if these are
     non-NULL.  If only a single thread is in the chain, then these
     fields point to self.  */
  struct thread_info *step_over_prev = NULL;
  struct thread_info *step_over_next = NULL;
};

/* A gdb::ref_ptr pointer to a thread_info.  */

using thread_info_ref
  = gdb::ref_ptr<struct thread_info, refcounted_object_ref_policy>;

/* A gdb::ref_ptr pointer to an inferior.  This would ideally be in
   inferior.h, but it can't due to header dependencies (inferior.h
   includes gdbthread.h).  */

using inferior_ref
  = gdb::ref_ptr<struct inferior, refcounted_object_ref_policy>;

/* Create an empty thread list, or empty the existing one.  */
extern void init_thread_list (void);

/* Add a thread to the thread list, print a message
   that a new thread is found, and return the pointer to
   the new thread.  Caller my use this pointer to 
   initialize the private thread data.  */
extern struct thread_info *add_thread (process_stratum_target *targ,
				       ptid_t ptid);

/* Same as add_thread, but does not print a message about new
   thread.  */
extern struct thread_info *add_thread_silent (process_stratum_target *targ,
					      ptid_t ptid);

/* Same as add_thread, and sets the private info.  */
extern struct thread_info *add_thread_with_info (process_stratum_target *targ,
						 ptid_t ptid,
						 private_thread_info *);

/* Delete thread THREAD and notify of thread exit.  If the thread is
   currently not deletable, don't actually delete it but still tag it
   as exited and do the notification.  */
extern void delete_thread (struct thread_info *thread);

/* Like delete_thread, but be quiet about it.  Used when the process
   this thread belonged to has already exited, for example.  */
extern void delete_thread_silent (struct thread_info *thread);

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
						   const address_space *aspace,
						   CORE_ADDR addr);

/* Returns whether to show inferior-qualified thread IDs, or plain
   thread numbers.  Inferior-qualified IDs are shown whenever we have
   multiple inferiors, or the only inferior left has number > 1.  */
extern int show_inferior_qualified_tids (void);

/* Return a string version of THR's thread ID.  If there are multiple
   inferiors, then this prints the inferior-qualifier form, otherwise
   it only prints the thread number.  The result is stored in a
   circular static buffer, NUMCELLS deep.  */
const char *print_thread_id (struct thread_info *thr);

/* Boolean test for an already-known ptid.  */
extern bool in_thread_list (process_stratum_target *targ, ptid_t ptid);

/* Boolean test for an already-known global thread id (GDB's homegrown
   global id, not the system's).  */
extern int valid_global_thread_id (int global_id);

/* Find (non-exited) thread PTID of inferior INF.  */
extern thread_info *find_thread_ptid (inferior *inf, ptid_t ptid);

/* Search function to lookup a (non-exited) thread by 'ptid'.  */
extern struct thread_info *find_thread_ptid (process_stratum_target *targ,
					     ptid_t ptid);

/* Search function to lookup a (non-exited) thread by 'ptid'.  Only
   searches in threads of INF.  */
extern struct thread_info *find_thread_ptid (inferior *inf, ptid_t ptid);

/* Find thread by GDB global thread ID.  */
struct thread_info *find_thread_global_id (int global_id);

/* Find thread by thread library specific handle in inferior INF.  */
struct thread_info *find_thread_by_handle
  (gdb::array_view<const gdb_byte> handle, struct inferior *inf);

/* Finds the first thread of the specified inferior.  */
extern struct thread_info *first_thread_of_inferior (inferior *inf);

/* Returns any thread of inferior INF, giving preference to the
   current thread.  */
extern struct thread_info *any_thread_of_inferior (inferior *inf);

/* Returns any non-exited thread of inferior INF, giving preference to
   the current thread, and to not executing threads.  */
extern struct thread_info *any_live_thread_of_inferior (inferior *inf);

/* Change the ptid of thread OLD_PTID to NEW_PTID.  */
void thread_change_ptid (process_stratum_target *targ,
			 ptid_t old_ptid, ptid_t new_ptid);

/* Iterator function to call a user-provided callback function
   once for each known thread.  */
typedef int (*thread_callback_func) (struct thread_info *, void *);
extern struct thread_info *iterate_over_threads (thread_callback_func, void *);

/* Pull in the internals of the inferiors/threads ranges and
   iterators.  Must be done after struct thread_info is defined.  */
#include "thread-iter.h"

/* Return a range that can be used to walk over threads, with
   range-for.

   Used like this, it walks over all threads of all inferiors of all
   targets:

       for (thread_info *thr : all_threads ())
	 { .... }

   FILTER_PTID can be used to filter out threads that don't match.
   FILTER_PTID can be:

   - minus_one_ptid, meaning walk all threads of all inferiors of
     PROC_TARGET.  If PROC_TARGET is NULL, then of all targets.

   - A process ptid, in which case walk all threads of the specified
     process.  PROC_TARGET must be non-NULL in this case.

   - A thread ptid, in which case walk that thread only.  PROC_TARGET
     must be non-NULL in this case.
*/

inline all_matching_threads_range
all_threads (process_stratum_target *proc_target = nullptr,
	     ptid_t filter_ptid = minus_one_ptid)
{
  return all_matching_threads_range (proc_target, filter_ptid);
}

/* Return a range that can be used to walk over all non-exited threads
   of all inferiors, with range-for.  Arguments are like all_threads
   above.  */

inline all_non_exited_threads_range
all_non_exited_threads (process_stratum_target *proc_target = nullptr,
			ptid_t filter_ptid = minus_one_ptid)
{
  return all_non_exited_threads_range (proc_target, filter_ptid);
}

/* Return a range that can be used to walk over all threads of all
   inferiors, with range-for, safely.  I.e., it is safe to delete the
   currently-iterated thread.  When combined with range-for, this
   allow convenient patterns like this:

     for (thread_info *t : all_threads_safe ())
       if (some_condition ())
	 delete f;
*/

inline all_threads_safe_range
all_threads_safe ()
{
  return {};
}

extern int thread_count (process_stratum_target *proc_target);

/* Return true if we have any thread in any inferior.  */
extern bool any_thread_p ();

/* Switch context to thread THR.  Also sets the STOP_PC global.  */
extern void switch_to_thread (struct thread_info *thr);

/* Switch context to no thread selected.  */
extern void switch_to_no_thread ();

/* Switch from one thread to another.  Does not read registers.  */
extern void switch_to_thread_no_regs (struct thread_info *thread);

/* Marks or clears thread(s) PTID of TARG as resumed.  If PTID is
   MINUS_ONE_PTID, applies to all threads of TARG.  If
   ptid_is_pid(PTID) is true, applies to all threads of the process
   pointed at by {TARG,PTID}.  */
extern void set_resumed (process_stratum_target *targ,
			 ptid_t ptid, bool resumed);

/* Marks thread PTID of TARG as running, or as stopped.  If PTID is
   minus_one_ptid, marks all threads of TARG.  */
extern void set_running (process_stratum_target *targ,
			 ptid_t ptid, bool running);

/* Marks or clears thread(s) PTID of TARG as having been requested to
   stop.  If PTID is MINUS_ONE_PTID, applies to all threads of TARG.
   If ptid_is_pid(PTID) is true, applies to all threads of the process
   pointed at by {TARG, PTID}.  If STOP, then the
   THREAD_STOP_REQUESTED observer is called with PTID as argument.  */
extern void set_stop_requested (process_stratum_target *targ,
				ptid_t ptid, bool stop);

/* Marks thread PTID of TARG as executing, or not.  If PTID is
   minus_one_ptid, marks all threads of TARG.

   Note that this is different from the running state.  See the
   description of state and executing fields of struct
   thread_info.  */
extern void set_executing (process_stratum_target *targ,
			   ptid_t ptid, bool executing);

/* True if any (known or unknown) thread of TARG is or may be
   executing.  */
extern bool threads_are_executing (process_stratum_target *targ);

/* Merge the executing property of thread PTID of TARG over to its
   thread state property (frontend running/stopped view).

   "not executing" -> "stopped"
   "executing"     -> "running"
   "exited"        -> "exited"

   If PTID is minus_one_ptid, go over all threads of TARG.

   Notifications are only emitted if the thread state did change.  */
extern void finish_thread_state (process_stratum_target *targ, ptid_t ptid);

/* Calls finish_thread_state on scope exit, unless release() is called
   to disengage.  */
using scoped_finish_thread_state
  = FORWARD_SCOPE_EXIT (finish_thread_state);

/* Commands with a prefix of `thread'.  */
extern struct cmd_list_element *thread_cmd_list;

extern void thread_command (const char *tidstr, int from_tty);

/* Print notices on thread events (attach, detach, etc.), set with
   `set print thread-events'.  */
extern bool print_thread_events;

/* Prints the list of threads and their details on UIOUT.  If
   REQUESTED_THREADS, a list of GDB ids/ranges, is not NULL, only
   print threads whose ID is included in the list.  If PID is not -1,
   only print threads from the process PID.  Otherwise, threads from
   all attached PIDs are printed.  If both REQUESTED_THREADS is not
   NULL and PID is not -1, then the thread is printed if it belongs to
   the specified process.  Otherwise, an error is raised.  */
extern void print_thread_info (struct ui_out *uiout,
			       const char *requested_threads,
			       int pid);

/* Save/restore current inferior/thread/frame.  */

class scoped_restore_current_thread
{
public:
  scoped_restore_current_thread ();
  ~scoped_restore_current_thread ();

  DISABLE_COPY_AND_ASSIGN (scoped_restore_current_thread);

  /* Cancel restoring on scope exit.  */
  void dont_restore () { m_dont_restore = true; }

private:
  void restore ();

  bool m_dont_restore = false;
  thread_info_ref m_thread;
  inferior_ref m_inf;

  frame_id m_selected_frame_id;
  int m_selected_frame_level;
  bool m_was_stopped;
};

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

/* Enable storing stack temporaries for thread THR and disable and
   clear the stack temporaries on destruction.  Holds a strong
   reference to THR.  */

class enable_thread_stack_temporaries
{
public:

  explicit enable_thread_stack_temporaries (struct thread_info *thr)
    : m_thr (thr)
  {
    gdb_assert (m_thr != NULL);

    m_thr->incref ();

    m_thr->stack_temporaries_enabled = true;
    m_thr->stack_temporaries.clear ();
  }

  ~enable_thread_stack_temporaries ()
  {
    m_thr->stack_temporaries_enabled = false;
    m_thr->stack_temporaries.clear ();

    m_thr->decref ();
  }

  DISABLE_COPY_AND_ASSIGN (enable_thread_stack_temporaries);

private:

  struct thread_info *m_thr;
};

extern bool thread_stack_temporaries_enabled_p (struct thread_info *tp);

extern void push_thread_stack_temporary (struct thread_info *tp, struct value *v);

extern value *get_last_thread_stack_temporary (struct thread_info *tp);

extern bool value_in_thread_stack_temporaries (struct value *,
					       struct thread_info *thr);

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

/* Check whether it makes sense to access a register of THREAD at this point.
   Returns true if registers may be accessed; false otherwise.  */
extern bool can_access_registers_thread (struct thread_info *thread);

/* Returns whether to show which thread hit the breakpoint, received a
   signal, etc. and ended up causing a user-visible stop.  This is
   true iff we ever detected multiple threads.  */
extern int show_thread_that_caused_stop (void);

/* Print the message for a thread or/and frame selected.  */
extern void print_selected_thread_frame (struct ui_out *uiout,
					 user_selected_what selection);

/* Helper for the CLI's "thread" command and for MI's -thread-select.
   Selects thread THR.  TIDSTR is the original string the thread ID
   was parsed from.  This is used in the error message if THR is not
   alive anymore.  */
extern void thread_select (const char *tidstr, class thread_info *thr);

#endif /* GDBTHREAD_H */
