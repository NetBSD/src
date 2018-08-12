/*	$NetBSD: parse_test.c,v 1.2 2018/08/12 13:02:39 christos Exp $	*/

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

/*! \file */

#include <config.h>

#include <atf-c.h>

#include <unistd.h>
#include <time.h>

#include <isc/parseint.h>

#include "isctest.h"

/*
 * Individual unit tests
 */

/* Test for 32 bit overflow on 64 bit machines in isc_parse_uint32 */
ATF_TC(parse_overflow);
ATF_TC_HEAD(parse_overflow, tc) {
	atf_tc_set_md_var(tc, "descr", "Check for 32 bit overflow");
}
ATF_TC_BODY(parse_overflow, tc) {
	isc_result_t result;
	isc_uint32_t output;
	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_parse_uint32(&output, "1234567890", 10);
	ATF_CHECK_EQ(ISC_R_SUCCESS, result);
	ATF_CHECK_EQ(1234567890, output);

	result = isc_parse_uint32(&output, "123456789012345", 10);
	ATF_CHECK_EQ(ISC_R_RANGE, result);

	result = isc_parse_uint32(&output, "12345678901234567890", 10);
	ATF_CHECK_EQ(ISC_R_RANGE, result);

	isc_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, parse_overflow);

	return (atf_no_error());
}

