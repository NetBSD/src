/*	$NetBSD: snprintbtest.c,v 1.2.30.1 2008/05/18 12:30:47 yamt Exp $	*/

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
#include <util.h>
#include <err.h>

struct {
	const char *fmt;
	uint64_t val;
	const char *res;
} test[] = {
	{ 
		"\10\2BITTWO\1BITONE",
		3, 
		"03<BITTWO,BITONE>"
	},
	{
		"\177\20b\05NOTBOOT\0b\06FPP\0b\013SDVMA\0b\015VIDEO\0"
		"b\020LORES\0b\021FPA\0b\022DIAG\0b\016CACHE\0"
		"b\017IOCACHE\0b\022LOOPBACK\0b\04DBGCACHE\0",
		0xe860,
		"0xe860<NOTBOOT,FPP,SDVMA,VIDEO,CACHE,IOCACHE>"
	}
};

int
main(int argc, char *argv[])
{
	char buf[1024];
	size_t i;

	for (i = 0; i < sizeof(test) / sizeof(test[0]); i++) {
		int len = snprintb(buf, sizeof(buf), test[i].fmt, test[i].val);
		int slen = (int)strlen(test[i].res);
		if (strcmp(test[i].res, buf) != 0)
			errx(1, "Unexpected result `%s' != `%s'\n",
			    buf, test[i].res);
		if (len != slen)
			errx(1, "Unexpected len in result %d != %d\n",
			    len, slen);
	}
	return 0;
}
