/* Macro definitions for Power PC running AIX.
   Copyright 1995 Free Software Foundation, Inc.

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

#ifndef TM_PPC_AIX_H
#define TM_PPC_AIX_H

/* Use generic RS6000 definitions. */
#include "rs6000/tm-rs6000.h"

#define GDB_TARGET_POWERPC

#undef PUSH_DUMMY_FRAME
#define PUSH_DUMMY_FRAME             generic_push_dummy_frame ()

#define PUSH_RETURN_ADDRESS(PC, SP)      ppc_push_return_address (PC, SP)

/* override the standard get_saved_register function with 
   one that takes account of generic CALL_DUMMY frames */
#define GET_SAVED_REGISTER(raw_buffer, optimized, addrp, frame, regnum, lval) \
      generic_get_saved_register (raw_buffer, optimized, addrp, frame, regnum, lval)

#define USE_GENERIC_DUMMY_FRAMES 1
#define CALL_DUMMY_BREAKPOINT_OFFSET (0)
#define CALL_DUMMY_LOCATION          AT_ENTRY_POINT
#define CALL_DUMMY_ADDRESS()         entry_point_address ()
#undef CALL_DUMMY_START_OFFSET
#define CALL_DUMMY_START_OFFSET      0

#endif /* TM_PPC_AIX_H */
