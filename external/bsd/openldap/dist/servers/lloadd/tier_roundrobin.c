/*	$NetBSD: tier_roundrobin.c,v 1.2 2025/09/05 21:16:24 christos Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: tier_roundrobin.c,v 1.2 2025/09/05 21:16:24 christos Exp $");

#include "portable.h"

#include "lload.h"

static LloadTierInit roundrobin_init;
static LloadTierBackendCb roundrobin_add_backend;
static LloadTierBackendCb roundrobin_remove_backend;
static LloadTierSelect roundrobin_select;

struct lload_tier_type roundrobin_tier;

static LloadTier *
roundrobin_init( void )
{
    LloadTier *tier;

    tier = ch_calloc( 1, sizeof(LloadTier) );

    tier->t_type = roundrobin_tier;
    ldap_pvt_thread_mutex_init( &tier->t_mutex );
    LDAP_CIRCLEQ_INIT( &tier->t_backends );

    return tier;
}

static int
roundrobin_add_backend( LloadTier *tier, LloadBackend *b )
{
    assert( b->b_tier == tier );
    LDAP_CIRCLEQ_INSERT_TAIL( &tier->t_backends, b, b_next );
    if ( !tier->t_private ) {
        tier->t_private = b;
    }
    tier->t_nbackends++;
    return LDAP_SUCCESS;
}

static int
roundrobin_remove_backend( LloadTier *tier, LloadBackend *b )
{
    LloadBackend *next = LDAP_CIRCLEQ_LOOP_NEXT( &tier->t_backends, b, b_next );

    assert_locked( &tier->t_mutex );
    assert_locked( &b->b_mutex );

    assert( b->b_tier == tier );

    LDAP_CIRCLEQ_REMOVE( &tier->t_backends, b, b_next );
    if ( b == tier->t_private ) {
        if ( tier->t_nbackends ) {
            tier->t_private = next;
        } else {
            assert( b == next );
            tier->t_private = NULL;
        }
    }
    tier->t_nbackends--;
    return LDAP_SUCCESS;
}

static int
roundrobin_select(
        LloadTier *tier,
        LloadOperation *op,
        LloadConnection **cp,
        int *res,
        char **message )
{
    LloadBackend *b, *first, *next;
    int rc = 0;

    checked_lock( &tier->t_mutex );
    first = b = tier->t_private;
    checked_unlock( &tier->t_mutex );

    if ( !first ) return rc;

    do {
        int result;

        checked_lock( &b->b_mutex );
        next = LDAP_CIRCLEQ_LOOP_NEXT( &tier->t_backends, b, b_next );

        result = backend_select( b, op, cp, res, message );
        checked_unlock( &b->b_mutex );

        rc |= result;
        if ( result && *cp ) {
            /*
             * Round-robin step:
             * Rotate the queue to put this backend at the end. The race here
             * is acceptable.
             */
            checked_lock( &tier->t_mutex );
            tier->t_private = next;
            checked_unlock( &tier->t_mutex );
            return rc;
        }

        b = next;
    } while ( b != first );

    return rc;
}

struct lload_tier_type roundrobin_tier = {
        .tier_name = "roundrobin",

        .tier_init = roundrobin_init,
        .tier_startup = tier_startup,
        .tier_reset = tier_reset,
        .tier_destroy = tier_destroy,

        .tier_oc = BER_BVC("olcBkLloadTierConfig"),
        .tier_backend_oc = BER_BVC("olcBkLloadBackendConfig"),

        .tier_add_backend = roundrobin_add_backend,
        .tier_remove_backend = roundrobin_remove_backend,

        .tier_select = roundrobin_select,
};
