/*	$NetBSD: cfg_test.c,v 1.2 2018/08/12 13:02:28 christos Exp $	*/

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

#include <errno.h>
#include <stdlib.h>

#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <isccfg/grammar.h>
#include <isccfg/namedconf.h>

#include <dns/log.h>

static void
check_result(isc_result_t result, const char *format, ...) {
	va_list args;

	if (result == ISC_R_SUCCESS)
		return;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, ": %s\n", isc_result_totext(result));
	exit(1);
}

static void
output(void *closure, const char *text, int textlen) {
	UNUSED(closure);
	(void) fwrite(text, 1, textlen, stdout);
}

static void
usage(void) {
	fprintf(stderr, "usage: cfg_test --rndc|--named "
		"[--grammar] [--memstats] conffile\n");
	exit(1);
}

int
main(int argc, char **argv) {
	isc_result_t result;
	isc_mem_t *mctx = NULL;
	isc_log_t *lctx = NULL;
	isc_logconfig_t *lcfg = NULL;
	isc_logdestination_t destination;
	cfg_parser_t *pctx = NULL;
	cfg_obj_t *cfg = NULL;
	cfg_type_t *type = NULL;
	isc_boolean_t grammar = ISC_FALSE;
	isc_boolean_t memstats = ISC_FALSE;
	char *filename = NULL;
	unsigned int zonetype = 0;

	RUNTIME_CHECK(isc_mem_create(0, 0, &mctx) == ISC_R_SUCCESS);

	result = isc_log_create(mctx, &lctx, &lcfg);
	check_result(result, "isc_log_create()");
	isc_log_setcontext(lctx);

	/*
	 * Create and install the default channel.
	 */
	destination.file.stream = stderr;
	destination.file.name = NULL;
	destination.file.versions = ISC_LOG_ROLLNEVER;
	destination.file.maximum_size = 0;
	result = isc_log_createchannel(lcfg, "_default",
				       ISC_LOG_TOFILEDESC,
				       ISC_LOG_DYNAMIC,
				       &destination, ISC_LOG_PRINTTIME);
	check_result(result, "isc_log_createchannel()");
	result = isc_log_usechannel(lcfg, "_default", NULL, NULL);
	check_result(result, "isc_log_usechannel()");

	/*
	 * Set the initial debug level.
	 */
	isc_log_setdebuglevel(lctx, 2);

	if (argc < 3)
		usage();

	while (argc > 1) {
		if (strcmp(argv[1], "--grammar") == 0) {
			grammar = ISC_TRUE;
		} else if (strcmp(argv[1], "--zonegrammar") == 0) {
			argv++, argc--;
			if (argc <= 1)  {
				usage();
			}
			if (strcmp(argv[1], "master") == 0 ||
			    strcmp(argv[1], "primary") == 0)
			{
				zonetype = CFG_ZONE_MASTER;
			} else if (strcmp(argv[1], "slave") == 0 ||
				   strcmp(argv[1], "seconary") == 0)
			{
				zonetype = CFG_ZONE_SLAVE;
			} else if (strcmp(argv[1], "stub") == 0) {
				zonetype = CFG_ZONE_STUB;
			} else if (strcmp(argv[1], "static-stub") == 0) {
				zonetype = CFG_ZONE_STATICSTUB;
			} else if (strcmp(argv[1], "hint") == 0) {
				zonetype = CFG_ZONE_HINT;
			} else if (strcmp(argv[1], "forward") == 0) {
				zonetype = CFG_ZONE_FORWARD;
			} else if (strcmp(argv[1], "redirect") == 0) {
				zonetype = CFG_ZONE_REDIRECT;
			} else if (strcmp(argv[1], "delegation-only") == 0) {
				zonetype = CFG_ZONE_DELEGATION;
			} else if (strcmp(argv[1], "in-view") == 0) {
				zonetype = CFG_ZONE_INVIEW;
			} else {
				usage();
			}
		} else if (strcmp(argv[1], "--memstats") == 0) {
			memstats = ISC_TRUE;
		} else if (strcmp(argv[1], "--named") == 0) {
			type = &cfg_type_namedconf;
		} else if (strcmp(argv[1], "--rndc") == 0) {
			type = &cfg_type_rndcconf;
		} else if (argv[1][0] == '-') {
			usage();
		} else {
			filename = argv[1];
		}
		argv++, argc--;
	}

	if (grammar) {
		if (type == NULL)
			usage();
		cfg_print_grammar(type, output, NULL);
	} else if (zonetype != 0) {
		cfg_print_zonegrammar(zonetype, output, NULL);
	} else {
		if (type == NULL || filename == NULL)
			usage();
		RUNTIME_CHECK(cfg_parser_create(mctx, lctx, &pctx) == ISC_R_SUCCESS);

		result = cfg_parse_file(pctx, filename, type, &cfg);

		fprintf(stderr, "read config: %s\n", isc_result_totext(result));

		if (result != ISC_R_SUCCESS)
			exit(1);

		cfg_print(cfg, output, NULL);

		cfg_obj_destroy(pctx, &cfg);

		cfg_parser_destroy(&pctx);
	}

	isc_log_destroy(&lctx);
	if (memstats)
		isc_mem_stats(mctx, stderr);
	isc_mem_destroy(&mctx);

	fflush(stdout);
	if (ferror(stdout)) {
		fprintf(stderr, "write error\n");
		return (1);
	} else
		return (0);
}
