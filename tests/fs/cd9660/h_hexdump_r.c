/*	$NetBSD: h_hexdump_r.c,v 1.1 2024/04/28 14:39:22 rillig Exp $	*/

/*
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Roland Illig.
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

/* Given the output from "hexdump -C", reconstruct the original file. */

#include <err.h>
#include <inttypes.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	H	"[0-9a-f]"
#define	HH	" (" H H ")"

static off_t off, noff;
static unsigned char prev_bytes[16], bytes[16], zeroes[16];

int
main(void)
{
	char line[81];
	regex_t data_re, end_re;
	regmatch_t m[18];

	if (regcomp(&data_re, "^(" H "{8,9})"
	    " " HH HH HH HH HH HH HH HH " " HH HH HH HH HH HH HH HH
	    "  \\|.{16}\\|$", REG_EXTENDED) != 0)
		err(1, "regcomp");
	if (regcomp(&end_re, "^(" H "{8,9})$", REG_EXTENDED) != 0)
		err(1, "regcomp");

	while (fgets(line, sizeof(line), stdin) != NULL) {
		line[strcspn(line, "\n")] = '\0';

		if (strcmp(line, "*") == 0)
			continue;

		if (regexec(&data_re, line, 18, m, 0) == 0) {
			noff = (off_t)strtoimax(line + m[1].rm_so, NULL, 16);
			for (size_t i = 0; i < 16; i++)
				bytes[i] = (unsigned char)strtoumax(
				    line + m[2 + i].rm_so, NULL, 16);

		} else if (regexec(&end_re, line, 2, m, 0) == 0) {
			noff = (off_t)strtoimax(line + m[1].rm_so, NULL, 16);
			if (off < noff) {
				if (fseeko(stdout, noff - 16, SEEK_SET) != 0)
					err(1, "fseeko");
				if (fwrite(prev_bytes, 1, 16, stdout) != 16)
					err(1, "fwrite");
			}
		} else
			err(1, "invalid line '%s'", line);

		if (memcmp(prev_bytes, zeroes, 16) != 0) {
			while (off < noff) {
				if (fwrite(prev_bytes, 1, 16, stdout) != 16)
					err(1, "fwrite");
				off += 16;
			}
			if (off != noff)
				err(1, "off");
		} else {
			if (fseeko(stdout, noff, SEEK_SET) != 0)
				err(1, "fseeko");
			off = noff;
		}

		memcpy(prev_bytes, bytes, 16);
	}
	return 0;
}
