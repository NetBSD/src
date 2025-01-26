/*	$NetBSD: histo_test.c,v 1.1.1.1 2025/01/26 16:12:36 christos Exp $	*/

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

/* ! \file */

#include <inttypes.h>
#include <math.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/histo.h>
#include <isc/result.h>
#include <isc/time.h>

#include <tests/isc.h>

#define TIME_LIMIT (123 * NS_PER_MS)

#define SUBRANGE 69

#if VERBOSE

#define TRACE(fmt, ...)                                                        \
	fprintf(stderr, "%s:%u:%s(): " fmt "\n", __FILE__, __LINE__, __func__, \
		__VA_ARGS__)

#define TRACETIME(fmt, ...) \
	TRACE("%u bits %.1f ms " fmt, bits, millis_since(start), ##__VA_ARGS__)

static double
millis_since(isc_nanosecs_t start) {
	isc_nanosecs_t end = isc_time_monotonic();
	return (double)(end - start) / NS_PER_MS;
}

#else
#define TRACE(...)
#define TRACETIME(...) UNUSED(start)
#endif

/*
 * Note: in many of these tests when adding data to a histogram,
 * we need to iterate using `key++` instead of `isc_histo_next()`
 * because the latter skips chunks that we want to fill but have
 * not yet done so.
 */

ISC_RUN_TEST_IMPL(basics) {
	isc_result_t result;
	for (uint bits = ISC_HISTO_MINBITS; bits <= ISC_HISTO_MAXBITS; bits++) {
		isc_nanosecs_t start = isc_time_monotonic();

		isc_histo_t *hg = NULL;
		isc_histo_create(mctx, bits, &hg);

		isc_histo_inc(hg, 0);

		uint64_t min, max, count;

		uint64_t prev_max = 0;
		uint key = 0;
		result = isc_histo_get(hg, key, &min, &max, &count);
		while (result == ISC_R_SUCCESS) {
			/* previous iteration already bumped this bucket */
			assert_int_equal(count, 1);

			/* min maps to this bucket */
			isc_histo_inc(hg, min);
			result = isc_histo_get(hg, key, &min, &max, &count);
			assert_int_equal(result, ISC_R_SUCCESS);
			assert_int_equal(count, 2);

			/* max maps to this bucket */
			isc_histo_add(hg, max, 2);
			result = isc_histo_get(hg, key, &min, &max, &count);
			assert_int_equal(result, ISC_R_SUCCESS);
			assert_int_equal(count, 4);

			/* put range covers this bucket */
			isc_histo_put(hg, min, max, 4);
			result = isc_histo_get(hg, key, &min, &max, &count);
			assert_int_equal(result, ISC_R_SUCCESS);
			assert_int_equal(count, 8);

			if (max < UINT64_MAX) {
				/* max + 1 maps to next bucket */
				isc_histo_inc(hg, max + 1);
				result = isc_histo_get(hg, key, &min, &max,
						       &count);
				assert_int_equal(result, ISC_R_SUCCESS);
				/* this bucket was not bumped */
				assert_int_equal(count, 8);
			}

			if (key == 0) {
				assert_int_equal(min, 0);
				assert_int_equal(max, 0);
			} else {
				/* no gap between buckets */
				assert_int_equal(min, prev_max + 1);
			}

			prev_max = max;
			key++;
			result = isc_histo_get(hg, key, &min, &max, &count);

			/* these tests can be slow */
			if (isc_time_monotonic() > start + TIME_LIMIT) {
				break;
			}
		}

		/* if we did not stop early */
		if (result != ISC_R_SUCCESS) {
			/* last bucket goes up to last possible value */
			assert_int_equal(max, UINT64_MAX);

			double pop;
			isc_histo_moments(hg, &pop, NULL, NULL);
			assert_int_equal((uint64_t)pop, key * 8);
		}

		isc_histo_destroy(&hg);

		TRACETIME("%u keys", key);
	}
}

ISC_RUN_TEST_IMPL(quantiles) {
	for (uint bits = ISC_HISTO_MINBITS; bits <= ISC_HISTO_MAXBITS; bits++) {
		isc_result_t result;
		uint64_t min, max, count;
		double pop, mean, sd;
		uint key;

		isc_nanosecs_t start = isc_time_monotonic();

		isc_histo_t *hg = NULL;
		isc_histo_create(mctx, bits, &hg);

		/* ensure empty histogram does not divide by zero */
		isc_histo_moments(hg, &pop, &mean, &sd);
		assert_true(pop == 0.0);
		assert_true(mean == 0.0);
		assert_true(sd == 0.0);

		for (key = 0; isc_histo_get(hg, key, &min, &max, &count) ==
			      ISC_R_SUCCESS;
		     key++)
		{
			/* inc twice so we can check bucket's midpoint */
			assert_int_equal(count, 0);
			isc_histo_inc(hg, min);
			isc_histo_inc(hg, max);
		}

		const uint buckets = key;

		/* no incs were lost */
		isc_histo_moments(hg, &pop, NULL, NULL);
		assert_float_equal(pop, buckets * 2, 0.5);

		/* two ranks per bucket */
		const uint quantum = ISC_HISTO_MAXQUANTILES / 2 - 1;
		uint64_t value[ISC_HISTO_MAXQUANTILES];
		double frac[ISC_HISTO_MAXQUANTILES];
		uint base = 0;

		for (key = 0; key < buckets; key++) {
			/* fill in the values one quantum at a time */
			if (key == 0 || key % quantum == buckets % quantum) {
				base = key;
				for (uint k = 0; k < quantum; k++) {
					double rank = (base + k) * 2;
					uint i = (quantum - k) * 2;
					frac[i - 1] = (rank + 1.0) / pop;
					frac[i - 0] = rank / pop;
				}
				frac[0] = (base + quantum) * 2 / pop;
				result = isc_histo_quantiles(
					hg, quantum * 2 + 1, frac, value);
				assert_int_equal(result, ISC_R_SUCCESS);
			}

			result = isc_histo_get(hg, key, &min, &max, &count);
			assert_int_equal(result, ISC_R_SUCCESS);
			assert_int_equal(count, 2);

			uint64_t lomin = min == 0 ? min : min - 1;
			uint64_t himin = min;
			uint64_t lomid = floor(min / 2.0 + max / 2.0);
			uint64_t himid = ceil(min / 2.0 + max / 2.0);
			uint64_t lomax = max;
			uint64_t himax = max == UINT64_MAX ? max : max + 1;

			uint i = (quantum + base - key) * 2;

			/* check fenceposts */
			assert_in_range(value[i - 0], lomin, himin);
			assert_in_range(value[i - 1], lomid, himid);
			assert_in_range(value[i - 2], lomax, himax);

			/* these tests can be slow */
			if (isc_time_monotonic() > start + TIME_LIMIT) {
				break;
			}
		}

		isc_histo_destroy(&hg);

		TRACETIME("");
	}
}

/*
 * ensure relative error is as expected
 */
ISC_RUN_TEST_IMPL(sigfigs) {
	assert_int_equal(ISC_HISTO_MINBITS,
			 isc_histo_digits_to_bits(ISC_HISTO_MINDIGITS));
	assert_int_equal(ISC_HISTO_MINDIGITS,
			 isc_histo_bits_to_digits(ISC_HISTO_MINBITS));
	assert_int_equal(ISC_HISTO_MAXBITS,
			 isc_histo_digits_to_bits(ISC_HISTO_MAXDIGITS));
	assert_int_equal(ISC_HISTO_MAXDIGITS,
			 isc_histo_bits_to_digits(ISC_HISTO_MAXBITS));

	uint log10 = 1;
	double exp10 = 1.0; /* sigdigs == 1 gives relative error of 1 */

	for (uint bits = ISC_HISTO_MINBITS; bits <= ISC_HISTO_MAXBITS; bits++) {
		isc_histo_t *hg = NULL;
		isc_histo_create(mctx, bits, &hg);

		uint digits = isc_histo_bits_to_digits(bits);
		assert_true(bits >= isc_histo_digits_to_bits(digits));

		if (log10 < digits) {
			log10 += 1;
			exp10 *= 10.0;
			assert_int_equal(log10, digits);
		}

		TRACE("%u binary %f decimal", 1 << bits, exp10);

		/* binary precision is better than decimal precision */
		double nominal = 1.0 / (double)(1 << bits);
		assert_true(nominal < 1.0 / exp10);

		/* start with key = 1 to avoid division by zero */
		uint64_t imin, imax;
		for (uint key = 1; isc_histo_get(hg, key, &imin, &imax, NULL) ==
				   ISC_R_SUCCESS;
		     key++)
		{
			double min = (double)imin;
			double max = (double)imax;
			double error = (max - min) / (max + min);
			assert_true(error < nominal);
		}

		isc_histo_destroy(&hg);
	}
}

ISC_RUN_TEST_IMPL(subrange) {
	for (uint bits = ISC_HISTO_MINBITS; bits <= ISC_HISTO_MAXBITS; bits++) {
		isc_result_t result;
		uint64_t min, max, count;

		isc_nanosecs_t start = isc_time_monotonic();

		isc_histo_t *hg = NULL;
		isc_histo_create(mctx, bits, &hg);

		uint64_t value[SUBRANGE + 1];
		double frac[SUBRANGE + 1];
		for (uint i = 0; i <= SUBRANGE; i++) {
			frac[i] = (double)(SUBRANGE - i) / (double)(SUBRANGE);
		}

		result = isc_histo_quantiles(hg, ARRAY_SIZE(frac), frac, value);
		assert_int_equal(result, ISC_R_UNSET);

		for (uint key = 0, top = SUBRANGE - 1;; key++, top++) {
			if (isc_histo_get(hg, key, &min, NULL, NULL) !=
			    ISC_R_SUCCESS)
			{
				break;
			}
			if (isc_histo_get(hg, top, NULL, &max, NULL) !=
			    ISC_R_SUCCESS)
			{
				break;
			}
			/*
			 * If we try adding more than one sample per bucket
			 * here, the test fails when buckets have different
			 * sizes because [min,max] spans multiple chunks.
			 */
			isc_histo_put(hg, min, max, SUBRANGE);

			result = isc_histo_quantiles(hg, ARRAY_SIZE(frac), frac,
						     value);
			assert_int_equal(result, ISC_R_SUCCESS);

			for (uint bucket = 0; bucket < SUBRANGE; bucket++) {
				result = isc_histo_get(hg, key + bucket, &min,
						       &max, &count);
				assert_int_equal(result, ISC_R_SUCCESS);
				/* did isc_histo_put() spread evenly? */
				assert_int_equal(count, 1);
				/* do the quantile values match? */
				assert_int_equal(value[SUBRANGE - bucket], min);
			}
			assert_int_equal(value[0], max);

			isc_histo_destroy(&hg);
			isc_histo_create(mctx, bits, &hg);

			/* these tests can be slow */
			if (isc_time_monotonic() > start + TIME_LIMIT) {
				break;
			}
		}
		isc_histo_destroy(&hg);

		TRACETIME("");
	}
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(basics)
ISC_TEST_ENTRY(quantiles)
ISC_TEST_ENTRY(sigfigs)
ISC_TEST_ENTRY(subrange)

ISC_TEST_LIST_END

ISC_TEST_MAIN
