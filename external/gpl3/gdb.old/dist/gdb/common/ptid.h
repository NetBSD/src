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

class ptid_t
{
public:
  /* Must have a trivial defaulted default constructor so that the
     type remains POD.  */
  ptid_t () noexcept = default;

  /* Make a ptid given the necessary PID, LWP, and TID components.

     A ptid with only a PID (LWP and TID equal to zero) is usually used to
     represent a whole process, including all its lwps/threads.  */

  explicit constexpr ptid_t (int pid, long lwp = 0, long tid = 0)
    : m_pid (pid), m_lwp (lwp), m_tid (tid)
  {}

  /* Fetch the pid (process id) component from the ptid.  */

  constexpr int pid () const
  { return m_pid; }

  /* Return true if the ptid's lwp member is non-zero.  */

  constexpr bool lwp_p () const
  { return m_lwp != 0; }

  /* Fetch the lwp (lightweight process) component from the ptid.  */

  constexpr long lwp () const
  { return m_lwp; }

  /* Return true if the ptid's tid member is non-zero.  */

  constexpr bool tid_p () const
  { return m_tid != 0; }

  /* Fetch the tid (thread id) component from a ptid.  */

  constexpr long tid () const
  { return m_tid; }

  /* Return true if the ptid represents a whole process, including all its
     lwps/threads.  Such ptids have the form of (pid, 0, 0), with
     pid != -1.  */

  constexpr bool is_pid () const
  {
    return (*this != make_null ()
	    && *this != make_minus_one ()
	    && m_lwp == 0
	    && m_tid == 0);
  }

  /* Compare two ptids to see if they are equal.  */

  constexpr bool operator== (const ptid_t &other) const
  {
    return (m_pid == other.m_pid
	    && m_lwp == other.m_lwp
	    && m_tid == other.m_tid);
  }

  /* Compare two ptids to see if they are different.  */

  constexpr bool operator!= (const ptid_t &other) const
  {
    return !(*this == other);
  }

  /* Return true if the ptid matches FILTER.  FILTER can be the wild
     card MINUS_ONE_PTID (all ptids match it); can be a ptid representing
     a process (ptid.is_pid () returns true), in which case, all lwps and
     threads of that given process match, lwps and threads of other
     processes do not; or, it can represent a specific thread, in which
     case, only that thread will match true.  The ptid must represent a
     specific LWP or THREAD, it can never be a wild card.  */

  constexpr bool matches (const ptid_t &filter) const
  {
    return (/* If filter represents any ptid, it's always a match.  */
	    filter == make_minus_one ()
	    /* If filter is only a pid, any ptid with that pid
	       matches.  */
	    || (filter.is_pid () && m_pid == filter.pid ())

	    /* Otherwise, this ptid only matches if it's exactly equal
	       to filter.  */
	    || *this == filter);
  }

  /* Make a null ptid.  */

  static constexpr ptid_t make_null ()
  { return ptid_t (0, 0, 0); }

  /* Make a minus one ptid.  */

  static constexpr ptid_t make_minus_one ()
  { return ptid_t (-1, 0, 0); }

private:
  /* Process id.  */
  int m_pid;

  /* Lightweight process id.  */
  long m_lwp;

  /* Thread id.  */
  long m_tid;
};

/* The null or zero ptid, often used to indicate no process. */

extern ptid_t null_ptid;

/* The (-1,0,0) ptid, often used to indicate either an error condition
   or a "don't care" condition, i.e, "run all threads."  */

extern ptid_t minus_one_ptid;


/* The following functions are kept for backwards compatibility.  The use of
   the ptid_t methods is preferred.  */

/* See ptid_t::ptid_t.  */

extern ptid_t ptid_build (int pid, long lwp, long tid);

/* See ptid_t::ptid_t.  */

extern ptid_t pid_to_ptid (int pid);

/* See ptid_t::pid.  */

extern int ptid_get_pid (const ptid_t &ptid);

/* See ptid_t::lwp.  */

extern long ptid_get_lwp (const ptid_t &ptid);

/* See ptid_t::tid.  */

extern long ptid_get_tid (const ptid_t &ptid);

/* See ptid_t::operator== and ptid_t::operator!=.  */

extern int ptid_equal (const ptid_t &ptid1, const ptid_t &ptid2);

/* See ptid_t::is_pid.  */

extern int ptid_is_pid (const ptid_t &ptid);

/* See ptid_t::lwp_p.  */

extern int ptid_lwp_p (const ptid_t &ptid);

/* See ptid_t::tid_p.  */

extern int ptid_tid_p (const ptid_t &ptid);

/* See ptid_t::matches.  */

extern int ptid_match (const ptid_t &ptid, const ptid_t &filter);

#endif
