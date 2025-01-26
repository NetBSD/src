/*	$NetBSD: histo.c,v 1.2 2025/01/26 16:25:37 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <isc/atomic.h>
#include <isc/histo.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/tid.h>

#define HISTO_MAGIC	    ISC_MAGIC('H', 's', 't', 'o')
#define HISTO_VALID(p)	    ISC_MAGIC_VALID(p, HISTO_MAGIC)
#define HISTOMULTI_MAGIC    ISC_MAGIC('H', 'g', 'M', 't')
#define HISTOMULTI_VALID(p) ISC_MAGIC_VALID(p, HISTOMULTI_MAGIC)

/*
 * Natural logarithms of 2 and 10 for converting precisions between
 * binary and decimal significant figures
 */
#define LN_2  0.693147180559945309
#define LN_10 2.302585092994045684

/*
 * The chunks array has a static size for simplicity, fixed as the
 * number of bits in a value. That means we waste a little extra space
 * that could be saved by omitting the exponents that are covered by
 * `sigbits`. The following macros calculate (at run time) the exact
 * number of buckets when we need to do accurate bounds checks.
 *
 * For a discussion of the floating point terminology, see the
 * commmentary on `value_to_key()` below.
 *
 * We often use the variable names `c` for chunk and `b` for bucket.
 */
#define CHUNKS 64

#define DENORMALS(hg) ((hg)->sigbits - 1)
#define MANTISSAS(hg) (1 << (hg)->sigbits)
#define EXPONENTS(hg) (CHUNKS - DENORMALS(hg))
#define BUCKETS(hg)   (EXPONENTS(hg) * MANTISSAS(hg))

#define MAXCHUNK(hg)  EXPONENTS(hg)
#define CHUNKSIZE(hg) MANTISSAS(hg)

typedef atomic_uint_fast64_t hg_bucket_t;
typedef atomic_ptr(hg_bucket_t) hg_chunk_t;

struct isc_histo {
	uint magic;
	uint sigbits;
	isc_mem_t *mctx;
	hg_chunk_t chunk[CHUNKS];
};

struct isc_histomulti {
	uint magic;
	uint size;
	isc_histo_t *hg[];
};

/**********************************************************************/

void
isc_histo_create(isc_mem_t *mctx, uint sigbits, isc_histo_t **hgp) {
	REQUIRE(sigbits >= ISC_HISTO_MINBITS);
	REQUIRE(sigbits <= ISC_HISTO_MAXBITS);
	REQUIRE(hgp != NULL);
	REQUIRE(*hgp == NULL);

	isc_histo_t *hg = isc_mem_get(mctx, sizeof(*hg));
	*hg = (isc_histo_t){
		.magic = HISTO_MAGIC,
		.sigbits = sigbits,
	};
	isc_mem_attach(mctx, &hg->mctx);

	*hgp = hg;
}

void
isc_histo_destroy(isc_histo_t **hgp) {
	REQUIRE(hgp != NULL);
	REQUIRE(HISTO_VALID(*hgp));

	isc_histo_t *hg = *hgp;
	*hgp = NULL;

	for (uint c = 0; c < CHUNKS; c++) {
		if (hg->chunk[c] != NULL) {
			isc_mem_cput(hg->mctx, hg->chunk[c], CHUNKSIZE(hg),
				     sizeof(hg_bucket_t));
		}
	}
	isc_mem_putanddetach(&hg->mctx, hg, sizeof(*hg));
}

/**********************************************************************/

uint
isc_histo_sigbits(isc_histo_t *hg) {
	REQUIRE(HISTO_VALID(hg));
	return hg->sigbits;
}

/*
 * use precomputed logs and builtins to avoid linking with libm
 */

uint
isc_histo_bits_to_digits(uint bits) {
	REQUIRE(bits >= ISC_HISTO_MINBITS);
	REQUIRE(bits <= ISC_HISTO_MAXBITS);
	return floor(1.0 - (1.0 - bits) * LN_2 / LN_10);
}

uint
isc_histo_digits_to_bits(uint digits) {
	REQUIRE(digits >= ISC_HISTO_MINDIGITS);
	REQUIRE(digits <= ISC_HISTO_MAXDIGITS);
	return ceil(1.0 - (1.0 - digits) * LN_10 / LN_2);
}

/**********************************************************************/

/*
 * The way we map buckets to keys is what gives the histogram a
 * consistent relative error across the whole range of `uint64_t`.
 * The mapping is log-linear: a chunk key is the logarithm of part
 * of the value (in other words, chunks are spaced exponentially);
 * and a bucket within a chunk is a linear function of another part
 * of the value.
 *
 * This log-linear spacing is similar to the size classes used by
 * jemalloc. It is also the way floating point numbers work: the
 * exponent is the log part, and the mantissa is the linear part.
 *
 * So, a chunk number is the log (base 2) of a `uint64_t`, which is
 * between 0 and 63, which is why there are up to 64 chunks. In
 * floating point terms the chunk number is the exponent. The
 * histogram's number of significant bits is the size of the
 * mantissa, which indexes buckets within each chunk.
 *
 * A fast way to get the logarithm of a positive integer is CLZ,
 * count leading zeroes.
 *
 * Chunk zero is special. Chunk 1 covers values between `CHUNKSIZE`
 * and `CHUNKSIZE * 2 - 1`, where `CHUNKSIZE == exponent << sigbits
 * == 1 << sigbits`. Each chunk has CHUNKSIZE buckets, so chunk 1 has
 * one value per bucket. There are CHUNKSIZE values before chunk 1
 * which map to chunk 0, so it also has one value per bucket. (Hence
 * the first two chunks have one value per bucket.) The values in
 * chunk 0 correspond to denormal nubers in floating point terms.
 * They are also the values where `63 - sigbits - clz` would be less
 * than one if denormals were not handled specially.
 *
 * This branchless conversion is due to Paul Khuong: see bin_down_of() in
 * https://pvk.ca/Blog/2015/06/27/linear-log-bucketing-fast-versatile-simple/
 *
 * This function is in the `isc_histo_inc()` fast path.
 */
static inline uint
value_to_key(const isc_histo_t *hg, uint64_t value) {
	/* ensure that denormal numbers are all in chunk zero */
	uint64_t chunked = value | CHUNKSIZE(hg);
	int clz = __builtin_clzll((unsigned long long)(chunked));
	/* actually 1 less than the exponent except for denormals */
	uint exponent = 63 - hg->sigbits - clz;
	/* mantissa has leading bit set except for denormals */
	uint mantissa = value >> exponent;
	/* leading bit of mantissa adds one to exponent */
	return (exponent << hg->sigbits) + mantissa;
}

/*
 * Inverse functions of `value_to_key()`, to get the minimum and
 * maximum values that map to a particular key.
 *
 * We must not cause undefined behaviour by hitting integer limits,
 * which is a risk when we aim to cover the entire range of `uint64_t`.
 *
 * The maximum value in the last bucket is UINT64_MAX, which
 * `key_to_maxval()` gets by deliberately subtracting `0 - 1`,
 * undeflowing a `uint64_t`. That is OK when unsigned.
 *
 * We must take care not to shift too much in `key_to_minval()`.
 * The largest key passed by `key_to_maxval()` is `BUCKETS(hg)`, so
 *	`exponent == EXPONENTS(hg) - 1 == 64 - sigbits`
 * which is always less than 64, so the size of the shift is OK.
 *
 * The `mantissa` in this edge case is just `chunksize`, which when
 * shifted becomes `1 << 64` which overflows `uint64_t` Again this is
 * OK when unsigned, so the return value is zero.
 */

static inline uint64_t
key_to_minval(const isc_histo_t *hg, uint key) {
	uint chunksize = CHUNKSIZE(hg);
	uint exponent = (key / chunksize) - 1;
	uint64_t mantissa = (key % chunksize) + chunksize;
	return key < chunksize ? key : mantissa << exponent;
}

static inline uint64_t
key_to_maxval(const isc_histo_t *hg, uint key) {
	return key_to_minval(hg, key + 1) - 1;
}

/**********************************************************************/

static hg_bucket_t *
key_to_new_bucket(isc_histo_t *hg, uint key) {
	/* slow path */
	uint chunksize = CHUNKSIZE(hg);
	uint chunk = key / chunksize;
	uint bucket = key % chunksize;
	hg_bucket_t *old_cp = NULL;
	hg_bucket_t *new_cp = isc_mem_cget(hg->mctx, CHUNKSIZE(hg),
					   sizeof(hg_bucket_t));
	hg_chunk_t *cpp = &hg->chunk[chunk];
	if (atomic_compare_exchange_strong_acq_rel(cpp, &old_cp, new_cp)) {
		return &new_cp[bucket];
	} else {
		/* lost the race, so use the winner's chunk */
		isc_mem_cput(hg->mctx, new_cp, CHUNKSIZE(hg),
			     sizeof(hg_bucket_t));
		return &old_cp[bucket];
	}
}

static hg_bucket_t *
get_chunk(const isc_histo_t *hg, uint chunk) {
	return atomic_load_acquire(&hg->chunk[chunk]);
}

static inline hg_bucket_t *
key_to_bucket(const isc_histo_t *hg, uint key) {
	/* fast path */
	uint chunksize = CHUNKSIZE(hg);
	uint chunk = key / chunksize;
	uint bucket = key % chunksize;
	hg_bucket_t *cp = get_chunk(hg, chunk);
	return cp == NULL ? NULL : &cp[bucket];
}

static inline uint64_t
bucket_count(const hg_bucket_t *bp) {
	return bp == NULL ? 0 : atomic_load_relaxed(bp);
}

static inline uint64_t
get_key_count(const isc_histo_t *hg, uint key) {
	return bucket_count(key_to_bucket(hg, key));
}

static inline void
add_key_count(isc_histo_t *hg, uint key, uint64_t inc) {
	/* fast path */
	if (inc > 0) {
		hg_bucket_t *bp = key_to_bucket(hg, key);
		bp = bp != NULL ? bp : key_to_new_bucket(hg, key);
		atomic_fetch_add_relaxed(bp, inc);
	}
}

/**********************************************************************/

void
isc_histo_add(isc_histo_t *hg, uint64_t value, uint64_t inc) {
	REQUIRE(HISTO_VALID(hg));
	add_key_count(hg, value_to_key(hg, value), inc);
}

void
isc_histo_inc(isc_histo_t *hg, uint64_t value) {
	isc_histo_add(hg, value, 1);
}

void
isc_histo_put(isc_histo_t *hg, uint64_t min, uint64_t max, uint64_t count) {
	REQUIRE(HISTO_VALID(hg));

	uint kmin = value_to_key(hg, min);
	uint kmax = value_to_key(hg, max);

	for (uint key = kmin; key <= kmax; key++) {
		uint64_t mid = ISC_MIN(max, key_to_maxval(hg, key));
		double in_bucket = mid - min + 1;
		double remaining = max - min + 1;
		uint64_t inc = ceil(count * in_bucket / remaining);
		add_key_count(hg, key, inc);
		count -= inc;
		min = mid + 1;
	}
}

isc_result_t
isc_histo_get(const isc_histo_t *hg, uint key, uint64_t *minp, uint64_t *maxp,
	      uint64_t *countp) {
	REQUIRE(HISTO_VALID(hg));

	if (key < BUCKETS(hg)) {
		SET_IF_NOT_NULL(minp, key_to_minval(hg, key));
		SET_IF_NOT_NULL(maxp, key_to_maxval(hg, key));
		SET_IF_NOT_NULL(countp, get_key_count(hg, key));
		return ISC_R_SUCCESS;
	} else {
		return ISC_R_RANGE;
	}
}

void
isc_histo_next(const isc_histo_t *hg, uint *keyp) {
	REQUIRE(HISTO_VALID(hg));
	REQUIRE(keyp != NULL);

	uint chunksize = CHUNKSIZE(hg);
	uint buckets = BUCKETS(hg);
	uint key = *keyp;

	key++;
	while (key < buckets && key % chunksize == 0 &&
	       key_to_bucket(hg, key) == NULL)
	{
		key += chunksize;
	}
	*keyp = key;
}

void
isc_histo_merge(isc_histo_t **targetp, const isc_histo_t *source) {
	REQUIRE(HISTO_VALID(source));
	REQUIRE(targetp != NULL);

	if (*targetp != NULL) {
		REQUIRE(HISTO_VALID(*targetp));
	} else {
		isc_histo_create(source->mctx, source->sigbits, targetp);
	}

	uint64_t min, max, count;
	for (uint key = 0;
	     isc_histo_get(source, key, &min, &max, &count) == ISC_R_SUCCESS;
	     isc_histo_next(source, &key))
	{
		isc_histo_put(*targetp, min, max, count);
	}
}

/**********************************************************************/

void
isc_histomulti_create(isc_mem_t *mctx, uint sigbits, isc_histomulti_t **hmp) {
	REQUIRE(hmp != NULL);
	REQUIRE(*hmp == NULL);

	uint size = isc_tid_count();
	INSIST(size > 0);

	isc_histomulti_t *hm = isc_mem_cget(mctx, 1,
					    STRUCT_FLEX_SIZE(hm, hg, size));
	*hm = (isc_histomulti_t){
		.magic = HISTOMULTI_MAGIC,
		.size = size,
	};

	for (uint i = 0; i < hm->size; i++) {
		isc_histo_create(mctx, sigbits, &hm->hg[i]);
	}

	*hmp = hm;
}

void
isc_histomulti_destroy(isc_histomulti_t **hmp) {
	REQUIRE(hmp != NULL);
	REQUIRE(HISTOMULTI_VALID(*hmp));

	isc_histomulti_t *hm = *hmp;
	isc_mem_t *mctx = hm->hg[0]->mctx;
	*hmp = NULL;

	for (uint i = 0; i < hm->size; i++) {
		isc_histo_destroy(&hm->hg[i]);
	}

	isc_mem_put(mctx, hm, STRUCT_FLEX_SIZE(hm, hg, hm->size));
}

void
isc_histomulti_merge(isc_histo_t **hgp, const isc_histomulti_t *hm) {
	REQUIRE(HISTOMULTI_VALID(hm));

	for (uint i = 0; i < hm->size; i++) {
		isc_histo_merge(hgp, hm->hg[i]);
	}
}

void
isc_histomulti_add(isc_histomulti_t *hm, uint64_t value, uint64_t inc) {
	REQUIRE(HISTOMULTI_VALID(hm));
	isc_histo_t *hg = hm->hg[isc_tid()];
	add_key_count(hg, value_to_key(hg, value), inc);
}

void
isc_histomulti_inc(isc_histomulti_t *hm, uint64_t value) {
	isc_histomulti_add(hm, value, 1);
}

/**********************************************************************/

/*
 * https://fanf2.user.srcf.net/hermes/doc/antiforgery/stats.pdf
 * equation 4 (incremental mean) and equation 44 (incremental variance)
 */
void
isc_histo_moments(const isc_histo_t *hg, double *pm0, double *pm1,
		  double *pm2) {
	REQUIRE(HISTO_VALID(hg));

	uint64_t pop = 0;
	double mean = 0.0;
	double sigma = 0.0;

	uint64_t min, max, count;
	for (uint key = 0;
	     isc_histo_get(hg, key, &min, &max, &count) == ISC_R_SUCCESS;
	     isc_histo_next(hg, &key))
	{
		if (count == 0) { /* avoid division by zero */
			continue;
		}
		double value = min / 2.0 + max / 2.0;
		double delta = value - mean;
		pop += count;
		mean += count * delta / pop;
		sigma += count * delta * (value - mean);
	}

	SET_IF_NOT_NULL(pm0, pop);
	SET_IF_NOT_NULL(pm1, mean);
	SET_IF_NOT_NULL(pm2, (pop > 0) ? sqrt(sigma / pop) : 0.0);
}

/*
 * Clamped linear interpolation
 *
 * `outrange` should be `((1 << n) - 1)` for some `n`; when `n` is larger
 * than 53, `outrange` can get rounded up to a power of 2, so we clamp the
 * result to keep within bounds (extra important when `max == UINT64_MAX`)
 */
static inline uint64_t
lerp(uint64_t min, uint64_t max, uint64_t lo, uint64_t in, uint64_t hi) {
	double inrange = (double)(hi - lo);
	double inpart = (double)(in - lo);
	double outrange = (double)(max - min);
	double outpart = round(outrange * inpart / inrange);
	return min + ISC_MIN((uint64_t)outpart, max - min);
}

/*
 * There is non-zero space for the inner value, and it is inside the bounds
 */
static inline bool
inside(uint64_t lo, uint64_t in, uint64_t hi) {
	return lo < hi && lo <= in && in <= hi;
}

isc_result_t
isc_histo_quantiles(const isc_histo_t *hg, uint size, const double *fraction,
		    uint64_t *value) {
	hg_bucket_t *chunk[CHUNKS];
	uint64_t total[CHUNKS];
	uint64_t rank[ISC_HISTO_MAXQUANTILES];

	REQUIRE(HISTO_VALID(hg));
	REQUIRE(0 < size && size <= ISC_HISTO_MAXQUANTILES);
	REQUIRE(fraction != NULL);
	REQUIRE(value != NULL);

	const uint maxchunk = MAXCHUNK(hg);
	const uint chunksize = CHUNKSIZE(hg);

	/*
	 * Find out which chunks exist and what their totals are. We take a
	 * copy of the chunk pointers to reduce the need for atomic ops
	 * later on. Scan from low to high so that higher buckets are more
	 * likely to be in the CPU cache when we scan from high to low.
	 */
	uint64_t population = 0;
	for (uint c = 0; c < maxchunk; c++) {
		chunk[c] = get_chunk(hg, c);
		total[c] = 0;
		if (chunk[c] != NULL) {
			for (uint b = chunksize; b-- > 0;) {
				total[c] += bucket_count(&chunk[c][b]);
			}
			population += total[c];
		}
	}

	/*
	 * Now we know the population, we can convert fractions to ranks.
	 * Also ensure they are within bounds and in decreasing order.
	 */
	for (uint i = 0; i < size; i++) {
		REQUIRE(0.0 <= fraction[i] && fraction[i] <= 1.0);
		REQUIRE(i == 0 || fraction[i - 1] > fraction[i]);
		rank[i] = round(fraction[i] * population);
	}

	/*
	 * Scan chunks from high to low, keeping track of the bounds on
	 * each chunk's ranks. Each time we match `rank[i]`, move on to the
	 * next rank and continue the scan from the same place.
	 */
	uint i = 0;
	uint64_t chunk_lo = population;
	for (uint c = maxchunk; c-- > 0;) {
		uint64_t chunk_hi = chunk_lo;
		chunk_lo = chunk_hi - total[c];

		/*
		 * Scan buckets backwards within this chunk, in a similar
		 * manner to the chunk scan. Skip all or part of the loop
		 * if the current rank is not in the chunk.
		 */
		uint64_t bucket_lo = chunk_hi;
		for (uint b = chunksize;
		     b-- > 0 && inside(chunk_lo, rank[i], chunk_hi);)
		{
			uint64_t bucket_hi = bucket_lo;
			bucket_lo = bucket_hi - bucket_count(&chunk[c][b]);

			/*
			 * Convert all ranks that fall in this bucket.
			 */
			while (inside(bucket_lo, rank[i], bucket_hi)) {
				uint key = chunksize * c + b;
				value[i] = lerp(key_to_minval(hg, key),
						key_to_maxval(hg, key),
						bucket_lo, rank[i], bucket_hi);
				if (++i == size) {
					return ISC_R_SUCCESS;
				}
			}
		}
	}

	return ISC_R_UNSET;
}

/**********************************************************************/
