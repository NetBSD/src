/* Macro definitions for ns32k running under BSD Unix.
   Copyright 1986, 1987, 1989, 1991, 1992, 1993 Free Software Foundation, Inc.

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
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	$Id: tm.h,v 1.4 1996/09/28 08:41:28 matthias Exp $
*/

/* Override number of expected traps from sysv. */
#define START_INFERIOR_TRAPS_EXPECTED 2

/* Most definitions from umax could be used. */
#define INVALID_FLOAT(p, s) isa_NAN(p, s)
#include "tm-umax.h"

/* Shared library code */
#include "solib.h"

/* We define our own fetch and store methods. */
#define FETCH_INFERIOR_REGISTERS

/* Saved Pc.  Get it from sigcontext if within sigtramp.  */

/* Offset to saved PC in sigcontext, from <machine/signal.h>.  */
#define SIGCONTEXT_PC_OFFSET 20

#undef FRAME_SAVED_PC(FRAME)
#define FRAME_SAVED_PC(FRAME) \
  (((FRAME)->signal_handler_caller \
    ? sigtramp_saved_pc (FRAME) \
    : read_memory_integer ((FRAME)->frame + 4, 4)) \
   )

#undef FRAME_NUM_ARGS
#define FRAME_NUM_ARGS(numargs, fi) numargs = frame_num_args(fi)

#undef FRAME_CHAIN
#define FRAME_CHAIN(thisframe)  \
  (read_memory_integer ((thisframe)->frame, 4) > (thisframe)->frame ? \
   read_memory_integer ((thisframe)->frame, 4) : 0)

#define FRAME_CHAIN_VALID(chain, thisframe)	\
  ((chain) != 0					\
   && !inside_main_func ((thisframe) -> pc))
