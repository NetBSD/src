/*	$NetBSD: ht_test.c,v 1.2.2.2 2024/02/25 15:47:52 martin Exp $	*/

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/hash.h>
#include <isc/ht.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <tests/isc.h>

/* INCLUDE LAST */

#define mctx __mctx
#include "ht.c"
#undef mctx

static void
test_ht_full(int bits, uintptr_t count) {
	isc_ht_t *ht = NULL;
	isc_result_t result;
	uintptr_t i;

	isc_ht_init(&ht, mctx, bits, ISC_HT_CASE_SENSITIVE);
	assert_non_null(ht);

	for (i = 1; i < count; i++) {
		/*
		 * Note: snprintf() is followed with strlcat()
		 * to ensure we are always filling the 16 byte key.
		 */
		unsigned char key[16];
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_add(ht, key, 16, (void *)i);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	for (i = 1; i < count; i++) {
		unsigned char key[16];
		void *f = NULL;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_find(ht, key, 16, &f);
		assert_int_equal(result, ISC_R_SUCCESS);
		assert_ptr_equal((void *)i, (void *)f);
	}

	for (i = 1; i < count; i++) {
		unsigned char key[16];
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_add(ht, key, 16, (void *)i);
		assert_int_equal(result, ISC_R_EXISTS);
	}

	for (i = 1; i < count; i++) {
		char key[64];
		/*
		 * Note: the key size is now strlen(key) which is bigger
		 * then the keys added above.
		 */
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_add(ht, (const unsigned char *)key, strlen(key),
				    (void *)i);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	for (i = 1; i < count; i++) {
		unsigned char key[16];
		void *f = NULL;
		/*
		 * Note: case of KEY is now in capitals,
		 */
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " KEY of a raw hashtable!!", sizeof(key));
		result = isc_ht_find(ht, key, 16, &f);
		assert_int_equal(result, ISC_R_NOTFOUND);
		assert_null(f);
	}

	for (i = 1; i < count; i++) {
		char key[64];
		void *f = NULL;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_find(ht, (const unsigned char *)key,
				     strlen(key), &f);
		assert_int_equal(result, ISC_R_SUCCESS);
		assert_ptr_equal(f, (void *)i);
	}

	for (i = 1; i < count; i++) {
		unsigned char key[16];
		void *f = NULL;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_delete(ht, key, 16);
		assert_int_equal(result, ISC_R_SUCCESS);
		result = isc_ht_find(ht, key, 16, &f);
		assert_int_equal(result, ISC_R_NOTFOUND);
		assert_null(f);
	}

	for (i = 1; i < count; i++) {
		unsigned char key[16];
		/*
		 * Note: upper case KEY.
		 */
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " KEY of a raw hashtable!!", sizeof(key));
		result = isc_ht_add(ht, key, 16, (void *)i);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	for (i = 1; i < count; i++) {
		char key[64];
		void *f = NULL;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_delete(ht, (const unsigned char *)key,
				       strlen(key));
		assert_int_equal(result, ISC_R_SUCCESS);
		result = isc_ht_find(ht, (const unsigned char *)key,
				     strlen(key), &f);
		assert_int_equal(result, ISC_R_NOTFOUND);
		assert_null(f);
	}

	for (i = 1; i < count; i++) {
		unsigned char key[16];
		void *f = NULL;
		/*
		 * Note: case of KEY is now in capitals,
		 */
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " KEY of a raw hashtable!!", sizeof(key));
		result = isc_ht_find(ht, key, 16, &f);
		assert_int_equal(result, ISC_R_SUCCESS);
		assert_ptr_equal((void *)i, (void *)f);
	}

	for (i = 1; i < count; i++) {
		unsigned char key[16];
		void *f = NULL;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_find(ht, key, 16, &f);
		assert_int_equal(result, ISC_R_NOTFOUND);
		assert_null(f);
	}

	isc_ht_destroy(&ht);
	assert_null(ht);
}

static void
test_ht_iterator(void) {
	isc_ht_t *ht = NULL;
	isc_result_t result;
	isc_ht_iter_t *iter = NULL;
	uintptr_t i;
	uintptr_t count = 10000;
	uint32_t walked;
	unsigned char key[16];
	size_t tksize;

	isc_ht_init(&ht, mctx, 16, ISC_HT_CASE_SENSITIVE);
	assert_non_null(ht);
	for (i = 1; i <= count; i++) {
		/*
		 * Note that the string we're snprintfing is always > 16 bytes
		 * so we are always filling the key.
		 */
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, "key of a raw hashtable!!", sizeof(key));
		result = isc_ht_add(ht, key, 16, (void *)i);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	walked = 0;
	isc_ht_iter_create(ht, &iter);

	for (result = isc_ht_iter_first(iter); result == ISC_R_SUCCESS;
	     result = isc_ht_iter_next(iter))
	{
		unsigned char *tkey = NULL;
		void *v = NULL;

		isc_ht_iter_current(iter, &v);
		isc_ht_iter_currentkey(iter, &tkey, &tksize);
		assert_int_equal(tksize, 16);
		i = (uintptr_t)v;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, "key of a raw hashtable!!", sizeof(key));
		assert_memory_equal(key, tkey, 16);
		walked++;
	}
	assert_int_equal(walked, count);
	assert_int_equal(result, ISC_R_NOMORE);

	/* erase odd */
	walked = 0;
	result = isc_ht_iter_first(iter);
	while (result == ISC_R_SUCCESS) {
		unsigned char *tkey = NULL;
		void *v = NULL;

		isc_ht_iter_current(iter, &v);
		isc_ht_iter_currentkey(iter, &tkey, &tksize);
		assert_int_equal(tksize, 16);
		i = (uintptr_t)v;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, "key of a raw hashtable!!", sizeof(key));
		assert_memory_equal(key, tkey, 16);
		if ((uintptr_t)v % 2 == 0) {
			result = isc_ht_iter_delcurrent_next(iter);
		} else {
			result = isc_ht_iter_next(iter);
		}
		walked++;
	}
	assert_int_equal(result, ISC_R_NOMORE);
	assert_int_equal(walked, count);

	/* erase even */
	walked = 0;
	result = isc_ht_iter_first(iter);
	while (result == ISC_R_SUCCESS) {
		unsigned char *tkey = NULL;
		void *v = NULL;

		isc_ht_iter_current(iter, &v);
		isc_ht_iter_currentkey(iter, &tkey, &tksize);
		assert_int_equal(tksize, 16);
		i = (uintptr_t)v;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, "key of a raw hashtable!!", sizeof(key));
		assert_memory_equal(key, tkey, 16);
		if ((uintptr_t)v % 2 == 1) {
			result = isc_ht_iter_delcurrent_next(iter);
		} else {
			result = isc_ht_iter_next(iter);
		}
		walked++;
	}
	assert_int_equal(result, ISC_R_NOMORE);
	assert_int_equal(walked, count / 2);

	walked = 0;
	for (result = isc_ht_iter_first(iter); result == ISC_R_SUCCESS;
	     result = isc_ht_iter_next(iter))
	{
		walked++;
	}

	assert_int_equal(result, ISC_R_NOMORE);
	assert_int_equal(walked, 0);

	isc_ht_iter_destroy(&iter);
	assert_null(iter);

	isc_ht_destroy(&ht);
	assert_null(ht);
}

/* 20 bit, 200K elements test */
ISC_RUN_TEST_IMPL(isc_ht_20) {
	UNUSED(state);
	test_ht_full(20, 200000);
}

/* 8 bit, 20000 elements crowded test */
ISC_RUN_TEST_IMPL(isc_ht_8) {
	UNUSED(state);
	test_ht_full(8, 20000);
}

ISC_RUN_TEST_IMPL(isc_ht_1) {
	UNUSED(state);
	test_ht_full(1, 100);
}

/* test hashtable iterator */

ISC_RUN_TEST_IMPL(isc_ht_iterator) {
	UNUSED(state);
	test_ht_iterator();
}

ISC_RUN_TEST_IMPL(isc_ht_case) {
	isc_ht_t *ht = NULL;
	void *f = NULL;
	isc_result_t result = ISC_R_UNSET;

	unsigned char lower[16] = { "test case" };
	unsigned char same[16] = { "test case" };
	unsigned char upper[16] = { "TEST CASE" };
	unsigned char mixed[16] = { "tEsT CaSe" };

	isc_ht_init(&ht, mctx, 8, ISC_HT_CASE_SENSITIVE);
	assert_non_null(ht);

	result = isc_ht_add(ht, lower, 16, (void *)lower);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_ht_add(ht, same, 16, (void *)same);
	assert_int_equal(result, ISC_R_EXISTS);

	result = isc_ht_add(ht, upper, 16, (void *)upper);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_ht_find(ht, mixed, 16, &f);
	assert_int_equal(result, ISC_R_NOTFOUND);
	assert_null(f);

	isc_ht_destroy(&ht);
	assert_null(ht);

	isc_ht_init(&ht, mctx, 8, ISC_HT_CASE_INSENSITIVE);
	assert_non_null(ht);

	result = isc_ht_add(ht, lower, 16, (void *)lower);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_ht_add(ht, same, 16, (void *)same);
	assert_int_equal(result, ISC_R_EXISTS);

	result = isc_ht_add(ht, upper, 16, (void *)upper);
	assert_int_equal(result, ISC_R_EXISTS);

	result = isc_ht_find(ht, mixed, 16, &f);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_ptr_equal(f, &lower);

	isc_ht_destroy(&ht);
	assert_null(ht);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(isc_ht_case)
ISC_TEST_ENTRY(isc_ht_20)
ISC_TEST_ENTRY(isc_ht_8)
ISC_TEST_ENTRY(isc_ht_1)
ISC_TEST_ENTRY(isc_ht_iterator)
ISC_TEST_LIST_END

ISC_TEST_MAIN
