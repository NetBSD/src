/* Common AArch64 functionality for MTE

   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

#include "arch/aarch64-mte.h"

/* See arch/aarch64-mte.h */

size_t
aarch64_mte_get_tag_granules (CORE_ADDR addr, size_t len, size_t granule_size)
{
  /* An empty range has 0 tag granules.  */
  if (len == 0)
    return 0;

  /* Start address */
  CORE_ADDR s_addr = align_down (addr, granule_size);
  /* End address */
  CORE_ADDR e_addr = align_down (addr + len - 1, granule_size);

  /* We always have at least 1 granule because len is non-zero at this
     point.  */
  return 1 + (e_addr - s_addr) / granule_size;
}

/* See arch/aarch64-mte.h */

CORE_ADDR
aarch64_mte_make_ltag_bits (CORE_ADDR value)
{
  return value & AARCH64_MTE_LOGICAL_MAX_VALUE;
}

/* See arch/aarch64-mte.h */

CORE_ADDR
aarch64_mte_make_ltag (CORE_ADDR value)
{
  return (aarch64_mte_make_ltag_bits (value)
	  << AARCH64_MTE_LOGICAL_TAG_START_BIT);
}

/* See arch/aarch64-mte.h */

CORE_ADDR
aarch64_mte_set_ltag (CORE_ADDR address, CORE_ADDR tag)
{
  /* Remove the existing tag.  */
  address &= ~aarch64_mte_make_ltag (AARCH64_MTE_LOGICAL_MAX_VALUE);

  /* Return the new tagged address.  */
  return address | aarch64_mte_make_ltag (tag);
}

/* See arch/aarch64-mte.h */

CORE_ADDR
aarch64_mte_get_ltag (CORE_ADDR address)
{
  CORE_ADDR ltag_addr = address >> AARCH64_MTE_LOGICAL_TAG_START_BIT;
  return aarch64_mte_make_ltag_bits (ltag_addr);
}
