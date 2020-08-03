/*	$NetBSD: netmgr-int.h,v 1.3 2020/08/03 17:23:42 christos Exp $	*/

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

#pragma once

#include <unistd.h>
#include <uv.h>

#include <isc/astack.h>
#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/condition.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/queue.h>
#include <isc/quota.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/stats.h>
#include <isc/thread.h>
#include <isc/util.h>

#include "uv-compat.h"

#define ISC_NETMGR_TID_UNKNOWN -1

#if !defined(WIN32)
/*
 * New versions of libuv support recvmmsg on unices.
 * Since recvbuf is only allocated per worker allocating a bigger one is not
 * that wasteful.
 * 20 here is UV__MMSG_MAXWIDTH taken from the current libuv source, nothing
 * will break if the original value changes.
 */
#define ISC_NETMGR_RECVBUF_SIZE (20 * 65536)
#else
#define ISC_NETMGR_RECVBUF_SIZE (65536)
#endif

/*
 * Single network event loop worker.
 */
typedef struct isc__networker {
	isc_nm_t *mgr;
	int id;		  /* thread id */
	uv_loop_t loop;	  /* libuv loop structure */
	uv_async_t async; /* async channel to send
			   * data to this networker */
	isc_mutex_t lock;
	isc_condition_t cond;
	bool paused;
	bool finished;
	isc_thread_t thread;
	isc_queue_t *ievents;	   /* incoming async events */
	isc_queue_t *ievents_prio; /* priority async events
				    * used for listening etc.
				    * can be processed while
				    * worker is paused */
	isc_refcount_t references;
	atomic_int_fast64_t pktcount;
	char *recvbuf;
	bool recvbuf_inuse;
} isc__networker_t;

/*
 * A general handle for a connection bound to a networker.  For UDP
 * connections we have peer address here, so both TCP and UDP can be
 * handled with a simple send-like function
 */
#define NMHANDLE_MAGIC	  ISC_MAGIC('N', 'M', 'H', 'D')
#define VALID_NMHANDLE(t) ISC_MAGIC_VALID(t, NMHANDLE_MAGIC)

typedef void (*isc__nm_closecb)(isc_nmhandle_t *);

struct isc_nmhandle {
	int magic;
	isc_refcount_t references;

	/*
	 * The socket is not 'attached' in the traditional
	 * reference-counting sense. Instead, we keep all handles in an
	 * array in the socket object.  This way, we don't have circular
	 * dependencies and we can close all handles when we're destroying
	 * the socket.
	 */
	isc_nmsocket_t *sock;
	size_t ah_pos; /* Position in the socket's
			* 'active handles' array */

	/*
	 * The handle is 'inflight' if netmgr is not currently processing
	 * it in any way - it might mean that e.g. a recursive resolution
	 * is happening. For an inflight handle we must wait for the
	 * calling code to finish before we can free it.
	 */
	atomic_bool inflight;

	isc_sockaddr_t peer;
	isc_sockaddr_t local;
	isc_nm_opaquecb_t doreset; /* reset extra callback, external */
	isc_nm_opaquecb_t dofree;  /* free extra callback, external */
	void *opaque;
	char extra[];
};

/*
 * An interface - an address we can listen on.
 */
struct isc_nmiface {
	isc_sockaddr_t addr;
};

typedef enum isc__netievent_type {
	netievent_udpsend,
	netievent_udprecv,
	netievent_udpstop,

	netievent_tcpconnect,
	netievent_tcpsend,
	netievent_tcprecv,
	netievent_tcpstartread,
	netievent_tcppauseread,
	netievent_tcpchildaccept,
	netievent_tcpaccept,
	netievent_tcpstop,
	netievent_tcpclose,

	netievent_tcpdnsclose,
	netievent_tcpdnssend,

	netievent_closecb,
	netievent_shutdown,
	netievent_stop,
	netievent_prio = 0xff, /* event type values higher than this
				* will be treated as high-priority
				* events, which can be processed
				* while the netmgr is paused.
				*/
	netievent_udplisten,
	netievent_tcplisten,
} isc__netievent_type;

/*
 * We have to split it because we can read and write on a socket
 * simultaneously.
 */
typedef union {
	isc_nm_recv_cb_t recv;
	isc_nm_accept_cb_t accept;
} isc__nm_readcb_t;

typedef union {
	isc_nm_cb_t send;
	isc_nm_cb_t connect;
} isc__nm_writecb_t;

typedef union {
	isc_nm_recv_cb_t recv;
	isc_nm_accept_cb_t accept;
	isc_nm_cb_t send;
	isc_nm_cb_t connect;
} isc__nm_cb_t;

/*
 * Wrapper around uv_req_t with 'our' fields in it.  req->data should
 * always point to its parent.  Note that we always allocate more than
 * sizeof(struct) because we make room for different req types;
 */
#define UVREQ_MAGIC    ISC_MAGIC('N', 'M', 'U', 'R')
#define VALID_UVREQ(t) ISC_MAGIC_VALID(t, UVREQ_MAGIC)

typedef struct isc__nm_uvreq {
	int magic;
	isc_nmsocket_t *sock;
	isc_nmhandle_t *handle;
	uv_buf_t uvbuf;	      /* translated isc_region_t, to be
			       * sent or received */
	isc_sockaddr_t local; /* local address */
	isc_sockaddr_t peer;  /* peer address */
	isc__nm_cb_t cb;      /* callback */
	void *cbarg;	      /* callback argument */
	uv_pipe_t ipc;	      /* used for sending socket
			       * uv_handles to other threads */
	union {
		uv_req_t req;
		uv_getaddrinfo_t getaddrinfo;
		uv_getnameinfo_t getnameinfo;
		uv_shutdown_t shutdown;
		uv_write_t write;
		uv_connect_t connect;
		uv_udp_send_t udp_send;
		uv_fs_t fs;
		uv_work_t work;
	} uv_req;
} isc__nm_uvreq_t;

typedef struct isc__netievent__socket {
	isc__netievent_type type;
	isc_nmsocket_t *sock;
} isc__netievent__socket_t;

typedef isc__netievent__socket_t isc__netievent_udplisten_t;
typedef isc__netievent__socket_t isc__netievent_udpstop_t;
typedef isc__netievent__socket_t isc__netievent_tcpstop_t;
typedef isc__netievent__socket_t isc__netievent_tcpclose_t;
typedef isc__netievent__socket_t isc__netievent_tcpdnsclose_t;
typedef isc__netievent__socket_t isc__netievent_startread_t;
typedef isc__netievent__socket_t isc__netievent_pauseread_t;
typedef isc__netievent__socket_t isc__netievent_closecb_t;

typedef struct isc__netievent__socket_req {
	isc__netievent_type type;
	isc_nmsocket_t *sock;
	isc__nm_uvreq_t *req;
} isc__netievent__socket_req_t;

typedef isc__netievent__socket_req_t isc__netievent_tcpconnect_t;
typedef isc__netievent__socket_req_t isc__netievent_tcplisten_t;
typedef isc__netievent__socket_req_t isc__netievent_tcpsend_t;
typedef isc__netievent__socket_req_t isc__netievent_tcpdnssend_t;

typedef struct isc__netievent__socket_streaminfo_quota {
	isc__netievent_type type;
	isc_nmsocket_t *sock;
	isc_uv_stream_info_t streaminfo;
	isc_quota_t *quota;
} isc__netievent__socket_streaminfo_quota_t;

typedef isc__netievent__socket_streaminfo_quota_t
	isc__netievent_tcpchildaccept_t;

typedef struct isc__netievent__socket_handle {
	isc__netievent_type type;
	isc_nmsocket_t *sock;
	isc_nmhandle_t *handle;
} isc__netievent__socket_handle_t;

typedef struct isc__netievent__socket_quota {
	isc__netievent_type type;
	isc_nmsocket_t *sock;
	isc_quota_t *quota;
} isc__netievent__socket_quota_t;

typedef isc__netievent__socket_quota_t isc__netievent_tcpaccept_t;

typedef struct isc__netievent_udpsend {
	isc__netievent_type type;
	isc_nmsocket_t *sock;
	isc_sockaddr_t peer;
	isc__nm_uvreq_t *req;
} isc__netievent_udpsend_t;

typedef struct isc__netievent {
	isc__netievent_type type;
} isc__netievent_t;

typedef isc__netievent_t isc__netievent_shutdown_t;
typedef isc__netievent_t isc__netievent_stop_t;

typedef union {
	isc__netievent_t ni;
	isc__netievent__socket_t nis;
	isc__netievent__socket_req_t nisr;
	isc__netievent_udpsend_t nius;
	isc__netievent__socket_quota_t nisq;
	isc__netievent__socket_streaminfo_quota_t nissq;
} isc__netievent_storage_t;

/*
 * Network manager
 */
#define NM_MAGIC    ISC_MAGIC('N', 'E', 'T', 'M')
#define VALID_NM(t) ISC_MAGIC_VALID(t, NM_MAGIC)

struct isc_nm {
	int magic;
	isc_refcount_t references;
	isc_mem_t *mctx;
	uint32_t nworkers;
	isc_mutex_t lock;
	isc_condition_t wkstatecond;
	isc__networker_t *workers;

	isc_stats_t *stats;

	isc_mempool_t *reqpool;
	isc_mutex_t reqlock;

	isc_mempool_t *evpool;
	isc_mutex_t evlock;

	atomic_uint_fast32_t workers_running;
	atomic_uint_fast32_t workers_paused;
	atomic_uint_fast32_t maxudp;
	atomic_bool paused;

	/*
	 * Active connections are being closed and new connections are
	 * no longer allowed.
	 */
	atomic_bool closing;

	/*
	 * A worker is actively waiting for other workers, for example to
	 * stop listening; that means no other thread can do the same thing
	 * or pause, or we'll deadlock. We have to either re-enqueue our
	 * event or wait for the other one to finish if we want to pause.
	 */
	atomic_bool interlocked;

	/*
	 * Timeout values for TCP connections, corresponding to
	 * tcp-intiial-timeout, tcp-idle-timeout, tcp-keepalive-timeout,
	 * and tcp-advertised-timeout. Note that these are stored in
	 * milliseconds so they can be used directly with the libuv timer,
	 * but they are configured in tenths of seconds.
	 */
	uint32_t init;
	uint32_t idle;
	uint32_t keepalive;
	uint32_t advertised;
};

typedef enum isc_nmsocket_type {
	isc_nm_udpsocket,
	isc_nm_udplistener, /* Aggregate of nm_udpsocks */
	isc_nm_tcpsocket,
	isc_nm_tcplistener,
	isc_nm_tcpdnslistener,
	isc_nm_tcpdnssocket
} isc_nmsocket_type;

/*%
 * A universal structure for either a single socket or a group of
 * dup'd/SO_REUSE_PORT-using sockets listening on the same interface.
 */
#define NMSOCK_MAGIC	ISC_MAGIC('N', 'M', 'S', 'K')
#define VALID_NMSOCK(t) ISC_MAGIC_VALID(t, NMSOCK_MAGIC)

/*%
 * Index into socket stat counter arrays.
 */
enum { STATID_OPEN = 0,
       STATID_OPENFAIL = 1,
       STATID_CLOSE = 2,
       STATID_BINDFAIL = 3,
       STATID_CONNECTFAIL = 4,
       STATID_CONNECT = 5,
       STATID_ACCEPTFAIL = 6,
       STATID_ACCEPT = 7,
       STATID_SENDFAIL = 8,
       STATID_RECVFAIL = 9,
       STATID_ACTIVE = 10 };

struct isc_nmsocket {
	/*% Unlocked, RO */
	int magic;
	int tid;
	isc_nmsocket_type type;
	isc_nm_t *mgr;
	/*% Parent socket for multithreaded listeners */
	isc_nmsocket_t *parent;
	/*% Listener socket this connection was accepted on */
	isc_nmsocket_t *listener;

	/*%
	 * quota is the TCP client, attached when a TCP connection
	 * is established. pquota is a non-attached pointer to the
	 * TCP client quota, stored in listening sockets but only
	 * attached in connected sockets.
	 */
	isc_quota_t *quota;
	isc_quota_t *pquota;
	isc_quota_cb_t quotacb;

	/*%
	 * Socket statistics
	 */
	const isc_statscounter_t *statsindex;

	/*%
	 * TCP read timeout timer.
	 */
	uv_timer_t timer;
	bool timer_initialized;
	uint64_t read_timeout;

	/*% outer socket is for 'wrapped' sockets - e.g. tcpdns in tcp */
	isc_nmsocket_t *outer;

	/*% server socket for connections */
	isc_nmsocket_t *server;

	/*% Child sockets for multi-socket setups */
	isc_nmsocket_t *children;
	int nchildren;
	isc_nmiface_t *iface;
	isc_nmhandle_t *tcphandle;

	/*% Extra data allocated at the end of each isc_nmhandle_t */
	size_t extrahandlesize;

	/*% TCP backlog */
	int backlog;

	/*% libuv data */
	uv_os_sock_t fd;
	union uv_any_handle uv_handle;

	/*% Peer address */
	isc_sockaddr_t peer;

	/* Atomic */
	/*% Number of running (e.g. listening) child sockets */
	atomic_int_fast32_t rchildren;

	/*%
	 * Socket is active if it's listening, working, etc. If it's
	 * closing, then it doesn't make a sense, for example, to
	 * push handles or reqs for reuse.
	 */
	atomic_bool active;
	atomic_bool destroying;

	/*%
	 * Socket is closed if it's not active and all the possible
	 * callbacks were fired, there are no active handles, etc.
	 * If active==false but closed==false, that means the socket
	 * is closing.
	 */
	atomic_bool closed;
	atomic_bool listening;
	atomic_bool listen_error;
	isc_refcount_t references;

	/*%
	 * TCPDNS socket has been set not to pipeliine.
	 */
	atomic_bool sequential;

	/*%
	 * TCPDNS socket has exceeded the maximum number of
	 * simultaneous requests per connection, so will be temporarily
	 * restricted from pipelining.
	 */
	atomic_bool overlimit;

	/*%
	 * TCPDNS socket in sequential mode is currently processing a packet,
	 * we need to wait until it finishes.
	 */
	atomic_bool processing;

	/*%
	 * A TCP socket has had isc_nm_pauseread() called.
	 */
	atomic_bool readpaused;

	/*%
	 * A TCP or TCPDNS socket has been set to use the keepalive
	 * timeout instead of the default idle timeout.
	 */
	atomic_bool keepalive;

	/*%
	 * 'spare' handles for that can be reused to avoid allocations,
	 * for UDP.
	 */
	isc_astack_t *inactivehandles;
	isc_astack_t *inactivereqs;

	/*%
	 * Used to wait for TCP listening events to complete, and
	 * for the number of running children to reach zero during
	 * shutdown.
	 */
	isc_mutex_t lock;
	isc_condition_t cond;

	/*%
	 * Used to pass a result back from TCP listening events.
	 */
	isc_result_t result;

	/*%
	 * List of active handles.
	 * ah - current position in 'ah_frees'; this represents the
	 *	current number of active handles;
	 * ah_size - size of the 'ah_frees' and 'ah_handles' arrays
	 * ah_handles - array pointers to active handles
	 *
	 * Adding a handle
	 *  - if ah == ah_size, reallocate
	 *  - x = ah_frees[ah]
	 *  - ah_frees[ah++] = 0;
	 *  - ah_handles[x] = handle
	 *  - x must be stored with the handle!
	 * Removing a handle:
	 *  - ah_frees[--ah] = x
	 *  - ah_handles[x] = NULL;
	 *
	 * XXX: for now this is locked with socket->lock, but we
	 * might want to change it to something lockless in the
	 * future.
	 */
	atomic_int_fast32_t ah;
	size_t ah_size;
	size_t *ah_frees;
	isc_nmhandle_t **ah_handles;

	/*% Buffer for TCPDNS processing */
	size_t buf_size;
	size_t buf_len;
	unsigned char *buf;

	/*%
	 * This function will be called with handle->sock
	 * as the argument whenever a handle's references drop
	 * to zero, after its reset callback has been called.
	 */
	isc_nm_opaquecb_t closehandle_cb;

	isc__nm_readcb_t rcb;
	void *rcbarg;

	isc__nm_cb_t accept_cb;
	void *accept_cbarg;
};

bool
isc__nm_in_netthread(void);
/*%
 * Returns 'true' if we're in the network thread.
 */

void *
isc__nm_get_ievent(isc_nm_t *mgr, isc__netievent_type type);
/*%<
 * Allocate an ievent and set the type.
 */
void
isc__nm_put_ievent(isc_nm_t *mgr, void *ievent);

void
isc__nm_enqueue_ievent(isc__networker_t *worker, isc__netievent_t *event);
/*%<
 * Enqueue an ievent onto a specific worker queue. (This the only safe
 * way to use an isc__networker_t from another thread.)
 */

void
isc__nm_alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf);
/*%<
 * Allocator for recv operations.
 *
 * Note that as currently implemented, this doesn't actually
 * allocate anything, it just assigns the the isc__networker's UDP
 * receive buffer to a socket, and marks it as "in use".
 */

void
isc__nm_free_uvbuf(isc_nmsocket_t *sock, const uv_buf_t *buf);
/*%<
 * Free a buffer allocated for a receive operation.
 *
 * Note that as currently implemented, this doesn't actually
 * free anything, marks the isc__networker's UDP receive buffer
 * as "not in use".
 */

isc_nmhandle_t *
isc__nmhandle_get(isc_nmsocket_t *sock, isc_sockaddr_t *peer,
		  isc_sockaddr_t *local);
/*%<
 * Get a handle for the socket 'sock', allocating a new one
 * if there isn't one available in 'sock->inactivehandles'.
 *
 * If 'peer' is not NULL, set the handle's peer address to 'peer',
 * otherwise set it to 'sock->peer'.
 *
 * If 'local' is not NULL, set the handle's local address to 'local',
 * otherwise set it to 'sock->iface->addr'.
 */

isc__nm_uvreq_t *
isc__nm_uvreq_get(isc_nm_t *mgr, isc_nmsocket_t *sock);
/*%<
 * Get a UV request structure for the socket 'sock', allocating a
 * new one if there isn't one available in 'sock->inactivereqs'.
 */

void
isc__nm_uvreq_put(isc__nm_uvreq_t **req, isc_nmsocket_t *sock);
/*%<
 * Completes the use of a UV request structure, setting '*req' to NULL.
 *
 * The UV request is pushed onto the 'sock->inactivereqs' stack or,
 * if that doesn't work, freed.
 */

void
isc__nmsocket_init(isc_nmsocket_t *sock, isc_nm_t *mgr, isc_nmsocket_type type,
		   isc_nmiface_t *iface);
/*%<
 * Initialize socket 'sock', attach it to 'mgr', and set it to type 'type'
 * and its interface to 'iface'.
 */

void
isc__nmsocket_prep_destroy(isc_nmsocket_t *sock);
/*%<
 * Market 'sock' as inactive, close it if necessary, and destroy it
 * if there are no remaining references or active handles.
 */

bool
isc__nmsocket_active(isc_nmsocket_t *sock);
/*%<
 * Determine whether 'sock' is active by checking 'sock->active'
 * or, for child sockets, 'sock->parent->active'.
 */

void
isc__nm_async_closecb(isc__networker_t *worker, isc__netievent_t *ev0);
/*%<
 * Issue a 'handle closed' callback on the socket.
 */

void
isc__nm_async_shutdown(isc__networker_t *worker, isc__netievent_t *ev0);
/*%<
 * Walk through all uv handles, get the underlying sockets and issue
 * close on them.
 */

isc_result_t
isc__nm_udp_send(isc_nmhandle_t *handle, isc_region_t *region, isc_nm_cb_t cb,
		 void *cbarg);
/*%<
 * Back-end implementation of isc_nm_send() for UDP handles.
 */

void
isc__nm_udp_stoplistening(isc_nmsocket_t *sock);

void
isc__nm_async_udplisten(isc__networker_t *worker, isc__netievent_t *ev0);

void
isc__nm_async_udpstop(isc__networker_t *worker, isc__netievent_t *ev0);
void
isc__nm_async_udpsend(isc__networker_t *worker, isc__netievent_t *ev0);
/*%<
 * Callback handlers for asynchronous UDP events (listen, stoplisten, send).
 */

isc_result_t
isc__nm_tcp_send(isc_nmhandle_t *handle, isc_region_t *region, isc_nm_cb_t cb,
		 void *cbarg);
/*%<
 * Back-end implementation of isc_nm_send() for TCP handles.
 */

isc_result_t
isc__nm_tcp_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg);

void
isc__nm_tcp_close(isc_nmsocket_t *sock);
/*%<
 * Close a TCP socket.
 */
isc_result_t
isc__nm_tcp_pauseread(isc_nmsocket_t *sock);
/*%<
 * Pause reading on this socket, while still remembering the callback.
 */

isc_result_t
isc__nm_tcp_resumeread(isc_nmsocket_t *sock);
/*%<
 * Resume reading from socket.
 *
 */

void
isc__nm_tcp_shutdown(isc_nmsocket_t *sock);
/*%<
 * Called on shutdown to close and clean up a listening TCP socket.
 */

void
isc__nm_tcp_stoplistening(isc_nmsocket_t *sock);

void
isc__nm_async_tcpconnect(isc__networker_t *worker, isc__netievent_t *ev0);
void
isc__nm_async_tcplisten(isc__networker_t *worker, isc__netievent_t *ev0);
void
isc__nm_async_tcpaccept(isc__networker_t *worker, isc__netievent_t *ev0);
void
isc__nm_async_tcpchildaccept(isc__networker_t *worker, isc__netievent_t *ev0);
void
isc__nm_async_tcpstop(isc__networker_t *worker, isc__netievent_t *ev0);
void
isc__nm_async_tcpsend(isc__networker_t *worker, isc__netievent_t *ev0);
void
isc__nm_async_startread(isc__networker_t *worker, isc__netievent_t *ev0);
void
isc__nm_async_pauseread(isc__networker_t *worker, isc__netievent_t *ev0);
void
isc__nm_async_tcp_startread(isc__networker_t *worker, isc__netievent_t *ev0);
void
isc__nm_async_tcp_pauseread(isc__networker_t *worker, isc__netievent_t *ev0);
void
isc__nm_async_tcpclose(isc__networker_t *worker, isc__netievent_t *ev0);
/*%<
 * Callback handlers for asynchronous TCP events (connect, listen,
 * stoplisten, send, read, pause, close).
 */

isc_result_t
isc__nm_tcpdns_send(isc_nmhandle_t *handle, isc_region_t *region,
		    isc_nm_cb_t cb, void *cbarg);
/*%<
 * Back-end implementation of isc_nm_send() for TCPDNS handles.
 */

void
isc__nm_tcpdns_close(isc_nmsocket_t *sock);
/*%<
 * Close a TCPDNS socket.
 */

void
isc__nm_tcpdns_stoplistening(isc_nmsocket_t *sock);

void
isc__nm_async_tcpdnsclose(isc__networker_t *worker, isc__netievent_t *ev0);

void
isc__nm_async_tcpdnssend(isc__networker_t *worker, isc__netievent_t *ev0);

#define isc__nm_uverr2result(x) \
	isc___nm_uverr2result(x, true, __FILE__, __LINE__)
isc_result_t
isc___nm_uverr2result(int uverr, bool dolog, const char *file,
		      unsigned int line);
/*%<
 * Convert a libuv error value into an isc_result_t.  The
 * list of supported error values is not complete; new users
 * of this function should add any expected errors that are
 * not already there.
 */

bool
isc__nm_acquire_interlocked(isc_nm_t *mgr);
/*%<
 * Try to acquire interlocked state; return true if successful.
 */

void
isc__nm_drop_interlocked(isc_nm_t *mgr);
/*%<
 * Drop interlocked state; signal waiters.
 */

void
isc__nm_acquire_interlocked_force(isc_nm_t *mgr);
/*%<
 * Actively wait for interlocked state.
 */

void
isc__nm_incstats(isc_nm_t *mgr, isc_statscounter_t counterid);
/*%<
 * Increment socket-related statistics counters.
 */

void
isc__nm_decstats(isc_nm_t *mgr, isc_statscounter_t counterid);
/*%<
 * Decrement socket-related statistics counters.
 */
