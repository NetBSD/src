/*	$NetBSD: mips_dsp.c,v 1.1.12.2 2014/08/20 00:03:12 tls Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: mips_dsp.c,v 1.1.12.2 2014/08/20 00:03:12 tls Exp $");

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

static void mips_dsp_state_save(lwp_t *);
static void mips_dsp_state_load(lwp_t *, u_int);
static void mips_dsp_state_release(lwp_t *);

const pcu_ops_t mips_dsp_ops = {
	.pcu_id = PCU_DSP,
	.pcu_state_save = mips_dsp_state_save,
	.pcu_state_load = mips_dsp_state_load,
	.pcu_state_release = mips_dsp_state_release
};

void
dsp_discard(void)
{
	pcu_discard(&mips_dsp_ops, false);
}

void
dsp_load(void)
{
	pcu_load(&mips_dsp_ops);
}

void
dsp_save(void)
{
	pcu_save(&mips_dsp_ops);
}

bool
dsp_used_p(void)
{
	return pcu_valid_p(&mips_dsp_ops);
}

void
mips_dsp_state_save(lwp_t *l)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct pcb * const pcb = lwp_getpcb(l);
	mips_reg_t * const dsp = pcb->pcb_dspregs.r_regs;
	uint32_t status;

	/*
	 * Don't do anything if the DSP is already off.
	 */
	if ((tf->tf_regs[_R_SR] & MIPS_SR_MX) == 0)
		return;

	l->l_cpu->ci_ev_dsp_saves.ev_count++;

	/*
	 * load DSP registers and establish lwp's DSP context.
	 */
	__asm volatile (
		".set push"				"\n\t"
		".set mips32r2"				"\n\t"
		".set dspr2"				"\n\t"
		".set noat"				"\n\t"
		".set noreorder"			"\n\t"
		"mfc0	%[status], $%[cp0_status]"	"\n\t"
		"or	%[status], %[mips_sr_mx]"	"\n\t"
		"mtc0	%[status], $%[cp0_status]"	"\n\t"
		"ehb"					"\n\t"
		"mflo	%[mullo1], $ac1"		"\n\t"
		"mfhi	%[mulhi1], $ac1"		"\n\t"
		"mflo	%[mullo2], $ac2"		"\n\t"
		"mfhi	%[mulhi2], $ac2"		"\n\t"
		"mflo	%[mullo3], $ac3"		"\n\t"
		"mfhi	%[mulhi3], $ac3"		"\n\t"
		"rddsp	%[dspctl]"			"\n\t"
		"xor	%[status], %[mips_sr_mx]"	"\n\t"
		"mtc0	%[status], $%[cp0_status]"	"\n\t"
		"ehb"					"\n\t"
		".set pop"
	    :	[status] "=&r" (status),
		[mullo1] "=r"(dsp[_R_MULLO1 - _R_DSPBASE]),
		[mulhi1] "=r"(dsp[_R_MULHI1 - _R_DSPBASE]),
		[mullo2] "=r"(dsp[_R_MULLO2 - _R_DSPBASE]),
		[mulhi2] "=r"(dsp[_R_MULHI2 - _R_DSPBASE]),
		[mullo3] "=r"(dsp[_R_MULLO3 - _R_DSPBASE]),
		[mulhi3] "=r"(dsp[_R_MULHI3 - _R_DSPBASE]),
		[dspctl] "=r"(dsp[_R_DSPCTL - _R_DSPBASE])
	    :	[mips_sr_mx] "r"(MIPS_SR_MX),
		[cp0_status] "n"(MIPS_COP_0_STATUS));
}

void
mips_dsp_state_load(lwp_t *l, u_int flags)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct pcb * const pcb = lwp_getpcb(l);
	mips_reg_t * const dsp = pcb->pcb_dspregs.r_regs;
	uint32_t status;

	l->l_cpu->ci_ev_dsp_loads.ev_count++;

	/*
	 * If this is the first time the state is being loaded, zero it first.
	 */
	if (__predict_false((flags & PCU_VALID) == 0)) {
		memset(&pcb->pcb_dspregs, 0, sizeof(pcb->pcb_dspregs));
	}

	/*
	 * Enable the DSP when this lwp return to userspace.
	 */
	tf->tf_regs[_R_SR] |= MIPS_SR_MX;

	/*
	 * load DSP registers and establish lwp's DSP context.
	 */
	__asm volatile (
		".set push"				"\n\t"
		".set mips32r2"				"\n\t"
		".set dspr2"				"\n\t"
		".set noat"				"\n\t"
		".set noreorder"			"\n\t"
		"mfc0	%[status], $%[cp0_status]"	"\n\t"
		"or	%[status], %[mips_sr_mx]"	"\n\t"
		"mtc0	%[status], $%[cp0_status]"	"\n\t"
		"ehb"					"\n\t"
		"mtlo	%[mullo1], $ac1"		"\n\t"
		"mthi	%[mulhi1], $ac1"		"\n\t"
		"mtlo	%[mullo2], $ac2"		"\n\t"
		"mthi	%[mulhi2], $ac2"		"\n\t"
		"mtlo	%[mullo3], $ac3"		"\n\t"
		"mthi	%[mulhi3], $ac3"		"\n\t"
		"wrdsp	%[dspctl]"			"\n\t"
		"xor	%[status], %[mips_sr_mx]"	"\n\t"
		"mtc0	%[status], $%[cp0_status]"	"\n\t"
		"ehb"					"\n\t"
		".set pop"
	    :	[status] "=&r" (status)
	    :	[mullo1] "r"(dsp[_R_MULLO1 - _R_DSPBASE]),
		[mulhi1] "r"(dsp[_R_MULHI1 - _R_DSPBASE]),
		[mullo2] "r"(dsp[_R_MULLO2 - _R_DSPBASE]),
		[mulhi2] "r"(dsp[_R_MULHI2 - _R_DSPBASE]),
		[mullo3] "r"(dsp[_R_MULLO3 - _R_DSPBASE]),
		[mulhi3] "r"(dsp[_R_MULHI3 - _R_DSPBASE]),
		[dspctl] "r"(dsp[_R_DSPCTL - _R_DSPBASE]),
		[mips_sr_mx] "r"(MIPS_SR_MX),
		[cp0_status] "n"(MIPS_COP_0_STATUS));
}

void
mips_dsp_state_release(lwp_t *l)
{
	KASSERT(l == curlwp);
	l->l_md.md_utf->tf_regs[_R_SR] &= ~MIPS_SR_MX;
}
