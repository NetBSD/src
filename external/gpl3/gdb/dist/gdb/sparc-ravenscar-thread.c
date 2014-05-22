/* Ravenscar SPARC target support.

   Copyright (C) 2004-2013 Free Software Foundation, Inc.

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
#include "sparc-tdep.h"
#include "inferior.h"
#include "ravenscar-thread.h"
#include "sparc-ravenscar-thread.h"

static void sparc_ravenscar_fetch_registers (struct regcache *regcache,
                                             int regnum);
static void sparc_ravenscar_store_registers (struct regcache *regcache,
                                             int regnum);
static void sparc_ravenscar_prepare_to_store (struct regcache *regcache);

/* Register offsets from a referenced address (exempli gratia the
   Thread_Descriptor).  The referenced address depends on the register
   number.  The Thread_Descriptor layout and the stack layout are documented
   in the GNAT sources, in sparc-bb.h.  */

static const int sparc_register_offsets[] =
{
  /* G0 - G7 */
  -1,   0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
  /* O0 - O7 */
  0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C,
  /* L0 - L7 */
  0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C,
  /* I0 - I7 */
  0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
  /* F0 - F31 */
  0x50, 0x54, 0x58, 0x5C, 0x60, 0x64, 0x68, 0x6C,
  0x70, 0x74, 0x78, 0x7C, 0x80, 0x84, 0x88, 0x8C,
  0x90, 0x94, 0x99, 0x9C, 0xA0, 0xA4, 0xA8, 0xAC,
  0xB0, 0xB4, 0xBB, 0xBC, 0xC0, 0xC4, 0xC8, 0xCC,
  /* Y  PSR   WIM   TBR   PC    NPC   FPSR  CPSR */
  0x40, 0x20, 0x44, -1,   0x1C, -1,   0x4C, -1
};

/* supply register REGNUM, which has been saved on REGISTER_ADDR, to the
   regcache.  */

static void
supply_register_at_address (struct regcache *regcache, int regnum,
                            CORE_ADDR register_addr)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int buf_size = register_size (gdbarch, regnum);
  char *buf;

  buf = (char *) alloca (buf_size);
  read_memory (register_addr, buf, buf_size);
  regcache_raw_supply (regcache, regnum, buf);
}

/* Return true if, for a non-running thread, REGNUM has been saved on the
   stack.  */

static int
register_on_stack_p (int regnum)
{
  return (regnum >= SPARC_L0_REGNUM && regnum <= SPARC_L7_REGNUM)
    || (regnum >= SPARC_I0_REGNUM && regnum <= SPARC_I7_REGNUM);
}

/* Return true if, for a non-running thread, REGNUM has been saved on the
   Thread_Descriptor.  */

static int
register_in_thread_descriptor_p (int regnum)
{
  return (regnum >= SPARC_O0_REGNUM && regnum <= SPARC_O7_REGNUM)
    || (regnum == SPARC32_PSR_REGNUM)
    || (regnum >= SPARC_G1_REGNUM && regnum <= SPARC_G7_REGNUM)
    || (regnum == SPARC32_Y_REGNUM)
    || (regnum == SPARC32_WIM_REGNUM)
    || (regnum == SPARC32_FSR_REGNUM)
    || (regnum >= SPARC_F0_REGNUM && regnum <= SPARC_F0_REGNUM + 31)
    || (regnum == SPARC32_PC_REGNUM);
}

/* to_fetch_registers when inferior_ptid is different from the running
   thread.  */

static void
sparc_ravenscar_fetch_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  const int sp_regnum = gdbarch_sp_regnum (gdbarch);
  const int num_regs = gdbarch_num_regs (gdbarch);
  int current_regnum;
  CORE_ADDR current_address;
  CORE_ADDR thread_descriptor_address;
  ULONGEST stack_address;

  /* The tid is the thread_id field, which is a pointer to the thread.  */
  thread_descriptor_address = (CORE_ADDR) ptid_get_tid (inferior_ptid);

  /* Read the saved SP in the context buffer.  */
  current_address = thread_descriptor_address
    + sparc_register_offsets [sp_regnum];
  supply_register_at_address (regcache, sp_regnum, current_address);
  regcache_cooked_read_unsigned (regcache, sp_regnum, &stack_address);

  /* Read registers.  */
  for (current_regnum = 0; current_regnum < num_regs; current_regnum ++)
    {
      if (register_in_thread_descriptor_p (current_regnum))
        {
          current_address = thread_descriptor_address
            + sparc_register_offsets [current_regnum];
          supply_register_at_address (regcache, current_regnum,
                                      current_address);
        }
      else if (register_on_stack_p (current_regnum))
        {
          current_address = stack_address
            + sparc_register_offsets [current_regnum];
          supply_register_at_address (regcache, current_regnum,
                                      current_address);
        }
    }
}

/* to_prepare_to_store when inferior_ptid is different from the running
   thread.  */

static void
sparc_ravenscar_prepare_to_store (struct regcache *regcache)
{
  /* Nothing to do.  */
}

/* to_store_registers when inferior_ptid is different from the running
   thread.  */

static void
sparc_ravenscar_store_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int buf_size = register_size (gdbarch, regnum);
  char buf [buf_size];
  ULONGEST register_address;

  if (register_in_thread_descriptor_p (regnum))
    register_address =
      ptid_get_tid (inferior_ptid) + sparc_register_offsets [regnum];
  else if (register_on_stack_p (regnum))
    {
      regcache_cooked_read_unsigned (regcache, SPARC_SP_REGNUM,
                                     &register_address);
      register_address += sparc_register_offsets [regnum];
    }
  else
    return;

  regcache_raw_collect (regcache, regnum, buf);
  write_memory (register_address,
                buf,
                buf_size);
}

static struct ravenscar_arch_ops sparc_ravenscar_ops =
{
  sparc_ravenscar_fetch_registers,
  sparc_ravenscar_store_registers,
  sparc_ravenscar_prepare_to_store
};

/* Register ravenscar_arch_ops in GDBARCH.  */

void
register_sparc_ravenscar_ops (struct gdbarch *gdbarch)
{
  set_gdbarch_ravenscar_ops (gdbarch, &sparc_ravenscar_ops);
}
