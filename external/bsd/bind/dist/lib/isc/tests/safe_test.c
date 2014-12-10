/*	$NetBSD: safe_test.c,v 1.3 2014/12/10 04:38:01 christos Exp $	*/

/*
 * Copyright (C) 2013  Internet Systems Consortium, Inc. ("ISC")
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

/* Id */

/* ! \file */

#include <config.h>

#include <atf-c.h>

#include <stdio.h>
#include <string.h>

#include <isc/safe.h>
#include <isc/util.h>

ATF_TC(isc_safe_memcmp);
ATF_TC_HEAD(isc_safe_memcmp, tc) {
	atf_tc_set_md_var(tc, "descr", "safe memcmp()");
}
ATF_TC_BODY(isc_safe_memcmp, tc) {
	UNUSED(tc);

	ATF_CHECK(isc_safe_memcmp("test", "test", 4));
	ATF_CHECK(!isc_safe_memcmp("test", "tesc", 4));
	ATF_CHECK(isc_safe_memcmp("\x00\x00\x00\x00", "\x00\x00\x00\x00", 4));
	ATF_CHECK(!isc_safe_memcmp("\x00\x00\x00\x00", "\x00\x00\x00\x01", 4));
	ATF_CHECK(!isc_safe_memcmp("\x00\x00\x00\x02", "\x00\x00\x00\x00", 4));
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_safe_memcmp);
	return (atf_no_error());
}

