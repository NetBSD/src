/*	$NetBSD: isctest.c,v 1.7 2022/09/23 12:15:34 christos Exp $	*/

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

/*! \file */

#include "isctest.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/socket.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

isc_mem_t *test_mctx = NULL;
isc_log_t *test_lctx = NULL;
isc_taskmgr_t *taskmgr = NULL;
isc_timermgr_t *timermgr = NULL;
isc_socketmgr_t *socketmgr = NULL;
isc_nm_t *netmgr = NULL;
isc_task_t *maintask = NULL;
int ncpus;

static bool test_running = false;

/*
 * Logging categories: this needs to match the list in bin/named/log.c.
 */
static isc_logcategory_t categories[] = { { "", 0 },
					  { "client", 0 },
					  { "network", 0 },
					  { "update", 0 },
					  { "queries", 0 },
					  { "unmatched", 0 },
					  { "update-security", 0 },
					  { "query-errors", 0 },
					  { NULL, 0 } };

static void
cleanup_managers(void) {
	if (maintask != NULL) {
		isc_task_shutdown(maintask);
		isc_task_destroy(&maintask);
	}

	isc_managers_destroy(netmgr == NULL ? NULL : &netmgr,
			     taskmgr == NULL ? NULL : &taskmgr);

	if (socketmgr != NULL) {
		isc_socketmgr_destroy(&socketmgr);
	}
	if (timermgr != NULL) {
		isc_timermgr_destroy(&timermgr);
	}
}

static isc_result_t
create_managers(unsigned int workers) {
	isc_result_t result;
	char *p;

	if (workers == 0) {
		workers = isc_os_ncpus();
	}

	p = getenv("ISC_TASK_WORKERS");
	if (p != NULL) {
		workers = atoi(p);
	}

	CHECK(isc_managers_create(test_mctx, workers, 0, &netmgr, &taskmgr));
	CHECK(isc_task_create_bound(taskmgr, 0, &maintask, 0));
	isc_taskmgr_setexcltask(taskmgr, maintask);

	CHECK(isc_timermgr_create(test_mctx, &timermgr));
	CHECK(isc_socketmgr_create(test_mctx, &socketmgr));
	return (ISC_R_SUCCESS);

cleanup:
	cleanup_managers();
	return (result);
}

isc_result_t
isc_test_begin(FILE *logfile, bool start_managers, unsigned int workers) {
	isc_result_t result;

	INSIST(!test_running);
	test_running = true;

	isc_mem_debugging |= ISC_MEM_DEBUGRECORD;

	INSIST(test_mctx == NULL);
	isc_mem_create(&test_mctx);

	if (logfile != NULL) {
		isc_logdestination_t destination;
		isc_logconfig_t *logconfig = NULL;

		INSIST(test_lctx == NULL);
		isc_log_create(test_mctx, &test_lctx, &logconfig);
		isc_log_registercategories(test_lctx, categories);
		isc_log_setcontext(test_lctx);

		destination.file.stream = logfile;
		destination.file.name = NULL;
		destination.file.versions = ISC_LOG_ROLLNEVER;
		destination.file.maximum_size = 0;
		isc_log_createchannel(logconfig, "stderr", ISC_LOG_TOFILEDESC,
				      ISC_LOG_DYNAMIC, &destination, 0);
		CHECK(isc_log_usechannel(logconfig, "stderr", NULL, NULL));
	}

	ncpus = isc_os_ncpus();

	if (start_managers) {
		CHECK(create_managers(workers));
	}

	return (ISC_R_SUCCESS);

cleanup:
	isc_test_end();
	return (result);
}

void
isc_test_end(void) {
	if (maintask != NULL) {
		isc_task_detach(&maintask);
	}

	cleanup_managers();

	if (test_lctx != NULL) {
		isc_log_destroy(&test_lctx);
	}
	if (test_mctx != NULL) {
		isc_mem_destroy(&test_mctx);
	}

	test_running = false;
}

/*
 * Sleep for 'usec' microseconds.
 */
void
isc_test_nap(uint32_t usec) {
	struct timespec ts;

	ts.tv_sec = usec / 1000000;
	ts.tv_nsec = (usec % 1000000) * 1000;
	nanosleep(&ts, NULL);
}
