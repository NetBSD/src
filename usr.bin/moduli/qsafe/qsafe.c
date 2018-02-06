/* $NetBSD: qsafe.c,v 1.4 2018/02/06 19:32:49 christos Exp $ */

/*-
 * Copyright 1994 Phil Karn <karn@qualcomm.com>
 * Copyright 1996-1998, 2003 William Allen206 Simpson <wsimpson@greendragon.com>
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
 * Test probable "safe" primes,
 *
 *  suitable for use as Diffie-Hellman moduli;
 *  that is, where q = (p-1)/2 is also prime.
 *
 * This is the second of two steps.
 * This step is processor intensive.
 *
 * 1996 May     William Allen Simpson
 *              extracted from earlier code by Phil Karn, April 1994.
 *              read large prime candidates list (q),
 *              and check prime probability of (p).
 * 1998 May     William Allen Simpson
 *              parameterized.
 *              optionally limit to a single generator.
 * 2000 Dec     Niels Provos
 *              convert from GMP to openssl BN.
 * 2003 Jun     William Allen Simpson
 *              change outfile definition slightly to match openssh mistake.
 *              move common file i/o to own file for better documentation.
 *              redo debugprint again.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <openssl/bn.h>
#include "qfile.h"

/* define DEBUGPRINT     1 */
#define TRIAL_MINIMUM           (4)

__dead static void     usage(void);

/*
 * perform a Miller-Rabin primality test
 * on the list of candidates
 * (checking both q and p)
 * from standard input.
 * The result is a list of so-call "safe" primes
 * to standard output,
 */
int
main(int argc, char *argv[])
{
	BIGNUM         *q, *p, *a;
	BN_CTX         *ctx;
	char           *cp;
	char           *lp;
	uint32_t        count_in = 0;
	uint32_t        count_out = 0;
	uint32_t        count_possible = 0;
	uint32_t        generator_known;
	uint32_t        generator_wanted = 0;
	uint32_t        in_tests;
	uint32_t        in_tries;
	uint32_t        in_type;
	uint32_t        in_size;
	int             trials;
	time_t          time_start;
	time_t          time_stop;

	setprogname(argv[0]);

	if (argc < 2) {
		usage();
	}

	if ((trials = strtoul(argv[1], NULL, 10)) < TRIAL_MINIMUM) {
		trials = TRIAL_MINIMUM;
	}

	if (argc > 2) {
		generator_wanted = strtoul(argv[2], NULL, 16);
	}

	time(&time_start);

	p = BN_new();
	q = BN_new();
	ctx = BN_CTX_new();

	(void)fprintf(stderr,
		"%.24s Final %d Miller-Rabin trials (%x generator)\n",
		ctime(&time_start), trials, generator_wanted);

	lp = (char *) malloc((unsigned long) QLINESIZE + 1);

	while (fgets(lp, QLINESIZE, stdin) != NULL) {
		size_t          ll = strlen(lp);

		count_in++;

		if (ll < 14 || *lp == '!' || *lp == '#') {
#ifdef  DEBUGPRINT
			(void)fprintf(stderr, "%10lu: comment or short"
				      " line\n", count_in);
#endif
			continue;
		}

		/* time */
		cp = &lp[14];	/* (skip) */

		/* type */
		in_type = strtoul(cp, &cp, 10);

		/* tests */
		in_tests = strtoul(cp, &cp, 10);
		if (in_tests & QTEST_COMPOSITE) {
#ifdef  DEBUGPRINT
			(void)fprintf(stderr, "%10lu: known composite\n",
				count_in);
#endif
			continue;
		}

		/* tries */
		in_tries = (uint32_t) strtoul(cp, &cp, 10);

		/* size (most significant bit) */
		in_size = (uint32_t) strtoul(cp, &cp, 10);

		/* generator (hex) */
		generator_known = (uint32_t) strtoul(cp, &cp, 16);

		/* Skip white space */
		cp += strspn(cp, " ");

		/* modulus (hex) */
		switch (in_type) {
		case QTYPE_SOPHIE_GERMAINE:
#ifdef  DEBUGPRINT
			(void)fprintf(stderr, "%10lu: (%lu) "
				      "Sophie-Germaine\n", count_in,
				      in_type);
#endif

			a = q;
			BN_hex2bn(&a, cp);
			/* p = 2*q + 1 */
			BN_lshift(p, q, 1);
			BN_add_word(p, 1UL);
			in_size += 1;
			generator_known = 0;

			break;

		default:
#ifdef  DEBUGPRINT
			(void)fprintf(stderr, "%10lu: (%lu)\n",
				      count_in,	in_type);
#endif
			a = p;
			BN_hex2bn(&a, cp);
			/* q = (p-1) / 2 */
			BN_rshift(q, p, 1);

			break;
		}

		/*
		 * due to earlier inconsistencies in interpretation, check the
		 * proposed bit size.
		 */
		if ((uint32_t)BN_num_bits(p) != (in_size + 1)) {
#ifdef  DEBUGPRINT
			(void)fprintf(stderr, "%10lu: bit size %ul "
				      "mismatch\n", count_in, in_size);
#endif
			continue;
		}

		if (in_size < QSIZE_MINIMUM) {
#ifdef  DEBUGPRINT
			(void)fprintf(stderr, "%10lu: bit size %ul "
				      "too short\n", count_in, in_size);
#endif
			continue;
		}

		if (in_tests & QTEST_MILLER_RABIN)
			in_tries += trials;
		else
			in_tries = trials;

		/*
		 * guess unknown generator
		 */
		if (generator_known == 0) {
			if (BN_mod_word(p, 24UL) == 11)
				generator_known = 2;
			else if (BN_mod_word(p, 12UL) == 5)
				generator_known = 3;
			else {
				BN_ULONG        r = BN_mod_word(p, 10UL);

				if (r == 3 || r == 7) {
					generator_known = 5;
				}
			}
		}

		/*
		 * skip tests when desired generator doesn't match
		 */
		if (generator_wanted > 0 &&
		    generator_wanted != generator_known) {
#ifdef  DEBUGPRINT
			(void)fprintf(stderr,
				      "%10lu: generator %ld != %ld\n",
				count_in, generator_known, generator_wanted);
#endif
			continue;
		}

		count_possible++;

		/*
		 * The (1/4)^N performance bound on Miller-Rabin is extremely
		 * pessimistic, so don't spend a lot of time really verifying
		 * that q is prime until after we know that p is also prime. A
		 * single pass will weed out the vast majority of composite
		 * q's.
		 */
		if (BN_is_prime_ex(q, 1, ctx, NULL) <= 0) {
#ifdef  DEBUGPRINT
			(void)fprintf(stderr, "%10lu: q failed first "
				      "possible prime test\n", count_in);
#endif
			continue;
		}

		/*
		 * q is possibly prime, so go ahead and really make sure that
		 * p is prime. If it is, then we can go back and do the same
		 * for q. If p is composite, chances are that will show up on
		 * the first Rabin-Miller iteration so it doesn't hurt to
		 * specify a high iteration count.
		 */
		if (!BN_is_prime_ex(p, trials, ctx, NULL)) {
#ifdef  DEBUGPRINT
			(void)fprintf(stderr, "%10lu: p is not prime\n",
				      count_in);
#endif
			continue;
		}

#ifdef  DEBUGPRINT
		(void)fprintf(stderr, "%10lu: p is almost certainly "
			      "prime\n", count_in);
#endif

		/* recheck q more rigorously */
		if (!BN_is_prime_ex(q, trials - 1, ctx, NULL)) {
#ifdef  DEBUGPRINT
			(void)fprintf(stderr, "%10lu: q is not prime\n",
				      count_in);
#endif
			continue;
		}
#ifdef  DEBUGPRINT
		fprintf(stderr, "%10lu: q is almost certainly prime\n",
			count_in);
#endif
		if (0 > qfileout(stdout,
				 QTYPE_SAFE,
				 (in_tests | QTEST_MILLER_RABIN),
				 in_tries,
				 in_size,
				 generator_known,
				 p)) {
			break;
		}
		count_out++;
#ifdef  DEBUGPRINT
		fflush(stderr);
		fflush(stdout);
#endif
	}

	time(&time_stop);
	free(lp);
	BN_free(p);
	BN_free(q);
	BN_CTX_free(ctx);
	fflush(stdout);		/* fclose(stdout); */
	/* fclose(stdin); */
	(void)fprintf(stderr,
	   "%.24s Found %u safe primes of %u candidates in %lu seconds\n",
		ctime(&time_stop), count_out, count_possible,
		(long) (time_stop - time_start));

	return (0);
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s <trials> [generator]\n",
		      getprogname());
	exit(1);
}
