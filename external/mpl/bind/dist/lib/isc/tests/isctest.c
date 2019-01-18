/*	$NetBSD: isctest.c,v 1.2.2.3 2019/01/18 08:50:00 pgoyette Exp $	*/

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
#include <stdlib.h>
#include <time.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/socket.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include "isctest.h"

isc_mem_t *mctx = NULL;
isc_log_t *lctx = NULL;
isc_taskmgr_t *taskmgr = NULL;
isc_timermgr_t *timermgr = NULL;
isc_socketmgr_t *socketmgr = NULL;
isc_task_t *maintask = NULL;
int ncpus;

static bool test_running = false;

/*
 * Logging categories: this needs to match the list in bin/named/log.c.
 */
static isc_logcategory_t categories[] = {
		{ "",                0 },
		{ "client",          0 },
		{ "network",         0 },
		{ "update",          0 },
		{ "queries",         0 },
		{ "unmatched",       0 },
		{ "update-security", 0 },
		{ "query-errors",    0 },
		{ NULL,              0 }
};

static void
cleanup_managers(void) {
	if (maintask != NULL) {
		isc_task_shutdown(maintask);
		isc_task_destroy(&maintask);
	}
	if (socketmgr != NULL) {
		isc_socketmgr_destroy(&socketmgr);
	}
	if (taskmgr != NULL) {
		isc_taskmgr_destroy(&taskmgr);
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

	CHECK(isc_taskmgr_create(mctx, workers, 0, &taskmgr));
	CHECK(isc_task_create(taskmgr, 0, &maintask));
	isc_taskmgr_setexcltask(taskmgr, maintask);

	CHECK(isc_timermgr_create(mctx, &timermgr));
	CHECK(isc_socketmgr_create(mctx, &socketmgr));
	return (ISC_R_SUCCESS);

 cleanup:
	cleanup_managers();
	return (result);
}

isc_result_t
isc_test_begin(FILE *logfile, bool start_managers,
	       unsigned int workers)
{
	isc_result_t result;

	INSIST(!test_running);
	test_running = true;

	isc_mem_debugging |= ISC_MEM_DEBUGRECORD;

	INSIST(mctx == NULL);
	CHECK(isc_mem_create(0, 0, &mctx));

	if (logfile != NULL) {
		isc_logdestination_t destination;
		isc_logconfig_t *logconfig = NULL;

		INSIST(lctx == NULL);
		CHECK(isc_log_create(mctx, &lctx, &logconfig));

		isc_log_registercategories(lctx, categories);
		isc_log_setcontext(lctx);

		destination.file.stream = logfile;
		destination.file.name = NULL;
		destination.file.versions = ISC_LOG_ROLLNEVER;
		destination.file.maximum_size = 0;
		CHECK(isc_log_createchannel(logconfig, "stderr",
					    ISC_LOG_TOFILEDESC,
					    ISC_LOG_DYNAMIC,
					    &destination, 0));
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
	if (taskmgr != NULL) {
		isc_taskmgr_destroy(&taskmgr);
	}

	cleanup_managers();

	if (lctx != NULL) {
		isc_log_destroy(&lctx);
	}
	if (mctx != NULL) {
		isc_mem_destroy(&mctx);
	}

	test_running = false;
}

/*
 * Sleep for 'usec' microseconds.
 */
void
isc_test_nap(uint32_t usec) {
#ifdef HAVE_NANOSLEEP
	struct timespec ts;

	ts.tv_sec = usec / 1000000;
	ts.tv_nsec = (usec % 1000000) * 1000;
	nanosleep(&ts, NULL);
#elif HAVE_USLEEP
	usleep(usec);
#else
	/*
	 * No fractional-second sleep function is available, so we
	 * round up to the nearest second and sleep instead
	 */
	sleep((usec / 1000000) + 1);
#endif
}
