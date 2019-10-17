/*	$NetBSD: socket.h,v 1.4.4.1 2019/10/17 19:34:21 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef ISC_SOCKET_H
#define ISC_SOCKET_H 1

/*****
 ***** Module Info
 *****/

/*! \file isc/socket.h
 * \brief Provides TCP and UDP sockets for network I/O.  The sockets are event
 * sources in the task system.
 *
 * When I/O completes, a completion event for the socket is posted to the
 * event queue of the task which requested the I/O.
 *
 * \li MP:
 *	The module ensures appropriate synchronization of data structures it
 *	creates and manipulates.
 *	Clients of this module must not be holding a socket's task's lock when
 *	making a call that affects that socket.  Failure to follow this rule
 *	can result in deadlock.
 *	The caller must ensure that isc_socketmgr_destroy() is called only
 *	once for a given manager.
 *
 * \li Reliability:
 *	No anticipated impact.
 *
 * \li Resources:
 *	TBS
 *
 * \li Security:
 *	No anticipated impact.
 *
 * \li Standards:
 *	None.
 */

/***
 *** Imports
 ***/

#include <inttypes.h>
#include <stdbool.h>

#include <isc/event.h>
#include <isc/eventclass.h>
#include <isc/lang.h>
#include <isc/json.h>
#include <isc/region.h>
#include <isc/sockaddr.h>
#include <isc/time.h>
#include <isc/types.h>
#include <isc/xml.h>

ISC_LANG_BEGINDECLS

/***
 *** Constants
 ***/

/*%
 * Maximum number of buffers in a scatter/gather read/write.  The operating
 * system in use must support at least this number (plus one on some.)
 */
#define ISC_SOCKET_MAXSCATTERGATHER	8

/*@{*/
/*!
 * Socket options:
 *
 * _REUSEADDRESS:	Set SO_REUSEADDR prior to calling bind(),
 * 			if a non-zero port is specified (applies to
 * 			AF_INET and AF_INET6).
 */
typedef enum {
	ISC_SOCKET_REUSEADDRESS	= 0x01U,
} isc_socket_options_t;
/*@}*/

/*%
 * Statistics counters.  Used as isc_statscounter_t values.
 */
enum {
	isc_sockstatscounter_udp4open = 0,
	isc_sockstatscounter_udp6open = 1,
	isc_sockstatscounter_tcp4open = 2,
	isc_sockstatscounter_tcp6open = 3,
	isc_sockstatscounter_unixopen = 4,

	isc_sockstatscounter_udp4openfail = 5,
	isc_sockstatscounter_udp6openfail = 6,
	isc_sockstatscounter_tcp4openfail = 7,
	isc_sockstatscounter_tcp6openfail = 8,
	isc_sockstatscounter_unixopenfail = 9,

	isc_sockstatscounter_udp4close = 10,
	isc_sockstatscounter_udp6close = 11,
	isc_sockstatscounter_tcp4close = 12,
	isc_sockstatscounter_tcp6close = 13,
	isc_sockstatscounter_unixclose = 14,
	isc_sockstatscounter_fdwatchclose = 15,

	isc_sockstatscounter_udp4bindfail = 16,
	isc_sockstatscounter_udp6bindfail = 17,
	isc_sockstatscounter_tcp4bindfail = 18,
	isc_sockstatscounter_tcp6bindfail = 19,
	isc_sockstatscounter_unixbindfail = 20,
	isc_sockstatscounter_fdwatchbindfail = 21,

	isc_sockstatscounter_udp4connect = 22,
	isc_sockstatscounter_udp6connect = 23,
	isc_sockstatscounter_tcp4connect = 24,
	isc_sockstatscounter_tcp6connect = 25,
	isc_sockstatscounter_unixconnect = 26,
	isc_sockstatscounter_fdwatchconnect = 27,

	isc_sockstatscounter_udp4connectfail = 28,
	isc_sockstatscounter_udp6connectfail = 29,
	isc_sockstatscounter_tcp4connectfail = 30,
	isc_sockstatscounter_tcp6connectfail = 31,
	isc_sockstatscounter_unixconnectfail = 32,
	isc_sockstatscounter_fdwatchconnectfail = 33,

	isc_sockstatscounter_tcp4accept = 34,
	isc_sockstatscounter_tcp6accept = 35,
	isc_sockstatscounter_unixaccept = 36,

	isc_sockstatscounter_tcp4acceptfail = 37,
	isc_sockstatscounter_tcp6acceptfail = 38,
	isc_sockstatscounter_unixacceptfail = 39,

	isc_sockstatscounter_udp4sendfail = 40,
	isc_sockstatscounter_udp6sendfail = 41,
	isc_sockstatscounter_tcp4sendfail = 42,
	isc_sockstatscounter_tcp6sendfail = 43,
	isc_sockstatscounter_unixsendfail = 44,
	isc_sockstatscounter_fdwatchsendfail = 45,

	isc_sockstatscounter_udp4recvfail = 46,
	isc_sockstatscounter_udp6recvfail = 47,
	isc_sockstatscounter_tcp4recvfail = 48,
	isc_sockstatscounter_tcp6recvfail = 49,
	isc_sockstatscounter_unixrecvfail = 50,
	isc_sockstatscounter_fdwatchrecvfail = 51,

	isc_sockstatscounter_udp4active = 52,
	isc_sockstatscounter_udp6active = 53,
	isc_sockstatscounter_tcp4active = 54,
	isc_sockstatscounter_tcp6active = 55,
	isc_sockstatscounter_unixactive = 56,

	isc_sockstatscounter_rawopen = 57,
	isc_sockstatscounter_rawopenfail = 58,
	isc_sockstatscounter_rawclose = 59,
	isc_sockstatscounter_rawrecvfail = 60,
	isc_sockstatscounter_rawactive = 61,

	isc_sockstatscounter_max = 62
};

/*@{*/
/*!
 * _ATTACHED:	Internal use only.
 * _TRUNC:	Packet was truncated on receive.
 * _CTRUNC:	Packet control information was truncated.  This can
 *		indicate that the packet is not complete, even though
 *		all the data is valid.
 * _TIMESTAMP:	The timestamp member is valid.
 * _PKTINFO:	The pktinfo member is valid.
 * _MULTICAST:	The UDP packet was received via a multicast transmission.
 * _DSCP:	The UDP DSCP value is valid.
 * _USEMINMTU:	Set the per packet IPV6_USE_MIN_MTU flag.
 */
typedef enum {
	ISC_SOCKEVENTATTR_ATTACHED =	0x10000000U, /* internal */
	ISC_SOCKEVENTATTR_TRUNC =	0x00800000U, /* public */
	ISC_SOCKEVENTATTR_CTRUNC =	0x00400000U, /* public */
	ISC_SOCKEVENTATTR_TIMESTAMP =	0x00200000U, /* public */
	ISC_SOCKEVENTATTR_PKTINFO =	0x00100000U, /* public */
	ISC_SOCKEVENTATTR_MULTICAST =	0x00080000U, /* public */
	ISC_SOCKEVENTATTR_DSCP =	0x00040000U, /* public */
	ISC_SOCKEVENTATTR_USEMINMTU =	0x00020000U  /* public */
} isc_sockeventattr_t;
/*@}*/

/***
 *** Types
 ***/

struct isc_socketevent {
	ISC_EVENT_COMMON(isc_socketevent_t);
	isc_result_t		result;		/*%< OK, EOF, whatever else */
	unsigned int		minimum;	/*%< minimum i/o for event */
	unsigned int		n;		/*%< bytes read or written */
	unsigned int		offset;		/*%< offset into buffer list */
	isc_region_t		region;		/*%< for single-buffer i/o */
	isc_sockaddr_t		address;	/*%< source address */
	isc_time_t		timestamp;	/*%< timestamp of packet recv */
	struct in6_pktinfo	pktinfo;	/*%< ipv6 pktinfo */
	isc_sockeventattr_t	attributes;	/*%< see isc_sockeventattr_t
						     enum */
	isc_eventdestructor_t   destroy;	/*%< original destructor */
	unsigned int		dscp;		/*%< UDP dscp value */
};

typedef struct isc_socket_newconnev isc_socket_newconnev_t;
struct isc_socket_newconnev {
	ISC_EVENT_COMMON(isc_socket_newconnev_t);
	isc_socket_t *		newsocket;
	isc_result_t		result;		/*%< OK, EOF, whatever else */
	isc_sockaddr_t		address;	/*%< source address */
};

typedef struct isc_socket_connev isc_socket_connev_t;
struct isc_socket_connev {
	ISC_EVENT_COMMON(isc_socket_connev_t);
	isc_result_t		result;		/*%< OK, EOF, whatever else */
};

#define ISC_SOCKEVENT_ANYEVENT  (0)
#define ISC_SOCKEVENT_RECVDONE	(ISC_EVENTCLASS_SOCKET + 1)
#define ISC_SOCKEVENT_SENDDONE	(ISC_EVENTCLASS_SOCKET + 2)
#define ISC_SOCKEVENT_NEWCONN	(ISC_EVENTCLASS_SOCKET + 3)
#define ISC_SOCKEVENT_CONNECT	(ISC_EVENTCLASS_SOCKET + 4)

/*
 * Internal events.
 */
#define ISC_SOCKEVENT_INTR	(ISC_EVENTCLASS_SOCKET + 256)
#define ISC_SOCKEVENT_INTW	(ISC_EVENTCLASS_SOCKET + 257)

typedef enum {
	isc_sockettype_udp = 1,
	isc_sockettype_tcp = 2,
	isc_sockettype_unix = 3,
	isc_sockettype_raw = 4,
	isc_sockettype_fdwatch = 5
} isc_sockettype_t;

/*@{*/
/*!
 * How a socket should be shutdown in isc_socket_shutdown() calls.
 */
#define ISC_SOCKSHUT_RECV	0x00000001	/*%< close read side */
#define ISC_SOCKSHUT_SEND	0x00000002	/*%< close write side */
#define ISC_SOCKSHUT_ALL	0x00000003	/*%< close them all */
/*@}*/

/*@{*/
/*!
 * What I/O events to cancel in isc_socket_cancel() calls.
 */
#define ISC_SOCKCANCEL_RECV	0x00000001	/*%< cancel recv */
#define ISC_SOCKCANCEL_SEND	0x00000002	/*%< cancel send */
#define ISC_SOCKCANCEL_ACCEPT	0x00000004	/*%< cancel accept */
#define ISC_SOCKCANCEL_CONNECT	0x00000008	/*%< cancel connect */
#define ISC_SOCKCANCEL_ALL	0x0000000f	/*%< cancel everything */
/*@}*/

/*@{*/
/*!
 * Flags for isc_socket_send() and isc_socket_recv() calls.
 */
#define ISC_SOCKFLAG_IMMEDIATE	0x00000001	/*%< send event only if needed */
#define ISC_SOCKFLAG_NORETRY	0x00000002	/*%< drop failed UDP sends */
/*@}*/

/*%
 * This structure is actually just the common prefix of a socket manager
 * object implementation's version of an isc_socketmgr_t.
 * \brief
 * Direct use of this structure by clients is forbidden.  socket implementations
 * may change the structure.  'magic' must be ISCAPI_SOCKETMGR_MAGIC for any
 * of the isc_socket_ routines to work.  socket implementations must maintain
 * all socket invariants.
 * In effect, this definition is used only for non-BIND9 version ("export")
 * of the library, and the export version does not work for win32.  So, to avoid
 * the definition conflict with win32/socket.c, we enable this definition only
 * for non-Win32 (i.e. Unix) platforms.
 */
#ifndef WIN32
struct isc_socketmgr {
	unsigned int		impmagic;
	unsigned int		magic;
};
#endif

#define ISCAPI_SOCKETMGR_MAGIC		ISC_MAGIC('A','s','m','g')
#define ISCAPI_SOCKETMGR_VALID(m)	((m) != NULL && \
					 (m)->magic == ISCAPI_SOCKETMGR_MAGIC)

/*%
 * This is the common prefix of a socket object.  The same note as
 * that for the socketmgr structure applies.
 */
#ifndef WIN32
struct isc_socket {
	unsigned int		impmagic;
	unsigned int		magic;
};
#endif

#define ISCAPI_SOCKET_MAGIC	ISC_MAGIC('A','s','c','t')
#define ISCAPI_SOCKET_VALID(s)	((s) != NULL && \
				 (s)->magic == ISCAPI_SOCKET_MAGIC)

/***
 *** Socket and Socket Manager Functions
 ***
 *** Note: all Ensures conditions apply only if the result is success for
 *** those functions which return an isc_result.
 ***/

isc_result_t
isc_socket_create(isc_socketmgr_t *manager,
		  int pf,
		  isc_sockettype_t type,
		  isc_socket_t **socketp);
/*%<
 * Create a new 'type' socket managed by 'manager'.
 *
 * Note:
 *
 *\li	'pf' is the desired protocol family, e.g. PF_INET or PF_INET6.
 *
 * Requires:
 *
 *\li	'manager' is a valid manager
 *
 *\li	'socketp' is a valid pointer, and *socketp == NULL
 *
 * Ensures:
 *
 *	'*socketp' is attached to the newly created socket
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_NORESOURCES
 *\li	#ISC_R_UNEXPECTED
 */

isc_result_t
isc_socket_dup(isc_socket_t *sock0, isc_socket_t **socketp);
/*%<
 * Duplicate an existing socket, reusing its file descriptor.
 */

void
isc_socket_cancel(isc_socket_t *sock, isc_task_t *task,
		  unsigned int how);
/*%<
 * Cancel pending I/O of the type specified by "how".
 *
 * Note: if "task" is NULL, then the cancel applies to all tasks using the
 * socket.
 *
 * Requires:
 *
 * \li	"socket" is a valid socket
 *
 * \li	"task" is NULL or a valid task
 *
 * "how" is a bitmask describing the type of cancelation to perform.
 * The type ISC_SOCKCANCEL_ALL will cancel all pending I/O on this
 * socket.
 *
 * \li ISC_SOCKCANCEL_RECV:
 *	Cancel pending isc_socket_recv() calls.
 *
 * \li ISC_SOCKCANCEL_SEND:
 *	Cancel pending isc_socket_send() and isc_socket_sendto() calls.
 *
 * \li ISC_SOCKCANCEL_ACCEPT:
 *	Cancel pending isc_socket_accept() calls.
 *
 * \li ISC_SOCKCANCEL_CONNECT:
 *	Cancel pending isc_socket_connect() call.
 */

void
isc_socket_shutdown(isc_socket_t *sock, unsigned int how);
/*%<
 * Shutdown 'socket' according to 'how'.
 *
 * Requires:
 *
 * \li	'socket' is a valid socket.
 *
 * \li	'task' is NULL or is a valid task.
 *
 * \li	If 'how' is 'ISC_SOCKSHUT_RECV' or 'ISC_SOCKSHUT_ALL' then
 *
 *		The read queue must be empty.
 *
 *		No further read requests may be made.
 *
 * \li	If 'how' is 'ISC_SOCKSHUT_SEND' or 'ISC_SOCKSHUT_ALL' then
 *
 *		The write queue must be empty.
 *
 *		No further write requests may be made.
 */

void
isc_socket_attach(isc_socket_t *sock, isc_socket_t **socketp);
/*%<
 * Attach *socketp to socket.
 *
 * Requires:
 *
 * \li	'socket' is a valid socket.
 *
 * \li	'socketp' points to a NULL socket.
 *
 * Ensures:
 *
 * \li	*socketp is attached to socket.
 */

void
isc_socket_detach(isc_socket_t **socketp);
/*%<
 * Detach *socketp from its socket.
 *
 * Requires:
 *
 * \li	'socketp' points to a valid socket.
 *
 * \li	If '*socketp' is the last reference to the socket,
 *	then:
 *
 *		There must be no pending I/O requests.
 *
 * Ensures:
 *
 * \li	*socketp is NULL.
 *
 * \li	If '*socketp' is the last reference to the socket,
 *	then:
 *
 *		The socket will be shutdown (both reading and writing)
 *		for all tasks.
 *
 *		All resources used by the socket have been freed
 */

isc_result_t
isc_socket_open(isc_socket_t *sock);
/*%<
 * Open a new socket file descriptor of the given socket structure.  It simply
 * opens a new descriptor; all of the other parameters including the socket
 * type are inherited from the existing socket.  This function is provided to
 * avoid overhead of destroying and creating sockets when many short-lived
 * sockets are frequently opened and closed.  When the efficiency is not an
 * issue, it should be safer to detach the unused socket and re-create a new
 * one.  This optimization may not be available for some systems, in which
 * case this function will return ISC_R_NOTIMPLEMENTED and must not be used.
 *
 * Requires:
 *
 * \li	there must be no other reference to this socket.
 *
 * \li	'socket' is a valid and previously closed by isc_socket_close()
 *
 * Returns:
 *	Same as isc_socket_create().
 * \li	ISC_R_NOTIMPLEMENTED
 */

isc_result_t
isc_socket_close(isc_socket_t *sock);
/*%<
 * Close a socket file descriptor of the given socket structure.  This function
 * is provided as an alternative to destroying an unused socket when overhead
 * destroying/re-creating sockets can be significant, and is expected to be
 * used with isc_socket_open().  This optimization may not be available for some
 * systems, in which case this function will return ISC_R_NOTIMPLEMENTED and
 * must not be used.
 *
 * Requires:
 *
 * \li	The socket must have a valid descriptor.
 *
 * \li	There must be no other reference to this socket.
 *
 * \li	There must be no pending I/O requests.
 *
 * Returns:
 * \li	#ISC_R_NOTIMPLEMENTED
 */

isc_result_t
isc_socket_bind(isc_socket_t *sock, const isc_sockaddr_t *addressp,
		isc_socket_options_t options);
/*%<
 * Bind 'socket' to '*addressp'.
 *
 * Requires:
 *
 * \li	'socket' is a valid socket
 *
 * \li	'addressp' points to a valid isc_sockaddr.
 *
 * Returns:
 *
 * \li	ISC_R_SUCCESS
 * \li	ISC_R_NOPERM
 * \li	ISC_R_ADDRNOTAVAIL
 * \li	ISC_R_ADDRINUSE
 * \li	ISC_R_BOUND
 * \li	ISC_R_UNEXPECTED
 */

isc_result_t
isc_socket_filter(isc_socket_t *sock, const char *filter);
/*%<
 * Inform the kernel that it should perform accept filtering.
 * If filter is NULL the current filter will be removed.:w
 */

isc_result_t
isc_socket_listen(isc_socket_t *sock, unsigned int backlog);
/*%<
 * Set listen mode on the socket.  After this call, the only function that
 * can be used (other than attach and detach) is isc_socket_accept().
 *
 * Notes:
 *
 * \li	'backlog' is as in the UNIX system call listen() and may be
 *	ignored by non-UNIX implementations.
 *
 * \li	If 'backlog' is zero, a reasonable system default is used, usually
 *	SOMAXCONN.
 *
 * Requires:
 *
 * \li	'socket' is a valid, bound TCP socket or a valid, bound UNIX socket.
 *
 * Returns:
 *
 * \li	ISC_R_SUCCESS
 * \li	ISC_R_UNEXPECTED
 */

isc_result_t
isc_socket_accept(isc_socket_t *sock,
		  isc_task_t *task, isc_taskaction_t action, void *arg);
/*%<
 * Queue accept event.  When a new connection is received, the task will
 * get an ISC_SOCKEVENT_NEWCONN event with the sender set to the listen
 * socket.  The new socket structure is sent inside the isc_socket_newconnev_t
 * event type, and is attached to the task 'task'.
 *
 * REQUIRES:
 * \li	'socket' is a valid TCP socket that isc_socket_listen() was called
 *	on.
 *
 * \li	'task' is a valid task
 *
 * \li	'action' is a valid action
 *
 * RETURNS:
 * \li	ISC_R_SUCCESS
 * \li	ISC_R_NOMEMORY
 * \li	ISC_R_UNEXPECTED
 */

isc_result_t
isc_socket_connect(isc_socket_t *sock, const isc_sockaddr_t *addressp,
		   isc_task_t *task, isc_taskaction_t action,
		   void *arg);
/*%<
 * Connect 'socket' to peer with address *saddr.  When the connection
 * succeeds, or when an error occurs, a CONNECT event with action 'action'
 * and arg 'arg' will be posted to the event queue for 'task'.
 *
 * Requires:
 *
 * \li	'socket' is a valid TCP socket
 *
 * \li	'addressp' points to a valid isc_sockaddr
 *
 * \li	'task' is a valid task
 *
 * \li	'action' is a valid action
 *
 * Returns:
 *
 * \li	ISC_R_SUCCESS
 * \li	ISC_R_NOMEMORY
 * \li	ISC_R_UNEXPECTED
 *
 * Posted event's result code:
 *
 * \li	ISC_R_SUCCESS
 * \li	ISC_R_TIMEDOUT
 * \li	ISC_R_CONNREFUSED
 * \li	ISC_R_NETUNREACH
 * \li	ISC_R_UNEXPECTED
 */

isc_result_t
isc_socket_getpeername(isc_socket_t *sock, isc_sockaddr_t *addressp);
/*%<
 * Get the name of the peer connected to 'socket'.
 *
 * Requires:
 *
 * \li	'socket' is a valid TCP socket.
 *
 * Returns:
 *
 * \li	ISC_R_SUCCESS
 * \li	ISC_R_TOOSMALL
 * \li	ISC_R_UNEXPECTED
 */

isc_result_t
isc_socket_getsockname(isc_socket_t *sock, isc_sockaddr_t *addressp);
/*%<
 * Get the name of 'socket'.
 *
 * Requires:
 *
 * \li	'socket' is a valid socket.
 *
 * Returns:
 *
 * \li	ISC_R_SUCCESS
 * \li	ISC_R_TOOSMALL
 * \li	ISC_R_UNEXPECTED
 */

/*@{*/
isc_result_t
isc_socket_recv(isc_socket_t *sock, isc_region_t *region,
		unsigned int minimum,
		isc_task_t *task, isc_taskaction_t action, void *arg);

isc_result_t
isc_socket_recv2(isc_socket_t *sock, isc_region_t *region,
		 unsigned int minimum, isc_task_t *task,
		 isc_socketevent_t *event, unsigned int flags);

/*!
 * Receive from 'socket', storing the results in region.
 *
 * Notes:
 *
 *\li	Let 'length' refer to the length of 'region' or to the sum of all
 *	available regions in the list of buffers '*buflist'.
 *
 *\li	If 'minimum' is non-zero and at least that many bytes are read,
 *	the completion event will be posted to the task 'task.'  If minimum
 *	is zero, the exact number of bytes requested in the region must
 * 	be read for an event to be posted.  This only makes sense for TCP
 *	connections, and is always set to 1 byte for UDP.
 *
 *\li	The read will complete when the desired number of bytes have been
 *	read, if end-of-input occurs, or if an error occurs.  A read done
 *	event with the given 'action' and 'arg' will be posted to the
 *	event queue of 'task'.
 *
 *\li	The caller may not modify 'region', the buffers which are passed
 *	into this function, or any data they refer to until the completion
 *	event is received.
 *
 *\li	For isc_socket_recv2():
 *	'event' is not NULL, and the non-socket specific fields are
 *	expected to be initialized.
 *
 *\li	For isc_socket_recv2():
 *	The only defined value for 'flags' is ISC_SOCKFLAG_IMMEDIATE.  If
 *	set and the operation completes, the return value will be
 *	ISC_R_SUCCESS and the event will be filled in and not sent.  If the
 *	operation does not complete, the return value will be
 *	ISC_R_INPROGRESS and the event will be sent when the operation
 *	completes.
 *
 * Requires:
 *
 *\li	'socket' is a valid, bound socket.
 *
 *\li	For isc_socket_recv():
 *	'region' is a valid region
 *
 *\li	For isc_socket_recvv():
 *	'buflist' is non-NULL, and '*buflist' contain at least one buffer.
 *
 *\li	'task' is a valid task
 *
 *\li	For isc_socket_recv() and isc_socket_recvv():
 *	action != NULL and is a valid action
 *
 *\li	For isc_socket_recv2():
 *	event != NULL
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_INPROGRESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_UNEXPECTED
 *
 * Event results:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_UNEXPECTED
 *\li	XXX needs other net-type errors
 */
/*@}*/

/*@{*/
isc_result_t
isc_socket_send(isc_socket_t *sock, isc_region_t *region,
		isc_task_t *task, isc_taskaction_t action, void *arg);
isc_result_t
isc_socket_sendto(isc_socket_t *sock, isc_region_t *region,
		  isc_task_t *task, isc_taskaction_t action, void *arg,
		  const isc_sockaddr_t *address, struct in6_pktinfo *pktinfo);
isc_result_t
isc_socket_sendto2(isc_socket_t *sock, isc_region_t *region,
		   isc_task_t *task,
		   const isc_sockaddr_t *address, struct in6_pktinfo *pktinfo,
		   isc_socketevent_t *event, unsigned int flags);

/*!
 * Send the contents of 'region' to the socket's peer.
 *
 * Notes:
 *
 *\li	Shutting down the requestor's task *may* result in any
 *	still pending writes being dropped or completed, depending on the
 *	underlying OS implementation.
 *
 *\li	If 'action' is NULL, then no completion event will be posted.
 *
 *\li	The caller may not modify 'region', the buffers which are passed
 *	into this function, or any data they refer to until the completion
 *	event is received.
 *
 *\li	For isc_socket_sendto2():
 *	'event' is not NULL, and the non-socket specific fields are
 *	expected to be initialized.
 *
 *\li	For isc_socket_sendto2():
 *	The only defined values for 'flags' are ISC_SOCKFLAG_IMMEDIATE
 *	and ISC_SOCKFLAG_NORETRY.
 *
 *\li	If ISC_SOCKFLAG_IMMEDIATE is set and the operation completes, the
 *	return value will be ISC_R_SUCCESS and the event will be filled
 *	in and not sent.  If the operation does not complete, the return
 *	value will be ISC_R_INPROGRESS and the event will be sent when
 *	the operation completes.
 *
 *\li	ISC_SOCKFLAG_NORETRY can only be set for UDP sockets.  If set
 *	and the send operation fails due to a transient error, the send
 *	will not be retried and the error will be indicated in the event.
 *	Using this option along with ISC_SOCKFLAG_IMMEDIATE allows the caller
 *	to specify a region that is allocated on the stack.
 *
 * Requires:
 *
 *\li	'socket' is a valid, bound socket.
 *
 *\li	For isc_socket_send():
 *	'region' is a valid region
 *
 *\li	For isc_socket_sendv() and isc_socket_sendtov():
 *	'buflist' is non-NULL, and '*buflist' contain at least one buffer.
 *
 *\li	'task' is a valid task
 *
 *\li	For isc_socket_sendv(), isc_socket_sendtov(), isc_socket_send(), and
 *	isc_socket_sendto():
 *	action == NULL or is a valid action
 *
 *\li	For isc_socket_sendto2():
 *	event != NULL
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_INPROGRESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_UNEXPECTED
 *
 * Event results:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_UNEXPECTED
 *\li	XXX needs other net-type errors
 */
/*@}*/

isc_result_t
isc_socketmgr_createinctx(isc_mem_t *mctx, isc_appctx_t *actx,
			  isc_socketmgr_t **managerp);

isc_result_t
isc_socketmgr_create(isc_mem_t *mctx, isc_socketmgr_t **managerp);

isc_result_t
isc_socketmgr_create2(isc_mem_t *mctx, isc_socketmgr_t **managerp,
		      unsigned int maxsocks, int nthreads);
/*%<
 * Create a socket manager.  If "maxsocks" is non-zero, it specifies the
 * maximum number of sockets that the created manager should handle.
 * isc_socketmgr_create() is equivalent of isc_socketmgr_create2() with
 * "maxsocks" being zero.
 * isc_socketmgr_createinctx() also associates the new manager with the
 * specified application context.
 *
 * Notes:
 *
 *\li	All memory will be allocated in memory context 'mctx'.
 *
 * Requires:
 *
 *\li	'mctx' is a valid memory context.
 *
 *\li	'managerp' points to a NULL isc_socketmgr_t.
 *
 *\li	'actx' is a valid application context (for createinctx()).
 *
 * Ensures:
 *
 *\li	'*managerp' is a valid isc_socketmgr_t.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_UNEXPECTED
 *\li	#ISC_R_NOTIMPLEMENTED
 */

isc_result_t
isc_socketmgr_getmaxsockets(isc_socketmgr_t *manager, unsigned int *nsockp);
/*%<
 * Returns in "*nsockp" the maximum number of sockets this manager may open.
 *
 * Requires:
 *
 *\li	'*manager' is a valid isc_socketmgr_t.
 *\li	'nsockp' is not NULL.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOTIMPLEMENTED
 */

void
isc_socketmgr_setstats(isc_socketmgr_t *manager, isc_stats_t *stats);
/*%<
 * Set a general socket statistics counter set 'stats' for 'manager'.
 *
 * Requires:
 * \li	'manager' is valid, hasn't opened any socket, and doesn't have
 *	stats already set.
 *
 *\li	stats is a valid statistics supporting socket statistics counters
 *	(see above).
 */

void
isc_socketmgr_destroy(isc_socketmgr_t **managerp);
/*%<
 * Destroy a socket manager.
 *
 * Notes:
 *
 *\li	This routine blocks until there are no sockets left in the manager,
 *	so if the caller holds any socket references using the manager, it
 *	must detach them before calling isc_socketmgr_destroy() or it will
 *	block forever.
 *
 * Requires:
 *
 *\li	'*managerp' is a valid isc_socketmgr_t.
 *
 *\li	All sockets managed by this manager are fully detached.
 *
 * Ensures:
 *
 *\li	*managerp == NULL
 *
 *\li	All resources used by the manager have been freed.
 */

isc_sockettype_t
isc_socket_gettype(isc_socket_t *sock);
/*%<
 * Returns the socket type for "sock."
 *
 * Requires:
 *
 *\li	"sock" is a valid socket.
 */

/*@{*/
void
isc_socket_ipv6only(isc_socket_t *sock, bool yes);
/*%<
 * If the socket is an IPv6 socket set/clear the IPV6_IPV6ONLY socket
 * option if the host OS supports this option.
 *
 * Requires:
 *\li	'sock' is a valid socket.
 */
/*@}*/

void
isc_socket_dscp(isc_socket_t *sock, isc_dscp_t dscp);
/*%<
 * Sets the Differentiated Services Code Point (DSCP) field for packets
 * transmitted on this socket.  If 'dscp' is -1, return immediately.
 *
 * Requires:
 *\li	'sock' is a valid socket.
 */

isc_socketevent_t *
isc_socket_socketevent(isc_mem_t *mctx, void *sender,
		       isc_eventtype_t eventtype, isc_taskaction_t action,
		       void *arg);
/*%<
 * Get a isc_socketevent_t to be used with isc_socket_sendto2(), etc.
 */

void
isc_socket_cleanunix(const isc_sockaddr_t *addr, bool active);

/*%<
 * Cleanup UNIX domain sockets in the file-system.  If 'active' is true
 * then just unlink the socket.  If 'active' is false try to determine
 * if there is a listener of the socket or not.  If no listener is found
 * then unlink socket.
 *
 * Prior to unlinking the path is tested to see if it a socket.
 *
 * Note: there are a number of race conditions which cannot be avoided
 *       both in the filesystem and any application using UNIX domain
 *	 sockets (e.g. socket is tested between bind() and listen(),
 *	 the socket is deleted and replaced in the file-system between
 *	 stat() and unlink()).
 */

isc_result_t
isc_socket_permunix(const isc_sockaddr_t *sockaddr, uint32_t perm,
		    uint32_t owner, uint32_t group);
/*%<
 * Set ownership and file permissions on the UNIX domain socket.
 *
 * Note: On Solaris this secures the directory containing
 *       the socket as Solaris do not honour the filesystem
 *	 permissions on the socket.
 *
 * Requires:
 * \li	'sockaddr' to be a valid UNIX domain sockaddr.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_FAILURE
 */

void isc_socket_setname(isc_socket_t *socket, const char *name, void *tag);
/*%<
 * Set the name and optional tag for a socket.  This allows tracking of the
 * owner or purpose for this socket, and is useful for tracing and statistics
 * reporting.
 */

const char *isc_socket_getname(isc_socket_t *socket);
/*%<
 * Get the name associated with a socket, if any.
 */

void *isc_socket_gettag(isc_socket_t *socket);
/*%<
 * Get the tag associated with a socket, if any.
 */

int isc_socket_getfd(isc_socket_t *socket);
/*%<
 * Get the file descriptor associated with a socket
 */

void
isc_socketmgr_setreserved(isc_socketmgr_t *mgr, uint32_t);
/*%<
 * Temporary.  For use by named only.
 */

void
isc_socketmgr_maxudp(isc_socketmgr_t *mgr, unsigned int maxudp);
/*%<
 * Test interface. Drop UDP packet > 'maxudp'.
 */

bool
isc_socket_hasreuseport(void);
/*%<
 * Return true if there is SO_REUSEPORT support
 */

#ifdef HAVE_LIBXML2
int
isc_socketmgr_renderxml(isc_socketmgr_t *mgr, xmlTextWriterPtr writer);
/*%<
 * Render internal statistics and other state into the XML document.
 */
#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON
isc_result_t
isc_socketmgr_renderjson(isc_socketmgr_t *mgr, json_object *stats);
/*%<
 * Render internal statistics and other state into JSON format.
 */
#endif /* HAVE_JSON */

/*%<
 * See isc_socketmgr_create() above.
 */
typedef isc_result_t
(*isc_socketmgrcreatefunc_t)(isc_mem_t *mctx, isc_socketmgr_t **managerp);
/*!
 * Flags for fdwatchcreate.
 */
#define ISC_SOCKFDWATCH_READ	0x00000001	/*%< watch for readable */
#define ISC_SOCKFDWATCH_WRITE	0x00000002	/*%< watch for writable */
/*@}*/

isc_result_t
isc_socket_fdwatchcreate(isc_socketmgr_t *manager,
			 int fd,
			 int flags,
			 isc_sockfdwatch_t callback,
			 void *cbarg,
			 isc_task_t *task,
			 isc_socket_t **socketp);
/*%<
 * Create a new file descriptor watch socket managed by 'manager'.
 *
 * Note:
 *
 *\li   'fd' is the already-opened file descriptor (must be less
 * 	than maxsockets).
 *\li	This function is not available on Windows.
 *\li	The callback function is called "in-line" - this means the function
 *	needs to return as fast as possible, as all other I/O will be suspended
 *	until the callback completes.
 *
 * Requires:
 *
 *\li	'manager' is a valid manager
 *
 *\li	'socketp' is a valid pointer, and *socketp == NULL
 *
 *\li	'fd' be opened.
 *
 * Ensures:
 *
 *	'*socketp' is attached to the newly created fdwatch socket
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_NORESOURCES
 *\li	#ISC_R_UNEXPECTED
 *\li	#ISC_R_RANGE
 */

isc_result_t
isc_socket_fdwatchpoke(isc_socket_t *sock,
		       int flags);
/*%<
 * Poke a file descriptor watch socket informing the manager that it
 * should restart watching the socket
 *
 * Note:
 *
 *\li   'sock' is the socket returned by isc_socket_fdwatchcreate
 *
 *\li   'flags' indicates what the manager should watch for on the socket
 *      in addition to what it may already be watching.  It can be one or
 *      both of ISC_SOCKFDWATCH_READ and ISC_SOCKFDWATCH_WRITE.  To
 *      temporarily disable watching on a socket the value indicating
 *      no more data should be returned from the call back routine.
 *
 *\li	This function is not available on Windows.
 *
 * Requires:
 *
 *\li	'sock' is a valid isc socket
 *
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 */

ISC_LANG_ENDDECLS

#endif /* ISC_SOCKET_H */
