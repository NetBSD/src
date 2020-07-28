/*	$NetBSD: chacha_neon.h,v 1.3 2020/07/28 20:08:48 riastradh Exp $	*/

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

#ifndef	_SYS_CRYPTO_CHACHA_ARCH_ARM_CHACHA_NEON_H
#define	_SYS_CRYPTO_CHACHA_ARCH_ARM_CHACHA_NEON_H

#include <sys/types.h>

#include <crypto/chacha/chacha_impl.h>

void	chacha_core_neon(uint8_t[restrict static 64],
	    const uint8_t[static 16],
	    const uint8_t[static 32],
	    const uint8_t[static 16],
	    unsigned);
void	hchacha_neon(uint8_t[restrict static 32],
	    const uint8_t[static 16],
	    const uint8_t[static 32],
	    const uint8_t[static 16],
	    unsigned);
void	chacha_stream_neon(uint8_t *restrict, size_t,
	    uint32_t,
	    const uint8_t[static 12],
	    const uint8_t[static 32],
	    unsigned);
void	chacha_stream_xor_neon(uint8_t *, const uint8_t *, size_t,
	    uint32_t,
	    const uint8_t[static 12],
	    const uint8_t[static 32],
	    unsigned);
void	xchacha_stream_neon(uint8_t *restrict, size_t,
	    uint32_t,
	    const uint8_t[static 24],
	    const uint8_t[static 32],
	    unsigned);
void	xchacha_stream_xor_neon(uint8_t *, const uint8_t *, size_t,
	    uint32_t,
	    const uint8_t[static 24],
	    const uint8_t[static 32],
	    unsigned);

/* Assembly helpers */
void	chacha_stream256_neon(uint8_t[restrict static 256], uint32_t,
	    const uint8_t[static 12],
	    const uint8_t[static 32],
	    const uint8_t[static 16],
	    unsigned);
void	chacha_stream_xor256_neon(uint8_t[restrict static 256],
	    const uint8_t[static 256],
	    uint32_t,
	    const uint8_t[static 12],
	    const uint8_t[static 32],
	    const uint8_t[static 16],
	    unsigned);

extern const struct chacha_impl chacha_neon_impl;

#endif	/* _SYS_CRYPTO_CHACHA_ARCH_ARM_CHACHA_NEON_H */
