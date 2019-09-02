/*	$NetBSD: nist_hash_drbg.h,v 1.1 2019/09/02 20:09:29 riastradh Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef	NIST_HASH_DRBG_H
#define	NIST_HASH_DRBG_H

#include <sys/types.h>

/* Instantiation: SHA-256 */

/* 10.1 DRBG Mechanisms Based on Hash Functions, Table 2, SHA-256 column */
#define	NIST_SHA256_HASH_DRBG_SEEDLEN		440u

#define	NIST_HASH_DRBG_SEEDLEN		NIST_SHA256_HASH_DRBG_SEEDLEN
#define	nist_hash_drbg			nist_sha256_hash_drbg
#define	nist_hash_drbg_destroy		nist_sha256_hash_drbg_destroy
#define	nist_hash_drbg_generate		nist_sha256_hash_drbg_generate
#define	nist_hash_drbg_initialize	nist_sha256_hash_drbg_initialize
#define	nist_hash_drbg_instantiate	nist_sha256_hash_drbg_instantiate
#define	nist_hash_drbg_reseed		nist_sha256_hash_drbg_reseed

/*
 * By 10.1 DRBG Mechanisms Based on Hash Functions, Table 2, the limit
 * is <2^48 requests between reseeds.  We truncate this to fit in
 * 32-bit signed integer instead for hysterical raisins.
 */
#define	NIST_HASH_DRBG_RESEED_INTERVAL	0x7fffffff

/* 10.1 DRBG Mechanisms Based on Hash Functions, Table 2 */
#define	NIST_HASH_DRBG_MAX_REQUEST	0x80000
#define	NIST_HASH_DRBG_MAX_REQUEST_BYTES (NIST_HASH_DRBG_MAX_REQUEST/8)

#define	NIST_HASH_DRBG_SEEDLEN_BYTES	(NIST_HASH_DRBG_SEEDLEN/8)

#define	NIST_HASH_DRBG_MIN_SEEDLEN_BYTES				      \
	MIN(32, NIST_HASH_DRBG_SEEDLEN_BYTES)

/* 10.1.1.1 Hash_DRBG Internal State */

struct nist_hash_drbg {
	uint8_t		V[NIST_HASH_DRBG_SEEDLEN_BYTES];
	uint8_t		C[NIST_HASH_DRBG_SEEDLEN_BYTES];
	unsigned	reseed_counter;
};

typedef struct nist_hash_drbg	NIST_HASH_DRBG;

int	nist_hash_drbg_initialize(void); /* self-test */
int	nist_hash_drbg_instantiate(struct nist_hash_drbg *,
	    const void *, size_t, const void *, size_t, const void *, size_t);
int	nist_hash_drbg_reseed(struct nist_hash_drbg *,
	    const void *, size_t, const void *, size_t);
int	nist_hash_drbg_generate(struct nist_hash_drbg *, void *, size_t,
	    const void *, size_t);
int	nist_hash_drbg_destroy(struct nist_hash_drbg *);

#endif	/* NIST_HASH_DRBG_H */
