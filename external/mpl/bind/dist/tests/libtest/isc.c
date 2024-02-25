/*	$NetBSD: isc.c,v 1.2.2.2 2024/02/25 15:47:54 martin Exp $	*/

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

#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include "netmgr_p.h"
#include "task_p.h"
#include "timer_p.h"

#include <tests/isc.h>

isc_mem_t *mctx = NULL;
isc_taskmgr_t *taskmgr = NULL;
isc_timermgr_t *timermgr = NULL;
isc_nm_t *netmgr = NULL;
unsigned int workers = 0;
isc_task_t *maintask = NULL;

int
setup_managers(void **state) {
	isc_result_t result;

	UNUSED(state);

	REQUIRE(mctx != NULL);

	if (workers == 0) {
		char *env_workers = getenv("ISC_TASK_WORKERS");
		if (env_workers != NULL) {
			workers = atoi(env_workers);
		} else {
			workers = isc_os_ncpus();
		}
		INSIST(workers > 0);
	}

	result = isc_managers_create(mctx, workers, 0, &netmgr, &taskmgr,
				     &timermgr);
	if (result != ISC_R_SUCCESS) {
		return (-1);
	}

	result = isc_task_create_bound(taskmgr, 0, &maintask, 0);
	if (result != ISC_R_SUCCESS) {
		return (-1);
	}

	isc_taskmgr_setexcltask(taskmgr, maintask);

	return (0);
}

int
teardown_managers(void **state) {
	UNUSED(state);

	isc_task_detach(&maintask);
	isc_managers_destroy(&netmgr, &taskmgr, &timermgr);

	return (0);
}
