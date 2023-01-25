/*	$NetBSD: zoneverify.c,v 1.10 2023/01/25 21:43:30 christos Exp $	*/

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
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <isc/base32.h>
#include <isc/buffer.h>
#include <isc/heap.h>
#include <isc/iterated_hash.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/keytable.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/nsec.h>
#include <dns/nsec3.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/secalg.h>
#include <dns/types.h>
#include <dns/zone.h>
#include <dns/zoneverify.h>

#include <dst/dst.h>

typedef struct vctx {
	isc_mem_t *mctx;
	dns_zone_t *zone;
	dns_db_t *db;
	dns_dbversion_t *ver;
	dns_name_t *origin;
	dns_keytable_t *secroots;
	bool goodksk;
	bool goodzsk;
	dns_rdataset_t keyset;
	dns_rdataset_t keysigs;
	dns_rdataset_t soaset;
	dns_rdataset_t soasigs;
	dns_rdataset_t nsecset;
	dns_rdataset_t nsecsigs;
	dns_rdataset_t nsec3paramset;
	dns_rdataset_t nsec3paramsigs;
	unsigned char revoked_ksk[256];
	unsigned char revoked_zsk[256];
	unsigned char standby_ksk[256];
	unsigned char standby_zsk[256];
	unsigned char ksk_algorithms[256];
	unsigned char zsk_algorithms[256];
	unsigned char bad_algorithms[256];
	unsigned char act_algorithms[256];
	isc_heap_t *expected_chains;
	isc_heap_t *found_chains;
} vctx_t;

struct nsec3_chain_fixed {
	uint8_t hash;
	uint8_t salt_length;
	uint8_t next_length;
	uint16_t iterations;
	/*
	 * The following non-fixed-length data is stored in memory after the
	 * fields declared above for each NSEC3 chain element:
	 *
	 * unsigned char	salt[salt_length];
	 * unsigned char	owner[next_length];
	 * unsigned char	next[next_length];
	 */
};

/*
 * Helper function used to calculate length of variable-length
 * data section in object pointed to by 'chain'.
 */
static size_t
chain_length(struct nsec3_chain_fixed *chain) {
	return (chain->salt_length + 2 * chain->next_length);
}

/*%
 * Log a zone verification error described by 'fmt' and the variable arguments
 * following it.  Either use dns_zone_logv() or print to stderr, depending on
 * whether the function was invoked from within named or by a standalone tool,
 * respectively.
 */
static void
zoneverify_log_error(const vctx_t *vctx, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	if (vctx->zone != NULL) {
		dns_zone_logv(vctx->zone, DNS_LOGCATEGORY_GENERAL,
			      ISC_LOG_ERROR, NULL, fmt, ap);
	} else {
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, "\n");
	}
	va_end(ap);
}

static bool
is_delegation(const vctx_t *vctx, const dns_name_t *name, dns_dbnode_t *node,
	      uint32_t *ttlp) {
	dns_rdataset_t nsset;
	isc_result_t result;

	if (dns_name_equal(name, vctx->origin)) {
		return (false);
	}

	dns_rdataset_init(&nsset);
	result = dns_db_findrdataset(vctx->db, node, vctx->ver,
				     dns_rdatatype_ns, 0, 0, &nsset, NULL);
	if (dns_rdataset_isassociated(&nsset)) {
		if (ttlp != NULL) {
			*ttlp = nsset.ttl;
		}
		dns_rdataset_disassociate(&nsset);
	}

	return ((result == ISC_R_SUCCESS));
}

/*%
 * Return true if version 'ver' of database 'db' contains a DNAME RRset at
 * 'node'; return false otherwise.
 */
static bool
has_dname(const vctx_t *vctx, dns_dbnode_t *node) {
	dns_rdataset_t dnameset;
	isc_result_t result;

	dns_rdataset_init(&dnameset);
	result = dns_db_findrdataset(vctx->db, node, vctx->ver,
				     dns_rdatatype_dname, 0, 0, &dnameset,
				     NULL);
	if (dns_rdataset_isassociated(&dnameset)) {
		dns_rdataset_disassociate(&dnameset);
	}

	return ((result == ISC_R_SUCCESS));
}

static bool
goodsig(const vctx_t *vctx, dns_rdata_t *sigrdata, const dns_name_t *name,
	dst_key_t **dstkeys, size_t nkeys, dns_rdataset_t *rdataset) {
	dns_rdata_rrsig_t sig;
	isc_result_t result;

	result = dns_rdata_tostruct(sigrdata, &sig, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	for (size_t key = 0; key < nkeys; key++) {
		if (sig.algorithm != dst_key_alg(dstkeys[key]) ||
		    sig.keyid != dst_key_id(dstkeys[key]) ||
		    !dns_name_equal(&sig.signer, vctx->origin))
		{
			continue;
		}
		result = dns_dnssec_verify(name, rdataset, dstkeys[key], false,
					   0, vctx->mctx, sigrdata, NULL);
		if (result == ISC_R_SUCCESS || result == DNS_R_FROMWILDCARD) {
			return (true);
		}
	}
	return (false);
}

static bool
nsec_bitmap_equal(dns_rdata_nsec_t *nsec, dns_rdata_t *rdata) {
	isc_result_t result;
	dns_rdata_nsec_t tmpnsec;

	result = dns_rdata_tostruct(rdata, &tmpnsec, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	if (nsec->len != tmpnsec.len ||
	    memcmp(nsec->typebits, tmpnsec.typebits, nsec->len) != 0)
	{
		return (false);
	}
	return (true);
}

static isc_result_t
verifynsec(const vctx_t *vctx, const dns_name_t *name, dns_dbnode_t *node,
	   const dns_name_t *nextname, isc_result_t *vresult) {
	unsigned char buffer[DNS_NSEC_BUFFERSIZE];
	char namebuf[DNS_NAME_FORMATSIZE];
	char nextbuf[DNS_NAME_FORMATSIZE];
	char found[DNS_NAME_FORMATSIZE];
	dns_rdataset_t rdataset;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_t tmprdata = DNS_RDATA_INIT;
	dns_rdata_nsec_t nsec;
	isc_result_t result;

	dns_rdataset_init(&rdataset);
	result = dns_db_findrdataset(vctx->db, node, vctx->ver,
				     dns_rdatatype_nsec, 0, 0, &rdataset, NULL);
	if (result != ISC_R_SUCCESS) {
		dns_name_format(name, namebuf, sizeof(namebuf));
		zoneverify_log_error(vctx, "Missing NSEC record for %s",
				     namebuf);
		*vresult = ISC_R_FAILURE;
		result = ISC_R_SUCCESS;
		goto done;
	}

	result = dns_rdataset_first(&rdataset);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "dns_rdataset_first(): %s",
				     isc_result_totext(result));
		goto done;
	}

	dns_rdataset_current(&rdataset, &rdata);
	result = dns_rdata_tostruct(&rdata, &nsec, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	/* Check next name is consistent */
	if (!dns_name_equal(&nsec.next, nextname)) {
		dns_name_format(name, namebuf, sizeof(namebuf));
		dns_name_format(nextname, nextbuf, sizeof(nextbuf));
		dns_name_format(&nsec.next, found, sizeof(found));
		zoneverify_log_error(vctx,
				     "Bad NSEC record for %s, next name "
				     "mismatch (expected:%s, found:%s)",
				     namebuf, nextbuf, found);
		*vresult = ISC_R_FAILURE;
		goto done;
	}

	/* Check bit map is consistent */
	result = dns_nsec_buildrdata(vctx->db, vctx->ver, node, nextname,
				     buffer, &tmprdata);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "dns_nsec_buildrdata(): %s",
				     isc_result_totext(result));
		goto done;
	}
	if (!nsec_bitmap_equal(&nsec, &tmprdata)) {
		dns_name_format(name, namebuf, sizeof(namebuf));
		zoneverify_log_error(vctx,
				     "Bad NSEC record for %s, bit map "
				     "mismatch",
				     namebuf);
		*vresult = ISC_R_FAILURE;
		goto done;
	}

	result = dns_rdataset_next(&rdataset);
	if (result != ISC_R_NOMORE) {
		dns_name_format(name, namebuf, sizeof(namebuf));
		zoneverify_log_error(vctx, "Multiple NSEC records for %s",
				     namebuf);
		*vresult = ISC_R_FAILURE;
		goto done;
	}

	*vresult = ISC_R_SUCCESS;
	result = ISC_R_SUCCESS;

done:
	if (dns_rdataset_isassociated(&rdataset)) {
		dns_rdataset_disassociate(&rdataset);
	}

	return (result);
}

static isc_result_t
check_no_rrsig(const vctx_t *vctx, const dns_rdataset_t *rdataset,
	       const dns_name_t *name, dns_dbnode_t *node) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	dns_rdataset_t sigrdataset;
	dns_rdatasetiter_t *rdsiter = NULL;
	isc_result_t result;

	dns_rdataset_init(&sigrdataset);
	result = dns_db_allrdatasets(vctx->db, node, vctx->ver, 0, 0, &rdsiter);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "dns_db_allrdatasets(): %s",
				     isc_result_totext(result));
		return (result);
	}
	for (result = dns_rdatasetiter_first(rdsiter); result == ISC_R_SUCCESS;
	     result = dns_rdatasetiter_next(rdsiter))
	{
		dns_rdatasetiter_current(rdsiter, &sigrdataset);
		if (sigrdataset.type == dns_rdatatype_rrsig &&
		    sigrdataset.covers == rdataset->type)
		{
			dns_name_format(name, namebuf, sizeof(namebuf));
			dns_rdatatype_format(rdataset->type, typebuf,
					     sizeof(typebuf));
			zoneverify_log_error(
				vctx,
				"Warning: Found unexpected signatures "
				"for %s/%s",
				namebuf, typebuf);
			break;
		}
		dns_rdataset_disassociate(&sigrdataset);
	}
	if (dns_rdataset_isassociated(&sigrdataset)) {
		dns_rdataset_disassociate(&sigrdataset);
	}
	dns_rdatasetiter_destroy(&rdsiter);

	return (ISC_R_SUCCESS);
}

static bool
chain_compare(void *arg1, void *arg2) {
	struct nsec3_chain_fixed *e1 = arg1, *e2 = arg2;
	/*
	 * Do each element in turn to get a stable sort.
	 */
	if (e1->hash < e2->hash) {
		return (true);
	}
	if (e1->hash > e2->hash) {
		return (false);
	}
	if (e1->iterations < e2->iterations) {
		return (true);
	}
	if (e1->iterations > e2->iterations) {
		return (false);
	}
	if (e1->salt_length < e2->salt_length) {
		return (true);
	}
	if (e1->salt_length > e2->salt_length) {
		return (false);
	}
	if (e1->next_length < e2->next_length) {
		return (true);
	}
	if (e1->next_length > e2->next_length) {
		return (false);
	}
	if (memcmp(e1 + 1, e2 + 1, chain_length(e1)) < 0) {
		return (true);
	}
	return (false);
}

static bool
chain_equal(const struct nsec3_chain_fixed *e1,
	    const struct nsec3_chain_fixed *e2, size_t data_length) {
	if (e1->hash != e2->hash) {
		return (false);
	}
	if (e1->iterations != e2->iterations) {
		return (false);
	}
	if (e1->salt_length != e2->salt_length) {
		return (false);
	}
	if (e1->next_length != e2->next_length) {
		return (false);
	}

	return (memcmp(e1 + 1, e2 + 1, data_length) == 0);
}

static void
record_nsec3(const vctx_t *vctx, const unsigned char *rawhash,
	     const dns_rdata_nsec3_t *nsec3, isc_heap_t *chains) {
	struct nsec3_chain_fixed *element = NULL;
	unsigned char *cp = NULL;
	size_t len;

	len = sizeof(*element) + nsec3->next_length * 2 + nsec3->salt_length;

	element = isc_mem_get(vctx->mctx, len);
	memset(element, 0, len);
	element->hash = nsec3->hash;
	element->salt_length = nsec3->salt_length;
	element->next_length = nsec3->next_length;
	element->iterations = nsec3->iterations;
	cp = (unsigned char *)(element + 1);
	memmove(cp, nsec3->salt, nsec3->salt_length);
	cp += nsec3->salt_length;
	memmove(cp, rawhash, nsec3->next_length);
	cp += nsec3->next_length;
	memmove(cp, nsec3->next, nsec3->next_length);
	isc_heap_insert(chains, element);
}

/*
 * Check whether any NSEC3 within 'rdataset' matches the parameters in
 * 'nsec3param'.
 */
static isc_result_t
find_nsec3_match(const dns_rdata_nsec3param_t *nsec3param,
		 dns_rdataset_t *rdataset, size_t rhsize,
		 dns_rdata_nsec3_t *nsec3_match) {
	isc_result_t result;

	/*
	 * Find matching NSEC3 record.
	 */
	for (result = dns_rdataset_first(rdataset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(rdataset))
	{
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, nsec3_match, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		if (nsec3_match->hash == nsec3param->hash &&
		    nsec3_match->next_length == rhsize &&
		    nsec3_match->iterations == nsec3param->iterations &&
		    nsec3_match->salt_length == nsec3param->salt_length &&
		    memcmp(nsec3_match->salt, nsec3param->salt,
			   nsec3param->salt_length) == 0)
		{
			return (ISC_R_SUCCESS);
		}
	}

	return (result);
}

static isc_result_t
match_nsec3(const vctx_t *vctx, const dns_name_t *name,
	    const dns_rdata_nsec3param_t *nsec3param, dns_rdataset_t *rdataset,
	    const unsigned char types[8192], unsigned int maxtype,
	    const unsigned char *rawhash, size_t rhsize,
	    isc_result_t *vresult) {
	unsigned char cbm[8244];
	char namebuf[DNS_NAME_FORMATSIZE];
	dns_rdata_nsec3_t nsec3;
	isc_result_t result;
	unsigned int len;

	result = find_nsec3_match(nsec3param, rdataset, rhsize, &nsec3);
	if (result != ISC_R_SUCCESS) {
		dns_name_format(name, namebuf, sizeof(namebuf));
		zoneverify_log_error(vctx, "Missing NSEC3 record for %s",
				     namebuf);
		*vresult = result;
		return (ISC_R_SUCCESS);
	}

	/*
	 * Check the type list.
	 */
	len = dns_nsec_compressbitmap(cbm, types, maxtype);
	if (nsec3.len != len || memcmp(cbm, nsec3.typebits, len) != 0) {
		dns_name_format(name, namebuf, sizeof(namebuf));
		zoneverify_log_error(vctx,
				     "Bad NSEC3 record for %s, bit map "
				     "mismatch",
				     namebuf);
		*vresult = ISC_R_FAILURE;
		return (ISC_R_SUCCESS);
	}

	/*
	 * Record chain.
	 */
	record_nsec3(vctx, rawhash, &nsec3, vctx->expected_chains);

	/*
	 * Make sure there is only one NSEC3 record with this set of
	 * parameters.
	 */
	for (result = dns_rdataset_next(rdataset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(rdataset))
	{
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &nsec3, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		if (nsec3.hash == nsec3param->hash &&
		    nsec3.iterations == nsec3param->iterations &&
		    nsec3.salt_length == nsec3param->salt_length &&
		    memcmp(nsec3.salt, nsec3param->salt, nsec3.salt_length) ==
			    0)
		{
			dns_name_format(name, namebuf, sizeof(namebuf));
			zoneverify_log_error(vctx,
					     "Multiple NSEC3 records with the "
					     "same parameter set for %s",
					     namebuf);
			*vresult = DNS_R_DUPLICATE;
			return (ISC_R_SUCCESS);
		}
	}
	if (result != ISC_R_NOMORE) {
		return (result);
	}

	*vresult = ISC_R_SUCCESS;

	return (ISC_R_SUCCESS);
}

static bool
innsec3params(const dns_rdata_nsec3_t *nsec3, dns_rdataset_t *nsec3paramset) {
	dns_rdata_nsec3param_t nsec3param;
	isc_result_t result;

	for (result = dns_rdataset_first(nsec3paramset);
	     result == ISC_R_SUCCESS; result = dns_rdataset_next(nsec3paramset))
	{
		dns_rdata_t rdata = DNS_RDATA_INIT;

		dns_rdataset_current(nsec3paramset, &rdata);
		result = dns_rdata_tostruct(&rdata, &nsec3param, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		if (nsec3param.flags == 0 && nsec3param.hash == nsec3->hash &&
		    nsec3param.iterations == nsec3->iterations &&
		    nsec3param.salt_length == nsec3->salt_length &&
		    memcmp(nsec3param.salt, nsec3->salt, nsec3->salt_length) ==
			    0)
		{
			return (true);
		}
	}
	return (false);
}

static isc_result_t
record_found(const vctx_t *vctx, const dns_name_t *name, dns_dbnode_t *node,
	     dns_rdataset_t *nsec3paramset) {
	unsigned char owner[NSEC3_MAX_HASH_LENGTH];
	dns_rdata_nsec3_t nsec3;
	dns_rdataset_t rdataset;
	dns_label_t hashlabel;
	isc_buffer_t b;
	isc_result_t result;

	if (nsec3paramset == NULL || !dns_rdataset_isassociated(nsec3paramset))
	{
		return (ISC_R_SUCCESS);
	}

	dns_rdataset_init(&rdataset);
	result = dns_db_findrdataset(vctx->db, node, vctx->ver,
				     dns_rdatatype_nsec3, 0, 0, &rdataset,
				     NULL);
	if (result != ISC_R_SUCCESS) {
		return (ISC_R_SUCCESS);
	}

	dns_name_getlabel(name, 0, &hashlabel);
	isc_region_consume(&hashlabel, 1);
	isc_buffer_init(&b, owner, sizeof(owner));
	result = isc_base32hex_decoderegion(&hashlabel, &b);
	if (result != ISC_R_SUCCESS) {
		result = ISC_R_SUCCESS;
		goto cleanup;
	}

	for (result = dns_rdataset_first(&rdataset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(&rdataset))
	{
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdataset_current(&rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &nsec3, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		if (nsec3.next_length != isc_buffer_usedlength(&b)) {
			continue;
		}

		/*
		 * We only care about NSEC3 records that match a NSEC3PARAM
		 * record.
		 */
		if (!innsec3params(&nsec3, nsec3paramset)) {
			continue;
		}

		/*
		 * Record chain.
		 */
		record_nsec3(vctx, owner, &nsec3, vctx->found_chains);
	}
	result = ISC_R_SUCCESS;

cleanup:
	dns_rdataset_disassociate(&rdataset);
	return (result);
}

static isc_result_t
isoptout(const vctx_t *vctx, const dns_rdata_nsec3param_t *nsec3param,
	 bool *optout) {
	dns_rdataset_t rdataset;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_nsec3_t nsec3;
	dns_fixedname_t fixed;
	dns_name_t *hashname;
	isc_result_t result;
	dns_dbnode_t *node = NULL;
	unsigned char rawhash[NSEC3_MAX_HASH_LENGTH];
	size_t rhsize = sizeof(rawhash);

	dns_fixedname_init(&fixed);
	result = dns_nsec3_hashname(&fixed, rawhash, &rhsize, vctx->origin,
				    vctx->origin, nsec3param->hash,
				    nsec3param->iterations, nsec3param->salt,
				    nsec3param->salt_length);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "dns_nsec3_hashname(): %s",
				     isc_result_totext(result));
		return (result);
	}

	dns_rdataset_init(&rdataset);
	hashname = dns_fixedname_name(&fixed);
	result = dns_db_findnsec3node(vctx->db, hashname, false, &node);
	if (result == ISC_R_SUCCESS) {
		result = dns_db_findrdataset(vctx->db, node, vctx->ver,
					     dns_rdatatype_nsec3, 0, 0,
					     &rdataset, NULL);
	}
	if (result != ISC_R_SUCCESS) {
		*optout = false;
		result = ISC_R_SUCCESS;
		goto done;
	}

	result = dns_rdataset_first(&rdataset);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "dns_rdataset_first(): %s",
				     isc_result_totext(result));
		goto done;
	}

	dns_rdataset_current(&rdataset, &rdata);

	result = dns_rdata_tostruct(&rdata, &nsec3, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	*optout = ((nsec3.flags & DNS_NSEC3FLAG_OPTOUT) != 0);

done:
	if (dns_rdataset_isassociated(&rdataset)) {
		dns_rdataset_disassociate(&rdataset);
	}
	if (node != NULL) {
		dns_db_detachnode(vctx->db, &node);
	}

	return (result);
}

static isc_result_t
verifynsec3(const vctx_t *vctx, const dns_name_t *name,
	    const dns_rdata_t *rdata, bool delegation, bool empty,
	    const unsigned char types[8192], unsigned int maxtype,
	    isc_result_t *vresult) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char hashbuf[DNS_NAME_FORMATSIZE];
	dns_rdataset_t rdataset;
	dns_rdata_nsec3param_t nsec3param;
	dns_fixedname_t fixed;
	dns_name_t *hashname;
	isc_result_t result, tvresult = ISC_R_UNSET;
	dns_dbnode_t *node = NULL;
	unsigned char rawhash[NSEC3_MAX_HASH_LENGTH];
	size_t rhsize = sizeof(rawhash);
	bool optout = false;

	result = dns_rdata_tostruct(rdata, &nsec3param, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	if (nsec3param.flags != 0) {
		return (ISC_R_SUCCESS);
	}

	if (!dns_nsec3_supportedhash(nsec3param.hash)) {
		return (ISC_R_SUCCESS);
	}

	if (nsec3param.iterations > DNS_NSEC3_MAXITERATIONS) {
		result = DNS_R_NSEC3ITERRANGE;
		zoneverify_log_error(vctx, "verifynsec3: %s",
				     isc_result_totext(result));
		return (result);
	}

	result = isoptout(vctx, &nsec3param, &optout);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	dns_fixedname_init(&fixed);
	result = dns_nsec3_hashname(
		&fixed, rawhash, &rhsize, name, vctx->origin, nsec3param.hash,
		nsec3param.iterations, nsec3param.salt, nsec3param.salt_length);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "dns_nsec3_hashname(): %s",
				     isc_result_totext(result));
		return (result);
	}

	/*
	 * We don't use dns_db_find() here as it works with the chosen
	 * nsec3 chain and we may also be called with uncommitted data
	 * from dnssec-signzone so the secure status of the zone may not
	 * be up to date.
	 */
	dns_rdataset_init(&rdataset);
	hashname = dns_fixedname_name(&fixed);
	result = dns_db_findnsec3node(vctx->db, hashname, false, &node);
	if (result == ISC_R_SUCCESS) {
		result = dns_db_findrdataset(vctx->db, node, vctx->ver,
					     dns_rdatatype_nsec3, 0, 0,
					     &rdataset, NULL);
	}
	if (result != ISC_R_SUCCESS &&
	    (!delegation || (empty && !optout) ||
	     (!empty && dns_nsec_isset(types, dns_rdatatype_ds))))
	{
		dns_name_format(name, namebuf, sizeof(namebuf));
		dns_name_format(hashname, hashbuf, sizeof(hashbuf));
		zoneverify_log_error(vctx, "Missing NSEC3 record for %s (%s)",
				     namebuf, hashbuf);
	} else if (result == ISC_R_NOTFOUND && delegation && (!empty || optout))
	{
		result = ISC_R_SUCCESS;
	} else if (result == ISC_R_SUCCESS) {
		result = match_nsec3(vctx, name, &nsec3param, &rdataset, types,
				     maxtype, rawhash, rhsize, &tvresult);
		if (result != ISC_R_SUCCESS) {
			goto done;
		}
		result = tvresult;
	}

	*vresult = result;
	result = ISC_R_SUCCESS;

done:
	if (dns_rdataset_isassociated(&rdataset)) {
		dns_rdataset_disassociate(&rdataset);
	}
	if (node != NULL) {
		dns_db_detachnode(vctx->db, &node);
	}

	return (result);
}

static isc_result_t
verifynsec3s(const vctx_t *vctx, const dns_name_t *name,
	     dns_rdataset_t *nsec3paramset, bool delegation, bool empty,
	     const unsigned char types[8192], unsigned int maxtype,
	     isc_result_t *vresult) {
	isc_result_t result;

	for (result = dns_rdataset_first(nsec3paramset);
	     result == ISC_R_SUCCESS; result = dns_rdataset_next(nsec3paramset))
	{
		dns_rdata_t rdata = DNS_RDATA_INIT;

		dns_rdataset_current(nsec3paramset, &rdata);
		result = verifynsec3(vctx, name, &rdata, delegation, empty,
				     types, maxtype, vresult);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		if (*vresult != ISC_R_SUCCESS) {
			break;
		}
	}
	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
	}
	return (result);
}

static isc_result_t
verifyset(vctx_t *vctx, dns_rdataset_t *rdataset, const dns_name_t *name,
	  dns_dbnode_t *node, dst_key_t **dstkeys, size_t nkeys) {
	unsigned char set_algorithms[256] = { 0 };
	char namebuf[DNS_NAME_FORMATSIZE];
	char algbuf[DNS_SECALG_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	dns_rdataset_t sigrdataset;
	dns_rdatasetiter_t *rdsiter = NULL;
	isc_result_t result;

	dns_rdataset_init(&sigrdataset);
	result = dns_db_allrdatasets(vctx->db, node, vctx->ver, 0, 0, &rdsiter);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "dns_db_allrdatasets(): %s",
				     isc_result_totext(result));
		return (result);
	}
	for (result = dns_rdatasetiter_first(rdsiter); result == ISC_R_SUCCESS;
	     result = dns_rdatasetiter_next(rdsiter))
	{
		dns_rdatasetiter_current(rdsiter, &sigrdataset);
		if (sigrdataset.type == dns_rdatatype_rrsig &&
		    sigrdataset.covers == rdataset->type)
		{
			break;
		}
		dns_rdataset_disassociate(&sigrdataset);
	}
	if (result != ISC_R_SUCCESS) {
		dns_name_format(name, namebuf, sizeof(namebuf));
		dns_rdatatype_format(rdataset->type, typebuf, sizeof(typebuf));
		zoneverify_log_error(vctx, "No signatures for %s/%s", namebuf,
				     typebuf);
		for (size_t i = 0; i < ARRAY_SIZE(set_algorithms); i++) {
			if (vctx->act_algorithms[i] != 0) {
				vctx->bad_algorithms[i] = 1;
			}
		}
		result = ISC_R_SUCCESS;
		goto done;
	}

	for (result = dns_rdataset_first(&sigrdataset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(&sigrdataset))
	{
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdata_rrsig_t sig;

		dns_rdataset_current(&sigrdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &sig, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		if (rdataset->ttl != sig.originalttl) {
			dns_name_format(name, namebuf, sizeof(namebuf));
			dns_rdatatype_format(rdataset->type, typebuf,
					     sizeof(typebuf));
			zoneverify_log_error(vctx,
					     "TTL mismatch for "
					     "%s %s keytag %u",
					     namebuf, typebuf, sig.keyid);
			continue;
		}
		if ((set_algorithms[sig.algorithm] != 0) ||
		    (vctx->act_algorithms[sig.algorithm] == 0))
		{
			continue;
		}
		if (goodsig(vctx, &rdata, name, dstkeys, nkeys, rdataset)) {
			dns_rdataset_settrust(rdataset, dns_trust_secure);
			dns_rdataset_settrust(&sigrdataset, dns_trust_secure);
			set_algorithms[sig.algorithm] = 1;
		}
	}
	result = ISC_R_SUCCESS;

	if (memcmp(set_algorithms, vctx->act_algorithms,
		   sizeof(set_algorithms)) != 0)
	{
		dns_name_format(name, namebuf, sizeof(namebuf));
		dns_rdatatype_format(rdataset->type, typebuf, sizeof(typebuf));
		for (size_t i = 0; i < ARRAY_SIZE(set_algorithms); i++) {
			if ((vctx->act_algorithms[i] != 0) &&
			    (set_algorithms[i] == 0))
			{
				dns_secalg_format(i, algbuf, sizeof(algbuf));
				zoneverify_log_error(vctx,
						     "No correct %s signature "
						     "for %s %s",
						     algbuf, namebuf, typebuf);
				vctx->bad_algorithms[i] = 1;
			}
		}
	}

done:
	if (dns_rdataset_isassociated(&sigrdataset)) {
		dns_rdataset_disassociate(&sigrdataset);
	}
	dns_rdatasetiter_destroy(&rdsiter);

	return (result);
}

static isc_result_t
verifynode(vctx_t *vctx, const dns_name_t *name, dns_dbnode_t *node,
	   bool delegation, dst_key_t **dstkeys, size_t nkeys,
	   dns_rdataset_t *nsecset, dns_rdataset_t *nsec3paramset,
	   const dns_name_t *nextname, isc_result_t *vresult) {
	unsigned char types[8192] = { 0 };
	unsigned int maxtype = 0;
	dns_rdataset_t rdataset;
	dns_rdatasetiter_t *rdsiter = NULL;
	isc_result_t result, tvresult = ISC_R_UNSET;

	REQUIRE(vresult != NULL || (nsecset == NULL && nsec3paramset == NULL));

	result = dns_db_allrdatasets(vctx->db, node, vctx->ver, 0, 0, &rdsiter);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "dns_db_allrdatasets(): %s",
				     isc_result_totext(result));
		return (result);
	}

	result = dns_rdatasetiter_first(rdsiter);
	dns_rdataset_init(&rdataset);
	while (result == ISC_R_SUCCESS) {
		dns_rdatasetiter_current(rdsiter, &rdataset);
		/*
		 * If we are not at a delegation then everything should be
		 * signed.  If we are at a delegation then only the DS set
		 * is signed.  The NS set is not signed at a delegation but
		 * its existence is recorded in the bit map.  Anything else
		 * other than NSEC and DS is not signed at a delegation.
		 */
		if (rdataset.type != dns_rdatatype_rrsig &&
		    rdataset.type != dns_rdatatype_dnskey &&
		    (!delegation || rdataset.type == dns_rdatatype_ds ||
		     rdataset.type == dns_rdatatype_nsec))
		{
			result = verifyset(vctx, &rdataset, name, node, dstkeys,
					   nkeys);
			if (result != ISC_R_SUCCESS) {
				dns_rdataset_disassociate(&rdataset);
				dns_rdatasetiter_destroy(&rdsiter);
				return (result);
			}
			dns_nsec_setbit(types, rdataset.type, 1);
			if (rdataset.type > maxtype) {
				maxtype = rdataset.type;
			}
		} else if (rdataset.type != dns_rdatatype_rrsig &&
			   rdataset.type != dns_rdatatype_dnskey)
		{
			if (rdataset.type == dns_rdatatype_ns) {
				dns_nsec_setbit(types, rdataset.type, 1);
			}
			result = check_no_rrsig(vctx, &rdataset, name, node);
			if (result != ISC_R_SUCCESS) {
				dns_rdataset_disassociate(&rdataset);
				dns_rdatasetiter_destroy(&rdsiter);
				return (result);
			}
		} else {
			dns_nsec_setbit(types, rdataset.type, 1);
		}
		dns_rdataset_disassociate(&rdataset);
		result = dns_rdatasetiter_next(rdsiter);
	}
	dns_rdatasetiter_destroy(&rdsiter);
	if (result != ISC_R_NOMORE) {
		zoneverify_log_error(vctx, "rdataset iteration failed: %s",
				     isc_result_totext(result));
		return (result);
	}

	if (vresult == NULL) {
		return (ISC_R_SUCCESS);
	}

	*vresult = ISC_R_SUCCESS;

	if (nsecset != NULL && dns_rdataset_isassociated(nsecset)) {
		result = verifynsec(vctx, name, node, nextname, &tvresult);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		*vresult = tvresult;
	}

	if (nsec3paramset != NULL && dns_rdataset_isassociated(nsec3paramset)) {
		result = verifynsec3s(vctx, name, nsec3paramset, delegation,
				      false, types, maxtype, &tvresult);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		if (*vresult == ISC_R_SUCCESS) {
			*vresult = tvresult;
		}
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
is_empty(const vctx_t *vctx, dns_dbnode_t *node, bool *empty) {
	dns_rdatasetiter_t *rdsiter = NULL;
	isc_result_t result;

	result = dns_db_allrdatasets(vctx->db, node, vctx->ver, 0, 0, &rdsiter);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "dns_db_allrdatasets(): %s",
				     isc_result_totext(result));
		return (result);
	}
	result = dns_rdatasetiter_first(rdsiter);
	dns_rdatasetiter_destroy(&rdsiter);

	*empty = (result == ISC_R_NOMORE);

	return (ISC_R_SUCCESS);
}

static isc_result_t
check_no_nsec(const vctx_t *vctx, const dns_name_t *name, dns_dbnode_t *node) {
	bool nsec_exists = false;
	dns_rdataset_t rdataset;
	isc_result_t result;

	dns_rdataset_init(&rdataset);
	result = dns_db_findrdataset(vctx->db, node, vctx->ver,
				     dns_rdatatype_nsec, 0, 0, &rdataset, NULL);
	if (result != ISC_R_NOTFOUND) {
		char namebuf[DNS_NAME_FORMATSIZE];
		dns_name_format(name, namebuf, sizeof(namebuf));
		zoneverify_log_error(vctx, "unexpected NSEC RRset at %s",
				     namebuf);
		nsec_exists = true;
	}

	if (dns_rdataset_isassociated(&rdataset)) {
		dns_rdataset_disassociate(&rdataset);
	}

	return (nsec_exists ? ISC_R_FAILURE : ISC_R_SUCCESS);
}

static void
free_element(isc_mem_t *mctx, struct nsec3_chain_fixed *e) {
	size_t len;

	len = sizeof(*e) + e->salt_length + 2 * e->next_length;
	isc_mem_put(mctx, e, len);
}

static void
free_element_heap(void *element, void *uap) {
	struct nsec3_chain_fixed *e = (struct nsec3_chain_fixed *)element;
	isc_mem_t *mctx = (isc_mem_t *)uap;

	free_element(mctx, e);
}

static bool
_checknext(const vctx_t *vctx, const struct nsec3_chain_fixed *first,
	   const struct nsec3_chain_fixed *e) {
	char buf[512];
	const unsigned char *d1 = (const unsigned char *)(first + 1);
	const unsigned char *d2 = (const unsigned char *)(e + 1);
	isc_buffer_t b;
	isc_region_t sr;

	d1 += first->salt_length + first->next_length;
	d2 += e->salt_length;

	if (memcmp(d1, d2, first->next_length) == 0) {
		return (true);
	}

	DE_CONST(d1 - first->next_length, sr.base);
	sr.length = first->next_length;
	isc_buffer_init(&b, buf, sizeof(buf));
	isc_base32hex_totext(&sr, 1, "", &b);
	zoneverify_log_error(vctx, "Break in NSEC3 chain at: %.*s",
			     (int)isc_buffer_usedlength(&b), buf);

	DE_CONST(d1, sr.base);
	sr.length = first->next_length;
	isc_buffer_init(&b, buf, sizeof(buf));
	isc_base32hex_totext(&sr, 1, "", &b);
	zoneverify_log_error(vctx, "Expected: %.*s",
			     (int)isc_buffer_usedlength(&b), buf);

	DE_CONST(d2, sr.base);
	sr.length = first->next_length;
	isc_buffer_init(&b, buf, sizeof(buf));
	isc_base32hex_totext(&sr, 1, "", &b);
	zoneverify_log_error(vctx, "Found: %.*s",
			     (int)isc_buffer_usedlength(&b), buf);

	return (false);
}

static bool
checknext(isc_mem_t *mctx, const vctx_t *vctx,
	  const struct nsec3_chain_fixed *first, struct nsec3_chain_fixed *prev,
	  const struct nsec3_chain_fixed *cur) {
	bool result = _checknext(vctx, prev, cur);

	if (prev != first) {
		free_element(mctx, prev);
	}

	return (result);
}

static bool
checklast(isc_mem_t *mctx, const vctx_t *vctx, struct nsec3_chain_fixed *first,
	  struct nsec3_chain_fixed *prev) {
	bool result = _checknext(vctx, prev, first);
	if (prev != first) {
		free_element(mctx, prev);
	}
	free_element(mctx, first);

	return (result);
}

static isc_result_t
verify_nsec3_chains(const vctx_t *vctx, isc_mem_t *mctx) {
	isc_result_t result = ISC_R_SUCCESS;
	struct nsec3_chain_fixed *e, *f = NULL;
	struct nsec3_chain_fixed *first = NULL, *prev = NULL;

	while ((e = isc_heap_element(vctx->expected_chains, 1)) != NULL) {
		isc_heap_delete(vctx->expected_chains, 1);
		if (f == NULL) {
			f = isc_heap_element(vctx->found_chains, 1);
		}
		if (f != NULL) {
			isc_heap_delete(vctx->found_chains, 1);

			/*
			 * Check that they match.
			 */
			if (chain_equal(e, f, chain_length(e))) {
				free_element(mctx, f);
				f = NULL;
			} else {
				if (result == ISC_R_SUCCESS) {
					zoneverify_log_error(vctx, "Expected "
								   "and found "
								   "NSEC3 "
								   "chains not "
								   "equal");
				}
				result = ISC_R_FAILURE;
				/*
				 * Attempt to resync found_chain.
				 */
				while (f != NULL && !chain_compare(e, f)) {
					free_element(mctx, f);
					f = isc_heap_element(vctx->found_chains,
							     1);
					if (f != NULL) {
						isc_heap_delete(
							vctx->found_chains, 1);
					}
					if (f != NULL &&
					    chain_equal(e, f, chain_length(e)))
					{
						free_element(mctx, f);
						f = NULL;
						break;
					}
				}
			}
		} else if (result == ISC_R_SUCCESS) {
			zoneverify_log_error(vctx, "Expected and found NSEC3 "
						   "chains "
						   "not equal");
			result = ISC_R_FAILURE;
		}

		if (first == NULL) {
			prev = first = e;
		} else if (!chain_equal(first, e, first->salt_length)) {
			if (!checklast(mctx, vctx, first, prev)) {
				result = ISC_R_FAILURE;
			}

			prev = first = e;
		} else {
			if (!checknext(mctx, vctx, first, prev, e)) {
				result = ISC_R_FAILURE;
			}

			prev = e;
		}
	}
	if (prev != NULL) {
		if (!checklast(mctx, vctx, first, prev)) {
			result = ISC_R_FAILURE;
		}
	}
	do {
		if (f != NULL) {
			if (result == ISC_R_SUCCESS) {
				zoneverify_log_error(vctx, "Expected and found "
							   "NSEC3 chains not "
							   "equal");
				result = ISC_R_FAILURE;
			}
			free_element(mctx, f);
		}
		f = isc_heap_element(vctx->found_chains, 1);
		if (f != NULL) {
			isc_heap_delete(vctx->found_chains, 1);
		}
	} while (f != NULL);

	return (result);
}

static isc_result_t
verifyemptynodes(const vctx_t *vctx, const dns_name_t *name,
		 const dns_name_t *prevname, bool isdelegation,
		 dns_rdataset_t *nsec3paramset, isc_result_t *vresult) {
	dns_namereln_t reln;
	int order;
	unsigned int labels, nlabels, i;
	dns_name_t suffix;
	isc_result_t result, tvresult = ISC_R_UNSET;

	*vresult = ISC_R_SUCCESS;

	reln = dns_name_fullcompare(prevname, name, &order, &labels);
	if (order >= 0) {
		return (ISC_R_SUCCESS);
	}

	nlabels = dns_name_countlabels(name);

	if (reln == dns_namereln_commonancestor ||
	    reln == dns_namereln_contains)
	{
		dns_name_init(&suffix, NULL);
		for (i = labels + 1; i < nlabels; i++) {
			dns_name_getlabelsequence(name, nlabels - i, i,
						  &suffix);
			if (nsec3paramset != NULL &&
			    dns_rdataset_isassociated(nsec3paramset))
			{
				result = verifynsec3s(
					vctx, &suffix, nsec3paramset,
					isdelegation, true, NULL, 0, &tvresult);
				if (result != ISC_R_SUCCESS) {
					return (result);
				}
				if (*vresult == ISC_R_SUCCESS) {
					*vresult = tvresult;
				}
			}
		}
	}

	return (ISC_R_SUCCESS);
}

static void
vctx_init(vctx_t *vctx, isc_mem_t *mctx, dns_zone_t *zone, dns_db_t *db,
	  dns_dbversion_t *ver, dns_name_t *origin, dns_keytable_t *secroots) {
	memset(vctx, 0, sizeof(*vctx));

	vctx->mctx = mctx;
	vctx->zone = zone;
	vctx->db = db;
	vctx->ver = ver;
	vctx->origin = origin;
	vctx->secroots = secroots;
	vctx->goodksk = false;
	vctx->goodzsk = false;

	dns_rdataset_init(&vctx->keyset);
	dns_rdataset_init(&vctx->keysigs);
	dns_rdataset_init(&vctx->soaset);
	dns_rdataset_init(&vctx->soasigs);
	dns_rdataset_init(&vctx->nsecset);
	dns_rdataset_init(&vctx->nsecsigs);
	dns_rdataset_init(&vctx->nsec3paramset);
	dns_rdataset_init(&vctx->nsec3paramsigs);

	vctx->expected_chains = NULL;
	isc_heap_create(mctx, chain_compare, NULL, 1024,
			&vctx->expected_chains);

	vctx->found_chains = NULL;
	isc_heap_create(mctx, chain_compare, NULL, 1024, &vctx->found_chains);
}

static void
vctx_destroy(vctx_t *vctx) {
	if (dns_rdataset_isassociated(&vctx->keyset)) {
		dns_rdataset_disassociate(&vctx->keyset);
	}
	if (dns_rdataset_isassociated(&vctx->keysigs)) {
		dns_rdataset_disassociate(&vctx->keysigs);
	}
	if (dns_rdataset_isassociated(&vctx->soaset)) {
		dns_rdataset_disassociate(&vctx->soaset);
	}
	if (dns_rdataset_isassociated(&vctx->soasigs)) {
		dns_rdataset_disassociate(&vctx->soasigs);
	}
	if (dns_rdataset_isassociated(&vctx->nsecset)) {
		dns_rdataset_disassociate(&vctx->nsecset);
	}
	if (dns_rdataset_isassociated(&vctx->nsecsigs)) {
		dns_rdataset_disassociate(&vctx->nsecsigs);
	}
	if (dns_rdataset_isassociated(&vctx->nsec3paramset)) {
		dns_rdataset_disassociate(&vctx->nsec3paramset);
	}
	if (dns_rdataset_isassociated(&vctx->nsec3paramsigs)) {
		dns_rdataset_disassociate(&vctx->nsec3paramsigs);
	}
	isc_heap_foreach(vctx->expected_chains, free_element_heap, vctx->mctx);
	isc_heap_destroy(&vctx->expected_chains);
	isc_heap_foreach(vctx->found_chains, free_element_heap, vctx->mctx);
	isc_heap_destroy(&vctx->found_chains);
}

static isc_result_t
check_apex_rrsets(vctx_t *vctx) {
	dns_dbnode_t *node = NULL;
	isc_result_t result;

	result = dns_db_findnode(vctx->db, vctx->origin, false, &node);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx,
				     "failed to find the zone's origin: %s",
				     isc_result_totext(result));
		return (result);
	}

	result = dns_db_findrdataset(vctx->db, node, vctx->ver,
				     dns_rdatatype_dnskey, 0, 0, &vctx->keyset,
				     &vctx->keysigs);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "Zone contains no DNSSEC keys");
		goto done;
	}

	result = dns_db_findrdataset(vctx->db, node, vctx->ver,
				     dns_rdatatype_soa, 0, 0, &vctx->soaset,
				     &vctx->soasigs);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "Zone contains no SOA record");
		goto done;
	}

	result = dns_db_findrdataset(vctx->db, node, vctx->ver,
				     dns_rdatatype_nsec, 0, 0, &vctx->nsecset,
				     &vctx->nsecsigs);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTFOUND) {
		zoneverify_log_error(vctx, "NSEC lookup failed");
		goto done;
	}

	result = dns_db_findrdataset(
		vctx->db, node, vctx->ver, dns_rdatatype_nsec3param, 0, 0,
		&vctx->nsec3paramset, &vctx->nsec3paramsigs);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTFOUND) {
		zoneverify_log_error(vctx, "NSEC3PARAM lookup failed");
		goto done;
	}

	if (!dns_rdataset_isassociated(&vctx->keysigs)) {
		zoneverify_log_error(vctx, "DNSKEY is not signed "
					   "(keys offline or inactive?)");
		result = ISC_R_FAILURE;
		goto done;
	}

	if (!dns_rdataset_isassociated(&vctx->soasigs)) {
		zoneverify_log_error(vctx, "SOA is not signed "
					   "(keys offline or inactive?)");
		result = ISC_R_FAILURE;
		goto done;
	}

	if (dns_rdataset_isassociated(&vctx->nsecset) &&
	    !dns_rdataset_isassociated(&vctx->nsecsigs))
	{
		zoneverify_log_error(vctx, "NSEC is not signed "
					   "(keys offline or inactive?)");
		result = ISC_R_FAILURE;
		goto done;
	}

	if (dns_rdataset_isassociated(&vctx->nsec3paramset) &&
	    !dns_rdataset_isassociated(&vctx->nsec3paramsigs))
	{
		zoneverify_log_error(vctx, "NSEC3PARAM is not signed "
					   "(keys offline or inactive?)");
		result = ISC_R_FAILURE;
		goto done;
	}

	if (!dns_rdataset_isassociated(&vctx->nsecset) &&
	    !dns_rdataset_isassociated(&vctx->nsec3paramset))
	{
		zoneverify_log_error(vctx, "No valid NSEC/NSEC3 chain for "
					   "testing");
		result = ISC_R_FAILURE;
		goto done;
	}

	result = ISC_R_SUCCESS;

done:
	dns_db_detachnode(vctx->db, &node);

	return (result);
}

/*%
 * Update 'vctx' tables tracking active and standby key algorithms used in the
 * verified zone based on the signatures made using 'dnskey' (prepared from
 * 'rdata') found at zone apex.  Set 'vctx->goodksk' or 'vctx->goodzsk' to true
 * if 'dnskey' correctly signs the DNSKEY RRset at zone apex and either
 * 'vctx->secroots' is NULL or 'dnskey' is present in 'vctx->secroots'.
 *
 * The variables to update are chosen based on 'is_ksk', which is true when
 * 'dnskey' is a KSK and false otherwise.
 */
static void
check_dnskey_sigs(vctx_t *vctx, const dns_rdata_dnskey_t *dnskey,
		  dns_rdata_t *keyrdata, bool is_ksk) {
	unsigned char *active_keys = NULL, *standby_keys = NULL;
	dns_keynode_t *keynode = NULL;
	bool *goodkey = NULL;
	dst_key_t *key = NULL;
	isc_result_t result;
	dns_rdataset_t dsset;

	active_keys = (is_ksk ? vctx->ksk_algorithms : vctx->zsk_algorithms);
	standby_keys = (is_ksk ? vctx->standby_ksk : vctx->standby_zsk);
	goodkey = (is_ksk ? &vctx->goodksk : &vctx->goodzsk);

	/*
	 * First, does this key sign the DNSKEY rrset?
	 */
	if (!dns_dnssec_selfsigns(keyrdata, vctx->origin, &vctx->keyset,
				  &vctx->keysigs, false, vctx->mctx))
	{
		if (!is_ksk &&
		    dns_dnssec_signs(keyrdata, vctx->origin, &vctx->soaset,
				     &vctx->soasigs, false, vctx->mctx))
		{
			if (active_keys[dnskey->algorithm] != DNS_KEYALG_MAX) {
				active_keys[dnskey->algorithm]++;
			}
		} else {
			if (standby_keys[dnskey->algorithm] != DNS_KEYALG_MAX) {
				standby_keys[dnskey->algorithm]++;
			}
		}
		return;
	}

	if (active_keys[dnskey->algorithm] != DNS_KEYALG_MAX) {
		active_keys[dnskey->algorithm]++;
	}

	/*
	 * If a trust anchor table was not supplied, a correctly self-signed
	 * DNSKEY RRset is good enough.
	 */
	if (vctx->secroots == NULL) {
		*goodkey = true;
		return;
	}

	/*
	 * Convert the supplied key rdata to dst_key_t. (If this
	 * fails we can't go further.)
	 */
	result = dns_dnssec_keyfromrdata(vctx->origin, keyrdata, vctx->mctx,
					 &key);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	/*
	 * Look up the supplied key in the trust anchor table.
	 * If we don't find an exact match, or if the keynode data
	 * is NULL, then we have neither a DNSKEY nor a DS format
	 * trust anchor, and can give up.
	 */
	result = dns_keytable_find(vctx->secroots, vctx->origin, &keynode);
	if (result != ISC_R_SUCCESS) {
		/* No such trust anchor */
		goto cleanup;
	}

	/*
	 * If the keynode has any DS format trust anchors, that means
	 * it doesn't have any DNSKEY ones. So, we can check for a DS
	 * match and then stop.
	 */
	dns_rdataset_init(&dsset);
	if (dns_keynode_dsset(keynode, &dsset)) {
		for (result = dns_rdataset_first(&dsset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(&dsset))
		{
			dns_rdata_t dsrdata = DNS_RDATA_INIT;
			dns_rdata_t newdsrdata = DNS_RDATA_INIT;
			unsigned char buf[DNS_DS_BUFFERSIZE];
			dns_rdata_ds_t ds;

			dns_rdata_reset(&dsrdata);
			dns_rdataset_current(&dsset, &dsrdata);
			result = dns_rdata_tostruct(&dsrdata, &ds, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);

			if (ds.key_tag != dst_key_id(key) ||
			    ds.algorithm != dst_key_alg(key))
			{
				continue;
			}

			result = dns_ds_buildrdata(vctx->origin, keyrdata,
						   ds.digest_type, buf,
						   &newdsrdata);
			if (result != ISC_R_SUCCESS) {
				continue;
			}

			if (dns_rdata_compare(&dsrdata, &newdsrdata) == 0) {
				dns_rdataset_settrust(&vctx->keyset,
						      dns_trust_secure);
				dns_rdataset_settrust(&vctx->keysigs,
						      dns_trust_secure);
				*goodkey = true;
				break;
			}
		}
		dns_rdataset_disassociate(&dsset);

		goto cleanup;
	}

cleanup:
	if (keynode != NULL) {
		dns_keytable_detachkeynode(vctx->secroots, &keynode);
	}
	if (key != NULL) {
		dst_key_free(&key);
	}
}

/*%
 * Check that the DNSKEY RR has at least one self signing KSK and one ZSK per
 * algorithm in it (or, if -x was used, one self-signing KSK).
 */
static isc_result_t
check_dnskey(vctx_t *vctx) {
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_dnskey_t dnskey;
	isc_result_t result;
	bool is_ksk;

	for (result = dns_rdataset_first(&vctx->keyset);
	     result == ISC_R_SUCCESS; result = dns_rdataset_next(&vctx->keyset))
	{
		dns_rdataset_current(&vctx->keyset, &rdata);
		result = dns_rdata_tostruct(&rdata, &dnskey, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		is_ksk = ((dnskey.flags & DNS_KEYFLAG_KSK) != 0);

		if ((dnskey.flags & DNS_KEYOWNER_ZONE) != 0 &&
		    (dnskey.flags & DNS_KEYFLAG_REVOKE) != 0)
		{
			if ((dnskey.flags & DNS_KEYFLAG_KSK) != 0 &&
			    !dns_dnssec_selfsigns(&rdata, vctx->origin,
						  &vctx->keyset, &vctx->keysigs,
						  false, vctx->mctx))
			{
				char namebuf[DNS_NAME_FORMATSIZE];
				char buffer[1024];
				isc_buffer_t buf;

				dns_name_format(vctx->origin, namebuf,
						sizeof(namebuf));
				isc_buffer_init(&buf, buffer, sizeof(buffer));
				result = dns_rdata_totext(&rdata, NULL, &buf);
				if (result != ISC_R_SUCCESS) {
					zoneverify_log_error(
						vctx, "dns_rdata_totext: %s",
						isc_result_totext(result));
					return (ISC_R_FAILURE);
				}
				zoneverify_log_error(
					vctx,
					"revoked KSK is not self signed:\n"
					"%s DNSKEY %.*s",
					namebuf,
					(int)isc_buffer_usedlength(&buf),
					buffer);
				return (ISC_R_FAILURE);
			}
			if ((dnskey.flags & DNS_KEYFLAG_KSK) != 0 &&
			    vctx->revoked_ksk[dnskey.algorithm] !=
				    DNS_KEYALG_MAX)
			{
				vctx->revoked_ksk[dnskey.algorithm]++;
			} else if ((dnskey.flags & DNS_KEYFLAG_KSK) == 0 &&
				   vctx->revoked_zsk[dnskey.algorithm] !=
					   DNS_KEYALG_MAX)
			{
				vctx->revoked_zsk[dnskey.algorithm]++;
			}
		} else {
			check_dnskey_sigs(vctx, &dnskey, &rdata, is_ksk);
		}
		dns_rdata_freestruct(&dnskey);
		dns_rdata_reset(&rdata);
	}

	return (ISC_R_SUCCESS);
}

static void
determine_active_algorithms(vctx_t *vctx, bool ignore_kskflag,
			    bool keyset_kskonly,
			    void (*report)(const char *, ...)) {
	char algbuf[DNS_SECALG_FORMATSIZE];

	report("Verifying the zone using the following algorithms:");

	for (size_t i = 0; i < ARRAY_SIZE(vctx->act_algorithms); i++) {
		if (ignore_kskflag) {
			vctx->act_algorithms[i] = (vctx->ksk_algorithms[i] !=
							   0 ||
						   vctx->zsk_algorithms[i] != 0)
							  ? 1
							  : 0;
		} else {
			vctx->act_algorithms[i] = vctx->ksk_algorithms[i] != 0
							  ? 1
							  : 0;
		}
		if (vctx->act_algorithms[i] != 0) {
			dns_secalg_format(i, algbuf, sizeof(algbuf));
			report("- %s", algbuf);
		}
	}

	if (ignore_kskflag || keyset_kskonly) {
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(vctx->ksk_algorithms); i++) {
		/*
		 * The counts should both be zero or both be non-zero.  Mark
		 * the algorithm as bad if this is not met.
		 */
		if ((vctx->ksk_algorithms[i] != 0) ==
		    (vctx->zsk_algorithms[i] != 0))
		{
			continue;
		}
		dns_secalg_format(i, algbuf, sizeof(algbuf));
		zoneverify_log_error(vctx, "Missing %s for algorithm %s",
				     (vctx->ksk_algorithms[i] != 0) ? "ZSK"
								    : "self-"
								      "signed "
								      "KSK",
				     algbuf);
		vctx->bad_algorithms[i] = 1;
	}
}

/*%
 * Check that all the records not yet verified were signed by keys that are
 * present in the DNSKEY RRset.
 */
static isc_result_t
verify_nodes(vctx_t *vctx, isc_result_t *vresult) {
	dns_fixedname_t fname, fnextname, fprevname, fzonecut;
	dns_name_t *name, *nextname, *prevname, *zonecut;
	dns_dbnode_t *node = NULL, *nextnode;
	dns_dbiterator_t *dbiter = NULL;
	dst_key_t **dstkeys;
	size_t count, nkeys = 0;
	bool done = false;
	isc_result_t tvresult = ISC_R_UNSET;
	isc_result_t result;

	name = dns_fixedname_initname(&fname);
	nextname = dns_fixedname_initname(&fnextname);
	dns_fixedname_init(&fprevname);
	prevname = NULL;
	dns_fixedname_init(&fzonecut);
	zonecut = NULL;

	count = dns_rdataset_count(&vctx->keyset);
	dstkeys = isc_mem_get(vctx->mctx, sizeof(*dstkeys) * count);

	for (result = dns_rdataset_first(&vctx->keyset);
	     result == ISC_R_SUCCESS; result = dns_rdataset_next(&vctx->keyset))
	{
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdataset_current(&vctx->keyset, &rdata);
		dstkeys[nkeys] = NULL;
		result = dns_dnssec_keyfromrdata(vctx->origin, &rdata,
						 vctx->mctx, &dstkeys[nkeys]);
		if (result == ISC_R_SUCCESS) {
			nkeys++;
		}
	}

	result = dns_db_createiterator(vctx->db, DNS_DB_NONSEC3, &dbiter);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "dns_db_createiterator(): %s",
				     isc_result_totext(result));
		goto done;
	}

	result = dns_dbiterator_first(dbiter);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "dns_dbiterator_first(): %s",
				     isc_result_totext(result));
		goto done;
	}

	while (!done) {
		bool isdelegation = false;

		result = dns_dbiterator_current(dbiter, &node, name);
		if (result != ISC_R_SUCCESS && result != DNS_R_NEWORIGIN) {
			zoneverify_log_error(vctx,
					     "dns_dbiterator_current(): %s",
					     isc_result_totext(result));
			goto done;
		}
		if (!dns_name_issubdomain(name, vctx->origin)) {
			result = check_no_nsec(vctx, name, node);
			if (result != ISC_R_SUCCESS) {
				dns_db_detachnode(vctx->db, &node);
				goto done;
			}
			dns_db_detachnode(vctx->db, &node);
			result = dns_dbiterator_next(dbiter);
			if (result == ISC_R_NOMORE) {
				done = true;
			} else if (result != ISC_R_SUCCESS) {
				zoneverify_log_error(vctx,
						     "dns_dbiterator_next(): "
						     "%s",
						     isc_result_totext(result));
				goto done;
			}
			continue;
		}
		if (is_delegation(vctx, name, node, NULL)) {
			zonecut = dns_fixedname_name(&fzonecut);
			dns_name_copynf(name, zonecut);
			isdelegation = true;
		} else if (has_dname(vctx, node)) {
			zonecut = dns_fixedname_name(&fzonecut);
			dns_name_copynf(name, zonecut);
		}
		nextnode = NULL;
		result = dns_dbiterator_next(dbiter);
		while (result == ISC_R_SUCCESS) {
			bool empty;
			result = dns_dbiterator_current(dbiter, &nextnode,
							nextname);
			if (result != ISC_R_SUCCESS &&
			    result != DNS_R_NEWORIGIN)
			{
				zoneverify_log_error(vctx,
						     "dns_dbiterator_current():"
						     " %s",
						     isc_result_totext(result));
				dns_db_detachnode(vctx->db, &node);
				goto done;
			}
			if (!dns_name_issubdomain(nextname, vctx->origin) ||
			    (zonecut != NULL &&
			     dns_name_issubdomain(nextname, zonecut)))
			{
				result = check_no_nsec(vctx, nextname,
						       nextnode);
				if (result != ISC_R_SUCCESS) {
					dns_db_detachnode(vctx->db, &node);
					dns_db_detachnode(vctx->db, &nextnode);
					goto done;
				}
				dns_db_detachnode(vctx->db, &nextnode);
				result = dns_dbiterator_next(dbiter);
				continue;
			}
			result = is_empty(vctx, nextnode, &empty);
			dns_db_detachnode(vctx->db, &nextnode);
			if (result != ISC_R_SUCCESS) {
				dns_db_detachnode(vctx->db, &node);
				goto done;
			}
			if (empty) {
				result = dns_dbiterator_next(dbiter);
				continue;
			}
			break;
		}
		if (result == ISC_R_NOMORE) {
			done = true;
			nextname = vctx->origin;
		} else if (result != ISC_R_SUCCESS) {
			zoneverify_log_error(vctx,
					     "iterating through the database "
					     "failed: %s",
					     isc_result_totext(result));
			dns_db_detachnode(vctx->db, &node);
			goto done;
		}
		result = verifynode(vctx, name, node, isdelegation, dstkeys,
				    nkeys, &vctx->nsecset, &vctx->nsec3paramset,
				    nextname, &tvresult);
		if (result != ISC_R_SUCCESS) {
			dns_db_detachnode(vctx->db, &node);
			goto done;
		}
		if (*vresult == ISC_R_UNSET) {
			*vresult = ISC_R_SUCCESS;
		}
		if (*vresult == ISC_R_SUCCESS) {
			*vresult = tvresult;
		}
		if (prevname != NULL) {
			result = verifyemptynodes(
				vctx, name, prevname, isdelegation,
				&vctx->nsec3paramset, &tvresult);
			if (result != ISC_R_SUCCESS) {
				dns_db_detachnode(vctx->db, &node);
				goto done;
			}
		} else {
			prevname = dns_fixedname_name(&fprevname);
		}
		dns_name_copynf(name, prevname);
		if (*vresult == ISC_R_SUCCESS) {
			*vresult = tvresult;
		}
		dns_db_detachnode(vctx->db, &node);
	}

	dns_dbiterator_destroy(&dbiter);

	result = dns_db_createiterator(vctx->db, DNS_DB_NSEC3ONLY, &dbiter);
	if (result != ISC_R_SUCCESS) {
		zoneverify_log_error(vctx, "dns_db_createiterator(): %s",
				     isc_result_totext(result));
		return (result);
	}

	for (result = dns_dbiterator_first(dbiter); result == ISC_R_SUCCESS;
	     result = dns_dbiterator_next(dbiter))
	{
		result = dns_dbiterator_current(dbiter, &node, name);
		if (result != ISC_R_SUCCESS && result != DNS_R_NEWORIGIN) {
			zoneverify_log_error(vctx,
					     "dns_dbiterator_current(): %s",
					     isc_result_totext(result));
			goto done;
		}
		result = verifynode(vctx, name, node, false, dstkeys, nkeys,
				    NULL, NULL, NULL, NULL);
		if (result != ISC_R_SUCCESS) {
			zoneverify_log_error(vctx, "verifynode: %s",
					     isc_result_totext(result));
			dns_db_detachnode(vctx->db, &node);
			goto done;
		}
		result = record_found(vctx, name, node, &vctx->nsec3paramset);
		dns_db_detachnode(vctx->db, &node);
		if (result != ISC_R_SUCCESS) {
			goto done;
		}
	}

	result = ISC_R_SUCCESS;

done:
	while (nkeys-- > 0U) {
		dst_key_free(&dstkeys[nkeys]);
	}
	isc_mem_put(vctx->mctx, dstkeys, sizeof(*dstkeys) * count);
	if (dbiter != NULL) {
		dns_dbiterator_destroy(&dbiter);
	}

	return (result);
}

static isc_result_t
check_bad_algorithms(const vctx_t *vctx, void (*report)(const char *, ...)) {
	char algbuf[DNS_SECALG_FORMATSIZE];
	bool first = true;

	for (size_t i = 0; i < ARRAY_SIZE(vctx->bad_algorithms); i++) {
		if (vctx->bad_algorithms[i] == 0) {
			continue;
		}
		if (first) {
			report("The zone is not fully signed "
			       "for the following algorithms:");
		}
		dns_secalg_format(i, algbuf, sizeof(algbuf));
		report(" %s", algbuf);
		first = false;
	}

	if (!first) {
		report(".");
	}

	return (first ? ISC_R_SUCCESS : ISC_R_FAILURE);
}

static void
print_summary(const vctx_t *vctx, bool keyset_kskonly,
	      void (*report)(const char *, ...)) {
	char algbuf[DNS_SECALG_FORMATSIZE];

	report("Zone fully signed:");
	for (size_t i = 0; i < ARRAY_SIZE(vctx->ksk_algorithms); i++) {
		if ((vctx->ksk_algorithms[i] == 0) &&
		    (vctx->standby_ksk[i] == 0) &&
		    (vctx->revoked_ksk[i] == 0) &&
		    (vctx->zsk_algorithms[i] == 0) &&
		    (vctx->standby_zsk[i] == 0) && (vctx->revoked_zsk[i] == 0))
		{
			continue;
		}
		dns_secalg_format(i, algbuf, sizeof(algbuf));
		report("Algorithm: %s: KSKs: "
		       "%u active, %u stand-by, %u revoked",
		       algbuf, vctx->ksk_algorithms[i], vctx->standby_ksk[i],
		       vctx->revoked_ksk[i]);
		report("%*sZSKs: "
		       "%u active, %u %s, %u revoked",
		       (int)strlen(algbuf) + 13, "", vctx->zsk_algorithms[i],
		       vctx->standby_zsk[i],
		       keyset_kskonly ? "present" : "stand-by",
		       vctx->revoked_zsk[i]);
	}
}

isc_result_t
dns_zoneverify_dnssec(dns_zone_t *zone, dns_db_t *db, dns_dbversion_t *ver,
		      dns_name_t *origin, dns_keytable_t *secroots,
		      isc_mem_t *mctx, bool ignore_kskflag, bool keyset_kskonly,
		      void (*report)(const char *, ...)) {
	const char *keydesc = (secroots == NULL ? "self-signed" : "trusted");
	isc_result_t result, vresult = ISC_R_UNSET;
	vctx_t vctx;

	vctx_init(&vctx, mctx, zone, db, ver, origin, secroots);

	result = check_apex_rrsets(&vctx);
	if (result != ISC_R_SUCCESS) {
		goto done;
	}

	result = check_dnskey(&vctx);
	if (result != ISC_R_SUCCESS) {
		goto done;
	}

	if (ignore_kskflag) {
		if (!vctx.goodksk && !vctx.goodzsk) {
			zoneverify_log_error(&vctx, "No %s DNSKEY found",
					     keydesc);
			result = ISC_R_FAILURE;
			goto done;
		}
	} else if (!vctx.goodksk) {
		zoneverify_log_error(&vctx, "No %s KSK DNSKEY found", keydesc);
		result = ISC_R_FAILURE;
		goto done;
	}

	determine_active_algorithms(&vctx, ignore_kskflag, keyset_kskonly,
				    report);

	result = verify_nodes(&vctx, &vresult);
	if (result != ISC_R_SUCCESS) {
		goto done;
	}

	result = verify_nsec3_chains(&vctx, mctx);
	if (vresult == ISC_R_UNSET) {
		vresult = ISC_R_SUCCESS;
	}
	if (result != ISC_R_SUCCESS && vresult == ISC_R_SUCCESS) {
		vresult = result;
	}

	result = check_bad_algorithms(&vctx, report);
	if (result != ISC_R_SUCCESS) {
		report("DNSSEC completeness test failed.");
		goto done;
	}

	result = vresult;
	if (result != ISC_R_SUCCESS) {
		report("DNSSEC completeness test failed (%s).",
		       dns_result_totext(result));
		goto done;
	}

	if (vctx.goodksk || ignore_kskflag) {
		print_summary(&vctx, keyset_kskonly, report);
	}

done:
	vctx_destroy(&vctx);

	return (result);
}
