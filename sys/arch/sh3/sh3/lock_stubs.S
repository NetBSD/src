/*	$NetBSD: lock_stubs.S,v 1.3 2007/03/14 01:14:25 uwe Exp $	*/

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

/*
 * LINTSTUB: include <sys/types.h>
 */


/*
 * LINTSTUB: Func: int _lock_cas(volatile uintptr_t *ptr, uintptr_t old, uintptr_t new);
 *
 *	Brute force with splhigh() for now.  Need to be redone as RAS.
 */
NENTRY(_lock_cas)
	mov	#PSL_IMASK, r2	! 0xf0 - gets sign extended
	stc	sr, r3
	extu.b	r2, r2		! undo sign extention
	mov	r3, r7		! preserve SR
	or	r2, r3		! mask all interrupts
	ldc	r3, sr

	mov.l	@r4, r1
	cmp/eq	r1, r5		! T = (*ptr == old)
	bf.s	1f
	 movt	r0		! retval = T
	mov.l	r6, @r4		! *ptr = new
1:
	ldc	r7, sr		! restore SR (cannot be in delay slot)
	rts
	 nop


/*
 * LINTSTUB: Func: void mutex_enter(kmutex_t *mtx);
 */
NENTRY(mutex_enter)
	sts.l	pr, @-sp
	mov.l	.L_curlwp, r6
	mov	#0, r5
	bsr	_lock_cas	! _lock_cas(&mtx->mtx_owner, 0, curlwp)
	 mov.l	@r6, r6
	tst	r0, r0
	bf.s	1f		! return if success
	 lds.l	@sp+, pr
	mov.l	.L_mutex_vector_enter, r0
	jmp	@r0		! otherwise - hard case
	 nop
	/* NOTREACHED */
1:	rts
	 nop


/*
 * LINTSTUB: Func: void mutex_exit(kmutex_t *mtx);
 */
NENTRY(mutex_exit)
	sts.l	pr, @-sp
	mov.l	.L_curlwp, r5
	mov	#0, r6
	bsr	_lock_cas	! _lock_cas(&mtx->mtx_owner, curlwp, 0)
	 mov.l	@r5, r5
	tst	r0, r0
	bf.s	1f		! return if success
	 lds.l	@sp+, pr
	mov.l	.L_mutex_vector_exit, r0
	jmp	@r0		! otherwise - hard case
	 nop
	/* NOTREACHED */
1:	rts
	 nop

	.align 2
.L_curlwp:		.long	_C_LABEL(curlwp)
.L_mutex_vector_enter:	.long	_C_LABEL(mutex_vector_enter)
.L_mutex_vector_exit:	.long	_C_LABEL(mutex_vector_exit)
