/*	$NetBSD: cpu_in_cksum.S,v 1.1 2018/04/25 11:06:49 ragge Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Anders Magnusson.
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
 * Copyright (c) 1988, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *
 *	@(#)in_cksum.c	8.1 (Berkeley) 6/10/93
 */

/*
 * Assembly version of cpu_in_cksum() for vax, following the structure
 * in the C version of the file but using vax instructions for speed.
 * Increases network traffic speed with almost 50% (NFS tests).
 */

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_in_cksum.S,v 1.1 2018/04/25 11:06:49 ragge Exp $");

#include "assym.h"

#define off	%r0
#define mlen	%r1
#define m	%r2
#define data	%r3
#define sum	%r4
#define len	%r5
#define byte_swapped	%r6
#define tmp	%r7
#
# int cpu_in_cksum(struct mbuf *m, int len, int off, uint32_t initial_sum)
#
ENTRY(cpu_in_cksum, R7|R6)

	subl2 $4,%sp

	movl 4(%ap),m
	movl 8(%ap),len
	movl 12(%ap),off
	movl 16(%ap),sum

	clrl byte_swapped

.Lfirstloop:				# for (;;) {
	tstl m				# if (__predict_false(m == NULL)) {
	jeql .Lout_of_data

	movl M_LEN(m),mlen		# mlen = m->m_len;
	cmpl off,mlen			# if (mlen > off) {
	jgeq 1f
	subl2 off,mlen			#	mlen -= off;
	addl3 M_DATA(m),off,data	#	data = mtod(m, uint8_t *) + off;
	jbr .Lpost_initial_offset	#	goto post_initial_offset;
1:					# }
	subl2 mlen,off			# off -= mlen;
	tstl len			# if (len == 0)
	jeql .Lsecondloop		#	break;
	movl M_NEXT(m),m		# m = m->m_next;
	jbr .Lfirstloop			# }

.Lthirdstmt:
	movl M_NEXT(m),m		# m = m->m_next) {
.Lsecondloop:				# for (; 
	tstl len			# len > 0;
	jeql .Lendsecond
	tstl m				# if (__predict_false(m == NULL)) {
	jeql .Lout_of_data

	movl M_LEN(m),mlen		# mlen = m->m_len;
	movl M_DATA(m),data		# data = mtod(m, uint8_t *);
.Lpost_initial_offset:
	tstl mlen			# if (mlen == 0)
	jeql .Lthirdstmt		#	continue;
	cmpl len,mlen			# if (mlen > len)
	jgeq 1f				
	movl len,mlen			#	mlen = len;
1:	subl2 mlen,len			# len -= mlen
	cmpl mlen,$16			# if (mlen < 16)
	jlss .Lshort_mbuf		#	goto short_mbuf;
#
#	Align on longword boundary
#
	blbc data,1f			# if ((uintptr_t)data & 1) {
	movzbl (data)+,tmp		#	tmp = *data++;
	addl2 tmp,sum			#	sum += tmp;
	adwc $0,sum
	rotl $8,sum,sum			#	sum = (sum << 8 | sum >> 24);
	xorl2 $1,byte_swapped		#	byte_swapped ^= 1;
	decl mlen			#	mlen--;
1:					# }
	bbc $1,data,1f			# if ((uintptr_t)data & 2) {
	movzwl (data)+,tmp		#	tmp = *data++; (word *)
	addl2 tmp,sum			#	sum += tmp;
	adwc $0,sum
	subl2 $2,mlen			#	mlen -= 2;
1:					# }
#
# Add 16 word in a chunk
#
2:	subl2 $32,mlen			# while ((mlen -= 32) >= 0) {
	jlss 1f
	addl2 (data)+,sum		#	sum += *(uint32_t *)data;
	adwc (data)+,sum		#	sum += *(uint32_t *)data;
	adwc (data)+,sum		#	sum += *(uint32_t *)data;
	adwc (data)+,sum		#	sum += *(uint32_t *)data;
	adwc (data)+,sum		#	sum += *(uint32_t *)data;
	adwc (data)+,sum		#	sum += *(uint32_t *)data;
	adwc (data)+,sum		#	sum += *(uint32_t *)data;
	adwc (data)+,sum		#	sum += *(uint32_t *)data;
	adwc $0,sum
	jbr 2b				# }

1:	addl2 $32,mlen			# mlen += 32;
	bbc $4,mlen,1f			# if (mlen >= 16) {
	addl2 (data)+,sum		#	sum += *(uint32_t *)data;
	adwc (data)+,sum		#	sum += *(uint32_t *)data;
	adwc (data)+,sum		#	sum += *(uint32_t *)data;
	adwc (data)+,sum		#	sum += *(uint32_t *)data;
	adwc $0,sum
	subl2 $16,mlen			#	mlen -= 16;
1:					# }

.Lshort_mbuf:				# short_mbuf:
	bbc $3,mlen,1f			# if (mlen >= 8) {
	addl2 (data)+,sum		#	sum += *(uint32_t *)data;
	adwc (data)+,sum		#	sum += *(uint32_t *)data;
	adwc $0,sum
	subl2 $8,mlen			#	mlen -= 8;
1:					# }
	bbc $2,mlen,1f			# if (mlen >= 4) {
	addl2 (data)+,sum		#	sum += *(uint32_t *)data;
	adwc $0,sum
	subl2 $4,mlen			#	mlen -= 4;
1:					# }

	bbc $1,mlen,1f			# if (mlen >= 2) {
	movzwl (data)+,tmp		#	tmp = *data++; (word *)
	addl2 tmp,sum			#	sum += tmp;
	adwc $0,sum
1:					# }
	blbc mlen,1f			# if (mlen & 1) {
	movzbl (data)+,tmp		#	tmp = *data++;
	addl2 tmp,sum			#	sum += tmp;
	adwc $0,sum
	rotl $8,sum,sum			#	sum = (sum << 8 | sum >> 24);
	xorl2 $1,byte_swapped		#	byte_swapped ^= 1;
1:					# }
	jbr .Lthirdstmt

.Lendsecond:
	tstl len			# if (len != 0)
	jneq .Lout_of_data		#	goto out_of_data;
	tstl byte_swapped		# if (byte_swapped) {
	jeql 1f
	rotl $8,sum,sum			# sum = (sum << 8 | sum >> 24);
1:	rotl $16,sum,tmp		# tmp = sum >> 16;
	addw2 tmp,sum			# sum(16) += tmp;
	bicl2 $0xffff0000,sum		# sum &= ~0xffff0000;
	adwc $0,sum
	xorl3 $0xffff,sum,%r0		# return (sum ^ 0xffff);
	ret
.Lout_of_data:
	pushab .Lin_cksum
	calls $1,printf
	mnegl $1,%r0
	ret

	.section	.rodata
.Lin_cksum:
	.asciz "in_cksum: out of data\n"

