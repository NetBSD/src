/*	$NetBSD: in_cksum.s,v 1.9.22.1 2001/03/30 22:29:06 he Exp $	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/asm.h>
#include "assym.h"

/*
 * Checksum routine for Internet Protocol family headers.
 *
 * in_cksum(m, len)
 *
 * Registers used:
 * %eax = sum
 * %ebx = m->m_data
 * %cl = rotation count to unswap
 * %edx = m->m_len
 * %ebp = m
 * %esi = len
 */

#define	SWAP \
	roll	$8, %eax	; \
	xorb	$8, %cl

#define	UNSWAP \
	roll	%cl, %eax

#define	MOP \
	adcl	$0, %eax

#define	ADVANCE(n) \
	leal	n(%ebx), %ebx	; \
	leal	-n(%edx), %edx	; \

#define	ADDBYTE \
	SWAP			; \
	addb	(%ebx), %ah

#define	ADDWORD \
	addw	(%ebx), %ax

#define	ADD(n) \
	addl	n(%ebx), %eax

#define	ADC(n) \
	adcl	n(%ebx), %eax

#define	REDUCE \
	movzwl	%ax, %edx	; \
	shrl	$16, %eax	; \
	addw	%dx, %ax	; \
	adcw	$0, %ax

ENTRY(in4_cksum)
	pushl	%ebp
	pushl	%ebx
	pushl	%esi

	movl	16(%esp), %ebp
	movzbl	20(%esp), %eax		/* sum = nxt */
	movl	28(%esp), %esi
	movl	M_DATA(%ebp), %ebx
	addl	%esi, %eax		/* sum += len */
	movl	24(%esp), %edx		/* %edx = off */
	shll	$8, %eax		/* sum = htons(sum) */

	ADD(IP_SRC)			/* sum += ip->ip_src */
	ADC(IP_DST)			/* sum += ip->ip_dst */
	MOP

mbuf_loop_0:
	testl	%ebp, %ebp
	jz	out_of_mbufs

	movl	M_DATA(%ebp), %ebx	/* %ebx = m_data */
	movl	M_LEN(%ebp), %ecx	/* %ecx = m_len */
	movl	M_NEXT(%ebp), %ebp

	subl	%ecx, %edx		/* %edx = off - m_len */
	jnb	mbuf_loop_0

	addl	%edx, %ebx		/* %ebx = m_data + off - m_len */
	negl	%edx			/* %edx = m_len - off */
	addl	%ecx, %ebx		/* %ebx = m_data + off */
	xorb	%cl, %cl

	/*
	 * The len == 0 case is handled really inefficiently, by going through
	 * the whole short_mbuf path once to get back to mbuf_loop_1 -- but
	 * this case never happens in practice, so it's sufficient that it
	 * doesn't explode.
	 */
	jmp	in4_entry


ENTRY(in_cksum)
	pushl	%ebp
	pushl	%ebx
	pushl	%esi

	movl	16(%esp), %ebp
	movl	20(%esp), %esi
	xorl	%eax, %eax
	xorb	%cl, %cl

mbuf_loop_1:
	testl	%esi, %esi
	jz	done

mbuf_loop_2:
	testl	%ebp, %ebp
	jz	out_of_mbufs

	movl	M_DATA(%ebp), %ebx
	movl	M_LEN(%ebp), %edx
	movl	M_NEXT(%ebp), %ebp

in4_entry:
	cmpl	%esi, %edx
	jbe	1f
	movl	%esi, %edx

1:
	subl	%edx, %esi

	cmpl	$32, %edx
	jb	short_mbuf

	testb	$3, %bl
	jz	dword_aligned

	testb	$1, %bl
	jz	byte_aligned

	ADDBYTE
	ADVANCE(1)
	MOP

	testb	$2, %bl
	jz	word_aligned

byte_aligned:
	ADDWORD
	ADVANCE(2)
	MOP

word_aligned:
dword_aligned:
	testb	$4, %bl
	jnz	qword_aligned

	ADD(0)
	ADVANCE(4)
	MOP

qword_aligned:
	testb	$8, %bl
	jz	oword_aligned

	ADD(0)
	ADC(4)
	ADVANCE(8)
	MOP

oword_aligned:
	subl	$128, %edx
	jb	finished_128

loop_128:
	ADD(12)
	ADC(0)
	ADC(4)
	ADC(8)
	ADC(28)
	ADC(16)
	ADC(20)
	ADC(24)
	ADC(44)
	ADC(32)
	ADC(36)
	ADC(40)
	ADC(60)
	ADC(48)
	ADC(52)
	ADC(56)
	ADC(76)
	ADC(64)
	ADC(68)
	ADC(72)
	ADC(92)
	ADC(80)
	ADC(84)
	ADC(88)
	ADC(108)
	ADC(96)
	ADC(100)
	ADC(104)
	ADC(124)
	ADC(112)
	ADC(116)
	ADC(120)
	leal	128(%ebx), %ebx
	MOP

	subl	$128, %edx
	jnb	loop_128

finished_128:
	subl	$32-128, %edx
	jb	finished_32

loop_32:
	ADD(12)
	ADC(0)
	ADC(4)
	ADC(8)
	ADC(28)
	ADC(16)
	ADC(20)
	ADC(24)
	leal	32(%ebx), %ebx
	MOP

	subl	$32, %edx
	jnb	loop_32

finished_32:
short_mbuf:
	testb	$16, %dl
	jz	finished_16

	ADD(12)
	ADC(0)
	ADC(4)
	ADC(8)
	leal	16(%ebx), %ebx
	MOP

finished_16:
	testb	$8, %dl
	jz	finished_8

	ADD(0)
	ADC(4)
	leal	8(%ebx), %ebx
	MOP

finished_8:
	testb	$4, %dl
	jz	finished_4

	ADD(0)
	leal	4(%ebx), %ebx
	MOP

finished_4:
	testb	$3, %dl
	jz	mbuf_loop_1

	testb	$2, %dl
	jz	finished_2

	ADDWORD
	leal	2(%ebx), %ebx
	MOP

	testb	$1, %dl
	jz	finished_1

finished_2:
	ADDBYTE
	MOP

finished_1:
mbuf_done:
	testl	%esi, %esi
	jnz	mbuf_loop_2

done:
	UNSWAP
	REDUCE
	notw	%ax

return:
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

out_of_mbufs:
	pushl	$1f
	call	_C_LABEL(printf)
	leal	4(%esp), %esp
	jmp	return
1:
	.asciz	"cksum: out of data\n"
