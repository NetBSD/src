/*	$NetBSD: refcount.h,v 1.2.2.3 2019/01/18 08:49:58 pgoyette Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#pragma once

#include <inttypes.h>

#include <isc/assertions.h>
#include <isc/atomic.h>
#include <isc/error.h>
#include <isc/lang.h>
#include <isc/mutex.h>
#include <isc/platform.h>
#include <isc/types.h>

/*! \file isc/refcount.h
 * \brief Implements a locked reference counter.
 *
 * These macros uses C11(-like) atomic functions to implement reference
 * counting.  The isc_refcount_t type must not be accessed directly.
 */

ISC_LANG_BEGINDECLS

typedef atomic_uint_fast32_t isc_refcount_t;

/** \def isc_refcount_init(ref, n)
 *  \brief Initialize the reference counter.
 *  \param[in] ref pointer to reference counter.
 *  \param[in] n an initial number of references.
 *  \return nothing.
 *
 *  \warning No memory barrier are being imposed here.
 */
#define isc_refcount_init(target, value)			\
	atomic_init(target, value)

/** \def isc_refcount_current(ref)
 *  \brief Returns current number of references.
 *  \param[in] ref pointer to reference counter.
 *  \returns current value of reference counter.
 *
 *   Undo implict promotion to 64 bits in our Windows implementation of
 *   atomic_load_explicit() by casting to uint_fast32_t.
 */

#define isc_refcount_current(target)				\
	(uint_fast32_t)atomic_load_explicit(target, memory_order_acquire)

/** \def isc_refcount_destroy(ref)
 *  \brief a destructor that makes sure that all references were cleared.
 *  \param[in] ref pointer to reference counter.
 *  \returns nothing.
 */
#define isc_refcount_destroy(target)				\
	ISC_REQUIRE(isc_refcount_current(target) == 0)

/** \def isc_refcount_increment0(ref)
 *  \brief increases reference counter by 1.
 *  \param[in] ref pointer to reference counter.
 *  \returns previous value of reference counter.
 */
#define isc_refcount_increment0(target)				\
	isc_refcount_increment(target)

/** \def isc_refcount_increment(ref)
 *  \brief increases reference counter by 1.
 *  \param[in] ref pointer to reference counter.
 *  \returns previous value of reference counter.
 */
#define isc_refcount_increment(target)				\
	atomic_fetch_add_explicit(target, 1, memory_order_relaxed)

/** \def isc_refcount_decrement(ref)
 *  \brief decreases reference counter by 1.
 *  \param[in] ref pointer to reference counter.
 *  \returns previous value of reference counter.
 */
#define isc_refcount_decrement(target)				\
	atomic_fetch_sub_explicit(target, 1, memory_order_release)

ISC_LANG_ENDDECLS
