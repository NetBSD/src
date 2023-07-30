/* Run a function on the main thread
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

#include "defs.h"
#include "run-on-main-thread.h"
#include "ser-event.h"
#if CXX_STD_THREAD
#include <thread>
#include <mutex>
#endif
#include "gdbsupport/event-loop.h"

/* The serial event used when posting runnables.  */

static struct serial_event *runnable_event;

/* Runnables that have been posted.  */

static std::vector<std::function<void ()>> runnables;

#if CXX_STD_THREAD

/* Mutex to hold when handling RUNNABLE_EVENT or RUNNABLES.  */

static std::mutex runnable_mutex;

/* The main thread.  */

static std::thread::id main_thread;

#endif

/* Run all the queued runnables.  */

static void
run_events (int error, gdb_client_data client_data)
{
  std::vector<std::function<void ()>> local;

  /* Hold the lock while changing the globals, but not while running
     the runnables.  */
  {
#if CXX_STD_THREAD
    std::lock_guard<std::mutex> lock (runnable_mutex);
#endif

    /* Clear the event fd.  Do this before flushing the events list,
       so that any new event post afterwards is sure to re-awaken the
       event loop.  */
    serial_event_clear (runnable_event);

    /* Move the vector in case running a runnable pushes a new
       runnable.  */
    local = std::move (runnables);
  }

  for (auto &item : local)
    {
      try
	{
	  item ();
	}
      catch (...)
	{
	  /* Ignore exceptions in the callback.  */
	}
    }
}

/* See run-on-main-thread.h.  */

void
run_on_main_thread (std::function<void ()> &&func)
{
#if CXX_STD_THREAD
  std::lock_guard<std::mutex> lock (runnable_mutex);
#endif
  runnables.emplace_back (std::move (func));
  serial_event_set (runnable_event);
}

/* See run-on-main-thread.h.  */

bool
is_main_thread ()
{
#if CXX_STD_THREAD
  return std::this_thread::get_id () == main_thread;
#else
  return true;
#endif
}

void _initialize_run_on_main_thread ();
void
_initialize_run_on_main_thread ()
{
#if CXX_STD_THREAD
  main_thread = std::this_thread::get_id ();
#endif
  runnable_event = make_serial_event ();
  add_file_handler (serial_event_fd (runnable_event), run_events, nullptr,
		    "run-on-main-thread");
}
