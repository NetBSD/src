/*	$NetBSD: result_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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

#include <string.h>

#include <isc/result.h>

ATF_TC(isc_result_toid);
ATF_TC_HEAD(isc_result_toid, tc) {
	atf_tc_set_md_var(tc, "descr", "convert result to identifier string");
}
ATF_TC_BODY(isc_result_toid, tc) {
	const char *id;

	id = isc_result_toid(ISC_R_SUCCESS);
	ATF_REQUIRE_STREQ("ISC_R_SUCCESS", id);

	id = isc_result_toid(ISC_R_FAILURE);
	ATF_REQUIRE_STREQ("ISC_R_FAILURE", id);
}

ATF_TC(isc_result_totext);
ATF_TC_HEAD(isc_result_totext, tc) {
	atf_tc_set_md_var(tc, "descr", "convert result to description string");
}
ATF_TC_BODY(isc_result_totext, tc) {
	const char *str;

	str = isc_result_totext(ISC_R_SUCCESS);
	ATF_REQUIRE_STREQ("success", str);

	str = isc_result_totext(ISC_R_FAILURE);
	ATF_REQUIRE_STREQ("failure", str);
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_result_toid);
	ATF_TP_ADD_TC(tp, isc_result_totext);

	return (atf_no_error());
}
