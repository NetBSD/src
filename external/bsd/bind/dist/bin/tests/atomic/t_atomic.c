/*	$NetBSD: t_atomic.c,v 1.4.4.1 2016/10/14 12:01:13 martin Exp $	*/

/*
 * Copyright (C) 2011, 2013, 2015  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <config.h>

#include <ctype.h>
#include <stdlib.h>

#include <isc/atomic.h>
#include <isc/mem.h>
#include <isc/util.h>
#include <isc/string.h>
#include <isc/print.h>
#include <isc/event.h>
#include <isc/task.h>

#include <tests/t_api.h>

char *progname;

#define CHECK(x) RUNTIME_CHECK(ISC_R_SUCCESS == (x))

isc_mem_t *mctx = NULL;
isc_taskmgr_t *task_manager = NULL;

#if defined(ISC_PLATFORM_HAVEXADD) || defined(ISC_PLATFORM_HAVEXADDQ) || \
    defined(ISC_PLATFORM_HAVEATOMICSTORE) || \
    defined(ISC_PLATFORM_HAVEATOMICSTOREQ)
static void
setup(void) {
	/* 1 */ CHECK(isc_mem_create(0, 0, &mctx));
	/* 2 */ CHECK(isc_taskmgr_create(mctx, 32, 0, &task_manager));
}

static void
teardown(void) {
	/* 2 */ isc_taskmgr_destroy(&task_manager);
	/* 1 */ isc_mem_destroy(&mctx);
}
#endif

#define TASKS 32
#define ITERATIONS 10000
#define COUNTS_PER_ITERATION 1000
#define INCREMENT_64 (isc_int64_t)0x0000000010000000
#define EXPECTED_COUNT_32 (TASKS * ITERATIONS * COUNTS_PER_ITERATION)
#define EXPECTED_COUNT_64 (TASKS * ITERATIONS * COUNTS_PER_ITERATION * INCREMENT_64)

typedef struct {
	isc_uint32_t iteration;
} counter_t;

counter_t counters[TASKS];

#if defined(ISC_PLATFORM_HAVEXADD)
static isc_int32_t counter_32;

static void
do_xadd(isc_task_t *task, isc_event_t *ev) {
	counter_t *state = (counter_t *)ev->ev_arg;
	int i;

	for (i = 0 ; i < COUNTS_PER_ITERATION ; i++) {
		isc_atomic_xadd(&counter_32, 1);
	}

	state->iteration++;
	if (state->iteration < ITERATIONS) {
		isc_task_send(task, &ev);
	} else {
		isc_event_free(&ev);
	}
}

static void
test_atomic_xadd() {
	int test_result;
	isc_task_t *tasks[TASKS];
	isc_event_t *event;
	int i;

	t_assert("test_atomic_xadd", 1, T_REQUIRED, "%s",
		 "ensure that isc_atomic_xadd() works.");

	setup();

	memset(counters, 0, sizeof(counters));
	counter_32 = 0;

	/*
	 * Create our tasks, and allocate an event to get the counters going.
	 */
	for (i = 0 ; i < TASKS ; i++) {
		tasks[i] = NULL;
		CHECK(isc_task_create(task_manager, 0, &tasks[i]));
		event = isc_event_allocate(mctx, NULL, 1000, do_xadd,
					   &counters[i], sizeof(struct isc_event));
		isc_task_sendanddetach(&tasks[i], &event);
	}

	teardown();

	test_result = T_PASS;
	t_info("32-bit counter %d, expected %d\n", counter_32, EXPECTED_COUNT_32);
	if (counter_32 != EXPECTED_COUNT_32)
		test_result = T_FAIL;
	t_result(test_result);

	counter_32 = 0;
}
#endif

#if defined(ISC_PLATFORM_HAVEXADDQ)
static isc_int64_t counter_64;

static void
do_xaddq(isc_task_t *task, isc_event_t *ev) {
	counter_t *state = (counter_t *)ev->ev_arg;
	int i;

	for (i = 0 ; i < COUNTS_PER_ITERATION ; i++) {
		isc_atomic_xaddq(&counter_64, INCREMENT_64);
	}

	state->iteration++;
	if (state->iteration < ITERATIONS) {
		isc_task_send(task, &ev);
	} else {
		isc_event_free(&ev);
	}
}

static void
test_atomic_xaddq() {
	int test_result;
	isc_task_t *tasks[TASKS];
	isc_event_t *event;
	int i;

	t_assert("test_atomic_xaddq", 1, T_REQUIRED, "%s",
		 "ensure that isc_atomic_xaddq() works.");

	setup();

	memset(counters, 0, sizeof(counters));
	counter_64 = 0;

	/*
	 * Create our tasks, and allocate an event to get the counters going.
	 */
	for (i = 0 ; i < TASKS ; i++) {
		tasks[i] = NULL;
		CHECK(isc_task_create(task_manager, 0, &tasks[i]));
		event = isc_event_allocate(mctx, NULL, 1000, do_xaddq,
					   &counters[i], sizeof(struct isc_event));
		isc_task_sendanddetach(&tasks[i], &event);
	}

	teardown();

	test_result = T_PASS;
	t_info("64-bit counter %"ISC_PRINT_QUADFORMAT"d, expected %"ISC_PRINT_QUADFORMAT"d\n",
	       counter_64, EXPECTED_COUNT_64);
	if (counter_64 != EXPECTED_COUNT_64)
		test_result = T_FAIL;
	t_result(test_result);

	counter_64 = 0;
}
#endif

#ifdef ISC_PLATFORM_HAVEATOMICSTORE
static isc_int32_t store_32;

static void
do_store(isc_task_t *task, isc_event_t *ev) {
	counter_t *state = (counter_t *)ev->ev_arg;
	int i;
	isc_uint8_t r;
	isc_uint32_t val;

	r = random() % 256;
	val = (r << 24) | (r << 16) | (r << 8) | r;

	for (i = 0 ; i < COUNTS_PER_ITERATION ; i++) {
		isc_atomic_store(&store_32, val);
	}

	state->iteration++;
	if (state->iteration < ITERATIONS) {
		isc_task_send(task, &ev);
	} else {
		isc_event_free(&ev);
	}
}

static void
test_atomic_store() {
	int test_result;
	isc_task_t *tasks[TASKS];
	isc_event_t *event;
	int i;
	isc_uint8_t r;
	isc_uint32_t val;

	t_assert("test_atomic_store", 1, T_REQUIRED, "%s",
		 "ensure that isc_atomic_store() works.");

	setup();

	memset(counters, 0, sizeof(counters));
	store_32 = 0;

	/*
	 * Create our tasks, and allocate an event to get the counters
	 * going.
	 */
	for (i = 0 ; i < TASKS ; i++) {
		tasks[i] = NULL;
		CHECK(isc_task_create(task_manager, 0, &tasks[i]));
		event = isc_event_allocate(mctx, NULL, 1000, do_store,
					   &counters[i],
					   sizeof(struct isc_event));
		isc_task_sendanddetach(&tasks[i], &event);
	}

	teardown();

	test_result = T_PASS;
	r = store_32 & 0xff;
	val = (r << 24) | (r << 16) | (r << 8) | r;
	t_info("32-bit store 0x%x, expected 0x%x\n",
	       (isc_uint32_t) store_32, val);
	if ((isc_uint32_t) store_32 != val)
		test_result = T_FAIL;
	t_result(test_result);

	store_32 = 0;
}
#endif

#if defined(ISC_PLATFORM_HAVEATOMICSTOREQ)
static isc_int64_t store_64;

static void
do_storeq(isc_task_t *task, isc_event_t *ev) {
	counter_t *state = (counter_t *)ev->ev_arg;
	int i;
	isc_uint8_t r;
	isc_uint64_t val;

	r = random() % 256;
	val = (((isc_uint64_t) r << 24) |
	       ((isc_uint64_t) r << 16) |
	       ((isc_uint64_t) r << 8) |
	       (isc_uint64_t) r);
	val |= ((isc_uint64_t) val << 32);

	for (i = 0 ; i < COUNTS_PER_ITERATION ; i++) {
		isc_atomic_storeq(&store_64, val);
	}

	state->iteration++;
	if (state->iteration < ITERATIONS) {
		isc_task_send(task, &ev);
	} else {
		isc_event_free(&ev);
	}
}

static void
test_atomic_storeq() {
	int test_result;
	isc_task_t *tasks[TASKS];
	isc_event_t *event;
	int i;
	isc_uint8_t r;
	isc_uint64_t val;

	t_assert("test_atomic_storeq", 1, T_REQUIRED, "%s",
		 "ensure that isc_atomic_storeq() works.");

	setup();

	memset(counters, 0, sizeof(counters));
	store_64 = 0;

	/*
	 * Create our tasks, and allocate an event to get the counters going.
	 */
	for (i = 0 ; i < TASKS ; i++) {
		tasks[i] = NULL;
		CHECK(isc_task_create(task_manager, 0, &tasks[i]));
		event = isc_event_allocate(mctx, NULL, 1000, do_storeq,
					   &counters[i], sizeof(struct isc_event));
		isc_task_sendanddetach(&tasks[i], &event);
	}

	teardown();

	test_result = T_PASS;
	r = store_64 & 0xff;
	val = (((isc_uint64_t) r << 24) |
	       ((isc_uint64_t) r << 16) |
	       ((isc_uint64_t) r << 8) |
	       (isc_uint64_t) r);
	val |= ((isc_uint64_t) val << 32);
	t_info("64-bit store 0x%"ISC_PRINT_QUADFORMAT"x, expected 0x%"ISC_PRINT_QUADFORMAT"x\n",
	       (isc_uint64_t) store_64, val);
	if ((isc_uint64_t) store_64 != val)
		test_result = T_FAIL;
	t_result(test_result);

	store_64 = 0;
}
#endif /* ISC_PLATFORM_HAVEATOMICSTOREQ */

testspec_t T_testlist[] = {
#if defined(ISC_PLATFORM_HAVEXADD)
	{ (PFV) test_atomic_xadd,	"test_atomic_xadd"		},
#endif
#if defined(ISC_PLATFORM_HAVEXADDQ)
	{ (PFV) test_atomic_xaddq,	"test_atomic_xaddq"		},
#endif
#ifdef ISC_PLATFORM_HAVEATOMICSTORE
	{ (PFV) test_atomic_store,	"test_atomic_store"		},
#endif
#if defined(ISC_PLATFORM_HAVEXADDQ)
	{ (PFV) test_atomic_storeq,	"test_atomic_storeq"		},
#endif
	{ (PFV) 0,			NULL }
};

#ifdef WIN32
int
main(int argc, char **argv) {
	t_settests(T_testlist);
	return (t_main(argc, argv));
}
#endif
