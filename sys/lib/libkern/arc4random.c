/*	$NetBSD: arc4random.c,v 1.4 2002/06/14 03:05:46 itojun Exp $	*/

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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <lib/libkern/libkern.h>

#if NRND > 0
#include <dev/rnd.h>
#endif

#define	ARC4_MAXRUNS 16384
#define	ARC4_RESEED_SECONDS 300
#define	ARC4_KEYBYTES 32 /* 256 bit key */

static u_int8_t arc4_i, arc4_j;
static int arc4_initialized = 0;
static int arc4_numruns = 0;
static u_int8_t arc4_sbox[256];
static struct timeval arc4_tv_nextreseed;

static u_int8_t arc4_randbyte(void);

static __inline void
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
arc4_randomstir(void)
{
	u_int8_t key[256];
	int r, n;

#if NRND > 0
	r = rnd_extract_data(key, ARC4_KEYBYTES, RND_EXTRACT_ANY);
#else
	r = 0;	/*XXX*/
#endif
	/* If r == 0 || -1, just use what was on the stack. */
	if (r > 0) {
		for (n = r; n < sizeof(key); n++)
			key[n] = key[n % r];
	}

	for (n = 0; n < 256; n++) {
		arc4_j = (arc4_j + arc4_sbox[n] + key[n]) % 256;
		arc4_swap(&arc4_sbox[n], &arc4_sbox[arc4_j]);
	}

	/* Reset for next reseed cycle. */
	arc4_tv_nextreseed = mono_time;
	arc4_tv_nextreseed.tv_sec += ARC4_RESEED_SECONDS;
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

	arc4_randomstir();
	arc4_initialized = 1;
}

/*
 * Generate a random byte.
 */
static u_int8_t
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

	/* Initialize array if needed. */
	if (!arc4_initialized)
		arc4_init();

	if ((++arc4_numruns > ARC4_MAXRUNS) || 
	    (mono_time.tv_sec > arc4_tv_nextreseed.tv_sec)) {
		arc4_randomstir();
	}

	ret = arc4_randbyte();
	ret |= arc4_randbyte() << 8;
	ret |= arc4_randbyte() << 16;
	ret |= arc4_randbyte() << 24;

	return ret;
}
