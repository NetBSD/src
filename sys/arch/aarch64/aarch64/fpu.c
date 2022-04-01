/* $NetBSD: fpu.c,v 1.12 2022/04/01 19:57:22 riastradh Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: fpu.c,v 1.12 2022/04/01 19:57:22 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/lwp.h>
#include <sys/evcnt.h>

#include <arm/cpufunc.h>
#include <arm/fpu.h>
#include <arm/cpufunc.h>

#include <aarch64/locore.h>
#include <aarch64/reg.h>
#include <aarch64/pcb.h>
#include <aarch64/armreg.h>
#include <aarch64/machdep.h>

static void fpu_state_load(lwp_t *, unsigned int);
static void fpu_state_save(lwp_t *);
static void fpu_state_release(lwp_t *);

const pcu_ops_t pcu_fpu_ops = {
	.pcu_id = PCU_FPU,
	.pcu_state_load = fpu_state_load,
	.pcu_state_save = fpu_state_save,
	.pcu_state_release = fpu_state_release
};

void
fpu_attach(struct cpu_info *ci)
{
	evcnt_attach_dynamic(&ci->ci_vfp_use, EVCNT_TYPE_MISC, NULL,
	    ci->ci_cpuname, "vfp use");
	evcnt_attach_dynamic(&ci->ci_vfp_reuse, EVCNT_TYPE_MISC, NULL,
	    ci->ci_cpuname, "vfp reuse");
	evcnt_attach_dynamic(&ci->ci_vfp_save, EVCNT_TYPE_MISC, NULL,
	    ci->ci_cpuname, "vfp save");
	evcnt_attach_dynamic(&ci->ci_vfp_release, EVCNT_TYPE_MISC, NULL,
	    ci->ci_cpuname, "vfp release");
}

static void
fpu_state_load(lwp_t *l, unsigned int flags)
{
	struct pcb * const pcb = lwp_getpcb(l);

	KASSERT(l == curlwp);

	if (__predict_false((flags & PCU_VALID) == 0)) {
		uint64_t mvfr1 = reg_mvfr1_el1_read();
		bool fp16 = false;
		uint32_t fpcr = 0;

		/*
		 * Determine whether ARMv8.2-FP16 binary16
		 * floating-point arithmetic is supported.
		 */
		switch (__SHIFTOUT(mvfr1, MVFR1_FPHP)) {
		case MVFR1_FPHP_HALF_ARITH:
			fp16 = true;
			break;
		}

		/* Rounding mode: round to nearest, ties to even.  */
		fpcr |= __SHIFTIN(FPCR_RN, FPCR_RMODE);

		/* NaN propagation or default NaN.   */
		switch (__SHIFTOUT(mvfr1, MVFR1_FPDNAN)) {
		case MVFR1_FPDNAN_NAN:
			/*
			 * IEEE 754 NaN propagation supported.  Don't
			 * enable default NaN mode.
			 */
			break;
		default:
			/*
			 * IEEE 754 NaN propagation not supported, so
			 * enable default NaN mode.
			 */
			fpcr |= FPCR_DN;
		}

		/* Subnormal arithmetic or flush-to-zero.  */
		switch (__SHIFTOUT(mvfr1, MVFR1_FPFTZ)) {
		case MVFR1_FPFTZ_DENORMAL:
			/*
			 * IEEE 754 subnormal arithmetic supported.
			 * Don't enable flush-to-zero mode.
			 */
			break;
		default:
			/*
			 * IEEE 754 subnormal arithmetic not supported,
			 * so enable flush-to-zero mode.  If FP16 is
			 * supported, also enable flush-to-zero for
			 * binary16 arithmetic.
			 */
			fpcr |= FPCR_FZ;
			if (fp16)
				fpcr |= FPCR_FZ16;
		}

		/* initialize fpregs */
		memset(&pcb->pcb_fpregs, 0, sizeof(pcb->pcb_fpregs));
		pcb->pcb_fpregs.fpcr = fpcr;

		curcpu()->ci_vfp_use.ev_count++;
	} else {
		curcpu()->ci_vfp_reuse.ev_count++;
	}

	/* allow user process to use FP */
	l->l_md.md_cpacr = CPACR_FPEN_ALL;
	reg_cpacr_el1_write(CPACR_FPEN_ALL);
	isb();

	if ((flags & PCU_REENABLE) == 0)
		load_fpregs(&pcb->pcb_fpregs);
}

static void
fpu_state_save(lwp_t *l)
{
	struct pcb * const pcb = lwp_getpcb(l);

	curcpu()->ci_vfp_save.ev_count++;

	reg_cpacr_el1_write(CPACR_FPEN_EL1);	/* fpreg access enable */
	isb();

	save_fpregs(&pcb->pcb_fpregs);

	reg_cpacr_el1_write(CPACR_FPEN_NONE);	/* fpreg access disable */
	isb();
}

static void
fpu_state_release(lwp_t *l)
{
	curcpu()->ci_vfp_release.ev_count++;

	/* disallow user process to use FP */
	l->l_md.md_cpacr = CPACR_FPEN_NONE;
	reg_cpacr_el1_write(CPACR_FPEN_NONE);
	isb();
}

static const struct fpreg zero_fpreg;

/*
 * True if this is a system thread with its own private FPU state.
 */
static inline bool
lwp_system_fpu_p(struct lwp *l)
{

	return (l->l_flag & (LW_SYSTEM|LW_SYSTEM_FPU)) ==
	    (LW_SYSTEM|LW_SYSTEM_FPU);
}

void
fpu_kern_enter(void)
{
	struct cpu_info *ci;
	int s;

	if (lwp_system_fpu_p(curlwp) && !cpu_intr_p()) {
		KASSERT(!cpu_softintr_p());
		return;
	}

	/*
	 * Block interrupts up to IPL_VM.  We must block preemption
	 * since -- if this is a user thread -- there is nowhere to
	 * save the kernel fpu state, and if we want this to be usable
	 * in interrupts, we can't let interrupts interfere with the
	 * fpu state in use since there's nowhere for them to save it.
	 */
	s = splvm();
	ci = curcpu();
	KASSERTMSG(ci->ci_cpl <= IPL_VM || cold, "cpl=%d", ci->ci_cpl);
	KASSERT(ci->ci_kfpu_spl == -1);
	ci->ci_kfpu_spl = s;

	/* Save any fpu state on the current CPU.  */
	pcu_save_all_on_cpu();

	/*
	 * Enable the fpu, and wait until it is enabled before
	 * executing any further instructions.
	 */
	reg_cpacr_el1_write(CPACR_FPEN_ALL);
	isb();
}

void
fpu_kern_leave(void)
{
	struct cpu_info *ci;
	int s;

	if (lwp_system_fpu_p(curlwp) && !cpu_intr_p()) {
		KASSERT(!cpu_softintr_p());
		return;
	}

	ci = curcpu();

	KASSERT(ci->ci_cpl == IPL_VM || cold);
	KASSERT(ci->ci_kfpu_spl != -1);

	/*
	 * Zero the fpu registers; otherwise we might leak secrets
	 * through Spectre-class attacks to userland, even if there are
	 * no bugs in fpu state management.
	 */
	load_fpregs(&zero_fpreg);

	/*
	 * Disable the fpu so that the kernel can't accidentally use
	 * it again.
	 */
	reg_cpacr_el1_write(CPACR_FPEN_NONE);
	isb();

	s = ci->ci_kfpu_spl;
	ci->ci_kfpu_spl = -1;
	splx(s);
}

void
kthread_fpu_enter_md(void)
{

	fpu_load(curlwp);
}

void
kthread_fpu_exit_md(void)
{

	/* XXX Should fpu_state_release zero the registers itself?  */
	load_fpregs(&zero_fpreg);
	fpu_discard(curlwp, 0);
}
