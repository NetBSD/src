/*	$NetBSD: nsec3_test.c,v 1.3.4.1 2019/09/12 19:18:15 martin Exp $	*/

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

#if HAVE_CMOCKA

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <sched.h> /* IWYU pragma: keep */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/util.h>
#include <isc/string.h>

#include <dns/db.h>
#include <dns/nsec3.h>

#include "dnstest.h"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dns_test_begin(NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	dns_test_end();

	return (0);
}

static void
iteration_test(const char *file, unsigned int expected) {
	isc_result_t result;
	dns_db_t *db = NULL;
	unsigned int iterations;

	result = dns_test_loaddb(&db, dns_dbtype_zone, "test", file);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_nsec3_maxiterations(db, NULL, mctx, &iterations);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_int_equal(iterations, expected);

	dns_db_detach(&db);
}

/*%
 * Structure containing parameters for nsec3param_salttotext_test().
 */
typedef struct {
	const char *nsec3param_text;	/* NSEC3PARAM RDATA in text form */
	const char *expected_salt;	/* string expected in target buffer */
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
	result = dns_test_rdatafromstring(&rdata, dns_rdataclass_in,
					  dns_rdatatype_nsec3param, buf,
					  sizeof(buf),
					  params->nsec3param_text, false);
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
	assert_true(length < sizeof(salt) - 1);	/* prevent buffer overwrite */
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
static void
max_iterations(void **state) {
	UNUSED(state);

	iteration_test("testdata/nsec3/1024.db", 150);
	iteration_test("testdata/nsec3/2048.db", 500);
	iteration_test("testdata/nsec3/4096.db", 2500);
	iteration_test("testdata/nsec3/min-1024.db", 150);
	iteration_test("testdata/nsec3/min-2048.db", 500);
}

/* check dns_nsec3param_salttotext() */
static void
nsec3param_salttotext(void **state) {
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

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(max_iterations,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(nsec3param_salttotext,
						_setup, _teardown),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif
