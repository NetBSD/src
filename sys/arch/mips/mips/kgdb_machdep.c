/*	$NetBSD: kgdb_machdep.c,v 1.17.32.1 2016/10/05 20:55:32 skrll Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kgdb_machdep.c,v 1.17.32.1 2016/10/05 20:55:32 skrll Exp $");

#include "opt_ddb.h"

#if defined(DDB)
#error "Can't build DDB and KGDB together."
#endif

/*
 * Machine-dependent functions for remote KGDB.  Originally written
 * for NetBSD/pc532 by Matthias Pfaller.  Modified for NetBSD/i386
 * by Jason R. Thorpe.  Modified for NetBSD/mips by Ethan Solomita
 */

#include "opt_cputype.h"	/* which mips CPUs do we support? */

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/kgdb.h>

#include <uvm/uvm_extern.h>

#include <mips/pte.h>
#include <mips/cpu.h>
#include <mips/locore.h>
#include <mips/mips_opcode.h>
#include <mips/reg.h>
#include <mips/trap.h>
#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_access.h>

/*
 * Is kva a valid address to access?  This is used by KGDB.
 */
static int
kvacc(vaddr_t kva)
{
	if (pmap_md_direct_mapped_vaddr_p(kva))
		return 1;
	
	if (kva < VM_MIN_KERNEL_ADDRESS || kva >= VM_MAX_KERNEL_ADDRESS)
		return 0;

	const pt_entry_t * const ptep = pmap_pte_lookup(pmap_kernel(), kva);
	return ptep != NULL && pte_valid_p(*pte);
}

/*
 * Determine if the memory at va..(va+len) is valid.
 */
int
kgdb_acc(vaddr_t va, size_t len)
{
	vaddr_t last_va;

	last_va = va + len + PAGE_SIZE - 1;
	va  &= ~PGOFSET;
	last_va &= ~PGOFSET;

	for (; va < last_va; va += PAGE_SIZE) {
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
	case T_TLB_MOD:
	case T_TLB_MOD+T_USER:
	case T_TLB_LD_MISS:
	case T_TLB_ST_MISS:
	case T_TLB_LD_MISS+T_USER:
	case T_TLB_ST_MISS+T_USER:
	case T_ADDR_ERR_LD:	/* misaligned access */
	case T_ADDR_ERR_ST:	/* misaligned access */
	case T_BUS_ERR_LD_ST:	/* BERR asserted to CPU */
	case T_ADDR_ERR_LD+T_USER:	/* misaligned or kseg access */
	case T_ADDR_ERR_ST+T_USER:	/* misaligned or kseg access */
	case T_BUS_ERR_IFETCH+T_USER:	/* BERR asserted to CPU */
	case T_BUS_ERR_LD_ST+T_USER:	/* BERR asserted to CPU */
		return (SIGSEGV);

	case T_BREAK:
	case T_BREAK+T_USER:
		return (SIGTRAP);

	case T_RES_INST+T_USER:
	case T_COP_UNUSABLE+T_USER:
		return (SIGILL);

	case T_FPE+T_USER:
	case T_OVFLOW+T_USER:
		return (SIGFPE);
		
	default:
		return (SIGEMT);
	}
}

mips_reg_t kgdb_cause, kgdb_vaddr; /* set by trap() */

/*
 * Translate the values stored in the db_regs_t struct to the format
 * understood by gdb.
 */
void
kgdb_getregs(db_regs_t *regs, kgdb_reg_t *gdb_regs)
{
	memset(gdb_regs, 0, KGDB_NUMREGS * sizeof(kgdb_reg_t));
	gdb_regs[ 1] = regs->r_regs[_R_AST];	/* AT */
	gdb_regs[ 2] = regs->r_regs[_R_V0];	/* V0 */
	gdb_regs[ 3] = regs->r_regs[_R_V1];	/* V1 */
	gdb_regs[ 4] = regs->r_regs[_R_A0];	/* A0 */
	gdb_regs[ 5] = regs->r_regs[_R_A1];	/* A1 */
	gdb_regs[ 6] = regs->r_regs[_R_A2];	/* A2 */
	gdb_regs[ 7] = regs->r_regs[_R_A3];	/* A3 */
	gdb_regs[ 8] = regs->r_regs[_R_T0];	/* T0 */
	gdb_regs[ 9] = regs->r_regs[_R_T1];	/* T1 */
	gdb_regs[10] = regs->r_regs[_R_T2];	/* T2 */
	gdb_regs[11] = regs->r_regs[_R_T3];	/* T3 */
	gdb_regs[12] = regs->r_regs[_R_T4];	/* T4 */
	gdb_regs[13] = regs->r_regs[_R_T5];	/* T5 */
	gdb_regs[14] = regs->r_regs[_R_T6];	/* T6 */
	gdb_regs[15] = regs->r_regs[_R_T7];	/* T7 */
	gdb_regs[16] = regs->r_regs[_R_S0];	/* S0 */
	gdb_regs[17] = regs->r_regs[_R_S1];	/* S1 */
	gdb_regs[18] = regs->r_regs[_R_S2];	/* S2 */
	gdb_regs[19] = regs->r_regs[_R_S3];	/* S3 */
	gdb_regs[20] = regs->r_regs[_R_S4];	/* S4 */
	gdb_regs[21] = regs->r_regs[_R_S5];	/* S5 */
	gdb_regs[22] = regs->r_regs[_R_S6];	/* S6 */
	gdb_regs[23] = regs->r_regs[_R_S7];	/* S7 */
	gdb_regs[24] = regs->r_regs[_R_T8];	/* T8 */
	gdb_regs[25] = regs->r_regs[_R_T9];	/* T9 */
	gdb_regs[28] = regs->r_regs[_R_GP];	/* GP */
	gdb_regs[29] = regs->r_regs[_R_SP];	/* SP */
	gdb_regs[30] = regs->r_regs[_R_S8];	/* S8 */
	gdb_regs[31] = regs->r_regs[_R_RA];	/* RA */
	gdb_regs[32] = regs->r_regs[_R_SR];	/* SR */
	gdb_regs[33] = regs->r_regs[_R_MULLO];	/* MULLO */
	gdb_regs[34] = regs->r_regs[_R_MULHI];	/* MULHI */
	gdb_regs[35] = kgdb_vaddr;		/* BAD VADDR */
	gdb_regs[36] = kgdb_cause;		/* CAUSE */
	gdb_regs[37] = regs->r_regs[_R_PC];	/* PC */
}

/*
 * Reverse the above.
 */
void
kgdb_setregs(db_regs_t *regs, kgdb_reg_t *gdb_regs)
{
	regs->r_regs[_R_PC] = gdb_regs[37];   /* PC */
}	

/*
 * Trap into kgdb to wait for debugger to connect,
 * noting on the console why nothing else is going on.
 */
void
kgdb_connect(int verbose)
{
	if (kgdb_dev < 0)
		return;

	if (verbose)
		printf("kgdb waiting...");

	__asm("break");

	if (verbose)
		printf("connected.\n");

	kgdb_debug_panic = 1;
}

/*
 * Decide what to do on panic.
 * (This is called by panic, like Debugger())
 */
void
kgdb_panic(void)
{
	if (kgdb_dev != NODEV && kgdb_debug_panic) {
		printf("entering kgdb\n");
		kgdb_connect(kgdb_active == 0);
	}
}
