/*	$NetBSD: isctest.h,v 1.3.2.2 2019/06/10 22:04:45 christos Exp $	*/

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

/*! \file */

#include <config.h>

#include <inttypes.h>
#include <stdbool.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#define CHECK(r) \
	do { \
		result = (r); \
		if (result != ISC_R_SUCCESS) \
			goto cleanup; \
	} while (/*CONSTCOND*/0)

extern isc_mem_t *mctx;
extern isc_log_t *lctx;
extern isc_taskmgr_t *taskmgr;
extern isc_timermgr_t *timermgr;
extern isc_socketmgr_t *socketmgr;
extern int ncpus;

isc_result_t
isc_test_begin(FILE *logfile, bool start_managers,
	       unsigned int workers);
/*%<
 * Begin test, logging to 'logfile' or default if not specified.
 *
 * If 'start_managers' is set, start a task manager, timer manager,
 * and socket manager.
 *
 * If 'workers' is zero, use the number of CPUs on the system as a default;
 * otherwise, set up the task manager with the specified number of worker
 * threads. The environment variable ISC_TASK_WORKERS overrides this value.
 */

void
isc_test_end(void);

void
isc_test_nap(uint32_t usec);
