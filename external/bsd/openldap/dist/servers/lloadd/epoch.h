/*	$NetBSD: epoch.h,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

/* epoch.h - epoch based memory reclamation */
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

#ifndef __LLOAD_EPOCH_H
#define __LLOAD_EPOCH_H

/** @file epoch.h
 *
 * Implementation of epoch based memory reclamation, in principle
 * similar to the algorithm presented in
 * https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-579.pdf
 */

typedef uintptr_t epoch_t;

/** @brief A callback function used to free object and associated data */
typedef void (dispose_cb)( void *object );

/** @brief Initiate global state */
void epoch_init( void );

/** @brief Finalise global state and free any objects still pending */
void epoch_shutdown( void );

/** @brief Register thread as active
 *
 * In order to safely access managed objects, a thread should call
 * this function or make sure no other thread is running (e.g. config
 * pause, late shutdown). After calling this, it is guaranteed that no
 * reachable objects will be freed before all threads have called
 * `epoch_leave( current_epoch + 1 )` so it is essential that there
 * is an upper limit to the amount of time between #epoch_join and
 * corresponding #epoch_leave or the number of unfreed objects might
 * grow without bounds.
 *
 * To simplify locking, memory is only freed when the current epoch
 * is advanced rather than on leaving it.
 *
 * Can be safely called multiple times by the same thread as long as
 * a matching #epoch_leave() call is made eventually.
 *
 * @return The observed epoch, to be passed to #epoch_leave()
 */
epoch_t epoch_join( void );

/** @brief Register thread as inactive
 *
 * A thread should call this after they are finished with work
 * performed since matching call to #epoch_join(). It is not safe
 * to keep a local reference to managed objects after this call
 * unless other precautions have been made to prevent it being
 * released.
 *
 * @param[in] epoch Epoch identifier returned by a previous call to
 * #epoch_join().
 */
void epoch_leave( epoch_t epoch );

/** @brief Return an unreachable object to be freed
 *
 * The object should already be unreachable at the point of call and
 * cb will be invoked when no other thread that could have seen it
 * is active any more. This happens when we have advanced by two
 * epochs.
 *
 * @param[in] ptr Object to be released/freed
 * @param[in] cb Callback to invoke when safe to do so
 */
void epoch_append( void *ptr, dispose_cb *cb );

/**
 * \defgroup Reference counting helpers
 */
/**@{*/

/** @brief Acquire a reference if possible
 *
 * Atomically, check reference count is non-zero and increment if so.
 * Returns old reference count.
 *
 * @param[in] refp Pointer to a reference counter
 * @return 0 if reference was already zero, non-zero if reference
 * count was successfully incremented
 */
int acquire_ref( uintptr_t *refp );

/** @brief Check reference count and try to decrement
 *
 * Atomically, decrement reference count if non-zero and register
 * object if decremented to zero. Returning previous reference count.
 *
 * @param[in] refp Pointer to a reference counter
 * @param[in] object The managed object
 * @param[in] cb Callback to invoke when safe to do so
 * @return 0 if reference was already zero, non-zero if reference
 * count was non-zero at the time of call
 */
int try_release_ref( uintptr_t *refp, void *object, dispose_cb *cb );

/** @brief Read reference count
 *
 * @param[in] object Pointer to the managed object
 * @param[in] ref_field Member where reference count is stored in
 * the object
 * @return Current value of reference counter
 */
#define IS_ALIVE( object, ref_field ) \
    __atomic_load_n( &(object)->ref_field, __ATOMIC_ACQUIRE )

/** @brief Release reference
 *
 * A cheaper alternative to #try_release_ref(), safe only when we know
 * reference count was already non-zero.
 *
 * @param[in] object The managed object
 * @param[in] ref_field Member where reference count is stored in
 * the object
 * @param[in] cb Callback to invoke when safe to do so
 */
#define RELEASE_REF( object, ref_field, cb ) \
    do { \
        assert( IS_ALIVE( (object), ref_field ) ); \
        if ( !__atomic_sub_fetch( \
                     &(object)->ref_field, 1, __ATOMIC_ACQ_REL ) ) { \
            epoch_append( object, (dispose_cb *)cb ); \
        } \
    } while (0)

/**@}*/

#endif /* __LLOAD_EPOCH_H */
