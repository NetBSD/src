/*	$NetBSD: kgdb_machdep.c,v 1.5 2003/01/22 21:44:56 kleink Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Allen Briggs for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ddb.h"

#if defined(DDB)
#error "Can't build DDB and KGDB together."
#endif

/*
 * Machine-dependent functions for remote KGDB.
 */

#include <sys/param.h>
#include <sys/kgdb.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#ifdef PPC_MPC6XX
#include <powerpc/mpc6xx/bat.h>
#else
#include <machine/bat.h>
#endif
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/pmap.h>

/*
 * Determine if the memory at va..(va+len) is valid.
 */
int
kgdb_acc(vaddr_t va, size_t len)
{
	vaddr_t   last_va;
	paddr_t   pa;
	u_int msr;
	u_int batu, batl;

	/* If translation is off, everything is fair game */
	asm volatile ("mfmsr %0" : "=r"(msr));
	if ((msr & PSL_DR) == 0) {
		return 1;
	}

	/* Now check battable registers */
	if ((mfpvr() >> 16) == MPC601) {
		asm volatile ("mfibatl %0,0" : "=r"(batl));
		asm volatile ("mfibatu %0,0" : "=r"(batu));
		if (BAT601_VALID_P(batl) &&
				BAT601_VA_MATCH_P(batu,batl,va))
			return 1;
		asm volatile ("mfibatl %0,1" : "=r"(batl));
		asm volatile ("mfibatu %0,1" : "=r"(batu));
		if (BAT601_VALID_P(batl) &&
				BAT601_VA_MATCH_P(batu,batl,va))
			return 1;
		asm volatile ("mfibatl %0,2" : "=r"(batl));
		asm volatile ("mfibatu %0,2" : "=r"(batu));
		if (BAT601_VALID_P(batl) &&
				BAT601_VA_MATCH_P(batu,batl,va))
			return 1;
		asm volatile ("mfibatl %0,3" : "=r"(batl));
		asm volatile ("mfibatu %0,3" : "=r"(batu));
		if (BAT601_VALID_P(batl) &&
				BAT601_VA_MATCH_P(batu,batl,va))
			return 1;
	} else {
		asm volatile ("mfdbatu %0,0" : "=r"(batu));
		if (BAT_VALID_P(batu,msr) &&
				BAT_VA_MATCH_P(batu,va) &&
				(batu & BAT_PP) != BAT_PP_NONE) {
			return 1;
		}
		asm volatile ("mfdbatu %0,1" : "=r"(batu));
		if (BAT_VALID_P(batu,msr) &&
				BAT_VA_MATCH_P(batu,va) &&
				(batu & BAT_PP) != BAT_PP_NONE) {
			return 1;
		}
		asm volatile ("mfdbatu %0,2" : "=r"(batu));
		if (BAT_VALID_P(batu,msr) &&
				BAT_VA_MATCH_P(batu,va) &&
				(batu & BAT_PP) != BAT_PP_NONE) {
			return 1;
		}
		asm volatile ("mfdbatu %0,3" : "=r"(batu));
		if (BAT_VALID_P(batu,msr) &&
				BAT_VA_MATCH_P(batu,va) &&
				(batu & BAT_PP) != BAT_PP_NONE) {
			return 1;
		}
	}

	last_va = va + len;
	va  &= ~PGOFSET;
	last_va &= ~PGOFSET;

	do {
		/*
		 * I think this should be able to handle
		 * non-pmap_kernel() va's, too.
		 */
		if (!pmap_extract(pmap_kernel(), va, &pa))
			return 0;
		va += PAGE_SIZE;
	} while (va <= last_va);

	return (1);
}

/*
 * Translate a trap number into a unix compatible signal value.
 * (gdb only understands unix signal numbers).  Some of these are bogus
 * and should be reviewed.
 */
int 
kgdb_signal(type)
	int type;
{
	switch (type) {
#ifdef PPC_IBM4XX
	case EXC_PIT:		/* 40x - Programmable interval timer */
	case EXC_FIT:		/* 40x - Fixed interval timer */
		return SIGALRM

	case EXC_CII:		/* 40x - Critical input interrupt */
	case EXC_WDOG:		/* 40x - Watchdog timer */
	case EXC_DEBUG:		/* 40x - Debug trap */
		return SIGTRAP;

	case EXC_DTMISS:	/* 40x - Instruction TLB miss */
	case EXC_ITMISS:	/* 40x - Data TLB miss */
		return SIGSEGV;
#endif

#ifdef PPC_MPC6XX
	case EXC_PERF:		/* 604/750/7400 - Performance monitoring */
	case EXC_BPT:		/* 604/750/7400 - Instruction breakpoint */
	case EXC_SMI:		/* 604/750/7400 - System management interrupt */
	case EXC_THRM:		/* 750/7400 - Thermal management interrupt */
		return SIGTRAP;

	case EXC_IMISS:		/* 603 - Instruction translation miss */
	case EXC_DLMISS:	/* 603 - Data load translation miss */
	case EXC_DSMISS:	/* 603 - Data store translation miss */
		return SIGSEGV;

	case EXC_RST:		/* All but IBM 4xx - Reset */
		return SIGURG;

	case EXC_VEC:		/* 7400 - Altivec unavailable */
	case EXC_VECAST:	/* 7400 - Altivec assist */
		return SIGFPE;
#endif

	case EXC_DECR:		/* Decrementer interrupt */
		return SIGALRM;

	case EXC_EXI:		/* External interrupt */
		return SIGINT;

	case EXC_PGM:		/* Program interrupt */
	case EXC_ALI:		/* Alignment */
		return SIGILL;

  case T_BREAKPOINT:
	case EXC_MCHK:		/* Machine check */
	case EXC_TRC:		/* Trace */
		return SIGTRAP;

	case EXC_ISI:		/* Instruction storage interrupt */
	case EXC_DSI:		/* Data storage interrupt */
		return SIGSEGV;

	case EXC_FPU:		/* Floating point unavailable */
	case EXC_FPA:		/* Floating-point assist */
		return SIGFPE;

	case EXC_SC:		/* System call */
		return SIGURG;

	case EXC_RSVD:		/* Reserved */
	case EXC_AST:		/* Floating point unavailable */
	default:
		return SIGEMT;
	}
}

/*
 * Translate the values stored in the kernel regs struct to the format
 * understood by gdb.
 */
void
kgdb_getregs(regs, gdb_regs)
	db_regs_t *regs;
	kgdb_reg_t *gdb_regs;
{
	memcpy(gdb_regs, regs, 32 * sizeof(unsigned long));
	gdb_regs[KGDB_PPC_PC_REG]  = regs->iar;
	gdb_regs[KGDB_PPC_MSR_REG] = regs->msr;
	gdb_regs[KGDB_PPC_CR_REG]  = regs->cr;
	gdb_regs[KGDB_PPC_LR_REG]  = regs->lr;
	gdb_regs[KGDB_PPC_CTR_REG] = regs->ctr;
	gdb_regs[KGDB_PPC_XER_REG] = regs->xer;
}

/*
 * Reverse the above.
 */
void
kgdb_setregs(regs, gdb_regs)
	db_regs_t *regs;
	kgdb_reg_t *gdb_regs;
{
	regs->xer = gdb_regs[KGDB_PPC_XER_REG];
	regs->ctr = gdb_regs[KGDB_PPC_CTR_REG];
	regs->lr  = gdb_regs[KGDB_PPC_LR_REG];
	regs->cr  = gdb_regs[KGDB_PPC_CR_REG];
	regs->msr = gdb_regs[KGDB_PPC_MSR_REG];
	regs->iar = gdb_regs[KGDB_PPC_PC_REG];
	memcpy(regs, gdb_regs, 32 * sizeof(unsigned long));
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

	asm volatile(BKPT_ASM);

	if (verbose && kgdb_active) {
		printf("kgdb connected.\n");
	}

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
