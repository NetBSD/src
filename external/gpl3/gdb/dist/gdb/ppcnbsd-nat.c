/* Native-dependent code for NetBSD/powerpc.

   Copyright (C) 2002, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

   Contributed by Wasabi Systems, Inc.

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

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/pcb.h>

#include "defs.h"
#include "gdbcore.h"
#include "inferior.h"
#include "regcache.h"

#include "gdb_assert.h"

#include "nbsd-nat.h"
#include "ppc-tdep.h"
#include "ppcnbsd-tdep.h"
#include "bsd-kvm.h"
#include "inf-ptrace.h"

#ifndef HAVE_GREGSET_T
typedef struct reg gregset_t;
#endif

#ifndef HAVE_FPREGSET_T
typedef struct fpreg fpregset_t;
#endif
#include "gregset.h"
 

/* Returns true if PT_GETREGS fetches this register.  */

static int
getregs_supplies (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  return ((regnum >= tdep->ppc_gp0_regnum
           && regnum < tdep->ppc_gp0_regnum + ppc_num_gprs)
          || regnum == tdep->ppc_lr_regnum
          || regnum == tdep->ppc_cr_regnum
          || regnum == tdep->ppc_xer_regnum
          || regnum == tdep->ppc_ctr_regnum
	  || regnum == gdbarch_pc_regnum (gdbarch));
}

/* Like above, but for PT_GETFPREGS.  */

static int
getfpregs_supplies (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* FIXME: jimb/2004-05-05: Some PPC variants don't have floating
     point registers.  Traditionally, GDB's register set has still
     listed the floating point registers for such machines, so this
     code is harmless.  However, the new E500 port actually omits the
     floating point registers entirely from the register set --- they
     don't even have register numbers assigned to them.

     It's not clear to me how best to update this code, so this assert
     will alert the first person to encounter the NetBSD/E500
     combination to the problem.  */
  gdb_assert (ppc_floating_point_unit_p (gdbarch));

  return ((regnum >= tdep->ppc_fp0_regnum
           && regnum < tdep->ppc_fp0_regnum + ppc_num_fprs)
	  || regnum == tdep->ppc_fpscr_regnum);
}

static void
ppcnbsd_fetch_inferior_registers (struct target_ops *ops,
				  struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  if (regnum == -1 || getregs_supplies (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &regs, TIDGET (inferior_ptid)) == -1)
        perror_with_name (_("Couldn't get registers"));

      ppc_supply_gregset (&ppcnbsd_gregset, regcache,
			  regnum, &regs, sizeof regs);
    }

  if (regnum == -1 || getfpregs_supplies (gdbarch, regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get FP registers"));

      ppc_supply_fpregset (&ppcnbsd_fpregset, regcache,
			   regnum, &fpregs, sizeof fpregs);
    }
}

static void
ppcnbsd_store_inferior_registers (struct target_ops *ops,
				  struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  if (regnum == -1 || getregs_supplies (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &regs, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get registers"));

      ppc_collect_gregset (&ppcnbsd_gregset, regcache,
			   regnum, &regs, sizeof regs);

      if (ptrace (PT_SETREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &regs, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't write registers"));
    }

  if (regnum == -1 || getfpregs_supplies (gdbarch, regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get FP registers"));

      ppc_collect_fpregset (&ppcnbsd_fpregset, regcache,
			    regnum, &fpregs, sizeof fpregs);

      if (ptrace (PT_SETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't set FP registers"));
    }
}

void
supply_gregset (struct regcache *regcache, const gregset_t *gregs)
{
  if (ptrace (PT_SETREGS, PIDGET (inferior_ptid),
	      (PTRACE_TYPE_ARG3) gregs, TIDGET (inferior_ptid)) == -1)
    perror_with_name (_("Couldn't write registers"));
}

void
supply_fpregset (struct regcache *regcache, const fpregset_t *fpregs)
{
  if (ptrace (PT_SETFPREGS, PIDGET (inferior_ptid),
	      (PTRACE_TYPE_ARG3) fpregs, TIDGET (inferior_ptid)) == -1)
    perror_with_name (_("Couldn't set FP registers"));
}

void
fill_gregset (const struct regcache *regcache, gregset_t *gregs, int regnum)
{
      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) gregs, TIDGET (inferior_ptid)) == -1)
        perror_with_name (_("Couldn't get registers"));
}

void
fill_fpregset (const struct regcache *regcache, fpregset_t *fpregs, int regnum)
{
      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) fpregs, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get FP registers"));
}

static int
ppcnbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  struct switchframe sf;
  struct callframe cf;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int i;

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_sp == 0)
    return 0;

  read_memory (pcb->pcb_sp, (gdb_byte *)&sf, sizeof sf);
  regcache_raw_supply (regcache, tdep->ppc_cr_regnum, &sf.sf_cr);
  regcache_raw_supply (regcache, tdep->ppc_gp0_regnum + 2, &sf.sf_fixreg2);
  for (i = 0 ; i < 19 ; i++)
    regcache_raw_supply (regcache, tdep->ppc_gp0_regnum + 13 + i,
			 &sf.sf_fixreg[i]);

  read_memory(sf.sf_sp, (gdb_byte *)&cf, sizeof(cf));
  regcache_raw_supply (regcache, tdep->ppc_gp0_regnum + 30, &cf.cf_r30);
  regcache_raw_supply (regcache, tdep->ppc_gp0_regnum + 31, &cf.cf_r31);
  regcache_raw_supply (regcache, tdep->ppc_gp0_regnum + 1, &cf.cf_sp);

  read_memory(cf.cf_sp, (gdb_byte *)&cf, sizeof(cf));
  regcache_raw_supply (regcache, tdep->ppc_lr_regnum, &cf.cf_lr);
  regcache_raw_supply (regcache, gdbarch_pc_regnum (gdbarch), &cf.cf_lr);

  return 1;
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_ppcnbsd_nat (void);

void
_initialize_ppcnbsd_nat (void)
{
  struct target_ops *t;

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (ppcnbsd_supply_pcb);

  /* Add in local overrides.  */
  t = inf_ptrace_target ();
  t->to_fetch_registers = ppcnbsd_fetch_inferior_registers;
  t->to_store_registers = ppcnbsd_store_inferior_registers;
  t->to_pid_to_exec_file = nbsd_pid_to_exec_file;
  add_target (t);
}
