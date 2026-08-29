/*	$NetBSD: rbtdb_test.c,v 1.1.1.2 2026/08/29 14:32:15 christos Exp $	*/

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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/util.h>

#include <dns/rbt.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdataslab.h>
#include <dns/rdatastruct.h>

#include "rbtdb_p.h"

#define ANCIENT(header)                                \
	((atomic_load_acquire(&(header)->attributes) & \
	  DNS_SLABHEADERATTR_ANCIENT) != 0)

#include <tests/dns.h>
#include <tests/isc.h>

/*
 * Add to cache DB 'db' an rdataset of type 'rtype' at 'name', with the single
 * rdata parsed from the text 'rdatastr', TTL 'ttl' relative to 'now' and
 * trust 'trust'.
 */
static void
cache_addrdataset(dns_db_t *db, const dns_name_t *name, isc_stdtime_t now,
		  dns_rdatatype_t rtype, const char *rdatastr, dns_ttl_t ttl,
		  dns_trust_t trust) {
	isc_result_t result;
	dns_rdata_t rdata;
	dns_dbnode_t *node = NULL;
	dns_rdatalist_t rdatalist;
	dns_rdataset_t rdataset, added;
	unsigned char rdatabuf[1024];

	dns_rdata_init(&rdata);
	result = dns_test_rdatafromstring(&rdata, dns_rdataclass_in, rtype,
					  rdatabuf, sizeof(rdatabuf), rdatastr,
					  false);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_rdatalist_init(&rdatalist);
	rdatalist.rdclass = dns_rdataclass_in;
	rdatalist.type = rtype;
	rdatalist.ttl = ttl;
	ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);

	dns_rdataset_init(&rdataset);
	dns_rdatalist_tordataset(&rdatalist, &rdataset);
	rdataset.trust = trust;

	result = dns_db_findnode(db, name, true, &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(node);

	dns_rdataset_init(&added);
	result = dns_db_addrdataset(db, node, NULL, now, &rdataset, 0, &added);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_rdataset_disassociate(&added);
	dns_db_detachnode(db, &node);
}

/*
 * Regression test for the secure-data check in dns__rbtdb_add() binding an
 * ancient header. A validated RRset whose TTL passed more than RBTDB_VIRTUAL
 * seconds ago is marked ancient during a lookup: its reference count drops
 * to zero, but it stays linked in the node as long as the node itself is
 * referenced. Caching an unvalidated negative entry covering all types at
 * that node then walks the node's headers looking for secure data to
 * protect; binding the ancient header would trip the reference counting
 * INSIST in dns__rbtdb_bindrdataset(). The ancient header must be skipped
 * and the negative entry cached.
 */
ISC_LOOP_TEST_IMPL(ncache_add_over_ancient_secure) {
	isc_result_t result;
	dns_db_t *db = NULL;
	isc_mem_t *dbmctx = NULL;
	isc_stdtime_t now = isc_stdtime_now();
	isc_stdtime_t future = now + 3600 + RBTDB_VIRTUAL + 2;
	dns_fixedname_t fname;
	dns_name_t *name = NULL;
	dns_dbnode_t *node = NULL;
	dns_slabheader_t *header = NULL;
	dns_rdatalist_t ncrdatalist;
	dns_rdataset_t ncrdataset, rdataset, added;

	isc_mem_create(&dbmctx);

	result = dns_db_create(dbmctx, "rbt", dns_rootname, dns_dbtype_cache,
			       dns_rdataclass_in, 0, NULL, &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_test_namefromstring("example.com.", &fname);
	name = dns_fixedname_name(&fname);

	cache_addrdataset(db, name, now, dns_rdatatype_a, "10.53.0.1", 3600,
			  dns_trust_secure);

	/*
	 * Hold the node and look the type up again after both the TTL and
	 * the RBTDB_VIRTUAL grace period have passed: the expired RRset can
	 * no longer be found, and the node reference keeps its dead header
	 * linked.
	 */
	result = dns_db_findnode(db, name, false, &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_rdataset_init(&rdataset);
	result = dns_db_findrdataset(db, node, NULL, dns_rdatatype_a, 0, future,
				     &rdataset, NULL);
	assert_int_equal(result, ISC_R_NOTFOUND);

	header = ((dns_rbtnode_t *)node)->data;
	assert_non_null(header);
	assert_true(header->trust >= dns_trust_secure);

	/*
	 * The lookup marks the expired header ancient only when it can
	 * upgrade the node lock; with the pthread rwlock implementation
	 * the upgrade never succeeds, so mark the header directly (a
	 * no-op when the lookup already did it).
	 */
	dns__rbtdb_mark_ancient(header);
	assert_true(ANCIENT(header));
	assert_int_equal(isc_refcount_current(&header->references), 0);

	/*
	 * Cache an unvalidated NXDOMAIN covering all types, shaped the way
	 * dns_ncache_add() builds it.
	 */
	dns_rdatalist_init(&ncrdatalist);
	ncrdatalist.rdclass = dns_rdataclass_in;
	ncrdatalist.covers = dns_rdatatype_any;
	ncrdatalist.ttl = 60;

	dns_rdataset_init(&ncrdataset);
	dns_rdatalist_tordataset(&ncrdatalist, &ncrdataset);
	ncrdataset.trust = dns_trust_pending_answer;
	ncrdataset.attributes |= DNS_RDATASETATTR_NEGATIVE |
				 DNS_RDATASETATTR_NXDOMAIN;

	dns_rdataset_init(&added);
	result = dns_db_addrdataset(db, node, NULL, future, &ncrdataset, 0,
				    &added);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true((added.attributes & DNS_RDATASETATTR_NEGATIVE) != 0);
	assert_true((added.attributes & DNS_RDATASETATTR_NXDOMAIN) != 0);

	dns_rdataset_disassociate(&added);
	dns_db_detachnode(db, &node);
	dns_db_detach(&db);
	isc_mem_detach(&dbmctx);
	isc_loopmgr_shutdown(loopmgr);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(ncache_add_over_ancient_secure, setup_managers,
		      teardown_managers)
ISC_TEST_LIST_END

ISC_TEST_MAIN
