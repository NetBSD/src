/*	$NetBSD: bin2blob.c,v 1.1 2008/05/04 23:50:32 ad Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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

/*
 * Reads data from stdin (must be a regular file so we can do stat()), and
 * outputs a 32-bit array of format:
 *
 * [0]	signature: BLOB
 * [1]	original, uncompressed length
 * [2]	length of compressed data that follows, unpadded
 * [n]	zlib compressed data, padded to 4 bytes
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: bin2blob.c,v 1.1 2008/05/04 23:50:32 ad Exp $");
#endif /* !lint */

#include <sys/module.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <zlib.h>

int	main(int, char **);

#define	SIG	0x424c4f42

void
put(uint32_t val)
{
	static int n;

	switch (n) {
	case 6:
		putc('\n', stdout);
		n = 0;
		/* FALLTHROUGH */
	case 0:
		putc('\t', stdout);
		break;
	}
	printf("0x%08x,", val);
	n++;
}

int
main(int argc, char **argv)
{
	struct stat sb;
	uint8_t *src, *dst;
	u_long slen, dlen;
	uint32_t *dp;

	if (fstat(STDIN_FILENO, &sb) < 0)
		err(1, "stat(stdin)");

	src = malloc(sb.st_size);
	dlen = sb.st_size * 1002 / 1000 + 12 + 3 + 4*2;
	dst = malloc(dlen);
	if (src == NULL || dst == NULL)
		errx(1, "malloc");

	if (read(STDIN_FILENO, src, sb.st_size) != sb.st_size)
		errx(1, "read");

	slen = sb.st_size;
	memset(dst, 0, dlen);
	if (compress2(dst, &dlen, src, slen, 9) != Z_OK)
		errx(1, "compress2");

	printf("static const uint32_t blob[] = {\n");
	put(SIG);
	put(slen);
	put(dlen);
	dlen = ((dlen + 3) & ~3) >> 2;
	dp = (uint32_t *)dst;
	while (dlen--)
		put(*dp++);
	printf("\n};\n");

	exit(EXIT_SUCCESS);
}
