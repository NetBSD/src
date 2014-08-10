/*	$NetBSD: altivec.c,v 1.28.2.1 2014/08/10 06:54:05 tls Exp $	*/

/*
 * Copyright (C) 1996 Wolfgang Solfrank.
 * Copyright (C) 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: altivec.c,v 1.28.2.1 2014/08/10 06:54:05 tls Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/atomic.h>

#include <uvm/uvm_extern.h>		/*  for vcopypage/vzeropage */

#include <powerpc/pcb.h>
#include <powerpc/altivec.h>
#include <powerpc/spr.h>
#include <powerpc/oea/spr.h>
#include <powerpc/psl.h>

static void vec_state_load(lwp_t *, u_int);
static void vec_state_save(lwp_t *);
static void vec_state_release(lwp_t *);

const pcu_ops_t vec_ops = {
	.pcu_id = PCU_VEC,
	.pcu_state_load = vec_state_load,
	.pcu_state_save = vec_state_save,
	.pcu_state_release = vec_state_release,
};

bool
vec_used_p(lwp_t *l)
{
	return pcu_valid_p(&vec_ops);
}

void
vec_mark_used(lwp_t *l)
{
	return pcu_discard(&vec_ops, true);
}

void
vec_state_load(lwp_t *l, u_int flags)
{
	struct pcb * const pcb = lwp_getpcb(l);

	if ((flags & PCU_VALID) == 0) {
		memset(&pcb->pcb_vr, 0, sizeof(pcb->pcb_vr));
		vec_mark_used(l);
	}

	/*
	 * Enable AltiVec temporarily (and disable interrupts).
	 */
	const register_t msr = mfmsr();
	mtmsr((msr & ~PSL_EE) | PSL_VEC);
	__asm volatile ("isync");

	/*
	 * Load the vector unit from vreg which is best done in
	 * assembly.
	 */
	vec_load_from_vreg(&pcb->pcb_vr);

	/*
	 * VRSAVE will be restored when trap frame returns
	 */
	l->l_md.md_utf->tf_vrsave = pcb->pcb_vr.vrsave;

	/*
	 * Restore MSR (turn off AltiVec)
	 */
	mtmsr(msr);
	__asm volatile ("isync");

	/*
	 * Mark vector registers as modified.
	 */
	l->l_md.md_flags |= PSL_VEC;
	l->l_md.md_utf->tf_srr1 |= PSL_VEC;
}

void
vec_state_save(lwp_t *l)
{
	struct pcb * const pcb = lwp_getpcb(l);

	/*
	 * Turn on AltiVEC, turn off interrupts.
	 */
	const register_t msr = mfmsr();
	mtmsr((msr & ~PSL_EE) | PSL_VEC);
	__asm volatile ("isync");

	/*
	 * Grab contents of vector unit.
	 */
	vec_unload_to_vreg(&pcb->pcb_vr);

	/*
	 * Save VRSAVE
	 */
	pcb->pcb_vr.vrsave = l->l_md.md_utf->tf_vrsave;

	/*
	 * Note that we aren't using any CPU resources and stop any
	 * data streams.
	 */
	__asm volatile ("dssall; sync");

	/*
	 * Restore MSR (turn off AltiVec)
	 */
	mtmsr(msr);
	__asm volatile ("isync");
}

void
vec_state_release(lwp_t *l)
{
	__asm volatile("dssall;sync");
	l->l_md.md_utf->tf_srr1 &= ~PSL_VEC;
	l->l_md.md_flags &= ~PSL_VEC;
}

void
vec_restore_from_mcontext(struct lwp *l, const mcontext_t *mcp)
{
	struct pcb * const pcb = lwp_getpcb(l);

	KASSERT(l == curlwp);

	/* we don't need to save the state, just drop it */
	pcu_discard(&vec_ops, true);
	memcpy(pcb->pcb_vr.vreg, &mcp->__vrf.__vrs, sizeof (pcb->pcb_vr.vreg));
	pcb->pcb_vr.vscr = mcp->__vrf.__vscr;
	pcb->pcb_vr.vrsave = mcp->__vrf.__vrsave;
	l->l_md.md_utf->tf_vrsave = pcb->pcb_vr.vrsave;
}

bool
vec_save_to_mcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flagp)
{
	struct pcb * const pcb = lwp_getpcb(l);

	KASSERT(l == curlwp);

	/* Save AltiVec context, if any. */
	if (!vec_used_p(l))
		return false;

	/*
	 * If we're the AltiVec owner, dump its context to the PCB first.
	 */
	pcu_save(&vec_ops);

	mcp->__gregs[_REG_MSR] |= PSL_VEC;
	mcp->__vrf.__vscr = pcb->pcb_vr.vscr;
	mcp->__vrf.__vrsave = l->l_md.md_utf->tf_vrsave;
	memcpy(mcp->__vrf.__vrs, pcb->pcb_vr.vreg, sizeof (mcp->__vrf.__vrs));
	*flagp |= _UC_POWERPC_VEC;
	return true;
}

#define ZERO_VEC	19

void
vzeropage(paddr_t pa)
{
	const paddr_t ea = pa + PAGE_SIZE;
	uint32_t vec[7], *vp = (void *) roundup((uintptr_t) vec, 16);
	register_t omsr, msr;

	__asm volatile("mfmsr %0" : "=r"(omsr) :);

	/*
	 * Turn on AltiVec, turn off interrupts.
	 */
	msr = (omsr & ~PSL_EE) | PSL_VEC;
	__asm volatile("sync; mtmsr %0; isync" :: "r"(msr));

	/*
	 * Save the VEC register we are going to use before we disable
	 * relocation.
	 */
	__asm("stvx %1,0,%0" :: "r"(vp), "n"(ZERO_VEC));
	__asm("vxor %0,%0,%0" :: "n"(ZERO_VEC));

	/*
	 * Zero the page using a single cache line.
	 */
	__asm volatile(
	    "   sync ;"
	    "   mfmsr  %[msr];"
	    "   rlwinm %[msr],%[msr],0,28,26;"	/* Clear PSL_DR */
	    "   mtmsr  %[msr];"			/* Turn off DMMU */
	    "   isync;"
	    "1: stvx   %[zv], %[pa], %[off0];"
	    "   stvxl  %[zv], %[pa], %[off16];"
	    "   stvx   %[zv], %[pa], %[off32];"
	    "   stvxl  %[zv], %[pa], %[off48];"
	    "   addi   %[pa], %[pa], 64;"
	    "   cmplw  %[pa], %[ea];"
	    "	blt+   1b;"
	    "   ori    %[msr], %[msr], 0x10;"	/* Set PSL_DR */
	    "   sync;"
	    "	mtmsr  %[msr];"			/* Turn on DMMU */
	    "   isync;"
	    :: [msr] "r"(msr), [pa] "b"(pa), [ea] "b"(ea),
	    [off0] "r"(0), [off16] "r"(16), [off32] "r"(32), [off48] "r"(48), 
	    [zv] "n"(ZERO_VEC));

	/*
	 * Restore VEC register (now that we can access the stack again).
	 */
	__asm("lvx %1,0,%0" :: "r"(vp), "n"(ZERO_VEC));

	/*
	 * Restore old MSR (AltiVec OFF).
	 */
	__asm volatile("sync; mtmsr %0; isync" :: "r"(omsr));
}

#define LO_VEC	16
#define HI_VEC	17

void
vcopypage(paddr_t dst, paddr_t src)
{
	const paddr_t edst = dst + PAGE_SIZE;
	uint32_t vec[11], *vp = (void *) roundup((uintptr_t) vec, 16);
	register_t omsr, msr;

	__asm volatile("mfmsr %0" : "=r"(omsr) :);

	/*
	 * Turn on AltiVec, turn off interrupts.
	 */
	msr = (omsr & ~PSL_EE) | PSL_VEC;
	__asm volatile("sync; mtmsr %0; isync" :: "r"(msr));

	/*
	 * Save the VEC registers we will be using before we disable
	 * relocation.
	 */
	__asm("stvx %2,%1,%0" :: "b"(vp), "r"( 0), "n"(LO_VEC));
	__asm("stvx %2,%1,%0" :: "b"(vp), "r"(16), "n"(HI_VEC));

	/*
	 * Copy the page using a single cache line, with DMMU
	 * disabled.  On most PPCs, two vector registers occupy one
	 * cache line.
	 */
	__asm volatile(
	    "   sync ;"
	    "   mfmsr  %[msr];"
	    "   rlwinm %[msr],%[msr],0,28,26;"	/* Clear PSL_DR */
	    "   mtmsr  %[msr];"			/* Turn off DMMU */
	    "   isync;"
	    "1: lvx    %[lv], %[src], %[off0];"
	    "   stvx   %[lv], %[dst], %[off0];"
	    "   lvxl   %[hv], %[src], %[off16];"
	    "   stvxl  %[hv], %[dst], %[off16];"
	    "   addi   %[src], %[src], 32;"
	    "   addi   %[dst], %[dst], 32;"
	    "   cmplw  %[dst], %[edst];"
	    "	blt+   1b;"
	    "   ori    %[msr], %[msr], 0x10;"	/* Set PSL_DR */
	    "   sync;"
	    "	mtmsr  %[msr];"			/* Turn on DMMU */
	    "   isync;"
	    :: [msr] "r"(msr), [src] "b"(src), [dst] "b"(dst),
	    [edst] "b"(edst), [off0] "r"(0), [off16] "r"(16), 
	    [lv] "n"(LO_VEC), [hv] "n"(HI_VEC));

	/*
	 * Restore VEC registers (now that we can access the stack again).
	 */
	__asm("lvx %2,%1,%0" :: "b"(vp), "r"( 0), "n"(LO_VEC));
	__asm("lvx %2,%1,%0" :: "b"(vp), "r"(16), "n"(HI_VEC));

	/*
	 * Restore old MSR (AltiVec OFF).
	 */
	__asm volatile("sync; mtmsr %0; isync" :: "r"(omsr));
}
