/*	$NetBSD: fpu.c,v 1.25 2011/01/18 01:02:55 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.25 2011/01/18 01:02:55 matt Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/siginfo.h>

//#include <uvm/uvm_extern.h>

#include <machine/pcb.h>
#include <machine/fpu.h>
#include <machine/psl.h>

#ifdef MULTIPROCESSOR
#include <arch/powerpc/pic/picvar.h>
#include <arch/powerpc/pic/ipivar.h>
static void fpu_mp_save_lwp(struct lwp *);
#endif

void
fpu_enable(void)
{
	struct cpu_info * const ci = curcpu();
	struct lwp * const l = curlwp;
	struct pcb * const pcb = lwp_getpcb(l);
	struct trapframe * const tf = l->l_md.md_utf;

	if (!(l->l_md.md_flags & MDLWP_USEDFPU)) {
		memset(&pcb->pcb_fpu, 0, sizeof pcb->pcb_fpu);
		l->l_md.md_flags |= MDLWP_USEDFPU;
	}

	const register_t msr = mfmsr();
        mtmsr((msr & ~PSL_EE) | PSL_FP);
	__asm volatile ("isync");

	if (ci->ci_fpulwp != l) {
		fpu_save_cpu(FPU_SAVE_AND_RELEASE);

		fpu_load_from_fpreg(&pcb->pcb_fpu);

		__asm volatile ("isync");

		ci->ci_fpulwp = l;
		l->l_md.md_fpucpu = ci;
		ci->ci_ev_fpusw.ev_count++;
	}

	tf->tf_srr1 |= PSL_FP | (pcb->pcb_flags & (PCB_FE0|PCB_FE1));
	l->l_md.md_flags |= MDLWP_OWNFPU;
	__asm volatile ("sync");
	mtmsr(msr);
}

/*
 * Save the contents of the current CPU's FPU to its PCB.
 */
void
fpu_save_cpu(enum fpu_op op)
{
	const register_t msr = mfmsr();
        mtmsr((msr & ~PSL_EE) | PSL_FP);
	__asm volatile ("isync");

	struct cpu_info * const ci = curcpu();
	lwp_t * const l = ci->ci_fpulwp;

	if (l->l_md.md_flags & MDLWP_OWNFPU) {
		struct pcb * const pcb = lwp_getpcb(l);

		fpu_unload_to_fpreg(&pcb->pcb_fpu);

		__asm volatile ("sync");

		if (op == FPU_SAVE_AND_RELEASE)
			ci->ci_fpulwp = ci->ci_data.cpu_idlelwp;
		__asm volatile ("sync");
	}
	mtmsr(msr);
}

#ifdef MULTIPROCESSOR

/*
 * Save a process's FPU state to its PCB.  The state is in another CPU
 * (though by the time our IPI is processed, it may have been flushed already).
 */
static void
mp_save_fpu_lwp(struct lwp *l)
{
	/*
	 * Send an IPI to the other CPU with the data and wait for that CP		 * to flush the data.  Note that the other CPU might have switched
	 * to a different proc's FPU state by the time it receives the IPI,
	 * but that will only result in an unnecessary reload.
	 */

	struct cpu_info *fpucpu;
	fpucpu = l->l_md.md_fpucpu;
	if (fpucpu == NULL)
		return;

	ppc_send_ipi(fpucpu->ci_index, PPC_IPI_FLUSH_FPU);

	/* Wait for flush. */
	for (u_int i = 0; i < 0x3fffffff; i++) {
		if (l->l_md.md_flags & MDLWP_OWNFPU) == 0)
			return;
	}

	aprint_error("mp_save_fpu_proc{%d} pid = %d.%d, fpucpu->ci_cpuid = %d\n",
	    cpu_number(), l->l_proc->p_pid, l->l_lid, fpucpu->ci_cpuid);
	panic("mp_save_fpu_proc: timed out");
}
#endif /* MULTIPROCESSOR */

/*
 * Save a process's FPU state to its PCB.  The state may be in any CPU.
 * The process must either be curproc or traced by curproc (and stopped).
 * (The point being that the process must not run on another CPU during
 * this function).
 */
void
fpu_save_lwp(struct lwp *l, enum fpu_op op)
{
	struct cpu_info * const ci = curcpu();

	KASSERT(l->l_md.md_fpucpu != NULL);

	/*
	 * If it's already in the PCB, there's nothing to do.
	 */
	if ((l->l_md.md_flags & MDLWP_OWNFPU) == 0)
		return;

	/*
	 * If we simply need to discard the information, then don't
	 * to save anything.
	 */
	if (op == FPU_DISCARD) {
#ifndef MULTIPROCESSOR
		KASSERT(ci == l->l_md.md_fpucpu);
#endif
		KASSERT(l == l->l_md.md_fpucpu->ci_fpulwp);
		atomic_cas_ptr(&l->l_md.md_fpucpu->ci_fpulwp, l,
		   l->l_md.md_fpucpu->ci_data.cpu_idlelwp);
		atomic_and_uint(&l->l_md.md_flags, ~MDLWP_OWNFPU);
		return;
	}

	/*
	 * If the state is in the current CPU, just flush the current CPU's
	 * state.
	 */
	if (l == ci->ci_fpulwp) {
		fpu_save_cpu(op);
		return;
	}

#ifdef MULTIPROCESSOR
	/*
	 * It must be on another CPU, flush it from there.
	 */
	fpu_mp_save_lwp(l);
#endif
}

bool
fpu_save_to_mcontext(lwp_t *l, mcontext_t *mcp, unsigned int *flagp)
{
	if ((l->l_md.md_flags & MDLWP_USEDFPU) != 0)
		return false;

	struct pcb * const pcb = lwp_getpcb(l);

	/* If we're the FPU owner, dump its context to the PCB first. */
	fpu_save_lwp(l, FPU_SAVE);
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

	/* we don't need to save the state, just drop it */
	fpu_save_lwp(l, FPU_DISCARD);
	(void)memcpy(&pcb->pcb_fpu.fpreg, &mcp->__fpregs.__fpu_regs,
	    sizeof (pcb->pcb_fpu.fpreg));
	((int *)&pcb->pcb_fpu.fpscr)[_QUAD_LOWWORD] = mcp->__fpregs.__fpu_fpscr;
}

#define	STICKYBITS	(FPSCR_VX|FPSCR_OX|FPSCR_UX|FPSCR_ZX|FPSCR_XX)
#define	STICKYSHIFT	25
#define	MASKBITS	(FPSCR_VE|FPSCR_OE|FPSCR_UE|FPSCR_ZE|FPSCR_XE)
#define	MASKSHIFT	3

int
fpu_get_fault_code(void)
{
#ifdef DIAGNOSTIC
	struct cpu_info * const ci = curcpu();
#endif
	struct pcb *pcb = curpcb;
	lwp_t *l = curlwp;
	register_t msr;
	uint64_t tmp, fpscr64;
	uint32_t fpscr, ofpscr;
	int code;

	KASSERT(l->l_md.md_fpucpu == ci);
	KASSERT(l->l_md.md_flags & MDLWP_USEDFPU);
	KASSERT(l->l_md.md_flags & MDLWP_OWNFPU);
	KASSERT(ci->ci_fpulwp == l);
	msr = mfmsr();
        mtmsr((msr & ~PSL_EE) | PSL_FP);
	__asm volatile ("isync");
	__asm volatile (
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
	__asm volatile ("isync");
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
