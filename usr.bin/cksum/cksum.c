/*	$NetBSD: cksum.c,v 1.15 2001/03/21 03:16:38 atatat Exp $	*/

/*-
 * Copyright (c) 1997 Jason R. Thorpe.  All rights reserved.
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James W. Williams of NASA Goddard Space Flight Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)cksum.c	8.2 (Berkeley) 4/28/95";
#endif
__RCSID("$NetBSD: cksum.c,v 1.15 2001/03/21 03:16:38 atatat Exp $");
#endif /* not lint */

#include <sys/cdefs.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <md5.h>
#include <md4.h>
#include <md2.h>
#include <sha1.h>
#include <rmd160.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#define HASH_MD2	0
#define HASH_MD4	1
#define HASH_MD5	2
#define HASH_SHA1	3
#define HASH_RMD160	4

typedef char *(*_filefunc)(const char *, char *);

struct hash {
	const char *progname;
	const char *hashname;
	void (*stringfunc)(const char *);
	void (*timetrialfunc)(void);
	void (*testsuitefunc)(void);
	void (*filterfunc)(int);
	char *(*filefunc)(const char *, char *);
} hashes[] = {
	{ "md2", "MD2",
	  MD2String, MD2TimeTrial, MD2TestSuite,
	  MD2Filter, MD2File },
	{ "md4", "MD4",
	  MD4String, MD4TimeTrial, MD4TestSuite,
	  MD4Filter, MD4File },
	{ "md5", "MD5",
	  MD5String, MD5TimeTrial, MD5TestSuite,
	  MD5Filter, MD5File },
	{ "sha1", "SHA1",
	  SHA1String, SHA1TimeTrial, SHA1TestSuite,
	  SHA1Filter, (_filefunc) SHA1File },
	{ "rmd160", "RMD160",
	  RMD160String, RMD160TimeTrial, RMD160TestSuite,
	  RMD160Filter, (_filefunc) RMD160File },
	{ NULL }
};

int	main __P((int, char **));
int	hash_digest_file __P((char *, struct hash *));
void	requirehash __P((const char *));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char **argv;
{
	register int ch, fd, rval, dosum, pflag, nohashstdin;
	u_int32_t len, val;
	char *fn;
	const char *progname;
	int (*cfncn) __P((int, u_int32_t *, u_int32_t *));
	void (*pfncn) __P((char *, u_int32_t, u_int32_t));
	struct hash *hash;

	cfncn = NULL;
	pfncn = NULL;
	dosum = pflag = nohashstdin = 0;

	setlocale(LC_ALL, "");

	progname = getprogname();

	for (hash = hashes; hash->hashname != NULL; hash++)
		if (strcmp(progname, hash->progname) == 0)
			break;

	if (hash->hashname == NULL) {
		hash = NULL;

		if (!strcmp(progname, "sum")) {
			dosum = 1;
			cfncn = csum1;
			pfncn = psum1;
		} else {
			cfncn = crc;
			pfncn = pcrc;
		}
	}

	while ((ch = getopt(argc, argv, "mo:ps:tx12456")) != -1)
		switch(ch) {
		case '2':
			if (dosum) {
				warnx("sum mutually exclusive with md2");
				usage();
			}
			hash = &hashes[HASH_MD2];
			break;
		case '4':
			if (dosum) {
				warnx("sum mutually exclusive with md4");
				usage();
			}
			hash = &hashes[HASH_MD4];
			break;
		case 'm':
		case '5':
			if (dosum) {
				warnx("sum mutually exclusive with md5");
				usage();
			}
			hash = &hashes[HASH_MD5];
			break;
		case '1':
			if (dosum) {
				warnx("sum mutually exclusive with sha1");
				usage();
			}
			hash = &hashes[HASH_SHA1];
			break;
		case '6':
			if (dosum) {
				warnx("sum mutually exclusive with rmd160");
				usage();
			}
			hash = &hashes[HASH_RMD160];
			break;
		case 'o':
			if (hash) {
				warnx("%s mutually exclusive with sum",
				      hash->hashname);
				usage();
			}
			if (!strcmp(optarg, "1")) {
				cfncn = csum1;
				pfncn = psum1;
			} else if (!strcmp(optarg, "2")) {
				cfncn = csum2;
				pfncn = psum2;
			} else {
				warnx("illegal argument to -o option");
				usage();
			}
			break;
		case 'p':
			if (hash == NULL)
				requirehash("-p");
			pflag = 1;
			break;
		case 's':
			if (hash == NULL)
				requirehash("-s");
			nohashstdin = 1;
			hash->stringfunc(optarg);
			break;
		case 't':
			if (hash == NULL)
				requirehash("-t");
			nohashstdin = 1;
			hash->timetrialfunc();
			break;
		case 'x':
			if (hash == NULL)
				requirehash("-x");
			nohashstdin = 1;
			hash->testsuitefunc();
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	fd = STDIN_FILENO;
	fn = NULL;
	rval = 0;
	do {
		if (*argv) {
			fn = *argv++;
			if (hash != NULL) {
				if (hash_digest_file(fn, hash)) {
					warn("%s", fn);
					rval = 1;
				}
				continue;
			}
			if ((fd = open(fn, O_RDONLY, 0)) < 0) {
				warn("%s", fn);
				rval = 1;
				continue;
			}
		} else if (hash && !nohashstdin) {
			hash->filterfunc(pflag);
		}

		if (hash == NULL) {
			if (cfncn(fd, &val, &len)) {
				warn("%s", fn ? fn : "stdin");
				rval = 1;
			} else
				pfncn(fn, val, len);
			(void)close(fd);
		}
	} while (*argv);
	exit(rval);
}

int
hash_digest_file(fn, hash)
	char *fn;
	struct hash *hash;
{
	char buf[41], *cp;

	cp = hash->filefunc(fn, buf);
	if (cp == NULL)
		return (1);

	printf("%s (%s) = %s\n", hash->hashname, fn, cp);
	return (0);
}

void
requirehash(flg)
	const char *flg;
{
	warnx("%s flag requires `md2', `md4', `md5', `sha1', or `rmd160'",
	    flg);
	usage();
}

void
usage()
{

	(void)fprintf(stderr, "usage: cksum [-m | [-o 1 | 2]] [file ...]\n");
	(void)fprintf(stderr, "       sum [file ...]\n");
	(void)fprintf(stderr,
	    "       md2 [-p | -t | -x | -s string] [file ...]\n");
	(void)fprintf(stderr,
	    "       md4 [-p | -t | -x | -s string] [file ...]\n");
	(void)fprintf(stderr,
	    "       md5 [-p | -t | -x | -s string] [file ...]\n");
	(void)fprintf(stderr,
	    "       sha1 [-p | -t | -x | -s string] [file ...]\n");
	(void)fprintf(stderr,
	    "       rmd160 [-p | -t | -x | -s string] [file ...]\n");
	exit(1);
}
