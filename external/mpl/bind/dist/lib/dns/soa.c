/*	$NetBSD: soa.c,v 1.3.2.2 2019/06/10 22:04:35 christos Exp $	*/

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

#include <inttypes.h>
#include <string.h>

#include <isc/buffer.h>
#include <isc/util.h>

#include <dns/rdata.h>
#include <dns/rdatastruct.h>
#include <dns/soa.h>

static inline uint32_t
decode_uint32(unsigned char *p) {
	return ((p[0] << 24) +
		(p[1] << 16) +
		(p[2] <<  8) +
		(p[3] <<  0));
}

static inline void
encode_uint32(uint32_t val, unsigned char *p) {
	p[0] = (uint8_t)(val >> 24);
	p[1] = (uint8_t)(val >> 16);
	p[2] = (uint8_t)(val >>  8);
	p[3] = (uint8_t)(val >>  0);
}

static uint32_t
soa_get(dns_rdata_t *rdata, int offset) {
	INSIST(rdata->type == dns_rdatatype_soa);
	/*
	 * Locate the field within the SOA RDATA based
	 * on its position relative to the end of the data.
	 *
	 * This is a bit of a kludge, but the alternative approach of
	 * using dns_rdata_tostruct() and dns_rdata_fromstruct() would
	 * involve a lot of unnecessary work (like building domain
	 * names and allocating temporary memory) when all we really
	 * want to do is to get 32 bits of fixed-sized data.
	 */
	INSIST(rdata->length >= 20);
	INSIST(offset >= 0 && offset <= 16);
	return (decode_uint32(rdata->data + rdata->length - 20 + offset));
}

isc_result_t
dns_soa_buildrdata(const dns_name_t *origin, const dns_name_t *contact,
		   dns_rdataclass_t rdclass,
		   uint32_t serial, uint32_t refresh,
		   uint32_t retry, uint32_t expire,
		   uint32_t minimum, unsigned char *buffer,
		   dns_rdata_t *rdata) {
	dns_rdata_soa_t soa;
	isc_buffer_t rdatabuf;

	REQUIRE(origin != NULL);
	REQUIRE(contact != NULL);

	memset(buffer, 0, DNS_SOA_BUFFERSIZE);
	isc_buffer_init(&rdatabuf, buffer, DNS_SOA_BUFFERSIZE);

	soa.common.rdtype = dns_rdatatype_soa;
	soa.common.rdclass = rdclass;
	soa.mctx = NULL;
	soa.serial = serial;
	soa.refresh = refresh;
	soa.retry = retry;
	soa.expire = expire;
	soa.minimum = minimum;
	dns_name_init(&soa.origin, NULL);
	dns_name_clone(origin, &soa.origin);
	dns_name_init(&soa.contact, NULL);
	dns_name_clone(contact, &soa.contact);

	return (dns_rdata_fromstruct(rdata, rdclass, dns_rdatatype_soa,
				      &soa, &rdatabuf));
}

uint32_t
dns_soa_getserial(dns_rdata_t *rdata) {
	return soa_get(rdata, 0);
}
uint32_t
dns_soa_getrefresh(dns_rdata_t *rdata) {
	return soa_get(rdata, 4);
}
uint32_t
dns_soa_getretry(dns_rdata_t *rdata) {
	return soa_get(rdata, 8);
}
uint32_t
dns_soa_getexpire(dns_rdata_t *rdata) {
	return soa_get(rdata, 12);
}
uint32_t
dns_soa_getminimum(dns_rdata_t *rdata) {
	return soa_get(rdata, 16);
}

static void
soa_set(dns_rdata_t *rdata, uint32_t val, int offset) {
	INSIST(rdata->type == dns_rdatatype_soa);
	INSIST(rdata->length >= 20);
	INSIST(offset >= 0 && offset <= 16);
	encode_uint32(val, rdata->data + rdata->length - 20 + offset);
}

void
dns_soa_setserial(uint32_t val, dns_rdata_t *rdata) {
	soa_set(rdata, val, 0);
}
void
dns_soa_setrefresh(uint32_t val, dns_rdata_t *rdata) {
	soa_set(rdata, val, 4);
}
void
dns_soa_setretry(uint32_t val, dns_rdata_t *rdata) {
	soa_set(rdata, val, 8);
}
void
dns_soa_setexpire(uint32_t val, dns_rdata_t *rdata) {
	soa_set(rdata, val, 12);
}
void
dns_soa_setminimum(uint32_t val, dns_rdata_t *rdata) {
	soa_set(rdata, val, 16);
}
