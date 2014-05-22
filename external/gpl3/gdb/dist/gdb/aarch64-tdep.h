/* Common target dependent code for GDB on AArch64 systems.

   Copyright (C) 2009-2013 Free Software Foundation, Inc.
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

/* Forward declarations.  */
struct gdbarch;
struct regset;

/* AArch64 Dwarf register numbering.  */
#define AARCH64_DWARF_X0   0
#define AARCH64_DWARF_SP  31
#define AARCH64_DWARF_V0  64

/* Register numbers of various important registers.  */
enum aarch64_regnum
{
  AARCH64_X0_REGNUM,		/* First integer register */

  /* Frame register in AArch64 code, if used.  */
  AARCH64_FP_REGNUM = AARCH64_X0_REGNUM + 29,
  AARCH64_LR_REGNUM = AARCH64_X0_REGNUM + 30,	/* Return address */
  AARCH64_SP_REGNUM,		/* Stack pointer */
  AARCH64_PC_REGNUM,		/* Program counter */
  AARCH64_CPSR_REGNUM,		/* Contains status register */
  AARCH64_V0_REGNUM,		/* First floating point / vector register */

  /* Last floating point / vector register */
  AARCH64_V31_REGNUM = AARCH64_V0_REGNUM + 31,
  AARCH64_FPSR_REGNUM,		/* Floating point status register */
  AARCH64_FPCR_REGNUM,		/* Floating point control register */

  /* Other useful registers.  */

  /* Last integer-like argument */
  AARCH64_LAST_X_ARG_REGNUM = AARCH64_X0_REGNUM + 7,
  AARCH64_STRUCT_RETURN_REGNUM = AARCH64_X0_REGNUM + 8,
  AARCH64_LAST_V_ARG_REGNUM = AARCH64_V0_REGNUM + 7
};

/* Size of integer registers.  */
#define X_REGISTER_SIZE  8
#define B_REGISTER_SIZE  1
#define H_REGISTER_SIZE  2
#define S_REGISTER_SIZE  4
#define D_REGISTER_SIZE  8
#define V_REGISTER_SIZE 16
#define Q_REGISTER_SIZE 16

/* Total number of general (X) registers.  */
#define AARCH64_X_REGISTER_COUNT 32

/* Target-dependent structure in gdbarch.  */
struct gdbarch_tdep
{
  /* Lowest address at which instructions will appear.  */
  CORE_ADDR lowest_pc;

  /* Offset to PC value in jump buffer.  If this is negative, longjmp
     support will be disabled.  */
  int jb_pc;

  /* And the size of each entry in the buf.  */
  size_t jb_elt_size;

  /* Cached core file helpers.  */
  struct regset *gregset;
  struct regset *fpregset;

  /* Types for AdvSISD registers.  */
  struct type *vnq_type;
  struct type *vnd_type;
  struct type *vns_type;
  struct type *vnh_type;
  struct type *vnb_type;
};

#endif /* aarch64-tdep.h */
