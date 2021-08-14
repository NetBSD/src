/*	$NetBSD: epoch.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

/* epoch.c - epoch based memory reclamation */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2018-2021 The OpenLDAP Foundation.
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

/** @file epoch.c
 *
 * Implementation of epoch based memory reclamation, in principle
 * similar to the algorithm presented in
 * https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-579.pdf
 *
 * Not completely lock-free at the moment.
 *
 * Also the problems with epoch based memory reclamation are still
 * present - a thread actively observing an epoch getting stuck will
 * prevent managed objects (in our case connections and operations)
 * from being freed, potentially running out of memory.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: epoch.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include "lload.h"
#include <epoch.h>

/* Has to be >= 3 */
#define EPOCH_MASK ( 1 << 2 )
#define EPOCH_PREV(epoch) ( ( (epoch) + EPOCH_MASK - 1 ) % EPOCH_MASK )
#define EPOCH_NEXT(epoch) ( ( (epoch) + 1 ) % EPOCH_MASK )

struct pending_ref {
    void *object;
    dispose_cb *dispose;
    struct pending_ref *next;
};

ldap_pvt_thread_rdwr_t epoch_mutex;

static epoch_t current_epoch;
static uintptr_t epoch_threads[EPOCH_MASK];
static struct pending_ref *references[EPOCH_MASK];

void
epoch_init( void )
{
    epoch_t epoch;

    current_epoch = 0;
    for ( epoch = 0; epoch < EPOCH_MASK; epoch++ ) {
        assert( !epoch_threads[epoch] );
        assert( !references[epoch] );
    }

    ldap_pvt_thread_rdwr_init( &epoch_mutex );
}

void
epoch_shutdown( void )
{
    epoch_t epoch;
    struct pending_ref *old, *next;

    for ( epoch = 0; epoch < EPOCH_MASK; epoch++ ) {
        assert( !epoch_threads[epoch] );
    }

    /*
     * Even with the work in epoch_leave(), shutdown code doesn't currently
     * observe any epoch, so there might still be references left to free.
     */
    epoch = EPOCH_PREV(current_epoch);
    next = references[epoch];
    references[epoch] = NULL;
    for ( old = next; old; old = next ) {
        next = old->next;

        old->dispose( old->object );
        ch_free( old );
    }

    epoch = current_epoch;
    next = references[epoch];
    references[epoch] = NULL;
    for ( old = next; old; old = next ) {
        next = old->next;

        old->dispose( old->object );
        ch_free( old );
    }

    /* No references should exist anywhere now */
    for ( epoch = 0; epoch < EPOCH_MASK; epoch++ ) {
        assert( !references[epoch] );
    }

    ldap_pvt_thread_rdwr_destroy( &epoch_mutex );
}

epoch_t
epoch_join( void )
{
    epoch_t epoch;
    struct pending_ref *old, *ref = NULL;

retry:
    /* TODO: make this completely lock-free */
    ldap_pvt_thread_rdwr_rlock( &epoch_mutex );
    epoch = current_epoch;
    __atomic_add_fetch( &epoch_threads[epoch], 1, __ATOMIC_ACQ_REL );
    ldap_pvt_thread_rdwr_runlock( &epoch_mutex );

    if ( __atomic_load_n(
                 &epoch_threads[EPOCH_PREV(epoch)], __ATOMIC_ACQUIRE ) ) {
        return epoch;
    }

    __atomic_exchange(
            &references[EPOCH_PREV(epoch)], &ref, &ref, __ATOMIC_ACQ_REL );

    Debug( LDAP_DEBUG_TRACE, "epoch_join: "
            "advancing epoch to %zu with %s objects to free\n",
            EPOCH_NEXT(epoch), ref ? "some" : "no" );

    ldap_pvt_thread_rdwr_wlock( &epoch_mutex );
    current_epoch = EPOCH_NEXT(epoch);
    ldap_pvt_thread_rdwr_wunlock( &epoch_mutex );

    if ( !ref ) {
        return epoch;
    }

    /*
     * The below is now safe to free outside epochs and we don't want to make
     * the current epoch last any longer than necessary.
     *
     * Looks like there might be fairness issues in massively parallel
     * environments but they haven't been observed on 32-core machines.
     */
    epoch_leave( epoch );

    for ( old = ref; old; old = ref ) {
        ref = old->next;

        old->dispose( old->object );
        ch_free( old );
    }

    goto retry;
}

void
epoch_leave( epoch_t epoch )
{
    struct pending_ref *p, *next, *old_refs = NULL, *current_refs = NULL;

    /* Are there other threads observing our epoch? */
    if ( __atomic_sub_fetch( &epoch_threads[epoch], 1, __ATOMIC_ACQ_REL ) ) {
        return;
    }

    /*
     * Optimisation for the case when we're mostly idle. Otherwise we won't
     * release resources until another thread comes by and joins the epoch
     * (twice), and there's no idea how soon (or late) that is going to happen.
     *
     * NB. There is no limit to the number of threads executing the following
     * code in parallel.
     */
    ldap_pvt_thread_rdwr_rlock( &epoch_mutex );
    /*
     * Anything could happen between the subtract and the lock being acquired
     * above, so check again. But once we hold this lock (and confirm no more
     * threads still observe either prospective epoch), noone will be able to
     * finish epoch_join until we've released epoch_mutex since it holds that:
     *
     * epoch_threads[EPOCH_PREV(current_epoch)] == 0
     *
     * and that leads epoch_join() to acquire a write lock on &epoch_mutex.
     */
    if ( __atomic_load_n( &epoch_threads[epoch], __ATOMIC_RELAXED ) ) {
        /* Epoch counter has run full circle */
        ldap_pvt_thread_rdwr_runlock( &epoch_mutex );
        return;
    } else if ( epoch == current_epoch ) {
        if ( __atomic_load_n(
                     &epoch_threads[EPOCH_PREV(epoch)], __ATOMIC_RELAXED ) ) {
            /* There is another (older) thread still running */
            ldap_pvt_thread_rdwr_runlock( &epoch_mutex );
            return;
        }

        /* We're all alone, it's safe to claim all references and free them. */
        __atomic_exchange( &references[EPOCH_PREV(epoch)], &old_refs,
                &old_refs, __ATOMIC_ACQ_REL );
        __atomic_exchange( &references[epoch], &current_refs, &current_refs,
                __ATOMIC_ACQ_REL );
    } else if ( epoch == EPOCH_PREV(current_epoch) ) {
        if ( __atomic_load_n(
                     &epoch_threads[EPOCH_NEXT(epoch)], __ATOMIC_RELAXED ) ) {
            /* There is another (newer) thread still running */
            ldap_pvt_thread_rdwr_runlock( &epoch_mutex );
            return;
        }

        /* We're all alone, it's safe to claim all references and free them. */
        __atomic_exchange(
                &references[epoch], &old_refs, &old_refs, __ATOMIC_ACQ_REL );
        __atomic_exchange( &references[EPOCH_NEXT(epoch)], &current_refs,
                &current_refs, __ATOMIC_ACQ_REL );
    }
    /*
     * Else the current_epoch has moved far enough that no references remain to
     * be freed.
     */
    ldap_pvt_thread_rdwr_runlock( &epoch_mutex );

    /*
     * Trigger a memory-independent read fence to make sure we're reading the
     * state after all threads actually finished - which might have happened
     * after we acquired epoch_mutex so ldap_pvt_thread_rdwr_rlock would not
     * catch everything.
     *
     * TODO is to confirm the below:
     * It might be that the tests and exchanges above only enforce a fence for
     * the locations affected, so we could still read stale memory for
     * unrelated locations? At least that's the only explanation I've been able
     * to establish for repeated crashes that seem to have gone away with this
     * in place.
     *
     * But then that's contrary to the second example in Acquire/Release
     * section here:
     * https://gcc.gnu.org/wiki/Atomic/GCCMM/AtomicSync
     */
    __atomic_thread_fence( __ATOMIC_ACQUIRE );

    for ( p = old_refs; p; p = next ) {
        next = p->next;

        p->dispose( p->object );
        ch_free( p );
    }

    for ( p = current_refs; p; p = next ) {
        next = p->next;

        p->dispose( p->object );
        ch_free( p );
    }
}

/*
 * Add the object to the "current global epoch", not the epoch our thread
 * entered.
 */
void
epoch_append( void *ptr, dispose_cb *cb )
{
    struct pending_ref *new;
    epoch_t epoch = __atomic_load_n( &current_epoch, __ATOMIC_ACQUIRE );

    /*
     * BTW, the following is not appropriate here:
     * assert( __atomic_load_n( &epoch_threads[epoch], __ATOMIC_RELAXED ) );
     *
     * We might be a thread lagging behind in the "previous epoch" with no
     * other threads executing at all.
     */

    new = ch_malloc( sizeof(struct pending_ref) );
    new->object = ptr;
    new->dispose = cb;
    new->next = __atomic_load_n( &references[epoch], __ATOMIC_ACQUIRE );

    while ( !__atomic_compare_exchange( &references[epoch], &new->next, &new, 0,
            __ATOMIC_RELEASE, __ATOMIC_RELAXED ) )
        /* iterate until we succeed */;
}

int
acquire_ref( uintptr_t *refp )
{
    uintptr_t refcnt, new_refcnt;

    refcnt = __atomic_load_n( refp, __ATOMIC_ACQUIRE );

    /*
     * If we just incremented the refcnt and checked for zero after, another
     * thread might falsely believe the object was going to stick around.
     *
     * Checking whether the object is still dead at disposal time might not be
     * able to distinguish it from being freed in a later epoch.
     */
    do {
        if ( !refcnt ) {
            return refcnt;
        }

        new_refcnt = refcnt + 1;
    } while ( !__atomic_compare_exchange( refp, &refcnt, &new_refcnt, 0,
            __ATOMIC_RELEASE, __ATOMIC_RELAXED ) );
    assert( new_refcnt == refcnt + 1 );

    return refcnt;
}

int
try_release_ref( uintptr_t *refp, void *object, dispose_cb *cb )
{
    uintptr_t refcnt, new_refcnt;

    refcnt = __atomic_load_n( refp, __ATOMIC_ACQUIRE );

    /* We promise the caller that we won't decrease refcnt below 0 */
    do {
        if ( !refcnt ) {
            return refcnt;
        }

        new_refcnt = refcnt - 1;
    } while ( !__atomic_compare_exchange( refp, &refcnt, &new_refcnt, 0,
            __ATOMIC_RELEASE, __ATOMIC_RELAXED ) );
    assert( new_refcnt == refcnt - 1 );

    if ( !new_refcnt ) {
        epoch_append( object, cb );
    }

    return refcnt;
}
