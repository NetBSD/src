/*	$NetBSD: lock_stubs.S,v 1.2.14.1 2007/04/17 06:50:14 thorpej Exp $	*/

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
 * with options MULTIPROCESSOR && DIAGNOSTIC && !I386_CPU.  The offsets
 * are for a kernel compiled without the I386_CPU option.  Where possible
 * we make each routine fit into an assumed 64-byte cache line.
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

#define	END(name,a)	.align	a; LABEL(name)

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
	ALIGN32

STUB(mutex_exit, mutex_vector_exit)		/* 0x0020, 19 bytes */
	movl	CPUVAR(CURLWP), %eax
	xorl	%ecx, %ecx
	cmpxchgl %ecx, MTX_OWNER(%edx)
	jnz,pn	_C_LABEL(mutex_vector_exit)
	ret

/*
 * void rw_enter(krwlock_t *rwl, krw_t op);
 *
 * Acquire one hold on a RW lock.
 */
	ALIGN64

STUB(rw_enter, rw_vector_enter)			/* 0x0040, 60 bytes */
	cmpl	$RW_READER, 8(%esp)
	jne	2f

	/*
	 * Reader
	 */
1:	movl	RW_OWNER(%edx), %eax
	testb	$(RW_WRITE_LOCKED|RW_WRITE_WANTED), %al
	leal	RW_READ_INCR(%eax), %ecx
	jnz,pn	_C_LABEL(rw_vector_enter)
	LOCK
	cmpxchgl %ecx, RW_OWNER(%edx)
	jnz,pn	1b
	ret

	/*
	 * Writer
	 */
2:	movl	CPUVAR(CURLWP), %ecx
	xorl	%eax, %eax
	orl	$RW_WRITE_LOCKED, %ecx
	LOCK
	cmpxchgl %ecx, RW_OWNER(%edx)
	jnz,pn	_C_LABEL(rw_vector_enter)
	ret

/*
 * void rw_exit(krwlock_t *rwl);
 *
 * Release one hold on a RW lock.
 */
	ALIGN64

STUB(rw_exit, rw_vector_exit)			/* 0x0080, 61 bytes */
	movl	RW_OWNER(%edx), %eax
	testb	$RW_WRITE_LOCKED, %al
	jnz	2f

	/*
	 * Reader
	 */
1:	testb	$RW_HAS_WAITERS, %al
	jnz,pn	3f
	cmpl	$RW_READ_INCR, %eax
	leal	-RW_READ_INCR(%eax), %ecx
	jb,pn	3f
	LOCK
	cmpxchgl %ecx, RW_OWNER(%edx)
	jnz,pn	1b
	ret

	/*
	 * Writer
	 */
2:	leal	-RW_WRITE_LOCKED(%eax), %ecx
	subl	CPUVAR(CURLWP), %ecx
	jnz,pn	3f
	LOCK
	cmpxchgl %ecx, RW_OWNER(%edx)
	jnz,pn	3f
	ret

	/*
	 * Slow path.
	 */
3:	jmp	_C_LABEL(rw_vector_exit)

#ifndef __XEN__

/*
 * void mutex_spin_enter(kmutex_t *mtx);
 *
 * Acquire a spin mutex and post a load fence.
 */
	ALIGN64

STUB(mutex_spin_enter, mutex_vector_enter)	/* 0x00c0, 51 bytes */
	movl	CPUVAR(SELF150), %eax
	movl	(CPU_INFO_ILEVEL-0x150)(%eax), %ecx
	subl	$1, (CPU_INFO_MTX_COUNT-0x150)(%eax)/* decl does not set CF */
	jnc	1f
	movl	%ecx, (CPU_INFO_MTX_OLDSPL-0x150)(%eax)
1:	movb	MTX_IPL(%edx), %ch
	cmpb	%ch, %cl
	jg,pn	2f
	movb	%ch, (CPU_INFO_ILEVEL-0x150)(%eax)/* splraiseipl() */
2:
#if defined(FULL)
	mov	$0x0100, %eax			/* new + expected value */
	LOCK
	cmpxchgb %ah, MTX_LOCK(%edx)		/* lock it */
	jnz,pn	_C_LABEL(mutex_spin_retry)
#endif
	ret

	ALIGN64
LABEL(mutex_spin_enter_end)

/*
 * void mutex_spin_exit(kmutex_t *mtx);
 *
 * Release a spin mutex and post a store fence.
 */
	ALIGN64

STUB(mutex_spin_exit, mutex_vector_exit)	/* 0x0100, 50 bytes */
#if defined(DIAGNOSTIC)
	movl	$0x0001, %eax			/* new + expected value */
	cmpxchgb %ah, MTX_LOCK(%edx)
	jnz,pn	_C_LABEL(mutex_vector_exit)
#elif defined(MULTIPROCESSOR)
	movb	$0x00,MTX_LOCK(%edx)
#endif
	movl	CPUVAR(SELF150), %eax
	movl	(CPU_INFO_MTX_OLDSPL-0x150)(%eax), %ecx
	incl	(CPU_INFO_MTX_COUNT-0x150)(%eax)
	jnz	1f
	cmpl	(CPU_INFO_ILEVEL-0x150)(%eax), %ecx
	movl    %ecx, 4(%esp)
	jae	1f
	movl	(CPU_INFO_IUNMASK-0x150)(%eax,%ecx,4), %edx
	cli
	testl	(CPU_INFO_IPENDING-0x150)(%eax), %edx
	jnz	_C_LABEL(Xspllower)		/* does sti */
	movl	%ecx, (CPU_INFO_ILEVEL-0x150)(%eax)
	sti
1:	ret

	ALIGN64
LABEL(mutex_spin_exit_end)

#if !defined(I386_CPU) && defined(I686_CPU) && !defined(DIAGNOSTIC)

/*
 * Patch for i686 CPUs where cli/sti is prohibitavely expensive.
 * Must be the same size as mutex_spin_exit().
 */
	ALIGN64

ENTRY(i686_mutex_spin_exit)			/* 64 bytes */
	mov	4(%esp),%edx
	xorl	%eax,%eax
	pushl	%edi
	fs
	movl	(CPU_INFO_SELF150)(%eax), %edi	/* now splx() */
	pushl	%ebx
	movl	(CPU_INFO_MTX_OLDSPL-0x150)(%edi), %ecx
	incl	(CPU_INFO_MTX_COUNT-0x150)(%edi)
	movb	%al, MTX_LOCK(%edx)		/* zero */
	movl	(CPU_INFO_ILEVEL-0x150)(%edi), %edx
	jnz	1f
	cmpl	%edx, %ecx			/* new level is lower? */
	movl	(CPU_INFO_IPENDING-0x150)(%edi), %eax
	jae,pn	1f
	testl	%eax,(CPU_INFO_IUNMASK-0x150)(%edi,%ecx,4)
	movl	%eax, %ebx
	/*
	 * On a P4 this jump is cheaper than patching in junk using
	 * cmovnz.  Is cmpxchg expensive if it fails?
	 */
	jnz,pn	2f
	cmpxchg8b (CPU_INFO_ISTATE-0x150)(%edi)	/* swap in new ilevel */
	jnz,pn	2f
1:
	popl	%ebx
	popl	%edi
	ret
2:
	popl	%ebx
	popl	%edi
	movl	%ecx,4(%esp)
LABEL(i686_mutex_spin_exit_patch)
	jmp	_C_LABEL(Xspllower)
	ALIGN64
LABEL(i686_mutex_spin_exit_end)

#endif	/* !defined(I386_CPU) && defined(I686_CPU) && !defined(DIAGNOSTIC) */

#else	/* !__XEN__ */

/* For now; strong alias not working for some reason. */
NENTRY(mutex_spin_enter)
	jmp	_C_LABEL(mutex_vector_enter)

NENTRY(mutex_spin_exit)
	jmp	_C_LABEL(mutex_vector_exit)

#endif	/* !__XEN__ */

#endif	/* !LOCKDEBUG */

/*
 * Memory barrier operations, may be patched at runtime.
 */
	.align	8

NENTRY(mb_read)
	lock
	addl	$0, 0(%esp)
	ret
END(mb_read_end, 8)

NENTRY(mb_write)
	/* Nothing at the moment. */
	ret
END(mb_write_end, 8)

NENTRY(mb_memory)
	lock
	addl	$0, 0(%esp)
	ret
END(mb_memory_end, 8)

#ifdef I686_CPU
NENTRY(sse2_mb_read)
	lfence
	ret
END(sse2_mb_read_end, 8)

NENTRY(sse2_mb_memory)
	mfence
	ret
END(sse2_mb_memory_end, 8)
#endif	/* I686_CPU */

/*
 * Make sure code after the ret is properly encoded with nopness
 * by gas, or could stall newer processors.
 */

NENTRY(x86_mb_nop)
	ret
END(x86_mb_nop_end, 8)
