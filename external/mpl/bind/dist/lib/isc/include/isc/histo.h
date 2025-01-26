/*	$NetBSD: histo.h,v 1.2 2025/01/26 16:25:41 christos Exp $	*/

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

#pragma once

#include <sys/types.h>

#include <isc/mem.h>

/*
 * An `isc_histo_t` is a thread-safe histogram of `uint64_t` values.
 * It keeps a count of how many values land in each bucket. Use the
 * `isc_histo_inc()`, `isc_histo_acc()`, and `isc_histo_put()`
 * functions to add values to the histogram.
 *
 * Values are mapped to buckets by rounding them according to a
 * configurable precision, expressed as a number of significant bits.
 * The bits <-> digits functions convert betwen decimal significant
 * digits (as in scientific notation) and binary significant bits.
 *
 * You can use the `isc_histo_get()` function to export data from the
 * histogram. The range of a bucket is returned as its minimum and
 * maximum values, inclusive, i.e. a closed interval. We use closed
 * intervals so we are able to express the maximum of the last bucket,
 * UINT64_MAX, although half-open intervals are more common in C.
 *
 * You can calculate some basic statistics directly from a histogram.
 * The `isc_histo_quantiles()` function can get a histogram's median,
 * 99th percentile, etc. The `isc_histo_moments()` function gets a
 * histogram's population, mean, and standard deviation.
 *
 * The size of a histogram depends on the range of values in the
 * stream of samples, not the number of samples. Bucket counters are
 * 64 bits each, and are allocated in chunks of `1 << sigbits` where
 * `sigbits` is the histogram's configured precision. There are at
 * most 64 chunks, one for each bit of a 64 bit value. Histograms with
 * greater precision have larger chunks.
 *
 * At the low end (values near zero) there is one value per bucket,
 * then two values, four, eight, etc. The number of values that map to
 * a bucket is the same in each chunk. Chunks 0 and 1 have one value
 * per bucket, (see `ISC_HISTO_UNITBUCKETS()` below), chunk 2 has 2
 * values per bucket, chunk 3 has 4, etc.
 *
 * The update cost is roughly constant and very small (not much more
 * than an atomic increment). It mostly depends on cache locality and
 * thread contention.
 *
 * There is no overflow checking for the 64 bit bucket counters. It
 * takes a few nanoseconds to add a sample to the histogram, so it
 * would take at least a few CPU-centuries to cause an overflow.
 * Aggregate statistics from a quarter of a million CPUs might
 * overflow in a day. (Provided that in both examples the CPUs are
 * doing nothing apart from repeatedly adding 1 to histogram buckets.)
 */

typedef struct isc_histo      isc_histo_t;
typedef struct isc_histomulti isc_histomulti_t;

#define ISC_HISTO_MINBITS      1
#define ISC_HISTO_MAXBITS      18
#define ISC_HISTO_MINDIGITS    1
#define ISC_HISTO_MAXDIGITS    6
#define ISC_HISTO_MAXQUANTILES 101 /* enough for all the percentiles */

/*
 * How many values map 1:1 to buckets for a given number of sigbits?
 * These are the buckets at the low end, starting from zero.
 */
#define ISC_HISTO_UNITBUCKETS(sigbits) (2 << (sigbits))

void
isc_histo_create(isc_mem_t *mctx, uint sigbits, isc_histo_t **hgp);
/*%<
 * Create a histogram.
 *
 * The relative error of values stored in the histogram is less than
 * `pow(2.0, -sigbits)`.
 *
 * Requires:
 *\li	`sigbits >= ISC_HISTO_MINBITS`
 *\li	`sigbits <= ISC_HISTO_MAXBITS`
 *\li	`hgp != NULL`
 *\li	`*hgp == NULL`
 *
 * Ensures:
 *\li	`*hgp` is a pointer to a histogram.
 */

void
isc_histo_destroy(isc_histo_t **hgp);
/*%<
 * Destroy a histogram
 *
 * Requires:
 *\li	`hgp != NULL`
 *\li	`*hgp` is a pointer to a valid histogram
 *
 * Ensures:
 *\li	all memory allocated by the histogram has been released
 *\li	`*hgp` is NULL
 */

uint
isc_histo_sigbits(isc_histo_t *hg);
/*%<
 * Get the histogram's `sigbits` setting
 *
 * Requires:
 *\li	`hg` is a pointer to a valid histogram
 */

uint
isc_histo_bits_to_digits(uint bits);
/*%<
 * Convert binary significant figures to decimal significant figures,
 * rounding down, i.e. get the decimal precision you can expect from a
 * given number of significant bits.
 *
 * Requires:
 *\li	`bits >= ISC_HISTO_MINBITS`
 *\li	`bits <= ISC_HISTO_MAXBITS`
 */

uint
isc_histo_digits_to_bits(uint digits);
/*%<
 * Convert decimal significant figures to binary significant figures,
 * rounding up, i.e. get the number of significant bits required to
 * achieve the given decimal precision.
 *
 * Requires:
 *\li	`digits >= ISC_HISTO_MINDIGS`
 *\li	`digits <= ISC_HISTO_MAXDIGS`
 */

/**********************************************************************/

void
isc_histo_inc(isc_histo_t *hg, uint64_t value);
/*%<
 * Add 1 to the value's bucket
 *
 * Requires:
 *\li	`hg` is a pointer to a valid histogram
 */

void
isc_histo_add(isc_histo_t *hg, uint64_t value, uint64_t inc);
/*%<
 * Add an arbitrary increment to the value's bucket
 *
 * Note: there is no counter overflow checking
 *
 * Requires:
 *\li	`hg` is a pointer to a valid histogram
 */

void
isc_histo_put(isc_histo_t *hg, uint64_t min, uint64_t max, uint64_t count);
/*
 * Import a collection of samples, where values between `min` and
 * `max` inclusive occurred `count` times. This function is a
 * counterpart to `isc_histo_get()`.
 *
 * Note: there is no counter overflow checking
 *
 * Requires:
 *\li	`min <= max`
 *\li	`hg` is a pointer to a valid histogram
 */

isc_result_t
isc_histo_get(const isc_histo_t *hg, uint key, uint64_t *minp, uint64_t *maxp,
	      uint64_t *countp);
/*%<
 * Export information about a bucket.
 *
 * This can be used as an iterator, by initializing `key` to zero
 * and incrementing by one or using `isc_histo_next()` until
 * `isc_histo_get()` returns ISC_R_RANGE. The number of iterations is
 * less than `64 << sigbits`. (64 for the maximum number of chunks,
 * multiplied by the size of each chunk.)
 *
 * It is also a counterpart to `isc_histo_put()`.
 *
 * If `minp` is non-NULL it is set to the minimum inclusive value
 * that maps to this bucket.
 *
 * If `maxp` is non-NULL it is set to the maximum inclusive value
 * that maps to this bucket.
 *
 * If `countp` is non-NULL it is set to the bucket's counter,
 * which can be zero.
 *
 * Requires:
 *\li	`hg` is a pointer to a valid histogram
 *
 * Returns:
 *\li	ISC_R_SUCCESS, if `key` is valid
 *\li	ISC_R_RANGE, otherwise
 */

void
isc_histo_next(const isc_histo_t *hg, uint *keyp);
/*%<
 * Skip to the next key, omitting chunks of unallocated buckets.
 *
 * This function does not skip buckets that have been allocated but
 * are zero. A chunk contains `1 << sigbits` buckets, and buckets
 * are created in bulk one chunk at a time.
 *
 * Example:
 *
 *	uint64_t min, max, count;
 *	for (uint key = 0;
 *	     isc_histo_get(hg, key, &min, &max, &count) == ISC_R_SUCCESS;
 *	     isc_histo_next(hg, &key))
 *	{
 *		// do something with the bucket
 *	}
 *
 * Requires:
 *\li	`hg` is a pointer to a valid histogram
 *\li	`keyp != NULL`
 */

void
isc_histo_merge(isc_histo_t **targetp, const isc_histo_t *source);
/*%<
 * Increase the counts in `*ptarget` by the counts recorded in `source`
 *
 * If `*targetp == NULL` then `*ptarget` is set to point to a new
 * histogram with the same `sigbits` as the `source`.
 *
 * This function uses `isc_histo_get()` and `isc_histo_next()` to
 * export the data from `source`, and `isc_histo_put()` to import it
 * into `*ptarget`.
 *
 * Requires:
 *\li	`targetp != NULL`
 *\li	`*targetp` is NULL or a pointer to a valid histogram
 *\li	`source` is a pointer to a valid histogram
 *
 * Ensures:
 *\li	`*targetp` is a pointer to a valid histogram
 */

/**********************************************************************/

void
isc_histomulti_create(isc_mem_t *mctx, uint sigbits, isc_histomulti_t **hmp);
/*%<
 * Create a multithreaded sharded histogram.
 *
 * Although an `isc_histo_t` is thread-safe, it can suffer
 * from cache contention under heavy load. To avoid this,
 * an `isc_histomulti_t` contains a histogram per thread,
 * so updates are local and low-contention.
 *
 * Requires:
 *\li	`sigbits >= ISC_HISTO_MINBITS`
 *\li	`sigbits <= ISC_HISTO_MAXBITS`
 *\li	`hmp != NULL`
 *\li	`*hmp == NULL`
 *
 * Ensures:
 *\li	`*hmp` is a pointer to a multithreaded sharded histogram.
 */

void
isc_histomulti_destroy(isc_histomulti_t **hmp);
/*%<
 * Destroy a multithreaded sharded histogram
 *
 * Requires:
 *\li	`hmp != NULL`
 *\li	`*hmp` is a pointer to a valid multithreaded sharded histogram
 *
 * Ensures:
 *\li	all memory allocated by the histogram has been released
 *\li	`*hmp == NULL`
 */

void
isc_histomulti_merge(isc_histo_t **targetp, const isc_histomulti_t *source);
/*%<
 * Increase the counts in `*targetp` by the counts recorded in `source`
 *
 * The target histogram is created if `*targetp` is NULL.
 *
 * Requires:
 *\li	`targetp != NULL`
 *\li	`*targetp` is NULL or a pointer to a valid histogram
 *\li	`source` is a pointer to a valid multithreaded sharded histogram
 *
 * Ensures:
 *\li	`*targetp` is a pointer to a valid histogram
 */

void
isc_histomulti_inc(isc_histomulti_t *hm, uint64_t value);
/*%<
 * Add 1 to the value's bucket
 *
 * Requires:
 *\li	`hm` is a pointer to a valid histomulti
 */

void
isc_histomulti_add(isc_histomulti_t *hm, uint64_t value, uint64_t inc);
/*%<
 * Add an arbitrary increment to the value's bucket
 *
 * Requires:
 *\li	`hm` is a pointer to a valid histomulti
 */

/**********************************************************************/

void
isc_histo_moments(const isc_histo_t *hg, double *pm0, double *pm1, double *pm2);
/*%<
 * Get the population, mean, and standard deviation of a histogram.
 *
 * If `pm0` is non-NULL it is set to the population of the histogram.
 * (Strictly speaking, the zeroth moment is `pop / pop == 1`.)
 *
 * If `pm1` is non-NULL it is set to the mean (first moment) of the
 * recorded data.
 *
 * If `pm2` is non-NULL it is set to the standard deviation of the
 * recorded data. The standard deviation is the square root of the
 * variance, which is the second moment about the mean.
 *
 * It is safe if the histogram is concurrently modified.
 *
 * Requires:
 *\li	`hg` is a pointer to a valid histogram
 */

isc_result_t
isc_histo_quantiles(const isc_histo_t *hg, uint size, const double *fraction,
		    uint64_t *value);
/*%<
 * The quantile function (aka inverse cumulative distribution function)
 * of the histogram. What value is greater than the given fraction of
 * the population?
 *
 * A fraction of 0.5 gets the median value: it is greater than half
 * the population. 0.75 gets the third quartile value, and 0.99 gets
 * the 99th percentile value. The fraction must be between 0.0 and 1.0
 * inclusive.
 *
 * https://enwp.org/Quantile_function
 *
 * This implementation allows you to query quantile values for
 * multiple fractions in one function call. Internally, it makes one
 * linear scan over the histogram's buckets to find all the fractions.
 * Buckets are scanned from high to low, so that querying large
 * quantiles is more efficient. The `fraction` array must be sorted in
 * decreasing order. The results are stored in the `value` array. Both
 * arrays have `size` elements.
 *
 * The results may be nonsense if the histogram is concurrently
 * modified. To get a stable copy you can call `isc_histo_merge()`.
 *
 * Requires:
 *\li	`hg` is a pointer to a valid histogram
 *\li	`0 < size && size <= ISC_HISTO_MAXQUANTILES`
 *\li	`fraction != NULL`
 *\li	`value != NULL`
 *\li	`0.0 <= fraction[i] && fraction[i] <= 1.0` for every element
 *\li	`fraction[i - 1] > fraction[i]` for every pair of elements
 *
 * Returns:
 *\li	ISC_R_SUCCESS, if results were stored in the `value` array
 *\li	ISC_R_UNSET, if the histogram is empty
 */

/**********************************************************************/
