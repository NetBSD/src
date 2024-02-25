/*	$NetBSD: isc.h,v 1.2.2.2 2024/02/25 15:47:49 martin Exp $	*/

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

/*! \file */

#include <inttypes.h>
#include <stdbool.h>
#include <uv.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include "netmgr_p.h"
#include "task_p.h"
#include "timer_p.h"

#define CHECK(r)                             \
	do {                                 \
		result = (r);                \
		if (result != ISC_R_SUCCESS) \
			goto cleanup;        \
	} while (0)

extern isc_mem_t      *mctx;
extern isc_nm_t	      *netmgr;
extern isc_taskmgr_t  *taskmgr;
extern isc_timermgr_t *timermgr;
extern unsigned int    workers;
extern isc_task_t     *maintask;

#define isc_test_nap(ms) uv_sleep(ms)

int
setup_managers(void **state);
int
teardown_managers(void **state);

#ifndef TESTS_DIR
#define TESTS_DIR "./"
#endif

/* clang-format off */
/* Copied from cmocka */
#define ISC_TEST_ENTRY(name)				\
	{ #name, run_test_##name, NULL, NULL, NULL },
#define ISC_TEST_ENTRY_SETUP(name) \
	{ #name, run_test_##name, setup_test_##name, NULL, NULL },
#define ISC_TEST_ENTRY_TEARDOWN(name) \
	{ #name, run_test_##name, NULL, teardown_test_##name, NULL },
#define ISC_TEST_ENTRY_SETUP_TEARDOWN(name) \
	{ #name, run_test_##name, setup_test_##name, teardown_test_##name, NULL },
#define ISC_TEST_ENTRY_CUSTOM(name, setup, teardown) \
	{ #name, run_test_##name, setup, teardown, NULL },
/* clang-format on */

#define ISC_SETUP_TEST_DECLARE(name) \
	int setup_test_##name(void **state __attribute__((unused)));

#define ISC_RUN_TEST_DECLARE(name) \
	void run_test_##name(void **state __attribute__((unused)));

#define ISC_TEARDOWN_TEST_DECLARE(name) \
	int teardown_test_##name(void **state __attribute__((unused)))

#define ISC_SETUP_TEST_IMPL(name)                                    \
	int setup_test_##name(void **state __attribute__((unused))); \
	int setup_test_##name(void **state __attribute__((unused)))

#define ISC_RUN_TEST_IMPL(name)                                     \
	void run_test_##name(void **state __attribute__((unused))); \
	void run_test_##name(void **state __attribute__((unused)))

#define ISC_TEARDOWN_TEST_IMPL(name)                                    \
	int teardown_test_##name(void **state __attribute__((unused))); \
	int teardown_test_##name(void **state __attribute__((unused)))

#define ISC_TEST_LIST_START const struct CMUnitTest tests[] = {
#define ISC_TEST_LIST_END \
	}                 \
	;

#define ISC_TEST_MAIN ISC_TEST_MAIN_CUSTOM(NULL, NULL)

#define ISC_TEST_MAIN_CUSTOM(setup, teardown)                       \
	int main(void) {                                            \
		int r;                                              \
                                                                    \
		signal(SIGPIPE, SIG_IGN);                           \
                                                                    \
		isc_mem_debugging |= ISC_MEM_DEBUGRECORD;           \
		isc_mem_create(&mctx);                              \
                                                                    \
		r = cmocka_run_group_tests(tests, setup, teardown); \
                                                                    \
		isc_mem_destroy(&mctx);                             \
                                                                    \
		return (r);                                         \
	}
