/*	$NetBSD: factor.c,v 1.38 2020/10/12 13:54:51 christos Exp $	*/
/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Landon Curt Noll.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
#include <sys/cdefs.h>
#ifdef __COPYRIGHT
__COPYRIGHT("@(#) Copyright (c) 1989, 1993\
	The Regents of the University of California.  All rights reserved.");
#endif
#ifdef __SCCSID
__SCCSID("@(#)factor.c	8.4 (Berkeley) 5/4/95");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: factor.c,v 1.38 2020/10/12 13:54:51 christos Exp $");
#endif
#ifdef __FBSDID
__FBSDID("$FreeBSD: head/usr.bin/factor/factor.c 356666 2020-01-12 20:25:11Z gad $");
#endif
#endif /* not lint */

/*
 * factor - factor a number into primes
 *
 * By: Landon Curt Noll   chongo@toad.com,   ...!{sun,tolsoft}!hoptoad!chongo
 *
 *   chongo <for a good prime call: 391581 * 2^216193 - 1> /\oo/\
 *
 * usage:
 *	factor [-hx] [number] ...
 *
 * The form of the output is:
 *
 *	number: factor1 factor1 factor2 factor3 factor3 factor3 ...
 *
 * where factor1 <= factor2 <= factor3 <= ...
 *
 * If the -h flag is specified, the output is in "human-readable" format.
 * Duplicate factors are printed in the form of x^n.
 *
 * If the -x flag is specified numbers are printed in hex.
 *
 * If no number args are given, the list of numbers are read from stdin.
 */

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "primes.h"

#ifdef HAVE_OPENSSL

#include <openssl/bn.h>

#define	PRIME_CHECKS	5

/* print factors for big numbers */
static void	pollard_pminus1(BIGNUM *, int, int);

#else

typedef long	BIGNUM;
typedef u_long	BN_ULONG;

#define BN_CTX			int
#define BN_CTX_new()		NULL
#define BN_new()		calloc(sizeof(BIGNUM), 1)
#define BN_free(a)		free(a)

#define BN_copy(a, b)		*(a) = *(b)
#define BN_cmp(a, b)		(*(a) - *(b))
#define BN_is_zero(v)		(*(v) == 0)
#define BN_is_one(v)		(*(v) == 1)
#define BN_mod_word(a, b)	(*(a) % (b))

static BIGNUM  *BN_dup(const BIGNUM *);
static int	BN_dec2bn(BIGNUM **, const char *);
static int	BN_hex2bn(BIGNUM **, const char *);
static BN_ULONG BN_div_word(BIGNUM *, BN_ULONG);
static void	BN_print_fp(FILE *, const BIGNUM *);

#endif

static void	BN_print_dec_fp(FILE *, const BIGNUM *);
static void	convert_str2bn(BIGNUM **, char *);
static void	pr_fact(BIGNUM *, int, int);	/* print factors of a value */
static void	pr_print(BIGNUM *, int, int);	/* print a prime */
static void	usage(void) __dead;

static BN_CTX	*ctx;			/* just use a global context */

int
main(int argc, char *argv[])
{
	BIGNUM *val;
	int ch, hflag, xflag;
	char *p, buf[LINE_MAX];		/* > max number of digits. */

	hflag = xflag = 0;
	ctx = BN_CTX_new();
	val = BN_new();
	if (val == NULL)
		errx(1, "can't initialise bignum");

	while ((ch = getopt(argc, argv, "hx")) != -1)
		switch (ch) {
		case 'h':
			hflag++;
			break;
		case 'x':
			xflag++;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* No args supplied, read numbers from stdin. */
	if (argc == 0)
		for (;;) {
			if (fgets(buf, sizeof(buf), stdin) == NULL) {
				if (ferror(stdin))
					err(1, "stdin");
				exit (0);
			}
			for (p = buf; isblank((unsigned char)*p); ++p);
			if (*p == '\n' || *p == '\0')
				continue;
			convert_str2bn(&val, p);
			pr_fact(val, hflag, xflag);
		}
	/* Factor the arguments. */
	else
		for (p = *argv; p != NULL; p = *++argv) {
			convert_str2bn(&val, p);
			pr_fact(val, hflag, xflag);
		}
	exit(0);
}

static void
pr_exp(int i, int xflag)
{
	printf(xflag && i > 9 ? "^0x%x" : "^%d", i);
}

static void
pr_uint64(uint64_t i, int xflag)
{
	printf(xflag ? " 0x%" PRIx64 : " %" PRIu64, i);
}

/*
 * pr_fact - print the factors of a number
 *
 * Print the factors of the number, from the lowest to the highest.
 * A factor will be printed multiple times if it divides the value
 * multiple times.
 *
 * If hflag is specified, duplicate factors are printed in "human-
 * readable" form of x^n.
 *
 * If the xflag is specified, numbers will be printed in hex.
 *
 * Factors are printed with leading tabs.
 */
static void
pr_fact(BIGNUM *val, int hflag, int xflag)
{
	int i;
	const uint64_t *fact;	/* The factor found. */

	/* Firewall - catch 0 and 1. */
	if (BN_is_zero(val))	/* Historical practice; 0 just exits. */
		exit(0);
	if (BN_is_one(val)) {
		printf("1: 1\n");
		return;
	}

	/* Factor value. */

	if (xflag) {
		fputs("0x", stdout);
		BN_print_fp(stdout, val);
	} else
		BN_print_dec_fp(stdout, val);
	putchar(':');
	fflush(stdout);
	for (fact = &prime[0]; !BN_is_one(val); ++fact) {
		/* Look for the smallest factor. */
		do {
			if (BN_mod_word(val, (BN_ULONG)*fact) == 0)
				break;
		} while (++fact <= pr_limit);

		/* Watch for primes larger than the table. */
		if (fact > pr_limit) {
#ifdef HAVE_OPENSSL
			BIGNUM *bnfact;

			bnfact = BN_new();
			BN_set_word(bnfact, *(fact - 1));
			if (!BN_sqr(bnfact, bnfact, ctx))
				errx(1, "error in BN_sqr()");
			if (BN_cmp(bnfact, val) > 0 ||
			    BN_is_prime_ex(val, PRIME_CHECKS, NULL, NULL) == 1)
				pr_print(val, hflag, xflag);
			else
				pollard_pminus1(val, hflag, xflag);
#else
			pr_print(val, hflag, xflag);
#endif
			pr_print(NULL, hflag, xflag);
			break;
		}

		/* Divide factor out until none are left. */
		i = 0;
		do {
			i++;
			if (!hflag)
				pr_uint64(*fact, xflag);
			BN_div_word(val, (BN_ULONG)*fact);
		} while (BN_mod_word(val, (BN_ULONG)*fact) == 0);

		if (hflag) {
			pr_uint64(*fact, xflag);
			if (i > 1)
				pr_exp(i, xflag);
		}

		/* Let the user know we're doing something. */
		fflush(stdout);
	}
	putchar('\n');
}

static void
pr_print(BIGNUM *val, int hflag, int xflag)
{
	static BIGNUM *sval;
	static int ex = 1;
	BIGNUM *pval;

	if (hflag) {
		if (sval == NULL) {
			sval = BN_dup(val);
			return;
		}

		if (val != NULL && BN_cmp(val, sval) == 0) {
			ex++;
			return;
		}
		pval = sval;
	} else if (val == NULL) {
		return;
	} else {
		pval = val;
	}

	if (xflag) {
		fputs(" 0x", stdout);
		BN_print_fp(stdout, pval);
	} else {
		putchar(' ');
		BN_print_dec_fp(stdout, pval);
	}

	if (hflag) {
		if (ex > 1)
			pr_exp(ex, xflag);

		if (val != NULL) {
			BN_copy(sval, val);
		} else {
			BN_free(sval);
			sval = NULL;
		}
		ex = 1;
	}
}

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-hx] [value ...]\n", getprogname());
	exit(1);
}

#ifdef HAVE_OPENSSL

/* pollard p-1, algorithm from Jim Gillogly, May 2000 */
static void
pollard_pminus1(BIGNUM *val, int hflag, int xflag)
{
	BIGNUM *base, *rbase, *num, *i, *x;

	base = BN_new();
	rbase = BN_new();
	num = BN_new();
	i = BN_new();
	x = BN_new();

	BN_set_word(rbase, 1);
newbase:
	if (!BN_add_word(rbase, 1))
		errx(1, "error in BN_add_word()");
	BN_set_word(i, 2);
	BN_copy(base, rbase);

	for (;;) {
		BN_mod_exp(base, base, i, val, ctx);
		if (BN_is_one(base))
			goto newbase;

		BN_copy(x, base);
		BN_sub_word(x, 1);
		if (!BN_gcd(x, x, val, ctx))
			errx(1, "error in BN_gcd()");

		if (!BN_is_one(x)) {
			if (BN_is_prime_ex(x, PRIME_CHECKS, NULL, NULL) == 1)
				pr_print(x, hflag, xflag);
			else
				pollard_pminus1(x, hflag, xflag);
			fflush(stdout);

			BN_div(num, NULL, val, x, ctx);
			if (BN_is_one(num))
				return;
			if (BN_is_prime_ex(num, PRIME_CHECKS, NULL,
			    NULL) == 1) {
				pr_print(num, hflag, xflag);
				fflush(stdout);
				return;
			}
			BN_copy(val, num);
		}
		if (!BN_add_word(i, 1))
			errx(1, "error in BN_add_word()");
	}
}

/*
 * Sigh..  No _decimal_ output to file functions in BN.
 */
static void
BN_print_dec_fp(FILE *fp, const BIGNUM *num)
{
	char *buf;

	buf = BN_bn2dec(num);
	if (buf == NULL)
		return;	/* XXX do anything here? */
	fprintf(fp, "%s", buf);
	free(buf);
}

#else

static void
BN_print_fp(FILE *fp, const BIGNUM *num)
{
	fprintf(fp, "%lx", (unsigned long)*num);
}

static void
BN_print_dec_fp(FILE *fp, const BIGNUM *num)
{
	fprintf(fp, "%lu", (unsigned long)*num);
}

static int
BN_dec2bn(BIGNUM **a, const char *str)
{
	char *p;

	errno = 0;
	**a = strtoul(str, &p, 10);
	return (errno == 0 ? 1 : 0);	/* OpenSSL returns 0 on error! */
}

static int
BN_hex2bn(BIGNUM **a, const char *str)
{
	char *p;

	errno = 0;
	**a = strtoul(str, &p, 16);
	return (errno == 0 ? 1 : 0);	/* OpenSSL returns 0 on error! */
}

static BN_ULONG
BN_div_word(BIGNUM *a, BN_ULONG b)
{
	BN_ULONG mod;

	mod = *a % b;
	*a /= b;
	return mod;
}

static BIGNUM *
BN_dup(const BIGNUM *a)
{
	BIGNUM *b = BN_new();
	BN_copy(b, a);
	return b;
}

#endif

/* Convert string pointed to by *str to a bignum.  */
static void
convert_str2bn(BIGNUM **val, char *p)
{
	int n = 0;

	if (*p == '+') p++;
	if (*p == '-')
		errx(1, "negative numbers aren't permitted.");
	if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
		n = BN_hex2bn(val, p + 2);
	} else {
		n = BN_dec2bn(val, p);
	}
	if (n == 0)
		errx(1, "%s: illegal numeric format.", p);
}
