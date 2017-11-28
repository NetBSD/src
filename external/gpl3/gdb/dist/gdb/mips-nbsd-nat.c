/* Native-dependent code for MIPS systems running NetBSD.

   Copyright (C) 2000-2017 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "regcache.h"
#include "target.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "mips-tdep.h"
#include "mips-nbsd-tdep.h"
#include "inf-ptrace.h"

/* Determine if PT_GETREGS fetches this register.  */
static int
getregs_supplies (struct gdbarch *gdbarch, int regno)
{
  return ((regno) >= MIPS_ZERO_REGNUM
	  && (regno) <= gdbarch_pc_regnum (gdbarch));
}

static void
mipsnbsd_fetch_inferior_registers (struct target_ops *ops,
				   struct regcache *regcache, int regno)
{
  pid_t pid = ptid_get_pid (regcache_get_ptid (regcache));

  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  if (regno == -1 || getregs_supplies (gdbarch, regno))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));
      
      mipsnbsd_supply_reg (regcache, (char *) &regs, regno);
      if (regno != -1)
	return;
    }

  if (regno == -1
      || regno >= gdbarch_fp0_regnum (get_regcache_arch (regcache)))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      mipsnbsd_supply_fpreg (regcache, (char *) &fpregs, regno);
    }
}

static void
mipsnbsd_store_inferior_registers (struct target_ops *ops,
				   struct regcache *regcache, int regno)
{
  pid_t pid = ptid_get_pid (regcache_get_ptid (regcache));

  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  if (regno == -1 || getregs_supplies (gdbarch, regno))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      mipsnbsd_fill_reg (regcache, (char *) &regs, regno);

      if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't write registers"));

      if (regno != -1)
	return;
    }

  if (regno == -1
      || regno >= gdbarch_fp0_regnum (get_regcache_arch (regcache)))
    {
      struct fpreg fpregs; 

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      mipsnbsd_fill_fpreg (regcache, (char *) &fpregs, regno);

      if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_mipsnbsd_nat (void);

void
_initialize_mipsnbsd_nat (void)
{
  struct target_ops *t;

  t = inf_ptrace_target ();
  t->to_fetch_registers = mipsnbsd_fetch_inferior_registers;
  t->to_store_registers = mipsnbsd_store_inferior_registers;
  add_target (t);
}
