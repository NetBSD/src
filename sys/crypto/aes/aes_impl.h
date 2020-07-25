/*	$NetBSD: aes_impl.h,v 1.2 2020/07/25 22:27:53 riastradh Exp $	*/

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

#ifndef	_CRYPTO_AES_AES_IMPL_H
#define	_CRYPTO_AES_AES_IMPL_H

#include <sys/types.h>

struct aesenc;
struct aesdec;

struct aes_impl {
	const char *ai_name;
	int	(*ai_probe)(void);
	void	(*ai_setenckey)(struct aesenc *, const uint8_t *, uint32_t);
	void	(*ai_setdeckey)(struct aesdec *, const uint8_t *, uint32_t);
	void	(*ai_enc)(const struct aesenc *, const uint8_t[static 16],
		    uint8_t[static 16], uint32_t);
	void	(*ai_dec)(const struct aesdec *, const uint8_t[static 16],
		    uint8_t[static 16], uint32_t);
	void	(*ai_cbc_enc)(const struct aesenc *, const uint8_t[static 16],
		    uint8_t[static 16], size_t, uint8_t[static 16], uint32_t);
	void	(*ai_cbc_dec)(const struct aesdec *, const uint8_t[static 16],
		    uint8_t[static 16], size_t, uint8_t[static 16], uint32_t);
	void	(*ai_xts_enc)(const struct aesenc *, const uint8_t[static 16],
		    uint8_t[static 16], size_t, uint8_t[static 16], uint32_t);
	void	(*ai_xts_dec)(const struct aesdec *, const uint8_t[static 16],
		    uint8_t[static 16], size_t, uint8_t[static 16], uint32_t);
	void	(*ai_cbcmac_update1)(const struct aesenc *,
		    const uint8_t[static 16], size_t, uint8_t[static 16],
		    uint32_t);
	void	(*ai_ccm_enc1)(const struct aesenc *,
		    const uint8_t[static 16], uint8_t[static 16],
		    size_t, uint8_t[static 32], uint32_t);
	void	(*ai_ccm_dec1)(const struct aesenc *,
		    const uint8_t[static 16], uint8_t[static 16],
		    size_t, uint8_t[static 32], uint32_t);
};

void	aes_md_init(const struct aes_impl *);

int	aes_selftest(const struct aes_impl *);

/* Internal subroutines dispatched to implementation for AES-CCM.  */
void	aes_cbcmac_update1(const struct aesenc *, const uint8_t[static 16],
	    size_t, uint8_t[static 16], uint32_t);
void	aes_ccm_enc1(const struct aesenc *, const uint8_t[static 16],
	    uint8_t[static 16], size_t, uint8_t[static 32], uint32_t);
void	aes_ccm_dec1(const struct aesenc *, const uint8_t[static 16],
	    uint8_t[static 16], size_t, uint8_t[static 32], uint32_t);

#endif	/* _CRYPTO_AES_AES_IMPL_H */
