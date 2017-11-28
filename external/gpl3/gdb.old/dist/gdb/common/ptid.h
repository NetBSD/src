/* The ptid_t type and common functions operating on it.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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

#ifndef PTID_H
#define PTID_H

/* The ptid struct is a collection of the various "ids" necessary for
   identifying the inferior process/thread being debugged.  This
   consists of the process id (pid), lightweight process id (lwp) and
   thread id (tid).  When manipulating ptids, the constructors,
   accessors, and predicates declared in this file should be used.  Do
   NOT access the struct ptid members directly.

   process_stratum targets that handle threading themselves should
   prefer using the ptid.lwp field, leaving the ptid.tid field for any
   thread_stratum target that might want to sit on top.
*/

struct ptid
{
  /* Process id.  */
  int pid;

  /* Lightweight process id.  */
  long lwp;

  /* Thread id.  */
  long tid;
};

typedef struct ptid ptid_t;

/* The null or zero ptid, often used to indicate no process. */
extern ptid_t null_ptid;

/* The (-1,0,0) ptid, often used to indicate either an error condition
   or a "don't care" condition, i.e, "run all threads."  */
extern ptid_t minus_one_ptid;

/* Make a ptid given the necessary PID, LWP, and TID components.  */
ptid_t ptid_build (int pid, long lwp, long tid);

/* Make a new ptid from just a pid.  This ptid is usually used to
   represent a whole process, including all its lwps/threads.  */
ptid_t pid_to_ptid (int pid);

/* Fetch the pid (process id) component from a ptid.  */
int ptid_get_pid (ptid_t ptid);

/* Fetch the lwp (lightweight process) component from a ptid.  */
long ptid_get_lwp (ptid_t ptid);

/* Fetch the tid (thread id) component from a ptid.  */
long ptid_get_tid (ptid_t ptid);

/* Compare two ptids to see if they are equal.  */
int ptid_equal (ptid_t ptid1, ptid_t ptid2);

/* Returns true if PTID represents a whole process, including all its
   lwps/threads.  Such ptids have the form of (pid,0,0), with pid !=
   -1.  */
int ptid_is_pid (ptid_t ptid);

/* Return true if PTID's lwp member is non-zero.  */
int ptid_lwp_p (ptid_t ptid);

/* Return true if PTID's tid member is non-zero.  */
int ptid_tid_p (ptid_t ptid);

/* Returns true if PTID matches filter FILTER.  FILTER can be the wild
   card MINUS_ONE_PTID (all ptid match it); can be a ptid representing
   a process (ptid_is_pid returns true), in which case, all lwps and
   threads of that given process match, lwps and threads of other
   processes do not; or, it can represent a specific thread, in which
   case, only that thread will match true.  PTID must represent a
   specific LWP or THREAD, it can never be a wild card.  */

extern int ptid_match (ptid_t ptid, ptid_t filter);

#endif
