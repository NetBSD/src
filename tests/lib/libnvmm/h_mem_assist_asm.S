/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

	.globl	test1_begin, test1_end
	.globl	test2_begin, test2_end
	.globl	test3_begin, test3_end
	.globl	test4_begin, test4_end
	.globl	test5_begin, test5_end
	.globl	test6_begin, test6_end
	.globl	test7_begin, test7_end
	.globl	test8_begin, test8_end
	.text
	.code64

#define TEST_END	\
	movq	$0xFFFFFFFFFFFFFFFF,%rcx;	\
	rdmsr	;

	.align	64
test1_begin:
	movq	$0x1000,%rax
	movq	$0x1000,%rbp

	movq	$0x1000,(%rax)

	movq	$1,%r11
	movq	$0x2000,(%rax,%r11,8)

	movq	(%rbp),%r8
	movq	8(%rbp),%rbx
	addq	%rbx,%r8
	movq	%r8,(%rbp)
	movb	$4,(%rbp)

	TEST_END
test1_end:

	.align	64
test2_begin:
	movq	$0x1000,%rax

	movq	$0x1000,(%rax)
	movq	$0x00FF,%rbx
	orb	%bl,(%rax)
	movq	$0x0400,%rcx
	orw	%cx,(%rax)

	TEST_END
test2_end:

	.align	64
test3_begin:
	movq	$0x1000,%rax

	movq	$0x1FFF,(%rax)
	movq	$0x1FF0,%rbx
	andq	%rbx,(%rax)
	movq	$0x10C1,%rcx
	andb	%cl,(%rax)

	TEST_END
test3_end:

	.align	64
test4_begin:
	movq	$0x1000,%rax

	movq	$0x1FFF,(%rax)
	movq	$0x1FF0,%rbx
	xorq	%rbx,(%rax)
	movq	$0x10C0,%rcx
	xorw	%cx,(%rax)

	TEST_END
test4_end:

	.align	64
test5_begin:
	movq	$0xFFFFFFFF00001000,%rax

	movq	$0x1FFF,(%eax)
	movb	$0,(%eax,%ebx,1)

	TEST_END
test5_end:

	.align	64
test6_begin:
	movq	$0xFFA0,%rax
	movabs	%rax,0x1000

	movabs	0x1000,%al
	orb	$0x0B,%al
	movabs	%al,0x1000

	TEST_END
test6_end:

	.align	64
test7_begin:
	movq	$0x56,%rax

	movq	$1,%rcx
	movq	$0x1000,%rdi
	rep	stosb

	movq	$0x1234,%rax
	stosw

	TEST_END
test7_end:

	.align	64
test8_begin:
	movq	$0x1008,%rsi
	movq	$0x12345678,(%rsi)

	movq	$0x1000,%rdi

	lodsw
	movw	%ax,(%rdi)
	addq	$2,%rdi

	lodsb
	movb	%al,(%rdi)
	addq	$1,%rdi

	lodsb
	movb	%al,(%rdi)
	addq	$2,%rdi

	TEST_END
test8_end:
