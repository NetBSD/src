/* Target-dependent code for NetBSD/i386, for GDB.
   Copyright 1988, 1989, 1991, 1992, 1994, 1996, 2000, 2001
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
#include "gdbcore.h"
#include "regcache.h"
#include "target.h"
#include "breakpoint.h"
#include "value.h"
#include "osabi.h"

#include "nbsd-tdep.h"

#include "solib-svr4.h"

int
m68knbsd_use_struct_convention (int gcc_p, struct type *type)
{
  return !(TYPE_LENGTH (type) == 1
	   || TYPE_LENGTH (type) == 2
	   || TYPE_LENGTH (type) == 4
	   || TYPE_LENGTH (type) == 8);
}

static int
m68knbsd_pc_in_sigtramp (CORE_ADDR pc, char *func_name)
{
  /* FIXME: Need to add support for kernel-provided signal trampolines.  */
  return (nbsd_pc_in_sigtramp (pc, func_name));
}

static void
m68knbsd_init_abi (struct gdbarch_info info,
                   struct gdbarch *gdbarch)
{
  /* Stop at main.  */
  set_gdbarch_frame_chain_valid (gdbarch, generic_func_frame_chain_valid);

  set_gdbarch_pc_in_sigtramp (gdbarch, m68knbsd_pc_in_sigtramp);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
                                nbsd_ilp32_solib_svr4_fetch_link_map_offsets);
}

void
_initialize_m68knbsd_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_m68k, GDB_OSABI_NETBSD_ELF,
			  m68knbsd_init_abi);
}
