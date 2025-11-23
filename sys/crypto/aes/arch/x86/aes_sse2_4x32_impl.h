/*	$NetBSD: aes_sse2_4x32_impl.h,v 1.1 2025/11/23 22:48:26 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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

#ifndef	_CRYPTO_AES_ARCH_X86_AES_SSE2_4X32_IMPL_H
#define	_CRYPTO_AES_ARCH_X86_AES_SSE2_4X32_IMPL_H

#include <sys/types.h>

#include <crypto/aes/aes.h>
#include <crypto/arch/x86/immintrin.h>
#include <crypto/arch/x86/immintrin_ext.h>

#include "aes_sse2_4x32_subr.h"

#define	br_dec32le	le32dec
#define	br_enc32le	le32enc

void aes_sse2_4x32_bitslice_Sbox(__m128i[static 8]);
void aes_sse2_4x32_bitslice_invSbox(__m128i[static 8]);
void aes_sse2_4x32_ortho(__m128i[static 8]);
unsigned aes_sse2_4x32_keysched(uint32_t[static 60], const void *, size_t);
void aes_sse2_4x32_skey_expand(uint32_t[static 120], unsigned,
    const uint32_t[static 60]);
void aes_sse2_4x32_bitslice_encrypt(unsigned, const uint32_t[static 120],
    __m128i[static 8]);
void aes_sse2_4x32_bitslice_decrypt(unsigned, const uint32_t[static 120],
    __m128i[static 8]);

#endif	/* _CRYPTO_AES_ARCH_X86_AES_SSE2_4X32_IMPL_H */
