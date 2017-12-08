/*	$NetBSD: h_segv.c,v 1.2 2017/12/08 14:40:45 christos Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: h_segv.c,v 1.2 2017/12/08 14:40:45 christos Exp $");

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <err.h>

// A faulting address
static int *p = (int *) 0xfefefef0;

static int flags;
#define F_RECURSE 	1
#define F_HANDLE	2
#define F_MASK		4

static struct {
	const char *n;
	int v;
} nv[] = {
	{ "recurse",	F_RECURSE },
	{ "handle",	F_HANDLE },
	{ "mask",	F_MASK },
};

static void
foo(int s)
{
        char buf[64];
        int i = snprintf(buf, sizeof(buf), "got %d\n", s);
        write(2, buf, i);
	if (flags & F_RECURSE)
		*p = 0;
        exit(EXIT_SUCCESS);
}

static __dead void
usage(void)
{
	fprintf(stderr, "Usage: %s recurse|mask|handle ...\n", getprogname());
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	if (argc == 1)
	    usage();

	for (int i = 1; i < argc; i++) {
		size_t j;
		for (j = 0; j < __arraycount(nv); j++)
			if (strcmp(nv[j].n, argv[i]) == 0) {
				flags |= nv[j].v;
				break;
			}
		if (j == __arraycount(nv))
			usage();
	}

	if (flags == 0)
		usage();

	if (flags & F_HANDLE) {
		struct sigaction sa;

		sa.sa_flags = SA_RESTART;
		sa.sa_handler = foo;
		sigemptyset(&sa.sa_mask);
		if (sigaction(SIGSEGV, &sa, NULL) == -1)
			err(EXIT_FAILURE, "sigaction");
	}

	if (flags & F_MASK) {
		sigset_t set;

		sigemptyset(&set);
		sigaddset(&set, SIGSEGV);
		if (sigprocmask(SIG_BLOCK, &set, NULL) == -1)
			err(EXIT_FAILURE, "sigprocmask");
	}

        *p = 1;
	return EXIT_SUCCESS;
}
