/* Target-dependent code for GNU/Linux on Xtensa processors.

   Copyright (C) 2007-2013 Free Software Foundation, Inc.

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
#include "linux-tdep.h"
#include "solib-svr4.h"
#include "symtab.h"

/* OS specific initialization of gdbarch.  */

static void
xtensa_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  linux_init_abi (info, gdbarch);

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_xtensa_linux_tdep;

void
_initialize_xtensa_linux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_xtensa, bfd_mach_xtensa, GDB_OSABI_LINUX,
			  xtensa_linux_init_abi);
}
