/*	$NetBSD: ashiftrt.S,v 1.1 2000/01/05 14:07:31 msaitoh Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

	.globl	___ashiftrt_r4_0, ___ashiftrt_r4_1, ___ashiftrt_r4_2
	.globl	___ashiftrt_r4_3, ___ashiftrt_r4_4, ___ashiftrt_r4_5
	.globl	___ashiftrt_r4_6, ___ashiftrt_r4_7, ___ashiftrt_r4_8
	.globl	___ashiftrt_r4_9, ___ashiftrt_r4_10, ___ashiftrt_r4_11
	.globl	___ashiftrt_r4_12, ___ashiftrt_r4_13, ___ashiftrt_r4_14
	.globl	___ashiftrt_r4_15, ___ashiftrt_r4_16, ___ashiftrt_r4_17
	.globl	___ashiftrt_r4_18, ___ashiftrt_r4_19, ___ashiftrt_r4_20
	.globl	___ashiftrt_r4_21, ___ashiftrt_r4_22, ___ashiftrt_r4_23
	.globl	___ashiftrt_r4_24, ___ashiftrt_r4_25, ___ashiftrt_r4_26
	.globl	___ashiftrt_r4_27, ___ashiftrt_r4_28, ___ashiftrt_r4_29
	.globl	___ashiftrt_r4_30, ___ashiftrt_r4_31
	.text

___ashiftrt_r4_31:
	shll	r4
	rts
	subc	r4, r4

___ashiftrt_r4_30:
	shar	r4

___ashiftrt_r4_29:
	shar	r4

___ashiftrt_r4_28:
	shar	r4

___ashiftrt_r4_27:
	shar	r4

___ashiftrt_r4_26:
	shar	r4

___ashiftrt_r4_25:
	shar	r4

___ashiftrt_r4_24:
	shlr16	r4
	shlr8	r4
	rts
	exts.b	r4, r4

___ashiftrt_r4_23:
	shar	r4

___ashiftrt_r4_22:
	shar	r4

___ashiftrt_r4_21:
	shar	r4

___ashiftrt_r4_20:
	shar	r4

___ashiftrt_r4_19:
	shar	r4

___ashiftrt_r4_18:
	shar	r4

___ashiftrt_r4_17:
	shar	r4

___ashiftrt_r4_16:
	shlr16	r4
	rts
	exts.w	r4, r4

___ashiftrt_r4_15:
	swap.w	r4, r4
	cmp/pz	r4
	bf/s	1f
	exts.w	r4, r4
	rts
	shll	r4
1:
	sett
	rts
	rotcl	r4

___ashiftrt_r4_14:
	shar	r4

___ashiftrt_r4_13:
	shar	r4

___ashiftrt_r4_12:
	shar	r4

___ashiftrt_r4_11:
	shar	r4

___ashiftrt_r4_10:
	shar	r4

___ashiftrt_r4_9:
	shar	r4

___ashiftrt_r4_8:
	shar	r4

___ashiftrt_r4_7:
	shar	r4

___ashiftrt_r4_6:
	shar	r4

___ashiftrt_r4_5:
	shar	r4

___ashiftrt_r4_4:
	shar	r4

___ashiftrt_r4_3:
	shar	r4

___ashiftrt_r4_2:
	shar	r4

___ashiftrt_r4_1:
	rts
	shar	r4

___ashiftrt_r4_0:
	rts
	nop
