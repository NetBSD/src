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
#include "gdbsupport/common-regcache.h"
#include "loongarch-hw-point.h"
#include "loongarch-linux-hw-point.h"

/* Number of hardware breakpoints/watchpoints the target supports.
   They are initialized with values obtained via ptrace.  */

int loongarch_num_bp_regs;
int loongarch_num_wp_regs;

/* Given the hardware breakpoint or watchpoint type TYPE and its
   length LEN, return the expected encoding for a hardware
   breakpoint/watchpoint control register.  */

static unsigned int
loongarch_point_encode_ctrl_reg (enum target_hw_bp_type type, int len)
{
  unsigned int ctrl, ttype, llen;

  gdb_assert (len <= LOONGARCH_HWP_MAX_LEN_PER_REG);

  /* type */
  switch (type)
    {
    case hw_write:
      ttype = 2;
      break;
    case hw_read:
      ttype = 1;
      break;
    case hw_access:
      ttype = 3;
      break;
    case hw_execute:
      ttype = 0;
      break;
    default:
      perror_with_name (_("Unrecognized watchpoint type"));
    }

  /* len */
  switch (len)
    {
    case 1:
      llen = 0b11;
      break;
    case 2:
      llen = 0b10;
      break;
    case 4:
      llen = 0b01;
      break;
    case 8:
      llen = 0b00;
      break;
    default:
      perror_with_name (_("Unrecognized watchpoint length"));
    }
  ctrl = 0;
  if (type != hw_execute) {
    /*  type and length bitmask */
    ctrl |= llen << 10;
    ctrl |= ttype << 8;
  }
  ctrl |= CTRL_PLV3_ENABLE;
  return ctrl;
}


/* Record the insertion of one breakpoint/watchpoint, as represented
   by ADDR and CTRL, in the process' arch-specific data area *STATE.  */

static int
loongarch_dr_state_insert_one_point (ptid_t ptid,
				   struct loongarch_debug_reg_state *state,
				   enum target_hw_bp_type type, CORE_ADDR addr,
				   int len, CORE_ADDR addr_orig)
{
  int i, idx, num_regs, is_watchpoint;
  unsigned int ctrl, *dr_ctrl_p, *dr_ref_count;
  CORE_ADDR *dr_addr_p;

  /* Set up state pointers.  */
  is_watchpoint = (type != hw_execute);
  if (is_watchpoint)
    {
      num_regs = loongarch_num_wp_regs;
      dr_addr_p = state->dr_addr_wp;
      dr_ctrl_p = state->dr_ctrl_wp;
      dr_ref_count = state->dr_ref_count_wp;
    }
  else
    {
      num_regs = loongarch_num_bp_regs;
      dr_addr_p = state->dr_addr_bp;
      dr_ctrl_p = state->dr_ctrl_bp;
      dr_ref_count = state->dr_ref_count_bp;
    }

  ctrl = loongarch_point_encode_ctrl_reg (type, len);

  /* Find an existing or free register in our cache.  */
  idx = -1;
  for (i = 0; i < num_regs; ++i)
    {
      if ((dr_ctrl_p[i] & CTRL_PLV3_ENABLE) == 0) // PLV3 disable
	{
	  gdb_assert (dr_ref_count[i] == 0);
	  idx = i;
	  /* no break; continue hunting for an existing one.  */
	}
      else if (dr_addr_p[i] == addr && dr_ctrl_p[i] == ctrl)
	{
	  idx = i;
	  gdb_assert (dr_ref_count[i] != 0);
	  break;
	}

    }

  /* No space.  */
  if (idx == -1)
    return -1;

  /* Update our cache.  */
  if ((dr_ctrl_p[idx] & CTRL_PLV3_ENABLE) == 0)
    {
      /* new entry */
      dr_addr_p[idx] = addr;
      dr_ctrl_p[idx] = ctrl;
      dr_ref_count[idx] = 1;

      /* Notify the change.  */
      loongarch_notify_debug_reg_change (ptid, is_watchpoint, idx);
    }
  else
    {
      /* existing entry */
      dr_ref_count[idx]++;
    }

  return 0;
}

/* Record the removal of one breakpoint/watchpoint, as represented by
   ADDR and CTRL, in the process' arch-specific data area *STATE.  */

static int
loongarch_dr_state_remove_one_point (ptid_t ptid,
				   struct loongarch_debug_reg_state *state,
				   enum target_hw_bp_type type, CORE_ADDR addr,
				   int len, CORE_ADDR addr_orig)
{
  int i, num_regs, is_watchpoint;
  unsigned int ctrl, *dr_ctrl_p, *dr_ref_count;
  CORE_ADDR *dr_addr_p;

  /* Set up state pointers.  */
  is_watchpoint = (type != hw_execute);
  if (is_watchpoint)
    {
      num_regs = loongarch_num_wp_regs;
      dr_addr_p = state->dr_addr_wp;
      dr_ctrl_p = state->dr_ctrl_wp;
      dr_ref_count = state->dr_ref_count_wp;
    }
  else
    {
      num_regs = loongarch_num_bp_regs;
      dr_addr_p = state->dr_addr_bp;
      dr_ctrl_p = state->dr_ctrl_bp;
      dr_ref_count = state->dr_ref_count_bp;
    }

  ctrl = loongarch_point_encode_ctrl_reg (type, len);

  /* Find the entry that matches the ADDR and CTRL.  */
  for (i = 0; i < num_regs; ++i)
    if (dr_addr_p[i] == addr && dr_ctrl_p[i] == ctrl)
      {
	gdb_assert (dr_ref_count[i] != 0);
	break;
      }

  /* Not found.  */
  if (i == num_regs)
    return -1;

  /* Clear our cache.  */
  if (--dr_ref_count[i] == 0)
    {
      dr_addr_p[i] = 0;
      dr_ctrl_p[i] = 0;

      /* Notify the change.  */
      loongarch_notify_debug_reg_change (ptid, is_watchpoint, i);
    }

  return 0;
}

int
loongarch_handle_breakpoint (enum target_hw_bp_type type, CORE_ADDR addr,
			   int len, int is_insert, ptid_t ptid,
			   struct loongarch_debug_reg_state *state)
{
  if (is_insert)
    return loongarch_dr_state_insert_one_point (ptid, state, type, addr,
						len, -1);
  else
    return loongarch_dr_state_remove_one_point (ptid, state, type, addr,
						len, -1);
}


int
loongarch_handle_watchpoint (enum target_hw_bp_type type, CORE_ADDR addr,
			     int len, int is_insert, ptid_t ptid,
			     struct loongarch_debug_reg_state *state)
{
  if (is_insert)
    return loongarch_dr_state_insert_one_point (ptid, state, type, addr,
						len, addr);
  else
    return loongarch_dr_state_remove_one_point (ptid, state, type, addr,
						len, addr);
}


/* See nat/loongarch-hw-point.h.  */

bool
loongarch_any_set_debug_regs_state (loongarch_debug_reg_state *state,
				    bool watchpoint)
{
  int count = watchpoint ? loongarch_num_wp_regs : loongarch_num_bp_regs;
  if (count == 0)
    return false;

  const CORE_ADDR *addr = watchpoint ? state->dr_addr_wp : state->dr_addr_bp;
  const unsigned int *ctrl = watchpoint ? state->dr_ctrl_wp : state->dr_ctrl_bp;

  for (int i = 0; i < count; i++)
    if (addr[i] != 0 || ctrl[i] != 0)
      return true;

  return false;
}

/* Print the values of the cached breakpoint/watchpoint registers.  */

void
loongarch_show_debug_reg_state (struct loongarch_debug_reg_state *state,
				const char *func, CORE_ADDR addr,
				int len, enum target_hw_bp_type type)
{
  int i;

  debug_printf ("%s", func);
  if (addr || len)
    debug_printf (" (addr=0x%08lx, len=%d, type=%s)",
		  (unsigned long) addr, len,
		  type == hw_write ? "hw-write-watchpoint"
		  : (type == hw_read ? "hw-read-watchpoint"
		     : (type == hw_access ? "hw-access-watchpoint"
			: (type == hw_execute ? "hw-breakpoint"
			   : "??unknown??"))));
  debug_printf (":\n");

  debug_printf ("\tBREAKPOINTs:\n");
  for (i = 0; i < loongarch_num_bp_regs; i++)
    debug_printf ("\tBP%d: addr=%s, ctrl=0x%08x, ref.count=%d\n",
		  i, core_addr_to_string_nz (state->dr_addr_bp[i]),
		  state->dr_ctrl_bp[i], state->dr_ref_count_bp[i]);

  debug_printf ("\tWATCHPOINTs:\n");
  for (i = 0; i < loongarch_num_wp_regs; i++)
    debug_printf ("\tWP%d: addr=%s, ctrl=0x%08x, ref.count=%d\n",
		  i, core_addr_to_string_nz (state->dr_addr_wp[i]),
		  state->dr_ctrl_wp[i], state->dr_ref_count_wp[i]);
}

/* Return true if we can watch a memory region that starts address
   ADDR and whose length is LEN in bytes.  */

int
loongarch_region_ok_for_watchpoint (CORE_ADDR addr, int len)
{
  /* Can not set watchpoints for zero or negative lengths.  */
  if (len <= 0)
    return 0;

  /* Must have hardware watchpoint debug register(s).  */
  if (loongarch_num_wp_regs == 0)
    return 0;

  return 1;
}

/* See nat/loongarch-hw-point.h*/

bool
loongarch_stopped_data_address (const struct loongarch_debug_reg_state *state,
				CORE_ADDR addr_trap, CORE_ADDR *addr_p)
{
  int i;

  for (i = loongarch_num_wp_regs - 1; i >= 0; --i)
    {
      const CORE_ADDR addr_watch = state->dr_addr_wp[i];

      if (state->dr_ref_count_wp[i]
	  && DR_CONTROL_ENABLED (state->dr_ctrl_wp[i])
	  && addr_trap == addr_watch)
	{
	  *addr_p = addr_watch;
	  return true;
	}
    }

  return false;
}
