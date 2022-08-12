/*	$NetBSD: hkdf_hmac_sha256.c,v 1.1 2022/08/12 10:49:17 riastradh Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: hkdf_hmac_sha256.c,v 1.1 2022/08/12 10:49:17 riastradh Exp $");

#include <sys/sha2.h>

#include <stdint.h>
#include <string.h>

#include "hkdf_hmac_sha256.h"

/* RFC 2104: HMAC */

struct hmacsha256 {
	SHA256_CTX sha256;
	uint8_t key[SHA256_BLOCK_LENGTH];
};

static void
hmacsha256_init(struct hmacsha256 *H, const void *key, size_t keylen)
{
	uint8_t hkey[SHA256_DIGEST_LENGTH];
	uint8_t ipad[SHA256_BLOCK_LENGTH];
	unsigned i;

	if (keylen > SHA256_BLOCK_LENGTH) { /* XXX should not happen here */
		SHA256_Init(&H->sha256);
		SHA256_Update(&H->sha256, key, keylen);
		SHA256_Final(hkey, &H->sha256);
		key = hkey;
		keylen = sizeof(hkey);
	}

	memset(H->key, 0, sizeof(H->key));
	memcpy(H->key, key, keylen);

	for (i = 0; i < SHA256_BLOCK_LENGTH; i++)
		ipad[i] = 0x36 ^ H->key[i];

	SHA256_Init(&H->sha256);
	SHA256_Update(&H->sha256, ipad, SHA256_BLOCK_LENGTH);

	explicit_memset(hkey, 0, sizeof(hkey));
	explicit_memset(ipad, 0, sizeof(ipad));
}

static void
hmacsha256_update(struct hmacsha256 *H, const void *buf, size_t buflen)
{

	SHA256_Update(&H->sha256, buf, buflen);
}

static void
hmacsha256_final(uint8_t h[static SHA256_DIGEST_LENGTH],
    struct hmacsha256 *H)
{
	uint8_t opad[SHA256_BLOCK_LENGTH];
	unsigned i;

	for (i = 0; i < SHA256_BLOCK_LENGTH; i++)
		opad[i] = 0x5c ^ H->key[i];

	SHA256_Final(h, &H->sha256);
	SHA256_Init(&H->sha256);
	SHA256_Update(&H->sha256, opad, SHA256_BLOCK_LENGTH);
	SHA256_Update(&H->sha256, h, SHA256_DIGEST_LENGTH);
	SHA256_Final(h, &H->sha256);

	explicit_memset(opad, 0, sizeof(opad));
	explicit_memset(H, 0, sizeof(*H));
}

/* RFC 5869 HKDF, Sec. 2.3 HKDF-Expand */

int
hkdf_hmac_sha256(void *okm, size_t L,
    const void *prk, size_t prklen,
    const void *info, size_t infolen)
{
	struct hmacsha256 hmacsha256;
	size_t n, tlen;
	uint8_t T[SHA256_DIGEST_LENGTH], *p = okm;
	uint8_t i;

	if (L > 255*SHA256_DIGEST_LENGTH)
		return -1;
	if (L == 0)
		return 0;

	for (tlen = 0, i = 1; L > 0; i++, tlen = SHA256_DIGEST_LENGTH) {
		hmacsha256_init(&hmacsha256, prk, prklen);
		hmacsha256_update(&hmacsha256, T, tlen);
		hmacsha256_update(&hmacsha256, info, infolen);
		hmacsha256_update(&hmacsha256, &i, 1);
		hmacsha256_final(T, &hmacsha256);
		n = (L < SHA256_DIGEST_LENGTH ? L : SHA256_DIGEST_LENGTH);
		memcpy(p, T, n);
		p += n;
		L -= n;
	}

	explicit_memset(T, 0, sizeof(T));
	return 0;
}

#if 0
#include <stdarg.h>
#include <stdio.h>

static void
hexdump(const void *buf, size_t len, const char *fmt, ...)
{
	va_list va;
	const uint8_t *p = buf;
	size_t i;

	printf("### ");
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
	printf(" (%zu bytes):\n", len);
	for (i = 0; i < len; i++) {
		if (i % 8 == 0)
			printf(" ");
		printf(" %02x", p[i]);
		if (i % 16 == 15)
			printf("\n");
	}
	if (i % 16)
		printf("\n");
}
#endif

int
hkdf_hmac_sha256_selftest(void)
{
	const struct {
		size_t L;
		const uint8_t *okm;
		size_t prklen;
		const uint8_t *prk;
		size_t infolen;
		const uint8_t *info;
	} C[] = {
		[0] = {		/* A.1 Test Case 1 with SHA-256 */
			.L = 42,
			.okm = (const uint8_t[]) {
				0x3c,0xb2,0x5f,0x25, 0xfa,0xac,0xd5,0x7a,
				0x90,0x43,0x4f,0x64, 0xd0,0x36,0x2f,0x2a,
				0x2d,0x2d,0x0a,0x90, 0xcf,0x1a,0x5a,0x4c,
				0x5d,0xb0,0x2d,0x56, 0xec,0xc4,0xc5,0xbf,
				0x34,0x00,0x72,0x08, 0xd5,0xb8,0x87,0x18,
				0x58,0x65,
			},
			.prklen = 32,
			.prk = (const uint8_t[]) {
				0x07,0x77,0x09,0x36, 0x2c,0x2e,0x32,0xdf,
				0x0d,0xdc,0x3f,0x0d, 0xc4,0x7b,0xba,0x63,
				0x90,0xb6,0xc7,0x3b, 0xb5,0x0f,0x9c,0x31,
				0x22,0xec,0x84,0x4a, 0xd7,0xc2,0xb3,0xe5,
			},
			.infolen = 10,
			.info = (const uint8_t[]) {
				0xf0,0xf1,0xf2,0xf3, 0xf4,0xf5,0xf6,0xf7,
				0xf8,0xf9,
			},
		},
		[1] = {		/* A.2 Test Case 2 with SHA-256, longer I/O */
			.L = 82,
			.okm = (const uint8_t[]) {
				0xb1,0x1e,0x39,0x8d, 0xc8,0x03,0x27,0xa1,
				0xc8,0xe7,0xf7,0x8c, 0x59,0x6a,0x49,0x34,
				0x4f,0x01,0x2e,0xda, 0x2d,0x4e,0xfa,0xd8,
				0xa0,0x50,0xcc,0x4c, 0x19,0xaf,0xa9,0x7c,
				0x59,0x04,0x5a,0x99, 0xca,0xc7,0x82,0x72,
				0x71,0xcb,0x41,0xc6, 0x5e,0x59,0x0e,0x09,
				0xda,0x32,0x75,0x60, 0x0c,0x2f,0x09,0xb8,
				0x36,0x77,0x93,0xa9, 0xac,0xa3,0xdb,0x71,
				0xcc,0x30,0xc5,0x81, 0x79,0xec,0x3e,0x87,
				0xc1,0x4c,0x01,0xd5, 0xc1,0xf3,0x43,0x4f,
				0x1d,0x87,
			},
			.prklen = 32,
			.prk = (const uint8_t[]) {
				0x06,0xa6,0xb8,0x8c, 0x58,0x53,0x36,0x1a,
				0x06,0x10,0x4c,0x9c, 0xeb,0x35,0xb4,0x5c,
				0xef,0x76,0x00,0x14, 0x90,0x46,0x71,0x01,
				0x4a,0x19,0x3f,0x40, 0xc1,0x5f,0xc2,0x44,
			},
			.infolen = 80,
			.info = (const uint8_t[]) {
				0xb0,0xb1,0xb2,0xb3, 0xb4,0xb5,0xb6,0xb7,
				0xb8,0xb9,0xba,0xbb, 0xbc,0xbd,0xbe,0xbf,
				0xc0,0xc1,0xc2,0xc3, 0xc4,0xc5,0xc6,0xc7,
				0xc8,0xc9,0xca,0xcb, 0xcc,0xcd,0xce,0xcf,
				0xd0,0xd1,0xd2,0xd3, 0xd4,0xd5,0xd6,0xd7,
				0xd8,0xd9,0xda,0xdb, 0xdc,0xdd,0xde,0xdf,
				0xe0,0xe1,0xe2,0xe3, 0xe4,0xe5,0xe6,0xe7,
				0xe8,0xe9,0xea,0xeb, 0xec,0xed,0xee,0xef,
				0xf0,0xf1,0xf2,0xf3, 0xf4,0xf5,0xf6,0xf7,
				0xf8,0xf9,0xfa,0xfb, 0xfc,0xfd,0xfe,0xff,
			},
		},
		[2] = {		/* A.3 Test Case 3 with SHA-256, empty info */
			.L = 42,
			.okm = (const uint8_t[]) {
				0x8d,0xa4,0xe7,0x75, 0xa5,0x63,0xc1,0x8f,
				0x71,0x5f,0x80,0x2a, 0x06,0x3c,0x5a,0x31,
				0xb8,0xa1,0x1f,0x5c, 0x5e,0xe1,0x87,0x9e,
				0xc3,0x45,0x4e,0x5f, 0x3c,0x73,0x8d,0x2d,
				0x9d,0x20,0x13,0x95, 0xfa,0xa4,0xb6,0x1a,
				0x96,0xc8,
			},
			.prklen = 32,
			.prk = (const uint8_t[]) {
				0x19,0xef,0x24,0xa3, 0x2c,0x71,0x7b,0x16,
				0x7f,0x33,0xa9,0x1d, 0x6f,0x64,0x8b,0xdf,
				0x96,0x59,0x67,0x76, 0xaf,0xdb,0x63,0x77,
				0xac,0x43,0x4c,0x1c, 0x29,0x3c,0xcb,0x04,
			},
			.infolen = 0,
			.info = NULL,
		},
	};
	uint8_t okm[128];
	unsigned i;

	for (i = 0; i < __arraycount(C); i++) {
		if (hkdf_hmac_sha256(okm, C[i].L, C[i].prk, C[i].prklen,
			C[i].info, C[i].infolen))
			return -1;
		if (memcmp(okm, C[i].okm, C[i].L))
			return -1;
	}

	return 0;
}

#if 0
int
main(void)
{
	return hkdf_hmac_sha256_selftest();
}
#endif
