/* The ptid_t type and common functions operating on it.

   Copyright (C) 1986-2017 Free Software Foundation, Inc.
   
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

ptid_t null_ptid = ptid_t::make_null ();
ptid_t minus_one_ptid = ptid_t::make_minus_one ();

/* See ptid.h.  */

ptid_t
ptid_build (int pid, long lwp, long tid)
{
  return ptid_t (pid, lwp, tid);
}

/* See ptid.h.  */

ptid_t
pid_to_ptid (int pid)
{
  return ptid_t (pid);
}

/* See ptid.h.  */

int
ptid_get_pid (const ptid_t &ptid)
{
  return ptid.pid ();
}

/* See ptid.h.  */

long
ptid_get_lwp (const ptid_t &ptid)
{
  return ptid.lwp ();
}

/* See ptid.h.  */

long
ptid_get_tid (const ptid_t &ptid)
{
  return ptid.tid ();
}

/* See ptid.h.  */

int
ptid_equal (const ptid_t &ptid1, const ptid_t &ptid2)
{
  return ptid1 == ptid2;
}

/* See ptid.h.  */

int
ptid_is_pid (const ptid_t &ptid)
{
  return ptid.is_pid ();
}

/* See ptid.h.  */

int
ptid_lwp_p (const ptid_t &ptid)
{
  return ptid.lwp_p ();
}

/* See ptid.h.  */

int
ptid_tid_p (const ptid_t &ptid)
{
  return ptid.tid_p ();
}

/* See ptid.h.  */

int
ptid_match (const ptid_t &ptid, const ptid_t &filter)
{
  return ptid.matches (filter);
}
