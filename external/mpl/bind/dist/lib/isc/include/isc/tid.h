/*	$NetBSD: tid.h,v 1.2 2025/01/26 16:25:43 christos Exp $	*/

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

#pragma once

#include <inttypes.h>

#include <isc/lang.h>
#include <isc/thread.h>

ISC_LANG_BEGINDECLS

#define ISC_TID_UNKNOWN UINT32_MAX

uint32_t
isc_tid_count(void);
/*%<
 * Returns the number of threads.
 */

extern thread_local uint32_t isc__tid_local;

static inline uint32_t
isc_tid(void) {
	return isc__tid_local;
}
/*%<
 * Returns the thread ID of the currently-running loop.
 */

/* Private */

void
isc__tid_init(uint32_t tid);

void
isc__tid_initcount(uint32_t count);

ISC_LANG_ENDDECLS
