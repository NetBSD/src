/*	$NetBSD: mem_test.c,v 1.3.2.3 2020/04/08 14:07:09 martin Exp $	*/

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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/file.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/stdio.h>
#include <isc/util.h>

#include "../mem_p.h"

#include "isctest.h"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = isc_test_begin(NULL, true, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static void *
default_memalloc(void *arg, size_t size) {
	UNUSED(arg);
	if (size == 0U) {
		size = 1;
	}
	/* cppcheck-suppress leakNoVarFunctionCall */
	return (malloc(size));
}

static void
default_memfree(void *arg, void *ptr) {
	UNUSED(arg);
	free(ptr);
}

static int
_teardown(void **state) {
	UNUSED(state);

	isc_test_end();

	return (0);
}

#define	MP1_FREEMAX	10
#define	MP1_FILLCNT	10
#define	MP1_MAXALLOC	30

#define	MP2_FREEMAX	25
#define	MP2_FILLCNT	25

/* general memory system tests */
static void
isc_mem_test(void **state) {
	isc_result_t result;
	void *items1[50];
	void *items2[50];
	void *tmp;
	isc_mem_t *localmctx = NULL;
	isc_mempool_t *mp1 = NULL, *mp2 = NULL;
	unsigned int i, j;
	int rval;

	UNUSED(state);

	result = isc_mem_create(0, 0, &localmctx);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_mempool_create(localmctx, 24, &mp1);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_mempool_create(localmctx, 31, &mp2);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_mempool_setfreemax(mp1, MP1_FREEMAX);
	isc_mempool_setfillcount(mp1, MP1_FILLCNT);
	isc_mempool_setmaxalloc(mp1, MP1_MAXALLOC);

	/*
	 * Allocate MP1_MAXALLOC items from the pool.  This is our max.
	 */
	for (i = 0; i < MP1_MAXALLOC; i++) {
		items1[i] = isc_mempool_get(mp1);
		assert_non_null(items1[i]);
	}

	/*
	 * Try to allocate one more.  This should fail.
	 */
	tmp = isc_mempool_get(mp1);
	assert_null(tmp);

	/*
	 * Free the first 11 items.  Verify that there are 10 free items on
	 * the free list (which is our max).
	 */
	for (i = 0; i < 11; i++) {
		isc_mempool_put(mp1, items1[i]);
		items1[i] = NULL;
	}

	rval = isc_mempool_getfreecount(mp1);
	assert_int_equal(rval, 10);

	rval = isc_mempool_getallocated(mp1);
	assert_int_equal(rval, 19);

	/*
	 * Now, beat up on mp2 for a while.  Allocate 50 items, then free
	 * them, then allocate 50 more, etc.
	 */

	isc_mempool_setfreemax(mp2, 25);
	isc_mempool_setfillcount(mp2, 25);

	for (j = 0; j < 500000; j++) {
		for (i = 0; i < 50; i++) {
			items2[i] = isc_mempool_get(mp2);
			assert_non_null(items2[i]);
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

	result = isc_mem_createx(0, 0, default_memalloc, default_memfree,
				 NULL, &localmctx,
				 ISC_MEMFLAG_FILL | ISC_MEMFLAG_INTERNAL);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_mempool_create(localmctx, 2, &mp1);
	assert_int_equal(result, ISC_R_SUCCESS);

	tmp = isc_mempool_get(mp1);
	assert_non_null(tmp);

	isc_mempool_put(mp1, tmp);

	isc_mempool_destroy(&mp1);

	isc_mem_destroy(&localmctx);

}

/* test TotalUse calculation */
static void
isc_mem_total_test(void **state) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	size_t before, after;
	ssize_t diff;
	int i;

	UNUSED(state);

	/* Local alloc, free */
	mctx2 = NULL;
	result = isc_mem_createx(0, 0, default_memalloc, default_memfree,
				 NULL, &mctx2, 0);
	if (result != ISC_R_SUCCESS) {
		goto out;
	}

	before = isc_mem_total(mctx2);

	for (i = 0; i < 100000; i++) {
		void *ptr;

		ptr = isc_mem_allocate(mctx2, 2048);
		isc_mem_free(mctx2, ptr);
	}

	after = isc_mem_total(mctx2);
	diff = after - before;

	/* 2048 +8 bytes extra for size_info */
	assert_int_equal(diff, (2048 + 8) * 100000);

	/* ISC_MEMFLAG_INTERNAL */

	before = isc_mem_total(mctx);

	for (i = 0; i < 100000; i++) {
		void *ptr;

		ptr = isc_mem_allocate(mctx, 2048);
		isc_mem_free(mctx, ptr);
	}

	after = isc_mem_total(mctx);
	diff = after - before;

	/* 2048 +8 bytes extra for size_info */
	assert_int_equal(diff, (2048 + 8) * 100000);

 out:
	if (mctx2 != NULL) {
		isc_mem_destroy(&mctx2);
	}

}

/* test InUse calculation */
static void
isc_mem_inuse_test(void **state) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	size_t before, after;
	ssize_t diff;
	void *ptr;

	UNUSED(state);

	mctx2 = NULL;
	result = isc_mem_createx(0, 0, default_memalloc, default_memfree,
				 NULL, &mctx2, 0);
	if (result != ISC_R_SUCCESS) {
		goto out;
	}

	before = isc_mem_inuse(mctx2);
	ptr = isc_mem_allocate(mctx2, 1024000);
	isc_mem_free(mctx2, ptr);
	after = isc_mem_inuse(mctx2);

	diff = after - before;

	assert_int_equal(diff, 0);

 out:
	if (mctx2 != NULL) {
		isc_mem_destroy(&mctx2);
	}

}

#if ISC_MEM_TRACKLINES

/* test mem with no flags */
static void
isc_mem_noflags_test(void **state) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	char buf[4096], *p, *q;
	FILE *f;
	void *ptr;

	result = isc_stdio_open("mem.output", "w", &f);
	assert_int_equal(result, ISC_R_SUCCESS);

	UNUSED(state);

	result = isc_mem_createx(0, 0, default_memalloc, default_memfree,
				 NULL, &mctx2, 0);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_mem_debugging = 0;
	ptr = isc_mem_get(mctx2, 2048);
	assert_non_null(ptr);
	isc__mem_printactive(mctx2, f);
	isc_mem_put(mctx2, ptr, 2048);
	isc_mem_destroy(&mctx2);
	isc_stdio_close(f);

	memset(buf, 0, sizeof(buf));
	result = isc_stdio_open("mem.output", "r", &f);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = isc_stdio_read(buf, sizeof(buf), 1, f, NULL);
	assert_int_equal(result, ISC_R_EOF);
	isc_stdio_close(f);
	isc_file_remove("mem.output");

	buf[sizeof(buf) - 1] = 0;

	p = strchr(buf, '\n');
	assert_non_null(p);
	assert_in_range(p, 0, buf + sizeof(buf) - 3);
	p += 2;
	q = strchr(p, '\n');
	assert_non_null(q);
	*q = '\0';
	assert_string_equal(p, "None.");

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;
}

/* test mem with record flag */
static void
isc_mem_recordflag_test(void **state) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	char buf[4096], *p;
	FILE *f;
	void *ptr;

	result = isc_stdio_open("mem.output", "w", &f);
	assert_int_equal(result, ISC_R_SUCCESS);

	UNUSED(state);

	result = isc_mem_createx(0, 0, default_memalloc, default_memfree,
				 NULL, &mctx2, 0);
	assert_int_equal(result, ISC_R_SUCCESS);
	ptr = isc_mem_get(mctx2, 2048);
	assert_non_null(ptr);
	isc__mem_printactive(mctx2, f);
	isc_mem_put(mctx2, ptr, 2048);
	isc_mem_destroy(&mctx2);
	isc_stdio_close(f);

	memset(buf, 0, sizeof(buf));
	result = isc_stdio_open("mem.output", "r", &f);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = isc_stdio_read(buf, sizeof(buf), 1, f, NULL);
	assert_int_equal(result, ISC_R_EOF);
	isc_stdio_close(f);
	isc_file_remove("mem.output");

	buf[sizeof(buf) - 1] = 0;

	p = strchr(buf, '\n');
	assert_non_null(p);
	assert_in_range(p, 0, buf + sizeof(buf) - 3);
	assert_memory_equal(p + 2, "ptr ", 4);
	p = strchr(p + 1, '\n');
	assert_non_null(p);
	assert_int_equal(strlen(p), 1);

}

/* test mem with trace flag */
static void
isc_mem_traceflag_test(void **state) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	char buf[4096], *p;
	FILE *f;
	void *ptr;

	/* redirect stderr so we can check trace output */
	f = freopen("mem.output", "w", stderr);
	assert_non_null(f);

	UNUSED(state);

	result = isc_mem_createx(0, 0, default_memalloc, default_memfree,
				 NULL, &mctx2, 0);
	isc_mem_debugging = ISC_MEM_DEBUGTRACE;
	assert_int_equal(result, ISC_R_SUCCESS);
	ptr = isc_mem_get(mctx2, 2048);
	assert_non_null(ptr);
	isc__mem_printactive(mctx2, f);
	isc_mem_put(mctx2, ptr, 2048);
	isc_mem_destroy(&mctx2);
	isc_stdio_close(f);

	memset(buf, 0, sizeof(buf));
	result = isc_stdio_open("mem.output", "r", &f);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = isc_stdio_read(buf, sizeof(buf), 1, f, NULL);
	assert_int_equal(result, ISC_R_EOF);
	isc_stdio_close(f);
	isc_file_remove("mem.output");

	/* return stderr to TTY so we can see errors */
	f = freopen("/dev/tty", "w", stderr);

	buf[sizeof(buf) - 1] = 0;

	assert_memory_equal(buf, "add ", 4);
	p = strchr(buf, '\n');
	assert_non_null(p);
	p = strchr(p + 1, '\n');
	assert_non_null(p);
	assert_in_range(p, 0, buf + sizeof(buf) - 3);
	assert_memory_equal(p + 2, "ptr ", 4);
	p = strchr(p + 1, '\n');
	assert_non_null(p);
	assert_memory_equal(p + 1, "del ", 4);

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;
}
#endif

/*
 * Main
 */

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(isc_mem_test,
				_setup, _teardown),
		cmocka_unit_test_setup_teardown(isc_mem_total_test,
				_setup, _teardown),
		cmocka_unit_test_setup_teardown(isc_mem_inuse_test,
				_setup, _teardown),

#if ISC_MEM_TRACKLINES
		cmocka_unit_test_setup_teardown(isc_mem_noflags_test,
				_setup, _teardown),
		cmocka_unit_test_setup_teardown(isc_mem_recordflag_test,
				_setup, _teardown),
		cmocka_unit_test_setup_teardown(isc_mem_traceflag_test,
				_setup, _teardown),
#endif
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
