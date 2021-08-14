/*	$NetBSD: backend.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2021 The OpenLDAP Foundation.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: backend.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include <ac/socket.h>
#include <ac/errno.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include <event2/event.h>
#include <event2/dns.h>

#include "lutil.h"
#include "lload.h"

static void
upstream_connect_cb( evutil_socket_t s, short what, void *arg )
{
    LloadPendingConnection *conn = arg;
    LloadBackend *b = conn->backend;
    int error = 0, rc = -1;
    epoch_t epoch;

    checked_lock( &b->b_mutex );
    Debug( LDAP_DEBUG_CONNS, "upstream_connect_cb: "
            "fd=%d connection callback for backend uri='%s'\n",
            s, b->b_uri.bv_val );

    if ( s != conn->fd ) {
        /* backend_reset has been here first */
        goto preempted;
    }

    epoch = epoch_join();

    if ( what == EV_WRITE ) {
        socklen_t optlen = sizeof(error);

        if ( getsockopt( conn->fd, SOL_SOCKET, SO_ERROR, (void *)&error,
                     &optlen ) < 0 ) {
            goto done;
        }
        if ( error == EINTR || error == EINPROGRESS || error == EWOULDBLOCK ) {
            checked_unlock( &b->b_mutex );
            epoch_leave( epoch );
            return;
        } else if ( error ) {
            goto done;
        } else if ( upstream_init( s, conn->backend ) == NULL ) {
            goto done;
        }
        rc = LDAP_SUCCESS;
    }

done:
    epoch_leave( epoch );

    LDAP_LIST_REMOVE( conn, next );
    if ( rc ) {
        evutil_closesocket( conn->fd );
        b->b_opening--;
        b->b_failed++;
        if ( what & EV_TIMEOUT ) {
            Debug( LDAP_DEBUG_ANY, "upstream_connect_cb: "
                    "fd=%d connection timed out\n",
                    s );
        } else {
            char ebuf[128];
            Debug( LDAP_DEBUG_ANY, "upstream_connect_cb: "
                    "fd=%d connection set up failed%s%s\n",
                    s, error ? ": " : "",
                    error ? sock_errstr( error, ebuf, sizeof(ebuf) ) : "" );
        }
        backend_retry( b );
    }
preempted:
    checked_unlock( &b->b_mutex );

    event_free( conn->event );
    ch_free( conn );
}

static void
upstream_name_cb( int result, struct evutil_addrinfo *res, void *arg )
{
    LloadBackend *b = arg;
    ber_socket_t s = AC_SOCKET_INVALID;
    epoch_t epoch;
    int rc;

    if ( result == EVUTIL_EAI_CANCEL ) {
        Debug( LDAP_DEBUG_ANY, "upstream_name_cb: "
                "cancelled\n" );
        return;
    }

    checked_lock( &b->b_mutex );
    /* We were already running when backend_reset tried to cancel us, but were
     * already stuck waiting for the mutex, nothing to do and b_opening has
     * been decremented as well */
    if ( b->b_dns_req == NULL ) {
        checked_unlock( &b->b_mutex );
        return;
    }
    b->b_dns_req = NULL;

    epoch = epoch_join();
    if ( result || !res ) {
        Debug( LDAP_DEBUG_ANY, "upstream_name_cb: "
                "name resolution failed for backend '%s': %s\n",
                b->b_uri.bv_val, evutil_gai_strerror( result ) );
        goto fail;
    }

    /* TODO: if we get failures, try the other addrinfos */
    if ( (s = socket( res->ai_family, SOCK_STREAM, 0 )) ==
            AC_SOCKET_INVALID ) {
        goto fail;
    }

    if ( ber_pvt_socket_set_nonblock( s, 1 ) ) {
        goto fail;
    }

#if defined(SO_KEEPALIVE) || defined(TCP_NODELAY)
    if ( b->b_proto == LDAP_PROTO_TCP ) {
        int dummy = 1;
#ifdef SO_KEEPALIVE
        if ( setsockopt( s, SOL_SOCKET, SO_KEEPALIVE, (char *)&dummy,
                     sizeof(dummy) ) == AC_SOCKET_ERROR ) {
            Debug( LDAP_DEBUG_TRACE, "upstream_name_cb: "
                    "setsockopt(%d, SO_KEEPALIVE) failed (ignored).\n",
                    s );
        }
        if ( bindconf.sb_keepalive.sk_idle > 0 ) {
#ifdef TCP_KEEPIDLE
            if ( setsockopt( s, IPPROTO_TCP, TCP_KEEPIDLE,
                         (void *)&bindconf.sb_keepalive.sk_idle,
                         sizeof(bindconf.sb_keepalive.sk_idle) ) ==
                    AC_SOCKET_ERROR ) {
                Debug( LDAP_DEBUG_TRACE, "upstream_name_cb: "
                        "setsockopt(%d, TCP_KEEPIDLE) failed (ignored).\n",
                        s );
            }
#else
            Debug( LDAP_DEBUG_TRACE, "upstream_name_cb: "
                    "sockopt TCP_KEEPIDLE not supported on this system.\n" );
#endif /* TCP_KEEPIDLE */
        }
        if ( bindconf.sb_keepalive.sk_probes > 0 ) {
#ifdef TCP_KEEPCNT
            if ( setsockopt( s, IPPROTO_TCP, TCP_KEEPCNT,
                         (void *)&bindconf.sb_keepalive.sk_probes,
                         sizeof(bindconf.sb_keepalive.sk_probes) ) ==
                    AC_SOCKET_ERROR ) {
                Debug( LDAP_DEBUG_TRACE, "upstream_name_cb: "
                        "setsockopt(%d, TCP_KEEPCNT) failed (ignored).\n",
                        s );
            }
#else
            Debug( LDAP_DEBUG_TRACE, "upstream_name_cb: "
                    "sockopt TCP_KEEPCNT not supported on this system.\n" );
#endif /* TCP_KEEPCNT */
        }
        if ( bindconf.sb_keepalive.sk_interval > 0 ) {
#ifdef TCP_KEEPINTVL
            if ( setsockopt( s, IPPROTO_TCP, TCP_KEEPINTVL,
                         (void *)&bindconf.sb_keepalive.sk_interval,
                         sizeof(bindconf.sb_keepalive.sk_interval) ) ==
                    AC_SOCKET_ERROR ) {
                Debug( LDAP_DEBUG_TRACE, "upstream_name_cb: "
                        "setsockopt(%d, TCP_KEEPINTVL) failed (ignored).\n",
                        s );
            }
#else
            Debug( LDAP_DEBUG_TRACE, "upstream_name_cb: "
                    "sockopt TCP_KEEPINTVL not supported on this system.\n" );
#endif /* TCP_KEEPINTVL */
        }
#endif /* SO_KEEPALIVE */
        if ( bindconf.sb_tcp_user_timeout > 0 ) {
#ifdef TCP_USER_TIMEOUT
            if ( setsockopt( s, IPPROTO_TCP, TCP_USER_TIMEOUT,
                         (void *)&bindconf.sb_tcp_user_timeout,
                         sizeof(bindconf.sb_tcp_user_timeout) ) ==
                    AC_SOCKET_ERROR ) {
                Debug( LDAP_DEBUG_TRACE, "upstream_name_cb: "
                        "setsockopt(%d, TCP_USER_TIMEOUT) failed (ignored).\n",
                        s );
            }
#else
            Debug( LDAP_DEBUG_TRACE, "upstream_name_cb: "
                    "sockopt TCP_USER_TIMEOUT not supported on this "
                    "system.\n" );
#endif /* TCP_USER_TIMEOUT */
        }
#ifdef TCP_NODELAY
        if ( setsockopt( s, IPPROTO_TCP, TCP_NODELAY, (char *)&dummy,
                     sizeof(dummy) ) == AC_SOCKET_ERROR ) {
            Debug( LDAP_DEBUG_TRACE, "upstream_name_cb: "
                    "setsockopt(%d, TCP_NODELAY) failed (ignored).\n",
                    s );
        }
#endif /* TCP_NODELAY */
    }
#endif /* SO_KEEPALIVE || TCP_NODELAY */

    if ( res->ai_family == PF_INET ) {
        struct sockaddr_in *ai = (struct sockaddr_in *)res->ai_addr;
        ai->sin_port = htons( b->b_port );
        rc = connect( s, (struct sockaddr *)ai, res->ai_addrlen );
    } else {
        struct sockaddr_in6 *ai = (struct sockaddr_in6 *)res->ai_addr;
        ai->sin6_port = htons( b->b_port );
        rc = connect( s, (struct sockaddr *)ai, res->ai_addrlen );
    }
    /* Asynchronous connect */
    if ( rc ) {
        LloadPendingConnection *conn;

        if ( errno != EINPROGRESS && errno != EWOULDBLOCK ) {
            Debug( LDAP_DEBUG_ANY, "upstream_name_cb: "
                    "failed to connect to server '%s'\n",
                    b->b_uri.bv_val );
            evutil_closesocket( s );
            goto fail;
        }

        conn = ch_calloc( 1, sizeof(LloadPendingConnection) );
        LDAP_LIST_ENTRY_INIT( conn, next );
        conn->backend = b;
        conn->fd = s;

        conn->event = event_new( lload_get_base( s ), s, EV_WRITE|EV_PERSIST,
                upstream_connect_cb, conn );
        if ( !conn->event ) {
            Debug( LDAP_DEBUG_ANY, "upstream_name_cb: "
                    "failed to acquire an event to finish upstream "
                    "connection setup.\n" );
            ch_free( conn );
            evutil_closesocket( s );
            goto fail;
        }

        event_add( conn->event, lload_timeout_net );
        LDAP_LIST_INSERT_HEAD( &b->b_connecting, conn, next );
        Debug( LDAP_DEBUG_CONNS, "upstream_name_cb: "
                "connection to backend uri=%s in progress\n",
                b->b_uri.bv_val );
    } else if ( upstream_init( s, b ) == NULL ) {
        goto fail;
    }

    checked_unlock( &b->b_mutex );
    evutil_freeaddrinfo( res );
    epoch_leave( epoch );
    return;

fail:
    if ( s != AC_SOCKET_INVALID ) {
        evutil_closesocket( s );
    }
    b->b_opening--;
    b->b_failed++;
    backend_retry( b );
    checked_unlock( &b->b_mutex );
    if ( res ) {
        evutil_freeaddrinfo( res );
    }
    epoch_leave( epoch );
}

LloadConnection *
backend_select( LloadOperation *op, int *res )
{
    LloadBackend *b, *first, *next;

    checked_lock( &backend_mutex );
    first = b = current_backend;
    checked_unlock( &backend_mutex );

    *res = LDAP_UNAVAILABLE;

    if ( !first ) {
        return NULL;
    }

    /* TODO: Two runs, one with trylock, then one actually locked if we don't
     * find anything? */
    do {
        lload_c_head *head;
        LloadConnection *c;

        checked_lock( &b->b_mutex );
        next = LDAP_CIRCLEQ_LOOP_NEXT( &backend, b, b_next );

        if ( b->b_max_pending && b->b_n_ops_executing >= b->b_max_pending ) {
            Debug( LDAP_DEBUG_CONNS, "backend_select: "
                    "backend %s too busy\n",
                    b->b_uri.bv_val );
            checked_unlock( &b->b_mutex );
            b = next;
            *res = LDAP_BUSY;
            continue;
        }

        if ( op->o_tag == LDAP_REQ_BIND
#ifdef LDAP_API_FEATURE_VERIFY_CREDENTIALS
                && !(lload_features & LLOAD_FEATURE_VC)
#endif /* LDAP_API_FEATURE_VERIFY_CREDENTIALS */
        ) {
            head = &b->b_bindconns;
        } else {
            head = &b->b_conns;
        }
        if ( !LDAP_CIRCLEQ_EMPTY( head ) ) {
            *res = LDAP_BUSY;
        }

        LDAP_CIRCLEQ_FOREACH ( c, head, c_next ) {
            checked_lock( &c->c_io_mutex );
            CONNECTION_LOCK(c);
            if ( c->c_state == LLOAD_C_READY && !c->c_pendingber &&
                    ( b->b_max_conn_pending == 0 ||
                            c->c_n_ops_executing < b->b_max_conn_pending ) ) {
                Debug( LDAP_DEBUG_CONNS, "backend_select: "
                        "selected connection connid=%lu for client "
                        "connid=%lu msgid=%d\n",
                        c->c_connid, op->o_client_connid, op->o_client_msgid );

                /* c_state is DYING if we're about to be unlinked */
                assert( IS_ALIVE( c, c_live ) );

                /*
                 * Round-robin step:
                 * Rotate the queue to put this connection at the end, same for
                 * the backend.
                 */
                LDAP_CIRCLEQ_MAKE_TAIL( head, c, c_next );

                checked_lock( &backend_mutex );
                current_backend = next;
                checked_unlock( &backend_mutex );

                b->b_n_ops_executing++;
                if ( op->o_tag == LDAP_REQ_BIND ) {
                    b->b_counters[LLOAD_STATS_OPS_BIND].lc_ops_received++;
                } else {
                    b->b_counters[LLOAD_STATS_OPS_OTHER].lc_ops_received++;
                }
                c->c_n_ops_executing++;
                c->c_counters.lc_ops_received++;

                checked_unlock( &b->b_mutex );
                *res = LDAP_SUCCESS;
                CONNECTION_ASSERT_LOCKED(c);
                assert_locked( &c->c_io_mutex );
                return c;
            }
            CONNECTION_UNLOCK(c);
            checked_unlock( &c->c_io_mutex );
        }
        checked_unlock( &b->b_mutex );

        b = next;
    } while ( b != first );

    return NULL;
}

/*
 * Will schedule a connection attempt if there is a need for it. Need exclusive
 * access to backend, its b_mutex is not touched here, though.
 */
void
backend_retry( LloadBackend *b )
{
    int requested;

    if ( slapd_shutdown ) {
        Debug( LDAP_DEBUG_CONNS, "backend_retry: "
                "shutting down\n" );
        return;
    }
    assert_locked( &b->b_mutex );

    requested = b->b_numconns;
#ifdef LDAP_API_FEATURE_VERIFY_CREDENTIALS
    if ( !(lload_features & LLOAD_FEATURE_VC) )
#endif /* LDAP_API_FEATURE_VERIFY_CREDENTIALS */
    {
        requested += b->b_numbindconns;
    }

    if ( b->b_active + b->b_bindavail + b->b_opening >= requested ) {
        Debug( LDAP_DEBUG_CONNS, "backend_retry: "
                "no more connections needed for this backend\n" );
        assert_locked( &b->b_mutex );
        return;
    }

    if ( b->b_opening > 0 ) {
        Debug( LDAP_DEBUG_CONNS, "backend_retry: "
                "retry in progress already\n" );
        assert( b->b_opening == 1 );
        assert_locked( &b->b_mutex );
        return;
    }

    /* We incremented b_opening when we activated the event, so it can't be
     * pending */
    assert( !event_pending( b->b_retry_event, EV_TIMEOUT, NULL ) );
    b->b_opening++;

    if ( b->b_failed > 0 ) {
        Debug( LDAP_DEBUG_CONNS, "backend_retry: "
                "scheduling a retry in %d ms\n",
                b->b_retry_timeout );
        event_add( b->b_retry_event, &b->b_retry_tv );
        assert_locked( &b->b_mutex );
        return;
    }

    Debug( LDAP_DEBUG_CONNS, "backend_retry: "
            "scheduling re-connection straight away\n" );

    if ( ldap_pvt_thread_pool_submit2(
                 &connection_pool, backend_connect_task, b, &b->b_cookie ) ) {
        Debug( LDAP_DEBUG_ANY, "backend_retry: "
                "failed to submit retry task, scheduling a retry instead\n" );
        /* The current implementation of ldap_pvt_thread_pool_submit2 can fail
         * and still set (an invalid) cookie */
        b->b_cookie = NULL;
        b->b_failed++;
        event_add( b->b_retry_event, &b->b_retry_tv );
    }
    assert_locked( &b->b_mutex );
}

void
backend_connect( evutil_socket_t s, short what, void *arg )
{
    struct evutil_addrinfo hints = {};
    LloadBackend *b = arg;
    struct evdns_getaddrinfo_request *request, *placeholder;
    char *hostname;
    epoch_t epoch;

    checked_lock( &b->b_mutex );
    assert( b->b_dns_req == NULL );

    if ( b->b_cookie ) {
        b->b_cookie = NULL;
    }

    if ( slapd_shutdown ) {
        Debug( LDAP_DEBUG_CONNS, "backend_connect: "
                "doing nothing, shutdown in progress\n" );
        b->b_opening--;
        checked_unlock( &b->b_mutex );
        return;
    }

    epoch = epoch_join();

    Debug( LDAP_DEBUG_CONNS, "backend_connect: "
            "%sattempting connection to %s\n",
            (what & EV_TIMEOUT) ? "retry timeout finished, " : "",
            b->b_host );

#ifdef LDAP_PF_LOCAL
    if ( b->b_proto == LDAP_PROTO_IPC ) {
        struct sockaddr_un addr;
        ber_socket_t s = socket( PF_LOCAL, SOCK_STREAM, 0 );
        int rc;

        if ( s == AC_SOCKET_INVALID ) {
            goto fail;
        }

        rc = ber_pvt_socket_set_nonblock( s, 1 );
        if ( rc ) {
            evutil_closesocket( s );
            goto fail;
        }

        if ( strlen( b->b_host ) > ( sizeof(addr.sun_path) - 1 ) ) {
            evutil_closesocket( s );
            goto fail;
        }
        memset( &addr, '\0', sizeof(addr) );
        addr.sun_family = AF_LOCAL;
        strcpy( addr.sun_path, b->b_host );

        rc = connect(
                s, (struct sockaddr *)&addr, sizeof(struct sockaddr_un) );
        /* Asynchronous connect */
        if ( rc ) {
            LloadPendingConnection *conn;

            if ( errno != EINPROGRESS && errno != EWOULDBLOCK ) {
                evutil_closesocket( s );
                goto fail;
            }

            conn = ch_calloc( 1, sizeof(LloadPendingConnection) );
            LDAP_LIST_ENTRY_INIT( conn, next );
            conn->backend = b;
            conn->fd = s;

            conn->event = event_new( lload_get_base( s ), s,
                    EV_WRITE|EV_PERSIST, upstream_connect_cb, conn );
            if ( !conn->event ) {
                Debug( LDAP_DEBUG_ANY, "backend_connect: "
                        "failed to acquire an event to finish upstream "
                        "connection setup.\n" );
                ch_free( conn );
                evutil_closesocket( s );
                goto fail;
            }

            event_add( conn->event, lload_timeout_net );
            LDAP_LIST_INSERT_HEAD( &b->b_connecting, conn, next );
            Debug( LDAP_DEBUG_CONNS, "backend_connect: "
                    "connection to backend uri=%s in progress\n",
                    b->b_uri.bv_val );
        } else if ( upstream_init( s, b ) == NULL ) {
            goto fail;
        }

        checked_unlock( &b->b_mutex );
        epoch_leave( epoch );
        return;
    }
#endif /* LDAP_PF_LOCAL */

    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = EVUTIL_AI_CANONNAME;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    hostname = b->b_host;

    /*
     * Picking any value on the stack. This is unique to our thread without
     * having to call ldap_pvt_thread_self.
     * We might have to revert to using ldap_pvt_thread_self eventually since
     * this betrays where exactly our stack lies - potentially weakening some
     * protections like ASLR.
     */
    placeholder = (struct evdns_getaddrinfo_request *)&request;
    b->b_dns_req = placeholder;
    checked_unlock( &b->b_mutex );

    request = evdns_getaddrinfo(
            dnsbase, hostname, NULL, &hints, upstream_name_cb, b );

    checked_lock( &b->b_mutex );
    assert( request || b->b_dns_req != placeholder );

    /* Record the request, unless upstream_name_cb or another thread
     * cleared it. Another thread is usually backend_reset or backend_connect
     * if upstream_name_cb finished and scheduled another one */
    if ( b->b_dns_req == placeholder ) {
        b->b_dns_req = request;
    }
    checked_unlock( &b->b_mutex );
    epoch_leave( epoch );
    return;

fail:
    b->b_opening--;
    b->b_failed++;
    backend_retry( b );
    checked_unlock( &b->b_mutex );
    epoch_leave( epoch );
}

void *
backend_connect_task( void *ctx, void *arg )
{
    backend_connect( -1, 0, arg );
    return NULL;
}

/*
 * Needs exclusive access to the backend and no other thread is allowed to call
 * backend_retry while we're handling this.
 *
 * If gentle == 0, a full pause must be in effect, else we risk deadlocking on
 * event_free().
 */
void
backend_reset( LloadBackend *b, int gentle )
{
    assert_locked( &b->b_mutex );
    if ( b->b_cookie ) {
        if ( ldap_pvt_thread_pool_retract( b->b_cookie ) ) {
            b->b_cookie = NULL;
            b->b_opening--;
        } else {
            /*
             * The task might not be cancelable because it just started
             * executing.
             *
             * Shutdown should be the only time when the thread pool is
             * in that state. Keep the cookie in to keep an eye on whether
             * it's finished yet.
             */
            assert( slapd_shutdown );
        }
    }
    /* Not safe to hold our mutex and call event_del/free if the event's
     * callback is running, relinquish the mutex while we do so. */
    if ( b->b_retry_event &&
            event_pending( b->b_retry_event, EV_TIMEOUT, NULL ) ) {
        assert( b->b_failed );
        checked_unlock( &b->b_mutex );
        event_del( b->b_retry_event );
        checked_lock( &b->b_mutex );
        b->b_opening--;
    }
    if ( b->b_dns_req ) {
        evdns_getaddrinfo_cancel( b->b_dns_req );
        b->b_dns_req = NULL;
        b->b_opening--;
    }
    while ( !LDAP_LIST_EMPTY( &b->b_connecting ) ) {
        LloadPendingConnection *pending = LDAP_LIST_FIRST( &b->b_connecting );

        Debug( LDAP_DEBUG_CONNS, "backend_reset: "
                "destroying socket pending connect() fd=%d\n",
                pending->fd );

        event_active( pending->event, EV_WRITE, 0 );
        evutil_closesocket( pending->fd );
        pending->fd = -1;
        LDAP_LIST_REMOVE( pending, next );

        if ( !gentle ) {
            /* None of the event bases are running, we're safe to free the
             * event right now and potentially free the backend itself */
            event_free( pending->event );
            ch_free( pending );
        }
        /* else, just let the event dispose of the resources on its own later */
        b->b_opening--;
    }
    connections_walk(
            &b->b_mutex, &b->b_preparing, lload_connection_close, &gentle );
    assert( LDAP_CIRCLEQ_EMPTY( &b->b_preparing ) );
    assert( b->b_opening == ( b->b_cookie ? 1 : 0 ) );
    b->b_failed = 0;

    connections_walk_last( &b->b_mutex, &b->b_bindconns, b->b_last_bindconn,
            lload_connection_close, &gentle );
    assert( gentle || b->b_bindavail == 0 );

    connections_walk_last( &b->b_mutex, &b->b_conns, b->b_last_conn,
            lload_connection_close, &gentle );
    assert( gentle || b->b_active == 0 );
    assert_locked( &b->b_mutex );
}

void
lload_backend_destroy( LloadBackend *b )
{
    LloadBackend *next = LDAP_CIRCLEQ_LOOP_NEXT( &backend, b, b_next );

    Debug( LDAP_DEBUG_CONNS, "lload_backend_destroy: "
            "destroying backend uri='%s', numconns=%d, numbindconns=%d\n",
            b->b_uri.bv_val, b->b_numconns, b->b_numbindconns );

    checked_lock( &b->b_mutex );
    b->b_numconns = b->b_numbindconns = 0;
    backend_reset( b, 0 );

    LDAP_CIRCLEQ_REMOVE( &backend, b, b_next );
    if ( b == next ) {
        current_backend = NULL;
    } else {
        current_backend = next;
    }

#ifdef BALANCER_MODULE
    if ( b->b_monitor ) {
        BackendDB *be;
        struct berval monitordn = BER_BVC("cn=monitor");
        int rc;

        be = select_backend( &monitordn, 0 );

        /* FIXME: implement proper subsys shutdown in back-monitor or make
         * backend just an entry, not a subsys */
        rc = b->b_monitor->mss_destroy( be, b->b_monitor );
        assert( rc == LDAP_SUCCESS );
    }
#endif /* BALANCER_MODULE */
    checked_unlock( &b->b_mutex );
    ldap_pvt_thread_mutex_destroy( &b->b_mutex );

    if ( b->b_retry_event ) {
        event_del( b->b_retry_event );
        event_free( b->b_retry_event );
        b->b_retry_event = NULL;
    }

    ch_free( b->b_host );
    ch_free( b->b_uri.bv_val );
    ch_free( b->b_name.bv_val );
    ch_free( b );
}

void
lload_backends_destroy( void )
{
    while ( !LDAP_CIRCLEQ_EMPTY( &backend ) ) {
        LloadBackend *b = LDAP_CIRCLEQ_FIRST( &backend );

        lload_backend_destroy( b );
    }
}
