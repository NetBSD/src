/*	$NetBSD: t_fdrestart.c,v 1.4 2023/11/18 19:46:55 riastradh Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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

#define	_KMEMUSER		/* ERESTART */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_fdrestart.c,v 1.4 2023/11/18 19:46:55 riastradh Exp $");

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <atf-c.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "h_macros.h"

struct fdrestart {
	void			(*op)(struct fdrestart *);
	int			fd;
	pthread_barrier_t	barrier;
};

static void
waitforbarrier(struct fdrestart *F, const char *caller)
{
	int error;

	error = pthread_barrier_wait(&F->barrier);
	switch (error) {
	case 0:
	case PTHREAD_BARRIER_SERIAL_THREAD:
		break;
	default:
		atf_tc_fail("%s: pthread_barrier_wait: %d, %s", caller, error,
		    strerror(error));
	}
}

static void
doread(struct fdrestart *F)
{
	char c;
	ssize_t nread;
	int error;

	/*
	 * Wait for the other thread to be ready.
	 */
	waitforbarrier(F, "reader");

	/*
	 * Start a read.  This should block, and then, when the other
	 * thread closes the fd, should be woken to fail with ERESTART.
	 */
	nread = rump_sys_read(F->fd, &c, sizeof(c));
	ATF_REQUIRE_EQ_MSG(nread, -1, "nread=%zd", nread);
	error = errno;
	ATF_REQUIRE_EQ_MSG(error, ERESTART, "errno=%d (%s)", error,
	    strerror(error));

	/*
	 * Now further attempts at I/O should fail with EBADF because
	 * the fd has been closed.
	 */
	nread = rump_sys_read(F->fd, &c, sizeof(c));
	ATF_REQUIRE_EQ_MSG(nread, -1, "nread=%zd", nread);
	error = errno;
	ATF_REQUIRE_EQ_MSG(error, EBADF, "errno=%d (%s)", error,
	    strerror(error));
}

static void
dowrite(struct fdrestart *F)
{
	static const char buf[1024*1024]; /* XXX >BIG_PIPE_SIZE */
	ssize_t nwrit;
	int error;

	/*
	 * Make sure the pipe's buffer is full first.
	 */
	for (;;) {
		int nspace;

		RL(rump_sys_ioctl(F->fd, FIONSPACE, &nspace));
		ATF_REQUIRE_MSG(nspace >= 0, "nspace=%d", nspace);
		if (nspace == 0)
			break;
		RL(rump_sys_write(F->fd, buf, (size_t)nspace));
	}

	/*
	 * Wait for the other thread to be ready.
	 */
	waitforbarrier(F, "writer");

	/*
	 * Start a write.  This should block, and then, when the other
	 * thread closes the fd, should be woken to fail with ERESTART.
	 */
	nwrit = rump_sys_write(F->fd, buf, sizeof(buf));
	ATF_REQUIRE_EQ_MSG(nwrit, -1, "nwrit=%zd", nwrit);
	error = errno;
	ATF_REQUIRE_EQ_MSG(error, ERESTART, "errno=%d (%s)", error,
	    strerror(error));

	/*
	 * Now further attempts at I/O should fail with EBADF because
	 * the fd has been closed.
	 */
	nwrit = rump_sys_write(F->fd, buf, sizeof(buf));
	ATF_REQUIRE_EQ_MSG(nwrit, -1, "nwrit=%zd", nwrit);
	error = errno;
	ATF_REQUIRE_EQ_MSG(error, EBADF, "errno=%d (%s)", error,
	    strerror(error));
}

static void *
doit(void *cookie)
{
	struct fdrestart *F = cookie;

	(*F->op)(F);

	return NULL;
}

static void
on_sigalrm(int signo)
{

	atf_tc_fail("timed out");
}

static void
testfdrestart(struct fdrestart *F)
{
	pthread_t t;

	ATF_REQUIRE_MSG(signal(SIGALRM, &on_sigalrm) != SIG_ERR,
	    "errno=%d (%s)", errno, strerror(errno));

	RZ(pthread_barrier_init(&F->barrier, NULL, 2));
	RZ(pthread_create(&t, NULL, &doit, F));
	waitforbarrier(F, "closer");	/* wait for thread to start */
	(void)sleep(1);			/* wait for op to start */
	(void)alarm(1);			/* set a deadline */
	RL(rump_sys_close(F->fd));	/* wake op in other thread */
	RZ(pthread_join(t, NULL));	/* wait for op to wake and fail */
	(void)alarm(0);			/* clear the deadline */
}

ATF_TC(pipe_read);
ATF_TC_HEAD(pipe_read, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pipe read fails on close");
}
ATF_TC_BODY(pipe_read, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int fd[2];

	rump_init();

	RL(rump_sys_pipe(fd));

	memset(F, 0, sizeof(*F));
	F->op = &doread;
	F->fd = fd[0];
	testfdrestart(F);
}

ATF_TC(pipe_write);
ATF_TC_HEAD(pipe_write, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pipe write fails on close");
}
ATF_TC_BODY(pipe_write, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int fd[2];

	rump_init();

	RL(rump_sys_pipe(fd));

	memset(F, 0, sizeof(*F));
	F->op = &dowrite;
	F->fd = fd[1];
	atf_tc_expect_fail("PR kern/57659");
	testfdrestart(F);
}

ATF_TC(socketpair_read);
ATF_TC_HEAD(socketpair_read, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test socketpair read fails on close");
}
ATF_TC_BODY(socketpair_read, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int fd[2];

	rump_init();

	RL(rump_sys_socketpair(AF_LOCAL, SOCK_STREAM, 0, fd));

	memset(F, 0, sizeof(*F));
	F->op = &doread;
	F->fd = fd[0];
	testfdrestart(F);
}

ATF_TC(socketpair_write);
ATF_TC_HEAD(socketpair_write, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test socketpair write fails on close");
}
ATF_TC_BODY(socketpair_write, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int fd[2];

	rump_init();

	RL(rump_sys_socketpair(AF_LOCAL, SOCK_STREAM, 0, fd));

	memset(F, 0, sizeof(*F));
	F->op = &dowrite;
	F->fd = fd[0];
	testfdrestart(F);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, pipe_read);
	ATF_TP_ADD_TC(tp, pipe_write);
	ATF_TP_ADD_TC(tp, socketpair_read);
	ATF_TP_ADD_TC(tp, socketpair_write);

	return atf_no_error();
}
