/*	$NetBSD: operation.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

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
__RCSID("$NetBSD: operation.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include "lutil.h"
#include "lload.h"

ldap_pvt_thread_mutex_t lload_pin_mutex;
unsigned long lload_next_pin = 1;

ber_tag_t
slap_req2res( ber_tag_t tag )
{
    switch ( tag ) {
        case LDAP_REQ_ADD:
        case LDAP_REQ_BIND:
        case LDAP_REQ_COMPARE:
        case LDAP_REQ_EXTENDED:
        case LDAP_REQ_MODIFY:
        case LDAP_REQ_MODRDN:
            tag++;
            break;

        case LDAP_REQ_DELETE:
            tag = LDAP_RES_DELETE;
            break;

        case LDAP_REQ_ABANDON:
        case LDAP_REQ_UNBIND:
            tag = LBER_SEQUENCE;
            break;

        case LDAP_REQ_SEARCH:
            tag = LDAP_RES_SEARCH_RESULT;
            break;

        default:
            tag = LBER_SEQUENCE;
    }

    return tag;
}

const char *
lload_msgtype2str( ber_tag_t tag )
{
    switch ( tag ) {
        case LDAP_REQ_ABANDON: return "abandon request";
        case LDAP_REQ_ADD: return "add request";
        case LDAP_REQ_BIND: return "bind request";
        case LDAP_REQ_COMPARE: return "compare request";
        case LDAP_REQ_DELETE: return "delete request";
        case LDAP_REQ_EXTENDED: return "extended request";
        case LDAP_REQ_MODIFY: return "modify request";
        case LDAP_REQ_RENAME: return "rename request";
        case LDAP_REQ_SEARCH: return "search request";
        case LDAP_REQ_UNBIND: return "unbind request";

        case LDAP_RES_ADD: return "add result";
        case LDAP_RES_BIND: return "bind result";
        case LDAP_RES_COMPARE: return "compare result";
        case LDAP_RES_DELETE: return "delete result";
        case LDAP_RES_EXTENDED: return "extended result";
        case LDAP_RES_INTERMEDIATE: return "intermediate response";
        case LDAP_RES_MODIFY: return "modify result";
        case LDAP_RES_RENAME: return "rename result";
        case LDAP_RES_SEARCH_ENTRY: return "search-entry response";
        case LDAP_RES_SEARCH_REFERENCE: return "search-reference response";
        case LDAP_RES_SEARCH_RESULT: return "search result";
    }
    return "unknown message";
}

int
operation_client_cmp( const void *left, const void *right )
{
    const LloadOperation *l = left, *r = right;

    assert( l->o_client_connid == r->o_client_connid );
    if ( l->o_client_msgid || r->o_client_msgid ) {
        return ( l->o_client_msgid < r->o_client_msgid ) ?
                -1 :
                ( l->o_client_msgid > r->o_client_msgid );
    } else {
        return ( l->o_pin_id < r->o_pin_id ) ? -1 :
                ( l->o_pin_id > r->o_pin_id );
    }
}

int
operation_upstream_cmp( const void *left, const void *right )
{
    const LloadOperation *l = left, *r = right;

    assert( l->o_upstream_connid == r->o_upstream_connid );
    if ( l->o_upstream_msgid || r->o_upstream_msgid ) {
        return ( l->o_upstream_msgid < r->o_upstream_msgid ) ?
                -1 :
                ( l->o_upstream_msgid > r->o_upstream_msgid );
    } else {
        return ( l->o_pin_id < r->o_pin_id ) ? -1 :
                ( l->o_pin_id > r->o_pin_id );
    }
}

/*
 * Entered holding c_mutex for now.
 */
LloadOperation *
operation_init( LloadConnection *c, BerElement *ber )
{
    LloadOperation *op;
    ber_tag_t tag;
    ber_len_t len;
    int rc;

    if ( !IS_ALIVE( c, c_live ) ) {
        return NULL;
    }

    op = ch_calloc( 1, sizeof(LloadOperation) );
    op->o_client = c;
    op->o_client_connid = c->c_connid;
    op->o_ber = ber;
    op->o_start = slap_get_time();

    ldap_pvt_thread_mutex_init( &op->o_link_mutex );

    op->o_refcnt = 1;

    tag = ber_get_int( ber, &op->o_client_msgid );
    if ( tag != LDAP_TAG_MSGID ) {
        goto fail;
    }

    if ( !op->o_client_msgid ) {
        goto fail;
    }

    CONNECTION_ASSERT_LOCKED(c);
    rc = ldap_tavl_insert( &c->c_ops, op, operation_client_cmp, ldap_avl_dup_error );
    if ( rc ) {
        Debug( LDAP_DEBUG_PACKETS, "operation_init: "
                "several operations with same msgid=%d in-flight "
                "from client connid=%lu\n",
                op->o_client_msgid, op->o_client_connid );
        goto fail;
    }

    tag = op->o_tag = ber_skip_element( ber, &op->o_request );
    switch ( tag ) {
        case LBER_ERROR:
            rc = -1;
            break;
    }
    if ( rc ) {
        ldap_tavl_delete( &c->c_ops, op, operation_client_cmp );
        goto fail;
    }

    tag = ber_peek_tag( ber, &len );
    if ( tag == LDAP_TAG_CONTROLS ) {
        ber_skip_element( ber, &op->o_ctrls );
    }

    switch ( op->o_tag ) {
        case LDAP_REQ_BIND:
            lload_stats.counters[LLOAD_STATS_OPS_BIND].lc_ops_received++;
            break;
        default:
            lload_stats.counters[LLOAD_STATS_OPS_OTHER].lc_ops_received++;
            break;
    }

    Debug( LDAP_DEBUG_STATS, "operation_init: "
            "received a new operation, %s with msgid=%d for client "
            "connid=%lu\n",
            lload_msgtype2str( op->o_tag ), op->o_client_msgid,
            op->o_client_connid );

    c->c_n_ops_executing++;
    return op;

fail:
    ch_free( op );
    return NULL;
}

void
operation_destroy( LloadOperation *op )
{
    Debug( LDAP_DEBUG_TRACE, "operation_destroy: "
            "op=%p destroyed operation from client connid=%lu, "
            "client msgid=%d\n",
            op, op->o_client_connid, op->o_client_msgid );

    assert( op->o_refcnt == 0 );
    assert( op->o_client == NULL );
    assert( op->o_upstream == NULL );

    ber_free( op->o_ber, 1 );
    ldap_pvt_thread_mutex_destroy( &op->o_link_mutex );
    ch_free( op );
}

int
operation_unlink( LloadOperation *op )
{
    LloadConnection *client, *upstream;
    uintptr_t prev_refcnt;
    int result = 0;

    if ( !( prev_refcnt = try_release_ref(
                    &op->o_refcnt, op, (dispose_cb *)operation_destroy ) ) ) {
        return result;
    }

    assert( prev_refcnt == 1 );

    Debug( LDAP_DEBUG_TRACE, "operation_unlink: "
            "unlinking operation between client connid=%lu and upstream "
            "connid=%lu "
            "client msgid=%d\n",
            op->o_client_connid, op->o_upstream_connid, op->o_client_msgid );

    checked_lock( &op->o_link_mutex );
    client = op->o_client;
    upstream = op->o_upstream;

    op->o_client = NULL;
    op->o_upstream = NULL;
    checked_unlock( &op->o_link_mutex );

    assert( client || upstream );

    if ( client ) {
        result |= operation_unlink_client( op, client );
        operation_update_global_rejected( op );
    }

    if ( upstream ) {
        result |= operation_unlink_upstream( op, upstream );
    }

    return result;
}

int
operation_unlink_client( LloadOperation *op, LloadConnection *client )
{
    LloadOperation *removed;
    int result = 0;

    Debug( LDAP_DEBUG_TRACE, "operation_unlink_client: "
            "unlinking operation op=%p msgid=%d client connid=%lu\n",
            op, op->o_client_msgid, op->o_client_connid );

    CONNECTION_LOCK(client);
    if ( (removed = ldap_tavl_delete(
                   &client->c_ops, op, operation_client_cmp )) ) {
        result = LLOAD_OP_DETACHING_CLIENT;

        assert( op == removed );
        client->c_n_ops_executing--;

        if ( client->c_state == LLOAD_C_BINDING ) {
            client->c_state = LLOAD_C_READY;
            if ( !BER_BVISNULL( &client->c_auth ) ) {
                ber_memfree( client->c_auth.bv_val );
                BER_BVZERO( &client->c_auth );
            }
            if ( !BER_BVISNULL( &client->c_sasl_bind_mech ) ) {
                ber_memfree( client->c_sasl_bind_mech.bv_val );
                BER_BVZERO( &client->c_sasl_bind_mech );
            }
            if ( op->o_pin_id ) {
                client->c_pin_id = 0;
            }
        }
    }
    if ( client->c_state == LLOAD_C_CLOSING && !client->c_ops ) {
        CONNECTION_DESTROY(client);
    } else {
        CONNECTION_UNLOCK(client);
    }

    return result;
}

int
operation_unlink_upstream( LloadOperation *op, LloadConnection *upstream )
{
    LloadOperation *removed;
    LloadBackend *b = NULL;
    int result = 0;

    Debug( LDAP_DEBUG_TRACE, "operation_unlink_upstream: "
            "unlinking operation op=%p msgid=%d upstream connid=%lu\n",
            op, op->o_upstream_msgid, op->o_upstream_connid );

    CONNECTION_LOCK(upstream);
    if ( (removed = ldap_tavl_delete(
                   &upstream->c_ops, op, operation_upstream_cmp )) ) {
        result |= LLOAD_OP_DETACHING_UPSTREAM;

        assert( op == removed );
        upstream->c_n_ops_executing--;

        if ( upstream->c_state == LLOAD_C_BINDING ) {
            assert( op->o_tag == LDAP_REQ_BIND && upstream->c_ops == NULL );
            upstream->c_state = LLOAD_C_READY;
            if ( !BER_BVISNULL( &upstream->c_sasl_bind_mech ) ) {
                ber_memfree( upstream->c_sasl_bind_mech.bv_val );
                BER_BVZERO( &upstream->c_sasl_bind_mech );
            }
        }
        operation_update_conn_counters( op, upstream );
        b = upstream->c_backend;
    }
    if ( upstream->c_state == LLOAD_C_CLOSING && !upstream->c_ops ) {
        CONNECTION_DESTROY(upstream);
    } else {
        CONNECTION_UNLOCK(upstream);
    }

    if ( b ) {
        checked_lock( &b->b_mutex );
        b->b_n_ops_executing--;
        operation_update_backend_counters( op, b );
        checked_unlock( &b->b_mutex );
    }

    return result;
}

int
operation_send_abandon( LloadOperation *op, LloadConnection *upstream )
{
    BerElement *ber;
    int rc = -1;

    if ( !IS_ALIVE( upstream, c_live ) ) {
        return rc;
    }

    checked_lock( &upstream->c_io_mutex );
    ber = upstream->c_pendingber;
    if ( ber == NULL && (ber = ber_alloc()) == NULL ) {
        Debug( LDAP_DEBUG_ANY, "operation_send_abandon: "
                "ber_alloc failed\n" );
        goto done;
    }
    upstream->c_pendingber = ber;

    Debug( LDAP_DEBUG_TRACE, "operation_send_abandon: "
            "abandoning %s msgid=%d on connid=%lu\n",
            lload_msgtype2str( op->o_tag ), op->o_upstream_msgid,
            op->o_upstream_connid );

    if ( op->o_tag == LDAP_REQ_BIND ) {
        rc = ber_printf( ber, "t{tit{ist{s}}}", LDAP_TAG_MESSAGE,
                LDAP_TAG_MSGID, upstream->c_next_msgid++,
                LDAP_REQ_BIND, LDAP_VERSION3, "", LDAP_AUTH_SASL, "" );
    } else {
        rc = ber_printf( ber, "t{titi}", LDAP_TAG_MESSAGE,
                LDAP_TAG_MSGID, upstream->c_next_msgid++,
                LDAP_REQ_ABANDON, op->o_upstream_msgid );
    }

    if ( rc < 0 ) {
        ber_free( ber, 1 );
        upstream->c_pendingber = NULL;
        goto done;
    }
    rc = LDAP_SUCCESS;

done:
    checked_unlock( &upstream->c_io_mutex );
    return rc;
}

/*
 * Will remove the operation from its upstream and if it was still there,
 * sends an abandon request.
 *
 * Being called from client_reset or request_abandon, the following hold:
 * - noone else is processing the read part of the client connection (no new
 *   operations come in there - relevant for the c_state checks)
 * - op->o_client_refcnt > op->o_client_live (and it follows that op->o_client != NULL)
 */
void
operation_abandon( LloadOperation *op )
{
    LloadConnection *c;

    checked_lock( &op->o_link_mutex );
    c = op->o_upstream;
    checked_unlock( &op->o_link_mutex );
    if ( !c || !IS_ALIVE( c, c_live ) ) {
        goto done;
    }

    /* for now consider all abandoned operations completed,
     * perhaps add a separate counter later */
    op->o_res = LLOAD_OP_COMPLETED;
    if ( !operation_unlink_upstream( op, c ) ) {
        /* The operation has already been abandoned or finished */
        Debug( LDAP_DEBUG_TRACE, "operation_abandon: "
                "%s from connid=%lu msgid=%d not present in connid=%lu any "
                "more\n",
                lload_msgtype2str( op->o_tag ), op->o_client_connid,
                op->o_client_msgid, op->o_upstream_connid );
        goto done;
    }

    if ( operation_send_abandon( op, c ) == LDAP_SUCCESS ) {
        connection_write_cb( -1, 0, c );
    }

done:
    operation_unlink( op );
}

void
operation_send_reject(
        LloadOperation *op,
        int result,
        const char *msg,
        int send_anyway )
{
    LloadConnection *c;
    BerElement *ber;
    int found;

    Debug( LDAP_DEBUG_TRACE, "operation_send_reject: "
            "rejecting %s from client connid=%lu with message: \"%s\"\n",
            lload_msgtype2str( op->o_tag ), op->o_client_connid, msg );

    checked_lock( &op->o_link_mutex );
    c = op->o_client;
    checked_unlock( &op->o_link_mutex );
    if ( !c || !IS_ALIVE( c, c_live ) ) {
        Debug( LDAP_DEBUG_TRACE, "operation_send_reject: "
                "not sending msgid=%d, client connid=%lu is dead\n",
                op->o_client_msgid, op->o_client_connid );

        goto done;
    }

    found = operation_unlink_client( op, c );
    if ( !found && !send_anyway ) {
        Debug( LDAP_DEBUG_TRACE, "operation_send_reject: "
                "msgid=%d not scheduled for client connid=%lu anymore, "
                "not sending\n",
                op->o_client_msgid, c->c_connid );
        goto done;
    }

    if ( op->o_client_msgid == 0 ) {
        assert( op->o_saved_msgid == 0 && op->o_pin_id );
        Debug( LDAP_DEBUG_TRACE, "operation_send_reject: "
                "operation pin=%lu is just a pin, not sending\n",
                op->o_pin_id );
        goto done;
    }

    checked_lock( &c->c_io_mutex );
    ber = c->c_pendingber;
    if ( ber == NULL && (ber = ber_alloc()) == NULL ) {
        checked_unlock( &c->c_io_mutex );
        Debug( LDAP_DEBUG_ANY, "operation_send_reject: "
                "ber_alloc failed, closing connid=%lu\n",
                c->c_connid );
        CONNECTION_LOCK_DESTROY(c);
        goto done;
    }
    c->c_pendingber = ber;

    ber_printf( ber, "t{tit{ess}}", LDAP_TAG_MESSAGE,
            LDAP_TAG_MSGID, op->o_client_msgid,
            slap_req2res( op->o_tag ), result, "", msg );

    checked_unlock( &c->c_io_mutex );

    connection_write_cb( -1, 0, c );

done:
    operation_unlink( op );
}

/*
 * Upstream is shutting down, signal the client if necessary, but we have to
 * call operation_destroy_from_upstream ourselves to detach upstream from the
 * op.
 *
 * Only called from upstream_destroy.
 */
void
operation_lost_upstream( LloadOperation *op )
{
    operation_send_reject( op, LDAP_OTHER,
            "connection to the remote server has been severed", 0 );
}

int
connection_timeout( LloadConnection *upstream, void *arg )
{
    LloadOperation *op;
    TAvlnode *ops = NULL, *node, *next;
    LloadBackend *b = upstream->c_backend;
    time_t threshold = *(time_t *)arg;
    int rc, nops = 0;

    CONNECTION_LOCK(upstream);
    for ( node = ldap_tavl_end( upstream->c_ops, TAVL_DIR_LEFT ); node &&
            ((LloadOperation *)node->avl_data)->o_start <
                    threshold; /* shortcut */
            node = next ) {
        LloadOperation *found_op;

        next = ldap_tavl_next( node, TAVL_DIR_RIGHT );
        op = node->avl_data;

        /* Have we received another response since? */
        if ( op->o_last_response && op->o_last_response >= threshold ) {
            continue;
        }

        op->o_res = LLOAD_OP_FAILED;
        found_op = ldap_tavl_delete( &upstream->c_ops, op, operation_upstream_cmp );
        assert( op == found_op );

        if ( upstream->c_state == LLOAD_C_BINDING ) {
            assert( op->o_tag == LDAP_REQ_BIND && upstream->c_ops == NULL );
            upstream->c_state = LLOAD_C_READY;
            if ( !BER_BVISNULL( &upstream->c_sasl_bind_mech ) ) {
                ber_memfree( upstream->c_sasl_bind_mech.bv_val );
                BER_BVZERO( &upstream->c_sasl_bind_mech );
            }
        }

        rc = ldap_tavl_insert( &ops, op, operation_upstream_cmp, ldap_avl_dup_error );
        assert( rc == LDAP_SUCCESS );

        Debug( LDAP_DEBUG_STATS2, "connection_timeout: "
                "timing out %s from connid=%lu msgid=%d sent to connid=%lu as "
                "msgid=%d\n",
                lload_msgtype2str( op->o_tag ), op->o_client_connid,
                op->o_client_msgid, op->o_upstream_connid,
                op->o_upstream_msgid );
        nops++;
    }

    if ( nops == 0 ) {
        CONNECTION_UNLOCK(upstream);
        return LDAP_SUCCESS;
    }
    upstream->c_n_ops_executing -= nops;
    upstream->c_counters.lc_ops_failed += nops;
    Debug( LDAP_DEBUG_STATS, "connection_timeout: "
            "timing out %d operations for connid=%lu\n",
            nops, upstream->c_connid );
    CONNECTION_UNLOCK(upstream);

    checked_lock( &b->b_mutex );
    b->b_n_ops_executing -= nops;
    checked_unlock( &b->b_mutex );

    for ( node = ldap_tavl_end( ops, TAVL_DIR_LEFT ); node;
            node = ldap_tavl_next( node, TAVL_DIR_RIGHT ) ) {
        op = node->avl_data;

        operation_send_reject( op,
                op->o_tag == LDAP_REQ_SEARCH ? LDAP_TIMELIMIT_EXCEEDED :
                                               LDAP_ADMINLIMIT_EXCEEDED,
                "upstream did not respond in time", 0 );

        if ( rc == LDAP_SUCCESS ) {
            rc = operation_send_abandon( op, upstream );
        }
        operation_unlink( op );
    }

    /* TODO: if operation_send_abandon failed, we need to kill the upstream */
    if ( rc == LDAP_SUCCESS ) {
        connection_write_cb( -1, 0, upstream );
    }

    CONNECTION_LOCK(upstream);
    if ( upstream->c_state == LLOAD_C_CLOSING && !upstream->c_ops ) {
        CONNECTION_DESTROY(upstream);
    } else {
        CONNECTION_UNLOCK(upstream);
    }

    /* just dispose of the AVL, most operations should already be gone */
    ldap_tavl_free( ops, NULL );
    return LDAP_SUCCESS;
}

void
operations_timeout( evutil_socket_t s, short what, void *arg )
{
    struct event *self = arg;
    LloadBackend *b;
    time_t threshold;

    Debug( LDAP_DEBUG_TRACE, "operations_timeout: "
            "running timeout task\n" );
    if ( !lload_timeout_api ) goto done;

    threshold = slap_get_time() - lload_timeout_api->tv_sec;

    LDAP_CIRCLEQ_FOREACH ( b, &backend, b_next ) {
        epoch_t epoch;

        checked_lock( &b->b_mutex );
        if ( b->b_n_ops_executing == 0 ) {
            checked_unlock( &b->b_mutex );
            continue;
        }

        epoch = epoch_join();

        Debug( LDAP_DEBUG_TRACE, "operations_timeout: "
                "timing out binds for backend uri=%s\n",
                b->b_uri.bv_val );
        connections_walk_last( &b->b_mutex, &b->b_bindconns, b->b_last_bindconn,
                connection_timeout, &threshold );

        Debug( LDAP_DEBUG_TRACE, "operations_timeout: "
                "timing out other operations for backend uri=%s\n",
                b->b_uri.bv_val );
        connections_walk_last( &b->b_mutex, &b->b_conns, b->b_last_conn,
                connection_timeout, &threshold );

        epoch_leave( epoch );
        checked_unlock( &b->b_mutex );
    }
done:
    Debug( LDAP_DEBUG_TRACE, "operations_timeout: "
            "timeout task finished\n" );
    evtimer_add( self, lload_timeout_api );
}

void
operation_update_global_rejected( LloadOperation *op )
{
    if ( op->o_res == LLOAD_OP_REJECTED ) {
        assert( op->o_upstream_connid == 0 );
        switch ( op->o_tag ) {
            case LDAP_REQ_BIND:
                lload_stats.counters[LLOAD_STATS_OPS_BIND].lc_ops_rejected++;
                break;
            default:
                lload_stats.counters[LLOAD_STATS_OPS_OTHER].lc_ops_rejected++;
                break;
        }
    }
}

void
operation_update_conn_counters( LloadOperation *op, LloadConnection *upstream )
{
    if ( op->o_res == LLOAD_OP_COMPLETED ) {
        upstream->c_counters.lc_ops_completed++;
    } else {
        upstream->c_counters.lc_ops_failed++;
    }
}

void
operation_update_backend_counters( LloadOperation *op, LloadBackend *b )
{
    int stat_type = op->o_tag == LDAP_REQ_BIND ? LLOAD_STATS_OPS_BIND :
                                                 LLOAD_STATS_OPS_OTHER;

    assert( b != NULL );
    if ( op->o_res == LLOAD_OP_COMPLETED ) {
        b->b_counters[stat_type].lc_ops_completed++;
    } else {
        b->b_counters[stat_type].lc_ops_failed++;
    }
}
