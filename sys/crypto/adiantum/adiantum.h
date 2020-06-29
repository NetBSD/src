/*	$NetBSD: adiantum.h,v 1.1 2020/06/29 23:44:01 riastradh Exp $	*/

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

#ifndef	_CRYPTO_ADIANTUM_ADIANTUM_H
#define	_CRYPTO_ADIANTUM_ADIANTUM_H

#include <sys/types.h>

#ifdef _KERNEL
#include <crypto/aes/aes.h>
#endif

struct adiantum {
	uint8_t		ks[32];		/* XChaCha12 key */

	/* BEGIN XCHACHA12 OUTPUT -- DO NOT REORDER */
	uint8_t		kk[32];		/* AES key */
	uint8_t		kt[16];		/* Poly1305 tweak key */
	uint8_t		kl[16];		/* Poly1305 message key */
	uint32_t	kn[268];	/* NH key */
	/* END XCHACHA12 OUTPUT */

	struct aesenc	kk_enc;		/* expanded AES key */
	struct aesdec	kk_dec;
};

#define	ADIANTUM_KEYBYTES	32
#define	ADIANTUM_BLOCKBYTES	16 /* size must be positive multiple of this */

void adiantum_init(struct adiantum *, const uint8_t[static ADIANTUM_KEYBYTES]);
void adiantum_enc(void *, const void *, size_t, const void *, size_t,
    const struct adiantum *);
void adiantum_dec(void *, const void *, size_t, const void *, size_t,
    const struct adiantum *);

int adiantum_selftest(void);

#endif	/* _CRYPTO_ADIANTUM_ADIANTUM_H */
