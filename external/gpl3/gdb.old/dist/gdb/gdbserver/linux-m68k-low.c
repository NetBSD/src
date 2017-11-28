/* GNU/Linux/m68k specific low level interface, for the remote server for GDB.
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

/* Defined in auto-generated file reg-m68k.c.  */
void init_registers_m68k (void);
extern const struct target_desc *tdesc_m68k;

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#define m68k_num_regs 29
#define m68k_num_gregs 18

/* This table must line up with REGISTER_NAMES in tm-m68k.h */
static int m68k_regmap[] =
{
#ifdef PT_D0
  PT_D0 * 4, PT_D1 * 4, PT_D2 * 4, PT_D3 * 4,
  PT_D4 * 4, PT_D5 * 4, PT_D6 * 4, PT_D7 * 4,
  PT_A0 * 4, PT_A1 * 4, PT_A2 * 4, PT_A3 * 4,
  PT_A4 * 4, PT_A5 * 4, PT_A6 * 4, PT_USP * 4,
  PT_SR * 4, PT_PC * 4,
#else
  14 * 4, 0 * 4, 1 * 4, 2 * 4, 3 * 4, 4 * 4, 5 * 4, 6 * 4,
  7 * 4, 8 * 4, 9 * 4, 10 * 4, 11 * 4, 12 * 4, 13 * 4, 15 * 4,
  17 * 4, 18 * 4,
#endif
#ifdef PT_FP0
  PT_FP0 * 4, PT_FP1 * 4, PT_FP2 * 4, PT_FP3 * 4,
  PT_FP4 * 4, PT_FP5 * 4, PT_FP6 * 4, PT_FP7 * 4,
  PT_FPCR * 4, PT_FPSR * 4, PT_FPIAR * 4
#else
  21 * 4, 24 * 4, 27 * 4, 30 * 4, 33 * 4, 36 * 4,
  39 * 4, 42 * 4, 45 * 4, 46 * 4, 47 * 4
#endif
};

static int
m68k_cannot_store_register (int regno)
{
  return (regno >= m68k_num_regs);
}

static int
m68k_cannot_fetch_register (int regno)
{
  return (regno >= m68k_num_regs);
}

#ifdef HAVE_PTRACE_GETREGS
#include <sys/procfs.h>
#include "nat/gdb_ptrace.h"

static void
m68k_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;

  for (i = 0; i < m68k_num_gregs; i++)
    collect_register (regcache, i, (char *) buf + m68k_regmap[i]);
}

static void
m68k_store_gregset (struct regcache *regcache, const void *buf)
{
  int i;

  for (i = 0; i < m68k_num_gregs; i++)
    supply_register (regcache, i, (const char *) buf + m68k_regmap[i]);
}

static void
m68k_fill_fpregset (struct regcache *regcache, void *buf)
{
  int i;

  for (i = m68k_num_gregs; i < m68k_num_regs; i++)
    collect_register (regcache, i, ((char *) buf
			 + (m68k_regmap[i] - m68k_regmap[m68k_num_gregs])));
}

static void
m68k_store_fpregset (struct regcache *regcache, const void *buf)
{
  int i;

  for (i = m68k_num_gregs; i < m68k_num_regs; i++)
    supply_register (regcache, i, ((const char *) buf
			 + (m68k_regmap[i] - m68k_regmap[m68k_num_gregs])));
}

#endif /* HAVE_PTRACE_GETREGS */

static struct regset_info m68k_regsets[] = {
#ifdef HAVE_PTRACE_GETREGS
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, sizeof (elf_gregset_t),
    GENERAL_REGS,
    m68k_fill_gregset, m68k_store_gregset },
  { PTRACE_GETFPREGS, PTRACE_SETFPREGS, 0, sizeof (elf_fpregset_t),
    FP_REGS,
    m68k_fill_fpregset, m68k_store_fpregset },
#endif /* HAVE_PTRACE_GETREGS */
  NULL_REGSET
};

static const gdb_byte m68k_breakpoint[] = { 0x4E, 0x4F };
#define m68k_breakpoint_len 2

/* Implementation of linux_target_ops method "sw_breakpoint_from_kind".  */

static const gdb_byte *
m68k_sw_breakpoint_from_kind (int kind, int *size)
{
  *size = m68k_breakpoint_len;
  return m68k_breakpoint;
}

static int
m68k_breakpoint_at (CORE_ADDR pc)
{
  unsigned char c[2];

  read_inferior_memory (pc, c, 2);
  if (c[0] == 0x4E && c[1] == 0x4F)
    return 1;

  return 0;
}

#include <asm/ptrace.h>

#ifdef PTRACE_GET_THREAD_AREA
/* Fetch the thread-local storage pointer for libthread_db.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  if (ptrace (PTRACE_GET_THREAD_AREA, lwpid, NULL, base) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) ((char *)*base - idx);

  return PS_OK;
}
#endif /* PTRACE_GET_THREAD_AREA */

static struct regsets_info m68k_regsets_info =
  {
    m68k_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct usrregs_info m68k_usrregs_info =
  {
    m68k_num_regs,
    m68k_regmap,
  };

static struct regs_info regs_info =
  {
    NULL, /* regset_bitmap */
    &m68k_usrregs_info,
    &m68k_regsets_info
  };

static const struct regs_info *
m68k_regs_info (void)
{
  return &regs_info;
}

static void
m68k_arch_setup (void)
{
  current_process ()->tdesc = tdesc_m68k;
}

struct linux_target_ops the_low_target = {
  m68k_arch_setup,
  m68k_regs_info,
  m68k_cannot_fetch_register,
  m68k_cannot_store_register,
  NULL, /* fetch_register */
  linux_get_pc_32bit,
  linux_set_pc_32bit,
  NULL, /* breakpoint_kind_from_pc */
  m68k_sw_breakpoint_from_kind,
  NULL,
  2,
  m68k_breakpoint_at,
};

void
initialize_low_arch (void)
{
  /* Initialize the Linux target descriptions.  */
  init_registers_m68k ();

  initialize_regsets_info (&m68k_regsets_info);
}
