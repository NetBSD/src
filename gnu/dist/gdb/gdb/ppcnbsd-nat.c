/* Native-dependent code for PowerPC's running NetBSD, for GDB.
   Copyright 2002 Free Software Foundation, Inc.
   Contributed by Wasabi Systems, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/pcb.h>

#include "defs.h"
#include "inferior.h"

#include "ppc-tdep.h"
#include "ppcnbsd-tdep.h"

/* Returns true if PT_GETREGS fetches this register.  */
static int
getregs_supplies (int regno)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);

  return ((regno >= 0 && regno <= 31)
          || regno == tdep->ppc_lr_regnum
          || regno == tdep->ppc_cr_regnum
          || regno == tdep->ppc_xer_regnum
          || regno == tdep->ppc_ctr_regnum
	  || regno == PC_REGNUM);
}

/* Like above, but for PT_GETFPREGS.  */
static int
getfpregs_supplies (int regno)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);

  return ((regno >= FP0_REGNUM && regno <= FP0_REGNUM + 31)
	  || regno == tdep->ppc_fpscr_regnum);
}

void
fetch_inferior_registers (int regno)
{
  if (regno == -1 || getregs_supplies (regno))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &regs, 0) == -1)
        perror_with_name ("Couldn't get registers");

      ppcnbsd_supply_reg ((char *) &regs, regno);
      if (regno != -1)
	return;
    }

  if (regno == -1 || getfpregs_supplies (regno))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &fpregs, 0) == -1)
	perror_with_name ("Couldn't get FP registers");

      ppcnbsd_supply_fpreg ((char *) &fpregs, regno);
      if (regno != -1)
	return;
    }
}

void
store_inferior_registers (int regno)
{
  if (regno == -1 || getregs_supplies (regno))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &regs, 0) == -1)
	perror_with_name ("Couldn't get registers");

      ppcnbsd_fill_reg ((char *) &regs, regno);

      if (ptrace (PT_SETREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &regs, 0) == -1)
	perror_with_name ("Couldn't write registers");

      if (regno != -1)
	return;
    }

  if (regno == -1 || getfpregs_supplies (regno))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &fpregs, 0) == -1)
	perror_with_name ("Couldn't get FP registers");

      ppcnbsd_fill_fpreg ((char *) &fpregs, regno);
      
      if (ptrace (PT_SETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &fpregs, 0) == -1)
	perror_with_name ("Couldn't set FP registers");
    }
}

#ifdef	FETCH_KCORE_REGISTERS
/*
 * Get registers from a kernel crash dump or live kernel.
 * Called by kcore-nbsd.c:get_kcore_registers().
 */
void
fetch_kcore_registers (pcb)
     struct pcb *pcb;
{
  const struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  struct switchframe sf;
  struct callframe cf;
  int regno;

  /*
   * get the register values out of the sys pcb and
   * store them where `read_register' will find them.
   */
  if (target_read_memory(pcb->pcb_sp, (char *)&sf, sizeof(sf)))
    error("Cannot read switchframe.");
  supply_register(2, (char *)&sf.fixreg2);
  supply_register(tdep->ppc_cr_regnum, (char *)&sf.cr);
  for (regno = 13; regno < 32; regno++)
    supply_register(regno, (char *)&sf.fixreg[regno - 13]);
  if (target_read_memory(sf.sp, (char *)&cf, sizeof(cf)))
    error("Cannot read callframe.");
  supply_register(30, (char *)&cf.r30);
  supply_register(31, (char *)&cf.r31);
  supply_register(1, (char *)&cf.sp);
  if (target_read_memory(cf.sp, (char *)&cf, sizeof(cf)))
    error("Cannot read switchframe.");
  supply_register(tdep->ppc_lr_regnum, (char *)&cf.lr);
  supply_register(PC_REGNUM, (char *)&cf.lr);
}
#endif	/* FETCH_KCORE_REGISTERS */
