/*	$NetBSD: zone.c,v 1.1.1.1 2018/08/12 12:07:38 christos Exp $	*/

/*
 * Zone management.
 *
 * Copyright (C) 2009-2015  Red Hat ; see COPYRIGHT for license
 */

#include <config.h>

#include <isc/util.h>

#include <dns/dyndb.h>
#include <dns/view.h>
#include <dns/zone.h>

#include "util.h"
#include "instance.h"
#include "lock.h"
#include "log.h"
#include "zone.h"

extern const char *impname;

/*
 * Create a new zone with origin 'name'. The zone stay invisible to clients
 * until it is explicitly added to a view.
 */
isc_result_t
create_zone(sample_instance_t * const inst, dns_name_t * const name,
	    dns_zone_t ** const rawp)
{
	isc_result_t result;
	dns_zone_t *raw = NULL;
	const char *zone_argv[1];
	char zone_name[DNS_NAME_FORMATSIZE];
	dns_acl_t *acl_any = NULL;

	REQUIRE(inst != NULL);
	REQUIRE(name != NULL);
	REQUIRE(rawp != NULL && *rawp == NULL);

	zone_argv[0] = inst->db_name;

	CHECK(dns_zone_create(&raw, inst->mctx));
	CHECK(dns_zone_setorigin(raw, name));
	dns_zone_setclass(raw, dns_rdataclass_in);
	dns_zone_settype(raw, dns_zone_master);
	CHECK(dns_zone_setdbtype(raw, 1, zone_argv));
	CHECK(dns_zonemgr_managezone(inst->zmgr, raw));

	/* This is completely insecure - use some sensible values instead! */
	CHECK(dns_acl_any(inst->mctx, &acl_any));
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
		if (dns_zone_getmgr(raw) != NULL)
			dns_zonemgr_releasezone(inst->zmgr, raw);
		dns_zone_detach(&raw);
	}
	if (acl_any != NULL)
		dns_acl_detach(&acl_any);

	return (result);
}

/*
 * Add zone to the view defined in inst->view. This will make the zone visible
 * to clients.
 */
static isc_result_t
publish_zone(sample_instance_t *inst, dns_zone_t *zone) {
	isc_result_t result;
	isc_boolean_t freeze = ISC_FALSE;
	dns_zone_t *zone_in_view = NULL;
	dns_view_t *view_in_zone = NULL;
	isc_result_t lock_state = ISC_R_IGNORE;

	REQUIRE(inst != NULL);
	REQUIRE(zone != NULL);

	/* Return success if the zone is already in the view as expected. */
	result = dns_view_findzone(inst->view, dns_zone_getorigin(zone),
				   &zone_in_view);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTFOUND)
		goto cleanup;

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
		freeze = ISC_TRUE;
		dns_view_thaw(inst->view);
	}

	dns_zone_setview(zone, inst->view);
	CHECK(dns_view_addzone(inst->view, zone));

cleanup:
	if (zone_in_view != NULL)
		dns_zone_detach(&zone_in_view);
	if (freeze)
		dns_view_freeze(inst->view);
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
	isc_boolean_t zone_dynamic;
	isc_uint32_t serial;

	result = dns_zone_load(zone);
	if (result != ISC_R_SUCCESS && result != DNS_R_UPTODATE
	    && result != DNS_R_DYNAMIC && result != DNS_R_CONTINUE)
		goto cleanup;
	zone_dynamic = (result == DNS_R_DYNAMIC);

	CHECK(dns_zone_getserial2(zone, &serial));
	dns_zone_log(zone, ISC_LOG_INFO, "loaded serial %u", serial);

	if (zone_dynamic)
		dns_zone_notify(zone);

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
		dns_zone_log(raw, ISC_LOG_ERROR,
			     "cannot add zone to view: %s",
			     dns_result_totext(result));
		goto cleanup;
	}

	CHECK(load_zone(raw));

cleanup:
	return (result);
}
