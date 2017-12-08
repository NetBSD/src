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
__KERNEL_RCSID(0, "$NetBSD: hexdump.c,v 1.3 2017/12/08 23:57:57 christos Exp $");

#ifdef DEBUG_HEXDUMP
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
static const char hexdigits[] = "0123456789abcdef";
#else
#include <lib/libkern/libkern.h>
#include <sys/systm.h>
#endif

#define MID (3 * 8)
#define BAR ((3 * 16) + 1)
#define ASC (BAR + 2)
#define NL (BAR + 18)

void
hexdump(const char *msg, const void *ptr, size_t len)
{
	size_t i, p, q;
	const unsigned char *u = ptr;
	char buf[NL + 2];

	if (msg)
		printf("%s: %zu bytes @ %p\n", msg, len, ptr);

	buf[BAR] = '|';
	buf[BAR + 1] = ' ';
	buf[NL] = '\n';
	buf[NL + 1] = '\0';
	memset(buf, ' ', BAR);
        for (q = p = i = 0; i < len; i++) {
		unsigned char c = u[i];
		buf[p++] = hexdigits[(c >> 4) & 0xf];
		buf[p++] = hexdigits[(c >> 0) & 0xf];
		buf[p++] = ' ';
                if (q == 7)
		       buf[p++] = ' ';

		buf[ASC + q++] = isprint(c) ? c : '.';

		if (q == 16) {
			q = p = 0;
			printf("%s", buf);
			memset(buf, ' ', BAR);
		}
        }
	if (q) {
		buf[ASC + q++] = '\n';
		buf[ASC + q] = '\0';
		printf("%s", buf);
	}
}

#ifdef DEBUG_HEXDUMP
int
main(int argc, char *argv[])
{
	hexdump("foo", main, atoi(argv[1]));
	return 0;
}
#endif
