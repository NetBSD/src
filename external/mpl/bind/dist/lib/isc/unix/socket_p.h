/*	$NetBSD: socket_p.h,v 1.1.1.1 2018/08/12 12:08:24 christos Exp $	*/

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


#ifndef ISC_SOCKET_P_H
#define ISC_SOCKET_P_H

/*! \file */

#ifdef ISC_PLATFORM_NEEDSYSSELECTH
#include <sys/select.h>
#endif

typedef struct isc_socketwait isc_socketwait_t;
int isc__socketmgr_waitevents(isc_socketmgr_t *, struct timeval *,
			      isc_socketwait_t **);
isc_result_t isc__socketmgr_dispatch(isc_socketmgr_t *, isc_socketwait_t *);
#endif /* ISC_SOCKET_P_H */
