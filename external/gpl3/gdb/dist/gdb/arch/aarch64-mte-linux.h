/* Common Linux target-dependent definitions for AArch64 MTE

   Copyright (C) 2021-2025 Free Software Foundation, Inc.

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

#ifndef GDB_ARCH_AARCH64_MTE_LINUX_H
#define GDB_ARCH_AARCH64_MTE_LINUX_H


/* Feature check for Memory Tagging Extension.  */
#ifndef HWCAP2_MTE
#define HWCAP2_MTE  (1 << 18)
#endif

/* The MTE regset consists of a single 64-bit register.  */
#define AARCH64_LINUX_SIZEOF_MTE 8

/* Memory tagging definitions.  */
#ifndef SEGV_MTEAERR
# define SEGV_MTEAERR 8
# define SEGV_MTESERR 9
#endif

/* Memory tag types for AArch64.  */
enum class aarch64_memtag_type
{
  /* MTE logical tag contained in pointers.  */
  mte_logical = 0,
  /* MTE allocation tag stored in memory tag granules.  */
  mte_allocation
};

/* Given a TAGS vector containing 1 MTE tag per byte, pack the data as
   2 tags per byte and resize the vector.  */
extern void aarch64_mte_pack_tags (gdb::byte_vector &tags);

/* Given a TAGS vector containing 2 MTE tags per byte, unpack the data as
   1 tag per byte and resize the vector.  If SKIP_FIRST is TRUE, skip the
   first unpacked element.  Otherwise leave it in the unpacked vector.  */
extern void aarch64_mte_unpack_tags (gdb::byte_vector &tags, bool skip_first);

#endif /* GDB_ARCH_AARCH64_MTE_LINUX_H */
