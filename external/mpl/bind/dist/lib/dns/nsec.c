/*	$NetBSD: nsec.c,v 1.3.4.1 2019/10/17 19:34:20 martin Exp $	*/

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

#include <stdbool.h>

#include <isc/log.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/nsec.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdatastruct.h>
#include <dns/result.h>

#include <dst/dst.h>

#define RETERR(x) do { \
	result = (x); \
	if (result != ISC_R_SUCCESS) \
		goto failure; \
	} while (/*CONSTCOND*/0)

void
dns_nsec_setbit(unsigned char *array, unsigned int type, unsigned int bit) {
	unsigned int shift, mask;

	shift = 7 - (type % 8);
	mask = 1 << shift;

	if (bit != 0)
		array[type / 8] |= mask;
	else
		array[type / 8] &= (~mask & 0xFF);
}

bool
dns_nsec_isset(const unsigned char *array, unsigned int type) {
	unsigned int byte, shift, mask;

	byte = array[type / 8];
	shift = 7 - (type % 8);
	mask = 1 << shift;

	return ((byte & mask) != 0);
}

unsigned int
dns_nsec_compressbitmap(unsigned char *map, const unsigned char *raw,
			unsigned int max_type)
{
	unsigned char *start = map;
	unsigned int window;
	int octet;

	if (raw == NULL)
		return (0);

	for (window = 0; window < 256; window++) {
		if (window * 256 > max_type)
			break;
		for (octet = 31; octet >= 0; octet--)
			if (*(raw + octet) != 0)
				break;
		if (octet < 0) {
			raw += 32;
			continue;
		}
		*map++ = window;
		*map++ = octet + 1;
		/*
		 * Note: potential overlapping move.
		 */
		memmove(map, raw, octet + 1);
		map += octet + 1;
		raw += 32;
	}
	return (unsigned int)(map - start);
}

isc_result_t
dns_nsec_buildrdata(dns_db_t *db, dns_dbversion_t *version,
		    dns_dbnode_t *node, const dns_name_t *target,
		    unsigned char *buffer, dns_rdata_t *rdata)
{
	isc_result_t result;
	dns_rdataset_t rdataset;
	isc_region_t r;
	unsigned int i;

	unsigned char *nsec_bits, *bm;
	unsigned int max_type;
	dns_rdatasetiter_t *rdsiter;

	REQUIRE(target != NULL);

	memset(buffer, 0, DNS_NSEC_BUFFERSIZE);
	dns_name_toregion(target, &r);
	memmove(buffer, r.base, r.length);
	r.base = buffer;
	/*
	 * Use the end of the space for a raw bitmap leaving enough
	 * space for the window identifiers and length octets.
	 */
	bm = r.base + r.length + 512;
	nsec_bits = r.base + r.length;
	dns_nsec_setbit(bm, dns_rdatatype_rrsig, 1);
	dns_nsec_setbit(bm, dns_rdatatype_nsec, 1);
	max_type = dns_rdatatype_nsec;
	dns_rdataset_init(&rdataset);
	rdsiter = NULL;
	result = dns_db_allrdatasets(db, node, version, 0, &rdsiter);
	if (result != ISC_R_SUCCESS)
		return (result);
	for (result = dns_rdatasetiter_first(rdsiter);
	     result == ISC_R_SUCCESS;
	     result = dns_rdatasetiter_next(rdsiter))
	{
		dns_rdatasetiter_current(rdsiter, &rdataset);
		if (rdataset.type != dns_rdatatype_nsec &&
		    rdataset.type != dns_rdatatype_nsec3 &&
		    rdataset.type != dns_rdatatype_rrsig) {
			if (rdataset.type > max_type)
				max_type = rdataset.type;
			dns_nsec_setbit(bm, rdataset.type, 1);
		}
		dns_rdataset_disassociate(&rdataset);
	}

	/*
	 * At zone cuts, deny the existence of glue in the parent zone.
	 */
	if (dns_nsec_isset(bm, dns_rdatatype_ns) &&
	    ! dns_nsec_isset(bm, dns_rdatatype_soa)) {
		for (i = 0; i <= max_type; i++) {
			if (dns_nsec_isset(bm, i) &&
			    ! dns_rdatatype_iszonecutauth((dns_rdatatype_t)i))
				dns_nsec_setbit(bm, i, 0);
		}
	}

	dns_rdatasetiter_destroy(&rdsiter);
	if (result != ISC_R_NOMORE)
		return (result);

	nsec_bits += dns_nsec_compressbitmap(nsec_bits, bm, max_type);

	r.length = (unsigned int)(nsec_bits - r.base);
	INSIST(r.length <= DNS_NSEC_BUFFERSIZE);
	dns_rdata_fromregion(rdata,
			     dns_db_class(db),
			     dns_rdatatype_nsec,
			     &r);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_nsec_build(dns_db_t *db, dns_dbversion_t *version, dns_dbnode_t *node,
	       const dns_name_t *target, dns_ttl_t ttl)
{
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned char data[DNS_NSEC_BUFFERSIZE];
	dns_rdatalist_t rdatalist;
	dns_rdataset_t rdataset;

	dns_rdataset_init(&rdataset);
	dns_rdata_init(&rdata);

	RETERR(dns_nsec_buildrdata(db, version, node, target, data, &rdata));

	dns_rdatalist_init(&rdatalist);
	rdatalist.rdclass = dns_db_class(db);
	rdatalist.type = dns_rdatatype_nsec;
	rdatalist.ttl = ttl;
	ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);
	RETERR(dns_rdatalist_tordataset(&rdatalist, &rdataset));
	result = dns_db_addrdataset(db, node, version, 0, &rdataset,
				    0, NULL);
	if (result == DNS_R_UNCHANGED)
		result = ISC_R_SUCCESS;

 failure:
	if (dns_rdataset_isassociated(&rdataset))
		dns_rdataset_disassociate(&rdataset);
	return (result);
}

bool
dns_nsec_typepresent(dns_rdata_t *nsec, dns_rdatatype_t type) {
	dns_rdata_nsec_t nsecstruct;
	isc_result_t result;
	bool present;
	unsigned int i, len, window;

	REQUIRE(nsec != NULL);
	REQUIRE(nsec->type == dns_rdatatype_nsec);

	/* This should never fail */
	result = dns_rdata_tostruct(nsec, &nsecstruct, NULL);
	INSIST(result == ISC_R_SUCCESS);

	present = false;
	for (i = 0; i < nsecstruct.len; i += len) {
		INSIST(i + 2 <= nsecstruct.len);
		window = nsecstruct.typebits[i];
		len = nsecstruct.typebits[i + 1];
		INSIST(len > 0 && len <= 32);
		i += 2;
		INSIST(i + len <= nsecstruct.len);
		if (window * 256 > type) {
			break;
		}
		if ((window + 1) * 256 <= type) {
			continue;
		}
		if (type < (window * 256) + len * 8) {
			present =
				dns_nsec_isset(&nsecstruct.typebits[i], type % 256);
		}
		break;
	}
	dns_rdata_freestruct(&nsecstruct);
	return (present);
}

isc_result_t
dns_nsec_nseconly(dns_db_t *db, dns_dbversion_t *version,
		  bool *answer)
{
	dns_dbnode_t *node = NULL;
	dns_rdataset_t rdataset;
	dns_rdata_dnskey_t dnskey;
	isc_result_t result;

	REQUIRE(answer != NULL);

	dns_rdataset_init(&rdataset);

	result = dns_db_getoriginnode(db, &node);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_db_findrdataset(db, node, version, dns_rdatatype_dnskey,
				     0, 0, &rdataset, NULL);
	dns_db_detachnode(db, &node);

	if (result == ISC_R_NOTFOUND)
		*answer = false;
	if (result != ISC_R_SUCCESS)
		return (result);
	for (result = dns_rdataset_first(&rdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(&rdataset)) {
		dns_rdata_t rdata = DNS_RDATA_INIT;

		dns_rdataset_current(&rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &dnskey, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		if (dnskey.algorithm == DST_ALG_RSAMD5 ||
		    dnskey.algorithm == DST_ALG_RSASHA1)
			break;
	}
	dns_rdataset_disassociate(&rdataset);
	if (result == ISC_R_SUCCESS)
		*answer = true;
	if (result == ISC_R_NOMORE) {
		*answer = false;
		result = ISC_R_SUCCESS;
	}
	return (result);
}

/*%
 * Return ISC_R_SUCCESS if we can determine that the name doesn't exist
 * or we can determine whether there is data or not at the name.
 * If the name does not exist return the wildcard name.
 *
 * Return ISC_R_IGNORE when the NSEC is not the appropriate one.
 */
isc_result_t
dns_nsec_noexistnodata(dns_rdatatype_t type, const dns_name_t *name,
		       const dns_name_t *nsecname, dns_rdataset_t *nsecset,
		       bool *exists, bool *data,
		       dns_name_t *wild, dns_nseclog_t logit, void *arg)
{
	int order;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_result_t result;
	dns_namereln_t relation;
	unsigned int olabels, nlabels, labels;
	dns_rdata_nsec_t nsec;
	bool atparent;
	bool ns;
	bool soa;

	REQUIRE(exists != NULL);
	REQUIRE(data != NULL);
	REQUIRE(nsecset != NULL &&
		nsecset->type == dns_rdatatype_nsec);

	result = dns_rdataset_first(nsecset);
	if (result != ISC_R_SUCCESS) {
		(*logit)(arg, ISC_LOG_DEBUG(3), "failure processing NSEC set");
		return (result);
	}
	dns_rdataset_current(nsecset, &rdata);

	(*logit)(arg, ISC_LOG_DEBUG(3), "looking for relevant NSEC");
	relation = dns_name_fullcompare(name, nsecname, &order, &olabels);

	if (order < 0) {
		/*
		 * The name is not within the NSEC range.
		 */
		(*logit)(arg, ISC_LOG_DEBUG(3),
			      "NSEC does not cover name, before NSEC");
		return (ISC_R_IGNORE);
	}

	if (order == 0) {
		/*
		 * The names are the same.   If we are validating "."
		 * then atparent should not be set as there is no parent.
		 */
		atparent = (olabels != 1) && dns_rdatatype_atparent(type);
		ns = dns_nsec_typepresent(&rdata, dns_rdatatype_ns);
		soa = dns_nsec_typepresent(&rdata, dns_rdatatype_soa);
		if (ns && !soa) {
			if (!atparent) {
				/*
				 * This NSEC record is from somewhere higher in
				 * the DNS, and at the parent of a delegation.
				 * It can not be legitimately used here.
				 */
				(*logit)(arg, ISC_LOG_DEBUG(3),
					      "ignoring parent nsec");
				return (ISC_R_IGNORE);
			}
		} else if (atparent && ns && soa) {
			/*
			 * This NSEC record is from the child.
			 * It can not be legitimately used here.
			 */
			(*logit)(arg, ISC_LOG_DEBUG(3),
				      "ignoring child nsec");
			return (ISC_R_IGNORE);
		}
		if (type == dns_rdatatype_cname || type == dns_rdatatype_nxt ||
		    type == dns_rdatatype_nsec || type == dns_rdatatype_key ||
		    !dns_nsec_typepresent(&rdata, dns_rdatatype_cname)) {
			*exists = true;
			*data = dns_nsec_typepresent(&rdata, type);
			(*logit)(arg, ISC_LOG_DEBUG(3),
				      "nsec proves name exists (owner) data=%d",
				      *data);
			return (ISC_R_SUCCESS);
		}
		(*logit)(arg, ISC_LOG_DEBUG(3), "NSEC proves CNAME exists");
		return (ISC_R_IGNORE);
	}

	if (relation == dns_namereln_subdomain &&
	    dns_nsec_typepresent(&rdata, dns_rdatatype_ns) &&
	    !dns_nsec_typepresent(&rdata, dns_rdatatype_soa))
	{
		/*
		 * This NSEC record is from somewhere higher in
		 * the DNS, and at the parent of a delegation or
		 * at a DNAME.
		 * It can not be legitimately used here.
		 */
		(*logit)(arg, ISC_LOG_DEBUG(3), "ignoring parent nsec");
		return (ISC_R_IGNORE);
	}

	if (relation == dns_namereln_subdomain &&
	    dns_nsec_typepresent(&rdata, dns_rdatatype_dname))
	{
		(*logit)(arg, ISC_LOG_DEBUG(3),
			 "nsec proves covered by dname");
		*exists = false;
		return (DNS_R_DNAME);
	}

	result = dns_rdata_tostruct(&rdata, &nsec, NULL);
	if (result != ISC_R_SUCCESS)
		return (result);
	relation = dns_name_fullcompare(&nsec.next, name, &order, &nlabels);
	if (order == 0) {
		dns_rdata_freestruct(&nsec);
		(*logit)(arg, ISC_LOG_DEBUG(3),
			      "ignoring nsec matches next name");
		return (ISC_R_IGNORE);
	}

	if (order < 0 && !dns_name_issubdomain(nsecname, &nsec.next)) {
		/*
		 * The name is not within the NSEC range.
		 */
		dns_rdata_freestruct(&nsec);
		(*logit)(arg, ISC_LOG_DEBUG(3),
			    "ignoring nsec because name is past end of range");
		return (ISC_R_IGNORE);
	}

	if (order > 0 && relation == dns_namereln_subdomain) {
		(*logit)(arg, ISC_LOG_DEBUG(3),
			      "nsec proves name exist (empty)");
		dns_rdata_freestruct(&nsec);
		*exists = true;
		*data = false;
		return (ISC_R_SUCCESS);
	}
	if (wild != NULL) {
		dns_name_t common;
		dns_name_init(&common, NULL);
		if (olabels > nlabels) {
			labels = dns_name_countlabels(nsecname);
			dns_name_getlabelsequence(nsecname, labels - olabels,
						  olabels, &common);
		} else {
			labels = dns_name_countlabels(&nsec.next);
			dns_name_getlabelsequence(&nsec.next, labels - nlabels,
						  nlabels, &common);
		}
		result = dns_name_concatenate(dns_wildcardname, &common,
					      wild, NULL);
		if (result != ISC_R_SUCCESS) {
			dns_rdata_freestruct(&nsec);
			(*logit)(arg, ISC_LOG_DEBUG(3),
				    "failure generating wildcard name");
			return (result);
		}
	}
	dns_rdata_freestruct(&nsec);
	(*logit)(arg, ISC_LOG_DEBUG(3), "nsec range ok");
	*exists = false;
	return (ISC_R_SUCCESS);
}
