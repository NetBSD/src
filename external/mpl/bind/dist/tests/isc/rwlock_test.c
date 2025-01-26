/*	$NetBSD: rwlock_test.c,v 1.2 2025/01/26 16:25:50 christos Exp $	*/

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
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/atomic.h>
#include <isc/barrier.h>
#include <isc/file.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/pause.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/rwlock.h>
#include <isc/stdio.h>
#include <isc/thread.h>
#include <isc/time.h>
#include <isc/util.h>

#include <tests/isc.h>

static unsigned int loops = 100;
static unsigned int delay_loop = 1;

static isc_rwlock_t rwlock;
static pthread_rwlock_t prwlock;
static isc_barrier_t barrier1;
static isc_barrier_t barrier2;

#define ITERS 20

#define DC	200
#define CNT_MIN 800
#define CNT_MAX 1600

static size_t shared_counter = 0;
static size_t expected_counter = SIZE_MAX;
static uint8_t boundary = 0;
static uint8_t *rnd;

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

	rnd = isc_mem_cget(mctx, loops, sizeof(rnd[0]));
	for (size_t i = 0; i < loops; i++) {
		rnd[i] = (uint8_t)isc_random_uniform(100);
	}

	return 0;
}

static int
teardown_env(void **state __attribute__((__unused__))) {
	isc_mem_cput(mctx, rnd, loops, sizeof(rnd[0]));

	return 0;
}

static int
rwlock_setup(void **state __attribute__((__unused__))) {
	isc_rwlock_init(&rwlock);

	isc_barrier_init(&barrier1, 2);
	isc_barrier_init(&barrier2, 2);
	if (pthread_rwlock_init(&prwlock, NULL) == -1) {
		return errno;
	}

	return 0;
}

static int
rwlock_teardown(void **state __attribute__((__unused__))) {
	if (pthread_rwlock_destroy(&prwlock) == -1) {
		return errno;
	}
	isc_barrier_destroy(&barrier2);
	isc_barrier_destroy(&barrier1);

	isc_rwlock_destroy(&rwlock);

	return 0;
}

/*
 * Simple single-threaded read lock/unlock test
 */
ISC_RUN_TEST_IMPL(isc_rwlock_rdlock) {
	isc_rwlock_lock(&rwlock, isc_rwlocktype_read);
	isc_pause_n(delay_loop);
	isc_rwlock_unlock(&rwlock, isc_rwlocktype_read);
}

/*
 * Simple single-threaded write lock/unlock test
 */
ISC_RUN_TEST_IMPL(isc_rwlock_wrlock) {
	isc_rwlock_lock(&rwlock, isc_rwlocktype_write);
	isc_pause_n(delay_loop);
	isc_rwlock_unlock(&rwlock, isc_rwlocktype_write);
}

/*
 * Simple single-threaded lock/tryupgrade/unlock test
 */
ISC_RUN_TEST_IMPL(isc_rwlock_tryupgrade) {
	isc_result_t result;
	isc_rwlock_lock(&rwlock, isc_rwlocktype_read);
	result = isc_rwlock_tryupgrade(&rwlock);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_rwlock_unlock(&rwlock, isc_rwlocktype_write);
}

static void *
trylock_thread1(void *arg __attribute__((__unused__))) {
	isc_rwlock_lock(&rwlock, isc_rwlocktype_write);

	isc_barrier_wait(&barrier1);
	isc_barrier_wait(&barrier2);

	isc_rwlock_unlock(&rwlock, isc_rwlocktype_write);

	isc_rwlock_lock(&rwlock, isc_rwlocktype_read);

	isc_barrier_wait(&barrier1);
	isc_barrier_wait(&barrier2);

	isc_rwlock_unlock(&rwlock, isc_rwlocktype_read);

	return NULL;
}

static void *
trylock_thread2(void *arg __attribute__((__unused__))) {
	isc_result_t result;

	isc_barrier_wait(&barrier1);

	result = isc_rwlock_trylock(&rwlock, isc_rwlocktype_read);
	assert_int_equal(result, ISC_R_LOCKBUSY);

	isc_barrier_wait(&barrier2);
	isc_barrier_wait(&barrier1);

	result = isc_rwlock_trylock(&rwlock, isc_rwlocktype_read);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_barrier_wait(&barrier2);

	isc_rwlock_unlock(&rwlock, isc_rwlocktype_read);

	return NULL;
}

ISC_RUN_TEST_IMPL(isc_rwlock_trylock) {
	isc_thread_t thread1;
	isc_thread_t thread2;

	isc_thread_create(trylock_thread1, NULL, &thread1);
	isc_thread_create(trylock_thread2, NULL, &thread2);

	isc_thread_join(thread2, NULL);
	isc_thread_join(thread1, NULL);
}

static void *
pthread_rwlock_thread(void *arg __attribute__((__unused__))) {
	size_t cont = *(size_t *)arg;

	for (size_t i = 0; i < loops; i++) {
		if (rnd[i] < boundary) {
			pthread_rwlock_wrlock(&prwlock);
			size_t v = shared_counter;
			isc_pause_n(delay_loop);
			shared_counter = v + 1;
			pthread_rwlock_unlock(&prwlock);
		} else {
			pthread_rwlock_rdlock(&prwlock);
			isc_pause_n(delay_loop);
			pthread_rwlock_unlock(&prwlock);
		}
		isc_pause_n(cont);
	}

	return NULL;
}

static void *
isc_rwlock_thread(void *arg __attribute__((__unused__))) {
	size_t cont = *(size_t *)arg;

	for (size_t i = 0; i < loops; i++) {
		if (rnd[i] < boundary) {
			isc_rwlock_lock(&rwlock, isc_rwlocktype_write);
			size_t v = shared_counter;
			isc_pause_n(delay_loop);
			shared_counter = v + 1;
			isc_rwlock_unlock(&rwlock, isc_rwlocktype_write);
		} else {
			isc_rwlock_lock(&rwlock, isc_rwlocktype_read);
			isc_pause_n(delay_loop);
			isc_rwlock_unlock(&rwlock, isc_rwlocktype_read);
		}
		isc_pause_n(cont);
	}

	return NULL;
}

static void
isc__rwlock_benchmark(isc_thread_t *threads, unsigned int nthreads,
		      uint8_t pct) {
	isc_time_t ts1, ts2;
	double t;
	int dc;
	size_t cont;

	expected_counter = ITERS * nthreads * loops *
			   ((CNT_MAX - CNT_MIN) / DC + 1);

	boundary = pct;

	/* PTHREAD RWLOCK */

	ts1 = isc_time_now_hires();

	shared_counter = 0;
	dc = DC;
	for (size_t l = 0; l < ITERS; l++) {
		for (cont = (dc > 0) ? CNT_MIN : CNT_MAX;
		     cont <= CNT_MAX && cont >= CNT_MIN; cont += dc)
		{
			for (size_t i = 0; i < nthreads; i++) {
				isc_thread_create(pthread_rwlock_thread, &cont,
						  &threads[i]);
			}
			for (size_t i = 0; i < nthreads; i++) {
				isc_thread_join(threads[i], NULL);
			}
		}
		dc = -dc;
	}

	ts2 = isc_time_now_hires();

	t = isc_time_microdiff(&ts2, &ts1);

	printf("[ TIME     ] isc_rwlock_benchmark: %zu pthread_rwlock loops in "
	       "%u threads, %2.3f%% writes, %2.3f seconds, %2.3f "
	       "calls/second\n",
	       expected_counter, nthreads,
	       (double)shared_counter * 100 / expected_counter, t / 1000000.0,
	       expected_counter / (t / 1000000.0));

	/* ISC RWLOCK */

	ts1 = isc_time_now_hires();

	dc = DC;
	shared_counter = 0;
	for (size_t l = 0; l < ITERS; l++) {
		for (cont = (dc > 0) ? CNT_MIN : CNT_MAX;
		     cont <= CNT_MAX && cont >= CNT_MIN; cont += dc)
		{
			for (size_t i = 0; i < nthreads; i++) {
				isc_thread_create(isc_rwlock_thread, &cont,
						  &threads[i]);
			}
			for (size_t i = 0; i < nthreads; i++) {
				isc_thread_join(threads[i], NULL);
			}
		}
		dc = -dc;
	}

	ts2 = isc_time_now_hires();

	t = isc_time_microdiff(&ts2, &ts1);

	printf("[ TIME     ] isc_rwlock_benchmark: %zu isc_rwlock loops in "
	       "%u threads, %2.3f%% writes, %2.3f seconds, %2.3f "
	       "calls/second\n",
	       expected_counter, nthreads,
	       (double)shared_counter * 100 / expected_counter, t / 1000000.0,
	       expected_counter / (t / 1000000.0));
}

ISC_RUN_TEST_IMPL(isc_rwlock_benchmark) {
	isc_thread_t *threads = isc_mem_cget(mctx, workers, sizeof(*threads));

	memset(threads, 0, sizeof(*threads) * workers);

	for (unsigned int nthreads = workers; nthreads > 0; nthreads /= 2) {
		isc__rwlock_benchmark(threads, nthreads, 0);
		isc__rwlock_benchmark(threads, nthreads, 1);
		isc__rwlock_benchmark(threads, nthreads, 10);
		isc__rwlock_benchmark(threads, nthreads, 50);
		isc__rwlock_benchmark(threads, nthreads, 90);
		isc__rwlock_benchmark(threads, nthreads, 99);
		isc__rwlock_benchmark(threads, nthreads, 100);
	}

	isc_mem_cput(mctx, threads, workers, sizeof(*threads));
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(isc_rwlock_rdlock, rwlock_setup, rwlock_teardown)
ISC_TEST_ENTRY_CUSTOM(isc_rwlock_wrlock, rwlock_setup, rwlock_teardown)
#if !defined(__SANITIZE_THREAD__)
ISC_TEST_ENTRY_CUSTOM(isc_rwlock_tryupgrade, rwlock_setup, rwlock_teardown)
ISC_TEST_ENTRY_CUSTOM(isc_rwlock_trylock, rwlock_setup, rwlock_teardown)
ISC_TEST_ENTRY_CUSTOM(isc_rwlock_benchmark, rwlock_setup, rwlock_teardown)
#endif /* __SANITIZE_THREAD__ */

ISC_TEST_LIST_END

ISC_TEST_MAIN_CUSTOM(setup_env, teardown_env)
