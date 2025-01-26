/*	$NetBSD: job.h,v 1.1.1.1 2025/01/26 16:12:31 christos Exp $	*/

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

/*! \file isc/job.h
 * \brief The isc_job unit provides a way to schedule jobs on the
 * currently running isc event loop. This is distinct from isc_async,
 * which can be used to send events to another, specified loop.
 *
 * The unit is built around the uv_idle_t primitive and it directly schedules
 * the callback to be run on the isc event loop.
 */

#pragma once

#include <inttypes.h>

#include <isc/lang.h>
#include <isc/mem.h>
#include <isc/refcount.h>
#include <isc/types.h>
#include <isc/urcu.h>

typedef void (*isc_job_cb)(void *);
typedef struct isc_job isc_job_t;

struct isc_job {
	isc_job_cb cb;
	void	  *cbarg;
	union {
		struct cds_wfcq_node wfcq_node;
		ISC_LINK(isc_job_t) link;
	};
};

#define ISC_JOB_INITIALIZER                   \
	{                                     \
		.link = ISC_LINK_INITIALIZER, \
	}

ISC_LANG_BEGINDECLS

void
isc_job_run(isc_loop_t *loop, isc_job_t *job, isc_job_cb cb, void *cbarg);
/*%<
 * Schedule the job callback 'cb' to be run on the currently
 * running event loop.
 *
 * Requires:
 *
 *\li	'loop' is the current loop
 *\li	'job' is initialized
 *\li	'cb' is a callback function, must be non-NULL
 *\li	'cbarg' is passed to the 'cb' as the only argument, may be NULL
 */

ISC_LANG_ENDDECLS
