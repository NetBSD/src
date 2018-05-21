/* Native-dependent code for NetBSD/aarch64.

   Copyright (C) 2017-2018 Free Software Foundation, Inc.

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
#include "target.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "nbsd-nat.h"
#include "aarch64-tdep.h"
#include "aarch64-nbsd-tdep.h"
#include "regcache.h"
#include "gdbcore.h"
#include "bsd-kvm.h"
#include "inf-ptrace.h"

/* Determine if PT_GETREGS fetches REGNUM.  */

static bool
getregs_supplies (struct gdbarch *gdbarch, int regnum)
{
  return (regnum >= AARCH64_X0_REGNUM && regnum <= AARCH64_CPSR_REGNUM);
}

/* Determine if PT_GETFPREGS fetches REGNUM.  */

static bool
getfpregs_supplies (struct gdbarch *gdbarch, int regnum)
{
  return (regnum >= AARCH64_V0_REGNUM && regnum <= AARCH64_FPCR_REGNUM);
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

static void
aarch64_nbsd_fetch_inferior_registers (struct target_ops *ops,
				    struct regcache *regcache, int regnum)
{
  ptid_t ptid = regcache_get_ptid (regcache);
  pid_t pid = ptid_get_pid (ptid);
  int tid = ptid_get_lwp (ptid);

  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  if (regnum == -1 || getregs_supplies (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, tid) == -1)
	perror_with_name (_("Couldn't get registers"));

      regcache_supply_regset (&aarch64_nbsd_gregset, regcache, regnum, &regs,
			       sizeof (regs));
    }

  if (regnum == -1 || getfpregs_supplies (gdbarch, regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, tid) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      regcache_supply_regset (&aarch64_nbsd_fpregset, regcache, regnum, &fpregs,
			       sizeof (fpregs));
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

static void
aarch64_nbsd_store_inferior_registers (struct target_ops *ops,
				    struct regcache *regcache, int regnum)
{
  ptid_t ptid = regcache_get_ptid (regcache);
  pid_t pid = ptid_get_pid (ptid);
  int tid = ptid_get_lwp (ptid);

  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  if (regnum == -1 || getregs_supplies (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, tid) == -1)
	perror_with_name (_("Couldn't get registers"));

      regcache_collect_regset (&aarch64_nbsd_gregset, regcache,regnum, &regs,
			       sizeof (regs));

      if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, tid) == -1)
	perror_with_name (_("Couldn't write registers"));
    }

  if (regnum == -1 || getfpregs_supplies (gdbarch, regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, tid) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      regcache_collect_regset (&aarch64_nbsd_fpregset, regcache,regnum, &fpregs,
				sizeof (fpregs));

      if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, tid) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}

void
_initialize_aarch64_nbsd_nat (void)
{
  struct target_ops *t;

  t = inf_ptrace_target ();
  t->to_fetch_registers = aarch64_nbsd_fetch_inferior_registers;
  t->to_store_registers = aarch64_nbsd_store_inferior_registers;
  nbsd_nat_add_target (t);
}
