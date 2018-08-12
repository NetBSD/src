/*	$NetBSD: makejournal.c,v 1.2 2018/08/12 13:02:28 christos Exp $	*/

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

#include <isc/entropy.h>
#include <isc/hash.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/fixedname.h>
#include <dns/journal.h>
#include <dns/log.h>
#include <dns/name.h>
#include <isc/print.h>
#include <dns/result.h>
#include <dns/types.h>

#include <stdlib.h>

#define CHECK(r) \
	do { \
		result = (r); \
		if (result != ISC_R_SUCCESS) \
		goto cleanup; \
	} while (/*CONSTCOND*/0)

isc_mem_t *mctx = NULL;
isc_log_t *lctx = NULL;
isc_entropy_t *ectx = NULL;

static isc_boolean_t hash_active = ISC_FALSE, dst_active = ISC_FALSE;

/*
 * Logging categories: this needs to match the list in bin/named/log.c.
 */
static isc_logcategory_t categories[] = {
		{ "",                0 },
		{ "client",          0 },
		{ "network",         0 },
		{ "update",          0 },
		{ "queries",         0 },
		{ "unmatched",       0 },
		{ "update-security", 0 },
		{ "query-errors",    0 },
		{ NULL,              0 }
};

static isc_result_t
loadzone(dns_db_t **db, const char *origin, const char *filename) {
	isc_result_t result;
	dns_fixedname_t fixed;
	dns_name_t *name;

	name = dns_fixedname_initname(&fixed);

	result = dns_name_fromstring(name, origin, 0, NULL);
	if (result != ISC_R_SUCCESS)
		return(result);

	result = dns_db_create(mctx, "rbt", name, dns_dbtype_zone,
			       dns_rdataclass_in, 0, NULL, db);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_db_load(*db, filename);
	return (result);
}

int
main(int argc, char **argv) {
	isc_result_t result;
	char *origin, *file1, *file2, *journal;
	dns_db_t *olddb = NULL, *newdb = NULL;
	isc_logdestination_t destination;
	isc_logconfig_t *logconfig = NULL;

	if (argc != 5) {
		printf("usage: %s origin file1 file2 journal\n", argv[0]);
		return (1);
	}

	origin = argv[1];
	file1 = argv[2];
	file2 = argv[3];
	journal = argv[4];

	isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
	CHECK(isc_mem_create(0, 0, &mctx));
	CHECK(isc_entropy_create(mctx, &ectx));

	CHECK(dst_lib_init(mctx, ectx, ISC_ENTROPY_BLOCKING));
	dst_active = ISC_TRUE;

	CHECK(isc_hash_create(mctx, ectx, DNS_NAME_MAXWIRE));
	hash_active = ISC_TRUE;

	CHECK(isc_log_create(mctx, &lctx, &logconfig));
	isc_log_registercategories(lctx, categories);
	isc_log_setcontext(lctx);
	dns_log_init(lctx);
	dns_log_setcontext(lctx);

	destination.file.stream = stderr;
	destination.file.name = NULL;
	destination.file.versions = ISC_LOG_ROLLNEVER;
	destination.file.maximum_size = 0;
	CHECK(isc_log_createchannel(logconfig, "stderr",
				    ISC_LOG_TOFILEDESC, ISC_LOG_DYNAMIC,
				    &destination, 0));
	CHECK(isc_log_usechannel(logconfig, "stderr", NULL, NULL));

	dns_result_register();

	result = loadzone(&olddb, origin, file1);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "Couldn't load %s: ", file1);
		goto cleanup;
	}

	result = loadzone(&newdb, origin, file2);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "Couldn't load %s: ", file2);
		goto cleanup;
	}

	result = dns_db_diff(mctx, newdb, NULL, olddb, NULL, journal);

  cleanup:
	if (result != ISC_R_SUCCESS)
		fprintf(stderr, "%s\n", isc_result_totext(result));

	if (newdb != NULL)
		dns_db_detach(&newdb);
	if (olddb != NULL)
		dns_db_detach(&olddb);

	if (lctx != NULL)
		isc_log_destroy(&lctx);
	if (dst_active) {
		dst_lib_destroy();
		dst_active = ISC_FALSE;
	}
	if (hash_active) {
		isc_hash_destroy();
		hash_active = ISC_FALSE;
	}
	if (ectx != NULL)
		isc_entropy_detach(&ectx);
	if (mctx != NULL)
		isc_mem_destroy(&mctx);

	return(result != ISC_R_SUCCESS ? 1 : 0);
}

