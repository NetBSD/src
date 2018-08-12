/*	$NetBSD: file_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atf-c.h>

#include <isc/file.h>
#include <isc/result.h>

ATF_TC(isc_file_sanitize);
ATF_TC_HEAD(isc_file_sanitize, tc) {
	atf_tc_set_md_var(tc, "descr", "sanitized filenames");
}

#define NAME "internal"
#define SHA "3bed2cb3a3acf7b6a8ef408420cc682d5520e26976d354254f528c965612054f"
#define TRUNC_SHA "3bed2cb3a3acf7b6"

#define BAD1 "in/internal"
#define BADHASH1 "8bbb97a888791399"

#define BAD2 "Internal"
#define BADHASH2 "2ea1842b445b0c81"

#define F(x) "testdata/file/" x ".test"

static void
touch(const char *filename) {
	int fd;

	unlink(filename);
	fd = creat(filename, 0644);
	if (fd != -1)
		close(fd);
}

ATF_TC_BODY(isc_file_sanitize, tc) {
	isc_result_t result;
	char buf[1024];

	ATF_CHECK(chdir(TESTS) != -1);

	result = isc_file_sanitize("testdata/file", NAME, "test", buf, 1024);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(strcmp(buf, F(NAME)) == 0);

	touch(F(TRUNC_SHA));
	result = isc_file_sanitize("testdata/file", NAME, "test", buf, 1024);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(strcmp(buf, F(TRUNC_SHA)) == 0);

	touch(F(SHA));
	result = isc_file_sanitize("testdata/file", NAME, "test", buf, 1024);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(strcmp(buf, F(SHA)) == 0);

	result = isc_file_sanitize("testdata/file", BAD1, "test", buf, 1024);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(strcmp(buf, F(BADHASH1)) == 0);

	result = isc_file_sanitize("testdata/file", BAD2, "test", buf, 1024);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(strcmp(buf, F(BADHASH2)) == 0);

	unlink(F(TRUNC_SHA));
	unlink(F(SHA));
}

ATF_TC(isc_file_template);
ATF_TC_HEAD(isc_file_template, tc) {
	atf_tc_set_md_var(tc, "descr", "file template");
}

ATF_TC_BODY(isc_file_template, tc) {
	isc_result_t result;
	char buf[1024];

	ATF_CHECK(chdir(TESTS) != -1);

	result = isc_file_template("/absolute/path", "file-XXXXXXXX",
				   buf, sizeof(buf));
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK_STREQ(buf, "/absolute/file-XXXXXXXX");

	result = isc_file_template("relative/path", "file-XXXXXXXX",
				   buf, sizeof(buf));
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK_STREQ(buf, "relative/file-XXXXXXXX");

	result = isc_file_template("/trailing/slash/", "file-XXXXXXXX",
				   buf, sizeof(buf));
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK_STREQ(buf, "/trailing/slash/file-XXXXXXXX");

	result = isc_file_template("relative/trailing/slash/", "file-XXXXXXXX",
				   buf, sizeof(buf));
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK_STREQ(buf, "relative/trailing/slash/file-XXXXXXXX");

	result = isc_file_template("/", "file-XXXXXXXX", buf, sizeof(buf));
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK_STREQ(buf, "/file-XXXXXXXX");

	result = isc_file_template("noslash", "file-XXXXXXXX",
				   buf, sizeof(buf));
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK_STREQ(buf, "file-XXXXXXXX");

	result = isc_file_template(NULL, "file-XXXXXXXX", buf, sizeof(buf));
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK_STREQ(buf, "file-XXXXXXXX");
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_file_sanitize);
	ATF_TP_ADD_TC(tp, isc_file_template);
	return (atf_no_error());
}

