/*	$NetBSD: t_sigio.c,v 1.1 2026/09/24 22:10:22 riastradh Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_sigio.c,v 1.1 2026/09/24 22:10:22 riastradh Exp $");

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "h_macros.h"

static int expected_si_fd = -1;
static int expected_si_code = -1;
static long expected_si_band = 0;
static long si_band_mask = 0;
static int siginfo_received;

static void
formatbit(char **bufp, size_t *lenp, bool *firstp, const char *name)
{
	int n;
	size_t k;

	n = snprintf(*bufp, *lenp, "%s%s", *firstp ? "" : ",", name);
	*firstp = false;
	k = ((unsigned)n >= *lenp ? *lenp : (unsigned)n);
	*bufp += k;
	*lenp -= k;
}

static char *
formatpollevents(char *buf, size_t len, long events)
{
	char *start = buf;
	bool first = true;
	int n;

	n = snprintf(buf, len, "0x%lx<", events);
	if ((unsigned)n >= len)
		goto out;
	buf += (unsigned)n;
	len -= (unsigned)n;
	if (events & POLLIN)
		formatbit(&buf, &len, &first, "POLLIN");
	if (events & POLLOUT)
		formatbit(&buf, &len, &first, "POLLOUT");
	if (events & POLLHUP)
		formatbit(&buf, &len, &first, "POLLHUP");
	if (events & POLLERR)
		formatbit(&buf, &len, &first, "POLLERR");
	if (events & POLLRDNORM)
		formatbit(&buf, &len, &first, "POLLRDNORM");
	if (events & POLLWRNORM)
		formatbit(&buf, &len, &first, "POLLWRNORM");
	if (events & POLLRDBAND)
		formatbit(&buf, &len, &first, "POLLRDBAND");
	if (events & POLLWRBAND)
		formatbit(&buf, &len, &first, "POLLWRBAND");
	if (events & POLLNVAL)
		formatbit(&buf, &len, &first, "POLLNVAL");
	snprintf(buf, len, ">");
out:	return start;
}

struct context {
	pthread_barrier_t *bar;
	int fd;
	int lowat;
};

static void *
start_closer(void *cookie)
{
	struct context *C = cookie;
	sigset_t mask;

	RL(sigemptyset(&mask));
	RL(sigaddset(&mask, SIGIO));
	RL(pthread_sigmask(SIG_BLOCK, &mask, NULL));

	fprintf(stderr, "[thread] waiting for barrier\n");
	(void)pthread_barrier_wait(C->bar);
	fprintf(stderr, "[thread] giving other thread a head start\n");
	RL(usleep(1));
	fprintf(stderr, "[thread] closing\n");
	RL(close(C->fd));
	fprintf(stderr, "[thread] closed\n");

	return NULL;
}

static void *
start_shutdownrd(void *cookie)
{
	struct context *C = cookie;
	sigset_t mask;

	RL(sigemptyset(&mask));
	RL(sigaddset(&mask, SIGIO));
	RL(pthread_sigmask(SIG_BLOCK, &mask, NULL));

	fprintf(stderr, "[thread] waiting for barrier\n");
	(void)pthread_barrier_wait(C->bar);
	fprintf(stderr, "[thread] giving other thread a head start\n");
	RL(usleep(1));
	fprintf(stderr, "[thread] shutdown(SHUT_RD)\n");
	RL(shutdown(C->fd, SHUT_RD));
	fprintf(stderr, "[thread] shutdown\n");

	return NULL;
}

static void *
start_shutdownwr(void *cookie)
{
	struct context *C = cookie;
	sigset_t mask;

	RL(sigemptyset(&mask));
	RL(sigaddset(&mask, SIGIO));
	RL(pthread_sigmask(SIG_BLOCK, &mask, NULL));

	fprintf(stderr, "[thread] waiting for barrier\n");
	(void)pthread_barrier_wait(C->bar);
	fprintf(stderr, "[thread] giving other thread a head start\n");
	RL(usleep(1));
	fprintf(stderr, "[thread] shutdown(SHUT_WR)\n");
	RL(shutdown(C->fd, SHUT_WR));
	fprintf(stderr, "[thread] shutdown\n");

	return NULL;
}

static void *
start_reader(void *cookie)
{
	struct context *C = cookie;
	sigset_t mask;
	char *buf;
	ssize_t nread;
	size_t resid = C->lowat;

	REQUIRE_LIBC(buf = malloc(C->lowat), NULL);

	RL(sigemptyset(&mask));
	RL(sigaddset(&mask, SIGIO));
	RL(pthread_sigmask(SIG_BLOCK, &mask, NULL));

	fprintf(stderr, "[thread] waiting for barrier\n");
	(void)pthread_barrier_wait(C->bar);
	fprintf(stderr, "[thread] giving other thread a head start\n");
	RL(usleep(1));
	fprintf(stderr, "[thread] reading first byte\n");
	RL(nread = read(C->fd, buf, 1));
	fprintf(stderr, "[thread] read first byte\n");
	ATF_CHECK_EQ_MSG(nread, 1, "nread=%zd", nread);
	resid -= (size_t)nread;
	RL(usleep(1));
	fprintf(stderr, "[thread] slept\n");

	if (C->lowat <= 1) {
		fprintf(stderr, "[thread] lowat=1, our job is done\n");
		goto out;
	}

	/*
	 * Reading a single byte out of the buffer shouldn't be enough
	 * to deliver SIGIO -- only clearing space for C->lowat bytes
	 * should be.
	 */
	ATF_CHECK_EQ_MSG(siginfo_received, 0, "siginfo_received=%d",
	    siginfo_received);

	while (resid > 0) {
		fprintf(stderr, "[thread] reading more bytes, %zu remain\n",
		    resid);
		RL(nread = read(C->fd, buf + C->lowat - resid, resid));
		fprintf(stderr, "[thread] read %zd bytes\n", nread);
		ATF_REQUIRE_MSG((size_t)nread <= resid, "nread=%zd resid=%zu",
		    nread, resid);
		resid -= (size_t)nread;
	}

out:	fprintf(stderr, "[thread] done reading\n");
	free(buf);
	return NULL;
}

static void *
start_writer(void *cookie)
{
	struct context *C = cookie;
	sigset_t mask;
	char ch = 1;
	ssize_t nwrit;

	RL(sigemptyset(&mask));
	RL(sigaddset(&mask, SIGIO));
	RL(pthread_sigmask(SIG_BLOCK, &mask, NULL));

	fprintf(stderr, "[thread] waiting for barrier\n");
	(void)pthread_barrier_wait(C->bar);
	fprintf(stderr, "[thread] giving other thread a head start\n");
	RL(usleep(1));
	fprintf(stderr, "[thread] writing\n");
	RL(nwrit = write(C->fd, &ch, 1));
	fprintf(stderr, "[thread] wrote %zd bytes\n", nwrit);
	ATF_CHECK_EQ_MSG(nwrit, 1, "nwrit=%zd", nwrit);

	return NULL;
}

static void
ptysetup(int *hostfd, int *appfd)
{
	struct termios t;
	char *pts;

	RL(*hostfd = posix_openpt(O_RDWR|O_NOCTTY));
	RL(grantpt(*hostfd));
	RL(unlockpt(*hostfd));
	REQUIRE_LIBC(pts = ptsname(*hostfd), NULL);
	RL(*appfd = open(pts, O_RDWR|O_NOCTTY));

	RL(tcgetattr(*appfd, &t));
	t.c_lflag &= ~ICANON;	/* block rather than drop input */
	RL(tcsetattr(*appfd, TCSANOW, &t));
}

static void
socksetup(int s[static 2], int af, const char *host)
{
	struct addrinfo *ai, hints = {
		.ai_family = af,
		.ai_socktype = SOCK_STREAM,
		.ai_flags = AI_NUMERICHOST|AI_PASSIVE,
	};
	union {
		struct sockaddr sa;
		struct sockaddr_storage ss;
	} addr;
	socklen_t addrlen = sizeof(addr);
	int server = -1;
	int error;

	error = getaddrinfo(host, NULL, &hints, &ai);
	ATF_REQUIRE_MSG(error == 0, "getaddrinfo: %d (%s)", error,
	    gai_strerror(errno));
	RL(server = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol));
	RL(bind(server, ai->ai_addr, ai->ai_addrlen));
	RL(getsockname(server, &addr.sa, &addrlen));
	RL(listen(server, 1));
	RL(s[0] = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol));
	RL(connect(s[0], &addr.sa, addrlen));
	RL(s[1] = accept(server, NULL, NULL));
	RL(close(server));
	freeaddrinfo(ai);
}

static void
fillpipebuf(int writefd)
{
	char buf[BUFSIZ] = {0};
	size_t n = 0;
	ssize_t nwrit;
	int flags;

	RL(flags = fcntl(writefd, F_GETFL));
	RL(fcntl(writefd, F_SETFL, flags|O_NONBLOCK));
	while ((nwrit = write(writefd, buf, sizeof(buf))) != -1)
		n += (size_t)nwrit;
	ATF_CHECK_EQ_MSG(errno, EAGAIN, "errno=%d", errno);
	RL(fcntl(writefd, F_SETFL, flags));
	fprintf(stderr, "filled %d with %zu bytes\n", writefd, n);
}

static void
check_write(int writefd)
{
	int flags;
	char c = 0;
	ssize_t nwrit;

	fprintf(stderr, "checking that we can write a byte...\n");
	RL(flags = fcntl(writefd, F_GETFL));
	RL(fcntl(writefd, F_SETFL, flags|O_NONBLOCK));
	RL(nwrit = write(writefd, &c, 1));
	ATF_CHECK_EQ_MSG(nwrit, 1, "nwrit=%zd", nwrit);
	RL(fcntl(writefd, F_SETFL, flags));
}

static void
check_sendnonblock(int sendfd)
{
	char c = 0;
	ssize_t nsent;

	fprintf(stderr, "checking that we can send a single byte...\n");
	RL(nsent = send(sendfd, &c, 1, MSG_DONTWAIT));
	ATF_CHECK_EQ_MSG(nsent, 1, "nsent=%zd", nsent);
}

static void
check_read(int readfd)
{
	char c;
	ssize_t nread;

	fprintf(stderr, "checking that we can read a byte...\n");
	RL(nread = read(readfd, &c, 1));
	ATF_CHECK_EQ_MSG(nread, 1, "nread=%zd", nread);
}

static void
check_write_fail(int writefd, int error)
{
	int flags;
	void (*sighandler)(int);
	char c = 0;
	ssize_t nwrit;

	fprintf(stderr, "checking that write fails with %d (%s)...\n",
	    error, strerror(error));

	RL(flags = fcntl(writefd, F_GETFL));
	RL(fcntl(writefd, F_SETFL, flags|O_NONBLOCK));

	REQUIRE_LIBC(sighandler = signal(SIGPIPE, SIG_IGN), SIG_ERR);
	ATF_CHECK_ERRNO(error, (nwrit = write(writefd, &c, 1)) == -1);
	ATF_CHECK_EQ_MSG(nwrit, -1, "nwrit=%zd", nwrit);
	REQUIRE_LIBC(signal(SIGPIPE, sighandler), SIG_ERR);

	RL(fcntl(writefd, F_SETFL, flags));
}

static void
check_read_eof(int readfd)
{
	int flags;
	char c;
	ssize_t nread;

	fprintf(stderr, "checking that read returns EOF...\n");

	RL(flags = fcntl(readfd, F_GETFL));
	RL(fcntl(readfd, F_SETFL, flags|O_NONBLOCK));

	RL(nread = read(readfd, &c, 1));
	ATF_CHECK_EQ_MSG(nread, 0, "nread=%zu", nread);

	RL(fcntl(readfd, F_SETFL, flags));
}

static void
check_sigio(int signo, siginfo_t *si)
{
	char actbuf[128], expbuf[128];

	fprintf(stderr, "[signal] signo=%d (%s)"
	    " si_code=%d si_fd=%d si_band=%s\n",
	    signo, strsignal(signo),
	    si->si_code, si->si_fd,
	    formatpollevents(actbuf, sizeof(actbuf), si->si_band));
	ATF_CHECK_EQ_MSG(signo, SIGIO, "signo=%d (%s)",
	    signo, strsignal(signo));
	ATF_CHECK_EQ_MSG(si->si_signo, SIGIO, "signo=%d (%s)",
	    si->si_signo, strsignal(si->si_signo));
	if (expected_si_fd != -1) {
		ATF_CHECK_EQ_MSG(si->si_fd, expected_si_fd,
		    "si_fd=%d, expected %d",
		    si->si_fd, expected_si_fd);
	}
	if (expected_si_code != -1) {
		ATF_CHECK_EQ_MSG(si->si_code, expected_si_code,
		    "si_code=%d, expected %d",
		    si->si_code, expected_si_code);
	}
	ATF_CHECK_EQ_MSG((si->si_band & si_band_mask), expected_si_band,
	    "si_band=%s, expected %s",
	    formatpollevents(actbuf, sizeof(actbuf), si->si_band),
	    formatpollevents(expbuf, sizeof(expbuf), expected_si_band));
}

static void
on_sigio(int signo, siginfo_t *si, void *ctx)
{

	check_sigio(signo, si);
	siginfo_received++;
}

static void
setup(sigset_t *omask)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = &on_sigio;
	RL(sigemptyset(&sa.sa_mask));
	RL(sigaddset(&sa.sa_mask, SIGIO));
	RL(pthread_sigmask(SIG_BLOCK, &sa.sa_mask, omask));
	RL(sigaction(SIGIO, &sa, NULL));
}

static void
wait_for_io(sigset_t *omask)
{

	ATF_CHECK_ERRNO(EINTR, sigsuspend(omask) == -1);
	ATF_CHECK_EQ_MSG(siginfo_received, 1, "siginfo_received=%d",
	    siginfo_received);
}

ATF_TC(fifo_read);
ATF_TC_HEAD(fifo_read, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test SIGIO when fifo is readable");
}
ATF_TC_BODY(fifo_read, tc)
{
	sigset_t omask;
	int fiford, fifowr, flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(mkfifo("fifo", 0600));
	RL(fiford = open("fifo", O_RDONLY|O_NONBLOCK));
	RL(flags = fcntl(fiford, F_GETFL));
	RL(fcntl(fiford, F_SETFL, flags & ~O_ASYNC));
	RL(fifowr = open("fifo", O_WRONLY));

	RL(fcntl(fiford, F_SETOWN, getpid()));
	RL(flags = fcntl(fiford, F_GETFL));
	RL(fcntl(fiford, F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = fifowr;
	RZ(pthread_create(&t, NULL, &start_writer, C));

	expected_si_fd = fiford;
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	/*
	 * SIGIO is delivered with wrong si_fd = -1, but correct
	 * si_code and si_band.
	 */
	atf_tc_expect_fail("PR kern/60785:"
	    " fifo delivers SIGIO with wrong si_fd");

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read(fiford);

	RZ(pthread_join(t, NULL));
}

ATF_TC(fifo_read_closed);
ATF_TC_HEAD(fifo_read_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when fifo is readable because closed");
}
ATF_TC_BODY(fifo_read_closed, tc)
{
	sigset_t omask;
	int fiford, fifowr, flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(mkfifo("fifo", 0600));
	RL(fiford = open("fifo", O_RDONLY|O_NONBLOCK));
	RL(flags = fcntl(fiford, F_GETFL));
	RL(fcntl(fiford, F_SETFL, flags & ~O_ASYNC));
	RL(fifowr = open("fifo", O_WRONLY));

	RL(fcntl(fifowr, F_SETOWN, getpid()));
	RL(flags = fcntl(fifowr, F_GETFL));
	RL(fcntl(fifowr, F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = fifowr;
	RZ(pthread_create(&t, NULL, &start_closer, C));

	expected_si_fd = fiford;
	expected_si_code = POLL_HUP;
	expected_si_band = POLLHUP;
	si_band_mask = ~0;

	/*
	 * Missing wakeups similar to some poll bugs.  Not really the
	 * same issue but we're tracking a lot of related stuff
	 * there...
	 */
	atf_tc_expect_signal(SIGALRM, "PR kern/59056: poll POLLHUP bugs");

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read_eof(fiford);

	RZ(pthread_join(t, NULL));
}

ATF_TC(fifo_write);
ATF_TC_HEAD(fifo_write, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test SIGIO when fifo is writable");
}
ATF_TC_BODY(fifo_write, tc)
{
	sigset_t omask;
	int fiford, fifowr, flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(mkfifo("fifo", 0600));
	RL(fiford = open("fifo", O_RDONLY|O_NONBLOCK));
	RL(flags = fcntl(fiford, F_GETFL));
	RL(fcntl(fiford, F_SETFL, flags & ~O_ASYNC));
	RL(fifowr = open("fifo", O_WRONLY));

	fillpipebuf(fifowr);
	RL(fcntl(fifowr, F_SETOWN, getpid()));
	RL(flags = fcntl(fifowr, F_GETFL));
	RL(fcntl(fifowr, F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = fiford;
	C->lowat = PIPE_BUF;
	RZ(pthread_create(&t, NULL, &start_reader, C));

	expected_si_fd = fifowr;
	expected_si_code = POLL_OUT;
	expected_si_band = POLLOUT|POLLWRNORM;
	si_band_mask = ~0;

	/*
	 * SIGIO is delivered with wrong si_fd = -1, but correct
	 * si_code and si_band.  Also delivered too early.
	 */
	atf_tc_expect_fail("PR kern/60785:"
	    " fifo delivers SIGIO with wrong si_fd"
	    "; "
	    "PR kern/60782:"
	    " SIGIO says socket ready for send before send does");

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_write(fifowr);

	RZ(pthread_join(t, NULL));
}

ATF_TC(fifo_write_closed);
ATF_TC_HEAD(fifo_write_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when fifo is writable because closed");
}
ATF_TC_BODY(fifo_write_closed, tc)
{
	sigset_t omask;
	int fiford, fifowr, flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(mkfifo("fifo", 0600));
	RL(fiford = open("fifo", O_RDONLY|O_NONBLOCK));
	RL(flags = fcntl(fiford, F_GETFL));
	RL(fcntl(fiford, F_SETFL, flags & ~O_ASYNC));
	RL(fifowr = open("fifo", O_WRONLY));

	fillpipebuf(fifowr);
	RL(fcntl(fifowr, F_SETOWN, getpid()));
	RL(flags = fcntl(fifowr, F_GETFL));
	RL(fcntl(fifowr, F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = fiford;
	RZ(pthread_create(&t, NULL, &start_closer, C));

	expected_si_fd = fifowr;
	expected_si_code = POLL_ERR;
	expected_si_band = POLLERR;
	si_band_mask = ~0;

	/*
	 * SIGIO is delivered with:
	 *
	 * - si_fd = -1
	 * - si_code = POLL_OUT (2)
	 * - si_band = POLLOUT|POLLWRNORM (0x40, POLLWRNORM is just an alias)
	 *
	 * which (except for the fd part) is consistent with sockets
	 * but not consistent with the POSIX.1-2024 definition of
	 * POLLERR, and is the same issue as various poll-related
	 * POLLHUP things.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_write_fail(fifowr, EPIPE);

	RZ(pthread_join(t, NULL));
}

ATF_TC(pipe_read);
ATF_TC_HEAD(pipe_read, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test SIGIO when pipe is readable");
}
ATF_TC_BODY(pipe_read, tc)
{
	sigset_t omask;
	int pipefd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(pipe(pipefd));
	RL(fcntl(pipefd[0], F_SETOWN, getpid()));
	RL(flags = fcntl(pipefd[0], F_GETFL));
	RL(fcntl(pipefd[0], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = pipefd[1];
	RZ(pthread_create(&t, NULL, &start_writer, C));

	expected_si_fd = pipefd[0];
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read(pipefd[0]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(pipe_read_closed);
ATF_TC_HEAD(pipe_read_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when pipe is readable because closed");
}
ATF_TC_BODY(pipe_read_closed, tc)
{
	sigset_t omask;
	int pipefd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(pipe(pipefd));
	RL(fcntl(pipefd[0], F_SETOWN, getpid()));
	RL(flags = fcntl(pipefd[0], F_GETFL));
	RL(fcntl(pipefd[0], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = pipefd[1];
	RZ(pthread_create(&t, NULL, &start_closer, C));

	expected_si_fd = pipefd[0];
	expected_si_code = POLL_HUP;
	expected_si_band = POLLHUP;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read_eof(pipefd[0]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(pipe_write);
ATF_TC_HEAD(pipe_write, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test SIGIO when pipe is writable");
}
ATF_TC_BODY(pipe_write, tc)
{
	sigset_t omask;
	int pipefd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(pipe(pipefd));
	fillpipebuf(pipefd[1]);
	RL(fcntl(pipefd[1], F_SETOWN, getpid()));
	RL(flags = fcntl(pipefd[1], F_GETFL));
	RL(fcntl(pipefd[1], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = pipefd[0];
	C->lowat = PIPE_BUF;
	RZ(pthread_create(&t, NULL, &start_reader, C));

	expected_si_fd = pipefd[1];
	expected_si_code = POLL_OUT;
	expected_si_band = POLLOUT|POLLWRNORM;
	si_band_mask = ~0;

	/*
	 * OK, not quite the same bug but we're tracking a bunch of
	 * related issues there...  The wrong fd is reported in SIGIO,
	 * because the sys_pipe.c logic has a bunch of variables
	 * backwards and confused about which side of the pipe is
	 * which.
	 */
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_write(pipefd[1]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(pipe_write_closed);
ATF_TC_HEAD(pipe_write_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when pipe is writable because closed");
}
ATF_TC_BODY(pipe_write_closed, tc)
{
	sigset_t omask;
	int pipefd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(pipe(pipefd));
	fillpipebuf(pipefd[1]);
	RL(fcntl(pipefd[1], F_SETOWN, getpid()));
	RL(flags = fcntl(pipefd[1], F_GETFL));
	RL(fcntl(pipefd[1], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = pipefd[0];
	RZ(pthread_create(&t, NULL, &start_closer, C));

	expected_si_fd = pipefd[1];
	expected_si_code = POLL_ERR;
	expected_si_band = POLLERR;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	atf_tc_expect_fail("PR kern/59056: poll POLLHUP bugs");
	wait_for_io(&omask);
	check_write_fail(pipefd[1], EPIPE);

	RZ(pthread_join(t, NULL));
}

ATF_TC(ptyapp_read);
ATF_TC_HEAD(ptyapp_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when app side of pty is readable");
}
ATF_TC_BODY(ptyapp_read, tc)
{
	sigset_t omask;
	int ptyhost, ptyapp, flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	ptysetup(&ptyhost, &ptyapp);
	RL(fcntl(ptyapp, F_SETOWN, getpid()));
	RL(flags = fcntl(ptyapp, F_GETFL));
	RL(fcntl(ptyapp, F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = ptyhost;
	RZ(pthread_create(&t, NULL, &start_writer, C));

	expected_si_fd = ptyapp;
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	atf_tc_expect_fail("PR kern/60783: tty: missing siginfo_t in SIGIO");
	wait_for_io(&omask);
	check_read(ptyapp);

	RZ(pthread_join(t, NULL));
}

ATF_TC(ptyapp_read_closed);
ATF_TC_HEAD(ptyapp_read_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when app side of pty is readable because host closed");
}
ATF_TC_BODY(ptyapp_read_closed, tc)
{
	sigset_t omask;
	int ptyhost, ptyapp, flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	ptysetup(&ptyhost, &ptyapp);
	RL(fcntl(ptyapp, F_SETOWN, getpid()));
	RL(flags = fcntl(ptyapp, F_GETFL));
	RL(fcntl(ptyapp, F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = ptyhost;
	RZ(pthread_create(&t, NULL, &start_closer, C));

	expected_si_fd = ptyapp;
	expected_si_code = POLL_HUP;
	expected_si_band = POLLHUP;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	atf_tc_expect_fail("PR kern/60783: tty: missing siginfo_t in SIGIO");
	wait_for_io(&omask);
	check_read_eof(ptyapp);

	RZ(pthread_join(t, NULL));
}

ATF_TC(ptyapp_write);
ATF_TC_HEAD(ptyapp_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when app side of pty is writable");
}
ATF_TC_BODY(ptyapp_write, tc)
{
	sigset_t omask;
	int ptyhost, ptyapp, flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	ptysetup(&ptyhost, &ptyapp);
	fillpipebuf(ptyapp);
	RL(fcntl(ptyapp, F_SETOWN, getpid()));
	RL(flags = fcntl(ptyapp, F_GETFL));
	RL(fcntl(ptyapp, F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = ptyhost;
	RL(ioctl(ptyapp, TIOCGQSIZE, &C->lowat));
	RZ(pthread_create(&t, NULL, &start_reader, C));

	expected_si_fd = ptyapp;
	expected_si_code = POLL_OUT;
	expected_si_band = POLLOUT|POLLWRNORM;
	si_band_mask = ~0;

	atf_tc_expect_signal(SIGALRM, "PR kern/60784:"
	    " pty doesn't deliver SIGIO when writable");

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_write(ptyapp);

	RZ(pthread_join(t, NULL));
}

ATF_TC(ptyapp_write_closed);
ATF_TC_HEAD(ptyapp_write_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when app side of pty is writable because host closed");
}
ATF_TC_BODY(ptyapp_write_closed, tc)
{
	sigset_t omask;
	int ptyhost, ptyapp, flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	ptysetup(&ptyhost, &ptyapp);
	fillpipebuf(ptyapp);
	RL(fcntl(ptyapp, F_SETOWN, getpid()));
	RL(flags = fcntl(ptyapp, F_GETFL));
	RL(fcntl(ptyapp, F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = ptyhost;
	RZ(pthread_create(&t, NULL, &start_closer, C));

	expected_si_fd = ptyapp;
	expected_si_code = POLL_ERR;
	expected_si_band = POLLERR;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	atf_tc_expect_fail("PR kern/60783: tty: missing siginfo_t in SIGIO");
	wait_for_io(&omask);
	check_write_fail(ptyapp, EPIPE);

	RZ(pthread_join(t, NULL));
}

ATF_TC(ptyhost_read);
ATF_TC_HEAD(ptyhost_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when host side of pty is readable");
}
ATF_TC_BODY(ptyhost_read, tc)
{
	sigset_t omask;
	int ptyhost, ptyapp, flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	ptysetup(&ptyhost, &ptyapp);
	RL(fcntl(ptyhost, F_SETOWN, getpid()));
	RL(flags = fcntl(ptyhost, F_GETFL));
	RL(fcntl(ptyhost, F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = ptyapp;
	RZ(pthread_create(&t, NULL, &start_writer, C));

	expected_si_fd = ptyhost;
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	atf_tc_expect_signal(SIGALRM, "PR kern/60784:"
	    " pty doesn't deliver SIGIO when writable");

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read(ptyhost);

	RZ(pthread_join(t, NULL));
}

ATF_TC(ptyhost_read_closed);
ATF_TC_HEAD(ptyhost_read_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when host side of pty is readable because app closed");
}
ATF_TC_BODY(ptyhost_read_closed, tc)
{
	sigset_t omask;
	int ptyhost, ptyapp, flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	ptysetup(&ptyhost, &ptyapp);
	RL(fcntl(ptyhost, F_SETOWN, getpid()));
	RL(flags = fcntl(ptyhost, F_GETFL));
	RL(fcntl(ptyhost, F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = ptyapp;
	RZ(pthread_create(&t, NULL, &start_closer, C));

	expected_si_fd = ptyhost;
	expected_si_code = POLL_HUP;
	expected_si_band = POLLHUP;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	atf_tc_expect_fail("PR kern/60783: tty: missing siginfo_t in SIGIO");
	wait_for_io(&omask);
	check_read_eof(ptyhost);

	RZ(pthread_join(t, NULL));
}

ATF_TC(ptyhost_write);
ATF_TC_HEAD(ptyhost_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when host side of pty is writable");
}
ATF_TC_BODY(ptyhost_write, tc)
{
	sigset_t omask;
	int ptyhost, ptyapp, flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	ptysetup(&ptyhost, &ptyapp);
	fillpipebuf(ptyhost);
	RL(fcntl(ptyhost, F_SETOWN, getpid()));
	RL(flags = fcntl(ptyhost, F_GETFL));
	RL(fcntl(ptyhost, F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = ptyapp;
	RL(ioctl(ptyhost, TIOCGQSIZE, &C->lowat));
	RZ(pthread_create(&t, NULL, &start_reader, C));

	expected_si_fd = ptyhost;
	expected_si_code = POLL_OUT;
	expected_si_band = POLLOUT|POLLWRNORM;
	si_band_mask = ~0;

	atf_tc_expect_signal(SIGALRM, "PR kern/60784:"
	    " pty doesn't deliver SIGIO when writable");

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_write(ptyhost);

	RZ(pthread_join(t, NULL));
}

ATF_TC(ptyhost_write_closed);
ATF_TC_HEAD(ptyhost_write_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when host side of pty is writable because app closed");
}
ATF_TC_BODY(ptyhost_write_closed, tc)
{
	sigset_t omask;
	int ptyhost, ptyapp, flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	ptysetup(&ptyhost, &ptyapp);
	fillpipebuf(ptyhost);
	RL(fcntl(ptyhost, F_SETOWN, getpid()));
	RL(flags = fcntl(ptyhost, F_GETFL));
	RL(fcntl(ptyhost, F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = ptyapp;
	RZ(pthread_create(&t, NULL, &start_closer, C));

	expected_si_fd = ptyhost;
	expected_si_code = POLL_ERR;
	expected_si_band = POLLERR;
	si_band_mask = ~0;

	atf_tc_expect_signal(SIGALRM, "PR kern/60784:"
	    " pty doesn't deliver SIGIO when writable");

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_write_fail(ptyhost, EPIPE);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_inet6_read);
ATF_TC_HEAD(socket_inet6_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is readable");
}
ATF_TC_BODY(socket_inet6_read, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	socksetup(sockfd, AF_INET6, "::1");
	RL(fcntl(sockfd[0], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[0], F_GETFL));
	RL(fcntl(sockfd[0], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[1];
	RZ(pthread_create(&t, NULL, &start_writer, C));

	expected_si_fd = sockfd[0];
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read(sockfd[0]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_inet6_read_closed);
ATF_TC_HEAD(socket_inet6_read_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is readable because closed");
}
ATF_TC_BODY(socket_inet6_read_closed, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	socksetup(sockfd, AF_INET6, "::1");
	RL(fcntl(sockfd[0], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[0], F_GETFL));
	RL(fcntl(sockfd[0], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[1];
	RZ(pthread_create(&t, NULL, &start_closer, C));

	/*
	 * XXX Is this really sensible?  No POLLHUP?  Different from
	 * pipes?  POLLIN is right in one sense that we can read
	 * without blocking, and it will report EOF -- but that is also
	 * the scenario in which POLLHUP is right.
	 */
	expected_si_fd = sockfd[0];
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read_eof(sockfd[0]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_inet6_read_shutdown);
ATF_TC_HEAD(socket_inet6_read_shutdown, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is readable because shutdown");
}
ATF_TC_BODY(socket_inet6_read_shutdown, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	socksetup(sockfd, AF_INET6, "::1");
	RL(fcntl(sockfd[0], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[0], F_GETFL));
	RL(fcntl(sockfd[0], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[1];
	RZ(pthread_create(&t, NULL, &start_shutdownwr, C));

	/*
	 * XXX Is this really sensible?  No POLLHUP?  Different from
	 * pipes?  POLLIN is right in one sense that we can read
	 * without blocking, and it will report EOF -- but that is also
	 * the scenario in which POLLHUP is right.
	 */
	expected_si_fd = sockfd[0];
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read_eof(sockfd[0]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_inet6_write);
ATF_TC_HEAD(socket_inet6_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is writable");
}
ATF_TC_BODY(socket_inet6_write, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	socklen_t optlen;
	pthread_t t;

	setup(&omask);

	socksetup(sockfd, AF_INET6, "::1");
	fillpipebuf(sockfd[1]);
	RL(fcntl(sockfd[1], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[1], F_GETFL));
	RL(fcntl(sockfd[1], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[0];
	optlen = sizeof(C->lowat);
	RL(getsockopt(sockfd[1], SOL_SOCKET, SO_SNDLOWAT, &C->lowat, &optlen));
	ATF_REQUIRE_EQ_MSG(optlen, sizeof(C->lowat),
	    "optlen=%zu sizeof(C->lowat)=%zu",
	    (size_t)optlen, sizeof(C->lowat));

	RZ(pthread_create(&t, NULL, &start_reader, C));

	expected_si_fd = sockfd[1];
	expected_si_code = POLL_OUT;
	expected_si_band = POLLOUT|POLLWRNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_sendnonblock(sockfd[1]);
	check_write(sockfd[1]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_inet6_write_closed);
ATF_TC_HEAD(socket_inet6_write_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is writable because closed");
}
ATF_TC_BODY(socket_inet6_write_closed, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	socksetup(sockfd, AF_INET6, "::1");
	fillpipebuf(sockfd[1]);
	RL(fcntl(sockfd[1], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[1], F_GETFL));
	RL(fcntl(sockfd[1], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[0];
	RZ(pthread_create(&t, NULL, &start_closer, C));

	/*
	 * We get POLLIN|POLLRDNORM because the TCP peer has sent FIN
	 * to indicate they won't write any more, which will manifest
	 * on our end by read/recv reporting EOF; TCP has no mechanism
	 * to indicate that they won't _read_ any more as in POLLERR.
	 *
	 * XXX However, the peer may ack some data, leading us to think
	 * we can write more.  So, to avoid a flaky test, let's shut
	 * down in the write direction.  Except, oops, that doesn't
	 * work because of PR kern/60786: poll reports socket writable
	 * after shutdown(SHUT_WR).  And while we could si_band for
	 * POLLIN, we don't get all bits at once like poll() does; we
	 * get _either_ code=POLL_IN band=POLLIN|POLLRDNORM _or_
	 * code=POLL_OUT band=POLLOUT|POLLWRNORM.
	 *
	 * I give up; I'll just not test the code and band for now...
	 */
	RL(shutdown(sockfd[1], SHUT_WR));
	expected_si_fd = sockfd[1];
	expected_si_code = -1;	/* POLL_IN */
	expected_si_band = 0;	/* POLLIN|POLLRDNORM */
	si_band_mask = 0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_write_fail(sockfd[1], EPIPE);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_inet6_write_shutdown);
ATF_TC_HEAD(socket_inet6_write_shutdown, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is writable because shutdown");
}
ATF_TC_BODY(socket_inet6_write_shutdown, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	socksetup(sockfd, AF_INET6, "::1");
	fillpipebuf(sockfd[1]);
	RL(fcntl(sockfd[1], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[1], F_GETFL));
	RL(fcntl(sockfd[1], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[0];
	RZ(pthread_create(&t, NULL, &start_shutdownrd, C));

	/*
	 * TCP doesn't have a way to say `I will discard any further
	 * bytes you throw at me, so please stop throwing them'.  But
	 * it can ack some data making us think we can continue sending
	 * more.
	 */
	expected_si_fd = sockfd[1];
	expected_si_code = POLL_OUT;
	expected_si_band = POLLOUT|POLLWRNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_write(sockfd[1]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_inet_read);
ATF_TC_HEAD(socket_inet_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is readable");
}
ATF_TC_BODY(socket_inet_read, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	socksetup(sockfd, AF_INET, "127.0.0.1");
	RL(fcntl(sockfd[0], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[0], F_GETFL));
	RL(fcntl(sockfd[0], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[1];
	RZ(pthread_create(&t, NULL, &start_writer, C));

	expected_si_fd = sockfd[0];
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read(sockfd[0]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_inet_read_closed);
ATF_TC_HEAD(socket_inet_read_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is readable because closed");
}
ATF_TC_BODY(socket_inet_read_closed, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	socksetup(sockfd, AF_INET, "127.0.0.1");
	RL(fcntl(sockfd[0], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[0], F_GETFL));
	RL(fcntl(sockfd[0], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[1];
	RZ(pthread_create(&t, NULL, &start_closer, C));

	/*
	 * XXX Is this really sensible?  No POLLHUP?  Different from
	 * pipes?  POLLIN is right in one sense that we can read
	 * without blocking, and it will report EOF -- but that is also
	 * the scenario in which POLLHUP is right.
	 */
	expected_si_fd = sockfd[0];
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read_eof(sockfd[0]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_inet_read_shutdown);
ATF_TC_HEAD(socket_inet_read_shutdown, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is readable because shutdown");
}
ATF_TC_BODY(socket_inet_read_shutdown, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	socksetup(sockfd, AF_INET, "127.0.0.1");
	RL(fcntl(sockfd[0], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[0], F_GETFL));
	RL(fcntl(sockfd[0], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[1];
	RZ(pthread_create(&t, NULL, &start_shutdownwr, C));

	/*
	 * XXX Is this really sensible?  No POLLHUP?  Different from
	 * pipes?  POLLIN is right in one sense that we can read
	 * without blocking, and it will report EOF -- but that is also
	 * the scenario in which POLLHUP is right.
	 */
	expected_si_fd = sockfd[0];
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read_eof(sockfd[0]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_inet_write);
ATF_TC_HEAD(socket_inet_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is writable");
}
ATF_TC_BODY(socket_inet_write, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	socklen_t optlen;
	pthread_t t;

	setup(&omask);

	socksetup(sockfd, AF_INET, "127.0.0.1");
	fillpipebuf(sockfd[1]);
	RL(fcntl(sockfd[1], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[1], F_GETFL));
	RL(fcntl(sockfd[1], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[0];
	optlen = sizeof(C->lowat);
	RL(getsockopt(sockfd[1], SOL_SOCKET, SO_SNDLOWAT, &C->lowat, &optlen));
	ATF_REQUIRE_EQ_MSG(optlen, sizeof(C->lowat),
	    "optlen=%zu sizeof(C->lowat)=%zu",
	    (size_t)optlen, sizeof(C->lowat));

	RZ(pthread_create(&t, NULL, &start_reader, C));

	expected_si_fd = sockfd[1];
	expected_si_code = POLL_OUT;
	expected_si_band = POLLOUT|POLLWRNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_sendnonblock(sockfd[1]);
	check_write(sockfd[1]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_inet_write_closed);
ATF_TC_HEAD(socket_inet_write_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is writable because closed");
}
ATF_TC_BODY(socket_inet_write_closed, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	socksetup(sockfd, AF_INET, "127.0.0.1");
	fillpipebuf(sockfd[1]);
	RL(fcntl(sockfd[1], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[1], F_GETFL));
	RL(fcntl(sockfd[1], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[0];
	RZ(pthread_create(&t, NULL, &start_closer, C));

	/*
	 * We get POLLIN|POLLRDNORM because the TCP peer has sent FIN
	 * to indicate they won't write any more, which will manifest
	 * on our end by read/recv reporting EOF; TCP has no mechanism
	 * to indicate that they won't _read_ any more as in POLLERR.
	 *
	 * XXX However, the peer may ack some data, leading us to think
	 * we can write more.  So, to avoid a flaky test, let's shut
	 * down in the write direction.  Except, oops, that doesn't
	 * work because of PR kern/60786: poll reports socket writable
	 * after shutdown(SHUT_WR).  And while we could si_band for
	 * POLLIN, we don't get all bits at once like poll() does; we
	 * get _either_ code=POLL_IN band=POLLIN|POLLRDNORM _or_
	 * code=POLL_OUT band=POLLOUT|POLLWRNORM.
	 *
	 * I give up; I'll just not test the code and band for now...
	 */
	RL(shutdown(sockfd[1], SHUT_WR));
	expected_si_fd = sockfd[1];
	expected_si_code = -1;	/* POLL_IN */
	expected_si_band = 0;	/* POLLIN|POLLRDNORM */
	si_band_mask = 0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_write_fail(sockfd[1], EPIPE);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_inet_write_shutdown);
ATF_TC_HEAD(socket_inet_write_shutdown, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is writable because shutdown");
}
ATF_TC_BODY(socket_inet_write_shutdown, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	socksetup(sockfd, AF_INET, "127.0.0.1");
	fillpipebuf(sockfd[1]);
	RL(fcntl(sockfd[1], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[1], F_GETFL));
	RL(fcntl(sockfd[1], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[0];
	RZ(pthread_create(&t, NULL, &start_shutdownrd, C));

	/*
	 * TCP doesn't have a way to say `I will discard any further
	 * bytes you throw at me, so please stop throwing them', so it
	 * can't report POLLERR.  But it can ack some data making us
	 * think we can continue sending more, so it does report
	 * POLLIN.
	 */
	expected_si_fd = sockfd[1];
	expected_si_code = POLL_OUT;
	expected_si_band = POLLOUT|POLLWRNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_write(sockfd[1]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_local_read);
ATF_TC_HEAD(socket_local_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is readable");
}
ATF_TC_BODY(socket_local_read, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd));
	RL(fcntl(sockfd[0], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[0], F_GETFL));
	RL(fcntl(sockfd[0], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[1];
	RZ(pthread_create(&t, NULL, &start_writer, C));

	expected_si_fd = sockfd[0];
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read(sockfd[0]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_local_read_closed);
ATF_TC_HEAD(socket_local_read_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is readable because closed");
}
ATF_TC_BODY(socket_local_read_closed, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd));
	RL(fcntl(sockfd[0], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[0], F_GETFL));
	RL(fcntl(sockfd[0], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[1];
	RZ(pthread_create(&t, NULL, &start_closer, C));

	/*
	 * XXX Is this really sensible?  No POLLHUP?  Different from
	 * pipes?  POLLIN is right in one sense that we can read
	 * without blocking, and it will report EOF -- but that is also
	 * the scenario in which POLLHUP is right.
	 */
	expected_si_fd = sockfd[0];
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read_eof(sockfd[0]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_local_read_shutdown);
ATF_TC_HEAD(socket_local_read_shutdown, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is readable because shutdown");
}
ATF_TC_BODY(socket_local_read_shutdown, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd));
	RL(fcntl(sockfd[0], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[0], F_GETFL));
	RL(fcntl(sockfd[0], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[1];
	RZ(pthread_create(&t, NULL, &start_shutdownwr, C));

	/*
	 * XXX Is this really sensible?  No POLLHUP?  Different from
	 * pipes?  POLLIN is right in one sense that we can read
	 * without blocking, and it will report EOF -- but that is also
	 * the scenario in which POLLHUP is right.
	 */
	expected_si_fd = sockfd[0];
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_read_eof(sockfd[0]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_local_write);
ATF_TC_HEAD(socket_local_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is writable");
}
ATF_TC_BODY(socket_local_write, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	socklen_t optlen;
	pthread_t t;

	setup(&omask);

	RL(socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd));
	fillpipebuf(sockfd[1]);
	RL(fcntl(sockfd[1], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[1], F_GETFL));
	RL(fcntl(sockfd[1], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[0];
	optlen = sizeof(C->lowat);
	RL(getsockopt(sockfd[1], SOL_SOCKET, SO_SNDLOWAT, &C->lowat, &optlen));
	ATF_REQUIRE_EQ_MSG(optlen, sizeof(C->lowat),
	    "optlen=%zu sizeof(C->lowat)=%zu",
	    (size_t)optlen, sizeof(C->lowat));

	atf_tc_expect_fail("PR kern/60782:"
	    " SIGIO says socket ready for send before send does");

	RZ(pthread_create(&t, NULL, &start_reader, C));

	expected_si_fd = sockfd[0];
	expected_si_code = POLL_OUT;
	expected_si_band = POLLOUT|POLLWRNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_sendnonblock(sockfd[1]);
	check_write(sockfd[1]);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_local_write_closed);
ATF_TC_HEAD(socket_local_write_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is writable because closed");
}
ATF_TC_BODY(socket_local_write_closed, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd));
	fillpipebuf(sockfd[1]);
	RL(fcntl(sockfd[1], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[1], F_GETFL));
	RL(fcntl(sockfd[1], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[0];
	RZ(pthread_create(&t, NULL, &start_closer, C));

	/*
	 * Note: This matches the TCP semantics but not the pipe
	 * semantics.
	 */
	expected_si_fd = sockfd[1];
	expected_si_code = POLL_IN;
	expected_si_band = POLLIN|POLLRDNORM;
	si_band_mask = ~0;

	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);
	wait_for_io(&omask);
	check_write_fail(sockfd[1], EPIPE);

	RZ(pthread_join(t, NULL));
}

ATF_TC(socket_local_write_shutdown);
ATF_TC_HEAD(socket_local_write_shutdown, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test SIGIO when socket is writable because shutdown");
}
ATF_TC_BODY(socket_local_write_shutdown, tc)
{
	sigset_t omask;
	int sockfd[2], flags;
	pthread_barrier_t bar;
	struct context ctx, *C = &ctx;
	pthread_t t;

	setup(&omask);

	RL(socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd));
	fillpipebuf(sockfd[1]);
	RL(fcntl(sockfd[1], F_SETOWN, getpid()));
	RL(flags = fcntl(sockfd[1], F_GETFL));
	RL(fcntl(sockfd[1], F_SETFL, flags | O_ASYNC));

	RZ(pthread_barrier_init(&bar, NULL, 2));

	memset(C, 0, sizeof(*C));
	C->bar = &bar;
	C->fd = sockfd[0];
	RZ(pthread_create(&t, NULL, &start_shutdownrd, C));

	/*
	 * XXX Should AF_LOCAL sockets report anything to the peer on
	 * shutdown(SHUT_RD)?
	 */
	REQUIRE_LIBC(alarm(1), (unsigned)-1);
	(void)pthread_barrier_wait(&bar);

	/*
	 * XXX Why does this return EAGAIN and not EPIPE?  Shouldn't
	 * this refuse to send any more data now that the reader has
	 * shutdown that direction?
	 */
	check_write_fail(sockfd[1], EAGAIN);

	RZ(pthread_join(t, NULL));
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, fifo_read);
	ATF_TP_ADD_TC(tp, fifo_read_closed);
	ATF_TP_ADD_TC(tp, fifo_write);
	ATF_TP_ADD_TC(tp, fifo_write_closed);
	ATF_TP_ADD_TC(tp, pipe_read);
	ATF_TP_ADD_TC(tp, pipe_read_closed);
	ATF_TP_ADD_TC(tp, pipe_write);
	ATF_TP_ADD_TC(tp, pipe_write_closed);
	ATF_TP_ADD_TC(tp, ptyapp_read);
	ATF_TP_ADD_TC(tp, ptyapp_read_closed);
	ATF_TP_ADD_TC(tp, ptyapp_write);
	ATF_TP_ADD_TC(tp, ptyapp_write_closed);
	ATF_TP_ADD_TC(tp, ptyhost_read);
	ATF_TP_ADD_TC(tp, ptyhost_read_closed);
	ATF_TP_ADD_TC(tp, ptyhost_write);
	ATF_TP_ADD_TC(tp, ptyhost_write_closed);
	ATF_TP_ADD_TC(tp, socket_inet6_read);
	ATF_TP_ADD_TC(tp, socket_inet6_read_closed);
	ATF_TP_ADD_TC(tp, socket_inet6_read_shutdown);
	ATF_TP_ADD_TC(tp, socket_inet6_write);
	ATF_TP_ADD_TC(tp, socket_inet6_write_closed);
	ATF_TP_ADD_TC(tp, socket_inet6_write_shutdown);
	ATF_TP_ADD_TC(tp, socket_inet_read);
	ATF_TP_ADD_TC(tp, socket_inet_read_closed);
	ATF_TP_ADD_TC(tp, socket_inet_read_shutdown);
	ATF_TP_ADD_TC(tp, socket_inet_write);
	ATF_TP_ADD_TC(tp, socket_inet_write_closed);
	ATF_TP_ADD_TC(tp, socket_inet_write_shutdown);
	ATF_TP_ADD_TC(tp, socket_local_read);
	ATF_TP_ADD_TC(tp, socket_local_read_closed);
	ATF_TP_ADD_TC(tp, socket_local_read_shutdown);
	ATF_TP_ADD_TC(tp, socket_local_write);
	ATF_TP_ADD_TC(tp, socket_local_write_closed);
	ATF_TP_ADD_TC(tp, socket_local_write_shutdown);

	return atf_no_error();
}
