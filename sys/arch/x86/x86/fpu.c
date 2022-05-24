/*	$NetBSD: fpu.c,v 1.78 2022/05/24 06:28:00 andvar Exp $	*/

/*
 * Copyright (c) 2008, 2019 The NetBSD Foundation, Inc.  All
 * rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Andrew Doran and Maxime Villard.
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
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.78 2022/05/24 06:28:00 andvar Exp $");

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

#ifdef XENPV
#define clts() HYPERVISOR_fpu_taskswitch(0)
#define stts() HYPERVISOR_fpu_taskswitch(1)
#endif

void fpu_handle_deferred(void);
void fpu_switch(struct lwp *, struct lwp *);

uint32_t x86_fpu_mxcsr_mask __read_mostly = 0;

static inline union savefpu *
fpu_lwp_area(struct lwp *l)
{
	struct pcb *pcb = lwp_getpcb(l);
	union savefpu *area = &pcb->pcb_savefpu;

	KASSERT((l->l_flag & LW_SYSTEM) == 0);
	if (l == curlwp) {
		fpu_save();
	}
	KASSERT(!(l->l_md.md_flags & MDL_FPU_IN_CPU));

	return area;
}

static inline void
fpu_save_lwp(struct lwp *l)
{
	struct pcb *pcb = lwp_getpcb(l);
	union savefpu *area = &pcb->pcb_savefpu;
	int s;

	s = splvm();
	if (l->l_md.md_flags & MDL_FPU_IN_CPU) {
		KASSERT((l->l_flag & LW_SYSTEM) == 0);
		fpu_area_save(area, x86_xsave_features, !(l->l_proc->p_flag & PK_32));
		l->l_md.md_flags &= ~MDL_FPU_IN_CPU;
	}
	splx(s);
}

/*
 * Bring curlwp's FPU state in memory. It will get installed back in the CPU
 * when returning to userland.
 */
void
fpu_save(void)
{
	fpu_save_lwp(curlwp);
}

void
fpuinit(struct cpu_info *ci)
{
	/*
	 * This might not be strictly necessary since it will be initialized
	 * for each process. However it does no harm.
	 */
	clts();
	fninit();
	stts();
}

void
fpuinit_mxcsr_mask(void)
{
#ifndef XENPV
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

static inline void
fpu_errata_amd(void)
{
	uint16_t sw;

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
	fnstsw(&sw);
	if (sw & 0x80)
		fnclex();
	fldummy();
}

#ifdef __x86_64__
#define XS64(x) (is_64bit ? x##64 : x)
#else
#define XS64(x) x
#endif

void
fpu_area_save(void *area, uint64_t xsave_features, bool is_64bit)
{
	switch (x86_fpu_save) {
	case FPU_SAVE_FSAVE:
		fnsave(area);
		break;
	case FPU_SAVE_FXSAVE:
		XS64(fxsave)(area);
		break;
	case FPU_SAVE_XSAVE:
		XS64(xsave)(area, xsave_features);
		break;
	case FPU_SAVE_XSAVEOPT:
		XS64(xsaveopt)(area, xsave_features);
		break;
	}

	stts();
}

void
fpu_area_restore(const void *area, uint64_t xsave_features, bool is_64bit)
{
	clts();

	switch (x86_fpu_save) {
	case FPU_SAVE_FSAVE:
		frstor(area);
		break;
	case FPU_SAVE_FXSAVE:
		if (cpu_vendor == CPUVENDOR_AMD)
			fpu_errata_amd();
		XS64(fxrstor)(area);
		break;
	case FPU_SAVE_XSAVE:
	case FPU_SAVE_XSAVEOPT:
		if (cpu_vendor == CPUVENDOR_AMD)
			fpu_errata_amd();
		XS64(xrstor)(area, xsave_features);
		break;
	}
}

void
fpu_handle_deferred(void)
{
	struct pcb *pcb = lwp_getpcb(curlwp);
	fpu_area_restore(&pcb->pcb_savefpu, x86_xsave_features,
	    !(curlwp->l_proc->p_flag & PK_32));
}

void
fpu_switch(struct lwp *oldlwp, struct lwp *newlwp)
{
	struct cpu_info *ci __diagused = curcpu();
	struct pcb *pcb;

	KASSERTMSG(ci->ci_ilevel >= IPL_SCHED, "cpu%d ilevel=%d",
	    cpu_index(ci), ci->ci_ilevel);

	if (oldlwp->l_md.md_flags & MDL_FPU_IN_CPU) {
		KASSERT(!(oldlwp->l_flag & LW_SYSTEM));
		pcb = lwp_getpcb(oldlwp);
		fpu_area_save(&pcb->pcb_savefpu, x86_xsave_features,
		    !(oldlwp->l_proc->p_flag & PK_32));
		oldlwp->l_md.md_flags &= ~MDL_FPU_IN_CPU;
	}
	KASSERT(!(newlwp->l_md.md_flags & MDL_FPU_IN_CPU));
}

void
fpu_lwp_fork(struct lwp *l1, struct lwp *l2)
{
	struct pcb *pcb2 = lwp_getpcb(l2);
	union savefpu *fpu_save;

	/* Kernel threads have no FPU. */
	if (__predict_false(l2->l_flag & LW_SYSTEM)) {
		return;
	}
	/* For init(8). */
	if (__predict_false(l1->l_flag & LW_SYSTEM)) {
		memset(&pcb2->pcb_savefpu, 0, x86_fpu_save_size);
		return;
	}

	fpu_save = fpu_lwp_area(l1);
	memcpy(&pcb2->pcb_savefpu, fpu_save, x86_fpu_save_size);
	l2->l_md.md_flags &= ~MDL_FPU_IN_CPU;
}

void
fpu_lwp_abandon(struct lwp *l)
{
	int s;

	KASSERT(l == curlwp);
	s = splvm();
	l->l_md.md_flags &= ~MDL_FPU_IN_CPU;
	stts();
	splx(s);
}

/* -------------------------------------------------------------------------- */

/*
 * fpu_kern_enter()
 *
 *	Begin using the FPU.  Raises to splvm, disabling most
 *	interrupts and rendering the thread non-preemptible; caller
 *	should not use this for long periods of time, and must call
 *	fpu_kern_leave() afterward.  Non-recursive -- you cannot call
 *	fpu_kern_enter() again without calling fpu_kern_leave() first.
 *
 *	Must be used only at IPL_VM or below -- never in IPL_SCHED or
 *	IPL_HIGH interrupt handlers.
 */
void
fpu_kern_enter(void)
{
	struct lwp *l = curlwp;
	struct cpu_info *ci;
	int s;

	s = splvm();

	ci = curcpu();
	KASSERTMSG(ci->ci_ilevel <= IPL_VM || cold, "ilevel=%d",
	    ci->ci_ilevel);
	KASSERT(ci->ci_kfpu_spl == -1);
	ci->ci_kfpu_spl = s;

	/*
	 * If we are in a softint and have a pinned lwp, the fpu state is that
	 * of the pinned lwp, so save it there.
	 */
	while ((l->l_pflag & LP_INTR) && (l->l_switchto != NULL))
		l = l->l_switchto;
	fpu_save_lwp(l);

	/*
	 * Clear CR0_TS, which fpu_save_lwp set if it saved anything --
	 * otherwise the CPU will trap if we try to use the FPU under
	 * the false impression that there has been a task switch since
	 * the last FPU usage requiring that we save the FPU state.
	 */
	clts();
}

/*
 * fpu_kern_leave()
 *
 *	End using the FPU after fpu_kern_enter().
 */
void
fpu_kern_leave(void)
{
	static const union savefpu zero_fpu __aligned(64);
	struct cpu_info *ci = curcpu();
	int s;

	KASSERT(ci->ci_ilevel == IPL_VM || cold);
	KASSERT(ci->ci_kfpu_spl != -1);

	/*
	 * Zero the fpu registers; otherwise we might leak secrets
	 * through Spectre-class attacks to userland, even if there are
	 * no bugs in fpu state management.
	 */
	fpu_area_restore(&zero_fpu, x86_xsave_features, false);

	/*
	 * Set CR0_TS again so that the kernel can't accidentally use
	 * the FPU.
	 */
	stts();

	s = ci->ci_kfpu_spl;
	ci->ci_kfpu_spl = -1;
	splx(s);
}

/* -------------------------------------------------------------------------- */

/*
 * The following table is used to ensure that the FPE_... value
 * that is passed as a trapcode to the signal handler of the user
 * process does not have more than one bit set.
 *
 * Multiple bits may be set if SSE simd instructions generate errors
 * on more than one value or if the user process modifies the control
 * word while a status word bit is already set (which this is a sign
 * of bad coding).
 * We have no choice than to narrow them down to one bit, since we must
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
 * This is a synchronous trap on either an x87 instruction (due to an unmasked
 * error on the previous x87 instruction) or on an SSE/SSE2/etc instruction due
 * to an error on the instruction itself.
 *
 * If trap actually generates a signal, then the fpu state is saved and then
 * copied onto the lwp's user-stack, and then recovered from there when the
 * signal returns.
 *
 * All this code needs to do is save the reason for the trap. For x87 traps the
 * status word bits need clearing to stop the trap re-occurring. For SSE traps
 * the mxcsr bits are 'sticky' and need clearing to not confuse a later trap.
 *
 * We come here with interrupts disabled.
 */
void
fputrap(struct trapframe *frame)
{
	uint32_t statbits;
	ksiginfo_t ksi;

	if (__predict_false(!USERMODE(frame->tf_cs))) {
		panic("fpu trap from kernel, trapframe %p\n", frame);
	}

	KASSERT(curlwp->l_md.md_flags & MDL_FPU_IN_CPU);

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

		/* Remove masked interrupts */
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

void
fpudna(struct trapframe *frame)
{
	panic("fpudna from %s, ip %p, trapframe %p",
	    USERMODE(frame->tf_cs) ? "userland" : "kernel",
	    (void *)X86_TF_RIP(frame), frame);
}

/* -------------------------------------------------------------------------- */

static inline void
fpu_xstate_reload(union savefpu *fpu_save, uint64_t xstate)
{
	/*
	 * Force a reload of the given xstate during the next XRSTOR.
	 */
	if (x86_fpu_save >= FPU_SAVE_XSAVE) {
		fpu_save->sv_xsave_hdr.xsh_xstate_bv |= xstate;
	}
}

void
fpu_set_default_cw(struct lwp *l, unsigned int x87_cw)
{
	union savefpu *fpu_save = fpu_lwp_area(l);
	struct pcb *pcb = lwp_getpcb(l);

	if (i386_use_fxsave) {
		fpu_save->sv_xmm.fx_cw = x87_cw;
		if (x87_cw != __INITIAL_NPXCW__) {
			fpu_xstate_reload(fpu_save, XCR0_X87);
		}
	} else {
		fpu_save->sv_87.s87_cw = x87_cw;
	}
	pcb->pcb_fpu_dflt_cw = x87_cw;
}

void
fpu_clear(struct lwp *l, unsigned int x87_cw)
{
	union savefpu *fpu_save;
	struct pcb *pcb;

	KASSERT(l == curlwp);
	fpu_save = fpu_lwp_area(l);

	switch (x86_fpu_save) {
	case FPU_SAVE_FSAVE:
		memset(&fpu_save->sv_87, 0, x86_fpu_save_size);
		fpu_save->sv_87.s87_tw = 0xffff;
		fpu_save->sv_87.s87_cw = x87_cw;
		break;
	case FPU_SAVE_FXSAVE:
		memset(&fpu_save->sv_xmm, 0, x86_fpu_save_size);
		fpu_save->sv_xmm.fx_mxcsr = __INITIAL_MXCSR__;
		fpu_save->sv_xmm.fx_mxcsr_mask = x86_fpu_mxcsr_mask;
		fpu_save->sv_xmm.fx_cw = x87_cw;
		break;
	case FPU_SAVE_XSAVE:
	case FPU_SAVE_XSAVEOPT:
		memset(&fpu_save->sv_xmm, 0, x86_fpu_save_size);
		fpu_save->sv_xmm.fx_mxcsr = __INITIAL_MXCSR__;
		fpu_save->sv_xmm.fx_mxcsr_mask = x86_fpu_mxcsr_mask;
		fpu_save->sv_xmm.fx_cw = x87_cw;
		if (__predict_false(x87_cw != __INITIAL_NPXCW__)) {
			fpu_xstate_reload(fpu_save, XCR0_X87);
		}
		break;
	}

	pcb = lwp_getpcb(l);
	pcb->pcb_fpu_dflt_cw = x87_cw;
}

void
fpu_sigreset(struct lwp *l)
{
	union savefpu *fpu_save = fpu_lwp_area(l);
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
process_write_fpregs_xmm(struct lwp *l, const struct fxsave *fpregs)
{
	union savefpu *fpu_save = fpu_lwp_area(l);

	if (i386_use_fxsave) {
		memcpy(&fpu_save->sv_xmm, fpregs, sizeof(fpu_save->sv_xmm));

		/*
		 * Invalid bits in mxcsr or mxcsr_mask will cause faults.
		 */
		fpu_save->sv_xmm.fx_mxcsr_mask &= x86_fpu_mxcsr_mask;
		fpu_save->sv_xmm.fx_mxcsr &= fpu_save->sv_xmm.fx_mxcsr_mask;

		fpu_xstate_reload(fpu_save, XCR0_X87 | XCR0_SSE);
	} else {
		process_xmm_to_s87(fpregs, &fpu_save->sv_87);
	}
}

void
process_write_fpregs_s87(struct lwp *l, const struct save87 *fpregs)
{
	union savefpu *fpu_save = fpu_lwp_area(l);

	if (i386_use_fxsave) {
		process_s87_to_xmm(fpregs, &fpu_save->sv_xmm);
		fpu_xstate_reload(fpu_save, XCR0_X87 | XCR0_SSE);
	} else {
		memcpy(&fpu_save->sv_87, fpregs, sizeof(fpu_save->sv_87));
	}
}

void
process_read_fpregs_xmm(struct lwp *l, struct fxsave *fpregs)
{
	union savefpu *fpu_save = fpu_lwp_area(l);

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
	union savefpu *fpu_save = fpu_lwp_area(l);

	if (i386_use_fxsave) {
		memset(fpregs, 0, sizeof(*fpregs));
		process_xmm_to_s87(&fpu_save->sv_xmm, fpregs);
	} else {
		memcpy(fpregs, &fpu_save->sv_87, sizeof(fpu_save->sv_87));
	}
}

int
process_read_xstate(struct lwp *l, struct xstate *xstate)
{
	union savefpu *fpu_save = fpu_lwp_area(l);

	if (x86_fpu_save == FPU_SAVE_FSAVE) {
		/* Convert from legacy FSAVE format. */
		memset(&xstate->xs_fxsave, 0, sizeof(xstate->xs_fxsave));
		process_s87_to_xmm(&fpu_save->sv_87, &xstate->xs_fxsave);

		/* We only got x87 data. */
		xstate->xs_rfbm = XCR0_X87;
		xstate->xs_xstate_bv = XCR0_X87;
		return 0;
	}

	/* Copy the legacy area. */
	memcpy(&xstate->xs_fxsave, fpu_save->sv_xsave_hdr.xsh_fxsave,
	    sizeof(xstate->xs_fxsave));

	if (x86_fpu_save == FPU_SAVE_FXSAVE) {
		/* FXSAVE means we've got x87 + SSE data. */
		xstate->xs_rfbm = XCR0_X87 | XCR0_SSE;
		xstate->xs_xstate_bv = XCR0_X87 | XCR0_SSE;
		return 0;
	}

	/* Copy the bitmap indicating which states are available. */
	xstate->xs_rfbm = x86_xsave_features & XCR0_FPU;
	xstate->xs_xstate_bv = fpu_save->sv_xsave_hdr.xsh_xstate_bv;
	KASSERT(!(xstate->xs_xstate_bv & ~xstate->xs_rfbm));

#define COPY_COMPONENT(xcr0_val, xsave_val, field)			\
	if (xstate->xs_xstate_bv & xcr0_val) {				\
		KASSERT(x86_xsave_offsets[xsave_val]			\
		    >= sizeof(struct xsave_header));			\
		KASSERT(x86_xsave_sizes[xsave_val]			\
		    >= sizeof(xstate->field));				\
		memcpy(&xstate->field,					\
		    (char*)fpu_save + x86_xsave_offsets[xsave_val],	\
		    sizeof(xstate->field));				\
	}

	COPY_COMPONENT(XCR0_YMM_Hi128, XSAVE_YMM_Hi128, xs_ymm_hi128);
	COPY_COMPONENT(XCR0_Opmask, XSAVE_Opmask, xs_opmask);
	COPY_COMPONENT(XCR0_ZMM_Hi256, XSAVE_ZMM_Hi256, xs_zmm_hi256);
	COPY_COMPONENT(XCR0_Hi16_ZMM, XSAVE_Hi16_ZMM, xs_hi16_zmm);

#undef COPY_COMPONENT

	return 0;
}

int
process_verify_xstate(const struct xstate *xstate)
{
	/* xstate_bv must be a subset of RFBM */
	if (xstate->xs_xstate_bv & ~xstate->xs_rfbm)
		return EINVAL;

	switch (x86_fpu_save) {
	case FPU_SAVE_FSAVE:
		if ((xstate->xs_rfbm & ~XCR0_X87))
			return EINVAL;
		break;
	case FPU_SAVE_FXSAVE:
		if ((xstate->xs_rfbm & ~(XCR0_X87 | XCR0_SSE)))
			return EINVAL;
		break;
	default:
		/* Verify whether no unsupported features are enabled */
		if ((xstate->xs_rfbm & ~(x86_xsave_features & XCR0_FPU)) != 0)
			return EINVAL;
	}

	return 0;
}

int
process_write_xstate(struct lwp *l, const struct xstate *xstate)
{
	union savefpu *fpu_save = fpu_lwp_area(l);

	/* Convert data into legacy FSAVE format. */
	if (x86_fpu_save == FPU_SAVE_FSAVE) {
		if (xstate->xs_xstate_bv & XCR0_X87)
			process_xmm_to_s87(&xstate->xs_fxsave, &fpu_save->sv_87);
		return 0;
	}

	/* If XSAVE is supported, make sure that xstate_bv is set correctly. */
	if (x86_fpu_save >= FPU_SAVE_XSAVE) {
		/*
		 * Bit-wise "xstate->xs_rfbm ? xstate->xs_xstate_bv :
		 *           fpu_save->sv_xsave_hdr.xsh_xstate_bv"
		 */
		fpu_save->sv_xsave_hdr.xsh_xstate_bv =
		    (fpu_save->sv_xsave_hdr.xsh_xstate_bv & ~xstate->xs_rfbm) |
		    xstate->xs_xstate_bv;
	}

	if (xstate->xs_xstate_bv & XCR0_X87) {
		/*
		 * X87 state is split into two areas, interspersed with SSE
		 * data.
		 */
		memcpy(&fpu_save->sv_xmm, &xstate->xs_fxsave, 24);
		memcpy(fpu_save->sv_xmm.fx_87_ac, xstate->xs_fxsave.fx_87_ac,
		    sizeof(xstate->xs_fxsave.fx_87_ac));
	}

	/*
	 * Copy MXCSR if either SSE or AVX state is requested, to match the
	 * XSAVE behavior for those flags.
	 */
	if (xstate->xs_xstate_bv & (XCR0_SSE|XCR0_YMM_Hi128)) {
		/*
		 * Invalid bits in mxcsr or mxcsr_mask will cause faults.
		 */
		fpu_save->sv_xmm.fx_mxcsr_mask = xstate->xs_fxsave.fx_mxcsr_mask
		    & x86_fpu_mxcsr_mask;
		fpu_save->sv_xmm.fx_mxcsr = xstate->xs_fxsave.fx_mxcsr &
		    fpu_save->sv_xmm.fx_mxcsr_mask;
	}

	if (xstate->xs_xstate_bv & XCR0_SSE) {
		memcpy(&fpu_save->sv_xsave_hdr.xsh_fxsave[160],
		    xstate->xs_fxsave.fx_xmm, sizeof(xstate->xs_fxsave.fx_xmm));
	}

#define COPY_COMPONENT(xcr0_val, xsave_val, field)			\
	if (xstate->xs_xstate_bv & xcr0_val) {				\
		KASSERT(x86_xsave_offsets[xsave_val]			\
		    >= sizeof(struct xsave_header));			\
		KASSERT(x86_xsave_sizes[xsave_val]			\
		    >= sizeof(xstate->field));				\
		memcpy((char *)fpu_save + x86_xsave_offsets[xsave_val],	\
		    &xstate->field, sizeof(xstate->field));		\
	}

	COPY_COMPONENT(XCR0_YMM_Hi128, XSAVE_YMM_Hi128, xs_ymm_hi128);
	COPY_COMPONENT(XCR0_Opmask, XSAVE_Opmask, xs_opmask);
	COPY_COMPONENT(XCR0_ZMM_Hi256, XSAVE_ZMM_Hi256, xs_zmm_hi256);
	COPY_COMPONENT(XCR0_Hi16_ZMM, XSAVE_Hi16_ZMM, xs_hi16_zmm);

#undef COPY_COMPONENT

	return 0;
}
