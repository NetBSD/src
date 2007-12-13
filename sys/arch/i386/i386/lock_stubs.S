/*	$NetBSD: lock_stubs.S,v 1.10.6.1 2007/12/13 21:54:42 bouyer Exp $	*/

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
 * The sizes listed against each function are for a kernel compiled
 * with options MULTIPROCESSOR && DIAGNOSTIC.  Where possible we make
 * each routine fit into an assumed 64-byte cache line.  Please check
 * alignment with 'objdump -d' after making changes.
 */

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: lock_stubs.S,v 1.10.6.1 2007/12/13 21:54:42 bouyer Exp $");

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_ddb.h"

#include <machine/cputypes.h>

#include "assym.h"

#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
#define	FULL
#endif

#define	ALIGN64		.align	64
#define	ALIGN32		.align	32
#define	LOCK(name)	LABEL(name)	lock
#define	END(name,a)	.align	a; LABEL(name)

#if !defined(LOCKDEBUG)

/*
 * void mutex_enter(kmutex_t *mtx);
 *
 * Acquire a mutex and post a load fence.
 */
	ALIGN64

ENTRY(mutex_enter)
	movl	4(%esp), %edx
	movl	CPUVAR(CURLWP), %ecx
	xorl	%eax, %eax
	LOCK(lockpatch1)
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
ENTRY(mutex_exit)
	movl	4(%esp), %edx
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
ENTRY(rw_enter)
	movl	4(%esp), %edx
	cmpl	$RW_READER, 8(%esp)
	jne	2f

	/*
	 * Reader
	 */
1:	movl	RW_OWNER(%edx), %eax
	testb	$(RW_WRITE_LOCKED|RW_WRITE_WANTED), %al
	leal	RW_READ_INCR(%eax), %ecx
	jnz,pn	_C_LABEL(rw_vector_enter)
	LOCK(lockpatch2)
	cmpxchgl %ecx, RW_OWNER(%edx)
	jnz,pn	1b
	ret

	/*
	 * Writer
	 */
2:	movl	CPUVAR(CURLWP), %ecx
	xorl	%eax, %eax
	orl	$RW_WRITE_LOCKED, %ecx
	LOCK(lockpatch3)
	cmpxchgl %ecx, RW_OWNER(%edx)
	jnz,pn	_C_LABEL(rw_vector_enter)
	ret

/*
 * void rw_exit(krwlock_t *rwl);
 *
 * Release one hold on a RW lock.
 */
ENTRY(rw_exit)
	movl	4(%esp), %edx
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
	LOCK(lockpatch4)
	cmpxchgl %ecx, RW_OWNER(%edx)
	jnz,pn	1b
	ret

	/*
	 * Writer
	 */
2:	leal	-RW_WRITE_LOCKED(%eax), %ecx
	subl	CPUVAR(CURLWP), %ecx
	jnz,pn	3f
	LOCK(lockpatch5)
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
ENTRY(mutex_spin_enter)
	movl	4(%esp), %edx
	movl	CPUVAR(ILEVEL), %ecx
	subl	$1, CPUVAR(MTX_COUNT)		/* decl does not set CF */
	jnc	1f
	movl	%ecx, CPUVAR(MTX_OLDSPL)
1:	movb	MTX_IPL(%edx), %ch
	cmpb	%ch, %cl
	jg,pn	2f
	movb	%ch, CPUVAR(ILEVEL)		/* splraiseipl() */
2:
#ifdef FULL
	mov	$0x0100, %eax			/* new + expected value */
	LOCK(lockpatch11)
	cmpxchgb %ah, MTX_LOCK(%edx)		/* lock it */
	jnz,pn	_C_LABEL(mutex_spin_retry)
#endif
	ret

	ALIGN64
LABEL(mutex_spin_enter_end)

/*
 * Release a spin mutex and post a store fence.
 */
ENTRY(mutex_spin_exit)
	movl	4(%esp), %edx
	movl	CPUVAR(MTX_OLDSPL), %ecx
	incl	CPUVAR(MTX_COUNT)
	movb	$0, MTX_LOCK(%edx)		/* zero */
	jnz	1f
	movl	%fs:CPU_INFO_IUNMASK(,%ecx,4), %edx
	cli
	testl	CPUVAR(IPENDING), %edx
	movl    %ecx, 4(%esp)
	jnz	_C_LABEL(Xspllower)		/* does sti */
	movl	%ecx, CPUVAR(ILEVEL)
	sti
1:	ret
	nop					/* XXX round up */
	.align	32
LABEL(mutex_spin_exit_end)

/*
 * Patch for i686 CPUs where cli/sti is prohibitavely expensive.
 * Must be the same size as mutex_spin_exit().
 */
ENTRY(i686_mutex_spin_exit)
	mov	4(%esp),%edx
	movl	CPUVAR(MTX_OLDSPL), %ecx
	incl	CPUVAR(MTX_COUNT)
	movb	%ch, MTX_LOCK(%edx)		/* zero */
	jnz	1f
	pushl	%ebx
0:
	movl	CPUVAR(IPENDING), %eax
	testl	%eax, %fs:CPU_INFO_IUNMASK(,%ecx,4)
	movl	%eax, %ebx
	/*
	 * On a P4 this jump is cheaper than patching in junk using
	 * cmovnz.  Is cmpxchg expensive if it fails?
	 */
	jnz	2f
	cmpxchg8b CPUVAR(ISTATE)		/* swap in new ilevel */
	jnz	0b
	popl	%ebx
1:
	ret
2:
	popl	%ebx
	movl	%ecx,4(%esp)
LABEL(i686_mutex_spin_exit_patch)
	jmp	_C_LABEL(Xspllower)
	.align	32
LABEL(i686_mutex_spin_exit_end)

#else	/* !__XEN__ */

/* For now; strong alias not working for some reason. */
ENTRY(mutex_spin_enter)
	jmp	_C_LABEL(mutex_vector_enter)

ENTRY(mutex_spin_exit)
	jmp	_C_LABEL(mutex_vector_exit)

#endif	/* !__XEN__ */

#endif	/* !LOCKDEBUG */

/*
 * int _lock_cas(uintptr_t *val, uintptr_t old, uintptr_t new);
 *
 * Perform an atomic compare-and-set operation.
 */
ENTRY(_lock_cas)
	mov	4(%esp),%edx
	movl	8(%esp), %eax
	movl	12(%esp), %ecx
	LOCK(lockpatch6)
	cmpxchgl %ecx, (%edx)
	movl	$0, %eax
	setz	%al
	ret

/*
 * Memory barrier operations, may be patched at runtime.
 */
	.align	8

ENTRY(mb_read)
	LOCK(lockpatch7)
	addl	$0, 0(%esp)
	ret
END(mb_read_end, 8)

ENTRY(mb_write)
	nop
	ret
END(mb_write_end, 8)

ENTRY(mb_memory)
	LOCK(lockpatch8)
	addl	$0, 0(%esp)
	ret
END(mb_memory_end, 8)

ENTRY(sse2_mb_read)
	lfence
	ret
END(sse2_mb_read_end, 8)

ENTRY(sse2_mb_memory)
	mfence
	ret
END(sse2_mb_memory_end, 8)

/*
 * Patchpoints to replace with NOP when ncpu == 1.
 */
#ifndef LOCKDEBUG
LABEL(x86_lockpatch)
	.long	lockpatch1, lockpatch2, lockpatch3, lockpatch4
	.long	lockpatch5, lockpatch6, lockpatch7, lockpatch8
#if defined(FULL) && !defined(__XEN__)
	.long	lockpatch11
#endif
	.long	0
#endif
