/*	$NetBSD: fpu.c,v 1.24 2017/11/04 08:58:30 maxv Exp $	*/

/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.  All
 * rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Andrew Doran.
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
 * Copyright (c) 1991 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)npx.c	7.2 (Berkeley) 5/12/91
 */

/*
 * Copyright (c) 1994, 1995, 1998 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1990 William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)npx.c	7.2 (Berkeley) 5/12/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.24 2017/11/04 08:58:30 maxv Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/kernel.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/cpufunc.h>
#include <machine/pcb.h>
#include <machine/trap.h>
#include <machine/specialreg.h>
#include <x86/cpu.h>
#include <x86/fpu.h>

/* Check some duplicate definitions match */
#include <machine/fenv.h>

#ifdef XEN
#define clts() HYPERVISOR_fpu_taskswitch(0)
#define stts() HYPERVISOR_fpu_taskswitch(1)
#endif

static uint32_t x86_fpu_mxcsr_mask __read_mostly = 0;

static inline union savefpu *
process_fpframe(struct lwp *lwp)
{
	struct pcb *pcb = lwp_getpcb(lwp);

	return &pcb->pcb_savefpu;
}

/* 
 * The following table is used to ensure that the FPE_... value
 * that is passed as a trapcode to the signal handler of the user
 * process does not have more than one bit set.
 * 
 * Multiple bits may be set if SSE simd instructions generate errors
 * on more than one value or if the user process modifies the control
 * word while a status word bit is already set (which this is a sign
 * of bad coding).
 * We have no choise than to narrow them down to one bit, since we must
 * not send a trapcode that is not exactly one of the FPE_ macros.
 *
 * The mechanism has a static table with 127 entries.  Each combination
 * of the 7 FPU status word exception bits directly translates to a
 * position in this table, where a single FPE_... value is stored.
 * This FPE_... value stored there is considered the "most important"
 * of the exception bits and will be sent as the signal code.  The
 * precedence of the bits is based upon Intel Document "Numerical
 * Applications", Chapter "Special Computational Situations".
 *
 * The code to choose one of these values does these steps:
 * 1) Throw away status word bits that cannot be masked.
 * 2) Throw away the bits currently masked in the control word,
 *    assuming the user isn't interested in them anymore.
 * 3) Reinsert status word bit 7 (stack fault) if it is set, which
 *    cannot be masked but must be presered.
 *    'Stack fault' is a sub-class of 'invalid operation'.
 * 4) Use the remaining bits to point into the trapcode table.
 *
 * The 6 maskable bits in order of their preference, as stated in the
 * above referenced Intel manual:
 * 1  Invalid operation (FP_X_INV)
 * 1a   Stack underflow
 * 1b   Stack overflow
 * 1c   Operand of unsupported format
 * 1d   SNaN operand.
 * 2  QNaN operand (not an exception, irrelavant here)
 * 3  Any other invalid-operation not mentioned above or zero divide
 *      (FP_X_INV, FP_X_DZ)
 * 4  Denormal operand (FP_X_DNML)
 * 5  Numeric over/underflow (FP_X_OFL, FP_X_UFL)
 * 6  Inexact result (FP_X_IMP) 
 *
 * NB: the above seems to mix up the mxscr error bits and the x87 ones.
 * They are in the same order, but there is no EN_SW_STACK_FAULT in the mmx
 * status.
 *
 * The table is nearly, but not quite, in bit order (ZERODIV and DENORM
 * are swapped).
 *
 * This table assumes that any stack fault is cleared - so that an INVOP
 * fault will only be reported as FLTSUB once.
 * This might not happen if the mask is being changed.
 */
#define FPE_xxx1(f) (f & EN_SW_INVOP \
		? (f & EN_SW_STACK_FAULT ? FPE_FLTSUB : FPE_FLTINV) \
	: f & EN_SW_ZERODIV ? FPE_FLTDIV \
	: f & EN_SW_DENORM ? FPE_FLTUND \
	: f & EN_SW_OVERFLOW ? FPE_FLTOVF \
	: f & EN_SW_UNDERFLOW ? FPE_FLTUND \
	: f & EN_SW_PRECLOSS ? FPE_FLTRES \
	: f & EN_SW_STACK_FAULT ? FPE_FLTSUB : 0)
#define	FPE_xxx2(f)	FPE_xxx1(f),	FPE_xxx1((f + 1))
#define	FPE_xxx4(f)	FPE_xxx2(f),	FPE_xxx2((f + 2))
#define	FPE_xxx8(f)	FPE_xxx4(f),	FPE_xxx4((f + 4))
#define	FPE_xxx16(f)	FPE_xxx8(f),	FPE_xxx8((f + 8))
#define	FPE_xxx32(f)	FPE_xxx16(f),	FPE_xxx16((f + 16))
static const uint8_t fpetable[128] = {
	FPE_xxx32(0), FPE_xxx32(32), FPE_xxx32(64), FPE_xxx32(96)
};
#undef FPE_xxx1
#undef FPE_xxx2
#undef FPE_xxx4
#undef FPE_xxx8
#undef FPE_xxx16
#undef FPE_xxx32

/*
 * Init the FPU.
 *
 * This might not be strictly necessary since it will be initialised
 * for each process.  However it does no harm.
 */
void
fpuinit(struct cpu_info *ci)
{

	clts();
	fninit();
	stts();
}

/*
 * Get the value of MXCSR_MASK supported by the CPU.
 */
void
fpuinit_mxcsr_mask(void)
{
#ifndef XEN
	union savefpu fpusave __aligned(16);
	u_long cr0, psl;

	memset(&fpusave, 0, sizeof(fpusave));

	/* Disable interrupts, and enable FPU */
	psl = x86_read_psl();
	x86_disable_intr();
	cr0 = rcr0();
	lcr0(cr0 & ~(CR0_EM|CR0_TS));

	/* Fill in the FPU area */
	fxsave(&fpusave);

	/* Restore previous state */
	lcr0(cr0);
	x86_write_psl(psl);

	if (fpusave.sv_xmm.fx_mxcsr_mask == 0) {
		x86_fpu_mxcsr_mask = __INITIAL_MXCSR_MASK__;
	} else {
		x86_fpu_mxcsr_mask = fpusave.sv_xmm.fx_mxcsr_mask;
	}
#else
	/*
	 * XXX: Does the detection above work on Xen?
	 */
	x86_fpu_mxcsr_mask = __INITIAL_MXCSR_MASK__;
#endif
}

/*
 * This is a synchronous trap on either an x87 instruction (due to an
 * unmasked error on the previous x87 instruction) or on an SSE/SSE2 etc
 * instruction due to an error on the instruction itself.
 *
 * If trap actually generates a signal, then the fpu state is saved
 * and then copied onto the process's user-stack, and then recovered
 * from there when the signal returns (or from the jmp_buf if the
 * signal handler exits with a longjmp()).
 *
 * All this code need to do is save the reason for the trap.
 * For x87 interrupts the status word bits need clearing to stop the
 * trap re-occurring.
 *
 * The mxcsr bits are 'sticky' and need clearing to not confuse a later trap.
 *
 * Since this is a synchronous trap, the fpu registers must still belong
 * to the correct process (we trap through an interrupt gate so that
 * interrupts are disabled on entry).
 * Interrupts (these better include IPIs) are left disabled until we've
 * finished looking at fpu registers.
 *
 * For amd64 the calling code (in amd64_trap.S) has already checked
 * that we trapped from usermode.
 */

void
fputrap(struct trapframe *frame)
{
	uint32_t statbits;
	ksiginfo_t ksi;

	if (!USERMODE(frame->tf_cs))
		panic("fpu trap from kernel, trapframe %p\n", frame);

	/*
	 * At this point, fpcurlwp should be curlwp.  If it wasn't, the TS bit
	 * should be set, and we should have gotten a DNA exception.
	 */
	KASSERT(curcpu()->ci_fpcurlwp == curlwp);

	if (frame->tf_trapno == T_XMM) {
		uint32_t mxcsr;
		x86_stmxcsr(&mxcsr);
		statbits = mxcsr;
		/* Clear the sticky status bits */
		mxcsr &= ~0x3f;
		x86_ldmxcsr(&mxcsr);

		/* Remove masked interrupts and non-status bits */
		statbits &= ~(statbits >> 7) & 0x3f;
		/* Mark this is an XMM status */
		statbits |= 0x10000;
	} else {
		uint16_t cw, sw;
		/* Get current control and status words */
		fnstcw(&cw);
		fnstsw(&sw);
		/* Clear any pending exceptions from status word */
		fnclex();

		/* Removed masked interrupts */
		statbits = sw & ~(cw & 0x3f);
	}

	/* Doesn't matter now if we get pre-empted */
	x86_enable_intr();

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGFPE;
	ksi.ksi_addr = (void *)X86_TF_RIP(frame);
	ksi.ksi_code = fpetable[statbits & 0x7f];
	ksi.ksi_trap = statbits;
	(*curlwp->l_proc->p_emul->e_trapsignal)(curlwp, &ksi);
}

/*
 * Implement device not available (DNA) exception
 *
 * If we were the last lwp to use the FPU, we can simply return.
 * Otherwise, we save the previous state, if necessary, and restore
 * our last saved state.
 *
 * Called directly from the trap 0x13 entry with interrupts still disabled.
 */
void
fpudna(struct trapframe *frame)
{
	struct cpu_info *ci;
	struct lwp *l, *fl;
	struct pcb *pcb;
	int s;

	if (!USERMODE(frame->tf_cs))
		panic("fpudna from kernel, ip %p, trapframe %p\n",
		    (void *)X86_TF_RIP(frame), frame);

	ci = curcpu();

	/* Save soft spl level - interrupts are hard disabled */
	s = splhigh();

	/* Save state on current CPU. */
	l = ci->ci_curlwp;
	pcb = lwp_getpcb(l);
	fl = ci->ci_fpcurlwp;
	if (fl != NULL) {
		/*
		 * It seems we can get here on Xen even if we didn't
		 * switch lwp.  In this case do nothing
		 */
		if (fl == l) {
			KASSERT(pcb->pcb_fpcpu == ci);
			clts();
			splx(s);
			return;
		}
		fpusave_cpu(true);
	}

	/* Save our state if on a remote CPU. */
	if (pcb->pcb_fpcpu != NULL) {
		/* Explicitly disable preemption before dropping spl. */
		kpreempt_disable();
		splx(s);

		/* Actually enable interrupts */
		x86_enable_intr();

		fpusave_lwp(l, true);
		KASSERT(pcb->pcb_fpcpu == NULL);
		s = splhigh();
		kpreempt_enable();
	}

	/*
	 * Restore state on this CPU, or initialize.  Ensure that
	 * the entire update is atomic with respect to FPU-sync IPIs.
	 */
	clts();
	ci->ci_fpcurlwp = l;
	pcb->pcb_fpcpu = ci;

	switch (x86_fpu_save) {
		case FPU_SAVE_FSAVE:
			frstor(&pcb->pcb_savefpu);
			break;

		case FPU_SAVE_FXSAVE:
			/*
			 * AMD FPU's do not restore FIP, FDP, and FOP on
			 * fxrstor, leaking other process's execution history.
			 * Clear them manually by loading a zero.
			 *
			 * Clear the ES bit in the x87 status word if it is
			 * currently set, in order to avoid causing a fault
			 * in the upcoming load.
			 */
			if (fngetsw() & 0x80)
				fnclex();
			fldummy();
			fxrstor(&pcb->pcb_savefpu);
			break;

		case FPU_SAVE_XSAVE:
		case FPU_SAVE_XSAVEOPT:
			xrstor(&pcb->pcb_savefpu, x86_xsave_features);
			break;
	}

	KASSERT(ci == curcpu());
	splx(s);
}

/*
 * Save current CPU's FPU state.  Must be called at IPL_HIGH.
 */
void
fpusave_cpu(bool save)
{
	struct cpu_info *ci;
	struct pcb *pcb;
	struct lwp *l;

	KASSERT(curcpu()->ci_ilevel == IPL_HIGH);

	ci = curcpu();
	l = ci->ci_fpcurlwp;
	if (l == NULL) {
		return;
	}
	pcb = lwp_getpcb(l);

	if (save) {
		clts();

		switch (x86_fpu_save) {
			case FPU_SAVE_FSAVE:
				fnsave(&pcb->pcb_savefpu);
				break;

			case FPU_SAVE_FXSAVE:
				fxsave(&pcb->pcb_savefpu);
				break;

			case FPU_SAVE_XSAVE:
				xsave(&pcb->pcb_savefpu, x86_xsave_features);
				break;

			case FPU_SAVE_XSAVEOPT:
				xsaveopt(&pcb->pcb_savefpu, x86_xsave_features);
				break;
		}
	}

	stts();
	pcb->pcb_fpcpu = NULL;
	ci->ci_fpcurlwp = NULL;
}

/*
 * Save l's FPU state, which may be on this processor or another processor.
 * It may take some time, so we avoid disabling preemption where possible.
 * Caller must know that the target LWP is stopped, otherwise this routine
 * may race against it.
 */
void
fpusave_lwp(struct lwp *l, bool save)
{
	struct pcb *pcb = lwp_getpcb(l);
	struct cpu_info *oci;
	int s, spins, ticks;

	spins = 0;
	ticks = hardclock_ticks;
	for (;;) {
		s = splhigh();
		oci = pcb->pcb_fpcpu;
		if (oci == NULL) {
			splx(s);
			break;
		}
		if (oci == curcpu()) {
			KASSERT(oci->ci_fpcurlwp == l);
			fpusave_cpu(save);
			splx(s);
			break;
		}
		splx(s);
#ifdef XEN
		if (xen_send_ipi(oci, XEN_IPI_SYNCH_FPU) != 0) {
			panic("xen_send_ipi(%s, XEN_IPI_SYNCH_FPU) failed.",
			    cpu_name(oci));
		}
#else
		x86_send_ipi(oci, X86_IPI_SYNCH_FPU);
#endif
		while (pcb->pcb_fpcpu == oci && ticks == hardclock_ticks) {
			x86_pause();
			spins++;
		}
		if (spins > 100000000) {
			panic("fpusave_lwp: did not");
		}
	}
}

void
fpu_set_default_cw(struct lwp *l, unsigned int x87_cw)
{
	union savefpu *fpu_save = process_fpframe(l);
	struct pcb *pcb = lwp_getpcb(l);

	if (i386_use_fxsave)
		fpu_save->sv_xmm.fx_cw = x87_cw;
	else
		fpu_save->sv_87.s87_cw = x87_cw;
	pcb->pcb_fpu_dflt_cw = x87_cw;
}

void
fpu_save_area_clear(struct lwp *l, unsigned int x87_cw)
{
	union savefpu *fpu_save;
	struct pcb *pcb;

	fpusave_lwp(l, false);
	fpu_save = process_fpframe(l);
	pcb = lwp_getpcb(l);

	if (i386_use_fxsave) {
		memset(&fpu_save->sv_xmm, 0, x86_fpu_save_size);
		fpu_save->sv_xmm.fx_mxcsr = __INITIAL_MXCSR__;
		fpu_save->sv_xmm.fx_mxcsr_mask = x86_fpu_mxcsr_mask;
		fpu_save->sv_xmm.fx_cw = x87_cw;

		/* Force a reload of CW */
		if ((x87_cw != __INITIAL_NPXCW__) &&
		    (x86_fpu_save == FPU_SAVE_XSAVE ||
		    x86_fpu_save == FPU_SAVE_XSAVEOPT)) {
			fpu_save->sv_xsave_hdr.xsh_xstate_bv |=
			    XCR0_X87;
		}
	} else {
		memset(&fpu_save->sv_87, 0, x86_fpu_save_size);
		fpu_save->sv_87.s87_tw = 0xffff;
		fpu_save->sv_87.s87_cw = x87_cw;
	}
	pcb->pcb_fpu_dflt_cw = x87_cw;
}

void
fpu_save_area_reset(struct lwp *l)
{
	union savefpu *fpu_save = process_fpframe(l);
	struct pcb *pcb = lwp_getpcb(l);

	/*
	 * For signal handlers the register values don't matter. Just reset
	 * a few fields.
	 */
	if (i386_use_fxsave) {
		fpu_save->sv_xmm.fx_mxcsr = __INITIAL_MXCSR__;
		fpu_save->sv_xmm.fx_mxcsr_mask = x86_fpu_mxcsr_mask;
		fpu_save->sv_xmm.fx_tw = 0;
		fpu_save->sv_xmm.fx_cw = pcb->pcb_fpu_dflt_cw;
	} else {
		fpu_save->sv_87.s87_tw = 0xffff;
		fpu_save->sv_87.s87_cw = pcb->pcb_fpu_dflt_cw;
	}
}

void
fpu_save_area_fork(struct pcb *pcb2, const struct pcb *pcb1)
{
	ssize_t extra;

	/*
	 * The pcb itself has been copied, but the xsave area
	 * extends further.
	 */
	extra = offsetof(struct pcb, pcb_savefpu) + x86_fpu_save_size -
	    sizeof (struct pcb);

	if (extra > 0)
		memcpy(pcb2 + 1, pcb1 + 1, extra);
}

void
process_write_fpregs_xmm(struct lwp *l, const struct fxsave *fpregs)
{
	union savefpu *fpu_save;

	fpusave_lwp(l, false);
	fpu_save = process_fpframe(l);

	if (i386_use_fxsave) {
		memcpy(&fpu_save->sv_xmm, fpregs, sizeof(fpu_save->sv_xmm));

		/*
		 * Invalid bits in mxcsr or mxcsr_mask will cause faults.
		 */
		fpu_save->sv_xmm.fx_mxcsr_mask &= x86_fpu_mxcsr_mask;
		fpu_save->sv_xmm.fx_mxcsr &= fpu_save->sv_xmm.fx_mxcsr_mask;

		/*
		 * Make sure the x87 and SSE bits are set in xstate_bv.
		 * Otherwise xrstor will not restore them.
		 */
		if (x86_fpu_save == FPU_SAVE_XSAVE ||
		    x86_fpu_save == FPU_SAVE_XSAVEOPT) {
			fpu_save->sv_xsave_hdr.xsh_xstate_bv |=
			    (XCR0_X87 | XCR0_SSE);
		}
	} else {
		process_xmm_to_s87(fpregs, &fpu_save->sv_87);
	}
}

void
process_write_fpregs_s87(struct lwp *l, const struct save87 *fpregs)
{
	union savefpu *fpu_save;

	if (i386_use_fxsave) {
		/* Save so we don't lose the xmm registers */
		fpusave_lwp(l, true);
		fpu_save = process_fpframe(l);
		process_s87_to_xmm(fpregs, &fpu_save->sv_xmm);

		/*
		 * Make sure the x87 and SSE bits are set in xstate_bv.
		 * Otherwise xrstor will not restore them.
		 */
		if (x86_fpu_save == FPU_SAVE_XSAVE ||
		    x86_fpu_save == FPU_SAVE_XSAVEOPT) {
			fpu_save->sv_xsave_hdr.xsh_xstate_bv |=
			    (XCR0_X87 | XCR0_SSE);
		}
	} else {
		fpusave_lwp(l, false);
		fpu_save = process_fpframe(l);
		memcpy(&fpu_save->sv_87, fpregs, sizeof(fpu_save->sv_87));
	}
}

void
process_read_fpregs_xmm(struct lwp *l, struct fxsave *fpregs)
{
	union savefpu *fpu_save;

	fpusave_lwp(l, true);
	fpu_save = process_fpframe(l);

	if (i386_use_fxsave) {
		memcpy(fpregs, &fpu_save->sv_xmm, sizeof(fpu_save->sv_xmm));
	} else {
		memset(fpregs, 0, sizeof(*fpregs));
		process_s87_to_xmm(&fpu_save->sv_87, fpregs);
	}
}

void
process_read_fpregs_s87(struct lwp *l, struct save87 *fpregs)
{
	union savefpu *fpu_save;

	fpusave_lwp(l, true);
	fpu_save = process_fpframe(l);

	if (i386_use_fxsave) {
		memset(fpregs, 0, sizeof(*fpregs));
		process_xmm_to_s87(&fpu_save->sv_xmm, fpregs);
	} else {
		memcpy(fpregs, &fpu_save->sv_87, sizeof(fpu_save->sv_87));
	}
}
