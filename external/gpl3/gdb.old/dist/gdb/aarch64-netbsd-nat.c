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

#include <machine/frame.h>
#include <machine/pcb.h>

#include "netbsd-nat.h"
#include "aarch64-tdep.h"
#include "aarch64-netbsd-tdep.h"
#include "regcache.h"
#include "gdbcore.h"
#include "bsd-kvm.h"
#include "inf-ptrace.h"

struct aarch64_nbsd_nat_target final : public nbsd_nat_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static aarch64_nbsd_nat_target the_aarch64_nbsd_nat_target;

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

void
aarch64_nbsd_nat_target::fetch_registers (struct regcache *regcache, int regnum)
{
  ptid_t ptid = regcache->ptid ();
  pid_t pid = ptid.pid ();
  int lwp = ptid.lwp ();

  struct gdbarch *gdbarch = regcache->arch ();
  if (regnum == -1 || getregs_supplies (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp) == -1)
	perror_with_name (_("Couldn't get registers"));

      regcache_supply_regset (&aarch64_nbsd_gregset, regcache, regnum, &regs,
			       sizeof (regs));
    }

  if (regnum == -1 || getfpregs_supplies (gdbarch, regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      regcache_supply_regset (&aarch64_nbsd_fpregset, regcache, regnum, &fpregs,
			       sizeof (fpregs));
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

void
aarch64_nbsd_nat_target::store_registers (struct regcache *regcache, int regnum)
{
  ptid_t ptid = regcache->ptid ();
  pid_t pid = ptid.pid ();
  int lwp = ptid.lwp ();

  struct gdbarch *gdbarch = regcache->arch ();
  if (regnum == -1 || getregs_supplies (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp) == -1)
	perror_with_name (_("Couldn't get registers"));

      regcache_collect_regset (&aarch64_nbsd_gregset, regcache,regnum, &regs,
			       sizeof (regs));

      if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp) == -1)
	perror_with_name (_("Couldn't write registers"));
    }

  if (regnum == -1 || getfpregs_supplies (gdbarch, regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      regcache_collect_regset (&aarch64_nbsd_fpregset, regcache,regnum, &fpregs,
				sizeof (fpregs));

      if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}

static int
aarch64_nbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  struct trapframe tf;
  int i;

  /* The following is true for NetBSD/arm64:

     The pcb contains the frame pointer at the point of the context
     switch in cpu_switchto().  At that point we have a stack frame as
     described by `struct trapframe', which has the following layout:

     x0..x30
     sp
     pc
     spsr
     tpidr

     This accounts for all callee-saved registers specified by the psABI.
     From this information we reconstruct the register state as it would
     look when we just returned from cpu_switchto().

     For kernel core dumps, dumpsys() builds a fake trapframe for us. */

  /* The trapframe pointer shouldn't be zero.  */
  if (pcb->pcb_tf == 0)
    return 0;

  /* Read the stack frame, and check its validity.  */
  read_memory ((uintptr_t)pcb->pcb_tf, (gdb_byte *) &tf, sizeof tf);

  for (i = 0; i <= 30; i++)
    {
      regcache->raw_supply (AARCH64_X0_REGNUM + i, &tf.tf_reg[i]);
    }
  regcache->raw_supply (AARCH64_SP_REGNUM, &tf.tf_sp);
  regcache->raw_supply (AARCH64_PC_REGNUM, &tf.tf_pc);

  regcache->raw_supply (AARCH64_FPCR_REGNUM, &pcb->pcb_fpregs.fpcr);
  regcache->raw_supply (AARCH64_FPSR_REGNUM, &pcb->pcb_fpregs.fpsr);

  return 1;
}

void
_initialize_aarch64_nbsd_nat ()
{
  add_inf_child_target (&the_aarch64_nbsd_nat_target);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (aarch64_nbsd_supply_pcb);
}
