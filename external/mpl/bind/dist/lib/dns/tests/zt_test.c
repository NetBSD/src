/*	$NetBSD: zt_test.c,v 1.10 2023/01/25 21:43:31 christos Exp $	*/

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

#if HAVE_CMOCKA

#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/app.h>
#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/print.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/name.h>
#include <dns/view.h>
#include <dns/zone.h>
#include <dns/zt.h>

#include "dnstest.h"

struct args {
	void *arg1;
	void *arg2;
	bool arg3;
};

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dns_test_begin(NULL, true);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	dns_test_end();

	return (0);
}

static isc_result_t
count_zone(dns_zone_t *zone, void *uap) {
	int *nzones = (int *)uap;

	UNUSED(zone);

	*nzones += 1;
	return (ISC_R_SUCCESS);
}

static isc_result_t
load_done(dns_zt_t *zt, dns_zone_t *zone, isc_task_t *task) {
	/* We treat zt as a pointer to a boolean for testing purposes */
	atomic_bool *done = (atomic_bool *)zt;

	UNUSED(zone);
	UNUSED(task);

	atomic_store(done, true);
	isc_app_shutdown();
	return (ISC_R_SUCCESS);
}

static isc_result_t
all_done(void *arg) {
	atomic_bool *done = (atomic_bool *)arg;

	atomic_store(done, true);
	isc_app_shutdown();
	return (ISC_R_SUCCESS);
}

static void
start_zt_asyncload(isc_task_t *task, isc_event_t *event) {
	struct args *args = (struct args *)(event->ev_arg);

	UNUSED(task);

	dns_zt_asyncload(args->arg1, false, all_done, args->arg2);

	isc_event_free(&event);
}

static void
start_zone_asyncload(isc_task_t *task, isc_event_t *event) {
	struct args *args = (struct args *)(event->ev_arg);

	UNUSED(task);

	dns_zone_asyncload(args->arg1, args->arg3, load_done, args->arg2);
	isc_event_free(&event);
}

/* apply a function to a zone table */
static void
apply(void **state) {
	isc_result_t result;
	dns_zone_t *zone = NULL;
	dns_view_t *view = NULL;
	int nzones = 0;

	UNUSED(state);

	result = dns_test_makezone("foo", &zone, NULL, true);
	assert_int_equal(result, ISC_R_SUCCESS);

	view = dns_zone_getview(zone);
	assert_non_null(view->zonetable);

	assert_int_equal(nzones, 0);
	result = dns_zt_apply(view->zonetable, isc_rwlocktype_read, false, NULL,
			      count_zone, &nzones);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(nzones, 1);

	/* These steps are necessary so the zone can be detached properly */
	result = dns_test_setupzonemgr();
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dns_test_managezone(zone);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_test_releasezone(zone);
	dns_test_closezonemgr();

	/* The view was left attached in dns_test_makezone() */
	dns_view_detach(&view);
	dns_zone_detach(&zone);
}

/* asynchronous zone load */
static void
asyncload_zone(void **state) {
	isc_result_t result;
	int n;
	dns_zone_t *zone = NULL;
	dns_view_t *view = NULL;
	dns_db_t *db = NULL;
	FILE *zonefile, *origfile;
	char buf[4096];
	atomic_bool done;
	int i = 0;
	struct args args;

	UNUSED(state);

	atomic_init(&done, false);

	result = dns_test_makezone("foo", &zone, NULL, true);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_test_setupzonemgr();
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dns_test_managezone(zone);
	assert_int_equal(result, ISC_R_SUCCESS);

	view = dns_zone_getview(zone);
	assert_non_null(view->zonetable);

	assert_false(dns__zone_loadpending(zone));
	assert_false(atomic_load(&done));
	zonefile = fopen("./zone.data", "wb");
	assert_non_null(zonefile);
	origfile = fopen("./testdata/zt/zone1.db", "r+b");
	assert_non_null(origfile);
	n = fread(buf, 1, 4096, origfile);
	fclose(origfile);
	fwrite(buf, 1, n, zonefile);
	fflush(zonefile);

	dns_zone_setfile(zone, "./zone.data", dns_masterformat_text,
			 &dns_master_style_default);

	args.arg1 = zone;
	args.arg2 = &done;
	args.arg3 = false;
	isc_app_onrun(dt_mctx, maintask, start_zone_asyncload, &args);

	isc_app_run();
	while (dns__zone_loadpending(zone) && i++ < 5000) {
		dns_test_nap(1000);
	}
	assert_true(atomic_load(&done));
	/* The zone should now be loaded; test it */
	result = dns_zone_getdb(zone, &db);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_db_detach(&db);
	/*
	 * Add something to zone file, reload zone with newonly - it should
	 * not be reloaded.
	 */
	fprintf(zonefile, "\nb in b 1.2.3.4\n");
	fflush(zonefile);
	fclose(zonefile);

	args.arg1 = zone;
	args.arg2 = &done;
	args.arg3 = true;
	isc_app_onrun(dt_mctx, maintask, start_zone_asyncload, &args);

	isc_app_run();

	while (dns__zone_loadpending(zone) && i++ < 5000) {
		dns_test_nap(1000);
	}
	assert_true(atomic_load(&done));
	/* The zone should now be loaded; test it */
	result = dns_zone_getdb(zone, &db);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_db_detach(&db);

	/* Now reload it without newonly - it should be reloaded */
	args.arg1 = zone;
	args.arg2 = &done;
	args.arg3 = false;
	isc_app_onrun(dt_mctx, maintask, start_zone_asyncload, &args);

	isc_app_run();

	while (dns__zone_loadpending(zone) && i++ < 5000) {
		dns_test_nap(1000);
	}
	assert_true(atomic_load(&done));
	/* The zone should now be loaded; test it */
	result = dns_zone_getdb(zone, &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_non_null(db);
	if (db != NULL) {
		dns_db_detach(&db);
	}

	dns_test_releasezone(zone);
	dns_test_closezonemgr();

	dns_zone_detach(&zone);
	dns_view_detach(&view);
}

/* asynchronous zone table load */
static void
asyncload_zt(void **state) {
	isc_result_t result;
	dns_zone_t *zone1 = NULL, *zone2 = NULL, *zone3 = NULL;
	dns_view_t *view;
	dns_zt_t *zt = NULL;
	dns_db_t *db = NULL;
	atomic_bool done;
	int i = 0;
	struct args args;

	UNUSED(state);

	atomic_init(&done, false);

	result = dns_test_makezone("foo", &zone1, NULL, true);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_zone_setfile(zone1, "testdata/zt/zone1.db", dns_masterformat_text,
			 &dns_master_style_default);
	view = dns_zone_getview(zone1);

	result = dns_test_makezone("bar", &zone2, view, false);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_zone_setfile(zone2, "testdata/zt/zone1.db", dns_masterformat_text,
			 &dns_master_style_default);

	/* This one will fail to load */
	result = dns_test_makezone("fake", &zone3, view, false);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_zone_setfile(zone3, "testdata/zt/nonexistent.db",
			 dns_masterformat_text, &dns_master_style_default);

	zt = view->zonetable;
	assert_non_null(zt);

	result = dns_test_setupzonemgr();
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dns_test_managezone(zone1);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dns_test_managezone(zone2);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dns_test_managezone(zone3);
	assert_int_equal(result, ISC_R_SUCCESS);

	assert_false(dns__zone_loadpending(zone1));
	assert_false(dns__zone_loadpending(zone2));
	assert_false(atomic_load(&done));

	args.arg1 = zt;
	args.arg2 = &done;
	isc_app_onrun(dt_mctx, maintask, start_zt_asyncload, &args);

	isc_app_run();
	while (!atomic_load(&done) && i++ < 5000) {
		dns_test_nap(1000);
	}
	assert_true(atomic_load(&done));

	/* Both zones should now be loaded; test them */
	result = dns_zone_getdb(zone1, &db);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(db);
	if (db != NULL) {
		dns_db_detach(&db);
	}

	result = dns_zone_getdb(zone2, &db);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(db);
	if (db != NULL) {
		dns_db_detach(&db);
	}

	dns_test_releasezone(zone3);
	dns_test_releasezone(zone2);
	dns_test_releasezone(zone1);
	dns_test_closezonemgr();

	dns_zone_detach(&zone1);
	dns_zone_detach(&zone2);
	dns_zone_detach(&zone3);
	dns_view_detach(&view);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(apply, _setup, _teardown),
		cmocka_unit_test_setup_teardown(asyncload_zone, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(asyncload_zt, _setup,
						_teardown),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* if HAVE_CMOCKA */
