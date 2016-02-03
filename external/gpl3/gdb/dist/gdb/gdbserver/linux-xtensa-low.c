/* GNU/Linux/Xtensa specific low level interface, for the remote server for GDB.
   Copyright (C) 2007-2015 Free Software Foundation, Inc.

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

/* Defined in auto-generated file reg-xtensa.c.  */
void init_registers_xtensa (void);
extern const struct target_desc *tdesc_xtensa;

#include <asm/ptrace.h>
#include <xtensa-config.h>

#include "xtensa-xtregs.c"

enum regnum {
	R_PC=0,	R_PS,
	R_LBEG,	R_LEND,	R_LCOUNT,
	R_SAR,
	R_WS, R_WB,
	R_A0 = 64
};

static void
xtensa_fill_gregset (struct regcache *regcache, void *buf)
{
  elf_greg_t* rset = (elf_greg_t*)buf;
  const struct target_desc *tdesc = regcache->tdesc;
  int ar0_regnum;
  char *ptr;
  int i;

  /* Take care of AR registers.  */

  ar0_regnum = find_regno (tdesc, "ar0");
  ptr = (char*)&rset[R_A0];

  for (i = ar0_regnum; i < ar0_regnum + XCHAL_NUM_AREGS; i++)
    {
      collect_register (regcache, i, ptr);
      ptr += register_size (tdesc, i);
    }

  /* Loop registers, if hardware has it.  */

#if XCHAL_HAVE_LOOPS
  collect_register_by_name (regcache, "lbeg", (char*)&rset[R_LBEG]);
  collect_register_by_name (regcache, "lend", (char*)&rset[R_LEND]);
  collect_register_by_name (regcache, "lcount", (char*)&rset[R_LCOUNT]);
#endif

  collect_register_by_name (regcache, "sar", (char*)&rset[R_SAR]);
  collect_register_by_name (regcache, "pc", (char*)&rset[R_PC]);
  collect_register_by_name (regcache, "ps", (char*)&rset[R_PS]);
  collect_register_by_name (regcache, "windowbase", (char*)&rset[R_WB]);
  collect_register_by_name (regcache, "windowstart", (char*)&rset[R_WS]);
}

static void
xtensa_store_gregset (struct regcache *regcache, const void *buf)
{
  const elf_greg_t* rset = (const elf_greg_t*)buf;
  const struct target_desc *tdesc = regcache->tdesc;
  int ar0_regnum;
  char *ptr;
  int i;

  /* Take care of AR registers.  */

  ar0_regnum = find_regno (tdesc, "ar0");
  ptr = (char *)&rset[R_A0];

  for (i = ar0_regnum; i < ar0_regnum + XCHAL_NUM_AREGS; i++)
    {
      supply_register (regcache, i, ptr);
      ptr += register_size (tdesc, i);
    }

  /* Loop registers, if hardware has it.  */

#if XCHAL_HAVE_LOOPS
  supply_register_by_name (regcache, "lbeg", (char*)&rset[R_LBEG]);
  supply_register_by_name (regcache, "lend", (char*)&rset[R_LEND]);
  supply_register_by_name (regcache, "lcount", (char*)&rset[R_LCOUNT]);
#endif

  supply_register_by_name (regcache, "sar", (char*)&rset[R_SAR]);
  supply_register_by_name (regcache, "pc", (char*)&rset[R_PC]);
  supply_register_by_name (regcache, "ps", (char*)&rset[R_PS]);
  supply_register_by_name (regcache, "windowbase", (char*)&rset[R_WB]);
  supply_register_by_name (regcache, "windowstart", (char*)&rset[R_WS]);
}

/* Xtensa GNU/Linux PTRACE interface includes extended register set.  */

static void
xtensa_fill_xtregset (struct regcache *regcache, void *buf)
{
  const xtensa_regtable_t *ptr;

  for (ptr = xtensa_regmap_table; ptr->name; ptr++)
    {
      collect_register_by_name (regcache, ptr->name,
				(char*)buf + ptr->ptrace_offset);
    }
}

static void
xtensa_store_xtregset (struct regcache *regcache, const void *buf)
{
  const xtensa_regtable_t *ptr;

  for (ptr = xtensa_regmap_table; ptr->name; ptr++)
    {
      supply_register_by_name (regcache, ptr->name,
				(char*)buf + ptr->ptrace_offset);
    }
}

static struct regset_info xtensa_regsets[] = {
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, sizeof (elf_gregset_t),
    GENERAL_REGS,
    xtensa_fill_gregset, xtensa_store_gregset },
  { PTRACE_GETXTREGS, PTRACE_SETXTREGS, 0, XTENSA_ELF_XTREG_SIZE,
    EXTENDED_REGS,
    xtensa_fill_xtregset, xtensa_store_xtregset },
  { 0, 0, 0, -1, -1, NULL, NULL }
};

#if XCHAL_HAVE_BE
#define XTENSA_BREAKPOINT {0xd2,0x0f}
#else
#define XTENSA_BREAKPOINT {0x2d,0xf0}
#endif

static const unsigned char xtensa_breakpoint[] = XTENSA_BREAKPOINT;
#define xtensa_breakpoint_len 2

static CORE_ADDR
xtensa_get_pc (struct regcache *regcache)
{
  unsigned long pc;

  collect_register_by_name (regcache, "pc", &pc);
  return pc;
}

static void
xtensa_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  unsigned long newpc = pc;
  supply_register_by_name (regcache, "pc", &newpc);
}

static int
xtensa_breakpoint_at (CORE_ADDR where)
{
    unsigned long insn;

    (*the_target->read_memory) (where, (unsigned char *) &insn,
				xtensa_breakpoint_len);
    return memcmp((char *) &insn,
		  xtensa_breakpoint, xtensa_breakpoint_len) == 0;
}

static struct regsets_info xtensa_regsets_info =
  {
    xtensa_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct regs_info regs_info =
  {
    NULL, /* regset_bitmap */
    NULL, /* usrregs */
    &xtensa_regsets_info
  };

static void
xtensa_arch_setup (void)
{
  current_process ()->tdesc = tdesc_xtensa;
}

static const struct regs_info *
xtensa_regs_info (void)
{
  return &regs_info;
}

struct linux_target_ops the_low_target = {
  xtensa_arch_setup,
  xtensa_regs_info,
  0,
  0,
  NULL, /* fetch_register */
  xtensa_get_pc,
  xtensa_set_pc,
  xtensa_breakpoint,
  xtensa_breakpoint_len,
  NULL,
  0,
  xtensa_breakpoint_at,
};


void
initialize_low_arch (void)
{
  /* Initialize the Linux target descriptions.  */
  init_registers_xtensa ();

  initialize_regsets_info (&xtensa_regsets_info);
}
