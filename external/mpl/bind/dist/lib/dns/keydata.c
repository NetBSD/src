/*	$NetBSD: keydata.c,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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


#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/rdata.h>
#include <dns/rdatastruct.h>
#include <dns/keydata.h>

isc_result_t
dns_keydata_todnskey(dns_rdata_keydata_t *keydata,
		     dns_rdata_dnskey_t *dnskey, isc_mem_t *mctx)
{
	REQUIRE(keydata != NULL && dnskey != NULL);

	dnskey->common.rdtype = dns_rdatatype_dnskey;
	dnskey->common.rdclass = keydata->common.rdclass;
	dnskey->mctx = mctx;
	dnskey->flags = keydata->flags;
	dnskey->protocol = keydata->protocol;
	dnskey->algorithm = keydata->algorithm;

	dnskey->datalen = keydata->datalen;

	if (mctx == NULL)
		dnskey->data = keydata->data;
	else {
		dnskey->data = isc_mem_allocate(mctx, dnskey->datalen);
		if (dnskey->data == NULL)
			return (ISC_R_NOMEMORY);
		memmove(dnskey->data, keydata->data, dnskey->datalen);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_keydata_fromdnskey(dns_rdata_keydata_t *keydata,
		       dns_rdata_dnskey_t *dnskey,
		       isc_uint32_t refresh, isc_uint32_t addhd,
		       isc_uint32_t removehd, isc_mem_t *mctx)
{
	REQUIRE(keydata != NULL && dnskey != NULL);

	keydata->common.rdtype = dns_rdatatype_keydata;
	keydata->common.rdclass = dnskey->common.rdclass;
	keydata->mctx = mctx;
	keydata->refresh = refresh;
	keydata->addhd = addhd;
	keydata->removehd = removehd;
	keydata->flags = dnskey->flags;
	keydata->protocol = dnskey->protocol;
	keydata->algorithm = dnskey->algorithm;

	keydata->datalen = dnskey->datalen;
	if (mctx == NULL)
		keydata->data = dnskey->data;
	else {
		keydata->data = isc_mem_allocate(mctx, keydata->datalen);
		if (keydata->data == NULL)
			return (ISC_R_NOMEMORY);
		memmove(keydata->data, dnskey->data, keydata->datalen);
	}

	return (ISC_R_SUCCESS);
}
