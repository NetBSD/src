/*	$NetBSD: Locore.c,v 1.2 2000/05/26 00:36:44 thorpej Exp $	*/

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

#include <sys/param.h>

__RCSID("$NetBSD: Locore.c,v 1.2 2000/05/26 00:36:44 thorpej Exp $");

#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <vm/vm.h>

#include <machine/machdep.h>

static void idle(void);

volatile int whichqs;

/*
 * Put process p on the run queue indicated by its priority.
 * Calls should be made at splstatclock(), and p->p_stat should be SRUN.
 */
void
setrunqueue(struct proc *p)
{
	struct  prochd *q;
	struct proc *oldlast;
	int which = p->p_priority >> 2;
	
#ifdef	DIAGNOSTIC
	if (p->p_back)
		panic("setrunqueue");
#endif
	q = &qs[which];
	whichqs |= 1 << which;
	p->p_forw = (struct proc *)q;
	p->p_back = oldlast = q->ph_rlink;
	q->ph_rlink = p;
	oldlast->p_forw = p;
}

/*
 * Remove process p from its run queue, which should be the one
 * indicated by its priority.
 * Calls should be made at splstatclock().
 */
void
remrunqueue(struct proc *p)
{
	int which = p->p_priority >> 2;
	struct prochd *q;

#ifdef	DIAGNOSTIC	
	if (!(whichqs & (1 << which)))
		panic("remrunqueue");
#endif
	p->p_forw->p_back = p->p_back;
	p->p_back->p_forw = p->p_forw;
	p->p_back = NULL;
	q = &qs[which];
	if (q->ph_link == (struct proc *)q)
		whichqs &= ~(1 << which);
}

/*
 * Idle
 */
static void
idle()
{

	spl0();
	while (whichqs == 0)
		continue;
	splhigh();
}

extern int want_resched; /* XXX should be in <machine/cpu.h> */

/*
 * Find the highest-priority runnable process and switch to it.
 */
void
cpu_switch(struct proc *p1)
{
	int which;
	int s;
	struct prochd *q;
	struct proc *p2;

#if 0
	printf("cpu_switch: %p ->", p1);
#endif
	curproc = NULL;
	s = splhigh();
	while (whichqs == 0)
		idle();
	which = ffs(whichqs) - 1;
	q = &qs[which];
	p2 = q->ph_link;
	remrunqueue(p2);
	want_resched = 0;
	p2->p_stat = SONPROC;
	curproc = p2;
#if 0
	printf(" %p\n", p2);
#endif
	if (p2 == p1)
		return;
	pmap_deactivate(p1);
	pmap_activate(p2);
	cpu_loswitch(&p1->p_addr->u_pcb.pcb_sf, p2->p_addr->u_pcb.pcb_sf);
	/* We only get back here after the other process has run. */
	splx(s);
}
