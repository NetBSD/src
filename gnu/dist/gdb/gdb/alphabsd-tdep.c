/* Common target dependent code for GDB on Alpha systems running BSD.
   Copyright 2000, 2001, 2002 Free Software Foundation, Inc.

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
#include "regcache.h"

#include "alpha-tdep.h"
#include "alphabsd-tdep.h"

/* Number of general-purpose registers.  */
#define NUM_GREGS 32

/* Number of floating-point registers.  */
#define NUM_FPREGS 31

/* Conviently, GDB uses the same register numbering as the
   ptrace register structure used by BSD on Alpha.  */

void
alphabsd_supply_reg (char *regs, int regno)
{
  int i;

  for (i = 0; i < NUM_GREGS; i++)
    {
      if (i == regno || regno == -1)
	{
	  if (CANNOT_FETCH_REGISTER (i))
	    supply_register (i, NULL);
	  else
	    supply_register (i, regs + (i * 8));
	}
    }

  /* The PC travels in the ZERO slot.  */
  if (regno == PC_REGNUM || regno == -1)
    supply_register (PC_REGNUM, regs + (31 * 8));
}

void
alphabsd_fill_reg (char *regs, int regno)
{
  int i;

  for (i = 0; i < NUM_GREGS; i++)
    if ((regno == i || regno == -1) && ! CANNOT_STORE_REGISTER (i))
      regcache_collect (i, regs + (i * 8));

  /* The PC travels in the ZERO slot.  */
  if (regno == PC_REGNUM || regno == -1)
    regcache_collect (PC_REGNUM, regs + (31 * 8));
}

void
alphabsd_supply_fpreg (char *fpregs, int regno)
{
  int i;

  for (i = FP0_REGNUM; i < FP0_REGNUM + NUM_FPREGS; i++)
    {
      if (i == regno || regno == -1)
	{
	  if (CANNOT_FETCH_REGISTER (i))
	    supply_register (i, NULL);
	  else
	    supply_register (i, fpregs + ((i - FP0_REGNUM) * 8));
	}
    }

  if (regno == ALPHA_FPCR_REGNUM || regno == -1)
    supply_register (ALPHA_FPCR_REGNUM, fpregs + (32 * 8));
}

void
alphabsd_fill_fpreg (char *fpregs, int regno)
{
  int i;

  for (i = FP0_REGNUM; i < FP0_REGNUM + NUM_FPREGS; i++)
    if ((regno == i || regno == -1) && ! CANNOT_STORE_REGISTER (i))
      regcache_collect (i, fpregs + ((i - FP0_REGNUM) * 8));

  if (regno == ALPHA_FPCR_REGNUM || regno == -1)
    regcache_collect (ALPHA_FPCR_REGNUM, fpregs + (32 * 8));
}
