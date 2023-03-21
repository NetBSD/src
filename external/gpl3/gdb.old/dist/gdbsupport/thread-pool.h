/* Thread pool

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

#ifndef GDBSUPPORT_THREAD_POOL_H
#define GDBSUPPORT_THREAD_POOL_H

#include <queue>
#include <thread>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include "gdbsupport/gdb_optional.h"

namespace gdb
{

/* A thread pool.

   There is a single global thread pool, see g_thread_pool.  Tasks can
   be submitted to the thread pool.  They will be processed in worker
   threads as time allows.  */
class thread_pool
{
public:
  /* The sole global thread pool.  */
  static thread_pool *g_thread_pool;

  ~thread_pool ();
  DISABLE_COPY_AND_ASSIGN (thread_pool);

  /* Set the thread count of this thread pool.  By default, no threads
     are created -- the thread count must be set first.  */
  void set_thread_count (size_t num_threads);

  /* Return the number of executing threads.  */
  size_t thread_count () const
  {
    return m_thread_count;
  }

  /* Post a task to the thread pool.  A future is returned, which can
     be used to wait for the result.  */
  std::future<void> post_task (std::function<void ()> func);

private:

  thread_pool () = default;

  /* The callback for each worker thread.  */
  void thread_function ();

  /* The current thread count.  */
  size_t m_thread_count = 0;

  /* A convenience typedef for the type of a task.  */
  typedef std::packaged_task<void ()> task;

  /* The tasks that have not been processed yet.  An optional is used
     to represent a task.  If the optional is empty, then this means
     that the receiving thread should terminate.  If the optional is
     non-empty, then it is an actual task to evaluate.  */
  std::queue<optional<task>> m_tasks;

  /* A condition variable and mutex that are used for communication
     between the main thread and the worker threads.  */
  std::condition_variable m_tasks_cv;
  std::mutex m_tasks_mutex;
};

}

#endif /* GDBSUPPORT_THREAD_POOL_H */
