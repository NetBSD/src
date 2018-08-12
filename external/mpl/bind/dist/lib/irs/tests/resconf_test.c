/*	$NetBSD: resconf_test.c,v 1.1.1.1 2018/08/12 12:08:29 christos Exp $	*/

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

#include <isc/mem.h>
#include <isc/util.h>

#include <irs/types.h>
#include <irs/resconf.h>

static isc_mem_t *mctx = NULL;

static void
setup_test() {
	isc_result_t result;

	result = isc_mem_create(0, 0, &mctx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * atf-run changes us to a /tmp directory, so tests
	 * that access test data files must first chdir to the proper
	 * location.
	 */
	ATF_REQUIRE(chdir(TESTS) != -1);
}

ATF_TC(irs_resconf_load);
ATF_TC_HEAD(irs_resconf_load, tc) {
	atf_tc_set_md_var(tc, "descr", "irs_resconf_load");
}
ATF_TC_BODY(irs_resconf_load, tc) {
	isc_result_t result;
	irs_resconf_t *resconf = NULL;
	unsigned int i;
	struct {
		const char *file;
		isc_result_t loadres;
		isc_result_t (*check)(irs_resconf_t *resconf);
		isc_result_t checkres;
	} tests[] = {
		{
			"testdata/domain.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/nameserver-v4.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/nameserver-v6.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options-debug.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options-ndots.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options-timeout.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options-unknown.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options-bad-ndots.conf", ISC_R_RANGE,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options-empty.conf", ISC_R_UNEXPECTEDEND,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/port.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/resolv.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/search.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/sortlist-v4.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/timeout.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/unknown.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}

	};

	UNUSED(tc);

	setup_test();

	for (i = 0; i < sizeof(tests)/sizeof(tests[1]); i++) {
		result = irs_resconf_load(mctx, tests[i].file, &resconf);
		ATF_CHECK_EQ_MSG(result, tests[i].loadres, "%s", tests[i].file);
		if (result == ISC_R_SUCCESS)
			ATF_CHECK_MSG(resconf != NULL, "%s", tests[i].file);
		else
			ATF_CHECK_MSG(resconf == NULL, "%s", tests[i].file);
		if (resconf != NULL && tests[i].check != NULL) {
			result = (tests[i].check)(resconf);
			ATF_CHECK_EQ_MSG(result, tests[i].checkres, "%s",
					 tests[i].file);
		}
		if (resconf != NULL)
			irs_resconf_destroy(&resconf);
	}

	isc_mem_detach(&mctx);
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, irs_resconf_load);
	return (atf_no_error());
}
