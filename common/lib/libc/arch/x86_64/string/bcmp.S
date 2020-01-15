/*	$NetBSD: bcmp.S,v 1.4 2020/01/15 10:56:49 ad Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: bcmp.S,v 1.4 2020/01/15 10:56:49 ad Exp $")
#endif

ENTRY(bcmp)
	movq	%rdx, %rcx	/* compare by longs, equality only */
	shrq	$3, %rcx
	jz	2f
1:
	movq	(%rdi), %rax
	cmpq	%rax, (%rsi)
	jne	5f
	decq	%rcx
	leaq	8(%rdi), %rdi
	leaq	8(%rsi), %rsi
	jnz	1b
2:
	andl	$7, %edx
	jz	4f
3:
	movb	(%rdi), %al	/* compare by chars, equality only */
	cmpb	%al, (%rsi)
	jne	5f
	decl	%edx
	leaq	1(%rdi), %rdi
	leaq	1(%rsi), %rsi
	jnz	3b
4:
	xorl	%eax, %eax
	ret
5:
	movl	$1, %eax
	ret
END(bcmp)
