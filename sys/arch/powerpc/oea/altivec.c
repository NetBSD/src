/*	$NetBSD: altivec.c,v 1.19 2011/01/18 02:25:42 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: altivec.c,v 1.19 2011/01/18 02:25:42 matt Exp $");

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

#ifdef MULTIPROCESSOR
#include <arch/powerpc/pic/picvar.h>
#include <arch/powerpc/pic/ipivar.h>
static void vec_mp_save_lwp(struct lwp *);
#endif

void
vec_enable(void)
{
	struct cpu_info *ci = curcpu();
	struct lwp *l = curlwp;
	register_t msr;

	KASSERT(l->l_md.md_veccpu != NULL);

	l->l_md.md_flags |= MDLWP_USEDVEC;

	/*
	 * Enable AltiVec temporarily (and disable interrupts).
	 */
	msr = mfmsr();
	mtmsr((msr & ~PSL_EE) | PSL_VEC);
	__asm volatile ("isync");

	if (ci->ci_veclwp != l) {
		struct pcb * const pcb = lwp_getpcb(l);
		struct trapframe * const tf = l->l_md.md_utf;

		vec_save_cpu(VEC_SAVE_AND_RELEASE);

		/*
		 * Load the vector unit from vreg which is best done in
		 * assembly.
		 */
		vec_load_from_vreg(&pcb->pcb_vr);

		/*
		 * VRSAVE will be restored when trap frame returns
		 */
		tf->tf_vrsave = pcb->pcb_vr.vrsave;

		/*
		 * Enable AltiVec when we return to user-mode.
		 * Record the new ownership of the AltiVec unit.
		 */
		ci->ci_veclwp = l;
		l->l_md.md_veccpu = ci;
		__asm volatile ("sync");
	}
	l->l_md.md_flags |= MDLWP_OWNVEC;

	/*
	 * Restore MSR (turn off AltiVec)
	 */
	mtmsr(msr);
}

void
vec_save_cpu(enum vec_op op)
{
	/*
	 * Turn on AltiVEC, turn off interrupts.
	 */
	const register_t msr = mfmsr();
	mtmsr((msr & ~PSL_EE) | PSL_VEC);
	__asm volatile ("isync");

	struct cpu_info * const ci = curcpu();
	lwp_t * const l = ci->ci_veclwp;

	if (l->l_md.md_flags & MDLWP_OWNVEC) {
		struct pcb * const pcb = lwp_getpcb(l);
		struct trapframe * const tf = l->l_md.md_utf;

		/*
		 * Grab contents of vector unit.
		 */
		vec_unload_to_vreg(&pcb->pcb_vr);

		/*
		 * Save VRSAVE
		 */
		pcb->pcb_vr.vrsave = tf->tf_vrsave;

		/*
		 * Note that we aren't using any CPU resources and stop any
		 * data streams.
		 */
		__asm volatile ("dssall; sync");

		/*
		 * Give up the VEC unit if are releasing it too.
		 */
		if (op == VEC_SAVE_AND_RELEASE)
			ci->ci_veclwp = ci->ci_data.cpu_idlelwp;
	}
	l->l_md.md_flags &= ~MDLWP_OWNVEC;

	/*
	 * Restore MSR (turn off AltiVec)
	 */
	mtmsr(msr);
}

#ifdef MULTIPROCESSOR
/*
 * Save a process's AltiVEC state to its PCB.  The state may be in any CPU.
 * The process must either be curproc or traced by curproc (and stopped).
 * (The point being that the process must not run on another CPU during
 * this function).
 */
static void
vec_mp_save_lwp(struct lwp *l)
{
	/*
	 * Send an IPI to the other CPU with the data and wait for that CPU
	 * to flush the data.  Note that the other CPU might have switched
	 * to a different proc's AltiVEC state by the time it receives the IPI,
	 * but that will only result in an unnecessary reload.
	 */

	if ((l->l_md.md_flags & MDLWP_OWNVEC) == 0)
		return;

	ppc_send_ipi(l->l_md.md_veccpu->ci_index, PPC_IPI_FLUSH_VEC);

	/* Wait for flush. */
	for (u_int i = 0; i < 0x3fffffff; i++) {
		if ((l->l_md.md_flags & MDLWP_OWNVEC) == 0)
			return;
	}

	panic("%s/%d timed out: pid = %d.%d, veccpu->ci_cpuid = %d\n",
	    __func__, cpu_number(), l->l_proc->p_pid, l->l_lid,
	    l->l_md.md_veccpu->ci_cpuid);
}
#endif /*MULTIPROCESSOR*/

/*
 * Save a process's AltiVEC state to its PCB.  The state may be in any CPU.
 * The process must either be curproc or traced by curproc (and stopped).
 * (The point being that the process must not run on another CPU during
 * this function).
 */
void
vec_save_lwp(struct lwp *l, enum vec_op op)
{
	struct cpu_info * const ci = curcpu();

	KASSERT(l->l_md.md_veccpu != NULL);

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
#ifndef MULTIPROCESSOR
		KASSERT(ci == l->l_md.md_veccpu);
#endif
		KASSERT(l == l->l_md.md_veccpu->ci_veclwp);
		KASSERT(l == curlwp || ci == l->l_md.md_veccpu);
		ci->ci_veclwp = ci->ci_data.cpu_idlelwp;
		atomic_and_uint(&l->l_md.md_flags, ~MDLWP_OWNVEC);
		return;
	}

	/*
	 * If the state is in the current CPU, just flush the current CPU's
	 * state.
	 */
	if (l == ci->ci_veclwp) {
		vec_save_cpu(op);
		return;
	}


#ifdef MULTIPROCESSOR
	/*
	 * It must be on another CPU, flush it from there.
	 */
	vec_mp_save_lwp(l);
#endif
}

void
vec_restore_from_mcontext(struct lwp *l, const mcontext_t *mcp)
{
	struct pcb * const pcb = lwp_getpcb(l);

	/* we don't need to save the state, just drop it */
	vec_save_lwp(l, VEC_DISCARD);
	memcpy(pcb->pcb_vr.vreg, &mcp->__vrf.__vrs, sizeof (pcb->pcb_vr.vreg));
	pcb->pcb_vr.vscr = mcp->__vrf.__vscr;
	pcb->pcb_vr.vrsave = mcp->__vrf.__vrsave;
	l->l_md.md_utf->tf_vrsave = pcb->pcb_vr.vrsave;
}

bool
vec_save_to_mcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flagp)
{
	/* Save AltiVec context, if any. */
	if ((l->l_md.md_flags & MDLWP_USEDVEC) == 0)
		return false;

	struct pcb * const pcb = lwp_getpcb(l);

	/*
	 * If we're the AltiVec owner, dump its context to the PCB first.
	 */
	vec_save_lwp(l, VEC_SAVE);

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
