/*	$NetBSD: busfunc.S,v 1.3.14.3 2008/01/09 01:44:42 matt Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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

#include <machine/asm.h>

#include "assym.h"

/* XXX */
#undef	_ALIGN_TEXT
#define	_ALIGN_TEXT	.align 16

#if X86_BUS_SPACE_IO != 0
#error depends on X86_BUS_SPACE_IO == 0
#endif

/*
 * uint8_t bus_space_read_1(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset);
 */
NENTRY(bus_space_read_1)
	addq	%rsi, %rdx
	testl	%edi, %edi
	je	1f
	movzbl	(%rdx), %eax
	ret
1:
	xorl	%eax, %eax
	inb	%dx, %al
	ret

/*
 * uint16_t bus_space_read_2(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset);
 */
NENTRY(bus_space_read_2)
	addq	%rsi, %rdx
	testl	%edi, %edi
	je	1f
	movzwl	(%rdx), %eax
	ret
1:
	xorl	%eax, %eax
	inw	%dx, %ax
	ret

/*
 * uint32_t bus_space_read_4(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset);
 */
NENTRY(bus_space_read_4)
	addq	%rsi, %rdx
	testl	%edi, %edi
	je	1f
	movl	(%rdx), %eax
	ret
1:
	inl	%dx, %eax
	ret

STRONG_ALIAS(bus_space_read_stream_1,bus_space_read_1)
STRONG_ALIAS(bus_space_read_stream_2,bus_space_read_2)
STRONG_ALIAS(bus_space_read_stream_4,bus_space_read_4)

/*
 * void bus_space_write_1(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, uint8_t value);
 */
NENTRY(bus_space_write_1)
	addq	%rsi, %rdx
	testl	%edi, %edi
	je	1f
	movb	%cl, (%rdx)
	ret
1:
	movl	%ecx, %eax
	outb	%al, %dx
	ret

/*
 * void bus_space_write_2(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, uint16_t value);
 */
NENTRY(bus_space_write_2)
	addq	%rsi, %rdx
	testl	%edi, %edi
	je	1f
	movw	%cx, (%rdx)
	ret
1:
	movl	%ecx, %eax
	outw	%ax, %dx
	ret

/*
 * void bus_space_write_4(bus_space_tag_t tag, bus_space_handle_t bsh,
 *     bus_size_t offset, uint32_t value);
 */
NENTRY(bus_space_write_4)
	addq	%rsi, %rdx
	testl	%edi, %edi
	je	1f
	movl	%ecx, (%rdx)
	ret
1:
	movl	%ecx, %eax
	outl	%eax, %dx
	ret

STRONG_ALIAS(bus_space_write_stream_1,bus_space_write_1)
STRONG_ALIAS(bus_space_write_stream_2,bus_space_write_2)
STRONG_ALIAS(bus_space_write_stream_4,bus_space_write_4)

/*
 * void bus_space_read_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, uint8_t *addr, size_t count);
 */
NENTRY(bus_space_read_multi_1)
	testl	%edi, %edi
	leaq	(%rsi,%rdx,1), %rdx
	jne	1f
	movq	%rcx, %rdi
	movq	%r8, %rcx
	rep
	insb	(%dx), %es:(%rdi)
	ret
	.align	16
1:
	movb	(%rdx), %al
	decq	%r8
	movb	%al, (%rcx)
	leaq	1(%rcx), %rcx
	jnz	1b
	ret

/*
 * void bus_space_read_multi_2(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, uint16_t *addr, size_t count);
 */
NENTRY(bus_space_read_multi_2)
	testl	%edi, %edi
	leaq	(%rsi,%rdx,1), %rdx
	jne	1f
	movq	%rcx, %rdi
	movq	%r8, %rcx
	rep
	insw	(%dx), %es:(%rdi)
	ret
	.align	16
1:
	movw	(%rdx), %ax
	decq	%r8
	movw	%ax, (%rcx)
	leaq	2(%rcx), %rcx
	jnz	1b
	ret

/*
 * void bus_space_read_multi_4(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, uint32_t *addr, size_t count);
 */
NENTRY(bus_space_read_multi_4)
	testl	%edi, %edi
	leaq	(%rsi,%rdx,1), %rdx
	jne	1f
	movq	%rcx, %rdi
	movq	%r8, %rcx
	rep
	insl	(%dx), %es:(%rdi)
	ret
	.align	16
1:
	movl	(%rdx), %eax
	decq	%r8
	movl	%eax, (%rcx)
	leaq	4(%rcx), %rcx
	jnz	1b
	ret

STRONG_ALIAS(bus_space_read_multi_stream_1,bus_space_read_multi_1)
STRONG_ALIAS(bus_space_read_multi_stream_2,bus_space_read_multi_2)
STRONG_ALIAS(bus_space_read_multi_stream_4,bus_space_read_multi_4)

/*
 * void bus_space_write_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, const uint8_t *addr, size_t count);
 */
NENTRY(bus_space_write_multi_1)
	testl	%edi, %edi
	leaq	(%rsi,%rdx,1), %rdx
	jne	1f
	movq	%rcx, %rsi
	movq	%r8, %rcx
	rep
	outsb	%ds:(%rsi), (%dx)
	ret
	.align	16
1:
	movb	(%rcx), %al
	decq	%r8
	movb	%al, (%rdx)
	leaq	1(%rcx), %rcx
	jnz	1b
	ret

/*
 * void bus_space_write_multi_2(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, const uint16_t *addr, size_t count);
 */
NENTRY(bus_space_write_multi_2)
	testl	%edi, %edi
	leaq	(%rsi,%rdx,1), %rdx
	jne	1f
	movq	%rcx, %rsi
	movq	%r8, %rcx
	rep
	outsw	%ds:(%rsi), (%dx)
	ret
	.align	16
1:
	movw	(%rcx), %ax
	decq	%r8
	movw	%ax, (%rdx)
	leaq	2(%rcx), %rcx
	jnz	1b
	ret

/*
 * void bus_space_write_multi_4(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, const uint32_t *addr, size_t count);
 */
NENTRY(bus_space_write_multi_4)
	testl	%edi, %edi
	leaq	(%rsi,%rdx,1), %rdx
	jne	1f
	movq	%rcx, %rsi
	movq	%r8, %rcx
	rep
	outsl	%ds:(%rsi), (%dx)
	ret
	.align	16
1:
	movl	(%rcx), %eax
	decq	%r8
	movl	%eax, (%rdx)
	leaq	4(%rcx), %rcx
	jnz	1b
	ret

STRONG_ALIAS(bus_space_write_multi_stream_1,bus_space_write_multi_1)
STRONG_ALIAS(bus_space_write_multi_stream_2,bus_space_write_multi_2)
STRONG_ALIAS(bus_space_write_multi_stream_4,bus_space_write_multi_4)

/*
 * void bus_space_read_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, uint8_t *addr, size_t count);
 */
NENTRY(bus_space_read_region_1)
	testl	%edi, %edi
	je	2f
1:
	addq	%rdx, %rsi
	movq	%rcx, %rdi
	movq	%r8, %rcx
	rep
	movsb	%ds:(%rsi), %es:(%rdi)
	ret
2:
	addl	%esi, %edx
3:
	inb	%dx, %al
	incl	%edx
	decq	%r8
	movb	%al, (%rcx)
	leaq	1(%rcx), %rcx
	jnz	3b
	ret

/*
 * void bus_space_read_region_2(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, uint16_t *addr, size_t count);
 */
NENTRY(bus_space_read_region_2)
	testl	%edi, %edi
	je	2f
1:
	addq	%rdx, %rsi
	movq	%rcx, %rdi
	movq	%r8, %rcx
	rep
	movsw	%ds:(%rsi), %es:(%rdi)
	ret
2:
	addl	%esi, %edx
3:
	inw	%dx, %ax
	addl	$2, %edx
	decq	%r8
	movw	%ax, (%rcx)
	leaq	2(%rcx), %rcx
	jnz	3b
	ret

/*
 * void bus_space_read_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, uint32_t *addr, size_t count);
 */
NENTRY(bus_space_read_region_4)
	testl	%edi, %edi
	je	2f
1:
	addq	%rdx, %rsi
	movq	%rcx, %rdi
	movq	%r8, %rcx
	rep
	movsl	%ds:(%rsi), %es:(%rdi)
	ret
2:
	addl	%esi, %edx
3:
	inl	%dx, %eax
	addl	$4, %edx
	decq	%r8
	movl	%eax, (%rcx)
	leaq	4(%rcx), %rcx
	jnz	3b
	ret

STRONG_ALIAS(bus_space_read_region_stream_1,bus_space_read_region_1)
STRONG_ALIAS(bus_space_read_region_stream_2,bus_space_read_region_2)
STRONG_ALIAS(bus_space_read_region_stream_4,bus_space_read_region_4)

/*
 * void bus_space_write_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, const uint8_t *addr, size_t count);
 */
NENTRY(bus_space_write_region_1)
	testl	%edi, %edi
	je	2f
1:
	leaq	(%rdx,%rsi,1), %rdi
	movq	%rcx, %rsi
	movq	%r8, %rcx
	rep
	movsb	%ds:(%rsi), %es:(%rdi)
	ret
2:
	addl	%esi, %edx
3:
	movb	(%rcx), %al
	incq	%rcx
	decq	%r8
	outb	%al, %dx
	leaq	1(%rdx), %rdx
	jnz	3b
	ret

/*
 * void bus_space_write_region_2(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, const uint16_t *addr, size_t count);
 */
NENTRY(bus_space_write_region_2)
	testl	%edi, %edi
	je	2f
1:
	leaq	(%rdx,%rsi,1), %rdi
	movq	%rcx, %rsi
	movq	%r8, %rcx
	rep
	movsw	%ds:(%rsi), %es:(%rdi)
	ret
2:
	addl	%esi, %edx
3:
	movw	(%rcx), %ax
	addq	$2, %rcx
	decq	%r8
	outw	%ax, %dx
	leaq	2(%rdx), %rdx
	jnz	3b
	ret

/*
 * void bus_space_write_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
 *    bus_size_t offset, const uint32_t *addr, size_t count);
 */
NENTRY(bus_space_write_region_4)
	testl	%edi, %edi
	je	2f
1:
	leaq	(%rdx,%rsi,1), %rdi
	movq	%rcx, %rsi
	movq	%r8, %rcx
	rep
	movsl	%ds:(%rsi), %es:(%rdi)
	ret
2:
	addl	%esi, %edx
3:
	movl	(%rcx), %eax
	addq	$4, %rcx
	decq	%r8
	outl	%eax, %dx
	leaq	4(%rdx), %rdx
	jnz	3b
	ret

STRONG_ALIAS(bus_space_write_region_stream_1,bus_space_write_region_1)
STRONG_ALIAS(bus_space_write_region_stream_2,bus_space_write_region_2)
STRONG_ALIAS(bus_space_write_region_stream_4,bus_space_write_region_4)
