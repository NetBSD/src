/*	$NetBSD: resconf_test.c,v 1.3.4.1 2019/09/12 19:18:15 martin Exp $	*/

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

#include <config.h>

#if HAVE_CMOCKA

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <sched.h> /* IWYU pragma: keep */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/mem.h>
#include <isc/util.h>

#include <irs/types.h>
#include <irs/resconf.h>

static isc_mem_t *mctx = NULL;

static void
setup_test() {
	isc_result_t result;

	result = isc_mem_create(0, 0, &mctx);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * the caller might run from another directory, but tests
	 * that access test data files must first chdir to the proper
	 * location.
	 */
	assert_return_code(chdir(TESTS), 0);
}

/* test irs_resconf_load() */
static void
irs_resconf_load_test(void **state) {
	isc_result_t result;
	irs_resconf_t *resconf = NULL;
	unsigned int i;
	struct {
		const char *file;
		isc_result_t loadres;
		isc_result_t (*check)(irs_resconf_t *resconf);
		isc_result_t checkres;
	} tests[] = {
		{
			"testdata/domain.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/nameserver-v4.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/nameserver-v6.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/nameserver-v6-scoped.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options-debug.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options-ndots.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options-timeout.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options-unknown.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options-bad-ndots.conf", ISC_R_RANGE,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/options-empty.conf", ISC_R_UNEXPECTEDEND,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/port.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/resolv.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/search.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/sortlist-v4.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/timeout.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}, {
			"testdata/unknown.conf", ISC_R_SUCCESS,
			NULL, ISC_R_SUCCESS
		}

	};

	UNUSED(state);

	setup_test();

	for (i = 0; i < sizeof(tests)/sizeof(tests[1]); i++) {
		result = irs_resconf_load(mctx, tests[i].file, &resconf);
		if (result != tests[i].loadres) {
			fail_msg("# unexpected result %s loading %s",
				 isc_result_totext(result), tests[i].file);
		}

		if (result == ISC_R_SUCCESS && resconf == NULL) {
			fail_msg("# NULL on success loading %s",
				 tests[i].file);
		} else if (result != ISC_R_SUCCESS && resconf != NULL) {
			fail_msg("# non-NULL on failure loading %s",
				 tests[i].file);
		}

		if (resconf != NULL && tests[i].check != NULL) {
			result = (tests[i].check)(resconf);
			if (result != tests[i].checkres) {
				fail_msg("# unexpected result %s loading %s",
					 isc_result_totext(result),
					 tests[i].file);
			}
		}
		if (resconf != NULL) {
			irs_resconf_destroy(&resconf);
		}
	}

	isc_mem_detach(&mctx);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(irs_resconf_load_test),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif
