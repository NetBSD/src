/*	$NetBSD: arc4random.c,v 1.35 2013/06/24 04:21:20 riastradh Exp $	*/

/*-
 * Copyright (c) 2002, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Thor Lancelot Simon.
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

/*-
 * THE BEER-WARE LICENSE
 *
 * <dan@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff.  If we meet some day, and you
 * think this stuff is worth it, you can buy me a beer in return.
 *
 * Dan Moschuk
 *
 * $FreeBSD: src/sys/libkern/arc4random.c,v 1.9 2001/08/30 12:30:58 bde Exp $
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/rngtest.h>
#include <sys/systm.h>
#include <sys/time.h>

#ifdef _STANDALONE
/*
 * XXX This is a load of bollocks.  Standalone has no entropy source.
 * This module should be removed from libkern once we confirm nobody is
 * using it.
 */
#define	time_uptime	1
typedef struct kmutex	*kmutex_t;
#define	MUTEX_DEFAULT	0
#define	IPL_VM		0
static void mutex_init(kmutex_t *m, int t, int i) {}
static void mutex_spin_enter(kmutex_t *m) {}
static void mutex_spin_exit(kmutex_t *m) {}
typedef void rndsink_callback_t(void *, const void *, size_t);
struct rndsink;
static struct rndsink *rndsink_create(size_t n, rndsink_callback_t c, void *a)
  { return NULL; }
static bool rndsink_request(struct rndsink *s, void *b, size_t n)
  { return true; }
#else  /* !_STANDALONE */
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/rndsink.h>
#endif /* _STANDALONE */

#include <lib/libkern/libkern.h>

/*
 * The best known attack that distinguishes RC4 output from a random
 * bitstream requires 2^25 bytes.  (see Paul and Preneel, Analysis of
 * Non-fortuitous Predictive States of the RC4 Keystream Generator.
 * INDOCRYPT 2003, pp52 – 67).
 *
 * However, we discard the first 1024 bytes of output, avoiding the
 * biases detected in this paper.  The best current attack that
 * can distinguish this "RC4[drop]" output seems to be Fleuhrer &
 * McGrew's attack which requires 2^30.6 bytes of output:
 * Fluhrer and McGrew, Statistical Analysis of the Alleged RC4
 * Keystream Generator. FSE 2000, pp19 – 30
 *
 * We begin trying to rekey at 2^24 bytes, and forcibly rekey at 2^29 bytes
 * even if the resulting key cannot be guaranteed to have full entropy.
 */
#define	ARC4_MAXBYTES		(16 * 1024 * 1024)
#define ARC4_HARDMAX		(512 * 1024 * 1024)
#define	ARC4_RESEED_SECONDS	300
#define	ARC4_KEYBYTES		16 /* 128 bit key */

static kmutex_t	arc4_mtx;
static struct rndsink *arc4_rndsink;

static u_int8_t arc4_i, arc4_j;
static int arc4_initialized = 0;
static int arc4_numbytes = 0;
static u_int8_t arc4_sbox[256];
static time_t arc4_nextreseed;

static rndsink_callback_t arc4_rndsink_callback;
static void arc4_randrekey(void);
static void arc4_randrekey_from(const uint8_t[ARC4_KEYBYTES], bool);
static void arc4_init(void);
static inline u_int8_t arc4_randbyte(void);
static inline void arc4randbytes_unlocked(void *, size_t);
void _arc4randbytes(void *, size_t);
uint32_t _arc4random(void);

static inline void
arc4_swap(u_int8_t *a, u_int8_t *b)
{
	u_int8_t c;

	c = *a;
	*a = *b;
	*b = c;
}

static void
arc4_rndsink_callback(void *context __unused, const void *seed, size_t bytes)
{

	KASSERT(bytes == ARC4_KEYBYTES);
	arc4_randrekey_from(seed, true);
}

/*
 * Stir our S-box with whatever we can get from the system entropy pool
 * now.
 */
static void
arc4_randrekey(void)
{
	uint8_t seed[ARC4_KEYBYTES];

	const bool full_entropy = rndsink_request(arc4_rndsink, seed,
	    sizeof(seed));
	arc4_randrekey_from(seed, full_entropy);
	explicit_memset(seed, 0, sizeof(seed));
}

/*
 * Stir our S-box with what's in seed.
 */
static void
arc4_randrekey_from(const uint8_t seed[ARC4_KEYBYTES], bool full_entropy)
{
	uint8_t key[256];
	size_t n;

	mutex_spin_enter(&arc4_mtx);

	(void)memcpy(key, seed, ARC4_KEYBYTES);

	/* Rekey the arc4 state.  */
	for (n = ARC4_KEYBYTES; n < sizeof(key); n++)
		key[n] = key[n % ARC4_KEYBYTES];

	for (n = 0; n < 256; n++) {
		arc4_j = (arc4_j + arc4_sbox[n] + key[n]) % 256;
		arc4_swap(&arc4_sbox[n], &arc4_sbox[arc4_j]);
	}
	arc4_i = arc4_j;

	explicit_memset(key, 0, sizeof(key));

	/*
	 * Throw away the first N words of output, as suggested in the
	 * paper "Weaknesses in the Key Scheduling Algorithm of RC4" by
	 * Fluher, Mantin, and Shamir.  (N = 256 in our case.)
	 */
	for (n = 0; n < 256 * 4; n++)
		arc4_randbyte();

	/*
	 * Reset for next reseed cycle.  If we don't have full entropy,
	 * caller has scheduled a reseed already.
	 */
	arc4_nextreseed = time_uptime +
	    (full_entropy? ARC4_RESEED_SECONDS : 0);
	arc4_numbytes = 0;

#if 0				/* XXX */
	arc4_rngtest();
#endif

	mutex_spin_exit(&arc4_mtx);
}

/*
 * Initialize our S-box to its beginning defaults.
 */
static void
arc4_init(void)
{
	int n;

	mutex_init(&arc4_mtx, MUTEX_DEFAULT, IPL_VM);
	arc4_rndsink = rndsink_create(ARC4_KEYBYTES, &arc4_rndsink_callback,
	    NULL);

	arc4_i = arc4_j = 0;
	for (n = 0; n < 256; n++)
		arc4_sbox[n] = (u_int8_t) n;

	arc4_randrekey();
	arc4_initialized = 1;
}

/*
 * Generate a random byte.
 */
static inline u_int8_t
arc4_randbyte(void)
{
	u_int8_t arc4_t;

	arc4_i = (arc4_i + 1) % 256;
	arc4_j = (arc4_j + arc4_sbox[arc4_i]) % 256;

	arc4_swap(&arc4_sbox[arc4_i], &arc4_sbox[arc4_j]);

	arc4_t = (arc4_sbox[arc4_i] + arc4_sbox[arc4_j]) % 256;
	return arc4_sbox[arc4_t];
}

static inline void
arc4randbytes_unlocked(void *p, size_t len)
{
	u_int8_t *buf = (u_int8_t *)p;
	size_t i;

	for (i = 0; i < len; buf[i] = arc4_randbyte(), i++)
		continue;
}

void
_arc4randbytes(void *p, size_t len)
{
	/* Initialize array if needed. */
	if (!arc4_initialized) {
		arc4_init();
		/* avoid conditionalizing locking */
		arc4randbytes_unlocked(p, len);
		arc4_numbytes += len;
		return;
	}
	mutex_spin_enter(&arc4_mtx);
	arc4randbytes_unlocked(p, len);
	arc4_numbytes += len;
	mutex_spin_exit(&arc4_mtx);
	if ((arc4_numbytes > ARC4_MAXBYTES) ||
	    (time_uptime > arc4_nextreseed)) {
		arc4_randrekey();
	}
}

u_int32_t
_arc4random(void)
{
        u_int32_t ret;
        u_int8_t *retc;

        retc = (u_int8_t *)&ret;

        _arc4randbytes(retc, sizeof(u_int32_t));
        return ret;
}
