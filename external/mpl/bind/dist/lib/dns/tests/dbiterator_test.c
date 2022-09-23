/*	$NetBSD: dbiterator_test.c,v 1.8 2022/09/23 12:15:32 christos Exp $	*/

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

#if HAVE_CMOCKA

#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/util.h>

#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/name.h>

#include "dnstest.h"

#define BUFLEN	    255
#define BIGBUFLEN   (64 * 1024)
#define TEST_ORIGIN "test"

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

static isc_result_t
make_name(const char *src, dns_name_t *name) {
	isc_buffer_t b;
	isc_buffer_constinit(&b, src, strlen(src));
	isc_buffer_add(&b, strlen(src));
	return (dns_name_fromtext(name, &b, dns_rootname, 0, NULL));
}

/* create: make sure we can create a dbiterator */
static void
test_create(const char *filename) {
	isc_result_t result;
	dns_db_t *db = NULL;
	dns_dbiterator_t *iter = NULL;

	result = dns_test_loaddb(&db, dns_dbtype_cache, TEST_ORIGIN, filename);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_createiterator(db, 0, &iter);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_dbiterator_destroy(&iter);
	dns_db_detach(&db);
}

static void
create(void **state) {
	UNUSED(state);

	test_create("testdata/dbiterator/zone1.data");
}

static void
create_nsec3(void **state) {
	UNUSED(state);

	test_create("testdata/dbiterator/zone2.data");
}

/* walk: walk a database */
static void
test_walk(const char *filename, int nodes) {
	isc_result_t result;
	dns_db_t *db = NULL;
	dns_dbiterator_t *iter = NULL;
	dns_dbnode_t *node = NULL;
	dns_name_t *name;
	dns_fixedname_t f;
	int i = 0;

	name = dns_fixedname_initname(&f);

	result = dns_test_loaddb(&db, dns_dbtype_cache, TEST_ORIGIN, filename);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_createiterator(db, 0, &iter);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (result = dns_dbiterator_first(iter); result == ISC_R_SUCCESS;
	     result = dns_dbiterator_next(iter))
	{
		result = dns_dbiterator_current(iter, &node, name);
		if (result == DNS_R_NEWORIGIN) {
			result = ISC_R_SUCCESS;
		}
		assert_int_equal(result, ISC_R_SUCCESS);
		dns_db_detachnode(db, &node);
		i++;
	}

	assert_int_equal(i, nodes);

	dns_dbiterator_destroy(&iter);
	dns_db_detach(&db);
}

static void
walk(void **state) {
	UNUSED(state);

	test_walk("testdata/dbiterator/zone1.data", 12);
}

static void
walk_nsec3(void **state) {
	UNUSED(state);

	test_walk("testdata/dbiterator/zone2.data", 33);
}

/* reverse: walk database backwards */
static void
test_reverse(const char *filename) {
	isc_result_t result;
	dns_db_t *db = NULL;
	dns_dbiterator_t *iter = NULL;
	dns_dbnode_t *node = NULL;
	dns_name_t *name;
	dns_fixedname_t f;
	int i = 0;

	name = dns_fixedname_initname(&f);

	result = dns_test_loaddb(&db, dns_dbtype_cache, TEST_ORIGIN, filename);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_createiterator(db, 0, &iter);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (result = dns_dbiterator_last(iter); result == ISC_R_SUCCESS;
	     result = dns_dbiterator_prev(iter))
	{
		result = dns_dbiterator_current(iter, &node, name);
		if (result == DNS_R_NEWORIGIN) {
			result = ISC_R_SUCCESS;
		}
		assert_int_equal(result, ISC_R_SUCCESS);
		dns_db_detachnode(db, &node);
		i++;
	}

	assert_int_equal(i, 12);

	dns_dbiterator_destroy(&iter);
	dns_db_detach(&db);
}

static void
reverse(void **state) {
	UNUSED(state);

	test_reverse("testdata/dbiterator/zone1.data");
}

static void
reverse_nsec3(void **state) {
	UNUSED(state);

	test_reverse("testdata/dbiterator/zone2.data");
}

/* seek: walk database starting at a particular node */
static void
test_seek_node(const char *filename, int nodes) {
	isc_result_t result;
	dns_db_t *db = NULL;
	dns_dbiterator_t *iter = NULL;
	dns_dbnode_t *node = NULL;
	dns_name_t *name, *seekname;
	dns_fixedname_t f1, f2;
	int i = 0;

	name = dns_fixedname_initname(&f1);
	seekname = dns_fixedname_initname(&f2);

	result = dns_test_loaddb(&db, dns_dbtype_cache, TEST_ORIGIN, filename);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_createiterator(db, 0, &iter);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = make_name("c." TEST_ORIGIN, seekname);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dbiterator_seek(iter, seekname);
	assert_int_equal(result, ISC_R_SUCCESS);

	while (result == ISC_R_SUCCESS) {
		result = dns_dbiterator_current(iter, &node, name);
		if (result == DNS_R_NEWORIGIN) {
			result = ISC_R_SUCCESS;
		}
		assert_int_equal(result, ISC_R_SUCCESS);
		dns_db_detachnode(db, &node);
		result = dns_dbiterator_next(iter);
		i++;
	}

	assert_int_equal(i, nodes);

	dns_dbiterator_destroy(&iter);
	dns_db_detach(&db);
}

static void
seek_node(void **state) {
	UNUSED(state);

	test_seek_node("testdata/dbiterator/zone1.data", 9);
}

static void
seek_node_nsec3(void **state) {
	UNUSED(state);

	test_seek_node("testdata/dbiterator/zone2.data", 30);
}

/*
 * seek_emty: walk database starting at an empty nonterminal node
 * (should fail)
 */
static void
test_seek_empty(const char *filename) {
	isc_result_t result;
	dns_db_t *db = NULL;
	dns_dbiterator_t *iter = NULL;
	dns_name_t *seekname;
	dns_fixedname_t f1;

	seekname = dns_fixedname_initname(&f1);

	result = dns_test_loaddb(&db, dns_dbtype_cache, TEST_ORIGIN, filename);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_createiterator(db, 0, &iter);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = make_name("d." TEST_ORIGIN, seekname);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dbiterator_seek(iter, seekname);
	assert_int_equal(result, DNS_R_PARTIALMATCH);

	dns_dbiterator_destroy(&iter);
	dns_db_detach(&db);
}

static void
seek_empty(void **state) {
	UNUSED(state);

	test_seek_empty("testdata/dbiterator/zone1.data");
}

static void
seek_empty_nsec3(void **state) {
	UNUSED(state);

	test_seek_empty("testdata/dbiterator/zone2.data");
}

/*
 * seek_nx: walk database starting at a nonexistent node
 */
static void
test_seek_nx(const char *filename) {
	isc_result_t result;
	dns_db_t *db = NULL;
	dns_dbiterator_t *iter = NULL;
	dns_name_t *seekname;
	dns_fixedname_t f1;

	seekname = dns_fixedname_initname(&f1);

	result = dns_test_loaddb(&db, dns_dbtype_cache, TEST_ORIGIN, filename);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_createiterator(db, 0, &iter);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = make_name("nonexistent." TEST_ORIGIN, seekname);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dbiterator_seek(iter, seekname);
	assert_int_equal(result, DNS_R_PARTIALMATCH);

	result = make_name("nonexistent.", seekname);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_dbiterator_seek(iter, seekname);
	assert_int_equal(result, ISC_R_NOTFOUND);

	dns_dbiterator_destroy(&iter);
	dns_db_detach(&db);
}

static void
seek_nx(void **state) {
	UNUSED(state);

	test_seek_nx("testdata/dbiterator/zone1.data");
}

static void
seek_nx_nsec3(void **state) {
	UNUSED(state);

	test_seek_nx("testdata/dbiterator/zone2.data");
}

/*
 * XXX:
 * dns_dbiterator API calls that are not yet part of this unit test:
 *
 * dns_dbiterator_pause
 * dns_dbiterator_origin
 * dns_dbiterator_setcleanmode
 */
int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(create, _setup, _teardown),
		cmocka_unit_test_setup_teardown(create_nsec3, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(walk, _setup, _teardown),
		cmocka_unit_test_setup_teardown(walk_nsec3, _setup, _teardown),
		cmocka_unit_test_setup_teardown(reverse, _setup, _teardown),
		cmocka_unit_test_setup_teardown(reverse_nsec3, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(seek_node, _setup, _teardown),
		cmocka_unit_test_setup_teardown(seek_node_nsec3, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(seek_empty, _setup, _teardown),
		cmocka_unit_test_setup_teardown(seek_empty_nsec3, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(seek_nx, _setup, _teardown),
		cmocka_unit_test_setup_teardown(seek_nx_nsec3, _setup,
						_teardown),
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

#endif /* if HAVE_CMOCKA */
