/*	$NetBSD: socket.c,v 1.25 2023/01/25 21:43:31 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <inttypes.h>
#include <stdbool.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(HAVE_SYS_SYSCTL_H) && !defined(__linux__)
#include <sys/sysctl.h>
#endif /* if defined(HAVE_SYS_SYSCTL_H) && !defined(__linux__) */
#include <sys/time.h>
#include <sys/uio.h>

#if defined(HAVE_LINUX_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H)
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif /* if defined(HAVE_LINUX_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H) \
	*/

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include <isc/app.h>
#include <isc/buffer.h>
#include <isc/condition.h>
#include <isc/formatcheck.h>
#include <isc/list.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/net.h>
#include <isc/once.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/refcount.h>
#include <isc/region.h>
#include <isc/resource.h>
#include <isc/socket.h>
#include <isc/stats.h>
#include <isc/strerr.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/util.h>

#ifdef ISC_PLATFORM_HAVESYSUNH
#include <sys/un.h>
#endif /* ifdef ISC_PLATFORM_HAVESYSUNH */
#ifdef HAVE_KQUEUE
#include <sys/event.h>
#endif /* ifdef HAVE_KQUEUE */
#ifdef HAVE_EPOLL_CREATE1
#include <sys/epoll.h>
#endif /* ifdef HAVE_EPOLL_CREATE1 */
#if defined(HAVE_SYS_DEVPOLL_H)
#include <sys/devpoll.h>
#elif defined(HAVE_DEVPOLL_H)
#include <devpoll.h>
#endif /* if defined(HAVE_SYS_DEVPOLL_H) */

#include <netinet/tcp.h>

#include "errno2result.h"

#ifdef ENABLE_TCP_FASTOPEN
#include <netinet/tcp.h>
#endif /* ifdef ENABLE_TCP_FASTOPEN */

#ifdef HAVE_JSON_C
#include <json_object.h>
#endif /* HAVE_JSON_C */

#ifdef HAVE_LIBXML2
#include <libxml/xmlwriter.h>
#define ISC_XMLCHAR (const xmlChar *)
#endif /* HAVE_LIBXML2 */

/*%
 * Choose the most preferable multiplex method.
 */
#if defined(HAVE_KQUEUE)
#define USE_KQUEUE
#elif defined(HAVE_EPOLL_CREATE1)
#define USE_EPOLL
#elif defined(HAVE_SYS_DEVPOLL_H) || defined(HAVE_DEVPOLL_H)
#define USE_DEVPOLL
typedef struct {
	unsigned int want_read : 1, want_write : 1;
} pollinfo_t;
#else /* if defined(HAVE_KQUEUE) */
#define USE_SELECT
#endif /* HAVE_KQUEUE */

/*
 * Set by the -T dscp option on the command line. If set to a value
 * other than -1, we check to make sure DSCP values match it, and
 * assert if not.
 */
int isc_dscp_check_value = -1;

/*%
 * Maximum number of allowable open sockets.  This is also the maximum
 * allowable socket file descriptor.
 *
 * Care should be taken before modifying this value for select():
 * The API standard doesn't ensure select() accept more than (the system default
 * of) FD_SETSIZE descriptors, and the default size should in fact be fine in
 * the vast majority of cases.  This constant should therefore be increased only
 * when absolutely necessary and possible, i.e., the server is exhausting all
 * available file descriptors (up to FD_SETSIZE) and the select() function
 * and FD_xxx macros support larger values than FD_SETSIZE (which may not
 * always by true, but we keep using some of them to ensure as much
 * portability as possible).  Note also that overall server performance
 * may be rather worsened with a larger value of this constant due to
 * inherent scalability problems of select().
 *
 * As a special note, this value shouldn't have to be touched if
 * this is a build for an authoritative only DNS server.
 */
#ifndef ISC_SOCKET_MAXSOCKETS
#if defined(USE_KQUEUE) || defined(USE_EPOLL) || defined(USE_DEVPOLL)
#ifdef TUNE_LARGE
#define ISC_SOCKET_MAXSOCKETS 21000
#else /* ifdef TUNE_LARGE */
#define ISC_SOCKET_MAXSOCKETS 4096
#endif /* TUNE_LARGE */
#elif defined(USE_SELECT)
#define ISC_SOCKET_MAXSOCKETS FD_SETSIZE
#endif /* USE_KQUEUE... */
#endif /* ISC_SOCKET_MAXSOCKETS */

#ifdef USE_SELECT
/*%
 * Mac OS X needs a special definition to support larger values in select().
 * We always define this because a larger value can be specified run-time.
 */
#ifdef __APPLE__
#define _DARWIN_UNLIMITED_SELECT
#endif /* __APPLE__ */
#endif /* USE_SELECT */

#ifdef ISC_SOCKET_USE_POLLWATCH
/*%
 * If this macro is defined, enable workaround for a Solaris /dev/poll kernel
 * bug: DP_POLL ioctl could keep sleeping even if socket I/O is possible for
 * some of the specified FD.  The idea is based on the observation that it's
 * likely for a busy server to keep receiving packets.  It specifically works
 * as follows: the socket watcher is first initialized with the state of
 * "poll_idle".  While it's in the idle state it keeps sleeping until a socket
 * event occurs.  When it wakes up for a socket I/O event, it moves to the
 * poll_active state, and sets the poll timeout to a short period
 * (ISC_SOCKET_POLLWATCH_TIMEOUT msec).  If timeout occurs in this state, the
 * watcher goes to the poll_checking state with the same timeout period.
 * In this state, the watcher tries to detect whether this is a break
 * during intermittent events or the kernel bug is triggered.  If the next
 * polling reports an event within the short period, the previous timeout is
 * likely to be a kernel bug, and so the watcher goes back to the active state.
 * Otherwise, it moves to the idle state again.
 *
 * It's not clear whether this is a thread-related bug, but since we've only
 * seen this with threads, this workaround is used only when enabling threads.
 */

typedef enum { poll_idle, poll_active, poll_checking } pollstate_t;

#ifndef ISC_SOCKET_POLLWATCH_TIMEOUT
#define ISC_SOCKET_POLLWATCH_TIMEOUT 10
#endif /* ISC_SOCKET_POLLWATCH_TIMEOUT */
#endif /* ISC_SOCKET_USE_POLLWATCH */

/*%
 * Per-FD lock buckets, we shuffle them around a bit as FDs come in herds.
 */
#define FDLOCK_BITS  10
#define FDLOCK_COUNT (1 << FDLOCK_BITS)
#define FDLOCK_ID(fd)                                   \
	(((fd) % (FDLOCK_COUNT) >> (FDLOCK_BITS / 2)) | \
	 (((fd) << (FDLOCK_BITS / 2)) % (FDLOCK_COUNT)))

/*%
 * Maximum number of events communicated with the kernel.  There should normally
 * be no need for having a large number.
 */
#if defined(USE_KQUEUE) || defined(USE_EPOLL) || defined(USE_DEVPOLL)
#ifndef ISC_SOCKET_MAXEVENTS
#ifdef TUNE_LARGE
#define ISC_SOCKET_MAXEVENTS 2048
#else /* ifdef TUNE_LARGE */
#define ISC_SOCKET_MAXEVENTS 64
#endif /* TUNE_LARGE */
#endif /* ifndef ISC_SOCKET_MAXEVENTS */
#endif /* if defined(USE_KQUEUE) || defined(USE_EPOLL) || defined(USE_DEVPOLL) \
	* */

/*%
 * Some systems define the socket length argument as an int, some as size_t,
 * some as socklen_t.  This is here so it can be easily changed if needed.
 */
#ifndef socklen_t
#define socklen_t unsigned int
#endif /* ifndef socklen_t */

/*%
 * Define what the possible "soft" errors can be.  These are non-fatal returns
 * of various network related functions, like recv() and so on.
 *
 * For some reason, BSDI (and perhaps others) will sometimes return <0
 * from recv() but will have errno==0.  This is broken, but we have to
 * work around it here.
 */
#define SOFT_ERROR(e)                                             \
	((e) == EAGAIN || (e) == EWOULDBLOCK || (e) == ENOBUFS || \
	 (e) == EINTR || (e) == 0)

#define DLVL(x) ISC_LOGCATEGORY_GENERAL, ISC_LOGMODULE_SOCKET, ISC_LOG_DEBUG(x)

/*!<
 * DLVL(90)  --  Function entry/exit and other tracing.
 * DLVL(70)  --  Socket "correctness" -- including returning of events, etc.
 * DLVL(60)  --  Socket data send/receive
 * DLVL(50)  --  Event tracing, including receiving/sending completion events.
 * DLVL(20)  --  Socket creation/destruction.
 */
#define TRACE_LEVEL	  90
#define CORRECTNESS_LEVEL 70
#define IOEVENT_LEVEL	  60
#define EVENT_LEVEL	  50
#define CREATION_LEVEL	  20

#define TRACE	    DLVL(TRACE_LEVEL)
#define CORRECTNESS DLVL(CORRECTNESS_LEVEL)
#define IOEVENT	    DLVL(IOEVENT_LEVEL)
#define EVENT	    DLVL(EVENT_LEVEL)
#define CREATION    DLVL(CREATION_LEVEL)

typedef isc_event_t intev_t;

#define SOCKET_MAGIC	ISC_MAGIC('I', 'O', 'i', 'o')
#define VALID_SOCKET(s) ISC_MAGIC_VALID(s, SOCKET_MAGIC)

/*!
 * IPv6 control information.  If the socket is an IPv6 socket we want
 * to collect the destination address and interface so the client can
 * set them on outgoing packets.
 */
#ifndef USE_CMSG
#define USE_CMSG 1
#endif /* ifndef USE_CMSG */

/*%
 * NetBSD and FreeBSD can timestamp packets.  XXXMLG Should we have
 * a setsockopt() like interface to request timestamps, and if the OS
 * doesn't do it for us, call gettimeofday() on every UDP receive?
 */
#ifdef SO_TIMESTAMP
#ifndef USE_CMSG
#define USE_CMSG 1
#endif /* ifndef USE_CMSG */
#endif /* ifdef SO_TIMESTAMP */

#if defined(SO_RCVBUF) && defined(ISC_RECV_BUFFER_SIZE)
#define SET_RCVBUF
#endif

#if defined(SO_SNDBUF) && defined(ISC_SEND_BUFFER_SIZE)
#define SET_SNDBUF
#endif

/*%
 * Instead of calculating the cmsgbuf lengths every time we take
 * a rule of thumb approach - sizes are taken from x86_64 linux,
 * multiplied by 2, everything should fit. Those sizes are not
 * large enough to cause any concern.
 */
#if defined(USE_CMSG)
#define CMSG_SP_IN6PKT 40
#else /* if defined(USE_CMSG) */
#define CMSG_SP_IN6PKT 0
#endif /* if defined(USE_CMSG) */

#if defined(USE_CMSG) && defined(SO_TIMESTAMP)
#define CMSG_SP_TIMESTAMP 32
#else /* if defined(USE_CMSG) && defined(SO_TIMESTAMP) */
#define CMSG_SP_TIMESTAMP 0
#endif /* if defined(USE_CMSG) && defined(SO_TIMESTAMP) */

#if defined(USE_CMSG) && (defined(IPV6_TCLASS) || defined(IP_TOS))
#define CMSG_SP_TCTOS 24
#else /* if defined(USE_CMSG) && (defined(IPV6_TCLASS) || defined(IP_TOS)) */
#define CMSG_SP_TCTOS 0
#endif /* if defined(USE_CMSG) && (defined(IPV6_TCLASS) || defined(IP_TOS)) */

#define CMSG_SP_INT 24

/* Align cmsg buffers to be safe on SPARC etc. */
#define RECVCMSGBUFLEN                                                       \
	ISC_ALIGN(2 * (CMSG_SP_IN6PKT + CMSG_SP_TIMESTAMP + CMSG_SP_TCTOS) + \
			  1,                                                 \
		  sizeof(void *))
#define SENDCMSGBUFLEN                                                    \
	ISC_ALIGN(2 * (CMSG_SP_IN6PKT + CMSG_SP_INT + CMSG_SP_TCTOS) + 1, \
		  sizeof(void *))

/*%
 * The number of times a send operation is repeated if the result is EINTR.
 */
#define NRETRIES 10

typedef struct isc__socketthread isc__socketthread_t;

#define NEWCONNSOCK(ev) ((ev)->newsocket)

struct isc_socket {
	/* Not locked. */
	unsigned int magic;
	isc_socketmgr_t *manager;
	isc_mutex_t lock;
	isc_sockettype_t type;
	const isc_statscounter_t *statsindex;
	isc_refcount_t references;

	/* Locked by socket lock. */
	ISC_LINK(isc_socket_t) link;
	int fd;
	int pf;
	int threadid;
	char name[16];
	void *tag;

	ISC_LIST(isc_socketevent_t) send_list;
	ISC_LIST(isc_socketevent_t) recv_list;
	ISC_LIST(isc_socket_newconnev_t) accept_list;
	ISC_LIST(isc_socket_connev_t) connect_list;

	isc_sockaddr_t peer_address; /* remote address */

	unsigned int listener : 1,	       /* listener socket */
		connected : 1, connecting : 1, /* connect pending
						* */
		bound  : 1,		       /* bound to local addr */
		dupped : 1, active : 1,	       /* currently active */
		pktdscp : 1;		       /* per packet dscp */

#ifdef ISC_PLATFORM_RECVOVERFLOW
	unsigned char overflow; /* used for MSG_TRUNC fake */
#endif				/* ifdef ISC_PLATFORM_RECVOVERFLOW */

	void			*fdwatcharg;
	isc_sockfdwatch_t	fdwatchcb;
	int			fdwatchflags;
	isc_task_t              *fdwatchtask;
	unsigned int		dscp;
};

#define SOCKET_MANAGER_MAGIC ISC_MAGIC('I', 'O', 'm', 'g')
#define VALID_MANAGER(m)     ISC_MAGIC_VALID(m, SOCKET_MANAGER_MAGIC)

struct isc_socketmgr {
	/* Not locked. */
	unsigned int magic;
	isc_mem_t *mctx;
	isc_mutex_t lock;
	isc_stats_t *stats;
	int nthreads;
	isc__socketthread_t *threads;
	unsigned int maxsocks;
	/* Locked by manager lock. */
	ISC_LIST(isc_socket_t) socklist;
	int reserved; /* unlocked */
	isc_condition_t shutdown_ok;
	size_t maxudp;
};

struct isc__socketthread {
	isc_socketmgr_t *manager;
	int threadid;
	isc_thread_t thread;
	int pipe_fds[2];
	isc_mutex_t *fdlock;
	/* Locked by fdlock. */
	isc_socket_t **fds;
	int *fdstate;
#ifdef USE_KQUEUE
	int kqueue_fd;
	int nevents;
	struct kevent *events;
#endif /* USE_KQUEUE */
#ifdef USE_EPOLL
	int epoll_fd;
	int nevents;
	struct epoll_event *events;
	uint32_t *epoll_events;
#endif /* USE_EPOLL */
#ifdef USE_DEVPOLL
	int devpoll_fd;
	isc_resourcevalue_t open_max;
	unsigned int calls;
	int nevents;
	struct pollfd *events;
	pollinfo_t *fdpollinfo;
#endif /* USE_DEVPOLL */
#ifdef USE_SELECT
	int fd_bufsize;
	fd_set *read_fds;
	fd_set *read_fds_copy;
	fd_set *write_fds;
	fd_set *write_fds_copy;
	int maxfd;
#endif /* USE_SELECT */
};

#define CLOSED	      0 /* this one must be zero */
#define MANAGED	      1
#define CLOSE_PENDING 2

/*
 * send() and recv() iovec counts
 */
#define MAXSCATTERGATHER_SEND (ISC_SOCKET_MAXSCATTERGATHER)
#ifdef ISC_PLATFORM_RECVOVERFLOW
#define MAXSCATTERGATHER_RECV (ISC_SOCKET_MAXSCATTERGATHER + 1)
#else /* ifdef ISC_PLATFORM_RECVOVERFLOW */
#define MAXSCATTERGATHER_RECV (ISC_SOCKET_MAXSCATTERGATHER)
#endif /* ifdef ISC_PLATFORM_RECVOVERFLOW */

static isc_result_t
socket_create(isc_socketmgr_t *manager0, int pf, isc_sockettype_t type,
	      isc_socket_t **socketp, isc_socket_t *dup_socket);
static void
send_recvdone_event(isc_socket_t *, isc_socketevent_t **);
static void
send_senddone_event(isc_socket_t *, isc_socketevent_t **);
static void
send_connectdone_event(isc_socket_t *, isc_socket_connev_t **);
static void
free_socket(isc_socket_t **);
static isc_result_t
allocate_socket(isc_socketmgr_t *, isc_sockettype_t, isc_socket_t **);
static void
destroy(isc_socket_t **);
static void
internal_accept(isc_socket_t *);
static void
internal_connect(isc_socket_t *);
static void
internal_recv(isc_socket_t *);
static void
internal_send(isc_socket_t *);
static void
process_cmsg(isc_socket_t *, struct msghdr *, isc_socketevent_t *);
static void
build_msghdr_send(isc_socket_t *, char *, isc_socketevent_t *, struct msghdr *,
		  struct iovec *, size_t *);
static void
build_msghdr_recv(isc_socket_t *, char *, isc_socketevent_t *, struct msghdr *,
		  struct iovec *, size_t *);
static bool
process_ctlfd(isc__socketthread_t *thread);
static void
setdscp(isc_socket_t *sock, isc_dscp_t dscp);
static void
dispatch_recv(isc_socket_t *sock);
static void
dispatch_send(isc_socket_t *sock);
static void
internal_fdwatch_read(isc_socket_t *sock);
static void
internal_fdwatch_write(isc_socket_t *sock);

#define SELECT_POKE_SHUTDOWN (-1)
#define SELECT_POKE_NOTHING  (-2)
#define SELECT_POKE_READ     (-3)
#define SELECT_POKE_ACCEPT   (-3) /*%< Same as _READ */
#define SELECT_POKE_WRITE    (-4)
#define SELECT_POKE_CONNECT  (-4) /*%< Same as _WRITE */
#define SELECT_POKE_CLOSE    (-5)

/*%
 * Shortcut index arrays to get access to statistics counters.
 */
enum {
	STATID_OPEN = 0,
	STATID_OPENFAIL = 1,
	STATID_CLOSE = 2,
	STATID_BINDFAIL = 3,
	STATID_CONNECTFAIL = 4,
	STATID_CONNECT = 5,
	STATID_ACCEPTFAIL = 6,
	STATID_ACCEPT = 7,
	STATID_SENDFAIL = 8,
	STATID_RECVFAIL = 9,
	STATID_ACTIVE = 10
};
static const isc_statscounter_t udp4statsindex[] = {
	isc_sockstatscounter_udp4open,
	isc_sockstatscounter_udp4openfail,
	isc_sockstatscounter_udp4close,
	isc_sockstatscounter_udp4bindfail,
	isc_sockstatscounter_udp4connectfail,
	isc_sockstatscounter_udp4connect,
	-1,
	-1,
	isc_sockstatscounter_udp4sendfail,
	isc_sockstatscounter_udp4recvfail,
	isc_sockstatscounter_udp4active
};
static const isc_statscounter_t udp6statsindex[] = {
	isc_sockstatscounter_udp6open,
	isc_sockstatscounter_udp6openfail,
	isc_sockstatscounter_udp6close,
	isc_sockstatscounter_udp6bindfail,
	isc_sockstatscounter_udp6connectfail,
	isc_sockstatscounter_udp6connect,
	-1,
	-1,
	isc_sockstatscounter_udp6sendfail,
	isc_sockstatscounter_udp6recvfail,
	isc_sockstatscounter_udp6active
};
static const isc_statscounter_t tcp4statsindex[] = {
	isc_sockstatscounter_tcp4open,	      isc_sockstatscounter_tcp4openfail,
	isc_sockstatscounter_tcp4close,	      isc_sockstatscounter_tcp4bindfail,
	isc_sockstatscounter_tcp4connectfail, isc_sockstatscounter_tcp4connect,
	isc_sockstatscounter_tcp4acceptfail,  isc_sockstatscounter_tcp4accept,
	isc_sockstatscounter_tcp4sendfail,    isc_sockstatscounter_tcp4recvfail,
	isc_sockstatscounter_tcp4active
};
static const isc_statscounter_t tcp6statsindex[] = {
	isc_sockstatscounter_tcp6open,	      isc_sockstatscounter_tcp6openfail,
	isc_sockstatscounter_tcp6close,	      isc_sockstatscounter_tcp6bindfail,
	isc_sockstatscounter_tcp6connectfail, isc_sockstatscounter_tcp6connect,
	isc_sockstatscounter_tcp6acceptfail,  isc_sockstatscounter_tcp6accept,
	isc_sockstatscounter_tcp6sendfail,    isc_sockstatscounter_tcp6recvfail,
	isc_sockstatscounter_tcp6active
};
static const isc_statscounter_t unixstatsindex[] = {
	isc_sockstatscounter_unixopen,	      isc_sockstatscounter_unixopenfail,
	isc_sockstatscounter_unixclose,	      isc_sockstatscounter_unixbindfail,
	isc_sockstatscounter_unixconnectfail, isc_sockstatscounter_unixconnect,
	isc_sockstatscounter_unixacceptfail,  isc_sockstatscounter_unixaccept,
	isc_sockstatscounter_unixsendfail,    isc_sockstatscounter_unixrecvfail,
	isc_sockstatscounter_unixactive
};
static const isc_statscounter_t rawstatsindex[] = {
	isc_sockstatscounter_rawopen,
	isc_sockstatscounter_rawopenfail,
	isc_sockstatscounter_rawclose,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	isc_sockstatscounter_rawrecvfail,
	isc_sockstatscounter_rawactive
};

static int
gen_threadid(isc_socket_t *sock);

static int
gen_threadid(isc_socket_t *sock) {
	return (sock->fd % sock->manager->nthreads);
}

static void
manager_log(isc_socketmgr_t *sockmgr, isc_logcategory_t *category,
	    isc_logmodule_t *module, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(5, 6);
static void
manager_log(isc_socketmgr_t *sockmgr, isc_logcategory_t *category,
	    isc_logmodule_t *module, int level, const char *fmt, ...) {
	char msgbuf[2048];
	va_list ap;

	if (!isc_log_wouldlog(isc_lctx, level)) {
		return;
	}

	va_start(ap, fmt);
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	va_end(ap);

	isc_log_write(isc_lctx, category, module, level, "sockmgr %p: %s",
		      sockmgr, msgbuf);
}

static void
thread_log(isc__socketthread_t *thread, isc_logcategory_t *category,
	   isc_logmodule_t *module, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(5, 6);
static void
thread_log(isc__socketthread_t *thread, isc_logcategory_t *category,
	   isc_logmodule_t *module, int level, const char *fmt, ...) {
	char msgbuf[2048];
	va_list ap;

	if (!isc_log_wouldlog(isc_lctx, level)) {
		return;
	}

	va_start(ap, fmt);
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	va_end(ap);

	isc_log_write(isc_lctx, category, module, level,
		      "sockmgr %p thread %d: %s", thread->manager,
		      thread->threadid, msgbuf);
}

static void
socket_log(isc_socket_t *sock, const isc_sockaddr_t *address,
	   isc_logcategory_t *category, isc_logmodule_t *module, int level,
	   const char *fmt, ...) ISC_FORMAT_PRINTF(6, 7);
static void
socket_log(isc_socket_t *sock, const isc_sockaddr_t *address,
	   isc_logcategory_t *category, isc_logmodule_t *module, int level,
	   const char *fmt, ...) {
	char msgbuf[2048];
	char peerbuf[ISC_SOCKADDR_FORMATSIZE];
	va_list ap;

	if (!isc_log_wouldlog(isc_lctx, level)) {
		return;
	}

	va_start(ap, fmt);
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	va_end(ap);

	if (address == NULL) {
		isc_log_write(isc_lctx, category, module, level,
			      "socket %p: %s", sock, msgbuf);
	} else {
		isc_sockaddr_format(address, peerbuf, sizeof(peerbuf));
		isc_log_write(isc_lctx, category, module, level,
			      "socket %p %s: %s", sock, peerbuf, msgbuf);
	}
}

/*%
 * Increment socket-related statistics counters.
 */
static void
inc_stats(isc_stats_t *stats, isc_statscounter_t counterid) {
	REQUIRE(counterid != -1);

	if (stats != NULL) {
		isc_stats_increment(stats, counterid);
	}
}

/*%
 * Decrement socket-related statistics counters.
 */
static void
dec_stats(isc_stats_t *stats, isc_statscounter_t counterid) {
	REQUIRE(counterid != -1);

	if (stats != NULL) {
		isc_stats_decrement(stats, counterid);
	}
}

static isc_result_t
watch_fd(isc__socketthread_t *thread, int fd, int msg) {
	isc_result_t result = ISC_R_SUCCESS;

#ifdef USE_KQUEUE
	struct kevent evchange;

	memset(&evchange, 0, sizeof(evchange));
	if (msg == SELECT_POKE_READ) {
		evchange.filter = EVFILT_READ;
	} else {
		evchange.filter = EVFILT_WRITE;
	}
	evchange.flags = EV_ADD;
	evchange.ident = fd;
	if (kevent(thread->kqueue_fd, &evchange, 1, NULL, 0, NULL) != 0) {
		result = isc__errno2result(errno);
	}

	return (result);
#elif defined(USE_EPOLL)
	struct epoll_event event;
	uint32_t oldevents;
	int ret;
	int op;

	oldevents = thread->epoll_events[fd];
	if (msg == SELECT_POKE_READ) {
		thread->epoll_events[fd] |= EPOLLIN;
	} else {
		thread->epoll_events[fd] |= EPOLLOUT;
	}

	event.events = thread->epoll_events[fd];
	memset(&event.data, 0, sizeof(event.data));
	event.data.fd = fd;

	op = (oldevents == 0U) ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
	if (thread->fds[fd] != NULL) {
		LOCK(&thread->fds[fd]->lock);
	}
	ret = epoll_ctl(thread->epoll_fd, op, fd, &event);
	if (thread->fds[fd] != NULL) {
		UNLOCK(&thread->fds[fd]->lock);
	}
	if (ret == -1) {
		if (errno == EEXIST) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "epoll_ctl(ADD/MOD) returned "
					 "EEXIST for fd %d",
					 fd);
		}
		result = isc__errno2result(errno);
	}

	return (result);
#elif defined(USE_DEVPOLL)
	struct pollfd pfd;

	memset(&pfd, 0, sizeof(pfd));
	if (msg == SELECT_POKE_READ) {
		pfd.events = POLLIN;
	} else {
		pfd.events = POLLOUT;
	}
	pfd.fd = fd;
	pfd.revents = 0;
	if (write(thread->devpoll_fd, &pfd, sizeof(pfd)) == -1) {
		result = isc__errno2result(errno);
	} else {
		if (msg == SELECT_POKE_READ) {
			thread->fdpollinfo[fd].want_read = 1;
		} else {
			thread->fdpollinfo[fd].want_write = 1;
		}
	}

	return (result);
#elif defined(USE_SELECT)
	LOCK(&thread->manager->lock);
	if (msg == SELECT_POKE_READ) {
		FD_SET(fd, thread->read_fds);
	}
	if (msg == SELECT_POKE_WRITE) {
		FD_SET(fd, thread->write_fds);
	}
	UNLOCK(&thread->manager->lock);

	return (result);
#endif /* ifdef USE_KQUEUE */
}

static isc_result_t
unwatch_fd(isc__socketthread_t *thread, int fd, int msg) {
	isc_result_t result = ISC_R_SUCCESS;

#ifdef USE_KQUEUE
	struct kevent evchange;

	memset(&evchange, 0, sizeof(evchange));
	if (msg == SELECT_POKE_READ) {
		evchange.filter = EVFILT_READ;
	} else {
		evchange.filter = EVFILT_WRITE;
	}
	evchange.flags = EV_DELETE;
	evchange.ident = fd;
	if (kevent(thread->kqueue_fd, &evchange, 1, NULL, 0, NULL) != 0) {
		result = isc__errno2result(errno);
	}

	return (result);
#elif defined(USE_EPOLL)
	struct epoll_event event;
	int ret;
	int op;

	if (msg == SELECT_POKE_READ) {
		thread->epoll_events[fd] &= ~(EPOLLIN);
	} else {
		thread->epoll_events[fd] &= ~(EPOLLOUT);
	}

	event.events = thread->epoll_events[fd];
	memset(&event.data, 0, sizeof(event.data));
	event.data.fd = fd;

	op = (event.events == 0U) ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
	ret = epoll_ctl(thread->epoll_fd, op, fd, &event);
	if (ret == -1 && errno != ENOENT) {
		char strbuf[ISC_STRERRORSIZE];
		strerror_r(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__, "epoll_ctl(DEL), %d: %s",
				 fd, strbuf);
		result = ISC_R_UNEXPECTED;
	}
	return (result);
#elif defined(USE_DEVPOLL)
	struct pollfd pfds[2];
	size_t writelen = sizeof(pfds[0]);

	memset(pfds, 0, sizeof(pfds));
	pfds[0].events = POLLREMOVE;
	pfds[0].fd = fd;

	/*
	 * Canceling read or write polling via /dev/poll is tricky.  Since it
	 * only provides a way of canceling per FD, we may need to re-poll the
	 * socket for the other operation.
	 */
	if (msg == SELECT_POKE_READ && thread->fdpollinfo[fd].want_write == 1) {
		pfds[1].events = POLLOUT;
		pfds[1].fd = fd;
		writelen += sizeof(pfds[1]);
	}
	if (msg == SELECT_POKE_WRITE && thread->fdpollinfo[fd].want_read == 1) {
		pfds[1].events = POLLIN;
		pfds[1].fd = fd;
		writelen += sizeof(pfds[1]);
	}

	if (write(thread->devpoll_fd, pfds, writelen) == -1) {
		result = isc__errno2result(errno);
	} else {
		if (msg == SELECT_POKE_READ) {
			thread->fdpollinfo[fd].want_read = 0;
		} else {
			thread->fdpollinfo[fd].want_write = 0;
		}
	}

	return (result);
#elif defined(USE_SELECT)
	LOCK(&thread->manager->lock);
	if (msg == SELECT_POKE_READ) {
		FD_CLR(fd, thread->read_fds);
	} else if (msg == SELECT_POKE_WRITE) {
		FD_CLR(fd, thread->write_fds);
	}
	UNLOCK(&thread->manager->lock);

	return (result);
#endif /* ifdef USE_KQUEUE */
}

/*
 * A poke message was received, perform a proper watch/unwatch
 * on a fd provided
 */
static void
wakeup_socket(isc__socketthread_t *thread, int fd, int msg) {
	isc_result_t result;
	int lockid = FDLOCK_ID(fd);

	/*
	 * This is a wakeup on a socket.  If the socket is not in the
	 * process of being closed, start watching it for either reads
	 * or writes.
	 */

	INSIST(fd >= 0 && fd < (int)thread->manager->maxsocks);

	if (msg == SELECT_POKE_CLOSE) {
		LOCK(&thread->fdlock[lockid]);
		INSIST(thread->fdstate[fd] == CLOSE_PENDING);
		thread->fdstate[fd] = CLOSED;
		(void)unwatch_fd(thread, fd, SELECT_POKE_READ);
		(void)unwatch_fd(thread, fd, SELECT_POKE_WRITE);
		(void)close(fd);
		UNLOCK(&thread->fdlock[lockid]);
		return;
	}

	LOCK(&thread->fdlock[lockid]);
	if (thread->fdstate[fd] == CLOSE_PENDING) {
		/*
		 * We accept (and ignore) any error from unwatch_fd() as we are
		 * closing the socket, hoping it doesn't leave dangling state in
		 * the kernel.
		 * Note that unwatch_fd() must be called after releasing the
		 * fdlock; otherwise it could cause deadlock due to a lock order
		 * reversal.
		 */
		(void)unwatch_fd(thread, fd, SELECT_POKE_READ);
		(void)unwatch_fd(thread, fd, SELECT_POKE_WRITE);
		UNLOCK(&thread->fdlock[lockid]);
		return;
	}
	if (thread->fdstate[fd] != MANAGED) {
		UNLOCK(&thread->fdlock[lockid]);
		return;
	}

	/*
	 * Set requested bit.
	 */
	result = watch_fd(thread, fd, msg);
	if (result != ISC_R_SUCCESS) {
		/*
		 * XXXJT: what should we do?  Ignoring the failure of watching
		 * a socket will make the application dysfunctional, but there
		 * seems to be no reasonable recovery process.
		 */
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_SOCKET, ISC_LOG_ERROR,
			      "failed to start watching FD (%d): %s", fd,
			      isc_result_totext(result));
	}
	UNLOCK(&thread->fdlock[lockid]);
}

/*
 * Poke the select loop when there is something for us to do.
 * The write is required (by POSIX) to complete.  That is, we
 * will not get partial writes.
 */
static void
select_poke(isc_socketmgr_t *mgr, int threadid, int fd, int msg) {
	int cc;
	int buf[2];
	char strbuf[ISC_STRERRORSIZE];

	buf[0] = fd;
	buf[1] = msg;

	do {
		cc = write(mgr->threads[threadid].pipe_fds[1], buf,
			   sizeof(buf));
#ifdef ENOSR
		/*
		 * Treat ENOSR as EAGAIN but loop slowly as it is
		 * unlikely to clear fast.
		 */
		if (cc < 0 && errno == ENOSR) {
			sleep(1);
			errno = EAGAIN;
		}
#endif /* ifdef ENOSR */
	} while (cc < 0 && SOFT_ERROR(errno));

	if (cc < 0) {
		strerror_r(errno, strbuf, sizeof(strbuf));
		FATAL_ERROR(__FILE__, __LINE__,
			    "write() failed during watcher poke: %s", strbuf);
	}

	INSIST(cc == sizeof(buf));
}

/*
 * Read a message on the internal fd.
 */
static void
select_readmsg(isc__socketthread_t *thread, int *fd, int *msg) {
	int buf[2];
	int cc;
	char strbuf[ISC_STRERRORSIZE];

	cc = read(thread->pipe_fds[0], buf, sizeof(buf));
	if (cc < 0) {
		*msg = SELECT_POKE_NOTHING;
		*fd = -1; /* Silence compiler. */
		if (SOFT_ERROR(errno)) {
			return;
		}

		strerror_r(errno, strbuf, sizeof(strbuf));
		FATAL_ERROR(__FILE__, __LINE__,
			    "read() failed during watcher poke: %s", strbuf);
	}
	INSIST(cc == sizeof(buf));

	*fd = buf[0];
	*msg = buf[1];
}

/*
 * Make a fd non-blocking.
 */
static isc_result_t
make_nonblock(int fd) {
	int ret;
	char strbuf[ISC_STRERRORSIZE];
#ifdef USE_FIONBIO_IOCTL
	int on = 1;
#else  /* ifdef USE_FIONBIO_IOCTL */
	int flags;
#endif /* ifdef USE_FIONBIO_IOCTL */

#ifdef USE_FIONBIO_IOCTL
	ret = ioctl(fd, FIONBIO, (char *)&on);
#else  /* ifdef USE_FIONBIO_IOCTL */
	flags = fcntl(fd, F_GETFL, 0);
	flags |= PORT_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
#endif /* ifdef USE_FIONBIO_IOCTL */

	if (ret == -1) {
		strerror_r(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
#ifdef USE_FIONBIO_IOCTL
				 "ioctl(%d, FIONBIO, &on): %s", fd,
#else  /* ifdef USE_FIONBIO_IOCTL */
				 "fcntl(%d, F_SETFL, %d): %s", fd, flags,
#endif /* ifdef USE_FIONBIO_IOCTL */
				 strbuf);

		return (ISC_R_UNEXPECTED);
	}

	return (ISC_R_SUCCESS);
}

#ifdef USE_CMSG
/*
 * Not all OSes support advanced CMSG macros: CMSG_LEN and CMSG_SPACE.
 * In order to ensure as much portability as possible, we provide wrapper
 * functions of these macros.
 * Note that cmsg_space() could run slow on OSes that do not have
 * CMSG_SPACE.
 */
static socklen_t
cmsg_len(socklen_t len) {
#ifdef CMSG_LEN
	return (CMSG_LEN(len));
#else  /* ifdef CMSG_LEN */
	socklen_t hdrlen;

	/*
	 * Cast NULL so that any pointer arithmetic performed by CMSG_DATA
	 * is correct.
	 */
	hdrlen = (socklen_t)CMSG_DATA(((struct cmsghdr *)NULL));
	return (hdrlen + len);
#endif /* ifdef CMSG_LEN */
}

static socklen_t
cmsg_space(socklen_t len) {
#ifdef CMSG_SPACE
	return (CMSG_SPACE(len));
#else  /* ifdef CMSG_SPACE */
	struct msghdr msg;
	struct cmsghdr *cmsgp;
	/*
	 * XXX: The buffer length is an ad-hoc value, but should be enough
	 * in a practical sense.
	 */
	char dummybuf[sizeof(struct cmsghdr) + 1024];

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = dummybuf;
	msg.msg_controllen = sizeof(dummybuf);

	cmsgp = (struct cmsghdr *)dummybuf;
	cmsgp->cmsg_len = cmsg_len(len);

	cmsgp = CMSG_NXTHDR(&msg, cmsgp);
	if (cmsgp != NULL) {
		return ((char *)cmsgp - (char *)msg.msg_control);
	} else {
		return (0);
	}
#endif /* ifdef CMSG_SPACE */
}
#endif /* USE_CMSG */

/*
 * Process control messages received on a socket.
 */
static void
process_cmsg(isc_socket_t *sock, struct msghdr *msg, isc_socketevent_t *dev) {
#ifdef USE_CMSG
	struct cmsghdr *cmsgp;
	struct in6_pktinfo *pktinfop;
#ifdef SO_TIMESTAMP
	void *timevalp;
#endif /* ifdef SO_TIMESTAMP */
#endif /* ifdef USE_CMSG */

	/*
	 * sock is used only when ISC_NET_BSD44MSGHDR and USE_CMSG are defined.
	 * msg and dev are used only when ISC_NET_BSD44MSGHDR is defined.
	 * They are all here, outside of the CPP tests, because it is
	 * more consistent with the usual ISC coding style.
	 */
	UNUSED(sock);
	UNUSED(msg);
	UNUSED(dev);

#ifdef MSG_TRUNC
	if ((msg->msg_flags & MSG_TRUNC) != 0) {
		dev->attributes |= ISC_SOCKEVENTATTR_TRUNC;
	}
#endif /* ifdef MSG_TRUNC */

#ifdef MSG_CTRUNC
	if ((msg->msg_flags & MSG_CTRUNC) != 0) {
		dev->attributes |= ISC_SOCKEVENTATTR_CTRUNC;
	}
#endif /* ifdef MSG_CTRUNC */

#ifndef USE_CMSG
	return;
#else /* ifndef USE_CMSG */
	if (msg->msg_controllen == 0U || msg->msg_control == NULL) {
		return;
	}

#ifdef SO_TIMESTAMP
	timevalp = NULL;
#endif /* ifdef SO_TIMESTAMP */
	pktinfop = NULL;

	cmsgp = CMSG_FIRSTHDR(msg);
	while (cmsgp != NULL) {
		socket_log(sock, NULL, TRACE, "processing cmsg %p", cmsgp);

		if (cmsgp->cmsg_level == IPPROTO_IPV6 &&
		    cmsgp->cmsg_type == IPV6_PKTINFO)
		{
			pktinfop = (struct in6_pktinfo *)CMSG_DATA(cmsgp);
			memmove(&dev->pktinfo, pktinfop,
				sizeof(struct in6_pktinfo));
			dev->attributes |= ISC_SOCKEVENTATTR_PKTINFO;
			socket_log(sock, NULL, TRACE,
				   "interface received on ifindex %u",
				   dev->pktinfo.ipi6_ifindex);
			if (IN6_IS_ADDR_MULTICAST(&pktinfop->ipi6_addr)) {
				dev->attributes |= ISC_SOCKEVENTATTR_MULTICAST;
			}
			goto next;
		}

#ifdef SO_TIMESTAMP
		if (cmsgp->cmsg_level == SOL_SOCKET &&
		    cmsgp->cmsg_type == SCM_TIMESTAMP)
		{
			struct timeval tv;
			timevalp = CMSG_DATA(cmsgp);
			memmove(&tv, timevalp, sizeof(tv));
			dev->timestamp.seconds = tv.tv_sec;
			dev->timestamp.nanoseconds = tv.tv_usec * 1000;
			dev->attributes |= ISC_SOCKEVENTATTR_TIMESTAMP;
			goto next;
		}
#endif /* ifdef SO_TIMESTAMP */

#ifdef IPV6_TCLASS
		if (cmsgp->cmsg_level == IPPROTO_IPV6 &&
		    cmsgp->cmsg_type == IPV6_TCLASS)
		{
			dev->dscp = *(int *)CMSG_DATA(cmsgp);
			dev->dscp >>= 2;
			dev->attributes |= ISC_SOCKEVENTATTR_DSCP;
			goto next;
		}
#endif /* ifdef IPV6_TCLASS */

#ifdef IP_TOS
		if (cmsgp->cmsg_level == IPPROTO_IP &&
		    (cmsgp->cmsg_type == IP_TOS
#ifdef IP_RECVTOS
		     || cmsgp->cmsg_type == IP_RECVTOS
#endif /* ifdef IP_RECVTOS */
		     ))
		{
			dev->dscp = (int)*(unsigned char *)CMSG_DATA(cmsgp);
			dev->dscp >>= 2;
			dev->attributes |= ISC_SOCKEVENTATTR_DSCP;
			goto next;
		}
#endif /* ifdef IP_TOS */
	next:
		cmsgp = CMSG_NXTHDR(msg, cmsgp);
	}
#endif /* USE_CMSG */
}

/*
 * Construct an iov array and attach it to the msghdr passed in.  This is
 * the SEND constructor, which will use the used region of the buffer
 * (if using a buffer list) or will use the internal region (if a single
 * buffer I/O is requested).
 *
 * Nothing can be NULL, and the done event must list at least one buffer
 * on the buffer linked list for this function to be meaningful.
 *
 * If write_countp != NULL, *write_countp will hold the number of bytes
 * this transaction can send.
 */
static void
build_msghdr_send(isc_socket_t *sock, char *cmsgbuf, isc_socketevent_t *dev,
		  struct msghdr *msg, struct iovec *iov, size_t *write_countp) {
	unsigned int iovcount;
	size_t write_count;
	struct cmsghdr *cmsgp;

	memset(msg, 0, sizeof(*msg));

	if (!sock->connected) {
		msg->msg_name = (void *)&dev->address.type.sa;
		msg->msg_namelen = dev->address.length;
	} else {
		msg->msg_name = NULL;
		msg->msg_namelen = 0;
	}

	write_count = dev->region.length - dev->n;
	iov[0].iov_base = (void *)(dev->region.base + dev->n);
	iov[0].iov_len = write_count;
	iovcount = 1;

	msg->msg_iov = iov;
	msg->msg_iovlen = iovcount;
	msg->msg_control = NULL;
	msg->msg_controllen = 0;
	msg->msg_flags = 0;
#if defined(USE_CMSG)

	if ((sock->type == isc_sockettype_udp) &&
	    ((dev->attributes & ISC_SOCKEVENTATTR_PKTINFO) != 0))
	{
		struct in6_pktinfo *pktinfop;

		socket_log(sock, NULL, TRACE, "sendto pktinfo data, ifindex %u",
			   dev->pktinfo.ipi6_ifindex);

		msg->msg_control = (void *)cmsgbuf;
		msg->msg_controllen = cmsg_space(sizeof(struct in6_pktinfo));
		INSIST(msg->msg_controllen <= SENDCMSGBUFLEN);

		cmsgp = (struct cmsghdr *)cmsgbuf;
		cmsgp->cmsg_level = IPPROTO_IPV6;
		cmsgp->cmsg_type = IPV6_PKTINFO;
		cmsgp->cmsg_len = cmsg_len(sizeof(struct in6_pktinfo));
		pktinfop = (struct in6_pktinfo *)CMSG_DATA(cmsgp);
		memmove(pktinfop, &dev->pktinfo, sizeof(struct in6_pktinfo));
	}

#if defined(IPV6_USE_MIN_MTU)
	if ((sock->type == isc_sockettype_udp) && (sock->pf == AF_INET6) &&
	    ((dev->attributes & ISC_SOCKEVENTATTR_USEMINMTU) != 0))
	{
		int use_min_mtu = 1; /* -1, 0, 1 */

		cmsgp = (struct cmsghdr *)(cmsgbuf + msg->msg_controllen);
		msg->msg_control = (void *)cmsgbuf;
		msg->msg_controllen += cmsg_space(sizeof(use_min_mtu));
		INSIST(msg->msg_controllen <= SENDCMSGBUFLEN);

		cmsgp->cmsg_level = IPPROTO_IPV6;
		cmsgp->cmsg_type = IPV6_USE_MIN_MTU;
		cmsgp->cmsg_len = cmsg_len(sizeof(use_min_mtu));
		memmove(CMSG_DATA(cmsgp), &use_min_mtu, sizeof(use_min_mtu));
	}
#endif /* if defined(IPV6_USE_MIN_MTU) */

	if (isc_dscp_check_value > -1) {
		if (sock->type == isc_sockettype_udp) {
			INSIST((int)dev->dscp == isc_dscp_check_value);
		} else if (sock->type == isc_sockettype_tcp) {
			INSIST((int)sock->dscp == isc_dscp_check_value);
		}
	}

#if defined(IP_TOS) || (defined(IPPROTO_IPV6) && defined(IPV6_TCLASS))
	if ((sock->type == isc_sockettype_udp) &&
	    ((dev->attributes & ISC_SOCKEVENTATTR_DSCP) != 0))
	{
		int dscp = (dev->dscp << 2) & 0xff;

		INSIST(dev->dscp < 0x40);

#ifdef IP_TOS
		if (sock->pf == AF_INET && sock->pktdscp) {
			cmsgp = (struct cmsghdr *)(cmsgbuf +
						   msg->msg_controllen);
			msg->msg_control = (void *)cmsgbuf;
			msg->msg_controllen += cmsg_space(sizeof(dscp));
			INSIST(msg->msg_controllen <= SENDCMSGBUFLEN);

			cmsgp->cmsg_level = IPPROTO_IP;
			cmsgp->cmsg_type = IP_TOS;
			cmsgp->cmsg_len = cmsg_len(sizeof(char));
			*(unsigned char *)CMSG_DATA(cmsgp) = dscp;
		} else if (sock->pf == AF_INET && sock->dscp != dev->dscp) {
			if (setsockopt(sock->fd, IPPROTO_IP, IP_TOS,
				       (void *)&dscp, sizeof(int)) < 0)
			{
				char strbuf[ISC_STRERRORSIZE];
				strerror_r(errno, strbuf, sizeof(strbuf));
				UNEXPECTED_ERROR(__FILE__, __LINE__,
						 "setsockopt(%d, IP_TOS, %.02x)"
						 " failed: %s",
						 sock->fd, dscp >> 2, strbuf);
			} else {
				sock->dscp = dscp;
			}
		}
#endif /* ifdef IP_TOS */
#if defined(IPPROTO_IPV6) && defined(IPV6_TCLASS)
		if (sock->pf == AF_INET6 && sock->pktdscp) {
			cmsgp = (struct cmsghdr *)(cmsgbuf +
						   msg->msg_controllen);
			msg->msg_control = (void *)cmsgbuf;
			msg->msg_controllen += cmsg_space(sizeof(dscp));
			INSIST(msg->msg_controllen <= SENDCMSGBUFLEN);

			cmsgp->cmsg_level = IPPROTO_IPV6;
			cmsgp->cmsg_type = IPV6_TCLASS;
			cmsgp->cmsg_len = cmsg_len(sizeof(dscp));
			memmove(CMSG_DATA(cmsgp), &dscp, sizeof(dscp));
		} else if (sock->pf == AF_INET6 && sock->dscp != dev->dscp) {
			if (setsockopt(sock->fd, IPPROTO_IPV6, IPV6_TCLASS,
				       (void *)&dscp, sizeof(int)) < 0)
			{
				char strbuf[ISC_STRERRORSIZE];
				strerror_r(errno, strbuf, sizeof(strbuf));
				UNEXPECTED_ERROR(__FILE__, __LINE__,
						 "setsockopt(%d, IPV6_TCLASS, "
						 "%.02x) failed: %s",
						 sock->fd, dscp >> 2, strbuf);
			} else {
				sock->dscp = dscp;
			}
		}
#endif /* if defined(IPPROTO_IPV6) && defined(IPV6_TCLASS) */
		if (msg->msg_controllen != 0 &&
		    msg->msg_controllen < SENDCMSGBUFLEN)
		{
			memset(cmsgbuf + msg->msg_controllen, 0,
			       SENDCMSGBUFLEN - msg->msg_controllen);
		}
	}
#endif /* if defined(IP_TOS) || (defined(IPPROTO_IPV6) && \
	* defined(IPV6_TCLASS))                           \
	* */
#endif /* USE_CMSG */

	if (write_countp != NULL) {
		*write_countp = write_count;
	}
}

/*
 * Construct an iov array and attach it to the msghdr passed in.  This is
 * the RECV constructor, which will use the available region of the buffer
 * (if using a buffer list) or will use the internal region (if a single
 * buffer I/O is requested).
 *
 * Nothing can be NULL, and the done event must list at least one buffer
 * on the buffer linked list for this function to be meaningful.
 *
 * If read_countp != NULL, *read_countp will hold the number of bytes
 * this transaction can receive.
 */
static void
build_msghdr_recv(isc_socket_t *sock, char *cmsgbuf, isc_socketevent_t *dev,
		  struct msghdr *msg, struct iovec *iov, size_t *read_countp) {
	unsigned int iovcount;
	size_t read_count;

	memset(msg, 0, sizeof(struct msghdr));

	if (sock->type == isc_sockettype_udp) {
		memset(&dev->address, 0, sizeof(dev->address));
		msg->msg_name = (void *)&dev->address.type.sa;
		msg->msg_namelen = sizeof(dev->address.type);
	} else { /* TCP */
		msg->msg_name = NULL;
		msg->msg_namelen = 0;
		dev->address = sock->peer_address;
	}

	read_count = dev->region.length - dev->n;
	iov[0].iov_base = (void *)(dev->region.base + dev->n);
	iov[0].iov_len = read_count;
	iovcount = 1;

	/*
	 * If needed, set up to receive that one extra byte.
	 */
#ifdef ISC_PLATFORM_RECVOVERFLOW
	if (sock->type == isc_sockettype_udp) {
		INSIST(iovcount < MAXSCATTERGATHER_RECV);
		iov[iovcount].iov_base = (void *)(&sock->overflow);
		iov[iovcount].iov_len = 1;
		iovcount++;
	}
#endif /* ifdef ISC_PLATFORM_RECVOVERFLOW */

	msg->msg_iov = iov;
	msg->msg_iovlen = iovcount;

#if defined(USE_CMSG)
	msg->msg_control = cmsgbuf;
	msg->msg_controllen = RECVCMSGBUFLEN;
#else  /* if defined(USE_CMSG) */
	msg->msg_control = NULL;
	msg->msg_controllen = 0;
#endif /* USE_CMSG */
	msg->msg_flags = 0;

	if (read_countp != NULL) {
		*read_countp = read_count;
	}
}

static void
set_dev_address(const isc_sockaddr_t *address, isc_socket_t *sock,
		isc_socketevent_t *dev) {
	if (sock->type == isc_sockettype_udp) {
		if (address != NULL) {
			dev->address = *address;
		} else {
			dev->address = sock->peer_address;
		}
	} else if (sock->type == isc_sockettype_tcp) {
		INSIST(address == NULL);
		dev->address = sock->peer_address;
	}
}

static void
destroy_socketevent(isc_event_t *event) {
	isc_socketevent_t *ev = (isc_socketevent_t *)event;

	(ev->destroy)(event);
}

static isc_socketevent_t *
allocate_socketevent(isc_mem_t *mctx, void *sender, isc_eventtype_t eventtype,
		     isc_taskaction_t action, void *arg) {
	isc_socketevent_t *ev;

	ev = (isc_socketevent_t *)isc_event_allocate(mctx, sender, eventtype,
						     action, arg, sizeof(*ev));

	ev->result = ISC_R_UNSET;
	ISC_LINK_INIT(ev, ev_link);
	ev->region.base = NULL;
	ev->n = 0;
	ev->offset = 0;
	ev->attributes = 0;
	ev->destroy = ev->ev_destroy;
	ev->ev_destroy = destroy_socketevent;
	ev->dscp = 0;

	return (ev);
}

#if defined(ISC_SOCKET_DEBUG)
static void
dump_msg(struct msghdr *msg) {
	unsigned int i;

	printf("MSGHDR %p\n", msg);
	printf("\tname %p, namelen %ld\n", msg->msg_name,
	       (long)msg->msg_namelen);
	printf("\tiov %p, iovlen %ld\n", msg->msg_iov, (long)msg->msg_iovlen);
	for (i = 0; i < (unsigned int)msg->msg_iovlen; i++) {
		printf("\t\t%u\tbase %p, len %ld\n", i,
		       msg->msg_iov[i].iov_base, (long)msg->msg_iov[i].iov_len);
	}
	printf("\tcontrol %p, controllen %ld\n", msg->msg_control,
	       (long)msg->msg_controllen);
}
#endif /* if defined(ISC_SOCKET_DEBUG) */

#define DOIO_SUCCESS 0 /* i/o ok, event sent */
#define DOIO_SOFT    1 /* i/o ok, soft error, no event sent */
#define DOIO_HARD    2 /* i/o error, event sent */
#define DOIO_EOF     3 /* EOF, no event sent */

static int
doio_recv(isc_socket_t *sock, isc_socketevent_t *dev) {
	int cc;
	struct iovec iov[MAXSCATTERGATHER_RECV];
	size_t read_count;
	struct msghdr msghdr;
	int recv_errno;
	char strbuf[ISC_STRERRORSIZE];
	char cmsgbuf[RECVCMSGBUFLEN] = { 0 };

	build_msghdr_recv(sock, cmsgbuf, dev, &msghdr, iov, &read_count);

#if defined(ISC_SOCKET_DEBUG)
	dump_msg(&msghdr);
#endif /* if defined(ISC_SOCKET_DEBUG) */

	cc = recvmsg(sock->fd, &msghdr, 0);
	recv_errno = errno;

#if defined(ISC_SOCKET_DEBUG)
	dump_msg(&msghdr);
#endif /* if defined(ISC_SOCKET_DEBUG) */

	if (cc < 0) {
		if (SOFT_ERROR(recv_errno)) {
			return (DOIO_SOFT);
		}

		if (isc_log_wouldlog(isc_lctx, IOEVENT_LEVEL)) {
			strerror_r(recv_errno, strbuf, sizeof(strbuf));
			socket_log(sock, NULL, IOEVENT,
				   "doio_recv: recvmsg(%d) %d bytes, err %d/%s",
				   sock->fd, cc, recv_errno, strbuf);
		}

#define SOFT_OR_HARD(_system, _isc)                                   \
	if (recv_errno == _system) {                                  \
		if (sock->connected) {                                \
			dev->result = _isc;                           \
			inc_stats(sock->manager->stats,               \
				  sock->statsindex[STATID_RECVFAIL]); \
			return (DOIO_HARD);                           \
		}                                                     \
		return (DOIO_SOFT);                                   \
	}
#define ALWAYS_HARD(_system, _isc)                            \
	if (recv_errno == _system) {                          \
		dev->result = _isc;                           \
		inc_stats(sock->manager->stats,               \
			  sock->statsindex[STATID_RECVFAIL]); \
		return (DOIO_HARD);                           \
	}

		SOFT_OR_HARD(ECONNREFUSED, ISC_R_CONNREFUSED);
		SOFT_OR_HARD(ENETUNREACH, ISC_R_NETUNREACH);
		SOFT_OR_HARD(EHOSTUNREACH, ISC_R_HOSTUNREACH);
		SOFT_OR_HARD(EHOSTDOWN, ISC_R_HOSTDOWN);
		SOFT_OR_HARD(ENOBUFS, ISC_R_NORESOURCES);
		/*
		 * Older operating systems may still return EPROTO in some
		 * situations, for example when receiving ICMP/ICMPv6 errors.
		 * A real life scenario is when ICMPv6 returns code 5 or 6.
		 * These codes are introduced in RFC 4443 from March 2006,
		 * and the document obsoletes RFC 1885. But unfortunately not
		 * all operating systems have caught up with the new standard
		 * (in 2020) and thus a generic protocol error is returned.
		 */
		SOFT_OR_HARD(EPROTO, ISC_R_HOSTUNREACH);
		/* Should never get this one but it was seen. */
#ifdef ENOPROTOOPT
		SOFT_OR_HARD(ENOPROTOOPT, ISC_R_HOSTUNREACH);
#endif /* ifdef ENOPROTOOPT */
		SOFT_OR_HARD(EINVAL, ISC_R_HOSTUNREACH);

#undef SOFT_OR_HARD
#undef ALWAYS_HARD

		dev->result = isc__errno2result(recv_errno);
		inc_stats(sock->manager->stats,
			  sock->statsindex[STATID_RECVFAIL]);
		return (DOIO_HARD);
	}

	/*
	 * On TCP and UNIX sockets, zero length reads indicate EOF,
	 * while on UDP sockets, zero length reads are perfectly valid,
	 * although strange.
	 */
	switch (sock->type) {
	case isc_sockettype_tcp:
	case isc_sockettype_unix:
		if (cc == 0) {
			return (DOIO_EOF);
		}
		break;
	case isc_sockettype_udp:
	case isc_sockettype_raw:
		break;
	case isc_sockettype_fdwatch:
	default:
		UNREACHABLE();
	}

	if (sock->type == isc_sockettype_udp) {
		dev->address.length = msghdr.msg_namelen;
		if (isc_sockaddr_getport(&dev->address) == 0) {
			if (isc_log_wouldlog(isc_lctx, IOEVENT_LEVEL)) {
				socket_log(sock, &dev->address, IOEVENT,
					   "dropping source port zero packet");
			}
			return (DOIO_SOFT);
		}
		/*
		 * Simulate a firewall blocking UDP responses bigger than
		 * 'maxudp' bytes.
		 */
		if (sock->manager->maxudp != 0 &&
		    cc > (int)sock->manager->maxudp)
		{
			return (DOIO_SOFT);
		}
	}

	socket_log(sock, &dev->address, IOEVENT, "packet received correctly");

	/*
	 * Overflow bit detection.  If we received MORE bytes than we should,
	 * this indicates an overflow situation.  Set the flag in the
	 * dev entry and adjust how much we read by one.
	 */
#ifdef ISC_PLATFORM_RECVOVERFLOW
	if ((sock->type == isc_sockettype_udp) && ((size_t)cc > read_count)) {
		dev->attributes |= ISC_SOCKEVENTATTR_TRUNC;
		cc--;
	}
#endif /* ifdef ISC_PLATFORM_RECVOVERFLOW */

	/*
	 * If there are control messages attached, run through them and pull
	 * out the interesting bits.
	 */
	process_cmsg(sock, &msghdr, dev);

	/*
	 * update the buffers (if any) and the i/o count
	 */
	dev->n += cc;

	/*
	 * If we read less than we expected, update counters,
	 * and let the upper layer poke the descriptor.
	 */
	if (((size_t)cc != read_count) && (dev->n < dev->minimum)) {
		return (DOIO_SOFT);
	}

	/*
	 * Full reads are posted, or partials if partials are ok.
	 */
	dev->result = ISC_R_SUCCESS;
	return (DOIO_SUCCESS);
}

/*
 * Returns:
 *	DOIO_SUCCESS	The operation succeeded.  dev->result contains
 *			ISC_R_SUCCESS.
 *
 *	DOIO_HARD	A hard or unexpected I/O error was encountered.
 *			dev->result contains the appropriate error.
 *
 *	DOIO_SOFT	A soft I/O error was encountered.  No senddone
 *			event was sent.  The operation should be retried.
 *
 *	No other return values are possible.
 */
static int
doio_send(isc_socket_t *sock, isc_socketevent_t *dev) {
	int cc;
	struct iovec iov[MAXSCATTERGATHER_SEND];
	size_t write_count;
	struct msghdr msghdr;
	char addrbuf[ISC_SOCKADDR_FORMATSIZE];
	int attempts = 0;
	int send_errno;
	char strbuf[ISC_STRERRORSIZE];
	char cmsgbuf[SENDCMSGBUFLEN] = { 0 };

	build_msghdr_send(sock, cmsgbuf, dev, &msghdr, iov, &write_count);

resend:
	if (sock->type == isc_sockettype_udp && sock->manager->maxudp != 0 &&
	    write_count > sock->manager->maxudp)
	{
		cc = write_count;
	} else {
		cc = sendmsg(sock->fd, &msghdr, 0);
	}
	send_errno = errno;

	/*
	 * Check for error or block condition.
	 */
	if (cc < 0) {
		if (send_errno == EINTR && ++attempts < NRETRIES) {
			goto resend;
		}

		if (SOFT_ERROR(send_errno)) {
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				dev->result = ISC_R_WOULDBLOCK;
			}
			return (DOIO_SOFT);
		}

#define SOFT_OR_HARD(_system, _isc)                                   \
	if (send_errno == _system) {                                  \
		if (sock->connected) {                                \
			dev->result = _isc;                           \
			inc_stats(sock->manager->stats,               \
				  sock->statsindex[STATID_SENDFAIL]); \
			return (DOIO_HARD);                           \
		}                                                     \
		return (DOIO_SOFT);                                   \
	}
#define ALWAYS_HARD(_system, _isc)                            \
	if (send_errno == _system) {                          \
		dev->result = _isc;                           \
		inc_stats(sock->manager->stats,               \
			  sock->statsindex[STATID_SENDFAIL]); \
		return (DOIO_HARD);                           \
	}

		SOFT_OR_HARD(ECONNREFUSED, ISC_R_CONNREFUSED);
		ALWAYS_HARD(EACCES, ISC_R_NOPERM);
		ALWAYS_HARD(EAFNOSUPPORT, ISC_R_ADDRNOTAVAIL);
		ALWAYS_HARD(EADDRNOTAVAIL, ISC_R_ADDRNOTAVAIL);
		ALWAYS_HARD(EHOSTUNREACH, ISC_R_HOSTUNREACH);
#ifdef EHOSTDOWN
		ALWAYS_HARD(EHOSTDOWN, ISC_R_HOSTUNREACH);
#endif /* ifdef EHOSTDOWN */
		ALWAYS_HARD(ENETUNREACH, ISC_R_NETUNREACH);
		SOFT_OR_HARD(ENOBUFS, ISC_R_NORESOURCES);
		ALWAYS_HARD(EPERM, ISC_R_HOSTUNREACH);
		ALWAYS_HARD(EPIPE, ISC_R_NOTCONNECTED);
		ALWAYS_HARD(ECONNRESET, ISC_R_CONNECTIONRESET);

#undef SOFT_OR_HARD
#undef ALWAYS_HARD

		/*
		 * The other error types depend on whether or not the
		 * socket is UDP or TCP.  If it is UDP, some errors
		 * that we expect to be fatal under TCP are merely
		 * annoying, and are really soft errors.
		 *
		 * However, these soft errors are still returned as
		 * a status.
		 */
		isc_sockaddr_format(&dev->address, addrbuf, sizeof(addrbuf));
		strerror_r(send_errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__, "internal_send: %s: %s",
				 addrbuf, strbuf);
		dev->result = isc__errno2result(send_errno);
		inc_stats(sock->manager->stats,
			  sock->statsindex[STATID_SENDFAIL]);
		return (DOIO_HARD);
	}

	if (cc == 0) {
		inc_stats(sock->manager->stats,
			  sock->statsindex[STATID_SENDFAIL]);
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "doio_send: send() returned 0");
	}

	/*
	 * If we write less than we expected, update counters, poke.
	 */
	dev->n += cc;
	if ((size_t)cc != write_count) {
		return (DOIO_SOFT);
	}

	/*
	 * Exactly what we wanted to write.  We're done with this
	 * entry.  Post its completion event.
	 */
	dev->result = ISC_R_SUCCESS;
	return (DOIO_SUCCESS);
}

/*
 * Kill.
 *
 * Caller must ensure that the socket is not locked and no external
 * references exist.
 */
static void
socketclose(isc__socketthread_t *thread, isc_socket_t *sock, int fd) {
	int lockid = FDLOCK_ID(fd);
	/*
	 * No one has this socket open, so the watcher doesn't have to be
	 * poked, and the socket doesn't have to be locked.
	 */
	LOCK(&thread->fdlock[lockid]);
	thread->fds[fd] = NULL;
	if (sock->type == isc_sockettype_fdwatch)
		thread->fdstate[fd] = CLOSED;
	else
		thread->fdstate[fd] = CLOSE_PENDING;
	UNLOCK(&thread->fdlock[lockid]);
	if (sock->type == isc_sockettype_fdwatch) {
		/*
		 * The caller may close the socket once this function returns,
		 * and `fd' may be reassigned for a new socket.  So we do
		 * unwatch_fd() here, rather than defer it via select_poke().
		 * Note: this may complicate data protection among threads and
		 * may reduce performance due to additional locks.  One way to
		 * solve this would be to dup() the watched descriptor, but we
		 * take a simpler approach at this moment.
		 */
		(void)unwatch_fd(thread, fd, SELECT_POKE_READ);
		(void)unwatch_fd(thread, fd, SELECT_POKE_WRITE);
	} else
		select_poke(thread->manager, thread->threadid, fd,
		    SELECT_POKE_CLOSE);

	inc_stats(thread->manager->stats, sock->statsindex[STATID_CLOSE]);

	LOCK(&sock->lock);
	if (sock->active == 1) {
		dec_stats(thread->manager->stats,
			  sock->statsindex[STATID_ACTIVE]);
		sock->active = 0;
	}
	UNLOCK(&sock->lock);

	/*
	 * update manager->maxfd here (XXX: this should be implemented more
	 * efficiently)
	 */
#ifdef USE_SELECT
	LOCK(&thread->manager->lock);
	if (thread->maxfd == fd) {
		int i;

		thread->maxfd = 0;
		for (i = fd - 1; i >= 0; i--) {
			lockid = FDLOCK_ID(i);

			LOCK(&thread->fdlock[lockid]);
			if (thread->fdstate[i] == MANAGED) {
				thread->maxfd = i;
				UNLOCK(&thread->fdlock[lockid]);
				break;
			}
			UNLOCK(&thread->fdlock[lockid]);
		}
		if (thread->maxfd < thread->pipe_fds[0]) {
			thread->maxfd = thread->pipe_fds[0];
		}
	}

	UNLOCK(&thread->manager->lock);
#endif /* USE_SELECT */
}

static void
destroy(isc_socket_t **sockp) {
	int fd = 0;
	isc_socket_t *sock = *sockp;
	isc_socketmgr_t *manager = sock->manager;
	isc__socketthread_t *thread = NULL;

	socket_log(sock, NULL, CREATION, "destroying");

	isc_refcount_destroy(&sock->references);

	LOCK(&sock->lock);
	INSIST(ISC_LIST_EMPTY(sock->connect_list));
	INSIST(ISC_LIST_EMPTY(sock->accept_list));
	INSIST(ISC_LIST_EMPTY(sock->recv_list));
	INSIST(ISC_LIST_EMPTY(sock->send_list));
	INSIST(sock->fd >= -1 && sock->fd < (int)manager->maxsocks);

	if (sock->fd >= 0) {
		fd = sock->fd;
		thread = &manager->threads[sock->threadid];
		sock->fd = -1;
		sock->threadid = -1;
	}
	UNLOCK(&sock->lock);

	if (fd > 0) {
		socketclose(thread, sock, fd);
	}

	LOCK(&manager->lock);

	ISC_LIST_UNLINK(manager->socklist, sock, link);

	if (ISC_LIST_EMPTY(manager->socklist)) {
		SIGNAL(&manager->shutdown_ok);
	}

	/* can't unlock manager as its memory context is still used */
	free_socket(sockp);

	UNLOCK(&manager->lock);
}

static isc_result_t
allocate_socket(isc_socketmgr_t *manager, isc_sockettype_t type,
		isc_socket_t **socketp) {
	isc_socket_t *sock;

	sock = isc_mem_get(manager->mctx, sizeof(*sock));

	sock->magic = 0;
	isc_refcount_init(&sock->references, 0);

	sock->manager = manager;
	sock->type = type;
	sock->fd = -1;
	sock->threadid = -1;
	sock->dscp = 0; /* TOS/TCLASS is zero until set. */
	sock->dupped = 0;
	sock->statsindex = NULL;
	sock->active = 0;

	ISC_LINK_INIT(sock, link);

	memset(sock->name, 0, sizeof(sock->name));
	sock->tag = NULL;

	/*
	 * Set up list of readers and writers to be initially empty.
	 */
	ISC_LIST_INIT(sock->recv_list);
	ISC_LIST_INIT(sock->send_list);
	ISC_LIST_INIT(sock->accept_list);
	ISC_LIST_INIT(sock->connect_list);

	sock->listener = 0;
	sock->connected = 0;
	sock->connecting = 0;
	sock->bound = 0;
	sock->pktdscp = 0;

	/*
	 * Initialize the lock.
	 */
	isc_mutex_init(&sock->lock);

	sock->magic = SOCKET_MAGIC;
	*socketp = sock;

	return (ISC_R_SUCCESS);
}

/*
 * This event requires that the various lists be empty, that the reference
 * count be 1, and that the magic number is valid.  The other socket bits,
 * like the lock, must be initialized as well.  The fd associated must be
 * marked as closed, by setting it to -1 on close, or this routine will
 * also close the socket.
 */
static void
free_socket(isc_socket_t **socketp) {
	isc_socket_t *sock = *socketp;
	*socketp = NULL;

	INSIST(VALID_SOCKET(sock));
	isc_refcount_destroy(&sock->references);
	LOCK(&sock->lock);
	INSIST(!sock->connecting);
	INSIST(ISC_LIST_EMPTY(sock->recv_list));
	INSIST(ISC_LIST_EMPTY(sock->send_list));
	INSIST(ISC_LIST_EMPTY(sock->accept_list));
	INSIST(ISC_LIST_EMPTY(sock->connect_list));
	INSIST(!ISC_LINK_LINKED(sock, link));
	UNLOCK(&sock->lock);

	sock->magic = 0;

	isc_mutex_destroy(&sock->lock);

	isc_mem_put(sock->manager->mctx, sock, sizeof(*sock));
}

#if defined(SET_RCVBUF)
static isc_once_t rcvbuf_once = ISC_ONCE_INIT;
static int rcvbuf = ISC_RECV_BUFFER_SIZE;

static void
set_rcvbuf(void) {
	int fd;
	int max = rcvbuf, min;
	socklen_t len;

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd == -1) {
		switch (errno) {
		case EPROTONOSUPPORT:
		case EPFNOSUPPORT:
		case EAFNOSUPPORT:
		/*
		 * Linux 2.2 (and maybe others) return EINVAL instead of
		 * EAFNOSUPPORT.
		 */
		case EINVAL:
			fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
			break;
		}
	}
	if (fd == -1) {
		return;
	}

	len = sizeof(min);
	if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&min, &len) == 0 &&
	    min < rcvbuf)
	{
	again:
		if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&rcvbuf,
			       sizeof(rcvbuf)) == -1)
		{
			if (errno == ENOBUFS && rcvbuf > min) {
				max = rcvbuf - 1;
				rcvbuf = (rcvbuf + min) / 2;
				goto again;
			} else {
				rcvbuf = min;
				goto cleanup;
			}
		} else {
			min = rcvbuf;
		}
		if (min != max) {
			rcvbuf = max;
			goto again;
		}
	}
cleanup:
	close(fd);
}
#endif /* ifdef SO_RCVBUF */

#if defined(SET_SNDBUF)
static isc_once_t sndbuf_once = ISC_ONCE_INIT;
static int sndbuf = ISC_SEND_BUFFER_SIZE;

static void
set_sndbuf(void) {
	int fd;
	int max = sndbuf, min;
	socklen_t len;

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd == -1) {
		switch (errno) {
		case EPROTONOSUPPORT:
		case EPFNOSUPPORT:
		case EAFNOSUPPORT:
		/*
		 * Linux 2.2 (and maybe others) return EINVAL instead of
		 * EAFNOSUPPORT.
		 */
		case EINVAL:
			fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
			break;
		}
	}
	if (fd == -1) {
		return;
	}

	len = sizeof(min);
	if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&min, &len) == 0 &&
	    min < sndbuf)
	{
	again:
		if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&sndbuf,
			       sizeof(sndbuf)) == -1)
		{
			if (errno == ENOBUFS && sndbuf > min) {
				max = sndbuf - 1;
				sndbuf = (sndbuf + min) / 2;
				goto again;
			} else {
				sndbuf = min;
				goto cleanup;
			}
		} else {
			min = sndbuf;
		}
		if (min != max) {
			sndbuf = max;
			goto again;
		}
	}
cleanup:
	close(fd);
}
#endif /* ifdef SO_SNDBUF */

static void
use_min_mtu(isc_socket_t *sock) {
#if !defined(IPV6_USE_MIN_MTU) && !defined(IPV6_MTU)
	UNUSED(sock);
#endif /* if !defined(IPV6_USE_MIN_MTU) && !defined(IPV6_MTU) */
#ifdef IPV6_USE_MIN_MTU
	/* use minimum MTU */
	if (sock->pf == AF_INET6) {
		int on = 1;
		(void)setsockopt(sock->fd, IPPROTO_IPV6, IPV6_USE_MIN_MTU,
				 (void *)&on, sizeof(on));
	}
#endif /* ifdef IPV6_USE_MIN_MTU */
#if defined(IPV6_MTU)
	/*
	 * Use minimum MTU on IPv6 sockets.
	 */
	if (sock->pf == AF_INET6) {
		int mtu = 1280;
		(void)setsockopt(sock->fd, IPPROTO_IPV6, IPV6_MTU, &mtu,
				 sizeof(mtu));
	}
#endif /* if defined(IPV6_MTU) */
}

static void
set_tcp_maxseg(isc_socket_t *sock, int size) {
#ifdef TCP_MAXSEG
	if (sock->type == isc_sockettype_tcp) {
		(void)setsockopt(sock->fd, IPPROTO_TCP, TCP_MAXSEG,
				 (void *)&size, sizeof(size));
	}
#endif /* ifdef TCP_MAXSEG */
}

static void
set_ip_disable_pmtud(isc_socket_t *sock) {
	/*
	 * Disable Path MTU Discover on IP packets
	 */
	if (sock->pf == AF_INET6) {
#if defined(IPV6_DONTFRAG)
		(void)setsockopt(sock->fd, IPPROTO_IPV6, IPV6_DONTFRAG,
				 &(int){ 0 }, sizeof(int));
#endif
#if defined(IPV6_MTU_DISCOVER) && defined(IP_PMTUDISC_OMIT)
		(void)setsockopt(sock->fd, IPPROTO_IPV6, IPV6_MTU_DISCOVER,
				 &(int){ IP_PMTUDISC_OMIT }, sizeof(int));
#endif
	} else if (sock->pf == AF_INET) {
#if defined(IP_DONTFRAG)
		(void)setsockopt(sock->fd, IPPROTO_IP, IP_DONTFRAG, &(int){ 0 },
				 sizeof(int));
#endif
#if defined(IP_MTU_DISCOVER) && defined(IP_PMTUDISC_OMIT)
		(void)setsockopt(sock->fd, IPPROTO_IP, IP_MTU_DISCOVER,
				 &(int){ IP_PMTUDISC_OMIT }, sizeof(int));
#endif
	}
}

static isc_result_t
opensocket(isc_socketmgr_t *manager, isc_socket_t *sock,
	   isc_socket_t *dup_socket) {
	isc_result_t result;
	char strbuf[ISC_STRERRORSIZE];
	const char *err = "socket";
	int tries = 0;
#if defined(USE_CMSG) || defined(SO_NOSIGPIPE)
	int on = 1;
#endif /* if defined(USE_CMSG) || defined(SO_NOSIGPIPE) */
#if defined(SET_RCVBUF) || defined(SET_SNDBUF)
	socklen_t optlen;
	int size = 0;
#endif

again:
	if (dup_socket == NULL) {
		switch (sock->type) {
		case isc_sockettype_udp:
			sock->fd = socket(sock->pf, SOCK_DGRAM, IPPROTO_UDP);
			break;
		case isc_sockettype_tcp:
			sock->fd = socket(sock->pf, SOCK_STREAM, IPPROTO_TCP);
			break;
		case isc_sockettype_unix:
			sock->fd = socket(sock->pf, SOCK_STREAM, 0);
			break;
		case isc_sockettype_raw:
			errno = EPFNOSUPPORT;
			/*
			 * PF_ROUTE is a alias for PF_NETLINK on linux.
			 */
#if defined(PF_ROUTE)
			if (sock->fd == -1 && sock->pf == PF_ROUTE) {
#ifdef NETLINK_ROUTE
				sock->fd = socket(sock->pf, SOCK_RAW,
						  NETLINK_ROUTE);
#else  /* ifdef NETLINK_ROUTE */
				sock->fd = socket(sock->pf, SOCK_RAW, 0);
#endif /* ifdef NETLINK_ROUTE */
				if (sock->fd != -1) {
#ifdef NETLINK_ROUTE
					struct sockaddr_nl sa;
					int n;

					/*
					 * Do an implicit bind.
					 */
					memset(&sa, 0, sizeof(sa));
					sa.nl_family = AF_NETLINK;
					sa.nl_groups = RTMGRP_IPV4_IFADDR |
						       RTMGRP_IPV6_IFADDR;
					n = bind(sock->fd,
						 (struct sockaddr *)&sa,
						 sizeof(sa));
					if (n < 0) {
						close(sock->fd);
						sock->fd = -1;
					}
#endif /* ifdef NETLINK_ROUTE */
					sock->bound = 1;
				}
			}
#endif /* if defined(PF_ROUTE) */
			break;
		case isc_sockettype_fdwatch:
			/*
			 * We should not be called for isc_sockettype_fdwatch
			 * sockets.
			 */
			INSIST(0);
			break;
		}
	} else {
		sock->fd = dup(dup_socket->fd);
		sock->dupped = 1;
		sock->bound = dup_socket->bound;
	}
	if (sock->fd == -1 && errno == EINTR && tries++ < 42) {
		goto again;
	}

#ifdef F_DUPFD
	/*
	 * Leave a space for stdio and TCP to work in.
	 */
	if (manager->reserved != 0 && sock->type == isc_sockettype_udp &&
	    sock->fd >= 0 && sock->fd < manager->reserved)
	{
		int newfd, tmp;
		newfd = fcntl(sock->fd, F_DUPFD, manager->reserved);
		tmp = errno;
		(void)close(sock->fd);
		errno = tmp;
		sock->fd = newfd;
		err = "isc_socket_create: fcntl/reserved";
	} else if (sock->fd >= 0 && sock->fd < 20) {
		int newfd, tmp;
		newfd = fcntl(sock->fd, F_DUPFD, 20);
		tmp = errno;
		(void)close(sock->fd);
		errno = tmp;
		sock->fd = newfd;
		err = "isc_socket_create: fcntl";
	}
#endif /* ifdef F_DUPFD */

	if (sock->fd >= (int)manager->maxsocks) {
		(void)close(sock->fd);
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_SOCKET, ISC_LOG_ERROR,
			      "socket: file descriptor exceeds limit (%d/%u)",
			      sock->fd, manager->maxsocks);
		inc_stats(manager->stats, sock->statsindex[STATID_OPENFAIL]);
		return (ISC_R_NORESOURCES);
	}

	if (sock->fd < 0) {
		switch (errno) {
		case EMFILE:
		case ENFILE:
			strerror_r(errno, strbuf, sizeof(strbuf));
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_SOCKET, ISC_LOG_ERROR,
				      "%s: %s", err, strbuf);
			FALLTHROUGH;
		case ENOBUFS:
			inc_stats(manager->stats,
				  sock->statsindex[STATID_OPENFAIL]);
			return (ISC_R_NORESOURCES);

		case EPROTONOSUPPORT:
		case EPFNOSUPPORT:
		case EAFNOSUPPORT:
		/*
		 * Linux 2.2 (and maybe others) return EINVAL instead of
		 * EAFNOSUPPORT.
		 */
		case EINVAL:
			inc_stats(manager->stats,
				  sock->statsindex[STATID_OPENFAIL]);
			return (ISC_R_FAMILYNOSUPPORT);

		default:
			strerror_r(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__, "%s() failed: %s",
					 err, strbuf);
			inc_stats(manager->stats,
				  sock->statsindex[STATID_OPENFAIL]);
			return (ISC_R_UNEXPECTED);
		}
	}

	if (dup_socket != NULL) {
		goto setup_done;
	}

	result = make_nonblock(sock->fd);
	if (result != ISC_R_SUCCESS) {
		(void)close(sock->fd);
		inc_stats(manager->stats, sock->statsindex[STATID_OPENFAIL]);
		return (result);
	}

#ifdef SO_NOSIGPIPE
	if (setsockopt(sock->fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&on,
		       sizeof(on)) < 0)
	{
		strerror_r(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "setsockopt(%d, SO_NOSIGPIPE) failed: %s",
				 sock->fd, strbuf);
		/* Press on... */
	}
#endif /* ifdef SO_NOSIGPIPE */

	/*
	 * Use minimum mtu if possible.
	 */
	if (sock->type == isc_sockettype_tcp && sock->pf == AF_INET6) {
		use_min_mtu(sock);
		set_tcp_maxseg(sock, 1280 - 20 - 40); /* 1280 - TCP - IPV6 */
	}

#if defined(USE_CMSG) || defined(SET_RCVBUF) || defined(SET_SNDBUF)
	if (sock->type == isc_sockettype_udp) {
#if defined(USE_CMSG)
#if defined(SO_TIMESTAMP)
		if (setsockopt(sock->fd, SOL_SOCKET, SO_TIMESTAMP, (void *)&on,
			       sizeof(on)) < 0 &&
		    errno != ENOPROTOOPT)
		{
			strerror_r(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "setsockopt(%d, SO_TIMESTAMP) failed: "
					 "%s",
					 sock->fd, strbuf);
			/* Press on... */
		}
#endif /* SO_TIMESTAMP */

#ifdef IPV6_RECVPKTINFO
		/* RFC 3542 */
		if ((sock->pf == AF_INET6) &&
		    (setsockopt(sock->fd, IPPROTO_IPV6, IPV6_RECVPKTINFO,
				(void *)&on, sizeof(on)) < 0))
		{
			strerror_r(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "setsockopt(%d, IPV6_RECVPKTINFO) "
					 "failed: %s",
					 sock->fd, strbuf);
		}
#else  /* ifdef IPV6_RECVPKTINFO */
		/* RFC 2292 */
		if ((sock->pf == AF_INET6) &&
		    (setsockopt(sock->fd, IPPROTO_IPV6, IPV6_PKTINFO,
				(void *)&on, sizeof(on)) < 0))
		{
			strerror_r(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "setsockopt(%d, IPV6_PKTINFO) failed: "
					 "%s",
					 sock->fd, strbuf);
		}
#endif /* IPV6_RECVPKTINFO */
#endif /* defined(USE_CMSG) */

#if defined(SET_RCVBUF)
		optlen = sizeof(size);
		if (getsockopt(sock->fd, SOL_SOCKET, SO_RCVBUF, (void *)&size,
			       &optlen) == 0 &&
		    size < rcvbuf)
		{
			RUNTIME_CHECK(isc_once_do(&rcvbuf_once, set_rcvbuf) ==
				      ISC_R_SUCCESS);
			if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVBUF,
				       (void *)&rcvbuf, sizeof(rcvbuf)) == -1)
			{
				strerror_r(errno, strbuf, sizeof(strbuf));
				UNEXPECTED_ERROR(__FILE__, __LINE__,
						 "setsockopt(%d, SO_RCVBUF, "
						 "%d) failed: %s",
						 sock->fd, rcvbuf, strbuf);
			}
		}
#endif /* if defined(SET_RCVBUF) */

#if defined(SET_SNDBUF)
		optlen = sizeof(size);
		if (getsockopt(sock->fd, SOL_SOCKET, SO_SNDBUF, (void *)&size,
			       &optlen) == 0 &&
		    size < sndbuf)
		{
			RUNTIME_CHECK(isc_once_do(&sndbuf_once, set_sndbuf) ==
				      ISC_R_SUCCESS);
			if (setsockopt(sock->fd, SOL_SOCKET, SO_SNDBUF,
				       (void *)&sndbuf, sizeof(sndbuf)) == -1)
			{
				strerror_r(errno, strbuf, sizeof(strbuf));
				UNEXPECTED_ERROR(__FILE__, __LINE__,
						 "setsockopt(%d, SO_SNDBUF, "
						 "%d) failed: %s",
						 sock->fd, sndbuf, strbuf);
			}
		}
#endif /* if defined(SO_SNDBUF) */
	}
#ifdef IPV6_RECVTCLASS
	if ((sock->pf == AF_INET6) &&
	    (setsockopt(sock->fd, IPPROTO_IPV6, IPV6_RECVTCLASS, (void *)&on,
			sizeof(on)) < 0))
	{
		strerror_r(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "setsockopt(%d, IPV6_RECVTCLASS) "
				 "failed: %s",
				 sock->fd, strbuf);
	}
#endif /* ifdef IPV6_RECVTCLASS */
#ifdef IP_RECVTOS
	if ((sock->pf == AF_INET) &&
	    (setsockopt(sock->fd, IPPROTO_IP, IP_RECVTOS, (void *)&on,
			sizeof(on)) < 0))
	{
		strerror_r(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "setsockopt(%d, IP_RECVTOS) "
				 "failed: %s",
				 sock->fd, strbuf);
	}
#endif /* ifdef IP_RECVTOS */
#endif /* defined(USE_CMSG) || defined(SET_RCVBUF) || defined(SET_SNDBUF) */

	set_ip_disable_pmtud(sock);

setup_done:
	inc_stats(manager->stats, sock->statsindex[STATID_OPEN]);
	if (sock->active == 0) {
		inc_stats(manager->stats, sock->statsindex[STATID_ACTIVE]);
		sock->active = 1;
	}

	return (ISC_R_SUCCESS);
}

/*
 * Create a 'type' socket or duplicate an existing socket, managed
 * by 'manager'.  Events will be posted to 'task' and when dispatched
 * 'action' will be called with 'arg' as the arg value.  The new
 * socket is returned in 'socketp'.
 */
static isc_result_t
socket_create(isc_socketmgr_t *manager, int pf, isc_sockettype_t type,
	      isc_socket_t **socketp, isc_socket_t *dup_socket) {
	isc_socket_t *sock = NULL;
	isc__socketthread_t *thread;
	isc_result_t result;
	int lockid;

	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(socketp != NULL && *socketp == NULL);
	REQUIRE(type != isc_sockettype_fdwatch);

	result = allocate_socket(manager, type, &sock);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	switch (sock->type) {
	case isc_sockettype_udp:
		sock->statsindex = (pf == AF_INET) ? udp4statsindex
						   : udp6statsindex;
#define DCSPPKT(pf) ((pf == AF_INET) ? ISC_NET_DSCPPKTV4 : ISC_NET_DSCPPKTV6)
		sock->pktdscp = (isc_net_probedscp() & DCSPPKT(pf)) != 0;
		break;
	case isc_sockettype_tcp:
		sock->statsindex = (pf == AF_INET) ? tcp4statsindex
						   : tcp6statsindex;
		break;
	case isc_sockettype_unix:
		sock->statsindex = unixstatsindex;
		break;
	case isc_sockettype_raw:
		sock->statsindex = rawstatsindex;
		break;
	default:
		UNREACHABLE();
	}

	sock->pf = pf;

	result = opensocket(manager, sock, dup_socket);
	if (result != ISC_R_SUCCESS) {
		free_socket(&sock);
		return (result);
	}

	if (sock->fd == -1) {
		abort();
	}
	sock->threadid = gen_threadid(sock);
	isc_refcount_increment0(&sock->references);
	thread = &manager->threads[sock->threadid];
	*socketp = sock;

	/*
	 * Note we don't have to lock the socket like we normally would because
	 * there are no external references to it yet.
	 */

	lockid = FDLOCK_ID(sock->fd);
	LOCK(&thread->fdlock[lockid]);
	thread->fds[sock->fd] = sock;
	thread->fdstate[sock->fd] = MANAGED;
#if defined(USE_EPOLL)
	thread->epoll_events[sock->fd] = 0;
#endif /* if defined(USE_EPOLL) */
#ifdef USE_DEVPOLL
	INSIST(thread->fdpollinfo[sock->fd].want_read == 0 &&
	       thread->fdpollinfo[sock->fd].want_write == 0);
#endif /* ifdef USE_DEVPOLL */
	UNLOCK(&thread->fdlock[lockid]);

	LOCK(&manager->lock);
	ISC_LIST_APPEND(manager->socklist, sock, link);
#ifdef USE_SELECT
	if (thread->maxfd < sock->fd) {
		thread->maxfd = sock->fd;
	}
#endif /* ifdef USE_SELECT */
	UNLOCK(&manager->lock);

	socket_log(sock, NULL, CREATION,
		   dup_socket != NULL ? "dupped" : "created");

	return (ISC_R_SUCCESS);
}

/*%
 * Create a new 'type' socket managed by 'manager'.  Events
 * will be posted to 'task' and when dispatched 'action' will be
 * called with 'arg' as the arg value.  The new socket is returned
 * in 'socketp'.
 */
isc_result_t
isc_socket_create(isc_socketmgr_t *manager0, int pf, isc_sockettype_t type,
		  isc_socket_t **socketp) {
	return (socket_create(manager0, pf, type, socketp, NULL));
}

/*%
 * Duplicate an existing socket.  The new socket is returned
 * in 'socketp'.
 */
isc_result_t
isc_socket_dup(isc_socket_t *sock, isc_socket_t **socketp) {
	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(socketp != NULL && *socketp == NULL);

	return (socket_create(sock->manager, sock->pf, sock->type, socketp,
			      sock));
}

isc_result_t
isc_socket_open(isc_socket_t *sock) {
	isc_result_t result;
	isc__socketthread_t *thread;

	REQUIRE(VALID_SOCKET(sock));

	LOCK(&sock->lock);

	REQUIRE(isc_refcount_current(&sock->references) >= 1);
	REQUIRE(sock->fd == -1);
	REQUIRE(sock->threadid == -1);
	REQUIRE(sock->type != isc_sockettype_fdwatch);

	result = opensocket(sock->manager, sock, NULL);

	UNLOCK(&sock->lock);

	if (result != ISC_R_SUCCESS) {
		sock->fd = -1;
	} else {
		sock->threadid = gen_threadid(sock);
		thread = &sock->manager->threads[sock->threadid];
		int lockid = FDLOCK_ID(sock->fd);

		LOCK(&thread->fdlock[lockid]);
		thread->fds[sock->fd] = sock;
		thread->fdstate[sock->fd] = MANAGED;
#if defined(USE_EPOLL)
		thread->epoll_events[sock->fd] = 0;
#endif /* if defined(USE_EPOLL) */
#ifdef USE_DEVPOLL
		INSIST(thread->fdpollinfo[sock->fd].want_read == 0 &&
		       thread->fdpollinfo[sock->fd].want_write == 0);
#endif /* ifdef USE_DEVPOLL */
		UNLOCK(&thread->fdlock[lockid]);

#ifdef USE_SELECT
		LOCK(&sock->manager->lock);
		if (thread->maxfd < sock->fd) {
			thread->maxfd = sock->fd;
		}
		UNLOCK(&sock->manager->lock);
#endif /* ifdef USE_SELECT */
	}

	return (result);
}

/*
 * Attach to a socket.  Caller must explicitly detach when it is done.
 */
void
isc_socket_attach(isc_socket_t *sock, isc_socket_t **socketp) {
	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(socketp != NULL && *socketp == NULL);

	int old_refs = isc_refcount_increment(&sock->references);
	REQUIRE(old_refs > 0);

	*socketp = sock;
}

/*
 * Dereference a socket.  If this is the last reference to it, clean things
 * up by destroying the socket.
 */
void
isc_socket_detach(isc_socket_t **socketp) {
	isc_socket_t *sock;

	REQUIRE(socketp != NULL);
	sock = *socketp;
	REQUIRE(VALID_SOCKET(sock));
	if (isc_refcount_decrement(&sock->references) == 1) {
		destroy(&sock);
	}

	*socketp = NULL;
}

isc_result_t
isc_socket_close(isc_socket_t *sock) {
	int fd;
	isc_socketmgr_t *manager;
	isc__socketthread_t *thread;
	fflush(stdout);
	REQUIRE(VALID_SOCKET(sock));

	LOCK(&sock->lock);

	REQUIRE(sock->type != isc_sockettype_fdwatch);
	REQUIRE(sock->fd >= 0 && sock->fd < (int)sock->manager->maxsocks);

	INSIST(!sock->connecting);
	INSIST(ISC_LIST_EMPTY(sock->recv_list));
	INSIST(ISC_LIST_EMPTY(sock->send_list));
	INSIST(ISC_LIST_EMPTY(sock->accept_list));
	INSIST(ISC_LIST_EMPTY(sock->connect_list));

	manager = sock->manager;
	thread = &manager->threads[sock->threadid];
	fd = sock->fd;
	sock->fd = -1;
	sock->threadid = -1;

	sock->dupped = 0;
	memset(sock->name, 0, sizeof(sock->name));
	sock->tag = NULL;
	sock->listener = 0;
	sock->connected = 0;
	sock->connecting = 0;
	sock->bound = 0;
	isc_sockaddr_any(&sock->peer_address);

	UNLOCK(&sock->lock);

	socketclose(thread, sock, fd);

	return (ISC_R_SUCCESS);
}

static void
dispatch_recv(isc_socket_t *sock) {
	if (sock->type != isc_sockettype_fdwatch) {
		internal_recv(sock);
	} else {
		internal_fdwatch_read(sock);
	}
}

static void
dispatch_send(isc_socket_t *sock) {
	if (sock->type != isc_sockettype_fdwatch) {
		internal_send(sock);
	} else {
		internal_fdwatch_write(sock);
	}
}

/*
 * Dequeue an item off the given socket's read queue, set the result code
 * in the done event to the one provided, and send it to the task it was
 * destined for.
 *
 * If the event to be sent is on a list, remove it before sending.  If
 * asked to, send and detach from the socket as well.
 *
 * Caller must have the socket locked if the event is attached to the socket.
 */
static void
send_recvdone_event(isc_socket_t *sock, isc_socketevent_t **dev) {
	isc_task_t *task;

	task = (*dev)->ev_sender;

	(*dev)->ev_sender = sock;

	if (ISC_LINK_LINKED(*dev, ev_link)) {
		ISC_LIST_DEQUEUE(sock->recv_list, *dev, ev_link);
	}

	if (((*dev)->attributes & ISC_SOCKEVENTATTR_ATTACHED) != 0) {
		isc_task_sendtoanddetach(&task, (isc_event_t **)dev,
					 sock->threadid);
	} else {
		isc_task_sendto(task, (isc_event_t **)dev, sock->threadid);
	}
}

/*
 * See comments for send_recvdone_event() above.
 *
 * Caller must have the socket locked if the event is attached to the socket.
 */
static void
send_senddone_event(isc_socket_t *sock, isc_socketevent_t **dev) {
	isc_task_t *task;

	INSIST(dev != NULL && *dev != NULL);

	task = (*dev)->ev_sender;
	(*dev)->ev_sender = sock;

	if (ISC_LINK_LINKED(*dev, ev_link)) {
		ISC_LIST_DEQUEUE(sock->send_list, *dev, ev_link);
	}

	if (((*dev)->attributes & ISC_SOCKEVENTATTR_ATTACHED) != 0) {
		isc_task_sendtoanddetach(&task, (isc_event_t **)dev,
					 sock->threadid);
	} else {
		isc_task_sendto(task, (isc_event_t **)dev, sock->threadid);
	}
}

/*
 * See comments for send_recvdone_event() above.
 *
 * Caller must have the socket locked if the event is attached to the socket.
 */
static void
send_connectdone_event(isc_socket_t *sock, isc_socket_connev_t **dev) {
	isc_task_t *task;

	INSIST(dev != NULL && *dev != NULL);

	task = (*dev)->ev_sender;
	(*dev)->ev_sender = sock;

	if (ISC_LINK_LINKED(*dev, ev_link)) {
		ISC_LIST_DEQUEUE(sock->connect_list, *dev, ev_link);
	}

	isc_task_sendtoanddetach(&task, (isc_event_t **)dev, sock->threadid);
}

/*
 * Call accept() on a socket, to get the new file descriptor.  The listen
 * socket is used as a prototype to create a new isc_socket_t.  The new
 * socket has one outstanding reference.  The task receiving the event
 * will be detached from just after the event is delivered.
 *
 * On entry to this function, the event delivered is the internal
 * readable event, and the first item on the accept_list should be
 * the done event we want to send.  If the list is empty, this is a no-op,
 * so just unlock and return.
 */
static void
internal_accept(isc_socket_t *sock) {
	isc_socketmgr_t *manager;
	isc__socketthread_t *thread, *nthread;
	isc_socket_newconnev_t *dev;
	isc_task_t *task;
	socklen_t addrlen;
	int fd;
	isc_result_t result = ISC_R_SUCCESS;
	char strbuf[ISC_STRERRORSIZE];
	const char *err = "accept";

	INSIST(VALID_SOCKET(sock));
	REQUIRE(sock->fd >= 0);

	socket_log(sock, NULL, TRACE, "internal_accept called, locked socket");

	manager = sock->manager;
	INSIST(VALID_MANAGER(manager));
	thread = &manager->threads[sock->threadid];

	INSIST(sock->listener);

	/*
	 * Get the first item off the accept list.
	 * If it is empty, unlock the socket and return.
	 */
	dev = ISC_LIST_HEAD(sock->accept_list);
	if (dev == NULL) {
		unwatch_fd(thread, sock->fd, SELECT_POKE_ACCEPT);
		UNLOCK(&sock->lock);
		return;
	}

	/*
	 * Try to accept the new connection.  If the accept fails with
	 * EAGAIN or EINTR, simply poke the watcher to watch this socket
	 * again.  Also ignore ECONNRESET, which has been reported to
	 * be spuriously returned on Linux 2.2.19 although it is not
	 * a documented error for accept().  ECONNABORTED has been
	 * reported for Solaris 8.  The rest are thrown in not because
	 * we have seen them but because they are ignored by other
	 * daemons such as BIND 8 and Apache.
	 */

	addrlen = sizeof(NEWCONNSOCK(dev)->peer_address.type);
	memset(&NEWCONNSOCK(dev)->peer_address.type, 0, addrlen);
	fd = accept(sock->fd, &NEWCONNSOCK(dev)->peer_address.type.sa,
		    (void *)&addrlen);

#ifdef F_DUPFD
	/*
	 * Leave a space for stdio to work in.
	 */
	if (fd >= 0 && fd < 20) {
		int newfd, tmp;
		newfd = fcntl(fd, F_DUPFD, 20);
		tmp = errno;
		(void)close(fd);
		errno = tmp;
		fd = newfd;
		err = "accept/fcntl";
	}
#endif /* ifdef F_DUPFD */

	if (fd < 0) {
		if (SOFT_ERROR(errno)) {
			goto soft_error;
		}
		switch (errno) {
		case ENFILE:
		case EMFILE:
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_SOCKET, ISC_LOG_ERROR,
				      "%s: too many open file descriptors",
				      err);
			goto soft_error;

		case ENOBUFS:
		case ENOMEM:
		case ECONNRESET:
		case ECONNABORTED:
		case EHOSTUNREACH:
		case EHOSTDOWN:
		case ENETUNREACH:
		case ENETDOWN:
		case ECONNREFUSED:
#ifdef EPROTO
		case EPROTO:
#endif /* ifdef EPROTO */
#ifdef ENONET
		case ENONET:
#endif /* ifdef ENONET */
			goto soft_error;
		default:
			break;
		}
		strerror_r(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "internal_accept: %s() failed: %s", err,
				 strbuf);
		fd = -1;
		result = ISC_R_UNEXPECTED;
	} else {
		if (addrlen == 0U) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "internal_accept(): "
					 "accept() failed to return "
					 "remote address");

			(void)close(fd);
			goto soft_error;
		} else if (NEWCONNSOCK(dev)->peer_address.type.sa.sa_family !=
			   sock->pf)
		{
			UNEXPECTED_ERROR(
				__FILE__, __LINE__,
				"internal_accept(): "
				"accept() returned peer address "
				"family %u (expected %u)",
				NEWCONNSOCK(dev)->peer_address.type.sa.sa_family,
				sock->pf);
			(void)close(fd);
			goto soft_error;
		} else if (fd >= (int)manager->maxsocks) {
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_SOCKET, ISC_LOG_ERROR,
				      "accept: file descriptor exceeds limit "
				      "(%d/%u)",
				      fd, manager->maxsocks);
			(void)close(fd);
			goto soft_error;
		}
	}

	if (fd != -1) {
		NEWCONNSOCK(dev)->peer_address.length = addrlen;
		NEWCONNSOCK(dev)->pf = sock->pf;
	}

	/*
	 * Pull off the done event.
	 */
	ISC_LIST_UNLINK(sock->accept_list, dev, ev_link);

	/*
	 * Poke watcher if there are more pending accepts.
	 */
	if (ISC_LIST_EMPTY(sock->accept_list)) {
		unwatch_fd(thread, sock->fd, SELECT_POKE_ACCEPT);
	}

	if (fd != -1) {
		result = make_nonblock(fd);
		if (result != ISC_R_SUCCESS) {
			(void)close(fd);
			fd = -1;
		}
	}

	/*
	 * We need to unlock sock->lock now to be able to lock manager->lock
	 * without risking a deadlock with xmlstats.
	 */
	UNLOCK(&sock->lock);

	/*
	 * -1 means the new socket didn't happen.
	 */
	if (fd != -1) {
		int lockid = FDLOCK_ID(fd);

		NEWCONNSOCK(dev)->fd = fd;
		NEWCONNSOCK(dev)->threadid = gen_threadid(NEWCONNSOCK(dev));
		NEWCONNSOCK(dev)->bound = 1;
		NEWCONNSOCK(dev)->connected = 1;
		nthread = &manager->threads[NEWCONNSOCK(dev)->threadid];

		/*
		 * We already hold a lock on one fdlock in accepting thread,
		 * we need to make sure that we don't double lock.
		 */
		bool same_bucket = (sock->threadid ==
				    NEWCONNSOCK(dev)->threadid) &&
				   (FDLOCK_ID(sock->fd) == lockid);

		/*
		 * Use minimum mtu if possible.
		 */
		use_min_mtu(NEWCONNSOCK(dev));
		set_tcp_maxseg(NEWCONNSOCK(dev), 1280 - 20 - 40);

		/*
		 * Ensure DSCP settings are inherited across accept.
		 */
		setdscp(NEWCONNSOCK(dev), sock->dscp);

		/*
		 * Save away the remote address
		 */
		dev->address = NEWCONNSOCK(dev)->peer_address;

		if (NEWCONNSOCK(dev)->active == 0) {
			inc_stats(manager->stats,
				  NEWCONNSOCK(dev)->statsindex[STATID_ACTIVE]);
			NEWCONNSOCK(dev)->active = 1;
		}

		if (!same_bucket) {
			LOCK(&nthread->fdlock[lockid]);
		}
		nthread->fds[fd] = NEWCONNSOCK(dev);
		nthread->fdstate[fd] = MANAGED;
#if defined(USE_EPOLL)
		nthread->epoll_events[fd] = 0;
#endif /* if defined(USE_EPOLL) */
		if (!same_bucket) {
			UNLOCK(&nthread->fdlock[lockid]);
		}

		LOCK(&manager->lock);

#ifdef USE_SELECT
		if (nthread->maxfd < fd) {
			nthread->maxfd = fd;
		}
#endif /* ifdef USE_SELECT */

		socket_log(sock, &NEWCONNSOCK(dev)->peer_address, CREATION,
			   "accepted connection, new socket %p",
			   dev->newsocket);

		ISC_LIST_APPEND(manager->socklist, NEWCONNSOCK(dev), link);

		UNLOCK(&manager->lock);

		inc_stats(manager->stats, sock->statsindex[STATID_ACCEPT]);
	} else {
		inc_stats(manager->stats, sock->statsindex[STATID_ACCEPTFAIL]);
		isc_refcount_decrementz(&NEWCONNSOCK(dev)->references);
		free_socket((isc_socket_t **)&dev->newsocket);
	}

	/*
	 * Fill in the done event details and send it off.
	 */
	dev->result = result;
	task = dev->ev_sender;
	dev->ev_sender = sock;

	isc_task_sendtoanddetach(&task, ISC_EVENT_PTR(&dev), sock->threadid);
	return;

soft_error:
	watch_fd(thread, sock->fd, SELECT_POKE_ACCEPT);
	UNLOCK(&sock->lock);

	inc_stats(manager->stats, sock->statsindex[STATID_ACCEPTFAIL]);
	return;
}

static void
internal_recv(isc_socket_t *sock) {
	isc_socketevent_t *dev;

	INSIST(VALID_SOCKET(sock));
	REQUIRE(sock->fd >= 0);

	dev = ISC_LIST_HEAD(sock->recv_list);
	if (dev == NULL) {
		goto finish;
	}

	socket_log(sock, NULL, IOEVENT, "internal_recv: event %p -> task %p",
		   dev, dev->ev_sender);

	/*
	 * Try to do as much I/O as possible on this socket.  There are no
	 * limits here, currently.
	 */
	while (dev != NULL) {
		switch (doio_recv(sock, dev)) {
		case DOIO_SOFT:
			goto finish;

		case DOIO_EOF:
			/*
			 * read of 0 means the remote end was closed.
			 * Run through the event queue and dispatch all
			 * the events with an EOF result code.
			 */
			do {
				dev->result = ISC_R_EOF;
				send_recvdone_event(sock, &dev);
				dev = ISC_LIST_HEAD(sock->recv_list);
			} while (dev != NULL);
			goto finish;

		case DOIO_SUCCESS:
		case DOIO_HARD:
			send_recvdone_event(sock, &dev);
			break;
		}

		dev = ISC_LIST_HEAD(sock->recv_list);
	}

finish:
	if (ISC_LIST_EMPTY(sock->recv_list)) {
		unwatch_fd(&sock->manager->threads[sock->threadid], sock->fd,
			   SELECT_POKE_READ);
	}
}

static void
internal_send(isc_socket_t *sock) {
	isc_socketevent_t *dev;

	INSIST(VALID_SOCKET(sock));
	REQUIRE(sock->fd >= 0);

	dev = ISC_LIST_HEAD(sock->send_list);
	if (dev == NULL) {
		goto finish;
	}
	socket_log(sock, NULL, EVENT, "internal_send: event %p -> task %p", dev,
		   dev->ev_sender);

	/*
	 * Try to do as much I/O as possible on this socket.  There are no
	 * limits here, currently.
	 */
	while (dev != NULL) {
		switch (doio_send(sock, dev)) {
		case DOIO_SOFT:
			goto finish;

		case DOIO_HARD:
		case DOIO_SUCCESS:
			send_senddone_event(sock, &dev);
			break;
		}

		dev = ISC_LIST_HEAD(sock->send_list);
	}

finish:
	if (ISC_LIST_EMPTY(sock->send_list)) {
		unwatch_fd(&sock->manager->threads[sock->threadid], sock->fd,
			   SELECT_POKE_WRITE);
	}
}

static void
internal_fdwatch_write(isc_socket_t *sock)
{
	int more_data;

	INSIST(VALID_SOCKET(sock));

	isc_refcount_increment(&sock->references);
	UNLOCK(&sock->lock);

	more_data = (sock->fdwatchcb)(sock->fdwatchtask, (isc_socket_t *)sock,
				      sock->fdwatcharg, ISC_SOCKFDWATCH_WRITE);

	LOCK(&sock->lock);

	if (isc_refcount_decrement(&sock->references) == 0) {
		UNLOCK(&sock->lock);
		destroy(&sock);
		return;
	}

	if (more_data)
		select_poke(sock->manager, sock->threadid, sock->fd,
		    SELECT_POKE_WRITE);
}

static void
internal_fdwatch_read(isc_socket_t *sock)
{
	int more_data;

	INSIST(VALID_SOCKET(sock));

	isc_refcount_increment(&sock->references);
	UNLOCK(&sock->lock);

	more_data = (sock->fdwatchcb)(sock->fdwatchtask, (isc_socket_t *)sock,
				      sock->fdwatcharg, ISC_SOCKFDWATCH_READ);

	LOCK(&sock->lock);

	if (isc_refcount_decrement(&sock->references) == 0) {
		UNLOCK(&sock->lock);
		destroy(&sock);
		return;
	}

	if (more_data)
		select_poke(sock->manager, sock->threadid, sock->fd,
		    SELECT_POKE_READ);
}

/*
 * Process read/writes on each fd here.  Avoid locking
 * and unlocking twice if both reads and writes are possible.
 */
static void
process_fd(isc__socketthread_t *thread, int fd, bool readable, bool writeable) {
	isc_socket_t *sock;
	int lockid = FDLOCK_ID(fd);

	/*
	 * If the socket is going to be closed, don't do more I/O.
	 */
	LOCK(&thread->fdlock[lockid]);
	if (thread->fdstate[fd] == CLOSE_PENDING) {
		UNLOCK(&thread->fdlock[lockid]);

		(void)unwatch_fd(thread, fd, SELECT_POKE_READ);
		(void)unwatch_fd(thread, fd, SELECT_POKE_WRITE);
		return;
	}

	sock = thread->fds[fd];
	if (sock == NULL) {
		UNLOCK(&thread->fdlock[lockid]);
		return;
	}

	LOCK(&sock->lock);

	if (sock->fd < 0) {
		/*
		 * Sock is being closed - the final external reference
		 * is gone but it was not yet removed from event loop
		 * and fdstate[]/fds[] as destroy() is waiting on
		 * thread->fdlock[lockid] or sock->lock that we're holding.
		 * Just release the locks and bail.
		 */
		UNLOCK(&sock->lock);
		UNLOCK(&thread->fdlock[lockid]);
		return;
	}

	REQUIRE(readable || writeable);
	if (writeable) {
		if (sock->connecting) {
			internal_connect(sock);
		} else {
			dispatch_send(sock);
		}
	}

	if (readable) {
		if (sock->listener) {
			internal_accept(sock); /* unlocks sock */
		} else {
			dispatch_recv(sock);
			UNLOCK(&sock->lock);
		}
	} else {
		UNLOCK(&sock->lock);
	}

	UNLOCK(&thread->fdlock[lockid]);

	/*
	 * Socket destruction might be pending, it will resume
	 * after releasing fdlock and sock->lock.
	 */
}

/*
 * process_fds is different for different event loops
 * it takes the events from event loops and for each FD
 * launches process_fd
 */
#ifdef USE_KQUEUE
static bool
process_fds(isc__socketthread_t *thread, struct kevent *events, int nevents) {
	int i;
	bool readable, writable;
	bool done = false;
	bool have_ctlevent = false;
	if (nevents == thread->nevents) {
		/*
		 * This is not an error, but something unexpected.  If this
		 * happens, it may indicate the need for increasing
		 * ISC_SOCKET_MAXEVENTS.
		 */
		thread_log(thread, ISC_LOGCATEGORY_GENERAL,
			   ISC_LOGMODULE_SOCKET, ISC_LOG_INFO,
			   "maximum number of FD events (%d) received",
			   nevents);
	}

	for (i = 0; i < nevents; i++) {
		REQUIRE(events[i].ident < thread->manager->maxsocks);
		if (events[i].ident == (uintptr_t)thread->pipe_fds[0]) {
			have_ctlevent = true;
			continue;
		}
		readable = (events[i].filter == EVFILT_READ);
		writable = (events[i].filter == EVFILT_WRITE);
		process_fd(thread, events[i].ident, readable, writable);
	}

	if (have_ctlevent) {
		done = process_ctlfd(thread);
	}

	return (done);
}
#elif defined(USE_EPOLL)
static bool
process_fds(isc__socketthread_t *thread, struct epoll_event *events,
	    int nevents) {
	int i;
	bool done = false;
	bool have_ctlevent = false;

	if (nevents == thread->nevents) {
		thread_log(thread, ISC_LOGCATEGORY_GENERAL,
			   ISC_LOGMODULE_SOCKET, ISC_LOG_INFO,
			   "maximum number of FD events (%d) received",
			   nevents);
	}

	for (i = 0; i < nevents; i++) {
		REQUIRE(events[i].data.fd < (int)thread->manager->maxsocks);
		if (events[i].data.fd == thread->pipe_fds[0]) {
			have_ctlevent = true;
			continue;
		}
		if ((events[i].events & EPOLLERR) != 0 ||
		    (events[i].events & EPOLLHUP) != 0)
		{
			/*
			 * epoll does not set IN/OUT bits on an erroneous
			 * condition, so we need to try both anyway.  This is a
			 * bit inefficient, but should be okay for such rare
			 * events.  Note also that the read or write attempt
			 * won't block because we use non-blocking sockets.
			 */
			int fd = events[i].data.fd;
			events[i].events |= thread->epoll_events[fd];
		}
		process_fd(thread, events[i].data.fd,
			   (events[i].events & EPOLLIN) != 0,
			   (events[i].events & EPOLLOUT) != 0);
	}

	if (have_ctlevent) {
		done = process_ctlfd(thread);
	}

	return (done);
}
#elif defined(USE_DEVPOLL)
static bool
process_fds(isc__socketthread_t *thread, struct pollfd *events, int nevents) {
	int i;
	bool done = false;
	bool have_ctlevent = false;

	if (nevents == thread->nevents) {
		thread_log(thread, ISC_LOGCATEGORY_GENERAL,
			   ISC_LOGMODULE_SOCKET, ISC_LOG_INFO,
			   "maximum number of FD events (%d) received",
			   nevents);
	}

	for (i = 0; i < nevents; i++) {
		REQUIRE(events[i].fd < (int)thread->manager->maxsocks);
		if (events[i].fd == thread->pipe_fds[0]) {
			have_ctlevent = true;
			continue;
		}
		process_fd(thread, events[i].fd,
			   (events[i].events & POLLIN) != 0,
			   (events[i].events & POLLOUT) != 0);
	}

	if (have_ctlevent) {
		done = process_ctlfd(thread);
	}

	return (done);
}
#elif defined(USE_SELECT)
static void
process_fds(isc__socketthread_t *thread, int maxfd, fd_set *readfds,
	    fd_set *writefds) {
	int i;

	REQUIRE(maxfd <= (int)thread->manager->maxsocks);

	for (i = 0; i < maxfd; i++) {
		if (i == thread->pipe_fds[0] || i == thread->pipe_fds[1]) {
			continue;
		}
		process_fd(thread, i, FD_ISSET(i, readfds),
			   FD_ISSET(i, writefds));
	}
}
#endif /* ifdef USE_KQUEUE */

static bool
process_ctlfd(isc__socketthread_t *thread) {
	int msg, fd;

	for (;;) {
		select_readmsg(thread, &fd, &msg);

		thread_log(thread, IOEVENT,
			   "watcher got message %d for socket %d", msg, fd);

		/*
		 * Nothing to read?
		 */
		if (msg == SELECT_POKE_NOTHING) {
			break;
		}

		/*
		 * Handle shutdown message.  We really should
		 * jump out of this loop right away, but
		 * it doesn't matter if we have to do a little
		 * more work first.
		 */
		if (msg == SELECT_POKE_SHUTDOWN) {
			return (true);
		}

		/*
		 * This is a wakeup on a socket.  Look
		 * at the event queue for both read and write,
		 * and decide if we need to watch on it now
		 * or not.
		 */
		wakeup_socket(thread, fd, msg);
	}

	return (false);
}

/*
 * This is the thread that will loop forever, always in a select or poll
 * call.
 *
 * When select returns something to do, do whatever's necessary and post
 * an event to the task that was requesting the action.
 */
static isc_threadresult_t
netthread(void *uap) {
	isc__socketthread_t *thread = uap;
	isc_socketmgr_t *manager = thread->manager;
	(void)manager;
	bool done;
	int cc;
#ifdef USE_KQUEUE
	const char *fnname = "kevent()";
#elif defined(USE_EPOLL)
	const char *fnname = "epoll_wait()";
#elif defined(USE_DEVPOLL)
	isc_result_t result;
	const char *fnname = "ioctl(DP_POLL)";
	struct dvpoll dvp;
	int pass;
#if defined(ISC_SOCKET_USE_POLLWATCH)
	pollstate_t pollstate = poll_idle;
#endif /* if defined(ISC_SOCKET_USE_POLLWATCH) */
#elif defined(USE_SELECT)
	const char *fnname = "select()";
	int maxfd;
	int ctlfd;
#endif /* ifdef USE_KQUEUE */
	char strbuf[ISC_STRERRORSIZE];

#if defined(USE_SELECT)
	/*
	 * Get the control fd here.  This will never change.
	 */
	ctlfd = thread->pipe_fds[0];
#endif /* if defined(USE_SELECT) */
	done = false;
	while (!done) {
		do {
#ifdef USE_KQUEUE
			cc = kevent(thread->kqueue_fd, NULL, 0, thread->events,
				    thread->nevents, NULL);
#elif defined(USE_EPOLL)
			cc = epoll_wait(thread->epoll_fd, thread->events,
					thread->nevents, -1);
#elif defined(USE_DEVPOLL)
			/*
			 * Re-probe every thousand calls.
			 */
			if (thread->calls++ > 1000U) {
				result = isc_resource_getcurlimit(
					isc_resource_openfiles,
					&thread->open_max);
				if (result != ISC_R_SUCCESS) {
					thread->open_max = 64;
				}
				thread->calls = 0;
			}
			for (pass = 0; pass < 2; pass++) {
				dvp.dp_fds = thread->events;
				dvp.dp_nfds = thread->nevents;
				if (dvp.dp_nfds >= thread->open_max) {
					dvp.dp_nfds = thread->open_max - 1;
				}
#ifndef ISC_SOCKET_USE_POLLWATCH
				dvp.dp_timeout = -1;
#else  /* ifndef ISC_SOCKET_USE_POLLWATCH */
				if (pollstate == poll_idle) {
					dvp.dp_timeout = -1;
				} else {
					dvp.dp_timeout =
						ISC_SOCKET_POLLWATCH_TIMEOUT;
				}
#endif /* ISC_SOCKET_USE_POLLWATCH */
				cc = ioctl(thread->devpoll_fd, DP_POLL, &dvp);
				if (cc == -1 && errno == EINVAL) {
					/*
					 * {OPEN_MAX} may have dropped.  Look
					 * up the current value and try again.
					 */
					result = isc_resource_getcurlimit(
						isc_resource_openfiles,
						&thread->open_max);
					if (result != ISC_R_SUCCESS) {
						thread->open_max = 64;
					}
				} else {
					break;
				}
			}
#elif defined(USE_SELECT)
			/*
			 * We will have only one thread anyway, we can lock
			 * manager lock and don't care
			 */
			LOCK(&manager->lock);
			memmove(thread->read_fds_copy, thread->read_fds,
				thread->fd_bufsize);
			memmove(thread->write_fds_copy, thread->write_fds,
				thread->fd_bufsize);
			maxfd = thread->maxfd + 1;
			UNLOCK(&manager->lock);

			cc = select(maxfd, thread->read_fds_copy,
				    thread->write_fds_copy, NULL, NULL);
#endif /* USE_KQUEUE */

			if (cc < 0 && !SOFT_ERROR(errno)) {
				strerror_r(errno, strbuf, sizeof(strbuf));
				FATAL_ERROR(__FILE__, __LINE__, "%s failed: %s",
					    fnname, strbuf);
			}

#if defined(USE_DEVPOLL) && defined(ISC_SOCKET_USE_POLLWATCH)
			if (cc == 0) {
				if (pollstate == poll_active) {
					pollstate = poll_checking;
				} else if (pollstate == poll_checking) {
					pollstate = poll_idle;
				}
			} else if (cc > 0) {
				if (pollstate == poll_checking) {
					/*
					 * XXX: We'd like to use a more
					 * verbose log level as it's actually an
					 * unexpected event, but the kernel bug
					 * reportedly happens pretty frequently
					 * (and it can also be a false positive)
					 * so it would be just too noisy.
					 */
					thread_log(thread,
						   ISC_LOGCATEGORY_GENERAL,
						   ISC_LOGMODULE_SOCKET,
						   ISC_LOG_DEBUG(1),
						   "unexpected POLL timeout");
				}
				pollstate = poll_active;
			}
#endif /* if defined(USE_DEVPOLL) && defined(ISC_SOCKET_USE_POLLWATCH) */
		} while (cc < 0);

#if defined(USE_KQUEUE) || defined(USE_EPOLL) || defined(USE_DEVPOLL)
		done = process_fds(thread, thread->events, cc);
#elif defined(USE_SELECT)
		process_fds(thread, maxfd, thread->read_fds_copy,
			    thread->write_fds_copy);

		/*
		 * Process reads on internal, control fd.
		 */
		if (FD_ISSET(ctlfd, thread->read_fds_copy)) {
			done = process_ctlfd(thread);
		}
#endif /* if defined(USE_KQUEUE) || defined(USE_EPOLL) || defined(USE_DEVPOLL) \
	* */
	}

	thread_log(thread, TRACE, "watcher exiting");
	return ((isc_threadresult_t)0);
}

void
isc_socketmgr_setreserved(isc_socketmgr_t *manager, uint32_t reserved) {
	REQUIRE(VALID_MANAGER(manager));

	manager->reserved = reserved;
}

void
isc_socketmgr_maxudp(isc_socketmgr_t *manager, unsigned int maxudp) {
	REQUIRE(VALID_MANAGER(manager));

	manager->maxudp = maxudp;
}

/*
 * Setup socket thread, thread->manager and thread->threadid must be filled.
 */

static isc_result_t
setup_thread(isc__socketthread_t *thread) {
	isc_result_t result = ISC_R_SUCCESS;
	int i;
	char strbuf[ISC_STRERRORSIZE];

	REQUIRE(thread != NULL);
	REQUIRE(VALID_MANAGER(thread->manager));
	REQUIRE(thread->threadid >= 0 &&
		thread->threadid < thread->manager->nthreads);

	thread->fds =
		isc_mem_get(thread->manager->mctx,
			    thread->manager->maxsocks * sizeof(isc_socket_t *));

	memset(thread->fds, 0,
	       thread->manager->maxsocks * sizeof(isc_socket_t *));

	thread->fdstate = isc_mem_get(thread->manager->mctx,
				      thread->manager->maxsocks * sizeof(int));

	memset(thread->fdstate, 0, thread->manager->maxsocks * sizeof(int));

	thread->fdlock = isc_mem_get(thread->manager->mctx,
				     FDLOCK_COUNT * sizeof(isc_mutex_t));

	for (i = 0; i < FDLOCK_COUNT; i++) {
		isc_mutex_init(&thread->fdlock[i]);
	}

	if (pipe(thread->pipe_fds) != 0) {
		strerror_r(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__, "pipe() failed: %s",
				 strbuf);
		return (ISC_R_UNEXPECTED);
	}
	RUNTIME_CHECK(make_nonblock(thread->pipe_fds[0]) == ISC_R_SUCCESS);

#ifdef USE_KQUEUE
	thread->nevents = ISC_SOCKET_MAXEVENTS;
	thread->events = isc_mem_get(thread->manager->mctx,
				     sizeof(struct kevent) * thread->nevents);

	thread->kqueue_fd = kqueue();
	if (thread->kqueue_fd == -1) {
		result = isc__errno2result(errno);
		strerror_r(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__, "kqueue failed: %s",
				 strbuf);
		isc_mem_put(thread->manager->mctx, thread->events,
			    sizeof(struct kevent) * thread->nevents);
		return (result);
	}

	result = watch_fd(thread, thread->pipe_fds[0], SELECT_POKE_READ);
	if (result != ISC_R_SUCCESS) {
		close(thread->kqueue_fd);
		isc_mem_put(thread->manager->mctx, thread->events,
			    sizeof(struct kevent) * thread->nevents);
	}
	return (result);

#elif defined(USE_EPOLL)
	thread->nevents = ISC_SOCKET_MAXEVENTS;
	thread->epoll_events =
		isc_mem_get(thread->manager->mctx,
			    (thread->manager->maxsocks * sizeof(uint32_t)));

	memset(thread->epoll_events, 0,
	       thread->manager->maxsocks * sizeof(uint32_t));

	thread->events =
		isc_mem_get(thread->manager->mctx,
			    sizeof(struct epoll_event) * thread->nevents);

	thread->epoll_fd = epoll_create(thread->nevents);
	if (thread->epoll_fd == -1) {
		result = isc__errno2result(errno);
		strerror_r(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__, "epoll_create failed: %s",
				 strbuf);
		return (result);
	}

	result = watch_fd(thread, thread->pipe_fds[0], SELECT_POKE_READ);
	return (result);

#elif defined(USE_DEVPOLL)
	thread->nevents = ISC_SOCKET_MAXEVENTS;
	result = isc_resource_getcurlimit(isc_resource_openfiles,
					  &thread->open_max);
	if (result != ISC_R_SUCCESS) {
		thread->open_max = 64;
	}
	thread->calls = 0;
	thread->events = isc_mem_get(thread->manager->mctx,
				     sizeof(struct pollfd) * thread->nevents);

	/*
	 * Note: fdpollinfo should be able to support all possible FDs, so
	 * it must have maxsocks entries (not nevents).
	 */
	thread->fdpollinfo =
		isc_mem_get(thread->manager->mctx,
			    sizeof(pollinfo_t) * thread->manager->maxsocks);
	memset(thread->fdpollinfo, 0,
	       sizeof(pollinfo_t) * thread->manager->maxsocks);
	thread->devpoll_fd = open("/dev/poll", O_RDWR);
	if (thread->devpoll_fd == -1) {
		result = isc__errno2result(errno);
		strerror_r(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "open(/dev/poll) failed: %s", strbuf);
		isc_mem_put(thread->manager->mctx, thread->events,
			    sizeof(struct pollfd) * thread->nevents);
		isc_mem_put(thread->manager->mctx, thread->fdpollinfo,
			    sizeof(pollinfo_t) * thread->manager->maxsocks);
		return (result);
	}
	result = watch_fd(thread, thread->pipe_fds[0], SELECT_POKE_READ);
	if (result != ISC_R_SUCCESS) {
		close(thread->devpoll_fd);
		isc_mem_put(thread->manager->mctx, thread->events,
			    sizeof(struct pollfd) * thread->nevents);
		isc_mem_put(thread->manager->mctx, thread->fdpollinfo,
			    sizeof(pollinfo_t) * thread->manager->maxsocks);
		return (result);
	}

	return (ISC_R_SUCCESS);
#elif defined(USE_SELECT)
	UNUSED(result);

#if ISC_SOCKET_MAXSOCKETS > FD_SETSIZE
	/*
	 * Note: this code should also cover the case of MAXSOCKETS <=
	 * FD_SETSIZE, but we separate the cases to avoid possible portability
	 * issues regarding howmany() and the actual representation of fd_set.
	 */
	thread->fd_bufsize = howmany(manager->maxsocks, NFDBITS) *
			     sizeof(fd_mask);
#else  /* if ISC_SOCKET_MAXSOCKETS > FD_SETSIZE */
	thread->fd_bufsize = sizeof(fd_set);
#endif /* if ISC_SOCKET_MAXSOCKETS > FD_SETSIZE */

	thread->read_fds = isc_mem_get(thread->manager->mctx,
				       thread->fd_bufsize);
	thread->read_fds_copy = isc_mem_get(thread->manager->mctx,
					    thread->fd_bufsize);
	thread->write_fds = isc_mem_get(thread->manager->mctx,
					thread->fd_bufsize);
	thread->write_fds_copy = isc_mem_get(thread->manager->mctx,
					     thread->fd_bufsize);
	memset(thread->read_fds, 0, thread->fd_bufsize);
	memset(thread->write_fds, 0, thread->fd_bufsize);

	(void)watch_fd(thread, thread->pipe_fds[0], SELECT_POKE_READ);
	thread->maxfd = thread->pipe_fds[0];

	return (ISC_R_SUCCESS);
#endif /* USE_KQUEUE */
}

static void
cleanup_thread(isc_mem_t *mctx, isc__socketthread_t *thread) {
	isc_result_t result;
	int i;

	result = unwatch_fd(thread, thread->pipe_fds[0], SELECT_POKE_READ);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__, "epoll_ctl(DEL) failed");
	}
#ifdef USE_KQUEUE
	close(thread->kqueue_fd);
	isc_mem_put(mctx, thread->events,
		    sizeof(struct kevent) * thread->nevents);
#elif defined(USE_EPOLL)
	close(thread->epoll_fd);

	isc_mem_put(mctx, thread->events,
		    sizeof(struct epoll_event) * thread->nevents);
#elif defined(USE_DEVPOLL)
	close(thread->devpoll_fd);
	isc_mem_put(mctx, thread->events,
		    sizeof(struct pollfd) * thread->nevents);
	isc_mem_put(mctx, thread->fdpollinfo,
		    sizeof(pollinfo_t) * thread->manager->maxsocks);
#elif defined(USE_SELECT)
	if (thread->read_fds != NULL) {
		isc_mem_put(mctx, thread->read_fds, thread->fd_bufsize);
	}
	if (thread->read_fds_copy != NULL) {
		isc_mem_put(mctx, thread->read_fds_copy, thread->fd_bufsize);
	}
	if (thread->write_fds != NULL) {
		isc_mem_put(mctx, thread->write_fds, thread->fd_bufsize);
	}
	if (thread->write_fds_copy != NULL) {
		isc_mem_put(mctx, thread->write_fds_copy, thread->fd_bufsize);
	}
#endif /* USE_KQUEUE */
	for (i = 0; i < (int)thread->manager->maxsocks; i++) {
		if (thread->fdstate[i] == CLOSE_PENDING) {
			/* no need to lock */
			(void)close(i);
		}
	}

#if defined(USE_EPOLL)
	isc_mem_put(thread->manager->mctx, thread->epoll_events,
		    thread->manager->maxsocks * sizeof(uint32_t));
#endif /* if defined(USE_EPOLL) */
	isc_mem_put(thread->manager->mctx, thread->fds,
		    thread->manager->maxsocks * sizeof(isc_socket_t *));
	isc_mem_put(thread->manager->mctx, thread->fdstate,
		    thread->manager->maxsocks * sizeof(int));

	for (i = 0; i < FDLOCK_COUNT; i++) {
		isc_mutex_destroy(&thread->fdlock[i]);
	}
	isc_mem_put(thread->manager->mctx, thread->fdlock,
		    FDLOCK_COUNT * sizeof(isc_mutex_t));
}

isc_result_t
isc_socketmgr_create(isc_mem_t *mctx, isc_socketmgr_t **managerp) {
	return (isc_socketmgr_create2(mctx, managerp, 0, 1));
}

isc_result_t
isc_socketmgr_create2(isc_mem_t *mctx, isc_socketmgr_t **managerp,
		      unsigned int maxsocks, int nthreads) {
	int i;
	isc_socketmgr_t *manager;

	REQUIRE(managerp != NULL && *managerp == NULL);

	if (maxsocks == 0) {
		maxsocks = ISC_SOCKET_MAXSOCKETS;
	}

	manager = isc_mem_get(mctx, sizeof(*manager));

	/* zero-clear so that necessary cleanup on failure will be easy */
	memset(manager, 0, sizeof(*manager));
	manager->maxsocks = maxsocks;
	manager->reserved = 0;
	manager->maxudp = 0;
	manager->nthreads = nthreads;
	manager->stats = NULL;

	manager->magic = SOCKET_MANAGER_MAGIC;
	manager->mctx = NULL;
	ISC_LIST_INIT(manager->socklist);
	isc_mutex_init(&manager->lock);
	isc_condition_init(&manager->shutdown_ok);

	/*
	 * Start up the select/poll thread.
	 */
	manager->threads = isc_mem_get(mctx, sizeof(isc__socketthread_t) *
						     manager->nthreads);
	isc_mem_attach(mctx, &manager->mctx);

	for (i = 0; i < manager->nthreads; i++) {
		manager->threads[i].manager = manager;
		manager->threads[i].threadid = i;
		setup_thread(&manager->threads[i]);
		isc_thread_create(netthread, &manager->threads[i],
				  &manager->threads[i].thread);
		char tname[1024];
		sprintf(tname, "sock-%d", i);
		isc_thread_setname(manager->threads[i].thread, tname);
	}

	*managerp = manager;

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_socketmgr_getmaxsockets(isc_socketmgr_t *manager, unsigned int *nsockp) {
	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(nsockp != NULL);

	*nsockp = manager->maxsocks;

	return (ISC_R_SUCCESS);
}

void
isc_socketmgr_setstats(isc_socketmgr_t *manager, isc_stats_t *stats) {
	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(ISC_LIST_EMPTY(manager->socklist));
	REQUIRE(manager->stats == NULL);
	REQUIRE(isc_stats_ncounters(stats) == isc_sockstatscounter_max);

	isc_stats_attach(stats, &manager->stats);
}

void
isc_socketmgr_destroy(isc_socketmgr_t **managerp) {
	isc_socketmgr_t *manager;

	/*
	 * Destroy a socket manager.
	 */

	REQUIRE(managerp != NULL);
	manager = *managerp;
	REQUIRE(VALID_MANAGER(manager));

	LOCK(&manager->lock);

	/*
	 * Wait for all sockets to be destroyed.
	 */
	while (!ISC_LIST_EMPTY(manager->socklist)) {
		manager_log(manager, CREATION, "sockets exist");
		WAIT(&manager->shutdown_ok, &manager->lock);
	}

	UNLOCK(&manager->lock);

	/*
	 * Here, poke our select/poll thread.  Do this by closing the write
	 * half of the pipe, which will send EOF to the read half.
	 * This is currently a no-op in the non-threaded case.
	 */
	for (int i = 0; i < manager->nthreads; i++) {
		select_poke(manager, i, 0, SELECT_POKE_SHUTDOWN);
	}

	/*
	 * Wait for thread to exit.
	 */
	for (int i = 0; i < manager->nthreads; i++) {
		isc_thread_join(manager->threads[i].thread, NULL);
		cleanup_thread(manager->mctx, &manager->threads[i]);
	}
	/*
	 * Clean up.
	 */
	isc_mem_put(manager->mctx, manager->threads,
		    sizeof(isc__socketthread_t) * manager->nthreads);
	(void)isc_condition_destroy(&manager->shutdown_ok);

	if (manager->stats != NULL) {
		isc_stats_detach(&manager->stats);
	}
	isc_mutex_destroy(&manager->lock);
	manager->magic = 0;
	isc_mem_putanddetach(&manager->mctx, manager, sizeof(*manager));

	*managerp = NULL;
}

static isc_result_t
socket_recv(isc_socket_t *sock, isc_socketevent_t *dev, isc_task_t *task,
	    unsigned int flags) {
	int io_state;
	bool have_lock = false;
	isc_task_t *ntask = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	dev->ev_sender = task;

	if (sock->type == isc_sockettype_udp) {
		io_state = doio_recv(sock, dev);
	} else {
		LOCK(&sock->lock);
		have_lock = true;

		if (ISC_LIST_EMPTY(sock->recv_list)) {
			io_state = doio_recv(sock, dev);
		} else {
			io_state = DOIO_SOFT;
		}
	}

	switch (io_state) {
	case DOIO_SOFT:
		/*
		 * We couldn't read all or part of the request right now, so
		 * queue it.
		 *
		 * Attach to socket and to task
		 */
		isc_task_attach(task, &ntask);
		dev->attributes |= ISC_SOCKEVENTATTR_ATTACHED;

		if (!have_lock) {
			LOCK(&sock->lock);
			have_lock = true;
		}

		/*
		 * Enqueue the request.  If the socket was previously not being
		 * watched, poke the watcher to start paying attention to it.
		 */
		bool do_poke = ISC_LIST_EMPTY(sock->recv_list);
		ISC_LIST_ENQUEUE(sock->recv_list, dev, ev_link);
		if (do_poke) {
			select_poke(sock->manager, sock->threadid, sock->fd,
				    SELECT_POKE_READ);
		}

		socket_log(sock, NULL, EVENT,
			   "socket_recv: event %p -> task %p", dev, ntask);

		if ((flags & ISC_SOCKFLAG_IMMEDIATE) != 0) {
			result = ISC_R_INPROGRESS;
		}
		break;

	case DOIO_EOF:
		dev->result = ISC_R_EOF;
		FALLTHROUGH;

	case DOIO_HARD:
	case DOIO_SUCCESS:
		if ((flags & ISC_SOCKFLAG_IMMEDIATE) == 0) {
			send_recvdone_event(sock, &dev);
		}
		break;
	}

	if (have_lock) {
		UNLOCK(&sock->lock);
	}

	return (result);
}

isc_result_t
isc_socket_recv(isc_socket_t *sock, isc_region_t *region, unsigned int minimum,
		isc_task_t *task, isc_taskaction_t action, void *arg) {
	isc_socketevent_t *dev;
	isc_socketmgr_t *manager;

	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(action != NULL);

	manager = sock->manager;
	REQUIRE(VALID_MANAGER(manager));

	INSIST(sock->bound);

	dev = allocate_socketevent(manager->mctx, sock, ISC_SOCKEVENT_RECVDONE,
				   action, arg);
	if (dev == NULL) {
		return (ISC_R_NOMEMORY);
	}

	return (isc_socket_recv2(sock, region, minimum, task, dev, 0));
}

isc_result_t
isc_socket_recv2(isc_socket_t *sock, isc_region_t *region, unsigned int minimum,
		 isc_task_t *task, isc_socketevent_t *event,
		 unsigned int flags) {
	event->ev_sender = sock;
	event->result = ISC_R_UNSET;
	event->region = *region;
	event->n = 0;
	event->offset = 0;
	event->attributes = 0;

	/*
	 * UDP sockets are always partial read.
	 */
	if (sock->type == isc_sockettype_udp) {
		event->minimum = 1;
	} else {
		if (minimum == 0) {
			event->minimum = region->length;
		} else {
			event->minimum = minimum;
		}
	}

	return (socket_recv(sock, event, task, flags));
}

static isc_result_t
socket_send(isc_socket_t *sock, isc_socketevent_t *dev, isc_task_t *task,
	    const isc_sockaddr_t *address, struct in6_pktinfo *pktinfo,
	    unsigned int flags) {
	int io_state;
	bool have_lock = false;
	isc_task_t *ntask = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	dev->ev_sender = task;

	set_dev_address(address, sock, dev);
	if (pktinfo != NULL) {
		dev->attributes |= ISC_SOCKEVENTATTR_PKTINFO;
		dev->pktinfo = *pktinfo;

		if (!isc_sockaddr_issitelocal(&dev->address) &&
		    !isc_sockaddr_islinklocal(&dev->address))
		{
			socket_log(sock, NULL, TRACE,
				   "pktinfo structure provided, ifindex %u "
				   "(set to 0)",
				   pktinfo->ipi6_ifindex);

			/*
			 * Set the pktinfo index to 0 here, to let the
			 * kernel decide what interface it should send on.
			 */
			dev->pktinfo.ipi6_ifindex = 0;
		}
	}

	if (sock->type == isc_sockettype_udp) {
		io_state = doio_send(sock, dev);
	} else {
		LOCK(&sock->lock);
		have_lock = true;

		if (ISC_LIST_EMPTY(sock->send_list)) {
			io_state = doio_send(sock, dev);
		} else {
			io_state = DOIO_SOFT;
		}
	}

	switch (io_state) {
	case DOIO_SOFT:
		/*
		 * We couldn't send all or part of the request right now, so
		 * queue it unless ISC_SOCKFLAG_NORETRY is set.
		 */
		if ((flags & ISC_SOCKFLAG_NORETRY) == 0) {
			isc_task_attach(task, &ntask);
			dev->attributes |= ISC_SOCKEVENTATTR_ATTACHED;

			if (!have_lock) {
				LOCK(&sock->lock);
				have_lock = true;
			}

			/*
			 * Enqueue the request.  If the socket was previously
			 * not being watched, poke the watcher to start
			 * paying attention to it.
			 */
			bool do_poke = ISC_LIST_EMPTY(sock->send_list);
			ISC_LIST_ENQUEUE(sock->send_list, dev, ev_link);
			if (do_poke) {
				select_poke(sock->manager, sock->threadid,
					    sock->fd, SELECT_POKE_WRITE);
			}
			socket_log(sock, NULL, EVENT,
				   "socket_send: event %p -> task %p", dev,
				   ntask);

			if ((flags & ISC_SOCKFLAG_IMMEDIATE) != 0) {
				result = ISC_R_INPROGRESS;
			}
			break;
		}

		FALLTHROUGH;

	case DOIO_HARD:
	case DOIO_SUCCESS:
		if (!have_lock) {
			LOCK(&sock->lock);
			have_lock = true;
		}
		if ((flags & ISC_SOCKFLAG_IMMEDIATE) == 0) {
			send_senddone_event(sock, &dev);
		}
		break;
	}

	if (have_lock) {
		UNLOCK(&sock->lock);
	}

	return (result);
}

isc_result_t
isc_socket_send(isc_socket_t *sock, isc_region_t *region, isc_task_t *task,
		isc_taskaction_t action, void *arg) {
	/*
	 * REQUIRE() checking is performed in isc_socket_sendto().
	 */
	return (isc_socket_sendto(sock, region, task, action, arg, NULL, NULL));
}

isc_result_t
isc_socket_sendto(isc_socket_t *sock, isc_region_t *region, isc_task_t *task,
		  isc_taskaction_t action, void *arg,
		  const isc_sockaddr_t *address, struct in6_pktinfo *pktinfo) {
	isc_socketevent_t *dev;
	isc_socketmgr_t *manager;

	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(region != NULL);
	REQUIRE(task != NULL);
	REQUIRE(action != NULL);

	manager = sock->manager;
	REQUIRE(VALID_MANAGER(manager));

	INSIST(sock->bound);

	dev = allocate_socketevent(manager->mctx, sock, ISC_SOCKEVENT_SENDDONE,
				   action, arg);
	if (dev == NULL) {
		return (ISC_R_NOMEMORY);
	}

	dev->region = *region;

	return (socket_send(sock, dev, task, address, pktinfo, 0));
}

isc_result_t
isc_socket_sendto2(isc_socket_t *sock, isc_region_t *region, isc_task_t *task,
		   const isc_sockaddr_t *address, struct in6_pktinfo *pktinfo,
		   isc_socketevent_t *event, unsigned int flags) {
	REQUIRE(VALID_SOCKET(sock));
	REQUIRE((flags & ~(ISC_SOCKFLAG_IMMEDIATE | ISC_SOCKFLAG_NORETRY)) ==
		0);
	if ((flags & ISC_SOCKFLAG_NORETRY) != 0) {
		REQUIRE(sock->type == isc_sockettype_udp);
	}
	event->ev_sender = sock;
	event->result = ISC_R_UNSET;
	event->region = *region;
	event->n = 0;
	event->offset = 0;
	event->attributes &= ~ISC_SOCKEVENTATTR_ATTACHED;

	return (socket_send(sock, event, task, address, pktinfo, flags));
}

void
isc_socket_cleanunix(const isc_sockaddr_t *sockaddr, bool active) {
#ifdef ISC_PLATFORM_HAVESYSUNH
	int s;
	struct stat sb;
	char strbuf[ISC_STRERRORSIZE];

	if (sockaddr->type.sa.sa_family != AF_UNIX) {
		return;
	}

#ifndef S_ISSOCK
#if defined(S_IFMT) && defined(S_IFSOCK)
#define S_ISSOCK(mode) ((mode & S_IFMT) == S_IFSOCK)
#elif defined(_S_IFMT) && defined(S_IFSOCK)
#define S_ISSOCK(mode) ((mode & _S_IFMT) == S_IFSOCK)
#endif /* if defined(S_IFMT) && defined(S_IFSOCK) */
#endif /* ifndef S_ISSOCK */

#ifndef S_ISFIFO
#if defined(S_IFMT) && defined(S_IFIFO)
#define S_ISFIFO(mode) ((mode & S_IFMT) == S_IFIFO)
#elif defined(_S_IFMT) && defined(S_IFIFO)
#define S_ISFIFO(mode) ((mode & _S_IFMT) == S_IFIFO)
#endif /* if defined(S_IFMT) && defined(S_IFIFO) */
#endif /* ifndef S_ISFIFO */

#if !defined(S_ISFIFO) && !defined(S_ISSOCK)
/* cppcheck-suppress preprocessorErrorDirective */
#error \
	You need to define S_ISFIFO and S_ISSOCK as appropriate for your platform.  See <sys/stat.h>.
#endif /* if !defined(S_ISFIFO) && !defined(S_ISSOCK) */

#ifndef S_ISFIFO
#define S_ISFIFO(mode) 0
#endif /* ifndef S_ISFIFO */

#ifndef S_ISSOCK
#define S_ISSOCK(mode) 0
#endif /* ifndef S_ISSOCK */

	if (stat(sockaddr->type.sunix.sun_path, &sb) < 0) {
		switch (errno) {
		case ENOENT:
			if (active) { /* We exited cleanly last time */
				break;
			}
			FALLTHROUGH;
		default:
			strerror_r(errno, strbuf, sizeof(strbuf));
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_SOCKET,
				      active ? ISC_LOG_ERROR : ISC_LOG_WARNING,
				      "isc_socket_cleanunix: stat(%s): %s",
				      sockaddr->type.sunix.sun_path, strbuf);
			return;
		}
	} else {
		if (!(S_ISSOCK(sb.st_mode) || S_ISFIFO(sb.st_mode))) {
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_SOCKET,
				      active ? ISC_LOG_ERROR : ISC_LOG_WARNING,
				      "isc_socket_cleanunix: %s: not a socket",
				      sockaddr->type.sunix.sun_path);
			return;
		}
	}

	if (active) {
		if (unlink(sockaddr->type.sunix.sun_path) < 0) {
			strerror_r(errno, strbuf, sizeof(strbuf));
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_SOCKET, ISC_LOG_ERROR,
				      "isc_socket_cleanunix: unlink(%s): %s",
				      sockaddr->type.sunix.sun_path, strbuf);
		}
		return;
	}

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		strerror_r(errno, strbuf, sizeof(strbuf));
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_SOCKET, ISC_LOG_WARNING,
			      "isc_socket_cleanunix: socket(%s): %s",
			      sockaddr->type.sunix.sun_path, strbuf);
		return;
	}

	if (connect(s, (const struct sockaddr *)&sockaddr->type.sunix,
		    sizeof(sockaddr->type.sunix)) < 0)
	{
		switch (errno) {
		case ECONNREFUSED:
		case ECONNRESET:
			if (unlink(sockaddr->type.sunix.sun_path) < 0) {
				strerror_r(errno, strbuf, sizeof(strbuf));
				isc_log_write(
					isc_lctx, ISC_LOGCATEGORY_GENERAL,
					ISC_LOGMODULE_SOCKET, ISC_LOG_WARNING,
					"isc_socket_cleanunix: "
					"unlink(%s): %s",
					sockaddr->type.sunix.sun_path, strbuf);
			}
			break;
		default:
			strerror_r(errno, strbuf, sizeof(strbuf));
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_SOCKET, ISC_LOG_WARNING,
				      "isc_socket_cleanunix: connect(%s): %s",
				      sockaddr->type.sunix.sun_path, strbuf);
			break;
		}
	}
	close(s);
#else  /* ifdef ISC_PLATFORM_HAVESYSUNH */
	UNUSED(sockaddr);
	UNUSED(active);
#endif /* ifdef ISC_PLATFORM_HAVESYSUNH */
}

isc_result_t
isc_socket_permunix(const isc_sockaddr_t *sockaddr, uint32_t perm,
		    uint32_t owner, uint32_t group) {
#ifdef ISC_PLATFORM_HAVESYSUNH
	isc_result_t result = ISC_R_SUCCESS;
	char strbuf[ISC_STRERRORSIZE];
	char path[sizeof(sockaddr->type.sunix.sun_path)];
#ifdef NEED_SECURE_DIRECTORY
	char *slash;
#endif /* ifdef NEED_SECURE_DIRECTORY */

	REQUIRE(sockaddr->type.sa.sa_family == AF_UNIX);
	INSIST(strlen(sockaddr->type.sunix.sun_path) < sizeof(path));
	strlcpy(path, sockaddr->type.sunix.sun_path, sizeof(path));

#ifdef NEED_SECURE_DIRECTORY
	slash = strrchr(path, '/');
	if (slash != NULL) {
		if (slash != path) {
			*slash = '\0';
		} else {
			strlcpy(path, "/", sizeof(path));
		}
	} else {
		strlcpy(path, ".", sizeof(path));
	}
#endif /* ifdef NEED_SECURE_DIRECTORY */

	if (chmod(path, perm) < 0) {
		strerror_r(errno, strbuf, sizeof(strbuf));
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_SOCKET, ISC_LOG_ERROR,
			      "isc_socket_permunix: chmod(%s, %d): %s", path,
			      perm, strbuf);
		result = ISC_R_FAILURE;
	}
	if (chown(path, owner, group) < 0) {
		strerror_r(errno, strbuf, sizeof(strbuf));
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_SOCKET, ISC_LOG_ERROR,
			      "isc_socket_permunix: chown(%s, %d, %d): %s",
			      path, owner, group, strbuf);
		result = ISC_R_FAILURE;
	}
	return (result);
#else  /* ifdef ISC_PLATFORM_HAVESYSUNH */
	UNUSED(sockaddr);
	UNUSED(perm);
	UNUSED(owner);
	UNUSED(group);
	return (ISC_R_NOTIMPLEMENTED);
#endif /* ifdef ISC_PLATFORM_HAVESYSUNH */
}

isc_result_t
isc_socket_bind(isc_socket_t *sock, const isc_sockaddr_t *sockaddr,
		isc_socket_options_t options) {
	char strbuf[ISC_STRERRORSIZE];
	int on = 1;

	REQUIRE(VALID_SOCKET(sock));

	LOCK(&sock->lock);

	INSIST(!sock->bound);
	INSIST(!sock->dupped);

	if (sock->pf != sockaddr->type.sa.sa_family) {
		UNLOCK(&sock->lock);
		return (ISC_R_FAMILYMISMATCH);
	}

	/*
	 * Only set SO_REUSEADDR when we want a specific port.
	 */
#ifdef AF_UNIX
	if (sock->pf == AF_UNIX) {
		goto bind_socket;
	}
#endif /* ifdef AF_UNIX */
	if ((options & ISC_SOCKET_REUSEADDRESS) != 0 &&
	    isc_sockaddr_getport(sockaddr) != (in_port_t)0)
	{
		if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, (void *)&on,
			       sizeof(on)) < 0)
		{
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "setsockopt(%d) failed", sock->fd);
		}
#if defined(__FreeBSD_kernel__) && defined(SO_REUSEPORT_LB)
		if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEPORT_LB,
			       (void *)&on, sizeof(on)) < 0)
		{
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "setsockopt(%d) failed", sock->fd);
		}
#elif defined(__linux__) && defined(SO_REUSEPORT)
		if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEPORT, (void *)&on,
			       sizeof(on)) < 0)
		{
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "setsockopt(%d) failed", sock->fd);
		}
#endif		/* if defined(__FreeBSD_kernel__) && defined(SO_REUSEPORT_LB) */
		/* Press on... */
	}
#ifdef AF_UNIX
bind_socket:
#endif /* ifdef AF_UNIX */
	if (bind(sock->fd, &sockaddr->type.sa, sockaddr->length) < 0) {
		inc_stats(sock->manager->stats,
			  sock->statsindex[STATID_BINDFAIL]);

		UNLOCK(&sock->lock);
		switch (errno) {
		case EACCES:
			return (ISC_R_NOPERM);
		case EADDRNOTAVAIL:
			return (ISC_R_ADDRNOTAVAIL);
		case EADDRINUSE:
			return (ISC_R_ADDRINUSE);
		case EINVAL:
			return (ISC_R_BOUND);
		default:
			strerror_r(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__, "bind: %s",
					 strbuf);
			return (ISC_R_UNEXPECTED);
		}
	}

	socket_log(sock, sockaddr, TRACE, "bound");
	sock->bound = 1;

	UNLOCK(&sock->lock);
	return (ISC_R_SUCCESS);
}

/*
 * Enable this only for specific OS versions, and only when they have repaired
 * their problems with it.  Until then, this is is broken and needs to be
 * disabled by default.  See RT22589 for details.
 */
#undef ENABLE_ACCEPTFILTER

isc_result_t
isc_socket_filter(isc_socket_t *sock, const char *filter) {
#if defined(SO_ACCEPTFILTER) && defined(ENABLE_ACCEPTFILTER)
	char strbuf[ISC_STRERRORSIZE];
	struct accept_filter_arg afa;
#else  /* if defined(SO_ACCEPTFILTER) && defined(ENABLE_ACCEPTFILTER) */
	UNUSED(sock);
	UNUSED(filter);
#endif /* if defined(SO_ACCEPTFILTER) && defined(ENABLE_ACCEPTFILTER) */

	REQUIRE(VALID_SOCKET(sock));

#if defined(SO_ACCEPTFILTER) && defined(ENABLE_ACCEPTFILTER)
	bzero(&afa, sizeof(afa));
	strlcpy(afa.af_name, filter, sizeof(afa.af_name));
	if (setsockopt(sock->fd, SOL_SOCKET, SO_ACCEPTFILTER, &afa,
		       sizeof(afa)) == -1)
	{
		strerror_r(errno, strbuf, sizeof(strbuf));
		socket_log(sock, NULL, CREATION,
			   "setsockopt(SO_ACCEPTFILTER): %s", strbuf);
		return (ISC_R_FAILURE);
	}
	return (ISC_R_SUCCESS);
#else  /* if defined(SO_ACCEPTFILTER) && defined(ENABLE_ACCEPTFILTER) */
	return (ISC_R_NOTIMPLEMENTED);
#endif /* if defined(SO_ACCEPTFILTER) && defined(ENABLE_ACCEPTFILTER) */
}

/*
 * Try enabling TCP Fast Open for a given socket if the OS supports it.
 */
static void
set_tcp_fastopen(isc_socket_t *sock, unsigned int backlog) {
#if defined(ENABLE_TCP_FASTOPEN) && defined(TCP_FASTOPEN)
	char strbuf[ISC_STRERRORSIZE];

/*
 * FreeBSD, as of versions 10.3 and 11.0, defines TCP_FASTOPEN while also
 * shipping a default kernel without TFO support, so we special-case it by
 * performing an additional runtime check for TFO support using sysctl to
 * prevent setsockopt() errors from being logged.
 */
#if defined(__FreeBSD__) && defined(HAVE_SYSCTLBYNAME)
#define SYSCTL_TFO "net.inet.tcp.fastopen.enabled"
	unsigned int enabled;
	size_t enabledlen = sizeof(enabled);
	static bool tfo_notice_logged = false;

	if (sysctlbyname(SYSCTL_TFO, &enabled, &enabledlen, NULL, 0) < 0) {
		/*
		 * This kernel does not support TCP Fast Open.  There is
		 * nothing more we can do.
		 */
		return;
	} else if (enabled == 0) {
		/*
		 * This kernel does support TCP Fast Open, but it is disabled
		 * by sysctl.  Notify the user, but do not nag.
		 */
		if (!tfo_notice_logged) {
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_SOCKET, ISC_LOG_NOTICE,
				      "TCP_FASTOPEN support is disabled by "
				      "sysctl (" SYSCTL_TFO " = 0)");
			tfo_notice_logged = true;
		}
		return;
	}
#endif /* if defined(__FreeBSD__) && defined(HAVE_SYSCTLBYNAME) */

#ifdef __APPLE__
	backlog = 1;
#else  /* ifdef __APPLE__ */
	backlog = backlog / 2;
	if (backlog == 0) {
		backlog = 1;
	}
#endif /* ifdef __APPLE__ */
	if (setsockopt(sock->fd, IPPROTO_TCP, TCP_FASTOPEN, (void *)&backlog,
		       sizeof(backlog)) < 0)
	{
		strerror_r(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "setsockopt(%d, TCP_FASTOPEN) failed with %s",
				 sock->fd, strbuf);
		/* TCP_FASTOPEN is experimental so ignore failures */
	}
#else  /* if defined(ENABLE_TCP_FASTOPEN) && defined(TCP_FASTOPEN) */
	UNUSED(sock);
	UNUSED(backlog);
#endif /* if defined(ENABLE_TCP_FASTOPEN) && defined(TCP_FASTOPEN) */
}

/*
 * Set up to listen on a given socket.  We do this by creating an internal
 * event that will be dispatched when the socket has read activity.  The
 * watcher will send the internal event to the task when there is a new
 * connection.
 *
 * Unlike in read, we don't preallocate a done event here.  Every time there
 * is a new connection we'll have to allocate a new one anyway, so we might
 * as well keep things simple rather than having to track them.
 */
isc_result_t
isc_socket_listen(isc_socket_t *sock, unsigned int backlog) {
	char strbuf[ISC_STRERRORSIZE];

	REQUIRE(VALID_SOCKET(sock));

	LOCK(&sock->lock);

	REQUIRE(!sock->listener);
	REQUIRE(sock->bound);
	REQUIRE(sock->type == isc_sockettype_tcp ||
		sock->type == isc_sockettype_unix);

	if (backlog == 0) {
		backlog = SOMAXCONN;
	}

	if (listen(sock->fd, (int)backlog) < 0) {
		UNLOCK(&sock->lock);
		strerror_r(errno, strbuf, sizeof(strbuf));

		UNEXPECTED_ERROR(__FILE__, __LINE__, "listen: %s", strbuf);

		return (ISC_R_UNEXPECTED);
	}

	set_tcp_fastopen(sock, backlog);

	sock->listener = 1;

	UNLOCK(&sock->lock);
	return (ISC_R_SUCCESS);
}

/*
 * This should try to do aggressive accept() XXXMLG
 */
isc_result_t
isc_socket_accept(isc_socket_t *sock, isc_task_t *task, isc_taskaction_t action,
		  void *arg) {
	isc_socket_newconnev_t *dev;
	isc_socketmgr_t *manager;
	isc_task_t *ntask = NULL;
	isc_socket_t *nsock;
	isc_result_t result;
	bool do_poke = false;

	REQUIRE(VALID_SOCKET(sock));
	manager = sock->manager;
	REQUIRE(VALID_MANAGER(manager));

	LOCK(&sock->lock);

	REQUIRE(sock->listener);

	/*
	 * Sender field is overloaded here with the task we will be sending
	 * this event to.  Just before the actual event is delivered the
	 * actual ev_sender will be touched up to be the socket.
	 */
	dev = (isc_socket_newconnev_t *)isc_event_allocate(
		manager->mctx, task, ISC_SOCKEVENT_NEWCONN, action, arg,
		sizeof(*dev));
	ISC_LINK_INIT(dev, ev_link);

	result = allocate_socket(manager, sock->type, &nsock);
	if (result != ISC_R_SUCCESS) {
		isc_event_free(ISC_EVENT_PTR(&dev));
		UNLOCK(&sock->lock);
		return (result);
	}

	/*
	 * Attach to socket and to task.
	 */
	isc_task_attach(task, &ntask);
	if (isc_task_exiting(ntask)) {
		free_socket(&nsock);
		isc_task_detach(&ntask);
		isc_event_free(ISC_EVENT_PTR(&dev));
		UNLOCK(&sock->lock);
		return (ISC_R_SHUTTINGDOWN);
	}
	isc_refcount_increment0(&nsock->references);
	nsock->statsindex = sock->statsindex;

	dev->ev_sender = ntask;
	dev->newsocket = nsock;

	/*
	 * Poke watcher here.  We still have the socket locked, so there
	 * is no race condition.  We will keep the lock for such a short
	 * bit of time waking it up now or later won't matter all that much.
	 */
	do_poke = ISC_LIST_EMPTY(sock->accept_list);
	ISC_LIST_ENQUEUE(sock->accept_list, dev, ev_link);
	if (do_poke) {
		select_poke(manager, sock->threadid, sock->fd,
			    SELECT_POKE_ACCEPT);
	}
	UNLOCK(&sock->lock);
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_socket_connect(isc_socket_t *sock, const isc_sockaddr_t *addr,
		   isc_task_t *task, isc_taskaction_t action, void *arg) {
	isc_socket_connev_t *dev;
	isc_task_t *ntask = NULL;
	isc_socketmgr_t *manager;
	int cc;
	char strbuf[ISC_STRERRORSIZE];
	char addrbuf[ISC_SOCKADDR_FORMATSIZE];

	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(addr != NULL);
	REQUIRE(task != NULL);
	REQUIRE(action != NULL);

	manager = sock->manager;
	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(addr != NULL);

	if (isc_sockaddr_ismulticast(addr)) {
		return (ISC_R_MULTICAST);
	}

	LOCK(&sock->lock);

	dev = (isc_socket_connev_t *)isc_event_allocate(
		manager->mctx, sock, ISC_SOCKEVENT_CONNECT, action, arg,
		sizeof(*dev));
	ISC_LINK_INIT(dev, ev_link);

	if (sock->connecting) {
		INSIST(isc_sockaddr_equal(&sock->peer_address, addr));
		goto queue;
	}

	if (sock->connected) {
		INSIST(isc_sockaddr_equal(&sock->peer_address, addr));
		dev->result = ISC_R_SUCCESS;
		isc_task_sendto(task, ISC_EVENT_PTR(&dev), sock->threadid);

		UNLOCK(&sock->lock);

		return (ISC_R_SUCCESS);
	}

	/*
	 * Try to do the connect right away, as there can be only one
	 * outstanding, and it might happen to complete.
	 */
	sock->peer_address = *addr;
	cc = connect(sock->fd, &addr->type.sa, addr->length);
	if (cc < 0) {
		/*
		 * The socket is nonblocking and the connection cannot be
		 * completed immediately.  It is possible to select(2) or
		 * poll(2) for completion by selecting the socket for writing.
		 * After select(2) indicates writability, use getsockopt(2) to
		 * read the SO_ERROR option at level SOL_SOCKET to determine
		 * whether connect() completed successfully (SO_ERROR is zero)
		 * or unsuccessfully (SO_ERROR is one of the usual error codes
		 * listed here, explaining the reason for the failure).
		 */
		if (sock->type == isc_sockettype_udp && errno == EINPROGRESS) {
			cc = 0;
			goto success;
		}
		if (SOFT_ERROR(errno) || errno == EINPROGRESS) {
			goto queue;
		}

		switch (errno) {
#define ERROR_MATCH(a, b)        \
	case a:                  \
		dev->result = b; \
		goto err_exit;
			ERROR_MATCH(EACCES, ISC_R_NOPERM);
			ERROR_MATCH(EADDRNOTAVAIL, ISC_R_ADDRNOTAVAIL);
			ERROR_MATCH(EAFNOSUPPORT, ISC_R_ADDRNOTAVAIL);
			ERROR_MATCH(ECONNREFUSED, ISC_R_CONNREFUSED);
			ERROR_MATCH(EHOSTUNREACH, ISC_R_HOSTUNREACH);
#ifdef EHOSTDOWN
			ERROR_MATCH(EHOSTDOWN, ISC_R_HOSTUNREACH);
#endif /* ifdef EHOSTDOWN */
			ERROR_MATCH(ENETUNREACH, ISC_R_NETUNREACH);
			ERROR_MATCH(ENOBUFS, ISC_R_NORESOURCES);
			ERROR_MATCH(EPERM, ISC_R_HOSTUNREACH);
			ERROR_MATCH(EPIPE, ISC_R_NOTCONNECTED);
			ERROR_MATCH(ETIMEDOUT, ISC_R_TIMEDOUT);
			ERROR_MATCH(ECONNRESET, ISC_R_CONNECTIONRESET);
#undef ERROR_MATCH
		}

		sock->connected = 0;

		strerror_r(errno, strbuf, sizeof(strbuf));
		isc_sockaddr_format(addr, addrbuf, sizeof(addrbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__, "connect(%s) %d/%s",
				 addrbuf, errno, strbuf);

		UNLOCK(&sock->lock);
		inc_stats(sock->manager->stats,
			  sock->statsindex[STATID_CONNECTFAIL]);
		isc_event_free(ISC_EVENT_PTR(&dev));
		return (ISC_R_UNEXPECTED);

	err_exit:
		sock->connected = 0;
		isc_task_sendto(task, ISC_EVENT_PTR(&dev), sock->threadid);

		UNLOCK(&sock->lock);
		inc_stats(sock->manager->stats,
			  sock->statsindex[STATID_CONNECTFAIL]);
		return (ISC_R_SUCCESS);
	}

	/*
	 * If connect completed, fire off the done event.
	 */
success:
	if (cc == 0) {
		sock->connected = 1;
		sock->bound = 1;
		dev->result = ISC_R_SUCCESS;
		isc_task_sendto(task, ISC_EVENT_PTR(&dev), sock->threadid);

		UNLOCK(&sock->lock);

		inc_stats(sock->manager->stats,
			  sock->statsindex[STATID_CONNECT]);

		return (ISC_R_SUCCESS);
	}

queue:

	/*
	 * Attach to task.
	 */
	isc_task_attach(task, &ntask);

	dev->ev_sender = ntask;

	/*
	 * Poke watcher here.  We still have the socket locked, so there
	 * is no race condition.  We will keep the lock for such a short
	 * bit of time waking it up now or later won't matter all that much.
	 */
	bool do_poke = ISC_LIST_EMPTY(sock->connect_list);
	ISC_LIST_ENQUEUE(sock->connect_list, dev, ev_link);
	if (do_poke && !sock->connecting) {
		sock->connecting = 1;
		select_poke(manager, sock->threadid, sock->fd,
			    SELECT_POKE_CONNECT);
	}

	UNLOCK(&sock->lock);
	return (ISC_R_SUCCESS);
}

/*
 * Called when a socket with a pending connect() finishes.
 */
static void
internal_connect(isc_socket_t *sock) {
	isc_socket_connev_t *dev;
	int cc;
	isc_result_t result;
	socklen_t optlen;
	char strbuf[ISC_STRERRORSIZE];
	char peerbuf[ISC_SOCKADDR_FORMATSIZE];

	INSIST(VALID_SOCKET(sock));
	REQUIRE(sock->fd >= 0);

	/*
	 * Get the first item off the connect list.
	 * If it is empty, unlock the socket and return.
	 */
	dev = ISC_LIST_HEAD(sock->connect_list);
	if (dev == NULL) {
		INSIST(!sock->connecting);
		goto finish;
	}

	INSIST(sock->connecting);
	sock->connecting = 0;

	/*
	 * Get any possible error status here.
	 */
	optlen = sizeof(cc);
	if (getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, (void *)&cc,
		       (void *)&optlen) != 0)
	{
		cc = errno;
	} else {
		errno = cc;
	}

	if (errno != 0) {
		/*
		 * If the error is EAGAIN, just re-select on this
		 * fd and pretend nothing strange happened.
		 */
		if (SOFT_ERROR(errno) || errno == EINPROGRESS) {
			sock->connecting = 1;
			return;
		}

		inc_stats(sock->manager->stats,
			  sock->statsindex[STATID_CONNECTFAIL]);

		/*
		 * Translate other errors into ISC_R_* flavors.
		 */
		switch (errno) {
#define ERROR_MATCH(a, b)   \
	case a:             \
		result = b; \
		break;
			ERROR_MATCH(EACCES, ISC_R_NOPERM);
			ERROR_MATCH(EADDRNOTAVAIL, ISC_R_ADDRNOTAVAIL);
			ERROR_MATCH(EAFNOSUPPORT, ISC_R_ADDRNOTAVAIL);
			ERROR_MATCH(ECONNREFUSED, ISC_R_CONNREFUSED);
			ERROR_MATCH(EHOSTUNREACH, ISC_R_HOSTUNREACH);
#ifdef EHOSTDOWN
			ERROR_MATCH(EHOSTDOWN, ISC_R_HOSTUNREACH);
#endif /* ifdef EHOSTDOWN */
			ERROR_MATCH(ENETUNREACH, ISC_R_NETUNREACH);
			ERROR_MATCH(ENOBUFS, ISC_R_NORESOURCES);
			ERROR_MATCH(EPERM, ISC_R_HOSTUNREACH);
			ERROR_MATCH(EPIPE, ISC_R_NOTCONNECTED);
			ERROR_MATCH(ETIMEDOUT, ISC_R_TIMEDOUT);
			ERROR_MATCH(ECONNRESET, ISC_R_CONNECTIONRESET);
#undef ERROR_MATCH
		default:
			result = ISC_R_UNEXPECTED;
			isc_sockaddr_format(&sock->peer_address, peerbuf,
					    sizeof(peerbuf));
			strerror_r(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "internal_connect: connect(%s) %s",
					 peerbuf, strbuf);
		}
	} else {
		inc_stats(sock->manager->stats,
			  sock->statsindex[STATID_CONNECT]);
		result = ISC_R_SUCCESS;
		sock->connected = 1;
		sock->bound = 1;
	}

	do {
		dev->result = result;
		send_connectdone_event(sock, &dev);
		dev = ISC_LIST_HEAD(sock->connect_list);
	} while (dev != NULL);

finish:
	unwatch_fd(&sock->manager->threads[sock->threadid], sock->fd,
		   SELECT_POKE_CONNECT);
}

isc_result_t
isc_socket_getpeername(isc_socket_t *sock, isc_sockaddr_t *addressp) {
	isc_result_t result;

	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(addressp != NULL);

	LOCK(&sock->lock);

	if (sock->connected) {
		*addressp = sock->peer_address;
		result = ISC_R_SUCCESS;
	} else {
		result = ISC_R_NOTCONNECTED;
	}

	UNLOCK(&sock->lock);

	return (result);
}

isc_result_t
isc_socket_getsockname(isc_socket_t *sock, isc_sockaddr_t *addressp) {
	socklen_t len;
	isc_result_t result;
	char strbuf[ISC_STRERRORSIZE];

	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(addressp != NULL);

	LOCK(&sock->lock);

	if (!sock->bound) {
		result = ISC_R_NOTBOUND;
		goto out;
	}

	result = ISC_R_SUCCESS;

	len = sizeof(addressp->type);
	if (getsockname(sock->fd, &addressp->type.sa, (void *)&len) < 0) {
		strerror_r(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__, "getsockname: %s", strbuf);
		result = ISC_R_UNEXPECTED;
		goto out;
	}
	addressp->length = (unsigned int)len;

out:
	UNLOCK(&sock->lock);

	return (result);
}

/*
 * Run through the list of events on this socket, and cancel the ones
 * queued for task "task" of type "how".  "how" is a bitmask.
 */
void
isc_socket_cancel(isc_socket_t *sock, isc_task_t *task, unsigned int how) {
	REQUIRE(VALID_SOCKET(sock));

	/*
	 * Quick exit if there is nothing to do.  Don't even bother locking
	 * in this case.
	 */
	if (how == 0) {
		return;
	}

	LOCK(&sock->lock);

	/*
	 * All of these do the same thing, more or less.
	 * Each will:
	 *	o If the internal event is marked as "posted" try to
	 *	  remove it from the task's queue.  If this fails, mark it
	 *	  as canceled instead, and let the task clean it up later.
	 *	o For each I/O request for that task of that type, post
	 *	  its done event with status of "ISC_R_CANCELED".
	 *	o Reset any state needed.
	 */
	if (((how & ISC_SOCKCANCEL_RECV) != 0) &&
	    !ISC_LIST_EMPTY(sock->recv_list))
	{
		isc_socketevent_t *dev;
		isc_socketevent_t *next;
		isc_task_t *current_task;

		dev = ISC_LIST_HEAD(sock->recv_list);

		while (dev != NULL) {
			current_task = dev->ev_sender;
			next = ISC_LIST_NEXT(dev, ev_link);

			if ((task == NULL) || (task == current_task)) {
				dev->result = ISC_R_CANCELED;
				send_recvdone_event(sock, &dev);
			}
			dev = next;
		}
	}

	if (((how & ISC_SOCKCANCEL_SEND) != 0) &&
	    !ISC_LIST_EMPTY(sock->send_list))
	{
		isc_socketevent_t *dev;
		isc_socketevent_t *next;
		isc_task_t *current_task;

		dev = ISC_LIST_HEAD(sock->send_list);

		while (dev != NULL) {
			current_task = dev->ev_sender;
			next = ISC_LIST_NEXT(dev, ev_link);

			if ((task == NULL) || (task == current_task)) {
				dev->result = ISC_R_CANCELED;
				send_senddone_event(sock, &dev);
			}
			dev = next;
		}
	}

	if (((how & ISC_SOCKCANCEL_ACCEPT) != 0) &&
	    !ISC_LIST_EMPTY(sock->accept_list))
	{
		isc_socket_newconnev_t *dev;
		isc_socket_newconnev_t *next;
		isc_task_t *current_task;

		dev = ISC_LIST_HEAD(sock->accept_list);
		while (dev != NULL) {
			current_task = dev->ev_sender;
			next = ISC_LIST_NEXT(dev, ev_link);

			if ((task == NULL) || (task == current_task)) {
				ISC_LIST_UNLINK(sock->accept_list, dev,
						ev_link);

				isc_refcount_decrementz(
					&NEWCONNSOCK(dev)->references);
				free_socket((isc_socket_t **)&dev->newsocket);

				dev->result = ISC_R_CANCELED;
				dev->ev_sender = sock;
				isc_task_sendtoanddetach(&current_task,
							 ISC_EVENT_PTR(&dev),
							 sock->threadid);
			}

			dev = next;
		}
	}

	if (((how & ISC_SOCKCANCEL_CONNECT) != 0) &&
	    !ISC_LIST_EMPTY(sock->connect_list))
	{
		isc_socket_connev_t *dev;
		isc_socket_connev_t *next;
		isc_task_t *current_task;

		INSIST(sock->connecting);
		sock->connecting = 0;

		dev = ISC_LIST_HEAD(sock->connect_list);

		while (dev != NULL) {
			current_task = dev->ev_sender;
			next = ISC_LIST_NEXT(dev, ev_link);

			if ((task == NULL) || (task == current_task)) {
				dev->result = ISC_R_CANCELED;
				send_connectdone_event(sock, &dev);
			}
			dev = next;
		}
	}

	UNLOCK(&sock->lock);
}

isc_sockettype_t
isc_socket_gettype(isc_socket_t *sock) {
	REQUIRE(VALID_SOCKET(sock));

	return (sock->type);
}

void
isc_socket_ipv6only(isc_socket_t *sock, bool yes) {
#if defined(IPV6_V6ONLY) && !defined(__OpenBSD__)
	int onoff = yes ? 1 : 0;
#else  /* if defined(IPV6_V6ONLY) */
	UNUSED(yes);
	UNUSED(sock);
#endif /* if defined(IPV6_V6ONLY) */

	REQUIRE(VALID_SOCKET(sock));
	INSIST(!sock->dupped);

#if defined(IPV6_V6ONLY) && !defined(__OpenBSD__)
	if (sock->pf == AF_INET6) {
		if (setsockopt(sock->fd, IPPROTO_IPV6, IPV6_V6ONLY,
			       (void *)&onoff, sizeof(int)) < 0)
		{
			char strbuf[ISC_STRERRORSIZE];
			strerror_r(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "setsockopt(%d, IPV6_V6ONLY) failed: "
					 "%s",
					 sock->fd, strbuf);
		}
	}
#endif /* ifdef IPV6_V6ONLY */
}

static void
setdscp(isc_socket_t *sock, isc_dscp_t dscp) {
#if defined(IP_TOS) || defined(IPV6_TCLASS)
	int value = dscp << 2;
#endif /* if defined(IP_TOS) || defined(IPV6_TCLASS) */

	sock->dscp = dscp;

#ifdef IP_TOS
	if (sock->pf == AF_INET) {
		if (setsockopt(sock->fd, IPPROTO_IP, IP_TOS, (void *)&value,
			       sizeof(value)) < 0)
		{
			char strbuf[ISC_STRERRORSIZE];
			strerror_r(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "setsockopt(%d, IP_TOS, %.02x) "
					 "failed: %s",
					 sock->fd, value >> 2, strbuf);
		}
	}
#endif /* ifdef IP_TOS */
#ifdef IPV6_TCLASS
	if (sock->pf == AF_INET6) {
		if (setsockopt(sock->fd, IPPROTO_IPV6, IPV6_TCLASS,
			       (void *)&value, sizeof(value)) < 0)
		{
			char strbuf[ISC_STRERRORSIZE];
			strerror_r(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "setsockopt(%d, IPV6_TCLASS, %.02x) "
					 "failed: %s",
					 sock->fd, dscp >> 2, strbuf);
		}
	}
#endif /* ifdef IPV6_TCLASS */
}

void
isc_socket_dscp(isc_socket_t *sock, isc_dscp_t dscp) {
	REQUIRE(VALID_SOCKET(sock));
	REQUIRE(dscp < 0x40);

#if !defined(IP_TOS) && !defined(IPV6_TCLASS)
	UNUSED(dscp);
#else  /* if !defined(IP_TOS) && !defined(IPV6_TCLASS) */
	if (dscp < 0) {
		return;
	}

	/* The DSCP value must not be changed once it has been set. */
	if (isc_dscp_check_value != -1) {
		INSIST(dscp == isc_dscp_check_value);
	}
#endif /* if !defined(IP_TOS) && !defined(IPV6_TCLASS) */

#ifdef notyet
	REQUIRE(!sock->dupped);
#endif /* ifdef notyet */

	setdscp(sock, dscp);
}

isc_socketevent_t *
isc_socket_socketevent(isc_mem_t *mctx, void *sender, isc_eventtype_t eventtype,
		       isc_taskaction_t action, void *arg) {
	return (allocate_socketevent(mctx, sender, eventtype, action, arg));
}

void
isc_socket_setname(isc_socket_t *sock, const char *name, void *tag) {
	/*
	 * Name 'sock'.
	 */

	REQUIRE(VALID_SOCKET(sock));

	LOCK(&sock->lock);
	strlcpy(sock->name, name, sizeof(sock->name));
	sock->tag = tag;
	UNLOCK(&sock->lock);
}

const char *
isc_socket_getname(isc_socket_t *sock) {
	return (sock->name);
}

void *
isc_socket_gettag(isc_socket_t *sock) {
	return (sock->tag);
}

int
isc_socket_getfd(isc_socket_t *sock) {
	return ((short)sock->fd);
}

static isc_once_t hasreuseport_once = ISC_ONCE_INIT;
static bool hasreuseport = false;

static void
init_hasreuseport(void) {
/*
 * SO_REUSEPORT works very differently on *BSD and on Linux (because why not).
 * We only want to use it on Linux, if it's available. On BSD we want to dup()
 * sockets instead of re-binding them.
 */
#if (defined(SO_REUSEPORT) && defined(__linux__)) || \
	(defined(SO_REUSEPORT_LB) && defined(__FreeBSD_kernel__))
	int sock, yes = 1;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		sock = socket(AF_INET6, SOCK_DGRAM, 0);
		if (sock < 0) {
			return;
		}
	}
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&yes,
		       sizeof(yes)) < 0)
	{
		close(sock);
		return;
#if defined(__FreeBSD_kernel__)
	} else if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT_LB, (void *)&yes,
			      sizeof(yes)) < 0)
#else  /* if defined(__FreeBSD_kernel__) */
	} else if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (void *)&yes,
			      sizeof(yes)) < 0)
#endif /* if defined(__FreeBSD_kernel__) */
	{
		close(sock);
		return;
	}
	hasreuseport = true;
	close(sock);
#endif /* if (defined(SO_REUSEPORT) && defined(__linux__)) || \
	* (defined(SO_REUSEPORT_LB) && defined(__FreeBSD_kernel__)) */
}

bool
isc_socket_hasreuseport() {
	RUNTIME_CHECK(isc_once_do(&hasreuseport_once, init_hasreuseport) ==
		      ISC_R_SUCCESS);
	return (hasreuseport);
}

#if defined(HAVE_LIBXML2) || defined(HAVE_JSON_C)
static const char *
_socktype(isc_sockettype_t type) {
	switch (type) {
	case isc_sockettype_udp:
		return ("udp");
	case isc_sockettype_tcp:
		return ("tcp");
	case isc_sockettype_unix:
		return ("unix");
	case isc_sockettype_fdwatch:
		return ("fdwatch");
	default:
		return ("not-initialized");
	}
}
#endif /* if defined(HAVE_LIBXML2) || defined(HAVE_JSON_C) */

#ifdef HAVE_LIBXML2
#define TRY0(a)                     \
	do {                        \
		xmlrc = (a);        \
		if (xmlrc < 0)      \
			goto error; \
	} while (0)
int
isc_socketmgr_renderxml(isc_socketmgr_t *mgr, void *writer0) {
	isc_socket_t *sock = NULL;
	char peerbuf[ISC_SOCKADDR_FORMATSIZE];
	isc_sockaddr_t addr;
	socklen_t len;
	int xmlrc;
	xmlTextWriterPtr writer = (xmlTextWriterPtr)writer0;

	LOCK(&mgr->lock);

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "sockets"));
	sock = ISC_LIST_HEAD(mgr->socklist);
	while (sock != NULL) {
		LOCK(&sock->lock);
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "socket"));

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "id"));
		TRY0(xmlTextWriterWriteFormatString(writer, "%p", sock));
		TRY0(xmlTextWriterEndElement(writer));

		if (sock->name[0] != 0) {
			TRY0(xmlTextWriterStartElement(writer,
						       ISC_XMLCHAR "name"));
			TRY0(xmlTextWriterWriteFormatString(writer, "%s",
							    sock->name));
			TRY0(xmlTextWriterEndElement(writer)); /* name */
		}

		TRY0(xmlTextWriterStartElement(writer,
					       ISC_XMLCHAR "references"));
		TRY0(xmlTextWriterWriteFormatString(
			writer, "%d",
			(int)isc_refcount_current(&sock->references)));
		TRY0(xmlTextWriterEndElement(writer));

		TRY0(xmlTextWriterWriteElement(
			writer, ISC_XMLCHAR "type",
			ISC_XMLCHAR _socktype(sock->type)));

		if (sock->connected) {
			isc_sockaddr_format(&sock->peer_address, peerbuf,
					    sizeof(peerbuf));
			TRY0(xmlTextWriterWriteElement(
				writer, ISC_XMLCHAR "peer-address",
				ISC_XMLCHAR peerbuf));
		}

		len = sizeof(addr);
		if (getsockname(sock->fd, &addr.type.sa, (void *)&len) == 0) {
			isc_sockaddr_format(&addr, peerbuf, sizeof(peerbuf));
			TRY0(xmlTextWriterWriteElement(
				writer, ISC_XMLCHAR "local-address",
				ISC_XMLCHAR peerbuf));
		}

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "states"));
		if (sock->listener) {
			TRY0(xmlTextWriterWriteElement(writer,
						       ISC_XMLCHAR "state",
						       ISC_XMLCHAR "listener"));
		}
		if (sock->connected) {
			TRY0(xmlTextWriterWriteElement(
				writer, ISC_XMLCHAR "state",
				ISC_XMLCHAR "connected"));
		}
		if (sock->connecting) {
			TRY0(xmlTextWriterWriteElement(
				writer, ISC_XMLCHAR "state",
				ISC_XMLCHAR "connecting"));
		}
		if (sock->bound) {
			TRY0(xmlTextWriterWriteElement(writer,
						       ISC_XMLCHAR "state",
						       ISC_XMLCHAR "bound"));
		}

		TRY0(xmlTextWriterEndElement(writer)); /* states */

		TRY0(xmlTextWriterEndElement(writer)); /* socket */

		UNLOCK(&sock->lock);
		sock = ISC_LIST_NEXT(sock, link);
	}
	TRY0(xmlTextWriterEndElement(writer)); /* sockets */

error:
	if (sock != NULL) {
		UNLOCK(&sock->lock);
	}

	UNLOCK(&mgr->lock);

	return (xmlrc);
}
#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
#define CHECKMEM(m)                              \
	do {                                     \
		if (m == NULL) {                 \
			result = ISC_R_NOMEMORY; \
			goto error;              \
		}                                \
	} while (0)

isc_result_t
isc_socketmgr_renderjson(isc_socketmgr_t *mgr, void *stats0) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_socket_t *sock = NULL;
	char peerbuf[ISC_SOCKADDR_FORMATSIZE];
	isc_sockaddr_t addr;
	socklen_t len;
	json_object *obj, *array = json_object_new_array();
	json_object *stats = (json_object *)stats0;

	CHECKMEM(array);

	LOCK(&mgr->lock);

	sock = ISC_LIST_HEAD(mgr->socklist);
	while (sock != NULL) {
		json_object *states, *entry = json_object_new_object();
		char buf[255];

		CHECKMEM(entry);
		json_object_array_add(array, entry);

		LOCK(&sock->lock);

		snprintf(buf, sizeof(buf), "%p", sock);
		obj = json_object_new_string(buf);
		CHECKMEM(obj);
		json_object_object_add(entry, "id", obj);

		if (sock->name[0] != 0) {
			obj = json_object_new_string(sock->name);
			CHECKMEM(obj);
			json_object_object_add(entry, "name", obj);
		}

		obj = json_object_new_int(
			(int)isc_refcount_current(&sock->references));
		CHECKMEM(obj);
		json_object_object_add(entry, "references", obj);

		obj = json_object_new_string(_socktype(sock->type));
		CHECKMEM(obj);
		json_object_object_add(entry, "type", obj);

		if (sock->connected) {
			isc_sockaddr_format(&sock->peer_address, peerbuf,
					    sizeof(peerbuf));
			obj = json_object_new_string(peerbuf);
			CHECKMEM(obj);
			json_object_object_add(entry, "peer-address", obj);
		}

		len = sizeof(addr);
		if (getsockname(sock->fd, &addr.type.sa, (void *)&len) == 0) {
			isc_sockaddr_format(&addr, peerbuf, sizeof(peerbuf));
			obj = json_object_new_string(peerbuf);
			CHECKMEM(obj);
			json_object_object_add(entry, "local-address", obj);
		}

		states = json_object_new_array();
		CHECKMEM(states);
		json_object_object_add(entry, "states", states);

		if (sock->listener) {
			obj = json_object_new_string("listener");
			CHECKMEM(obj);
			json_object_array_add(states, obj);
		}

		if (sock->connected) {
			obj = json_object_new_string("connected");
			CHECKMEM(obj);
			json_object_array_add(states, obj);
		}

		if (sock->connecting) {
			obj = json_object_new_string("connecting");
			CHECKMEM(obj);
			json_object_array_add(states, obj);
		}

		if (sock->bound) {
			obj = json_object_new_string("bound");
			CHECKMEM(obj);
			json_object_array_add(states, obj);
		}

		UNLOCK(&sock->lock);
		sock = ISC_LIST_NEXT(sock, link);
	}

	json_object_object_add(stats, "sockets", array);
	array = NULL;
	result = ISC_R_SUCCESS;

error:
	if (array != NULL) {
		json_object_put(array);
	}

	if (sock != NULL) {
		UNLOCK(&sock->lock);
	}

	UNLOCK(&mgr->lock);

	return (result);
}
#endif /* HAVE_JSON_C */

/*
 * Create a new 'type' socket managed by 'manager'.  Events
 * will be posted to 'task' and when dispatched 'action' will be
 * called with 'arg' as the arg value.  The new socket is returned
 * in 'socketp'.
 */
isc_result_t
isc_socket_fdwatchcreate(isc_socketmgr_t *manager, int fd, int flags,
			 isc_sockfdwatch_t callback, void *cbarg,
			 isc_task_t *task, isc_socket_t **socketp)
{
	isc_socket_t *sock = NULL;
	isc__socketthread_t *thread;
	isc_result_t result;
	int lockid;

	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(socketp != NULL && *socketp == NULL);

	if (fd < 0 || (unsigned int)fd >= manager->maxsocks)
		return (ISC_R_RANGE);

	result = allocate_socket(manager, isc_sockettype_fdwatch, &sock);
	if (result != ISC_R_SUCCESS)
		return (result);

	sock->fd = fd;
	sock->fdwatcharg = cbarg;
	sock->fdwatchcb = callback;
	sock->fdwatchflags = flags;
	sock->fdwatchtask = task;

	sock->threadid = gen_threadid(sock);
	isc_refcount_init(&sock->references, 1);
	thread = &manager->threads[sock->threadid];
	*socketp = (isc_socket_t *)sock;

	/*
	 * Note we don't have to lock the socket like we normally would because
	 * there are no external references to it yet.
	 */

	lockid = FDLOCK_ID(sock->fd);
	LOCK(&thread->fdlock[lockid]);
	thread->fds[sock->fd] = sock;
	thread->fdstate[sock->fd] = MANAGED;

#if defined(USE_EPOLL)
	manager->epoll_events[sock->fd] = 0;
#endif
#ifdef USE_DEVPOLL
	INSIST(thread->fdpollinfo[sock->fd].want_read == 0 &&
	       thread->fdpollinfo[sock->fd].want_write == 0);
#endif /* ifdef USE_DEVPOLL */
	UNLOCK(&thread->fdlock[lockid]);

	LOCK(&manager->lock);
	ISC_LIST_APPEND(manager->socklist, sock, link);
#ifdef USE_SELECT
	if (thread->maxfd < sock->fd)
		thread->maxfd = sock->fd;
#endif
	UNLOCK(&manager->lock);

	sock->active = 1;
	if (flags & ISC_SOCKFDWATCH_READ)
		select_poke(sock->manager, sock->threadid, sock->fd,
		    SELECT_POKE_READ);
	if (flags & ISC_SOCKFDWATCH_WRITE)
		select_poke(sock->manager, sock->threadid, sock->fd,
		    SELECT_POKE_WRITE);

	socket_log(sock, NULL, CREATION, "fdwatch-created");

	return (ISC_R_SUCCESS);
}

/*
 * Indicate to the manager that it should watch the socket again.
 * This can be used to restart watching if the previous event handler
 * didn't indicate there was more data to be processed.  Primarily
 * it is for writing but could be used for reading if desired
 */

isc_result_t
isc_socket_fdwatchpoke(isc_socket_t *sock, int flags)
{
	REQUIRE(VALID_SOCKET(sock));

	/*
	 * We check both flags first to allow us to get the lock
	 * once but only if we need it.
	 */

	if ((flags & (ISC_SOCKFDWATCH_READ | ISC_SOCKFDWATCH_WRITE)) != 0) {
		LOCK(&sock->lock);
		if ((flags & ISC_SOCKFDWATCH_READ) != 0)
			select_poke(sock->manager, sock->threadid, sock->fd,
				    SELECT_POKE_READ);
		if ((flags & ISC_SOCKFDWATCH_WRITE) != 0)
			select_poke(sock->manager, sock->threadid, sock->fd,
				    SELECT_POKE_WRITE);
		UNLOCK(&sock->lock);
	}

	socket_log(sock, NULL, TRACE, "fdwatch-poked flags: %d", flags);

	return (ISC_R_SUCCESS);
}
