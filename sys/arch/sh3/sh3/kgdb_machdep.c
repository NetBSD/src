/*	$NetBSD: kgdb_machdep.c,v 1.8.6.2 2002/05/09 12:29:17 uch Exp $	*/

/*-
 * Copyright (c) 1997, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

#include "opt_ddb.h"

#if defined(DDB)
#error "Can't build DDB and KGDB together."
#endif

/*
 * Machine-dependent functions for remote KGDB.  Originally written
 * for NetBSD/pc532 by Matthias Pfaller.  Modified for NetBSD/i386
 * by Jason R. Thorpe.  Modified for NetBSD/mips by Ethan Solomita.
 * Modified for NetBSD/sh3 by UCHIYAMA Yasushi.
 */

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/reboot.h>
#include <sys/kgdb.h>

#include <uvm/uvm_extern.h>

#include <sh3/cpu.h>

#include <machine/db_machdep.h>
#include <ddb/db_access.h>

/*
 * Is kva a valid address to access?  This is used by KGDB.
 */
static int
kvacc(vaddr_t kva)
{
	pt_entry_t *pte;

	if (kva < SH3_P1SEG_BASE)
		return (0);

	if (kva < SH3_P2SEG_BASE)
		return (1);

	if (kva >= VM_MAX_KERNEL_ADDRESS)
		return (0);

	/* check kva is kernel virtual. */
	if ((kva < VM_MIN_KERNEL_ADDRESS) ||
	    (kva >= VM_MAX_KERNEL_ADDRESS))
		return (0);

	/* check page which related kva is valid. */
	pte = __pmap_kpte_lookup(kva);
	if (!(*pte & PG_V))
		return (0);

	return (1);
}

/*
 * Determine if the memory at va..(va+len) is valid.
 */
int
kgdb_acc(vaddr_t va, size_t len)
{
	vaddr_t last_va;

	last_va = va + len + NBPG - 1;
	va  &= ~PGOFSET;
	last_va &= ~PGOFSET;

	for (; va < last_va; va += NBPG) {
		if (kvacc(va) == 0)
			return 0;
	}

	return (1);
}

/*
 * Translate a trap number into a unix compatible signal value.
 * (gdb only understands unix signal numbers).
 */
int
kgdb_signal(int type)
{

	switch (type) {
	case EXPEVT_TLB_MISS_LD:
	case EXPEVT_TLB_MISS_ST:
	case EXPEVT_TLB_MOD:
	case EXPEVT_TLB_PROT_LD:
	case EXPEVT_TLB_PROT_ST:
	case EXPEVT_ADDR_ERR_LD:
	case EXPEVT_ADDR_ERR_ST:
	case EXPEVT_TLB_MISS_LD | EXP_USER:
	case EXPEVT_TLB_MISS_ST | EXP_USER:
	case EXPEVT_TLB_MOD | EXP_USER:
	case EXPEVT_TLB_PROT_LD | EXP_USER:
	case EXPEVT_TLB_PROT_ST | EXP_USER:
	case EXPEVT_ADDR_ERR_LD | EXP_USER:
	case EXPEVT_ADDR_ERR_ST | EXP_USER:
		return (SIGSEGV);

	case EXPEVT_TRAPA:
	case EXPEVT_BREAK:
	case EXPEVT_TRAPA | EXP_USER:
	case EXPEVT_BREAK | EXP_USER:
		return (SIGTRAP);

	case EXPEVT_RES_INST:
	case EXPEVT_SLOT_INST:
	case EXPEVT_RES_INST | EXP_USER:
	case EXPEVT_SLOT_INST | EXP_USER:
		return (SIGILL);

	default:
		return (SIGEMT);
	}
}

/*
 * Translate the values stored in the db_regs_t struct to the format
 * understood by gdb. (gdb-5.1.1/gdb/config/sh/tm-sh.h)
 */
void
kgdb_getregs(db_regs_t *regs, kgdb_reg_t *gdb_regs)
{
	u_int32_t r;

	memset(gdb_regs, 0, KGDB_NUMREGS * sizeof(kgdb_reg_t));
	gdb_regs[ 0] = regs->tf_r0;
	gdb_regs[ 1] = regs->tf_r1;
	gdb_regs[ 2] = regs->tf_r2;
	gdb_regs[ 3] = regs->tf_r3;
	gdb_regs[ 4] = regs->tf_r4;
	gdb_regs[ 5] = regs->tf_r5;
	gdb_regs[ 6] = regs->tf_r6;
	gdb_regs[ 7] = regs->tf_r7;
	gdb_regs[ 8] = regs->tf_r8;
	gdb_regs[ 9] = regs->tf_r9;
	gdb_regs[10] = regs->tf_r10;
	gdb_regs[11] = regs->tf_r11;
	gdb_regs[12] = regs->tf_r12;
	gdb_regs[13] = regs->tf_r13;
	gdb_regs[14] = regs->tf_r14;
	gdb_regs[15] = regs->tf_r15;
	gdb_regs[16] = regs->tf_spc;
	gdb_regs[17] = regs->tf_pr;
	__asm__ __volatile__("stc vbr, %0" : "=r"(r));
	gdb_regs[19] = r;
	gdb_regs[20] = regs->tf_mach;
	gdb_regs[21] = regs->tf_macl;
	gdb_regs[22] = regs->tf_ssr;

	/* How treat register bank 1 ? */
}

/*
 * Reverse the above.
 */
void
kgdb_setregs(db_regs_t *regs, kgdb_reg_t *gdb_regs)
{

	regs->tf_r0	= gdb_regs[ 0];
	regs->tf_r1	= gdb_regs[ 1];
	regs->tf_r2	= gdb_regs[ 2];
	regs->tf_r3	= gdb_regs[ 3];
	regs->tf_r4	= gdb_regs[ 4];
	regs->tf_r5	= gdb_regs[ 5];
	regs->tf_r6	= gdb_regs[ 6];
	regs->tf_r7	= gdb_regs[ 7];
	regs->tf_r8	= gdb_regs[ 8];
	regs->tf_r9	= gdb_regs[ 9];
	regs->tf_r10	= gdb_regs[10];
	regs->tf_r11	= gdb_regs[11];
	regs->tf_r12	= gdb_regs[12];
	regs->tf_r13	= gdb_regs[13];
	regs->tf_r14	= gdb_regs[14];
	regs->tf_r15	= gdb_regs[15];
	regs->tf_spc	= gdb_regs[16];
	regs->tf_pr	= gdb_regs[17];
	__asm__ __volatile__("ldc %0, vbr" :: "r"(gdb_regs[19]));
	regs->tf_mach	= gdb_regs[20];
	regs->tf_macl	= gdb_regs[21];
	regs->tf_ssr	= gdb_regs[22];
}

/*
 * Trap into kgdb to wait for debugger to connect,
 * noting on the console why nothing else is going on.
 */
void
kgdb_connect(int verbose)
{

	if (kgdb_dev < 0) {
		printf("kgdb_dev=%d\n", kgdb_dev);
		return;
	}

	if (verbose)
		printf("kgdb waiting...");

	__asm__ __volatile__("trapa %0" :: "i"(_SH_TRA_BREAK));

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
