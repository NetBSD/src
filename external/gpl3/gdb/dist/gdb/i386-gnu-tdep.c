/* Target-dependent code for the GNU Hurd.
   Copyright (C) 2002-2017 Free Software Foundation, Inc.

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
#include "osabi.h"
#include "solib-svr4.h"

#include "i386-tdep.h"

/* From <sys/ucontext.h>.  */
static int i386gnu_gregset_reg_offset[] =
{
  11 * 4,		/* %eax */
  10 * 4,		/* %ecx */
  9 * 4,		/* %edx */
  8 * 4,		/* %ebx */
  17 * 4,		/* %uesp */
  6 * 4,		/* %ebp */
  5 * 4,		/* %esi */
  4 * 4,		/* %edi */
  14 * 4,		/* %eip */
  16 * 4,		/* %efl */
  15 * 4,		/* %cs */
  18 * 4,		/* %ss */
  3 * 4,		/* %ds */
  2 * 4,		/* %es */
  1 * 4,		/* %fs */
  0 * 4,		/* %gs */
};

static void
i386gnu_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* GNU uses ELF.  */
  i386_elf_init_abi (info, gdbarch);

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  tdep->gregset_reg_offset = i386gnu_gregset_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386gnu_gregset_reg_offset);
  tdep->sizeof_gregset = 19 * 4;

  tdep->jb_pc_offset = 20;	/* From <bits/setjmp.h>.  */
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern void _initialize_i386gnu_tdep (void);

void
_initialize_i386gnu_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_HURD, i386gnu_init_abi);
}
