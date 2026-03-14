/* Target-dependent code for GNU/Linux using SVR4-style libraries.

   Copyright (C) 2025 Free Software Foundation, Inc.

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

#include "solib-svr4-linux.h"

/* See solib-svr4-linux.h.  */

solib_ops_up
make_linux_ilp32_svr4_solib_ops (program_space *pspace)
{
  return std::make_unique<linux_ilp32_svr4_solib_ops> (pspace);
}

/* See solib-svr4-linux.h.  */

link_map_offsets *
linux_ilp32_svr4_solib_ops::fetch_link_map_offsets () const
{
  static link_map_offsets lmo;
  static link_map_offsets *lmp = nullptr;

  if (lmp == nullptr)
    {
      lmp = &lmo;

      lmo.r_version_offset = 0;
      lmo.r_version_size = 4;
      lmo.r_map_offset = 4;
      lmo.r_brk_offset = 8;
      lmo.r_ldsomap_offset = -1;
      lmo.r_next_offset = 20;

      /* Everything we need is in the first 20 bytes.  */
      lmo.link_map_size = 20;
      lmo.l_addr_offset = 0;
      lmo.l_name_offset = 4;
      lmo.l_ld_offset = 8;
      lmo.l_next_offset = 12;
      lmo.l_prev_offset = 16;
    }

  return lmp;
}

/* See solib-svr4-linux.h.  */

solib_ops_up
make_linux_lp64_svr4_solib_ops (program_space *pspace)
{
  return std::make_unique<linux_lp64_svr4_solib_ops> (pspace);
}

/* See linux-tdep.h.  */

link_map_offsets *
linux_lp64_svr4_solib_ops::fetch_link_map_offsets () const
{
  static link_map_offsets lmo;
  static link_map_offsets *lmp = nullptr;

  if (lmp == nullptr)
    {
      lmp = &lmo;

      lmo.r_version_offset = 0;
      lmo.r_version_size = 4;
      lmo.r_map_offset = 8;
      lmo.r_brk_offset = 16;
      lmo.r_ldsomap_offset = -1;
      lmo.r_next_offset = 40;

      /* Everything we need is in the first 40 bytes.  */
      lmo.link_map_size = 40;
      lmo.l_addr_offset = 0;
      lmo.l_name_offset = 8;
      lmo.l_ld_offset = 16;
      lmo.l_next_offset = 24;
      lmo.l_prev_offset = 32;
    }

  return lmp;
}
