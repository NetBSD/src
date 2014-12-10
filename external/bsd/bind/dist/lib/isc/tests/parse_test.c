/*	$NetBSD: parse_test.c,v 1.1.1.3 2014/12/10 03:34:44 christos Exp $	*/

/*
 * Copyright (C) 2012, 2013  Internet Systems Consortium, Inc. ("ISC")
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

	result = isc_test_begin(NULL, ISC_TRUE);
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

