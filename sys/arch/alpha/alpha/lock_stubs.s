/* $NetBSD: lock_stubs.s,v 1.1.2.5 2002/03/22 00:18:22 thorpej Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

__KERNEL_RCSID(7, "$NetBSD: lock_stubs.s,v 1.1.2.5 2002/03/22 00:18:22 thorpej Exp $")

/*
 * Assembly language lock stubs.  These handle the common ("easy")
 * cases for mutexes.
 */

#include "opt_lockdebug.h"

	.text
inc7:	.stabs	__FILE__,132,0,0,inc7; .loc	1 __LINE__

NESTED_NOPROFILE(mutex_enter, 1, 0, ra, 0, 0)
	LDGP(pv)

	GET_CURPROC
1:	ldq	t0, 0(v0)		/* current thread */
	ldq_l	t1, 0(a0)		/* load lock contents */
	bne	t1, 3f			/* != NULL, hard case */
	stq_c	t0, 0(a0)		/* acquire lock */
	beq	t0, 2f			/* failed, try again */

	mb				/* succeeded */
#if defined(MUTEX_DEBUG)
	stq	ra, MTX_DBG_LOCKED(a0)
#endif
	RET

2:	br	1b

	/* Do a tail-call to mutex_vector_enter() */
3:	jmp	zero, mutex_vector_enter
END(mutex_enter)

NESTED_NOPROFILE(mutex_tryenter, 1, 0, ra, 0, 0)
	LDGP(pv)

	GET_CURPROC
1:	ldq	t0, 0(v0)		/* current thread */
	ldq_l	t1, 0(a0)		/* load lock contents */
	bne	t1, 3f			/* != NULL, hard case */
	stq_c	t0, 0(a0)		/* acquire lock */
	beq	t0, 2f			/* failed, try again */

	mb				/* succeeded */
#if defined(MUTEX_DEBUG)
	stq	ra, MTX_DBG_LOCKED(a0)
#endif
	RET

2:	br	1b

	/* Do a tail-call to mutex_vector_tryenter() */
3:	jmp	zero, mutex_vector_tryenter
END(mutex_tryenter)

/*
 * THIS IS IMPORTANT WHEN WE ADD SUPPORT FOR KERNEL PREEMPTION.
 *
 * There is a critical section within mutex_exit(); if we are
 * preempted, between checking for waiters and releasing the
 * lock, then we must check for waiters again.
 *
 * On some architectures, we could simply disable interrupts
 * (and, thus, preemption) for the short duration of this
 * critical section.
 *
 * On others, where disabling interrupts might be too expensive,
 * a restartable sequence could be used; in the interrupt handler,
 * if the PC is within the critical section, then then PC should
 * be reset to the beginning of the critical section so that the
 * sequence will be restarted when we are resumed.  NOTE: In this
 * case, it is very important that the insn that actually clears
 * the lock must NEVER be executed twice.
 *
 * On the Alpha, we can use load-locked/store-conditional.
 */
NESTED_NOPROFILE(mutex_exit, 1, 0, ra, 0, 0)
	LDGP(pv)
#if !defined(MUTEX_DEBUG)
	GET_CURPROC
	ldq	t0, 0(v0)		/* current thread */

	/* BEGIN CRITICAL SECTION */
1:	mb
	mov	zero, t3
	ldq_l	t1, 0(a0)		/* load lock contents */
	cmpeq	t0, t1, t0		/* are they the same? */
	bfalse	t0, 3f			/* no, hard case */

	stq_c	t3, 0(a0)		/* release lock */
	beq	t3, 2f
	/* END CRITICAL SECTION */

	RET

2:	br	1b
#endif /* ! MUTEX_DEBUG */
	/* Do a tail-call to mutex_vector_exit() */
3:	jmp	zero, mutex_vector_exit
END(mutex_exit)
