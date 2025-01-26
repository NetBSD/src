/*	$NetBSD: mutex_test.c,v 1.2 2025/01/26 16:25:49 christos Exp $	*/

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
#include <isc/pause.h>
#include <isc/result.h>
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

ISC_RUN_TEST_IMPL(isc_mutex) {
	isc_mutex_t lock;

	isc_mutex_init(&lock);

	for (size_t i = 0; i < loops; i++) {
		isc_mutex_lock(&lock);
		isc_pause_n(delay_loop);
		isc_mutex_unlock(&lock);
	}

	isc_mutex_destroy(&lock);
}

#define ITERS 20

#define DC	200
#define CNT_MIN 800
#define CNT_MAX 1600

static size_t shared_counter = 0;
static size_t expected_counter = SIZE_MAX;
static isc_mutex_t lock;
static pthread_mutex_t mutex;

static void *
pthread_mutex_thread(void *arg) {
	size_t cont = *(size_t *)arg;

	for (size_t i = 0; i < loops; i++) {
		pthread_mutex_lock(&mutex);
		size_t v = shared_counter;
		isc_pause_n(delay_loop);
		shared_counter = v + 1;
		pthread_mutex_unlock(&mutex);
		isc_pause_n(cont);
	}

	return NULL;
}

static void *
isc_mutex_thread(void *arg) {
	size_t cont = *(size_t *)arg;

	for (size_t i = 0; i < loops; i++) {
		isc_mutex_lock(&lock);
		size_t v = shared_counter;
		isc_pause_n(delay_loop);
		shared_counter = v + 1;
		isc_mutex_unlock(&lock);
		isc_pause_n(cont);
	}

	return NULL;
}

ISC_RUN_TEST_IMPL(isc_mutex_benchmark) {
	isc_thread_t *threads = isc_mem_cget(mctx, workers, sizeof(*threads));
	isc_time_t ts1, ts2;
	double t;
	int dc;
	size_t cont;
	int r;

	memset(threads, 0, sizeof(*threads) * workers);

	expected_counter = ITERS * workers * loops *
			   ((CNT_MAX - CNT_MIN) / DC + 1);

	/* PTHREAD MUTEX */

	r = pthread_mutex_init(&mutex, NULL);
	assert_int_not_equal(r, -1);

	ts1 = isc_time_now_hires();

	shared_counter = 0;
	dc = DC;
	for (size_t l = 0; l < ITERS; l++) {
		for (cont = (dc > 0) ? CNT_MIN : CNT_MAX;
		     cont <= CNT_MAX && cont >= CNT_MIN; cont += dc)
		{
			for (size_t i = 0; i < workers; i++) {
				isc_thread_create(pthread_mutex_thread, &cont,
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

	printf("[ TIME     ] isc_mutex_benchmark: %zu pthread_mutex "
	       "loops in "
	       "%u threads, %2.3f seconds, %2.3f calls/second\n",
	       shared_counter, workers, t / 1000000.0,
	       shared_counter / (t / 1000000.0));

	r = pthread_mutex_destroy(&mutex);
	assert_int_not_equal(r, -1);

	/* ISC MUTEX */

	isc_mutex_init(&lock);

	ts1 = isc_time_now_hires();

	dc = DC;
	shared_counter = 0;
	for (size_t l = 0; l < ITERS; l++) {
		for (cont = (dc > 0) ? CNT_MIN : CNT_MAX;
		     cont <= CNT_MAX && cont >= CNT_MIN; cont += dc)
		{
			for (size_t i = 0; i < workers; i++) {
				isc_thread_create(isc_mutex_thread, &cont,
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

	printf("[ TIME     ] isc_mutex_benchmark: %zu isc_mutex loops "
	       "in %u "
	       "threads, %2.3f seconds, %2.3f calls/second\n",
	       shared_counter, workers, t / 1000000.0,
	       shared_counter / (t / 1000000.0));

	isc_mutex_destroy(&lock);

	isc_mem_cput(mctx, threads, workers, sizeof(*threads));
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_mutex)
#if !defined(__SANITIZE_THREAD__)
ISC_TEST_ENTRY(isc_mutex_benchmark)
#endif /* __SANITIZE_THREAD__ */

ISC_TEST_LIST_END

ISC_TEST_MAIN_CUSTOM(setup_env, NULL)
