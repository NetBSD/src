/* SPU target-dependent code for GDB, the GNU debugger.
   Copyright (C) 2006-2017 Free Software Foundation, Inc.

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

#ifndef SPU_TDEP_H
#define SPU_TDEP_H

/* Number of registers.  */
#define SPU_NUM_REGS         130
#define SPU_NUM_PSEUDO_REGS  6
#define SPU_NUM_GPRS	     128

/* Register numbers of various important registers.  */
enum spu_regnum
{
  /* SPU calling convention.  */
  SPU_LR_REGNUM = 0,		/* Link register.  */
  SPU_RAW_SP_REGNUM = 1,	/* Stack pointer (full register).  */
  SPU_ARG1_REGNUM = 3,		/* First argument register.  */
  SPU_ARGN_REGNUM = 74,		/* Last argument register.  */
  SPU_SAVED1_REGNUM = 80,	/* First call-saved register.  */
  SPU_SAVEDN_REGNUM = 127,	/* Last call-saved register.  */
  SPU_FP_REGNUM = 127,		/* Frame pointer.  */

  /* Special registers.  */
  SPU_ID_REGNUM = 128,		/* SPU ID register.  */
  SPU_PC_REGNUM = 129,		/* Next program counter.  */
  SPU_SP_REGNUM = 130,		/* Stack pointer (preferred slot).  */
  SPU_FPSCR_REGNUM = 131,	/* Floating point status/control register.  */
  SPU_SRR0_REGNUM = 132,	/* SRR0 register.  */
  SPU_LSLR_REGNUM = 133,	/* Local store limit register.  */
  SPU_DECR_REGNUM = 134,	/* Decrementer value.  */
  SPU_DECR_STATUS_REGNUM = 135	/* Decrementer status.  */
};

/* Address conversions.

   In a combined PPU/SPU debugging session, we have to consider multiple
   address spaces: the PPU 32- or 64-bit address space, and the 32-bit
   local store address space for each SPU context.  As it is currently
   not yet possible to use the program_space / address_space mechanism
   to represent this, we encode all those addresses into one single
   64-bit address for the whole process.  For SPU programs using overlays,
   this address space must also include separate ranges reserved for the
   LMA of overlay sections.


   The following combinations are supported for combined debugging:

   PPU address (this relies on the fact that PPC 64-bit user space
   addresses can never have the highest-most bit set):

      +-+---------------------------------+
      |0|              ADDR [63]          |
      +-+---------------------------------+

   SPU address for SPU context with id SPU (this assumes that SPU
   IDs, which are file descriptors, are never larger than 2^30):

      +-+-+--------------+----------------+
      |1|0|    SPU [30]  |    ADDR [32]   |
      +-+-+--------------+----------------+

   SPU overlay section LMA for SPU context with id SPU:

      +-+-+--------------+----------------+
      |1|1|    SPU [30]  |    ADDR [32]   |
      +-+-+--------------+----------------+


   In SPU stand-alone debugging mode (using spu-linux-nat.c),
   the following combinations are supported:

   SPU address:

      +-+-+--------------+----------------+
      |0|0|     0        |    ADDR [32]   |
      +-+-+--------------+----------------+

   SPU overlay section LMA:

      +-+-+--------------+----------------+
      |0|1|     0        |    ADDR [32]   |
      +-+-+--------------+----------------+


   The following macros allow manipulation of addresses in the
   above formats.  */

#define SPUADDR(spu, addr) \
  ((spu) != -1? (ULONGEST)1 << 63 | (ULONGEST)(spu) << 32 | (addr) : (addr))

#define SPUADDR_SPU(addr) \
  (((addr) & (ULONGEST)1 << 63) \
   ? (int) ((ULONGEST)(addr) >> 32 & 0x3fffffff) \
   : -1)

#define SPUADDR_ADDR(addr) \
  (((addr) & (ULONGEST)1 << 63)? (ULONGEST)(addr) & 0xffffffff : (addr))

#define SPU_OVERLAY_LMA ((ULONGEST)1 << 62)

#endif
