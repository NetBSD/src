/* Native-dependent code for AMD64.

   Copyright (C) 2003-2014 Free Software Foundation, Inc.

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
#include "gdbarch.h"
#include "regcache.h"

#include "gdb_assert.h"
#include <string.h>

#include "i386-tdep.h"
#include "amd64-tdep.h"
#include "amd64-nat.h"

/* The following bits of code help with implementing debugging 32-bit
   code natively on AMD64.  The idea is to define two mappings between
   the register number as used by GDB and the register set used by the
   host to represent the general-purpose registers; one for 32-bit
   code and one for 64-bit code.  The mappings are specified by the
   follwing variables and consist of an array of offsets within the
   register set indexed by register number, and the number of
   registers supported by the mapping.  We don't need mappings for the
   floating-point and SSE registers, since the difference between
   64-bit and 32-bit variants are negligable.  The difference in the
   number of SSE registers is already handled by the target code.  */

/* General-purpose register mapping for native 32-bit code.  */
int *amd64_native_gregset32_reg_offset;
int amd64_native_gregset32_num_regs = I386_NUM_GREGS;

/* General-purpose register mapping for native 64-bit code.  */
int *amd64_native_gregset64_reg_offset;
int amd64_native_gregset64_num_regs = AMD64_NUM_GREGS;

/* Return the offset of REGNUM within the appropriate native
   general-purpose register set.  */

static int
amd64_native_gregset_reg_offset (struct gdbarch *gdbarch, int regnum)
{
  int *reg_offset = amd64_native_gregset64_reg_offset;
  int num_regs = amd64_native_gregset64_num_regs;

  gdb_assert (regnum >= 0);

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      reg_offset = amd64_native_gregset32_reg_offset;
      num_regs = amd64_native_gregset32_num_regs;
    }

  if (num_regs > gdbarch_num_regs (gdbarch))
    num_regs = gdbarch_num_regs (gdbarch);

  if (regnum < num_regs && regnum < gdbarch_num_regs (gdbarch))
    return reg_offset[regnum];

  return -1;
}

/* Return whether the native general-purpose register set supplies
   register REGNUM.  */

int
amd64_native_gregset_supplies_p (struct gdbarch *gdbarch, int regnum)
{
  return (amd64_native_gregset_reg_offset (gdbarch, regnum) != -1);
}


/* Supply register REGNUM, whose contents are stored in GREGS, to
   REGCACHE.  If REGNUM is -1, supply all appropriate registers.  */

void
amd64_supply_native_gregset (struct regcache *regcache,
			     const void *gregs, int regnum)
{
  const char *regs = gregs;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int num_regs = amd64_native_gregset64_num_regs;
  int i;

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    num_regs = amd64_native_gregset32_num_regs;

  if (num_regs > gdbarch_num_regs (gdbarch))
    num_regs = gdbarch_num_regs (gdbarch);

  for (i = 0; i < num_regs; i++)
    {
      if (regnum == -1 || regnum == i)
	{
	  int offset = amd64_native_gregset_reg_offset (gdbarch, i);

	  if (offset != -1)
	    regcache_raw_supply (regcache, i, regs + offset);
	}
    }
}

/* Collect register REGNUM from REGCACHE and store its contents in
   GREGS.  If REGNUM is -1, collect and store all appropriate
   registers.  */

void
amd64_collect_native_gregset (const struct regcache *regcache,
			      void *gregs, int regnum)
{
  char *regs = gregs;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int num_regs = amd64_native_gregset64_num_regs;
  int i;

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      num_regs = amd64_native_gregset32_num_regs;

      /* Make sure %eax, %ebx, %ecx, %edx, %esi, %edi, %ebp, %esp and
         %eip get zero-extended to 64 bits.  */
      for (i = 0; i <= I386_EIP_REGNUM; i++)
	{
	  if (regnum == -1 || regnum == i)
	    memset (regs + amd64_native_gregset_reg_offset (gdbarch, i), 0, 8);
	}
      /* Ditto for %cs, %ss, %ds, %es, %fs, and %gs.  */
      for (i = I386_CS_REGNUM; i <= I386_GS_REGNUM; i++)
	{
	  if (regnum == -1 || regnum == i)
	    memset (regs + amd64_native_gregset_reg_offset (gdbarch, i), 0, 8);
	}
    }

  if (num_regs > gdbarch_num_regs (gdbarch))
    num_regs = gdbarch_num_regs (gdbarch);

  for (i = 0; i < num_regs; i++)
    {
      if (regnum == -1 || regnum == i)
	{
	  int offset = amd64_native_gregset_reg_offset (gdbarch, i);

	  if (offset != -1)
	    regcache_raw_collect (regcache, i, regs + offset);
	}
    }
}
