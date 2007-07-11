/*	$NetBSD: lock_stubs.S,v 1.2.10.1 2007/07/11 20:01:54 mjf Exp $	*/

/*
 * Copyright (c) 2007 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sh3/asm.h>
#include <sh3/psl.h>

#include "opt_lockdebug.h"


/*
 * LINTSTUB: include <sys/types.h>
 * LINTSTUB: include <sys/mutex.h>
 */


/*
 * LINTSTUB: Func: int _lock_cas(volatile uintptr_t *ptr, uintptr_t old, uintptr_t new);
 *
 *	Atomic compare-and-swap for kernel use.  SuperH machines are
 *	uniprocessor, so we need to be atomic only w.r.t. interrupts.
 *	Implement this as (the only in-kernel) RAS, with restart check
 *	done in ast() on return from interrupt.
 *
 *	Mutex stubs below depend on this function:
 *	    r4 - preserved, passed along to mutex_vector_{enter,exit}
 *	    r7 - preserved, used to save pr (to avoid touching stack)
 *	     T - still reflects cas success status (to avoid another test)
 */
NENTRY(_lock_cas)
ALTENTRY(_lock_cas_ras_start)
	mov.l	@r4, r1
	cmp/eq	r1, r5		! T = (*ptr == old)
	bf	1f
	mov.l	r6, @r4		! *ptr = new
ALTENTRY(_lock_cas_ras_end)
1:	rts
	 movt	r0		! retval = T
	SET_ENTRY_SIZE(_lock_cas)


#if !defined(LOCKDEBUG)

/*
 * LINTSTUB: Func: void mutex_enter(kmutex_t *mtx);
 */
NENTRY(mutex_enter)
	sts	pr, r7		! depend on _lock_cas not clobbering r7
	mov.l	.L_curlwp, r6
	mov	#0, r5
	bsr	_lock_cas	! _lock_cas(&mtx->mtx_owner, 0, curlwp)
	 mov.l	@r6, r6
	!! T bit still indicates if cas was successful
	bf.s	1f		! hard case if cas failed
	 lds	r7, pr
	rts
	 nop

	!! depend on _lock_cas not clobbering mtx in r4
1:	mov.l	.L_mutex_vector_enter, r0
	jmp	@r0
	 nop
	/* NOTREACHED */


/*
 * LINTSTUB: Func: void mutex_exit(kmutex_t *mtx);
 */
NENTRY(mutex_exit)
	sts	pr, r7		! depend on _lock_cas not clobbering r7
	mov.l	.L_curlwp, r5
	mov	#0, r6
	bsr	_lock_cas	! _lock_cas(&mtx->mtx_owner, curlwp, 0)
	 mov.l	@r5, r5
	!! T bit still indicates if cas was successful
	bf.s	1f		! hard case if cas failed
	 lds	r7, pr
	rts
	 nop

	!! depend on _lock_cas not clobbering mtx in r4
1:	mov.l	.L_mutex_vector_exit, r0
	jmp	@r0
	 nop
	/* NOTREACHED */

	.align 2
.L_curlwp:		.long	_C_LABEL(curlwp)
.L_mutex_vector_enter:	.long	_C_LABEL(mutex_vector_enter)
.L_mutex_vector_exit:	.long	_C_LABEL(mutex_vector_exit)

#endif /* !LOCKDEBUG */
