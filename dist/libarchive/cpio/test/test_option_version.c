/*-
 * Copyright (c) 2003-2007 Tim Kientzle
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "test.h"
__FBSDID("$FreeBSD$");

/*
 * Test that --version option works and generates reasonable output.
 */

static void
verify(const char *q, size_t s)
{
	/* Version message should start with name of program, then space. */
	failure("version message too short");
	if (!assert(s > 6))
		return;
	failure("Version message should begin with 'bsdcpio'");
	if (!assertEqualMem(q, "bsdcpio ", 8))
		/* If we're not testing bsdcpio, don't keep going. */
		return;
	q += 8; s -= 8;
	/* Version number is a series of digits and periods. */
	while (s > 0 && (*q == '.' || (*q >= '0' && *q <= '9'))) {
		++q;
		--s;
	}
	/* Version number terminated by space. */
	assert(s > 1);
	assert(*q == ' ');
	++q; --s;
	/* Separator. */
	assertEqualMem(q, "-- ", 3);
	q += 3; s -= 3;
	/* libarchive name and version number */
	assert(s > 11);
	assertEqualMem(q, "libarchive ", 11);
	q += 11; s -= 11;
	/* Version number is a series of digits and periods. */
	while (s > 0 && (*q == '.' || (*q >= '0' && *q <= '9'))) {
		++q;
		--s;
	}
	/* All terminated by a newline. */
	assert(s >= 1);
	assertEqualMem(q, "\n", 1);
}


DEFINE_TEST(test_option_version)
{
	int r;
	char *p;
	size_t s;

	r = systemf("%s --version >version.stdout 2>version.stderr", testprog);
	/* --version should generate nothing to stderr. */
	assertEmptyFile("version.stderr");
	/* Verify format of version message. */
	p = slurpfile(&s, "version.stdout");
	verify(p, s);
	free(p);
}
