/*	$NetBSD: rmixl_subr.S,v 1.5.14.1 2017/12/03 11:36:28 jdolecek Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors.
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

#include "opt_cputype.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>

RCSID("$NetBSD: rmixl_subr.S,v 1.5.14.1 2017/12/03 11:36:28 jdolecek Exp $");

#include "assym.h"

#define RMIXL_COP_0_EIRR	_(9), 6
#define RMIXL_COP_0_EIMR	_(9), 7

	.set    noreorder
	.set	arch=xlr
	.text

/*
 * read XLS Processor Control register
 *
 * uint64_t rmixl_mfcr(u_int cr);
 */
LEAF(rmixl_mfcr)
#if defined(__mips_o32)
#error O32 not supported
#endif
	j	ra
	 mfcr	v0, a0
END(rmixl_mfcr)

/*
 * write XLS Processor Control register
 *
 * void rmixl_mtcr(u_int cr, uint64_t val);
 */
LEAF(rmixl_mtcr)
#if defined(__mips_o32)
#error O32 not supported
#endif
	j	ra
	 mtcr	a1, a0
END(rmixl_mtcr)

/*
 * void rmixl_eirr_ack(uint64_t eimr, uint64_t vecbit, uint64_t preserve)
 *
 *	ack in EIRR the irq we are about to handle
 *	disable all interrupts to prevent a race that would allow
 *	e.g. softints set from a higher interrupt getting
 *	clobbered by the EIRR read-modify-write 
 */
LEAF(rmixl_eirr_ack)
	dmtc0	zero, RMIXL_COP_0_EIMR	/* EIMR = 0 */
	COP0_SYNC
	dmfc0	a3, RMIXL_COP_0_EIRR	/* a3 = EIRR */
	and	a3, a2			/* a3 &= preserve */
	or	a3, a1			/* a3 |= vecbit */
	dmtc0	a3, RMIXL_COP_0_EIRR	/* EIRR = a3 */
	COP0_SYNC
	dmtc0	a0, RMIXL_COP_0_EIMR	/* EIMR = eimr */
	JR_HB_RA
END(rmixl_eirr_ack)

#ifdef MULTIPROCESSOR
/*
 * rmixlfw_wakeup_cpu(func, args, mask, callback)
 */
NESTED(rmixlfw_wakeup_cpu, CALLFRAME_SIZ+4*SZREG, ra)
	PTR_ADDU	sp, sp, -(CALLFRAME_SIZ+4*SZREG)
	REG_S		ra, CALLFRAME_RA(sp)
	REG_S		s0, CALLFRAME_S0(sp)
	REG_S		gp, CALLFRAME_SIZ+0*SZREG(sp)
	REG_S		t8, CALLFRAME_SIZ+1*SZREG(sp)
	mfc0		t0, MIPS_COP_0_STATUS
	REG_S		t0, CALLFRAME_SIZ+2*SZREG(sp)

	move		s0, sp			/* save sp */
#ifdef _LP64
	dsll32		t0, sp, 0		/* nuke upper half */
	dsrl32		t0, t0, 0		/*  "    "     "   */
	li		t1, MIPS_KSEG0_START
	or		sp, t0, t1		/* set MIPS_KSEG0_START */
#endif
	jalr		a3			/* callback to firmware */
	 nop
	move		sp, s0			/* restore sp */

	REG_L		t0, CALLFRAME_SIZ+2*SZREG(sp)
	mtc0		t0, MIPS_COP_0_STATUS
	REG_L		t8, CALLFRAME_SIZ+1*SZREG(sp)
	REG_L		gp, CALLFRAME_SIZ+0*SZREG(sp)
	REG_L		s0, CALLFRAME_S0(sp)
	REG_L		ra, CALLFRAME_RA(sp)
	jr		ra
	 PTR_ADDU	sp, sp, (CALLFRAME_SIZ+4*SZREG)
END(rmixlfw_wakeup_cpu)

/*
 * rmixl_cpu_trampoline - entry point for subordinate (non-#0) CPU wakeup
 */
NESTED(rmixl_cpu_trampoline, CALLFRAME_SIZ, ra)
#ifdef _LP64
	/*
	 * reconstruct trampoline args addr:
	 * sign-extend 32 bit KSEG0 address in a0
	 * to make proper 64 bit KSEG0 addr
	 */
	sll		s0, a0, 0
	li		t0, MIPS_SR_KX
#else
	li		t0, 0
#endif

	mtc0		zero, RMIXL_COP_0_EIMR	/* disable all in MIPS_COP_0_EIMR */

	mtc0		t0, MIPS_COP_0_STATUS

	/* ensure COP_0_EBASE field 'EBASE' is 0 */
	mfc0		t0, MIPS_COP_0_EBASE	/* MIPS_COP_0_EBASE */
	and		t0, t0, 0x3ff
	mtc0		t0, MIPS_COP_0_EBASE	/* MIPS_COP_0_EBASE */

	/*
	 * load our stack pointer from trampoline args
	 */
	REG_L		sp, 0*SZREG(s0)		/* XXX ta_sp */

	/*
	 * load our (idle) lwp from trampoline args
	 * save in t8 reg dedicated as 'mips_curlwp'
	 */
	REG_L		t8, 1*SZREG(s0)		/* XXX ta_lwp */

	/*
	 * load our ta_cpuinfo from trampoline args and pass in a1
	 * jump to common mips cpu_trampoline
	 */
	REG_L		a1, 2*SZREG(s0)		/* XXX ta_cpuinfo */
	dmtc0		a1, MIPS_COP_0_OSSCRATCH
	j		cpu_trampoline
	 nop

	/* NOTREACHED */

END(rmixl_cpu_trampoline)

#endif	/* MULTIPROCESSOR */
