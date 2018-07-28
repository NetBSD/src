/*	$NetBSD: base64.c,v 1.2.2.2 2018/07/28 04:38:13 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: base64.c,v 1.2.2.2 2018/07/28 04:38:13 pgoyette Exp $");

#include <ctype.h>
#include <errno.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static size_t
getinput(FILE *fin, uint8_t in[3])
{
	size_t res;
	int c;

	for (res = 0; res < 3 && (c = getc(fin)) != EOF; res++)
		in[res] = (uint8_t)c;
	for (size_t i = res; i < 3; i++)
		in[i] = 0;

	return res;
}

static int
putoutput(FILE *fout, uint8_t out[4], size_t len, size_t wrap, size_t *pos)
{
	size_t i;

	for (i = 0; i < len + 1; i++) {
		if (out[i] >= 64) {
			return EINVAL;
		}
		if (fputc(B64[out[i]], fout) == -1)
			return errno;
		if (++(*pos) == wrap) {
			if (fputc('\n', fout) == -1)
				return errno;
			*pos = 0;
		}
	}
	for (; i < 4; i++) {
		if (fputc('=', fout) == -1)
			return errno;
		if (++(*pos) == wrap) {
			if (fputc('\n', fout) == -1)
				return errno;
			*pos = 0;
		}
	}

	return 0;
}

static void
encode(uint8_t out[4], uint8_t in[3])
{
	out[0] = in[0] >> 2;
	out[1] = (uint8_t)(((in[0] & 0x03) << 4) | (in[1] >> 4));
	out[2] = (uint8_t)(((in[1] & 0x0f) << 2) | (in[2] >> 6));
	out[3] = in[2] & 0x3f;
}

static int
b64_encode(FILE *fout, FILE *fin, size_t wrap)
{
	uint8_t in[3];
	uint8_t out[4];
	size_t ilen;
	size_t pos = 0;
	int e;

	while ((ilen = getinput(fin, in)) > 2) {
		encode(out, in);
		if ((e = putoutput(fout, out, ilen, wrap, &pos)) != 0)
			return e;
	}

	if (ilen != 0) {
		encode(out, in);
		if ((e = putoutput(fout, out, ilen, wrap, &pos)) != 0)
			return e;
	}

	if (pos && wrap) {
		if (fputc('\n', fout) == -1)
			return errno;
	}
	return 0;
}
		

static int
b64_decode(FILE *fout, FILE *fin, bool ignore)
{
	int state, c;
	uint8_t b, out;
	char *pos;

	state = 0;
	out = 0;

	while ((c = getc(fin)) != -1) {
		if (ignore && isspace(c))
			continue;

		if (c == '=')
			break;

		pos = strchr(B64, c);
		if (pos == NULL)
			return EFTYPE;

		b = (uint8_t)(pos - B64);

		switch (state) {
		case 0:
			out = (uint8_t)(b << 2); 
			break;
		case 1:
			out |= b >> 4;
			if (fputc(out, fout) == -1)
				return errno;
			out = (uint8_t)((b & 0xf) << 4);
			break;
		case 2:
			out |= b >> 2;
			if (fputc(out, fout) == -1)
				return errno;
			out = (uint8_t)((b & 0x3) << 6);
			break;
		case 3:
			out |= b;
			if (fputc(out, fout) == -1)
				return errno;
			out = 0;
			break;
		default:
			abort();
		}
		state = (state + 1) & 3;
	}

	if (c == '=') {
		switch (state) {
		case 0:
		case 1:
			return EFTYPE;
		case 2:
			while ((c = getc(fin)) != -1) {
				if (ignore && isspace(c))
					continue;
				break;
			}
			if (c != '=')
				return EFTYPE;
			/*FALLTHROUGH*/
		case 3:
			while ((c = getc(fin)) != -1) {
				if (ignore && isspace(c))
					continue;
				break;
			}
			if (c != -1)
				return EFTYPE;
			return 0;
		default:
			abort();
		}
	}

	if (c != -1 || state != 0)
		return EFTYPE;

	return 0;
}

static __dead void 
usage(void)
{
	fprintf(stderr, "Usage: %s [-di] [-w <wrap>] [<file>]...\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

static void
doit(FILE *fout, FILE *fin, bool decode, bool ignore, size_t wrap)
{
	int e;

	if (decode)
		e = b64_decode(stdout, stdin, ignore);
	else
		e = b64_encode(stdout, stdin, wrap);

	if (e == 0)
		return;
	errc(EXIT_FAILURE, e, "%scoding failed", decode ? "De": "En");
}

int
main(int argc, char *argv[])
{
	bool decode = false;
	size_t wrap = 76;
	bool ignore = false;
	int c;

	while ((c = getopt(argc, argv, "b:Ddiw:")) != -1) {
		switch (c) {
		case 'D':
			decode = ignore = true;
			break;
		case 'd':
			decode = true;
			break;
		case 'i':
			ignore = true;
			break;
		case 'b':
		case 'w':
			wrap = (size_t)atoi(optarg);
			break;
		default:
			usage();
		}
	}

	if (optind == argc) {
		doit(stdout, stdin, decode, ignore, wrap);
		return EXIT_SUCCESS;
	}

	for (c = optind; c < argc; c++) {
		FILE *fp = strcmp(argv[c], "-") == 0 ?
		    stdin : fopen(argv[c], "r");
		if (fp == NULL)
			err(EXIT_FAILURE, "Can't open `%s'", argv[c]);
		doit(stdout, fp, decode, ignore, wrap);
		if (fp != stdin)
			fclose(fp);
		fclose(fp);
	}

	return EXIT_SUCCESS;
}
