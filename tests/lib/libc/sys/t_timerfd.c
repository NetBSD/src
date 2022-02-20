/* $NetBSD: t_timerfd.c,v 1.4 2022/02/20 15:21:14 thorpej Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 2020\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_timerfd.c,v 1.4 2022/02/20 15:21:14 thorpej Exp $");

#include <sys/types.h>
#include <sys/event.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <atf-c.h>

#include "isqemu.h"

struct helper_context {
	int	fd;

	pthread_barrier_t barrier;
};

static void
init_helper_context(struct helper_context * const ctx)
{

	memset(ctx, 0, sizeof(*ctx));

	ATF_REQUIRE(pthread_barrier_init(&ctx->barrier, NULL, 2) == 0);
}

static bool
wait_barrier(struct helper_context * const ctx)
{
	int rv = pthread_barrier_wait(&ctx->barrier);

	return rv == 0 || rv == PTHREAD_BARRIER_SERIAL_THREAD;
}

static bool
check_value_against_bounds(uint64_t value, uint64_t lower, uint64_t upper)
{

	/*
	 * If running under QEMU make sure the upper bound is large
	 * enough for the effect of kern/43997
	 */
	if (isQEMU()) {
		upper *= 4;
	}

	if (value < lower || value > upper) {
		printf("val %" PRIu64 " not in [ %" PRIu64 ", %" PRIu64 " ]\n",
		    value, lower, upper);
	}

	return value >= lower && value <= upper;
}

/*****************************************************************************/

static int
timerfd_read(int fd, uint64_t *valp)
{
	uint64_t val;

	switch (read(fd, &val, sizeof(val))) {
	case -1:
		return -1;

	case sizeof(val):
		*valp = val;
		return 0;

	default:
		/* ?? Should never happen. */
		errno = EIO;
		return -1;
	}
}

/*****************************************************************************/

ATF_TC(timerfd_create);
ATF_TC_HEAD(timerfd_create, tc)
{
	atf_tc_set_md_var(tc, "descr", "validates timerfd_create()");
}
ATF_TC_BODY(timerfd_create, tc)
{
	int fd;

	ATF_REQUIRE((fd = timerfd_create(CLOCK_REALTIME, 0)) >= 0);
	(void) close(fd);

	ATF_REQUIRE((fd = timerfd_create(CLOCK_MONOTONIC, 0)) >= 0);
	(void) close(fd);

	ATF_REQUIRE_ERRNO(EINVAL,
	    (fd = timerfd_create(CLOCK_VIRTUAL, 0)) == -1);

	ATF_REQUIRE_ERRNO(EINVAL,
	    (fd = timerfd_create(CLOCK_PROF, 0)) == -1);

	ATF_REQUIRE_ERRNO(EINVAL,
	    (fd = timerfd_create(CLOCK_REALTIME,
	    			    ~(TFD_CLOEXEC | TFD_NONBLOCK))) == -1);
}

/*****************************************************************************/

ATF_TC(timerfd_bogusfd);
ATF_TC_HEAD(timerfd_bogusfd, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "validates rejection of bogus fds by timerfd_{get,set}time()");
}
ATF_TC_BODY(timerfd_bogusfd, tc)
{
	struct itimerspec its = { 0 };
	int fd;

	ATF_REQUIRE((fd = kqueue()) >= 0);	/* arbitrary fd type */

	ATF_REQUIRE_ERRNO(EINVAL,
	    timerfd_gettime(fd, &its) == -1);

	its.it_value.tv_sec = 5;
	ATF_REQUIRE_ERRNO(EINVAL,
	    timerfd_settime(fd, 0, &its, NULL) == -1);

	(void) close(fd);
}

/*****************************************************************************/

ATF_TC(timerfd_block);
ATF_TC_HEAD(timerfd_block, tc)
{
	atf_tc_set_md_var(tc, "descr", "validates blocking behavior");
}
ATF_TC_BODY(timerfd_block, tc)
{
	struct timespec then, now, delta;
	uint64_t val;
	int fd;

	ATF_REQUIRE((fd = timerfd_create(CLOCK_MONOTONIC, 0)) >= 0);

	const struct itimerspec its = {
		.it_value = { .tv_sec = 1, .tv_nsec = 0 },
		.it_interval = { .tv_sec = 0, .tv_nsec = 0 },
	};

	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &then) == 0);
	ATF_REQUIRE(timerfd_settime(fd, 0, &its, NULL) == 0);
	ATF_REQUIRE(timerfd_read(fd, &val) == 0);
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
	ATF_REQUIRE(check_value_against_bounds(val, 1, 1));

	timespecsub(&now, &then, &delta);
	ATF_REQUIRE(check_value_against_bounds(delta.tv_sec, 1, 1));

	(void) close(fd);
}

/*****************************************************************************/

ATF_TC(timerfd_repeating);
ATF_TC_HEAD(timerfd_repeating, tc)
{
	atf_tc_set_md_var(tc, "descr", "validates repeating timer behavior");
}
ATF_TC_BODY(timerfd_repeating, tc)
{
	struct timespec then, now, delta;
	uint64_t val;
	int fd;

	ATF_REQUIRE((fd = timerfd_create(CLOCK_MONOTONIC,
					    TFD_NONBLOCK)) >= 0);

	const struct itimerspec its = {
		.it_value = { .tv_sec = 0, .tv_nsec = 200000000 },
		.it_interval = { .tv_sec = 0, .tv_nsec = 200000000 },
	};

	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &then) == 0);
	ATF_REQUIRE(timerfd_settime(fd, 0, &its, NULL) == 0);
	ATF_REQUIRE(sleep(1) == 0);
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
	ATF_REQUIRE(timerfd_read(fd, &val) == 0);
	/* allow some slop */
	ATF_REQUIRE(check_value_against_bounds(val, 3, 5));

	timespecsub(&now, &then, &delta);
	ATF_REQUIRE(check_value_against_bounds(delta.tv_sec, 1, 1));

	(void) close(fd);
}

/*****************************************************************************/

ATF_TC(timerfd_abstime);
ATF_TC_HEAD(timerfd_abstime, tc)
{
	atf_tc_set_md_var(tc, "descr", "validates specifying abstime");
}
ATF_TC_BODY(timerfd_abstime, tc)
{
	struct timespec then, now, delta;
	uint64_t val;
	int fd;

	ATF_REQUIRE((fd = timerfd_create(CLOCK_MONOTONIC, 0)) >= 0);

	struct itimerspec its = {
		.it_value = { .tv_sec = 0, .tv_nsec = 0 },
		.it_interval = { .tv_sec = 0, .tv_nsec = 0 },
	};

	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &then) == 0);
	its.it_value = then;
	its.it_value.tv_sec += 1;
	ATF_REQUIRE(timerfd_settime(fd, TFD_TIMER_ABSTIME, &its, NULL) == 0);
	ATF_REQUIRE(timerfd_read(fd, &val) == 0);
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
	ATF_REQUIRE(check_value_against_bounds(val, 1, 1));

	timespecsub(&now, &then, &delta);
	ATF_REQUIRE(check_value_against_bounds(delta.tv_sec, 1, 1));

	(void) close(fd);
}

/*****************************************************************************/

ATF_TC(timerfd_cancel_on_set_immed);
ATF_TC_HEAD(timerfd_cancel_on_set_immed, tc)
{
	atf_tc_set_md_var(tc, "descr", "validates cancel-on-set - immediate");
	atf_tc_set_md_var(tc, "require.user", "root");
}
ATF_TC_BODY(timerfd_cancel_on_set_immed, tc)
{
	struct timespec now;
	uint64_t val;
	int fd;

	ATF_REQUIRE((fd = timerfd_create(CLOCK_REALTIME, 0)) >= 0);

	const struct itimerspec its = {
		.it_value = { .tv_sec = 60 * 60, .tv_nsec = 0 },
		.it_interval = { .tv_sec = 0, .tv_nsec = 0 },
	};

	ATF_REQUIRE(clock_gettime(CLOCK_REALTIME, &now) == 0);
	ATF_REQUIRE(timerfd_settime(fd, TFD_TIMER_CANCEL_ON_SET,
				    &its, NULL) == 0);
	ATF_REQUIRE(clock_settime(CLOCK_REALTIME, &now) == 0);
	ATF_REQUIRE_ERRNO(ECANCELED, timerfd_read(fd, &val) == -1);

	(void) close(fd);
}

/*****************************************************************************/

static void *
timerfd_cancel_on_set_block_helper(void * const v)
{
	struct helper_context * const ctx = v;
	struct timespec now;

	ATF_REQUIRE(wait_barrier(ctx));

	ATF_REQUIRE(sleep(2) == 0);
	ATF_REQUIRE(clock_gettime(CLOCK_REALTIME, &now) == 0);
	ATF_REQUIRE(clock_settime(CLOCK_REALTIME, &now) == 0);

	return NULL;
}

ATF_TC(timerfd_cancel_on_set_block);
ATF_TC_HEAD(timerfd_cancel_on_set_block, tc)
{
	atf_tc_set_md_var(tc, "descr", "validates cancel-on-set - blocking");
	atf_tc_set_md_var(tc, "require.user", "root");
}
ATF_TC_BODY(timerfd_cancel_on_set_block, tc)
{
	struct helper_context ctx;
	pthread_t helper;
	void *join_val;
	uint64_t val;
	int fd;

	ATF_REQUIRE((fd = timerfd_create(CLOCK_REALTIME, 0)) >= 0);

	const struct itimerspec its = {
		.it_value = { .tv_sec = 60 * 60, .tv_nsec = 0 },
		.it_interval = { .tv_sec = 0, .tv_nsec = 0 },
	};

	init_helper_context(&ctx);

	ATF_REQUIRE(timerfd_settime(fd, TFD_TIMER_CANCEL_ON_SET,
				    &its, NULL) == 0);
	ATF_REQUIRE(pthread_create(&helper, NULL,
				timerfd_cancel_on_set_block_helper, &ctx) == 0);
	ATF_REQUIRE(wait_barrier(&ctx));
	ATF_REQUIRE_ERRNO(ECANCELED, timerfd_read(fd, &val) == -1);

	ATF_REQUIRE(pthread_join(helper, &join_val) == 0);

	(void) close(fd);
}

/*****************************************************************************/

ATF_TC(timerfd_select_poll_kevent_immed);
ATF_TC_HEAD(timerfd_select_poll_kevent_immed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "validates select/poll/kevent behavior - immediate return");
}
ATF_TC_BODY(timerfd_select_poll_kevent_immed, tc)
{
	const struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
	struct itimerspec its;
	struct timeval tv;
	struct stat st;
	struct pollfd fds[1];
	uint64_t val;
	fd_set readfds, writefds, exceptfds;
	int fd;
	int kq;
	struct kevent kev[1];

	ATF_REQUIRE((fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)) >= 0);

	ATF_REQUIRE((kq = kqueue()) >= 0);
	EV_SET(&kev[0], fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
	ATF_REQUIRE(kevent(kq, kev, 1, NULL, 0, &ts) == 0);

	/*
	 * fd should be writable but not readable.  Pass all of the
	 * event bits; we should only get back POLLOUT | POLLWRNORM.
	 * (It's writable only in so far as we'll get an error if we try.)
	 */
	fds[0].fd = fd;
	fds[0].events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI |
	    POLLOUT | POLLWRNORM | POLLWRBAND | POLLHUP;
	fds[0].revents = 0;
	ATF_REQUIRE(poll(fds, 1, 0) == 1);
	ATF_REQUIRE(fds[0].revents == (POLLOUT | POLLWRNORM));

	/*
	 * As above; fd should only be set in writefds upon return
	 * from the select() call.
	 */
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_SET(fd, &readfds);
	FD_SET(fd, &writefds);
	FD_SET(fd, &exceptfds);
	ATF_REQUIRE(select(fd + 1, &readfds, &writefds, &exceptfds, &tv) == 1);
	ATF_REQUIRE(!FD_ISSET(fd, &readfds));
	ATF_REQUIRE(FD_ISSET(fd, &writefds));
	ATF_REQUIRE(!FD_ISSET(fd, &exceptfds));

	/*
	 * Now set a one-shot half-second timer, wait for it to expire, and
	 * then check again.
	 */
	memset(&its, 0, sizeof(its));
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 500000000;
	ATF_REQUIRE(timerfd_settime(fd, 0, &its, NULL) == 0);
	ATF_REQUIRE(sleep(2) == 0);

	/* Verify it actually fired via the stat() back-channel. */
	ATF_REQUIRE(fstat(fd, &st) == 0);
	ATF_REQUIRE(st.st_size == 1);

	fds[0].fd = fd;
	fds[0].events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI |
	    POLLOUT | POLLWRNORM | POLLWRBAND | POLLHUP;
	fds[0].revents = 0;
	ATF_REQUIRE(poll(fds, 1, 0) == 1);
	ATF_REQUIRE(fds[0].revents == (POLLIN | POLLRDNORM |
				       POLLOUT | POLLWRNORM));

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_SET(fd, &readfds);
	FD_SET(fd, &writefds);
	FD_SET(fd, &exceptfds);
	ATF_REQUIRE(select(fd + 1, &readfds, &writefds, &exceptfds, &tv) == 2);
	ATF_REQUIRE(FD_ISSET(fd, &readfds));
	ATF_REQUIRE(FD_ISSET(fd, &writefds));
	ATF_REQUIRE(!FD_ISSET(fd, &exceptfds));

	/*
	 * Check that we get an EVFILT_READ event on fd.
	 */
	memset(kev, 0, sizeof(kev));
	ATF_REQUIRE(kevent(kq, NULL, 0, kev, 1, &ts) == 1);
	ATF_REQUIRE(kev[0].ident == (uintptr_t)fd);
	ATF_REQUIRE(kev[0].filter == EVFILT_READ);
	ATF_REQUIRE((kev[0].flags & (EV_EOF | EV_ERROR)) == 0);
	ATF_REQUIRE(kev[0].data == 1);

	/*
	 * Read the timerfd to ensure we get the correct numnber of
	 * expirations.
	 */
	ATF_REQUIRE(timerfd_read(fd, &val) == 0);
	ATF_REQUIRE(val == 1);

	/* And ensure that we would block if we tried again. */
	ATF_REQUIRE_ERRNO(EAGAIN, timerfd_read(fd, &val) == -1);

	(void) close(kq);
	(void) close(fd);
}

/*****************************************************************************/

ATF_TC(timerfd_select_poll_kevent_block);
ATF_TC_HEAD(timerfd_select_poll_kevent_block, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "validates select/poll/kevent behavior - blocking");
}
ATF_TC_BODY(timerfd_select_poll_kevent_block, tc)
{
	const struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
	struct timespec then, now;
	struct pollfd fds[1];
	fd_set readfds;
	int fd;
	int kq;
	struct kevent kev[1];

	ATF_REQUIRE((fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)) >= 0);

	ATF_REQUIRE((kq = kqueue()) >= 0);
	EV_SET(&kev[0], fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
	ATF_REQUIRE(kevent(kq, kev, 1, NULL, 0, &ts) == 0);

	/*
	 * For each of these tests, we do the following:
	 *
	 * - Get the current time.
	 * - Set a 1-second one-shot timer.
	 * - Block in the multiplexing call.
	 * - Get the current time and verify that the timer expiration
	 *   interval has passed.
	 */

	const struct itimerspec its = {
		.it_value = { .tv_sec = 1, .tv_nsec = 0 },
		.it_interval = { .tv_sec = 0, .tv_nsec = 0 },
	};

	/* poll(2) */
	fds[0].fd = fd;
	fds[0].events = POLLIN | POLLRDNORM;
	fds[0].revents = 0;

	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &then) == 0);
	ATF_REQUIRE(timerfd_settime(fd, 0, &its, NULL) == 0);
	ATF_REQUIRE(poll(fds, 1, INFTIM) == 1);
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
	ATF_REQUIRE(fds[0].revents == (POLLIN | POLLRDNORM));
	ATF_REQUIRE(now.tv_sec - then.tv_sec >= 1);

	/* select(2) */
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &then) == 0);
	ATF_REQUIRE(timerfd_settime(fd, 0, &its, NULL) == 0);
	ATF_REQUIRE(select(fd + 1, &readfds, NULL, NULL, NULL) == 1);
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
	ATF_REQUIRE(FD_ISSET(fd, &readfds));
	ATF_REQUIRE(now.tv_sec - then.tv_sec >= 1);

	/* kevent(2) */
	memset(kev, 0, sizeof(kev));
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &then) == 0);
	ATF_REQUIRE(timerfd_settime(fd, 0, &its, NULL) == 0);
	ATF_REQUIRE(kevent(kq, NULL, 0, kev, 1, NULL) == 1);
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
	ATF_REQUIRE(kev[0].ident == (uintptr_t)fd);
	ATF_REQUIRE(kev[0].filter == EVFILT_READ);
	ATF_REQUIRE((kev[0].flags & (EV_EOF | EV_ERROR)) == 0);
	ATF_REQUIRE(kev[0].data == 1);

	(void) close(kq);
	(void) close(fd);
}

/*****************************************************************************/

static void *
timerfd_restart_helper(void * const v)
{
	struct helper_context * const ctx = v;

	ATF_REQUIRE(wait_barrier(ctx));

	/*
	 * Wait 5 seconds (that should give the main thread time to
	 * block), and then close the descriptor.
	 */
	ATF_REQUIRE(sleep(5) == 0);
	ATF_REQUIRE(close(ctx->fd) == 0);

	return NULL;
}

ATF_TC(timerfd_restart);
ATF_TC_HEAD(timerfd_restart, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "exercises the 'restart' fileop code path");
}
ATF_TC_BODY(timerfd_restart, tc)
{
	struct timespec then, now, delta;
	struct helper_context ctx;
	uint64_t val;
	pthread_t helper;
	void *join_val;

	init_helper_context(&ctx);

	ATF_REQUIRE((ctx.fd = timerfd_create(CLOCK_MONOTONIC, 0)) >= 0);

	const struct itimerspec its = {
		.it_value = { .tv_sec = 60 * 60, .tv_nsec = 0 },
		.it_interval = { .tv_sec = 0, .tv_nsec = 0 },
	};
	ATF_REQUIRE(timerfd_settime(ctx.fd, 0, &its, NULL) == 0);


	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &then) == 0);
	ATF_REQUIRE(pthread_create(&helper, NULL,
				   timerfd_restart_helper, &ctx) == 0);

	/*
	 * Wait for the helper to be ready, and then immediately block
	 * in read().  The helper will close the file, and we should get
	 * EBADF after a few seconds.
	 */
	ATF_REQUIRE(wait_barrier(&ctx));
	ATF_REQUIRE_ERRNO(EBADF, timerfd_read(ctx.fd, &val) == -1);
	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &now) == 0);

	timespecsub(&now, &then, &delta);
	ATF_REQUIRE(delta.tv_sec >= 5);

	/* Reap the helper. */
	ATF_REQUIRE(pthread_join(helper, &join_val) == 0);
}

/*****************************************************************************/

ATF_TC(timerfd_fcntl);
ATF_TC_HEAD(timerfd_fcntl, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "validates fcntl behavior");
}

ATF_TC_BODY(timerfd_fcntl, tc)
{
	int tfd;
	int val;

	ATF_REQUIRE((tfd = timerfd_create(CLOCK_MONOTONIC, 0)) >= 0);
	ATF_REQUIRE((fcntl(tfd, F_GETFL) & O_NONBLOCK) == 0);
	ATF_REQUIRE(fcntl(tfd, F_SETFL, O_NONBLOCK) == 0);
	ATF_REQUIRE((fcntl(tfd, F_GETFL) & O_NONBLOCK) != 0);
	ATF_REQUIRE((fcntl(tfd, F_GETFD) & FD_CLOEXEC) == 0);

	/* If the timer hasn't fired, there is no readable data. */
	ATF_REQUIRE(ioctl(tfd, FIONREAD, &val) == 0);
	ATF_REQUIRE(val == 0);

	ATF_REQUIRE_ERRNO(ENOTTY, ioctl(tfd, FIONWRITE, &val) == -1);
	ATF_REQUIRE_ERRNO(ENOTTY, ioctl(tfd, FIONSPACE, &val) == -1);
	(void)close(tfd);

	ATF_REQUIRE((tfd = timerfd_create(CLOCK_MONOTONIC,
					  TFD_NONBLOCK | TFD_CLOEXEC)) >= 0);
	ATF_REQUIRE((fcntl(tfd, F_GETFL) & ~O_ACCMODE) == O_NONBLOCK);
	ATF_REQUIRE((fcntl(tfd, F_GETFD) & FD_CLOEXEC) != 0);
	ATF_REQUIRE(fcntl(tfd, F_SETFD, 0) == 0);
	ATF_REQUIRE((fcntl(tfd, F_GETFD) & FD_CLOEXEC) == 0);
	ATF_REQUIRE(fcntl(tfd, F_SETFD, FD_CLOEXEC) == 0);
	ATF_REQUIRE((fcntl(tfd, F_GETFD) & FD_CLOEXEC) != 0);
	(void)close(tfd);
}

/*****************************************************************************/

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, timerfd_create);
	ATF_TP_ADD_TC(tp, timerfd_bogusfd);
	ATF_TP_ADD_TC(tp, timerfd_block);
	ATF_TP_ADD_TC(tp, timerfd_repeating);
	ATF_TP_ADD_TC(tp, timerfd_abstime);
	ATF_TP_ADD_TC(tp, timerfd_cancel_on_set_block);
	ATF_TP_ADD_TC(tp, timerfd_cancel_on_set_immed);
	ATF_TP_ADD_TC(tp, timerfd_select_poll_kevent_immed);
	ATF_TP_ADD_TC(tp, timerfd_select_poll_kevent_block);
	ATF_TP_ADD_TC(tp, timerfd_restart);
	ATF_TP_ADD_TC(tp, timerfd_fcntl);

	return atf_no_error();
}
