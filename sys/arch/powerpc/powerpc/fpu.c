/*	$NetBSD: fpu.c,v 1.10 2003/07/15 02:54:47 lukem Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.10 2003/07/15 02:54:47 lukem Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <machine/fpu.h>
#include <machine/psl.h>

void
enable_fpu(void)
{
	struct cpu_info *ci = curcpu();
	struct lwp *l = curlwp;
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct trapframe *tf = trapframe(l);
	int msr;

	KASSERT(pcb->pcb_fpcpu == NULL);
	if (!(pcb->pcb_flags & PCB_FPU)) {
		memset(&pcb->pcb_fpu, 0, sizeof pcb->pcb_fpu);
		pcb->pcb_flags |= PCB_FPU;
	}
	msr = mfmsr();
        mtmsr((msr & ~PSL_EE) | PSL_FP);
	__asm __volatile ("isync");
	if (ci->ci_fpulwp) {
		save_fpu_cpu();
	}
	KASSERT(ci->ci_fpulwp == NULL);
	__asm __volatile ("lfd 0,0(%0); mtfsf 0xff,0" :: "b"(&pcb->pcb_fpu.fpscr));
	__asm ("lfd 0,0(%0);"
	     "lfd 1,8(%0);"
	     "lfd 2,16(%0);"
	     "lfd 3,24(%0);"
	     "lfd 4,32(%0);"
	     "lfd 5,40(%0);"
	     "lfd 6,48(%0);"
	     "lfd 7,56(%0);"
	     "lfd 8,64(%0);"
	     "lfd 9,72(%0);"
	     "lfd 10,80(%0);"
	     "lfd 11,88(%0);"
	     "lfd 12,96(%0);"
	     "lfd 13,104(%0);"
	     "lfd 14,112(%0);"
	     "lfd 15,120(%0);"
	     "lfd 16,128(%0);"
	     "lfd 17,136(%0);"
	     "lfd 18,144(%0);"
	     "lfd 19,152(%0);"
	     "lfd 20,160(%0);"
	     "lfd 21,168(%0);"
	     "lfd 22,176(%0);"
	     "lfd 23,184(%0);"
	     "lfd 24,192(%0);"
	     "lfd 25,200(%0);"
	     "lfd 26,208(%0);"
	     "lfd 27,216(%0);"
	     "lfd 28,224(%0);"
	     "lfd 29,232(%0);"
	     "lfd 30,240(%0);"
	     "lfd 31,248(%0)" :: "b"(&pcb->pcb_fpu.fpr[0]));
	__asm __volatile ("isync");
	tf->srr1 |= PSL_FP;
	ci->ci_fpulwp = l;
	pcb->pcb_fpcpu = ci;
	__asm __volatile ("sync");
	mtmsr(msr);
}

/*
 * Save the contents of the current CPU's FPU to its PCB.
 */
void
save_fpu_cpu(void)
{
	struct cpu_info *ci = curcpu();
	struct lwp *l;
	struct pcb *pcb;
	int msr;

	msr = mfmsr();
        mtmsr((msr & ~PSL_EE) | PSL_FP);
	__asm __volatile ("isync");
	l = ci->ci_fpulwp;
	if (l == NULL) {
		goto out;
	}
	pcb = &l->l_addr->u_pcb;
	__asm ("stfd 0,0(%0);"
	     "stfd 1,8(%0);"
	     "stfd 2,16(%0);"
	     "stfd 3,24(%0);"
	     "stfd 4,32(%0);"
	     "stfd 5,40(%0);"
	     "stfd 6,48(%0);"
	     "stfd 7,56(%0);"
	     "stfd 8,64(%0);"
	     "stfd 9,72(%0);"
	     "stfd 10,80(%0);"
	     "stfd 11,88(%0);"
	     "stfd 12,96(%0);"
	     "stfd 13,104(%0);"
	     "stfd 14,112(%0);"
	     "stfd 15,120(%0);"
	     "stfd 16,128(%0);"
	     "stfd 17,136(%0);"
	     "stfd 18,144(%0);"
	     "stfd 19,152(%0);"
	     "stfd 20,160(%0);"
	     "stfd 21,168(%0);"
	     "stfd 22,176(%0);"
	     "stfd 23,184(%0);"
	     "stfd 24,192(%0);"
	     "stfd 25,200(%0);"
	     "stfd 26,208(%0);"
	     "stfd 27,216(%0);"
	     "stfd 28,224(%0);"
	     "stfd 29,232(%0);"
	     "stfd 30,240(%0);"
	     "stfd 31,248(%0)" :: "b"(&pcb->pcb_fpu.fpr[0]));
	__asm __volatile ("mffs 0; stfd 0,0(%0)" :: "b"(&pcb->pcb_fpu.fpscr));
	__asm __volatile ("sync");
	pcb->pcb_fpcpu = NULL;
	ci->ci_fpulwp = NULL;
	ci->ci_ev_fpusw.ev_count++;
	__asm __volatile ("sync");
 out:
	mtmsr(msr);
}

/*
 * Save a process's FPU state to its PCB.  The state may be in any CPU.
 * The process must either be curproc or traced by curproc (and stopped).
 * (The point being that the process must not run on another CPU during
 * this function).
 */
void
save_fpu_lwp(l)
	struct lwp *l;
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct cpu_info *ci = curcpu();

	/*
	 * If it's already in the PCB, there's nothing to do.
	 */

	if (pcb->pcb_fpcpu == NULL) {
		return;
	}

	/*
	 * If the state is in the current CPU, just flush the current CPU's
	 * state.
	 */

	if (l == ci->ci_fpulwp) {
		save_fpu_cpu();
		return;
	}

#ifdef MULTIPROCESSOR

	/*
	 * It must be on another CPU, flush it from there.
	 */

	mp_save_fpu_lwp(l);
#endif
}
