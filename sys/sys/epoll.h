/*	$NetBSD: epoll.h,v 1.3 2024/10/30 15:56:12 riastradh Exp $	*/

/*-
 * Copyright (c) 2007 Roman Divacky
 * Copyright (c) 2014 Dmitry Chagin <dchagin@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _SYS_EPOLL_H_
#define	_SYS_EPOLL_H_

#include <sys/fcntl.h>			/* for O_CLOEXEC */
#include <sys/featuretest.h>
#include <sys/sigtypes.h>		/* for sigset_t */
#include <sys/types.h>			/* for uint32_t, uint64_t */

struct timespec;

#define	EPOLL_CLOEXEC	O_CLOEXEC

#define	EPOLLIN		0x00000001
#define	EPOLLPRI	0x00000002
#define	EPOLLOUT	0x00000004
#define	EPOLLERR	0x00000008
#define	EPOLLHUP	0x00000010
#define	EPOLLRDNORM	0x00000040
#define	EPOLLRDBAND	0x00000080
#define	EPOLLWRNORM	0x00000100
#define	EPOLLWRBAND	0x00000200
#define	EPOLLMSG	0x00000400
#define	EPOLLRDHUP	0x00002000
#define	EPOLLWAKEUP	0x20000000
#define	EPOLLONESHOT	0x40000000
#define	EPOLLET		0x80000000

#define	EPOLL_CTL_ADD	1
#define	EPOLL_CTL_DEL	2
#define	EPOLL_CTL_MOD	3

#ifdef _KERNEL
#define	EPOLL_MAX_EVENTS	(4 * 1024 * 1024)
typedef uint64_t		epoll_data_t;
#else
union epoll_data {
	void		*ptr;
	int		fd;
	uint32_t	u32;
	uint64_t	u64;
};

typedef union epoll_data	epoll_data_t;
#endif

struct epoll_event {
	uint32_t	events;
	epoll_data_t	data;
};

#ifdef _KERNEL
int	epoll_ctl_common(struct lwp *l, register_t *retval, int epfd, int op,
	    int fd, struct epoll_event *event);
int	epoll_wait_common(struct lwp *l, register_t *retval, int epfd,
	    struct epoll_event *events, int maxevents, struct timespec *tsp,
	    const sigset_t *nss);
#else	/* !_KERNEL */
__BEGIN_DECLS
#ifdef _NETBSD_SOURCE
int	epoll_create(int size);
int	epoll_create1(int flags);
int	epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int	epoll_wait(int epfd, struct epoll_event *events, int maxevents,
	    int timeout);
int	epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
	    int timeout, const sigset_t *sigmask);
int	epoll_pwait2(int epfd, struct epoll_event *events, int maxevents,
	    const struct timespec *timeout, const sigset_t *sigmask);
#endif	/* _NETBSD_SOURCE */
__END_DECLS
#endif	/* !_KERNEL */

#endif	/* !_SYS_EPOLL_H_ */
