/* Parameters for Hewlett-Packard 9000/300 native support under bsd.
   Copyright 1986, 1987, 1989, 1991, 1992, 1993  Free Software Foundation, Inc.

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

	$Id: nm.h,v 1.1 1994/01/28 12:41:52 pk Exp $
*/

#define PTRACE_ARG3_TYPE        caddr_t

/* This is BSD 4.3 or something like it.  */

/* Get kernel u area address at run-time using BSD style nlist ().  */
#define KERNEL_U_ADDR_BSD

/* This is a piece of magic that is given a register number REGNO
   and as BLOCKEND the address in the system of the end of the user structure
   and stores in ADDR the address in the kernel or core dump
   of that register.  */

#define REGISTER_U_ADDR(addr, blockend, regno)				\
{									\
  if (regno < PS_REGNUM)						\
    addr = (int) &((struct frame *)(blockend))->f_regs[regno];		\
  else if (regno == PS_REGNUM)						\
    addr = (int) &((struct frame *)(blockend))->f_stackadj;		\
  else if (regno == PC_REGNUM)						\
    addr = (int) &((struct frame *)(blockend))->f_pc;			\
  else if (regno < FPC_REGNUM)						\
    addr = (int)							\
      &((struct user *)0)->u_pcb.pcb_fpregs.fpf_regs[((regno)-FP0_REGNUM)*3];\
  else if (regno == FPC_REGNUM)						\
    addr = (int) &((struct user *)0)->u_pcb.pcb_fpregs.fpf_fpcr;	\
  else if (regno == FPS_REGNUM)						\
    addr = (int) &((struct user *)0)->u_pcb.pcb_fpregs.fpf_fpsr;	\
  else									\
    addr = (int) &((struct user *)0)->u_pcb.pcb_fpregs.fpf_fpiar;	\
}
