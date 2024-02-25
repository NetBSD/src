/*	$NetBSD: named-journalprint.c,v 1.7.2.1 2024/02/25 15:45:47 martin Exp $	*/

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

#include <stdlib.h>

#include <isc/commandline.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/util.h>

#include <dns/journal.h>
#include <dns/log.h>
#include <dns/types.h>

const char *progname = NULL;

static void
usage(void) {
	fprintf(stderr, "Usage: %s [-dux] journal\n", progname);
	exit(1);
}

/*
 * Setup logging to use stderr.
 */
static isc_result_t
setup_logging(isc_mem_t *mctx, FILE *errout, isc_log_t **logp) {
	isc_logdestination_t destination;
	isc_logconfig_t *logconfig = NULL;
	isc_log_t *log = NULL;

	isc_log_create(mctx, &log, &logconfig);
	isc_log_setcontext(log);
	dns_log_init(log);
	dns_log_setcontext(log);

	destination.file.stream = errout;
	destination.file.name = NULL;
	destination.file.versions = ISC_LOG_ROLLNEVER;
	destination.file.maximum_size = 0;
	isc_log_createchannel(logconfig, "stderr", ISC_LOG_TOFILEDESC,
			      ISC_LOG_DYNAMIC, &destination, 0);

	RUNTIME_CHECK(isc_log_usechannel(logconfig, "stderr", NULL, NULL) ==
		      ISC_R_SUCCESS);

	*logp = log;
	return (ISC_R_SUCCESS);
}

int
main(int argc, char **argv) {
	char *file;
	isc_mem_t *mctx = NULL;
	isc_result_t result;
	isc_log_t *lctx = NULL;
	uint32_t flags = 0U;
	int ch;
	bool compact = false;
	bool downgrade = false;
	bool upgrade = false;
	unsigned int serial = 0;
	char *endp = NULL;

	progname = argv[0];
	while ((ch = isc_commandline_parse(argc, argv, "c:dux")) != -1) {
		switch (ch) {
		case 'c':
			compact = true;
			serial = strtoul(isc_commandline_argument, &endp, 0);
			if (endp == isc_commandline_argument || *endp != 0) {
				fprintf(stderr, "invalid serial: %s\n",
					isc_commandline_argument);
				exit(1);
			}
			break;
		case 'd':
			downgrade = true;
			break;
		case 'u':
			upgrade = true;
			break;
		case 'x':
			flags |= DNS_JOURNAL_PRINTXHDR;
			break;
		default:
			usage();
		}
	}

	argc -= isc_commandline_index;
	argv += isc_commandline_index;

	if (argc != 1) {
		usage();
	}
	file = argv[0];

	isc_mem_create(&mctx);
	RUNTIME_CHECK(setup_logging(mctx, stderr, &lctx) == ISC_R_SUCCESS);

	if (upgrade) {
		flags = DNS_JOURNAL_COMPACTALL;
		result = dns_journal_compact(mctx, file, 0, flags, 0);
	} else if (downgrade) {
		flags = DNS_JOURNAL_COMPACTALL | DNS_JOURNAL_VERSION1;
		result = dns_journal_compact(mctx, file, 0, flags, 0);
	} else if (compact) {
		flags = 0;
		result = dns_journal_compact(mctx, file, serial, flags, 0);
	} else {
		result = dns_journal_print(mctx, flags, file, stdout);
		if (result == DNS_R_NOJOURNAL) {
			fprintf(stderr, "%s\n", isc_result_totext(result));
		}
	}
	isc_log_destroy(&lctx);
	isc_mem_detach(&mctx);
	return (result != ISC_R_SUCCESS ? 1 : 0);
}
