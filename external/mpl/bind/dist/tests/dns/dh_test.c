/*	$NetBSD: dh_test.c,v 1.2.2.2 2024/02/25 15:47:39 martin Exp $	*/

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

/*
 * As a workaround, include an OpenSSL header file before including cmocka.h,
 * because OpenSSL 3.1.0 uses __attribute__(malloc), conflicting with a
 * redefined malloc in cmocka.h.
 */
#include <openssl/err.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/name.h>

#include "dst_internal.h"

#include <tests/dns.h>

static int
setup_test(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dst_lib_init(mctx, NULL);

	if (result != ISC_R_SUCCESS) {
		return (1);
	}

	return (0);
}

static int
teardown_test(void **state) {
	UNUSED(state);

	dst_lib_destroy();

	return (0);
}

/* OpenSSL DH_compute_key() failure */
ISC_RUN_TEST_IMPL(dh_computesecret) {
	dst_key_t *key = NULL;
	isc_buffer_t buf;
	unsigned char array[1024];
	isc_result_t result;
	dns_fixedname_t fname;
	dns_name_t *name;

	UNUSED(state);

	name = dns_fixedname_initname(&fname);
	isc_buffer_constinit(&buf, "dh.", 3);
	isc_buffer_add(&buf, 3);
	result = dns_name_fromtext(name, &buf, NULL, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dst_key_fromfile(name, 18602, DST_ALG_DH,
				  DST_TYPE_PUBLIC | DST_TYPE_KEY, TESTS_DIR,
				  mctx, &key);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_init(&buf, array, sizeof(array));
	result = dst_key_computesecret(key, key, &buf);
	assert_int_equal(result, DST_R_NOTPRIVATEKEY);
	result = key->func->computesecret(key, key, &buf);
	assert_int_equal(result, DST_R_COMPUTESECRETFAILURE);

	dst_key_free(&key);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(dh_computesecret, setup_test, teardown_test)
ISC_TEST_LIST_END

ISC_TEST_MAIN
