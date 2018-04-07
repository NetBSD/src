/*	$NetBSD: inet_ntop_test.c,v 1.1.1.1 2018/04/07 21:44:11 christos Exp $	*/

/*
 * Copyright (C) 2017  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <config.h>

#include <atf-c.h>

/*
 * Force the prototype for isc_net_ntop to be declared.
 */
#include <isc/platform.h>
#undef ISC_PLATFORM_NEEDNTOP
#define ISC_PLATFORM_NEEDNTOP
#include "../inet_ntop.c"

ATF_TC(isc_net_ntop);
ATF_TC_HEAD(isc_net_ntop, tc) {
	atf_tc_set_md_var(tc, "descr", "isc_net_ntop implementation");
}
ATF_TC_BODY(isc_net_ntop, tc) {
	char buf[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")];
	int r;
	size_t i;
	unsigned char abuf[16];
	struct {
		int		family;
		const char *	address;
	} testdata[] = {
		{ AF_INET, "0.0.0.0" },
		{ AF_INET, "0.1.0.0" },
		{ AF_INET, "0.0.2.0" },
		{ AF_INET, "0.0.0.3" },
		{ AF_INET, "255.255.255.255" },
		{ AF_INET6, "::" },
		{ AF_INET6, "::1.2.3.4" },
		{ AF_INET6, "::ffff:1.2.3.4" },
		{ AF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff" }
	};

	for (i = 0; i < sizeof(testdata)/sizeof(testdata[0]); i++) {
		r = inet_pton(testdata[i].family, testdata[i].address, abuf);
		ATF_REQUIRE_EQ_MSG(r, 1, "%s", testdata[i].address);
		isc_net_ntop(testdata[i].family, abuf, buf, sizeof(buf));
		ATF_CHECK_STREQ(buf, testdata[i].address);
	}
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_net_ntop);
	return (atf_no_error());
}
