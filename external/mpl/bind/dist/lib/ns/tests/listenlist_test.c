/*	$NetBSD: listenlist_test.c,v 1.8 2022/09/23 12:15:36 christos Exp $	*/

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

#include <isc/util.h>

#if HAVE_CMOCKA

#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/list.h>
#include <isc/print.h>
#include <isc/random.h>

#include <dns/acl.h>

#include <ns/listenlist.h>

#include "nstest.h"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = ns_test_begin(NULL, true);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	ns_test_end();

	return (0);
}

/* test that ns_listenlist_default() works */
static void
ns_listenlist_default_test(void **state) {
	isc_result_t result;
	in_port_t port = 5300 + isc_random8();
	ns_listenlist_t *list = NULL;
	ns_listenelt_t *elt;
	int count;

	UNUSED(state);

	result = ns_listenlist_default(mctx, port, -1, false, &list);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(list);

	assert_false(ISC_LIST_EMPTY(list->elts));

	count = 0;
	elt = ISC_LIST_HEAD(list->elts);
	while (elt != NULL) {
		ns_listenelt_t *next = ISC_LIST_NEXT(elt, link);
		dns_acl_t *acl = NULL;

		dns_acl_attach(elt->acl, &acl);
		ISC_LIST_UNLINK(list->elts, elt, link);
		ns_listenelt_destroy(elt);
		elt = next;

		assert_true(dns_acl_isnone(acl));
		dns_acl_detach(&acl);
		count++;
	}

	assert_true(ISC_LIST_EMPTY(list->elts));
	assert_int_equal(count, 1);

	ns_listenlist_detach(&list);

	result = ns_listenlist_default(mctx, port, -1, true, &list);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_false(ISC_LIST_EMPTY(list->elts));

	/* This time just use ns_listenlist_detach() to destroy elements */
	count = 0;
	elt = ISC_LIST_HEAD(list->elts);
	while (elt != NULL) {
		ns_listenelt_t *next = ISC_LIST_NEXT(elt, link);
		assert_true(dns_acl_isany(elt->acl));
		elt = next;
		count++;
	}

	assert_int_equal(count, 1);

	ns_listenlist_detach(&list);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(ns_listenlist_default_test,
						_setup, _teardown),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");

	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* HAVE_CMOCKA */
