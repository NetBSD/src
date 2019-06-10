/* Target dependent code for GDB on TI C6x systems.

   Copyright (C) 2010-2017 Free Software Foundation, Inc.
   Contributed by Andrew Jenner <andrew@codesourcery.com>
   Contributed by Yao Qi <yao@codesourcery.com>

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

#include "server.h"
#include "linux-low.h"

#include "nat/gdb_ptrace.h"
#include <endian.h>

#include "gdb_proc_service.h"

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif

/* There are at most 69 registers accessible in ptrace.  */
#define TIC6X_NUM_REGS 69

#include <asm/ptrace.h>

/* Defined in auto-generated file tic6x-c64xp-linux.c.  */
void init_registers_tic6x_c64xp_linux (void);
extern const struct target_desc *tdesc_tic6x_c64xp_linux;

/* Defined in auto-generated file tic6x-c64x-linux.c.  */
void init_registers_tic6x_c64x_linux (void);
extern const struct target_desc *tdesc_tic6x_c64x_linux;

/* Defined in auto-generated file tic62x-c6xp-linux.c.  */
void init_registers_tic6x_c62x_linux (void);
extern const struct target_desc *tdesc_tic6x_c62x_linux;

union tic6x_register
{
  unsigned char buf[4];

  int reg32;
};

/* Return the ptrace ``address'' of register REGNO.  */

#if __BYTE_ORDER == __BIG_ENDIAN
static int tic6x_regmap_c64xp[] = {
  /* A0 - A15 */
  53, 52, 55, 54, 57, 56, 59, 58,
  61, 60, 63, 62, 65, 64, 67, 66,
  /* B0 - B15 */
  23, 22, 25, 24, 27, 26, 29, 28,
  31, 30, 33, 32, 35, 34, 69, 68,
  /* CSR PC */
  5, 4,
  /* A16 - A31 */
  37, 36, 39, 38, 41, 40, 43, 42,
  45, 44, 47, 46, 49, 48, 51, 50,
  /* B16 - B31 */
  7,  6,  9,  8,  11, 10, 13, 12,
  15, 14, 17, 16, 19, 18, 21, 20,
  /* TSR, ILC, RILC */
  1,  2, 3
};

static int tic6x_regmap_c64x[] = {
  /* A0 - A15 */
  51, 50, 53, 52, 55, 54, 57, 56,
  59, 58, 61, 60, 63, 62, 65, 64,
  /* B0 - B15 */
  21, 20, 23, 22, 25, 24, 27, 26,
  29, 28, 31, 30, 33, 32, 67, 66,
  /* CSR PC */
  3,  2,
  /* A16 - A31 */
  35, 34, 37, 36, 39, 38, 41, 40,
  43, 42, 45, 44, 47, 46, 49, 48,
  /* B16 - B31 */
  5,  4,  7,  6,  9,  8,  11, 10,
  13, 12, 15, 14, 17, 16, 19, 18,
  -1, -1, -1
};

static int tic6x_regmap_c62x[] = {
  /* A0 - A15 */
  19, 18, 21, 20, 23, 22, 25, 24,
  27, 26, 29, 28, 31, 30, 33, 32,
  /* B0 - B15 */
   5,  4,  7,  6,  9,  8, 11, 10,
  13, 12, 15, 14, 17, 16, 35, 34,
  /* CSR, PC */
  3, 2,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1
};

#else
static int tic6x_regmap_c64xp[] = {
  /* A0 - A15 */
  52, 53, 54, 55, 56, 57, 58, 59,
  60, 61, 62, 63, 64, 65, 66, 67,
  /* B0 - B15 */
  22, 23, 24, 25, 26, 27, 28, 29,
  30, 31, 32, 33, 34, 35, 68, 69,
  /* CSR PC */
   4,  5,
  /* A16 - A31 */
  36, 37, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51,
  /* B16 -B31 */
   6,  7,  8,  9, 10, 11, 12, 13,
  14, 15, 16, 17, 18, 19, 20, 31,
  /* TSR, ILC, RILC */
  0,  3, 2
};

static int tic6x_regmap_c64x[] = {
  /* A0 - A15 */
  50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, 64, 65,
  /* B0 - B15 */
  20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 32, 33, 66, 67,
  /* CSR PC */
  2,  3,
  /* A16 - A31 */
  34, 35, 36, 37, 38, 39, 40, 41,
  42, 43, 44, 45, 46, 47, 48, 49,
  /* B16 - B31 */
  4,  5,  6,  7,  8,  9,  10, 11,
  12, 13, 14, 15, 16, 17, 18, 19,
  -1, -1, -1
};

static int tic6x_regmap_c62x[] = {
  /* A0 - A15 */
  18, 19, 20, 21, 22, 23, 24, 25,
  26, 27, 28, 29, 30, 31, 32, 33,
  /* B0 - B15 */
  4,  5,  6,  7,  8,  9, 10, 11,
  12, 13, 14, 15, 16, 17, 34, 35,
  /* CSR PC */
  2,  3,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1
};

#endif

extern struct linux_target_ops the_low_target;

static int *tic6x_regmap;
static unsigned int tic6x_breakpoint;
#define tic6x_breakpoint_len 4

/* Implementation of linux_target_ops method "sw_breakpoint_from_kind".  */

static const gdb_byte *
tic6x_sw_breakpoint_from_kind (int kind, int *size)
{
  *size = tic6x_breakpoint_len;
  return (const gdb_byte *) &tic6x_breakpoint;
}

/* Forward definition.  */
static struct usrregs_info tic6x_usrregs_info;

static const struct target_desc *
tic6x_read_description (void)
{
  register unsigned int csr asm ("B2");
  unsigned int cpuid;
  const struct target_desc *tdesc;

  /* Determine the CPU we're running on to find the register order.  */
  __asm__ ("MVC .S2 CSR,%0" : "=r" (csr) :);
  cpuid = csr >> 24;
  switch (cpuid)
    {
    case 0x00: /* C62x */
    case 0x02: /* C67x */
      tic6x_regmap = tic6x_regmap_c62x;
      tic6x_breakpoint = 0x0000a122;  /* BNOP .S2 0,5 */
      tdesc = tdesc_tic6x_c62x_linux;
      break;
    case 0x03: /* C67x+ */
      tic6x_regmap = tic6x_regmap_c64x;
      tic6x_breakpoint = 0x0000a122;  /* BNOP .S2 0,5 */
      tdesc = tdesc_tic6x_c64x_linux;
      break;
    case 0x0c: /* C64x */
      tic6x_regmap = tic6x_regmap_c64x;
      tic6x_breakpoint = 0x0000a122;  /* BNOP .S2 0,5 */
      tdesc = tdesc_tic6x_c64x_linux;
      break;
    case 0x10: /* C64x+ */
    case 0x14: /* C674x */
    case 0x15: /* C66x */
      tic6x_regmap = tic6x_regmap_c64xp;
      tic6x_breakpoint = 0x56454314;  /* illegal opcode */
      tdesc = tdesc_tic6x_c64xp_linux;
      break;
    default:
      error ("Unknown CPU ID 0x%02x", cpuid);
    }
  tic6x_usrregs_info.regmap = tic6x_regmap;
  return tdesc;
}

static int
tic6x_cannot_fetch_register (int regno)
{
  return (tic6x_regmap[regno] == -1);
}

static int
tic6x_cannot_store_register (int regno)
{
  return (tic6x_regmap[regno] == -1);
}

static CORE_ADDR
tic6x_get_pc (struct regcache *regcache)
{
  union tic6x_register pc;

  collect_register_by_name (regcache, "PC", pc.buf);
  return pc.reg32;
}

static void
tic6x_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  union tic6x_register newpc;

  newpc.reg32 = pc;
  supply_register_by_name (regcache, "PC", newpc.buf);
}

static int
tic6x_breakpoint_at (CORE_ADDR where)
{
  unsigned int insn;

  (*the_target->read_memory) (where, (unsigned char *) &insn, 4);
  if (insn == tic6x_breakpoint)
    return 1;

  /* If necessary, recognize more trap instructions here.  GDB only uses the
     one.  */
  return 0;
}

/* Fetch the thread-local storage pointer for libthread_db.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  if (ptrace (PTRACE_GET_THREAD_AREA, lwpid, NULL, base) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) ((char *) *base - idx);

  return PS_OK;
}

static void
tic6x_collect_register (struct regcache *regcache, int regno,
			union tic6x_register *reg)
{
  union tic6x_register tmp_reg;

  collect_register (regcache, regno, &tmp_reg.reg32);
  reg->reg32 = tmp_reg.reg32;
}

static void
tic6x_supply_register (struct regcache *regcache, int regno,
		       const union tic6x_register *reg)
{
  int offset = 0;

  supply_register (regcache, regno, reg->buf + offset);
}

static void
tic6x_fill_gregset (struct regcache *regcache, void *buf)
{
  union tic6x_register *regset = buf;
  int i;

  for (i = 0; i < TIC6X_NUM_REGS; i++)
    if (tic6x_regmap[i] != -1)
      tic6x_collect_register (regcache, i, regset + tic6x_regmap[i]);
}

static void
tic6x_store_gregset (struct regcache *regcache, const void *buf)
{
  const union tic6x_register *regset = buf;
  int i;

  for (i = 0; i < TIC6X_NUM_REGS; i++)
    if (tic6x_regmap[i] != -1)
      tic6x_supply_register (regcache, i, regset + tic6x_regmap[i]);
}

static struct regset_info tic6x_regsets[] = {
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, TIC6X_NUM_REGS * 4, GENERAL_REGS,
    tic6x_fill_gregset, tic6x_store_gregset },
  NULL_REGSET
};

static void
tic6x_arch_setup (void)
{
  current_process ()->tdesc = tic6x_read_description ();
}

/* Support for hardware single step.  */

static int
tic6x_supports_hardware_single_step (void)
{
  return 1;
}

static struct regsets_info tic6x_regsets_info =
  {
    tic6x_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct usrregs_info tic6x_usrregs_info =
  {
    TIC6X_NUM_REGS,
    NULL, /* Set in tic6x_read_description.  */
  };

static struct regs_info regs_info =
  {
    NULL, /* regset_bitmap */
    &tic6x_usrregs_info,
    &tic6x_regsets_info
  };

static const struct regs_info *
tic6x_regs_info (void)
{
  return &regs_info;
}

struct linux_target_ops the_low_target = {
  tic6x_arch_setup,
  tic6x_regs_info,
  tic6x_cannot_fetch_register,
  tic6x_cannot_store_register,
  NULL, /* fetch_register */
  tic6x_get_pc,
  tic6x_set_pc,
  NULL, /* breakpoint_kind_from_pc */
  tic6x_sw_breakpoint_from_kind,
  NULL,
  0,
  tic6x_breakpoint_at,
  NULL, /* supports_z_point_type */
  NULL, /* insert_point */
  NULL, /* remove_point */
  NULL, /* stopped_by_watchpoint */
  NULL, /* stopped_data_address */
  NULL, /* collect_ptrace_register */
  NULL, /* supply_ptrace_register */
  NULL, /* siginfo_fixup */
  NULL, /* new_process */
  NULL, /* new_thread */
  NULL, /* new_fork */
  NULL, /* prepare_to_resume */
  NULL, /* process_qsupported */
  NULL, /* supports_tracepoints */
  NULL, /* get_thread_area */
  NULL, /* install_fast_tracepoint_jump_pad */
  NULL, /* emit_ops */
  NULL, /* get_min_fast_tracepoint_insn_len */
  NULL, /* supports_range_stepping */
  NULL, /* breakpoint_kind_from_current_state */
  tic6x_supports_hardware_single_step,
};

void
initialize_low_arch (void)
{
  /* Initialize the Linux target descriptions.  */
  init_registers_tic6x_c64xp_linux ();
  init_registers_tic6x_c64x_linux ();
  init_registers_tic6x_c62x_linux ();

  initialize_regsets_info (&tic6x_regsets_info);
}
