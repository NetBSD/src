/*	$NetBSD: lload.h,v 1.3 2025/09/05 21:16:24 christos Exp $	*/

/* lload.h - load balancer include file */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2024 The OpenLDAP Foundation.
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

#ifndef _LLOAD_H_
#define _LLOAD_H_

#include "ldap_defaults.h"

#include <stdio.h>
#include <ac/stdlib.h>

#include <sys/types.h>
#include <ac/syslog.h>
#include <ac/regex.h>
#include <ac/signal.h>
#include <ac/socket.h>
#include <ac/time.h>
#include <ac/param.h>

#include "ldap_avl.h"

#include "../servers/slapd/slap.h"
#include "../slapd/back-monitor/back-monitor.h"

#ifndef ldap_debug
#define ldap_debug slap_debug
#endif

#include "ldap_log.h"

#include <ldap.h>
#include <ldap_schema.h>

#include "lber_pvt.h"
#include "ldap_pvt.h"
#include "ldap_pvt_thread.h"
#include "ldap_queue.h"

#include <event2/event.h>

#ifdef HAVE_CYRUS_SASL
#ifdef HAVE_SASL_SASL_H
#include <sasl/sasl.h>
#else
#include <sasl.h>
#endif
#endif /* HAVE_CYRUS_SASL */

LDAP_BEGIN_DECL

#ifdef SERVICE_NAME
#undef SERVICE_NAME
#endif

#define SERVICE_NAME OPENLDAP_PACKAGE "-lloadd"

#define LLOAD_SB_MAX_INCOMING_CLIENT ( ( 1 << 24 ) - 1 )
#define LLOAD_SB_MAX_INCOMING_UPSTREAM ( ( 1 << 24 ) - 1 )

#define LLOAD_CONN_MAX_PDUS_PER_CYCLE_DEFAULT 10

#define BER_BV_OPTIONAL( bv ) ( BER_BVISNULL( bv ) ? NULL : ( bv ) )

#include <epoch.h>

#define checked_lock( mutex ) \
    if ( ldap_pvt_thread_mutex_lock( mutex ) != 0 ) assert(0)
#define checked_unlock( mutex ) \
    if ( ldap_pvt_thread_mutex_unlock( mutex ) != 0 ) assert(0)

#ifdef LDAP_THREAD_DEBUG
#define assert_locked( mutex ) \
    if ( ldap_pvt_thread_mutex_trylock( mutex ) == 0 ) assert(0)
#else
#define assert_locked( mutex ) ( (void)0 )
#endif

typedef struct LloadTier LloadTier;
typedef struct LloadBackend LloadBackend;
typedef struct LloadPendingConnection LloadPendingConnection;
typedef struct LloadConnection LloadConnection;
typedef struct LloadOperation LloadOperation;
typedef struct LloadChange LloadChange;
/* end of forward declarations */

typedef LDAP_STAILQ_HEAD(TierSt, LloadTier) lload_t_head;
typedef LDAP_CIRCLEQ_HEAD(BeSt, LloadBackend) lload_b_head;
typedef LDAP_CIRCLEQ_HEAD(ConnSt, LloadConnection) lload_c_head;

LDAP_SLAPD_V (lload_t_head) tiers;
LDAP_SLAPD_V (lload_c_head) clients;
LDAP_SLAPD_V (struct slap_bindconf) bindconf;
LDAP_SLAPD_V (struct berval) lloadd_identity;

/* Used to coordinate server (un)pause, shutdown */
LDAP_SLAPD_V (ldap_pvt_thread_mutex_t) lload_wait_mutex;
LDAP_SLAPD_V (ldap_pvt_thread_cond_t) lload_pause_cond;
LDAP_SLAPD_V (ldap_pvt_thread_cond_t) lload_wait_cond;

typedef int lload_cf_aux_table_parse_x( struct berval *val,
        void *bc,
        slap_cf_aux_table *tab0,
        const char *tabmsg,
        int unparse );

typedef struct LloadListener LloadListener;

enum lc_type {
    LLOAD_CHANGE_UNDEFINED = 0,
    LLOAD_CHANGE_MODIFY,
    LLOAD_CHANGE_ADD,
    LLOAD_CHANGE_DEL,
};

enum lc_object {
    LLOAD_UNDEFINED = 0,
    LLOAD_DAEMON,
    /*
    LLOAD_BINDCONF,
    */
    LLOAD_TIER,
    LLOAD_BACKEND,
};

enum lcf_daemon {
    LLOAD_DAEMON_MOD_THREADS = 1 << 0,
    LLOAD_DAEMON_MOD_FEATURES = 1 << 1,
    LLOAD_DAEMON_MOD_TLS = 1 << 2,
    LLOAD_DAEMON_MOD_LISTENER_ADD = 1 << 3,
    LLOAD_DAEMON_MOD_LISTENER_REPLACE = 1 << 4,
    LLOAD_DAEMON_MOD_BINDCONF = 1 << 5,
};

enum lcf_tier {
    LLOAD_TIER_MOD_TYPE = 1 << 0,
};

enum lcf_backend {
    LLOAD_BACKEND_MOD_OTHER = 1 << 0,
    LLOAD_BACKEND_MOD_CONNS = 1 << 1,
};

struct LloadChange {
    enum lc_type type;
    enum lc_object object;
    union {
        int generic;
        enum lcf_daemon daemon;
        enum lcf_tier tier;
        enum lcf_backend backend;
    } flags;
    void *target;
};

typedef enum {
#ifdef LDAP_API_FEATURE_VERIFY_CREDENTIALS
    LLOAD_FEATURE_VC = 1 << 0,
#endif /* LDAP_API_FEATURE_VERIFY_CREDENTIALS */
    LLOAD_FEATURE_PROXYAUTHZ = 1 << 1,
    LLOAD_FEATURE_PAUSE = 1 << 2,
} lload_features_t;

#define LLOAD_FEATURE_SUPPORTED_MASK ( \
    LLOAD_FEATURE_PROXYAUTHZ | \
    0 )

#ifdef BALANCER_MODULE
#define LLOAD_TLS_CTX ( lload_use_slap_tls_ctx ? slap_tls_ctx : lload_tls_ctx )
#else
#define LLOAD_TLS_CTX ( lload_tls_ctx )
#endif

enum lload_tls_type {
    LLOAD_CLEARTEXT = 0,
    LLOAD_LDAPS,
    LLOAD_STARTTLS_OPTIONAL,
    LLOAD_STARTTLS,
    LLOAD_TLS_ESTABLISHED,
};

struct LloadPendingConnection {
    LloadBackend *backend;

    struct event *event;
    ber_socket_t fd;

    LDAP_LIST_ENTRY(LloadPendingConnection) next;
};

typedef struct lload_counters_t {
    ldap_pvt_mp_t lc_ops_completed;
    ldap_pvt_mp_t lc_ops_received;
    ldap_pvt_mp_t lc_ops_forwarded;
    ldap_pvt_mp_t lc_ops_rejected;
    ldap_pvt_mp_t lc_ops_failed;
} lload_counters_t;

enum {
    LLOAD_STATS_OPS_BIND = 0,
    LLOAD_STATS_OPS_OTHER,
    LLOAD_STATS_OPS_LAST
};

typedef struct lload_global_stats_t {
    ldap_pvt_mp_t global_incoming;
    ldap_pvt_mp_t global_outgoing;
    lload_counters_t counters[LLOAD_STATS_OPS_LAST];
} lload_global_stats_t;

typedef LloadTier *(LloadTierInit)( void );
typedef int (LloadTierConfigCb)( LloadTier *tier, char *arg );
typedef int (LloadTierBackendConfigCb)( LloadTier *tier, LloadBackend *b, char *arg );
typedef int (LloadTierCb)( LloadTier *tier );
typedef int (LloadTierResetCb)( LloadTier *tier, int shutdown );
typedef int (LloadTierBackendCb)( LloadTier *tier, LloadBackend *b );
typedef void (LloadTierChange)( LloadTier *tier, LloadChange *change );
typedef int (LloadTierSelect)( LloadTier *tier,
        LloadOperation *op,
        LloadConnection **cp,
        int *res,
        char **message );

struct lload_tier_type {
    char *tier_name;

    struct berval tier_oc, tier_backend_oc;

    LloadTierInit *tier_init;
    LloadTierConfigCb *tier_config;
    LloadTierBackendConfigCb *tier_backend_config;
    LloadTierCb *tier_startup;
    LloadTierCb *tier_update;
    LloadTierResetCb *tier_reset;
    LloadTierCb *tier_destroy;

    LloadTierBackendCb *tier_add_backend;
    LloadTierBackendCb *tier_remove_backend;
    LloadTierChange *tier_change;

    LloadTierSelect *tier_select;
};

struct LloadTier {
    struct lload_tier_type t_type;
    ldap_pvt_thread_mutex_t t_mutex;

    lload_b_head t_backends;
    int t_nbackends;

    enum {
        LLOAD_TIER_EXCLUSIVE = 1 << 0, /* Reject if busy */
    } t_flags;

    struct berval t_name;
#ifdef BALANCER_MODULE
    monitor_subsys_t *t_monitor;
#endif /* BALANCER_MODULE */

    void *t_private;
    LDAP_STAILQ_ENTRY(LloadTier) t_next;
};

/* Can hold mutex when locking a linked connection */
struct LloadBackend {
    ldap_pvt_thread_mutex_t b_mutex;

    struct berval b_name, b_uri;
    int b_proto, b_port;
    enum lload_tls_type b_tls, b_tls_conf;
    char *b_host;

    int b_retry_timeout, b_failed;
    struct event *b_retry_event;
    struct timeval b_retry_tv;

    int b_numconns, b_numbindconns;
    int b_bindavail, b_active, b_opening;
    lload_c_head b_conns, b_bindconns, b_preparing;
    LDAP_LIST_HEAD(ConnectingSt, LloadPendingConnection) b_connecting;
    LloadConnection *b_last_conn, *b_last_bindconn;

    long b_max_pending, b_max_conn_pending;
    long b_n_ops_executing;

    lload_counters_t b_counters[LLOAD_STATS_OPS_LAST];

    LloadTier *b_tier;

    time_t b_last_update;
    uintptr_t b_fitness;
    int b_weight;

    uintptr_t b_operation_count;
    uintptr_t b_operation_time;

#ifdef BALANCER_MODULE
    monitor_subsys_t *b_monitor;
#endif /* BALANCER_MODULE */

    struct evdns_getaddrinfo_request *b_dns_req;
    void *b_cookie;

    LDAP_CIRCLEQ_ENTRY(LloadBackend) b_next;
};

typedef int (*LloadOperationHandler)( LloadConnection *client,
        LloadOperation *op,
        BerElement *ber );
typedef int (*RequestHandler)( LloadConnection *c, LloadOperation *op );
typedef struct lload_exop_handlers_t {
    struct berval oid;
    RequestHandler func;
} ExopHandler;

typedef int (*CONNECTION_PDU_CB)( LloadConnection *c );
typedef void (*CONNECTION_DESTROY_CB)( LloadConnection *c );

/* connection state (protected by c_mutex) */
enum sc_state {
    LLOAD_C_INVALID = 0, /* MUST BE ZERO (0) */
    LLOAD_C_READY,       /* ready */
    LLOAD_C_CLOSING,     /* closing */
    LLOAD_C_ACTIVE,      /* exclusive operation (tls setup, ...) in progress */
    LLOAD_C_BINDING,     /* binding */
    LLOAD_C_DYING,       /* part-processed dead waiting to be freed, someone
                          * might still be observing it */
};
enum sc_type {
    LLOAD_C_OPEN = 0,  /* regular connection */
    LLOAD_C_PREPARING, /* upstream connection not assigned yet */
    LLOAD_C_BIND, /* connection used to handle bind client requests if VC not enabled */
    LLOAD_C_PRIVILEGED, /* connection can override proxyauthz control */
};
enum sc_io_state {
    LLOAD_C_OPERATIONAL = 0,        /* all is good */
    LLOAD_C_READ_HANDOVER = 1 << 0, /* A task to process PDUs is scheduled or
                                     * running, do not re-enable c_read_event */
    LLOAD_C_READ_PAUSE = 1 << 1,    /* We want to pause reading until the client
                                     * has sufficiently caught up with what we
                                     * sent */
};

/* Tracking whether an operation might cause a client to restrict which
 * upstreams are eligible */
enum op_restriction {
    LLOAD_OP_NOT_RESTRICTED, /* no restrictions in place */
    LLOAD_OP_RESTRICTED_WRITE, /* client is restricted to a certain backend with
                                * a timeout attached */
    LLOAD_OP_RESTRICTED_BACKEND, /* client is restricted to a certain backend,
                                  * without a timeout */
    LLOAD_OP_RESTRICTED_UPSTREAM, /* client is restricted to a certain upstream */
    LLOAD_OP_RESTRICTED_ISOLATE, /* TODO: client is restricted to a certain
                                  * upstream and removes the upstream from the
                                  * pool */
    LLOAD_OP_RESTRICTED_REJECT, /* operation should not be forwarded to any
                                 * backend, either it is processed internally
                                 * or rejected */
};

/*
 * represents a connection from an ldap client/to ldap server
 */
struct LloadConnection {
    enum sc_state c_state; /* connection state */
    enum sc_type c_type;
    enum sc_io_state c_io_state;
    ber_socket_t c_fd;

/*
 * LloadConnection reference counting:
 * - connection has a reference counter in c_refcnt
 * - also a liveness/validity token is added to c_refcnt during
 *   lload_connection_init, its existence is tracked in c_live and is usually the
 *   only one that prevents it from being destroyed
 * - anyone who needs to be able to relock the connection after unlocking it has
 *   to use acquire_ref(), they need to make sure a matching
 *   RELEASE_REF( c, c_refcnt, c->c_destroy ); is run eventually
 * - when a connection is considered dead, use CONNECTION_DESTROY on a locked
 *   connection, it will be made unreachable from normal places and either
 *   scheduled for reclamation when safe to do so or if anyone still holds a
 *   reference, it just gets unlocked and reclaimed after the last ref is
 *   released
 * - CONNECTION_LOCK_DESTROY is a shorthand for locking and CONNECTION_DESTROY
 */
    ldap_pvt_thread_mutex_t c_mutex; /* protect the connection */
    uintptr_t c_refcnt, c_live;
    CONNECTION_DESTROY_CB c_unlink;
    CONNECTION_DESTROY_CB c_destroy;
    CONNECTION_PDU_CB c_pdu_cb;
#define CONNECTION_ASSERT_LOCKED(c) assert_locked( &(c)->c_mutex )
#define CONNECTION_LOCK(c) \
    do { \
        checked_lock( &(c)->c_mutex ); \
    } while (0)
#define CONNECTION_UNLOCK(c) \
    do { \
        checked_unlock( &(c)->c_mutex ); \
    } while (0)
#define CONNECTION_UNLINK_(c) \
    do { \
        if ( __atomic_exchange_n( &(c)->c_live, 0, __ATOMIC_ACQ_REL ) ) { \
            (c)->c_unlink( (c) ); \
            RELEASE_REF( (c), c_refcnt, c->c_destroy ); \
        } \
    } while (0)
#define CONNECTION_DESTROY(c) \
    do { \
        CONNECTION_UNLINK_(c); \
        CONNECTION_UNLOCK(c); \
    } while (0)
#define CONNECTION_LOCK_DESTROY(c) \
    do { \
        CONNECTION_LOCK(c); \
        CONNECTION_DESTROY(c); \
    } while (0);

    Sockbuf *c_sb; /* ber connection stuff */

    /* set by connection_init */
    unsigned long c_connid;    /* unique id of this connection */
    struct berval c_peer_name; /* peer name (trans=addr:port) */
    time_t c_starttime;        /* when the connection was opened */

    time_t c_activitytime;  /* when the connection was last used */
    ber_int_t c_next_msgid; /* msgid of the next message */

    /* must not be used while holding either mutex */
    struct event *c_read_event, *c_write_event;
    struct timeval *c_read_timeout;

    /* can only be changed by binding thread */
    struct berval c_sasl_bind_mech; /* mech in progress */
    struct berval c_auth;           /* authcDN (possibly in progress) */

    unsigned long c_pin_id;

#ifdef HAVE_CYRUS_SASL
    sasl_conn_t *c_sasl_authctx;
    void *c_sasl_defaults;
#ifdef SASL_CHANNEL_BINDING /* 2.1.25+ */
    sasl_channel_binding_t *c_sasl_cbinding; /* Else cyrus-sasl would happily
                                              * leak it on sasl_dispose */
#endif /* SASL_CHANNEL_BINDING */
#endif /* HAVE_CYRUS_SASL */

#ifdef LDAP_API_FEATURE_VERIFY_CREDENTIALS
    struct berval c_vc_cookie;
#endif /* LDAP_API_FEATURE_VERIFY_CREDENTIALS */

    /* Can be held while acquiring c_mutex to inject things into c_ops or
     * destroy the connection */
    ldap_pvt_thread_mutex_t c_io_mutex; /* only one pdu written at a time */

    BerElement *c_currentber; /* ber we're attempting to read */
    BerElement *c_pendingber; /* ber we're attempting to write */

    TAvlnode *c_ops; /* Operations pending on the connection */

#ifdef HAVE_TLS
    enum lload_tls_type c_is_tls; /* true if this LDAP over raw TLS */
#endif

    long c_n_ops_executing;      /* num of ops currently executing */
    long c_n_ops_completed;      /* num of ops completed */
    lload_counters_t c_counters; /* per connection operation counters */

    enum op_restriction c_restricted;
    uintptr_t c_restricted_inflight;
    time_t c_restricted_at;
    LloadBackend *c_backend;
    LloadConnection *c_linked_upstream;

    TAvlnode *c_linked;

#ifdef BALANCER_MODULE
    struct berval c_monitor_dn;
#endif /* BALANCER_MODULE */

    /*
     * Protected by the CIRCLEQ mutex:
     * - Client: clients_mutex
     * - Upstream: b->b_mutex
     */
    LDAP_CIRCLEQ_ENTRY(LloadConnection) c_next;
};

enum op_state {
    LLOAD_OP_NOT_FREEING = 0,
    LLOAD_OP_DETACHING_CLIENT = 1 << 1,
    LLOAD_OP_DETACHING_UPSTREAM = 1 << 0,
};

#define LLOAD_OP_DETACHING_MASK \
    ( LLOAD_OP_DETACHING_UPSTREAM | LLOAD_OP_DETACHING_CLIENT )

/* operation result for monitoring purposes */
enum op_result {
    LLOAD_OP_REJECTED,  /* operation was not forwarded */
    LLOAD_OP_COMPLETED, /* operation sent and response received */
    LLOAD_OP_FAILED, /* operation was forwarded, but no response was received */
};

/*
 * Operation reference tracking:
 * - o_refcnt is set to 1, never incremented
 * - OPERATION_UNLINK sets it to 0 and on transition from 1 clears both
 *   connection links (o_client, o_upstream)
 */
struct LloadOperation {
    uintptr_t o_refcnt;
#define OPERATION_UNLINK(op) \
    try_release_ref( &(op)->o_refcnt, (op), \
            (dispose_cb *)operation_unlink, \
            (dispose_cb *)operation_destroy )

    LloadConnection *o_client;
    unsigned long o_client_connid;
    ber_int_t o_client_msgid;
    ber_int_t o_saved_msgid;
    enum op_restriction o_restricted;

    LloadConnection *o_upstream;
    unsigned long o_upstream_connid;
    ber_int_t o_upstream_msgid;
    struct timeval o_last_response;

    /* Protects o_client, o_upstream links */
    ldap_pvt_thread_mutex_t o_link_mutex;

    ber_tag_t o_tag;
    struct timeval o_start;
    unsigned long o_pin_id;

    enum op_result o_res;
    BerElement *o_ber;
    BerValue o_request, o_ctrls;
};

struct restriction_entry {
    struct berval oid;
    enum op_restriction action;
};

/*
 * listener; need to access it from monitor backend
 */
struct LloadListener {
    struct berval sl_url;
    struct berval sl_name;
    mode_t sl_perms;
#ifdef HAVE_TLS
    int sl_is_tls;
#endif
    int sl_is_proxied;
    struct event_base *base;
    struct evconnlistener *listener;
    int sl_mute; /* Listener is temporarily disabled due to emfile */
    int sl_busy; /* Listener is busy (accept thread activated) */
    ber_socket_t sl_sd;
    Sockaddr sl_sa;
#define sl_addr sl_sa.sa_in_addr
#define LDAP_TCP_BUFFER
#ifdef LDAP_TCP_BUFFER
    int sl_tcp_rmem; /* custom TCP read buffer size */
    int sl_tcp_wmem; /* custom TCP write buffer size */
#endif
};

typedef int (*CONNCB)( LloadConnection *c, void *arg );

/* config requires a bi_private with configuration data - dummy for now */
struct lload_conf_info {
    int dummy;
};
LDAP_END_DECL

#include "proto-lload.h"
#endif /* _LLOAD_H_ */
