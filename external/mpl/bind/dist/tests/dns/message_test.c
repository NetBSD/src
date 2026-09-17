/*	$NetBSD: message_test.c,v 1.1.1.1 2026/09/17 17:45:09 christos Exp $	*/

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

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdataset.h>
#include <dns/rdatatype.h>

#include <tests/dns.h>

#define TTL 300

/*
 * Helpers for assembling a DNS message in wire format by hand, so that
 * it can contain things dns_message_rendersection() would never produce,
 * such as the same RR listed several times.
 */

static void
put_name(isc_buffer_t *wire, const char *namestr) {
	dns_fixedname_t fname;
	isc_region_t r;

	dns_test_namefromstring(namestr, &fname);
	dns_name_toregion(dns_fixedname_name(&fname), &r);
	isc_buffer_putmem(wire, r.base, r.length);
}

static void
put_header(isc_buffer_t *wire, unsigned int ancount, unsigned int nscount) {
	isc_buffer_putuint16(wire, 0x1234); /* ID */
	isc_buffer_putuint16(wire, DNS_MESSAGEFLAG_QR);
	isc_buffer_putuint16(wire, 1); /* QDCOUNT */
	isc_buffer_putuint16(wire, ancount);
	isc_buffer_putuint16(wire, nscount);
	isc_buffer_putuint16(wire, 0); /* ARCOUNT */
}

static void
put_question(isc_buffer_t *wire, const char *qname, dns_rdatatype_t qtype) {
	put_name(wire, qname);
	isc_buffer_putuint16(wire, qtype);
	isc_buffer_putuint16(wire, dns_rdataclass_in);
}

static void
put_rr_with_ttl(isc_buffer_t *wire, const char *owner, dns_rdatatype_t type,
		dns_ttl_t ttl, const char *rdatatext) {
	unsigned char rdatabuf[512];
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_result_t result;

	result = dns_test_rdatafromstring(&rdata, dns_rdataclass_in, type,
					  rdatabuf, sizeof(rdatabuf), rdatatext,
					  false);
	assert_int_equal(result, ISC_R_SUCCESS);

	put_name(wire, owner);
	isc_buffer_putuint16(wire, type);
	isc_buffer_putuint16(wire, dns_rdataclass_in);
	isc_buffer_putuint32(wire, ttl);
	isc_buffer_putuint16(wire, rdata.length);
	isc_buffer_putmem(wire, rdata.data, rdata.length);
}

static void
put_rr(isc_buffer_t *wire, const char *owner, dns_rdatatype_t type,
       const char *rdatatext) {
	put_rr_with_ttl(wire, owner, type, TTL, rdatatext);
}

static isc_result_t
parse(isc_buffer_t *wire, unsigned int options, dns_message_t **msgp) {
	dns_message_create(mctx, NULL, NULL, DNS_MESSAGE_INTENTPARSE, msgp);
	return dns_message_parse(*msgp, wire, options);
}

/*
 * Get the rdataset of the given type at 'owner' in 'section'.
 */
static dns_rdataset_t *
get_rdataset(dns_message_t *msg, dns_section_t section, const char *owner,
	     dns_rdatatype_t type) {
	dns_fixedname_t fname;
	dns_rdataset_t *rdataset = NULL;
	isc_result_t result;

	dns_test_namefromstring(owner, &fname);
	result = dns_message_findname(msg, section, dns_fixedname_name(&fname),
				      type, 0, NULL, &rdataset);
	assert_int_equal(result, ISC_R_SUCCESS);

	return rdataset;
}

/*
 * A record of a singleton type repeated with identical RDATA is kept once;
 * repeats of other types are all retained, as before.
 */
ISC_RUN_TEST_IMPL(parse_duplicate_singleton) {
	unsigned char wirebuf[1024];
	isc_buffer_t wire;
	dns_message_t *msg = NULL;
	isc_result_t result;
	dns_rdataset_t *rdataset = NULL;

	isc_buffer_init(&wire, wirebuf, sizeof(wirebuf));
	put_header(&wire, 4, 2);
	put_question(&wire, "dup.example.", dns_rdatatype_a);
	put_rr_with_ttl(&wire, "dup.example.", dns_rdatatype_cname, 600,
			"target.example.");
	put_rr_with_ttl(&wire, "dup.example.", dns_rdatatype_cname, 300,
			"target.example.");
	put_rr_with_ttl(&wire, "target.example.", dns_rdatatype_a, 600,
			"10.0.0.1");
	put_rr_with_ttl(&wire, "target.example.", dns_rdatatype_a, 1200,
			"10.0.0.1");
	put_rr_with_ttl(&wire, "example.", dns_rdatatype_soa, 0,
			"ns.example. hostmaster.example. 1 3600 600 86400 300");
	put_rr_with_ttl(&wire, "example.", dns_rdatatype_soa, 1200,
			"ns.example. hostmaster.example. 1 3600 600 86400 300");

	result = parse(&wire, 0, &msg);
	assert_int_equal(result, ISC_R_SUCCESS);

	rdataset = get_rdataset(msg, DNS_SECTION_ANSWER, "dup.example.",
				dns_rdatatype_cname);
	assert_non_null(rdataset);
	assert_int_equal(dns_rdataset_count(rdataset), 1);
	assert_int_equal(rdataset->ttl, 300);

	rdataset = get_rdataset(msg, DNS_SECTION_ANSWER, "target.example.",
				dns_rdatatype_a);
	assert_non_null(rdataset);
	assert_int_equal(dns_rdataset_count(rdataset), 2);
	assert_int_equal(rdataset->ttl, 600);

	rdataset = get_rdataset(msg, DNS_SECTION_AUTHORITY, "example.",
				dns_rdatatype_soa);
	assert_non_null(rdataset);
	assert_int_equal(dns_rdataset_count(rdataset), 1);
	assert_int_equal(rdataset->ttl, 0);

	dns_message_detach(&msg);
}

/*
 * A singleton type with two different RDATA is still a malformed message.
 */
ISC_RUN_TEST_IMPL(parse_conflicting_singleton) {
	unsigned char wirebuf[1024];
	isc_buffer_t wire;
	dns_message_t *msg = NULL;
	isc_result_t result;
	dns_rdataset_t *rdataset = NULL;

	isc_buffer_init(&wire, wirebuf, sizeof(wirebuf));
	put_header(&wire, 2, 0);
	put_question(&wire, "dup.example.", dns_rdatatype_a);
	put_rr(&wire, "dup.example.", dns_rdatatype_cname, "target.example.");
	put_rr(&wire, "dup.example.", dns_rdatatype_cname, "other.example.");

	result = parse(&wire, 0, &msg);
	assert_int_equal(result, DNS_R_FORMERR);
	dns_message_detach(&msg);

	/*
	 * With best-effort parsing the problem is reported but the message
	 * is still usable, and only the first CNAME survives.
	 */
	isc_buffer_first(&wire);
	result = parse(&wire, DNS_MESSAGEPARSE_BESTEFFORT, &msg);
	assert_int_equal(result, DNS_R_RECOVERABLE);

	rdataset = get_rdataset(msg, DNS_SECTION_ANSWER, "dup.example.",
				dns_rdatatype_cname);
	assert_non_null(rdataset);
	assert_int_equal(dns_rdataset_count(rdataset), 2);

	dns_message_detach(&msg);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(parse_duplicate_singleton)
ISC_TEST_ENTRY(parse_conflicting_singleton)
ISC_TEST_LIST_END

ISC_TEST_MAIN
