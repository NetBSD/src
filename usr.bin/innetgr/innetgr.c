/*	$NetBSD: innetgr.c,v 1.10 2016/09/03 05:50:06 dholland Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: innetgr.c,v 1.10 2016/09/03 05:50:06 dholland Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netgroup.h>

static __dead void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: %s [-v] [-d domain] [-h host] [-u user] netgroup\n",
	    getprogname());
	exit(2);
}

int
main(int argc, char *argv[])
{
	int c, ok, verbose = 0;
	char *user = NULL;
	char *host = NULL;
	char *domain = NULL;

	while ((c = getopt(argc, argv, "h:u:d:v")) != -1)
		switch (c) {
		case 'u':
		   	user = optarg;
			break;
		case 'h':
			host = optarg;
			break;
		case 'd':
			domain = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
		}

	if (optind >= argc)
		usage();

	for(; optind < argc; optind++) {
		ok = innetgr(argv[optind], host, user, domain);
		if (verbose)
			printf("%s: %d\n", argv[optind], ok);
		if (ok)
			return 0;
	}
	return 1;
}
