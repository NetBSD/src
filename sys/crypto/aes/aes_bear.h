/*	$NetBSD: aes_bear.h,v 1.2 2020/06/29 23:36:59 riastradh Exp $	*/

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

#ifndef	_CRYPTO_AES_AES_BEAR_H
#define	_CRYPTO_AES_AES_BEAR_H

#include <sys/types.h>
#include <sys/endian.h>

#include <crypto/aes/aes.h>

#define	br_dec32le	le32dec
#define	br_enc32le	le32enc

void	br_aes_ct_bitslice_Sbox(uint32_t *);
void	br_aes_ct_bitslice_invSbox(uint32_t *);
void	br_aes_ct_ortho(uint32_t *);
u_int	br_aes_ct_keysched(uint32_t *, const void *, size_t);
void	br_aes_ct_skey_expand(uint32_t *, unsigned, const uint32_t *);
void	br_aes_ct_bitslice_encrypt(unsigned, const uint32_t *, uint32_t *);
void	br_aes_ct_bitslice_decrypt(unsigned, const uint32_t *, uint32_t *);

/* NetBSD additions */

void	br_aes_ct_inv_mix_columns(uint32_t *);
u_int	br_aes_ct_keysched_stdenc(uint32_t *, const void *, size_t);
u_int	br_aes_ct_keysched_stddec(uint32_t *, const void *, size_t);

extern struct aes_impl	aes_bear_impl;

#endif	/* _CRYPTO_AES_AES_BEAR_H */
