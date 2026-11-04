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

#ifndef GDB_NAT_LOONGARCH_HW_POINT_H
#define GDB_NAT_LOONGARCH_HW_POINT_H

/* Macro definitions, data structures, and code for the hardware
   breakpoint and hardware watchpoint support follow.  We use the
   following abbreviations throughout the code:

   hw - hardware
   bp - breakpoint
   wp - watchpoint  */

/* Maximum number of hardware breakpoint and watchpoint registers.
   Neither of these values may exceed the width of dr_changed_t
   measured in bits.  */

#define LOONGARCH_HBP_MAX_NUM 14
#define LOONGARCH_HWP_MAX_NUM 14


/* The maximum length of a memory region that can be watched by one
   hardware watchpoint register.  */

#define LOONGARCH_HWP_MAX_LEN_PER_REG 8
#define CTRL_PLV3_ENABLE		0x10

#define DR_CONTROL_ENABLED(ctrl)  ((ctrl & CTRL_PLV3_ENABLE) == CTRL_PLV3_ENABLE)

/* Structure for managing the hardware breakpoint/watchpoint resources.
   DR_ADDR_* stores the address, DR_CTRL_* stores the control register
   content, and DR_REF_COUNT_* counts the numbers of references to the
   corresponding bp/wp, by which way the limited hardware resources
   are not wasted on duplicated bp/wp settings (though so far gdb has
   done a good job by not sending duplicated bp/wp requests).  */

struct loongarch_debug_reg_state
{
  /* hardware breakpoint */
  CORE_ADDR dr_addr_bp[LOONGARCH_HBP_MAX_NUM];
  unsigned int dr_ctrl_bp[LOONGARCH_HBP_MAX_NUM];
  unsigned int dr_ref_count_bp[LOONGARCH_HBP_MAX_NUM];

  /* hardware watchpoint */
  CORE_ADDR dr_addr_wp[LOONGARCH_HWP_MAX_NUM];
  unsigned int dr_ctrl_wp[LOONGARCH_HWP_MAX_NUM];
  unsigned int dr_ref_count_wp[LOONGARCH_HWP_MAX_NUM];
};

extern int loongarch_num_bp_regs;
extern int loongarch_num_wp_regs;

/* Invoked when IDXth breakpoint/watchpoint register pair needs to be
   updated.  */

void loongarch_notify_debug_reg_change (ptid_t ptid, int is_watchpoint,
				      unsigned int idx);


int loongarch_handle_breakpoint (enum target_hw_bp_type type, CORE_ADDR addr,
				 int len, int is_insert, ptid_t ptid,
				 struct loongarch_debug_reg_state *state);

int loongarch_handle_watchpoint (enum target_hw_bp_type type, CORE_ADDR addr,
				 int len, int is_insert, ptid_t ptid,
				 struct loongarch_debug_reg_state *state);

/* Return TRUE if there are any hardware breakpoints.  If WATCHPOINT is TRUE,
   check hardware watchpoints instead.  */

bool loongarch_any_set_debug_regs_state (loongarch_debug_reg_state *state,
					 bool watchpoint);

/* Print the values of the cached breakpoint/watchpoint registers.  */

void loongarch_show_debug_reg_state (struct loongarch_debug_reg_state *state,
				     const char *func, CORE_ADDR addr,
				     int len, enum target_hw_bp_type type);

/* Return true if we can watch a memory region that starts address
   ADDR and whose length is LEN in bytes.  */

int loongarch_region_ok_for_watchpoint (CORE_ADDR addr, int len);

/* Helper for the "stopped_data_address/low_stopped_data_address" target
   method.  Returns TRUE if a hardware watchpoint trap at ADDR_TRAP matches
   a set watchpoint.  The address of the matched watchpoint is returned in
   *ADDR_P.  */

bool loongarch_stopped_data_address (const struct loongarch_debug_reg_state *state,
				     CORE_ADDR addr_trap, CORE_ADDR *addr_p);

#endif /* GDB_NAT_LOONGARCH_HW_POINT_H */
