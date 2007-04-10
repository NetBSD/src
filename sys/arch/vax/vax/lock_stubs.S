/*	$NetBSD: lock_stubs.S,v 1.2.8.2 2007/04/10 13:23:20 ad Exp $	*/

/*-
 * Copyright (c) 2002, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Andrew Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include <machine/asm.h>
#include "assym.h"

#ifndef LOCKDEBUG
/*
 * void mutex_enter(kmutex_t *);
 *
 *
 */
NENTRY(mutex_enter, 0)
	movl	4(%ap), %r0			/* get mutex */
	bbssi	$31, (%r0), 1f			/* is there an owner? */
	mfpr	$PR_SSP, %r1			/*   Note, get curcpu */
	movl	CI_CURLWP(%r1),(%r0)		/*   set owner to curlwp */
	ret					/*   and return */
1:
	callg	(%ap), _C_LABEL(mutex_vector_enter)
						/* there is an owner */
						/*   so go slow */
	ret

	

/*
 * void mutex_exit(kmutex_t *);
 */
NENTRY(mutex_exit, 0)
	movl	4(%ap), %r0			/* get mutex */
	blbs	MTX_ID(%r0), 1f			/* go slow if this is SPIN */
	mfpr	$PR_SSP, %r1			/* get curcpu */
	cmpl	(%r0),CI_CURLWP(%r1)		/* is the owner still us and */
						/*    no waiters? */
	bneq	1f				/*   no, slow path */
	clrl	(%r0)				/* clear owner */
	ret

1:	callg	(%ap), _C_LABEL(mutex_vector_exit)
	ret

/*
 * void mutex_spin_enter(kmutex_t *);
 */
NENTRY(mutex_spin_enter, 0)
	movl	4(%ap), %r0			/* get spin mutex */
#ifdef DIAGNOSTIC
	blbc	MTX_ID(%r0), 3f
#endif
	mfpr	$PR_IPL, %r2			/* get current IPL */
	movzbl	MTX_IPL(%r0), %r3
	cmpl	%r3, %r2			/* does mutex have > IPL? */
	bleq	1f				/*   no, leave IPL alone */ 
	mtpr	%r3, $PR_IPL			/*   yes, raise IPL */
1:	mfpr	$PR_SSP, %r1			/* get curcpu */
	sobgeq	CI_MTX_COUNT(%r1), 2f		/* decr mutex count */
	brb	3f
2:	movl	%r2, CI_MTX_OLDSPL(%r1)		/*   save was-current IPL */
3:	
#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR)
	bbssi	$0, (%r0), 4f			/* take out mutex */
	ret
4:	callg	(%ap), _C_LABEL(mutex_spin_retry)	/* slow path */
#endif
	ret

/*
 * void mutex_spin_exit(kmutex_t *);
 */
NENTRY(mutex_spin_exit, 0)
	movl	4(%ap), %r0			/* get spin mutex */
#ifdef DIAGNOSTIC
	blbc	MTX_ID(%r0), 2f			/* assert this is a spinlock */
#endif
#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR)
	bbcci	$0, (%r0), 2f			/* clear mutex */
#endif
	mfpr	$PR_SSP, %r1			/* get curcpu */
	movl	CI_MTX_OLDSPL(%r1), %r2		/* fetch oldspl */
	aoblss	$MTX_COUNT_BIAS, CI_MTX_COUNT(%r1), 1f	 /* incr mtx count */
	mtpr	%r2, $PR_IPL			/*   yes, restore saved ipl */
1:	ret	

#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR)
2:	callg	(%ap), _C_LABEL(mutex_vector_exit)	/* slow path */
	ret
#endif

#endif /* LOCKDEBUG */

/*
 *	bool _rw_cas(krwlock_t *rw, uintptr_t old, uintptr_t new);
 *	{
 *		bool rv = false;
 *		int cnt = 4;
 *		while (rw->rw_owner == old && --cnt != 0) {
 *			if (__cpu_simple_try_lock(&rw->rw_lock)) {
 *				if (old == rw->rw_owner) {
 *					rw->rw_owner = new;
 *					rv = true;
 *				}
 *				__cpu_simple_unlock(&rw->rw_lock);
 *				break;
 *			}
 *		}
 *		return rv;
 *	}
 */
NENTRY(_rw_cas, 0)
	clrl	%r0			/* rv = false */
	movl	$4, %r3			/* cnt = 4 */
	movq	4(%ap), %r1		/* cache rw, old */
1:	decl	%r3			/* --cnt */
	beql	2f			/* if cnt == 0, return */
	cmpl	(%r1), %r2		/* rw->rw_owner == old? */
	bneq	2f			/*    nope, return */
	bbssi	$0, RW_LOCK(%r1), 1b	/* get lock, loop if failure */
	cmpl	(%r1), %r2		/* rw->rw_owner still == old? */
	bneq	2f			/*    nope, return */
	movl	12(%ap),(%r1)		/* update rw->rw_owner with new */
	incl	%r0			/* indicate success */
	clrb	RW_LOCK(%r1)		/* release lock */
2:	ret				/* return */
