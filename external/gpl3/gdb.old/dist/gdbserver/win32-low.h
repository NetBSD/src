/* Internal interfaces for the Win32 specific target code for gdbserver.
   Copyright (C) 2007-2020 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_WIN32_LOW_H
#define GDBSERVER_WIN32_LOW_H

#include <windows.h>
#include "nat/windows-nat.h"

struct target_desc;

/* The inferior's target description.  This is a global because the
   Windows ports support neither bi-arch nor multi-process.  */
extern const struct target_desc *win32_tdesc;
#ifdef __x86_64__
extern const struct target_desc *wow64_win32_tdesc;

extern bool wow64_process;

typedef BOOL (WINAPI *winapi_Wow64GetThreadContext) (HANDLE, PWOW64_CONTEXT);
extern winapi_Wow64GetThreadContext win32_Wow64GetThreadContext;
#endif

struct win32_target_ops
{
  /* Architecture-specific setup.  */
  void (*arch_setup) (void);

  /* The number of target registers.  */
  int (*num_regs) (void);

  /* Perform initializations on startup.  */
  void (*initial_stuff) (void);

  /* Fetch the context from the inferior.  */
  void (*get_thread_context) (windows_nat::windows_thread_info *th);

  /* Called just before resuming the thread.  */
  void (*prepare_to_resume) (windows_nat::windows_thread_info *th);

  /* Called when a thread was added.  */
  void (*thread_added) (windows_nat::windows_thread_info *th);

  /* Fetch register from gdbserver regcache data.  */
  void (*fetch_inferior_register) (struct regcache *regcache,
				   windows_nat::windows_thread_info *th,
				   int r);

  /* Store a new register value into the thread context of TH.  */
  void (*store_inferior_register) (struct regcache *regcache,
				   windows_nat::windows_thread_info *th,
				   int r);

  void (*single_step) (windows_nat::windows_thread_info *th);

  const unsigned char *breakpoint;
  int breakpoint_len;

  /* Amount by which to decrement the PC after a breakpoint is
     hit.  */
  int decr_pc_after_break;

  /* Get the PC register from REGCACHE.  */
  CORE_ADDR (*get_pc) (struct regcache *regcache);
  /* Set the PC register in REGCACHE.  */
  void (*set_pc) (struct regcache *regcache, CORE_ADDR newpc);

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

/* Target ops definitions for a Win32 target.  */

class win32_process_target : public process_stratum_target
{
public:

  int create_inferior (const char *program,
		       const std::vector<char *> &program_args) override;

  int attach (unsigned long pid) override;

  int kill (process_info *proc) override;

  int detach (process_info *proc) override;

  void mourn (process_info *proc) override;

  void join (int pid) override;

  bool thread_alive (ptid_t pid) override;

  void resume (thread_resume *resume_info, size_t n) override;

  ptid_t wait (ptid_t ptid, target_waitstatus *status,
	       int options) override;

  void fetch_registers (regcache *regcache, int regno) override;

  void store_registers (regcache *regcache, int regno) override;

  int read_memory (CORE_ADDR memaddr, unsigned char *myaddr,
		   int len) override;

  int write_memory (CORE_ADDR memaddr, const unsigned char *myaddr,
		    int len) override;

  void request_interrupt () override;

  bool supports_z_point_type (char z_type) override;

  int insert_point (enum raw_bkpt_type type, CORE_ADDR addr,
		    int size, raw_breakpoint *bp) override;

  int remove_point (enum raw_bkpt_type type, CORE_ADDR addr,
		    int size, raw_breakpoint *bp) override;

  bool supports_hardware_single_step () override;

  bool stopped_by_watchpoint () override;

  CORE_ADDR stopped_data_address () override;

#ifdef _WIN32_WCE
  void hostio_last_error (char *buf) override;
#endif

  bool supports_qxfer_siginfo () override;

  int qxfer_siginfo (const char *annex, unsigned char *readbuf,
		     unsigned const char *writebuf,
		     CORE_ADDR offset, int len) override;

  bool supports_get_tib_address () override;

  int get_tib_address (ptid_t ptid, CORE_ADDR *addr) override;

  const gdb_byte *sw_breakpoint_from_kind (int kind, int *size) override;

  CORE_ADDR read_pc (regcache *regcache) override;

  void write_pc (regcache *regcache, CORE_ADDR pc) override;

  bool stopped_by_sw_breakpoint () override;

  bool supports_stopped_by_sw_breakpoint () override;
};

/* Retrieve the context for this thread, if not already retrieved.  */
extern void win32_require_context (windows_nat::windows_thread_info *th);

/* Map the Windows error number in ERROR to a locale-dependent error
   message string and return a pointer to it.  Typically, the values
   for ERROR come from GetLastError.

   The string pointed to shall not be modified by the application,
   but may be overwritten by a subsequent call to strwinerror

   The strwinerror function does not change the current setting
   of GetLastError.  */
extern char * strwinerror (DWORD error);

#endif /* GDBSERVER_WIN32_LOW_H */
