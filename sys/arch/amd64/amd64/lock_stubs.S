/*	$NetBSD: lock_stubs.S,v 1.1.2.4 2006/10/20 19:41:41 ad Exp $	*/

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

#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR)
#define	FULL
#endif

#undef	_ALIGN_TEXT
#define	_ALIGN_TEXT	.align 32

/*
 * void	_lock_cas(uintptr_t *val, uintptr_t old, uintptr_t new);
 */
NENTRY(_lock_cas)
	movq	%rsi, %rax
	lock
	cmpxchgq %rdx, (%rdi)
	movq	$0, %rax
	setz	%al
	ret

/*
 * void	_lock_set_waiters(uintptr_t *val, uintptr_t need, uintptr_t set);
 *
 * Note: 'need' must be non-zero.
 */
NENTRY(_lock_set_waiters)
	movq	(%rdi), %rax
1:	testq	%rsi, %rax
	jz	2f
	orq	%rax, %rdx
	lock
	cmpxchgq %rdx, (%rdi)
	jnz	1b

	ret

2:	xorq	%rax, %rax
	ret

#ifndef LOCKDEBUG

/*
 * void mutex_enter(kmutex_t *mtx);
 */
NENTRY(mutex_enter)
	cmpb	$0xff, MTX_ALLONE(%rdi)		/* spin mutex? */
	jne	2f				/* no: branch */

	movzbl	MTX_MINSPL(%rdi), %ecx		/* new SPL */
	movl	CPUVAR(ILEVEL), %esi		/* %rsi for vector_enter */
	cmpl	%ecx, %esi			/* higher? */
	cmovgl	%esi, %ecx
	movl	%ecx, CPUVAR(ILEVEL)		/* splraiseipl() */

#if defined(FULL)
	movl	$0x0100, %eax			/* new + expected value */
	lock
	cmpxchgb %ah, MTX_LOCK(%rdi)		/* lock */
	jnz	_C_LABEL(mutex_vector_enter)	/* failed; hard case */
#endif
	movl	%esi, %eax
	movb	%al, MTX_OLDSPL(%rdi)		/* save old SPL */
	ret

2:	movq	CPUVAR(CURLWP), %rcx
	xorq	%rax, %rax
	lock
	cmpxchgq %rcx, (%rdi)
	jnz	_C_LABEL(mutex_vector_enter)
	ret

/*
 * void mutex_exit(kmutex_t *mtx);
 */
NENTRY(mutex_exit)
	cmpb	$0xff, MTX_ALLONE(%rdi)		/* spin mutex? */
	jne	1f				/* no: branch */

	movzbl	MTX_OLDSPL(%rdi), %ecx		/* for SPLLOWER */
#if defined(FULL)
	movl	$0x0001, %eax			/* new + expected value */
	cmpxchgb %ah, MTX_LOCK(%rdi)		/* unlock */
	jnz	_C_LABEL(mutex_vector_exit)	/* hard case if problems */
#endif
	SPLLOWER(_C_LABEL(Xspllower))		/* splx */
	ret

1:	movq	CPUVAR(CURLWP), %rax
	xorq	%rcx, %rcx
	lock
	cmpxchgq %rcx, (%rdi)
	jnz     _C_LABEL(mutex_vector_exit)
	ret

/*
 * void	rw_enter(krwlock_t *rwl, krw_t op);
 */
NENTRY(rw_enter)
	cmpl	$RW_READER, %esi
	movq	(%rdi), %rax
	jne	2f

1:	testb	$(RW_WRITE_LOCKED|RW_WRITE_WANTED), %al
	leaq	RW_READ_INCR(%rax), %rdx 
	jnz	_C_LABEL(rw_vector_enter)
	lock
	cmpxchgq %rdx, (%rdi)
	jnz	1b
	ret

2:	testq	%rax, %rax
	movq	%rax, %rcx
	jnz	_C_LABEL(rw_vector_enter)
	addq	CPUVAR(CURLWP), %rcx
	leaq	RW_WRITE_LOCKED(%rcx), %rcx
	jz	_C_LABEL(rw_vector_enter)
	lock
	cmpxchgq %rcx, (%rdi)
	jnz	_C_LABEL(rw_vector_enter)
	ret

/*
 * void	rw_exit(krwlock_t *rwl);
 */
NENTRY(rw_exit)
	movq	(%rdi), %rax
	testb	$RW_WRITE_LOCKED, %al
	jnz	2f

1:	testb	$RW_HAS_WAITERS, %al
	movq	$RW_READER, %rsi
	jnz	_C_LABEL(rw_vector_exit)
	testq	$RW_THREAD, %rax
	leaq	-RW_READ_INCR(%rax), %rdx
	jz	_C_LABEL(rw_vector_exit)
	lock
	cmpxchgq %rdx, (%rdi)
	jnz	1b

	ret

2:	leaq	-RW_WRITE_LOCKED(%rax), %rdx
	subq	CPUVAR(CURLWP), %rdx
	movq	$RW_WRITER, %rsi
	jnz	_C_LABEL(rw_vector_exit)
	lock
	cmpxchgq %rdx, (%rdi)
	jnz	_C_LABEL(rw_vector_exit)

	ret

#endif	/* LOCKDEBUG */
