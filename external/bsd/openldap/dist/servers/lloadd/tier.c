/*	$NetBSD: tier.c,v 1.2 2025/09/05 21:16:24 christos Exp $	*/

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
__RCSID("$NetBSD: tier.c,v 1.2 2025/09/05 21:16:24 christos Exp $");

#include "portable.h"

#include "lload.h"

lload_t_head tiers;

int
tier_startup( LloadTier *tier )
{
    LloadBackend *b;

    LDAP_CIRCLEQ_FOREACH ( b, &tier->t_backends, b_next ) {
        checked_lock( &b->b_mutex );
        if ( !b->b_retry_event ) {
            b->b_retry_event = evtimer_new( daemon_base, backend_connect, b );
            if ( !b->b_retry_event ) {
                Debug( LDAP_DEBUG_ANY, "tier_startup: "
                        "%s failed to allocate retry event\n",
                        tier->t_type.tier_name );
                return -1;
            }
        }
        backend_retry( b );
        checked_unlock( &b->b_mutex );
    }
    return LDAP_SUCCESS;
}

int
tier_reset( LloadTier *tier, int shutdown )
{
    LloadBackend *b;

    LDAP_CIRCLEQ_FOREACH ( b, &tier->t_backends, b_next ) {
        epoch_t epoch = epoch_join();

        checked_lock( &b->b_mutex );
        if ( shutdown ) {
            b->b_numconns = b->b_numbindconns = 0;
        }
        backend_reset( b, 1 );
        backend_retry( b );
        checked_unlock( &b->b_mutex );

        epoch_leave( epoch );
    }
    return LDAP_SUCCESS;
}

int
tier_destroy( LloadTier *tier )
{
    while ( !LDAP_CIRCLEQ_EMPTY( &tier->t_backends ) ) {
        LloadBackend *b = LDAP_CIRCLEQ_FIRST( &tier->t_backends );
        epoch_t epoch = epoch_join();

        lload_backend_destroy( b );

        epoch_leave( epoch );
    }

#ifdef BALANCER_MODULE
    if ( tier->t_monitor ) {
        /* FIXME: implement proper subsys shutdown in back-monitor or make
         * backend just an entry, not a subsys */
        if ( slapd_shutdown ) {
            /* Just drop backlink, back-monitor will call mss_destroy later */
            assert( tier->t_monitor->mss_private == tier );
            tier->t_monitor->mss_private = NULL;
        } else {
            BackendDB *be;
            struct berval monitordn = BER_BVC("cn=monitor");
            int rc;

            be = select_backend( &monitordn, 0 );

            rc = tier->t_monitor->mss_destroy( be, tier->t_monitor );
            assert( rc == LDAP_SUCCESS );
        }
    }
#endif /* BALANCER_MODULE */

    ch_free( tier->t_name.bv_val );
    ch_free( tier );
    return LDAP_SUCCESS;
}

void
lload_tiers_destroy( void )
{
    while ( !LDAP_STAILQ_EMPTY( &tiers ) ) {
        LloadTier *tier = LDAP_STAILQ_FIRST( &tiers );

        LDAP_STAILQ_REMOVE_HEAD( &tiers, t_next );
        tier->t_type.tier_destroy( tier );
    }
}

void
lload_tiers_shutdown( void )
{
    lload_tiers_reset( 1 );
}

void
lload_tiers_reset( int shutdown )
{
    LloadTier *tier;

    LDAP_STAILQ_FOREACH ( tier, &tiers, t_next ) {
        tier->t_type.tier_reset( tier, shutdown );
    }
}

void
lload_tiers_update( evutil_socket_t s, short what, void *arg )
{
    LloadTier *tier;

    LDAP_STAILQ_FOREACH ( tier, &tiers, t_next ) {
        if ( tier->t_type.tier_update ) {
            tier->t_type.tier_update( tier );
        }
    }
}

extern struct lload_tier_type roundrobin_tier;
extern struct lload_tier_type weighted_tier;
extern struct lload_tier_type bestof_tier;

struct {
    char *name;
    struct lload_tier_type *type;
} tier_types[] = {
        { "roundrobin", &roundrobin_tier },
        { "weighted", &weighted_tier },
        { "bestof", &bestof_tier },

        { NULL }
};

struct lload_tier_type *
lload_tier_find( char *name )
{
    int i;

    for ( i = 0; tier_types[i].name; i++ ) {
        if ( !strcasecmp( name, tier_types[i].name ) ) {
            return tier_types[i].type;
        }
    }
    return NULL;
}
