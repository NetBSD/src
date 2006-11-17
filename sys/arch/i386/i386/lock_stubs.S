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
 * i386 lock stubs.
 *
 * There are two sets of stubs, one for 80386 CPUs and one for later CPUs.
 * We could patch this in at boot time, but the maintenance overhead would
 * be too high.
 *
 * Note on the 80386 stubs: the 80386 doesn't have a compare-and-exchange
 * operation.  Stepping A of the i486 has these instructions wired to a
 * different opcode, so should use these stubs also.  They are rare, so we
 * don't make the effort.
 *
 * Since we can't do compare-and-exchange atomically with an 80386, we must
 * disable interrupts in order to support preemption.  On the i386 this is
 * cheap to do.  For other architectures a restartable sequence may be a
 * better option.
 *
 * The 80386 lock stubs do not support SMP.
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
#define	STUB(name)						\
	NENTRY(name) ;						\
	cmpl	$CPUCLASS_386, _C_LABEL(cpu_class) ;		\
	movl	4(%esp), %edx ;					\
	je	_C_LABEL(_80386_/**/name)
#else
#define	STUB(name)						\
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
 */
STUB(_lock_cas)
	movl	8(%esp), %eax
	movl	12(%esp), %ecx
	LOCK
	cmpxchgl %ecx, (%edx)
	movl	$0, %eax
	setz	%al
	ret

/*
 * int _lock_set_waiters(uintptr_t *val, uintptr_t need, uintptr_t set);
 *
 * Note: 'need' must be non-zero.
 */
STUB(_lock_set_waiters)
	movl	12(%esp), %ecx
	movl	(%edx), %eax

1:	testl	8(%esp), %eax
	jz	2f
	orl	%eax, %ecx
	LOCK
	cmpxchgl %ecx, (%edx)
	jnz	1b
	ret					/* %eax is non zero */

2:	xorl	%eax, %eax
	ret

#if !defined(LOCKDEBUG)

/*
 * void mutex_enter(kmutex_t *mtx);
 */
STUB(mutex_enter)
	testb	$MUTEX_BIT_SPIN, MTX_FLAGS(%edx)/* spin mutex? */
	jz	3f				/* no: branch */
	movzbl	MTX_MINSPL(%edx), %eax
	movl	CPUVAR(ILEVEL), %ecx
	cmpl	%eax, %ecx
	jg	1f
	movl	%eax, CPUVAR(ILEVEL)		/* splraiseipl() */
1:
#if defined(FULL)
	movl	$0x0301, %eax			/* expected+new value */
	LOCK
	cmpxchgb %ah, MTX_FLAGS(%edx)
	jnz	4f
#endif
	subl	$1, CPUVAR(MTX_COUNT)		/* decl does not set CF */
	jnc	2f
	movl	%ecx, CPUVAR(MTX_OLDSPL)
2:	ret

3:	movl	CPUVAR(CURLWP), %ecx
	xorl	%eax, %eax
	LOCK
	cmpxchgl %ecx, MTX_OWNER(%edx)
	jnz	4f
	ret

4:	movl	(%esp), %eax
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	call	_C_LABEL(mutex_vector_enter)
	leave
 	ret

/*
 * void mutex_exit(kmutex_t *mtx);
 */
STUB(mutex_exit)
	testb	$MUTEX_BIT_SPIN, MTX_FLAGS(%edx)/* spin mutex? */
	jz	2f				/* no: branch */
#if defined(FULL)
	movl	$0x0103, %eax			/* expected+new value */
	cmpxchgb %ah, MTX_FLAGS(%edx)
	jnz	_C_LABEL(mutex_vector_exit)
#endif
	movl	CPUVAR(MTX_OLDSPL), %ecx
	incl	CPUVAR(MTX_COUNT)
	jnz	1f
	SPLLOWER(_C_LABEL(Xspllower))
1:	ret

2:	movl	CPUVAR(CURLWP), %eax
	xorl	%ecx, %ecx
	LOCK
	cmpxchgl %ecx, MTX_OWNER(%edx)
	jnz     _C_LABEL(mutex_vector_exit)
	ret

/*
 * void rw_enter(krwlock_t *rwl, krw_t op);
 */
STUB(rw_enter)
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
 */
STUB(rw_exit)
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

#ifdef I386_CPU

/*
 * void _lock_cas(uintptr_t *val, uintptr_t old, uintptr_t new);
 */

NENTRY(_80386__lock_cas)
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

/*
 * void _lock_set_waiters(uintptr_t *val, uintptr_t need, uintptr_t set);
 *
 * Note: 'need' must be non-zero.
 */
NENTRY(_80386__lock_set_waiters)
	movl	8(%esp), %eax
	movl	12(%esp), %ecx
	pushfl
	cli
	testl	%eax, (%edx)
	jz	2f
	orl	%ecx, (%edx)
	popfl
	ret					/* %eax is non zero */

2:	popfl
	xorl	%eax, %eax
	ret

/*
 * void _mutex_enter(kmutex_t *mtx);
 */
NENTRY(_80386_mutex_enter)
	testb	$MUTEX_BIT_SPIN, MTX_FLAGS(%edx)/* spin mutex? */
	jz	3f				/* no: branch */
	movzbl	MTX_MINSPL(%edx), %eax
	movl	CPUVAR(ILEVEL), %ecx
	cmpl	%eax, %ecx
	jg	1f
	movl	%eax, CPUVAR(ILEVEL)		/* splraiseipl() */
1:
#ifdef FULL
	movb	$MUTEX_SPIN_LOCKED, %al
	xchgb	%al, MTX_FLAGS(%edx)
	cmpb	$MUTEX_SPIN_UNLOCKED, %al
	jne	5f
#endif
	subl	$1, CPUVAR(MTX_COUNT)		/* decl does not set CF */
	jnc	2f
	movl	%ecx, CPUVAR(MTX_OLDSPL)
2:	ret

3:	movl	CPUVAR(CURLWP), %ecx
	pushfl
	cli
	cmpl	$0, MTX_OWNER(%edx)
	jne	4f
	movl	%ecx, MTX_OWNER(%edx)
	popfl
	ret

4:	popfl
5:	movl	(%esp), %eax
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	call	_C_LABEL(mutex_vector_enter)
	leave
 	ret

/*
 * void _mutex_exit(kmutex_t *mtx);
 */
NENTRY(_80386_mutex_exit)
	testb	$MUTEX_BIT_SPIN, MTX_FLAGS(%edx)/* spin mutex? */
	jz	2f				/* no: branch */
#ifdef FULL
	movb	$MUTEX_SPIN_UNLOCKED, %al
	xchgb	%al, MTX_FLAGS(%edx)
	cmpb	$MUTEX_SPIN_LOCKED, %al
	jne	4f
#endif
	incl	CPUVAR(MTX_COUNT)
	jnz	1f
	movl	CPUVAR(MTX_OLDSPL), %ecx
	SPLLOWER(_C_LABEL(Xspllower))
1:	ret

2:	movl	CPUVAR(CURLWP), %ecx
	pushfl
	cli
	cmpl	%ecx, MTX_OWNER(%edx)
	jne	3f
	movl	$0, MTX_OWNER(%edx)
	popfl
	ret

3:	popfl
4:	jmp	_C_LABEL(mutex_vector_exit)

/*
 * void rw_enter(krwlock_t *rwl, krw_t op);
 */
NENTRY(_80386_rw_enter)
	cmpl	$RW_READER, 8(%esp)
	pushfl
	cli
	movl	RW_OWNER(%edx), %eax
	jne	2f

	/*
	 * Reader
	 */
	testb	$(RW_WRITE_LOCKED|RW_WRITE_WANTED), %al
	jnz	3f
	addl	$RW_READ_INCR, %eax
	movl	%eax, RW_OWNER(%edx)
	popfl
	ret

	/*
	 * Writer
	 */
2:	testl	%eax, %eax
	jnz	3f
	movl	%eax, %ecx
	addl	CPUVAR(CURLWP), %eax
	jz	3f
	orb	$RW_WRITE_LOCKED, %al
	movl	%eax, RW_OWNER(%edx)
	popfl
	ret

3:	popfl
	jmp	_C_LABEL(rw_vector_enter)

/*
 * void rw_exit(krwlock_t *rwl);
 */
NENTRY(_80386_rw_exit)
	pushfl
	cli
	movl	RW_OWNER(%edx), %eax
	testb	$RW_WRITE_LOCKED, %al
	jnz	2f

	movl	$RW_READER, %ecx
	testb	$RW_HAS_WAITERS, %al
	jnz	3f
	testl	$RW_THREAD, %eax
	jz	3f
	subl	$-RW_READ_INCR, %eax
	movl	%eax, RW_OWNER(%edx)
	popfl
	ret

2:	movl	$RW_WRITER, %ecx
	movl	RW_OWNER(%edx), %eax
	subl	$RW_WRITE_LOCKED, %eax
	subl	CPUVAR(CURLWP), %eax
	jnz	3f
	movl	%eax, RW_OWNER(%edx)
	popfl
	ret

3:	popfl
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ecx
	pushl	%edx
	call	_C_LABEL(rw_vector_exit)
	leave
	ret

#endif	/* I386_CPU */

#endif	/* LOCKDEBUG */
