/* Parallel for loops

   Copyright (C) 2019-2020 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_PARALLEL_FOR_H
#define GDBSUPPORT_PARALLEL_FOR_H

#include <algorithm>
#if CXX_STD_THREAD
#include <thread>
#include "gdbsupport/thread-pool.h"
#endif

namespace gdb
{

/* A very simple "parallel for".  This splits the range of iterators
   into subranges, and then passes each subrange to the callback.  The
   work may or may not be done in separate threads.

   This approach was chosen over having the callback work on single
   items because it makes it simple for the caller to do
   once-per-subrange initialization and destruction.  */

template<class RandomIt, class RangeFunction>
void
parallel_for_each (RandomIt first, RandomIt last, RangeFunction callback)
{
#if CXX_STD_THREAD
  /* So we can use a local array below.  */
  const size_t local_max = 16;
  size_t n_threads = std::min (thread_pool::g_thread_pool->thread_count (),
			       local_max);
  size_t n_actual_threads = 0;
  std::future<void> futures[local_max];

  size_t n_elements = last - first;
  if (n_threads > 1)
    {
      /* Arbitrarily require that there should be at least 10 elements
	 in a thread.  */
      if (n_elements / n_threads < 10)
	n_threads = std::max (n_elements / 10, (size_t) 1);
      size_t elts_per_thread = n_elements / n_threads;
      n_actual_threads = n_threads - 1;
      for (int i = 0; i < n_actual_threads; ++i)
	{
	  RandomIt end = first + elts_per_thread;
	  auto task = [=] ()
		      {
			callback (first, end);
		      };

	  futures[i] = gdb::thread_pool::g_thread_pool->post_task (task);
	  first = end;
	}
    }
#endif /* CXX_STD_THREAD */

  /* Process all the remaining elements in the main thread.  */
  callback (first, last);

#if CXX_STD_THREAD
  for (int i = 0; i < n_actual_threads; ++i)
    futures[i].wait ();
#endif /* CXX_STD_THREAD */
}

}

#endif /* GDBSUPPORT_PARALLEL_FOR_H */
