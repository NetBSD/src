/*	$NetBSD: tier_weighted.c,v 1.1.1.1 2025/09/05 21:09:46 christos Exp $	*/

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
__RCSID("$NetBSD: tier_weighted.c,v 1.1.1.1 2025/09/05 21:09:46 christos Exp $");

#include "portable.h"

#include <ac/string.h>

#include "lload.h"
#include "lutil.h"

static LloadTierInit weighted_init;
static LloadTierBackendCb weighted_add_backend;
static LloadTierBackendCb weighted_remove_backend;
static LloadTierSelect weighted_select;

struct lload_tier_type weighted_tier;

/*
 * Linear Congruential Generator - we don't need
 * high quality randomness, and we don't want to
 * interfere with anyone else's use of srand().
 *
 * The PRNG here cycles thru 941,955 numbers.
 */
static float weighted_seed;

static void
weighted_srand( int seed )
{
    weighted_seed = (float)seed / (float)RAND_MAX;
}

static float
weighted_rand()
{
    float val = 9821.0 * weighted_seed + .211327;
    weighted_seed = val - (int)val;
    return weighted_seed;
}

static void
weighted_shuffle( LloadBackend **b, int n )
{
    int i, j, p;
    uintptr_t total = 0, r;

    for ( i = 0; i < n; i++ )
        total += b[i]->b_weight;

    /* all weights are zero, do a straight Fisher-Yates shuffle */
    if ( !total ) {
        while ( n ) {
            LloadBackend *t;
            i = weighted_rand() * n--;
            t = b[n];
            b[n] = b[i];
            b[i] = t;
        }
        return;
    }

    /* Do a shuffle per RFC2782 Page 4 */
    p = n;
    for ( i = 0; i < n - 1; i++ ) {
        r = weighted_rand() * total;
        for ( j = 0; j < p; j++ ) {
            r -= b[j]->b_weight;
            if ( r <= 0 ) {
                if ( j ) {
                    LloadBackend *t = b[0];
                    b[0] = b[j];
                    b[j] = t;
                }
                total -= b[0]->b_weight;
                b++;
                p--;
                break;
            }
        }
        /* TODO: once we have total == 0, should we jump over to the previous
         * case? */
    }
}

LloadTier *
weighted_init( void )
{
    LloadTier *tier;

    tier = ch_calloc( 1, sizeof(LloadTier) );

    tier->t_type = weighted_tier;
    ldap_pvt_thread_mutex_init( &tier->t_mutex );
    LDAP_CIRCLEQ_INIT( &tier->t_backends );

    weighted_srand( rand() );

    return tier;
}

int
weighted_add_backend( LloadTier *tier, LloadBackend *to_add )
{
    LloadBackend *b;
    uintptr_t added = 1;

    assert( to_add->b_tier == tier );

    /* This requires us to use LDAP_CIRCLEQ_ENTRY_INIT() every time we have
     * removed the backend from the list */
    if ( LDAP_CIRCLEQ_NEXT( to_add, b_next ) ) {
        added = 0;
        LDAP_CIRCLEQ_REMOVE( &tier->t_backends, to_add, b_next );
    }

    /*
     * Keep it sorted. The only thing RFC 2782 specifies is that weight 0
     * entries are at the front of the list so they have a chance to be
     * selected.
     *
     * Even with that in mind, there is a problem outlined in the RFC 2782
     * errata[0] where the ordering affects the likelihood of an entry being
     * selected with weight 0 entries in the mix - they are an afterthought
     * into the design after all.
     *
     * [0]. https://www.rfc-editor.org/errata/eid2984
     */
    LDAP_CIRCLEQ_FOREACH ( b, &tier->t_backends, b_next ) {
        if ( to_add->b_weight < b->b_weight ) {
            LDAP_CIRCLEQ_INSERT_BEFORE( &tier->t_backends, b, to_add, b_next );
            goto done;
        }
    }
    LDAP_CIRCLEQ_INSERT_TAIL( &tier->t_backends, to_add, b_next );

done:
    tier->t_nbackends += added;
    return LDAP_SUCCESS;
}

static int
weighted_remove_backend( LloadTier *tier, LloadBackend *b )
{
    assert_locked( &tier->t_mutex );
    assert_locked( &b->b_mutex );

    assert( b->b_tier == tier );
    assert( tier->t_nbackends );

    LDAP_CIRCLEQ_REMOVE( &tier->t_backends, b, b_next );
    LDAP_CIRCLEQ_ENTRY_INIT( b, b_next );
    tier->t_nbackends--;

    return LDAP_SUCCESS;
}

int
weighted_select(
        LloadTier *tier,
        LloadOperation *op,
        LloadConnection **cp,
        int *res,
        char **message )
{
    LloadBackend *b, **sorted;
    int rc = 0, i = 0;

    if ( !tier->t_nbackends ) return rc;

    sorted = ch_malloc( tier->t_nbackends * sizeof(LloadBackend *) );

    LDAP_CIRCLEQ_FOREACH ( b, &tier->t_backends, b_next ) {
        sorted[i++] = b;
    }

    assert( i == tier->t_nbackends );

    weighted_shuffle( sorted, tier->t_nbackends );

    for ( i = 0; i < tier->t_nbackends; i++ ) {
        int result;

        checked_lock( &sorted[i]->b_mutex );
        result = backend_select( sorted[i], op, cp, res, message );
        checked_unlock( &sorted[i]->b_mutex );

        rc |= result;
        if ( result && *cp ) {
            break;
        }
    }

    ch_free( sorted );
    return rc;
}

struct lload_tier_type weighted_tier = {
        .tier_name = "weighted",

        .tier_init = weighted_init,
        .tier_startup = tier_startup,
        .tier_reset = tier_reset,
        .tier_destroy = tier_destroy,

        .tier_oc = BER_BVC("olcBkLloadTierConfig"),
        .tier_backend_oc = BER_BVC("olcBkLloadBackendConfig"),

        .tier_add_backend = weighted_add_backend,
        .tier_remove_backend = weighted_remove_backend,

        .tier_select = weighted_select,
};
