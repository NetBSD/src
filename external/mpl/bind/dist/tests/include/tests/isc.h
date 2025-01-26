/*	$NetBSD: isc.h,v 1.4 2025/01/26 16:25:49 christos Exp $	*/

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

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/hash.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/timer.h>
#include <isc/util.h>
#include <isc/uv.h>

extern isc_mem_t     *mctx;
extern isc_log_t     *lctx;
extern isc_loop_t    *mainloop;
extern isc_loopmgr_t *loopmgr;
extern isc_nm_t	     *netmgr;
extern int	      ncpus;
extern unsigned int   workers;
extern bool	      debug;

int
setup_mctx(void **state);
int
teardown_mctx(void **state);

int
setup_workers(void **state);

int
setup_loopmgr(void **state);
int
teardown_loopmgr(void **state);

int
setup_netmgr(void **state);
int
teardown_netmgr(void **state);

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
	int setup_test_##name(void **state ISC_ATTR_UNUSED);

#define ISC_RUN_TEST_DECLARE(name) \
	void run_test_##name(void **state ISC_ATTR_UNUSED);

#define ISC_TEARDOWN_TEST_DECLARE(name) \
	int teardown_test_##name(void **state ISC_ATTR_UNUSED)

#define ISC_LOOP_TEST_CUSTOM_DECLARE(name, setup, teardown) \
	void run_test_##name(void **state ISC_ATTR_UNUSED); \
	void loop_test_##name(void *arg ISC_ATTR_UNUSED);

#define ISC_LOOP_TEST_DECLARE(name) \
	ISC_LOOP_TEST_CUSTOM_DECLARE(name, NULL, NULL)

#define ISC_LOOP_TEST_SETUP_DECLARE(name) \
	ISC_LOOP_TEST_CUSTOM_DECLARE(name, setup_loop_##name, NULL)

#define ISC_LOOP_TEST_SETUP_TEARDOWN_DECLARE(name)            \
	ISC_LOOP_TEST_CUSTOM_DECLARE(name, setup_loop_##name, \
				     teardown_loop_##name)

#define ISC_LOOP_TEST_TEARDOWN_DECLARE(name) \
	ISC_LOOP_TEST_CUSTOM_DECLARE(name, NULL, teardown_loop_##name)

#define ISC_LOOP_SETUP_DECLARE(name) \
	void setup_loop_##name(void *arg ISC_ATTR_UNUSED);

#define ISC_SETUP_TEST_IMPL(name)                            \
	int setup_test_##name(void **state ISC_ATTR_UNUSED); \
	int setup_test_##name(void **state ISC_ATTR_UNUSED)

#define ISC_RUN_TEST_IMPL(name)                             \
	void run_test_##name(void **state ISC_ATTR_UNUSED); \
	void run_test_##name(void **state ISC_ATTR_UNUSED)

#define ISC_TEARDOWN_TEST_IMPL(name)                            \
	int teardown_test_##name(void **state ISC_ATTR_UNUSED); \
	int teardown_test_##name(void **state ISC_ATTR_UNUSED)

#define ISC_TEST_LIST_START const struct CMUnitTest tests[] = {
#define ISC_TEST_LIST_END \
	}                 \
	;

#define ISC_LOOP_TEST_CUSTOM_IMPL(name, setup, teardown)                   \
	void run_test_##name(void **state ISC_ATTR_UNUSED);                \
	void loop_test_##name(void *arg ISC_ATTR_UNUSED);                  \
	void run_test_##name(void **state ISC_ATTR_UNUSED) {               \
		isc_job_cb setup_loop = setup;                             \
		isc_job_cb teardown_loop = teardown;                       \
		if (setup_loop != NULL) {                                  \
			isc_loop_setup(mainloop, setup_loop, state);       \
		}                                                          \
		if (teardown_loop != NULL) {                               \
			isc_loop_teardown(mainloop, teardown_loop, state); \
		}                                                          \
		isc_loop_setup(mainloop, loop_test_##name, state);         \
		isc_loopmgr_run(loopmgr);                                  \
	}                                                                  \
	void loop_test_##name(void *arg ISC_ATTR_UNUSED)

#define ISC_LOOP_TEST_IMPL(name) ISC_LOOP_TEST_CUSTOM_IMPL(name, NULL, NULL)

#define ISC_LOOP_TEST_SETUP_IMPL(name) \
	ISC_LOOP_TEST_CUSTOM_IMPL(name, setup_loop_##name, NULL)

#define ISC_LOOP_TEST_SETUP_TEARDOWN_IMPL(name) \
	ISC_LOOP_TEST_CUSTOM_IMPL(name, setup_loop_##name, teardown_loop_##name)

#define ISC_LOOP_TEST_TEARDOWN_IMPL(name) \
	ISC_LOOP_TEST_CUSTOM_IMPL(name, NULL, teardown_loop_##name)

#define ISC_LOOP_SETUP_IMPL(name)                          \
	void setup_loop_##name(void *arg ISC_ATTR_UNUSED); \
	void setup_loop_##name(void *arg ISC_ATTR_UNUSED)

#define ISC_LOOP_TEARDOWN_IMPL(name)                          \
	void teardown_loop_##name(void *arg ISC_ATTR_UNUSED); \
	void teardown_loop_##name(void *arg ISC_ATTR_UNUSED)

#define ISC_TEST_DECLARE(name) void run_test_##name(void **state);

#define ISC_TEST_LIST_START const struct CMUnitTest tests[] = {
#define ISC_TEST_LIST_END \
	}                 \
	;

#define ISC_TEST_MAIN ISC_TEST_MAIN_CUSTOM(NULL, NULL)

#define ISC_TEST_MAIN_CUSTOM(setup, teardown)                                           \
	int main(int argc, char **argv) {                                               \
		int		  c, r;                                                 \
		size_t		  i, j;                                                 \
		struct CMUnitTest selected[ARRAY_SIZE(tests)];                          \
                                                                                        \
		signal(SIGPIPE, SIG_IGN);                                               \
                                                                                        \
		memset(selected, 0, sizeof(selected));                                  \
                                                                                        \
		setup_mctx(NULL);                                                       \
		setup_workers(NULL);                                                    \
                                                                                        \
		while ((c = isc_commandline_parse(argc, argv, "dlt:")) != -1)           \
		{                                                                       \
			switch (c) {                                                    \
			case 'd':                                                       \
				debug = true;                                           \
				break;                                                  \
			case 'l':                                                       \
				for (i = 0;                                             \
				     i < (sizeof(tests) / sizeof(tests[0]));            \
				     i++)                                               \
				{                                                       \
					if (tests[i].name != NULL) {                    \
						fprintf(stdout, "%s\n",                 \
							tests[i].name);                 \
					}                                               \
				}                                                       \
				return (0);                                             \
			case 't':                                                       \
				for (i = 0; i < ARRAY_SIZE(tests) &&                    \
					    tests[i].name != NULL;                      \
				     i++)                                               \
				{                                                       \
					if (strcmp(tests[i].name,                       \
						   isc_commandline_argument) !=         \
					    0)                                          \
					{                                               \
						continue;                               \
					}                                               \
					for (j = 0;                                     \
					     j < ARRAY_SIZE(selected) &&                \
					     selected[j].name != NULL;                  \
					     j++)                                       \
					{                                               \
						if (strcmp(tests[j].name,               \
							   isc_commandline_argument) == \
						    0)                                  \
						{                                       \
							break;                          \
						}                                       \
					}                                               \
					if (j < ARRAY_SIZE(selected) &&                 \
					    selected[j].name == NULL)                   \
					{                                               \
						selected[j] = tests[i];                 \
						break;                                  \
					}                                               \
				}                                                       \
				if (i == ARRAY_SIZE(tests)) {                           \
					fprintf(stderr, "unknown test '%s'\n",          \
						isc_commandline_argument);              \
					exit(1);                                        \
				}                                                       \
				break;                                                  \
			default:                                                        \
				fprintf(stderr, "Usage: %s [-dl] [-t test]\n",          \
					argv[0]);                                       \
				exit(1);                                                \
			}                                                               \
		}                                                                       \
                                                                                        \
		if (selected[0].name != NULL) {                                         \
			r = cmocka_run_group_tests(selected, setup, teardown);          \
		} else {                                                                \
			r = cmocka_run_group_tests(tests, setup, teardown);             \
		}                                                                       \
                                                                                        \
		isc_mem_destroy(&mctx);                                                 \
                                                                                        \
		return (r);                                                             \
	}
