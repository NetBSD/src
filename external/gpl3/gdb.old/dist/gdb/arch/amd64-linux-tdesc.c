/* Target description related code for GNU/Linux x86-64.

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
#include "arch/amd64-linux-tdesc.h"
#include "arch/amd64.h"
#include "arch/x86-linux-tdesc-features.h"


/* See arch/amd64-linux-tdesc.h.  */

const struct target_desc *
amd64_linux_read_description (uint64_t xcr0, bool is_x32)
{
  /* The type used for the amd64 and x32 target description caches.  */
  using tdesc_cache_type = std::unordered_map<uint64_t, const target_desc_up>;

  /* Caches for the previously seen amd64 and x32 target descriptions,
     indexed by the xcr0 value that created the target description.  These
     need to be static within this function to ensure they are initialised
     before first use.  */
  static tdesc_cache_type amd64_tdesc_cache, x32_tdesc_cache;

  tdesc_cache_type &tdesc_cache = is_x32 ? x32_tdesc_cache : amd64_tdesc_cache;

  /* Only some bits are checked when creating a tdesc, but the XCR0 value
     contains other feature bits that are not relevant for tdesc creation.
     When indexing into the TDESC_CACHE we need to use a consistent xcr0
     value otherwise we might fail to find an existing tdesc which has the
     same set of relevant bits set.  */
  xcr0 &= is_x32
    ? x86_linux_x32_xcr0_feature_mask ()
    : x86_linux_amd64_xcr0_feature_mask ();

  const auto it = tdesc_cache.find (xcr0);
  if (it != tdesc_cache.end ())
    return it->second.get ();

  /* Create the previously unseen target description.  */
  target_desc_up tdesc (amd64_create_target_description (xcr0, is_x32,
							 true, true));
  x86_linux_post_init_tdesc (tdesc.get (), true);

  /* Add to the cache, and return a pointer borrowed from the
     target_desc_up.  This is safe as the cache (and the pointers contained
     within it) are not deleted until GDB exits.  */
  target_desc *ptr = tdesc.get ();
  tdesc_cache.emplace (xcr0, std::move (tdesc));
  return ptr;
}
