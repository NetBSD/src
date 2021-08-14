/*	$NetBSD: connection.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: connection.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include <stdio.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <ac/socket.h>
#include <ac/errno.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include "lload.h"

#include "lutil.h"
#include "lutil_ldap.h"

static unsigned long conn_nextid = 0;

static void
lload_connection_assign_nextid( LloadConnection *conn )
{
    conn->c_connid = __atomic_fetch_add( &conn_nextid, 1, __ATOMIC_RELAXED );
}

/*
 * We start off with the connection muted and c_currentber holding the pdu we
 * received.
 *
 * We run c->c_pdu_cb for each pdu, stopping once we hit an error, have to wait
 * on reading or after we process lload_conn_max_pdus_per_cycle pdus so as to
 * maintain fairness and not hog the worker thread forever.
 *
 * If we've run out of pdus immediately available from the stream or hit the
 * budget, we unmute the connection.
 *
 * c->c_pdu_cb might return an 'error' and not free the connection. That can
 * happen when changing the state or when client is blocked on writing and
 * already has a pdu pending on the same operation, it's their job to make sure
 * we're woken up again.
 */
void *
handle_pdus( void *ctx, void *arg )
{
    LloadConnection *c = arg;
    int pdus_handled = 0;
    epoch_t epoch;

    /* A reference was passed on to us */
    assert( IS_ALIVE( c, c_refcnt ) );

    epoch = epoch_join();
    for ( ;; ) {
        BerElement *ber;
        ber_tag_t tag;
        ber_len_t len;

        if ( c->c_pdu_cb( c ) ) {
            /* Error/reset, get rid ouf our reference and bail */
            goto done;
        }

        if ( !IS_ALIVE( c, c_live ) ) {
            break;
        }

        if ( ++pdus_handled >= lload_conn_max_pdus_per_cycle ) {
            /* Do not read now, re-enable read event instead */
            break;
        }

        ber = c->c_currentber;
        if ( ber == NULL && (ber = ber_alloc()) == NULL ) {
            Debug( LDAP_DEBUG_ANY, "handle_pdus: "
                    "connid=%lu, ber_alloc failed\n",
                    c->c_connid );
            CONNECTION_LOCK_DESTROY(c);
            goto done;
        }
        c->c_currentber = ber;

        checked_lock( &c->c_io_mutex );
        if ( (lload_features & LLOAD_FEATURE_PAUSE) &&
                (c->c_io_state & LLOAD_C_READ_PAUSE) ) {
            goto pause;
        }
        tag = ber_get_next( c->c_sb, &len, ber );
        checked_unlock( &c->c_io_mutex );
        if ( tag != LDAP_TAG_MESSAGE ) {
            int err = sock_errno();

            if ( err != EWOULDBLOCK && err != EAGAIN ) {
                if ( err || tag == LBER_ERROR ) {
                    char ebuf[128];
                    Debug( LDAP_DEBUG_ANY, "handle_pdus: "
                            "ber_get_next on fd=%d failed errno=%d (%s)\n",
                            c->c_fd, err,
                            sock_errstr( err, ebuf, sizeof(ebuf) ) );
                } else {
                    Debug( LDAP_DEBUG_STATS, "handle_pdus: "
                            "ber_get_next on fd=%d connid=%lu received "
                            "a strange PDU tag=%lx\n",
                            c->c_fd, c->c_connid, tag );
                }

                c->c_currentber = NULL;
                ber_free( ber, 1 );
                CONNECTION_LOCK_DESTROY(c);
                goto done;
            }
            break;
        }

        assert( IS_ALIVE( c, c_refcnt ) );
        epoch_leave( epoch );
        epoch = epoch_join();
        assert( IS_ALIVE( c, c_refcnt ) );
    }

    checked_lock( &c->c_io_mutex );
    if ( !(lload_features & LLOAD_FEATURE_PAUSE) ||
            !(c->c_io_state & LLOAD_C_READ_PAUSE) ) {
        event_add( c->c_read_event, c->c_read_timeout );
        Debug( LDAP_DEBUG_CONNS, "handle_pdus: "
                "re-enabled read event on connid=%lu\n",
                c->c_connid );
    }
pause:
    c->c_io_state &= ~LLOAD_C_READ_HANDOVER;
    checked_unlock( &c->c_io_mutex );

done:
    RELEASE_REF( c, c_refcnt, c->c_destroy );
    epoch_leave( epoch );
    return NULL;
}

/*
 * Initial read on the connection, if we get an LDAP PDU, submit the
 * processing of this and successive ones to the work queue.
 *
 * If we can't submit it to the queue (overload), process this one and return
 * to the event loop immediately after.
 */
void
connection_read_cb( evutil_socket_t s, short what, void *arg )
{
    LloadConnection *c = arg;
    BerElement *ber;
    ber_tag_t tag;
    ber_len_t len;
    epoch_t epoch;
    int pause;

    if ( !IS_ALIVE( c, c_live ) ) {
        event_del( c->c_read_event );
        Debug( LDAP_DEBUG_CONNS, "connection_read_cb: "
                "suspended read event on a dead connid=%lu\n",
                c->c_connid );
        return;
    }

    if ( what & EV_TIMEOUT ) {
        Debug( LDAP_DEBUG_CONNS, "connection_read_cb: "
                "connid=%lu, timeout reached, destroying\n",
                c->c_connid );
        /* Make sure the connection stays around for us to unlock it */
        epoch = epoch_join();
        CONNECTION_LOCK_DESTROY(c);
        epoch_leave( epoch );
        return;
    }

    if ( !acquire_ref( &c->c_refcnt ) ) {
        return;
    }
    epoch = epoch_join();

    Debug( LDAP_DEBUG_CONNS, "connection_read_cb: "
            "connection connid=%lu ready to read\n",
            c->c_connid );

    ber = c->c_currentber;
    if ( ber == NULL && (ber = ber_alloc()) == NULL ) {
        Debug( LDAP_DEBUG_ANY, "connection_read_cb: "
                "connid=%lu, ber_alloc failed\n",
                c->c_connid );
        goto out;
    }
    c->c_currentber = ber;

    checked_lock( &c->c_io_mutex );
    assert( !(c->c_io_state & LLOAD_C_READ_HANDOVER) );
    tag = ber_get_next( c->c_sb, &len, ber );
    pause = c->c_io_state & LLOAD_C_READ_PAUSE;
    checked_unlock( &c->c_io_mutex );

    if ( tag != LDAP_TAG_MESSAGE ) {
        int err = sock_errno();

        if ( err != EWOULDBLOCK && err != EAGAIN ) {
            if ( err || tag == LBER_ERROR ) {
                char ebuf[128];
                Debug( LDAP_DEBUG_STATS, "connection_read_cb: "
                        "ber_get_next on fd=%d failed errno=%d (%s)\n",
                        c->c_fd, err,
                        sock_errstr( err, ebuf, sizeof(ebuf) ) );
            } else {
                Debug( LDAP_DEBUG_STATS, "connection_read_cb: "
                        "ber_get_next on fd=%d connid=%lu received "
                        "a strange PDU tag=%lx\n",
                        c->c_fd, c->c_connid, tag );
            }

            c->c_currentber = NULL;
            ber_free( ber, 1 );

            event_del( c->c_read_event );
            Debug( LDAP_DEBUG_CONNS, "connection_read_cb: "
                    "suspended read event on dying connid=%lu\n",
                    c->c_connid );
            CONNECTION_LOCK_DESTROY(c);
            goto out;
        }
        if ( !(lload_features & LLOAD_FEATURE_PAUSE) || !pause ) {
            event_add( c->c_read_event, c->c_read_timeout );
            Debug( LDAP_DEBUG_CONNS, "connection_read_cb: "
                    "re-enabled read event on connid=%lu\n",
                    c->c_connid );
        }
        goto out;
    }

    checked_lock( &c->c_io_mutex );
    c->c_io_state |= LLOAD_C_READ_HANDOVER;
    checked_unlock( &c->c_io_mutex );
    event_del( c->c_read_event );

    if ( !lload_conn_max_pdus_per_cycle ||
            ldap_pvt_thread_pool_submit( &connection_pool, handle_pdus, c ) ) {
        /* If we're overloaded or configured as such, process one and resume in
         * the next cycle. */
        int rc = c->c_pdu_cb( c );

        checked_lock( &c->c_io_mutex );
        c->c_io_state &= ~LLOAD_C_READ_HANDOVER;
        if ( rc == LDAP_SUCCESS &&
                ( !(lload_features & LLOAD_FEATURE_PAUSE) ||
                        !(c->c_io_state & LLOAD_C_READ_PAUSE) ) ) {
            event_add( c->c_read_event, c->c_read_timeout );
        }
        checked_unlock( &c->c_io_mutex );
        goto out;
    }

    Debug( LDAP_DEBUG_CONNS, "connection_read_cb: "
            "suspended read event on connid=%lu\n",
            c->c_connid );

    /*
     * We have scheduled a call to handle_pdus to take care of handling this
     * and further requests, its reference is now owned by that task.
     */
    epoch_leave( epoch );
    return;

out:
    RELEASE_REF( c, c_refcnt, c->c_destroy );
    epoch_leave( epoch );
}

void
connection_write_cb( evutil_socket_t s, short what, void *arg )
{
    LloadConnection *c = arg;
    epoch_t epoch;

    Debug( LDAP_DEBUG_CONNS, "connection_write_cb: "
            "considering writing to%s connid=%lu what=%hd\n",
            c->c_live ? " live" : " dead", c->c_connid, what );
    if ( !IS_ALIVE( c, c_live ) ) {
        return;
    }

    if ( what & EV_TIMEOUT ) {
        Debug( LDAP_DEBUG_CONNS, "connection_write_cb: "
                "connid=%lu, timeout reached, destroying\n",
                c->c_connid );
        /* Make sure the connection stays around for us to unlock it */
        epoch = epoch_join();
        CONNECTION_LOCK_DESTROY(c);
        epoch_leave( epoch );
        return;
    }

    /* Before we acquire any locks */
    event_del( c->c_write_event );

    if ( !acquire_ref( &c->c_refcnt ) ) {
        return;
    }

    /* If what == 0, we have a caller as opposed to being a callback */
    if ( what ) {
        epoch = epoch_join();
    }

    checked_lock( &c->c_io_mutex );
    Debug( LDAP_DEBUG_CONNS, "connection_write_cb: "
            "have something to write to connection connid=%lu\n",
            c->c_connid );

    /* We might have been beaten to flushing the data by another thread */
    if ( c->c_pendingber && ber_flush( c->c_sb, c->c_pendingber, 1 ) ) {
        int err = sock_errno();

        if ( err != EWOULDBLOCK && err != EAGAIN ) {
            char ebuf[128];
            checked_unlock( &c->c_io_mutex );
            Debug( LDAP_DEBUG_ANY, "connection_write_cb: "
                    "ber_flush on fd=%d failed errno=%d (%s)\n",
                    c->c_fd, err, sock_errstr( err, ebuf, sizeof(ebuf) ) );
            CONNECTION_LOCK_DESTROY(c);
            goto done;
        }

        if ( !(c->c_io_state & LLOAD_C_READ_PAUSE) ) {
            Debug( LDAP_DEBUG_CONNS, "connection_write_cb: "
                    "connection connid=%lu blocked on writing, marking "
                    "paused\n",
                    c->c_connid );
        }
        c->c_io_state |= LLOAD_C_READ_PAUSE;

        /* TODO: Do not reset write timeout unless we wrote something */
        event_add( c->c_write_event, lload_write_timeout );
    } else {
        c->c_pendingber = NULL;
        if ( c->c_io_state & LLOAD_C_READ_PAUSE ) {
            c->c_io_state ^= LLOAD_C_READ_PAUSE;
            Debug( LDAP_DEBUG_CONNS, "connection_write_cb: "
                    "Unpausing connection connid=%lu\n",
                    c->c_connid );
            if ( !(c->c_io_state & LLOAD_C_READ_HANDOVER) ) {
                event_add( c->c_read_event, c->c_read_timeout );
            }
        }
    }
    checked_unlock( &c->c_io_mutex );

done:
    RELEASE_REF( c, c_refcnt, c->c_destroy );
    if ( what ) {
        epoch_leave( epoch );
    }
}

void
connection_destroy( LloadConnection *c )
{
    assert( c );
    Debug( LDAP_DEBUG_CONNS, "connection_destroy: "
            "destroying connection connid=%lu\n",
            c->c_connid );

    CONNECTION_ASSERT_LOCKED(c);
    assert( c->c_live == 0 );
    assert( c->c_refcnt == 0 );
    assert( c->c_state == LLOAD_C_INVALID );

    ber_sockbuf_free( c->c_sb );

    if ( c->c_currentber ) {
        ber_free( c->c_currentber, 1 );
        c->c_currentber = NULL;
    }
    if ( c->c_pendingber ) {
        ber_free( c->c_pendingber, 1 );
        c->c_pendingber = NULL;
    }

    if ( !BER_BVISNULL( &c->c_sasl_bind_mech ) ) {
        ber_memfree( c->c_sasl_bind_mech.bv_val );
        BER_BVZERO( &c->c_sasl_bind_mech );
    }
#ifdef HAVE_CYRUS_SASL
    if ( c->c_sasl_defaults ) {
        lutil_sasl_freedefs( c->c_sasl_defaults );
        c->c_sasl_defaults = NULL;
    }
    if ( c->c_sasl_authctx ) {
#ifdef SASL_CHANNEL_BINDING /* 2.1.25+ */
        if ( c->c_sasl_cbinding ) {
            ch_free( c->c_sasl_cbinding );
        }
#endif
        sasl_dispose( &c->c_sasl_authctx );
    }
#endif /* HAVE_CYRUS_SASL */

    CONNECTION_UNLOCK(c);

    ldap_pvt_thread_mutex_destroy( &c->c_io_mutex );
    ldap_pvt_thread_mutex_destroy( &c->c_mutex );

    ch_free( c );

    listeners_reactivate();
}

/*
 * Called holding mutex, will walk cq calling cb on all connections whose
 * c_connid <= cq_last->c_connid that still exist at the time we get to them.
 */
void
connections_walk_last(
        ldap_pvt_thread_mutex_t *cq_mutex,
        lload_c_head *cq,
        LloadConnection *cq_last,
        CONNCB cb,
        void *arg )
{
    LloadConnection *c = cq_last;
    uintptr_t last_connid;

    if ( LDAP_CIRCLEQ_EMPTY( cq ) ) {
        return;
    }
    assert_locked( cq_mutex );

    last_connid = c->c_connid;
    c = LDAP_CIRCLEQ_LOOP_NEXT( cq, c, c_next );

    while ( !acquire_ref( &c->c_refcnt ) ) {
        c = LDAP_CIRCLEQ_LOOP_NEXT( cq, c, c_next );
        if ( c->c_connid >= last_connid ) {
            assert_locked( cq_mutex );
            return;
        }
    }

    /*
     * Notes:
     * - we maintain the connections in the cq CIRCLEQ_ in ascending c_connid
     *   order
     * - the connection with the highest c_connid is passed in cq_last
     * - we can only use cq when we hold cq_mutex
     * - connections might be added to or removed from cq while we're busy
     *   processing connections
     * - we need a way to detect we've finished looping around cq for some
     *   definition of looping around
     */
    do {
        int rc;

        checked_unlock( cq_mutex );

        rc = cb( c, arg );
        RELEASE_REF( c, c_refcnt, c->c_destroy );

        checked_lock( cq_mutex );
        if ( rc || LDAP_CIRCLEQ_EMPTY( cq ) ) {
            break;
        }

        do {
            LloadConnection *old = c;
            c = LDAP_CIRCLEQ_LOOP_NEXT( cq, c, c_next );
            if ( c->c_connid <= old->c_connid || c->c_connid > last_connid ) {
                assert_locked( cq_mutex );
                return;
            }
        } while ( !acquire_ref( &c->c_refcnt ) );
    } while ( c->c_connid <= last_connid );
    assert_locked( cq_mutex );
}

void
connections_walk(
        ldap_pvt_thread_mutex_t *cq_mutex,
        lload_c_head *cq,
        CONNCB cb,
        void *arg )
{
    LloadConnection *cq_last = LDAP_CIRCLEQ_LAST( cq );
    return connections_walk_last( cq_mutex, cq, cq_last, cb, arg );
}

int
lload_connection_close( LloadConnection *c, void *arg )
{
    int gentle = *(int *)arg;
    LloadOperation *op;

    Debug( LDAP_DEBUG_CONNS, "lload_connection_close: "
            "marking connection connid=%lu closing\n",
            c->c_connid );

    /* We were approached from the connection list */
    assert( IS_ALIVE( c, c_refcnt ) );

    CONNECTION_LOCK(c);
    if ( !gentle || !c->c_ops ) {
        CONNECTION_DESTROY(c);
        return LDAP_SUCCESS;
    }

    /* The first thing we do is make sure we don't get new Operations in */
    c->c_state = LLOAD_C_CLOSING;

    do {
        TAvlnode *node = ldap_tavl_end( c->c_ops, TAVL_DIR_LEFT );
        op = node->avl_data;

        /* Close operations that would need client action to resolve,
         * only SASL binds in progress do that right now */
        if ( op->o_client_msgid || op->o_upstream_msgid ) {
            break;
        }

        CONNECTION_UNLOCK(c);
        operation_unlink( op );
        CONNECTION_LOCK(c);
    } while ( c->c_ops );

    CONNECTION_UNLOCK(c);
    return LDAP_SUCCESS;
}

LloadConnection *
lload_connection_init( ber_socket_t s, const char *peername, int flags )
{
    LloadConnection *c;

    assert( peername != NULL );

    if ( s == AC_SOCKET_INVALID ) {
        Debug( LDAP_DEBUG_ANY, "lload_connection_init: "
                "init of socket fd=%ld invalid\n",
                (long)s );
        return NULL;
    }

    assert( s >= 0 );

    c = ch_calloc( 1, sizeof(LloadConnection) );

    c->c_fd = s;
    c->c_sb = ber_sockbuf_alloc();
    ber_sockbuf_ctrl( c->c_sb, LBER_SB_OPT_SET_FD, &s );

#ifdef LDAP_PF_LOCAL
    if ( flags & CONN_IS_IPC ) {
#ifdef LDAP_DEBUG
        ber_sockbuf_add_io( c->c_sb, &ber_sockbuf_io_debug,
                LBER_SBIOD_LEVEL_PROVIDER, (void *)"ipc_" );
#endif
        ber_sockbuf_add_io( c->c_sb, &ber_sockbuf_io_fd,
                LBER_SBIOD_LEVEL_PROVIDER, (void *)&s );
    } else
#endif /* LDAP_PF_LOCAL */
    {
#ifdef LDAP_DEBUG
        ber_sockbuf_add_io( c->c_sb, &ber_sockbuf_io_debug,
                LBER_SBIOD_LEVEL_PROVIDER, (void *)"tcp_" );
#endif
        ber_sockbuf_add_io( c->c_sb, &ber_sockbuf_io_tcp,
                LBER_SBIOD_LEVEL_PROVIDER, (void *)&s );
    }

#ifdef LDAP_DEBUG
    ber_sockbuf_add_io(
            c->c_sb, &ber_sockbuf_io_debug, INT_MAX, (void *)"lload_" );
#endif

    c->c_next_msgid = 1;
    c->c_refcnt = c->c_live = 1;
    c->c_destroy = connection_destroy;

    LDAP_CIRCLEQ_ENTRY_INIT( c, c_next );

    ldap_pvt_thread_mutex_init( &c->c_mutex );
    ldap_pvt_thread_mutex_init( &c->c_io_mutex );

    lload_connection_assign_nextid( c );

    Debug( LDAP_DEBUG_CONNS, "lload_connection_init: "
            "connection connid=%lu allocated for socket fd=%d peername=%s\n",
            c->c_connid, s, peername );

    c->c_state = LLOAD_C_ACTIVE;

    return c;
}
