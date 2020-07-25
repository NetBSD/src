/*	$NetBSD: aes_ssse3.h,v 1.3 2020/07/25 22:31:04 riastradh Exp $	*/

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

#ifndef	_CRYPTO_AES_ARCH_X86_AES_SSSE3_H
#define	_CRYPTO_AES_ARCH_X86_AES_SSSE3_H

#include <sys/types.h>

#include <crypto/aes/aes_impl.h>

struct aesenc;
struct aesdec;

/*
 * These functions MUST NOT use any vector registers for parameters or
 * results -- the caller is compiled with -mno-sse &c. in the kernel,
 * and dynamically turns on the vector unit just before calling them.
 * Internal subroutines that use the vector unit for parameters are
 * declared in aes_ssse3_impl.h instead.
 */

void aes_ssse3_setenckey(struct aesenc *, const uint8_t *, unsigned);
void aes_ssse3_setdeckey(struct aesdec *, const uint8_t *, unsigned);

void aes_ssse3_enc(const struct aesenc *, const uint8_t[static 16],
    uint8_t[static 16], uint32_t);
void aes_ssse3_dec(const struct aesdec *, const uint8_t[static 16],
    uint8_t[static 16], uint32_t);
void aes_ssse3_cbc_enc(const struct aesenc *, const uint8_t[static 16],
    uint8_t[static 16], size_t, uint8_t[static 16], uint32_t);
void aes_ssse3_cbc_dec(const struct aesdec *, const uint8_t[static 16],
    uint8_t[static 16], size_t, uint8_t[static 16], uint32_t);
void aes_ssse3_xts_enc(const struct aesenc *, const uint8_t[static 16],
    uint8_t[static 16], size_t, uint8_t[static 16], uint32_t);
void aes_ssse3_xts_dec(const struct aesdec *, const uint8_t[static 16],
    uint8_t[static 16], size_t, uint8_t[static 16], uint32_t);
void aes_ssse3_cbcmac_update1(const struct aesenc *, const uint8_t[static 16],
    size_t, uint8_t[static 16], uint32_t);
void aes_ssse3_ccm_enc1(const struct aesenc *, const uint8_t[static 16],
    uint8_t[static 16], size_t, uint8_t[static 32], uint32_t);
void aes_ssse3_ccm_dec1(const struct aesenc *, const uint8_t[static 16],
    uint8_t[static 16], size_t, uint8_t[static 32], uint32_t);

int aes_ssse3_selftest(void);

extern struct aes_impl aes_ssse3_impl;

#endif	/* _CRYPTO_AES_ARCH_X86_AES_SSSE3_H */
