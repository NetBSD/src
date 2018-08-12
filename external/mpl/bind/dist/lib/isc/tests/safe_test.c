/*	$NetBSD: safe_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */


/* ! \file */

#include <config.h>

#include <atf-c.h>

#include <stdio.h>
#include <string.h>

#include <isc/safe.h>
#include <isc/util.h>

ATF_TC(isc_safe_memequal);
ATF_TC_HEAD(isc_safe_memequal, tc) {
	atf_tc_set_md_var(tc, "descr", "safe memequal()");
}
ATF_TC_BODY(isc_safe_memequal, tc) {
	UNUSED(tc);

	ATF_CHECK(isc_safe_memequal("test", "test", 4));
	ATF_CHECK(!isc_safe_memequal("test", "tesc", 4));
	ATF_CHECK(isc_safe_memequal("\x00\x00\x00\x00",
				    "\x00\x00\x00\x00", 4));
	ATF_CHECK(!isc_safe_memequal("\x00\x00\x00\x00",
				     "\x00\x00\x00\x01", 4));
	ATF_CHECK(!isc_safe_memequal("\x00\x00\x00\x02",
				     "\x00\x00\x00\x00", 4));
}

ATF_TC(isc_safe_memcompare);
ATF_TC_HEAD(isc_safe_memcompare, tc) {
	atf_tc_set_md_var(tc, "descr", "safe memcompare()");
}
ATF_TC_BODY(isc_safe_memcompare, tc) {
	UNUSED(tc);

	ATF_CHECK(isc_safe_memcompare("test", "test", 4) == 0);
	ATF_CHECK(isc_safe_memcompare("test", "tesc", 4) > 0);
	ATF_CHECK(isc_safe_memcompare("test", "tesy", 4) < 0);
	ATF_CHECK(isc_safe_memcompare("\x00\x00\x00\x00",
				      "\x00\x00\x00\x00", 4) == 0);
	ATF_CHECK(isc_safe_memcompare("\x00\x00\x00\x00",
				      "\x00\x00\x00\x01", 4) < 0);
	ATF_CHECK(isc_safe_memcompare("\x00\x00\x00\x02",
				      "\x00\x00\x00\x00", 4) > 0);
}

ATF_TC(isc_safe_memwipe);
ATF_TC_HEAD(isc_safe_memwipe, tc) {
	atf_tc_set_md_var(tc, "descr", "isc_safe_memwipe()");
}
ATF_TC_BODY(isc_safe_memwipe, tc) {
	UNUSED(tc);

	/* These should pass. */
	isc_safe_memwipe(NULL, 0);
	isc_safe_memwipe((void *) -1, 0);
	isc_safe_memwipe(NULL, 42);

	/*
	 * isc_safe_memwipe(ptr, size) should function same as
	 * memset(ptr, 0, size);
	 */
	{
		char buf1[4] = { 1, 2, 3, 4 };
		char buf2[4] = { 1, 2, 3, 4 };

		isc_safe_memwipe(buf1, sizeof(buf1));
		memset(buf2, 0, sizeof(buf2));

		ATF_CHECK(memcmp(buf1, buf2, sizeof(buf1)) == 0);
	}

	/*
	 * Boundary test.
	 */
	{
		char buf1[4] = { 1, 2, 3, 4 };
		char buf2[4] = { 1, 2, 3, 4 };

		/*
		 * We wipe 3 elements on purpose, keeping the 4th in
		 * place.
		 */
		isc_safe_memwipe(buf1, 3);
		memset(buf2, 0, 3);

		ATF_CHECK(memcmp(buf1, buf2, sizeof(buf1)) == 0);
	}
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_safe_memequal);
	ATF_TP_ADD_TC(tp, isc_safe_memcompare);
	ATF_TP_ADD_TC(tp, isc_safe_memwipe);
	return (atf_no_error());
}
