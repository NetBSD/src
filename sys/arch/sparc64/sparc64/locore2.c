/*	$NetBSD: locore2.c,v 1.2.12.5 2002/05/10 16:55:57 petrov Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)locore2.c	8.4 (Berkeley) 12/10/93
 */

/*
 * Primitives which are in locore.s on other machines,
 * but which have no reason to be assembly-coded on SPARC.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/resourcevar.h>

#include <machine/cpu.h>

/*
 * Put lwp l on the run queue indicated by its priority.
 * Calls should be made at splstatclock(), and l->l_stat should be LSRUN.
 */
void
setrunqueue(l)
	register struct lwp *l;
{
	register struct prochd *q;
	register struct lwp *oldlast;
	register int which = l->l_priority >> 2;

	if (l->l_back != NULL)
		panic("setrunqueue");
	q = &sched_qs[which];
	sched_whichqs |= 1 << which;
	l->l_forw = (struct lwp *)q;
	l->l_back = oldlast = q->ph_rlink;
	q->ph_rlink = l;
	oldlast->l_forw = l;
}

/*
 * Remove lwp l from its run queue, which should be the one
 * indicated by its priority.  Calls should be made at splstatclock().
 */
void
remrunqueue(l)
	register struct lwp *l;
{
	register int which = l->l_priority >> 2;
	register struct prochd *q;

	if ((sched_whichqs & (1 << which)) == 0)
		panic("remrq");
	l->l_forw->l_back = l->l_back;
	l->l_back->l_forw = l->l_forw;
	l->l_back = NULL;
	q = &sched_qs[which];
	if (q->ph_link == (struct lwp *)q)
		sched_whichqs &= ~(1 << which);
}
