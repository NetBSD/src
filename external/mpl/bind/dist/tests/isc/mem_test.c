/*	$NetBSD: mem_test.c,v 1.3 2025/01/26 16:25:49 christos Exp $	*/

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

#include <fcntl.h>
#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/atomic.h>
#include <isc/file.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/os.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/stdio.h>
#include <isc/thread.h>
#include <isc/time.h>
#include <isc/util.h>

#include "mem_p.h"

#include <tests/isc.h>

#define MP1_FREEMAX  10
#define MP1_FILLCNT  10
#define MP1_MAXALLOC 30

#define MP2_FREEMAX 25
#define MP2_FILLCNT 25

/* general memory system tests */
ISC_RUN_TEST_IMPL(isc_mem_get) {
	void *items1[50];
	void *items2[50];
	void *tmp;
	isc_mempool_t *mp1 = NULL, *mp2 = NULL;
	unsigned int i, j;
	int rval;

	isc_mempool_create(mctx, 24, &mp1);
	isc_mempool_create(mctx, 31, &mp2);

	isc_mempool_setfreemax(mp1, MP1_FREEMAX);
	isc_mempool_setfillcount(mp1, MP1_FILLCNT);

	/*
	 * Allocate MP1_MAXALLOC items from the pool.  This is our max.
	 */
	for (i = 0; i < MP1_MAXALLOC; i++) {
		items1[i] = isc_mempool_get(mp1);
		assert_non_null(items1[i]);
	}

	/*
	 * Free the first 11 items.  Verify that there are 10 free items on
	 * the free list (which is our max).
	 */
	for (i = 0; i < 11; i++) {
		isc_mempool_put(mp1, items1[i]);
		items1[i] = NULL;
	}

#if !__SANITIZE_ADDRESS__
	rval = isc_mempool_getfreecount(mp1);
	assert_int_equal(rval, 10);
#endif /* !__SANITIZE_ADDRESS__ */

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

	isc_mempool_create(mctx, 2, &mp1);

	tmp = isc_mempool_get(mp1);
	assert_non_null(tmp);

	isc_mempool_put(mp1, tmp);

	isc_mempool_destroy(&mp1);
}

/* zeroed memory system tests */
ISC_RUN_TEST_IMPL(isc_mem_cget_zero) {
	uint8_t *ptr;
	bool zeroed;
	uint8_t expected[4096] = { 0 };

	/* Skip the test if the memory is zeroed even in normal case */
	zeroed = true;
	ptr = isc_mem_get(mctx, sizeof(expected));
	for (size_t i = 0; i < sizeof(expected); i++) {
		if (ptr[i] != expected[i]) {
			zeroed = false;
			break;
		}
	}
	isc_mem_put(mctx, ptr, sizeof(expected));
	if (zeroed) {
		skip();
		return;
	}

	ptr = isc_mem_cget(mctx, 1, sizeof(expected));
	assert_memory_equal(ptr, expected, sizeof(expected));
	isc_mem_put(mctx, ptr, sizeof(expected));
}

ISC_RUN_TEST_IMPL(isc_mem_callocate_zero) {
	uint8_t *ptr;
	bool zeroed;
	uint8_t expected[4096] = { 0 };

	/* Skip the test if the memory is zeroed even in normal case */
	zeroed = true;
	ptr = isc_mem_get(mctx, sizeof(expected));
	for (size_t i = 0; i < sizeof(expected); i++) {
		if (ptr[i] != expected[i]) {
			zeroed = false;
			break;
		}
	}
	isc_mem_put(mctx, ptr, sizeof(expected));
	if (zeroed) {
		skip();
		return;
	}

	ptr = isc_mem_callocate(mctx, 1, sizeof(expected));
	assert_memory_equal(ptr, expected, sizeof(expected));
	isc_mem_free(mctx, ptr);
}

/* test InUse calculation */
ISC_RUN_TEST_IMPL(isc_mem_inuse) {
	isc_mem_t *mctx2 = NULL;
	size_t before, after;
	ssize_t diff;
	void *ptr;

	mctx2 = NULL;
	isc_mem_create(&mctx2);

	before = isc_mem_inuse(mctx2);
	ptr = isc_mem_allocate(mctx2, 1024000);
	isc_mem_free(mctx2, ptr);
	after = isc_mem_inuse(mctx2);

	diff = after - before;

	assert_int_equal(diff, 0);

	isc_mem_destroy(&mctx2);
}

ISC_RUN_TEST_IMPL(isc_mem_zeroget) {
	uint8_t *data = NULL;

	data = isc_mem_get(mctx, 0);
	assert_non_null(data);
	isc_mem_put(mctx, data, 0);
}

#define REGET_INIT_SIZE	  1024
#define REGET_GROW_SIZE	  2048
#define REGET_SHRINK_SIZE 512

ISC_RUN_TEST_IMPL(isc_mem_reget) {
	uint8_t *data = NULL;

	/* test that we can reget NULL */
	data = isc_mem_reget(mctx, NULL, 0, REGET_INIT_SIZE);
	assert_non_null(data);
	isc_mem_put(mctx, data, REGET_INIT_SIZE);

	/* test that we can re-get a zero-length allocation */
	data = isc_mem_get(mctx, 0);
	assert_non_null(data);

	data = isc_mem_reget(mctx, data, 0, REGET_INIT_SIZE);
	assert_non_null(data);

	for (size_t i = 0; i < REGET_INIT_SIZE; i++) {
		data[i] = i % UINT8_MAX;
	}

	data = isc_mem_reget(mctx, data, REGET_INIT_SIZE, REGET_GROW_SIZE);
	assert_non_null(data);

	for (size_t i = 0; i < REGET_INIT_SIZE; i++) {
		assert_int_equal(data[i], i % UINT8_MAX);
	}

	for (size_t i = REGET_GROW_SIZE; i > 0; i--) {
		data[i - 1] = i % UINT8_MAX;
	}

	data = isc_mem_reget(mctx, data, REGET_GROW_SIZE, REGET_SHRINK_SIZE);
	assert_non_null(data);

	for (size_t i = REGET_SHRINK_SIZE; i > 0; i--) {
		assert_int_equal(data[i - 1], i % UINT8_MAX);
	}

	isc_mem_put(mctx, data, REGET_SHRINK_SIZE);
}

ISC_RUN_TEST_IMPL(isc_mem_reallocate) {
	uint8_t *data = NULL;

	/* test that we can reallocate NULL */
	data = isc_mem_reallocate(mctx, NULL, REGET_INIT_SIZE);
	assert_non_null(data);
	isc_mem_free(mctx, data);

	/* test that we can re-get a zero-length allocation */
	data = isc_mem_allocate(mctx, 0);
	assert_non_null(data);

	data = isc_mem_reallocate(mctx, data, REGET_INIT_SIZE);
	assert_non_null(data);

	for (size_t i = 0; i < REGET_INIT_SIZE; i++) {
		data[i] = i % UINT8_MAX;
	}

	data = isc_mem_reallocate(mctx, data, REGET_GROW_SIZE);
	assert_non_null(data);

	for (size_t i = 0; i < REGET_INIT_SIZE; i++) {
		assert_int_equal(data[i], i % UINT8_MAX);
	}

	for (size_t i = REGET_GROW_SIZE; i > 0; i--) {
		data[i - 1] = i % UINT8_MAX;
	}

	data = isc_mem_reallocate(mctx, data, REGET_SHRINK_SIZE);
	assert_non_null(data);

	for (size_t i = REGET_SHRINK_SIZE; i > 0; i--) {
		assert_int_equal(data[i - 1], i % UINT8_MAX);
	}

	isc_mem_free(mctx, data);
}

ISC_RUN_TEST_IMPL(isc_mem_overmem) {
	isc_mem_t *omctx = NULL;
	isc_mem_create(&omctx);
	assert_non_null(omctx);

	isc_mem_setwater(omctx, 1024, 512);

	/* inuse < lo_water */
	void *data1 = isc_mem_allocate(omctx, 256);
	assert_false(isc_mem_isovermem(omctx));

	/* lo_water < inuse < hi_water */
	void *data2 = isc_mem_allocate(omctx, 512);
	assert_false(isc_mem_isovermem(omctx));

	/* hi_water < inuse */
	void *data3 = isc_mem_allocate(omctx, 512);
	assert_true(isc_mem_isovermem(omctx));

	/* lo_water < inuse < hi_water */
	isc_mem_free(omctx, data2);
	assert_true(isc_mem_isovermem(omctx));

	/* inuse < lo_water */
	isc_mem_free(omctx, data3);
	assert_false(isc_mem_isovermem(omctx));

	/* inuse == 0 */
	isc_mem_free(omctx, data1);
	assert_false(isc_mem_isovermem(omctx));

	isc_mem_destroy(&omctx);
}

#if ISC_MEM_TRACKLINES

/* test mem with no flags */
ISC_RUN_TEST_IMPL(isc_mem_noflags) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	char buf[4096], *p;
	FILE *f;
	void *ptr;

	result = isc_stdio_open("mem.output", "w", &f);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_mem_debugging = 0;
	isc_mem_create(&mctx2);
	ptr = isc_mem_get(mctx2, 2048);
	assert_non_null(ptr);
	isc__mem_printactive(mctx2, f);
	isc_mem_put(mctx2, ptr, 2048);
	isc_mem_destroy(&mctx2);
	isc_mem_debugging = ISC_MEM_DEBUGRECORD;
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
	assert_null(p);
}

/* test mem with record flag */
ISC_RUN_TEST_IMPL(isc_mem_recordflag) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	char buf[4096], *p;
	FILE *f;
	void *ptr;

	result = isc_stdio_open("mem.output", "w", &f);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_mem_create(&mctx2);
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
ISC_RUN_TEST_IMPL(isc_mem_traceflag) {
	isc_result_t result;
	isc_mem_t *mctx2 = NULL;
	char buf[4096], *p;
	FILE *f;
	void *ptr;

	/* redirect stderr so we can check trace output */
	f = freopen("mem.output", "w", stderr);
	assert_non_null(f);

	isc_mem_debugging = ISC_MEM_DEBUGRECORD | ISC_MEM_DEBUGTRACE;
	isc_mem_create(&mctx2);
	ptr = isc_mem_get(mctx2, 2048);
	assert_non_null(ptr);
	isc__mem_printactive(mctx2, f);
	isc_mem_put(mctx2, ptr, 2048);
	isc_mem_destroy(&mctx2);
	isc_mem_debugging = ISC_MEM_DEBUGRECORD;
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

	assert_memory_equal(buf, "create ", 6);
	p = strchr(buf, '\n');
	assert_non_null(p);

	assert_memory_equal(p + 1, "add ", 4);
	p = strchr(p + 1, '\n');
	assert_non_null(p);
	p = strchr(p + 1, '\n');
	assert_non_null(p);
	assert_in_range(p, 0, buf + sizeof(buf) - 3);
	assert_memory_equal(p + 2, "ptr ", 4);
	p = strchr(p + 1, '\n');
	assert_non_null(p);
	assert_memory_equal(p + 1, "del ", 4);
}
#endif /* if ISC_MEM_TRACKLINES */

#if !defined(__SANITIZE_THREAD__)

#define ITERS	  512
#define NUM_ITEMS 1024 /* 768 */
#define ITEM_SIZE 65534

static atomic_size_t mem_size;

static void *
mem_thread(void *arg) {
	isc_mem_t *mctx2 = (isc_mem_t *)arg;
	void *items[NUM_ITEMS];
	size_t size = atomic_load(&mem_size);
	while (!atomic_compare_exchange_weak(&mem_size, &size, size / 2)) {
		;
	}

	for (int i = 0; i < ITERS; i++) {
		for (int j = 0; j < NUM_ITEMS; j++) {
			items[j] = isc_mem_get(mctx2, size);
		}
		for (int j = 0; j < NUM_ITEMS; j++) {
			isc_mem_put(mctx2, items[j], size);
		}
	}

	return NULL;
}

ISC_RUN_TEST_IMPL(isc_mem_benchmark) {
	int nthreads = ISC_MAX(ISC_MIN(isc_os_ncpus(), 32), 1);
	isc_thread_t threads[32];
	isc_time_t ts1, ts2;
	double t;

	atomic_init(&mem_size, ITEM_SIZE);

	ts1 = isc_time_now();

	for (int i = 0; i < nthreads; i++) {
		isc_thread_create(mem_thread, mctx, &threads[i]);
	}
	for (int i = 0; i < nthreads; i++) {
		isc_thread_join(threads[i], NULL);
	}

	ts2 = isc_time_now();

	t = isc_time_microdiff(&ts2, &ts1);

	printf("[ TIME     ] isc_mem_benchmark: "
	       "%d isc_mem_{get,put} calls, %f seconds, %f "
	       "calls/second\n",
	       nthreads * ITERS * NUM_ITEMS, t / 1000000.0,
	       (nthreads * ITERS * NUM_ITEMS) / (t / 1000000.0));
}

#endif /* __SANITIZE_THREAD */

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_mem_get)
ISC_TEST_ENTRY(isc_mem_cget_zero)
ISC_TEST_ENTRY(isc_mem_callocate_zero)
ISC_TEST_ENTRY(isc_mem_inuse)
ISC_TEST_ENTRY(isc_mem_zeroget)
ISC_TEST_ENTRY(isc_mem_reget)
ISC_TEST_ENTRY(isc_mem_reallocate)
ISC_TEST_ENTRY(isc_mem_overmem)

#if ISC_MEM_TRACKLINES
ISC_TEST_ENTRY(isc_mem_noflags)
ISC_TEST_ENTRY(isc_mem_recordflag)
/*
 * traceflag_test closes stderr, which causes weird
 * side effects for any next test trying to use libuv.
 * This test has to be the last one to avoid problems.
 */
ISC_TEST_ENTRY(isc_mem_traceflag)
#endif /* if ISC_MEM_TRACKLINES */
#if !defined(__SANITIZE_THREAD__)
ISC_TEST_ENTRY(isc_mem_benchmark)
#endif /* __SANITIZE_THREAD__ */

ISC_TEST_LIST_END

ISC_TEST_MAIN
