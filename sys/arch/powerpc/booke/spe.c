/*	$NetBSD: spe.c,v 1.6 2012/12/26 19:05:04 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: spe.c,v 1.6 2012/12/26 19:05:04 matt Exp $");

#include "opt_altivec.h"

#ifdef PPC_HAVE_SPE

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/siginfo.h>
#include <sys/pcu.h>

#include <powerpc/altivec.h>
#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>
#include <powerpc/psl.h>
#include <powerpc/pcb.h>

static void vec_state_load(lwp_t *, u_int);
static void vec_state_save(lwp_t *, u_int);
static void vec_state_release(lwp_t *, u_int);

const pcu_ops_t vec_ops = {
	.pcu_id = PCU_VEC,
	.pcu_state_load = vec_state_load,
	.pcu_state_save = vec_state_save,
	.pcu_state_release = vec_state_release,
};

bool
vec_used_p(lwp_t *l)
{
	return (l->l_md.md_flags & MDLWP_USEDVEC) != 0;
}

void
vec_mark_used(lwp_t *l)
{
	l->l_md.md_flags |= MDLWP_USEDVEC;
}

void
vec_state_load(lwp_t *l, u_int flags)
{
	struct pcb * const pcb = lwp_getpcb(l);

	if (__predict_false(!vec_used_p(l))) {
		memset(&pcb->pcb_vr, 0, sizeof(pcb->pcb_vr));
		vec_mark_used(l);
	}

	/*
	 * Enable SPE temporarily (and disable interrupts).
	 */
	const register_t msr = mfmsr();
	mtmsr((msr & ~PSL_EE) | PSL_SPV);
	__asm volatile ("isync");

	/*
	 * Call an assembly routine to do load everything.
	 */
	vec_load_from_vreg(&pcb->pcb_vr);
	__asm volatile ("sync");


	/*
	 * Restore MSR (turn off SPE)
	 */
	mtmsr(msr);
	__asm volatile ("isync");

	/*
	 * Note that vector has now been used.
	 */
	l->l_md.md_flags |= MDLWP_USEDVEC;
	l->l_md.md_utf->tf_srr1 |= PSL_SPV;
}

void
vec_state_save(lwp_t *l, u_int flags)
{
	struct pcb * const pcb = lwp_getpcb(l);

	/*
	 * Turn on SPE, turn off interrupts.
	 */
	const register_t msr = mfmsr();
	mtmsr((msr & ~PSL_EE) | PSL_SPV);
	__asm volatile ("isync");

	/*
	 * Save the vector state which is best done in assembly.
	 */
	vec_unload_to_vreg(&pcb->pcb_vr);
	__asm volatile ("sync");

	/*
	 * Restore MSR (turn off SPE)
	 */
	mtmsr(msr);
	__asm volatile ("isync");
}

void
vec_state_release(lwp_t *l, u_int flags)
{
	/*
	 * Turn off SPV so the next SPE instruction will cause a
	 * SPE unavailable exception
	 */
	l->l_md.md_utf->tf_srr1 &= ~PSL_SPV;
}

void
vec_restore_from_mcontext(lwp_t *l, const mcontext_t *mcp)
{
	struct pcb * const pcb = lwp_getpcb(l);
	const union __vr *vr = mcp->__vrf.__vrs;

	KASSERT(l == curlwp);

	vec_save();

	/* grab the accumulator */
	pcb->pcb_vr.vreg[8][0] = vr->__vr32[2];
	pcb->pcb_vr.vreg[8][1] = vr->__vr32[3];

	/*
	 * We store the high parts of each register in the first 8 vectors.
	 */
	for (u_int i = 0; i < 8; i++, vr += 4) {
		pcb->pcb_vr.vreg[i][0] = vr[0].__vr32[0];
		pcb->pcb_vr.vreg[i][1] = vr[1].__vr32[0];
		pcb->pcb_vr.vreg[i][2] = vr[2].__vr32[0];
		pcb->pcb_vr.vreg[i][3] = vr[3].__vr32[0];
	}
	l->l_md.md_utf->tf_spefscr = pcb->pcb_vr.vscr = mcp->__vrf.__vscr;
	pcb->pcb_vr.vrsave = mcp->__vrf.__vrsave;
}

bool
vec_save_to_mcontext(lwp_t *l, mcontext_t *mcp, unsigned int *flagp)
{
	struct pcb * const pcb = lwp_getpcb(l);

	KASSERT(l == curlwp);

	if (!vec_used_p(l))
		return false;

	vec_save();

	mcp->__gregs[_REG_MSR] |= PSL_SPV;

	union __vr *vr = mcp->__vrf.__vrs;
	const register_t *fixreg = l->l_md.md_utf->tf_fixreg;
	for (u_int i = 0; i < 32; i++, vr += 4, fixreg += 4) {
		vr[0].__vr32[0] = pcb->pcb_vr.vreg[i][0];
		vr[0].__vr32[1] = fixreg[0];
		vr[0].__vr32[2] = 0;
		vr[0].__vr32[3] = 0;
		vr[1].__vr32[0] = pcb->pcb_vr.vreg[i][1];
		vr[1].__vr32[1] = fixreg[1];
		vr[1].__vr32[2] = 0;
		vr[1].__vr32[3] = 0;
		vr[2].__vr32[0] = pcb->pcb_vr.vreg[i][2];
		vr[2].__vr32[1] = fixreg[2];
		vr[2].__vr32[2] = 0;
		vr[2].__vr32[3] = 0;
		vr[3].__vr32[0] = pcb->pcb_vr.vreg[i][3];
		vr[3].__vr32[1] = fixreg[3];
		vr[3].__vr32[2] = 0;
		vr[3].__vr32[3] = 0;
	}

	mcp->__vrf.__vrs[0].__vr32[2] = pcb->pcb_vr.vreg[8][0];
	mcp->__vrf.__vrs[0].__vr32[3] = pcb->pcb_vr.vreg[8][1];

	mcp->__vrf.__vrsave = pcb->pcb_vr.vrsave;
	mcp->__vrf.__vscr = l->l_md.md_utf->tf_spefscr;

	*flagp |= _UC_POWERPC_SPE;

	return true;
}

static const struct {
	uint32_t mask;
	int code;
} spefscr_siginfo_map[] = {
	{ SPEFSCR_FINV|SPEFSCR_FINVH, FPE_FLTINV },
	{ SPEFSCR_FOVF|SPEFSCR_FOVFH, FPE_FLTOVF },
	{ SPEFSCR_FUNF|SPEFSCR_FUNFH, FPE_FLTUND },
	{ SPEFSCR_FX  |SPEFSCR_FXH,   FPE_FLTRES },
	{ SPEFSCR_FDBZ|SPEFSCR_FDBZH, FPE_FLTDIV },
	{ SPEFSCR_OV  |SPEFSCR_OVH,   FPE_INTOVF },
};

int
vec_siginfo_code(const struct trapframe *tf)
{
	for (u_int i = 0; i < __arraycount(spefscr_siginfo_map); i++) {
		if (tf->tf_spefscr & spefscr_siginfo_map[i].mask)
			return spefscr_siginfo_map[i].code;
	}
	return 0;
}

#endif /* PPC_HAVE_SPE */
