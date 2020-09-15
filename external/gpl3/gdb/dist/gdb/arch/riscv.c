/* Copyright (C) 2018-2020 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "riscv.h"
#include <stdlib.h>
#include <unordered_map>

#include "../features/riscv/32bit-cpu.c"
#include "../features/riscv/64bit-cpu.c"
#include "../features/riscv/32bit-fpu.c"
#include "../features/riscv/64bit-fpu.c"

#ifndef GDBSERVER
#define STATIC_IN_GDB static
#else
#define STATIC_IN_GDB
#endif

/* See arch/riscv.h.  */

STATIC_IN_GDB target_desc *
riscv_create_target_description (const struct riscv_gdbarch_features features)
{
  /* Now we should create a new target description.  */
  target_desc *tdesc = allocate_target_description ();

#ifndef IN_PROCESS_AGENT
  std::string arch_name = "riscv";

  if (features.xlen == 4)
    arch_name.append (":rv32i");
  else if (features.xlen == 8)
    arch_name.append (":rv64i");
  else if (features.xlen == 16)
    arch_name.append (":rv128i");

  if (features.flen == 4)
    arch_name.append ("f");
  else if (features.flen == 8)
    arch_name.append ("d");
  else if (features.flen == 16)
    arch_name.append ("q");

  set_tdesc_architecture (tdesc, arch_name.c_str ());
#endif

  long regnum = 0;

  /* For now we only support creating 32-bit or 64-bit x-registers.  */
  if (features.xlen == 4)
    regnum = create_feature_riscv_32bit_cpu (tdesc, regnum);
  else if (features.xlen == 8)
    regnum = create_feature_riscv_64bit_cpu (tdesc, regnum);

  /* For now we only support creating 32-bit or 64-bit f-registers.  */
  if (features.flen == 4)
    regnum = create_feature_riscv_32bit_fpu (tdesc, regnum);
  else if (features.flen == 8)
    regnum = create_feature_riscv_64bit_fpu (tdesc, regnum);

  return tdesc;
}

#ifndef GDBSERVER

/* Wrapper used by std::unordered_map to generate hash for feature set.  */
struct riscv_gdbarch_features_hasher
{
  std::size_t
  operator() (const riscv_gdbarch_features &features) const noexcept
  {
    return features.hash ();
  }
};

/* Cache of previously seen target descriptions, indexed by the feature set
   that created them.  */
static std::unordered_map<riscv_gdbarch_features,
			  const target_desc_up,
			  riscv_gdbarch_features_hasher> riscv_tdesc_cache;

/* See arch/riscv.h.  */

const target_desc *
riscv_lookup_target_description (const struct riscv_gdbarch_features features)
{
  /* Lookup in the cache.  If we find it then return the pointer out of
     the target_desc_up (which is a unique_ptr).  This is safe as the
     riscv_tdesc_cache will exist until GDB exits.  */
  const auto it = riscv_tdesc_cache.find (features);
  if (it != riscv_tdesc_cache.end ())
    return it->second.get ();

  target_desc *tdesc = riscv_create_target_description (features);

  /* Add to the cache.  Work around a problem with g++ 4.8 (PR96537):
     Call the target_desc_up constructor explictly instead of implicitly.  */
  riscv_tdesc_cache.emplace (features, target_desc_up (tdesc));

  return tdesc;
}

#endif /* !GDBSERVER */
