/*	$NetBSD: proc_subr.s,v 1.4.26.1 2002/12/18 05:00:59 gmcgarry Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: locore.s 1.66 92/12/22$
 *
 *	@(#)locore.s	8.6 (Berkeley) 5/27/94
 */

/*
 * Assembly routines related to process manipulation.
 */

/*
 * NOTICE: This is not a standalone file.  To use it, #include it in
 * your port's locore.s, like so:
 *
 *	#include <m68k/m68k/proc_subr.s>
 */

/*
 * The following primitives manipulate the run queues.  _whichqs tells which
 * of the 32 queues _qs have processes in them.  Setrunqueue puts processes
 * into queues, remrunqueue removes them from queues.  The running process is
 * on no queue, other processes are on a queue related to p->p_priority,
 * divided by 4 actually to shrink the 0-127 range of priorities into the 32
 * available queues.
 */

/*
 * Setrunqueue(p)
 *
 * Call should be made at spl6(), and p->p_stat should be SRUN
 */
ENTRY(setrunqueue)
	movl	%sp@(4),%a0
#ifdef DIAGNOSTIC
	tstl	%a0@(P_BACK)
	jne	Lset1
	tstl	%a0@(P_WCHAN)
	jne	Lset1
	cmpb	#SRUN,%a0@(P_STAT)
	jne	Lset1
#endif
	clrl	%d0
	movb	%a0@(P_PRIORITY),%d0
	lsrb	#2,%d0
	movl	_C_LABEL(sched_whichqs),%d1
	bset	%d0,%d1
	movl	%d1,_C_LABEL(sched_whichqs)
	lslb	#3,%d0
	addl	#_C_LABEL(sched_qs),%d0
	movl	%d0,%a0@(P_FORW)
	movl	%d0,%a1
	movl	%a1@(P_BACK),%a0@(P_BACK)
	movl	%a0,%a1@(P_BACK)
	movl	%a0@(P_BACK),%a1
	movl	%a0,%a1@(P_FORW)
	rts
#ifdef DIAGNOSTIC
Lset1:
	PANIC("setrunqueue")
#endif

/*
 * remrunqueue(p)
 *
 * Call should be made at spl6().
 */
ENTRY(remrunqueue)
	movl	%sp@(4),%a0
	movb	%a0@(P_PRIORITY),%d0
#ifdef DIAGNOSTIC
	lsrb	#2,%d0
	movl	_C_LABEL(sched_whichqs),%d1
	btst	%d0,%d1
	jeq	Lrem2
#endif
	movl	%a0@(P_BACK),%a1
	clrl	%a0@(P_BACK)
	movl	%a0@(P_FORW),%a0
	movl	%a0,%a1@(P_FORW)
	movl	%a1,%a0@(P_BACK)
	cmpal	%a0,%a1
	jne	Lrem1
#ifndef DIAGNOSTIC
	lsrb	#2,%d0
	movl	_C_LABEL(sched_whichqs),%d1
#endif
	bclr	%d0,%d1
	movl	%d1,_C_LABEL(sched_whichqs)
Lrem1:
	rts
#ifdef DIAGNOSTIC
Lrem2:
	PANIC("remrunqueue")
#endif

/*
 * struct proc *nextrunqueue(void);
 */
ENTRY(nextrunqueue)
	movl	#0,%a0			| default to none found

	movl	_C_LABEL(sched_whichqs),%d0
	tstl	%d0
	jeq	2f

	movl	%d0,%d1
	negl	%d0
	andl	%d1,%d0
	bfffo	%d0{#0:#32},%d1
	eorib	#31,%d1

	movl	%d1,%d0
	lslb	#3,%d1			| convert queue number to index
	addl	#_C_LABEL(sched_qs),%d1	| locate queue (q)
	movl	%d1,%a1

	movl	%a1@(P_FORW),%a0	| p = q->p_forw
#ifdef DIAGNOSTIC
	cmpal	%d1,%a0			| anyone on queue?
	jeq	_C_LABEL(nextrunqueue_error)	| no, panic
#endif
	movl	%a0@(P_FORW),%a1@(P_FORW)	| q->p_forw = p->p_forw
	movl	%a0@(P_FORW),%a1	| n = p->p_forw
	movl	%d1,%a1@(P_BACK)	| n->p_back = q
	clrl	%a0@(P_BACK)		| clear back link

	cmpal	%d1,%a1			| anyone left on queue?
	jne	1f			| yes, skip

	movl	_C_LABEL(sched_whichqs),%d1
	bclr	%d0,%d1			| no, clear bit
	movl	%d1,_C_LABEL(sched_whichqs)
1:
#ifdef DIAGNOSTIC
	tstl	%a0@(P_WCHAN)
	jne	_C_LABEL(nextrunqueue_error)
	cmpb	#SRUN,%a0@(P_STAT)
	jne	_C_LABEL(nextrunqueue_error)
#endif
2:
	rts

ASENTRY(nextrunqueue_error)
	PANIC("nextrunqueue");
	/*NOTREACHED*/
