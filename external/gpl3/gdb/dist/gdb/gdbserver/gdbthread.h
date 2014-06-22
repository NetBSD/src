/* Multi-thread control defs for remote server for GDB.
   Copyright (C) 1993-2014 Free Software Foundation, Inc.

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

#ifndef GDB_THREAD_H
#define GDB_THREAD_H

#include "server.h"
#include "inferiors.h"

struct btrace_target_info;

struct thread_info
{
  struct inferior_list_entry entry;
  void *target_data;
  void *regcache_data;

  /* The last resume GDB requested on this thread.  */
  enum resume_kind last_resume_kind;

  /* The last wait status reported for this thread.  */
  struct target_waitstatus last_status;

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
  struct wstep_state *while_stepping;

  /* Branch trace target information for this thread.  */
  struct btrace_target_info *btrace;
};

extern struct inferior_list all_threads;

void remove_thread (struct thread_info *thread);
void add_thread (ptid_t ptid, void *target_data);

struct thread_info *find_thread_ptid (ptid_t ptid);
struct thread_info *gdb_id_to_thread (unsigned int);

/* Get current thread ID (Linux task ID).  */
#define current_ptid ((struct inferior_list_entry *) current_inferior)->id
#endif /* GDB_THREAD_H */
