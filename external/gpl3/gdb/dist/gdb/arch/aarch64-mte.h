/* Common AArch64 definitions for MTE

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

#ifndef GDB_ARCH_AARCH64_MTE_H
#define GDB_ARCH_AARCH64_MTE_H


/* We have one tag per 16 bytes of memory.  */
#define AARCH64_MTE_GRANULE_SIZE 16
#define AARCH64_MTE_TAG_BIT_SIZE 4
#define AARCH64_MTE_LOGICAL_TAG_START_BIT 56
#define AARCH64_MTE_LOGICAL_MAX_VALUE 0xf

/* Return the number of tag granules in the memory range
   [ADDR, ADDR + LEN) given GRANULE_SIZE.  */
extern size_t aarch64_mte_get_tag_granules (CORE_ADDR addr, size_t len,
					    size_t granule_size);

/* Return the 4-bit tag made from VALUE.  */
extern CORE_ADDR aarch64_mte_make_ltag_bits (CORE_ADDR value);

/* Return the 4-bit tag that can be OR-ed to an address.  */
extern CORE_ADDR aarch64_mte_make_ltag (CORE_ADDR value);

/* Helper to set the logical TAG for a 64-bit ADDRESS.

   It is always possible to set the logical tag.  */
extern CORE_ADDR aarch64_mte_set_ltag (CORE_ADDR address, CORE_ADDR tag);

/* Helper to get the logical tag from a 64-bit ADDRESS.

   It is always possible to get the logical tag.  */
extern CORE_ADDR aarch64_mte_get_ltag (CORE_ADDR address);

#endif /* GDB_ARCH_AARCH64_MTE_H */
