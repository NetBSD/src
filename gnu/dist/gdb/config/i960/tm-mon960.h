/* Parameters for Intel 960 running MON960 monitor, for GDB, the GNU debugger.
   Copyright (C) 1990-1991 Free Software Foundation, Inc.
   Contributed by Intel Corporation and Cygnus Support.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/*****************************************************************************
 * Definitions to target GDB to an i960 debugged over a serial line.
 ******************************************************************************/

#include "i960/tm-i960.h"

/* redefined from tm-i960.h */
/* Number of machine registers */
#undef NUM_REGS 
#define NUM_REGS 40

/* Initializer for an array of names of registers.
   There should be NUM_REGS strings in this initializer.  */
#undef REGISTER_NAMES 
#define REGISTER_NAMES { \
	/*  0 */ "pfp", "sp",  "rip", "r3",  "r4",  "r5",  "r6",  "r7", \
	/*  8 */ "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",\
	/* 16 */ "g0",  "g1",  "g2",  "g3",  "g4",  "g5",  "g6",  "g7", \
	/* 24 */ "g8",  "g9",  "g10", "g11", "g12", "g13", "g14", "fp", \
	/* 32 */ "pc",  "ac",  "tc",  "ip",  "fp0", "fp1", "fp2", "fp3",\
}


/* Override the standard gdb prompt when compiled for this target.  */

#define	DEFAULT_PROMPT	"(gdb960) "

/* Additional command line options accepted by mon960 gdb's, for handling
   the remote-mon960.c interface.  These should really be target-specific
   rather than architecture-specific.  */

/* FIXME - should use this instead of the "send_break" hack in monitor.c */
extern int mon960_initial_brk;	/* Send a BREAK to reset board first */
extern char *mon960_ttyname;	/* Name of serial port to talk to mon960 */

#define	ADDITIONAL_OPTIONS \
	/* FIXME {"brk", no_argument, &mon960_initial_brk, 1}, */	\
	{"ser", required_argument, 0, 1004},  /* 1004 is magic cookie for ADDL_CASES */

#define	ADDITIONAL_OPTION_CASES	\
	case 1004:	/* -ser option:  remote mon960 auto-start */	\
	  mon960_ttyname = optarg;	\
	  break;

#define	ADDITIONAL_OPTION_HELP \
	"\
  /* FIXME - -brk              Send a break to a Mon960 target to reset it.\n*/\
  -ser SERIAL       Open remote Mon960 session to SERIAL port.\n\
"

/* FRAME_CHAIN_VALID returns zero if the given frame is the outermost one
   and has no caller.

   On the i960, each various target system type defines FRAME_CHAIN_VALID,
   since it differs between Nindy, Mon960 and VxWorks, the currently supported
   targets types.  */

#define	FRAME_CHAIN_VALID(chain, thisframe) \
	mon960_frame_chain_valid (chain, thisframe)

extern int
mon960_frame_chain_valid();		/* See i960-tdep.c */

/* Sequence of bytes for breakpoint instruction */

#define BREAKPOINT {0x00, 0x3e, 0x00, 0x66}

/* Amount ip must be decremented by after a breakpoint.
 * This is often the number of bytes in BREAKPOINT but not always.
 */

#define DECR_PC_AFTER_BREAK 4
