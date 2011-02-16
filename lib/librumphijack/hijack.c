/*      $NetBSD: hijack.c,v 1.43 2011/02/16 17:56:46 pooka Exp $	*/

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
__RCSID("$NetBSD: hijack.c,v 1.43 2011/02/16 17:56:46 pooka Exp $");

#define __ssp_weak_name(fun) _hijack_ ## fun

#include <sys/param.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/poll.h>

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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

enum dualcall {
	DUALCALL_WRITE, DUALCALL_WRITEV,
	DUALCALL_IOCTL, DUALCALL_FCNTL,
	DUALCALL_SOCKET, DUALCALL_ACCEPT, DUALCALL_BIND, DUALCALL_CONNECT,
	DUALCALL_GETPEERNAME, DUALCALL_GETSOCKNAME, DUALCALL_LISTEN,
	DUALCALL_RECVFROM, DUALCALL_RECVMSG,
	DUALCALL_SENDTO, DUALCALL_SENDMSG,
	DUALCALL_GETSOCKOPT, DUALCALL_SETSOCKOPT,
	DUALCALL_SHUTDOWN,
	DUALCALL_READ, DUALCALL_READV,
	DUALCALL_DUP2,
	DUALCALL_CLOSE,
	DUALCALL_POLLTS,
	DUALCALL_KEVENT,
	DUALCALL__NUM
};

#define RSYS_STRING(a) __STRING(a)
#define RSYS_NAME(a) RSYS_STRING(__CONCAT(RUMP_SYS_RENAME_,a))

/*
 * Would be nice to get this automatically in sync with libc.
 * Also, this does not work for compat-using binaries!
 */
#if !__NetBSD_Prereq__(5,99,7)
#define REALSELECT select
#define REALPOLLTS pollts
#define REALKEVENT kevent
#else
#define REALSELECT _sys___select50
#define REALPOLLTS _sys___pollts50
#define REALKEVENT _sys___kevent50
#endif
#define REALREAD _sys_read

int REALSELECT(int, fd_set *, fd_set *, fd_set *, struct timeval *);
int REALPOLLTS(struct pollfd *, nfds_t,
	       const struct timespec *, const sigset_t *);
int REALKEVENT(int, const struct kevent *, size_t, struct kevent *, size_t,
	       const struct timespec *);
ssize_t REALREAD(int, void *, size_t);

#define S(a) __STRING(a)
struct sysnames {
	enum dualcall scm_callnum;
	const char *scm_hostname;
	const char *scm_rumpname;
} syscnames[] = {
	{ DUALCALL_SOCKET,	"__socket30",	RSYS_NAME(SOCKET)	},
	{ DUALCALL_ACCEPT,	"accept",	RSYS_NAME(ACCEPT)	},
	{ DUALCALL_BIND,	"bind",		RSYS_NAME(BIND)		},
	{ DUALCALL_CONNECT,	"connect",	RSYS_NAME(CONNECT)	},
	{ DUALCALL_GETPEERNAME,	"getpeername",	RSYS_NAME(GETPEERNAME)	},
	{ DUALCALL_GETSOCKNAME,	"getsockname",	RSYS_NAME(GETSOCKNAME)	},
	{ DUALCALL_LISTEN,	"listen",	RSYS_NAME(LISTEN)	},
	{ DUALCALL_RECVFROM,	"recvfrom",	RSYS_NAME(RECVFROM)	},
	{ DUALCALL_RECVMSG,	"recvmsg",	RSYS_NAME(RECVMSG)	},
	{ DUALCALL_SENDTO,	"sendto",	RSYS_NAME(SENDTO)	},
	{ DUALCALL_SENDMSG,	"sendmsg",	RSYS_NAME(SENDMSG)	},
	{ DUALCALL_GETSOCKOPT,	"getsockopt",	RSYS_NAME(GETSOCKOPT)	},
	{ DUALCALL_SETSOCKOPT,	"setsockopt",	RSYS_NAME(SETSOCKOPT)	},
	{ DUALCALL_SHUTDOWN,	"shutdown",	RSYS_NAME(SHUTDOWN)	},
	{ DUALCALL_READ,	S(REALREAD),	RSYS_NAME(READ)		},
	{ DUALCALL_READV,	"readv",	RSYS_NAME(READV)	},
	{ DUALCALL_WRITE,	"write",	RSYS_NAME(WRITE)	},
	{ DUALCALL_WRITEV,	"writev",	RSYS_NAME(WRITEV)	},
	{ DUALCALL_IOCTL,	"ioctl",	RSYS_NAME(IOCTL)	},
	{ DUALCALL_FCNTL,	"fcntl",	RSYS_NAME(FCNTL)	},
	{ DUALCALL_DUP2,	"dup2",		RSYS_NAME(DUP2)		},
	{ DUALCALL_CLOSE,	"close",	RSYS_NAME(CLOSE)	},
	{ DUALCALL_POLLTS,	S(REALPOLLTS),	RSYS_NAME(POLLTS)	},
	{ DUALCALL_KEVENT,	S(REALKEVENT),	RSYS_NAME(KEVENT)	},
};
#undef S

struct bothsys {
	void *bs_host;
	void *bs_rump;
} syscalls[DUALCALL__NUM];
#define GETSYSCALL(which, name) syscalls[DUALCALL_##name].bs_##which

pid_t	(*host_fork)(void);
int	(*host_daemon)(int, int);
int	(*host_execve)(const char *, char *const[], char *const[]);

static uint32_t dup2mask;
#define ISDUP2D(fd) (((fd) < 32) && (1<<(fd) & dup2mask))
#define SETDUP2(fd) \
    do { if ((fd) < 32) dup2mask |= (1<<(fd)); } while (/*CONSTCOND*/0)
#define CLRDUP2(fd) \
    do { if ((fd) < 32) dup2mask &= ~(1<<(fd)); } while (/*CONSTCOND*/0)

//#define DEBUGJACK
#ifdef DEBUGJACK
#define DPRINTF(x) mydprintf x
static void
mydprintf(const char *fmt, ...)
{
	va_list ap;

	if (ISDUP2D(STDERR_FILENO))
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

#else
#define DPRINTF(x)
#endif

#define FDCALL(type, name, rcname, args, proto, vars)			\
type name args								\
{									\
	type (*fun) proto;						\
									\
	DPRINTF(("%s -> %d\n", __STRING(name), fd));			\
	if (fd_isrump(fd)) {						\
		fun = syscalls[rcname].bs_rump;				\
		fd = fd_host2rump(fd);					\
	} else {							\
		fun = syscalls[rcname].bs_host;				\
	}								\
									\
	return fun vars;						\
}

/*
 * This is called from librumpclient in case of LD_PRELOAD.
 * It ensures correct RTLD_NEXT.
 *
 * ... except, it's apparently extremely difficult to force
 * at least gcc to generate an actual stack frame here.  So
 * sprinkle some volatile foobar and baz to throw the optimizer
 * off the scent and generate a variable assignment with the
 * return value.  The posterboy for this meltdown is amd64
 * with -O2.  At least with gcc 4.1.3 i386 works regardless of
 * optimization.
 */
volatile int rumphijack_unrope; /* there, unhang yourself */
static void *
hijackdlsym(void *handle, const char *symbol)
{
	void *rv;

	rv = dlsym(handle, symbol);
	rumphijack_unrope = *(volatile int *)rv;

	return (void *)rv;
}

/* low calorie sockets? */
static bool hostlocalsockets = true;

static void __attribute__((constructor))
rcinit(void)
{
	char buf[64];
	extern void *(*rumpclient_dlsym)(void *, const char *);
	unsigned i, j;

	rumpclient_dlsym = hijackdlsym;
	host_fork = dlsym(RTLD_NEXT, "fork");
	host_daemon = dlsym(RTLD_NEXT, "daemon");
	host_execve = dlsym(RTLD_NEXT, "execve");

	/*
	 * In theory cannot print anything during lookups because
	 * we might not have the call vector set up.  so, the errx()
	 * is a bit of a strech, but it might work.
	 */

	for (i = 0; i < DUALCALL__NUM; i++) {
		/* build runtime O(1) access */
		for (j = 0; j < __arraycount(syscnames); j++) {
			if (syscnames[j].scm_callnum == i)
				break;
		}

		if (j == __arraycount(syscnames))
			errx(1, "rumphijack error: syscall pos %d missing", i);

		syscalls[i].bs_host = dlsym(RTLD_NEXT,
		    syscnames[j].scm_hostname);
		if (syscalls[i].bs_host == NULL)
			errx(1, "hostcall %s not found missing",
			    syscnames[j].scm_hostname);

		syscalls[i].bs_rump = dlsym(RTLD_NEXT,
		    syscnames[j].scm_rumpname);
		if (syscalls[i].bs_rump == NULL)
			errx(1, "rumpcall %s not found missing",
			    syscnames[j].scm_rumpname);
	}

	if (rumpclient_init() == -1)
		err(1, "rumpclient init");

	/* set client persistence level */
	if (getenv_r("RUMPHIJACK_RETRY", buf, sizeof(buf)) == -1) {
		if (errno == ERANGE)
			err(1, "invalid RUMPHIJACK_RETRY");
		rumpclient_setconnretry(RUMPCLIENT_RETRYCONN_INFTIME);
	} else {
		if (strcmp(buf, "die") == 0)
			rumpclient_setconnretry(RUMPCLIENT_RETRYCONN_DIE);
		else if (strcmp(buf, "inftime") == 0)
			rumpclient_setconnretry(RUMPCLIENT_RETRYCONN_INFTIME);
		else if (strcmp(buf, "once") == 0)
			rumpclient_setconnretry(RUMPCLIENT_RETRYCONN_ONCE);
		else {
			time_t timeout;

			timeout = (time_t)strtoll(buf, NULL, 10);
			if (timeout <= 0)
				errx(1, "RUMPHIJACK_RETRY must be keyword "
				    "or a positive integer, got: %s", buf);

			rumpclient_setconnretry(timeout);
		}
	}

	if (getenv_r("RUMPHIJACK__DUP2MASK", buf, sizeof(buf)) == 0) {
		dup2mask = strtoul(buf, NULL, 10);
	}
}

/* XXX: need runtime selection.  low for now due to FD_SETSIZE */
#define HIJACK_FDOFF 128
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

#define assertfd(_fd_) assert(ISDUP2D(_fd_) || (_fd_) >= HIJACK_FDOFF)

static int
dodup(int oldd, int minfd)
{
	int (*op_fcntl)(int, int, ...);
	int newd;
	int isrump;

	DPRINTF(("dup -> %d (minfd %d)\n", oldd, minfd));
	if (fd_isrump(oldd)) {
		op_fcntl = GETSYSCALL(rump, FCNTL);
		oldd = fd_host2rump(oldd);
		isrump = 1;
	} else {
		op_fcntl = GETSYSCALL(host, FCNTL);
		isrump = 0;
	}

	newd = op_fcntl(oldd, F_DUPFD, minfd);

	if (isrump)
		newd = fd_rump2host(newd);
	DPRINTF(("dup <- %d\n", newd));

	return newd;
}

int __socket30(int, int, int);
int
__socket30(int domain, int type, int protocol)
{
	int (*op_socket)(int, int, int);
	int fd;
	bool dohost;

	dohost = hostlocalsockets && (domain == AF_LOCAL);

	if (dohost)
		op_socket = GETSYSCALL(host, SOCKET);
	else
		op_socket = GETSYSCALL(rump, SOCKET);
	fd = op_socket(domain, type, protocol);

	if (!dohost)
		fd = fd_rump2host(fd);
	DPRINTF(("socket <- %d\n", fd));

	return fd;
}

int
accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	int (*op_accept)(int, struct sockaddr *, socklen_t *);
	int fd;
	bool isrump;

	isrump = fd_isrump(s);

	DPRINTF(("accept -> %d", s));
	if (isrump) {
		op_accept = GETSYSCALL(rump, ACCEPT);
		s = fd_host2rump(s);
	} else {
		op_accept = GETSYSCALL(host, ACCEPT);
	}
	fd = op_accept(s, addr, addrlen);
	if (fd != -1 && isrump)
		fd = fd_rump2host(fd);

	DPRINTF((" <- %d\n", fd));

	return fd;
}

/*
 * ioctl and fcntl are varargs calls and need special treatment
 */
int
ioctl(int fd, unsigned long cmd, ...)
{
	int (*op_ioctl)(int, unsigned long cmd, ...);
	va_list ap;
	int rv;

	DPRINTF(("ioctl -> %d\n", fd));
	if (fd_isrump(fd)) {
		fd = fd_host2rump(fd);
		op_ioctl = GETSYSCALL(rump, IOCTL);
	} else {
		op_ioctl = GETSYSCALL(host, IOCTL);
	}

	va_start(ap, cmd);
	rv = op_ioctl(fd, cmd, va_arg(ap, void *));
	va_end(ap);
	return rv;
}

#include <syslog.h>
int
fcntl(int fd, int cmd, ...)
{
	int (*op_fcntl)(int, int, ...);
	va_list ap;
	int rv, minfd, i;

	DPRINTF(("fcntl -> %d (cmd %d)\n", fd, cmd));

	switch (cmd) {
	case F_DUPFD:
		va_start(ap, cmd);
		minfd = va_arg(ap, int);
		va_end(ap);
		return dodup(fd, minfd);

	case F_CLOSEM:
		/*
		 * So, if fd < HIJACKOFF, we want to do a host closem.
		 */

		if (fd < HIJACK_FDOFF) {
			int closemfd = fd;

			if (rumpclient__closenotify(&closemfd,
			    RUMPCLIENT_CLOSE_FCLOSEM) == -1)
				return -1;
			op_fcntl = GETSYSCALL(host, FCNTL);
			rv = op_fcntl(closemfd, cmd);
			if (rv)
				return rv;
		}

		/*
		 * Additionally, we want to do a rump closem, but only
		 * for the file descriptors not within the dup2mask.
		 */

		/* why don't we offer fls()? */
		for (i = 31; i >= 0; i--) {
			if (dup2mask & 1<<i)
				break;
		}
		
		if (fd >= HIJACK_FDOFF)
			fd -= HIJACK_FDOFF;
		else
			fd = 0;
		fd = MAX(i+1, fd);

		/* hmm, maybe we should close rump fd's not within dup2mask? */

		return rump_sys_fcntl(fd, F_CLOSEM);

	case F_MAXFD:
		/*
		 * For maxfd, if there's a rump kernel fd, return
		 * it hostified.  Otherwise, return host's MAXFD
		 * return value.
		 */
		if ((rv = rump_sys_fcntl(fd, F_MAXFD)) != -1) {
			/*
			 * This might go a little wrong in case
			 * of dup2 to [012], but I'm not sure if
			 * there's a justification for tracking
			 * that info.  Consider e.g.
			 * dup2(rumpfd, 2) followed by rump_sys_open()
			 * returning 1.  We should return 1+HIJACKOFF,
			 * not 2+HIJACKOFF.  However, if [01] is not
			 * open, the correct return value is 2.
			 */
			return fd_rump2host(fd);
		} else {
			op_fcntl = GETSYSCALL(host, FCNTL);
			return op_fcntl(fd, F_MAXFD);
		}
		/*NOTREACHED*/

	default:
		if (fd_isrump(fd)) {
			fd = fd_host2rump(fd);
			op_fcntl = GETSYSCALL(rump, FCNTL);
		} else {
			op_fcntl = GETSYSCALL(host, FCNTL);
		}

		va_start(ap, cmd);
		rv = op_fcntl(fd, cmd, va_arg(ap, void *));
		va_end(ap);
		return rv;
	}
	/*NOTREACHED*/
}

int
close(int fd)
{
	int (*op_close)(int);
	int rv;

	DPRINTF(("close -> %d\n", fd));
	if (fd_isrump(fd)) {
		int undup2 = 0;

		if (ISDUP2D(fd))
			undup2 = 1;
		fd = fd_host2rump(fd);
		op_close = GETSYSCALL(rump, CLOSE);
		rv = op_close(fd);
		if (rv == 0 && undup2)
			CLRDUP2(fd);
	} else {
		if (rumpclient__closenotify(&fd, RUMPCLIENT_CLOSE_CLOSE) == -1)
			return -1;
		op_close = GETSYSCALL(host, CLOSE);
		rv = op_close(fd);
	}

	return rv;
}

/*
 * write cannot issue a standard debug printf due to recursion
 */
ssize_t
write(int fd, const void *buf, size_t blen)
{
	ssize_t (*op_write)(int, const void *, size_t);

	if (fd_isrump(fd)) {
		fd = fd_host2rump(fd);
		op_write = GETSYSCALL(rump, WRITE);
	} else {
		op_write = GETSYSCALL(host, WRITE);
	}

	return op_write(fd, buf, blen);
}

/*
 * dup2 is special.  we allow dup2 of a rump kernel fd to 0-2 since
 * many programs do that.  dup2 of a rump kernel fd to another value
 * not >= fdoff is an error.
 *
 * Note: cannot rump2host newd, because it is often hardcoded.
 */
int
dup2(int oldd, int newd)
{
	int (*host_dup2)(int, int);
	int rv;

	DPRINTF(("dup2 -> %d (o) -> %d (n)\n", oldd, newd));

	if (fd_isrump(oldd)) {
		if (!(newd >= 0 && newd <= 2))
			return EBADF;
		oldd = fd_host2rump(oldd);
		rv = rump_sys_dup2(oldd, newd);
		if (rv != -1)
			SETDUP2(newd);
	} else {
		host_dup2 = syscalls[DUALCALL_DUP2].bs_host;
		if (rumpclient__closenotify(&newd, RUMPCLIENT_CLOSE_DUP2) == -1)
			return -1;
		rv = host_dup2(oldd, newd);
	}

	return rv;
}

int
dup(int oldd)
{

	return dodup(oldd, 0);
}

pid_t
fork()
{
	pid_t rv;

	DPRINTF(("fork\n"));

	rv = rumpclient__dofork(host_fork);

	DPRINTF(("fork returns %d\n", rv));
	return rv;
}
/* we do not have the luxury of not requiring a stackframe */
__strong_alias(__vfork14,fork);

int
daemon(int nochdir, int noclose)
{
	struct rumpclient_fork *rf;

	if ((rf = rumpclient_prefork()) == NULL)
		return -1;

	if (host_daemon(nochdir, noclose) == -1)
		return -1;

	if (rumpclient_fork_init(rf) == -1)
		return -1;

	return 0;
}

int
execve(const char *path, char *const argv[], char *const envp[])
{
	char buf[128];
	char *dup2str;
	char **newenv;
	size_t nelem;
	int rv, sverrno;

	snprintf(buf, sizeof(buf), "RUMPHIJACK__DUP2MASK=%u", dup2mask);
	dup2str = malloc(strlen(buf)+1);
	if (dup2str == NULL)
		return ENOMEM;
	strcpy(dup2str, buf);

	for (nelem = 0; envp && envp[nelem]; nelem++)
		continue;
	newenv = malloc(sizeof(*newenv) * nelem+2);
	if (newenv == NULL) {
		free(dup2str);
		return ENOMEM;
	}
	memcpy(newenv, envp, nelem*sizeof(*newenv));
	newenv[nelem] = dup2str;
	newenv[nelem+1] = NULL;

	rv = rumpclient_exec(path, argv, newenv);

	_DIAGASSERT(rv != 0);
	sverrno = errno;
	free(newenv);
	free(dup2str);
	errno = sverrno;
	return rv;
}

/*
 * select is done by calling poll.
 */
int
REALSELECT(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	struct timeval *timeout)
{
	struct pollfd *pfds;
	struct timespec ts, *tsp = NULL;
	nfds_t realnfds;
	int i, j;
	int rv, incr;

	DPRINTF(("select\n"));

	/*
	 * Well, first we must scan the fds to figure out how many
	 * fds there really are.  This is because up to and including
	 * nb5 poll() silently refuses nfds > process_maxopen_fds.
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

	if (realnfds) {
		pfds = calloc(realnfds, sizeof(*pfds));
		if (!pfds)
			return -1;
	} else {
		pfds = NULL;
	}

	for (i = 0, j = 0; i < nfds; i++) {
		incr = 0;
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
	assert(j == (int)realnfds);

	if (timeout) {
		TIMEVAL_TO_TIMESPEC(timeout, &ts);
		tsp = &ts;
	}
	rv = REALPOLLTS(pfds, realnfds, tsp, NULL);
	/*
	 * "If select() returns with an error the descriptor sets
	 * will be unmodified"
	 */
	if (rv < 0)
		goto out;

	/*
	 * zero out results (can't use FD_ZERO for the
	 * obvious select-me-not reason).  whee.
	 *
	 * We do this here since some software ignores the return
	 * value of select, and hence if the timeout expires, it may
	 * assume all input descriptors have activity.
	 */
	for (i = 0; i < nfds; i++) {
		if (readfds)
			FD_CLR(i, readfds);
		if (writefds)
			FD_CLR(i, writefds);
		if (exceptfds)
			FD_CLR(i, exceptfds);
	}
	if (rv == 0)
		goto out;

	/*
	 * We have >0 fds with activity.  Harvest the results.
	 */
	for (i = 0; i < (int)realnfds; i++) {
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
		if (fds[i].fd == -1)
			continue;

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
	int (*op_pollts)(struct pollfd *, nfds_t, const struct timespec *,
			 const sigset_t *);
	struct pollarg *parg = arg;
	intptr_t rv;

	op_pollts = GETSYSCALL(host, POLLTS);
	rv = op_pollts(parg->pfds, parg->nfds, parg->ts, parg->sigmask);
	if (rv == -1)
		parg->errnum = errno;
	rump_sys_write(parg->pipefd, &rv, sizeof(rv));

	return (void *)(intptr_t)rv;
}

int
REALPOLLTS(struct pollfd *fds, nfds_t nfds, const struct timespec *ts,
	const sigset_t *sigmask)
{
	int (*op_pollts)(struct pollfd *, nfds_t, const struct timespec *,
			 const sigset_t *);
	int (*host_close)(int);
	int hostcall = 0, rumpcall = 0;
	pthread_t pt;
	nfds_t i;
	int rv;

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
			pfd_rump[i].revents = pfd_host[i].revents = 0;
			fds[i].revents = 0;
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

		op_pollts = GETSYSCALL(rump, POLLTS);
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
		host_close = GETSYSCALL(host, CLOSE);
		if (rpipe[0] != -1)
			rump_sys_close(rpipe[0]);
		if (rpipe[1] != -1)
			rump_sys_close(rpipe[1]);
		if (hpipe[0] != -1)
			host_close(hpipe[0]);
		if (hpipe[1] != -1)
			host_close(hpipe[1]);
		free(pfd_host);
		free(pfd_rump);
		errno = sverrno;
	} else {
		if (hostcall) {
			op_pollts = GETSYSCALL(host, POLLTS);
		} else {
			op_pollts = GETSYSCALL(rump, POLLTS);
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
		ts.tv_nsec = (timeout % 1000) * 1000*1000;

		tsp = &ts;
	}

	return REALPOLLTS(fds, nfds, tsp, NULL);
}

int
REALKEVENT(int kq, const struct kevent *changelist, size_t nchanges,
	struct kevent *eventlist, size_t nevents,
	const struct timespec *timeout)
{
	int (*op_kevent)(int, const struct kevent *, size_t,
		struct kevent *, size_t, const struct timespec *);
	const struct kevent *ev;
	size_t i;

	/*
	 * Check that we don't attempt to kevent rump kernel fd's.
	 * That needs similar treatment to select/poll, but is slightly
	 * trickier since we need to manage to different kq descriptors.
	 * (TODO, in case you're wondering).
	 */
	for (i = 0; i < nchanges; i++) {
		ev = &changelist[i];
		if (ev->filter == EVFILT_READ || ev->filter == EVFILT_WRITE ||
		    ev->filter == EVFILT_VNODE) {
			if (fd_isrump((int)ev->ident))
				return ENOTSUP;
		}
	}

	op_kevent = GETSYSCALL(host, KEVENT);
	return op_kevent(kq, changelist, nchanges, eventlist, nevents, timeout);
}

/*
 * Rest are std type calls.
 */

FDCALL(int, bind, DUALCALL_BIND,					\
	(int fd, const struct sockaddr *name, socklen_t namelen),	\
	(int, const struct sockaddr *, socklen_t),			\
	(fd, name, namelen))

FDCALL(int, connect, DUALCALL_CONNECT,					\
	(int fd, const struct sockaddr *name, socklen_t namelen),	\
	(int, const struct sockaddr *, socklen_t),			\
	(fd, name, namelen))

FDCALL(int, getpeername, DUALCALL_GETPEERNAME,				\
	(int fd, struct sockaddr *name, socklen_t *namelen),		\
	(int, struct sockaddr *, socklen_t *),				\
	(fd, name, namelen))

FDCALL(int, getsockname, DUALCALL_GETSOCKNAME, 				\
	(int fd, struct sockaddr *name, socklen_t *namelen),		\
	(int, struct sockaddr *, socklen_t *),				\
	(fd, name, namelen))

FDCALL(int, listen, DUALCALL_LISTEN,	 				\
	(int fd, int backlog),						\
	(int, int),							\
	(fd, backlog))

FDCALL(ssize_t, recvfrom, DUALCALL_RECVFROM, 				\
	(int fd, void *buf, size_t len, int flags,			\
	    struct sockaddr *from, socklen_t *fromlen),			\
	(int, void *, size_t, int, struct sockaddr *, socklen_t *),	\
	(fd, buf, len, flags, from, fromlen))

FDCALL(ssize_t, sendto, DUALCALL_SENDTO, 				\
	(int fd, const void *buf, size_t len, int flags,		\
	    const struct sockaddr *to, socklen_t tolen),		\
	(int, const void *, size_t, int,				\
	    const struct sockaddr *, socklen_t),			\
	(fd, buf, len, flags, to, tolen))

FDCALL(ssize_t, recvmsg, DUALCALL_RECVMSG, 				\
	(int fd, struct msghdr *msg, int flags),			\
	(int, struct msghdr *, int),					\
	(fd, msg, flags))

FDCALL(ssize_t, sendmsg, DUALCALL_SENDMSG, 				\
	(int fd, const struct msghdr *msg, int flags),			\
	(int, const struct msghdr *, int),				\
	(fd, msg, flags))

FDCALL(int, getsockopt, DUALCALL_GETSOCKOPT, 				\
	(int fd, int level, int optn, void *optval, socklen_t *optlen),	\
	(int, int, int, void *, socklen_t *),				\
	(fd, level, optn, optval, optlen))

FDCALL(int, setsockopt, DUALCALL_SETSOCKOPT, 				\
	(int fd, int level, int optn,					\
	    const void *optval, socklen_t optlen),			\
	(int, int, int, const void *, socklen_t),			\
	(fd, level, optn, optval, optlen))

FDCALL(int, shutdown, DUALCALL_SHUTDOWN, 				\
	(int fd, int how),						\
	(int, int),							\
	(fd, how))

#if _FORTIFY_SOURCE > 0
#define STUB(fun) __ssp_weak_name(fun)
ssize_t _sys_readlink(const char * __restrict, char * __restrict, size_t);
ssize_t
STUB(readlink)(const char * __restrict path, char * __restrict buf,
    size_t bufsiz)
{
	return _sys_readlink(path, buf, bufsiz);
}

char *_sys_getcwd(char *, size_t);
char *
STUB(getcwd)(char *buf, size_t size)
{
	return _sys_getcwd(buf, size);
}
#else
#define STUB(fun) fun
#endif

FDCALL(ssize_t, REALREAD, DUALCALL_READ,				\
	(int fd, void *buf, size_t buflen),				\
	(int, void *, size_t),						\
	(fd, buf, buflen))

FDCALL(ssize_t, readv, DUALCALL_READV, 					\
	(int fd, const struct iovec *iov, int iovcnt),			\
	(int, const struct iovec *, int),				\
	(fd, iov, iovcnt))

FDCALL(ssize_t, writev, DUALCALL_WRITEV, 				\
	(int fd, const struct iovec *iov, int iovcnt),			\
	(int, const struct iovec *, int),				\
	(fd, iov, iovcnt))
