/*	$NetBSD: atomic.S,v 1.7.2.2 2008/01/09 01:20:53 matt Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe, and by Andrew Doran.
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

#include <machine/asm.h>

#ifdef _KERNEL
#define	LOCK(n)		.Lpatch/**/n:	lock
#define	ALIAS(f, t)	STRONG_ALIAS(f,t)
#define	END(a)		_ALIGN_TEXT; LABEL(a)
#else
#define	LOCK(n)		lock
#define	ALIAS(f, t)	WEAK_ALIAS(f,t)
#define	END(a)		/* nothing */
#endif

	.text

NENTRY(_atomic_add_32)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	LOCK(1)
	addl	%eax, (%edx)
	ret

NENTRY(_atomic_add_32_nv)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	movl	%eax, %ecx
	LOCK(2)
	xaddl	%eax, (%edx)
	addl	%ecx, %eax
	ret

NENTRY(_atomic_and_32)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	LOCK(3)
	andl	%eax, (%edx)
	ret

NENTRY(_atomic_and_32_nv)
	movl	4(%esp), %edx
	movl	(%edx), %eax
1:
	movl	%eax, %ecx
	andl	8(%esp), %ecx
	LOCK(4)
	cmpxchgl %ecx, (%edx)
	jnz	1b
	movl	%ecx, %eax
	ret

NENTRY(_atomic_dec_32)
	movl	4(%esp), %edx
	LOCK(5)
	decl	(%edx)
	ret

NENTRY(_atomic_dec_32_nv)
	movl	4(%esp), %edx
	movl	$-1, %eax
	LOCK(6)
	xaddl	%eax, (%edx)
	decl	%eax
	ret

NENTRY(_atomic_inc_32)
	movl	4(%esp), %edx
	LOCK(7)
	incl	(%edx)
	ret

NENTRY(_atomic_inc_32_nv)
	movl	4(%esp), %edx
	movl	$1, %eax
	LOCK(8)
	xaddl	%eax, (%edx)
	incl	%eax
	ret

NENTRY(_atomic_or_32)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	LOCK(9)
	orl	%eax, (%edx)
	ret

NENTRY(_atomic_or_32_nv)
	movl	4(%esp), %edx
	movl	(%edx), %eax
1:
	movl	%eax, %ecx
	orl	8(%esp), %ecx
	LOCK(10)
	cmpxchgl %ecx, (%edx)
	jnz	1b
	movl	%ecx, %eax
	ret

NENTRY(_atomic_swap_32)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	xchgl	%eax, (%edx)
	ret

NENTRY(_atomic_cas_32)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	movl	12(%esp), %ecx
	LOCK(12)
	cmpxchgl %ecx, (%edx)
	/* %eax now contains the old value */
	ret

NENTRY(_membar_consumer)
	LOCK(13)
	addl	$0, -4(%esp)
	ret
END(membar_consumer_end)

NENTRY(_membar_producer)
	/* A store is enough */
	movl	$0, -4(%esp)
	ret
END(membar_producer_end)

NENTRY(_membar_enter)
	/* A store is enough */
	movl	$0, -4(%esp)
	ret
END(membar_enter_end)

NENTRY(_membar_exit)
	/* A store is enough */
	movl	$0, -4(%esp)
	ret
END(membar_exit_end)

NENTRY(_membar_sync)
	LOCK(14)
	addl	$0, -4(%esp)
	ret
END(membar_sync_end)

#ifdef _KERNEL
NENTRY(_atomic_cas_64)
	cli
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movl	16(%esp), %eax
	movl	20(%esp), %edx
	movl	24(%esp), %ebx
	movl	28(%esp), %ecx
	cmpl	0(%edi), %eax
	jne	2f
	cmpl	4(%edi), %edx
	jne	2f
	movl	%ebx, 0(%edi)
	movl	%ecx, 4(%edi)
1:
	popl	%ebx
	popl	%edi
	sti
	ret
2:
	movl	0(%edi), %eax
	movl	4(%edi), %edx
	jmp	1b
END(_atomic_cas_64_end)

NENTRY(_atomic_cas_cx8)
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movl	16(%esp), %eax
	movl	20(%esp), %edx
	movl	24(%esp), %ebx
	movl	28(%esp), %ecx
	LOCK(15)
	cmpxchg8b (%edi)
	popl	%ebx
	popl	%edi
	ret
	.space	32, 0x90
END(_atomic_cas_cx8_end)

NENTRY(sse2_lfence)
	lfence
	ret
END(sse2_lfence_end)

NENTRY(sse2_mfence)
	mfence
	ret
END(sse2_mfence_end)

atomic_lockpatch:
	.globl	atomic_lockpatch
	.long	.Lpatch1, .Lpatch2, .Lpatch3, .Lpatch4, .Lpatch5
	.long	.Lpatch6, .Lpatch7, .Lpatch8, .Lpatch9, .Lpatch10
	.long	.Lpatch12, .Lpatch13, .Lpatch14, .Lpatch15, 0
#else
NENTRY(_atomic_cas_64)
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movl	16(%esp), %eax
	movl	20(%esp), %edx
	movl	24(%esp), %ebx
	movl	28(%esp), %ecx
	lock
	cmpxchg8b (%edi)
	popl	%ebx
	popl	%edi
	ret
#endif	/* _KERNEL */

ALIAS(atomic_add_32,_atomic_add_32)
ALIAS(atomic_add_int,_atomic_add_32)
ALIAS(atomic_add_long,_atomic_add_32)
ALIAS(atomic_add_ptr,_atomic_add_32)

ALIAS(atomic_add_32_nv,_atomic_add_32_nv)
ALIAS(atomic_add_int_nv,_atomic_add_32_nv)
ALIAS(atomic_add_long_nv,_atomic_add_32_nv)
ALIAS(atomic_add_ptr_nv,_atomic_add_32_nv)

ALIAS(atomic_and_32,_atomic_and_32)
ALIAS(atomic_and_uint,_atomic_and_32)
ALIAS(atomic_and_ulong,_atomic_and_32)
ALIAS(atomic_and_ptr,_atomic_and_32)

ALIAS(atomic_and_32_nv,_atomic_and_32_nv)
ALIAS(atomic_and_uint_nv,_atomic_and_32_nv)
ALIAS(atomic_and_ulong_nv,_atomic_and_32_nv)
ALIAS(atomic_and_ptr_nv,_atomic_and_32_nv)

ALIAS(atomic_dec_32,_atomic_dec_32)
ALIAS(atomic_dec_uint,_atomic_dec_32)
ALIAS(atomic_dec_ulong,_atomic_dec_32)
ALIAS(atomic_dec_ptr,_atomic_dec_32)

ALIAS(atomic_dec_32_nv,_atomic_dec_32_nv)
ALIAS(atomic_dec_uint_nv,_atomic_dec_32_nv)
ALIAS(atomic_dec_ulong_nv,_atomic_dec_32_nv)
ALIAS(atomic_dec_ptr_nv,_atomic_dec_32_nv)

ALIAS(atomic_inc_32,_atomic_inc_32)
ALIAS(atomic_inc_uint,_atomic_inc_32)
ALIAS(atomic_inc_ulong,_atomic_inc_32)
ALIAS(atomic_inc_ptr,_atomic_inc_32)

ALIAS(atomic_inc_32_nv,_atomic_inc_32_nv)
ALIAS(atomic_inc_uint_nv,_atomic_inc_32_nv)
ALIAS(atomic_inc_ulong_nv,_atomic_inc_32_nv)
ALIAS(atomic_inc_ptr_nv,_atomic_inc_32_nv)

ALIAS(atomic_or_32,_atomic_or_32)
ALIAS(atomic_or_uint,_atomic_or_32)
ALIAS(atomic_or_ulong,_atomic_or_32)
ALIAS(atomic_or_ptr,_atomic_or_32)

ALIAS(atomic_or_32_nv,_atomic_or_32_nv)
ALIAS(atomic_or_uint_nv,_atomic_or_32_nv)
ALIAS(atomic_or_ulong_nv,_atomic_or_32_nv)
ALIAS(atomic_or_ptr_nv,_atomic_or_32_nv)

ALIAS(atomic_swap_32,_atomic_swap_32)
ALIAS(atomic_swap_uint,_atomic_swap_32)
ALIAS(atomic_swap_ulong,_atomic_swap_32)
ALIAS(atomic_swap_ptr,_atomic_swap_32)

ALIAS(atomic_cas_32,_atomic_cas_32)
ALIAS(atomic_cas_uint,_atomic_cas_32)
ALIAS(atomic_cas_ulong,_atomic_cas_32)
ALIAS(atomic_cas_ptr,_atomic_cas_32)

ALIAS(atomic_cas_64,_atomic_cas_64)

ALIAS(membar_consumer,_membar_consumer)
ALIAS(membar_producer,_membar_producer)
ALIAS(membar_enter,_membar_enter)
ALIAS(membar_exit,_membar_exit)
ALIAS(membar_sync,_membar_sync)

STRONG_ALIAS(_atomic_add_int,_atomic_add_32)
STRONG_ALIAS(_atomic_add_long,_atomic_add_32)
STRONG_ALIAS(_atomic_add_ptr,_atomic_add_32)

STRONG_ALIAS(_atomic_add_int_nv,_atomic_add_32_nv)
STRONG_ALIAS(_atomic_add_long_nv,_atomic_add_32_nv)
STRONG_ALIAS(_atomic_add_ptr_nv,_atomic_add_32_nv)

STRONG_ALIAS(_atomic_and_uint,_atomic_and_32)
STRONG_ALIAS(_atomic_and_ulong,_atomic_and_32)
STRONG_ALIAS(_atomic_and_ptr,_atomic_and_32)

STRONG_ALIAS(_atomic_and_uint_nv,_atomic_and_32_nv)
STRONG_ALIAS(_atomic_and_ulong_nv,_atomic_and_32_nv)
STRONG_ALIAS(_atomic_and_ptr_nv,_atomic_and_32_nv)

STRONG_ALIAS(_atomic_dec_uint,_atomic_dec_32)
STRONG_ALIAS(_atomic_dec_ulong,_atomic_dec_32)
STRONG_ALIAS(_atomic_dec_ptr,_atomic_dec_32)

STRONG_ALIAS(_atomic_dec_uint_nv,_atomic_dec_32_nv)
STRONG_ALIAS(_atomic_dec_ulong_nv,_atomic_dec_32_nv)
STRONG_ALIAS(_atomic_dec_ptr_nv,_atomic_dec_32_nv)

STRONG_ALIAS(_atomic_inc_uint,_atomic_inc_32)
STRONG_ALIAS(_atomic_inc_ulong,_atomic_inc_32)
STRONG_ALIAS(_atomic_inc_ptr,_atomic_inc_32)

STRONG_ALIAS(_atomic_inc_uint_nv,_atomic_inc_32_nv)
STRONG_ALIAS(_atomic_inc_ulong_nv,_atomic_inc_32_nv)
STRONG_ALIAS(_atomic_inc_ptr_nv,_atomic_inc_32_nv)

STRONG_ALIAS(_atomic_or_uint,_atomic_or_32)
STRONG_ALIAS(_atomic_or_ulong,_atomic_or_32)
STRONG_ALIAS(_atomic_or_ptr,_atomic_or_32)

STRONG_ALIAS(_atomic_or_uint_nv,_atomic_or_32_nv)
STRONG_ALIAS(_atomic_or_ulong_nv,_atomic_or_32_nv)
STRONG_ALIAS(_atomic_or_ptr_nv,_atomic_or_32_nv)

STRONG_ALIAS(_atomic_swap_uint,_atomic_swap_32)
STRONG_ALIAS(_atomic_swap_ulong,_atomic_swap_32)
STRONG_ALIAS(_atomic_swap_ptr,_atomic_swap_32)

STRONG_ALIAS(_atomic_cas_uint,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ulong,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ptr,_atomic_cas_32)
