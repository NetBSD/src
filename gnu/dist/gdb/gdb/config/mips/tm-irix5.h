/* Target machine description for SGI Iris under Irix 5, for GDB.
   Copyright 1990, 1991, 1992, 1993, 1994, 1995, 1998, 2000
   Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "mips/tm-irix3.h"

/* FIXME: cagney/2000-04-04: Testing the _MIPS_SIM_NABI32 and
   _MIPS_SIM in a tm-*.h file is simply wrong!  Those are
   host-dependant macros (provided by /usr/include) and stop any
   chance of the target being cross compiled */
#if defined (_MIPS_SIM_NABI32) && _MIPS_SIM == _MIPS_SIM_NABI32
/*
 * Irix 6 (n32 ABI) has 32-bit GP regs and 64-bit FP regs
 */

#undef  REGISTER_BYTES
#define REGISTER_BYTES (MIPS_NUMREGS * 8 + (NUM_REGS - MIPS_NUMREGS) * MIPS_REGSIZE)

#undef  REGISTER_BYTE
#define REGISTER_BYTE(N) \
     (((N) < FP0_REGNUM) ? (N) * MIPS_REGSIZE : \
      ((N) < FP0_REGNUM + 32) ?     \
      FP0_REGNUM * MIPS_REGSIZE + \
      ((N) - FP0_REGNUM) * sizeof(double) : \
      32 * sizeof(double) + ((N) - 32) * MIPS_REGSIZE)

#undef  REGISTER_VIRTUAL_TYPE
#define REGISTER_VIRTUAL_TYPE(N) \
	(((N) >= FP0_REGNUM && (N) < FP0_REGNUM+32) ? builtin_type_double \
	 : ((N) == 32 /*SR*/) ? builtin_type_uint32 \
	 : ((N) >= 70 && (N) <= 89) ? builtin_type_uint32 \
	 : builtin_type_int)

#endif /* N32 */


/* The signal handler trampoline is called _sigtramp.  */
#undef IN_SIGTRAMP
#define IN_SIGTRAMP(pc, name) ((name) && STREQ ("_sigtramp", name))

/* Irix 5 saves a full 64 bits for each register.  We skip 2 * 4 to
   get to the saved PC (the register mask and status register are both
   32 bits) and then another 4 to get to the lower 32 bits.  We skip
   the same 4 bytes, plus the 8 bytes for the PC to get to the
   registers, and add another 4 to get to the lower 32 bits.  We skip
   8 bytes per register.  */
#undef SIGFRAME_PC_OFF
#define SIGFRAME_PC_OFF		(SIGFRAME_BASE + 2 * 4 + 4)
#undef SIGFRAME_REGSAVE_OFF
#define SIGFRAME_REGSAVE_OFF	(SIGFRAME_BASE + 2 * 4 + 8 + 4)
#undef SIGFRAME_FPREGSAVE_OFF
#define SIGFRAME_FPREGSAVE_OFF	(SIGFRAME_BASE + 2 * 4 + 8 + 32 * 8 + 4)
#define SIGFRAME_REG_SIZE	8
