/*	$NetBSD: atomic_or.S,v 1.1.2.3 2007/04/26 04:04:39 thorpej Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include "atomic_op_asm.h"

	.text
	.align	64

NENTRY(_atomic_or_8)
	LOCK
	orb	%sil, (%rdi)
	ret
ATOMIC_OP_ALIAS(atomic_or_8,_atomic_or_8)
ATOMIC_OP_ALIAS(atomic_or_uchar,_atomic_or_8)
STRONG_ALIAS(_atomic_or_uchar,_atomic_or_8)

NENTRY(_atomic_or_8_nv)
	movzbl	(%rdi), %eax
1:	movb	%sil, %dl
	orb	%al, %dl
	LOCK
	cmpxchgb %dl, (%rdi)
	/* %eax now contains the old value */
	jne	1b
	movzbl	%dl, %eax
	ret
ATOMIC_OP_ALIAS(atomic_or_8_nv,_atomic_or_8_nv)
ATOMIC_OP_ALIAS(atomic_or_uchar_nv,_atomic_or_8_nv)
STRONG_ALIAS(_atomic_or_uchar_nv,_atomic_or_8_nv)

NENTRY(_atomic_or_16)
	LOCK
	orw	%si, (%rdi)
	ret
ATOMIC_OP_ALIAS(atomic_or_16,_atomic_or_16)
ATOMIC_OP_ALIAS(atomic_or_ushort,_atomic_or_16)
STRONG_ALIAS(_atomic_or_ushort,_atomic_or_16)

NENTRY(_atomic_or_16_nv)
	movzwl	(%rdi), %eax
1:	movw	%si, %dx
	orw	%ax, %dx
	LOCK
	cmpxchgw %dx, (%rdi)
	/* %eax now contains the old value */
	jne	1b
	movzwl	%dx, %eax
	ret
ATOMIC_OP_ALIAS(atomic_or_16_nv,_atomic_or_16_nv)
ATOMIC_OP_ALIAS(atomic_or_ushort_nv,_atomic_or_16_nv)
STRONG_ALIAS(_atomic_or_ushort_nv,_atomic_or_16_nv)

NENTRY(_atomic_or_32)
	LOCK
	orl	%esi, (%rdi)
	ret
ATOMIC_OP_ALIAS(atomic_or_32,_atomic_or_32)
ATOMIC_OP_ALIAS(atomic_or_uint,_atomic_or_32)
STRONG_ALIAS(_atomic_or_uint,_atomic_or_32)

NENTRY(_atomic_or_32_nv)
	movl	(%rdi), %eax
1:	movl	%esi, %edx
	orl	%eax, %edx
	LOCK
	cmpxchgl %edx, (%rdi)
	/* %eax now contains the old value */
	jne	1b
	movl	%edx, %eax
	ret
ATOMIC_OP_ALIAS(atomic_or_32_nv,_atomic_or_32_nv)
ATOMIC_OP_ALIAS(atomic_or_uint_nv,_atomic_or_32_nv)
STRONG_ALIAS(_atomic_or_uint_nv,_atomic_or_32_nv)

NENTRY(_atomic_or_64)
	LOCK
	orq	%rsi, (%rdi)
	ret
ATOMIC_OP_ALIAS(atomic_or_64,_atomic_or_64)
ATOMIC_OP_ALIAS(atomic_or_ulong,_atomic_or_64)
STRONG_ALIAS(_atomic_or_ulong,_atomic_or_64)

NENTRY(_atomic_or_64_nv)
	movq	(%rdi), %rax
1:	movq	%rsi, %rdx
	orq	%rax, %rdx
	LOCK
	cmpxchgq %rdx, (%rdi)
	/* %rax now contains the old value */
	jne	1b
	movq	%rdx, %rax
	ret
ATOMIC_OP_ALIAS(atomic_or_64_nv,_atomic_or_64_nv)
ATOMIC_OP_ALIAS(atomic_or_ulong_nv,_atomic_or_64_nv)
STRONG_ALIAS(_atomic_or_ulong_nv,_atomic_or_64_nv)
