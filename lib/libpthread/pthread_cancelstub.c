/*	$NetBSD: pthread_cancelstub.c,v 1.41 2022/02/12 14:59:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2002, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams and Andrew Doran.
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

/* Disable namespace mangling, Fortification is useless here anyway. */
#undef _FORTIFY_SOURCE

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_cancelstub.c,v 1.41 2022/02/12 14:59:32 riastradh Exp $");

/* Need to use libc-private names for atomic operations. */
#include "../../common/lib/libc/atomic/atomic_op_namespace.h"

#ifndef lint


/*
 * This is necessary because the names are always weak (they are not
 * POSIX functions).
 */
#define	fsync_range	_fsync_range
#define	pollts		_pollts

/*
 * XXX this is necessary to get the prototypes for the __sigsuspend14
 * XXX and __msync13 internal names, instead of the application-visible
 * XXX sigsuspend and msync names. It's kind of gross, but we're pretty
 * XXX intimate with libc already.
 */
#define __LIBC12_SOURCE__

#include <sys/msg.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <poll.h>
#include <stdarg.h>
#include <unistd.h>

#include <signal.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/event.h>

#include <compat/sys/mman.h>
#include <compat/sys/poll.h>
#include <compat/sys/select.h>
#include <compat/sys/event.h>
#include <compat/sys/wait.h>
#include <compat/include/mqueue.h>
#include <compat/include/signal.h>

#include "pthread.h"
#include "pthread_int.h"
#include "reentrant.h"

int	pthread__cancel_stub_binder;

int	_sys_accept(int, struct sockaddr *, socklen_t *);
int	_sys___aio_suspend50(const struct aiocb * const [], int,
	    const struct timespec *);
int	__aio_suspend50(const struct aiocb * const [], int,
	    const struct timespec *);
int	_sys_clock_nanosleep(clockid_t clock_id, int flags,
		   const struct timespec *rqtp, struct timespec *rmtp);
int	_sys_close(int);
int	_sys_connect(int, const struct sockaddr *, socklen_t);
int	_sys_fcntl(int, int, ...);
int	_sys_fdatasync(int);
int	_sys_fsync(int);
int	_sys_fsync_range(int, int, off_t, off_t);
int	_sys___kevent50(int, const struct kevent *, size_t, struct kevent *,
	    size_t, const struct timespec *);
int	_sys_mq_send(mqd_t, const char *, size_t, unsigned);
ssize_t	_sys_mq_receive(mqd_t, char *, size_t, unsigned *);
int	_sys___mq_timedsend50(mqd_t, const char *, size_t, unsigned,
	    const struct timespec *);
ssize_t	_sys___mq_timedreceive50(mqd_t, char *, size_t, unsigned *,
	    const struct timespec *);
ssize_t	_sys_msgrcv(int, void *, size_t, long, int);
int	_sys_msgsnd(int, const void *, size_t, int);
int	_sys___msync13(void *, size_t, int);
int	_sys___nanosleep50(const struct timespec *, struct timespec *);
int	__nanosleep50(const struct timespec *, struct timespec *);
int	_sys_open(const char *, int, ...);
int	_sys_openat(int, const char *, int, ...);
int	_sys_poll(struct pollfd *, nfds_t, int);
int	_sys___pollts50(struct pollfd *, nfds_t, const struct timespec *,
	    const sigset_t *);
ssize_t	_sys_pread(int, void *, size_t, off_t);
int	_sys___pselect50(int, fd_set *, fd_set *, fd_set *,
	    const struct timespec *, const sigset_t *);
ssize_t	_sys_pwrite(int, const void *, size_t, off_t);
ssize_t	_sys_read(int, void *, size_t);
ssize_t	_sys_readv(int, const struct iovec *, int);
ssize_t	_sys_recvfrom(int, void * restrict, size_t, int,
    struct sockaddr * restrict, socklen_t * restrict);
ssize_t _sys_recvmsg(int, struct msghdr *, int);
int _sys_recvmmsg(int, struct mmsghdr *, unsigned int, unsigned int,
    struct timespec *);
ssize_t _sys_sendto(int, const void *, size_t, int, const struct sockaddr *,
    socklen_t);
ssize_t _sys_sendmsg(int, const struct msghdr *, int);
int _sys_sendmmsg(int, struct mmsghdr *, unsigned int, unsigned int);
int	_sys___select50(int, fd_set *, fd_set *, fd_set *, struct timeval *);
int	_sys___wait450(pid_t, int *, int, struct rusage *);
ssize_t	_sys_write(int, const void *, size_t);
ssize_t	_sys_writev(int, const struct iovec *, int);
int	_sys___sigsuspend14(const sigset_t *);
int	____sigtimedwait50(const sigset_t * __restrict, siginfo_t * __restrict,
	    struct timespec * __restrict);
int	__sigsuspend14(const sigset_t *);

#define TESTCANCEL(id) 	do {						\
	if (__predict_true(!__uselibcstub) &&				\
	    __predict_false((id)->pt_cancel))				\
		pthread__cancelled();					\
	} while (/*CONSTCOND*/0)


int
accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_accept(s, addr, addrlen);
	TESTCANCEL(self);
	
	return retval;
}

int
__aio_suspend50(const struct aiocb * const list[], int nent,
    const struct timespec *timeout)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys___aio_suspend50(list, nent, timeout);
	TESTCANCEL(self);

	return retval;
}

int
__kevent50(int fd, const struct kevent *ev, size_t nev, struct kevent *rev,
    size_t nrev, const struct timespec *ts)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys___kevent50(fd, ev, nev, rev, nrev, ts);
	TESTCANCEL(self);

	return retval;
}

int
clock_nanosleep(clockid_t clock_id, int flags,
    const struct timespec *rqtp, struct timespec *rmtp)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_clock_nanosleep(clock_id, flags, rqtp, rmtp);
	TESTCANCEL(self);
	
	return retval;
}

int
close(int d)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_close(d);
	TESTCANCEL(self);
	
	return retval;
}

int
connect(int s, const struct sockaddr *addr, socklen_t namelen)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_connect(s, addr, namelen);
	TESTCANCEL(self);
	
	return retval;
}

int
fcntl(int fd, int cmd, ...)
{
	int retval;
	pthread_t self;
	va_list ap;

	self = pthread__self();
	TESTCANCEL(self);
	va_start(ap, cmd);
	retval = _sys_fcntl(fd, cmd, va_arg(ap, void *));
	va_end(ap);
	TESTCANCEL(self);

	return retval;
}

int
fdatasync(int d)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_fdatasync(d);
	TESTCANCEL(self);
	
	return retval;
}

int
fsync(int d)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_fsync(d);
	TESTCANCEL(self);
	
	return retval;
}

int
fsync_range(int d, int f, off_t s, off_t e)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_fsync_range(d, f, s, e);
	TESTCANCEL(self);

	return retval;
}

int
mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_mq_send(mqdes, msg_ptr, msg_len, msg_prio);
	TESTCANCEL(self);

	return retval;
}

ssize_t
mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_mq_receive(mqdes, msg_ptr, msg_len, msg_prio);
	TESTCANCEL(self);

	return retval;
}

int
__mq_timedsend50(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
    unsigned msg_prio, const struct timespec *abst)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys___mq_timedsend50(mqdes, msg_ptr, msg_len, msg_prio, abst);
	TESTCANCEL(self);

	return retval;
}

ssize_t
__mq_timedreceive50(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio,
    const struct timespec *abst)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys___mq_timedreceive50(mqdes, msg_ptr, msg_len, msg_prio, abst);
	TESTCANCEL(self);

	return retval;
}

ssize_t
msgrcv(int msgid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_msgrcv(msgid, msgp, msgsz, msgtyp, msgflg);
	TESTCANCEL(self);

	return retval;
}

int
msgsnd(int msgid, const void *msgp, size_t msgsz, int msgflg)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_msgsnd(msgid, msgp, msgsz, msgflg);
	TESTCANCEL(self);

	return retval;
}

int
__msync13(void *addr, size_t len, int flags)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys___msync13(addr, len, flags);
	TESTCANCEL(self);

	return retval;
}

int
open(const char *path, int flags, ...)
{
	int retval;
	pthread_t self;
	va_list ap;

	self = pthread__self();
	TESTCANCEL(self);
	va_start(ap, flags);
	retval = _sys_open(path, flags, va_arg(ap, mode_t));
	va_end(ap);
	TESTCANCEL(self);

	return retval;
}

int
openat(int fd, const char *path, int flags, ...)
{
	int retval;
	pthread_t self;
	va_list ap;

	self = pthread__self();
	TESTCANCEL(self);
	va_start(ap, flags);
	retval = _sys_openat(fd, path, flags, va_arg(ap, mode_t));
	va_end(ap);
	TESTCANCEL(self);

	return retval;
}

int
__nanosleep50(const struct timespec *rqtp, struct timespec *rmtp)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	/*
	 * For now, just nanosleep.  In the future, maybe pass a ucontext_t
	 * to _lwp_nanosleep() and allow it to recycle our kernel stack.
	 */
	retval = _sys___nanosleep50(rqtp, rmtp);
	TESTCANCEL(self);

	return retval;
}

int
poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_poll(fds, nfds, timeout);
	TESTCANCEL(self);

	return retval;
}

int
__pollts50(struct pollfd *fds, nfds_t nfds, const struct timespec *ts,
    const sigset_t *sigmask)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys___pollts50(fds, nfds, ts, sigmask);
	TESTCANCEL(self);

	return retval;
}

ssize_t
pread(int d, void *buf, size_t nbytes, off_t offset)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_pread(d, buf, nbytes, offset);
	TESTCANCEL(self);

	return retval;
}

int
__pselect50(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, 
    const struct timespec *timeout, const sigset_t *sigmask)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys___pselect50(nfds, readfds, writefds, exceptfds, timeout,
	    sigmask);
	TESTCANCEL(self);

	return retval;
}

ssize_t
pwrite(int d, const void *buf, size_t nbytes, off_t offset)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_pwrite(d, buf, nbytes, offset);
	TESTCANCEL(self);

	return retval;
}

ssize_t
read(int d, void *buf, size_t nbytes)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_read(d, buf, nbytes);
	TESTCANCEL(self);

	return retval;
}

ssize_t
readv(int d, const struct iovec *iov, int iovcnt)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_readv(d, iov, iovcnt);
	TESTCANCEL(self);

	return retval;
}

ssize_t
recvfrom(int s, void * restrict buf, size_t len, int flags,
    struct sockaddr * restrict from, socklen_t * restrict fromlen)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_recvfrom(s, buf, len, flags, from, fromlen);
	TESTCANCEL(self);

	return retval;
}

ssize_t
recvmsg(int s, struct msghdr *msg, int flags)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_recvmsg(s, msg, flags);
	TESTCANCEL(self);

	return retval;
}

int
recvmmsg(int s, struct mmsghdr *mmsg, unsigned int vlen,
    unsigned int flags, struct timespec *timeout)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_recvmmsg(s, mmsg, vlen, flags, timeout);
	TESTCANCEL(self);

	return retval;
}

int
__select50(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, 
    struct timeval *timeout)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys___select50(nfds, readfds, writefds, exceptfds, timeout);
	TESTCANCEL(self);

	return retval;
}

ssize_t
sendto(int s, const void *msg, size_t len, int flags,
    const struct sockaddr *to, socklen_t tolen)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_sendto(s, msg, len, flags, to, tolen);
	TESTCANCEL(self);

	return retval;
}

ssize_t
sendmsg(int s, const struct msghdr *msg, int flags)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_sendmsg(s, msg, flags);
	TESTCANCEL(self);

	return retval;
}

int
sendmmsg(int s, struct mmsghdr *mmsg, unsigned int vlen,
    unsigned int flags)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_sendmmsg(s, mmsg, vlen, flags);
	TESTCANCEL(self);

	return retval;
}


pid_t
__wait450(pid_t wpid, int *status, int options, struct rusage *rusage)
{
	pid_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys___wait450(wpid, status, options, rusage);
	TESTCANCEL(self);

	return retval;
}

ssize_t
write(int d, const void *buf, size_t nbytes)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_write(d, buf, nbytes);
	TESTCANCEL(self);

	return retval;
}

ssize_t
writev(int d, const struct iovec *iov, int iovcnt)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_writev(d, iov, iovcnt);
	TESTCANCEL(self);

	return retval;
}

int
__sigsuspend14(const sigset_t *sigmask)
{
	pthread_t self;
	int retval;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys___sigsuspend14(sigmask);
	TESTCANCEL(self);

	return retval;
}

int
__sigtimedwait50(const sigset_t * __restrict set, siginfo_t * __restrict info,
    const struct timespec * __restrict timeout)
{
	pthread_t self;
	int retval;
	struct timespec tout, *tp;

	if (timeout) {
		tout = *timeout;
		tp = &tout;
	} else
		tp = NULL;

	self = pthread__self();
	TESTCANCEL(self);
	retval = ____sigtimedwait50(set, info, tp);
	TESTCANCEL(self);

	return retval;
}

int
sigwait(const sigset_t * __restrict set, int * __restrict sig)
{
	pthread_t	self;
	int		saved_errno;
	int		new_errno;
	int		retval;

	self = pthread__self();
	saved_errno = errno;
	TESTCANCEL(self);
	retval = ____sigtimedwait50(set, NULL, NULL);
	TESTCANCEL(self);
	new_errno = errno;
	errno = saved_errno;
	if (retval < 0) {
		return new_errno;
	}
	*sig = retval;
	return 0;
}

__strong_alias(_close, close)
__strong_alias(_clock_nanosleep, clock_nanosleep)
__strong_alias(_fcntl, fcntl)
__strong_alias(_fdatasync, fdatasync)
__strong_alias(_fsync, fsync)
__weak_alias(fsync_range, _fsync_range)
__strong_alias(_mq_send, mq_send)
__strong_alias(_mq_receive, mq_receive)
__strong_alias(_msgrcv, msgrcv)
__strong_alias(_msgsnd, msgsnd)
__strong_alias(___msync13, __msync13)
__strong_alias(___nanosleep50, __nanosleep50)
__strong_alias(_open, open)
__strong_alias(_openat, openat)
__strong_alias(_poll, poll)
__strong_alias(_pread, pread)
__strong_alias(_pwrite, pwrite)
__strong_alias(_read, read)
__strong_alias(_readv, readv)
__strong_alias(_recvfrom, recvfrom)
__strong_alias(_recvmsg, recvmsg)
__strong_alias(_recvmmsg, recvmmsg)
__strong_alias(_sendmsg, sendmsg)
__strong_alias(_sendmmsg, sendmmsg)
__strong_alias(_sendto, sendto)
__strong_alias(_sigwait, sigwait)
__strong_alias(_write, write)
__strong_alias(_writev, writev)

#endif	/* !lint */
