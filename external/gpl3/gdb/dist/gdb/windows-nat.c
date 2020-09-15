/* Target-vector operations for controlling windows child processes, for GDB.

   Copyright (C) 1995-2020 Free Software Foundation, Inc.

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
#include <algorithm>
#include <vector>

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
#include "gdbsupport/gdb_tilde_expand.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/gdb_wait.h"
#include "nat/windows-nat.h"

using namespace windows_nat;

#define AdjustTokenPrivileges		dyn_AdjustTokenPrivileges
#define DebugActiveProcessStop		dyn_DebugActiveProcessStop
#define DebugBreakProcess		dyn_DebugBreakProcess
#define DebugSetProcessKillOnExit	dyn_DebugSetProcessKillOnExit
#define EnumProcessModules		dyn_EnumProcessModules
#define EnumProcessModulesEx		dyn_EnumProcessModulesEx
#define GetModuleInformation		dyn_GetModuleInformation
#define LookupPrivilegeValueA		dyn_LookupPrivilegeValueA
#define OpenProcessToken		dyn_OpenProcessToken
#define GetConsoleFontSize		dyn_GetConsoleFontSize
#define GetCurrentConsoleFont		dyn_GetCurrentConsoleFont
#define Wow64SuspendThread		dyn_Wow64SuspendThread
#define Wow64GetThreadContext		dyn_Wow64GetThreadContext
#define Wow64SetThreadContext		dyn_Wow64SetThreadContext
#define Wow64GetThreadSelectorEntry	dyn_Wow64GetThreadSelectorEntry

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

#ifdef __x86_64__
typedef BOOL WINAPI (EnumProcessModulesEx_ftype) (HANDLE, HMODULE *, DWORD,
						  LPDWORD, DWORD);
static EnumProcessModulesEx_ftype *EnumProcessModulesEx;
#endif

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

#ifdef __x86_64__
typedef DWORD WINAPI (Wow64SuspendThread_ftype) (HANDLE);
static Wow64SuspendThread_ftype *Wow64SuspendThread;

typedef BOOL WINAPI (Wow64GetThreadContext_ftype) (HANDLE, PWOW64_CONTEXT);
static Wow64GetThreadContext_ftype *Wow64GetThreadContext;

typedef BOOL WINAPI (Wow64SetThreadContext_ftype) (HANDLE,
						   const WOW64_CONTEXT *);
static Wow64SetThreadContext_ftype *Wow64SetThreadContext;

typedef BOOL WINAPI (Wow64GetThreadSelectorEntry_ftype) (HANDLE, DWORD,
							 PLDT_ENTRY);
static Wow64GetThreadSelectorEntry_ftype *Wow64GetThreadSelectorEntry;
#endif

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
#ifdef __CYGWIN__
static CONTEXT saved_context;	/* Contains the saved context from a
				   cygwin signal.  */
#endif

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
#define DEBUG_EXEC(x)	if (debug_exec)		debug_printf x
#define DEBUG_EVENTS(x)	if (debug_events)	debug_printf x
#define DEBUG_MEM(x)	if (debug_memory)	debug_printf x
#define DEBUG_EXCEPT(x)	if (debug_exceptions)	debug_printf x

static void cygwin_set_dr (int i, CORE_ADDR addr);
static void cygwin_set_dr7 (unsigned long val);
static CORE_ADDR cygwin_get_dr (int i);
static unsigned long cygwin_get_dr6 (void);
static unsigned long cygwin_get_dr7 (void);

static std::vector<windows_thread_info *> thread_list;

/* Counts of things.  */
static int saw_create;
static int open_process_used = 0;
#ifdef __x86_64__
static bool wow64_process = false;
#endif

/* User options.  */
static bool new_console = false;
#ifdef __CYGWIN__
static bool cygwin_exceptions = false;
#endif
static bool new_group = true;
static bool debug_exec = false;		/* show execution */
static bool debug_events = false;	/* show events from kernel */
static bool debug_memory = false;	/* show target memory accesses */
static bool debug_exceptions = false;	/* show target exceptions */
static bool useshell = false;		/* use shell for subprocesses */

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

/* See windows_nat_target::resume to understand why this is commented
   out.  */
#if 0
/* This vector maps the target's idea of an exception (extracted
   from the DEBUG_EVENT structure) to GDB's idea.  */

struct xlate_exception
  {
    DWORD them;
    enum gdb_signal us;
  };

static const struct xlate_exception xlate[] =
{
  {EXCEPTION_ACCESS_VIOLATION, GDB_SIGNAL_SEGV},
  {STATUS_STACK_OVERFLOW, GDB_SIGNAL_SEGV},
  {EXCEPTION_BREAKPOINT, GDB_SIGNAL_TRAP},
  {DBG_CONTROL_C, GDB_SIGNAL_INT},
  {EXCEPTION_SINGLE_STEP, GDB_SIGNAL_TRAP},
  {STATUS_FLOAT_DIVIDE_BY_ZERO, GDB_SIGNAL_FPE}
};

#endif /* 0 */

struct windows_nat_target final : public x86_nat_target<inf_child_target>
{
  void close () override;

  void attach (const char *, int) override;

  bool attach_no_wait () override
  { return true; }

  void detach (inferior *, int) override;

  void resume (ptid_t, int , enum gdb_signal) override;

  ptid_t wait (ptid_t, struct target_waitstatus *, int) override;

  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  bool stopped_by_sw_breakpoint () override
  {
    windows_thread_info *th
      = thread_rec (inferior_ptid, DONT_INVALIDATE_CONTEXT);
    return th->stopped_at_software_breakpoint;
  }

  bool supports_stopped_by_sw_breakpoint () override
  {
    return true;
  }

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  void files_info () override;

  void kill () override;

  void create_inferior (const char *, const std::string &,
			char **, int) override;

  void mourn_inferior () override;

  bool thread_alive (ptid_t ptid) override;

  std::string pid_to_str (ptid_t) override;

  void interrupt () override;

  char *pid_to_exec_file (int pid) override;

  ptid_t get_ada_task_ptid (long lwp, long thread) override;

  bool get_tib_address (ptid_t ptid, CORE_ADDR *addr) override;

  const char *thread_name (struct thread_info *) override;

  int get_windows_debug_event (int pid, struct target_waitstatus *ourstatus);

  void do_initial_windows_stuff (DWORD pid, bool attaching);
};

static windows_nat_target the_windows_nat_target;

/* Set the MAPPINGS static global to OFFSETS.
   See the description of MAPPINGS for more details.  */

static void
windows_set_context_register_offsets (const int *offsets)
{
  mappings = offsets;
}

/* Set the function that should be used by this module to determine
   whether a given register is a segment register or not.  */

static void
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

/* See nat/windows-nat.h.  */

windows_thread_info *
windows_nat::thread_rec (ptid_t ptid, thread_disposition_type disposition)
{
  for (windows_thread_info *th : thread_list)
    if (th->tid == ptid.lwp ())
      {
	if (!th->suspended)
	  {
	    switch (disposition)
	      {
	      case DONT_INVALIDATE_CONTEXT:
		/* Nothing.  */
		break;
	      case INVALIDATE_CONTEXT:
		if (ptid.lwp () != current_event.dwThreadId)
		  th->suspend ();
		th->reload_context = true;
		break;
	      case DONT_SUSPEND:
		th->reload_context = true;
		th->suspended = -1;
		break;
	      }
	  }
	return th;
      }

  return NULL;
}

/* Add a thread to the thread list.

   PTID is the ptid of the thread to be added.
   H is its Windows handle.
   TLB is its thread local base.
   MAIN_THREAD_P should be true if the thread to be added is
   the main thread, false otherwise.  */

static windows_thread_info *
windows_add_thread (ptid_t ptid, HANDLE h, void *tlb, bool main_thread_p)
{
  windows_thread_info *th;

  gdb_assert (ptid.lwp () != 0);

  if ((th = thread_rec (ptid, DONT_INVALIDATE_CONTEXT)))
    return th;

  CORE_ADDR base = (CORE_ADDR) (uintptr_t) tlb;
#ifdef __x86_64__
  /* For WOW64 processes, this is actually the pointer to the 64bit TIB,
     and the 32bit TIB is exactly 2 pages after it.  */
  if (wow64_process)
    base += 0x2000;
#endif
  th = new windows_thread_info (ptid.lwp (), h, base);
  thread_list.push_back (th);

  /* Add this new thread to the list of threads.

     To be consistent with what's done on other platforms, we add
     the main thread silently (in reality, this thread is really
     more of a process to the user than a thread).  */
  if (main_thread_p)
    add_thread_silent (&the_windows_nat_target, ptid);
  else
    add_thread (&the_windows_nat_target, ptid);

  /* Set the debug registers for the new thread if they are used.  */
  if (debug_registers_used)
    {
#ifdef __x86_64__
      if (wow64_process)
	{
	  /* Only change the value of the debug registers.  */
	  th->wow64_context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	  CHECK (Wow64GetThreadContext (th->h, &th->wow64_context));
	  th->wow64_context.Dr0 = dr[0];
	  th->wow64_context.Dr1 = dr[1];
	  th->wow64_context.Dr2 = dr[2];
	  th->wow64_context.Dr3 = dr[3];
	  th->wow64_context.Dr6 = DR6_CLEAR_VALUE;
	  th->wow64_context.Dr7 = dr[7];
	  CHECK (Wow64SetThreadContext (th->h, &th->wow64_context));
	  th->wow64_context.ContextFlags = 0;
	}
      else
#endif
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
    }
  return th;
}

/* Clear out any old thread list and reinitialize it to a
   pristine state.  */
static void
windows_init_thread_list (void)
{
  DEBUG_EVENTS (("gdb: windows_init_thread_list\n"));
  init_thread_list ();

  for (windows_thread_info *here : thread_list)
    delete here;

  thread_list.clear ();
}

/* Delete a thread from the list of threads.

   PTID is the ptid of the thread to be deleted.
   EXIT_CODE is the thread's exit code.
   MAIN_THREAD_P should be true if the thread to be deleted is
   the main thread, false otherwise.  */

static void
windows_delete_thread (ptid_t ptid, DWORD exit_code, bool main_thread_p)
{
  DWORD id;

  gdb_assert (ptid.lwp () != 0);

  id = ptid.lwp ();

  /* Emit a notification about the thread being deleted.

     Note that no notification was printed when the main thread
     was created, and thus, unless in verbose mode, we should be
     symmetrical, and avoid that notification for the main thread
     here as well.  */

  if (info_verbose)
    printf_unfiltered ("[Deleting %s]\n", target_pid_to_str (ptid).c_str ());
  else if (print_thread_events && !main_thread_p)
    printf_unfiltered (_("[%s exited with code %u]\n"),
		       target_pid_to_str (ptid).c_str (),
		       (unsigned) exit_code);

  delete_thread (find_thread_ptid (&the_windows_nat_target, ptid));

  auto iter = std::find_if (thread_list.begin (), thread_list.end (),
			    [=] (windows_thread_info *th)
			    {
			      return th->tid == id;
			    });

  if (iter != thread_list.end ())
    {
      delete *iter;
      thread_list.erase (iter);
    }
}

/* Fetches register number R from the given windows_thread_info,
   and supplies its value to the given regcache.

   This function assumes that R is non-negative.  A failed assertion
   is raised if that is not true.

   This function assumes that TH->RELOAD_CONTEXT is not set, meaning
   that the windows_thread_info has an up-to-date context.  A failed
   assertion is raised if that assumption is violated.  */

static void
windows_fetch_one_register (struct regcache *regcache,
			    windows_thread_info *th, int r)
{
  gdb_assert (r >= 0);
  gdb_assert (!th->reload_context);

  char *context_ptr = (char *) &th->context;
#ifdef __x86_64__
  if (wow64_process)
    context_ptr = (char *) &th->wow64_context;
#endif

  char *context_offset = context_ptr + mappings[r];
  struct gdbarch *gdbarch = regcache->arch ();
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  gdb_assert (!gdbarch_read_pc_p (gdbarch));
  gdb_assert (gdbarch_pc_regnum (gdbarch) >= 0);
  gdb_assert (!gdbarch_write_pc_p (gdbarch));

  if (r == I387_FISEG_REGNUM (tdep))
    {
      long l = *((long *) context_offset) & 0xffff;
      regcache->raw_supply (r, (char *) &l);
    }
  else if (r == I387_FOP_REGNUM (tdep))
    {
      long l = (*((long *) context_offset) >> 16) & ((1 << 11) - 1);
      regcache->raw_supply (r, (char *) &l);
    }
  else if (segment_register_p (r))
    {
      /* GDB treats segment registers as 32bit registers, but they are
	 in fact only 16 bits long.  Make sure we do not read extra
	 bits from our source buffer.  */
      long l = *((long *) context_offset) & 0xffff;
      regcache->raw_supply (r, (char *) &l);
    }
  else
    {
      if (th->stopped_at_software_breakpoint
	  && !th->pc_adjusted
	  && r == gdbarch_pc_regnum (gdbarch))
	{
	  int size = register_size (gdbarch, r);
	  if (size == 4)
	    {
	      uint32_t value;
	      memcpy (&value, context_offset, size);
	      value -= gdbarch_decr_pc_after_break (gdbarch);
	      memcpy (context_offset, &value, size);
	    }
	  else
	    {
	      gdb_assert (size == 8);
	      uint64_t value;
	      memcpy (&value, context_offset, size);
	      value -= gdbarch_decr_pc_after_break (gdbarch);
	      memcpy (context_offset, &value, size);
	    }
	  /* Make sure we only rewrite the PC a single time.  */
	  th->pc_adjusted = true;
	}
      regcache->raw_supply (r, context_offset);
    }
}

void
windows_nat_target::fetch_registers (struct regcache *regcache, int r)
{
  windows_thread_info *th = thread_rec (regcache->ptid (), INVALIDATE_CONTEXT);

  /* Check if TH exists.  Windows sometimes uses a non-existent
     thread id in its events.  */
  if (th == NULL)
    return;

  if (th->reload_context)
    {
#ifdef __CYGWIN__
      if (have_saved_context)
	{
	  /* Lie about where the program actually is stopped since
	     cygwin has informed us that we should consider the signal
	     to have occurred at another location which is stored in
	     "saved_context.  */
	  memcpy (&th->context, &saved_context,
		  __COPY_CONTEXT_SIZE);
	  have_saved_context = 0;
	}
      else
#endif
#ifdef __x86_64__
      if (wow64_process)
	{
	  th->wow64_context.ContextFlags = CONTEXT_DEBUGGER_DR;
	  CHECK (Wow64GetThreadContext (th->h, &th->wow64_context));
	  /* Copy dr values from that thread.
	     But only if there were not modified since last stop.
	     PR gdb/2388 */
	  if (!debug_registers_changed)
	    {
	      dr[0] = th->wow64_context.Dr0;
	      dr[1] = th->wow64_context.Dr1;
	      dr[2] = th->wow64_context.Dr2;
	      dr[3] = th->wow64_context.Dr3;
	      dr[6] = th->wow64_context.Dr6;
	      dr[7] = th->wow64_context.Dr7;
	    }
	}
      else
#endif
	{
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
      th->reload_context = false;
    }

  if (r < 0)
    for (r = 0; r < gdbarch_num_regs (regcache->arch()); r++)
      windows_fetch_one_register (regcache, th, r);
  else
    windows_fetch_one_register (regcache, th, r);
}

/* Collect the register number R from the given regcache, and store
   its value into the corresponding area of the given thread's context.

   This function assumes that R is non-negative.  A failed assertion
   assertion is raised if that is not true.  */

static void
windows_store_one_register (const struct regcache *regcache,
			    windows_thread_info *th, int r)
{
  gdb_assert (r >= 0);

  char *context_ptr = (char *) &th->context;
#ifdef __x86_64__
  if (wow64_process)
    context_ptr = (char *) &th->wow64_context;
#endif

  regcache->raw_collect (r, context_ptr + mappings[r]);
}

/* Store a new register value into the context of the thread tied to
   REGCACHE.  */

void
windows_nat_target::store_registers (struct regcache *regcache, int r)
{
  windows_thread_info *th = thread_rec (regcache->ptid (), INVALIDATE_CONTEXT);

  /* Check if TH exists.  Windows sometimes uses a non-existent
     thread id in its events.  */
  if (th == NULL)
    return;

  if (r < 0)
    for (r = 0; r < gdbarch_num_regs (regcache->arch ()); r++)
      windows_store_one_register (regcache, th, r);
  else
    windows_store_one_register (regcache, th, r);
}

/* Maintain a linked list of "so" information.  */
struct lm_info_windows : public lm_info_base
{
  LPVOID load_addr = 0;
  CORE_ADDR text_offset = 0;
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
  lm_info_windows *li = new lm_info_windows;
  so->lm_info = li;
  li->load_addr = load_addr;
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
	{
	  warning (_("dll path for \"%s\" too long or inaccessible"), name);
	  strcpy (so->so_name, so->so_original_name);
	}
    }
  /* Record cygwin1.dll .text start/end.  */
  p = strchr (so->so_name, '\0') - (sizeof ("/cygwin1.dll") - 1);
  if (p >= so->so_name && strcasecmp (p, "/cygwin1.dll") == 0)
    {
      asection *text = NULL;

      gdb_bfd_ref_ptr abfd (gdb_bfd_open (so->so_name, "pei-i386"));

      if (abfd == NULL)
	return so;

      if (bfd_check_format (abfd.get (), bfd_object))
	text = bfd_get_section_by_name (abfd.get (), ".text");

      if (!text)
	return so;

      /* The symbols in a dll are offset by 0x1000, which is the
	 offset from 0 of the first byte in an image - because of the
	 file header and the section alignment.  */
      cygwin_load_start = (CORE_ADDR) (uintptr_t) ((char *)
						   load_addr + 0x1000);
      cygwin_load_end = cygwin_load_start + bfd_section_size (text);
    }
#endif

  return so;
}

/* See nat/windows-nat.h.  */

void
windows_nat::handle_load_dll ()
{
  LOAD_DLL_DEBUG_INFO *event = &current_event.u.LoadDll;
  const char *dll_name;

  /* Try getting the DLL name via the lpImageName field of the event.
     Note that Microsoft documents this fields as strictly optional,
     in the sense that it might be NULL.  And the first DLL event in
     particular is explicitly documented as "likely not pass[ed]"
     (source: MSDN LOAD_DLL_DEBUG_INFO structure).  */
  dll_name = get_image_name (current_process_handle,
			     event->lpImageName, event->fUnicode);
  if (!dll_name)
    return;

  solib_end->next = windows_make_so (dll_name, event->lpBaseOfDll);
  solib_end = solib_end->next;

  lm_info_windows *li = (lm_info_windows *) solib_end->lm_info;

  DEBUG_EVENTS (("gdb: Loading dll \"%s\" at %s.\n", solib_end->so_name,
		 host_address_to_string (li->load_addr)));
}

static void
windows_free_so (struct so_list *so)
{
  lm_info_windows *li = (lm_info_windows *) so->lm_info;

  delete li;
  xfree (so);
}

/* See nat/windows-nat.h.  */

void
windows_nat::handle_unload_dll ()
{
  LPVOID lpBaseOfDll = current_event.u.UnloadDll.lpBaseOfDll;
  struct so_list *so;

  for (so = &solib_start; so->next != NULL; so = so->next)
    {
      lm_info_windows *li_next = (lm_info_windows *) so->next->lm_info;

      if (li_next->load_addr == lpBaseOfDll)
	{
	  struct so_list *sodel = so->next;

	  so->next = sodel->next;
	  if (!so->next)
	    solib_end = so;
	  DEBUG_EVENTS (("gdb: Unloading dll \"%s\".\n", sodel->so_name));

	  windows_free_so (sodel);
	  return;
	}
    }

  /* We did not find any DLL that was previously loaded at this address,
     so register a complaint.  We do not report an error, because we have
     observed that this may be happening under some circumstances.  For
     instance, running 32bit applications on x64 Windows causes us to receive
     4 mysterious UNLOAD_DLL_DEBUG_EVENTs during the startup phase (these
     events are apparently caused by the WOW layer, the interface between
     32bit and 64bit worlds).  */
  complaint (_("dll starting at %s not found."),
	     host_address_to_string (lpBaseOfDll));
}

/* Call FUNC wrapped in a TRY/CATCH that swallows all GDB
   exceptions.  */

static void
catch_errors (void (*func) ())
{
  try
    {
      func ();
    }
  catch (const gdb_exception &ex)
    {
      exception_print (gdb_stderr, ex);
    }
}

/* Clear list of loaded DLLs.  */
static void
windows_clear_solib (void)
{
  struct so_list *so;

  for (so = solib_start.next; so; so = solib_start.next)
    {
      solib_start.next = so->next;
      windows_free_so (so);
    }

  solib_end = &solib_start;
}

static void
signal_event_command (const char *args, int from_tty)
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

/* See nat/windows-nat.h.  */

int
windows_nat::handle_output_debug_string (struct target_waitstatus *ourstatus)
{
  int retval = 0;

  gdb::unique_xmalloc_ptr<char> s
    = (target_read_string
       ((CORE_ADDR) (uintptr_t) current_event.u.DebugString.lpDebugStringData,
	1024));
  if (s == nullptr || !*(s.get ()))
    /* nothing to do */;
  else if (!startswith (s.get (), _CYGWIN_SIGNAL_STRING))
    {
#ifdef __CYGWIN__
      if (!startswith (s.get (), "cYg"))
#endif
	{
	  char *p = strchr (s.get (), '\0');

	  if (p > s.get () && *--p == '\n')
	    *p = '\0';
	  warning (("%s"), s.get ());
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
      int sig = strtol (s.get () + sizeof (_CYGWIN_SIGNAL_STRING) - 1, &p, 0);
      gdb_signal gotasig = gdb_signal_from_host (sig);

      ourstatus->value.sig = gotasig;
      if (gotasig)
	{
	  LPCVOID x;
	  SIZE_T n;

	  ourstatus->kind = TARGET_WAITKIND_STOPPED;
	  retval = strtoul (p, &p, 0);
	  if (!retval)
	    retval = current_event.dwThreadId;
	  else if ((x = (LPCVOID) (uintptr_t) strtoull (p, NULL, 0))
		   && ReadProcessMemory (current_process_handle, x,
					 &saved_context,
					 __COPY_CONTEXT_SIZE, &n)
		   && n == __COPY_CONTEXT_SIZE)
	    have_saved_context = 1;
	}
    }
#endif

  return retval;
}

static int
display_selector (HANDLE thread, DWORD sel)
{
  LDT_ENTRY info;
  BOOL ret;
#ifdef __x86_64__
  if (wow64_process)
    ret = Wow64GetThreadSelectorEntry (thread, sel, &info);
  else
#endif
    ret = GetThreadSelectorEntry (thread, sel, &info);
  if (ret)
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
	  printf_filtered ("Unknown type 0x%lx",
			   (unsigned long) info.HighWord.Bits.Type);
	}
      if ((info.HighWord.Bits.Type & 0x1) == 0)
	puts_filtered(", N.Acc");
      puts_filtered (")\n");
      if ((info.HighWord.Bits.Type & 0x10) == 0)
	puts_filtered("System selector ");
      printf_filtered ("Priviledge level = %ld. ",
		       (unsigned long) info.HighWord.Bits.Dpl);
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
display_selectors (const char * args, int from_tty)
{
  if (inferior_ptid == null_ptid)
    {
      puts_filtered ("Impossible to display selectors now.\n");
      return;
    }

  windows_thread_info *current_windows_thread
    = thread_rec (inferior_ptid, DONT_INVALIDATE_CONTEXT);

  if (!args)
    {
#ifdef __x86_64__
      if (wow64_process)
	{
	  puts_filtered ("Selector $cs\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->wow64_context.SegCs);
	  puts_filtered ("Selector $ds\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->wow64_context.SegDs);
	  puts_filtered ("Selector $es\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->wow64_context.SegEs);
	  puts_filtered ("Selector $ss\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->wow64_context.SegSs);
	  puts_filtered ("Selector $fs\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->wow64_context.SegFs);
	  puts_filtered ("Selector $gs\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->wow64_context.SegGs);
	}
      else
#endif
	{
	  puts_filtered ("Selector $cs\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->context.SegCs);
	  puts_filtered ("Selector $ds\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->context.SegDs);
	  puts_filtered ("Selector $es\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->context.SegEs);
	  puts_filtered ("Selector $ss\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->context.SegSs);
	  puts_filtered ("Selector $fs\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->context.SegFs);
	  puts_filtered ("Selector $gs\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->context.SegGs);
	}
    }
  else
    {
      int sel;
      sel = parse_and_eval_long (args);
      printf_filtered ("Selector \"%s\"\n",args);
      display_selector (current_windows_thread->h, sel);
    }
}

/* See nat/windows-nat.h.  */

bool
windows_nat::handle_ms_vc_exception (const EXCEPTION_RECORD *rec)
{
  if (rec->NumberParameters >= 3
      && (rec->ExceptionInformation[0] & 0xffffffff) == 0x1000)
    {
      DWORD named_thread_id;
      windows_thread_info *named_thread;
      CORE_ADDR thread_name_target;

      thread_name_target = rec->ExceptionInformation[1];
      named_thread_id = (DWORD) (0xffffffff & rec->ExceptionInformation[2]);

      if (named_thread_id == (DWORD) -1)
	named_thread_id = current_event.dwThreadId;

      named_thread = thread_rec (ptid_t (current_event.dwProcessId,
					 named_thread_id, 0),
				 DONT_INVALIDATE_CONTEXT);
      if (named_thread != NULL)
	{
	  int thread_name_len;
	  gdb::unique_xmalloc_ptr<char> thread_name
	    = target_read_string (thread_name_target, 1025, &thread_name_len);
	  if (thread_name_len > 0)
	    {
	      thread_name.get ()[thread_name_len - 1] = '\0';
	      named_thread->name = std::move (thread_name);
	    }
	}

      return true;
    }

  return false;
}

/* See nat/windows-nat.h.  */

bool
windows_nat::handle_access_violation (const EXCEPTION_RECORD *rec)
{
#ifdef __CYGWIN__
  /* See if the access violation happened within the cygwin DLL
     itself.  Cygwin uses a kind of exception handling to deal with
     passed-in invalid addresses.  gdb should not treat these as real
     SEGVs since they will be silently handled by cygwin.  A real SEGV
     will (theoretically) be caught by cygwin later in the process and
     will be sent as a cygwin-specific-signal.  So, ignore SEGVs if
     they show up within the text segment of the DLL itself.  */
  const char *fn;
  CORE_ADDR addr = (CORE_ADDR) (uintptr_t) rec->ExceptionAddress;

  if ((!cygwin_exceptions && (addr >= cygwin_load_start
			      && addr < cygwin_load_end))
      || (find_pc_partial_function (addr, &fn, NULL, NULL)
	  && startswith (fn, "KERNEL32!IsBad")))
    return true;
#endif
  return false;
}

/* Resume thread specified by ID, or all artificially suspended
   threads, if we are continuing execution.  KILLED non-zero means we
   have killed the inferior, so we should ignore weird errors due to
   threads shutting down.  */
static BOOL
windows_continue (DWORD continue_status, int id, int killed)
{
  BOOL res;

  desired_stop_thread_id = id;

  if (matching_pending_stop (debug_events))
    return TRUE;

  for (windows_thread_info *th : thread_list)
    if (id == -1 || id == (int) th->tid)
      {
	if (!th->suspended)
	  continue;
#ifdef __x86_64__
	if (wow64_process)
	  {
	    if (debug_registers_changed)
	      {
		th->wow64_context.ContextFlags |= CONTEXT_DEBUG_REGISTERS;
		th->wow64_context.Dr0 = dr[0];
		th->wow64_context.Dr1 = dr[1];
		th->wow64_context.Dr2 = dr[2];
		th->wow64_context.Dr3 = dr[3];
		th->wow64_context.Dr6 = DR6_CLEAR_VALUE;
		th->wow64_context.Dr7 = dr[7];
	      }
	    if (th->wow64_context.ContextFlags)
	      {
		DWORD ec = 0;

		if (GetExitCodeThread (th->h, &ec)
		    && ec == STILL_ACTIVE)
		  {
		    BOOL status = Wow64SetThreadContext (th->h,
							 &th->wow64_context);

		    if (!killed)
		      CHECK (status);
		  }
		th->wow64_context.ContextFlags = 0;
	      }
	  }
	else
#endif
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
	  }
	th->resume ();
      }
    else
      {
	/* When single-stepping a specific thread, other threads must
	   be suspended.  */
	th->suspend ();
      }

  res = continue_last_debug_event (continue_status, debug_events);

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
  windows_add_thread (ptid_t (current_event.dwProcessId, 0,
			      current_event.dwThreadId),
		      current_event.u.CreateThread.hThread,
		      current_event.u.CreateThread.lpThreadLocalBase,
		      true /* main_thread_p */);
  return current_event.dwThreadId;
}

void
windows_nat_target::resume (ptid_t ptid, int step, enum gdb_signal sig)
{
  windows_thread_info *th;
  DWORD continue_status = DBG_CONTINUE;

  /* A specific PTID means `step only this thread id'.  */
  int resume_all = ptid == minus_one_ptid;

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
	  for (const xlate_exception &x : xlate)
	    if (x.us == sig)
	      {
		current_event.u.Exception.ExceptionRecord.ExceptionCode
		  = x.them;
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

  DEBUG_EXEC (("gdb: windows_resume (pid=%d, tid=0x%x, step=%d, sig=%d);\n",
	       ptid.pid (), (unsigned) ptid.lwp (), step, sig));

  /* Get context for currently selected thread.  */
  th = thread_rec (inferior_ptid, DONT_INVALIDATE_CONTEXT);
  if (th)
    {
#ifdef __x86_64__
      if (wow64_process)
	{
	  if (step)
	    {
	      /* Single step by setting t bit.  */
	      struct regcache *regcache = get_current_regcache ();
	      struct gdbarch *gdbarch = regcache->arch ();
	      fetch_registers (regcache, gdbarch_ps_regnum (gdbarch));
	      th->wow64_context.EFlags |= FLAG_TRACE_BIT;
	    }

	  if (th->wow64_context.ContextFlags)
	    {
	      if (debug_registers_changed)
		{
		  th->wow64_context.Dr0 = dr[0];
		  th->wow64_context.Dr1 = dr[1];
		  th->wow64_context.Dr2 = dr[2];
		  th->wow64_context.Dr3 = dr[3];
		  th->wow64_context.Dr6 = DR6_CLEAR_VALUE;
		  th->wow64_context.Dr7 = dr[7];
		}
	      CHECK (Wow64SetThreadContext (th->h, &th->wow64_context));
	      th->wow64_context.ContextFlags = 0;
	    }
	}
      else
#endif
	{
	  if (step)
	    {
	      /* Single step by setting t bit.  */
	      struct regcache *regcache = get_current_regcache ();
	      struct gdbarch *gdbarch = regcache->arch ();
	      fetch_registers (regcache, gdbarch_ps_regnum (gdbarch));
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
    }

  /* Allow continuing with the same signal that interrupted us.
     Otherwise complain.  */

  if (resume_all)
    windows_continue (continue_status, -1, 0);
  else
    windows_continue (continue_status, ptid.lwp (), 0);
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

int
windows_nat_target::get_windows_debug_event (int pid,
					     struct target_waitstatus *ourstatus)
{
  BOOL debug_event;
  DWORD continue_status, event_code;
  DWORD thread_id = 0;

  /* If there is a relevant pending stop, report it now.  See the
     comment by the definition of "pending_stops" for details on why
     this is needed.  */
  gdb::optional<pending_stop> stop = fetch_pending_stop (debug_events);
  if (stop.has_value ())
    {
      thread_id = stop->thread_id;
      *ourstatus = stop->status;

      ptid_t ptid (current_event.dwProcessId, thread_id);
      windows_thread_info *th = thread_rec (ptid, INVALIDATE_CONTEXT);
      th->reload_context = 1;

      return thread_id;
    }

  last_sig = GDB_SIGNAL_0;

  if (!(debug_event = wait_for_debug_event (&current_event, 1000)))
    goto out;

  continue_status = DBG_CONTINUE;

  event_code = current_event.dwDebugEventCode;
  ourstatus->kind = TARGET_WAITKIND_SPURIOUS;
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
	  inferior *inf = find_inferior_pid (this, current_event.dwProcessId);
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
      windows_add_thread
        (ptid_t (current_event.dwProcessId, current_event.dwThreadId, 0),
	 current_event.u.CreateThread.hThread,
	 current_event.u.CreateThread.lpThreadLocalBase,
	 false /* main_thread_p */);

      break;

    case EXIT_THREAD_DEBUG_EVENT:
      DEBUG_EVENTS (("gdb: kernel event for pid=%u tid=0x%x code=%s)\n",
		     (unsigned) current_event.dwProcessId,
		     (unsigned) current_event.dwThreadId,
		     "EXIT_THREAD_DEBUG_EVENT"));
      windows_delete_thread (ptid_t (current_event.dwProcessId,
				     current_event.dwThreadId, 0),
			     current_event.u.ExitThread.dwExitCode,
			     false /* main_thread_p */);
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
      /* Add the main thread.  */
      windows_add_thread
        (ptid_t (current_event.dwProcessId,
		 current_event.dwThreadId, 0),
	 current_event.u.CreateProcessInfo.hThread,
	 current_event.u.CreateProcessInfo.lpThreadLocalBase,
	 true /* main_thread_p */);
      thread_id = current_event.dwThreadId;
      break;

    case EXIT_PROCESS_DEBUG_EVENT:
      DEBUG_EVENTS (("gdb: kernel event for pid=%u tid=0x%x code=%s)\n",
		     (unsigned) current_event.dwProcessId,
		     (unsigned) current_event.dwThreadId,
		     "EXIT_PROCESS_DEBUG_EVENT"));
      if (!windows_initialization_done)
	{
	  target_terminal::ours ();
	  target_mourn_inferior (inferior_ptid);
	  error (_("During startup program exited with code 0x%x."),
		 (unsigned int) current_event.u.ExitProcess.dwExitCode);
	}
      else if (saw_create == 1)
	{
	  windows_delete_thread (ptid_t (current_event.dwProcessId,
					 current_event.dwThreadId, 0),
				 0, true /* main_thread_p */);
	  DWORD exit_status = current_event.u.ExitProcess.dwExitCode;
	  /* If the exit status looks like a fatal exception, but we
	     don't recognize the exception's code, make the original
	     exit status value available, to avoid losing
	     information.  */
	  int exit_signal
	    = WIFSIGNALED (exit_status) ? WTERMSIG (exit_status) : -1;
	  if (exit_signal == -1)
	    {
	      ourstatus->kind = TARGET_WAITKIND_EXITED;
	      ourstatus->value.integer = exit_status;
	    }
	  else
	    {
	      ourstatus->kind = TARGET_WAITKIND_SIGNALLED;
	      ourstatus->value.sig = gdb_signal_from_host (exit_signal);
	    }
	  thread_id = current_event.dwThreadId;
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
      catch_errors (handle_load_dll);
      ourstatus->kind = TARGET_WAITKIND_LOADED;
      ourstatus->value.integer = 0;
      thread_id = current_event.dwThreadId;
      break;

    case UNLOAD_DLL_DEBUG_EVENT:
      DEBUG_EVENTS (("gdb: kernel event for pid=%u tid=0x%x code=%s)\n",
		     (unsigned) current_event.dwProcessId,
		     (unsigned) current_event.dwThreadId,
		     "UNLOAD_DLL_DEBUG_EVENT"));
      if (saw_create != 1 || ! windows_initialization_done)
	break;
      catch_errors (handle_unload_dll);
      ourstatus->kind = TARGET_WAITKIND_LOADED;
      ourstatus->value.integer = 0;
      thread_id = current_event.dwThreadId;
      break;

    case EXCEPTION_DEBUG_EVENT:
      DEBUG_EVENTS (("gdb: kernel event for pid=%u tid=0x%x code=%s)\n",
		     (unsigned) current_event.dwProcessId,
		     (unsigned) current_event.dwThreadId,
		     "EXCEPTION_DEBUG_EVENT"));
      if (saw_create != 1)
	break;
      switch (handle_exception (ourstatus, debug_exceptions))
	{
	case HANDLE_EXCEPTION_UNHANDLED:
	default:
	  continue_status = DBG_EXCEPTION_NOT_HANDLED;
	  break;
	case HANDLE_EXCEPTION_HANDLED:
	  thread_id = current_event.dwThreadId;
	  break;
	case HANDLE_EXCEPTION_IGNORED:
	  continue_status = DBG_CONTINUE;
	  break;
	}
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
      CHECK (windows_continue (continue_status, desired_stop_thread_id, 0));
    }
  else if (desired_stop_thread_id != -1 && desired_stop_thread_id != thread_id)
    {
      /* Pending stop.  See the comment by the definition of
	 "pending_stops" for details on why this is needed.  */
      DEBUG_EVENTS (("get_windows_debug_event - "
		     "unexpected stop in 0x%x (expecting 0x%x)\n",
		     thread_id, desired_stop_thread_id));

      if (current_event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT
	  && ((current_event.u.Exception.ExceptionRecord.ExceptionCode
	       == EXCEPTION_BREAKPOINT)
	      || (current_event.u.Exception.ExceptionRecord.ExceptionCode
		  == STATUS_WX86_BREAKPOINT))
	  && windows_initialization_done)
	{
	  ptid_t ptid = ptid_t (current_event.dwProcessId, thread_id, 0);
	  windows_thread_info *th = thread_rec (ptid, INVALIDATE_CONTEXT);
	  th->stopped_at_software_breakpoint = true;
	  th->pc_adjusted = false;
	}
      pending_stops.push_back ({thread_id, *ourstatus, current_event});
      thread_id = 0;
      CHECK (windows_continue (continue_status, desired_stop_thread_id, 0));
    }

out:
  return thread_id;
}

/* Wait for interesting events to occur in the target process.  */
ptid_t
windows_nat_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
			  int options)
{
  int pid = -1;

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
      retval = get_windows_debug_event (pid, ourstatus);
      SetConsoleCtrlHandler (&ctrl_c_handler, FALSE);

      if (retval)
	{
	  ptid_t result = ptid_t (current_event.dwProcessId, retval, 0);

	  if (ourstatus->kind != TARGET_WAITKIND_EXITED
	      && ourstatus->kind !=  TARGET_WAITKIND_SIGNALLED)
	    {
	      windows_thread_info *th = thread_rec (result, INVALIDATE_CONTEXT);

	      if (th != nullptr)
		{
		  th->stopped_at_software_breakpoint = false;
		  if (current_event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT
		      && ((current_event.u.Exception.ExceptionRecord.ExceptionCode
			   == EXCEPTION_BREAKPOINT)
			  || (current_event.u.Exception.ExceptionRecord.ExceptionCode
			      == STATUS_WX86_BREAKPOINT))
		      && windows_initialization_done)
		    {
		      th->stopped_at_software_breakpoint = true;
		      th->pc_adjusted = false;
		    }
		}
	    }

	  return result;
	}
      else
	{
	  int detach = 0;

	  if (deprecated_ui_loop_hook != NULL)
	    detach = deprecated_ui_loop_hook (0);

	  if (detach)
	    kill ();
	}
    }
}

/* Iterate over all DLLs currently mapped by our inferior, and
   add them to our list of solibs.  */

static void
windows_add_all_dlls (void)
{
  HMODULE dummy_hmodule;
  DWORD cb_needed;
  HMODULE *hmodules;
  int i;

#ifdef __x86_64__
  if (wow64_process)
    {
      if (EnumProcessModulesEx (current_process_handle, &dummy_hmodule,
				sizeof (HMODULE), &cb_needed,
				LIST_MODULES_32BIT) == 0)
	return;
    }
  else
#endif
    {
      if (EnumProcessModules (current_process_handle, &dummy_hmodule,
			      sizeof (HMODULE), &cb_needed) == 0)
	return;
    }

  if (cb_needed < 1)
    return;

  hmodules = (HMODULE *) alloca (cb_needed);
#ifdef __x86_64__
  if (wow64_process)
    {
      if (EnumProcessModulesEx (current_process_handle, hmodules,
				cb_needed, &cb_needed,
				LIST_MODULES_32BIT) == 0)
	return;
    }
  else
#endif
    {
      if (EnumProcessModules (current_process_handle, hmodules,
			      cb_needed, &cb_needed) == 0)
	return;
    }

  char system_dir[__PMAX];
  char syswow_dir[__PMAX];
  size_t system_dir_len = 0;
  bool convert_syswow_dir = false;
#ifdef __x86_64__
  if (wow64_process)
#endif
    {
      /* This fails on 32bit Windows because it has no SysWOW64 directory,
	 and in this case a path conversion isn't necessary.  */
      UINT len = GetSystemWow64DirectoryA (syswow_dir, sizeof (syswow_dir));
      if (len > 0)
	{
	  /* Check that we have passed a large enough buffer.  */
	  gdb_assert (len < sizeof (syswow_dir));

	  len = GetSystemDirectoryA (system_dir, sizeof (system_dir));
	  /* Error check.  */
	  gdb_assert (len != 0);
	  /* Check that we have passed a large enough buffer.  */
	  gdb_assert (len < sizeof (system_dir));

	  strcat (system_dir, "\\");
	  strcat (syswow_dir, "\\");
	  system_dir_len = strlen (system_dir);

	  convert_syswow_dir = true;
	}

    }
  for (i = 1; i < (int) (cb_needed / sizeof (HMODULE)); i++)
    {
      MODULEINFO mi;
#ifdef __USEWIDE
      wchar_t dll_name[__PMAX];
      char dll_name_mb[__PMAX];
#else
      char dll_name[__PMAX];
#endif
      const char *name;
      if (GetModuleInformation (current_process_handle, hmodules[i],
				&mi, sizeof (mi)) == 0)
	continue;
      if (GetModuleFileNameEx (current_process_handle, hmodules[i],
			       dll_name, sizeof (dll_name)) == 0)
	continue;
#ifdef __USEWIDE
      wcstombs (dll_name_mb, dll_name, __PMAX);
      name = dll_name_mb;
#else
      name = dll_name;
#endif
      /* Convert the DLL path of 32bit processes returned by
	 GetModuleFileNameEx from the 64bit system directory to the
	 32bit syswow64 directory if necessary.  */
      std::string syswow_dll_path;
      if (convert_syswow_dir
	  && strncasecmp (name, system_dir, system_dir_len) == 0
	  && strchr (name + system_dir_len, '\\') == nullptr)
	{
	  syswow_dll_path = syswow_dir;
	  syswow_dll_path += name + system_dir_len;
	  name = syswow_dll_path.c_str();
	}

      solib_end->next = windows_make_so (name, mi.lpBaseOfDll);
      solib_end = solib_end->next;
    }
}

void
windows_nat_target::do_initial_windows_stuff (DWORD pid, bool attaching)
{
  int i;
  struct inferior *inf;

  last_sig = GDB_SIGNAL_0;
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
  if (!target_is_pushed (this))
    push_target (this);
  disable_breakpoints_in_shlibs ();
  windows_clear_solib ();
  clear_proceed_status (0);
  init_wait_for_inferior ();

#ifdef __x86_64__
  ignore_first_breakpoint = !attaching && wow64_process;

  if (!wow64_process)
    {
      windows_set_context_register_offsets (amd64_mappings);
      windows_set_segment_register_p (amd64_windows_segment_register_p);
    }
  else
#endif
    {
      windows_set_context_register_offsets (i386_mappings);
      windows_set_segment_register_p (i386_windows_segment_register_p);
    }

  inf = current_inferior ();
  inferior_appeared (inf, pid);
  inf->attach_flag = attaching;

  target_terminal::init ();
  target_terminal::inferior ();

  windows_initialization_done = 0;

  ptid_t last_ptid;

  while (1)
    {
      struct target_waitstatus status;

      last_ptid = this->wait (minus_one_ptid, &status, 0);

      /* Note windows_wait returns TARGET_WAITKIND_SPURIOUS for thread
	 events.  */
      if (status.kind != TARGET_WAITKIND_LOADED
	  && status.kind != TARGET_WAITKIND_SPURIOUS)
	break;

      this->resume (minus_one_ptid, 0, GDB_SIGNAL_0);
    }

  switch_to_thread (find_thread_ptid (this, last_ptid));

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

void
windows_nat_target::attach (const char *args, int from_tty)
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
    error (_("Can't attach to process %u (error %u)"),
	   (unsigned) pid, (unsigned) GetLastError ());

  DebugSetProcessKillOnExit (FALSE);

  if (from_tty)
    {
      const char *exec_file = get_exec_file (0);

      if (exec_file)
	printf_unfiltered ("Attaching to program `%s', %s\n", exec_file,
			   target_pid_to_str (ptid_t (pid)).c_str ());
      else
	printf_unfiltered ("Attaching to %s\n",
			   target_pid_to_str (ptid_t (pid)).c_str ());
    }

#ifdef __x86_64__
  HANDLE h = OpenProcess (PROCESS_QUERY_INFORMATION, FALSE, pid);
  if (h != NULL)
    {
      BOOL wow64;
      if (IsWow64Process (h, &wow64))
	wow64_process = wow64;
      CloseHandle (h);
    }
#endif

  do_initial_windows_stuff (pid, 1);
  target_terminal::ours ();
}

void
windows_nat_target::detach (inferior *inf, int from_tty)
{
  int detached = 1;

  ptid_t ptid = minus_one_ptid;
  resume (ptid, 0, GDB_SIGNAL_0);

  if (!DebugActiveProcessStop (current_event.dwProcessId))
    {
      error (_("Can't detach process %u (error %u)"),
	     (unsigned) current_event.dwProcessId, (unsigned) GetLastError ());
      detached = 0;
    }
  DebugSetProcessKillOnExit (FALSE);

  if (detached && from_tty)
    {
      const char *exec_file = get_exec_file (0);
      if (exec_file == 0)
	exec_file = "";
      printf_unfiltered ("Detaching from program: %s, Pid %u\n", exec_file,
			 (unsigned) current_event.dwProcessId);
    }

  x86_cleanup_dregs ();
  switch_to_no_thread ();
  detach_inferior (inf);

  maybe_unpush_target ();
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
#ifdef __x86_64__
  if (wow64_process)
    {
      if (!EnumProcessModulesEx (current_process_handle, &dh_buf,
				 sizeof (HMODULE), &cbNeeded,
				 LIST_MODULES_32BIT) || !cbNeeded)
	return 0;
    }
  else
#endif
    {
      if (!EnumProcessModules (current_process_handle, &dh_buf,
			       sizeof (HMODULE), &cbNeeded) || !cbNeeded)
	return 0;
    }

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

char *
windows_nat_target::pid_to_exec_file (int pid)
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

void
windows_nat_target::files_info ()
{
  struct inferior *inf = current_inferior ();

  printf_unfiltered ("\tUsing the running image of %s %s.\n",
		     inf->attach_flag ? "attached" : "child",
		     target_pid_to_str (inferior_ptid).c_str ());
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

#ifndef __CYGWIN__

/* Redirection of inferior I/O streams for native MS-Windows programs.
   Unlike on Unix, where this is handled by invoking the inferior via
   the shell, on MS-Windows we need to emulate the cmd.exe shell.

   The official documentation of the cmd.exe redirection features is here:

     http://www.microsoft.com/resources/documentation/windows/xp/all/proddocs/en-us/redirection.mspx

   (That page talks about Windows XP, but there's no newer
   documentation, so we assume later versions of cmd.exe didn't change
   anything.)

   Caveat: the documentation on that page seems to include a few lies.
   For example, it describes strange constructs 1<&2 and 2<&1, which
   seem to work only when 1>&2 resp. 2>&1 would make sense, and so I
   think the cmd.exe parser of the redirection symbols simply doesn't
   care about the < vs > distinction in these cases.  Therefore, the
   supported features are explicitly documented below.

   The emulation below aims at supporting all the valid use cases
   supported by cmd.exe, which include:

     < FILE    redirect standard input from FILE
     0< FILE   redirect standard input from FILE
     <&N       redirect standard input from file descriptor N
     0<&N      redirect standard input from file descriptor N
     > FILE    redirect standard output to FILE
     >> FILE   append standard output to FILE
     1>> FILE  append standard output to FILE
     >&N       redirect standard output to file descriptor N
     1>&N      redirect standard output to file descriptor N
     >>&N      append standard output to file descriptor N
     1>>&N     append standard output to file descriptor N
     2> FILE   redirect standard error to FILE
     2>> FILE  append standard error to FILE
     2>&N      redirect standard error to file descriptor N
     2>>&N     append standard error to file descriptor N

     Note that using N > 2 in the above construct is supported, but
     requires that the corresponding file descriptor be open by some
     means elsewhere or outside GDB.  Also note that using ">&0" or
     "<&2" will generally fail, because the file descriptor redirected
     from is normally open in an incompatible mode (e.g., FD 0 is open
     for reading only).  IOW, use of such tricks is not recommended;
     you are on your own.

     We do NOT support redirection of file descriptors above 2, as in
     "3>SOME-FILE", because MinGW compiled programs don't (supporting
     that needs special handling in the startup code that MinGW
     doesn't have).  Pipes are also not supported.

     As for invalid use cases, where the redirection contains some
     error, the emulation below will detect that and produce some
     error and/or failure.  But the behavior in those cases is not
     bug-for-bug compatible with what cmd.exe does in those cases.
     That's because what cmd.exe does then is not well defined, and
     seems to be a side effect of the cmd.exe parsing of the command
     line more than anything else.  For example, try redirecting to an
     invalid file name, as in "> foo:bar".

     There are also minor syntactic deviations from what cmd.exe does
     in some corner cases.  For example, it doesn't support the likes
     of "> &foo" to mean redirect to file named literally "&foo"; we
     do support that here, because that, too, sounds like some issue
     with the cmd.exe parser.  Another nicety is that we support
     redirection targets that use file names with forward slashes,
     something cmd.exe doesn't -- this comes in handy since GDB
     file-name completion can be used when typing the command line for
     the inferior.  */

/* Support routines for redirecting standard handles of the inferior.  */

/* Parse a single redirection spec, open/duplicate the specified
   file/fd, and assign the appropriate value to one of the 3 standard
   file descriptors. */
static int
redir_open (const char *redir_string, int *inp, int *out, int *err)
{
  int *fd, ref_fd = -2;
  int mode;
  const char *fname = redir_string + 1;
  int rc = *redir_string;

  switch (rc)
    {
    case '0':
      fname++;
      /* FALLTHROUGH */
    case '<':
      fd = inp;
      mode = O_RDONLY;
      break;
    case '1': case '2':
      fname++;
      /* FALLTHROUGH */
    case '>':
      fd = (rc == '2') ? err : out;
      mode = O_WRONLY | O_CREAT;
      if (*fname == '>')
	{
	  fname++;
	  mode |= O_APPEND;
	}
      else
	mode |= O_TRUNC;
      break;
    default:
      return -1;
    }

  if (*fname == '&' && '0' <= fname[1] && fname[1] <= '9')
    {
      /* A reference to a file descriptor.  */
      char *fdtail;
      ref_fd = (int) strtol (fname + 1, &fdtail, 10);
      if (fdtail > fname + 1 && *fdtail == '\0')
	{
	  /* Don't allow redirection when open modes are incompatible.  */
	  if ((ref_fd == 0 && (fd == out || fd == err))
	      || ((ref_fd == 1 || ref_fd == 2) && fd == inp))
	    {
	      errno = EPERM;
	      return -1;
	    }
	  if (ref_fd == 0)
	    ref_fd = *inp;
	  else if (ref_fd == 1)
	    ref_fd = *out;
	  else if (ref_fd == 2)
	    ref_fd = *err;
	}
      else
	{
	  errno = EBADF;
	  return -1;
	}
    }
  else
    fname++;	/* skip the separator space */
  /* If the descriptor is already open, close it.  This allows
     multiple specs of redirections for the same stream, which is
     somewhat nonsensical, but still valid and supported by cmd.exe.
     (But cmd.exe only opens a single file in this case, the one
     specified by the last redirection spec on the command line.)  */
  if (*fd >= 0)
    _close (*fd);
  if (ref_fd == -2)
    {
      *fd = _open (fname, mode, _S_IREAD | _S_IWRITE);
      if (*fd < 0)
	return -1;
    }
  else if (ref_fd == -1)
    *fd = -1;	/* reset to default destination */
  else
    {
      *fd = _dup (ref_fd);
      if (*fd < 0)
	return -1;
    }
  /* _open just sets a flag for O_APPEND, which won't be passed to the
     inferior, so we need to actually move the file pointer.  */
  if ((mode & O_APPEND) != 0)
    _lseek (*fd, 0L, SEEK_END);
  return 0;
}

/* Canonicalize a single redirection spec and set up the corresponding
   file descriptor as specified.  */
static int
redir_set_redirection (const char *s, int *inp, int *out, int *err)
{
  char buf[__PMAX + 2 + 5]; /* extra space for quotes & redirection string */
  char *d = buf;
  const char *start = s;
  int quote = 0;

  *d++ = *s++;	/* copy the 1st character, < or > or a digit */
  if ((*start == '>' || *start == '1' || *start == '2')
      && *s == '>')
    {
      *d++ = *s++;
      if (*s == '>' && *start != '>')
	*d++ = *s++;
    }
  else if (*start == '0' && *s == '<')
    *d++ = *s++;
  /* cmd.exe recognizes "&N" only immediately after the redirection symbol.  */
  if (*s != '&')
    {
      while (isspace (*s))  /* skip whitespace before file name */
	s++;
      *d++ = ' ';	    /* separate file name with a single space */
    }

  /* Copy the file name.  */
  while (*s)
    {
      /* Remove quoting characters from the file name in buf[].  */
      if (*s == '"')	/* could support '..' quoting here */
	{
	  if (!quote)
	    quote = *s++;
	  else if (*s == quote)
	    {
	      quote = 0;
	      s++;
	    }
	  else
	    *d++ = *s++;
	}
      else if (*s == '\\')
	{
	  if (s[1] == '"')	/* could support '..' here */
	    s++;
	  *d++ = *s++;
	}
      else if (isspace (*s) && !quote)
	break;
      else
	*d++ = *s++;
      if (d - buf >= sizeof (buf) - 1)
	{
	  errno = ENAMETOOLONG;
	  return 0;
	}
    }
  *d = '\0';

  /* Windows doesn't allow redirection characters in file names, so we
     can bail out early if they use them, or if there's no target file
     name after the redirection symbol.  */
  if (d[-1] == '>' || d[-1] == '<')
    {
      errno = ENOENT;
      return 0;
    }
  if (redir_open (buf, inp, out, err) == 0)
    return s - start;
  return 0;
}

/* Parse the command line for redirection specs and prepare the file
   descriptors for the 3 standard streams accordingly.  */
static bool
redirect_inferior_handles (const char *cmd_orig, char *cmd,
			   int *inp, int *out, int *err)
{
  const char *s = cmd_orig;
  char *d = cmd;
  int quote = 0;
  bool retval = false;

  while (isspace (*s))
    *d++ = *s++;

  while (*s)
    {
      if (*s == '"')	/* could also support '..' quoting here */
	{
	  if (!quote)
	    quote = *s;
	  else if (*s == quote)
	    quote = 0;
	}
      else if (*s == '\\')
	{
	  if (s[1] == '"')	/* escaped quote char */
	    s++;
	}
      else if (!quote)
	{
	  /* Process a single redirection candidate.  */
	  if (*s == '<' || *s == '>'
	      || ((*s == '1' || *s == '2') && s[1] == '>')
	      || (*s == '0' && s[1] == '<'))
	    {
	      int skip = redir_set_redirection (s, inp, out, err);

	      if (skip <= 0)
		return false;
	      retval = true;
	      s += skip;
	    }
	}
      if (*s)
	*d++ = *s++;
    }
  *d = '\0';
  return retval;
}
#endif	/* !__CYGWIN__ */

/* Start an inferior windows child process and sets inferior_ptid to its pid.
   EXEC_FILE is the file to run.
   ALLARGS is a string containing the arguments to the program.
   ENV is the environment vector to pass.  Errors reported with error().  */

void
windows_nat_target::create_inferior (const char *exec_file,
				     const std::string &origallargs,
				     char **in_env, int from_tty)
{
  STARTUPINFO si;
#ifdef __CYGWIN__
  cygwin_buf_t real_path[__PMAX];
  cygwin_buf_t shell[__PMAX]; /* Path to shell */
  cygwin_buf_t infcwd[__PMAX];
  const char *sh;
  cygwin_buf_t *toexec;
  cygwin_buf_t *cygallargs;
  cygwin_buf_t *args;
  char **old_env = NULL;
  PWCHAR w32_env;
  size_t len;
  int tty;
  int ostdin, ostdout, ostderr;
#else  /* !__CYGWIN__ */
  char shell[__PMAX]; /* Path to shell */
  const char *toexec;
  char *args, *allargs_copy;
  size_t args_len, allargs_len;
  int fd_inp = -1, fd_out = -1, fd_err = -1;
  HANDLE tty = INVALID_HANDLE_VALUE;
  bool redirected = false;
  char *w32env;
  char *temp;
  size_t envlen;
  int i;
  size_t envsize;
  char **env;
#endif	/* !__CYGWIN__ */
  const char *allargs = origallargs.c_str ();
  PROCESS_INFORMATION pi;
  BOOL ret;
  DWORD flags = 0;
  const char *inferior_tty = current_inferior ()->tty ();

  if (!exec_file)
    error (_("No executable specified, use `target exec'."));

  const char *inferior_cwd = get_inferior_cwd ();
  std::string expanded_infcwd;
  if (inferior_cwd != NULL)
    {
      expanded_infcwd = gdb_tilde_expand (inferior_cwd);
      /* Mirror slashes on inferior's cwd.  */
      std::replace (expanded_infcwd.begin (), expanded_infcwd.end (),
		    '/', '\\');
      inferior_cwd = expanded_infcwd.c_str ();
    }

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
#else  /* !__USEWIDE */
      cygallargs = allargs;
#endif
    }
  else
    {
      sh = get_shell ();
      if (cygwin_conv_path (CCP_POSIX_TO_WIN_W, sh, shell, __PMAX) < 0)
      	error (_("Error starting executable via shell: %d"), errno);
#ifdef __USEWIDE
      len = sizeof (L" -c 'exec  '") + mbstowcs (NULL, exec_file, 0)
	    + mbstowcs (NULL, allargs, 0) + 2;
      cygallargs = (wchar_t *) alloca (len * sizeof (wchar_t));
      swprintf (cygallargs, len, L" -c 'exec %s %s'", exec_file, allargs);
#else  /* !__USEWIDE */
      len = (sizeof (" -c 'exec  '") + strlen (exec_file)
	     + strlen (allargs) + 2);
      cygallargs = (char *) alloca (len);
      xsnprintf (cygallargs, len, " -c 'exec %s %s'", exec_file, allargs);
#endif	/* __USEWIDE */
      toexec = shell;
      flags |= DEBUG_PROCESS;
    }

  if (inferior_cwd != NULL
      && cygwin_conv_path (CCP_POSIX_TO_WIN_W, inferior_cwd,
			   infcwd, strlen (inferior_cwd)) < 0)
    error (_("Error converting inferior cwd: %d"), errno);

#ifdef __USEWIDE
  args = (cygwin_buf_t *) alloca ((wcslen (toexec) + wcslen (cygallargs) + 2)
				  * sizeof (wchar_t));
  wcscpy (args, toexec);
  wcscat (args, L" ");
  wcscat (args, cygallargs);
#else  /* !__USEWIDE */
  args = (cygwin_buf_t *) alloca (strlen (toexec) + strlen (cygallargs) + 2);
  strcpy (args, toexec);
  strcat (args, " ");
  strcat (args, cygallargs);
#endif	/* !__USEWIDE */

#ifdef CW_CVT_ENV_TO_WINENV
  /* First try to create a direct Win32 copy of the POSIX environment. */
  w32_env = (PWCHAR) cygwin_internal (CW_CVT_ENV_TO_WINENV, in_env);
  if (w32_env != (PWCHAR) -1)
    flags |= CREATE_UNICODE_ENVIRONMENT;
  else
    /* If that fails, fall back to old method tweaking GDB's environment. */
#endif	/* CW_CVT_ENV_TO_WINENV */
    {
      /* Reset all Win32 environment variables to avoid leftover on next run. */
      clear_win32_environment (environ);
      /* Prepare the environment vars for CreateProcess.  */
      old_env = environ;
      environ = in_env;
      cygwin_internal (CW_SYNC_WINENV);
      w32_env = NULL;
    }

  if (inferior_tty == nullptr)
    tty = ostdin = ostdout = ostderr = -1;
  else
    {
      tty = open (inferior_tty, O_RDWR | O_NOCTTY);
      if (tty < 0)
	{
	  print_sys_errmsg (inferior_tty, errno);
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
		       inferior_cwd != NULL ? infcwd : NULL, /* current
								directory */
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
      ::close (tty);
      dup2 (ostdin, 0);
      dup2 (ostdout, 1);
      dup2 (ostderr, 2);
      ::close (ostdin);
      ::close (ostdout);
      ::close (ostderr);
    }
#else  /* !__CYGWIN__ */
  allargs_len = strlen (allargs);
  allargs_copy = strcpy ((char *) alloca (allargs_len + 1), allargs);
  if (strpbrk (allargs_copy, "<>") != NULL)
    {
      int e = errno;
      errno = 0;
      redirected =
	redirect_inferior_handles (allargs, allargs_copy,
				   &fd_inp, &fd_out, &fd_err);
      if (errno)
	warning (_("Error in redirection: %s."), safe_strerror (errno));
      else
	errno = e;
      allargs_len = strlen (allargs_copy);
    }
  /* If not all the standard streams are redirected by the command
     line, use INFERIOR_TTY for those which aren't.  */
  if (inferior_tty != nullptr
      && !(fd_inp >= 0 && fd_out >= 0 && fd_err >= 0))
    {
      SECURITY_ATTRIBUTES sa;
      sa.nLength = sizeof(sa);
      sa.lpSecurityDescriptor = 0;
      sa.bInheritHandle = TRUE;
      tty = CreateFileA (inferior_tty, GENERIC_READ | GENERIC_WRITE,
			 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if (tty == INVALID_HANDLE_VALUE)
	warning (_("Warning: Failed to open TTY %s, error %#x."),
		 inferior_tty, (unsigned) GetLastError ());
    }
  if (redirected || tty != INVALID_HANDLE_VALUE)
    {
      if (fd_inp >= 0)
	si.hStdInput = (HANDLE) _get_osfhandle (fd_inp);
      else if (tty != INVALID_HANDLE_VALUE)
	si.hStdInput = tty;
      else
	si.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
      if (fd_out >= 0)
	si.hStdOutput = (HANDLE) _get_osfhandle (fd_out);
      else if (tty != INVALID_HANDLE_VALUE)
	si.hStdOutput = tty;
      else
	si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
      if (fd_err >= 0)
	si.hStdError = (HANDLE) _get_osfhandle (fd_err);
      else if (tty != INVALID_HANDLE_VALUE)
	si.hStdError = tty;
      else
	si.hStdError = GetStdHandle (STD_ERROR_HANDLE);
      si.dwFlags |= STARTF_USESTDHANDLES;
    }

  toexec = exec_file;
  /* Build the command line, a space-separated list of tokens where
     the first token is the name of the module to be executed.
     To avoid ambiguities introduced by spaces in the module name,
     we quote it.  */
  args_len = strlen (toexec) + 2 /* quotes */ + allargs_len + 2;
  args = (char *) alloca (args_len);
  xsnprintf (args, args_len, "\"%s\" %s", toexec, allargs_copy);

  flags |= DEBUG_ONLY_THIS_PROCESS;

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
			inferior_cwd, /* current directory */
			&si,
			&pi);
  if (tty != INVALID_HANDLE_VALUE)
    CloseHandle (tty);
  if (fd_inp >= 0)
    _close (fd_inp);
  if (fd_out >= 0)
    _close (fd_out);
  if (fd_err >= 0)
    _close (fd_err);
#endif	/* !__CYGWIN__ */

  if (!ret)
    error (_("Error creating process %s, (error %u)."),
	   exec_file, (unsigned) GetLastError ());

#ifdef __x86_64__
  BOOL wow64;
  if (IsWow64Process (pi.hProcess, &wow64))
    wow64_process = wow64;
#endif

  CloseHandle (pi.hThread);
  CloseHandle (pi.hProcess);

  if (useshell && shell[0] != '\0')
    saw_create = -1;
  else
    saw_create = 0;

  do_initial_windows_stuff (pi.dwProcessId, 0);

  /* windows_continue (DBG_CONTINUE, -1, 0); */
}

void
windows_nat_target::mourn_inferior ()
{
  (void) windows_continue (DBG_CONTINUE, -1, 0);
  x86_cleanup_dregs();
  if (open_process_used)
    {
      CHECK (CloseHandle (current_process_handle));
      open_process_used = 0;
    }
  siginfo_er.ExceptionCode = 0;
  inf_child_target::mourn_inferior ();
}

/* Send a SIGINT to the process group.  This acts just like the user typed a
   ^C on the controlling terminal.  */

void
windows_nat_target::interrupt ()
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

void
windows_nat_target::kill ()
{
  CHECK (TerminateProcess (current_process_handle, 0));

  for (;;)
    {
      if (!windows_continue (DBG_CONTINUE, -1, 1))
	break;
      if (!wait_for_debug_event (&current_event, INFINITE))
	break;
      if (current_event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
	break;
    }

  target_mourn_inferior (inferior_ptid);	/* Or just windows_mourn_inferior?  */
}

void
windows_nat_target::close ()
{
  DEBUG_EVENTS (("gdb: windows_close, inferior_ptid=%d\n",
		inferior_ptid.pid ()));
}

/* Convert pid to printable format.  */
std::string
windows_nat_target::pid_to_str (ptid_t ptid)
{
  if (ptid.lwp () != 0)
    return string_printf ("Thread %d.0x%lx", ptid.pid (), ptid.lwp ());

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
    {
      lm_info_windows *li = (lm_info_windows *) so->lm_info;

      windows_xfer_shared_library (so->so_name, (CORE_ADDR)
				   (uintptr_t) li->load_addr,
				   &li->text_offset,
				   target_gdbarch (), &obstack);
    }
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

/* Helper for windows_nat_target::xfer_partial that handles signal info.  */

static enum target_xfer_status
windows_xfer_siginfo (gdb_byte *readbuf, ULONGEST offset, ULONGEST len,
		      ULONGEST *xfered_len)
{
  char *buf = (char *) &siginfo_er;
  size_t bufsize = sizeof (siginfo_er);

#ifdef __x86_64__
  EXCEPTION_RECORD32 er32;
  if (wow64_process)
    {
      buf = (char *) &er32;
      bufsize = sizeof (er32);

      er32.ExceptionCode = siginfo_er.ExceptionCode;
      er32.ExceptionFlags = siginfo_er.ExceptionFlags;
      er32.ExceptionRecord = (uintptr_t) siginfo_er.ExceptionRecord;
      er32.ExceptionAddress = (uintptr_t) siginfo_er.ExceptionAddress;
      er32.NumberParameters = siginfo_er.NumberParameters;
      int i;
      for (i = 0; i < EXCEPTION_MAXIMUM_PARAMETERS; i++)
	er32.ExceptionInformation[i] = siginfo_er.ExceptionInformation[i];
    }
#endif

  if (siginfo_er.ExceptionCode == 0)
    return TARGET_XFER_E_IO;

  if (readbuf == nullptr)
    return TARGET_XFER_E_IO;

  if (offset > bufsize)
    return TARGET_XFER_E_IO;

  if (offset + len > bufsize)
    len = bufsize - offset;

  memcpy (readbuf, buf + offset, len);
  *xfered_len = len;

  return TARGET_XFER_OK;
}

enum target_xfer_status
windows_nat_target::xfer_partial (enum target_object object,
				  const char *annex, gdb_byte *readbuf,
				  const gdb_byte *writebuf, ULONGEST offset,
				  ULONGEST len, ULONGEST *xfered_len)
{
  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
      return windows_xfer_memory (readbuf, writebuf, offset, len, xfered_len);

    case TARGET_OBJECT_LIBRARIES:
      return windows_xfer_shared_libraries (this, object, annex, readbuf,
					    writebuf, offset, len, xfered_len);

    case TARGET_OBJECT_SIGNAL_INFO:
      return windows_xfer_siginfo (readbuf, offset, len, xfered_len);

    default:
      if (beneath () == NULL)
	{
	  /* This can happen when requesting the transfer of unsupported
	     objects before a program has been started (and therefore
	     with the current_target having no target beneath).  */
	  return TARGET_XFER_E_IO;
	}
      return beneath ()->xfer_partial (object, annex,
				       readbuf, writebuf, offset, len,
				       xfered_len);
    }
}

/* Provide thread local base, i.e. Thread Information Block address.
   Returns 1 if ptid is found and sets *ADDR to thread_local_base.  */

bool
windows_nat_target::get_tib_address (ptid_t ptid, CORE_ADDR *addr)
{
  windows_thread_info *th;

  th = thread_rec (ptid, DONT_INVALIDATE_CONTEXT);
  if (th == NULL)
    return false;

  if (addr != NULL)
    *addr = th->thread_local_base;

  return true;
}

ptid_t
windows_nat_target::get_ada_task_ptid (long lwp, long thread)
{
  return ptid_t (inferior_ptid.pid (), lwp, 0);
}

/* Implementation of the to_thread_name method.  */

const char *
windows_nat_target::thread_name (struct thread_info *thr)
{
  return thread_rec (thr->ptid, DONT_INVALIDATE_CONTEXT)->name.get ();
}


void _initialize_windows_nat ();
void
_initialize_windows_nat ()
{
  x86_dr_low.set_control = cygwin_set_dr7;
  x86_dr_low.set_addr = cygwin_set_dr;
  x86_dr_low.get_addr = cygwin_get_dr;
  x86_dr_low.get_status = cygwin_get_dr6;
  x86_dr_low.get_control = cygwin_get_dr7;

  /* x86_dr_low.debug_register_length field is set by
     calling x86_set_debug_register_length function
     in processor windows specific native file.  */

  add_inf_child_target (&the_windows_nat_target);

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

bool
windows_nat_target::thread_alive (ptid_t ptid)
{
  gdb_assert (ptid.lwp () != 0);

  return (WaitForSingleObject (thread_rec (ptid, DONT_INVALIDATE_CONTEXT)->h, 0)
	  != WAIT_OBJECT_0);
}

void _initialize_check_for_gdb_ini ();
void
_initialize_check_for_gdb_ini ()
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
 
/* Load any functions which may not be available in ancient versions
   of Windows.  */

void _initialize_loadable ();
void
_initialize_loadable ()
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
#ifdef __x86_64__
      GPA (hm, Wow64SuspendThread);
      GPA (hm, Wow64GetThreadContext);
      GPA (hm, Wow64SetThreadContext);
      GPA (hm, Wow64GetThreadSelectorEntry);
#endif
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
#ifdef __x86_64__
      GPA (hm, EnumProcessModulesEx);
#endif
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
