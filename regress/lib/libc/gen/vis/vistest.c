/*	$NetBSD: vistest.c,v 1.1 2002/03/23 17:42:21 christos Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Christos Zoulas.
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

#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <vis.h>

static int styles[] = {
	VIS_OCTAL,
	VIS_CSTYLE,
	VIS_SP,
	VIS_TAB,
	VIS_NL,
	VIS_WHITE,
	VIS_SAFE,
#if 0	/* Not reversible */
	VIS_NOSLASH,
#endif
#ifdef VIS_HTTPSTYLE
	VIS_HTTPSTYLE
#endif
};

#define SIZE	256

int
main(int argc, char *argv[])
{
	char *srcbuf = malloc(SIZE);
	char *dstbuf = malloc(SIZE);
	char *visbuf = malloc(SIZE * 4 + 1);
	int i, j;
	int n = sizeof(styles) / sizeof(styles[0]);

	for (i = 0; i < SIZE; i++)
		srcbuf[i] = (char)i;

	for (i = 0; i < n; i++) {
		strsvisx(visbuf, srcbuf, SIZE, styles[i], "");
		/*
		 * XXX: The strunvisx api is busted; flags should be
		 * UNVIS_ flags, buf we follow FreeBSD's lead. This
		 * needs to be redone, by moving UNVIS_END into the
		 * VIS_ space, and bump the library/symbol.
		 */
		strunvisx(dstbuf, visbuf, styles[i] & VIS_HTTPSTYLE);
		for (j = 0; j < SIZE; j++)
			if (dstbuf[j] != (char)j)
				errx(1, "Failed for style %x, char %d [%d]",
				    styles[i], j, dstbuf[j]);
	}
	return 0;
}
