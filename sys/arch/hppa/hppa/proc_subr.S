/*	$NetBSD: proc_subr.S,v 1.1 2002/06/05 01:04:20 fredette Exp $	*/

/*	$OpenBSD: locore.S,v 1.46 2001/09/20 18:33:03 mickey Exp $	*/

/*
 * Copyright (c) 1998-2001 Michael Shalayeff
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
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Portitions of this file are derived from other sources, see
 * the copyrights and acknowledgements below.
 */
/*
 * Copyright (c) 1990,1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *	Utah $Hdr: locore.s 1.62 94/12/15$
 */
/*
 *  (c) Copyright 1988 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

/*
 * Assembly routines related to process manipulation.
 */     

/*
 * NOTICE: This is not a standalone file.  To use it, #include it in
 * your port's locore.S, like so:
 *
 *      #include <hppa/hppa/proc_subr.s>
 */

	.import	sched_whichqs, data
	.import	sched_qs, data
/*
 * setrunqueue(struct proc *p);
 * Insert a process on the appropriate queue.  Should be called at splclock().
 */
	.align	32
ENTRY(setrunqueue,0)
#ifdef DIAGNOSTIC
	ldw	P_BACK(arg0), t1
	comb,<>,n r0, t1, $setrunqueue_panic
	ldw	P_WCHAN(arg0), t1
	comb,<>,n r0, t1, $setrunqueue_panic
	ldb	P_STAT(arg0), t1
	comib,=,n SRUN, t1, $setrunqueue_ok
$setrunqueue_panic
	copy	arg0, arg1
	ldil	L%panic, r1
	ldil	L%Lsrqpstr, arg0
	ldo	R%panic(r1), r1
	ldo	R%Lsrqpstr(arg0), arg0
	.call
	blr	%r0, rp
	bv,n	%r0(r1)
	nop
Lsrqpstr
	.asciz	"setrunqueue(%p)"
	.align	8
$setrunqueue_ok
#endif

	ldb	P_PRIORITY(arg0), t2
	ldil	L%sched_qs, t4
	extru	t2, 29, 5, t1		/* t1 = (priority / 4); (queue #) */
	ldo	R%sched_qs(t4), t4
	sh3add	t1, t4, t4		/* t4 = &sched_qs[t1]; */
	ldil	L%sched_whichqs, t2
	ldw	R%sched_whichqs(t2), t3
	mtctl	t1, sar
	vdepi	1, 1, t3
	stw	t3, R%sched_whichqs(t2)	/* sched_whichqs |= (1 << t1); */

#if 0
	/* this actually trashes all the regs we use, be advised ;) */
	copy	t1, arg1
	copy	t4, arg2
	ldil	L%printf, r1
	ldil	L%Lsrqfmt, arg0
	ldo	R%printf(r1), r1
	ldo	R%Lsrqfmt(arg0), arg0
	.call
	blr	%r0, rp
	bv,n	%r0(r1)
	nop
#endif
	ldw	P_BACK(t4), t2
	stw	t4, P_FORW(arg0)
	stw	arg0, P_BACK(t4)
	stw	arg0, P_FORW(t2)
	bv	0(rp)
	stw	t2, P_BACK(arg0)
Lsrqfmt
	.asciz	"setrunqueue: bit=%x, sched_qs=%p\n"
	.align	8
EXIT(setrunqueue)

/*
 * remrunqueue(struct proc *p);
 * Remove a process from its queue.  Should be called at splclock().
 */
	.align	32
ENTRY(remrunqueue,0)
	ldb	P_PRIORITY(arg0), t2
	extru	t2, 29, 5, t1		/* t1 = (priority / 4); (queue #) */
	mtsar	t1
	ldil	L%sched_whichqs, t2
	ldw	R%sched_whichqs(t2), t3	/* t3 = sched_whichqs; */

#ifdef DIAGNOSTIC
	bvb,<,n	t3, remrunqueue_ok

Lremrunqueue_panic
	copy	arg0, arg1
	copy	t1, arg2
	ldil	L%panic, r1
	ldil	L%Lrrqpstr, arg0
	ldo	R%panic(r1), r1
	ldo	R%Lrrqpstr(arg0), arg0
	.call
	blr	%r0, rp
	bv,n	%r0(r1)

Lrrqpstr
	.asciz	"remrunqueue(%p), bit=%x"
	.align	8
remrunqueue_ok
#endif
	ldw	P_BACK(arg0), t4
	stw	r0, P_BACK(arg0)
	ldw	P_FORW(arg0), arg0
	stw	arg0, P_FORW(t4)
	stw	t4, P_BACK(arg0)
	comb,<>	t4, arg0, Lqnempty
	nop

	vdepi	0, 1, t3
	stw	t3, R%sched_whichqs(t2)
Lqnempty
	bv	0(rp)
	nop
EXIT(remrunqueue)
