/* Target-dependent code for X86-based targets.

   Copyright (C) 2018-2025 Free Software Foundation, Inc.

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

#ifndef GDB_X86_TDEP_H
#define GDB_X86_TDEP_H

/* Fill SSP to the shadow stack pointer in GDB's REGCACHE.  */

extern void x86_supply_ssp (regcache *regcache, const uint64_t ssp);

/* Collect the value of the shadow stack pointer in GDB's REGCACHE and
   write it to SSP.  */

extern void x86_collect_ssp (const regcache *regcache, uint64_t &ssp);

/* Checks whether PC lies in an indirect branch thunk using registers
   REGISTER_NAMES[LO] (inclusive) to REGISTER_NAMES[HI] (exclusive).  */

extern bool x86_in_indirect_branch_thunk (CORE_ADDR pc,
					  const char * const *register_names,
					  int lo, int hi);

#endif /* GDB_X86_TDEP_H */
