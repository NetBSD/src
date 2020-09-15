/* Common target-dependent functionality for RISC-V

   Copyright (C) 2018-2020 Free Software Foundation, Inc.

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

#ifndef ARCH_RISCV_H
#define ARCH_RISCV_H

#include "gdbsupport/tdesc.h"

/* The set of RISC-V architectural features that we track that impact how
   we configure the actual gdbarch instance.  We hold one of these in the
   gdbarch_tdep structure, and use it to distinguish between different
   RISC-V gdbarch instances.

   The information in here ideally comes from the target description,
   however, if the target doesn't provide a target description then we will
   create a default target description by first populating one of these
   based on what we know about the binary being executed, and using that to
   drive default target description creation.  */

struct riscv_gdbarch_features
{
  /* The size of the x-registers in bytes.  This is either 4 (RV32), 8
     (RV64), or 16 (RV128).  No other value is valid.  Initialise to the
     invalid 0 value so we can spot if one of these is used
     uninitialised.  */
  int xlen = 0;

  /* The size of the f-registers in bytes.  This is either 4 (RV32), 8
     (RV64), or 16 (RV128).  This can also hold the value 0 to indicate
     that there are no f-registers.  No other value is valid.  */
  int flen = 0;

  /* Equality operator.  */
  bool operator== (const struct riscv_gdbarch_features &rhs) const
  {
    return (xlen == rhs.xlen && flen == rhs.flen);
  }

  /* Inequality operator.  */
  bool operator!= (const struct riscv_gdbarch_features &rhs) const
  {
    return !((*this) == rhs);
  }

  /* Used by std::unordered_map to hash feature sets.  */
  std::size_t hash () const noexcept
  {
    std::size_t val = ((xlen & 0x1f) << 5 | (flen & 0x1f) << 0);
    return val;
  }
};

#ifdef GDBSERVER

/* Create and return a target description that is compatible with FEATURES.
   This is only used directly from the gdbserver where the created target
   description is modified after it is return.  */

target_desc *riscv_create_target_description
	(const struct riscv_gdbarch_features features);

#else

/* Lookup an already existing target description matching FEATURES, or
   create a new target description if this is the first time we have seen
   FEATURES.  For the same FEATURES the same target_desc is always
   returned.  This is important when trying to lookup gdbarch objects as
   GDBARCH_LIST_LOOKUP_BY_INFO performs a pointer comparison on target
   descriptions to find candidate gdbarch objects.  */

const target_desc *riscv_lookup_target_description
	(const struct riscv_gdbarch_features features);

#endif /* GDBSERVER */


#endif /* ARCH_RISCV_H */
