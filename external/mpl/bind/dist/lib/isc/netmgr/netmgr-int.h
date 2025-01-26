/*	$NetBSD: netmgr-int.h,v 1.13 2025/01/26 16:25:43 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#pragma once

#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <isc/atomic.h>
#include <isc/barrier.h>
#include <isc/buffer.h>
#include <isc/condition.h>
#include <isc/dnsstream.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/proxy2.h>
#include <isc/quota.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/stats.h>
#include <isc/thread.h>
#include <isc/tid.h>
#include <isc/time.h>
#include <isc/tls.h>
#include <isc/util.h>
#include <isc/uv.h>

#include "../loop_p.h"

#define ISC_NETMGR_TID_UNKNOWN -1

/*
 * Receive buffers
 */
#if HAVE_DECL_UV_UDP_MMSG_CHUNK
/*
 * The value 20 here is UV__MMSG_MAXWIDTH taken from the current libuv source,
 * libuv will not receive more that 20 datagrams in a single recvmmsg call.
 */
#define ISC_NETMGR_UDP_RECVBUF_SIZE (20 * UINT16_MAX)
#else
/*
 * A single DNS message size
 */
#define ISC_NETMGR_UDP_RECVBUF_SIZE UINT16_MAX
#endif
#define ISC_NETMGR_UDP_SENDBUF_SIZE UINT16_MAX

/*
 * The TCP send and receive buffers can fit one maximum sized DNS message plus
 * its size, the receive buffer here affects TCP, DoT and DoH.
 */
#define ISC_NETMGR_TCP_SENDBUF_SIZE (sizeof(uint16_t) + UINT16_MAX)
#define ISC_NETMGR_TCP_RECVBUF_SIZE (sizeof(uint16_t) + UINT16_MAX)

/* Pick the larger buffer */
#define ISC_NETMGR_RECVBUF_SIZE                                     \
	(ISC_NETMGR_UDP_RECVBUF_SIZE >= ISC_NETMGR_TCP_RECVBUF_SIZE \
		 ? ISC_NETMGR_UDP_RECVBUF_SIZE                      \
		 : ISC_NETMGR_TCP_RECVBUF_SIZE)

/*
 * Make sure our RECVBUF size is large enough
 */

STATIC_ASSERT(ISC_NETMGR_UDP_RECVBUF_SIZE <= ISC_NETMGR_RECVBUF_SIZE,
	      "UDP receive buffer size must be smaller or equal than worker "
	      "receive buffer size");

STATIC_ASSERT(ISC_NETMGR_TCP_RECVBUF_SIZE <= ISC_NETMGR_RECVBUF_SIZE,
	      "TCP receive buffer size must be smaller or equal than worker "
	      "receive buffer size");

/*%
 * Maximum outstanding DNS message that we process in a single TCP read.
 */
#define ISC_NETMGR_MAX_STREAM_CLIENTS_PER_CONN 23

/*%
 * Regular TCP buffer size.
 */
#define NM_REG_BUF 4096

/*%
 * Larger buffer for when the regular one isn't enough; this will
 * hold two full DNS packets with lengths.  netmgr receives 64k at
 * most in TCPDNS or TLSDNS connections, so there's no risk of overrun
 * when using a buffer this size.
 */
#define NM_BIG_BUF ISC_NETMGR_TCP_RECVBUF_SIZE * 2

/*%
 * Maximum segment size (MSS) of TCP socket on which the server responds to
 * queries. Value lower than common MSS on Ethernet (1220, that is 1280 (IPv6
 * minimum link MTU) - 40 (IPv6 fixed header) - 20 (TCP fixed header)) will
 * address path MTU problem.
 */
#define NM_MAXSEG (1280 - 20 - 40)

/*%
 * How many isc_nmhandles and isc_nm_uvreqs will we be
 * caching for reuse in a socket.
 */
#define ISC_NM_NMSOCKET_MAX  64
#define ISC_NM_NMHANDLES_MAX 64
#define ISC_NM_UVREQS_MAX    64

/*% ISC_PROXY2_MIN_AF_UNIX_SIZE is the largest type when TLVs are not used */
#define ISC_NM_PROXY2_DEFAULT_BUFFER_SIZE (ISC_PROXY2_MIN_AF_UNIX_SIZE)

/*
 * Define ISC_NETMGR_TRACE to activate tracing of handles and sockets.
 * This will impair performance but enables us to quickly determine,
 * if netmgr resources haven't been cleaned up on shutdown, which ones
 * are still in use.
 */
#if ISC_NETMGR_TRACE
#define TRACE_SIZE 8

#if defined(__linux__)
#include <syscall.h>
#define gettid() (uint64_t)syscall(SYS_gettid)
#elif defined(__FreeBSD__)
#include <pthread_np.h>
#define gettid() (uint64_t)(pthread_getthreadid_np())
#elif defined(__OpenBSD__)
#include <unistd.h>
#define gettid() (uint64_t)(getthrid())
#elif defined(__NetBSD__)
#include <lwp.h>
#define gettid() (uint64_t)(_lwp_self())
#elif defined(__DragonFly__)
#include <unistd.h>
#define gettid() (uint64_t)(lwp_gettid())
#else
#define gettid() (uint64_t)(pthread_self())
#endif

#define NETMGR_TRACE_LOG(format, ...)                                \
	fprintf(stderr, "%" PRIu64 ":%d:%s:%u:%s:" format, gettid(), \
		isc_tid(), file, line, func, __VA_ARGS__)

#define FLARG                                                                 \
	, const char *func ISC_ATTR_UNUSED, const char *file ISC_ATTR_UNUSED, \
		unsigned int line ISC_ATTR_UNUSED

#define FLARG_PASS , func, file, line
#define isc__nm_uvreq_get(sock) \
	isc___nm_uvreq_get(sock, __func__, __FILE__, __LINE__)
#define isc__nm_uvreq_put(req) \
	isc___nm_uvreq_put(req, __func__, __FILE__, __LINE__)
#define isc__nmsocket_init(sock, mgr, type, iface, parent)            \
	isc___nmsocket_init(sock, mgr, type, iface, parent, __func__, \
			    __FILE__, __LINE__)
#define isc__nmsocket_put(sockp) \
	isc___nmsocket_put(sockp, __func__, __FILE__, __LINE__)
#define isc__nmsocket_attach(sock, target) \
	isc___nmsocket_attach(sock, target, __func__, __FILE__, __LINE__)
#define isc__nmsocket_detach(socketp) \
	isc___nmsocket_detach(socketp, __func__, __FILE__, __LINE__)
#define isc__nmsocket_close(socketp) \
	isc___nmsocket_close(socketp, __func__, __FILE__, __LINE__)
#define isc__nmhandle_get(sock, peer, local) \
	isc___nmhandle_get(sock, peer, local, __func__, __FILE__, __LINE__)
#define isc__nmsocket_prep_destroy(sock) \
	isc___nmsocket_prep_destroy(sock, __func__, __FILE__, __LINE__)
#define isc__nm_get_read_req(sock, sockaddr) \
	isc___nm_get_read_req(sock, sockaddr, __func__, __FILE__, __LINE__)
#else
#define NETMGR_TRACE_LOG(format, ...)

#define FLARG
#define FLARG_PASS
#define isc__nm_uvreq_get(sock) isc___nm_uvreq_get(sock)
#define isc__nm_uvreq_put(req)	isc___nm_uvreq_put(req)
#define isc__nmsocket_init(sock, mgr, type, iface, parent) \
	isc___nmsocket_init(sock, mgr, type, iface, parent)
#define isc__nmsocket_put(sockp)	   isc___nmsocket_put(sockp)
#define isc__nmsocket_attach(sock, target) isc___nmsocket_attach(sock, target)
#define isc__nmsocket_detach(socketp)	   isc___nmsocket_detach(socketp)
#define isc__nmsocket_close(socketp)	   isc___nmsocket_close(socketp)
#define isc__nmhandle_get(sock, peer, local) \
	isc___nmhandle_get(sock, peer, local)
#define isc__nmsocket_prep_destroy(sock) isc___nmsocket_prep_destroy(sock)
#define isc__nm_get_read_req(sock, sockaddr) \
	isc___nm_get_read_req(sock, sockaddr)
#endif

typedef struct isc__nm_uvreq isc__nm_uvreq_t;

/*
 * Single network event loop worker.
 */
typedef struct isc__networker {
	isc_mem_t *mctx;
	isc_refcount_t references;
	isc_loop_t *loop;
	isc_nm_t *netmgr;
	bool shuttingdown;

	char *recvbuf;
	bool recvbuf_inuse;

	ISC_LIST(isc_nmsocket_t) active_sockets;

	isc_mempool_t *nmsocket_pool;
	isc_mempool_t *uvreq_pool;
} isc__networker_t;

ISC_REFCOUNT_DECL(isc__networker);

#ifdef ISC_NETMGR_TRACE
void
isc__nm_dump_active(isc__networker_t *worker);

void
isc__nm_dump_active_manager(isc_nm_t *netmgr);
#endif /* ISC_NETMGR_TRACE */

/*
 * A general handle for a connection bound to a networker.  For UDP
 * connections we have peer address here, so both TCP and UDP can be
 * handled with a simple send-like function
 */
#define NMHANDLE_MAGIC ISC_MAGIC('N', 'M', 'H', 'D')
#define VALID_NMHANDLE(t)                      \
	(ISC_MAGIC_VALID(t, NMHANDLE_MAGIC) && \
	 atomic_load(&(t)->references) > 0)

typedef void (*isc__nm_closecb)(isc_nmhandle_t *);
typedef struct isc_nm_http_session isc_nm_http_session_t;

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

	isc_nm_http_session_t *httpsession;

	isc_sockaddr_t peer;
	isc_sockaddr_t local;
	bool proxy_is_unspec;
	struct isc_nmhandle *proxy_udphandle;
	isc_nm_opaquecb_t doreset; /* reset extra callback, external */
	isc_nm_opaquecb_t dofree;  /* free extra callback, external */
#if ISC_NETMGR_TRACE
	void *backtrace[TRACE_SIZE];
	int backtrace_size;
#endif
	LINK(isc_nmhandle_t) active_link;
	LINK(isc_nmhandle_t) inactive_link;

	void *opaque;

	isc_job_t job;
};

typedef union {
	isc_nm_recv_cb_t recv;
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

typedef struct isc__nm_uvreq isc__nm_uvreq_t;
struct isc__nm_uvreq {
	int magic;
	isc_nmsocket_t *sock;
	isc_nmhandle_t *handle;
	char tcplen[2];	       /* The TCP DNS message length */
	uv_buf_t uvbuf;	       /* translated isc_region_t, to be
				* sent or received */
	isc_sockaddr_t local;  /* local address */
	isc_sockaddr_t peer;   /* peer address */
	isc__nm_cb_t cb;       /* callback */
	void *cbarg;	       /* callback argument */
	isc_nm_timer_t *timer; /* TCP write timer */
	int connect_tries;     /* connect retries */
	isc_result_t result;

	union {
		uv_handle_t handle;
		uv_write_t write;
		uv_connect_t connect;
		uv_udp_send_t udp_send;
	} uv_req;
	ISC_LINK(isc__nm_uvreq_t) link;
	ISC_LINK(isc__nm_uvreq_t) active_link;

	isc_job_t job;
};

/*
 * Network manager
 */
#define NM_MAGIC    ISC_MAGIC('N', 'E', 'T', 'M')
#define VALID_NM(t) ISC_MAGIC_VALID(t, NM_MAGIC)

struct isc_nm {
	int magic;
	isc_refcount_t references;
	isc_mem_t *mctx;
	isc_loopmgr_t *loopmgr;
	uint32_t nloops;
	isc__networker_t *workers;

	isc_stats_t *stats;

	atomic_uint_fast32_t maxudp;

	bool load_balance_sockets;

	/*
	 * Active connections are being closed and new connections are
	 * no longer allowed.
	 */
	atomic_bool shuttingdown;

	/*
	 * Timeout values for TCP connections, corresponding to
	 * tcp-intiial-timeout, tcp-idle-timeout, tcp-keepalive-timeout,
	 * and tcp-advertised-timeout. Note that these are stored in
	 * milliseconds so they can be used directly with the libuv timer,
	 * but they are configured in tenths of seconds.
	 */
	atomic_uint_fast32_t init;
	atomic_uint_fast32_t idle;
	atomic_uint_fast32_t keepalive;
	atomic_uint_fast32_t advertised;

	/*
	 * Socket SO_RCVBUF and SO_SNDBUF values
	 */
	atomic_int_fast32_t recv_udp_buffer_size;
	atomic_int_fast32_t send_udp_buffer_size;
	atomic_int_fast32_t recv_tcp_buffer_size;
	atomic_int_fast32_t send_tcp_buffer_size;
};

/*%
 * A universal structure for either a single socket or a group of
 * dup'd/SO_REUSE_PORT-using sockets listening on the same interface.
 */
#define NMSOCK_MAGIC	ISC_MAGIC('N', 'M', 'S', 'K')
#define VALID_NMSOCK(t) ISC_MAGIC_VALID(t, NMSOCK_MAGIC)

/*%
 * Index into socket stat counter arrays.
 */
typedef enum {
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
	STATID_ACTIVE = 10,
	STATID_CLIENTS = 11,
	STATID_MAX = 12,
} isc__nm_statid_t;

typedef struct isc_nmsocket_tls_send_req {
	isc_nmsocket_t *tlssock;
	isc_buffer_t data;
	isc_nm_cb_t cb;
	void *cbarg;
	isc_nmhandle_t *handle;
	bool finish;
	uint8_t smallbuf[512];
} isc_nmsocket_tls_send_req_t;

#if HAVE_LIBNGHTTP2

typedef enum isc_http_request_type {
	ISC_HTTP_REQ_GET,
	ISC_HTTP_REQ_POST,
	ISC_HTTP_REQ_UNSUPPORTED
} isc_http_request_type_t;

typedef enum isc_http_scheme_type {
	ISC_HTTP_SCHEME_HTTP,
	ISC_HTTP_SCHEME_HTTP_SECURE,
	ISC_HTTP_SCHEME_UNSUPPORTED
} isc_http_scheme_type_t;

typedef struct isc_nm_httphandler {
	int magic;
	char *path;
	isc_nm_recv_cb_t cb;
	void *cbarg;
	LINK(struct isc_nm_httphandler) link;
} isc_nm_httphandler_t;

struct isc_nm_http_endpoints {
	uint32_t magic;
	isc_mem_t *mctx;

	ISC_LIST(isc_nm_httphandler_t) handlers;

	isc_refcount_t references;
	atomic_bool in_use;
};

typedef struct isc_nmsocket_h2 {
	isc_nmsocket_t *psock; /* owner of the structure */
	char *request_path;
	char *query_data;
	size_t query_data_len;
	bool query_too_large;

	isc_buffer_t rbuf;
	isc_buffer_t wbuf;

	int32_t stream_id;
	isc_nm_http_session_t *session;

	/* maximum concurrent streams (server-side) */
	atomic_uint_fast32_t max_concurrent_streams;

	uint32_t min_ttl; /* used to set "max-age" in responses */

	isc_http_request_type_t request_type;
	isc_http_scheme_type_t request_scheme;

	size_t content_length;
	char clenbuf[128];

	char cache_control_buf[128];

	int headers_error_code;
	size_t headers_data_processed;

	isc_nm_recv_cb_t cb;
	void *cbarg;
	LINK(struct isc_nmsocket_h2) link;

	isc_nm_http_endpoints_t **listener_endpoints;
	size_t n_listener_endpoints;

	isc_nm_http_endpoints_t *peer_endpoints;

	bool response_submitted;
	struct {
		char *uri;
		bool post;
		isc_tlsctx_t *tlsctx;
		isc_sockaddr_t local_interface;
		void *cstream;
		const char *tls_peer_verify_string;
	} connect;
} isc_nmsocket_h2_t;
#endif /* HAVE_LIBNGHTTP2 */

typedef void (*isc_nm_closehandlecb_t)(void *arg);
/*%<
 * Opaque callback function, used for isc_nmhandle 'reset' and 'free'
 * callbacks.
 */

struct isc_nmsocket {
	/*% Unlocked, RO */
	int magic;
	uint32_t tid;
	isc_refcount_t references;
	isc_nmsocket_type type;
	isc__networker_t *worker;

	isc_barrier_t listen_barrier;
	isc_barrier_t stop_barrier;

	/*% Parent socket for multithreaded listeners */
	isc_nmsocket_t *parent;

	/*% TLS stuff */
	struct tlsstream {
		bool server;
		BIO *bio_in;
		BIO *bio_out;
		isc_tls_t *tls;
		isc_tlsctx_t *ctx;
		isc_tlsctx_t **listener_tls_ctx; /*%< A context reference per
						    worker */
		size_t n_listener_tls_ctx;
		isc_tlsctx_client_session_cache_t *client_sess_cache;
		bool client_session_saved;
		isc_nmsocket_t *tlslistener;
		isc_nmsocket_t *tlssocket;
		atomic_bool result_updated;
		enum {
			TLS_INIT,
			TLS_HANDSHAKE,
			TLS_IO,
			TLS_CLOSED
		} state; /*%< The order of these is significant */
		size_t nsending;
		bool tcp_nodelay_value;
		isc_nmsocket_tls_send_req_t *send_req; /*%< Send req to reuse */
		bool reading;
	} tlsstream;

#if HAVE_LIBNGHTTP2
	isc_nmsocket_h2_t *h2;
#endif /* HAVE_LIBNGHTTP2 */

	struct {
		isc_dnsstream_assembler_t *input;
		bool reading;
		isc_nmsocket_t *listener;
		isc_nmsocket_t *sock;
		size_t nsending;
		void *send_req;
		bool dot_alpn_negotiated;
		const char *tls_verify_error;
	} streamdns;

	struct {
		isc_nmsocket_t *sock;
		bool reading;
		size_t nsending;
		void *send_req;
		union {
			isc_proxy2_handler_t *handler; /* server */
			isc_buffer_t *outbuf;	       /* client */
		} proxy2;
		bool header_processed;
		bool extra_processed; /* data arrived past header processed */
		isc_nmsocket_t **udp_server_socks; /* UDP sockets */
		size_t udp_server_socks_num;
	} proxy;

	/*%
	 * pquota is a non-attached pointer to the TCP client quota, stored in
	 * listening sockets.
	 */
	isc_quota_t *pquota;
	isc_job_t quotacb;

	/*%
	 * Socket statistics
	 */
	const isc_statscounter_t *statsindex;

	/*%
	 * TCP read/connect timeout timers.
	 */
	uv_timer_t read_timer;
	uint64_t read_timeout;
	uint64_t connect_timeout;

	/*%
	 * TCP write timeout timer.
	 */
	uint64_t write_timeout;

	/*
	 * Reading was throttled over TCP as the peer does not read the
	 * data we are sending back.
	 */
	bool reading_throttled;

	/*% outer socket is for 'wrapped' sockets - e.g. tcpdns in tcp */
	isc_nmsocket_t *outer;

	/*% server socket for connections */
	isc_nmsocket_t *server;

	/*% client socket for connections */
	isc_nmsocket_t *listener;

	/*% Child sockets for multi-socket setups */
	isc_nmsocket_t *children;
	uint_fast32_t nchildren;
	isc_sockaddr_t iface;
	isc_nmhandle_t *statichandle;
	isc_nmhandle_t *outerhandle;

	/*% TCP backlog */
	int backlog;

	/*% libuv data */
	uv_os_sock_t fd;
	union uv_any_handle uv_handle;

	/*% Peer address */
	isc_sockaddr_t peer;

	/*%
	 * Socket is active if it's listening, working, etc. If it's
	 * closing, then it doesn't make a sense, for example, to
	 * push handles or reqs for reuse.
	 */
	bool active;
	bool destroying;

	bool route_sock;

	/*%
	 * Socket is closed if it's not active and all the possible
	 * callbacks were fired, there are no active handles, etc.
	 * If active==false but closed==false, that means the socket
	 * is closing.
	 */
	bool closing;
	bool closed;
	bool connecting;
	bool connected;
	bool accepting;
	bool reading;
	bool timedout;

	/*%
	 * A timestamp of when the connection acceptance was delayed due
	 * to quota.
	 */
	isc_nanosecs_t quota_accept_ts;

	/*%
	 * Established an outgoing connection, as client not server.
	 */
	bool client;

	/*%
	 * The socket is processing read callback, this is guard to not read
	 * data before the readcb is back.
	 */
	bool processing;

	/*%
	 * A TCP or TCPDNS socket has been set to use the keepalive
	 * timeout instead of the default idle timeout.
	 */
	bool keepalive;

	/*%
	 * 'spare' handles for that can be reused to avoid allocations, for UDP.
	 */
	ISC_LIST(isc_nmhandle_t) inactive_handles;

	size_t inactive_handles_cur;
	size_t inactive_handles_max;

	/*%
	 * 'active' handles and uvreqs, mostly for debugging purposes.
	 */
	ISC_LIST(isc_nmhandle_t) active_handles;
	ISC_LIST(isc__nm_uvreq_t) active_uvreqs;

	size_t active_handles_cur;
	size_t active_handles_max;

	/*%
	 * Used to pass a result back from listen or connect events.
	 */
	isc_result_t result;

	/*%
	 * This function will be called with handle->sock
	 * as the argument whenever a handle's references drop
	 * to zero, after its reset callback has been called.
	 */
	isc_nm_closehandlecb_t closehandle_cb;

	isc_nmhandle_t *recv_handle;
	isc_nm_recv_cb_t recv_cb;
	void *recv_cbarg;

	isc_nm_cb_t connect_cb;
	void *connect_cbarg;

	isc_nm_accept_cb_t accept_cb;
	void *accept_cbarg;

	bool barriers_initialised;
	bool manual_read_timer;
#if ISC_NETMGR_TRACE
	void *backtrace[TRACE_SIZE];
	int backtrace_size;
#endif
	LINK(isc_nmsocket_t) active_link;

	isc_job_t job;
};

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
isc___nmhandle_get(isc_nmsocket_t *sock, isc_sockaddr_t const *peer,
		   isc_sockaddr_t const *local FLARG);
/*%<
 * Get a handle for the socket 'sock', allocating a new one
 * if there isn't one available in 'sock->inactivehandles'.
 *
 * If 'peer' is not NULL, set the handle's peer address to 'peer',
 * otherwise set it to 'sock->peer'.
 *
 * If 'local' is not NULL, set the handle's local address to 'local',
 * otherwise set it to 'sock->iface->addr'.
 *
 * 'sock' will be attached to 'handle->sock'. The caller may need
 * to detach the socket afterward.
 */

isc__nm_uvreq_t *
isc___nm_uvreq_get(isc_nmsocket_t *sock FLARG);
/*%<
 * Get a UV request structure for the socket 'sock', allocating a
 * new one if there isn't one available in 'sock->inactivereqs'.
 */

void
isc___nm_uvreq_put(isc__nm_uvreq_t **req FLARG);
/*%<
 * Completes the use of a UV request structure, setting '*req' to NULL.
 *
 * The UV request is pushed onto the 'sock->inactivereqs' stack or,
 * if that doesn't work, freed.
 */

void
isc___nmsocket_init(isc_nmsocket_t *sock, isc__networker_t *worker,
		    isc_nmsocket_type type, isc_sockaddr_t *iface,
		    isc_nmsocket_t *parent FLARG);
/*%<
 * Initialize socket 'sock', attach it to 'mgr', and set it to type 'type'
 * and its interface to 'iface'.
 */

void
isc___nmsocket_attach(isc_nmsocket_t *sock, isc_nmsocket_t **target FLARG);
/*%<
 * Attach to a socket, increasing refcount
 */

void
isc___nmsocket_detach(isc_nmsocket_t **socketp FLARG);
/*%<
 * Detach from socket, decreasing refcount and possibly destroying the
 * socket if it's no longer referenced.
 */

void
isc___nmsocket_prep_destroy(isc_nmsocket_t *sock FLARG);
/*%<
 * Market 'sock' as inactive, close it if necessary, and destroy it
 * if there are no remaining references or active handles.
 */

void
isc__nmsocket_shutdown(isc_nmsocket_t *sock);
/*%<
 * Initiate the socket shutdown which actively calls the active
 * callbacks.
 */

void
isc__nmsocket_reset(isc_nmsocket_t *sock);
/*%<
 * Reset and close the socket.
 */

bool
isc__nmsocket_active(isc_nmsocket_t *sock);
/*%<
 * Determine whether 'sock' is active by checking 'sock->active'
 * or, for child sockets, 'sock->parent->active'.
 */

void
isc__nmsocket_clearcb(isc_nmsocket_t *sock);
/*%<
 * Clear the recv and accept callbacks in 'sock'.
 */

void
isc__nmsocket_timer_stop(isc_nmsocket_t *sock);
void
isc__nmsocket_timer_start(isc_nmsocket_t *sock);
void
isc__nmsocket_timer_restart(isc_nmsocket_t *sock);
bool
isc__nmsocket_timer_running(isc_nmsocket_t *sock);
/*%<
 * Start/stop/restart/check the timeout on the socket
 */

void
isc__nm_connectcb(isc_nmsocket_t *sock, isc__nm_uvreq_t *uvreq,
		  isc_result_t eresult, bool async);

void
isc__nm_readcb(isc_nmsocket_t *sock, isc__nm_uvreq_t *uvreq,
	       isc_result_t eresult, bool async);
/*%<
 * Issue a read callback on the socket, used to call the callback
 * on failed conditions when the event can't be scheduled on the uv loop.
 *
 */

void
isc__nm_sendcb(isc_nmsocket_t *sock, isc__nm_uvreq_t *uvreq,
	       isc_result_t eresult, bool async);
/*%<
 * Issue a write callback on the socket, used to call the callback
 * on failed conditions when the event can't be scheduled on the uv loop.
 */

void
isc__nm_udp_send(isc_nmhandle_t *handle, const isc_region_t *region,
		 isc_nm_cb_t cb, void *cbarg);
/*%<
 * Back-end implementation of isc_nm_send() for UDP handles.
 */

void
isc__nm_udp_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg);
/*
 * Back-end implementation of isc_nm_read() for UDP handles.
 */

void
isc__nm_udp_close(isc_nmsocket_t *sock);
/*%<
 * Close a UDP socket.
 */

void
isc__nm_udp_shutdown(isc_nmsocket_t *sock);
/*%<
 * Called during the shutdown process to close and clean up connected
 * sockets.
 */

void
isc__nm_udp_stoplistening(isc_nmsocket_t *sock);
/*%<
 * Stop listening on 'sock'.
 */

void
isc__nm_udp_settimeout(isc_nmhandle_t *handle, uint32_t timeout);
/*%<
 * Set or clear the recv timeout for the UDP socket associated with 'handle'.
 */

void
isc__nm_tcp_send(isc_nmhandle_t *handle, const isc_region_t *region,
		 isc_nm_cb_t cb, void *cbarg);
/*%<
 * Back-end implementation of isc_nm_send() for TCP handles.
 */

void
isc__nm_tcp_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg);
/*
 * Start reading on this handle.
 */

void
isc__nm_tcp_close(isc_nmsocket_t *sock);
/*%<
 * Close a TCP socket.
 */
void
isc__nm_tcp_read_stop(isc_nmhandle_t *handle);
/*%<
 * Stop reading on this handle.
 */

void
isc__nm_tcp_shutdown(isc_nmsocket_t *sock);
/*%<
 * Called during the shutdown process to close and clean up connected
 * sockets.
 */

void
isc__nm_tcp_stoplistening(isc_nmsocket_t *sock);
/*%<
 * Stop listening on 'sock'.
 */

void
isc__nm_tcp_settimeout(isc_nmhandle_t *handle, uint32_t timeout);
/*%<
 * Set the read timeout for the TCP socket associated with 'handle'.
 */

void
isc__nmhandle_tcp_set_manual_timer(isc_nmhandle_t *handle, const bool manual);

void
isc__nm_tcp_senddns(isc_nmhandle_t *handle, const isc_region_t *region,
		    isc_nm_cb_t cb, void *cbarg);
/*%<
 * The same as 'isc__nm_tcp_send()', but with data length sent
 * ahead of data (two bytes (16 bit) in big-endian format).
 */

void
isc__nm_tls_send(isc_nmhandle_t *handle, const isc_region_t *region,
		 isc_nm_cb_t cb, void *cbarg);

/*%<
 * Back-end implementation of isc_nm_send() for TLSDNS handles.
 */

void
isc__nm_tls_senddns(isc_nmhandle_t *handle, const isc_region_t *region,
		    isc_nm_cb_t cb, void *cbarg);
/*%<
 * The same as 'isc__nm_tls_send()', but with data length sent
 * ahead of data (two bytes (16 bit) in big-endian format).
 */

void
isc__nm_tls_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg);
/*%<
 * Start reading on the TLS handle.
 */

void
isc__nm_tls_close(isc_nmsocket_t *sock);
/*%<
 * Close a TLS socket.
 */

void
isc__nm_tls_read_stop(isc_nmhandle_t *handle);
/*%<
 * Stop reading on the TLS handle.
 */

void
isc__nm_tls_cleanup_data(isc_nmsocket_t *sock);

void
isc__nm_tls_stoplistening(isc_nmsocket_t *sock);

void
isc__nm_tls_settimeout(isc_nmhandle_t *handle, uint32_t timeout);
void
isc__nm_tls_cleartimeout(isc_nmhandle_t *handle);
/*%<
 * Set the read timeout and reset the timer for the socket
 * associated with 'handle', and the TCP socket it wraps
 * around.
 */

void
isc__nmsocket_tls_reset(isc_nmsocket_t *sock);

void
isc__nmhandle_tls_set_manual_timer(isc_nmhandle_t *handle, const bool manual);

const char *
isc__nm_tls_verify_tls_peer_result_string(const isc_nmhandle_t *handle);

void
isc__nmhandle_tls_keepalive(isc_nmhandle_t *handle, bool value);
/*%<
 * Set the keepalive value on the underlying TCP handle.
 */

void
isc__nm_async_tls_set_tlsctx(isc_nmsocket_t *listener, isc_tlsctx_t *tlsctx,
			     const int tid);

void
isc__nmhandle_tls_setwritetimeout(isc_nmhandle_t *handle,
				  uint64_t write_timeout);

bool
isc__nmsocket_tls_timer_running(isc_nmsocket_t *sock);

void
isc__nmsocket_tls_timer_restart(isc_nmsocket_t *sock);

void
isc__nmsocket_tls_timer_stop(isc_nmsocket_t *sock);

void
isc__nm_tls_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result,
			   bool async);

void
isc__nmhandle_tls_get_selected_alpn(isc_nmhandle_t *handle,
				    const unsigned char **alpn,
				    unsigned int *alpnlen);

isc_result_t
isc__nmhandle_tls_set_tcp_nodelay(isc_nmhandle_t *handle, const bool value);

#if HAVE_LIBNGHTTP2

void
isc__nm_http_stoplistening(isc_nmsocket_t *sock);

void
isc__nm_http_settimeout(isc_nmhandle_t *handle, uint32_t timeout);
void
isc__nm_http_cleartimeout(isc_nmhandle_t *handle);
/*%<
 * Set the read timeout and reset the timer for the socket
 * associated with 'handle', and the TLS/TCP socket it wraps
 * around.
 */

void
isc__nmhandle_http_keepalive(isc_nmhandle_t *handle, bool value);
/*%<
 * Set the keepalive value on the underlying session handle
 */

void
isc__nm_http_cleanup_data(isc_nmsocket_t *sock);

isc_result_t
isc__nm_http_request(isc_nmhandle_t *handle, isc_region_t *region,
		     isc_nm_recv_cb_t reply_cb, void *cbarg);

void
isc__nm_http_send(isc_nmhandle_t *handle, const isc_region_t *region,
		  isc_nm_cb_t cb, void *cbarg);

void
isc__nm_http_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg);

void
isc__nm_http_close(isc_nmsocket_t *sock);

void
isc__nm_http_bad_request(isc_nmhandle_t *handle);
/*%<
 * Respond to the request with 400 "Bad Request" status.
 *
 * Requires:
 * \li 'handle' is a valid HTTP netmgr handle object, referencing a server-side
 * socket
 */

bool
isc__nm_http_has_encryption(const isc_nmhandle_t *handle);

void
isc__nm_http_set_maxage(isc_nmhandle_t *handle, const uint32_t ttl);

const char *
isc__nm_http_verify_tls_peer_result_string(const isc_nmhandle_t *handle);

bool
isc__nm_parse_httpquery(const char *query_string, const char **start,
			size_t *len);

char *
isc__nm_base64url_to_base64(isc_mem_t *mem, const char *base64url,
			    const size_t base64url_len, size_t *res_len);

char *
isc__nm_base64_to_base64url(isc_mem_t *mem, const char *base64,
			    const size_t base64_len, size_t *res_len);

void
isc__nm_httpsession_attach(isc_nm_http_session_t *source,
			   isc_nm_http_session_t **targetp);
void
isc__nm_httpsession_detach(isc_nm_http_session_t **sessionp);

isc_nmhandle_t *
isc__nm_httpsession_handle(isc_nm_http_session_t *session);

void
isc__nm_http_set_tlsctx(isc_nmsocket_t *sock, isc_tlsctx_t *tlsctx);

void
isc__nm_http_set_max_streams(isc_nmsocket_t *listener,
			     const uint32_t max_concurrent_streams);

#endif

void
isc__nm_streamdns_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb,
		       void *cbarg);

void
isc__nm_streamdns_send(isc_nmhandle_t *handle, const isc_region_t *region,
		       isc_nm_cb_t cb, void *cbarg);

void
isc__nm_streamdns_close(isc_nmsocket_t *sock);

void
isc__nm_streamdns_stoplistening(isc_nmsocket_t *sock);

void
isc__nm_streamdns_cleanup_data(isc_nmsocket_t *sock);

void
isc__nmhandle_streamdns_cleartimeout(isc_nmhandle_t *handle);

void
isc__nmhandle_streamdns_settimeout(isc_nmhandle_t *handle, uint32_t timeout);

void
isc__nmhandle_streamdns_keepalive(isc_nmhandle_t *handle, bool value);

void
isc__nmhandle_streamdns_setwritetimeout(isc_nmhandle_t *handle,
					uint32_t timeout);

bool
isc__nm_streamdns_has_encryption(const isc_nmhandle_t *handle);

const char *
isc__nm_streamdns_verify_tls_peer_result_string(const isc_nmhandle_t *handle);

void
isc__nm_streamdns_set_tlsctx(isc_nmsocket_t *listener, isc_tlsctx_t *tlsctx);

isc_result_t
isc__nm_streamdns_xfr_checkperm(isc_nmsocket_t *sock);

void
isc__nmsocket_streamdns_reset(isc_nmsocket_t *sock);

bool
isc__nmsocket_streamdns_timer_running(isc_nmsocket_t *sock);

void
isc__nmsocket_streamdns_timer_stop(isc_nmsocket_t *sock);

void
isc__nmsocket_streamdns_timer_restart(isc_nmsocket_t *sock);

void
isc__nm_streamdns_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result,
				 bool async);

bool
isc__nm_valid_proxy_addresses(const isc_sockaddr_t *src,
			      const isc_sockaddr_t *dst);

void
isc__nm_proxystream_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result,
				   bool async);

void
isc__nm_proxystream_stoplistening(isc_nmsocket_t *sock);

void
isc__nm_proxystream_cleanup_data(isc_nmsocket_t *sock);

void
isc__nmhandle_proxystream_cleartimeout(isc_nmhandle_t *handle);

void
isc__nmhandle_proxystream_settimeout(isc_nmhandle_t *handle, uint32_t timeout);

void
isc__nmhandle_proxystream_keepalive(isc_nmhandle_t *handle, bool value);

void
isc__nmhandle_proxystream_setwritetimeout(isc_nmhandle_t *handle,
					  uint64_t write_timeout);

void
isc__nmsocket_proxystream_reset(isc_nmsocket_t *sock);

bool
isc__nmsocket_proxystream_timer_running(isc_nmsocket_t *sock);

void
isc__nmsocket_proxystream_timer_restart(isc_nmsocket_t *sock);

void
isc__nmsocket_proxystream_timer_stop(isc_nmsocket_t *sock);

void
isc__nmhandle_proxystream_set_manual_timer(isc_nmhandle_t *handle,
					   const bool manual);

isc_result_t
isc__nmhandle_proxystream_set_tcp_nodelay(isc_nmhandle_t *handle,
					  const bool value);

void
isc__nm_proxystream_read_stop(isc_nmhandle_t *handle);

void
isc__nm_proxystream_close(isc_nmsocket_t *sock);

void
isc__nm_proxystream_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb,
			 void *cbarg);

void
isc__nm_proxystream_send(isc_nmhandle_t *handle, isc_region_t *region,
			 isc_nm_cb_t cb, void *cbarg);

void
isc__nm_proxystream_senddns(isc_nmhandle_t *handle, isc_region_t *region,
			    isc_nm_cb_t cb, void *cbarg);

void
isc__nm_proxystream_set_tlsctx(isc_nmsocket_t *listener, isc_tlsctx_t *tlsctx);

bool
isc__nm_proxystream_has_encryption(const isc_nmhandle_t *handle);

const char *
isc__nm_proxystream_verify_tls_peer_result_string(const isc_nmhandle_t *handle);

void
isc__nmhandle_proxystream_get_selected_alpn(isc_nmhandle_t *handle,
					    const unsigned char **alpn,
					    unsigned int *alpnlen);

void
isc__nm_proxyudp_failed_read_cb(isc_nmsocket_t *sock, const isc_result_t result,
				const bool async);

void
isc__nm_proxyudp_stoplistening(isc_nmsocket_t *listener);

void
isc__nm_proxyudp_cleanup_data(isc_nmsocket_t *sock);

void
isc__nmhandle_proxyudp_cleartimeout(isc_nmhandle_t *handle);

void
isc__nmhandle_proxyudp_settimeout(isc_nmhandle_t *handle, uint32_t timeout);

void
isc__nmhandle_proxyudp_setwritetimeout(isc_nmhandle_t *handle,
				       uint64_t write_timeout);

bool
isc__nmsocket_proxyudp_timer_running(isc_nmsocket_t *sock);

void
isc__nmsocket_proxyudp_timer_restart(isc_nmsocket_t *sock);

void
isc__nmsocket_proxyudp_timer_stop(isc_nmsocket_t *sock);

void
isc__nm_proxyudp_close(isc_nmsocket_t *sock);

void
isc__nm_proxyudp_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg);

void
isc__nm_proxyudp_send(isc_nmhandle_t *handle, isc_region_t *region,
		      isc_nm_cb_t cb, void *cbarg);

void
isc__nm_incstats(isc_nmsocket_t *sock, isc__nm_statid_t id);
/*%<
 * Increment socket-related statistics counters.
 */

void
isc__nm_decstats(isc_nmsocket_t *sock, isc__nm_statid_t id);
/*%<
 * Decrement socket-related statistics counters.
 */

isc_result_t
isc__nm_socket(int domain, int type, int protocol, uv_os_sock_t *sockp);
/*%<
 * Platform independent socket() version
 */

void
isc__nm_closesocket(uv_os_sock_t sock);
/*%<
 * Platform independent closesocket() version
 */

isc_result_t
isc__nm_socket_reuse(uv_os_sock_t fd, int val);
/*%<
 * Set the SO_REUSEADDR or SO_REUSEPORT (or equivalent) socket option on the fd
 */

isc_result_t
isc__nm_socket_reuse_lb(uv_os_sock_t fd);
/*%<
 * Set the SO_REUSEPORT_LB (or equivalent) socket option on the fd
 */

isc_result_t
isc__nm_socket_disable_pmtud(uv_os_sock_t fd, sa_family_t sa_family);
/*%<
 * Disable the Path MTU Discovery, either by disabling IP(V6)_DONTFRAG socket
 * option, or setting the IP(V6)_MTU_DISCOVER socket option to IP_PMTUDISC_OMIT
 */

isc_result_t
isc__nm_socket_v6only(uv_os_sock_t fd, sa_family_t sa_family);
/*%<
 * Restrict the socket to sending and receiving IPv6 packets only
 */

isc_result_t
isc__nm_socket_connectiontimeout(uv_os_sock_t fd, int timeout_ms);
/*%<
 * Set the connection timeout in milliseconds, on non-Linux platforms,
 * the minimum value must be at least 1000 (1 second).
 */

isc_result_t
isc__nm_socket_tcp_nodelay(const uv_os_sock_t fd, bool value);
/*%<
 * Disables/Enables Nagle's algorithm on a TCP socket (sets TCP_NODELAY if
 * 'value' equals 'true' or vice versa).
 */

isc_result_t
isc__nm_socket_tcp_maxseg(uv_os_sock_t fd, int size);
/*%<
 * Set the TCP maximum segment size
 */

isc_result_t
isc__nm_socket_min_mtu(uv_os_sock_t fd, sa_family_t sa_family);
/*%<
 * Use minimum MTU on IPv6 sockets
 */

void
isc__nm_set_network_buffers(isc_nm_t *nm, uv_handle_t *handle);
/*%>
 * Sets the pre-configured network buffers size on the handle.
 */

void
isc__nmsocket_barrier_init(isc_nmsocket_t *listener);
/*%>
 * Initialise the socket synchronisation barrier according to the
 * number of children.
 */

void
isc__nmsocket_stop(isc_nmsocket_t *listener);
/*%>
 * Broadcast "stop" event for a listener socket across all workers and
 * wait its processing completion - then, stop and close the underlying
 * transport listener socket.
 *
 * The primitive is used in multi-layer transport listener sockets to
 * implement shutdown properly: after the broadcasted events has been
 * processed it is safe to destroy the shared data within the listener
 * socket (including shutting down the underlying transport listener
 * socket).
 */

void
isc__nm_udp_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result,
			   bool async);
void
isc__nm_tcp_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result,
			   bool async);

isc__nm_uvreq_t *
isc___nm_get_read_req(isc_nmsocket_t *sock, isc_sockaddr_t *sockaddr FLARG);

void
isc__nm_alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf);

void
isc__nm_udp_read_cb(uv_udp_t *handle, ssize_t nrecv, const uv_buf_t *buf,
		    const struct sockaddr *addr, unsigned int flags);
void
isc__nm_tcp_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

isc_result_t
isc__nm_start_reading(isc_nmsocket_t *sock);
void
isc__nm_stop_reading(isc_nmsocket_t *sock);
bool
isc__nmsocket_closing(isc_nmsocket_t *sock);
bool
isc__nm_closing(isc__networker_t *worker);

void
isc__nm_failed_send_cb(isc_nmsocket_t *sock, isc__nm_uvreq_t *req,
		       isc_result_t eresult, bool async);
void
isc__nm_failed_connect_cb(isc_nmsocket_t *sock, isc__nm_uvreq_t *req,
			  isc_result_t eresult, bool async);
void
isc__nm_failed_read_cb(isc_nmsocket_t *sock, isc_result_t result, bool async);

void
isc__nm_accept_connection_log(isc_nmsocket_t *sock, isc_result_t result,
			      bool can_log_quota);

/*
 * Timeout callbacks
 */
void
isc__nmsocket_connecttimeout_cb(uv_timer_t *timer);
void
isc__nmsocket_readtimeout_cb(uv_timer_t *timer);
void
isc__nmsocket_writetimeout_cb(void *data, isc_result_t eresult);

/*
 * Bind to the socket, but allow binding to IPv6 tentative addresses reported by
 * the route socket by setting IP_FREEBIND (or equivalent).
 */
int
isc__nm_udp_freebind(uv_udp_t *handle, const struct sockaddr *addr,
		     unsigned int flags);

int
isc__nm_tcp_freebind(uv_tcp_t *handle, const struct sockaddr *addr,
		     unsigned int flags);

void
isc__nmsocket_log_tls_session_reuse(isc_nmsocket_t *sock, isc_tls_t *tls);

/*
 * Logging helpers
 */
void
isc__netmgr_log(const isc_nm_t *netmgr, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(3, 4);
void
isc__nmsocket_log(const isc_nmsocket_t *sock, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(3, 4);
void
isc__nmhandle_log(const isc_nmhandle_t *handle, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(3, 4);

void
isc__nm_received_proxy_header_log(isc_nmhandle_t *handle,
				  const isc_proxy2_command_t cmd,
				  const int socktype,
				  const isc_sockaddr_t *restrict src_addr,
				  const isc_sockaddr_t *restrict dst_addr,
				  const isc_region_t *restrict tlvs);

void
isc__nmhandle_set_manual_timer(isc_nmhandle_t *handle, const bool manual);
/*
 * Set manual read timer control mode - so that it will not get reset
 * automatically on read nor get started when read is initiated.
 */

void
isc__nmhandle_get_selected_alpn(isc_nmhandle_t *handle,
				const unsigned char **alpn,
				unsigned int *alpnlen);
/*
 * Returns a non zero terminated ALPN identifier via 'alpn'. The
 * length of the identifier is returned via 'alpnlen'. If after the
 * call either 'alpn == NULL' or 'alpnlen == 0', then identifier was
 * not negotiated of the underlying protocol of the connection
 * represented via the given handle does not support ALPN.
 */

void
isc__nm_senddns(isc_nmhandle_t *handle, isc_region_t *region, isc_nm_cb_t cb,
		void *cbarg);
/*%<
 * The same as 'isc_nm_send()', but with data length sent
 * ahead of data (two bytes (16 bit) in big-endian format).
 */
