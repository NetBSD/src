/*	$NetBSD: in_cksum.s,v 1.2 1996/07/03 13:07:17 mycroft Exp $	*/

/*-
 * Copyright (c) 1994, 1995, 1996 Charles M. Hannum.  All rights reserved.
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
 * %ebx = m
 * %cl = rotation count to unswap
 * %edx = len
 * %esi = m->m_len
 * %ebp = m->m_data
 */

#define	SWAP \
	roll	$8, %eax	; \
	xorb	$8, %cl

#define	UNSWAP \
	roll	%cl, %eax

#define	MOP \
	adcl	$0, %eax

#define	ADVANCE(n) \
	leal	n(%ebp), %ebp	; \
	leal	-n(%esi), %esi	; \

#define	ADDBYTE \
	SWAP			; \
	addb	(%ebp), %ah

#define	ADDWORD \
	addw	(%ebp), %ax

#define	ADD(n) \
	addl	n(%ebp), %eax

#define	ADC(n) \
	adcl	n(%ebp), %eax

#define	REDUCE \
	movzwl	%ax, %esi	; \
	shrl	$16, %eax	; \
	addw	%si, %ax	; \
	adcw	$0, %ax

ENTRY(in_cksum)
	pushl	%ebp
	pushl	%ebx
	pushl	%esi

	movl	16(%esp), %ebx
	movl	20(%esp), %edx
	xorl	%eax, %eax
	xorb	%cl, %cl

mbuf_loop_1:
	testl	%edx, %edx
	jz	done

mbuf_loop_2:
	testl	%ebx, %ebx
	jz	out_of_mbufs

	movl	M_DATA(%ebx), %ebp
	movl	M_LEN(%ebx), %esi
	movl	M_NEXT(%ebx), %ebx

	cmpl	%edx, %esi
	jbe	1f
	movl	%edx, %esi

1:
	subl	%esi, %edx

	cmpl	$16, %esi
	jb	short_mbuf

	testl	$3, %ebp
	jz	dword_aligned

	testl	$1, %ebp
	jz	byte_aligned

	ADDBYTE
	ADVANCE(1)
	MOP

	testl	$2, %ebp
	jz	word_aligned

byte_aligned:
	ADDWORD
	ADVANCE(2)
	MOP

word_aligned:
dword_aligned:
	testl	$4, %ebp
	jnz	qword_aligned

	ADD(0)
	ADVANCE(4)
	MOP

qword_aligned:
	testl	$8, %ebp
	jz	oword_aligned

	ADD(0)
	ADC(4)
	ADVANCE(8)
	MOP

oword_aligned:
	subl	$128, %esi
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
	leal	128(%ebp), %ebp
	MOP

	subl	$128, %esi
	jnb	loop_128

finished_128:
	addl	$128, %esi

	subl	$32, %esi
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
	leal	32(%ebp), %ebp
	MOP

	subl	$32, %esi
	jnb	loop_32

finished_32:
	testl	$16, %esi
	jz	finished_16

	ADD(12)
	ADC(0)
	ADC(4)
	ADC(8)
	leal	16(%ebp), %ebp
	MOP

finished_16:
short_mbuf:
	testl	$8, %esi
	jz	finished_8

	ADD(0)
	ADC(4)
	leal	8(%ebp), %ebp
	MOP

finished_8:
	testl	$4, %esi
	jz	finished_4

	ADD(0)
	leal	4(%ebp), %ebp
	MOP

finished_4:
	testl	$3, %esi
	jz	mbuf_loop_1

	testl	$2, %esi
	jz	finished_2

	ADDWORD
	leal	2(%ebp), %ebp
	MOP

	testl	$1, %esi
	jz	finished_1

finished_2:
	ADDBYTE
	MOP

finished_1:
mbuf_done:
	testl	%edx, %edx
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
	call	_printf
	leal	4(%esp), %esp
	jmp	return
1:
	.asciz	"cksum: out of data\n"
