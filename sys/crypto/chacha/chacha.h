/*	$NetBSD: chacha.h,v 1.1 2020/07/25 22:46:34 riastradh Exp $	*/

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

#ifndef	_SYS_CRYPTO_CHACHA_CHACHA_H
#define	_SYS_CRYPTO_CHACHA_CHACHA_H

#include <sys/types.h>

#define	CHACHA_KEYBYTES			32

#define	CHACHA_CORE_KEYBYTES		CHACHA_KEYBYTES
#define	CHACHA_CORE_INBYTES		16
#define	CHACHA_CORE_CONSTBYTES		16
#define	CHACHA_CORE_OUTBYTES		64

#define	HCHACHA_KEYBYTES		CHACHA_KEYBYTES
#define	HCHACHA_INBYTES			16
#define	HCHACHA_CONSTBYTES		16
#define	HCHACHA_OUTBYTES		32

#define	CHACHA_STREAM_KEYBYTES		CHACHA_KEYBYTES
#define	CHACHA_STREAM_NONCEBYTES	12

#define	XCHACHA_STREAM_KEYBYTES		CHACHA_KEYBYTES
#define	XCHACHA_STREAM_NONCEBYTES	24

extern const uint8_t chacha_const32[16];

void	chacha_core(uint8_t[restrict static CHACHA_CORE_OUTBYTES],
	    const uint8_t[static CHACHA_CORE_INBYTES],
	    const uint8_t[static CHACHA_CORE_KEYBYTES],
	    const uint8_t[static CHACHA_CORE_CONSTBYTES],
	    unsigned);
void	hchacha(uint8_t[restrict static HCHACHA_OUTBYTES],
	    const uint8_t[static HCHACHA_INBYTES],
	    const uint8_t[static HCHACHA_KEYBYTES],
	    const uint8_t[static HCHACHA_CONSTBYTES],
	    unsigned);
void	chacha_stream(uint8_t *restrict, size_t,
	    uint32_t,
	    const uint8_t[static CHACHA_STREAM_NONCEBYTES],
	    const uint8_t[static CHACHA_STREAM_KEYBYTES],
	    unsigned);
void	chacha_stream_xor(uint8_t *, const uint8_t *, size_t,
	    uint32_t,
	    const uint8_t[static CHACHA_STREAM_NONCEBYTES],
	    const uint8_t[static CHACHA_STREAM_KEYBYTES],
	    unsigned);
void	xchacha_stream(uint8_t *restrict, size_t,
	    uint32_t,
	    const uint8_t[static XCHACHA_STREAM_NONCEBYTES],
	    const uint8_t[static XCHACHA_STREAM_KEYBYTES],
	    unsigned);
void	xchacha_stream_xor(uint8_t *, const uint8_t *, size_t,
	    uint32_t,
	    const uint8_t[static XCHACHA_STREAM_NONCEBYTES],
	    const uint8_t[static XCHACHA_STREAM_KEYBYTES],
	    unsigned);

#endif	/* _SYS_CRYPTO_CHACHA_CHACHA_H */
