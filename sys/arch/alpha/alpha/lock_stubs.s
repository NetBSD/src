/*	$NetBSD: lock_stubs.s,v 1.2.12.1 2007/04/18 06:48:29 thorpej Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: lock_stubs.s,v 1.2.12.1 2007/04/18 06:48:29 thorpej Exp $");

#include "assym.h"

#if defined(MULTIPROCESSOR)
#define	MB		mb
#else
#define	MB		/* nothing */
#endif

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
