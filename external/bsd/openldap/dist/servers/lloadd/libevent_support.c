/*	$NetBSD: libevent_support.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

/* libevent_support.c - routines to bridge libldap and libevent */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2017-2021 The OpenLDAP Foundation.
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
__RCSID("$NetBSD: libevent_support.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include <ac/time.h>

#include <event2/event.h>
#include <event2/thread.h>

#include "lload.h"
#include "ldap_pvt_thread.h"

static void *
lload_libevent_mutex_init( unsigned locktype )
{
    int rc;
    ldap_pvt_thread_mutex_t *mutex =
            ch_malloc( sizeof(ldap_pvt_thread_mutex_t) );

    if ( locktype & EVTHREAD_LOCKTYPE_RECURSIVE ) {
        rc = ldap_pvt_thread_mutex_recursive_init( mutex );
    } else {
        rc = ldap_pvt_thread_mutex_init( mutex );
    }
    if ( rc ) {
        ch_free( mutex );
        mutex = NULL;
    }
    return mutex;
}

static void
lload_libevent_mutex_destroy( void *lock, unsigned locktype )
{
    int rc;
    ldap_pvt_thread_mutex_t *mutex = lock;

    rc = ldap_pvt_thread_mutex_destroy( mutex );
    assert( rc == 0 );
    ch_free( mutex );
}

static int
lload_libevent_mutex_lock( unsigned mode, void *lock )
{
    ldap_pvt_thread_mutex_t *mutex = lock;

    if ( mode & EVTHREAD_TRY ) {
        return ldap_pvt_thread_mutex_trylock( mutex );
    } else {
        return ldap_pvt_thread_mutex_lock( mutex );
    }
}

static int
lload_libevent_mutex_unlock( unsigned mode, void *lock )
{
    ldap_pvt_thread_mutex_t *mutex = lock;

    return ldap_pvt_thread_mutex_unlock( mutex );
}

static void *
lload_libevent_cond_init( unsigned condtype )
{
    int rc;
    ldap_pvt_thread_cond_t *cond =
            ch_malloc( sizeof(ldap_pvt_thread_cond_t) );

    assert( condtype == 0 );
    rc = ldap_pvt_thread_cond_init( cond );
    if ( rc ) {
        ch_free( cond );
        cond = NULL;
    }
    return cond;
}

static void
lload_libevent_cond_destroy( void *c )
{
    int rc;
    ldap_pvt_thread_cond_t *cond = c;

    rc = ldap_pvt_thread_cond_destroy( cond );
    assert( rc == 0 );
    ch_free( c );
}

static int
lload_libevent_cond_signal( void *c, int broadcast )
{
    ldap_pvt_thread_cond_t *cond = c;

    if ( broadcast ) {
        return ldap_pvt_thread_cond_broadcast( cond );
    } else {
        return ldap_pvt_thread_cond_signal( cond );
    }
}

static int
lload_libevent_cond_timedwait(
        void *c,
        void *lock,
        const struct timeval *timeout )
{
    ldap_pvt_thread_cond_t *cond = c;
    ldap_pvt_thread_mutex_t *mutex = lock;

    /*
     * libevent does not seem to request a timeout, this is true as of 2.1.8
     * that has just been marked the first stable release of the 2.1 series
     */
    assert( timeout == NULL );

    return ldap_pvt_thread_cond_wait( cond, mutex );
}

int
lload_libevent_init( void )
{
    struct evthread_lock_callbacks cbs = {
        EVTHREAD_LOCK_API_VERSION,
        EVTHREAD_LOCKTYPE_RECURSIVE,
        lload_libevent_mutex_init,
        lload_libevent_mutex_destroy,
        lload_libevent_mutex_lock,
        lload_libevent_mutex_unlock
    };
    struct evthread_condition_callbacks cond_cbs = {
        EVTHREAD_CONDITION_API_VERSION,
        lload_libevent_cond_init,
        lload_libevent_cond_destroy,
        lload_libevent_cond_signal,
        lload_libevent_cond_timedwait
    };

#ifndef BALANCER_MODULE
    /* only necessary if lload is a server, slapd already calls
     * ldap_pvt_thread_initialize() */
    if ( ldap_pvt_thread_initialize() ) {
        return -1;
    }
#endif

    evthread_set_lock_callbacks( &cbs );
    evthread_set_condition_callbacks( &cond_cbs );
    evthread_set_id_callback( ldap_pvt_thread_self );
    return 0;
}

void
lload_libevent_destroy( void )
{
    libevent_global_shutdown();
}
