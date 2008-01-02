/*	$NetBSD: lock_stubs.S,v 1.10.6.1 2008/01/02 21:46:59 bouyer Exp $	*/

/*-
 * Copyright (c) 2006, 2007 The NetBSD Foundation, Inc.
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
#include <machine/frameasm.h>

#include "assym.h"

#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
#define	FULL
#endif

#define	END(name,a)	.align	a; LABEL(name)
#define	LOCK(num)	.Lpatch/**/num: lock

#ifndef LOCKDEBUG

/*
 * void mutex_enter(kmutex_t *mtx);
 *
 * Acquire a mutex and post a load fence.
 */
	.align	64

NENTRY(mutex_enter)
	movq	CPUVAR(CURLWP), %rcx
	xorq	%rax, %rax
	LOCK(1)
	cmpxchgq %rcx, MTX_OWNER(%rdi)
	jnz,pn	_C_LABEL(mutex_vector_enter)
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
	jnz,pn	_C_LABEL(mutex_vector_exit)
	ret

/*
 * void mutex_spin_enter(kmutex_t *mtx);
 *
 * Acquire a spin mutex and post a load fence.
 */
NENTRY(mutex_spin_enter)
	movq	CPUVAR(SELF), %r8
#if defined(FULL)
	movl	$0x0100, %eax			/* new + expected value */
#endif
	movl	CPU_INFO_ILEVEL(%r8), %esi
	subl	$1, CPU_INFO_MTX_COUNT(%r8)	/* decl doesnt set CF */
	movzbl	MTX_IPL(%rdi), %ecx		/* new SPL */
	cmovncl	CPU_INFO_MTX_OLDSPL(%r8), %esi
	cmpl	%ecx, %esi			/* higher? */
	movl	%esi, CPU_INFO_MTX_OLDSPL(%r8)
	cmovgl	%esi, %ecx
	movl	%ecx, CPU_INFO_ILEVEL(%r8)	/* splraiseipl() */
#if defined(FULL)
	LOCK(11)
	cmpxchgb %ah, MTX_LOCK(%rdi)		/* lock */
	jnz,pn	_C_LABEL(mutex_spin_retry)	/* failed; hard case */
#endif
	ret

/*
 * void mutex_spin_exit(kmutex_t *mtx);
 *
 * Release a spin mutex and post a load fence.
 */
NENTRY(mutex_spin_exit)
#ifdef DIAGNOSTIC

	movl	$0x0001, %eax			/* new + expected value */
	movq	CPUVAR(SELF), %r8
	cmpxchgb %ah, MTX_LOCK(%rdi)		/* unlock */
	jnz,pn	_C_LABEL(mutex_vector_exit)	/* hard case if problems */
	movl	CPU_INFO_MTX_OLDSPL(%r8), %edi
	incl	CPU_INFO_MTX_COUNT(%r8)
	jnz	1f
	cmpl	CPU_INFO_ILEVEL(%r8), %edi
	jae	1f
	movl	CPU_INFO_IUNMASK(%r8,%rdi,4), %esi
	CLI(ax)
	testl	CPU_INFO_IPENDING(%r8), %esi
	jnz	_C_LABEL(Xspllower)
	movl	%edi, CPU_INFO_ILEVEL(%r8)
	STI(ax)
1:	rep					/* double byte ret as branch */
	ret					/* target: see AMD docs */

#else	/* DIAGNOSTIC */

	movq	CPUVAR(SELF), %rsi
#ifdef MULTIPROCESSOR
	movb	$0x00, MTX_LOCK(%rdi)
#endif
	movl	CPU_INFO_MTX_OLDSPL(%rsi), %ecx
	incl	CPU_INFO_MTX_COUNT(%rsi)
	movl	CPU_INFO_ILEVEL(%rsi),%edx
	cmovnzl	%edx,%ecx
	cmpl	%edx,%ecx			/* new level is lower? */
	pushq	%rbx
	jae,pn	2f
1:
	movl	CPU_INFO_IPENDING(%rsi),%eax
	testl	%eax,CPU_INFO_IUNMASK(%rsi,%rcx,4)/* deferred interrupts? */
	movl	%eax,%ebx
	jnz,pn	3f
	cmpxchg8b CPU_INFO_ISTATE(%rsi)		/* swap in new ilevel */
	jnz,pn	1b
2:
	popq	%rbx
	ret
3:
	popq	%rbx
	movl	%ecx, %edi
	jmp	_C_LABEL(Xspllower)

#endif	/* DIAGNOSTIC */

/*
 * void	rw_enter(krwlock_t *rwl, krw_t op);
 *
 * Acquire one hold on a RW lock.
 */
NENTRY(rw_enter)
	cmpl	$RW_READER, %esi
	jne	2f

	/*
	 * Reader: this is the most common case.
	 */
1:	movq	RW_OWNER(%rdi), %rax
	testb	$(RW_WRITE_LOCKED|RW_WRITE_WANTED), %al
	leaq	RW_READ_INCR(%rax), %rdx 
	jnz,pn	_C_LABEL(rw_vector_enter)
	LOCK(2)
	cmpxchgq %rdx, RW_OWNER(%rdi)
	jnz,pn	1b
	ret

	/*
	 * Writer: if the compare-and-set fails, don't bother retrying.
	 */
2:	movq	CPUVAR(CURLWP), %rcx
	xorq	%rax, %rax
	orq	$RW_WRITE_LOCKED, %rcx
	LOCK(3)
	cmpxchgq %rcx, RW_OWNER(%rdi)
	jnz,pn	_C_LABEL(rw_vector_enter)
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
	jnz,pn	3f
	cmpq	$RW_READ_INCR, %rax
	leaq	-RW_READ_INCR(%rax), %rdx
	jb,pn	3f
	LOCK(4)
	cmpxchgq %rdx, RW_OWNER(%rdi)
	jnz,pn	1b
	ret

	/*
	 * Writer
	 */
2:	leaq	-RW_WRITE_LOCKED(%rax), %rdx
	subq	CPUVAR(CURLWP), %rdx
	jnz,pn	3f
	LOCK(5)
	cmpxchgq %rdx, RW_OWNER(%rdi)
	jnz	3f
	ret

3:	jmp	_C_LABEL(rw_vector_exit)

#endif	/* LOCKDEBUG */

/*
 * Spinlocks.
 */
NENTRY(__cpu_simple_lock_init)
	movb	$0, (%rdi)
	ret

NENTRY(__cpu_simple_lock)
	movl	$0x0100, %eax
1:
	LOCK(6)
	cmpxchgb %ah, (%rdi)
	jnz	2f
	ret
2:
	movl	$0x0100, %eax
	pause
	nop
	nop
	cmpb	$0, (%rdi)
	je	1b
	jmp	2b

NENTRY(__cpu_simple_unlock)
	movb	$0, (%rdi)
	ret

NENTRY(__cpu_simple_lock_try)
	movl	$0x0100, %eax
	LOCK(7)
	cmpxchgb %ah, (%rdi)
	movl	$0, %eax
	setz	%al	
	ret

/*
 * Patchpoints to replace with NOP when ncpu == 1.
 */
#ifndef LOCKDEBUG
LABEL(x86_lockpatch)
	.quad	.Lpatch1, .Lpatch2, .Lpatch3, .Lpatch4
	.quad	.Lpatch5, .Lpatch6, .Lpatch7
#ifdef FULL
	.quad	.Lpatch11
#endif
	.quad	0
#endif
