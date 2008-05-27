/* $OpenLDAP: pkg/ldap/servers/slapd/daemon.c,v 1.380.2.12 2008/05/27 20:12:44 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
 * Portions Copyright 2007 by Howard Chu, Symas Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include "slap.h"
#include "ldap_pvt_thread.h"
#include "lutil.h"

#include "ldap_rq.h"

#if defined(HAVE_SYS_EPOLL_H) && defined(HAVE_EPOLL)
# include <sys/epoll.h>
#elif defined(SLAP_X_DEVPOLL) && defined(HAVE_SYS_DEVPOLL_H) && defined(HAVE_DEVPOLL)
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <sys/devpoll.h>
#endif /* ! epoll && ! /dev/poll */

#ifdef HAVE_TCPD
int allow_severity = LOG_INFO;
int deny_severity = LOG_NOTICE;
#endif /* TCP Wrappers */

#ifdef LDAP_PF_LOCAL
# include <sys/stat.h>
/* this should go in <ldap.h> as soon as it is accepted */
# define LDAPI_MOD_URLEXT		"x-mod"
#endif /* LDAP_PF_LOCAL */

#ifdef LDAP_PF_INET6
int slap_inet4or6 = AF_UNSPEC;
#else /* ! INETv6 */
int slap_inet4or6 = AF_INET;
#endif /* ! INETv6 */

/* globals */
time_t starttime;
ber_socket_t dtblsize;
slap_ssf_t local_ssf = LDAP_PVT_SASL_LOCAL_SSF;
struct runqueue_s slapd_rq;

Listener **slap_listeners = NULL;

#ifndef SLAPD_LISTEN_BACKLOG
#define SLAPD_LISTEN_BACKLOG 1024
#endif /* ! SLAPD_LISTEN_BACKLOG */

static ber_socket_t wake_sds[2];
static int emfile;

static volatile int waking;
#ifdef NO_THREADS
#define WAKE_LISTENER(w)	do { \
	if ((w) && ++waking < 5) { \
		tcp_write( SLAP_FD2SOCK(wake_sds[1]), "0", 1 ); \
	} \
} while (0)
#else /* ! NO_THREADS */
#define WAKE_LISTENER(w)	do { \
	if (w) { \
		tcp_write( SLAP_FD2SOCK(wake_sds[1]), "0", 1 ); \
	} \
} while (0)
#endif /* ! NO_THREADS */

volatile sig_atomic_t slapd_shutdown = 0;
volatile sig_atomic_t slapd_gentle_shutdown = 0;
volatile sig_atomic_t slapd_abrupt_shutdown = 0;

#ifdef HAVE_WINSOCK
ldap_pvt_thread_mutex_t slapd_ws_mutex;
SOCKET *slapd_ws_sockets;
#define	SD_READ 1
#define	SD_WRITE	2
#define	SD_ACTIVE	4
#define	SD_LISTENER	8
#endif

static struct slap_daemon {
	ldap_pvt_thread_mutex_t	sd_mutex;
#ifdef HAVE_TCPD
	ldap_pvt_thread_mutex_t	sd_tcpd_mutex;
#endif /* TCP Wrappers */

	ber_socket_t		sd_nactives;
	int			sd_nwriters;
	int			sd_nfds;

#if defined(HAVE_EPOLL)
	struct epoll_event	*sd_epolls;
	int			*sd_index;
	int			sd_epfd;
#elif defined(SLAP_X_DEVPOLL) && defined(HAVE_DEVPOLL)
	/* eXperimental */
	struct pollfd		*sd_pollfd;
	int			*sd_index;
	Listener		**sd_l;
	int			sd_dpfd;
#else /* ! epoll && ! /dev/poll */
#ifdef HAVE_WINSOCK
	char	*sd_flags;
	char	*sd_rflags;
#else /* ! HAVE_WINSOCK */
	fd_set			sd_actives;
	fd_set			sd_readers;
	fd_set			sd_writers;
#endif /* ! HAVE_WINSOCK */
#endif /* ! epoll && ! /dev/poll */
} slap_daemon;

/*
 * NOTE: naming convention for macros:
 *
 * - SLAP_SOCK_* and SLAP_EVENT_* for public interface that deals
 *   with file descriptors and events respectively
 *
 * - SLAP_<type>_* for private interface; type by now is one of
 *   EPOLL, DEVPOLL, SELECT
 *
 * private interface should not be used in the code.
 */
#if defined(HAVE_EPOLL)
/***************************************
 * Use epoll infrastructure - epoll(4) *
 ***************************************/
# define SLAP_EVENT_FNAME		"epoll"
# define SLAP_EVENTS_ARE_INDEXED	0
# define SLAP_EPOLL_SOCK_IX(s)		(slap_daemon.sd_index[(s)])
# define SLAP_EPOLL_SOCK_EP(s)		(slap_daemon.sd_epolls[SLAP_EPOLL_SOCK_IX(s)])
# define SLAP_EPOLL_SOCK_EV(s)		(SLAP_EPOLL_SOCK_EP(s).events)
# define SLAP_SOCK_IS_ACTIVE(s)		(SLAP_EPOLL_SOCK_IX(s) != -1)
# define SLAP_SOCK_NOT_ACTIVE(s)	(SLAP_EPOLL_SOCK_IX(s) == -1)
# define SLAP_EPOLL_SOCK_IS_SET(s, mode)	(SLAP_EPOLL_SOCK_EV(s) & (mode))

# define SLAP_SOCK_IS_READ(s)		SLAP_EPOLL_SOCK_IS_SET((s), EPOLLIN)
# define SLAP_SOCK_IS_WRITE(s)		SLAP_EPOLL_SOCK_IS_SET((s), EPOLLOUT)

# define SLAP_EPOLL_SOCK_SET(s, mode)	do { \
	if ( (SLAP_EPOLL_SOCK_EV(s) & (mode)) != (mode) ) {	\
		SLAP_EPOLL_SOCK_EV(s) |= (mode); \
		epoll_ctl( slap_daemon.sd_epfd, EPOLL_CTL_MOD, (s), \
			&SLAP_EPOLL_SOCK_EP(s) ); \
	} \
} while (0)

# define SLAP_EPOLL_SOCK_CLR(s, mode)	do { \
	if ( (SLAP_EPOLL_SOCK_EV(s) & (mode)) ) { \
		SLAP_EPOLL_SOCK_EV(s) &= ~(mode);	\
		epoll_ctl( slap_daemon.sd_epfd, EPOLL_CTL_MOD, s, \
			&SLAP_EPOLL_SOCK_EP(s) ); \
	} \
} while (0)

# define SLAP_SOCK_SET_READ(s)		SLAP_EPOLL_SOCK_SET(s, EPOLLIN)
# define SLAP_SOCK_SET_WRITE(s)		SLAP_EPOLL_SOCK_SET(s, EPOLLOUT)

# define SLAP_SOCK_CLR_READ(s)		SLAP_EPOLL_SOCK_CLR((s), EPOLLIN)
# define SLAP_SOCK_CLR_WRITE(s)		SLAP_EPOLL_SOCK_CLR((s), EPOLLOUT)

#  define SLAP_SOCK_SET_SUSPEND(s) \
	( slap_daemon.sd_suspend[SLAP_EPOLL_SOCK_IX(s)] = 1 )
#  define SLAP_SOCK_CLR_SUSPEND(s) \
	( slap_daemon.sd_suspend[SLAP_EPOLL_SOCK_IX(s)] = 0 )
#  define SLAP_SOCK_IS_SUSPEND(s) \
	( slap_daemon.sd_suspend[SLAP_EPOLL_SOCK_IX(s)] == 1 )

# define SLAP_EPOLL_EVENT_CLR(i, mode)	(revents[(i)].events &= ~(mode))

# define SLAP_EVENT_MAX			slap_daemon.sd_nfds

/* If a Listener address is provided, store that as the epoll data.
 * Otherwise, store the address of this socket's slot in the
 * index array. If we can't do this add, the system is out of
 * resources and we need to shutdown.
 */
# define SLAP_SOCK_ADD(s, l)		do { \
	int rc; \
	SLAP_EPOLL_SOCK_IX((s)) = slap_daemon.sd_nfds; \
	SLAP_EPOLL_SOCK_EP((s)).data.ptr = (l) ? (l) : (void *)(&SLAP_EPOLL_SOCK_IX(s)); \
	SLAP_EPOLL_SOCK_EV((s)) = EPOLLIN; \
	rc = epoll_ctl(slap_daemon.sd_epfd, EPOLL_CTL_ADD, \
		(s), &SLAP_EPOLL_SOCK_EP((s))); \
	if ( rc == 0 ) { \
		slap_daemon.sd_nfds++; \
	} else { \
		Debug( LDAP_DEBUG_ANY, \
			"daemon: epoll_ctl(ADD,fd=%d) failed, errno=%d, shutting down\n", \
			s, errno, 0 ); \
		slapd_shutdown = 2; \
	} \
} while (0)

# define SLAP_EPOLL_EV_LISTENER(ptr) \
	(((int *)(ptr) >= slap_daemon.sd_index && \
	(int *)(ptr) <= &slap_daemon.sd_index[dtblsize]) ? 0 : 1 )

# define SLAP_EPOLL_EV_PTRFD(ptr)		(SLAP_EPOLL_EV_LISTENER(ptr) ? \
	((Listener *)ptr)->sl_sd : \
	(ber_socket_t) ((int *)(ptr) - slap_daemon.sd_index))

# define SLAP_SOCK_DEL(s)		do { \
	int fd, rc, index = SLAP_EPOLL_SOCK_IX((s)); \
	if ( index < 0 ) break; \
	rc = epoll_ctl(slap_daemon.sd_epfd, EPOLL_CTL_DEL, \
		(s), &SLAP_EPOLL_SOCK_EP((s))); \
	slap_daemon.sd_epolls[index] = \
		slap_daemon.sd_epolls[slap_daemon.sd_nfds-1]; \
	fd = SLAP_EPOLL_EV_PTRFD(slap_daemon.sd_epolls[index].data.ptr); \
	slap_daemon.sd_index[fd] = index; \
	slap_daemon.sd_index[(s)] = -1; \
	slap_daemon.sd_nfds--; \
} while (0)

# define SLAP_EVENT_CLR_READ(i)		SLAP_EPOLL_EVENT_CLR((i), EPOLLIN)
# define SLAP_EVENT_CLR_WRITE(i)	SLAP_EPOLL_EVENT_CLR((i), EPOLLOUT)

# define SLAP_EPOLL_EVENT_CHK(i, mode)	(revents[(i)].events & mode)

# define SLAP_EVENT_IS_READ(i)		SLAP_EPOLL_EVENT_CHK((i), EPOLLIN)
# define SLAP_EVENT_IS_WRITE(i)		SLAP_EPOLL_EVENT_CHK((i), EPOLLOUT)
# define SLAP_EVENT_IS_LISTENER(i)	SLAP_EPOLL_EV_LISTENER(revents[(i)].data.ptr)
# define SLAP_EVENT_LISTENER(i)		((Listener *)(revents[(i)].data.ptr))

# define SLAP_EVENT_FD(i)		SLAP_EPOLL_EV_PTRFD(revents[(i)].data.ptr)

# define SLAP_SOCK_INIT		do { \
	slap_daemon.sd_epolls = ch_calloc(1, \
		( sizeof(struct epoll_event) * 2 \
			+ sizeof(int) ) * dtblsize * 2); \
	slap_daemon.sd_index = (int *)&slap_daemon.sd_epolls[ 2 * dtblsize ]; \
	slap_daemon.sd_epfd = epoll_create( dtblsize ); \
	for ( i = 0; i < dtblsize; i++ ) slap_daemon.sd_index[i] = -1; \
} while (0)

# define SLAP_SOCK_DESTROY		do { \
	if ( slap_daemon.sd_epolls != NULL ) { \
		ch_free( slap_daemon.sd_epolls ); \
		slap_daemon.sd_epolls = NULL; \
		slap_daemon.sd_index = NULL; \
		close( slap_daemon.sd_epfd ); \
	} \
} while ( 0 )

# define SLAP_EVENT_DECL		struct epoll_event *revents

# define SLAP_EVENT_INIT		do { \
	revents = slap_daemon.sd_epolls + dtblsize; \
} while (0)

# define SLAP_EVENT_WAIT(tvp, nsp)	do { \
	*(nsp) = epoll_wait( slap_daemon.sd_epfd, revents, \
		dtblsize, (tvp) ? (tvp)->tv_sec * 1000 : -1 ); \
} while (0)

#elif defined(SLAP_X_DEVPOLL) && defined(HAVE_DEVPOLL)

/*************************************************************
 * Use Solaris' (>= 2.7) /dev/poll infrastructure - poll(7d) *
 *************************************************************/
# define SLAP_EVENT_FNAME		"/dev/poll"
# define SLAP_EVENTS_ARE_INDEXED	0
/*
 * - sd_index	is used much like with epoll()
 * - sd_l	is maintained as an array containing the address
 *		of the listener; the index is the fd itself
 * - sd_pollfd	is used to keep track of what data has been
 *		registered in /dev/poll
 */
# define SLAP_DEVPOLL_SOCK_IX(s)	(slap_daemon.sd_index[(s)])
# define SLAP_DEVPOLL_SOCK_LX(s)	(slap_daemon.sd_l[(s)])
# define SLAP_DEVPOLL_SOCK_EP(s)	(slap_daemon.sd_pollfd[SLAP_DEVPOLL_SOCK_IX((s))])
# define SLAP_DEVPOLL_SOCK_FD(s)	(SLAP_DEVPOLL_SOCK_EP((s)).fd)
# define SLAP_DEVPOLL_SOCK_EV(s)	(SLAP_DEVPOLL_SOCK_EP((s)).events)
# define SLAP_SOCK_IS_ACTIVE(s)		(SLAP_DEVPOLL_SOCK_IX((s)) != -1)
# define SLAP_SOCK_NOT_ACTIVE(s)	(SLAP_DEVPOLL_SOCK_IX((s)) == -1)
# define SLAP_SOCK_IS_SET(s, mode)	(SLAP_DEVPOLL_SOCK_EV((s)) & (mode))

# define SLAP_SOCK_IS_READ(s)		SLAP_SOCK_IS_SET((s), POLLIN)
# define SLAP_SOCK_IS_WRITE(s)		SLAP_SOCK_IS_SET((s), POLLOUT)

/* as far as I understand, any time we need to communicate with the kernel
 * about the number and/or properties of a file descriptor we need it to
 * wait for, we have to rewrite the whole set */
# define SLAP_DEVPOLL_WRITE_POLLFD(s, pfd, n, what, shdn)	do { \
	int rc; \
	size_t size = (n) * sizeof( struct pollfd ); \
	/* FIXME: use pwrite? */ \
	rc = write( slap_daemon.sd_dpfd, (pfd), size ); \
	if ( rc != size ) { \
		Debug( LDAP_DEBUG_ANY, "daemon: " SLAP_EVENT_FNAME ": " \
			"%s fd=%d failed errno=%d\n", \
			(what), (s), errno ); \
		if ( (shdn) ) { \
			slapd_shutdown = 2; \
		} \
	} \
} while (0)

# define SLAP_DEVPOLL_SOCK_SET(s, mode) 	do { \
	Debug( LDAP_DEBUG_CONNS, "SLAP_SOCK_SET_%s(%d)=%d\n", \
		(mode) == POLLIN ? "READ" : "WRITE", (s), \
		( (SLAP_DEVPOLL_SOCK_EV((s)) & (mode)) != (mode) ) ); \
	if ( (SLAP_DEVPOLL_SOCK_EV((s)) & (mode)) != (mode) ) { \
		struct pollfd pfd; \
		SLAP_DEVPOLL_SOCK_EV((s)) |= (mode); \
		pfd.fd = SLAP_DEVPOLL_SOCK_FD((s)); \
		pfd.events = /* (mode) */ SLAP_DEVPOLL_SOCK_EV((s)); \
		SLAP_DEVPOLL_WRITE_POLLFD((s), &pfd, 1, "SET", 0); \
	} \
} while (0)

# define SLAP_DEVPOLL_SOCK_CLR(s, mode)		do { \
	Debug( LDAP_DEBUG_CONNS, "SLAP_SOCK_CLR_%s(%d)=%d\n", \
		(mode) == POLLIN ? "READ" : "WRITE", (s), \
		( (SLAP_DEVPOLL_SOCK_EV((s)) & (mode)) == (mode) ) ); \
	if ((SLAP_DEVPOLL_SOCK_EV((s)) & (mode)) == (mode) ) { \
		struct pollfd pfd[2]; \
		SLAP_DEVPOLL_SOCK_EV((s)) &= ~(mode); \
		pfd[0].fd = SLAP_DEVPOLL_SOCK_FD((s)); \
		pfd[0].events = POLLREMOVE; \
		pfd[1] = SLAP_DEVPOLL_SOCK_EP((s)); \
		SLAP_DEVPOLL_WRITE_POLLFD((s), &pfd[0], 2, "CLR", 0); \
	} \
} while (0)

# define SLAP_SOCK_SET_READ(s)		SLAP_DEVPOLL_SOCK_SET(s, POLLIN)
# define SLAP_SOCK_SET_WRITE(s)		SLAP_DEVPOLL_SOCK_SET(s, POLLOUT)

# define SLAP_SOCK_CLR_READ(s)		SLAP_DEVPOLL_SOCK_CLR((s), POLLIN)
# define SLAP_SOCK_CLR_WRITE(s)		SLAP_DEVPOLL_SOCK_CLR((s), POLLOUT)

#  define SLAP_SOCK_SET_SUSPEND(s) \
	( slap_daemon.sd_suspend[SLAP_DEVPOLL_SOCK_IX((s))] = 1 )
#  define SLAP_SOCK_CLR_SUSPEND(s) \
	( slap_daemon.sd_suspend[SLAP_DEVPOLL_SOCK_IX((s))] = 0 )
#  define SLAP_SOCK_IS_SUSPEND(s) \
	( slap_daemon.sd_suspend[SLAP_DEVPOLL_SOCK_IX((s))] == 1 )

# define SLAP_DEVPOLL_EVENT_CLR(i, mode)	(revents[(i)].events &= ~(mode))

# define SLAP_EVENT_MAX			slap_daemon.sd_nfds

/* If a Listener address is provided, store that in the sd_l array.
 * If we can't do this add, the system is out of resources and we 
 * need to shutdown.
 */
# define SLAP_SOCK_ADD(s, l)		do { \
	Debug( LDAP_DEBUG_CONNS, "SLAP_SOCK_ADD(%d, %p)\n", (s), (l), 0 ); \
	SLAP_DEVPOLL_SOCK_IX((s)) = slap_daemon.sd_nfds; \
	SLAP_DEVPOLL_SOCK_LX((s)) = (l); \
	SLAP_DEVPOLL_SOCK_FD((s)) = (s); \
	SLAP_DEVPOLL_SOCK_EV((s)) = POLLIN; \
	SLAP_DEVPOLL_WRITE_POLLFD((s), &SLAP_DEVPOLL_SOCK_EP((s)), 1, "ADD", 1); \
	slap_daemon.sd_nfds++; \
} while (0)

# define SLAP_DEVPOLL_EV_LISTENER(ptr)	((ptr) != NULL)

# define SLAP_SOCK_DEL(s)		do { \
	int fd, index = SLAP_DEVPOLL_SOCK_IX((s)); \
	Debug( LDAP_DEBUG_CONNS, "SLAP_SOCK_DEL(%d)\n", (s), 0, 0 ); \
	if ( index < 0 ) break; \
	if ( index < slap_daemon.sd_nfds - 1 ) { \
		struct pollfd pfd = slap_daemon.sd_pollfd[index]; \
		fd = slap_daemon.sd_pollfd[slap_daemon.sd_nfds - 1].fd; \
		slap_daemon.sd_pollfd[index] = slap_daemon.sd_pollfd[slap_daemon.sd_nfds - 1]; \
		slap_daemon.sd_pollfd[slap_daemon.sd_nfds - 1] = pfd; \
		slap_daemon.sd_index[fd] = index; \
	} \
	slap_daemon.sd_index[(s)] = -1; \
	slap_daemon.sd_pollfd[slap_daemon.sd_nfds - 1].events = POLLREMOVE; \
	SLAP_DEVPOLL_WRITE_POLLFD((s), &slap_daemon.sd_pollfd[slap_daemon.sd_nfds - 1], 1, "DEL", 0); \
	slap_daemon.sd_pollfd[slap_daemon.sd_nfds - 1].events = 0; \
	slap_daemon.sd_nfds--; \
} while (0)

# define SLAP_EVENT_CLR_READ(i)		SLAP_DEVPOLL_EVENT_CLR((i), POLLIN)
# define SLAP_EVENT_CLR_WRITE(i)	SLAP_DEVPOLL_EVENT_CLR((i), POLLOUT)

# define SLAP_DEVPOLL_EVENT_CHK(i, mode)	(revents[(i)].events & (mode))

# define SLAP_EVENT_FD(i)		(revents[(i)].fd)

# define SLAP_EVENT_IS_READ(i)		SLAP_DEVPOLL_EVENT_CHK((i), POLLIN)
# define SLAP_EVENT_IS_WRITE(i)		SLAP_DEVPOLL_EVENT_CHK((i), POLLOUT)
# define SLAP_EVENT_IS_LISTENER(i)	SLAP_DEVPOLL_EV_LISTENER(SLAP_DEVPOLL_SOCK_LX(SLAP_EVENT_FD((i))))
# define SLAP_EVENT_LISTENER(i)		SLAP_DEVPOLL_SOCK_LX(SLAP_EVENT_FD((i)))

# define SLAP_SOCK_INIT		do { \
	slap_daemon.sd_pollfd = ch_calloc( 1, \
		( sizeof(struct pollfd) * 2 \
			+ sizeof( int ) \
			+ sizeof( Listener * ) ) * dtblsize ); \
	slap_daemon.sd_index = (int *)&slap_daemon.sd_pollfd[ 2 * dtblsize ]; \
	slap_daemon.sd_l = (Listener **)&slap_daemon.sd_index[ dtblsize ]; \
	slap_daemon.sd_dpfd = open( SLAP_EVENT_FNAME, O_RDWR ); \
	if ( slap_daemon.sd_dpfd == -1 ) { \
		Debug( LDAP_DEBUG_ANY, "daemon: " SLAP_EVENT_FNAME ": " \
			"open(\"" SLAP_EVENT_FNAME "\") failed errno=%d\n", \
			errno, 0, 0 ); \
		SLAP_SOCK_DESTROY; \
		return -1; \
	} \
	for ( i = 0; i < dtblsize; i++ ) { \
		slap_daemon.sd_pollfd[i].fd = -1; \
		slap_daemon.sd_index[i] = -1; \
	} \
} while (0)

# define SLAP_SOCK_DESTROY		do { \
	if ( slap_daemon.sd_pollfd != NULL ) { \
		ch_free( slap_daemon.sd_pollfd ); \
		slap_daemon.sd_pollfd = NULL; \
		slap_daemon.sd_index = NULL; \
		slap_daemon.sd_l = NULL; \
		close( slap_daemon.sd_dpfd ); \
	} \
} while ( 0 )

# define SLAP_EVENT_DECL		struct pollfd *revents

# define SLAP_EVENT_INIT		do { \
	revents = &slap_daemon.sd_pollfd[ dtblsize ]; \
} while (0)

# define SLAP_EVENT_WAIT(tvp, nsp)	do { \
	struct dvpoll		sd_dvpoll; \
	sd_dvpoll.dp_timeout = (tvp) ? (tvp)->tv_sec * 1000 : -1; \
	sd_dvpoll.dp_nfds = dtblsize; \
	sd_dvpoll.dp_fds = revents; \
	*(nsp) = ioctl( slap_daemon.sd_dpfd, DP_POLL, &sd_dvpoll ); \
} while (0)

#else /* ! epoll && ! /dev/poll */
# ifdef HAVE_WINSOCK
# define SLAP_EVENT_FNAME		"WSselect"
/* Winsock provides a "select" function but its fd_sets are
 * actually arrays of sockets. Since these sockets are handles
 * and not a contiguous range of small integers, we manage our
 * own "fd" table of socket handles and use their indices as
 * descriptors.
 *
 * All of our listener/connection structures use fds; the actual
 * I/O functions use sockets. The SLAP_FD2SOCK macro in proto-slap.h
 * handles the mapping.
 *
 * Despite the mapping overhead, this is about 45% more efficient
 * than just using Winsock's select and FD_ISSET directly.
 *
 * Unfortunately Winsock's select implementation doesn't scale well
 * as the number of connections increases. This probably needs to be
 * rewritten to use the Winsock overlapped/asynchronous I/O functions.
 */
# define SLAP_EVENTS_ARE_INDEXED	1
# define SLAP_EVENT_DECL		fd_set readfds, writefds
# define SLAP_EVENT_INIT	do { \
	int i; \
	FD_ZERO( &readfds ); \
	FD_ZERO( &writefds ); \
	memset( slap_daemon.sd_rflags, 0, slap_daemon.sd_nfds ); \
	for ( i=0; i<slap_daemon.sd_nfds; i++ ) { \
		if ( slap_daemon.sd_flags[i] & SD_READ ) \
			FD_SET( slapd_ws_sockets[i], &readfds );\
		if ( slap_daemon.sd_flags[i] & SD_WRITE ) \
			FD_SET( slapd_ws_sockets[i], &writefds ); \
	} } while ( 0 )

# define SLAP_EVENT_MAX		slap_daemon.sd_nfds

# define SLAP_EVENT_WAIT(tvp, nsp)	do { \
	int i; \
	*(nsp) = select( SLAP_EVENT_MAX, &readfds, \
		nwriters > 0 ? &writefds : NULL, NULL, (tvp) ); \
	for ( i=0; i<readfds.fd_count; i++) { \
		int fd = slapd_sock2fd(readfds.fd_array[i]); \
		if ( fd >= 0 ) { \
			slap_daemon.sd_rflags[fd] = SD_READ; \
			if ( fd >= *(nsp)) *(nsp) = fd+1; \
		} \
	} \
	for ( i=0; i<writefds.fd_count; i++) { \
		int fd = slapd_sock2fd(writefds.fd_array[i]); \
		if ( fd >= 0 ) { \
			slap_daemon.sd_rflags[fd] = SD_WRITE; \
			if ( fd >= *(nsp)) *(nsp) = fd+1; \
		} \
	} \
} while (0)

# define SLAP_EVENT_IS_READ(fd)		(slap_daemon.sd_rflags[fd] & SD_READ)
# define SLAP_EVENT_IS_WRITE(fd)	(slap_daemon.sd_rflags[fd] & SD_WRITE)

# define SLAP_EVENT_CLR_READ(fd) 	slap_daemon.sd_rflags[fd] &= ~SD_READ;
# define SLAP_EVENT_CLR_WRITE(fd)	slap_daemon.sd_rflags[fd] &= ~SD_WRITE;

# define SLAP_SOCK_INIT		do { \
	ldap_pvt_thread_mutex_init( &slapd_ws_mutex ); \
	slapd_ws_sockets = ch_malloc( dtblsize * ( sizeof(SOCKET) + 2)); \
	slap_daemon.sd_flags = (char *)(slapd_ws_sockets + dtblsize); \
	slap_daemon.sd_rflags = slap_daemon.sd_flags + dtblsize; \
	memset( slapd_ws_sockets, -1, dtblsize * sizeof(SOCKET) ); \
	slapd_ws_sockets[0] = wake_sds[0]; \
	slapd_ws_sockets[1] = wake_sds[1]; \
	wake_sds[0] = 0; \
	wake_sds[1] = 1; \
	slap_daemon.sd_nfds = 2; \
	} while ( 0 )

# define SLAP_SOCK_DESTROY	do { \
	ch_free( slapd_ws_sockets ); slapd_ws_sockets = NULL; \
	slap_daemon.sd_flags = NULL; \
	slap_daemon.sd_rflags = NULL; \
	ldap_pvt_thread_mutex_destroy( &slapd_ws_mutex ); \
	} while ( 0 )

# define SLAP_SOCK_IS_ACTIVE(fd) ( slap_daemon.sd_flags[fd] & SD_ACTIVE )
# define SLAP_SOCK_IS_READ(fd) ( slap_daemon.sd_flags[fd] & SD_READ )
# define SLAP_SOCK_IS_WRITE(fd) ( slap_daemon.sd_flags[fd] & SD_WRITE )
# define SLAP_SOCK_NOT_ACTIVE(fd)	(!slap_daemon.sd_flags[fd])

# define SLAP_SOCK_SET_READ(fd)		( slap_daemon.sd_flags[fd] |= SD_READ )
# define SLAP_SOCK_SET_WRITE(fd)		( slap_daemon.sd_flags[fd] |= SD_WRITE )

# define SLAP_SELECT_ADDTEST(s)	do { \
	if ((s) >= slap_daemon.sd_nfds) slap_daemon.sd_nfds = (s)+1; \
} while (0)

# define SLAP_SOCK_CLR_READ(fd)		( slap_daemon.sd_flags[fd] &= ~SD_READ )
# define SLAP_SOCK_CLR_WRITE(fd)		( slap_daemon.sd_flags[fd] &= ~SD_WRITE )

# define SLAP_SOCK_ADD(s, l)	do { \
	SLAP_SELECT_ADDTEST((s)); \
	slap_daemon.sd_flags[s] = SD_ACTIVE|SD_READ; \
} while ( 0 )

# define SLAP_SOCK_DEL(s) do { \
	slap_daemon.sd_flags[s] = 0; \
	slapd_sockdel( s ); \
} while ( 0 )

# else /* !HAVE_WINSOCK */

/**************************************
 * Use select system call - select(2) *
 **************************************/
# define SLAP_EVENT_FNAME		"select"
/* select */
# define SLAP_EVENTS_ARE_INDEXED	1
# define SLAP_EVENT_DECL		fd_set readfds, writefds

# define SLAP_EVENT_INIT		do { \
	AC_MEMCPY( &readfds, &slap_daemon.sd_readers, sizeof(fd_set) );	\
	if ( nwriters )	{ \
		AC_MEMCPY( &writefds, &slap_daemon.sd_writers, sizeof(fd_set) ); \
	} else { \
		FD_ZERO( &writefds ); \
	} \
} while (0)

# ifdef FD_SETSIZE
#  define SLAP_SELECT_CHK_SETSIZE	do { \
	if (dtblsize > FD_SETSIZE) dtblsize = FD_SETSIZE; \
} while (0)
# else /* ! FD_SETSIZE */
#  define SLAP_SELECT_CHK_SETSIZE	do { ; } while (0)
# endif /* ! FD_SETSIZE */

# define SLAP_SOCK_INIT			do { \
	SLAP_SELECT_CHK_SETSIZE; \
	FD_ZERO(&slap_daemon.sd_actives); \
	FD_ZERO(&slap_daemon.sd_readers); \
	FD_ZERO(&slap_daemon.sd_writers); \
} while (0)

# define SLAP_SOCK_DESTROY

# define SLAP_SOCK_IS_ACTIVE(fd)	FD_ISSET((fd), &slap_daemon.sd_actives)
# define SLAP_SOCK_IS_READ(fd)		FD_ISSET((fd), &slap_daemon.sd_readers)
# define SLAP_SOCK_IS_WRITE(fd)		FD_ISSET((fd), &slap_daemon.sd_writers)

# define SLAP_SOCK_NOT_ACTIVE(fd)	(!SLAP_SOCK_IS_ACTIVE(fd) && \
	 !SLAP_SOCK_IS_READ(fd) && !SLAP_SOCK_IS_WRITE(fd))

# define SLAP_SOCK_SET_READ(fd)	FD_SET((fd), &slap_daemon.sd_readers)
# define SLAP_SOCK_SET_WRITE(fd)	FD_SET((fd), &slap_daemon.sd_writers)

# define SLAP_EVENT_MAX		slap_daemon.sd_nfds
# define SLAP_SELECT_ADDTEST(s)	do { \
	if ((s) >= slap_daemon.sd_nfds) slap_daemon.sd_nfds = (s)+1; \
} while (0)

# define SLAP_SOCK_CLR_READ(fd)		FD_CLR((fd), &slap_daemon.sd_readers)
# define SLAP_SOCK_CLR_WRITE(fd)	FD_CLR((fd), &slap_daemon.sd_writers)

# define SLAP_SOCK_ADD(s, l)		do { \
	SLAP_SELECT_ADDTEST((s)); \
	FD_SET((s), &slap_daemon.sd_actives); \
	FD_SET((s), &slap_daemon.sd_readers); \
} while (0)

# define SLAP_SOCK_DEL(s)		do { \
	FD_CLR((s), &slap_daemon.sd_actives); \
	FD_CLR((s), &slap_daemon.sd_readers); \
	FD_CLR((s), &slap_daemon.sd_writers); \
} while (0)

# define SLAP_EVENT_IS_READ(fd)		FD_ISSET((fd), &readfds)
# define SLAP_EVENT_IS_WRITE(fd)	FD_ISSET((fd), &writefds)

# define SLAP_EVENT_CLR_READ(fd) 	FD_CLR((fd), &readfds)
# define SLAP_EVENT_CLR_WRITE(fd)	FD_CLR((fd), &writefds)

# define SLAP_EVENT_WAIT(tvp, nsp)	do { \
	*(nsp) = select( SLAP_EVENT_MAX, &readfds, \
		nwriters > 0 ? &writefds : NULL, NULL, (tvp) ); \
} while (0)
# endif /* !HAVE_WINSOCK */
#endif /* ! epoll && ! /dev/poll */

#ifdef HAVE_SLP
/*
 * SLP related functions
 */
#include <slp.h>

#define LDAP_SRVTYPE_PREFIX "service:ldap://"
#define LDAPS_SRVTYPE_PREFIX "service:ldaps://"
static char** slapd_srvurls = NULL;
static SLPHandle slapd_hslp = 0;
int slapd_register_slp = 0;
const char *slapd_slp_attrs = NULL;

static SLPError slapd_slp_cookie;

static void
slapd_slp_init( const char* urls )
{
	int i;
	SLPError err;

	slapd_srvurls = ldap_str2charray( urls, " " );

	if ( slapd_srvurls == NULL ) return;

	/* find and expand INADDR_ANY URLs */
	for ( i = 0; slapd_srvurls[i] != NULL; i++ ) {
		if ( strcmp( slapd_srvurls[i], "ldap:///" ) == 0 ) {
			slapd_srvurls[i] = (char *) ch_realloc( slapd_srvurls[i],
				strlen( global_host ) +
				sizeof( LDAP_SRVTYPE_PREFIX ) );
			strcpy( lutil_strcopy(slapd_srvurls[i],
				LDAP_SRVTYPE_PREFIX ), global_host );
		} else if ( strcmp( slapd_srvurls[i], "ldaps:///" ) == 0 ) {
			slapd_srvurls[i] = (char *) ch_realloc( slapd_srvurls[i],
				strlen( global_host ) +
				sizeof( LDAPS_SRVTYPE_PREFIX ) );
			strcpy( lutil_strcopy(slapd_srvurls[i],
				LDAPS_SRVTYPE_PREFIX ), global_host );
		}
	}

	/* open the SLP handle */
	err = SLPOpen( "en", 0, &slapd_hslp );

	if ( err != SLP_OK ) {
		Debug( LDAP_DEBUG_CONNS, "daemon: SLPOpen() failed with %ld\n",
			(long)err, 0, 0 );
	}
}

static void
slapd_slp_deinit( void )
{
	if ( slapd_srvurls == NULL ) return;

	ldap_charray_free( slapd_srvurls );
	slapd_srvurls = NULL;

	/* close the SLP handle */
	SLPClose( slapd_hslp );
}

static void
slapd_slp_regreport(
	SLPHandle	hslp,
	SLPError	errcode,
	void		*cookie )
{
	/* return the error code in the cookie */
	*(SLPError*)cookie = errcode; 
}

static void
slapd_slp_reg()
{
	int i;
	SLPError err;

	if ( slapd_srvurls == NULL ) return;

	for ( i = 0; slapd_srvurls[i] != NULL; i++ ) {
		if ( strncmp( slapd_srvurls[i], LDAP_SRVTYPE_PREFIX,
				sizeof( LDAP_SRVTYPE_PREFIX ) - 1 ) == 0 ||
			strncmp( slapd_srvurls[i], LDAPS_SRVTYPE_PREFIX,
				sizeof( LDAPS_SRVTYPE_PREFIX ) - 1 ) == 0 )
		{
			err = SLPReg( slapd_hslp,
				slapd_srvurls[i],
				SLP_LIFETIME_MAXIMUM,
				"ldap",
				(slapd_slp_attrs) ? slapd_slp_attrs : "",
				SLP_TRUE,
				slapd_slp_regreport,
				&slapd_slp_cookie );

			if ( err != SLP_OK || slapd_slp_cookie != SLP_OK ) {
				Debug( LDAP_DEBUG_CONNS,
					"daemon: SLPReg(%s) failed with %ld, cookie = %ld\n",
					slapd_srvurls[i], (long)err, (long)slapd_slp_cookie );
			}	
		}
	}
}

static void
slapd_slp_dereg( void )
{
	int i;
	SLPError err;

	if ( slapd_srvurls == NULL ) return;

	for ( i = 0; slapd_srvurls[i] != NULL; i++ ) {
		err = SLPDereg( slapd_hslp,
			slapd_srvurls[i],
			slapd_slp_regreport,
			&slapd_slp_cookie );
		
		if ( err != SLP_OK || slapd_slp_cookie != SLP_OK ) {
			Debug( LDAP_DEBUG_CONNS,
				"daemon: SLPDereg(%s) failed with %ld, cookie = %ld\n",
				slapd_srvurls[i], (long)err, (long)slapd_slp_cookie );
		}
	}
}
#endif /* HAVE_SLP */

#ifdef HAVE_WINSOCK
/* Manage the descriptor to socket table */
ber_socket_t
slapd_socknew( ber_socket_t s )
{
	ber_socket_t i;
	ldap_pvt_thread_mutex_lock( &slapd_ws_mutex );
	for ( i = 0; i < dtblsize && slapd_ws_sockets[i] != INVALID_SOCKET; i++ );
	if ( i == dtblsize ) {
		WSASetLastError( WSAEMFILE );
	} else {
		slapd_ws_sockets[i] = s;
	}
	ldap_pvt_thread_mutex_unlock( &slapd_ws_mutex );
	return i;
}

void
slapd_sockdel( ber_socket_t s )
{
	ldap_pvt_thread_mutex_lock( &slapd_ws_mutex );
	slapd_ws_sockets[s] = INVALID_SOCKET;
	ldap_pvt_thread_mutex_unlock( &slapd_ws_mutex );
}

ber_socket_t
slapd_sock2fd( ber_socket_t s )
{
	ber_socket_t i;
	for ( i=0; i<dtblsize && slapd_ws_sockets[i] != s; i++);
	if ( i == dtblsize )
		i = -1;
	return i;
}
#endif

/*
 * Add a descriptor to daemon control
 *
 * If isactive, the descriptor is a live server session and is subject
 * to idletimeout control. Otherwise, the descriptor is a passive
 * listener or an outbound client session, and not subject to
 * idletimeout. The underlying event handler may record the Listener
 * argument to differentiate Listener's from real sessions.
 */
static void
slapd_add( ber_socket_t s, int isactive, Listener *sl )
{
	ldap_pvt_thread_mutex_lock( &slap_daemon.sd_mutex );

	assert( SLAP_SOCK_NOT_ACTIVE(s) );

	if ( isactive ) slap_daemon.sd_nactives++;

	SLAP_SOCK_ADD(s, sl);

	Debug( LDAP_DEBUG_CONNS, "daemon: added %ldr%s listener=%p\n",
		(long) s, isactive ? " (active)" : "", (void *)sl );

	ldap_pvt_thread_mutex_unlock( &slap_daemon.sd_mutex );

	WAKE_LISTENER(1);
}

/*
 * NOTE: unused
 */
void
slapd_sd_lock( void )
{
	ldap_pvt_thread_mutex_lock( &slap_daemon.sd_mutex );
}

/*
 * NOTE: unused
 */
void
slapd_sd_unlock( void )
{
	ldap_pvt_thread_mutex_unlock( &slap_daemon.sd_mutex );
}

/*
 * Remove the descriptor from daemon control
 */
void
slapd_remove(
	ber_socket_t s,
	Sockbuf *sb,
	int wasactive,
	int wake,
	int locked )
{
	int waswriter;
	int wasreader;

	if ( !locked )
		ldap_pvt_thread_mutex_lock( &slap_daemon.sd_mutex );

	assert( SLAP_SOCK_IS_ACTIVE( s ));

	if ( wasactive ) slap_daemon.sd_nactives--;

	waswriter = SLAP_SOCK_IS_WRITE(s);
	wasreader = SLAP_SOCK_IS_READ(s);

	Debug( LDAP_DEBUG_CONNS, "daemon: removing %ld%s%s\n",
		(long) s,
		wasreader ? "r" : "",
		waswriter ? "w" : "" );

	if ( waswriter ) slap_daemon.sd_nwriters--;

	SLAP_SOCK_DEL(s);

	if ( sb )
		ber_sockbuf_free(sb);

	/* If we ran out of file descriptors, we dropped a listener from
	 * the select() loop. Now that we're removing a session from our
	 * control, we can try to resume a dropped listener to use.
	 */
	if ( emfile ) {
		int i;
		for ( i = 0; slap_listeners[i] != NULL; i++ ) {
			Listener *lr = slap_listeners[i];

			if ( lr->sl_sd == AC_SOCKET_INVALID ) continue;
			if ( lr->sl_sd == s ) continue;
			if ( lr->sl_mute ) {
				lr->sl_mute = 0;
				emfile--;
				break;
			}
		}
		/* Walked the entire list without enabling anything; emfile
		 * counter is stale. Reset it.
		 */
		if ( slap_listeners[i] == NULL ) emfile = 0;
	}
	ldap_pvt_thread_mutex_unlock( &slap_daemon.sd_mutex );
	WAKE_LISTENER(wake || slapd_gentle_shutdown == 2);
}

void
slapd_clr_write( ber_socket_t s, int wake )
{
	ldap_pvt_thread_mutex_lock( &slap_daemon.sd_mutex );

	if ( SLAP_SOCK_IS_WRITE( s )) {
		assert( SLAP_SOCK_IS_ACTIVE( s ));

		SLAP_SOCK_CLR_WRITE( s );
		slap_daemon.sd_nwriters--;
	}

	ldap_pvt_thread_mutex_unlock( &slap_daemon.sd_mutex );
	WAKE_LISTENER(wake);
}

void
slapd_set_write( ber_socket_t s, int wake )
{
	ldap_pvt_thread_mutex_lock( &slap_daemon.sd_mutex );

	assert( SLAP_SOCK_IS_ACTIVE( s ));

	if ( !SLAP_SOCK_IS_WRITE( s )) {
		SLAP_SOCK_SET_WRITE( s );
		slap_daemon.sd_nwriters++;
	}

	ldap_pvt_thread_mutex_unlock( &slap_daemon.sd_mutex );
	WAKE_LISTENER(wake);
}

int
slapd_clr_read( ber_socket_t s, int wake )
{
	int rc = 1;
	ldap_pvt_thread_mutex_lock( &slap_daemon.sd_mutex );

	if ( SLAP_SOCK_IS_ACTIVE( s )) {
		SLAP_SOCK_CLR_READ( s );
		rc = 0;
	}
	ldap_pvt_thread_mutex_unlock( &slap_daemon.sd_mutex );
	if ( !rc )
		WAKE_LISTENER(wake);
	return rc;
}

void
slapd_set_read( ber_socket_t s, int wake )
{
	ldap_pvt_thread_mutex_lock( &slap_daemon.sd_mutex );

	assert( SLAP_SOCK_IS_ACTIVE( s ));
	if (!SLAP_SOCK_IS_READ( s )) SLAP_SOCK_SET_READ( s );

	ldap_pvt_thread_mutex_unlock( &slap_daemon.sd_mutex );
	WAKE_LISTENER(wake);
}

static void
slapd_close( ber_socket_t s )
{
	Debug( LDAP_DEBUG_CONNS, "daemon: closing %ld\n",
		(long) s, 0, 0 );
	tcp_close( SLAP_FD2SOCK(s) );
#ifdef HAVE_WINSOCK
	slapd_sockdel( s );
#endif
}

static void
slap_free_listener_addresses( struct sockaddr **sal )
{
	struct sockaddr **sap;
	if (sal == NULL) return;
	for (sap = sal; *sap != NULL; sap++) ch_free(*sap);
	ch_free(sal);
}

#if defined(LDAP_PF_LOCAL) || defined(SLAP_X_LISTENER_MOD)
static int
get_url_perms(
	char 	**exts,
	mode_t	*perms,
	int	*crit )
{
	int	i;

	assert( exts != NULL );
	assert( perms != NULL );
	assert( crit != NULL );

	*crit = 0;
	for ( i = 0; exts[ i ]; i++ ) {
		char	*type = exts[ i ];
		int	c = 0;

		if ( type[ 0 ] == '!' ) {
			c = 1;
			type++;
		}

		if ( strncasecmp( type, LDAPI_MOD_URLEXT "=",
			sizeof(LDAPI_MOD_URLEXT "=") - 1 ) == 0 )
		{
			char *value = type + ( sizeof(LDAPI_MOD_URLEXT "=") - 1 );
			mode_t p = 0;
			int j;

			switch (strlen(value)) {
			case 4:
				/* skip leading '0' */
				if ( value[ 0 ] != '0' ) return LDAP_OTHER;
				value++;

			case 3:
				for ( j = 0; j < 3; j++) {
					int	v;

					v = value[ j ] - '0';

					if ( v < 0 || v > 7 ) return LDAP_OTHER;

					p |= v << 3*(2-j);
				}
				break;

			case 10:
				for ( j = 1; j < 10; j++ ) {
					static mode_t	m[] = { 0, 
						S_IRUSR, S_IWUSR, S_IXUSR,
						S_IRGRP, S_IWGRP, S_IXGRP,
						S_IROTH, S_IWOTH, S_IXOTH
					};
					static const char	c[] = "-rwxrwxrwx"; 

					if ( value[ j ] == c[ j ] ) {
						p |= m[ j ];
	
					} else if ( value[ j ] != '-' ) {
						return LDAP_OTHER;
					}
				}
				break;

			default:
				return LDAP_OTHER;
			} 

			*crit = c;
			*perms = p;

			return LDAP_SUCCESS;
		}
	}

	return LDAP_OTHER;
}
#endif /* LDAP_PF_LOCAL || SLAP_X_LISTENER_MOD */

/* port = 0 indicates AF_LOCAL */
static int
slap_get_listener_addresses(
	const char *host,
	unsigned short port,
	struct sockaddr ***sal )
{
	struct sockaddr **sap;

#ifdef LDAP_PF_LOCAL
	if ( port == 0 ) {
		*sal = ch_malloc(2 * sizeof(void *));
		if (*sal == NULL) return -1;

		sap = *sal;
		*sap = ch_malloc(sizeof(struct sockaddr_un));
		if (*sap == NULL) goto errexit;
		sap[1] = NULL;

		if ( strlen(host) >
			(sizeof(((struct sockaddr_un *)*sap)->sun_path) - 1) )
		{
			Debug( LDAP_DEBUG_ANY,
				"daemon: domain socket path (%s) too long in URL",
				host, 0, 0);
			goto errexit;
		}

		(void)memset( (void *)*sap, '\0', sizeof(struct sockaddr_un) );
		(*sap)->sa_family = AF_LOCAL;
		strcpy( ((struct sockaddr_un *)*sap)->sun_path, host );
	} else
#endif /* LDAP_PF_LOCAL */
	{
#ifdef HAVE_GETADDRINFO
		struct addrinfo hints, *res, *sai;
		int n, err;
		char serv[7];

		memset( &hints, '\0', sizeof(hints) );
		hints.ai_flags = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = slap_inet4or6;
		snprintf(serv, sizeof serv, "%d", port);

		if ( (err = getaddrinfo(host, serv, &hints, &res)) ) {
			Debug( LDAP_DEBUG_ANY, "daemon: getaddrinfo() failed: %s\n",
				AC_GAI_STRERROR(err), 0, 0);
			return -1;
		}

		sai = res;
		for (n=2; (sai = sai->ai_next) != NULL; n++) {
			/* EMPTY */ ;
		}
		*sal = ch_calloc(n, sizeof(void *));
		if (*sal == NULL) return -1;

		sap = *sal;
		*sap = NULL;

		for ( sai=res; sai; sai=sai->ai_next ) {
			if( sai->ai_addr == NULL ) {
				Debug( LDAP_DEBUG_ANY, "slap_get_listener_addresses: "
					"getaddrinfo ai_addr is NULL?\n", 0, 0, 0 );
				freeaddrinfo(res);
				goto errexit;
			}

			switch (sai->ai_family) {
#  ifdef LDAP_PF_INET6
			case AF_INET6:
				*sap = ch_malloc(sizeof(struct sockaddr_in6));
				if (*sap == NULL) {
					freeaddrinfo(res);
					goto errexit;
				}
				*(struct sockaddr_in6 *)*sap =
					*((struct sockaddr_in6 *)sai->ai_addr);
				break;
#  endif /* LDAP_PF_INET6 */
			case AF_INET:
				*sap = ch_malloc(sizeof(struct sockaddr_in));
				if (*sap == NULL) {
					freeaddrinfo(res);
					goto errexit;
				}
				*(struct sockaddr_in *)*sap =
					*((struct sockaddr_in *)sai->ai_addr);
				break;
			default:
				*sap = NULL;
				break;
			}

			if (*sap != NULL) {
				(*sap)->sa_family = sai->ai_family;
				sap++;
				*sap = NULL;
			}
		}

		freeaddrinfo(res);

#else /* ! HAVE_GETADDRINFO */
		int i, n = 1;
		struct in_addr in;
		struct hostent *he = NULL;

		if ( host == NULL ) {
			in.s_addr = htonl(INADDR_ANY);

		} else if ( !inet_aton( host, &in ) ) {
			he = gethostbyname( host );
			if( he == NULL ) {
				Debug( LDAP_DEBUG_ANY,
					"daemon: invalid host %s", host, 0, 0);
				return -1;
			}
			for (n = 0; he->h_addr_list[n]; n++) /* empty */;
		}

		*sal = ch_malloc((n+1) * sizeof(void *));
		if (*sal == NULL) return -1;

		sap = *sal;
		for ( i = 0; i<n; i++ ) {
			sap[i] = ch_malloc(sizeof(struct sockaddr_in));
			if (*sap == NULL) goto errexit;

			(void)memset( (void *)sap[i], '\0', sizeof(struct sockaddr_in) );
			sap[i]->sa_family = AF_INET;
			((struct sockaddr_in *)sap[i])->sin_port = htons(port);
			AC_MEMCPY( &((struct sockaddr_in *)sap[i])->sin_addr,
				he ? (struct in_addr *)he->h_addr_list[i] : &in,
				sizeof(struct in_addr) );
		}
		sap[i] = NULL;
#endif /* ! HAVE_GETADDRINFO */
	}

	return 0;

errexit:
	slap_free_listener_addresses(*sal);
	return -1;
}

static int
slap_open_listener(
	const char* url,
	int *listeners,
	int *cur )
{
	int	num, tmp, rc;
	Listener l;
	Listener *li;
	LDAPURLDesc *lud;
	unsigned short port;
	int err, addrlen = 0;
	struct sockaddr **sal, **psal;
	int socktype = SOCK_STREAM;	/* default to COTS */
	ber_socket_t s;

#if defined(LDAP_PF_LOCAL) || defined(SLAP_X_LISTENER_MOD)
	/*
	 * use safe defaults
	 */
	int	crit = 1;
#endif /* LDAP_PF_LOCAL || SLAP_X_LISTENER_MOD */

	rc = ldap_url_parse( url, &lud );

	if( rc != LDAP_URL_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"daemon: listen URL \"%s\" parse error=%d\n",
			url, rc, 0 );
		return rc;
	}

	l.sl_url.bv_val = NULL;
	l.sl_mute = 0;
	l.sl_busy = 0;

#ifndef HAVE_TLS
	if( ldap_pvt_url_scheme2tls( lud->lud_scheme ) ) {
		Debug( LDAP_DEBUG_ANY, "daemon: TLS not supported (%s)\n",
			url, 0, 0 );
		ldap_free_urldesc( lud );
		return -1;
	}

	if(! lud->lud_port ) lud->lud_port = LDAP_PORT;

#else /* HAVE_TLS */
	l.sl_is_tls = ldap_pvt_url_scheme2tls( lud->lud_scheme );

	if(! lud->lud_port ) {
		lud->lud_port = l.sl_is_tls ? LDAPS_PORT : LDAP_PORT;
	}
#endif /* HAVE_TLS */

	port = (unsigned short) lud->lud_port;

	tmp = ldap_pvt_url_scheme2proto(lud->lud_scheme);
	if ( tmp == LDAP_PROTO_IPC ) {
#ifdef LDAP_PF_LOCAL
		if ( lud->lud_host == NULL || lud->lud_host[0] == '\0' ) {
			err = slap_get_listener_addresses(LDAPI_SOCK, 0, &sal);
		} else {
			err = slap_get_listener_addresses(lud->lud_host, 0, &sal);
		}
#else /* ! LDAP_PF_LOCAL */

		Debug( LDAP_DEBUG_ANY, "daemon: URL scheme not supported: %s",
			url, 0, 0);
		ldap_free_urldesc( lud );
		return -1;
#endif /* ! LDAP_PF_LOCAL */
	} else {
		if( lud->lud_host == NULL || lud->lud_host[0] == '\0'
			|| strcmp(lud->lud_host, "*") == 0 )
		{
			err = slap_get_listener_addresses(NULL, port, &sal);
		} else {
			err = slap_get_listener_addresses(lud->lud_host, port, &sal);
		}
	}

#ifdef LDAP_CONNECTIONLESS
	l.sl_is_udp = ( tmp == LDAP_PROTO_UDP );
#endif /* LDAP_CONNECTIONLESS */

#if defined(LDAP_PF_LOCAL) || defined(SLAP_X_LISTENER_MOD)
	if ( lud->lud_exts ) {
		err = get_url_perms( lud->lud_exts, &l.sl_perms, &crit );
	} else {
		l.sl_perms = S_IRWXU | S_IRWXO;
	}
#endif /* LDAP_PF_LOCAL || SLAP_X_LISTENER_MOD */

	ldap_free_urldesc( lud );
	if ( err ) return -1;

	/* If we got more than one address returned, we need to make space
	 * for it in the slap_listeners array.
	 */
	for ( num=0; sal[num]; num++ ) /* empty */;
	if ( num > 1 ) {
		*listeners += num-1;
		slap_listeners = ch_realloc( slap_listeners,
			(*listeners + 1) * sizeof(Listener *) );
	}

	psal = sal;
	while ( *sal != NULL ) {
		char *af;
		switch( (*sal)->sa_family ) {
		case AF_INET:
			af = "IPv4";
			break;
#ifdef LDAP_PF_INET6
		case AF_INET6:
			af = "IPv6";
			break;
#endif /* LDAP_PF_INET6 */
#ifdef LDAP_PF_LOCAL
		case AF_LOCAL:
			af = "Local";
			break;
#endif /* LDAP_PF_LOCAL */
		default:
			sal++;
			continue;
		}

#ifdef LDAP_CONNECTIONLESS
		if( l.sl_is_udp ) socktype = SOCK_DGRAM;
#endif /* LDAP_CONNECTIONLESS */

		s = socket( (*sal)->sa_family, socktype, 0);
		if ( s == AC_SOCKET_INVALID ) {
			int err = sock_errno();
			Debug( LDAP_DEBUG_ANY,
				"daemon: %s socket() failed errno=%d (%s)\n",
				af, err, sock_errstr(err) );
			sal++;
			continue;
		}
		l.sl_sd = SLAP_SOCKNEW( s );

		if ( l.sl_sd >= dtblsize ) {
			Debug( LDAP_DEBUG_ANY,
				"daemon: listener descriptor %ld is too great %ld\n",
				(long) l.sl_sd, (long) dtblsize, 0 );
			tcp_close( s );
			sal++;
			continue;
		}

#ifdef LDAP_PF_LOCAL
		if ( (*sal)->sa_family == AF_LOCAL ) {
			unlink( ((struct sockaddr_un *)*sal)->sun_path );
		} else
#endif /* LDAP_PF_LOCAL */
		{
#ifdef SO_REUSEADDR
			/* enable address reuse */
			tmp = 1;
			rc = setsockopt( s, SOL_SOCKET, SO_REUSEADDR,
				(char *) &tmp, sizeof(tmp) );
			if ( rc == AC_SOCKET_ERROR ) {
				int err = sock_errno();
				Debug( LDAP_DEBUG_ANY, "slapd(%ld): "
					"setsockopt(SO_REUSEADDR) failed errno=%d (%s)\n",
					(long) l.sl_sd, err, sock_errstr(err) );
			}
#endif /* SO_REUSEADDR */
		}

		switch( (*sal)->sa_family ) {
		case AF_INET:
			addrlen = sizeof(struct sockaddr_in);
			break;
#ifdef LDAP_PF_INET6
		case AF_INET6:
#ifdef IPV6_V6ONLY
			/* Try to use IPv6 sockets for IPv6 only */
			tmp = 1;
			rc = setsockopt( s , IPPROTO_IPV6, IPV6_V6ONLY,
				(char *) &tmp, sizeof(tmp) );
			if ( rc == AC_SOCKET_ERROR ) {
				int err = sock_errno();
				Debug( LDAP_DEBUG_ANY, "slapd(%ld): "
					"setsockopt(IPV6_V6ONLY) failed errno=%d (%s)\n",
					(long) l.sl_sd, err, sock_errstr(err) );
			}
#endif /* IPV6_V6ONLY */
			addrlen = sizeof(struct sockaddr_in6);
			break;
#endif /* LDAP_PF_INET6 */

#ifdef LDAP_PF_LOCAL
		case AF_LOCAL:
#ifdef LOCAL_CREDS
			{
				int one = 1;
				setsockopt( s, 0, LOCAL_CREDS, &one, sizeof( one ) );
			}
#endif /* LOCAL_CREDS */

			addrlen = sizeof( struct sockaddr_un );
			break;
#endif /* LDAP_PF_LOCAL */
		}

#ifdef LDAP_PF_LOCAL
		/* create socket with all permissions set for those systems
		 * that honor permissions on sockets (e.g. Linux); typically,
		 * only write is required.  To exploit filesystem permissions,
		 * place the socket in a directory and use directory's
		 * permissions.  Need write perms to the directory to 
		 * create/unlink the socket; likely need exec perms to access
		 * the socket (ITS#4709) */
		{
			mode_t old_umask;

			if ( (*sal)->sa_family == AF_LOCAL ) {
				old_umask = umask( 0 );
			}
#endif /* LDAP_PF_LOCAL */
			rc = bind( s, *sal, addrlen );
#ifdef LDAP_PF_LOCAL
			if ( (*sal)->sa_family == AF_LOCAL ) {
				umask( old_umask );
			}
		}
#endif /* LDAP_PF_LOCAL */
		if ( rc ) {
			err = sock_errno();
			Debug( LDAP_DEBUG_ANY,
				"daemon: bind(%ld) failed errno=%d (%s)\n",
				(long)l.sl_sd, err, sock_errstr( err ) );
			tcp_close( s );
			sal++;
			continue;
		}

		switch ( (*sal)->sa_family ) {
#ifdef LDAP_PF_LOCAL
		case AF_LOCAL: {
			char *addr = ((struct sockaddr_un *)*sal)->sun_path;
			l.sl_name.bv_len = strlen(addr) + sizeof("PATH=") - 1;
			l.sl_name.bv_val = ber_memalloc( l.sl_name.bv_len + 1 );
			snprintf( l.sl_name.bv_val, l.sl_name.bv_len + 1, 
				"PATH=%s", addr );
		} break;
#endif /* LDAP_PF_LOCAL */

		case AF_INET: {
			char *s;
#if defined( HAVE_GETADDRINFO ) && defined( HAVE_INET_NTOP )
			char addr[INET_ADDRSTRLEN];
			inet_ntop( AF_INET, &((struct sockaddr_in *)*sal)->sin_addr,
				addr, sizeof(addr) );
			s = addr;
#else /* ! HAVE_GETADDRINFO || ! HAVE_INET_NTOP */
			s = inet_ntoa( ((struct sockaddr_in *) *sal)->sin_addr );
#endif /* ! HAVE_GETADDRINFO || ! HAVE_INET_NTOP */
			port = ntohs( ((struct sockaddr_in *)*sal) ->sin_port );
			l.sl_name.bv_val =
				ber_memalloc( sizeof("IP=255.255.255.255:65535") );
			snprintf( l.sl_name.bv_val, sizeof("IP=255.255.255.255:65535"),
				"IP=%s:%d",
				 s != NULL ? s : SLAP_STRING_UNKNOWN, port );
			l.sl_name.bv_len = strlen( l.sl_name.bv_val );
		} break;

#ifdef LDAP_PF_INET6
		case AF_INET6: {
			char addr[INET6_ADDRSTRLEN];
			inet_ntop( AF_INET6, &((struct sockaddr_in6 *)*sal)->sin6_addr,
				addr, sizeof addr);
			port = ntohs( ((struct sockaddr_in6 *)*sal)->sin6_port );
			l.sl_name.bv_len = strlen(addr) + sizeof("IP=[]:65535");
			l.sl_name.bv_val = ber_memalloc( l.sl_name.bv_len );
			snprintf( l.sl_name.bv_val, l.sl_name.bv_len, "IP=[%s]:%d", 
				addr, port );
			l.sl_name.bv_len = strlen( l.sl_name.bv_val );
		} break;
#endif /* LDAP_PF_INET6 */

		default:
			Debug( LDAP_DEBUG_ANY, "daemon: unsupported address family (%d)\n",
				(int) (*sal)->sa_family, 0, 0 );
			break;
		}

		AC_MEMCPY(&l.sl_sa, *sal, addrlen);
		ber_str2bv( url, 0, 1, &l.sl_url);
		li = ch_malloc( sizeof( Listener ) );
		*li = l;
		slap_listeners[*cur] = li;
		(*cur)++;
		sal++;
	}

	slap_free_listener_addresses(psal);

	if ( l.sl_url.bv_val == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			"slap_open_listener: failed on %s\n", url, 0, 0 );
		return -1;
	}

	Debug( LDAP_DEBUG_TRACE, "daemon: listener initialized %s\n",
		l.sl_url.bv_val, 0, 0 );
	return 0;
}

static int sockinit(void);
static int sockdestroy(void);

int
slapd_daemon_init( const char *urls )
{
	int i, j, n, rc;
	char **u;

	Debug( LDAP_DEBUG_ARGS, "daemon_init: %s\n",
		urls ? urls : "<null>", 0, 0 );

	ldap_pvt_thread_mutex_init( &slap_daemon.sd_mutex );
#ifdef HAVE_TCPD
	ldap_pvt_thread_mutex_init( &slap_daemon.sd_tcpd_mutex );
#endif /* TCP Wrappers */

	if( (rc = sockinit()) != 0 ) return rc;

#ifdef HAVE_SYSCONF
	dtblsize = sysconf( _SC_OPEN_MAX );
#elif defined(HAVE_GETDTABLESIZE)
	dtblsize = getdtablesize();
#else /* ! HAVE_SYSCONF && ! HAVE_GETDTABLESIZE */
	dtblsize = FD_SETSIZE;
#endif /* ! HAVE_SYSCONF && ! HAVE_GETDTABLESIZE */

	/* open a pipe (or something equivalent connected to itself).
	 * we write a byte on this fd whenever we catch a signal. The main
	 * loop will be select'ing on this socket, and will wake up when
	 * this byte arrives.
	 */
	if( (rc = lutil_pair( wake_sds )) < 0 ) {
		Debug( LDAP_DEBUG_ANY,
			"daemon: lutil_pair() failed rc=%d\n", rc, 0, 0 );
		return rc;
	}

	SLAP_SOCK_INIT;

	if( urls == NULL ) urls = "ldap:///";

	u = ldap_str2charray( urls, " " );

	if( u == NULL || u[0] == NULL ) {
		Debug( LDAP_DEBUG_ANY, "daemon_init: no urls (%s) provided.\n",
			urls, 0, 0 );
		if ( u )
			ldap_charray_free( u );
		return -1;
	}

	for( i=0; u[i] != NULL; i++ ) {
		Debug( LDAP_DEBUG_TRACE, "daemon_init: listen on %s\n",
			u[i], 0, 0 );
	}

	if( i == 0 ) {
		Debug( LDAP_DEBUG_ANY, "daemon_init: no listeners to open (%s)\n",
			urls, 0, 0 );
		ldap_charray_free( u );
		return -1;
	}

	Debug( LDAP_DEBUG_TRACE, "daemon_init: %d listeners to open...\n",
		i, 0, 0 );
	slap_listeners = ch_malloc( (i+1)*sizeof(Listener *) );

	for(n = 0, j = 0; u[n]; n++ ) {
		if ( slap_open_listener( u[n], &i, &j ) ) {
			ldap_charray_free( u );
			return -1;
		}
	}
	slap_listeners[j] = NULL;

	Debug( LDAP_DEBUG_TRACE, "daemon_init: %d listeners opened\n",
		i, 0, 0 );


#ifdef HAVE_SLP
	if( slapd_register_slp ) {
		slapd_slp_init( urls );
		slapd_slp_reg();
	}
#endif /* HAVE_SLP */

	ldap_charray_free( u );

	return !i;
}


int
slapd_daemon_destroy( void )
{
	connections_destroy();
	tcp_close( SLAP_FD2SOCK(wake_sds[1]) );
	tcp_close( SLAP_FD2SOCK(wake_sds[0]) );
	sockdestroy();

#ifdef HAVE_SLP
	if( slapd_register_slp ) {
		slapd_slp_dereg();
		slapd_slp_deinit();
	}
#endif /* HAVE_SLP */

#ifdef HAVE_TCPD
	ldap_pvt_thread_mutex_destroy( &slap_daemon.sd_tcpd_mutex );
#endif /* TCP Wrappers */

	ldap_pvt_thread_mutex_destroy( &slap_daemon.sd_mutex );
	return 0;
}


static void
close_listeners(
	int remove )
{
	int l;

	for ( l = 0; slap_listeners[l] != NULL; l++ ) {
		Listener *lr = slap_listeners[l];

		if ( lr->sl_sd != AC_SOCKET_INVALID ) {
			if ( remove ) slapd_remove( lr->sl_sd, NULL, 0, 0, 0 );

#ifdef LDAP_PF_LOCAL
			if ( lr->sl_sa.sa_addr.sa_family == AF_LOCAL ) {
				unlink( lr->sl_sa.sa_un_addr.sun_path );
			}
#endif /* LDAP_PF_LOCAL */

			slapd_close( lr->sl_sd );
		}

		if ( lr->sl_url.bv_val ) {
			ber_memfree( lr->sl_url.bv_val );
		}

		if ( lr->sl_name.bv_val ) {
			ber_memfree( lr->sl_name.bv_val );
		}

		free( lr );
		slap_listeners[l] = NULL;
	}
}

static int
slap_listener(
	Listener *sl )
{
	Sockaddr		from;

	ber_socket_t s, sfd;
	ber_socklen_t len = sizeof(from);
	Connection *c;
	slap_ssf_t ssf = 0;
	struct berval authid = BER_BVNULL;
#ifdef SLAPD_RLOOKUPS
	char hbuf[NI_MAXHOST];
#endif /* SLAPD_RLOOKUPS */

	char	*dnsname = NULL;
	char	*peeraddr = NULL;
#ifdef LDAP_PF_LOCAL
	char peername[MAXPATHLEN + sizeof("PATH=")];
#ifdef LDAP_PF_LOCAL_SENDMSG
	char peerbuf[8];
	struct berval peerbv = BER_BVNULL;
#endif
#elif defined(LDAP_PF_INET6)
	char peername[sizeof("IP=[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]:65535")];
#else /* ! LDAP_PF_LOCAL && ! LDAP_PF_INET6 */
	char peername[sizeof("IP=255.255.255.255:65336")];
#endif /* LDAP_PF_LOCAL */
	int cflag;

	Debug( LDAP_DEBUG_TRACE,
		">>> slap_listener(%s)\n",
		sl->sl_url.bv_val, 0, 0 );

	peername[0] = '\0';

#ifdef LDAP_CONNECTIONLESS
	if ( sl->sl_is_udp ) return 1;
#endif /* LDAP_CONNECTIONLESS */

#  ifdef LDAP_PF_LOCAL
	/* FIXME: apparently accept doesn't fill
	 * the sun_path sun_path member */
	from.sa_un_addr.sun_path[0] = '\0';
#  endif /* LDAP_PF_LOCAL */

	s = accept( SLAP_FD2SOCK( sl->sl_sd ), (struct sockaddr *) &from, &len );

	/* Resume the listener FD to allow concurrent-processing of
	 * additional incoming connections.
	 */
	sl->sl_busy = 0;
	WAKE_LISTENER(1);

	if ( s == AC_SOCKET_INVALID ) {
		int err = sock_errno();

		if(
#ifdef EMFILE
		    err == EMFILE ||
#endif /* EMFILE */
#ifdef ENFILE
		    err == ENFILE ||
#endif /* ENFILE */
		    0 )
		{
			ldap_pvt_thread_mutex_lock( &slap_daemon.sd_mutex );
			emfile++;
			/* Stop listening until an existing session closes */
			sl->sl_mute = 1;
			ldap_pvt_thread_mutex_unlock( &slap_daemon.sd_mutex );
		}

		Debug( LDAP_DEBUG_ANY,
			"daemon: accept(%ld) failed errno=%d (%s)\n",
			(long) sl->sl_sd, err, sock_errstr(err) );
		ldap_pvt_thread_yield();
		return 0;
	}
	sfd = SLAP_SOCKNEW( s );

	/* make sure descriptor number isn't too great */
	if ( sfd >= dtblsize ) {
		Debug( LDAP_DEBUG_ANY,
			"daemon: %ld beyond descriptor table size %ld\n",
			(long) sfd, (long) dtblsize, 0 );

		tcp_close(s);
		ldap_pvt_thread_yield();
		return 0;
	}

#ifdef LDAP_DEBUG
	ldap_pvt_thread_mutex_lock( &slap_daemon.sd_mutex );
	/* newly accepted stream should not be in any of the FD SETS */
	assert( SLAP_SOCK_NOT_ACTIVE( sfd ));
	ldap_pvt_thread_mutex_unlock( &slap_daemon.sd_mutex );
#endif /* LDAP_DEBUG */

#if defined( SO_KEEPALIVE ) || defined( TCP_NODELAY )
#ifdef LDAP_PF_LOCAL
	/* for IPv4 and IPv6 sockets only */
	if ( from.sa_addr.sa_family != AF_LOCAL )
#endif /* LDAP_PF_LOCAL */
	{
		int rc;
		int tmp;
#ifdef SO_KEEPALIVE
		/* enable keep alives */
		tmp = 1;
		rc = setsockopt( s, SOL_SOCKET, SO_KEEPALIVE,
			(char *) &tmp, sizeof(tmp) );
		if ( rc == AC_SOCKET_ERROR ) {
			int err = sock_errno();
			Debug( LDAP_DEBUG_ANY,
				"slapd(%ld): setsockopt(SO_KEEPALIVE) failed "
				"errno=%d (%s)\n", (long) sfd, err, sock_errstr(err) );
		}
#endif /* SO_KEEPALIVE */
#ifdef TCP_NODELAY
		/* enable no delay */
		tmp = 1;
		rc = setsockopt( s, IPPROTO_TCP, TCP_NODELAY,
			(char *)&tmp, sizeof(tmp) );
		if ( rc == AC_SOCKET_ERROR ) {
			int err = sock_errno();
			Debug( LDAP_DEBUG_ANY,
				"slapd(%ld): setsockopt(TCP_NODELAY) failed "
				"errno=%d (%s)\n", (long) sfd, err, sock_errstr(err) );
		}
#endif /* TCP_NODELAY */
	}
#endif /* SO_KEEPALIVE || TCP_NODELAY */

	Debug( LDAP_DEBUG_CONNS,
		"daemon: listen=%ld, new connection on %ld\n",
		(long) sl->sl_sd, (long) sfd, 0 );

	cflag = 0;
	switch ( from.sa_addr.sa_family ) {
#  ifdef LDAP_PF_LOCAL
	case AF_LOCAL:
		cflag |= CONN_IS_IPC;

		/* FIXME: apparently accept doesn't fill
		 * the sun_path sun_path member */
		if ( from.sa_un_addr.sun_path[0] == '\0' ) {
			AC_MEMCPY( from.sa_un_addr.sun_path,
					sl->sl_sa.sa_un_addr.sun_path,
					sizeof( from.sa_un_addr.sun_path ) );
		}

		sprintf( peername, "PATH=%s", from.sa_un_addr.sun_path );
		ssf = local_ssf;
		{
			uid_t uid;
			gid_t gid;

#ifdef LDAP_PF_LOCAL_SENDMSG
			peerbv.bv_val = peerbuf;
			peerbv.bv_len = sizeof( peerbuf );
#endif
			if( LUTIL_GETPEEREID( s, &uid, &gid, &peerbv ) == 0 ) {
				authid.bv_val = ch_malloc(
					STRLENOF( "gidNumber=4294967295+uidNumber=4294967295,"
					"cn=peercred,cn=external,cn=auth" ) + 1 );
				authid.bv_len = sprintf( authid.bv_val,
					"gidNumber=%d+uidNumber=%d,"
					"cn=peercred,cn=external,cn=auth",
					(int) gid, (int) uid );
				assert( authid.bv_len <=
					STRLENOF( "gidNumber=4294967295+uidNumber=4294967295,"
					"cn=peercred,cn=external,cn=auth" ) );
			}
		}
		dnsname = "local";
		break;
#endif /* LDAP_PF_LOCAL */

#  ifdef LDAP_PF_INET6
	case AF_INET6:
	if ( IN6_IS_ADDR_V4MAPPED(&from.sa_in6_addr.sin6_addr) ) {
		peeraddr = inet_ntoa( *((struct in_addr *)
					&from.sa_in6_addr.sin6_addr.s6_addr[12]) );
		sprintf( peername, "IP=%s:%d",
			 peeraddr != NULL ? peeraddr : SLAP_STRING_UNKNOWN,
			 (unsigned) ntohs( from.sa_in6_addr.sin6_port ) );
	} else {
		char addr[INET6_ADDRSTRLEN];

		peeraddr = (char *) inet_ntop( AF_INET6,
				      &from.sa_in6_addr.sin6_addr,
				      addr, sizeof addr );
		sprintf( peername, "IP=[%s]:%d",
			 peeraddr != NULL ? peeraddr : SLAP_STRING_UNKNOWN,
			 (unsigned) ntohs( from.sa_in6_addr.sin6_port ) );
	}
	break;
#  endif /* LDAP_PF_INET6 */

	case AF_INET:
		peeraddr = inet_ntoa( from.sa_in_addr.sin_addr );
		sprintf( peername, "IP=%s:%d",
			peeraddr != NULL ? peeraddr : SLAP_STRING_UNKNOWN,
			(unsigned) ntohs( from.sa_in_addr.sin_port ) );
		break;

	default:
		slapd_close(sfd);
		return 0;
	}

	if ( ( from.sa_addr.sa_family == AF_INET )
#ifdef LDAP_PF_INET6
		|| ( from.sa_addr.sa_family == AF_INET6 )
#endif /* LDAP_PF_INET6 */
		)
	{
		dnsname = NULL;
#ifdef SLAPD_RLOOKUPS
		if ( use_reverse_lookup ) {
			char *herr;
			if (ldap_pvt_get_hname( (const struct sockaddr *)&from, len, hbuf,
				sizeof(hbuf), &herr ) == 0) {
				ldap_pvt_str2lower( hbuf );
				dnsname = hbuf;
			}
		}
#endif /* SLAPD_RLOOKUPS */

#ifdef HAVE_TCPD
		{
			int rc;
			ldap_pvt_thread_mutex_lock( &slap_daemon.sd_tcpd_mutex );
			rc = hosts_ctl("slapd",
				dnsname != NULL ? dnsname : SLAP_STRING_UNKNOWN,
				peeraddr != NULL ? peeraddr : SLAP_STRING_UNKNOWN,
				SLAP_STRING_UNKNOWN );
			ldap_pvt_thread_mutex_unlock( &slap_daemon.sd_tcpd_mutex );
			if ( !rc ) {
				/* DENY ACCESS */
				Statslog( LDAP_DEBUG_STATS,
					"fd=%ld DENIED from %s (%s)\n",
					(long) sfd,
					dnsname != NULL ? dnsname : SLAP_STRING_UNKNOWN,
					peeraddr != NULL ? peeraddr : SLAP_STRING_UNKNOWN,
					0, 0 );
				slapd_close(sfd);
				return 0;
			}
		}
#endif /* HAVE_TCPD */
	}

#ifdef HAVE_TLS
	if ( sl->sl_is_tls ) cflag |= CONN_IS_TLS;
#endif
	c = connection_init(sfd, sl,
		dnsname != NULL ? dnsname : SLAP_STRING_UNKNOWN,
		peername, cflag, ssf,
		authid.bv_val ? &authid : NULL
		LDAP_PF_LOCAL_SENDMSG_ARG(&peerbv));

	if( authid.bv_val ) ch_free(authid.bv_val);

	if( !c ) {
		Debug( LDAP_DEBUG_ANY,
			"daemon: connection_init(%ld, %s, %s) failed.\n",
			(long) sfd, peername, sl->sl_name.bv_val );
		slapd_close(sfd);
		return 0;
	}

	Statslog( LDAP_DEBUG_STATS,
		"conn=%ld fd=%ld ACCEPT from %s (%s)\n",
		c->c_connid, (long) sfd, peername, sl->sl_name.bv_val,
		0 );

	return 0;
}

static void*
slap_listener_thread(
	void* ctx,
	void* ptr )
{
	int		rc;
	Listener	*sl = (Listener *)ptr;

	rc = slap_listener( sl );

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"slap_listener_thread(%s): failed err=%d",
			sl->sl_url.bv_val, rc, 0 );
	}

	return (void*)NULL;
}

static int
slap_listener_activate(
	Listener* sl )
{
	int rc;

	Debug( LDAP_DEBUG_TRACE, "slap_listener_activate(%d): %s\n",
		sl->sl_sd, sl->sl_busy ? "busy" : "", 0 );

	sl->sl_busy++;

	rc = ldap_pvt_thread_pool_submit( &connection_pool,
		slap_listener_thread, (void *) sl );

	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			"listener_activate(%d): submit failed (%d)\n",
			sl->sl_sd, rc, 0 );
	}
	return rc;
}

static void *
slapd_daemon_task(
	void *ptr )
{
	int l;
	time_t last_idle_check = 0;
	struct timeval idle;
	int ebadf = 0;

#define SLAPD_IDLE_CHECK_LIMIT 4

	if ( global_idletimeout > 0 ) {
		last_idle_check = slap_get_time();
		/* Set the select timeout.
		 * Don't just truncate, preserve the fractions of
		 * seconds to prevent sleeping for zero time.
		 */
		idle.tv_sec = global_idletimeout / SLAPD_IDLE_CHECK_LIMIT;
		idle.tv_usec = global_idletimeout - \
			( idle.tv_sec * SLAPD_IDLE_CHECK_LIMIT );
		idle.tv_usec *= 1000000 / SLAPD_IDLE_CHECK_LIMIT;
	} else {
		idle.tv_sec = 0;
		idle.tv_usec = 0;
	}

	slapd_add( wake_sds[0], 0, NULL );

	for ( l = 0; slap_listeners[l] != NULL; l++ ) {
		if ( slap_listeners[l]->sl_sd == AC_SOCKET_INVALID ) continue;

#ifdef LDAP_CONNECTIONLESS
		/* Since this is connectionless, the data port is the
		 * listening port. The listen() and accept() calls
		 * are unnecessary.
		 */
		if ( slap_listeners[l]->sl_is_udp )
			continue;
#endif /* LDAP_CONNECTIONLESS */

		if ( listen( SLAP_FD2SOCK( slap_listeners[l]->sl_sd ), SLAPD_LISTEN_BACKLOG ) == -1 ) {
			int err = sock_errno();

#ifdef LDAP_PF_INET6
			/* If error is EADDRINUSE, we are trying to listen to INADDR_ANY and
			 * we are already listening to in6addr_any, then we want to ignore
			 * this and continue.
			 */
			if ( err == EADDRINUSE ) {
				int i;
				struct sockaddr_in sa = slap_listeners[l]->sl_sa.sa_in_addr;
				struct sockaddr_in6 sa6;
				
				if ( sa.sin_family == AF_INET &&
				     sa.sin_addr.s_addr == htonl(INADDR_ANY) ) {
					for ( i = 0 ; i < l; i++ ) {
						sa6 = slap_listeners[i]->sl_sa.sa_in6_addr;
						if ( sa6.sin6_family == AF_INET6 &&
						     !memcmp( &sa6.sin6_addr, &in6addr_any,
								sizeof(struct in6_addr) ) )
						{
							break;
						}
					}

					if ( i < l ) {
						/* We are already listening to in6addr_any */
						Debug( LDAP_DEBUG_CONNS,
							"daemon: Attempt to listen to 0.0.0.0 failed, "
							"already listening on ::, assuming IPv4 included\n",
							0, 0, 0 );
						slapd_close( slap_listeners[l]->sl_sd );
						slap_listeners[l]->sl_sd = AC_SOCKET_INVALID;
						continue;
					}
				}
			}
#endif /* LDAP_PF_INET6 */
			Debug( LDAP_DEBUG_ANY,
				"daemon: listen(%s, 5) failed errno=%d (%s)\n",
					slap_listeners[l]->sl_url.bv_val, err,
					sock_errstr(err) );
			return (void*)-1;
		}

		/* make the listening socket non-blocking */
		if ( ber_pvt_socket_set_nonblock( SLAP_FD2SOCK( slap_listeners[l]->sl_sd ), 1 ) < 0 ) {
			Debug( LDAP_DEBUG_ANY, "slapd_daemon_task: "
				"set nonblocking on a listening socket failed\n",
				0, 0, 0 );
			slapd_shutdown = 2;
			return (void*)-1;
		}

		slapd_add( slap_listeners[l]->sl_sd, 0, slap_listeners[l] );
	}

#ifdef HAVE_NT_SERVICE_MANAGER
	if ( started_event != NULL ) {
		ldap_pvt_thread_cond_signal( &started_event );
	}
#endif /* HAVE_NT_SERVICE_MANAGER */

	/* initialization complete. Here comes the loop. */

	while ( !slapd_shutdown ) {
		ber_socket_t		i;
		int			ns, nwriters;
		int			at;
		ber_socket_t		nfds;
#if SLAP_EVENTS_ARE_INDEXED
		ber_socket_t		nrfds, nwfds;
#endif /* SLAP_EVENTS_ARE_INDEXED */
#define SLAPD_EBADF_LIMIT 16

		time_t			now;

		SLAP_EVENT_DECL;

		struct timeval		tv;
		struct timeval		*tvp;

		struct timeval		cat;
		time_t			tdelta = 1;
		struct re_s*		rtask;

		now = slap_get_time();

		if ( ( global_idletimeout > 0 ) &&
			difftime( last_idle_check +
				global_idletimeout/SLAPD_IDLE_CHECK_LIMIT, now ) < 0 )
		{
			connections_timeout_idle( now );
			last_idle_check = now;
		}
		tv = idle;

#ifdef SIGHUP
		if ( slapd_gentle_shutdown ) {
			ber_socket_t active;

			if ( slapd_gentle_shutdown == 1 ) {
				BackendDB *be;
				Debug( LDAP_DEBUG_ANY, "slapd gentle shutdown\n", 0, 0, 0 );
				close_listeners( 1 );
				frontendDB->be_restrictops |= SLAP_RESTRICT_OP_WRITES;
				LDAP_STAILQ_FOREACH(be, &backendDB, be_next) {
					be->be_restrictops |= SLAP_RESTRICT_OP_WRITES;
				}
				slapd_gentle_shutdown = 2;
			}

			ldap_pvt_thread_mutex_lock( &slap_daemon.sd_mutex );
			active = slap_daemon.sd_nactives;
			ldap_pvt_thread_mutex_unlock( &slap_daemon.sd_mutex );
			if ( active == 0 ) {
				slapd_shutdown = 1;
				break;
			}
		}
#endif /* SIGHUP */
		at = 0;

		ldap_pvt_thread_mutex_lock( &slap_daemon.sd_mutex );

		nwriters = slap_daemon.sd_nwriters;

		for ( l = 0; slap_listeners[l] != NULL; l++ ) {
			Listener *lr = slap_listeners[l];

			if ( lr->sl_sd == AC_SOCKET_INVALID ) continue;

			if ( lr->sl_mute || lr->sl_busy )
			{
				SLAP_SOCK_CLR_READ( lr->sl_sd );
			} else {
				SLAP_SOCK_SET_READ( lr->sl_sd );
			}
		}

		SLAP_EVENT_INIT;

		nfds = SLAP_EVENT_MAX;

		if ( global_idletimeout && slap_daemon.sd_nactives ) at = 1;

		ldap_pvt_thread_mutex_unlock( &slap_daemon.sd_mutex );

		if ( at 
#if defined(HAVE_YIELDING_SELECT) || defined(NO_THREADS)
			&&  ( tv.tv_sec || tv.tv_usec )
#endif /* HAVE_YIELDING_SELECT || NO_THREADS */
			)
		{
			tvp = &tv;
		} else {
			tvp = NULL;
		}

		ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
		rtask = ldap_pvt_runqueue_next_sched( &slapd_rq, &cat );
		while ( rtask && cat.tv_sec && cat.tv_sec <= now ) {
			if ( ldap_pvt_runqueue_isrunning( &slapd_rq, rtask )) {
				ldap_pvt_runqueue_resched( &slapd_rq, rtask, 0 );
			} else {
				ldap_pvt_runqueue_runtask( &slapd_rq, rtask );
				ldap_pvt_runqueue_resched( &slapd_rq, rtask, 0 );
				ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
				ldap_pvt_thread_pool_submit( &connection_pool,
					rtask->routine, (void *) rtask );
				ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
			}
			rtask = ldap_pvt_runqueue_next_sched( &slapd_rq, &cat );
		}
		ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );

		if ( rtask && cat.tv_sec ) {
			/* NOTE: diff __should__ always be >= 0,
			 * AFAI understand; however (ITS#4872),
			 * time_t might be unsigned in some systems,
			 * while difftime() returns a double */
			double diff = difftime( cat.tv_sec, now );
			if ( diff <= 0 ) {
				diff = tdelta;
			}
			if ( tvp == NULL || diff < tv.tv_sec ) {
				tv.tv_sec = diff;
				tv.tv_usec = 0;
				tvp = &tv;
			}
		}

		for ( l = 0; slap_listeners[l] != NULL; l++ ) {
			Listener *lr = slap_listeners[l];

			if ( lr->sl_sd == AC_SOCKET_INVALID ) {
				continue;
			}

			if ( lr->sl_mute ) {
				Debug( LDAP_DEBUG_CONNS,
					"daemon: " SLAP_EVENT_FNAME ": "
					"listen=%d muted\n",
					lr->sl_sd, 0, 0 );
				continue;
			}

			if ( lr->sl_busy ) {
				Debug( LDAP_DEBUG_CONNS,
					"daemon: " SLAP_EVENT_FNAME ": "
					"listen=%d busy\n",
					lr->sl_sd, 0, 0 );
				continue;
			}

			Debug( LDAP_DEBUG_CONNS,
				"daemon: " SLAP_EVENT_FNAME ": "
				"listen=%d active_threads=%d tvp=%s\n",
				lr->sl_sd, at, tvp == NULL ? "NULL" : "zero" );
		}

		SLAP_EVENT_WAIT( tvp, &ns );
		switch ( ns ) {
		case -1: {	/* failure - try again */
				int err = sock_errno();

				if ( err != EINTR ) {
					ebadf++;

					/* Don't log unless we got it twice in a row */
					if ( !( ebadf & 1 ) ) {
						Debug( LDAP_DEBUG_ANY,
							"daemon: "
							SLAP_EVENT_FNAME
							"failed count %d "
							"err (%d): %s\n",
							ebadf, err,
							sock_errstr( err ) );
					}
					if ( ebadf >= SLAPD_EBADF_LIMIT ) {
						slapd_shutdown = 2;
					}
				}
			}
			continue;

		case 0:		/* timeout - let threads run */
			ebadf = 0;
#ifndef HAVE_YIELDING_SELECT
			Debug( LDAP_DEBUG_CONNS, "daemon: " SLAP_EVENT_FNAME
				"timeout - yielding\n",
				0, 0, 0 );

			ldap_pvt_thread_yield();
#endif /* ! HAVE_YIELDING_SELECT */
			continue;

		default:	/* something happened - deal with it */
			if ( slapd_shutdown ) continue;

			ebadf = 0;
			Debug( LDAP_DEBUG_CONNS,
				"daemon: activity on %d descriptor%s\n",
				ns, ns != 1 ? "s" : "", 0 );
			/* FALL THRU */
		}

#if SLAP_EVENTS_ARE_INDEXED
		if ( SLAP_EVENT_IS_READ( wake_sds[0] ) ) {
			char c[BUFSIZ];
			SLAP_EVENT_CLR_READ( wake_sds[0] );
			waking = 0;
			tcp_read( SLAP_FD2SOCK(wake_sds[0]), c, sizeof(c) );
			Debug( LDAP_DEBUG_CONNS, "daemon: waked\n", 0, 0, 0 );
			continue;
		}

		/* The event slot equals the descriptor number - this is
		 * true for Unix select and poll. We treat Windows select
		 * like this too, even though it's a kludge.
		 */
		for ( l = 0; slap_listeners[l] != NULL; l++ ) {
			int rc;

			if ( ns <= 0 ) break;
			if ( slap_listeners[l]->sl_sd == AC_SOCKET_INVALID ) continue;
#ifdef LDAP_CONNECTIONLESS
			if ( slap_listeners[l]->sl_is_udp ) continue;
#endif /* LDAP_CONNECTIONLESS */
			if ( !SLAP_EVENT_IS_READ( slap_listeners[l]->sl_sd ) ) continue;
			
			/* clear events */
			SLAP_EVENT_CLR_READ( slap_listeners[l]->sl_sd );
			SLAP_EVENT_CLR_WRITE( slap_listeners[l]->sl_sd );
			ns--;

			rc = slap_listener_activate( slap_listeners[l] );
		}

		/* bypass the following tests if no descriptors left */
		if ( ns <= 0 ) {
#ifndef HAVE_YIELDING_SELECT
			ldap_pvt_thread_yield();
#endif /* HAVE_YIELDING_SELECT */
			continue;
		}

		Debug( LDAP_DEBUG_CONNS, "daemon: activity on:", 0, 0, 0 );
		nrfds = 0;
		nwfds = 0;
		for ( i = 0; i < nfds; i++ ) {
			int	r, w;

			r = SLAP_EVENT_IS_READ( i );
			/* writefds was not initialized if nwriters was zero */
			w = nwriters ? SLAP_EVENT_IS_WRITE( i ) : 0;
			if ( r || w ) {
				Debug( LDAP_DEBUG_CONNS, " %d%s%s", i,
				    r ? "r" : "", w ? "w" : "" );
				if ( r ) {
					nrfds++;
					ns--;
				}
				if ( w ) {
					nwfds++;
					ns--;
				}
			}
			if ( ns <= 0 ) break;
		}
		Debug( LDAP_DEBUG_CONNS, "\n", 0, 0, 0 );

		/* loop through the writers */
		for ( i = 0; nwfds > 0; i++ ) {
			ber_socket_t wd;
			if ( ! SLAP_EVENT_IS_WRITE( i ) ) continue;
			wd = i;

			SLAP_EVENT_CLR_WRITE( wd );
			nwfds--;

			Debug( LDAP_DEBUG_CONNS,
				"daemon: write active on %d\n",
				wd, 0, 0 );

			/*
			 * NOTE: it is possible that the connection was closed
			 * and that the stream is now inactive.
			 * connection_write() must validate the stream is still
			 * active.
			 *
			 * ITS#4338: if the stream is invalid, there is no need to
			 * close it here. It has already been closed in connection.c.
			 */
			if ( connection_write( wd ) < 0 ) {
				if ( SLAP_EVENT_IS_READ( wd ) ) {
					SLAP_EVENT_CLR_READ( (unsigned) wd );
					nrfds--;
				}
			}
		}

		for ( i = 0; nrfds > 0; i++ ) {
			ber_socket_t rd;
			if ( ! SLAP_EVENT_IS_READ( i ) ) continue;
			rd = i;
			SLAP_EVENT_CLR_READ( rd );
			nrfds--;

			Debug ( LDAP_DEBUG_CONNS,
				"daemon: read activity on %d\n", rd, 0, 0 );
			/*
			 * NOTE: it is possible that the connection was closed
			 * and that the stream is now inactive.
			 * connection_read() must valid the stream is still
			 * active.
			 */

			connection_read_activate( rd );
		}
#else	/* !SLAP_EVENTS_ARE_INDEXED */
	/* FIXME */
	/* The events are returned in an arbitrary list. This is true
	 * for /dev/poll, epoll and kqueue. In order to prioritize things
	 * so that we can handle wake_sds first, listeners second, and then
	 * all other connections last (as we do for select), we would need
	 * to use multiple event handles and cascade them.
	 *
	 * That seems like a bit of hassle. So the wake_sds check has been
	 * skipped. For epoll and kqueue we can associate arbitrary data with
	 * an event, so we could use pointers to the listener structure
	 * instead of just the file descriptor. For /dev/poll we have to
	 * search the listeners array for a matching descriptor.
	 *
	 * We now handle wake events when we see them; they are not given
	 * higher priority.
	 */
#ifdef LDAP_DEBUG
		Debug( LDAP_DEBUG_CONNS, "daemon: activity on:", 0, 0, 0 );

		for ( i = 0; i < ns; i++ ) {
			int	r, w, fd;

			/* Don't log listener events */
			if ( SLAP_EVENT_IS_LISTENER( i )
#ifdef LDAP_CONNECTIONLESS
				&& !( (SLAP_EVENT_LISTENER( i ))->sl_is_udp )
#endif /* LDAP_CONNECTIONLESS */
				)
			{
				continue;
			}

			fd = SLAP_EVENT_FD( i );
			/* Don't log internal wake events */
			if ( fd == wake_sds[0] ) continue;

			r = SLAP_EVENT_IS_READ( i );
			w = SLAP_EVENT_IS_WRITE( i );
			if ( r || w ) {
				Debug( LDAP_DEBUG_CONNS, " %d%s%s", fd,
				    r ? "r" : "", w ? "w" : "" );
			}
		}
		Debug( LDAP_DEBUG_CONNS, "\n", 0, 0, 0 );
#endif /* LDAP_DEBUG */

		for ( i = 0; i < ns; i++ ) {
			int rc = 1, fd;

			if ( SLAP_EVENT_IS_LISTENER( i ) ) {
				rc = slap_listener_activate( SLAP_EVENT_LISTENER( i ) );
			}

			/* If we found a regular listener, rc is now zero, and we
			 * can skip the data portion. But if it was a UDP listener
			 * then rc is still 1, and we want to handle the data.
			 */
			if ( rc ) {
				fd = SLAP_EVENT_FD( i );

				/* Handle wake events */
				if ( fd == wake_sds[0] ) {
					char c[BUFSIZ];
					waking = 0;
					tcp_read( SLAP_FD2SOCK(wake_sds[0]), c, sizeof(c) );
					break;
				}

				if ( SLAP_EVENT_IS_WRITE( i ) ) {
					Debug( LDAP_DEBUG_CONNS,
						"daemon: write active on %d\n",
						fd, 0, 0 );

					SLAP_EVENT_CLR_WRITE( i );

					/*
					 * NOTE: it is possible that the connection was closed
					 * and that the stream is now inactive.
					 * connection_write() must valid the stream is still
					 * active.
					 */
					if ( connection_write( fd ) < 0 ) {
						continue;
					}
				}
				/* If event is a read */
				if ( SLAP_EVENT_IS_READ( i ) ) {
					Debug( LDAP_DEBUG_CONNS,
						"daemon: read active on %d\n",
						fd, 0, 0 );

					SLAP_EVENT_CLR_READ( i );
					connection_read_activate( fd );
				} else {
					Debug( LDAP_DEBUG_CONNS,
						"daemon: hangup on %d\n", fd, 0, 0 );
				}
			}
		}
#endif	/* SLAP_EVENTS_ARE_INDEXED */

#ifndef HAVE_YIELDING_SELECT
		ldap_pvt_thread_yield();
#endif /* ! HAVE_YIELDING_SELECT */
	}

	if ( slapd_shutdown == 1 ) {
		Debug( LDAP_DEBUG_ANY,
			"daemon: shutdown requested and initiated.\n",
			0, 0, 0 );

	} else if ( slapd_shutdown == 2 ) {
#ifdef HAVE_NT_SERVICE_MANAGER
			Debug( LDAP_DEBUG_ANY,
			       "daemon: shutdown initiated by Service Manager.\n",
			       0, 0, 0);
#else /* !HAVE_NT_SERVICE_MANAGER */
			Debug( LDAP_DEBUG_ANY,
			       "daemon: abnormal condition, shutdown initiated.\n",
			       0, 0, 0 );
#endif /* !HAVE_NT_SERVICE_MANAGER */
	} else {
		Debug( LDAP_DEBUG_ANY,
		       "daemon: no active streams, shutdown initiated.\n",
		       0, 0, 0 );
	}

	if ( slapd_gentle_shutdown != 2 ) close_listeners ( 0 );

	if ( !slapd_gentle_shutdown ) {
		slapd_abrupt_shutdown = 1;
		connections_shutdown();
	}

	Debug( LDAP_DEBUG_ANY,
		"slapd shutdown: waiting for %d threads to terminate\n",
		ldap_pvt_thread_pool_backload( &connection_pool ), 0, 0 );
	ldap_pvt_thread_pool_destroy( &connection_pool, 1 );

	free( slap_listeners );
	slap_listeners = NULL;

	return NULL;
}


#ifdef LDAP_CONNECTIONLESS
static int
connectionless_init( void )
{
	int l;

	for ( l = 0; slap_listeners[l] != NULL; l++ ) {
		Listener *lr = slap_listeners[l];
		Connection *c;

		if ( !lr->sl_is_udp ) {
			continue;
		}

		c = connection_init( lr->sl_sd, lr, "", "",
			CONN_IS_UDP, (slap_ssf_t) 0, NULL
			LDAP_PF_LOCAL_SENDMSG_ARG(NULL));

		if ( !c ) {
			Debug( LDAP_DEBUG_TRACE,
				"connectionless_init: failed on %s (%d)\n",
				lr->sl_url, lr->sl_sd, 0 );
			return -1;
		}
		lr->sl_is_udp++;
	}

	return 0;
}
#endif /* LDAP_CONNECTIONLESS */

int
slapd_daemon( void )
{
	int rc;

#ifdef LDAP_CONNECTIONLESS
	connectionless_init();
#endif /* LDAP_CONNECTIONLESS */

#define SLAPD_LISTENER_THREAD 1
#if defined( SLAPD_LISTENER_THREAD )
	{
		ldap_pvt_thread_t	listener_tid;

		/* listener as a separate THREAD */
		rc = ldap_pvt_thread_create( &listener_tid,
			0, slapd_daemon_task, NULL );

		if ( rc != 0 ) {
			Debug( LDAP_DEBUG_ANY,
			"listener ldap_pvt_thread_create failed (%d)\n", rc, 0, 0 );
			return rc;
		}
 
  		/* wait for the listener thread to complete */
  		ldap_pvt_thread_join( listener_tid, (void *)NULL );
	}
#else /* ! SLAPD_LISTENER_THREAD */
	/* experimental code */
	slapd_daemon_task( NULL );
#endif /* ! SLAPD_LISTENER_THREAD */

	return 0;
}

static int
sockinit( void )
{
#if defined( HAVE_WINSOCK2 )
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 2, 0 );

	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		/* Tell the user that we couldn't find a usable */
		/* WinSock DLL.					 */
		return -1;
	}

	/* Confirm that the WinSock DLL supports 2.0.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.0 in addition to 2.0, it will still return */
	/* 2.0 in wVersion since that is the version we	     */
	/* requested.					     */

	if ( LOBYTE( wsaData.wVersion ) != 2 ||
		HIBYTE( wsaData.wVersion ) != 0 )
	{
	    /* Tell the user that we couldn't find a usable */
	    /* WinSock DLL.				     */
	    WSACleanup();
	    return -1;
	}

	/* The WinSock DLL is acceptable. Proceed. */
#elif defined( HAVE_WINSOCK )
	WSADATA wsaData;
	if ( WSAStartup( 0x0101, &wsaData ) != 0 ) return -1;
#endif /* ! HAVE_WINSOCK2 && ! HAVE_WINSOCK */

	return 0;
}

static int
sockdestroy( void )
{
#if defined( HAVE_WINSOCK2 ) || defined( HAVE_WINSOCK )
	WSACleanup();
#endif /* HAVE_WINSOCK2 || HAVE_WINSOCK */
	SLAP_SOCK_DESTROY;

	return 0;
}

RETSIGTYPE
slap_sig_shutdown( int sig )
{
#if 0
	Debug(LDAP_DEBUG_TRACE, "slap_sig_shutdown: signal %d\n", sig, 0, 0);
#endif

	/*
	 * If the NT Service Manager is controlling the server, we don't
	 * want SIGBREAK to kill the server. For some strange reason,
	 * SIGBREAK is generated when a user logs out.
	 */

#if defined(HAVE_NT_SERVICE_MANAGER) && defined(SIGBREAK)
	if (is_NT_Service && sig == SIGBREAK) {
		/* empty */;
	} else
#endif /* HAVE_NT_SERVICE_MANAGER && SIGBREAK */
#ifdef SIGHUP
	if (sig == SIGHUP && global_gentlehup && slapd_gentle_shutdown == 0) {
		slapd_gentle_shutdown = 1;
	} else
#endif /* SIGHUP */
	{
		slapd_shutdown = 1;
	}

	WAKE_LISTENER(1);

	/* reinstall self */
	(void) SIGNAL_REINSTALL( sig, slap_sig_shutdown );
}

RETSIGTYPE
slap_sig_wake( int sig )
{
	WAKE_LISTENER(1);

	/* reinstall self */
	(void) SIGNAL_REINSTALL( sig, slap_sig_wake );
}


void
slapd_add_internal( ber_socket_t s, int isactive )
{
	slapd_add( s, isactive, NULL );
}

Listener **
slapd_get_listeners( void )
{
	return slap_listeners;
}

void
slap_wake_listener()
{
	WAKE_LISTENER(1);
}
