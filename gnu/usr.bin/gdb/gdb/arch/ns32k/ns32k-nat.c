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

	$Id: ns32k-nat.c,v 1.2 1995/07/28 08:00:17 phil Exp $
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

#ifdef notdef
struct reg {
	unsigned int 	r_r7;
	unsigned int 	r_r6;
	unsigned int 	r_r5;
	unsigned int 	r_r4;
	unsigned int 	r_r3;
	unsigned int 	r_r2;
	unsigned int 	r_r1;
	unsigned int 	r_r0;

	unsigned int 	r_sp;
	unsigned int 	r_sb;
	unsigned int 	r_fp;
	unsigned int 	r_pc;
	unsigned int 	r_psr;
};
#define REGISTER_NAMES {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", \
                        "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", \
			"sp", "fp", "pc", "ps",                         \
			"fsr",                                          \
			"l0", "l1", "l2", "l3", "xx",                   \
			}

struct gdb_regs {
	unsigned int	r0;
	unsigned int	r1;
	unsigned int	r2;
	unsigned int	r3;
	unsigned int	r4;
	unsigned int	r5;
	unsigned int	r6;
	unsigned int	r7;
	float		f0;
	float		f1;
	float		f2;
	float		f3;
	float		f4;
	float		f5;
	float		f6;
	float		f7;
	unsigned int	sp;
	unsigned int	fp;
	unsigned int	pc;
	unsigned int	ps;
	unsigned int	fsr;
	double		l0;
	double		l1;
	double		l2;
	double		l3;
	double		l4;
	double		l5;
	double		l6;
	double		l7;
};

#endif

int regMap[26] = {
	7, 6, 5, 4, 3, 2, 1, 0,
	-1, -1, -1, -1, -1, -1, -1, -1,
	8, 10, 11, 12,
	-1,
	-1, -1, -1, -1, -1
};


int	inFetchReg = 0;

void
fetch_register(int regno)
{
	union      {
		struct reg	regs;
		int		r[13];
	} u;
	struct fpreg	fp_regs;
	int		*p;
	char		mess[128];

	if (regMap[regno] == -1) {
		/* Supply zeroes */
		memset(mess, '\0', REGISTER_RAW_SIZE (regno));
		supply_register(regno, mess);
		return;
	}

	if (!inFetchReg) {
		inFetchReg = 1;
		if (ptrace(PT_GETREGS, inferior_pid, (PTRACE_ARG3_TYPE) &u.regs,
				0) == -1) {
			sprintf(mess, "reading register %s (#%d)",
					reg_names[regno], regno);
			perror_with_name(mess);
		}

		if (ptrace(PT_GETFPREGS, inferior_pid,
				  (PTRACE_ARG3_TYPE) &fp_regs, 0) == -1) {
			sprintf(mess, "reading fp regs");
			perror_with_name(mess);

		} else {
			fprintf(stderr, "FP Regs: fpsr %08x\n", fp_regs.r_fpsr);
			p = (int *) &fp_regs.r_f0;
			fprintf(stderr, "     f0 %08x %08x\n", p[1], p[0]);
			p += 2;
			fprintf(stderr, "     f1 %08x %08x\n", p[1], p[0]);
			p += 2;
			fprintf(stderr, "     f2 %08x %08x\n", p[1], p[0]);
			p += 2;
			fprintf(stderr, "     f3 %08x %08x\n", p[1], p[0]);
			p += 2;
			fprintf(stderr, "     f4 %08x %08x\n", p[1], p[0]);
			p += 2;
			fprintf(stderr, "     f5 %08x %08x\n", p[1], p[0]);
			p += 2;
			fprintf(stderr, "     f6 %08x %08x\n", p[1], p[0]);
			p += 2;
			fprintf(stderr, "     f7 %08x %08x\n", p[1], p[0]);
			p += 2;
		}
	}

	supply_register(regno, &u.r[regMap[regno]]);
}


/* Fetch all registers, or just one, from the child process.  */

void
fetch_inferior_registers(int regno)
{
	if (regno == -1)
		for (regno = 0; regno < NUM_REGS; regno++)
			fetch_register(regno);
	else
		fetch_register(regno);

	inFetchReg = 0;
}

#ifdef notdef

void
fetch_inferior_registers(regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg fp_regs;

  ptrace (PT_GETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);

  memcpy (&registers[REGISTER_BYTE (0)], &inferior_registers,
		sizeof(unsigned int) * 14);

#ifdef notdef
  ptrace (PT_GETFPREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &fp_regs, 0);
fprintf(stderr, "FP Regs: fpsr %08x\n", fp_regs.r_fpsr);
fprintf(stderr, "         f0 %f\n", fp_regs.r_f0);
fprintf(stderr, "         f1 %f\n", fp_regs.r_f1);
fprintf(stderr, "         f2 %f\n", fp_regs.r_f2);
fprintf(stderr, "         f3 %f\n", fp_regs.r_f3);
fprintf(stderr, "         f4 %f\n", fp_regs.r_f4);
fprintf(stderr, "         f5 %f\n", fp_regs.r_f5);
fprintf(stderr, "         f6 %f\n", fp_regs.r_f6);
fprintf(stderr, "         f7 %f\n", fp_regs.r_f7);
#endif

  registers_fetched ();
}

#endif

void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;

  memcpy (&inferior_registers, &registers[REGISTER_BYTE (0)],
		sizeof(unsigned int) * 14);

  ptrace (PT_SETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);
}
