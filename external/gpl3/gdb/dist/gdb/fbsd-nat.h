/* Native-dependent code for FreeBSD.

   Copyright (C) 2004-2020 Free Software Foundation, Inc.

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

#ifndef FBSD_NAT_H
#define FBSD_NAT_H

#include "inf-ptrace.h"
#include <osreldate.h>
#include <sys/proc.h>

/* FreeBSD kernels 11.3 and later report valid si_code values for
   SIGTRAP on all architectures.  Older FreeBSD kernels that supported
   TRAP_BRKPT did not report valid values for MIPS and sparc64.  Even
   older kernels without TRAP_BRKPT support did not report valid
   values on any architecture.  */
#if (__FreeBSD_kernel_version >= 1102502) || (__FreeBSD_version >= 1102502)
# define USE_SIGTRAP_SIGINFO
#elif defined(TRAP_BRKPT)
# if !defined(__mips__) && !defined(__sparc64__)
#  define USE_SIGTRAP_SIGINFO
# endif
#endif

/* A prototype FreeBSD target.  */

class fbsd_nat_target : public inf_ptrace_target
{
public:
  char *pid_to_exec_file (int pid) override;

  int find_memory_regions (find_memory_region_ftype func, void *data) override;

  bool info_proc (const char *, enum info_proc_what) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

#ifdef PT_LWPINFO
  bool thread_alive (ptid_t ptid) override;
  std::string pid_to_str (ptid_t) override;

#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_TDNAME
  const char *thread_name (struct thread_info *) override;
#endif

  void update_thread_list () override;

  thread_control_capabilities get_thread_control_capabilities () override
  { return tc_schedlock; }

  void resume (ptid_t, int, enum gdb_signal) override;

  ptid_t wait (ptid_t, struct target_waitstatus *, int) override;

  void post_startup_inferior (ptid_t) override;
  void post_attach (int) override;

#ifdef USE_SIGTRAP_SIGINFO
  bool supports_stopped_by_sw_breakpoint () override;
  bool stopped_by_sw_breakpoint () override;
#endif

#ifdef TDP_RFPPWAIT
  bool follow_fork (bool, bool) override;

  int insert_fork_catchpoint (int) override;
  int remove_fork_catchpoint (int) override;

  int insert_vfork_catchpoint (int) override;
  int remove_vfork_catchpoint (int) override;
#endif

#ifdef PL_FLAG_EXEC
  int insert_exec_catchpoint (int) override;
  int remove_exec_catchpoint (int) override;
#endif

#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_SYSCALL_CODE
  int set_syscall_catchpoint (int, bool, int, gdb::array_view<const int>)
    override;
#endif
#endif /* PT_LWPINFO */

  bool supports_multi_process () override;
};

#endif /* fbsd-nat.h */
