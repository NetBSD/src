/* Target-vector operations for controlling windows child processes, for GDB.

   Copyright (C) 1995-2016 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions, A Red Hat Company.

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

/* Originally by Steve Chamberlain, sac@cygnus.com */

#include "defs.h"
#include "frame.h"		/* required by inferior.h */
#include "inferior.h"
#include "infrun.h"
#include "target.h"
#include "gdbcore.h"
#include "command.h"
#include "completer.h"
#include "regcache.h"
#include "top.h"
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <windows.h>
#include <imagehlp.h>
#include <psapi.h>
#ifdef __CYGWIN__
#include <wchar.h>
#include <sys/cygwin.h>
#include <cygwin/version.h>
#endif

#include "buildsym.h"
#include "filenames.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdb_bfd.h"
#include "gdb_obstack.h"
#include "gdbthread.h"
#include "gdbcmd.h"
#include <unistd.h>
#include "exec.h"
#include "solist.h"
#include "solib.h"
#include "xml-support.h"
#include "inttypes.h"

#include "i386-tdep.h"
#include "i387-tdep.h"

#include "windows-tdep.h"
#include "windows-nat.h"
#include "x86-nat.h"
#include "complaints.h"
#include "inf-child.h"

#define AdjustTokenPrivileges		dyn_AdjustTokenPrivileges
#define DebugActiveProcessStop		dyn_DebugActiveProcessStop
#define DebugBreakProcess		dyn_DebugBreakProcess
#define DebugSetProcessKillOnExit	dyn_DebugSetProcessKillOnExit
#define EnumProcessModules		dyn_EnumProcessModules
#define GetModuleInformation		dyn_GetModuleInformation
#define LookupPrivilegeValueA		dyn_LookupPrivilegeValueA
#define OpenProcessToken		dyn_OpenProcessToken
#define GetConsoleFontSize		dyn_GetConsoleFontSize
#define GetCurrentConsoleFont		dyn_GetCurrentConsoleFont

typedef BOOL WINAPI (AdjustTokenPrivileges_ftype) (HANDLE, BOOL,
						   PTOKEN_PRIVILEGES,
						   DWORD, PTOKEN_PRIVILEGES,
						   PDWORD);
static AdjustTokenPrivileges_ftype *AdjustTokenPrivileges;

typedef BOOL WINAPI (DebugActiveProcessStop_ftype) (DWORD);
static DebugActiveProcessStop_ftype *DebugActiveProcessStop;

typedef BOOL WINAPI (DebugBreakProcess_ftype) (HANDLE);
static DebugBreakProcess_ftype *DebugBreakProcess;

typedef BOOL WINAPI (DebugSetProcessKillOnExit_ftype) (BOOL);
static DebugSetProcessKillOnExit_ftype *DebugSetProcessKillOnExit;

typedef BOOL WINAPI (EnumProcessModules_ftype) (HANDLE, HMODULE *, DWORD,
						LPDWORD);
static EnumProcessModules_ftype *EnumProcessModules;

typedef BOOL WINAPI (GetModuleInformation_ftype) (HANDLE, HMODULE,
						  LPMODULEINFO, DWORD);
static GetModuleInformation_ftype *GetModuleInformation;

typedef BOOL WINAPI (LookupPrivilegeValueA_ftype) (LPCSTR, LPCSTR, PLUID);
static LookupPrivilegeValueA_ftype *LookupPrivilegeValueA;

typedef BOOL WINAPI (OpenProcessToken_ftype) (HANDLE, DWORD, PHANDLE);
static OpenProcessToken_ftype *OpenProcessToken;

typedef BOOL WINAPI (GetCurrentConsoleFont_ftype) (HANDLE, BOOL,
						   CONSOLE_FONT_INFO *);
static GetCurrentConsoleFont_ftype *GetCurrentConsoleFont;

typedef COORD WINAPI (GetConsoleFontSize_ftype) (HANDLE, DWORD);
static GetConsoleFontSize_ftype *GetConsoleFontSize;

#undef STARTUPINFO
#undef CreateProcess
#undef GetModuleFileNameEx

#ifndef __CYGWIN__
# define __PMAX	(MAX_PATH + 1)
  typedef DWORD WINAPI (GetModuleFileNameEx_ftype) (HANDLE, HMODULE, LPSTR, DWORD);
  static GetModuleFileNameEx_ftype *GetModuleFileNameEx;
# define STARTUPINFO STARTUPINFOA
# define CreateProcess CreateProcessA
# define GetModuleFileNameEx_name "GetModuleFileNameExA"
# define bad_GetModuleFileNameEx bad_GetModuleFileNameExA
#else
# define __PMAX	PATH_MAX
/* The starting and ending address of the cygwin1.dll text segment.  */
  static CORE_ADDR cygwin_load_start;
  static CORE_ADDR cygwin_load_end;
#   define __USEWIDE
    typedef wchar_t cygwin_buf_t;
    typedef DWORD WINAPI (GetModuleFileNameEx_ftype) (HANDLE, HMODULE,
						      LPWSTR, DWORD);
    static GetModuleFileNameEx_ftype *GetModuleFileNameEx;
#   define STARTUPINFO STARTUPINFOW
#   define CreateProcess CreateProcessW
#   define GetModuleFileNameEx_name "GetModuleFileNameExW"
#   define bad_GetModuleFileNameEx bad_GetModuleFileNameExW
#endif

static int have_saved_context;	/* True if we've saved context from a
				   cygwin signal.  */
static CONTEXT saved_context;	/* Containes the saved context from a
				   cygwin signal.  */

/* If we're not using the old Cygwin header file set, define the
   following which never should have been in the generic Win32 API
   headers in the first place since they were our own invention...  */
#ifndef _GNU_H_WINDOWS_H
enum
  {
    FLAG_TRACE_BIT = 0x100,
  };
#endif

#ifndef CONTEXT_EXTENDED_REGISTERS
/* This macro is only defined on ia32.  It only makes sense on this target,
   so define it as zero if not already defined.  */
#define CONTEXT_EXTENDED_REGISTERS 0
#endif

#define CONTEXT_DEBUGGER_DR CONTEXT_FULL | CONTEXT_FLOATING_POINT \
        | CONTEXT_SEGMENTS | CONTEXT_DEBUG_REGISTERS \
        | CONTEXT_EXTENDED_REGISTERS

static uintptr_t dr[8];
static int debug_registers_changed;
static int debug_registers_used;

static int windows_initialization_done;
#define DR6_CLEAR_VALUE 0xffff0ff0

/* The string sent by cygwin when it processes a signal.
   FIXME: This should be in a cygwin include file.  */
#ifndef _CYGWIN_SIGNAL_STRING
#define _CYGWIN_SIGNAL_STRING "cYgSiGw00f"
#endif

#define CHECK(x)	check (x, __FILE__,__LINE__)
#define DEBUG_EXEC(x)	if (debug_exec)		printf_unfiltered x
#define DEBUG_EVENTS(x)	if (debug_events)	printf_unfiltered x
#define DEBUG_MEM(x)	if (debug_memory)	printf_unfiltered x
#define DEBUG_EXCEPT(x)	if (debug_exceptions)	printf_unfiltered x

static void windows_interrupt (struct target_ops *self, ptid_t);
static int windows_thread_alive (struct target_ops *, ptid_t);
static void windows_kill_inferior (struct target_ops *);

static void cygwin_set_dr (int i, CORE_ADDR addr);
static void cygwin_set_dr7 (unsigned long val);
static CORE_ADDR cygwin_get_dr (int i);
static unsigned long cygwin_get_dr6 (void);
static unsigned long cygwin_get_dr7 (void);

static enum gdb_signal last_sig = GDB_SIGNAL_0;
/* Set if a signal was received from the debugged process.  */

/* Thread information structure used to track information that is
   not available in gdb's thread structure.  */
typedef struct windows_thread_info_struct
  {
    struct windows_thread_info_struct *next;
    DWORD id;
    HANDLE h;
    CORE_ADDR thread_local_base;
    char *name;
    int suspended;
    int reload_context;
    CONTEXT context;
    STACKFRAME sf;
  }
windows_thread_info;

static windows_thread_info thread_head;

/* The process and thread handles for the above context.  */

static DEBUG_EVENT current_event;	/* The current debug event from
					   WaitForDebugEvent */
static HANDLE current_process_handle;	/* Currently executing process */
static windows_thread_info *current_thread;	/* Info on currently selected thread */
static DWORD main_thread_id;		/* Thread ID of the main thread */

/* Counts of things.  */
static int exception_count = 0;
static int event_count = 0;
static int saw_create;
static int open_process_used = 0;

/* User options.  */
static int new_console = 0;
#ifdef __CYGWIN__
static int cygwin_exceptions = 0;
#endif
static int new_group = 1;
static int debug_exec = 0;		/* show execution */
static int debug_events = 0;		/* show events from kernel */
static int debug_memory = 0;		/* show target memory accesses */
static int debug_exceptions = 0;	/* show target exceptions */
static int useshell = 0;		/* use shell for subprocesses */

/* This vector maps GDB's idea of a register's number into an offset
   in the windows exception context vector.

   It also contains the bit mask needed to load the register in question.

   The contents of this table can only be computed by the units
   that provide CPU-specific support for Windows native debugging.
   These units should set the table by calling
   windows_set_context_register_offsets.

   One day we could read a reg, we could inspect the context we
   already have loaded, if it doesn't have the bit set that we need,
   we read that set of registers in using GetThreadContext.  If the
   context already contains what we need, we just unpack it.  Then to
   write a register, first we have to ensure that the context contains
   the other regs of the group, and then we copy the info in and set
   out bit.  */

static const int *mappings;

/* The function to use in order to determine whether a register is
   a segment register or not.  */
static segment_register_p_ftype *segment_register_p;

/* This vector maps the target's idea of an exception (extracted
   from the DEBUG_EVENT structure) to GDB's idea.  */

struct xlate_exception
  {
    int them;
    enum gdb_signal us;
  };

static const struct xlate_exception
  xlate[] =
{
  {EXCEPTION_ACCESS_VIOLATION, GDB_SIGNAL_SEGV},
  {STATUS_STACK_OVERFLOW, GDB_SIGNAL_SEGV},
  {EXCEPTION_BREAKPOINT, GDB_SIGNAL_TRAP},
  {DBG_CONTROL_C, GDB_SIGNAL_INT},
  {EXCEPTION_SINGLE_STEP, GDB_SIGNAL_TRAP},
  {STATUS_FLOAT_DIVIDE_BY_ZERO, GDB_SIGNAL_FPE},
  {-1, GDB_SIGNAL_UNKNOWN}};

/* Set the MAPPINGS static global to OFFSETS.
   See the description of MAPPINGS for more details.  */

void
windows_set_context_register_offsets (const int *offsets)
{
  mappings = offsets;
}

/* See windows-nat.h.  */

void
windows_set_segment_register_p (segment_register_p_ftype *fun)
{
  segment_register_p = fun;
}

static void
check (BOOL ok, const char *file, int line)
{
  if (!ok)
    printf_filtered ("error return %s:%d was %u\n", file, line,
		     (unsigned) GetLastError ());
}

/* Find a thread record given a thread id.  If GET_CONTEXT is not 0,
   then also retrieve the context for this thread.  If GET_CONTEXT is
   negative, then don't suspend the thread.  */
static windows_thread_info *
thread_rec (DWORD id, int get_context)
{
  windows_thread_info *th;

  for (th = &thread_head; (th = th->next) != NULL;)
    if (th->id == id)
      {
	if (!th->suspended && get_context)
	  {
	    if (get_context > 0 && id != current_event.dwThreadId)
	      {
		if (SuspendThread (th->h) == (DWORD) -1)
		  {
		    DWORD err = GetLastError ();

		    /* We get Access Denied (5) when trying to suspend
		       threads that Windows started on behalf of the
		       debuggee, usually when those threads are just
		       about to exit.
		       We can get Invalid Handle (6) if the main thread
		       has exited.  */
		    if (err != ERROR_INVALID_HANDLE
			&& err != ERROR_ACCESS_DENIED)
		      warning (_("SuspendThread (tid=0x%x) failed."
				 " (winerr %u)"),
			       (unsigned) id, (unsigned) err);
		    th->suspended = -1;
		  }
		else
		  th->suspended = 1;
	      }
	    else if (get_context < 0)
	      th->suspended = -1;
	    th->reload_context = 1;
	  }
	return th;
      }

  return NULL;
}

/* Add a thread to the thread list.  */
static windows_thread_info *
windows_add_thread (ptid_t ptid, HANDLE h, void *tlb)
{
  windows_thread_info *th;
  DWORD id;

  gdb_assert (ptid_get_tid (ptid) != 0);

  id = ptid_get_tid (ptid);

  if ((th = thread_rec (id, FALSE)))
    return th;

  th = XCNEW (windows_thread_info);
  th->id = id;
  th->h = h;
  th->thread_local_base = (CORE_ADDR) (uintptr_t) tlb;
  th->next = thread_head.next;
  thread_head.next = th;
  add_thread (ptid);
  /* Set the debug registers for the new thread if they are used.  */
  if (debug_registers_used)
    {
      /* Only change the value of the debug registers.  */
      th->context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
      CHECK (GetThreadContext (th->h, &th->context));
      th->context.Dr0 = dr[0];
      th->context.Dr1 = dr[1];
      th->context.Dr2 = dr[2];
      th->context.Dr3 = dr[3];
      th->context.Dr6 = DR6_CLEAR_VALUE;
      th->context.Dr7 = dr[7];
      CHECK (SetThreadContext (th->h, &th->context));
      th->context.ContextFlags = 0;
    }
  return th;
}

/* Clear out any old thread list and reintialize it to a
   pristine state.  */
static void
windows_init_thread_list (void)
{
  windows_thread_info *th = &thread_head;

  DEBUG_EVENTS (("gdb: windows_init_thread_list\n"));
  init_thread_list ();
  while (th->next != NULL)
    {
      windows_thread_info *here = th->next;
      th->next = here->next;
      xfree (here);
    }
  thread_head.next = NULL;
}

/* Delete a thread from the list of threads.  */
static void
windows_delete_thread (ptid_t ptid, DWORD exit_code)
{
  windows_thread_info *th;
  DWORD id;

  gdb_assert (ptid_get_tid (ptid) != 0);

  id = ptid_get_tid (ptid);

  if (info_verbose)
    printf_unfiltered ("[Deleting %s]\n", target_pid_to_str (ptid));
  else if (print_thread_events && id != main_thread_id)
    printf_unfiltered (_("[%s exited with code %u]\n"),
		       target_pid_to_str (ptid), (unsigned) exit_code);
  delete_thread (ptid);

  for (th = &thread_head;
       th->next != NULL && th->next->id != id;
       th = th->next)
    continue;

  if (th->next != NULL)
    {
      windows_thread_info *here = th->next;
      th->next = here->next;
      xfree (here);
    }
}

static void
do_windows_fetch_inferior_registers (struct regcache *regcache, int r)
{
  char *context_offset = ((char *) &current_thread->context) + mappings[r];
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  long l;

  if (!current_thread)
    return;	/* Windows sometimes uses a non-existent thread id in its
		   events.  */

  if (current_thread->reload_context)
    {
#ifdef __CYGWIN__
      if (have_saved_context)
	{
	  /* Lie about where the program actually is stopped since
	     cygwin has informed us that we should consider the signal
	     to have occurred at another location which is stored in
	     "saved_context.  */
	  memcpy (&current_thread->context, &saved_context,
		  __COPY_CONTEXT_SIZE);
	  have_saved_context = 0;
	}
      else
#endif
	{
	  windows_thread_info *th = current_thread;
	  th->context.ContextFlags = CONTEXT_DEBUGGER_DR;
	  CHECK (GetThreadContext (th->h, &th->context));
	  /* Copy dr values from that thread.
	     But only if there were not modified since last stop.
	     PR gdb/2388 */
	  if (!debug_registers_changed)
	    {
	      dr[0] = th->context.Dr0;
	      dr[1] = th->context.Dr1;
	      dr[2] = th->context.Dr2;
	      dr[3] = th->context.Dr3;
	      dr[6] = th->context.Dr6;
	      dr[7] = th->context.Dr7;
	    }
	}
      current_thread->reload_context = 0;
    }

  if (r == I387_FISEG_REGNUM (tdep))
    {
      l = *((long *) context_offset) & 0xffff;
      regcache_raw_supply (regcache, r, (char *) &l);
    }
  else if (r == I387_FOP_REGNUM (tdep))
    {
      l = (*((long *) context_offset) >> 16) & ((1 << 11) - 1);
      regcache_raw_supply (regcache, r, (char *) &l);
    }
  else if (segment_register_p (r))
    {
      /* GDB treats segment registers as 32bit registers, but they are
	 in fact only 16 bits long.  Make sure we do not read extra
	 bits from our source buffer.  */
      l = *((long *) context_offset) & 0xffff;
      regcache_raw_supply (regcache, r, (char *) &l);
    }
  else if (r >= 0)
    regcache_raw_supply (regcache, r, context_offset);
  else
    {
      for (r = 0; r < gdbarch_num_regs (gdbarch); r++)
	do_windows_fetch_inferior_registers (regcache, r);
    }
}

static void
windows_fetch_inferior_registers (struct target_ops *ops,
				  struct regcache *regcache, int r)
{
  current_thread = thread_rec (ptid_get_tid (inferior_ptid), TRUE);
  /* Check if current_thread exists.  Windows sometimes uses a non-existent
     thread id in its events.  */
  if (current_thread)
    do_windows_fetch_inferior_registers (regcache, r);
}

static void
do_windows_store_inferior_registers (const struct regcache *regcache, int r)
{
  if (!current_thread)
    /* Windows sometimes uses a non-existent thread id in its events.  */;
  else if (r >= 0)
    regcache_raw_collect (regcache, r,
			  ((char *) &current_thread->context) + mappings[r]);
  else
    {
      for (r = 0; r < gdbarch_num_regs (get_regcache_arch (regcache)); r++)
	do_windows_store_inferior_registers (regcache, r);
    }
}

/* Store a new register value into the current thread context.  */
static void
windows_store_inferior_registers (struct target_ops *ops,
				  struct regcache *regcache, int r)
{
  current_thread = thread_rec (ptid_get_tid (inferior_ptid), TRUE);
  /* Check if current_thread exists.  Windows sometimes uses a non-existent
     thread id in its events.  */
  if (current_thread)
    do_windows_store_inferior_registers (regcache, r);
}

/* Encapsulate the information required in a call to
   symbol_file_add_args.  */
struct safe_symbol_file_add_args
{
  char *name;
  int from_tty;
  struct section_addr_info *addrs;
  int mainline;
  int flags;
  struct ui_file *err, *out;
  struct objfile *ret;
};

/* Maintain a linked list of "so" information.  */
struct lm_info
{
  LPVOID load_addr;
};

static struct so_list solib_start, *solib_end;

static struct so_list *
windows_make_so (const char *name, LPVOID load_addr)
{
  struct so_list *so;
  char *p;
#ifndef __CYGWIN__
  char buf[__PMAX];
  char cwd[__PMAX];
  WIN32_FIND_DATA w32_fd;
  HANDLE h = FindFirstFile(name, &w32_fd);

  if (h == INVALID_HANDLE_VALUE)
    strcpy (buf, name);
  else
    {
      FindClose (h);
      strcpy (buf, name);
      if (GetCurrentDirectory (MAX_PATH + 1, cwd))
	{
	  p = strrchr (buf, '\\');
	  if (p)
	    p[1] = '\0';
	  SetCurrentDirectory (buf);
	  GetFullPathName (w32_fd.cFileName, MAX_PATH, buf, &p);
	  SetCurrentDirectory (cwd);
	}
    }
  if (strcasecmp (buf, "ntdll.dll") == 0)
    {
      GetSystemDirectory (buf, sizeof (buf));
      strcat (buf, "\\ntdll.dll");
    }
#else
  cygwin_buf_t buf[__PMAX];

  buf[0] = 0;
  if (access (name, F_OK) != 0)
    {
      if (strcasecmp (name, "ntdll.dll") == 0)
#ifdef __USEWIDE
	{
	  GetSystemDirectoryW (buf, sizeof (buf) / sizeof (wchar_t));
	  wcscat (buf, L"\\ntdll.dll");
	}
#else
	{
	  GetSystemDirectoryA (buf, sizeof (buf) / sizeof (wchar_t));
	  strcat (buf, "\\ntdll.dll");
	}
#endif
    }
#endif
  so = XCNEW (struct so_list);
  so->lm_info = XNEW (struct lm_info);
  so->lm_info->load_addr = load_addr;
  strcpy (so->so_original_name, name);
#ifndef __CYGWIN__
  strcpy (so->so_name, buf);
#else
  if (buf[0])
    cygwin_conv_path (CCP_WIN_W_TO_POSIX, buf, so->so_name,
		      SO_NAME_MAX_PATH_SIZE);
  else
    {
      char *rname = realpath (name, NULL);
      if (rname && strlen (rname) < SO_NAME_MAX_PATH_SIZE)
	{
	  strcpy (so->so_name, rname);
	  free (rname);
	}
      else
	error (_("dll path too long"));
    }
  /* Record cygwin1.dll .text start/end.  */
  p = strchr (so->so_name, '\0') - (sizeof ("/cygwin1.dll") - 1);
  if (p >= so->so_name && strcasecmp (p, "/cygwin1.dll") == 0)
    {
      bfd *abfd;
      asection *text = NULL;
      CORE_ADDR text_vma;

      abfd = gdb_bfd_open (so->so_name, "pei-i386", -1);

      if (!abfd)
	return so;

      if (bfd_check_format (abfd, bfd_object))
	text = bfd_get_section_by_name (abfd, ".text");

      if (!text)
	{
	  gdb_bfd_unref (abfd);
	  return so;
	}

      /* The symbols in a dll are offset by 0x1000, which is the
	 offset from 0 of the first byte in an image - because of the
	 file header and the section alignment.  */
      cygwin_load_start = (CORE_ADDR) (uintptr_t) ((char *)
						   load_addr + 0x1000);
      cygwin_load_end = cygwin_load_start + bfd_section_size (abfd, text);

      gdb_bfd_unref (abfd);
    }
#endif

  return so;
}

static char *
get_image_name (HANDLE h, void *address, int unicode)
{
#ifdef __CYGWIN__
  static char buf[__PMAX];
#else
  static char buf[(2 * __PMAX) + 1];
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

  /* See if we could read the address of a string, and that the
     address isn't null.  */
  if (!ReadProcessMemory (h, address,  &address_ptr,
			  sizeof (address_ptr), &done)
      || done != sizeof (address_ptr) || !address_ptr)
    return NULL;

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
      wcstombs (buf, unicode_address, __PMAX);
#else
      WideCharToMultiByte (CP_ACP, 0, unicode_address, len, buf, sizeof buf,
			   0, 0);
#endif
    }

  return buf;
}

/* Handle a DLL load event, and return 1.

   This function assumes that this event did not occur during inferior
   initialization, where their event info may be incomplete (see
   do_initial_windows_stuff and windows_add_all_dlls for more info
   on how we handle DLL loading during that phase).  */

static int
handle_load_dll (void *dummy)
{
  LOAD_DLL_DEBUG_INFO *event = &current_event.u.LoadDll;
  char *dll_name;

  /* Try getting the DLL name via the lpImageName field of the event.
     Note that Microsoft documents this fields as strictly optional,
     in the sense that it might be NULL.  And the first DLL event in
     particular is explicitly documented as "likely not pass[ed]"
     (source: MSDN LOAD_DLL_DEBUG_INFO structure).  */
  dll_name = get_image_name (current_process_handle,
			     event->lpImageName, event->fUnicode);
  if (!dll_name)
    return 1;

  solib_end->next = windows_make_so (dll_name, event->lpBaseOfDll);
  solib_end = solib_end->next;

  DEBUG_EVENTS (("gdb: Loading dll \"%s\" at %s.\n", solib_end->so_name,
		 host_address_to_string (solib_end->lm_info->load_addr)));

  return 1;
}

static void
windows_free_so (struct so_list *so)
{
  if (so->lm_info)
    xfree (so->lm_info);
  xfree (so);
}

/* Handle a DLL unload event.
   Return 1 if successful, or zero otherwise.

   This function assumes that this event did not occur during inferior
   initialization, where their event info may be incomplete (see
   do_initial_windows_stuff and windows_add_all_dlls for more info
   on how we handle DLL loading during that phase).  */

static int
handle_unload_dll (void *dummy)
{
  LPVOID lpBaseOfDll = current_event.u.UnloadDll.lpBaseOfDll;
  struct so_list *so;

  for (so = &solib_start; so->next != NULL; so = so->next)
    if (so->next->lm_info->load_addr == lpBaseOfDll)
      {
	struct so_list *sodel = so->next;

	so->next = sodel->next;
	if (!so->next)
	  solib_end = so;
	DEBUG_EVENTS (("gdb: Unloading dll \"%s\".\n", sodel->so_name));

	windows_free_so (sodel);
	return 1;
      }

  /* We did not find any DLL that was previously loaded at this address,
     so register a complaint.  We do not report an error, because we have
     observed that this may be happening under some circumstances.  For
     instance, running 32bit applications on x64 Windows causes us to receive
     4 mysterious UNLOAD_DLL_DEBUG_EVENTs during the startup phase (these
     events are apparently caused by the WOW layer, the interface between
     32bit and 64bit worlds).  */
  complaint (&symfile_complaints, _("dll starting at %s not found."),
	     host_address_to_string (lpBaseOfDll));

  return 0;
}

/* Clear list of loaded DLLs.  */
static void
windows_clear_solib (void)
{
  solib_start.next = NULL;
  solib_end = &solib_start;
}

static void
signal_event_command (char *args, int from_tty)
{
  uintptr_t event_id = 0;
  char *endargs = NULL;

  if (args == NULL)
    error (_("signal-event requires an argument (integer event id)"));

  event_id = strtoumax (args, &endargs, 10);

  if ((errno == ERANGE) || (event_id == 0) || (event_id > UINTPTR_MAX) ||
      ((HANDLE) event_id == INVALID_HANDLE_VALUE))
    error (_("Failed to convert `%s' to event id"), args);

  SetEvent ((HANDLE) event_id);
  CloseHandle ((HANDLE) event_id);
}

/* Handle DEBUG_STRING output from child process.
   Cygwin prepends its messages with a "cygwin:".  Interpret this as
   a Cygwin signal.  Otherwise just print the string as a warning.  */
static int
handle_output_debug_string (struct target_waitstatus *ourstatus)
{
  char *s = NULL;
  int retval = 0;

  if (!target_read_string
	((CORE_ADDR) (uintptr_t) current_event.u.DebugString.lpDebugStringData,
	&s, 1024, 0)
      || !s || !*s)
    /* nothing to do */;
  else if (!startswith (s, _CYGWIN_SIGNAL_STRING))
    {
#ifdef __CYGWIN__
      if (!startswith (s, "cYg"))
#endif
	{
	  char *p = strchr (s, '\0');

	  if (p > s && *--p == '\n')
	    *p = '\0';
	  warning (("%s"), s);
	}
    }
#ifdef __CYGWIN__
  else
    {
      /* Got a cygwin signal marker.  A cygwin signal is followed by
	 the signal number itself and then optionally followed by the
	 thread id and address to saved context within the DLL.  If
	 these are supplied, then the given thread is assumed to have
	 issued the signal and the context from the thread is assumed
	 to be stored at the given address in the inferior.  Tell gdb
	 to treat this like a real signal.  */
      char *p;
      int sig = strtol (s + sizeof (_CYGWIN_SIGNAL_STRING) - 1, &p, 0);
      gdb_signal gotasig = gdb_signal_from_host (sig);

      ourstatus->value.sig = gotasig;
      if (gotasig)
	{
	  LPCVOID x;
	  SIZE_T n;

	  ourstatus->kind = TARGET_WAITKIND_STOPPED;
	  retval = strtoul (p, &p, 0);
	  if (!retval)
	    retval = main_thread_id;
	  else if ((x = (LPCVOID) (uintptr_t) strtoull (p, NULL, 0))
		   && ReadProcessMemory (current_process_handle, x,
					 &saved_context,
					 __COPY_CONTEXT_SIZE, &n)
		   && n == __COPY_CONTEXT_SIZE)
	    have_saved_context = 1;
	}
    }
#endif

  if (s)
    xfree (s);
  return retval;
}

static int
display_selector (HANDLE thread, DWORD sel)
{
  LDT_ENTRY info;
  if (GetThreadSelectorEntry (thread, sel, &info))
    {
      int base, limit;
      printf_filtered ("0x%03x: ", (unsigned) sel);
      if (!info.HighWord.Bits.Pres)
	{
	  puts_filtered ("Segment not present\n");
	  return 0;
	}
      base = (info.HighWord.Bits.BaseHi << 24) +
	     (info.HighWord.Bits.BaseMid << 16)
	     + info.BaseLow;
      limit = (info.HighWord.Bits.LimitHi << 16) + info.LimitLow;
      if (info.HighWord.Bits.Granularity)
	limit = (limit << 12) | 0xfff;
      printf_filtered ("base=0x%08x limit=0x%08x", base, limit);
      if (info.HighWord.Bits.Default_Big)
	puts_filtered(" 32-bit ");
      else
	puts_filtered(" 16-bit ");
      switch ((info.HighWord.Bits.Type & 0xf) >> 1)
	{
	case 0:
	  puts_filtered ("Data (Read-Only, Exp-up");
	  break;
	case 1:
	  puts_filtered ("Data (Read/Write, Exp-up");
	  break;
	case 2:
	  puts_filtered ("Unused segment (");
	  break;
	case 3:
	  puts_filtered ("Data (Read/Write, Exp-down");
	  break;
	case 4:
	  puts_filtered ("Code (Exec-Only, N.Conf");
	  break;
	case 5:
	  puts_filtered ("Code (Exec/Read, N.Conf");
	  break;
	case 6:
	  puts_filtered ("Code (Exec-Only, Conf");
	  break;
	case 7:
	  puts_filtered ("Code (Exec/Read, Conf");
	  break;
	default:
	  printf_filtered ("Unknown type 0x%x",info.HighWord.Bits.Type);
	}
      if ((info.HighWord.Bits.Type & 0x1) == 0)
	puts_filtered(", N.Acc");
      puts_filtered (")\n");
      if ((info.HighWord.Bits.Type & 0x10) == 0)
	puts_filtered("System selector ");
      printf_filtered ("Priviledge level = %d. ", info.HighWord.Bits.Dpl);
      if (info.HighWord.Bits.Granularity)
	puts_filtered ("Page granular.\n");
      else
	puts_filtered ("Byte granular.\n");
      return 1;
    }
  else
    {
      DWORD err = GetLastError ();
      if (err == ERROR_NOT_SUPPORTED)
	printf_filtered ("Function not supported\n");
      else
	printf_filtered ("Invalid selector 0x%x.\n", (unsigned) sel);
      return 0;
    }
}

static void
display_selectors (char * args, int from_tty)
{
  if (!current_thread)
    {
      puts_filtered ("Impossible to display selectors now.\n");
      return;
    }
  if (!args)
    {

      puts_filtered ("Selector $cs\n");
      display_selector (current_thread->h,
	current_thread->context.SegCs);
      puts_filtered ("Selector $ds\n");
      display_selector (current_thread->h,
	current_thread->context.SegDs);
      puts_filtered ("Selector $es\n");
      display_selector (current_thread->h,
	current_thread->context.SegEs);
      puts_filtered ("Selector $ss\n");
      display_selector (current_thread->h,
	current_thread->context.SegSs);
      puts_filtered ("Selector $fs\n");
      display_selector (current_thread->h,
	current_thread->context.SegFs);
      puts_filtered ("Selector $gs\n");
      display_selector (current_thread->h,
	current_thread->context.SegGs);
    }
  else
    {
      int sel;
      sel = parse_and_eval_long (args);
      printf_filtered ("Selector \"%s\"\n",args);
      display_selector (current_thread->h, sel);
    }
}

#define DEBUG_EXCEPTION_SIMPLE(x)       if (debug_exceptions) \
  printf_unfiltered ("gdb: Target exception %s at %s\n", x, \
    host_address_to_string (\
      current_event.u.Exception.ExceptionRecord.ExceptionAddress))

static int
handle_exception (struct target_waitstatus *ourstatus)
{
  DWORD code = current_event.u.Exception.ExceptionRecord.ExceptionCode;

  ourstatus->kind = TARGET_WAITKIND_STOPPED;

  /* Record the context of the current thread.  */
  thread_rec (current_event.dwThreadId, -1);

  switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_ACCESS_VIOLATION");
      ourstatus->value.sig = GDB_SIGNAL_SEGV;
#ifdef __CYGWIN__
      {
	/* See if the access violation happened within the cygwin DLL
	   itself.  Cygwin uses a kind of exception handling to deal
	   with passed-in invalid addresses.  gdb should not treat
	   these as real SEGVs since they will be silently handled by
	   cygwin.  A real SEGV will (theoretically) be caught by
	   cygwin later in the process and will be sent as a
	   cygwin-specific-signal.  So, ignore SEGVs if they show up
	   within the text segment of the DLL itself.  */
	const char *fn;
	CORE_ADDR addr = (CORE_ADDR) (uintptr_t)
	  current_event.u.Exception.ExceptionRecord.ExceptionAddress;

	if ((!cygwin_exceptions && (addr >= cygwin_load_start
				    && addr < cygwin_load_end))
	    || (find_pc_partial_function (addr, &fn, NULL, NULL)
		&& startswith (fn, "KERNEL32!IsBad")))
	  return 0;
      }
#endif
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
      DEBUG_EXCEPTION_SIMPLE ("EXCEPTION_BREAKPOINT");
      ourstatus->value.sig = GDB_SIGNAL_TRAP;
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
    default:
      /* Treat unhandled first chance exceptions specially.  */
      if (current_event.u.Exception.dwFirstChance)
	return 0;
      printf_unfiltered ("gdb: unknown target exception 0x%08x at %s\n",
	(unsigned) current_event.u.Exception.ExceptionRecord.ExceptionCode,
	host_address_to_string (
	  current_event.u.Exception.ExceptionRecord.ExceptionAddress));
      ourstatus->value.sig = GDB_SIGNAL_UNKNOWN;
      break;
    }
  exception_count++;
  last_sig = ourstatus->value.sig;
  return 1;
}

/* Resume thread specified by ID, or all artificially suspended
   threads, if we are continuing execution.  KILLED non-zero means we
   have killed the inferior, so we should ignore weird errors due to
   threads shutting down.  */
static BOOL
windows_continue (DWORD continue_status, int id, int killed)
{
  int i;
  windows_thread_info *th;
  BOOL res;

  DEBUG_EVENTS (("ContinueDebugEvent (cpid=%d, ctid=0x%x, %s);\n",
		  (unsigned) current_event.dwProcessId,
		  (unsigned) current_event.dwThreadId,
		  continue_status == DBG_CONTINUE ?
		  "DBG_CONTINUE" : "DBG_EXCEPTION_NOT_HANDLED"));

  for (th = &thread_head; (th = th->next) != NULL;)
    if ((id == -1 || id == (int) th->id)
	&& th->suspended)
      {
	if (debug_registers_changed)
	  {
	    th->context.ContextFlags |= CONTEXT_DEBUG_REGISTERS;
	    th->context.Dr0 = dr[0];
	    th->context.Dr1 = dr[1];
	    th->context.Dr2 = dr[2];
	    th->context.Dr3 = dr[3];
	    th->context.Dr6 = DR6_CLEAR_VALUE;
	    th->context.Dr7 = dr[7];
	  }
	if (th->context.ContextFlags)
	  {
	    DWORD ec = 0;

	    if (GetExitCodeThread (th->h, &ec)
		&& ec == STILL_ACTIVE)
	      {
		BOOL status = SetThreadContext (th->h, &th->context);

		if (!killed)
		  CHECK (status);
	      }
	    th->context.ContextFlags = 0;
	  }
	if (th->suspended > 0)
	  (void) ResumeThread (th->h);
	th->suspended = 0;
      }

  res = ContinueDebugEvent (current_event.dwProcessId,
			    current_event.dwThreadId,
			    continue_status);

  if (!res)
    error (_("Failed to resume program execution"
	     " (ContinueDebugEvent failed, error %u)"),
	   (unsigned int) GetLastError ());

  debug_registers_changed = 0;
  return res;
}

/* Called in pathological case where Windows fails to send a
   CREATE_PROCESS_DEBUG_EVENT after an attach.  */
static DWORD
fake_create_process (void)
{
  current_process_handle = OpenProcess (PROCESS_ALL_ACCESS, FALSE,
					current_event.dwProcessId);
  if (current_process_handle != NULL)
    open_process_used = 1;
  else
    {
      error (_("OpenProcess call failed, GetLastError = %u"),
       (unsigned) GetLastError ());
      /*  We can not debug anything in that case.  */
    }
  main_thread_id = current_event.dwThreadId;
  current_thread = windows_add_thread (
		     ptid_build (current_event.dwProcessId, 0,
				 current_event.dwThreadId),
		     current_event.u.CreateThread.hThread,
		     current_event.u.CreateThread.lpThreadLocalBase);
  return main_thread_id;
}

static void
windows_resume (struct target_ops *ops,
		ptid_t ptid, int step, enum gdb_signal sig)
{
  windows_thread_info *th;
  DWORD continue_status = DBG_CONTINUE;

  /* A specific PTID means `step only this thread id'.  */
  int resume_all = ptid_equal (ptid, minus_one_ptid);

  /* If we're continuing all threads, it's the current inferior that
     should be handled specially.  */
  if (resume_all)
    ptid = inferior_ptid;

  if (sig != GDB_SIGNAL_0)
    {
      if (current_event.dwDebugEventCode != EXCEPTION_DEBUG_EVENT)
	{
	  DEBUG_EXCEPT(("Cannot continue with signal %d here.\n",sig));
	}
      else if (sig == last_sig)
	continue_status = DBG_EXCEPTION_NOT_HANDLED;
      else
#if 0
/* This code does not seem to work, because
  the kernel does probably not consider changes in the ExceptionRecord
  structure when passing the exception to the inferior.
  Note that this seems possible in the exception handler itself.  */
	{
	  int i;
	  for (i = 0; xlate[i].them != -1; i++)
	    if (xlate[i].us == sig)
	      {
		current_event.u.Exception.ExceptionRecord.ExceptionCode
		  = xlate[i].them;
		continue_status = DBG_EXCEPTION_NOT_HANDLED;
		break;
	      }
	  if (continue_status == DBG_CONTINUE)
	    {
	      DEBUG_EXCEPT(("Cannot continue with signal %d.\n",sig));
	    }
	}
#endif
	DEBUG_EXCEPT(("Can only continue with received signal %d.\n",
	  last_sig));
    }

  last_sig = GDB_SIGNAL_0;

  DEBUG_EXEC (("gdb: windows_resume (pid=%d, tid=%ld, step=%d, sig=%d);\n",
	       ptid_get_pid (ptid), ptid_get_tid (ptid), step, sig));

  /* Get context for currently selected thread.  */
  th = thread_rec (ptid_get_tid (inferior_ptid), FALSE);
  if (th)
    {
      if (step)
	{
	  /* Single step by setting t bit.  */
	  struct regcache *regcache = get_current_regcache ();
	  struct gdbarch *gdbarch = get_regcache_arch (regcache);
	  windows_fetch_inferior_registers (ops, regcache,
					    gdbarch_ps_regnum (gdbarch));
	  th->context.EFlags |= FLAG_TRACE_BIT;
	}

      if (th->context.ContextFlags)
	{
	  if (debug_registers_changed)
	    {
	      th->context.Dr0 = dr[0];
	      th->context.Dr1 = dr[1];
	      th->context.Dr2 = dr[2];
	      th->context.Dr3 = dr[3];
	      th->context.Dr6 = DR6_CLEAR_VALUE;
	      th->context.Dr7 = dr[7];
	    }
	  CHECK (SetThreadContext (th->h, &th->context));
	  th->context.ContextFlags = 0;
	}
    }

  /* Allow continuing with the same signal that interrupted us.
     Otherwise complain.  */

  if (resume_all)
    windows_continue (continue_status, -1, 0);
  else
    windows_continue (continue_status, ptid_get_tid (ptid), 0);
}

/* Ctrl-C handler used when the inferior is not run in the same console.  The
   handler is in charge of interrupting the inferior using DebugBreakProcess.
   Note that this function is not available prior to Windows XP.  In this case
   we emit a warning.  */
static BOOL WINAPI
ctrl_c_handler (DWORD event_type)
{
  const int attach_flag = current_inferior ()->attach_flag;

  /* Only handle Ctrl-C and Ctrl-Break events.  Ignore others.  */
  if (event_type != CTRL_C_EVENT && event_type != CTRL_BREAK_EVENT)
    return FALSE;

  /* If the inferior and the debugger share the same console, do nothing as
     the inferior has also received the Ctrl-C event.  */
  if (!new_console && !attach_flag)
    return TRUE;

  if (!DebugBreakProcess (current_process_handle))
    warning (_("Could not interrupt program.  "
	       "Press Ctrl-c in the program console."));

  /* Return true to tell that Ctrl-C has been handled.  */
  return TRUE;
}

/* Get the next event from the child.  Returns a non-zero thread id if the event
   requires handling by WFI (or whatever).  */
static int
get_windows_debug_event (struct target_ops *ops,
			 int pid, struct target_waitstatus *ourstatus)
{
  BOOL debug_event;
  DWORD continue_status, event_code;
  windows_thread_info *th;
  static windows_thread_info dummy_thread_info;
  DWORD thread_id = 0;

  last_sig = GDB_SIGNAL_0;

  if (!(debug_event = WaitForDebugEvent (&current_event, 1000)))
    goto out;

  event_count++;
  continue_status = DBG_CONTINUE;

  event_code = current_event.dwDebugEventCode;
  ourstatus->kind = TARGET_WAITKIND_SPURIOUS;
  th = NULL;
  have_saved_context = 0;

  switch (event_code)
    {
    case CREATE_THREAD_DEBUG_EVENT:
      DEBUG_EVENTS (("gdb: kernel event for pid=%u tid=0x%x code=%s)\n",
		     (unsigned) current_event.dwProcessId,
		     (unsigned) current_event.dwThreadId,
		     "CREATE_THREAD_DEBUG_EVENT"));
      if (saw_create != 1)
	{
	  struct inferior *inf;
	  inf = find_inferior_pid (current_event.dwProcessId);
	  if (!saw_create && inf->attach_flag)
	    {
	      /* Kludge around a Windows bug where first event is a create
		 thread event.  Caused when attached process does not have
		 a main thread.  */
	      thread_id = fake_create_process ();
	      if (thread_id)
		saw_create++;
	    }
	  break;
	}
      /* Record the existence of this thread.  */
      thread_id = current_event.dwThreadId;
      th = windows_add_thread (ptid_build (current_event.dwProcessId, 0,
					 current_event.dwThreadId),
			     current_event.u.CreateThread.hThread,
			     current_event.u.CreateThread.lpThreadLocalBase);

      break;

    case EXIT_THREAD_DEBUG_EVENT:
      DEBUG_EVENTS (("gdb: kernel event for pid=%u tid=0x%x code=%s)\n",
		     (unsigned) current_event.dwProcessId,
		     (unsigned) current_event.dwThreadId,
		     "EXIT_THREAD_DEBUG_EVENT"));

      if (current_event.dwThreadId != main_thread_id)
	{
	  windows_delete_thread (ptid_build (current_event.dwProcessId, 0,
					     current_event.dwThreadId),
				 current_event.u.ExitThread.dwExitCode);
	  th = &dummy_thread_info;
	}
      break;

    case CREATE_PROCESS_DEBUG_EVENT:
      DEBUG_EVENTS (("gdb: kernel event for pid=%u tid=0x%x code=%s)\n",
		     (unsigned) current_event.dwProcessId,
		     (unsigned) current_event.dwThreadId,
		     "CREATE_PROCESS_DEBUG_EVENT"));
      CloseHandle (current_event.u.CreateProcessInfo.hFile);
      if (++saw_create != 1)
	break;

      current_process_handle = current_event.u.CreateProcessInfo.hProcess;
      if (main_thread_id)
	windows_delete_thread (ptid_build (current_event.dwProcessId, 0,
					   main_thread_id),
			       0);
      main_thread_id = current_event.dwThreadId;
      /* Add the main thread.  */
      th = windows_add_thread (ptid_build (current_event.dwProcessId, 0,
					   current_event.dwThreadId),
	     current_event.u.CreateProcessInfo.hThread,
	     current_event.u.CreateProcessInfo.lpThreadLocalBase);
      thread_id = current_event.dwThreadId;
      break;

    case EXIT_PROCESS_DEBUG_EVENT:
      DEBUG_EVENTS (("gdb: kernel event for pid=%u tid=0x%x code=%s)\n",
		     (unsigned) current_event.dwProcessId,
		     (unsigned) current_event.dwThreadId,
		     "EXIT_PROCESS_DEBUG_EVENT"));
      if (!windows_initialization_done)
	{
	  target_terminal_ours ();
	  target_mourn_inferior ();
	  error (_("During startup program exited with code 0x%x."),
		 (unsigned int) current_event.u.ExitProcess.dwExitCode);
	}
      else if (saw_create == 1)
	{
	  ourstatus->kind = TARGET_WAITKIND_EXITED;
	  ourstatus->value.integer = current_event.u.ExitProcess.dwExitCode;
	  thread_id = main_thread_id;
	}
      break;

    case LOAD_DLL_DEBUG_EVENT:
      DEBUG_EVENTS (("gdb: kernel event for pid=%u tid=0x%x code=%s)\n",
		     (unsigned) current_event.dwProcessId,
		     (unsigned) current_event.dwThreadId,
		     "LOAD_DLL_DEBUG_EVENT"));
      CloseHandle (current_event.u.LoadDll.hFile);
      if (saw_create != 1 || ! windows_initialization_done)
	break;
      catch_errors (handle_load_dll, NULL, (char *) "", RETURN_MASK_ALL);
      ourstatus->kind = TARGET_WAITKIND_LOADED;
      ourstatus->value.integer = 0;
      thread_id = main_thread_id;
      break;

    case UNLOAD_DLL_DEBUG_EVENT:
      DEBUG_EVENTS (("gdb: kernel event for pid=%u tid=0x%x code=%s)\n",
		     (unsigned) current_event.dwProcessId,
		     (unsigned) current_event.dwThreadId,
		     "UNLOAD_DLL_DEBUG_EVENT"));
      if (saw_create != 1 || ! windows_initialization_done)
	break;
      catch_errors (handle_unload_dll, NULL, (char *) "", RETURN_MASK_ALL);
      ourstatus->kind = TARGET_WAITKIND_LOADED;
      ourstatus->value.integer = 0;
      thread_id = main_thread_id;
      break;

    case EXCEPTION_DEBUG_EVENT:
      DEBUG_EVENTS (("gdb: kernel event for pid=%u tid=0x%x code=%s)\n",
		     (unsigned) current_event.dwProcessId,
		     (unsigned) current_event.dwThreadId,
		     "EXCEPTION_DEBUG_EVENT"));
      if (saw_create != 1)
	break;
      if (handle_exception (ourstatus))
	thread_id = current_event.dwThreadId;
      else
	continue_status = DBG_EXCEPTION_NOT_HANDLED;
      break;

    case OUTPUT_DEBUG_STRING_EVENT:	/* Message from the kernel.  */
      DEBUG_EVENTS (("gdb: kernel event for pid=%u tid=0x%x code=%s)\n",
		     (unsigned) current_event.dwProcessId,
		     (unsigned) current_event.dwThreadId,
		     "OUTPUT_DEBUG_STRING_EVENT"));
      if (saw_create != 1)
	break;
      thread_id = handle_output_debug_string (ourstatus);
      break;

    default:
      if (saw_create != 1)
	break;
      printf_unfiltered ("gdb: kernel event for pid=%u tid=0x%x\n",
			 (unsigned) current_event.dwProcessId,
			 (unsigned) current_event.dwThreadId);
      printf_unfiltered ("                 unknown event code %u\n",
			 (unsigned) current_event.dwDebugEventCode);
      break;
    }

  if (!thread_id || saw_create != 1)
    {
      CHECK (windows_continue (continue_status, -1, 0));
    }
  else
    {
      inferior_ptid = ptid_build (current_event.dwProcessId, 0,
				  thread_id);
      current_thread = th;
      if (!current_thread)
	current_thread = thread_rec (thread_id, TRUE);
    }

out:
  return thread_id;
}

/* Wait for interesting events to occur in the target process.  */
static ptid_t
windows_wait (struct target_ops *ops,
	      ptid_t ptid, struct target_waitstatus *ourstatus, int options)
{
  int pid = -1;

  target_terminal_ours ();

  /* We loop when we get a non-standard exception rather than return
     with a SPURIOUS because resume can try and step or modify things,
     which needs a current_thread->h.  But some of these exceptions mark
     the birth or death of threads, which mean that the current thread
     isn't necessarily what you think it is.  */

  while (1)
    {
      int retval;

      /* If the user presses Ctrl-c while the debugger is waiting
	 for an event, he expects the debugger to interrupt his program
	 and to get the prompt back.  There are two possible situations:

	   - The debugger and the program do not share the console, in
	     which case the Ctrl-c event only reached the debugger.
	     In that case, the ctrl_c handler will take care of interrupting
	     the inferior.  Note that this case is working starting with
	     Windows XP.  For Windows 2000, Ctrl-C should be pressed in the
	     inferior console.

	   - The debugger and the program share the same console, in which
	     case both debugger and inferior will receive the Ctrl-c event.
	     In that case the ctrl_c handler will ignore the event, as the
	     Ctrl-c event generated inside the inferior will trigger the
	     expected debug event.

	     FIXME: brobecker/2008-05-20: If the inferior receives the
	     signal first and the delay until GDB receives that signal
	     is sufficiently long, GDB can sometimes receive the SIGINT
	     after we have unblocked the CTRL+C handler.  This would
	     lead to the debugger stopping prematurely while handling
	     the new-thread event that comes with the handling of the SIGINT
	     inside the inferior, and then stop again immediately when
	     the user tries to resume the execution in the inferior.
	     This is a classic race that we should try to fix one day.  */
      SetConsoleCtrlHandler (&ctrl_c_handler, TRUE);
      retval = get_windows_debug_event (ops, pid, ourstatus);
      SetConsoleCtrlHandler (&ctrl_c_handler, FALSE);

      if (retval)
	return ptid_build (current_event.dwProcessId, 0, retval);
      else
	{
	  int detach = 0;

	  if (deprecated_ui_loop_hook != NULL)
	    detach = deprecated_ui_loop_hook (0);

	  if (detach)
	    windows_kill_inferior (ops);
	}
    }
}

/* Iterate over all DLLs currently mapped by our inferior, and
   add them to our list of solibs.  */

static void
windows_add_all_dlls (void)
{
  struct so_list *so;
  HMODULE dummy_hmodule;
  DWORD cb_needed;
  HMODULE *hmodules;
  int i;

  if (EnumProcessModules (current_process_handle, &dummy_hmodule,
			  sizeof (HMODULE), &cb_needed) == 0)
    return;

  if (cb_needed < 1)
    return;

  hmodules = (HMODULE *) alloca (cb_needed);
  if (EnumProcessModules (current_process_handle, hmodules,
			  cb_needed, &cb_needed) == 0)
    return;

  for (i = 1; i < (int) (cb_needed / sizeof (HMODULE)); i++)
    {
      MODULEINFO mi;
#ifdef __USEWIDE
      wchar_t dll_name[__PMAX];
      char name[__PMAX];
#else
      char dll_name[__PMAX];
      char *name;
#endif
      if (GetModuleInformation (current_process_handle, hmodules[i],
				&mi, sizeof (mi)) == 0)
	continue;
      if (GetModuleFileNameEx (current_process_handle, hmodules[i],
			       dll_name, sizeof (dll_name)) == 0)
	continue;
#ifdef __USEWIDE
      wcstombs (name, dll_name, __PMAX);
#else
      name = dll_name;
#endif

      solib_end->next = windows_make_so (name, mi.lpBaseOfDll);
      solib_end = solib_end->next;
    }
}

static void
do_initial_windows_stuff (struct target_ops *ops, DWORD pid, int attaching)
{
  int i;
  struct inferior *inf;
  struct thread_info *tp;

  last_sig = GDB_SIGNAL_0;
  event_count = 0;
  exception_count = 0;
  open_process_used = 0;
  debug_registers_changed = 0;
  debug_registers_used = 0;
  for (i = 0; i < sizeof (dr) / sizeof (dr[0]); i++)
    dr[i] = 0;
#ifdef __CYGWIN__
  cygwin_load_start = cygwin_load_end = 0;
#endif
  current_event.dwProcessId = pid;
  memset (&current_event, 0, sizeof (current_event));
  if (!target_is_pushed (ops))
    push_target (ops);
  disable_breakpoints_in_shlibs ();
  windows_clear_solib ();
  clear_proceed_status (0);
  init_wait_for_inferior ();

  inf = current_inferior ();
  inferior_appeared (inf, pid);
  inf->attach_flag = attaching;

  /* Make the new process the current inferior, so terminal handling
     can rely on it.  When attaching, we don't know about any thread
     id here, but that's OK --- nothing should be referencing the
     current thread until we report an event out of windows_wait.  */
  inferior_ptid = pid_to_ptid (pid);

  target_terminal_init ();
  target_terminal_inferior ();

  windows_initialization_done = 0;

  while (1)
    {
      struct target_waitstatus status;

      windows_wait (ops, minus_one_ptid, &status, 0);

      /* Note windows_wait returns TARGET_WAITKIND_SPURIOUS for thread
	 events.  */
      if (status.kind != TARGET_WAITKIND_LOADED
	  && status.kind != TARGET_WAITKIND_SPURIOUS)
	break;

      windows_resume (ops, minus_one_ptid, 0, GDB_SIGNAL_0);
    }

  /* Now that the inferior has been started and all DLLs have been mapped,
     we can iterate over all DLLs and load them in.

     We avoid doing it any earlier because, on certain versions of Windows,
     LOAD_DLL_DEBUG_EVENTs are sometimes not complete.  In particular,
     we have seen on Windows 8.1 that the ntdll.dll load event does not
     include the DLL name, preventing us from creating an associated SO.
     A possible explanation is that ntdll.dll might be mapped before
     the SO info gets created by the Windows system -- ntdll.dll is
     the first DLL to be reported via LOAD_DLL_DEBUG_EVENT and other DLLs
     do not seem to suffer from that problem.

     Rather than try to work around this sort of issue, it is much
     simpler to just ignore DLL load/unload events during the startup
     phase, and then process them all in one batch now.  */
  windows_add_all_dlls ();

  windows_initialization_done = 1;
  return;
}

/* Try to set or remove a user privilege to the current process.  Return -1
   if that fails, the previous setting of that privilege otherwise.

   This code is copied from the Cygwin source code and rearranged to allow
   dynamically loading of the needed symbols from advapi32 which is only
   available on NT/2K/XP.  */
static int
set_process_privilege (const char *privilege, BOOL enable)
{
  HANDLE token_hdl = NULL;
  LUID restore_priv;
  TOKEN_PRIVILEGES new_priv, orig_priv;
  int ret = -1;
  DWORD size;

  if (!OpenProcessToken (GetCurrentProcess (),
			 TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
			 &token_hdl))
    goto out;

  if (!LookupPrivilegeValueA (NULL, privilege, &restore_priv))
    goto out;

  new_priv.PrivilegeCount = 1;
  new_priv.Privileges[0].Luid = restore_priv;
  new_priv.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

  if (!AdjustTokenPrivileges (token_hdl, FALSE, &new_priv,
			      sizeof orig_priv, &orig_priv, &size))
    goto out;
#if 0
  /* Disabled, otherwise every `attach' in an unprivileged user session
     would raise the "Failed to get SE_DEBUG_NAME privilege" warning in
     windows_attach().  */
  /* AdjustTokenPrivileges returns TRUE even if the privilege could not
     be enabled.  GetLastError () returns an correct error code, though.  */
  if (enable && GetLastError () == ERROR_NOT_ALL_ASSIGNED)
    goto out;
#endif

  ret = orig_priv.Privileges[0].Attributes == SE_PRIVILEGE_ENABLED ? 1 : 0;

out:
  if (token_hdl)
    CloseHandle (token_hdl);

  return ret;
}

/* Attach to process PID, then initialize for debugging it.  */
static void
windows_attach (struct target_ops *ops, const char *args, int from_tty)
{
  BOOL ok;
  DWORD pid;

  pid = parse_pid_to_attach (args);

  if (set_process_privilege (SE_DEBUG_NAME, TRUE) < 0)
    {
      printf_unfiltered ("Warning: Failed to get SE_DEBUG_NAME privilege\n");
      printf_unfiltered ("This can cause attach to "
			 "fail on Windows NT/2K/XP\n");
    }

  windows_init_thread_list ();
  ok = DebugActiveProcess (pid);
  saw_create = 0;

#ifdef __CYGWIN__
  if (!ok)
    {
      /* Try fall back to Cygwin pid.  */
      pid = cygwin_internal (CW_CYGWIN_PID_TO_WINPID, pid);

      if (pid > 0)
	ok = DebugActiveProcess (pid);
  }
#endif

  if (!ok)
    error (_("Can't attach to process."));

  DebugSetProcessKillOnExit (FALSE);

  if (from_tty)
    {
      char *exec_file = (char *) get_exec_file (0);

      if (exec_file)
	printf_unfiltered ("Attaching to program `%s', %s\n", exec_file,
			   target_pid_to_str (pid_to_ptid (pid)));
      else
	printf_unfiltered ("Attaching to %s\n",
			   target_pid_to_str (pid_to_ptid (pid)));

      gdb_flush (gdb_stdout);
    }

  do_initial_windows_stuff (ops, pid, 1);
  target_terminal_ours ();
}

static void
windows_detach (struct target_ops *ops, const char *args, int from_tty)
{
  int detached = 1;

  ptid_t ptid = {-1};
  windows_resume (ops, ptid, 0, GDB_SIGNAL_0);

  if (!DebugActiveProcessStop (current_event.dwProcessId))
    {
      error (_("Can't detach process %u (error %u)"),
	     (unsigned) current_event.dwProcessId, (unsigned) GetLastError ());
      detached = 0;
    }
  DebugSetProcessKillOnExit (FALSE);

  if (detached && from_tty)
    {
      char *exec_file = get_exec_file (0);
      if (exec_file == 0)
	exec_file = "";
      printf_unfiltered ("Detaching from program: %s, Pid %u\n", exec_file,
			 (unsigned) current_event.dwProcessId);
      gdb_flush (gdb_stdout);
    }

  x86_cleanup_dregs ();
  inferior_ptid = null_ptid;
  detach_inferior (current_event.dwProcessId);

  inf_child_maybe_unpush_target (ops);
}

/* Try to determine the executable filename.

   EXE_NAME_RET is a pointer to a buffer whose size is EXE_NAME_MAX_LEN.

   Upon success, the filename is stored inside EXE_NAME_RET, and
   this function returns nonzero.

   Otherwise, this function returns zero and the contents of
   EXE_NAME_RET is undefined.  */

static int
windows_get_exec_module_filename (char *exe_name_ret, size_t exe_name_max_len)
{
  DWORD len;
  HMODULE dh_buf;
  DWORD cbNeeded;

  cbNeeded = 0;
  if (!EnumProcessModules (current_process_handle, &dh_buf,
			   sizeof (HMODULE), &cbNeeded) || !cbNeeded)
    return 0;

  /* We know the executable is always first in the list of modules,
     which we just fetched.  So no need to fetch more.  */

#ifdef __CYGWIN__
  {
    /* Cygwin prefers that the path be in /x/y/z format, so extract
       the filename into a temporary buffer first, and then convert it
       to POSIX format into the destination buffer.  */
    cygwin_buf_t *pathbuf = (cygwin_buf_t *) alloca (exe_name_max_len * sizeof (cygwin_buf_t));

    len = GetModuleFileNameEx (current_process_handle,
			       dh_buf, pathbuf, exe_name_max_len);
    if (len == 0)
      error (_("Error getting executable filename: %u."),
	     (unsigned) GetLastError ());
    if (cygwin_conv_path (CCP_WIN_W_TO_POSIX, pathbuf, exe_name_ret,
			  exe_name_max_len) < 0)
      error (_("Error converting executable filename to POSIX: %d."), errno);
  }
#else
  len = GetModuleFileNameEx (current_process_handle,
			     dh_buf, exe_name_ret, exe_name_max_len);
  if (len == 0)
    error (_("Error getting executable filename: %u."),
	   (unsigned) GetLastError ());
#endif

    return 1;	/* success */
}

/* The pid_to_exec_file target_ops method for this platform.  */

static char *
windows_pid_to_exec_file (struct target_ops *self, int pid)
{
  static char path[__PMAX];
#ifdef __CYGWIN__
  /* Try to find exe name as symlink target of /proc/<pid>/exe.  */
  int nchars;
  char procexe[sizeof ("/proc/4294967295/exe")];

  xsnprintf (procexe, sizeof (procexe), "/proc/%u/exe", pid);
  nchars = readlink (procexe, path, sizeof(path));
  if (nchars > 0 && nchars < sizeof (path))
    {
      path[nchars] = '\0';	/* Got it */
      return path;
    }
#endif

  /* If we get here then either Cygwin is hosed, this isn't a Cygwin version
     of gdb, or we're trying to debug a non-Cygwin windows executable.  */
  if (!windows_get_exec_module_filename (path, sizeof (path)))
    path[0] = '\0';

  return path;
}

/* Print status information about what we're accessing.  */

static void
windows_files_info (struct target_ops *ignore)
{
  struct inferior *inf = current_inferior ();

  printf_unfiltered ("\tUsing the running image of %s %s.\n",
		     inf->attach_flag ? "attached" : "child",
		     target_pid_to_str (inferior_ptid));
}

/* Modify CreateProcess parameters for use of a new separate console.
   Parameters are:
   *FLAGS: DWORD parameter for general process creation flags.
   *SI: STARTUPINFO structure, for which the console window size and
   console buffer size is filled in if GDB is running in a console.
   to create the new console.
   The size of the used font is not available on all versions of
   Windows OS.  Furthermore, the current font might not be the default
   font, but this is still better than before.
   If the windows and buffer sizes are computed,
   SI->DWFLAGS is changed so that this information is used
   by CreateProcess function.  */

static void
windows_set_console_info (STARTUPINFO *si, DWORD *flags)
{
  HANDLE hconsole = CreateFile ("CONOUT$", GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);

  if (hconsole != INVALID_HANDLE_VALUE)
    {
      CONSOLE_SCREEN_BUFFER_INFO sbinfo;
      COORD font_size;
      CONSOLE_FONT_INFO cfi;

      GetCurrentConsoleFont (hconsole, FALSE, &cfi);
      font_size = GetConsoleFontSize (hconsole, cfi.nFont);
      GetConsoleScreenBufferInfo(hconsole, &sbinfo);
      si->dwXSize = sbinfo.srWindow.Right - sbinfo.srWindow.Left + 1;
      si->dwYSize = sbinfo.srWindow.Bottom - sbinfo.srWindow.Top + 1;
      if (font_size.X)
	si->dwXSize *= font_size.X;
      else
	si->dwXSize *= 8;
      if (font_size.Y)
	si->dwYSize *= font_size.Y;
      else
	si->dwYSize *= 12;
      si->dwXCountChars = sbinfo.dwSize.X;
      si->dwYCountChars = sbinfo.dwSize.Y;
      si->dwFlags |= STARTF_USESIZE | STARTF_USECOUNTCHARS;
    }
  *flags |= CREATE_NEW_CONSOLE;
}

#ifndef __CYGWIN__
/* Function called by qsort to sort environment strings.  */

static int
envvar_cmp (const void *a, const void *b)
{
  const char **p = (const char **) a;
  const char **q = (const char **) b;
  return strcasecmp (*p, *q);
}
#endif

#ifdef __CYGWIN__
static void
clear_win32_environment (char **env)
{
  int i;
  size_t len;
  wchar_t *copy = NULL, *equalpos;

  for (i = 0; env[i] && *env[i]; i++)
    {
      len = mbstowcs (NULL, env[i], 0) + 1;
      copy = (wchar_t *) xrealloc (copy, len * sizeof (wchar_t));
      mbstowcs (copy, env[i], len);
      equalpos = wcschr (copy, L'=');
      if (equalpos)
        *equalpos = L'\0';
      SetEnvironmentVariableW (copy, NULL);
    }
  xfree (copy);
}
#endif

/* Start an inferior windows child process and sets inferior_ptid to its pid.
   EXEC_FILE is the file to run.
   ALLARGS is a string containing the arguments to the program.
   ENV is the environment vector to pass.  Errors reported with error().  */

static void
windows_create_inferior (struct target_ops *ops, char *exec_file,
		       char *allargs, char **in_env, int from_tty)
{
  STARTUPINFO si;
#ifdef __CYGWIN__
  cygwin_buf_t real_path[__PMAX];
  cygwin_buf_t shell[__PMAX]; /* Path to shell */
  const char *sh;
  cygwin_buf_t *toexec;
  cygwin_buf_t *cygallargs;
  cygwin_buf_t *args;
  char **old_env = NULL;
  PWCHAR w32_env;
  size_t len;
  int tty;
  int ostdin, ostdout, ostderr;
#else
  char real_path[__PMAX];
  char shell[__PMAX]; /* Path to shell */
  char *toexec;
  char *args;
  size_t args_len;
  HANDLE tty;
  char *w32env;
  char *temp;
  size_t envlen;
  int i;
  size_t envsize;
  char **env;
#endif
  PROCESS_INFORMATION pi;
  BOOL ret;
  DWORD flags = 0;
  const char *inferior_io_terminal = get_inferior_io_terminal ();

  if (!exec_file)
    error (_("No executable specified, use `target exec'."));

  memset (&si, 0, sizeof (si));
  si.cb = sizeof (si);

  if (new_group)
    flags |= CREATE_NEW_PROCESS_GROUP;

  if (new_console)
    windows_set_console_info (&si, &flags);

#ifdef __CYGWIN__
  if (!useshell)
    {
      flags |= DEBUG_ONLY_THIS_PROCESS;
      if (cygwin_conv_path (CCP_POSIX_TO_WIN_W, exec_file, real_path,
			    __PMAX * sizeof (cygwin_buf_t)) < 0)
	error (_("Error starting executable: %d"), errno);
      toexec = real_path;
#ifdef __USEWIDE
      len = mbstowcs (NULL, allargs, 0) + 1;
      if (len == (size_t) -1)
	error (_("Error starting executable: %d"), errno);
      cygallargs = (wchar_t *) alloca (len * sizeof (wchar_t));
      mbstowcs (cygallargs, allargs, len);
#else
      cygallargs = allargs;
#endif
    }
  else
    {
      sh = getenv ("SHELL");
      if (!sh)
	sh = "/bin/sh";
      if (cygwin_conv_path (CCP_POSIX_TO_WIN_W, sh, shell, __PMAX) < 0)
      	error (_("Error starting executable via shell: %d"), errno);
#ifdef __USEWIDE
      len = sizeof (L" -c 'exec  '") + mbstowcs (NULL, exec_file, 0)
	    + mbstowcs (NULL, allargs, 0) + 2;
      cygallargs = (wchar_t *) alloca (len * sizeof (wchar_t));
      swprintf (cygallargs, len, L" -c 'exec %s %s'", exec_file, allargs);
#else
      len = (sizeof (" -c 'exec  '") + strlen (exec_file)
	     + strlen (allargs) + 2);
      cygallargs = (char *) alloca (len);
      xsnprintf (cygallargs, len, " -c 'exec %s %s'", exec_file, allargs);
#endif
      toexec = shell;
      flags |= DEBUG_PROCESS;
    }

#ifdef __USEWIDE
  args = (cygwin_buf_t *) alloca ((wcslen (toexec) + wcslen (cygallargs) + 2)
				  * sizeof (wchar_t));
  wcscpy (args, toexec);
  wcscat (args, L" ");
  wcscat (args, cygallargs);
#else
  args = (cygwin_buf_t *) alloca (strlen (toexec) + strlen (cygallargs) + 2);
  strcpy (args, toexec);
  strcat (args, " ");
  strcat (args, cygallargs);
#endif

#ifdef CW_CVT_ENV_TO_WINENV
  /* First try to create a direct Win32 copy of the POSIX environment. */
  w32_env = (PWCHAR) cygwin_internal (CW_CVT_ENV_TO_WINENV, in_env);
  if (w32_env != (PWCHAR) -1)
    flags |= CREATE_UNICODE_ENVIRONMENT;
  else
    /* If that fails, fall back to old method tweaking GDB's environment. */
#endif
    {
      /* Reset all Win32 environment variables to avoid leftover on next run. */
      clear_win32_environment (environ);
      /* Prepare the environment vars for CreateProcess.  */
      old_env = environ;
      environ = in_env;
      cygwin_internal (CW_SYNC_WINENV);
      w32_env = NULL;
    }

  if (!inferior_io_terminal)
    tty = ostdin = ostdout = ostderr = -1;
  else
    {
      tty = open (inferior_io_terminal, O_RDWR | O_NOCTTY);
      if (tty < 0)
	{
	  print_sys_errmsg (inferior_io_terminal, errno);
	  ostdin = ostdout = ostderr = -1;
	}
      else
	{
	  ostdin = dup (0);
	  ostdout = dup (1);
	  ostderr = dup (2);
	  dup2 (tty, 0);
	  dup2 (tty, 1);
	  dup2 (tty, 2);
	}
    }

  windows_init_thread_list ();
  ret = CreateProcess (0,
		       args,	/* command line */
		       NULL,	/* Security */
		       NULL,	/* thread */
		       TRUE,	/* inherit handles */
		       flags,	/* start flags */
		       w32_env,	/* environment */
		       NULL,	/* current directory */
		       &si,
		       &pi);
  if (w32_env)
    /* Just free the Win32 environment, if it could be created. */
    free (w32_env);
  else
    {
      /* Reset all environment variables to avoid leftover on next run. */
      clear_win32_environment (in_env);
      /* Restore normal GDB environment variables.  */
      environ = old_env;
      cygwin_internal (CW_SYNC_WINENV);
    }

  if (tty >= 0)
    {
      close (tty);
      dup2 (ostdin, 0);
      dup2 (ostdout, 1);
      dup2 (ostderr, 2);
      close (ostdin);
      close (ostdout);
      close (ostderr);
    }
#else
  toexec = exec_file;
  /* Build the command line, a space-separated list of tokens where
     the first token is the name of the module to be executed.
     To avoid ambiguities introduced by spaces in the module name,
     we quote it.  */
  args_len = strlen (toexec) + 2 /* quotes */ + strlen (allargs) + 2;
  args = (char *) alloca (args_len);
  xsnprintf (args, args_len, "\"%s\" %s", toexec, allargs);

  flags |= DEBUG_ONLY_THIS_PROCESS;

  if (!inferior_io_terminal)
    tty = INVALID_HANDLE_VALUE;
  else
    {
      SECURITY_ATTRIBUTES sa;
      sa.nLength = sizeof(sa);
      sa.lpSecurityDescriptor = 0;
      sa.bInheritHandle = TRUE;
      tty = CreateFileA (inferior_io_terminal, GENERIC_READ | GENERIC_WRITE,
			 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if (tty == INVALID_HANDLE_VALUE)
	warning (_("Warning: Failed to open TTY %s, error %#x."),
		 inferior_io_terminal, (unsigned) GetLastError ());
      else
	{
	  si.hStdInput = tty;
	  si.hStdOutput = tty;
	  si.hStdError = tty;
	  si.dwFlags |= STARTF_USESTDHANDLES;
	}
    }

  /* CreateProcess takes the environment list as a null terminated set of
     strings (i.e. two nulls terminate the list).  */

  /* Get total size for env strings.  */
  for (envlen = 0, i = 0; in_env[i] && *in_env[i]; i++)
    envlen += strlen (in_env[i]) + 1;

  envsize = sizeof (in_env[0]) * (i + 1);
  env = (char **) alloca (envsize);
  memcpy (env, in_env, envsize);
  /* Windows programs expect the environment block to be sorted.  */
  qsort (env, i, sizeof (char *), envvar_cmp);

  w32env = (char *) alloca (envlen + 1);

  /* Copy env strings into new buffer.  */
  for (temp = w32env, i = 0; env[i] && *env[i]; i++)
    {
      strcpy (temp, env[i]);
      temp += strlen (temp) + 1;
    }

  /* Final nil string to terminate new env.  */
  *temp = 0;

  windows_init_thread_list ();
  ret = CreateProcessA (0,
			args,	/* command line */
			NULL,	/* Security */
			NULL,	/* thread */
			TRUE,	/* inherit handles */
			flags,	/* start flags */
			w32env,	/* environment */
			NULL,	/* current directory */
			&si,
			&pi);
  if (tty != INVALID_HANDLE_VALUE)
    CloseHandle (tty);
#endif

  if (!ret)
    error (_("Error creating process %s, (error %u)."),
	   exec_file, (unsigned) GetLastError ());

  CloseHandle (pi.hThread);
  CloseHandle (pi.hProcess);

  if (useshell && shell[0] != '\0')
    saw_create = -1;
  else
    saw_create = 0;

  do_initial_windows_stuff (ops, pi.dwProcessId, 0);

  /* windows_continue (DBG_CONTINUE, -1, 0); */
}

static void
windows_mourn_inferior (struct target_ops *ops)
{
  (void) windows_continue (DBG_CONTINUE, -1, 0);
  x86_cleanup_dregs();
  if (open_process_used)
    {
      CHECK (CloseHandle (current_process_handle));
      open_process_used = 0;
    }
  inf_child_mourn_inferior (ops);
}

/* Send a SIGINT to the process group.  This acts just like the user typed a
   ^C on the controlling terminal.  */

static void
windows_interrupt (struct target_ops *self, ptid_t ptid)
{
  DEBUG_EVENTS (("gdb: GenerateConsoleCtrlEvent (CTRLC_EVENT, 0)\n"));
  CHECK (GenerateConsoleCtrlEvent (CTRL_C_EVENT, current_event.dwProcessId));
  registers_changed ();		/* refresh register state */
}

/* Helper for windows_xfer_partial that handles memory transfers.
   Arguments are like target_xfer_partial.  */

static enum target_xfer_status
windows_xfer_memory (gdb_byte *readbuf, const gdb_byte *writebuf,
		     ULONGEST memaddr, ULONGEST len, ULONGEST *xfered_len)
{
  SIZE_T done = 0;
  BOOL success;
  DWORD lasterror = 0;

  if (writebuf != NULL)
    {
      DEBUG_MEM (("gdb: write target memory, %s bytes at %s\n",
		  pulongest (len), core_addr_to_string (memaddr)));
      success = WriteProcessMemory (current_process_handle,
				    (LPVOID) (uintptr_t) memaddr, writebuf,
				    len, &done);
      if (!success)
	lasterror = GetLastError ();
      FlushInstructionCache (current_process_handle,
			     (LPCVOID) (uintptr_t) memaddr, len);
    }
  else
    {
      DEBUG_MEM (("gdb: read target memory, %s bytes at %s\n",
		  pulongest (len), core_addr_to_string (memaddr)));
      success = ReadProcessMemory (current_process_handle,
				   (LPCVOID) (uintptr_t) memaddr, readbuf,
				   len, &done);
      if (!success)
	lasterror = GetLastError ();
    }
  *xfered_len = (ULONGEST) done;
  if (!success && lasterror == ERROR_PARTIAL_COPY && done > 0)
    return TARGET_XFER_OK;
  else
    return success ? TARGET_XFER_OK : TARGET_XFER_E_IO;
}

static void
windows_kill_inferior (struct target_ops *ops)
{
  CHECK (TerminateProcess (current_process_handle, 0));

  for (;;)
    {
      if (!windows_continue (DBG_CONTINUE, -1, 1))
	break;
      if (!WaitForDebugEvent (&current_event, INFINITE))
	break;
      if (current_event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
	break;
    }

  target_mourn_inferior ();	/* Or just windows_mourn_inferior?  */
}

static void
windows_close (struct target_ops *self)
{
  DEBUG_EVENTS (("gdb: windows_close, inferior_ptid=%d\n",
		ptid_get_pid (inferior_ptid)));
}

/* Convert pid to printable format.  */
static char *
windows_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  static char buf[80];

  if (ptid_get_tid (ptid) != 0)
    {
      snprintf (buf, sizeof (buf), "Thread %d.0x%lx",
		ptid_get_pid (ptid), ptid_get_tid (ptid));
      return buf;
    }

  return normal_pid_to_str (ptid);
}

static enum target_xfer_status
windows_xfer_shared_libraries (struct target_ops *ops,
			       enum target_object object, const char *annex,
			       gdb_byte *readbuf, const gdb_byte *writebuf,
			       ULONGEST offset, ULONGEST len,
			       ULONGEST *xfered_len)
{
  struct obstack obstack;
  const char *buf;
  LONGEST len_avail;
  struct so_list *so;

  if (writebuf)
    return TARGET_XFER_E_IO;

  obstack_init (&obstack);
  obstack_grow_str (&obstack, "<library-list>\n");
  for (so = solib_start.next; so; so = so->next)
    windows_xfer_shared_library (so->so_name, (CORE_ADDR)
				 (uintptr_t) so->lm_info->load_addr,
				 target_gdbarch (), &obstack);
  obstack_grow_str0 (&obstack, "</library-list>\n");

  buf = (const char *) obstack_finish (&obstack);
  len_avail = strlen (buf);
  if (offset >= len_avail)
    len= 0;
  else
    {
      if (len > len_avail - offset)
	len = len_avail - offset;
      memcpy (readbuf, buf + offset, len);
    }

  obstack_free (&obstack, NULL);
  *xfered_len = (ULONGEST) len;
  return len != 0 ? TARGET_XFER_OK : TARGET_XFER_EOF;
}

static enum target_xfer_status
windows_xfer_partial (struct target_ops *ops, enum target_object object,
		      const char *annex, gdb_byte *readbuf,
		      const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
		      ULONGEST *xfered_len)
{
  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
      return windows_xfer_memory (readbuf, writebuf, offset, len, xfered_len);

    case TARGET_OBJECT_LIBRARIES:
      return windows_xfer_shared_libraries (ops, object, annex, readbuf,
					    writebuf, offset, len, xfered_len);

    default:
      return ops->beneath->to_xfer_partial (ops->beneath, object, annex,
					    readbuf, writebuf, offset, len,
					    xfered_len);
    }
}

/* Provide thread local base, i.e. Thread Information Block address.
   Returns 1 if ptid is found and sets *ADDR to thread_local_base.  */

static int
windows_get_tib_address (struct target_ops *self,
			 ptid_t ptid, CORE_ADDR *addr)
{
  windows_thread_info *th;

  th = thread_rec (ptid_get_tid (ptid), 0);
  if (th == NULL)
    return 0;

  if (addr != NULL)
    *addr = th->thread_local_base;

  return 1;
}

static ptid_t
windows_get_ada_task_ptid (struct target_ops *self, long lwp, long thread)
{
  return ptid_build (ptid_get_pid (inferior_ptid), 0, lwp);
}

static struct target_ops *
windows_target (void)
{
  struct target_ops *t = inf_child_target ();

  t->to_close = windows_close;
  t->to_attach = windows_attach;
  t->to_attach_no_wait = 1;
  t->to_detach = windows_detach;
  t->to_resume = windows_resume;
  t->to_wait = windows_wait;
  t->to_fetch_registers = windows_fetch_inferior_registers;
  t->to_store_registers = windows_store_inferior_registers;
  t->to_xfer_partial = windows_xfer_partial;
  t->to_files_info = windows_files_info;
  t->to_kill = windows_kill_inferior;
  t->to_create_inferior = windows_create_inferior;
  t->to_mourn_inferior = windows_mourn_inferior;
  t->to_thread_alive = windows_thread_alive;
  t->to_pid_to_str = windows_pid_to_str;
  t->to_interrupt = windows_interrupt;
  t->to_pid_to_exec_file = windows_pid_to_exec_file;
  t->to_get_ada_task_ptid = windows_get_ada_task_ptid;
  t->to_get_tib_address = windows_get_tib_address;

  return t;
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_windows_nat;

void
_initialize_windows_nat (void)
{
  struct target_ops *t;

  t = windows_target ();

  x86_use_watchpoints (t);

  x86_dr_low.set_control = cygwin_set_dr7;
  x86_dr_low.set_addr = cygwin_set_dr;
  x86_dr_low.get_addr = cygwin_get_dr;
  x86_dr_low.get_status = cygwin_get_dr6;
  x86_dr_low.get_control = cygwin_get_dr7;

  /* x86_dr_low.debug_register_length field is set by
     calling x86_set_debug_register_length function
     in processor windows specific native file.  */

  add_target (t);

#ifdef __CYGWIN__
  cygwin_internal (CW_SET_DOS_FILE_WARNING, 0);
#endif

  add_com ("signal-event", class_run, signal_event_command, _("\
Signal a crashed process with event ID, to allow its debugging.\n\
This command is needed in support of setting up GDB as JIT debugger on \
MS-Windows.  The command should be invoked from the GDB command line using \
the '-ex' command-line option.  The ID of the event that blocks the \
crashed process will be supplied by the Windows JIT debugging mechanism."));

#ifdef __CYGWIN__
  add_setshow_boolean_cmd ("shell", class_support, &useshell, _("\
Set use of shell to start subprocess."), _("\
Show use of shell to start subprocess."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("cygwin-exceptions", class_support,
			   &cygwin_exceptions, _("\
Break when an exception is detected in the Cygwin DLL itself."), _("\
Show whether gdb breaks on exceptions in the Cygwin DLL itself."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);
#endif

  add_setshow_boolean_cmd ("new-console", class_support, &new_console, _("\
Set creation of new console when creating child process."), _("\
Show creation of new console when creating child process."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("new-group", class_support, &new_group, _("\
Set creation of new group when creating child process."), _("\
Show creation of new group when creating child process."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("debugexec", class_support, &debug_exec, _("\
Set whether to display execution in child process."), _("\
Show whether to display execution in child process."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("debugevents", class_support, &debug_events, _("\
Set whether to display kernel events in child process."), _("\
Show whether to display kernel events in child process."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("debugmemory", class_support, &debug_memory, _("\
Set whether to display memory accesses in child process."), _("\
Show whether to display memory accesses in child process."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("debugexceptions", class_support,
			   &debug_exceptions, _("\
Set whether to display kernel exceptions in child process."), _("\
Show whether to display kernel exceptions in child process."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  init_w32_command_list ();

  add_cmd ("selector", class_info, display_selectors,
	   _("Display selectors infos."),
	   &info_w32_cmdlist);
}

/* Hardware watchpoint support, adapted from go32-nat.c code.  */

/* Pass the address ADDR to the inferior in the I'th debug register.
   Here we just store the address in dr array, the registers will be
   actually set up when windows_continue is called.  */
static void
cygwin_set_dr (int i, CORE_ADDR addr)
{
  if (i < 0 || i > 3)
    internal_error (__FILE__, __LINE__,
		    _("Invalid register %d in cygwin_set_dr.\n"), i);
  dr[i] = addr;
  debug_registers_changed = 1;
  debug_registers_used = 1;
}

/* Pass the value VAL to the inferior in the DR7 debug control
   register.  Here we just store the address in D_REGS, the watchpoint
   will be actually set up in windows_wait.  */
static void
cygwin_set_dr7 (unsigned long val)
{
  dr[7] = (CORE_ADDR) val;
  debug_registers_changed = 1;
  debug_registers_used = 1;
}

/* Get the value of debug register I from the inferior.  */

static CORE_ADDR
cygwin_get_dr (int i)
{
  return dr[i];
}

/* Get the value of the DR6 debug status register from the inferior.
   Here we just return the value stored in dr[6]
   by the last call to thread_rec for current_event.dwThreadId id.  */
static unsigned long
cygwin_get_dr6 (void)
{
  return (unsigned long) dr[6];
}

/* Get the value of the DR7 debug status register from the inferior.
   Here we just return the value stored in dr[7] by the last call to
   thread_rec for current_event.dwThreadId id.  */

static unsigned long
cygwin_get_dr7 (void)
{
  return (unsigned long) dr[7];
}

/* Determine if the thread referenced by "ptid" is alive
   by "polling" it.  If WaitForSingleObject returns WAIT_OBJECT_0
   it means that the thread has died.  Otherwise it is assumed to be alive.  */
static int
windows_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  int tid;

  gdb_assert (ptid_get_tid (ptid) != 0);
  tid = ptid_get_tid (ptid);

  return WaitForSingleObject (thread_rec (tid, FALSE)->h, 0) == WAIT_OBJECT_0
    ? FALSE : TRUE;
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_check_for_gdb_ini;

void
_initialize_check_for_gdb_ini (void)
{
  char *homedir;
  if (inhibit_gdbinit)
    return;

  homedir = getenv ("HOME");
  if (homedir)
    {
      char *p;
      char *oldini = (char *) alloca (strlen (homedir) +
				      sizeof ("gdb.ini") + 1);
      strcpy (oldini, homedir);
      p = strchr (oldini, '\0');
      if (p > oldini && !IS_DIR_SEPARATOR (p[-1]))
	*p++ = '/';
      strcpy (p, "gdb.ini");
      if (access (oldini, 0) == 0)
	{
	  int len = strlen (oldini);
	  char *newini = (char *) alloca (len + 2);

	  xsnprintf (newini, len + 2, "%.*s.gdbinit",
		     (int) (len - (sizeof ("gdb.ini") - 1)), oldini);
	  warning (_("obsolete '%s' found. Rename to '%s'."), oldini, newini);
	}
    }
}

/* Define dummy functions which always return error for the rare cases where
   these functions could not be found.  */
static BOOL WINAPI
bad_DebugActiveProcessStop (DWORD w)
{
  return FALSE;
}
static BOOL WINAPI
bad_DebugBreakProcess (HANDLE w)
{
  return FALSE;
}
static BOOL WINAPI
bad_DebugSetProcessKillOnExit (BOOL w)
{
  return FALSE;
}
static BOOL WINAPI
bad_EnumProcessModules (HANDLE w, HMODULE *x, DWORD y, LPDWORD z)
{
  return FALSE;
}

#ifdef __USEWIDE
static DWORD WINAPI
bad_GetModuleFileNameExW (HANDLE w, HMODULE x, LPWSTR y, DWORD z)
{
  return 0;
}
#else
static DWORD WINAPI
bad_GetModuleFileNameExA (HANDLE w, HMODULE x, LPSTR y, DWORD z)
{
  return 0;
}
#endif

static BOOL WINAPI
bad_GetModuleInformation (HANDLE w, HMODULE x, LPMODULEINFO y, DWORD z)
{
  return FALSE;
}

static BOOL WINAPI
bad_OpenProcessToken (HANDLE w, DWORD x, PHANDLE y)
{
  return FALSE;
}

static BOOL WINAPI
bad_GetCurrentConsoleFont (HANDLE w, BOOL bMaxWindow, CONSOLE_FONT_INFO *f)
{
  f->nFont = 0;
  return 1;
}
static COORD WINAPI
bad_GetConsoleFontSize (HANDLE w, DWORD nFont)
{
  COORD size;
  size.X = 8;
  size.Y = 12;
  return size;
}
 
/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_loadable;

/* Load any functions which may not be available in ancient versions
   of Windows.  */

void
_initialize_loadable (void)
{
  HMODULE hm = NULL;

#define GPA(m, func)					\
  func = (func ## _ftype *) GetProcAddress (m, #func)

  hm = LoadLibrary ("kernel32.dll");
  if (hm)
    {
      GPA (hm, DebugActiveProcessStop);
      GPA (hm, DebugBreakProcess);
      GPA (hm, DebugSetProcessKillOnExit);
      GPA (hm, GetConsoleFontSize);
      GPA (hm, DebugActiveProcessStop);
      GPA (hm, GetCurrentConsoleFont);
    }

  /* Set variables to dummy versions of these processes if the function
     wasn't found in kernel32.dll.  */
  if (!DebugBreakProcess)
    DebugBreakProcess = bad_DebugBreakProcess;
  if (!DebugActiveProcessStop || !DebugSetProcessKillOnExit)
    {
      DebugActiveProcessStop = bad_DebugActiveProcessStop;
      DebugSetProcessKillOnExit = bad_DebugSetProcessKillOnExit;
    }
  if (!GetConsoleFontSize)
    GetConsoleFontSize = bad_GetConsoleFontSize;
  if (!GetCurrentConsoleFont)
    GetCurrentConsoleFont = bad_GetCurrentConsoleFont;

  /* Load optional functions used for retrieving filename information
     associated with the currently debugged process or its dlls.  */
  hm = LoadLibrary ("psapi.dll");
  if (hm)
    {
      GPA (hm, EnumProcessModules);
      GPA (hm, GetModuleInformation);
      GetModuleFileNameEx = (GetModuleFileNameEx_ftype *)
        GetProcAddress (hm, GetModuleFileNameEx_name);
    }

  if (!EnumProcessModules || !GetModuleInformation || !GetModuleFileNameEx)
    {
      /* Set variables to dummy versions of these processes if the function
	 wasn't found in psapi.dll.  */
      EnumProcessModules = bad_EnumProcessModules;
      GetModuleInformation = bad_GetModuleInformation;
      GetModuleFileNameEx = bad_GetModuleFileNameEx;
      /* This will probably fail on Windows 9x/Me.  Let the user know
	 that we're missing some functionality.  */
      warning(_("\
cannot automatically find executable file or library to read symbols.\n\
Use \"file\" or \"dll\" command to load executable/libraries directly."));
    }

  hm = LoadLibrary ("advapi32.dll");
  if (hm)
    {
      GPA (hm, OpenProcessToken);
      GPA (hm, LookupPrivilegeValueA);
      GPA (hm, AdjustTokenPrivileges);
      /* Only need to set one of these since if OpenProcessToken fails nothing
	 else is needed.  */
      if (!OpenProcessToken || !LookupPrivilegeValueA
	  || !AdjustTokenPrivileges)
	OpenProcessToken = bad_OpenProcessToken;
    }

#undef GPA
}
