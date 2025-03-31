/*	$NetBSD: t_cancellation.c,v 1.2 2025/03/31 14:07:10 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_cancellation.c,v 1.2 2025/03/31 14:07:10 riastradh Exp $");

#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <aio.h>
#include <atf-c.h>
#include <fcntl.h>
#include <mqueue.h>
#include <paths.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <string.h>
#include <termios.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#include "h_macros.h"

static const char *
c11thrd_err(int error)
{
	static char buf[32];

	switch (error) {
	case thrd_busy:		return "thrd_busy";
	case thrd_nomem:	return "thrd_nomem";
	case thrd_success:	return "thrd_success";
	case thrd_timedout:	return "thrd_timedout";
	default:
		snprintf(buf, sizeof(buf), "thrd_%d", error);
		return buf;
	}
}

#define	RT(x) do							      \
{									      \
	int RT_rv = (x);						      \
	ATF_REQUIRE_MSG(RT_rv == 0, "%s: %d (%s)",			      \
	    #x, RT_rv, c11thrd_err(RT_rv));				      \
} while (0)

pthread_barrier_t bar;
bool cleanup_done;

static void
cleanup(void *cookie)
{
	bool *cleanup_donep = cookie;

	*cleanup_donep = true;
}

/* POSIX style */
static void *
emptythread(void *cookie)
{
	return NULL;
}

/* C11 style */
static int
emptythrd(void *cookie)
{
	return 123;
}

static void
cleanup_pthread_join(void *cookie)
{
	pthread_t *tp = cookie;
	void *result;

	RZ(pthread_join(*tp, &result));
	ATF_CHECK_MSG(result == NULL, "result=%p", result);
}

static void
cleanup_thrd_join(void *cookie)
{
	thrd_t *tp = cookie;
	int result;

	RT(thrd_join(*tp, &result));
	ATF_CHECK_MSG(result == 123, "result=%d", result);
}

static void
cleanup_msgid(void *cookie)
{
	int *msgidp = cookie;

	/*
	 * These message queue identifiers are persistent, so make sure
	 * to clean them up; otherwise the operator will have to run
	 * `ipcrm -q all' from time to time or else the tests will fail
	 * with ENOSPC.
	 */
	RL(msgctl(*msgidp, IPC_RMID, NULL));
}

/*
 * List of cancellation points in POSIX:
 *
 * https://pubs.opengroup.org/onlinepubs/9799919799.2024edition/functions/V2_chap02.html#tag_16_09_05_02
 */

#if 0
atomic_bool cancelpointreadydone;
#endif

static void
cancelpointready(void)
{

	(void)pthread_barrier_wait(&bar);
	RL(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
#if 0
	atomic_store_release(&cancelpointreadydone, true);
#endif
}

static int
acceptsetup(void)
{
	struct sockaddr_un sun = { .sun_family = AF_LOCAL };
	int sock;

	strncpy(sun.sun_path, "sock", sizeof(sun.sun_path));
	RL(sock = socket(PF_LOCAL, SOCK_STREAM, 0));
	RL(bind(sock, (const struct sockaddr *)&sun, sizeof(sun)));
	RL(listen(sock, 1));

	return sock;
}

static void
cancelpoint_accept(void)
{
	const int sock = acceptsetup();

	cancelpointready();
	RL(accept(sock, NULL, NULL));
}

static void
cancelpoint_accept4(void)
{
	const int sock = acceptsetup();

	cancelpointready();
	RL(accept4(sock, NULL, NULL, O_CLOEXEC));
}

static void
cancelpoint_aio_suspend(void)
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
cancelpoint_clock_nanosleep(void)
{
	/* XXX test all CLOCK_*? */
	struct timespec t = {.tv_sec = 1, .tv_nsec = 0};

	cancelpointready();
	RL(clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL));
}

static void
cancelpoint_close(void)
{
	int fd;

	RL(fd = open("/dev/null", O_RDWR));
	cancelpointready();
	RL(close(fd));
}

static void
cancelpoint_cnd_timedwait(void)
{
	cnd_t cnd;
	mtx_t mtx;
	struct timespec t = {.tv_sec = 1, .tv_nsec = 0};

	RT(cnd_init(&cnd));
	RT(mtx_init(&mtx, mtx_plain));
	cancelpointready();
	RT(mtx_lock(&mtx));
	RT(cnd_timedwait(&cnd, &mtx, &t));
	RT(mtx_unlock(&mtx));
}

static void
cancelpoint_cnd_wait(void)
{
	cnd_t cnd;
	mtx_t mtx;

	RT(cnd_init(&cnd));
	RT(mtx_init(&mtx, mtx_plain));
	cancelpointready();
	RT(mtx_lock(&mtx));
	RT(cnd_wait(&cnd, &mtx));
	RT(mtx_unlock(&mtx));
}

static void
cancelpoint_connect(void)
{
	struct sockaddr_un sun = { .sun_family = AF_LOCAL };
	int sock;

	strncpy(sun.sun_path, "sock", sizeof(sun.sun_path));
	RL(sock = socket(PF_LOCAL, SOCK_STREAM, 0));
	cancelpointready();
	RL(connect(sock, (const struct sockaddr *)&sun, sizeof(sun)));
}

static void
cancelpoint_creat(void)
{

	cancelpointready();
	RL(creat("file", 0666));
}

static void
cancelpoint_fcntl_F_SETLKW(void)
{
	int fd;
	struct flock fl = {
		.l_start = 0,
		.l_len = 0,
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
	};

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	cancelpointready();
	RL(fcntl(fd, F_SETLKW, &fl));
}

static void
cancelpoint_fcntl_F_OFD_SETLKW(void)
{
#ifdef F_OFD_SETLKW
	int fd;
	struct flock fl = {
		.l_start = 0,
		.l_len = 0,
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
	};

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	cancelpointready();
	RL(fcntl(fd, F_OFD_SETLKW, &fl));
#else
	atf_tc_expect_fail("PR kern/59241: POSIX.1-2024:"
	    " OFD-owned file locks");
	atf_tc_fail("no F_OFD_SETLKW");
#endif
}

static void
cancelpoint_fdatasync(void)
{
	int fd;

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	cancelpointready();
	RL(fdatasync(fd));
}

static void
cancelpoint_fsync(void)
{
	int fd;

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	cancelpointready();
	RL(fsync(fd));
}

static void
cancelpoint_lockf_F_LOCK(void)
{
	int fd;

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	cancelpointready();
	RL(lockf(fd, F_LOCK, 0));
}

static void
cancelpoint_mq_receive(void)
{
	mqd_t mq;
	char buf[32];

	RL(mq = mq_open("mq", O_RDWR|O_CREAT, 0666, NULL));
	cancelpointready();
	RL(mq_receive(mq, buf, sizeof(buf), NULL));
}

static void
cancelpoint_mq_send(void)
{
	mqd_t mq;
	char buf[32] = {0};

	RL(mq = mq_open("mq", O_RDWR|O_CREAT, 0666, NULL));
	cancelpointready();
	RL(mq_send(mq, buf, sizeof(buf), 0));
}

static void
cancelpoint_mq_timedreceive(void)
{
	mqd_t mq;
	char buf[32];
	struct timespec t = {.tv_sec = 1, .tv_nsec = 0};

	RL(mq = mq_open("mq", O_RDWR|O_CREAT, 0666, NULL));
	cancelpointready();
	RL(mq_timedreceive(mq, buf, sizeof(buf), NULL, &t));
}

static void
cancelpoint_mq_timedsend(void)
{
	mqd_t mq;
	char buf[32] = {0};
	struct timespec t = {.tv_sec = 1, .tv_nsec = 0};

	RL(mq = mq_open("mq", O_RDWR|O_CREAT, 0666, NULL));
	cancelpointready();
	RL(mq_timedsend(mq, buf, sizeof(buf), 0, &t));
}

static void
cancelpoint_msgrcv(void)
{
	int msgid;
	char buf[32];

	RL(msgid = msgget(IPC_PRIVATE, IPC_CREAT));
	pthread_cleanup_push(&cleanup_msgid, &msgid);
	cancelpointready();
	RL(msgrcv(msgid, buf, sizeof(buf), 0, 0));
	pthread_cleanup_pop(/*execute*/1);
}

static void
cancelpoint_msgsnd(void)
{
	int msgid;
	char buf[32] = {0};

	RL(msgid = msgget(IPC_PRIVATE, IPC_CREAT));
	pthread_cleanup_push(&cleanup_msgid, &msgid);
	cancelpointready();
	RL(msgsnd(msgid, buf, sizeof(buf), 0));
	pthread_cleanup_pop(/*execute*/1);
}

static void
cancelpoint_msync(void)
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
	RL(msync(map, pagesize, MS_SYNC));
}

static void
cancelpoint_nanosleep(void)
{
	/* XXX test all CLOCK_*? */
	struct timespec t = {.tv_sec = 1, .tv_nsec = 0};

	cancelpointready();
	RL(nanosleep(&t, NULL));
}

static void
cancelpoint_open(void)
{

	cancelpointready();
	RL(open("file", O_RDWR));
}

static void
cancelpoint_openat(void)
{

	cancelpointready();
	RL(openat(AT_FDCWD, "file", O_RDWR));
}

static void
cancelpoint_pause(void)
{

	cancelpointready();
	RL(pause());
}

static void
cancelpoint_poll(void)
{
	int fd[2];
	struct pollfd pfd;

	RL(pipe(fd));
	pfd.fd = fd[0];
	pfd.events = POLLIN;
	cancelpointready();
	RL(poll(&pfd, 1, 1000));
}

static void
cancelpoint_posix_close(void)
{
#if 0
	int fd;

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	cancelpointready();
	RL(posix_close(fd, POSIX_CLOSE_RESTART));
#else
	atf_tc_expect_fail("PR kern/58929: POSIX.1-2024 compliance:"
	    " posix_close, POSIX_CLOSE_RESTART");
	atf_tc_fail("no posix_close");
#endif
}

static void
cancelpoint_ppoll(void)
{
	int fd[2];
	struct pollfd pfd;
	struct timespec t = {.tv_sec = 1, .tv_nsec = 0};

	RL(pipe(fd));
	pfd.fd = fd[0];
	pfd.events = POLLIN;
	cancelpointready();
	RL(ppoll(&pfd, 1, &t, NULL));
}

static void
cancelpoint_pread(void)
{
	int fd;
	char buf[1];

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	cancelpointready();
	RL(pread(fd, buf, sizeof(buf), 1));
}


static void
cancelpoint_pselect(void)
{
	int fd[2];
	fd_set readfd;
	struct timespec t = {.tv_sec = 1, .tv_nsec = 0};

	FD_ZERO(&readfd);

	RL(pipe(fd));
	FD_SET(fd[0], &readfd);
	cancelpointready();
	RL(pselect(fd[0] + 1, &readfd, NULL, NULL, &t, NULL));
}

static void
cancelpoint_pthread_cond_clockwait(void)
{
#if 0
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	struct timespec t = {.tv_sec = 1, .tv_nsec = 0};

	RZ(pthread_cond_init(&cond, NULL));
	RZ(pthread_mutex_init(&mutex, NULL));
	cancelpointready();
	RZ(pthread_mutex_lock(&mutex));
	RZ(pthread_cond_clockwait(&cond, &mutex, CLOCK_MONOTONIC, &t));
	RZ(pthread_mutex_unlock(&mutex));
#else
	atf_tc_expect_fail("PR lib/59142: POSIX.1-2024:"
	    " pthread_cond_clockwait and company");
	atf_tc_fail("no posix_cond_clockwait");
#endif
}

static void
cancelpoint_pthread_cond_timedwait(void)
{
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	struct timespec t = {.tv_sec = 1, .tv_nsec = 0};

	RZ(pthread_cond_init(&cond, NULL));
	RZ(pthread_mutex_init(&mutex, NULL));
	cancelpointready();
	RZ(pthread_mutex_lock(&mutex));
	RZ(pthread_cond_timedwait(&cond, &mutex, &t));
	RZ(pthread_mutex_unlock(&mutex));
}

static void
cancelpoint_pthread_cond_wait(void)
{
	pthread_cond_t cond;
	pthread_mutex_t mutex;

	RZ(pthread_cond_init(&cond, NULL));
	RZ(pthread_mutex_init(&mutex, NULL));
	cancelpointready();
	RZ(pthread_mutex_lock(&mutex));
	RZ(pthread_cond_wait(&cond, &mutex));
	RZ(pthread_mutex_unlock(&mutex));
}

static void
cancelpoint_pthread_join(void)
{
	pthread_t t;

	RZ(pthread_create(&t, NULL, &emptythread, NULL));
	pthread_cleanup_push(&cleanup_pthread_join, &t);
	cancelpointready();
	RZ(pthread_join(t, NULL));
	pthread_cleanup_pop(/*execute*/0);
}

static void
cancelpoint_pthread_testcancel(void)
{

	cancelpointready();
	pthread_testcancel();
}

static void
cancelpoint_pwrite(void)
{
	int fd;
	char buf[1] = {0};

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	cancelpointready();
	RL(pwrite(fd, buf, sizeof(buf), 1));
}

static void
cancelpoint_read(void)
{
	int fd;
	char buf[1];

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	cancelpointready();
	RL(read(fd, buf, sizeof(buf)));
}

static void
cancelpoint_readv(void)
{
	int fd;
	char buf[1];
	struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	cancelpointready();
	RL(readv(fd, &iov, 1));
}

static void
cancelpoint_recv(void)
{
	struct sockaddr_un sun = { .sun_family = AF_LOCAL };
	int sock;
	char buf[1];

	strncpy(sun.sun_path, "sock", sizeof(sun.sun_path));
	RL(sock = socket(PF_LOCAL, SOCK_DGRAM, 0));
	RL(bind(sock, (const struct sockaddr *)&sun, sizeof(sun)));
	cancelpointready();
	RL(recv(sock, buf, sizeof(buf), 0));
}

static void
cancelpoint_recvfrom(void)
{
	struct sockaddr_un sun = { .sun_family = AF_LOCAL };
	int sock;
	char buf[1];
	struct sockaddr_storage ss;
	socklen_t len = sizeof(ss);

	strncpy(sun.sun_path, "sock", sizeof(sun.sun_path));
	RL(sock = socket(PF_LOCAL, SOCK_DGRAM, 0));
	RL(bind(sock, (const struct sockaddr *)&sun, sizeof(sun)));
	cancelpointready();
	RL(recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&ss, &len));
}

static void
cancelpoint_recvmsg(void)
{
	struct sockaddr_un sun = { .sun_family = AF_LOCAL };
	int sock;
	char buf[1];
	struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };
	struct msghdr msg = {
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};

	strncpy(sun.sun_path, "sock", sizeof(sun.sun_path));
	RL(sock = socket(PF_LOCAL, SOCK_DGRAM, 0));
	RL(bind(sock, (const struct sockaddr *)&sun, sizeof(sun)));
	cancelpointready();
	RL(recvmsg(sock, &msg, 0));
}

static void
cancelpoint_select(void)
{
	int fd[2];
	fd_set readfd;
	struct timeval t = {.tv_sec = 1, .tv_usec = 0};

	FD_ZERO(&readfd);

	RL(pipe(fd));
	FD_SET(fd[0], &readfd);
	cancelpointready();
	RL(select(fd[0] + 1, &readfd, NULL, NULL, &t));
}

static void
cancelpoint_send(void)
{
	struct sockaddr_un sun = { .sun_family = AF_LOCAL };
	int sock;
	char buf[1] = {0};

	strncpy(sun.sun_path, "sock", sizeof(sun.sun_path));
	RL(sock = socket(PF_LOCAL, SOCK_DGRAM, 0));
	RL(bind(sock, (const struct sockaddr *)&sun, sizeof(sun)));
	cancelpointready();
	RL(send(sock, buf, sizeof(buf), 0));
}

static void
cancelpoint_sendto(void)
{
	struct sockaddr_un sun = { .sun_family = AF_LOCAL };
	int sock;
	char buf[1] = {0};

	strncpy(sun.sun_path, "sock", sizeof(sun.sun_path));
	RL(sock = socket(PF_LOCAL, SOCK_DGRAM, 0));
	cancelpointready();
	RL(sendto(sock, buf, sizeof(buf), 0, (const struct sockaddr *)&sun,
		sizeof(sun)));
}

static void
cancelpoint_sendmsg(void)
{
	struct sockaddr_un sun = { .sun_family = AF_LOCAL };
	int sock;
	char buf[1] = {0};
	struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };
	struct msghdr msg = {
		.msg_name = (struct sockaddr *)&sun,
		.msg_namelen = sizeof(sun),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};

	strncpy(sun.sun_path, "sock", sizeof(sun.sun_path));
	RL(sock = socket(PF_LOCAL, SOCK_DGRAM, 0));
	cancelpointready();
	RL(sendmsg(sock, &msg, 0));
}

static void
cancelpoint_sigsuspend(void)
{
	sigset_t mask, omask;

	RL(sigfillset(&mask));
	RL(sigprocmask(SIG_BLOCK, &mask, &omask));
	cancelpointready();
	RL(sigsuspend(&omask));
}

static void
cancelpoint_sigtimedwait(void)
{
	sigset_t mask, omask;
	siginfo_t info;
	struct timespec t = {.tv_sec = 1, .tv_nsec = 0};

	RL(sigfillset(&mask));
	RL(sigprocmask(SIG_BLOCK, &mask, &omask));
	cancelpointready();
	RL(sigtimedwait(&omask, &info, &t));
}

static void
cancelpoint_sigwait(void)
{
	sigset_t mask, omask;
	int sig;

	RL(sigfillset(&mask));
	RL(sigprocmask(SIG_BLOCK, &mask, &omask));
	cancelpointready();
	RL(sigwait(&omask, &sig));
}

static void
cancelpoint_sigwaitinfo(void)
{
	sigset_t mask, omask;
	siginfo_t info;

	RL(sigfillset(&mask));
	RL(sigprocmask(SIG_BLOCK, &mask, &omask));
	cancelpointready();
	RL(sigwaitinfo(&omask, &info));
}

static void
cancelpoint_sleep(void)
{

	cancelpointready();
	(void)sleep(1);
}

static void
cancelpoint_tcdrain(void)
{
	int hostfd, appfd;
	char *pts;

	RL(hostfd = posix_openpt(O_RDWR|O_NOCTTY));
	RL(grantpt(hostfd));
	RL(unlockpt(hostfd));
	REQUIRE_LIBC(pts = ptsname(hostfd), NULL);
	RL(appfd = open(pts, O_RDWR|O_NOCTTY));
	cancelpointready();
	RL(tcdrain(appfd));
}

static void
cancelpoint_thrd_join(void)
{
	thrd_t t;

	RT(thrd_create(&t, &emptythrd, NULL));
	pthread_cleanup_push(&cleanup_thrd_join, &t);
	cancelpointready();
	RT(thrd_join(t, NULL));
	pthread_cleanup_pop(/*execute*/0);
}

static void
cancelpoint_thrd_sleep(void)
{
	struct timespec t = {.tv_sec = 1, .tv_nsec = 0};

	cancelpointready();
	RT(thrd_sleep(&t, NULL));
}

static void
cancelpoint_wait(void)
{

	cancelpointready();
	RL(wait(NULL));
}

static void
cancelpoint_waitid(void)
{

	cancelpointready();
	RL(waitid(P_ALL, 0, NULL, 0));
}

static void
cancelpoint_waitpid(void)
{

	cancelpointready();
	RL(waitpid(-1, NULL, 0));
}

static void
cancelpoint_write(void)
{
	int fd;
	char buf[1] = {0};

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	cancelpointready();
	RL(write(fd, buf, sizeof(buf)));
}

static void
cancelpoint_writev(void)
{
	int fd;
	char buf[1] = {0};
	struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };

	RL(fd = open("file", O_RDWR|O_CREAT, 0666));
	cancelpointready();
	RL(writev(fd, &iov, 1));
}

static void *
thread_cancelpoint(void *cookie)
{
	void (*cancelpoint)(void) = cookie;

	RL(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));
	(void)pthread_barrier_wait(&bar);

	pthread_cleanup_push(&cleanup, &cleanup_done);
	(*cancelpoint)();
	pthread_cleanup_pop(/*execute*/0);

	return NULL;
}

static void
test_cancelpoint_before(void (*cancelpoint)(void))
{
	pthread_t t;
	void *result;

	RZ(pthread_barrier_init(&bar, NULL, 2));
	RZ(pthread_create(&t, NULL, &thread_cancelpoint, cancelpoint));

	(void)pthread_barrier_wait(&bar);
	fprintf(stderr, "cancel\n");
	RZ(pthread_cancel(t));
	(void)pthread_barrier_wait(&bar);

	alarm(1);
	RZ(pthread_join(t, &result));
	ATF_CHECK_MSG(result == PTHREAD_CANCELED,
	    "result=%p PTHREAD_CANCELED=%p", result, PTHREAD_CANCELED);
	ATF_CHECK(cleanup_done);
}

#if 0
static void
test_cancelpoint_wakeup(void (*cancelpoint)(void))
{
	pthread_t t;

	RZ(pthread_barrier_init(&bar, NULL, 2));
	RZ(pthread_create(&t, NULL, &cancelpoint_thread, cancelpoint));

	(void)pthread_barrier_wait(&bar);
	while (!atomic_load_acquire(&cancelpointreadydone))
		continue;
	while (!pthread_sleeping(t)) /* XXX find a way to do this */
		continue;
	RZ(pthread_cancel(t));
}
#endif

#define	TEST_CANCELPOINT(CANCELPOINT, XFAIL)				      \
ATF_TC(CANCELPOINT);							      \
ATF_TC_HEAD(CANCELPOINT, tc)						      \
{									      \
	atf_tc_set_md_var(tc, "descr", "Test cancellation point: "	      \
	    #CANCELPOINT);						      \
}									      \
ATF_TC_BODY(CANCELPOINT, tc)						      \
{									      \
	XFAIL;								      \
	test_cancelpoint_before(&CANCELPOINT);				      \
}
#define	ADD_TEST_CANCELPOINT(CANCELPOINT)				      \
	ATF_TP_ADD_TC(tp, CANCELPOINT)

TEST_CANCELPOINT(cancelpoint_accept, __nothing)
TEST_CANCELPOINT(cancelpoint_accept4,
    atf_tc_expect_signal(SIGALRM,
	"PR lib/59240: POSIX.1-2024: cancellation point audit"))
TEST_CANCELPOINT(cancelpoint_aio_suspend, __nothing)
TEST_CANCELPOINT(cancelpoint_clock_nanosleep, __nothing)
TEST_CANCELPOINT(cancelpoint_close, __nothing)
TEST_CANCELPOINT(cancelpoint_cnd_timedwait, __nothing)
TEST_CANCELPOINT(cancelpoint_cnd_wait, __nothing)
TEST_CANCELPOINT(cancelpoint_connect, __nothing)
TEST_CANCELPOINT(cancelpoint_creat, __nothing)
TEST_CANCELPOINT(cancelpoint_fcntl_F_SETLKW, __nothing)
TEST_CANCELPOINT(cancelpoint_fcntl_F_OFD_SETLKW, __nothing)
TEST_CANCELPOINT(cancelpoint_fdatasync, __nothing)
TEST_CANCELPOINT(cancelpoint_fsync, __nothing)
TEST_CANCELPOINT(cancelpoint_lockf_F_LOCK, __nothing)
TEST_CANCELPOINT(cancelpoint_mq_receive, __nothing)
TEST_CANCELPOINT(cancelpoint_mq_send, __nothing)
TEST_CANCELPOINT(cancelpoint_mq_timedreceive, __nothing)
TEST_CANCELPOINT(cancelpoint_mq_timedsend, __nothing)
TEST_CANCELPOINT(cancelpoint_msgrcv, __nothing)
TEST_CANCELPOINT(cancelpoint_msgsnd, __nothing)
TEST_CANCELPOINT(cancelpoint_msync, __nothing)
TEST_CANCELPOINT(cancelpoint_nanosleep, __nothing)
TEST_CANCELPOINT(cancelpoint_open, __nothing)
TEST_CANCELPOINT(cancelpoint_openat, __nothing)
TEST_CANCELPOINT(cancelpoint_pause, __nothing)
TEST_CANCELPOINT(cancelpoint_poll, __nothing)
TEST_CANCELPOINT(cancelpoint_posix_close, __nothing)
TEST_CANCELPOINT(cancelpoint_ppoll, __nothing)
TEST_CANCELPOINT(cancelpoint_pread, __nothing)
TEST_CANCELPOINT(cancelpoint_pselect, __nothing)
TEST_CANCELPOINT(cancelpoint_pthread_cond_clockwait, __nothing)
TEST_CANCELPOINT(cancelpoint_pthread_cond_timedwait, __nothing)
TEST_CANCELPOINT(cancelpoint_pthread_cond_wait, __nothing)
TEST_CANCELPOINT(cancelpoint_pthread_join, __nothing)
TEST_CANCELPOINT(cancelpoint_pthread_testcancel, __nothing)
TEST_CANCELPOINT(cancelpoint_pwrite, __nothing)
TEST_CANCELPOINT(cancelpoint_read, __nothing)
TEST_CANCELPOINT(cancelpoint_readv, __nothing)
TEST_CANCELPOINT(cancelpoint_recv, __nothing)
TEST_CANCELPOINT(cancelpoint_recvfrom, __nothing)
TEST_CANCELPOINT(cancelpoint_recvmsg, __nothing)
TEST_CANCELPOINT(cancelpoint_select, __nothing)
TEST_CANCELPOINT(cancelpoint_send, __nothing)
TEST_CANCELPOINT(cancelpoint_sendto, __nothing)
TEST_CANCELPOINT(cancelpoint_sendmsg, __nothing)
TEST_CANCELPOINT(cancelpoint_sigsuspend, __nothing)
TEST_CANCELPOINT(cancelpoint_sigtimedwait, __nothing)
TEST_CANCELPOINT(cancelpoint_sigwait, __nothing)
TEST_CANCELPOINT(cancelpoint_sigwaitinfo, __nothing)
TEST_CANCELPOINT(cancelpoint_sleep, __nothing)
TEST_CANCELPOINT(cancelpoint_tcdrain,
    atf_tc_expect_fail("PR lib/59240: POSIX.1-2024: cancellation point audit"))
TEST_CANCELPOINT(cancelpoint_thrd_join, __nothing)
TEST_CANCELPOINT(cancelpoint_thrd_sleep, __nothing)
TEST_CANCELPOINT(cancelpoint_wait, __nothing)
TEST_CANCELPOINT(cancelpoint_waitid, __nothing)
TEST_CANCELPOINT(cancelpoint_waitpid, __nothing)
TEST_CANCELPOINT(cancelpoint_write, __nothing)
TEST_CANCELPOINT(cancelpoint_writev, __nothing)

ATF_TC(cleanuppop0);
ATF_TC_HEAD(cleanuppop0, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pthread_cleanup_pop(0)");
}
ATF_TC_BODY(cleanuppop0, tc)
{

	pthread_cleanup_push(&cleanup, &cleanup_done);
	pthread_cleanup_pop(/*execute*/0);
	ATF_CHECK(!cleanup_done);
}

ATF_TC(cleanuppop1);
ATF_TC_HEAD(cleanuppop1, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test pthread_cleanup_pop(1)");
}
ATF_TC_BODY(cleanuppop1, tc)
{

	pthread_cleanup_push(&cleanup, &cleanup_done);
	pthread_cleanup_pop(/*execute*/1);
	ATF_CHECK(cleanup_done);
}

static void *
cancelself_async(void *cookie)
{
	int *n = cookie;

	RZ(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL));

	pthread_cleanup_push(&cleanup, &cleanup_done);

	*n = 1;
	RZ(pthread_cancel(pthread_self())); /* cancel */
	*n = 2;
	RZ(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));
	pthread_testcancel();
	*n = 3;
	RZ(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
	pthread_testcancel();
	*n = 4;

	pthread_cleanup_pop(/*execute*/0);
	return NULL;
}

ATF_TC(cancelself_async);
ATF_TC_HEAD(cancelself_async, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test pthread_cancel(pthread_self()) async");
}
ATF_TC_BODY(cancelself_async, tc)
{
	int n = 0;
	pthread_t t;

	RZ(pthread_create(&t, NULL, &cancelself_async, &n));

	alarm(1);
	RZ(pthread_join(t, NULL));

	atf_tc_expect_fail("lib/59135: PTHREAD_CANCEL_ASYNCHRONOUS"
	    " doesn't do much");
	ATF_CHECK_MSG(n == 1, "n=%d", n);
	atf_tc_expect_pass();
	ATF_CHECK(cleanup_done);
}

static void *
cancelself_deferred(void *cookie)
{
	int *n = cookie;

	/* PTHREAD_CANCEL_DEFERRED by default */

	pthread_cleanup_push(&cleanup, &cleanup_done);

	*n = 1;
	RZ(pthread_cancel(pthread_self()));
	*n = 2;
	RZ(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));
	*n = 3;
	pthread_testcancel();
	*n = 4;
	RZ(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
	*n = 5;
	pthread_testcancel(); /* cancel */
	*n = 6;

	pthread_cleanup_pop(/*execute*/0);
	return NULL;
}

ATF_TC(cancelself_deferred);
ATF_TC_HEAD(cancelself_deferred, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test pthread_cancel(pthread_self()) deferred");
}
ATF_TC_BODY(cancelself_deferred, tc)
{
	int n = 0;
	pthread_t t;

	RZ(pthread_create(&t, NULL, &cancelself_deferred, &n));

	alarm(1);
	RZ(pthread_join(t, NULL));

	ATF_CHECK_MSG(n == 5, "n=%d", n);
	ATF_CHECK(cleanup_done);
}

static void *
defaults(void *cookie)
{
	int state, type;

	fprintf(stderr, "created thread\n");

	RZ(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &state));
	RZ(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &type));

	ATF_CHECK_MSG(state == PTHREAD_CANCEL_ENABLE,
	    "state=%d PTHREAD_CANCEL_ENABLE=%d PTHREAD_CANCEL_DISABLE=%d",
	    state, PTHREAD_CANCEL_ENABLE, PTHREAD_CANCEL_DISABLE);

	ATF_CHECK_MSG(type == PTHREAD_CANCEL_DEFERRED,
	    "type=%d"
	    " PTHREAD_CANCEL_DEFERRED=%d PTHREAD_CANCEL_ASYNCHRONOUS=%d",
	    type, PTHREAD_CANCEL_DEFERRED, PTHREAD_CANCEL_ASYNCHRONOUS);

	return NULL;
}

ATF_TC(defaults);
ATF_TC_HEAD(defaults, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test default cancelability");
}
ATF_TC_BODY(defaults, tc)
{
	pthread_t t;

	fprintf(stderr, "initial thread\n");
	(void)defaults(NULL);

	RZ(pthread_create(&t, NULL, &defaults, NULL));

	alarm(1);
	RZ(pthread_join(t, NULL));
}

static void *
disable_enable(void *cookie)
{
	int *n = cookie;

	pthread_cleanup_push(&cleanup, &cleanup_done);

	*n = 1;
	pthread_testcancel();
	*n = 2;
	RZ(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));
	*n = 3;
	(void)pthread_barrier_wait(&bar);
	*n = 4;
	pthread_testcancel();
	*n = 5;
	(void)pthread_barrier_wait(&bar);
	*n = 6;
	pthread_testcancel();
	*n = 7;
	RZ(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
	*n = 8;
	pthread_testcancel(); /* cancel */
	*n = 9;

	pthread_cleanup_pop(/*execute*/0);
	return NULL;
}

ATF_TC(disable_enable);
ATF_TC_HEAD(disable_enable, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test disabling and re-enabling cancellation");
}
ATF_TC_BODY(disable_enable, tc)
{
	int n = 0;
	pthread_t t;

	RZ(pthread_barrier_init(&bar, NULL, 2));

	RZ(pthread_create(&t, NULL, &disable_enable, &n));

	(void)pthread_barrier_wait(&bar);
	RZ(pthread_cancel(t));
	(void)pthread_barrier_wait(&bar);

	alarm(1);
	RZ(pthread_join(t, NULL));

	ATF_CHECK_MSG(n == 8, "n=%d", n);
	ATF_CHECK(cleanup_done);
}

static void *
notestcancel_loop_async(void *cookie)
{

	RZ(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL));

	pthread_cleanup_push(&cleanup, &cleanup_done);
	(void)pthread_barrier_wait(&bar);
	for (;;)
		__insn_barrier();
	pthread_cleanup_pop(/*execute*/0);

	return NULL;
}

ATF_TC(notestcancel_loop_async);
ATF_TC_HEAD(notestcancel_loop_async, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test nothing in a loop with PTHREAD_CANCEL_ASYNCHRONOUS");
}
ATF_TC_BODY(notestcancel_loop_async, tc)
{
	pthread_t t;
	void *result;

	RZ(pthread_barrier_init(&bar, NULL, 2));
	RZ(pthread_create(&t, NULL, &notestcancel_loop_async, NULL));

	(void)pthread_barrier_wait(&bar);
	RZ(pthread_cancel(t));

	atf_tc_expect_signal(SIGALRM, "lib/59135: PTHREAD_CANCEL_ASYNCHRONOUS"
	    " doesn't do much");
	alarm(1);
	RZ(pthread_join(t, &result));
	ATF_CHECK_MSG(result == PTHREAD_CANCELED,
	    "result=%p PTHREAD_CANCELED=%p", result, PTHREAD_CANCELED);
	ATF_CHECK(cleanup_done);
}

static void *
disable_enable_async(void *cookie)
{
	int *n = cookie;

	pthread_cleanup_push(&cleanup, &cleanup_done);

	RZ(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL));

	*n = 1;
	pthread_testcancel();
	*n = 2;
	RZ(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));
	*n = 3;
	(void)pthread_barrier_wait(&bar);
	*n = 4;
	pthread_testcancel();
	*n = 5;
	(void)pthread_barrier_wait(&bar);
	*n = 6;
	pthread_testcancel();
	*n = 7;
	RZ(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)); /* cancel */
	*n = 8;
	pthread_testcancel();
	*n = 9;

	pthread_cleanup_pop(/*execute*/0);
	return NULL;
}

ATF_TC(disable_enable_async);
ATF_TC_HEAD(disable_enable_async, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test disabling and re-enabling cancellation when asynchronous");
}
ATF_TC_BODY(disable_enable_async, tc)
{
	int n = 0;
	pthread_t t;

	RZ(pthread_barrier_init(&bar, NULL, 2));

	RZ(pthread_create(&t, NULL, &disable_enable_async, &n));

	(void)pthread_barrier_wait(&bar);
	RZ(pthread_cancel(t));
	(void)pthread_barrier_wait(&bar);

	alarm(1);
	RZ(pthread_join(t, NULL));

	ATF_CHECK_MSG(n == 7, "n=%d", n);
	ATF_CHECK(cleanup_done);
}

static void *
disable_enable_setcanceltype_async(void *cookie)
{
	int *n = cookie;

	pthread_cleanup_push(&cleanup, &cleanup_done);

	*n = 1;
	pthread_testcancel();
	*n = 2;
	RZ(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));
	*n = 3;
	(void)pthread_barrier_wait(&bar);
	*n = 4;
	pthread_testcancel();
	*n = 5;
	(void)pthread_barrier_wait(&bar);
	*n = 6;
	pthread_testcancel();
	*n = 7;
	RZ(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL));
	*n = 8;
	pthread_testcancel();
	*n = 9;
	RZ(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)); /* cancel */
	*n = 10;
	pthread_testcancel();
	*n = 11;

	pthread_cleanup_pop(/*execute*/0);
	return NULL;
}

ATF_TC(disable_enable_setcanceltype_async);
ATF_TC_HEAD(disable_enable_setcanceltype_async, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test disabling cancellation, setting it async, and re-enabling");
}
ATF_TC_BODY(disable_enable_setcanceltype_async, tc)
{
	int n = 0;
	pthread_t t;

	RZ(pthread_barrier_init(&bar, NULL, 2));

	RZ(pthread_create(&t, NULL, &disable_enable_setcanceltype_async, &n));

	(void)pthread_barrier_wait(&bar);
	RZ(pthread_cancel(t));
	(void)pthread_barrier_wait(&bar);

	alarm(1);
	RZ(pthread_join(t, NULL));

	ATF_CHECK_MSG(n == 9, "n=%d", n);
	ATF_CHECK(cleanup_done);
}

static void *
setcanceltype_async(void *cookie)
{
	int *n = cookie;

	pthread_cleanup_push(&cleanup, &cleanup_done);

	*n = 1;
	pthread_testcancel();
	*n = 2;
	(void)pthread_barrier_wait(&bar);
	*n = 3;
	(void)pthread_barrier_wait(&bar);
	*n = 4;
	RZ(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,
		NULL)); /* cancel */
	*n = 5;

	pthread_cleanup_pop(/*execute*/0);
	return NULL;
}

ATF_TC(setcanceltype_async);
ATF_TC_HEAD(setcanceltype_async, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test disabling cancellation, setting it async, and re-enabling");
}
ATF_TC_BODY(setcanceltype_async, tc)
{
	int n = 0;
	pthread_t t;

	RZ(pthread_barrier_init(&bar, NULL, 2));

	RZ(pthread_create(&t, NULL, &setcanceltype_async, &n));

	(void)pthread_barrier_wait(&bar);
	RZ(pthread_cancel(t));
	(void)pthread_barrier_wait(&bar);

	alarm(1);
	RZ(pthread_join(t, NULL));

	ATF_CHECK_MSG(n == 4, "n=%d", n);
	ATF_CHECK(cleanup_done);
}

static void
sighandler(int signo)
{
	int state;

	RZ(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state));
	RZ(pthread_setcancelstate(state, NULL));
}

static void *
sigsafecancelstate(void *cookie)
{
	atomic_ulong *n = cookie;
	char name[128];

	pthread_cleanup_push(&cleanup, &cleanup_done);
	REQUIRE_LIBC(signal(SIGUSR1, &sighandler), SIG_ERR);

	(void)pthread_barrier_wait(&bar);

	while (atomic_load_explicit(n, memory_order_relaxed) != 0) {
		/*
		 * Do some things that might take the same lock as
		 * pthread_setcancelstate.
		 */
		RZ(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL));
		RZ(pthread_getname_np(pthread_self(), name, sizeof(name)));
		RZ(pthread_setname_np(pthread_self(), "%s", name));
	}

	pthread_cleanup_pop(/*execute*/1);
	return NULL;
}

ATF_TC(sigsafecancelstate);
ATF_TC_HEAD(sigsafecancelstate, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test pthread_setcancelstate async-signal-safety");
}
ATF_TC_BODY(sigsafecancelstate, tc)
{
	pthread_t t;
	atomic_ulong n = 10000;
	void *result;

	RZ(pthread_barrier_init(&bar, NULL, 2));
	RZ(pthread_create(&t, NULL, &sigsafecancelstate, &n));

	(void)pthread_barrier_wait(&bar);

	while (atomic_load_explicit(&n, memory_order_relaxed)) {
		pthread_kill(t, SIGUSR1);
		atomic_store_explicit(&n,
		    atomic_load_explicit(&n, memory_order_relaxed) - 1,
		    memory_order_relaxed);
	}

	alarm(1);
	RZ(pthread_join(t, &result));
	ATF_CHECK_MSG(result == NULL, "result=%p", result);
	ATF_CHECK(cleanup_done);
}

static void *
testcancel_loop(void *cookie)
{

	pthread_cleanup_push(&cleanup, &cleanup_done);
	(void)pthread_barrier_wait(&bar);
	for (;;)
		pthread_testcancel();
	pthread_cleanup_pop(/*execute*/0);

	return NULL;
}

ATF_TC(testcancel_loop);
ATF_TC_HEAD(testcancel_loop, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test pthread_testcancel in a loop");
}
ATF_TC_BODY(testcancel_loop, tc)
{
	pthread_t t;
	void *result;

	RZ(pthread_barrier_init(&bar, NULL, 2));
	RZ(pthread_create(&t, NULL, &testcancel_loop, NULL));

	(void)pthread_barrier_wait(&bar);
	RZ(pthread_cancel(t));

	alarm(1);
	RZ(pthread_join(t, &result));
	ATF_CHECK_MSG(result == PTHREAD_CANCELED,
	    "result=%p PTHREAD_CANCELED=%p", result, PTHREAD_CANCELED);
	ATF_CHECK(cleanup_done);
}

ATF_TP_ADD_TCS(tp)
{

	ADD_TEST_CANCELPOINT(cancelpoint_accept);
	ADD_TEST_CANCELPOINT(cancelpoint_accept4);
	ADD_TEST_CANCELPOINT(cancelpoint_aio_suspend);
	ADD_TEST_CANCELPOINT(cancelpoint_clock_nanosleep);
	ADD_TEST_CANCELPOINT(cancelpoint_close);
	ADD_TEST_CANCELPOINT(cancelpoint_cnd_timedwait);
	ADD_TEST_CANCELPOINT(cancelpoint_cnd_wait);
	ADD_TEST_CANCELPOINT(cancelpoint_connect);
	ADD_TEST_CANCELPOINT(cancelpoint_creat);
	ADD_TEST_CANCELPOINT(cancelpoint_fcntl_F_SETLKW);
	ADD_TEST_CANCELPOINT(cancelpoint_fcntl_F_OFD_SETLKW);
	ADD_TEST_CANCELPOINT(cancelpoint_fdatasync);
	ADD_TEST_CANCELPOINT(cancelpoint_fsync);
	ADD_TEST_CANCELPOINT(cancelpoint_lockf_F_LOCK);
	ADD_TEST_CANCELPOINT(cancelpoint_mq_receive);
	ADD_TEST_CANCELPOINT(cancelpoint_mq_send);
	ADD_TEST_CANCELPOINT(cancelpoint_mq_timedreceive);
	ADD_TEST_CANCELPOINT(cancelpoint_mq_timedsend);
	ADD_TEST_CANCELPOINT(cancelpoint_msgrcv);
	ADD_TEST_CANCELPOINT(cancelpoint_msgsnd);
	ADD_TEST_CANCELPOINT(cancelpoint_msync);
	ADD_TEST_CANCELPOINT(cancelpoint_nanosleep);
	ADD_TEST_CANCELPOINT(cancelpoint_open);
	ADD_TEST_CANCELPOINT(cancelpoint_openat);
	ADD_TEST_CANCELPOINT(cancelpoint_pause);
	ADD_TEST_CANCELPOINT(cancelpoint_poll);
	ADD_TEST_CANCELPOINT(cancelpoint_posix_close);
	ADD_TEST_CANCELPOINT(cancelpoint_ppoll);
	ADD_TEST_CANCELPOINT(cancelpoint_pread);
	ADD_TEST_CANCELPOINT(cancelpoint_pselect);
	ADD_TEST_CANCELPOINT(cancelpoint_pthread_cond_clockwait);
	ADD_TEST_CANCELPOINT(cancelpoint_pthread_cond_timedwait);
	ADD_TEST_CANCELPOINT(cancelpoint_pthread_cond_wait);
	ADD_TEST_CANCELPOINT(cancelpoint_pthread_join);
	ADD_TEST_CANCELPOINT(cancelpoint_pthread_testcancel);
	ADD_TEST_CANCELPOINT(cancelpoint_pwrite);
	ADD_TEST_CANCELPOINT(cancelpoint_read);
	ADD_TEST_CANCELPOINT(cancelpoint_readv);
	ADD_TEST_CANCELPOINT(cancelpoint_recv);
	ADD_TEST_CANCELPOINT(cancelpoint_recvfrom);
	ADD_TEST_CANCELPOINT(cancelpoint_recvmsg);
	ADD_TEST_CANCELPOINT(cancelpoint_select);
	ADD_TEST_CANCELPOINT(cancelpoint_send);
	ADD_TEST_CANCELPOINT(cancelpoint_sendto);
	ADD_TEST_CANCELPOINT(cancelpoint_sendmsg);
	ADD_TEST_CANCELPOINT(cancelpoint_sigsuspend);
	ADD_TEST_CANCELPOINT(cancelpoint_sigtimedwait);
	ADD_TEST_CANCELPOINT(cancelpoint_sigwait);
	ADD_TEST_CANCELPOINT(cancelpoint_sigwaitinfo);
	ADD_TEST_CANCELPOINT(cancelpoint_sleep);
	ADD_TEST_CANCELPOINT(cancelpoint_tcdrain);
	ADD_TEST_CANCELPOINT(cancelpoint_thrd_join);
	ADD_TEST_CANCELPOINT(cancelpoint_thrd_sleep);
	ADD_TEST_CANCELPOINT(cancelpoint_wait);
	ADD_TEST_CANCELPOINT(cancelpoint_waitid);
	ADD_TEST_CANCELPOINT(cancelpoint_waitpid);
	ADD_TEST_CANCELPOINT(cancelpoint_write);
	ADD_TEST_CANCELPOINT(cancelpoint_writev);

	ATF_TP_ADD_TC(tp, cleanuppop0);
	ATF_TP_ADD_TC(tp, cleanuppop1);
	ATF_TP_ADD_TC(tp, cancelself_async);
	ATF_TP_ADD_TC(tp, cancelself_deferred);
	ATF_TP_ADD_TC(tp, defaults);
	ATF_TP_ADD_TC(tp, disable_enable);
	ATF_TP_ADD_TC(tp, disable_enable_async);
	ATF_TP_ADD_TC(tp, disable_enable_setcanceltype_async);
	ATF_TP_ADD_TC(tp, setcanceltype_async);
	ATF_TP_ADD_TC(tp, notestcancel_loop_async);
	ATF_TP_ADD_TC(tp, sigsafecancelstate);
	ATF_TP_ADD_TC(tp, testcancel_loop);

	return atf_no_error();
}
