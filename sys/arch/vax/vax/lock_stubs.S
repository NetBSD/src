/*	$NetBSD: lock_stubs.S,v 1.17.18.1 2014/08/20 00:03:27 tls Exp $	*/

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

/*
 * Bugfix for mutex_spin_enter, which wasn't reentrant.
 *
 * Fixed by Johnny Billquist and Chuck Silvers
 */

#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include <machine/asm.h>
#include "assym.h"

#ifdef MULTIPROCESSOR
	.section	.bss
	.p2align	2
	.lcomm		cashash,256	/* 2048 bits */
#endif

#ifndef LOCKDEBUG
#if MTX_OWNER != 0
#error MTX_OWNER != 0, need to add offset to (%r1)
#endif

/*
 * void mutex_enter(kmutex_t *);
 */
ENTRY(mutex_enter, 0)
	movl	4(%ap), %r1		/* get mutex (ptr) */
#ifdef DIAGNOSTIC
	blbs	(%r1), 1f		/* go slow if this is SPIN */
#endif
	clrl	%r2			/* set old value (zero) */
	mfpr	$PR_SSP, %r3		/* set new value (curlwp) */
#ifndef MULTIPROCESSOR
	addl3	$CI_CAS_ADDR, L_CPU(%r3), %r4 /* r4 == &curcpu()->ci_cas_addr */
#endif
	bsbw	_do_cas+2		/* do the compare-and-swap */
	tstl	%r0			/* is the old value what we wanted? */
	beql	2f			/*  yep, just branch to the return */
1:	callg	(%ap), _C_LABEL(mutex_vector_enter)
					/*  nope, there's an owner so go slow */
2:	ret

/*
 * void mutex_exit(kmutex_t *);
 */
ENTRY(mutex_exit, 0)
	movl	4(%ap), %r1		/* get mutex (ptr) */
#ifdef DIAGNOSTIC
	blbs	(%r1), 1f		/* go slow if this is SPIN */
#endif
	mfpr	$PR_SSP, %r2		/* get curlwp (old) */
	clrl	%r3			/* get zero (new) */
#ifndef MULTIPROCESSOR
	addl3	$CI_CAS_ADDR, L_CPU(%r2), %r4 /* r4 == &curcpu()->ci_cas_addr */
#endif
	bsbw	_do_cas+2		/* do the compare-and-swap */
	cmpl	%r0,%r2			/* return == old? */
	beql	2f			/*   yes, branch to return */
1:	callg	(%ap), _C_LABEL(mutex_vector_exit) /*   no, slow path */
2:	ret

/*
 * void mutex_spin_enter(kmutex_t *);
 */
ENTRY(mutex_spin_enter, 0)
	movl	4(%ap), %r0			/* get spin mutex */
#ifdef DIAGNOSTIC
	blbc	(%r0), 3f
#endif
	mfpr	$PR_IPL, %r2			/* get current IPL */
	movzbl	MTX_IPL(%r0), %r3
	cmpl	%r3, %r2			/* does mutex have > IPL? */
	bleq	1f				/*   no, leave IPL alone */ 
	mtpr	%r3, $PR_IPL			/*   yes, raise IPL */
1:	mfpr	$PR_SSP, %r4			/* get curlwp */
	movl	L_CPU(%r4),%r4			/* get cpu_info */
	decl	CI_MTX_COUNT(%r4)		/* decr muntex count */
	bcc	3f				/* previous active - don't save IPL */
	movl	%r2, CI_MTX_OLDSPL(%r4)		/*   nope, save old IPL */
3:
#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR)
	bbssi	$0, MTX_LOCK(%r0), 4f		/* take out mutex */
	ret
4:	callg	(%ap), _C_LABEL(mutex_spin_retry)	/* slow path */
#else
	movb	$1, MTX_LOCK(%r0)		/* for ddb use only */
#endif
	ret

/*
 * void mutex_spin_exit(kmutex_t *);
 */
ENTRY(mutex_spin_exit, 0)
	movl	4(%ap), %r0			/* get spin mutex */
#if defined(DIAGNOSTIC)
	blbc	(%r0), 2f			/* assert this is a spinlock */
#endif
#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR)
	bbcci	$0, MTX_LOCK(%r0), 2f		/* clear mutex */
#else
	clrb	MTX_LOCK(%r0)			/* for ddb use only */
#endif
	mfpr	$PR_SSP, %r4			/* get curlwp */
	movl	L_CPU(%r4), %r4			/* get cpu_info */
	movl	CI_MTX_OLDSPL(%r4), %r2		/* fetch oldspl */
	incl	CI_MTX_COUNT(%r4)		/* incr mtx count */
	bneq	1f				/* still held? */
	mtpr	%r2, $PR_IPL			/*   no, restore saved ipl */
1:	ret	

#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR)
2:	callg	(%ap), _C_LABEL(mutex_vector_exit)	/* slow path */
	ret
#endif

#if RW_READER != 0
#error RW_READER != 0, change tstl to cmpl $RW_READER
#endif
#if RW_HAS_WAITERS != 1
#error RW_HAS_WAITERS != 1, don't use blbs
#endif
#if RW_OWNER != 0
#error RW_OWNER != 0, need to add to loads
#endif
/*
 * void rw_enter(krwlock_t *rwl, krw_t op);
 */
ENTRY(rw_enter, 0)
	movl	4(%ap), %r1			/* grab rwlock ptr */
	tstl	8(%ap)				/* is this a reader op? */
	bneq	2f				/*   nope, branch to writer */

	movl	(%r1), %r2			/* get owner field */
	bitl	$(RW_WRITE_LOCKED|RW_WRITE_WANTED), %r2
						/* write active or pending? */
	bneq	3f				/*   yep, go slow */
	addl3	$RW_READ_INCR, %r2, %r3		/* incr. reader count (new) */
#ifndef MULTIPROCESSOR
	mfpr	$PR_SSP, %r4
	addl3	$CI_CAS_ADDR, L_CPU(%r4), %r4 /* r4 == &curcpu()->ci_cas_addr */
#endif
1:	bsbw	_do_cas+2			/* do the compare-and-swap */
	cmpl	%r0, %r2			/* did it succeed? */
	bneq	3f				/*   nope, go slow */
	ret					/*   yep, return */

2:	clrl	%r2				/* get old value (zero) */
	mfpr	$PR_SSP, %r3			/* get new value (curlwp) */
#ifndef MULTIPROCESSOR
	addl3	$CI_CAS_ADDR, L_CPU(%r3), %r4 /* r4 == &curcpu()->ci_cas_addr */
#endif
	bisl2	$RW_WRITE_LOCKED, %r3		/* show that it's a write */
	brb	1b				/* do the compare-and-swap */

3:	callg	(%ap), _C_LABEL(rw_vector_enter)
	ret

/*
 * void rw_exit(krwlock_t *rwl, krw_t op);
 */
ENTRY(rw_exit, 0)
	movl	4(%ap), %r1			/* grab rwlock ptr */
	movl	(%r1), %r2			/* grab owner (old) */
	bitl	$RW_WRITE_LOCKED, %r2		/* is it write locked? */
	bneq	2f				/*   yep, do the write case */

	blbs	%r2, 3f				/* RW_HAS_WAITERS mbz */
	subl3	$RW_READ_INCR, %r2, %r3		/* decr. reader count (new) */
	blss	3f				/* if less than 0, go slow */
#ifndef MULTIPROCESSOR
	mfpr	$PR_SSP, %r4			/* get curlwp */
	addl3	$CI_CAS_ADDR, L_CPU(%r4), %r4 /* r4 == &curcpu()->ci_cas_addr */
#endif
1:	bsbw	_do_cas+2			/* do the compare-and-swap */
	cmpl	%r0, %r2			/* did it succeed? */
	bneq	3f				/*   nope, go slow */
	ret					/*   yes, return */

2:	mfpr	$PR_SSP, %r2			/* get old (curlwp) */
#ifndef MULTIPROCESSOR
	addl3	$CI_CAS_ADDR, L_CPU(%r2), %r4 /* r4 == &curcpu()->ci_cas_addr */
#endif
	bisl2	$RW_WRITE_LOCKED, %r2		/* show that it's a write */
	clrl	%r3				/* get new (zero) */
	brb	1b				/* do the compare-and-swap */

3:	callg	(%ap), _C_LABEL(rw_vector_exit)
	ret

/*
 * bool rw_tryenter(krwlock_t *krw, krw_t op);
 */
ENTRY(rw_tryenter, 0)
	movl	4(%ap), %r1			/* get rwlock ptr */
	tstl	8(%ap)				/* is this a reader op? */
	bneq	3f				/*   nope, branch to writer */

	movl	(%r1), %r2			/* get owner field (old) */
	bitl	$(RW_WRITE_LOCKED|RW_WRITE_WANTED), %r2
						/* write active or pending? */
	bneq	2f				/*   yes, return failure */
	addl3	$RW_READ_INCR, %r2, %r3		/* incr reader count (new) */
#ifndef MULTIPROCESSOR
	mfpr	$PR_SSP, %r4
	addl3	$CI_CAS_ADDR, L_CPU(%r4), %r4 /* r4 == &curcpu()->ci_cas_addr */
#endif
1:	bsbw	_do_cas+2			/* do the compare-and-swap */
	cmpl	%r0, %r2			/* did it succeed? */
	bneq	2f				/*   no, we failed. */
	movl	$1,%r0				/*   yes, indicate success */
	ret					/* return */
2:	clrl	%r0				/* indicate failure */
	ret					/* return */

3:	clrl	%r2				/* set old value (0) */
	mfpr	$PR_SSP, %r3			/* set new value (curlwp) */
#ifndef MULTIPROCESSOR
	addl3	$CI_CAS_ADDR, L_CPU(%r3), %r4 /* r4 == &curcpu()->ci_cas_addr */
#endif
	bisl2	$RW_WRITE_LOCKED, %r3		/* show that it's a write */
	brb	1b				/* do the compare-and-swap */
#endif /* LOCKDEBUG */

/*
 * _atomic_cas_32(volatile uint32_t *p, uint32_t old, uint32_t new);
 */
ENTRY(_atomic_cas_32, 0)
	movq	4(%ap), %r1		/* cache ptr, old */
	movl	12(%ap), %r3		/* cache new */
#ifndef MULTIPROCESSOR
	mfpr	$PR_SSP, %r4
	addl3	$CI_CAS_ADDR, L_CPU(%r4), %r4 /* r4 == &curcpu()->ci_cas_addr */
#endif
	bsbw	_do_cas+2		/* do it */
	ret

ENTRY(_do_cas, 0)
#ifdef MULTIPROCESSOR
	movl	(%r1), %r0		/* get value */
	cmpl	%r0, %r2		/* does it equal old? */
	bneq	4f			/*    nope, return */
	extzv	$2,$11,%r1,%r4		/* gets bits 2-12 (our hash) */
	/*
	 * Lock everyone out on this cpu.
	 */
	mfpr	$PR_IPL, %r5		/* save IPL */
	mtpr	$IPL_HIGH, $PR_IPL	/* block everything */
1:	bbssi	%r4,cashash,1b		/* is this pos in the hash table set */
	movl	(%r1), %r0		/* get value again */
	cmpl	%r0, %r2		/* does it still equal old? */
	bneq	2f			/*    nope, return */
	movl	%r3,(%r1)		/* update *ptr with new */
2:
	bbcci	%r4,cashash,3f		/* clear this pos in the hash table */
3:
	mtpr	%r5, $PR_IPL		/* restore IPL */
4:
	rsb				/* return */
#else
/*
 * entry:
 *	r1 = address to be CAS'ed
 *	r2 = old value
 *	r3 = new value
 *	r4 = global cell to hold CAS address (common to all callers)
 *	     e.g. address of curcpu()->ci_cas_addr
 * exit:
 *	r0 = old value
 */
	.globl	cas32_ras_start, cas32_ras_end
cas32_ras_start:
	movl	%r1, (%r4)
	movl	*(%r4), %r0
	cmpl	%r2, %r0
	bneq	1f
	movl	%r3, *(%r4)
cas32_ras_end:
1:
	movl	$CASMAGIC, (%r4)
	rsb
#endif /* !MULTIPROCESSOR */
STRONG_ALIAS(atomic_cas_ptr,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ptr,_atomic_cas_32)
STRONG_ALIAS(atomic_cas_uint,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_uint,_atomic_cas_32)
STRONG_ALIAS(atomic_cas_ulong,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ulong,_atomic_cas_32)
STRONG_ALIAS(atomic_cas_32,_atomic_cas_32)

STRONG_ALIAS(atomic_cas_ptr_ni,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ptr_ni,_atomic_cas_32)
STRONG_ALIAS(atomic_cas_uint_ni,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_uint_ni,_atomic_cas_32)
STRONG_ALIAS(atomic_cas_ulong_ni,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ulong_ni,_atomic_cas_32)
STRONG_ALIAS(atomic_cas_32_ni,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_32_ni,_atomic_cas_32)
