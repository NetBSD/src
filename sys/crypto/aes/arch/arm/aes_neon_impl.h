/*	$NetBSD: aes_neon_impl.h,v 1.3 2020/08/08 14:47:01 riastradh Exp $	*/

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

#ifndef	_CRYPTO_AES_ARCH_ARM_AES_NEON_IMPL_H
#define	_CRYPTO_AES_ARCH_ARM_AES_NEON_IMPL_H

#include <sys/types.h>

#include "arm_neon.h"
#include "arm_neon_imm.h"

#include <crypto/aes/aes.h>
#include <crypto/aes/arch/arm/aes_neon.h>

uint8x16_t aes_neon_enc1(const struct aesenc *, uint8x16_t, unsigned);
uint8x16_t aes_neon_dec1(const struct aesdec *, uint8x16_t, unsigned);

#ifdef __aarch64__

uint8x16x2_t aes_neon_enc2(const struct aesenc *, uint8x16x2_t, unsigned);
uint8x16x2_t aes_neon_dec2(const struct aesdec *, uint8x16x2_t, unsigned);

#else

static inline uint8x16x2_t
aes_neon_enc2(const struct aesenc *enc, uint8x16x2_t b2, unsigned nrounds)
{

	return (uint8x16x2_t) { .val = {
		[0] = aes_neon_enc1(enc, b2.val[0], nrounds),
		[1] = aes_neon_enc1(enc, b2.val[1], nrounds),
	} };
}

static inline uint8x16x2_t
aes_neon_dec2(const struct aesdec *dec, uint8x16x2_t b2, unsigned nrounds)
{

	return (uint8x16x2_t) { .val = {
		[0] = aes_neon_dec1(dec, b2.val[0], nrounds),
		[1] = aes_neon_dec1(dec, b2.val[1], nrounds),
	} };
}

#endif

#endif	/* _CRYPTO_AES_ARCH_ARM_AES_NEON_IMPL_H */
