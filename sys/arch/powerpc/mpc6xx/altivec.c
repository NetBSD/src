/*	$NetBSD: altivec.c,v 1.1 2002/07/02 15:22:47 matt Exp $	*/

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
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/pool.h>

#include <powerpc/altivec.h>
#include <powerpc/spr.h>
#include <powerpc/psl.h>

struct pool vecpool;

void
enable_vec(struct proc *p)
{
	struct pcb *pcb = &p->p_addr->u_pcb;
	struct trapframe *tf = trapframe(p);
	struct vreg *vr = pcb->pcb_vr;
	int msr, scratch;

	/*
	 * Enable AltiVec when we return to user-mode.
	 */
	tf->srr1 |= PSL_VEC;

	/*
	 * Allocate a vreg structure if we haven't done so.
	 */
	if (!(pcb->pcb_flags & PCB_ALTIVEC)) {
		vr = pcb->pcb_vr = pool_get(&vecpool, PR_WAITOK);
		memset(vr, 0, sizeof (*vr));
		pcb->pcb_flags |= PCB_ALTIVEC;
	}

	/*
	 * Enable AltiVec temporarily
	 */
	__asm __volatile ("mfmsr %0; oris %1,%0,%2@h; mtmsr %1; isync"
	    :	"=r"(msr), "=r"(scratch)
	    :	"J"(PSL_VEC));

	/*
	 * Restore VSCR by first loading it into a vector and then into VSCR.
	 * (this needs to done before loading the user's vector registers
	 * since we need to a scratch vector register)
	 */
	__asm __volatile("lvx %2,%0,%1; mtvscr %2" \
	    ::	"r"(vr), "r"(offsetof(struct vreg, vreg[32])), "n"(0));

	/*
	 * VRSAVE will be restored when trap frame returns
	 */
	tf->vrsave = vr->vrsave;

#define	LVX(n,vr)	__asm /*__volatile*/("lvx %2,%0,%1" \
	    ::	"r"(vr), "r"(offsetof(struct vreg, vreg[n])), "n"(n));

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

	/*
	 * Restore MSR (turn off AltiVec)
	 */
	__asm __volatile ("mtmsr %0; isync" :: "r"(msr));
}

void
save_vec(struct proc *p)
{
	struct pcb *pcb = &p->p_addr->u_pcb;
	struct trapframe *tf = trapframe(p);
	struct vreg *vr = pcb->pcb_vr;
	int msr, scratch;
	
	/*
	 * Turn on AltiVEC
	 */
	__asm __volatile ("mfmsr %0; oris %1,%0,%2@h; mtmsr %1; isync"
	    :	"=r"(msr), "=r"(scratch)
	    :	"J"(PSL_VEC));

#define	STVX(n,vr)	__asm /*__volatile*/("stvx %2,%0,%1" \
	    ::	"r"(vr), "r"(offsetof(struct vreg, vreg[n])), "n"(n));

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
	__asm __volatile("mfvscr %2; stvx %2,%0,%1" \
	    ::	"r"(vr), "r"(offsetof(struct vreg, vreg[32])), "n"(0));

	/*
	 * Save VRSAVE
	 */
	vr->vrsave = tf->vrsave;

	/*
	 * Restore MSR (turn off AltiVec).
	 */
	__asm __volatile ("mtmsr %0; isync" :: "r"(msr));

	/*
	 * Note that we aren't using any CPU resources
	 */
	pcb->pcb_veccpu = NULL;
	curcpu()->ci_vecproc = NULL;
	tf->srr1 &= ~PSL_VEC;
}

void
init_vec(void)
{
	pool_init(&vecpool, sizeof(struct vreg), 16, 0, 0, "vecpl", NULL);
}
