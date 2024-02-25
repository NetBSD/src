/*	$NetBSD: zone.c,v 1.5.2.1 2024/02/25 15:44:08 martin Exp $	*/

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
 * Copyright (C) Red Hat
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
 * Zone management.
 */

#include "zone.h"
#include <inttypes.h>
#include <stdbool.h>

#include <isc/util.h>

#include <dns/dyndb.h>
#include <dns/view.h>
#include <dns/zone.h>

#include "instance.h"
#include "lock.h"
#include "log.h"
#include "util.h"

extern const char *impname;

/*
 * Create a new zone with origin 'name'. The zone stay invisible to clients
 * until it is explicitly added to a view.
 */
isc_result_t
create_zone(sample_instance_t *const inst, dns_name_t *const name,
	    dns_zone_t **const rawp) {
	isc_result_t result;
	dns_zone_t *raw = NULL;
	const char *zone_argv[1];
	char zone_name[DNS_NAME_FORMATSIZE];
	dns_acl_t *acl_any = NULL;

	REQUIRE(inst != NULL);
	REQUIRE(name != NULL);
	REQUIRE(rawp != NULL && *rawp == NULL);

	zone_argv[0] = inst->db_name;

	result = dns_zone_create(&raw, inst->mctx);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR, "create_zone: dns_zone_create -> %s\n",
			  isc_result_totext(result));
		goto cleanup;
	}
	result = dns_zone_setorigin(raw, name);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "create_zone: dns_zone_setorigin -> %s\n",
			  isc_result_totext(result));
		goto cleanup;
	}
	dns_zone_setclass(raw, dns_rdataclass_in);
	dns_zone_settype(raw, dns_zone_primary);
	dns_zone_setdbtype(raw, 1, zone_argv);

	result = dns_zonemgr_managezone(inst->zmgr, raw);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "create_zone: dns_zonemgr_managezone -> %s\n",
			  isc_result_totext(result));
		goto cleanup;
	}

	/* This is completely insecure - use some sensible values instead! */
	result = dns_acl_any(inst->mctx, &acl_any);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR, "create_zone: dns_acl_any -> %s\n",
			  isc_result_totext(result));
		goto cleanup;
	}
	dns_zone_setupdateacl(raw, acl_any);
	dns_zone_setqueryacl(raw, acl_any);
	dns_zone_setxfracl(raw, acl_any);
	dns_acl_detach(&acl_any);

	*rawp = raw;
	return (ISC_R_SUCCESS);

cleanup:
	dns_name_format(name, zone_name, DNS_NAME_FORMATSIZE);
	log_error_r("failed to create new zone '%s'", zone_name);

	if (raw != NULL) {
		if (dns_zone_getmgr(raw) != NULL) {
			dns_zonemgr_releasezone(inst->zmgr, raw);
		}
		dns_zone_detach(&raw);
	}
	if (acl_any != NULL) {
		dns_acl_detach(&acl_any);
	}

	return (result);
}

/*
 * Add zone to the view defined in inst->view. This will make the zone visible
 * to clients.
 */
static isc_result_t
publish_zone(sample_instance_t *inst, dns_zone_t *zone) {
	isc_result_t result;
	bool freeze = false;
	dns_zone_t *zone_in_view = NULL;
	dns_view_t *view_in_zone = NULL;
	isc_result_t lock_state = ISC_R_IGNORE;

	REQUIRE(inst != NULL);
	REQUIRE(zone != NULL);

	/* Return success if the zone is already in the view as expected. */
	result = dns_view_findzone(inst->view, dns_zone_getorigin(zone),
				   &zone_in_view);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTFOUND) {
		goto cleanup;
	}

	view_in_zone = dns_zone_getview(zone);
	if (view_in_zone != NULL) {
		/* Zone has a view set -> view should contain the same zone. */
		if (zone_in_view == zone) {
			/* Zone is already published in the right view. */
			CLEANUP_WITH(ISC_R_SUCCESS);
		} else if (view_in_zone != inst->view) {
			/*
			 * Un-published inactive zone will have
			 * inst->view in zone but will not be present
			 * in the view itself.
			 */
			dns_zone_log(zone, ISC_LOG_ERROR,
				     "zone->view doesn't "
				     "match data in the view");
			CLEANUP_WITH(ISC_R_UNEXPECTED);
		}
	}

	if (zone_in_view != NULL) {
		dns_zone_log(zone, ISC_LOG_ERROR,
			     "cannot publish zone: view already "
			     "contains another zone with this name");
		CLEANUP_WITH(ISC_R_UNEXPECTED);
	}

	run_exclusive_enter(inst, &lock_state);
	if (inst->view->frozen) {
		freeze = true;
		dns_view_thaw(inst->view);
	}

	dns_zone_setview(zone, inst->view);
	result = dns_view_addzone(inst->view, zone);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "publish_zone: dns_view_addzone -> %s\n",
			  isc_result_totext(result));
		goto cleanup;
	}

cleanup:
	if (zone_in_view != NULL) {
		dns_zone_detach(&zone_in_view);
	}
	if (freeze) {
		dns_view_freeze(inst->view);
	}
	run_exclusive_exit(inst, lock_state);

	return (result);
}

/*
 * @warning Never call this on raw part of in-line secure zone, call it only
 * on the secure zone!
 */
static isc_result_t
load_zone(dns_zone_t *zone) {
	isc_result_t result;
	bool zone_dynamic;
	uint32_t serial;

	result = dns_zone_load(zone, false);
	if (result != ISC_R_SUCCESS && result != DNS_R_UPTODATE &&
	    result != DNS_R_DYNAMIC && result != DNS_R_CONTINUE)
	{
		goto cleanup;
	}
	zone_dynamic = (result == DNS_R_DYNAMIC);

	result = dns_zone_getserial(zone, &serial);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "load_zone: dns_zone_getserial -> %s\n",
			  isc_result_totext(result));
		goto cleanup;
	}
	dns_zone_log(zone, ISC_LOG_INFO, "loaded serial %u", serial);

	if (zone_dynamic) {
		dns_zone_notify(zone);
	}

cleanup:
	return (result);
}

/*
 * Add zone to view and call dns_zone_load().
 */
isc_result_t
activate_zone(sample_instance_t *inst, dns_zone_t *raw) {
	isc_result_t result;

	/*
	 * Zone has to be published *before* zone load
	 * otherwise it will race with zone->view != NULL check
	 * in zone_maintenance() in zone.c.
	 */
	result = publish_zone(inst, raw);
	if (result != ISC_R_SUCCESS) {
		dns_zone_log(raw, ISC_LOG_ERROR, "cannot add zone to view: %s",
			     isc_result_totext(result));
		goto cleanup;
	}

	result = load_zone(raw);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR, "activate_zone: load_zone -> %s\n",
			  isc_result_totext(result));
		goto cleanup;
	}

cleanup:
	return (result);
}
