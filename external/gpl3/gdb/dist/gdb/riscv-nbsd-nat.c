/* Native-dependent code for NetBSD/riscv.

   Copyright (C) 2018-2020 Free Software Foundation, Inc.

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

#include "nbsd-nat.h"
#include "riscv-tdep.h"
#include "riscv-nbsd-tdep.h"
#include "inf-ptrace.h"

struct riscv_nbsd_nat_target final : public nbsd_nat_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static riscv_nbsd_nat_target the_riscv_nbsd_nat_target;

/* Determine if PT_GETREGS fetches REGNUM.  */

static bool
getregs_supplies (int regnum)
{
  return regnum >= RISCV_RA_REGNUM && regnum <= RISCV_PC_REGNUM;
}

/* Determine if PT_GETFPREGS fetches REGNUM.  */

static bool
getfpregs_supplies (int regnum)
{
  return ((regnum >= RISCV_FIRST_FP_REGNUM && regnum <= RISCV_LAST_FP_REGNUM)
	  || regnum == RISCV_CSR_FCSR_REGNUM);
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

void
riscv_nbsd_nat_target::fetch_registers (struct regcache *regcache,
					int regnum)
{
  pid_t pid = regcache->ptid ().pid ();
  int lwp = regcache->ptid ().lwp ();

  if (regnum == -1 || regnum == RISCV_ZERO_REGNUM)
    regcache->raw_supply_zeroed (RISCV_ZERO_REGNUM);
  if (regnum == -1 || getregs_supplies (regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      regcache->supply_regset (&riscv_nbsd_gregset, regnum, &regs,
			       sizeof (regs));
    }

  if (regnum == -1 || getfpregs_supplies (regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      regcache->supply_regset (&riscv_nbsd_fpregset, regnum, &fpregs,
			       sizeof (fpregs));
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

void
riscv_nbsd_nat_target::store_registers (struct regcache *regcache,
					int regnum)
{
  pid_t pid = regcache->ptid ().pid ();
  int lwp = regcache->ptid ().lwp ();

  if (regnum == -1 || getregs_supplies (regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      regcache->collect_regset (&riscv_nbsd_gregset, regnum, &regs,
			       sizeof (regs));

      if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't write registers"));
    }

  if (regnum == -1 || getfpregs_supplies (regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      regcache->collect_regset (&riscv_nbsd_fpregset, regnum, &fpregs,
				sizeof (fpregs));

      if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}

/* Initialize RISC-V NetBSD native support.  */

void _initialize_riscv_nbsd_nat ();
void
_initialize_riscv_nbsd_nat ()
{
  add_inf_child_target (&the_riscv_nbsd_nat_target);

//  bsd_kvm_add_target (riscv_nbsd_supply_pcb);
}


