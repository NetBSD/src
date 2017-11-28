/* Native-dependent code for x86 (i386 and x86-64).

   Copyright (C) 2001-2016 Free Software Foundation, Inc.

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

#include "defs.h"
#include "x86-nat.h"
#include "gdbcmd.h"
#include "inferior.h"

/* Support for hardware watchpoints and breakpoints using the x86
   debug registers.

   This provides several functions for inserting and removing
   hardware-assisted breakpoints and watchpoints, testing if one or
   more of the watchpoints triggered and at what address, checking
   whether a given region can be watched, etc.

   The functions below implement debug registers sharing by reference
   counts, and allow to watch regions up to 16 bytes long.  */

/* Low-level function vector.  */
struct x86_dr_low_type x86_dr_low;

/* Per-process data.  We don't bind this to a per-inferior registry
   because of targets like x86 GNU/Linux that need to keep track of
   processes that aren't bound to any inferior (e.g., fork children,
   checkpoints).  */

struct x86_process_info
{
  /* Linked list.  */
  struct x86_process_info *next;

  /* The process identifier.  */
  pid_t pid;

  /* Copy of x86 hardware debug registers.  */
  struct x86_debug_reg_state state;
};

static struct x86_process_info *x86_process_list = NULL;

/* Find process data for process PID.  */

static struct x86_process_info *
x86_find_process_pid (pid_t pid)
{
  struct x86_process_info *proc;

  for (proc = x86_process_list; proc; proc = proc->next)
    if (proc->pid == pid)
      return proc;

  return NULL;
}

/* Add process data for process PID.  Returns newly allocated info
   object.  */

static struct x86_process_info *
x86_add_process (pid_t pid)
{
  struct x86_process_info *proc = XCNEW (struct x86_process_info);

  proc->pid = pid;
  proc->next = x86_process_list;
  x86_process_list = proc;

  return proc;
}

/* Get data specific info for process PID, creating it if necessary.
   Never returns NULL.  */

static struct x86_process_info *
x86_process_info_get (pid_t pid)
{
  struct x86_process_info *proc;

  proc = x86_find_process_pid (pid);
  if (proc == NULL)
    proc = x86_add_process (pid);

  return proc;
}

/* Get debug registers state for process PID.  */

struct x86_debug_reg_state *
x86_debug_reg_state (pid_t pid)
{
  return &x86_process_info_get (pid)->state;
}

/* See declaration in x86-nat.h.  */

void
x86_forget_process (pid_t pid)
{
  struct x86_process_info *proc, **proc_link;

  proc = x86_process_list;
  proc_link = &x86_process_list;

  while (proc != NULL)
    {
      if (proc->pid == pid)
	{
	  *proc_link = proc->next;

	  xfree (proc);
	  return;
	}

      proc_link = &proc->next;
      proc = *proc_link;
    }
}

/* Clear the reference counts and forget everything we knew about the
   debug registers.  */

void
x86_cleanup_dregs (void)
{
  /* Starting from scratch has the same effect.  */
  x86_forget_process (ptid_get_pid (inferior_ptid));
}

/* Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE.  Return 0 on success, -1 on failure.  */

static int
x86_insert_watchpoint (struct target_ops *self, CORE_ADDR addr, int len,
		       enum target_hw_bp_type type, struct expression *cond)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (ptid_get_pid (inferior_ptid));

  return x86_dr_insert_watchpoint (state, type, addr, len);
}

/* Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE.  Return 0 on success, -1 on failure.  */
static int
x86_remove_watchpoint (struct target_ops *self, CORE_ADDR addr, int len,
		       enum target_hw_bp_type type, struct expression *cond)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (ptid_get_pid (inferior_ptid));

  return x86_dr_remove_watchpoint (state, type, addr, len);
}

/* Return non-zero if we can watch a memory region that starts at
   address ADDR and whose length is LEN bytes.  */

static int
x86_region_ok_for_watchpoint (struct target_ops *self,
			      CORE_ADDR addr, int len)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (ptid_get_pid (inferior_ptid));

  return x86_dr_region_ok_for_watchpoint (state, addr, len);
}

/* If the inferior has some break/watchpoint that triggered, set the
   address associated with that break/watchpoint and return non-zero.
   Otherwise, return zero.  */

static int
x86_stopped_data_address (struct target_ops *ops, CORE_ADDR *addr_p)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (ptid_get_pid (inferior_ptid));

  return x86_dr_stopped_data_address (state, addr_p);
}

/* Return non-zero if the inferior has some watchpoint that triggered.
   Otherwise return zero.  */

static int
x86_stopped_by_watchpoint (struct target_ops *ops)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (ptid_get_pid (inferior_ptid));

  return x86_dr_stopped_by_watchpoint (state);
}

/* Insert a hardware-assisted breakpoint at BP_TGT->reqstd_address.
   Return 0 on success, EBUSY on failure.  */

static int
x86_insert_hw_breakpoint (struct target_ops *self, struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (ptid_get_pid (inferior_ptid));

  bp_tgt->placed_address = bp_tgt->reqstd_address;
  return x86_dr_insert_watchpoint (state, hw_execute,
				   bp_tgt->placed_address, 1) ? EBUSY : 0;
}

/* Remove a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, -1 on failure.  */

static int
x86_remove_hw_breakpoint (struct target_ops *self, struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (ptid_get_pid (inferior_ptid));

  return x86_dr_remove_watchpoint (state, hw_execute,
				   bp_tgt->placed_address, 1);
}

/* Returns the number of hardware watchpoints of type TYPE that we can
   set.  Value is positive if we can set CNT watchpoints, zero if
   setting watchpoints of type TYPE is not supported, and negative if
   CNT is more than the maximum number of watchpoints of type TYPE
   that we can support.  TYPE is one of bp_hardware_watchpoint,
   bp_read_watchpoint, bp_write_watchpoint, or bp_hardware_breakpoint.
   CNT is the number of such watchpoints used so far (including this
   one).  OTHERTYPE is non-zero if other types of watchpoints are
   currently enabled.

   We always return 1 here because we don't have enough information
   about possible overlap of addresses that they want to watch.  As an
   extreme example, consider the case where all the watchpoints watch
   the same address and the same region length: then we can handle a
   virtually unlimited number of watchpoints, due to debug register
   sharing implemented via reference counts in x86-nat.c.  */

static int
x86_can_use_hw_breakpoint (struct target_ops *self,
			   enum bptype type, int cnt, int othertype)
{
  return 1;
}

static void
add_show_debug_regs_command (void)
{
  /* A maintenance command to enable printing the internal DRi mirror
     variables.  */
  add_setshow_boolean_cmd ("show-debug-regs", class_maintenance,
			   &show_debug_regs, _("\
Set whether to show variables that mirror the x86 debug registers."), _("\
Show whether to show variables that mirror the x86 debug registers."), _("\
Use \"on\" to enable, \"off\" to disable.\n\
If enabled, the debug registers values are shown when GDB inserts\n\
or removes a hardware breakpoint or watchpoint, and when the inferior\n\
triggers a breakpoint or watchpoint."),
			   NULL,
			   NULL,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);
}

/* There are only two global functions left.  */

void
x86_use_watchpoints (struct target_ops *t)
{
  /* After a watchpoint trap, the PC points to the instruction after the
     one that caused the trap.  Therefore we don't need to step over it.
     But we do need to reset the status register to avoid another trap.  */
  t->to_have_continuable_watchpoint = 1;

  t->to_can_use_hw_breakpoint = x86_can_use_hw_breakpoint;
  t->to_region_ok_for_hw_watchpoint = x86_region_ok_for_watchpoint;
  t->to_stopped_by_watchpoint = x86_stopped_by_watchpoint;
  t->to_stopped_data_address = x86_stopped_data_address;
  t->to_insert_watchpoint = x86_insert_watchpoint;
  t->to_remove_watchpoint = x86_remove_watchpoint;
  t->to_insert_hw_breakpoint = x86_insert_hw_breakpoint;
  t->to_remove_hw_breakpoint = x86_remove_hw_breakpoint;
}

void
x86_set_debug_register_length (int len)
{
  /* This function should be called only once for each native target.  */
  gdb_assert (x86_dr_low.debug_register_length == 0);
  gdb_assert (len == 4 || len == 8);
  x86_dr_low.debug_register_length = len;
  add_show_debug_regs_command ();
}
