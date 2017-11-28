/* GNU/Linux/BFIN specific low level interface, for the remote server for GDB.

   Copyright (C) 2005-2017 Free Software Foundation, Inc.

   Contributed by Analog Devices, Inc.

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

#include "server.h"
#include "linux-low.h"
#include <asm/ptrace.h>

/* Defined in auto-generated file reg-bfin.c.  */
void init_registers_bfin (void);
extern const struct target_desc *tdesc_bfin;

static int bfin_regmap[] =
{
  PT_R0, PT_R1, PT_R2, PT_R3, PT_R4, PT_R5, PT_R6, PT_R7,
  PT_P0, PT_P1, PT_P2, PT_P3, PT_P4, PT_P5, PT_USP, PT_FP,
  PT_I0, PT_I1, PT_I2, PT_I3, PT_M0, PT_M1, PT_M2, PT_M3,
  PT_B0, PT_B1, PT_B2, PT_B3, PT_L0, PT_L1, PT_L2, PT_L3,
  PT_A0X, PT_A0W, PT_A1X, PT_A1W, PT_ASTAT, PT_RETS,
  PT_LC0, PT_LT0, PT_LB0, PT_LC1, PT_LT1, PT_LB1,
  -1 /* PT_CYCLES */, -1 /* PT_CYCLES2 */,
  -1 /* PT_USP */, PT_SEQSTAT, PT_SYSCFG, PT_PC, PT_RETX, PT_RETN, PT_RETE,
  PT_PC,
};

#define bfin_num_regs ARRAY_SIZE (bfin_regmap)

static int
bfin_cannot_store_register (int regno)
{
  return (regno >= bfin_num_regs);
}

static int
bfin_cannot_fetch_register (int regno)
{
  return (regno >= bfin_num_regs);
}

#define bfin_breakpoint_len 2
static const gdb_byte bfin_breakpoint[bfin_breakpoint_len] = {0xa1, 0x00};

/* Implementation of linux_target_ops method "sw_breakpoint_from_kind".  */

static const gdb_byte *
bfin_sw_breakpoint_from_kind (int kind, int *size)
{
  *size = bfin_breakpoint_len;
  return bfin_breakpoint;
}

static int
bfin_breakpoint_at (CORE_ADDR where)
{
  unsigned char insn[bfin_breakpoint_len];

  read_inferior_memory(where, insn, bfin_breakpoint_len);
  if (insn[0] == bfin_breakpoint[0]
      && insn[1] == bfin_breakpoint[1])
    return 1;

  /* If necessary, recognize more trap instructions here.  GDB only uses the
     one.  */
  return 0;
}

static void
bfin_arch_setup (void)
{
  current_process ()->tdesc = tdesc_bfin;
}

/* Support for hardware single step.  */

static int
bfin_supports_hardware_single_step (void)
{
  return 1;
}

static struct usrregs_info bfin_usrregs_info =
  {
    bfin_num_regs,
    bfin_regmap,
  };

static struct regs_info regs_info =
  {
    NULL, /* regset_bitmap */
    &bfin_usrregs_info,
  };

static const struct regs_info *
bfin_regs_info (void)
{
  return &regs_info;
}

struct linux_target_ops the_low_target = {
  bfin_arch_setup,
  bfin_regs_info,
  bfin_cannot_fetch_register,
  bfin_cannot_store_register,
  NULL, /* fetch_register */
  linux_get_pc_32bit,
  linux_set_pc_32bit,
  NULL, /* breakpoint_kind_from_pc */
  bfin_sw_breakpoint_from_kind,
  NULL, /* get_next_pcs */
  2,
  bfin_breakpoint_at,
  NULL, /* supports_z_point_type */
  NULL, /* insert_point */
  NULL, /* remove_point */
  NULL, /* stopped_by_watchpoint */
  NULL, /* stopped_data_address */
  NULL, /* collect_ptrace_register */
  NULL, /* supply_ptrace_register */
  NULL, /* siginfo_fixup */
  NULL, /* new_process */
  NULL, /* new_thread */
  NULL, /* new_fork */
  NULL, /* prepare_to_resume */
  NULL, /* process_qsupported */
  NULL, /* supports_tracepoints */
  NULL, /* get_thread_area */
  NULL, /* install_fast_tracepoint_jump_pad */
  NULL, /* emit_ops */
  NULL, /* get_min_fast_tracepoint_insn_len */
  NULL, /* supports_range_stepping */
  NULL, /* breakpoint_kind_from_current_state */
  bfin_supports_hardware_single_step,
};


void
initialize_low_arch (void)
{
  init_registers_bfin ();
}
