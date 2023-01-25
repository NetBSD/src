/*	$NetBSD: rdatalist.c,v 1.7 2023/01/25 21:43:30 christos Exp $	*/

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

#include <stddef.h>
#include <string.h>

#include <isc/util.h>

#include <dns/name.h>
#include <dns/nsec3.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>

#include "rdatalist_p.h"

static dns_rdatasetmethods_t methods = {
	isc__rdatalist_disassociate,
	isc__rdatalist_first,
	isc__rdatalist_next,
	isc__rdatalist_current,
	isc__rdatalist_clone,
	isc__rdatalist_count,
	isc__rdatalist_addnoqname,
	isc__rdatalist_getnoqname,
	isc__rdatalist_addclosest,
	isc__rdatalist_getclosest,
	NULL, /* settrust */
	NULL, /* expire */
	NULL, /* clearprefetch */
	isc__rdatalist_setownercase,
	isc__rdatalist_getownercase,
	NULL /* addglue */
};

void
dns_rdatalist_init(dns_rdatalist_t *rdatalist) {
	REQUIRE(rdatalist != NULL);

	/*
	 * Initialize rdatalist.
	 */

	rdatalist->rdclass = 0;
	rdatalist->type = 0;
	rdatalist->covers = 0;
	rdatalist->ttl = 0;
	ISC_LIST_INIT(rdatalist->rdata);
	ISC_LINK_INIT(rdatalist, link);
	memset(rdatalist->upper, 0xeb, sizeof(rdatalist->upper));
	/*
	 * Clear upper set bit.
	 */
	rdatalist->upper[0] &= ~0x01;
}

isc_result_t
dns_rdatalist_tordataset(dns_rdatalist_t *rdatalist, dns_rdataset_t *rdataset) {
	/*
	 * Make 'rdataset' refer to the rdata in 'rdatalist'.
	 */

	REQUIRE(rdatalist != NULL);
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(!dns_rdataset_isassociated(rdataset));

	/* Check if dns_rdatalist_init has was called. */
	REQUIRE(rdatalist->upper[0] == 0xea);

	rdataset->methods = &methods;
	rdataset->rdclass = rdatalist->rdclass;
	rdataset->type = rdatalist->type;
	rdataset->covers = rdatalist->covers;
	rdataset->ttl = rdatalist->ttl;
	rdataset->trust = 0;
	rdataset->private1 = rdatalist;
	rdataset->private2 = NULL;
	rdataset->private3 = NULL;
	rdataset->privateuint4 = 0;
	rdataset->private5 = NULL;

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_rdatalist_fromrdataset(dns_rdataset_t *rdataset,
			   dns_rdatalist_t **rdatalist) {
	REQUIRE(rdatalist != NULL && rdataset != NULL);
	*rdatalist = rdataset->private1;

	return (ISC_R_SUCCESS);
}

void
isc__rdatalist_disassociate(dns_rdataset_t *rdataset) {
	UNUSED(rdataset);
}

isc_result_t
isc__rdatalist_first(dns_rdataset_t *rdataset) {
	dns_rdatalist_t *rdatalist;

	rdatalist = rdataset->private1;
	rdataset->private2 = ISC_LIST_HEAD(rdatalist->rdata);

	if (rdataset->private2 == NULL) {
		return (ISC_R_NOMORE);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc__rdatalist_next(dns_rdataset_t *rdataset) {
	dns_rdata_t *rdata;

	REQUIRE(rdataset != NULL);

	rdata = rdataset->private2;
	if (rdata == NULL) {
		return (ISC_R_NOMORE);
	}

	rdataset->private2 = ISC_LIST_NEXT(rdata, link);

	if (rdataset->private2 == NULL) {
		return (ISC_R_NOMORE);
	}

	return (ISC_R_SUCCESS);
}

void
isc__rdatalist_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata) {
	dns_rdata_t *list_rdata;

	REQUIRE(rdataset != NULL);

	list_rdata = rdataset->private2;
	INSIST(list_rdata != NULL);

	dns_rdata_clone(list_rdata, rdata);
}

void
isc__rdatalist_clone(dns_rdataset_t *source, dns_rdataset_t *target) {
	REQUIRE(source != NULL);
	REQUIRE(target != NULL);

	*target = *source;

	/*
	 * Reset iterator state.
	 */
	target->private2 = NULL;
}

unsigned int
isc__rdatalist_count(dns_rdataset_t *rdataset) {
	dns_rdatalist_t *rdatalist;
	dns_rdata_t *rdata;
	unsigned int count;

	REQUIRE(rdataset != NULL);

	rdatalist = rdataset->private1;

	count = 0;
	for (rdata = ISC_LIST_HEAD(rdatalist->rdata); rdata != NULL;
	     rdata = ISC_LIST_NEXT(rdata, link))
	{
		count++;
	}

	return (count);
}

isc_result_t
isc__rdatalist_addnoqname(dns_rdataset_t *rdataset, const dns_name_t *name) {
	dns_rdataset_t *neg = NULL;
	dns_rdataset_t *negsig = NULL;
	dns_rdataset_t *rdset;
	dns_ttl_t ttl;

	REQUIRE(rdataset != NULL);

	for (rdset = ISC_LIST_HEAD(name->list); rdset != NULL;
	     rdset = ISC_LIST_NEXT(rdset, link))
	{
		if (rdset->rdclass != rdataset->rdclass) {
			continue;
		}
		if (rdset->type == dns_rdatatype_nsec ||
		    rdset->type == dns_rdatatype_nsec3)
		{
			neg = rdset;
		}
	}
	if (neg == NULL) {
		return (ISC_R_NOTFOUND);
	}

	for (rdset = ISC_LIST_HEAD(name->list); rdset != NULL;
	     rdset = ISC_LIST_NEXT(rdset, link))
	{
		if (rdset->type == dns_rdatatype_rrsig &&
		    rdset->covers == neg->type)
		{
			negsig = rdset;
		}
	}

	if (negsig == NULL) {
		return (ISC_R_NOTFOUND);
	}
	/*
	 * Minimise ttl.
	 */
	ttl = rdataset->ttl;
	if (neg->ttl < ttl) {
		ttl = neg->ttl;
	}
	if (negsig->ttl < ttl) {
		ttl = negsig->ttl;
	}
	rdataset->ttl = neg->ttl = negsig->ttl = ttl;
	rdataset->attributes |= DNS_RDATASETATTR_NOQNAME;
	rdataset->private6 = name;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc__rdatalist_getnoqname(dns_rdataset_t *rdataset, dns_name_t *name,
			  dns_rdataset_t *neg, dns_rdataset_t *negsig) {
	dns_rdataclass_t rdclass;
	dns_rdataset_t *tneg = NULL;
	dns_rdataset_t *tnegsig = NULL;
	const dns_name_t *noqname;

	REQUIRE(rdataset != NULL);
	REQUIRE((rdataset->attributes & DNS_RDATASETATTR_NOQNAME) != 0);

	rdclass = rdataset->rdclass;
	noqname = rdataset->private6;

	(void)dns_name_dynamic(noqname); /* Sanity Check. */

	for (rdataset = ISC_LIST_HEAD(noqname->list); rdataset != NULL;
	     rdataset = ISC_LIST_NEXT(rdataset, link))
	{
		if (rdataset->rdclass != rdclass) {
			continue;
		}
		if (rdataset->type == dns_rdatatype_nsec ||
		    rdataset->type == dns_rdatatype_nsec3)
		{
			tneg = rdataset;
		}
	}
	if (tneg == NULL) {
		return (ISC_R_NOTFOUND);
	}

	for (rdataset = ISC_LIST_HEAD(noqname->list); rdataset != NULL;
	     rdataset = ISC_LIST_NEXT(rdataset, link))
	{
		if (rdataset->type == dns_rdatatype_rrsig &&
		    rdataset->covers == tneg->type)
		{
			tnegsig = rdataset;
		}
	}
	if (tnegsig == NULL) {
		return (ISC_R_NOTFOUND);
	}

	dns_name_clone(noqname, name);
	dns_rdataset_clone(tneg, neg);
	dns_rdataset_clone(tnegsig, negsig);
	return (ISC_R_SUCCESS);
}

isc_result_t
isc__rdatalist_addclosest(dns_rdataset_t *rdataset, const dns_name_t *name) {
	dns_rdataset_t *neg = NULL;
	dns_rdataset_t *negsig = NULL;
	dns_rdataset_t *rdset;
	dns_ttl_t ttl;

	REQUIRE(rdataset != NULL);

	for (rdset = ISC_LIST_HEAD(name->list); rdset != NULL;
	     rdset = ISC_LIST_NEXT(rdset, link))
	{
		if (rdset->rdclass != rdataset->rdclass) {
			continue;
		}
		if (rdset->type == dns_rdatatype_nsec ||
		    rdset->type == dns_rdatatype_nsec3)
		{
			neg = rdset;
		}
	}
	if (neg == NULL) {
		return (ISC_R_NOTFOUND);
	}

	for (rdset = ISC_LIST_HEAD(name->list); rdset != NULL;
	     rdset = ISC_LIST_NEXT(rdset, link))
	{
		if (rdset->type == dns_rdatatype_rrsig &&
		    rdset->covers == neg->type)
		{
			negsig = rdset;
		}
	}

	if (negsig == NULL) {
		return (ISC_R_NOTFOUND);
	}
	/*
	 * Minimise ttl.
	 */
	ttl = rdataset->ttl;
	if (neg->ttl < ttl) {
		ttl = neg->ttl;
	}
	if (negsig->ttl < ttl) {
		ttl = negsig->ttl;
	}
	rdataset->ttl = neg->ttl = negsig->ttl = ttl;
	rdataset->attributes |= DNS_RDATASETATTR_CLOSEST;
	rdataset->private7 = name;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc__rdatalist_getclosest(dns_rdataset_t *rdataset, dns_name_t *name,
			  dns_rdataset_t *neg, dns_rdataset_t *negsig) {
	dns_rdataclass_t rdclass;
	dns_rdataset_t *tneg = NULL;
	dns_rdataset_t *tnegsig = NULL;
	const dns_name_t *closest;

	REQUIRE(rdataset != NULL);
	REQUIRE((rdataset->attributes & DNS_RDATASETATTR_CLOSEST) != 0);

	rdclass = rdataset->rdclass;
	closest = rdataset->private7;

	(void)dns_name_dynamic(closest); /* Sanity Check. */

	for (rdataset = ISC_LIST_HEAD(closest->list); rdataset != NULL;
	     rdataset = ISC_LIST_NEXT(rdataset, link))
	{
		if (rdataset->rdclass != rdclass) {
			continue;
		}
		if (rdataset->type == dns_rdatatype_nsec ||
		    rdataset->type == dns_rdatatype_nsec3)
		{
			tneg = rdataset;
		}
	}
	if (tneg == NULL) {
		return (ISC_R_NOTFOUND);
	}

	for (rdataset = ISC_LIST_HEAD(closest->list); rdataset != NULL;
	     rdataset = ISC_LIST_NEXT(rdataset, link))
	{
		if (rdataset->type == dns_rdatatype_rrsig &&
		    rdataset->covers == tneg->type)
		{
			tnegsig = rdataset;
		}
	}
	if (tnegsig == NULL) {
		return (ISC_R_NOTFOUND);
	}

	dns_name_clone(closest, name);
	dns_rdataset_clone(tneg, neg);
	dns_rdataset_clone(tnegsig, negsig);
	return (ISC_R_SUCCESS);
}

void
isc__rdatalist_setownercase(dns_rdataset_t *rdataset, const dns_name_t *name) {
	dns_rdatalist_t *rdatalist;
	unsigned int i;

	/*
	 * We do not need to worry about label lengths as they are all
	 * less than or equal to 63.
	 */
	rdatalist = rdataset->private1;
	memset(rdatalist->upper, 0, sizeof(rdatalist->upper));
	for (i = 1; i < name->length; i++) {
		if (name->ndata[i] >= 0x41 && name->ndata[i] <= 0x5a) {
			rdatalist->upper[i / 8] |= 1 << (i % 8);
			/*
			 * Record that upper has been set.
			 */
		}
	}
	/*
	 * Record that upper has been set.
	 */
	rdatalist->upper[0] |= 0x01;
}

void
isc__rdatalist_getownercase(const dns_rdataset_t *rdataset, dns_name_t *name) {
	dns_rdatalist_t *rdatalist;
	unsigned int i;

	rdatalist = rdataset->private1;
	if ((rdatalist->upper[0] & 0x01) == 0) {
		return;
	}
	for (i = 0; i < name->length; i++) {
		/*
		 * Set the case bit if it does not match the recorded bit.
		 */
		if (name->ndata[i] >= 0x61 && name->ndata[i] <= 0x7a &&
		    (rdatalist->upper[i / 8] & (1 << (i % 8))) != 0)
		{
			name->ndata[i] &= ~0x20; /* clear the lower case bit */
		} else if (name->ndata[i] >= 0x41 && name->ndata[i] <= 0x5a &&
			   (rdatalist->upper[i / 8] & (1 << (i % 8))) == 0)
		{
			name->ndata[i] |= 0x20; /* set the lower case bit */
		}
	}
}
