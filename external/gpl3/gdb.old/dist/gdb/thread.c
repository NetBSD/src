/* Multi-process/thread control for GDB, the GNU debugger.

   Copyright (C) 1986-2020 Free Software Foundation, Inc.

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

#include "defs.h"
#include "symtab.h"
#include "frame.h"
#include "inferior.h"
#include "gdbsupport/environ.h"
#include "value.h"
#include "target.h"
#include "gdbthread.h"
#include "command.h"
#include "gdbcmd.h"
#include "regcache.h"
#include "btrace.h"

#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#include "ui-out.h"
#include "observable.h"
#include "annotate.h"
#include "cli/cli-decode.h"
#include "cli/cli-option.h"
#include "gdb_regex.h"
#include "cli/cli-utils.h"
#include "thread-fsm.h"
#include "tid-parse.h"
#include <algorithm>
#include "gdbsupport/gdb_optional.h"
#include "inline-frame.h"
#include "stack.h"

/* Definition of struct thread_info exported to gdbthread.h.  */

/* Prototypes for local functions.  */

static int highest_thread_num;

/* The current/selected thread.  */
static thread_info *current_thread_;

/* RAII type used to increase / decrease the refcount of each thread
   in a given list of threads.  */

class scoped_inc_dec_ref
{
public:
  explicit scoped_inc_dec_ref (const std::vector<thread_info *> &thrds)
    : m_thrds (thrds)
  {
    for (thread_info *thr : m_thrds)
      thr->incref ();
  }

  ~scoped_inc_dec_ref ()
  {
    for (thread_info *thr : m_thrds)
      thr->decref ();
  }

private:
  const std::vector<thread_info *> &m_thrds;
};

/* Returns true if THR is the current thread.  */

static bool
is_current_thread (const thread_info *thr)
{
  return thr == current_thread_;
}

struct thread_info*
inferior_thread (void)
{
  gdb_assert (current_thread_ != nullptr);
  return current_thread_;
}

/* Delete the breakpoint pointed at by BP_P, if there's one.  */

static void
delete_thread_breakpoint (struct breakpoint **bp_p)
{
  if (*bp_p != NULL)
    {
      delete_breakpoint (*bp_p);
      *bp_p = NULL;
    }
}

void
delete_step_resume_breakpoint (struct thread_info *tp)
{
  if (tp != NULL)
    delete_thread_breakpoint (&tp->control.step_resume_breakpoint);
}

void
delete_exception_resume_breakpoint (struct thread_info *tp)
{
  if (tp != NULL)
    delete_thread_breakpoint (&tp->control.exception_resume_breakpoint);
}

/* See gdbthread.h.  */

void
delete_single_step_breakpoints (struct thread_info *tp)
{
  if (tp != NULL)
    delete_thread_breakpoint (&tp->control.single_step_breakpoints);
}

/* Delete the breakpoint pointed at by BP_P at the next stop, if
   there's one.  */

static void
delete_at_next_stop (struct breakpoint **bp)
{
  if (*bp != NULL)
    {
      (*bp)->disposition = disp_del_at_next_stop;
      *bp = NULL;
    }
}

/* See gdbthread.h.  */

int
thread_has_single_step_breakpoints_set (struct thread_info *tp)
{
  return tp->control.single_step_breakpoints != NULL;
}

/* See gdbthread.h.  */

int
thread_has_single_step_breakpoint_here (struct thread_info *tp,
					const address_space *aspace,
					CORE_ADDR addr)
{
  struct breakpoint *ss_bps = tp->control.single_step_breakpoints;

  return (ss_bps != NULL
	  && breakpoint_has_location_inserted_here (ss_bps, aspace, addr));
}

/* See gdbthread.h.  */

void
thread_cancel_execution_command (struct thread_info *thr)
{
  if (thr->thread_fsm != NULL)
    {
      thr->thread_fsm->clean_up (thr);
      delete thr->thread_fsm;
      thr->thread_fsm = NULL;
    }
}

static void
clear_thread_inferior_resources (struct thread_info *tp)
{
  /* NOTE: this will take care of any left-over step_resume breakpoints,
     but not any user-specified thread-specific breakpoints.  We can not
     delete the breakpoint straight-off, because the inferior might not
     be stopped at the moment.  */
  delete_at_next_stop (&tp->control.step_resume_breakpoint);
  delete_at_next_stop (&tp->control.exception_resume_breakpoint);
  delete_at_next_stop (&tp->control.single_step_breakpoints);

  delete_longjmp_breakpoint_at_next_stop (tp->global_num);

  bpstat_clear (&tp->control.stop_bpstat);

  btrace_teardown (tp);

  thread_cancel_execution_command (tp);

  clear_inline_frame_state (tp);
}

/* Set the TP's state as exited.  */

static void
set_thread_exited (thread_info *tp, bool silent)
{
  /* Dead threads don't need to step-over.  Remove from queue.  */
  if (tp->step_over_next != NULL)
    thread_step_over_chain_remove (tp);

  if (tp->state != THREAD_EXITED)
    {
      gdb::observers::thread_exit.notify (tp, silent);

      /* Tag it as exited.  */
      tp->state = THREAD_EXITED;

      /* Clear breakpoints, etc. associated with this thread.  */
      clear_thread_inferior_resources (tp);
    }
}

void
init_thread_list (void)
{
  highest_thread_num = 0;

  for (thread_info *tp : all_threads_safe ())
    {
      inferior *inf = tp->inf;

      if (tp->deletable ())
	delete tp;
      else
	set_thread_exited (tp, 1);

      inf->thread_list = NULL;
    }
}

/* Allocate a new thread of inferior INF with target id PTID and add
   it to the thread list.  */

static struct thread_info *
new_thread (struct inferior *inf, ptid_t ptid)
{
  thread_info *tp = new thread_info (inf, ptid);

  if (inf->thread_list == NULL)
    inf->thread_list = tp;
  else
    {
      struct thread_info *last;

      for (last = inf->thread_list; last->next != NULL; last = last->next)
	gdb_assert (ptid != last->ptid
		    || last->state == THREAD_EXITED);

      gdb_assert (ptid != last->ptid
		  || last->state == THREAD_EXITED);

      last->next = tp;
    }

  return tp;
}

struct thread_info *
add_thread_silent (process_stratum_target *targ, ptid_t ptid)
{
  inferior *inf = find_inferior_ptid (targ, ptid);

  /* We may have an old thread with the same id in the thread list.
     If we do, it must be dead, otherwise we wouldn't be adding a new
     thread with the same id.  The OS is reusing this id --- delete
     the old thread, and create a new one.  */
  thread_info *tp = find_thread_ptid (inf, ptid);
  if (tp != nullptr)
    delete_thread (tp);

  tp = new_thread (inf, ptid);
  gdb::observers::new_thread.notify (tp);

  return tp;
}

struct thread_info *
add_thread_with_info (process_stratum_target *targ, ptid_t ptid,
		      private_thread_info *priv)
{
  thread_info *result = add_thread_silent (targ, ptid);

  result->priv.reset (priv);

  if (print_thread_events)
    printf_unfiltered (_("[New %s]\n"), target_pid_to_str (ptid).c_str ());

  annotate_new_thread ();
  return result;
}

struct thread_info *
add_thread (process_stratum_target *targ, ptid_t ptid)
{
  return add_thread_with_info (targ, ptid, NULL);
}

private_thread_info::~private_thread_info () = default;

thread_info::thread_info (struct inferior *inf_, ptid_t ptid_)
  : ptid (ptid_), inf (inf_)
{
  gdb_assert (inf_ != NULL);

  this->global_num = ++highest_thread_num;
  this->per_inf_num = ++inf_->highest_thread_num;

  /* Nothing to follow yet.  */
  memset (&this->pending_follow, 0, sizeof (this->pending_follow));
  this->pending_follow.kind = TARGET_WAITKIND_SPURIOUS;
  this->suspend.waitstatus.kind = TARGET_WAITKIND_IGNORE;
}

thread_info::~thread_info ()
{
  xfree (this->name);
}

/* See gdbthread.h.  */

bool
thread_info::deletable () const
{
  /* If this is the current thread, or there's code out there that
     relies on it existing (refcount > 0) we can't delete yet.  */
  return refcount () == 0 && !is_current_thread (this);
}

/* Add TP to the end of the step-over chain LIST_P.  */

static void
step_over_chain_enqueue (struct thread_info **list_p, struct thread_info *tp)
{
  gdb_assert (tp->step_over_next == NULL);
  gdb_assert (tp->step_over_prev == NULL);

  if (*list_p == NULL)
    {
      *list_p = tp;
      tp->step_over_prev = tp->step_over_next = tp;
    }
  else
    {
      struct thread_info *head = *list_p;
      struct thread_info *tail = head->step_over_prev;

      tp->step_over_prev = tail;
      tp->step_over_next = head;
      head->step_over_prev = tp;
      tail->step_over_next = tp;
    }
}

/* Remove TP from step-over chain LIST_P.  */

static void
step_over_chain_remove (struct thread_info **list_p, struct thread_info *tp)
{
  gdb_assert (tp->step_over_next != NULL);
  gdb_assert (tp->step_over_prev != NULL);

  if (*list_p == tp)
    {
      if (tp == tp->step_over_next)
	*list_p = NULL;
      else
	*list_p = tp->step_over_next;
    }

  tp->step_over_prev->step_over_next = tp->step_over_next;
  tp->step_over_next->step_over_prev = tp->step_over_prev;
  tp->step_over_prev = tp->step_over_next = NULL;
}

/* See gdbthread.h.  */

struct thread_info *
thread_step_over_chain_next (struct thread_info *tp)
{
  struct thread_info *next = tp->step_over_next;

  return (next == step_over_queue_head ? NULL : next);
}

/* See gdbthread.h.  */

int
thread_is_in_step_over_chain (struct thread_info *tp)
{
  return (tp->step_over_next != NULL);
}

/* See gdbthread.h.  */

void
thread_step_over_chain_enqueue (struct thread_info *tp)
{
  step_over_chain_enqueue (&step_over_queue_head, tp);
}

/* See gdbthread.h.  */

void
thread_step_over_chain_remove (struct thread_info *tp)
{
  step_over_chain_remove (&step_over_queue_head, tp);
}

/* Delete the thread referenced by THR.  If SILENT, don't notify
   the observer of this exit.
   
   THR must not be NULL or a failed assertion will be raised.  */

static void
delete_thread_1 (thread_info *thr, bool silent)
{
  gdb_assert (thr != nullptr);

  struct thread_info *tp, *tpprev = NULL;

  for (tp = thr->inf->thread_list; tp; tpprev = tp, tp = tp->next)
    if (tp == thr)
      break;

  if (!tp)
    return;

  set_thread_exited (tp, silent);

  if (!tp->deletable ())
    {
       /* Will be really deleted some other time.  */
       return;
     }

  if (tpprev)
    tpprev->next = tp->next;
  else
    tp->inf->thread_list = tp->next;

  delete tp;
}

/* See gdbthread.h.  */

void
delete_thread (thread_info *thread)
{
  delete_thread_1 (thread, false /* not silent */);
}

void
delete_thread_silent (thread_info *thread)
{
  delete_thread_1 (thread, true /* silent */);
}

struct thread_info *
find_thread_global_id (int global_id)
{
  for (thread_info *tp : all_threads ())
    if (tp->global_num == global_id)
      return tp;

  return NULL;
}

static struct thread_info *
find_thread_id (struct inferior *inf, int thr_num)
{
  for (thread_info *tp : inf->threads ())
    if (tp->per_inf_num == thr_num)
      return tp;

  return NULL;
}

/* See gdbthread.h.  */

struct thread_info *
find_thread_ptid (process_stratum_target *targ, ptid_t ptid)
{
  inferior *inf = find_inferior_ptid (targ, ptid);
  if (inf == NULL)
    return NULL;
  return find_thread_ptid (inf, ptid);
}

/* See gdbthread.h.  */

struct thread_info *
find_thread_ptid (inferior *inf, ptid_t ptid)
{
  for (thread_info *tp : inf->non_exited_threads ())
    if (tp->ptid == ptid)
      return tp;

  return NULL;
}

/* See gdbthread.h.  */

struct thread_info *
find_thread_by_handle (gdb::array_view<const gdb_byte> handle,
		       struct inferior *inf)
{
  return target_thread_handle_to_thread_info (handle.data (),
					      handle.size (),
					      inf);
}

/*
 * Thread iterator function.
 *
 * Calls a callback function once for each thread, so long as
 * the callback function returns false.  If the callback function
 * returns true, the iteration will end and the current thread
 * will be returned.  This can be useful for implementing a
 * search for a thread with arbitrary attributes, or for applying
 * some operation to every thread.
 *
 * FIXME: some of the existing functionality, such as
 * "Thread apply all", might be rewritten using this functionality.
 */

struct thread_info *
iterate_over_threads (int (*callback) (struct thread_info *, void *),
		      void *data)
{
  for (thread_info *tp : all_threads_safe ())
    if ((*callback) (tp, data))
      return tp;

  return NULL;
}

/* See gdbthread.h.  */

bool
any_thread_p ()
{
  for (thread_info *tp ATTRIBUTE_UNUSED : all_threads ())
    return true;
  return false;
}

int
thread_count (process_stratum_target *proc_target)
{
  auto rng = all_threads (proc_target);
  return std::distance (rng.begin (), rng.end ());
}

/* Return the number of non-exited threads in the thread list.  */

static int
live_threads_count (void)
{
  auto rng = all_non_exited_threads ();
  return std::distance (rng.begin (), rng.end ());
}

int
valid_global_thread_id (int global_id)
{
  for (thread_info *tp : all_threads ())
    if (tp->global_num == global_id)
      return 1;

  return 0;
}

bool
in_thread_list (process_stratum_target *targ, ptid_t ptid)
{
  return find_thread_ptid (targ, ptid) != nullptr;
}

/* Finds the first thread of the inferior.  */

thread_info *
first_thread_of_inferior (inferior *inf)
{
  return inf->thread_list;
}

thread_info *
any_thread_of_inferior (inferior *inf)
{
  gdb_assert (inf->pid != 0);

  /* Prefer the current thread.  */
  if (inf == current_inferior ())
    return inferior_thread ();

  for (thread_info *tp : inf->non_exited_threads ())
    return tp;

  return NULL;
}

thread_info *
any_live_thread_of_inferior (inferior *inf)
{
  struct thread_info *curr_tp = NULL;
  struct thread_info *tp_executing = NULL;

  gdb_assert (inf != NULL && inf->pid != 0);

  /* Prefer the current thread if it's not executing.  */
  if (inferior_ptid != null_ptid && current_inferior () == inf)
    {
      /* If the current thread is dead, forget it.  If it's not
	 executing, use it.  Otherwise, still choose it (below), but
	 only if no other non-executing thread is found.  */
      curr_tp = inferior_thread ();
      if (curr_tp->state == THREAD_EXITED)
	curr_tp = NULL;
      else if (!curr_tp->executing)
	return curr_tp;
    }

  for (thread_info *tp : inf->non_exited_threads ())
    {
      if (!tp->executing)
	return tp;

      tp_executing = tp;
    }

  /* If both the current thread and all live threads are executing,
     prefer the current thread.  */
  if (curr_tp != NULL)
    return curr_tp;

  /* Otherwise, just return an executing thread, if any.  */
  return tp_executing;
}

/* Return true if TP is an active thread.  */
static bool
thread_alive (thread_info *tp)
{
  if (tp->state == THREAD_EXITED)
    return false;

  /* Ensure we're looking at the right target stack.  */
  gdb_assert (tp->inf == current_inferior ());

  return target_thread_alive (tp->ptid);
}

/* Switch to thread TP if it is alive.  Returns true if successfully
   switched, false otherwise.  */

static bool
switch_to_thread_if_alive (thread_info *thr)
{
  scoped_restore_current_thread restore_thread;

  /* Switch inferior first, so that we're looking at the right target
     stack.  */
  switch_to_inferior_no_thread (thr->inf);

  if (thread_alive (thr))
    {
      switch_to_thread (thr);
      restore_thread.dont_restore ();
      return true;
    }

  return false;
}

/* See gdbthreads.h.  */

void
prune_threads (void)
{
  scoped_restore_current_thread restore_thread;

  for (thread_info *tp : all_threads_safe ())
    {
      switch_to_inferior_no_thread (tp->inf);

      if (!thread_alive (tp))
	delete_thread (tp);
    }
}

/* See gdbthreads.h.  */

void
delete_exited_threads (void)
{
  for (thread_info *tp : all_threads_safe ())
    if (tp->state == THREAD_EXITED)
      delete_thread (tp);
}

/* Return true value if stack temporaries are enabled for the thread
   TP.  */

bool
thread_stack_temporaries_enabled_p (thread_info *tp)
{
  if (tp == NULL)
    return false;
  else
    return tp->stack_temporaries_enabled;
}

/* Push V on to the stack temporaries of the thread with id PTID.  */

void
push_thread_stack_temporary (thread_info *tp, struct value *v)
{
  gdb_assert (tp != NULL && tp->stack_temporaries_enabled);
  tp->stack_temporaries.push_back (v);
}

/* Return true if VAL is among the stack temporaries of the thread
   TP.  Return false otherwise.  */

bool
value_in_thread_stack_temporaries (struct value *val, thread_info *tp)
{
  gdb_assert (tp != NULL && tp->stack_temporaries_enabled);
  for (value *v : tp->stack_temporaries)
    if (v == val)
      return true;

  return false;
}

/* Return the last of the stack temporaries for thread with id PTID.
   Return NULL if there are no stack temporaries for the thread.  */

value *
get_last_thread_stack_temporary (thread_info *tp)
{
  struct value *lastval = NULL;

  gdb_assert (tp != NULL);
  if (!tp->stack_temporaries.empty ())
    lastval = tp->stack_temporaries.back ();

  return lastval;
}

void
thread_change_ptid (process_stratum_target *targ,
		    ptid_t old_ptid, ptid_t new_ptid)
{
  struct inferior *inf;
  struct thread_info *tp;

  /* It can happen that what we knew as the target inferior id
     changes.  E.g, target remote may only discover the remote process
     pid after adding the inferior to GDB's list.  */
  inf = find_inferior_ptid (targ, old_ptid);
  inf->pid = new_ptid.pid ();

  tp = find_thread_ptid (inf, old_ptid);
  tp->ptid = new_ptid;

  gdb::observers::thread_ptid_changed.notify (targ, old_ptid, new_ptid);
}

/* See gdbthread.h.  */

void
set_resumed (process_stratum_target *targ, ptid_t ptid, bool resumed)
{
  for (thread_info *tp : all_non_exited_threads (targ, ptid))
    tp->resumed = resumed;
}

/* Helper for set_running, that marks one thread either running or
   stopped.  */

static bool
set_running_thread (struct thread_info *tp, bool running)
{
  bool started = false;

  if (running && tp->state == THREAD_STOPPED)
    started = true;
  tp->state = running ? THREAD_RUNNING : THREAD_STOPPED;

  if (!running)
    {
      /* If the thread is now marked stopped, remove it from
	 the step-over queue, so that we don't try to resume
	 it until the user wants it to.  */
      if (tp->step_over_next != NULL)
	thread_step_over_chain_remove (tp);
    }

  return started;
}

/* See gdbthread.h.  */

void
thread_info::set_running (bool running)
{
  if (set_running_thread (this, running))
    gdb::observers::target_resumed.notify (this->ptid);
}

void
set_running (process_stratum_target *targ, ptid_t ptid, bool running)
{
  /* We try not to notify the observer if no thread has actually
     changed the running state -- merely to reduce the number of
     messages to the MI frontend.  A frontend is supposed to handle
     multiple *running notifications just fine.  */
  bool any_started = false;

  for (thread_info *tp : all_non_exited_threads (targ, ptid))
    if (set_running_thread (tp, running))
      any_started = true;

  if (any_started)
    gdb::observers::target_resumed.notify (ptid);
}


/* Helper for set_executing.  Set's the thread's 'executing' field
   from EXECUTING, and if EXECUTING is true also clears the thread's
   stop_pc.  */

static void
set_executing_thread (thread_info *thr, bool executing)
{
  thr->executing = executing;
  if (executing)
    thr->suspend.stop_pc = ~(CORE_ADDR) 0;
}

void
set_executing (process_stratum_target *targ, ptid_t ptid, bool executing)
{
  for (thread_info *tp : all_non_exited_threads (targ, ptid))
    set_executing_thread (tp, executing);

  /* It only takes one running thread to spawn more threads.  */
  if (executing)
    targ->threads_executing = true;
  /* Only clear the flag if the caller is telling us everything is
     stopped.  */
  else if (minus_one_ptid == ptid)
    targ->threads_executing = false;
}

/* See gdbthread.h.  */

bool
threads_are_executing (process_stratum_target *target)
{
  return target->threads_executing;
}

void
set_stop_requested (process_stratum_target *targ, ptid_t ptid, bool stop)
{
  for (thread_info *tp : all_non_exited_threads (targ, ptid))
    tp->stop_requested = stop;

  /* Call the stop requested observer so other components of GDB can
     react to this request.  */
  if (stop)
    gdb::observers::thread_stop_requested.notify (ptid);
}

void
finish_thread_state (process_stratum_target *targ, ptid_t ptid)
{
  bool any_started = false;

  for (thread_info *tp : all_non_exited_threads (targ, ptid))
    if (set_running_thread (tp, tp->executing))
      any_started = true;

  if (any_started)
    gdb::observers::target_resumed.notify (ptid);
}

/* See gdbthread.h.  */

void
validate_registers_access (void)
{
  /* No selected thread, no registers.  */
  if (inferior_ptid == null_ptid)
    error (_("No thread selected."));

  thread_info *tp = inferior_thread ();

  /* Don't try to read from a dead thread.  */
  if (tp->state == THREAD_EXITED)
    error (_("The current thread has terminated"));

  /* ... or from a spinning thread.  FIXME: This isn't actually fully
     correct.  It'll allow an user-requested access (e.g., "print $pc"
     at the prompt) when a thread is not executing for some internal
     reason, but is marked running from the user's perspective.  E.g.,
     the thread is waiting for its turn in the step-over queue.  */
  if (tp->executing)
    error (_("Selected thread is running."));
}

/* See gdbthread.h.  */

bool
can_access_registers_thread (thread_info *thread)
{
  /* No thread, no registers.  */
  if (thread == NULL)
    return false;

  /* Don't try to read from a dead thread.  */
  if (thread->state == THREAD_EXITED)
    return false;

  /* ... or from a spinning thread.  FIXME: see validate_registers_access.  */
  if (thread->executing)
    return false;

  return true;
}

int
pc_in_thread_step_range (CORE_ADDR pc, struct thread_info *thread)
{
  return (pc >= thread->control.step_range_start
	  && pc < thread->control.step_range_end);
}

/* Helper for print_thread_info.  Returns true if THR should be
   printed.  If REQUESTED_THREADS, a list of GDB ids/ranges, is not
   NULL, only print THR if its ID is included in the list.  GLOBAL_IDS
   is true if REQUESTED_THREADS is list of global IDs, false if a list
   of per-inferior thread ids.  If PID is not -1, only print THR if it
   is a thread from the process PID.  Otherwise, threads from all
   attached PIDs are printed.  If both REQUESTED_THREADS is not NULL
   and PID is not -1, then the thread is printed if it belongs to the
   specified process.  Otherwise, an error is raised.  */

static int
should_print_thread (const char *requested_threads, int default_inf_num,
		     int global_ids, int pid, struct thread_info *thr)
{
  if (requested_threads != NULL && *requested_threads != '\0')
    {
      int in_list;

      if (global_ids)
	in_list = number_is_in_list (requested_threads, thr->global_num);
      else
	in_list = tid_is_in_list (requested_threads, default_inf_num,
				  thr->inf->num, thr->per_inf_num);
      if (!in_list)
	return 0;
    }

  if (pid != -1 && thr->ptid.pid () != pid)
    {
      if (requested_threads != NULL && *requested_threads != '\0')
	error (_("Requested thread not found in requested process"));
      return 0;
    }

  if (thr->state == THREAD_EXITED)
    return 0;

  return 1;
}

/* Return the string to display in "info threads"'s "Target Id"
   column, for TP.  */

static std::string
thread_target_id_str (thread_info *tp)
{
  std::string target_id = target_pid_to_str (tp->ptid);
  const char *extra_info = target_extra_thread_info (tp);
  const char *name = tp->name != nullptr ? tp->name : target_thread_name (tp);

  if (extra_info != nullptr && name != nullptr)
    return string_printf ("%s \"%s\" (%s)", target_id.c_str (), name,
			  extra_info);
  else if (extra_info != nullptr)
    return string_printf ("%s (%s)", target_id.c_str (), extra_info);
  else if (name != nullptr)
    return string_printf ("%s \"%s\"", target_id.c_str (), name);
  else
    return target_id;
}

/* Like print_thread_info, but in addition, GLOBAL_IDS indicates
   whether REQUESTED_THREADS is a list of global or per-inferior
   thread ids.  */

static void
print_thread_info_1 (struct ui_out *uiout, const char *requested_threads,
		     int global_ids, int pid,
		     int show_global_ids)
{
  int default_inf_num = current_inferior ()->num;

  update_thread_list ();

  /* Whether we saw any thread.  */
  bool any_thread = false;
  /* Whether the current thread is exited.  */
  bool current_exited = false;

  thread_info *current_thread = (inferior_ptid != null_ptid
				 ? inferior_thread () : NULL);

  {
    /* For backward compatibility, we make a list for MI.  A table is
       preferable for the CLI, though, because it shows table
       headers.  */
    gdb::optional<ui_out_emit_list> list_emitter;
    gdb::optional<ui_out_emit_table> table_emitter;

    /* We'll be switching threads temporarily below.  */
    scoped_restore_current_thread restore_thread;

    if (uiout->is_mi_like_p ())
      list_emitter.emplace (uiout, "threads");
    else
      {
	int n_threads = 0;
	/* The width of the "Target Id" column.  Grown below to
	   accommodate the largest entry.  */
	size_t target_id_col_width = 17;

	for (thread_info *tp : all_threads ())
	  {
	    if (!should_print_thread (requested_threads, default_inf_num,
				      global_ids, pid, tp))
	      continue;

	    if (!uiout->is_mi_like_p ())
	      {
		/* Switch inferiors so we're looking at the right
		   target stack.  */
		switch_to_inferior_no_thread (tp->inf);

		target_id_col_width
		  = std::max (target_id_col_width,
			      thread_target_id_str (tp).size ());
	      }

	    ++n_threads;
	  }

	if (n_threads == 0)
	  {
	    if (requested_threads == NULL || *requested_threads == '\0')
	      uiout->message (_("No threads.\n"));
	    else
	      uiout->message (_("No threads match '%s'.\n"),
			      requested_threads);
	    return;
	  }

	table_emitter.emplace (uiout, show_global_ids ? 5 : 4,
			       n_threads, "threads");

	uiout->table_header (1, ui_left, "current", "");
	uiout->table_header (4, ui_left, "id-in-tg", "Id");
	if (show_global_ids)
	  uiout->table_header (4, ui_left, "id", "GId");
	uiout->table_header (target_id_col_width, ui_left,
			     "target-id", "Target Id");
	uiout->table_header (1, ui_left, "frame", "Frame");
	uiout->table_body ();
      }

    for (inferior *inf : all_inferiors ())
      for (thread_info *tp : inf->threads ())
	{
	  int core;

	  any_thread = true;
	  if (tp == current_thread && tp->state == THREAD_EXITED)
	    current_exited = true;

	  if (!should_print_thread (requested_threads, default_inf_num,
				    global_ids, pid, tp))
	    continue;

	  ui_out_emit_tuple tuple_emitter (uiout, NULL);

	  if (!uiout->is_mi_like_p ())
	    {
	      if (tp == current_thread)
		uiout->field_string ("current", "*");
	      else
		uiout->field_skip ("current");

	      uiout->field_string ("id-in-tg", print_thread_id (tp));
	    }

	  if (show_global_ids || uiout->is_mi_like_p ())
	    uiout->field_signed ("id", tp->global_num);

	  /* Switch to the thread (and inferior / target).  */
	  switch_to_thread (tp);

	  /* For the CLI, we stuff everything into the target-id field.
	     This is a gross hack to make the output come out looking
	     correct.  The underlying problem here is that ui-out has no
	     way to specify that a field's space allocation should be
	     shared by several fields.  For MI, we do the right thing
	     instead.  */

	  if (uiout->is_mi_like_p ())
	    {
	      uiout->field_string ("target-id", target_pid_to_str (tp->ptid));

	      const char *extra_info = target_extra_thread_info (tp);
	      if (extra_info != nullptr)
		uiout->field_string ("details", extra_info);

	      const char *name = (tp->name != nullptr
				  ? tp->name
				  : target_thread_name (tp));
	      if (name != NULL)
		uiout->field_string ("name", name);
	    }
	  else
	    {
	      uiout->field_string ("target-id",
				   thread_target_id_str (tp).c_str ());
	    }

	  if (tp->state == THREAD_RUNNING)
	    uiout->text ("(running)\n");
	  else
	    {
	      /* The switch above put us at the top of the stack (leaf
		 frame).  */
	      print_stack_frame (get_selected_frame (NULL),
				 /* For MI output, print frame level.  */
				 uiout->is_mi_like_p (),
				 LOCATION, 0);
	    }

	  if (uiout->is_mi_like_p ())
	    {
	      const char *state = "stopped";

	      if (tp->state == THREAD_RUNNING)
		state = "running";
	      uiout->field_string ("state", state);
	    }

	  core = target_core_of_thread (tp->ptid);
	  if (uiout->is_mi_like_p () && core != -1)
	    uiout->field_signed ("core", core);
	}

    /* This end scope restores the current thread and the frame
       selected before the "info threads" command, and it finishes the
       ui-out list or table.  */
  }

  if (pid == -1 && requested_threads == NULL)
    {
      if (uiout->is_mi_like_p () && inferior_ptid != null_ptid)
	uiout->field_signed ("current-thread-id", current_thread->global_num);

      if (inferior_ptid != null_ptid && current_exited)
	uiout->message ("\n\
The current thread <Thread ID %s> has terminated.  See `help thread'.\n",
			print_thread_id (inferior_thread ()));
      else if (any_thread && inferior_ptid == null_ptid)
	uiout->message ("\n\
No selected thread.  See `help thread'.\n");
    }
}

/* See gdbthread.h.  */

void
print_thread_info (struct ui_out *uiout, const char *requested_threads,
		   int pid)
{
  print_thread_info_1 (uiout, requested_threads, 1, pid, 0);
}

/* The options for the "info threads" command.  */

struct info_threads_opts
{
  /* For "-gid".  */
  bool show_global_ids = false;
};

static const gdb::option::option_def info_threads_option_defs[] = {

  gdb::option::flag_option_def<info_threads_opts> {
    "gid",
    [] (info_threads_opts *opts) { return &opts->show_global_ids; },
    N_("Show global thread IDs."),
  },

};

/* Create an option_def_group for the "info threads" options, with
   IT_OPTS as context.  */

static inline gdb::option::option_def_group
make_info_threads_options_def_group (info_threads_opts *it_opts)
{
  return {{info_threads_option_defs}, it_opts};
}

/* Implementation of the "info threads" command.

   Note: this has the drawback that it _really_ switches
	 threads, which frees the frame cache.  A no-side
	 effects info-threads command would be nicer.  */

static void
info_threads_command (const char *arg, int from_tty)
{
  info_threads_opts it_opts;

  auto grp = make_info_threads_options_def_group (&it_opts);
  gdb::option::process_options
    (&arg, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, grp);

  print_thread_info_1 (current_uiout, arg, 0, -1, it_opts.show_global_ids);
}

/* Completer for the "info threads" command.  */

static void
info_threads_command_completer (struct cmd_list_element *ignore,
				completion_tracker &tracker,
				const char *text, const char *word_ignored)
{
  const auto grp = make_info_threads_options_def_group (nullptr);

  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, grp))
    return;

  /* Convenience to let the user know what the option can accept.  */
  if (*text == '\0')
    {
      gdb::option::complete_on_all_options (tracker, grp);
      /* Keep this "ID" in sync with what "help info threads"
	 says.  */
      tracker.add_completion (make_unique_xstrdup ("ID"));
    }
}

/* See gdbthread.h.  */

void
switch_to_thread_no_regs (struct thread_info *thread)
{
  struct inferior *inf = thread->inf;

  set_current_program_space (inf->pspace);
  set_current_inferior (inf);

  current_thread_ = thread;
  inferior_ptid = current_thread_->ptid;
}

/* See gdbthread.h.  */

void
switch_to_no_thread ()
{
  if (current_thread_ == nullptr)
    return;

  current_thread_ = nullptr;
  inferior_ptid = null_ptid;
  reinit_frame_cache ();
}

/* See gdbthread.h.  */

void
switch_to_thread (thread_info *thr)
{
  gdb_assert (thr != NULL);

  if (is_current_thread (thr))
    return;

  switch_to_thread_no_regs (thr);

  reinit_frame_cache ();
}

/* See gdbsupport/common-gdbthread.h.  */

void
switch_to_thread (process_stratum_target *proc_target, ptid_t ptid)
{
  thread_info *thr = find_thread_ptid (proc_target, ptid);
  switch_to_thread (thr);
}

static void
restore_selected_frame (struct frame_id a_frame_id, int frame_level)
{
  struct frame_info *frame = NULL;
  int count;

  /* This means there was no selected frame.  */
  if (frame_level == -1)
    {
      select_frame (NULL);
      return;
    }

  gdb_assert (frame_level >= 0);

  /* Restore by level first, check if the frame id is the same as
     expected.  If that fails, try restoring by frame id.  If that
     fails, nothing to do, just warn the user.  */

  count = frame_level;
  frame = find_relative_frame (get_current_frame (), &count);
  if (count == 0
      && frame != NULL
      /* The frame ids must match - either both valid or both outer_frame_id.
	 The latter case is not failsafe, but since it's highly unlikely
	 the search by level finds the wrong frame, it's 99.9(9)% of
	 the time (for all practical purposes) safe.  */
      && frame_id_eq (get_frame_id (frame), a_frame_id))
    {
      /* Cool, all is fine.  */
      select_frame (frame);
      return;
    }

  frame = frame_find_by_id (a_frame_id);
  if (frame != NULL)
    {
      /* Cool, refound it.  */
      select_frame (frame);
      return;
    }

  /* Nothing else to do, the frame layout really changed.  Select the
     innermost stack frame.  */
  select_frame (get_current_frame ());

  /* Warn the user.  */
  if (frame_level > 0 && !current_uiout->is_mi_like_p ())
    {
      warning (_("Couldn't restore frame #%d in "
		 "current thread.  Bottom (innermost) frame selected:"),
	       frame_level);
      /* For MI, we should probably have a notification about
	 current frame change.  But this error is not very
	 likely, so don't bother for now.  */
      print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
    }
}

void
scoped_restore_current_thread::restore ()
{
  /* If an entry of thread_info was previously selected, it won't be
     deleted because we've increased its refcount.  The thread represented
     by this thread_info entry may have already exited (due to normal exit,
     detach, etc), so the thread_info.state is THREAD_EXITED.  */
  if (m_thread != NULL
      /* If the previously selected thread belonged to a process that has
	 in the mean time exited (or killed, detached, etc.), then don't revert
	 back to it, but instead simply drop back to no thread selected.  */
      && m_inf->pid != 0)
    switch_to_thread (m_thread.get ());
  else
    switch_to_inferior_no_thread (m_inf.get ());

  /* The running state of the originally selected thread may have
     changed, so we have to recheck it here.  */
  if (inferior_ptid != null_ptid
      && m_was_stopped
      && m_thread->state == THREAD_STOPPED
      && target_has_registers
      && target_has_stack
      && target_has_memory)
    restore_selected_frame (m_selected_frame_id, m_selected_frame_level);
}

scoped_restore_current_thread::~scoped_restore_current_thread ()
{
  if (!m_dont_restore)
    {
      try
	{
	  restore ();
	}
      catch (const gdb_exception &ex)
	{
	  /* We're in a dtor, there's really nothing else we can do
	     but swallow the exception.  */
	}
    }
}

scoped_restore_current_thread::scoped_restore_current_thread ()
{
  m_inf = inferior_ref::new_reference (current_inferior ());

  if (inferior_ptid != null_ptid)
    {
      m_thread = thread_info_ref::new_reference (inferior_thread ());

      struct frame_info *frame;

      m_was_stopped = m_thread->state == THREAD_STOPPED;
      if (m_was_stopped
	  && target_has_registers
	  && target_has_stack
	  && target_has_memory)
	{
	  /* When processing internal events, there might not be a
	     selected frame.  If we naively call get_selected_frame
	     here, then we can end up reading debuginfo for the
	     current frame, but we don't generally need the debuginfo
	     at this point.  */
	  frame = get_selected_frame_if_set ();
	}
      else
	frame = NULL;

      try
	{
	  m_selected_frame_id = get_frame_id (frame);
	  m_selected_frame_level = frame_relative_level (frame);
	}
      catch (const gdb_exception_error &ex)
	{
	  m_selected_frame_id = null_frame_id;
	  m_selected_frame_level = -1;

	  /* Better let this propagate.  */
	  if (ex.error == TARGET_CLOSE_ERROR)
	    throw;
	}
    }
}

/* See gdbthread.h.  */

int
show_thread_that_caused_stop (void)
{
  return highest_thread_num > 1;
}

/* See gdbthread.h.  */

int
show_inferior_qualified_tids (void)
{
  return (inferior_list->next != NULL || inferior_list->num != 1);
}

/* See gdbthread.h.  */

const char *
print_thread_id (struct thread_info *thr)
{
  char *s = get_print_cell ();

  if (show_inferior_qualified_tids ())
    xsnprintf (s, PRINT_CELL_SIZE, "%d.%d", thr->inf->num, thr->per_inf_num);
  else
    xsnprintf (s, PRINT_CELL_SIZE, "%d", thr->per_inf_num);
  return s;
}

/* Sort an array of struct thread_info pointers by thread ID (first by
   inferior number, and then by per-inferior thread number).  Sorts in
   ascending order.  */

static bool
tp_array_compar_ascending (const thread_info *a, const thread_info *b)
{
  if (a->inf->num != b->inf->num)
    return a->inf->num < b->inf->num;

  return (a->per_inf_num < b->per_inf_num);
}

/* Sort an array of struct thread_info pointers by thread ID (first by
   inferior number, and then by per-inferior thread number).  Sorts in
   descending order.  */

static bool
tp_array_compar_descending (const thread_info *a, const thread_info *b)
{
  if (a->inf->num != b->inf->num)
    return a->inf->num > b->inf->num;

  return (a->per_inf_num > b->per_inf_num);
}

/* Switch to thread THR and execute CMD.
   FLAGS.QUIET controls the printing of the thread information.
   FLAGS.CONT and FLAGS.SILENT control how to handle errors.  */

static void
thr_try_catch_cmd (thread_info *thr, const char *cmd, int from_tty,
		   const qcs_flags &flags)
{
  switch_to_thread (thr);

  /* The thread header is computed before running the command since
     the command can change the inferior, which is not permitted
     by thread_target_id_str.  */
  std::string thr_header =
    string_printf (_("\nThread %s (%s):\n"), print_thread_id (thr),
		   thread_target_id_str (thr).c_str ());

  try
    {
      std::string cmd_result = execute_command_to_string
	(cmd, from_tty, gdb_stdout->term_out ());
      if (!flags.silent || cmd_result.length () > 0)
	{
	  if (!flags.quiet)
	    printf_filtered ("%s", thr_header.c_str ());
	  printf_filtered ("%s", cmd_result.c_str ());
	}
    }
  catch (const gdb_exception_error &ex)
    {
      if (!flags.silent)
	{
	  if (!flags.quiet)
	    printf_filtered ("%s", thr_header.c_str ());
	  if (flags.cont)
	    printf_filtered ("%s\n", ex.what ());
	  else
	    throw;
	}
    }
}

/* Option definition of "thread apply"'s "-ascending" option.  */

static const gdb::option::flag_option_def<> ascending_option_def = {
  "ascending",
  N_("\
Call COMMAND for all threads in ascending order.\n\
The default is descending order."),
};

/* The qcs command line flags for the "thread apply" commands.  Keep
   this in sync with the "frame apply" commands.  */

using qcs_flag_option_def
  = gdb::option::flag_option_def<qcs_flags>;

static const gdb::option::option_def thr_qcs_flags_option_defs[] = {
  qcs_flag_option_def {
    "q", [] (qcs_flags *opt) { return &opt->quiet; },
    N_("Disables printing the thread information."),
  },

  qcs_flag_option_def {
    "c", [] (qcs_flags *opt) { return &opt->cont; },
    N_("Print any error raised by COMMAND and continue."),
  },

  qcs_flag_option_def {
    "s", [] (qcs_flags *opt) { return &opt->silent; },
    N_("Silently ignore any errors or empty output produced by COMMAND."),
  },
};

/* Create an option_def_group for the "thread apply all" options, with
   ASCENDING and FLAGS as context.  */

static inline std::array<gdb::option::option_def_group, 2>
make_thread_apply_all_options_def_group (bool *ascending,
					 qcs_flags *flags)
{
  return {{
    { {ascending_option_def.def ()}, ascending},
    { {thr_qcs_flags_option_defs}, flags },
  }};
}

/* Create an option_def_group for the "thread apply" options, with
   FLAGS as context.  */

static inline gdb::option::option_def_group
make_thread_apply_options_def_group (qcs_flags *flags)
{
  return {{thr_qcs_flags_option_defs}, flags};
}

/* Apply a GDB command to a list of threads.  List syntax is a whitespace
   separated list of numbers, or ranges, or the keyword `all'.  Ranges consist
   of two numbers separated by a hyphen.  Examples:

   thread apply 1 2 7 4 backtrace       Apply backtrace cmd to threads 1,2,7,4
   thread apply 2-7 9 p foo(1)  Apply p foo(1) cmd to threads 2->7 & 9
   thread apply all x/i $pc   Apply x/i $pc cmd to all threads.  */

static void
thread_apply_all_command (const char *cmd, int from_tty)
{
  bool ascending = false;
  qcs_flags flags;

  auto group = make_thread_apply_all_options_def_group (&ascending,
							&flags);
  gdb::option::process_options
    (&cmd, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group);

  validate_flags_qcs ("thread apply all", &flags);

  if (cmd == NULL || *cmd == '\000')
    error (_("Please specify a command at the end of 'thread apply all'"));

  update_thread_list ();

  int tc = live_threads_count ();
  if (tc != 0)
    {
      /* Save a copy of the thread list and increment each thread's
	 refcount while executing the command in the context of each
	 thread, in case the command is one that wipes threads.  E.g.,
	 detach, kill, disconnect, etc., or even normally continuing
	 over an inferior or thread exit.  */
      std::vector<thread_info *> thr_list_cpy;
      thr_list_cpy.reserve (tc);

      for (thread_info *tp : all_non_exited_threads ())
	thr_list_cpy.push_back (tp);
      gdb_assert (thr_list_cpy.size () == tc);

      /* Increment the refcounts, and restore them back on scope
	 exit.  */
      scoped_inc_dec_ref inc_dec_ref (thr_list_cpy);

      auto *sorter = (ascending
		      ? tp_array_compar_ascending
		      : tp_array_compar_descending);
      std::sort (thr_list_cpy.begin (), thr_list_cpy.end (), sorter);

      scoped_restore_current_thread restore_thread;

      for (thread_info *thr : thr_list_cpy)
	if (switch_to_thread_if_alive (thr))
	  thr_try_catch_cmd (thr, cmd, from_tty, flags);
    }
}

/* Completer for "thread apply [ID list]".  */

static void
thread_apply_command_completer (cmd_list_element *ignore,
				completion_tracker &tracker,
				const char *text, const char * /*word*/)
{
  /* Don't leave this to complete_options because there's an early
     return below.  */
  tracker.set_use_custom_word_point (true);

  tid_range_parser parser;
  parser.init (text, current_inferior ()->num);

  try
    {
      while (!parser.finished ())
	{
	  int inf_num, thr_start, thr_end;

	  if (!parser.get_tid_range (&inf_num, &thr_start, &thr_end))
	    break;

	  if (parser.in_star_range () || parser.in_thread_range ())
	    parser.skip_range ();
	}
    }
  catch (const gdb_exception_error &ex)
    {
      /* get_tid_range throws if it parses a negative number, for
	 example.  But a seemingly negative number may be the start of
	 an option instead.  */
    }

  const char *cmd = parser.cur_tok ();

  if (cmd == text)
    {
      /* No thread ID list yet.  */
      return;
    }

  /* Check if we're past a valid thread ID list already.  */
  if (parser.finished ()
      && cmd > text && !isspace (cmd[-1]))
    return;

  /* We're past the thread ID list, advance word point.  */
  tracker.advance_custom_word_point_by (cmd - text);
  text = cmd;

  const auto group = make_thread_apply_options_def_group (nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group))
    return;

  complete_nested_command_line (tracker, text);
}

/* Completer for "thread apply all".  */

static void
thread_apply_all_command_completer (cmd_list_element *ignore,
				    completion_tracker &tracker,
				    const char *text, const char *word)
{
  const auto group = make_thread_apply_all_options_def_group (nullptr,
							      nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group))
    return;

  complete_nested_command_line (tracker, text);
}

/* Implementation of the "thread apply" command.  */

static void
thread_apply_command (const char *tidlist, int from_tty)
{
  qcs_flags flags;
  const char *cmd = NULL;
  tid_range_parser parser;

  if (tidlist == NULL || *tidlist == '\000')
    error (_("Please specify a thread ID list"));

  parser.init (tidlist, current_inferior ()->num);
  while (!parser.finished ())
    {
      int inf_num, thr_start, thr_end;

      if (!parser.get_tid_range (&inf_num, &thr_start, &thr_end))
	break;
    }

  cmd = parser.cur_tok ();

  auto group = make_thread_apply_options_def_group (&flags);
  gdb::option::process_options
    (&cmd, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group);

  validate_flags_qcs ("thread apply", &flags);

  if (*cmd == '\0')
    error (_("Please specify a command following the thread ID list"));

  if (tidlist == cmd || isdigit (cmd[0]))
    invalid_thread_id_error (cmd);

  scoped_restore_current_thread restore_thread;

  parser.init (tidlist, current_inferior ()->num);
  while (!parser.finished ())
    {
      struct thread_info *tp = NULL;
      struct inferior *inf;
      int inf_num, thr_num;

      parser.get_tid (&inf_num, &thr_num);
      inf = find_inferior_id (inf_num);
      if (inf != NULL)
	tp = find_thread_id (inf, thr_num);

      if (parser.in_star_range ())
	{
	  if (inf == NULL)
	    {
	      warning (_("Unknown inferior %d"), inf_num);
	      parser.skip_range ();
	      continue;
	    }

	  /* No use looking for threads past the highest thread number
	     the inferior ever had.  */
	  if (thr_num >= inf->highest_thread_num)
	    parser.skip_range ();

	  /* Be quiet about unknown threads numbers.  */
	  if (tp == NULL)
	    continue;
	}

      if (tp == NULL)
	{
	  if (show_inferior_qualified_tids () || parser.tid_is_qualified ())
	    warning (_("Unknown thread %d.%d"), inf_num, thr_num);
	  else
	    warning (_("Unknown thread %d"), thr_num);
	  continue;
	}

      if (!switch_to_thread_if_alive (tp))
	{
	  warning (_("Thread %s has terminated."), print_thread_id (tp));
	  continue;
	}

      thr_try_catch_cmd (tp, cmd, from_tty, flags);
    }
}


/* Implementation of the "taas" command.  */

static void
taas_command (const char *cmd, int from_tty)
{
  if (cmd == NULL || *cmd == '\0')
    error (_("Please specify a command to apply on all threads"));
  std::string expanded = std::string ("thread apply all -s ") + cmd;
  execute_command (expanded.c_str (), from_tty);
}

/* Implementation of the "tfaas" command.  */

static void
tfaas_command (const char *cmd, int from_tty)
{
  if (cmd == NULL || *cmd == '\0')
    error (_("Please specify a command to apply on all frames of all threads"));
  std::string expanded
    = std::string ("thread apply all -s -- frame apply all -s ") + cmd;
  execute_command (expanded.c_str (), from_tty);
}

/* Switch to the specified thread, or print the current thread.  */

void
thread_command (const char *tidstr, int from_tty)
{
  if (tidstr == NULL)
    {
      if (inferior_ptid == null_ptid)
	error (_("No thread selected"));

      if (target_has_stack)
	{
	  struct thread_info *tp = inferior_thread ();

	  if (tp->state == THREAD_EXITED)
	    printf_filtered (_("[Current thread is %s (%s) (exited)]\n"),
			     print_thread_id (tp),
			     target_pid_to_str (inferior_ptid).c_str ());
	  else
	    printf_filtered (_("[Current thread is %s (%s)]\n"),
			     print_thread_id (tp),
			     target_pid_to_str (inferior_ptid).c_str ());
	}
      else
	error (_("No stack."));
    }
  else
    {
      ptid_t previous_ptid = inferior_ptid;

      thread_select (tidstr, parse_thread_id (tidstr, NULL));

      /* Print if the thread has not changed, otherwise an event will
	 be sent.  */
      if (inferior_ptid == previous_ptid)
	{
	  print_selected_thread_frame (current_uiout,
				       USER_SELECTED_THREAD
				       | USER_SELECTED_FRAME);
	}
      else
	{
	  gdb::observers::user_selected_context_changed.notify
	    (USER_SELECTED_THREAD | USER_SELECTED_FRAME);
	}
    }
}

/* Implementation of `thread name'.  */

static void
thread_name_command (const char *arg, int from_tty)
{
  struct thread_info *info;

  if (inferior_ptid == null_ptid)
    error (_("No thread selected"));

  arg = skip_spaces (arg);

  info = inferior_thread ();
  xfree (info->name);
  info->name = arg ? xstrdup (arg) : NULL;
}

/* Find thread ids with a name, target pid, or extra info matching ARG.  */

static void
thread_find_command (const char *arg, int from_tty)
{
  const char *tmp;
  unsigned long match = 0;

  if (arg == NULL || *arg == '\0')
    error (_("Command requires an argument."));

  tmp = re_comp (arg);
  if (tmp != 0)
    error (_("Invalid regexp (%s): %s"), tmp, arg);

  update_thread_list ();
  for (thread_info *tp : all_threads ())
    {
      if (tp->name != NULL && re_exec (tp->name))
	{
	  printf_filtered (_("Thread %s has name '%s'\n"),
			   print_thread_id (tp), tp->name);
	  match++;
	}

      tmp = target_thread_name (tp);
      if (tmp != NULL && re_exec (tmp))
	{
	  printf_filtered (_("Thread %s has target name '%s'\n"),
			   print_thread_id (tp), tmp);
	  match++;
	}

      std::string name = target_pid_to_str (tp->ptid);
      if (!name.empty () && re_exec (name.c_str ()))
	{
	  printf_filtered (_("Thread %s has target id '%s'\n"),
			   print_thread_id (tp), name.c_str ());
	  match++;
	}

      tmp = target_extra_thread_info (tp);
      if (tmp != NULL && re_exec (tmp))
	{
	  printf_filtered (_("Thread %s has extra info '%s'\n"),
			   print_thread_id (tp), tmp);
	  match++;
	}
    }
  if (!match)
    printf_filtered (_("No threads match '%s'\n"), arg);
}

/* Print notices when new threads are attached and detached.  */
bool print_thread_events = true;
static void
show_print_thread_events (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Printing of thread events is %s.\n"),
		    value);
}

/* See gdbthread.h.  */

void
thread_select (const char *tidstr, thread_info *tp)
{
  if (!switch_to_thread_if_alive (tp))
    error (_("Thread ID %s has terminated."), tidstr);

  annotate_thread_changed ();

  /* Since the current thread may have changed, see if there is any
     exited thread we can now delete.  */
  delete_exited_threads ();
}

/* Print thread and frame switch command response.  */

void
print_selected_thread_frame (struct ui_out *uiout,
			     user_selected_what selection)
{
  struct thread_info *tp = inferior_thread ();

  if (selection & USER_SELECTED_THREAD)
    {
      if (uiout->is_mi_like_p ())
	{
	  uiout->field_signed ("new-thread-id",
			       inferior_thread ()->global_num);
	}
      else
	{
	  uiout->text ("[Switching to thread ");
	  uiout->field_string ("new-thread-id", print_thread_id (tp));
	  uiout->text (" (");
	  uiout->text (target_pid_to_str (inferior_ptid).c_str ());
	  uiout->text (")]");
	}
    }

  if (tp->state == THREAD_RUNNING)
    {
      if (selection & USER_SELECTED_THREAD)
	uiout->text ("(running)\n");
    }
  else if (selection & USER_SELECTED_FRAME)
    {
      if (selection & USER_SELECTED_THREAD)
	uiout->text ("\n");

      if (has_stack_frames ())
	print_stack_frame_to_uiout (uiout, get_selected_frame (NULL),
				    1, SRC_AND_LOC, 1);
    }
}

/* Update the 'threads_executing' global based on the threads we know
   about right now.  This is used by infrun to tell whether we should
   pull events out of the current target.  */

static void
update_threads_executing (void)
{
  process_stratum_target *targ = current_inferior ()->process_target ();

  if (targ == NULL)
    return;

  targ->threads_executing = false;

  for (inferior *inf : all_non_exited_inferiors (targ))
    {
      if (!inf->has_execution ())
	continue;

      /* If the process has no threads, then it must be we have a
	 process-exit event pending.  */
      if (inf->thread_list == NULL)
	{
	  targ->threads_executing = true;
	  return;
	}

      for (thread_info *tp : inf->non_exited_threads ())
	{
	  if (tp->executing)
	    {
	      targ->threads_executing = true;
	      return;
	    }
	}
    }
}

void
update_thread_list (void)
{
  target_update_thread_list ();
  update_threads_executing ();
}

/* Return a new value for the selected thread's id.  Return a value of
   0 if no thread is selected.  If GLOBAL is true, return the thread's
   global number.  Otherwise return the per-inferior number.  */

static struct value *
thread_num_make_value_helper (struct gdbarch *gdbarch, int global)
{
  int int_val;

  if (inferior_ptid == null_ptid)
    int_val = 0;
  else
    {
      thread_info *tp = inferior_thread ();
      if (global)
	int_val = tp->global_num;
      else
	int_val = tp->per_inf_num;
    }

  return value_from_longest (builtin_type (gdbarch)->builtin_int, int_val);
}

/* Return a new value for the selected thread's per-inferior thread
   number.  Return a value of 0 if no thread is selected, or no
   threads exist.  */

static struct value *
thread_id_per_inf_num_make_value (struct gdbarch *gdbarch,
				  struct internalvar *var,
				  void *ignore)
{
  return thread_num_make_value_helper (gdbarch, 0);
}

/* Return a new value for the selected thread's global id.  Return a
   value of 0 if no thread is selected, or no threads exist.  */

static struct value *
global_thread_id_make_value (struct gdbarch *gdbarch, struct internalvar *var,
			     void *ignore)
{
  return thread_num_make_value_helper (gdbarch, 1);
}

/* Commands with a prefix of `thread'.  */
struct cmd_list_element *thread_cmd_list = NULL;

/* Implementation of `thread' variable.  */

static const struct internalvar_funcs thread_funcs =
{
  thread_id_per_inf_num_make_value,
  NULL,
  NULL
};

/* Implementation of `gthread' variable.  */

static const struct internalvar_funcs gthread_funcs =
{
  global_thread_id_make_value,
  NULL,
  NULL
};

void _initialize_thread ();
void
_initialize_thread ()
{
  static struct cmd_list_element *thread_apply_list = NULL;
  cmd_list_element *c;

  const auto info_threads_opts = make_info_threads_options_def_group (nullptr);

  /* Note: keep this "ID" in sync with what "info threads [TAB]"
     suggests.  */
  static std::string info_threads_help
    = gdb::option::build_help (_("\
Display currently known threads.\n\
Usage: info threads [OPTION]... [ID]...\n\
\n\
Options:\n\
%OPTIONS%\
If ID is given, it is a space-separated list of IDs of threads to display.\n\
Otherwise, all threads are displayed."),
			       info_threads_opts);

  c = add_info ("threads", info_threads_command, info_threads_help.c_str ());
  set_cmd_completer_handle_brkchars (c, info_threads_command_completer);

  add_prefix_cmd ("thread", class_run, thread_command, _("\
Use this command to switch between threads.\n\
The new thread ID must be currently known."),
		  &thread_cmd_list, "thread ", 1, &cmdlist);

#define THREAD_APPLY_OPTION_HELP "\
Prints per-inferior thread number and target system's thread id\n\
followed by COMMAND output.\n\
\n\
By default, an error raised during the execution of COMMAND\n\
aborts \"thread apply\".\n\
\n\
Options:\n\
%OPTIONS%"

  const auto thread_apply_opts = make_thread_apply_options_def_group (nullptr);

  static std::string thread_apply_help = gdb::option::build_help (_("\
Apply a command to a list of threads.\n\
Usage: thread apply ID... [OPTION]... COMMAND\n\
ID is a space-separated list of IDs of threads to apply COMMAND on.\n"
THREAD_APPLY_OPTION_HELP),
			       thread_apply_opts);

  c = add_prefix_cmd ("apply", class_run, thread_apply_command,
		      thread_apply_help.c_str (),
		      &thread_apply_list, "thread apply ", 1,
		      &thread_cmd_list);
  set_cmd_completer_handle_brkchars (c, thread_apply_command_completer);

  const auto thread_apply_all_opts
    = make_thread_apply_all_options_def_group (nullptr, nullptr);

  static std::string thread_apply_all_help = gdb::option::build_help (_("\
Apply a command to all threads.\n\
\n\
Usage: thread apply all [OPTION]... COMMAND\n"
THREAD_APPLY_OPTION_HELP),
			       thread_apply_all_opts);

  c = add_cmd ("all", class_run, thread_apply_all_command,
	       thread_apply_all_help.c_str (),
	       &thread_apply_list);
  set_cmd_completer_handle_brkchars (c, thread_apply_all_command_completer);

  c = add_com ("taas", class_run, taas_command, _("\
Apply a command to all threads (ignoring errors and empty output).\n\
Usage: taas [OPTION]... COMMAND\n\
shortcut for 'thread apply all -s [OPTION]... COMMAND'\n\
See \"help thread apply all\" for available options."));
  set_cmd_completer_handle_brkchars (c, thread_apply_all_command_completer);

  c = add_com ("tfaas", class_run, tfaas_command, _("\
Apply a command to all frames of all threads (ignoring errors and empty output).\n\
Usage: tfaas [OPTION]... COMMAND\n\
shortcut for 'thread apply all -s -- frame apply all -s [OPTION]... COMMAND'\n\
See \"help frame apply all\" for available options."));
  set_cmd_completer_handle_brkchars (c, frame_apply_all_cmd_completer);

  add_cmd ("name", class_run, thread_name_command,
	   _("Set the current thread's name.\n\
Usage: thread name [NAME]\n\
If NAME is not given, then any existing name is removed."), &thread_cmd_list);

  add_cmd ("find", class_run, thread_find_command, _("\
Find threads that match a regular expression.\n\
Usage: thread find REGEXP\n\
Will display thread ids whose name, target ID, or extra info matches REGEXP."),
	   &thread_cmd_list);

  add_com_alias ("t", "thread", class_run, 1);

  add_setshow_boolean_cmd ("thread-events", no_class,
			   &print_thread_events, _("\
Set printing of thread events (such as thread start and exit)."), _("\
Show printing of thread events (such as thread start and exit)."), NULL,
			   NULL,
			   show_print_thread_events,
			   &setprintlist, &showprintlist);

  create_internalvar_type_lazy ("_thread", &thread_funcs, NULL);
  create_internalvar_type_lazy ("_gthread", &gthread_funcs, NULL);
}
