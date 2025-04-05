/*	$NetBSD: t_compat_cancel.c,v 1.1 2025/04/05 11:22:32 riastradh Exp $	*/

/*
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

#define	__LIBC12_SOURCE__	/* expose compat declarations */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_compat_cancel.c,v 1.1 2025/04/05 11:22:32 riastradh Exp $");

#include <sys/event.h>
#include <sys/mman.h>

#include <aio.h>
#include <atf-c.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>

#include <compat/sys/event.h>
#include <compat/sys/mman.h>
#include <compat/sys/poll.h>
#include <compat/sys/select.h>

#include <compat/include/aio.h>
#include <compat/include/mqueue.h>
#include <compat/include/signal.h>
#include <compat/include/time.h>

#include "cancelpoint.h"
#include "h_macros.h"

pthread_barrier_t bar;
bool cleanup_done;

static void
cancelpoint_compat100_kevent(void)
{
	int kq;
	struct kevent100 ev;

	memset(&ev, 0, sizeof(ev));
	ev.ident = SIGUSR1;
	ev.filter = EVFILT_SIGNAL;
	ev.flags = EV_ADD|EV_ENABLE;
	ev.fflags = 0;
	ev.data = 0;
	ev.udata = 0;

	RL(kq = kqueue());
	RL(__kevent50(kq, &ev, 1, NULL, 1, &(const struct timespec){0,0}));
	cancelpointready();
	RL(__kevent50(kq, NULL, 0, &ev, 1, NULL));
}

static void
cancelpoint_compat13_msync(void)
{
	const unsigned long pagesize = sysconf(_SC_PAGESIZE);
	int fd;
	void *map;

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	RL(ftruncate(fd, pagesize));
	REQUIRE_LIBC(map = mmap(NULL, pagesize, PROT_READ|PROT_WRITE,
		MAP_SHARED, fd, 0),
	    MAP_FAILED);
	cancelpointready();
	RL(msync(map, pagesize));
}

static void
cancelpoint_compat50___sigtimedwait(void)
{
	sigset_t mask, omask;
	siginfo_t info;
	struct timespec50 t = {.tv_sec = 2, .tv_nsec = 0};

	RL(__sigfillset14(&mask));
	RL(__sigprocmask14(SIG_BLOCK, &mask, &omask));
	cancelpointready();
	RL(__sigtimedwait(&omask, &info, &t));
}

static void
cancelpoint_compat50_aio_suspend(void)
{
	int fd[2];
	char buf[32];
	struct aiocb aio = {
		.aio_offset = 0,
		.aio_buf = buf,
		.aio_nbytes = sizeof(buf),
		.aio_fildes = -1,
	};
	const struct aiocb *const aiolist[] = { &aio };

	RL(pipe(fd));
	aio.aio_fildes = fd[0];
	RL(aio_read(&aio));
	cancelpointready();
	RL(aio_suspend(aiolist, __arraycount(aiolist), NULL));
}

static void
cancelpoint_compat50_kevent(void)
{
	int kq;
	struct kevent100 ev;

	memset(&ev, 0, sizeof(ev));
	ev.ident = SIGUSR1;
	ev.filter = EVFILT_SIGNAL;
	ev.flags = EV_ADD|EV_ENABLE;
	ev.fflags = 0;
	ev.data = 0;
	ev.udata = 0;

	RL(kq = kqueue());
	RL(kevent(kq, &ev, 1, NULL, 1, &(const struct timespec50){0,0}));
	cancelpointready();
	RL(kevent(kq, NULL, 0, &ev, 1, NULL));
}

static void
cancelpoint_compat50_mq_timedreceive(void)
{
	mqd_t mq;
	char buf[32];
	struct timespec50 t = {.tv_sec = 2, .tv_nsec = 0};

	RL(mq = mq_open("mq", O_RDWR|O_CREAT, 0666, NULL));
	cancelpointready();
	RL(mq_timedreceive(mq, buf, sizeof(buf), NULL, &t));
}

static void
cancelpoint_compat50_mq_timedsend(void)
{
	mqd_t mq;
	char buf[32] = {0};
	struct timespec50 t = {.tv_sec = 2, .tv_nsec = 0};

	RL(mq = mq_open("mq", O_RDWR|O_CREAT, 0666, NULL));
	cancelpointready();
	RL(mq_timedsend(mq, buf, sizeof(buf), 0, &t));
}

static void
cancelpoint_compat50_nanosleep(void)
{
	struct timespec50 t = {.tv_sec = 2, .tv_nsec = 0};

	cancelpointready();
	RL(nanosleep(&t, NULL));
}

static void
cancelpoint_compat50_pollts(void)
{
	int fd[2];
	struct pollfd pfd;
	struct timespec50 t = {.tv_sec = 2, .tv_nsec = 0};

	RL(pipe(fd));
	pfd.fd = fd[0];
	pfd.events = POLLIN;
	cancelpointready();
	RL(pollts(&pfd, 1, &t, NULL));
}

static void
cancelpoint_compat50_pselect(void)
{
	int fd[2];
	fd_set readfd;
	struct timespec50 t = {.tv_sec = 2, .tv_nsec = 0};

	FD_ZERO(&readfd);

	RL(pipe(fd));
	FD_SET(fd[0], &readfd);
	cancelpointready();
	RL(pselect(fd[0] + 1, &readfd, NULL, NULL, &t, NULL));
}

static void
cancelpoint_compat50_select(void)
{
	int fd[2];
	fd_set readfd;
	struct timeval50 t = {.tv_sec = 1, .tv_usec = 0};

	FD_ZERO(&readfd);

	RL(pipe(fd));
	FD_SET(fd[0], &readfd);
	cancelpointready();
	RL(select(fd[0] + 1, &readfd, NULL, NULL, &t));
}

static void
cancelpoint_compat14_sigsuspend(void)
{
	sigset13_t mask, omask;

	RL(sigfillset(&mask));
	RL(sigprocmask(SIG_BLOCK, &mask, &omask));
	cancelpointready();
	RL(sigsuspend(&omask));
}

static void
cancelpoint_compat50_sigtimedwait(void)
{
	sigset_t mask, omask;
	siginfo_t info;
	struct timespec50 t = {.tv_sec = 2, .tv_nsec = 0};

	RL(__sigfillset14(&mask));
	RL(__sigprocmask14(SIG_BLOCK, &mask, &omask));
	cancelpointready();
	RL(sigtimedwait(&omask, &info, &t));
}

TEST_CANCELPOINT(cancelpoint_compat100_kevent, __nothing)
TEST_CANCELPOINT(cancelpoint_compat13_msync, __nothing)
TEST_CANCELPOINT(cancelpoint_compat14_sigsuspend,
    atf_tc_expect_signal(SIGALRM, "PR lib/59240: POSIX.1-2024:"
	" cancellation point audit"))
TEST_CANCELPOINT(cancelpoint_compat50___sigtimedwait,
    atf_tc_expect_signal(SIGALRM, "PR lib/59240: POSIX.1-2024:"
	" cancellation point audit"))
TEST_CANCELPOINT(cancelpoint_compat50_aio_suspend, __nothing)
TEST_CANCELPOINT(cancelpoint_compat50_kevent, __nothing)
TEST_CANCELPOINT(cancelpoint_compat50_mq_timedreceive, __nothing)
TEST_CANCELPOINT(cancelpoint_compat50_mq_timedsend, __nothing)
TEST_CANCELPOINT(cancelpoint_compat50_nanosleep, __nothing)
TEST_CANCELPOINT(cancelpoint_compat50_pollts, __nothing)
TEST_CANCELPOINT(cancelpoint_compat50_pselect, __nothing)
TEST_CANCELPOINT(cancelpoint_compat50_select, __nothing)
TEST_CANCELPOINT(cancelpoint_compat50_sigtimedwait,
    atf_tc_expect_signal(SIGALRM, "PR lib/59240: POSIX.1-2024:"
	" cancellation point audit"))

ATF_TP_ADD_TCS(tp)
{

	ADD_TEST_CANCELPOINT(cancelpoint_compat100_kevent);
	ADD_TEST_CANCELPOINT(cancelpoint_compat13_msync);
	ADD_TEST_CANCELPOINT(cancelpoint_compat14_sigsuspend);
	ADD_TEST_CANCELPOINT(cancelpoint_compat50___sigtimedwait);
	ADD_TEST_CANCELPOINT(cancelpoint_compat50_aio_suspend);
	ADD_TEST_CANCELPOINT(cancelpoint_compat50_kevent);
	ADD_TEST_CANCELPOINT(cancelpoint_compat50_mq_timedreceive);
	ADD_TEST_CANCELPOINT(cancelpoint_compat50_mq_timedsend);
	ADD_TEST_CANCELPOINT(cancelpoint_compat50_nanosleep);
	ADD_TEST_CANCELPOINT(cancelpoint_compat50_pollts);
	ADD_TEST_CANCELPOINT(cancelpoint_compat50_pselect);
	ADD_TEST_CANCELPOINT(cancelpoint_compat50_select);
	ADD_TEST_CANCELPOINT(cancelpoint_compat50_sigtimedwait);

	return atf_no_error();
}
