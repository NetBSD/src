/*	$NetBSD: tfmtcheck.c,v 1.1 2002/03/23 17:42:21 christos Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Allen Briggs.
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

#include <stdio.h>
#include <stdlib.h>

const char *fmtcheck __P((const char *f1, const char *f2))
                __attribute__((__format_arg__(2)));

#include <err.h>

struct test_fmt {
	char	*fmt1;
	char	*fmt2;
	int	correct;
} test_fmts[] = {
	{ "%d", "%d", 1 },
	{ "%2d", "%2.2d", 1 },
	{ "%x", "%d", 1 },
	{ "%u", "%d", 1 },
	{ "%03d", "%d", 1 },
	{ "%-2d", "%d", 1 },
	{ "%d", "%-12.1d", 1 },
	{ "%d", "%-01.3d", 1 },
	{ "%X", "%-01.3d", 1 },
	{ "%D", "%ld", 1 },
	{ "%s", "%s", 1 },
	{ "%s", "This is a %s test", 1 },
	{ "Hi, there.  This is a %s test", "%s", 1 },
	{ "%d", "%s", 2 },
	{ "%e", "%s", 2 },
	{ "%r", "%d", 2 },
	{ "%*.2d", "%*d", 1 },
	{ "%2.*d", "%*d", 2 },
	{ "%*d", "%*d", 1 },
	{ "%-3", "%d", 2 },
	{ "%d %s", "%d", 2 },
	{ "%*.*.*d", "%*.*.*d", 2 },
	{ "%d", "%d %s", 1 },
	{ "%40s", "%20s", 1 },
	{ "%x %x %x", "%o %u %d", 1 },
	{ "%o %u %d", "%x %x %X", 1 },
	{ "%#o %u %#-d", "%x %#x %X", 1 },
	{ "%qd", "%llx", 1 },
	{ "%%", "%llx", 1 },
	{ "%p %30s %#llx %-10.*e", "This number %lu%% and string %s has %qd numbers and %.*g floats", 1 },
};

int
main(int argc, char *argv[])
{
	int		i, n, r;
	const char	*f, *cf, *f1, *f2;

	r = 0;
	n = sizeof(test_fmts) / sizeof(test_fmts[0]);
	for (i=0 ; i<n ; i++) {
		f1 = test_fmts[i].fmt1;
		f2 = test_fmts[i].fmt2;
		f = fmtcheck(f1, f2);
		if (test_fmts[i].correct == 1) {
			cf = f1;
		} else {
			cf = f2;
		}
		if (f != cf) {
			r++;
			errx(1, "Test %d: (%s) vs. (%s) failed "
			    "(should have returned %s)", i, f1, f2,
			    (test_fmts[i].correct == 1) ? "1st" : "2nd");
		}
	}
	exit(0);
}
