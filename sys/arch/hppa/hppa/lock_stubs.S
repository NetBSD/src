/*	$NetBSD: lock_stubs.S,v 1.6.6.1 2007/07/15 13:16:06 ad Exp $	*/

/*-
 * Copyright (c) 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran and Nick Hudson.
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

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_cputype.h"
#include "opt_ddb.h"

#define __MUTEX_PRIVATE

#include <machine/asm.h>
#include <machine/mutex.h>

#include "assym.h"

/*
 * int _lock_cas(uintptr_t *val, uintptr_t old, uintptr_t new);
 *
 * Perform an atomic compare-and-set operation.
 *
 * On single CPU systems, this can use a restartable sequence:
 * there we don't need the overhead of interlocking.
 */

#ifndef MULTIPROCESSOR

	.global _lock_cas_ras_start
	.global _lock_cas_ras_end

	.global mutex_enter_crit_start
	.global mutex_enter_crit_end

	.import	curlwp, data
	.import mutex_vector_enter, code
	.import mutex_vector_exit, code
	.import mutex_wakeup, code

LEAF_ENTRY(_lock_cas)
_lock_cas_ras_start:
	ldw	0(%arg0),%t1
	comb,<>	%arg1, %t1, 1f
	 ldi	0,%ret0
_lock_cas_ras_end:
	stw	%arg2,0(%arg0)
	ldi	1,%ret0
1:
	bv,n	%r0(%rp)

EXIT(_lock_cas)

#ifndef LOCKDEBUG
/*
 * void mutex_exit(kmutex_t *mtx);
 */

LEAF_ENTRY(mutex_exit)
	/*
	 * If its a spin mutex or unowned, we have to take the slow path.
	 */
	ldi	MUTEX_ADAPTIVE_UNOWNED,%t1
	ldw	MTX_OWNER(%arg0),%t2
	depi	0,27,1,%t2			/* bit27 = 0 */
	comb,=	%t1,%t2,.Lexit_slowpath
	 nop

	/*
	 * We know that it's an adapative mutex.  Clear the owners
	 * field and release the lock.
	 */

	ldi	1,%t2				/* unlocked = 1 */
	stw	%t1,MTX_OWNER(%arg0)
	stw	%t2,MTX_LOCK(%arg0)
	sync

	/*	
	 * We have posted a read memory barrier so the check of mtx_waiters
	 * will happen in sequence.  If it's set then trap into mutex_wakeup()
	 * to wake up any threads sleeping on the lock.
	 */
	ldb	MTX_WAITERS(%arg0),%t4
	comib,=	0,%t4,.Lexit_done
	 nop

	ldil	L%mutex_wakeup, %t1
	ldo	R%mutex_wakeup(%t1), %t1
	.call
	bv,n	%r0(%t1)

.Lexit_slowpath:
	ldil	L%mutex_vector_exit, %t1
	ldo	R%mutex_vector_exit(%t1), %t1
	.call
	bv,n	%r0(%t1)
.Lexit_done:
	bv,n	%r0(%rp)

EXIT(mutex_exit)

/*
 * void mutex_enter(kmutex_t *mtx)
 */

LEAF_ENTRY(mutex_enter)

	/*
	 * It might be a spin lock, or might be already owned.
	 * We short circut the request and go straight into
	 * mutex_vector_enter() if the owners field is not clear.
	 */

	ldi	MUTEX_ADAPTIVE_UNOWNED,%t1
	ldw	MTX_OWNER(%arg0),%t2
	comb,=,n %t1,%t2,.Lmutexunowned
.Lenter_slowpath:

	ldil	L%mutex_vector_enter, %t1
	ldo	R%mutex_vector_enter(%t1), %t1
	.call
	bv,n	%r0(%t1)
	 nop

	/*
	 * We now know that it's an adaptive mutex.  Grab the spin
	 * lock, which is an atomic operation.  Once we have that,
	 * we can set the owner field.  If we can't get it, we
	 * need to go the slow path.
	 *
	 * Even if we are preempted between acquiring the lock and
	 * setting the owners field, there is no problem - noone
	 * else can acquire the mutex while the lock is held.
	 */
.Lmutexunowned:
	ldcw	MTX_LOCK(%arg0), %ret0

mutex_enter_crit_start:
	comib,=	0,%ret0,.Lenter_slowpath

	ldil	L%curlwp, %t1
	ldw	R%curlwp(%t1), %t2

	bv	%r0(%rp)
mutex_enter_crit_end:
	 stw	%t2,MTX_OWNER(%arg0)
EXIT(mutex_enter)

#endif	/* !LOCKDEBUG */

#endif /* !MULTIPROCESSOR */
