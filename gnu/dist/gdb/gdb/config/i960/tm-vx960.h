// OBSOLETE /* Parameters for VxWorks Intel 960's, for GDB, the GNU debugger.
// OBSOLETE    Copyright 1986, 1987, 1988, 1989, 1990, 1991, 1993, 1998, 1999
// OBSOLETE    Free Software Foundation, Inc.
// OBSOLETE    Contributed by Cygnus Support.
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
// OBSOLETE #include "i960/tm-i960.h"
// OBSOLETE #include "config/tm-vxworks.h"
// OBSOLETE 
// OBSOLETE /* Under VxWorks the IP isn't filled in.  Skip it, go with RIP, which has
// OBSOLETE    the real value.  */
// OBSOLETE #undef PC_REGNUM
// OBSOLETE #define PC_REGNUM RIP_REGNUM
// OBSOLETE 
// OBSOLETE /* We have more complex, useful breakpoints on the target.
// OBSOLETE    Amount ip must be decremented by after a breakpoint.  */
// OBSOLETE 
// OBSOLETE #define	DECR_PC_AFTER_BREAK	0
// OBSOLETE 
// OBSOLETE /* We are guaranteed to have a zero frame pointer at bottom of stack, too. */
// OBSOLETE 
// OBSOLETE #define FRAME_CHAIN_VALID(chain, thisframe) nonnull_frame_chain_valid (chain, thisframe)
// OBSOLETE 
// OBSOLETE /* Breakpoint patching is handled at the target end in VxWorks.  */
// OBSOLETE /* #define BREAKPOINT {0x00, 0x3e, 0x00, 0x66} */
// OBSOLETE 
// OBSOLETE /* Number of registers in a ptrace_getregs call. */
// OBSOLETE 
// OBSOLETE #define VX_NUM_REGS (16 + 16 + 3)
// OBSOLETE 
// OBSOLETE /* Number of registers in a ptrace_getfpregs call. */
// OBSOLETE 
// OBSOLETE     /* @@ Can't use this -- the rdb library for the 960 target
// OBSOLETE        doesn't support setting or retrieving FP regs.  KR  */
// OBSOLETE 
// OBSOLETE /* #define VX_SIZE_FPREGS (REGISTER_RAW_SIZE (FP0_REGNUM) * 4) */
