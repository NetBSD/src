/*	$NetBSD: named-journalprint.c,v 1.2 2018/08/12 13:02:30 christos Exp $	*/

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

#include <isc/log.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/util.h>

#include <dns/journal.h>
#include <dns/log.h>
#include <dns/result.h>
#include <dns/types.h>

#include <stdlib.h>

/*
 * Setup logging to use stderr.
 */
static isc_result_t
setup_logging(isc_mem_t *mctx, FILE *errout, isc_log_t **logp) {
	isc_logdestination_t destination;
	isc_logconfig_t *logconfig = NULL;
	isc_log_t *log = NULL;

	RUNTIME_CHECK(isc_log_create(mctx, &log, &logconfig) == ISC_R_SUCCESS);
	isc_log_setcontext(log);
	dns_log_init(log);
	dns_log_setcontext(log);

	destination.file.stream = errout;
	destination.file.name = NULL;
	destination.file.versions = ISC_LOG_ROLLNEVER;
	destination.file.maximum_size = 0;
	RUNTIME_CHECK(isc_log_createchannel(logconfig, "stderr",
					    ISC_LOG_TOFILEDESC,
					    ISC_LOG_DYNAMIC,
					    &destination, 0) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_log_usechannel(logconfig, "stderr",
					 NULL, NULL) == ISC_R_SUCCESS);

	*logp = log;
	return (ISC_R_SUCCESS);
}

int
main(int argc, char **argv) {
	char *file;
	isc_mem_t *mctx = NULL;
	isc_result_t result;
	isc_log_t *lctx = NULL;

	if (argc != 2) {
		printf("usage: %s journal\n", argv[0]);
		return(1);
	}

	file = argv[1];

	RUNTIME_CHECK(isc_mem_create(0, 0, &mctx) == ISC_R_SUCCESS);
	RUNTIME_CHECK(setup_logging(mctx, stderr, &lctx) == ISC_R_SUCCESS);

	result = dns_journal_print(mctx, file, stdout);
	if (result == DNS_R_NOJOURNAL)
		fprintf(stderr, "%s\n", dns_result_totext(result));
	isc_log_destroy(&lctx);
	isc_mem_detach(&mctx);
	return(result != ISC_R_SUCCESS ? 1 : 0);
}
