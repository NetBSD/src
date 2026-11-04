/* Self tests for parallel_for_each

   Copyright (C) 2021-2025 Free Software Foundation, Inc.

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

#include "gdbsupport/selftest.h"
#include "gdbsupport/parallel-for.h"

#if CXX_STD_THREAD

#include "gdbsupport/thread-pool.h"

namespace selftests {
namespace parallel_for {

struct save_restore_n_threads
{
  save_restore_n_threads ()
    : n_threads (gdb::thread_pool::g_thread_pool->thread_count ())
  {
  }

  ~save_restore_n_threads ()
  {
    gdb::thread_pool::g_thread_pool->set_thread_count (n_threads);
  }

  int n_threads;
};

using foreach_callback_t = gdb::function_view<void (int first, int last)>;
using do_foreach_t = gdb::function_view<void (int first, int last,
					     foreach_callback_t)>;

static void
test_one (int n_threads, do_foreach_t do_foreach)
{
  save_restore_n_threads saver;
  gdb::thread_pool::g_thread_pool->set_thread_count (n_threads);

  {
    constexpr int upper_bound = 1000;
    std::atomic<int> counter (0);
    do_foreach (0, upper_bound,
		[&] (int start, int end) { counter += end - start; });
    SELF_CHECK (counter == upper_bound);
  }

  {
    std::atomic<int> counter (0);
    do_foreach (0, 0, [&] (int start, int end) { counter += end - start; });
    SELF_CHECK (counter == 0);
  }

  {
    /* Check that if there are fewer tasks than threads, then we won't
       end up with a null result.  */
    std::vector<std::unique_ptr<int>> intresults;
    std::atomic<bool> any_empty_tasks (false);

    do_foreach (0, 1,
		[&] (int start, int end)
		  {
		    if (start == end)
		      any_empty_tasks = true;

		    return std::make_unique<int> (end - start);
		  });

    SELF_CHECK (!any_empty_tasks);
    SELF_CHECK (std::all_of (intresults.begin (), intresults.end (),
			     [] (const std::unique_ptr<int> &entry)
			       { return entry != nullptr; }));
  }
}

static void
test_parallel_for_each ()
{
  const std::vector<do_foreach_t> for_each_functions
    {
      [] (int start, int end, foreach_callback_t callback)
      { gdb::parallel_for_each<1> (start, end, callback); },
      [] (int start, int end, foreach_callback_t callback)
      { gdb::sequential_for_each (start, end, callback);}
    };

  for (int n_threads : { 0, 1, 3 })
    for (const auto &for_each_function : for_each_functions)
      test_one (n_threads, for_each_function);
}

} /* namespace parallel_for */
} /* namespace selftests */

#endif /* CXX_STD_THREAD */

INIT_GDB_FILE (parallel_for_selftests)
{
#ifdef CXX_STD_THREAD
  selftests::register_test ("parallel_for",
			    selftests::parallel_for::test_parallel_for_each);
#endif /* CXX_STD_THREAD */
}
