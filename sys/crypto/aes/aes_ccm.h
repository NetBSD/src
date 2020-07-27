/*	$NetBSD: aes_ccm.h,v 1.2 2020/07/27 20:44:30 riastradh Exp $	*/

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

#ifndef	_CRYPTO_AES_AES_CCM_H
#define	_CRYPTO_AES_AES_CCM_H

#include <sys/types.h>

struct aesenc;

struct aes_ccm {
	const struct aesenc	*enc;
	uint8_t			authctr[32];	/* authenticator and counter */
	uint8_t			out[16];	/* AES output block */
	size_t			mlen, mleft;
	unsigned		i, nr, L, M;
};

void aes_ccm_init(struct aes_ccm *, unsigned, const struct aesenc *,
    unsigned, unsigned, const uint8_t *, unsigned, const void *, size_t,
    size_t);
void aes_ccm_enc(struct aes_ccm *, const void *, void *, size_t);
void aes_ccm_dec(struct aes_ccm *, const void *, void *, size_t);
void aes_ccm_tag(struct aes_ccm *, void *);
int aes_ccm_verify(struct aes_ccm *, const void *);

int aes_ccm_selftest(void);

#endif	/* _CRYPTO_AES_AES_CCM_H */
