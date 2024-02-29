/*	$NetBSD: timer_p.h,v 1.1.4.2 2024/02/29 11:39:02 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef ISC_TIMER_P_H
#define ISC_TIMER_P_H

/*! \file */

isc_result_t
isc__timermgr_nextevent(isc_timermgr_t *timermgr, isc_time_t *when);

void
isc__timermgr_dispatch(isc_timermgr_t *timermgr);

#endif /* ISC_TIMER_P_H */
