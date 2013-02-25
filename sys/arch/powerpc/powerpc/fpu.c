/*	$NetBSD: fpu.c,v 1.31.12.1 2013/02/25 00:28:54 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.31.12.1 2013/02/25 00:28:54 tls Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/siginfo.h>
#include <sys/pcu.h>

#include <machine/pcb.h>
#include <machine/fpu.h>
#include <machine/psl.h>

#ifdef PPC_HAVE_FPU
static void fpu_state_load(lwp_t *, u_int);
static void fpu_state_save(lwp_t *, u_int);
static void fpu_state_release(lwp_t *, u_int);
#endif

const pcu_ops_t fpu_ops = {
	.pcu_id = PCU_FPU,
#ifdef PPC_HAVE_FPU
	.pcu_state_load = fpu_state_load,
	.pcu_state_save = fpu_state_save,
	.pcu_state_release = fpu_state_release,
#endif
};

bool
fpu_used_p(lwp_t *l)
{
	return (l->l_md.md_flags & MDLWP_USEDFPU) != 0;
}

void
fpu_mark_used(lwp_t *l)
{
	l->l_md.md_flags |= MDLWP_USEDFPU;
}

#ifdef PPC_HAVE_FPU
void
fpu_state_load(lwp_t *l, u_int flags)
{
	struct pcb * const pcb = lwp_getpcb(l);

	if (__predict_false(!fpu_used_p(l))) {
		memset(&pcb->pcb_fpu, 0, sizeof(pcb->pcb_fpu));
		fpu_mark_used(l);
	}

	const register_t msr = mfmsr();
        mtmsr((msr & ~PSL_EE) | PSL_FP);
	__asm volatile ("isync");

	fpu_load_from_fpreg(&pcb->pcb_fpu);
	__asm volatile ("sync");

	mtmsr(msr);
	__asm volatile ("isync");

	curcpu()->ci_ev_fpusw.ev_count++;
	l->l_md.md_utf->tf_srr1 |= PSL_FP|(pcb->pcb_flags & (PCB_FE0|PCB_FE1));
	l->l_md.md_flags |= MDLWP_USEDFPU;
}

/*
 * Save the contents of the current CPU's FPU to its PCB.
 */
void
fpu_state_save(lwp_t *l, u_int flags)
{
	struct pcb * const pcb = lwp_getpcb(l);

	const register_t msr = mfmsr();
        mtmsr((msr & ~PSL_EE) | PSL_FP);
	__asm volatile ("isync");

	fpu_unload_to_fpreg(&pcb->pcb_fpu);
	__asm volatile ("sync");

	mtmsr(msr);
	__asm volatile ("isync");
}

void
fpu_state_release(lwp_t *l, u_int flags)
{
	l->l_md.md_utf->tf_srr1 &= ~PSL_FP;
}

#define	STICKYBITS	(FPSCR_VX|FPSCR_OX|FPSCR_UX|FPSCR_ZX|FPSCR_XX)
#define	STICKYSHIFT	25
#define	MASKBITS	(FPSCR_VE|FPSCR_OE|FPSCR_UE|FPSCR_ZE|FPSCR_XE)
#define	MASKSHIFT	3

int
fpu_get_fault_code(void)
{
	lwp_t * const l = curlwp;
	struct pcb * const pcb = lwp_getpcb(l);
	uint64_t fpscr64;
	uint32_t fpscr, ofpscr;
	int code;

	int s = splsoftclock();	/* disable preemption */

	struct cpu_info * const ci = curcpu();
	/*
	 * If we got preempted, we may be running on a different CPU.  So we
	 * need to check for that.
	 */
	KASSERT(fpu_used_p(l));
	if (__predict_true(l->l_pcu_cpu[PCU_FPU] == ci)) {
		uint64_t tmp;
		const register_t msr = mfmsr();
		mtmsr((msr & ~PSL_EE) | PSL_FP);
		__asm volatile ("isync");
		__asm volatile (
			"stfd	0,0(%[tmp])\n"		/* save f0 */
			"mffs	0\n"			/* get FPSCR */
			"stfd	0,0(%[fpscr64])\n"	/* store a temp copy */
			"mtfsb0	0\n"			/* clear FPSCR_FX */
			"mtfsb0	24\n"			/* clear FPSCR_VE */
			"mtfsb0	25\n"			/* clear FPSCR_OE */
			"mtfsb0	26\n"			/* clear FPSCR_UE */
			"mtfsb0	27\n"			/* clear FPSCR_ZE */
			"mtfsb0	28\n"			/* clear FPSCR_XE */
			"mffs	0\n"			/* get FPSCR */
			"stfd	0,0(%[fpscr])\n"	/* store it */
			"lfd	0,0(%[tmp])\n"		/* restore f0 */
		    ::	[tmp] "b"(&tmp),
			[fpscr] "b"(&pcb->pcb_fpu.fpscr),
			[fpscr64] "b"(&fpscr64));
		mtmsr(msr);
		__asm volatile ("isync");
	} else {
		/*
		 * We got preempted to a different CPU so we need to save
		 * our FPU state.
		 */
		fpu_save();
		fpscr64 = *(uint64_t *)&pcb->pcb_fpu.fpscr;
		((uint32_t *)&pcb->pcb_fpu.fpscr)[_QUAD_LOWWORD] &= ~MASKBITS;
	}

	splx(s);	/* allow preemption */

	/*
	 * Now determine the fault type.  First we test to see if any of sticky
	 * bits correspond to the enabled exceptions.  If so, we only test
	 * those bits.  If not, we look at all the bits.  (In reality, we only
	 * could get an exception if FPSCR_FEX changed state.  So we should
	 * have at least one bit that corresponds).
	 */
	ofpscr = (uint32_t)fpscr64;
	ofpscr &= ofpscr << (STICKYSHIFT - MASKSHIFT);
	fpscr = ((uint32_t *)&pcb->pcb_fpu.fpscr)[_QUAD_LOWWORD];
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
#endif /* PPC_HAVE_FPU */

bool
fpu_save_to_mcontext(lwp_t *l, mcontext_t *mcp, unsigned int *flagp)
{
	KASSERT(l == curlwp);

	if (!pcu_used_p(&fpu_ops))
		return false;

	struct pcb * const pcb = lwp_getpcb(l);

#ifdef PPC_HAVE_FPU
	/* If we're the FPU owner, dump its context to the PCB first. */
	pcu_save(&fpu_ops);
#endif
	(void)memcpy(mcp->__fpregs.__fpu_regs, pcb->pcb_fpu.fpreg,
	    sizeof (mcp->__fpregs.__fpu_regs));
	mcp->__fpregs.__fpu_fpscr =
	    ((int *)&pcb->pcb_fpu.fpscr)[_QUAD_LOWWORD];
	mcp->__fpregs.__fpu_valid = 1;
	*flagp |= _UC_FPU;
	return true;
}

void
fpu_restore_from_mcontext(lwp_t *l, const mcontext_t *mcp)
{
	if (!mcp->__fpregs.__fpu_valid)
		return;

	struct pcb * const pcb = lwp_getpcb(l);

#ifdef PPC_HAVE_FPU
	/* we don't need to save the state, just drop it */
	if (l == curlwp)
		pcu_discard(&fpu_ops);
#endif
	(void)memcpy(&pcb->pcb_fpu.fpreg, &mcp->__fpregs.__fpu_regs,
	    sizeof (pcb->pcb_fpu.fpreg));
	((int *)&pcb->pcb_fpu.fpscr)[_QUAD_LOWWORD] = mcp->__fpregs.__fpu_fpscr;
}
