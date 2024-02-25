/*	$NetBSD: nsec3_test.c,v 1.2.2.2 2024/02/25 15:47:40 martin Exp $	*/

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

#include <isc/string.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/nsec3.h>

#include <tests/dns.h>

static void
iteration_test(const char *file, unsigned int expected) {
	isc_result_t result;
	dns_db_t *db = NULL;
	unsigned int iterations;

	result = dns_test_loaddb(&db, dns_dbtype_zone, "test", file);
	assert_int_equal(result, ISC_R_SUCCESS);

	iterations = dns_nsec3_maxiterations();

	assert_int_equal(iterations, expected);

	dns_db_detach(&db);
}

/*%
 * Structure containing parameters for nsec3param_salttotext_test().
 */
typedef struct {
	const char *nsec3param_text; /* NSEC3PARAM RDATA in text form */
	const char *expected_salt;   /* string expected in target buffer */
} nsec3param_salttotext_test_params_t;

/*%
 * Check whether dns_nsec3param_salttotext() handles supplied text form
 * NSEC3PARAM RDATA correctly: test whether the result of calling the former is
 * as expected and whether it properly checks available buffer space.
 *
 * Assumes supplied text form NSEC3PARAM RDATA is valid as testing handling of
 * invalid NSEC3PARAM RDATA is out of scope of this unit test.
 */
static void
nsec3param_salttotext_test(const nsec3param_salttotext_test_params_t *params) {
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_nsec3param_t nsec3param;
	unsigned char buf[1024];
	isc_result_t result;
	char salt[64];
	size_t length;

	/*
	 * Prepare a dns_rdata_nsec3param_t structure for testing.
	 */
	result = dns_test_rdatafromstring(
		&rdata, dns_rdataclass_in, dns_rdatatype_nsec3param, buf,
		sizeof(buf), params->nsec3param_text, false);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dns_rdata_tostruct(&rdata, &nsec3param, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Check typical use.
	 */
	result = dns_nsec3param_salttotext(&nsec3param, salt, sizeof(salt));
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_string_equal(salt, params->expected_salt);

	/*
	 * Ensure available space in the buffer is checked before the salt is
	 * printed to it and that the amount of space checked for includes the
	 * terminating NULL byte.
	 */
	length = strlen(params->expected_salt);
	assert_true(length < sizeof(salt) - 1); /* prevent buffer overwrite */
	assert_true(length > 0U);		/* prevent length underflow */

	result = dns_nsec3param_salttotext(&nsec3param, salt, length - 1);
	assert_int_equal(result, ISC_R_NOSPACE);

	result = dns_nsec3param_salttotext(&nsec3param, salt, length);
	assert_int_equal(result, ISC_R_NOSPACE);

	result = dns_nsec3param_salttotext(&nsec3param, salt, length + 1);
	assert_int_equal(result, ISC_R_SUCCESS);
}

/*
 * check that appropriate max iterations is returned for different
 * key size mixes
 */
ISC_RUN_TEST_IMPL(max_iterations) {
	UNUSED(state);

	iteration_test(TESTS_DIR "/testdata/nsec3/1024.db", 150);
	iteration_test(TESTS_DIR "/testdata/nsec3/2048.db", 150);
	iteration_test(TESTS_DIR "/testdata/nsec3/4096.db", 150);
	iteration_test(TESTS_DIR "/testdata/nsec3/min-1024.db", 150);
	iteration_test(TESTS_DIR "/testdata/nsec3/min-2048.db", 150);
}

/* check dns_nsec3param_salttotext() */
ISC_RUN_TEST_IMPL(nsec3param_salttotext) {
	size_t i;

	const nsec3param_salttotext_test_params_t tests[] = {
		/*
		 * Tests with non-empty salts.
		 */
		{ "0 0 10 0123456789abcdef", "0123456789ABCDEF" },
		{ "0 1 11 0123456789abcdef", "0123456789ABCDEF" },
		{ "1 0 12 42", "42" },
		{ "1 1 13 42", "42" },
		/*
		 * Test with empty salt.
		 */
		{ "0 0 0 -", "-" },
	};

	UNUSED(state);

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		nsec3param_salttotext_test(&tests[i]);
	}
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(max_iterations)
ISC_TEST_ENTRY(nsec3param_salttotext)
ISC_TEST_LIST_END

ISC_TEST_MAIN
