/* Mitsubishi Electric Corp. D30V Simulator.
   Copyright (C) 1997, Free Software Foundation, Inc.
   Contributed by Cygnus Support.

This file is part of GDB, the GNU debugger.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#ifndef _D30V_ALU_H_
#define _D30V_ALU_H_

#define ALU_CARRY (PSW_VAL(PSW_C) != 0)

#include "sim-alu.h"

#define ALU16_END(TARG, HIGH)						\
{									\
  unsigned32 mask, value;						\
  if (ALU16_HAD_OVERFLOW) {						\
    mask = BIT32 (PSW_V) | BIT32 (PSW_VA) | BIT32 (PSW_C);		\
    value = BIT32 (PSW_V) | BIT32 (PSW_VA);				\
  }									\
  else {								\
    mask = BIT32 (PSW_V) | BIT32 (PSW_C);				\
    value = 0;								\
  }									\
  if (ALU16_HAD_CARRY_BORROW)						\
    value |= BIT32 (PSW_C);						\
  if (HIGH)								\
    WRITE32_QUEUE_MASK (TARG, ALU16_OVERFLOW_RESULT<<16, 0xffff0000);	\
  else									\
    WRITE32_QUEUE_MASK (TARG, ALU16_OVERFLOW_RESULT, 0x0000ffff);	\
  WRITE32_QUEUE_MASK (&PSW, value, mask);				\
}

#define ALU32_END(TARG)							\
{									\
  unsigned32 mask, value;						\
  if (ALU32_HAD_OVERFLOW) {						\
    mask = BIT32 (PSW_V) | BIT32 (PSW_VA) | BIT32 (PSW_C);		\
    value = BIT32 (PSW_V) | BIT32 (PSW_VA);				\
  }									\
  else {								\
    mask = BIT32 (PSW_V) | BIT32 (PSW_C);				\
    value = 0;								\
  }									\
  if (ALU32_HAD_CARRY_BORROW)						\
    value |= BIT32 (PSW_C);						\
  WRITE32_QUEUE (TARG, ALU32_OVERFLOW_RESULT);				\
  WRITE32_QUEUE_MASK (&PSW, value, mask);				\
}

#define ALU_END(TARG) ALU32_END(TARG)


/* PSW & Flag manipulation */

#define PSW_SET(BIT,VAL) BLIT32(PSW, (BIT), (VAL))
#define PSW_VAL(BIT) EXTRACTED32(PSW, (BIT), (BIT))

#define PSW_F(FLAG) (17 + ((FLAG) % 8) * 2)
#define PSW_FLAG_SET(FLAG,VAL) PSW_SET(PSW_F(FLAG), VAL)
#define PSW_FLAG_VAL(FLAG) PSW_VAL(PSW_F(FLAG))

#define PSW_SET_QUEUE(BIT,VAL)						\
do {									\
  unsigned32 mask = BIT32 (BIT);					\
  unsigned32 bitval = (VAL) ? mask : 0;					\
  WRITE32_QUEUE_MASK (&PSW, bitval, mask);				\
} while (0)

#define PSW_FLAG_SET_QUEUE(FLAG,VAL)					\
do {									\
  unsigned32 mask = BIT32 (PSW_F (FLAG));				\
  unsigned32 bitval = (VAL) ? mask : 0;					\
  WRITE32_QUEUE_MASK (&PSW, bitval, mask);				\
} while (0)

/* Bring data in from the cold */

#define IMEM(EA) \
(sim_core_read_8(STATE_CPU (sd, 0), cia, exec_map, (EA)))

#define MEM(SIGN, EA, NR_BYTES) \
((SIGN##_##NR_BYTES) sim_core_read_unaligned_##NR_BYTES(STATE_CPU (sd, 0), cia, read_map, (EA)))

#define STORE(EA, NR_BYTES, VAL) \
do { \
  sim_core_write_unaligned_##NR_BYTES(STATE_CPU (sd, 0), cia, write_map, (EA), (VAL)); \
} while (0)


#endif
