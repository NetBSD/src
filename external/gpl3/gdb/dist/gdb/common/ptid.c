/* The ptid_t type and common functions operating on it.

   Copyright (C) 1986-2013 Free Software Foundation, Inc.
   
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

#include "ptid.h"

/* Oft used ptids */
ptid_t null_ptid = { 0, 0, 0 };
ptid_t minus_one_ptid = { -1, 0, 0 };

/* Create a ptid given the necessary PID, LWP, and TID components.  */

ptid_t
ptid_build (int pid, long lwp, long tid)
{
  ptid_t ptid;

  ptid.pid = pid;
  ptid.lwp = lwp;
  ptid.tid = tid;
  return ptid;
}

/* Create a ptid from just a pid.  */

ptid_t
pid_to_ptid (int pid)
{
  return ptid_build (pid, 0, 0);
}

/* Fetch the pid (process id) component from a ptid.  */

int
ptid_get_pid (ptid_t ptid)
{
  return ptid.pid;
}

/* Fetch the lwp (lightweight process) component from a ptid.  */

long
ptid_get_lwp (ptid_t ptid)
{
  return ptid.lwp;
}

/* Fetch the tid (thread id) component from a ptid.  */

long
ptid_get_tid (ptid_t ptid)
{
  return ptid.tid;
}

/* ptid_equal() is used to test equality of two ptids.  */

int
ptid_equal (ptid_t ptid1, ptid_t ptid2)
{
  return (ptid1.pid == ptid2.pid
	  && ptid1.lwp == ptid2.lwp
	  && ptid1.tid == ptid2.tid);
}

/* Returns true if PTID represents a process.  */

int
ptid_is_pid (ptid_t ptid)
{
  if (ptid_equal (minus_one_ptid, ptid))
    return 0;
  if (ptid_equal (null_ptid, ptid))
    return 0;

  return (ptid_get_lwp (ptid) == 0 && ptid_get_tid (ptid) == 0);
}
