/*	$NetBSD: awaitkey.c,v 1.1.6.2 2013/02/25 00:28:48 tls Exp $	*/

/*-
 * Copyright (c) 2013 Izumi Tsutsui.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libkern/libkern.h>
#include <luna68k/stand/boot/samachdep.h>

static void print_countdown(const char *, int);

#define FMTLEN	40

static void
print_countdown(const char *pfmt, int n)
{
	int len, i;
	char fmtbuf[FMTLEN];

	len = snprintf(fmtbuf, FMTLEN, pfmt, n);
	printf("%s", fmtbuf);
	for (i = 0; i < len; i++)
		putchar('\b');
}

/*
 * awaitkey(const char *pfmt, int timeout, bool tell)
 *
 * Wait timeout seconds until any input from stdin.
 * print countdown message using "pfmt" if tell is true.
 * Requires tgetchar(), which returns 0 if there is no input.
 */
char
awaitkey(const char *pfmt, int timeout, bool tell)
{
	uint32_t otick;
	char c = 0;

	if (timeout <= 0)
		goto out;

	if (tell)
		print_countdown(pfmt, timeout);

	otick = tick;

	for (;;) {
		c = tgetchar();
		if (c != 0)
			break;
		if (tick - otick >= hz) {
			otick = tick;
			if (--timeout == 0)
				break;
			if (tell)
				print_countdown(pfmt, timeout);
		}
	}

 out:
	if (tell) {
		printf(pfmt, 0);
		printf("\n");
	}
	return c;
}
