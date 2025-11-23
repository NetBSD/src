/*	$NetBSD: t_dlclose_thread.c,v 1.1 2025/11/23 22:01:13 riastradh Exp $	*/

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
__RCSID("$NetBSD: t_dlclose_thread.c,v 1.1 2025/11/23 22:01:13 riastradh Exp $");

#include <atf-c.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

#include "h_macros.h"

pthread_barrier_t bar;
atomic_bool stop = false;

int sleep_init;
int sleep_fini;
int dlopen_cookie;
int dlclose_cookie;

static const char *const libh_helper_dso[] = {
	"libh_helper_dso1.so",
	"libh_helper_dso2.so",
	"libh_helper_dso3.so",
};

static void *
dlclose_thread(void *cookie)
{
	const unsigned i = (uintptr_t)cookie % __arraycount(libh_helper_dso);
	void *handle;

	(void)pthread_barrier_wait(&bar);
	while (!atomic_load_explicit(&stop, memory_order_relaxed)) {
		handle = dlopen(libh_helper_dso[i], RTLD_LAZY | RTLD_LOCAL);
		ATF_REQUIRE(handle != NULL);
		dlclose(handle);
	}
	return NULL;
}

ATF_TC(dlclose_thread);
ATF_TC_HEAD(dlclose_thread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test concurrent dlopen and dlclose with destructors");
}
ATF_TC_BODY(dlclose_thread, tc)
{
	pthread_t t[2*__arraycount(libh_helper_dso)];
	unsigned i;

	RZ(pthread_barrier_init(&bar, NULL, 1 + __arraycount(t)));
	for (i = 0; i < __arraycount(t); i++) {
		RZ(pthread_create(&t[i], NULL, &dlclose_thread,
			(void *)(uintptr_t)i));
	}
	atf_tc_expect_signal(-1, "PR lib/59751:"
	    " dlclose is not MT-safe depending on the libraries unloaded");
	(void)pthread_barrier_wait(&bar);
	(void)sleep(1);
	atomic_store_explicit(&stop, true, memory_order_relaxed);
	for (i = 0; i < __arraycount(t); i++)
		RZ(pthread_join(t[i], NULL));
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, dlclose_thread);
	return atf_no_error();
}
