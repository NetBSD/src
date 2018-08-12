/*	$NetBSD: dstrandom_test.c,v 1.1.1.1 2018/08/12 12:08:20 christos Exp $	*/

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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <isc/entropy.h>
#include <isc/mem.h>
#include <isc/platform.h>
#include <isc/util.h>

#include <dst/dst.h>

isc_mem_t *mctx = NULL;
isc_entropy_t *ectx = NULL;
unsigned char buffer[128];

ATF_TC(isc_entropy_getdata);
ATF_TC_HEAD(isc_entropy_getdata, tc) {
	atf_tc_set_md_var(tc, "descr",
			  "isc_entropy_getdata() examples");
	atf_tc_set_md_var(tc, "X-randomfile",
			  "testdata/dstrandom/random.data");
}
ATF_TC_BODY(isc_entropy_getdata, tc) {
	isc_result_t result;
	unsigned int returned, status;
	int ret;
	const char *randomfile = atf_tc_get_md_var(tc, "X-randomfile");

	isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
	result = isc_mem_create(0, 0, &mctx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = isc_entropy_create(mctx, &ectx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = dst_lib_init(mctx, ectx, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

#ifdef ISC_PLATFORM_CRYPTORANDOM
	isc_entropy_usehook(ectx, ISC_TRUE);

	returned = 0;
	result = isc_entropy_getdata(ectx, buffer, sizeof(buffer),
				     &returned, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE(returned == sizeof(buffer));

	status = isc_entropy_status(ectx);
	ATF_REQUIRE_EQ(status, 0);

	isc_entropy_usehook(ectx, ISC_FALSE);
#endif

	ret = chdir(TESTS);
	ATF_REQUIRE_EQ(ret, 0);

	result = isc_entropy_createfilesource(ectx, randomfile);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	returned = 0;
	result = isc_entropy_getdata(ectx, buffer, sizeof(buffer),
				     &returned, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE(returned == sizeof(buffer));

	status = isc_entropy_status(ectx);
	ATF_REQUIRE(status > 0);

	dst_lib_destroy();
	isc_entropy_detach(&ectx);
	ATF_REQUIRE(ectx == NULL);
	isc_mem_destroy(&mctx);
	ATF_REQUIRE(mctx == NULL);
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_entropy_getdata);

	return (atf_no_error());
}

