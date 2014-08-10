/*      $NetBSD: hijack.c,v 1.107.2.1 2014/08/10 06:52:22 tls Exp $	*/

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

#include <rump/rumpuser_port.h>

#if !defined(lint)
__RCSID("$NetBSD: hijack.c,v 1.107.2.1 2014/08/10 06:52:22 tls Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>

#ifdef PLATFORM_HAS_NBVFSSTAT
#include <sys/statvfs.h>
#endif

#ifdef PLATFORM_HAS_KQUEUE
#include <sys/event.h>
#endif

#ifdef PLATFORM_HAS_NBQUOTA
#include <sys/quotactl.h>
#endif

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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <rump/rumpclient.h>
#include <rump/rump_syscalls.h>

#include "hijack.h"

/*
 * XXX: Consider autogenerating this, syscnames[] and syscalls[] with
 * a DSL where the tool also checks the symbols exported by this library
 * to make sure all relevant calls are accounted for.
 */
enum dualcall {
	DUALCALL_WRITE, DUALCALL_WRITEV, DUALCALL_PWRITE, DUALCALL_PWRITEV,
	DUALCALL_IOCTL, DUALCALL_FCNTL,
	DUALCALL_SOCKET, DUALCALL_ACCEPT, DUALCALL_BIND, DUALCALL_CONNECT,
	DUALCALL_GETPEERNAME, DUALCALL_GETSOCKNAME, DUALCALL_LISTEN,
	DUALCALL_RECVFROM, DUALCALL_RECVMSG,
	DUALCALL_SENDTO, DUALCALL_SENDMSG,
	DUALCALL_GETSOCKOPT, DUALCALL_SETSOCKOPT,
	DUALCALL_SHUTDOWN,
	DUALCALL_READ, DUALCALL_READV, DUALCALL_PREAD, DUALCALL_PREADV,
	DUALCALL_DUP2,
	DUALCALL_CLOSE,
	DUALCALL_POLLTS,

#ifndef __linux__
	DUALCALL_STAT, DUALCALL_LSTAT, DUALCALL_FSTAT,
#endif

	DUALCALL_CHMOD, DUALCALL_LCHMOD, DUALCALL_FCHMOD,
	DUALCALL_CHOWN, DUALCALL_LCHOWN, DUALCALL_FCHOWN,
	DUALCALL_OPEN,
	DUALCALL_CHDIR, DUALCALL_FCHDIR,
	DUALCALL_LSEEK,
	DUALCALL_UNLINK, DUALCALL_SYMLINK, DUALCALL_READLINK,
	DUALCALL_LINK, DUALCALL_RENAME,
	DUALCALL_MKDIR, DUALCALL_RMDIR,
	DUALCALL_UTIMES, DUALCALL_LUTIMES, DUALCALL_FUTIMES,
	DUALCALL_TRUNCATE, DUALCALL_FTRUNCATE,
	DUALCALL_FSYNC,
	DUALCALL_ACCESS,

#ifndef __linux__
	DUALCALL___GETCWD,
	DUALCALL_GETDENTS,
#endif

#ifndef __linux__
	DUALCALL_MKNOD,
#endif

#ifdef PLATFORM_HAS_NBFILEHANDLE
	DUALCALL_GETFH, DUALCALL_FHOPEN, DUALCALL_FHSTAT, DUALCALL_FHSTATVFS1,
#endif

#ifdef PLATFORM_HAS_KQUEUE
	DUALCALL_KEVENT,
#endif

#ifdef PLATFORM_HAS_NBSYSCTL
	DUALCALL___SYSCTL,
#endif

#ifdef PLATFORM_HAS_NFSSVC
	DUALCALL_NFSSVC,
#endif

#ifdef PLATFORM_HAS_NBVFSSTAT
	DUALCALL_STATVFS1, DUALCALL_FSTATVFS1, DUALCALL_GETVFSSTAT, 
#endif

#ifdef PLATFORM_HAS_NBMOUNT
	DUALCALL_MOUNT, DUALCALL_UNMOUNT,
#endif

#ifdef PLATFORM_HAS_FSYNC_RANGE
	DUALCALL_FSYNC_RANGE,
#endif

#ifdef PLATFORM_HAS_CHFLAGS
	DUALCALL_CHFLAGS, DUALCALL_LCHFLAGS, DUALCALL_FCHFLAGS,
#endif

#ifdef PLATFORM_HAS_NBQUOTA
	DUALCALL_QUOTACTL,
#endif
	DUALCALL__NUM
};

#define RSYS_STRING(a) __STRING(a)
#define RSYS_NAME(a) RSYS_STRING(__CONCAT(RUMP_SYS_RENAME_,a))

/*
 * Would be nice to get this automatically in sync with libc.
 * Also, this does not work for compat-using binaries (we should
 * provide all previous interfaces, not just the current ones)
 */
#if defined(__NetBSD__)

#if !__NetBSD_Prereq__(5,99,7)
#define REALSELECT select
#define REALPOLLTS pollts
#define REALKEVENT kevent
#define REALSTAT __stat30
#define REALLSTAT __lstat30
#define REALFSTAT __fstat30
#define REALUTIMES utimes
#define REALLUTIMES lutimes
#define REALFUTIMES futimes
#define REALMKNOD mknod
#define REALFHSTAT __fhstat40
#else /* >= 5.99.7 */
#define REALSELECT _sys___select50
#define REALPOLLTS _sys___pollts50
#define REALKEVENT _sys___kevent50
#define REALSTAT __stat50
#define REALLSTAT __lstat50
#define REALFSTAT __fstat50
#define REALUTIMES __utimes50
#define REALLUTIMES __lutimes50
#define REALFUTIMES __futimes50
#define REALMKNOD __mknod50
#define REALFHSTAT __fhstat50
#endif /* < 5.99.7 */

#define REALREAD _sys_read
#define REALPREAD _sys_pread
#define REALPWRITE _sys_pwrite
#define REALGETDENTS __getdents30
#define REALMOUNT __mount50
#define REALGETFH __getfh30
#define REALFHOPEN __fhopen40
#define REALFHSTATVFS1 __fhstatvfs140
#define OLDREALQUOTACTL __quotactl50	/* 5.99.48-62 only */
#define REALSOCKET __socket30

#define LSEEK_ALIAS _lseek
#define VFORK __vfork14

int REALSTAT(const char *, struct stat *);
int REALLSTAT(const char *, struct stat *);
int REALFSTAT(int, struct stat *);
int REALMKNOD(const char *, mode_t, dev_t);
int REALGETDENTS(int, char *, size_t);

int __getcwd(char *, size_t);

#elif defined(__linux__) /* glibc, really */

#define REALREAD read
#define REALPREAD pread
#define REALPWRITE pwrite
#define REALSELECT select
#define REALPOLLTS ppoll
#define REALUTIMES utimes
#define REALLUTIMES lutimes
#define REALFUTIMES futimes
#define REALFHSTAT fhstat
#define REALSOCKET socket

#else /* !NetBSD && !linux */

#error platform not supported

#endif /* platform */

int REALSELECT(int, fd_set *, fd_set *, fd_set *, struct timeval *);
int REALPOLLTS(struct pollfd *, nfds_t,
	       const struct timespec *, const sigset_t *);
int REALKEVENT(int, const struct kevent *, size_t, struct kevent *, size_t,
	       const struct timespec *);
ssize_t REALREAD(int, void *, size_t);
ssize_t REALPREAD(int, void *, size_t, off_t);
ssize_t REALPWRITE(int, const void *, size_t, off_t);
int REALUTIMES(const char *, const struct timeval [2]);
int REALLUTIMES(const char *, const struct timeval [2]);
int REALFUTIMES(int, const struct timeval [2]);
int REALMOUNT(const char *, const char *, int, void *, size_t);
int REALGETFH(const char *, void *, size_t *);
int REALFHOPEN(const void *, size_t, int);
int REALFHSTAT(const void *, size_t, struct stat *);
int REALFHSTATVFS1(const void *, size_t, struct statvfs *, int);
int REALSOCKET(int, int, int);

#ifdef PLATFORM_HAS_NBQUOTA
int OLDREALQUOTACTL(const char *, struct plistref *);
#endif

#define S(a) __STRING(a)
struct sysnames {
	enum dualcall scm_callnum;
	const char *scm_hostname;
	const char *scm_rumpname;
} syscnames[] = {
	{ DUALCALL_SOCKET,	S(REALSOCKET),	RSYS_NAME(SOCKET)	},
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
	{ DUALCALL_PREAD,	S(REALPREAD),	RSYS_NAME(PREAD)	},
	{ DUALCALL_PREADV,	"preadv",	RSYS_NAME(PREADV)	},
	{ DUALCALL_WRITE,	"write",	RSYS_NAME(WRITE)	},
	{ DUALCALL_WRITEV,	"writev",	RSYS_NAME(WRITEV)	},
	{ DUALCALL_PWRITE,	S(REALPWRITE),	RSYS_NAME(PWRITE)	},
	{ DUALCALL_PWRITEV,	"pwritev",	RSYS_NAME(PWRITEV)	},
	{ DUALCALL_IOCTL,	"ioctl",	RSYS_NAME(IOCTL)	},
	{ DUALCALL_FCNTL,	"fcntl",	RSYS_NAME(FCNTL)	},
	{ DUALCALL_DUP2,	"dup2",		RSYS_NAME(DUP2)		},
	{ DUALCALL_CLOSE,	"close",	RSYS_NAME(CLOSE)	},
	{ DUALCALL_POLLTS,	S(REALPOLLTS),	RSYS_NAME(POLLTS)	},
#ifndef __linux__
	{ DUALCALL_STAT,	S(REALSTAT),	RSYS_NAME(STAT)		},
	{ DUALCALL_LSTAT,	S(REALLSTAT),	RSYS_NAME(LSTAT)	},
	{ DUALCALL_FSTAT,	S(REALFSTAT),	RSYS_NAME(FSTAT)	},
#endif
	{ DUALCALL_CHOWN,	"chown",	RSYS_NAME(CHOWN)	},
	{ DUALCALL_LCHOWN,	"lchown",	RSYS_NAME(LCHOWN)	},
	{ DUALCALL_FCHOWN,	"fchown",	RSYS_NAME(FCHOWN)	},
	{ DUALCALL_CHMOD,	"chmod",	RSYS_NAME(CHMOD)	},
	{ DUALCALL_LCHMOD,	"lchmod",	RSYS_NAME(LCHMOD)	},
	{ DUALCALL_FCHMOD,	"fchmod",	RSYS_NAME(FCHMOD)	},
	{ DUALCALL_UTIMES,	S(REALUTIMES),	RSYS_NAME(UTIMES)	},
	{ DUALCALL_LUTIMES,	S(REALLUTIMES),	RSYS_NAME(LUTIMES)	},
	{ DUALCALL_FUTIMES,	S(REALFUTIMES),	RSYS_NAME(FUTIMES)	},
	{ DUALCALL_OPEN,	"open",		RSYS_NAME(OPEN)		},
	{ DUALCALL_CHDIR,	"chdir",	RSYS_NAME(CHDIR)	},
	{ DUALCALL_FCHDIR,	"fchdir",	RSYS_NAME(FCHDIR)	},
	{ DUALCALL_LSEEK,	"lseek",	RSYS_NAME(LSEEK)	},
	{ DUALCALL_UNLINK,	"unlink",	RSYS_NAME(UNLINK)	},
	{ DUALCALL_SYMLINK,	"symlink",	RSYS_NAME(SYMLINK)	},
	{ DUALCALL_READLINK,	"readlink",	RSYS_NAME(READLINK)	},
	{ DUALCALL_LINK,	"link",		RSYS_NAME(LINK)		},
	{ DUALCALL_RENAME,	"rename",	RSYS_NAME(RENAME)	},
	{ DUALCALL_MKDIR,	"mkdir",	RSYS_NAME(MKDIR)	},
	{ DUALCALL_RMDIR,	"rmdir",	RSYS_NAME(RMDIR)	},
	{ DUALCALL_TRUNCATE,	"truncate",	RSYS_NAME(TRUNCATE)	},
	{ DUALCALL_FTRUNCATE,	"ftruncate",	RSYS_NAME(FTRUNCATE)	},
	{ DUALCALL_FSYNC,	"fsync",	RSYS_NAME(FSYNC)	},
	{ DUALCALL_ACCESS,	"access",	RSYS_NAME(ACCESS)	},

#ifndef __linux__
	{ DUALCALL___GETCWD,	"__getcwd",	RSYS_NAME(__GETCWD)	},
	{ DUALCALL_GETDENTS,	S(REALGETDENTS),RSYS_NAME(GETDENTS)	},
#endif

#ifndef __linux__
	{ DUALCALL_MKNOD,	S(REALMKNOD),	RSYS_NAME(MKNOD)	},
#endif

#ifdef PLATFORM_HAS_NBFILEHANDLE
	{ DUALCALL_GETFH,	S(REALGETFH),	RSYS_NAME(GETFH)	},
	{ DUALCALL_FHOPEN,	S(REALFHOPEN),	RSYS_NAME(FHOPEN)	},
	{ DUALCALL_FHSTAT,	S(REALFHSTAT),	RSYS_NAME(FHSTAT)	},
	{ DUALCALL_FHSTATVFS1,	S(REALFHSTATVFS1),RSYS_NAME(FHSTATVFS1)	},
#endif

#ifdef PLATFORM_HAS_KQUEUE
	{ DUALCALL_KEVENT,	S(REALKEVENT),	RSYS_NAME(KEVENT)	},
#endif

#ifdef PLATFORM_HAS_NBSYSCTL
	{ DUALCALL___SYSCTL,	"__sysctl",	RSYS_NAME(__SYSCTL)	},
#endif

#ifdef PLATFORM_HAS_NFSSVC
	{ DUALCALL_NFSSVC,	"nfssvc",	RSYS_NAME(NFSSVC)	},
#endif

#ifdef PLATFORM_HAS_NBVFSSTAT
	{ DUALCALL_STATVFS1,	"statvfs1",	RSYS_NAME(STATVFS1)	},
	{ DUALCALL_FSTATVFS1,	"fstatvfs1",	RSYS_NAME(FSTATVFS1)	},
	{ DUALCALL_GETVFSSTAT,	"getvfsstat",	RSYS_NAME(GETVFSSTAT)	},
#endif

#ifdef PLATFORM_HAS_NBMOUNT
	{ DUALCALL_MOUNT,	S(REALMOUNT),	RSYS_NAME(MOUNT)	},
	{ DUALCALL_UNMOUNT,	"unmount",	RSYS_NAME(UNMOUNT)	},
#endif

#ifdef PLATFORM_HAS_FSYNC_RANGE
	{ DUALCALL_FSYNC_RANGE,	"fsync_range",	RSYS_NAME(FSYNC_RANGE)	},
#endif

#ifdef PLATFORM_HAS_CHFLAGS
	{ DUALCALL_CHFLAGS,	"chflags",	RSYS_NAME(CHFLAGS)	},
	{ DUALCALL_LCHFLAGS,	"lchflags",	RSYS_NAME(LCHFLAGS)	},
	{ DUALCALL_FCHFLAGS,	"fchflags",	RSYS_NAME(FCHFLAGS)	},
#endif /* PLATFORM_HAS_CHFLAGS */

#ifdef PLATFORM_HAS_NBQUOTA
#if __NetBSD_Prereq__(5,99,63)
	{ DUALCALL_QUOTACTL,	"__quotactl",	RSYS_NAME(__QUOTACTL)	},
#elif __NetBSD_Prereq__(5,99,48)
	{ DUALCALL_QUOTACTL,	S(OLDREALQUOTACTL),RSYS_NAME(QUOTACTL)	},
#endif
#endif /* PLATFORM_HAS_NBQUOTA */

};
#undef S

struct bothsys {
	void *bs_host;
	void *bs_rump;
} syscalls[DUALCALL__NUM];
#define GETSYSCALL(which, name) syscalls[DUALCALL_##name].bs_##which

static pid_t	(*host_fork)(void);
static int	(*host_daemon)(int, int);
static void *	(*host_mmap)(void *, size_t, int, int, int, off_t);

/*
 * This tracks if our process is in a subdirectory of /rump.
 * It's preserved over exec.
 */
static bool pwdinrump;

enum pathtype { PATH_HOST, PATH_RUMP, PATH_RUMPBLANKET };

static bool		fd_isrump(int);
static enum pathtype	path_isrump(const char *);

/* default FD_SETSIZE is 256 ==> default fdoff is 128 */
static int hijack_fdoff = FD_SETSIZE/2;

/*
 * Maintain a mapping table for the usual dup2 suspects.
 * Could use atomic ops to operate on dup2vec, but an application
 * racing there is not well-defined, so don't bother.
 */
/* note: you cannot change this without editing the env-passing code */
#define DUP2HIGH 2
static uint32_t dup2vec[DUP2HIGH+1];
#define DUP2BIT (1<<31)
#define DUP2ALIAS (1<<30)
#define DUP2FDMASK ((1<<30)-1)

static bool
isdup2d(int fd)
{

	return fd <= DUP2HIGH && fd >= 0 && dup2vec[fd] & DUP2BIT;
}

static int
mapdup2(int hostfd)
{

	_DIAGASSERT(isdup2d(hostfd));
	return dup2vec[hostfd] & DUP2FDMASK;
}

static int
unmapdup2(int rumpfd)
{
	int i;

	for (i = 0; i <= DUP2HIGH; i++) {
		if (dup2vec[i] & DUP2BIT &&
		    (dup2vec[i] & DUP2FDMASK) == (unsigned)rumpfd)
			return i;
	}
	return -1;
}

static void
setdup2(int hostfd, int rumpfd)
{

	if (hostfd > DUP2HIGH) {
		_DIAGASSERT(0);
		return;
	}

	dup2vec[hostfd] = DUP2BIT | DUP2ALIAS | rumpfd;
}

static void
clrdup2(int hostfd)
{

	if (hostfd > DUP2HIGH) {
		_DIAGASSERT(0);
		return;
	}

	dup2vec[hostfd] = 0;
}

static bool
killdup2alias(int rumpfd)
{
	int hostfd;

	if ((hostfd = unmapdup2(rumpfd)) == -1)
		return false;

	if (dup2vec[hostfd] & DUP2ALIAS) {
		dup2vec[hostfd] &= ~DUP2ALIAS;
		return true;
	}
	return false;
}

//#define DEBUGJACK
#ifdef DEBUGJACK
#define DPRINTF(x) mydprintf x
static void
mydprintf(const char *fmt, ...)
{
	va_list ap;

	if (isdup2d(STDERR_FILENO))
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static const char *
whichfd(int fd)
{

	if (fd == -1)
		return "-1";
	else if (fd_isrump(fd))
		return "rump";
	else
		return "host";
}

static const char *
whichpath(const char *path)
{

	if (path_isrump(path))
		return "rump";
	else
		return "host";
}

#else
#define DPRINTF(x)
#endif

#define FDCALL(type, name, rcname, args, proto, vars)			\
type name args								\
{									\
	type (*fun) proto;						\
									\
	DPRINTF(("%s -> %d (%s)\n", __STRING(name), fd,	whichfd(fd)));	\
	if (fd_isrump(fd)) {						\
		fun = syscalls[rcname].bs_rump;				\
		fd = fd_host2rump(fd);					\
	} else {							\
		fun = syscalls[rcname].bs_host;				\
	}								\
									\
	return fun vars;						\
}

#define PATHCALL(type, name, rcname, args, proto, vars)			\
type name args								\
{									\
	type (*fun) proto;						\
	enum pathtype pt;						\
									\
	DPRINTF(("%s -> %s (%s)\n", __STRING(name), path,		\
	    whichpath(path)));						\
	if ((pt = path_isrump(path)) != PATH_HOST) {			\
		fun = syscalls[rcname].bs_rump;				\
		if (pt == PATH_RUMP)					\
			path = path_host2rump(path);			\
	} else {							\
		fun = syscalls[rcname].bs_host;				\
	}								\
									\
	return fun vars;						\
}

#define VFSCALL(bit, type, name, rcname, args, proto, vars)		\
type name args								\
{									\
	type (*fun) proto;						\
									\
	DPRINTF(("%s (0x%x, 0x%x)\n", __STRING(name), bit, vfsbits));	\
	if (vfsbits & bit) {						\
		fun = syscalls[rcname].bs_rump;				\
	} else {							\
		fun = syscalls[rcname].bs_host;				\
	}								\
									\
	return fun vars;						\
}

/*
 * These variables are set from the RUMPHIJACK string and control
 * which operations can product rump kernel file descriptors.
 * This should be easily extendable for future needs.
 */
#define RUMPHIJACK_DEFAULT "path=/rump,socket=all:nolocal"
static bool rumpsockets[PF_MAX];
static const char *rumpprefix;
static size_t rumpprefixlen;

static struct {
	int pf;
	const char *name;
} socketmap[] = {
	{ PF_LOCAL, "local" },
	{ PF_INET, "inet" },
#ifdef PF_LINK
	{ PF_LINK, "link" },
#endif
#ifdef PF_OROUTE
	{ PF_OROUTE, "oroute" },
#endif
	{ PF_ROUTE, "route" },
	{ PF_INET6, "inet6" },
#ifdef PF_MPLS 
	{ PF_MPLS, "mpls" },
#endif
	{ -1, NULL }
};

static void
sockparser(char *buf)
{
	char *p, *l = NULL;
	bool value;
	int i;

	/* if "all" is present, it must be specified first */
	if (strncmp(buf, "all", strlen("all")) == 0) {
		for (i = 0; i < (int)__arraycount(rumpsockets); i++) {
			rumpsockets[i] = true;
		}
		buf += strlen("all");
		if (*buf == ':')
			buf++;
	}

	for (p = strtok_r(buf, ":", &l); p; p = strtok_r(NULL, ":", &l)) {
		value = true;
		if (strncmp(p, "no", strlen("no")) == 0) {
			value = false;
			p += strlen("no");
		}

		for (i = 0; socketmap[i].name; i++) {
			if (strcmp(p, socketmap[i].name) == 0) {
				rumpsockets[socketmap[i].pf] = value;
				break;
			}
		}
		if (socketmap[i].name == NULL) {
			errx(1, "invalid socket specifier %s", p);
		}
	}
}

static void
pathparser(char *buf)
{

	/* sanity-check */
	if (*buf != '/')
		errx(1, "hijack path specifier must begin with ``/''");
	rumpprefixlen = strlen(buf);
	if (rumpprefixlen < 2)
		errx(1, "invalid hijack prefix: %s", buf);
	if (buf[rumpprefixlen-1] == '/' && strspn(buf, "/") != rumpprefixlen)
		errx(1, "hijack prefix may end in slash only if pure "
		    "slash, gave %s", buf);

	if ((rumpprefix = strdup(buf)) == NULL)
		err(1, "strdup");
	rumpprefixlen = strlen(rumpprefix);
}

static struct blanket {
	const char *pfx;
	size_t len;
} *blanket;
static int nblanket;

static void
blanketparser(char *buf)
{
	char *p, *l = NULL;
	int i;

	for (nblanket = 0, p = buf; p; p = strchr(p+1, ':'), nblanket++)
		continue;

	blanket = malloc(nblanket * sizeof(*blanket));
	if (blanket == NULL)
		err(1, "alloc blanket %d", nblanket);

	for (p = strtok_r(buf, ":", &l), i = 0; p;
	    p = strtok_r(NULL, ":", &l), i++) {
		blanket[i].pfx = strdup(p);
		if (blanket[i].pfx == NULL)
			err(1, "strdup blanket");
		blanket[i].len = strlen(p);

		if (blanket[i].len == 0 || *blanket[i].pfx != '/')
			errx(1, "invalid blanket specifier %s", p);
		if (*(blanket[i].pfx + blanket[i].len-1) == '/')
			errx(1, "invalid blanket specifier %s", p);
	}
}

#define VFSBIT_NFSSVC		0x01
#define VFSBIT_GETVFSSTAT	0x02
#define VFSBIT_FHCALLS		0x04
static unsigned vfsbits;

static struct {
	int bit;
	const char *name;
} vfscalls[] = {
	{ VFSBIT_NFSSVC, "nfssvc" },
	{ VFSBIT_GETVFSSTAT, "getvfsstat" },
	{ VFSBIT_FHCALLS, "fhcalls" },
	{ -1, NULL }
};

static void
vfsparser(char *buf)
{
	char *p, *l = NULL;
	bool turnon;
	unsigned int fullmask;
	int i;

	/* build the full mask and sanity-check while we're at it */
	fullmask = 0;
	for (i = 0; vfscalls[i].name != NULL; i++) {
		if (fullmask & vfscalls[i].bit)
			errx(1, "problem exists between vi and chair");
		fullmask |= vfscalls[i].bit;
	}


	/* if "all" is present, it must be specified first */
	if (strncmp(buf, "all", strlen("all")) == 0) {
		vfsbits = fullmask;
		buf += strlen("all");
		if (*buf == ':')
			buf++;
	}

	for (p = strtok_r(buf, ":", &l); p; p = strtok_r(NULL, ":", &l)) {
		turnon = true;
		if (strncmp(p, "no", strlen("no")) == 0) {
			turnon = false;
			p += strlen("no");
		}

		for (i = 0; vfscalls[i].name; i++) {
			if (strcmp(p, vfscalls[i].name) == 0) {
				if (turnon)
					vfsbits |= vfscalls[i].bit;
				else
					vfsbits &= ~vfscalls[i].bit;
				break;
			}
		}
		if (vfscalls[i].name == NULL) {
			errx(1, "invalid vfscall specifier %s", p);
		}
	}
}

static bool rumpsysctl = false;

static void
sysctlparser(char *buf)
{

	if (buf == NULL) {
		rumpsysctl = true;
		return;
	}

	if (strcasecmp(buf, "y") == 0 || strcasecmp(buf, "yes") == 0 ||
	    strcasecmp(buf, "yep") == 0 || strcasecmp(buf, "tottakai") == 0) {
		rumpsysctl = true;
		return;
	}
	if (strcasecmp(buf, "n") == 0 || strcasecmp(buf, "no") == 0) {
		rumpsysctl = false;
		return;
	}

	errx(1, "sysctl value should be y(es)/n(o), gave: %s", buf);
}

static void
fdoffparser(char *buf)
{
	unsigned long fdoff;
	char *ep;

	if (*buf == '-') {
		errx(1, "fdoff must not be negative");
	}
	fdoff = strtoul(buf, &ep, 10);
	if (*ep != '\0')
		errx(1, "invalid fdoff specifier \"%s\"", buf);
	if (fdoff >= INT_MAX/2 || fdoff < 3)
		errx(1, "fdoff out of range");
	hijack_fdoff = fdoff;
}

static struct {
	void (*parsefn)(char *);
	const char *name;
	bool needvalues;
} hijackparse[] = {
	{ sockparser, "socket", true },
	{ pathparser, "path", true },
	{ blanketparser, "blanket", true },
	{ vfsparser, "vfs", true },
	{ sysctlparser, "sysctl", false },
	{ fdoffparser, "fdoff", true },
	{ NULL, NULL, false },
};

static void
parsehijack(char *hijack)
{
	char *p, *p2, *l;
	const char *hijackcopy;
	bool nop2;
	int i;

	if ((hijackcopy = strdup(hijack)) == NULL)
		err(1, "strdup");

	/* disable everything explicitly */
	for (i = 0; i < PF_MAX; i++)
		rumpsockets[i] = false;

	for (p = strtok_r(hijack, ",", &l); p; p = strtok_r(NULL, ",", &l)) {
		nop2 = false;
		p2 = strchr(p, '=');
		if (!p2) {
			nop2 = true;
			p2 = p + strlen(p);
		}

		for (i = 0; hijackparse[i].parsefn; i++) {
			if (strncmp(hijackparse[i].name, p,
			    (size_t)(p2-p)) == 0) {
				if (nop2 && hijackparse[i].needvalues)
					errx(1, "invalid hijack specifier: %s",
					    hijackcopy);
				hijackparse[i].parsefn(nop2 ? NULL : p2+1);
				break;
			}
		}

		if (hijackparse[i].parsefn == NULL)
			errx(1, "invalid hijack specifier name in %s", p);
	}

}

static void __attribute__((constructor))
rcinit(void)
{
	char buf[1024];
	unsigned i, j;

	host_fork = dlsym(RTLD_NEXT, "fork");
	host_daemon = dlsym(RTLD_NEXT, "daemon");
	host_mmap = dlsym(RTLD_NEXT, "mmap");

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
			errx(1, "hostcall %s not found!",
			    syscnames[j].scm_hostname);

		syscalls[i].bs_rump = dlsym(RTLD_NEXT,
		    syscnames[j].scm_rumpname);
		if (syscalls[i].bs_rump == NULL)
			errx(1, "rumpcall %s not found!",
			    syscnames[j].scm_rumpname);
	}

	if (rumpclient_init() == -1)
		err(1, "rumpclient init");

	/* check which syscalls we're supposed to hijack */
	if (getenv_r("RUMPHIJACK", buf, sizeof(buf)) == -1) {
		strcpy(buf, RUMPHIJACK_DEFAULT);
	}
	parsehijack(buf);

	/* set client persistence level */
	if (getenv_r("RUMPHIJACK_RETRYCONNECT", buf, sizeof(buf)) != -1) {
		if (strcmp(buf, "die") == 0)
			rumpclient_setconnretry(RUMPCLIENT_RETRYCONN_DIE);
		else if (strcmp(buf, "inftime") == 0)
			rumpclient_setconnretry(RUMPCLIENT_RETRYCONN_INFTIME);
		else if (strcmp(buf, "once") == 0)
			rumpclient_setconnretry(RUMPCLIENT_RETRYCONN_ONCE);
		else {
			time_t timeout;
			char *ep;

			timeout = (time_t)strtoll(buf, &ep, 10);
			if (timeout <= 0 || ep != buf + strlen(buf))
				errx(1, "RUMPHIJACK_RETRYCONNECT must be "
				    "keyword or integer, got: %s", buf);

			rumpclient_setconnretry(timeout);
		}
	}

	if (getenv_r("RUMPHIJACK__DUP2INFO", buf, sizeof(buf)) == 0) {
		if (sscanf(buf, "%u,%u,%u",
		    &dup2vec[0], &dup2vec[1], &dup2vec[2]) != 3) {
			warnx("invalid dup2mask: %s", buf);
			memset(dup2vec, 0, sizeof(dup2vec));
		}
		unsetenv("RUMPHIJACK__DUP2INFO");
	}
	if (getenv_r("RUMPHIJACK__PWDINRUMP", buf, sizeof(buf)) == 0) {
		pwdinrump = true;
		unsetenv("RUMPHIJACK__PWDINRUMP");
	}
}

static int
fd_rump2host(int fd)
{

	if (fd == -1)
		return fd;
	return fd + hijack_fdoff;
}

static int
fd_rump2host_withdup(int fd)
{
	int hfd;

	_DIAGASSERT(fd != -1);
	hfd = unmapdup2(fd);
	if (hfd != -1) {
		_DIAGASSERT(hfd <= DUP2HIGH);
		return hfd;
	}
	return fd_rump2host(fd);
}

static int
fd_host2rump(int fd)
{

	if (!isdup2d(fd))
		return fd - hijack_fdoff;
	else
		return mapdup2(fd);
}

static bool
fd_isrump(int fd)
{

	return isdup2d(fd) || fd >= hijack_fdoff;
}

#define assertfd(_fd_) assert(ISDUP2D(_fd_) || (_fd_) >= hijack_fdoff)

static enum pathtype
path_isrump(const char *path)
{
	size_t plen;
	int i;

	if (rumpprefix == NULL && nblanket == 0)
		return PATH_HOST;

	if (*path == '/') {
		plen = strlen(path);
		if (rumpprefix && plen >= rumpprefixlen) {
			if (strncmp(path, rumpprefix, rumpprefixlen) == 0
			    && (plen == rumpprefixlen
			      || *(path + rumpprefixlen) == '/')) {
				return PATH_RUMP;
			}
		}
		for (i = 0; i < nblanket; i++) {
			if (strncmp(path, blanket[i].pfx, blanket[i].len) == 0)
				return PATH_RUMPBLANKET;
		}

		return PATH_HOST;
	} else {
		return pwdinrump ? PATH_RUMP : PATH_HOST;
	}
}

static const char *rootpath = "/";
static const char *
path_host2rump(const char *path)
{
	const char *rv;

	if (*path == '/') {
		rv = path + rumpprefixlen;
		if (*rv == '\0')
			rv = rootpath;
	} else {
		rv = path;
	}

	return rv;
}

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
		if (minfd >= hijack_fdoff)
			minfd -= hijack_fdoff;
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

/*
 * Check that host fd value does not exceed fdoffset and if necessary
 * dup the file descriptor so that it doesn't collide with the dup2mask.
 */
static int
fd_host2host(int fd)
{
	int (*op_fcntl)(int, int, ...) = GETSYSCALL(host, FCNTL);
	int (*op_close)(int) = GETSYSCALL(host, CLOSE);
	int ofd, i;

	if (fd >= hijack_fdoff) {
		op_close(fd);
		errno = ENFILE;
		return -1;
	}

	for (i = 1; isdup2d(fd); i++) {
		ofd = fd;
		fd = op_fcntl(ofd, F_DUPFD, i);
		op_close(ofd);
	}

	return fd;
}

int
open(const char *path, int flags, ...)
{
	int (*op_open)(const char *, int, ...);
	bool isrump;
	va_list ap;
	enum pathtype pt;
	int fd;

	DPRINTF(("open -> %s (%s)\n", path, whichpath(path)));

	if ((pt = path_isrump(path)) != PATH_HOST) {
		if (pt == PATH_RUMP)
			path = path_host2rump(path);
		op_open = GETSYSCALL(rump, OPEN);
		isrump = true;
	} else {
		op_open = GETSYSCALL(host, OPEN);
		isrump = false;
	}

	va_start(ap, flags);
	fd = op_open(path, flags, va_arg(ap, mode_t));
	va_end(ap);

	if (isrump)
		fd = fd_rump2host(fd);
	else
		fd = fd_host2host(fd);

	DPRINTF(("open <- %d (%s)\n", fd, whichfd(fd)));
	return fd;
}

int
chdir(const char *path)
{
	int (*op_chdir)(const char *);
	enum pathtype pt;
	int rv;

	if ((pt = path_isrump(path)) != PATH_HOST) {
		op_chdir = GETSYSCALL(rump, CHDIR);
		if (pt == PATH_RUMP)
			path = path_host2rump(path);
	} else {
		op_chdir = GETSYSCALL(host, CHDIR);
	}

	rv = op_chdir(path);
	if (rv == 0)
		pwdinrump = pt != PATH_HOST;

	return rv;
}

int
fchdir(int fd)
{
	int (*op_fchdir)(int);
	bool isrump;
	int rv;

	if (fd_isrump(fd)) {
		op_fchdir = GETSYSCALL(rump, FCHDIR);
		isrump = true;
		fd = fd_host2rump(fd);
	} else {
		op_fchdir = GETSYSCALL(host, FCHDIR);
		isrump = false;
	}

	rv = op_fchdir(fd);
	if (rv == 0) {
		pwdinrump = isrump;
	}

	return rv;
}

#ifndef __linux__
int
__getcwd(char *bufp, size_t len)
{
	int (*op___getcwd)(char *, size_t);
	size_t prefixgap;
	bool iamslash;
	int rv;

	if (pwdinrump && rumpprefix) {
		if (rumpprefix[rumpprefixlen-1] == '/')
			iamslash = true;
		else
			iamslash = false;

		if (iamslash)
			prefixgap = rumpprefixlen - 1; /* ``//+path'' */
		else
			prefixgap = rumpprefixlen; /* ``/pfx+/path'' */
		if (len <= prefixgap) {
			errno = ERANGE;
			return -1;
		}

		op___getcwd = GETSYSCALL(rump, __GETCWD);
		rv = op___getcwd(bufp + prefixgap, len - prefixgap);
		if (rv == -1)
			return rv;

		/* augment the "/" part only for a non-root path */
		memcpy(bufp, rumpprefix, rumpprefixlen);

		/* append / only to non-root cwd */
		if (rv != 2)
			bufp[prefixgap] = '/';

		/* don't append extra slash in the purely-slash case */
		if (rv == 2 && !iamslash)
			bufp[rumpprefixlen] = '\0';
	} else if (pwdinrump) {
		/* assume blanket.  we can't provide a prefix here */
		op___getcwd = GETSYSCALL(rump, __GETCWD);
		rv = op___getcwd(bufp, len);
	} else {
		op___getcwd = GETSYSCALL(host, __GETCWD);
		rv = op___getcwd(bufp, len);
	}

	return rv;
}
#endif

static int
moveish(const char *from, const char *to,
    int (*rump_op)(const char *, const char *),
    int (*host_op)(const char *, const char *))
{
	int (*op)(const char *, const char *);
	enum pathtype ptf, ptt;

	if ((ptf = path_isrump(from)) != PATH_HOST) {
		if ((ptt = path_isrump(to)) == PATH_HOST) {
			errno = EXDEV;
			return -1;
		}

		if (ptf == PATH_RUMP)
			from = path_host2rump(from);
		if (ptt == PATH_RUMP)
			to = path_host2rump(to);
		op = rump_op;
	} else {
		if (path_isrump(to) != PATH_HOST) {
			errno = EXDEV;
			return -1;
		}

		op = host_op;
	}

	return op(from, to);
}

int
link(const char *from, const char *to)
{
	return moveish(from, to,
	    GETSYSCALL(rump, LINK), GETSYSCALL(host, LINK));
}

int
rename(const char *from, const char *to)
{
	return moveish(from, to,
	    GETSYSCALL(rump, RENAME), GETSYSCALL(host, RENAME));
}

int
REALSOCKET(int domain, int type, int protocol)
{
	int (*op_socket)(int, int, int);
	int fd;
	bool isrump;

	isrump = domain < PF_MAX && rumpsockets[domain];

	if (isrump)
		op_socket = GETSYSCALL(rump, SOCKET);
	else
		op_socket = GETSYSCALL(host, SOCKET);
	fd = op_socket(domain, type, protocol);

	if (isrump)
		fd = fd_rump2host(fd);
	else
		fd = fd_host2host(fd);
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
	else
		fd = fd_host2host(fd);

	DPRINTF((" <- %d\n", fd));

	return fd;
}

/*
 * ioctl() and fcntl() are varargs calls and need special treatment.
 */

/*
 * Various [Linux] libc's have various signatures for ioctl so we
 * need to handle the discrepancies.  On NetBSD, we use the
 * one with unsigned long cmd.
 */
int
#ifdef HAVE_IOCTL_CMD_INT
ioctl(int fd, int cmd, ...)
{
	int (*op_ioctl)(int, int cmd, ...);
#else
ioctl(int fd, unsigned long cmd, ...)
{
	int (*op_ioctl)(int, unsigned long cmd, ...);
#endif
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

int
fcntl(int fd, int cmd, ...)
{
	int (*op_fcntl)(int, int, ...);
	va_list ap;
	int rv, minfd;

	DPRINTF(("fcntl -> %d (cmd %d)\n", fd, cmd));

	switch (cmd) {
	case F_DUPFD:
		va_start(ap, cmd);
		minfd = va_arg(ap, int);
		va_end(ap);
		return dodup(fd, minfd);

#ifdef F_CLOSEM
	case F_CLOSEM: {
		int maxdup2, i;

		/*
		 * So, if fd < HIJACKOFF, we want to do a host closem.
		 */

		if (fd < hijack_fdoff) {
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
		 * for the file descriptors not dup2'd.
		 */

		for (i = 0, maxdup2 = 0; i <= DUP2HIGH; i++) {
			if (dup2vec[i] & DUP2BIT) {
				int val;

				val = dup2vec[i] & DUP2FDMASK;
				maxdup2 = MAX(val, maxdup2);
			}
		}
		
		if (fd >= hijack_fdoff)
			fd -= hijack_fdoff;
		else
			fd = 0;
		fd = MAX(maxdup2+1, fd);

		/* hmm, maybe we should close rump fd's not within dup2mask? */
		return rump_sys_fcntl(fd, F_CLOSEM);
	}
#endif /* F_CLOSEM */

#ifdef F_MAXFD
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
#endif /* F_MAXFD */

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
		bool undup2 = false;
		int ofd;

		if (isdup2d(ofd = fd)) {
			undup2 = true;
		}

		fd = fd_host2rump(fd);
		if (!undup2 && killdup2alias(fd)) {
			return 0;
		}

		op_close = GETSYSCALL(rump, CLOSE);
		rv = op_close(fd);
		if (rv == 0 && undup2) {
			clrdup2(ofd);
		}
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
 * file descriptor passing
 *
 * we intercept sendmsg and recvmsg to convert file descriptors in
 * control messages.  an attempt to send a descriptor from a different kernel
 * is rejected.  (ENOTSUP)
 */

static int
msg_convert(struct msghdr *msg, int (*func)(int))
{
	struct cmsghdr *cmsg;

	for (cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL;
	    cmsg = CMSG_NXTHDR(msg, cmsg)) {
		if (cmsg->cmsg_level == SOL_SOCKET &&
		    cmsg->cmsg_type == SCM_RIGHTS) {
			int *fdp = (void *)CMSG_DATA(cmsg);
			const size_t size =
			    cmsg->cmsg_len - __CMSG_ALIGN(sizeof(*cmsg));
			const int nfds = (int)(size / sizeof(int));
			const int * const efdp = fdp + nfds;

			while (fdp < efdp) {
				const int newval = func(*fdp);

				if (newval < 0) {
					return ENOTSUP;
				}
				*fdp = newval;
				fdp++;
			}
		}
	}
	return 0;
}

ssize_t
recvmsg(int fd, struct msghdr *msg, int flags)
{
	ssize_t (*op_recvmsg)(int, struct msghdr *, int);
	ssize_t ret;
	const bool isrump = fd_isrump(fd);

	if (isrump) {
		fd = fd_host2rump(fd);
		op_recvmsg = GETSYSCALL(rump, RECVMSG);
	} else {
		op_recvmsg = GETSYSCALL(host, RECVMSG);
	}
	ret = op_recvmsg(fd, msg, flags);
	if (ret == -1) {
		return ret;
	}
	/*
	 * convert descriptors in the message.
	 */
	if (isrump) {
		msg_convert(msg, fd_rump2host);
	} else {
		msg_convert(msg, fd_host2host);
	}
	return ret;
}

ssize_t
recv(int fd, void *buf, size_t len, int flags)
{

	return recvfrom(fd, buf, len, flags, NULL, NULL);
}

ssize_t
send(int fd, const void *buf, size_t len, int flags)
{

	return sendto(fd, buf, len, flags, NULL, 0);
}

static int
fd_check_rump(int fd)
{

	return fd_isrump(fd) ? 0 : -1;
}

static int
fd_check_host(int fd)
{

	return !fd_isrump(fd) ? 0 : -1;
}

ssize_t
sendmsg(int fd, const struct msghdr *msg, int flags)
{
	ssize_t (*op_sendmsg)(int, const struct msghdr *, int);
	const bool isrump = fd_isrump(fd);
	int error;

	/*
	 * reject descriptors from a different kernel.
	 */
	error = msg_convert(__UNCONST(msg),
	    isrump ? fd_check_rump: fd_check_host);
	if (error != 0) {
		errno = error;
		return -1;
	}
	/*
	 * convert descriptors in the message to raw values.
	 */
	if (isrump) {
		fd = fd_host2rump(fd);
		/*
		 * XXX we directly modify the given message assuming:
		 * - cmsg is writable (typically on caller's stack)
		 * - caller don't care cmsg's contents after calling sendmsg.
		 *   (thus no need to restore values)
		 *
		 * it's safer to copy and modify instead.
		 */
		msg_convert(__UNCONST(msg), fd_host2rump);
		op_sendmsg = GETSYSCALL(rump, SENDMSG);
	} else {
		op_sendmsg = GETSYSCALL(host, SENDMSG);
	}
	return op_sendmsg(fd, msg, flags);
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
		int (*op_close)(int) = GETSYSCALL(host, CLOSE);

		/* only allow fd 0-2 for cross-kernel dup */
		if (!(newd >= 0 && newd <= 2 && !fd_isrump(newd))) {
			errno = EBADF;
			return -1;
		}

		/* regular dup2? */
		if (fd_isrump(newd)) {
			newd = fd_host2rump(newd);
			rv = rump_sys_dup2(oldd, newd);
			return fd_rump2host(rv);
		}

		/*
		 * dup2 rump => host?  just establish an
		 * entry in the mapping table.
		 */
		op_close(newd);
		setdup2(newd, fd_host2rump(oldd));
		rv = 0;
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
fork(void)
{
	pid_t rv;

	DPRINTF(("fork\n"));

	rv = rumpclient__dofork(host_fork);

	DPRINTF(("fork returns %d\n", rv));
	return rv;
}
#ifdef VFORK
/* we do not have the luxury of not requiring a stackframe */
__strong_alias(VFORK,fork);
#endif

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
	const char *pwdinrumpstr;
	char **newenv;
	size_t nelem;
	int rv, sverrno;
	int bonus = 2, i = 0;

	snprintf(buf, sizeof(buf), "RUMPHIJACK__DUP2INFO=%u,%u,%u",
	    dup2vec[0], dup2vec[1], dup2vec[2]);
	dup2str = strdup(buf);
	if (dup2str == NULL) {
		errno = ENOMEM;
		return -1;
	}

	if (pwdinrump) {
		pwdinrumpstr = "RUMPHIJACK__PWDINRUMP=true";
		bonus++;
	} else {
		pwdinrumpstr = NULL;
	}

	for (nelem = 0; envp && envp[nelem]; nelem++)
		continue;
	newenv = malloc(sizeof(*newenv) * (nelem+bonus));
	if (newenv == NULL) {
		free(dup2str);
		errno = ENOMEM;
		return -1;
	}
	memcpy(newenv, envp, nelem*sizeof(*newenv));
	newenv[nelem+i] = dup2str;
	i++;

	if (pwdinrumpstr) {
		newenv[nelem+i] = __UNCONST(pwdinrumpstr);
		i++;
	}
	newenv[nelem+i] = NULL;
	_DIAGASSERT(i < bonus);

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

	DPRINTF(("select %d %p %p %p %p\n", nfds,
	    readfds, writefds, exceptfds, timeout));

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

	return (void *)rv;
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

	DPRINTF(("poll %p %d %p %p\n", fds, (int)nfds, ts, sigmask));
	checkpoll(fds, nfds, &hostcall, &rumpcall);

	if (hostcall && rumpcall) {
		struct pollfd *pfd_host = NULL, *pfd_rump = NULL;
		int rpipe[2] = {-1,-1}, hpipe[2] = {-1,-1};
		struct pollarg parg;
		void *trv_val;
		int sverrno = 0, rv_rump, rv_host, errno_rump, errno_host;

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

		/*
		 * then, open two pipes, one for notifications
		 * to each kernel.
		 *
		 * At least the rump pipe should probably be
		 * cached, along with the helper threads.  This
		 * should give a microbenchmark improvement (haven't
		 * experienced a macro-level problem yet, though).
		 */
		if ((rv = rump_sys_pipe(rpipe)) == -1) {
			sverrno = errno;
		}
		if (rv == 0 && (rv = pipe(hpipe)) == -1) {
			sverrno = errno;
		}

		/* split vectors (or signal errors) */
		for (i = 0; i < nfds; i++) {
			int fd;

			fds[i].revents = 0;
			if (fds[i].fd == -1) {
				pfd_host[i].fd = -1;
				pfd_rump[i].fd = -1;
			} else if (fd_isrump(fds[i].fd)) {
				pfd_host[i].fd = -1;
				fd = fd_host2rump(fds[i].fd);
				if (fd == rpipe[0] || fd == rpipe[1]) {
					fds[i].revents = POLLNVAL;
					if (rv != -1)
						rv++;
				}
				pfd_rump[i].fd = fd;
				pfd_rump[i].events = fds[i].events;
			} else {
				pfd_rump[i].fd = -1;
				fd = fds[i].fd;
				if (fd == hpipe[0] || fd == hpipe[1]) {
					fds[i].revents = POLLNVAL;
					if (rv != -1)
						rv++;
				}
				pfd_host[i].fd = fd;
				pfd_host[i].events = fds[i].events;
			}
			pfd_rump[i].revents = pfd_host[i].revents = 0;
		}
		if (rv) {
			goto out;
		}

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
		rv_rump = op_pollts(pfd_rump, nfds+1, ts, NULL);
		errno_rump = errno;
		write(hpipe[1], &rv, sizeof(rv));
		pthread_join(pt, &trv_val);
		rv_host = (int)(intptr_t)trv_val;
		errno_host = parg.errnum;

		/* strip cross-thread notification from real results */
		if (rv_host > 0 && pfd_host[nfds].revents & POLLIN) {
			rv_host--;
		}
		if (rv_rump > 0 && pfd_rump[nfds].revents & POLLIN) {
			rv_rump--;
		}

		/* then merge the results into what's reported to the caller */
		if (rv_rump > 0 || rv_host > 0) {
			/* SUCCESS */

			rv = 0;
			if (rv_rump > 0) {
				for (i = 0; i < nfds; i++) {
					if (pfd_rump[i].fd != -1)
						fds[i].revents
						    = pfd_rump[i].revents;
				}
				rv += rv_rump;
			}
			if (rv_host > 0) {
				for (i = 0; i < nfds; i++) {
					if (pfd_host[i].fd != -1)
						fds[i].revents
						    = pfd_host[i].revents;
				}
				rv += rv_host;
			}
			assert(rv > 0);
			sverrno = 0;
		} else if (rv_rump == -1 || rv_host == -1) {
			/* ERROR */

			/* just pick one kernel at "random" */
			rv = -1;
			if (rv_host == -1) {
				sverrno = errno_host;
			} else if (rv_rump == -1) {
				sverrno = errno_rump;
			}
		} else {
			/* TIMEOUT */

			rv = 0;
			assert(rv_rump == 0 && rv_host == 0);
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
			adjustpoll(fds, nfds, fd_rump2host_withdup);
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

#ifdef PLATFORM_HAS_KQUEUE
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
			if (fd_isrump((int)ev->ident)) {
				errno = ENOTSUP;
				return -1;
			}
		}
	}

	op_kevent = GETSYSCALL(host, KEVENT);
	return op_kevent(kq, changelist, nchanges, eventlist, nevents, timeout);
}
#endif /* PLATFORM_HAS_KQUEUE */

/*
 * mmapping from a rump kernel is not supported, so disallow it.
 */
void *
mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{

	if (flags & MAP_FILE && fd_isrump(fd)) {
		errno = ENOSYS;
		return MAP_FAILED;
	}
	return host_mmap(addr, len, prot, flags, fd, offset);
}

#ifdef PLATFORM_HAS_NBSYSCTL
/*
 * these go to one or the other on a per-process configuration
 */
int __sysctl(const int *, unsigned int, void *, size_t *, const void *, size_t);
int
__sysctl(const int *name, unsigned int namelen, void *old, size_t *oldlenp,
	const void *new, size_t newlen)
{
	int (*op___sysctl)(const int *, unsigned int, void *, size_t *,
	    const void *, size_t);

	if (rumpsysctl) {
		op___sysctl = GETSYSCALL(rump, __SYSCTL);
	} else {
		op___sysctl = GETSYSCALL(host, __SYSCTL);
		/* we haven't inited yet */
		if (__predict_false(op___sysctl == NULL)) {
			op___sysctl = rumphijack_dlsym(RTLD_NEXT, "__sysctl");
		}
	}

	return op___sysctl(name, namelen, old, oldlenp, new, newlen);
}
#endif

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

FDCALL(ssize_t, REALREAD, DUALCALL_READ,				\
	(int fd, void *buf, size_t buflen),				\
	(int, void *, size_t),						\
	(fd, buf, buflen))

#ifdef __linux__
ssize_t __read_chk(int, void *, size_t)
    __attribute__((alias("read")));
#endif

FDCALL(ssize_t, readv, DUALCALL_READV, 					\
	(int fd, const struct iovec *iov, int iovcnt),			\
	(int, const struct iovec *, int),				\
	(fd, iov, iovcnt))

FDCALL(ssize_t, REALPREAD, DUALCALL_PREAD,				\
	(int fd, void *buf, size_t nbytes, off_t offset),		\
	(int, void *, size_t, off_t),					\
	(fd, buf, nbytes, offset))

FDCALL(ssize_t, preadv, DUALCALL_PREADV, 				\
	(int fd, const struct iovec *iov, int iovcnt, off_t offset),	\
	(int, const struct iovec *, int, off_t),			\
	(fd, iov, iovcnt, offset))

FDCALL(ssize_t, writev, DUALCALL_WRITEV, 				\
	(int fd, const struct iovec *iov, int iovcnt),			\
	(int, const struct iovec *, int),				\
	(fd, iov, iovcnt))

FDCALL(ssize_t, REALPWRITE, DUALCALL_PWRITE,				\
	(int fd, const void *buf, size_t nbytes, off_t offset),		\
	(int, const void *, size_t, off_t),				\
	(fd, buf, nbytes, offset))

FDCALL(ssize_t, pwritev, DUALCALL_PWRITEV, 				\
	(int fd, const struct iovec *iov, int iovcnt, off_t offset),	\
	(int, const struct iovec *, int, off_t),			\
	(fd, iov, iovcnt, offset))

#ifndef __linux__
FDCALL(int, REALFSTAT, DUALCALL_FSTAT,					\
	(int fd, struct stat *sb),					\
	(int, struct stat *),						\
	(fd, sb))
#endif

#ifdef PLATFORM_HAS_NBVFSSTAT
FDCALL(int, fstatvfs1, DUALCALL_FSTATVFS1,				\
	(int fd, struct statvfs *buf, int flags),			\
	(int, struct statvfs *, int),					\
	(fd, buf, flags))
#endif

FDCALL(off_t, lseek, DUALCALL_LSEEK,					\
	(int fd, off_t offset, int whence),				\
	(int, off_t, int),						\
	(fd, offset, whence))
#ifdef LSEEK_ALIAS
__strong_alias(LSEEK_ALIAS,lseek);
#endif

#ifndef __linux__
FDCALL(int, REALGETDENTS, DUALCALL_GETDENTS,				\
	(int fd, char *buf, size_t nbytes),				\
	(int, char *, size_t),						\
	(fd, buf, nbytes))
#endif

FDCALL(int, fchown, DUALCALL_FCHOWN,					\
	(int fd, uid_t owner, gid_t group),				\
	(int, uid_t, gid_t),						\
	(fd, owner, group))

FDCALL(int, fchmod, DUALCALL_FCHMOD,					\
	(int fd, mode_t mode),						\
	(int, mode_t),							\
	(fd, mode))

FDCALL(int, ftruncate, DUALCALL_FTRUNCATE,				\
	(int fd, off_t length),						\
	(int, off_t),							\
	(fd, length))

FDCALL(int, fsync, DUALCALL_FSYNC,					\
	(int fd),							\
	(int),								\
	(fd))

#ifdef PLATFORM_HAS_FSYNC_RANGE
FDCALL(int, fsync_range, DUALCALL_FSYNC_RANGE,				\
	(int fd, int how, off_t start, off_t length),			\
	(int, int, off_t, off_t),					\
	(fd, how, start, length))
#endif

FDCALL(int, futimes, DUALCALL_FUTIMES,					\
	(int fd, const struct timeval *tv),				\
	(int, const struct timeval *),					\
	(fd, tv))

#ifdef PLATFORM_HAS_CHFLAGS
FDCALL(int, fchflags, DUALCALL_FCHFLAGS,				\
	(int fd, u_long flags),						\
	(int, u_long),							\
	(fd, flags))
#endif

/*
 * path-based selectors
 */

#ifndef __linux__
PATHCALL(int, REALSTAT, DUALCALL_STAT,					\
	(const char *path, struct stat *sb),				\
	(const char *, struct stat *),					\
	(path, sb))

PATHCALL(int, REALLSTAT, DUALCALL_LSTAT,				\
	(const char *path, struct stat *sb),				\
	(const char *, struct stat *),					\
	(path, sb))
#endif

PATHCALL(int, chown, DUALCALL_CHOWN,					\
	(const char *path, uid_t owner, gid_t group),			\
	(const char *, uid_t, gid_t),					\
	(path, owner, group))

PATHCALL(int, lchown, DUALCALL_LCHOWN,					\
	(const char *path, uid_t owner, gid_t group),			\
	(const char *, uid_t, gid_t),					\
	(path, owner, group))

PATHCALL(int, chmod, DUALCALL_CHMOD,					\
	(const char *path, mode_t mode),				\
	(const char *, mode_t),						\
	(path, mode))

PATHCALL(int, lchmod, DUALCALL_LCHMOD,					\
	(const char *path, mode_t mode),				\
	(const char *, mode_t),						\
	(path, mode))

#ifdef PLATFORM_HAS_NBVFSSTAT
PATHCALL(int, statvfs1, DUALCALL_STATVFS1,				\
	(const char *path, struct statvfs *buf, int flags),		\
	(const char *, struct statvfs *, int),				\
	(path, buf, flags))
#endif

PATHCALL(int, unlink, DUALCALL_UNLINK,					\
	(const char *path),						\
	(const char *),							\
	(path))

PATHCALL(int, symlink, DUALCALL_SYMLINK,				\
	(const char *target, const char *path),				\
	(const char *, const char *),					\
	(target, path))

/*
 * readlink() can be called from malloc which can be called
 * from dlsym() during init
 */
ssize_t
readlink(const char *path, char *buf, size_t bufsiz)
{
	int (*op_readlink)(const char *, char *, size_t);
	enum pathtype pt;

	if ((pt = path_isrump(path)) != PATH_HOST) {
		op_readlink = GETSYSCALL(rump, READLINK);
		if (pt == PATH_RUMP)
			path = path_host2rump(path);
	} else {
		op_readlink = GETSYSCALL(host, READLINK);
	}

	if (__predict_false(op_readlink == NULL)) {
		errno = ENOENT;
		return -1;
	}

	return op_readlink(path, buf, bufsiz);
}

PATHCALL(int, mkdir, DUALCALL_MKDIR,					\
	(const char *path, mode_t mode),				\
	(const char *, mode_t),						\
	(path, mode))

PATHCALL(int, rmdir, DUALCALL_RMDIR,					\
	(const char *path),						\
	(const char *),							\
	(path))

PATHCALL(int, utimes, DUALCALL_UTIMES,					\
	(const char *path, const struct timeval *tv),			\
	(const char *, const struct timeval *),				\
	(path, tv))

PATHCALL(int, lutimes, DUALCALL_LUTIMES,				\
	(const char *path, const struct timeval *tv),			\
	(const char *, const struct timeval *),				\
	(path, tv))

#ifdef PLATFORM_HAS_CHFLAGS
PATHCALL(int, chflags, DUALCALL_CHFLAGS,				\
	(const char *path, u_long flags),				\
	(const char *, u_long),						\
	(path, flags))

PATHCALL(int, lchflags, DUALCALL_LCHFLAGS,				\
	(const char *path, u_long flags),				\
	(const char *, u_long),						\
	(path, flags))
#endif /* PLATFORM_HAS_CHFLAGS */

PATHCALL(int, truncate, DUALCALL_TRUNCATE,				\
	(const char *path, off_t length),				\
	(const char *, off_t),						\
	(path, length))

PATHCALL(int, access, DUALCALL_ACCESS,					\
	(const char *path, int mode),					\
	(const char *, int),						\
	(path, mode))

#ifndef __linux__
PATHCALL(int, REALMKNOD, DUALCALL_MKNOD,				\
	(const char *path, mode_t mode, dev_t dev),			\
	(const char *, mode_t, dev_t),					\
	(path, mode, dev))
#endif

/*
 * Note: with mount the decisive parameter is the mount
 * destination directory.  This is because we don't really know
 * about the "source" directory in a generic call (and besides,
 * it might not even exist, cf. nfs).
 */
#ifdef PLATFORM_HAS_NBMOUNT
PATHCALL(int, REALMOUNT, DUALCALL_MOUNT,				\
	(const char *type, const char *path, int flags,			\
	    void *data, size_t dlen),					\
	(const char *, const char *, int, void *, size_t),		\
	(type, path, flags, data, dlen))

PATHCALL(int, unmount, DUALCALL_UNMOUNT,				\
	(const char *path, int flags),					\
	(const char *, int),						\
	(path, flags))
#endif /* PLATFORM_HAS_NBMOUNT */

#ifdef PLATFORM_HAS_NBQUOTA
#if __NetBSD_Prereq__(5,99,63)
PATHCALL(int, __quotactl, DUALCALL_QUOTACTL,				\
	(const char *path, struct quotactl_args *args),			\
	(const char *, struct quotactl_args *),				\
	(path, args))
#elif __NetBSD_Prereq__(5,99,48)
PATHCALL(int, OLDREALQUOTACTL, DUALCALL_QUOTACTL,			\
	(const char *path, struct plistref *p),				\
	(const char *, struct plistref *),				\
	(path, p))
#endif
#endif /* PLATFORM_HAS_NBQUOTA */

#ifdef PLATFORM_HAS_NBFILEHANDLE
PATHCALL(int, REALGETFH, DUALCALL_GETFH,				\
	(const char *path, void *fhp, size_t *fh_size),			\
	(const char *, void *, size_t *),				\
	(path, fhp, fh_size))
#endif

/*
 * These act different on a per-process vfs configuration
 */

#ifdef PLATFORM_HAS_NBVFSSTAT
VFSCALL(VFSBIT_GETVFSSTAT, int, getvfsstat, DUALCALL_GETVFSSTAT,	\
	(struct statvfs *buf, size_t buflen, int flags),		\
	(struct statvfs *, size_t, int),				\
	(buf, buflen, flags))
#endif

#ifdef PLATFORM_HAS_NBFILEHANDLE
VFSCALL(VFSBIT_FHCALLS, int, REALFHOPEN, DUALCALL_FHOPEN,		\
	(const void *fhp, size_t fh_size, int flags),			\
	(const char *, size_t, int),					\
	(fhp, fh_size, flags))

VFSCALL(VFSBIT_FHCALLS, int, REALFHSTAT, DUALCALL_FHSTAT,		\
	(const void *fhp, size_t fh_size, struct stat *sb),		\
	(const char *, size_t, struct stat *),				\
	(fhp, fh_size, sb))

VFSCALL(VFSBIT_FHCALLS, int, REALFHSTATVFS1, DUALCALL_FHSTATVFS1,	\
	(const void *fhp, size_t fh_size, struct statvfs *sb, int flgs),\
	(const char *, size_t, struct statvfs *, int),			\
	(fhp, fh_size, sb, flgs))
#endif


#ifdef PLATFORM_HAS_NFSSVC

/* finally, put nfssvc here.  "keep the namespace clean" */
#include <nfs/rpcv2.h>
#include <nfs/nfs.h>

int
nfssvc(int flags, void *argstructp)
{
	int (*op_nfssvc)(int, void *);

	if (vfsbits & VFSBIT_NFSSVC){
		struct nfsd_args *nfsdargs;

		/* massage the socket descriptor if necessary */
		if (flags == NFSSVC_ADDSOCK) {
			nfsdargs = argstructp;
			nfsdargs->sock = fd_host2rump(nfsdargs->sock);
		}
		op_nfssvc = GETSYSCALL(rump, NFSSVC);
	} else
		op_nfssvc = GETSYSCALL(host, NFSSVC);

	return op_nfssvc(flags, argstructp);
}
#endif /* PLATFORM_HAS_NFSSVC */
