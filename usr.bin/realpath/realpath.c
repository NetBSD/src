/*	$NetBSD: realpath.c,v 1.3 2023/05/25 17:24:17 kre Exp $	*/
/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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

#include <sys/cdefs.h>
#if !defined(lint)
#if 0
__FBSDID("$FreeBSD: head/bin/realpath/realpath.c 326025 2017-11-20 19:49:47Z pfg $");
#else
__RCSID("$NetBSD: realpath.c,v 1.3 2023/05/25 17:24:17 kre Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool process(char *path);
static void usage(void) __dead;

static const char options[] = "Eeq";

char dot[] = ".";

bool eflag = false;		/* default to -E mode */
bool qflag = false;

int
main(int argc, char *argv[])
{
	char *path;
	int ch, rval;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, options)) != -1) {
		switch (ch) {
		case 'e':
			eflag = true;
			break;
		case 'E':
			eflag = false;
			break;
		case 'q':
			qflag = true;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	path = *argv != NULL ? *argv++ : dot;
	rval = 0;
	do {
		if (path[0] == '\0') {
			/* ignore -q for this one */
			warnx("Invalid path ''");
			rval = 1;
			continue;
		}
		if (!process(path))
			rval = 1;;
	} while ((path = *argv++) != NULL);
	exit(rval);
}

static bool
process(char *path)
{
	char buf[PATH_MAX];
	char buf2[sizeof buf];
	char lbuf[PATH_MAX];
	char *pp, *p, *q, *r, *s;
	struct stat sb;
	bool dir_reqd = false;

	if ((p = realpath(path, buf)) != NULL) {
		(void)printf("%s\n", p);
		return true;
	}

	if (eflag || errno != ENOENT) {
		if (!qflag)
			warn("%s", path);
		return false;
	}

	p = strrchr(path, '/');
	while (p != NULL && p > &path[1] && p[1] == '\0') {
		dir_reqd = true;
		*p = '\0';
		p = strrchr(path, '/');
	}

	if (p == NULL) {
		p = realpath(".", buf);
		if (p == NULL) {
			warnx("relative path; current location unknown");
			return false;
		}
		if ((size_t)snprintf(buf2, sizeof buf2, "%s/%s", buf, path)
		    >= sizeof buf2) {
			if (!qflag)
				warnx("%s/%s: path too long", p, path);
			return false;
		}
		path = buf2;
		p = strrchr(path, '/');
		if (p == NULL)
			abort();
	}

	*p = '\0';
	pp = ++p;

	q = path; r = buf; s = buf2;
	while (realpath(*q ? q : "/", r) != NULL) {
		ssize_t llen;

		if (strcmp(r, "/") == 0 || strcmp(r, "//") == 0)
			r++;
		if ((size_t)snprintf(s, sizeof buf, "%s/%s", r, pp)
		    >= sizeof buf)
			return false;

		if (lstat(s, &sb) == -1 || !S_ISLNK(sb.st_mode)) {
			(void)printf("%s\n", s);
			return true;
		}

		q = strchr(r, '\0');
		if (q >= &r[sizeof buf - 3]) {
			*p = '/';
			if (!qflag)
				warnx("Expanded path for %s too long\n", path);
			return false;
		}

		if ((llen = readlink(s, lbuf, sizeof lbuf - 2)) == -1) {
			*p = '/';
			if (!qflag)
				warn("%s", path);
			return false;
		}
		lbuf[llen] = '\0';

		if (lbuf[0] == '/') {
			q = lbuf;
			if (dir_reqd) {
				lbuf[llen++] = '/';
				lbuf[llen] = '\0';
			}
		} else {
			if (q != buf2) {
				q = buf2;
				r = buf;
			} else {
				q = buf;
				r = buf2;
			}

			if ((size_t)snprintf(q, sizeof buf, "%s/%s%s", r, lbuf,
			    (dir_reqd ? "/" : "")) >= sizeof buf) {
				*p = '/';
				if (!qflag)
					warnx("Expanded path for %s too long\n",
					    path);
				return false;
			}
		}

		s = realpath(q, r);
		if (s != NULL) {
			/* this case should almost never happen (race) */
			(void)printf("%s\n", s);
			return true;
		}
		if (errno != ENOENT) {
			*p = '/';
			if (!qflag)
				warn("%s", path);
			return false;
		}

		pp = strrchr(q, '/');
		if (pp == NULL) {
			/* we just put one there, where did it go? */
			abort();
		}
		if (dir_reqd) {
			*pp = '\0';
			pp = strrchr(q, '/');
			if (pp == NULL)
				abort();
		}
		*pp++ = '\0';

		s = q;
	}

	*p = '/';

	if (!qflag)
		warn("%s", path);
	return false;
}

static void
usage(void)
{

	(void)fprintf(stderr, "usage: %s [-%s] [path ...]\n",
	    getprogname(), options);
  	exit(1);
}
