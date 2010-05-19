/*	$NetBSD: main.c,v 1.1 2010/05/19 21:55:36 christos Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: main.c,v 1.1 2010/05/19 21:55:36 christos Exp $");

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <err.h>
#include <string.h>
#include <errno.h>

#define ETHER_ADDR_LEN 6

int ether_aton_r(u_char *dest, size_t len, const char *str);

int
main(int argc, char *argv[])
{
	static const struct {
		u_char res[ETHER_ADDR_LEN];
		const char *str;
		int error;
	} tests[] = {
#define ZERO { 0, 0, 0, 0, 0, 0 }
	    { { 0, 1, 0x22, 3, 0x14, 5 }, "0:1:22-3:14:05", 0 },
	    { { 0, 1, 0x22, 3, 0x14, 5 }, "000122031405", 0 },
	    { ZERO,			  "0:1:2-3:04:05:06", ENAMETOOLONG },
	    { ZERO,			  "0:1:2-3:04:", ENOBUFS },
	    { ZERO,			  "0:1:2-3:04:x7", EINVAL },
	    { ZERO,			  "1:x-3:04:05:7", EINVAL },
	    { ZERO,			  NULL, 0 },
	};
	u_char dest[ETHER_ADDR_LEN];
	size_t t;
	int e, r;
	const char *s;
	int error = 0;

	for (t = 0; tests[t].str; t++) {
		s = tests[t].str;
		if ((e = tests[t].error) == 0) {
			if (ether_aton_r(dest, sizeof(dest), s) != e) {
				error++;
				warnx("ether_aton_r failed on `%s'", s);
			}
			if (memcmp(dest, tests[t].res, sizeof(dest)) != 0) {
				error++;
				warnx("ether_aton_r unexpected result on `%s'",
				    s);
			}
		} else {
			if ((r = ether_aton_r(dest, sizeof(dest), s)) != e) {
				error++;
				warnx("ether_aton_r succeeded on `%s' "
				    "(%d != %d)", s, r, e);
			}
		}
	}
	return error;
}
