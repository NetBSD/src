/*	$NetBSD: userret.h,v 1.16 2009/11/21 17:40:29 rmind Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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

#include "opt_altivec.h"

#include <sys/userret.h>

#include <powerpc/fpu.h>

/*
 * Define the code needed before returning to user mode, for
 * trap and syscall.
 */
static __inline void
userret(struct lwp *l, struct trapframe *frame)
{
#if defined(PPC_HAVE_FPU) || defined(ALTIVEC)
	struct cpu_info * const ci = curcpu();
#endif
#ifdef PPC_HAVE_FPU
	struct pcb * const pcb = lwp_getpcb(l);
#endif

	/* Invoke MI userret code */
	mi_userret(l);

	frame->srr1 &= PSL_USERSRR1;	/* clear SRR1 status bits */

	/*
	 * If someone stole the fp or vector unit while we were away,
	 * disable it.  Note that if the PSL FP/VEC bits aren't set, then
	 * we don't own it.
	 */
#ifdef PPC_HAVE_FPU
	if ((frame->srr1 & PSL_FP) &&
	    (l != ci->ci_fpulwp || pcb->pcb_fpcpu != ci)) {
		frame->srr1 &= ~(PSL_FP|PSL_FE0|PSL_FE1);
	}
#endif
#ifdef ALTIVEC
	/*
	 * We need to manually restore PSL_VEC each time we return
	 * to user mode since PSL_VEC is not preserved in SRR1.
	 */
	if (frame->srr1 & PSL_VEC) {
		if (l != ci->ci_veclwp)
			frame->srr1 &= ~PSL_VEC;
	} else {
		if (l == ci->ci_veclwp)
			frame->srr1 |= PSL_VEC;
	}

	/*
	 * If the new process isn't the current AltiVec process on this
	 * CPU, we need to stop any data streams that are active (since
	 * it will be a different address space).
	 */
	if (ci->ci_veclwp != NULL && ci->ci_veclwp != l) {
		__asm volatile("dssall;sync");
	}
#endif
}
