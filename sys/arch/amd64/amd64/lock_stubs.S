/*	$NetBSD: lock_stubs.S,v 1.1.2.6 2006/12/29 20:27:41 ad Exp $	*/

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
 *
 * Perform an atomic compare-and-set operation.
 */
NENTRY(_lock_cas)
	movq	%rsi, %rax
	LOCK
	cmpxchgq %rdx, (%rdi)
	movq	$0, %rax
	setz	%al				/* = 1 if success */
	ret

#ifndef LOCKDEBUG

/*
 * void mutex_enter(kmutex_t *mtx);
 *
 * Acquire a mutex and post a load fence.
 */
NENTRY(mutex_enter)
	movq	CPUVAR(CURLWP), %rcx
	xorq	%rax, %rax
	LOCK
	cmpxchgq %rcx, MTX_OWNER(%rdi)
	jnz	_C_LABEL(mutex_vector_enter)
	ret

/*
 * void mutex_exit(kmutex_t *mtx);
 *
 * Release a mutex and post a load fence.
 *
 * See comments in mutex_vector_enter() about doing this operation unlocked
 * on multiprocessor systems, and comments in arch/x86/include/lock.h about
 * memory ordering on Intel x86 systems.
 */
NENTRY(mutex_exit)
	movq	CPUVAR(CURLWP), %rax
	xorq	%rdx, %rdx
	cmpxchgq %rdx, MTX_OWNER(%rdi)
	jnz     _C_LABEL(mutex_vector_exit)
	ret

/*
 * void smutex_enter(kmutex_t *mtx);
 *
 * Acquire a spin mutex and post a load fence.
 */
NENTRY(smutex_enter)
	movzbl	MTX_MINSPL(%rdi), %ecx		/* new SPL */
	movl	CPUVAR(ILEVEL), %esi		/* %rsi for vector_enter */
	cmpl	%ecx, %esi			/* higher? */
	cmovgl	%esi, %ecx
	movl	%ecx, CPUVAR(ILEVEL)		/* splraiseipl() */

#if defined(FULL)
	movl	$0x0100, %eax			/* new + expected value */
	LOCK
	cmpxchgb %ah, MTX_LOCK(%rdi)		/* lock */
	movq	(%rsp), %rdx			/* for vector_enter */
	jnz	_C_LABEL(smutex_vector_enter)	/* failed; hard case */
#endif

	subl	$1, CPUVAR(MTX_COUNT)		/* decl doesnt set CF */
	cmovncl	CPUVAR(MTX_OLDSPL), %esi
	movl	%esi, CPUVAR(MTX_OLDSPL)
	ret

/*
 * void smutex_exit(kmutex_t *mtx);
 *
 * Release a spin mutex and post a load fence.
 */
NENTRY(smutex_exit)
#if defined(FULL)
	movl	$0x0001, %eax			/* new + expected value */
	cmpxchgb %ah, MTX_LOCK(%rdi)		/* unlock */
	jnz	_C_LABEL(mutex_vector_exit)	/* hard case if problems */
#endif
	incl	CPUVAR(MTX_COUNT)
	movl	CPUVAR(MTX_OLDSPL), %edi
	jnz	1f
	SPLLOWER(_C_LABEL(Xspllower))		/* splx */
1:	ret					/* double ret as branch */
	ret					/* target: see AMD docs */

/*
 * void	rw_enter(krwlock_t *rwl, krw_t op);
 *
 * Acquire one hold on a RW lock.
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
	_ALIGN_TEXT

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
 *
 * Release one hold on a RW lock.
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
	_ALIGN_TEXT

2:	leaq	-RW_WRITE_LOCKED(%rax), %rdx
	subq	CPUVAR(CURLWP), %rdx
	movq	$RW_WRITER, %rsi
	jnz	_C_LABEL(rw_vector_exit)
	LOCK
	cmpxchgq %rdx, RW_OWNER(%rdi)
	jnz	_C_LABEL(rw_vector_exit)
	ret

#endif	/* LOCKDEBUG */

