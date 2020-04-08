/* $NetBSD: t_lwp_tid.c,v 1.1.2.2 2020/04/08 14:09:09 martin Exp $ */

/*-
 * Copyright (c) 2019, 2020 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 2019\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_lwp_tid.c,v 1.1.2.2 2020/04/08 14:09:09 martin Exp $");

#include <sys/mman.h>
#include <errno.h>
#include <lwp.h>
#include <stdio.h>
#include <time.h>

#include <atf-c.h>

#define	MAX_LWPS	8
#define	STACK_SIZE	65536
#define	LWP_TID_MASK	0x3fffffff	/* placeholder */

struct lwp_data {
	ucontext_t	context;
	void		*stack_base;
	lwpid_t		lwpid;
	lwpid_t		threadid;
};

struct lwp_data lwp_data[MAX_LWPS];
lwpid_t master_tid;

static bool
tid_is_unique(int index, lwpid_t tid)
{
	if (tid == master_tid)
		return false;

	for (int i = 0; i < MAX_LWPS; i++) {
		if (i == index)
			continue;
		if (lwp_data[i].threadid == tid)
			return false;
	}

	return true;
}

static void
gettid_test(void *arg)
{
	struct lwp_data *d = arg;

	d->threadid = _lwp_gettid();
	_lwp_exit();
}

ATF_TC(lwp_gettid);
ATF_TC_HEAD(lwp_gettid, tc)
{
	atf_tc_set_md_var(tc, "descr", "checks _lwp_gettid()");
}

ATF_TC_BODY(lwp_gettid, tc)
{
	int i;

	master_tid = _lwp_gettid();
	ATF_REQUIRE(master_tid != 0);
	ATF_REQUIRE((master_tid & ~LWP_TID_MASK) == 0);

	/* Initialize our LWP data. */
	for (i = 0; i < MAX_LWPS; i++) {
		lwp_data[i].stack_base = mmap(NULL, STACK_SIZE,
		    PROT_READ | PROT_WRITE,
		    MAP_ANON | MAP_STACK | MAP_PRIVATE, -1, 0);
		ATF_REQUIRE(lwp_data[i].stack_base != MAP_FAILED);
		_lwp_makecontext(&lwp_data[i].context, gettid_test,
		    &lwp_data[i], NULL, lwp_data[i].stack_base,
		    STACK_SIZE);
		lwp_data[i].threadid = 0;
	}

	/* Now kick off all the LWPs... */
	for (i = 0; i < MAX_LWPS; i++) {
		ATF_REQUIRE(_lwp_create(&lwp_data[i].context, 0,
		    &lwp_data[i].lwpid) == 0);
		printf("Started LWP %d\n", lwp_data[i].lwpid);
	}

	/* ...and wait for them to exit. */
	for (i = 0; i < MAX_LWPS;) {
		lwpid_t reaped_lwpid;

		int rv = _lwp_wait(0, &reaped_lwpid);
		if (rv == 0) {
			printf("Reaped LWP %d\n", reaped_lwpid);
			i++;
			continue;
		} else {
			ATF_REQUIRE(errno != EDEADLK);
			continue;
		}
	}

	/* Check the TIDs. */
	for (i = 0; i < MAX_LWPS; i++) {
		printf("Checking tid[%d] %d (0x%08x)\n", i,
		    lwp_data[i].threadid, lwp_data[i].threadid);
		ATF_REQUIRE(lwp_data[i].threadid != 0);
		ATF_REQUIRE((lwp_data[i].threadid & ~LWP_TID_MASK) == 0);
		ATF_REQUIRE(tid_is_unique(i, lwp_data[i].threadid));
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, lwp_gettid);

	return atf_no_error();
}
