/*	$NetBSD: dh_test.c,v 1.3.2.3 2020/04/13 08:02:57 martin Exp $	*/

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

#include <dns/name.h>

#include <dst/result.h>

#include <pk11/site.h>

#include "../dst_internal.h"

#include "dnstest.h"

#if USE_OPENSSL
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

/* OpenSSL DH_compute_key() failure */
static void
dh_computesecret(void **state) {
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
				  DST_TYPE_PUBLIC | DST_TYPE_KEY,
				  "./", mctx, &key);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_init(&buf, array, sizeof(array));
	result = dst_key_computesecret(key, key, &buf);
	assert_int_equal(result, DST_R_NOTPRIVATEKEY);
	result = key->func->computesecret(key, key, &buf);
	assert_int_equal(result, DST_R_COMPUTESECRETFAILURE);

	dst_key_free(&key);
}
#endif /* USE_OPENSSL */

int
main(void) {
#if USE_OPENSSL
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(dh_computesecret,
						_setup, _teardown),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
#else
	print_message("1..0 # Skipped: dh test broken with PKCS11");
#endif
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif
