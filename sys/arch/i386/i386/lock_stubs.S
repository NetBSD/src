

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
 * The 80386 lock stubs do not support SMP.  To do this, the layout of the
 * locks would need to change so that there is an interlock that we can
 * manipulate using xchg.
 */

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_cputype.h"
#include "opt_ddb.h"		/* for SPLLOWER() */

#include <machine/asm.h>
#include <machine/cputypes.h>

#include "assym.h"

#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR)
#define	FULL
#endif

#undef	_ALIGN_TEXT
#define	_ALIGN_TEXT	.align 32

#if defined(I386_CPU)
#define	IF386(name)						\
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class) ;		\
	je	_C_LABEL(name)	
#else
#define	IF386(name)	/* nothing */
#endif

/*
 * void	_lock_cas(uintptr_t *val, uintptr_t old, uintptr_t new);
 */
NENTRY(_lock_cas)
	IF386(_80386_lock_cas)

	movl	4(%esp), %edx
	movl	8(%esp), %eax
	movl	12(%esp), %ecx
	lock
	cmpxchgl %ecx, (%edx)
	movl	$0, %eax
	setz	%al
	ret

/*
 * void	_lock_set_waiters(uintptr_t *val, uintptr_t need, uintptr_t set);
 *
 * Note: 'need' must be non-zero.
 */
NENTRY(_lock_set_waiters)
	IF386(_80386_lock_set_waiters)

	movl	4(%esp), %edx
	movl	12(%esp), %ecx

1:	testl	8(%esp), %eax
	jz	2f
	orl	%eax, %ecx
	lock
	cmpxchgl %ecx, (%edx)
	jnz	1b
	ret					/* %eax is non zero */

2:	xorl	%eax, %eax
	ret

#if !defined(LOCKDEBUG)

/*
 * void mutex_enter(kmutex_t *mtx);
 */
NENTRY(mutex_enter)
	IF386(_80386_mutex_enter)

	movl	4(%esp), %edx
	cmpb	$0xff, MTX_ALLONE(%edx)		/* spin mutex? */
	jne	2f				/* no: branch */
	movzbl	MTX_MINSPL(%edx), %eax
	movl	CPUVAR(ILEVEL), %ecx
	cmpl	%eax, %ecx
	jg	1f
	movl	%eax, CPUVAR(ILEVEL)		/* splraiseipl() */

1:
#if defined(FULL)
	movl	$0x0100, %eax			/* expected+new value */
	lock
	cmpxchgb %ah, MTX_LOCK(%edx)
	jnz	3f
#endif
	movb	%cl, MTX_OLDSPL(%edx)
	ret

2:	movl	CPUVAR(CURLWP), %ecx
	xorl	%eax, %eax
	lock
	cmpxchgl %ecx, (%edx)
	jnz	3f
	ret

3:	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ecx
	pushl	%edx
	call	_C_LABEL(mutex_vector_enter)
	leave
	ret

/*
 * void mutex_exit(kmutex_t *mtx);
 */
NENTRY(mutex_exit)
	IF386(_80386_mutex_exit)

	movl	4(%esp), %edx
	cmpb	$0xff, MTX_ALLONE(%edx)		/* spin mutex? */
	jne	1f				/* no: branch */
	movzbl	MTX_OLDSPL(%edx), %ecx		/* for SPLLOWER */
#if defined(FULL)
	movl	$0x0001, %eax			/* expected+new value */
	cmpxchgb %ah, MTX_LOCK(%edx)
	jnz	_C_LABEL(mutex_vector_exit)
#endif
	SPLLOWER(_C_LABEL(Xspllower))
	/* ret */

1:	movl	CPUVAR(CURLWP), %eax
	xorl	%ecx, %ecx
	lock
	cmpxchgl %ecx, (%edx)
	jnz     _C_LABEL(mutex_vector_exit)
	ret

/*
 * void	rw_enter(krwlock_t *rwl, krw_t op);
 */
NENTRY(rw_enter)
	IF386(_80386_rw_enter)

	movl	4(%esp), %edx
	cmpl	$RW_READER, 8(%esp)
	movl	(%edx), %eax
	jne	2f

	/*
	 * Reader
	 */
1:	testb	$(RW_WRITE_LOCKED|RW_WRITE_WANTED), %al
	leal	RW_READ_INCR(%eax), %ecx
	jnz	_C_LABEL(rw_vector_enter)
	lock
	cmpxchgl %ecx, (%edx)
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
	lock
	cmpxchgl %ecx, (%edx)
	jnz	_C_LABEL(rw_vector_enter)
	ret

/*
 * void	rw_exit(krwlock_t *rwl);
 */
NENTRY(rw_exit)
	IF386(_80386_rw_exit)

	movl	4(%esp), %edx
	movl	(%edx), %eax
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
	lock
	cmpxchgl %ecx, (%edx)
	jnz	1b
	ret

	/*
	 * Writer
	 */
2:	leal	-RW_WRITE_LOCKED(%eax), %ecx
	subl	CPUVAR(CURLWP), %ecx
	jnz	4f
	lock
	cmpxchgl %ecx, (%edx)
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
 * void	_lock_cas(uintptr_t *val, uintptr_t old, uintptr_t new);
 */

NENTRY(_80386_lock_cas)
	movl	4(%esp), %edx
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
 * void	_lock_set_waiters(uintptr_t *val, uintptr_t need, uintptr_t set);
 *
 * Note: 'need' must be non-zero.
 */
NENTRY(_80386_lock_set_waiters)
	movl	4(%esp), %edx
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
	movl	4(%esp), %edx
	cmpb	$0xff, MTX_ALLONE(%edx)		/* spin mutex? */
	jne	2f				/* no: branch */
	movzbl	MTX_MINSPL(%edx), %eax
	movl	CPUVAR(ILEVEL), %ecx
	cmpl	%eax, %ecx
	jg	1f
	movl	%eax, CPUVAR(ILEVEL)		/* splraiseipl() */
1:
#ifdef FULL
	movb	$1, %al
	xchgb	%al, MTX_LOCK(%edx)
	testb	%al, %al
	jnz	4f
#endif
	movb	%cl, MTX_OLDSPL(%edx)
	ret

2:	movl	CPUVAR(CURLWP), %ecx
	pushfl
	cli
	cmpl	$0, (%edx)
	jne	3f
	movl	%ecx, (%edx)
	popfl
	ret

3:	popfl	
4:	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ecx
	pushl	%edx
	call	_C_LABEL(mutex_vector_enter)
	leave
	ret

/*
 * void _mutex_exit(kmutex_t *mtx);
 */
NENTRY(_80386_mutex_exit)
	movl	4(%esp), %edx
	cmpb	$0xff, MTX_ALLONE(%edx)		/* spin mutex? */
	jne	1f				/* no: branch */
	movzbl	MTX_OLDSPL(%edx), %ecx		/* for SPLLOWER */
#ifdef FULL
	xorb	%al, %al
	xchgb	%al, MTX_LOCK(%edx)
	testb	%al, %al
	jz	3f
#endif
	SPLLOWER(_C_LABEL(Xspllower))
	/* ret */

1:	movl	CPUVAR(CURLWP), %ecx
	pushfl
	cli
	cmpl	%ecx, (%edx)
	jne	2f
	movl	$0, (%edx)
	popfl
	ret

2:	popfl
3:	jmp	_C_LABEL(mutex_vector_exit)

/*
 * void	rw_enter(krwlock_t *rwl, krw_t op);
 */
NENTRY(_80386_rw_enter)
	movl	4(%esp), %edx
	cmpl	$RW_READER, 8(%esp)
	pushfl
	cli
	movl	(%edx), %eax
	jne	2f

	/*
	 * Reader
	 */
	testb	$(RW_WRITE_LOCKED|RW_WRITE_WANTED), %al
	jnz	3f
	addl	$RW_READ_INCR, %eax
	movl	%eax, (%edx)
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
	movl	%eax, (%edx)
	popfl
	ret

3:	popfl
	jmp	_C_LABEL(rw_vector_enter)

/*
 * void	rw_exit(krwlock_t *rwl);
 */
NENTRY(_80386_rw_exit)
	movl	4(%esp), %edx
	pushfl
	cli
	movl	(%edx), %eax
	testb	$RW_WRITE_LOCKED, %al
	jnz	2f

	movl	$RW_READER, %ecx
	testb	$RW_HAS_WAITERS, %al
	jnz	3f
	testl	$RW_THREAD, %eax
	jz	3f
	subl	$-RW_READ_INCR, %eax
	movl	%eax, (%edx)
	popfl
	ret

2:	movl	$RW_WRITER, %ecx
	movl	(%edx), %eax
	subl	$RW_WRITE_LOCKED, %eax
	subl	CPUVAR(CURLWP), %eax
	jnz	3f
	movl	%eax, (%edx)
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
