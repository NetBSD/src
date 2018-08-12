/*	$NetBSD: random_test.c,v 1.2 2018/08/12 13:02:39 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * IMPORTANT NOTE:
 * These tests work by generating a large number of pseudo-random numbers
 * and then statistically analyzing them to determine whether they seem
 * random. The test is expected to fail on occasion by random happenstance.
 */

#include <config.h>

#include <isc/random.h>
#include <isc/result.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/util.h>

#include <atf-c.h>

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define REPS 25000

typedef double (pvalue_func_t)(isc_mem_t *mctx,
			       isc_uint16_t *values, size_t length);

/* igamc(), igam(), etc. were adapted (and cleaned up) from the Cephes
 * math library:
 *
 * Cephes Math Library Release 2.8:  June, 2000
 * Copyright 1985, 1987, 2000 by Stephen L. Moshier
 *
 * The Cephes math library was released into the public domain as part
 * of netlib.
*/

static double MACHEP = 1.11022302462515654042E-16;
static double MAXLOG = 7.09782712893383996843E2;
static double big = 4.503599627370496e15;
static double biginv =	2.22044604925031308085e-16;

static double igamc(double a, double x);
static double igam(double a, double x);

static double
igamc(double a, double x) {
	double ans, ax, c, yc, r, t, y, z;
	double pk, pkm1, pkm2, qk, qkm1, qkm2;

	if ((x <= 0) || (a <= 0))
		return (1.0);

	if ((x < 1.0) || (x < a))
		return (1.0 - igam(a, x));

	ax = a * log(x) - x - lgamma(a);
	if (ax < -MAXLOG) {
		fprintf(stderr, "igamc: UNDERFLOW, ax=%f\n", ax);
		return (0.0);
	}
	ax = exp(ax);

	/* continued fraction */
	y = 1.0 - a;
	z = x + y + 1.0;
	c = 0.0;
	pkm2 = 1.0;
	qkm2 = x;
	pkm1 = x + 1.0;
	qkm1 = z * x;
	ans = pkm1 / qkm1;

	do {
		c += 1.0;
		y += 1.0;
		z += 2.0;
		yc = y * c;
		pk = pkm1 * z  -  pkm2 * yc;
		qk = qkm1 * z  -  qkm2 * yc;
		if (qk != 0) {
			r = pk / qk;
			t = fabs((ans - r) / r);
			ans = r;
		} else
			t = 1.0;

		pkm2 = pkm1;
		pkm1 = pk;
		qkm2 = qkm1;
		qkm1 = qk;

		if (fabs(pk) > big) {
			pkm2 *= biginv;
			pkm1 *= biginv;
			qkm2 *= biginv;
			qkm1 *= biginv;
		}
	} while (t > MACHEP);

	return (ans * ax);
}

static double
igam(double a, double x) {
	double ans, ax, c, r;

	if ((x <= 0) || (a <= 0))
		return (0.0);

	if ((x > 1.0) && (x > a))
		return (1.0 - igamc(a, x));

	/* Compute  x**a * exp(-x) / md_gamma(a)  */
	ax = a * log(x) - x - lgamma(a);
	if( ax < -MAXLOG ) {
		fprintf(stderr, "igam: UNDERFLOW, ax=%f\n", ax);
		return (0.0);
	}
	ax = exp(ax);

	/* power series */
	r = a;
	c = 1.0;
	ans = 1.0;

	do {
		r += 1.0;
		c *= x / r;
		ans += c;
	} while (c / ans > MACHEP);

	return (ans * ax / a);
}

static isc_int8_t scounts_table[65536];
static isc_uint8_t bitcounts_table[65536];

static isc_int8_t
scount_calculate(isc_uint16_t n) {
	int i;
	isc_int8_t sc;

	sc = 0;
	for (i = 0; i < 16; i++) {
		isc_uint16_t lsb;

		lsb = n & 1;
		if (lsb != 0)
			sc += 1;
		else
			sc -= 1;

		n >>= 1;
	}

	return (sc);
}

static isc_uint8_t
bitcount_calculate(isc_uint16_t n) {
	int i;
	isc_uint8_t bc;

	bc = 0;
	for (i = 0; i < 16; i++) {
		isc_uint16_t lsb;

		lsb = n & 1;
		if (lsb != 0)
			bc += 1;

		n >>= 1;
	}

	return (bc);
}

static void
tables_init(void) {
	isc_uint32_t i;

	for (i = 0; i < 65536; i++) {
		scounts_table[i] = scount_calculate(i);
		bitcounts_table[i] = bitcount_calculate(i);
	}
}

/*
 * The following code for computing Marsaglia's rank is based on the
 * implementation in cdbinrnk.c from the diehard tests by George
 * Marsaglia.
 *
 * This function destroys (modifies) the data passed in bits.
 */
static isc_uint32_t
matrix_binaryrank(isc_uint32_t *bits, size_t rows, size_t cols) {
	size_t i, j, k;
	unsigned int rt = 0;
	isc_uint32_t rank = 0;
	isc_uint32_t tmp;

	for (k = 0; k < rows; k++) {
		i = k;

		while (rt >= cols || ((bits[i] >> rt) & 1) == 0) {
			i++;

			if (i < rows)
				continue;
			else {
				rt++;
				if (rt < cols) {
					i = k;
					continue;
				}
			}

			return (rank);
		}

		rank++;
		if (i != k) {
			tmp = bits[i];
			bits[i] = bits[k];
			bits[k] = tmp;
		}

		for (j = i + 1; j < rows; j++) {
			if (((bits[j] >> rt) & 1) == 0)
				continue;
			else
				bits[j] ^= bits[k];
		}

		rt++;
	}

	return (rank);
}

static void
random_test(pvalue_func_t *func, isc_boolean_t word_sized) {
	isc_mem_t *mctx = NULL;
	isc_result_t result;
	isc_rng_t *rng;
	isc_uint32_t m;
	isc_uint32_t j;
	isc_uint32_t histogram[11] = { 0 };
	isc_uint32_t passed;
	double proportion;
	double p_hat;
	double lower_confidence, higher_confidence;
	double chi_square;
	double p_value_t;
	double alpha;

	tables_init();

	result = isc_mem_create(0, 0, &mctx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	rng = NULL;
	result = isc_rng_create(mctx, NULL, &rng);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	m = 1000;
	passed = 0;

	for (j = 0; j < m; j++) {
		isc_uint32_t i;
		isc_uint16_t values[REPS];
		double p_value;

		if (word_sized) {
			for (i = 0; i < REPS; i++) {
				isc_rng_randombytes(rng, &values[i],
						    sizeof(values[i]));
			}
		} else {
			isc_rng_randombytes(rng, values, sizeof(values));
		}

		p_value = (*func)(mctx, values, REPS);
		if (p_value >= 0.01) {
			passed++;
		}

		ATF_REQUIRE(p_value >= 0.0);
		ATF_REQUIRE(p_value <= 1.0);

		i = (int) floor(p_value * 10);
		histogram[i]++;
	}

	isc_rng_detach(&rng);

	/*
	 * Check proportion of sequences passing a test (see section
	 * 4.2.1 in NIST SP 800-22).
	 */
	alpha = 0.01; /* the significance level */
	proportion = (double) passed / (double) m;
	p_hat = 1.0 - alpha;
	lower_confidence = p_hat - (3.0 * sqrt((p_hat * (1.0 - p_hat)) / m));
	higher_confidence = p_hat + (3.0 * sqrt((p_hat * (1.0 - p_hat)) / m));

	/* Debug message, not displayed when running via atf-run */
	printf("passed=%u/1000\n", passed);
	printf("higher_confidence=%f, lower_confidence=%f, proportion=%f\n",
	       higher_confidence, lower_confidence, proportion);

	ATF_REQUIRE(proportion >= lower_confidence);
	ATF_REQUIRE(proportion <= higher_confidence);

	/*
	 * Check uniform distribution of p-values (see section 4.2.2 in
	 * NIST SP 800-22).
	 */

	/* Fold histogram[10] (p_value = 1.0) into histogram[9] for
	 * interval [0.9, 1.0]
	 */
	histogram[9] += histogram[10];
	histogram[10] = 0;

	/* Pre-requisite that at least 55 sequences are processed. */
	ATF_REQUIRE(m >= 55);

	chi_square = 0.0;
	for (j = 0; j < 10; j++) {
		double numer;
		double denom;

		/* Debug message, not displayed when running via atf-run */
		printf("hist%u=%u ", j, histogram[j]);

		numer = (histogram[j] - (m / 10.0)) *
			(histogram[j] - (m / 10.0));
		denom = m / 10.0;
		chi_square += numer / denom;
	}

	printf("\n");

	p_value_t = igamc(9 / 2.0, chi_square / 2.0);

	ATF_REQUIRE(p_value_t >= 0.0001);
}

/*
 * This is a frequency (monobits) test taken from the NIST SP 800-22
 * RNG test suite.
 */
static double
monobit(isc_mem_t *mctx, isc_uint16_t *values, size_t length) {
	size_t i;
	isc_int32_t scount;
	isc_uint32_t numbits;
	double s_obs;
	double p_value;

	UNUSED(mctx);

	numbits = length * 16;
	scount = 0;

	for (i = 0; i < length; i++)
		scount += scounts_table[values[i]];

	/* Preconditions (section 2.1.7 in NIST SP 800-22) */
	ATF_REQUIRE(numbits >= 100);

	/* Debug message, not displayed when running via atf-run */
	printf("numbits=%u, scount=%d\n", numbits, scount);

	s_obs = abs(scount) / sqrt(numbits);
	p_value = erfc(s_obs / sqrt(2.0));

	return (p_value);
}

/*
 * This is the runs test taken from the NIST SP 800-22 RNG test suite.
 */
static double
runs(isc_mem_t *mctx, isc_uint16_t *values, size_t length) {
	size_t i;
	isc_uint32_t bcount;
	isc_uint32_t numbits;
	double pi;
	double tau;
	isc_uint32_t j;
	isc_uint32_t b;
	isc_uint8_t bit_this;
	isc_uint8_t bit_prev;
	isc_uint32_t v_obs;
	double numer;
	double denom;
	double p_value;

	UNUSED(mctx);

	numbits = length * 16;
	bcount = 0;

	for (i = 0; i < REPS; i++)
		bcount += bitcounts_table[values[i]];

	/* Debug message, not displayed when running via atf-run */
	printf("numbits=%u, bcount=%u\n", numbits, bcount);

	pi = (double) bcount / (double) numbits;
	tau = 2.0 / sqrt(numbits);

	/* Preconditions (section 2.3.7 in NIST SP 800-22) */
	ATF_REQUIRE(numbits >= 100);

	/*
	 * Pre-condition implied from the monobit test. This can fail
	 * for some sequences, and the p-value is taken as 0 in these
	 * cases.
	 */
	if (fabs(pi - 0.5) >= tau)
		return (0.0);

	/* Compute v_obs */
	j = 0;
	b = 14;
	bit_prev = (values[j] & (1U << 15)) == 0 ? 0 : 1;

	v_obs = 0;

	for (i = 1; i < numbits; i++) {
		bit_this = (values[j] & (1U << b)) == 0 ? 0 : 1;
		if (b == 0) {
			b = 15;
			j++;
		} else {
			b--;
		}

		v_obs += bit_this ^ bit_prev;

		bit_prev = bit_this;
	}

	v_obs += 1;

	numer = fabs(v_obs - (2.0 * numbits * pi * (1.0 - pi)));
	denom = 2.0 * sqrt(2.0 * numbits) * pi * (1.0 - pi);

	p_value = erfc(numer / denom);

	return (p_value);
}

/*
 * This is the block frequency test taken from the NIST SP 800-22 RNG
 * test suite.
 */
static double
blockfrequency(isc_mem_t *mctx, isc_uint16_t *values, size_t length) {
	isc_uint32_t i;
	isc_uint32_t numbits;
	isc_uint32_t mbits;
	isc_uint32_t mwords;
	isc_uint32_t numblocks;
	double *pi;
	double chi_square;
	double p_value;

	numbits = length * 16;
	mbits = 32000;
	mwords = mbits / 16;
	numblocks = numbits / mbits;

	/* Debug message, not displayed when running via atf-run */
	printf("numblocks=%u\n", numblocks);

	/* Preconditions (section 2.2.7 in NIST SP 800-22) */
	ATF_REQUIRE(numbits >= 100);
	ATF_REQUIRE(mbits >= 20);
	ATF_REQUIRE((double) mbits > (0.01 * numbits));
	ATF_REQUIRE(numblocks < 100);
	ATF_REQUIRE(numbits >= (mbits * numblocks));

	pi = isc_mem_get(mctx, numblocks * sizeof(double));
	ATF_REQUIRE(pi != NULL);

	for (i = 0; i < numblocks; i++) {
		isc_uint32_t j;
		pi[i] = 0.0;
		for (j = 0; j < mwords; j++) {
			isc_uint32_t idx;

			idx = i * mwords + j;
			pi[i] += bitcounts_table[values[idx]];
		}
		pi[i] /= mbits;
	}

	/* Compute chi_square */
	chi_square = 0.0;
	for (i = 0; i < numblocks; i++)
		chi_square += (pi[i] - 0.5) * (pi[i] - 0.5);

	chi_square *= 4 * mbits;

	isc_mem_put(mctx, pi, numblocks * sizeof(double));

	/* Debug message, not displayed when running via atf-run */
	printf("chi_square=%f\n", chi_square);

	p_value = igamc(numblocks * 0.5, chi_square * 0.5);

	return (p_value);
}

/*
 * This is the binary matrix rank test taken from the NIST SP 800-22 RNG
 * test suite.
 */
static double
binarymatrixrank(isc_mem_t *mctx, isc_uint16_t *values, size_t length) {
	isc_uint32_t i;
	size_t matrix_m;
	size_t matrix_q;
	isc_uint32_t num_matrices;
	size_t numbits;
	isc_uint32_t fm_0;
	isc_uint32_t fm_1;
	isc_uint32_t fm_rest;
	double term1;
	double term2;
	double term3;
	double chi_square;
	double p_value;

	UNUSED(mctx);

	matrix_m = 32;
	matrix_q = 32;
	num_matrices = length / ((matrix_m * matrix_q) / 16);
	numbits = num_matrices * matrix_m * matrix_q;

	/* Preconditions (section 2.5.7 in NIST SP 800-22) */
	ATF_REQUIRE(matrix_m == 32);
	ATF_REQUIRE(matrix_q == 32);
	ATF_REQUIRE(numbits >= (38 * matrix_m * matrix_q));

	fm_0 = 0;
	fm_1 = 0;
	fm_rest = 0;
	for (i = 0; i < num_matrices; i++) {
		/*
		 * Each isc_uint32_t supplies 32 bits, so a 32x32 bit matrix
		 * takes up isc_uint32_t array of size 32.
		 */
		isc_uint32_t bits[32];
		int j;
		isc_uint32_t rank;

		for (j = 0; j < 32; j++) {
			size_t idx;
			isc_uint32_t r1;
			isc_uint32_t r2;

			idx = i * ((matrix_m * matrix_q) / 16);
			idx += j * 2;

			r1 = values[idx];
			r2 = values[idx + 1];
			bits[j] = (r1 << 16) | r2;
		}

		rank = matrix_binaryrank(bits, matrix_m, matrix_q);

		if (rank == matrix_m)
			fm_0++;
		else if (rank == (matrix_m - 1))
			fm_1++;
		else
			fm_rest++;
	}

	/* Compute chi_square */
	term1 = ((fm_0 - (0.2888 * num_matrices)) *
		 (fm_0 - (0.2888 * num_matrices))) / (0.2888 * num_matrices);
	term2 = ((fm_1 - (0.5776 * num_matrices)) *
		 (fm_1 - (0.5776 * num_matrices))) / (0.5776 * num_matrices);
	term3 = ((fm_rest - (0.1336 * num_matrices)) *
		 (fm_rest - (0.1336 * num_matrices))) / (0.1336 * num_matrices);

	chi_square = term1 + term2 + term3;

	/* Debug message, not displayed when running via atf-run */
	printf("fm_0=%u, fm_1=%u, fm_rest=%u, chi_square=%f\n",
	       fm_0, fm_1, fm_rest, chi_square);

	p_value = exp(-chi_square * 0.5);

	return (p_value);
}

ATF_TC(isc_rng_monobit_16);
ATF_TC_HEAD(isc_rng_monobit_16, tc) {
	atf_tc_set_md_var(tc, "descr", "Monobit test for the RNG");
}

ATF_TC_BODY(isc_rng_monobit_16, tc) {
	UNUSED(tc);

	random_test(monobit, ISC_TRUE);
}

ATF_TC(isc_rng_runs_16);
ATF_TC_HEAD(isc_rng_runs_16, tc) {
	atf_tc_set_md_var(tc, "descr", "Runs test for the RNG");
}

ATF_TC_BODY(isc_rng_runs_16, tc) {
	UNUSED(tc);

	random_test(runs, ISC_TRUE);
}

ATF_TC(isc_rng_blockfrequency_16);
ATF_TC_HEAD(isc_rng_blockfrequency_16, tc) {
	atf_tc_set_md_var(tc, "descr", "Block frequency test for the RNG");
}

ATF_TC_BODY(isc_rng_blockfrequency_16, tc) {
	UNUSED(tc);

	random_test(blockfrequency, ISC_TRUE);
}

ATF_TC(isc_rng_binarymatrixrank_16);
ATF_TC_HEAD(isc_rng_binarymatrixrank_16, tc) {
	atf_tc_set_md_var(tc, "descr", "Binary matrix rank test for the RNG");
}

/*
 * This is the binary matrix rank test taken from the NIST SP 800-22 RNG
 * test suite.
 */
ATF_TC_BODY(isc_rng_binarymatrixrank_16, tc) {
	UNUSED(tc);

	random_test(binarymatrixrank, ISC_TRUE);
}

ATF_TC(isc_rng_monobit_bytes);
ATF_TC_HEAD(isc_rng_monobit_bytes, tc) {
	atf_tc_set_md_var(tc, "descr", "Monobit test for the RNG");
}

ATF_TC_BODY(isc_rng_monobit_bytes, tc) {
	UNUSED(tc);

	random_test(monobit, ISC_FALSE);
}

ATF_TC(isc_rng_runs_bytes);
ATF_TC_HEAD(isc_rng_runs_bytes, tc) {
	atf_tc_set_md_var(tc, "descr", "Runs test for the RNG");
}

ATF_TC_BODY(isc_rng_runs_bytes, tc) {
	UNUSED(tc);

	random_test(runs, ISC_FALSE);
}

ATF_TC(isc_rng_blockfrequency_bytes);
ATF_TC_HEAD(isc_rng_blockfrequency_bytes, tc) {
	atf_tc_set_md_var(tc, "descr", "Block frequency test for the RNG");
}

ATF_TC_BODY(isc_rng_blockfrequency_bytes, tc) {
	UNUSED(tc);

	random_test(blockfrequency, ISC_FALSE);
}

ATF_TC(isc_rng_binarymatrixrank_bytes);
ATF_TC_HEAD(isc_rng_binarymatrixrank_bytes, tc) {
	atf_tc_set_md_var(tc, "descr", "Binary matrix rank test for the RNG");
}

/*
 * This is the binary matrix rank test taken from the NIST SP 800-22 RNG
 * test suite.
 */
ATF_TC_BODY(isc_rng_binarymatrixrank_bytes, tc) {
	UNUSED(tc);

	random_test(binarymatrixrank, ISC_FALSE);
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_rng_monobit_16);
	ATF_TP_ADD_TC(tp, isc_rng_runs_16);
	ATF_TP_ADD_TC(tp, isc_rng_blockfrequency_16);
	ATF_TP_ADD_TC(tp, isc_rng_binarymatrixrank_16);
	ATF_TP_ADD_TC(tp, isc_rng_monobit_bytes);
	ATF_TP_ADD_TC(tp, isc_rng_runs_bytes);
	ATF_TP_ADD_TC(tp, isc_rng_blockfrequency_bytes);
	ATF_TP_ADD_TC(tp, isc_rng_binarymatrixrank_bytes);

	return (atf_no_error());
}
