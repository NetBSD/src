/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(0, "$NetBSD: mips_fpu.c,v 1.1.2.5 2011/04/29 08:26:28 matt Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/cpu.h>
#ifdef DIAGNOSTIC
#include <sys/proc.h>
#endif
#include <sys/lwp.h>

#include <mips/locore.h>
#include <mips/regnum.h>
#include <mips/pcb.h>

#ifndef NOFPU
static void mips_fpu_state_save(lwp_t *, bool);
static void mips_fpu_state_load(lwp_t *, bool);

static struct {
	kmutex_t fpx_mutex;
#ifdef MULTIPROCESSOR
	kcondvar_t fpx_cv;
#endif
} fp_lockinfo __aligned(COHERENCY_UNIT);
#define	fp_mutex	fp_lockinfo.fpx_mutex
#define	fp_cv		fp_lockinfo.fpx_cv
#endif /* !NOFPU */

void
fpu_init(void)
{
#ifndef NOFPU
	struct cpu_info * const ci = curcpu();

	ci->ci_fpcurlwp = ci->ci_data.cpu_idlelwp;

	mutex_init(&fp_mutex, MUTEX_DEFAULT, IPL_NONE);
#ifdef MULTIPROCESSOR
	cv_init(&fp_cv, "fpsave");
#endif
#endif /* !NOFPU */
}

#ifndef NOFPU
static struct cpu_info *
fp_lock(void)
{
	mutex_enter(&fp_mutex);
	kpreempt_disable();

	return curcpu();
}

static inline void
fp_unlock(void)
{
	mutex_exit(&fp_mutex);
	kpreempt_enable();
}

static void
fpcpu_acquire(lwp_t *l)
{
	struct cpu_info * const ci = curcpu();
	KASSERT(kpreempt_disabled());
	KASSERT(mutex_owned(&fp_mutex));
	KASSERT(l == curlwp);
	KASSERT(l->l_cpu == ci);
	/*
	 * Verify the previous owner doesn't think he still owns the FPU.
	 */
	KASSERT((ci->ci_fpcurlwp->l_md.md_utf->tf_regs[_R_SR] & MIPS_SR_COP_1_BIT) == 0);

	ci->ci_fpcurlwp->l_fpcpu = NULL;
	ci->ci_fpcurlwp = l;
	l->l_fpcpu = l->l_cpu;
#ifdef MULTIPROCESSOR
	cv_broadcast(&fp_cv);
#endif
}

static void
fpcpu_release(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(mutex_owned(&fp_mutex));
	KASSERT(ci->ci_data.cpu_idlelwp->l_fpcpu == NULL);
	/*
	 * Verify the previous owner doesn't think he still owns the FPU.
	 */
	KASSERT((ci->ci_fpcurlwp->l_md.md_utf->tf_regs[_R_SR] & MIPS_SR_COP_1_BIT) == 0);

	ci->ci_fpcurlwp->l_fpcpu = NULL;
	ci->ci_fpcurlwp = ci->ci_data.cpu_idlelwp;
#ifdef MULTIPROCESSOR
	cv_broadcast(&fp_cv);
#endif
}
#endif /* !NOFPU */

void
fpu_discard(void)
{
#ifndef NOFPU
	lwp_t * const l = curlwp;
	if (l->l_fpcpu != NULL) {
		fp_lock();
		if (__predict_true(l->l_fpcpu != NULL)) {
			KASSERT(l->l_fpcpu->ci_fpcurlwp == l);
			l->l_md.md_utf->tf_regs[_R_SR] &= ~MIPS_SR_COP_1_BIT;
			fpcpu_release(l->l_fpcpu);
		}
		fp_unlock();
	}
#endif
}

void
fpu_load(void)
{
#ifndef NOFPU
	lwp_t * const l = curlwp;
	struct trapframe * const tf = l->l_md.md_utf;
	struct cpu_info *ci;

	KASSERT(l == curlwp);

	ci = fp_lock();

	/*
	 * Does this CPU already have our FPU state loaded?
	 */
	if (l->l_fpcpu == ci) {
		KASSERT(ci->ci_fpcurlwp == l);
		tf->tf_regs[_R_SR] |= MIPS_SR_COP_1_BIT;
		fp_unlock();
		return;
	}
#ifdef MULTIPROCESSOR
	/*
	 * While another CPU has our FPU state, keep asking for it to free it.
	 */
	if (l->l_cpu != NULL) {
		kpreempt_enable();
		while (l->l_fpcpu != NULL) {
			cpu_send_ipi(l->l_fpcpu, IPI_FPSAVE);
			cv_wait(&fp_cv, &fp_mutex);
		}
		kpreempt_disable();
		ci = curcpu();
	}
#endif /* MULTIPROCESSOR */
	KASSERT(ci->ci_fpcurlwp != l);

	/*
	 * Save the current FPU state, if any.
	 */
	if (ci->ci_fpcurlwp->l_fpcpu == ci) {
		mips_fpu_state_save(ci->ci_fpcurlwp, true);
	}

	/*
	 * Now acquire this CPU's FPU for this lwp.
	 */
	fpcpu_acquire(l);

	/*
	 * Now load the FPU
	 */
	mips_fpu_state_load(l, fpu_used_p(l));

	fp_unlock();
#endif /* !NOFPU */
}

void
fpu_save(void)
{
#ifndef NOFPU
	fpu_save_lwp(curlwp);
#endif
}

void
fpu_save_lwp(lwp_t *l)
{
#ifndef NOFPU
	if (!fpu_used_p(l) || l->l_fpcpu == NULL)
		return;

	struct cpu_info *ci = fp_lock();

	/*
	 * If the FPU state is on this CPU, save it to the PCB.  However
	 * we leave the FPU attached to this LWP since this might just be
	 * an inquiry.
	 */
	if (l->l_fpcpu == ci) {
		KASSERT(ci->ci_fpcurlwp == l);
		mips_fpu_state_save(l, false);
		fp_unlock();
		return;
	}

#ifdef MULTIPROCESSOR
	/*
	 * Our FP state is on another CPU so we need send it an IPI
	 * to get it to save it.  That CPU will broadcast when it has
	 * saved the FP state and released the FPU.
	 */
	kpreempt_enable();
	while (l->l_fpcpu != NULL) {
		cpu_send_ipi(l->l_fpcpu, IPI_FPSAVE);
		cv_wait(&fp_cv, &fp_mutex);
	}
	kpreempt_disable();
#endif /* MULTIPROCESSOR */

	fp_unlock();
#endif /* !NOFPU */
}

void
fpusave_cpu(struct cpu_info *ci)
{
#ifndef NOFPU
	fp_lock();

	/*
	 * If the current FPU is dirty, save it.
	 */
	if (ci->ci_fpcurlwp->l_fpcpu == ci) {
		mips_fpu_state_save(ci->ci_fpcurlwp, true);

		/*
		 * Release the FPU
		 */
		fpcpu_release(ci);
	}

	fp_unlock();
#endif /* !NOFPU */
}

#ifndef NOFPU
void
mips_fpu_state_save(lwp_t *l, bool release)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct pcb * const pcb = lwp_getpcb(l);
	mips_fpreg_t * const fp = pcb->pcb_fpregs.r_regs;
	uint32_t status, fpcsr;

	/*
	 * Don't do anything if the FPU is already off.
	 */
	if ((tf->tf_regs[_R_SR] & MIPS_SR_COP_1_BIT) == 0)
		return;

	l->l_cpu->ci_ev_fpu_saves.ev_count++;

	/*
	 * enable COP1 to read FPCSR register.
	 * interrupts remain on.
	 */
	__asm volatile (
		".set noreorder"	"\n\t"
		".set noat"		"\n\t"
		"mfc0	%0, $%3"	"\n\t"
		"mtc0	%2, $%3"	"\n\t"
		___STRING(COP0_HAZARD_FPUENABLE)
		"cfc1	%1, $31"	"\n\t"
		"cfc1	%1, $31"	"\n\t"
		".set reorder"		"\n\t"
		".set at"
	    :	"=&r" (status), "=r"(fpcsr)
	    :	"r"(tf->tf_regs[_R_SR] & (MIPS_SR_COP_1_BIT|MIPS3_SR_FR|MIPS_SR_KX|MIPS_SR_INT_IE)),
		"n"(MIPS_COP_0_STATUS));

	/*
	 * Make sure we don't reenable FP when we return to userspace.
	 */
	if (release) {
		tf->tf_regs[_R_SR] ^= MIPS_SR_COP_1_BIT;
	}

	/*
	 * save FPCSR and FP register values.
	 */
#if !defined(__mips_o32)
	if (tf->tf_regs[_R_SR] & MIPS3_SR_FR) {
		KASSERT(_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi));
		fp[32] = fpcsr;
		__asm volatile (
			".set noreorder			;"
			"sdc1	$f0, (0*%d1)(%0)	;"
			"sdc1	$f1, (1*%d1)(%0)	;"
			"sdc1	$f2, (2*%d1)(%0)	;"
			"sdc1	$f3, (3*%d1)(%0)	;"
			"sdc1	$f4, (4*%d1)(%0)	;"
			"sdc1	$f5, (5*%d1)(%0)	;"
			"sdc1	$f6, (6*%d1)(%0)	;"
			"sdc1	$f7, (7*%d1)(%0)	;"
			"sdc1	$f8, (8*%d1)(%0)	;"
			"sdc1	$f9, (9*%d1)(%0)	;"
			"sdc1	$f10, (10*%d1)(%0)	;"
			"sdc1	$f11, (11*%d1)(%0)	;"
			"sdc1	$f12, (12*%d1)(%0)	;"
			"sdc1	$f13, (13*%d1)(%0)	;"
			"sdc1	$f14, (14*%d1)(%0)	;"
			"sdc1	$f15, (15*%d1)(%0)	;"
			"sdc1	$f16, (16*%d1)(%0)	;"
			"sdc1	$f17, (17*%d1)(%0)	;"
			"sdc1	$f18, (18*%d1)(%0)	;"
			"sdc1	$f19, (19*%d1)(%0)	;"
			"sdc1	$f20, (20*%d1)(%0)	;"
			"sdc1	$f21, (21*%d1)(%0)	;"
			"sdc1	$f22, (22*%d1)(%0)	;"
			"sdc1	$f23, (23*%d1)(%0)	;"
			"sdc1	$f24, (24*%d1)(%0)	;"
			"sdc1	$f25, (25*%d1)(%0)	;"
			"sdc1	$f26, (26*%d1)(%0)	;"
			"sdc1	$f27, (27*%d1)(%0)	;"
			"sdc1	$f28, (28*%d1)(%0)	;"
			"sdc1	$f29, (29*%d1)(%0)	;"
			"sdc1	$f30, (30*%d1)(%0)	;"
			"sdc1	$f31, (31*%d1)(%0)	;"
			".set reorder" :: "r"(fp), "i"(sizeof(fp[0])));
	} else
#endif /* !defined(__mips_o32) */
	{
		KASSERT(!_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi));
		((int *)fp)[32] = fpcsr;
		__asm volatile (
			".set noreorder			;"
			"swc1	$f0, (0*%d1)(%0)	;"
			"swc1	$f1, (1*%d1)(%0)	;"
			"swc1	$f2, (2*%d1)(%0)	;"
			"swc1	$f3, (3*%d1)(%0)	;"
			"swc1	$f4, (4*%d1)(%0)	;"
			"swc1	$f5, (5*%d1)(%0)	;"
			"swc1	$f6, (6*%d1)(%0)	;"
			"swc1	$f7, (7*%d1)(%0)	;"
			"swc1	$f8, (8*%d1)(%0)	;"
			"swc1	$f9, (9*%d1)(%0)	;"
			"swc1	$f10, (10*%d1)(%0)	;"
			"swc1	$f11, (11*%d1)(%0)	;"
			"swc1	$f12, (12*%d1)(%0)	;"
			"swc1	$f13, (13*%d1)(%0)	;"
			"swc1	$f14, (14*%d1)(%0)	;"
			"swc1	$f15, (15*%d1)(%0)	;"
			"swc1	$f16, (16*%d1)(%0)	;"
			"swc1	$f17, (17*%d1)(%0)	;"
			"swc1	$f18, (18*%d1)(%0)	;"
			"swc1	$f19, (19*%d1)(%0)	;"
			"swc1	$f20, (20*%d1)(%0)	;"
			"swc1	$f21, (21*%d1)(%0)	;"
			"swc1	$f22, (22*%d1)(%0)	;"
			"swc1	$f23, (23*%d1)(%0)	;"
			"swc1	$f24, (24*%d1)(%0)	;"
			"swc1	$f25, (25*%d1)(%0)	;"
			"swc1	$f26, (26*%d1)(%0)	;"
			"swc1	$f27, (27*%d1)(%0)	;"
			"swc1	$f28, (28*%d1)(%0)	;"
			"swc1	$f29, (29*%d1)(%0)	;"
			"swc1	$f30, (30*%d1)(%0)	;"
			"swc1	$f31, (31*%d1)(%0)	;"
		".set reorder" :: "r"(fp), "i"(4));
	}
	/*
	 * stop COP1
	 */
	__asm volatile ("mtc0 %0, $%1" :: "r"(status), "n"(MIPS_COP_0_STATUS));
}

void
mips_fpu_state_load(lwp_t *l, bool used)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct pcb * const pcb = lwp_getpcb(l);
	mips_fpreg_t * const fp = pcb->pcb_fpregs.r_regs;
	uint32_t status;
	uint32_t fpcsr;

	l->l_cpu->ci_ev_fpu_loads.ev_count++;

	/*
	 * If this is the first time the state is being loaded, zero it first.
	 */
	if (__predict_false(!used)) {
		memset(&pcb->pcb_fpregs, 0, sizeof(pcb->pcb_fpregs));
	}

	/*
	 * Enable the FP when this lwp return to userspace.
	 */
	tf->tf_regs[_R_SR] |= MIPS_SR_COP_1_BIT;

	/*
	 * enabling COP1 to load FP registers.  Interrupts will remain on.
	 */
	__asm volatile(
		".set noreorder"			"\n\t"
		".set noat"				"\n\t"
		"mfc0	%0, $%2" 			"\n\t"
		"mtc0	%1, $%2"			"\n\t"
		___STRING(COP0_HAZARD_FPUENABLE)
		".set reorder"				"\n\t"
		".set at"
	    : "=&r"(status)
	    : "r"(tf->tf_regs[_R_SR] & (MIPS_SR_COP_1_BIT|MIPS3_SR_FR|MIPS_SR_KX|MIPS_SR_INT_IE)), "n"(MIPS_COP_0_STATUS));

	/*
	 * load FP registers and establish processes' FP context.
	 */
#if !defined(__mips_o32)
	if (tf->tf_regs[_R_SR] & MIPS3_SR_FR) {
		KASSERT(_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi));
		__asm volatile (
			".set noreorder			;"
			"ldc1	$f0, (0*%d1)(%0)	;"
			"ldc1	$f1, (1*%d1)(%0)	;"
			"ldc1	$f2, (2*%d1)(%0)	;"
			"ldc1	$f3, (3*%d1)(%0)	;"
			"ldc1	$f4, (4*%d1)(%0)	;"
			"ldc1	$f5, (5*%d1)(%0)	;"
			"ldc1	$f6, (6*%d1)(%0)	;"
			"ldc1	$f7, (7*%d1)(%0)	;"
			"ldc1	$f8, (8*%d1)(%0)	;"
			"ldc1	$f9, (9*%d1)(%0)	;"
			"ldc1	$f10, (10*%d1)(%0)	;"
			"ldc1	$f11, (11*%d1)(%0)	;"
			"ldc1	$f12, (12*%d1)(%0)	;"
			"ldc1	$f13, (13*%d1)(%0)	;"
			"ldc1	$f14, (14*%d1)(%0)	;"
			"ldc1	$f15, (15*%d1)(%0)	;"
			"ldc1	$f16, (16*%d1)(%0)	;"
			"ldc1	$f17, (17*%d1)(%0)	;"
			"ldc1	$f18, (18*%d1)(%0)	;"
			"ldc1	$f19, (19*%d1)(%0)	;"
			"ldc1	$f20, (20*%d1)(%0)	;"
			"ldc1	$f21, (21*%d1)(%0)	;"
			"ldc1	$f22, (22*%d1)(%0)	;"
			"ldc1	$f23, (23*%d1)(%0)	;"
			"ldc1	$f24, (24*%d1)(%0)	;"
			"ldc1	$f25, (25*%d1)(%0)	;"
			"ldc1	$f26, (26*%d1)(%0)	;"
			"ldc1	$f27, (27*%d1)(%0)	;"
			"ldc1	$f28, (28*%d1)(%0)	;"
			"ldc1	$f29, (29*%d1)(%0)	;"
			"ldc1	$f30, (30*%d1)(%0)	;"
			"ldc1	$f31, (31*%d1)(%0)	;"
			".set reorder" :: "r"(fp), "i"(sizeof(fp[0])));
		fpcsr = fp[32];
	} else
#endif
	{
		KASSERT(!_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi));
		__asm volatile (
			".set noreorder			;"
			"lwc1	$f0, (0*%d1)(%0)	;"
			"lwc1	$f1, (1*%d1)(%0)	;"
			"lwc1	$f2, (2*%d1)(%0)	;"
			"lwc1	$f3, (3*%d1)(%0)	;"
			"lwc1	$f4, (4*%d1)(%0)	;"
			"lwc1	$f5, (5*%d1)(%0)	;"
			"lwc1	$f6, (6*%d1)(%0)	;"
			"lwc1	$f7, (7*%d1)(%0)	;"
			"lwc1	$f8, (8*%d1)(%0)	;"
			"lwc1	$f9, (9*%d1)(%0)	;"
			"lwc1	$f10, (10*%d1)(%0)	;"
			"lwc1	$f11, (11*%d1)(%0)	;"
			"lwc1	$f12, (12*%d1)(%0)	;"
			"lwc1	$f13, (13*%d1)(%0)	;"
			"lwc1	$f14, (14*%d1)(%0)	;"
			"lwc1	$f15, (15*%d1)(%0)	;"
			"lwc1	$f16, (16*%d1)(%0)	;"
			"lwc1	$f17, (17*%d1)(%0)	;"
			"lwc1	$f18, (18*%d1)(%0)	;"
			"lwc1	$f19, (19*%d1)(%0)	;"
			"lwc1	$f20, (20*%d1)(%0)	;"
			"lwc1	$f21, (21*%d1)(%0)	;"
			"lwc1	$f22, (22*%d1)(%0)	;"
			"lwc1	$f23, (23*%d1)(%0)	;"
			"lwc1	$f24, (24*%d1)(%0)	;"
			"lwc1	$f25, (25*%d1)(%0)	;"
			"lwc1	$f26, (26*%d1)(%0)	;"
			"lwc1	$f27, (27*%d1)(%0)	;"
			"lwc1	$f28, (28*%d1)(%0)	;"
			"lwc1	$f29, (29*%d1)(%0)	;"
			"lwc1	$f30, (30*%d1)(%0)	;"
			"lwc1	$f31, (31*%d1)(%0)	;"
			".set reorder"
		    :
		    : "r"(fp), "i"(4));
		fpcsr = ((int *)fp)[32];
	}

	/*
	 * load FPCSR and stop COP1 again
	 */
	__asm volatile(
		".set noreorder"	"\n\t"
		".set noat"		"\n\t"
		"ctc1	%0, $31"	"\n\t"
		"mtc0	%1, $%2"	"\n\t"
		".set reorder"		"\n\t"
		".set at"
	    ::	"r"(fpcsr &~ MIPS_FPU_EXCEPTION_BITS), "r"(status),
		"n"(MIPS_COP_0_STATUS));
}
#endif /* !NOFPU */
