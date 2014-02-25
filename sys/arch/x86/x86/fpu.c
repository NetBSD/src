/*	$NetBSD: fpu.c,v 1.9 2014/02/25 22:16:52 dsl Exp $	*/

/*-
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

/*-
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

/*-
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

/*
 * XXXfvdl update copyright notice. this started out as a stripped isa/npx.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.9 2014/02/25 22:16:52 dsl Exp $");

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

static inline union savefpu *
process_fpframe(struct lwp *lwp)
{
	struct pcb *pcb = lwp_getpcb(lwp);

	return &pcb->pcb_savefpu;
}

/*
 * We do lazy initialization and switching using the TS bit in cr0 and the
 * MDL_USEDFPU bit in mdlwp.
 *
 * DNA exceptions are handled like this:
 *
 * 1) If there is no FPU, send SIGILL.
 * 2) If someone else has used the FPU, save its state into that lwp's PCB.
 * 3a) If MDL_USEDFPU is not set, set it and initialize the FPU.
 * 3b) Otherwise, reload the lwp's previous FPU state.
 *
 * When a lwp is created or exec()s, its saved cr0 image has the TS bit
 * set and the MDL_USEDFPU bit clear.  The MDL_USEDFPU bit is set when the
 * lwp first gets a DNA and the FPU is initialized.  The TS bit is turned
 * off when the FPU is used, and turned on again later when the lwp's FPU
 * state is saved.
 */

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
 * This might not be structly necessary since it will be initialised
 * for each process.  However it does no harm.
 */
void
fpuinit(struct cpu_info *ci)
{
	if (!i386_fpu_present)
		return;

	clts();
	fninit();
	stts();
}

void
fpu_set_default_cw(struct lwp *lwp, unsigned int x87_cw)
{
	union savefpu *fpu_save = process_fpframe(lwp);

	if (i386_use_fxsave)
		fpu_save->sv_xmm.fx_cw = x87_cw;
	else
		fpu_save->sv_87.s87_cw = x87_cw;
	fpu_save->sv_os.fxo_dflt_cw = x87_cw;
}

static void
send_sigill(void *rip)
{
	/* No fpu (486SX) - send SIGILL */
	ksiginfo_t ksi;

	x86_enable_intr();
	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGILL;
	ksi.ksi_addr = rip;
	(*curlwp->l_proc->p_emul->e_trapsignal)(curlwp, &ksi);
	return;
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

	if (!USERMODE(frame->tf_cs, frame->tf_eflags))
		panic("fpu trap from kernel, trapframe %p\n", frame);

	if (i386_fpu_present == 0) {
		send_sigill((void *)X86_TF_RIP(frame));
		return;
	}

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

	if (!USERMODE(frame->tf_cs, frame->tf_eflags))
		panic("fpudna from kernel, ip %p, trapframe %p\n",
		    (void *)X86_TF_RIP(frame), frame);

	if (i386_fpu_present == 0) {
		send_sigill((void *)X86_TF_RIP(frame));
		return;
	}

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
		KPREEMPT_DISABLE(l);
		splx(s);

		/* Actually enable interrupts */
		x86_enable_intr();

		fpusave_lwp(l, true);
		KASSERT(pcb->pcb_fpcpu == NULL);
		s = splhigh();
		KPREEMPT_ENABLE(l);
	}

	/*
	 * Restore state on this CPU, or initialize.  Ensure that
	 * the entire update is atomic with respect to FPU-sync IPIs.
	 */
	clts();
	ci->ci_fpcurlwp = l;
	pcb->pcb_fpcpu = ci;

	if (i386_use_fxsave) {
		if (x86_xsave_features != 0) {
			xrstor(&pcb->pcb_savefpu, x86_xsave_features);
		} else {
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
		}
	} else {
		frstor(&pcb->pcb_savefpu);
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
		if (i386_use_fxsave) {
			if (x86_xsave_features != 0)
				xsave(&pcb->pcb_savefpu, x86_xsave_features);
			else
				fxsave(&pcb->pcb_savefpu);
		} else {
			fnsave(&pcb->pcb_savefpu);
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
#else /* XEN */
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

/*
 * exec needs to clear the fpu save area to avoid leaking info from the
 * old process to userspace.
 * We must also (later) load these values into the fpu - otherwise the process
 * will see another processes fpu registers.
 */
void
fpu_save_area_clear(struct lwp *lwp, unsigned int x87_cw)
{
	union savefpu *fpu_save;

	fpusave_lwp(lwp, false);

	fpu_save = process_fpframe(lwp);

	if (i386_use_fxsave) {
		memset(&fpu_save->sv_xmm, 0, sizeof fpu_save->sv_xmm);
		fpu_save->sv_xmm.fx_mxcsr = __INITIAL_MXCSR__;
		fpu_save->sv_xmm.fx_mxcsr_mask = __INITIAL_MXCSR_MASK__;
		fpu_save->sv_xmm.fx_cw = x87_cw;
	} else {
		memset(&fpu_save->sv_87, 0, x86_fpu_save_size);
		fpu_save->sv_87.s87_tw = 0xffff;
		fpu_save->sv_87.s87_cw = x87_cw;
	}
	fpu_save->sv_os.fxo_dflt_cw = x87_cw;
}

/* For signal handlers the register values don't matter */
void
fpu_save_area_reset(struct lwp *lwp)
{
	union savefpu *fpu_save = process_fpframe(lwp);

	if (i386_use_fxsave) {
		fpu_save->sv_xmm.fx_mxcsr = __INITIAL_MXCSR__;
		fpu_save->sv_xmm.fx_mxcsr_mask = __INITIAL_MXCSR_MASK__;
		fpu_save->sv_xmm.fx_tw = 0;
		fpu_save->sv_xmm.fx_cw = fpu_save->sv_os.fxo_dflt_cw;
	} else {
		fpu_save->sv_87.s87_tw = 0xffff;
		fpu_save->sv_87.s87_cw = fpu_save->sv_os.fxo_dflt_cw;
	}
}

/* During fork the xsave data needs to be copied */
void
fpu_save_area_fork(struct pcb *pcb2, const struct pcb *pcb1)
{
	ssize_t extra;

	/* The pcb itself has been copied, but the xsave area
	 * extends further. */

	extra = offsetof(struct pcb, pcb_savefpu) + x86_fpu_save_size -
	    sizeof (struct pcb);

	if (extra > 0)
		memcpy(pcb2 + 1, pcb1 + 1, extra);
}


/*
 * Write the FP registers.
 * Buffer has usually come from userspace so should not be trusted.
 */
void
process_write_fpregs_xmm(struct lwp *lwp, const struct fxsave *fpregs)
{
	union savefpu *fpu_save;

	fpusave_lwp(lwp, false);
	fpu_save = process_fpframe(lwp);

	if (i386_use_fxsave) {
		memcpy(&fpu_save->sv_xmm, fpregs,
		    sizeof fpu_save->sv_xmm);
		/* Invalid bits in the mxcsr_mask will cause faults */
		fpu_save->sv_xmm.fx_mxcsr_mask &= __INITIAL_MXCSR_MASK__;
	} else {
		process_xmm_to_s87(fpregs, &fpu_save->sv_87);
	}
}

/* We need to use x87 format for 32bit ptrace */
void
process_write_fpregs_s87(struct lwp *lwp, const struct save87 *fpregs)
{

	if (i386_use_fxsave) {
		/* Save so we don't lose the xmm registers */
		fpusave_lwp(lwp, true);
		process_s87_to_xmm(fpregs, &process_fpframe(lwp)->sv_xmm);
	} else {
		fpusave_lwp(lwp, false);
		memcpy(&process_fpframe(lwp)->sv_87, fpregs,
		    sizeof process_fpframe(lwp)->sv_87);
	}
}

/*
 * Read fpu registers, the buffer is usually copied out to userspace.
 * Ensure we write to the entire structure.
 */
void
process_read_fpregs_xmm(struct lwp *lwp, struct fxsave *fpregs)
{
	fpusave_lwp(lwp, true);

	if (i386_use_fxsave) {
		memcpy(fpregs, &process_fpframe(lwp)->sv_xmm,
		    sizeof process_fpframe(lwp)->sv_xmm);
	} else {
		/* This usually gets copied to userspace */
		memset(fpregs, 0, sizeof *fpregs);
		process_s87_to_xmm(&process_fpframe(lwp)->sv_87, fpregs);

	}
}

void
process_read_fpregs_s87(struct lwp *lwp, struct save87 *fpregs)
{
	fpusave_lwp(lwp, true);

	if (i386_use_fxsave) {
		memset(fpregs, 0, 12);
		process_xmm_to_s87(&process_fpframe(lwp)->sv_xmm, fpregs);
	} else {
		memcpy(fpregs, &process_fpframe(lwp)->sv_87,
		    sizeof process_fpframe(lwp)->sv_87);
	}
}
