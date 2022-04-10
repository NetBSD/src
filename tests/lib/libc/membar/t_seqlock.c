/*	$NetBSD: t_seqlock.c,v 1.2 2022/04/10 11:36:32 riastradh Exp $	*/

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
__RCSID("$NetBSD: t_seqlock.c,v 1.2 2022/04/10 11:36:32 riastradh Exp $");

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

#ifdef	BROKEN_PRODUCER
#undef	membar_producer
#define	membar_producer()	asm volatile("" ::: "memory")
#endif	/* BROKEN_PRODUCER */

#ifdef	BROKEN_CONSUMER
#undef	membar_consumer
#define	membar_consumer()	asm volatile("" ::: "memory")
#endif	/* BROKEN_CONSUMER */

volatile sig_atomic_t times_up;

volatile unsigned version;

volatile struct {
	uint64_t s;
} __aligned(COHERENCY_UNIT) stats[16];

uint64_t results[2];

static void *
writer(void *cookie)
{
	uint64_t s;
	unsigned i;

	for (s = 0; !times_up; s++) {
		version |= 1;
		membar_producer();
		for (i = __arraycount(stats); i --> 0;)
			stats[i].s = s;
		membar_producer();
		version |= 1;
		version += 1;

		/*
		 * Not needed for correctness, but this helps on Cavium
		 * Octeon cnMIPS CPUs which require issuing a sync
		 * plunger to unclog store buffers which can otherwise
		 * stay clogged for hundreds of thousands of cycles,
		 * giving very little concurrency to this test.
		 * Without this, the reader spends most of its time
		 * thinking an update is in progress.
		 */
		membar_producer();
	}

	return NULL;
}

static void *
reader(void *cookie)
{
	uint64_t s;
	unsigned v, result, i;
	volatile unsigned *vp = &version;
	volatile uint64_t t;

	while (!times_up) {
		/*
		 * Prime the cache with possibly stale garbage.
		 */
		t = stats[0].s;

		/*
		 * Normally we would do
		 *
		 *	while ((v = version) & 1)
		 *		SPINLOCK_BACKOFF_HOOK;
		 *
		 * to avoid copying out a version that we know is in
		 * flux, but it's not wrong to copy out a version in
		 * flux -- just wasteful.
		 *
		 * Reading the version unconditionally, and then
		 * copying out the record, better exercises plausible
		 * bugs in PowerPC membars based on `isync' that
		 * provide the desired ordering only if separated from
		 * the load by a conditional branch based on the load.
		 */
		v = *vp;
		membar_consumer();
		s = stats[0].s;
		for (result = 0, i = 1; i < __arraycount(stats); i++)
			result |= (s != stats[i].s);
		membar_consumer();
		if ((v & ~1u) != *vp)
			continue;
		results[result]++;
	}

	(void)t;

	return NULL;
}

ATF_TC(seqlock);
ATF_TC_HEAD(seqlock, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify membar_producer/consumer work for seqlocks");
}
ATF_TC_BODY(seqlock, tc)
{
	pthread_t t[2];
	void *(*start[2])(void *) = { &reader, &writer };
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
		error = pthread_create(&t[i], NULL, start[i],
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
	ATF_REQUIRE(results[0] != 0);
	ATF_REQUIRE_MSG(results[1] == 0,
	    "%"PRIu64" good snapshots, %"PRIu64" bad snapshots",
	    results[0], results[1]);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, seqlock);
	return atf_no_error();
}
