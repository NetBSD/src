/*	$NetBSD: aes_ni_64.S,v 1.6 2020/07/27 20:57:23 riastradh Exp $	*/

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

#include <machine/asm.h>

RCSID("$NetBSD: aes_ni_64.S,v 1.6 2020/07/27 20:57:23 riastradh Exp $")

/*
 * MOVDQA/MOVDQU are Move Double Quadword (Aligned/Unaligned), defined
 * to operate on integers; MOVAPS/MOVUPS are Move (Aligned/Unaligned)
 * Packed Single, defined to operate on binary32 floats.  They have
 * exactly the same architectural effects (move a 128-bit quantity from
 * memory into an xmm register).
 *
 * In principle, they might have different microarchitectural effects
 * so that MOVAPS/MOVUPS might incur a penalty when the register is
 * later used for integer paths, but in practice they don't.  So we use
 * the one whose instruction encoding is shorter -- MOVAPS/MOVUPS.
 */
#define	movdqa	movaps
#define	movdqu	movups

/*
 * aesni_setenckey128(struct aesenc *enckey@rdi, const uint8_t key[16] @rsi)
 *
 *	Expand a 16-byte AES-128 key into 10 round keys.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_setenckey128)
	movdqu	(%rsi),%xmm0	/* load master key into %xmm0 */
	movdqa	%xmm0,(%rdi)	/* store master key as the first round key */
	lea	0x10(%rdi),%rdi	/* advance %rdi to next round key */
	aeskeygenassist $0x1,%xmm0,%xmm2
	call	aesni_expand128
	aeskeygenassist $0x2,%xmm0,%xmm2
	call	aesni_expand128
	aeskeygenassist $0x4,%xmm0,%xmm2
	call	aesni_expand128
	aeskeygenassist $0x8,%xmm0,%xmm2
	call	aesni_expand128
	aeskeygenassist $0x10,%xmm0,%xmm2
	call	aesni_expand128
	aeskeygenassist $0x20,%xmm0,%xmm2
	call	aesni_expand128
	aeskeygenassist $0x40,%xmm0,%xmm2
	call	aesni_expand128
	aeskeygenassist $0x80,%xmm0,%xmm2
	call	aesni_expand128
	aeskeygenassist $0x1b,%xmm0,%xmm2
	call	aesni_expand128
	aeskeygenassist $0x36,%xmm0,%xmm2
	call	aesni_expand128
	ret
END(aesni_setenckey128)

/*
 * aesni_setenckey192(struct aesenc *enckey@rdi, const uint8_t key[24] @rsi)
 *
 *	Expand a 24-byte AES-192 key into 12 round keys.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_setenckey192)
	movdqu	(%rsi),%xmm0	/* load master key [0:128) into %xmm0 */
	movq	0x10(%rsi),%xmm1 /* load master key [128:192) into %xmm1 */
	movdqa	%xmm0,(%rdi)	/* store master key [0:128) as round key */
	lea	0x10(%rdi),%rdi /* advance %rdi to next round key */
	aeskeygenassist $0x1,%xmm1,%xmm2
	call	aesni_expand192a
	aeskeygenassist $0x2,%xmm0,%xmm2
	call	aesni_expand192b
	aeskeygenassist $0x4,%xmm1,%xmm2
	call	aesni_expand192a
	aeskeygenassist $0x8,%xmm0,%xmm2
	call	aesni_expand192b
	aeskeygenassist $0x10,%xmm1,%xmm2
	call	aesni_expand192a
	aeskeygenassist $0x20,%xmm0,%xmm2
	call	aesni_expand192b
	aeskeygenassist $0x40,%xmm1,%xmm2
	call	aesni_expand192a
	aeskeygenassist $0x80,%xmm0,%xmm2
	call	aesni_expand192b
	ret
END(aesni_setenckey192)

/*
 * aesni_setenckey256(struct aesenc *enckey@rdi, const uint8_t key[32] @rsi)
 *
 *	Expand a 32-byte AES-256 key into 14 round keys.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_setenckey256)
	movdqu	(%rsi),%xmm0	/* load master key [0:128) into %xmm0 */
	movdqu	0x10(%rsi),%xmm1 /* load master key [128:256) into %xmm1 */
	movdqa	%xmm0,(%rdi)	/* store master key [0:128) as round key */
	movdqa	%xmm1,0x10(%rdi) /* store master key [128:256) as round key */
	lea	0x20(%rdi),%rdi	/* advance %rdi to next round key */
	aeskeygenassist $0x1,%xmm1,%xmm2
	call	aesni_expand256a
	aeskeygenassist $0x1,%xmm0,%xmm2
	call	aesni_expand256b
	aeskeygenassist $0x2,%xmm1,%xmm2
	call	aesni_expand256a
	aeskeygenassist $0x2,%xmm0,%xmm2
	call	aesni_expand256b
	aeskeygenassist $0x4,%xmm1,%xmm2
	call	aesni_expand256a
	aeskeygenassist $0x4,%xmm0,%xmm2
	call	aesni_expand256b
	aeskeygenassist $0x8,%xmm1,%xmm2
	call	aesni_expand256a
	aeskeygenassist $0x8,%xmm0,%xmm2
	call	aesni_expand256b
	aeskeygenassist $0x10,%xmm1,%xmm2
	call	aesni_expand256a
	aeskeygenassist $0x10,%xmm0,%xmm2
	call	aesni_expand256b
	aeskeygenassist $0x20,%xmm1,%xmm2
	call	aesni_expand256a
	aeskeygenassist $0x20,%xmm0,%xmm2
	call	aesni_expand256b
	aeskeygenassist $0x40,%xmm1,%xmm2
	call	aesni_expand256a
	ret
END(aesni_setenckey256)

/*
 * aesni_expand128(uint128_t *rkp@rdi, uint128_t prk@xmm0,
 *     uint128_t keygenassist@xmm2)
 *
 *	1. Compute the AES-128 round key using the previous round key.
 *	2. Store it at *rkp.
 *	3. Set %xmm0 to it.
 *	4. Advance %rdi to point at the next round key.
 *
 *	Internal ABI.  On entry:
 *
 *		%rdi = rkp, pointer to round key to compute
 *		%xmm0 = (prk[0], prk[1], prk[2], prk[3])
 *		%xmm2 = (xxx, xxx, xxx, t = Rot(SubWord(prk[3])) ^ RCON)
 *
 *	On exit:
 *
 *		%rdi = &rkp[1], rkp advanced by one round key
 *		%xmm0 = rk, the round key we just computed
 *		%xmm2 = garbage
 *		%xmm4 = garbage
 *		%xmm5 = garbage
 *		%xmm6 = garbage
 *
 *	Note: %xmm1 is preserved (as are %xmm3 and %xmm7 through %xmm15,
 *	and all other registers).
 */
	.text
	_ALIGN_TEXT
	.type	aesni_expand128,@function
aesni_expand128:
	/*
	 * %xmm2 := (%xmm2[3], %xmm2[3], %xmm2[3], %xmm2[3]),
	 * i.e., set each word of %xmm2 to t := Rot(SubWord(prk[3])) ^ RCON.
	 */
	pshufd	$0b11111111,%xmm2,%xmm2

	/*
	 * %xmm4 := (0, prk[0], prk[1], prk[2])
	 * %xmm5 := (0, 0, prk[0], prk[1])
	 * %xmm6 := (0, 0, 0, prk[0])
	 */
	movdqa	%xmm0,%xmm4
	movdqa	%xmm0,%xmm5
	movdqa	%xmm0,%xmm6
	pslldq	$4,%xmm4
	pslldq	$8,%xmm5
	pslldq	$12,%xmm6

	/*
	 * %xmm0 := (rk[0] = t ^ prk[0],
	 *     rk[1] = t ^ prk[0] ^ prk[1],
	 *     rk[2] = t ^ prk[0] ^ prk[1] ^ prk[2],
	 *     rk[3] = t ^ prk[0] ^ prk[1] ^ prk[2] ^ prk[3])
	 */
	pxor	%xmm2,%xmm0
	pxor	%xmm4,%xmm0
	pxor	%xmm5,%xmm0
	pxor	%xmm6,%xmm0

	movdqa	%xmm0,(%rdi)	/* store round key */
	lea	0x10(%rdi),%rdi	/* advance to next round key address */
	ret
END(aesni_expand128)

/*
 * aesni_expand192a(uint128_t *rkp@rdi, uint128_t prk@xmm0,
 *     uint64_t rklo@xmm1, uint128_t keygenassist@xmm2)
 *
 *	Set even-numbered AES-192 round key.
 *
 *	Internal ABI.  On entry:
 *
 *		%rdi = rkp, pointer to two round keys to compute
 *		%xmm0 = (prk[0], prk[1], prk[2], prk[3])
 *		%xmm1 = (rklo[0], rklo[1], xxx, xxx)
 *		%xmm2 = (xxx, t = Rot(SubWord(rklo[1])) ^ RCON, xxx, xxx)
 *
 *	On exit:
 *
 *		%rdi = &rkp[2], rkp advanced by two round keys
 *		%xmm0 = nrk, second round key we just computed
 *		%xmm1 = rk, first round key we just computed
 *		%xmm2 = garbage
 *		%xmm4 = garbage
 *		%xmm5 = garbage
 *		%xmm6 = garbage
 *		%xmm7 = garbage
 */
	.text
	_ALIGN_TEXT
	.type	aesni_expand192a,@function
aesni_expand192a:
	/*
	 * %xmm2 := (%xmm2[1], %xmm2[1], %xmm2[1], %xmm2[1]),
	 * i.e., set each word of %xmm2 to t := Rot(SubWord(rklo[1])) ^ RCON.
	 */
	pshufd	$0b01010101,%xmm2,%xmm2

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
	 * %xmm4 := (prk[0], prk[1], prk[2], prk[3])
	 * %xmm5 := (0, prk[0], prk[1], prk[2])
	 * %xmm6 := (0, 0, prk[0], prk[1])
	 * %xmm7 := (0, 0, 0, prk[0])
	 */
	movdqa	%xmm0,%xmm4
	movdqa	%xmm0,%xmm5
	movdqa	%xmm0,%xmm6
	movdqa	%xmm0,%xmm7
	pslldq	$4,%xmm5
	pslldq	$8,%xmm6
	pslldq	$12,%xmm7

	/* %xmm4 := (rk[2], rk[3], nrk[0], nrk[1]) */
	pxor	%xmm2,%xmm4
	pxor	%xmm5,%xmm4
	pxor	%xmm6,%xmm4
	pxor	%xmm7,%xmm4

	/*
	 * At this point, rk is split across %xmm1 (rk[0],rk[1],...) and
	 * %xmm4 (rk[2],rk[3],...); nrk is in %xmm4 (...,nrk[0],nrk[1]);
	 * and we have yet to compute nrk[2] or nrk[3], which requires
	 * rklo[0] and rklo[1] in %xmm1 (rklo[0], rklo[1], ...).  We need
	 * nrk to end up in %xmm0 at the end, so gather rk into %xmm1 and
	 * nrk into %xmm0.
	 */

	/* %xmm0 := (nrk[0], nrk[1], nrk[1], nrk[1]) */
	pshufd	$0b11111110,%xmm4,%xmm0

	/*
	 * %xmm6 := (0, 0, rklo[0], rklo[1])
	 * %xmm7 := (0, 0, 0, rklo[0])
	 */
	movdqa	%xmm1,%xmm6
	movdqa	%xmm1,%xmm7

	pslldq	$8,%xmm6
	pslldq	$12,%xmm7

	/*
	 * %xmm0 := (nrk[0],
	 *     nrk[1],
	 *     nrk[2] = nrk[1] ^ rklo[0],
	 *     nrk[3] = nrk[1] ^ rklo[0] ^ rklo[1])
	 */
	pxor	%xmm6,%xmm0
	pxor	%xmm7,%xmm0

	/* %xmm1 := (rk[0], rk[1], rk[2], rk[3]) */
	shufps	$0b01000100,%xmm4,%xmm1

	movdqa	%xmm1,(%rdi)		/* store round key */
	movdqa	%xmm0,0x10(%rdi)	/* store next round key */
	lea	0x20(%rdi),%rdi		/* advance two round keys */
	ret
END(aesni_expand192a)

/*
 * aesni_expand192b(uint128_t *roundkey@rdi, uint128_t prk@xmm0,
 *     uint128_t keygenassist@xmm2)
 *
 *	Set odd-numbered AES-192 round key.
 *
 *	Internal ABI.  On entry:
 *
 *		%rdi = rkp, pointer to round key to compute
 *		%xmm0 = (prk[0], prk[1], prk[2], prk[3])
 *		%xmm1 = (xxx, xxx, pprk[2], pprk[3])
 *		%xmm2 = (xxx, xxx, xxx, t = Rot(Sub(prk[3])) ^ RCON)
 *
 *	On exit:
 *
 *		%rdi = &rkp[1], rkp advanced by one round key
 *		%xmm0 = rk, the round key we just computed
 *		%xmm1 = (nrk[0], nrk[1], xxx, xxx), half of next round key
 *		%xmm2 = garbage
 *		%xmm4 = garbage
 *		%xmm5 = garbage
 *		%xmm6 = garbage
 *		%xmm7 = garbage
 */
	.text
	_ALIGN_TEXT
	.type	aesni_expand192b,@function
aesni_expand192b:
	/*
	 * %xmm2 := (%xmm2[3], %xmm2[3], %xmm2[3], %xmm2[3]),
	 * i.e., set each word of %xmm2 to t := Rot(Sub(prk[3])) ^ RCON.
	 */
	pshufd	$0b11111111,%xmm2,%xmm2

	/*
	 * We need to compute:
	 *
	 * rk[0] := Rot(Sub(prk[3])) ^ RCON ^ pprk[2]
	 * rk[1] := Rot(Sub(prk[3])) ^ RCON ^ pprk[2] ^ pprk[3]
	 * rk[2] := Rot(Sub(prk[3])) ^ RCON ^ pprk[2] ^ pprk[3] ^ prk[0]
	 * rk[3] := Rot(Sub(prk[3])) ^ RCON ^ pprk[2] ^ pprk[3] ^ prk[0]
	 *     ^ prk[1]
	 * nrk[0] := Rot(Sub(prk[3])) ^ RCON ^ pprk[2] ^ pprk[3] ^ prk[0]
	 *     ^ prk[1] ^ prk[2]
	 * nrk[1] := Rot(Sub(prk[3])) ^ RCON ^ pprk[2] ^ pprk[3] ^ prk[0]
	 *     ^ prk[1] ^ prk[2] ^ prk[3]
	 */

	/* %xmm1 := (pprk[2], pprk[3], prk[0], prk[1]) */
	shufps	$0b01001110,%xmm0,%xmm1

	/*
	 * %xmm5 := (0, pprk[2], pprk[3], prk[0])
	 * %xmm6 := (0, 0, pprk[2], pprk[3])
	 * %xmm7 := (0, 0, 0, pprk[2])
	 */
	movdqa	%xmm1,%xmm5
	movdqa	%xmm1,%xmm6
	movdqa	%xmm1,%xmm7
	pslldq	$4,%xmm5
	pslldq	$8,%xmm6
	pslldq	$12,%xmm7

	/* %xmm1 := (rk[0], rk[1], rk[2], rk[3) */
	pxor	%xmm2,%xmm1
	pxor	%xmm5,%xmm1
	pxor	%xmm6,%xmm1
	pxor	%xmm7,%xmm1

	/* %xmm4 := (prk[2], prk[3], xxx, xxx) */
	pshufd	$0b00001110,%xmm0,%xmm4

	/* %xmm5 := (0, prk[2], xxx, xxx) */
	movdqa	%xmm4,%xmm5
	pslldq	$4,%xmm5

	/* %xmm0 := (rk[0], rk[1], rk[2], rk[3]) */
	movdqa	%xmm1,%xmm0

	/* %xmm1 := (rk[3], rk[3], xxx, xxx) */
	shufps	$0b00001111,%xmm1,%xmm1

	/*
	 * %xmm1 := (nrk[0] = rk[3] ^ prk[2],
	 *     nrk[1] = rk[3] ^ prk[2] ^ prk[3],
	 *     xxx,
	 *     xxx)
	 */
	pxor	%xmm4,%xmm1
	pxor	%xmm5,%xmm1

	movdqa	%xmm0,(%rdi)	/* store round key */
	lea	0x10(%rdi),%rdi	/* advance to next round key address */
	ret
END(aesni_expand192b)

/*
 * aesni_expand256a(uint128_t *rkp@rdi, uint128_t pprk@xmm0,
 *     uint128_t prk@xmm1, uint128_t keygenassist@xmm2)
 *
 *	Set even-numbered AES-256 round key.
 *
 *	Internal ABI.  On entry:
 *
 *		%rdi = rkp, pointer to round key to compute
 *		%xmm0 = (pprk[0], pprk[1], pprk[2], pprk[3])
 *		%xmm1 = (prk[0], prk[1], prk[2], prk[3])
 *		%xmm2 = (xxx, xxx, xxx, t = Rot(SubWord(prk[3])))
 *
 *	On exit:
 *
 *		%rdi = &rkp[1], rkp advanced by one round key
 *		%xmm0 = rk, the round key we just computed
 *		%xmm1 = prk, previous round key, preserved from entry
 *		%xmm2 = garbage
 *		%xmm4 = garbage
 *		%xmm5 = garbage
 *		%xmm6 = garbage
 *
 *	The computation turns out to be the same as for AES-128; the
 *	previous round key does not figure into it, only the
 *	previous-previous round key.
 */
	aesni_expand256a = aesni_expand128

/*
 * aesni_expand256b(uint128_t *rkp@rdi, uint128_t prk@xmm0,
 *     uint128_t pprk@xmm1, uint128_t keygenassist@xmm2)
 *
 *	Set odd-numbered AES-256 round key.
 *
 *	Internal ABI.  On entry:
 *
 *		%rdi = rkp, pointer to round key to compute
 *		%xmm0 = (prk[0], prk[1], prk[2], prk[3])
 *		%xmm1 = (pprk[0], pprk[1], pprk[2], pprk[3])
 *		%xmm2 = (xxx, xxx, t = Sub(prk[3]), xxx)
 *
 *	On exit:
 *
 *		%rdi = &rkp[1], rkp advanced by one round key
 *		%xmm0 = prk, previous round key, preserved from entry
 *		%xmm1 = rk, the round key we just computed
 *		%xmm2 = garbage
 *		%xmm4 = garbage
 *		%xmm5 = garbage
 *		%xmm6 = garbage
 */
	.text
	_ALIGN_TEXT
	.type	aesni_expand256b,@function
aesni_expand256b:
	/*
	 * %xmm2 := (%xmm2[3], %xmm2[3], %xmm2[3], %xmm2[3]),
	 * i.e., set each word of %xmm2 to t := Sub(prk[3]).
	 */
	pshufd	$0b10101010,%xmm2,%xmm2

	/*
	 * %xmm4 := (0, pprk[0], pprk[1], pprk[2])
	 * %xmm5 := (0, 0, pprk[0], pprk[1])
	 * %xmm6 := (0, 0, 0, pprk[0])
	 */
	movdqa	%xmm1,%xmm4
	movdqa	%xmm1,%xmm5
	movdqa	%xmm1,%xmm6
	pslldq	$4,%xmm4
	pslldq	$8,%xmm5
	pslldq	$12,%xmm6

	/*
	 * %xmm0 := (rk[0] = t ^ pprk[0],
	 *     rk[1] = t ^ pprk[0] ^ pprk[1],
	 *     rk[2] = t ^ pprk[0] ^ pprk[1] ^ pprk[2],
	 *     rk[3] = t ^ pprk[0] ^ pprk[1] ^ pprk[2] ^ pprk[3])
	 */
	pxor	%xmm2,%xmm1
	pxor	%xmm4,%xmm1
	pxor	%xmm5,%xmm1
	pxor	%xmm6,%xmm1

	movdqa	%xmm1,(%rdi)	/* store round key */
	lea	0x10(%rdi),%rdi	/* advance to next round key address */
	ret
END(aesni_expand256b)

/*
 * aesni_enctodec(const struct aesenc *enckey@rdi, struct aesdec *deckey@rsi,
 *     uint32_t nrounds@rdx)
 *
 *	Convert AES encryption round keys to AES decryption round keys.
 *	`rounds' must be between 10 and 14.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_enctodec)
	shl	$4,%edx		/* rdx := byte offset of last round key */
	movdqa	(%rdi,%rdx),%xmm0	/* load last round key */
	movdqa	%xmm0,(%rsi)	/* store last round key verbatim */
	jmp	2f
	_ALIGN_TEXT
1:	movdqa	(%rdi,%rdx),%xmm0	/* load round key */
	aesimc	%xmm0,%xmm0	/* convert encryption to decryption */
	movdqa	%xmm0,(%rsi)	/* store round key */
2:	sub	$0x10,%rdx	/* advance to next round key */
	lea	0x10(%rsi),%rsi
	jnz	1b		/* repeat if more rounds */
	movdqa	(%rdi),%xmm0	/* load first round key */
	movdqa	%xmm0,(%rsi)	/* store first round key verbatim */
	ret
END(aesni_enctodec)

/*
 * aesni_enc(const struct aesenc *enckey@rdi, const uint8_t in[16] @rsi,
 *     uint8_t out[16] @rdx, uint32_t nrounds@ecx)
 *
 *	Encrypt a single block.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_enc)
	movdqu	(%rsi),%xmm0
	call	aesni_enc1
	movdqu	%xmm0,(%rdx)
	ret
END(aesni_enc)

/*
 * aesni_dec(const struct aesdec *deckey@rdi, const uint8_t in[16] @rsi,
 *     uint8_t out[16] @rdx, uint32_t nrounds@ecx)
 *
 *	Decrypt a single block.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_dec)
	movdqu	(%rsi),%xmm0
	call	aesni_dec1
	movdqu	%xmm0,(%rdx)
	ret
END(aesni_dec)

/*
 * aesni_cbc_enc(const struct aesenc *enckey@rdi, const uint8_t *in@rsi,
 *     uint8_t *out@rdx, size_t nbytes@rcx, uint8_t iv[16] @r8,
 *     uint32_t nrounds@r9d)
 *
 *	Encrypt a contiguous sequence of blocks with AES-CBC.
 *
 *	nbytes must be an integral multiple of 16.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_cbc_enc)
	cmp	$0,%rcx
	jz	2f
	mov	%rcx,%r10		/* r10 := nbytes */
	movdqu	(%r8),%xmm0		/* xmm0 := chaining value */
	_ALIGN_TEXT
1:	movdqu	(%rsi),%xmm1		/* xmm1 := plaintext block */
	lea	0x10(%rsi),%rsi
	pxor	%xmm1,%xmm0		/* xmm0 := cv ^ ptxt */
	mov	%r9d,%ecx		/* ecx := nrounds */
	call	aesni_enc1		/* xmm0 := ciphertext block */
	movdqu	%xmm0,(%rdx)
	lea	0x10(%rdx),%rdx
	sub	$0x10,%r10
	jnz	1b			/* repeat if r10 is nonzero */
	movdqu	%xmm0,(%r8)		/* store chaining value */
2:	ret
END(aesni_cbc_enc)

/*
 * aesni_cbc_dec1(const struct aesdec *deckey@rdi, const uint8_t *in@rsi,
 *     uint8_t *out@rdx, size_t nbytes@rcx, const uint8_t iv[16] @r8,
 *     uint32_t nrounds@r9)
 *
 *	Decrypt a contiguous sequence of blocks with AES-CBC.
 *
 *	nbytes must be a positive integral multiple of 16.  This routine
 *	is not vectorized; use aesni_cbc_dec8 for >=8 blocks at once.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_cbc_dec1)
	push	%rbp			/* create stack frame uint128[1] */
	mov	%rsp,%rbp
	sub	$0x10,%rsp
	movdqu	(%r8),%xmm8		/* xmm8 := iv */
	movdqa	%xmm8,(%rsp)		/* save iv */
	mov	%rcx,%r10		/* r10 := nbytes */
	movdqu	-0x10(%rsi,%r10),%xmm0	/* xmm0 := last ciphertext block */
	movdqu	%xmm0,(%r8)		/* update iv */
	jmp	2f
	_ALIGN_TEXT
1:	movdqu	-0x10(%rsi,%r10),%xmm8	/* xmm8 := chaining value */
	pxor	%xmm8,%xmm0		/* xmm0 := ptxt */
	movdqu	%xmm0,(%rdx,%r10)	/* store plaintext block */
	movdqa	%xmm8,%xmm0		/* move cv = ciphertext block */
2:	mov	%r9d,%ecx		/* ecx := nrounds */
	call	aesni_dec1		/* xmm0 := cv ^ ptxt */
	sub	$0x10,%r10
	jnz	1b			/* repeat if more blocks */
	pxor	(%rsp),%xmm0		/* xmm0 := ptxt */
	movdqu	%xmm0,(%rdx)		/* store first plaintext block */
	leave
	ret
END(aesni_cbc_dec1)

/*
 * aesni_cbc_dec8(const struct aesdec *deckey@rdi, const uint8_t *in@rsi,
 *     uint8_t *out@rdx, size_t nbytes@rcx, const uint8_t iv[16] @r8,
 *     uint32_t nrounds@r9)
 *
 *	Decrypt a contiguous sequence of 8-block units with AES-CBC.
 *
 *	nbytes must be a positive integral multiple of 128.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_cbc_dec8)
	push	%rbp			/* create stack frame uint128[1] */
	mov	%rsp,%rbp
	sub	$0x10,%rsp
	movdqu	(%r8),%xmm8		/* xmm8 := iv */
	movdqa	%xmm8,(%rsp)		/* save iv */
	mov	%rcx,%r10		/* r10 := nbytes */
	movdqu	-0x10(%rsi,%r10),%xmm7	/* xmm7 := ciphertext block[n-1] */
	movdqu	%xmm7,(%r8)		/* update iv */
	jmp	2f
	_ALIGN_TEXT
1:	movdqu	-0x10(%rsi,%r10),%xmm7	/* xmm7 := cv[0] */
	pxor	%xmm7,%xmm0		/* xmm0 := ptxt[0] */
	movdqu	%xmm0,(%rdx,%r10)	/* store plaintext block */
2:	movdqu	-0x20(%rsi,%r10),%xmm6	/* xmm6 := ciphertext block[n-2] */
	movdqu	-0x30(%rsi,%r10),%xmm5	/* xmm5 := ciphertext block[n-3] */
	movdqu	-0x40(%rsi,%r10),%xmm4	/* xmm4 := ciphertext block[n-4] */
	movdqu	-0x50(%rsi,%r10),%xmm3	/* xmm3 := ciphertext block[n-5] */
	movdqu	-0x60(%rsi,%r10),%xmm2	/* xmm2 := ciphertext block[n-6] */
	movdqu	-0x70(%rsi,%r10),%xmm1	/* xmm1 := ciphertext block[n-7] */
	movdqu	-0x80(%rsi,%r10),%xmm0	/* xmm0 := ciphertext block[n-8] */
	movdqa	%xmm6,%xmm15		/* xmm[8+i] := cv[i], 0<i<8 */
	movdqa	%xmm5,%xmm14
	movdqa	%xmm4,%xmm13
	movdqa	%xmm3,%xmm12
	movdqa	%xmm2,%xmm11
	movdqa	%xmm1,%xmm10
	movdqa	%xmm0,%xmm9
	mov	%r9d,%ecx		/* ecx := nrounds */
	call	aesni_dec8		/* xmm[i] := cv[i] ^ ptxt[i], 0<=i<8 */
	pxor	%xmm15,%xmm7		/* xmm[i] := ptxt[i], 0<i<8 */
	pxor	%xmm14,%xmm6
	pxor	%xmm13,%xmm5
	pxor	%xmm12,%xmm4
	pxor	%xmm11,%xmm3
	pxor	%xmm10,%xmm2
	pxor	%xmm9,%xmm1
	movdqu	%xmm7,-0x10(%rdx,%r10)	/* store plaintext blocks */
	movdqu	%xmm6,-0x20(%rdx,%r10)
	movdqu	%xmm5,-0x30(%rdx,%r10)
	movdqu	%xmm4,-0x40(%rdx,%r10)
	movdqu	%xmm3,-0x50(%rdx,%r10)
	movdqu	%xmm2,-0x60(%rdx,%r10)
	movdqu	%xmm1,-0x70(%rdx,%r10)
	sub	$0x80,%r10
	jnz	1b			/* repeat if more blocks */
	pxor	(%rsp),%xmm0		/* xmm0 := ptxt[0] */
	movdqu	%xmm0,(%rdx)		/* store first plaintext block */
	leave
	ret
END(aesni_cbc_dec8)

/*
 * aesni_xts_enc1(const struct aesenc *enckey@rdi, const uint8_t *in@rsi,
 *     uint8_t *out@rdx, size_t nbytes@rcx, uint8_t tweak[16] @r8,
 *     uint32_t nrounds@r9d)
 *
 *	Encrypt a contiguous sequence of blocks with AES-XTS.
 *
 *	nbytes must be a positive integral multiple of 16.  This routine
 *	is not vectorized; use aesni_xts_enc8 for >=8 blocks at once.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_xts_enc1)
	mov	%rcx,%r10		/* r10 := nbytes */
	movdqu	(%r8),%xmm15		/* xmm15 := tweak */
	_ALIGN_TEXT
1:	movdqu	(%rsi),%xmm0		/* xmm0 := ptxt */
	lea	0x10(%rsi),%rsi		/* advance rdi to next block */
	pxor	%xmm15,%xmm0		/* xmm0 := ptxt ^ tweak */
	mov	%r9d,%ecx		/* ecx := nrounds */
	call	aesni_enc1		/* xmm0 := AES(ptxt ^ tweak) */
	pxor	%xmm15,%xmm0		/* xmm0 := AES(ptxt ^ tweak) ^ tweak */
	movdqu	%xmm0,(%rdx)		/* store ciphertext block */
	lea	0x10(%rdx),%rdx		/* advance rsi to next block */
	call	aesni_xts_mulx		/* xmm15 *= x; trash xmm0 */
	sub	$0x10,%r10
	jnz	1b			/* repeat if more blocks */
	movdqu	%xmm15,(%r8)		/* update tweak */
	ret
END(aesni_xts_enc1)

/*
 * aesni_xts_enc8(const struct aesenc *enckey@rdi, const uint8_t *in@rsi,
 *     uint8_t *out@rdx, size_t nbytes@rcx, uint8_t tweak[16] @r8,
 *     uint32_t nrounds@r9d)
 *
 *	Encrypt a contiguous sequence of blocks with AES-XTS.
 *
 *	nbytes must be a positive integral multiple of 128.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_xts_enc8)
	push	%rbp			/* create stack frame uint128[1] */
	mov	%rsp,%rbp
	sub	$0x10,%rsp
	mov	%rcx,%r10		/* r10 := nbytes */
	movdqu	(%r8),%xmm15		/* xmm15 := tweak[0] */
	_ALIGN_TEXT
1:	movdqa	%xmm15,%xmm8		/* xmm8 := tweak[0] */
	call	aesni_xts_mulx		/* xmm15 := tweak[1] */
	movdqa	%xmm15,%xmm9		/* xmm9 := tweak[1] */
	call	aesni_xts_mulx		/* xmm15 := tweak[2] */
	movdqa	%xmm15,%xmm10		/* xmm10 := tweak[2] */
	call	aesni_xts_mulx		/* xmm15 := tweak[3] */
	movdqa	%xmm15,%xmm11		/* xmm11 := tweak[3] */
	call	aesni_xts_mulx		/* xmm15 := tweak[4] */
	movdqa	%xmm15,%xmm12		/* xmm12 := tweak[4] */
	call	aesni_xts_mulx		/* xmm15 := tweak[5] */
	movdqa	%xmm15,%xmm13		/* xmm13 := tweak[5] */
	call	aesni_xts_mulx		/* xmm15 := tweak[6] */
	movdqa	%xmm15,%xmm14		/* xmm14 := tweak[6] */
	call	aesni_xts_mulx		/* xmm15 := tweak[7] */
	movdqu	(%rsi),%xmm0		/* xmm[i] := ptxt[i] */
	movdqu	0x10(%rsi),%xmm1
	movdqu	0x20(%rsi),%xmm2
	movdqu	0x30(%rsi),%xmm3
	movdqu	0x40(%rsi),%xmm4
	movdqu	0x50(%rsi),%xmm5
	movdqu	0x60(%rsi),%xmm6
	movdqu	0x70(%rsi),%xmm7
	lea	0x80(%rsi),%rsi		/* advance rsi to next block group */
	movdqa	%xmm8,(%rsp)		/* save tweak[0] */
	pxor	%xmm8,%xmm0		/* xmm[i] := ptxt[i] ^ tweak[i] */
	pxor	%xmm9,%xmm1
	pxor	%xmm10,%xmm2
	pxor	%xmm11,%xmm3
	pxor	%xmm12,%xmm4
	pxor	%xmm13,%xmm5
	pxor	%xmm14,%xmm6
	pxor	%xmm15,%xmm7
	mov	%r9d,%ecx		/* ecx := nrounds */
	call	aesni_enc8		/* xmm[i] := AES(ptxt[i] ^ tweak[i]) */
	pxor	(%rsp),%xmm0		/* xmm[i] := AES(...) ^ tweak[i] */
	pxor	%xmm9,%xmm1
	pxor	%xmm10,%xmm2
	pxor	%xmm11,%xmm3
	pxor	%xmm12,%xmm4
	pxor	%xmm13,%xmm5
	pxor	%xmm14,%xmm6
	pxor	%xmm15,%xmm7
	movdqu	%xmm0,(%rdx)		/* store ciphertext blocks */
	movdqu	%xmm1,0x10(%rdx)
	movdqu	%xmm2,0x20(%rdx)
	movdqu	%xmm3,0x30(%rdx)
	movdqu	%xmm4,0x40(%rdx)
	movdqu	%xmm5,0x50(%rdx)
	movdqu	%xmm6,0x60(%rdx)
	movdqu	%xmm7,0x70(%rdx)
	lea	0x80(%rdx),%rdx		/* advance rdx to next block group */
	call	aesni_xts_mulx		/* xmm15 := tweak[8] */
	sub	$0x80,%r10
	jnz	1b			/* repeat if more block groups */
	movdqu	%xmm15,(%r8)		/* update tweak */
	leave
	ret
END(aesni_xts_enc8)

/*
 * aesni_xts_dec1(const struct aesdec *deckey@rdi, const uint8_t *in@rsi,
 *     uint8_t *out@rdx, size_t nbytes@rcx, uint8_t tweak[16] @r8,
 *     uint32_t nrounds@r9d)
 *
 *	Decrypt a contiguous sequence of blocks with AES-XTS.
 *
 *	nbytes must be a positive integral multiple of 16.  This routine
 *	is not vectorized; use aesni_xts_dec8 for >=8 blocks at once.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_xts_dec1)
	mov	%rcx,%r10		/* r10 := nbytes */
	movdqu	(%r8),%xmm15		/* xmm15 := tweak */
	_ALIGN_TEXT
1:	movdqu	(%rsi),%xmm0		/* xmm0 := ctxt */
	lea	0x10(%rsi),%rsi		/* advance rdi to next block */
	pxor	%xmm15,%xmm0		/* xmm0 := ctxt ^ tweak */
	mov	%r9d,%ecx		/* ecx := nrounds */
	call	aesni_dec1		/* xmm0 := AES(ctxt ^ tweak) */
	pxor	%xmm15,%xmm0		/* xmm0 := AES(ctxt ^ tweak) ^ tweak */
	movdqu	%xmm0,(%rdx)		/* store plaintext block */
	lea	0x10(%rdx),%rdx		/* advance rsi to next block */
	call	aesni_xts_mulx		/* xmm15 *= x; trash xmm0 */
	sub	$0x10,%r10
	jnz	1b			/* repeat if more blocks */
	movdqu	%xmm15,(%r8)		/* update tweak */
	ret
END(aesni_xts_dec1)

/*
 * aesni_xts_dec8(const struct aesdec *deckey@rdi, const uint8_t *in@rsi,
 *     uint8_t *out@rdx, size_t nbytes@rcx, uint8_t tweak[16] @r8,
 *     uint32_t nrounds@r9d)
 *
 *	Decrypt a contiguous sequence of blocks with AES-XTS.
 *
 *	nbytes must be a positive integral multiple of 128.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_xts_dec8)
	push	%rbp			/* create stack frame uint128[1] */
	mov	%rsp,%rbp
	sub	$0x10,%rsp
	mov	%rcx,%r10		/* r10 := nbytes */
	movdqu	(%r8),%xmm15		/* xmm15 := tweak[0] */
	_ALIGN_TEXT
1:	movdqa	%xmm15,%xmm8		/* xmm8 := tweak[0] */
	call	aesni_xts_mulx		/* xmm15 := tweak[1] */
	movdqa	%xmm15,%xmm9		/* xmm9 := tweak[1] */
	call	aesni_xts_mulx		/* xmm15 := tweak[2] */
	movdqa	%xmm15,%xmm10		/* xmm10 := tweak[2] */
	call	aesni_xts_mulx		/* xmm15 := tweak[3] */
	movdqa	%xmm15,%xmm11		/* xmm11 := tweak[3] */
	call	aesni_xts_mulx		/* xmm51 := tweak[4] */
	movdqa	%xmm15,%xmm12		/* xmm12 := tweak[4] */
	call	aesni_xts_mulx		/* xmm15 := tweak[5] */
	movdqa	%xmm15,%xmm13		/* xmm13 := tweak[5] */
	call	aesni_xts_mulx		/* xmm15 := tweak[6] */
	movdqa	%xmm15,%xmm14		/* xmm14 := tweak[6] */
	call	aesni_xts_mulx		/* xmm15 := tweak[7] */
	movdqu	(%rsi),%xmm0		/* xmm[i] := ptxt[i] */
	movdqu	0x10(%rsi),%xmm1
	movdqu	0x20(%rsi),%xmm2
	movdqu	0x30(%rsi),%xmm3
	movdqu	0x40(%rsi),%xmm4
	movdqu	0x50(%rsi),%xmm5
	movdqu	0x60(%rsi),%xmm6
	movdqu	0x70(%rsi),%xmm7
	lea	0x80(%rsi),%rsi		/* advance rsi to next block group */
	movdqa	%xmm8,(%rsp)		/* save tweak[0] */
	pxor	%xmm8,%xmm0		/* xmm[i] := ptxt[i] ^ tweak[i] */
	pxor	%xmm9,%xmm1
	pxor	%xmm10,%xmm2
	pxor	%xmm11,%xmm3
	pxor	%xmm12,%xmm4
	pxor	%xmm13,%xmm5
	pxor	%xmm14,%xmm6
	pxor	%xmm15,%xmm7
	mov	%r9d,%ecx		/* ecx := nrounds */
	call	aesni_dec8		/* xmm[i] := AES(ptxt[i] ^ tweak[i]) */
	pxor	(%rsp),%xmm0		/* xmm[i] := AES(...) ^ tweak[i] */
	pxor	%xmm9,%xmm1
	pxor	%xmm10,%xmm2
	pxor	%xmm11,%xmm3
	pxor	%xmm12,%xmm4
	pxor	%xmm13,%xmm5
	pxor	%xmm14,%xmm6
	pxor	%xmm15,%xmm7
	movdqu	%xmm0,(%rdx)		/* store ciphertext blocks */
	movdqu	%xmm1,0x10(%rdx)
	movdqu	%xmm2,0x20(%rdx)
	movdqu	%xmm3,0x30(%rdx)
	movdqu	%xmm4,0x40(%rdx)
	movdqu	%xmm5,0x50(%rdx)
	movdqu	%xmm6,0x60(%rdx)
	movdqu	%xmm7,0x70(%rdx)
	lea	0x80(%rdx),%rdx		/* advance rdx to next block group */
	call	aesni_xts_mulx		/* xmm15 := tweak[8] */
	sub	$0x80,%r10
	jnz	1b			/* repeat if more block groups */
	movdqu	%xmm15,(%r8)		/* update tweak */
	leave
	ret
END(aesni_xts_dec8)

/*
 * aesni_xts_mulx(tweak@xmm15)
 *
 *	Multiply xmm15 by x, modulo x^128 + x^7 + x^2 + x + 1, in place.
 *	Uses %xmm0 as temporary.
 */
	.text
	_ALIGN_TEXT
	.type	aesni_xts_mulx,@function
aesni_xts_mulx:
	/*
	 * Simultaneously determine
	 * (a) whether the high bit of the low quadword must be
	 *     shifted into the low bit of the high quadword, and
	 * (b) whether the high bit of the high quadword must be
	 *     carried into x^128 = x^7 + x^2 + x + 1.
	 */
	pxor	%xmm0,%xmm0	/* xmm0 := 0 */
	pcmpgtq	%xmm15,%xmm0	/* xmm0[i] := -1 if 0 > xmm15[i] else 0 */
	pshufd	$0b01001110,%xmm0,%xmm0	/* swap halves of xmm0 */
	pand	xtscarry(%rip),%xmm0	/* copy xtscarry according to mask */
	psllq	$1,%xmm15	/* shift */
	pxor	%xmm0,%xmm15	/* incorporate (a) and (b) */
	ret
END(aesni_xts_mulx)

	.section .rodata
	.p2align 4
	.type	xtscarry,@object
xtscarry:
	.byte	0x87,0,0,0, 0,0,0,0,  1,0,0,0, 0,0,0,0
END(xtscarry)

/*
 * aesni_xts_update(const uint8_t in[16] @rdi, uint8_t out[16] @rsi)
 *
 *	Update an AES-XTS tweak.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_xts_update)
	movdqu	(%rdi),%xmm15
	call	aesni_xts_mulx
	movdqu	%xmm15,(%rsi)
	ret
END(aesni_xts_update)

/*
 * aesni_cbcmac_update1(const struct aesenc *enckey@rdi, const uint8_t *in@rsi,
 *     size_t nbytes@rdx, uint8_t auth[16] @rcx, uint32_t nrounds@r8d)
 *
 *	Update CBC-MAC.
 *
 *	nbytes must be a positive integral multiple of 16.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_cbcmac_update1)
	movdqu	(%rcx),%xmm0		/* xmm0 := auth */
	mov	%rdx,%r10		/* r10 := nbytes */
	mov	%rcx,%rdx		/* rdx := &auth */
	_ALIGN_TEXT
1:	pxor	(%rsi),%xmm0		/* xmm0 ^= plaintext block */
	lea	0x10(%rsi),%rsi
	mov	%r8d,%ecx		/* ecx := nrounds */
	call	aesni_enc1		/* xmm0 := auth'; trash rax,rcx,xmm8 */
	sub	$0x10,%r10
	jnz	1b
	movdqu	%xmm0,(%rdx)		/* store auth' */
	ret
END(aesni_cbcmac_update1)

/*
 * aesni_ccm_enc1(const struct aesenc *enckey@rdi, const uint8_t *in@rsi,
 *     uint8_t *out@rdx, size_t nbytes@rcx,
 *     uint8_t authctr[32] @r8, uint32_t nrounds@r9d)
 *
 *	Update CCM encryption.
 *
 *	nbytes must be a positive integral multiple of 16.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_ccm_enc1)
	mov	%rcx,%r10		/* r10 := nbytes */
	movdqu	0x10(%r8),%xmm2		/* xmm2 := ctr (be) */
	movdqa	bswap32(%rip),%xmm4	/* xmm4 := bswap32 table */
	movdqa	ctr32_inc(%rip),%xmm5	/* xmm5 := (0,0,0,1) (le) */
	movdqu	(%r8),%xmm0		/* xmm0 := auth */
	pshufb	%xmm4,%xmm2		/* xmm2 := ctr (le) */
	_ALIGN_TEXT
1:	movdqu	(%rsi),%xmm3		/* xmm3 := plaintext block */
	paddd	%xmm5,%xmm2		/* increment ctr (32-bit) */
	lea	0x10(%rsi),%rsi
	movdqa	%xmm2,%xmm1		/* xmm1 := ctr (le) */
	mov	%r9d,%ecx		/* ecx := nrounds */
	pshufb	%xmm4,%xmm1		/* xmm1 := ctr (be) */
	pxor	%xmm3,%xmm0		/* xmm0 := auth ^ ptxt */
	call	aesni_enc2		/* trash rax/rcx/xmm8 */
	pxor	%xmm1,%xmm3		/* xmm3 := ciphertext block */
	sub	$0x10,%r10		/* count down bytes */
	movdqu	%xmm3,(%rdx)		/* store ciphertext block */
	lea	0x10(%rdx),%rdx
	jnz	1b			/* repeat if more blocks */
	pshufb	%xmm4,%xmm2		/* xmm2 := ctr (be) */
	movdqu	%xmm0,(%r8)		/* store updated auth */
	movdqu	%xmm2,0x10(%r8)		/* store updated ctr */
	ret
END(aesni_ccm_enc1)

/*
 * aesni_ccm_dec1(const struct aesenc *enckey@rdi, const uint8_t *in@rsi,
 *     uint8_t *out@rdx, size_t nbytes@rcx,
 *     uint8_t authctr[32] @r8, uint32_t nrounds@r9d)
 *
 *	Update CCM decryption.
 *
 *	nbytes must be a positive integral multiple of 16.
 *
 *	Standard ABI calling convention.
 */
ENTRY(aesni_ccm_dec1)
	movdqu	0x10(%r8),%xmm2		/* xmm2 := ctr (be) */
	movdqa	bswap32(%rip),%xmm4	/* xmm4 := bswap32 table */
	movdqa	ctr32_inc(%rip),%xmm5	/* xmm5 := (0,0,0,1) (le) */
	movdqu	(%r8),%xmm1		/* xmm1 := auth */
	pshufb	%xmm4,%xmm2		/* xmm2 := ctr (le) */
	mov	%rcx,%r10		/* r10 := nbytes */

	/* Decrypt the first block.  */
	paddd	%xmm5,%xmm2		/* increment ctr (32-bit) */
	mov	%r9d,%ecx		/* ecx := nrounds */
	movdqa	%xmm2,%xmm0		/* xmm0 := ctr (le) */
	movdqu	(%rsi),%xmm3		/* xmm3 := ctxt */
	pshufb	%xmm4,%xmm0		/* xmm0 := ctr (be) */
	lea	0x10(%rsi),%rsi
	call	aesni_enc1		/* xmm0 := pad; trash rax/rcx/xmm8 */
	jmp	2f

	_ALIGN_TEXT
1:	/*
	 * Authenticate the last block and decrypt the next block
	 * simultaneously.
	 *
	 *	xmm1 = auth ^ ptxt[-1]
	 *	xmm2 = ctr[-1] (le)
	 */
	paddd	%xmm5,%xmm2		/* increment ctr (32-bit) */
	mov	%r9d,%ecx		/* ecx := nrounds */
	movdqa	%xmm2,%xmm0		/* xmm0 := ctr (le) */
	movdqu	(%rsi),%xmm3		/* xmm3 := ctxt */
	pshufb	%xmm4,%xmm0		/* xmm0 := ctr (be) */
	lea	0x10(%rsi),%rsi
	call	aesni_enc2		/* xmm0 := pad, xmm1 := auth';
					 * trash rax/rcx/xmm8 */
2:	pxor	%xmm0,%xmm3		/* xmm3 := ptxt */
	sub	$0x10,%r10
	movdqu	%xmm3,(%rdx)		/* store plaintext */
	lea	0x10(%rdx),%rdx
	pxor	%xmm3,%xmm1		/* xmm1 := auth ^ ptxt */
	jnz	1b

	/* Authenticate the last block.  */
	movdqa	%xmm1,%xmm0		/* xmm0 := auth ^ ptxt */
	mov	%r9d,%ecx		/* ecx := nrounds */
	call	aesni_enc1		/* xmm0 := auth' */
	pshufb	%xmm4,%xmm2		/* xmm2 := ctr (be) */
	movdqu	%xmm0,(%r8)		/* store updated auth */
	movdqu	%xmm2,0x10(%r8)		/* store updated ctr */
	ret
END(aesni_ccm_dec1)

	.section .rodata
	.p2align 4
	.type	bswap32,@object
bswap32:
	.byte	3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12
END(bswap32)

	.section .rodata
	.p2align 4
	.type	ctr32_inc,@object
ctr32_inc:
	.byte	0,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,0,0
END(ctr32_inc)

/*
 * aesni_enc1(const struct aesenc *enckey@rdi, uint128_t block@xmm0,
 *     uint32_t nrounds@ecx)
 *
 *	Encrypt a single AES block in %xmm0.
 *
 *	Internal ABI.  Uses %rax and %xmm8 as temporaries.  Destroys %ecx.
 */
	.text
	_ALIGN_TEXT
	.type	aesni_enc1,@function
aesni_enc1:
	pxor	(%rdi),%xmm0	/* xor in first round key */
	shl	$4,%ecx		/* ecx := total byte size of round keys */
	lea	0x10(%rdi,%rcx),%rax	/* rax := end of round key array */
	neg	%rcx		/* rcx := byte offset of round key from end */
	jmp	2f
	_ALIGN_TEXT
1:	aesenc	%xmm8,%xmm0
2:	movdqa	(%rax,%rcx),%xmm8	/* load round key */
	add	$0x10,%rcx
	jnz	1b		/* repeat if more rounds */
	aesenclast %xmm8,%xmm0
	ret
END(aesni_enc1)

/*
 * aesni_enc2(const struct aesenc *enckey@rdi, uint128_t block0@xmm0,
 *     uint128_t block1@xmm1, uint32_t nrounds@ecx)
 *
 *	Encrypt two AES blocks in %xmm0 and %xmm1.
 *
 *	Internal ABI.  Uses %rax and %xmm8 as temporaries.  Destroys %ecx.
 */
	.text
	_ALIGN_TEXT
	.type	aesni_enc2,@function
aesni_enc2:
	movdqa	(%rdi),%xmm8	/* xmm8 := first round key */
	shl	$4,%ecx		/* ecx := total byte size of round keys */
	lea	0x10(%rdi,%rcx),%rax	/* rax := end of round key array */
	neg	%rcx		/* rcx := byte offset of round key from end */
	pxor	%xmm8,%xmm0	/* xor in first round key */
	pxor	%xmm8,%xmm1
	jmp	2f
	_ALIGN_TEXT
1:	aesenc	%xmm8,%xmm0
	aesenc	%xmm8,%xmm1
2:	movdqa	(%rax,%rcx),%xmm8	/* load round key */
	add	$0x10,%rcx
	jnz	1b		/* repeat if there's more */
	aesenclast %xmm8,%xmm0
	aesenclast %xmm8,%xmm1
	ret
END(aesni_enc2)

/*
 * aesni_enc8(const struct aesenc *enckey@rdi, uint128_t block0@xmm0, ...,
 *     block7@xmm7, uint32_t nrounds@ecx)
 *
 *	Encrypt eight AES blocks in %xmm0 through %xmm7 in parallel.
 *
 *	Internal ABI.  Uses %rax and %xmm8 as temporaries.  Destroys %ecx.
 */
	.text
	_ALIGN_TEXT
	.type	aesni_enc8,@function
aesni_enc8:
	movdqa	(%rdi),%xmm8	/* xor in first round key */
	pxor	%xmm8,%xmm0
	pxor	%xmm8,%xmm1
	pxor	%xmm8,%xmm2
	pxor	%xmm8,%xmm3
	pxor	%xmm8,%xmm4
	pxor	%xmm8,%xmm5
	pxor	%xmm8,%xmm6
	pxor	%xmm8,%xmm7
	shl	$4,%ecx		/* ecx := total byte size of round keys */
	lea	0x10(%rdi,%rcx),%rax	/* rax := end of round key array */
	neg	%rcx		/* rcx := byte offset of round key from end */
	jmp	2f
	_ALIGN_TEXT
1:	aesenc	%xmm8,%xmm0
	aesenc	%xmm8,%xmm1
	aesenc	%xmm8,%xmm2
	aesenc	%xmm8,%xmm3
	aesenc	%xmm8,%xmm4
	aesenc	%xmm8,%xmm5
	aesenc	%xmm8,%xmm6
	aesenc	%xmm8,%xmm7
2:	movdqa	(%rax,%rcx),%xmm8	/* load round key */
	add	$0x10,%rcx
	jnz	1b		/* repeat if more rounds */
	aesenclast %xmm8,%xmm0
	aesenclast %xmm8,%xmm1
	aesenclast %xmm8,%xmm2
	aesenclast %xmm8,%xmm3
	aesenclast %xmm8,%xmm4
	aesenclast %xmm8,%xmm5
	aesenclast %xmm8,%xmm6
	aesenclast %xmm8,%xmm7
	ret
END(aesni_enc8)

/*
 * aesni_dec1(const struct aesdec *deckey@rdi, uint128_t block@xmm0,
 *     uint32_t nrounds@ecx)
 *
 *	Decrypt a single AES block in %xmm0.
 *
 *	Internal ABI.  Uses %rax and %xmm8 as temporaries.  Destroys %ecx.
 */
	.text
	_ALIGN_TEXT
	.type	aesni_dec1,@function
aesni_dec1:
	pxor	(%rdi),%xmm0	/* xor in first round key */
	shl	$4,%ecx		/* ecx := byte offset of round key */
	lea	0x10(%rdi,%rcx),%rax	/* rax := pointer to round key */
	neg	%rcx		/* rcx := byte offset of round key from end */
	jmp	2f
	_ALIGN_TEXT
1:	aesdec	%xmm8,%xmm0
2:	movdqa	(%rax,%rcx),%xmm8	/* load round key */
	add	$0x10,%rcx
	jnz	1b		/* repeat if more rounds */
	aesdeclast %xmm8,%xmm0
	ret
END(aesni_dec1)

/*
 * aesni_dec8(const struct aesdec *deckey@rdi, uint128_t block0@xmm0, ...,
 *     block7@xmm7, uint32_t nrounds@ecx)
 *
 *	Decrypt eight AES blocks in %xmm0 through %xmm7 in parallel.
 *
 *	Internal ABI.  Uses %xmm8 as temporary.  Destroys %rcx.
 */
	.text
	_ALIGN_TEXT
	.type	aesni_dec8,@function
aesni_dec8:
	movdqa	(%rdi),%xmm8	/* xor in first round key */
	pxor	%xmm8,%xmm0
	pxor	%xmm8,%xmm1
	pxor	%xmm8,%xmm2
	pxor	%xmm8,%xmm3
	pxor	%xmm8,%xmm4
	pxor	%xmm8,%xmm5
	pxor	%xmm8,%xmm6
	pxor	%xmm8,%xmm7
	shl	$4,%ecx		/* ecx := byte offset of round key */
	lea	0x10(%rdi,%rcx),%rax	/* rax := pointer to round key */
	neg	%rcx		/* rcx := byte offset of round key from end */
	jmp	2f
	_ALIGN_TEXT
1:	aesdec	%xmm8,%xmm0
	aesdec	%xmm8,%xmm1
	aesdec	%xmm8,%xmm2
	aesdec	%xmm8,%xmm3
	aesdec	%xmm8,%xmm4
	aesdec	%xmm8,%xmm5
	aesdec	%xmm8,%xmm6
	aesdec	%xmm8,%xmm7
2:	movdqa	(%rax,%rcx),%xmm8	/* load round key */
	add	$0x10,%rcx
	jnz	1b		/* repeat if more rounds */
	aesdeclast %xmm8,%xmm0
	aesdeclast %xmm8,%xmm1
	aesdeclast %xmm8,%xmm2
	aesdeclast %xmm8,%xmm3
	aesdeclast %xmm8,%xmm4
	aesdeclast %xmm8,%xmm5
	aesdeclast %xmm8,%xmm6
	aesdeclast %xmm8,%xmm7
	ret
END(aesni_dec8)
