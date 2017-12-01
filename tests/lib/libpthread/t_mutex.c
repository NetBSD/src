/* $NetBSD: t_mutex.c,v 1.19 2017/12/01 13:25:29 kre Exp $ */

/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 2008\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_mutex.c,v 1.19 2017/12/01 13:25:29 kre Exp $");

#include <sys/time.h> /* For timespecadd */
#include <inttypes.h> /* For UINT16_MAX */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/sched.h>
#include <sys/param.h>

#include <atf-c.h>

#include "h_common.h"

static pthread_mutex_t mutex;
static pthread_mutex_t static_mutex = PTHREAD_MUTEX_INITIALIZER;
static int global_x;

#ifdef TIMEDMUTEX
/* This code is used for verifying non-timed specific code */
static struct timespec ts_lengthy = {
	.tv_sec = UINT16_MAX,
	.tv_nsec = 0
};
/* This code is used for verifying timed-only specific code */
static struct timespec ts_shortlived = {
	.tv_sec = 0,
	.tv_nsec = 120
};

static int
mutex_lock(pthread_mutex_t *m, const struct timespec *ts)
{
	struct timespec ts_wait;
	ATF_REQUIRE(clock_gettime(CLOCK_REALTIME, &ts_wait) != -1);
	timespecadd(&ts_wait, ts, &ts_wait);

	return pthread_mutex_timedlock(m, &ts_wait);
}
#else
#define mutex_lock(a, b) pthread_mutex_lock(a)
#endif

static void *
mutex1_threadfunc(void *arg)
{
	int *param;

	printf("2: Second thread.\n");

	param = arg;
	printf("2: Locking mutex\n");
	mutex_lock(&mutex, &ts_lengthy);
	printf("2: Got mutex. *param = %d\n", *param);
	ATF_REQUIRE_EQ(*param, 20);
	(*param)++;

	pthread_mutex_unlock(&mutex);

	return param;
}

ATF_TC(mutex1);
ATF_TC_HEAD(mutex1, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks mutexes");
}
ATF_TC_BODY(mutex1, tc)
{
	int x;
	pthread_t new;
	void *joinval;

	printf("1: Mutex-test 1\n");

	PTHREAD_REQUIRE(pthread_mutex_init(&mutex, NULL));
	x = 1;
	PTHREAD_REQUIRE(mutex_lock(&mutex, &ts_lengthy));
	PTHREAD_REQUIRE(pthread_create(&new, NULL, mutex1_threadfunc, &x));
	printf("1: Before changing the value.\n");
	sleep(2);
	x = 20;
	printf("1: Before releasing the mutex.\n");
	sleep(2);
	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));
	printf("1: After releasing the mutex.\n");
	PTHREAD_REQUIRE(pthread_join(new, &joinval));

	PTHREAD_REQUIRE(mutex_lock(&mutex, &ts_lengthy));
	printf("1: Thread joined. X was %d. Return value (int) was %d\n",
		x, *(int *)joinval);
	ATF_REQUIRE_EQ(x, 21);
	ATF_REQUIRE_EQ(*(int *)joinval, 21);
	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));
}

static void *
mutex2_threadfunc(void *arg)
{
	long count = *(int *)arg;

	printf("2: Second thread (%p). Count is %ld\n", pthread_self(), count);

	while (count--) {
		PTHREAD_REQUIRE(mutex_lock(&mutex, &ts_lengthy));
		global_x++;
		PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));
	}

	return (void *)count;
}

ATF_TC(mutex2);
ATF_TC_HEAD(mutex2, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks mutexes");
}
ATF_TC_BODY(mutex2, tc)
{
	int count, count2;
	pthread_t new;
	void *joinval;

	printf("1: Mutex-test 2\n");

	PTHREAD_REQUIRE(pthread_mutex_init(&mutex, NULL));
	
	global_x = 0;
	count = count2 = 10000000;

	PTHREAD_REQUIRE(mutex_lock(&mutex, &ts_lengthy));
	PTHREAD_REQUIRE(pthread_create(&new, NULL, mutex2_threadfunc, &count2));

	printf("1: Thread %p\n", pthread_self());

	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));

	while (count--) {
		PTHREAD_REQUIRE(mutex_lock(&mutex, &ts_lengthy));
		global_x++;
		PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));
	}

	PTHREAD_REQUIRE(pthread_join(new, &joinval));

	PTHREAD_REQUIRE(mutex_lock(&mutex, &ts_lengthy));
	printf("1: Thread joined. X was %d. Return value (long) was %ld\n",
		global_x, (long)joinval);
	ATF_REQUIRE_EQ(global_x, 20000000);
}

static void *
mutex3_threadfunc(void *arg)
{
	long count = *(int *)arg;

	printf("2: Second thread (%p). Count is %ld\n", pthread_self(), count);

	while (count--) {
		PTHREAD_REQUIRE(mutex_lock(&static_mutex, &ts_lengthy));
		global_x++;
		PTHREAD_REQUIRE(pthread_mutex_unlock(&static_mutex));
	}

	return (void *)count;
}

ATF_TC(mutex3);
ATF_TC_HEAD(mutex3, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks mutexes using a static "
	    "initializer");
}
ATF_TC_BODY(mutex3, tc)
{
	int count, count2;
	pthread_t new;
	void *joinval;

	printf("1: Mutex-test 3\n");

	global_x = 0;
	count = count2 = 10000000;

	PTHREAD_REQUIRE(mutex_lock(&static_mutex, &ts_lengthy));
	PTHREAD_REQUIRE(pthread_create(&new, NULL, mutex3_threadfunc, &count2));

	printf("1: Thread %p\n", pthread_self());

	PTHREAD_REQUIRE(pthread_mutex_unlock(&static_mutex));

	while (count--) {
		PTHREAD_REQUIRE(mutex_lock(&static_mutex, &ts_lengthy));
		global_x++;
		PTHREAD_REQUIRE(pthread_mutex_unlock(&static_mutex));
	}

	PTHREAD_REQUIRE(pthread_join(new, &joinval));

	PTHREAD_REQUIRE(mutex_lock(&static_mutex, &ts_lengthy));
	printf("1: Thread joined. X was %d. Return value (long) was %ld\n",
		global_x, (long)joinval);
	ATF_REQUIRE_EQ(global_x, 20000000);
}

static void *
mutex4_threadfunc(void *arg)
{
	int *param;

	printf("2: Second thread.\n");

	param = arg;
	printf("2: Locking mutex\n");
	PTHREAD_REQUIRE(mutex_lock(&mutex, &ts_lengthy));
	printf("2: Got mutex. *param = %d\n", *param);
	(*param)++;

	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));

	return param;
}

ATF_TC(mutex4);
ATF_TC_HEAD(mutex4, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks mutexes");
}
ATF_TC_BODY(mutex4, tc)
{
	int x;
	pthread_t new;
	pthread_mutexattr_t mattr;
	void *joinval;

	printf("1: Mutex-test 4\n");

	PTHREAD_REQUIRE(pthread_mutexattr_init(&mattr));
	PTHREAD_REQUIRE(pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE));

	PTHREAD_REQUIRE(pthread_mutex_init(&mutex, &mattr));

	PTHREAD_REQUIRE(pthread_mutexattr_destroy(&mattr));

	x = 1;
	PTHREAD_REQUIRE(mutex_lock(&mutex, &ts_lengthy));
	PTHREAD_REQUIRE(pthread_create(&new, NULL, mutex4_threadfunc, &x));

	printf("1: Before recursively acquiring the mutex.\n");
	PTHREAD_REQUIRE(mutex_lock(&mutex, &ts_lengthy));

	printf("1: Before releasing the mutex once.\n");
	sleep(2);
	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));
	printf("1: After releasing the mutex once.\n");

	x = 20;

	printf("1: Before releasing the mutex twice.\n");
	sleep(2);
	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));
	printf("1: After releasing the mutex twice.\n");

	PTHREAD_REQUIRE(pthread_join(new, &joinval));

	PTHREAD_REQUIRE(mutex_lock(&mutex, &ts_lengthy));
	printf("1: Thread joined. X was %d. Return value (int) was %d\n",
		x, *(int *)joinval);
	ATF_REQUIRE_EQ(x, 21);
	ATF_REQUIRE_EQ(*(int *)joinval, 21);
	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));
}

static pthread_mutexattr_t attr5;
static pthread_mutex_t mutex5;
static int min_fifo_prio, max_fifo_prio;

static void *
child_func(void* arg)
{
	int res;

	printf("child is waiting\n");
	res = _sched_protect(-2);
	ATF_REQUIRE_EQ_MSG(res, -1, "sched_protect returned %d", res);
	ATF_REQUIRE_EQ(errno, ENOENT);
	PTHREAD_REQUIRE(mutex_lock(&mutex5, &ts_lengthy));
	printf("child is owning resource\n");
	res = _sched_protect(-2);
	ATF_REQUIRE_EQ(res,  max_fifo_prio);
	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex5));
	printf("child is done\n");
	
	return 0;
}

ATF_TC(mutex5);
ATF_TC_HEAD(mutex5, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks mutexes for priority setting");
	atf_tc_set_md_var(tc, "require.user", "root");
}

ATF_TC_BODY(mutex5, tc)
{
	int res;
	struct sched_param param;
	pthread_t child;

	min_fifo_prio = sched_get_priority_min(SCHED_FIFO);
	max_fifo_prio = sched_get_priority_max(SCHED_FIFO);
	printf("min prio for FIFO = %d\n", min_fifo_prio);
	param.sched_priority = min_fifo_prio;

	/* = 0 OTHER, 1 FIFO, 2 RR, -1 NONE */
	res = sched_setscheduler(getpid(), SCHED_FIFO, &param);
	printf("previous policy used = %d\n", res);

	res = sched_getscheduler(getpid());
	ATF_REQUIRE_EQ_MSG(res, SCHED_FIFO, "sched %d != FIFO %d", res, 
	    SCHED_FIFO);

	PTHREAD_REQUIRE(pthread_mutexattr_init(&attr5));
	PTHREAD_REQUIRE(pthread_mutexattr_setprotocol(&attr5,
	    PTHREAD_PRIO_PROTECT));
	PTHREAD_REQUIRE(pthread_mutexattr_setprioceiling(&attr5,
	    max_fifo_prio));
	
	PTHREAD_REQUIRE(pthread_mutex_init(&mutex5, &attr5));
	PTHREAD_REQUIRE(mutex_lock(&mutex5, &ts_lengthy));
	printf("enter critical section for main\n");
	PTHREAD_REQUIRE(pthread_create(&child, NULL, child_func, NULL));
	printf("main starts to sleep\n");
	sleep(10);
	printf("main completes\n");
	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex5));
	PTHREAD_REQUIRE(pthread_join(child, NULL));
}

ATF_TC(mutexattr1);
ATF_TC_HEAD(mutexattr1, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks mutexattr");
}

ATF_TC_BODY(mutexattr1, tc)
{
	pthread_mutexattr_t mattr;
	int protocol, target;
	
	PTHREAD_REQUIRE(pthread_mutexattr_init(&mattr));

	target = PTHREAD_PRIO_NONE;
	PTHREAD_REQUIRE(pthread_mutexattr_setprotocol(&mattr, target));
	PTHREAD_REQUIRE(pthread_mutexattr_getprotocol(&mattr, &protocol));
	ATF_REQUIRE_EQ(protocol, target);

	/*
	target = PTHREAD_PRIO_INHERIT;
	PTHREAD_REQUIRE(pthread_mutexattr_setprotocol(&mattr, target));
	PTHREAD_REQUIRE(pthread_mutexattr_getprotocol(&mattr, &protocol));
	ATF_REQUIRE_EQ(protocol, target);
	*/

	target = PTHREAD_PRIO_PROTECT;
	PTHREAD_REQUIRE(pthread_mutexattr_setprotocol(&mattr, target));
	PTHREAD_REQUIRE(pthread_mutexattr_getprotocol(&mattr, &protocol));
	ATF_REQUIRE_EQ(protocol, target);
}

ATF_TC(mutexattr2);
ATF_TC_HEAD(mutexattr2, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks mutexattr");
}

ATF_TC_BODY(mutexattr2, tc)
{
	pthread_mutexattr_t mattr;

	PTHREAD_REQUIRE(pthread_mutexattr_init(&mattr));
	int max_prio = sched_get_priority_max(SCHED_FIFO);
	int min_prio = sched_get_priority_min(SCHED_FIFO);
	for (int i = min_prio; i <= max_prio; i++) {
		int prioceiling;
		int protocol;

		PTHREAD_REQUIRE(pthread_mutexattr_getprotocol(&mattr,
		    &protocol));

		printf("priority: %d\nprotocol: %d\n", i, protocol);
		PTHREAD_REQUIRE(pthread_mutexattr_setprioceiling(&mattr, i));
		PTHREAD_REQUIRE(pthread_mutexattr_getprioceiling(&mattr,
		    &prioceiling));
		printf("prioceiling: %d\n", prioceiling);
		ATF_REQUIRE_EQ(i, prioceiling);
	}
}

#ifdef TIMEDMUTEX
ATF_TC(timedmutex1);
ATF_TC_HEAD(timedmutex1, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks timeout on selflock");
}

ATF_TC_BODY(timedmutex1, tc)
{

	printf("Timed mutex-test 1\n");

	PTHREAD_REQUIRE(pthread_mutex_init(&mutex, NULL));

	printf("Before acquiring mutex\n");
	PTHREAD_REQUIRE(pthread_mutex_lock(&mutex));

	printf("Before endeavor to reacquire timed-mutex (timeout expected)\n");
	PTHREAD_REQUIRE_STATUS(mutex_lock(&mutex, &ts_shortlived),
	    ETIMEDOUT);

	printf("Unlocking mutex\n");
	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));
}

ATF_TC(timedmutex2);
ATF_TC_HEAD(timedmutex2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks timeout on selflock with timedlock");
}

ATF_TC_BODY(timedmutex2, tc)
{

	printf("Timed mutex-test 2\n");

	PTHREAD_REQUIRE(pthread_mutex_init(&mutex, NULL));

	printf("Before acquiring mutex with timedlock\n");
	PTHREAD_REQUIRE(mutex_lock(&mutex, &ts_lengthy));

	printf("Before endeavor to reacquire timed-mutex (timeout expected)\n");
	PTHREAD_REQUIRE_STATUS(mutex_lock(&mutex, &ts_shortlived),
	    ETIMEDOUT);

	printf("Unlocking mutex\n");
	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));
}

ATF_TC(timedmutex3);
ATF_TC_HEAD(timedmutex3, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks timeout on selflock in a new thread");
}

static void *
timedmtx_thrdfunc(void *arg)
{
	printf("Before endeavor to reacquire timed-mutex (timeout expected)\n");
	PTHREAD_REQUIRE_STATUS(mutex_lock(&mutex, &ts_shortlived),
	    ETIMEDOUT);

	return NULL;
}

ATF_TC_BODY(timedmutex3, tc)
{
	pthread_t new;

	printf("Timed mutex-test 3\n");

	PTHREAD_REQUIRE(pthread_mutex_init(&mutex, NULL));

	printf("Before acquiring mutex with timedlock\n");
	PTHREAD_REQUIRE(pthread_mutex_lock(&mutex));

	printf("Before creating new thread\n");
	PTHREAD_REQUIRE(pthread_create(&new, NULL, timedmtx_thrdfunc, NULL));

	printf("Before joining the mutex\n");
	PTHREAD_REQUIRE(pthread_join(new, NULL));

	printf("Unlocking mutex\n");
	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));
}

ATF_TC(timedmutex4);
ATF_TC_HEAD(timedmutex4, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks timeout on selflock with timedlock in a new thread");
}

ATF_TC_BODY(timedmutex4, tc)
{
	pthread_t new;

	printf("Timed mutex-test 4\n");

	PTHREAD_REQUIRE(pthread_mutex_init(&mutex, NULL));

	printf("Before acquiring mutex with timedlock\n");
	PTHREAD_REQUIRE(mutex_lock(&mutex, &ts_lengthy));

	printf("Before creating new thread\n");
	PTHREAD_REQUIRE(pthread_create(&new, NULL, timedmtx_thrdfunc, NULL));

	printf("Before joining the mutex\n");
	PTHREAD_REQUIRE(pthread_join(new, NULL));

	printf("Unlocking mutex\n");
	PTHREAD_REQUIRE(pthread_mutex_unlock(&mutex));
}
#endif

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, mutex1);
	ATF_TP_ADD_TC(tp, mutex2);
	ATF_TP_ADD_TC(tp, mutex3);
	ATF_TP_ADD_TC(tp, mutex4);
	ATF_TP_ADD_TC(tp, mutex5);
	ATF_TP_ADD_TC(tp, mutexattr1);
	ATF_TP_ADD_TC(tp, mutexattr2);

#ifdef TIMEDMUTEX
	ATF_TP_ADD_TC(tp, timedmutex1);
	ATF_TP_ADD_TC(tp, timedmutex2);
	ATF_TP_ADD_TC(tp, timedmutex3);
	ATF_TP_ADD_TC(tp, timedmutex4);
#endif

	return atf_no_error();
}
