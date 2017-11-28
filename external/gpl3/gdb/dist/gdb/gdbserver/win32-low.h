/* Internal interfaces for the Win32 specific target code for gdbserver.
   Copyright (C) 2007-2017 Free Software Foundation, Inc.

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

#include <windows.h>

struct target_desc;

/* The inferior's target description.  This is a global because the
   Windows ports support neither bi-arch nor multi-process.  */
extern const struct target_desc *win32_tdesc;

/* Thread information structure used to track extra information about
   each thread.  */
typedef struct win32_thread_info
{
  /* The Win32 thread identifier.  */
  DWORD tid;

  /* The handle to the thread.  */
  HANDLE h;

  /* Thread Information Block address.  */
  CORE_ADDR thread_local_base;

  /* Non zero if SuspendThread was called on this thread.  */
  int suspended;

#ifdef _WIN32_WCE
  /* The context as retrieved right after suspending the thread. */
  CONTEXT base_context;
#endif

  /* The context of the thread, including any manipulations.  */
  CONTEXT context;

  /* Whether debug registers changed since we last set CONTEXT back to
     the thread.  */
  int debug_registers_changed;
} win32_thread_info;

struct win32_target_ops
{
  /* Architecture-specific setup.  */
  void (*arch_setup) (void);

  /* The number of target registers.  */
  int num_regs;

  /* Perform initializations on startup.  */
  void (*initial_stuff) (void);

  /* Fetch the context from the inferior.  */
  void (*get_thread_context) (win32_thread_info *th);

  /* Called just before resuming the thread.  */
  void (*prepare_to_resume) (win32_thread_info *th);

  /* Called when a thread was added.  */
  void (*thread_added) (win32_thread_info *th);

  /* Fetch register from gdbserver regcache data.  */
  void (*fetch_inferior_register) (struct regcache *regcache,
				   win32_thread_info *th, int r);

  /* Store a new register value into the thread context of TH.  */
  void (*store_inferior_register) (struct regcache *regcache,
				   win32_thread_info *th, int r);

  void (*single_step) (win32_thread_info *th);

  const unsigned char *breakpoint;
  int breakpoint_len;

  /* Breakpoint/Watchpoint related functions.  See target.h for comments.  */
  int (*supports_z_point_type) (char z_type);
  int (*insert_point) (enum raw_bkpt_type type, CORE_ADDR addr,
		       int size, struct raw_breakpoint *bp);
  int (*remove_point) (enum raw_bkpt_type type, CORE_ADDR addr,
		       int size, struct raw_breakpoint *bp);
  int (*stopped_by_watchpoint) (void);
  CORE_ADDR (*stopped_data_address) (void);
};

extern struct win32_target_ops the_low_target;

/* Retrieve the context for this thread, if not already retrieved.  */
extern void win32_require_context (win32_thread_info *th);

/* Map the Windows error number in ERROR to a locale-dependent error
   message string and return a pointer to it.  Typically, the values
   for ERROR come from GetLastError.

   The string pointed to shall not be modified by the application,
   but may be overwritten by a subsequent call to strwinerror

   The strwinerror function does not change the current setting
   of GetLastError.  */
extern char * strwinerror (DWORD error);

/* in wincecompat.c */

extern void to_back_slashes (char *);
