/*	$NetBSD: t_sha512trunc.c,v 1.2.2.2 2024/03/25 14:14:55 martin Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_sha512trunc.c,v 1.2.2.2 2024/03/25 14:14:55 martin Exp $");

#include <stddef.h>

#include <atf-c.h>

#include <openssl/evp.h>

#include "h_macros.h"

struct testcase {
	const unsigned char in[128];
	size_t inlen;
	const unsigned char out[32];
};

static void
check(const struct testcase *C, size_t n, size_t digestlen, const EVP_MD *md)
{
	enum { C0 = 0xc0, C1 = 0xc1 };
	unsigned char *buf, *digest, *p0, *p1;
	size_t i;

	ATF_REQUIRE_MSG(digestlen <= INT_MAX, "digestlen=%zu", digestlen);
	ATF_REQUIRE_EQ_MSG((int)digestlen, EVP_MD_size(md),
	    "expected %d, got %d", (int)digestlen, EVP_MD_size(md));

	ATF_REQUIRE_MSG(digestlen < SIZE_MAX - 2048,
	    "digestlen=%zu", digestlen);
	REQUIRE_LIBC(buf = malloc(digestlen + 2048), NULL);
	p0 = buf;
	digest = buf + 1;
	p1 = buf + 1 + digestlen;

	for (i = 0; i < n; i++) {
		EVP_MD_CTX *ctx;
		unsigned digestlen1;

		*p0 = C0;
		*p1 = C1;

#define	REQUIRE(x)	ATF_REQUIRE_MSG((x), "i=%zu", i)
		REQUIRE(ctx = EVP_MD_CTX_new());
		REQUIRE(EVP_DigestInit_ex(ctx, md, NULL));
		REQUIRE(EVP_DigestUpdate(ctx, C->in, C->inlen));
		REQUIRE(EVP_DigestFinal_ex(ctx, digest, &digestlen1));
#undef	REQUIRE
		ATF_CHECK_MSG(digestlen == digestlen1,
		    "i=%zu: expected %zu got %u", i, digestlen, digestlen1);
		EVP_MD_CTX_free(ctx);

		ATF_CHECK_MSG(memcmp(digest, C->out, digestlen) == 0,
		    "i=%zu", i);

		ATF_CHECK_EQ_MSG(*p0, C0, "expected 0x%x got 0x%hhx", C0, *p0);
		ATF_CHECK_EQ_MSG(*p1, C1, "expected 0x%x got 0x%hhx", C1, *p1);
	}
}

/*
 * Test vectors from:
 *
 * https://csrc.nist.gov/Projects/Cryptographic-Algorithm-Validation-Program/Secure-Hashing#Testing
 */

ATF_TC(sha512_224);
ATF_TC_HEAD(sha512_224, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test SHA512-224");
}
ATF_TC_BODY(sha512_224, tc)
{
	static const struct testcase C[] = {
		[0] = {
			.inlen = 0,
			.out = {
				0x6e,0xd0,0xdd,0x02, 0x80,0x6f,0xa8,0x9e,
				0x25,0xde,0x06,0x0c, 0x19,0xd3,0xac,0x86,
				0xca,0xbb,0x87,0xd6, 0xa0,0xdd,0xd0,0x5c,
				0x33,0x3b,0x84,0xf4,
			},
		},
		[1] = {
			.inlen = 1,
			.in = {
				0xcf,
			},
			.out = {
				0x41,0x99,0x23,0x9e, 0x87,0xd4,0x7b,0x6f,
				0xed,0xa0,0x16,0x80, 0x2b,0xf3,0x67,0xfb,
				0x6e,0x8b,0x56,0x55, 0xef,0xf6,0x22,0x5c,
				0xb2,0x66,0x8f,0x4a,
			},
		},
	};

	check(C, __arraycount(C), 28, EVP_sha512_224());
}

ATF_TC(sha512_256);
ATF_TC_HEAD(sha512_256, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test SHA512-256");
}
ATF_TC_BODY(sha512_256, tc)
{
	static const struct testcase C[] = {
		[0] = {
			.inlen = 0,
			.out = {
				0xc6,0x72,0xb8,0xd1, 0xef,0x56,0xed,0x28,
				0xab,0x87,0xc3,0x62, 0x2c,0x51,0x14,0x06,
				0x9b,0xdd,0x3a,0xd7, 0xb8,0xf9,0x73,0x74,
				0x98,0xd0,0xc0,0x1e, 0xce,0xf0,0x96,0x7a,
			},
		},
		[1] = {
			.inlen = 1,
			.in = {
				0xfa,
			},
			.out = {
				0xc4,0xef,0x36,0x92, 0x3c,0x64,0xe5,0x1e,
				0x87,0x57,0x20,0xe5, 0x50,0x29,0x8a,0x5a,
				0xb8,0xa3,0xf2,0xf8, 0x75,0xb1,0xe1,0xa4,
				0xc9,0xb9,0x5b,0xab, 0xf7,0x34,0x4f,0xef,
			},
		},
	};

	check(C, __arraycount(C), 32, EVP_sha512_256());
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, sha512_224);
	ATF_TP_ADD_TC(tp, sha512_256);

	return atf_no_error();
}
