/* Target description related code for GNU/Linux x86 (i386 and x86-64).

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

#include "arch/x86-linux-tdesc-features.h"

/* A structure used to describe a single xstate feature bit that might, or
   might not, be checked for when creating a target description for one of
   i386, amd64, or x32.

   The different CPU/ABI types check for different xstate features when
   creating a target description.

   We want to cache target descriptions, and this is currently done in
   three separate caches, one each for i386, amd64, and x32.  Additionally,
   the caching we're discussing here is Linux only, and for Linux, the only
   thing that has an impact on target description creation is the xcr0
   value.

   In order to ensure the cache functions correctly we need to filter out
   only those xcr0 feature bits that are relevant, we can then cache target
   descriptions based on the relevant feature bits.  Two xcr0 values might
   be different, but have the same relevant feature bits.  In this case we
   would expect the two xcr0 values to map to the same cache entry.  */

struct x86_xstate_feature {
  /* The xstate feature mask.  This is a mask against an xcr0 value.  */
  uint64_t feature;

  /* Is this feature checked when creating an i386 target description.  */
  bool is_i386;

  /* Is this feature checked when creating an amd64 target description.  */
  bool is_amd64;

  /* Is this feature checked when creating an x32 target description.  */
  bool is_x32;
};

/* A constant table that describes all of the xstate features that are
   checked when building a target description for i386, amd64, or x32.

   If in the future, due to simplifications or refactoring, this table ever
   ends up with 'true' for every xcr0 feature on every target type, then this
   is an indication that this table should probably be removed, and that the
   rest of the code in this file can be simplified.  */

static constexpr x86_xstate_feature x86_linux_all_xstate_features[] = {
  /* Feature,           i386,	amd64,	x32.  */
  { X86_XSTATE_PKRU,	true,	true, 	true },
  { X86_XSTATE_AVX512,	true,	true, 	true },
  { X86_XSTATE_AVX,	true,	true, 	true },
  { X86_XSTATE_SSE,	true,	false, 	false },
  { X86_XSTATE_X87,	true,	false, 	false }
};

/* Return a compile time constant which is a mask of all the xstate features
   that are checked for when building an i386 target description.  */

static constexpr uint64_t
x86_linux_i386_xcr0_feature_mask_1 ()
{
  uint64_t mask = 0;

  for (const auto &entry : x86_linux_all_xstate_features)
    if (entry.is_i386)
      mask |= entry.feature;

  return mask;
}

/* Return a compile time constant which is a mask of all the xstate features
   that are checked for when building an amd64 target description.  */

static constexpr uint64_t
x86_linux_amd64_xcr0_feature_mask_1 ()
{
  uint64_t mask = 0;

  for (const auto &entry : x86_linux_all_xstate_features)
    if (entry.is_amd64)
      mask |= entry.feature;

  return mask;
}

/* Return a compile time constant which is a mask of all the xstate features
   that are checked for when building an x32 target description.  */

static constexpr uint64_t
x86_linux_x32_xcr0_feature_mask_1 ()
{
  uint64_t mask = 0;

  for (const auto &entry : x86_linux_all_xstate_features)
    if (entry.is_x32)
      mask |= entry.feature;

  return mask;
}

/* See arch/x86-linux-tdesc-features.h.  */

uint64_t
x86_linux_i386_xcr0_feature_mask ()
{
  return x86_linux_i386_xcr0_feature_mask_1 ();
}

/* See arch/x86-linux-tdesc-features.h.  */

uint64_t
x86_linux_amd64_xcr0_feature_mask ()
{
  return x86_linux_amd64_xcr0_feature_mask_1 ();
}

/* See arch/x86-linux-tdesc-features.h.  */

uint64_t
x86_linux_x32_xcr0_feature_mask ()
{
  return x86_linux_x32_xcr0_feature_mask_1 ();
}

#ifdef GDBSERVER

/* See arch/x86-linux-tdesc-features.h.  */

int
x86_linux_xcr0_to_tdesc_idx (uint64_t xcr0)
{
  /* The following table shows which features are checked for when creating
     the target descriptions (see nat/x86-linux-tdesc.c), the feature order
     represents the bit order within the generated index number.

     i386  | x87 sse avx avx512 pkru
     amd64 |         avx avx512 pkru
     i32   |         avx avx512 pkru

     The features are ordered so that for each mode (i386, amd64, i32) the
     generated index will form a continuous range.  */

  int idx = 0;

  for (int i = 0; i < ARRAY_SIZE (x86_linux_all_xstate_features); ++i)
    {
      if ((xcr0 & x86_linux_all_xstate_features[i].feature)
	  == x86_linux_all_xstate_features[i].feature)
	idx |= (1 << i);
    }

  return idx;
}

#endif /* GDBSERVER */

#ifdef IN_PROCESS_AGENT

/* Return a compile time constant which is a count of the number of xstate
   features that are checked for when building an i386 target description.  */

static constexpr int
x86_linux_i386_tdesc_count_1 ()
{
  uint64_t count = 0;

  for (const auto &entry : x86_linux_all_xstate_features)
    if (entry.is_i386)
      ++count;

  gdb_assert (count > 0);

  return (1 << count);
}

/* Return a compile time constant which is a count of the number of xstate
   features that are checked for when building an amd64 target description.  */

static constexpr int
x86_linux_amd64_tdesc_count_1 ()
{
  uint64_t count = 0;

  for (const auto &entry : x86_linux_all_xstate_features)
    if (entry.is_amd64)
      ++count;

  gdb_assert (count > 0);

  return (1 << count);
}

/* Return a compile time constant which is a count of the number of xstate
   features that are checked for when building an x32 target description.  */

static constexpr int
x86_linux_x32_tdesc_count_1 ()
{
  uint64_t count = 0;

  for (const auto &entry : x86_linux_all_xstate_features)
    if (entry.is_x32)
      ++count;

  gdb_assert (count > 0);

  return (1 << count);
}

/* See arch/x86-linux-tdesc-features.h.  */

int
x86_linux_amd64_tdesc_count ()
{
  return x86_linux_amd64_tdesc_count_1 ();
}

/* See arch/x86-linux-tdesc-features.h.  */

int
x86_linux_x32_tdesc_count ()
{
  return x86_linux_x32_tdesc_count_1 ();
}

/* See arch/x86-linux-tdesc-features.h.  */

int
x86_linux_i386_tdesc_count ()
{
  return x86_linux_i386_tdesc_count_1 ();
}

/* See arch/x86-linux-tdesc-features.h.  */

uint64_t
x86_linux_tdesc_idx_to_xcr0 (int idx)
{
  uint64_t xcr0 = 0;

  for (int i = 0; i < ARRAY_SIZE (x86_linux_all_xstate_features); ++i)
    {
      if ((idx & (1 << i)) != 0)
	xcr0 |= x86_linux_all_xstate_features[i].feature;
    }

  return xcr0;
}

#endif /* IN_PROCESS_AGENT */
