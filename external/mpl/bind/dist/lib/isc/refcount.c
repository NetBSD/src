/*	$NetBSD: refcount.c,v 1.2 2018/08/12 13:02:37 christos Exp $	*/

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


#include <config.h>

#include <stddef.h>

#include <isc/mutex.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/util.h>

isc_result_t
isc_refcount_init(isc_refcount_t *ref, unsigned int n) {
	REQUIRE(ref != NULL);

	ref->refs = n;
#if defined(ISC_PLATFORM_USETHREADS) && !defined(ISC_REFCOUNT_HAVEATOMIC)
	return (isc_mutex_init(&ref->lock));
#else
	return (ISC_R_SUCCESS);
#endif
}
