/*	$NetBSD: syncptr.c,v 1.1.1.1 2018/08/12 12:07:38 christos Exp $	*/

/*
 * Automatic A/AAAA/PTR record synchronization.
 *
 * Copyright (C) 2009-2015  Red Hat ; see COPYRIGHT for license
 */

#include <config.h>

#include <isc/event.h>
#include <isc/eventclass.h>
#include <isc/netaddr.h>
#include <isc/task.h>
#include <isc/util.h>

#include <dns/byaddr.h>
#include <dns/db.h>
#include <dns/name.h>
#include <dns/view.h>
#include <dns/zone.h>

#include "instance.h"
#include "syncptr.h"
#include "util.h"

/* Almost random value. See eventclass.h */
#define SYNCPTR_WRITE_EVENT (ISC_EVENTCLASS(1025) + 1)

/*
 * Event used for making changes to reverse zones.
 */
typedef struct syncptrevent syncptrevent_t;
struct syncptrevent {
	ISC_EVENT_COMMON(syncptrevent_t);
	isc_mem_t *mctx;
	dns_zone_t *zone;
	dns_diff_t diff;
	dns_fixedname_t ptr_target_name; /* referenced by owner name in tuple */
	isc_buffer_t b; /* referenced by target name in tuple */
	unsigned char buf[DNS_NAME_MAXWIRE];
};

/*
 * Write diff generated in syncptr() to reverse zone.
 *
 * This function will be called asynchronously and syncptr() will not get
 * any result from it.
 *
 */
static void
syncptr_write(isc_task_t *task, isc_event_t *event) {
	syncptrevent_t *pevent = (syncptrevent_t *)event;
	dns_dbversion_t *version = NULL;
	dns_db_t *db = NULL;
	isc_result_t result;

	REQUIRE(event->ev_type == SYNCPTR_WRITE_EVENT);

	UNUSED(task);

	CHECK(dns_zone_getdb(pevent->zone, &db));
	CHECK(dns_db_newversion(db, &version));
	CHECK(dns_diff_apply(&pevent->diff, db, version));

cleanup:
	if (db != NULL) {
		if (version != NULL)
			dns_db_closeversion(db, &version, ISC_TRUE);
		dns_db_detach(&db);
	}
	dns_zone_detach(&pevent->zone);
	dns_diff_clear(&pevent->diff);
	isc_event_free(&event);
}

/*
 * Find a reverse zone for given IP address.
 *
 * @param[in]  rdata      IP address as A/AAAA record
 * @param[out] name       Owner name for the PTR record
 * @param[out] zone       DNS zone for reverse record matching the IP address
 *
 * @retval ISC_R_SUCCESS  DNS name derived from given IP address belongs to an
 * 			  reverse zone managed by this driver instance.
 * 			  PTR record synchronization can continue.
 * @retval ISC_R_NOTFOUND Suitable reverse zone was not found because it
 * 			  does not exist or is not managed by this driver.
 */
static isc_result_t
syncptr_find_zone(sample_instance_t *inst, dns_rdata_t *rdata,
		  dns_name_t *name, dns_zone_t **zone)
{
	isc_result_t result;
	isc_netaddr_t isc_ip; /* internal net address representation */
	dns_rdata_in_a_t ipv4;
	dns_rdata_in_aaaa_t ipv6;

	REQUIRE(inst != NULL);
	REQUIRE(zone != NULL && *zone == NULL);

	switch (rdata->type) {
	case dns_rdatatype_a:
		CHECK(dns_rdata_tostruct(rdata, &ipv4, inst->mctx));
		isc_netaddr_fromin(&isc_ip, &ipv4.in_addr);
		break;

	case dns_rdatatype_aaaa:
		CHECK(dns_rdata_tostruct(rdata, &ipv6, inst->mctx));
		isc_netaddr_fromin6(&isc_ip, &ipv6.in6_addr);
		break;

	default:
		fatal_error("unsupported address type 0x%x", rdata->type);
		break;
	}

	/*
	 * Convert IP address to PTR owner name.
	 *
	 * @example
	 * 192.168.0.1 -> 1.0.168.192.in-addr.arpa
	 */
	CHECK(dns_byaddr_createptrname2(&isc_ip, 0, name));

	/* Find a zone containing owner name of the PTR record. */
	result = dns_zt_find(inst->view->zonetable, name, 0, NULL, zone);
	if (result == DNS_R_PARTIALMATCH)
		result = ISC_R_SUCCESS;
	else if (result != ISC_R_SUCCESS)
		goto cleanup;

	/* Make sure that the zone is managed by this driver. */
	if (*zone != inst->zone1 && *zone != inst->zone2) {
		dns_zone_detach(zone);
		result = ISC_R_NOTFOUND;
	}

cleanup:
	if (rdata->type == dns_rdatatype_a)
		dns_rdata_freestruct(&ipv4);
	else
		dns_rdata_freestruct(&ipv6);

	return (result);
}

/*
 * Generate update event for PTR record to reflect change in A/AAAA record.
 *
 * @pre Reverse zone is managed by this driver.
 *
 * @param[in]  a_name  DNS domain of modified A/AAAA record
 * @param[in]  af      Address family
 * @param[in]  ip_str  IP address as a string (IPv4 or IPv6)
 * @param[in]  mod_op  LDAP_MOD_DELETE if A/AAAA record is being deleted
 *                     or LDAP_MOD_ADD if A/AAAA record is being added.
 *
 * @retval ISC_R_SUCCESS Event for PTR record update was generated and send.
 *                       Change to reverse zone will be done asynchronously.
 * @retval other	 Synchronization failed - reverse doesn't exist,
 * 			 is not managed by this driver instance,
 * 			 memory allocation error, etc.
 */
static isc_result_t
syncptr(sample_instance_t *inst, dns_name_t *name,
	dns_rdata_t *addr_rdata, dns_ttl_t ttl, dns_diffop_t op)
{
	isc_result_t result;
	isc_mem_t *mctx = inst->mctx;
	dns_fixedname_t ptr_name;
	dns_zone_t *ptr_zone = NULL;
	dns_rdata_ptr_t ptr_struct;
	dns_rdata_t ptr_rdata = DNS_RDATA_INIT;
	dns_difftuple_t *tp = NULL;
	isc_task_t *task = NULL;
	syncptrevent_t *pevent = NULL;

	dns_fixedname_init(&ptr_name);
	DNS_RDATACOMMON_INIT(&ptr_struct, dns_rdatatype_ptr, dns_rdataclass_in);
	dns_name_init(&ptr_struct.ptr, NULL);

	pevent = (syncptrevent_t *)isc_event_allocate(inst->mctx, inst,
						      SYNCPTR_WRITE_EVENT,
						      syncptr_write, NULL,
						      sizeof(syncptrevent_t));
	if (pevent == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	isc_buffer_init(&pevent->b, pevent->buf, sizeof(pevent->buf));
	dns_fixedname_init(&pevent->ptr_target_name);

	/* Check if reverse zone is managed by this driver */
	result = syncptr_find_zone(inst, addr_rdata,
				   dns_fixedname_name(&ptr_name), &ptr_zone);
	if (result != ISC_R_SUCCESS) {
		log_error_r("PTR record synchonization skipped: reverse zone "
			    "is not managed by driver instance '%s'",
			    inst->db_name);
		goto cleanup;
	}

	/* Reverse zone is managed by this driver, prepare PTR record */
	pevent->zone = NULL;
	dns_zone_attach(ptr_zone, &pevent->zone);
	CHECK(dns_name_copy(name, dns_fixedname_name(&pevent->ptr_target_name),
			    NULL));
	dns_name_clone(dns_fixedname_name(&pevent->ptr_target_name),
		       &ptr_struct.ptr);
	dns_diff_init(inst->mctx, &pevent->diff);
	CHECK(dns_rdata_fromstruct(&ptr_rdata, dns_rdataclass_in,
				   dns_rdatatype_ptr, &ptr_struct, &pevent->b));

	/* Create diff */
	CHECK(dns_difftuple_create(mctx, op, dns_fixedname_name(&ptr_name),
				   ttl, &ptr_rdata, &tp));
	dns_diff_append(&pevent->diff, &tp);

	/*
	 * Send update event to the reverse zone.
	 * It will be processed asynchronously.
	 */
	dns_zone_gettask(ptr_zone, &task);
	isc_task_send(task, (isc_event_t **)&pevent);

cleanup:
	if (ptr_zone != NULL)
		dns_zone_detach(&ptr_zone);
	if (tp != NULL)
		dns_difftuple_free(&tp);
	if (task != NULL)
		isc_task_detach(&task);
	if (pevent != NULL)
		isc_event_free((isc_event_t **)&pevent);

	return (result);
}

/*
 * Generate update event for every rdata in rdataset.
 *
 * @param[in]  name      Owner name for A/AAAA records in rdataset.
 * @param[in]  rdataset  A/AAAA records.
 * @param[in]  op	 DNS_DIFFOP_ADD / DNS_DIFFOP_DEL for adding / deleting
 * 			 the rdata
 */
isc_result_t
syncptrs(sample_instance_t *inst, dns_name_t *name,
	 dns_rdataset_t *rdataset, dns_diffop_t op)
{
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;

	for (result = dns_rdataset_first(rdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(rdataset)) {
		dns_rdataset_current(rdataset, &rdata);
		result = syncptr(inst, name, &rdata, rdataset->ttl, op);
		if (result != ISC_R_SUCCESS && result != ISC_R_NOTFOUND)
			goto cleanup;
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;

cleanup:
	return (result);
}
