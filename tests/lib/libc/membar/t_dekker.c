/*	$NetBSD: t_dekker.c,v 1.3 2022/04/10 11:36:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_dekker.c,v 1.3 2022/04/10 11:36:32 riastradh Exp $");

#include <sys/atomic.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#include <assert.h>
#include <atf-c.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#ifdef	BROKEN_SYNC
#undef	membar_sync
#define	membar_sync()	asm volatile("" ::: "memory")
#endif	/* BROKEN_SYNC */

volatile sig_atomic_t times_up;

volatile unsigned turn __aligned(COHERENCY_UNIT);
volatile struct {
	unsigned v;
} __aligned(COHERENCY_UNIT) waiting[2];
__CTASSERT(sizeof(waiting) == 2*COHERENCY_UNIT);

volatile uint64_t C;
uint64_t TC[2];

static void
lock(unsigned me)
{

top:	waiting[me].v = 1;
	membar_sync();
	while (waiting[1 - me].v) {
		if (turn != me) {
			waiting[me].v = 0;
			while (turn != me)
				continue;
			goto top;
		}
	}
	membar_acquire();
}

static void
unlock(unsigned me)
{

	membar_release();
	turn = 1 - me;
	waiting[me].v = 0;

	/*
	 * Not needed for correctness, but this helps on Cavium Octeon
	 * cnMIPS CPUs which require issuing a sync plunger to unclog
	 * store buffers which can otherwise stay clogged for hundreds
	 * of thousands of cycles, giving very little concurrency to
	 * this test.
	 */
	membar_producer();
}

static void *
thread(void *cookie)
{
	unsigned me = (unsigned)(uintptr_t)cookie;
	uint64_t C_local = 0;

	while (!times_up) {
		C_local++;
		lock(me);
		C++;
		unlock(me);
	}

	TC[me] = C_local;

	return NULL;
}

ATF_TC(dekker);
ATF_TC_HEAD(dekker, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify membar_sync works for Dekker's algorithm");
}
ATF_TC_BODY(dekker, tc)
{
	pthread_t t[2];
	unsigned i;
	int ncpu;
	size_t ncpulen = sizeof(ncpu);
	int error;

	alarm(10);

	if (sysctlbyname("hw.ncpu", &ncpu, &ncpulen, NULL, 0) == -1)
		atf_tc_fail("hw.ncpu: (%d) %s", errno, strerror(errno));
	assert(ncpulen == sizeof(ncpu));
	if (ncpu == 1)
		atf_tc_skip("membar tests are only for multicore systems");

	for (i = 0; i < 2; i++) {
		error = pthread_create(&t[i], NULL, &thread,
		    (void *)(uintptr_t)i);
		if (error)
			errc(1, error, "pthread_create");
	}
	sleep(5);
	times_up = 1;
	for (i = 0; i < 2; i++) {
		error = pthread_join(t[i], NULL);
		if (error)
			errc(1, error, "pthread_join");
	}
	ATF_REQUIRE_MSG(C == TC[0] + TC[1],
	    "%"PRIu64" != %"PRIu64" + %"PRIu64" (off by %"PRIdMAX")",
	    C, TC[0], TC[1], TC[0] + TC[1] - C);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, dekker);
	return atf_no_error();
}
