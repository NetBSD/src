/*	$NetBSD: h_sort.c,v 1.3 2025/03/02 23:11:19 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: h_sort.c,v 1.3 2025/03/02 23:11:19 riastradh Exp $");

#include <assert.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
heapsort_r_wrapper(void *a, size_t n, size_t sz,
    int (*cmp)(const void *, const void *, void *), void *cookie)
{

	if (heapsort_r(a, n, sz, cmp, cookie) == -1)
		err(1, "heapsort_r");
}

static void
mergesort_r_wrapper(void *a, size_t n, size_t sz,
    int (*cmp)(const void *, const void *, void *), void *cookie)
{

	if (mergesort_r(a, n, sz, cmp, cookie) == -1)
		err(1, "mergesort_r");
}

struct context {
	const char	*buf;
	const size_t	*linepos;
};

static int
cmp(const void *va, const void *vb, void *cookie)
{
	const struct context *C = cookie;
	const size_t *a = va;
	const size_t *b = vb;

	return strcmp(C->buf + C->linepos[*a], C->buf + C->linepos[*b]);
}

static void __dead
usage(void)
{

	fprintf(stderr, "Usage: %s [-n] <sortfn>\n", getprogname());
	exit(1);
}

int
main(int argc, char **argv)
{
	int ch;
	int nflag = 0;
	void (*sortfn)(void *, size_t, size_t,
	    int (*)(const void *, const void *, void *), void *);
	char *buf = NULL;
	size_t nbuf;
	size_t *linepos = NULL;
	size_t nlines;
	size_t *permutation = NULL;
	size_t off;
	ssize_t nread;
	char *p;
	size_t i;
	int error;

	/*
	 * Parse arguments.
	 */
	setprogname(argv[0]);
	while ((ch = getopt(argc, argv, "hn")) != -1) {
		switch (ch) {
		case 'n':
			nflag = 1;
			break;
		case '?':
		case 'h':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage();
	if (strcmp(argv[0], "heapsort_r") == 0)
		sortfn = &heapsort_r_wrapper;
	else if (strcmp(argv[0], "mergesort_r") == 0)
		sortfn = &mergesort_r_wrapper;
	else if (strcmp(argv[0], "qsort_r") == 0)
		sortfn = &qsort_r;
	else
		errx(1, "unknown sort: %s", argv[0]);

	/*
	 * Allocate an initial 4K buffer.
	 */
	nbuf = 0x1000;
	error = reallocarr(&buf, nbuf, 1);
	if (error)
		errc(1, error, "reallocarr");

	/*
	 * Read the input into a contiguous buffer.  Reject input with
	 * embedded NULs so we can use strcmp(3) to compare lines.
	 */
	off = 0;
	while ((nread = read(STDIN_FILENO, buf + off, nbuf - off - 1)) != 0) {
		if (nread == -1)
			err(1, "read");
		if ((size_t)nread > nbuf - off - 1)
			errx(1, "overlong read: %zu", (size_t)nread);
		if (memchr(buf + off, '\0', (size_t)nread) != NULL)
			errx(1, "NUL byte in input");
		off += (size_t)nread;

		/*
		 * If we filled the buffer, reallocate it with double
		 * the size.  Bail if that would overflow.
		 */
		if (nbuf - off == 1) {
			if (nbuf > SIZE_MAX/2)
				errx(1, "input overflow");
			nbuf *= 2;
			error = reallocarr(&buf, nbuf, 1);
			if (error)
				errc(1, error, "reallocarr");
		}
	}

	/*
	 * If the input was empty, nothing to do.
	 */
	if (off == 0)
		return 0;

	/*
	 * NUL-terminate the input and count the lines.  The last line
	 * may have no trailing \n.
	 */
	buf[off] = '\0';
	nlines = 1;
	for (p = buf; (p = strchr(p, '\n')) != NULL;) {
		if (*++p == '\0')
			break;
		nlines++;
	}

	/*
	 * Create an array of line positions to sort.  NUL-terminate
	 * each line so we can use strcmp(3).
	 */
	error = reallocarr(&linepos, nlines, sizeof(linepos[0]));
	if (error)
		errc(1, error, "reallocarr");
	i = 0;
	for (p = buf; linepos[i++] = p - buf, (p = strchr(p, '\n')) != NULL;) {
		*p = '\0';
		if (*++p == '\0')
			break;
	}
	assert(i == nlines);

	/*
	 * Create an array of permuted line numbers.
	 */
	error = reallocarr(&permutation, nlines, sizeof(permutation[0]));
	if (error)
		errc(1, error, "reallocarr");
	for (i = 0; i < nlines; i++)
		permutation[i] = i;

	/*
	 * Sort the lines via comparison function that consults the
	 * buffer as a cookie.
	 */
	(*sortfn)(permutation, nlines, sizeof(permutation[0]), &cmp,
	    &(struct context) { .buf = buf, .linepos = linepos });

	/*
	 * Print the lines in sorted order with the original line
	 * numbers.
	 */
	for (i = 0; i < nlines; i++) {
		const size_t j = permutation[i];
		if (nflag)
			printf("%zu %s\n", j, buf + linepos[j]);
		else
			printf("%s\n", buf + linepos[j]);
	}
	fflush(stdout);
	return ferror(stdout);
}
