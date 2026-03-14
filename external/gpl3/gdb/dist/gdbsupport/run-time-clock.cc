/* User/system CPU time clocks that follow the std::chrono interface.
   Copyright (C) 2016-2025 Free Software Foundation, Inc.

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

#include "run-time-clock.h"
#if defined HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

using namespace std::chrono;

run_time_clock::time_point
run_time_clock::now () noexcept
{
  return time_point (microseconds (get_run_time ()));
}

#ifdef HAVE_GETRUSAGE
static std::chrono::microseconds
timeval_to_microseconds (struct timeval *tv)
{
  return (seconds (tv->tv_sec) + microseconds (tv->tv_usec));
}
#endif

/* See run-time-clock.h.  */

bool
get_run_time_thread_scope_available ()
{
#if defined HAVE_GETRUSAGE && defined RUSAGE_THREAD
  return true;
#else
  return false;
#endif
}

void
get_run_time (user_cpu_time_clock::time_point &user,
	      system_cpu_time_clock::time_point &system,
	      run_time_scope scope) noexcept
{
#ifdef HAVE_GETRUSAGE

  /* If we can't provide thread scope run time usage, fallback to
     process scope.  This will at least be right if GDB is built
     without threading support in the first place (or is set to use
     zero worker threads).  */
# ifndef RUSAGE_THREAD
#  define RUSAGE_THREAD RUSAGE_SELF
# endif

  struct rusage rusage;
  int who;

  switch (scope)
    {
    case run_time_scope::thread:
      who = RUSAGE_THREAD;
      break;

    case run_time_scope::process:
      who = RUSAGE_SELF;
      break;

    default:
      gdb_assert_not_reached ("invalid run_time_scope value");
    }

  getrusage (who, &rusage);

  microseconds utime = timeval_to_microseconds (&rusage.ru_utime);
  microseconds stime = timeval_to_microseconds (&rusage.ru_stime);
  user = user_cpu_time_clock::time_point (utime);
  system = system_cpu_time_clock::time_point (stime);
#else
  user = user_cpu_time_clock::time_point (microseconds (get_run_time ()));
  system = system_cpu_time_clock::time_point (microseconds::zero ());
#endif
}
