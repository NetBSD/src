/*	$NetBSD: spe.c,v 1.2 2011/01/18 01:02:52 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: spe.c,v 1.2 2011/01/18 01:02:52 matt Exp $");

#include "opt_altivec.h"

#ifdef PPC_HAVE_SPE

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/siginfo.h>

#include <powerpc/altivec.h>
#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>
#include <powerpc/psl.h>
#include <powerpc/pcb.h>

void
vec_enable(void)
{
	struct cpu_info * const ci = curcpu();
	lwp_t * const l = curlwp;

	l->l_md.md_flags |= MDLWP_USEDVEC;

	/*
	 * Enable SPE temporarily (and disable interrupts).
	 */
	const register_t msr = mfmsr();
	mtmsr((msr & ~PSL_EE) | PSL_SPV);
	__asm volatile ("isync");

	if (ci->ci_veclwp != l) {
		struct pcb * const pcb = lwp_getpcb(l);
		/*
		 * Save the existing state (if any).
		 */
		vec_save_cpu(VEC_SAVE_AND_RELEASE);

		/*
		 * Call an assembly routine to do load everything.
		 */
		vec_load_from_vreg(&pcb->pcb_vr);

		/*
		 * Enable SPE when we return to user-mode (we overload the
		 * ALTIVEC flags).  Record the new ownership of the SPE unit.
		 */
		ci->ci_veclwp = l;
		l->l_md.md_veccpu = ci;
	}
	__asm volatile ("sync");
	l->l_md.md_flags |= MDLWP_OWNVEC;

	/*
	 * Restore MSR (turn off SPE)
	 */
	mtmsr(msr);
}

void
vec_save_cpu(enum vec_op op)
{
	/*
	 * Turn on SPE, turn off interrupts.
	 */
	const register_t msr = mfmsr();
	mtmsr((msr & ~PSL_EE) | PSL_SPV);
	__asm volatile ("isync");

	struct cpu_info * const ci = curcpu();
	lwp_t * const l = ci->ci_veclwp;

	KASSERTMSG(l->l_md.md_veccpu == ci,
	    ("%s: veccpu (%p) != ci (%p)\n", __func__, l->l_md.md_veccpu, ci));
	if (l->l_md.md_flags & MDLWP_OWNVEC) {
		struct pcb * const pcb = lwp_getpcb(l);

		/*
		 * Save the vector state which is best done in assembly.
		 */
		vec_unload_to_vreg(&pcb->pcb_vr);

		/*
		 * Indicate that VEC unit is unloaded
		 */
		l->l_md.md_flags &= ~MDLWP_OWNVEC;

		/*
		 * If asked to, give up the VEC unit.
		 */
		if (op == VEC_SAVE_AND_RELEASE)
			ci->ci_veclwp = ci->ci_data.cpu_idlelwp;
	}

	/*
	 * Restore MSR (turn off SPE)
	 */
	mtmsr(msr);
}

/*
 * Save a lwp's SPE state to its PCB.  The lwp must either be curlwp or traced
 * by curlwp (and stopped).  (The point being that the lwp must not be onproc
 * on another CPU during this function).
 */
void
vec_save_lwp(lwp_t *l, enum vec_op op)
{
	struct cpu_info * const ci = curcpu();

	/*
	 * If it's already in the PCB, there's nothing to do.
	 */
	if ((l->l_md.md_flags & MDLWP_OWNVEC) == 0)
		return;

	/*
	 * If we simply need to discard the information, then don't
	 * to save anything.
	 */
	if (op == VEC_DISCARD) {
		struct cpu_info * const veccpu = l->l_md.md_veccpu;
#ifndef MULTIPROCESSOR
		KASSERT(ci == veccpu);
#endif
		KASSERT(l == veccpu->ci_veclwp);
		KASSERT(l == curlwp || ci == veccpu);
		ci->ci_veclwp = ci->ci_data.cpu_idlelwp;
		atomic_and_uint(&l->l_md.md_flags, ~MDLWP_OWNVEC);
		return;
	}

	KASSERT(l == ci->ci_veclwp);
	vec_save_cpu(op);
}

void
vec_restore_from_mcontext(lwp_t *l, const mcontext_t *mcp)
{
	struct pcb * const pcb = lwp_getpcb(l);
	const union __vr *vr = mcp->__vrf.__vrs;

	vec_save_lwp(l, VEC_DISCARD);

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

	if ((l->l_md.md_flags & MDLWP_USEDVEC) == 0)
		return false;

	vec_save_lwp(l, VEC_SAVE);

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
