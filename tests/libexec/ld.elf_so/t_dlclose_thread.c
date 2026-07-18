/*	$NetBSD: t_dlclose_thread.c,v 1.6 2026/07/18 04:26:42 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_dlclose_thread.c,v 1.6 2026/07/18 04:26:42 riastradh Exp $");

#include <atf-c.h>
#include <dlfcn.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

#include "h_macros.h"

/*
 * abortafter(sec)
 *
 *	Arrange to deliver SIGABRT after the specified number of
 *	seconds.  We use SIGABRT rather than SIGALRM in order to get a
 *	core dump so the atf test harness will give a stack trace.
 */
static void
abortafter(unsigned sec)
{
	timer_t t;
	struct sigevent sigev = {
		.sigev_notify = SIGEV_SIGNAL,
		.sigev_signo = SIGABRT, /* yields core dump -> stack trace */
	};
	const struct itimerspec it = {
		.it_value = {sec,0},
		.it_interval = {0,0},
	};

	RL(timer_create(CLOCK_MONOTONIC, &sigev, &t));
	RL(timer_settime(t, TIMER_RELTIME, &it, NULL));
}

pthread_barrier_t bar;
atomic_bool stop = false;

int sleep_init;
int sleep_fini;
int dlopen_cookie;
int dlclose_cookie;

#define	MAXNDSOS	5

static void *
dlclose_thread(void *cookie)
{
	const char *const path = cookie;
	void *handle;

	(void)pthread_barrier_wait(&bar);
	while (!atomic_load_explicit(&stop, memory_order_relaxed)) {
		handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
		ATF_REQUIRE(handle != NULL);
		dlclose(handle);
	}
	return NULL;
}

static void
test_dlclose_thread(const char *const *dsos, size_t ndsos)
{
	pthread_t t[2*MAXNDSOS];
	unsigned i;

	ATF_REQUIRE_MSG(ndsos <= MAXNDSOS, "ndso=%zu", ndsos);

	RZ(pthread_barrier_init(&bar, NULL, 1 + 2*ndsos));
	for (i = 0; i < 2*ndsos; i++) {
		const char *const dso = dsos[i % ndsos];
		RZ(pthread_create(&t[i], NULL, &dlclose_thread,
			__UNCONST(dso)));
	}
	(void)pthread_barrier_wait(&bar);
	(void)sleep(5);
	atomic_store_explicit(&stop, true, memory_order_relaxed);
	abortafter(5);
	for (i = 0; i < 2*ndsos; i++)
		RZ(pthread_join(t[i], NULL));
}

ATF_TC(dlclose_thread);
ATF_TC_HEAD(dlclose_thread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test concurrent dlopen and dlclose with destructors");
	atf_tc_set_md_var(tc, "timeout", "20");
}
ATF_TC_BODY(dlclose_thread, tc)
{
	static const char *const dsos[] = {
		"libh_helper_dso1.so",
		"libh_helper_dso2.so",
		"libh_helper_dso3.so",
	};

	test_dlclose_thread(dsos, __arraycount(dsos));
}

ATF_TC(dlclose_thread_recursive);
ATF_TC_HEAD(dlclose_thread_recursive, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test concurrent dlopen/dlclose with recursive dlopen/dlclose");
	atf_tc_set_md_var(tc, "timeout", "20");
}
ATF_TC_BODY(dlclose_thread_recursive, tc)
{
	static const char *const dsos[] = {
		"libh_helper_dso1.so",
		"libh_helper_dso2.so",
		"libh_helper_dso3.so",
		"libh_helper_recurdso.so",
	};

	test_dlclose_thread(dsos, __arraycount(dsos));
}

ATF_TC(dlclose_thread_recursive2);
ATF_TC_HEAD(dlclose_thread_recursive2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test concurrent dlopen/dlclose with recursive^2 dlopen/dlclose");
	atf_tc_set_md_var(tc, "timeout", "20");
}
ATF_TC_BODY(dlclose_thread_recursive2, tc)
{
	static const char *const dsos[] = {
		"libh_helper_dso1.so",
		"libh_helper_dso2.so",
		"libh_helper_dso3.so",
		"libh_helper_recurdso.so",
		"libh_helper_recurdso2.so",
	};

	test_dlclose_thread(dsos, __arraycount(dsos));
}

ATF_TC(dlclose_recursive);
ATF_TC_HEAD(dlclose_recursive, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test dlopen/dlclose with recursive dlopen/dlclose in init/fini");
	atf_tc_set_md_var(tc, "timeout", "20");
}
ATF_TC_BODY(dlclose_recursive, tc)
{
	void *handle;
	int error;

	abortafter(5);

	handle = dlopen("libh_helper_recurdso.so", RTLD_LAZY | RTLD_LOCAL);
	ATF_REQUIRE_MSG(handle != NULL, "%s", dlerror());
	ATF_REQUIRE_MSG((error = dlclose(handle)) == 0,
	    "error=%d (%s)", error, dlerror());
}

ATF_TC(dlclose_recursive2);
ATF_TC_HEAD(dlclose_recursive2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test recursive dlopen/dlclose in init/fini with dependencies");
	atf_tc_set_md_var(tc, "timeout", "20");
}
ATF_TC_BODY(dlclose_recursive2, tc)
{
	void *handle;
	int error;

	abortafter(5);

	handle = dlopen("libh_helper_recurdso2.so", RTLD_LAZY | RTLD_LOCAL);
	ATF_REQUIRE_MSG(handle != NULL, "%s", dlerror());
	ATF_REQUIRE_MSG((error = dlclose(handle)) == 0,
	    "error=%d (%s)", error, dlerror());
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, dlclose_recursive);
	ATF_TP_ADD_TC(tp, dlclose_recursive2);
	ATF_TP_ADD_TC(tp, dlclose_thread);
	ATF_TP_ADD_TC(tp, dlclose_thread_recursive);
	ATF_TP_ADD_TC(tp, dlclose_thread_recursive2);
	return atf_no_error();
}
