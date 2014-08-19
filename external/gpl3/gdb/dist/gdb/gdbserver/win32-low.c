/* Low level interface to Windows debugging, for gdbserver.
   Copyright (C) 2006-2014 Free Software Foundation, Inc.

   Contributed by Leo Zayas.  Based on "win32-nat.c" from GDB.

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

#include "server.h"
#include "regcache.h"
#include "gdb/signals.h"
#include "gdb/fileio.h"
#include "mem-break.h"
#include "win32-low.h"
#include "gdbthread.h"
#include "dll.h"
#include "hostio.h"

#include <stdint.h>
#include <windows.h>
#include <winnt.h>
#include <imagehlp.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <process.h>

#ifndef USE_WIN32API
#include <sys/cygwin.h>
#endif

#define OUTMSG(X) do { printf X; fflush (stderr); } while (0)

#define OUTMSG2(X) \
  do						\
    {						\
      if (debug_threads)			\
	{					\
	  printf X;				\
	  fflush (stderr);			\
	}					\
    } while (0)

#ifndef _T
#define _T(x) TEXT (x)
#endif

#ifndef COUNTOF
#define COUNTOF(STR) (sizeof (STR) / sizeof ((STR)[0]))
#endif

#ifdef _WIN32_WCE
# define GETPROCADDRESS(DLL, PROC) \
  ((winapi_ ## PROC) GetProcAddress (DLL, TEXT (#PROC)))
#else
# define GETPROCADDRESS(DLL, PROC) \
  ((winapi_ ## PROC) GetProcAddress (DLL, #PROC))
#endif

int using_threads = 1;

/* Globals.  */
static int attaching = 0;
static HANDLE current_process_handle = NULL;
static DWORD current_process_id = 0;
static DWORD main_thread_id = 0;
static enum gdb_signal last_sig = GDB_SIGNAL_0;

/* The current debug event from WaitForDebugEvent.  */
static DEBUG_EVENT current_event;

/* A status that hasn't been reported to the core yet, and so
   win32_wait should return it next, instead of fetching the next
   debug event off the win32 API.  */
static struct target_waitstatus cached_status;

/* Non zero if an interrupt request is to be satisfied by suspending
   all threads.  */
static int soft_interrupt_requested = 0;

/* Non zero if the inferior is stopped in a simulated breakpoint done
   by suspending all the threads.  */
static int faked_breakpoint = 0;

const struct target_desc *win32_tdesc;

#define NUM_REGS (the_low_target.num_regs)

typedef BOOL (WINAPI *winapi_DebugActiveProcessStop) (DWORD dwProcessId);
typedef BOOL (WINAPI *winapi_DebugSetProcessKillOnExit) (BOOL KillOnExit);
typedef BOOL (WINAPI *winapi_DebugBreakProcess) (HANDLE);
typedef BOOL (WINAPI *winapi_GenerateConsoleCtrlEvent) (DWORD, DWORD);

static ptid_t win32_wait (ptid_t ptid, struct target_waitstatus *ourstatus,
			  int options);
static void win32_resume (struct thread_resume *resume_info, size_t n);
#ifndef _WIN32_WCE
static void win32_ensure_ntdll_loaded (void);
#endif

/* Get the thread ID from the current selected inferior (the current
   thread).  */
static ptid_t
current_inferior_ptid (void)
{
  return ((struct inferior_list_entry*) current_inferior)->id;
}

/* The current debug event from WaitForDebugEvent.  */
static ptid_t
debug_event_ptid (DEBUG_EVENT *event)
{
  return ptid_build (event->dwProcessId, event->dwThreadId, 0);
}

/* Get the thread context of the thread associated with TH.  */

static void
win32_get_thread_context (win32_thread_info *th)
{
  memset (&th->context, 0, sizeof (CONTEXT));
  (*the_low_target.get_thread_context) (th, &current_event);
#ifdef _WIN32_WCE
  memcpy (&th->base_context, &th->context, sizeof (CONTEXT));
#endif
}

/* Set the thread context of the thread associated with TH.  */

static void
win32_set_thread_context (win32_thread_info *th)
{
#ifdef _WIN32_WCE
  /* Calling SuspendThread on a thread that is running kernel code
     will report that the suspending was successful, but in fact, that
     will often not be true.  In those cases, the context returned by
     GetThreadContext will not be correct by the time the thread
     stops, hence we can't set that context back into the thread when
     resuming - it will most likelly crash the inferior.
     Unfortunately, there is no way to know when the thread will
     really stop.  To work around it, we'll only write the context
     back to the thread when either the user or GDB explicitly change
     it between stopping and resuming.  */
  if (memcmp (&th->context, &th->base_context, sizeof (CONTEXT)) != 0)
#endif
    (*the_low_target.set_thread_context) (th, &current_event);
}

/* Find a thread record given a thread id.  If GET_CONTEXT is set then
   also retrieve the context for this thread.  */
static win32_thread_info *
thread_rec (ptid_t ptid, int get_context)
{
  struct thread_info *thread;
  win32_thread_info *th;

  thread = (struct thread_info *) find_inferior_id (&all_threads, ptid);
  if (thread == NULL)
    return NULL;

  th = inferior_target_data (thread);
  if (get_context && th->context.ContextFlags == 0)
    {
      if (!th->suspended)
	{
	  if (SuspendThread (th->h) == (DWORD) -1)
	    {
	      DWORD err = GetLastError ();
	      OUTMSG (("warning: SuspendThread failed in thread_rec, "
		       "(error %d): %s\n", (int) err, strwinerror (err)));
	    }
	  else
	    th->suspended = 1;
	}

      win32_get_thread_context (th);
    }

  return th;
}

/* Add a thread to the thread list.  */
static win32_thread_info *
child_add_thread (DWORD pid, DWORD tid, HANDLE h, void *tlb)
{
  win32_thread_info *th;
  ptid_t ptid = ptid_build (pid, tid, 0);

  if ((th = thread_rec (ptid, FALSE)))
    return th;

  th = xcalloc (1, sizeof (*th));
  th->tid = tid;
  th->h = h;
  th->thread_local_base = (CORE_ADDR) (uintptr_t) tlb;

  add_thread (ptid, th);

  if (the_low_target.thread_added != NULL)
    (*the_low_target.thread_added) (th);

  return th;
}

/* Delete a thread from the list of threads.  */
static void
delete_thread_info (struct inferior_list_entry *thread)
{
  win32_thread_info *th = inferior_target_data ((struct thread_info *) thread);

  remove_thread ((struct thread_info *) thread);
  CloseHandle (th->h);
  free (th);
}

/* Delete a thread from the list of threads.  */
static void
child_delete_thread (DWORD pid, DWORD tid)
{
  struct inferior_list_entry *thread;
  ptid_t ptid;

  /* If the last thread is exiting, just return.  */
  if (all_threads.head == all_threads.tail)
    return;

  ptid = ptid_build (pid, tid, 0);
  thread = find_inferior_id (&all_threads, ptid);
  if (thread == NULL)
    return;

  delete_thread_info (thread);
}

/* These watchpoint related wrapper functions simply pass on the function call
   if the low target has registered a corresponding function.  */

static int
win32_insert_point (char type, CORE_ADDR addr, int len)
{
  if (the_low_target.insert_point != NULL)
    return the_low_target.insert_point (type, addr, len);
  else
    /* Unsupported (see target.h).  */
    return 1;
}

static int
win32_remove_point (char type, CORE_ADDR addr, int len)
{
  if (the_low_target.remove_point != NULL)
    return the_low_target.remove_point (type, addr, len);
  else
    /* Unsupported (see target.h).  */
    return 1;
}

static int
win32_stopped_by_watchpoint (void)
{
  if (the_low_target.stopped_by_watchpoint != NULL)
    return the_low_target.stopped_by_watchpoint ();
  else
    return 0;
}

static CORE_ADDR
win32_stopped_data_address (void)
{
  if (the_low_target.stopped_data_address != NULL)
    return the_low_target.stopped_data_address ();
  else
    return 0;
}


/* Transfer memory from/to the debugged process.  */
static int
child_xfer_memory (CORE_ADDR memaddr, char *our, int len,
		   int write, struct target_ops *target)
{
  BOOL success;
  SIZE_T done = 0;
  DWORD lasterror = 0;
  uintptr_t addr = (uintptr_t) memaddr;

  if (write)
    {
      success = WriteProcessMemory (current_process_handle, (LPVOID) addr,
				    (LPCVOID) our, len, &done);
      if (!success)
	lasterror = GetLastError ();
      FlushInstructionCache (current_process_handle, (LPCVOID) addr, len);
    }
  else
    {
      success = ReadProcessMemory (current_process_handle, (LPCVOID) addr,
				   (LPVOID) our, len, &done);
      if (!success)
	lasterror = GetLastError ();
    }
  if (!success && lasterror == ERROR_PARTIAL_COPY && done > 0)
    return done;
  else
    return success ? done : -1;
}

/* Clear out any old thread list and reinitialize it to a pristine
   state. */
static void
child_init_thread_list (void)
{
  for_each_inferior (&all_threads, delete_thread_info);
}

static void
do_initial_child_stuff (HANDLE proch, DWORD pid, int attached)
{
  struct process_info *proc;

  last_sig = GDB_SIGNAL_0;

  current_process_handle = proch;
  current_process_id = pid;
  main_thread_id = 0;

  soft_interrupt_requested = 0;
  faked_breakpoint = 0;

  memset (&current_event, 0, sizeof (current_event));

  proc = add_process (pid, attached);
  proc->tdesc = win32_tdesc;
  child_init_thread_list ();

  if (the_low_target.initial_stuff != NULL)
    (*the_low_target.initial_stuff) ();

  cached_status.kind = TARGET_WAITKIND_IGNORE;

  /* Flush all currently pending debug events (thread and dll list) up
     to the initial breakpoint.  */
  while (1)
    {
      struct target_waitstatus status;

      win32_wait (minus_one_ptid, &status, 0);

      /* Note win32_wait doesn't return thread events.  */
      if (status.kind != TARGET_WAITKIND_LOADED)
	{
	  cached_status = status;
	  break;
	}

      {
	struct thread_resume resume;

	resume.thread = minus_one_ptid;
	resume.kind = resume_continue;
	resume.sig = 0;

	win32_resume (&resume, 1);
      }
    }

#ifndef _WIN32_WCE
  win32_ensure_ntdll_loaded ();
#endif
}

/* Resume all artificially suspended threads if we are continuing
   execution.  */
static int
continue_one_thread (struct inferior_list_entry *this_thread, void *id_ptr)
{
  struct thread_info *thread = (struct thread_info *) this_thread;
  int thread_id = * (int *) id_ptr;
  win32_thread_info *th = inferior_target_data (thread);

  if ((thread_id == -1 || thread_id == th->tid)
      && th->suspended)
    {
      if (th->context.ContextFlags)
	{
	  win32_set_thread_context (th);
	  th->context.ContextFlags = 0;
	}

      if (ResumeThread (th->h) == (DWORD) -1)
	{
	  DWORD err = GetLastError ();
	  OUTMSG (("warning: ResumeThread failed in continue_one_thread, "
		   "(error %d): %s\n", (int) err, strwinerror (err)));
	}
      th->suspended = 0;
    }

  return 0;
}

static BOOL
child_continue (DWORD continue_status, int thread_id)
{
  /* The inferior will only continue after the ContinueDebugEvent
     call.  */
  find_inferior (&all_threads, continue_one_thread, &thread_id);
  faked_breakpoint = 0;

  if (!ContinueDebugEvent (current_event.dwProcessId,
			   current_event.dwThreadId,
			   continue_status))
    return FALSE;

  return TRUE;
}

/* Fetch register(s) from the current thread context.  */
static void
child_fetch_inferior_registers (struct regcache *regcache, int r)
{
  int regno;
  win32_thread_info *th = thread_rec (current_inferior_ptid (), TRUE);
  if (r == -1 || r > NUM_REGS)
    child_fetch_inferior_registers (regcache, NUM_REGS);
  else
    for (regno = 0; regno < r; regno++)
      (*the_low_target.fetch_inferior_register) (regcache, th, regno);
}

/* Store a new register value into the current thread context.  We don't
   change the program's context until later, when we resume it.  */
static void
child_store_inferior_registers (struct regcache *regcache, int r)
{
  int regno;
  win32_thread_info *th = thread_rec (current_inferior_ptid (), TRUE);
  if (r == -1 || r == 0 || r > NUM_REGS)
    child_store_inferior_registers (regcache, NUM_REGS);
  else
    for (regno = 0; regno < r; regno++)
      (*the_low_target.store_inferior_register) (regcache, th, regno);
}

/* Map the Windows error number in ERROR to a locale-dependent error
   message string and return a pointer to it.  Typically, the values
   for ERROR come from GetLastError.

   The string pointed to shall not be modified by the application,
   but may be overwritten by a subsequent call to strwinerror

   The strwinerror function does not change the current setting
   of GetLastError.  */

char *
strwinerror (DWORD error)
{
  static char buf[1024];
  TCHAR *msgbuf;
  DWORD lasterr = GetLastError ();
  DWORD chars = FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM
			       | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			       NULL,
			       error,
			       0, /* Default language */
			       (LPVOID)&msgbuf,
			       0,
			       NULL);
  if (chars != 0)
    {
      /* If there is an \r\n appended, zap it.  */
      if (chars >= 2
	  && msgbuf[chars - 2] == '\r'
	  && msgbuf[chars - 1] == '\n')
	{
	  chars -= 2;
	  msgbuf[chars] = 0;
	}

      if (chars > ((COUNTOF (buf)) - 1))
	{
	  chars = COUNTOF (buf) - 1;
	  msgbuf [chars] = 0;
	}

#ifdef UNICODE
      wcstombs (buf, msgbuf, chars + 1);
#else
      strncpy (buf, msgbuf, chars + 1);
#endif
      LocalFree (msgbuf);
    }
  else
    sprintf (buf, "unknown win32 error (%u)", (unsigned) error);

  SetLastError (lasterr);
  return buf;
}

static BOOL
create_process (const char *program, char *args,
		DWORD flags, PROCESS_INFORMATION *pi)
{
  BOOL ret;

#ifdef _WIN32_WCE
  wchar_t *p, *wprogram, *wargs;
  size_t argslen;

  wprogram = alloca ((strlen (program) + 1) * sizeof (wchar_t));
  mbstowcs (wprogram, program, strlen (program) + 1);

  for (p = wprogram; *p; ++p)
    if (L'/' == *p)
      *p = L'\\';

  argslen = strlen (args);
  wargs = alloca ((argslen + 1) * sizeof (wchar_t));
  mbstowcs (wargs, args, argslen + 1);

  ret = CreateProcessW (wprogram, /* image name */
			wargs,    /* command line */
			NULL,     /* security, not supported */
			NULL,     /* thread, not supported */
			FALSE,    /* inherit handles, not supported */
			flags,    /* start flags */
			NULL,     /* environment, not supported */
			NULL,     /* current directory, not supported */
			NULL,     /* start info, not supported */
			pi);      /* proc info */
#else
  STARTUPINFOA si = { sizeof (STARTUPINFOA) };

  ret = CreateProcessA (program,  /* image name */
			args,     /* command line */
			NULL,     /* security */
			NULL,     /* thread */
			TRUE,     /* inherit handles */
			flags,    /* start flags */
			NULL,     /* environment */
			NULL,     /* current directory */
			&si,      /* start info */
			pi);      /* proc info */
#endif

  return ret;
}

/* Start a new process.
   PROGRAM is a path to the program to execute.
   ARGS is a standard NULL-terminated array of arguments,
   to be passed to the inferior as ``argv''.
   Returns the new PID on success, -1 on failure.  Registers the new
   process with the process list.  */
static int
win32_create_inferior (char *program, char **program_args)
{
#ifndef USE_WIN32API
  char real_path[PATH_MAX];
  char *orig_path, *new_path, *path_ptr;
#endif
  BOOL ret;
  DWORD flags;
  char *args;
  int argslen;
  int argc;
  PROCESS_INFORMATION pi;
  DWORD err;

  /* win32_wait needs to know we're not attaching.  */
  attaching = 0;

  if (!program)
    error ("No executable specified, specify executable to debug.\n");

  flags = DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS;

#ifndef USE_WIN32API
  orig_path = NULL;
  path_ptr = getenv ("PATH");
  if (path_ptr)
    {
      int size = cygwin_conv_path_list (CCP_POSIX_TO_WIN_A, path_ptr, NULL, 0);
      orig_path = alloca (strlen (path_ptr) + 1);
      new_path = alloca (size);
      strcpy (orig_path, path_ptr);
      cygwin_conv_path_list (CCP_POSIX_TO_WIN_A, path_ptr, new_path, size);
      setenv ("PATH", new_path, 1);
     }
  cygwin_conv_path (CCP_POSIX_TO_WIN_A, program, real_path, PATH_MAX);
  program = real_path;
#endif

  argslen = 1;
  for (argc = 1; program_args[argc]; argc++)
    argslen += strlen (program_args[argc]) + 1;
  args = alloca (argslen);
  args[0] = '\0';
  for (argc = 1; program_args[argc]; argc++)
    {
      /* FIXME: Can we do better about quoting?  How does Cygwin
	 handle this?  */
      strcat (args, " ");
      strcat (args, program_args[argc]);
    }
  OUTMSG2 (("Command line is \"%s\"\n", args));

#ifdef CREATE_NEW_PROCESS_GROUP
  flags |= CREATE_NEW_PROCESS_GROUP;
#endif

  ret = create_process (program, args, flags, &pi);
  err = GetLastError ();
  if (!ret && err == ERROR_FILE_NOT_FOUND)
    {
      char *exename = alloca (strlen (program) + 5);
      strcat (strcpy (exename, program), ".exe");
      ret = create_process (exename, args, flags, &pi);
      err = GetLastError ();
    }

#ifndef USE_WIN32API
  if (orig_path)
    setenv ("PATH", orig_path, 1);
#endif

  if (!ret)
    {
      error ("Error creating process \"%s%s\", (error %d): %s\n",
	     program, args, (int) err, strwinerror (err));
    }
  else
    {
      OUTMSG2 (("Process created: %s\n", (char *) args));
    }

#ifndef _WIN32_WCE
  /* On Windows CE this handle can't be closed.  The OS reuses
     it in the debug events, while the 9x/NT versions of Windows
     probably use a DuplicateHandle'd one.  */
  CloseHandle (pi.hThread);
#endif

  do_initial_child_stuff (pi.hProcess, pi.dwProcessId, 0);

  return current_process_id;
}

/* Attach to a running process.
   PID is the process ID to attach to, specified by the user
   or a higher layer.  */
static int
win32_attach (unsigned long pid)
{
  HANDLE h;
  winapi_DebugSetProcessKillOnExit DebugSetProcessKillOnExit = NULL;
  DWORD err;
#ifdef _WIN32_WCE
  HMODULE dll = GetModuleHandle (_T("COREDLL.DLL"));
#else
  HMODULE dll = GetModuleHandle (_T("KERNEL32.DLL"));
#endif
  DebugSetProcessKillOnExit = GETPROCADDRESS (dll, DebugSetProcessKillOnExit);

  h = OpenProcess (PROCESS_ALL_ACCESS, FALSE, pid);
  if (h != NULL)
    {
      if (DebugActiveProcess (pid))
	{
	  if (DebugSetProcessKillOnExit != NULL)
	    DebugSetProcessKillOnExit (FALSE);

	  /* win32_wait needs to know we're attaching.  */
	  attaching = 1;
	  do_initial_child_stuff (h, pid, 1);
	  return 0;
	}

      CloseHandle (h);
    }

  err = GetLastError ();
  error ("Attach to process failed (error %d): %s\n",
	 (int) err, strwinerror (err));
}

/* Handle OUTPUT_DEBUG_STRING_EVENT from child process.  */
static void
handle_output_debug_string (struct target_waitstatus *ourstatus)
{
#define READ_BUFFER_LEN 1024
  CORE_ADDR addr;
  char s[READ_BUFFER_LEN + 1] = { 0 };
  DWORD nbytes = current_event.u.DebugString.nDebugStringLength;

  if (nbytes == 0)
    return;

  if (nbytes > READ_BUFFER_LEN)
    nbytes = READ_BUFFER_LEN;

  addr = (CORE_ADDR) (size_t) current_event.u.DebugString.lpDebugStringData;

  if (current_event.u.DebugString.fUnicode)
    {
      /* The event tells us how many bytes, not chars, even
	 in Unicode.  */
      WCHAR buffer[(READ_BUFFER_LEN + 1) / sizeof (WCHAR)] = { 0 };
      if (read_inferior_memory (addr, (unsigned char *) buffer, nbytes) != 0)
	return;
      wcstombs (s, buffer, (nbytes + 1) / sizeof (WCHAR));
    }
  else
    {
      if (read_inferior_memory (addr, (unsigned char *) s, nbytes) != 0)
	return;
    }

  if (strncmp (s, "cYg", 3) != 0)
    {
      if (!server_waiting)
	{
	  OUTMSG2(("%s", s));
	  return;
	}

      monitor_output (s);
    }
#undef READ_BUFFER_LEN
}

static void
win32_clear_inferiors (void)
{
  if (current_process_handle != NULL)
    CloseHandle (current_process_handle);

  for_each_inferior (&all_threads, delete_thread_info);
  clear_inferiors ();
}

/* Kill all inferiors.  */
static int
win32_kill (int pid)
{
  struct process_info *process;

  if (current_process_handle == NULL)
    return -1;

  TerminateProcess (current_process_handle, 0);
  for (;;)
    {
      if (!child_continue (DBG_CONTINUE, -1))
	break;
      if (!WaitForDebugEvent (&current_event, INFINITE))
	break;
      if (current_event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
	break;
      else if (current_event.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
	{
	  struct target_waitstatus our_status = { 0 };
	  handle_output_debug_string (&our_status);
	}
    }

  win32_clear_inferiors ();

  process = find_process_pid (pid);
  remove_process (process);
  return 0;
}

/* Detach from inferior PID.  */
static int
win32_detach (int pid)
{
  struct process_info *process;
  winapi_DebugActiveProcessStop DebugActiveProcessStop = NULL;
  winapi_DebugSetProcessKillOnExit DebugSetProcessKillOnExit = NULL;
#ifdef _WIN32_WCE
  HMODULE dll = GetModuleHandle (_T("COREDLL.DLL"));
#else
  HMODULE dll = GetModuleHandle (_T("KERNEL32.DLL"));
#endif
  DebugActiveProcessStop = GETPROCADDRESS (dll, DebugActiveProcessStop);
  DebugSetProcessKillOnExit = GETPROCADDRESS (dll, DebugSetProcessKillOnExit);

  if (DebugSetProcessKillOnExit == NULL
      || DebugActiveProcessStop == NULL)
    return -1;

  {
    struct thread_resume resume;
    resume.thread = minus_one_ptid;
    resume.kind = resume_continue;
    resume.sig = 0;
    win32_resume (&resume, 1);
  }

  if (!DebugActiveProcessStop (current_process_id))
    return -1;

  DebugSetProcessKillOnExit (FALSE);
  process = find_process_pid (pid);
  remove_process (process);

  win32_clear_inferiors ();
  return 0;
}

static void
win32_mourn (struct process_info *process)
{
  remove_process (process);
}

/* Wait for inferiors to end.  */
static void
win32_join (int pid)
{
  HANDLE h = OpenProcess (PROCESS_ALL_ACCESS, FALSE, pid);
  if (h != NULL)
    {
      WaitForSingleObject (h, INFINITE);
      CloseHandle (h);
    }
}

/* Return 1 iff the thread with thread ID TID is alive.  */
static int
win32_thread_alive (ptid_t ptid)
{
  int res;

  /* Our thread list is reliable; don't bother to poll target
     threads.  */
  if (find_inferior_id (&all_threads, ptid) != NULL)
    res = 1;
  else
    res = 0;
  return res;
}

/* Resume the inferior process.  RESUME_INFO describes how we want
   to resume.  */
static void
win32_resume (struct thread_resume *resume_info, size_t n)
{
  DWORD tid;
  enum gdb_signal sig;
  int step;
  win32_thread_info *th;
  DWORD continue_status = DBG_CONTINUE;
  ptid_t ptid;

  /* This handles the very limited set of resume packets that GDB can
     currently produce.  */

  if (n == 1 && ptid_equal (resume_info[0].thread, minus_one_ptid))
    tid = -1;
  else if (n > 1)
    tid = -1;
  else
    /* Yes, we're ignoring resume_info[0].thread.  It'd be tricky to make
       the Windows resume code do the right thing for thread switching.  */
    tid = current_event.dwThreadId;

  if (!ptid_equal (resume_info[0].thread, minus_one_ptid))
    {
      sig = resume_info[0].sig;
      step = resume_info[0].kind == resume_step;
    }
  else
    {
      sig = 0;
      step = 0;
    }

  if (sig != GDB_SIGNAL_0)
    {
      if (current_event.dwDebugEventCode != EXCEPTION_DEBUG_EVENT)
	{
	  OUTMSG (("Cannot continue with signal %d here.\n", sig));
	}
      else if (sig == last_sig)
	continue_status = DBG_EXCEPTION_NOT_HANDLED;
      else
	OUTMSG (("Can only continue with recieved signal %d.\n", last_sig));
    }

  last_sig = GDB_SIGNAL_0;

  /* Get context for the currently selected thread.  */
  ptid = debug_event_ptid (&current_event);
  th = thread_rec (ptid, FALSE);
  if (th)
    {
      if (th->context.ContextFlags)
	{
	  /* Move register values from the inferior into the thread
	     context structure.  */
	  regcache_invalidate ();

	  if (step)
	    {
	      if (the_low_target.single_step != NULL)
		(*the_low_target.single_step) (th);
	      else
		error ("Single stepping is not supported "
		       "in this configuration.\n");
	    }

	  win32_set_thread_context (th);
	  th->context.ContextFlags = 0;
	}
    }

  /* Allow continuing with the same signal that interrupted us.
     Otherwise complain.  */

  child_continue (continue_status, tid);
}

static void
win32_add_one_solib (const char *name, CORE_ADDR load_addr)
{
  char buf[MAX_PATH + 1];
  char buf2[MAX_PATH + 1];

#ifdef _WIN32_WCE
  WIN32_FIND_DATA w32_fd;
  WCHAR wname[MAX_PATH + 1];
  mbstowcs (wname, name, MAX_PATH);
  HANDLE h = FindFirstFile (wname, &w32_fd);
#else
  WIN32_FIND_DATAA w32_fd;
  HANDLE h = FindFirstFileA (name, &w32_fd);
#endif

  if (h == INVALID_HANDLE_VALUE)
    strcpy (buf, name);
  else
    {
      FindClose (h);
      strcpy (buf, name);
#ifndef _WIN32_WCE
      {
	char cwd[MAX_PATH + 1];
	char *p;
	if (GetCurrentDirectoryA (MAX_PATH + 1, cwd))
	  {
	    p = strrchr (buf, '\\');
	    if (p)
	      p[1] = '\0';
	    SetCurrentDirectoryA (buf);
	    GetFullPathNameA (w32_fd.cFileName, MAX_PATH, buf, &p);
	    SetCurrentDirectoryA (cwd);
	  }
      }
#endif
    }

#ifndef _WIN32_WCE
  if (strcasecmp (buf, "ntdll.dll") == 0)
    {
      GetSystemDirectoryA (buf, sizeof (buf));
      strcat (buf, "\\ntdll.dll");
    }
#endif

#ifdef __CYGWIN__
  cygwin_conv_path (CCP_WIN_A_TO_POSIX, buf, buf2, sizeof (buf2));
#else
  strcpy (buf2, buf);
#endif

  loaded_dll (buf2, load_addr);
}

static char *
get_image_name (HANDLE h, void *address, int unicode)
{
  static char buf[(2 * MAX_PATH) + 1];
  DWORD size = unicode ? sizeof (WCHAR) : sizeof (char);
  char *address_ptr;
  int len = 0;
  char b[2];
  SIZE_T done;

  /* Attempt to read the name of the dll that was detected.
     This is documented to work only when actively debugging
     a program.  It will not work for attached processes. */
  if (address == NULL)
    return NULL;

#ifdef _WIN32_WCE
  /* Windows CE reports the address of the image name,
     instead of an address of a pointer into the image name.  */
  address_ptr = address;
#else
  /* See if we could read the address of a string, and that the
     address isn't null. */
  if (!ReadProcessMemory (h, address,  &address_ptr,
			  sizeof (address_ptr), &done)
      || done != sizeof (address_ptr)
      || !address_ptr)
    return NULL;
#endif

  /* Find the length of the string */
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

      WideCharToMultiByte (CP_ACP, 0, unicode_address, len, buf, len, 0, 0);
    }

  return buf;
}

typedef BOOL (WINAPI *winapi_EnumProcessModules) (HANDLE, HMODULE *,
						  DWORD, LPDWORD);
typedef BOOL (WINAPI *winapi_GetModuleInformation) (HANDLE, HMODULE,
						    LPMODULEINFO, DWORD);
typedef DWORD (WINAPI *winapi_GetModuleFileNameExA) (HANDLE, HMODULE,
						     LPSTR, DWORD);

static winapi_EnumProcessModules win32_EnumProcessModules;
static winapi_GetModuleInformation win32_GetModuleInformation;
static winapi_GetModuleFileNameExA win32_GetModuleFileNameExA;

static BOOL
load_psapi (void)
{
  static int psapi_loaded = 0;
  static HMODULE dll = NULL;

  if (!psapi_loaded)
    {
      psapi_loaded = 1;
      dll = LoadLibrary (TEXT("psapi.dll"));
      if (!dll)
	return FALSE;
      win32_EnumProcessModules =
	      GETPROCADDRESS (dll, EnumProcessModules);
      win32_GetModuleInformation =
	      GETPROCADDRESS (dll, GetModuleInformation);
      win32_GetModuleFileNameExA =
	      GETPROCADDRESS (dll, GetModuleFileNameExA);
    }

  return (win32_EnumProcessModules != NULL
	  && win32_GetModuleInformation != NULL
	  && win32_GetModuleFileNameExA != NULL);
}

static int
psapi_get_dll_name (LPVOID BaseAddress, char *dll_name_ret)
{
  DWORD len;
  MODULEINFO mi;
  size_t i;
  HMODULE dh_buf[1];
  HMODULE *DllHandle = dh_buf;
  DWORD cbNeeded;
  BOOL ok;

  if (!load_psapi ())
    goto failed;

  cbNeeded = 0;
  ok = (*win32_EnumProcessModules) (current_process_handle,
				    DllHandle,
				    sizeof (HMODULE),
				    &cbNeeded);

  if (!ok || !cbNeeded)
    goto failed;

  DllHandle = (HMODULE *) alloca (cbNeeded);
  if (!DllHandle)
    goto failed;

  ok = (*win32_EnumProcessModules) (current_process_handle,
				    DllHandle,
				    cbNeeded,
				    &cbNeeded);
  if (!ok)
    goto failed;

  for (i = 0; i < ((size_t) cbNeeded / sizeof (HMODULE)); i++)
    {
      if (!(*win32_GetModuleInformation) (current_process_handle,
					  DllHandle[i],
					  &mi,
					  sizeof (mi)))
	{
	  DWORD err = GetLastError ();
	  error ("Can't get module info: (error %d): %s\n",
		 (int) err, strwinerror (err));
	}

      if (mi.lpBaseOfDll == BaseAddress)
	{
	  len = (*win32_GetModuleFileNameExA) (current_process_handle,
					       DllHandle[i],
					       dll_name_ret,
					       MAX_PATH);
	  if (len == 0)
	    {
	      DWORD err = GetLastError ();
	      error ("Error getting dll name: (error %d): %s\n",
		     (int) err, strwinerror (err));
	    }
	  return 1;
	}
    }

failed:
  dll_name_ret[0] = '\0';
  return 0;
}

#ifndef _WIN32_WCE
/* On certain versions of Windows, the information about ntdll.dll
   is not available yet at the time we get the LOAD_DLL_DEBUG_EVENT,
   thus preventing us from reporting this DLL as an SO. This has been
   witnessed on Windows 8.1, for instance.  A possible explanation
   is that ntdll.dll might be mapped before the SO info gets created
   by the Windows system -- ntdll.dll is the first DLL to be reported
   via LOAD_DLL_DEBUG_EVENT and other DLLs do not seem to suffer from
   that problem.

   If we indeed are missing ntdll.dll, this function tries to recover
   from this issue, after the fact.  Do nothing if we encounter any
   issue trying to locate that DLL.  */

static void
win32_ensure_ntdll_loaded (void)
{
  struct inferior_list_entry *dll_e;
  size_t i;
  HMODULE dh_buf[1];
  HMODULE *DllHandle = dh_buf;
  DWORD cbNeeded;
  BOOL ok;

  for (dll_e = all_dlls.head; dll_e != NULL; dll_e = dll_e->next)
    {
      struct dll_info *dll = (struct dll_info *) dll_e;

      if (strcasecmp (lbasename (dll->name), "ntdll.dll") == 0)
	return;
    }

  if (!load_psapi ())
    return;

  cbNeeded = 0;
  ok = (*win32_EnumProcessModules) (current_process_handle,
				    DllHandle,
				    sizeof (HMODULE),
				    &cbNeeded);

  if (!ok || !cbNeeded)
    return;

  DllHandle = (HMODULE *) alloca (cbNeeded);
  if (!DllHandle)
    return;

  ok = (*win32_EnumProcessModules) (current_process_handle,
				    DllHandle,
				    cbNeeded,
				    &cbNeeded);
  if (!ok)
    return;

  for (i = 0; i < ((size_t) cbNeeded / sizeof (HMODULE)); i++)
    {
      MODULEINFO mi;
      char dll_name[MAX_PATH];

      if (!(*win32_GetModuleInformation) (current_process_handle,
					  DllHandle[i],
					  &mi,
					  sizeof (mi)))
	continue;
      if ((*win32_GetModuleFileNameExA) (current_process_handle,
					 DllHandle[i],
					 dll_name,
					 MAX_PATH) == 0)
	continue;
      if (strcasecmp (lbasename (dll_name), "ntdll.dll") == 0)
	{
	  win32_add_one_solib (dll_name,
			       (CORE_ADDR) (uintptr_t) mi.lpBaseOfDll);
	  return;
	}
    }
}
#endif

typedef HANDLE (WINAPI *winapi_CreateToolhelp32Snapshot) (DWORD, DWORD);
typedef BOOL (WINAPI *winapi_Module32First) (HANDLE, LPMODULEENTRY32);
typedef BOOL (WINAPI *winapi_Module32Next) (HANDLE, LPMODULEENTRY32);

static winapi_CreateToolhelp32Snapshot win32_CreateToolhelp32Snapshot;
static winapi_Module32First win32_Module32First;
static winapi_Module32Next win32_Module32Next;
#ifdef _WIN32_WCE
typedef BOOL (WINAPI *winapi_CloseToolhelp32Snapshot) (HANDLE);
static winapi_CloseToolhelp32Snapshot win32_CloseToolhelp32Snapshot;
#endif

static BOOL
load_toolhelp (void)
{
  static int toolhelp_loaded = 0;
  static HMODULE dll = NULL;

  if (!toolhelp_loaded)
    {
      toolhelp_loaded = 1;
#ifndef _WIN32_WCE
      dll = GetModuleHandle (_T("KERNEL32.DLL"));
#else
      dll = LoadLibrary (L"TOOLHELP.DLL");
#endif
      if (!dll)
	return FALSE;

      win32_CreateToolhelp32Snapshot =
	GETPROCADDRESS (dll, CreateToolhelp32Snapshot);
      win32_Module32First = GETPROCADDRESS (dll, Module32First);
      win32_Module32Next = GETPROCADDRESS (dll, Module32Next);
#ifdef _WIN32_WCE
      win32_CloseToolhelp32Snapshot =
	GETPROCADDRESS (dll, CloseToolhelp32Snapshot);
#endif
    }

  return (win32_CreateToolhelp32Snapshot != NULL
	  && win32_Module32First != NULL
	  && win32_Module32Next != NULL
#ifdef _WIN32_WCE
	  && win32_CloseToolhelp32Snapshot != NULL
#endif
	  );
}

static int
toolhelp_get_dll_name (LPVOID BaseAddress, char *dll_name_ret)
{
  HANDLE snapshot_module;
  MODULEENTRY32 modEntry = { sizeof (MODULEENTRY32) };
  int found = 0;

  if (!load_toolhelp ())
    return 0;

  snapshot_module = win32_CreateToolhelp32Snapshot (TH32CS_SNAPMODULE,
						    current_event.dwProcessId);
  if (snapshot_module == INVALID_HANDLE_VALUE)
    return 0;

  /* Ignore the first module, which is the exe.  */
  if (win32_Module32First (snapshot_module, &modEntry))
    while (win32_Module32Next (snapshot_module, &modEntry))
      if (modEntry.modBaseAddr == BaseAddress)
	{
#ifdef UNICODE
	  wcstombs (dll_name_ret, modEntry.szExePath, MAX_PATH + 1);
#else
	  strcpy (dll_name_ret, modEntry.szExePath);
#endif
	  found = 1;
	  break;
	}

#ifdef _WIN32_WCE
  win32_CloseToolhelp32Snapshot (snapshot_module);
#else
  CloseHandle (snapshot_module);
#endif
  return found;
}

static void
handle_load_dll (void)
{
  LOAD_DLL_DEBUG_INFO *event = &current_event.u.LoadDll;
  char dll_buf[MAX_PATH + 1];
  char *dll_name = NULL;
  CORE_ADDR load_addr;

  dll_buf[0] = dll_buf[sizeof (dll_buf) - 1] = '\0';

  /* Windows does not report the image name of the dlls in the debug
     event on attaches.  We resort to iterating over the list of
     loaded dlls looking for a match by image base.  */
  if (!psapi_get_dll_name (event->lpBaseOfDll, dll_buf))
    {
      if (!server_waiting)
	/* On some versions of Windows and Windows CE, we can't create
	   toolhelp snapshots while the inferior is stopped in a
	   LOAD_DLL_DEBUG_EVENT due to a dll load, but we can while
	   Windows is reporting the already loaded dlls.  */
	toolhelp_get_dll_name (event->lpBaseOfDll, dll_buf);
    }

  dll_name = dll_buf;

  if (*dll_name == '\0')
    dll_name = get_image_name (current_process_handle,
			       event->lpImageName, event->fUnicode);
  if (!dll_name)
    return;

  /* The symbols in a dll are offset by 0x1000, which is the
     offset from 0 of the first byte in an image - because
     of the file header and the section alignment. */

  load_addr = (CORE_ADDR) (uintptr_t) event->lpBaseOfDll + 0x1000;
  win32_add_one_solib (dll_name, load_addr);
}

static void
handle_unload_dll (void)
{
  CORE_ADDR load_addr =
	  (CORE_ADDR) (uintptr_t) current_event.u.UnloadDll.lpBaseOfDll;
  load_addr += 0x1000;
  unloaded_dll (NULL, load_addr);
}

static void
handle_exception (struct target_waitstatus *ourstatus)
{
  DWORD code = current_event.u.Exception.ExceptionRecord.ExceptionCode;

  ourstatus->kind = TARGET_WAITKIND_STOPPED;

  switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:
      OUTMSG2 (("EXCEPTION_ACCESS_VIOLATION"));
      ourstatus->value.sig = GDB_SIGNAL_SEGV;
      break;
    case STATUS_STACK_OVERFLOW:
      OUTMSG2 (("STATUS_STACK_OVERFLOW"));
      ourstatus->value.sig = GDB_SIGNAL_SEGV;
      break;
    case STATUS_FLOAT_DENORMAL_OPERAND:
      OUTMSG2 (("STATUS_FLOAT_DENORMAL_OPERAND"));
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      OUTMSG2 (("EXCEPTION_ARRAY_BOUNDS_EXCEEDED"));
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_FLOAT_INEXACT_RESULT:
      OUTMSG2 (("STATUS_FLOAT_INEXACT_RESULT"));
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_FLOAT_INVALID_OPERATION:
      OUTMSG2 (("STATUS_FLOAT_INVALID_OPERATION"));
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_FLOAT_OVERFLOW:
      OUTMSG2 (("STATUS_FLOAT_OVERFLOW"));
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_FLOAT_STACK_CHECK:
      OUTMSG2 (("STATUS_FLOAT_STACK_CHECK"));
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_FLOAT_UNDERFLOW:
      OUTMSG2 (("STATUS_FLOAT_UNDERFLOW"));
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_FLOAT_DIVIDE_BY_ZERO:
      OUTMSG2 (("STATUS_FLOAT_DIVIDE_BY_ZERO"));
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_INTEGER_DIVIDE_BY_ZERO:
      OUTMSG2 (("STATUS_INTEGER_DIVIDE_BY_ZERO"));
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case STATUS_INTEGER_OVERFLOW:
      OUTMSG2 (("STATUS_INTEGER_OVERFLOW"));
      ourstatus->value.sig = GDB_SIGNAL_FPE;
      break;
    case EXCEPTION_BREAKPOINT:
      OUTMSG2 (("EXCEPTION_BREAKPOINT"));
      ourstatus->value.sig = GDB_SIGNAL_TRAP;
#ifdef _WIN32_WCE
      /* Remove the initial breakpoint.  */
      check_breakpoints ((CORE_ADDR) (long) current_event
			 .u.Exception.ExceptionRecord.ExceptionAddress);
#endif
      break;
    case DBG_CONTROL_C:
      OUTMSG2 (("DBG_CONTROL_C"));
      ourstatus->value.sig = GDB_SIGNAL_INT;
      break;
    case DBG_CONTROL_BREAK:
      OUTMSG2 (("DBG_CONTROL_BREAK"));
      ourstatus->value.sig = GDB_SIGNAL_INT;
      break;
    case EXCEPTION_SINGLE_STEP:
      OUTMSG2 (("EXCEPTION_SINGLE_STEP"));
      ourstatus->value.sig = GDB_SIGNAL_TRAP;
      break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      OUTMSG2 (("EXCEPTION_ILLEGAL_INSTRUCTION"));
      ourstatus->value.sig = GDB_SIGNAL_ILL;
      break;
    case EXCEPTION_PRIV_INSTRUCTION:
      OUTMSG2 (("EXCEPTION_PRIV_INSTRUCTION"));
      ourstatus->value.sig = GDB_SIGNAL_ILL;
      break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      OUTMSG2 (("EXCEPTION_NONCONTINUABLE_EXCEPTION"));
      ourstatus->value.sig = GDB_SIGNAL_ILL;
      break;
    default:
      if (current_event.u.Exception.dwFirstChance)
	{
	  ourstatus->kind = TARGET_WAITKIND_SPURIOUS;
	  return;
	}
      OUTMSG2 (("gdbserver: unknown target exception 0x%08x at 0x%s",
	    (unsigned) current_event.u.Exception.ExceptionRecord.ExceptionCode,
	    phex_nz ((uintptr_t) current_event.u.Exception.ExceptionRecord.
	    ExceptionAddress, sizeof (uintptr_t))));
      ourstatus->value.sig = GDB_SIGNAL_UNKNOWN;
      break;
    }
  OUTMSG2 (("\n"));
  last_sig = ourstatus->value.sig;
}


static void
suspend_one_thread (struct inferior_list_entry *entry)
{
  struct thread_info *thread = (struct thread_info *) entry;
  win32_thread_info *th = inferior_target_data (thread);

  if (!th->suspended)
    {
      if (SuspendThread (th->h) == (DWORD) -1)
	{
	  DWORD err = GetLastError ();
	  OUTMSG (("warning: SuspendThread failed in suspend_one_thread, "
		   "(error %d): %s\n", (int) err, strwinerror (err)));
	}
      else
	th->suspended = 1;
    }
}

static void
fake_breakpoint_event (void)
{
  OUTMSG2(("fake_breakpoint_event\n"));

  faked_breakpoint = 1;

  memset (&current_event, 0, sizeof (current_event));
  current_event.dwThreadId = main_thread_id;
  current_event.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
  current_event.u.Exception.ExceptionRecord.ExceptionCode
    = EXCEPTION_BREAKPOINT;

  for_each_inferior (&all_threads, suspend_one_thread);
}

#ifdef _WIN32_WCE
static int
auto_delete_breakpoint (CORE_ADDR stop_pc)
{
  return 1;
}
#endif

/* Get the next event from the child.  */

static int
get_child_debug_event (struct target_waitstatus *ourstatus)
{
  ptid_t ptid;

  last_sig = GDB_SIGNAL_0;
  ourstatus->kind = TARGET_WAITKIND_SPURIOUS;

  /* Check if GDB sent us an interrupt request.  */
  check_remote_input_interrupt_request ();

  if (soft_interrupt_requested)
    {
      soft_interrupt_requested = 0;
      fake_breakpoint_event ();
      goto gotevent;
    }

#ifndef _WIN32_WCE
  attaching = 0;
#else
  if (attaching)
    {
      /* WinCE doesn't set an initial breakpoint automatically.  To
	 stop the inferior, we flush all currently pending debug
	 events -- the thread list and the dll list are always
	 reported immediatelly without delay, then, we suspend all
	 threads and pretend we saw a trap at the current PC of the
	 main thread.

	 Contrary to desktop Windows, Windows CE *does* report the dll
	 names on LOAD_DLL_DEBUG_EVENTs resulting from a
	 DebugActiveProcess call.  This limits the way we can detect
	 if all the dlls have already been reported.  If we get a real
	 debug event before leaving attaching, the worst that will
	 happen is the user will see a spurious breakpoint.  */

      current_event.dwDebugEventCode = 0;
      if (!WaitForDebugEvent (&current_event, 0))
	{
	  OUTMSG2(("no attach events left\n"));
	  fake_breakpoint_event ();
	  attaching = 0;
	}
      else
	OUTMSG2(("got attach event\n"));
    }
  else
#endif
    {
      /* Keep the wait time low enough for confortable remote
	 interruption, but high enough so gdbserver doesn't become a
	 bottleneck.  */
      if (!WaitForDebugEvent (&current_event, 250))
        {
	  DWORD e  = GetLastError();

	  if (e == ERROR_PIPE_NOT_CONNECTED)
	    {
	      /* This will happen if the loader fails to succesfully
		 load the application, e.g., if the main executable
		 tries to pull in a non-existing export from a
		 DLL.  */
	      ourstatus->kind = TARGET_WAITKIND_EXITED;
	      ourstatus->value.integer = 1;
	      return 1;
	    }

	  return 0;
        }
    }

 gotevent:

  switch (current_event.dwDebugEventCode)
    {
    case CREATE_THREAD_DEBUG_EVENT:
      OUTMSG2 (("gdbserver: kernel event CREATE_THREAD_DEBUG_EVENT "
		"for pid=%u tid=%x)\n",
		(unsigned) current_event.dwProcessId,
		(unsigned) current_event.dwThreadId));

      /* Record the existence of this thread.  */
      child_add_thread (current_event.dwProcessId,
			current_event.dwThreadId,
			current_event.u.CreateThread.hThread,
			current_event.u.CreateThread.lpThreadLocalBase);
      break;

    case EXIT_THREAD_DEBUG_EVENT:
      OUTMSG2 (("gdbserver: kernel event EXIT_THREAD_DEBUG_EVENT "
		"for pid=%u tid=%x\n",
		(unsigned) current_event.dwProcessId,
		(unsigned) current_event.dwThreadId));
      child_delete_thread (current_event.dwProcessId,
			   current_event.dwThreadId);

      current_inferior = (struct thread_info *) all_threads.head;
      return 1;

    case CREATE_PROCESS_DEBUG_EVENT:
      OUTMSG2 (("gdbserver: kernel event CREATE_PROCESS_DEBUG_EVENT "
		"for pid=%u tid=%x\n",
		(unsigned) current_event.dwProcessId,
		(unsigned) current_event.dwThreadId));
      CloseHandle (current_event.u.CreateProcessInfo.hFile);

      current_process_handle = current_event.u.CreateProcessInfo.hProcess;
      main_thread_id = current_event.dwThreadId;

      ourstatus->kind = TARGET_WAITKIND_EXECD;
      ourstatus->value.execd_pathname = "Main executable";

      /* Add the main thread.  */
      child_add_thread (current_event.dwProcessId,
			main_thread_id,
			current_event.u.CreateProcessInfo.hThread,
			current_event.u.CreateProcessInfo.lpThreadLocalBase);

      ourstatus->value.related_pid = debug_event_ptid (&current_event);
#ifdef _WIN32_WCE
      if (!attaching)
	{
	  /* Windows CE doesn't set the initial breakpoint
	     automatically like the desktop versions of Windows do.
	     We add it explicitly here.	 It will be removed as soon as
	     it is hit.	 */
	  set_breakpoint_at ((CORE_ADDR) (long) current_event.u
			     .CreateProcessInfo.lpStartAddress,
			     auto_delete_breakpoint);
	}
#endif
      break;

    case EXIT_PROCESS_DEBUG_EVENT:
      OUTMSG2 (("gdbserver: kernel event EXIT_PROCESS_DEBUG_EVENT "
		"for pid=%u tid=%x\n",
		(unsigned) current_event.dwProcessId,
		(unsigned) current_event.dwThreadId));
      ourstatus->kind = TARGET_WAITKIND_EXITED;
      ourstatus->value.integer = current_event.u.ExitProcess.dwExitCode;
      child_continue (DBG_CONTINUE, -1);
      CloseHandle (current_process_handle);
      current_process_handle = NULL;
      break;

    case LOAD_DLL_DEBUG_EVENT:
      OUTMSG2 (("gdbserver: kernel event LOAD_DLL_DEBUG_EVENT "
		"for pid=%u tid=%x\n",
		(unsigned) current_event.dwProcessId,
		(unsigned) current_event.dwThreadId));
      CloseHandle (current_event.u.LoadDll.hFile);
      handle_load_dll ();

      ourstatus->kind = TARGET_WAITKIND_LOADED;
      ourstatus->value.sig = GDB_SIGNAL_TRAP;
      break;

    case UNLOAD_DLL_DEBUG_EVENT:
      OUTMSG2 (("gdbserver: kernel event UNLOAD_DLL_DEBUG_EVENT "
		"for pid=%u tid=%x\n",
		(unsigned) current_event.dwProcessId,
		(unsigned) current_event.dwThreadId));
      handle_unload_dll ();
      ourstatus->kind = TARGET_WAITKIND_LOADED;
      ourstatus->value.sig = GDB_SIGNAL_TRAP;
      break;

    case EXCEPTION_DEBUG_EVENT:
      OUTMSG2 (("gdbserver: kernel event EXCEPTION_DEBUG_EVENT "
		"for pid=%u tid=%x\n",
		(unsigned) current_event.dwProcessId,
		(unsigned) current_event.dwThreadId));
      handle_exception (ourstatus);
      break;

    case OUTPUT_DEBUG_STRING_EVENT:
      /* A message from the kernel (or Cygwin).  */
      OUTMSG2 (("gdbserver: kernel event OUTPUT_DEBUG_STRING_EVENT "
		"for pid=%u tid=%x\n",
		(unsigned) current_event.dwProcessId,
		(unsigned) current_event.dwThreadId));
      handle_output_debug_string (ourstatus);
      break;

    default:
      OUTMSG2 (("gdbserver: kernel event unknown "
		"for pid=%u tid=%x code=%x\n",
		(unsigned) current_event.dwProcessId,
		(unsigned) current_event.dwThreadId,
		(unsigned) current_event.dwDebugEventCode));
      break;
    }

  ptid = debug_event_ptid (&current_event);
  current_inferior =
    (struct thread_info *) find_inferior_id (&all_threads, ptid);
  return 1;
}

/* Wait for the inferior process to change state.
   STATUS will be filled in with a response code to send to GDB.
   Returns the signal which caused the process to stop. */
static ptid_t
win32_wait (ptid_t ptid, struct target_waitstatus *ourstatus, int options)
{
  struct regcache *regcache;

  if (cached_status.kind != TARGET_WAITKIND_IGNORE)
    {
      /* The core always does a wait after creating the inferior, and
	 do_initial_child_stuff already ran the inferior to the
	 initial breakpoint (or an exit, if creating the process
	 fails).  Report it now.  */
      *ourstatus = cached_status;
      cached_status.kind = TARGET_WAITKIND_IGNORE;
      return debug_event_ptid (&current_event);
    }

  while (1)
    {
      if (!get_child_debug_event (ourstatus))
	continue;

      switch (ourstatus->kind)
	{
	case TARGET_WAITKIND_EXITED:
	  OUTMSG2 (("Child exited with retcode = %x\n",
		    ourstatus->value.integer));
	  win32_clear_inferiors ();
	  return pid_to_ptid (current_event.dwProcessId);
	case TARGET_WAITKIND_STOPPED:
	case TARGET_WAITKIND_LOADED:
	  OUTMSG2 (("Child Stopped with signal = %d \n",
		    ourstatus->value.sig));

	  regcache = get_thread_regcache (current_inferior, 1);
	  child_fetch_inferior_registers (regcache, -1);
	  return debug_event_ptid (&current_event);
	default:
	  OUTMSG (("Ignoring unknown internal event, %d\n", ourstatus->kind));
	  /* fall-through */
	case TARGET_WAITKIND_SPURIOUS:
	case TARGET_WAITKIND_EXECD:
	  /* do nothing, just continue */
	  child_continue (DBG_CONTINUE, -1);
	  break;
	}
    }
}

/* Fetch registers from the inferior process.
   If REGNO is -1, fetch all registers; otherwise, fetch at least REGNO.  */
static void
win32_fetch_inferior_registers (struct regcache *regcache, int regno)
{
  child_fetch_inferior_registers (regcache, regno);
}

/* Store registers to the inferior process.
   If REGNO is -1, store all registers; otherwise, store at least REGNO.  */
static void
win32_store_inferior_registers (struct regcache *regcache, int regno)
{
  child_store_inferior_registers (regcache, regno);
}

/* Read memory from the inferior process.  This should generally be
   called through read_inferior_memory, which handles breakpoint shadowing.
   Read LEN bytes at MEMADDR into a buffer at MYADDR.  */
static int
win32_read_inferior_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
{
  return child_xfer_memory (memaddr, (char *) myaddr, len, 0, 0) != len;
}

/* Write memory to the inferior process.  This should generally be
   called through write_inferior_memory, which handles breakpoint shadowing.
   Write LEN bytes from the buffer at MYADDR to MEMADDR.
   Returns 0 on success and errno on failure.  */
static int
win32_write_inferior_memory (CORE_ADDR memaddr, const unsigned char *myaddr,
			     int len)
{
  return child_xfer_memory (memaddr, (char *) myaddr, len, 1, 0) != len;
}

/* Send an interrupt request to the inferior process. */
static void
win32_request_interrupt (void)
{
  winapi_DebugBreakProcess DebugBreakProcess;
  winapi_GenerateConsoleCtrlEvent GenerateConsoleCtrlEvent;

#ifdef _WIN32_WCE
  HMODULE dll = GetModuleHandle (_T("COREDLL.DLL"));
#else
  HMODULE dll = GetModuleHandle (_T("KERNEL32.DLL"));
#endif

  GenerateConsoleCtrlEvent = GETPROCADDRESS (dll, GenerateConsoleCtrlEvent);

  if (GenerateConsoleCtrlEvent != NULL
      && GenerateConsoleCtrlEvent (CTRL_BREAK_EVENT, current_process_id))
    return;

  /* GenerateConsoleCtrlEvent can fail if process id being debugged is
     not a process group id.
     Fallback to XP/Vista 'DebugBreakProcess', which generates a
     breakpoint exception in the interior process.  */

  DebugBreakProcess = GETPROCADDRESS (dll, DebugBreakProcess);

  if (DebugBreakProcess != NULL
      && DebugBreakProcess (current_process_handle))
    return;

  /* Last resort, suspend all threads manually.  */
  soft_interrupt_requested = 1;
}

#ifdef _WIN32_WCE
int
win32_error_to_fileio_error (DWORD err)
{
  switch (err)
    {
    case ERROR_BAD_PATHNAME:
    case ERROR_FILE_NOT_FOUND:
    case ERROR_INVALID_NAME:
    case ERROR_PATH_NOT_FOUND:
      return FILEIO_ENOENT;
    case ERROR_CRC:
    case ERROR_IO_DEVICE:
    case ERROR_OPEN_FAILED:
      return FILEIO_EIO;
    case ERROR_INVALID_HANDLE:
      return FILEIO_EBADF;
    case ERROR_ACCESS_DENIED:
    case ERROR_SHARING_VIOLATION:
      return FILEIO_EACCES;
    case ERROR_NOACCESS:
      return FILEIO_EFAULT;
    case ERROR_BUSY:
      return FILEIO_EBUSY;
    case ERROR_ALREADY_EXISTS:
    case ERROR_FILE_EXISTS:
      return FILEIO_EEXIST;
    case ERROR_BAD_DEVICE:
      return FILEIO_ENODEV;
    case ERROR_DIRECTORY:
      return FILEIO_ENOTDIR;
    case ERROR_FILENAME_EXCED_RANGE:
    case ERROR_INVALID_DATA:
    case ERROR_INVALID_PARAMETER:
    case ERROR_NEGATIVE_SEEK:
      return FILEIO_EINVAL;
    case ERROR_TOO_MANY_OPEN_FILES:
      return FILEIO_EMFILE;
    case ERROR_HANDLE_DISK_FULL:
    case ERROR_DISK_FULL:
      return FILEIO_ENOSPC;
    case ERROR_WRITE_PROTECT:
      return FILEIO_EROFS;
    case ERROR_NOT_SUPPORTED:
      return FILEIO_ENOSYS;
    }

  return FILEIO_EUNKNOWN;
}

static void
wince_hostio_last_error (char *buf)
{
  DWORD winerr = GetLastError ();
  int fileio_err = win32_error_to_fileio_error (winerr);
  sprintf (buf, "F-1,%x", fileio_err);
}
#endif

/* Write Windows OS Thread Information Block address.  */

static int
win32_get_tib_address (ptid_t ptid, CORE_ADDR *addr)
{
  win32_thread_info *th;
  th = thread_rec (ptid, 0);
  if (th == NULL)
    return 0;
  if (addr != NULL)
    *addr = th->thread_local_base;
  return 1;
}

static struct target_ops win32_target_ops = {
  win32_create_inferior,
  win32_attach,
  win32_kill,
  win32_detach,
  win32_mourn,
  win32_join,
  win32_thread_alive,
  win32_resume,
  win32_wait,
  win32_fetch_inferior_registers,
  win32_store_inferior_registers,
  NULL, /* prepare_to_access_memory */
  NULL, /* done_accessing_memory */
  win32_read_inferior_memory,
  win32_write_inferior_memory,
  NULL, /* lookup_symbols */
  win32_request_interrupt,
  NULL, /* read_auxv */
  win32_insert_point,
  win32_remove_point,
  win32_stopped_by_watchpoint,
  win32_stopped_data_address,
  NULL, /* read_offsets */
  NULL, /* get_tls_address */
  NULL, /* qxfer_spu */
#ifdef _WIN32_WCE
  wince_hostio_last_error,
#else
  hostio_last_error_from_errno,
#endif
  NULL, /* qxfer_osdata */
  NULL, /* qxfer_siginfo */
  NULL, /* supports_non_stop */
  NULL, /* async */
  NULL, /* start_non_stop */
  NULL, /* supports_multi_process */
  NULL, /* handle_monitor_command */
  NULL, /* core_of_thread */
  NULL, /* read_loadmap */
  NULL, /* process_qsupported */
  NULL, /* supports_tracepoints */
  NULL, /* read_pc */
  NULL, /* write_pc */
  NULL, /* thread_stopped */
  win32_get_tib_address
};

/* Initialize the Win32 backend.  */
void
initialize_low (void)
{
  set_target_ops (&win32_target_ops);
  if (the_low_target.breakpoint != NULL)
    set_breakpoint_data (the_low_target.breakpoint,
			 the_low_target.breakpoint_len);
  the_low_target.arch_setup ();
}
