/*	$NetBSD: aes_armv8_64.S,v 1.12 2020/08/08 14:47:01 riastradh Exp $	*/

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

#include <aarch64/asm.h>

RCSID("$NetBSD: aes_armv8_64.S,v 1.12 2020/08/08 14:47:01 riastradh Exp $")

	.arch_extension	aes

/*
 * uint32_t rcon[10]
 *
 *	Table mapping n ---> x^n mod (x^8 + x^4 + x^3 + x + 1) in GF(2).
 *	Such elements of GF(8) need only eight bits to be represented,
 *	but we store them in 4-byte units so we can copy one into all
 *	four 4-byte lanes of a vector register with a single LD1R.  The
 *	access pattern is fixed, so indices into this table are never
 *	secret.
 */
	.section .rodata
	.p2align 2
	.type	rcon,@object
rcon:
	.long	0x01
	.long	0x02
	.long	0x04
	.long	0x08
	.long	0x10
	.long	0x20
	.long	0x40
	.long	0x80
	.long	0x1b
	.long	0x36
END(rcon)

/*
 * uint128_t unshiftrows_rotword_1
 *
 *	Table for TBL instruction to undo ShiftRows, and then do
 *	RotWord on word 1, and then copy it into all the other words.
 */
	.section .rodata
	.p2align 4
	.type	unshiftrows_rotword_1,@object
unshiftrows_rotword_1:
	.byte	0x01,0x0e,0x0b,0x04
	.byte	0x01,0x0e,0x0b,0x04
	.byte	0x01,0x0e,0x0b,0x04
	.byte	0x01,0x0e,0x0b,0x04
END(unshiftrows_rotword_1)

/*
 * uint128_t unshiftrows_3
 *
 *	Table for TBL instruction to undo ShiftRows, and then copy word
 *	3 into all the other words.
 */
	.section .rodata
	.p2align 4
	.type	unshiftrows_3,@object
unshiftrows_3:
	.byte	0x0c,0x09,0x06,0x03
	.byte	0x0c,0x09,0x06,0x03
	.byte	0x0c,0x09,0x06,0x03
	.byte	0x0c,0x09,0x06,0x03
END(unshiftrows_3)

/*
 * uint128_t unshiftrows_rotword_3
 *
 *	Table for TBL instruction to undo ShiftRows, and then do
 *	RotWord on word 3, and then copy it into all the other words.
 */
	.section .rodata
	.p2align 4
	.type	unshiftrows_rotword_3,@object
unshiftrows_rotword_3:
	.byte	0x09,0x06,0x03,0x0c
	.byte	0x09,0x06,0x03,0x0c
	.byte	0x09,0x06,0x03,0x0c
	.byte	0x09,0x06,0x03,0x0c
END(unshiftrows_rotword_3)

/*
 * aesarmv8_setenckey128(struct aesenc *enckey@x0, const uint8_t key[16] @x1)
 *
 *	Expand a 16-byte AES-128 key into 10 round keys.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_setenckey128)
	ldr	q1, [x1]	/* q1 := master key */

	adrl	x4, unshiftrows_rotword_3
	eor	v0.16b, v0.16b, v0.16b	/* q0 := 0 */
	ldr	q16, [x4]	/* q16 := unshiftrows_rotword_3 table */

	str	q1, [x0], #0x10	/* store master key as first round key */
	mov	x2, #10		/* round count */
	adrl	x3, rcon	/* round constant */

1:	/*
	 * q0 = 0
	 * v1.4s = (prk[0], prk[1], prk[2], prk[3])
	 * x0 = pointer to round key to compute
	 * x2 = round count
	 * x3 = rcon pointer
	 */

	/* q3 := ShiftRows(SubBytes(q1)) */
	mov	v3.16b, v1.16b
	aese	v3.16b, v0.16b

	/* v3.4s[i] := RotWords(SubBytes(prk[3])) ^ RCON */
	ld1r	{v4.4s}, [x3], #4
	tbl	v3.16b, {v3.16b}, v16.16b
	eor	v3.16b, v3.16b, v4.16b

	/*
	 * v5.4s := (0,prk[0],prk[1],prk[2])
	 * v6.4s := (0,0,prk[0],prk[1])
	 * v7.4s := (0,0,0,prk[0])
	 */
	ext	v5.16b, v0.16b, v1.16b, #12
	ext	v6.16b, v0.16b, v1.16b, #8
	ext	v7.16b, v0.16b, v1.16b, #4

	/* v1.4s := (rk[0], rk[1], rk[2], rk[3]) */
	eor	v1.16b, v1.16b, v3.16b
	eor	v1.16b, v1.16b, v5.16b
	eor	v1.16b, v1.16b, v6.16b
	eor	v1.16b, v1.16b, v7.16b

	subs	x2, x2, #1	/* count down rounds */
	str	q1, [x0], #0x10	/* store round key */
	b.ne	1b

	ret
END(aesarmv8_setenckey128)

/*
 * aesarmv8_setenckey192(struct aesenc *enckey@x0, const uint8_t key[24] @x1)
 *
 *	Expand a 24-byte AES-192 key into 12 round keys.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_setenckey192)
	ldr	q1, [x1], #0x10	/* q1 := master key[0:128) */
	ldr	d2, [x1]	/* d2 := master key[128:192) */

	adrl	x4, unshiftrows_rotword_1
	adrl	x5, unshiftrows_rotword_3
	eor	v0.16b, v0.16b, v0.16b	/* q0 := 0 */
	ldr	q16, [x4]	/* q16 := unshiftrows_rotword_1 */
	ldr	q17, [x5]	/* q17 := unshiftrows_rotword_3 */

	str	q1, [x0], #0x10	/* store master key[0:128) as round key */
	mov	x2, #12		/* round count */
	adrl	x3, rcon	/* round constant */

1:	/*
	 * q0 = 0
	 * v1.4s = (prk[0], prk[1], prk[2], prk[3])
	 * v2.4s = (rklo[0], rklo[1], xxx, xxx)
	 * x0 = pointer to three round keys to compute
	 * x2 = round count
	 * x3 = rcon pointer
	 */

	/* q3 := ShiftRows(SubBytes(q2)) */
	mov	v3.16b, v2.16b
	aese	v3.16b, v0.16b

	/* v3.4s[i] := RotWords(SubBytes(rklo[1])) ^ RCON */
	ld1r	{v4.4s}, [x3], #4
	tbl	v3.16b, {v3.16b}, v16.16b
	eor	v3.16b, v3.16b, v4.16b

	/*
	 * We need to compute:
	 *
	 * rk[0] := rklo[0]
	 * rk[1] := rklo[1]
	 * rk[2] := Rot(Sub(rklo[1])) ^ RCON ^ prk[0]
	 * rk[3] := Rot(Sub(rklo[1])) ^ RCON ^ prk[0] ^ prk[1]
	 * nrk[0] := Rot(Sub(rklo[1])) ^ RCON ^ prk[0] ^ prk[1] ^ prk[2]
	 * nrk[1] := Rot(Sub(rklo[1])) ^ RCON ^ prk[0] ^ ... ^ prk[3]
	 * nrk[2] := Rot(Sub(rklo[1])) ^ RCON ^ prk[0] ^ ... ^ prk[3] ^ rklo[0]
	 * nrk[3] := Rot(Sub(rklo[1])) ^ RCON ^ prk[0] ^ ... ^ prk[3] ^ rklo[0]
	 *     ^ rklo[1]
	 */

	/*
	 * v5.4s := (0,prk[0],prk[1],prk[2])
	 * v6.4s := (0,0,prk[0],prk[1])
	 * v7.4s := (0,0,0,prk[0])
	 */
	ext	v5.16b, v0.16b, v1.16b, #12
	ext	v6.16b, v0.16b, v1.16b, #8
	ext	v7.16b, v0.16b, v1.16b, #4

	/* v5.4s := (rk[2], rk[3], nrk[0], nrk[1]) */
	eor	v5.16b, v5.16b, v1.16b
	eor	v5.16b, v5.16b, v3.16b
	eor	v5.16b, v5.16b, v6.16b
	eor	v5.16b, v5.16b, v7.16b

	/*
	 * At this point, rk is split across v2.4s = (rk[0],rk[1],...)
	 * and v5.4s = (rk[2],rk[3],...); nrk is in v5.4s =
	 * (...,nrk[0],nrk[1]); and we have yet to compute nrk[2] or
	 * nrk[3], which requires rklo[0] and rklo[1] in v2.4s =
	 * (rklo[0],rklo[1],...).
	 */

	/* v1.4s := (nrk[0], nrk[1], nrk[1], nrk[1]) */
	dup	v1.4s, v5.s[3]
	mov	v1.s[0], v5.s[2]

	/*
	 * v6.4s := (0, 0, rklo[0], rklo[1])
	 * v7.4s := (0, 0, 0, rklo[0])
	 */
	ext	v6.16b, v0.16b, v2.16b, #8
	ext	v7.16b, v0.16b, v2.16b, #4

	/* v3.4s := (nrk[0], nrk[1], nrk[2], nrk[3]) */
	eor	v3.16b, v1.16b, v6.16b
	eor	v3.16b, v3.16b, v7.16b

	/*
	 * Recall v2.4s = (rk[0], rk[1], xxx, xxx)
	 * and v5.4s = (rk[2], rk[3], xxx, xxx).  Set
	 * v2.4s := (rk[0], rk[1], rk[2], rk[3])
	 */
	mov	v2.d[1], v5.d[0]

	/* store two round keys */
	stp	q2, q3, [x0], #0x20

	/*
	 * Live vector registers at this point:
	 *
	 *	q0 = zero
	 *	q2 = rk
	 *	q3 = nrk
	 *	v5.4s = (rk[2], rk[3], nrk[0], nrk[1])
	 *	q16 = unshiftrows_rotword_1
	 *	q17 = unshiftrows_rotword_3
	 *
	 * We have to compute, in q1:
	 *
	 * nnrk[0] := Rot(Sub(nrk[3])) ^ RCON' ^ rk[2]
	 * nnrk[1] := Rot(Sub(nrk[3])) ^ RCON' ^ rk[2] ^ rk[3]
	 * nnrk[2] := Rot(Sub(nrk[3])) ^ RCON' ^ rk[2] ^ rk[3] ^ nrk[0]
	 * nnrk[3] := Rot(Sub(nrk[3])) ^ RCON' ^ rk[2] ^ rk[3] ^ nrk[0]
	 *     ^ nrk[1]
	 *
	 * And, if there's any more afterward, in q2:
	 *
	 * nnnrklo[0] := Rot(Sub(nrk[3])) ^ RCON' ^ rk[2] ^ rk[3] ^ nrk[0]
	 *     ^ nrk[1] ^ nrk[2]
	 * nnnrklo[1] := Rot(Sub(nrk[3])) ^ RCON' ^ rk[2] ^ rk[3] ^ nrk[0]
	 *     ^ nrk[1] ^ nrk[2] ^ nrk[3]
	 */

	/* q1 := RotWords(SubBytes(q3)) */
	mov	v1.16b, v3.16b
	aese	v1.16b, v0.16b

	/* v1.4s[i] := RotWords(SubBytes(nrk[3])) ^ RCON' */
	ld1r	{v4.4s}, [x3], #4
	tbl	v1.16b, {v1.16b}, v17.16b
	eor	v1.16b, v1.16b, v4.16b

	/*
	 * v5.4s := (rk[2], rk[3], nrk[0], nrk[1]) [already]
	 * v4.4s := (0, rk[2], rk[3], nrk[0])
	 * v6.4s := (0, 0, rk[2], rk[3])
	 * v7.4s := (0, 0, 0, rk[2])
	 */
	ext	v4.16b, v0.16b, v5.16b, #12
	ext	v6.16b, v0.16b, v5.16b, #8
	ext	v7.16b, v0.16b, v5.16b, #4

	/* v1.4s := (nnrk[0], nnrk[1], nnrk[2], nnrk[3]) */
	eor	v1.16b, v1.16b, v5.16b
	eor	v1.16b, v1.16b, v4.16b
	eor	v1.16b, v1.16b, v6.16b
	eor	v1.16b, v1.16b, v7.16b

	subs	x2, x2, #3	/* count down three rounds */
	str	q1, [x0], #0x10	/* store third round key */
	b.eq	2f

	/*
	 * v4.4s := (nrk[2], nrk[3], xxx, xxx)
	 * v5.4s := (0, nrk[2], xxx, xxx)
	 */
	ext	v4.16b, v3.16b, v0.16b, #8
	ext	v5.16b, v0.16b, v4.16b, #12

	/* v2.4s := (nnrk[3], nnrk[3], xxx, xxx) */
	dup	v2.4s, v1.s[3]

	/*
	 * v2.4s := (nnnrklo[0] = nnrk[3] ^ nrk[2],
	 *     nnnrklo[1] = nnrk[3] ^ nrk[2] ^ nrk[3],
	 *     xxx, xxx)
	 */
	eor	v2.16b, v2.16b, v4.16b
	eor	v2.16b, v2.16b, v5.16b

	b	1b

2:	ret
END(aesarmv8_setenckey192)

/*
 * aesarmv8_setenckey256(struct aesenc *enckey@x0, const uint8_t key[32] @x1)
 *
 *	Expand a 32-byte AES-256 key into 14 round keys.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_setenckey256)
	/* q1 := key[0:128), q2 := key[128:256) */
	ldp	q1, q2, [x1], #0x20

	adrl	x4, unshiftrows_rotword_3
	adrl	x5, unshiftrows_3
	eor	v0.16b, v0.16b, v0.16b	/* q0 := 0 */
	ldr	q16, [x4]	/* q16 := unshiftrows_rotword_3 */
	ldr	q17, [x5]	/* q17 := unshiftrows_3 */

	/* store master key as first two round keys */
	stp	q1, q2, [x0], #0x20
	mov	x2, #14		/* round count */
	adrl	x3, rcon	/* round constant */

1:	/*
	 * q0 = 0
	 * v1.4s = (pprk[0], pprk[1], pprk[2], pprk[3])
	 * v2.4s = (prk[0], prk[1], prk[2], prk[3])
	 * x2 = round count
	 * x3 = rcon pointer
	 */

	/* q3 := ShiftRows(SubBytes(q2)) */
	mov	v3.16b, v2.16b
	aese	v3.16b, v0.16b

	/* v3.4s[i] := RotWords(SubBytes(prk[3])) ^ RCON */
	ld1r	{v4.4s}, [x3], #4
	tbl	v3.16b, {v3.16b}, v16.16b
	eor	v3.16b, v3.16b, v4.16b

	/*
	 * v5.4s := (0,pprk[0],pprk[1],pprk[2])
	 * v6.4s := (0,0,pprk[0],pprk[1])
	 * v7.4s := (0,0,0,pprk[0])
	 */
	ext	v5.16b, v0.16b, v1.16b, #12
	ext	v6.16b, v0.16b, v1.16b, #8
	ext	v7.16b, v0.16b, v1.16b, #4

	/* v1.4s := (rk[0], rk[1], rk[2], rk[3]) */
	eor	v1.16b, v1.16b, v3.16b
	eor	v1.16b, v1.16b, v5.16b
	eor	v1.16b, v1.16b, v6.16b
	eor	v1.16b, v1.16b, v7.16b

	subs	x2, x2, #2		/* count down two rounds */
	b.eq	2f			/* stop if this is the last one */

	/* q3 := ShiftRows(SubBytes(q1)) */
	mov	v3.16b, v1.16b
	aese	v3.16b, v0.16b

	/* v3.4s[i] := SubBytes(rk[3]) */
	tbl	v3.16b, {v3.16b}, v17.16b

	/*
	 * v5.4s := (0,prk[0],prk[1],prk[2])
	 * v6.4s := (0,0,prk[0],prk[1])
	 * v7.4s := (0,0,0,prk[0])
	 */
	ext	v5.16b, v0.16b, v2.16b, #12
	ext	v6.16b, v0.16b, v2.16b, #8
	ext	v7.16b, v0.16b, v2.16b, #4

	/* v2.4s := (nrk[0], nrk[1], nrk[2], nrk[3]) */
	eor	v2.16b, v2.16b, v3.16b
	eor	v2.16b, v2.16b, v5.16b
	eor	v2.16b, v2.16b, v6.16b
	eor	v2.16b, v2.16b, v7.16b

	stp	q1, q2, [x0], #0x20	/* store two round keys */
	b	1b

2:	str	q1, [x0]		/* store last round key */
	ret
END(aesarmv8_setenckey256)

/*
 * aesarmv8_enctodec(const struct aesenc *enckey@x0, struct aesdec *deckey@x1,
 *     uint32_t nrounds@x2)
 *
 *	Convert AES encryption round keys to AES decryption round keys.
 *	`rounds' must be between 10 and 14.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_enctodec)
	ldr	q0, [x0, x2, lsl #4]	/* load last round key */
	b	2f
	_ALIGN_TEXT
1:	aesimc	v0.16b, v0.16b	/* convert encryption to decryption */
2:	str	q0, [x1], #0x10	/* store round key */
	subs	x2, x2, #1	/* count down round */
	ldr	q0, [x0, x2, lsl #4]	/* load previous round key */
	b.ne	1b		/* repeat if there's more */
	str	q0, [x1]	/* store first round key verbatim */
	ret
END(aesarmv8_enctodec)

/*
 * aesarmv8_enc(const struct aesenc *enckey@x0, const uint8_t in[16] @x1,
 *     uint8_t out[16] @x2, uint32_t nrounds@x3)
 *
 *	Encrypt a single block.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_enc)
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	ldr	q0, [x1]	/* q0 := ptxt */
	bl	aesarmv8_enc1	/* q0 := ctxt; trash x0/x3/q16 */
	str	q0, [x2]	/* store ctxt */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
	ret
END(aesarmv8_enc)

/*
 * aesarmv8_dec(const struct aesdec *deckey@x0, const uint8_t in[16] @x1,
 *     uint8_t out[16] @x2, uint32_t nrounds@x3)
 *
 *	Decrypt a single block.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_dec)
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	ldr	q0, [x1]	/* q0 := ctxt */
	bl	aesarmv8_dec1	/* q0 := ptxt; trash x0/x3/q16 */
	str	q0, [x2]	/* store ptxt */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
	ret
END(aesarmv8_dec)

/*
 * aesarmv8_cbc_enc(const struct aesenc *enckey@x0, const uint8_t *in@x1,
 *     uint8_t *out@x2, size_t nbytes@x3, uint8_t iv[16] @x4,
 *     uint32_t nrounds@x5)
 *
 *	Encrypt a contiguous sequence of blocks with AES-CBC.
 *
 *	nbytes must be an integral multiple of 16.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_cbc_enc)
	cbz	x3, 2f			/* stop if nothing to do */
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	mov	x9, x0			/* x9 := enckey */
	mov	x10, x3			/* x10 := nbytes */
	ldr	q0, [x4]		/* q0 := chaining value */
	_ALIGN_TEXT
1:	ldr	q1, [x1], #0x10		/* q1 := plaintext block */
	eor	v0.16b, v0.16b, v1.16b	/* q0 := cv ^ ptxt */
	mov	x0, x9			/* x0 := enckey */
	mov	x3, x5			/* x3 := nrounds */
	bl	aesarmv8_enc1		/* q0 := ctxt; trash x0/x3/q16 */
	subs	x10, x10, #0x10		/* count down nbytes */
	str	q0, [x2], #0x10		/* store ciphertext block */
	b.ne	1b			/* repeat if x10 is nonzero */
	str	q0, [x4]		/* store chaining value */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
2:	ret
END(aesarmv8_cbc_enc)

/*
 * aesarmv8_cbc_dec1(const struct aesdec *deckey@x0, const uint8_t *in@x1,
 *     uint8_t *out@x2, size_t nbytes@x3, const uint8_t iv[16] @x4,
 *     uint32_t nrounds@x5)
 *
 *	Decrypt a contiguous sequence of blocks with AES-CBC.
 *
 *	nbytes must be a positive integral multiple of 16.  This routine
 *	is not vectorized; use aesarmv8_cbc_dec8 for >=8 blocks at once.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_cbc_dec1)
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	ldr	q24, [x4]		/* q24 := iv */
	mov	x9, x0			/* x9 := enckey */
	mov	x10, x3			/* x10 := nbytes */
	add	x1, x1, x3		/* x1 := pointer past end of in */
	add	x2, x2, x3		/* x2 := pointer past end of out */
	ldr	q0, [x1, #-0x10]!	/* q0 := last ciphertext block */
	str	q0, [x4]		/* update iv */
	b	2f
	_ALIGN_TEXT
1:	ldr	q31, [x1, #-0x10]!	/* q31 := chaining value */
	eor	v0.16b, v0.16b, v31.16b	/* q0 := plaintext block */
	str	q0, [x2, #-0x10]!	/* store plaintext block */
	mov	v0.16b, v31.16b		/* move cv = ciphertext block */
2:	mov	x0, x9			/* x0 := enckey */
	mov	x3, x5			/* x3 := nrounds */
	bl	aesarmv8_dec1		/* q0 := cv ^ ptxt; trash x0/x3/q16 */
	subs	x10, x10, #0x10		/* count down nbytes */
	b.ne	1b			/* repeat if more blocks */
	eor	v0.16b, v0.16b, v24.16b	/* q0 := first plaintext block */
	str	q0, [x2, #-0x10]!	/* store first plaintext block */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
	ret
END(aesarmv8_cbc_dec1)

/*
 * aesarmv8_cbc_dec8(const struct aesdec *deckey@x0, const uint8_t *in@x1,
 *     uint8_t *out@x2, size_t nbytes@x3, const uint8_t iv[16] @x4,
 *     uint32_t nrounds@x5)
 *
 *	Decrypt a contiguous sequence of 8-block units with AES-CBC.
 *
 *	nbytes must be a positive integral multiple of 128.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_cbc_dec8)
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	ldr	q24, [x4]		/* q24 := iv */
	mov	x9, x0			/* x9 := enckey */
	mov	x10, x3			/* x10 := nbytes */
	add	x1, x1, x3		/* x1 := pointer past end of in */
	add	x2, x2, x3		/* x2 := pointer past end of out */
	ldp	q6, q7, [x1, #-0x20]!	/* q6, q7 := last ciphertext blocks */
	str	q7, [x4]		/* update iv */
	b	2f
	_ALIGN_TEXT
1:	ldp	q6, q7, [x1, #-0x20]!
	eor	v0.16b, v0.16b, v7.16b	/* q0 := pt0 */
	stp	q0, q1, [x2, #-0x20]!
2:	ldp	q4, q5, [x1, #-0x20]!
	ldp	q2, q3, [x1, #-0x20]!
	ldp	q0, q1, [x1, #-0x20]!
	mov	v31.16b, v6.16b		/* q[24+i] := cv[i], 0<i<8 */
	mov	v30.16b, v5.16b
	mov	v29.16b, v4.16b
	mov	v28.16b, v3.16b
	mov	v27.16b, v2.16b
	mov	v26.16b, v1.16b
	mov	v25.16b, v0.16b
	mov	x0, x9			/* x0 := enckey */
	mov	x3, x5			/* x3 := nrounds */
	bl	aesarmv8_dec8		/* q[i] := cv[i] ^ pt[i];
					 * trash x0/x3/q16 */
	eor	v7.16b, v7.16b, v31.16b	/* q[i] := pt[i] */
	eor	v6.16b, v6.16b, v30.16b
	eor	v5.16b, v5.16b, v29.16b
	eor	v4.16b, v4.16b, v28.16b
	eor	v3.16b, v3.16b, v27.16b
	eor	v2.16b, v2.16b, v26.16b
	eor	v1.16b, v1.16b, v25.16b
	subs	x10, x10, #0x80		/* count down nbytes */
	stp	q6, q7, [x2, #-0x20]!	/* store plaintext blocks */
	stp	q4, q5, [x2, #-0x20]!
	stp	q2, q3, [x2, #-0x20]!
	b.ne	1b			/* repeat if there's more */
	eor	v0.16b, v0.16b, v24.16b	/* q0 := pt0 */
	stp	q0, q1, [x2, #-0x20]!	/* store first two plaintext blocks */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
	ret
END(aesarmv8_cbc_dec8)

/*
 * aesarmv8_xts_enc1(const struct aesenc *enckey@x0, const uint8_t *in@x1,
 *     uint8_t *out@x2, size_t nbytes@x3, uint8_t tweak[16] @x4,
 *     uint32_t nrounds@x5)
 *
 *	Encrypt a contiguous sequence of blocks with AES-XTS.
 *
 *	nbytes must be a positive integral multiple of 16.  This routine
 *	is not vectorized; use aesarmv8_xts_enc8 for >=8 blocks at once.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_xts_enc1)
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	mov	x9, x0			/* x9 := enckey */
	mov	x10, x3			/* x10 := nbytes */
	ldr	q31, [x4]		/* q31 := tweak */
	_ALIGN_TEXT
1:	ldr	q0, [x1], #0x10		/* q0 := ptxt */
	mov	x0, x9			/* x0 := enckey */
	mov	x3, x5			/* x3 := nrounds */
	eor	v0.16b, v0.16b, v31.16b	/* q0 := ptxt ^ tweak */
	bl	aesarmv8_enc1		/* q0 := AES(...); trash x0/x3/q16 */
	eor	v0.16b, v0.16b, v31.16b	/* q0 := AES(ptxt ^ tweak) ^ tweak */
	str	q0, [x2], #0x10		/* store ciphertext block */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	subs	x10, x10, #0x10		/* count down nbytes */
	b.ne	1b			/* repeat if more blocks */
	str	q31, [x4]		/* update tweak */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
	ret
END(aesarmv8_xts_enc1)

/*
 * aesarmv8_xts_enc8(const struct aesenc *enckey@x0, const uint8_t *in@x1,
 *     uint8_t *out@x2, size_t nbytes@x3, uint8_t tweak[16] @x4,
 *     uint32_t nrounds@x5)
 *
 *	Encrypt a contiguous sequence of blocks with AES-XTS.
 *
 *	nbytes must be a positive integral multiple of 128.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_xts_enc8)
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	mov	x9, x0			/* x9 := enckey */
	mov	x10, x3			/* x10 := nbytes */
	ldr	q31, [x4]		/* q31 := tweak */
	_ALIGN_TEXT
1:	mov	v24.16b, v31.16b	/* q24 := tweak[0] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	mov	v25.16b, v31.16b	/* q25 := tweak[1] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	mov	v26.16b, v31.16b	/* q26 := tweak[2] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	mov	v27.16b, v31.16b	/* q27 := tweak[3] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	mov	v28.16b, v31.16b	/* q28 := tweak[4] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	mov	v29.16b, v31.16b	/* q29 := tweak[5] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	mov	v30.16b, v31.16b	/* q30 := tweak[6] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
					/* q31 := tweak[7] */
	ldp	q0, q1, [x1], #0x20	/* q[i] := ptxt[i] */
	ldp	q2, q3, [x1], #0x20
	ldp	q4, q5, [x1], #0x20
	ldp	q6, q7, [x1], #0x20
	eor	v0.16b, v0.16b, v24.16b	/* q[i] := ptxt[i] ^ tweak[i] */
	eor	v1.16b, v1.16b, v25.16b
	eor	v2.16b, v2.16b, v26.16b
	eor	v3.16b, v3.16b, v27.16b
	eor	v4.16b, v4.16b, v28.16b
	eor	v5.16b, v5.16b, v29.16b
	eor	v6.16b, v6.16b, v30.16b
	eor	v7.16b, v7.16b, v31.16b
	mov	x0, x9			/* x0 := enckey */
	mov	x3, x5			/* x3 := nrounds */
	bl	aesarmv8_enc8		/* encrypt q0-q7; trash x0/x3/q16 */
	eor	v0.16b, v0.16b, v24.16b	/* q[i] := AES(...) ^ tweak[i] */
	eor	v1.16b, v1.16b, v25.16b
	eor	v2.16b, v2.16b, v26.16b
	eor	v3.16b, v3.16b, v27.16b
	eor	v4.16b, v4.16b, v28.16b
	eor	v5.16b, v5.16b, v29.16b
	eor	v6.16b, v6.16b, v30.16b
	eor	v7.16b, v7.16b, v31.16b
	stp	q0, q1, [x2], #0x20	/* store ciphertext blocks */
	stp	q2, q3, [x2], #0x20
	stp	q4, q5, [x2], #0x20
	stp	q6, q7, [x2], #0x20
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	subs	x10, x10, #0x80		/* count down nbytes */
	b.ne	1b			/* repeat if more block groups */
	str	q31, [x4]		/* update tweak */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
	ret
END(aesarmv8_xts_enc8)

/*
 * aesarmv8_xts_dec1(const struct aesdec *deckey@x0, const uint8_t *in@x1,
 *     uint8_t *out@x2, size_t nbytes@x3, uint8_t tweak[16] @x4,
 *     uint32_t nrounds@x5)
 *
 *	Decrypt a contiguous sequdece of blocks with AES-XTS.
 *
 *	nbytes must be a positive integral multiple of 16.  This routine
 *	is not vectorized; use aesarmv8_xts_dec8 for >=8 blocks at once.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_xts_dec1)
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	mov	x9, x0			/* x9 := deckey */
	mov	x10, x3			/* x10 := nbytes */
	ldr	q31, [x4]		/* q31 := tweak */
	_ALIGN_TEXT
1:	ldr	q0, [x1], #0x10		/* q0 := ctxt */
	mov	x0, x9			/* x0 := deckey */
	mov	x3, x5			/* x3 := nrounds */
	eor	v0.16b, v0.16b, v31.16b	/* q0 := ctxt ^ tweak */
	bl	aesarmv8_dec1		/* q0 := AES(...); trash x0/x3/q16 */
	eor	v0.16b, v0.16b, v31.16b	/* q0 := AES(ctxt ^ tweak) ^ tweak */
	str	q0, [x2], #0x10		/* store plaintext block */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	subs	x10, x10, #0x10		/* count down nbytes */
	b.ne	1b			/* repeat if more blocks */
	str	q31, [x4]		/* update tweak */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
	ret
END(aesarmv8_xts_dec1)

/*
 * aesarmv8_xts_dec8(const struct aesdec *deckey@x0, const uint8_t *in@x1,
 *     uint8_t *out@x2, size_t nbytes@x3, uint8_t tweak[16] @x4,
 *     uint32_t nrounds@x5)
 *
 *	Decrypt a contiguous sequdece of blocks with AES-XTS.
 *
 *	nbytes must be a positive integral multiple of 128.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_xts_dec8)
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	mov	x9, x0			/* x9 := deckey */
	mov	x10, x3			/* x10 := nbytes */
	ldr	q31, [x4]		/* q31 := tweak */
	_ALIGN_TEXT
1:	mov	v24.16b, v31.16b	/* q24 := tweak[0] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	mov	v25.16b, v31.16b	/* q25 := tweak[1] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	mov	v26.16b, v31.16b	/* q26 := tweak[2] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	mov	v27.16b, v31.16b	/* q27 := tweak[3] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	mov	v28.16b, v31.16b	/* q28 := tweak[4] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	mov	v29.16b, v31.16b	/* q29 := tweak[5] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	mov	v30.16b, v31.16b	/* q30 := tweak[6] */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
					/* q31 := tweak[7] */
	ldp	q0, q1, [x1], #0x20	/* q[i] := ctxt[i] */
	ldp	q2, q3, [x1], #0x20
	ldp	q4, q5, [x1], #0x20
	ldp	q6, q7, [x1], #0x20
	eor	v0.16b, v0.16b, v24.16b	/* q[i] := ctxt[i] ^ tweak[i] */
	eor	v1.16b, v1.16b, v25.16b
	eor	v2.16b, v2.16b, v26.16b
	eor	v3.16b, v3.16b, v27.16b
	eor	v4.16b, v4.16b, v28.16b
	eor	v5.16b, v5.16b, v29.16b
	eor	v6.16b, v6.16b, v30.16b
	eor	v7.16b, v7.16b, v31.16b
	mov	x0, x9			/* x0 := deckey */
	mov	x3, x5			/* x3 := nrounds */
	bl	aesarmv8_dec8		/* decrypt q0-q7; trash x0/x3/q16 */
	eor	v0.16b, v0.16b, v24.16b	/* q[i] := AES(...) ^ tweak[i] */
	eor	v1.16b, v1.16b, v25.16b
	eor	v2.16b, v2.16b, v26.16b
	eor	v3.16b, v3.16b, v27.16b
	eor	v4.16b, v4.16b, v28.16b
	eor	v5.16b, v5.16b, v29.16b
	eor	v6.16b, v6.16b, v30.16b
	eor	v7.16b, v7.16b, v31.16b
	stp	q0, q1, [x2], #0x20	/* store plaintext blocks */
	stp	q2, q3, [x2], #0x20
	stp	q4, q5, [x2], #0x20
	stp	q6, q7, [x2], #0x20
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	subs	x10, x10, #0x80		/* count down nbytes */
	b.ne	1b			/* repeat if more block groups */
	str	q31, [x4]		/* update tweak */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
	ret
END(aesarmv8_xts_dec8)

/*
 * aesarmv8_xts_mulx(tweak@q31)
 *
 *	Multiply q31 by x, modulo x^128 + x^7 + x^2 + x + 1, in place.
 *	Uses x0 and q0/q1 as temporaries.
 */
	.text
	_ALIGN_TEXT
	.type	aesarmv8_xts_mulx,@function
aesarmv8_xts_mulx:
	/*
	 * Simultaneously determine
	 * (a) whether the high bit of the low half must be
	 *     shifted into the low bit of the high half, and
	 * (b) whether the high bit of the high half must be
	 *     carried into x^128 = x^7 + x^2 + x + 1.
	 */
	adrl	x0, xtscarry
	cmlt	v1.2d, v31.2d, #0 /* v1.2d[i] := -1 if v31.2d[i] < 0, else 0 */
	ldr	q0, [x0]		/* q0 := xtscarry */
	ext	v1.16b, v1.16b, v1.16b, #8 /* swap halves of q1 */
	shl	v31.2d, v31.2d, #1	/* shift */
	and	v0.16b, v0.16b, v1.16b	/* copy xtscarry according to mask */
	eor	v31.16b, v31.16b, v0.16b /* incorporate (a) and (b) */
	ret
END(aesarmv8_xts_mulx)

	.section .rodata
	.p2align 4
	.type	xtscarry,@object
xtscarry:
	.byte	0x87,0,0,0, 0,0,0,0,  1,0,0,0, 0,0,0,0
END(xtscarry)

/*
 * aesarmv8_xts_update(const uint8_t in[16] @x0, uint8_t out[16] @x1)
 *
 *	Update an AES-XTS tweak.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_xts_update)
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	ldr	q31, [x0]		/* load tweak */
	bl	aesarmv8_xts_mulx	/* q31 *= x; trash x0/q0/q1 */
	str	q31, [x1]		/* store tweak */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
	ret
END(aesarmv8_xts_update)

/*
 * aesarmv8_cbcmac_update1(const struct aesenc *enckey@x0,
 *     const uint8_t *in@x1, size_t nbytes@x2, uint8_t auth[16] @x3,
 *     uint32_t nrounds@x4)
 *
 *	Update CBC-MAC.
 *
 *	nbytes must be a positive integral multiple of 16.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_cbcmac_update1)
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	ldr	q0, [x3]		/* q0 := initial authenticator */
	mov	x9, x0			/* x9 := enckey */
	mov	x5, x3			/* x5 := &auth (enc1 trashes x3) */
	_ALIGN_TEXT
1:	ldr	q1, [x1], #0x10		/* q1 := plaintext block */
	mov	x0, x9			/* x0 := enckey */
	mov	x3, x4			/* x3 := nrounds */
	eor	v0.16b, v0.16b, v1.16b	/* q0 := auth ^ ptxt */
	bl	aesarmv8_enc1		/* q0 := auth'; trash x0/x3/q16 */
	subs	x2, x2, #0x10		/* count down nbytes */
	b.ne	1b			/* repeat if x10 is nonzero */
	str	q0, [x5]		/* store updated authenticator */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
	ret
END(aesarmv8_cbcmac_update1)

/*
 * aesarmv8_ccm_enc1(const struct aesenc *enckey@x0, const uint8_t *in@x1,
 *     uint8_t *out@x2, size_t nbytes@x3, uint8_t authctr[32] @x4,
 *     uint32_t nrounds@x5)
 *
 *	Update CCM encryption.
 *
 *	nbytes must be a positive integral multiple of 16.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_ccm_enc1)
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	ldp	q0, q2, [x4]		/* q0 := auth, q2 := ctr (be) */
	adrl	x11, ctr32_inc		/* x11 := &ctr32_inc */
	ld1	{v5.4s}, [x11]		/* q5 := (0,0,0,1) (host-endian) */
	mov	x9, x0			/* x9 := enckey */
	mov	x10, x3			/* x10 := nbytes */
	rev32	v2.16b, v2.16b		/* q2 := ctr (host-endian) */
	_ALIGN_TEXT
1:	ldr	q3, [x1], #0x10		/* q3 := plaintext block */
	add	v2.4s, v2.4s, v5.4s	/* increment ctr (32-bit) */
	mov	x0, x9			/* x0 := enckey */
	mov	x3, x5			/* x3 := nrounds */
	rev32	v1.16b, v2.16b		/* q1 := ctr (big-endian) */
	eor	v0.16b, v0.16b, v3.16b	/* q0 := auth ^ ptxt */
	bl	aesarmv8_enc2		/* q0 := auth', q1 := pad;
					 * trash x0/x3/q16 */
	eor	v3.16b, v1.16b, v3.16b	/* q3 := ciphertext block */
	subs	x10, x10, #0x10		/* count down bytes */
	str	q3, [x2], #0x10		/* store ciphertext block */
	b.ne	1b			/* repeat if more blocks */
	rev32	v2.16b, v2.16b		/* q2 := ctr (big-endian) */
	stp	q0, q2, [x4]		/* store updated auth/ctr */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
	ret
END(aesarmv8_ccm_enc1)

/*
 * aesarmv8_ccm_dec1(const struct aesenc *enckey@x0, const uint8_t *in@x1,
 *     uint8_t *out@x2, size_t nbytes@x3, uint8_t authctr[32] @x4,
 *     uint32_t nrounds@x5)
 *
 *	Update CCM decryption.
 *
 *	nbytes must be a positive integral multiple of 16.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesarmv8_ccm_dec1)
	stp	fp, lr, [sp, #-16]!	/* push stack frame */
	mov	fp, sp
	ldp	q1, q2, [x4]		/* q1 := auth, q2 := ctr (be) */
	adrl	x11, ctr32_inc		/* x11 := &ctr32_inc */
	ld1	{v5.4s}, [x11]		/* q5 := (0,0,0,1) (host-endian) */
	mov	x9, x0			/* x9 := enckey */
	mov	x10, x3			/* x10 := nbytes */
	rev32	v2.16b, v2.16b		/* q2 := ctr (host-endian) */

	/* Decrypt the first block.  */
	add	v2.4s, v2.4s, v5.4s	/* increment ctr (32-bit) */
	mov	x3, x5			/* x3 := nrounds */
	rev32	v0.16b, v2.16b		/* q0 := ctr (big-endian) */
	ldr	q3, [x1], #0x10		/* q3 := ctxt */
	bl	aesarmv8_enc1		/* q0 := pad; trash x0/x3/q16 */
	b	2f

	_ALIGN_TEXT
1:	/*
	 * Authenticate the last block and decrypt the next block
	 * simultaneously.
	 *
	 *	q1 = auth ^ ptxt[-1]
	 *	q2 = ctr[-1] (le)
	 */
	add	v2.4s, v2.4s, v5.4s	/* increment ctr (32-bit) */
	mov	x0, x9			/* x0 := enckey */
	mov	x3, x5			/* x3 := nrounds */
	rev32	v0.16b, v2.16b		/* q0 := ctr (big-endian) */
	ldr	q3, [x1], #0x10		/* q3 := ctxt */
	bl	aesarmv8_enc2		/* q0 := pad, q1 := auth';
					 * trash x0/x3/q16 */
2:	eor	v3.16b, v0.16b, v3.16b	/* q3 := plaintext block */
	subs	x10, x10, #0x10
	str	q3, [x2], #0x10		/* store plaintext */
	eor	v1.16b, v1.16b, v3.16b	/* q1 := auth ^ ptxt */
	b.ne	1b

	rev32	v2.16b, v2.16b		/* q2 := ctr (big-endian) */

	/* Authenticate the last block.  */
	mov	x0, x9			/* x0 := enckey */
	mov	x3, x5			/* x3 := nrounds */
	mov	v0.16b, v1.16b		/* q0 := auth ^ ptxt */
	bl	aesarmv8_enc1		/* q0 := auth'; trash x0/x3/q16 */

	stp	q0, q2, [x4]		/* store updated auth/ctr */
	ldp	fp, lr, [sp], #16	/* pop stack frame */
	ret
END(aesarmv8_ccm_dec1)

	.section .rodata
	.p2align 4
	.type	ctr32_inc,@object
ctr32_inc:
	.int	0, 0, 0, 1
END(ctr32_inc)

/*
 * aesarmv8_enc1(const struct aesenc *enckey@x0,
 *     uint128_t block@q0, uint32_t nrounds@x3)
 *
 *	Encrypt a single AES block in q0.
 *
 *	Internal ABI.  Uses q16 as temporary.  Destroys x0 and x3.
 */
	.text
	_ALIGN_TEXT
	.type	aesarmv8_enc1,@function
aesarmv8_enc1:
	ldr	q16, [x0], #0x10	/* load round key */
	sub	x3, x3, #1
	_ALIGN_TEXT
1:	/* q0 := MixColumns(ShiftRows(SubBytes(AddRoundKey_q16(q0)))) */
	aese	v0.16b, v16.16b
	aesmc	v0.16b, v0.16b
	ldr	q16, [x0], #0x10
	subs	x3, x3, #1
	b.ne	1b
	/* q0 := ShiftRows(SubBytes(AddRoundKey_q16(q0))) */
	aese	v0.16b, v16.16b
	ldr	q16, [x0]		/* load last round key */
	/* q0 := AddRoundKey_q16(q0) */
	eor	v0.16b, v0.16b, v16.16b
	ret
END(aesarmv8_enc1)

/*
 * aesarmv8_enc2(const struct aesenc *enckey@x0,
 *     uint128_t block@q0, uint128_t block@q1, uint32_t nrounds@x3)
 *
 *	Encrypt two AES blocks in q0 and q1.
 *
 *	Internal ABI.  Uses q16 as temporary.  Destroys x0 and x3.
 */
	.text
	_ALIGN_TEXT
	.type	aesarmv8_enc2,@function
aesarmv8_enc2:
	ldr	q16, [x0], #0x10	/* load round key */
	sub	x3, x3, #1
	_ALIGN_TEXT
1:	/* q[i] := MixColumns(ShiftRows(SubBytes(AddRoundKey_q16(q[i])))) */
	aese	v0.16b, v16.16b
	aesmc	v0.16b, v0.16b
	aese	v1.16b, v16.16b
	aesmc	v1.16b, v1.16b
	ldr	q16, [x0], #0x10	/* load next round key */
	subs	x3, x3, #1
	b.ne	1b
	/* q[i] := ShiftRows(SubBytes(AddRoundKey_q16(q[i]))) */
	aese	v0.16b, v16.16b
	aese	v1.16b, v16.16b
	ldr	q16, [x0]		/* load last round key */
	/* q[i] := AddRoundKey_q16(q[i]) */
	eor	v0.16b, v0.16b, v16.16b
	eor	v1.16b, v1.16b, v16.16b
	ret
END(aesarmv8_enc2)

/*
 * aesarmv8_enc8(const struct aesenc *enckey@x0,
 *     uint128_t block0@q0, ..., uint128_t block7@q7,
 *     uint32_t nrounds@x3)
 *
 *	Encrypt eight AES blocks in q0 through q7 in parallel.
 *
 *	Internal ABI.  Uses q16 as temporary.  Destroys x0 and x3.
 */
	.text
	_ALIGN_TEXT
	.type	aesarmv8_enc8,@function
aesarmv8_enc8:
	ldr	q16, [x0], #0x10	/* load round key */
	sub	x3, x3, #1
	_ALIGN_TEXT
1:	/* q[i] := MixColumns(ShiftRows(SubBytes(AddRoundKey_q16(q[i])))) */
	aese	v0.16b, v16.16b
	aesmc	v0.16b, v0.16b
	aese	v1.16b, v16.16b
	aesmc	v1.16b, v1.16b
	aese	v2.16b, v16.16b
	aesmc	v2.16b, v2.16b
	aese	v3.16b, v16.16b
	aesmc	v3.16b, v3.16b
	aese	v4.16b, v16.16b
	aesmc	v4.16b, v4.16b
	aese	v5.16b, v16.16b
	aesmc	v5.16b, v5.16b
	aese	v6.16b, v16.16b
	aesmc	v6.16b, v6.16b
	aese	v7.16b, v16.16b
	aesmc	v7.16b, v7.16b
	ldr	q16, [x0], #0x10	/* load next round key */
	subs	x3, x3, #1
	b.ne	1b
	/* q[i] := ShiftRows(SubBytes(AddRoundKey_q16(q[i]))) */
	aese	v0.16b, v16.16b
	aese	v1.16b, v16.16b
	aese	v2.16b, v16.16b
	aese	v3.16b, v16.16b
	aese	v4.16b, v16.16b
	aese	v5.16b, v16.16b
	aese	v6.16b, v16.16b
	aese	v7.16b, v16.16b
	ldr	q16, [x0]		/* load last round key */
	/* q[i] := AddRoundKey_q16(q[i]) */
	eor	v0.16b, v0.16b, v16.16b
	eor	v1.16b, v1.16b, v16.16b
	eor	v2.16b, v2.16b, v16.16b
	eor	v3.16b, v3.16b, v16.16b
	eor	v4.16b, v4.16b, v16.16b
	eor	v5.16b, v5.16b, v16.16b
	eor	v6.16b, v6.16b, v16.16b
	eor	v7.16b, v7.16b, v16.16b
	ret
END(aesarmv8_enc8)

/*
 * aesarmv8_dec1(const struct aesdec *deckey@x0,
 *     uint128_t block@q0, uint32_t nrounds@x3)
 *
 *	Decrypt a single AES block in q0.
 *
 *	Internal ABI.  Uses q16 as temporary.  Destroys x0 and x3.
 */
	.text
	_ALIGN_TEXT
	.type	aesarmv8_dec1,@function
aesarmv8_dec1:
	ldr	q16, [x0], #0x10	/* load round key */
	sub	x3, x3, #1
	_ALIGN_TEXT
1:	/* q0 := InSubBytes(InShiftRows(AddRoundKey_q16(q0))) */
	aesd	v0.16b, v16.16b
	/* q0 := InMixColumns(q0) */
	aesimc	v0.16b, v0.16b
	ldr	q16, [x0], #0x10	/* load next round key */
	subs	x3, x3, #1
	b.ne	1b
	/* q0 := InSubBytes(InShiftRows(AddRoundKey_q16(q0))) */
	aesd	v0.16b, v16.16b
	ldr	q16, [x0]		/* load last round key */
	/* q0 := AddRoundKey_q16(q0) */
	eor	v0.16b, v0.16b, v16.16b
	ret
END(aesarmv8_dec1)

/*
 * aesarmv8_dec8(const struct aesdec *deckey@x0,
 *     uint128_t block0@q0, ..., uint128_t block7@q7,
 *     uint32_t nrounds@x3)
 *
 *	Decrypt eight AES blocks in q0 through q7 in parallel.
 *
 *	Internal ABI.  Uses q16 as temporary.  Destroys x0 and x3.
 */
	.text
	_ALIGN_TEXT
	.type	aesarmv8_dec8,@function
aesarmv8_dec8:
	ldr	q16, [x0], #0x10	/* load round key */
	sub	x3, x3, #1
	_ALIGN_TEXT
1:	/* q[i] := InSubBytes(InShiftRows(AddRoundKey_q16(q[i]))) */
	aesd	v0.16b, v16.16b
	/* q[i] := InMixColumns(q[i]) */
	aesimc	v0.16b, v0.16b
	aesd	v1.16b, v16.16b
	aesimc	v1.16b, v1.16b
	aesd	v2.16b, v16.16b
	aesimc	v2.16b, v2.16b
	aesd	v3.16b, v16.16b
	aesimc	v3.16b, v3.16b
	aesd	v4.16b, v16.16b
	aesimc	v4.16b, v4.16b
	aesd	v5.16b, v16.16b
	aesimc	v5.16b, v5.16b
	aesd	v6.16b, v16.16b
	aesimc	v6.16b, v6.16b
	aesd	v7.16b, v16.16b
	aesimc	v7.16b, v7.16b
	ldr	q16, [x0], #0x10	/* load next round key */
	subs	x3, x3, #1
	b.ne	1b
	/* q[i] := InSubBytes(InShiftRows(AddRoundKey_q16(q[i]))) */
	aesd	v0.16b, v16.16b
	aesd	v1.16b, v16.16b
	aesd	v2.16b, v16.16b
	aesd	v3.16b, v16.16b
	aesd	v4.16b, v16.16b
	aesd	v5.16b, v16.16b
	aesd	v6.16b, v16.16b
	aesd	v7.16b, v16.16b
	ldr	q16, [x0]		/* load last round key */
	/* q[i] := AddRoundKey_q16(q[i]) */
	eor	v0.16b, v0.16b, v16.16b
	eor	v1.16b, v1.16b, v16.16b
	eor	v2.16b, v2.16b, v16.16b
	eor	v3.16b, v3.16b, v16.16b
	eor	v4.16b, v4.16b, v16.16b
	eor	v5.16b, v5.16b, v16.16b
	eor	v6.16b, v6.16b, v16.16b
	eor	v7.16b, v7.16b, v16.16b
	ret
END(aesarmv8_dec8)
