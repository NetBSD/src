/*	$NetBSD: Locore.c,v 1.7.8.1 2001/11/15 08:16:27 thorpej Exp $	*/

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

#include "opt_lockdebug.h"

#include <sys/param.h>

__RCSID("$NetBSD: Locore.c,v 1.7.8.1 2001/11/15 08:16:27 thorpej Exp $");

#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <machine/machdep.h>

static void idle(void);

/*
 * Put process p on the run queue indicated by its priority.
 * Calls should be made at splstatclock(), and p->p_stat should be SRUN.
 */
void
setrunqueue(struct lwp *l)
{
	struct prochd *q;
	struct lwp *oldlast;
	int which = l->l_priority >> 2;
	
#ifdef	DIAGNOSTIC
	if (l->l_back)
		panic("setrunqueue");
#endif
	q = &sched_qs[which];
	sched_whichqs |= 1 << which;
	l->l_forw = (struct lwp *)q;
	l->l_back = oldlast = q->ph_rlink;
	q->ph_rlink = l;
	oldlast->l_forw = l;
}

/*
 * Remove process p from its run queue, which should be the one
 * indicated by its priority.
 * Calls should be made at splstatclock().
 */
void
remrunqueue(struct lwp *l)
{
	int which = l->l_priority >> 2;
	struct prochd *q;

#ifdef	DIAGNOSTIC	
	if (!(sched_whichqs & (1 << which)))
		panic("remrunqueue");
#endif
	l->l_forw->l_back = l->l_back;
	l->l_back->l_forw = l->l_forw;
	l->l_back = NULL;
	q = &sched_qs[which];
	if (q->ph_link == (struct lwp *)q)
		sched_whichqs &= ~(1 << which);
}

/*
 * Idle
 */
void
idle()
{

#if defined(LOCKDEBUG)
	sched_unlock_idle();
#endif
	spl0();
	while (sched_whichqs == 0)
		continue;
	splhigh();
#if defined(LOCKDEBUG)
	sched_lock_idle();
#endif
}

extern int want_resched; /* XXX should be in <machine/cpu.h> */

/*
 * Find the highest-priority runnable process and switch to it.
 */
int
cpu_switch(struct lwp *l1)
{
	int which;
	struct prochd *q;
	struct lwp *l2;

	/*
	 * We enter here with interrupts blocked and sched_lock held.
	 */

#if 0
	printf("cpu_switch: %p ->", l1);
#endif
	curproc = NULL;
	while (sched_whichqs == 0)
		idle();
	which = ffs(sched_whichqs) - 1;
	q = &sched_qs[which];
	l2 = q->ph_link;
	remrunqueue(l2);
	want_resched = 0;
#ifdef LOCKDEBUG
	sched_unlock_idle();
#endif
	/* p->p_cpu initialized in fork1() for single-processor */
	l2->l_stat = LSONPROC;
	curproc = l2;
#if 0
	printf(" %p\n", l2);
#endif
	if (l2 == l1)
		return (0);
	pmap_deactivate(l1);
	pmap_activate(l2);
	cpu_loswitch(&l1->l_addr->u_pcb.pcb_sf, l2->l_addr->u_pcb.pcb_sf);
	/* We only get back here after the other process has run. */
	return (1);
}

/*
 * Switch to the specified process.
 */
void
cpu_preempt(struct lwp *old, struct lwp *new)
{

	/*
	 * We enter here with interrupts blocked and sched_lock held.
	 */
	remrunqueue(new);
#ifdef LOCKDEBUG
	sched_unlock_idle();
#endif
	/* p->p_cpu initialized in fork1() for single-processor */
	new->l_stat = LSONPROC;
	curproc = new;
	if (new == old)
		return;
	pmap_deactivate(old);
	pmap_activate(new);
	cpu_loswitch(&old->l_addr->u_pcb.pcb_sf, new->l_addr->u_pcb.pcb_sf);
	/* We only get back here after the other process has run. */
}
