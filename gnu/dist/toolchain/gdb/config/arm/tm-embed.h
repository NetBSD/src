/* Definitions to target GDB to ARM embedded systems.
   Copyright 1986-1989, 1991, 1993-1999 Free Software Foundation, Inc.

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

#ifndef TM_ARMEMBED_H
#define TM_ARMEMBED_H

/* Include the common ARM definitions. */
#include "arm/tm-arm.h"

/* I don't know the real values for these.  */
#define TARGET_UPAGES UPAGES
#define TARGET_NBPG NBPG

/* Address of end of stack space.  */
#define STACK_END_ADDR (0x01000000 - (TARGET_UPAGES * TARGET_NBPG))

/* The first 0x20 bytes are the trap vectors.  */
#undef LOWEST_PC
#define LOWEST_PC	0x20

/* Override defaults.  */

#undef THUMB_LE_BREAKPOINT
#define THUMB_LE_BREAKPOINT {0xbe,0xbe}       
#undef THUMB_BE_BREAKPOINT
#define THUMB_BE_BREAKPOINT {0xbe,0xbe}       

/* Specify that for the native compiler variables for a particular
   lexical context are listed after the beginning LBRAC instead of
   before in the executables list of symbols.  */
#define VARIABLES_INSIDE_BLOCK(desc, gcc_p) (!(gcc_p))

/* Functions for dealing with Thumb call thunks.  */
#define IN_SOLIB_CALL_TRAMPOLINE(pc, name)	arm_in_call_stub (pc, name)
#define SKIP_TRAMPOLINE_CODE(pc)		arm_skip_stub (pc)
extern int arm_in_call_stub PARAMS ((CORE_ADDR pc, char *name));
extern CORE_ADDR arm_skip_stub PARAMS ((CORE_ADDR pc));

/* Function to determine whether MEMADDR is in a Thumb function.  */
extern int arm_pc_is_thumb PARAMS ((bfd_vma memaddr));

/* Function to determine whether MEMADDR is in a call dummy called from
   a Thumb function.  */
extern int arm_pc_is_thumb_dummy PARAMS ((bfd_vma memaddr));


#undef  IN_SIGTRAMP
#define IN_SIGTRAMP(pc, name) 0

#endif /* TM_ARMEMBED_H */
