/* $NetBSD: qsieve.c,v 1.1 2006/01/24 18:59:23 elad Exp $ */

/*-
 * Copyright 1994 Phil Karn <karn@qualcomm.com>
 * Copyright 1996-1998, 2003 William Allen Simpson <wsimpson@greendragon.com>
 * Copyright 2000 Niels Provos <provos@citi.umich.edu>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Sieve candidates for "safe" primes,
 *  suitable for use as Diffie-Hellman moduli;
 *  that is, where q = (p-1)/2 is also prime.
 *
 * This is the first of two steps.
 * This step is memory intensive.
 *
 * 1996 May     William Allen Simpson
 *              extracted from earlier code by Phil Karn, April 1994.
 *              save large primes list for later processing.
 * 1998 May     William Allen Simpson
 *              parameterized.
 * 2000 Dec     Niels Provos
 *              convert from GMP to openssl BN.
 * 2003 Jun     William Allen Simpson
 *              change outfile definition slightly to match openssh mistake.
 *              move common file i/o to own file for better documentation.
 *              redo memory again.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/bn.h>
#include <string.h>
#include <err.h>
#include "qfile.h"

/* define DEBUG_LARGE 1 */
/* define DEBUG_SMALL 1 */

/*
 * Using virtual memory can cause thrashing.  This should be the largest
 * number that is supported without a large amount of disk activity --
 * that would increase the run time from hours to days or weeks!
 */
#define LARGE_MINIMUM   (8UL)	/* megabytes */

/*
 * Do not increase this number beyond the unsigned integer bit size.
 * Due to a multiple of 4, it must be LESS than 128 (yielding 2**30 bits).
 */
#define LARGE_MAXIMUM   (127UL)	/* megabytes */

/*
 * Constant: assuming 8 bit bytes and 32 bit words
 */
#define SHIFT_BIT       (3)
#define SHIFT_BYTE      (2)
#define SHIFT_WORD      (SHIFT_BIT+SHIFT_BYTE)
#define SHIFT_MEGABYTE  (20)
#define SHIFT_MEGAWORD  (SHIFT_MEGABYTE-SHIFT_BYTE)

/*
 * Constant: when used with 32-bit integers, the largest sieve prime
 * has to be less than 2**32.
 */
#define SMALL_MAXIMUM   (0xffffffffUL)

/*
 * Constant: can sieve all primes less than 2**32, as 65537**2 > 2**32-1.
 */
#define TINY_NUMBER     (1UL<<16)

/*
 * Ensure enough bit space for testing 2*q.
 */
#define TEST_MAXIMUM    (1UL<<16)
#define TEST_MINIMUM    (QSIZE_MINIMUM + 1)
/* real TEST_MINIMUM    (1UL << (SHIFT_WORD - TEST_POWER)) */
#define TEST_POWER      (3)	/* 2**n, n < SHIFT_WORD */

/*
 * bit operations on 32-bit words
 */
#define BIT_CLEAR(a,n)  ((a)[(n)>>SHIFT_WORD] &= ~(1U << ((n) & 31)))
#define BIT_SET(a,n)    ((a)[(n)>>SHIFT_WORD] |= (1U << ((n) & 31)))
#define BIT_TEST(a,n)   ((a)[(n)>>SHIFT_WORD] & (1U << ((n) & 31)))

/*
 * sieve relative to the initial value
 */
uint32_t       *LargeSieve;
uint32_t        largewords;
uint32_t        largetries;
uint32_t        largenumbers;
uint32_t        largememory;	/* megabytes */
uint32_t        largebits;
BIGNUM         *largebase;

/*
 * sieve 2**30 in 2**16 parts
 */
uint32_t       *SmallSieve;
uint32_t        smallbits;
uint32_t        smallbase;

/*
 * sieve 2**16
 */
uint32_t       *TinySieve;
uint32_t        tinybits;

static void     usage(void);
void            sieve_large(uint32_t);

/*
 * Sieve p's and q's with small factors
 */
void
sieve_large(uint32_t s)
{
	BN_ULONG        r;
	BN_ULONG        u;

#ifdef  DEBUG_SMALL
	(void)fprintf(stderr, "%lu\n", s);
#endif
	largetries++;
	/* r = largebase mod s */
	r = BN_mod_word(largebase, (BN_ULONG) s);
	if (r == 0) {
		/* s divides into largebase exactly */
		u = 0;
	} else {
		/* largebase+u is first entry divisible by s */
		u = s - r;
	}

	if (u < largebits * 2) {
		/*
		 * The sieve omits p's and q's divisible by 2, so ensure that
		 * largebase+u is odd. Then, step through the sieve in
		 * increments of 2*s
		 */
		if (u & 0x1) {
			/* Make largebase+u odd, and u even */
			u += s;
		}

		/* Mark all multiples of 2*s */
		for (u /= 2; u < largebits; u += s) {
			BIT_SET(LargeSieve, (uint32_t)u);
		}
	}

	/* r = p mod s */
	r = (2 * r + 1) % s;

	if (r == 0) {
		/* s divides p exactly */
		u = 0;
	} else {
		/* p+u is first entry divisible by s */
		u = s - r;
	}

	if (u < largebits * 4) {
		/*
		 * The sieve omits p's divisible by 4, so ensure that
		 * largebase+u is not. Then, step through the sieve in
		 * increments of 4*s
		 */
		while (u & 0x3) {
			if (SMALL_MAXIMUM - u < s) {
				return;
			}

			u += s;
		}

		/* Mark all multiples of 4*s */
		for (u /= 4; u < largebits; u += s) {
			BIT_SET(LargeSieve, (uint32_t)u);
		}
	}
}

/*
 * list candidates for Sophie-Germaine primes
 * (where q = (p-1)/2)
 * to standard output.
 * The list is checked against small known primes
 * (less than 2**30).
 */
int
main(int argc, char *argv[])
{
	BIGNUM         *q;
	uint32_t        j;
	int             power;
	uint32_t        r;
	uint32_t        s;
	uint32_t        smallwords = TINY_NUMBER >> 6;
	uint32_t        t;
	time_t          time_start;
	time_t          time_stop;
	uint32_t        tinywords = TINY_NUMBER >> 6;
	unsigned int    i;

	setprogname(argv[0]);

	if (argc < 3) {
		usage();
	}

	/*
         * Set power to the length in bits of the prime to be generated.
         * This is changed to 1 less than the desired safe prime moduli p.
         */
	power = (int) strtoul(argv[2], NULL, 10);
	if (power > TEST_MAXIMUM) {
		errx(1, "Too many bits: %d > %lu.", power,
		     (unsigned long)TEST_MAXIMUM);
	} else if (power < TEST_MINIMUM) {
		errx(1, "Too few bits: %d < %lu.", power,
		     (unsigned long)TEST_MINIMUM);
	}

	power--;		/* decrement before squaring */

	/*
         * The density of ordinary primes is on the order of 1/bits, so the
         * density of safe primes should be about (1/bits)**2. Set test range
         * to something well above bits**2 to be reasonably sure (but not
         * guaranteed) of catching at least one safe prime.
	 */
	largewords = (uint32_t)((unsigned long)
			(power * power) >> (SHIFT_WORD - TEST_POWER));

	/*
         * Need idea of how much memory is available. We don't have to use all
         * of it.
	 */
	largememory = (uint32_t)strtoul(argv[1], NULL, 10);
	if (largememory > LARGE_MAXIMUM) {
		warnx("Limited memory: %u MB; limit %lu MB.", largememory,
		      LARGE_MAXIMUM);
		largememory = LARGE_MAXIMUM;
	}

	if (largewords <= (largememory << SHIFT_MEGAWORD)) {
		warnx("Increased memory: %u MB; need %u bytes.",
		      largememory, (largewords << SHIFT_BYTE));
		largewords = (largememory << SHIFT_MEGAWORD);
	} else if (largememory > 0) {
		warnx("Decreased memory: %u MB; want %u bytes.",
		      largememory, (largewords << SHIFT_BYTE));
		largewords = (largememory << SHIFT_MEGAWORD);
	}

	if ((TinySieve = (uint32_t *) calloc((size_t) tinywords, sizeof(uint32_t))) == NULL) {
		errx(1, "Insufficient memory for tiny sieve: need %u byts.",
		     tinywords << SHIFT_BYTE);
	}
	tinybits = tinywords << SHIFT_WORD;

	if ((SmallSieve = (uint32_t *) calloc((size_t) smallwords, sizeof(uint32_t))) == NULL) {
		errx(1, "Insufficient memory for small sieve: need %u bytes.",
		     smallwords << SHIFT_BYTE);
	}
	smallbits = smallwords << SHIFT_WORD;

	/*
	 * dynamically determine available memory
	 */
	while ((LargeSieve = (uint32_t *)calloc((size_t)largewords,
						sizeof(uint32_t))) == NULL) {
		/* 1/4 MB chunks */
		largewords -= (1L << (SHIFT_MEGAWORD - 2));
	}
	largebits = largewords << SHIFT_WORD;
	largenumbers = largebits * 2;	/* even numbers excluded */

	/* validation check: count the number of primes tried */
	largetries = 0;

	q = BN_new();
	largebase = BN_new();

	/*
         * Generate random starting point for subprime search, or use
         * specified parameter.
	 */
	if (argc < 4) {
		BN_rand(largebase, power, 1, 1);
	} else {
		BIGNUM         *a;

		a = largebase;
		BN_hex2bn(&a, argv[2]);
	}

	/* ensure odd */
	if (!BN_is_odd(largebase)) {
		BN_set_bit(largebase, 0);
	}

	time(&time_start);
	(void)fprintf(stderr,
		"%.24s Sieve next %u plus %d-bit start point:\n# ",
		ctime(&time_start), largenumbers, power);
	BN_print_fp(stderr, largebase);
	(void)fprintf(stderr, "\n");

	/*
         * TinySieve
         */
	for (i = 0; i < tinybits; i++) {
		if (BIT_TEST(TinySieve, i)) {
			/* 2*i+3 is composite */
			continue;
		}

		/* The next tiny prime */
		t = 2 * i + 3;

		/* Mark all multiples of t */
		for (j = i + t; j < tinybits; j += t) {
			BIT_SET(TinySieve, j);
		}

		sieve_large(t);
	}

	/*
         * Start the small block search at the next possible prime. To avoid
         * fencepost errors, the last pass is skipped.
         */
	for (smallbase = TINY_NUMBER + 3;
	     smallbase < (SMALL_MAXIMUM - TINY_NUMBER);
	     smallbase += TINY_NUMBER) {
		for (i = 0; i < tinybits; i++) {
			if (BIT_TEST(TinySieve, i)) {
				/* 2*i+3 is composite */
				continue;
			}

			/* The next tiny prime */
			t = 2 * i + 3;
			r = smallbase % t;

			if (r == 0) {
				/* t divides into smallbase exactly */
				s = 0;
			} else {
				/* smallbase+s is first entry divisible by t */
				s = t - r;
			}

			/*
			 * The sieve omits even numbers, so ensure that
			 * smallbase+s is odd. Then, step through the sieve in
			 * increments of 2*t
			 */
			if (s & 1) {
				/* Make smallbase+s odd, and s even */
				s += t;
			}

			/* Mark all multiples of 2*t */
			for (s /= 2; s < smallbits; s += t) {
				BIT_SET(SmallSieve, s);
			}
		}

		/*
                 * SmallSieve
                 */
		for (i = 0; i < smallbits; i++) {
			if (BIT_TEST(SmallSieve, i)) {
				/* 2*i+smallbase is composite */
				continue;
			}

			/* The next small prime */
			sieve_large((2 * i) + smallbase);
		}

		memset(SmallSieve, 0, (size_t)(smallwords << SHIFT_BYTE));
	}

	time(&time_stop);
	(void)fprintf(stderr,
		"%.24s Sieved with %u small primes in %lu seconds\n",
		ctime(&time_stop), largetries,
		(long) (time_stop - time_start));

	for (j = r = 0; j < largebits; j++) {
		if (BIT_TEST(LargeSieve, j)) {
			/* Definitely composite, skip */
			continue;
		}

#ifdef  DEBUG_LARGE
		(void)fprintf(stderr, "test q = largebase+%lu\n", 2 * j);
#endif

		BN_set_word(q, (unsigned long)(2 * j));
		BN_add(q, q, largebase);

		if (0 > qfileout(stdout,
				 (uint32_t) QTYPE_SOPHIE_GERMAINE,
				 (uint32_t) QTEST_SIEVE,
				 largetries,
				 (uint32_t) (power - 1), /* MSB */
				 (uint32_t) (0), /* generator unknown */
				 q)) {
			break;
		}

		r++;		/* count q */
	}

	time(&time_stop);

	free(LargeSieve);
	free(SmallSieve);
	free(TinySieve);

	fflush(stdout);
	/* fclose(stdout); */

	(void) fprintf(stderr, "%.24s Found %u candidates\n",
	    ctime(&time_stop), r);

	return (0);
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s <megabytes> <bits> [initial]\n"
		"Possible values for <megabytes>: 0, %lu to %lu\n"
		"Possible values for <bits>: %lu to %lu\n",
		getprogname(),
		LARGE_MINIMUM,
		LARGE_MAXIMUM,
		(unsigned long) TEST_MINIMUM,
		(unsigned long) TEST_MAXIMUM);

	exit(1);
}
