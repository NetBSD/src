/*	$NetBSD: monitor.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

/* init.c - initialize various things */
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
__RCSID("$NetBSD: monitor.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "lload.h"
#include "lber_pvt.h"

#include "ldap_rq.h"
#include "lload-config.h"
#include "../slapd/back-monitor/back-monitor.h"

#define LLOAD_MONITOR_BALANCER_NAME "Load Balancer"
#define LLOAD_MONITOR_BALANCER_RDN \
    SLAPD_MONITOR_AT "=" LLOAD_MONITOR_BALANCER_NAME
#define LLOAD_MONITOR_BALANCER_DN \
    LLOAD_MONITOR_BALANCER_RDN "," SLAPD_MONITOR_BACKEND_DN

#define LLOAD_MONITOR_INCOMING_NAME "Incoming Connections"
#define LLOAD_MONITOR_INCOMING_RDN \
    SLAPD_MONITOR_AT "=" LLOAD_MONITOR_INCOMING_NAME
#define LLOAD_MONITOR_INCOMING_DN \
    LLOAD_MONITOR_INCOMING_RDN "," LLOAD_MONITOR_BALANCER_DN

#define LLOAD_MONITOR_OPERATIONS_NAME "Operations"
#define LLOAD_MONITOR_OPERATIONS_RDN \
    SLAPD_MONITOR_AT "=" LLOAD_MONITOR_OPERATIONS_NAME
#define LLOAD_MONITOR_OPERATIONS_DN \
    LLOAD_MONITOR_OPERATIONS_RDN "," LLOAD_MONITOR_BALANCER_DN

#define LLOAD_MONITOR_BACKENDS_NAME "Backend Servers"
#define LLOAD_MONITOR_BACKENDS_RDN \
    SLAPD_MONITOR_AT "=" LLOAD_MONITOR_BACKENDS_NAME
#define LLOAD_MONITOR_BACKENDS_DN \
    LLOAD_MONITOR_BACKENDS_RDN "," LLOAD_MONITOR_BALANCER_DN

struct lload_monitor_ops_t {
    struct berval rdn;
} lload_monitor_op[] = {
    { BER_BVC("cn=Bind") },
    { BER_BVC("cn=Other") },

    { BER_BVNULL }
};

static ObjectClass *oc_olmBalancer;
static ObjectClass *oc_olmBalancerServer;
static ObjectClass *oc_olmBalancerConnection;
static ObjectClass *oc_olmBalancerOperation;

static ObjectClass *oc_monitorContainer;
static ObjectClass *oc_monitorCounterObject;

static AttributeDescription *ad_olmServerURI;
static AttributeDescription *ad_olmReceivedOps;
static AttributeDescription *ad_olmForwardedOps;
static AttributeDescription *ad_olmRejectedOps;
static AttributeDescription *ad_olmCompletedOps;
static AttributeDescription *ad_olmFailedOps;
static AttributeDescription *ad_olmConnectionType;
static AttributeDescription *ad_olmPendingOps;
static AttributeDescription *ad_olmPendingConnections;
static AttributeDescription *ad_olmActiveConnections;
static AttributeDescription *ad_olmIncomingConnections;
static AttributeDescription *ad_olmOutgoingConnections;

static struct {
    char *name;
    char *oid;
} s_oid[] = {
    { "olmBalancerAttributes", "olmModuleAttributes:1" },
    { "olmBalancerObjectClasses", "olmModuleObjectClasses:1" },

    { NULL }
};

static struct {
    char *desc;
    AttributeDescription **ad;
} s_at[] = {
    { "( olmBalancerAttributes:1 "
      "NAME ( 'olmServerURI' ) "
      "DESC 'URI of a backend server' "
      "SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
      "EQUALITY caseIgnoreMatch "
      "NO-USER-MODIFICATION "
      "USAGE dSAOperation )",
        &ad_olmServerURI },
    { "( olmBalancerAttributes:2 "
      "NAME ( 'olmReceivedOps' ) "
      "DESC 'monitor received operations' "
      "SUP monitorCounter "
      "NO-USER-MODIFICATION "
      "USAGE dSAOperation )",
        &ad_olmReceivedOps },
    { "( olmBalancerAttributes:3 "
      "NAME ( 'olmForwardedOps' ) "
      "DESC 'monitor forwarded operations' "
      "SUP monitorCounter "
      "NO-USER-MODIFICATION "
      "USAGE dSAOperation )",
        &ad_olmForwardedOps },
    { "( olmBalancerAttributes:4 "
      "NAME ( 'olmRejectedOps' ) "
      "DESC 'monitor rejected operations' "
      "SUP monitorCounter "
      "NO-USER-MODIFICATION "
      "USAGE dSAOperation )",
        &ad_olmRejectedOps },
    { "( olmBalancerAttributes:5 "
      "NAME ( 'olmCompletedOps' ) "
      "DESC 'monitor completed operations' "
      "SUP monitorCounter "
      "NO-USER-MODIFICATION "
      "USAGE dSAOperation )",
        &ad_olmCompletedOps },
    { "( olmBalancerAttributes:6 "
      "NAME ( 'olmFailedOps' ) "
      "DESC 'monitor failed operations' "
      "SUP monitorCounter "
      "NO-USER-MODIFICATION "
      "USAGE dSAOperation )",
        &ad_olmFailedOps },
    { "( olmBalancerAttributes:7 "
      "NAME ( 'olmPendingOps' ) "
      "DESC 'monitor number of pending operations' "
      "EQUALITY integerMatch "
      "SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 "
      "NO-USER-MODIFICATION "
      "USAGE dSAOperation )",
        &ad_olmPendingOps },
    { "( olmBalancerAttributes:8 "
      "NAME ( 'olmPendingConnections' ) "
      "DESC 'monitor number of pending connections' "
      "EQUALITY integerMatch "
      "SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 "
      "NO-USER-MODIFICATION "
      "USAGE dSAOperation )",
        &ad_olmPendingConnections },
    { "( olmBalancerAttributes:9 "
      "NAME ( 'olmActiveConnections' ) "
      "DESC 'monitor number of active connections' "
      "EQUALITY integerMatch "
      "SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 "
      "NO-USER-MODIFICATION "
      "USAGE dSAOperation )",
        &ad_olmActiveConnections },
    { "( olmBalancerAttributes:10 "
      "NAME ( 'olmConnectionType' ) "
      "DESC 'Connection type' "
      "EQUALITY caseIgnoreMatch "
      "SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
      "NO-USER-MODIFICATION "
      "USAGE dSAOperation )",
        &ad_olmConnectionType },
    { "( olmBalancerAttributes:11 "
      "NAME ( 'olmIncomingConnections' ) "
      "DESC 'monitor number of incoming connections' "
      "EQUALITY integerMatch "
      "SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 "
      "NO-USER-MODIFICATION "
      "USAGE dSAOperation )",
        &ad_olmIncomingConnections },
    { "( olmBalancerAttributes:12 "
      "NAME ( 'olmOutgoingConnections' ) "
      "DESC 'monitor number of active connections' "
      "EQUALITY integerMatch "
      "SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 "
      "NO-USER-MODIFICATION "
      "USAGE dSAOperation )",
        &ad_olmOutgoingConnections },

    { NULL }
};

static struct {
    char *name;
    ObjectClass **oc;
} s_moc[] = {
    { "monitorContainer", &oc_monitorContainer },
    { "monitorCounterObject", &oc_monitorCounterObject },

    { NULL }
};

static struct {
    char *desc;
    ObjectClass **oc;
} s_oc[] = {
    { "( olmBalancerObjectClasses:1 "
      "NAME ( 'olmBalancer' ) "
      "SUP top STRUCTURAL "
      "MAY ( "
      "olmIncomingConnections "
      "$ olmOutgoingConnections "
      ") )",
        &oc_olmBalancer },
    { "( olmBalancerObjectClasses:2 "
      "NAME ( 'olmBalancerServer' ) "
      "SUP top STRUCTURAL "
      "MAY ( "
      "olmServerURI "
      "$ olmActiveConnections "
      "$ olmPendingConnections "
      "$ olmPendingOps"
      "$ olmReceivedOps "
      "$ olmCompletedOps "
      "$ olmFailedOps "
      ") )",
        &oc_olmBalancerServer },

    { "( olmBalancerObjectClasses:3 "
      "NAME ( 'olmBalancerOperation' ) "
      "SUP top STRUCTURAL "
      "MAY ( "
      "olmReceivedOps "
      "$ olmForwardedOps "
      "$ olmRejectedOps "
      "$ olmCompletedOps "
      "$ olmFailedOps "
      ") )",
        &oc_olmBalancerOperation },
    { "( olmBalancerObjectClasses:4 "
      "NAME ( 'olmBalancerConnection' ) "
      "SUP top STRUCTURAL "
      "MAY ( "
      "olmConnectionType "
      "$ olmPendingOps "
      "$ olmReceivedOps "
      "$ olmCompletedOps "
      "$ olmFailedOps "
      ") )",
        &oc_olmBalancerConnection },
    { NULL }
};

static int
lload_monitor_subsystem_destroy( BackendDB *be, monitor_subsys_t *ms )
{
    return LDAP_SUCCESS;
}

static int
lload_monitor_backend_destroy( BackendDB *be, monitor_subsys_t *ms )
{
    LloadBackend *b = ms->mss_private;
    monitor_extra_t *mbe;
    int rc = LDAP_SUCCESS;

    mbe = (monitor_extra_t *)be->bd_info->bi_extra;
    if ( b->b_monitor ) {
        ms->mss_destroy = lload_monitor_subsystem_destroy;

        assert( b->b_monitor == ms );
        b->b_monitor = NULL;

        rc = mbe->unregister_entry( &ms->mss_ndn );
        ber_memfree( ms->mss_dn.bv_val );
        ber_memfree( ms->mss_ndn.bv_val );
    }

    return rc;
}

static void
lload_monitor_balancer_dispose( void **priv )
{
    return;
}

static int
lload_monitor_balancer_free( Entry *e, void **priv )
{
    return LDAP_SUCCESS;
}

static int
lload_monitor_balancer_update(
        Operation *op,
        SlapReply *rs,
        Entry *e,
        void *priv )
{
    Attribute *a;

    a = attr_find( e->e_attrs, ad_olmIncomingConnections );
    assert( a != NULL );

    UI2BV( &a->a_vals[0], lload_stats.global_incoming );

    a = attr_find( e->e_attrs, ad_olmOutgoingConnections );
    assert( a != NULL );

    UI2BV( &a->a_vals[0], lload_stats.global_outgoing );
    return SLAP_CB_CONTINUE;
}

static int
lload_monitor_ops_update( Operation *op, SlapReply *rs, Entry *e, void *priv )
{
    Attribute *a;
    lload_counters_t *counters = (lload_counters_t *)priv;

    a = attr_find( e->e_attrs, ad_olmReceivedOps );
    assert( a != NULL );
    UI2BV( &a->a_vals[0], counters->lc_ops_received );

    a = attr_find( e->e_attrs, ad_olmForwardedOps );
    assert( a != NULL );
    UI2BV( &a->a_vals[0], counters->lc_ops_forwarded );

    a = attr_find( e->e_attrs, ad_olmRejectedOps );
    assert( a != NULL );
    UI2BV( &a->a_vals[0], counters->lc_ops_rejected );

    a = attr_find( e->e_attrs, ad_olmCompletedOps );
    assert( a != NULL );
    UI2BV( &a->a_vals[0], counters->lc_ops_completed );

    a = attr_find( e->e_attrs, ad_olmFailedOps );
    assert( a != NULL );
    UI2BV( &a->a_vals[0], counters->lc_ops_failed );

    return SLAP_CB_CONTINUE;
}

static void
lload_monitor_ops_dispose( void **priv )
{
    return;
}

static int
lload_monitor_ops_free( Entry *e, void **priv )
{
    return LDAP_SUCCESS;
}

static int
lload_monitor_balancer_init( BackendDB *be, monitor_subsys_t *ms )
{
    monitor_extra_t *mbe;
    Entry *e;
    int rc;
    monitor_callback_t *cb;
    struct berval value = BER_BVC("0");

    assert( be != NULL );

    mbe = (monitor_extra_t *)be->bd_info->bi_extra;

    dnNormalize( 0, NULL, NULL, &ms->mss_dn, &ms->mss_ndn, NULL );

    e = mbe->entry_stub( &ms->mss_dn, &ms->mss_ndn, &ms->mss_rdn,
            oc_olmBalancer, NULL, NULL );
    if ( e == NULL ) {
        Debug( LDAP_DEBUG_ANY, "lload_monitor_balancer_init: "
                "unable to create entry \"%s,%s\"\n",
                ms->mss_rdn.bv_val, ms->mss_ndn.bv_val );
        return -1;
    }

    ch_free( ms->mss_ndn.bv_val );
    ber_dupbv( &ms->mss_dn, &e->e_name );
    ber_dupbv( &ms->mss_ndn, &e->e_nname );

    cb = ch_calloc( sizeof(monitor_callback_t), 1 );
    cb->mc_update = lload_monitor_balancer_update;
    cb->mc_free = lload_monitor_balancer_free;
    cb->mc_dispose = lload_monitor_balancer_dispose;
    cb->mc_private = NULL;

    attr_merge_normalize_one( e, ad_olmIncomingConnections, &value, NULL );
    attr_merge_normalize_one( e, ad_olmOutgoingConnections, &value, NULL );

    rc = mbe->register_entry( e, cb, ms, 0 );
    if ( rc != LDAP_SUCCESS ) {
        Debug( LDAP_DEBUG_ANY, "lload_monitor_balancer_init: "
                "unable to register entry \"%s\" for monitoring\n",
                e->e_name.bv_val );
        goto done;
    }

done:
    entry_free( e );

    return rc;
}

static int
lload_monitor_ops_init( BackendDB *be, monitor_subsys_t *ms )
{
    monitor_extra_t *mbe;
    Entry *e, *parent;
    int rc;
    int i;
    struct berval value = BER_BVC("0");

    assert( be != NULL );

    mbe = (monitor_extra_t *)be->bd_info->bi_extra;

    dnNormalize( 0, NULL, NULL, &ms->mss_dn, &ms->mss_ndn, NULL );
    ms->mss_destroy = lload_monitor_subsystem_destroy;

    parent = mbe->entry_stub( &ms->mss_dn, &ms->mss_ndn, &ms->mss_rdn,
            oc_monitorContainer, NULL, NULL );
    if ( parent == NULL ) {
        Debug( LDAP_DEBUG_ANY, "lload_monitor_ops_init: "
                "unable to create entry \"%s,%s\"\n",
                ms->mss_rdn.bv_val, ms->mss_ndn.bv_val );
        return -1;
    }
    ch_free( ms->mss_ndn.bv_val );
    ber_dupbv( &ms->mss_dn, &parent->e_name );
    ber_dupbv( &ms->mss_ndn, &parent->e_nname );

    rc = mbe->register_entry( parent, NULL, ms, MONITOR_F_PERSISTENT_CH );
    if ( rc != LDAP_SUCCESS ) {
        Debug( LDAP_DEBUG_ANY, "lload_monitor_ops_init: "
                "unable to register entry \"%s\" for monitoring\n",
                parent->e_name.bv_val );
        goto done;
    }

    for ( i = 0; lload_monitor_op[i].rdn.bv_val != NULL; i++ ) {
        monitor_callback_t *cb;
        e = mbe->entry_stub( &parent->e_name, &parent->e_nname,
                &lload_monitor_op[i].rdn, oc_olmBalancerOperation, NULL, NULL );
        if ( e == NULL ) {
            Debug( LDAP_DEBUG_ANY, "lload_monitor_ops_init: "
                    "unable to create entry \"%s,%s\"\n",
                    lload_monitor_op[i].rdn.bv_val, parent->e_nname.bv_val );
            return -1;
        }

        /* attr_merge_normalize_one( e, ad_olmDbOperations, &value, NULL ); */

        /*
         * We cannot share a single callback between entries.
         *
         * monitor_cache_destroy() tries to free all callbacks and it's called
         * before mss_destroy() so we have no chance of handling it ourselves
         */
        cb = ch_calloc( sizeof(monitor_callback_t), 1 );
        cb->mc_update = lload_monitor_ops_update;
        cb->mc_free = lload_monitor_ops_free;
        cb->mc_dispose = lload_monitor_ops_dispose;
        cb->mc_private = &lload_stats.counters[i];

        attr_merge_normalize_one( e, ad_olmReceivedOps, &value, NULL );
        attr_merge_normalize_one( e, ad_olmForwardedOps, &value, NULL );
        attr_merge_normalize_one( e, ad_olmRejectedOps, &value, NULL );
        attr_merge_normalize_one( e, ad_olmCompletedOps, &value, NULL );
        attr_merge_normalize_one( e, ad_olmFailedOps, &value, NULL );

        rc = mbe->register_entry( e, cb, ms, 0 );

        entry_free( e );

        if ( rc != LDAP_SUCCESS ) {
            Debug( LDAP_DEBUG_ANY, "lload_monitor_ops_init: "
                    "unable to register entry \"%s\" for monitoring\n",
                    e->e_name.bv_val );
            ch_free( cb );
            break;
        }
    }

done:
    entry_free( parent );
    return rc;
}

static int
lload_monitor_in_conn_entry( LloadConnection *conn, void *argv )
{
    Entry *e;
    monitor_entry_t *mp;
    struct lload_monitor_conn_arg *arg = argv;
    monitor_extra_t *mbe = arg->op->o_bd->bd_info->bi_extra;
    char buf[SLAP_TEXT_BUFLEN];
    struct berval bv;

    bv.bv_val = buf;
    bv.bv_len = snprintf(
            bv.bv_val, SLAP_TEXT_BUFLEN, "cn=Connection %lu", conn->c_connid );

    e = mbe->entry_stub( &arg->ms->mss_dn, &arg->ms->mss_ndn, &bv,
            oc_olmBalancerConnection, NULL, NULL );

    mp = mbe->entrypriv_create();
    e->e_private = mp;
    mp->mp_info = arg->ms;
    mp->mp_flags = MONITOR_F_SUB | MONITOR_F_VOLATILE;

    *arg->ep = e;
    arg->ep = &mp->mp_next;

    return 0;
}

static int
lload_monitor_in_conn_create(
        Operation *op,
        SlapReply *rs,
        struct berval *ndn,
        Entry *e_parent,
        Entry **ep )
{
    monitor_entry_t *mp_parent;
    struct lload_monitor_conn_arg arg = {
        .op = op,
        .ep = ep,
    };

    assert( e_parent->e_private != NULL );

    mp_parent = e_parent->e_private;
    arg.ms = (monitor_subsys_t *)mp_parent->mp_info;

    checked_lock( &clients_mutex );
    connections_walk(
            &clients_mutex, &clients, lload_monitor_in_conn_entry, &arg );
    checked_unlock( &clients_mutex );

    return 0;
}

static int
lload_monitor_up_conn_entry( LloadConnection *c, void *argv )
{
    Entry *e;
    monitor_entry_t *mp;
    struct lload_monitor_conn_arg *arg = argv;
    monitor_extra_t *mbe = arg->op->o_bd->bd_info->bi_extra;
    char buf[SLAP_TEXT_BUFLEN];
    struct berval bv_rdn,
        bv_type = BER_BVNULL,
        bv_pending = BER_BVNULL,
        bv_received = BER_BVNULL,
        bv_completed = BER_BVNULL,
        bv_failed = BER_BVNULL;

    bv_rdn.bv_val = buf;
    bv_rdn.bv_len = snprintf(
            bv_rdn.bv_val, SLAP_TEXT_BUFLEN, "cn=Connection %lu", c->c_connid );

    e = mbe->entry_stub( &arg->ms->mss_dn, &arg->ms->mss_ndn, &bv_rdn,
            oc_olmBalancerConnection, NULL, NULL );

    switch ( c->c_type ) {
        case LLOAD_C_OPEN: {
            struct berval bv = BER_BVC("regular");
            bv_type = bv;
        } break;
        case LLOAD_C_PREPARING: {
            struct berval bv = BER_BVC("preparing");
            bv_type = bv;
        } break;
        case LLOAD_C_BIND: {
            struct berval bv = BER_BVC("bind");
            bv_type = bv;
        } break;
        case LLOAD_C_PRIVILEGED: {
            struct berval bv = BER_BVC("privileged");
            bv_type = bv;
        } break;
        default: {
            struct berval bv = BER_BVC("unknown");
            bv_type = bv;
        } break;
    }

    UI2BV( &bv_pending, (long long unsigned int)c->c_n_ops_executing );
    UI2BV( &bv_received, c->c_counters.lc_ops_received );
    UI2BV( &bv_completed, c->c_counters.lc_ops_completed );
    UI2BV( &bv_failed, c->c_counters.lc_ops_failed );

    attr_merge_normalize_one( e, ad_olmConnectionType, &bv_type, NULL );
    attr_merge_normalize_one( e, ad_olmPendingOps, &bv_pending, NULL );
    attr_merge_normalize_one( e, ad_olmReceivedOps, &bv_received, NULL );
    attr_merge_normalize_one( e, ad_olmCompletedOps, &bv_completed, NULL );
    attr_merge_normalize_one( e, ad_olmFailedOps, &bv_failed, NULL );

    ch_free( bv_pending.bv_val );
    ch_free( bv_received.bv_val );
    ch_free( bv_completed.bv_val );
    ch_free( bv_failed.bv_val );
    mp = mbe->entrypriv_create();
    e->e_private = mp;
    mp->mp_info = arg->ms;
    mp->mp_flags = MONITOR_F_SUB | MONITOR_F_VOLATILE;

    *arg->ep = e;
    arg->ep = &mp->mp_next;

    return 0;
}

static int
lload_monitor_up_conn_create(
        Operation *op,
        SlapReply *rs,
        struct berval *ndn,
        Entry *e_parent,
        Entry **ep )
{
    monitor_entry_t *mp_parent;
    monitor_subsys_t *ms;
    LloadBackend *b;
    struct lload_monitor_conn_arg arg = {
            .op = op,
            .ep = ep,
    };

    assert( e_parent->e_private != NULL );

    mp_parent = e_parent->e_private;
    ms = (monitor_subsys_t *)mp_parent->mp_info;
    b = ms->mss_private;

    if ( !b ) {
        return -1;
    }

    arg.ms = ms;

    checked_lock( &b->b_mutex );
    connections_walk_last( &b->b_mutex, &b->b_conns, b->b_last_conn,
            lload_monitor_up_conn_entry, &arg );

    connections_walk_last( &b->b_mutex, &b->b_bindconns, b->b_last_bindconn,
            lload_monitor_up_conn_entry, &arg );
    checked_unlock( &b->b_mutex );

    return 0;
}

int
lload_monitor_incoming_conn_init( BackendDB *be, monitor_subsys_t *ms )
{
    monitor_extra_t *mbe;
    Entry *e;
    int rc;

    assert( be != NULL );
    mbe = (monitor_extra_t *)be->bd_info->bi_extra;

    ms->mss_create = lload_monitor_in_conn_create;
    ms->mss_destroy = lload_monitor_subsystem_destroy;

    dnNormalize( 0, NULL, NULL, &ms->mss_dn, &ms->mss_ndn, NULL );

    e = mbe->entry_stub( &ms->mss_dn, &ms->mss_ndn, &ms->mss_rdn,
            oc_monitorContainer, NULL, NULL );
    if ( e == NULL ) {
        Debug( LDAP_DEBUG_ANY, "lload_monitor_incoming_conn_init: "
                "unable to create entry \"%s,%s\"\n",
                ms->mss_rdn.bv_val, ms->mss_ndn.bv_val );
        return -1;
    }
    ch_free( ms->mss_ndn.bv_val );
    ber_dupbv( &ms->mss_dn, &e->e_name );
    ber_dupbv( &ms->mss_ndn, &e->e_nname );

    rc = mbe->register_entry( e, NULL, ms, MONITOR_F_VOLATILE_CH );

    if ( rc != LDAP_SUCCESS ) {
        Debug( LDAP_DEBUG_ANY, "lload_monitor_incoming_conn_init: "
                "unable to register entry \"%s\" for monitoring\n",
                e->e_name.bv_val );
        goto done;
    }
done:
    entry_free( e );

    return rc;
}

static int
lload_monitor_server_update(
        Operation *op,
        SlapReply *rs,
        Entry *e,
        void *priv )
{
    Attribute *a;
    LloadBackend *b = priv;
    LloadConnection *c;
    LloadPendingConnection *pc;
    ldap_pvt_mp_t active = 0, pending = 0, received = 0, completed = 0,
                  failed = 0;
    int i;

    checked_lock( &b->b_mutex );
    active = b->b_active + b->b_bindavail;

    LDAP_CIRCLEQ_FOREACH ( c, &b->b_preparing, c_next ) {
        pending++;
    }

    LDAP_LIST_FOREACH( pc, &b->b_connecting, next ) {
        pending++;
    }

    for ( i = 0; i < LLOAD_STATS_OPS_LAST; i++ ) {
        received += b->b_counters[i].lc_ops_received;
        completed += b->b_counters[i].lc_ops_completed;
        failed += b->b_counters[i].lc_ops_failed;
    }

    a = attr_find( e->e_attrs, ad_olmPendingOps );
    assert( a != NULL );
    UI2BV( &a->a_vals[0], (long long unsigned int)b->b_n_ops_executing );

    checked_unlock( &b->b_mutex );

    /* Right now, there is no way to retrieve the entry from monitor's
     * cache to replace URI at the moment it is modified */
    a = attr_find( e->e_attrs, ad_olmServerURI );
    assert( a != NULL );
    ber_bvreplace( &a->a_vals[0], &b->b_uri );

    a = attr_find( e->e_attrs, ad_olmActiveConnections );
    assert( a != NULL );
    UI2BV( &a->a_vals[0], active );

    a = attr_find( e->e_attrs, ad_olmPendingConnections );
    assert( a != NULL );
    UI2BV( &a->a_vals[0], pending );

    a = attr_find( e->e_attrs, ad_olmReceivedOps );
    assert( a != NULL );
    UI2BV( &a->a_vals[0], received );

    a = attr_find( e->e_attrs, ad_olmCompletedOps );
    assert( a != NULL );
    UI2BV( &a->a_vals[0], completed );

    a = attr_find( e->e_attrs, ad_olmFailedOps );
    assert( a != NULL );
    UI2BV( &a->a_vals[0], failed );

    return SLAP_CB_CONTINUE;
}

static int
lload_monitor_backend_open( BackendDB *be, monitor_subsys_t *ms )
{
    Entry *e;
    struct berval value = BER_BVC("0");
    monitor_extra_t *mbe;
    monitor_callback_t *cb;
    LloadBackend *b = ms->mss_private;
    int rc;

    assert( be != NULL );
    mbe = (monitor_extra_t *)be->bd_info->bi_extra;

    dnNormalize( 0, NULL, NULL, &ms->mss_dn, &ms->mss_ndn, NULL );
    e = mbe->entry_stub( &ms->mss_dn, &ms->mss_ndn, &ms->mss_rdn,
            oc_olmBalancerServer, NULL, NULL );
    if ( e == NULL ) {
        Debug( LDAP_DEBUG_ANY, "lload_monitor_backend_open: "
                "unable to create entry \"%s,%s\"\n",
                ms->mss_rdn.bv_val, ms->mss_ndn.bv_val );
        return -1;
    }

    ch_free( ms->mss_ndn.bv_val );
    ber_dupbv( &ms->mss_dn, &e->e_name );
    ber_dupbv( &ms->mss_ndn, &e->e_nname );

    cb = ch_calloc( sizeof(monitor_callback_t), 1 );
    cb->mc_update = lload_monitor_server_update;
    cb->mc_free = NULL;
    cb->mc_dispose = NULL;
    cb->mc_private = b;

    attr_merge_normalize_one( e, ad_olmServerURI, &b->b_uri, NULL );
    attr_merge_normalize_one( e, ad_olmActiveConnections, &value, NULL );
    attr_merge_normalize_one( e, ad_olmPendingConnections, &value, NULL );
    attr_merge_normalize_one( e, ad_olmPendingOps, &value, NULL );
    attr_merge_normalize_one( e, ad_olmReceivedOps, &value, NULL );
    attr_merge_normalize_one( e, ad_olmCompletedOps, &value, NULL );
    attr_merge_normalize_one( e, ad_olmFailedOps, &value, NULL );

    rc = mbe->register_entry( e, cb, ms, MONITOR_F_VOLATILE_CH );

    if ( rc != LDAP_SUCCESS ) {
        Debug( LDAP_DEBUG_ANY, "lload_monitor_backend_open: "
                "unable to register entry \"%s\" for monitoring\n",
                e->e_name.bv_val );
        goto done;
    }

    b->b_monitor = ms;
    ms->mss_destroy = lload_monitor_backend_destroy;

done:
    entry_free( e );
    return rc;
}

int
lload_monitor_backend_init( BackendInfo *bi, LloadBackend *b )
{
    monitor_extra_t *mbe;
    monitor_subsys_t *bk_mss;

    mbe = (monitor_extra_t *)bi->bi_extra;

    /* FIXME: With back-monitor as it works now, there is no way to know when
     * this can be safely freed so we leak it on shutdown */
    bk_mss = ch_calloc( 1, sizeof(monitor_subsys_t) );
    bk_mss->mss_rdn.bv_len = sizeof("cn=") + b->b_name.bv_len;
    bk_mss->mss_rdn.bv_val = ch_malloc( bk_mss->mss_rdn.bv_len );
    bk_mss->mss_rdn.bv_len = snprintf( bk_mss->mss_rdn.bv_val,
            bk_mss->mss_rdn.bv_len, "cn=%s", b->b_name.bv_val );

    ber_str2bv( LLOAD_MONITOR_BACKENDS_DN, 0, 0, &bk_mss->mss_dn );
    bk_mss->mss_name = b->b_name.bv_val;
    bk_mss->mss_flags = MONITOR_F_VOLATILE_CH;
    bk_mss->mss_open = lload_monitor_backend_open;
    bk_mss->mss_create = lload_monitor_up_conn_create;
    bk_mss->mss_destroy = lload_monitor_subsystem_destroy;
    bk_mss->mss_update = NULL;
    bk_mss->mss_private = b;

    if ( mbe->register_subsys_late( bk_mss ) ) {
        Debug( LDAP_DEBUG_ANY, "lload_monitor_backend_init: "
                "failed to register backend %s\n",
                bk_mss->mss_name );
        return -1;
    }
    return LDAP_SUCCESS;
}

int
lload_monitor_backends_init( BackendDB *be, monitor_subsys_t *ms )
{
    monitor_extra_t *mbe;
    Entry *e;
    int rc;
    LloadBackend *b;

    assert( be != NULL );
    mbe = (monitor_extra_t *)be->bd_info->bi_extra;

    dnNormalize( 0, NULL, NULL, &ms->mss_dn, &ms->mss_ndn, NULL );

    e = mbe->entry_stub( &ms->mss_dn, &ms->mss_ndn, &ms->mss_rdn,
            oc_monitorContainer, NULL, NULL );
    if ( e == NULL ) {
        Debug( LDAP_DEBUG_ANY, "lload_monitor_incoming_conn_init: "
                "unable to create entry \"%s,%s\"\n",
                ms->mss_rdn.bv_val, ms->mss_ndn.bv_val );
        return -1;
    }
    ch_free( ms->mss_ndn.bv_val );
    ber_dupbv( &ms->mss_dn, &e->e_name );
    ber_dupbv( &ms->mss_ndn, &e->e_nname );

    rc = mbe->register_entry( e, NULL, ms, MONITOR_F_PERSISTENT_CH );

    if ( rc != LDAP_SUCCESS ) {
        Debug( LDAP_DEBUG_ANY, "lload_monitor_backends_init: "
                "unable to register entry \"%s\" for monitoring\n",
                e->e_name.bv_val );
        goto done;
    }

    LDAP_CIRCLEQ_FOREACH ( b, &backend, b_next ) {
        if ( (rc = lload_monitor_backend_init( be->bd_info, b )) ) {
            break;
        }
    }
done:
    entry_free( e );

    return rc;
}

static int
lload_monitor_incoming_count( LloadConnection *conn, void *argv )
{
    lload_global_stats_t *tmp_stats = argv;
    tmp_stats->global_incoming++;
    return 0;
}

/*
 * Update all global statistics other than rejected and received,
 * which are updated in real time
 */
void *
lload_monitor_update_global_stats( void *ctx, void *arg )
{
    struct re_s *rtask = arg;
    lload_global_stats_t tmp_stats = {};
    LloadBackend *b;
    int i;

    Debug( LDAP_DEBUG_TRACE, "lload_monitor_update_global_stats: "
            "updating stats\n" );

    /* count incoming connections */
    checked_lock( &clients_mutex );
    connections_walk( &clients_mutex, &clients, lload_monitor_incoming_count,
            &tmp_stats );
    checked_unlock( &clients_mutex );

    LDAP_CIRCLEQ_FOREACH ( b, &backend, b_next ) {
        checked_lock( &b->b_mutex );
        tmp_stats.global_outgoing += b->b_active + b->b_bindavail;

        /* merge completed and failed stats */
        for ( i = 0; i < LLOAD_STATS_OPS_LAST; i++ ) {
            tmp_stats.counters[i].lc_ops_completed +=
                    b->b_counters[i].lc_ops_completed;
            tmp_stats.counters[i].lc_ops_failed +=
                    b->b_counters[i].lc_ops_failed;
        }
        checked_unlock( &b->b_mutex );
    }

    /* update lload_stats */
    lload_stats.global_outgoing = tmp_stats.global_outgoing;
    lload_stats.global_incoming = tmp_stats.global_incoming;
    for ( i = 0; i < LLOAD_STATS_OPS_LAST; i++ ) {
        lload_stats.counters[i].lc_ops_completed =
                tmp_stats.counters[i].lc_ops_completed;
        lload_stats.counters[i].lc_ops_failed =
                tmp_stats.counters[i].lc_ops_failed;
    }

    /* reschedule */
    checked_lock( &slapd_rq.rq_mutex );
    ldap_pvt_runqueue_stoptask( &slapd_rq, rtask );
    checked_unlock( &slapd_rq.rq_mutex );
    return NULL;
}

static char *lload_subsys_rdn[] = {
    LLOAD_MONITOR_BALANCER_RDN,
    LLOAD_MONITOR_INCOMING_RDN,
    LLOAD_MONITOR_OPERATIONS_RDN,
    LLOAD_MONITOR_BACKENDS_RDN,
    NULL
};

static struct monitor_subsys_t balancer_subsys[] = {
    {
        LLOAD_MONITOR_BALANCER_NAME,
        BER_BVNULL,
        BER_BVC(SLAPD_MONITOR_BACKEND_DN),
        BER_BVNULL,
        { BER_BVC("Load Balancer information"),
          BER_BVNULL },
        MONITOR_F_PERSISTENT_CH,
        lload_monitor_balancer_init,
        lload_monitor_subsystem_destroy, /* destroy */
        NULL,   /* update */
        NULL,   /* create */
        NULL    /* modify */
    },
    {
        LLOAD_MONITOR_INCOMING_NAME,
        BER_BVNULL,
        BER_BVC(LLOAD_MONITOR_BALANCER_DN),
        BER_BVNULL,
        { BER_BVC("Load Balancer incoming connections"),
          BER_BVNULL },
        MONITOR_F_VOLATILE_CH,
        lload_monitor_incoming_conn_init,
        lload_monitor_subsystem_destroy, /* destroy */
        NULL,   /* update */
        NULL,   /* create */
        NULL    /* modify */
    },
    {
        LLOAD_MONITOR_OPERATIONS_NAME,
        BER_BVNULL,
        BER_BVC(LLOAD_MONITOR_BALANCER_DN),
        BER_BVNULL,
        { BER_BVC("Load Balancer global operation statistics"),
          BER_BVNULL },
        MONITOR_F_PERSISTENT_CH,
        lload_monitor_ops_init,
        lload_monitor_subsystem_destroy, /* destroy */
        NULL,   /* update */
        NULL,   /* create */
        NULL    /* modify */
    },
    {
        LLOAD_MONITOR_BACKENDS_NAME,
        BER_BVNULL,
        BER_BVC(LLOAD_MONITOR_BALANCER_DN),
        BER_BVNULL,
        { BER_BVC("Load Balancer Backends information"),
          BER_BVNULL },
        MONITOR_F_PERSISTENT_CH,
        lload_monitor_backends_init,
        lload_monitor_subsystem_destroy, /* destroy */
        NULL,   /* update */
        NULL,   /* create */
        NULL    /* modify */
    },
    { NULL }
};

int
lload_monitor_open( void )
{
    static int lload_monitor_initialized_failure = 1;
    static int lload_monitor_initialized = 0;
    BackendInfo *mi;
    monitor_extra_t *mbe;
    monitor_subsys_t *mss;
    ConfigArgs c;
    char *argv[3], **rdn;
    int i, rc;

    /* check if monitor is configured and usable */
    mi = backend_info( "monitor" );
    if ( !mi || !mi->bi_extra ) {
        Debug( LDAP_DEBUG_CONFIG, "lload_monitor_open: "
                "monitor backend not available, monitoring disabled\n" );
        return 0;
    }
    mbe = mi->bi_extra;

    /* don't bother if monitor is not configured */
    if ( !mbe->is_configured() ) {
        static int warning = 0;

        if ( warning++ == 0 ) {
            Debug( LDAP_DEBUG_CONFIG, "lload_monitor_open: "
                    "monitoring disabled; "
                    "configure monitor database to enable\n" );
        }

        return 0;
    }

    if ( lload_monitor_initialized++ ) {
        return lload_monitor_initialized_failure;
    }

    argv[0] = "lload monitor";
    c.argv = argv;
    c.argc = 3;
    c.fname = argv[0];
    for ( i = 0; s_oid[i].name; i++ ) {
        argv[1] = s_oid[i].name;
        argv[2] = s_oid[i].oid;

        if ( parse_oidm( &c, 0, NULL ) != 0 ) {
            Debug( LDAP_DEBUG_ANY, "lload_monitor_open: "
                    "unable to add objectIdentifier \"%s=%s\"\n",
                    s_oid[i].name, s_oid[i].oid );
            return 2;
        }
    }

    for ( i = 0; s_at[i].desc != NULL; i++ ) {
        rc = register_at( s_at[i].desc, s_at[i].ad, 1 );
        if ( rc != LDAP_SUCCESS ) {
            Debug( LDAP_DEBUG_ANY, "lload_monitor_open: "
                    "register_at failed for attributeType (%s)\n",
                    s_at[i].desc );
            return 3;

        } else {
            (*s_at[i].ad)->ad_type->sat_flags |= SLAP_AT_HIDE;
        }
    }

    for ( i = 0; s_oc[i].desc != NULL; i++ ) {
        rc = register_oc( s_oc[i].desc, s_oc[i].oc, 1 );
        if ( rc != LDAP_SUCCESS ) {
            Debug( LDAP_DEBUG_ANY, "lload_monitor_open: "
                    "register_oc failed for objectClass (%s)\n",
                    s_oc[i].desc );
            return 4;

        } else {
            (*s_oc[i].oc)->soc_flags |= SLAP_OC_HIDE;
        }
    }

    for ( i = 0; s_moc[i].name != NULL; i++ ) {
        *s_moc[i].oc = oc_find( s_moc[i].name );
        if ( !*s_moc[i].oc ) {
            Debug( LDAP_DEBUG_ANY, "lload_monitor_open: "
                    "failed to find objectClass (%s)\n",
                    s_moc[i].name );
            return 5;
        }
    }

    /* register the subsystems - Servers are registered in backends_init */
    for ( mss = balancer_subsys, rdn = lload_subsys_rdn; mss->mss_name;
            mss++, rdn++ ) {
        ber_str2bv( *rdn, 0, 1, &mss->mss_rdn );
        if ( mbe->register_subsys_late( mss ) ) {
            Debug( LDAP_DEBUG_ANY, "lload_monitor_open: "
                    "failed to register %s subsystem\n",
                    mss->mss_name );
            return -1;
        }
    }

    checked_lock( &slapd_rq.rq_mutex );
    ldap_pvt_runqueue_insert( &slapd_rq, 1, lload_monitor_update_global_stats,
            NULL, "lload_monitor_update_global_stats", "lloadd" );
    checked_unlock( &slapd_rq.rq_mutex );

    return (lload_monitor_initialized_failure = LDAP_SUCCESS);
}
