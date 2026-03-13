/* Multi-thread control defs for remote server for GDB.
   Copyright (C) 1993-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_GDBTHREAD_H
#define GDBSERVER_GDBTHREAD_H

#include "gdbsupport/function-view.h"
#include "gdbsupport/intrusive_list.h"

struct btrace_target_info;
struct regcache;

struct thread_info : public intrusive_list_node<thread_info>
{
  thread_info (ptid_t id, process_info *process, void *target_data)
    : id (id), m_process (process), m_target_data (target_data)
  {}

  ~thread_info ()
  {
    free_register_cache (m_regcache);
  }

  /* Return the process owning this thread.  */
  process_info *process () const
  { return m_process; }

  struct regcache *regcache ()
  { return m_regcache; }

  void set_regcache (struct regcache *regcache)
  { m_regcache = regcache; }

  void *target_data ()
  { return m_target_data; }

  /* The id of this thread.  */
  ptid_t id;

  /* The last resume GDB requested on this thread.  */
  enum resume_kind last_resume_kind = resume_continue;

  /* The last wait status reported for this thread.  */
  struct target_waitstatus last_status;

  /* True if LAST_STATUS hasn't been reported to GDB yet.  */
  int status_pending_p = 0;

  /* Given `while-stepping', a thread may be collecting data for more
     than one tracepoint simultaneously.  E.g.:

    ff0001  INSN1 <-- TP1, while-stepping 10 collect $regs
    ff0002  INSN2
    ff0003  INSN3 <-- TP2, collect $regs
    ff0004  INSN4 <-- TP3, while-stepping 10 collect $regs
    ff0005  INSN5

   Notice that when instruction INSN5 is reached, the while-stepping
   actions of both TP1 and TP3 are still being collected, and that TP2
   had been collected meanwhile.  The whole range of ff0001-ff0005
   should be single-stepped, due to at least TP1's while-stepping
   action covering the whole range.

   On the other hand, the same tracepoint with a while-stepping action
   may be hit by more than one thread simultaneously, hence we can't
   keep the current step count in the tracepoint itself.

   This is the head of the list of the states of `while-stepping'
   tracepoint actions this thread is now collecting; NULL if empty.
   Each item in the list holds the current step of the while-stepping
   action.  */
  struct wstep_state *while_stepping = nullptr;

  /* Branch trace target information for this thread.  */
  struct btrace_target_info *btrace = nullptr;

  /* Thread options GDB requested with QThreadOptions.  */
  gdb_thread_options thread_options = 0;
  
private:
  process_info *m_process;
  struct regcache *m_regcache = nullptr;
  void *m_target_data;
};

/* Return a pointer to the first thread, or NULL if there isn't one.  */

thread_info *get_first_thread (void);

thread_info *find_thread_ptid (ptid_t ptid);

/* Find any thread of the PID process.  Returns NULL if none is
   found.  */
thread_info *find_any_thread_of_pid (int pid);

/* Find the first thread for which FUNC returns true.  Return NULL if no thread
   satisfying FUNC is found.  */

thread_info *find_thread (gdb::function_view<bool (thread_info *)> func);

/* Like the above, but only consider threads with pid PID.  */

thread_info *find_thread (int pid,
			  gdb::function_view<bool (thread_info *)> func);

/* Find the first thread that matches FILTER for which FUNC returns true.
   Return NULL if no thread satisfying these conditions is found.  */

thread_info *find_thread (ptid_t filter,
			  gdb::function_view<bool (thread_info *)> func);

/* Invoke FUNC for each thread.  */

void for_each_thread (gdb::function_view<void (thread_info *)> func);

/* Like the above, but only consider threads matching PTID.  */

void for_each_thread
  (ptid_t ptid, gdb::function_view<void (thread_info *)> func);

/* Find a random thread that matches PTID and for which FUNC (THREAD)
   returns true.  If no entry is found then return nullptr.  */

thread_info *find_thread_in_random
  (gdb::function_view<bool (thread_info *)> func);

/* Like the above, but only consider threads matching PTID.  */

thread_info *find_thread_in_random
  (ptid_t ptid, gdb::function_view<bool (thread_info *)> func);

/* Switch the current thread.  */

void switch_to_thread (thread_info *thread);

/* Save/restore current thread.  */

class scoped_restore_current_thread
{
public:
  scoped_restore_current_thread ();
  ~scoped_restore_current_thread ();

  DISABLE_COPY_AND_ASSIGN (scoped_restore_current_thread);

  /* Cancel restoring on scope exit.  */
  void dont_restore () { m_dont_restore = true; }

private:
  bool m_dont_restore = false;
  process_info *m_process;
  thread_info *m_thread;
};

#endif /* GDBSERVER_GDBTHREAD_H */
