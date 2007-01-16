/*	$NetBSD: lock_stubs.S,v 1.1.2.10 2007/01/16 01:26:19 ad Exp $	*/

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
 * Note on the 80386: the 80386 doesn't have a compare-and-exchange
 * operation.  Stepping A of the i486 has these instructions wired to a
 * different opcode, so should use these stubs also.  They are rare, so
 * we don't make the effort.
 *
 * The sizes listed against each function are for a kernel compiled
 * with options MULTIPROCESSOR.  The offsets are for a kernel compiled
 * without the I386_CPU option.  Where possible we make each routine fit
 * into an assumed 64-byte cache line.
 */

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_cputype.h"
#include "opt_ddb.h"

#include <machine/asm.h>
#include <machine/cputypes.h>

#include "assym.h"

#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
#define	FULL
#endif

#if defined(I386_CPU)
#define	STUB(name, alternate)					\
	NENTRY(name) ;						\
	cmpl	$CPUCLASS_386, _C_LABEL(cpu_class) ;		\
	movl	4(%esp), %edx ;					\
	je	_C_LABEL(alternate)
#define	ALIGN64		.align	16	/* don't bother */
#define	ALIGN32		.align	16	/* don't bother */
#else
#define	STUB(name, alternate)					\
	NENTRY(name) ;						\
	movl	4(%esp), %edx
#define	ALIGN64		.align	64
#define	ALIGN32		.align	32
#endif

#if defined(MULTIPROCESSOR)
#define	LOCK		lock
#else
#define	LOCK		/* nothing */
#endif

#if defined(I686_CPU)
#define	B_NL		cs	/* branch not likely */
#define	B_L		ds	/* branch likely */
#else
#define	B_NL		/* nothing */
#define	B_L		/* nothing */
#endif

#if !defined(LOCKDEBUG)

/*
 * void mutex_enter(kmutex_t *mtx);
 *
 * Acquire a mutex and post a load fence.
 */
	ALIGN64

STUB(mutex_enter, mutex_vector_enter)		/* 0x0000, 20 bytes */
	movl	CPUVAR(CURLWP), %ecx
	xorl	%eax, %eax
	LOCK
	cmpxchgl %ecx, MTX_OWNER(%edx)
	B_NL
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
	ALIGN32

STUB(mutex_exit, mutex_vector_exit)		/* 0x0020, 19 bytes */
	movl	CPUVAR(CURLWP), %eax
	xorl	%ecx, %ecx
	cmpxchgl %ecx, MTX_OWNER(%edx)
	B_NL
	jnz     _C_LABEL(mutex_vector_exit)
	ret

/*
 * void rw_enter(krwlock_t *rwl, krw_t op);
 *
 * Acquire one hold on a RW lock.
 */
	ALIGN64

STUB(rw_enter, rw_vector_enter)			/* 0x0040, 64 bytes */
	cmpl	$RW_READER, 8(%esp)
	movl	RW_OWNER(%edx), %eax
	jne	2f

	/*
	 * Reader
	 */
1:	testb	$(RW_WRITE_LOCKED|RW_WRITE_WANTED), %al
	leal	RW_READ_INCR(%eax), %ecx
	B_NL
	jnz	3f
	LOCK
	cmpxchgl %ecx, RW_OWNER(%edx)
	B_NL
	jnz	1b
	ret

	/*
	 * Writer
	 */
2:	testl	%eax, %eax
	movl	%eax, %ecx
	B_NL
	jnz	3f
	addl	CPUVAR(CURLWP), %ecx
	leal	RW_WRITE_LOCKED(%ecx), %ecx     
	jz	3f
	LOCK
	cmpxchgl %ecx, RW_OWNER(%edx)
	B_NL
	jnz	3f
	ret

	/*
	 * Slow path.
	 */
3:	jmp	_C_LABEL(rw_vector_enter)

/*
 * void rw_exit(krwlock_t *rwl);
 *
 * Release one hold on a RW lock.
 */
	ALIGN64

STUB(rw_exit, rw_vector_exit)			/* 0x0080, 64 bytes */
	movl	RW_OWNER(%edx), %eax
	testb	$RW_WRITE_LOCKED, %al
	jnz	2f

	/*
	 * Reader
	 */
1:	testb	$RW_HAS_WAITERS, %al
	B_NL
	jnz	3f
	cmpl	$RW_READ_INCR, %eax
	leal	-RW_READ_INCR(%eax), %ecx
	B_NL
	jb	3f
	LOCK
	cmpxchgl %ecx, RW_OWNER(%edx)
	B_NL
	jnz	1b
	ret

	/*
	 * Writer
	 */
2:	leal	-RW_WRITE_LOCKED(%eax), %ecx
	subl	CPUVAR(CURLWP), %ecx
	B_NL
	jnz	3f
	LOCK
	cmpxchgl %ecx, RW_OWNER(%edx)
	B_NL
	jnz	3f
	ret

	/*
	 * Slow path.
	 */
3:	jmp	_C_LABEL(rw_vector_exit)


/*
 * void smutex_enter(kmutex_t *mtx);
 *
 * Acquire a spin mutex and post a load fence.
 */
	ALIGN64

STUB(smutex_enter, mutex_vector_enter)		/* 0x00c0, 51 bytes */
	movl	CPUVAR(SELF150), %eax
	movl	(CPU_INFO_ILEVEL-0x150)(%eax), %ecx
	subl	$1, (CPU_INFO_MTX_COUNT-0x150)(%eax)/* decl does not set CF */
	jnc	1f
	movl	%ecx, (CPU_INFO_MTX_OLDSPL-0x150)(%eax)
1:	movb	MTX_IPL(%edx), %ch
	cmpb	%ch, %cl
	B_NL
	jg	2f
	movb	%ch, (CPU_INFO_ILEVEL-0x150)(%eax)/* splraiseipl() */
2:
#if defined(FULL)
	mov	$0x0100, %eax			/* new + expected value */
	LOCK
	cmpxchgb %ah, MTX_LOCK(%edx)		/* lock it */
	B_NL
	jnz	_C_LABEL(smutex_vector_enter)
#endif
	ret

/*
 * void smutex_exit(kmutex_t *mtx);
 *
 * Release a spin mutex and post a store fence.
 */
	ALIGN64

STUB(smutex_exit, mutex_vector_exit)		/* 0x0100, 64 bytes */
#if defined(FULL)
	movl	$0x0001, %eax			/* new + expected value */
	cmpxchgb %ah, MTX_LOCK(%edx)
	B_NL
	jnz	_C_LABEL(mutex_vector_exit)
#endif
	movl	CPUVAR(SELF150), %eax		/* now splx() */
	movl	(CPU_INFO_MTX_OLDSPL-0x150)(%eax), %ecx
	incl	(CPU_INFO_MTX_COUNT-0x150)(%eax)
	jnz	2f
	cmpl	(CPU_INFO_ILEVEL-0x150)(%eax), %ecx
	movl    %ecx, 4(%esp)
	B_NL
	jae	2f
	movl	(CPU_INFO_IUNMASK-0x150)(%eax,%ecx,4), %edx
	cli
	testl	(CPU_INFO_IPENDING-0x150)(%eax), %edx
	B_NL
	jnz	_C_LABEL(Xspllower)		/* does sti */
	movl	%ecx, (CPU_INFO_ILEVEL-0x150)(%eax)
	sti
2:	ret

#endif	/* !LOCKDEBUG */

/*
 * int _lock_cas(uintptr_t *val, uintptr_t old, uintptr_t new);
 *
 * Perform an atomic compare-and-set operation.
 */
	ALIGN64

STUB(_lock_cas, _80386_lock_cas)		/* 0x0140, 32 bytes */
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
_80386_lock_cas:
	movl	8(%esp), %eax
	movl	12(%esp), %ecx
	cli
	cmpl	%eax, (%edx)
	jne	1f
	movl	%ecx, (%edx)
	movb	$1, %al
	sti
	ret

1:	sti
	xorl	%eax, %eax
	ret
#endif	/* I386_CPU */
