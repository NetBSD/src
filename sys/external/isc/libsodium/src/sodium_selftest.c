/*	$NetBSD: sodium_selftest.c,v 1.2.2.2 2024/10/09 10:49:03 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

#ifdef _KERNEL

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sodium_selftest.c,v 1.2.2.2 2024/10/09 10:49:03 martin Exp $");

#include <sys/types.h>

#include <sys/systm.h>

#include <lib/libkern/libkern.h>

#else

#include <sys/cdefs.h>
__RCSID("$NetBSD: sodium_selftest.c,v 1.2.2.2 2024/10/09 10:49:03 martin Exp $");

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void
hexdump(int (*prf)(const char *, ...) __printflike(1,2), const char *prefix,
    const void *buf, size_t len)
{
	const uint8_t *p = buf;
	size_t i;

	(*prf)("%s (%zu bytes @ %p)\n", prefix, len, buf);
	for (i = 0; i < len; i++) {
		if (i % 16 == 8)
			(*prf)("  ");
		else
			(*prf)(" ");
		(*prf)("%02hhx", p[i]);
		if ((i + 1) % 16 == 0)
			(*prf)("\n");
	}
	if (i % 16)
		(*prf)("\n");
}

#endif

#include <crypto/sodium/crypto_aead_chacha20poly1305.h>
#include <crypto/sodium/crypto_aead_xchacha20poly1305.h>
#include <crypto/sodium/sodium_selftest.h>

/*
 * Test misalignments up to and including this.  Would be nice to do
 * this up to, say, 15, but that takes 16^5 = 2^20 ~ 1m trials, which
 * is a bit steep as a self-test.
 */
#define	TESTALIGN	1

int
crypto_aead_chacha20poly1305_ietf_selftest(void)
{
	/* https://datatracker.ietf.org/doc/html/rfc8439#section-2.8.2 */
	static const uint8_t plaintext[] = {
		0x4c,0x61,0x64,0x69, 0x65,0x73,0x20,0x61,
		0x6e,0x64,0x20,0x47, 0x65,0x6e,0x74,0x6c,
		0x65,0x6d,0x65,0x6e, 0x20,0x6f,0x66,0x20,
		0x74,0x68,0x65,0x20, 0x63,0x6c,0x61,0x73,
		0x73,0x20,0x6f,0x66, 0x20,0x27,0x39,0x39,
		0x3a,0x20,0x49,0x66, 0x20,0x49,0x20,0x63,
		0x6f,0x75,0x6c,0x64, 0x20,0x6f,0x66,0x66,
		0x65,0x72,0x20,0x79, 0x6f,0x75,0x20,0x6f,
		0x6e,0x6c,0x79,0x20, 0x6f,0x6e,0x65,0x20,
		0x74,0x69,0x70,0x20, 0x66,0x6f,0x72,0x20,
		0x74,0x68,0x65,0x20, 0x66,0x75,0x74,0x75,
		0x72,0x65,0x2c,0x20, 0x73,0x75,0x6e,0x73,
		0x63,0x72,0x65,0x65, 0x6e,0x20,0x77,0x6f,
		0x75,0x6c,0x64,0x20, 0x62,0x65,0x20,0x69,
		0x74,0x2e,
	};
	static const uint8_t aad[] = {
		0x50,0x51,0x52,0x53, 0xc0,0xc1,0xc2,0xc3,
		0xc4,0xc5,0xc6,0xc7,
	};
	static const uint8_t key[] = {
		0x80,0x81,0x82,0x83, 0x84,0x85,0x86,0x87,
		0x88,0x89,0x8a,0x8b, 0x8c,0x8d,0x8e,0x8f,
		0x90,0x91,0x92,0x93, 0x94,0x95,0x96,0x97,
		0x98,0x99,0x9a,0x9b, 0x9c,0x9d,0x9e,0x9f,
	};
	static const uint8_t nonce[] = {
		0x07,0x00,0x00,0x00,
		0x40,0x41,0x42,0x43, 0x44,0x45,0x46,0x47,
	};
	static const uint8_t ciphertext[] = {
		0xd3,0x1a,0x8d,0x34, 0x64,0x8e,0x60,0xdb,
		0x7b,0x86,0xaf,0xbc, 0x53,0xef,0x7e,0xc2,
		0xa4,0xad,0xed,0x51, 0x29,0x6e,0x08,0xfe,
		0xa9,0xe2,0xb5,0xa7, 0x36,0xee,0x62,0xd6,
		0x3d,0xbe,0xa4,0x5e, 0x8c,0xa9,0x67,0x12,
		0x82,0xfa,0xfb,0x69, 0xda,0x92,0x72,0x8b,
		0x1a,0x71,0xde,0x0a, 0x9e,0x06,0x0b,0x29,
		0x05,0xd6,0xa5,0xb6, 0x7e,0xcd,0x3b,0x36,
		0x92,0xdd,0xbd,0x7f, 0x2d,0x77,0x8b,0x8c,
		0x98,0x03,0xae,0xe3, 0x28,0x09,0x1b,0x58,
		0xfa,0xb3,0x24,0xe4, 0xfa,0xd6,0x75,0x94,
		0x55,0x85,0x80,0x8b, 0x48,0x31,0xd7,0xbc,
		0x3f,0xf4,0xde,0xf0, 0x8e,0x4b,0x7a,0x9d,
		0xe5,0x76,0xd2,0x65, 0x86,0xce,0xc6,0x4b,
		0x61,0x16,

		0x1a,0xe1,0x0b,0x59, 0x4f,0x09,0xe2,0x6a,
		0x7e,0x90,0x2e,0xcb, 0xd0,0x60,0x06,0x91,
	};
	uint8_t inbuf[sizeof(ciphertext) + TESTALIGN];
	uint8_t outbuf[sizeof(ciphertext) + TESTALIGN];
	uint8_t aadbuf[sizeof(aad) + TESTALIGN];
	uint8_t noncebuf[sizeof(nonce) + TESTALIGN];
	uint8_t keybuf[sizeof(key) + TESTALIGN];
	unsigned i, j, k, L, M;

	/*
	 * Iterate over alignment and misalignment of all four inputs
	 * (plaintext/ciphertext, associated data, nonce, and key), and
	 * the output (ciphertext/plaintext).
	 *
	 * With apologies for the quirky nonindentation here -- it just
	 * gets nested a little too much.
	 */
	for (i = 0; i <= TESTALIGN; i++) {
	for (j = 0; j <= TESTALIGN; j++) {
	for (k = 0; k <= TESTALIGN; k++) {
	for (L = 0; L <= TESTALIGN; L++) {
	for (M = 0; M <= TESTALIGN; M++) {
		unsigned long long outsize = 0;
		int error;
		char t[128];
		unsigned u;

		/*
		 * Verify encryption produces the expected ciphertext.
		 */
		memset(inbuf, 0, sizeof(inbuf));
		memset(aadbuf, 0, sizeof(aadbuf));
		memset(noncebuf, 0, sizeof(noncebuf));
		memset(keybuf, 0, sizeof(keybuf));
		memset(outbuf, 0, sizeof(outbuf));

		memcpy(inbuf + i, plaintext, sizeof(plaintext));
		memcpy(aadbuf + j, aad, sizeof(aad));
		memcpy(noncebuf + k, nonce, sizeof(nonce));
		memcpy(keybuf + L, key, sizeof(key));

		error = crypto_aead_chacha20poly1305_ietf_encrypt(outbuf + M,
		    &outsize,
		    inbuf + i, sizeof(plaintext),
		    aadbuf + j, sizeof(aad),
		    NULL,	/* secret nonce, not supported */
		    noncebuf + k,
		    keybuf + L);
		if (error) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "encrypt", i, j, k, L, M);
			printf("%s: encrypt error=%d\n", t, error);
			return -1;
		}
		if (outsize != sizeof(ciphertext)) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "encrypt", i, j, k, L, M);
			printf("%s: outsize=%llu is not %zu\n", t,
			    outsize, sizeof(ciphertext));
			return -1;
		}
		if (memcmp(outbuf + M, ciphertext, sizeof(ciphertext)) != 0) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "encrypt", i, j, k, L, M);
			hexdump(printf, t, outbuf + M, sizeof(ciphertext));
			return -1;
		}

		/*
		 * Verify decryption of the valid ciphertext succeeds
		 * and produces the expected plaintext.
		 */
		memset(inbuf, 0, sizeof(inbuf));
		memset(aadbuf, 0, sizeof(aadbuf));
		memset(noncebuf, 0, sizeof(noncebuf));
		memset(keybuf, 0, sizeof(keybuf));
		memset(outbuf, 0, sizeof(outbuf));

		memcpy(inbuf + i, ciphertext, sizeof(ciphertext));
		memcpy(aadbuf + j, aad, sizeof(aad));
		memcpy(noncebuf + k, nonce, sizeof(nonce));
		memcpy(keybuf + L, key, sizeof(key));

		error = crypto_aead_chacha20poly1305_ietf_decrypt(outbuf + M,
		    &outsize,
		    NULL,	/* secret nonce, not supported */
		    inbuf + i, sizeof(ciphertext),
		    aadbuf + j, sizeof(aad),
		    noncebuf + k,
		    keybuf + L);
		if (error) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "decrypt", i, j, k, L, M);
			printf("%s: decrypt error=%d\n", t, error);
			return -1;
		}
		if (outsize != sizeof(plaintext)) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "decrypt", i, j, k, L, M);
			printf("%s: outsize=%llu is not %zu\n", t,
			    outsize, sizeof(plaintext));
			return -1;
		}
		if (memcmp(outbuf + M, plaintext, sizeof(plaintext)) != 0) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "decrypt", i, j, k, L, M);
			hexdump(printf, t, outbuf + M, sizeof(ciphertext));
			return -1;
		}

		/*
		 * Verify decryption of a corrupted ciphertext fails
		 * and produces all-zero output.
		 */
		memset(outbuf, 0x5a, sizeof(outbuf));
		inbuf[i] ^= 0x80;
		error = crypto_aead_chacha20poly1305_ietf_decrypt(outbuf + M,
		    &outsize,
		    NULL,	/* secret nonce, not supported */
		    inbuf + i, sizeof(ciphertext),
		    aadbuf + j, sizeof(aad),
		    noncebuf + k,
		    keybuf + L);
		inbuf[i] ^= 0x80;
		if (error == 0) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "msg forgery", i, j, k, L, M);
			printf("%s: wrongly accepted\n", t);
			return -1;
		}
		for (u = 0; u < sizeof(plaintext); u++) {
			if (outbuf[M + u] != 0) {
				snprintf(t, sizeof(t),
				    "%s: %s i=%u j=%u k=%u L=%u M=%u",
				    __func__, "msg forgery", i, j, k, L, M);
				hexdump(printf, t, outbuf + M,
				    sizeof(plaintext));
				return -1;
			}
		}

		/*
		 * Verify decryption with corrupted associated data
		 * fails and produces all-zero output.
		 */
		memset(outbuf, 0xac, sizeof(outbuf));
		aadbuf[j] ^= 0x80;
		error = crypto_aead_chacha20poly1305_ietf_decrypt(outbuf + M,
		    &outsize,
		    NULL,	/* secret nonce, not supported */
		    inbuf + i, sizeof(ciphertext),
		    aadbuf + j, sizeof(aad),
		    noncebuf + k,
		    keybuf + L);
		aadbuf[j] ^= 0x80;
		if (error == 0) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "aad forgery", i, j, k, L, M);
			printf("%s: wrongly accepted\n", t);
			return -1;
		}
		for (u = 0; u < sizeof(plaintext); u++) {
			if (outbuf[M + u] != 0) {
				snprintf(t, sizeof(t),
				    "%s: %s i=%u j=%u k=%u L=%u M=%u",
				    __func__, "aad forgery", i, j, k, L, M);
				hexdump(printf, t, outbuf + M,
				    sizeof(plaintext));
				return -1;
			}
		}
	}
	}
	}
	}
	}

	return 0;
}

int
crypto_aead_xchacha20poly1305_ietf_selftest(void)
{
	/* https://datatracker.ietf.org/doc/html/draft-irtf-cfrg-xchacha#appendix-A.3.1 */
	static const uint8_t plaintext[] = {
		0x4c,0x61,0x64,0x69, 0x65,0x73,0x20,0x61,
		0x6e,0x64,0x20,0x47, 0x65,0x6e,0x74,0x6c,
		0x65,0x6d,0x65,0x6e, 0x20,0x6f,0x66,0x20,
		0x74,0x68,0x65,0x20, 0x63,0x6c,0x61,0x73,
		0x73,0x20,0x6f,0x66, 0x20,0x27,0x39,0x39,
		0x3a,0x20,0x49,0x66, 0x20,0x49,0x20,0x63,
		0x6f,0x75,0x6c,0x64, 0x20,0x6f,0x66,0x66,
		0x65,0x72,0x20,0x79, 0x6f,0x75,0x20,0x6f,
		0x6e,0x6c,0x79,0x20, 0x6f,0x6e,0x65,0x20,
		0x74,0x69,0x70,0x20, 0x66,0x6f,0x72,0x20,
		0x74,0x68,0x65,0x20, 0x66,0x75,0x74,0x75,
		0x72,0x65,0x2c,0x20, 0x73,0x75,0x6e,0x73,
		0x63,0x72,0x65,0x65, 0x6e,0x20,0x77,0x6f,
		0x75,0x6c,0x64,0x20, 0x62,0x65,0x20,0x69,
		0x74,0x2e,
	};
	static const uint8_t aad[] = {
		0x50,0x51,0x52,0x53, 0xc0,0xc1,0xc2,0xc3,
		0xc4,0xc5,0xc6,0xc7,
	};
	static const uint8_t key[] = {
		0x80,0x81,0x82,0x83, 0x84,0x85,0x86,0x87,
		0x88,0x89,0x8a,0x8b, 0x8c,0x8d,0x8e,0x8f,
		0x90,0x91,0x92,0x93, 0x94,0x95,0x96,0x97,
		0x98,0x99,0x9a,0x9b, 0x9c,0x9d,0x9e,0x9f,
	};
	static const uint8_t nonce[] = {
		0x40,0x41,0x42,0x43, 0x44,0x45,0x46,0x47,
		0x48,0x49,0x4a,0x4b, 0x4c,0x4d,0x4e,0x4f,
		0x50,0x51,0x52,0x53, 0x54,0x55,0x56,0x57,
		0x00,0x00,0x00,0x00,
	};
	static const uint8_t ciphertext[] = {
		0xbd,0x6d,0x17,0x9d, 0x3e,0x83,0xd4,0x3b,
		0x95,0x76,0x57,0x94, 0x93,0xc0,0xe9,0x39,
		0x57,0x2a,0x17,0x00, 0x25,0x2b,0xfa,0xcc,
		0xbe,0xd2,0x90,0x2c, 0x21,0x39,0x6c,0xbb,
		0x73,0x1c,0x7f,0x1b, 0x0b,0x4a,0xa6,0x44,
		0x0b,0xf3,0xa8,0x2f, 0x4e,0xda,0x7e,0x39,
		0xae,0x64,0xc6,0x70, 0x8c,0x54,0xc2,0x16,
		0xcb,0x96,0xb7,0x2e, 0x12,0x13,0xb4,0x52,
		0x2f,0x8c,0x9b,0xa4, 0x0d,0xb5,0xd9,0x45,
		0xb1,0x1b,0x69,0xb9, 0x82,0xc1,0xbb,0x9e,
		0x3f,0x3f,0xac,0x2b, 0xc3,0x69,0x48,0x8f,
		0x76,0xb2,0x38,0x35, 0x65,0xd3,0xff,0xf9,
		0x21,0xf9,0x66,0x4c, 0x97,0x63,0x7d,0xa9,
		0x76,0x88,0x12,0xf6, 0x15,0xc6,0x8b,0x13,
		0xb5,0x2e,

		0xc0,0x87,0x59,0x24, 0xc1,0xc7,0x98,0x79,
		0x47,0xde,0xaf,0xd8, 0x78,0x0a,0xcf,0x49,
	};
	uint8_t inbuf[sizeof(ciphertext) + TESTALIGN];
	uint8_t outbuf[sizeof(ciphertext) + TESTALIGN];
	uint8_t aadbuf[sizeof(aad) + TESTALIGN];
	uint8_t noncebuf[sizeof(nonce) + TESTALIGN];
	uint8_t keybuf[sizeof(key) + TESTALIGN];
	unsigned i, j, k, L, M;

	/*
	 * Iterate over alignment and misalignment of all four inputs
	 * (plaintext/ciphertext, associated data, nonce, and key), and
	 * the output (ciphertext/plaintext).
	 *
	 * With apologies for the quirky nonindentation here -- it just
	 * gets nested a little too much.
	 */
	for (i = 0; i <= TESTALIGN; i++) {
	for (j = 0; j <= TESTALIGN; j++) {
	for (k = 0; k <= TESTALIGN; k++) {
	for (L = 0; L <= TESTALIGN; L++) {
	for (M = 0; M <= TESTALIGN; M++) {
		unsigned long long outsize = 0;
		int error;
		char t[128];
		unsigned u;

		/*
		 * Verify encryption produces the expected ciphertext.
		 */
		memset(inbuf, 0, sizeof(inbuf));
		memset(aadbuf, 0, sizeof(aadbuf));
		memset(noncebuf, 0, sizeof(noncebuf));
		memset(keybuf, 0, sizeof(keybuf));
		memset(outbuf, 0, sizeof(outbuf));

		memcpy(inbuf + i, plaintext, sizeof(plaintext));
		memcpy(aadbuf + j, aad, sizeof(aad));
		memcpy(noncebuf + k, nonce, sizeof(nonce));
		memcpy(keybuf + L, key, sizeof(key));

		error = crypto_aead_xchacha20poly1305_ietf_encrypt(outbuf + M,
		    &outsize,
		    inbuf + i, sizeof(plaintext),
		    aadbuf + j, sizeof(aad),
		    NULL,	/* secret nonce, not supported */
		    noncebuf + k,
		    keybuf + L);
		if (error) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "encrypt", i, j, k, L, M);
			printf("%s: encrypt error=%d\n", t, error);
			return -1;
		}
		if (outsize != sizeof(ciphertext)) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "encrypt", i, j, k, L, M);
			printf("%s: outsize=%llu is not %zu\n", t,
			    outsize, sizeof(ciphertext));
			return -1;
		}
		if (memcmp(outbuf + M, ciphertext, sizeof(ciphertext)) != 0) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "encrypt", i, j, k, L, M);
			hexdump(printf, t, outbuf + M, sizeof(ciphertext));
			return -1;
		}

		/*
		 * Verify decryption of the valid ciphertext succeeds
		 * and produces the expected plaintext.
		 */
		memset(inbuf, 0, sizeof(inbuf));
		memset(aadbuf, 0, sizeof(aadbuf));
		memset(noncebuf, 0, sizeof(noncebuf));
		memset(keybuf, 0, sizeof(keybuf));
		memset(outbuf, 0, sizeof(outbuf));

		memcpy(inbuf + i, ciphertext, sizeof(ciphertext));
		memcpy(aadbuf + j, aad, sizeof(aad));
		memcpy(noncebuf + k, nonce, sizeof(nonce));
		memcpy(keybuf + L, key, sizeof(key));

		error = crypto_aead_xchacha20poly1305_ietf_decrypt(outbuf + M,
		    &outsize,
		    NULL,	/* secret nonce, not supported */
		    inbuf + i, sizeof(ciphertext),
		    aadbuf + j, sizeof(aad),
		    noncebuf + k,
		    keybuf + L);
		if (error) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "decrypt", i, j, k, L, M);
			printf("%s: decrypt error=%d\n", t, error);
			return -1;
		}
		if (outsize != sizeof(plaintext)) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "decrypt", i, j, k, L, M);
			printf("%s: outsize=%llu is not %zu\n", t,
			    outsize, sizeof(plaintext));
			return -1;
		}
		if (memcmp(outbuf + M, plaintext, sizeof(plaintext)) != 0) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "decrypt", i, j, k, L, M);
			hexdump(printf, t, outbuf + M, sizeof(ciphertext));
			return -1;
		}

		/*
		 * Verify decryption of a corrupted ciphertext fails
		 * and produces all-zero output.
		 */
		memset(outbuf, 0x5a, sizeof(outbuf));
		inbuf[i] ^= 0x80;
		error = crypto_aead_xchacha20poly1305_ietf_decrypt(outbuf + M,
		    &outsize,
		    NULL,	/* secret nonce, not supported */
		    inbuf + i, sizeof(ciphertext),
		    aadbuf + j, sizeof(aad),
		    noncebuf + k,
		    keybuf + L);
		inbuf[i] ^= 0x80;
		if (error == 0) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "msg forgery", i, j, k, L, M);
			printf("%s: wrongly accepted\n", t);
			return -1;
		}
		for (u = 0; u < sizeof(plaintext); u++) {
			if (outbuf[M + u] != 0) {
				snprintf(t, sizeof(t),
				    "%s: %s i=%u j=%u k=%u L=%u M=%u",
				    __func__, "msg forgery", i, j, k, L, M);
				hexdump(printf, t, outbuf + M,
				    sizeof(plaintext));
				return -1;
			}
		}

		/*
		 * Verify decryption with corrupted associated data
		 * fails and produces all-zero output.
		 */
		memset(outbuf, 0xac, sizeof(outbuf));
		aadbuf[j] ^= 0x80;
		error = crypto_aead_xchacha20poly1305_ietf_decrypt(outbuf + M,
		    &outsize,
		    NULL,	/* secret nonce, not supported */
		    inbuf + i, sizeof(ciphertext),
		    aadbuf + j, sizeof(aad),
		    noncebuf + k,
		    keybuf + L);
		aadbuf[j] ^= 0x80;
		if (error == 0) {
			snprintf(t, sizeof(t),
			    "%s: %s i=%u j=%u k=%u L=%u M=%u",
			    __func__, "aad forgery", i, j, k, L, M);
			printf("%s: wrongly accepted\n", t);
			return -1;
		}
		for (u = 0; u < sizeof(plaintext); u++) {
			if (outbuf[M + u] != 0) {
				snprintf(t, sizeof(t),
				    "%s: %s i=%u j=%u k=%u L=%u M=%u",
				    __func__, "aad forgery", i, j, k, L, M);
				hexdump(printf, t, outbuf + M,
				    sizeof(plaintext));
				return -1;
			}
		}
	}
	}
	}
	}
	}

	return 0;
}

int
sodium_selftest(void)
{
	int result = 0;

	result |= crypto_aead_chacha20poly1305_ietf_selftest();
	result |= crypto_aead_xchacha20poly1305_ietf_selftest();

	return result;
}

#ifdef SODIUM_SELFTEST_MAIN
int
main(void)
{

	return sodium_selftest();
}
#endif
