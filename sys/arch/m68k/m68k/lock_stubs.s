/*	$NetBSD: lock_stubs.s,v 1.8 2009/11/11 11:25:52 skrll Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran and Michael Hitch.
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

#include <machine/asm.h>

#include "assym.h"

	.file	"lock_stubs.s"
	.text

#if defined(__mc68010__)
/*
 * int _atomic_cas_32(volatile uint32_t *val, uint32_t old, uint32_t new);
 *
 * The 68010 does not have a cas instruction, so we implement this as
 * a restartable atomic sequence.  For an example of how this is used,
 * see sun68k/sun68k/isr.c
 */
ENTRY(_atomic_cas_32)
	movl	%sp@(4),%a0

	.globl _C_LABEL(_atomic_cas_ras_start)
_C_LABEL(_atomic_cas_ras_start):
	movl	%a0@,%d0
	cmpl	%sp@(8),%d0
	jne	1f
	movl	%sp@(12),%a0@
	.globl	_C_LABEL(_atomic_cas_ras_end)
_C_LABEL(_atomic_cas_ras_end):

1:
	movl	%d0, %a0	/* pointers return also in %a0 */
	rts

STRONG_ALIAS(atomic_cas_ptr,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ptr,_atomic_cas_32)
STRONG_ALIAS(atomic_cas_uint,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_uint,_atomic_cas_32)
STRONG_ALIAS(atomic_cas_ulong,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ulong,_atomic_cas_32)
STRONG_ALIAS(atomic_cas_32,_atomic_cas_32)

STRONG_ALIAS(atomic_cas_32_ni,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_32_ni,_atomic_cas_32)

STRONG_ALIAS(atomic_cas_ptr_ni,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ptr_ni,_atomic_cas_32)
STRONG_ALIAS(atomic_cas_uint_ni,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_uint_ni,_atomic_cas_32)
STRONG_ALIAS(atomic_cas_ulong_ni,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ulong_ni,_atomic_cas_32)
#endif /* __mc68010__ */

#if !defined(LOCKDEBUG)

/*
 * void mutex_enter(kmutex_t *mtx);
 */
ENTRY(mutex_enter)
#if !defined(__mc68010__)
	movq	#0,%d0
	movl	_C_LABEL(curlwp),%d1
	movl	%sp@(4),%a0
	casl	%d0,%d1,%a0@
	bnes	1f
	rts
#endif /* !__mc68010__ */
1:	jra	_C_LABEL(mutex_vector_enter)

/*
 * void mutex_exit(kmutex_t *mtx);
 */
ENTRY(mutex_exit)
#if !defined(__mc68010__)
	movl	_C_LABEL(curlwp),%d0
	movq	#0,%d1
	movl	%sp@(4),%a0
	casl	%d0,%d1,%a0@
	bnes	1f
	rts
#endif /* !__mc68010__ */
1:	jra	_C_LABEL(mutex_vector_exit)

#endif	/* !LOCKDEBUG */
