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

#include "common-defs.h"
#include "ptid.h"

/* See ptid.h for these.  */

ptid_t null_ptid = { 0, 0, 0 };
ptid_t minus_one_ptid = { -1, 0, 0 };

/* See ptid.h.  */

ptid_t
ptid_build (int pid, long lwp, long tid)
{
  ptid_t ptid;

  ptid.pid = pid;
  ptid.lwp = lwp;
  ptid.tid = tid;
  return ptid;
}

/* See ptid.h.  */

ptid_t
pid_to_ptid (int pid)
{
  return ptid_build (pid, 0, 0);
}

/* See ptid.h.  */

int
ptid_get_pid (ptid_t ptid)
{
  return ptid.pid;
}

/* See ptid.h.  */

long
ptid_get_lwp (ptid_t ptid)
{
  return ptid.lwp;
}

/* See ptid.h.  */

long
ptid_get_tid (ptid_t ptid)
{
  return ptid.tid;
}

/* See ptid.h.  */

int
ptid_equal (ptid_t ptid1, ptid_t ptid2)
{
  return (ptid1.pid == ptid2.pid
	  && ptid1.lwp == ptid2.lwp
	  && ptid1.tid == ptid2.tid);
}

/* See ptid.h.  */

int
ptid_is_pid (ptid_t ptid)
{
  if (ptid_equal (minus_one_ptid, ptid)
      || ptid_equal (null_ptid, ptid))
    return 0;

  return (ptid_get_lwp (ptid) == 0 && ptid_get_tid (ptid) == 0);
}

/* See ptid.h.  */

int
ptid_lwp_p (ptid_t ptid)
{
  if (ptid_equal (minus_one_ptid, ptid)
      || ptid_equal (null_ptid, ptid))
    return 0;

  return (ptid_get_lwp (ptid) != 0);
}

/* See ptid.h.  */

int
ptid_tid_p (ptid_t ptid)
{
  if (ptid_equal (minus_one_ptid, ptid)
      || ptid_equal (null_ptid, ptid))
    return 0;

  return (ptid_get_tid (ptid) != 0);
}

int
ptid_match (ptid_t ptid, ptid_t filter)
{
  if (ptid_equal (filter, minus_one_ptid))
    return 1;
  if (ptid_is_pid (filter)
      && ptid_get_pid (ptid) == ptid_get_pid (filter))
    return 1;
  else if (ptid_equal (ptid, filter))
    return 1;

  return 0;
}
