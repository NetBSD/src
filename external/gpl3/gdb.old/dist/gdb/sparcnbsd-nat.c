/* Native-dependent code for NetBSD/sparc.

   Copyright (C) 2002-2016 Free Software Foundation, Inc.

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

#include "nbsd-nat.h"
#include "sparc-tdep.h"
#include "sparc-nat.h"

/* Support for debugging kernel virtual memory images.  */

#include <sys/types.h>
#include <machine/pcb.h>
#include <machine/reg.h>

#include "bsd-kvm.h"

#ifndef HAVE_GREGSET_T
typedef struct reg gregset_t;
#endif

#ifndef HAVE_FPREGSET_T
typedef struct fpreg fpregset_t;
#endif
#include "gregset.h"
 
void
supply_gregset (struct regcache *regcache, const gregset_t *gregs)
{
  sparc_supply_gregset (sparc_gregmap, regcache, -1, gregs);
}

void
supply_fpregset (struct regcache *regcache, const fpregset_t *fpregs)
{
  sparc_supply_fpregset (sparc_fpregmap, regcache, -1, fpregs);
}

void
fill_gregset (const struct regcache *regcache, gregset_t *gregs, int regnum)
{
  sparc_collect_gregset (sparc_gregmap, regcache, regnum, gregs);
}

void
fill_fpregset (const struct regcache *regcache, fpregset_t *fpregs, int regnum)
{
  sparc_collect_fpregset (sparc_fpregmap, regcache, regnum, fpregs);
}

static int
sparc32nbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  /* The following is true for NetBSD 1.6.2:

     The pcb contains %sp, %pc, %psr and %wim.  From this information
     we reconstruct the register state as it would look when we just
     returned from cpu_switch().  */

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_sp == 0)
    return 0;

  regcache_raw_supply (regcache, SPARC_SP_REGNUM, &pcb->pcb_sp);
  regcache_raw_supply (regcache, SPARC_O7_REGNUM, &pcb->pcb_pc);
  regcache_raw_supply (regcache, SPARC32_PSR_REGNUM, &pcb->pcb_psr);
  regcache_raw_supply (regcache, SPARC32_WIM_REGNUM, &pcb->pcb_wim);
  regcache_raw_supply (regcache, SPARC32_PC_REGNUM, &pcb->pcb_pc);

  sparc_supply_rwindow (regcache, pcb->pcb_sp, -1);

  return 1;
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_sparcnbsd_nat (void);

void
_initialize_sparcnbsd_nat (void)
{
  struct target_ops *t;
  sparc_gregmap = &sparc32nbsd_gregmap;
  sparc_fpregmap = &sparc32_bsd_fpregmap;

  /* Add some extra features to the generic SPARC target.  */
  t = sparc_target ();
  t->to_pid_to_exec_file = nbsd_pid_to_exec_file;
  add_target (t);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (sparc32nbsd_supply_pcb);
}
