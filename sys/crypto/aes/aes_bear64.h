/*	$NetBSD: aes_bear64.h,v 1.1 2025/11/23 22:44:13 riastradh Exp $	*/

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

#ifndef	_CRYPTO_AES_AES_BEAR64_H
#define	_CRYPTO_AES_AES_BEAR64_H

#include <sys/types.h>
#include <sys/endian.h>

#include <crypto/aes/aes.h>

#define	br_dec32le	le32dec
#define	br_enc32le	le32enc

void	br_aes_ct64_bitslice_Sbox(uint64_t[static 8]);
void	br_aes_ct64_bitslice_invSbox(uint64_t[static 8]);
void	br_aes_ct64_ortho(uint64_t[static 8]);
void	br_aes_ct64_interleave_in(uint64_t[static 1], uint64_t[static 1],
	    const uint32_t[static 4]);
void	br_aes_ct64_interleave_out(uint32_t[static 4], uint64_t, uint64_t);
u_int	br_aes_ct64_keysched(uint64_t[static 30], const void *, size_t);
void	br_aes_ct64_skey_expand(uint64_t[static 120], unsigned,
	    const uint64_t[static 30]);
void	br_aes_ct64_bitslice_encrypt(unsigned, const uint64_t[static 120],
	    uint64_t[static 8]);
void	br_aes_ct64_bitslice_decrypt(unsigned, const uint64_t[static 120],
	    uint64_t[static 8]);

/* NetBSD additions */

void	br_aes_ct64_inv_mix_columns(uint64_t[static 8]);
u_int	br_aes_ct64_keysched_stdenc(uint32_t *, const void *, size_t);
u_int	br_aes_ct64_keysched_stddec(uint32_t *, const void *, size_t);

extern struct aes_impl	aes_bear64_impl;

#endif	/* _CRYPTO_AES_AES_BEAR64_H */
