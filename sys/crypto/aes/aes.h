/*	$NetBSD: aes.h,v 1.3 2020/07/25 22:12:57 riastradh Exp $	*/

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

#ifndef	_CRYPTO_AES_AES_H
#define	_CRYPTO_AES_AES_H

#include <sys/types.h>
#include <sys/cdefs.h>

/*
 * struct aes
 *
 *	Expanded round keys.
 */
union aes {
	uint32_t	aes_rk[60];
	uint64_t	aes_rk64[30];
} __aligned(16);

#define	AES_128_NROUNDS	10
#define	AES_192_NROUNDS	12
#define	AES_256_NROUNDS	14

struct aesenc {
	union aes	aese_aes;
};

struct aesdec {
	union aes	aesd_aes;
};

uint32_t aes_setenckey128(struct aesenc *, const uint8_t[static 16]);
uint32_t aes_setenckey192(struct aesenc *, const uint8_t[static 24]);
uint32_t aes_setenckey256(struct aesenc *, const uint8_t[static 32]);
uint32_t aes_setdeckey128(struct aesdec *, const uint8_t[static 16]);
uint32_t aes_setdeckey192(struct aesdec *, const uint8_t[static 24]);
uint32_t aes_setdeckey256(struct aesdec *, const uint8_t[static 32]);

void	aes_enc(const struct aesenc *, const uint8_t[static 16],
	    uint8_t[static 16], uint32_t);
void	aes_dec(const struct aesdec *, const uint8_t[static 16],
	    uint8_t[static 16], uint32_t);

void	aes_cbc_enc(struct aesenc *, const uint8_t[static 16],
	    uint8_t[static 16], size_t, uint8_t[static 16], uint32_t);
void	aes_cbc_dec(struct aesdec *, const uint8_t[static 16],
	    uint8_t[static 16], size_t, uint8_t[static 16], uint32_t);

void	aes_xts_enc(struct aesenc *, const uint8_t[static 16],
	    uint8_t[static 16], size_t, uint8_t[static 16], uint32_t);
void	aes_xts_dec(struct aesdec *, const uint8_t[static 16],
	    uint8_t[static 16], size_t, uint8_t[static 16], uint32_t);

#endif	/* _CRYPTO_AES_AES_H */
