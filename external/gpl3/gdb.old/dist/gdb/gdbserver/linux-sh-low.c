/* GNU/Linux/SH specific low level interface, for the remote server for GDB.
   Copyright (C) 1995-2016 Free Software Foundation, Inc.

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

/* Defined in auto-generated file reg-sh.c.  */
void init_registers_sh (void);
extern const struct target_desc *tdesc_sh;

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#include <asm/ptrace.h>

#define sh_num_regs 41

/* Currently, don't check/send MQ.  */
static int sh_regmap[] = {
 0,	4,	8,	12,	16,	20,	24,	28,
 32,	36,	40,	44,	48,	52,	56,	60,

 REG_PC*4,   REG_PR*4,   REG_GBR*4,  -1,
 REG_MACH*4, REG_MACL*4, REG_SR*4,
 REG_FPUL*4, REG_FPSCR*4,

 REG_FPREG0*4+0,   REG_FPREG0*4+4,   REG_FPREG0*4+8,   REG_FPREG0*4+12,
 REG_FPREG0*4+16,  REG_FPREG0*4+20,  REG_FPREG0*4+24,  REG_FPREG0*4+28,
 REG_FPREG0*4+32,  REG_FPREG0*4+36,  REG_FPREG0*4+40,  REG_FPREG0*4+44,
 REG_FPREG0*4+48,  REG_FPREG0*4+52,  REG_FPREG0*4+56,  REG_FPREG0*4+60,
};

static int
sh_cannot_store_register (int regno)
{
  return 0;
}

static int
sh_cannot_fetch_register (int regno)
{
  return 0;
}

/* Correct in either endianness, obviously.  */
static const unsigned short sh_breakpoint = 0xc3c3;
#define sh_breakpoint_len 2

/* Implementation of linux_target_ops method "sw_breakpoint_from_kind".  */

static const gdb_byte *
sh_sw_breakpoint_from_kind (int kind, int *size)
{
  *size = sh_breakpoint_len;
  return (const gdb_byte *) &sh_breakpoint;
}

static int
sh_breakpoint_at (CORE_ADDR where)
{
  unsigned short insn;

  (*the_target->read_memory) (where, (unsigned char *) &insn, 2);
  if (insn == sh_breakpoint)
    return 1;

  /* If necessary, recognize more trap instructions here.  GDB only uses the
     one.  */
  return 0;
}

/* Support for hardware single step.  */

static int
sh_supports_hardware_single_step (void)
{
  return 1;
}

/* Provide only a fill function for the general register set.  ps_lgetregs
   will use this for NPTL support.  */

static void sh_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;

  for (i = 0; i < 23; i++)
    if (sh_regmap[i] != -1)
      collect_register (regcache, i, (char *) buf + sh_regmap[i]);
}

static struct regset_info sh_regsets[] = {
  { 0, 0, 0, 0, GENERAL_REGS, sh_fill_gregset, NULL },
  NULL_REGSET
};

static struct regsets_info sh_regsets_info =
  {
    sh_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct usrregs_info sh_usrregs_info =
  {
    sh_num_regs,
    sh_regmap,
  };

static struct regs_info regs_info =
  {
    NULL, /* regset_bitmap */
    &sh_usrregs_info,
    &sh_regsets_info
  };

static const struct regs_info *
sh_regs_info (void)
{
  return &regs_info;
}

static void
sh_arch_setup (void)
{
  current_process ()->tdesc = tdesc_sh;
}

struct linux_target_ops the_low_target = {
  sh_arch_setup,
  sh_regs_info,
  sh_cannot_fetch_register,
  sh_cannot_store_register,
  NULL, /* fetch_register */
  linux_get_pc_32bit,
  linux_set_pc_32bit,
  NULL, /* breakpoint_kind_from_pc */
  sh_sw_breakpoint_from_kind,
  NULL,
  0,
  sh_breakpoint_at,
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
  sh_supports_hardware_single_step,
};

void
initialize_low_arch (void)
{
  init_registers_sh ();

  initialize_regsets_info (&sh_regsets_info);
}
