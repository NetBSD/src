/*	$NetBSD: ast.c,v 1.2.2.7 2002/06/24 22:03:45 nathanw Exp $	*/

/*
 * Copyright (c) 1994,1995 Mark Brinicombe
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
 *	This product includes software developed by the RiscBSD team.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * ast.c
 *
 * Code to handle ast's and returns to user mode
 *
 * Created      : 11/10/94
 */

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/acct.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/savar.h>
#include <sys/vmmeter.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/cpu.h>

#include <arm/cpufunc.h>

#include <uvm/uvm_extern.h>

#ifdef acorn26
#include <machine/machdep.h>
#endif

/*
 * Prototypes
 */
void ast __P((struct trapframe *));
 
int want_resched = 0;
int astpending;

void
userret(struct lwp *l)
{
	struct proc *p = l->l_proc;
	int sig;

	/* Take pending signals. */
	while ((sig = (CURSIG(l))) != 0)
		postsig(sig);

	/* Invoke per-process kernel-exit handling, if any */
	if (p->p_userret)
		(p->p_userret)(l, p->p_userret_arg);

	/* Invoke any pending upcalls. */
	if (l->l_flag & L_SA_UPCALL)
		sa_upcall_userret(l);

	curcpu()->ci_schedstate.spc_curpriority = l->l_priority = l->l_usrpri;
}


/*
 * Handle asynchronous system traps.
 * This is called from the irq handler to deliver signals
 * and switch processes if required.
 */

void
ast(struct trapframe *tf)
{
	struct lwp *l = curlwp;
	struct proc *p;

#ifdef acorn26
	/* Enable interrupts if they were enabled before the trap. */
	if ((tf->tf_r15 & R15_IRQ_DISABLE) == 0)
		int_on();
#else
	/* Interrupts were restored by exception_exit. */
#endif

	uvmexp.traps++;
	uvmexp.softs++;

#ifdef DEBUG
	if (l == NULL)
		panic("ast: no curlwp!");
	if (&l->l_addr->u_pcb == 0)
		panic("ast: no pcb!");
#endif	

	p = l->l_proc;

	if (p->p_flag & P_OWEUPC) {
		p->p_flag &= ~P_OWEUPC;
		ADDUPROF(p);
	}

	/* Allow a forced task switch. */
	if (want_resched)
		preempt(NULL);

	userret(l);
}

/* End of ast.c */
