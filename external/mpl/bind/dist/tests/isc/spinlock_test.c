/*	$NetBSD: spinlock_test.c,v 1.2 2025/01/26 16:25:50 christos Exp $	*/

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

#ifdef HAVE_PTHREAD_SPIN_INIT
#define HAD_PTHREAD_SPIN_INIT 1
#undef HAVE_PTHREAD_SPIN_INIT
#endif

#include <isc/atomic.h>
#include <isc/file.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/pause.h>
#include <isc/result.h>
#include <isc/spinlock.h>
#include <isc/stdio.h>
#include <isc/thread.h>
#include <isc/time.h>
#include <isc/util.h>

#include <tests/isc.h>

static unsigned int loops = 100;
static unsigned int delay_loop = 1;

static int
setup_env(void **unused __attribute__((__unused__))) {
	char *env = getenv("ISC_BENCHMARK_LOOPS");
	if (env != NULL) {
		loops = atoi(env);
	}
	assert_int_not_equal(loops, 0);

	env = getenv("ISC_BENCHMARK_DELAY");
	if (env != NULL) {
		delay_loop = atoi(env);
	}
	assert_int_not_equal(delay_loop, 0);

	return 0;
}

ISC_RUN_TEST_IMPL(isc_spinlock) {
	isc_spinlock_t lock;

	isc_spinlock_init(&lock);

	for (size_t i = 0; i < loops; i++) {
		isc_spinlock_lock(&lock);
		isc_pause_n(delay_loop);
		isc_spinlock_unlock(&lock);
	}

	isc_spinlock_destroy(&lock);
}

#define ITERS 20

#define DC	200
#define CNT_MIN 800
#define CNT_MAX 1600

static size_t shared_counter = 0;
static size_t expected_counter = SIZE_MAX;

#if HAD_PTHREAD_SPIN_INIT
static pthread_spinlock_t spin;

static void *
pthread_spin_thread(void *arg) {
	size_t cont = *(size_t *)arg;

	for (size_t i = 0; i < loops; i++) {
		pthread_spin_lock(&spin);
		size_t v = shared_counter;
		isc_pause_n(delay_loop);
		shared_counter = v + 1;
		pthread_spin_unlock(&spin);
		isc_pause_n(cont);
	}

	return NULL;
}
#endif

static isc_spinlock_t lock;

static void *
isc_spinlock_thread(void *arg) {
	size_t cont = *(size_t *)arg;

	for (size_t i = 0; i < loops; i++) {
		isc_spinlock_lock(&lock);
		size_t v = shared_counter;
		isc_pause_n(delay_loop);
		shared_counter = v + 1;
		isc_spinlock_unlock(&lock);
		isc_pause_n(cont);
	}

	return NULL;
}

ISC_RUN_TEST_IMPL(isc_spinlock_benchmark) {
	isc_thread_t *threads = isc_mem_cget(mctx, workers, sizeof(*threads));
	isc_time_t ts1, ts2;
	double t;
	int dc;
	size_t cont;

	memset(threads, 0, sizeof(*threads) * workers);

	expected_counter = ITERS * workers * loops *
			   ((CNT_MAX - CNT_MIN) / DC + 1);

	/* PTHREAD SPINLOCK */

#if HAD_PTHREAD_SPIN_INIT
	int r = pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);
	assert_int_not_equal(r, -1);

	ts1 = isc_time_now_hires();

	shared_counter = 0;
	dc = DC;
	for (size_t l = 0; l < ITERS; l++) {
		for (cont = (dc > 0) ? CNT_MIN : CNT_MAX;
		     cont <= CNT_MAX && cont >= CNT_MIN; cont += dc)
		{
			for (size_t i = 0; i < workers; i++) {
				isc_thread_create(pthread_spin_thread, &cont,
						  &threads[i]);
			}
			for (size_t i = 0; i < workers; i++) {
				isc_thread_join(threads[i], NULL);
			}
		}
		dc = -dc;
	}
	assert_int_equal(shared_counter, expected_counter);

	ts2 = isc_time_now_hires();

	t = isc_time_microdiff(&ts2, &ts1);

	printf("[ TIME     ] isc_spinlock_benchmark: %zu pthread_spin "
	       "loops in "
	       "%u threads, %2.3f seconds, %2.3f calls/second\n",
	       shared_counter, workers, t / 1000000.0,
	       shared_counter / (t / 1000000.0));

	r = pthread_spin_destroy(&spin);
	assert_int_not_equal(r, -1);
#endif

	/* ISC SPINLOCK */

	isc_spinlock_init(&lock);

	ts1 = isc_time_now_hires();

	dc = DC;
	shared_counter = 0;
	for (size_t l = 0; l < ITERS; l++) {
		for (cont = (dc > 0) ? CNT_MIN : CNT_MAX;
		     cont <= CNT_MAX && cont >= CNT_MIN; cont += dc)
		{
			for (size_t i = 0; i < workers; i++) {
				isc_thread_create(isc_spinlock_thread, &cont,
						  &threads[i]);
			}
			for (size_t i = 0; i < workers; i++) {
				isc_thread_join(threads[i], NULL);
			}
		}
		dc = -dc;
	}
	assert_int_equal(shared_counter, expected_counter);

	ts2 = isc_time_now_hires();

	t = isc_time_microdiff(&ts2, &ts1);

	printf("[ TIME     ] isc_spinlock_benchmark: %zu isc_spinlock loops "
	       "in %u "
	       "threads, %2.3f seconds, %2.3f calls/second\n",
	       shared_counter, workers, t / 1000000.0,
	       shared_counter / (t / 1000000.0));

	isc_spinlock_destroy(&lock);

	isc_mem_cput(mctx, threads, workers, sizeof(*threads));
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_spinlock)
#if !defined(__SANITIZE_THREAD__)
ISC_TEST_ENTRY(isc_spinlock_benchmark)
#endif /* __SANITIZE_THREAD__ */

ISC_TEST_LIST_END

ISC_TEST_MAIN_CUSTOM(setup_env, NULL)
