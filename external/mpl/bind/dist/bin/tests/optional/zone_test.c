/*	$NetBSD: zone_test.c,v 1.9 2023/01/25 21:43:24 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <stdlib.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <isc/app.h>
#include <isc/commandline.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/socket.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/fixedname.h>
#include <dns/rdataclass.h>
#include <dns/rdataset.h>
#include <dns/result.h>
#include <dns/zone.h>

static int debug = 0;
static int quiet = 0;
static int stats = 0;
static isc_mem_t *mctx = NULL;
dns_zone_t *zone = NULL;
isc_nm_t *netmgr = NULL;
isc_taskmgr_t *taskmgr = NULL;
isc_timermgr_t *timermgr = NULL;
isc_socketmgr_t *socketmgr = NULL;
dns_zonemgr_t *zonemgr = NULL;
dns_zonetype_t zonetype = dns_zone_primary;
isc_sockaddr_t addr;

#define ERRRET(result, function)                                        \
	do {                                                            \
		if (result != ISC_R_SUCCESS) {                          \
			fprintf(stderr, "%s() returned %s\n", function, \
				dns_result_totext(result));             \
			return;                                         \
		}                                                       \
	} while (0)

#define ERRCONT(result, function)                               \
	if (result != ISC_R_SUCCESS) {                          \
		fprintf(stderr, "%s() returned %s\n", function, \
			dns_result_totext(result));             \
		continue;                                       \
	} else                                                  \
		(void)NULL

static void
usage(void) {
	fprintf(stderr, "usage: zone_test [-dqsSM] [-c class] [-f file] "
			"zone\n");
	exit(1);
}

static void
setup(const char *zonename, const char *filename, const char *classname) {
	isc_result_t result;
	dns_rdataclass_t rdclass;
	isc_consttextregion_t region;
	isc_buffer_t buffer;
	dns_fixedname_t fixorigin;
	dns_name_t *origin;
	const char *rbt = "rbt";

	if (debug) {
		fprintf(stderr, "loading \"%s\" from \"%s\" class \"%s\"\n",
			zonename, filename, classname);
	}
	result = dns_zone_create(&zone, mctx);
	ERRRET(result, "dns_zone_new");

	dns_zone_settype(zone, zonetype);

	isc_buffer_constinit(&buffer, zonename, strlen(zonename));
	isc_buffer_add(&buffer, strlen(zonename));
	dns_fixedname_init(&fixorigin);
	result = dns_name_fromtext(dns_fixedname_name(&fixorigin), &buffer,
				   dns_rootname, 0, NULL);
	ERRRET(result, "dns_name_fromtext");
	origin = dns_fixedname_name(&fixorigin);

	result = dns_zone_setorigin(zone, origin);
	ERRRET(result, "dns_zone_setorigin");

	dns_zone_setdbtype(zone, 1, &rbt);

	result = dns_zone_setfile(zone, filename, dns_masterformat_text,
				  &dns_master_style_default);
	ERRRET(result, "dns_zone_setfile");

	region.base = classname;
	region.length = strlen(classname);
	result = dns_rdataclass_fromtext(&rdclass,
					 (isc_textregion_t *)(void *)&region);
	ERRRET(result, "dns_rdataclass_fromtext");

	dns_zone_setclass(zone, rdclass);

	if (zonetype == dns_zone_secondary) {
		dns_zone_setprimaries(zone, &addr, 1);
	}

	result = dns_zone_load(zone, false);
	ERRRET(result, "dns_zone_load");

	result = dns_zonemgr_managezone(zonemgr, zone);
	ERRRET(result, "dns_zonemgr_managezone");
}

static void
print_rdataset(dns_name_t *name, dns_rdataset_t *rdataset) {
	isc_buffer_t text;
	char t[1000];
	isc_result_t result;
	isc_region_t r;

	isc_buffer_init(&text, t, sizeof(t));
	result = dns_rdataset_totext(rdataset, name, false, false, &text);
	isc_buffer_usedregion(&text, &r);
	if (result == ISC_R_SUCCESS) {
		printf("%.*s", (int)r.length, (char *)r.base);
	} else {
		printf("%s\n", dns_result_totext(result));
	}
}

static void
query(void) {
	char buf[1024];
	dns_fixedname_t name;
	dns_fixedname_t found;
	dns_db_t *db;
	isc_buffer_t buffer;
	isc_result_t result;
	dns_rdataset_t rdataset;
	dns_rdataset_t sigset;
	fd_set rfdset = { { 0 } };

	db = NULL;
	result = dns_zone_getdb(zone, &db);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "%s() returned %s\n", "dns_zone_getdb",
			dns_result_totext(result));
		return;
	}

	dns_fixedname_init(&found);
	dns_rdataset_init(&rdataset);
	dns_rdataset_init(&sigset);

	do {
		char *s;
		fprintf(stdout, "zone_test ");
		fflush(stdout);
		FD_ZERO(&rfdset);
		FD_SET(0, &rfdset);
		select(1, &rfdset, NULL, NULL, NULL);
		if (fgets(buf, sizeof(buf), stdin) == NULL) {
			fprintf(stdout, "\n");
			break;
		}
		buf[sizeof(buf) - 1] = '\0';

		s = strchr(buf, '\n');
		if (s != NULL) {
			*s = '\0';
		}
		s = strchr(buf, '\r');
		if (s != NULL) {
			*s = '\0';
		}
		if (strcmp(buf, "dump") == 0) {
			dns_zone_dumptostream(zone, stdout,
					      dns_masterformat_text,
					      &dns_master_style_default, 0);
			continue;
		}
		if (strlen(buf) == 0U) {
			continue;
		}
		dns_fixedname_init(&name);
		isc_buffer_init(&buffer, buf, strlen(buf));
		isc_buffer_add(&buffer, strlen(buf));
		result = dns_name_fromtext(dns_fixedname_name(&name), &buffer,
					   dns_rootname, 0, NULL);
		ERRCONT(result, "dns_name_fromtext");

		result = dns_db_find(db, dns_fixedname_name(&name),
				     NULL /*version*/, dns_rdatatype_a,
				     0 /*options*/, 0 /*time*/, NULL /*nodep*/,
				     dns_fixedname_name(&found), &rdataset,
				     &sigset);
		fprintf(stderr, "%s() returned %s\n", "dns_db_find",
			dns_result_totext(result));
		switch (result) {
		case DNS_R_DELEGATION:
			print_rdataset(dns_fixedname_name(&found), &rdataset);
			break;
		case ISC_R_SUCCESS:
			print_rdataset(dns_fixedname_name(&name), &rdataset);
			break;
		default:
			break;
		}

		if (dns_rdataset_isassociated(&rdataset)) {
			dns_rdataset_disassociate(&rdataset);
		}
		if (dns_rdataset_isassociated(&sigset)) {
			dns_rdataset_disassociate(&sigset);
		}
	} while (1);
	dns_rdataset_invalidate(&rdataset);
	dns_db_detach(&db);
}

int
main(int argc, char **argv) {
	int c;
	char *filename = NULL;
	const char *classname = "IN";

	while ((c = isc_commandline_parse(argc, argv, "cdf:m:qsMS")) != EOF) {
		switch (c) {
		case 'c':
			classname = isc_commandline_argument;
			break;
		case 'd':
			debug++;
			break;
		case 'f':
			if (filename != NULL) {
				usage();
			}
			filename = isc_commandline_argument;
			break;
		case 'm':
			memset(&addr, 0, sizeof(addr));
			addr.type.sin.sin_family = AF_INET;
			if (inet_pton(AF_INET, isc_commandline_argument,
				      &addr.type.sin.sin_addr) != 1)
			{
				fprintf(stderr, "bad master address '%s'\n",
					isc_commandline_argument);
				exit(1);
			}
			addr.type.sin.sin_port = htons(53);
			break;
		case 'q':
			quiet++;
			break;
		case 's':
			stats++;
			break;
		case 'S':
			zonetype = dns_zone_secondary;
			break;
		case 'M':
			zonetype = dns_zone_primary;
			break;
		default:
			usage();
		}
	}

	if (argv[isc_commandline_index] == NULL) {
		usage();
	}

	RUNTIME_CHECK(isc_app_start() == ISC_R_SUCCESS);
	isc_mem_create(&mctx);
	RUNTIME_CHECK(isc_managers_create(mctx, 2, 0, NULL, &taskmgr) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_timermgr_create(mctx, &timermgr) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_socketmgr_create(mctx, &socketmgr) == ISC_R_SUCCESS);
	RUNTIME_CHECK(dns_zonemgr_create(mctx, taskmgr, timermgr, socketmgr,
					 &zonemgr) == ISC_R_SUCCESS);
	if (filename == NULL) {
		filename = argv[isc_commandline_index];
	}
	setup(argv[isc_commandline_index], filename, classname);
	query();
	if (zone != NULL) {
		dns_zone_detach(&zone);
	}
	dns_zonemgr_shutdown(zonemgr);
	dns_zonemgr_detach(&zonemgr);
	isc_socketmgr_destroy(&socketmgr);
	isc_managers_destroy(&netmgr, &taskmgr);
	isc_timermgr_destroy(&timermgr);
	if (!quiet && stats) {
		isc_mem_stats(mctx, stdout);
	}
	isc_mem_destroy(&mctx);

	return (0);
}
