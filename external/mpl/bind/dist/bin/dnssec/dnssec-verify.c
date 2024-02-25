/*	$NetBSD: dnssec-verify.c,v 1.6.2.1 2024/02/25 15:43:04 martin Exp $	*/

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

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include <isc/app.h>
#include <isc/attributes.h>
#include <isc/base32.h>
#include <isc/commandline.h>
#include <isc/event.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/hex.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/os.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/rwlock.h>
#include <isc/serial.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/diff.h>
#include <dns/dnssec.h>
#include <dns/ds.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/master.h>
#include <dns/masterdump.h>
#include <dns/nsec.h>
#include <dns/nsec3.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/soa.h>
#include <dns/time.h>
#include <dns/zoneverify.h>

#include <dst/dst.h>

#include "dnssectool.h"

const char *program = "dnssec-verify";

static isc_stdtime_t now;
static isc_mem_t *mctx = NULL;
static dns_masterformat_t inputformat = dns_masterformat_text;
static dns_db_t *gdb;		  /* The database */
static dns_dbversion_t *gversion; /* The database version */
static dns_rdataclass_t gclass;	  /* The class */
static dns_name_t *gorigin;	  /* The database origin */
static bool ignore_kskflag = false;
static bool keyset_kskonly = false;

static void
report(const char *format, ...) {
	if (!quiet) {
		char buf[4096];
		va_list args;

		va_start(args, format);
		vsnprintf(buf, sizeof(buf), format, args);
		va_end(args);
		fprintf(stdout, "%s\n", buf);
	}
}

/*%
 * Load the zone file from disk
 */
static void
loadzone(char *file, char *origin, dns_rdataclass_t rdclass, dns_db_t **db) {
	isc_buffer_t b;
	int len;
	dns_fixedname_t fname;
	dns_name_t *name;
	isc_result_t result;

	len = strlen(origin);
	isc_buffer_init(&b, origin, len);
	isc_buffer_add(&b, len);

	name = dns_fixedname_initname(&fname);
	result = dns_name_fromtext(name, &b, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		fatal("failed converting name '%s' to dns format: %s", origin,
		      isc_result_totext(result));
	}

	result = dns_db_create(mctx, "rbt", name, dns_dbtype_zone, rdclass, 0,
			       NULL, db);
	check_result(result, "dns_db_create()");

	result = dns_db_load(*db, file, inputformat, 0);
	switch (result) {
	case DNS_R_SEENINCLUDE:
	case ISC_R_SUCCESS:
		break;
	case DNS_R_NOTZONETOP:
		/*
		 * Comparing pointers (vs. using strcmp()) is intentional: we
		 * want to check whether -o was supplied on the command line,
		 * not whether origin and file contain the same string.
		 */
		if (origin == file) {
			fatal("failed loading zone '%s' from file '%s': "
			      "use -o to specify a different zone origin",
			      origin, file);
		}
		FALLTHROUGH;
	default:
		fatal("failed loading zone from '%s': %s", file,
		      isc_result_totext(result));
	}
}

noreturn static void
usage(void);

static void
usage(void) {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t%s [options] zonefile [keys]\n", program);

	fprintf(stderr, "\n");

	fprintf(stderr, "Version: %s\n", PACKAGE_VERSION);

	fprintf(stderr, "Options: (default value in parenthesis) \n");
	fprintf(stderr, "\t-v debuglevel (0)\n");
	fprintf(stderr, "\t-q quiet\n");
	fprintf(stderr, "\t-V:\tprint version information\n");
	fprintf(stderr, "\t-o origin:\n");
	fprintf(stderr, "\t\tzone origin (name of zonefile)\n");
	fprintf(stderr, "\t-I format:\n");
	fprintf(stderr, "\t\tfile format of input zonefile (text)\n");
	fprintf(stderr, "\t-c class (IN)\n");
	fprintf(stderr, "\t-E engine:\n");
	fprintf(stderr, "\t\tname of an OpenSSL engine to use\n");
	fprintf(stderr, "\t-x:\tDNSKEY record signed with KSKs only, "
			"not ZSKs\n");
	fprintf(stderr, "\t-z:\tAll records signed with KSKs\n");
	exit(0);
}

int
main(int argc, char *argv[]) {
	char *origin = NULL, *file = NULL;
	char *inputformatstr = NULL;
	isc_result_t result;
	isc_log_t *log = NULL;
	const char *engine = NULL;
	char *classname = NULL;
	dns_rdataclass_t rdclass;
	char *endp;
	int ch;

#define CMDLINE_FLAGS "c:E:hm:o:I:qv:Vxz"

	/*
	 * Process memory debugging argument first.
	 */
	while ((ch = isc_commandline_parse(argc, argv, CMDLINE_FLAGS)) != -1) {
		switch (ch) {
		case 'm':
			if (strcasecmp(isc_commandline_argument, "record") == 0)
			{
				isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
			}
			if (strcasecmp(isc_commandline_argument, "trace") == 0)
			{
				isc_mem_debugging |= ISC_MEM_DEBUGTRACE;
			}
			if (strcasecmp(isc_commandline_argument, "usage") == 0)
			{
				isc_mem_debugging |= ISC_MEM_DEBUGUSAGE;
			}
			break;
		default:
			break;
		}
	}
	isc_commandline_reset = true;
	check_result(isc_app_start(), "isc_app_start");

	isc_mem_create(&mctx);

	isc_commandline_errprint = false;

	while ((ch = isc_commandline_parse(argc, argv, CMDLINE_FLAGS)) != -1) {
		switch (ch) {
		case 'c':
			classname = isc_commandline_argument;
			break;

		case 'E':
			engine = isc_commandline_argument;
			break;

		case 'I':
			inputformatstr = isc_commandline_argument;
			break;

		case 'm':
			break;

		case 'o':
			origin = isc_commandline_argument;
			break;

		case 'v':
			endp = NULL;
			verbose = strtol(isc_commandline_argument, &endp, 0);
			if (*endp != '\0') {
				fatal("verbose level must be numeric");
			}
			break;

		case 'q':
			quiet = true;
			break;

		case 'x':
			keyset_kskonly = true;
			break;

		case 'z':
			ignore_kskflag = true;
			break;

		case '?':
			if (isc_commandline_option != '?') {
				fprintf(stderr, "%s: invalid argument -%c\n",
					program, isc_commandline_option);
			}
			FALLTHROUGH;

		case 'h':
			/* Does not return. */
			usage();

		case 'V':
			/* Does not return. */
			version(program);

		default:
			fprintf(stderr, "%s: unhandled option -%c\n", program,
				isc_commandline_option);
			exit(1);
		}
	}

	result = dst_lib_init(mctx, engine);
	if (result != ISC_R_SUCCESS) {
		fatal("could not initialize dst: %s",
		      isc_result_totext(result));
	}

	isc_stdtime_get(&now);

	rdclass = strtoclass(classname);

	setup_logging(mctx, &log);

	argc -= isc_commandline_index;
	argv += isc_commandline_index;

	if (argc < 1) {
		usage();
	}

	file = argv[0];

	argc -= 1;
	argv += 1;

	POST(argc);
	POST(argv);

	if (origin == NULL) {
		origin = file;
	}

	if (inputformatstr != NULL) {
		if (strcasecmp(inputformatstr, "text") == 0) {
			inputformat = dns_masterformat_text;
		} else if (strcasecmp(inputformatstr, "raw") == 0) {
			inputformat = dns_masterformat_raw;
		} else {
			fatal("unknown file format: %s\n", inputformatstr);
		}
	}

	gdb = NULL;
	report("Loading zone '%s' from file '%s'\n", origin, file);
	loadzone(file, origin, rdclass, &gdb);
	gorigin = dns_db_origin(gdb);
	gclass = dns_db_class(gdb);

	gversion = NULL;
	result = dns_db_newversion(gdb, &gversion);
	check_result(result, "dns_db_newversion()");

	result = dns_zoneverify_dnssec(NULL, gdb, gversion, gorigin, NULL, mctx,
				       ignore_kskflag, keyset_kskonly, report);

	dns_db_closeversion(gdb, &gversion, false);
	dns_db_detach(&gdb);

	cleanup_logging(&log);
	dst_lib_destroy();
	if (verbose > 10) {
		isc_mem_stats(mctx, stdout);
	}
	isc_mem_destroy(&mctx);

	(void)isc_app_finish();

	return (result == ISC_R_SUCCESS ? 0 : 1);
}
