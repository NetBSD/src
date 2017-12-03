/*	$NetBSD: prng.c,v 1.2.2.2 2017/12/03 11:35:48 jdolecek Exp $	*/

/*
 * Copyright (c) 2017 The NetBSD Foundation, Inc. All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#include "prekern.h"
#include <sys/sha1.h>
#include <sys/sha2.h>

#define _KERNEL
#include <machine/bootinfo.h>
#undef _KERNEL

#define CPUID_SEF_RDSEED	__BIT(18)
#define CPUID2_RDRAND	0x40000000
static bool has_rdrand = false;
static bool has_rdseed = false;

#define RND_SAVEWORDS	128
typedef struct {
	uint32_t entropy;
	uint8_t data[RND_SAVEWORDS * sizeof(uint32_t)];
	uint8_t digest[SHA1_DIGEST_LENGTH];
} rndsave_t;

#define RNGSTATE_SIZE	(SHA512_DIGEST_LENGTH / 2)
#define RNGDATA_SIZE	(SHA512_DIGEST_LENGTH / 2)
struct {
	uint8_t state[RNGSTATE_SIZE];
	uint8_t data[RNGDATA_SIZE];
	size_t nused;
} rng;

static struct btinfo_common *
prng_lookup_bootinfo(int type)
{
	extern struct bootinfo bootinfo;
	struct btinfo_common *bic;
	bool found;
	int i;

	bic = (struct btinfo_common *)(bootinfo.bi_data);
	found = false;
	for (i = 0; i < bootinfo.bi_nentries && !found; i++) {
		if (bic->type == type)
			found = true;
		else
			bic = (struct btinfo_common *)
			    ((uint8_t *)bic + bic->len);
	}
	return found ? bic : NULL;
}

static void
prng_get_entropy_file(SHA512_CTX *ctx)
{
	struct bi_modulelist_entry *bi, *bimax;
	struct btinfo_modulelist *biml;
	uint8_t digest[SHA1_DIGEST_LENGTH];
	rndsave_t *rndsave;
	SHA1_CTX sig;

	biml =
	    (struct btinfo_modulelist *)prng_lookup_bootinfo(BTINFO_MODULELIST);
	if (biml == NULL) {
		return;
	}

	bi = (struct bi_modulelist_entry *)((uint8_t *)biml + sizeof(*biml));
	bimax = bi + biml->num;
	for (; bi < bimax; bi++) {
		if (bi->type != BI_MODULE_RND) {
			continue;
		}
		if (bi->len != sizeof(rndsave_t)) {
			fatal("rndsave_t size mismatch");
		}
		rndsave = (rndsave_t *)(vaddr_t)bi->base;

		/* check the signature */
		SHA1Init(&sig);
		SHA1Update(&sig, (uint8_t *)&rndsave->entropy,
		    sizeof(rndsave->entropy));
		SHA1Update(&sig, rndsave->data, sizeof(rndsave->data));
		SHA1Final(digest, &sig);
		if (memcmp(digest, rndsave->digest, sizeof(digest))) {
			fatal("bad SHA1 checksum");
		}

		SHA512_Update(ctx, rndsave->data, sizeof(rndsave->data));
	}
}

/*
 * Add 32 bytes of rdseed/rdrand and 8 bytes of rdtsc to the context.
 */
static void
prng_get_entropy_data(SHA512_CTX *ctx)
{
	uint64_t buf[8], val;
	size_t i;

	if (has_rdseed) {
		for (i = 0; i < 8; i++) {
			if (rdseed(&buf[i]) == -1) {
				break;
			}
		}
		SHA512_Update(ctx, (uint8_t *)buf, i * sizeof(uint64_t));
	} else if (has_rdrand) {
		for (i = 0; i < 8; i++) {
			if (rdrand(&buf[i]) == -1) {
				break;
			}
		}
		SHA512_Update(ctx, (uint8_t *)buf, i * sizeof(uint64_t));
	}

	val = rdtsc();
	SHA512_Update(ctx, (uint8_t *)&val, sizeof(val));
}

void
prng_init(void)
{
	uint8_t digest[SHA512_DIGEST_LENGTH];
	SHA512_CTX ctx;
	u_int descs[4];

	memset(&rng, 0, sizeof(rng));

	/* detect cpu features */
	cpuid(0x07, 0x00, descs);
	has_rdseed = (descs[1] & CPUID_SEF_RDSEED) != 0;
	cpuid(0x01, 0x00, descs);
	has_rdrand = (descs[2] & CPUID2_RDRAND) != 0;

	SHA512_Init(&ctx);
	prng_get_entropy_file(&ctx);
	prng_get_entropy_data(&ctx);
	SHA512_Final(digest, &ctx);

	memcpy(rng.state, digest, RNGSTATE_SIZE);
	memcpy(rng.data, digest + RNGSTATE_SIZE, RNGDATA_SIZE);
}

static void
prng_round(void)
{
	uint8_t digest[SHA512_DIGEST_LENGTH];
	SHA512_CTX ctx;

	SHA512_Init(&ctx);
	SHA512_Update(&ctx, rng.state, RNGSTATE_SIZE);
	prng_get_entropy_data(&ctx);
	SHA512_Final(digest, &ctx);

	memcpy(rng.state, digest, RNGSTATE_SIZE);
	memcpy(rng.data, digest + RNGSTATE_SIZE, RNGDATA_SIZE);

	rng.nused = 0;
}

void
prng_get_rand(void *buf, size_t sz)
{
	uint8_t *ptr = (uint8_t *)buf;
	size_t consumed;

	ASSERT(sz <= RNGDATA_SIZE);
	if (rng.nused + sz > RNGDATA_SIZE) {
		/* Fill what can be */
		consumed = RNGDATA_SIZE - rng.nused;
		memcpy(ptr, &rng.data[rng.nused], consumed);

		/* Go through another round */
		prng_round();

		/* Fill the rest */
		memcpy(ptr + consumed, &rng.data[rng.nused],
		    sz - consumed);

		rng.nused += (sz - consumed);
	} else {
		memcpy(ptr, &rng.data[rng.nused], sz);
		rng.nused += sz;
	}
}
