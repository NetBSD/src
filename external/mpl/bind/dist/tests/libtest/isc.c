/*	$NetBSD: isc.c,v 1.4 2025/01/26 16:25:51 christos Exp $	*/

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
#include <sys/resource.h>
#include <time.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/loop.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/string.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <tests/isc.h>

isc_mem_t *mctx = NULL;
isc_log_t *lctx = NULL;
isc_loop_t *mainloop = NULL;
isc_loopmgr_t *loopmgr = NULL;
isc_nm_t *netmgr = NULL;
unsigned int workers = 0;
bool debug = false;

static void
adjustnofile(void) {
	struct rlimit rl;

	if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
		if (rl.rlim_cur != rl.rlim_max) {
			rl.rlim_cur = rl.rlim_max;
			setrlimit(RLIMIT_NOFILE, &rl);
		}
	}
}

int
setup_workers(void **state ISC_ATTR_UNUSED) {
	char *env_workers = getenv("ISC_TASK_WORKERS");
	if (env_workers != NULL) {
		workers = atoi(env_workers);
	} else {
		workers = isc_os_ncpus();

		/* We always need at least two loops for some of the tests */
		if (workers < 2) {
			workers = 2;
		}
	}
	INSIST(workers != 0);

	return 0;
}

int
setup_mctx(void **state ISC_ATTR_UNUSED) {
	isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
	isc_mem_create(&mctx);

	return 0;
}

int
teardown_mctx(void **state ISC_ATTR_UNUSED) {
	isc_mem_destroy(&mctx);

	return 0;
}

int
setup_loopmgr(void **state ISC_ATTR_UNUSED) {
	REQUIRE(mctx != NULL);

	setup_workers(state);

	isc_loopmgr_create(mctx, workers, &loopmgr);
	mainloop = isc_loop_main(loopmgr);

	return 0;
}

int
teardown_loopmgr(void **state ISC_ATTR_UNUSED) {
	REQUIRE(netmgr == NULL);

	mainloop = NULL;
	isc_loopmgr_destroy(&loopmgr);

	return 0;
}

int
setup_netmgr(void **state ISC_ATTR_UNUSED) {
	REQUIRE(loopmgr != NULL);

	adjustnofile();

	isc_netmgr_create(mctx, loopmgr, &netmgr);

	return 0;
}

int
teardown_netmgr(void **state ISC_ATTR_UNUSED) {
	REQUIRE(loopmgr != NULL);

	isc_netmgr_destroy(&netmgr);

	return 0;
}

int
setup_managers(void **state) {
	setup_loopmgr(state);
	setup_netmgr(state);

	return 0;
}

int
teardown_managers(void **state) {
	teardown_netmgr(state);
	teardown_loopmgr(state);

	return 0;
}
