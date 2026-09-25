/*	$NetBSD: t_fdrestart.c,v 1.8 2026/09/25 01:10:26 riastradh Exp $	*/

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
__RCSID("$NetBSD: t_fdrestart.c,v 1.8 2026/09/25 01:10:26 riastradh Exp $");

#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fs/ptyfs/ptyfs.h>
#include <fs/tmpfs/tmpfs_args.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "h_macros.h"

struct fdrestart {
	void			(*op)(struct fdrestart *);
	int			fd;
	pthread_barrier_t	barrier;
	struct sockaddr		*sa;
	socklen_t		salen;
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
	ATF_CHECK_EQ_MSG(error, ERESTART, "errno=%d (%s)", error,
	    strerror(error));

	/*
	 * Now further attempts at I/O should fail with EBADF because
	 * the fd has been closed.
	 */
	nread = rump_sys_read(F->fd, &c, sizeof(c));
	ATF_REQUIRE_EQ_MSG(nread, -1, "nread=%zd", nread);
	error = errno;
	ATF_CHECK_EQ_MSG(error, EBADF, "errno=%d (%s)", error,
	    strerror(error));
}

static void
dopollread(struct fdrestart *F)
{
	struct pollfd pfd;
	int nfds;

	/*
	 * Prepare poll inputs so we can start ASAP when we cross the
	 * barrier.
	 */
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = F->fd;
	pfd.events = POLLIN;

	/*
	 * Wait for the other thread to be ready.
	 */
	waitforbarrier(F, "reader");

	/*
	 * Wait for readability.  This should block, and then, when the
	 * other thread closes the fd, should be woken to fail with
	 * ERESTART.
	 */
	nfds = rump_sys_poll(&pfd, 1, -1);
	ATF_REQUIRE_EQ_MSG(nfds, 1, "nfds=%d", nfds);
	ATF_CHECK_EQ_MSG(pfd.revents, POLLNVAL, "revents=0x%x", pfd.revents);

	/*
	 * Further attempts to poll should return POLLNVAL immediately
	 * because the fd has been closed.
	 */
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = F->fd;
	pfd.events = POLLOUT;
	nfds = rump_sys_poll(&pfd, 1, -1);
	ATF_REQUIRE_EQ_MSG(nfds, 1, "nfds=%d", nfds);
	ATF_CHECK_EQ_MSG(pfd.revents, POLLNVAL, "revents=0x%x", pfd.revents);
}

static void
doselectread(struct fdrestart *F)
{
	fd_set rfd;
	int nfds;

	/*
	 * Prepare select inputs so we can start ASAP when we cross the
	 * barrier.
	 */
	FD_ZERO(&rfd);
	FD_SET(F->fd, &rfd);

	/*
	 * Wait for the other thread to be ready.
	 */
	waitforbarrier(F, "reader");

	/*
	 * Wait for readability.  This should block, and then, when the
	 * other thread closes the fd, should be woken to fail with
	 * ERESTART.
	 */
	nfds = rump_sys_select(F->fd + 1, &rfd, /*wfd*/NULL, /*efd*/NULL,
	    /*timeout*/NULL);
	ATF_CHECK_ERRNO(EBADF, nfds == -1);

	/*
	 * Further attempts to select should fail with EBADF
	 * immediately because the fd has been closed.
	 */
	FD_ZERO(&rfd);
	FD_SET(F->fd, &rfd);
	nfds = rump_sys_select(F->fd + 1, &rfd, /*wfd*/NULL, /*efd*/NULL,
	    /*timeout*/NULL);
	ATF_CHECK_ERRNO(EBADF, nfds == -1);
}

static void
fillpipebuf(int fd)
{
	static const char buf[1024*1024]; /* XXX >BIG_PIPE_SIZE */

	for (;;) {
		int nspace;

		RL(rump_sys_ioctl(fd, FIONSPACE, &nspace));
		ATF_REQUIRE_MSG(nspace >= 0, "nspace=%d", nspace);
		if (nspace == 0)
			break;
		RL(rump_sys_write(fd, buf, (size_t)nspace));
	}
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
	fillpipebuf(F->fd);

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
	ATF_CHECK_EQ_MSG(error, ERESTART, "errno=%d (%s)", error,
	    strerror(error));

	/*
	 * Now further attempts at I/O should fail with EBADF because
	 * the fd has been closed.
	 */
	nwrit = rump_sys_write(F->fd, buf, sizeof(buf));
	ATF_REQUIRE_EQ_MSG(nwrit, -1, "nwrit=%zd", nwrit);
	error = errno;
	ATF_CHECK_EQ_MSG(error, EBADF, "errno=%d (%s)", error,
	    strerror(error));
}

static void
dopollwrite(struct fdrestart *F)
{
	struct pollfd pfd;
	int nfds;

	/*
	 * Make sure the pipe's buffer is full first.
	 */
	fillpipebuf(F->fd);

	/*
	 * Prepare poll inputs so we can start ASAP when we cross the
	 * barrier.
	 */
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = F->fd;
	pfd.events = POLLOUT;

	/*
	 * Wait for the other thread to be ready.
	 */
	waitforbarrier(F, "writer");

	/*
	 * Wait for writability.  This should block, and then, when the
	 * other thread closes the fd, should be woken to fail with
	 * ERESTART.
	 */
	nfds = rump_sys_poll(&pfd, 1, -1);
	ATF_REQUIRE_EQ_MSG(nfds, 1, "nfds=%d", nfds);
	ATF_CHECK_EQ_MSG(pfd.revents, POLLNVAL, "revents=0x%x", pfd.revents);

	/*
	 * Further attempts to poll should return POLLNVAL immediately
	 * because the fd has been closed.
	 */
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = F->fd;
	pfd.events = POLLOUT;
	nfds = rump_sys_poll(&pfd, 1, -1);
	ATF_REQUIRE_EQ_MSG(nfds, 1, "nfds=%d", nfds);
	ATF_CHECK_EQ_MSG(pfd.revents, POLLNVAL, "revents=0x%x", pfd.revents);
}

static void
doselectwrite(struct fdrestart *F)
{
	fd_set wfd;
	int nfds;

	/*
	 * Make sure the pipe's buffer is full first.
	 */
	fillpipebuf(F->fd);

	/*
	 * Prepare select inputs so we can start ASAP when we cross the
	 * barrier.
	 */
	FD_ZERO(&wfd);
	FD_SET(F->fd, &wfd);

	/*
	 * Wait for the other thread to be ready.
	 */
	waitforbarrier(F, "writer");

	/*
	 * Wait for writability.  This should block, and then, when the
	 * other thread closes the fd, should be woken to fail with
	 * ERESTART.
	 */
	nfds = rump_sys_select(F->fd + 1, /*rfd*/NULL, &wfd, /*efd*/NULL,
	    /*timeout*/NULL);
	ATF_CHECK_ERRNO(EBADF, nfds == -1);

	/*
	 * Further attempts to select should fail with EBADF
	 * immediately because the fd has been closed.
	 */
	FD_ZERO(&wfd);
	FD_SET(F->fd, &wfd);
	nfds = rump_sys_select(F->fd + 1, /*rfd*/NULL, &wfd, /*efd*/NULL,
	    /*timeout*/NULL);
	ATF_CHECK_ERRNO(EBADF, nfds == -1);
}

static void
doconnect(struct fdrestart *F)
{

	/*
	 * Wait for the other thread to be ready.
	 */
	waitforbarrier(F, "connector");

	/*
	 * Wait for a socket connection to be accepted.
	 */
	ATF_CHECK_ERRNO(ERESTART,
	    rump_sys_connect(F->fd, F->sa, F->salen) == -1);
}

static void
doaccept(struct fdrestart *F)
{

	/*
	 * Wait for the other thread to be ready.
	 */
	waitforbarrier(F, "acceptor");

	/*
	 * Wait to accept a socket connection.
	 */
	ATF_CHECK_ERRNO(ERESTART, rump_sys_accept(F->fd, NULL, NULL) == -1);
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

	REQUIRE_LIBC(signal(SIGALRM, &on_sigalrm), SIG_ERR);

	RZ(pthread_barrier_init(&F->barrier, NULL, 2));
	RZ(pthread_create(&t, NULL, &doit, F));
	waitforbarrier(F, "closer");	/* wait for thread to start */
	(void)sleep(1);			/* wait for op to start */
	(void)alarm(1);			/* set a deadline */
	RL(rump_sys_close(F->fd));	/* wake op in other thread */
	RZ(pthread_join(t, NULL));	/* wait for op to wake and fail */
	(void)alarm(0);			/* clear the deadline */
}

static int
fifo_setup(int flags)
{
	struct tmpfs_args args;
	int rfd, wfd, fd;

	/*
	 * Mount a tmpfs so we can use fifos.  The rumpfs shim doesn't
	 * support them, or at least doesn't support setting and
	 * clearing O_NONBLOCK with fcntl on them.
	 */
	memset(&args, 0, sizeof(args));
	args.ta_version = TMPFS_ARGS_VERSION;
	args.ta_root_mode = 0777;
	RL(rump_sys_mkdir("/mnt", 0777));
	RL(rump_sys_mount(MOUNT_TMPFS, "/mnt", 0, &args, sizeof(args)));

	/*
	 * Create a fifo.
	 */
	RL(rump_sys_mkfifo("/mnt/fifo", 0600));

	/*
	 * Open the reader side first.  This is necessary because it is
	 * allowed to succeed without blocking when there is no peer,
	 * whereas opening the writer side either blocks or fails with
	 * ENXIO when there is no peer.
	 */
	RL(rfd = rump_sys_open("/mnt/fifo", O_RDONLY|O_NONBLOCK));

	/*
	 * If the caller asked for the read side, return it.
	 * Otherwise, open the write side (but leave the reader side
	 * open so that write will block rather than fail with
	 * EPIPE/SIGPIPE).
	 */
	switch (flags) {
	case O_RDONLY:
		fd = rfd;
		goto out;
	case O_WRONLY:
		RL(wfd = rump_sys_open("/mnt/fifo", O_WRONLY|O_NONBLOCK));
		fd = wfd;
		goto out;
	default:
		atf_tc_fail("invalid fifo setup flags");
	}

out:	/*
	 * Whichever side the caller wanted, make it blocking.
	 */
	RL(flags = rump_sys_fcntl(fd, F_GETFL));
	RL(rump_sys_fcntl(fd, F_SETFL, flags & ~O_NONBLOCK));
	return fd;
}

static void
ptysetup(int *hostfd, int *appfd)
{
	struct ptyfs_args args;
	struct ptmget pm;
	struct termios t;
	char *pts;

	(void)rump_sys_mkdir("/dev", 0777);
	(void)rump_sys_mkdir("/dev/pts", 0777);

	memset(&args, 0, sizeof(args));
	args.version = PTYFS_ARGSVERSION;
	args.mode = 0777;
	RL(rump_sys_mount(MOUNT_PTYFS, "/dev/pts", 0, &args, sizeof(args)));

	RL(*hostfd = rump_sys_open("/dev/ptmx", O_RDWR|O_NOCTTY));
	RL(rump_sys_ioctl(*hostfd, TIOCGRANTPT, 0));
	/* unlockpt -- noop */
	RL(rump_sys_ioctl(*hostfd, TIOCPTSNAME, &pm));
	pts = pm.sn;
	RL(*appfd = rump_sys_open(pts, O_RDWR|O_NOCTTY));

	RL(rump_sys_ioctl(*appfd, TIOCGETA, &t));
	t.c_lflag &= ~ICANON;	/* block rather than drop input */
	RL(rump_sys_ioctl(*appfd, TIOCSETA, &t));

	fprintf(stderr, "hostfd=%d appfd=%d\n", *hostfd, *appfd);
}

union sockaddr_union {
	struct sockaddr		sa;
	struct sockaddr_un	sun;
};

static void
socksetup(int *serverfd, union sockaddr_union *sun, socklen_t *sunlenp)
{

	ATF_REQUIRE(sizeof(sun->sun) <= *sunlenp);
	memset(sun, 0, sizeof(*sun));
	sun->sun.sun_family = AF_LOCAL;
	strlcpy(sun->sun.sun_path, "sock", sizeof(sun->sun.sun_path));
	*sunlenp = SUN_LEN(&sun->sun);

	RL(*serverfd = rump_sys_socket(AF_LOCAL, SOCK_STREAM, 0));
	RL(rump_sys_bind(*serverfd, &sun->sa, *sunlenp));
	RL(rump_sys_listen(*serverfd, 1));
}

ATF_TC(fifo_read);
ATF_TC_HEAD(fifo_read, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test named fifo read fails on close");
}
ATF_TC_BODY(fifo_read, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;

	rump_init();

	memset(F, 0, sizeof(*F));
	F->op = &doread;
	F->fd = fifo_setup(O_RDONLY);
	atf_tc_expect_fail("PR kern/57659:" /* similar bug for fifos */
	    " closing pipe writefd fails to wake concurrent write"
	    " on same writefd");
	testfdrestart(F);
}

ATF_TC(fifo_pollread);
ATF_TC_HEAD(fifo_pollread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test poll waiting for named fifo readability fails on close");
}
ATF_TC_BODY(fifo_pollread, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;

	rump_init();

	memset(F, 0, sizeof(*F));
	F->op = &dopollread;
	F->fd = fifo_setup(O_RDONLY);
	testfdrestart(F);
}

ATF_TC(fifo_selectread);
ATF_TC_HEAD(fifo_selectread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test select waiting for named fifo readability fails on close");
}
ATF_TC_BODY(fifo_selectread, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;

	rump_init();

	memset(F, 0, sizeof(*F));
	F->op = &doselectread;
	F->fd = fifo_setup(O_RDONLY);
	testfdrestart(F);
}

ATF_TC(fifo_write);
ATF_TC_HEAD(fifo_write, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test named fifo write fails on close");
}
ATF_TC_BODY(fifo_write, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;

	rump_init();

	memset(F, 0, sizeof(*F));
	F->op = &dowrite;
	F->fd = fifo_setup(O_WRONLY);
	atf_tc_expect_fail("PR kern/57659:" /* similar bug for fifos */
	    " closing pipe writefd fails to wake concurrent write"
	    " on same writefd");
	testfdrestart(F);
}

ATF_TC(fifo_pollwrite);
ATF_TC_HEAD(fifo_pollwrite, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test poll waiting for named fifo writability fails on close");
}
ATF_TC_BODY(fifo_pollwrite, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;

	rump_init();

	memset(F, 0, sizeof(*F));
	F->op = &dopollwrite;
	F->fd = fifo_setup(O_WRONLY);
	testfdrestart(F);
}

ATF_TC(fifo_selectwrite);
ATF_TC_HEAD(fifo_selectwrite, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test select waiting for named fifo writability fails on close");
}
ATF_TC_BODY(fifo_selectwrite, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;

	rump_init();

	memset(F, 0, sizeof(*F));
	F->op = &doselectwrite;
	F->fd = fifo_setup(O_WRONLY);
	testfdrestart(F);
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

ATF_TC(pipe_pollread);
ATF_TC_HEAD(pipe_pollread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test poll waiting for pipe readability wakes on close");
}
ATF_TC_BODY(pipe_pollread, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int fd[2];

	rump_init();

	RL(rump_sys_pipe(fd));

	memset(F, 0, sizeof(*F));
	F->op = &dopollread;
	F->fd = fd[0];
	testfdrestart(F);
}

ATF_TC(pipe_selectread);
ATF_TC_HEAD(pipe_selectread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test select waiting for pipe readability wakes on close");
}
ATF_TC_BODY(pipe_selectread, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int fd[2];

	rump_init();

	RL(rump_sys_pipe(fd));

	memset(F, 0, sizeof(*F));
	F->op = &doselectread;
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

ATF_TC(pipe_pollwrite);
ATF_TC_HEAD(pipe_pollwrite, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test poll waiting for pipe writability wakes on close");
}
ATF_TC_BODY(pipe_pollwrite, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int fd[2];

	rump_init();

	RL(rump_sys_pipe(fd));

	memset(F, 0, sizeof(*F));
	F->op = &dopollwrite;
	F->fd = fd[1];
	testfdrestart(F);
}

ATF_TC(pipe_selectwrite);
ATF_TC_HEAD(pipe_selectwrite, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test select waiting for pipe writability wakes on close");
}
ATF_TC_BODY(pipe_selectwrite, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int fd[2];

	rump_init();

	RL(rump_sys_pipe(fd));

	memset(F, 0, sizeof(*F));
	F->op = &doselectwrite;
	F->fd = fd[1];
	testfdrestart(F);
}

ATF_TC(ptyhost_read);
ATF_TC_HEAD(ptyhost_read, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test ptyhost read fails on close");
}
ATF_TC_BODY(ptyhost_read, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int hostfd, appfd;

	rump_init();

	ptysetup(&hostfd, &appfd);

	memset(F, 0, sizeof(*F));
	F->op = &doread;
	F->fd = hostfd;
	atf_tc_expect_fail("PR kern/57659:" /* similar bug for pty host side */
	    " closing pipe writefd fails to wake concurrent write"
	    " on same writefd");
	testfdrestart(F);
}

ATF_TC(ptyhost_pollread);
ATF_TC_HEAD(ptyhost_pollread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test poll waiting for ptyhost readability wakes on close");
}
ATF_TC_BODY(ptyhost_pollread, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int hostfd, appfd;

	rump_init();

	ptysetup(&hostfd, &appfd);

	memset(F, 0, sizeof(*F));
	F->op = &dopollread;
	F->fd = hostfd;
	testfdrestart(F);
}

ATF_TC(ptyhost_selectread);
ATF_TC_HEAD(ptyhost_selectread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test select waiting for ptyhost readability wakes on close");
}
ATF_TC_BODY(ptyhost_selectread, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int hostfd, appfd;

	rump_init();

	ptysetup(&hostfd, &appfd);

	memset(F, 0, sizeof(*F));
	F->op = &doselectread;
	F->fd = hostfd;
	testfdrestart(F);
}

ATF_TC(ptyhost_write);
ATF_TC_HEAD(ptyhost_write, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test ptyhost write fails on close");
}
ATF_TC_BODY(ptyhost_write, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int hostfd, appfd;

	rump_init();

	ptysetup(&hostfd, &appfd);

	memset(F, 0, sizeof(*F));
	F->op = &dowrite;
	F->fd = hostfd;
	atf_tc_expect_fail("PR kern/57659:" /* similar bug for pty host side */
	    " closing pipe writefd fails to wake concurrent write"
	    " on same writefd");
	testfdrestart(F);
}

ATF_TC(ptyhost_pollwrite);
ATF_TC_HEAD(ptyhost_pollwrite, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test poll waiting for ptyhost writability wakes on close");
}
ATF_TC_BODY(ptyhost_pollwrite, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int hostfd, appfd;

	rump_init();

	ptysetup(&hostfd, &appfd);

	memset(F, 0, sizeof(*F));
	F->op = &dopollwrite;
	F->fd = hostfd;
	testfdrestart(F);
}

ATF_TC(ptyhost_selectwrite);
ATF_TC_HEAD(ptyhost_selectwrite, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test select waiting for ptyhost writability wakes on close");
}
ATF_TC_BODY(ptyhost_selectwrite, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int hostfd, appfd;

	rump_init();

	ptysetup(&hostfd, &appfd);

	memset(F, 0, sizeof(*F));
	F->op = &doselectwrite;
	F->fd = hostfd;
	testfdrestart(F);
}

ATF_TC(ptyapp_read);
ATF_TC_HEAD(ptyapp_read, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test ptyapp read fails on close");
}
ATF_TC_BODY(ptyapp_read, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int hostfd, appfd;

	rump_init();

	ptysetup(&hostfd, &appfd);

	memset(F, 0, sizeof(*F));
	F->op = &doread;
	F->fd = appfd;
	atf_tc_expect_fail("PR kern/57659:" /* similar bug for pty app side */
	    " closing pipe writefd fails to wake concurrent write"
	    " on same writefd");
	testfdrestart(F);
}

ATF_TC(ptyapp_pollread);
ATF_TC_HEAD(ptyapp_pollread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test poll waiting for ptyapp readability wakes on close");
}
ATF_TC_BODY(ptyapp_pollread, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int hostfd, appfd;

	rump_init();

	ptysetup(&hostfd, &appfd);

	memset(F, 0, sizeof(*F));
	F->op = &dopollread;
	F->fd = appfd;
	testfdrestart(F);
}

ATF_TC(ptyapp_selectread);
ATF_TC_HEAD(ptyapp_selectread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test select waiting for ptyapp readability wakes on close");
}
ATF_TC_BODY(ptyapp_selectread, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int hostfd, appfd;

	rump_init();

	ptysetup(&hostfd, &appfd);

	memset(F, 0, sizeof(*F));
	F->op = &doselectread;
	F->fd = appfd;
	testfdrestart(F);
}

ATF_TC(ptyapp_write);
ATF_TC_HEAD(ptyapp_write, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test ptyapp write fails on close");
}
ATF_TC_BODY(ptyapp_write, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int hostfd, appfd;

	rump_init();

	ptysetup(&hostfd, &appfd);

	memset(F, 0, sizeof(*F));
	F->op = &dowrite;
	F->fd = appfd;
	atf_tc_expect_fail("PR kern/57659:" /* similar bug for pty app side */
	    " closing pipe writefd fails to wake concurrent write"
	    " on same writefd");
	testfdrestart(F);
}

ATF_TC(ptyapp_pollwrite);
ATF_TC_HEAD(ptyapp_pollwrite, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test poll waiting for ptyapp writability wakes on close");
}
ATF_TC_BODY(ptyapp_pollwrite, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int hostfd, appfd;

	rump_init();

	ptysetup(&hostfd, &appfd);

	memset(F, 0, sizeof(*F));
	F->op = &dopollwrite;
	F->fd = appfd;
	atf_tc_expect_fail("PR kern/57659:" /* similar bug for pty app side */
	    " closing pipe writefd fails to wake concurrent write"
	    " on same writefd");
	testfdrestart(F);
}

ATF_TC(ptyapp_selectwrite);
ATF_TC_HEAD(ptyapp_selectwrite, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test select waiting for ptyapp writability wakes on close");
}
ATF_TC_BODY(ptyapp_selectwrite, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int hostfd, appfd;

	rump_init();

	ptysetup(&hostfd, &appfd);

	memset(F, 0, sizeof(*F));
	F->op = &doselectwrite;
	F->fd = appfd;
	atf_tc_expect_fail("PR kern/57659:" /* similar bug for pty app side */
	    " closing pipe writefd fails to wake concurrent write"
	    " on same writefd");
	testfdrestart(F);
}

ATF_TC(socket_accept);
ATF_TC_HEAD(socket_accept, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test socket accept fails on close");
}
ATF_TC_BODY(socket_accept, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int serverfd;
	union sockaddr_union sun;
	socklen_t socklen = sizeof(sun);

	rump_init();

	socksetup(&serverfd, &sun, &socklen);

	memset(F, 0, sizeof(*F));
	F->op = &doaccept;
	F->fd = serverfd;
	F->sa = &sun.sa;
	F->salen = socklen;
	testfdrestart(F);
}

ATF_TC(socket_connect);
ATF_TC_HEAD(socket_connect, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test socket connect fails on close");
}
ATF_TC_BODY(socket_connect, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int serverfd;
	union sockaddr_union sun;
	socklen_t socklen = sizeof(sun);
	int clientfd;
	int connwait = 1;

	rump_init();


	socksetup(&serverfd, &sun, &socklen);
	RL(clientfd = rump_sys_socket(AF_LOCAL, SOCK_STREAM, 0));

	RL(rump_sys_setsockopt(clientfd, SOL_LOCAL, LOCAL_CONNWAIT, &connwait,
		    sizeof(connwait)));

	memset(F, 0, sizeof(*F));
	F->op = &doconnect;
	F->fd = clientfd;
	F->sa = &sun.sa;
	F->salen = socklen;
	atf_tc_expect_fail("PR kern/57659:"
	    " closing pipe writefd fails to wake concurrent write"
	    " on same writefd");
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

ATF_TC(socketpair_pollread);
ATF_TC_HEAD(socketpair_pollread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test socketpair poll for readability wakes on close");
}
ATF_TC_BODY(socketpair_pollread, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int fd[2];

	rump_init();

	RL(rump_sys_socketpair(AF_LOCAL, SOCK_STREAM, 0, fd));

	memset(F, 0, sizeof(*F));
	F->op = &dopollread;
	F->fd = fd[0];
	testfdrestart(F);
}

ATF_TC(socketpair_selectread);
ATF_TC_HEAD(socketpair_selectread, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test socketpair select for readability wakes on close");
}
ATF_TC_BODY(socketpair_selectread, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int fd[2];

	rump_init();

	RL(rump_sys_socketpair(AF_LOCAL, SOCK_STREAM, 0, fd));

	memset(F, 0, sizeof(*F));
	F->op = &doselectread;
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

ATF_TC(socketpair_pollwrite);
ATF_TC_HEAD(socketpair_pollwrite, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test poll waiting for socket writability wakes on close");
}
ATF_TC_BODY(socketpair_pollwrite, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int fd[2];

	rump_init();

	RL(rump_sys_socketpair(AF_LOCAL, SOCK_STREAM, 0, fd));

	memset(F, 0, sizeof(*F));
	F->op = &dopollwrite;
	F->fd = fd[0];
	testfdrestart(F);
}

ATF_TC(socketpair_selectwrite);
ATF_TC_HEAD(socketpair_selectwrite, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test select waiting for socket writability wakes on close");
}
ATF_TC_BODY(socketpair_selectwrite, tc)
{
	struct fdrestart fdrestart, *F = &fdrestart;
	int fd[2];

	rump_init();

	RL(rump_sys_socketpair(AF_LOCAL, SOCK_STREAM, 0, fd));

	memset(F, 0, sizeof(*F));
	F->op = &doselectwrite;
	F->fd = fd[0];
	testfdrestart(F);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, fifo_pollread);
	ATF_TP_ADD_TC(tp, fifo_pollwrite);
	ATF_TP_ADD_TC(tp, fifo_read);
	ATF_TP_ADD_TC(tp, fifo_selectread);
	ATF_TP_ADD_TC(tp, fifo_selectwrite);
	ATF_TP_ADD_TC(tp, fifo_write);
	ATF_TP_ADD_TC(tp, pipe_pollread);
	ATF_TP_ADD_TC(tp, pipe_pollwrite);
	ATF_TP_ADD_TC(tp, pipe_read);
	ATF_TP_ADD_TC(tp, pipe_selectread);
	ATF_TP_ADD_TC(tp, pipe_selectwrite);
	ATF_TP_ADD_TC(tp, pipe_write);
	ATF_TP_ADD_TC(tp, ptyapp_pollread);
	ATF_TP_ADD_TC(tp, ptyapp_pollwrite);
	ATF_TP_ADD_TC(tp, ptyapp_read);
	ATF_TP_ADD_TC(tp, ptyapp_selectread);
	ATF_TP_ADD_TC(tp, ptyapp_selectwrite);
	ATF_TP_ADD_TC(tp, ptyapp_write);
	ATF_TP_ADD_TC(tp, ptyhost_pollread);
	ATF_TP_ADD_TC(tp, ptyhost_pollwrite);
	ATF_TP_ADD_TC(tp, ptyhost_read);
	ATF_TP_ADD_TC(tp, ptyhost_selectread);
	ATF_TP_ADD_TC(tp, ptyhost_selectwrite);
	ATF_TP_ADD_TC(tp, ptyhost_write);
	ATF_TP_ADD_TC(tp, socket_accept);
	ATF_TP_ADD_TC(tp, socket_connect);
	ATF_TP_ADD_TC(tp, socketpair_pollread);
	ATF_TP_ADD_TC(tp, socketpair_pollwrite);
	ATF_TP_ADD_TC(tp, socketpair_read);
	ATF_TP_ADD_TC(tp, socketpair_selectread);
	ATF_TP_ADD_TC(tp, socketpair_selectwrite);
	ATF_TP_ADD_TC(tp, socketpair_write);

	return atf_no_error();
}
