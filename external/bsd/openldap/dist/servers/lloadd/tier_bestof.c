/*	$NetBSD: tier_bestof.c,v 1.2 2025/09/05 21:16:24 christos Exp $	*/

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
__RCSID("$NetBSD: tier_bestof.c,v 1.2 2025/09/05 21:16:24 christos Exp $");

#include "portable.h"

#include <ac/string.h>
#include <math.h>

#include "lload.h"
#include "lutil.h"

static LloadTierInit bestof_init;
static LloadTierBackendConfigCb bestof_backend_options;
static LloadTierBackendCb bestof_add_backend;
static LloadTierBackendCb bestof_remove_backend;
static LloadTierSelect bestof_select;

struct lload_tier_type bestof_tier;

/*
 * xorshift - we don't need high quality randomness, and we don't want to
 * interfere with anyone else's use of srand() but we still want something with
 * little bias.
 *
 * The PRNG here cycles thru 2^64−1 numbers.
 */
static uint64_t bestof_seed;

static void
bestof_srand( int seed )
{
    bestof_seed = seed;
}

static uint64_t
bestof_rand()
{
    uint64_t val = bestof_seed;
    val ^= val << 13;
    val ^= val >> 7;
    val ^= val << 17;
    bestof_seed = val;
    return val;
}

static int
bestof_cmp( const void *left, const void *right )
{
    const LloadBackend *l = left;
    const LloadBackend *r = right;
    struct timeval now;
    uintptr_t count, diff;
    float a = l->b_fitness, b = r->b_fitness, factor = 1;

    gettimeofday( &now, NULL );
    /* We assume this is less than a second after the last update */
    factor = 1 / ( pow( ( 1 / factor ) + 1, now.tv_usec / 1000000.0 ) - 1 );

    count = __atomic_load_n( &l->b_operation_count, __ATOMIC_RELAXED );
    diff = __atomic_load_n( &l->b_operation_time, __ATOMIC_RELAXED );
    if ( count ) {
        a = ( a * factor + (float)diff * l->b_weight / count ) / ( factor + 1 );
    }

    count = __atomic_load_n( &r->b_operation_count, __ATOMIC_RELAXED );
    diff = __atomic_load_n( &r->b_operation_time, __ATOMIC_RELAXED );
    if ( count ) {
        b = ( b * factor + (float)diff * r->b_weight / count ) / ( factor + 1 );
    }

    return (a - b < 0) ? -1 : (a - b == 0) ? 0 : 1;
}

LloadTier *
bestof_init( void )
{
    LloadTier *tier;
    int seed;

    tier = ch_calloc( 1, sizeof(LloadTier) );

    tier->t_type = bestof_tier;
    ldap_pvt_thread_mutex_init( &tier->t_mutex );
    LDAP_CIRCLEQ_INIT( &tier->t_backends );

    /* Make sure we don't pass 0 as a seed */
    do {
        seed = rand();
    } while ( !seed );
    bestof_srand( seed );

    return tier;
}

int
bestof_add_backend( LloadTier *tier, LloadBackend *b )
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
bestof_remove_backend( LloadTier *tier, LloadBackend *b )
{
    LloadBackend *next = LDAP_CIRCLEQ_LOOP_NEXT( &tier->t_backends, b, b_next );

    assert_locked( &tier->t_mutex );
    assert_locked( &b->b_mutex );

    assert( b->b_tier == tier );
    assert( tier->t_private );

    LDAP_CIRCLEQ_REMOVE( &tier->t_backends, b, b_next );
    LDAP_CIRCLEQ_ENTRY_INIT( b, b_next );

    if ( b == next ) {
        tier->t_private = NULL;
    } else {
        tier->t_private = next;
    }
    tier->t_nbackends--;

    return LDAP_SUCCESS;
}

static int
bestof_backend_options( LloadTier *tier, LloadBackend *b, char *arg )
{
    struct berval weight = BER_BVC("weight=");
    unsigned long l;

    if ( !strncasecmp( arg, weight.bv_val, weight.bv_len ) ) {
        if ( lutil_atoulx( &l, &arg[weight.bv_len], 0 ) != 0 ) {
            Debug( LDAP_DEBUG_ANY, "bestof_backend_options: "
                    "cannot parse %s as weight\n",
                    arg );
            return 1;
        }
        b->b_weight = l;
        return 0;
    }

    return 1;
}

static int
bestof_update( LloadTier *tier )
{
    LloadBackend *b, *first, *next;
    time_t now = slap_get_time();

    checked_lock( &tier->t_mutex );
    first = b = tier->t_private;
    checked_unlock( &tier->t_mutex );

    if ( !first ) return LDAP_SUCCESS;

    do {
        int steps;
        checked_lock( &b->b_mutex );

        steps = now - b->b_last_update;
        if ( b->b_weight && steps > 0 ) {
            uintptr_t count, diff;
            float factor = 1;

            count = __atomic_exchange_n(
                    &b->b_operation_count, 0, __ATOMIC_RELAXED );
            diff = __atomic_exchange_n(
                    &b->b_operation_time, 0, __ATOMIC_RELAXED );

            /* Smear values over time - rolling average */
            if ( count ) {
                float fitness = b->b_weight * diff;

                /* Stretch factor accordingly favouring the latest value */
                if ( steps > 10 ) {
                    factor = 0; /* No recent data */
                } else if ( steps > 1 ) {
                    factor = 1 / ( pow( ( 1 / factor ) + 1, steps ) - 1 );
                }

                b->b_fitness = ( factor * b->b_fitness + fitness / count ) /
                        ( factor + 1 );
                b->b_last_update = now;
            }
        }

        next = LDAP_CIRCLEQ_LOOP_NEXT( &tier->t_backends, b, b_next );
        checked_unlock( &b->b_mutex );
        b = next;
    } while ( b != first );

    return LDAP_SUCCESS;
}

int
bestof_select(
        LloadTier *tier,
        LloadOperation *op,
        LloadConnection **cp,
        int *res,
        char **message )
{
    LloadBackend *first, *next, *b, *b0, *b1;
    int result = 0, rc = 0, n = tier->t_nbackends;
    int i0, i1, i = 0;

    checked_lock( &tier->t_mutex );
    first = b0 = b = tier->t_private;
    checked_unlock( &tier->t_mutex );

    if ( !first ) return rc;

    if ( tier->t_nbackends == 1 ) {
        goto fallback;
    }

    /* Pick two backend indices at random */
    i0 = bestof_rand() % n;
    i1 = bestof_rand() % ( n - 1 );
    if ( i1 >= i0 ) {
        i1 += 1;
    } else {
        int tmp = i0;
        i0 = i1;
        i1 = tmp;
    }
    assert( i0 < i1 );

    /*
     * FIXME: use a static array in t_private so we don't have to do any of
     * this
     */
    for ( i = 0; i < i1; i++ ) {
        if ( i == i0 ) {
            b0 = b;
        }
        checked_lock( &b->b_mutex );
        next = LDAP_CIRCLEQ_LOOP_NEXT( &tier->t_backends, b, b_next );
        checked_unlock( &b->b_mutex );
        b = next;
    }
    b1 = b;
    assert( b0 != b1 );

    if ( bestof_cmp( b0, b1 ) < 0 ) {
        checked_lock( &b0->b_mutex );
        result = backend_select( b0, op, cp, res, message );
        checked_unlock( &b0->b_mutex );
    } else {
        checked_lock( &b1->b_mutex );
        result = backend_select( b1, op, cp, res, message );
        checked_unlock( &b1->b_mutex );
    }

    rc |= result;
    if ( result && *cp ) {
        checked_lock( &tier->t_mutex );
        tier->t_private = LDAP_CIRCLEQ_LOOP_NEXT(
                &tier->t_backends, (*cp)->c_backend, b_next );
        checked_unlock( &tier->t_mutex );
        return rc;
    }

    /* Preferred backends deemed unusable, do a round robin from scratch */
    b = first;
fallback:
    do {
        checked_lock( &b->b_mutex );
        next = LDAP_CIRCLEQ_LOOP_NEXT( &tier->t_backends, b, b_next );

        rc = backend_select( b, op, cp, res, message );
        checked_unlock( &b->b_mutex );

        if ( rc && *cp ) {
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

struct lload_tier_type bestof_tier = {
        .tier_name = "bestof",

        .tier_init = bestof_init,
        .tier_startup = tier_startup,
        .tier_update = bestof_update,
        .tier_reset = tier_reset,
        .tier_destroy = tier_destroy,

        .tier_oc = BER_BVC("olcBkLloadTierConfig"),
        .tier_backend_oc = BER_BVC("olcBkLloadBackendConfig"),

        .tier_add_backend = bestof_add_backend,
        .tier_remove_backend = bestof_remove_backend,

        .tier_select = bestof_select,
};
