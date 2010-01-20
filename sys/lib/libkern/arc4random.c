/*	$NetBSD: arc4random.c,v 1.20.10.1 2010/01/20 05:29:25 snj Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#ifdef _KERNEL
#include "rnd.h"
#else
#define NRND 0
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#ifdef _KERNEL
#include <sys/kernel.h>
#endif
#include <sys/systm.h>

#include <lib/libkern/libkern.h>

#if NRND > 0
#include <sys/rnd.h>
#endif

#define	ARC4_MAXRUNS 16384
#define	ARC4_RESEED_SECONDS 300
#define	ARC4_KEYBYTES 32 /* 256 bit key */

#ifdef _STANDALONE
#define	time_uptime	1	/* XXX ugly! */
#endif /* _STANDALONE */

static u_int8_t arc4_i, arc4_j;
static int arc4_initialized = 0;
static int arc4_numruns = 0;
static u_int8_t arc4_sbox[256];
static time_t arc4_nextreseed;

static inline u_int8_t arc4_randbyte(void);

static inline void
arc4_swap(u_int8_t *a, u_int8_t *b)
{
	u_int8_t c;

	c = *a;
	*a = *b;
	*b = c;
}

/*
 * Stir our S-box.
 */
static void
arc4_randrekey(void)
{
	u_int8_t key[256];
	static int cur_keybytes;
	int n, byteswanted;
#if NRND > 0
	int r;
#endif

	if(!arc4_initialized)
		/* The first time through, we must take what we can get */
		byteswanted = 0;
	else
		/* Don't rekey with less entropy than we already have */
		byteswanted = cur_keybytes;

#if NRND > 0	/* XXX without rnd, we will key from the stack, ouch! */
	r = rnd_extract_data(key, ARC4_KEYBYTES, RND_EXTRACT_GOOD);

	if (r < ARC4_KEYBYTES) {
		if (r >= byteswanted) {
			(void)rnd_extract_data(key + r,
					       ARC4_KEYBYTES - r,
					       RND_EXTRACT_ANY);
		} else {
			/* don't replace a good key with a bad one! */
			arc4_nextreseed = time_uptime + ARC4_RESEED_SECONDS;
			arc4_numruns = 0;
			/* we should just ask rnd(4) to rekey us when
			   it can, but for now, we'll just try later. */
			return;
		}
	}

	cur_keybytes = r;

	for (n = ARC4_KEYBYTES; n < sizeof(key); n++)
			key[n] = key[n % ARC4_KEYBYTES];
#endif
	for (n = 0; n < 256; n++) {
		arc4_j = (arc4_j + arc4_sbox[n] + key[n]) % 256;
		arc4_swap(&arc4_sbox[n], &arc4_sbox[arc4_j]);
	}
	arc4_i = arc4_j;

	/* Reset for next reseed cycle. */
	arc4_nextreseed = time_uptime + ARC4_RESEED_SECONDS;
	arc4_numruns = 0;

	/*
	 * Throw away the first N words of output, as suggested in the
	 * paper "Weaknesses in the Key Scheduling Algorithm of RC4"
	 * by Fluher, Mantin, and Shamir.  (N = 256 in our case.)
	 */
	for (n = 0; n < 256 * 4; n++)
		arc4_randbyte();
}

/*
 * Initialize our S-box to its beginning defaults.
 */
static void
arc4_init(void)
{
	int n;

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

u_int32_t
arc4random(void)
{
	u_int32_t ret;
	int i;

	/* Initialize array if needed. */
	if (!arc4_initialized)
		arc4_init();

	if ((++arc4_numruns > ARC4_MAXRUNS) ||
	    (time_uptime > arc4_nextreseed)) {
		arc4_randrekey();
	}

	for (i = 0, ret = 0; i <= 24; ret |= arc4_randbyte() << i, i += 8)
		continue;
	return ret;
}

void
arc4randbytes(void *p, size_t len)
{
	u_int8_t *buf;
	size_t i;

	/* Initialize array if needed. */
	if (!arc4_initialized)
		arc4_init();

	buf = (u_int8_t *)p;

	for (i = 0; i < len; buf[i] = arc4_randbyte(), i++)
		continue;
	arc4_numruns += len / sizeof(u_int32_t);
	if ((arc4_numruns > ARC4_MAXRUNS) ||
	    (time_uptime > arc4_nextreseed)) {
		arc4_randrekey();
	}
}
