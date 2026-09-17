/*	$NetBSD: ncache_test.c,v 1.1.1.1 2026/09/17 17:45:09 christos Exp $	*/

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

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/stdtime.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/ncache.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/result.h>

#include <tests/dns.h>

/*
 * dns_ncache_add() turns every RRset it keeps from the authority section
 * into one negative cache record laid out as
 *
 *	owner name | type | trust | count | count * (rdlen | rdata)
 *
 * The rdata is copied verbatim and never parsed, so the tests below fill
 * it from this buffer and only care about its length.
 */
static unsigned char filler[UINT16_MAX];

/*
 * dns_rdataslab_fromrdataset() stores each record behind a 2-byte length,
 * or an 8-byte length, offset and order header with --enable-fixed-rrset,
 * and refuses a record that would exceed DNS_RDATA_MAXLENGTH together
 * with that header.
 */
#if DNS_RDATASET_FIXED
#define SLAB_OVERHEAD 8
#else
#define SLAB_OVERHEAD 2
#endif

static dns_name_t *
name_fromstring(dns_fixedname_t *fixed, const char *namestr) {
	dns_name_t *name = dns_fixedname_initname(fixed);
	isc_result_t result;

	result = dns_name_fromstring(name, namestr, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	return name;
}

static dns_message_t *
negative_response(void) {
	dns_message_t *msg = NULL;

	dns_message_create(mctx, NULL, NULL, DNS_MESSAGE_INTENTRENDER, &msg);
	msg->flags = DNS_MESSAGEFLAG_QR | DNS_MESSAGEFLAG_AA;
	msg->rcode = dns_rcode_nxdomain;

	return msg;
}

/*
 * Add an RRset of 'count' rdatas to the authority section of 'msg', at
 * 'ownerstr', marked for negative caching the way the resolver marks
 * them.  The rdata lengths are chosen so that the negative cache record
 * built from the RRset is exactly 'size' bytes long.
 */
static void
add_rrset(dns_message_t *msg, const char *ownerstr, dns_rdatatype_t type,
	  dns_rdatatype_t covers, unsigned int count, size_t size) {
	dns_name_t *name = NULL;
	dns_rdatalist_t *rdatalist = NULL;
	dns_rdataset_t *rdataset = NULL;
	size_t payload, rdlen;

	dns_message_gettempname(msg, &name);
	name_fromstring((dns_fixedname_t *)name, ownerstr);
	name->attributes.ncache = true;

	dns_message_gettemprdatalist(msg, &rdatalist);
	rdatalist->rdclass = dns_rdataclass_in;
	rdatalist->type = type;
	rdatalist->covers = covers;
	rdatalist->ttl = 3600;

	/*
	 * Split the rdata bytes evenly, giving the remainder to the first
	 * rdatas one byte at a time.
	 */
	assert_true(size >= name->length + 5 + count * 2);
	payload = size - name->length - 5 - count * 2;
	rdlen = payload / count;

	for (unsigned int i = 0; i < count; i++) {
		dns_rdata_t *rdata = NULL;

		dns_message_gettemprdata(msg, &rdata);
		rdata->rdclass = dns_rdataclass_in;
		rdata->type = type;
		rdata->data = filler;
		rdata->length = rdlen + (i < payload % count ? 1 : 0);
		assert_true(rdata->length <= DNS_RDATA_MAXLENGTH);
		ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
	}

	dns_message_gettemprdataset(msg, &rdataset);
	dns_rdatalist_tordataset(rdatalist, rdataset);
	rdataset->trust = dns_trust_authauthority;
	rdataset->attributes |= DNS_RDATASETATTR_NCACHE;
	ISC_LIST_APPEND(name->list, rdataset, link);

	dns_message_addname(msg, name, DNS_SECTION_AUTHORITY);
	/*
	 * dns_ncache_add() skips the section when its count is zero.
	 */
	msg->counts[DNS_SECTION_AUTHORITY] += count;
}

/*
 * Cache 'msg' as an NXDOMAIN for 'qname' in a fresh cache and return the
 * result of dns_ncache_add(), and in 'find_result' what a lookup of
 * 'qname' sees afterwards: DNS_R_NCACHENXDOMAIN if the record was stored,
 * ISC_R_NOTFOUND if it was rejected.
 */
static isc_result_t
ncache_add(dns_message_t *msg, const dns_name_t *qname,
	   isc_result_t *find_result) {
	isc_stdtime_t now = isc_stdtime_now();
	dns_db_t *db = NULL;
	dns_dbnode_t *node = NULL;
	dns_fixedname_t ffound;
	dns_name_t *found = dns_fixedname_initname(&ffound);
	dns_rdataset_t rdataset = DNS_RDATASET_INIT;
	isc_result_t result;

	result = dns_db_create(mctx, CACHEDB_DEFAULT, dns_rootname,
			       dns_dbtype_cache, dns_rdataclass_in, 0, NULL,
			       &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_findnode(db, qname, true, &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_ncache_add(msg, db, node, dns_rdatatype_any, now, 0, 3600,
				NULL);

	dns_db_detachnode(db, &node);

	*find_result = dns_db_find(db, qname, NULL, dns_rdatatype_a, 0, now,
				   NULL, found, &rdataset, NULL);
	if (dns_rdataset_isassociated(&rdataset)) {
		dns_rdataset_disassociate(&rdataset);
	}

	dns_db_detach(&db);

	return result;
}

/*
 * A record that does not fit in dns_rdata_t.length must be rejected.
 * Its size used to be truncated to 16 bits instead, so a record of
 * exactly 65536 bytes was committed to the cache with a length of zero
 * and every subsequent reader of the cache entry failed an assertion.
 */
ISC_LOOP_TEST_IMPL(ncache_add_size) {
	struct {
		size_t size;
		isc_result_t expected;
	} tests[] = {
		/*
		 * The largest record that can be stored: the slab needs
		 * SLAB_OVERHEAD more bytes for its record header.
		 */
		{ DNS_RDATA_MAXLENGTH - SLAB_OVERHEAD, ISC_R_SUCCESS },
		{ DNS_RDATA_MAXLENGTH + 1, ISC_R_NOSPACE },
		{ 65536, ISC_R_NOSPACE },
	};
	dns_fixedname_t fqname;
	dns_name_t *qname = name_fromstring(&fqname, "missing.example.");

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		dns_message_t *msg = negative_response();
		isc_result_t result, find_result;

		/*
		 * Two RRSIG(NSEC) records, because a single rdata cannot be
		 * longer than DNS_RDATA_MAXLENGTH.
		 */
		add_rrset(msg, "example.", dns_rdatatype_rrsig,
			  dns_rdatatype_nsec, 2, tests[i].size);

		result = ncache_add(msg, qname, &find_result);
		assert_int_equal(result, tests[i].expected);
		assert_int_equal(find_result, tests[i].expected == ISC_R_SUCCESS
						      ? DNS_R_NCACHENXDOMAIN
						      : ISC_R_NOTFOUND);

		dns_message_detach(&msg);
	}

	isc_loopmgr_shutdown(loopmgr);
}

/*
 * A negative response holds at most one SOA record.  The message parser
 * and the resolver enforce that on their own, so exercise the checks in
 * dns_ncache_add() with hand-built messages.
 */
ISC_LOOP_TEST_IMPL(ncache_add_soa) {
	dns_fixedname_t fqname;
	dns_name_t *qname = name_fromstring(&fqname, "missing.example.");
	dns_message_t *msg = NULL;
	isc_result_t result, find_result;

	/* One SOA is fine. */
	msg = negative_response();
	add_rrset(msg, "example.", dns_rdatatype_soa, dns_rdatatype_none, 1,
		  100);
	result = ncache_add(msg, qname, &find_result);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(find_result, DNS_R_NCACHENXDOMAIN);
	dns_message_detach(&msg);

	/* Two SOA records in one RRset are not. */
	msg = negative_response();
	add_rrset(msg, "example.", dns_rdatatype_soa, dns_rdatatype_none, 2,
		  100);
	result = ncache_add(msg, qname, &find_result);
	assert_int_equal(result, DNS_R_TOOMANYRECORDS);
	assert_int_equal(find_result, ISC_R_NOTFOUND);
	dns_message_detach(&msg);

	/* Neither are two SOA RRsets. */
	msg = negative_response();
	add_rrset(msg, "example.", dns_rdatatype_soa, dns_rdatatype_none, 1,
		  100);
	add_rrset(msg, "sub.example.", dns_rdatatype_soa, dns_rdatatype_none, 1,
		  100);
	result = ncache_add(msg, qname, &find_result);
	assert_int_equal(result, DNS_R_TOOMANYRECORDS);
	assert_int_equal(find_result, ISC_R_NOTFOUND);
	dns_message_detach(&msg);

	isc_loopmgr_shutdown(loopmgr);
}

/*
 * dns_ncache_add() stores at most DNS_NCACHE_RDATA (100) records.
 */
ISC_LOOP_TEST_IMPL(ncache_add_count) {
	dns_fixedname_t fqname;
	dns_name_t *qname = name_fromstring(&fqname, "missing.example.");
	dns_message_t *msg = negative_response();
	isc_result_t result, find_result;

	for (unsigned int i = 0; i < 101; i++) {
		char owner[64];

		snprintf(owner, sizeof(owner), "nsec%u.example.", i);
		add_rrset(msg, owner, dns_rdatatype_nsec, dns_rdatatype_none, 1,
			  100);
	}

	result = ncache_add(msg, qname, &find_result);
	assert_int_equal(result, DNS_R_TOOMANYRECORDS);
	assert_int_equal(find_result, ISC_R_NOTFOUND);
	dns_message_detach(&msg);

	isc_loopmgr_shutdown(loopmgr);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(ncache_add_size, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(ncache_add_soa, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(ncache_add_count, setup_managers, teardown_managers)
ISC_TEST_LIST_END

ISC_TEST_MAIN
