/* Native-dependent code for BSD Unix running on ns32k's, for GDB.
   Copyright 1988, 1989, 1991, 1992 Free Software Foundation, Inc.

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

	$Id: ns32k-nat.c,v 1.3 1995/08/29 08:03:29 phil Exp $
*/

#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/user.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <sys/ptrace.h>

#include "defs.h"
#include "inferior.h"

/* Incomplete -- PAN */

#define RF(dst, src) \
	memcpy(&registers[REGISTER_BYTE(dst)], &src, sizeof(src))

#define RS(src, dst) \
	memcpy(&dst, &registers[REGISTER_BYTE(src)], sizeof(dst))

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fpregisters;

  ptrace (PT_GETREGS, inferior_pid,
  		(PTRACE_ARG3_TYPE) &inferior_registers, 0);
  ptrace (PT_GETFPREGS, inferior_pid,
  		(PTRACE_ARG3_TYPE) &inferior_fpregisters, 0);

  RF(R0_REGNUM + 0, inferior_registers.r_r0);
  RF(R0_REGNUM + 1, inferior_registers.r_r1);
  RF(R0_REGNUM + 2, inferior_registers.r_r2);
  RF(R0_REGNUM + 3, inferior_registers.r_r3);
  RF(R0_REGNUM + 4, inferior_registers.r_r4);
  RF(R0_REGNUM + 5, inferior_registers.r_r5);
  RF(R0_REGNUM + 6, inferior_registers.r_r6);
  RF(R0_REGNUM + 7, inferior_registers.r_r7);

  RF(SP_REGNUM	  , inferior_registers.r_sp);
  RF(FP_REGNUM	  , inferior_registers.r_fp);
  RF(PC_REGNUM	  , inferior_registers.r_pc);
  RF(PS_REGNUM	  , inferior_registers.r_psr);

  RF(FPS_REGNUM   , inferior_fpregisters.r_fsr);
  RF(FP0_REGNUM +0, inferior_fpregisters.r_freg[0]);
  RF(FP0_REGNUM +2, inferior_fpregisters.r_freg[2]);
  RF(FP0_REGNUM +4, inferior_fpregisters.r_freg[4]);
  RF(FP0_REGNUM +6, inferior_fpregisters.r_freg[6]);
  registers_fetched ();
}
  
void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fpregisters;

  RS(R0_REGNUM + 0, inferior_registers.r_r0);
  RS(R0_REGNUM + 1, inferior_registers.r_r1);
  RS(R0_REGNUM + 2, inferior_registers.r_r2);
  RS(R0_REGNUM + 3, inferior_registers.r_r3);
  RS(R0_REGNUM + 4, inferior_registers.r_r4);
  RS(R0_REGNUM + 5, inferior_registers.r_r5);
  RS(R0_REGNUM + 6, inferior_registers.r_r6);
  RS(R0_REGNUM + 7, inferior_registers.r_r7);

  RS(SP_REGNUM	  , inferior_registers.r_sp);
  RS(FP_REGNUM	  , inferior_registers.r_fp);
  RS(PC_REGNUM	  , inferior_registers.r_pc);
  RS(PS_REGNUM	  , inferior_registers.r_psr);

  RS(FPS_REGNUM   , inferior_fpregisters.r_fsr);
  RS(FP0_REGNUM +0, inferior_fpregisters.r_freg[0]);
  RS(FP0_REGNUM +2, inferior_fpregisters.r_freg[2]);
  RS(FP0_REGNUM +4, inferior_fpregisters.r_freg[4]);
  RS(FP0_REGNUM +6, inferior_fpregisters.r_freg[6]);

  ptrace (PT_SETREGS, inferior_pid,
  		(PTRACE_ARG3_TYPE) &inferior_registers, 0);
  ptrace (PT_SETFPREGS, inferior_pid,
  		(PTRACE_ARG3_TYPE) &inferior_fpregisters, 0);
}

void
fetch_kcore_registers (pcb)
struct pcb *pcb;
{
	return;
}

void
clear_regs()
{
	return;
}

