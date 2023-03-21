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

#include "gdbsupport/common-defs.h"
#include "nat/windows-nat.h"
#include "gdbsupport/common-debug.h"

namespace windows_nat
{

HANDLE current_process_handle;
DWORD current_process_id;
DWORD main_thread_id;
enum gdb_signal last_sig = GDB_SIGNAL_0;
DEBUG_EVENT current_event;

/* The most recent event from WaitForDebugEvent.  Unlike
   current_event, this is guaranteed never to come from a pending
   stop.  This is important because only data from the most recent
   event from WaitForDebugEvent can be used when calling
   ContinueDebugEvent.  */
static DEBUG_EVENT last_wait_event;

DWORD desired_stop_thread_id = -1;
std::vector<pending_stop> pending_stops;
EXCEPTION_RECORD siginfo_er;

#ifdef __x86_64__
bool ignore_first_breakpoint = false;
#endif

/* Note that 'debug_events' must be locally defined in the relevant
   functions.  */
#define DEBUG_EVENTS(x)	if (debug_events) debug_printf x

windows_thread_info::~windows_thread_info ()
{
}

void
windows_thread_info::suspend ()
{
  if (suspended != 0)
    return;

  if (SuspendThread (h) == (DWORD) -1)
    {
      DWORD err = GetLastError ();

      /* We get Access Denied (5) when trying to suspend
	 threads that Windows started on behalf of the
	 debuggee, usually when those threads are just
	 about to exit.
	 We can get Invalid Handle (6) if the main thread
	 has exited.  */
      if (err != ERROR_INVALID_HANDLE && err != ERROR_ACCESS_DENIED)
	warning (_("SuspendThread (tid=0x%x) failed. (winerr %u)"),
		 (unsigned) tid, (unsigned) err);
      suspended = -1;
    }
  else
    suspended = 1;
}

void
windows_thread_info::resume ()
{
  if (suspended > 0)
    {
      stopped_at_software_breakpoint = false;

      if (ResumeThread (h) == (DWORD) -1)
	{
	  DWORD err = GetLastError ();
	  warning (_("warning: ResumeThread (tid=0x%x) failed. (winerr %u)"),
		   (unsigned) tid, (unsigned) err);
	}
    }
  suspended = 0;
}

const char *
get_image_name (HANDLE h, void *address, int unicode)
{
#ifdef __CYGWIN__
  static char buf[MAX_PATH];
#else
  static char buf[(2 * MAX_PATH) + 1];
#endif
  DWORD size = unicode ? sizeof (WCHAR) : sizeof (char);
  char *address_ptr;
  int len = 0;
  char b[2];
  SIZE_T done;

  /* Attempt to read the name of the dll that was detected.
     This is documented to work only when actively debugging
     a program.  It will not work for attached processes.  */
  if (address == NULL)
    return NULL;

#ifdef _WIN32_WCE
  /* Windows CE reports the address of the image name,
     instead of an address of a pointer into the image name.  */
  address_ptr = address;
#else
  /* See if we could read the address of a string, and that the
     address isn't null.  */
  if (!ReadProcessMemory (h, address,  &address_ptr,
			  sizeof (address_ptr), &done)
      || done != sizeof (address_ptr)
      || !address_ptr)
    return NULL;
#endif

  /* Find the length of the string.  */
  while (ReadProcessMemory (h, address_ptr + len++ * size, &b, size, &done)
	 && (b[0] != 0 || b[size - 1] != 0) && done == size)
    continue;

  if (!unicode)
    ReadProcessMemory (h, address_ptr, buf, len, &done);
  else
    {
      WCHAR *unicode_address = (WCHAR *) alloca (len * sizeof (WCHAR));
      ReadProcessMemory (h, address_ptr, unicode_address, len * sizeof (WCHAR),
			 &done);
#ifdef __CYGWIN__
      wcstombs (buf, unicode_address, MAX_PATH);
#else
      WideCharToMultiByte (CP_ACP, 0, unicode_address, len, buf, sizeof buf,
			   0, 0);
#endif
    }

  return buf;
}

/* The exception thrown by a program to tell the debugger the name of
   a thread.  The exception record contains an ID of a thread and a
   name to give it.  This exception has no documented name, but MSDN
   dubs it "MS_VC_EXCEPTION" in one code example.  */
#define MS_VC_EXCEPTION 0x406d1388

handle_exception_result
handle_exception (struct target_waitstatus *ourstatus, bool debug_exceptions)
{
#define DEBUG_EXCEPTION_SIMPLE(x)       if (debug_exceptions) \
  debug_printf ("gdb: Target exception %s at %s\n", x, \
    host_address_to_string (\
      current_event.u.Exception.ExceptionRecord.ExceptionAddress))

  EXCEPTION_RECORD *rec = &current_event.u.Exception.ExceptionRecord;
  DWORD code = rec->ExceptionCode;
  handle_exception_result result = HANDLE_EXCEPTION_HANDLED;

  memcpy (&siginfo_er, rec, sizeof siginfo_er);

  ourstatus->kind = TARGET_WAITKIND_STOPPED;

  /* Record the context of the current thread.  */
  thread_rec (ptid_t (current_event.dwProcessId, current_event.dwThreadId, 0),
	      DONT_SUSPEND);

  switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_ACCESS_VIOLATION");
      ourstatus->value.sig = GDB_SIGNAL_SEGV;
      if (handle_access_violation (rec))
	return HANDLE_EXCEPTION_UNHANDLED;
      break;
    case STATUS_STACK_OVERFLOW:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_STACK_OVERFLOW");
      ourstatus->value.sig = GDB_SIGNAL_SEGV;
      break;
    case STATUS_FLOAT_DENORMAL_OPERAND:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_DENORMAL_OPERAND");
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_ARRAY_BOUNDS_EXCEEDED");
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_FLOAT_INEXACT_RESULT:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_INEXACT_RESULT");
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_FLOAT_INVALID_OPERATION:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_INVALID_OPERATION");
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_FLOAT_OVERFLOW:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_OVERFLOW");
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_FLOAT_STACK_CHECK:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_STACK_CHECK");
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_FLOAT_UNDERFLOW:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_UNDERFLOW");
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_FLOAT_DIVIDE_BY_ZERO:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_FLOAT_DIVIDE_BY_ZERO");
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_INTEGER_DIVIDE_BY_ZERO:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_INTEGER_DIVIDE_BY_ZERO");
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_INTEGER_OVERFLOW:
      DEBUG_EXCEPTION_SIMPLE ("STATUS_INTEGER_OVERFLOW");
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case EXCEPTION_BREAKPOINT:
#ifdef __x86_64__
      if (ignore_first_breakpoint)
	{
	  /* For WOW64 processes, there are always 2 breakpoint exceptions
	     on startup, first a BREAKPOINT for the 64bit ntdll.dll,
	     then a WX86_BREAKPOINT for the 32bit ntdll.dll.
	     Here we only care about the WX86_BREAKPOINT's.  */
	  ourstatus->kind = TARGET_WAITKIND_SPURIOUS;
	  ignore_first_breakpoint = false;
	}
#endif
      /* FALLTHROUGH */
    case STATUS_WX86_BREAKPOINT:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_BREAKPOINT");
      ourstatus->value.sig = GDB_SIGNAL_TRAP;
#ifdef _WIN32_WCE
      /* Remove the initial breakpoint.  */
      check_breakpoints ((CORE_ADDR) (long) current_event
			 .u.Exception.ExceptionRecord.ExceptionAddress);
#endif
      break;
    case DBG_CONTROL_C:
      DEBUG_EXCEPTION_SIMPLE ("DBG_CONTROL_C");
      ourstatus->value.sig = GDB_SIGNAL_INT;
      break;
    case DBG_CONTROL_BREAK:
      DEBUG_EXCEPTION_SIMPLE ("DBG_CONTROL_BREAK");
      ourstatus->value.sig = GDB_SIGNAL_INT;
      break;
    case EXCEPTION_SINGLE_STEP:
    case STATUS_WX86_SINGLE_STEP:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_SINGLE_STEP");
      ourstatus->value.sig = GDB_SIGNAL_TRAP;
      break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_ILLEGAL_INSTRUCTION");
      ourstatus->value.sig = GDB_SIGNAL_ILL;
      break;
    case EXCEPTION_PRIV_INSTRUCTION:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_PRIV_INSTRUCTION");
      ourstatus->value.sig = GDB_SIGNAL_ILL;
      break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_NONCONTINUABLE_EXCEPTION");
      ourstatus->value.sig = GDB_SIGNAL_ILL;
      break;
    case MS_VC_EXCEPTION:
      DEBUG_EXCEPTION_SIMPLE ("MS_VC_EXCEPTION");
      if (handle_ms_vc_exception (rec))
	{
	  ourstatus->value.sig = GDB_SIGNAL_TRAP;
	  result = HANDLE_EXCEPTION_IGNORED;
	  break;
	}
	/* treat improperly formed exception as unknown */
	/* FALLTHROUGH */
    default:
      /* Treat unhandled first chance exceptions specially.  */
      if (current_event.u.Exception.dwFirstChance)
	return HANDLE_EXCEPTION_UNHANDLED;
      debug_printf ("gdb: unknown target exception 0x%08x at %s\n",
	(unsigned) current_event.u.Exception.ExceptionRecord.ExceptionCode,
	host_address_to_string (
	  current_event.u.Exception.ExceptionRecord.ExceptionAddress));
      ourstatus->value.sig = GDB_SIGNAL_UNKNOWN;
      break;
    }

  last_sig = ourstatus->value.sig;
  return result;

#undef DEBUG_EXCEPTION_SIMPLE
}

/* See nat/windows-nat.h.  */

bool
matching_pending_stop (bool debug_events)
{
  /* If there are pending stops, and we might plausibly hit one of
     them, we don't want to actually continue the inferior -- we just
     want to report the stop.  In this case, we just pretend to
     continue.  See the comment by the definition of "pending_stops"
     for details on why this is needed.  */
  for (const auto &item : pending_stops)
    {
      if (desired_stop_thread_id == -1
	  || desired_stop_thread_id == item.thread_id)
	{
	  DEBUG_EVENTS (("windows_continue - pending stop anticipated, "
			 "desired=0x%x, item=0x%x\n",
			 desired_stop_thread_id, item.thread_id));
	  return true;
	}
    }

  return false;
}

/* See nat/windows-nat.h.  */

gdb::optional<pending_stop>
fetch_pending_stop (bool debug_events)
{
  gdb::optional<pending_stop> result;
  for (auto iter = pending_stops.begin ();
       iter != pending_stops.end ();
       ++iter)
    {
      if (desired_stop_thread_id == -1
	  || desired_stop_thread_id == iter->thread_id)
	{
	  result = *iter;
	  current_event = iter->event;

	  DEBUG_EVENTS (("get_windows_debug_event - "
			 "pending stop found in 0x%x (desired=0x%x)\n",
			 iter->thread_id, desired_stop_thread_id));

	  pending_stops.erase (iter);
	  break;
	}
    }

  return result;
}

/* See nat/windows-nat.h.  */

BOOL
continue_last_debug_event (DWORD continue_status, bool debug_events)
{
  DEBUG_EVENTS (("ContinueDebugEvent (cpid=%d, ctid=0x%x, %s);\n",
		  (unsigned) last_wait_event.dwProcessId,
		  (unsigned) last_wait_event.dwThreadId,
		  continue_status == DBG_CONTINUE ?
		  "DBG_CONTINUE" : "DBG_EXCEPTION_NOT_HANDLED"));

  return ContinueDebugEvent (last_wait_event.dwProcessId,
			     last_wait_event.dwThreadId,
			     continue_status);
}

/* See nat/windows-nat.h.  */

BOOL
wait_for_debug_event (DEBUG_EVENT *event, DWORD timeout)
{
  BOOL result = WaitForDebugEvent (event, timeout);
  if (result)
    last_wait_event = *event;
  return result;
}

}
