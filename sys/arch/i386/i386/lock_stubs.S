/*	$NetBSD: lock_stubs.S,v 1.32.2.1 2020/01/19 21:08:29 ad Exp $	*/

/*-
 * Copyright (c) 2006, 2007, 2008, 2009 The NetBSD Foundation, Inc.
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
 * Where possible we make each routine fit into an assumed 64-byte cache
 * line.  Please check alignment with 'objdump -d' after making changes. 
 */

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: lock_stubs.S,v 1.32.2.1 2020/01/19 21:08:29 ad Exp $");

#include "opt_lockdebug.h"

#include <machine/cputypes.h>
#include <machine/frameasm.h>

#include "assym.h"

#define	ALIGN64		.align	64
#define	ALIGN32		.align	32
#define	LOCK(num)	\
	HOTPATCH(HP_NAME_NOLOCK, 1)	; \
	lock
#define	RET(num)	\
	HOTPATCH(HP_NAME_RETFENCE, 3)	; \
	ret; nop; nop			; \
	ret

#define	ENDLABEL(name,a) .align	a; LABEL(name)

#if !defined(LOCKDEBUG)

/*
 * void mutex_enter(kmutex_t *mtx);
 *
 * Acquire a mutex and post a load fence.
 */
	ALIGN64

ENTRY(mutex_enter)
	movl	4(%esp), %edx
	xorl	%eax, %eax
	movl	%fs:CPU_INFO_CURLWP(%eax), %ecx
	LOCK(1)
	cmpxchgl %ecx, (%edx)
	jnz	1f
	RET(1)
1:
	jmp	_C_LABEL(mutex_vector_enter)
END(mutex_enter)

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
	xorl	%ecx, %ecx
	movl	%fs:CPU_INFO_CURLWP(%ecx), %eax
	cmpxchgl %ecx, (%edx)
	jnz	1f
	ret
1:
	jmp	_C_LABEL(mutex_vector_exit)
END(mutex_exit)

#ifndef XENPV

/*
 * void mutex_spin_enter(kmutex_t *mtx);
 *
 * Acquire a spin mutex and post a load fence.
 */
ENTRY(mutex_spin_enter)
	movl	4(%esp), %edx
	movb	CPUVAR(ILEVEL), %cl
	movb	MTX_IPL(%edx), %ch
	movl	$0x01, %eax
	cmpb	%ch, %cl
	jg	1f
	movb	%ch, CPUVAR(ILEVEL)		/* splraiseipl() */
1:
	subl	%eax, CPUVAR(MTX_COUNT)		/* decl does not set CF */
	jnc	2f
	movb	%cl, CPUVAR(MTX_OLDSPL)
2:
	xchgb	%al, MTX_LOCK(%edx)		/* lock it */
	testb	%al, %al
	jnz	3f
	RET(6)
3:
	jmp	_C_LABEL(mutex_spin_retry)

	ALIGN64
LABEL(mutex_spin_enter_end)
END(mutex_spin_enter)

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
	.space 32
	.align	32
LABEL(mutex_spin_exit_end)
END(mutex_spin_exit)

/*
 * Patch for i686 CPUs where cli/sti is prohibitively expensive.
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
	jnz	2f
	movl	%eax, %ebx
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
	.space 16
	.align	32
LABEL(i686_mutex_spin_exit_end)
END(i686_mutex_spin_exit)

#else	/* !XENPV */

/* For now; strong alias not working for some reason. */
ENTRY(mutex_spin_enter)
	jmp	_C_LABEL(mutex_vector_enter)

ENTRY(mutex_spin_exit)
	jmp	_C_LABEL(mutex_vector_exit)

#endif	/* !XENPV */

#endif	/* !LOCKDEBUG */

/*
 * Spinlocks.
 */
ENTRY(__cpu_simple_lock_init)
	movl	4(%esp), %edx
	movb	$0, (%edx)
	ret
END(__cpu_simple_lock_init)

ENTRY(__cpu_simple_lock)
	movl	4(%esp), %edx
	movl	$0x0100, %eax
1:
	LOCK(6)
	cmpxchgb %ah, (%edx)
	jnz	2f
	RET(7)
2:
	movl	$0x0100, %eax
	pause
	nop
	nop
	cmpb	$0, (%edx)
	je	1b
	jmp	2b
END(__cpu_simple_lock)

ENTRY(__cpu_simple_unlock)
	movl	4(%esp), %edx
	movb	$0, (%edx)
	ret
END(__cpu_simple_unlock)

ENTRY(__cpu_simple_lock_try)
	movl	4(%esp), %edx
	movl	$0x0100, %eax
	LOCK(7)
	cmpxchgb %ah, (%edx)
	movl	$0, %eax
	setz	%al	
	RET(8)
END(__cpu_simple_lock_try)

