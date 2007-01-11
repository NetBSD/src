/*	$NetBSD: lock_stubs.s,v 1.1.36.2 2007/01/11 22:22:58 ad Exp $	*/

/*-
 * Copyright (c) 2002, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Andrew Doran.
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

#include <machine/param.h>
#include <machine/asm.h>

#include "assym.h"

#undef CURLWP
#if defined(MULTIPROCESSOR)
#define	CURLWP	(CPUINFO_VA+CI_CURLWP)
#define	MB_READ	membar #LoadLoad
#define	MB_MEM	membar #LoadLoad | #StoreLoad | #LoadStore	
#else
#define	CURLWP	_C_LABEL(curlwp)
#define	MB_READ	/* nothing */
#define	MB_MEM	/* nothing */
#endif

#ifdef __arch64__
#define	CASPTR	casx
#define	LDPTR	ldx
#define	STPTR	stx
#else
#define	CASPTR	cas
#define	LDPTR	ld
#define	STPTR	st
#endif /* __arch64__ */

/*
 * void _lock_cas(uintptr_t *ptr, uintptr_t old, uintptr_t new);
 */
_ENTRY(_C_LABEL(_lock_cas))
	CASPTR	[%o0], %o1, %o2			! compare-and-swap
	MB_MEM
	cmp	%o1, %o2			! expected == actual?
	bne	1f				! nope
	 or	%g0, 1, %o0
	retl
	 nop
1:	retl
	 mov	%g0, %o0

#if !defined(LOCKDEBUG)

/*
 * void mutex_enter(kmutex_t *);
 */
_ENTRY(_C_LABEL(mutex_enter))
	sethi	%hi(CURLWP), %o3
	LDPTR	[%o3 + %lo(CURLWP)], %o3	! current thread
	CASPTR	[%o0], %g0, %o3			! compare-and-swap
	MB_READ
	tst	%o3				! lock was unowned?
	bnz	_C_LABEL(mutex_vector_enter)	! nope, hard case
	 nop
	retl
	 nop

/*
 * void mutex_exit(kmutex_t *);
 *
 * XXX This should use a restartable sequence.  See mutex_vector_enter().
 */
_ENTRY(_C_LABEL(mutex_exit))
	sethi	%hi(CURLWP), %o3
	LDPTR	[%o3 + %lo(CURLWP)], %o3	! current thread
	mov	%g0, %o4			! new value (0)
	MB_MEM
	CASPTR	[%o0], %o3, %o4			! compare-and-swap
	cmp	%o3, %o4			! were they the same?
	bne	_C_LABEL(mutex_vector_exit)	! nope, hard case
	 nop
	retl
	 nop

#endif	/* !LOCKDEBUG */
