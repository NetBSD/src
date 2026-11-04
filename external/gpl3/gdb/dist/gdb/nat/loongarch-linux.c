/* Native-dependent code for GNU/Linux on LoongArch processors.

   Copyright (C) 2024-2025 Free Software Foundation, Inc.
   Contributed by Loongson Ltd.

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

#include "gdbsupport/break-common.h"
#include "nat/linux-nat.h"
#include "nat/loongarch-linux-hw-point.h"
#include "nat/loongarch-linux.h"

#include "elf/common.h"
#include "nat/gdb_ptrace.h"
#include <asm/ptrace.h>
#include <sys/uio.h>

/* Called when resuming a thread LWP.
   The hardware debug registers are updated when there is any change.  */

void
loongarch_linux_prepare_to_resume (struct lwp_info *lwp)
{
  struct arch_lwp_info *info = lwp_arch_private_info (lwp);

  /* NULL means this is the main thread still going through the shell,
     or, no watchpoint has been set yet.  In that case, there's
     nothing to do.  */
  if (info == NULL)
    return;

  if (DR_HAS_CHANGED (info->dr_changed_bp)
      || DR_HAS_CHANGED (info->dr_changed_wp))
    {
      ptid_t ptid = ptid_of_lwp (lwp);
      int tid = ptid.lwp ();
      struct loongarch_debug_reg_state *state
	= loongarch_get_debug_reg_state (ptid.pid ());

      if (show_debug_regs)
	debug_printf ("prepare_to_resume thread %d\n", tid);

      /* Watchpoints.  */
      if (DR_HAS_CHANGED (info->dr_changed_wp))
	{
	  loongarch_linux_set_debug_regs (state, tid, 1);
	  DR_CLEAR_CHANGED (info->dr_changed_wp);
	}

      /* Breakpoints.  */
      if (DR_HAS_CHANGED (info->dr_changed_bp))
	{
	  loongarch_linux_set_debug_regs (state, tid, 0);
	  DR_CLEAR_CHANGED (info->dr_changed_bp);
	}
    }
}

/* Function to call when a new thread is detected.  */

void
loongarch_linux_new_thread (struct lwp_info *lwp)
{
  ptid_t ptid = ptid_of_lwp (lwp);
  struct loongarch_debug_reg_state *state
	= loongarch_get_debug_reg_state (ptid.pid ());
  struct arch_lwp_info *info = XCNEW (struct arch_lwp_info);

  /* If there are hardware breakpoints/watchpoints in the process then mark that
     all the hardware breakpoint/watchpoint register pairs for this thread need
     to be initialized (with data from arch_process_info.debug_reg_state).  */
  if (loongarch_any_set_debug_regs_state (state, false))
    DR_MARK_ALL_CHANGED (info->dr_changed_bp, loongarch_num_bp_regs);
  if (loongarch_any_set_debug_regs_state (state, true))
    DR_MARK_ALL_CHANGED (info->dr_changed_wp, loongarch_num_wp_regs);

  lwp_set_arch_private_info (lwp, info);
}

/* See nat/loongarch-linux.h.  */

void
loongarch_linux_delete_thread (struct arch_lwp_info *arch_lwp)
{
  xfree (arch_lwp);
}
