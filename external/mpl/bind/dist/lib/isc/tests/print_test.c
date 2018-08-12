/*	$NetBSD: print_test.c,v 1.2 2018/08/12 13:02:39 christos Exp $	*/

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

#include <config.h>

#include <atf-c.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Workout if we need to force the inclusion of print.c so we can test
 * it on all platforms even if we don't include it in libisc.
 */
#include <isc/platform.h>
#if !defined(ISC_PLATFORM_NEEDPRINTF) && \
    !defined(ISC_PLATFORM_NEEDFPRINTF) && \
    !defined(ISC_PLATFORM_NEEDSPRINTF) && \
    !defined(ISC_PLATFORM_NEEDVSNPRINTF)
#define ISC__PRINT_SOURCE
#include "../print.c"
#else
#if !defined(ISC_PLATFORM_NEEDPRINTF) || \
    !defined(ISC_PLATFORM_NEEDFPRINTF) || \
    !defined(ISC_PLATFORM_NEEDSPRINTF) || \
    !defined(ISC_PLATFORM_NEEDVSNPRINTF)
#define ISC__PRINT_SOURCE
#endif
#include <isc/print.h>
#include <isc/types.h>
#include <isc/util.h>
#endif

ATF_TC(snprintf);
ATF_TC_HEAD(snprintf, tc) {
	atf_tc_set_md_var(tc, "descr", "snprintf implementation");
}
ATF_TC_BODY(snprintf, tc) {
	char buf[10000];
	isc_uint64_t ll = 8589934592ULL;
	isc_uint64_t nn = 20000000000000ULL;
	isc_uint64_t zz = 10000000000000000000ULL;
	float pi = 3.141;
	int n;
	size_t size;

	UNUSED(tc);

	/*
	 * 4294967296 <= 8589934592 < 1000000000^2 to verify fix for
	 * RT#36505.
	 */

	memset(buf, 0xff, sizeof(buf));
	n = isc_print_snprintf(buf, sizeof(buf), "%qu", ll);
	ATF_CHECK_EQ(n, 10);
	ATF_CHECK_STREQ(buf, "8589934592");

	memset(buf, 0xff, sizeof(buf));
	n = isc_print_snprintf(buf, sizeof(buf), "%llu", ll);
	ATF_CHECK_EQ(n, 10);
	ATF_CHECK_STREQ(buf, "8589934592");

	memset(buf, 0xff, sizeof(buf));
	n = isc_print_snprintf(buf, sizeof(buf), "%qu", nn);
	ATF_CHECK_EQ(n, 14);
	ATF_CHECK_STREQ(buf, "20000000000000");

	memset(buf, 0xff, sizeof(buf));
	n = isc_print_snprintf(buf, sizeof(buf), "%llu", nn);
	ATF_CHECK_EQ(n, 14);
	ATF_CHECK_STREQ(buf, "20000000000000");

	memset(buf, 0xff, sizeof(buf));
	n = isc_print_snprintf(buf, sizeof(buf), "%qu", zz);
	ATF_CHECK_EQ(n, 20);
	ATF_CHECK_STREQ(buf, "10000000000000000000");

	memset(buf, 0xff, sizeof(buf));
	n = isc_print_snprintf(buf, sizeof(buf), "%llu", zz);
	ATF_CHECK_EQ(n, 20);
	ATF_CHECK_STREQ(buf, "10000000000000000000");

	memset(buf, 0xff, sizeof(buf));
	n = isc_print_snprintf(buf, sizeof(buf), "%lld", nn);
	ATF_CHECK_EQ(n, 14);
	ATF_CHECK_STREQ(buf, "20000000000000");

	size = 1000;
	memset(buf, 0xff, sizeof(buf));
	n = isc_print_snprintf(buf, sizeof(buf), "%zu", size);
	ATF_CHECK_EQ(n, 4);
	ATF_CHECK_STREQ(buf, "1000");

	size = 1000;
	memset(buf, 0xff, sizeof(buf));
	n = isc_print_snprintf(buf, sizeof(buf), "%zx", size);
	ATF_CHECK_EQ(n, 3);
	ATF_CHECK_STREQ(buf, "3e8");

	size = 1000;
	memset(buf, 0xff, sizeof(buf));
	n = isc_print_snprintf(buf, sizeof(buf), "%zo", size);
	ATF_CHECK_EQ(n, 4);
	ATF_CHECK_STREQ(buf, "1750");

	zz = 0xf5f5f5f5f5f5f5f5ULL;
	memset(buf, 0xff, sizeof(buf));
	n = isc_print_snprintf(buf, sizeof(buf), "0x%"ISC_PRINT_QUADFORMAT"x", zz);
	ATF_CHECK_EQ(n, 18);
	ATF_CHECK_STREQ(buf, "0xf5f5f5f5f5f5f5f5");

	n = isc_print_snprintf(buf, sizeof(buf), "%.2f", pi);
	ATF_CHECK_EQ(n, 4);
	ATF_CHECK_STREQ(buf, "3.14");

	/* Similar to the above, but additional characters follows */
	n = isc_print_snprintf(buf, sizeof(buf), "%.2f1592", pi);
	ATF_CHECK_EQ(n, 8);
	ATF_CHECK_STREQ(buf, "3.141592");

	/* Similar to the above, but with leading spaces */
	n = isc_print_snprintf(buf, sizeof(buf), "% 8.2f1592", pi);
	ATF_CHECK_EQ(n, 12);
	ATF_CHECK_STREQ(buf, "    3.141592");

	/* Similar to the above, but with trail spaces after the 4 */
	n = isc_print_snprintf(buf, sizeof(buf), "%-8.2f1592", pi);
	ATF_CHECK_EQ(n, 12);
	ATF_CHECK_STREQ(buf, "3.14    1592");
}

ATF_TC(fprintf);
ATF_TC_HEAD(fprintf, tc) {
	atf_tc_set_md_var(tc, "descr", "fprintf implementation");
}
ATF_TC_BODY(fprintf, tc) {
	FILE *f;
	int n;
	size_t size;
	char buf[10000];

	UNUSED(tc);

	f = fopen("fprintf.test", "w+");
	ATF_REQUIRE(f != NULL);

	size = 1000;
	n = isc_print_fprintf(f, "%zu", size);
	ATF_CHECK_EQ(n, 4);

	rewind(f);

	memset(buf, 0, sizeof(buf));
	n = fread(buf, 1, sizeof(buf), f);
	ATF_CHECK_EQ(n, 4);

	fclose(f);

	ATF_CHECK_STREQ(buf, "1000");

	if ((n > 0) && (!strcmp(buf, "1000")))
		unlink("fprintf.test");
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, snprintf);
	ATF_TP_ADD_TC(tp, fprintf);
	return (atf_no_error());
}
