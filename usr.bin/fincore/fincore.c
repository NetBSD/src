/*	$NetBSD: fincore.c,v 1.1.4.2 2012/04/17 00:09:31 yamt Exp $	*/

/*-
 * Copyright (c) 2011 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * a utility to query which file pages are cached
 *
 * inspired by:
 * 	http://net.doit.wisc.edu/~plonka/fincore/
 * 	http://www.usenix.org/events/lisa07/tech/plonka.html
 */

#include <sys/cdefs.h>
#if defined(__NetBSD__)
#ifndef lint
__RCSID("$NetBSD: fincore.c,v 1.1.4.2 2012/04/17 00:09:31 yamt Exp $");
#endif /* not lint */
#endif /* defined(__NetBSD__) */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if !defined(__arraycount)
#define	__arraycount(a)	(sizeof(a)/sizeof(*a))
#endif /* !defined(__arraycount) */

size_t page_size;
bool do_summary;
bool be_quiet;

/*
 * fincore: query which pages of the file are in-core.
 *
 * this function is intended to be compatible with:
 * 	http://lwn.net/Articles/371538/
 * 	http://libprefetch.cs.ucla.edu/
 *
 * while this can be implemented in kernel much more efficiently, i'm not
 * sure if making this a syscall in 2011 is a good idea.  this API does not
 * seem scalable for sparsely cached huge files.  the expected scalability
 * has been changed since the time when mincore was invented.
 *
 * some references:
 * 	http://wiki.postgresql.org/images/a/a2/Pgfincore_pgday10.pdf
 */

static int
fincore(int fd, off_t startoff, off_t endoff, unsigned char *vec)
{
	off_t off;
	size_t chunk_size;

	for (off = startoff; off < endoff;
	    off += chunk_size, vec += chunk_size / page_size) {
		void *vp;

		chunk_size = MIN((off_t)(1024 * page_size), endoff - off);
		vp = mmap(NULL, chunk_size, PROT_NONE, MAP_FILE|MAP_SHARED,
		    fd, off);
		if (vp == MAP_FAILED) {
			return -1;
		}
		if (mincore(vp, chunk_size,
#if !defined(__linux__)
		    (char *)
#endif /* !defined(__linux__) */
		    vec)) {
			munmap(vp, chunk_size);
			return -1;
		}
		if (munmap(vp, chunk_size)) {
			return -1;
		}
	}
	return 0;
}

static void
do_file(const char *name)
{
	unsigned char vec[4096];
	struct stat st;
	uintmax_t n;	/* number of pages in-core */
	off_t off;
	size_t chunk_size;
	int fd;
	bool header_done = false;

	fd = open(name, O_RDONLY);
	if (fd == -1) {
		err(EXIT_FAILURE, "open %s", name);
	}
	if (fstat(fd, &st)) {
		err(EXIT_FAILURE, "fstat %s", name);
	}
	n = 0;
	for (off = 0; off < st.st_size; off += chunk_size) {
		unsigned int i;

		chunk_size = MIN(__arraycount(vec) * page_size,
		    roundup(st.st_size - off, page_size));
		if (fincore(fd, off, off + chunk_size, vec)) {
			printf("\n");
			err(EXIT_FAILURE, "fincore %s", name);
		}
		for (i = 0; i < chunk_size / page_size; i++) {
			if (vec[i] == 0) {
				continue;
			}
			if (!do_summary) {
				if (!header_done) {
					printf("%s:", name);
					header_done = true;
				}
				printf(" %ju",
				    (uintmax_t)(off / page_size + i));
			}
			n++;
		}
	}
	close(fd);
	if (do_summary && (n != 0 || !be_quiet)) {
		const uintmax_t total = howmany(st.st_size, page_size);
		const double pct = (total != 0) ? ((double)n / total * 100) : 0;

		if (!header_done) {
			printf("%s:", name);
			header_done = true;
		}
		printf(" %ju / %ju in-core pages (%0.2f%%)", n, total, pct);
	}
	if (header_done) {
		printf("\n");
	} else if (!be_quiet) {
		printf("%s: \n", name);
	}
}

int
/*ARGSUSED*/
main(int argc, char *argv[])
{
	extern int optind;
	long l;
	int ch;

	while ((ch = getopt(argc, argv, "sq")) != -1) {
		switch (ch) {
		case 's':
			do_summary = true;
			break;
		case 'q':
			be_quiet = true;
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}

	l = sysconf(_SC_PAGESIZE);
	if (l == -1) {
		/*
		 * sysconf doesn't always set errno.  bad API.
		 */
		errx(EXIT_FAILURE, "_SC_PAGESIZE");
	}
	page_size = (size_t)l;

	argc -= optind;
	argv += optind;
	while (argc > 0) {
		do_file(argv[0]);
		argc--;
		argv++;
	}
	return EXIT_SUCCESS;
}
