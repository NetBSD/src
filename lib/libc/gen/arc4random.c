/*	$NetBSD: arc4random.c,v 1.16 2012/08/18 15:04:53 dsl Exp $	*/
/*	$OpenBSD: arc4random.c,v 1.6 2001/06/05 05:05:38 pvalchev Exp $	*/

/*
 * Arc4 random number generator for OpenBSD.
 * Copyright 1996 David Mazieres <dm@lcs.mit.edu>.
 *
 * Modification and redistribution in source and binary forms is
 * permitted provided that due credit is given to the author and the
 * OpenBSD project by leaving this copyright notice intact.
 */

/*
 * This code is derived from section 17.1 of Applied Cryptography,
 * second edition, which describes a stream cipher allegedly
 * compatible with RSA Labs "RC4" cipher (the actual description of
 * which is a trade secret).  The same algorithm is used as a stream
 * cipher called "arcfour" in Tatu Ylonen's ssh package.
 *
 * Here the stream cipher has been modified always to include the time
 * when initializing the state.  That makes it impossible to
 * regenerate the same random sequence twice, so this can't be used
 * for encryption, but will generate good random numbers.
 *
 * RC4 is a registered trademark of RSA Laboratories.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: arc4random.c,v 1.16 2012/08/18 15:04:53 dsl Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include "reentrant.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/sysctl.h>

#ifdef __weak_alias
__weak_alias(arc4random,_arc4random)
#endif

#define RSIZE 256
struct arc4_stream {
	mutex_t mtx;
	int initialized;
	uint8_t i;
	uint8_t j;
	uint8_t s[RSIZE];
};

#ifdef _REENTRANT
#define LOCK(rs) { \
		int isthreaded = __isthreaded; \
		if (isthreaded)        \
			mutex_lock(&(rs)->mtx);
#define UNLOCK(rs) \
		if (isthreaded)        \
			mutex_unlock(&(rs)->mtx);      \
	}
#else
#define LOCK(rs) 
#define UNLOCK(rs)
#endif


/* XXX lint explodes with an internal error if only mtx is initialized! */
static struct arc4_stream rs = { .i = 0, .mtx = MUTEX_INITIALIZER };

static inline void arc4_addrandom(struct arc4_stream *, u_char *, int);
static void arc4_stir(struct arc4_stream *);
static inline uint8_t arc4_getbyte(struct arc4_stream *);
static inline uint32_t arc4_getword(struct arc4_stream *);

static __noinline void
arc4_init(struct arc4_stream *as)
{
	int n;
	for (n = 0; n < RSIZE; n++)
		as->s[n] = n;
	as->i = 0;
	as->j = 0;

	as->initialized = 1;
	arc4_stir(as);
}

static inline int
arc4_check_init(struct arc4_stream *as)
{
	if (__predict_true(rs.initialized))
		return 0;

	arc4_init(as);
	return 1;
}

static inline void
arc4_addrandom(struct arc4_stream *as, u_char *dat, int datlen)
{
	uint8_t si;
	int n;

	as->i--;
	for (n = 0; n < RSIZE; n++) {
		as->i = (as->i + 1);
		si = as->s[as->i];
		as->j = (as->j + si + dat[n % datlen]);
		as->s[as->i] = as->s[as->j];
		as->s[as->j] = si;
	}
	as->j = as->i;
}

static void
arc4_stir(struct arc4_stream *as)
{
	int rdat[32];
	int mib[] = { CTL_KERN, KERN_URND };
	size_t len;
	size_t i, j;

	/*
	 * This code once opened and read /dev/urandom on each
	 * call.  That causes repeated rekeying of the kernel stream
	 * generator, which is very wasteful.  Because of application
	 * behavior, caching the fd doesn't really help.  So we just
	 * fill up the tank from sysctl, which is a tiny bit slower
	 * for us but much friendlier to other entropy consumers.
	 */

	for (i = 0; i < __arraycount(rdat); i++) {
		len = sizeof(rdat[i]);
		if (sysctl(mib, 2, &rdat[i], &len, NULL, 0) == -1)
			abort();
	}

	arc4_addrandom(as, (void *) &rdat, (int)sizeof(rdat));

	/*
	 * Throw away the first N words of output, as suggested in the
	 * paper "Weaknesses in the Key Scheduling Algorithm of RC4"
	 * by Fluher, Mantin, and Shamir.  (N = 256 in our case.)
	 */
	for (j = 0; j < RSIZE * 4; j++)
		arc4_getbyte(as);
}

static inline uint8_t
arc4_getbyte(struct arc4_stream *as)
{
	uint8_t si, sj;

	as->i = (as->i + 1);
	si = as->s[as->i];
	as->j = (as->j + si);
	sj = as->s[as->j];
	as->s[as->i] = sj;
	as->s[as->j] = si;
	return (as->s[(si + sj) & 0xff]);
}

static inline uint32_t
arc4_getword(struct arc4_stream *as)
{
	uint32_t val;
	val = arc4_getbyte(as) << 24;
	val |= arc4_getbyte(as) << 16;
	val |= arc4_getbyte(as) << 8;
	val |= arc4_getbyte(as);
	return val;
}

void
arc4random_stir(void)
{
	LOCK(&rs);
	if (__predict_false(!arc4_check_init(&rs)))	/* init() stirs */
		arc4_stir(&rs);
	UNLOCK(&rs);
}

void
arc4random_addrandom(u_char *dat, int datlen)
{
	LOCK(&rs);
	arc4_check_init(&rs);
	arc4_addrandom(&rs, dat, datlen);
	UNLOCK(&rs);
}

uint32_t
arc4random(void)
{
	uint32_t v;

	LOCK(&rs);
	arc4_check_init(&rs);
	v = arc4_getword(&rs);
	UNLOCK(&rs);
	return v;
}

void
arc4random_buf(void *buf, size_t len)
{
	uint8_t *bp = buf;
	uint8_t *ep = bp + len;
	int skip;

	LOCK(&rs);
	arc4_check_init(&rs);

	skip = arc4_getbyte(&rs) % 3;
	while (skip--)
		(void)arc4_getbyte(&rs);

	while (bp < ep)
		*bp++ = arc4_getbyte(&rs);
	UNLOCK(&rs);
}

/*-
 * Written by Damien Miller.
 * With simplifications by Jinmei Tatuya.
 */

/*
 * Calculate a uniformly distributed random number less than
 * upper_bound avoiding "modulo bias".
 *
 * Uniformity is achieved by generating new random numbers
 * until the one returned is outside the range
 * [0, 2^32 % upper_bound[. This guarantees the selected
 * random number will be inside the range
 * [2^32 % upper_bound, 2^32[ which maps back to
 * [0, upper_bound[ after reduction modulo upper_bound.
 */
uint32_t
arc4random_uniform(uint32_t upper_bound)
{
	uint32_t r, min;

	if (upper_bound < 2)
		return 0;

	/* calculate (2^32 % upper_bound) avoiding 64-bit math */
	/* ((2^32 - x) % x) == (2^32 % x) when x <= 2^31 */
	min = (0xFFFFFFFFU - upper_bound + 1) % upper_bound;

	LOCK(&rs);
	arc4_check_init(&rs);

	if (arc4_getbyte(&rs) & 1)
		(void)arc4_getbyte(&rs);

	/*
	 * This could theoretically loop forever but each retry has
	 * p > 0.5 (worst case, usually far better) of selecting a
	 * number inside the range we need, so it should rarely need
	 * to re-roll (at all).
	 */
	do
		r = arc4_getword(&rs);
	while (r < min);
	UNLOCK(&rs);

	return r % upper_bound;
}
