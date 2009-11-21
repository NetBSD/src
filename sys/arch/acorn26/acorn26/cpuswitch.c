/*	$NetBSD: cpuswitch.c,v 1.17 2009/11/21 20:32:17 rmind Exp $	*/

/*
 * Copyright (c) 2000 Ben Harris.
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

/*
 * Some additional routines that happened to be in locore.S traditionally,
 * but have no need to be coded in assembly.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpuswitch.c,v 1.17 2009/11/21 20:32:17 rmind Exp $");

#include "opt_lockdebug.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/systm.h>
#include <sys/ras.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/frame.h>
#include <machine/machdep.h>

/*
 * Switch to the indicated lwp.
 */
lwp_t *
cpu_switchto(lwp_t *old, lwp_t *new, bool returning)
{
	struct cpu_info * const ci = curcpu();
	struct pcb *pcb;
	struct proc *p2;

	/*
	 * We enter here with interrupts blocked and sched_lock held.
	 */

#if 0
	printf("cpu_switchto: %p -> %p", old, new);
#endif

	curlwp = new;
	pcb = lwp_getpcb(curlwp);
	ci->ci_curpcb = pcb;

	if ((new->l_flag & LW_SYSTEM) == 0) {
		/* Check for Restartable Atomic Sequences. */
		p2 = new->l_proc;
		if (p2->p_raslist != NULL) {
			struct trapframe *tf = pcb->pcb_tf;
			void *pc;

			pc = ras_lookup(p2, (void *)(tf->tf_r15 & R15_PC));
			if (pc != (void *) -1)
				tf->tf_r15 = (tf->tf_r15 & ~R15_PC) |
				    (register_t) pc;
		}
	}

	return cpu_loswitch(old, new);
}
