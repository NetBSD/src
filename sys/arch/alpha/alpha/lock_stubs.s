/*	$NetBSD: lock_stubs.s,v 1.5 2021/07/11 01:58:41 thorpej Exp $	*/

/*-
 * Copyright (c) 2007, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran, and by Jason R. Thorpe.
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

#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

#include <machine/asm.h>

__KERNEL_RCSID(0, "$NetBSD: lock_stubs.s,v 1.5 2021/07/11 01:58:41 thorpej Exp $");

#include "assym.h"

#if defined(MULTIPROCESSOR)
#define	MB		mb
#else
#define	MB		/* nothing */
#endif

/*
 * int	_lock_cas(uintptr_t *ptr, uintptr_t old, uintptr_t new)
 */
LEAF(_lock_cas, 3)
1:
	mov	a2, v0
	ldq_l	t1, 0(a0)
	cmpeq	t1, a1, t1
	beq	t1, 2f
	stq_c	v0, 0(a0)
	beq	v0, 3f
	MB	
	RET
2:
	mov	zero, v0
	MB
	RET
3:
	br	1b
	END(_lock_cas)

#if !defined(LOCKDEBUG)

/*
 * void mutex_enter(kmutex_t *mtx);
 */
LEAF(mutex_enter, 1)
	LDGP(pv)
	GET_CURLWP	/* Note: GET_CURLWP clobbers v0, t0, t8...t11. */
1:
	mov	v0, t1
	ldq_l	t2, 0(a0)
	bne	t2, 2f
	stq_c	t1, 0(a0)
	beq	t1, 3f
	MB
	RET
2:
	lda	t12, mutex_vector_enter
	jmp	(t12)
3:
	br	1b
	END(mutex_enter)

/*
 * void mutex_exit(kmutex_t *mtx);
 */
LEAF(mutex_exit, 1)
	LDGP(pv)
	MB
	GET_CURLWP	/* Note: GET_CURLWP clobbers v0, t0, t8...t11. */
	mov	zero, t3
1:
	ldq_l	t2, 0(a0)
	cmpeq	v0, t2, t2
	beq	t2, 2f
	stq_c	t3, 0(a0)
	beq	t3, 3f
	RET
2:
	lda	t12, mutex_vector_exit
	jmp	(t12)
3:
	br	1b
	END(mutex_exit)

/*
 * void rw_enter(krwlock_t *rwl, krw_t op);
 *
 * Acquire one hold on a RW lock.
 */
LEAF(rw_enter, 2)
	LDGP(pv)

	/*
	 * RW_READER == 0 (we have a compile-time assert in machdep.c
	 * to ensure this).
	 *
	 * Acquire for read is the most common case.
	 */
	bne	a1, 3f

	/* Acquiring for read. */
1:	ldq_l	t0, 0(a0)
	and	t0, (RW_WRITE_LOCKED|RW_WRITE_WANTED), t1
	addq	t0, RW_READ_INCR, t2
	bne	t1, 4f		/* contended */
	stq_c	t2, 0(a0)
	beq	t2, 2f		/* STQ_C failed; retry */
	MB
	RET

2:	br	1b

3:	/* Acquiring for write. */
	GET_CURLWP	/* Note: GET_CURLWP clobbers v0, t0, t8...t11. */
	ldq_l	t0, 0(a0)
	or	v0, RW_WRITE_LOCKED, t2
	bne	t0, 4f		/* contended */
	stq_c	t2, 0(a0)
	beq	t2, 4f		/* STQ_C failed; consider it contended */
	MB
	RET

4:	lda	pv, rw_vector_enter
	jmp	(pv)
	END(rw_enter)

/*
 * int rw_tryenter(krwlock_t *rwl, krw_t op);
 *
 * Try to acquire one hold on a RW lock.
 */
LEAF(rw_tryenter, 2)
	LDGP(pv)

	/* See above. */
	bne	a1, 3f

	/* Acquiring for read. */
1:	ldq_l	t0, 0(a0)
	and	t0, (RW_WRITE_LOCKED|RW_WRITE_WANTED), t1
	addq	t0, RW_READ_INCR, v0
	bne	t1, 4f		/* contended */
	stq_c	v0, 0(a0)
	beq	v0, 2f		/* STQ_C failed; retry */
	MB
	RET			/* v0 contains non-zero LOCK_FLAG from STQ_C */

2:	br	1b

	/* Acquiring for write. */
3:	GET_CURLWP	/* Note: GET_CURLWP clobbers v0, t0, t8...t11. */
	ldq_l	t0, 0(a0)
	or	v0, RW_WRITE_LOCKED, v0
	bne	t0, 4f		/* contended */
	stq_c	v0, 0(a0)
	/*
	 * v0 now contains the LOCK_FLAG value from STQ_C, which is either
	 * 0 for failure, or non-zero for success.  In either case, v0's
	 * value is correct.  Go ahead and perform the memory barrier even
	 * in the failure case because we expect it to be rare and it saves
	 * a branch-not-taken instruction in the success case.
	 */
	MB
	RET

4:	mov	zero, v0	/* return 0 (failure) */
	RET
	END(rw_tryenter)

/*
 * void rw_exit(krwlock_t *rwl);
 *
 * Release one hold on a RW lock.
 */
LEAF(rw_exit, 1)
	LDGP(pv)
	MB

	/*
	 * Check for write-lock release, and get the owner/count field
	 * on its own for sanity-checking against expected values.
	 */
	ldq	a1, 0(a0)
	and	a1, RW_WRITE_LOCKED, t1
	srl	a1, RW_READ_COUNT_SHIFT, a2
	bne	t1, 3f

	/*
	 * Releasing a read-lock.  Make sure the count is non-zero.
	 * If it is zero, take the slow path where the juicy diagnostic
	 * checks are located.
	 */
	beq	a2, 4f

	/*
	 * We do the following trick to check to see if we're releasing
	 * the last read-count and there are waiters:
	 *
	 *	1. Set v0 to 1.
	 *	2. Shift the new read count into t1.
	 *	3. Conditally move t1 to v0 based on low-bit-set of t0
	 *	   (RW_HAS_WAITERS).  If RW_HAS_WAITERS is not set, then
	 *	   the move will not take place, and v0 will remain 1.
	 *	   Otherwise, v0 will contain the updated read count.
	 *	4. Jump to slow path if v0 == 0.
	 */
1:	ldq_l	t0, 0(a0)
	ldiq	v0, 1
	subq	t0, RW_READ_INCR, t2
	srl	t2, RW_READ_COUNT_SHIFT, t1
	cmovlbs	t0, t1, v0
	beq	v0, 4f
	stq_c	t2, 0(a0)
	beq	t2, 2f		/* STQ_C failed; try again */
	RET

2:	br	1b

	/*
	 * Releasing a write-lock.  Make sure the owner field points
	 * to our LWP.  If it does not, take the slow path where the
	 * juicy diagnostic checks are located.  a2 contains the owner
	 * field shifted down.  Shift it back up to compare to curlwp;
	 * this conveniently discards the bits we don't want to compare.
	 */
3:	GET_CURLWP	/* Note: GET_CURLWP clobbers v0, t0, t8...t11. */
	sll	a2, RW_READ_COUNT_SHIFT, a2
	mov	zero, t2	/* fast-path write-unlock stores NULL */
	cmpeq	v0, a2, v0	/* v0 = (owner == curlwp) */
	ldq_l	t0, 0(a0)
	beq	v0, 4f		/* owner field mismatch; need slow path */
	blbs	t0, 4f		/* RW_HAS_WAITERS set; need slow-path */
	stq_c	t2, 0(a0)
	beq	t2, 4f		/* STQ_C failed; need slow-path */
	RET

4:	lda	pv, rw_vector_exit
	jmp	(pv)
	END(rw_exit)

#endif	/* !LOCKDEBUG */
