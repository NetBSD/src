/*	$NetBSD: fpu.c,v 1.16 2004/04/16 23:58:08 matt Exp $	*/

/*
 * Copyright (C) 1996 Wolfgang Solfrank.
 * Copyright (C) 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.16 2004/04/16 23:58:08 matt Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/siginfo.h>

#include <machine/fpu.h>
#include <machine/psl.h>

void
enable_fpu(void)
{
	struct cpu_info *ci = curcpu();
	struct lwp *l = curlwp;
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct trapframe *tf = trapframe(l);
	int msr;

	KASSERT(pcb->pcb_fpcpu == NULL);
	if (!(pcb->pcb_flags & PCB_FPU)) {
		memset(&pcb->pcb_fpu, 0, sizeof pcb->pcb_fpu);
		pcb->pcb_flags |= PCB_FPU;
	}
	/*
	 * If we own the CPU but FP is disabled, simply enable it and return.
	 */
	if (ci->ci_fpulwp == l) {
		tf->srr1 |= PSL_FP | (pcb->pcb_flags & (PCB_FE0|PCB_FE1));
		return;
	}
	msr = mfmsr();
        mtmsr((msr & ~PSL_EE) | PSL_FP);
	__asm __volatile ("isync");
	if (ci->ci_fpulwp) {
		save_fpu_cpu();
	}
	KASSERT(ci->ci_fpulwp == NULL);
	__asm __volatile (
		"lfd	0,0(%0)\n"
		"mtfsf	0xff,0\n"
	    :: "b"(&pcb->pcb_fpu.fpscr));
	__asm (
		"lfd	0,0(%0)\n"
		"lfd	1,8(%0)\n"
		"lfd	2,16(%0)\n"
		"lfd	3,24(%0)\n"
		"lfd	4,32(%0)\n"
		"lfd	5,40(%0)\n"
		"lfd	6,48(%0)\n"
		"lfd	7,56(%0)\n"
		"lfd	8,64(%0)\n"
		"lfd	9,72(%0)\n"
		"lfd	10,80(%0)\n"
		"lfd	11,88(%0)\n"
		"lfd	12,96(%0)\n"
		"lfd	13,104(%0)\n"
		"lfd	14,112(%0)\n"
		"lfd	15,120(%0)\n"
		"lfd	16,128(%0)\n"
		"lfd	17,136(%0)\n"
		"lfd	18,144(%0)\n"
		"lfd	19,152(%0)\n"
		"lfd	20,160(%0)\n"
		"lfd	21,168(%0)\n"
		"lfd	22,176(%0)\n"
		"lfd	23,184(%0)\n"
		"lfd	24,192(%0)\n"
		"lfd	25,200(%0)\n"
		"lfd	26,208(%0)\n"
		"lfd	27,216(%0)\n"
		"lfd	28,224(%0)\n"
		"lfd	29,232(%0)\n"
		"lfd	30,240(%0)\n"
		"lfd	31,248(%0)\n"
	    :: "b"(&pcb->pcb_fpu.fpreg[0]));
	__asm __volatile ("isync");
	tf->srr1 |= PSL_FP | (pcb->pcb_flags & (PCB_FE0|PCB_FE1));
	ci->ci_fpulwp = l;
	pcb->pcb_fpcpu = ci;
	__asm __volatile ("sync");
	mtmsr(msr);
}

/*
 * Save the contents of the current CPU's FPU to its PCB.
 */
void
save_fpu_cpu(void)
{
	struct cpu_info *ci = curcpu();
	struct lwp *l;
	struct pcb *pcb;
	int msr;

	msr = mfmsr();
        mtmsr((msr & ~PSL_EE) | PSL_FP);
	__asm __volatile ("isync");
	l = ci->ci_fpulwp;
	if (l == NULL) {
		goto out;
	}
	pcb = &l->l_addr->u_pcb;
	__asm (
		"stfd	0,0(%0)\n"
		"stfd	1,8(%0)\n"
		"stfd	2,16(%0)\n"
		"stfd	3,24(%0)\n"
		"stfd	4,32(%0)\n"
		"stfd	5,40(%0)\n"
		"stfd	6,48(%0)\n"
		"stfd	7,56(%0)\n"
		"stfd	8,64(%0)\n"
		"stfd	9,72(%0)\n"
		"stfd	10,80(%0)\n"
		"stfd	11,88(%0)\n"
		"stfd	12,96(%0)\n"
		"stfd	13,104(%0)\n"
		"stfd	14,112(%0)\n"
		"stfd	15,120(%0)\n"
		"stfd	16,128(%0)\n"
		"stfd	17,136(%0)\n"
		"stfd	18,144(%0)\n"
		"stfd	19,152(%0)\n"
		"stfd	20,160(%0)\n"
		"stfd	21,168(%0)\n"
		"stfd	22,176(%0)\n"
		"stfd	23,184(%0)\n"
		"stfd	24,192(%0)\n"
		"stfd	25,200(%0)\n"
		"stfd	26,208(%0)\n"
		"stfd	27,216(%0)\n"
		"stfd	28,224(%0)\n"
		"stfd	29,232(%0)\n"
		"stfd	30,240(%0)\n"
		"stfd	31,248(%0)\n"
	    :: "b"(&pcb->pcb_fpu.fpreg[0]));
	__asm __volatile (
		"mffs	0\n"
		"stfd	0,0(%0)\n"
	    :: "b"(&pcb->pcb_fpu.fpscr));
	__asm __volatile ("sync");
	pcb->pcb_fpcpu = NULL;
	ci->ci_fpulwp = NULL;
	ci->ci_ev_fpusw.ev_count++;
	__asm __volatile ("sync");
 out:
	mtmsr(msr);
}

/*
 * Save a process's FPU state to its PCB.  The state may be in any CPU.
 * The process must either be curproc or traced by curproc (and stopped).
 * (The point being that the process must not run on another CPU during
 * this function).
 */
void
save_fpu_lwp(struct lwp *l, int discard)
{
	struct pcb * const pcb = &l->l_addr->u_pcb;
	struct cpu_info * const ci = curcpu();

	/*
	 * If it's already in the PCB, there's nothing to do.
	 */

	if (pcb->pcb_fpcpu == NULL)
		return;

	/*
	 * If we simply need to discard the information, then don't
	 * to save anything.
	 */
	if (discard) {
#ifndef MULTIPROCESSOR
		KASSERT(ci == pcb->pcb_fpcpu);
#endif
		KASSERT(l == pcb->pcb_fpcpu->ci_fpulwp);
		pcb->pcb_fpcpu->ci_fpulwp = NULL;
		pcb->pcb_fpcpu = NULL;
		pcb->pcb_flags &= ~PCB_FPU;
		return;
	}

	/*
	 * If the state is in the current CPU, just flush the current CPU's
	 * state.
	 */
	if (l == ci->ci_fpulwp) {
		save_fpu_cpu();
		return;
	}

#ifdef MULTIPROCESSOR
	/*
	 * It must be on another CPU, flush it from there.
	 */
	mp_save_fpu_lwp(l);
#endif
}

#define	STICKYBITS	(FPSCR_VX|FPSCR_OX|FPSCR_UX|FPSCR_ZX|FPSCR_XX)
#define	STICKYSHIFT	25
#define	MASKBITS	(FPSCR_VE|FPSCR_OE|FPSCR_UE|FPSCR_ZE|FPSCR_XE)
#define	MASKSHIFT	3

int
get_fpu_fault_code(void)
{
#ifdef DIAGNOSTIC
	struct cpu_info *ci = curcpu();
#endif
	struct pcb *pcb = curpcb;
	register_t msr;
	uint64_t tmp, fpscr64;
	uint32_t fpscr, ofpscr;
	int code;

	KASSERT(pcb->pcb_fpcpu == ci);
	KASSERT(pcb->pcb_flags & PCB_FPU);
	KASSERT(ci->ci_fpulwp == curlwp);
	msr = mfmsr();
        mtmsr((msr & ~PSL_EE) | PSL_FP);
	__asm __volatile ("isync");
	__asm __volatile (
		"stfd	0,0(%0)\n"	/* save f0 */
		"mffs	0\n"		/* get FPSCR */
		"stfd	0,0(%2)\n"	/* store a temp copy */
		"mtfsb0	0\n"		/* clear FPSCR_FX */
		"mtfsb0	24\n"		/* clear FPSCR_VE */
		"mtfsb0	25\n"		/* clear FPSCR_OE */
		"mtfsb0	26\n"		/* clear FPSCR_UE */
		"mtfsb0	27\n"		/* clear FPSCR_ZE */
		"mtfsb0	28\n"		/* clear FPSCR_XE */
		"mffs	0\n"		/* get FPSCR */
		"stfd	0,0(%1)\n"	/* store it */
		"lfd	0,0(%0)\n"	/* restore f0 */
	    :: "b"(&tmp), "b"(&pcb->pcb_fpu.fpscr), "b"(&fpscr64));
        mtmsr(msr);
	__asm __volatile ("isync");
	/*
	 * Now determine the fault type.  First we test to see if any of sticky
	 * bits correspond to the enabled exceptions.  If so, we only test
	 * those bits.  If not, we look at all the bits.  (In reality, we only
	 * could get an exception if FPSCR_FEX changed state.  So we should
	 * have at least one bit that corresponds).
	 */
	ofpscr = (uint32_t)fpscr64;
	ofpscr &= ofpscr << (STICKYSHIFT - MASKSHIFT);
	fpscr = (uint32_t)(*(uint64_t *)&pcb->pcb_fpu.fpscr);
	if (fpscr & ofpscr & STICKYBITS)
		fpscr &= ofpscr;

	/*
	 * Let's determine what the appropriate code is.
	 */
	if (fpscr & FPSCR_VX)		code = FPE_FLTINV;
	else if (fpscr & FPSCR_OX)	code = FPE_FLTOVF;
	else if (fpscr & FPSCR_UX)	code = FPE_FLTUND;
	else if (fpscr & FPSCR_ZX)	code = FPE_FLTDIV;
	else if (fpscr & FPSCR_XX)	code = FPE_FLTRES;
	else				code = 0;
	return code;
}
