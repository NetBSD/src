/*	$NetBSD: altivec.c,v 1.15 2009/11/21 17:40:29 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: altivec.c,v 1.15 2009/11/21 17:40:29 rmind Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/pool.h>

#include <uvm/uvm_extern.h>

#include <powerpc/altivec.h>
#include <powerpc/spr.h>
#include <powerpc/psl.h>

#ifdef MULTIPROCESSOR
#include <arch/powerpc/pic/picvar.h>
#include <arch/powerpc/pic/ipivar.h>
static void mp_save_vec_lwp(struct lwp *);
#endif

void
enable_vec(void)
{
	struct cpu_info *ci = curcpu();
	struct lwp *l = curlwp;
	struct pcb *pcb = lwp_getpcb(l);
	struct trapframe *tf = trapframe(l);
	struct vreg *vr = &pcb->pcb_vr;
	register_t msr;

	KASSERT(pcb->pcb_veccpu == NULL);

	pcb->pcb_flags |= PCB_ALTIVEC;

	/*
	 * Enable AltiVec temporarily (and disable interrupts).
	 */
	msr = mfmsr();
	mtmsr((msr & ~PSL_EE) | PSL_VEC);
	__asm volatile ("isync");
	if (ci->ci_veclwp) {
		save_vec_cpu();
	}
	KASSERT(curcpu()->ci_veclwp == NULL);

	/*
	 * Restore VSCR by first loading it into a vector and then into VSCR.
	 * (this needs to done before loading the user's vector registers
	 * since we need to use a scratch vector register)
	 */
	__asm volatile("vxor %2,%2,%2; lvewx %2,%0,%1; mtvscr %2" \
	    ::	"b"(vr), "r"(offsetof(struct vreg, vscr)), "n"(0));

	/*
	 * VRSAVE will be restored when trap frame returns
	 */
	tf->tf_xtra[TF_VRSAVE] = vr->vrsave;

#define	LVX(n,vr)	__asm /*volatile*/("lvx %2,%0,%1" \
	    ::	"b"(vr), "r"(offsetof(struct vreg, vreg[n])), "n"(n));

	/*
	 * Load all 32 vector registers
	 */
	LVX( 0,vr);	LVX( 1,vr);	LVX( 2,vr);	LVX( 3,vr);
	LVX( 4,vr);	LVX( 5,vr);	LVX( 6,vr);	LVX( 7,vr);
	LVX( 8,vr);	LVX( 9,vr);	LVX(10,vr);	LVX(11,vr);
	LVX(12,vr);	LVX(13,vr);	LVX(14,vr);	LVX(15,vr);

	LVX(16,vr);	LVX(17,vr);	LVX(18,vr);	LVX(19,vr);
	LVX(20,vr);	LVX(21,vr);	LVX(22,vr);	LVX(23,vr);
	LVX(24,vr);	LVX(25,vr);	LVX(26,vr);	LVX(27,vr);
	LVX(28,vr);	LVX(29,vr);	LVX(30,vr);	LVX(31,vr);
	__asm volatile ("isync");

	/*
	 * Enable AltiVec when we return to user-mode.
	 * Record the new ownership of the AltiVec unit.
	 */
	curcpu()->ci_veclwp = l;
	pcb->pcb_veccpu = curcpu();
	pcb->pcb_flags |= PCB_OWNALTIVEC;
	__asm volatile ("sync");

	/*
	 * Restore MSR (turn off AltiVec)
	 */
	mtmsr(msr);
}

void
save_vec_cpu(void)
{
	struct cpu_info *ci = curcpu();
	struct lwp *l;
	struct pcb *pcb;
	struct vreg *vr;
	struct trapframe *tf;
	register_t msr;
	
	/*
	 * Turn on AltiVEC, turn off interrupts.
	 */
	msr = mfmsr();
	mtmsr((msr & ~PSL_EE) | PSL_VEC);
	__asm volatile ("isync");
	l = ci->ci_veclwp;
	if (l == NULL)
		goto out;
	pcb = lwp_getpcb(l);
	vr = &pcb->pcb_vr;
	tf = trapframe(l);

#define	STVX(n,vr)	__asm /*volatile*/("stvx %2,%0,%1" \
	    ::	"b"(vr), "r"(offsetof(struct vreg, vreg[n])), "n"(n));

	/*
	 * Save the vector registers.
	 */
	STVX( 0,vr);	STVX( 1,vr);	STVX( 2,vr);	STVX( 3,vr);
	STVX( 4,vr);	STVX( 5,vr);	STVX( 6,vr);	STVX( 7,vr);
	STVX( 8,vr);	STVX( 9,vr);	STVX(10,vr);	STVX(11,vr);
	STVX(12,vr);	STVX(13,vr);	STVX(14,vr);	STVX(15,vr);

	STVX(16,vr);	STVX(17,vr);	STVX(18,vr);	STVX(19,vr);
	STVX(20,vr);	STVX(21,vr);	STVX(22,vr);	STVX(23,vr);
	STVX(24,vr);	STVX(25,vr);	STVX(26,vr);	STVX(27,vr);
	STVX(28,vr);	STVX(29,vr);	STVX(30,vr);	STVX(31,vr);

	/*
	 * Save VSCR (this needs to be done after save the vector registers
	 * since we need to use one as scratch).
	 */
	__asm volatile("mfvscr %2; stvewx %2,%0,%1" \
	    ::	"b"(vr), "r"(offsetof(struct vreg, vscr)), "n"(0));

	/*
	 * Save VRSAVE
	 */
	vr->vrsave = tf->tf_xtra[TF_VRSAVE];

	/*
	 * Note that we aren't using any CPU resources and stop any
	 * data streams.
	 */
	pcb->pcb_veccpu = NULL;
	ci->ci_veclwp = NULL;
	__asm volatile ("dssall; sync");

 out:

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
mp_save_vec_lwp(struct lwp *l)
{
	struct pcb *pcb = lwp_getpcb(l);
	struct cpu_info *veccpu;
	int i;

	/*
	 * Send an IPI to the other CPU with the data and wait for that CPU
	 * to flush the data.  Note that the other CPU might have switched
	 * to a different proc's AltiVEC state by the time it receives the IPI,
	 * but that will only result in an unnecessary reload.
	 */

	veccpu = pcb->pcb_veccpu;
	if (veccpu == NULL)
		return;

	ppc_send_ipi(veccpu->ci_index, PPC_IPI_FLUSH_VEC);

	/* Wait for flush. */
	for (i = 0; i < 0x3fffffff; i++)
		if (pcb->pcb_veccpu == NULL)
			return;

	aprint_error("mp_save_vec_lwp{%d} pid = %d.%d, veccpu->ci_cpuid = %d\n",
	    cpu_number(), l->l_proc->p_pid, l->l_lid, veccpu->ci_cpuid);
	panic("mp_save_vec_lwp: timed out");
}
#endif /*MULTIPROCESSOR*/

/*
 * Save a process's AltiVEC state to its PCB.  The state may be in any CPU.
 * The process must either be curproc or traced by curproc (and stopped).
 * (The point being that the process must not run on another CPU during
 * this function).
 */
void
save_vec_lwp(struct lwp *l, int discard)
{
	struct pcb * const pcb = lwp_getpcb(l);
	struct cpu_info * const ci = curcpu();

	/*
	 * If it's already in the PCB, there's nothing to do.
	 */
	if (pcb->pcb_veccpu == NULL)
		return;

	/*
	 * If we simply need to discard the information, then don't
	 * to save anything.
	 */
	if (discard) {
#ifndef MULTIPROCESSOR
		KASSERT(ci == pcb->pcb_veccpu);
#endif
		KASSERT(l == pcb->pcb_veccpu->ci_veclwp);
		pcb->pcb_veccpu->ci_veclwp = NULL;
		pcb->pcb_veccpu = NULL;
		pcb->pcb_flags &= ~PCB_OWNALTIVEC;
		return;
	}

	/*
	 * If the state is in the current CPU, just flush the current CPU's
	 * state.
	 */
	if (l == ci->ci_veclwp) {
		save_vec_cpu();
		return;
	}


#ifdef MULTIPROCESSOR
	/*
	 * It must be on another CPU, flush it from there.
	 */

	mp_save_vec_lwp(l);
#endif
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
