/* Macro definitions for ns532, Mach 3.0
   Copyright (C) 1992 Free Software Foundation, Inc.

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

/* Include common definitions for Mach3 systems */
#include "nm-m3.h"

/* Define offsets to access CPROC stack when it does not have
 * a kernel thread.
 */
#define MACHINE_CPROC_SP_OFFSET 20
#define MACHINE_CPROC_PC_OFFSET 16
#define MACHINE_CPROC_FP_OFFSET 12

#include <ns532/psl.h>
#include <ns532/vmparam.h>

/* Thread flavors used in re-setting the T bit.
 * @@ this is also bad for cross debugging.
 */
#define TRACE_FLAVOR		NS532_THREAD_STATE
#define TRACE_FLAVOR_SIZE	NS532_THREAD_STATE_COUNT
#define TRACE_SET(x,state) \
  	((struct ns532_thread_state *)state)->psr |= PSR_T
#define TRACE_CLEAR(x,state) \
  	((((struct ns532_thread_state *)state)->psr &= ~PSR_T), 1)

/* we can do it */
#define ATTACH_DETACH 1

/* Address of end of stack space.
 * for MACH, see <ns532/vmparam.h>
 */
#define STACK_END_ADDR USRSTACK

#include "ns32k/tm-umax.h"

/* tm-umax.h assumes a 32082 fpu. We have a 32382 fpu. */
#undef REGISTER_NAMES
#undef NUM_REGS
#undef REGISTER_BYTES
/* Initializer for an array of names of registers.
   There should be NUM_REGS strings in this initializer.  */

#define REGISTER_NAMES {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",	\
 			"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",	\
			"sp", "fp", "pc", "ps",				\
 			"fsr",						\
			"l0", "l1", "l2", "l3", "l4", "l5", "l6", "l7", "xx",			\
 			}

#define NUM_REGS		29

/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers'.  */
#define REGISTER_BYTES \
  ((NUM_REGS - 4) * REGISTER_RAW_SIZE(R0_REGNUM) \
   + 8            * REGISTER_RAW_SIZE(LP0_REGNUM))

/* tm-umax.h assumes a 32082 fpu. We have a 32382 fpu. */
#undef REGISTER_NAMES
#undef NUM_REGS
#undef REGISTER_BYTES
/* Initializer for an array of names of registers.
   There should be NUM_REGS strings in this initializer.  */

#define REGISTER_NAMES {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",	\
 			"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",	\
			"sp", "fp", "pc", "ps",				\
 			"fsr",						\
			"l0", "l1", "l2", "l3", "l4", "l5", "l6", "l7", "xx",			\
 			}

#define NUM_REGS		29

/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers'.  */
#define REGISTER_BYTES \
  ((NUM_REGS - 4) * REGISTER_RAW_SIZE(R0_REGNUM) \
   + 8            * REGISTER_RAW_SIZE(LP0_REGNUM))
