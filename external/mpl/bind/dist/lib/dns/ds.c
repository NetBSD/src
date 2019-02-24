/*	$NetBSD: ds.c,v 1.4 2019/02/24 20:01:30 christos Exp $	*/

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

#include <string.h>

#include <isc/buffer.h>
#include <isc/region.h>
#include <isc/md.h>
#include <isc/util.h>

#include <dns/ds.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdatastruct.h>
#include <dns/result.h>

#include <dst/dst.h>

isc_result_t
dns_ds_buildrdata(dns_name_t *owner, dns_rdata_t *key,
		  unsigned int digest_type, unsigned char *buffer,
		  dns_rdata_t *rdata)
{
	dns_fixedname_t fname;
	dns_name_t *name;
	unsigned char digest[ISC_MAX_MD_SIZE];
	unsigned int digestlen;
	isc_region_t r;
	isc_buffer_t b;
	dns_rdata_ds_t ds;
	isc_md_t *md;
	isc_md_type_t md_type = 0;
	isc_result_t ret;

	REQUIRE(key != NULL);
	REQUIRE(key->type == dns_rdatatype_dnskey ||
		key->type == dns_rdatatype_cdnskey);

	if (!dst_ds_digest_supported(digest_type)) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	name = dns_fixedname_initname(&fname);
	(void)dns_name_downcase(owner, name, NULL);

	memset(buffer, 0, DNS_DS_BUFFERSIZE);
	isc_buffer_init(&b, buffer, DNS_DS_BUFFERSIZE);

	md = isc_md_new();
	if (md == NULL) {
		return (ISC_R_NOMEMORY);
	}

	switch (digest_type) {
	case DNS_DSDIGEST_SHA1:
		md_type = ISC_MD_SHA1;
		break;

	case DNS_DSDIGEST_SHA384:
		md_type = ISC_MD_SHA384;
		break;

	case DNS_DSDIGEST_SHA256:
	default:
		md_type = ISC_MD_SHA256;
		break;
	}

	ret = isc_md_init(md, md_type);
	if (ret != ISC_R_SUCCESS) {
		goto end;
	}

	dns_name_toregion(name, &r);

	ret = isc_md_update(md, r.base, r.length);
	if (ret != ISC_R_SUCCESS) {
		goto end;
	}

	dns_rdata_toregion(key, &r);
	INSIST(r.length >= 4);

	ret = isc_md_update(md, r.base, r.length);
	if (ret != ISC_R_SUCCESS) {
		goto end;
	}

	ret = isc_md_final(md, digest, &digestlen);
	if (ret != ISC_R_SUCCESS) {
		goto end;
	}

	ds.mctx = NULL;
	ds.common.rdclass = key->rdclass;
	ds.common.rdtype = dns_rdatatype_ds;
	ds.algorithm = r.base[3];
	ds.key_tag = dst_region_computeid(&r);
	ds.digest_type = digest_type;
	ds.digest = digest;
	ds.length = digestlen;

	ret = dns_rdata_fromstruct(rdata, key->rdclass, dns_rdatatype_ds,
				   &ds, &b);
end:
	if (md != NULL) {
		isc_md_free(md);
	}
	return (ret);
}
