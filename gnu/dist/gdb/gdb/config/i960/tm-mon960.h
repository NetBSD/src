// OBSOLETE /* Parameters for Intel 960 running MON960 monitor, for GDB, the GNU debugger.
// OBSOLETE    Copyright 1990, 1991, 1996, 1999, 2000 Free Software Foundation, Inc.
// OBSOLETE    Contributed by Intel Corporation and Cygnus Support.
// OBSOLETE 
// OBSOLETE    This file is part of GDB.
// OBSOLETE 
// OBSOLETE    This program is free software; you can redistribute it and/or modify
// OBSOLETE    it under the terms of the GNU General Public License as published by
// OBSOLETE    the Free Software Foundation; either version 2 of the License, or
// OBSOLETE    (at your option) any later version.
// OBSOLETE 
// OBSOLETE    This program is distributed in the hope that it will be useful,
// OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE    GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE    You should have received a copy of the GNU General Public License
// OBSOLETE    along with this program; if not, write to the Free Software
// OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330,
// OBSOLETE    Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE /*****************************************************************************
// OBSOLETE  * Definitions to target GDB to an i960 debugged over a serial line.
// OBSOLETE  ******************************************************************************/
// OBSOLETE 
// OBSOLETE #include "i960/tm-i960.h"
// OBSOLETE 
// OBSOLETE /* forward declarations */
// OBSOLETE struct frame_info;
// OBSOLETE 
// OBSOLETE /* redefined from tm-i960.h */
// OBSOLETE /* Number of machine registers */
// OBSOLETE #undef NUM_REGS
// OBSOLETE #define NUM_REGS 40
// OBSOLETE 
// OBSOLETE /* Initializer for an array of names of registers.
// OBSOLETE    There should be NUM_REGS strings in this initializer.  */
// OBSOLETE #undef REGISTER_NAMES
// OBSOLETE #define REGISTER_NAMES { \
// OBSOLETE 	/*  0 */ "pfp", "sp",  "rip", "r3",  "r4",  "r5",  "r6",  "r7", \
// OBSOLETE 	/*  8 */ "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",\
// OBSOLETE 	/* 16 */ "g0",  "g1",  "g2",  "g3",  "g4",  "g5",  "g6",  "g7", \
// OBSOLETE 	/* 24 */ "g8",  "g9",  "g10", "g11", "g12", "g13", "g14", "fp", \
// OBSOLETE 	/* 32 */ "pc",  "ac",  "tc",  "ip",  "fp0", "fp1", "fp2", "fp3",\
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Override the standard gdb prompt when compiled for this target.  */
// OBSOLETE 
// OBSOLETE #define	DEFAULT_PROMPT	"(gdb960) "
// OBSOLETE 
// OBSOLETE /* FRAME_CHAIN_VALID returns zero if the given frame is the outermost one
// OBSOLETE    and has no caller.
// OBSOLETE 
// OBSOLETE    On the i960, each various target system type defines FRAME_CHAIN_VALID,
// OBSOLETE    since it differs between Nindy, Mon960 and VxWorks, the currently supported
// OBSOLETE    target types.  */
// OBSOLETE 
// OBSOLETE extern int mon960_frame_chain_valid (CORE_ADDR, struct frame_info *);
// OBSOLETE #define	FRAME_CHAIN_VALID(chain, thisframe) mon960_frame_chain_valid (chain, thisframe)
// OBSOLETE 
// OBSOLETE /* Sequence of bytes for breakpoint instruction */
// OBSOLETE 
// OBSOLETE #define BREAKPOINT {0x00, 0x3e, 0x00, 0x66}
// OBSOLETE 
// OBSOLETE /* Amount ip must be decremented by after a breakpoint.
// OBSOLETE  * This is often the number of bytes in BREAKPOINT but not always.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE #define DECR_PC_AFTER_BREAK 4
