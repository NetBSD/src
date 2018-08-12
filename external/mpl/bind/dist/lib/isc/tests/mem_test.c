/*	$NetBSD: mem_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <atf-c.h>

#include "isctest.h"

#include <isc/file.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/stdio.h>

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

ATF_TC(isc_mem);
ATF_TC_HEAD(isc_mem, tc) {
	atf_tc_set_md_var(tc, "descr", "general memory system tests");
}

#define	MP1_FREEMAX	10
#define	MP1_FILLCNT	10
#define	MP1_MAXALLOC	30

#define	MP2_FREEMAX	25
#define	MP2_FILLCNT	25

ATF_TC_BODY(isc_mem, tc) {
	isc_result_t result;
	void *items1[50];
	void *items2[50];
	void *tmp;
	isc_mem_t *localmctx = NULL;
	isc_mempool_t *mp1 = NULL, *mp2 = NULL;
	unsigned int i, j;
	int rval;

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_mem_create(0, 0, &localmctx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_mempool_create(localmctx, 24, &mp1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_mempool_create(localmctx, 31, &mp2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_mempool_setfreemax(mp1, MP1_FREEMAX);
	isc_mempool_setfillcount(mp1, MP1_FILLCNT);
	isc_mempool_setmaxalloc(mp1, MP1_MAXALLOC);

	/*
	 * Allocate MP1_MAXALLOC items from the pool.  This is our max.
	 */
	for (i = 0; i < MP1_MAXALLOC; i++) {
		items1[i] = isc_mempool_get(mp1);
		ATF_CHECK(items1[i] != NULL);
	}

	/*
	 * Try to allocate one more.  This should fail.
	 */
	tmp = isc_mempool_get(mp1);
	ATF_CHECK_EQ(tmp, NULL);

	/*
	 * Free the first 11 items.  Verify that there are 10 free items on
	 * the free list (which is our max).
	 */
	for (i = 0; i < 11; i++) {
		isc_mempool_put(mp1, items1[i]);
		items1[i] = NULL;
	}

	rval = isc_mempool_getfreecount(mp1);
	ATF_CHECK_EQ(rval, 10);

	rval = isc_mempool_getallocated(mp1);
	ATF_CHECK_EQ(rval, 19);

	/*
	 * Now, beat up on mp2 for a while.  Allocate 50 items, then free
	 * them, then allocate 50 more, etc.
	 */

	isc_mempool_setfreemax(mp2, 25);
	isc_mempool_setfillcount(mp2, 25);

	for (j = 0; j < 500000; j++) {
		for (i = 0; i < 50; i++) {
			items2[i] = isc_mempool_get(mp2);
			ATF_CHECK(items2[i] != NULL);
		}
		for (i = 0; i < 50; i++) {
			isc_mempool_put(mp2, items2[i]);
			items2[i] = NULL;
		}
	}

	/*
	 * Free all the other items and blow away this pool.
	 */
	for (i = 11; i < MP1_MAXALLOC; i++) {
		isc_mempool_put(mp1, items1[i]);
		items1[i] = NULL;
	}

	isc_mempool_destroy(&mp1);
	isc_mempool_destroy(&mp2);

	isc_mem_destroy(&localmctx);

	isc_test_end();
}

ATF_TC(isc_mem_total);
ATF_TC_HEAD(isc_mem_total, tc) {
	atf_tc_set_md_var(tc, "descr", "test TotalUse calculation");
}

ATF_TC_BODY(isc_mem_total, tc) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	size_t before, after;
	ssize_t diff;
	int i;

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* Local alloc, free */
	mctx2 = NULL;
	result = isc_mem_createx2(0, 0, default_memalloc, default_memfree,
				  NULL, &mctx2, 0);
	if (result != ISC_R_SUCCESS)
		goto out;

	before = isc_mem_total(mctx2);

	for (i = 0; i < 100000; i++) {
		void *ptr;

		ptr = isc_mem_allocate(mctx2, 2048);
		isc_mem_free(mctx2, ptr);
	}

	after = isc_mem_total(mctx2);
	diff = after - before;

	printf("total_before=%lu, total_after=%lu, total_diff=%lu\n",
	       (unsigned long)before, (unsigned long)after,
	       (unsigned long)diff);
	/* 2048 +8 bytes extra for size_info */
	ATF_CHECK_EQ(diff, (2048 + 8) * 100000);

	/* ISC_MEMFLAG_INTERNAL */

	before = isc_mem_total(mctx);

	for (i = 0; i < 100000; i++) {
		void *ptr;

		ptr = isc_mem_allocate(mctx, 2048);
		isc_mem_free(mctx, ptr);
	}

	after = isc_mem_total(mctx);
	diff = after - before;

	printf("total_before=%lu, total_after=%lu, total_diff=%lu\n",
	       (unsigned long)before, (unsigned long)after,
	       (unsigned long)diff);
	/* 2048 +8 bytes extra for size_info */
	ATF_CHECK_EQ(diff, (2048 + 8) * 100000);

 out:
	if (mctx2 != NULL)
		isc_mem_destroy(&mctx2);

	isc_test_end();
}

ATF_TC(isc_mem_inuse);
ATF_TC_HEAD(isc_mem_inuse, tc) {
	atf_tc_set_md_var(tc, "descr", "test InUse calculation");
}

ATF_TC_BODY(isc_mem_inuse, tc) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	size_t before, during, after;
	ssize_t diff;
	void *ptr;

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	mctx2 = NULL;
	result = isc_mem_createx2(0, 0, default_memalloc, default_memfree,
				  NULL, &mctx2, 0);
	if (result != ISC_R_SUCCESS)
		goto out;

	before = isc_mem_inuse(mctx2);
	ptr = isc_mem_allocate(mctx2, 1024000);
	during = isc_mem_inuse(mctx2);
	isc_mem_free(mctx2, ptr);
	after = isc_mem_inuse(mctx2);

	diff = after - before;

	printf("inuse_before=%lu, inuse_during=%lu, inuse_after=%lu\n",
	       (unsigned long)before, (unsigned long)during,
	       (unsigned long)after);
	ATF_REQUIRE_EQ(diff, 0);

 out:
	if (mctx2 != NULL)
		isc_mem_destroy(&mctx2);

	isc_test_end();
}

#if ISC_MEM_TRACKLINES
ATF_TC(isc_mem_noflags);
ATF_TC_HEAD(isc_mem_noflags, tc) {
	atf_tc_set_md_var(tc, "descr", "test mem with no flags");
}

ATF_TC_BODY(isc_mem_noflags, tc) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	char buf[4096], *p, *q;
	FILE *f;
	void *ptr;

	result = isc_stdio_open("mem.output", "w", &f);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_mem_createx2(0, 0, default_memalloc, default_memfree,
				  NULL, &mctx2, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	isc_mem_debugging = 0;
	ptr = isc_mem_get(mctx2, 2048);
	ATF_CHECK(ptr != NULL);
	isc__mem_printactive(mctx2, f);
	isc_mem_put(mctx2, ptr, 2048);
	isc_mem_destroy(&mctx2);
	isc_stdio_close(f);

	memset(buf, 0, sizeof(buf));
	result = isc_stdio_open("mem.output", "r", &f);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = isc_stdio_read(buf, sizeof(buf), 1, f, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_EOF);
	isc_stdio_close(f);
	isc_file_remove("mem.output");

	buf[sizeof(buf) - 1] = 0;

	p = strchr(buf, '\n');
	ATF_REQUIRE(p != NULL);
	ATF_REQUIRE(p < buf + sizeof(buf) - 2);
	p += 2;
	q = strchr(p, '\n');
	ATF_REQUIRE(q != NULL);
	*q = '\0';
	ATF_CHECK_STREQ(p, "None.");

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;
	isc_test_end();
}

ATF_TC(isc_mem_recordflag);
ATF_TC_HEAD(isc_mem_recordflag, tc) {
	atf_tc_set_md_var(tc, "descr", "test mem with record flag");
}

ATF_TC_BODY(isc_mem_recordflag, tc) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	char buf[4096], *p;
	FILE *f;
	void *ptr;

	result = isc_stdio_open("mem.output", "w", &f);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_test_begin(NULL, ISC_FALSE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_mem_createx2(0, 0, default_memalloc, default_memfree,
				  NULL, &mctx2, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ptr = isc_mem_get(mctx2, 2048);
	ATF_CHECK(ptr != NULL);
	isc__mem_printactive(mctx2, f);
	isc_mem_put(mctx2, ptr, 2048);
	isc_mem_destroy(&mctx2);
	isc_stdio_close(f);

	memset(buf, 0, sizeof(buf));
	result = isc_stdio_open("mem.output", "r", &f);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = isc_stdio_read(buf, sizeof(buf), 1, f, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_EOF);
	isc_stdio_close(f);
	isc_file_remove("mem.output");

	buf[sizeof(buf) - 1] = 0;

	p = strchr(buf, '\n');
	ATF_REQUIRE(p != NULL);
	ATF_REQUIRE(p < buf + sizeof(buf) - 2);
	ATF_CHECK(strncmp(p + 2, "ptr ", 4) == 0);
	p = strchr(p + 1, '\n');
	ATF_REQUIRE(p != NULL);
	ATF_CHECK(strlen(p) == 1);

	isc_test_end();
}

ATF_TC(isc_mem_traceflag);
ATF_TC_HEAD(isc_mem_traceflag, tc) {
	atf_tc_set_md_var(tc, "descr", "test mem with trace flag");
}

ATF_TC_BODY(isc_mem_traceflag, tc) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	char buf[4096], *p;
	FILE *f;
	void *ptr;

	/* redirect stderr so we can check trace output */
	f = freopen("mem.output", "w", stderr);
	ATF_REQUIRE(f != NULL);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_mem_createx2(0, 0, default_memalloc, default_memfree,
				  NULL, &mctx2, 0);
	isc_mem_debugging = ISC_MEM_DEBUGTRACE;
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ptr = isc_mem_get(mctx2, 2048);
	ATF_CHECK(ptr != NULL);
	isc__mem_printactive(mctx2, f);
	isc_mem_put(mctx2, ptr, 2048);
	isc_mem_destroy(&mctx2);
	isc_stdio_close(f);

	memset(buf, 0, sizeof(buf));
	result = isc_stdio_open("mem.output", "r", &f);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = isc_stdio_read(buf, sizeof(buf), 1, f, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_EOF);
	isc_stdio_close(f);
	isc_file_remove("mem.output");

	/* return stderr to TTY so we can see errors */
	f = freopen("/dev/tty", "w", stderr);

	buf[sizeof(buf) - 1] = 0;

	ATF_CHECK(strncmp(buf, "add ", 4) == 0);
	p = strchr(buf, '\n');
	ATF_REQUIRE(p != NULL);
	p = strchr(p + 1, '\n');
	ATF_REQUIRE(p != NULL);
	ATF_REQUIRE(p < buf + sizeof(buf) - 2);
	ATF_CHECK(strncmp(p + 2, "ptr ", 4) == 0);
	p = strchr(p + 1, '\n');
	ATF_REQUIRE(p != NULL);
	ATF_CHECK(strncmp(p + 1, "del ", 4) == 0);

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;
	isc_test_end();
}
#endif

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_mem);
	ATF_TP_ADD_TC(tp, isc_mem_total);
	ATF_TP_ADD_TC(tp, isc_mem_inuse);
#if ISC_MEM_TRACKLINES
	ATF_TP_ADD_TC(tp, isc_mem_noflags);
	ATF_TP_ADD_TC(tp, isc_mem_recordflag);
	ATF_TP_ADD_TC(tp, isc_mem_traceflag);
#endif

	return (atf_no_error());
}
