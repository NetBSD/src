/* Target definitions for GNU/Linux on ARM, for GDB.
   Copyright 1999, 2000 Free Software Foundation, Inc.

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

#ifndef TM_ARMLINUX_H
#define TM_ARMLINUX_H

/* Include the common ARM target definitions.  */
#include "arm/tm-arm.h"

#include "tm-linux.h"

/* Target byte order on ARM Linux is little endian and not selectable.  */
#undef TARGET_BYTE_ORDER_SELECTABLE_P
#define TARGET_BYTE_ORDER_SELECTABLE_P	0

/* Under ARM Linux the traditional way of performing a breakpoint is to
   execute a particular software interrupt, rather than use a particular
   undefined instruction to provoke a trap.  Upon exection of the software
   interrupt the kernel stops the inferior with a SIGTRAP, and wakes the
   debugger.  Since ARM Linux is little endian, and doesn't support Thumb
   at the moment we redefined ARM_LE_BREAKPOINT to use the correct software
   interrupt.  */
#undef ARM_LE_BREAKPOINT
#define ARM_LE_BREAKPOINT	{0x01,0x00,0x9f,0xef}

/* This sequence of words used in the CALL_DUMMY are the following 
   instructions:

   mov  lr, pc
   mov  pc, r4
   swi	bkpt_swi

   Note this is 12 bytes.  */

#undef CALL_DUMMY
#define CALL_DUMMY {0xe1a0e00f, 0xe1a0f004, 0xef9f001}

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */
extern void arm_linux_extract_return_value (struct type *, char[], char *);
#undef EXTRACT_RETURN_VALUE
#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
	arm_linux_extract_return_value ((TYPE), (REGBUF), (VALBUF))

/* Things needed for making the inferior call functions.  

   FIXME:  This and arm_push_arguments should be merged.  However this 
   	   function breaks on a little endian host, big endian target
   	   using the COFF file format.  ELF is ok.  
   	   
   	   ScottB.  */

#undef PUSH_ARGUMENTS
#define PUSH_ARGUMENTS(nargs, args, sp, struct_return, struct_addr) \
     sp = arm_linux_push_arguments ((nargs), (args), (sp), (struct_return), \
     				    (struct_addr))
extern CORE_ADDR arm_linux_push_arguments (int, struct value **, CORE_ADDR, 
					   int, CORE_ADDR);

/* The first page is not writeable in ARM Linux.  */
#undef LOWEST_PC
#define LOWEST_PC	0x8000

/* Define NO_SINGLE_STEP if ptrace(PT_STEP,...) fails to function correctly
   on ARM Linux.  This is the case on 2.0.x kernels, 2.1.x kernels and some 
   2.2.x kernels.  This will include the implementation of single_step()
   in armlinux-tdep.c.  See armlinux-ss.c for more details. */
/* #define NO_SINGLE_STEP	1 */

/* Offset to saved PC in sigcontext structure, from <asm/sigcontext.h> */
#define SIGCONTEXT_PC_OFFSET	(sizeof(unsigned long) * 18)

/* Figure out where the longjmp will land.  The code expects that longjmp
   has just been entered and the code had not altered the registers, so
   the arguments are are still in r0-r1.  r0 points at the jmp_buf structure
   from which the target pc (JB_PC) is extracted.  This pc value is copied
   into ADDR.  This routine returns true on success */
extern int arm_get_longjmp_target (CORE_ADDR *);
#define GET_LONGJMP_TARGET(addr)	arm_get_longjmp_target (addr)

/* On ARM Linux, each call to a library routine goes through a small piece
   of trampoline code in the ".plt" section.  The  wait_for_inferior() 
   routine uses this macro to detect when we have stepped into one of 
   these fragments.  We do not use lookup_solib_trampoline_symbol_by_pc,
   because we cannot always find the shared library trampoline symbols.  */
extern int in_plt_section (CORE_ADDR, char *);
#define IN_SOLIB_CALL_TRAMPOLINE(pc, name) in_plt_section((pc), (name))

/* On ARM Linux, a call to a library routine does not have to go through
   any trampoline code.  */
#define IN_SOLIB_RETURN_TRAMPOLINE(pc, name)	0

/* If PC is in a shared library trampoline code, return the PC
   where the function itself actually starts.  If not, return 0.  */
extern CORE_ADDR find_solib_trampoline_target (CORE_ADDR pc);   
#define SKIP_TRAMPOLINE_CODE(pc)  find_solib_trampoline_target (pc)

/* When we call a function in a shared library, and the PLT sends us
   into the dynamic linker to find the function's real address, we
   need to skip over the dynamic linker call.  This function decides
   when to skip, and where to skip to.  See the comments for
   SKIP_SOLIB_RESOLVER at the top of infrun.c.  */
extern CORE_ADDR arm_skip_solib_resolver (CORE_ADDR pc);
#define SKIP_SOLIB_RESOLVER arm_skip_solib_resolver

/* When we call a function in a shared library, and the PLT sends us
   into the dynamic linker to find the function's real address, we
   need to skip over the dynamic linker call.  This function decides
   when to skip, and where to skip to.  See the comments for
   SKIP_SOLIB_RESOLVER at the top of infrun.c.  */
#if 0   
#undef IN_SOLIB_DYNSYM_RESOLVE_CODE
extern CORE_ADDR arm_in_solib_dynsym_resolve_code (CORE_ADDR pc, char *name);
#define IN_SOLIB_DYNSYM_RESOLVE_CODE  arm_in_solib_dynsym_resolve_code
/* ScottB: Current definition is 
extern CORE_ADDR in_svr4_dynsym_resolve_code (CORE_ADDR pc, char *name);
#define IN_SOLIB_DYNSYM_RESOLVE_CODE  in_svr4_dynsym_resolve_code */
#endif

#endif /* TM_ARMLINUX_H */
