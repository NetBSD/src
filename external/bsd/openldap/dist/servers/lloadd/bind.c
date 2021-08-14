/*	$NetBSD: bind.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

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
__RCSID("$NetBSD: bind.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include <ac/socket.h>
#include <ac/errno.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include "lutil.h"
#include "lload.h"

struct berval mech_external = BER_BVC("EXTERNAL");

int
bind_mech_external(
        LloadConnection *client,
        LloadOperation *op,
        struct berval *credentials )
{
    BerValue binddn;
    void *ssl;
    char *ptr, *message = "";
    int result = LDAP_SUCCESS;

    CONNECTION_ASSERT_LOCKED(client);
    client->c_state = LLOAD_C_READY;
    client->c_type = LLOAD_C_OPEN;

    op->o_res = LLOAD_OP_COMPLETED;

    /*
     * We only support implicit assertion.
     *
     * Although RFC 4513 says the credentials field must be missing, RFC 4422
     * doesn't and libsasl2 will pass a zero-length string to send. We have to
     * allow that.
     */
    if ( !BER_BVISEMPTY( credentials ) ) {
        result = LDAP_UNWILLING_TO_PERFORM;
        message = "proxy authorization is not supported";
        goto done;
    }

#ifdef HAVE_TLS
    ssl = ldap_pvt_tls_sb_ctx( client->c_sb );
    if ( !ssl || ldap_pvt_tls_get_peer_dn( ssl, &binddn, NULL, 0 ) ) {
        result = LDAP_INVALID_CREDENTIALS;
        message = "no externally negotiated identity";
        goto done;
    }
    client->c_auth.bv_len = binddn.bv_len + STRLENOF("dn:");
    client->c_auth.bv_val = ch_malloc( client->c_auth.bv_len + 1 );

    ptr = lutil_strcopy( client->c_auth.bv_val, "dn:" );
    ptr = lutil_strncopy( ptr, binddn.bv_val, binddn.bv_len );
    *ptr = '\0';

    ber_memfree( binddn.bv_val );

    if ( !ber_bvstrcasecmp( &client->c_auth, &lloadd_identity ) ) {
        client->c_type = LLOAD_C_PRIVILEGED;
    }
#else /* ! HAVE_TLS */
    result = LDAP_AUTH_METHOD_NOT_SUPPORTED;
    message = "requested SASL mechanism not supported";
#endif /* ! HAVE_TLS */

done:
    CONNECTION_UNLOCK(client);
    operation_send_reject( op, result, message, 1 );
    return LDAP_SUCCESS;
}

static int
client_bind(
        LloadOperation *op,
        LloadConnection *upstream,
        struct berval *binddn,
        ber_tag_t tag,
        struct berval *auth )
{
    ber_printf( upstream->c_pendingber, "t{titOtO}", LDAP_TAG_MESSAGE,
            LDAP_TAG_MSGID, op->o_upstream_msgid,
            LDAP_REQ_BIND, &op->o_request,
            LDAP_TAG_CONTROLS, BER_BV_OPTIONAL( &op->o_ctrls ) );

    return 0;
}

#ifdef LDAP_API_FEATURE_VERIFY_CREDENTIALS
static int
client_bind_as_vc(
        LloadOperation *op,
        LloadConnection *upstream,
        struct berval *binddn,
        ber_tag_t tag,
        struct berval *auth )
{
    CONNECTION_LOCK(upstream);
    ber_printf( upstream->c_pendingber, "t{tit{tst{{tOOtOtO}}}}", LDAP_TAG_MESSAGE,
            LDAP_TAG_MSGID, op->o_upstream_msgid,
            LDAP_REQ_EXTENDED,
            LDAP_TAG_EXOP_REQ_OID, LDAP_EXOP_VERIFY_CREDENTIALS,
            LDAP_TAG_EXOP_REQ_VALUE,
            LDAP_TAG_EXOP_VERIFY_CREDENTIALS_COOKIE, BER_BV_OPTIONAL( &upstream->c_vc_cookie ),
            &binddn, tag, &auth,
            LDAP_TAG_EXOP_VERIFY_CREDENTIALS_CONTROLS, BER_BV_OPTIONAL( &op->o_ctrls ) );
    CONNECTION_UNLOCK(upstream);
    return 0;
}
#endif /* LDAP_API_FEATURE_VERIFY_CREDENTIALS */

/*
 * The client connection can be in the following states:
 * 1) there are between zero and many non-bind operations pending
 *    client->c_state == LLOAD_C_READY && client->c_pin_id == 0
 * 2) there is one bind operation pending (waiting on an upstream response)
 *    a) It is a simple bind
 *    b) It is a SASL bind
 * 3) there is one SASL bind in progress (received a LDAP_SASL_BIND_IN_PROGRESS
 *    response)
 *
 * In cases 2 and 3, client->c_state == LLOAD_C_BINDING, a SASL bind is in
 * progress/pending if c_sasl_bind_mech is set.
 *
 * In the first case, client_reset abandons all operations on the respective
 * upstreams, case 2a has client_reset send an anonymous bind to upstream to
 * terminate the bind. In cases 2b and 3, c_pin_id is set and we retrieve the
 * op. The rest is the same for both.
 *
 * If c_pin_id is unset, we request an upstream connection assigned, otherwise,
 * we try to reuse the pinned upstream. In the case of no upstream, we reject
 * the request. A SASL bind request means we acquire a new pin_id if we don't
 * have one already.
 *
 * We have to reset c_auth (which holds the current or pending identity) and
 * make sure we set it up eventually:
 * - In the case of a simple bind, we already know the final identity being
 *   requested so we set it up immediately
 * - In SASL binds, for mechanisms we implement ourselves (EXTERNAL), we set it
 *   up at some point
 * - Otherwise, we have to ask the upstream what it thinks as the bind
 *   succeeds, we send an LDAP "Who Am I?" exop, this is one of the few
 *   requests we send on our own. If we implement the mechanism, we provide the
 *   identity (EXTERNAL uses the client certificate DN)
 *
 * At the end of the request processing, if nothing goes wrong, we're in state
 * 2b (with c_pin_id set to the op's o_pin_id), or state 2a (we could reset
 * c_pin_id/o_pin_id if we wanted but we don't always do that at the moment).
 * If something does go wrong, we're either tearing down the client or we
 * reject the request and switch to state 1 (clearing c_pin_id).
 *
 * As usual, we have to make any changes to the target connection before we've
 * sent the PDU over it - while we are in charge of the read side and nothing
 * happens there without our ceding control, the other read side could wake up
 * at any time and preempt us.
 *
 * On a response (in handle_bind_response):
 * - to a simple bind, clear c_auth on a failure otherwise keep it while we
 *   just reset the client to state 1
 * - failure response to a SASL bind - reset client to state 1
 * - LDAP_SASL_BIND_IN_PROGRESS - clear o_*_msgid from the op (have to
 *   remove+reinsert it from the respective c_ops!), we need it since it is the
 *   vessel maintaining the pin between client and upstream
 * - all of the above forward the response immediately
 * - LDAP_SUCCESS for a SASL bind - we send a "Who Am I?" request to retrieve
 *   the client's DN, only on receiving the response do we finalise the
 *   exchange by forwarding the successful bind response
 *
 * We can't do the same for VC Exop since the exchange is finished at the end
 * and we need a change to the VC Exop spec to have the server (optionally?)
 * respond with the final authzid (saving us a roundtrip as well).
 */
int
request_bind( LloadConnection *client, LloadOperation *op )
{
    LloadConnection *upstream = NULL;
    BerElement *ber, *copy;
    struct berval binddn, auth, mech = BER_BVNULL;
    ber_int_t version;
    ber_tag_t tag;
    unsigned long pin;
    int res, rc = LDAP_SUCCESS;

    CONNECTION_LOCK(client);
    pin = client->c_pin_id;

    if ( pin ) {
        LloadOperation *pinned_op, needle = {
            .o_client_connid = client->c_connid,
            .o_client_msgid = 0,
            .o_pin_id = client->c_pin_id,
        };

        Debug( LDAP_DEBUG_CONNS, "request_bind: "
                "client connid=%lu is pinned pin=%lu\n",
                client->c_connid, pin );

        pinned_op =
                ldap_tavl_delete( &client->c_ops, &needle, operation_client_cmp );
        if ( pinned_op ) {
            assert( op->o_tag == pinned_op->o_tag );

            pinned_op->o_client_msgid = op->o_client_msgid;

            /* Preserve the new BerElement and its pointers, reclaim the old
             * one in operation_destroy_from_client if it's still there */
            needle.o_ber = pinned_op->o_ber;
            pinned_op->o_ber = op->o_ber;
            op->o_ber = needle.o_ber;

            pinned_op->o_request = op->o_request;
            pinned_op->o_ctrls = op->o_ctrls;

            /* No one has seen this operation yet, plant the pin back in its stead */
            client->c_n_ops_executing--;
            op->o_res = LLOAD_OP_COMPLETED;
            ldap_tavl_delete( &client->c_ops, op, operation_client_cmp );
            op->o_client = NULL;
            assert( op->o_upstream == NULL );

            rc = ldap_tavl_insert( &client->c_ops, pinned_op, operation_client_cmp,
                    ldap_avl_dup_error );
            assert( rc == LDAP_SUCCESS );

            /* No one has seen this operation yet */
            op->o_refcnt--;
            operation_destroy( op );

            /* We didn't start a new operation, just continuing an existing one */
            lload_stats.counters[LLOAD_STATS_OPS_BIND].lc_ops_received--;

            op = pinned_op;
        }
    }

    ldap_tavl_delete( &client->c_ops, op, operation_client_cmp );
    client->c_n_ops_executing--;

    client_reset( client );

    client->c_state = LLOAD_C_BINDING;
    client->c_type = LLOAD_C_OPEN;

    if ( (copy = ber_alloc()) == NULL ) {
        goto fail;
    }
    ber_init2( copy, &op->o_request, 0 );

    tag = ber_get_int( copy, &version );
    if ( tag == LBER_ERROR ) {
        Debug( LDAP_DEBUG_PACKETS, "request_bind: "
                "failed to parse version field\n" );
        goto fail;
    } else if ( version != LDAP_VERSION3 ) {
        CONNECTION_UNLOCK(client);
        operation_send_reject(
                op, LDAP_PROTOCOL_ERROR, "LDAP version unsupported", 1 );
        CONNECTION_LOCK(client);
        goto fail;
    }

    tag = ber_get_stringbv( copy, &binddn, LBER_BV_NOTERM );
    if ( tag == LBER_ERROR ) {
        Debug( LDAP_DEBUG_PACKETS, "request_bind: "
                "failed to parse bind name field\n" );
        goto fail;
    }

    if ( !BER_BVISNULL( &client->c_auth ) ) {
        ch_free( client->c_auth.bv_val );
        BER_BVZERO( &client->c_auth );
    }

    tag = ber_skip_element( copy, &auth );
    if ( tag == LDAP_AUTH_SIMPLE ) {
        if ( !BER_BVISEMPTY( &binddn ) ) {
            char *ptr;
            client->c_auth.bv_len = STRLENOF("dn:") + binddn.bv_len;
            client->c_auth.bv_val = ch_malloc( client->c_auth.bv_len + 1 );

            ptr = lutil_strcopy( client->c_auth.bv_val, "dn:" );
            ptr = lutil_strncopy( ptr, binddn.bv_val, binddn.bv_len );
            *ptr = '\0';
        }

        if ( !BER_BVISNULL( &client->c_sasl_bind_mech ) ) {
            ber_memfree( client->c_sasl_bind_mech.bv_val );
            BER_BVZERO( &client->c_sasl_bind_mech );
        }
    } else if ( tag == LDAP_AUTH_SASL ) {
        ber_init2( copy, &auth, 0 );

        if ( ber_get_stringbv( copy, &mech, LBER_BV_NOTERM ) == LBER_ERROR ) {
            goto fail;
        }
        if ( !ber_bvcmp( &mech, &mech_external ) ) {
            struct berval credentials = BER_BVNULL;

            ber_get_stringbv( copy, &credentials, LBER_BV_NOTERM );
            rc = bind_mech_external( client, op, &credentials );

            /* terminate the upstream side if client switched mechanisms */
            if ( pin ) {
                operation_abandon( op );
            }

            ber_free( copy, 0 );
            return rc;
        } else if ( BER_BVISNULL( &client->c_sasl_bind_mech ) ) {
            ber_dupbv( &client->c_sasl_bind_mech, &mech );
        } else if ( ber_bvcmp( &mech, &client->c_sasl_bind_mech ) ) {
            ber_bvreplace( &client->c_sasl_bind_mech, &mech );
        }
    } else {
        goto fail;
    }

    rc = ldap_tavl_insert( &client->c_ops, op, operation_client_cmp, ldap_avl_dup_error );
    assert( rc == LDAP_SUCCESS );
    client->c_n_ops_executing++;
    CONNECTION_UNLOCK(client);

    if ( pin ) {
        checked_lock( &op->o_link_mutex );
        upstream = op->o_upstream;
        checked_unlock( &op->o_link_mutex );

        if ( upstream ) {
            checked_lock( &upstream->c_io_mutex );
            CONNECTION_LOCK(upstream);
            if ( !IS_ALIVE( upstream, c_live ) ) {
                CONNECTION_UNLOCK(upstream);
                checked_unlock( &upstream->c_io_mutex );
                upstream = NULL;
            }
        }
    }

    /* If we were pinned but lost the link, don't look for a new upstream, we
     * have to reject the op and clear pin */
    if ( upstream ) {
        /* No need to do anything */
    } else if ( !pin ) {
        upstream = backend_select( op, &res );
    } else {
        Debug( LDAP_DEBUG_STATS, "request_bind: "
                "connid=%lu, msgid=%d pinned upstream lost\n",
                op->o_client_connid, op->o_client_msgid );
        operation_send_reject( op, LDAP_OTHER,
                "connection to the remote server has been severed", 1 );
        pin = 0;
        goto done;
    }

    if ( !upstream ) {
        Debug( LDAP_DEBUG_STATS, "request_bind: "
                "connid=%lu, msgid=%d no available connection found\n",
                op->o_client_connid, op->o_client_msgid );
        operation_send_reject( op, res, "no connections available", 1 );
        assert( client->c_pin_id == 0 );
        goto done;
    }
    assert_locked( &upstream->c_io_mutex );
    /*
     * At this point, either:
     * - upstream is READY and pin == 0
     * - upstream is BINDING, pin != 0 and op->o_upstream_msgid == 0
     *
     * A pinned upstream we marked for closing at some point ago should have
     * closed by now.
     */

    ber = upstream->c_pendingber;
    if ( ber == NULL && (ber = ber_alloc()) == NULL ) {
        checked_unlock( &upstream->c_io_mutex );
        if ( !pin ) {
            LloadBackend *b = upstream->c_backend;

            upstream->c_n_ops_executing--;
            CONNECTION_UNLOCK(upstream);

            checked_lock( &b->b_mutex );
            b->b_n_ops_executing--;
            operation_update_backend_counters( op, b );
            checked_unlock( &b->b_mutex );
        } else {
            CONNECTION_UNLOCK(upstream);
        }

        Debug( LDAP_DEBUG_ANY, "request_bind: "
                "ber_alloc failed\n" );

        operation_unlink( op );

        CONNECTION_LOCK(client);
        goto fail;
    }
    upstream->c_pendingber = ber;

    if ( !pin ) {
        lload_stats.counters[LLOAD_STATS_OPS_BIND].lc_ops_forwarded++;
    }

    if ( pin ) {
        ldap_tavl_delete( &upstream->c_ops, op, operation_upstream_cmp );
        if ( tag == LDAP_AUTH_SIMPLE ) {
            pin = op->o_pin_id = 0;
        }
    } else if ( tag == LDAP_AUTH_SASL && !op->o_pin_id ) {
        checked_lock( &lload_pin_mutex );
        pin = op->o_pin_id = lload_next_pin++;
        Debug( LDAP_DEBUG_CONNS, "request_bind: "
                "client connid=%lu allocated pin=%lu linking it to upstream "
                "connid=%lu\n",
                op->o_client_connid, pin, upstream->c_connid );
        checked_unlock( &lload_pin_mutex );
    }

    op->o_upstream = upstream;
    op->o_upstream_connid = upstream->c_connid;
    op->o_upstream_msgid = upstream->c_next_msgid++;
    op->o_res = LLOAD_OP_FAILED;

    /* Was it unlinked in the meantime? No need to send a response since the
     * client is dead */
    if ( !IS_ALIVE( op, o_refcnt ) ) {
        LloadBackend *b = upstream->c_backend;

        upstream->c_n_ops_executing--;
        checked_unlock( &upstream->c_io_mutex );
        CONNECTION_UNLOCK(upstream);

        checked_lock( &b->b_mutex );
        b->b_n_ops_executing--;
        checked_unlock( &b->b_mutex );

        assert( !IS_ALIVE( client, c_live ) );
        checked_lock( &op->o_link_mutex );
        if ( op->o_upstream ) {
            op->o_upstream = NULL;
        }
        checked_unlock( &op->o_link_mutex );
        rc = -1;
        goto done;
    }

    if ( BER_BVISNULL( &mech ) ) {
        if ( !BER_BVISNULL( &upstream->c_sasl_bind_mech ) ) {
            ber_memfree( upstream->c_sasl_bind_mech.bv_val );
            BER_BVZERO( &upstream->c_sasl_bind_mech );
        }
    } else if ( ber_bvcmp( &upstream->c_sasl_bind_mech, &mech ) ) {
        ber_bvreplace( &upstream->c_sasl_bind_mech, &mech );
    }

    Debug( LDAP_DEBUG_TRACE, "request_bind: "
            "added bind from client connid=%lu to upstream connid=%lu "
            "as msgid=%d\n",
            op->o_client_connid, op->o_upstream_connid, op->o_upstream_msgid );
    if ( ldap_tavl_insert( &upstream->c_ops, op, operation_upstream_cmp,
                 ldap_avl_dup_error ) ) {
        assert(0);
    }
    upstream->c_state = LLOAD_C_BINDING;
    CONNECTION_UNLOCK(upstream);

#ifdef LDAP_API_FEATURE_VERIFY_CREDENTIALS
    if ( lload_features & LLOAD_FEATURE_VC ) {
        rc = client_bind_as_vc( op, upstream, &binddn, tag, &auth );
    } else
#endif /* LDAP_API_FEATURE_VERIFY_CREDENTIALS */
    {
        rc = client_bind( op, upstream, &binddn, tag, &auth );
    }
    checked_unlock( &upstream->c_io_mutex );

done:

    CONNECTION_LOCK(client);
    if ( rc == LDAP_SUCCESS ) {
        client->c_pin_id = pin;
        CONNECTION_UNLOCK(client);

        if ( upstream ) {
            connection_write_cb( -1, 0, upstream );
        }
    } else {
fail:
        rc = -1;

        client->c_pin_id = 0;
        CONNECTION_DESTROY(client);
    }

    ber_free( copy, 0 );
    return rc;
}

/*
 * Remember the response, but first ask the server what
 * authorization identity has been negotiated.
 *
 * Also, this request will fail if the server thinks a SASL
 * confidentiality/integrity layer has been negotiated so we catch
 * it early and no other clients are affected.
 */
int
finish_sasl_bind(
        LloadConnection *upstream,
        LloadOperation *op,
        BerElement *ber )
{
    BerElement *output;
    LloadOperation *removed;
    ber_int_t msgid;
    int rc;

    CONNECTION_ASSERT_LOCKED(upstream);
    removed = ldap_tavl_delete( &upstream->c_ops, op, operation_upstream_cmp );
    if ( !removed ) {
        assert( upstream->c_state != LLOAD_C_BINDING );
        /* FIXME: has client replaced this bind since? */
        assert(0);
    }
    assert( removed == op && upstream->c_state == LLOAD_C_BINDING );

    CONNECTION_UNLOCK(upstream);

    checked_lock( &upstream->c_io_mutex );
    output = upstream->c_pendingber;
    if ( output == NULL && (output = ber_alloc()) == NULL ) {
        checked_unlock( &upstream->c_io_mutex );
        CONNECTION_LOCK_DESTROY(upstream);
        return -1;
    }
    upstream->c_pendingber = output;

    msgid = upstream->c_next_msgid++;
    ber_printf( output, "t{tit{ts}}", LDAP_TAG_MESSAGE,
            LDAP_TAG_MSGID, msgid,
            LDAP_REQ_EXTENDED,
            LDAP_TAG_EXOP_REQ_OID, LDAP_EXOP_WHO_AM_I );

    /* Make sure noone flushes the buffer before we re-insert the operation */
    CONNECTION_LOCK(upstream);
    checked_unlock( &upstream->c_io_mutex );

    op->o_upstream_msgid = msgid;

    /* remember the response for later */
    ber_free( op->o_ber, 1 );
    op->o_ber = ber;

    /* Could we have been unlinked in the meantime? */
    rc = ldap_tavl_insert(
            &upstream->c_ops, op, operation_upstream_cmp, ldap_avl_dup_error );
    assert( rc == LDAP_SUCCESS );

    CONNECTION_UNLOCK(upstream);

    Debug( LDAP_DEBUG_TRACE, "finish_sasl_bind: "
            "SASL exchange in lieu of client connid=%lu to upstream "
            "connid=%lu finished, resolving final authzid name msgid=%d\n",
            op->o_client_connid, op->o_upstream_connid, op->o_upstream_msgid );

    connection_write_cb( -1, 0, upstream );
    return LDAP_SUCCESS;
}

int
handle_bind_response(
        LloadConnection *client,
        LloadOperation *op,
        BerElement *ber )
{
    LloadConnection *upstream;
    BerValue response;
    BerElement *copy;
    LloadOperation *removed;
    ber_int_t result;
    ber_tag_t tag;
    int rc = LDAP_SUCCESS;

    if ( (copy = ber_alloc()) == NULL ) {
        rc = -1;
        goto done;
    }

    tag = ber_peek_element( ber, &response );
    assert( tag == LDAP_RES_BIND );

    ber_init2( copy, &response, 0 );

    tag = ber_get_enum( copy, &result );
    ber_free( copy, 0 );

    if ( tag == LBER_ERROR ) {
        rc = -1;
        goto done;
    }

    Debug( LDAP_DEBUG_STATS, "handle_bind_response: "
            "received response for bind request msgid=%d by client "
            "connid=%lu, result=%d\n",
            op->o_client_msgid, op->o_client_connid, result );

    checked_lock( &op->o_link_mutex );
    upstream = op->o_upstream;
    checked_unlock( &op->o_link_mutex );
    if ( !upstream ) {
        return LDAP_SUCCESS;
    }

    CONNECTION_LOCK(upstream);
    if ( !ldap_tavl_find( upstream->c_ops, op, operation_upstream_cmp ) ) {
        /*
         * operation might not be found because:
         * - it has timed out (only happens when debugging/hung/...)
         *   a response has been sent for us, we must not send another
         * - it has been abandoned (new bind, unbind)
         *   no response is expected
         * - ???
         */
        CONNECTION_UNLOCK(upstream);
        return LDAP_SUCCESS;
    }

    /*
     * We might be marked for closing, forward the response if we can, but do
     * no more if it's a SASL bind - just finish the operation and send failure
     * in that case (since we can't resolve the bind identity correctly).
     */
    if ( upstream->c_state == LLOAD_C_CLOSING ) {
        /* FIXME: this is too ad-hoc */
        if ( ( result == LDAP_SUCCESS ||
                     result == LDAP_SASL_BIND_IN_PROGRESS ) &&
                !BER_BVISNULL( &upstream->c_sasl_bind_mech ) ) {
            CONNECTION_UNLOCK(upstream);
            operation_send_reject(
                    op, LDAP_OTHER, "upstream connection is closing", 0 );

            ber_free( ber, 1 );
            return LDAP_SUCCESS;
        }

        assert( op->o_client_msgid && op->o_upstream_msgid );
        op->o_pin_id = 0;

    } else if ( result == LDAP_SASL_BIND_IN_PROGRESS ) {
        ldap_tavl_delete( &upstream->c_ops, op, operation_upstream_cmp );
        op->o_upstream_msgid = 0;
        rc = ldap_tavl_insert(
                &upstream->c_ops, op, operation_upstream_cmp, ldap_avl_dup_error );
        assert( rc == LDAP_SUCCESS );
    } else {
        int sasl_finished = 0;
        if ( !BER_BVISNULL( &upstream->c_sasl_bind_mech ) ) {
            sasl_finished = 1;
            ber_memfree( upstream->c_sasl_bind_mech.bv_val );
            BER_BVZERO( &upstream->c_sasl_bind_mech );
        }

        assert( op->o_client_msgid && op->o_upstream_msgid );
        op->o_pin_id = 0;

        if ( (lload_features & LLOAD_FEATURE_PROXYAUTHZ) && sasl_finished &&
                result == LDAP_SUCCESS ) {
            return finish_sasl_bind( upstream, op, ber );
        }
        op->o_res = LLOAD_OP_COMPLETED;
    }
    CONNECTION_UNLOCK(upstream);

    if ( !op->o_pin_id ) {
        operation_unlink_upstream( op, upstream );
    }

    CONNECTION_LOCK(client);
    removed = ldap_tavl_delete( &client->c_ops, op, operation_client_cmp );
    assert( !removed || op == removed );

    if ( client->c_state == LLOAD_C_BINDING ) {
        assert( removed );
        switch ( result ) {
            case LDAP_SASL_BIND_IN_PROGRESS:
                op->o_saved_msgid = op->o_client_msgid;
                op->o_client_msgid = 0;
                rc = ldap_tavl_insert( &client->c_ops, op, operation_client_cmp,
                        ldap_avl_dup_error );
                assert( rc == LDAP_SUCCESS );
                break;
            case LDAP_SUCCESS:
            default: {
                client->c_state = LLOAD_C_READY;
                client->c_type = LLOAD_C_OPEN;
                client->c_pin_id = 0;
                client->c_n_ops_executing--;
                if ( !BER_BVISNULL( &client->c_auth ) ) {
                    if ( result != LDAP_SUCCESS ) {
                        ber_memfree( client->c_auth.bv_val );
                        BER_BVZERO( &client->c_auth );
                    } else if ( !ber_bvstrcasecmp(
                                        &client->c_auth, &lloadd_identity ) ) {
                        client->c_type = LLOAD_C_PRIVILEGED;
                    }
                }
                if ( !BER_BVISNULL( &client->c_sasl_bind_mech ) ) {
                    ber_memfree( client->c_sasl_bind_mech.bv_val );
                    BER_BVZERO( &client->c_sasl_bind_mech );
                }
                break;
            }
        }
    } else {
        if ( removed ) {
            client->c_n_ops_executing--;
        }
        assert( client->c_state == LLOAD_C_DYING ||
                client->c_state == LLOAD_C_CLOSING );
    }
    CONNECTION_UNLOCK(client);

done:
    if ( rc ) {
        operation_send_reject( op, LDAP_OTHER, "internal error", 1 );

        ber_free( ber, 1 );
        return LDAP_SUCCESS;
    }
    return forward_final_response( client, op, ber );
}

int
handle_whoami_response(
        LloadConnection *client,
        LloadOperation *op,
        BerElement *ber )
{
    LloadConnection *upstream;
    BerValue matched, diagmsg;
    BerElement *saved_response = op->o_ber;
    LloadOperation *removed;
    ber_int_t result;
    ber_tag_t tag;
    ber_len_t len;

    Debug( LDAP_DEBUG_TRACE, "handle_whoami_response: "
            "connid=%ld received whoami response in lieu of connid=%ld\n",
            op->o_upstream_connid, client->c_connid );

    tag = ber_scanf( ber, "{emm" /* "}" */,
            &result, &matched, &diagmsg );
    if ( tag == LBER_ERROR ) {
        operation_send_reject( op, LDAP_OTHER, "upstream protocol error", 0 );
        return -1;
    }

    checked_lock( &op->o_link_mutex );
    upstream = op->o_upstream;
    checked_unlock( &op->o_link_mutex );
    if ( !upstream ) {
        return LDAP_SUCCESS;
    }

    op->o_res = LLOAD_OP_COMPLETED;
    /* Clear upstream status */
    operation_unlink_upstream( op, upstream );

    if ( result == LDAP_PROTOCOL_ERROR ) {
        LloadBackend *b;

        CONNECTION_LOCK(upstream);
        b = upstream->c_backend;
        Debug( LDAP_DEBUG_ANY, "handle_whoami_response: "
                "Who Am I? extended operation not supported on backend %s, "
                "proxyauthz with clients that do SASL binds will not work "
                "msg=%s!\n",
                b->b_uri.bv_val, diagmsg.bv_val );
        CONNECTION_UNLOCK(upstream);
        operation_send_reject( op, LDAP_OTHER, "upstream protocol error", 0 );
        return -1;
    }

    tag = ber_peek_tag( ber, &len );

    CONNECTION_LOCK(client);

    assert( client->c_state == LLOAD_C_BINDING ||
            client->c_state == LLOAD_C_CLOSING );

    assert( BER_BVISNULL( &client->c_auth ) );
    if ( !BER_BVISNULL( &client->c_auth ) ) {
        ber_memfree( client->c_auth.bv_val );
        BER_BVZERO( &client->c_auth );
    }

    if ( tag == LDAP_TAG_EXOP_RES_VALUE ) {
        tag = ber_scanf( ber, "o", &client->c_auth );
        if ( tag == LBER_ERROR ) {
            CONNECTION_DESTROY(client);
            return -1;
        }
    }

    removed = ldap_tavl_delete( &client->c_ops, op, operation_client_cmp );
    assert( !removed || op == removed );
    op->o_pin_id = 0;
    if ( removed ) {
        client->c_n_ops_executing--;
    }

    Debug( LDAP_DEBUG_TRACE, "handle_whoami_response: "
            "connid=%ld new authid=%s\n",
            client->c_connid, client->c_auth.bv_val );

    if ( client->c_state == LLOAD_C_BINDING ) {
        client->c_state = LLOAD_C_READY;
        client->c_type = LLOAD_C_OPEN;
        client->c_pin_id = 0;
        if ( !BER_BVISNULL( &client->c_auth ) &&
                !ber_bvstrcasecmp( &client->c_auth, &lloadd_identity ) ) {
            client->c_type = LLOAD_C_PRIVILEGED;
        }
        if ( !BER_BVISNULL( &client->c_sasl_bind_mech ) ) {
            ber_memfree( client->c_sasl_bind_mech.bv_val );
            BER_BVZERO( &client->c_sasl_bind_mech );
        }
    }

    CONNECTION_UNLOCK(client);

    /* defer the disposal of ber to operation_destroy */
    op->o_ber = ber;

    return forward_final_response( client, op, saved_response );
}

#ifdef LDAP_API_FEATURE_VERIFY_CREDENTIALS
int
handle_vc_bind_response(
        LloadConnection *client,
        LloadOperation *op,
        BerElement *ber )
{
    BerElement *output;
    BerValue matched, diagmsg, creds = BER_BVNULL, controls = BER_BVNULL;
    ber_int_t result;
    ber_tag_t tag;
    ber_len_t len;
    int rc = 0;

    tag = ber_scanf( ber, "{emm" /* "}" */,
            &result, &matched, &diagmsg );
    if ( tag == LBER_ERROR ) {
        rc = -1;
        goto done;
    }

    tag = ber_peek_tag( ber, &len );
    if ( result == LDAP_PROTOCOL_ERROR ) {
        LloadConnection *upstream;

        checked_lock( &op->o_link_mutex );
        upstream = op->o_upstream;
        checked_unlock( &op->o_link_mutex );
        if ( upstream ) {
            LloadBackend *b;

            CONNECTION_LOCK(upstream);
            b = upstream->c_backend;
            Debug( LDAP_DEBUG_ANY, "handle_vc_bind_response: "
                    "VC extended operation not supported on backend %s\n",
                    b->b_uri.bv_val );
            CONNECTION_UNLOCK(upstream);
        }
    }

    Debug( LDAP_DEBUG_STATS, "handle_vc_bind_response: "
            "received response for bind request msgid=%d by client "
            "connid=%lu, result=%d\n",
            op->o_client_msgid, op->o_client_connid, result );

    CONNECTION_LOCK(client);

    if ( tag == LDAP_TAG_EXOP_VERIFY_CREDENTIALS_COOKIE ) {
        if ( !BER_BVISNULL( &client->c_vc_cookie ) ) {
            ber_memfree( client->c_vc_cookie.bv_val );
        }
        tag = ber_scanf( ber, "o", &client->c_vc_cookie );
        if ( tag == LBER_ERROR ) {
            rc = -1;
            CONNECTION_UNLOCK(client);
            goto done;
        }
        tag = ber_peek_tag( ber, &len );
    }

    if ( tag == LDAP_TAG_EXOP_VERIFY_CREDENTIALS_SCREDS ) {
        tag = ber_scanf( ber, "m", &creds );
        if ( tag == LBER_ERROR ) {
            rc = -1;
            CONNECTION_UNLOCK(client);
            goto done;
        }
        tag = ber_peek_tag( ber, &len );
    }

    if ( tag == LDAP_TAG_EXOP_VERIFY_CREDENTIALS_CONTROLS ) {
        tag = ber_scanf( ber, "m", &controls );
        if ( tag == LBER_ERROR ) {
            rc = -1;
            CONNECTION_UNLOCK(client);
            goto done;
        }
    }

    if ( client->c_state == LLOAD_C_BINDING ) {
        switch ( result ) {
            case LDAP_SASL_BIND_IN_PROGRESS:
                break;
            case LDAP_SUCCESS:
            default: {
                client->c_state = LLOAD_C_READY;
                client->c_type = LLOAD_C_OPEN;
                client->c_pin_id = 0;
                if ( result != LDAP_SUCCESS ) {
                    ber_memfree( client->c_auth.bv_val );
                    BER_BVZERO( &client->c_auth );
                } else if ( !ber_bvstrcasecmp(
                                    &client->c_auth, &lloadd_identity ) ) {
                    client->c_type = LLOAD_C_PRIVILEGED;
                }
                if ( !BER_BVISNULL( &client->c_vc_cookie ) ) {
                    ber_memfree( client->c_vc_cookie.bv_val );
                    BER_BVZERO( &client->c_vc_cookie );
                }
                if ( !BER_BVISNULL( &client->c_sasl_bind_mech ) ) {
                    ber_memfree( client->c_sasl_bind_mech.bv_val );
                    BER_BVZERO( &client->c_sasl_bind_mech );
                }
                break;
            }
        }
    } else {
        assert( client->c_state == LLOAD_C_INVALID ||
                client->c_state == LLOAD_C_CLOSING );
    }
    CONNECTION_UNLOCK(client);

    checked_lock( &client->c_io_mutex );
    output = client->c_pendingber;
    if ( output == NULL && (output = ber_alloc()) == NULL ) {
        rc = -1;
        checked_unlock( &client->c_io_mutex );
        goto done;
    }
    client->c_pendingber = output;

    rc = ber_printf( output, "t{tit{eOOtO}tO}", LDAP_TAG_MESSAGE,
            LDAP_TAG_MSGID, op->o_client_msgid, LDAP_RES_BIND,
            result, &matched, &diagmsg,
            LDAP_TAG_SASL_RES_CREDS, BER_BV_OPTIONAL( &creds ),
            LDAP_TAG_CONTROLS, BER_BV_OPTIONAL( &controls ) );

    checked_unlock( &client->c_io_mutex );
    if ( rc >= 0 ) {
        connection_write_cb( -1, 0, client );
        rc = 0;
    }

done:
    operation_unlink( op );
    ber_free( ber, 1 );
    return rc;
}
#endif /* LDAP_API_FEATURE_VERIFY_CREDENTIALS */
