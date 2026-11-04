/* Target description related code for GNU/Linux i386.

   Copyright (C) 2024 Free Software Foundation, Inc.

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

#include "arch/x86-linux-tdesc.h"
#include "arch/i386-linux-tdesc.h"
#include "arch/i386.h"
#include "arch/x86-linux-tdesc-features.h"

/* See arch/i386-linux-tdesc.h.  */

const target_desc *
i386_linux_read_description (uint64_t xcr0)
{
  /* Cache of previously seen i386 target descriptions, indexed by the xcr0
     value that created the target description.  This needs to be static
     within this function to ensure it is initialised before first use.  */
  static std::unordered_map<uint64_t, const target_desc_up> i386_tdesc_cache;

  /* Only some bits are checked when creating a tdesc, but the XCR0 value
     contains other feature bits that are not relevant for tdesc creation.
     When indexing into the I386_TDESC_CACHE we need to use a consistent
     xcr0 value otherwise we might fail to find an existing tdesc which has
     the same set of relevant bits set.  */
  xcr0 &= x86_linux_i386_xcr0_feature_mask ();

  const auto it = i386_tdesc_cache.find (xcr0);
  if (it != i386_tdesc_cache.end ())
    return it->second.get ();

  /* Create the previously unseen target description.  */
  target_desc_up tdesc (i386_create_target_description (xcr0, true, false));
  x86_linux_post_init_tdesc (tdesc.get (), false);

  /* Add to the cache, and return a pointer borrowed from the
     target_desc_up.  This is safe as the cache (and the pointers contained
     within it) are not deleted until GDB exits.  */
  target_desc *ptr = tdesc.get ();
  i386_tdesc_cache.emplace (xcr0, std::move (tdesc));
  return ptr;
}
