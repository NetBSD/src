/* Native-dependent code for UltraSPARC systems running NetBSD.
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
  /* FIXME: PS_REGNUM for 32-bit code.  */
  return (regno == TSTATE_REGNUM
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
  return ((regno >= FP0_REGNUM && regno <= (FP0_REGNUM + 47))
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
      union {
	struct reg32 regs32;
	struct reg64 regs64;
      } regs;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &regs, 0) == -1)
        perror_with_name ("Couldn't get registers");

      if (gdbarch_ptr_bit (current_gdbarch) == 32)
        sparcnbsd_supply_reg32 ((char *) &regs.regs32, regno);
      else
	sparcnbsd_supply_reg64 ((char *) &regs.regs64, regno);
      if (regno != -1)
	return;
    }

  if (regno == -1 || getfpregs_supplies (regno))
    {
      union {
        struct fpreg32 fpregs32;
	struct fpreg64 fpregs64;
      } fpregs;

      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &fpregs, 0) == -1)
        perror_with_name ("Couldn't get floating point registers");

      if (gdbarch_ptr_bit (current_gdbarch) == 32)
        sparcnbsd_supply_fpreg32 ((char *) &fpregs.fpregs32, regno);
      else
        sparcnbsd_supply_fpreg64 ((char *) &fpregs.fpregs64, regno);
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
      union {
	struct reg32 regs32;
	struct reg64 regs64;
      } regs;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &regs, 0) == -1)
	perror_with_name ("Couldn't get registers");

      if (gdbarch_ptr_bit (current_gdbarch) == 32)
        sparcnbsd_fill_reg32 ((char *) &regs.regs32, regno);
      else
	sparcnbsd_fill_reg64 ((char *) &regs.regs64, regno);

      if (ptrace (PT_SETREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &regs, 0) == -1)
	perror_with_name ("Couldn't write registers");

      /* Deal with the stack regs.  */
      if (regno == -1 || regno == SP_REGNUM
	  || (regno >= L0_REGNUM && regno <= I7_REGNUM))
	{
	  CORE_ADDR sp = read_register (SP_REGNUM);
	  int i;
	  char buf[8];

	  if (sp & 1)
	    {
	      /* Registers are 64-bit.  */
	      sp += 2047;

	      for (i = L0_REGNUM; i <= I7_REGNUM; i++)
		{
		  if (regno == -1 || regno == SP_REGNUM || regno == i)
		    {
		      regcache_collect (i, buf);
		      target_write_memory (sp + ((i - L0_REGNUM) * 8),
					   buf, sizeof (buf));
		    }
		}
            }
	  else
	    {
	      /* Registers are 32-bit.  Toss any sign-extension of the stack
		 pointer.

		 FIXME: We don't currently handle 32-bit code in a binary
		 that indicated LP64.  Do we have to care about that?  */
              if (gdbarch_ptr_bit (current_gdbarch) != 32)
		internal_error
		    (__FILE__, __LINE__,
		    "store_inferior_registers: 32-bit code in 64-bit inferior");

	      sp &= 0xffffffffUL;
	      for (i = L0_REGNUM; i <= I7_REGNUM; i++)
	        {
	          if (regno == -1 || regno == SP_REGNUM || regno == i)
		    {
		      regcache_collect (i, buf);
		      target_write_memory (sp + ((i - L0_REGNUM) * 4), buf, 4);
		    }
		}
	    }
	}

      if (regno != -1)
	return;
    }

  if (regno == -1 || getfpregs_supplies (regno))
    {
      union {
	struct fpreg32 fpregs32;
	struct fpreg64 fpregs64;
      } fpregs;

      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &fpregs, 0) == -1)
	perror_with_name ("Couldn't get floating point registers");

      if (gdbarch_ptr_bit (current_gdbarch) == 32)
        sparcnbsd_fill_fpreg32 ((char *) &fpregs.fpregs32, regno);
      else
	sparcnbsd_fill_fpreg64 ((char *) &fpregs.fpregs64, regno);
      
      if (ptrace (PT_SETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_ARG3_TYPE) &fpregs, 0) == -1)
	perror_with_name ("Couldn't write floating point registers");

      if (regno != -1)
	return;
    }
}
