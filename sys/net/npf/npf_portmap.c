/*-
 * Copyright (c) 2019 Mindaugas Rasiukevicius <rmind at noxt eu>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * NPF port map mechanism.
 *
 *	The port map is a bitmap used to track TCP/UDP ports used for
 *	translation.  Port maps are per IP addresses, therefore multiple
 *	NAT policies operating on the same IP address will share the
 *	same port map.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_portmap.c,v 1.7 2020/08/28 06:35:50 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/bitops.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/cprng.h>
#include <sys/thmap.h>
#endif

#include "npf_impl.h"

/*
 * Port map uses two-level bitmaps with compression to efficiently
 * represent the maximum of 65536 (2^16) values.
 *
 * Level 0: 64 chunks each representing 1048 bits in two modes:
 *
 *	a) If PORTMAP_L1_TAG, then up to 5 values are packed in the
 *	64-bit integer using 12 bits for each value, starting from the
 *	most significant bits.  The four 4 least significant bits are
 *	unused or reserved for pointer tagging.
 *
 *	b) If there are more than 5 values, then PORTMAP_L1_TAG is set
 *	and the value serves as a pointer to the second level bitmap.
 *
 * Level 1: 16 chunks each representing 64 bits in plain uint64_t.
 */

#define	PORTMAP_MAX_BITS	(65536U)
#define	PORTMAP_MASK		(PORTMAP_MAX_BITS - 1)

#define	PORTMAP_L0_SHIFT	(10) // or 11
#define	PORTMAP_L0_MASK		((1U << PORTMAP_L0_SHIFT) - 1)
#define	PORTMAP_L0_WORDS	(PORTMAP_MAX_BITS >> PORTMAP_L0_SHIFT)

#define	PORTMAP_L1_SHIFT	(6)
#define	PORTMAP_L1_MASK		((1U << PORTMAP_L1_SHIFT) - 1)
#define	PORTMAP_L1_WORDS	\
    ((PORTMAP_MAX_BITS / PORTMAP_L0_WORDS) >> PORTMAP_L1_SHIFT)

#define	PORTMAP_L1_TAG		(UINT64_C(1)) // use level 1
#define	PORTMAP_L1_GET(p)	((void *)((uintptr_t)(p) & ~(uintptr_t)3))

CTASSERT(sizeof(uint64_t) >= sizeof(uintptr_t));

typedef struct {
	volatile uint64_t	bits1[PORTMAP_L1_WORDS];
} bitmap_l1_t;

typedef struct bitmap {
	npf_addr_t		addr;
	volatile uint64_t	bits0[PORTMAP_L0_WORDS];
	LIST_ENTRY(bitmap)	entry;
	unsigned		addr_len;
} bitmap_t;

#define	NPF_PORTMAP_MINPORT	1024
#define	NPF_PORTMAP_MAXPORT	65535

struct npf_portmap {
	thmap_t	*		addr_map;
	LIST_HEAD(, bitmap)	bitmap_list;
	kmutex_t		list_lock;
	int			min_port;
	int			max_port;
};

static kmutex_t			portmap_lock;

void
npf_portmap_sysinit(void)
{

	mutex_init(&portmap_lock, MUTEX_DEFAULT, IPL_SOFTNET);
}

void
npf_portmap_sysfini(void)
{

	mutex_destroy(&portmap_lock);
}

void
npf_portmap_init(npf_t *npf)
{
	npf_portmap_t *pm = npf_portmap_create(
	    NPF_PORTMAP_MINPORT, NPF_PORTMAP_MAXPORT);
	npf_param_t param_map[] = {
		{
			"portmap.min_port",
			&pm->min_port,
			.default_val = NPF_PORTMAP_MINPORT,
			.min = 1024, .max = 65535
		},
		{
			"portmap.max_port",
			&pm->max_port,
			.default_val = 49151, // RFC 6335
			.min = 1024, .max = 65535
		}
	};

	npf_param_register(npf, param_map, __arraycount(param_map));
	npf->portmap = pm;
}

void
npf_portmap_fini(npf_t *npf)
{

	npf_portmap_destroy(npf->portmap);
	npf->portmap = NULL; // diagnostic
}

npf_portmap_t *
npf_portmap_create(int min_port, int max_port)
{
	npf_portmap_t *pm;

	pm = kmem_zalloc(sizeof(npf_portmap_t), KM_SLEEP);
	mutex_init(&pm->list_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	pm->addr_map = thmap_create(0, NULL, THMAP_NOCOPY);
	pm->min_port = min_port;
	pm->max_port = max_port;
	return pm;
}

void
npf_portmap_destroy(npf_portmap_t *pm)
{
	npf_portmap_flush(pm);
	KASSERT(LIST_EMPTY(&pm->bitmap_list));

	thmap_destroy(pm->addr_map);
	mutex_destroy(&pm->list_lock);
	kmem_free(pm, sizeof(npf_portmap_t));
}

/////////////////////////////////////////////////////////////////////////

#if defined(_LP64)
#define	__npf_atomic_cas_64	atomic_cas_64
#else
static uint64_t
__npf_atomic_cas_64(volatile uint64_t *ptr, uint64_t old, uint64_t new)
{
	uint64_t prev;

	mutex_enter(&portmap_lock);
	prev = *ptr;
	if (prev == old) {
		*ptr = new;
	}
	mutex_exit(&portmap_lock);

	return prev;
}
#endif

/*
 * bitmap_word_isset: test whether the bit value is in the packed array.
 *
 * => Return true if any value equals the bit number value.
 *
 * Packed array: 60 MSB bits, 5 values, 12 bits each.
 *
 * Reference: "Bit Twiddling Hacks" by S.E. Anderson, Stanford.
 * Based on the hasvalue() and haszero() ideas.  Since values are
 * represented by upper 60 bits, we shift right by 4.
 */
static bool
bitmap_word_isset(uint64_t x, unsigned bit)
{
	uint64_t m, r;

	bit++;
	KASSERT((x & PORTMAP_L1_TAG) == 0);
	KASSERT(bit <= (PORTMAP_L0_MASK + 1));

	m = (x >> 4) ^ (UINT64_C(0x1001001001001) * bit);
	r = (m - UINT64_C(0x1001001001001)) & (~m & UINT64_C(0x800800800800800));
	return r != 0;
}

/*
 * bitmap_word_cax: compare-and-xor on packed array elements.
 */
static uint64_t
bitmap_word_cax(uint64_t x, int exp, int bit)
{
	unsigned e = exp + 1;

	/*
	 * We need to distinguish "no value" from zero.  Just add one,
	 * since we use 12 bits to represent 11 bit values.
	 */
	bit++;
	KASSERT((unsigned)bit <= (PORTMAP_L0_MASK + 1));
	KASSERT((x & PORTMAP_L1_TAG) == 0);

	if (((x >> 52) & 0xfff) == e)
		return x ^ ((uint64_t)bit << 52);
	if (((x >> 40) & 0xfff) == e)
		return x ^ ((uint64_t)bit << 40);
	if (((x >> 28) & 0xfff) == e)
		return x ^ ((uint64_t)bit << 28);
	if (((x >> 16) & 0xfff) == e)
		return x ^ ((uint64_t)bit << 16);
	if (((x >>  4) & 0xfff) == e)
		return x ^ ((uint64_t)bit << 4);
	return 0;
}

static unsigned
bitmap_word_unpack(uint64_t x, unsigned bitvals[static 5])
{
	unsigned n = 0;
	uint64_t v;

	KASSERT((x & PORTMAP_L1_TAG) == 0);

	if ((v = ((x >> 52)) & 0xfff) != 0)
		bitvals[n++] = v - 1;
	if ((v = ((x >> 40)) & 0xfff) != 0)
		bitvals[n++] = v - 1;
	if ((v = ((x >> 28)) & 0xfff) != 0)
		bitvals[n++] = v - 1;
	if ((v = ((x >> 16)) & 0xfff) != 0)
		bitvals[n++] = v - 1;
	if ((v = ((x >>  4)) & 0xfff) != 0)
		bitvals[n++] = v - 1;
	return n;
}

#if 0
static bool
bitmap_isset(const bitmap_t *bm, unsigned bit)
{
	unsigned i, chunk_bit;
	uint64_t bval, b;
	bitmap_l1_t *bm1;

	KASSERT(bit < PORTMAP_MAX_BITS);
	i = bit >> PORTMAP_L0_SHIFT;
	bval = atomic_load_relaxed(&bm->bits0[i]);

	/*
	 * Empty check.  Note: we can test the whole word against zero,
	 * since zero bit values in the packed array result in bits set.
	 */
	if (bval == 0)
		return false;

	/* Level 0 check. */
	chunk_bit = bit & PORTMAP_L0_MASK;
	if ((bval & PORTMAP_L1_TAG) == 0)
		return bitmap_word_isset(bval, chunk_bit);

	/* Level 1 check. */
	bm1 = PORTMAP_L1_GET(bval);
	KASSERT(bm1 != NULL);
	i = chunk_bit >> PORTMAP_L1_SHIFT;
	b = UINT64_C(1) << (chunk_bit & PORTMAP_L1_MASK);
	return (bm1->bits1[i] & b) != 0;
}
#endif

static bool
bitmap_set(bitmap_t *bm, unsigned bit)
{
	unsigned i, chunk_bit;
	uint64_t bval, b, oval, nval;
	bitmap_l1_t *bm1;
again:
	KASSERT(bit < PORTMAP_MAX_BITS);
	i = bit >> PORTMAP_L0_SHIFT;
	chunk_bit = bit & PORTMAP_L0_MASK;
	bval = bm->bits0[i];

	if ((bval & PORTMAP_L1_TAG) == 0) {
		unsigned n = 0, bitvals[5];
		uint64_t bm1p;

		if (bitmap_word_isset(bval, chunk_bit)) {
			return false;
		}

		/*
		 * Look for a zero-slot and put a value there.
		 */
		if ((nval = bitmap_word_cax(bval, -1, chunk_bit)) != 0) {
			KASSERT((nval & PORTMAP_L1_TAG) == 0);
			if (__npf_atomic_cas_64(&bm->bits0[i], bval, nval) != bval) {
				goto again;
			}
			return true;
		}

		/*
		 * Full: allocate L1 block and copy over the current
		 * values into the level.
		 */
		bm1 = kmem_intr_zalloc(sizeof(bitmap_l1_t), KM_NOSLEEP);
		if (bm1 == NULL) {
			return false; // error
		}
		n = bitmap_word_unpack(bval, bitvals);
		while (n--) {
			const unsigned v = bitvals[n];
			const unsigned off = v >> PORTMAP_L1_SHIFT;

			KASSERT(v <= PORTMAP_L0_MASK);
			KASSERT(off < (sizeof(uint64_t) * CHAR_BIT));
			bm1->bits1[off] |= UINT64_C(1) << (v & PORTMAP_L1_MASK);
		}

		/*
		 * Attempt to set the L1 structure.  Note: there is no
		 * ABA problem since the we compare the actual values.
		 * Note: CAS serves as a memory barrier.
		 */
		bm1p = (uintptr_t)bm1;
		KASSERT((bm1p & PORTMAP_L1_TAG) == 0);
		bm1p |= PORTMAP_L1_TAG;
		if (__npf_atomic_cas_64(&bm->bits0[i], bval, bm1p) != bval) {
			kmem_intr_free(bm1, sizeof(bitmap_l1_t));
			goto again;
		}
		bval = bm1p;
	}

	bm1 = PORTMAP_L1_GET(bval);
	KASSERT(bm1 != NULL);
	i = chunk_bit >> PORTMAP_L1_SHIFT;
	b = UINT64_C(1) << (chunk_bit & PORTMAP_L1_MASK);

	oval = bm1->bits1[i];
	if (oval & b) {
		return false;
	}
	nval = oval | b;
	if (__npf_atomic_cas_64(&bm1->bits1[i], oval, nval) != oval) {
		goto again;
	}
	return true;
}

static bool
bitmap_clr(bitmap_t *bm, unsigned bit)
{
	unsigned i, chunk_bit;
	uint64_t bval, b, oval, nval;
	bitmap_l1_t *bm1;
again:
	KASSERT(bit < PORTMAP_MAX_BITS);
	i = bit >> PORTMAP_L0_SHIFT;
	chunk_bit = bit & PORTMAP_L0_MASK;
	bval = bm->bits0[i];

	if ((bval & PORTMAP_L1_TAG) == 0) {
		if (!bitmap_word_isset(bval, chunk_bit)) {
			return false;
		}
		nval = bitmap_word_cax(bval, chunk_bit, chunk_bit);
		KASSERT((nval & PORTMAP_L1_TAG) == 0);
		if (__npf_atomic_cas_64(&bm->bits0[i], bval, nval) != bval) {
			goto again;
		}
		return true;
	}

	bm1 = PORTMAP_L1_GET(bval);
	KASSERT(bm1 != NULL);
	i = chunk_bit >> PORTMAP_L1_SHIFT;
	b = UINT64_C(1) << (chunk_bit & PORTMAP_L1_MASK);

	oval = bm1->bits1[i];
	if ((oval & b) == 0) {
		return false;
	}
	nval = oval & ~b;
	if (__npf_atomic_cas_64(&bm1->bits1[i], oval, nval) != oval) {
		goto again;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////

static bitmap_t *
npf_portmap_autoget(npf_portmap_t *pm, unsigned alen, const npf_addr_t *addr)
{
	bitmap_t *bm;

	KASSERT(pm && pm->addr_map);
	KASSERT(alen && alen <= sizeof(npf_addr_t));

	/* Lookup the port map for this address. */
	bm = thmap_get(pm->addr_map, addr, alen);
	if (bm == NULL) {
		void *ret;

		/*
		 * Allocate a new port map for this address and
		 * attempt to insert it.
		 */
		bm = kmem_intr_zalloc(sizeof(bitmap_t), KM_NOSLEEP);
		if (bm == NULL) {
			return NULL;
		}
		memcpy(&bm->addr, addr, alen);
		bm->addr_len = alen;

		int s = splsoftnet();
		ret = thmap_put(pm->addr_map, &bm->addr, alen, bm);
		splx(s);

		if (ret == bm) {
			/* Success: insert the bitmap into the list. */
			mutex_enter(&pm->list_lock);
			LIST_INSERT_HEAD(&pm->bitmap_list, bm, entry);
			mutex_exit(&pm->list_lock);
		} else {
			/* Race: use an existing bitmap. */
			kmem_free(bm, sizeof(bitmap_t));
			bm = ret;
		}
	}
	return bm;
}

/*
 * npf_portmap_flush: free all bitmaps and remove all addresses.
 *
 * => Concurrent calls to this routine are not allowed; therefore no
 * need to acquire locks.
 */
void
npf_portmap_flush(npf_portmap_t *pm)
{
	bitmap_t *bm;

	while ((bm = LIST_FIRST(&pm->bitmap_list)) != NULL) {
		for (unsigned i = 0; i < PORTMAP_L0_WORDS; i++) {
			uintptr_t bm1 = bm->bits0[i];

			if (bm1 & PORTMAP_L1_TAG) {
				bitmap_l1_t *bm1p = PORTMAP_L1_GET(bm1);
				kmem_intr_free(bm1p, sizeof(bitmap_l1_t));
			}
			bm->bits0[i] = UINT64_C(0);
		}
		LIST_REMOVE(bm, entry);
		thmap_del(pm->addr_map, &bm->addr, bm->addr_len);
		kmem_intr_free(bm, sizeof(bitmap_t));
	}
	/* Note: the caller ensures there are no active references. */
	thmap_gc(pm->addr_map, thmap_stage_gc(pm->addr_map));
}

/*
 * npf_portmap_get: allocate and return a port from the given portmap.
 *
 * => Returns the port value in network byte-order.
 * => Zero indicates a failure.
 */
in_port_t
npf_portmap_get(npf_portmap_t *pm, int alen, const npf_addr_t *addr)
{
	const unsigned min_port = atomic_load_relaxed(&pm->min_port);
	const unsigned max_port = atomic_load_relaxed(&pm->max_port);
	const unsigned port_delta = max_port - min_port + 1;
	unsigned bit, target;
	bitmap_t *bm;

	/* Sanity check: the user might set incorrect parameters. */
	if (__predict_false(min_port > max_port)) {
		return 0;
	}

	bm = npf_portmap_autoget(pm, alen, addr);
	if (__predict_false(bm == NULL)) {
		/* No memory. */
		return 0;
	}

	/* Randomly select a port. */
	target = min_port + (cprng_fast32() % port_delta);
	bit = target;
next:
	if (bitmap_set(bm, bit)) {
		/* Success. */
		return htons(bit);
	}
	bit = min_port + ((bit + 1) % port_delta);
	if (target != bit) {
		/* Next.. */
		goto next;
	}
	/* No space. */
	return 0;
}

/*
 * npf_portmap_take: allocate a specific port in the portmap.
 */
bool
npf_portmap_take(npf_portmap_t *pm, int alen,
    const npf_addr_t *addr, in_port_t port)
{
	bitmap_t *bm = npf_portmap_autoget(pm, alen, addr);

	port = ntohs(port);
	if (!bm || port < pm->min_port || port > pm->max_port) {
		/* Out of memory / invalid port. */
		return false;
	}
	return bitmap_set(bm, port);
}

/*
 * npf_portmap_put: release the port, making it available in the portmap.
 *
 * => The port value should be in network byte-order.
 */
void
npf_portmap_put(npf_portmap_t *pm, int alen,
    const npf_addr_t *addr, in_port_t port)
{
	bitmap_t *bm;

	bm = npf_portmap_autoget(pm, alen, addr);
	if (bm) {
		port = ntohs(port);
		bitmap_clr(bm, port);
	}
}
