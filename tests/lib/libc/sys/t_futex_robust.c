/* $NetBSD: t_futex_robust.c,v 1.2 2020/05/01 01:44:30 thorpej Exp $ */

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_futex_robust.c,v 1.2 2020/05/01 01:44:30 thorpej Exp $");

#include <sys/mman.h>
#include <errno.h>
#include <lwp.h>
#include <stdio.h>
#include <time.h>

#include <atf-c.h>

#include <libc/include/futex_private.h>

#define	STACK_SIZE	65536
#define	NLOCKS		16

struct futex_lock_pos {
	struct futex_robust_list	list;
	int				fword;
};
struct futex_lock_pos pos_locks[NLOCKS];

struct futex_lock_neg {
	int				fword;
	struct futex_robust_list	list;
};
struct futex_lock_neg neg_locks[NLOCKS];

struct lwp_data {
	ucontext_t	context;
	void		*stack_base;
	lwpid_t		lwpid;
	lwpid_t		threadid;
	struct futex_robust_list_head rhead;

	/* Results to be asserted by main thread. */
	bool		set_robust_list_failed;
};

struct lwp_data lwp_data;

static void
setup_lwp_context(void (*func)(void *))
{

	memset(&lwp_data, 0, sizeof(lwp_data));
	lwp_data.stack_base = mmap(NULL, STACK_SIZE,
	    PROT_READ | PROT_WRITE,
	    MAP_ANON | MAP_STACK | MAP_PRIVATE, -1, 0);
	ATF_REQUIRE(lwp_data.stack_base != MAP_FAILED);
	_lwp_makecontext(&lwp_data.context, func,
	    &lwp_data, NULL, lwp_data.stack_base, STACK_SIZE);
	lwp_data.threadid = 0;
}

static void
do_cleanup(void)
{
	if (lwp_data.stack_base != NULL &&
	    lwp_data.stack_base != MAP_FAILED) {
		(void) munmap(lwp_data.stack_base, STACK_SIZE);
	}
	memset(&lwp_data, 0, sizeof(lwp_data));
	memset(pos_locks, 0, sizeof(pos_locks));
	memset(neg_locks, 0, sizeof(neg_locks));
}

static void
test_pos_robust_list(void *arg)
{
	struct lwp_data *d = arg;
	int i;

	d->rhead.list.next = &d->rhead.list;
	d->rhead.futex_offset = offsetof(struct futex_lock_pos, fword) -
	    offsetof(struct futex_lock_pos, list);
	d->rhead.pending_list = NULL;

	if (__futex_set_robust_list(&d->rhead, sizeof(d->rhead)) != 0) {
		d->set_robust_list_failed = true;
		_lwp_exit();
	}

	memset(pos_locks, 0, sizeof(pos_locks));

	d->threadid = _lwp_self();

	for (i = 0; i < NLOCKS-1; i++) {
		pos_locks[i].fword = _lwp_self();
		pos_locks[i].list.next = d->rhead.list.next;
		d->rhead.list.next = &pos_locks[i].list;
	}

	pos_locks[i].fword = _lwp_self();
	d->rhead.pending_list = &pos_locks[i].list;

	_lwp_exit();
}

static void
test_neg_robust_list(void *arg)
{
	struct lwp_data *d = arg;
	int i;

	d->rhead.list.next = &d->rhead.list;
	d->rhead.futex_offset = offsetof(struct futex_lock_neg, fword) -
	    offsetof(struct futex_lock_neg, list);
	d->rhead.pending_list = NULL;

	if (__futex_set_robust_list(&d->rhead, sizeof(d->rhead)) != 0) {
		d->set_robust_list_failed = true;
		_lwp_exit();
	}

	memset(neg_locks, 0, sizeof(neg_locks));

	d->threadid = _lwp_self();

	for (i = 0; i < NLOCKS-1; i++) {
		neg_locks[i].fword = _lwp_self();
		neg_locks[i].list.next = d->rhead.list.next;
		d->rhead.list.next = &neg_locks[i].list;
	}

	neg_locks[i].fword = _lwp_self();
	d->rhead.pending_list = &neg_locks[i].list;

	_lwp_exit();
}

static void
test_unmapped_robust_list(void *arg)
{
	struct lwp_data *d = arg;

	d->rhead.list.next = &d->rhead.list;
	d->rhead.futex_offset = offsetof(struct futex_lock_pos, fword) -
	    offsetof(struct futex_lock_pos, list);
	d->rhead.pending_list = NULL;

	if (__futex_set_robust_list((void *)sizeof(d->rhead),
				    sizeof(d->rhead)) != 0) {
		d->set_robust_list_failed = true;
		_lwp_exit();
	}

	memset(pos_locks, 0, sizeof(pos_locks));

	d->threadid = _lwp_self();

	_lwp_exit();
}

static void
test_evil_circular_robust_list(void *arg)
{
	struct lwp_data *d = arg;
	int i;

	d->rhead.list.next = &d->rhead.list;
	d->rhead.futex_offset = offsetof(struct futex_lock_pos, fword) -
	    offsetof(struct futex_lock_pos, list);
	d->rhead.pending_list = NULL;

	if (__futex_set_robust_list(&d->rhead, sizeof(d->rhead)) != 0) {
		d->set_robust_list_failed = true;
		_lwp_exit();
	}

	memset(pos_locks, 0, sizeof(pos_locks));

	d->threadid = _lwp_self();

	for (i = 0; i < NLOCKS; i++) {
		pos_locks[i].fword = _lwp_self();
		pos_locks[i].list.next = d->rhead.list.next;
		d->rhead.list.next = &pos_locks[i].list;
	}

	/* Make a loop. */
	pos_locks[0].list.next = pos_locks[NLOCKS-1].list.next;

	_lwp_exit();
}

static void
test_bad_pending_robust_list(void *arg)
{
	struct lwp_data *d = arg;
	int i;

	d->rhead.list.next = &d->rhead.list;
	d->rhead.futex_offset = offsetof(struct futex_lock_pos, fword) -
	    offsetof(struct futex_lock_pos, list);
	d->rhead.pending_list = NULL;

	if (__futex_set_robust_list(&d->rhead, sizeof(d->rhead)) != 0) {
		d->set_robust_list_failed = true;
		_lwp_exit();
	}

	memset(pos_locks, 0, sizeof(pos_locks));

	d->threadid = _lwp_self();

	for (i = 0; i < NLOCKS; i++) {
		pos_locks[i].fword = _lwp_self();
		pos_locks[i].list.next = d->rhead.list.next;
		d->rhead.list.next = &pos_locks[i].list;
	}

	d->rhead.pending_list = (void *)sizeof(d->rhead);

	_lwp_exit();
}

ATF_TC_WITH_CLEANUP(futex_robust_positive);
ATF_TC_HEAD(futex_robust_positive, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "checks futex robust list with positive futex word offset");
}

ATF_TC_BODY(futex_robust_positive, tc)
{
	int i;

	setup_lwp_context(test_pos_robust_list);

	ATF_REQUIRE(_lwp_create(&lwp_data.context, 0, &lwp_data.lwpid) == 0);
	ATF_REQUIRE(_lwp_wait(lwp_data.lwpid, NULL) == 0);

	ATF_REQUIRE(lwp_data.set_robust_list_failed == false);

	for (i = 0; i < NLOCKS; i++) {
		ATF_REQUIRE((pos_locks[i].fword & FUTEX_TID_MASK) ==
		    lwp_data.threadid);
		ATF_REQUIRE((pos_locks[i].fword & FUTEX_OWNER_DIED) != 0);
	}
}

ATF_TC_CLEANUP(futex_robust_positive, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_robust_negative);
ATF_TC_HEAD(futex_robust_negative, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "checks futex robust list with negative futex word offset");
}

ATF_TC_BODY(futex_robust_negative, tc)
{
	int i;

	setup_lwp_context(test_neg_robust_list);

	ATF_REQUIRE(_lwp_create(&lwp_data.context, 0, &lwp_data.lwpid) == 0);
	ATF_REQUIRE(_lwp_wait(lwp_data.lwpid, NULL) == 0);

	ATF_REQUIRE(lwp_data.set_robust_list_failed == false);

	for (i = 0; i < NLOCKS; i++) {
		ATF_REQUIRE((neg_locks[i].fword & FUTEX_TID_MASK) ==
		    lwp_data.threadid);
		ATF_REQUIRE((neg_locks[i].fword & FUTEX_OWNER_DIED) != 0);
	}
}

ATF_TC_CLEANUP(futex_robust_negative, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_robust_unmapped);
ATF_TC_HEAD(futex_robust_unmapped, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "checks futex robust list with unmapped robust list pointer");
}

ATF_TC_BODY(futex_robust_unmapped, tc)
{

	setup_lwp_context(test_unmapped_robust_list);

	ATF_REQUIRE(_lwp_create(&lwp_data.context, 0, &lwp_data.lwpid) == 0);
	ATF_REQUIRE(_lwp_wait(lwp_data.lwpid, NULL) == 0);

	ATF_REQUIRE(lwp_data.set_robust_list_failed == false);

	/*
	 * No additional validation; just exercises a code path
	 * in the kernel.
	 */
}

ATF_TC_CLEANUP(futex_robust_unmapped, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_robust_evil_circular);
ATF_TC_HEAD(futex_robust_evil_circular, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "checks futex robust list processing faced with a deliberately "
	    "ciruclar list");
}

ATF_TC_BODY(futex_robust_evil_circular, tc)
{
	int i;

	setup_lwp_context(test_evil_circular_robust_list);

	ATF_REQUIRE(_lwp_create(&lwp_data.context, 0, &lwp_data.lwpid) == 0);
	ATF_REQUIRE(_lwp_wait(lwp_data.lwpid, NULL) == 0);

	ATF_REQUIRE(lwp_data.set_robust_list_failed == false);

	for (i = 0; i < NLOCKS; i++) {
		ATF_REQUIRE((pos_locks[i].fword & FUTEX_TID_MASK) ==
		    lwp_data.threadid);
		ATF_REQUIRE((pos_locks[i].fword & FUTEX_OWNER_DIED) != 0);
	}
}

ATF_TC_CLEANUP(futex_robust_evil_circular, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_robust_bad_pending);
ATF_TC_HEAD(futex_robust_bad_pending, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "checks futex robust list processing with a bad pending pointer");
}

ATF_TC_BODY(futex_robust_bad_pending, tc)
{
	int i;

	setup_lwp_context(test_bad_pending_robust_list);

	ATF_REQUIRE(_lwp_create(&lwp_data.context, 0, &lwp_data.lwpid) == 0);
	ATF_REQUIRE(_lwp_wait(lwp_data.lwpid, NULL) == 0);

	ATF_REQUIRE(lwp_data.set_robust_list_failed == false);

	for (i = 0; i < NLOCKS; i++) {
		ATF_REQUIRE((pos_locks[i].fword & FUTEX_TID_MASK) ==
		    lwp_data.threadid);
		ATF_REQUIRE((pos_locks[i].fword & FUTEX_OWNER_DIED) != 0);
	}
}

ATF_TC_CLEANUP(futex_robust_bad_pending, tc)
{
	do_cleanup();
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, futex_robust_positive);
	ATF_TP_ADD_TC(tp, futex_robust_negative);
	ATF_TP_ADD_TC(tp, futex_robust_unmapped);
	ATF_TP_ADD_TC(tp, futex_robust_evil_circular);
	ATF_TP_ADD_TC(tp, futex_robust_bad_pending);

	return atf_no_error();
}
