/*	$NetBSD: duration_test.c,v 1.2 2020/05/24 19:46:29 christos Exp $	*/

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

#if HAVE_CMOCKA

#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/lex.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/types.h>
#include <isc/util.h>

#include <isccfg/cfg.h>
#include <isccfg/grammar.h>
#include <isccfg/namedconf.h>

#define CHECK(r)                             \
	do {                                 \
		result = (r);                \
		if (result != ISC_R_SUCCESS) \
			goto cleanup;        \
	} while (/*CONSTCOND*/0)

isc_mem_t *mctx = NULL;
isc_log_t *lctx = NULL;
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
cleanup() {
	if (lctx != NULL) {
		isc_log_destroy(&lctx);
	}
	if (mctx != NULL) {
		isc_mem_destroy(&mctx);
	}
}

static isc_result_t
setup() {
	isc_result_t result;

	isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
	isc_mem_create(&mctx);

	isc_logdestination_t destination;
	isc_logconfig_t *logconfig = NULL;

	isc_log_create(mctx, &lctx, &logconfig);
	isc_log_registercategories(lctx, categories);
	isc_log_setcontext(lctx);

	destination.file.stream = stderr;
	destination.file.name = NULL;
	destination.file.versions = ISC_LOG_ROLLNEVER;
	destination.file.maximum_size = 0;
	isc_log_createchannel(logconfig, "stderr", ISC_LOG_TOFILEDESC,
			      ISC_LOG_DYNAMIC, &destination, 0);
	CHECK(isc_log_usechannel(logconfig, "stderr", NULL, NULL));

	return (ISC_R_SUCCESS);

cleanup:
	cleanup();
	return (result);
}

struct duration_conf {
	const char *string;
	uint32_t time;
};
typedef struct duration_conf duration_conf_t;

/* test cfg_obj_asduration() */
static void
cfg_obj_asduration_test(void **state) {
	isc_result_t result;
	duration_conf_t durations[] = {
		{ .string = "PT0S", .time = 0 },
		{ .string = "PT42S", .time = 42 },
		{ .string = "PT10m", .time = 600 },
		{ .string = "PT10m4S", .time = 604 },
		{ .string = "pT2H", .time = 7200 },
		{ .string = "Pt2H3S", .time = 7203 },
		{ .string = "PT2h1m3s", .time = 7263 },
		{ .string = "p7d", .time = 604800 },
		{ .string = "P7DT2h", .time = 612000 },
		{ .string = "P2W", .time = 1209600 },
		{ .string = "P3M", .time = 8035200 },
		{ .string = "P3MT10M", .time = 8035800 },
		{ .string = "p5y", .time = 157680000 },
		{ .string = "P5YT2H", .time = 157687200 },
		{ .string = "P1Y1M1DT1H1M1S", .time = 34304461 },
		{ .string = "0", .time = 0 },
		{ .string = "30", .time = 30 },
		{ .string = "42s", .time = 42 },
		{ .string = "10m", .time = 600 },
		{ .string = "2H", .time = 7200 },
		{ .string = "7d", .time = 604800 },
		{ .string = "2w", .time = 1209600 },
	};
	int num = 22;
	isc_buffer_t buf1;
	cfg_parser_t *p1 = NULL;
	cfg_obj_t *c1 = NULL;

	UNUSED(state);

	setup();

	for (int i = 0; i < num; i++) {
		const cfg_listelt_t *element;
		const cfg_obj_t *kasps = NULL;
		char conf[64];
		sprintf(&conf[0],
			"dnssec-policy \"dp\"\n{\nsignatures-refresh %s;\n};\n",
			durations[i].string);

		isc_buffer_init(&buf1, conf, strlen(conf) - 1);
		isc_buffer_add(&buf1, strlen(conf) - 1);

		/* Parse with default line numbering */
		result = cfg_parser_create(mctx, lctx, &p1);
		assert_int_equal(result, ISC_R_SUCCESS);

		result = cfg_parse_buffer(p1, &buf1, "text1", 0,
					  &cfg_type_namedconf, 0, &c1);
		assert_int_equal(result, ISC_R_SUCCESS);

		(void)cfg_map_get(c1, "dnssec-policy", &kasps);
		assert_non_null(kasps);
		for (element = cfg_list_first(kasps); element != NULL;
		     element = cfg_list_next(element))
		{
			const cfg_obj_t *d1 = NULL;
			const cfg_obj_t *kopts = NULL;
			cfg_obj_t *kconf = cfg_listelt_value(element);
			assert_non_null(kconf);

			kopts = cfg_tuple_get(kconf, "options");
			result = cfg_map_get(kopts, "signatures-refresh", &d1);
			assert_int_equal(result, ISC_R_SUCCESS);

			assert_int_equal(durations[i].time,
					 cfg_obj_asduration(d1));
		}

		cfg_obj_destroy(p1, &c1);
		cfg_parser_destroy(&p1);
	}

	cleanup();
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(cfg_obj_asduration_test),
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

#endif /* if HAVE_CMOCKA */
