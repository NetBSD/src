/*	$NetBSD: aes_keysched.c,v 1.2 2025/11/23 22:44:14 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: aes_keysched.c,v 1.2 2025/11/23 22:44:14 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_aes.h"
#endif

#include <sys/types.h>

#include <crypto/aes/aes_keysched.h>

#ifdef AES_BEAR64
#include <crypto/aes/aes_bear64.h>
#else
#include <crypto/aes/aes_bear.h>
#endif

/*
 * aes_keysched_enc(rk, key, keybytes)
 *
 *	Compute the standard AES encryption key schedule, expanding a
 *	16-, 24-, or 32-byte key into 44, 52, or 60 32-bit round keys
 *	for encryption.  Returns the number of rounds for the key of
 *	this length.
 */
u_int
aes_keysched_enc(uint32_t *rk, const void *key, size_t keybytes)
{

#ifdef AES_BEAR64
	return br_aes_ct64_keysched_stdenc(rk, key, keybytes);
#else
	return br_aes_ct_keysched_stdenc(rk, key, keybytes);
#endif
}

/*
 * aes_keysched_dec(rk, key, keybytes)
 *
 *	Compute the standard AES decryption key schedule, expanding a
 *	16-, 24-, or 32-byte key into 44, 52, or 60 32-bit round keys
 *	and applying InvMixColumns for decryption.  Returns the number
 *	of rounds for the key of this length.
 */
u_int
aes_keysched_dec(uint32_t *rk, const void *key, size_t keybytes)
{

#ifdef AES_BEAR64
	return br_aes_ct64_keysched_stddec(rk, key, keybytes);
#else
	return br_aes_ct_keysched_stddec(rk, key, keybytes);
#endif
}
