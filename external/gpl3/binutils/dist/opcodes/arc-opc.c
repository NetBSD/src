/* Opcode table for the ARC.
   Copyright (C) 1994-2015 Free Software Foundation, Inc.

   Contributed by Claudiu Zissulescu (claziss@synopsys.com)

   This file is part of libopcodes.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "sysdep.h"
#include <stdio.h>
#include "bfd.h"
#include "opcode/arc.h"
#include "opintl.h"
#include "libiberty.h"

/* Insert RB register into a 32-bit opcode.  */
static unsigned
insert_rb (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0x07) << 24) | (((value >> 3) & 0x07) << 12);
}

static int
extract_rb (unsigned insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = (((insn >> 12) & 0x07) << 3) | ((insn >> 24) & 0x07);

  if (value == 0x3e && invalid)
    *invalid = TRUE; /* A limm operand, it should be extracted in a
			different way.  */

  return value;
}

static unsigned
insert_rad (unsigned insn,
	    int value,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value & 0x01)
    *errmsg = _("Improper register value.");

  return insn | (value & 0x3F);
}

static unsigned
insert_rcd (unsigned insn,
	    int value,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value & 0x01)
    *errmsg = _("Improper register value.");

  return insn | ((value & 0x3F) << 6);
}

/* Dummy insert ZERO operand function.  */

static unsigned
insert_za (unsigned insn,
	   int value,
	   const char **errmsg)
{
  if (value)
    *errmsg = _("operand is not zero");
  return insn;
}

/* Insert Y-bit in bbit/br instructions.  This function is called only
   when solving fixups.  */

static unsigned
insert_Ybit (unsigned insn,
	     int value,
	     const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value > 0)
    insn |= 0x08;

  return insn;
}

/* Insert Y-bit in bbit/br instructions.  This function is called only
   when solving fixups.  */

static unsigned
insert_NYbit (unsigned insn,
	      int value,
	      const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value < 0)
    insn |= 0x08;

  return insn;
}

/* Insert H register into a 16-bit opcode.  */

static unsigned
insert_rhv1 (unsigned insn,
	     int value,
	     const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn |= ((value & 0x07) << 5) | ((value >> 3) & 0x07);
}

static int
extract_rhv1 (unsigned insn ATTRIBUTE_UNUSED,
	      bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  return value;
}

/* Insert H register into a 16-bit opcode.  */

static unsigned
insert_rhv2 (unsigned insn,
	     int value,
	     const char **errmsg)
{
  if (value == 0x1E)
    *errmsg =
      _("Register R30 is a limm indicator for this type of instruction.");
  return insn |= ((value & 0x07) << 5) | ((value >> 3) & 0x03);
}

static int
extract_rhv2 (unsigned insn ATTRIBUTE_UNUSED,
	      bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = ((insn >> 5) & 0x07) | ((insn & 0x03) << 3);

  return value;
}

static unsigned
insert_r0 (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 0)
    *errmsg = _("Register must be R0.");
  return insn;
}

static int
extract_r0 (unsigned insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 0;
}


static unsigned
insert_r1 (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 1)
    *errmsg = _("Register must be R1.");
  return insn;
}

static int
extract_r1 (unsigned insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 1;
}

static unsigned
insert_r2 (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 2)
    *errmsg = _("Register must be R2.");
  return insn;
}

static int
extract_r2 (unsigned insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 2;
}

static unsigned
insert_r3 (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 3)
    *errmsg = _("Register must be R3.");
  return insn;
}

static int
extract_r3 (unsigned insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 3;
}

static unsigned
insert_sp (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 28)
    *errmsg = _("Register must be SP.");
  return insn;
}

static int
extract_sp (unsigned insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 28;
}

static unsigned
insert_gp (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 26)
    *errmsg = _("Register must be GP.");
  return insn;
}

static int
extract_gp (unsigned insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 26;
}

static unsigned
insert_pcl (unsigned insn,
	    int value,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 63)
    *errmsg = _("Register must be PCL.");
  return insn;
}

static int
extract_pcl (unsigned insn ATTRIBUTE_UNUSED,
	     bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 63;
}

static unsigned
insert_blink (unsigned insn,
	      int value,
	      const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 31)
    *errmsg = _("Register must be BLINK.");
  return insn;
}

static int
extract_blink (unsigned insn ATTRIBUTE_UNUSED,
	       bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 31;
}

static unsigned
insert_ilink1 (unsigned insn,
	       int value,
	       const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 29)
    *errmsg = _("Register must be ILINK1.");
  return insn;
}

static int
extract_ilink1 (unsigned insn ATTRIBUTE_UNUSED,
		bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 29;
}

static unsigned
insert_ilink2 (unsigned insn,
	       int value,
	       const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 30)
    *errmsg = _("Register must be ILINK2.");
  return insn;
}

static int
extract_ilink2 (unsigned insn ATTRIBUTE_UNUSED,
		bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 30;
}

static unsigned
insert_ras (unsigned insn,
	    int value,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  switch (value)
    {
    case 0:
    case 1:
    case 2:
    case 3:
      insn |= value;
      break;
    case 12:
    case 13:
    case 14:
    case 15:
      insn |= (value - 8);
      break;
    default:
      *errmsg = _("Register must be either r0-r3 or r12-r15.");
      break;
    }
  return insn;
}

static int
extract_ras (unsigned insn ATTRIBUTE_UNUSED,
	     bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = insn & 0x07;
  if (value > 3)
    return (value + 8);
  else
    return value;
}

static unsigned
insert_rbs (unsigned insn,
	    int value,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  switch (value)
    {
    case 0:
    case 1:
    case 2:
    case 3:
      insn |= value << 8;
      break;
    case 12:
    case 13:
    case 14:
    case 15:
      insn |= ((value - 8)) << 8;
      break;
    default:
      *errmsg = _("Register must be either r0-r3 or r12-r15.");
      break;
    }
  return insn;
}

static int
extract_rbs (unsigned insn ATTRIBUTE_UNUSED,
	     bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = (insn >> 8) & 0x07;
  if (value > 3)
    return (value + 8);
  else
    return value;
}

static unsigned
insert_rcs (unsigned insn,
	    int value,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  switch (value)
    {
    case 0:
    case 1:
    case 2:
    case 3:
      insn |= value << 5;
      break;
    case 12:
    case 13:
    case 14:
    case 15:
      insn |= ((value - 8)) << 5;
      break;
    default:
      *errmsg = _("Register must be either r0-r3 or r12-r15.");
      break;
    }
  return insn;
}

static int
extract_rcs (unsigned insn ATTRIBUTE_UNUSED,
	     bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = (insn >> 5) & 0x07;
  if (value > 3)
    return (value + 8);
  else
    return value;
}

static unsigned
insert_simm3s (unsigned insn,
	       int value,
	       const char **errmsg ATTRIBUTE_UNUSED)
{
  int tmp = 0;
  switch (value)
    {
    case -1:
      tmp = 0x07;
      break;
    case 0:
      tmp = 0x00;
      break;
    case 1:
      tmp = 0x01;
      break;
    case 2:
      tmp = 0x02;
      break;
    case 3:
      tmp = 0x03;
      break;
    case 4:
      tmp = 0x04;
      break;
    case 5:
      tmp = 0x05;
      break;
    case 6:
      tmp = 0x06;
      break;
    default:
      *errmsg = _("Accepted values are from -1 to 6.");
      break;
    }

  insn |= tmp << 8;
  return insn;
}

static int
extract_simm3s (unsigned insn ATTRIBUTE_UNUSED,
		bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = (insn >> 8) & 0x07;
  if (value == 7)
    return -1;
  else
    return value;
}

static unsigned
insert_rrange (unsigned insn,
	       int value,
	       const char **errmsg ATTRIBUTE_UNUSED)
{
  int reg1 = (value >> 16) & 0xFFFF;
  int reg2 = value & 0xFFFF;
  if (reg1 != 13)
    {
      *errmsg = _("First register of the range should be r13.");
      return insn;
    }
  if (reg2 < 13 || reg2 > 26)
    {
      *errmsg = _("Last register of the range doesn't fit.");
      return insn;
    }
  insn |= ((reg2 - 12) & 0x0F) << 1;
  return insn;
}

static int
extract_rrange (unsigned insn  ATTRIBUTE_UNUSED,
		bfd_boolean * invalid  ATTRIBUTE_UNUSED)
{
  return (insn >> 1) & 0x0F;
}

static unsigned
insert_fpel (unsigned insn,
	     int value,
	     const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 27)
    {
      *errmsg = _("Invalid register number, should be fp.");
      return insn;
    }

  insn |= 0x0100;
  return insn;
}

static int
extract_fpel (unsigned insn  ATTRIBUTE_UNUSED,
	      bfd_boolean * invalid  ATTRIBUTE_UNUSED)
{
  return (insn & 0x0100) ? 27 : -1;
}

static unsigned
insert_blinkel (unsigned insn,
		int value,
		const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 31)
    {
      *errmsg = _("Invalid register number, should be blink.");
      return insn;
    }

  insn |= 0x0200;
  return insn;
}

static int
extract_blinkel (unsigned insn  ATTRIBUTE_UNUSED,
		 bfd_boolean * invalid  ATTRIBUTE_UNUSED)
{
  return (insn & 0x0200) ? 31 : -1;
}

static unsigned
insert_pclel (unsigned insn,
	      int value,
	      const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 63)
    {
      *errmsg = _("Invalid register number, should be pcl.");
      return insn;
    }

  insn |= 0x0400;
  return insn;
}

static int
extract_pclel (unsigned insn  ATTRIBUTE_UNUSED,
	       bfd_boolean * invalid  ATTRIBUTE_UNUSED)
{
  return (insn & 0x0400) ? 63 : -1;
}

#define INSERT_W6
/* mask = 00000000000000000000111111000000
   insn = 00011bbb000000000BBBwwwwwwDaaZZ1.  */
static unsigned
insert_w6 (unsigned insn ATTRIBUTE_UNUSED,
	   int value ATTRIBUTE_UNUSED,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value >> 0) & 0x003f) << 6;

  return insn;
}

#define EXTRACT_W6
/* mask = 00000000000000000000111111000000.  */
static int
extract_w6 (unsigned insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;

  return value;
}

#define INSERT_G_S
/* mask = 0000011100022000
   insn = 01000ggghhhGG0HH.  */
static unsigned
insert_g_s (unsigned insn ATTRIBUTE_UNUSED,
	    int value ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value >> 0) & 0x0007) << 8;
  insn |= ((value >> 3) & 0x0003) << 3;

  return insn;
}

#define EXTRACT_G_S
/* mask = 0000011100022000.  */
static int
extract_g_s (unsigned insn ATTRIBUTE_UNUSED,
	     bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 8) & 0x0007) << 0;
  value |= ((insn >> 3) & 0x0003) << 3;

  /* Extend the sign.  */
  int signbit = 1 << (6 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* Include the generic extract/insert functions.  Order is important
   as some of the functions present in the .h may be disabled via
   defines.  */
#include "arc-fxi.h"

/* Abbreviations for instruction subsets.  */
#define BASE			ARC_OPCODE_BASE

/* The flag operands table.

   The format of the table is
   NAME CODE BITS SHIFT FAVAIL.  */
const struct arc_flag_operand arc_flag_operands[] =
{
#define F_NULL	0
  { 0, 0, 0, 0, 0},
#define F_ALWAYS    (F_NULL + 1)
  { "al", 0, 0, 0, 0 },
#define F_RA	    (F_ALWAYS + 1)
  { "ra", 0, 0, 0, 0 },
#define F_EQUAL	    (F_RA + 1)
  { "eq", 1, 5, 0, 1 },
#define F_ZERO	    (F_EQUAL + 1)
  { "z",  1, 5, 0, 0 },
#define F_NOTEQUAL  (F_ZERO + 1)
  { "ne", 2, 5, 0, 1 },
#define F_NOTZERO   (F_NOTEQUAL + 1)
  { "nz", 2, 5, 0, 0 },
#define F_POZITIVE  (F_NOTZERO + 1)
  { "p",  3, 5, 0, 1 },
#define F_PL	    (F_POZITIVE + 1)
  { "pl", 3, 5, 0, 0 },
#define F_NEGATIVE  (F_PL + 1)
  { "n",  4, 5, 0, 1 },
#define F_MINUS	    (F_NEGATIVE + 1)
  { "mi", 4, 5, 0, 0 },
#define F_CARRY	    (F_MINUS + 1)
  { "c",  5, 5, 0, 1 },
#define F_CARRYSET  (F_CARRY + 1)
  { "cs", 5, 5, 0, 0 },
#define F_LOWER	    (F_CARRYSET + 1)
  { "lo", 5, 5, 0, 0 },
#define F_CARRYCLR  (F_LOWER + 1)
  { "cc", 6, 5, 0, 0 },
#define F_NOTCARRY (F_CARRYCLR + 1)
  { "nc", 6, 5, 0, 1 },
#define F_HIGHER   (F_NOTCARRY + 1)
  { "hs", 6, 5, 0, 0 },
#define F_OVERFLOWSET (F_HIGHER + 1)
  { "vs", 7, 5, 0, 0 },
#define F_OVERFLOW (F_OVERFLOWSET + 1)
  { "v",  7, 5, 0, 1 },
#define F_NOTOVERFLOW (F_OVERFLOW + 1)
  { "nv", 8, 5, 0, 1 },
#define F_OVERFLOWCLR (F_NOTOVERFLOW + 1)
  { "vc", 8, 5, 0, 0 },
#define F_GT	   (F_OVERFLOWCLR + 1)
  { "gt", 9, 5, 0, 1 },
#define F_GE	   (F_GT + 1)
  { "ge", 10, 5, 0, 1 },
#define F_LT	   (F_GE + 1)
  { "lt", 11, 5, 0, 1 },
#define F_LE	   (F_LT + 1)
  { "le", 12, 5, 0, 1 },
#define F_HI	   (F_LE + 1)
  { "hi", 13, 5, 0, 1 },
#define F_LS	   (F_HI + 1)
  { "ls", 14, 5, 0, 1 },
#define F_PNZ	   (F_LS + 1)
  { "pnz", 15, 5, 0, 1 },

  /* FLAG.  */
#define F_FLAG     (F_PNZ + 1)
  { "f",  1, 1, 15, 1 },
#define F_FFAKE     (F_FLAG + 1)
  { "f",  0, 0, 0, 1 },

  /* Delay slot.  */
#define F_ND	   (F_FFAKE + 1)
  { "nd", 0, 1, 5, 0 },
#define F_D	   (F_ND + 1)
  { "d",  1, 1, 5, 1 },
#define F_DFAKE	   (F_D + 1)
  { "d",  0, 0, 0, 1 },

  /* Data size.  */
#define F_SIZEB1   (F_DFAKE + 1)
  { "b", 1, 2, 1, 1 },
#define F_SIZEB7   (F_SIZEB1 + 1)
  { "b", 1, 2, 7, 1 },
#define F_SIZEB17  (F_SIZEB7 + 1)
  { "b", 1, 2, 17, 1 },
#define F_SIZEW1   (F_SIZEB17 + 1)
  { "w", 2, 2, 1, 0 },
#define F_SIZEW7   (F_SIZEW1 + 1)
  { "w", 2, 2, 7, 0 },
#define F_SIZEW17  (F_SIZEW7 + 1)
  { "w", 2, 2, 17, 0 },

  /* Sign extension.  */
#define F_SIGN6   (F_SIZEW17 + 1)
  { "x", 1, 1, 6, 1 },
#define F_SIGN16  (F_SIGN6 + 1)
  { "x", 1, 1, 16, 1 },
#define F_SIGNX   (F_SIGN16 + 1)
  { "x", 0, 0, 0, 1 },

  /* Address write-back modes.  */
#define F_A3       (F_SIGNX + 1)
  { "a", 1, 2, 3, 0 },
#define F_A9       (F_A3 + 1)
  { "a", 1, 2, 9, 0 },
#define F_A22      (F_A9 + 1)
  { "a", 1, 2, 22, 0 },
#define F_AW3      (F_A22 + 1)
  { "aw", 1, 2, 3, 1 },
#define F_AW9      (F_AW3 + 1)
  { "aw", 1, 2, 9, 1 },
#define F_AW22     (F_AW9 + 1)
  { "aw", 1, 2, 22, 1 },
#define F_AB3      (F_AW22 + 1)
  { "ab", 2, 2, 3, 1 },
#define F_AB9      (F_AB3 + 1)
  { "ab", 2, 2, 9, 1 },
#define F_AB22     (F_AB9 + 1)
  { "ab", 2, 2, 22, 1 },
#define F_AS3      (F_AB22 + 1)
  { "as", 3, 2, 3, 1 },
#define F_AS9      (F_AS3 + 1)
  { "as", 3, 2, 9, 1 },
#define F_AS22     (F_AS9 + 1)
  { "as", 3, 2, 22, 1 },
#define F_ASFAKE   (F_AS22 + 1)
  { "as", 0, 0, 0, 1 },

  /* Cache bypass.  */
#define F_DI5     (F_ASFAKE + 1)
  { "di", 1, 1, 5, 1 },
#define F_DI11    (F_DI5 + 1)
  { "di", 1, 1, 11, 1 },
#define F_DI15    (F_DI11 + 1)
  { "di", 1, 1, 15, 1 },

  /* ARCv2 specific.  */
#define F_NT     (F_DI15 + 1)
  { "nt", 0, 1, 3, 1},
#define F_T      (F_NT + 1)
  { "t", 1, 1, 3, 1},
#define F_H1     (F_T + 1)
  { "h", 2, 2, 1, 1 },
#define F_H7     (F_H1 + 1)
  { "h", 2, 2, 7, 1 },
#define F_H17    (F_H7 + 1)
  { "h", 2, 2, 17, 1 },

  /* Fake Flags.  */
#define F_NE   (F_H17 + 1)
  { "ne", 0, 0, 0, 1 },
};

const unsigned arc_num_flag_operands = ARRAY_SIZE (arc_flag_operands);

/* Table of the flag classes.

   The format of the table is
   CLASS {FLAG_CODE}.  */
const struct arc_flag_class arc_flag_classes[] =
{
#define C_EMPTY     0
  { FNONE, { F_NULL } },

#define C_CC	    (C_EMPTY + 1)
  { CND, { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO,
	   F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET,
	   F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET,
	   F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT,
	   F_LE, F_HI, F_LS, F_PNZ, F_NULL } },

#define C_AA_ADDR3  (C_CC + 1)
#define C_AA27	    (C_CC + 1)
  { WBM, { F_A3, F_AW3, F_AB3, F_AS3, F_NULL } },
#define C_AA_ADDR9  (C_AA_ADDR3 + 1)
#define C_AA21	     (C_AA_ADDR3 + 1)
  { WBM, { F_A9, F_AW9, F_AB9, F_AS9, F_NULL } },
#define C_AA_ADDR22 (C_AA_ADDR9 + 1)
#define C_AA8	   (C_AA_ADDR9 + 1)
  { WBM, { F_A22, F_AW22, F_AB22, F_AS22, F_NULL } },

#define C_F	    (C_AA_ADDR22 + 1)
  { FLG, { F_FLAG, F_NULL } },
#define C_FHARD	    (C_F + 1)
  { FLG, { F_FFAKE, F_NULL } },

#define C_T	    (C_FHARD + 1)
  { SBP, { F_NT, F_T, F_NULL } },
#define C_D	    (C_T + 1)
  { DLY, { F_ND, F_D, F_NULL } },

#define C_DHARD	    (C_D + 1)
  { DLY, { F_DFAKE, F_NULL } },

#define C_DI20	    (C_DHARD + 1)
  { DIF, { F_DI11, F_NULL }},
#define C_DI16	    (C_DI20 + 1)
  { DIF, { F_DI15, F_NULL }},
#define C_DI26	    (C_DI16 + 1)
  { DIF, { F_DI5, F_NULL }},

#define C_X25	    (C_DI26 + 1)
  { SGX, { F_SIGN6, F_NULL }},
#define C_X15	   (C_X25 + 1)
  { SGX, { F_SIGN16, F_NULL }},
#define C_XHARD	   (C_X15 + 1)
#define C_X	   (C_X15 + 1)
  { SGX, { F_SIGNX, F_NULL }},

#define C_ZZ13	      (C_X + 1)
  { SZM, { F_SIZEB17, F_SIZEW17, F_H17, F_NULL}},
#define C_ZZ23	      (C_ZZ13 + 1)
  { SZM, { F_SIZEB7, F_SIZEW7, F_H7, F_NULL}},
#define C_ZZ29	      (C_ZZ23 + 1)
  { SZM, { F_SIZEB1, F_SIZEW1, F_H1, F_NULL}},

#define C_AS	    (C_ZZ29 + 1)
  { SZM, { F_ASFAKE, F_NULL}},

#define C_NE	    (C_AS + 1)
  { CND, { F_NE, F_NULL}},
};

/* The operands table.

   The format of the operands table is:

   BITS SHIFT DEFAULT_RELOC FLAGS INSERT_FUN EXTRACT_FUN.  */
const struct arc_operand arc_operands[] =
{
  /* The fields are bits, shift, insert, extract, flags.  The zero
     index is used to indicate end-of-list.  */
#define UNUSED		0
  { 0, 0, 0, 0, 0, 0 },
  /* The plain integer register fields.  Used by 32 bit
     instructions.  */
#define RA		(UNUSED + 1)
  { 6, 0, 0, ARC_OPERAND_IR, 0, 0 },
#define RB		(RA + 1)
  { 6, 12, 0, ARC_OPERAND_IR, insert_rb, extract_rb },
#define RC		(RB + 1)
  { 6, 6, 0, ARC_OPERAND_IR, 0, 0 },
#define RBdup		(RC + 1)
  { 6, 12, 0, ARC_OPERAND_IR | ARC_OPERAND_DUPLICATE, insert_rb, extract_rb },

#define RAD		(RBdup + 1)
  { 6, 0, 0, ARC_OPERAND_IR | ARC_OPERAND_TRUNCATE, insert_rad, 0 },
#define RCD		(RAD + 1)
  { 6, 6, 0, ARC_OPERAND_IR | ARC_OPERAND_TRUNCATE, insert_rcd, 0 },

  /* The plain integer register fields.  Used by short
     instructions.  */
#define RA16		(RCD + 1)
#define RA_S		(RCD + 1)
  { 4, 0, 0, ARC_OPERAND_IR, insert_ras, extract_ras },
#define RB16		(RA16 + 1)
#define RB_S		(RA16 + 1)
  { 4, 8, 0, ARC_OPERAND_IR, insert_rbs, extract_rbs },
#define RB16dup		(RB16 + 1)
#define RB_Sdup		(RB16 + 1)
  { 4, 8, 0, ARC_OPERAND_IR | ARC_OPERAND_DUPLICATE, insert_rbs, extract_rbs },
#define RC16		(RB16dup + 1)
#define RC_S		(RB16dup + 1)
  { 4, 5, 0, ARC_OPERAND_IR, insert_rcs, extract_rcs },
#define R6H		(RC16 + 1)   /* 6bit register field 'h' used
					by V1 cpus.  */
  { 6, 5, 0, ARC_OPERAND_IR, insert_rhv1, extract_rhv1 },
#define R5H		(R6H + 1)    /* 5bit register field 'h' used
					by V2 cpus.  */
#define RH_S		(R6H + 1)    /* 5bit register field 'h' used
					by V2 cpus.  */
  { 5, 5, 0, ARC_OPERAND_IR, insert_rhv2, extract_rhv2 },
#define R5Hdup		(R5H + 1)
#define RH_Sdup		(R5H + 1)
  { 5, 5, 0, ARC_OPERAND_IR | ARC_OPERAND_DUPLICATE,
    insert_rhv2, extract_rhv2 },

#define RG		(R5Hdup + 1)
#define G_S		(R5Hdup + 1)
  { 5, 5, 0, ARC_OPERAND_IR, insert_g_s, extract_g_s },

  /* Fix registers.  */
#define R0		(RG + 1)
#define R0_S		(RG + 1)
  { 0, 0, 0, ARC_OPERAND_IR, insert_r0, extract_r0 },
#define R1		(R0 + 1)
#define R1_S		(R0 + 1)
  { 1, 0, 0, ARC_OPERAND_IR, insert_r1, extract_r1 },
#define R2		(R1 + 1)
#define R2_S		(R1 + 1)
  { 2, 0, 0, ARC_OPERAND_IR, insert_r2, extract_r2 },
#define R3		(R2 + 1)
#define R3_S		(R2 + 1)
  { 2, 0, 0, ARC_OPERAND_IR, insert_r3, extract_r3 },
#define SP		(R3 + 1)
#define SP_S		(R3 + 1)
  { 5, 0, 0, ARC_OPERAND_IR, insert_sp, extract_sp },
#define SPdup		(SP + 1)
#define SP_Sdup		(SP + 1)
  { 5, 0, 0, ARC_OPERAND_IR | ARC_OPERAND_DUPLICATE, insert_sp, extract_sp },
#define GP		(SPdup + 1)
#define GP_S		(SPdup + 1)
  { 5, 0, 0, ARC_OPERAND_IR, insert_gp, extract_gp },

#define PCL_S		(GP + 1)
  { 1, 0, 0, ARC_OPERAND_IR | ARC_OPERAND_NCHK, insert_pcl, extract_pcl },

#define BLINK		(PCL_S + 1)
#define BLINK_S		(PCL_S + 1)
  { 5, 0, 0, ARC_OPERAND_IR, insert_blink, extract_blink },

#define ILINK1		(BLINK + 1)
  { 5, 0, 0, ARC_OPERAND_IR, insert_ilink1, extract_ilink1 },
#define ILINK2		(ILINK1 + 1)
  { 5, 0, 0, ARC_OPERAND_IR, insert_ilink2, extract_ilink2 },

  /* Long immediate.  */
#define LIMM		(ILINK2 + 1)
#define LIMM_S		(ILINK2 + 1)
  { 32, 0, BFD_RELOC_ARC_32_ME, ARC_OPERAND_LIMM, insert_limm, 0 },
#define LIMMdup		(LIMM + 1)
  { 32, 0, 0, ARC_OPERAND_LIMM | ARC_OPERAND_DUPLICATE, insert_limm, 0 },

  /* Special operands.  */
#define ZA		(LIMMdup + 1)
#define ZB		(LIMMdup + 1)
#define ZA_S		(LIMMdup + 1)
#define ZB_S		(LIMMdup + 1)
#define ZC_S		(LIMMdup + 1)
  { 0, 0, 0, ARC_OPERAND_UNSIGNED, insert_za, 0 },

#define RRANGE_EL	(ZA + 1)
  { 4, 0, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_NCHK | ARC_OPERAND_TRUNCATE,
    insert_rrange, extract_rrange},
#define FP_EL		(RRANGE_EL + 1)
  { 1, 0, 0, ARC_OPERAND_IR | ARC_OPERAND_IGNORE | ARC_OPERAND_NCHK,
    insert_fpel, extract_fpel },
#define BLINK_EL	(FP_EL + 1)
  { 1, 0, 0, ARC_OPERAND_IR | ARC_OPERAND_IGNORE | ARC_OPERAND_NCHK,
    insert_blinkel, extract_blinkel },
#define PCL_EL		(BLINK_EL + 1)
  { 1, 0, 0, ARC_OPERAND_IR | ARC_OPERAND_IGNORE | ARC_OPERAND_NCHK,
    insert_pclel, extract_pclel },

  /* Fake operand to handle the T flag.  */
#define BRAKET		(PCL_EL + 1)
#define BRAKETdup	(PCL_EL + 1)
  { 0, 0, 0, ARC_OPERAND_FAKE | ARC_OPERAND_BRAKET, 0, 0 },

  /* Fake operand to handle the T flag.  */
#define FKT_T		(BRAKET + 1)
  { 1, 3, 0, ARC_OPERAND_FAKE, insert_Ybit, 0 },
  /* Fake operand to handle the T flag.  */
#define FKT_NT		(FKT_T + 1)
  { 1, 3, 0, ARC_OPERAND_FAKE, insert_NYbit, 0 },

  /* UIMM6_20 mask = 00000000000000000000111111000000.  */
#define UIMM6_20       (FKT_NT + 1)
  {6, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm6_20, extract_uimm6_20},

  /* SIMM12_20 mask = 00000000000000000000111111222222.  */
#define SIMM12_20	(UIMM6_20 + 1)
  {12, 0, 0, ARC_OPERAND_SIGNED, insert_simm12_20, extract_simm12_20},

  /* SIMM3_5_S mask = 0000011100000000.  */
#define SIMM3_5_S	(SIMM12_20 + 1)
  {3, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_NCHK,
   insert_simm3s, extract_simm3s},

  /* UIMM7_A32_11_S mask = 0000000000011111.  */
#define UIMM7_A32_11_S	     (SIMM3_5_S + 1)
  {7, 0, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED32
   | ARC_OPERAND_TRUNCATE | ARC_OPERAND_IGNORE, insert_uimm7_a32_11_s,
   extract_uimm7_a32_11_s},

  /* UIMM7_9_S mask = 0000000001111111.  */
#define UIMM7_9_S	(UIMM7_A32_11_S + 1)
  {7, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm7_9_s, extract_uimm7_9_s},

  /* UIMM3_13_S mask = 0000000000000111.  */
#define UIMM3_13_S	 (UIMM7_9_S + 1)
  {3, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm3_13_s, extract_uimm3_13_s},

  /* SIMM11_A32_7_S mask = 0000000111111111.  */
#define SIMM11_A32_7_S	     (UIMM3_13_S + 1)
  {11, 0, BFD_RELOC_ARC_SDA16_LD2, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32
   | ARC_OPERAND_TRUNCATE, insert_simm11_a32_7_s, extract_simm11_a32_7_s},

  /* UIMM6_13_S mask = 0000000002220111.  */
#define UIMM6_13_S	 (SIMM11_A32_7_S + 1)
  {6, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm6_13_s, extract_uimm6_13_s},
  /* UIMM5_11_S mask = 0000000000011111.  */
#define UIMM5_11_S	 (UIMM6_13_S + 1)
  {5, 0, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_IGNORE, insert_uimm5_11_s,
   extract_uimm5_11_s},

  /* SIMM9_A16_8 mask = 00000000111111102000000000000000.  */
#define SIMM9_A16_8	  (UIMM5_11_S + 1)
  {9, 0, -SIMM9_A16_8, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16
   | ARC_OPERAND_PCREL | ARC_OPERAND_TRUNCATE, insert_simm9_a16_8,
   extract_simm9_a16_8},

  /* UIMM6_8 mask = 00000000000000000000111111000000.	 */
#define UIMM6_8	      (SIMM9_A16_8 + 1)
  {6, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm6_8, extract_uimm6_8},

  /* SIMM21_A16_5 mask = 00000111111111102222222222000000.  */
#define SIMM21_A16_5	   (UIMM6_8 + 1)
  {21, 0, BFD_RELOC_ARC_S21H_PCREL, ARC_OPERAND_SIGNED
   | ARC_OPERAND_ALIGNED16 | ARC_OPERAND_TRUNCATE,
   insert_simm21_a16_5, extract_simm21_a16_5},

  /* SIMM25_A16_5 mask = 00000111111111102222222222003333.  */
#define SIMM25_A16_5	   (SIMM21_A16_5 + 1)
  {25, 0, BFD_RELOC_ARC_S25H_PCREL, ARC_OPERAND_SIGNED
   | ARC_OPERAND_ALIGNED16 | ARC_OPERAND_TRUNCATE | ARC_OPERAND_PCREL,
   insert_simm25_a16_5, extract_simm25_a16_5},

  /* SIMM10_A16_7_S mask = 0000000111111111.  */
#define SIMM10_A16_7_S	     (SIMM25_A16_5 + 1)
  {10, 0, -SIMM10_A16_7_S, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16
   | ARC_OPERAND_TRUNCATE | ARC_OPERAND_PCREL, insert_simm10_a16_7_s,
   extract_simm10_a16_7_s},

#define SIMM10_A16_7_Sbis    (SIMM10_A16_7_S + 1)
  {10, 0, -SIMM10_A16_7_Sbis, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16
   | ARC_OPERAND_TRUNCATE, insert_simm10_a16_7_s, extract_simm10_a16_7_s},

  /* SIMM7_A16_10_S mask = 0000000000111111.  */
#define SIMM7_A16_10_S	     (SIMM10_A16_7_Sbis + 1)
  {7, 0, -SIMM7_A16_10_S, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16
   | ARC_OPERAND_TRUNCATE | ARC_OPERAND_PCREL, insert_simm7_a16_10_s,
   extract_simm7_a16_10_s},

  /* SIMM21_A32_5 mask = 00000111111111002222222222000000.  */
#define SIMM21_A32_5	   (SIMM7_A16_10_S + 1)
  {21, 0, BFD_RELOC_ARC_S21W_PCREL, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32
   | ARC_OPERAND_TRUNCATE | ARC_OPERAND_PCREL, insert_simm21_a32_5,
   extract_simm21_a32_5},

  /* SIMM25_A32_5 mask = 00000111111111002222222222003333.  */
#define SIMM25_A32_5	   (SIMM21_A32_5 + 1)
  {25, 0, BFD_RELOC_ARC_S25W_PCREL, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32
   | ARC_OPERAND_TRUNCATE | ARC_OPERAND_PCREL, insert_simm25_a32_5,
   extract_simm25_a32_5},

  /* SIMM13_A32_5_S mask = 0000011111111111.  */
#define SIMM13_A32_5_S	     (SIMM25_A32_5 + 1)
  {13, 0, BFD_RELOC_ARC_S13_PCREL, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32
   | ARC_OPERAND_TRUNCATE | ARC_OPERAND_PCREL, insert_simm13_a32_5_s,
   extract_simm13_a32_5_s},

  /* SIMM8_A16_9_S mask = 0000000001111111.  */
#define SIMM8_A16_9_S	    (SIMM13_A32_5_S + 1)
  {8, 0, -SIMM8_A16_9_S, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16
   | ARC_OPERAND_TRUNCATE | ARC_OPERAND_PCREL, insert_simm8_a16_9_s,
   extract_simm8_a16_9_s},

  /* UIMM3_23 mask = 00000000000000000000000111000000.  */
#define UIMM3_23       (SIMM8_A16_9_S + 1)
  {3, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm3_23, extract_uimm3_23},

  /* UIMM10_6_S mask = 0000001111111111.  */
#define UIMM10_6_S	 (UIMM3_23 + 1)
  {10, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm10_6_s, extract_uimm10_6_s},

  /* UIMM6_11_S mask = 0000002200011110.  */
#define UIMM6_11_S	 (UIMM10_6_S + 1)
  {6, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm6_11_s, extract_uimm6_11_s},

  /* SIMM9_8 mask = 00000000111111112000000000000000.	 */
#define SIMM9_8	      (UIMM6_11_S + 1)
  {9, 0, BFD_RELOC_ARC_SDA_LDST, ARC_OPERAND_SIGNED | ARC_OPERAND_IGNORE,
   insert_simm9_8, extract_simm9_8},

  /* UIMM10_A32_8_S mask = 0000000011111111.  */
#define UIMM10_A32_8_S	     (SIMM9_8 + 1)
  {10, 0, -UIMM10_A32_8_S, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED32
   | ARC_OPERAND_TRUNCATE | ARC_OPERAND_PCREL, insert_uimm10_a32_8_s,
   extract_uimm10_a32_8_s},

  /* SIMM9_7_S mask = 0000000111111111.  */
#define SIMM9_7_S	(UIMM10_A32_8_S + 1)
  {9, 0, BFD_RELOC_ARC_SDA16_LD, ARC_OPERAND_SIGNED, insert_simm9_7_s,
   extract_simm9_7_s},

  /* UIMM6_A16_11_S mask = 0000000000011111.  */
#define UIMM6_A16_11_S	     (SIMM9_7_S + 1)
  {6, 0, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED16
   | ARC_OPERAND_TRUNCATE  | ARC_OPERAND_IGNORE, insert_uimm6_a16_11_s,
   extract_uimm6_a16_11_s},

  /* UIMM5_A32_11_S mask = 0000020000011000.  */
#define UIMM5_A32_11_S	     (UIMM6_A16_11_S + 1)
  {5, 0, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED32
   | ARC_OPERAND_TRUNCATE | ARC_OPERAND_IGNORE, insert_uimm5_a32_11_s,
   extract_uimm5_a32_11_s},

  /* SIMM11_A32_13_S mask = 0000022222200111.	 */
#define SIMM11_A32_13_S	      (UIMM5_A32_11_S + 1)
  {11, 0, BFD_RELOC_ARC_SDA16_ST2, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32
   | ARC_OPERAND_TRUNCATE, insert_simm11_a32_13_s, extract_simm11_a32_13_s},

  /* UIMM7_13_S mask = 0000000022220111.  */
#define UIMM7_13_S	 (SIMM11_A32_13_S + 1)
  {7, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm7_13_s, extract_uimm7_13_s},

  /* UIMM6_A16_21 mask = 00000000000000000000011111000000.  */
#define UIMM6_A16_21	   (UIMM7_13_S + 1)
  {6, 0, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED16
   | ARC_OPERAND_TRUNCATE, insert_uimm6_a16_21, extract_uimm6_a16_21},

  /* UIMM7_11_S mask = 0000022200011110.  */
#define UIMM7_11_S	 (UIMM6_A16_21 + 1)
  {7, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm7_11_s, extract_uimm7_11_s},

  /* UIMM7_A16_20 mask = 00000000000000000000111111000000.  */
#define UIMM7_A16_20	   (UIMM7_11_S + 1)
  {7, 0, -UIMM7_A16_20, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED16
   | ARC_OPERAND_TRUNCATE | ARC_OPERAND_PCREL, insert_uimm7_a16_20,
   extract_uimm7_a16_20},

  /* SIMM13_A16_20 mask = 00000000000000000000111111222222.  */
#define SIMM13_A16_20	    (UIMM7_A16_20 + 1)
  {13, 0, -SIMM13_A16_20, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16
   | ARC_OPERAND_TRUNCATE | ARC_OPERAND_PCREL, insert_simm13_a16_20,
   extract_simm13_a16_20},

  /* UIMM8_8_S mask = 0000000011111111.  */
#define UIMM8_8_S	(SIMM13_A16_20 + 1)
  {8, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm8_8_s, extract_uimm8_8_s},

  /* W6 mask = 00000000000000000000111111000000.  */
#define W6	 (UIMM8_8_S + 1)
  {6, 0, 0, ARC_OPERAND_SIGNED, insert_w6, extract_w6},

  /* UIMM6_5_S mask = 0000011111100000.  */
#define UIMM6_5_S	(W6 + 1)
  {6, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm6_5_s, extract_uimm6_5_s},
};

const unsigned arc_num_operands = ARRAY_SIZE (arc_operands);

const unsigned arc_Toperand = FKT_T;
const unsigned arc_NToperand = FKT_NT;

/* The opcode table.

   The format of the opcode table is:

   NAME OPCODE MASK CPU CLASS SUBCLASS { OPERANDS } { FLAGS }.  */
const struct arc_opcode arc_opcodes[] =
{
#include "arc-tbl.h"
};

const unsigned arc_num_opcodes = ARRAY_SIZE (arc_opcodes);

/* List with special cases instructions and the applicable flags.  */
const struct arc_flag_special arc_flag_special_cases[] =
{
  { "b", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	   F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	   F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	   F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "bl", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	    F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	    F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	    F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "br", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	    F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	    F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	    F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "j", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	   F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	   F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	   F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "jl", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	    F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	    F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	    F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "lp", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	    F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	    F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	    F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "set", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	     F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	     F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	     F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "ld", { F_SIZEB17, F_SIZEW17, F_H17, F_NULL } },
  { "st", { F_SIZEB1, F_SIZEW1, F_H1, F_NULL } }
};

const unsigned arc_num_flag_special = ARRAY_SIZE (arc_flag_special_cases);

/* Relocations.  */
#undef DEF
#define DEF(NAME, EXC1, EXC2, RELOC1, RELOC2)	\
  { #NAME, EXC1, EXC2, RELOC1, RELOC2}

const struct arc_reloc_equiv_tab arc_reloc_equiv[] =
{
  DEF (sda, "ld", F_AS9, BFD_RELOC_ARC_SDA_LDST, BFD_RELOC_ARC_SDA_LDST2),
  DEF (sda, "st", F_AS9, BFD_RELOC_ARC_SDA_LDST, BFD_RELOC_ARC_SDA_LDST2),
  DEF (sda, "ldw", F_AS9, BFD_RELOC_ARC_SDA_LDST, BFD_RELOC_ARC_SDA_LDST1),
  DEF (sda, "ldh", F_AS9, BFD_RELOC_ARC_SDA_LDST, BFD_RELOC_ARC_SDA_LDST1),
  DEF (sda, "stw", F_AS9, BFD_RELOC_ARC_SDA_LDST, BFD_RELOC_ARC_SDA_LDST1),
  DEF (sda, "sth", F_AS9, BFD_RELOC_ARC_SDA_LDST, BFD_RELOC_ARC_SDA_LDST1),

  /* Short instructions.  */
  DEF (sda, 0, F_NULL, BFD_RELOC_ARC_SDA16_LD, BFD_RELOC_ARC_SDA16_LD),
  DEF (sda, 0, F_NULL, -SIMM10_A16_7_Sbis, BFD_RELOC_ARC_SDA16_LD1),
  DEF (sda, 0, F_NULL, BFD_RELOC_ARC_SDA16_LD2, BFD_RELOC_ARC_SDA16_LD2),
  DEF (sda, 0, F_NULL, BFD_RELOC_ARC_SDA16_ST2, BFD_RELOC_ARC_SDA16_ST2),

  DEF (sda, 0, F_NULL, BFD_RELOC_ARC_32_ME, BFD_RELOC_ARC_SDA32_ME),
  DEF (sda, 0, F_NULL, BFD_RELOC_ARC_SDA_LDST, BFD_RELOC_ARC_SDA_LDST),

  DEF (plt, 0, F_NULL, BFD_RELOC_ARC_S25H_PCREL,
       BFD_RELOC_ARC_S25H_PCREL_PLT),
  DEF (plt, 0, F_NULL, BFD_RELOC_ARC_S21H_PCREL,
       BFD_RELOC_ARC_S21H_PCREL_PLT),
  DEF (plt, 0, F_NULL, BFD_RELOC_ARC_S25W_PCREL,
       BFD_RELOC_ARC_S25W_PCREL_PLT),
  DEF (plt, 0, F_NULL, BFD_RELOC_ARC_S21W_PCREL,
       BFD_RELOC_ARC_S21W_PCREL_PLT),

  DEF (plt, 0, F_NULL, BFD_RELOC_ARC_32_ME, BFD_RELOC_ARC_PLT32),
};

const unsigned arc_num_equiv_tab = ARRAY_SIZE (arc_reloc_equiv);

const struct arc_pseudo_insn arc_pseudo_insns[] =
{
  { "push", "st", ".aw", 5, { { RC, 0, 0, 0 }, { BRAKET, 1, 0, 1 },
			      { RB, 1, 28, 2 }, { SIMM9_8, 1, -4, 3 },
			      { BRAKETdup, 1, 0, 4} } },
  { "pop", "ld", ".ab", 5, { { RA, 0, 0, 0 }, { BRAKET, 1, 0, 1 },
			     { RB, 1, 28, 2 }, { SIMM9_8, 1, 4, 3 },
			     { BRAKETdup, 1, 0, 4} } },

  { "brgt", "brlt", NULL, 3, { { RB, 0, 0, 1 }, { RC, 0, 0, 0 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brgt", "brge", NULL, 3, { { RB, 0, 0, 0 }, { UIMM6_8, 0, 1, 1 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brgt", "brlt", NULL, 3, { { RB, 0, 0, 1 }, { LIMM, 0, 0, 0 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brgt", "brlt", NULL, 3, { { LIMM, 0, 0, 1 }, { RC, 0, 0, 0 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brgt", "brge", NULL, 3, { { LIMM, 0, 0, 0 }, { UIMM6_8, 0, 1, 1 },
			       { SIMM9_A16_8, 0, 0, 2 } } },

  { "brhi", "brlo", NULL, 3, { { RB, 0, 0, 1 }, { RC, 0, 0, 0 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brhi", "brhs", NULL, 3, { { RB, 0, 0, 0 }, { UIMM6_8, 0, 1, 1 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brhi", "brlo", NULL, 3, { { RB, 0, 0, 1 }, { LIMM, 0, 0, 0 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brhi", "brlo", NULL, 3, { { LIMM, 0, 0, 1 }, { RC, 0, 0, 0 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brhi", "brhs", NULL, 3, { { LIMM, 0, 0, 0 }, { UIMM6_8, 0, 1, 1 },
			       { SIMM9_A16_8, 0, 0, 2 } } },

  { "brle", "brge", NULL, 3, { { RB, 0, 0, 1 }, { RC, 0, 0, 0 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brle", "brlt", NULL, 3, { { RB, 0, 0, 0 }, { UIMM6_8, 0, 1, 1 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brle", "brge", NULL, 3, { { RB, 0, 0, 1 }, { LIMM, 0, 0, 0 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brle", "brge", NULL, 3, { { LIMM, 0, 0, 1 }, { RC, 0, 0, 0 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brle", "brlt", NULL, 3, { { LIMM, 0, 0, 0 }, { UIMM6_8, 0, 1, 1 },
			       { SIMM9_A16_8, 0, 0, 2 } } },

  { "brls", "brhs", NULL, 3, { { RB, 0, 0, 1 }, { RC, 0, 0, 0 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brls", "brlo", NULL, 3, { { RB, 0, 0, 0 }, { UIMM6_8, 0, 1, 1 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brls", "brhs", NULL, 3, { { RB, 0, 0, 1 }, { LIMM, 0, 0, 0 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brls", "brhs", NULL, 3, { { LIMM, 0, 0, 1 }, { RC, 0, 0, 0 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
  { "brls", "brlo", NULL, 3, { { LIMM, 0, 0, 0 }, { UIMM6_8, 0, 1, 1 },
			       { SIMM9_A16_8, 0, 0, 2 } } },
};

const unsigned arc_num_pseudo_insn =
  sizeof (arc_pseudo_insns) / sizeof (*arc_pseudo_insns);

const struct arc_aux_reg arc_aux_regs[] =
{
#undef DEF
#define DEF(ADDR, NAME)				\
  { ADDR, #NAME, sizeof (#NAME)-1 },

#include "arc-regs.h"

#undef DEF
};

const unsigned arc_num_aux_regs = ARRAY_SIZE (arc_aux_regs);
