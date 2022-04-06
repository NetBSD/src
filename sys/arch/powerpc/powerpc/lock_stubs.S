/*	$NetBSD: lock_stubs.S,v 1.14 2022/04/06 22:47:57 riastradh Exp $	*/

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

#include <machine/asm.h>

#include "assym.h"

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#endif

#if defined(MULTIPROCESSOR)
#define	ISYNC	isync
#define	SYNC	sync
#else
#define	ISYNC	/* nothing */
#define	SYNC	/* nothing */
#endif

	.text

#if __HAVE_MUTEX_STUBS
/*
 * int _lock_cas(uintptr_t *ptr, uintptr_t old, uintptr_t new);
 */
ENTRY(_lock_cas)
1:
	lptrarx	%r10,0,%r3
	cmpptr	%r10,%r4
	bne-	2f
	IBM405_ERRATA77_DCBT(0,%r3)
	stptrcx. %r5,0,%r3
	bne-	1b
	SYNC
	li	%r3,1
	blr
2:
	li	%r3,0
	blr

#if !defined(LOCKDEBUG)
/*
 * void mutex_enter(kmutex_t *);
 */
ENTRY(mutex_enter)
1:
	lptrarx	%r10,0,%r3
	cmpptri	%r10,0
	bne-	2f
	IBM405_ERRATA77_DCBT(0,%r3)
	stptrcx. %r13,0,%r3
	bne-	1b
	ISYNC
	blr
2:
	b	_C_LABEL(mutex_vector_enter)

/*
 * void mutex_exit(kmutex_t *);
 */
ENTRY(mutex_exit)
	SYNC
	li	%r7,0
1:
	lptrarx	%r10,0,%r3
	cmpptr	%r10,%r13
	bne-	2f
	IBM405_ERRATA77_DCBT(0,%r3)
	stptrcx. %r7,0,%r3
	bne-	1b
	blr
2:
	b	_C_LABEL(mutex_vector_exit)

#endif	/* !LOCKDEBUG */

/*
 * void rw_enter(krwlock_t *krw, krw_t op);
 */
#if RW_READ_INCR != 32
#error RW_READ_INCR != 32, clrrXi need fixing
#endif
#if RW_OWNER != 0
#error RW_OWNER != 0, ldptr should be ldptru
#endif

#if __HAVE_RW_STUBS

ENTRY(rw_enter)
	cmpwi	%r3,RW_READER
	bne-	1f

	ldptr	%r9,RW_OWNER(%r3)
	clrrptri %r9,%r9,5		/* clear low 5 bits */
	addi	%r7,%r9,RW_READ_INCR
	b	2f
1:
	li	%r9,0
	ori	%r7,%r13,RW_WRITE_LOCKED

2:	lptrarx	%r10,0,%r3
	cmpptr	%r10,%r9
	bne-	3f
	IBM405_ERRATA77_DCBT(0,%r3)
	stptrcx. %r7,0,%r3
	bne-	2b
	ISYNC
	blr

3:	b	_C_LABEL(rw_vector_enter)

/*
 * bool rw_tryenter(krwlock_t *krw, krw_t op);
 */
ENTRY(rw_tryenter)
	cmpwi	%r3,RW_READER
	bne-	1f

	ldptr	%r9,RW_OWNER(%r3)
	clrrptri %r9,%r9,5		/* clear low 5 bits */
	addi	%r7,%r9,RW_READ_INCR
	b	2f

1:	li	%r9,0
	ori	%r7,%r13,RW_WRITE_LOCKED

2:	lptrarx	%r10,0,%r3
	cmpptr	%r10,%r9
	bne-	3f
	IBM405_ERRATA77_DCBT(0,%r3)
	stptrcx. %r7,0,%r3
	bne-	2b

	ISYNC
	li	%r3,1
	blr

3:	li	%r3,0
	blr

/*
 * void rw_exit(krwlock_t *krw, krw_t op);
 */
ENTRY(rw_exit)
	ldptr	%r9,RW_OWNER(%r3)
	SYNC
	andi.	%r0,%r9,RW_WRITE_LOCKED
	bne-	1f

	clrrptri. %r9,%r9,5		/* clear low 5 bits */
	beq-	3f			/* if 0, no reader, go slow */

	addi	%r7,%r9,-RW_READ_INCR
	b	2f
1:
	li	%r7,0
	ori	%r9,%r13,RW_WRITE_LOCKED

2:	lptrarx	%r10,0,%r3
	cmpptr	%r10,%r9
	bne-	3f
	IBM405_ERRATA77_DCBT(0,%r3)
	stptrcx. %r7,0,%r3
	bne-	2b

	blr

3:	b	_C_LABEL(rw_vector_exit)

#endif	/* __HAVE_RW_STUBS */

#endif	/* __HAVE_MUTEX_STUBS */
