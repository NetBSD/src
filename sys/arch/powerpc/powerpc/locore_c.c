/*	$NetBSD: locore_c.c,v 1.2 2003/07/15 02:54:48 lukem Exp $	*/

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

/*
 * Some additional routines that happened to be in locore.S traditionally,
 * but have no need to be coded in assembly.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: locore_c.c,v 1.2 2003/07/15 02:54:48 lukem Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/systm.h>

/*
 * Put process p on the run queue indicated by its priority.
 * Calls should be made at splstatclock(), and p->p_stat should be SRUN.
 */
void
setrunqueue(l)
	struct lwp *l;
{
	struct  prochd *q;
	struct lwp *oldlast;
	int which = l->l_priority >> 2;
	
#ifdef	DIAGNOSTIC
	if (l->l_back)
		panic("setrunqueue");
#endif
	q = &sched_qs[which];
	sched_whichqs |= 0x80000000 >> which;
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
remrunqueue(l)
	struct lwp *l;
{
	int which = l->l_priority >> 2;
	struct prochd *q;

#ifdef	DIAGNOSTIC	
	if (!(sched_whichqs & (0x80000000 >> which)))
		panic("remrunqueue");
#endif
	l->l_forw->l_back = l->l_back;
	l->l_back->l_forw = l->l_forw;
	l->l_back = NULL;
	q = &sched_qs[which];
	if (q->ph_link == (struct lwp *)q)
		sched_whichqs &= ~(0x80000000 >> which);
}
