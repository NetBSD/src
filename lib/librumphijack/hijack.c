/*      $NetBSD: hijack.c,v 1.4 2011/01/08 21:30:24 pooka Exp $	*/

/*-
 * Copyright (c) 2011 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: hijack.c,v 1.4 2011/01/08 21:30:24 pooka Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/poll.h>

#include <rump/rump.h>
#include <rump/rumpclient.h>
#include <rump/rump_syscalls.h>

#include <assert.h>
#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

enum {	RUMPCALL_SOCKET, RUMPCALL_ACCEPT, RUMPCALL_BIND, RUMPCALL_CONNECT,
	RUMPCALL_GETPEERNAME, RUMPCALL_GETSOCKNAME, RUMPCALL_LISTEN,
	RUMPCALL_RECVFROM, RUMPCALL_RECVMSG,
	RUMPCALL_SENDTO, RUMPCALL_SENDMSG,
	RUMPCALL_GETSOCKOPT, RUMPCALL_SETSOCKOPT,
	RUMPCALL_SHUTDOWN,
	RUMPCALL_READ, RUMPCALL_READV,
	RUMPCALL_WRITE, RUMPCALL_WRITEV,
	RUMPCALL_IOCTL, RUMPCALL_FCNTL,
	RUMPCALL_CLOSE,
	RUMPCALL_POLLTS,
	RUMPCALL__NUM
};

const char *sysnames[] = {
	"__socket30",
	"accept",
	"bind",
	"connect",
	"getpeername",
	"getsockname",
	"listen",
	"recvfrom",
	"recvmsg",
	"sendto",
	"sendmsg",
	"getsockopt",
	"setsockopt",
	"shutdown",
	"read",
	"readv",
	"write",
	"writev",
	"ioctl",
	"fcntl",
	"close",
	"__pollts50",
};

static ssize_t	(*host_read)(int, void *, size_t);
static ssize_t	(*host_readv)(int, const struct iovec *, int);
static ssize_t	(*host_write)(int, const void *, size_t);
static ssize_t	(*host_writev)(int, const struct iovec *, int);
static int	(*host_ioctl)(int, unsigned long, ...);
static int	(*host_fcntl)(int, int, ...);
static int	(*host_close)(int);
static int	(*host_pollts)(struct pollfd *, nfds_t,
			       const struct timespec *, const sigset_t *);
static pid_t	(*host_fork)(void);
static int	(*host_dup2)(int, int);

static void *rumpcalls[RUMPCALL__NUM];

/*
 * This is called from librumpclient in case of LD_PRELOAD.
 * It ensures correct RTLD_NEXT.
 */
static void *
hijackdlsym(void *handle, const char *symbol)
{

	return dlsym(handle, symbol);
}

static void __attribute__((constructor))
rcinit(void)
{
	int (*rumpcinit)(void);
	void **rumpcdlsym;
	void *hand;
	int i;

	hand = dlopen("librumpclient.so", RTLD_LAZY|RTLD_GLOBAL);
	if (!hand)
		err(1, "cannot open librumpclient.so");
	rumpcinit = dlsym(hand, "rumpclient_init");
	_DIAGASSERT(rumpcinit);

	rumpcdlsym = dlsym(hand, "rumpclient_dlsym");
	*rumpcdlsym = hijackdlsym;

	host_read = dlsym(RTLD_NEXT, "read");
	host_readv = dlsym(RTLD_NEXT, "readv");
	host_write = dlsym(RTLD_NEXT, "write");
	host_writev = dlsym(RTLD_NEXT, "writev");
	host_ioctl = dlsym(RTLD_NEXT, "ioctl");
	host_fcntl = dlsym(RTLD_NEXT, "fcntl");
	host_close = dlsym(RTLD_NEXT, "close");
	host_pollts = dlsym(RTLD_NEXT, "pollts");
	host_fork = dlsym(RTLD_NEXT, "fork");
	host_dup2 = dlsym(RTLD_NEXT, "dup2");

	for (i = 0; i < RUMPCALL__NUM; i++) {
		char sysname[128];

		snprintf(sysname, sizeof(sysname), "rump_sys_%s", sysnames[i]);
		rumpcalls[i] = dlsym(hand, sysname);
		if (!rumpcalls[i]) {
			fprintf(stderr, "cannot find symbol: %s\n", sysname);
			exit(1);
		}
	}

	if (rumpcinit() == -1)
		err(1, "rumpclient init");
}

//#define DEBUGJACK
#ifdef DEBUGJACK
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

static unsigned dup2mask;
#define ISDUP2D(fd) (((fd+1) & dup2mask) == ((fd)+1))

/* XXX: need runtime selection.  low for now due to FD_SETSIZE */
#define HIJACK_FDOFF 128
#define HIJACK_SELECT 128 /* XXX */
#define HIJACK_ASSERT 128 /* XXX */
static int
fd_rump2host(int fd)
{

	if (fd == -1)
		return fd;

	if (!ISDUP2D(fd))
		fd += HIJACK_FDOFF;

	return fd;
}

static int
fd_host2rump(int fd)
{

	if (!ISDUP2D(fd))
		fd -= HIJACK_FDOFF;
	return fd;
}

static bool
fd_isrump(int fd)
{

	return ISDUP2D(fd) || fd >= HIJACK_FDOFF;
}

#define assertfd(_fd_) assert(ISDUP2D(_fd_) || (_fd_) >= HIJACK_ASSERT)
#undef HIJACK_FDOFF

/*
 * Following wrappers always call the rump kernel.
 */

int __socket30(int, int, int);
int
__socket30(int domain, int type, int protocol)
{
	int (*rc_socket)(int, int, int);
	int fd;

	rc_socket = rumpcalls[RUMPCALL_SOCKET];
	fd = rc_socket(domain, type, protocol);

	DPRINTF(("socket <- %d\n", fd_rump2host(fd)));

	return fd_rump2host(fd);
}

int
accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	int (*rc_accept)(int, struct sockaddr *, socklen_t *);
	int fd;

	DPRINTF(("accept -> %d", s));
	assertfd(s);
	rc_accept = rumpcalls[RUMPCALL_ACCEPT];
	fd = rc_accept(fd_host2rump(s), addr, addrlen);
	DPRINTF((" <- %d\n", fd_rump2host(fd)));

	return fd_rump2host(fd);
}

int
bind(int s, const struct sockaddr *name, socklen_t namelen)
{
	int (*rc_bind)(int, const struct sockaddr *, socklen_t);

	DPRINTF(("bind -> %d\n", s));
	assertfd(s);
	rc_bind = rumpcalls[RUMPCALL_BIND];

	return rc_bind(fd_host2rump(s), name, namelen);
}

int
connect(int s, const struct sockaddr *name, socklen_t namelen)
{
	int (*rc_connect)(int, const struct sockaddr *, socklen_t);

	DPRINTF(("connect -> %d\n", s));
	assertfd(s);
	rc_connect = rumpcalls[RUMPCALL_CONNECT];

	return rc_connect(fd_host2rump(s), name, namelen);
}

int
getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
	int (*rc_getpeername)(int, struct sockaddr *, socklen_t *);

	DPRINTF(("getpeername -> %d\n", s));
	assertfd(s);
	rc_getpeername = rumpcalls[RUMPCALL_GETPEERNAME];
	return rc_getpeername(fd_host2rump(s), name, namelen);
}

int
getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
	int (*rc_getsockname)(int, struct sockaddr *, socklen_t *);

	DPRINTF(("getsockname -> %d\n", s));
	assertfd(s);
	rc_getsockname = rumpcalls[RUMPCALL_GETSOCKNAME];
	return rc_getsockname(fd_host2rump(s), name, namelen);
}

int
listen(int s, int backlog)
{
	int (*rc_listen)(int, int);

	DPRINTF(("listen -> %d\n", s));
	assertfd(s);
	rc_listen = rumpcalls[RUMPCALL_LISTEN];
	return rc_listen(fd_host2rump(s), backlog);
}

ssize_t
recv(int s, void *buf, size_t len, int flags)
{

	return recvfrom(s, buf, len, flags, NULL, NULL);
}

ssize_t
recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from,
	socklen_t *fromlen)
{
	int (*rc_recvfrom)(int, void *, size_t, int,
	    struct sockaddr *, socklen_t *);

	DPRINTF(("recvfrom\n"));
	assertfd(s);
	rc_recvfrom = rumpcalls[RUMPCALL_RECVFROM];
	return rc_recvfrom(fd_host2rump(s), buf, len, flags, from, fromlen);
}

ssize_t
recvmsg(int s, struct msghdr *msg, int flags)
{
	int (*rc_recvmsg)(int, struct msghdr *, int);

	DPRINTF(("recvmsg\n"));
	assertfd(s);
	rc_recvmsg = rumpcalls[RUMPCALL_RECVMSG];
	return rc_recvmsg(fd_host2rump(s), msg, flags);
}

ssize_t
send(int s, const void *buf, size_t len, int flags)
{

	return sendto(s, buf, len, flags, NULL, 0);
}

ssize_t
sendto(int s, const void *buf, size_t len, int flags,
	const struct sockaddr *to, socklen_t tolen)
{
	int (*rc_sendto)(int, const void *, size_t, int,
	    const struct sockaddr *, socklen_t);

	if (s == -1)
		return len;

	DPRINTF(("sendto\n"));
	assertfd(s);
	rc_sendto = rumpcalls[RUMPCALL_SENDTO];
	return rc_sendto(fd_host2rump(s), buf, len, flags, to, tolen);
}

ssize_t
sendmsg(int s, const struct msghdr *msg, int flags)
{
	int (*rc_sendmsg)(int, const struct msghdr *, int);

	DPRINTF(("sendmsg\n"));
	assertfd(s);
	rc_sendmsg = rumpcalls[RUMPCALL_SENDTO];
	return rc_sendmsg(fd_host2rump(s), msg, flags);
}

int
getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
	int (*rc_getsockopt)(int, int, int, void *, socklen_t *);

	DPRINTF(("getsockopt\n"));
	assertfd(s);
	rc_getsockopt = rumpcalls[RUMPCALL_GETSOCKOPT];
	return rc_getsockopt(fd_host2rump(s), level, optname, optval, optlen);
}

int
setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
	int (*rc_setsockopt)(int, int, int, const void *, socklen_t);

	DPRINTF(("setsockopt\n"));
	assertfd(s);
	rc_setsockopt = rumpcalls[RUMPCALL_SETSOCKOPT];
	return rc_setsockopt(fd_host2rump(s), level, optname, optval, optlen);
}

int
shutdown(int s, int how)
{
	int (*rc_shutdown)(int, int);

	DPRINTF(("shutdown\n"));
	assertfd(s);
	rc_shutdown = rumpcalls[RUMPCALL_SHUTDOWN];
	return rc_shutdown(fd_host2rump(s), how);
}

/*
 * dup2 is special.  we allow dup2 of a rump kernel fd to 0-2 since
 * many programs do that.  dup2 of a rump kernel fd to another value
 * not >= fdoff is an error.
 *
 * Note: cannot rump2host newd, because it is often hardcoded.
 *
 * XXX: should disable debug prints after stdout/stderr are dup2'd
 */
int
dup2(int oldd, int newd)
{
	int rv;

	DPRINTF(("dup2 -> %d (o) -> %d (n)\n", oldd, newd));

	if (fd_isrump(oldd)) {
		if (!(newd >= 0 && newd <= 2))
			return EBADF;
		oldd = fd_host2rump(oldd);
		rv = rump_sys_dup2(oldd, newd);
		if (rv != -1)
			dup2mask |= newd+1;
		return rv;
	} else {
		return host_dup2(oldd, newd);
	}
}

/*
 * We just wrap fork the appropriate rump client calls to preserve
 * the file descriptors of the forked parent in the child, but
 * prevent double use of connection fd.
 */

pid_t
fork()
{
	struct rumpclient_fork *rf;
	pid_t rv;

	DPRINTF(("fork\n"));

	if ((rf = rumpclient_prefork()) == NULL)
		return -1;

	switch ((rv = host_fork())) {
	case -1:
		/* XXX: cancel rf */
		break;
	case 0:
		if (rumpclient_fork_init(rf) == -1)
			rv = -1;
		break;
	default:
		break;
	}

	DPRINTF(("fork returns %d\n", rv));
	return rv;
}

/*
 * Hybrids
 */

ssize_t
read(int fd, void *buf, size_t len)
{
	int (*op_read)(int, void *, size_t);
	ssize_t n;

	DPRINTF(("read %d\n", fd));
	if (fd_isrump(fd)) {
		fd = fd_host2rump(fd);
		op_read = rumpcalls[RUMPCALL_READ];
	} else {
		op_read = host_read;
	}

	n = op_read(fd, buf, len);
	return n;
}

ssize_t
readv(int fd, const struct iovec *iov, int iovcnt)
{
	int (*op_readv)(int, const struct iovec *, int);

	if (fd_isrump(fd)) {
		fd = fd_host2rump(fd);
		op_readv = rumpcalls[RUMPCALL_READV];
	} else {
		op_readv = host_readv;
	}

	DPRINTF(("readv\n"));
	return op_readv(fd, iov, iovcnt);
}

ssize_t
write(int fd, const void *buf, size_t len)
{
	int (*op_write)(int, const void *, size_t);

	if (fd_isrump(fd)) {
		fd = fd_host2rump(fd);
		op_write = rumpcalls[RUMPCALL_WRITE];
	} else {
		op_write = host_write;
	}

	return op_write(fd, buf, len);
}

ssize_t
writev(int fd, const struct iovec *iov, int iovcnt)
{
	int (*op_writev)(int, const struct iovec *, int);

	if (fd_isrump(fd)) {
		fd = fd_host2rump(fd);
		op_writev = rumpcalls[RUMPCALL_WRITEV];
	} else {
		op_writev = host_writev;
	}

	return op_writev(fd, iov, iovcnt);
}

int
ioctl(int fd, unsigned long cmd, ...)
{
	int (*op_ioctl)(int, unsigned long cmd, ...);
	va_list ap;
	int rv;

	DPRINTF(("ioctl\n"));
	if (fd_isrump(fd)) {
		fd = fd_host2rump(fd);
		op_ioctl = rumpcalls[RUMPCALL_IOCTL];
	} else {
		op_ioctl = host_ioctl;
	}

	va_start(ap, cmd);
	rv = op_ioctl(fd, cmd, va_arg(ap, void *));
	va_end(ap);
	return rv;
}

int
fcntl(int fd, int cmd, ...)
{
	int (*op_fcntl)(int, int, ...);
	va_list ap;
	int rv;

	DPRINTF(("fcntl\n"));
	if (fd_isrump(fd)) {
		fd = fd_host2rump(fd);
		op_fcntl = rumpcalls[RUMPCALL_FCNTL];
	} else {
		op_fcntl = host_fcntl;
	}

	va_start(ap, cmd);
	rv = op_fcntl(fd, cmd, va_arg(ap, void *));
	va_end(ap);
	return rv;
}

int
close(int fd)
{
	int (*op_close)(int);

	DPRINTF(("close %d\n", fd));
	if (fd_isrump(fd)) {
		fd = fd_host2rump(fd);
		op_close = rumpcalls[RUMPCALL_CLOSE];
	} else {
		op_close = host_close;
	}

	return op_close(fd);
}

int
select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	struct timeval *timeout)
{
	struct pollfd *pfds;
	struct timespec ts, *tsp = NULL;
	nfds_t i, j, realnfds;
	int rv, incr;

	/*
	 * Well, first we must scan the fds to figure out how many
	 * fds there really are.  This is because up to and including
	 * nb5 poll() silently refuses nfds > process_open_fds.
	 * Seems to be fixed in current, thank the maker.
	 * god damn cluster...bomb.
	 */
	
	for (i = 0, realnfds = 0; i < nfds; i++) {
		if (readfds && FD_ISSET(i, readfds)) {
			realnfds++;
			continue;
		}
		if (writefds && FD_ISSET(i, writefds)) {
			realnfds++;
			continue;
		}
		if (exceptfds && FD_ISSET(i, exceptfds)) {
			realnfds++;
			continue;
		}
	}

	pfds = malloc(sizeof(*pfds) * realnfds);
	if (!pfds)
		return -1;

	for (i = 0, j = 0; i < nfds; i++) {
		incr = 0;
		pfds[j].events = pfds[j].revents = 0;
		if (readfds && FD_ISSET(i, readfds)) {
			pfds[j].fd = i;
			pfds[j].events |= POLLIN;
			incr=1;
		}
		if (writefds && FD_ISSET(i, writefds)) {
			pfds[j].fd = i;
			pfds[j].events |= POLLOUT;
			incr=1;
		}
		if (exceptfds && FD_ISSET(i, exceptfds)) {
			pfds[j].fd = i;
			pfds[j].events |= POLLHUP|POLLERR;
			incr=1;
		}
		if (incr)
			j++;
	}

	if (timeout) {
		TIMEVAL_TO_TIMESPEC(timeout, &ts);
		tsp = &ts;
	}
	rv = pollts(pfds, realnfds, tsp, NULL);
	if (rv <= 0)
		goto out;

	/*
	 * ok, harvest results.  first zero out entries (can't use
	 * FD_ZERO for the obvious select-me-not reason).  whee.
	 */
	for (i = 0; i < nfds; i++) {
		if (readfds)
			FD_CLR(i, readfds);
		if (writefds)
			FD_CLR(i, writefds);
		if (exceptfds)
			FD_CLR(i, exceptfds);
	}

	/* and then plug in the results */
	for (i = 0; i < realnfds; i++) {
		if (readfds) {
			if (pfds[i].revents & POLLIN) {
				FD_SET(pfds[i].fd, readfds);
			}
		}
		if (writefds) {
			if (pfds[i].revents & POLLOUT) {
				FD_SET(pfds[i].fd, writefds);
			}
		}
		if (exceptfds) {
			if (pfds[i].revents & (POLLHUP|POLLERR)) {
				FD_SET(pfds[i].fd, exceptfds);
			}
		}
	}

 out:
	free(pfds);
	return rv;
}

static void
checkpoll(struct pollfd *fds, nfds_t nfds, int *hostcall, int *rumpcall)
{
	nfds_t i;

	for (i = 0; i < nfds; i++) {
		if (fd_isrump(fds[i].fd))
			(*rumpcall)++;
		else
			(*hostcall)++;
	}
}

static void
adjustpoll(struct pollfd *fds, nfds_t nfds, int (*fdadj)(int))
{
	nfds_t i;

	for (i = 0; i < nfds; i++) {
		fds[i].fd = fdadj(fds[i].fd);
	}
}

struct mytimespec {
	uint64_t tv_sec;
	long tv_nsec;
};

/*
 * poll is easy as long as the call comes in the fds only in one
 * kernel.  otherwise its quite tricky...
 */
struct pollarg {
	struct pollfd *pfds;
	nfds_t nfds;
	const struct timespec *ts;
	const sigset_t *sigmask;
	int pipefd;
	int errnum;
};

static void *
hostpoll(void *arg)
{
	struct pollarg *parg = arg;
	intptr_t rv;

	rv = host_pollts(parg->pfds, parg->nfds, parg->ts, parg->sigmask);
	if (rv == -1)
		parg->errnum = errno;
	rump_sys_write(parg->pipefd, &rv, sizeof(rv));

	return (void *)(intptr_t)rv;
}

int
pollts(struct pollfd *fds, nfds_t nfds, const struct timespec *ts,
	const sigset_t *sigmask)
{
	int (*op_pollts)(struct pollfd *, nfds_t, const struct timespec *,
			 const sigset_t *);
	int hostcall = 0, rumpcall = 0;
	pthread_t pt;
	nfds_t i;
	int rv;

#if 0
	/* XXX: quick 5.0 kludge.  do syscall compat in rumpclient properly */
	struct mytimespec mts;
	if (ts) {
		mts.tv_sec = ts->tv_sec;
		mts.tv_nsec = ts->tv_nsec;
		ts = (struct timespec *)&mts;
	}
#endif

	DPRINTF(("poll\n"));
	checkpoll(fds, nfds, &hostcall, &rumpcall);

	if (hostcall && rumpcall) {
		struct pollfd *pfd_host = NULL, *pfd_rump = NULL;
		int rpipe[2] = {-1,-1}, hpipe[2] = {-1,-1};
		struct pollarg parg;
		uintptr_t lrv;
		int sverrno = 0, trv;

		/*
		 * ok, this is where it gets tricky.  We must support
		 * this since it's a very common operation in certain
		 * types of software (telnet, netcat, etc).  We allocate
		 * two vectors and run two poll commands in separate
		 * threads.  Whichever returns first "wins" and the
		 * other kernel's fds won't show activity.
		 */
		rv = -1;

		/* allocate full vector for O(n) joining after call */
		pfd_host = malloc(sizeof(*pfd_host)*(nfds+1));
		if (!pfd_host)
			goto out;
		pfd_rump = malloc(sizeof(*pfd_rump)*(nfds+1));
		if (!pfd_rump) {
			goto out;
		}

		/* split vectors */
		for (i = 0; i < nfds; i++) {
			if (fds[i].fd == -1) {
				pfd_host[i].fd = -1;
				pfd_rump[i].fd = -1;
			} else if (fd_isrump(fds[i].fd)) {
				pfd_host[i].fd = -1;
				pfd_rump[i].fd = fd_host2rump(fds[i].fd);
				pfd_rump[i].events = fds[i].events;
			} else {
				pfd_rump[i].fd = -1;
				pfd_host[i].fd = fds[i].fd;
				pfd_host[i].events = fds[i].events;
			}
		}

		/*
		 * then, open two pipes, one for notifications
		 * to each kernel.
		 */
		if (rump_sys_pipe(rpipe) == -1)
			goto out;
		if (pipe(hpipe) == -1)
			goto out;

		pfd_host[nfds].fd = hpipe[0];
		pfd_host[nfds].events = POLLIN;
		pfd_rump[nfds].fd = rpipe[0];
		pfd_rump[nfds].events = POLLIN;

		/*
		 * then, create a thread to do host part and meanwhile
		 * do rump kernel part right here
		 */

		parg.pfds = pfd_host;
		parg.nfds = nfds+1;
		parg.ts = ts;
		parg.sigmask = sigmask;
		parg.pipefd = rpipe[1];
		pthread_create(&pt, NULL, hostpoll, &parg);

		op_pollts = rumpcalls[RUMPCALL_POLLTS];
		lrv = op_pollts(pfd_rump, nfds+1, ts, NULL);
		sverrno = errno;
		write(hpipe[1], &rv, sizeof(rv));
		pthread_join(pt, (void *)&trv);

		/* check who "won" and merge results */
		if (lrv != 0 && pfd_host[nfds].revents & POLLIN) {
			rv = trv;

			for (i = 0; i < nfds; i++) {
				if (pfd_rump[i].fd != -1)
					fds[i].revents = pfd_rump[i].revents;
			}
			sverrno = parg.errnum;
		} else if (trv != 0 && pfd_rump[nfds].revents & POLLIN) {
			rv = trv;

			for (i = 0; i < nfds; i++) {
				if (pfd_host[i].fd != -1)
					fds[i].revents = pfd_host[i].revents;
			}
		} else {
			rv = 0;
		}

 out:
		if (rpipe[0] != -1)
			rump_sys_close(rpipe[0]);
		if (rpipe[1] != -1)
			rump_sys_close(rpipe[1]);
		if (hpipe[0] != -1)
			close(hpipe[0]);
		if (hpipe[1] != -1)
			close(hpipe[1]);
		free(pfd_host);
		free(pfd_rump);
		errno = sverrno;
	} else {
		if (hostcall) {
			op_pollts = host_pollts;
		} else {
			op_pollts = rumpcalls[RUMPCALL_POLLTS];
			adjustpoll(fds, nfds, fd_host2rump);
		}

		rv = op_pollts(fds, nfds, ts, sigmask);
		if (rumpcall)
			adjustpoll(fds, nfds, fd_rump2host);
	}

	return rv;
}

int
poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	struct timespec ts;
	struct timespec *tsp = NULL;

	if (timeout != INFTIM) {
		ts.tv_sec = timeout / 1000;
		ts.tv_nsec = (timeout % 1000) * 1000;

		tsp = &ts;
	}

	return pollts(fds, nfds, tsp, NULL);
}
