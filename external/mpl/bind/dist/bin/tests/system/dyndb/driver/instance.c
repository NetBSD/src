/*	$NetBSD: instance.c,v 1.5 2022/09/23 12:15:25 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0 AND ISC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Copyright (C) 2009-2015 Red Hat
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND AUTHORS DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Driver instance object.
 *
 * One instance is equivalent to dynamic-db section in named.conf.
 * This module parses arguments and provide high-level operations
 * instance init/zone load/instance destroy.
 */

#include "instance.h"

#include <isc/task.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/dyndb.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/view.h>
#include <dns/zone.h>

#include "db.h"
#include "log.h"
#include "util.h"
#include "zone.h"

/*
 * Parse parameters and convert them to zone names. Caller has to deallocate
 * resulting DNS names.
 *
 * @param[in]  argv NULL-terminated string array of length 2 (excluding NULL)
 * 		    Each string has to be a valid DNS name.
 * @param[out] z1   Zone name from argv[0]
 * @param[out] z2   Zone name from argv[1]
 */
static isc_result_t
parse_params(isc_mem_t *mctx, int argc, char **argv, dns_name_t *z1,
	     dns_name_t *z2) {
	isc_result_t result;
	int i;

	REQUIRE(argv != NULL);
	REQUIRE(z1 != NULL);
	REQUIRE(z2 != NULL);

	for (i = 0; i < argc; i++) {
		log_info("param: '%s'", argv[i]);
	}
	log_info("number of params: %d", i);

	if (argc != 2) {
		log_error("exactly two parameters "
			  "(absolute zone names) are required");
		result = ISC_R_FAILURE;
		goto cleanup;
	}
	result = dns_name_fromstring2(z1, argv[0], dns_rootname, 0, mctx);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "parse_params: dns_name_fromstring2 -> %s",
			  isc_result_totext(result));
		goto cleanup;
	}
	result = dns_name_fromstring2(z2, argv[1], dns_rootname, 0, mctx);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "parse_params: dns_name_fromstring2 -> %s",
			  isc_result_totext(result));
		goto cleanup;
	}

	result = ISC_R_SUCCESS;

cleanup:
	return (result);
}

/*
 * Initialize new driver instance. It will not create zones until
 * load_sample_instance_zones() is called.
 */
isc_result_t
new_sample_instance(isc_mem_t *mctx, const char *db_name, int argc, char **argv,
		    const dns_dyndbctx_t *dctx,
		    sample_instance_t **sample_instp) {
	isc_result_t result;
	sample_instance_t *inst = NULL;

	REQUIRE(sample_instp != NULL && *sample_instp == NULL);

	CHECKED_MEM_GET_PTR(mctx, inst);
	ZERO_PTR(inst);
	isc_mem_attach(mctx, &inst->mctx);

	inst->db_name = isc_mem_strdup(mctx, db_name);

	inst->zone1_name = dns_fixedname_initname(&inst->zone1_fn);
	inst->zone2_name = dns_fixedname_initname(&inst->zone2_fn);

	result = parse_params(mctx, argc, argv, inst->zone1_name,
			      inst->zone2_name);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "new_sample_instance: parse_params -> %s",
			  isc_result_totext(result));
		goto cleanup;
	}

	dns_view_attach(dctx->view, &inst->view);
	dns_zonemgr_attach(dctx->zmgr, &inst->zmgr);
	isc_task_attach(dctx->task, &inst->task);

	/* Register new DNS DB implementation. */
	result = dns_db_register(db_name, create_db, inst, mctx, &inst->db_imp);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "new_sample_instance: dns_db_register -> %s",
			  isc_result_totext(result));
		goto cleanup;
	}

	*sample_instp = inst;
	result = ISC_R_SUCCESS;

cleanup:
	if (result != ISC_R_SUCCESS) {
		destroy_sample_instance(&inst);
	}
	return (result);
}

/*
 * Create empty zones, add fake SOA, NS, and A records, load fake zones
 * and add them to inst->view.
 */
isc_result_t
load_sample_instance_zones(sample_instance_t *inst) {
	isc_result_t result;

	result = create_zone(inst, inst->zone1_name, &inst->zone1);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "load_sample_instance_zones: create_zone -> %s",
			  isc_result_totext(result));
		goto cleanup;
	}
	result = activate_zone(inst, inst->zone1);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "load_sample_instance_zones: activate_zone -> %s",
			  isc_result_totext(result));
		goto cleanup;
	}

	result = create_zone(inst, inst->zone2_name, &inst->zone2);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "load_sample_instance_zones: create_zone -> %s",
			  isc_result_totext(result));
		goto cleanup;
	}
	result = activate_zone(inst, inst->zone2);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "load_sample_instance_zones: activate_zone -> %s",
			  isc_result_totext(result));
		goto cleanup;
	}

cleanup:
	return (result);
}

void
destroy_sample_instance(sample_instance_t **instp) {
	sample_instance_t *inst;
	REQUIRE(instp != NULL);

	inst = *instp;
	*instp = NULL;
	if (inst == NULL) {
		return;
	}

	if (inst->db_name != NULL) {
		isc_mem_free(inst->mctx, inst->db_name);
	}
	if (inst->zone1 != NULL) {
		dns_zone_detach(&inst->zone1);
	}
	if (inst->zone2 != NULL) {
		dns_zone_detach(&inst->zone2);
	}
	if (inst->db_imp != NULL) {
		dns_db_unregister(&inst->db_imp);
	}

	dns_view_detach(&inst->view);
	dns_zonemgr_detach(&inst->zmgr);
	isc_task_detach(&inst->task);

	MEM_PUT_AND_DETACH(inst);
}
