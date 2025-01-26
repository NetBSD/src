/*	$NetBSD: job_p.h,v 1.2 2025/01/26 16:25:37 christos Exp $	*/

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

#include <isc/job.h>
#include <isc/loop.h>
#include <isc/os.h>
#include <isc/uv.h>

/*%
 * NOTE: We are using struct __cds_wfcq_head that doesn't have an internal
 * mutex, because we are only using enqueue and splice, and those don't need
 * any synchronization (see urcu/wfcqueue.h for detailed description).
 */
STATIC_ASSERT(ISC_OS_CACHELINE_SIZE >= sizeof(struct __cds_wfcq_head),
	      "ISC_OS_CACHELINE_SIZE smaller than "
	      "sizeof(struct __cds_wfcq_head)");

typedef struct isc_jobqueue {
	struct __cds_wfcq_head head;
	uint8_t __padding[ISC_OS_CACHELINE_SIZE -
			  sizeof(struct __cds_wfcq_head)];
	struct cds_wfcq_tail tail;
} isc_jobqueue_t;

typedef ISC_LIST(isc_job_t) isc_joblist_t;

void
isc__job_cb(uv_idle_t *handle);

void
isc__job_close(uv_handle_t *handle);
