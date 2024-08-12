/* Native-dependent code for AArch64.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

#include "gdbarch.h"
#include "inferior.h"
#include "cli/cli-cmds.h"
#include "aarch64-nat.h"

#include <unordered_map>

/* Hash table storing per-process data.  We don't bind this to a
   per-inferior registry because of targets like x86 GNU/Linux that
   need to keep track of processes that aren't bound to any inferior
   (e.g., fork children, checkpoints).  */

static std::unordered_map<pid_t, aarch64_debug_reg_state>
aarch64_debug_process_state;

/* See aarch64-nat.h.  */

struct aarch64_debug_reg_state *
aarch64_lookup_debug_reg_state (pid_t pid)
{
  auto it = aarch64_debug_process_state.find (pid);
  if (it != aarch64_debug_process_state.end ())
    return &it->second;

  return nullptr;
}

/* See aarch64-nat.h.  */

struct aarch64_debug_reg_state *
aarch64_get_debug_reg_state (pid_t pid)
{
  return &aarch64_debug_process_state[pid];
}

/* See aarch64-nat.h.  */

void
aarch64_remove_debug_reg_state (pid_t pid)
{
  aarch64_debug_process_state.erase (pid);
}

/* Returns the number of hardware watchpoints of type TYPE that we can
   set.  Value is positive if we can set CNT watchpoints, zero if
   setting watchpoints of type TYPE is not supported, and negative if
   CNT is more than the maximum number of watchpoints of type TYPE
   that we can support.  TYPE is one of bp_hardware_watchpoint,
   bp_read_watchpoint, bp_write_watchpoint, or bp_hardware_breakpoint.
   CNT is the number of such watchpoints used so far (including this
   one).  OTHERTYPE is non-zero if other types of watchpoints are
   currently enabled.  */

int
aarch64_can_use_hw_breakpoint (enum bptype type, int cnt, int othertype)
{
  if (type == bp_hardware_watchpoint || type == bp_read_watchpoint
      || type == bp_access_watchpoint || type == bp_watchpoint)
    {
      if (aarch64_num_wp_regs == 0)
	return 0;
    }
  else if (type == bp_hardware_breakpoint)
    {
      if (aarch64_num_bp_regs == 0)
	return 0;
    }
  else
    gdb_assert_not_reached ("unexpected breakpoint type");

  /* We always return 1 here because we don't have enough information
     about possible overlap of addresses that they want to watch.  As an
     extreme example, consider the case where all the watchpoints watch
     the same address and the same region length: then we can handle a
     virtually unlimited number of watchpoints, due to debug register
     sharing implemented via reference counts.  */
  return 1;
}

/* Insert a hardware-assisted breakpoint at BP_TGT->reqstd_address.
   Return 0 on success, -1 on failure.  */

int
aarch64_insert_hw_breakpoint (struct gdbarch *gdbarch,
			      struct bp_target_info *bp_tgt)
{
  int ret;
  CORE_ADDR addr = bp_tgt->placed_address = bp_tgt->reqstd_address;
  int len;
  const enum target_hw_bp_type type = hw_execute;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (inferior_ptid.pid ());

  gdbarch_breakpoint_from_pc (gdbarch, &addr, &len);

  if (show_debug_regs)
    gdb_printf (gdb_stdlog,
		"insert_hw_breakpoint on entry (addr=0x%08lx, len=%d))\n",
		(unsigned long) addr, len);

  ret = aarch64_handle_breakpoint (type, addr, len, 1 /* is_insert */,
				   inferior_ptid, state);

  if (show_debug_regs)
    {
      aarch64_show_debug_reg_state (state,
				    "insert_hw_breakpoint", addr, len, type);
    }

  return ret;
}

/* Remove a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, -1 on failure.  */

int
aarch64_remove_hw_breakpoint (struct gdbarch *gdbarch,
			      struct bp_target_info *bp_tgt)
{
  int ret;
  CORE_ADDR addr = bp_tgt->placed_address;
  int len = 4;
  const enum target_hw_bp_type type = hw_execute;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (inferior_ptid.pid ());

  gdbarch_breakpoint_from_pc (gdbarch, &addr, &len);

  if (show_debug_regs)
    gdb_printf (gdb_stdlog,
		"remove_hw_breakpoint on entry (addr=0x%08lx, len=%d))\n",
		(unsigned long) addr, len);

  ret = aarch64_handle_breakpoint (type, addr, len, 0 /* is_insert */,
				   inferior_ptid, state);

  if (show_debug_regs)
    {
      aarch64_show_debug_reg_state (state,
				    "remove_hw_watchpoint", addr, len, type);
    }

  return ret;
}

/* Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE.  Return 0 on success, -1 on failure.  */

int
aarch64_insert_watchpoint (CORE_ADDR addr, int len, enum target_hw_bp_type type,
			   struct expression *cond)
{
  int ret;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (inferior_ptid.pid ());

  if (show_debug_regs)
    gdb_printf (gdb_stdlog,
		"insert_watchpoint on entry (addr=0x%08lx, len=%d)\n",
		(unsigned long) addr, len);

  gdb_assert (type != hw_execute);

  ret = aarch64_handle_watchpoint (type, addr, len, 1 /* is_insert */,
				   inferior_ptid, state);

  if (show_debug_regs)
    {
      aarch64_show_debug_reg_state (state,
				    "insert_watchpoint", addr, len, type);
    }

  return ret;
}

/* Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE.  Return 0 on success, -1 on failure.  */

int
aarch64_remove_watchpoint (CORE_ADDR addr, int len, enum target_hw_bp_type type,
			   struct expression *cond)
{
  int ret;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (inferior_ptid.pid ());

  if (show_debug_regs)
    gdb_printf (gdb_stdlog,
		"remove_watchpoint on entry (addr=0x%08lx, len=%d)\n",
		(unsigned long) addr, len);

  gdb_assert (type != hw_execute);

  ret = aarch64_handle_watchpoint (type, addr, len, 0 /* is_insert */,
				   inferior_ptid, state);

  if (show_debug_regs)
    {
      aarch64_show_debug_reg_state (state,
				    "remove_watchpoint", addr, len, type);
    }

  return ret;
}

/* Define AArch64 maintenance commands.  */

static void
add_show_debug_regs_command (void)
{
  /* A maintenance command to enable printing the internal DRi mirror
     variables.  */
  add_setshow_boolean_cmd ("show-debug-regs", class_maintenance,
			   &show_debug_regs, _("\
Set whether to show variables that mirror the AArch64 debug registers."), _("\
Show whether to show variables that mirror the AArch64 debug registers."), _("\
Use \"on\" to enable, \"off\" to disable.\n\
If enabled, the debug registers values are shown when GDB inserts\n\
or removes a hardware breakpoint or watchpoint, and when the inferior\n\
triggers a breakpoint or watchpoint."),
			   NULL,
			   NULL,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);
}

void
aarch64_initialize_hw_point ()
{
  add_show_debug_regs_command ();
}
