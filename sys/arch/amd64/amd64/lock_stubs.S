/*	$NetBSD: lock_stubs.S,v 1.1.2.5 2006/11/17 16:34:32 ad Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
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

/*
 * AMD64 lock stubs.  Calling convention:
 *
 * %rdi		arg 1
 * %rsi		arg 2
 * %rdx		arg 3
 * %rax		return value
 */

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"

#include <machine/asm.h>
#include <machine/intrdefs.h>

#include "assym.h"

#undef	_ALIGN_TEXT
#define	_ALIGN_TEXT	.align 32

#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
#define	FULL
#endif

#if defined(MULTIPROCESSOR)
#define	LOCK		lock
#else
#define	LOCK		/* nothing */
#endif

/*
 * int _lock_cas(uintptr_t *val, uintptr_t old, uintptr_t new);
 */
NENTRY(_lock_cas)
	movq	%rsi, %rax
	LOCK
	cmpxchgq %rdx, (%rdi)
	movq	$0, %rax
	setz	%al				/* = 1 if success */
	ret

/*
 * int _lock_set_waiters(uintptr_t *val, uintptr_t need, uintptr_t set);
 *
 * Note: 'need' must be non-zero.
 */
NENTRY(_lock_set_waiters)
	movq	(%rdi), %rax

1:	testq	%rsi, %rax
	jz	2f
	orq	%rax, %rdx
	LOCK
	cmpxchgq %rdx, (%rdi)
	jnz	1b
	ret					/* x86 spec, %eax is nonzero */

2:	xorq	%rax, %rax
	ret

#ifndef LOCKDEBUG

/*
 * void mutex_enter(kmutex_t *mtx);
 */
NENTRY(mutex_enter)
	testb	$MUTEX_BIT_SPIN, MTX_FLAGS(%rdi)/* spin mutex? */
	jz	2f				/* no: branch */

	/*
	 * Spin
	 */
	movzbl	MTX_MINSPL(%rdi), %ecx		/* new SPL */
	movl	CPUVAR(ILEVEL), %esi		/* %rsi for vector_enter */
	cmpl	%ecx, %esi			/* higher? */
	cmovgl	%esi, %ecx
	movl	%ecx, CPUVAR(ILEVEL)		/* splraiseipl() */

#if defined(FULL)
	movl	$0x0301, %eax			/* new + expected value */
	LOCK
	cmpxchgb %ah, MTX_FLAGS(%rdi)		/* lock */
	jnz	_C_LABEL(mutex_vector_enter)	/* failed; hard case */
#endif
	subl	$1, CPUVAR(MTX_COUNT)		/* decl doesnt set CF */
	cmovncl	CPUVAR(MTX_OLDSPL), %esi
	movl	%esi, CPUVAR(MTX_OLDSPL)
	ret

	/*
	 * Adaptive
	 */
2:	movq	CPUVAR(CURLWP), %rcx
	xorq	%rax, %rax
	LOCK
	cmpxchgq %rcx, MTX_OWNER(%rdi)
	jnz	_C_LABEL(mutex_vector_enter)
	ret

/*
 * void mutex_exit(kmutex_t *mtx);
 */
NENTRY(mutex_exit)
	testb	$MUTEX_BIT_SPIN, MTX_FLAGS(%rdi)/* spin mutex? */
	jz	2f				/* no: branch */

	/*
	 * Spin
	 */
#if defined(FULL)
	movl	$0x0103, %eax			/* new + expected value */
	cmpxchgb %ah, MTX_FLAGS(%rdi)		/* unlock */
	jnz	_C_LABEL(mutex_vector_exit)	/* hard case if problems */
#endif
	movl	CPUVAR(MTX_OLDSPL), %ecx
	incl	CPUVAR(MTX_COUNT)
	jnz	1f
	SPLLOWER(_C_LABEL(Xspllower))		/* splx */
1:	ret

	/*
	 * Adaptive
	 */
2:	movq	CPUVAR(CURLWP), %rax
	xorq	%rcx, %rcx
	LOCK
	cmpxchgq %rcx, MTX_OWNER(%rdi)
	jnz     _C_LABEL(mutex_vector_exit)
	ret

/*
 * void	rw_enter(krwlock_t *rwl, krw_t op);
 */
NENTRY(rw_enter)
	cmpl	$RW_READER, %esi
	movq	RW_OWNER(%rdi), %rax
	jne	2f

	/*
	 * Reader: this is the most common case.
	 */
1:	testb	$(RW_WRITE_LOCKED|RW_WRITE_WANTED), %al
	leaq	RW_READ_INCR(%rax), %rdx 
	jnz	_C_LABEL(rw_vector_enter)
	LOCK
	cmpxchgq %rdx, RW_OWNER(%rdi)
	jnz	1b
	ret

	/*
	 * Writer: if the compare-and-set fails, don't bother retrying.
	 */
2:	testq	%rax, %rax
	movq	%rax, %rcx
	jnz	_C_LABEL(rw_vector_enter)
	addq	CPUVAR(CURLWP), %rcx
	leaq	RW_WRITE_LOCKED(%rcx), %rcx
	jz	_C_LABEL(rw_vector_enter)
	LOCK
	cmpxchgq %rcx, RW_OWNER(%rdi)
	jnz	_C_LABEL(rw_vector_enter)
	ret

/*
 * void	rw_exit(krwlock_t *rwl);
 */
NENTRY(rw_exit)
	movq	RW_OWNER(%rdi), %rax
	testb	$RW_WRITE_LOCKED, %al
	jnz	2f

	/*
	 * Reader
	 */
1:	testb	$RW_HAS_WAITERS, %al
	movq	$RW_READER, %rsi
	jnz	_C_LABEL(rw_vector_exit)
	testq	$RW_THREAD, %rax
	leaq	-RW_READ_INCR(%rax), %rdx
	jz	_C_LABEL(rw_vector_exit)
	LOCK
	cmpxchgq %rdx, RW_OWNER(%rdi)
	jnz	1b
	ret

	/*
	 * Writer
	 */
2:	leaq	-RW_WRITE_LOCKED(%rax), %rdx
	subq	CPUVAR(CURLWP), %rdx
	movq	$RW_WRITER, %rsi
	jnz	_C_LABEL(rw_vector_exit)
	LOCK
	cmpxchgq %rdx, RW_OWNER(%rdi)
	jnz	_C_LABEL(rw_vector_exit)
	ret

#endif	/* LOCKDEBUG */
