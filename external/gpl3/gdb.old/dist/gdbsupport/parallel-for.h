/* Parallel for loops

   Copyright (C) 2019-2023 Free Software Foundation, Inc.

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
#include <type_traits>
#include "gdbsupport/invoke-result.h"
#include "gdbsupport/thread-pool.h"
#include "gdbsupport/function-view.h"

namespace gdb
{

namespace detail
{

/* This is a helper class that is used to accumulate results for
   parallel_for.  There is a specialization for 'void', below.  */
template<typename T>
struct par_for_accumulator
{
public:

  explicit par_for_accumulator (size_t n_threads)
    : m_futures (n_threads)
  {
  }

  /* The result type that is accumulated.  */
  typedef std::vector<T> result_type;

  /* Post the Ith task to a background thread, and store a future for
     later.  */
  void post (size_t i, std::function<T ()> task)
  {
    m_futures[i]
      = gdb::thread_pool::g_thread_pool->post_task (std::move (task));
  }

  /* Invoke TASK in the current thread, then compute all the results
     from all background tasks and put them into a result vector,
     which is returned.  */
  result_type finish (gdb::function_view<T ()> task)
  {
    result_type result (m_futures.size () + 1);

    result.back () = task ();

    for (size_t i = 0; i < m_futures.size (); ++i)
      result[i] = m_futures[i].get ();

    return result;
  }

  /* Resize the results to N.  */
  void resize (size_t n)
  {
    m_futures.resize (n);
  }

private:
  
  /* A vector of futures coming from the tasks run in the
     background.  */
  std::vector<gdb::future<T>> m_futures;
};

/* See the generic template.  */
template<>
struct par_for_accumulator<void>
{
public:

  explicit par_for_accumulator (size_t n_threads)
    : m_futures (n_threads)
  {
  }

  /* This specialization does not compute results.  */
  typedef void result_type;

  void post (size_t i, std::function<void ()> task)
  {
    m_futures[i]
      = gdb::thread_pool::g_thread_pool->post_task (std::move (task));
  }

  result_type finish (gdb::function_view<void ()> task)
  {
    task ();

    for (auto &future : m_futures)
      {
	/* Use 'get' and not 'wait', to propagate any exception.  */
	future.get ();
      }
  }

  /* Resize the results to N.  */
  void resize (size_t n)
  {
    m_futures.resize (n);
  }

private:

  std::vector<gdb::future<void>> m_futures;
};

}

/* A very simple "parallel for".  This splits the range of iterators
   into subranges, and then passes each subrange to the callback.  The
   work may or may not be done in separate threads.

   This approach was chosen over having the callback work on single
   items because it makes it simple for the caller to do
   once-per-subrange initialization and destruction.

   The parameter N says how batching ought to be done -- there will be
   at least N elements processed per thread.  Setting N to 0 is not
   allowed.

   If the function returns a non-void type, then a vector of the
   results is returned.  The size of the resulting vector depends on
   the number of threads that were used.  */

template<class RandomIt, class RangeFunction>
typename gdb::detail::par_for_accumulator<
    typename gdb::invoke_result<RangeFunction, RandomIt, RandomIt>::type
  >::result_type
parallel_for_each (unsigned n, RandomIt first, RandomIt last,
		   RangeFunction callback,
		   gdb::function_view<size_t(RandomIt)> task_size = nullptr)
{
  using result_type
    = typename gdb::invoke_result<RangeFunction, RandomIt, RandomIt>::type;

  /* If enabled, print debug info about how the work is distributed across
     the threads.  */
  const bool parallel_for_each_debug = false;

  size_t n_worker_threads = thread_pool::g_thread_pool->thread_count ();
  size_t n_threads = n_worker_threads;
  size_t n_elements = last - first;
  size_t elts_per_thread = 0;
  size_t elts_left_over = 0;
  size_t total_size = 0;
  size_t size_per_thread = 0;
  size_t max_element_size = n_elements == 0 ? 1 : SIZE_MAX / n_elements;

  if (n_threads > 1)
    {
      if (task_size != nullptr)
	{
	  gdb_assert (n == 1);
	  for (RandomIt i = first; i != last; ++i)
	    {
	      size_t element_size = task_size (i);
	      gdb_assert (element_size > 0);
	      if (element_size > max_element_size)
		/* We could start scaling here, but that doesn't seem to be
		   worth the effort.  */
		element_size = max_element_size;
	      size_t prev_total_size = total_size;
	      total_size += element_size;
	      /* Check for overflow.  */
	      gdb_assert (prev_total_size < total_size);
	    }
	  size_per_thread = total_size / n_threads;
	}
      else
	{
	  /* Require that there should be at least N elements in a
	     thread.  */
	  gdb_assert (n > 0);
	  if (n_elements / n_threads < n)
	    n_threads = std::max (n_elements / n, (size_t) 1);
	  elts_per_thread = n_elements / n_threads;
	  elts_left_over = n_elements % n_threads;
	  /* n_elements == n_threads * elts_per_thread + elts_left_over. */
	}
    }

  size_t count = n_threads == 0 ? 0 : n_threads - 1;
  gdb::detail::par_for_accumulator<result_type> results (count);

  if (parallel_for_each_debug)
    {
      debug_printf (_("Parallel for: n_elements: %zu\n"), n_elements);
      if (task_size != nullptr)
	{
	  debug_printf (_("Parallel for: total_size: %zu\n"), total_size);
	  debug_printf (_("Parallel for: size_per_thread: %zu\n"), size_per_thread);
	}
      else
	{
	  debug_printf (_("Parallel for: minimum elements per thread: %u\n"), n);
	  debug_printf (_("Parallel for: elts_per_thread: %zu\n"), elts_per_thread);
	}
    }

  size_t remaining_size = total_size;
  for (int i = 0; i < count; ++i)
    {
      RandomIt end;
      size_t chunk_size = 0;
      if (task_size == nullptr)
	{
	  end = first + elts_per_thread;
	  if (i < elts_left_over)
	    /* Distribute the leftovers over the worker threads, to avoid having
	       to handle all of them in a single thread.  */
	    end++;
	}
      else
	{
	  RandomIt j;
	  for (j = first; j < last && chunk_size < size_per_thread; ++j)
	    {
	      size_t element_size = task_size (j);
	      if (element_size > max_element_size)
		element_size = max_element_size;
	      chunk_size += element_size;
	    }
	  end = j;
	  remaining_size -= chunk_size;
	}

      /* This case means we don't have enough elements to really
	 distribute them.  Rather than ever submit a task that does
	 nothing, we short-circuit here.  */
      if (first == end)
	end = last;

      if (end == last)
	{
	  /* We're about to dispatch the last batch of elements, which
	     we normally process in the main thread.  So just truncate
	     the result list here.  This avoids submitting empty tasks
	     to the thread pool.  */
	  count = i;
	  results.resize (count);
	  break;
	}

      if (parallel_for_each_debug)
	{
	  debug_printf (_("Parallel for: elements on worker thread %i\t: %zu"),
			i, (size_t)(end - first));
	  if (task_size != nullptr)
	    debug_printf (_("\t(size: %zu)"), chunk_size);
	  debug_printf (_("\n"));
	}
      results.post (i, [=] ()
        {
	  return callback (first, end);
	});
      first = end;
    }

  for (int i = count; i < n_worker_threads; ++i)
    if (parallel_for_each_debug)
      {
	debug_printf (_("Parallel for: elements on worker thread %i\t: 0"), i);
	if (task_size != nullptr)
	  debug_printf (_("\t(size: 0)"));
	debug_printf (_("\n"));
      }

  /* Process all the remaining elements in the main thread.  */
  if (parallel_for_each_debug)
    {
      debug_printf (_("Parallel for: elements on main thread\t\t: %zu"),
		    (size_t)(last - first));
      if (task_size != nullptr)
	debug_printf (_("\t(size: %zu)"), remaining_size);
      debug_printf (_("\n"));
    }
  return results.finish ([=] ()
    {
      return callback (first, last);
    });
}

/* A sequential drop-in replacement of parallel_for_each.  This can be useful
   when debugging multi-threading behaviour, and you want to limit
   multi-threading in a fine-grained way.  */

template<class RandomIt, class RangeFunction>
typename gdb::detail::par_for_accumulator<
    typename gdb::invoke_result<RangeFunction, RandomIt, RandomIt>::type
  >::result_type
sequential_for_each (unsigned n, RandomIt first, RandomIt last,
		     RangeFunction callback,
		     gdb::function_view<size_t(RandomIt)> task_size = nullptr)
{
  using result_type = typename gdb::invoke_result<RangeFunction, RandomIt, RandomIt>::type;

  gdb::detail::par_for_accumulator<result_type> results (0);

  /* Process all the remaining elements in the main thread.  */
  return results.finish ([=] ()
    {
      return callback (first, last);
    });
}

}

#endif /* GDBSUPPORT_PARALLEL_FOR_H */
