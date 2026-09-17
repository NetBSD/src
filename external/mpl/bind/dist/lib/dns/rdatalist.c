/*	$NetBSD: rdatalist.c,v 1.10 2026/09/17 18:01:15 christos Exp $	*/

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
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>

static dns_rdatasetmethods_t methods = {
	.disassociate = dns_rdatalist_disassociate,
	.first = dns_rdatalist_first,
	.next = dns_rdatalist_next,
	.current = dns_rdatalist_current,
	.clone = dns_rdatalist_clone,
	.count = dns_rdatalist_count,
	.addnoqname = dns_rdatalist_addnoqname,
	.getnoqname = dns_rdatalist_getnoqname,
	.setownercase = dns_rdatalist_setownercase,
	.getownercase = dns_rdatalist_getownercase,
};

void
dns_rdatalist_init(dns_rdatalist_t *rdatalist) {
	REQUIRE(rdatalist != NULL);

	/*
	 * Initialize rdatalist.
	 */
	*rdatalist = (dns_rdatalist_t){
		.rdata = ISC_LIST_INITIALIZER,
		.link = ISC_LINK_INITIALIZER,
	};
	memset(rdatalist->upper, 0xeb, sizeof(rdatalist->upper));

	/*
	 * Clear upper set bit.
	 */
	rdatalist->upper[0] &= ~0x01;
}

void
dns_rdatalist_tordataset(dns_rdatalist_t *rdatalist, dns_rdataset_t *rdataset) {
	/*
	 * Make 'rdataset' refer to the rdata in 'rdatalist'.
	 */

	REQUIRE(rdatalist != NULL);
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(!dns_rdataset_isassociated(rdataset));

	/* Check if dns_rdatalist_init has was called. */
	REQUIRE(rdatalist->upper[0] == 0xea);

	*rdataset = (dns_rdataset_t){
		.methods = &methods,
		.rdclass = rdatalist->rdclass,
		.type = rdatalist->type,
		.covers = rdatalist->covers,
		.ttl = rdatalist->ttl,
		.rdlist.list = rdatalist,

		.link = rdataset->link,
		.count = rdataset->count,
		.attributes = rdataset->attributes,
		.magic = rdataset->magic,
	};
}

void
dns_rdatalist_fromrdataset(dns_rdataset_t *rdataset,
			   dns_rdatalist_t **rdatalist) {
	REQUIRE(rdatalist != NULL && rdataset != NULL);
	REQUIRE(rdataset->methods == &methods);

	*rdatalist = rdataset->rdlist.list;
}

void
dns_rdatalist_disassociate(dns_rdataset_t *rdataset DNS__DB_FLARG) {
	UNUSED(rdataset);
}

isc_result_t
dns_rdatalist_first(dns_rdataset_t *rdataset) {
	dns_rdatalist_t *rdatalist = NULL;

	rdatalist = rdataset->rdlist.list;
	rdataset->rdlist.iter = ISC_LIST_HEAD(rdatalist->rdata);

	if (rdataset->rdlist.iter == NULL) {
		return ISC_R_NOMORE;
	}

	return ISC_R_SUCCESS;
}

isc_result_t
dns_rdatalist_next(dns_rdataset_t *rdataset) {
	dns_rdata_t *rdata;

	rdata = rdataset->rdlist.iter;
	if (rdata == NULL) {
		return ISC_R_NOMORE;
	}

	rdataset->rdlist.iter = ISC_LIST_NEXT(rdata, link);

	if (rdataset->rdlist.iter == NULL) {
		return ISC_R_NOMORE;
	}

	return ISC_R_SUCCESS;
}

void
dns_rdatalist_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata) {
	dns_rdata_t *list_rdata;

	list_rdata = rdataset->rdlist.iter;
	INSIST(list_rdata != NULL);

	dns_rdata_clone(list_rdata, rdata);
}

void
dns_rdatalist_clone(dns_rdataset_t *source,
		    dns_rdataset_t *target DNS__DB_FLARG) {
	REQUIRE(source != NULL);
	REQUIRE(target != NULL);

	*target = *source;

	target->rdlist.iter = NULL;
}

unsigned int
dns_rdatalist_count(dns_rdataset_t *rdataset) {
	dns_rdatalist_t *rdatalist;
	dns_rdata_t *rdata;
	unsigned int count;

	REQUIRE(rdataset != NULL);

	rdatalist = rdataset->rdlist.list;

	count = 0;
	for (rdata = ISC_LIST_HEAD(rdatalist->rdata); rdata != NULL;
	     rdata = ISC_LIST_NEXT(rdata, link))
	{
		count++;
	}

	return count;
}

/*
 * Find the denial of existence proof of the given type at 'name': the
 * NSEC or NSEC3 rdataset together with the RRSIG rdataset covering it.
 */
static bool
findproof(const dns_name_t *name, dns_rdataclass_t rdclass,
	  dns_rdatatype_t type, dns_rdataset_t **negp,
	  dns_rdataset_t **negsigp) {
	dns_rdataset_t *neg = NULL;
	dns_rdataset_t *negsig = NULL;
	dns_rdataset_t *rdset = NULL;

	REQUIRE(type == dns_rdatatype_nsec || type == dns_rdatatype_nsec3);

	ISC_LIST_FOREACH(name->list, rdset, link) {
		if (rdset->rdclass != rdclass) {
			continue;
		}

		if (rdset->type == type) {
			neg = rdset;
		} else if (rdset->type == dns_rdatatype_rrsig &&
			   rdset->covers == type)
		{
			negsig = rdset;
		}

		if (neg != NULL && negsig != NULL) {
			*negp = neg;
			*negsigp = negsig;
			return true;
		}
	}

	return false;
}

isc_result_t
dns_rdatalist_addnoqname(dns_rdataset_t *rdataset, const dns_name_t *name,
			 dns_rdatatype_t type) {
	dns_rdataset_t *neg = NULL;
	dns_rdataset_t *negsig = NULL;
	dns_ttl_t ttl;

	REQUIRE(rdataset != NULL);
	REQUIRE(DNS_NAME_VALID(name));

	if (!findproof(name, rdataset->rdclass, type, &neg, &negsig)) {
		return ISC_R_NOTFOUND;
	}

	/*
	 * Minimise ttl.
	 */
	ttl = ISC_MIN(rdataset->ttl, ISC_MIN(neg->ttl, negsig->ttl));
	rdataset->ttl = neg->ttl = negsig->ttl = ttl;
	rdataset->attributes |= DNS_RDATASETATTR_NOQNAME;
	rdataset->rdlist.noqname = name;
	rdataset->rdlist.noqnametype = type;

	return ISC_R_SUCCESS;
}

isc_result_t
dns_rdatalist_getnoqname(dns_rdataset_t *rdataset, dns_name_t *name,
			 dns_rdataset_t *neg,
			 dns_rdataset_t *negsig DNS__DB_FLARG) {
	dns_rdataset_t *tneg = NULL;
	dns_rdataset_t *tnegsig = NULL;
	const dns_name_t *noqname;

	REQUIRE(rdataset != NULL);
	REQUIRE((rdataset->attributes & DNS_RDATASETATTR_NOQNAME) != 0);

	noqname = rdataset->rdlist.noqname;

	REQUIRE(DNS_NAME_VALID(noqname));

	if (!findproof(noqname, rdataset->rdclass, rdataset->rdlist.noqnametype,
		       &tneg, &tnegsig))
	{
		return ISC_R_NOTFOUND;
	}

	dns_name_clone(noqname, name);
	dns_rdataset_clone(tneg, neg);
	dns_rdataset_clone(tnegsig, negsig);
	return ISC_R_SUCCESS;
}

void
dns_rdatalist_setownercase(dns_rdataset_t *rdataset, const dns_name_t *name) {
	dns_rdatalist_t *rdatalist;
	unsigned int i;

	/*
	 * We do not need to worry about label lengths as they are all
	 * less than or equal to 63.
	 */
	rdatalist = rdataset->rdlist.list;
	memset(rdatalist->upper, 0, sizeof(rdatalist->upper));
	for (i = 1; i < name->length; i++) {
		if (name->ndata[i] >= 0x41 && name->ndata[i] <= 0x5a) {
			rdatalist->upper[i / 8] |= 1 << (i % 8);
		}
	}
	/*
	 * Record that upper has been set.
	 */
	rdatalist->upper[0] |= 0x01;
}

void
dns_rdatalist_getownercase(const dns_rdataset_t *rdataset, dns_name_t *name) {
	dns_rdatalist_t *rdatalist;
	unsigned int i;

	rdatalist = rdataset->rdlist.list;
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
