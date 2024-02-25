/*	$NetBSD: file_test.c,v 1.2.2.2 2024/02/25 15:47:51 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <fcntl.h>
#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/file.h>
#include <isc/result.h>
#include <isc/util.h>

#include <tests/isc.h>

#define NAME	  "internal"
#define SHA	  "3bed2cb3a3acf7b6a8ef408420cc682d5520e26976d354254f528c965612054f"
#define TRUNC_SHA "3bed2cb3a3acf7b6"

#define BAD1	 "in/internal"
#define BADHASH1 "8bbb97a888791399"

#define BAD2	 "Internal"
#define BADHASH2 "2ea1842b445b0c81"

#define F(x) "testdata/file/" x ".test"

static void
touch(const char *filename) {
	int fd;

	unlink(filename);
	fd = creat(filename, 0644);
	if (fd != -1) {
		close(fd);
	}
}

/* test sanitized filenames */
ISC_RUN_TEST_IMPL(isc_file_sanitize) {
	isc_result_t result;
	char buf[1024];

	UNUSED(state);

	assert_return_code(chdir(TESTS_DIR), 0);

	result = isc_file_sanitize("testdata/file", NAME, "test", buf, 1024);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(strcmp(buf, F(NAME)), 0);

	touch(F(TRUNC_SHA));
	result = isc_file_sanitize("testdata/file", NAME, "test", buf, 1024);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(strcmp(buf, F(TRUNC_SHA)), 0);

	touch(F(SHA));
	result = isc_file_sanitize("testdata/file", NAME, "test", buf, 1024);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(strcmp(buf, F(SHA)), 0);

	result = isc_file_sanitize("testdata/file", BAD1, "test", buf, 1024);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(strcmp(buf, F(BADHASH1)), 0);

	result = isc_file_sanitize("testdata/file", BAD2, "test", buf, 1024);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(strcmp(buf, F(BADHASH2)), 0);

	unlink(F(TRUNC_SHA));
	unlink(F(SHA));
}

/* test filename templates */
ISC_RUN_TEST_IMPL(isc_file_template) {
	isc_result_t result;
	char buf[1024];

	UNUSED(state);

	assert_return_code(chdir(TESTS_DIR), 0);

	result = isc_file_template("/absolute/path", "file-XXXXXXXX", buf,
				   sizeof(buf));
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, "/absolute/file-XXXXXXXX");

	result = isc_file_template("relative/path", "file-XXXXXXXX", buf,
				   sizeof(buf));
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, "relative/file-XXXXXXXX");

	result = isc_file_template("/trailing/slash/", "file-XXXXXXXX", buf,
				   sizeof(buf));
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, "/trailing/slash/file-XXXXXXXX");

	result = isc_file_template("relative/trailing/slash/", "file-XXXXXXXX",
				   buf, sizeof(buf));
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, "relative/trailing/slash/file-XXXXXXXX");

	result = isc_file_template("/", "file-XXXXXXXX", buf, sizeof(buf));
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, "/file-XXXXXXXX");

	result = isc_file_template("noslash", "file-XXXXXXXX", buf,
				   sizeof(buf));
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, "file-XXXXXXXX");

	result = isc_file_template(NULL, "file-XXXXXXXX", buf, sizeof(buf));
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(buf, "file-XXXXXXXX");
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_file_sanitize)
ISC_TEST_ENTRY(isc_file_template)

ISC_TEST_LIST_END

ISC_TEST_MAIN
