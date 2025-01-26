/*	$NetBSD: zt.c,v 1.11 2025/01/26 16:25:26 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>

#include <isc/atomic.h>
#include <isc/file.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/tid.h>
#include <isc/util.h>

#include <dns/log.h>
#include <dns/name.h>
#include <dns/qp.h>
#include <dns/rdataclass.h>
#include <dns/view.h>
#include <dns/zone.h>
#include <dns/zt.h>

#define ZTMAGIC	     ISC_MAGIC('Z', 'T', 'b', 'l')
#define VALID_ZT(zt) ISC_MAGIC_VALID(zt, ZTMAGIC)

struct dns_zt {
	unsigned int magic;
	isc_mem_t *mctx;
	dns_qpmulti_t *multi;

	atomic_bool flush;
	isc_refcount_t references;
	isc_refcount_t loads_pending;
};

struct zt_load_params {
	dns_zt_t *zt;
	dns_zt_callback_t *loaddone;
	void *loaddone_arg;
	bool newonly;
};

struct zt_freeze_params {
	dns_view_t *view;
	bool freeze;
};

static void
ztqpattach(void *uctx ISC_ATTR_UNUSED, void *pval,
	   uint32_t ival ISC_ATTR_UNUSED) {
	dns_zone_t *zone = pval;
	dns_zone_ref(zone);
}

static void
ztqpdetach(void *uctx ISC_ATTR_UNUSED, void *pval,
	   uint32_t ival ISC_ATTR_UNUSED) {
	dns_zone_t *zone = pval;
	dns_zone_detach(&zone);
}

static size_t
ztqpmakekey(dns_qpkey_t key, void *uctx ISC_ATTR_UNUSED, void *pval,
	    uint32_t ival ISC_ATTR_UNUSED) {
	dns_zone_t *zone = pval;
	dns_name_t *name = dns_zone_getorigin(zone);
	return dns_qpkey_fromname(key, name);
}

static void
ztqptriename(void *uctx, char *buf, size_t size) {
	dns_view_t *view = uctx;
	snprintf(buf, size, "view %s zone table", view->name);
}

static dns_qpmethods_t ztqpmethods = {
	ztqpattach,
	ztqpdetach,
	ztqpmakekey,
	ztqptriename,
};

void
dns_zt_create(isc_mem_t *mctx, dns_view_t *view, dns_zt_t **ztp) {
	dns_qpmulti_t *multi = NULL;
	dns_zt_t *zt = NULL;

	REQUIRE(ztp != NULL && *ztp == NULL);
	REQUIRE(view != NULL);

	dns_qpmulti_create(mctx, &ztqpmethods, view, &multi);

	zt = isc_mem_get(mctx, sizeof(*zt));
	*zt = (dns_zt_t){
		.magic = ZTMAGIC,
		.multi = multi,
		.references = 1,
	};

	isc_mem_attach(mctx, &zt->mctx);

	*ztp = zt;
}

/*
 * XXXFANF it isn't clear whether this function will be useful. There
 * is only one zone table per view, so it is probably enough to let
 * the qp-trie auto-GC do its thing. However it might be problematic
 * if a very large zone is replaced, and its database memory is
 * retained for a long time.
 */
void
dns_zt_compact(dns_zt_t *zt) {
	dns_qp_t *qp = NULL;

	REQUIRE(VALID_ZT(zt));

	dns_qpmulti_write(zt->multi, &qp);
	dns_qp_compact(qp, DNS_QPGC_ALL);
	dns_qpmulti_commit(zt->multi, &qp);
}

isc_result_t
dns_zt_mount(dns_zt_t *zt, dns_zone_t *zone) {
	isc_result_t result;
	dns_qp_t *qp = NULL;

	REQUIRE(VALID_ZT(zt));

	dns_qpmulti_write(zt->multi, &qp);
	result = dns_qp_insert(qp, zone, 0);
	dns_qp_compact(qp, DNS_QPGC_MAYBE);
	dns_qpmulti_commit(zt->multi, &qp);

	return result;
}

isc_result_t
dns_zt_unmount(dns_zt_t *zt, dns_zone_t *zone) {
	isc_result_t result;
	dns_qp_t *qp = NULL;

	REQUIRE(VALID_ZT(zt));

	dns_qpmulti_write(zt->multi, &qp);
	result = dns_qp_deletename(qp, dns_zone_getorigin(zone), NULL, NULL);
	dns_qp_compact(qp, DNS_QPGC_MAYBE);
	dns_qpmulti_commit(zt->multi, &qp);

	return result;
}

isc_result_t
dns_zt_find(dns_zt_t *zt, const dns_name_t *name, dns_ztfind_t options,
	    dns_zone_t **zonep) {
	isc_result_t result;
	dns_qpread_t qpr;
	void *pval = NULL;
	dns_ztfind_t exactmask = DNS_ZTFIND_NOEXACT | DNS_ZTFIND_EXACT;
	dns_ztfind_t exactopts = options & exactmask;
	dns_qpchain_t chain;

	REQUIRE(VALID_ZT(zt));
	REQUIRE(exactopts != exactmask);

	dns_qpmulti_query(zt->multi, &qpr);

	if (exactopts == DNS_ZTFIND_EXACT) {
		result = dns_qp_getname(&qpr, name, &pval, NULL);
	} else {
		result = dns_qp_lookup(&qpr, name, NULL, NULL, &chain, &pval,
				       NULL);
		if (exactopts == DNS_ZTFIND_NOEXACT && result == ISC_R_SUCCESS)
		{
			/* get pval from the previous chain link */
			int len = dns_qpchain_length(&chain);
			if (len >= 2) {
				dns_qpchain_node(&chain, len - 2, NULL, &pval,
						 NULL);
				result = DNS_R_PARTIALMATCH;
			} else {
				result = ISC_R_NOTFOUND;
			}
		}
	}
	dns_qpread_destroy(zt->multi, &qpr);

	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		dns_zone_t *zone = pval;
		/*
		 * If DNS_ZTFIND_MIRROR is set and the zone which was
		 * determined to be the deepest match for the supplied name is
		 * a mirror zone which is expired or not yet loaded, treat it
		 * as non-existent.  This will trigger a fallback to recursion
		 * instead of returning a SERVFAIL.
		 *
		 * Note that currently only the deepest match in the zone table
		 * is checked.  Consider a server configured with two mirror
		 * zones: "bar" and its child, "foo.bar".  If zone data is
		 * available for "bar" but not for "foo.bar", a query with
		 * QNAME equal to or below "foo.bar" will cause ISC_R_NOTFOUND
		 * to be returned, not DNS_R_PARTIALMATCH, despite zone data
		 * being available for "bar".  This is considered to be an edge
		 * case, handling which more appropriately is possible, but
		 * arguably not worth the added complexity.
		 */
		if ((options & DNS_ZTFIND_MIRROR) != 0 &&
		    dns_zone_gettype(zone) == dns_zone_mirror &&
		    !dns_zone_isloaded(zone))
		{
			result = ISC_R_NOTFOUND;
		} else {
			dns_zone_attach(zone, zonep);
		}
	}

	return result;
}

void
dns_zt_attach(dns_zt_t *zt, dns_zt_t **ztp) {
	REQUIRE(VALID_ZT(zt));
	REQUIRE(ztp != NULL && *ztp == NULL);

	isc_refcount_increment(&zt->references);

	*ztp = zt;
}

static isc_result_t
flush(dns_zone_t *zone, void *uap) {
	UNUSED(uap);
	return dns_zone_flush(zone);
}

static void
zt_destroy(dns_zt_t *zt) {
	isc_refcount_destroy(&zt->references);
	isc_refcount_destroy(&zt->loads_pending);

	if (atomic_load_acquire(&zt->flush)) {
		(void)dns_zt_apply(zt, false, NULL, flush, NULL);
	}

	dns_qpmulti_destroy(&zt->multi);
	zt->magic = 0;
	isc_mem_putanddetach(&zt->mctx, zt, sizeof(*zt));
}

void
dns_zt_detach(dns_zt_t **ztp) {
	dns_zt_t *zt;

	REQUIRE(ztp != NULL && VALID_ZT(*ztp));

	zt = *ztp;
	*ztp = NULL;

	if (isc_refcount_decrement(&zt->references) == 1) {
		zt_destroy(zt);
	}
}

void
dns_zt_flush(dns_zt_t *zt) {
	REQUIRE(VALID_ZT(zt));
	atomic_store_release(&zt->flush, true);
}

static isc_result_t
load(dns_zone_t *zone, void *uap) {
	isc_result_t result;
	result = dns_zone_load(zone, uap != NULL);
	if (result == DNS_R_CONTINUE || result == DNS_R_UPTODATE ||
	    result == DNS_R_DYNAMIC)
	{
		result = ISC_R_SUCCESS;
	}
	return result;
}

isc_result_t
dns_zt_load(dns_zt_t *zt, bool stop, bool newonly) {
	REQUIRE(VALID_ZT(zt));
	return dns_zt_apply(zt, stop, NULL, load, newonly ? &newonly : NULL);
}

static void
loaded_all(struct zt_load_params *params) {
	if (params->loaddone != NULL) {
		params->loaddone(params->loaddone_arg);
	}
	isc_mem_put(params->zt->mctx, params, sizeof(*params));
}

/*
 * Decrement the loads_pending counter; when counter reaches
 * zero, call the loaddone callback that was initially set by
 * dns_zt_asyncload().
 */
static isc_result_t
loaded_one(void *uap) {
	struct zt_load_params *params = uap;
	dns_zt_t *zt = params->zt;

	REQUIRE(VALID_ZT(zt));

	if (isc_refcount_decrement(&zt->loads_pending) == 1) {
		loaded_all(params);
	}

	if (isc_refcount_decrement(&zt->references) == 1) {
		zt_destroy(zt);
	}

	return ISC_R_SUCCESS;
}

/*
 * Initiates asynchronous loading of zone 'zone'.  'callback' is a
 * pointer to a function which will be used to inform the caller when
 * the zone loading is complete.
 */
static isc_result_t
asyncload(dns_zone_t *zone, void *uap) {
	struct zt_load_params *params = uap;
	struct dns_zt *zt = params->zt;
	isc_result_t result;

	REQUIRE(VALID_ZT(zt));
	REQUIRE(zone != NULL);

	isc_refcount_increment(&zt->references);
	isc_refcount_increment(&zt->loads_pending);

	result = dns_zone_asyncload(zone, params->newonly, loaded_one, params);
	if (result != ISC_R_SUCCESS) {
		/*
		 * Caller is holding a reference to zt->loads_pending
		 * and zt->references so these can't decrement to zero.
		 */
		isc_refcount_decrement1(&zt->references);
		isc_refcount_decrement1(&zt->loads_pending);
	}
	return ISC_R_SUCCESS;
}

isc_result_t
dns_zt_asyncload(dns_zt_t *zt, bool newonly, dns_zt_callback_t *loaddone,
		 void *arg) {
	isc_result_t result;
	uint_fast32_t loads_pending;
	struct zt_load_params *params = NULL;

	REQUIRE(VALID_ZT(zt));

	/*
	 * Obtain a reference to zt->loads_pending so that asyncload can
	 * safely decrement both zt->references and zt->loads_pending
	 * without going to zero.
	 */
	loads_pending = isc_refcount_increment0(&zt->loads_pending);
	INSIST(loads_pending == 0);

	params = isc_mem_get(zt->mctx, sizeof(*params));
	*params = (struct zt_load_params){
		.zt = zt,
		.newonly = newonly,
		.loaddone = loaddone,
		.loaddone_arg = arg,
	};

	result = dns_zt_apply(zt, false, NULL, asyncload, params);

	/*
	 * Have all the loads completed?
	 */
	if (isc_refcount_decrement(&zt->loads_pending) == 1) {
		loaded_all(params);
	}

	return result;
}

static isc_result_t
freezezones(dns_zone_t *zone, void *uap) {
	struct zt_freeze_params *params = uap;
	bool frozen;
	isc_result_t result = ISC_R_SUCCESS;
	char classstr[DNS_RDATACLASS_FORMATSIZE];
	char zonename[DNS_NAME_FORMATSIZE];
	dns_zone_t *raw = NULL;
	dns_view_t *view;
	const char *vname;
	const char *sep;
	int level;

	dns_zone_getraw(zone, &raw);
	if (raw != NULL) {
		zone = raw;
	}
	if (params->view != dns_zone_getview(zone)) {
		if (raw != NULL) {
			dns_zone_detach(&raw);
		}
		return ISC_R_SUCCESS;
	}
	if (dns_zone_gettype(zone) != dns_zone_primary) {
		if (raw != NULL) {
			dns_zone_detach(&raw);
		}
		return ISC_R_SUCCESS;
	}
	if (!dns_zone_isdynamic(zone, true)) {
		if (raw != NULL) {
			dns_zone_detach(&raw);
		}
		return ISC_R_SUCCESS;
	}

	frozen = dns_zone_getupdatedisabled(zone);
	if (params->freeze) {
		if (frozen) {
			result = DNS_R_FROZEN;
		}
		if (result == ISC_R_SUCCESS) {
			result = dns_zone_flush(zone);
		}
		if (result == ISC_R_SUCCESS) {
			dns_zone_setupdatedisabled(zone, params->freeze);
		}
	} else {
		if (frozen) {
			result = dns_zone_loadandthaw(zone);
			if (result == DNS_R_CONTINUE ||
			    result == DNS_R_UPTODATE)
			{
				result = ISC_R_SUCCESS;
			}
		}
	}
	view = dns_zone_getview(zone);
	if (strcmp(view->name, "_bind") == 0 ||
	    strcmp(view->name, "_default") == 0)
	{
		vname = "";
		sep = "";
	} else {
		vname = view->name;
		sep = " ";
	}
	dns_rdataclass_format(dns_zone_getclass(zone), classstr,
			      sizeof(classstr));
	dns_name_format(dns_zone_getorigin(zone), zonename, sizeof(zonename));
	level = (result != ISC_R_SUCCESS) ? ISC_LOG_ERROR : ISC_LOG_DEBUG(1);
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_ZONE,
		      level, "%s zone '%s/%s'%s%s: %s",
		      params->freeze ? "freezing" : "thawing", zonename,
		      classstr, sep, vname, isc_result_totext(result));
	if (raw != NULL) {
		dns_zone_detach(&raw);
	}
	return result;
}

isc_result_t
dns_zt_freezezones(dns_zt_t *zt, dns_view_t *view, bool freeze) {
	isc_result_t result, tresult;
	struct zt_freeze_params params = { view, freeze };

	REQUIRE(VALID_ZT(zt));

	result = dns_zt_apply(zt, false, &tresult, freezezones, &params);
	if (tresult == ISC_R_NOTFOUND) {
		tresult = ISC_R_SUCCESS;
	}
	return (result == ISC_R_SUCCESS) ? tresult : result;
}

typedef void
setview_cb(dns_zone_t *zone);

static isc_result_t
setview(dns_zone_t *zone, void *arg) {
	setview_cb *cb = arg;
	cb(zone);
	return ISC_R_SUCCESS;
}

void
dns_zt_setviewcommit(dns_zt_t *zt) {
	dns_zt_apply(zt, false, NULL, setview, dns_zone_setviewcommit);
}

void
dns_zt_setviewrevert(dns_zt_t *zt) {
	dns_zt_apply(zt, false, NULL, setview, dns_zone_setviewrevert);
}

isc_result_t
dns_zt_apply(dns_zt_t *zt, bool stop, isc_result_t *sub,
	     isc_result_t (*action)(dns_zone_t *, void *), void *uap) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t tresult = ISC_R_SUCCESS;
	dns_qpiter_t qpi;
	dns_qpread_t qpr;
	void *zone = NULL;

	REQUIRE(VALID_ZT(zt));
	REQUIRE(action != NULL);

	dns_qpmulti_query(zt->multi, &qpr);
	dns_qpiter_init(&qpr, &qpi);

	while (dns_qpiter_next(&qpi, NULL, &zone, NULL) == ISC_R_SUCCESS) {
		result = action(zone, uap);
		if (tresult == ISC_R_SUCCESS) {
			tresult = result;
		}
		if (result != ISC_R_SUCCESS && stop) {
			break;
		}
	}
	dns_qpread_destroy(zt->multi, &qpr);

	SET_IF_NOT_NULL(sub, tresult);

	return result;
}
