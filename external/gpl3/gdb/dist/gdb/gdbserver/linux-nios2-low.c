/* GNU/Linux/Nios II specific low level interface, for the remote server for
   GDB.
   Copyright (C) 2008-2014 Free Software Foundation, Inc.

   Contributed by Mentor Graphics, Inc.

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
#include <sys/ptrace.h>
#include <endian.h>
#include "gdb_proc_service.h"
#include <asm/ptrace.h>

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif

/* The following definition must agree with the number of registers
   defined in "struct user_regs" in GLIBC
   (ports/sysdeps/unix/sysv/linux/nios2/sys/user.h), and also with
   NIOS2_NUM_REGS in GDB proper.  */

#define nios2_num_regs 49

/* Defined in auto-generated file nios2-linux.c.  */

void init_registers_nios2_linux (void);
extern const struct target_desc *tdesc_nios2_linux;

/* This union is used to convert between int and byte buffer
   representations of register contents.  */

union nios2_register
{
  unsigned char buf[4];
  int reg32;
};

/* Return the ptrace ``address'' of register REGNO. */

static int nios2_regmap[] = {
  -1,  1,  2,  3,  4,  5,  6,  7,
  8,  9,  10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47,
  48,
  0
};

/* Implement the arch_setup linux_target_ops method.  */

static void
nios2_arch_setup (void)
{
  current_process ()->tdesc = tdesc_nios2_linux;
}

/* Implement the cannot_fetch_register linux_target_ops method.  */

static int
nios2_cannot_fetch_register (int regno)
{
  if (nios2_regmap[regno] == -1)
    return 1;

  return 0;
}

/* Implement the cannot_store_register linux_target_ops method.  */

static int
nios2_cannot_store_register (int regno)
{
  if (nios2_regmap[regno] == -1)
    return 1;

  return 0;
}

/* Implement the get_pc linux_target_ops method.  */

static CORE_ADDR
nios2_get_pc (struct regcache *regcache)
{
  union nios2_register pc;

  collect_register_by_name (regcache, "pc", pc.buf);
  return pc.reg32;
}

/* Implement the set_pc linux_target_ops method.  */

static void
nios2_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  union nios2_register newpc;

  newpc.reg32 = pc;
  supply_register_by_name (regcache, "pc", newpc.buf);
}

/* Breakpoint support.  */

static const unsigned int nios2_breakpoint = 0x003b6ffa;
#define nios2_breakpoint_len 4

/* Implement the breakpoint_reinsert_addr linux_target_ops method.  */

static CORE_ADDR
nios2_reinsert_addr (void)
{
  union nios2_register ra;
  struct regcache *regcache = get_thread_regcache (current_inferior, 1);

  collect_register_by_name (regcache, "ra", ra.buf);
  return ra.reg32;
}

/* Implement the breakpoint_at linux_target_ops method.  */

static int
nios2_breakpoint_at (CORE_ADDR where)
{
  unsigned int insn;

  (*the_target->read_memory) (where, (unsigned char *) &insn, 4);
  if (insn == nios2_breakpoint)
    return 1;
  return 0;
}

/* Fetch the thread-local storage pointer for libthread_db.  */

ps_err_e
ps_get_thread_area (const struct ps_prochandle *ph,
                    lwpid_t lwpid, int idx, void **base)
{
  if (ptrace (PTRACE_GET_THREAD_AREA, lwpid, NULL, base) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) ((char *) *base - idx);

  return PS_OK;
}

#ifdef HAVE_PTRACE_GETREGS

/* Helper functions to collect/supply a single register REGNO.  */

static void
nios2_collect_register (struct regcache *regcache, int regno,
			union nios2_register *reg)
{
  union nios2_register tmp_reg;

  collect_register (regcache, regno, &tmp_reg.reg32);
  reg->reg32 = tmp_reg.reg32;
}

static void
nios2_supply_register (struct regcache *regcache, int regno,
		       const union nios2_register *reg)
{
  supply_register (regcache, regno, reg->buf);
}

/* We have only a single register set on Nios II.  */

static void
nios2_fill_gregset (struct regcache *regcache, void *buf)
{
  union nios2_register *regset = buf;
  int i;

  for (i = 1; i < nios2_num_regs; i++)
    nios2_collect_register (regcache, i, regset + i);
}

static void
nios2_store_gregset (struct regcache *regcache, const void *buf)
{
  const union nios2_register *regset = buf;
  int i;

  for (i = 0; i < nios2_num_regs; i++)
    nios2_supply_register (regcache, i, regset + i);
}
#endif /* HAVE_PTRACE_GETREGS */

static struct regset_info nios2_regsets[] =
{
#ifdef HAVE_PTRACE_GETREGS
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, nios2_num_regs * 4, GENERAL_REGS,
    nios2_fill_gregset, nios2_store_gregset },
#endif /* HAVE_PTRACE_GETREGS */
  { 0, 0, 0, -1, -1, NULL, NULL }
};

static struct regsets_info nios2_regsets_info =
  {
    nios2_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct usrregs_info nios2_usrregs_info =
  {
    nios2_num_regs,
    nios2_regmap,
  };

static struct regs_info regs_info =
  {
    NULL, /* regset_bitmap */
    &nios2_usrregs_info,
    &nios2_regsets_info
  };

static const struct regs_info *
nios2_regs_info (void)
{
  return &regs_info;
}

struct linux_target_ops the_low_target =
{
  nios2_arch_setup,
  nios2_regs_info,
  nios2_cannot_fetch_register,
  nios2_cannot_store_register,
  NULL,
  nios2_get_pc,
  nios2_set_pc,
  (const unsigned char *) &nios2_breakpoint,
  nios2_breakpoint_len,
  nios2_reinsert_addr,
  0,
  nios2_breakpoint_at,
};

void
initialize_low_arch (void)
{
  init_registers_nios2_linux ();

  initialize_regsets_info (&nios2_regsets_info);
}
