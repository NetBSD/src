/* Ravenscar PowerPC target support.

   Copyright (C) 2011-2017 Free Software Foundation, Inc.

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
#include "ppc-tdep.h"
#include "inferior.h"
#include "ravenscar-thread.h"
#include "ppc-ravenscar-thread.h"

#define NO_OFFSET -1

/* See ppc-tdep.h for register numbers.  */

static const int powerpc_context_offsets[] =
{
  /* R0 - R32 */
  NO_OFFSET, 0,         4,         NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, 8,         12,        16,
  20,        24,        28,        32,
  36,        40,        44,        48,
  52,        56,        60,        64,
  68,        72,        76,        80,

  /* F0 - F31 */
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, 96,        104,
  112,       120,       128,       136,
  144,       152,       160,       168,
  176,       184,       192,       200,
  208,       216,       224,       232,

  /* PC, MSR, CR, LR */
  88,        NO_OFFSET, 84,        NO_OFFSET,

  /* CTR, XER, FPSCR  */
  NO_OFFSET, NO_OFFSET, 240
};

static const int e500_context_offsets[] =
{
  /* R0 - R32 */
  NO_OFFSET, 4,         12,        NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, 20,        28,        36,
  44,        52,        60,        68,
  76,        84,        92,        100,
  108,       116,       124,       132,
  140,       148,       156,       164,

  /* F0 - F31 */
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,

  /* PC, MSR, CR, LR */
  172,       NO_OFFSET, 168,       NO_OFFSET,

  /* CTR, XER, FPSCR, MQ  */
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,

  /* Upper R0-R32.  */
  NO_OFFSET, 0,         8,        NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, 16,        24,        32,
  40,        48,        56,        64,
  72,        80,        88,        96,
  104,       112,       120,       128,
  136,       144,       152,       160,

  /* ACC, FSCR */
  NO_OFFSET, 176
};

/* The register layout info.  */

struct ravenscar_reg_info
{
  /* A table providing the offset relative to the context structure
     where each register is saved.  */
  const int *context_offsets;

  /* The number of elements in the context_offsets table above.  */
  int context_offsets_size;
};

/* supply register REGNUM, which has been saved on REGISTER_ADDR, to the
   regcache.  */

static void
supply_register_at_address (struct regcache *regcache, int regnum,
                            CORE_ADDR register_addr)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int buf_size = register_size (gdbarch, regnum);
  gdb_byte *buf;

  buf = (gdb_byte *) alloca (buf_size);
  read_memory (register_addr, buf, buf_size);
  regcache_raw_supply (regcache, regnum, buf);
}

/* Return true if, for a non-running thread, REGNUM has been saved on the
   Thread_Descriptor.  */

static int
register_in_thread_descriptor_p (const struct ravenscar_reg_info *reg_info,
				 int regnum)
{
  return (regnum < reg_info->context_offsets_size
	  && reg_info->context_offsets[regnum] != NO_OFFSET);
}

/* to_fetch_registers when inferior_ptid is different from the running
   thread.  */

static void
ppc_ravenscar_generic_fetch_registers
  (const struct ravenscar_reg_info *reg_info,
   struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  const int num_regs = gdbarch_num_regs (gdbarch);
  int current_regnum;
  CORE_ADDR current_address;
  CORE_ADDR thread_descriptor_address;

  /* The tid is the thread_id field, which is a pointer to the thread.  */
  thread_descriptor_address = (CORE_ADDR) ptid_get_tid (inferior_ptid);

  /* Read registers.  */
  for (current_regnum = 0; current_regnum < num_regs; current_regnum++)
    {
      if (register_in_thread_descriptor_p (reg_info, current_regnum))
        {
          current_address = thread_descriptor_address
            + reg_info->context_offsets[current_regnum];
          supply_register_at_address (regcache, current_regnum,
                                      current_address);
        }
    }
}

/* to_prepare_to_store when inferior_ptid is different from the running
   thread.  */

static void
ppc_ravenscar_generic_prepare_to_store (struct regcache *regcache)
{
  /* Nothing to do.  */
}

/* to_store_registers when inferior_ptid is different from the running
   thread.  */

static void
ppc_ravenscar_generic_store_registers
  (const struct ravenscar_reg_info *reg_info,
   struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int buf_size = register_size (gdbarch, regnum);
  gdb_byte buf[buf_size];
  ULONGEST register_address;

  if (register_in_thread_descriptor_p (reg_info, regnum))
    register_address
      = ptid_get_tid (inferior_ptid) + reg_info->context_offsets [regnum];
  else
    return;

  regcache_raw_collect (regcache, regnum, buf);
  write_memory (register_address,
                buf,
                buf_size);
}

/* The ravenscar_reg_info for most PowerPC targets.  */

static const struct ravenscar_reg_info ppc_reg_info =
{
  powerpc_context_offsets,
  ARRAY_SIZE (powerpc_context_offsets),
};

/* Implement the to_fetch_registers ravenscar_arch_ops method
   for most PowerPC targets.  */

static void
ppc_ravenscar_powerpc_fetch_registers (struct regcache *regcache, int regnum)
{
  ppc_ravenscar_generic_fetch_registers (&ppc_reg_info, regcache, regnum);
}

/* Implement the to_store_registers ravenscar_arch_ops method
   for most PowerPC targets.  */

static void
ppc_ravenscar_powerpc_store_registers (struct regcache *regcache, int regnum)
{
  ppc_ravenscar_generic_store_registers (&ppc_reg_info, regcache, regnum);
}

/* The ravenscar_arch_ops vector for most PowerPC targets.  */

static struct ravenscar_arch_ops ppc_ravenscar_powerpc_ops =
{
  ppc_ravenscar_powerpc_fetch_registers,
  ppc_ravenscar_powerpc_store_registers,
  ppc_ravenscar_generic_prepare_to_store
};

/* Register ppc_ravenscar_powerpc_ops in GDBARCH.  */

void
register_ppc_ravenscar_ops (struct gdbarch *gdbarch)
{
  set_gdbarch_ravenscar_ops (gdbarch, &ppc_ravenscar_powerpc_ops);
}

/* The ravenscar_reg_info for E500 targets.  */

static const struct ravenscar_reg_info e500_reg_info =
{
  e500_context_offsets,
  ARRAY_SIZE (e500_context_offsets),
};

/* Implement the to_fetch_registers ravenscar_arch_ops method
   for E500 targets.  */

static void
ppc_ravenscar_e500_fetch_registers (struct regcache *regcache, int regnum)
{
  ppc_ravenscar_generic_fetch_registers (&e500_reg_info, regcache, regnum);
}

/* Implement the to_store_registers ravenscar_arch_ops method
   for E500 targets.  */

static void
ppc_ravenscar_e500_store_registers (struct regcache *regcache, int regnum)
{
  ppc_ravenscar_generic_store_registers (&e500_reg_info, regcache, regnum);
}

/* The ravenscar_arch_ops vector for E500 targets.  */

static struct ravenscar_arch_ops ppc_ravenscar_e500_ops =
{
  ppc_ravenscar_e500_fetch_registers,
  ppc_ravenscar_e500_store_registers,
  ppc_ravenscar_generic_prepare_to_store
};

/* Register ppc_ravenscar_e500_ops in GDBARCH.  */

void
register_e500_ravenscar_ops (struct gdbarch *gdbarch)
{
  set_gdbarch_ravenscar_ops (gdbarch, &ppc_ravenscar_e500_ops);
}
