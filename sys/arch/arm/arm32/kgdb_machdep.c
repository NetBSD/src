/*	$NetBSD: kgdb_machdep.c,v 1.4 2005/12/24 22:45:34 perry Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kgdb_machdep.c,v 1.4 2005/12/24 22:45:34 perry Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kgdb.h>

#include <uvm/uvm_extern.h>

#include <machine/frame.h>
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

	last_va = va + len;
	va  &= ~PGOFSET;
	last_va &= ~PGOFSET;

	do {
		if (db_validate_address(va))
			return (0);
		va  += PAGE_SIZE;
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
	case T_BREAKPOINT:
		return(SIGTRAP);
	case -1:
		return(SIGSEGV);
	default:
		return(SIGINT);
	}
}

/*
 * Definitions exported from gdb.
 */

/*
 * Translate the values stored in the kernel regs struct to the format
 * understood by gdb.
 */
void
kgdb_getregs(regs, gdb_regs)
	db_regs_t *regs;
	kgdb_reg_t *gdb_regs;
{

	gdb_regs[KGDB_REGNUM_R0 +  0] = regs->tf_r0;
	gdb_regs[KGDB_REGNUM_R0 +  1] = regs->tf_r1;
	gdb_regs[KGDB_REGNUM_R0 +  2] = regs->tf_r2;
	gdb_regs[KGDB_REGNUM_R0 +  3] = regs->tf_r3;
	gdb_regs[KGDB_REGNUM_R0 +  4] = regs->tf_r4;
	gdb_regs[KGDB_REGNUM_R0 +  5] = regs->tf_r5;
	gdb_regs[KGDB_REGNUM_R0 +  6] = regs->tf_r6;
	gdb_regs[KGDB_REGNUM_R0 +  7] = regs->tf_r7;
	gdb_regs[KGDB_REGNUM_R0 +  8] = regs->tf_r8;
	gdb_regs[KGDB_REGNUM_R0 +  9] = regs->tf_r9;
	gdb_regs[KGDB_REGNUM_R0 + 10] = regs->tf_r10;
	gdb_regs[KGDB_REGNUM_R0 + 11] = regs->tf_r11;
	gdb_regs[KGDB_REGNUM_R0 + 12] = regs->tf_r12;
	gdb_regs[KGDB_REGNUM_R0 + 13] = regs->tf_svc_sp;
	gdb_regs[KGDB_REGNUM_R0 + 14] = regs->tf_svc_lr;
	gdb_regs[KGDB_REGNUM_R0 + 15] = regs->tf_pc;

	gdb_regs[KGDB_REGNUM_SPSR] = regs->tf_spsr;
}

/*
 * Reverse the above.
 */
void
kgdb_setregs(regs, gdb_regs)
	db_regs_t *regs;
	kgdb_reg_t *gdb_regs;
{
	regs->tf_r0     = gdb_regs[KGDB_REGNUM_R0 +  0];
	regs->tf_r1     = gdb_regs[KGDB_REGNUM_R0 +  1];
	regs->tf_r2     = gdb_regs[KGDB_REGNUM_R0 +  2];
	regs->tf_r3     = gdb_regs[KGDB_REGNUM_R0 +  3];
	regs->tf_r4     = gdb_regs[KGDB_REGNUM_R0 +  4];
	regs->tf_r5     = gdb_regs[KGDB_REGNUM_R0 +  5];
	regs->tf_r6     = gdb_regs[KGDB_REGNUM_R0 +  6];
	regs->tf_r7     = gdb_regs[KGDB_REGNUM_R0 +  7];
	regs->tf_r8     = gdb_regs[KGDB_REGNUM_R0 +  8];
	regs->tf_r9     = gdb_regs[KGDB_REGNUM_R0 +  9];
	regs->tf_r10    = gdb_regs[KGDB_REGNUM_R0 + 10];
	regs->tf_r11    = gdb_regs[KGDB_REGNUM_R0 + 11];
	regs->tf_r12    = gdb_regs[KGDB_REGNUM_R0 + 12];
	regs->tf_svc_sp = gdb_regs[KGDB_REGNUM_R0 + 13];
	regs->tf_svc_lr = gdb_regs[KGDB_REGNUM_R0 + 14];
	regs->tf_pc     = gdb_regs[KGDB_REGNUM_R0 + 15];

	regs->tf_spsr = gdb_regs[KGDB_REGNUM_SPSR];
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

	__asm volatile(KBPT_ASM);

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
