/* GNU/Linux/m32r specific low level interface, for the remote server for GDB.
   Copyright (C) 2005-2015 Free Software Foundation, Inc.

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

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

/* Defined in auto-generated file reg-m32r.c.  */
void init_registers_m32r (void);
extern const struct target_desc *tdesc_m32r;

#define m32r_num_regs 25

static int m32r_regmap[] = {
#ifdef PT_R0
  PT_R0, PT_R1, PT_R2, PT_R3, PT_R4, PT_R5, PT_R6, PT_R7,
  PT_R8, PT_R9, PT_R10, PT_R11, PT_R12, PT_FP, PT_LR, PT_SPU,
  PT_PSW, PT_CBR, PT_SPI, PT_SPU, PT_BPC, PT_PC, PT_ACCL, PT_ACCH, PT_EVB
#else
  4 * 4, 4 * 5, 4 * 6, 4 * 7, 4 * 0, 4 * 1, 4 * 2, 4 * 8,
  4 * 9, 4 * 10, 4 * 11, 4 * 12, 4 * 13, 4 * 24, 4 * 25, 4 * 23,
  4 * 19, 4 * 31, 4 * 26, 4 * 23, 4 * 20, 4 * 30, 4 * 16, 4 * 15, 4 * 32
#endif
};

static int
m32r_cannot_store_register (int regno)
{
  return (regno >= m32r_num_regs);
}

static int
m32r_cannot_fetch_register (int regno)
{
  return (regno >= m32r_num_regs);
}

static CORE_ADDR
m32r_get_pc (struct regcache *regcache)
{
  unsigned long pc;
  collect_register_by_name (regcache, "pc", &pc);
  if (debug_threads)
    debug_printf ("stop pc is %08lx\n", pc);
  return pc;
}

static void
m32r_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  unsigned long newpc = pc;
  supply_register_by_name (regcache, "pc", &newpc);
}

static const unsigned short m32r_breakpoint = 0x10f1;
#define m32r_breakpoint_len 2

static int
m32r_breakpoint_at (CORE_ADDR where)
{
  unsigned short insn;

  (*the_target->read_memory) (where, (unsigned char *) &insn,
			      m32r_breakpoint_len);
  if (insn == m32r_breakpoint)
    return 1;

  /* If necessary, recognize more trap instructions here.  GDB only uses the
     one.  */
  return 0;
}

static void
m32r_arch_setup (void)
{
  current_process ()->tdesc = tdesc_m32r;
}

static struct usrregs_info m32r_usrregs_info =
  {
    m32r_num_regs,
    m32r_regmap,
  };

static struct regs_info regs_info =
  {
    NULL, /* regset_bitmap */
    &m32r_usrregs_info,
  };

static const struct regs_info *
m32r_regs_info (void)
{
  return &regs_info;
}

struct linux_target_ops the_low_target = {
  m32r_arch_setup,
  m32r_regs_info,
  m32r_cannot_fetch_register,
  m32r_cannot_store_register,
  NULL, /* fetch_register */
  m32r_get_pc,
  m32r_set_pc,
  (const unsigned char *) &m32r_breakpoint,
  m32r_breakpoint_len,
  NULL,
  0,
  m32r_breakpoint_at,
};

void
initialize_low_arch (void)
{
  init_registers_m32r ();
}
