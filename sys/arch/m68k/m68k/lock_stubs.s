/*	$NetBSD: lock_stubs.s,v 1.1.8.1 2007/03/13 16:50:00 ad Exp $	*/

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

#include <machine/asm.h>

#include "assym.h"

#if defined(DIAGNOSTIC)
#define	FULL
#endif

	.file	"lock_stubs.s"
	.text

#if !defined(__mc68010__)
/*
 * int _lock_cas(uintptr_t *val, uintptr_t old, uintptr_t new);
 */
ENTRY(_lock_cas)
	movl	%sp@(4),%a0		| a0 = val address
	movl	%sp@(8),%d0		| d0 = old value
	movl	%sp@(12),%d1		| d1 = new value
	casl	%d0,%d1,%a0@		| compare old, set new
	beqb	1f			| matched and set
	movq	#0,%d0
	rts
1:	movq	#1,%d0
	rts
#else /* __mc68010__ */
/*
 * int _lock_cas(uintptr_t *val, uintptr_t old, uintptr_t new);
 *
 * The 68010 does not have a cas instruction, so we implement this as
 * a restartable atomic sequence.  For an example of how this is used,
 * see sun68k/sun68k/isr.c
 */
ENTRY_NOPROFILE(_lock_cas)
	movl	%sp@(4),%a0

	.globl _C_LABEL(_lock_cas_ras_start)
_C_LABEL(_lock_cas_ras_start):
	movl	%a0@,%d0
	cmpl	%sp@(8),%d0
	jne	1f
	movl	%sp@(12),%a0@
	.globl	_C_LABEL(_lock_cas_ras_end)
_C_LABEL(_lock_cas_ras_end):

	movq	#1,%d0
	rts
1:
	clrl	%d0
	rts
#endif /* ! __mc68010__ */

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
