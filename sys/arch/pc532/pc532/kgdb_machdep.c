/*	$NetBSD: kgdb_machdep.c,v 1.8 2000/06/29 07:51:46 mrg Exp $	*/

/*
 * Copyright (c) 1996 Matthias Pfaller.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Matthias Pfaller.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kgdb.h>

#include <uvm/uvm_extern.h>

#include <machine/cpufunc.h>
#include <machine/frame.h>
#include <machine/pte.h>
#include <machine/reg.h>
#include <machine/trap.h>

/*
 * Determine if the memory at va..(va+len) is valid.
 */
int
kgdb_acc(va, len)
	vaddr_t va;
	size_t len;
{
	vaddr_t last_va;
	pt_entry_t *pte;

	last_va = va + len;
	va  &= ~PGOFSET;
	last_va &= ~PGOFSET;

	do {
		pte = kvtopte(va);
		if ((*pte & PG_V) == 0)
			return (0);
		va  += NBPG;
	} while (va < last_va);

	return (1);
}

/*
 * Translate a trap number into a unix compatible signal value.
 * (gdb only understands unix signal numbers).
 */
int 
kgdb_signal(type)
	int type;
{
	switch (type) {
	case T_NVI:
	case T_NMI:
	case T_AST:
		return(SIGINT);
	case T_ABT:
	case T_NBE:
	case T_RBE:
		return(SIGBUS);
	case T_SLAVE:
	case T_ILL:
	case T_SVC:
	case T_FLG:
	case T_UND:
		return(SIGILL);
	case T_DVZ:
	case T_OVF:
		return(SIGFPE);
	case T_BPT:
	case T_DBG:
	case T_TRC:
		return(SIGTRAP);
	default:
		return(SIGEMT);
	}
}

/*
 * Definitions exported from gdb.
 */
#define NUM_REGS		25
#define	R0_REGNUM 0		/* General register 0 */
#define SP_REGNUM 16		/* Contains address of top of stack */
#define FP_REGNUM 17		/* Contains address of executing stack frame */
#define PC_REGNUM 18		/* Contains program counter */
#define PS_REGNUM 19		/* Contains processor status */

/*
 * Translate the values stored in the kernel regs struct to the format
 * understood by gdb.
 */
void
kgdb_getregs(regs, gdb_regs)
	db_regs_t *regs;
	kgdb_reg_t *gdb_regs;
{
	gdb_regs[R0_REGNUM + 0] = regs->tf_regs.r_r0;
	gdb_regs[R0_REGNUM + 1] = regs->tf_regs.r_r1;
	gdb_regs[R0_REGNUM + 2] = regs->tf_regs.r_r2;
	gdb_regs[R0_REGNUM + 3] = regs->tf_regs.r_r3;
	gdb_regs[R0_REGNUM + 4] = regs->tf_regs.r_r4;
	gdb_regs[R0_REGNUM + 5] = regs->tf_regs.r_r5;
	gdb_regs[R0_REGNUM + 6] = regs->tf_regs.r_r6;
	gdb_regs[R0_REGNUM + 7] = regs->tf_regs.r_r7;

	gdb_regs[SP_REGNUM    ] = regs->tf_regs.r_sp;
	gdb_regs[FP_REGNUM    ] = regs->tf_regs.r_fp;
	gdb_regs[PC_REGNUM    ] = regs->tf_regs.r_pc;
	gdb_regs[PS_REGNUM    ] = regs->tf_regs.r_psr;
}

/*
 * Reverse the above.
 */
void
kgdb_setregs(regs, gdb_regs)
	db_regs_t *regs;
	kgdb_reg_t *gdb_regs;
{
	regs->tf_regs.r_r0  = gdb_regs[R0_REGNUM + 0];
	regs->tf_regs.r_r1  = gdb_regs[R0_REGNUM + 1];
	regs->tf_regs.r_r2  = gdb_regs[R0_REGNUM + 2];
	regs->tf_regs.r_r3  = gdb_regs[R0_REGNUM + 3];
	regs->tf_regs.r_r4  = gdb_regs[R0_REGNUM + 4];
	regs->tf_regs.r_r5  = gdb_regs[R0_REGNUM + 5];
	regs->tf_regs.r_r6  = gdb_regs[R0_REGNUM + 6];
	regs->tf_regs.r_r7  = gdb_regs[R0_REGNUM + 7];

	regs->tf_regs.r_sp  = gdb_regs[SP_REGNUM];
	regs->tf_regs.r_fp  = gdb_regs[FP_REGNUM];
	regs->tf_regs.r_pc  = gdb_regs[PC_REGNUM];
	regs->tf_regs.r_psr = gdb_regs[PS_REGNUM]
				| (regs->tf_regs.r_psr & PSL_T);
}	

/*
 * Trap into kgdb to wait for debugger to connect,
 * noting on the console why nothing else is going on.
 */
void
kgdb_connect(verbose)
	int verbose;
{

	if (kgdb_dev < 0)
		return;

	if (verbose)
		printf("kgdb waiting...");

	breakpoint();

	if (verbose)
		printf("connected.\n");

	kgdb_debug_panic = 1;
}

/*
 * Decide what to do on panic.
 * (This is called by panic, like Debugger())
 */
void
kgdb_panic()
{
	if (kgdb_dev >= 0 && kgdb_debug_panic) {
		printf("entering kgdb\n");
		kgdb_connect(kgdb_active == 0);
	}
}
