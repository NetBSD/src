/* Native-dependent code for NetBSD/i386.

   Copyright (C) 2004-2014 Free Software Foundation, Inc.

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

#include "i386-tdep.h"
#include "i387-tdep.h"
#include "i386bsd-nat.h"

/* Support for debugging kernel virtual memory images.  */

#include <sys/types.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/pcb.h>

#include "nbsd-nat.h"
#include "bsd-kvm.h"

#ifndef HAVE_GREGSET_T
typedef struct reg gregset_t;
#endif

#ifndef HAVE_FPREGSET_T
typedef struct fpreg fpregset_t;
#endif

#include "gregset.h" 

static int
i386nbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  struct switchframe sf;

  /* The following is true for NetBSD 1.6.2 and after:

     The pcb contains %esp and %ebp at the point of the context switch
     in cpu_switch()/cpu_switchto().  At that point we have a stack frame as
     described by `struct switchframe', which for NetBSD (2.0 and later) has
     the following layout:

     %edi
     %esi
     %ebx
     return address

     we reconstruct the register state as it would look when we just
     returned from cpu_switch()/cpu_switchto().

     For core dumps the pcb is saved by savectx()/dumpsys() and contains the
     stack pointer and frame pointer.  A new dumpsys() fakes a switchframe
     whereas older code isn't reliable so use an iffy heuristic to detect this
     and use the frame pointer to recover enough state.  */

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_esp == 0)
    return 0;

  read_memory (pcb->pcb_esp, (gdb_byte *) &sf, sizeof sf);

  if ( (unsigned long)sf.sf_eip >= (unsigned long)0xc0100000 )
    {
      /* Yes, we have a switchframe that matches cpu_switchto() or
         the new dumpsys().  */

      pcb->pcb_esp += sizeof (struct switchframe);
      regcache_raw_supply (regcache, I386_EDI_REGNUM, &sf.sf_edi);
      regcache_raw_supply (regcache, I386_ESI_REGNUM, &sf.sf_esi);
      regcache_raw_supply (regcache, I386_EBP_REGNUM, &pcb->pcb_ebp);
      regcache_raw_supply (regcache, I386_ESP_REGNUM, &pcb->pcb_esp);
      regcache_raw_supply (regcache, I386_EBX_REGNUM, &sf.sf_ebx);
      regcache_raw_supply (regcache, I386_EIP_REGNUM, &sf.sf_eip);
    }
  else
    {
      CORE_ADDR pc, fp;

      /* No, the pcb must have been last updated by savectx() in old
         dumpsys(). Use the frame pointer to recover enough state.  */

      read_memory (pcb->pcb_ebp, (gdb_byte *) &fp, sizeof(fp));
      read_memory (pcb->pcb_ebp + 4, (gdb_byte *) &pc, sizeof(pc));

      regcache_raw_supply (regcache, I386_ESP_REGNUM, &pcb->pcb_ebp);
      regcache_raw_supply (regcache, I386_EBP_REGNUM, &fp);
      regcache_raw_supply (regcache, I386_EIP_REGNUM, &pc);
    }

  return 1;
}

void
supply_gregset (struct regcache *regcache, const gregset_t *gregsetp)
{
  i386bsd_supply_gregset (regcache, gregsetp);
}

/* Fill register REGNUM (if it is a general-purpose register) in
   *GREGSETP with the value in GDB's register cache.  If REGNUM is -1,
   do this for all registers.  */

void
fill_gregset (const struct regcache *regcache,
              gregset_t *gregsetp, int regnum)
{
  i386bsd_collect_gregset (regcache, gregsetp, regnum);
}

/* Transfering floating-point registers between GDB, inferiors and cores.  */
   
/* Fill GDB's register cache with the floating-point and SSE register
   values in *FPREGSETP.  */

void
supply_fpregset (struct regcache *regcache, const fpregset_t *fpregsetp)
{  
  i387_supply_fsave (regcache, -1, fpregsetp);
}
   
/* Fill register REGNUM (if it is a floating-point or SSE register) in
   *FPREGSETP with the value in GDB's register cache.  If REGNUM is
   -1, do this for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
               fpregset_t *fpregsetp, int regnum)
{
  i387_collect_fsave (regcache, regnum, fpregsetp);
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_i386nbsd_nat (void);

void
_initialize_i386nbsd_nat (void)
{
  struct target_ops *t;

  /* Add some extra features to the common *BSD/i386 target.  */
  t = i386bsd_target ();
  t->to_pid_to_exec_file = nbsd_pid_to_exec_file;
  add_target (t);
 
  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (i386nbsd_supply_pcb);
}
