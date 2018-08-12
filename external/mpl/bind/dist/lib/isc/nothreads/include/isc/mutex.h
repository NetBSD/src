/*	$NetBSD: mutex.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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

#ifndef ISC_MUTEX_H
#define ISC_MUTEX_H 1

#include <isc/result.h>		/* for ISC_R_ codes */

typedef int isc_mutex_t;

#define isc_mutex_init(mp) \
	(*(mp) = 0, ISC_R_SUCCESS)
#define isc_mutex_lock(mp) \
	((*(mp))++ == 0 ? ISC_R_SUCCESS : ISC_R_UNEXPECTED)
#define isc_mutex_unlock(mp) \
	(--(*(mp)) == 0 ? ISC_R_SUCCESS : ISC_R_UNEXPECTED)
#define isc_mutex_trylock(mp) \
	(*(mp) == 0 ? ((*(mp))++, ISC_R_SUCCESS) : ISC_R_LOCKBUSY)
#define isc_mutex_destroy(mp) \
	(*(mp) == 0 ? (*(mp) = -1, ISC_R_SUCCESS) : ISC_R_UNEXPECTED)
#define isc_mutex_stats(fp)

#endif /* ISC_MUTEX_H */
