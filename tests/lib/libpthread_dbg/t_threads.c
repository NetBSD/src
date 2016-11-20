/*	$NetBSD: t_threads.c,v 1.3 2016/11/20 16:13:03 kamil Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__RCSID("$NetBSD: t_threads.c,v 1.3 2016/11/20 16:13:03 kamil Exp $");

#include <dlfcn.h>
#include <pthread.h>
#include <pthread_dbg.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#include <atf-c.h>

#include "h_common.h"

ATF_TC(threads1);
ATF_TC_HEAD(threads1, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Asserts that td_thr_iter() call without extra logic works");
}

static volatile int exiting1;

static void *
busyFunction1(void *arg)
{

	while (exiting1 == 0)
		usleep(50000);

	return NULL;
}

static int
iterateThreads1(td_thread_t *thread, void *arg)
{

	return TD_ERR_OK;
}

ATF_TC_BODY(threads1, tc)
{
	struct td_proc_callbacks_t dummy_callbacks;
	td_proc_t *main_ta;
	const size_t max_threads = 10;
	size_t i;
	pthread_t threads[max_threads];

	dummy_callbacks.proc_read	= basic_proc_read;
	dummy_callbacks.proc_write	= basic_proc_write;
	dummy_callbacks.proc_lookup	= basic_proc_lookup;
	dummy_callbacks.proc_regsize	= dummy_proc_regsize;
	dummy_callbacks.proc_getregs	= dummy_proc_getregs;
	dummy_callbacks.proc_setregs	= dummy_proc_setregs;

	for (i = 0; i < max_threads; i++) {
		printf("Creating thread %zu\n", i);
		PTHREAD_REQUIRE
		    (pthread_create(&threads[i], NULL, busyFunction1, NULL));
	}

	printf("Calling td_open(3)\n");
	ATF_REQUIRE(td_open(&dummy_callbacks, NULL, &main_ta) == TD_ERR_OK);

	ATF_REQUIRE(td_thr_iter(main_ta, iterateThreads1, NULL) == TD_ERR_OK);

	exiting1 = 1;

	printf("Calling td_close(3)\n");
	ATF_REQUIRE(td_close(main_ta) == TD_ERR_OK);
}

ATF_TC(threads2);
ATF_TC_HEAD(threads2, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Asserts that td_thr_iter() call is executed for each thread once");
}

static volatile int exiting2;

static void *
busyFunction2(void *arg)
{

	while (exiting2 == 0)
		usleep(50000);

	return NULL;
}

static int
iterateThreads2(td_thread_t *thread, void *arg)
{
	int *counter = (int *)arg;

	++(*counter);

	return TD_ERR_OK;
}

ATF_TC_BODY(threads2, tc)
{
	struct td_proc_callbacks_t dummy_callbacks;
	td_proc_t *main_ta;
	const size_t max_threads = 10;
	size_t i;
	pthread_t threads[max_threads];
	int count = 0;

	dummy_callbacks.proc_read	= basic_proc_read;
	dummy_callbacks.proc_write	= basic_proc_write;
	dummy_callbacks.proc_lookup	= basic_proc_lookup;
	dummy_callbacks.proc_regsize	= dummy_proc_regsize;
	dummy_callbacks.proc_getregs	= dummy_proc_getregs;
	dummy_callbacks.proc_setregs	= dummy_proc_setregs;


	for (i = 0; i < max_threads; i++) {
		printf("Creating thread %zu\n", i);
		PTHREAD_REQUIRE
		    (pthread_create(&threads[i], NULL, busyFunction2, NULL));
	}

	printf("Calling td_open(3)\n");
	ATF_REQUIRE(td_open(&dummy_callbacks, NULL, &main_ta) == TD_ERR_OK);

	ATF_REQUIRE(td_thr_iter(main_ta, iterateThreads2, &count) == TD_ERR_OK);

	exiting2 = 1;

	printf("Calling td_close(3)\n");
	ATF_REQUIRE(td_close(main_ta) == TD_ERR_OK);

	ATF_REQUIRE_EQ_MSG(count, max_threads + 1,
	    "counted threads (%d) != expected threads (%d)",
	    count, max_threads + 1);
}

ATF_TC(threads3);
ATF_TC_HEAD(threads3, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Asserts that for each td_thr_iter() call td_thr_info() is valid");
}

static volatile int exiting3;

static void *
busyFunction3(void *arg)
{

	while (exiting3 == 0)
		usleep(50000);

	return NULL;
}

static int
iterateThreads3(td_thread_t *thread, void *arg)
{
	int *counter = (int *)arg;
	td_thread_info_t info;

	ATF_REQUIRE(td_thr_info(thread, &info) == TD_ERR_OK);

	++(*counter);

	return TD_ERR_OK;
}

ATF_TC_BODY(threads3, tc)
{
	struct td_proc_callbacks_t dummy_callbacks;
	td_proc_t *main_ta;
	const size_t max_threads = 10;
	size_t i;
	pthread_t threads[max_threads];
	int count = 0;

	dummy_callbacks.proc_read	= basic_proc_read;
	dummy_callbacks.proc_write	= basic_proc_write;
	dummy_callbacks.proc_lookup	= basic_proc_lookup;
	dummy_callbacks.proc_regsize	= dummy_proc_regsize;
	dummy_callbacks.proc_getregs	= dummy_proc_getregs;
	dummy_callbacks.proc_setregs	= dummy_proc_setregs;


	for (i = 0; i < max_threads; i++) {
		printf("Creating thread %zu\n", i);
		PTHREAD_REQUIRE
		    (pthread_create(&threads[i], NULL, busyFunction3, NULL));
	}

	printf("Calling td_open(3)\n");
	ATF_REQUIRE(td_open(&dummy_callbacks, NULL, &main_ta) == TD_ERR_OK);

	ATF_REQUIRE(td_thr_iter(main_ta, iterateThreads3, &count) == TD_ERR_OK);

	exiting3 = 1;

	printf("Calling td_close(3)\n");
	ATF_REQUIRE(td_close(main_ta) == TD_ERR_OK);

	ATF_REQUIRE_EQ_MSG(count, max_threads + 1,
	    "counted threads (%d) != expected threads (%d)",
	    count, max_threads + 1);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, threads1);
	ATF_TP_ADD_TC(tp, threads2);
	ATF_TP_ADD_TC(tp, threads3);

	return atf_no_error();
}
