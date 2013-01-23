/*	$NetBSD: mips_fpu.c,v 1.7.2.2 2013/01/23 00:05:55 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mips_fpu.c,v 1.7.2.2 2013/01/23 00:05:55 yamt Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/pcu.h>

#include <mips/locore.h>
#include <mips/regnum.h>
#include <mips/pcb.h>

static void mips_fpu_state_save(lwp_t *, u_int);
static void mips_fpu_state_load(lwp_t *, u_int);
static void mips_fpu_state_release(lwp_t *, u_int);

const pcu_ops_t mips_fpu_ops = {
	.pcu_id = PCU_FPU,
	.pcu_state_save = mips_fpu_state_save,
	.pcu_state_load = mips_fpu_state_load,
	.pcu_state_release = mips_fpu_state_release
};

void
fpu_discard(void)
{
	pcu_discard(&mips_fpu_ops);
}

void
fpu_load(void)
{
	pcu_load(&mips_fpu_ops);
}

void
fpu_save(void)
{
	pcu_save(&mips_fpu_ops);
}

bool
fpu_used_p(void)
{
	return pcu_used_p(&mips_fpu_ops);
}

void
mips_fpu_state_save(lwp_t *l, u_int flags)
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
		".set at"		"\n\t"
		".set reorder"		"\n\t"
	    :	"=&r" (status), "=r"(fpcsr)
	    :	"r"(tf->tf_regs[_R_SR] & (MIPS_SR_COP_1_BIT|MIPS3_SR_FR|MIPS_SR_KX|MIPS_SR_INT_IE)),
		"n"(MIPS_COP_0_STATUS));

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
mips_fpu_state_load(lwp_t *l, u_int flags)
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
	if (__predict_false((flags & PCU_LOADED) == 0)) {
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
		".set at"				"\n\t"
		".set reorder"				"\n\t"
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
		"nop"			"\n\t"	/* XXX: Hack */
		"mtc0	%1, $%2"	"\n\t"
		".set at"		"\n\t"
		".set reorder"		"\n\t"
	    ::	"r"(fpcsr &~ MIPS_FPU_EXCEPTION_BITS), "r"(status),
		"n"(MIPS_COP_0_STATUS));
}

void
mips_fpu_state_release(lwp_t *l, u_int flags)
{

	l->l_md.md_utf->tf_regs[_R_SR] &= ~MIPS_SR_COP_1_BIT;
}
