/*	$NetBSD: module_init.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

/* module_init.c - module initialization functions */
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
__RCSID("$NetBSD: module_init.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "../servers/slapd/slap.h"
#include "../servers/slapd/slap-config.h"

#include "lload.h"
#include "lber_pvt.h"

#include "ldap_rq.h"

ldap_pvt_thread_t lloadd_main_thread;
struct lload_conf_info lload_info;

void *
lload_start_daemon( void *arg )
{
    int rc = 0;

    daemon_base = event_base_new();
    if ( !daemon_base ) {
        Debug( LDAP_DEBUG_ANY, "lload_start_daemon: "
                "main event base allocation failed\n" );
        rc = 1;
        goto done;
    }

    rc = lloadd_daemon( daemon_base );
done:
    if ( rc != LDAP_SUCCESS ) {
        assert( lloadd_inited == 0 );
        checked_lock( &lload_wait_mutex );
        ldap_pvt_thread_cond_signal( &lload_wait_cond );
        checked_unlock( &lload_wait_mutex );
    }
    return (void *)(uintptr_t)rc;
}

static int
lload_pause_cb( BackendInfo *bi )
{
    if ( daemon_base ) {
        lload_pause_server();
    }
    return 0;
}

static int
lload_unpause_cb( BackendInfo *bi )
{
    if ( daemon_base ) {
        lload_unpause_server();
    }
    return 0;
}

int
lload_back_open( BackendInfo *bi )
{
    int rc = 0;

    if ( slapMode & SLAP_TOOL_MODE ) {
        return 0;
    }

    /* This will fail if we ever try to instantiate more than one lloadd within
     * the process */
    epoch_init();

    if ( lload_tls_init() != 0 ) {
        return -1;
    }

    if ( lload_monitor_open() != 0 ) {
        return -1;
    }

    assert( lloadd_get_listeners() );

    checked_lock( &lload_wait_mutex );
    rc = ldap_pvt_thread_create( &lloadd_main_thread,
            0, lload_start_daemon, NULL );
    if ( !rc ) {
        ldap_pvt_thread_cond_wait( &lload_wait_cond, &lload_wait_mutex );
        if ( lloadd_inited != 1 ) {
            ldap_pvt_thread_join( lloadd_main_thread, (void *)NULL );
            rc = -1;
        }
    }
    checked_unlock( &lload_wait_mutex );
    return rc;
}

int
lload_back_close( BackendInfo *bi )
{
    if ( slapMode & SLAP_TOOL_MODE ) {
        return 0;
    }

    assert( lloadd_inited == 1 );

    checked_lock( &lload_wait_mutex );
    event_base_loopexit( daemon_base, NULL );
    ldap_pvt_thread_cond_wait( &lload_wait_cond, &lload_wait_mutex );
    checked_unlock( &lload_wait_mutex );
    ldap_pvt_thread_join( lloadd_main_thread, (void *)NULL );

    return 0;
}

int
lload_back_initialize( BackendInfo *bi )
{
    bi->bi_flags = SLAP_BFLAG_STANDALONE;
    bi->bi_open = lload_back_open;
    bi->bi_config = config_generic_wrapper;
    bi->bi_pause = lload_pause_cb;
    bi->bi_unpause = lload_unpause_cb;
    bi->bi_close = lload_back_close;
    bi->bi_destroy = 0;

    bi->bi_db_init = 0;
    bi->bi_db_config = 0;
    bi->bi_db_open = 0;
    bi->bi_db_close = 0;
    bi->bi_db_destroy = 0;

    bi->bi_op_bind = 0;
    bi->bi_op_unbind = 0;
    bi->bi_op_search = 0;
    bi->bi_op_compare = 0;
    bi->bi_op_modify = 0;
    bi->bi_op_modrdn = 0;
    bi->bi_op_add = 0;
    bi->bi_op_delete = 0;
    bi->bi_op_abandon = 0;

    bi->bi_extended = 0;

    bi->bi_chk_referrals = 0;

    bi->bi_connection_init = 0;
    bi->bi_connection_destroy = 0;

    if ( lload_global_init() ) {
        return -1;
    }

    bi->bi_private = &lload_info;
    return lload_back_init_cf( bi );
}

SLAP_BACKEND_INIT_MODULE( lload )
