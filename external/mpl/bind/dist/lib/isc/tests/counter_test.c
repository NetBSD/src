/*	$NetBSD: counter_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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
#include <stdlib.h>

#include <atf-c.h>

#include <isc/counter.h>
#include <isc/result.h>

#include "isctest.h"

ATF_TC(isc_counter);
ATF_TC_HEAD(isc_counter, tc) {
	atf_tc_set_md_var(tc, "descr", "isc counter object");
}
ATF_TC_BODY(isc_counter, tc) {
	isc_result_t result;
	isc_counter_t *counter = NULL;
	int i;

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_counter_create(mctx, 0, &counter);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	for (i = 0; i < 10; i++) {
		result = isc_counter_increment(counter);
		ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	}

	ATF_CHECK_EQ(isc_counter_used(counter), 10);

	isc_counter_setlimit(counter, 15);
	for (i = 0; i < 10; i++) {
		result = isc_counter_increment(counter);
		if (result != ISC_R_SUCCESS)
			break;
	}

	ATF_CHECK_EQ(isc_counter_used(counter), 15);

	isc_counter_detach(&counter);
	isc_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_counter);
	return (atf_no_error());
}

