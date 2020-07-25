/*	$NetBSD: aes_sse2.h,v 1.4 2020/07/25 22:29:56 riastradh Exp $	*/

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

#ifndef	_CRYPTO_AES_ARCH_X86_AES_SSE2_H
#define	_CRYPTO_AES_ARCH_X86_AES_SSE2_H

#include <sys/types.h>

#include <crypto/aes/aes_impl.h>

struct aesenc;
struct aesdec;

/*
 * These functions MUST NOT use any vector registers for parameters or
 * results -- the caller is compiled with -mno-sse &c. in the kernel,
 * and dynamically turns on the vector unit just before calling them.
 * Internal subroutines that use the vector unit for parameters are
 * declared in aes_sse2_impl.h instead.
 */

void aes_sse2_setkey(uint64_t[static 30], const void *, uint32_t);

void aes_sse2_enc(const struct aesenc *, const uint8_t in[static 16],
    uint8_t[static 16], uint32_t);
void aes_sse2_dec(const struct aesdec *, const uint8_t in[static 16],
    uint8_t[static 16], uint32_t);
void aes_sse2_cbc_enc(const struct aesenc *, const uint8_t[static 16],
    uint8_t[static 16], size_t nbytes, uint8_t[static 16], uint32_t);
void aes_sse2_cbc_dec(const struct aesdec *, const uint8_t[static 16],
    uint8_t[static 16], size_t nbytes, uint8_t[static 16], uint32_t);
void aes_sse2_xts_enc(const struct aesenc *, const uint8_t[static 16],
    uint8_t[static 16], size_t nbytes, uint8_t[static 16], uint32_t);
void aes_sse2_xts_dec(const struct aesdec *, const uint8_t[static 16],
    uint8_t[static 16], size_t nbytes, uint8_t[static 16], uint32_t);
void aes_sse2_cbcmac_update1(const struct aesenc *, const uint8_t[static 16],
    size_t, uint8_t[static 16], uint32_t);
void aes_sse2_ccm_enc1(const struct aesenc *, const uint8_t[static 16],
    uint8_t[static 16], size_t, uint8_t[static 32], uint32_t);
void aes_sse2_ccm_dec1(const struct aesenc *, const uint8_t[static 16],
    uint8_t[static 16], size_t, uint8_t[static 32], uint32_t);

int aes_sse2_selftest(void);

extern struct aes_impl aes_sse2_impl;

#endif	/* _CRYPTO_AES_ARCH_X86_AES_SSE2_H */
