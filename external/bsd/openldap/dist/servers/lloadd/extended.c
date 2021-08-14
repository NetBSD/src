/*	$NetBSD: extended.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

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
__RCSID("$NetBSD: extended.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include <ac/string.h>

#include "lutil.h"
#include "lload.h"

Avlnode *lload_exop_handlers = NULL;

#ifdef HAVE_TLS
void *lload_tls_ctx;
LDAP *lload_tls_ld, *lload_tls_backend_ld;
#ifdef BALANCER_MODULE
int lload_use_slap_tls_ctx = 0;
#endif
#endif /* HAVE_TLS */

int
handle_starttls( LloadConnection *c, LloadOperation *op )
{
    struct event_base *base = event_get_base( c->c_read_event );
    LloadOperation *found;
    BerElement *output;
    char *msg = NULL;
    int rc = LDAP_SUCCESS;

    CONNECTION_LOCK(c);
    found = ldap_tavl_delete( &c->c_ops, op, operation_client_cmp );
    assert( op == found );
    c->c_n_ops_executing--;

#ifdef HAVE_TLS
    if ( c->c_is_tls == LLOAD_TLS_ESTABLISHED ) {
        rc = LDAP_OPERATIONS_ERROR;
        msg = "TLS layer already in effect";
    } else if ( c->c_state == LLOAD_C_BINDING ) {
        rc = LDAP_OPERATIONS_ERROR;
        msg = "bind in progress";
    } else if ( c->c_ops ) {
        rc = LDAP_OPERATIONS_ERROR;
        msg = "cannot start TLS when operations are outstanding";
    } else if ( !LLOAD_TLS_CTX ) {
        rc = LDAP_UNAVAILABLE;
        msg = "Could not initialize TLS";
    }
#else /* ! HAVE_TLS */
    rc = LDAP_UNAVAILABLE;
    msg = "Could not initialize TLS";
#endif /* ! HAVE_TLS */

    CONNECTION_UNLOCK(c);

    Debug( LDAP_DEBUG_STATS, "handle_starttls: "
            "handling StartTLS exop connid=%lu rc=%d msg=%s\n",
            c->c_connid, rc, msg );

    if ( rc ) {
        /* We've already removed the operation from the queue */
        operation_send_reject( op, rc, msg, 1 );
        return LDAP_SUCCESS;
    }

#ifdef HAVE_TLS
    event_del( c->c_read_event );
    event_del( c->c_write_event );
    /*
     * At this point, we are the only thread handling the connection:
     * - there are no upstream operations
     * - the I/O callbacks have been successfully removed
     *
     * This means we can safely reconfigure both I/O events now.
     */

    checked_lock( &c->c_io_mutex );
    output = c->c_pendingber;
    if ( output == NULL && (output = ber_alloc()) == NULL ) {
        checked_unlock( &c->c_io_mutex );
        operation_unlink( op );
        CONNECTION_LOCK_DESTROY(c);
        return -1;
    }
    c->c_pendingber = output;
    ber_printf( output, "t{tit{ess}}", LDAP_TAG_MESSAGE,
            LDAP_TAG_MSGID, op->o_client_msgid,
            LDAP_RES_EXTENDED, LDAP_SUCCESS, "", "" );
    c->c_io_state &= ~LLOAD_C_READ_HANDOVER;
    checked_unlock( &c->c_io_mutex );

    CONNECTION_LOCK(c);
    c->c_read_timeout = lload_timeout_net;
    event_assign( c->c_read_event, base, c->c_fd, EV_READ|EV_PERSIST,
            client_tls_handshake_cb, c );
    event_add( c->c_read_event, c->c_read_timeout );

    event_assign( c->c_write_event, base, c->c_fd, EV_WRITE,
            client_tls_handshake_cb, c );
    /* We already have something to write */
    event_add( c->c_write_event, lload_write_timeout );

    op->o_res = LLOAD_OP_COMPLETED;
    CONNECTION_UNLOCK(c);

    operation_unlink( op );

    return -1;
#endif /* HAVE_TLS */
}

int
request_extended( LloadConnection *c, LloadOperation *op )
{
    ExopHandler *handler, needle = {};
    BerElement *copy;
    struct berval bv;
    ber_tag_t tag;

    if ( (copy = ber_alloc()) == NULL ) {
        operation_send_reject( op, LDAP_OTHER, "internal error", 0 );
        CONNECTION_LOCK_DESTROY(c);
        return -1;
    }

    ber_init2( copy, &op->o_request, 0 );

    tag = ber_skip_element( copy, &bv );
    if ( tag != LDAP_TAG_EXOP_REQ_OID ) {
        Debug( LDAP_DEBUG_STATS, "request_extended: "
                "no OID present in extended request\n" );
        operation_send_reject( op, LDAP_PROTOCOL_ERROR, "decoding error", 0 );
        CONNECTION_LOCK_DESTROY(c);
        return -1;
    }

    needle.oid = bv;

    handler = ldap_avl_find( lload_exop_handlers, &needle, exop_handler_cmp );
    if ( handler ) {
        Debug( LDAP_DEBUG_TRACE, "request_extended: "
                "handling exop OID %.*s internally\n",
                (int)bv.bv_len, bv.bv_val );
        ber_free( copy, 0 );
        return handler->func( c, op );
    }
    ber_free( copy, 0 );

    if ( c->c_state == LLOAD_C_BINDING ) {
        operation_send_reject( op, LDAP_PROTOCOL_ERROR, "bind in progress", 0 );
        return LDAP_SUCCESS;
    }
    return request_process( c, op );
}

ExopHandler lload_exops[] = {
        { BER_BVC(LDAP_EXOP_START_TLS), handle_starttls },
        { BER_BVNULL }
};

int
exop_handler_cmp( const void *left, const void *right )
{
    const struct lload_exop_handlers_t *l = left, *r = right;
    return ber_bvcmp( &l->oid, &r->oid );
}

int
lload_register_exop_handlers( struct lload_exop_handlers_t *handler )
{
    for ( ; !BER_BVISNULL( &handler->oid ); handler++ ) {
        Debug( LDAP_DEBUG_TRACE, "lload_register_exop_handlers: "
                "registering handler for exop oid=%s\n",
                handler->oid.bv_val );
        if ( ldap_avl_insert( &lload_exop_handlers, handler, exop_handler_cmp,
                     ldap_avl_dup_error ) ) {
            Debug( LDAP_DEBUG_ANY, "lload_register_exop_handlers: "
                    "failed to register handler for exop oid=%s\n",
                    handler->oid.bv_val );
            return -1;
        }
    }

    return LDAP_SUCCESS;
}

int
lload_exop_init( void )
{
    if ( lload_register_exop_handlers( lload_exops ) ) {
        return -1;
    }

    return LDAP_SUCCESS;
}
