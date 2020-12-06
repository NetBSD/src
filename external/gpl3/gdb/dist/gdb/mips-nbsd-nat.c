/* Native-dependent code for MIPS systems running NetBSD.

   Copyright (C) 2000-2020 Free Software Foundation, Inc.

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

/* We define this to get types like register_t.  */
#include "defs.h"
#include "inferior.h"
#include "regcache.h"
#include "target.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "mips-tdep.h"
#include "nbsd-nat.h"
#include "mips-nbsd-tdep.h"
#include "inf-ptrace.h"
#include "bsd-kvm.h"

#include "machine/pcb.h"

class mips_nbsd_nat_target final : public nbsd_nat_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static mips_nbsd_nat_target the_mips_nbsd_nat_target;

/* Determine if PT_GETREGS fetches this register.  */
static int
getregs_supplies (struct gdbarch *gdbarch, int regno)
{
  return ((regno) >= MIPS_ZERO_REGNUM
	  && (regno) <= gdbarch_pc_regnum (gdbarch));
}

void
mips_nbsd_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  ptid_t ptid = regcache->ptid ();
  pid_t pid = ptid.pid ();
  int lwp = ptid.lwp ();

  struct gdbarch *gdbarch = regcache->arch ();
  if (regno == -1 || getregs_supplies (gdbarch, regno))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp) == -1)
	perror_with_name (_("Couldn't get registers"));
      
      mipsnbsd_supply_reg (regcache, (char *) &regs, regno);
      if (regno != -1)
	return;
    }

  if (regno == -1
      || regno >= gdbarch_fp0_regnum (regcache->arch ()))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      mipsnbsd_supply_fpreg (regcache, (char *) &fpregs, regno);
    }
}

void
mips_nbsd_nat_target::store_registers (struct regcache *regcache, int regno)
{
  ptid_t ptid = regcache->ptid ();
  pid_t pid = ptid.pid ();
  int lwp = ptid.lwp ();

  struct gdbarch *gdbarch = regcache->arch ();
  if (regno == -1 || getregs_supplies (gdbarch, regno))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp) == -1)
	perror_with_name (_("Couldn't get registers"));

      mipsnbsd_fill_reg (regcache, (char *) &regs, regno);

      if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp) == -1)
	perror_with_name (_("Couldn't write registers"));

      if (regno != -1)
	return;
    }

  if (regno == -1
      || regno >= gdbarch_fp0_regnum (regcache->arch ()))
    {
      struct fpreg fpregs; 

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      mipsnbsd_fill_fpreg (regcache, (char *) &fpregs, regno);

      if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}

static int
mipsnbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  struct label_t sf;

  sf = pcb->pcb_context;

  /* really should test for n{32,64} abi for this register
     unless this is purely the "n" ABI */

  regcache->raw_supply (MIPS_S0_REGNUM, &sf.val[_L_S0]);
  regcache->raw_supply (MIPS_S1_REGNUM, &sf.val[_L_S1]);
  regcache->raw_supply (MIPS_S2_REGNUM, &sf.val[_L_S2]);
  regcache->raw_supply (MIPS_S3_REGNUM, &sf.val[_L_S3]);
  regcache->raw_supply (MIPS_S4_REGNUM, &sf.val[_L_S4]);
  regcache->raw_supply (MIPS_S5_REGNUM, &sf.val[_L_S5]);
  regcache->raw_supply (MIPS_S6_REGNUM, &sf.val[_L_S6]);
  regcache->raw_supply (MIPS_S7_REGNUM, &sf.val[_L_S7]);

  regcache->raw_supply (MIPS_S8_REGNUM, &sf.val[_L_S8]);

  regcache->raw_supply (MIPS_T8_REGNUM, &sf.val[_L_T8]);

  regcache->raw_supply (MIPS_GP_REGNUM, &sf.val[_L_GP]);

  regcache->raw_supply (MIPS_SP_REGNUM, &sf.val[_L_SP]);
  regcache->raw_supply (MIPS_RA_REGNUM, &sf.val[_L_RA]);
  regcache->raw_supply (MIPS_PS_REGNUM, &sf.val[_L_SR]);

  /* provide the return address of the savectx as the current pc */
  regcache->raw_supply (MIPS_EMBED_PC_REGNUM, &sf.val[_L_RA]);

  return 0;
}

void _initialize_mipsnbsd_nat ();
void
_initialize_mipsnbsd_nat ()
{
  add_inf_child_target (&the_mips_nbsd_nat_target);
  bsd_kvm_add_target (mipsnbsd_supply_pcb);
}
