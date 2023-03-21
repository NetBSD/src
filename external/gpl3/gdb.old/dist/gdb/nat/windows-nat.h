/* Internal interfaces for the Windows code
   Copyright (C) 1995-2020 Free Software Foundation, Inc.

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

#ifndef NAT_WINDOWS_NAT_H
#define NAT_WINDOWS_NAT_H

#include <windows.h>
#include <vector>

#include "gdbsupport/gdb_optional.h"
#include "target/waitstatus.h"

#define STATUS_WX86_BREAKPOINT 0x4000001F
#define STATUS_WX86_SINGLE_STEP 0x4000001E

namespace windows_nat
{

/* Thread information structure used to track extra information about
   each thread.  */
struct windows_thread_info
{
  windows_thread_info (DWORD tid_, HANDLE h_, CORE_ADDR tlb)
    : tid (tid_),
      h (h_),
      thread_local_base (tlb)
  {
  }

  ~windows_thread_info ();

  DISABLE_COPY_AND_ASSIGN (windows_thread_info);

  /* Ensure that this thread has been suspended.  */
  void suspend ();

  /* Resume the thread if it has been suspended.  */
  void resume ();

  /* The Win32 thread identifier.  */
  DWORD tid;

  /* The handle to the thread.  */
  HANDLE h;

  /* Thread Information Block address.  */
  CORE_ADDR thread_local_base;

  /* This keeps track of whether SuspendThread was called on this
     thread.  -1 means there was a failure or that the thread was
     explicitly not suspended, 1 means it was called, and 0 means it
     was not.  */
  int suspended = 0;

#ifdef _WIN32_WCE
  /* The context as retrieved right after suspending the thread. */
  CONTEXT base_context {};
#endif

  /* The context of the thread, including any manipulations.  */
  union
  {
    CONTEXT context {};
#ifdef __x86_64__
    WOW64_CONTEXT wow64_context;
#endif
  };

  /* Whether debug registers changed since we last set CONTEXT back to
     the thread.  */
  bool debug_registers_changed = false;

  /* Nonzero if CONTEXT is invalidated and must be re-read from the
     inferior thread.  */
  bool reload_context = false;

  /* True if this thread is currently stopped at a software
     breakpoint.  This is used to offset the PC when needed.  */
  bool stopped_at_software_breakpoint = false;

  /* True if we've adjusted the PC after hitting a software
     breakpoint, false otherwise.  This lets us avoid multiple
     adjustments if the registers are read multiple times.  */
  bool pc_adjusted = false;

  /* The name of the thread, allocated by xmalloc.  */
  gdb::unique_xmalloc_ptr<char> name;
};


/* Possible values to pass to 'thread_rec'.  */
enum thread_disposition_type
{
  /* Do not invalidate the thread's context, and do not suspend the
     thread.  */
  DONT_INVALIDATE_CONTEXT,
  /* Invalidate the context, but do not suspend the thread.  */
  DONT_SUSPEND,
  /* Invalidate the context and suspend the thread.  */
  INVALIDATE_CONTEXT
};

/* Find a thread record given a thread id.  THREAD_DISPOSITION
   controls whether the thread is suspended, and whether the context
   is invalidated.

   This function must be supplied by the embedding application.  */
extern windows_thread_info *thread_rec (ptid_t ptid,
					thread_disposition_type disposition);


/* Handle OUTPUT_DEBUG_STRING_EVENT from child process.  Updates
   OURSTATUS and returns the thread id if this represents a thread
   change (this is specific to Cygwin), otherwise 0.

   Cygwin prepends its messages with a "cygwin:".  Interpret this as
   a Cygwin signal.  Otherwise just print the string as a warning.

   This function must be supplied by the embedding application.  */
extern int handle_output_debug_string (struct target_waitstatus *ourstatus);

/* Handle a DLL load event.

   This function assumes that the current event did not occur during
   inferior initialization.

   This function must be supplied by the embedding application.  */

extern void handle_load_dll ();

/* Handle a DLL unload event.

   This function assumes that this event did not occur during inferior
   initialization.

   This function must be supplied by the embedding application.  */

extern void handle_unload_dll ();

/* Handle MS_VC_EXCEPTION when processing a stop.  MS_VC_EXCEPTION is
   somewhat undocumented but is used to tell the debugger the name of
   a thread.

   Return true if the exception was handled; return false otherwise.

   This function must be supplied by the embedding application.  */

extern bool handle_ms_vc_exception (const EXCEPTION_RECORD *rec);

/* When EXCEPTION_ACCESS_VIOLATION is processed, we give the embedding
   application a chance to change it to be considered "unhandled".
   This function must be supplied by the embedding application.  If it
   returns true, then the exception is "unhandled".  */

extern bool handle_access_violation (const EXCEPTION_RECORD *rec);


/* Currently executing process */
extern HANDLE current_process_handle;
extern DWORD current_process_id;
extern DWORD main_thread_id;
extern enum gdb_signal last_sig;

/* The current debug event from WaitForDebugEvent or from a pending
   stop.  */
extern DEBUG_EVENT current_event;

/* The ID of the thread for which we anticipate a stop event.
   Normally this is -1, meaning we'll accept an event in any
   thread.  */
extern DWORD desired_stop_thread_id;

/* A single pending stop.  See "pending_stops" for more
   information.  */
struct pending_stop
{
  /* The thread id.  */
  DWORD thread_id;

  /* The target waitstatus we computed.  */
  target_waitstatus status;

  /* The event.  A few fields of this can be referenced after a stop,
     and it seemed simplest to store the entire event.  */
  DEBUG_EVENT event;
};

/* A vector of pending stops.  Sometimes, Windows will report a stop
   on a thread that has been ostensibly suspended.  We believe what
   happens here is that two threads hit a breakpoint simultaneously,
   and the Windows kernel queues the stop events.  However, this can
   result in the strange effect of trying to single step thread A --
   leaving all other threads suspended -- and then seeing a stop in
   thread B.  To handle this scenario, we queue all such "pending"
   stops here, and then process them once the step has completed.  See
   PR gdb/22992.  */
extern std::vector<pending_stop> pending_stops;

/* Contents of $_siginfo */
extern EXCEPTION_RECORD siginfo_er;

#ifdef __x86_64__
/* Ignore first breakpoint exception of WOW64 process */
extern bool ignore_first_breakpoint;
#endif

/* Return the name of the DLL referenced by H at ADDRESS.  UNICODE
   determines what sort of string is read from the inferior.  Returns
   the name of the DLL, or NULL on error.  If a name is returned, it
   is stored in a static buffer which is valid until the next call to
   get_image_name.  */
extern const char *get_image_name (HANDLE h, void *address, int unicode);

typedef enum
{
  HANDLE_EXCEPTION_UNHANDLED = 0,
  HANDLE_EXCEPTION_HANDLED,
  HANDLE_EXCEPTION_IGNORED
} handle_exception_result;

extern handle_exception_result handle_exception
  (struct target_waitstatus *ourstatus, bool debug_exceptions);

/* Return true if there is a pending stop matching
   desired_stop_thread_id.  If DEBUG_EVENTS is true, logging will be
   enabled.  */

extern bool matching_pending_stop (bool debug_events);

/* See if a pending stop matches DESIRED_STOP_THREAD_ID.  If so,
   remove it from the list of pending stops, set 'current_event', and
   return it.  Otherwise, return an empty optional.  */

extern gdb::optional<pending_stop> fetch_pending_stop (bool debug_events);

/* A simple wrapper for ContinueDebugEvent that continues the last
   waited-for event.  If DEBUG_EVENTS is true, logging will be
   enabled.  */

extern BOOL continue_last_debug_event (DWORD continue_status,
				       bool debug_events);

/* A simple wrapper for WaitForDebugEvent that also sets the internal
   'last_wait_event' on success.  */

extern BOOL wait_for_debug_event (DEBUG_EVENT *event, DWORD timeout);

}

#endif
