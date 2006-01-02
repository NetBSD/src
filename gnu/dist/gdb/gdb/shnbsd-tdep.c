/* Target-dependent code for SuperH running NetBSD, for GDB.
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
#include "gdbcore.h"
#include "inferior.h"
#include "regcache.h"
#include "value.h"

#include "solib-svr4.h"

#include "nbsd-tdep.h"
#include "sh-tdep.h"
#include "shnbsd-tdep.h"

/* Convert an r0-r15 register number into an offset into a ptrace
   register structure.  */
static const int regmap[] =
{
  (20 * 4),	/* r0 */
  (19 * 4),	/* r1 */
  (18 * 4),	/* r2 */ 
  (17 * 4),	/* r3 */ 
  (16 * 4),	/* r4 */
  (15 * 4),	/* r5 */
  (14 * 4),	/* r6 */
  (13 * 4),	/* r7 */
  (12 * 4),	/* r8 */ 
  (11 * 4),	/* r9 */
  (10 * 4),	/* r10 */
  ( 9 * 4),	/* r11 */
  ( 8 * 4),	/* r12 */
  ( 7 * 4),	/* r13 */
  ( 6 * 4),	/* r14 */
  ( 5 * 4),	/* r15 */
};

#define SIZEOF_STRUCT_REG (21 * 4)

void
shnbsd_supply_reg (char *regs, int regno)
{
  int i;

  if (regno == PC_REGNUM || regno == -1)
    supply_register (PC_REGNUM, regs + (0 * 4));

  if (regno == SR_REGNUM || regno == -1)
    supply_register (SR_REGNUM, regs + (1 * 4));

  if (regno == PR_REGNUM || regno == -1)
    supply_register (PR_REGNUM, regs + (2 * 4));

  if (regno == MACH_REGNUM || regno == -1)
    supply_register (MACH_REGNUM, regs + (3 * 4));

  if (regno == MACL_REGNUM || regno == -1)
    supply_register (MACL_REGNUM, regs + (4 * 4));

  if ((regno >= R0_REGNUM && regno <= (R0_REGNUM + 15)) || regno == -1)
    {
      for (i = R0_REGNUM; i <= (R0_REGNUM + 15); i++)
	if (regno == i || regno == -1)
          supply_register (i, regs + regmap[i - R0_REGNUM]);
    }
}

void
shnbsd_fill_reg (char *regs, int regno)
{
  int i;

  if (regno == PC_REGNUM || regno == -1)
    regcache_collect (PC_REGNUM, regs + (0 * 4));

  if (regno == SR_REGNUM || regno == -1)
    regcache_collect (SR_REGNUM, regs + (1 * 4));

  if (regno == PR_REGNUM || regno == -1)
    regcache_collect (PR_REGNUM, regs + (2 * 4));

  if (regno == MACH_REGNUM || regno == -1)
    regcache_collect (MACH_REGNUM, regs + (3 * 4));

  if (regno == MACL_REGNUM || regno == -1)
    regcache_collect (MACL_REGNUM, regs + (4 * 4));

  if ((regno >= R0_REGNUM && regno <= (R0_REGNUM + 15)) || regno == -1)
    {
      for (i = R0_REGNUM; i <= (R0_REGNUM + 15); i++)
	if (regno == i || regno == -1)
          regcache_collect (i, regs + regmap[i - R0_REGNUM]);
    }
}

static void
fetch_core_registers (char *core_reg_sect, unsigned core_reg_size,
                      int which, CORE_ADDR ignore)
{
  /* We get everything from the .reg section.  */
  if (which != 0)
    return;

  if (core_reg_size < SIZEOF_STRUCT_REG)
    {
      warning ("Wrong size register set in core file.");
      return;
    }

  /* Integer registers.  */
  shnbsd_supply_reg (core_reg_sect, -1);
}

static void
fetch_elfcore_registers (char *core_reg_sect, unsigned core_reg_size,
                         int which, CORE_ADDR ignore)
{
  switch (which)
    {
    case 0:  /* Integer registers.  */
      if (core_reg_size != SIZEOF_STRUCT_REG)
	warning ("Wrong size register set in core file.");
      else
	shnbsd_supply_reg (core_reg_sect, -1);
      break;

    default:
      /* Don't know what kind of register request this is; just ignore it.  */
      break;
    }
}

static struct core_fns shnbsd_core_fns =
{
  bfd_target_unknown_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};

static struct core_fns shnbsd_elfcore_fns =
{
  bfd_target_elf_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_elfcore_registers,		/* core_read_registers */
  NULL					/* next */
};

static int
shnbsd_pc_in_sigtramp (CORE_ADDR pc, char *func_name)
{
  /* FIXME: Need to add support for kernel-provided signal trampolines.  */
  return (nbsd_pc_in_sigtramp (pc, func_name));
}


/* Hitachi SH instruction encoding masks and opcodes */

/*
 *	xxxx x--x ---- ----	0xf900 mask
 *	1001 1**1 ---- ----	0x8900 match
 * BT	1000 1001 dddd dddd
 * BF	1000 1011 dddd dddd
 * BT/S	1000 1101 dddd dddd
 * BF/S	1000 1111 dddd dddd
 *	---- --x- ---- ----	T or F?
 *	---- -x-- ---- ----	has delay slot
 */
#define CONDITIONAL_BRANCH_P(insn) (((insn) & 0xf900) == 0x8900)
#define CONDITIONAL_BRANCH_TAKEN_P(insn, sr) ((((insn) >> 9) ^ sr) & 0x1)
#define CONDITIONAL_BRANCH_SLOT_P(insn) (((insn) & 0x0400) != 0)

/*
 *	xxx- ---- ---- ----	0xe000 mask
 *	101* ---- ---- ----	0xa000 match
 * BRA	1010 dddd dddd dddd
 * BSR	1011 dddd dddd dddd
 */
#define BRANCH_P(insn) (((insn) & 0xe000) == 0xa000)

/*
 *	xxxx ---- xx-x xxxx	0xf0df mask
 *	0000 ---- 00*0 0011	0x0003 match
 * BSRF	0000 rrrr 0000 0011
 * BRAF	0000 rrrr 0010 0011
 */
#define BRANCH_FAR_P(insn) (((insn) & 0xf0df) == 0x0003)
#define BRANCH_FAR_REG(insn) (((insn) >> 8) & 0xf)

/*
 *	xxxx ---- xx-x xxxx	0xf0df mask
 *	0100 ---- 00*0 1011	0x400b match
 * JSR	0100 rrrr 0000 1011
 * JMP	0100 rrrr 0010 1011
 */
#define JUMP_P(insn) (((insn) & 0xf0df) == 0x400b)
#define JUMP_REG(insn) (((insn) >> 8) & 0xf)

/*
 * RTS	0000 0000 0000 1011
 * RTE	0000 0000 0010 1011
 */
#define RTS_INSN 0x000b
#define RTE_INSN 0x002b

/*
 *	xxxx xxxx ---- ----	0xff00 mask
 *TRAPA 1100 0011 tttt tttt	0xc300 match
 */
#define TRAPA_P(insn) (((insn) & 0xff00) == 0xc300)


/* signed 8-bit displacement */
static int
shnbsd_displacement_8 (unsigned short insn)
{
  int displacement;

  if (insn & 0x80)
    displacement = insn | 0xffffff00;
  else
    displacement = insn & 0x000000ff;

  return displacement;
}

/* signed 12-bit displacement */
static int
shnbsd_displacement_12 (unsigned short insn)
{
  int displacement;

  if (insn & 0x800)
    displacement = insn | 0xfffff000;
  else
    displacement = insn & 0x00000fff;

  return displacement;
}

static CORE_ADDR
shnbsd_get_next_pc (CORE_ADDR pc)
{
  unsigned short insn;
  ULONGEST sr;
  int displacement;
  int reg;
  CORE_ADDR next_pc;
  int delay_slot;

  insn = read_memory_integer (pc, sizeof (insn));
  delay_slot = 0;

  /* As we cannot step through the delay slot, we break at the target
     address of the control transfer.  One tricky case is when the
     target of the jump is the delay slot of that same instruction
     (e.g. PLT entries use such code).  In that case we cannot set the
     break to the target, as trapa is illegal in the delay slot.  Set
     break to the next instruction instead, we are guaranteed to
     arrive there, as control transfers are illegal in the delay
     slot. */

  /* BT, BF, BT/S, BF/S */
  if (CONDITIONAL_BRANCH_P(insn))
    {
      sr = read_register (gdbarch_tdep (current_gdbarch)->SR_REGNUM);

      if (!CONDITIONAL_BRANCH_TAKEN_P(insn, sr))
	next_pc = pc + 2;
      else
	{
	  displacement = shnbsd_displacement_8 (insn);

	  next_pc = pc + 4 + (displacement << 1);
	  delay_slot = CONDITIONAL_BRANCH_SLOT_P(insn);
	}
    }

  /* BRA, BSR */
  else if (BRANCH_P(insn))
    {
      displacement = shnbsd_displacement_12 (insn);

      next_pc = pc + 4 + (displacement << 1);
      delay_slot = 1;
    }

  /* BRAF, BSRF */
  else if (BRANCH_FAR_P(insn))
    {
      displacement = read_register (BRANCH_FAR_REG(insn));

      next_pc = pc + 4 + displacement;
      delay_slot = 1;
    }

  /* JMP, JSR */
  else if (JUMP_P(insn))
    {
      next_pc = read_register (JUMP_REG(insn));
      delay_slot = 1;
    }

  /* RTS */
  else if (insn == RTS_INSN)
    {
      next_pc = read_register (gdbarch_tdep (current_gdbarch)->PR_REGNUM);
      delay_slot = 1;
    }

  /* RTE - XXX: privileged */
  else if (insn == RTE_INSN)
    {
      next_pc = read_register (gdbarch_tdep (current_gdbarch)->SPC_REGNUM);
      delay_slot = 1;
    }

  /* TRAPA */
  else if (TRAPA_P(insn))
    next_pc = pc + 2;		/* XXX: after return from syscall */

  /* not a control transfer instruction */
  else
    next_pc = pc + 2;

  /* jumping to our own delay slot? */
  if (delay_slot && next_pc == pc + 2)
    next_pc += 2;

  return next_pc;
}

/* Single step (in a painstaking fashion) by inspecting the current
   instruction and setting a breakpoint on the "next" instruction
   which would be executed.
 */
void
shnbsd_software_single_step (enum target_signal ignore,
			     int insert_breakpoints_p)
{
  static CORE_ADDR next_pc;
  typedef char binsn_quantum[BREAKPOINT_MAX];
  static binsn_quantum break_mem;
  CORE_ADDR pc;

  if (insert_breakpoints_p)
    {
      pc = read_pc ();
      next_pc = shnbsd_get_next_pc (pc);

      target_insert_breakpoint (next_pc, break_mem);
    }
  else
    {
      target_remove_breakpoint (next_pc, break_mem);
      write_pc (next_pc);
    }
}

static void
shnbsd_init_abi (struct gdbarch_info info,
                  struct gdbarch *gdbarch)
{
  set_gdbarch_pc_in_sigtramp (gdbarch, shnbsd_pc_in_sigtramp);

  /* NetBSD SuperH ports do not provide single step support via ptrace(2);
     we must use software single-stepping.  */
  set_gdbarch_software_single_step (gdbarch, shnbsd_software_single_step);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
		                nbsd_ilp32_solib_svr4_fetch_link_map_offsets);
}

void
_initialize_shnbsd_tdep (void)
{
  add_core_fns (&shnbsd_core_fns);
  add_core_fns (&shnbsd_elfcore_fns);

  gdbarch_register_osabi (bfd_arch_sh, GDB_OSABI_NETBSD_ELF, shnbsd_init_abi);
}
