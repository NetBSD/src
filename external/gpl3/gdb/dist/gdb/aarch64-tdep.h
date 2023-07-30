/* Common target dependent code for GDB on AArch64 systems.

   Copyright (C) 2009-2023 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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


#ifndef AARCH64_TDEP_H
#define AARCH64_TDEP_H

#include "arch/aarch64.h"
#include "displaced-stepping.h"
#include "infrun.h"
#include "gdbarch.h"

/* Forward declarations.  */
struct gdbarch;
struct regset;

/* AArch64 Dwarf register numbering.  */
#define AARCH64_DWARF_X0   0
#define AARCH64_DWARF_SP  31
#define AARCH64_DWARF_PC  32
#define AARCH64_DWARF_RA_SIGN_STATE  34
#define AARCH64_DWARF_V0  64
#define AARCH64_DWARF_SVE_VG   46
#define AARCH64_DWARF_SVE_FFR  47
#define AARCH64_DWARF_SVE_P0   48
#define AARCH64_DWARF_SVE_Z0   96

/* Size of integer registers.  */
#define X_REGISTER_SIZE  8
#define B_REGISTER_SIZE  1
#define H_REGISTER_SIZE  2
#define S_REGISTER_SIZE  4
#define D_REGISTER_SIZE  8
#define Q_REGISTER_SIZE 16

/* Total number of general (X) registers.  */
#define AARCH64_X_REGISTER_COUNT 32
/* Total number of D registers.  */
#define AARCH64_D_REGISTER_COUNT 32

/* The maximum number of modified instructions generated for one
   single-stepped instruction.  */
#define AARCH64_DISPLACED_MODIFIED_INSNS 1

/* Target-dependent structure in gdbarch.  */
struct aarch64_gdbarch_tdep : gdbarch_tdep_base
{
  /* Lowest address at which instructions will appear.  */
  CORE_ADDR lowest_pc = 0;

  /* Offset to PC value in jump buffer.  If this is negative, longjmp
     support will be disabled.  */
  int jb_pc = 0;

  /* And the size of each entry in the buf.  */
  size_t jb_elt_size = 0;

  /* Types for AdvSISD registers.  */
  struct type *vnq_type = nullptr;
  struct type *vnd_type = nullptr;
  struct type *vns_type = nullptr;
  struct type *vnh_type = nullptr;
  struct type *vnb_type = nullptr;
  struct type *vnv_type = nullptr;

  /* syscall record.  */
  int (*aarch64_syscall_record) (struct regcache *regcache,
				 unsigned long svc_number) = nullptr;

  /* The VQ value for SVE targets, or zero if SVE is not supported.  */
  uint64_t vq = 0;

  /* Returns true if the target supports SVE.  */
  bool has_sve () const
  {
    return vq != 0;
  }

  int pauth_reg_base = 0;
  int ra_sign_state_regnum = 0;

  /* Returns true if the target supports pauth.  */
  bool has_pauth () const
  {
    return pauth_reg_base != -1;
  }

  /* First MTE register.  This is -1 if no MTE registers are available.  */
  int mte_reg_base = 0;

  /* Returns true if the target supports MTE.  */
  bool has_mte () const
  {
    return mte_reg_base != -1;
  }

  /* TLS registers.  This is -1 if the TLS registers are not available.  */
  int tls_regnum_base = 0;
  int tls_register_count = 0;

  bool has_tls() const
  {
    return tls_regnum_base != -1;
  }

  /* The W pseudo-registers.  */
  int w_pseudo_base = 0;
  int w_pseudo_count = 0;
};

const target_desc *aarch64_read_description (const aarch64_features &features);
aarch64_features
aarch64_features_from_target_desc (const struct target_desc *tdesc);

extern int aarch64_process_record (struct gdbarch *gdbarch,
			       struct regcache *regcache, CORE_ADDR addr);

displaced_step_copy_insn_closure_up
  aarch64_displaced_step_copy_insn (struct gdbarch *gdbarch,
				    CORE_ADDR from, CORE_ADDR to,
				    struct regcache *regs);

void aarch64_displaced_step_fixup (struct gdbarch *gdbarch,
				   displaced_step_copy_insn_closure *dsc,
				   CORE_ADDR from, CORE_ADDR to,
				   struct regcache *regs);

bool aarch64_displaced_step_hw_singlestep (struct gdbarch *gdbarch);

#endif /* aarch64-tdep.h */
