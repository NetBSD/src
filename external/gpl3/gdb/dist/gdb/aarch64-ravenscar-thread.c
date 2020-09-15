/* Ravenscar Aarch64 target support.

   Copyright (C) 2017-2020 Free Software Foundation, Inc.

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
#include "aarch64-tdep.h"
#include "inferior.h"
#include "ravenscar-thread.h"
#include "aarch64-ravenscar-thread.h"
#include "gdbarch.h"

#define NO_OFFSET -1

/* See aarch64-tdep.h for register numbers.  */

static const int aarch64_context_offsets[] =
{
  /* X0 - X28 */
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, 0,
  8,         16,        24,        32,
  40,        48,        56,        64,
  72,

  /* FP, LR, SP, PC, CPSR */
  /* Note that as task switch is synchronous, PC is in fact the LR here */
  80,        88,        96,        88,
  NO_OFFSET,

  /* Q0 - Q31 */
  112,       128,       144,       160,
  176,       192,       208,       224,
  240,       256,       272,       288,
  304,       320,       336,       352,
  368,       384,       400,       416,
  432,       448,       464,       480,
  496,       512,       528,       544,
  560,       576,       592,       608,

  /* FPSR, FPCR */
  104,       108,

  /* FPU Saved field */
  624
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
  struct gdbarch *gdbarch = regcache->arch ();
  int buf_size = register_size (gdbarch, regnum);
  gdb_byte *buf;

  buf = (gdb_byte *) alloca (buf_size);
  read_memory (register_addr, buf, buf_size);
  regcache->raw_supply (regnum, buf);
}

/* Return true if, for a non-running thread, REGNUM has been saved on the
   Thread_Descriptor.  */

static int
register_in_thread_descriptor_p (const struct ravenscar_reg_info *reg_info,
				 int regnum)
{
  /* Check FPU registers */
  return (regnum < reg_info->context_offsets_size
	  && reg_info->context_offsets[regnum] != NO_OFFSET);
}

/* to_fetch_registers when inferior_ptid is different from the running
   thread.  */

static void
aarch64_ravenscar_generic_fetch_registers
  (const struct ravenscar_reg_info *reg_info,
   struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const int num_regs = gdbarch_num_regs (gdbarch);
  int current_regnum;
  CORE_ADDR current_address;
  CORE_ADDR thread_descriptor_address;

  /* The tid is the thread_id field, which is a pointer to the thread.  */
  thread_descriptor_address = (CORE_ADDR) inferior_ptid.tid ();

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

/* to_store_registers when inferior_ptid is different from the running
   thread.  */

static void
aarch64_ravenscar_generic_store_registers
  (const struct ravenscar_reg_info *reg_info,
   struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int buf_size = register_size (gdbarch, regnum);
  gdb_byte buf[buf_size];
  ULONGEST register_address;

  if (register_in_thread_descriptor_p (reg_info, regnum))
    register_address
      = inferior_ptid.tid () + reg_info->context_offsets [regnum];
  else
    return;

  regcache->raw_collect (regnum, buf);
  write_memory (register_address,
                buf,
                buf_size);
}

/* The ravenscar_reg_info for most Aarch64 targets.  */

static const struct ravenscar_reg_info aarch64_reg_info =
{
  aarch64_context_offsets,
  ARRAY_SIZE (aarch64_context_offsets),
};

struct aarch64_ravenscar_ops : public ravenscar_arch_ops
{
  void fetch_registers (struct regcache *regcache, int regnum) override
  {
    aarch64_ravenscar_generic_fetch_registers
      (&aarch64_reg_info, regcache, regnum);
  }

  void store_registers (struct regcache *regcache, int regnum) override
  {
    aarch64_ravenscar_generic_store_registers
      (&aarch64_reg_info, regcache, regnum);
  }
};

/* The ravenscar_arch_ops vector for most Aarch64 targets.  */

static struct aarch64_ravenscar_ops aarch64_ravenscar_ops;

/* Register aarch64_ravenscar_ops in GDBARCH.  */

void
register_aarch64_ravenscar_ops (struct gdbarch *gdbarch)
{
  set_gdbarch_ravenscar_ops (gdbarch, &aarch64_ravenscar_ops);
}
