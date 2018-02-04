/* $NetBSD: bcopy.S,v 1.1 2018/02/04 21:52:16 skrll Exp $ */

/*
 * Copyright (c) 2018 Ryo Shimizu <ryo@nerv.org>
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
RCSID("$NetBSD: bcopy.S,v 1.1 2018/02/04 21:52:16 skrll Exp $")
#endif

#if defined(MEMCOPY)

/*
 * void *memcpy(void * restrict dst, const void * restrict src, size_t len);
 */
#define FUNCTION		memcpy
#define NO_OVERLAP
#define SRC0			x1
#define DST0			x0
#define LEN			x2

#elif defined(MEMMOVE)

/*
 * void *memmove(void *dst, const void *src, size_t len);
 */
#define FUNCTION		memmove
#undef NO_OVERLAP
#define SRC0			x1
#define DST0			x0
#define LEN			x2

#else /* !MEMCOPY && !MEMMOVE */

/*
 * void bcopy(const void *src, void *dst, size_t len);
 */
#define FUNCTION		bcopy
#define NO_OVERLAP
#define SRC0			x0
#define DST0			x1
#define LEN			x2

#endif /* MEMCOPY/MEMMOVE/BCOPY */

/* caller-saved temporary registers. breakable. */
#define TMP_X			x3
#define TMP_Xw			w3
#define TMP_D			x4
#define TMP_S			x5
#define DST			x6
#define SRC			x7
#define DATA0			x8
#define DATA0w			w8
#define DATA1			x9
#define DATA1w			w9
#define DATA2			x10
#define SRC_ALIGNBIT		x11	/* (SRC & 7) * 8 */
#define DST_ALIGNBIT		x12	/* (DST & 7) * 8 */
#define SRC_DST_ALIGNBIT	x13	/* = SRC_ALIGNBIT - DST_ALIGNBIT */
#define DST_SRC_ALIGNBIT	x14	/* = -SRC_DST_ALIGNBIT */

#define STP_ALIGN		16	/* align before stp/ldp. 8 or 16 */
#define SMALLSIZE		32

	.text
	.align	5

#ifndef NO_OVERLAP
#ifndef STRICT_ALIGNMENT
backward_ignore_align:
	prfm	PLDL1KEEP, [SRC0]
	add	SRC0, SRC0, LEN
	add	DST, DST0, LEN
	cmp	LEN, #SMALLSIZE
	bcs	copy_backward
copy_backward_small:
	cmp	LEN, #8
	bcs	9f

	/* 0 <= len < 8 */
	/* if (len & 4) { *--(uint32_t *)dst = *--(uint32_t *)src; } */
	tbz	LEN, #2, 1f
	ldr	TMP_Xw, [SRC0, #-4]!
	str	TMP_Xw, [DST, #-4]!
1:
	/* if (len & 2) { *--(uint16_t *)dst = *--(uint16_t *)src; } */
	tbz	LEN, #1, 1f
	ldrh	TMP_Xw, [SRC0, #-2]!
	strh	TMP_Xw, [DST, #-2]!
1:
	/* if (len & 1) { *--(uint8_t *)dst = *--(uint8_t *)src; } */
	tbz	LEN, #0, 1f
	ldrb	TMP_Xw, [SRC0, #-1]!
	strb	TMP_Xw, [DST, #-1]!
1:
	ret
9:

	cmp	LEN, #16
	bcs	9f

	/* 8 <= len < 16 */
	/* *--(uint64_t *)dst = *--(uint64_t *)src; */
	ldr	TMP_X, [SRC0, #-8]!
	str	TMP_X, [DST, #-8]!
	/* if (len & 4) { *--(uint32_t *)dst = *--(uint32_t *)src; } */
	tbz	LEN, #2, 1f
	ldr	TMP_Xw, [SRC0, #-4]!
	str	TMP_Xw, [DST, #-4]!
1:
	/* if (len & 2) { *--(uint16_t *)dst = *--(uint16_t *)src; } */
	tbz	LEN, #1, 1f
	ldrh	TMP_Xw, [SRC0, #-2]!
	strh	TMP_Xw, [DST, #-2]!
1:
	/* if (len & 1) { *--(uint8_t *)dst = *--(uint8_t *)src; } */
	tbz	LEN, #0, 1f
	ldrb	TMP_Xw, [SRC0, #-1]!
	strb	TMP_Xw, [DST, #-1]!
1:
	ret
9:

	/* 16 <= len < 32 */
	ldp	DATA0, DATA1, [SRC0, #-16]!
	stp	DATA0, DATA1, [DST, #-16]!
	/* if (len & 8) { *--(uint64_t *)dst = *--(uint64_t *)src; } */
	tbz	LEN, #3, 1f
	ldr	TMP_X, [SRC0, #-8]!
	str	TMP_X, [DST, #-8]!
1:
	/* if (len & 4) { *--(uint32_t *)dst = *--(uint32_t *)src; } */
	tbz	LEN, #2, 1f
	ldr	TMP_Xw, [SRC0, #-4]!
	str	TMP_Xw, [DST, #-4]!
1:
	/* if (len & 2) { *--(uint16_t *)dst = *--(uint16_t *)src; } */
	tbz	LEN, #1, 1f
	ldrh	TMP_Xw, [SRC0, #-2]!
	strh	TMP_Xw, [DST, #-2]!
1:
	/* if (len & 1) { *--(uint8_t *)dst = *--(uint8_t *)src; } */
	tbz	LEN, #0, 1f
	ldrb	TMP_Xw, [SRC0, #-1]!
	strb	TMP_Xw, [DST, #-1]!
1:
	ret
#endif /* !STRICT_ALIGNMENT */

	.align	4
copy_backward:
	/* DST is not aligned at this point */
#ifndef STRICT_ALIGNMENT
	cmp	LEN, #512	/* pre-alignment can be overhead when small */
	bcc	9f
#endif
	/* if (DST & 1) { *--(uint8_t *)dst = *--(uint8_t *)src; } */
	tbz	DST, #0, 1f
	ldrb	TMP_Xw, [SRC0, #-1]!
	strb	TMP_Xw, [DST, #-1]!
	sub	LEN, LEN, #1
1:
	/* if (DST & 2) { *--(uint16_t *)dst = *--(uint16_t *)src; } */
	tbz	DST, #1, 1f
	ldrh	TMP_Xw, [SRC0, #-2]!
	strh	TMP_Xw, [DST, #-2]!
	sub	LEN, LEN, #2
1:
	/* if (DST & 4) { *--(uint32_t *)dst = *--(uint32_t *)src; } */
	tbz	DST, #2, 1f
	ldr	TMP_Xw, [SRC0, #-4]!
	str	TMP_Xw, [DST, #-4]!
	sub	LEN, LEN, #4
1:
#if (STP_ALIGN > 8)
	/* if (DST & 8) { *--(uint64_t *)dst = *--(uint64_t *)src; } */
	tbz	DST, #3, 1f
	ldr	TMP_X, [SRC0, #-8]!
	str	TMP_X, [DST, #-8]!
	sub	LEN, LEN, #8
1:
#endif /* (STP_ALIGN > 8) */
9:

	cmp	LEN, #1024
	bhs	backward_copy1k
backward_less1k:
	/* copy 16*n bytes */
	and	TMP_D, LEN, #(1023-15)		/* len &= 1023; len &= ~15; */
	adr	TMP_X, 8f
	sub	LEN, LEN, TMP_D
	sub	TMP_X, TMP_X, TMP_D, lsr #1	/* jump to (8f - len/2) */
	br	TMP_X
backward_copy1k:	/* copy 16*64 bytes */
	sub	LEN, LEN, #1024
	.rept	(1024 / 16)
	ldp	DATA0, DATA1, [SRC0, #-16]!	/* *--dst = *--src; */
	stp	DATA0, DATA1, [DST, #-16]!
	.endr
8:
	cbz	LEN, done
	cmp	LEN, #1024
	bhs	backward_copy1k
	cmp	LEN, #16
	bhs	backward_less1k

	/* if (len & 16) { *--(uint128_t *)dst = *--(uint128_t *)src; } */
	tbz	LEN, #4, 1f
	ldp	DATA0, DATA1, [SRC0, #-16]!
	ldp	DATA0, DATA1, [DST, #-16]!
1:
	/* if (len & 8) { *--(uint64_t *)dst = *--(uint64_t *)src; } */
	tbz	LEN, #3, 1f
	ldr	TMP_X, [SRC0, #-8]!
	str	TMP_X, [DST, #-8]!
1:
	/* if (len & 4) { *--(uint32_t *)dst = *--(uint32_t *)src; } */
	tbz	LEN, #2, 1f
	ldr	TMP_Xw, [SRC0, #-4]!
	str	TMP_Xw, [DST, #-4]!
1:
	/* if (len & 2) { *--(uint16_t *)dst = *--(uint16_t *)src; } */
	tbz	LEN, #1, 1f
	ldrh	TMP_Xw, [SRC0, #-2]!
	strh	TMP_Xw, [DST, #-2]!
1:
	/* if (len & 1) { *--(uint8_t *)dst = *--(uint8_t *)src; } */
	tbz	LEN, #0, 1f
	ldrb	TMP_Xw, [SRC0, #-1]!
	strb	TMP_Xw, [DST, #-1]!
1:
	ret
#endif /* !NO_OVERLAP */


#if defined(STRICT_ALIGNMENT) && !defined(NO_OVERLAP)
	.align	5
backward_copy:
	prfm	PLDL1KEEP, [SRC0]
	add	DST, DST0, LEN
	add	SRC0, SRC0, LEN
	cmp	LEN, #SMALLSIZE
	bcs	strict_backward

	cmp	LEN, #10
	bcs	9f
backward_tiny:
	/* copy 1-10 bytes */
	adr	TMP_X, 8f
	sub	TMP_X, TMP_X, LEN, lsl #3	/* jump to (8f - len*2) */
	br	TMP_X
	.rept	10
	ldrb	TMP_Xw, [SRC0, #-1]!
	strb	TMP_Xw, [DST, #-1]!
	.endr
8:
	ret
9:
	/* length is small(<32), and src or dst may be unaligned */
	eor	TMP_X, SRC0, DST0
	ands	TMP_X, TMP_X, #7
	bne	notaligned_backward_small

samealign_backward_small:
	/* if (dst & 1) { *--(uint8_t *)dst = *--(uint8_t *)src; } */
	tbz	DST, #0, 1f
	ldrb	TMP_Xw, [SRC0, #-1]!
	strb	TMP_Xw, [DST, #-1]!
	sub	LEN, LEN, #1
1:
	/* if (dst & 2) { *--(uint16_t *)dst = *--(uint16_t *)src; } */
	tbz	DST, #1, 1f
	ldrh	TMP_Xw, [SRC0, #-2]!
	strh	TMP_Xw, [DST, #-2]!
	sub	LEN, LEN, #2
1:
	/* if (dst & 4) { *--(uint32_t *)dst = *--(uint32_t *)src; } */
	tbz	DST, #2, 1f
	ldr	TMP_Xw, [SRC0, #-4]!
	str	TMP_Xw, [DST, #-4]!
	sub	LEN, LEN, #4
1:
	/* if (len & 16) { *--(uint128_t *)dst = *--(uint128_t *)src; } */
	tbz	LEN, #4, 1f
	ldp	DATA0, DATA1, [SRC0, #-16]!
	stp	DATA0, DATA1, [DST, #-16]!
1:
	/* if (len & 8) { *--(uint64_t *)dst = *--(uint64_t *)src; } */
	tbz	LEN, #3, 1f
	ldr	TMP_X, [SRC0, #-8]!
	str	TMP_X, [DST, #-8]!
1:
	/* if (len & 4) { *--(uint32_t *)dst = *--(uint32_t *)src; } */
	tbz	LEN, #2, 1f
	ldr	TMP_Xw, [SRC0, #-4]!
	str	TMP_Xw, [DST, #-4]!
1:
	/* if (len & 2) { *--(uint16_t *)dst = *--(uint16_t *)src; } */
	tbz	LEN, #1, 1f
	ldrh	TMP_Xw, [SRC0, #-2]!
	strh	TMP_Xw, [DST, #-2]!
1:
	/* if (len & 1) { *--(uint8_t *)dst = *--(uint8_t *)src; } */
	tbz	LEN, #0, 1f
	ldrb	TMP_Xw, [SRC0, #-1]!
	strb	TMP_Xw, [DST, #-1]!
1:
	ret

notaligned_backward_small:
	/* length is small, and src or dst may be unaligned */
	sub	TMP_S, SRC0, LEN	/* tmp_s = src - len */
1:					/* do { */
	ldrb	TMP_Xw, [SRC0, #-1]!
	strb	TMP_Xw, [DST, #-1]!	/*  *(char *)dst++ = *(char *)src++ */
	cmp	TMP_S, SRC0		/* while (tmp_s < src) */
	blo	1b
	ret

strict_backward:
	/* src or dst may be unaligned */
	and	SRC_ALIGNBIT, SRC0, #7
	and	DST_ALIGNBIT, DST, #7
	lsl	SRC_ALIGNBIT, SRC_ALIGNBIT, #3
	lsl	DST_ALIGNBIT, DST_ALIGNBIT, #3
	sub	SRC_DST_ALIGNBIT, SRC_ALIGNBIT, DST_ALIGNBIT
	cbz	SRC_DST_ALIGNBIT, copy_backward	/* same alignment? */

	and	SRC, SRC0, #~7
	and	DST, DST, #~7
	neg	DST_SRC_ALIGNBIT, SRC_DST_ALIGNBIT

#if BYTE_ORDER == LITTLE_ENDIAN
	tbz	SRC_DST_ALIGNBIT, #63, 5f	/* if(SRC_DST_ALIGNBIT < 0) { */

	cmp	SRC, SRC0			/* don't access out of range */
	beq	1f
	ldr	DATA1, [SRC]
1:
	ldr	DATA0, [SRC, #-8]!

	lsl	DATA1, DATA1, DST_SRC_ALIGNBIT	/* data1 =                    */
	lsr	TMP_X, DATA0, SRC_DST_ALIGNBIT	/* (data1<<dst_src_alignbit)| */
	orr	DATA1, DATA1, TMP_X		/* (data0<<src_dst_alignbit); */

	b	9f				/* }                          */
5:						/* else {                     */
	ldr	DATA0, [SRC]			/*  data0 = *src;             */
	lsr	DATA1, DATA0, SRC_DST_ALIGNBIT	/*  data1=data0>>src_dst_abit;*/
9:						/* }                          */

	cbz	DST_ALIGNBIT, 9f	/* if (dst_alignbit != 0) {           */
	mov	TMP_D, DST		/*   tmp_d = dst;                     */

	tbz	DST_ALIGNBIT, #(2+3), 1f /*   if (dst_ailgnbit & (4<<3)) {    */
	str	DATA1w, [TMP_D], #4	/*      *(uint32_t *)tmp_d++ = data1; */
	lsr	DATA1, DATA1, #32	/*      data1 >>= 32;                 */
1:					/*    }                               */
	tbz	DST_ALIGNBIT, #(1+3), 1f /*   if (dst_ailgnbit & (2<<3)) {    */
	strh	DATA1w, [TMP_D], #2	/*      *(uint16_t *)tmp_d++ = data1; */
	lsr	DATA1, DATA1, #16	/*      data1 >>= 16;                 */
1:					/*    }                               */
	tbz	DST_ALIGNBIT, #(0+3), 1f /*   if (dst_alignbit & (1<<3)) {    */
	strb	DATA1w, [TMP_D]		/*      *(uint8_t *)tmp_d = data1;    */
1:					/*    }                               */

	sub	LEN, LEN, DST_ALIGNBIT, lsr #3	/* len -=(dst_alignbit>>3);   */
9:					/* }                                  */
#else /* BYTE_ORDER */
	tbz	SRC_DST_ALIGNBIT, #63, 5f	/* if(SRC_DST_ALIGNBIT < 0) { */

	cmp	SRC, SRC0			/* don't access out of range */
	beq	1f
	ldr	DATA1, [SRC]
1:
	ldr	DATA0, [SRC, #-8]!

	lsr	DATA1, DATA1, DST_SRC_ALIGNBIT	/* data1 =                    */
	lsl	TMP_X, DATA0, SRC_DST_ALIGNBIT	/* (data1>>dst_src_alignbit)| */
	orr	DATA1, DATA1, TMP_X		/* (data0<<src_dst_alignbit); */

	b	9f				/* }                          */
5:						/* else {                     */
	ldr	DATA0, [SRC]			/*  data0 = *src;             */
	lsr	DATA1, DATA0, DST_SRC_ALIGNBIT	/*  data1=data0<<dst_src_abit;*/
9:						/* }                          */

	cbz	DST_ALIGNBIT, 9f	/* if (dst_alignbit != 0) {           */
	mov	TMP_D, DST		/*   tmp_d = dst;                     */

	tbz	DST_ALIGNBIT, #(2+3), 1f /*   if (dst_ailgnbit & (4<<3)) {    */
	lsr	TMP_X, DATA1, #32	/*      x = data1 >> 32;              */
	str	TMP_Xw, [TMP_D], #4	/*      *(uint32_t *)tmp_d++ = x;     */
1:					/*    }                               */
	tbz	DST_ALIGNBIT, #(1+3), 1f /*   if (dst_ailgnbit & (2<<3)) {    */
	lsr	TMP_X, DATA1, #16	/*      x = data1 >> 16;              */
	strh	TMP_Xw, [TMP_D], #2	/*      *(uint16_t *)tmp_d++ = x;     */
1:					/*    }                               */
	tbz	DST_ALIGNBIT, #(0+3), 1f /*   if (dst_alignbit & (1<<3)) {    */
	lsr	TMP_X, DATA1, #8	/*      x = data1 >> 8;               */
	strb	TMP_Xw, [TMP_D], #1	/*      *(uint8_t *)tmp_d++ = x;      */
1:					/*    }                               */

	sub	LEN, LEN, DST_ALIGNBIT, lsr #3	/* len -=(dst_alignbit>>3);   */
9:					/* }                                  */
#endif /* BYTE_ORDER */


backward_shifting_copy_loop:
	ldp	DATA2, DATA1, [SRC, #-16]!
#if BYTE_ORDER == LITTLE_ENDIAN
	/* data0 = (data1 >> src_dst_alignbit) | (data0 << dst_src_alignbit); */
	lsl	DATA0, DATA0, DST_SRC_ALIGNBIT
	lsr	TMP_X, DATA1, SRC_DST_ALIGNBIT
	orr	DATA0, DATA0, TMP_X
	/* data1 = (data2 >> src_dst_alignbit) | (data1 << dst_src_alignbit); */
	lsl	DATA1, DATA1, DST_SRC_ALIGNBIT
	lsr	TMP_X, DATA2, SRC_DST_ALIGNBIT
	orr	DATA1, DATA1, TMP_X
#else /* BYTE_ORDER */
	/* data0 = (data1 << src_dst_alignbit) | (data0 >> dst_src_alignbit); */
	lsr	DATA0, DATA0, DST_SRC_ALIGNBIT
	lsl	TMP_X, DATA1, SRC_DST_ALIGNBIT
	orr	DATA0, DATA0, TMP_X
	/* data1 = (data2 << src_dst_alignbit) | (data1 >> dst_src_alignbit); */
	lsr	DATA1, DATA1, DST_SRC_ALIGNBIT
	lsl	TMP_X, DATA2, SRC_DST_ALIGNBIT
	orr	DATA1, DATA1, TMP_X
#endif /* BYTE_ORDER */
	stp	DATA1, DATA0, [DST, #-16]!
	mov	DATA0, DATA2
	sub	LEN, LEN, #16
	cmp	LEN, #16
	bhs	backward_shifting_copy_loop


	/* write 8 bytes */
	tbz	LEN, #3, 9f

	ldr	DATA1, [SRC, #-8]!
#if BYTE_ORDER == LITTLE_ENDIAN
	/* data0 = (data1 >> src_dst_alignbit) | (data0 << dst_src_alignbit); */
	lsl	DATA0, DATA0, DST_SRC_ALIGNBIT
	lsr	TMP_X, DATA1, SRC_DST_ALIGNBIT
	orr	DATA0, DATA0, TMP_X
#else /* BYTE_ORDER */
	/* data0 = (data1 << src_dst_alignbit) | (data0 >> dst_src_alignbit); */
	lsr	DATA0, DATA0, DST_SRC_ALIGNBIT
	lsl	TMP_X, DATA1, SRC_DST_ALIGNBIT
	orr	DATA0, DATA0, TMP_X
#endif /* BYTE_ORDER */
	str	DATA0, [DST, #-8]!
	mov	DATA0, DATA1
	sub	LEN, LEN, #8
9:

	cbz	LEN, backward_shifting_copy_done

	/* copy last 1-7 bytes */
	and	TMP_X, SRC_DST_ALIGNBIT, #63
	cmp	LEN, TMP_X, lsr #3
	bls	1f
	ldr	DATA1, [SRC, #-8]!	/* don't access out of range */
1:

#if BYTE_ORDER == LITTLE_ENDIAN
	/* data0 = (data1 >> src_dst_alignbit) | (data0 << dst_src_alignbit); */
	lsl	DATA0, DATA0, DST_SRC_ALIGNBIT
	lsr	TMP_X, DATA1, SRC_DST_ALIGNBIT
	orr	DATA0, DATA0, TMP_X
#else /* BYTE_ORDER */
	/* data0 = (data1 << src_dst_alignbit) | (data0 >> dst_src_alignbit); */
	lsr	DATA0, DATA0, DST_SRC_ALIGNBIT
	lsl	TMP_X, DATA1, SRC_DST_ALIGNBIT
	orr	DATA0, DATA0, TMP_X
#endif /* BYTE_ORDER */

#if BYTE_ORDER == LITTLE_ENDIAN
	tbz	LEN, #2, 1f
	ror	DATA0, DATA0, #32
	str	DATA0w, [DST, #-4]!
1:
	tbz	LEN, #1, 1f
	ror	DATA0, DATA0, #48
	strh	DATA0w, [DST, #-2]!
1:
	tbz	LEN, #0, 1f
	ror	DATA0, DATA0, #56
	strb	DATA0w, [DST, #-1]!
1:
#else /* BYTE_ORDER */
	tbz	LEN, #2, 1f
	str	DATA0w, [DST, #-4]!
	lsr	DATA0, DATA0, #32
1:
	tbz	LEN, #1, 1f
	strh	DATA0w, [DST, #-2]!
	lsr	DATA0, DATA0, #16
1:
	tbz	LEN, #0, 1f
	strb	DATA0w, [DST, #-1]!
1:
#endif /* BYTE_ORDER */
backward_shifting_copy_done:
	ret
#endif /* defined(STRICT_ALIGNMENT) && !defined(NO_OVERLAP) */


	.align	5
ENTRY(FUNCTION)
#ifdef STRICT_ALIGNMENT
	cbz	LEN, done
#ifndef NO_OVERLAP
	cmp	SRC0, DST0
	beq	done
	bcc	backward_copy
#endif /* NO_OVERLAP */
	mov	DST, DST0
	cmp	LEN, #SMALLSIZE
	bcs	strict_forward

	cmp	LEN, #10
	bcs	9f
forward_tiny:
	/* copy 1-10 bytes */
	adr	TMP_X, 8f
	sub	TMP_X, TMP_X, LEN, lsl #3	/* jump to (8f - len*2) */
	br	TMP_X
	.rept	10
	ldrb	TMP_Xw, [SRC0], #1
	strb	TMP_Xw, [DST], #1
	.endr
8:
	ret
9:
	/* length is small(<32), and src or dst may be unaligned */
	eor	TMP_X, SRC0, DST0
	ands	TMP_X, TMP_X, #7
	bne	notaligned_forward_small
samealign_forward_small:
	/* if (dst & 1) { *(uint8_t *)dst++ = *(uint8_t *)src++; } */
	tbz	DST, #0, 1f
	ldrb	TMP_Xw, [SRC0], #1
	strb	TMP_Xw, [DST], #1
	sub	LEN, LEN, #1
1:
	/* if (dst & 2) { *(uint16_t *)dst++ = *(uint16_t *)src++; } */
	tbz	DST, #1, 1f
	ldrh	TMP_Xw, [SRC0], #2
	strh	TMP_Xw, [DST], #2
	sub	LEN, LEN, #2
1:
	/* if (dst & 4) { *(uint32_t *)dst++ = *(uint32_t *)src++; } */
	tbz	DST, #2, 1f
	ldr	TMP_Xw, [SRC0], #4
	str	TMP_Xw, [DST], #4
	sub	LEN, LEN, #4
1:
	/* if (len & 16) { *(uint128_t *)dst++ = *(uint128_t *)src++; } */
	tbz	LEN, #4, 1f
	ldp	DATA0, DATA1, [SRC0], #16
	stp	DATA0, DATA1, [DST], #16
1:
	/* if (len & 8) { *(uint64_t *)dst++ = *(uint64_t *)src++; } */
	tbz	LEN, #3, 1f
	ldr	TMP_X, [SRC0], #8
	str	TMP_X, [DST], #8
1:
	/* if (len & 4) { *(uint32_t *)dst++ = *(uint32_t *)src++; } */
	tbz	LEN, #2, 1f
	ldr	TMP_Xw, [SRC0], #4
	str	TMP_Xw, [DST], #4
1:
	/* if (len & 2) { *(uint16_t *)dst++ = *(uint16_t *)src++; } */
	tbz	LEN, #1, 1f
	ldrh	TMP_Xw, [SRC0], #2
	strh	TMP_Xw, [DST], #2
1:
	/* if (len & 1) { *(uint8_t *)dst++ = *(uint8_t *)src++; } */
	tbz	LEN, #0, 1f
	ldrb	TMP_Xw, [SRC0], #1
	strb	TMP_Xw, [DST], #1
1:
	ret

notaligned_forward_small:
	/* src and dst are not aligned... */
	prfm	PLDL1KEEP, [SRC0]
	prfm	PLDL1KEEP, [SRC0, #8]
	prfm	PLDL1KEEP, [SRC0, #16]
	add	TMP_S, SRC0, LEN	/* tmp_s = src + len */
1:					/* do { */
	ldrb	TMP_Xw, [SRC0], #1
	strb	TMP_Xw, [DST], #1	/*  *(char *)dst++ = *(char *)src++ */
	cmp	SRC0, TMP_S		/* while (src < tmp_s); */
	blo	1b
	ret

strict_forward:
	/* src or dst may be unaligned */
	and	SRC_ALIGNBIT, SRC0, #7
	and	DST_ALIGNBIT, DST0, #7
	lsl	SRC_ALIGNBIT, SRC_ALIGNBIT, #3
	lsl	DST_ALIGNBIT, DST_ALIGNBIT, #3
	sub	SRC_DST_ALIGNBIT, SRC_ALIGNBIT, DST_ALIGNBIT
	cbz	SRC_DST_ALIGNBIT, copy_forward	/* same alignment? */

	and	SRC, SRC0, #~7
	and	DST, DST0, #~7
	neg	DST_SRC_ALIGNBIT, SRC_DST_ALIGNBIT

#if BYTE_ORDER == LITTLE_ENDIAN
	tbz	DST_SRC_ALIGNBIT, #63, 5f	/* if(DST_SRC_ALIGNBIT < 0) { */
	ldp	DATA1, DATA0, [SRC], #16
	neg	TMP_X, SRC_ALIGNBIT
	lsr	DATA1, DATA1, SRC_ALIGNBIT	/* data1 =                    */
	lsl	TMP_X, DATA0, TMP_X		/*  (data1 >> src_alignbit) | */
	orr	DATA1, DATA1, TMP_X		/*  (data0 << -src_alignbit); */
	b	9f
5:
	ldr	DATA0, [SRC], #8
	lsr	DATA1, DATA0, SRC_ALIGNBIT
9:

	cbz	DST_ALIGNBIT, 5f
	mov	TMP_D, DST0
	/* if (tmp_d & 1) { *(uint8_t *)tmp_d++ = data1; } */
	tbz	TMP_D, #0, 1f
	strb	DATA1w, [TMP_D], #1
	lsr	DATA1, DATA1, #8
1:
	/* if (tmp_d & 2) { *(uint16_t *)tmp_d++ = data1; } */
	tbz	TMP_D, #1, 1f
	strh	DATA1w, [TMP_D], #2
	lsr	DATA1, DATA1, #16
1:
	/* if (tmp-d & 4) { *(uint32_t *)tmp_d++ = data1; } */
	tbz	TMP_D, #2, 1f
	str	DATA1w, [TMP_D], #4
1:
	add	DST, DST, #8
	b	9f
5:
	str	DATA1, [DST], #8
9:
	sub	LEN, LEN, #8
	add	LEN, LEN, DST_ALIGNBIT, lsr #3
#else /* BYTE_ORDER */
	tbz	DST_SRC_ALIGNBIT, #63, 5f	/* if(DST_SRC_ALIGNBIT < 0) { */
	ldp	DATA1, DATA0, [SRC], #16
	neg	TMP_X, SRC_ALIGNBIT
	lsl	DATA1, DATA1, SRC_ALIGNBIT	/* data1 =                    */
	lsr	TMP_X, DATA0, TMP_X		/*  (data1 << src_alignbit) | */
	orr	DATA1, DATA1, TMP_X		/*  (data0 >> -src_alignbit); */
	b	9f
5:
	ldr	DATA0, [SRC], #8
	lsl	DATA1, DATA0, SRC_ALIGNBIT
9:

	cbz	DST_ALIGNBIT, 5f
	mov	TMP_D, DST0
	/* if (tmp_d & 1) { *(uint8_t *)tmp_d++ = data1 >> 56; } */
	tbz	TMP_D, #0, 1f
	lsr	TMP_X, DATA1, #56
	strb	TMP_Xw, [TMP_D], #1
1:
	/* if (tmp_d & 2) { *(uint16_t *)tmp_d++ = data1 >> 48; } */
	tbz	TMP_D, #1, 1f
	lsr	TMP_X, DATA1, #48
	strh	TMP_Xw, [TMP_D], #2
1:
	/* if (tmp-d & 4) { *(uint32_t *)tmp_d++ = data1 >> 32; } */
	tbz	TMP_D, #2, 1f
	lsr	TMP_X, DATA1, #32
	str	TMP_Xw, [TMP_D], #4
1:
	add	DST, DST, #8
	b	9f
5:
	str	DATA1, [DST], #8
9:
	sub	LEN, LEN, #8
	add	LEN, LEN, DST_ALIGNBIT, lsr #3
#endif /* BYTE_ORDER */

shifting_copy_loop:
	ldp	DATA1, DATA2, [SRC], #16
#if BYTE_ORDER == LITTLE_ENDIAN
	/* data0 = (data0 >> src_dst_alignbit) | (data1 << dst_src_alignbit) */
	lsr	DATA0, DATA0, SRC_DST_ALIGNBIT
	lsl	TMP_X, DATA1, DST_SRC_ALIGNBIT
	orr	DATA0, DATA0, TMP_X
	/* data1 = (data1 >> src_dst_alignbit) | (data2 << dst_src_alignbit) */
	lsr	DATA1, DATA1, SRC_DST_ALIGNBIT
	lsl	TMP_X, DATA2, DST_SRC_ALIGNBIT
	orr	DATA1, DATA1, TMP_X
#else /* BYTE_ORDER */
	/* data0 = (data0 << src_dst_alignbit) | (data1 >> dst_src_alignbit) */
	lsl	DATA0, DATA0, SRC_DST_ALIGNBIT
	lsr	TMP_X, DATA1, DST_SRC_ALIGNBIT
	orr	DATA0, DATA0, TMP_X
	/* data1 = (data1 << src_dst_alignbit) | (data2 >> dst_src_alignbit) */
	lsl	DATA1, DATA1, SRC_DST_ALIGNBIT
	lsr	TMP_X, DATA2, DST_SRC_ALIGNBIT
	orr	DATA1, DATA1, TMP_X
#endif /* BYTE_ORDER */
	stp	DATA0, DATA1, [DST], #16
	mov	DATA0, DATA2
	sub	LEN, LEN, #16
	cmp	LEN, #16
	bhs	shifting_copy_loop


	/* write 8 bytes */
	tbz	LEN, #3, 9f
	ldr	DATA1, [SRC], #8
#if BYTE_ORDER == LITTLE_ENDIAN
	/* data0 = (data0 >> src_dst_alignbit) | (data1 << dst_src_alignbit) */
	lsr	DATA0, DATA0, SRC_DST_ALIGNBIT
	lsl	TMP_X, DATA1, DST_SRC_ALIGNBIT
	orr	DATA0, DATA0, TMP_X
#else /* BYTE_ORDER */
	/* data0 = (data0 << src_dst_alignbit) | (data1 >> dst_src_alignbit) */
	lsl	DATA0, DATA0, SRC_DST_ALIGNBIT
	lsr	TMP_X, DATA1, DST_SRC_ALIGNBIT
	orr	DATA0, DATA0, TMP_X
#endif /* BYTE_ORDER */
	str	DATA0, [DST], #8
	mov	DATA0, DATA1
	sub	LEN, LEN, #8
9:

	cbz	LEN, shifting_copy_done

	/* copy last 1-7 bytes */
	and	TMP_X, DST_SRC_ALIGNBIT, #63
	cmp	LEN, TMP_X, lsr #3
	bls	1f
	ldr	DATA1, [SRC], #8	/* don't access out of range */
1:

#if BYTE_ORDER == LITTLE_ENDIAN
	/* data0 = (data0 >> src_dst_alignbit) | (data1 << dst_src_alignbit) */
	lsr	DATA0, DATA0, SRC_DST_ALIGNBIT
	lsl	TMP_X, DATA1, DST_SRC_ALIGNBIT
	orr	DATA0, DATA0, TMP_X
#else /* BYTE_ORDER */
	/* data0 = (data0 << src_dst_alignbit) | (data1 >> dst_src_alignbit) */
	lsl	DATA0, DATA0, SRC_DST_ALIGNBIT
	lsr	TMP_X, DATA1, DST_SRC_ALIGNBIT
	orr	DATA0, DATA0, TMP_X
#endif /* BYTE_ORDER */

#if BYTE_ORDER == LITTLE_ENDIAN
	/* if (len & 4) { *(uint32_t *)dst++ = data0; } */
	tbz	LEN, #2, 1f
	str	DATA0w, [DST], #4
	lsr	DATA0, DATA0, #32
1:
	/* if (len & 2) { *(uint16_t *)dst++ = data0; } */
	tbz	LEN, #1, 1f
	strh	DATA0w, [DST], #2
	lsr	DATA0, DATA0, #16
1:
	/* if (len & 1) { *(uint8_t *)dst++ = data0; } */
	tbz	LEN, #0, 1f
	strb	DATA0w, [DST], #1
1:
#else /* BYTE_ORDER */
	/* if (len & 4) { *(uint32_t *)dst++ = data0 >> 32; } */
	tbz	LEN, #2, 1f
	lsr	TMP_X, DATA0, #32
	str	TMP_Xw, [DST], #4
1:
	/* if (len & 2) { *(uint16_t *)dst++ = data0 >> 16; } */
	tbz	LEN, #1, 1f
	lsr	TMP_X, DATA0, #16
	strh	TMP_Xw, [DST], #2
1:
	/* if (len & 1) { *(uint8_t *)dst++ = data0 >> 8; } */
	tbz	LEN, #0, 1f
	lsr	TMP_X, DATA0, #8
	strb	TMP_Xw, [DST], #1
1:
#endif /* BYTE_ORDER */
shifting_copy_done:
	ret

#else /* STRICT_ALIGNMENT */
#ifndef NO_OVERLAP
	cbz	LEN, done
	cmp	SRC0, DST0
	beq	done
	bcc	backward_ignore_align
#endif /* NO_OVERLAP */

	prfm	PLDL1KEEP, [SRC0]
	cmp	LEN, #SMALLSIZE
	bcs	copy_forward
	mov	DST, DST0

copy_forward_small:
	cmp	LEN, #8
	bcs	9f

	/* 0 <= len < 8 */
	/* if (len & 4) { *(uint32_t *)dst++ = *(uint32_t *)src++; } */
	tbz	LEN, #2, 1f
	ldr	TMP_Xw, [SRC0], #4
	str	TMP_Xw, [DST], #4
1:
	/* if (len & 2) { *(uint16_t *)dst++ = *(uint16_t *)src++; } */
	tbz	LEN, #1, 1f
	ldrh	TMP_Xw, [SRC0], #2
	strh	TMP_Xw, [DST], #2
1:
	/* if (len & 1) { *(uint8_t *)dst++ = *(uint8_t *)src++; } */
	tbz	LEN, #0, 1f
	ldrb	TMP_Xw, [SRC0], #1
	strb	TMP_Xw, [DST], #1
1:
	ret
9:

	prfm	PLDL1KEEP, [SRC0, #8]
	cmp	LEN, #16
	bcs	9f

	/* 8 <= len < 16 */
	/* *(uint64_t *)dst++ = *(uint64_t *)src++; */
	ldr	TMP_X, [SRC0], #8
	str	TMP_X, [DST], #8
	/* if (len & 4) { *(uint32_t *)dst++ = *(uint32_t *)src++; } */
	tbz	LEN, #2, 1f
	ldr	TMP_Xw, [SRC0], #4
	str	TMP_Xw, [DST], #4
1:
	/* if (len & 2) { *(uint16_t *)dst++ = *(uint16_t *)src++; } */
	tbz	LEN, #1, 1f
	ldrh	TMP_Xw, [SRC0], #2
	strh	TMP_Xw, [DST], #2
1:
	/* if (len & 1) { *(uint8_t *)dst++ = *(uint8_t *)src++; } */
	tbz	LEN, #0, 1f
	ldrb	TMP_Xw, [SRC0], #1
	strb	TMP_Xw, [DST], #1
1:
	ret
9:

	/* 16 <= len < 32 */
	prfm	PLDL1KEEP, [SRC0, 16]
	prfm	PLDL1KEEP, [SRC0, 24]
	ldp	DATA0, DATA1, [SRC0], #16
	stp	DATA0, DATA1, [DST], #16
	/* if (len & 8) { *(uint64_t *)dst++ = *(uint64_t *)src++; } */
	tbz	LEN, #3, 1f
	ldr	TMP_X, [SRC0], #8
	str	TMP_X, [DST], #8
1:
	/* if (len & 4) { *(uint32_t *)dst++ = *(uint32_t *)src++; } */
	tbz	LEN, #2, 1f
	ldr	TMP_Xw, [SRC0], #4
	str	TMP_Xw, [DST], #4
1:
	/* if (len & 2) { *(uint16_t *)dst++ = *(uint16_t *)src++; } */
	tbz	LEN, #1, 1f
	ldrh	TMP_Xw, [SRC0], #2
	strh	TMP_Xw, [DST], #2
1:
	/* if (len & 1) { *(uint8_t *)dst++ = *(uint8_t *)src++; } */
	tbz	LEN, #0, 1f
	ldrb	TMP_Xw, [SRC0], #1
	strb	TMP_Xw, [DST], #1
1:
	ret
#endif /* !STRICT_ALIGNMENT */

	.align	4
copy_forward:
	/* DST is not aligned at this point */
	mov	DST, DST0
#ifndef STRICT_ALIGNMENT
	cmp	LEN, #512	/* pre-alignment can be overhead when small */
	bcc	9f
#endif /* STRICT_ALIGNMENT */
	/* if (DST & 1) { *(uint8_t *)dst++ = *(uint8_t *)src++; } */
	tbz	DST, #0, 1f
	ldrb	TMP_Xw, [SRC0], #1
	strb	TMP_Xw, [DST], #1
	sub	LEN, LEN, #1
1:
	/* if (DST & 2) { *(uint16_t *)dst++ = *(uint16_t *)src++; } */
	tbz	DST, #1, 1f
	ldrh	TMP_Xw, [SRC0], #2
	strh	TMP_Xw, [DST], #2
	sub	LEN, LEN, #2
1:
	/* if (DST & 4) { *(uint32_t *)dst++ = *(uint32_t *)src++; } */
	tbz	DST, #2, 1f
	ldr	TMP_Xw, [SRC0], #4
	str	TMP_Xw, [DST], #4
	sub	LEN, LEN, #4
1:
#if (STP_ALIGN > 8)
	/* if (DST & 8) { *(uint64_t *)dst++ = *(uint64_t *)src++; } */
	tbz	DST, #3, 1f
	ldr	TMP_X, [SRC0], #8
	str	TMP_X, [DST], #8
	sub	LEN, LEN, #8
1:
#endif /* (STP_ALIGN > 8) */
9:

	cmp	LEN, #1024
	bhs	forward_copy1k
forward_less1k:
	/* copy 16*n bytes */
	and	TMP_D, LEN, #(1023-15)		/* len &= 1023; len &= ~15; */
	adr	TMP_X, 8f
	sub	LEN, LEN, TMP_D
	sub	TMP_X, TMP_X, TMP_D, lsr #1	/* jump to (8f - len/2) */
	br	TMP_X
forward_copy1k:	/* copy 16*64 bytes */
	sub	LEN, LEN, #1024
	.rept	(1024 / 16)
	ldp	DATA0, DATA1, [SRC0], #16	/* *dst++ = *src++; */
	stp	DATA0, DATA1, [DST], #16
	.endr
8:
	cbz	LEN, done
	cmp	LEN, #1024
	bhs	forward_copy1k
	cmp	LEN, #16
	bhs	forward_less1k

	/* if (len & 16) { *(uint128_t *)dst++ = *(uint128_t *)src++; } */
	tbz	LEN, #4, 1f
	ldp	DATA0, DATA1, [SRC0], #16
	stp	DATA0, DATA1, [DST], #16
1:
	/* if (len & 8) { *(uint64_t *)dst++ = *(uint64_t *)src++; } */
	tbz	LEN, #3, 1f
	ldr	TMP_X, [SRC0], #8
	str	TMP_X, [DST], #8
1:
	/* if (len & 4) { *(uint32_t *)dst++ = *(uint32_t *)src++; } */
	tbz	LEN, #2, 1f
	ldr	TMP_Xw, [SRC0], #4
	str	TMP_Xw, [DST], #4
1:
	/* if (len & 2) { *(uint16_t *)dst++ = *(uint16_t *)src++; } */
	tbz	LEN, #1, 1f
	ldrh	TMP_Xw, [SRC0], #2
	strh	TMP_Xw, [DST], #2
1:
	/* if (len & 1) { *(uint8_t *)dst++ = *(uint8_t *)src++; } */
	tbz	LEN, #0, 1f
	ldrb	TMP_Xw, [SRC0], #1
	strb	TMP_Xw, [DST], #1
1:
done:
	ret
END(FUNCTION)
