/*	$NetBSD: netgroup.c,v 1.5 2007/12/15 19:44:52 perry Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__RCSID("$NetBSD: netgroup.c,v 1.5 2007/12/15 19:44:52 perry Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netgroup.h>

static void usage __P((void)) __dead;
int main __P((int, char *[]));

static void
usage()
{

	(void)fprintf(stderr, "usage: %s [-hud] <netgroup>\n", getprogname());
	exit(1);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int c, i = 0;
	const char *p[3];

	while ((c = getopt(argc, argv, "hud")) != -1)
		switch (c) {
		case 'h':
			i = 0;
			break;
		case 'u':
			i = 1;
			break;
		case 'd':
			i = 2;
			break;
		default:
			usage();
		}

	if (optind >= argc)
		usage();

	for(; optind < argc; optind++) {
		setnetgrent(argv[optind]);
		while (getnetgrent(&p[0], &p[1], &p[2]))
			if (p[i])
				printf("%s\n", p[i]);
		endnetgrent();
	}
	return 0;
}
