/*	$NetBSD: ds.c,v 1.1.2.2 2024/02/24 13:06:57 martin Exp $	*/

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

#include <string.h>

#include <isc/buffer.h>
#include <isc/md.h>
#include <isc/region.h>
#include <isc/util.h>

#include <dns/ds.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdatastruct.h>
#include <dns/result.h>

#include <dst/dst.h>

isc_result_t
dns_ds_fromkeyrdata(const dns_name_t *owner, dns_rdata_t *key,
		    dns_dsdigest_t digest_type, unsigned char *digest,
		    dns_rdata_ds_t *dsrdata) {
	isc_result_t result;
	dns_fixedname_t fname;
	dns_name_t *name;
	unsigned int digestlen;
	isc_region_t r;
	isc_md_t *md;
	const isc_md_type_t *md_type = NULL;

	REQUIRE(key != NULL);
	REQUIRE(key->type == dns_rdatatype_dnskey ||
		key->type == dns_rdatatype_cdnskey);

	if (!dst_ds_digest_supported(digest_type)) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	switch (digest_type) {
	case DNS_DSDIGEST_SHA1:
		md_type = ISC_MD_SHA1;
		break;

	case DNS_DSDIGEST_SHA384:
		md_type = ISC_MD_SHA384;
		break;

	case DNS_DSDIGEST_SHA256:
		md_type = ISC_MD_SHA256;
		break;

	default:
		UNREACHABLE();
	}

	name = dns_fixedname_initname(&fname);
	(void)dns_name_downcase(owner, name, NULL);

	md = isc_md_new();
	if (md == NULL) {
		return (ISC_R_NOMEMORY);
	}

	result = isc_md_init(md, md_type);
	if (result != ISC_R_SUCCESS) {
		goto end;
	}

	dns_name_toregion(name, &r);

	result = isc_md_update(md, r.base, r.length);
	if (result != ISC_R_SUCCESS) {
		goto end;
	}

	dns_rdata_toregion(key, &r);
	INSIST(r.length >= 4);

	result = isc_md_update(md, r.base, r.length);
	if (result != ISC_R_SUCCESS) {
		goto end;
	}

	result = isc_md_final(md, digest, &digestlen);
	if (result != ISC_R_SUCCESS) {
		goto end;
	}

	dsrdata->mctx = NULL;
	dsrdata->common.rdclass = key->rdclass;
	dsrdata->common.rdtype = dns_rdatatype_ds;
	dsrdata->algorithm = r.base[3];
	dsrdata->key_tag = dst_region_computeid(&r);
	dsrdata->digest_type = digest_type;
	dsrdata->digest = digest;
	dsrdata->length = digestlen;

end:
	isc_md_free(md);
	return (result);
}

isc_result_t
dns_ds_buildrdata(dns_name_t *owner, dns_rdata_t *key,
		  dns_dsdigest_t digest_type, unsigned char *buffer,
		  dns_rdata_t *rdata) {
	isc_result_t result;
	unsigned char digest[ISC_MAX_MD_SIZE];
	dns_rdata_ds_t ds;
	isc_buffer_t b;

	result = dns_ds_fromkeyrdata(owner, key, digest_type, digest, &ds);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	memset(buffer, 0, DNS_DS_BUFFERSIZE);
	isc_buffer_init(&b, buffer, DNS_DS_BUFFERSIZE);
	result = dns_rdata_fromstruct(rdata, key->rdclass, dns_rdatatype_ds,
				      &ds, &b);
	return (result);
}
