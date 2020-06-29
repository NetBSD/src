/*	$NetBSD: aes_sse2_impl.h,v 1.2 2020/06/29 23:50:05 riastradh Exp $	*/

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

#ifndef	_CRYPTO_AES_ARCH_X86_AES_SSE2_IMPL_H
#define	_CRYPTO_AES_ARCH_X86_AES_SSE2_IMPL_H

#include <sys/types.h>

#include <crypto/aes/aes.h>
#include <crypto/aes/arch/x86/aes_sse2.h>
#include <crypto/aes/arch/x86/immintrin.h>
#include <crypto/aes/arch/x86/immintrin_ext.h>

void aes_sse2_bitslice_Sbox(__m128i[static 4]);
void aes_sse2_bitslice_invSbox(__m128i[static 4]);
void aes_sse2_ortho(__m128i[static 4]);
__m128i aes_sse2_interleave_in(__m128i);
__m128i aes_sse2_interleave_out(__m128i);
unsigned aes_sse2_keysched(uint64_t *, const void *, size_t);
void aes_sse2_skey_expand(uint64_t *, unsigned, const uint64_t *);
void aes_sse2_bitslice_encrypt(unsigned, const uint64_t *, __m128i[static 4]);
void aes_sse2_bitslice_decrypt(unsigned, const uint64_t *, __m128i[static 4]);

#endif	/* _CRYPTO_AES_ARCH_X86_AES_SSE2_IMPL_H */
