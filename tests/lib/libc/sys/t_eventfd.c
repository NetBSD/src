/* $NetBSD: t_eventfd.c,v 1.3 2022/02/20 15:21:14 thorpej Exp $ */

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
__RCSID("$NetBSD: t_eventfd.c,v 1.3 2022/02/20 15:21:14 thorpej Exp $");

#include <sys/types.h>
#include <sys/event.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <atf-c.h>

struct helper_context {
	int	efd;

	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_barrier_t barrier;
	int	state;
};

static void
init_helper_context(struct helper_context * const ctx)
{
	pthread_condattr_t condattr;

	memset(ctx, 0, sizeof(*ctx));

	ATF_REQUIRE(pthread_mutex_init(&ctx->mutex, NULL) == 0);

	ATF_REQUIRE(pthread_condattr_init(&condattr) == 0);
	ATF_REQUIRE(pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC) == 0);
	ATF_REQUIRE(pthread_cond_init(&ctx->cond, &condattr) == 0);
	ATF_REQUIRE(pthread_condattr_destroy(&condattr) == 0);

	ATF_REQUIRE(pthread_barrier_init(&ctx->barrier, NULL, 2) == 0);
}

static void
set_state(struct helper_context * const ctx, int const new)
{
	pthread_mutex_lock(&ctx->mutex);
	ctx->state = new;
	pthread_cond_signal(&ctx->cond);
	pthread_mutex_unlock(&ctx->mutex);
}

static int
get_state(struct helper_context * const ctx)
{
	int rv;

	pthread_mutex_lock(&ctx->mutex);
	rv = ctx->state;
	pthread_mutex_unlock(&ctx->mutex);

	return rv;
}

static bool
wait_state(struct helper_context * const ctx, int const val)
{
	struct timespec deadline;
	int error;
	bool rv;

	pthread_mutex_lock(&ctx->mutex);

	ATF_REQUIRE(clock_gettime(CLOCK_MONOTONIC, &deadline) == 0);
	deadline.tv_sec += 5;

	while (ctx->state != val) {
		error = pthread_cond_timedwait(&ctx->cond, &ctx->mutex,
		    &deadline);
		if (error) {
			break;
		}
	}
	rv = ctx->state == val;

	pthread_mutex_unlock(&ctx->mutex);

	return rv;
}

static bool
wait_barrier(struct helper_context * const ctx)
{
	int rv = pthread_barrier_wait(&ctx->barrier);

	return rv == 0 || rv == PTHREAD_BARRIER_SERIAL_THREAD;
}

/*****************************************************************************/

static void *
eventfd_normal_helper(void * const v)
{
	struct helper_context * const ctx = v;
	eventfd_t efd_value;

	ATF_REQUIRE(wait_barrier(ctx));

	/* Read the value.  This will reset it to zero. */
	ATF_REQUIRE(get_state(ctx) == 666);
	ATF_REQUIRE(eventfd_read(ctx->efd, &efd_value) == 0);

	/* Assert the value. */
	ATF_REQUIRE(efd_value == 0xcafebabe);

	set_state(ctx, 0);

	/* Wait for the main thread to prep the next test. */
	ATF_REQUIRE(wait_barrier(ctx));

	/* Read the value. */
	ATF_REQUIRE(eventfd_read(ctx->efd, &efd_value) == 0);

	/* Assert the value. */
	ATF_REQUIRE(efd_value == 0xbeefcafe);

	ATF_REQUIRE(wait_barrier(ctx));

	return NULL;
}

ATF_TC(eventfd_normal);
ATF_TC_HEAD(eventfd_normal, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "validates basic normal eventfd operation");
}
ATF_TC_BODY(eventfd_normal, tc)
{
	struct helper_context ctx;
	pthread_t helper;
	void *join_val;

	init_helper_context(&ctx);

	ATF_REQUIRE((ctx.efd = eventfd(0, 0)) >= 0);

	ATF_REQUIRE(pthread_create(&helper, NULL,
				   eventfd_normal_helper, &ctx) == 0);

	/*
	 * Wait for the helper to block in read().  Give it some time
	 * so that if the read fails or returns immediately, we'll
	 * notice.
	 */
	set_state(&ctx, 666);
	ATF_REQUIRE(wait_barrier(&ctx));
	sleep(2);
	ATF_REQUIRE(get_state(&ctx) == 666);

	/* Write a distinct value; helper will assert it. */
	ATF_REQUIRE(eventfd_write(ctx.efd, 0xcafebabe) == 0);

	/* Wait for helper to read the value. */
	ATF_REQUIRE(wait_state(&ctx, 0));

	/* Helper is now blocked in a barrier. */

	/* Test additive property of the efd value. */
	ATF_REQUIRE(eventfd_write(ctx.efd, 0x0000cafe) == 0);
	ATF_REQUIRE(eventfd_write(ctx.efd, 0xbeef0000) == 0);

	/* Satisfy the barrier; helper will read value and assert 0xbeefcafe. */
	ATF_REQUIRE(wait_barrier(&ctx));

	/* And wait for it to finish. */
	ATF_REQUIRE(wait_barrier(&ctx));

	/* Reap the helper. */
	ATF_REQUIRE(pthread_join(helper, &join_val) == 0);

	(void) close(ctx.efd);
}

/*****************************************************************************/

ATF_TC(eventfd_semaphore);
ATF_TC_HEAD(eventfd_semaphore, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "validates semaphore and non-blocking eventfd operation");
}
ATF_TC_BODY(eventfd_semaphore, tc)
{
	eventfd_t efd_value;
	int efd;

	ATF_REQUIRE((efd = eventfd(3, EFD_SEMAPHORE | EFD_NONBLOCK)) >= 0);

	/* 3 reads should succeed without blocking. */
	ATF_REQUIRE(eventfd_read(efd, &efd_value) == 0);
	ATF_REQUIRE(efd_value == 1);

	ATF_REQUIRE(eventfd_read(efd, &efd_value) == 0);
	ATF_REQUIRE(efd_value == 1);

	ATF_REQUIRE(eventfd_read(efd, &efd_value) == 0);
	ATF_REQUIRE(efd_value == 1);

	/* This one should block. */
	ATF_REQUIRE_ERRNO(EAGAIN,
	    eventfd_read(efd, &efd_value) == -1);

	/* Add 1 to the semaphore. */
	ATF_REQUIRE(eventfd_write(efd, 1) == 0);

	/* One more read allowed. */
	ATF_REQUIRE(eventfd_read(efd, &efd_value) == 0);
	ATF_REQUIRE(efd_value == 1);

	/* And this one again should block. */
	ATF_REQUIRE_ERRNO(EAGAIN,
	    eventfd_read(efd, &efd_value) == -1);

	(void) close(efd);
}

/*****************************************************************************/

ATF_TC(eventfd_select_poll_kevent_immed);
ATF_TC_HEAD(eventfd_select_poll_kevent_immed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "validates select/poll/kevent behavior - immediate return");
}
ATF_TC_BODY(eventfd_select_poll_kevent_immed, tc)
{
	const struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
	struct timeval tv;
	struct pollfd fds[1];
	fd_set readfds, writefds, exceptfds;
	int efd;
	int kq;
	struct kevent kev[2];

	ATF_REQUIRE((efd = eventfd(0, EFD_NONBLOCK)) >= 0);

	ATF_REQUIRE((kq = kqueue()) >= 0);
	EV_SET(&kev[0], efd, EVFILT_READ, EV_ADD, 0, 0, NULL);
	EV_SET(&kev[1], efd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
	ATF_REQUIRE(kevent(kq, kev, 2, NULL, 0, &ts) == 0);

	/*
	 * efd should be writable but not readable.  Pass all of the
	 * event bits; we should only get back POLLOUT | POLLWRNORM.
	 */
	fds[0].fd = efd;
	fds[0].events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI |
	    POLLOUT | POLLWRNORM | POLLWRBAND | POLLHUP;
	fds[0].revents = 0;
	ATF_REQUIRE(poll(fds, 1, 0) == 1);
	ATF_REQUIRE(fds[0].revents == (POLLOUT | POLLWRNORM));

	/*
	 * As above; efd should only be set in writefds upon return
	 * from the select() call.
	 */
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_SET(efd, &readfds);
	FD_SET(efd, &writefds);
	FD_SET(efd, &exceptfds);
	ATF_REQUIRE(select(efd + 1, &readfds, &writefds, &exceptfds, &tv) == 1);
	ATF_REQUIRE(!FD_ISSET(efd, &readfds));
	ATF_REQUIRE(FD_ISSET(efd, &writefds));
	ATF_REQUIRE(!FD_ISSET(efd, &exceptfds));

	/*
	 * Check that we get an EVFILT_WRITE event (and only that event)
	 * on efd.
	 */
	memset(kev, 0, sizeof(kev));
	ATF_REQUIRE(kevent(kq, NULL, 0, kev, 2, &ts) == 1);
	ATF_REQUIRE(kev[0].ident == (uintptr_t)efd);
	ATF_REQUIRE(kev[0].filter == EVFILT_WRITE);
	ATF_REQUIRE((kev[0].flags & (EV_EOF | EV_ERROR)) == 0);
	ATF_REQUIRE(kev[0].data == 0);

	/*
	 * Write the maximum value into the eventfd.  This should result
	 * in the eventfd becoming readable but NOT writable.
	 */
	ATF_REQUIRE(eventfd_write(efd, UINT64_MAX - 1) == 0);

	fds[0].fd = efd;
	fds[0].events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI |
	    POLLOUT | POLLWRNORM | POLLWRBAND | POLLHUP;
	fds[0].revents = 0;
	ATF_REQUIRE(poll(fds, 1, 0) == 1);
	ATF_REQUIRE(fds[0].revents == (POLLIN | POLLRDNORM));

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_SET(efd, &readfds);
	FD_SET(efd, &writefds);
	FD_SET(efd, &exceptfds);
	ATF_REQUIRE(select(efd + 1, &readfds, &writefds, &exceptfds, &tv) == 1);
	ATF_REQUIRE(FD_ISSET(efd, &readfds));
	ATF_REQUIRE(!FD_ISSET(efd, &writefds));
	ATF_REQUIRE(!FD_ISSET(efd, &exceptfds));

	/*
	 * Check that we get an EVFILT_READ event (and only that event)
	 * on efd.
	 */
	memset(kev, 0, sizeof(kev));
	ATF_REQUIRE(kevent(kq, NULL, 0, kev, 2, &ts) == 1);
	ATF_REQUIRE(kev[0].ident == (uintptr_t)efd);
	ATF_REQUIRE(kev[0].filter == EVFILT_READ);
	ATF_REQUIRE((kev[0].flags & (EV_EOF | EV_ERROR)) == 0);
	ATF_REQUIRE(kev[0].data == (int64_t)(UINT64_MAX - 1));

	(void) close(kq);
	(void) close(efd);
}

/*****************************************************************************/

static void *
eventfd_select_poll_kevent_block_helper(void * const v)
{
	struct helper_context * const ctx = v;
	struct pollfd fds[1];
	fd_set selfds;
	eventfd_t efd_value;
	int kq;
	struct kevent kev[1];

	fds[0].fd = ctx->efd;
	fds[0].events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI;
	fds[0].revents = 0;

	ATF_REQUIRE_ERRNO(EAGAIN,
	    eventfd_read(ctx->efd, &efd_value) == -1);

	ATF_REQUIRE(wait_barrier(ctx));

	ATF_REQUIRE(get_state(ctx) == 666);
	ATF_REQUIRE(poll(fds, 1, INFTIM) == 1);
	ATF_REQUIRE(fds[0].revents == (POLLIN | POLLRDNORM));
	set_state(ctx, 0);

	ATF_REQUIRE(wait_barrier(ctx));

	/*
	 * The maximum value was written to the eventfd, so we
	 * should block waiting for writability.
	 */
	fds[0].fd = ctx->efd;
	fds[0].events = POLLOUT | POLLWRNORM;
	fds[0].revents = 0;

	ATF_REQUIRE_ERRNO(EAGAIN,
	    eventfd_write(ctx->efd, UINT64_MAX - 1) == -1);

	ATF_REQUIRE(wait_barrier(ctx));

	ATF_REQUIRE(get_state(ctx) == 666);
	ATF_REQUIRE(poll(fds, 1, INFTIM) == 1);
	ATF_REQUIRE(fds[0].revents == (POLLOUT | POLLWRNORM));
	set_state(ctx, 0);

	ATF_REQUIRE(wait_barrier(ctx));

	/*
	 * Now, the same dance again, with select().
	 */

	FD_ZERO(&selfds);
	FD_SET(ctx->efd, &selfds);

	ATF_REQUIRE_ERRNO(EAGAIN,
	    eventfd_read(ctx->efd, &efd_value) == -1);

	ATF_REQUIRE(wait_barrier(ctx));

	ATF_REQUIRE(get_state(ctx) == 666);
	ATF_REQUIRE(select(ctx->efd + 1, &selfds, NULL, NULL, NULL) == 1);
	ATF_REQUIRE(FD_ISSET(ctx->efd, &selfds));
	set_state(ctx, 0);

	ATF_REQUIRE(wait_barrier(ctx));

	FD_ZERO(&selfds);
	FD_SET(ctx->efd, &selfds);

	ATF_REQUIRE_ERRNO(EAGAIN,
	    eventfd_write(ctx->efd, UINT64_MAX - 1) == -1);

	ATF_REQUIRE(wait_barrier(ctx));

	ATF_REQUIRE(get_state(ctx) == 666);
	ATF_REQUIRE(select(ctx->efd + 1, NULL, &selfds, NULL, NULL) == 1);
	ATF_REQUIRE(FD_ISSET(ctx->efd, &selfds));
	set_state(ctx, 0);

	ATF_REQUIRE(wait_barrier(ctx));

	/*
	 * Now, the same dance again, with kevent().
	 */
	ATF_REQUIRE((kq = kqueue()) >= 0);

	EV_SET(&kev[0], ctx->efd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, NULL);
	ATF_REQUIRE(kevent(kq, kev, 1, NULL, 0, NULL) == 0);

	ATF_REQUIRE_ERRNO(EAGAIN,
	    eventfd_read(ctx->efd, &efd_value) == -1);

	ATF_REQUIRE(wait_barrier(ctx));

	ATF_REQUIRE(get_state(ctx) == 666);
	ATF_REQUIRE(kevent(kq, NULL, 0, kev, 1, NULL) == 1);
	ATF_REQUIRE(kev[0].ident == (uintptr_t)ctx->efd);
	ATF_REQUIRE(kev[0].filter == EVFILT_READ);
	ATF_REQUIRE((kev[0].flags & (EV_EOF | EV_ERROR)) == 0);
	ATF_REQUIRE(kev[0].data == (int64_t)(UINT64_MAX - 1));
	set_state(ctx, 0);

	ATF_REQUIRE(wait_barrier(ctx));

	EV_SET(&kev[0], ctx->efd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0,
	       NULL);
	ATF_REQUIRE(kevent(kq, kev, 1, NULL, 0, NULL) == 0);

	ATF_REQUIRE_ERRNO(EAGAIN,
	    eventfd_write(ctx->efd, UINT64_MAX - 1) == -1);

	ATF_REQUIRE(wait_barrier(ctx));

	ATF_REQUIRE(get_state(ctx) == 666);
	ATF_REQUIRE(kevent(kq, NULL, 0, kev, 1, NULL) == 1);
	ATF_REQUIRE(kev[0].ident == (uintptr_t)ctx->efd);
	ATF_REQUIRE(kev[0].filter == EVFILT_WRITE);
	ATF_REQUIRE((kev[0].flags & (EV_EOF | EV_ERROR)) == 0);
	ATF_REQUIRE(kev[0].data == 0);
	set_state(ctx, 0);

	ATF_REQUIRE(wait_barrier(ctx));

	(void) close(kq);

	return NULL;
}

ATF_TC(eventfd_select_poll_kevent_block);
ATF_TC_HEAD(eventfd_select_poll_kevent_block, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "validates select/poll/kevent behavior - return after blocking");
}
ATF_TC_BODY(eventfd_select_poll_kevent_block, tc)
{
	struct helper_context ctx;
	pthread_t helper;
	eventfd_t efd_value;
	void *join_val;

	init_helper_context(&ctx);

	ATF_REQUIRE((ctx.efd = eventfd(0, EFD_NONBLOCK)) >= 0);

	ATF_REQUIRE(pthread_create(&helper, NULL,
				   eventfd_select_poll_kevent_block_helper,
				   &ctx) == 0);

	/*
	 * Wait for the helper to block in poll().  Give it some time
	 * so that if the poll returns immediately, we'll notice.
	 */
	set_state(&ctx, 666);
	ATF_REQUIRE(wait_barrier(&ctx));
	sleep(2);
	ATF_REQUIRE(get_state(&ctx) == 666);

	/*
	 * Write the max value to the eventfd so that it becomes readable
	 * and unblocks the helper waiting in poll().
	 */
	ATF_REQUIRE(eventfd_write(ctx.efd, UINT64_MAX - 1) == 0);

	/*
	 * Ensure the helper woke from the poll() call.
	 */
	ATF_REQUIRE(wait_barrier(&ctx));
	ATF_REQUIRE(get_state(&ctx) == 0);

	/*
	 * Wait for the helper to block in poll(), this time waiting
	 * for writability.
	 */
	set_state(&ctx, 666);
	ATF_REQUIRE(wait_barrier(&ctx));
	sleep(2);
	ATF_REQUIRE(get_state(&ctx) == 666);

	/*
	 * Now read the value, which will reset the eventfd to 0 and
	 * unblock the poll() call.
	 */
	ATF_REQUIRE(eventfd_read(ctx.efd, &efd_value) == 0);
	ATF_REQUIRE(efd_value == UINT64_MAX - 1);

	/*
	 * Ensure that the helper woke from the poll() call.
	 */
	ATF_REQUIRE(wait_barrier(&ctx));
	ATF_REQUIRE(get_state(&ctx) == 0);

	/*
	 * Wait for the helper to block in select(), waiting for readability.
	 */
	set_state(&ctx, 666);
	ATF_REQUIRE(wait_barrier(&ctx));
	sleep(2);
	ATF_REQUIRE(get_state(&ctx) == 666);

	/*
	 * Write the max value to the eventfd so that it becomes readable
	 * and unblocks the helper waiting in select().
	 */
	efd_value = UINT64_MAX - 1;
	ATF_REQUIRE(eventfd_write(ctx.efd, UINT64_MAX - 1) == 0);

	/*
	 * Ensure the helper woke from the select() call.
	 */
	ATF_REQUIRE(wait_barrier(&ctx));
	ATF_REQUIRE(get_state(&ctx) == 0);

	/*
	 * Wait for the helper to block in select(), this time waiting
	 * for writability.
	 */
	set_state(&ctx, 666);
	ATF_REQUIRE(wait_barrier(&ctx));
	sleep(2);
	ATF_REQUIRE(get_state(&ctx) == 666);

	/*
	 * Now read the value, which will reset the eventfd to 0 and
	 * unblock the select() call.
	 */
	ATF_REQUIRE(eventfd_read(ctx.efd, &efd_value) == 0);
	ATF_REQUIRE(efd_value == UINT64_MAX - 1);

	/*
	 * Ensure that the helper woke from the select() call.
	 */
	ATF_REQUIRE(wait_barrier(&ctx));
	ATF_REQUIRE(get_state(&ctx) == 0);

	/*
	 * Wait for the helper to block in kevent(), waiting for readability.
	 */
	set_state(&ctx, 666);
	ATF_REQUIRE(wait_barrier(&ctx));
	sleep(2);
	ATF_REQUIRE(get_state(&ctx) == 666);

	/*
	 * Write the max value to the eventfd so that it becomes readable
	 * and unblocks the helper waiting in kevent().
	 */
	efd_value = UINT64_MAX - 1;
	ATF_REQUIRE(eventfd_write(ctx.efd, UINT64_MAX - 1) == 0);

	/*
	 * Ensure the helper woke from the kevent() call.
	 */
	ATF_REQUIRE(wait_barrier(&ctx));
	ATF_REQUIRE(get_state(&ctx) == 0);

	/*
	 * Wait for the helper to block in kevent(), this time waiting
	 * for writability.
	 */
	set_state(&ctx, 666);
	ATF_REQUIRE(wait_barrier(&ctx));
	sleep(2);
	ATF_REQUIRE(get_state(&ctx) == 666);

	/*
	 * Now read the value, which will reset the eventfd to 0 and
	 * unblock the select() call.
	 */
	ATF_REQUIRE(eventfd_read(ctx.efd, &efd_value) == 0);
	ATF_REQUIRE(efd_value == UINT64_MAX - 1);

	/*
	 * Ensure that the helper woke from the kevent() call.
	 */
	ATF_REQUIRE(wait_barrier(&ctx));
	ATF_REQUIRE(get_state(&ctx) == 0);

	/* Reap the helper. */
	ATF_REQUIRE(pthread_join(helper, &join_val) == 0);

	(void) close(ctx.efd);
}

/*****************************************************************************/

static void *
eventfd_restart_helper(void * const v)
{
	struct helper_context * const ctx = v;
	eventfd_t efd_value;

	/*
	 * Issue a single read to ensure that the descriptor is valid.
	 * Thius will not block because it was created with an initial
	 * count of 1.
	 */
	ATF_REQUIRE(eventfd_read(ctx->efd, &efd_value) == 0);
	ATF_REQUIRE(efd_value == 1);

	ATF_REQUIRE(wait_barrier(ctx));

	/*
	 * Block in read.  The main thread will close the descriptor,
	 * which should unblock us and result in EBADF.
	 */
	ATF_REQUIRE(get_state(ctx) == 666);
	ATF_REQUIRE_ERRNO(EBADF, eventfd_read(ctx->efd, &efd_value) == -1);
	set_state(ctx, 0);

	ATF_REQUIRE(wait_barrier(ctx));

	return NULL;
}

ATF_TC(eventfd_restart);
ATF_TC_HEAD(eventfd_restart, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "exercises the 'restart' fileop code path");
}
ATF_TC_BODY(eventfd_restart, tc)
{
	struct helper_context ctx;
	pthread_t helper;
	void *join_val;

	init_helper_context(&ctx);

	ATF_REQUIRE((ctx.efd = eventfd(1, 0)) >= 0);

	ATF_REQUIRE(pthread_create(&helper, NULL,
				   eventfd_restart_helper, &ctx) == 0);

	/*
	 * Wait for the helper to block in read().  Give it some time
	 * so that if the poll returns immediately, we'll notice.
	 */
	set_state(&ctx, 666);
	ATF_REQUIRE(wait_barrier(&ctx));
	sleep(2);
	ATF_REQUIRE(get_state(&ctx) == 666);

	/*
	 * Close the descriptor.  This should unblock the reader,
	 * and cause it to receive EBADF.
	 */
	ATF_REQUIRE(close(ctx.efd) == 0);

	/*
	 * Ensure that the helper woke from the read() call.
	 */
	ATF_REQUIRE(wait_barrier(&ctx));
	ATF_REQUIRE(get_state(&ctx) == 0);

	/* Reap the helper. */
	ATF_REQUIRE(pthread_join(helper, &join_val) == 0);
}

/*****************************************************************************/

ATF_TC(eventfd_badflags);
ATF_TC_HEAD(eventfd_badflags, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "validates behavior when eventfd() called with bad flags");
}
ATF_TC_BODY(eventfd_badflags, tc)
{
	ATF_REQUIRE_ERRNO(EINVAL,
	    eventfd(0, ~(EFD_SEMAPHORE | EFD_CLOEXEC | EFD_NONBLOCK)) == -1);
}

/*****************************************************************************/

ATF_TC(eventfd_bufsize);
ATF_TC_HEAD(eventfd_bufsize, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "validates expected buffer size behavior");
}
ATF_TC_BODY(eventfd_bufsize, tc)
{
	eventfd_t efd_value[2];
	int efd;

	ATF_REQUIRE((efd = eventfd(1, EFD_NONBLOCK)) >= 0);

	ATF_REQUIRE_ERRNO(EINVAL,
	    read(efd, efd_value, sizeof(efd_value[0]) - 1) == -1);

	efd_value[0] = 0xdeadbeef;
	efd_value[1] = 0xdeadbeef;
	ATF_REQUIRE(read(efd, efd_value, sizeof(efd_value)) ==
	    sizeof(efd_value[0]));
	ATF_REQUIRE(efd_value[0] == 1);
	ATF_REQUIRE(efd_value[1] == 0xdeadbeef);

	ATF_REQUIRE_ERRNO(EINVAL,
	    write(efd, efd_value, sizeof(efd_value[0]) - 1) == -1);
	ATF_REQUIRE(write(efd, efd_value, sizeof(efd_value)) ==
	    sizeof(efd_value[0]));

	ATF_REQUIRE(read(efd, efd_value, sizeof(efd_value)) ==
	    sizeof(efd_value[0]));
	ATF_REQUIRE(efd_value[0] == 1);
	ATF_REQUIRE(efd_value[1] == 0xdeadbeef);

	(void) close(efd);
}

/*****************************************************************************/

ATF_TC(eventfd_fcntl);
ATF_TC_HEAD(eventfd_fcntl, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "validates fcntl behavior");
}
ATF_TC_BODY(eventfd_fcntl, tc)
{
	int efd;
	int val;

	ATF_REQUIRE((efd = eventfd(1, 0)) >= 0);
	ATF_REQUIRE((fcntl(efd, F_GETFL) & O_NONBLOCK) == 0);
	ATF_REQUIRE(fcntl(efd, F_SETFL, O_NONBLOCK) == 0);
	ATF_REQUIRE((fcntl(efd, F_GETFL) & O_NONBLOCK) != 0);
	ATF_REQUIRE((fcntl(efd, F_GETFD) & FD_CLOEXEC) == 0);

	ATF_REQUIRE(ioctl(efd, FIONREAD, &val) == 0);
	ATF_REQUIRE(val == sizeof(eventfd_t));

	ATF_REQUIRE(ioctl(efd, FIONWRITE, &val) == 0);
	ATF_REQUIRE(val == 0);

	ATF_REQUIRE_ERRNO(ENOTTY, ioctl(efd, FIONSPACE, &val) == -1);
	(void)close(efd);

	ATF_REQUIRE((efd = eventfd(1, EFD_NONBLOCK | EFD_CLOEXEC)) >= 0);
	ATF_REQUIRE((fcntl(efd, F_GETFL) & ~O_ACCMODE) == O_NONBLOCK);
	ATF_REQUIRE((fcntl(efd, F_GETFD) & FD_CLOEXEC) != 0);
	ATF_REQUIRE(fcntl(efd, F_SETFD, 0) == 0);
	ATF_REQUIRE((fcntl(efd, F_GETFD) & FD_CLOEXEC) == 0);
	ATF_REQUIRE(fcntl(efd, F_SETFD, FD_CLOEXEC) == 0);
	ATF_REQUIRE((fcntl(efd, F_GETFD) & FD_CLOEXEC) != 0);
	(void)close(efd);
}

/*****************************************************************************/

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, eventfd_normal);
	ATF_TP_ADD_TC(tp, eventfd_semaphore);
	ATF_TP_ADD_TC(tp, eventfd_badflags);
	ATF_TP_ADD_TC(tp, eventfd_bufsize);
	ATF_TP_ADD_TC(tp, eventfd_select_poll_kevent_immed);
	ATF_TP_ADD_TC(tp, eventfd_select_poll_kevent_block);
	ATF_TP_ADD_TC(tp, eventfd_restart);
	ATF_TP_ADD_TC(tp, eventfd_fcntl);

	return atf_no_error();
}
