/* Native-dependent code for BSD Unix running on i386's, for GDB.
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

	$Id: m68k-nat.c,v 1.6 1995/01/26 18:28:18 mycroft Exp $
*/

#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/user.h>
#include <machine/reg.h>
#include <sys/ptrace.h>

#include "defs.h"
#include "inferior.h"

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fp_registers;

  ptrace (PT_GETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);

  memcpy (&registers[REGISTER_BYTE (0)], &inferior_registers, 4*18);

  ptrace (PT_GETFPREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_fp_registers, 0);

  memcpy (&registers[REGISTER_BYTE (18)], &inferior_fp_registers, 8*12+4*3);

  registers_fetched ();
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fp_registers;

  memcpy (&inferior_registers, &registers[REGISTER_BYTE (0)], 4*18);

  ptrace (PT_SETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);

  memcpy (&inferior_fp_registers, &registers[REGISTER_BYTE (18)], 8*12+4*3);

  ptrace (PT_SETFPREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_fp_registers, 0);
}

void
fetch_kcore_registers(pcb)
struct pcb *pcb;
{
	int i;

	for (i = 2; i < 8; ++i)
		supply_register(i, &pcb->pcb_regs[i-2]);
	for (i = 10; i < 16; ++i)
		supply_register(i, &pcb->pcb_regs[i-4]);

	/* fake 'scratch' regs d0, d1, a0, a1 */
	i = 0;
	supply_register(0, &i); supply_register(1, &i);
	supply_register(8, &i); supply_register(9, &i);

	if (target_read_memory(pcb->pcb_regs[10] + 4, &i, sizeof i, 0))
		supply_register(PC_REGNUM, &i);

	supply_register(PS_REGNUM, &pcb->pcb_ps);

	for (i = FP0_REGNUM; i < NUM_REGS; ++i) {
		int fpreg;

		REGISTER_U_ADDR(fpreg, 0, i);
		supply_register(i, ((char *)pcb) + fpreg);
	}

	return;
}

void
clear_regs()
{
	u_long reg = 0;
	float freg = 0.0;
	int i = 0;

	for (; i < FP0_REGNUM; i++)
		supply_register(i, (char *)&reg);
	for (; i < FPC_REGNUM; i++)
		supply_register(i, (char *)&freg);
	for (; i < NUM_REGS; i++)
		supply_register(i, (char *)&reg);
}
