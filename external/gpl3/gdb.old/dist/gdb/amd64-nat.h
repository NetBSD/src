/* Native-dependent code for AMD64.

   Copyright (C) 2003-2017 Free Software Foundation, Inc.

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

#ifndef AMD64_NAT_H
#define AMD64_NAT_H 1

struct regcache;

/* General-purpose register set description for native 32-bit code.  */
extern int *amd64_native_gregset32_reg_offset;
extern int amd64_native_gregset32_num_regs;

/* General-purpose register set description for native 64-bit code.  */
extern int *amd64_native_gregset64_reg_offset;
extern int amd64_native_gregset64_num_regs;

/* Return whether the native general-purpose register set supplies
   register REGNUM.  */

extern int amd64_native_gregset_supplies_p (struct gdbarch *gdbarch,
					    int regnum);

/* Supply register REGNUM, whose contents are store in BUF, to
   REGCACHE.  If REGNUM is -1, supply all appropriate registers.  */

extern void amd64_supply_native_gregset (struct regcache *regcache,
					 const void *gregs, int regnum);

/* Collect register REGNUM from REGCACHE and store its contents in
   GREGS.  If REGNUM is -1, collect and store all appropriate
   registers.  */

extern void amd64_collect_native_gregset (const struct regcache *regcache,
					  void *gregs, int regnum);

/* Create a prototype *BSD/amd64 target.  The client can override it
   with local methods.  */

extern struct target_ops *amd64bsd_target (void);

#endif /* amd64-nat.h */
