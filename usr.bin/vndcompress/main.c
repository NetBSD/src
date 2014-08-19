/*	$NetBSD: main.c,v 1.1.2.3 2014/08/20 00:05:05 tls Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__RCSID("$NetBSD: main.c,v 1.1.2.3 2014/08/20 00:05:05 tls Exp $");

#include <assert.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

static int (*operation)(int, char **, const struct options *) = &vndcompress;

int
main(int argc, char **argv)
{
	static const struct options zero_options;
	struct options options = zero_options, *O = &options;
	int ch;

	setprogname(argv[0]);

	if (strcmp(getprogname(), "vndcompress") == 0)
		operation = &vndcompress;
	else if (strcmp(getprogname(), "vnduncompress") == 0)
		operation = &vnduncompress;
	else
		warnx("unknown program name, defaulting to vndcompress: %s",
		    getprogname());

	while ((ch = getopt(argc, argv, "b:cdk:l:p:rRw:")) != -1) {
		switch (ch) {
		case 'b':
			if (ISSET(O->flags, FLAG_b)) {
				warnx("-b may be supplied only once");
				usage();
			}
			O->flags |= FLAG_b;
			__CTASSERT(MIN_BLOCKSIZE <= MAX_BLOCKSIZE);
			__CTASSERT(MAX_BLOCKSIZE <= LLONG_MAX);
			O->blocksize = strsuftoll("block size", optarg,
			    MIN_BLOCKSIZE, MAX_BLOCKSIZE);
			break;

		case 'c':
			if (ISSET(O->flags, FLAG_d)) {
				warnx("-c and -d are mutually exclusive");
				usage();
			}
			O->flags |= FLAG_c;
			operation = &vndcompress;
			break;

		case 'd':
			if (ISSET(O->flags, FLAG_c)) {
				warnx("-c and -d are mutually exclusive");
				usage();
			}
			O->flags |= FLAG_d;
			operation = &vnduncompress;
			break;

		case 'k':
			if (ISSET(O->flags, FLAG_k)) {
				warnx("-k may be supplied only once");
				usage();
			}
			O->flags |= FLAG_k;
			O->checkpoint_blocks = strsuftoll("checkpoint blocks",
			    optarg,
			    0, MIN(UINT32_MAX, (OFF_MAX / MIN_BLOCKSIZE)));
			break;

		case 'l':
			if (ISSET(O->flags, FLAG_l)) {
				warnx("-l may be supplied only once");
				usage();
			}
			O->flags |= FLAG_l;
			O->length = strsuftoll("length", optarg,
			    0, MIN(OFF_MAX, UINT64_MAX));
			break;

		case 'p':
			O->flags |= FLAG_p;
			__CTASSERT(OFF_MAX <= LLONG_MAX);
			O->end_block = strsuftoll("end block", optarg,
			    0, MIN(UINT32_MAX, (OFF_MAX / MIN_BLOCKSIZE)));
			break;

		case 'r':
			O->flags |= FLAG_r;
			break;

		case 'R':
			O->flags |= FLAG_R;
			break;

		case 'w':
			if (ISSET(O->flags, FLAG_w)) {
				warnx("-w may be supplied only once");
				usage();
			}
			O->flags |= FLAG_w;
			O->window_size = strsuftoll("window size", optarg,
			    0, MAX_WINDOW_SIZE);
			break;

		case '?':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (operation == &vnduncompress) {
		if (ISSET(O->flags, ~(FLAG_d | FLAG_w)))
			usage();
	} else {
		assert(operation == &vndcompress);
		if (ISSET(O->flags, ~(FLAG_b | FLAG_c | FLAG_k | FLAG_l |
			    FLAG_p | FLAG_r | FLAG_R | FLAG_w)))
			usage();
		if (ISSET(O->flags, FLAG_R) && !ISSET(O->flags, FLAG_r)) {
			warnx("-R makes no sense without -r");
			usage();
		}
	}

	return (*operation)(argc, argv, O);
}

void __dead
usage(void)
{

	(void)fprintf(stderr,
	    "Usage: %s -c [-rR] [-b <blocksize>] [-k <checkpoint-blocks>]\n"
	    "          [-l <length>] [-p <partial-offset>] [-w <winsize>]\n"
	    "          <image> <compressed-image> [<blocksize>]\n"
	    "       %s -d <compressed-image> <image>\n",
	    getprogname(), getprogname());
	exit(1);
}
