/* Native-dependent code for FreeBSD/mips.

   Copyright (C) 2017 Free Software Foundation, Inc.

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

#include "fbsd-nat.h"
#include "mips-tdep.h"
#include "mips-fbsd-tdep.h"
#include "inf-ptrace.h"

/* Determine if PT_GETREGS fetches this register.  */

static bool
getregs_supplies (struct gdbarch *gdbarch, int regnum)
{
  return (regnum >= MIPS_ZERO_REGNUM
	  && regnum <= gdbarch_pc_regnum (gdbarch));
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

static void
mips_fbsd_fetch_inferior_registers (struct target_ops *ops,
				    struct regcache *regcache, int regnum)
{
  pid_t pid = get_ptrace_pid (regcache_get_ptid (regcache));

  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  if (regnum == -1 || getregs_supplies (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      mips_fbsd_supply_gregs (regcache, regnum, &regs, sizeof (register_t));
      if (regnum != -1)
	return;
    }

  if (regnum == -1
      || regnum >= gdbarch_fp0_regnum (get_regcache_arch (regcache)))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      mips_fbsd_supply_fpregs (regcache, regnum, &fpregs,
			       sizeof (f_register_t));
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

static void
mips_fbsd_store_inferior_registers (struct target_ops *ops,
				    struct regcache *regcache, int regnum)
{
  pid_t pid = get_ptrace_pid (regcache_get_ptid (regcache));

  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  if (regnum == -1 || getregs_supplies (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      mips_fbsd_collect_gregs (regcache, regnum, (char *) &regs,
			       sizeof (register_t));

      if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't write registers"));

      if (regnum != -1)
	return;
    }

  if (regnum == -1
      || regnum >= gdbarch_fp0_regnum (get_regcache_arch (regcache)))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      mips_fbsd_collect_fpregs (regcache, regnum, (char *) &fpregs,
				sizeof (f_register_t));

      if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_mips_fbsd_nat (void);

void
_initialize_mips_fbsd_nat (void)
{
  struct target_ops *t;

  t = inf_ptrace_target ();
  t->to_fetch_registers = mips_fbsd_fetch_inferior_registers;
  t->to_store_registers = mips_fbsd_store_inferior_registers;
  fbsd_nat_add_target (t);
}
