/* $NetBSD: t_futex_ops.c,v 1.5.2.1 2020/11/01 15:22:58 thorpej Exp $ */

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
__COPYRIGHT("@(#) Copyright (c) 2019, 2020\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_futex_ops.c,v 1.5.2.1 2020/11/01 15:22:58 thorpej Exp $");

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <atomic.h>
#include <errno.h>
#include <lwp.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <limits.h>
#include <sched.h>
#include <unistd.h>

#include <atf-c.h>

#include <libc/include/futex_private.h>

#define	LOAD(x)		(*(volatile int *)(x))
#define	STORE(x, y)	*(volatile int *)(x) = (y)

#if 0
#define	DPRINTF(x)	printf x
#else
#define	DPRINTF(x)	__nothing
#endif

#define	STACK_SIZE	65536

static volatile int futex_word;
static volatile int futex_word1;

static volatile unsigned int nlwps_running;

struct lwp_data {
	ucontext_t	context;
	void		(*func)(void *);
	void		*stack_base;
	lwpid_t		lwpid;
	pid_t		child;
	lwpid_t		threadid;
	int		wait_op;
	int		op_flags;
	int		bitset;
	volatile int	*futex_ptr;
	volatile int	*error_ptr;
	int		block_val;
	pri_t		rt_prio;

	void		(*exit_func)(void);

	int		futex_error;
};

#define	WAITER_LWP0		0
#define	WAITER_LWP1		1
#define	WAITER_LWP2		2
#define	WAITER_LWP3		3
#define	WAITER_LWP4		4
#define	WAITER_LWP5		5
#define	NLWPS			6

struct lwp_data lwp_data[NLWPS];

static const char *bs_path = "t_futex_ops_backing_store";
static int bs_fd = -1;
static int *bs_addr = MAP_FAILED;
static void *bs_source_buffer = NULL;
static void *bs_verify_buffer = NULL;
static long bs_pagesize;

static int pri_min;
static int pri_max;

static void
setup_rt_params(void)
{
	long pri;

	ATF_REQUIRE((pri = sysconf(_SC_SCHED_PRI_MIN)) != -1);
	pri_min = (int)pri;
	ATF_REQUIRE((pri = sysconf(_SC_SCHED_PRI_MAX)) != -1);
	pri_max = (int)pri;
}

static void
create_lwp_waiter(struct lwp_data *d)
{
	ATF_REQUIRE(_lwp_create(&d->context, 0, &d->lwpid) == 0);
}

static void
exit_lwp_waiter(void)
{
	_lwp_exit();
}

static void
reap_lwp_waiter(struct lwp_data *d)
{
	ATF_REQUIRE(_lwp_wait(d->lwpid, NULL) == 0);
}

static void
create_proc_waiter(struct lwp_data *d)
{
	pid_t pid;

	ATF_REQUIRE((pid = fork()) != -1);
	if (pid == 0) {
		(*d->func)(d);
		_exit(666);		/* backstop */
	} else
		d->child = pid;
}

static void
exit_proc_waiter(void)
{
	_exit(0);
}

static void
reap_proc_waiter(struct lwp_data *d)
{
	int status;

	ATF_REQUIRE(waitpid(d->child, &status, 0) == d->child);
	ATF_REQUIRE(WIFEXITED(status));
	ATF_REQUIRE(WEXITSTATUS(status) == 0);
}

static void
setup_lwp_context(struct lwp_data *d, void (*func)(void *))
{

	memset(d, 0, sizeof(*d));
	d->stack_base = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
	    MAP_ANON | MAP_STACK | MAP_PRIVATE, -1, 0);
	ATF_REQUIRE(d->stack_base != MAP_FAILED);
	_lwp_makecontext(&d->context, func, d, NULL, d->stack_base, STACK_SIZE);
	d->threadid = 0;
	d->func = func;
}

static void
simple_test_waiter_lwp(void *arg)
{
	struct lwp_data *d = arg;

	d->threadid = _lwp_self();

	membar_producer();
	atomic_inc_uint(&nlwps_running);
	membar_sync();

	if (__futex(d->futex_ptr, d->wait_op | d->op_flags,
		    d->block_val, NULL, NULL, 0, d->bitset) == -1) {
		d->futex_error = errno;
		membar_sync();
		atomic_dec_uint(&nlwps_running);
		_lwp_exit();
	} else {
		d->futex_error = 0;
	}

	membar_sync();
	atomic_dec_uint(&nlwps_running);

	_lwp_exit();
}

static void
rt_simple_test_waiter_lwp(void *arg)
{
	struct lwp_data *d = arg;
	struct sched_param sp;
	int policy;

	d->threadid = _lwp_self();

	ATF_REQUIRE(_sched_getparam(getpid(), d->threadid, &policy, &sp) == 0);
	policy = SCHED_RR;
	sp.sched_priority = d->rt_prio;
	ATF_REQUIRE(_sched_setparam(getpid(), d->threadid, policy, &sp) == 0);

	simple_test_waiter_lwp(arg);
}

static bool
verify_zero_bs(void)
{

	if (bs_verify_buffer == NULL) {
		bs_verify_buffer = malloc(bs_pagesize);
		ATF_REQUIRE(bs_verify_buffer != NULL);
	}

	ATF_REQUIRE(pread(bs_fd, bs_verify_buffer,
			  bs_pagesize, 0) == bs_pagesize);

	return (memcmp(bs_verify_buffer, bs_source_buffer, bs_pagesize) == 0);
}

static void
create_bs(int map_flags)
{

	bs_pagesize = sysconf(_SC_PAGESIZE);
	ATF_REQUIRE(bs_pagesize > 0);

	if ((map_flags & (MAP_FILE | MAP_ANON)) == MAP_FILE) {
		bs_source_buffer = calloc(1, bs_pagesize);
		ATF_REQUIRE(bs_source_buffer != NULL);

		bs_fd = open(bs_path, O_RDWR | O_CREAT | O_EXCL, 0644);
		ATF_REQUIRE(bs_fd != -1);

		ATF_REQUIRE(pwrite(bs_fd, bs_source_buffer,
				   bs_pagesize, 0) == bs_pagesize);
		ATF_REQUIRE(verify_zero_bs());
	}

	bs_addr = mmap(NULL, bs_pagesize, PROT_READ | PROT_WRITE,
		       map_flags | MAP_HASSEMAPHORE, bs_fd, 0);
	ATF_REQUIRE(bs_addr != MAP_FAILED);
}

static void
cleanup_bs(void)
{

	if (bs_fd != -1) {
		(void) close(bs_fd);
		bs_fd = -1;
		(void) unlink(bs_path);
	}
	if (bs_source_buffer != NULL) {
		free(bs_source_buffer);
		bs_source_buffer = NULL;
	}
	if (bs_verify_buffer != NULL) {
		free(bs_verify_buffer);
		bs_verify_buffer = NULL;
	}
	if (bs_addr != MAP_FAILED) {
		munmap(bs_addr, bs_pagesize);
		bs_addr = MAP_FAILED;
	}
}

static void
do_cleanup(void)
{
	int i;

	for (i = 0; i < NLWPS; i++) {
		struct lwp_data *d = &lwp_data[i];
		if (d->stack_base != NULL && d->stack_base != MAP_FAILED) {
			(void) munmap(d->stack_base, STACK_SIZE);
		}
	}
	memset(lwp_data, 0, sizeof(lwp_data));
	STORE(&futex_word, 0);
	STORE(&futex_word1, 0);
	nlwps_running = 0;

	cleanup_bs();
}

/*****************************************************************************/

static void
wait_wake_test_waiter_lwp(void *arg)
{
	struct lwp_data *d = arg;

	d->threadid = _lwp_self();

	STORE(d->futex_ptr, 1);
	membar_sync();

	/* This will block because *futex_ptr == 1. */
	if (__futex(d->futex_ptr, FUTEX_WAIT | d->op_flags,
		    1, NULL, NULL, 0, 0) == -1) {
		STORE(d->error_ptr, errno);
		(*d->exit_func)();
	} else {
		STORE(d->error_ptr, 0);
	}

	do {
		membar_sync();
		sleep(1);
	} while (LOAD(d->futex_ptr) != 0);

	STORE(d->futex_ptr, 2);
	membar_sync();

	do {
		membar_sync();
		sleep(1);
	} while (LOAD(d->futex_ptr) != 3);

	/* This will not block because futex_word != 666. */
	if (__futex(d->futex_ptr, FUTEX_WAIT | d->op_flags,
		    666, NULL, NULL, 0, 0) == -1) {
		/* This SHOULD be EAGAIN. */
		STORE(d->error_ptr, errno);
	}

	STORE(d->futex_ptr, 4);
	membar_sync();

	(*d->exit_func)();
}

static void
do_futex_wait_wake_test(volatile int *futex_ptr, volatile int *error_ptr,
			void (*create_func)(struct lwp_data *),
			void (*exit_func)(void),
			void (*reap_func)(struct lwp_data *),
			int flags)
{
	struct lwp_data *wlwp = &lwp_data[WAITER_LWP0];
	int tries;

	if (error_ptr == NULL)
		error_ptr = &wlwp->futex_error;

	if (create_func == NULL)
		create_func = create_lwp_waiter;
	if (exit_func == NULL)
		exit_func = exit_lwp_waiter;
	if (reap_func == NULL)
		reap_func = reap_lwp_waiter;

	setup_lwp_context(wlwp, wait_wake_test_waiter_lwp);

	DPRINTF(("futex_basic_wait_wake: testing with flags 0x%x\n", flags));
	wlwp->op_flags = flags;
	wlwp->error_ptr = error_ptr;
	STORE(error_ptr, -1);
	wlwp->futex_ptr = futex_ptr;
	STORE(futex_ptr, 0);
	wlwp->exit_func = exit_func;
	membar_sync();

	DPRINTF(("futex_basic_wait_wake: creating watier LWP\n"));
	(*create_func)(wlwp);

	DPRINTF(("futex_basic_wait_wake: waiting for LWP %d to enter futex\n",
	    wlwp->lwpid));
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (LOAD(futex_ptr) == 1)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE(LOAD(futex_ptr) == 1);

	/*
	 * If the LWP is blocked in the futex, it will not have yet
	 * modified *error_ptr.
	 */
	DPRINTF(("futex_basic_wait_wake: checking for successful wait (%d)\n",
	    LOAD(error_ptr)));
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (LOAD(error_ptr) == -1)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE(LOAD(error_ptr) == -1);

	/* Make sure invalid #wakes in rejected. */
	ATF_REQUIRE_ERRNO(EINVAL,
	    __futex(futex_ptr, FUTEX_WAKE | flags,
		    -1, NULL, NULL, 0, 0) == -1);

	DPRINTF(("futex_basic_wait_wake: waking 1 waiter\n"));
	ATF_REQUIRE(__futex(futex_ptr, FUTEX_WAKE | flags,
			    1, NULL, NULL, 0, 0) == 1);

	DPRINTF(("futex_basic_wait_wake: checking for successful wake (%d)\n",
	    LOAD(error_ptr)));
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (LOAD(error_ptr) == 0)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE(LOAD(error_ptr) == 0);

	STORE(futex_ptr, 0);
	membar_sync();

	DPRINTF(("futex_basic_wait_wake: waiting for LWP to advance (2)\n"));
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (LOAD(futex_ptr) == 2)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE(LOAD(futex_ptr) == 2);

	STORE(futex_ptr, 3);
	membar_sync();

	DPRINTF(("futex_basic_wait_wake: waiting for LWP to advance (4)\n"));
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (LOAD(futex_ptr) == 4)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE(LOAD(futex_ptr) == 4);

	DPRINTF(("futex_basic_wait_wake: checking for expected EGAIN\n"));
	ATF_REQUIRE(LOAD(error_ptr) == EAGAIN);

	DPRINTF(("futex_basic_wait_wake: reaping LWP %d\n", wlwp->lwpid));
	(*reap_func)(wlwp);
}

ATF_TC_WITH_CLEANUP(futex_basic_wait_wake_private);
ATF_TC_HEAD(futex_basic_wait_wake_private, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests basic futex WAIT + WAKE operations (PRIVATE)");
}
ATF_TC_BODY(futex_basic_wait_wake_private, tc)
{
	do_futex_wait_wake_test(&futex_word, NULL,
				NULL, NULL, NULL,
				FUTEX_PRIVATE_FLAG);
}
ATF_TC_CLEANUP(futex_basic_wait_wake_private, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_basic_wait_wake_shared);
ATF_TC_HEAD(futex_basic_wait_wake_shared, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests basic futex WAIT + WAKE operations (SHARED)");
}
ATF_TC_BODY(futex_basic_wait_wake_shared, tc)
{
	do_futex_wait_wake_test(&futex_word, NULL,
				NULL, NULL, NULL,
				0);
}
ATF_TC_CLEANUP(futex_basic_wait_wake_shared, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_wait_wake_anon_bs_private);
ATF_TC_HEAD(futex_wait_wake_anon_bs_private, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAIT + WAKE operations (MAP_ANON + PRIVATE)");
}
ATF_TC_BODY(futex_wait_wake_anon_bs_private, tc)
{
	create_bs(MAP_ANON | MAP_PRIVATE);
	do_futex_wait_wake_test(&bs_addr[0], NULL,
				NULL, NULL, NULL,
				FUTEX_PRIVATE_FLAG);
}
ATF_TC_CLEANUP(futex_wait_wake_anon_bs_private, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_wait_wake_anon_bs_shared);
ATF_TC_HEAD(futex_wait_wake_anon_bs_shared, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAIT + WAKE operations (MAP_ANON + SHARED)");
}
ATF_TC_BODY(futex_wait_wake_anon_bs_shared, tc)
{
	create_bs(MAP_ANON | MAP_PRIVATE);
	do_futex_wait_wake_test(&bs_addr[0], NULL,
				NULL, NULL, NULL,
				0);
}
ATF_TC_CLEANUP(futex_wait_wake_anon_bs_shared, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_wait_wake_file_bs_private);
ATF_TC_HEAD(futex_wait_wake_file_bs_private, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAIT + WAKE operations (MAP_FILE + PRIVATE)");
}
ATF_TC_BODY(futex_wait_wake_file_bs_private, tc)
{
	/*
	 * This combination (non-COW mapped file + PRIVATE futex)
	 * doesn't really make sense, but we should make sure it
	 * works as expected.
	 */
	create_bs(MAP_FILE | MAP_SHARED);
	do_futex_wait_wake_test(&bs_addr[0], NULL,
				NULL, NULL, NULL,
				FUTEX_PRIVATE_FLAG);
	ATF_REQUIRE(! verify_zero_bs());
}
ATF_TC_CLEANUP(futex_wait_wake_file_bs_private, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_wait_wake_file_bs_cow_private);
ATF_TC_HEAD(futex_wait_wake_file_bs_cow_private, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAIT + WAKE operations (MAP_FILE COW + PRIVATE)");
}
ATF_TC_BODY(futex_wait_wake_file_bs_cow_private, tc)
{
	create_bs(MAP_FILE | MAP_PRIVATE);
	do_futex_wait_wake_test(&bs_addr[0], NULL,
				NULL, NULL, NULL,
				FUTEX_PRIVATE_FLAG);
	ATF_REQUIRE(verify_zero_bs());
}
ATF_TC_CLEANUP(futex_wait_wake_file_bs_cow_private, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_wait_wake_file_bs_shared);
ATF_TC_HEAD(futex_wait_wake_file_bs_shared, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAIT + WAKE operations (MAP_FILE + SHARED)");
}
ATF_TC_BODY(futex_wait_wake_file_bs_shared, tc)
{
	create_bs(MAP_FILE | MAP_SHARED);
	do_futex_wait_wake_test(&bs_addr[0], NULL,
				NULL, NULL, NULL,
				0);
	ATF_REQUIRE(! verify_zero_bs());
}
ATF_TC_CLEANUP(futex_wait_wake_file_bs_shared, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_wait_wake_file_bs_cow_shared);
ATF_TC_HEAD(futex_wait_wake_file_bs_cow_shared, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAIT + WAKE operations (MAP_FILE COW + SHARED)");
}
ATF_TC_BODY(futex_wait_wake_file_bs_cow_shared, tc)
{
	/*
	 * This combination (COW mapped file + SHARED futex)
	 * doesn't really make sense, but we should make sure it
	 * works as expected.
	 */
	create_bs(MAP_FILE | MAP_PRIVATE);
	do_futex_wait_wake_test(&bs_addr[0], NULL,
				NULL, NULL, NULL,
				0);
	ATF_REQUIRE(verify_zero_bs());
}
ATF_TC_CLEANUP(futex_wait_wake_file_bs_cow_shared, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_wait_wake_anon_bs_shared_proc);
ATF_TC_HEAD(futex_wait_wake_anon_bs_shared_proc, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests multiproc futex WAIT + WAKE operations (MAP_ANON + SHARED)");
}
ATF_TC_BODY(futex_wait_wake_anon_bs_shared_proc, tc)
{
	create_bs(MAP_ANON | MAP_SHARED);
	do_futex_wait_wake_test(&bs_addr[0], &bs_addr[1],
				create_proc_waiter,
				exit_proc_waiter,
				reap_proc_waiter,
				0);
}
ATF_TC_CLEANUP(futex_wait_wake_anon_bs_shared_proc, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_wait_wake_file_bs_shared_proc);
ATF_TC_HEAD(futex_wait_wake_file_bs_shared_proc, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests multiproc futex WAIT + WAKE operations (MAP_ANON + SHARED)");
}
ATF_TC_BODY(futex_wait_wake_file_bs_shared_proc, tc)
{
	create_bs(MAP_FILE | MAP_SHARED);
	do_futex_wait_wake_test(&bs_addr[0], &bs_addr[1],
				create_proc_waiter,
				exit_proc_waiter,
				reap_proc_waiter,
				0);
}
ATF_TC_CLEANUP(futex_wait_wake_file_bs_shared_proc, tc)
{
	do_cleanup();
}

/*****************************************************************************/

ATF_TC(futex_wait_pointless_bitset);
ATF_TC_HEAD(futex_wait_pointless_bitset, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests basic futex WAIT + WAKE operations (SHARED)");
}
ATF_TC_BODY(futex_wait_pointless_bitset, tc)
{

	futex_word = 1;
	ATF_REQUIRE_ERRNO(EINVAL,
	    __futex(&futex_word, FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG,
		1, NULL, NULL, 0, 0) == -1);
}

static void
do_futex_wait_wake_bitset_test(int flags)
{
	struct lwp_data *wlwp0 = &lwp_data[WAITER_LWP0];
	struct lwp_data *wlwp1 = &lwp_data[WAITER_LWP1];
	int i, tries;

	for (i = WAITER_LWP0; i <= WAITER_LWP1; i++) {
		setup_lwp_context(&lwp_data[i], simple_test_waiter_lwp);
		lwp_data[i].op_flags = flags;
		lwp_data[i].futex_error = -1;
		lwp_data[i].bitset = __BIT(i);
		lwp_data[i].wait_op = FUTEX_WAIT_BITSET;
		lwp_data[i].futex_ptr = &futex_word;
		lwp_data[i].block_val = 1;
	}

	STORE(&futex_word, 1);
	membar_sync();

	ATF_REQUIRE(_lwp_create(&wlwp0->context, 0, &wlwp0->lwpid) == 0);
	ATF_REQUIRE(_lwp_create(&wlwp1->context, 0, &wlwp1->lwpid) == 0);

	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 2)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 2, "waiters failed to start");

	/* Ensure they're blocked. */
	ATF_REQUIRE(wlwp0->futex_error == -1);
	ATF_REQUIRE(wlwp1->futex_error == -1);

	/* Make sure invalid #wakes in rejected. */
	ATF_REQUIRE_ERRNO(EINVAL,
	    __futex(&futex_word, FUTEX_WAKE_BITSET | flags,
		    -1, NULL, NULL, 0, 0) == -1);

	/* This should result in no wakeups because no bits are set. */
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_BITSET | flags,
			    INT_MAX, NULL, NULL, 0, 0) == 0);

	/* This should result in no wakeups because the wrongs bits are set. */
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_BITSET | flags,
			    INT_MAX, NULL, NULL, 0,
			    ~(wlwp0->bitset | wlwp1->bitset)) == 0);

	/* Trust, but verify. */
	sleep(1);
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 2)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 2, "waiters exited unexpectedly");

	/* Wake up the first LWP. */
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_BITSET | flags,
			    INT_MAX, NULL, NULL, 0,
			    wlwp0->bitset) == 1);
	sleep(1);
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 1)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE(nlwps_running == 1);
	ATF_REQUIRE(wlwp0->futex_error == 0);
	ATF_REQUIRE(_lwp_wait(wlwp0->lwpid, NULL) == 0);

	/* Wake up the second LWP. */
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_BITSET | flags,
			    INT_MAX, NULL, NULL, 0,
			    wlwp1->bitset) == 1);
	sleep(1);
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 0)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE(nlwps_running == 0);
	ATF_REQUIRE(wlwp1->futex_error == 0);
	ATF_REQUIRE(_lwp_wait(wlwp1->lwpid, NULL) == 0);
}

ATF_TC_WITH_CLEANUP(futex_wait_wake_bitset);
ATF_TC_HEAD(futex_wait_wake_bitset, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAIT_BITSET + WAKE_BITSET operations");
}
ATF_TC_BODY(futex_wait_wake_bitset, tc)
{
	do_futex_wait_wake_bitset_test(FUTEX_PRIVATE_FLAG);
}
ATF_TC_CLEANUP(futex_wait_wake_bitset, tc)
{
	do_cleanup();
}

/*****************************************************************************/

static void
do_futex_requeue_test(int flags, int op)
{
	struct lwp_data *wlwp0 = &lwp_data[WAITER_LWP0];
	struct lwp_data *wlwp1 = &lwp_data[WAITER_LWP1];
	struct lwp_data *wlwp2 = &lwp_data[WAITER_LWP2];
	struct lwp_data *wlwp3 = &lwp_data[WAITER_LWP3];
	const int good_val3 = (op == FUTEX_CMP_REQUEUE) ?   1 : 0;
	const int bad_val3  = (op == FUTEX_CMP_REQUEUE) ? 666 : 0;
	int i, tries;

	for (i = WAITER_LWP0; i <= WAITER_LWP3; i++) {
		setup_lwp_context(&lwp_data[i], simple_test_waiter_lwp);
		lwp_data[i].op_flags = flags;
		lwp_data[i].futex_error = -1;
		lwp_data[i].futex_ptr = &futex_word;
		lwp_data[i].block_val = 1;
		lwp_data[i].bitset = 0;
		lwp_data[i].wait_op = FUTEX_WAIT;
	}

	STORE(&futex_word, 1);
	STORE(&futex_word1, 1);
	membar_sync();

	ATF_REQUIRE(_lwp_create(&wlwp0->context, 0, &wlwp0->lwpid) == 0);
	ATF_REQUIRE(_lwp_create(&wlwp1->context, 0, &wlwp1->lwpid) == 0);
	ATF_REQUIRE(_lwp_create(&wlwp2->context, 0, &wlwp2->lwpid) == 0);
	ATF_REQUIRE(_lwp_create(&wlwp3->context, 0, &wlwp3->lwpid) == 0);

	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 4)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 4, "waiters failed to start");

	/* Ensure they're blocked. */
	ATF_REQUIRE(wlwp0->futex_error == -1);
	ATF_REQUIRE(wlwp1->futex_error == -1);
	ATF_REQUIRE(wlwp2->futex_error == -1);
	ATF_REQUIRE(wlwp3->futex_error == -1);

	/* Make sure invalid #wakes and #requeues are rejected. */
	ATF_REQUIRE_ERRNO(EINVAL,
	    __futex(&futex_word, op | flags,
		    -1, NULL, &futex_word1, INT_MAX, bad_val3) == -1);

	ATF_REQUIRE_ERRNO(EINVAL,
	    __futex(&futex_word, op | flags,
		    0, NULL, &futex_word1, -1, bad_val3) == -1);

	/*
	 * FUTEX 0: 4 LWPs
	 * FUTEX 1: 0 LWPs
	 */

	if (op == FUTEX_CMP_REQUEUE) {
		/* This should fail because the futex_word value is 1. */
		ATF_REQUIRE_ERRNO(EAGAIN,
		    __futex(&futex_word, op | flags,
			    0, NULL, &futex_word1, INT_MAX, bad_val3) == -1);
	}

	/*
	 * FUTEX 0: 4 LWPs
	 * FUTEX 1: 0 LWPs
	 */

	/* Move all waiters from 0 to 1. */
	ATF_REQUIRE(__futex(&futex_word, op | flags,
			    0, NULL, &futex_word1, INT_MAX, good_val3) == 4);

	/*
	 * FUTEX 0: 0 LWPs
	 * FUTEX 1: 4 LWPs
	 */

	if (op == FUTEX_CMP_REQUEUE) {
		/* This should fail because the futex_word1 value is 1. */
		ATF_REQUIRE_ERRNO(EAGAIN,
		    __futex(&futex_word1, op | flags,
			    1, NULL, &futex_word, 1, bad_val3) == -1);
	}

	/*
	 * FUTEX 0: 0 LWPs
	 * FUTEX 1: 4 LWPs
	 */

	/* Wake one waiter on 1, move one waiter to 0. */
	ATF_REQUIRE(__futex(&futex_word1, op | flags,
			    1, NULL, &futex_word, 1, good_val3) == 2);

	/*
	 * FUTEX 0: 1 LWP
	 * FUTEX 1: 2 LWPs
	 */

	/* Wake all waiters on 0 (should be 1). */
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE | flags,
			    INT_MAX, NULL, NULL, 0, 0) == 1);

	/* Wake all waiters on 1 (should be 2). */
	ATF_REQUIRE(__futex(&futex_word1, FUTEX_WAKE | flags,
			    INT_MAX, NULL, NULL, 0, 0) == 2);

	/* Trust, but verify. */
	sleep(1);
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 0)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 0, "waiters failed to exit");

	ATF_REQUIRE(_lwp_wait(wlwp0->lwpid, NULL) == 0);
	ATF_REQUIRE(_lwp_wait(wlwp1->lwpid, NULL) == 0);
	ATF_REQUIRE(_lwp_wait(wlwp2->lwpid, NULL) == 0);
	ATF_REQUIRE(_lwp_wait(wlwp3->lwpid, NULL) == 0);
}

ATF_TC_WITH_CLEANUP(futex_requeue);
ATF_TC_HEAD(futex_requeue, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex REQUEUE operations");
}
ATF_TC_BODY(futex_requeue, tc)
{
	do_futex_requeue_test(FUTEX_PRIVATE_FLAG, FUTEX_REQUEUE);
}
ATF_TC_CLEANUP(futex_requeue, tc)
{
	do_cleanup();
}

ATF_TC_WITH_CLEANUP(futex_cmp_requeue);
ATF_TC_HEAD(futex_cmp_requeue, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex CMP_REQUEUE operations");
}
ATF_TC_BODY(futex_cmp_requeue, tc)
{
	do_futex_requeue_test(FUTEX_PRIVATE_FLAG, FUTEX_CMP_REQUEUE);
}
ATF_TC_CLEANUP(futex_cmp_requeue, tc)
{
	do_cleanup();
}

/*****************************************************************************/

static void
do_futex_wake_op_op_test(int flags)
{
	int op;

	futex_word = 0;
	futex_word1 = 0;

	/*
	 * The op= operations should work even if there are no waiters.
	 */

	/*
	 * Because these operations use both futex addresses, exercise
	 * rejecting unaligned futex addresses here.
	 */
	op = FUTEX_OP(FUTEX_OP_SET, 1, FUTEX_OP_CMP_EQ, 0);
	ATF_REQUIRE_ERRNO(EINVAL,
	    __futex((int *)1, FUTEX_WAKE_OP | flags,
		    0, NULL, &futex_word1, 0, op) == -1);
	ATF_REQUIRE(futex_word1 == 0);

	ATF_REQUIRE_ERRNO(EINVAL,
	    __futex(&futex_word, FUTEX_WAKE_OP | flags,
		    0, NULL, (int *)1, 0, op) == -1);
	ATF_REQUIRE(futex_word == 0);

	/* Check unmapped uaddr2 handling, too. */
	ATF_REQUIRE_ERRNO(EFAULT,
	    __futex(&futex_word, FUTEX_WAKE_OP | flags,
		    0, NULL, NULL, 0, op) == -1);
	ATF_REQUIRE(futex_word == 0);

	op = FUTEX_OP(FUTEX_OP_SET, 1, FUTEX_OP_CMP_EQ, 0);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 0, op) == 0);
	ATF_REQUIRE(futex_word1 == 1);

	op = FUTEX_OP(FUTEX_OP_ADD, 1, FUTEX_OP_CMP_EQ, 0);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 0, op) == 0);
	ATF_REQUIRE(futex_word1 == 2);

	op = FUTEX_OP(FUTEX_OP_OR, 2, FUTEX_OP_CMP_EQ, 0);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 0, op) == 0);
	ATF_REQUIRE(futex_word1 == 2);

	/* This should fail because of invalid shift value 32. */
	op = FUTEX_OP(FUTEX_OP_OR | FUTEX_OP_OPARG_SHIFT, 32,
		      FUTEX_OP_CMP_EQ, 0);
	ATF_REQUIRE_ERRNO(EINVAL,
	    __futex(&futex_word, FUTEX_WAKE_OP | flags,
		    0, NULL, &futex_word1, 0, op) == -1);
	ATF_REQUIRE(futex_word1 == 2);

	op = FUTEX_OP(FUTEX_OP_OR | FUTEX_OP_OPARG_SHIFT, 31,
		      FUTEX_OP_CMP_EQ, 0);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 0, op) == 0);
	ATF_REQUIRE(futex_word1 == (int)0x80000002);

	op = FUTEX_OP(FUTEX_OP_ANDN | FUTEX_OP_OPARG_SHIFT, 31,
		      FUTEX_OP_CMP_EQ, 0);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 0, op) == 0);
	ATF_REQUIRE(futex_word1 == 2);

	op = FUTEX_OP(FUTEX_OP_XOR, 2, FUTEX_OP_CMP_EQ, 0);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 0, op) == 0);
	ATF_REQUIRE(futex_word1 == 0);
}

ATF_TC_WITH_CLEANUP(futex_wake_op_op);
ATF_TC_HEAD(futex_wake_op_op, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAKE_OP OP operations");
}
ATF_TC_BODY(futex_wake_op_op, tc)
{
	do_futex_wake_op_op_test(FUTEX_PRIVATE_FLAG);
}
ATF_TC_CLEANUP(futex_wake_op_op, tc)
{
	do_cleanup();
}

static void
create_wake_op_test_lwps(int flags)
{
	int i;

	futex_word1 = 0;
	membar_sync();

	for (i = WAITER_LWP0; i <= WAITER_LWP5; i++) {
		setup_lwp_context(&lwp_data[i], simple_test_waiter_lwp);
		lwp_data[i].op_flags = flags;
		lwp_data[i].futex_error = -1;
		lwp_data[i].futex_ptr = &futex_word1;
		lwp_data[i].block_val = 0;
		lwp_data[i].bitset = 0;
		lwp_data[i].wait_op = FUTEX_WAIT;
		ATF_REQUIRE(_lwp_create(&lwp_data[i].context, 0,
					&lwp_data[i].lwpid) == 0);
	}

	for (i = 0; i < 5; i++) {
		membar_sync();
		if (nlwps_running == 6)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 6, "waiters failed to start");

	/* Ensure they're blocked. */
	for (i = WAITER_LWP0; i <= WAITER_LWP5; i++) {
		ATF_REQUIRE(lwp_data[i].futex_error == -1);
	}
}

static void
reap_wake_op_test_lwps(void)
{
	int i;

	for (i = WAITER_LWP0; i <= WAITER_LWP5; i++) {
		ATF_REQUIRE(_lwp_wait(lwp_data[i].lwpid, NULL) == 0);
	}
}

static void
do_futex_wake_op_cmp_test(int flags)
{
	int tries, op;

	futex_word = 0;
	membar_sync();

	/*
	 * Verify and negative and positive for each individual
	 * compare.
	 */

	create_wake_op_test_lwps(flags);

	/* #LWPs = 6 */
	op = FUTEX_OP(FUTEX_OP_SET, 0, FUTEX_OP_CMP_EQ, 1);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 1, op) == 0);
	ATF_REQUIRE(futex_word1 == 0);

	op = FUTEX_OP(FUTEX_OP_SET, 1, FUTEX_OP_CMP_EQ, 0);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 1, op) == 1);
	ATF_REQUIRE(futex_word1 == 1);

	/* #LWPs = 5 */
	op = FUTEX_OP(FUTEX_OP_SET, 1, FUTEX_OP_CMP_NE, 1);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 1, op) == 0);
	ATF_REQUIRE(futex_word1 == 1);

	op = FUTEX_OP(FUTEX_OP_SET, 2, FUTEX_OP_CMP_NE, 2);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 1, op) == 1);
	ATF_REQUIRE(futex_word1 == 2);

	/* #LWPs = 4 */
	op = FUTEX_OP(FUTEX_OP_SET, 2, FUTEX_OP_CMP_LT, 2);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 1, op) == 0);
	ATF_REQUIRE(futex_word1 == 2);

	op = FUTEX_OP(FUTEX_OP_SET, 2, FUTEX_OP_CMP_LT, 3);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 1, op) == 1);
	ATF_REQUIRE(futex_word1 == 2);

	/* #LWPs = 3 */
	op = FUTEX_OP(FUTEX_OP_SET, 1, FUTEX_OP_CMP_LE, 1);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 1, op) == 0);
	ATF_REQUIRE(futex_word1 == 1);

	op = FUTEX_OP(FUTEX_OP_SET, 1, FUTEX_OP_CMP_LE, 1);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 1, op) == 1);
	ATF_REQUIRE(futex_word1 == 1);

	/* #LWPs = 2 */
	op = FUTEX_OP(FUTEX_OP_SET, 3, FUTEX_OP_CMP_GT, 3);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 1, op) == 0);
	ATF_REQUIRE(futex_word1 == 3);

	op = FUTEX_OP(FUTEX_OP_SET, 2, FUTEX_OP_CMP_GT, 2);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 1, op) == 1);
	ATF_REQUIRE(futex_word1 == 2);

	/* #LWPs = 1 */
	op = FUTEX_OP(FUTEX_OP_SET, 3, FUTEX_OP_CMP_GE, 4);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 1, op) == 0);
	ATF_REQUIRE(futex_word1 == 3);
	
	op = FUTEX_OP(FUTEX_OP_SET, 2, FUTEX_OP_CMP_GE, 3);
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE_OP | flags,
			    0, NULL, &futex_word1, 1, op) == 1);
	ATF_REQUIRE(futex_word1 == 2);

	/* #LWPs = 0 */

	/* Trust, but verify. */
	sleep(1);
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 0)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 0, "waiters failed to exit");

	reap_wake_op_test_lwps();

	/*
	 * Verify wakes on uaddr work even if the uaddr2 comparison
	 * fails.
	 */

	create_wake_op_test_lwps(flags);

	/* #LWPs = 6 */
	ATF_REQUIRE(futex_word == 0);
	op = FUTEX_OP(FUTEX_OP_SET, 0, FUTEX_OP_CMP_EQ, 666);
	ATF_REQUIRE(__futex(&futex_word1, FUTEX_WAKE_OP | flags,
			    INT_MAX, NULL, &futex_word, 0, op) == 6);
	ATF_REQUIRE(futex_word == 0);

	/* #LWPs = 0 */

	/* Trust, but verify. */
	sleep(1);
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 0)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 0, "waiters failed to exit");

	reap_wake_op_test_lwps();
}

ATF_TC_WITH_CLEANUP(futex_wake_op_cmp);
ATF_TC_HEAD(futex_wake_op_cmp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAKE_OP CMP operations");
}
ATF_TC_BODY(futex_wake_op_cmp, tc)
{
	do_futex_wake_op_cmp_test(FUTEX_PRIVATE_FLAG);
}
ATF_TC_CLEANUP(futex_wake_op_cmp, tc)
{
	do_cleanup();
}

/*****************************************************************************/

static void
do_futex_wait_timeout(bool relative, clockid_t clock)
{
	struct timespec ts;
	struct timespec deadline;
	int op = relative ? FUTEX_WAIT : FUTEX_WAIT_BITSET;

	if (clock == CLOCK_REALTIME)
		op |= FUTEX_CLOCK_REALTIME;

	ATF_REQUIRE(clock_gettime(clock, &deadline) == 0);
	deadline.tv_sec += 2;
	if (relative) {
		ts.tv_sec = 2;
		ts.tv_nsec = 0;
	} else {
		ts = deadline;
	}

	futex_word = 1;
	ATF_REQUIRE_ERRNO(ETIMEDOUT,
	    __futex(&futex_word, op | FUTEX_PRIVATE_FLAG,
		    1, &ts, NULL, 0, FUTEX_BITSET_MATCH_ANY) == -1);

	/* Can't reliably check CLOCK_REALTIME in the presence of NTP. */
	if (clock != CLOCK_REALTIME) {
		ATF_REQUIRE(clock_gettime(clock, &ts) == 0);
		ATF_REQUIRE(ts.tv_sec >= deadline.tv_sec);
		ATF_REQUIRE(ts.tv_sec > deadline.tv_sec ||
			    ts.tv_nsec >= deadline.tv_nsec);
	}
}

ATF_TC(futex_wait_timeout_relative);
ATF_TC_HEAD(futex_wait_timeout_relative, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAIT with relative timeout");
}
ATF_TC_BODY(futex_wait_timeout_relative, tc)
{
	do_futex_wait_timeout(true, CLOCK_MONOTONIC);
}

ATF_TC(futex_wait_timeout_relative_rt);
ATF_TC_HEAD(futex_wait_timeout_relative_rt, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAIT with relative timeout (REALTIME)");
}
ATF_TC_BODY(futex_wait_timeout_relative_rt, tc)
{
	do_futex_wait_timeout(true, CLOCK_REALTIME);
}

ATF_TC(futex_wait_timeout_deadline);
ATF_TC_HEAD(futex_wait_timeout_deadline, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAIT with absolute deadline");
}
ATF_TC_BODY(futex_wait_timeout_deadline, tc)
{
	do_futex_wait_timeout(false, CLOCK_MONOTONIC);
}

ATF_TC(futex_wait_timeout_deadline_rt);
ATF_TC_HEAD(futex_wait_timeout_deadline_rt, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAIT with absolute deadline (REALTIME)");
}
ATF_TC_BODY(futex_wait_timeout_deadline_rt, tc)
{
	do_futex_wait_timeout(false, CLOCK_REALTIME);
}

/*****************************************************************************/

static void
sig_noop(int sig __unused)
{
}

static void (*old_act)(int) = SIG_DFL;

static void
do_futex_wait_evil_unmapped(int map_flags)
{
	int i;

	create_bs(map_flags);

	old_act = signal(SIGUSR1, sig_noop);
	ATF_REQUIRE(old_act != SIG_ERR);

	setup_lwp_context(&lwp_data[0], simple_test_waiter_lwp);
	lwp_data[0].op_flags = 0;
	lwp_data[0].futex_error = -1;
	lwp_data[0].futex_ptr = &bs_addr[0];
	lwp_data[0].block_val = 0;
	lwp_data[0].bitset = 0;
	lwp_data[0].wait_op = FUTEX_WAIT;
	ATF_REQUIRE(_lwp_create(&lwp_data[0].context, 0,
				&lwp_data[0].lwpid) == 0);

	for (i = 0; i < 5; i++) {
		membar_sync();
		if (nlwps_running == 1)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 1, "waiters failed to start");

	/* Ensure it's blocked. */
	ATF_REQUIRE(lwp_data[0].futex_error == -1);

	/* Rudely unmap the backing store. */
	cleanup_bs();

	/* Signal the waiter so that it leaves the futex. */
	ATF_REQUIRE(_lwp_kill(lwp_data[0].threadid, SIGUSR1) == 0);

	/* Yay! No panic! */

	reap_lwp_waiter(&lwp_data[0]);
}

ATF_TC_WITH_CLEANUP(futex_wait_evil_unmapped_anon);
ATF_TC_HEAD(futex_wait_evil_unmapped_anon, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests futex WAIT while futex is unmapped - anon memory");
}
ATF_TC_BODY(futex_wait_evil_unmapped_anon, tc)
{
	do_futex_wait_evil_unmapped(MAP_ANON);
}
ATF_TC_CLEANUP(futex_wait_evil_unmapped_anon, tc)
{
	signal(SIGUSR1, old_act);
	do_cleanup();
}

/*****************************************************************************/

static void
do_test_wake_highest_pri(void)
{
	lwpid_t waiter;
	int tries;

	setup_rt_params();

	futex_word = 0;
	membar_sync();

	setup_lwp_context(&lwp_data[0], rt_simple_test_waiter_lwp);
	lwp_data[0].rt_prio = pri_min;
	lwp_data[0].op_flags = FUTEX_PRIVATE_FLAG;
	lwp_data[0].futex_error = -1;
	lwp_data[0].futex_ptr = &futex_word;
	lwp_data[0].block_val = 0;
	lwp_data[0].bitset = 0;
	lwp_data[0].wait_op = FUTEX_WAIT;
	ATF_REQUIRE(_lwp_create(&lwp_data[0].context, 0,
				&lwp_data[0].lwpid) == 0);

	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 1)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 1, "lowpri waiter failed to start");

	/* Ensure it's blocked. */
	ATF_REQUIRE(lwp_data[0].futex_error == -1);

	setup_lwp_context(&lwp_data[1], rt_simple_test_waiter_lwp);
	lwp_data[1].rt_prio = pri_max;
	lwp_data[1].op_flags = FUTEX_PRIVATE_FLAG;
	lwp_data[1].futex_error = -1;
	lwp_data[1].futex_ptr = &futex_word;
	lwp_data[1].block_val = 0;
	lwp_data[1].bitset = 0;
	lwp_data[1].wait_op = FUTEX_WAIT;
	ATF_REQUIRE(_lwp_create(&lwp_data[1].context, 0,
				&lwp_data[1].lwpid) == 0);

	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 2)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 2, "highpri waiter failed to start");

	/* Ensure it's blocked. */
	ATF_REQUIRE(lwp_data[1].futex_error == -1);

	/* Wake the first LWP.  We should get the highpri thread. */
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE | FUTEX_PRIVATE_FLAG,
			    1, NULL, NULL, 0, 0) == 1);
	sleep(1);
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 1)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE(nlwps_running == 1);
	ATF_REQUIRE(_lwp_wait(0, &waiter) == 0);
	ATF_REQUIRE(waiter == lwp_data[1].threadid);

	/* Wake the second LWP.  We should get the lowpri thread. */
	ATF_REQUIRE(__futex(&futex_word, FUTEX_WAKE | FUTEX_PRIVATE_FLAG,
			    1, NULL, NULL, 0, 0) == 1);
	sleep(1);
	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 0)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE(nlwps_running == 0);
	ATF_REQUIRE(_lwp_wait(0, &waiter) == 0);
	ATF_REQUIRE(waiter == lwp_data[0].threadid);
}

ATF_TC_WITH_CLEANUP(futex_wake_highest_pri);
ATF_TC_HEAD(futex_wake_highest_pri, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests that futex WAKE wakes the highest priority waiter");
	atf_tc_set_md_var(tc, "require.user", "root");
}
ATF_TC_BODY(futex_wake_highest_pri, tc)
{
	do_test_wake_highest_pri();
}
ATF_TC_CLEANUP(futex_wake_highest_pri, tc)
{
	do_cleanup();
}

/*****************************************************************************/

static void
do_test_rw_handoff_read(void)
{
	int i, tries;
	int rv;

	futex_word = FUTEX_WAITERS;
	membar_sync();

	for (i = 0; i < 3; i++) {
		setup_lwp_context(&lwp_data[i], simple_test_waiter_lwp);
		lwp_data[i].op_flags = FUTEX_PRIVATE_FLAG;
		lwp_data[i].futex_error = -1;
		lwp_data[i].futex_ptr = &futex_word;
		lwp_data[i].block_val = futex_word;
		lwp_data[i].bitset = FUTEX_RW_READER;
		lwp_data[i].wait_op = FUTEX_NETBSD_RW_WAIT;
		ATF_REQUIRE(_lwp_create(&lwp_data[i].context, 0,
					&lwp_data[i].lwpid) == 0);
	}

	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 3)
			break;
		sleep(1);
	}
	membar_sync();

	ATF_REQUIRE_EQ_MSG(nlwps_running, 3, "read-waiters failed to start");

	/* Ensure they're all blocked. */
	ATF_REQUIRE(lwp_data[0].futex_error == -1);
	ATF_REQUIRE(lwp_data[1].futex_error == -1);
	ATF_REQUIRE(lwp_data[2].futex_error == -1);

	/* Ensure a regular wake errors out. */
	rv = __futex(&futex_word,
		     FUTEX_WAKE | FUTEX_PRIVATE_FLAG, INT_MAX, NULL,
		     NULL, 0, 0);
	ATF_REQUIRE(rv == -1 && errno == EINVAL);

	/*
	 * Issue a hand-off.  It should wake all 3 readers and update
	 * the futex word.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    FUTEX_WAITERS, NULL, NULL, 0, 0) == 3);
	ATF_REQUIRE(futex_word == 3);

	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 0)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 0, "read-waiters failed to exit");

	/* Ensure they all exited error-free. */
	ATF_REQUIRE(lwp_data[0].futex_error == 0);
	reap_lwp_waiter(&lwp_data[0]);

	ATF_REQUIRE(lwp_data[1].futex_error == 0);
	reap_lwp_waiter(&lwp_data[1]);

	ATF_REQUIRE(lwp_data[2].futex_error == 0);
	reap_lwp_waiter(&lwp_data[2]);
}

ATF_TC_WITH_CLEANUP(futex_rw_handoff_read);
ATF_TC_HEAD(futex_rw_handoff_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests rwlock direct hand-off to readers");
}
ATF_TC_BODY(futex_rw_handoff_read, tc)
{
	atf_tc_skip("futex_rw_handoff is currently broken");
	do_test_rw_handoff_read();
}
ATF_TC_CLEANUP(futex_rw_handoff_read, tc)
{
	do_cleanup();
}

/*****************************************************************************/

static void
do_test_rw_handoff_write(void)
{
	unsigned int i, tries;
	lwpid_t lid;

	/*
	 * The kernel should not care about the WRITE_WANTED bit, and
	 * should use the contents of the sleepqs as the truth.
	 */
	futex_word = FUTEX_WAITERS;
	membar_sync();

	for (i = 0; i < 3; i++) {
		setup_lwp_context(&lwp_data[i], simple_test_waiter_lwp);
		lwp_data[i].op_flags = FUTEX_PRIVATE_FLAG;
		lwp_data[i].futex_error = -1;
		lwp_data[i].futex_ptr = &futex_word;
		lwp_data[i].block_val = futex_word;
		lwp_data[i].bitset = FUTEX_RW_WRITER;
		lwp_data[i].wait_op = FUTEX_NETBSD_RW_WAIT;
		ATF_REQUIRE(_lwp_create(&lwp_data[i].context, 0,
					&lwp_data[i].lwpid) == 0);

		/*
		 * Wait for each one to start in-turn, because we want
		 * to know the order in which the LWPs block on the futex.
		 */
		for (tries = 0; tries < 5; tries++) {
			membar_sync();
			if (nlwps_running == i + 1)
				break;
			sleep(1);
		}
		membar_sync();
	}

	ATF_REQUIRE_EQ_MSG(nlwps_running, 3, "write-waiters failed to start");

	/* Ensure they're all blocked. */
	ATF_REQUIRE(lwp_data[0].futex_error == -1);
	ATF_REQUIRE(lwp_data[1].futex_error == -1);
	ATF_REQUIRE(lwp_data[2].futex_error == -1);

	/*
	 * Issue a hand-off for each waiter.  This should awaken one
	 * at a time and update the futex word.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    FUTEX_WAITERS, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_LOCKED);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_WANTED);
	ATF_REQUIRE(futex_word & FUTEX_WAITERS);
	lid = futex_word & FUTEX_TID_MASK;
	ATF_REQUIRE(lid == lwp_data[0].lwpid);

	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_LOCKED);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_WANTED);
	ATF_REQUIRE(futex_word & FUTEX_WAITERS);
	lid = futex_word & FUTEX_TID_MASK;
	ATF_REQUIRE(lid == lwp_data[1].lwpid);

	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_LOCKED);
	ATF_REQUIRE((futex_word & FUTEX_RW_WRITE_WANTED) == 0);
	ATF_REQUIRE((futex_word & FUTEX_WAITERS) == 0);
	lid = futex_word & FUTEX_TID_MASK;
	ATF_REQUIRE(lid == lwp_data[2].lwpid);

	/*
	 * Issue one final hand-off; it should result in a fully
	 * released lock word.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 0);
	ATF_REQUIRE(futex_word == 0);

	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 0)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 0, "read-waiters failed to exit");

	/* Ensure they all exited error-free. */
	ATF_REQUIRE(lwp_data[0].futex_error == 0);
	reap_lwp_waiter(&lwp_data[0]);

	ATF_REQUIRE(lwp_data[1].futex_error == 0);
	reap_lwp_waiter(&lwp_data[1]);

	ATF_REQUIRE(lwp_data[2].futex_error == 0);
	reap_lwp_waiter(&lwp_data[2]);
}

ATF_TC_WITH_CLEANUP(futex_rw_handoff_write);
ATF_TC_HEAD(futex_rw_handoff_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests rwlock direct hand-off to writers");
}
ATF_TC_BODY(futex_rw_handoff_write, tc)
{
	atf_tc_skip("futex_rw_handoff is currently broken");
	do_test_rw_handoff_write();
}
ATF_TC_CLEANUP(futex_rw_handoff_write, tc)
{
	do_cleanup();
}

/*****************************************************************************/

static void
do_test_rw_handoff_write_preferred(void)
{
	unsigned int i, tries;
	lwpid_t lid;

	/*
	 * The kernel should not care about the WRITE_WANTED bit, and
	 * should use the contents of the sleepqs as the truth.
	 */
	futex_word = FUTEX_WAITERS;
	membar_sync();

	for (i = 0; i < 2; i++) {
		setup_lwp_context(&lwp_data[i], simple_test_waiter_lwp);
		lwp_data[i].op_flags = FUTEX_PRIVATE_FLAG;
		lwp_data[i].futex_error = -1;
		lwp_data[i].futex_ptr = &futex_word;
		lwp_data[i].block_val = futex_word;
		lwp_data[i].bitset = FUTEX_RW_READER;
		lwp_data[i].wait_op = FUTEX_NETBSD_RW_WAIT;
		ATF_REQUIRE(_lwp_create(&lwp_data[i].context, 0,
					&lwp_data[i].lwpid) == 0);

		/*
		 * Wait for each one to start in-turn, because we want
		 * to know the order in which the LWPs block on the futex.
		 */
		for (tries = 0; tries < 5; tries++) {
			membar_sync();
			if (nlwps_running == i + 1)
				break;
			sleep(1);
		}
		membar_sync();
	}

	ATF_REQUIRE_EQ_MSG(nlwps_running, 2, "read-waiters failed to start");

	setup_lwp_context(&lwp_data[2], simple_test_waiter_lwp);
	lwp_data[2].op_flags = FUTEX_PRIVATE_FLAG;
	lwp_data[2].futex_error = -1;
	lwp_data[2].futex_ptr = &futex_word;
	lwp_data[2].block_val = futex_word;
	lwp_data[2].bitset = FUTEX_RW_WRITER;
	lwp_data[2].wait_op = FUTEX_NETBSD_RW_WAIT;
	ATF_REQUIRE(_lwp_create(&lwp_data[2].context, 0,
				&lwp_data[2].lwpid) == 0);

	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 3)
			break;
		sleep(1);
	}
	membar_sync();

	ATF_REQUIRE_EQ_MSG(nlwps_running, 3, "write-waiter failed to start");

	/* Ensure they're all blocked. */
	ATF_REQUIRE(lwp_data[0].futex_error == -1);
	ATF_REQUIRE(lwp_data[1].futex_error == -1);
	ATF_REQUIRE(lwp_data[2].futex_error == -1);

	/*
	 * Issue a hand-off.  It should select the writer despite
	 * the fact that it is the most recent waiter.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    FUTEX_WAITERS, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_LOCKED);
	ATF_REQUIRE((futex_word & FUTEX_RW_WRITE_WANTED) == 0);
	ATF_REQUIRE(futex_word & FUTEX_WAITERS);
	lid = futex_word & FUTEX_TID_MASK;
	ATF_REQUIRE(lid == lwp_data[2].lwpid);

	/*
	 * Issue another hand-off.  It should awaken both waiting
	 * readers.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 2);
	ATF_REQUIRE(futex_word == 2);

	/*
	 * Issue one final hand-off; it should result in a fully
	 * released lock word.  Note that we don't have any
	 * outstaning waiters waiting, and therefore there will
	 * not be a kernel futex; this exercises a specific code
	 * path in the kernel designed to handle this.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 0);
	ATF_REQUIRE(futex_word == 0);

	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 0)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 0, "read-waiters failed to exit");

	/* Ensure they all exited error-free. */
	ATF_REQUIRE(lwp_data[0].futex_error == 0);
	reap_lwp_waiter(&lwp_data[0]);

	ATF_REQUIRE(lwp_data[1].futex_error == 0);
	reap_lwp_waiter(&lwp_data[1]);

	ATF_REQUIRE(lwp_data[2].futex_error == 0);
	reap_lwp_waiter(&lwp_data[2]);
}

ATF_TC_WITH_CLEANUP(futex_rw_handoff_write_preferred);
ATF_TC_HEAD(futex_rw_handoff_write_preferred, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests that rwlock direct hand-off prefers writers");
}
ATF_TC_BODY(futex_rw_handoff_write_preferred, tc)
{
	atf_tc_skip("futex_rw_handoff is currently broken");
	do_test_rw_handoff_write_preferred();
}
ATF_TC_CLEANUP(futex_rw_handoff_write_preferred, tc)
{
	do_cleanup();
}

/*****************************************************************************/

static void
do_test_rw_handoff_write_locked_rt_reader_preferred(void)
{
	unsigned int i, tries;
	lwpid_t lid;

	setup_rt_params();

	/*
	 * We need to show the rwlock as write-locked to ensure
	 * the rt read-waiter blocks.
	 */
	futex_word =
	    FUTEX_WAITERS | FUTEX_RW_WRITE_WANTED | FUTEX_RW_WRITE_LOCKED;
	membar_sync();

	/*
	 * Create LWPs in the following order:
	 *
	 *	0 - non-RT reader
	 *	1 - RT reader
	 *	2 - non-RT writer
	 *
	 * We expect them to be awakened in this order:
	 *
	 *	1 -> 2 -> 0
	 */
	for (i = 0; i < 3; i++) {
		if (i == 1) {
			setup_lwp_context(&lwp_data[i],
			    rt_simple_test_waiter_lwp);
			lwp_data[i].rt_prio = pri_min;
		} else {
			setup_lwp_context(&lwp_data[i], simple_test_waiter_lwp);
		}
		lwp_data[i].op_flags = FUTEX_PRIVATE_FLAG;
		lwp_data[i].futex_error = -1;
		lwp_data[i].futex_ptr = &futex_word;
		lwp_data[i].block_val = futex_word;
		lwp_data[i].bitset = i == 2 ? FUTEX_RW_WRITER : FUTEX_RW_READER;
		lwp_data[i].wait_op = FUTEX_NETBSD_RW_WAIT;
		ATF_REQUIRE(_lwp_create(&lwp_data[i].context, 0,
					&lwp_data[i].lwpid) == 0);

		/*
		 * Wait for each one to start in-turn, because we want
		 * to know the order in which the LWPs block on the futex.
		 */
		for (tries = 0; tries < 5; tries++) {
			membar_sync();
			if (nlwps_running == i + 1)
				break;
			sleep(1);
		}
		membar_sync();
	}

	ATF_REQUIRE_EQ_MSG(nlwps_running, 3, "waiters failed to start");

	/* Ensure they're all blocked. */
	ATF_REQUIRE(lwp_data[0].futex_error == -1);
	ATF_REQUIRE(lwp_data[1].futex_error == -1);
	ATF_REQUIRE(lwp_data[2].futex_error == -1);

	/*
	 * Issue a hand-off.  It should select the RT reader, and should
	 * indicate that there are still waiters, including a write-waiter.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE((futex_word & FUTEX_RW_WRITE_LOCKED) == 0);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_WANTED);
	ATF_REQUIRE(futex_word & FUTEX_WAITERS);
	lid = futex_word & FUTEX_TID_MASK;
	ATF_REQUIRE(lid == 1);
	ATF_REQUIRE(_lwp_wait(0, &lid) == 0);
	ATF_REQUIRE(lid == lwp_data[1].lwpid);

	/*
	 * Issue another hand-off.  It should awaken the writer and
	 * indicate only readers waiting.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_LOCKED);
	ATF_REQUIRE((futex_word & FUTEX_RW_WRITE_WANTED) == 0);
	ATF_REQUIRE(futex_word & FUTEX_WAITERS);
	lid = futex_word & FUTEX_TID_MASK;
	ATF_REQUIRE(lid == lwp_data[2].lwpid);
	ATF_REQUIRE(_lwp_wait(0, &lid) == 0);
	ATF_REQUIRE(lid == lwp_data[2].lwpid);

	/*
	 * Issue another hand-off.  It would awaken the non-rt
	 * reader and indicate no waiters waiting.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE(futex_word == 1);
	ATF_REQUIRE(_lwp_wait(0, &lid) == 0);
	ATF_REQUIRE(lid == lwp_data[0].lwpid);

	/*
	 * Issue one final hand-off; it should result in a fully
	 * released lock word.  Note that we don't have any
	 * outstaning waiters waiting, and therefore there will
	 * not be a kernel futex; this exercises a specific code
	 * path in the kernel designed to handle this.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 0);
	ATF_REQUIRE(futex_word == 0);

	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 0)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 0, "threads failed to exit");

	/* Ensure they all exited error-free. */
	ATF_REQUIRE(lwp_data[0].futex_error == 0);
	ATF_REQUIRE(lwp_data[1].futex_error == 0);
	ATF_REQUIRE(lwp_data[2].futex_error == 0);
}

ATF_TC_WITH_CLEANUP(futex_rw_handoff_write_locked_rt_reader_preferred);
ATF_TC_HEAD(futex_rw_handoff_write_locked_rt_reader_preferred, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests that write-locked rwlock direct hand-off "
	    "prefers rt readers over non-rt writers");
	atf_tc_set_md_var(tc, "require.user", "root");
}
ATF_TC_BODY(futex_rw_handoff_write_locked_rt_reader_preferred, tc)
{
	atf_tc_skip("futex_rw_handoff is currently broken");
	do_test_rw_handoff_write_locked_rt_reader_preferred();
}
ATF_TC_CLEANUP(futex_rw_handoff_write_locked_rt_reader_preferred, tc)
{
	do_cleanup();
}

/*****************************************************************************/

static void
do_test_rw_wait_write_wanted_rt_reader_preferred(void)
{
	unsigned int i, tries;
	lwpid_t lid;

	setup_rt_params();

	/*
	 * The kernel should not care about the WRITE_WANTED bit, and
	 * should use the contents of the sleepqs as the truth.
	 */
	futex_word = FUTEX_WAITERS;
	membar_sync();

	/*
	 * Create LWPs in the following order:
	 *
	 *	0 - non-RT writer
	 *	1 - non-RT reader
	 *	2 - RT reader
	 *
	 * We expect:
	 *
	 * ==> non-RT reader blocks because WRITE_WANTED.
	 * ==> RT reader does not block and acquires the read-lock.
	 *
	 * We then expect hand-offs to awaken the remaining waiters
	 * in this order:
	 *
	 *	0 -> 1
	 */
	for (i = 0; i < 3; i++) {
		if (i == 2) {
			setup_lwp_context(&lwp_data[i],
			    rt_simple_test_waiter_lwp);
			lwp_data[i].rt_prio = pri_min;
		} else {
			setup_lwp_context(&lwp_data[i], simple_test_waiter_lwp);
		}
		lwp_data[i].op_flags = FUTEX_PRIVATE_FLAG;
		lwp_data[i].futex_error = -1;
		lwp_data[i].futex_ptr = &futex_word;
		lwp_data[i].block_val = futex_word;
		lwp_data[i].bitset = i == 0 ? FUTEX_RW_WRITER : FUTEX_RW_READER;
		lwp_data[i].wait_op = FUTEX_NETBSD_RW_WAIT;
		ATF_REQUIRE(_lwp_create(&lwp_data[i].context, 0,
					&lwp_data[i].lwpid) == 0);

		/*
		 * Wait for the first 2 to start in-turn, because we want
		 * to know the order in which the LWPs block on the futex.
		 */
		for (tries = 0; i < 2 && tries < 5; tries++) {
			membar_sync();
			if (nlwps_running == i + 1)
				break;
			sleep(1);
		}
		membar_sync();
	}

	sleep(2);

	ATF_REQUIRE_EQ_MSG(nlwps_running, 2, "waiters failed to start");

	/* Ensure the first two are blocked and the 3rd one succeeded. */
	ATF_REQUIRE(lwp_data[0].futex_error == -1);
	ATF_REQUIRE(lwp_data[1].futex_error == -1);
	ATF_REQUIRE(lwp_data[2].futex_error == 0);

	/*
	 * Verify the locked-ness and waiter-status of the rwlock.
	 * Also verify it was the expected LWP that made it through
	 * the gate.
	 */
	ATF_REQUIRE(futex_word == (FUTEX_WAITERS | FUTEX_RW_WRITE_WANTED | 1));
	ATF_REQUIRE(_lwp_wait(0, &lid) == 0);
	ATF_REQUIRE(lid == lwp_data[2].lwpid);

	/*
	 * Issue a hand-off.  It should awaken the writer and indicate only
	 * readers waiting.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_LOCKED);
	ATF_REQUIRE((futex_word & FUTEX_RW_WRITE_WANTED) == 0);
	ATF_REQUIRE(futex_word & FUTEX_WAITERS);
	lid = futex_word & FUTEX_TID_MASK;
	ATF_REQUIRE(lid == lwp_data[0].lwpid);
	ATF_REQUIRE(_lwp_wait(0, &lid) == 0);
	ATF_REQUIRE(lid == lwp_data[0].lwpid);

	/*
	 * Issue another hand-off.  It would awaken the non-rt
	 * reader and indicate no waiters waiting.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE(futex_word == 1);
	ATF_REQUIRE(_lwp_wait(0, &lid) == 0);
	ATF_REQUIRE(lid == lwp_data[1].lwpid);

	/*
	 * Issue one final hand-off; it should result in a fully
	 * released lock word.  Note that we don't have any
	 * outstaning waiters waiting, and therefore there will
	 * not be a kernel futex; this exercises a specific code
	 * path in the kernel designed to handle this.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 0);
	ATF_REQUIRE(futex_word == 0);

	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 0)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 0, "threads failed to exit");

	/* Ensure they all exited error-free. */
	ATF_REQUIRE(lwp_data[0].futex_error == 0);
	ATF_REQUIRE(lwp_data[1].futex_error == 0);
}

ATF_TC_WITH_CLEANUP(futex_rw_wait_write_wanted_rt_reader_preferred);
ATF_TC_HEAD(futex_rw_wait_write_wanted_rt_reader_preferred, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests that a wait by a rt-reader acquires "
	    "even if the rwlock is wanted by a non-rt writer");
	atf_tc_set_md_var(tc, "require.user", "root");
}
ATF_TC_BODY(futex_rw_wait_write_wanted_rt_reader_preferred, tc)
{
	atf_tc_skip("futex_rw_handoff is currently broken");
	do_test_rw_wait_write_wanted_rt_reader_preferred();
}
ATF_TC_CLEANUP(futex_rw_wait_write_wanted_rt_reader_preferred, tc)
{
	do_cleanup();
}

/*****************************************************************************/

static void
do_test_rw_handoff_rt_prio_order(void)
{
	unsigned int i, tries;
	lwpid_t lid;

	setup_rt_params();

	/*
	 * We need to show the rwlock as write-locked to ensure
	 * the rt read-waiter blocks.
	 */
	futex_word =
	    FUTEX_WAITERS | FUTEX_RW_WRITE_WANTED | FUTEX_RW_WRITE_LOCKED;
	membar_sync();

	/*
	 * Create LWPs in the following order:
	 *
	 *	0 - pri_min+0 writer
	 *	1 - pri_min+1 reader
	 *	2 - pri_min+2 writer
	 *	3 - pri_min+3 reader
	 *
	 * We expect only one to be awakened at each hand-off and for them
	 * to be awakened in this order:
	 *
	 *	3 -> 2 -> 1 -> 0
	 */
	for (i = 0; i < 4; i++) {
		setup_lwp_context(&lwp_data[i], rt_simple_test_waiter_lwp);
		lwp_data[i].rt_prio = pri_min + i;
		lwp_data[i].op_flags = FUTEX_PRIVATE_FLAG;
		lwp_data[i].futex_error = -1;
		lwp_data[i].futex_ptr = &futex_word;
		lwp_data[i].block_val = futex_word;
		lwp_data[i].bitset = i & 1 ? FUTEX_RW_READER : FUTEX_RW_WRITER;
		lwp_data[i].wait_op = FUTEX_NETBSD_RW_WAIT;
		ATF_REQUIRE(_lwp_create(&lwp_data[i].context, 0,
					&lwp_data[i].lwpid) == 0);

		/*
		 * Wait for each one to start in-turn, because we want
		 * to know the order in which the LWPs block on the futex.
		 */
		for (tries = 0; tries < 5; tries++) {
			membar_sync();
			if (nlwps_running == i + 1)
				break;
			sleep(1);
		}
		membar_sync();
	}

	ATF_REQUIRE_EQ_MSG(nlwps_running, 4, "waiters failed to start");

	/* Ensure they're all blocked. */
	ATF_REQUIRE(lwp_data[0].futex_error == -1);
	ATF_REQUIRE(lwp_data[1].futex_error == -1);
	ATF_REQUIRE(lwp_data[2].futex_error == -1);
	ATF_REQUIRE(lwp_data[3].futex_error == -1);

	/*
	 * Issue a hand-off.  It should select the pri_min+3 reader, and should
	 * indicate that there are still waiters, including a write-waiter.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE((futex_word & FUTEX_RW_WRITE_LOCKED) == 0);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_WANTED);
	ATF_REQUIRE(futex_word & FUTEX_WAITERS);
	lid = futex_word & FUTEX_TID_MASK;
	ATF_REQUIRE(lid == 1);
	ATF_REQUIRE(_lwp_wait(0, &lid) == 0);
	ATF_REQUIRE(lid == lwp_data[3].lwpid);

	/*
	 * Issue another hand-off.  It should awaken the pri_min+2 writer and
	 * indicate that there are still waiters, including a write-waiter.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_LOCKED);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_WANTED);
	ATF_REQUIRE(futex_word & FUTEX_WAITERS);
	lid = futex_word & FUTEX_TID_MASK;
	ATF_REQUIRE(lid == lwp_data[2].lwpid);
	ATF_REQUIRE(_lwp_wait(0, &lid) == 0);
	ATF_REQUIRE(lid == lwp_data[2].lwpid);

	/*
	 * Issue another a hand-off.  It should select the pri_min+1 reader,
	 * and should indicate that there are still waiters, including a
	 * write-waiter.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE((futex_word & FUTEX_RW_WRITE_LOCKED) == 0);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_WANTED);
	ATF_REQUIRE(futex_word & FUTEX_WAITERS);
	lid = futex_word & FUTEX_TID_MASK;
	ATF_REQUIRE(lid == 1);
	ATF_REQUIRE(_lwp_wait(0, &lid) == 0);
	ATF_REQUIRE(lid == lwp_data[1].lwpid);

	/*
	 * Issue another hand-off.  It should awaken the pri_min+0 writer and
	 * indicate that there are no more waiters.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 1);
	ATF_REQUIRE(futex_word & FUTEX_RW_WRITE_LOCKED);
	ATF_REQUIRE((futex_word & FUTEX_RW_WRITE_WANTED) == 0);
	ATF_REQUIRE((futex_word & FUTEX_WAITERS) == 0);
	lid = futex_word & FUTEX_TID_MASK;
	ATF_REQUIRE(lid == lwp_data[0].lwpid);
	ATF_REQUIRE(_lwp_wait(0, &lid) == 0);
	ATF_REQUIRE(lid == lwp_data[0].lwpid);

	/*
	 * Issue one final hand-off; it should result in a fully
	 * released lock word.  Note that we don't have any
	 * outstaning waiters waiting, and therefore there will
	 * not be a kernel futex; this exercises a specific code
	 * path in the kernel designed to handle this.
	 */
	ATF_REQUIRE(__futex(&futex_word,
			    FUTEX_NETBSD_RW_HANDOFF | FUTEX_PRIVATE_FLAG,
			    futex_word, NULL, NULL, 0, 0) == 0);
	ATF_REQUIRE(futex_word == 0);

	for (tries = 0; tries < 5; tries++) {
		membar_sync();
		if (nlwps_running == 0)
			break;
		sleep(1);
	}
	membar_sync();
	ATF_REQUIRE_EQ_MSG(nlwps_running, 0, "threads failed to exit");

	/* Ensure they all exited error-free. */
	ATF_REQUIRE(lwp_data[0].futex_error == 0);
	ATF_REQUIRE(lwp_data[1].futex_error == 0);
	ATF_REQUIRE(lwp_data[2].futex_error == 0);
	ATF_REQUIRE(lwp_data[3].futex_error == 0);
}

ATF_TC_WITH_CLEANUP(futex_rw_handoff_rt_prio_order);
ATF_TC_HEAD(futex_rw_handoff_rt_prio_order, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "tests that hand-off to mixed readers/writer occurs "
	    "in strict priority order");
	atf_tc_set_md_var(tc, "require.user", "root");
}
ATF_TC_BODY(futex_rw_handoff_rt_prio_order, tc)
{
	atf_tc_skip("futex_rw_handoff is currently broken");
	do_test_rw_handoff_rt_prio_order();
}
ATF_TC_CLEANUP(futex_rw_handoff_rt_prio_order, tc)
{
	do_cleanup();
}

/*****************************************************************************/

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, futex_basic_wait_wake_private);
	ATF_TP_ADD_TC(tp, futex_basic_wait_wake_shared);
	ATF_TP_ADD_TC(tp, futex_wait_wake_anon_bs_private);
	ATF_TP_ADD_TC(tp, futex_wait_wake_anon_bs_shared);
	ATF_TP_ADD_TC(tp, futex_wait_wake_file_bs_private);
	ATF_TP_ADD_TC(tp, futex_wait_wake_file_bs_shared);
	ATF_TP_ADD_TC(tp, futex_wait_wake_file_bs_cow_private);
	ATF_TP_ADD_TC(tp, futex_wait_wake_file_bs_cow_shared);

	ATF_TP_ADD_TC(tp, futex_wait_wake_anon_bs_shared_proc);
	ATF_TP_ADD_TC(tp, futex_wait_wake_file_bs_shared_proc);

	ATF_TP_ADD_TC(tp, futex_wait_pointless_bitset);
	ATF_TP_ADD_TC(tp, futex_wait_wake_bitset);

	ATF_TP_ADD_TC(tp, futex_wait_timeout_relative);
	ATF_TP_ADD_TC(tp, futex_wait_timeout_relative_rt);
	ATF_TP_ADD_TC(tp, futex_wait_timeout_deadline);
	ATF_TP_ADD_TC(tp, futex_wait_timeout_deadline_rt);

	ATF_TP_ADD_TC(tp, futex_wait_evil_unmapped_anon);

	ATF_TP_ADD_TC(tp, futex_requeue);
	ATF_TP_ADD_TC(tp, futex_cmp_requeue);

	ATF_TP_ADD_TC(tp, futex_wake_op_op);
	ATF_TP_ADD_TC(tp, futex_wake_op_cmp);

	ATF_TP_ADD_TC(tp, futex_wake_highest_pri);

	ATF_TP_ADD_TC(tp, futex_rw_handoff_read);
	ATF_TP_ADD_TC(tp, futex_rw_handoff_write);
	ATF_TP_ADD_TC(tp, futex_rw_handoff_write_preferred);
	ATF_TP_ADD_TC(tp, futex_rw_handoff_write_locked_rt_reader_preferred);
	ATF_TP_ADD_TC(tp, futex_rw_wait_write_wanted_rt_reader_preferred);
	ATF_TP_ADD_TC(tp, futex_rw_handoff_rt_prio_order);

	return atf_no_error();
}
