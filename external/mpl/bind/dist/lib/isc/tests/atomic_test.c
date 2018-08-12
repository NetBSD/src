/*	$NetBSD: atomic_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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

#include <atf-c.h>

#include <isc/atomic.h>
#include <isc/print.h>
#include <isc/result.h>

#include "isctest.h"

#define TASKS 32
#define ITERATIONS 1000
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

ATF_TC(atomic_xadd);
ATF_TC_HEAD(atomic_xadd, tc) {
	atf_tc_set_md_var(tc, "descr", "atomic XADD");
}
ATF_TC_BODY(atomic_xadd, tc) {
	isc_result_t result;
	isc_task_t *tasks[TASKS];
	isc_event_t *event = NULL;
	int i;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	memset(counters, 0, sizeof(counters));
	counter_32 = 0;

	/*
	 * Create our tasks, and allocate an event to get the counters going.
	 */
	for (i = 0 ; i < TASKS ; i++) {
		tasks[i] = NULL;
		ATF_REQUIRE_EQ(isc_task_create(taskmgr, 0, &tasks[i]),
			       ISC_R_SUCCESS);
		event = isc_event_allocate(mctx, NULL, 1000, do_xadd,
					   &counters[i],
					   sizeof(struct isc_event));
		ATF_REQUIRE(event != NULL);
		isc_task_sendanddetach(&tasks[i], &event);
	}

	isc_test_end();

	printf("32-bit counter %d, expected %d\n",
	       counter_32, EXPECTED_COUNT_32);

	ATF_CHECK_EQ(counter_32, EXPECTED_COUNT_32);
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

ATF_TC(atomic_xaddq);
ATF_TC_HEAD(atomic_xaddq, tc) {
	atf_tc_set_md_var(tc, "descr", "atomic XADDQ");
}
ATF_TC_BODY(atomic_xaddq, tc) {
	isc_result_t result;
	isc_task_t *tasks[TASKS];
	isc_event_t *event = NULL;
	int i;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	memset(counters, 0, sizeof(counters));
	counter_64 = 0;

	/*
	 * Create our tasks, and allocate an event to get the counters going.
	 */
	for (i = 0 ; i < TASKS ; i++) {
		tasks[i] = NULL;
		ATF_REQUIRE_EQ(isc_task_create(taskmgr, 0, &tasks[i]),
			       ISC_R_SUCCESS);
		event = isc_event_allocate(mctx, NULL, 1000, do_xaddq,
					   &counters[i],
					   sizeof(struct isc_event));
		ATF_REQUIRE(event != NULL);
		isc_task_sendanddetach(&tasks[i], &event);
	}

	isc_test_end();

	printf("64-bit counter %"ISC_PRINT_QUADFORMAT"d, "
	       "expected %"ISC_PRINT_QUADFORMAT"d\n",
	       counter_64, EXPECTED_COUNT_64);

	ATF_CHECK_EQ(counter_64, EXPECTED_COUNT_64);
	counter_32 = 0;
}
#endif

#if defined(ISC_PLATFORM_HAVEATOMICSTORE)
static isc_int32_t store_32;

static void
do_store(isc_task_t *task, isc_event_t *ev) {
	counter_t *state = (counter_t *)ev->ev_arg;
	int i;
	isc_uint32_t r;
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

ATF_TC(atomic_store);
ATF_TC_HEAD(atomic_store, tc) {
	atf_tc_set_md_var(tc, "descr", "atomic STORE");
}
ATF_TC_BODY(atomic_store, tc) {
	isc_result_t result;
	isc_task_t *tasks[TASKS];
	isc_event_t *event = NULL;
	isc_uint32_t val;
	isc_uint32_t r;
	int i;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	memset(counters, 0, sizeof(counters));
	store_32 = 0;

	/*
	 * Create our tasks, and allocate an event to get the counters
	 * going.
	 */
	for (i = 0 ; i < TASKS ; i++) {
		tasks[i] = NULL;
		ATF_REQUIRE_EQ(isc_task_create(taskmgr, 0, &tasks[i]),
			       ISC_R_SUCCESS);
		event = isc_event_allocate(mctx, NULL, 1000, do_store,
					   &counters[i],
					   sizeof(struct isc_event));
		ATF_REQUIRE(event != NULL);
		isc_task_sendanddetach(&tasks[i], &event);
	}

	isc_test_end();

	r = store_32 & 0xff;
	val = (r << 24) | (r << 16) | (r << 8) | r;

	printf("32-bit store 0x%x, expected 0x%x\n",
	       (isc_uint32_t) store_32, val);

	ATF_CHECK_EQ((isc_uint32_t) store_32, val);
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

ATF_TC(atomic_storeq);
ATF_TC_HEAD(atomic_storeq, tc) {
	atf_tc_set_md_var(tc, "descr", "atomic STOREQ");
}
ATF_TC_BODY(atomic_storeq, tc) {
	isc_result_t result;
	isc_task_t *tasks[TASKS];
	isc_event_t *event = NULL;
	isc_uint64_t val;
	isc_uint32_t r;
	int i;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	memset(counters, 0, sizeof(counters));
	store_64 = 0;

	/*
	 * Create our tasks, and allocate an event to get the counters
	 * going.
	 */
	for (i = 0 ; i < TASKS ; i++) {
		tasks[i] = NULL;
		ATF_REQUIRE_EQ(isc_task_create(taskmgr, 0, &tasks[i]),
			       ISC_R_SUCCESS);
		event = isc_event_allocate(mctx, NULL, 1000, do_storeq,
					   &counters[i],
					   sizeof(struct isc_event));
		ATF_REQUIRE(event != NULL);
		isc_task_sendanddetach(&tasks[i], &event);
	}

	isc_test_end();

	r = store_64 & 0xff;
	val = (((isc_uint64_t) r << 24) |
	       ((isc_uint64_t) r << 16) |
	       ((isc_uint64_t) r << 8) |
	       (isc_uint64_t) r);
	val |= ((isc_uint64_t) val << 32);

	printf("64-bit store 0x%"ISC_PRINT_QUADFORMAT"x, "
	       "expected 0x%"ISC_PRINT_QUADFORMAT"x\n",
	       (isc_uint64_t) store_64, val);

	ATF_CHECK_EQ((isc_uint64_t) store_64, val);
	store_64 = 0;
}
#endif

#if !defined(ISC_PLATFORM_HAVEXADD) && \
    !defined(ISC_PLATFORM_HAVEXADDQ) && \
    !defined(ISC_PLATFORM_HAVEATOMICSTOREQ)
ATF_TC(untested);
ATF_TC_HEAD(untested, tc) {
	atf_tc_set_md_var(tc, "descr", "skipping aes test");
}
ATF_TC_BODY(untested, tc) {
	UNUSED(tc);
	atf_tc_skip("AES not available");
}
#endif /* !HAVEXADD, !HAVEXADDQ, !HAVEATOMICSTOREQ */

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
#if defined(ISC_PLATFORM_HAVEXADD)
	ATF_TP_ADD_TC(tp, atomic_xadd);
#endif
#if defined(ISC_PLATFORM_HAVEXADDQ)
	ATF_TP_ADD_TC(tp, atomic_xaddq);
#endif
#ifdef ISC_PLATFORM_HAVEATOMICSTORE
	ATF_TP_ADD_TC(tp, atomic_store);
#endif
#if defined(ISC_PLATFORM_HAVEATOMICSTOREQ)
	ATF_TP_ADD_TC(tp, atomic_storeq);
#endif
#if !defined(ISC_PLATFORM_HAVEXADD) && \
    !defined(ISC_PLATFORM_HAVEXADDQ) && \
    !defined(ISC_PLATFORM_HAVEATOMICSTOREQ)
	ATF_TP_ADD_TC(tp, untested);
#endif

	return (atf_no_error());
}
