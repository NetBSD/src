/* Target dependent functions for ns532 based PC532 board.
   Copyright (C) 1992, 1993 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*
 * HISTORY
 * $Log: ns32k-tdep.c,v $
 * Revision 1.3  1995/08/29 08:03:31  phil
 * Changes from Matthias Pfaller to get gdb to work.
 *
 * Revision 1.2  1994/05/24 23:58:24  phil
 * Follow changes to sys/arch/pc532/include/reg.h.
 *
 * Revision 1.1  1994/04/28  17:11:31  phil
 * Adding ns32k support.
 *
 * Revision 2.1.1.1  93/04/16  16:35:44  pds
 * 	Added copyright notice and whist markers.
 * 	[93/04/16            pds]
 * 
 */

/* @@@ isa_NAN should be in ieee generic float routines file */

/* Check for bad floats/doubles in P

   LEN says whether this is a single-precision or double-precision float.
   
   Returns 0 when valid IEEE floating point number
   	   1 when LEN is invalid
	   2 when NAN
	   3 when denormalized
 */

#include <sys/types.h>
#include <machine/reg.h>

#define SINGLE_EXP_BITS  8
#define DOUBLE_EXP_BITS 11
int
isa_NAN(p, len)
     int *p, len;
{
  int exponent;
  if (len == 4)
    {
      exponent = *p;
      exponent = exponent << 1 >> (32 - SINGLE_EXP_BITS - 1);
      if (exponent == -1)
	return 2;		/* NAN */
      else if (!exponent && (*p & 0x7fffffff)) /* Mask sign bit off */
	return 3;		/* Denormalized */
      else
	return 0;
    }
  else if (len == 8)
    {
      exponent = *(p+1);
      exponent = exponent << 1 >> (32 - DOUBLE_EXP_BITS - 1);
      if (exponent == -1)
	return 2;		/* NAN */
      else if (!exponent && ((*p & 0x7fffffff) || *(p+1)))
	return 3;		/* Denormalized */
      else
	return 0;
    }
  else return 1;
}

/* this table must line up with REGISTER_NAMES in tm-ns32k.h */
static int regmap[] = 
{
  REG_R0, REG_R1, REG_R2, REG_R3, REG_R4, REG_R5, REG_R6, REG_R7,
  0, 0, 0, 0, 0, 0, 0, 0,
  REG_SP, REG_FP, REG_PC, REG_PSR,
  0, 0, 0, 0, 0
};

/* blockend is the value of u.u_ar0, and points to the
   place where r7 is stored.  */

int
ns32k_register_u_addr (blockend, regnum)
     int blockend;
     int regnum;
{
    return (blockend + 4 * regmap[regnum]);
}

