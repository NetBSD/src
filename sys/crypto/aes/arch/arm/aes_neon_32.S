/*	$NetBSD: aes_neon_32.S,v 1.6 2020/08/16 18:02:03 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#include <arm/asm.h>

RCSID("$NetBSD: aes_neon_32.S,v 1.6 2020/08/16 18:02:03 riastradh Exp $")

	.fpu	neon

	.text
	.p2align 2
.Lconstants_addr:
	.long	.Lconstants - .

	.section .rodata
	.p2align 4
.Lconstants:

	.type	inv,_ASM_TYPE_OBJECT
inv:
	.byte	0x80,0x01,0x08,0x0D,0x0F,0x06,0x05,0x0E
	.byte	0x02,0x0C,0x0B,0x0A,0x09,0x03,0x07,0x04
END(inv)

	.type	inva,_ASM_TYPE_OBJECT
inva:
	.byte	0x80,0x07,0x0B,0x0F,0x06,0x0A,0x04,0x01
	.byte	0x09,0x08,0x05,0x02,0x0C,0x0E,0x0D,0x03
END(inva)

	.type	mc_forward,_ASM_TYPE_OBJECT
mc_forward:
	.byte	0x01,0x02,0x03,0x00,0x05,0x06,0x07,0x04	/* 0 */
	.byte	0x09,0x0A,0x0B,0x08,0x0D,0x0E,0x0F,0x0C

	.byte	0x05,0x06,0x07,0x04,0x09,0x0A,0x0B,0x08	/* 1 */
	.byte	0x0D,0x0E,0x0F,0x0C,0x01,0x02,0x03,0x00

	.byte	0x09,0x0A,0x0B,0x08,0x0D,0x0E,0x0F,0x0C	/* 2 */
	.byte	0x01,0x02,0x03,0x00,0x05,0x06,0x07,0x04

.Lmc_forward_3:
	.byte	0x0D,0x0E,0x0F,0x0C,0x01,0x02,0x03,0x00	/* 3 */
	.byte	0x05,0x06,0x07,0x04,0x09,0x0A,0x0B,0x08
END(mc_forward)

	.type	mc_backward,_ASM_TYPE_OBJECT
mc_backward:
	.byte	0x03,0x00,0x01,0x02,0x07,0x04,0x05,0x06	/* 0 */
	.byte	0x0B,0x08,0x09,0x0A,0x0F,0x0C,0x0D,0x0E

	.byte	0x0F,0x0C,0x0D,0x0E,0x03,0x00,0x01,0x02	/* 1 */
	.byte	0x07,0x04,0x05,0x06,0x0B,0x08,0x09,0x0A

	.byte	0x0B,0x08,0x09,0x0A,0x0F,0x0C,0x0D,0x0E	/* 2 */
	.byte	0x03,0x00,0x01,0x02,0x07,0x04,0x05,0x06

	.byte	0x07,0x04,0x05,0x06,0x0B,0x08,0x09,0x0A	/* 3 */
	.byte	0x0F,0x0C,0x0D,0x0E,0x03,0x00,0x01,0x02
END(mc_backward)

	.type	sr,_ASM_TYPE_OBJECT
sr:
	.byte	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07	/* 0 */
	.byte	0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F

	.byte	0x00,0x05,0x0A,0x0F,0x04,0x09,0x0E,0x03	/* 1 */
	.byte	0x08,0x0D,0x02,0x07,0x0C,0x01,0x06,0x0B

	.byte	0x00,0x09,0x02,0x0B,0x04,0x0D,0x06,0x0F	/* 2 */
	.byte	0x08,0x01,0x0A,0x03,0x0C,0x05,0x0E,0x07

	.byte	0x00,0x0D,0x0A,0x07,0x04,0x01,0x0E,0x0B	/* 3 */
	.byte	0x08,0x05,0x02,0x0F,0x0C,0x09,0x06,0x03
END(sr)

	.type	iptlo,_ASM_TYPE_OBJECT
iptlo:
	.byte	0x00,0x70,0x2A,0x5A,0x98,0xE8,0xB2,0xC2
	.byte	0x08,0x78,0x22,0x52,0x90,0xE0,0xBA,0xCA
END(iptlo)

	.type	ipthi,_ASM_TYPE_OBJECT
ipthi:
	.byte	0x00,0x4D,0x7C,0x31,0x7D,0x30,0x01,0x4C
	.byte	0x81,0xCC,0xFD,0xB0,0xFC,0xB1,0x80,0xCD
END(ipthi)

	.type	sb1_0,_ASM_TYPE_OBJECT
sb1_0:
	.byte	0x00,0x3E,0x50,0xCB,0x8F,0xE1,0x9B,0xB1
	.byte	0x44,0xF5,0x2A,0x14,0x6E,0x7A,0xDF,0xA5
END(sb1_0)

	.type	sb1_1,_ASM_TYPE_OBJECT
sb1_1:
	.byte	0x00,0x23,0xE2,0xFA,0x15,0xD4,0x18,0x36
	.byte	0xEF,0xD9,0x2E,0x0D,0xC1,0xCC,0xF7,0x3B
END(sb1_1)

	.type	sb2_0,_ASM_TYPE_OBJECT
sb2_0:
	.byte	0x00,0x24,0x71,0x0B,0xC6,0x93,0x7A,0xE2
	.byte	0xCD,0x2F,0x98,0xBC,0x55,0xE9,0xB7,0x5E
END(sb2_0)

	.type	sb2_1,_ASM_TYPE_OBJECT
sb2_1:
	.byte	0x00,0x29,0xE1,0x0A,0x40,0x88,0xEB,0x69
	.byte	0x4A,0x23,0x82,0xAB,0xC8,0x63,0xA1,0xC2
END(sb2_1)

	.type	sbo_0,_ASM_TYPE_OBJECT
sbo_0:
	.byte	0x00,0xC7,0xBD,0x6F,0x17,0x6D,0xD2,0xD0
	.byte	0x78,0xA8,0x02,0xC5,0x7A,0xBF,0xAA,0x15
END(sbo_0)

	.type	sbo_1,_ASM_TYPE_OBJECT
sbo_1:
	.byte	0x00,0x6A,0xBB,0x5F,0xA5,0x74,0xE4,0xCF
	.byte	0xFA,0x35,0x2B,0x41,0xD1,0x90,0x1E,0x8E
END(sbo_1)

	.type	diptlo,_ASM_TYPE_OBJECT
diptlo:
	.byte	0x00,0x5F,0x54,0x0B,0x04,0x5B,0x50,0x0F
	.byte	0x1A,0x45,0x4E,0x11,0x1E,0x41,0x4A,0x15
END(diptlo)

	.type	dipthi,_ASM_TYPE_OBJECT
dipthi:
	.byte	0x00,0x65,0x05,0x60,0xE6,0x83,0xE3,0x86
	.byte	0x94,0xF1,0x91,0xF4,0x72,0x17,0x77,0x12
END(dipthi)

	.type	dsb9_0,_ASM_TYPE_OBJECT
dsb9_0:
	.byte	0x00,0xD6,0x86,0x9A,0x53,0x03,0x1C,0x85
	.byte	0xC9,0x4C,0x99,0x4F,0x50,0x1F,0xD5,0xCA
END(dsb9_0)

	.type	dsb9_1,_ASM_TYPE_OBJECT
dsb9_1:
	.byte	0x00,0x49,0xD7,0xEC,0x89,0x17,0x3B,0xC0
	.byte	0x65,0xA5,0xFB,0xB2,0x9E,0x2C,0x5E,0x72
END(dsb9_1)

	.type	dsbd_0,_ASM_TYPE_OBJECT
dsbd_0:
	.byte	0x00,0xA2,0xB1,0xE6,0xDF,0xCC,0x57,0x7D
	.byte	0x39,0x44,0x2A,0x88,0x13,0x9B,0x6E,0xF5
END(dsbd_0)

	.type	dsbd_1,_ASM_TYPE_OBJECT
dsbd_1:
	.byte	0x00,0xCB,0xC6,0x24,0xF7,0xFA,0xE2,0x3C
	.byte	0xD3,0xEF,0xDE,0x15,0x0D,0x18,0x31,0x29
END(dsbd_1)

	.type	dsbb_0,_ASM_TYPE_OBJECT
dsbb_0:
	.byte	0x00,0x42,0xB4,0x96,0x92,0x64,0x22,0xD0
	.byte	0x04,0xD4,0xF2,0xB0,0xF6,0x46,0x26,0x60
END(dsbb_0)

	.type	dsbb_1,_ASM_TYPE_OBJECT
dsbb_1:
	.byte	0x00,0x67,0x59,0xCD,0xA6,0x98,0x94,0xC1
	.byte	0x6B,0xAA,0x55,0x32,0x3E,0x0C,0xFF,0xF3
END(dsbb_1)

	.type	dsbe_0,_ASM_TYPE_OBJECT
dsbe_0:
	.byte	0x00,0xD0,0xD4,0x26,0x96,0x92,0xF2,0x46
	.byte	0xB0,0xF6,0xB4,0x64,0x04,0x60,0x42,0x22
END(dsbe_0)

	.type	dsbe_1,_ASM_TYPE_OBJECT
dsbe_1:
	.byte	0x00,0xC1,0xAA,0xFF,0xCD,0xA6,0x55,0x0C
	.byte	0x32,0x3E,0x59,0x98,0x6B,0xF3,0x67,0x94
END(dsbe_1)

	.type	dsbo_0,_ASM_TYPE_OBJECT
dsbo_0:
	.byte	0x00,0x40,0xF9,0x7E,0x53,0xEA,0x87,0x13
	.byte	0x2D,0x3E,0x94,0xD4,0xB9,0x6D,0xAA,0xC7
END(dsbo_0)

	.type	dsbo_1,_ASM_TYPE_OBJECT
dsbo_1:
	.byte	0x00,0x1D,0x44,0x93,0x0F,0x56,0xD7,0x12
	.byte	0x9C,0x8E,0xC5,0xD8,0x59,0x81,0x4B,0xCA
END(dsbo_1)

/*
 * aes_neon_enc1(enc, x, nrounds)
 *
 *	With -mfloat-abi=hard:
 *
 * uint8x16_t@q0
 * aes_neon_enc1(const struct aesenc *enc@r0, uint8x16_t x@q0,
 *     unsigned nrounds@r1)
 *
 *	With -mfloat-abi=soft(fp) (i.e., __SOFTFP__):
 *
 * uint8x16_t@(r0,r1,r2,r3)
 * aes_neon_enc1(const struct aesenc *enc@r0,
 *     uint8x16_t x@(r2,r3,sp[0],sp[4]), nrounds@sp[8])
 */
ENTRY(aes_neon_enc1)
#ifdef __SOFTFP__
#ifdef __ARM_BIG_ENDIAN
	vmov	d0, r3, r2		/* d0 := x lo */
#else
	vmov	d0, r2, r3		/* d0 := x lo */
#endif
	vldr	d1, [sp]		/* d1 := x hi */
	ldr	r1, [sp, #8]		/* r1 := nrounds */
#endif
	push	{r4, r5, r6, r7, r8, r10, r11, lr}
	vpush	{d8-d15}

	/*
	 * r3: rmod4
	 * r4: mc_forward
	 * r5: mc_backward
	 * r6,r7,r8,r10,r11,r12: temporaries
	 * q0={d0-d1}: x/ak/A
	 * q1={d2-d3}: 0x0f0f...
	 * q2={d4-d5}: lo/k/j/io
	 * q3={d6-d7}: hi/i/jo
	 * q4={d8-d9}: iptlo
	 * q5={d10-d11}: ipthi
	 * q6={d12-d13}: sb1[0]/sbo[0]
	 * q7={d14-d15}: sb1[1]/sbo[1]
	 * q8={d16-d17}: sb2[0]
	 * q9={d18-d19}: sb2[1]
	 * q10={d20-d21}: inv
	 * q11={d22-d23}: inva
	 * q12={d24-d25}: ir/iak/iakr/sb1_0(io)/mc_backward[rmod4]
	 * q13={d26-d27}: jr/jak/jakr/sb1_1(jo)/mc_forward[rmod4]
	 * q14={d28-d29}: rk/A2/A2_B_D
	 * q15={d30-d31}: A2_B/sr[rmod4]
	 */

	/* r12 := .Lconstants - .Lconstants_addr, r11 := .Lconstants_addr */
	ldr	r12, .Lconstants_addr
	adr	r11, .Lconstants_addr

	vld1.8	{d28-d29}, [r0 :128]!	/* q14 = *rk++ */
	movw	r3, #0
	vmov.i8	q1, #0x0f

	/* r12 := .Lconstants */
	add	r12, r12, r11

	/* (q4, q5) := (iptlo, ipthi) */
	add	r6, r12, #(iptlo - .Lconstants)
	add	r7, r12, #(ipthi - .Lconstants)
	vld1.8	{d8-d9}, [r6 :128]
	vld1.8	{d10-d11}, [r7 :128]

	/* load the rest of the constants */
	add	r4, r12, #(sb1_0 - .Lconstants)
	add	r5, r12, #(sb1_1 - .Lconstants)
	add	r6, r12, #(sb2_0 - .Lconstants)
	add	r7, r12, #(sb2_1 - .Lconstants)
	add	r8, r12, #(inv - .Lconstants)
	add	r10, r12, #(inva - .Lconstants)
	vld1.8	{d12-d13}, [r4 :128]	/* q6 = sb1[0] */
	vld1.8	{d14-d15}, [r5 :128]	/* q7 = sb1[1] */
	vld1.8	{d16-d17}, [r6 :128]	/* q8 = sb2[0] */
	vld1.8	{d18-d19}, [r7 :128]	/* q9 = sb2[1] */
	vld1.8	{d20-d21}, [r8 :128]	/* q10 = inv */
	vld1.8	{d22-d23}, [r10 :128]	/* q11 = inva */

	/* (r4, r5) := (&mc_forward[0], &mc_backward[0]) */
	add	r4, r12, #(mc_forward - .Lconstants)
	add	r5, r12, #(mc_backward - .Lconstants)

	/* (q2, q3) := (lo, hi) */
	vshr.u8	q3, q0, #4
	vand	q2, q0, q1		/* q2 := x & 0x0f0f... */
	vand	q3, q3, q1		/* q3 := (x >> 4) & 0x0f0f... */

	/* (q2, q3) := (iptlo(lo), ipthi(hi)) */
	vtbl.8	d4, {d8-d9}, d4
	vtbl.8	d5, {d8-d9}, d5
	vtbl.8	d6, {d10-d11}, d6
	vtbl.8	d7, {d10-d11}, d7

	/* q0 := rk[0] + iptlo(lo) + ipthi(hi) */
	veor	q0, q14, q2
	veor	q0, q0, q3

	b	2f

	_ALIGN_TEXT
1:	vld1.8	{d28-d29}, [r0 :128]!	/* q14 = *rk++ */

	/* q0 := A = rk[i] + sb1_0(io) + sb1_1(jo) */
	vtbl.8	d24, {d12-d13}, d4
	vtbl.8	d25, {d12-d13}, d5
	vtbl.8	d26, {d14-d15}, d6
	vtbl.8	d27, {d14-d15}, d7
	veor	q0, q14, q12
	veor	q0, q0, q13

	/* q14 := A2 = sb2_0[io] + sb2_1[jo] */
	vtbl.8	d24, {d16-d17}, d4
	vtbl.8	d25, {d16-d17}, d5
	vtbl.8	d26, {d18-d19}, d6
	vtbl.8	d27, {d18-d19}, d7
	veor	q14, q12, q13

	/* (q12, q13) := (mc_forward[rmod4], mc_backward[rmod4]) */
	add	r6, r4, r3, lsl #4
	add	r7, r5, r3, lsl #4
	vld1.8	{d24-d25}, [r6]
	vld1.8	{d26-d27}, [r7]

	/* q15 := A2_B = A2 + A(mcf) */
	vtbl.8	d30, {d0-d1}, d24
	vtbl.8	d31, {d0-d1}, d25
	veor	q15, q15, q14

	/* q14 := A2_B_D = A2_B + A(mcb) */
	vtbl.8	d28, {d0-d1}, d26
	vtbl.8	d29, {d0-d1}, d27
	veor	q14, q14, q15

	/* q0 := x = A2_B_D + A2_B(mcf) */
	vtbl.8	d0, {d30-d31}, d24
	vtbl.8	d1, {d30-d31}, d25
	veor	q0, q0, q14

2:	/*
	 * SubBytes
	 */

	/* (q2, q3) := (k, i) */
	vshr.u8	q3, q0, #4
	vand	q2, q0, q1		/* q2 := x & 0x0f0f... */
	vand	q3, q3, q1		/* q3 := (x >> 4) & 0x0f0f... */

	/* q0 := a/k */
	vtbl.8	d0, {d22-d23}, d4
	vtbl.8	d1, {d22-d23}, d5

	/* q2 := j = i + k */
	veor	q2, q3, q2

	/* q12 := ir = 1/i */
	vtbl.8	d24, {d20-d21}, d6
	vtbl.8	d25, {d20-d21}, d7

	/* q13 := jr = 1/j */
	vtbl.8	d26, {d20-d21}, d4
	vtbl.8	d27, {d20-d21}, d5

	/* q12 := iak = 1/i + a/k */
	veor	q12, q12, q0

	/* q13 := jak = 1/j + a/k */
	veor	q13, q13, q0

	/* q12 := iakr = 1/(1/i + a/k) */
	vtbl.8	d24, {d20-d21}, d24
	vtbl.8	d25, {d20-d21}, d25

	/* q13 := jakr = 1/(1/j + a/k) */
	vtbl.8	d26, {d20-d21}, d26
	vtbl.8	d27, {d20-d21}, d27

	/* q2 := io = j + 1/(1/i + a/k) */
	veor	q2, q2, q12

	/* q3 := jo = i + 1/(1/j + a/k) */
	veor	q3, q3, q13

	/* advance round */
	add	r3, r3, #1
	subs	r1, r1, #1
	and	r3, r3, #3
	bne	1b

	/* (q6, q7, q15) := (sbo[0], sbo[1], sr[rmod4]) */
	add	r8, r12, #(sr - .Lconstants)
	add	r6, r12, #(sbo_0 - .Lconstants)
	add	r7, r12, #(sbo_1 - .Lconstants)
	add	r8, r8, r3, lsl #4
	vld1.8	{d12-d13}, [r6 :128]
	vld1.8	{d14-d15}, [r7 :128]
	vld1.8	{d30-d31}, [r8 :128]

	vld1.8	{d28-d29}, [r0 :128]!	/* q14 = *rk++ */

	/* (q2, q3) := (sbo_0(io), sbo_1(jo)) */
	vtbl.8	d4, {d12-d13}, d4
	vtbl.8	d5, {d12-d13}, d5
	vtbl.8	d6, {d14-d15}, d6
	vtbl.8	d7, {d14-d15}, d7

	/* q2 := x = rk[nr] + sbo_0(io) + sbo_1(jo) */
	veor	q2, q2, q14
	veor	q2, q2, q3

	/* q0 := x(sr[rmod4]) */
	vtbl.8	d0, {d4-d5}, d30
	vtbl.8	d1, {d4-d5}, d31

	vpop	{d8-d15}
	pop	{r4, r5, r6, r7, r8, r10, r11, lr}
#ifdef __SOFTFP__
#ifdef __ARM_BIG_ENDIAN
	vmov	r1, r0, d0
	vmov	r3, r2, d1
#else
	vmov	r0, r1, d0
	vmov	r2, r3, d1
#endif
#endif
	bx	lr
END(aes_neon_enc1)

/*
 * aes_neon_dec1(dec, x, nrounds)
 *
 *	With -mfloat-abi=hard:
 *
 * uint8x16_t@q0
 * aes_neon_dec1(const struct aesdec *dec@r0, uint8x16_t x@q0,
 *     unsigned nrounds@r1)
 *
 *	With -mfloat-abi=soft(fp) (here spelled `#ifdef _KERNEL'):
 *
 * uint8x16_t@(r0,r1,r2,r3)
 * aes_neon_dec1(const struct aesdec *dec@r0,
 *     uint8x16_t x@(r2,r3,sp[0],sp[4]), nrounds@sp[8])
 */
ENTRY(aes_neon_dec1)
#ifdef __SOFTFP__
#ifdef __ARM_BIG_ENDIAN
	vmov	d0, r3, r2		/* d0 := x lo */
#else
	vmov	d0, r2, r3		/* d0 := x lo */
#endif
	vldr	d1, [sp]		/* d1 := x hi */
	ldr	r1, [sp, #8]		/* r1 := nrounds */
#endif
	push	{r4, r5, r6, r7, r8, r10, r11, lr}
	vpush	{d8-d15}

	/*
	 * r3: 3 & ~(nrounds - 1)
	 * q0={d0-d1}: x/ak
	 * q1={d2-d3}: 0x0f0f...
	 * q2={d4-d5}: lo/k/j/io
	 * q3={d6-d7}: hi/i/jo
	 * q4={d8-d9}: diptlo/dsb9[0]
	 * q5={d10-d11}: dipthi/dsb9[1]
	 * q6={d12-d13}: dsbb[0]/dsbo[0]
	 * q7={d14-d15}: dsbb[1]/dsbo[1]
	 * q8={d16-d17}: dsbd[0]/dsbe[0]
	 * q9={d18-d19}: dsbd[1]/dsbe[0]
	 * q10={d20-d21}: inv
	 * q11={d22-d23}: inva
	 * q12={d24-d25}: ir/iak/iakr/dsbX_0(io)
	 * q13={d26-d27}: jr/jak/jakr/dsbX_1(jo)
	 * q14={d28-d29}: rk/xmc
	 * q15={d30-d31}: mc/sr[3 & ~(nrounds - 1)]
	 */

	/* r12 := .Lconstants - .Lconstants_addr, r11 := .Lconstants_addr */
	ldr	r12, .Lconstants_addr
	adr	r11, .Lconstants_addr

	vld1.8	{d28-d29}, [r0 :128]!	/* q14 = *rk++ */
	rsb	r3, r1, #0		/* r3 := ~(x - 1) = -x */
	vmov.i8	q1, #0x0f
	and	r3, r3, #3		/* r3 := 3 & ~(x - 1) */

	/* r12 := .Lconstants */
	add	r12, r12, r11

	/* (q4, q5) := (diptlo, dipthi) */
	add	r6, r12, #(diptlo - .Lconstants)
	add	r7, r12, #(dipthi - .Lconstants)
	vld1.8	{d8-d9}, [r6 :128]
	vld1.8	{d10-d11}, [r7 :128]

	/* load the rest of the constants */
	add	r4, r12, #(dsbb_0 - .Lconstants)
	add	r5, r12, #(dsbb_1 - .Lconstants)
	add	r6, r12, #(inv - .Lconstants)
	add	r7, r12, #(inva - .Lconstants)
	add	r8, r12, #(.Lmc_forward_3 - .Lconstants)
	vld1.8	{d12-d13}, [r4 :128]	/* q6 := dsbb[0] */
	vld1.8	{d14-d15}, [r5 :128]	/* q7 := dsbb[1] */
	vld1.8	{d20-d21}, [r6 :128]	/* q10 := inv */
	vld1.8	{d22-d23}, [r7 :128]	/* q11 := inva */
	vld1.8	{d30-d31}, [r8 :128]	/* q15 := mc_forward[3] */

	/* (q2, q3) := (lo, hi) */
	vshr.u8	q3, q0, #4
	vand	q2, q0, q1		/* q2 := x & 0x0f0f... */
	vand	q3, q3, q1		/* q3 := (x >> 4) & 0x0f0f... */

	/* (q2, q3) := (diptlo(lo), dipthi(hi)) */
	vtbl.8	d4, {d8-d9}, d4
	vtbl.8	d5, {d8-d9}, d5
	vtbl.8	d6, {d10-d11}, d6
	vtbl.8	d7, {d10-d11}, d7

	/* load dsb9 */
	add	r4, r12, #(dsb9_0 - .Lconstants)
	add	r5, r12, #(dsb9_1 - .Lconstants)
	vld1.8	{d8-d9}, [r4 :128]	/* q4 := dsb9[0] */
	vld1.8	{d10-d11}, [r5 :128]	/* q5 := dsb9[1] */

	/* q0 := rk[0] + diptlo(lo) + dipthi(hi) */
	veor	q0, q14, q2
	veor	q0, q0, q3

	b	2f

	_ALIGN_TEXT
1:	/* load dsbd */
	add	r4, r12, #(dsbd_0 - .Lconstants)
	vld1.8	{d16-d17}, [r4 :128]!	/* q8 := dsbd[0] */
	vld1.8	{d18-d19}, [r4 :128]	/* q9 := dsbd[1] */

	vld1.8	{d28-d29}, [r0 :128]!	/* q14 = *rk++ */

	/* q0 := rk[i] + dsb9_0(io) + dsb9_1(jo) */
	vtbl.8	d24, {d8-d9}, d4
	vtbl.8	d25, {d8-d9}, d5
	vtbl.8	d26, {d10-d11}, d6
	vtbl.8	d27, {d10-d11}, d7
	veor	q0, q14, q12
	veor	q0, q0, q13

	/* q14 := x(mc) */
	vtbl.8	d28, {d0-d1}, d30
	vtbl.8	d29, {d0-d1}, d31

	/* q0 := x(mc) + dsbd_0(io) + dsbd_1(jo) */
	vtbl.8	d24, {d16-d17}, d4
	vtbl.8	d25, {d16-d17}, d5
	vtbl.8	d26, {d18-d19}, d6
	vtbl.8	d27, {d18-d19}, d7
	veor	q0, q14, q12
	veor	q0, q0, q13

	/* load dsbe */
	add	r4, r12, #(dsbe_0 - .Lconstants)
	vld1.8	{d16-d17}, [r4 :128]!	/* q8 := dsbe[0] */
	vld1.8	{d18-d19}, [r4 :128]	/* q9 := dsbe[1] */

	/* q0 := x(mc) + dsbb_0(io) + dsbb_1(jo) */
	vtbl.8	d28, {d0-d1}, d30
	vtbl.8	d29, {d0-d1}, d31
	vtbl.8	d24, {d12-d13}, d4
	vtbl.8	d25, {d12-d13}, d5
	vtbl.8	d26, {d14-d15}, d6
	vtbl.8	d27, {d14-d15}, d7
	veor	q0, q14, q12
	veor	q0, q0, q13

	/* q0 := x(mc) + dsbe_0(io) + dsbe_1(jo) */
	vtbl.8	d28, {d0-d1}, d30
	vtbl.8	d29, {d0-d1}, d31
	vtbl.8	d24, {d16-d17}, d4
	vtbl.8	d25, {d16-d17}, d5
	vtbl.8	d26, {d18-d19}, d6
	vtbl.8	d27, {d18-d19}, d7
	veor	q0, q14, q12
	veor	q0, q0, q13

	/* q15 := mc := mc <<< 12*8 */
	vext.8	q15, q15, q15, #12

2:	/*
	 * SubBytes
	 */

	/* (q2, q3) := (k, i) */
	vshr.u8	q3, q0, #4
	vand	q2, q0, q1		/* q2 := x & 0x0f0f... */
	vand	q3, q3, q1		/* q3 := (x >> 4) & 0x0f0f... */

	/* q0 := a/k */
	vtbl.8	d0, {d22-d23}, d4
	vtbl.8	d1, {d22-d23}, d5

	/* q2 := j = i + k */
	veor	q2, q3, q2

	/* q12 := ir = 1/i */
	vtbl.8	d24, {d20-d21}, d6
	vtbl.8	d25, {d20-d21}, d7

	/* q13 := jr = 1/j */
	vtbl.8	d26, {d20-d21}, d4
	vtbl.8	d27, {d20-d21}, d5

	/* q12 := iak = 1/i + a/k */
	veor	q12, q12, q0

	/* q13 := jak = 1/j + a/k */
	veor	q13, q13, q0

	/* q12 := iakr = 1/(1/i + a/k) */
	vtbl.8	d24, {d20-d21}, d24
	vtbl.8	d25, {d20-d21}, d25

	/* q13 := jakr = 1/(1/j + a/k) */
	vtbl.8	d26, {d20-d21}, d26
	vtbl.8	d27, {d20-d21}, d27

	/* q2 := io = j + 1/(1/i + a/k) */
	veor	q2, q2, q12

	/* q3 := jo = i + 1/(1/j + a/k) */
	veor	q3, q3, q13

	/* advance round */
	subs	r1, r1, #1
	bne	1b

	/* (q6, q7, q15) := (dsbo[0], dsbo[1], sr[i]) */
	add	r8, r12, #(sr - .Lconstants)
	add	r6, r12, #(dsbo_0 - .Lconstants)
	add	r7, r12, #(dsbo_1 - .Lconstants)
	add	r8, r8, r3, lsl #4
	vld1.8	{d12-d13}, [r6 :128]
	vld1.8	{d14-d15}, [r7 :128]
	vld1.8	{d30-d31}, [r8 :128]

	vld1.8	{d28-d29}, [r0 :128]!	/* q14 = *rk++ */

	/* (q2, q3) := (dsbo_0(io), dsbo_1(jo)) */
	vtbl.8	d4, {d12-d13}, d4
	vtbl.8	d5, {d12-d13}, d5
	vtbl.8	d6, {d14-d15}, d6
	vtbl.8	d7, {d14-d15}, d7

	/* q2 := x = rk[nr] + dsbo_0(io) + dsbo_1(jo) */
	veor	q2, q2, q14
	veor	q2, q2, q3

	/* q0 := x(sr[i]) */
	vtbl.8	d0, {d4-d5}, d30
	vtbl.8	d1, {d4-d5}, d31

	vpop	{d8-d15}
	pop	{r4, r5, r6, r7, r8, r10, r11, lr}
#ifdef __SOFTFP__
#ifdef __ARM_BIG_ENDIAN
	vmov	r1, r0, d0
	vmov	r3, r2, d1
#else
	vmov	r0, r1, d0
	vmov	r2, r3, d1
#endif
#endif
	bx	lr
END(aes_neon_dec1)
