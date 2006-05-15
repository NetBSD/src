/* Target-dependent code for GNU/Linux Super-H.

   Copyright 2005 Free Software Foundation, Inc.

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
#include "osabi.h"

#include "solib-svr4.h"

static void
sh_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern void _initialize_sh_linux_tdep (void);

void
_initialize_sh_linux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_sh, 0, GDB_OSABI_LINUX, sh_linux_init_abi);
}
