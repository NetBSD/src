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

#ifndef GDB_SOLIB_SVR4_LINUX_H
#define GDB_SOLIB_SVR4_LINUX_H

#include "solib-svr4.h"

/* solib_ops for ILP32 Linux systems.  */

struct linux_ilp32_svr4_solib_ops : public svr4_solib_ops
{
  using svr4_solib_ops::svr4_solib_ops;

  link_map_offsets *fetch_link_map_offsets () const override;
};

/* solib_ops for LP64 Linux systems.  */

struct linux_lp64_svr4_solib_ops : public svr4_solib_ops
{
  using svr4_solib_ops::svr4_solib_ops;

  link_map_offsets *fetch_link_map_offsets () const override;
};

/* Return a new solib_ops for ILP32 Linux systems.  */

extern solib_ops_up make_linux_ilp32_svr4_solib_ops (program_space *pspace);

/* Return a new solib_ops for LP64 Linux systems.  */

extern solib_ops_up make_linux_lp64_svr4_solib_ops (program_space *pspace);

#endif /* GDB_SOLIB_SVR4_LINUX_H */
