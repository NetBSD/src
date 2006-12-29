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
 * Note on the 80386: the 80386 doesn't have a compare-and-exchange
 * operation.  Stepping A of the i486 has these instructions wired to a
 * different opcode, so should use these stubs also.  They are rare, so
 * we don't make the effort.
 */

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_cputype.h"
#include "opt_ddb.h"

#include <machine/asm.h>
#include <machine/cputypes.h>

#include "assym.h"

#undef	_ALIGN_TEXT
#define	_ALIGN_TEXT	.align 32

#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
#define	FULL
#endif

#if defined(I386_CPU)
#define	STUB(name, alternate)					\
	NENTRY(name) ;						\
	cmpl	$CPUCLASS_386, _C_LABEL(cpu_class) ;		\
	movl	4(%esp), %edx ;					\
	je	_C_LABEL(alternate)
#else
#define	STUB(name, alternate)					\
	NENTRY(name) ;						\
	movl	4(%esp), %edx
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
STUB(_lock_cas, _80386_lock_cas)
	movl	8(%esp), %eax
	movl	12(%esp), %ecx
	LOCK
	cmpxchgl %ecx, (%edx)
	movl	$0, %eax
	setz	%al
	ret

#ifdef I386_CPU
/*
 * Since we can't do compare-and-exchange atomically with an 80386, we must
 * disable interrupts in order to support preemption.  On the i386 this is
 * cheap to do.  For other architectures a restartable sequence is usually
 * a better option.
 */
	_ALIGN_TEXT:

_80386_lock_cas:
	movl	8(%esp), %eax
	movl	12(%esp), %ecx
	pushfl
	cli
	cmpl	%eax, (%edx)
	jne	1f
	movl	%ecx, (%edx)
	movb	$1, %al
	popfl
	ret

1:	popfl
	xorl	%eax, %eax
	ret
#endif	/* I386_CPU */

#if !defined(LOCKDEBUG)

/*
 * void mutex_enter(kmutex_t *mtx);
 *
 * Acquire a mutex and post a load fence.
 */
STUB(mutex_enter, mutex_vector_enter)
	movl	CPUVAR(CURLWP), %ecx
	xorl	%eax, %eax
	LOCK
	cmpxchgl %ecx, MTX_OWNER(%edx)
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
STUB(mutex_exit, mutex_vector_exit)
	movl	CPUVAR(CURLWP), %eax
	xorl	%ecx, %ecx
	cmpxchgl %ecx, MTX_OWNER(%edx)
	jnz     _C_LABEL(mutex_vector_exit)
	ret

/*
 * void smutex_enter(kmutex_t *mtx);
 *
 * Acquire a spin mutex and post a load fence.
 */
STUB(smutex_enter, mutex_vector_enter)
	movzbl	MTX_MINSPL(%edx), %eax
	movl	CPUVAR(ILEVEL), %ecx
	cmpl	%eax, %ecx
	jg	1f
	movl	%eax, CPUVAR(ILEVEL)		/* splraiseipl() */
1:
#if defined(FULL)
	mov	$0x0100, %eax			/* new + expected value */
	LOCK
	cmpxchgb %ah, MTX_LOCK(%edx)		/* implied 'lock' prefix */
	jnz	3f
#endif
	subl	$1, CPUVAR(MTX_COUNT)		/* decl does not set CF */
	jnc	2f
	movl	%ecx, CPUVAR(MTX_OLDSPL)
2:	ret

3:	movl	(%esp), %eax
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	call	_C_LABEL(smutex_vector_enter)
	leave
 	ret

/*
 * void smutex_exit(kmutex_t *mtx);
 *
 * Acquire a spin mutex and post a store fence.
 */
STUB(smutex_exit, mutex_vector_exit)
#if defined(FULL)
	movl	$0x0001, %eax			/* new + expected value */
	cmpxchgb %ah, MTX_LOCK(%edx)
	jnz	_C_LABEL(mutex_vector_exit)
#endif
	movl	CPUVAR(MTX_OLDSPL), %ecx
	incl	CPUVAR(MTX_COUNT)
	jnz	1f
	SPLLOWER(_C_LABEL(Xspllower))
1:	ret

/*
 * void rw_enter(krwlock_t *rwl, krw_t op);
 *
 * Acquire one hold on a RW lock.
 */
STUB(rw_enter, rw_vector_enter)
	cmpl	$RW_READER, 8(%esp)
	movl	RW_OWNER(%edx), %eax
	jne	2f

	/*
	 * Reader
	 */
1:	testb	$(RW_WRITE_LOCKED|RW_WRITE_WANTED), %al
	leal	RW_READ_INCR(%eax), %ecx
	jnz	_C_LABEL(rw_vector_enter)
	LOCK
	cmpxchgl %ecx, RW_OWNER(%edx)
	jnz	1b
	ret

	/*
	 * Writer
	 */
2:	testl	%eax, %eax
	movl	%eax, %ecx
	jnz	_C_LABEL(rw_vector_enter)
	addl	CPUVAR(CURLWP), %ecx
	leal	RW_WRITE_LOCKED(%ecx), %ecx     
	jz	_C_LABEL(rw_vector_enter)
	LOCK
	cmpxchgl %ecx, RW_OWNER(%edx)
	jnz	_C_LABEL(rw_vector_enter)
	ret

/*
 * void rw_exit(krwlock_t *rwl);
 *
 * Release one hold on a RW lock.
 */
STUB(rw_exit, rw_vector_exit)
	movl	RW_OWNER(%edx), %eax
	testb	$RW_WRITE_LOCKED, %al
	jnz	2f

	/*
	 * Reader
	 */
1:	testb	$RW_HAS_WAITERS, %al
	jnz	3f
	testl	$RW_THREAD, %eax
	leal	-RW_READ_INCR(%eax), %ecx
	jz	3f
	LOCK
	cmpxchgl %ecx, RW_OWNER(%edx)
	jnz	1b
	ret

	/*
	 * Writer
	 */
2:	leal	-RW_WRITE_LOCKED(%eax), %ecx
	subl	CPUVAR(CURLWP), %ecx
	jnz	4f
	LOCK
	cmpxchgl %ecx, RW_OWNER(%edx)
	jnz	4f
	ret

3:	pushl	%ebp
	movl	%esp, %ebp
	pushl	$RW_READER
	pushl	%edx
	call	_C_LABEL(rw_vector_exit)
	leave
	ret

4:	pushl	%ebp
	movl	%esp, %ebp
	pushl	$RW_WRITER
	pushl	%edx
	call	_C_LABEL(rw_vector_exit)
	leave
	ret

#endif	/* LOCKDEBUG */
