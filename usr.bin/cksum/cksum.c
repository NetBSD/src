/*	$NetBSD: cksum.c,v 1.9 1997/06/26 23:24:01 kleink Exp $	*/

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

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)cksum.c	8.2 (Berkeley) 4/28/95";
#endif
static char rcsid[] = "$NetBSD: cksum.c,v 1.9 1997/06/26 23:24:01 kleink Exp $";
#endif /* not lint */

#include <sys/cdefs.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

int md5_digest_file __P((char *));
void requiremd5 __P((const char *));
void usage __P((void));

int
main(argc, argv)
	int argc;
	char **argv;
{
	register int ch, fd, rval, domd5, dosum, pflag, nomd5stdin;
	u_int32_t len, val;
	char *fn;
	int (*cfncn) __P((int, u_int32_t *, u_int32_t *));
	void (*pfncn) __P((char *, u_int32_t, u_int32_t));
	extern char *__progname;

	dosum = domd5 = pflag = nomd5stdin = 0;

	setlocale(LC_ALL, "");

	if (!strcmp(__progname, "md5"))
		domd5 = 1;
	else if (!strcmp(__progname, "sum")) {
		dosum = 1;
		cfncn = csum1;
		pfncn = psum1;
	} else {
		cfncn = crc;
		pfncn = pcrc;
	}

	while ((ch = getopt(argc, argv, "mo:ps:tx")) != -1)
		switch(ch) {
		case 'm':
			if (dosum) {
				warnx("sum mutually exclusive with md5");
				usage();
			}
			domd5 = 1;
			break;
		case 'o':
			if (domd5) {
				warnx("md5 mutually exclusive with sum");
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
			if (!domd5)
				requiremd5("-p");
			pflag = 1;
			break;
		case 's':
			if (!domd5)
				requiremd5("-s");
			nomd5stdin = 1;
			MDString(optarg);
			break;
		case 't':
			if (!domd5)
				requiremd5("-t");
			MDTimeTrial();
			nomd5stdin = 1;
			break;
		case 'x':
			if (!domd5)
				requiremd5("-x");
			MDTestSuite();
			nomd5stdin = 1;
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
			if (domd5) {
				if (md5_digest_file(fn)) {
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
		} else if (domd5 && !nomd5stdin)
			MDFilter(pflag);

		if (!domd5) {
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
md5_digest_file(fn)
	char *fn;
{
	char buf[33], *cp;

	cp = MD5File(fn, buf);
	if (cp == NULL)
		return (1);

	printf("MD5 (%s) = %s\n", fn, cp);
	return (0);
}

void
requiremd5(flg)
	const char *flg;
{
	warnx("%s flag requires `md5' or -m", flg);
	usage();
}

void
usage()
{

	(void)fprintf(stderr, "usage: cksum [-m | [-o 1 | 2]] [file ...]\n");
	(void)fprintf(stderr, "       sum [file ...]\n");
	(void)fprintf(stderr,
	    "       md5 [-p | -t | -x | -s string] [file ...]\n");
	exit(1);
}
