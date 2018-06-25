/*	$NetBSD: fpu.c,v 1.28.2.1 2018/06/25 07:25:47 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.28.2.1 2018/06/25 07:25:47 pgoyette Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/xcall.h>

#include <machine/cpu.h>
#include <machine/cpuvar.h>
#include <machine/cputypes.h>
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

bool x86_fpu_eager __read_mostly = false;

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
 *    cannot be masked but must be preserved.
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
 * 2  QNaN operand (not an exception, irrelevant here)
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
	u_long psl;

	memset(&fpusave, 0, sizeof(fpusave));

	/* Disable interrupts, and enable FPU */
	psl = x86_read_psl();
	x86_disable_intr();
	clts();

	/* Fill in the FPU area */
	fxsave(&fpusave);

	/* Restore previous state */
	stts();
	x86_write_psl(psl);

	if (fpusave.sv_xmm.fx_mxcsr_mask == 0) {
		x86_fpu_mxcsr_mask = __INITIAL_MXCSR_MASK__;
	} else {
		x86_fpu_mxcsr_mask = fpusave.sv_xmm.fx_mxcsr_mask;
	}
#else
	/*
	 * XXX XXX XXX: On Xen the FXSAVE above faults. That's because
	 * &fpusave is not 16-byte aligned. Stack alignment problem
	 * somewhere, it seems.
	 */
	x86_fpu_mxcsr_mask = __INITIAL_MXCSR_MASK__;
#endif
}

static void
fpu_clear_amd(void)
{
	/*
	 * AMD FPUs do not restore FIP, FDP, and FOP on fxrstor and xrstor
	 * when FSW.ES=0, leaking other threads' execution history.
	 *
	 * Clear them manually by loading a zero (fldummy). We do this
	 * unconditionally, regardless of FSW.ES.
	 *
	 * Before that, clear the ES bit in the x87 status word if it is
	 * currently set, in order to avoid causing a fault in the
	 * upcoming load.
	 *
	 * Newer generations of AMD CPUs have CPUID_Fn80000008_EBX[2],
	 * which indicates that FIP/FDP/FOP are restored (same behavior
	 * as Intel). We're not using it though.
	 */
	if (fngetsw() & 0x80)
		fnclex();
	fldummy();
}

static void
fpu_save(struct lwp *l)
{
	struct pcb *pcb = lwp_getpcb(l);

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

static void
fpu_restore(struct lwp *l)
{
	struct pcb *pcb = lwp_getpcb(l);

	switch (x86_fpu_save) {
	case FPU_SAVE_FSAVE:
		frstor(&pcb->pcb_savefpu);
		break;
	case FPU_SAVE_FXSAVE:
		if (cpu_vendor == CPUVENDOR_AMD)
			fpu_clear_amd();
		fxrstor(&pcb->pcb_savefpu);
		break;
	case FPU_SAVE_XSAVE:
	case FPU_SAVE_XSAVEOPT:
		if (cpu_vendor == CPUVENDOR_AMD)
			fpu_clear_amd();
		xrstor(&pcb->pcb_savefpu, x86_xsave_features);
		break;
	}
}

static void
fpu_eagerrestore(struct lwp *l)
{
	struct pcb *pcb = lwp_getpcb(l);
	struct cpu_info *ci = curcpu();

	clts();
	KASSERT(ci->ci_fpcurlwp == NULL);
	KASSERT(pcb->pcb_fpcpu == NULL);
	ci->ci_fpcurlwp = l;
	pcb->pcb_fpcpu = ci;
	fpu_restore(l);
}

void
fpu_eagerswitch(struct lwp *oldlwp, struct lwp *newlwp)
{
	int s;

	s = splhigh();
	fpusave_cpu(true);
	if (!(newlwp->l_flag & LW_SYSTEM))
		fpu_eagerrestore(newlwp);
	splx(s);
}

/* -------------------------------------------------------------------------- */

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
		if (__predict_false(x86_fpu_eager)) {
			panic("%s: FPU busy with EagerFPU enabled",
			    __func__);
		}

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
		if (__predict_false(x86_fpu_eager)) {
			panic("%s: LWP busy with EagerFPU enabled",
			    __func__);
		}

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

	fpu_restore(l);

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
		fpu_save(l);
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

	if (i386_use_fxsave) {
		fpu_save->sv_xmm.fx_cw = x87_cw;

		/* Force a reload of CW */
		if ((x87_cw != __INITIAL_NPXCW__) &&
		    (x86_fpu_save == FPU_SAVE_XSAVE ||
		    x86_fpu_save == FPU_SAVE_XSAVEOPT)) {
			fpu_save->sv_xsave_hdr.xsh_xstate_bv |=
			    XCR0_X87;
		}
	} else {
		fpu_save->sv_87.s87_cw = x87_cw;
	}
	pcb->pcb_fpu_dflt_cw = x87_cw;
}

void
fpu_save_area_clear(struct lwp *l, unsigned int x87_cw)
{
	union savefpu *fpu_save;
	struct pcb *pcb;
	int s;

	KASSERT(l == curlwp);
	KASSERT((l->l_flag & LW_SYSTEM) == 0);
	fpu_save = process_fpframe(l);
	pcb = lwp_getpcb(l);

	s = splhigh();
	if (x86_fpu_eager) {
		KASSERT(pcb->pcb_fpcpu == NULL ||
		    pcb->pcb_fpcpu == curcpu());
		fpusave_cpu(false);
	} else {
		splx(s);
		fpusave_lwp(l, false);
	}
	KASSERT(pcb->pcb_fpcpu == NULL);

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

	if (x86_fpu_eager) {
		fpu_eagerrestore(l);
		splx(s);
	}
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

	KASSERT(pcb2->pcb_fpcpu == NULL);
}

/* -------------------------------------------------------------------------- */

static void
process_xmm_to_s87(const struct fxsave *sxmm, struct save87 *s87)
{
	unsigned int tag, ab_tag;
	const struct fpaccfx *fx_reg;
	struct fpacc87 *s87_reg;
	int i;

	/*
	 * For historic reasons core dumps and ptrace all use the old save87
	 * layout.  Convert the important parts.
	 * getucontext gets what we give it.
	 * setucontext should return something given by getucontext, but
	 * we are (at the moment) willing to change it.
	 *
	 * It really isn't worth setting the 'tag' bits to 01 (zero) or
	 * 10 (NaN etc) since the processor will set any internal bits
	 * correctly when the value is loaded (the 287 believed them).
	 *
	 * Additionally the s87_tw and s87_tw are 'indexed' by the actual
	 * register numbers, whereas the registers themselves have ST(0)
	 * first. Pairing the values and tags can only be done with
	 * reference to the 'top of stack'.
	 *
	 * If any x87 registers are used, they will typically be from
	 * r7 downwards - so the high bits of the tag register indicate
	 * used registers. The conversions are not optimised for this.
	 *
	 * The ABI we use requires the FP stack to be empty on every
	 * function call. I think this means that the stack isn't expected
	 * to overflow - overflow doesn't drop a core in my testing.
	 *
	 * Note that this code writes to all of the 's87' structure that
	 * actually gets written to userspace.
	 */

	/* FPU control/status */
	s87->s87_cw = sxmm->fx_cw;
	s87->s87_sw = sxmm->fx_sw;
	/* tag word handled below */
	s87->s87_ip = sxmm->fx_ip;
	s87->s87_opcode = sxmm->fx_opcode;
	s87->s87_dp = sxmm->fx_dp;

	/* FP registers (in stack order) */
	fx_reg = sxmm->fx_87_ac;
	s87_reg = s87->s87_ac;
	for (i = 0; i < 8; fx_reg++, s87_reg++, i++)
		*s87_reg = fx_reg->r;

	/* Tag word and registers. */
	ab_tag = sxmm->fx_tw & 0xff;	/* Bits set if valid */
	if (ab_tag == 0) {
		/* none used */
		s87->s87_tw = 0xffff;
		return;
	}

	tag = 0;
	/* Separate bits of abridged tag word with zeros */
	for (i = 0x80; i != 0; tag <<= 1, i >>= 1)
		tag |= ab_tag & i;
	/* Replicate and invert so that 0 => 0b11 and 1 => 0b00 */
	s87->s87_tw = (tag | tag >> 1) ^ 0xffff;
}

static void
process_s87_to_xmm(const struct save87 *s87, struct fxsave *sxmm)
{
	unsigned int tag, ab_tag;
	struct fpaccfx *fx_reg;
	const struct fpacc87 *s87_reg;
	int i;

	/*
	 * ptrace gives us registers in the save87 format and
	 * we must convert them to the correct format.
	 *
	 * This code is normally used when overwriting the processes
	 * registers (in the pcb), so it musn't change any other fields.
	 *
	 * There is a lot of pad in 'struct fxsave', if the destination
	 * is written to userspace, it must be zeroed first.
	 */

	/* FPU control/status */
	sxmm->fx_cw = s87->s87_cw;
	sxmm->fx_sw = s87->s87_sw;
	/* tag word handled below */
	sxmm->fx_ip = s87->s87_ip;
	sxmm->fx_opcode = s87->s87_opcode;
	sxmm->fx_dp = s87->s87_dp;

	/* Tag word */
	tag = s87->s87_tw;	/* 0b11 => unused */
	if (tag == 0xffff) {
		/* All unused - values don't matter, zero for safety */
		sxmm->fx_tw = 0;
		memset(&sxmm->fx_87_ac, 0, sizeof sxmm->fx_87_ac);
		return;
	}

	tag ^= 0xffff;		/* So 0b00 is unused */
	tag |= tag >> 1;	/* Look at even bits */
	ab_tag = 0;
	i = 1;
	do
		ab_tag |= tag & i;
	while ((tag >>= 1) >= (i <<= 1));
	sxmm->fx_tw = ab_tag;

	/* FP registers (in stack order) */
	fx_reg = sxmm->fx_87_ac;
	s87_reg = s87->s87_ac;
	for (i = 0; i < 8; fx_reg++, s87_reg++, i++)
		fx_reg->r = *s87_reg;
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

/* -------------------------------------------------------------------------- */

static volatile unsigned long eagerfpu_cpu_barrier1 __cacheline_aligned;
static volatile unsigned long eagerfpu_cpu_barrier2 __cacheline_aligned;

static void
eager_change_cpu(void *arg1, void *arg2)
{
	struct cpu_info *ci = curcpu();
	bool enabled = (bool)arg1;
	int s;

	s = splhigh();

	/* Rendez-vous 1. */
	atomic_dec_ulong(&eagerfpu_cpu_barrier1);
	while (atomic_cas_ulong(&eagerfpu_cpu_barrier1, 0, 0) != 0) {
		x86_pause();
	}

	fpusave_cpu(true);
	if (ci == &cpu_info_primary) {
		x86_fpu_eager = enabled;
	}

	/* Rendez-vous 2. */
	atomic_dec_ulong(&eagerfpu_cpu_barrier2);
	while (atomic_cas_ulong(&eagerfpu_cpu_barrier2, 0, 0) != 0) {
		x86_pause();
	}

	splx(s);
}

static int
eager_change(bool enabled)
{
	struct cpu_info *ci = NULL;
	CPU_INFO_ITERATOR cii;
	uint64_t xc;

	mutex_enter(&cpu_lock);

	/*
	 * We expect all the CPUs to be online.
	 */
	for (CPU_INFO_FOREACH(cii, ci)) {
		struct schedstate_percpu *spc = &ci->ci_schedstate;
		if (spc->spc_flags & SPCF_OFFLINE) {
			printf("[!] cpu%d offline, EagerFPU not changed\n",
			    cpu_index(ci));
			mutex_exit(&cpu_lock);
			return EOPNOTSUPP;
		}
	}

	/* Initialize the barriers */
	eagerfpu_cpu_barrier1 = ncpu;
	eagerfpu_cpu_barrier2 = ncpu;

	printf("[+] %s EagerFPU...",
	    enabled ? "Enabling" : "Disabling");
	xc = xc_broadcast(0, eager_change_cpu,
	    (void *)enabled, NULL);
	xc_wait(xc);
	printf(" done!\n");

	mutex_exit(&cpu_lock);

	return 0;
}

static int
sysctl_machdep_fpu_eager(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error;
	bool val;

	val = *(bool *)rnode->sysctl_data;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;

	if (val == x86_fpu_eager)
		return 0;
	return eager_change(val);
}

void sysctl_eagerfpu_init(struct sysctllog **);

void
sysctl_eagerfpu_init(struct sysctllog **clog)
{
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_READWRITE,
		       CTLTYPE_BOOL, "fpu_eager",
		       SYSCTL_DESCR("Whether the kernel uses Eager FPU Switch"),
		       sysctl_machdep_fpu_eager, 0,
		       &x86_fpu_eager, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
}
