/* Native-dependent code for SPARC systems running NetBSD.
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

#include "defs.h"
#include "inferior.h"
#include "regcache.h"

#include "sparcnbsd-tdep.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

/* NOTE: We don't bother with any of the deferred_store nonsense; it
   makes things a lot more complicated than they need to be.  */

/* Determine if PT_GETREGS fetches this register.  */
static int
getregs_supplies (int regno)
{
  return (regno == PS_REGNUM
	  || regno == PC_REGNUM
	  || regno == NPC_REGNUM
	  || regno == Y_REGNUM
	  || (regno >= G0_REGNUM && regno <= G7_REGNUM)
	  || (regno >= O0_REGNUM && regno <= O7_REGNUM)
	  /* stack regs (handled by sparcnbsd_supply_reg)  */
	  || (regno >= L0_REGNUM && regno <= I7_REGNUM));
}

/* Determine if PT_GETFPREGS fetches this register.  */
static int
getfpregs_supplies (int regno)
{
  return ((regno >= FP0_REGNUM && regno <= (FP0_REGNUM + 31))
	  || regno == FPS_REGNUM);
}

void
fetch_inferior_registers (int regno)
{
  /* We don't use deferred stores.  */
  if (deferred_stores)
    internal_error (__FILE__, __LINE__,
		    "fetch_inferior_registers: deferred stores pending");

  if (regno == -1 || getregs_supplies (regno))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &regs, 0) == -1)
        perror_with_name ("Couldn't get registers");

      sparcnbsd_supply_reg32 ((char *) &regs, regno);
      if (regno != -1)
	return;
    }

  if (regno == -1 || getfpregs_supplies (regno))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &fpregs, 0) == -1)
        perror_with_name ("Couldn't get floating point registers");

      sparcnbsd_supply_fpreg32 ((char *) &fpregs, regno);
      if (regno != -1)
	return;
    }
}

void
store_inferior_registers (int regno)
{
  /* We don't use deferred stores.  */
  if (deferred_stores)
    internal_error (__FILE__, __LINE__,
		    "store_inferior_registers: deferred stores pending");

  if (regno == -1 || getregs_supplies (regno))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &regs, 0) == -1)
	perror_with_name ("Couldn't get registers");

      sparcnbsd_fill_reg32 ((char *) &regs, regno);

      if (ptrace (PT_SETREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &regs, 0) == -1)
	perror_with_name ("Couldn't write registers");

      /* Deal with the stack regs.  */
      if (regno == -1 || regno == SP_REGNUM
	  || (regno >= L0_REGNUM && regno <= I7_REGNUM))
	{
	  CORE_ADDR sp = read_register (SP_REGNUM);
	  int i;
	  char buf[4];

	  for (i = L0_REGNUM; i <= I7_REGNUM; i++)
	    {
	      if (regno == -1 || regno == SP_REGNUM || regno == i)
		{
		  regcache_collect (i, buf);
		  target_write_memory (sp + ((i - L0_REGNUM) * 4),
				       buf, sizeof (buf));
		}
	    }
	}

      if (regno != -1)
	return;
    }

  if (regno == -1 || getfpregs_supplies (regno))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &fpregs, 0) == -1)
	perror_with_name ("Couldn't get floating point registers");

      sparcnbsd_fill_fpreg32 ((char *) &fpregs, regno);
      
      if (ptrace (PT_SETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &fpregs, 0) == -1)
	perror_with_name ("Couldn't write floating point registers");

      if (regno != -1)
	return;
    }
}

void
nbsd_reg_to_internal (rgs)
	char *rgs;
{
  sparcnbsd_supply_reg32(rgs, -1);
}

void
nbsd_fpreg_to_internal (frgs)
	char *frgs;
{
  sparcnbsd_supply_fpreg32(frgs, -1);
}

void
nbsd_internal_to_reg (regs)
	char *regs;
{
  sparcnbsd_fill_reg32(regs, -1);
}

void
nbsd_internal_to_fpreg (fpregs)
	char *fpregs;
{
  sparcnbsd_fill_fpreg32(fpregs, -1);
}

