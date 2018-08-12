/*	$NetBSD: ht_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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

/* ! \file */

#include <config.h>

#include <atf-c.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <isc/hash.h>
#include <isc/ht.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <inttypes.h>

static void *
default_memalloc(void *arg, size_t size) {
	UNUSED(arg);
	if (size == 0U)
		size = 1;
	return (malloc(size));
}

static void
default_memfree(void *arg, void *ptr) {
	UNUSED(arg);
	free(ptr);
}

static void test_ht_full(int bits, uintptr_t count) {
	isc_ht_t *ht = NULL;
	isc_result_t result;
	isc_mem_t *mctx = NULL;
	uintptr_t i;

	result = isc_mem_createx2(0, 0, default_memalloc, default_memfree,
				  NULL, &mctx, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_ht_init(&ht, mctx, bits);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE(ht != NULL);

	for (i = 1; i < count; i++) {
		/*
		 * Note: snprintf() is followed with strlcat()
		 * to ensure we are always filling the 16 byte key.
		 */
		unsigned char key[16];
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_add(ht, key, 16, (void *) i);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	}

	for (i = 1; i < count; i++) {
		unsigned char key[16];
		void *f = NULL;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_find(ht, key, 16, &f);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
		ATF_REQUIRE_EQ(i, (uintptr_t) f);
	}

	for (i = 1; i < count; i++) {
		unsigned char key[16];
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_add(ht, key, 16, (void *) i);
		ATF_REQUIRE_EQ(result, ISC_R_EXISTS);
	}

	for (i = 1; i < count; i++) {
		char key[64];
		/*
		 * Note: the key size is now strlen(key) which is bigger
		 * then the keys added above.
		 */
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_add(ht, (const unsigned char *) key,
				    strlen(key), (void *) i);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
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
		ATF_REQUIRE_EQ(result, ISC_R_NOTFOUND);
		ATF_REQUIRE_EQ(f, NULL);
	}

	for (i = 1; i < count; i++) {
		char key[64];
		void *f = NULL;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_find(ht, (const unsigned char *) key,
				     strlen(key), &f);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
		ATF_REQUIRE_EQ(f, (void *) i);
	}

	for (i = 1; i < count; i++) {
		unsigned char key[16];
		void *f = NULL;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_delete(ht, key, 16);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
		result = isc_ht_find(ht, key, 16, &f);
		ATF_REQUIRE_EQ(result, ISC_R_NOTFOUND);
		ATF_REQUIRE_EQ(f, NULL);
	}

	for (i = 1; i < count; i++) {
		unsigned char key[16];
		/*
		 * Note: upper case KEY.
		 */
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " KEY of a raw hashtable!!", sizeof(key));
		result = isc_ht_add(ht, key, 16, (void *) i);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	}

	for (i = 1; i < count; i++) {
		char key[64];
		void *f = NULL;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_delete(ht, (const unsigned char *) key,
				       strlen(key));
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
		result = isc_ht_find(ht, (const unsigned char *) key,
				     strlen(key), &f);
		ATF_REQUIRE_EQ(result, ISC_R_NOTFOUND);
		ATF_REQUIRE_EQ(f, NULL);
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
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
		ATF_REQUIRE_EQ(i, (uintptr_t) f);
	}

	for (i = 1; i < count; i++) {
		unsigned char key[16];
		void *f = NULL;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, " key of a raw hashtable!!", sizeof(key));
		result = isc_ht_find(ht, key, 16, &f);
		ATF_REQUIRE_EQ(result, ISC_R_NOTFOUND);
		ATF_REQUIRE_EQ(f, NULL);
	}

	isc_ht_destroy(&ht);
	ATF_REQUIRE_EQ(ht, NULL);
}

static void test_ht_iterator() {
	isc_ht_t *ht = NULL;
	isc_result_t result;
	isc_mem_t *mctx = NULL;
	isc_ht_iter_t * iter = NULL;
	uintptr_t i;
	void *v;
	uintptr_t count = 10000;
	isc_uint32_t walked;
	unsigned char key[16];
	unsigned char *tkey;
	size_t tksize;

	result = isc_mem_createx2(0, 0, default_memalloc, default_memfree,
				  NULL, &mctx, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_ht_init(&ht, mctx, 16);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE(ht != NULL);
	for (i = 1; i <= count; i++) {
		/*
		 * Note that the string we're snprintfing is always > 16 bytes
		 * so we are always filling the key.
		 */
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, "key of a raw hashtable!!", sizeof(key));
		result = isc_ht_add(ht, key, 16, (void *) i);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	}

	walked = 0;
	result = isc_ht_iter_create(ht, &iter);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	for (result = isc_ht_iter_first(iter);
	     result == ISC_R_SUCCESS;
	     result = isc_ht_iter_next(iter))
	{
		isc_ht_iter_current(iter, &v);
		isc_ht_iter_currentkey(iter, &tkey, &tksize);
		ATF_REQUIRE_EQ(tksize, 16);
		i = (uintptr_t)v;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, "key of a raw hashtable!!", sizeof(key));
		ATF_REQUIRE_EQ(memcmp(key, tkey, 16), 0);
		walked++;
	}
	ATF_REQUIRE_EQ(walked, count);
	ATF_REQUIRE_EQ(result, ISC_R_NOMORE);

	/* erase odd */
	walked = 0;
	result = isc_ht_iter_first(iter);
	while (result == ISC_R_SUCCESS) {
		isc_ht_iter_current(iter, &v);
		isc_ht_iter_currentkey(iter, &tkey, &tksize);
		ATF_REQUIRE_EQ(tksize, 16);
		i = (uintptr_t)v;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, "key of a raw hashtable!!", sizeof(key));
		ATF_REQUIRE_EQ(memcmp(key, tkey, 16), 0);
		if ((uintptr_t)v % 2 == 0) {
			result = isc_ht_iter_delcurrent_next(iter);
		} else {
			result = isc_ht_iter_next(iter);
		}
		walked++;
	}
	ATF_REQUIRE_EQ(result, ISC_R_NOMORE);
	ATF_REQUIRE_EQ(walked, count);

	/* erase even */
	walked = 0;
	result = isc_ht_iter_first(iter);
	while (result == ISC_R_SUCCESS) {
		isc_ht_iter_current(iter, &v);
		isc_ht_iter_currentkey(iter, &tkey, &tksize);
		ATF_REQUIRE_EQ(tksize, 16);
		i = (uintptr_t)v;
		snprintf((char *)key, sizeof(key), "%u", (unsigned int)i);
		strlcat((char *)key, "key of a raw hashtable!!", sizeof(key));
		ATF_REQUIRE_EQ(memcmp(key, tkey, 16), 0);
		if ((uintptr_t)v % 2 == 1) {
			result = isc_ht_iter_delcurrent_next(iter);
		} else {
			result = isc_ht_iter_next(iter);
		}
		walked++;
	}
	ATF_REQUIRE_EQ(result, ISC_R_NOMORE);
	ATF_REQUIRE_EQ(walked, count/2);

	walked = 0;
	for (result = isc_ht_iter_first(iter);
	     result == ISC_R_SUCCESS;
	     result = isc_ht_iter_next(iter))
	{
		walked++;
	}

	ATF_REQUIRE_EQ(result, ISC_R_NOMORE);
	ATF_REQUIRE_EQ(walked, 0);

	isc_ht_destroy(&ht);
	ATF_REQUIRE_EQ(ht, NULL);
}

ATF_TC(isc_ht_20);
ATF_TC_HEAD(isc_ht_20, tc) {
	atf_tc_set_md_var(tc, "descr", "20 bit, 200K elements test");
}

ATF_TC_BODY(isc_ht_20, tc) {
	UNUSED(tc);
	test_ht_full(20, 200000);
}


ATF_TC(isc_ht_8);
ATF_TC_HEAD(isc_ht_8, tc) {
	atf_tc_set_md_var(tc, "descr", "8 bit, 20000 elements crowded test");
}

ATF_TC_BODY(isc_ht_8, tc) {
	UNUSED(tc);
	test_ht_full(8, 20000);
}

ATF_TC(isc_ht_1);
ATF_TC_HEAD(isc_ht_1, tc) {
	atf_tc_set_md_var(tc, "descr", "1 bit, 100 elements corner case test");
}

ATF_TC_BODY(isc_ht_1, tc) {
	UNUSED(tc);
	test_ht_full(1, 100);
}

/* xxxwpk we should limit the size of hashtable, 32bit doesn't make sense */
#if 0
ATF_TC(isc_ht_32);
ATF_TC_HEAD(isc_ht_32, tc) {
	atf_tc_set_md_var(tc, "descr", "32 bit, 10000 elements corner case test");
}

ATF_TC_BODY(isc_ht_32, tc) {
	UNUSED(tc);
	test_ht_full(32, 10000);
}
#endif

ATF_TC(isc_ht_iterator);
ATF_TC_HEAD(isc_ht_iterator, tc) {
	atf_tc_set_md_var(tc, "descr", "hashtable iterator");
}

ATF_TC_BODY(isc_ht_iterator, tc) {
	UNUSED(tc);
	test_ht_iterator();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_ht_20);
	ATF_TP_ADD_TC(tp, isc_ht_8);
	ATF_TP_ADD_TC(tp, isc_ht_1);
/*	ATF_TP_ADD_TC(tp, isc_ht_32); */
	ATF_TP_ADD_TC(tp, isc_ht_iterator);
	return (atf_no_error());
}
