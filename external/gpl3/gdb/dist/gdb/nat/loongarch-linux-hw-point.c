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
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "gdbsupport/break-common.h"
#include "gdbsupport/common-regcache.h"
#include "nat/linux-nat.h"
#include "loongarch-linux-hw-point.h"

#include <sys/uio.h>

/* The order in which <sys/ptrace.h> and <asm/ptrace.h> are included
   can be important.  <sys/ptrace.h> often declares various PTRACE_*
   enums.  <asm/ptrace.h> often defines preprocessor constants for
   these very same symbols.  When that's the case, build errors will
   result when <asm/ptrace.h> is included before <sys/ptrace.h>.  */

#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include "elf/common.h"

/* See loongarch-linux-hw-point.h  */

/* Helper for loongarch_notify_debug_reg_change.  Records the
   information about the change of one hardware breakpoint/watchpoint
   setting for the thread LWP.
   N.B.  The actual updating of hardware debug registers is not
   carried out until the moment the thread is resumed.  */

static int
loongarch_dr_change_callback (struct lwp_info *lwp, int is_watchpoint,
			      unsigned int idx)
{
  int tid = ptid_of_lwp (lwp).lwp ();
  struct arch_lwp_info *info = lwp_arch_private_info (lwp);
  dr_changed_t *dr_changed_ptr;
  dr_changed_t dr_changed;

  if (info == NULL)
    {
      info = XCNEW (struct arch_lwp_info);
      lwp_set_arch_private_info (lwp, info);
    }

  if (show_debug_regs)
    {
      debug_printf ("loongarch_dr_change_callback: \n\tOn entry:\n");
      debug_printf ("\ttid%d, dr_changed_bp=0x%s, "
		    "dr_changed_wp=0x%s\n", tid,
		    phex (info->dr_changed_bp, 8),
		    phex (info->dr_changed_wp, 8));
    }

  dr_changed_ptr = is_watchpoint ? &info->dr_changed_wp
		   : &info->dr_changed_bp;
  dr_changed = *dr_changed_ptr;

  gdb_assert (idx >= 0
	      && (idx <= (is_watchpoint ? loongarch_num_wp_regs
			  : loongarch_num_bp_regs)));

  /* The actual update is done later just before resuming the lwp,
     we just mark that one register pair needs updating.  */
  DR_MARK_N_CHANGED (dr_changed, idx);
  *dr_changed_ptr = dr_changed;

  /* If the lwp isn't stopped, force it to momentarily pause, so
     we can update its debug registers.  */
  if (!lwp_is_stopped (lwp))
    linux_stop_lwp (lwp);

  if (show_debug_regs)
    {
      debug_printf ("\tOn exit:\n\ttid%d, dr_changed_bp=0x%s, "
		    "dr_changed_wp=0x%s\n", tid,
		    phex (info->dr_changed_bp, 8),
		    phex (info->dr_changed_wp, 8));
    }

  return 0;
}

/* Notify each thread that their IDXth breakpoint/watchpoint register
   pair needs to be updated.  The message will be recorded in each
   thread's arch-specific data area, the actual updating will be done
   when the thread is resumed.  */

void
loongarch_notify_debug_reg_change (ptid_t ptid,
				 int is_watchpoint, unsigned int idx)
{
  ptid_t pid_ptid = ptid_t (ptid.pid ());

  iterate_over_lwps (pid_ptid, [=] (struct lwp_info *info)
			       {
				 return loongarch_dr_change_callback (info,
								      is_watchpoint,
								      idx);
			       });
}

/* Call ptrace to set the thread TID's hardware breakpoint/watchpoint
   registers with data from *STATE.  */

void
loongarch_linux_set_debug_regs (struct loongarch_debug_reg_state *state,
			      int tid, int watchpoint)
{
  int i, count;
  struct iovec iov;
  struct loongarch_user_watch_state regs;
  const CORE_ADDR *addr;
  const unsigned int *ctrl;

  memset (&regs, 0, sizeof (regs));
  iov.iov_base = &regs;
  count = watchpoint ? loongarch_num_wp_regs : loongarch_num_bp_regs;
  addr = watchpoint ? state->dr_addr_wp : state->dr_addr_bp;
  ctrl = watchpoint ? state->dr_ctrl_wp : state->dr_ctrl_bp;

  if (count == 0)
    return;

  iov.iov_len = (offsetof (struct loongarch_user_watch_state, dbg_regs)
		 + count * sizeof (regs.dbg_regs[0]));
  for (i = 0; i < count; i++)
    {
      regs.dbg_regs[i].addr = addr[i];
      regs.dbg_regs[i].ctrl = ctrl[i];
    }

  if (ptrace(PTRACE_SETREGSET, tid,
	     watchpoint ? NT_LOONGARCH_HW_WATCH : NT_LOONGARCH_HW_BREAK,
	     (void *) &iov))
    {
      if (errno == EINVAL)
	error (_("Invalid argument setting hardware debug registers"));
      else
	error (_("Unexpected error setting hardware debug registers"));
    }

}

/* Get the hardware debug register capacity information from the
   process represented by TID.  */

void
loongarch_linux_get_debug_reg_capacity (int tid)
{
  struct iovec iov;
  struct loongarch_user_watch_state dreg_state;
  int result;
  iov.iov_base = &dreg_state;
  iov.iov_len = sizeof (dreg_state);

  /* Get hardware watchpoint register info.  */
  result = ptrace (PTRACE_GETREGSET, tid, NT_LOONGARCH_HW_WATCH, &iov);

  if (result == 0)
    {
      loongarch_num_wp_regs = LOONGARCH_DEBUG_NUM_SLOTS (dreg_state.dbg_info);
      if (loongarch_num_wp_regs > LOONGARCH_HWP_MAX_NUM)
	{
	  warning (_("Unexpected number of hardware watchpoint registers"
		     " reported by ptrace, got %d, expected %d."),
		   loongarch_num_wp_regs, LOONGARCH_HWP_MAX_NUM);
	  loongarch_num_wp_regs = LOONGARCH_HWP_MAX_NUM;
	}
    }
  else
    {
      warning (_("Unable to determine the number of hardware watchpoints"
		 " available."));
      loongarch_num_wp_regs = 0;
    }

  /* Get hardware breakpoint register info.  */
  result = ptrace (PTRACE_GETREGSET, tid, NT_LOONGARCH_HW_BREAK, &iov);
  if ( result == 0)
    {
      loongarch_num_bp_regs = LOONGARCH_DEBUG_NUM_SLOTS (dreg_state.dbg_info);
      if (loongarch_num_bp_regs > LOONGARCH_HBP_MAX_NUM)
	{
	  warning (_("Unexpected number of hardware breakpoint registers"
		     " reported by ptrace, got %d, expected %d."),
		   loongarch_num_bp_regs, LOONGARCH_HBP_MAX_NUM);
	  loongarch_num_bp_regs = LOONGARCH_HBP_MAX_NUM;
	}
    }
  else
    {
      warning (_("Unable to determine the number of hardware breakpoints"
		 " available."));
      loongarch_num_bp_regs = 0;
    }
}
