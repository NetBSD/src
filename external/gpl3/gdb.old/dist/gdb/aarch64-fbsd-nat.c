/* Native-dependent code for FreeBSD/aarch64.

   Copyright (C) 2017-2020 Free Software Foundation, Inc.

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
#include "regcache.h"
#include "target.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "fbsd-nat.h"
#include "aarch64-tdep.h"
#include "aarch64-fbsd-tdep.h"
#include "inf-ptrace.h"

struct aarch64_fbsd_nat_target final : public fbsd_nat_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static aarch64_fbsd_nat_target the_aarch64_fbsd_nat_target;

/* Determine if PT_GETREGS fetches REGNUM.  */

static bool
getregs_supplies (int regnum)
{
  return (regnum >= AARCH64_X0_REGNUM && regnum <= AARCH64_CPSR_REGNUM);
}

/* Determine if PT_GETFPREGS fetches REGNUM.  */

static bool
getfpregs_supplies (int regnum)
{
  return (regnum >= AARCH64_V0_REGNUM && regnum <= AARCH64_FPCR_REGNUM);
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

void
aarch64_fbsd_nat_target::fetch_registers (struct regcache *regcache,
					  int regnum)
{
  pid_t pid = get_ptrace_pid (regcache->ptid ());

  if (regnum == -1 || getregs_supplies (regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      regcache->supply_regset (&aarch64_fbsd_gregset, regnum, &regs,
			       sizeof (regs));
    }

  if (regnum == -1 || getfpregs_supplies (regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      regcache->supply_regset (&aarch64_fbsd_fpregset, regnum, &fpregs,
			       sizeof (fpregs));
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

void
aarch64_fbsd_nat_target::store_registers (struct regcache *regcache,
					  int regnum)
{
  pid_t pid = get_ptrace_pid (regcache->ptid ());

  if (regnum == -1 || getregs_supplies (regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      regcache->collect_regset (&aarch64_fbsd_gregset, regnum, &regs,
			       sizeof (regs));

      if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't write registers"));
    }

  if (regnum == -1 || getfpregs_supplies (regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      regcache->collect_regset (&aarch64_fbsd_fpregset, regnum, &fpregs,
				sizeof (fpregs));

      if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}

void _initialize_aarch64_fbsd_nat ();
void
_initialize_aarch64_fbsd_nat ()
{
  add_inf_child_target (&the_aarch64_fbsd_nat_target);
}
