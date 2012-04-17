/* Native-dependent code for NetBSD/sparc64.

   Copyright (C) 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "regcache.h"
#include "target.h"

#include "nbsd-nat.h"
#include "sparc64-tdep.h"
#include "sparc-nat.h"

#include <machine/reg.h>

#ifndef HAVE_GREGSET_T
typedef struct reg gregset_t;
#endif

#ifndef HAVE_FPREGSET_T
typedef struct fpreg fpregset_t;
#endif
#include "gregset.h"
 
/* NetBSD is different from the other OSes that support both SPARC and
   UltraSPARC in that the result of ptrace(2) depends on whether the
   traced process is 32-bit or 64-bit.  */

static void
sparc64nbsd_supply_gregset (const struct sparc_gregset *gregset,
			    struct regcache *regcache,
			    int regnum, const void *gregs)
{
  int sparc32 = (gdbarch_ptr_bit (get_regcache_arch (regcache)) == 32);

  if (sparc32)
    sparc32_supply_gregset (&sparc32nbsd_gregset, regcache, regnum, gregs);
  else
    sparc64_supply_gregset (&sparc64nbsd_gregset, regcache, regnum, gregs);
}

static void
sparc64nbsd_collect_gregset (const struct sparc_gregset *gregset,
			     const struct regcache *regcache,
			     int regnum, void *gregs)
{
  int sparc32 = (gdbarch_ptr_bit (get_regcache_arch (regcache)) == 32);

  if (sparc32)
    sparc32_collect_gregset (&sparc32nbsd_gregset, regcache, regnum, gregs);
  else
    sparc64_collect_gregset (&sparc64nbsd_gregset, regcache, regnum, gregs);
}

static void
sparc64nbsd_supply_fpregset (struct regcache *regcache,
			     int regnum, const void *fpregs)
{
  int sparc32 = (gdbarch_ptr_bit (get_regcache_arch (regcache)) == 32);

  if (sparc32)
    sparc32_supply_fpregset (regcache, regnum, fpregs);
  else
    sparc64_supply_fpregset (regcache, regnum, fpregs);
}

static void
sparc64nbsd_collect_fpregset (const struct regcache *regcache,
			      int regnum, void *fpregs)
{
  int sparc32 = (gdbarch_ptr_bit (get_regcache_arch (regcache)) == 32);

  if (sparc32)
    sparc32_collect_fpregset (regcache, regnum, fpregs);
  else
    sparc64_collect_fpregset (regcache, regnum, fpregs);
}

/* Determine whether `gregset_t' contains register REGNUM.  */

static int
sparc64nbsd_gregset_supplies_p (struct gdbarch *gdbarch, int regnum)
{
  if (gdbarch_ptr_bit (gdbarch) == 32)
    return sparc32_gregset_supplies_p (gdbarch, regnum);

  /* Integer registers.  */
  if ((regnum >= SPARC_G1_REGNUM && regnum <= SPARC_G7_REGNUM)
      || (regnum >= SPARC_O0_REGNUM && regnum <= SPARC_O7_REGNUM)
      || (regnum >= SPARC_L0_REGNUM && regnum <= SPARC_L7_REGNUM)
      || (regnum >= SPARC_I0_REGNUM && regnum <= SPARC_I7_REGNUM))
    return 1;

  /* Control registers.  */
  if (regnum == SPARC64_PC_REGNUM
      || regnum == SPARC64_NPC_REGNUM
      || regnum == SPARC64_STATE_REGNUM
      || regnum == SPARC64_Y_REGNUM)
    return 1;

  return 0;
}

/* Determine whether `fpregset_t' contains register REGNUM.  */

static int
sparc64nbsd_fpregset_supplies_p (struct gdbarch *gdbarch, int regnum)
{
  if (gdbarch_ptr_bit (gdbarch) == 32)
    return sparc32_fpregset_supplies_p (gdbarch, regnum);

  /* Floating-point registers.  */
  if ((regnum >= SPARC_F0_REGNUM && regnum <= SPARC_F31_REGNUM)
      || (regnum >= SPARC64_F32_REGNUM && regnum <= SPARC64_F62_REGNUM))
    return 1;

  /* Control registers.  */
  if (regnum == SPARC64_FSR_REGNUM)
    return 1;

  return 0;
}

void
supply_gregset (struct regcache *regcache, const gregset_t *gregs)
{
  sparc64nbsd_supply_gregset (sparc_gregset, regcache, -1, gregs);
}

void
supply_fpregset (struct regcache *regcache, const fpregset_t *fpregs)
{
  sparc64nbsd_supply_fpregset (regcache, -1, fpregs);
}

void
fill_gregset (const struct regcache *regcache, gregset_t *gregs, int regnum)
{
  sparc64nbsd_collect_gregset (sparc_gregset, regcache, regnum, gregs);
}

void
fill_fpregset (const struct regcache *regcache, fpregset_t *fpregs, int regnum)
{
  sparc64nbsd_collect_fpregset (regcache, regnum, fpregs);
}
/* Support for debugging kernel virtual memory images.  */

#include <sys/types.h>
#include <machine/pcb.h>

#include "bsd-kvm.h"

static int
sparc64nbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  u_int64_t state;
  int regnum;

  /* The following is true for NetBSD 1.6.2:

     The pcb contains %sp and %pc, %pstate and %cwp.  From this
     information we reconstruct the register state as it would look
     when we just returned from cpu_switch().  */

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_sp == 0)
    return 0;

  /* If the program counter is zero, this is probably a core dump, and
     we can get %pc from the stack.  */
  if (pcb->pcb_pc == 0)
      read_memory(pcb->pcb_sp + BIAS - 176 + (11 * 8), 
		  (gdb_byte *)&pcb->pcb_pc, sizeof pcb->pcb_pc);

  regcache_raw_supply (regcache, SPARC_SP_REGNUM, &pcb->pcb_sp);
  regcache_raw_supply (regcache, SPARC64_PC_REGNUM, &pcb->pcb_pc);

  state = pcb->pcb_pstate << 8 | pcb->pcb_cwp;
  regcache_raw_supply (regcache, SPARC64_STATE_REGNUM, &state);

  sparc_supply_rwindow (regcache, pcb->pcb_sp, -1);

  return 1;
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_sparc64nbsd_nat (void);

void
_initialize_sparc64nbsd_nat (void)
{
  struct target_ops *t;

  sparc_supply_gregset = sparc64nbsd_supply_gregset;
  sparc_collect_gregset = sparc64nbsd_collect_gregset;
  sparc_supply_fpregset = sparc64nbsd_supply_fpregset;
  sparc_collect_fpregset = sparc64nbsd_collect_fpregset;
  sparc_gregset_supplies_p = sparc64nbsd_gregset_supplies_p;
  sparc_fpregset_supplies_p = sparc64nbsd_fpregset_supplies_p;

  /* Add some extra features to the generic SPARC target.  */
  t = sparc_target ();
  t->to_pid_to_exec_file = nbsd_pid_to_exec_file;
  add_target (t);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (sparc64nbsd_supply_pcb);
}
