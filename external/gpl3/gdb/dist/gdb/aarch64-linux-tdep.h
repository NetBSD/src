/* GNU/Linux on AArch64 target support, prototypes.

   Copyright (C) 2012-2013 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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

struct regcache;

extern void aarch64_linux_supply_gregset (struct regcache *regcache,
					  const gdb_byte *gregs_buf);
extern void aarch64_linux_supply_fpregset (struct regcache *regcache,
					   const gdb_byte *fpregs_buf);
