/* Target-dependent code for OpenBSD/i386.
   Copyright 1988, 1989, 1991, 1992, 1994, 1996, 2000, 2001, 2002
   Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "gdbcore.h"
#include "regcache.h"

#include "i386-tdep.h"
#include "i387-tdep.h"

/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_i386obsd_tdep (void);

#define SIZEOF_STRUCT_REG	(16 * 4)

static void
i386obsd_supply_reg (char *regs, int regno)
{
  int i;

  for (i = 0; i <= 15; i++)
    if (regno == i || regno == -1)
      supply_register (i, regs + i * 4);
}

static void
fetch_core_registers (char *core_reg_sect, unsigned core_reg_size, int which,
                      CORE_ADDR ignore)
{
  char *regs, *fsave;

  /* We get everything from one section.  */
  if (which != 0)
    return;

  if (core_reg_size < (SIZEOF_STRUCT_REG + 108))
    {
      warning ("Wrong size register set in core file.");
      return;
    }

  regs = core_reg_sect;
  fsave = core_reg_sect + SIZEOF_STRUCT_REG;

  /* Integer registers.  */
  i386obsd_supply_reg (regs, -1);

  /* Floating point registers.  */
  i387_supply_fsave (fsave);
}

static struct core_fns i386obsd_core_fns =
{
  bfd_target_unknown_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};


CORE_ADDR i386obsd_sigtramp_start = 0xbfbfdf20;
CORE_ADDR i386obsd_sigtramp_end = 0xbfbfdff0;

/* From <machine/signal.h>.  */
int i386obsd_sc_pc_offset = 44;
int i386obsd_sc_sp_offset = 56;

static void 
i386obsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Obviously OpenBSD is BSD-based.  */
  i386bsd_init_abi (info, gdbarch);

  /* OpenBSD uses -freg-struct-return by default.  */
  tdep->struct_return = reg_struct_return;

  /* OpenBSD uses a different memory layout.  */
  tdep->sigtramp_start = i386obsd_sigtramp_start;
  tdep->sigtramp_end = i386obsd_sigtramp_end;

  /* OpenBSD has a `struct sigcontext' that's different from the
     origional 4.3 BSD.  */
  tdep->sc_pc_offset = i386obsd_sc_pc_offset;
  tdep->sc_sp_offset = i386obsd_sc_sp_offset;
}

void
_initialize_i386obsd_tdep (void)
{
  add_core_fns (&i386obsd_core_fns);

  /* FIXME: kettenis/20021020: Since OpenBSD/i386 binaries are
     indistingushable from NetBSD/i386 a.out binaries, building a GDB
     that should support both these targets will probably not work as
     expected.  */
#define GDB_OSABI_OPENBSD_AOUT GDB_OSABI_NETBSD_AOUT

  gdbarch_register_osabi (bfd_arch_i386, GDB_OSABI_OPENBSD_AOUT,
			  i386obsd_init_abi);
}
