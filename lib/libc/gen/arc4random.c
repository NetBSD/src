/*	$NetBSD: arc4random.c,v 1.20.2.1 2014/08/20 00:02:14 tls Exp $	*/
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
__RCSID("$NetBSD: arc4random.c,v 1.20.2.1 2014/08/20 00:02:14 tls Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include "reentrant.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/sysctl.h>

#ifdef __weak_alias
__weak_alias(arc4random,_arc4random)
__weak_alias(arc4random_addrandom,_arc4random_addrandom)
__weak_alias(arc4random_buf,_arc4random_buf)
__weak_alias(arc4random_stir,_arc4random_stir)
__weak_alias(arc4random_uniform,_arc4random_uniform)
#endif

#define REKEY_BYTES	1600000

struct arc4_stream {
	bool inited;
	uint8_t i;
	uint8_t j;
	uint8_t s[(uint8_t)~0u + 1u];	/* 256 to you and me */
	size_t count;
	mutex_t mtx;
};

#ifdef _REENTRANT
#define LOCK(rs)	do { \
				if (__isthreaded) mutex_lock(&(rs)->mtx); \
			} while (/*CONSTCOND*/ 0)
#define UNLOCK(rs)	do { \
				if (__isthreaded) mutex_unlock(&(rs)->mtx); \
			} while (/*CONSTCOND*/ 0)
#else
#define LOCK(rs)
#define UNLOCK(rs)
#endif

#define S(n) (n)
#define S4(n) S(n), S(n + 1), S(n + 2), S(n + 3)
#define S16(n) S4(n), S4(n + 4), S4(n + 8), S4(n + 12)
#define S64(n) S16(n), S16(n + 16), S16(n + 32), S16(n + 48)
#define S256 S64(0), S64(64), S64(128), S64(192)

static struct arc4_stream rs = { .inited = false,
		.i = 0xff, .j = 0, .s = { S256 },
		.count = 0, .mtx = MUTEX_INITIALIZER };

#undef S
#undef S4
#undef S16
#undef S64
#undef S256

static inline void arc4_addrandom(struct arc4_stream *, u_char *, int);
static __noinline void arc4_stir(struct arc4_stream *);
static inline uint8_t arc4_getbyte(struct arc4_stream *);
static inline uint32_t arc4_getword(struct arc4_stream *);

#ifdef _REENTRANT
static void
arc4_fork_prepare(void)
{

	LOCK(&rs);
}

static void
arc4_fork_parent(void)
{

	UNLOCK(&rs);
}
#else
#define arc4_fork_prepare	NULL
#define arc4_fork_parent	NULL
#endif

static void
arc4_fork_child(void)
{

	/* Reset the counter to a force new stir after forking */
	rs.count = 0;
	UNLOCK(&rs);
}

static inline void
arc4_check_init(struct arc4_stream *as)
{
	/*
	 * pthread_atfork(3) only allows async-signal-safe functions in
	 * the child handler.
	 * NetBSD's mutex_unlock is async-signal safe, other implementations
	 * may not be.
	 */

	if (__predict_false(!as->inited)) {
		as->inited = true;
		pthread_atfork(arc4_fork_prepare,
		    arc4_fork_parent, arc4_fork_child);
	}
}

static inline void
arc4_addrandom(struct arc4_stream *as, u_char *dat, int datlen)
{
	uint8_t si;
	size_t n;

	for (n = 0; n < __arraycount(as->s); n++) {
		as->i = (as->i + 1);
		si = as->s[as->i];
		as->j = (as->j + si + dat[n % datlen]);
		as->s[as->i] = as->s[as->j];
		as->s[as->j] = si;
	}
}

static __noinline void
arc4_stir(struct arc4_stream *as)
{
	int rdat[32];
	int mib[] = { CTL_KERN, KERN_URND };
	size_t len;
	size_t i, j;

	arc4_check_init(as);

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
	for (j = 0; j < __arraycount(as->s) * sizeof(uint32_t); j++)
		arc4_getbyte(as);

	/* Stir again after REKEY_BYTES bytes, or if the pid changes */
	as->count = REKEY_BYTES;
}

static inline void
arc4_stir_if_needed(struct arc4_stream *as, size_t len)
{

	if (__predict_false(as->count <= len))
		arc4_stir(as);
	else
		as->count -= len;
}

static __inline uint8_t
arc4_getbyte_ij(struct arc4_stream *as, uint8_t *i, uint8_t *j)
{
	uint8_t si, sj;

	*i = *i + 1;
	si = as->s[*i];
	*j = *j + si;
	sj = as->s[*j];
	as->s[*i] = sj;
	as->s[*j] = si;
	return (as->s[(si + sj) & 0xff]);
}

static inline uint8_t
arc4_getbyte(struct arc4_stream *as)
{

	return arc4_getbyte_ij(as, &as->i, &as->j);
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
	arc4_stir(&rs);
	UNLOCK(&rs);
}

void
arc4random_addrandom(u_char *dat, int datlen)
{

	LOCK(&rs);
	arc4_stir_if_needed(&rs, datlen);
	arc4_addrandom(&rs, dat, datlen);
	UNLOCK(&rs);
}

uint32_t
arc4random(void)
{
	uint32_t v;

	LOCK(&rs);
	arc4_stir_if_needed(&rs, sizeof(v));
	v = arc4_getword(&rs);
	UNLOCK(&rs);
	return v;
}

void
arc4random_buf(void *buf, size_t len)
{
	uint8_t *bp = buf;
	uint8_t *ep = bp + len;
	uint8_t i, j;

	LOCK(&rs);
	arc4_stir_if_needed(&rs, len);

	/* cache i and j - compiler can't know 'buf' doesn't alias them */
	i = rs.i;
	j = rs.j;

	while (bp < ep)
		*bp++ = arc4_getbyte_ij(&rs, &i, &j);
	rs.i = i;
	rs.j = j;

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
	arc4_stir_if_needed(&rs, sizeof(r));

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
