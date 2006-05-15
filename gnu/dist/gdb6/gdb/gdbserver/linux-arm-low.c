/* GNU/Linux/ARM specific low level interface, for the remote server for GDB.
   Copyright 1995, 1996, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "server.h"
#include "linux-low.h"

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#define arm_num_regs 26

static int arm_regmap[] = {
  0, 4, 8, 12, 16, 20, 24, 28,
  32, 36, 40, 44, 48, 52, 56, 60,
  -1, -1, -1, -1, -1, -1, -1, -1, -1,
  64
};

static int
arm_cannot_store_register (int regno)
{
  return (regno >= arm_num_regs);
}

static int
arm_cannot_fetch_register (int regno)
{
  return (regno >= arm_num_regs);
}

extern int debug_threads;

static CORE_ADDR
arm_get_pc ()
{
  unsigned long pc;
  collect_register_by_name ("pc", &pc);
  if (debug_threads)
    fprintf (stderr, "stop pc is %08lx\n", pc);
  return pc;
}

static void
arm_set_pc (CORE_ADDR pc)
{
  unsigned long newpc = pc;
  supply_register_by_name ("pc", &newpc);
}

/* Correct in either endianness.  We do not support Thumb yet.  */
static const unsigned long arm_breakpoint = 0xef9f0001;
#define arm_breakpoint_len 4

/* For new EABI binaries.  We recognize it regardless of which ABI
   is used for gdbserver, so single threaded debugging should work
   OK, but for multi-threaded debugging we only insert the current
   ABI's breakpoint instruction.  For now at least.  */
static const unsigned long arm_eabi_breakpoint = 0xe7f001f0;

static int
arm_breakpoint_at (CORE_ADDR where)
{
  unsigned long insn;

  (*the_target->read_memory) (where, (unsigned char *) &insn, 4);
  if (insn == arm_breakpoint)
    return 1;

  if (insn == arm_eabi_breakpoint)
    return 1;

  /* If necessary, recognize more trap instructions here.  GDB only uses the
     two.  */

  return 0;
}

/* We only place breakpoints in empty marker functions, and thread locking
   is outside of the function.  So rather than importing software single-step,
   we can just run until exit.  */
static CORE_ADDR
arm_reinsert_addr ()
{
  unsigned long pc;
  collect_register_by_name ("lr", &pc);
  return pc;
}

struct linux_target_ops the_low_target = {
  arm_num_regs,
  arm_regmap,
  arm_cannot_fetch_register,
  arm_cannot_store_register,
  arm_get_pc,
  arm_set_pc,
#ifndef __ARM_EABI__
  (const unsigned char *) &arm_breakpoint,
#else
  (const unsigned char *) &arm_eabi_breakpoint,
#endif
  arm_breakpoint_len,
  arm_reinsert_addr,
  0,
  arm_breakpoint_at,
};
