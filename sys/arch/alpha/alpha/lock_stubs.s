/*	$NetBSD: lock_stubs.s,v 1.3 2008/04/28 20:23:10 martin Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

__KERNEL_RCSID(0, "$NetBSD: lock_stubs.s,v 1.3 2008/04/28 20:23:10 martin Exp $");

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
	GET_CURLWP
1:
	ldq	t1, 0(v0)
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
	GET_CURLWP
	ldq	t1, 0(v0)
	mov	zero, t3
1:
	ldq_l	t2, 0(a0)
	cmpeq	t1, t2, t2
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

#endif	/* !LOCKDEBUG */
