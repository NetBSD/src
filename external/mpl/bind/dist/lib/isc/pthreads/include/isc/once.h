/*	$NetBSD: once.h,v 1.1.1.1 2018/08/12 12:08:28 christos Exp $	*/

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


#ifndef ISC_ONCE_H
#define ISC_ONCE_H 1

/*! \file */

#include <pthread.h>

#include <isc/platform.h>
#include <isc/result.h>

typedef pthread_once_t isc_once_t;

#ifdef ISC_PLATFORM_BRACEPTHREADONCEINIT
/*!
 * This accomodates systems that define PTHRAD_ONCE_INIT improperly.
 */
#define ISC_ONCE_INIT { PTHREAD_ONCE_INIT }
#else
/*!
 * This is the usual case.
 */
#define ISC_ONCE_INIT PTHREAD_ONCE_INIT
#endif

/* XXX We could do fancier error handling... */

#define isc_once_do(op, f) \
	((pthread_once((op), (f)) == 0) ? \
	 ISC_R_SUCCESS : ISC_R_UNEXPECTED)

#endif /* ISC_ONCE_H */
