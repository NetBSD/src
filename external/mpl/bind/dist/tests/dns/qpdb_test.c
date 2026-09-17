/*	$NetBSD: qpdb_test.c,v 1.6 2026/09/17 18:01:18 christos Exp $	*/

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
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#define KEEP_BEFORE

/* Include the main file */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include "qpcache.c"
#pragma GCC diagnostic pop

#include <tests/dns.h>

/* Set to true (or use -v option) for verbose output */
static bool verbose = false;

static void
make_rdatalist(dns_rdatalist_t *rdatalist, dns_rdataset_t *rdataset,
	       dns_rdata_t *rdata, dns_rdatatype_t type, dns_rdatatype_t covers,
	       unsigned char *data, size_t length) {
	dns_rdata_init(rdata);
	rdata->data = data;
	rdata->length = length;
	rdata->rdclass = dns_rdataclass_in;
	rdata->type = type;

	dns_rdatalist_init(rdatalist);
	rdatalist->rdclass = dns_rdataclass_in;
	rdatalist->type = type;
	rdatalist->covers = covers;
	rdatalist->ttl = 60;
	ISC_LIST_APPEND(rdatalist->rdata, rdata, link);

	dns_rdataset_init(rdataset);
	dns_rdatalist_tordataset(rdatalist, rdataset);
	rdataset->trust = dns_trust_answer;
}

/*
 * Add to a cache DB 'db' an rdataset of type 'rtype' at a name
 * <idx>.example.com. The rdataset would contain one data, and rdata_len is
 * its length. 'rtype' is supposed to be some private type whose data can be
 * arbitrary (and it doesn't matter in this test).
 */
static void
overmempurge_addrdataset(dns_db_t *db, isc_stdtime_t now, int idx,
			 dns_rdatatype_t rtype, size_t rdata_len,
			 bool longname) {
	isc_result_t result;
	dns_rdata_t rdata;
	dns_dbnode_t *node = NULL;
	dns_rdatalist_t rdatalist;
	dns_rdataset_t rdataset;
	dns_fixedname_t fname;
	dns_name_t *name;
	char namebuf[DNS_NAME_FORMATSIZE];
	unsigned char rdatabuf[65535] = { 0 }; /* large enough for any valid
						  RDATA */

	REQUIRE(rdata_len <= sizeof(rdatabuf));

	if (longname) {
		/*
		 * Build a longest possible name (in wire format) that would
		 * result in a new rbt node with the long name data.
		 */
		snprintf(namebuf, sizeof(namebuf),
			 "%010d.%010dabcdef%010dabcdef%010dabcdef%010dabcde."
			 "%010dabcdef%010dabcdef%010dabcdef%010dabcde."
			 "%010dabcdef%010dabcdef%010dabcdef%010dabcde."
			 "%010dabcdef%010dabcdef%010dabcdef01.",
			 idx, idx, idx, idx, idx, idx, idx, idx, idx, idx, idx,
			 idx, idx, idx, idx, idx);
	} else {
		snprintf(namebuf, sizeof(namebuf), "%d.example.com.", idx);
	}
	dns_test_namefromstring(namebuf, &fname);
	name = dns_fixedname_name(&fname);

	result = dns_db_findnode(db, name, true, &node);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(node);

	dns_rdata_init(&rdata);
	rdata.length = rdata_len;
	rdata.data = rdatabuf;
	rdata.rdclass = dns_rdataclass_in;
	rdata.type = rtype;

	dns_rdatalist_init(&rdatalist);
	rdatalist.rdclass = dns_rdataclass_in;
	rdatalist.type = rtype;
	rdatalist.ttl = 3600;
	ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);

	dns_rdataset_init(&rdataset);
	dns_rdatalist_tordataset(&rdatalist, &rdataset);

	result = dns_db_addrdataset(db, node, NULL, now, &rdataset, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_db_detachnode(db, &node);
}

/*
 * Add to cache DB 'db' an rdataset of type 'rtype' at 'name', with the single
 * rdata parsed from the text 'rdatastr'. The rdataset is given TTL 'ttl'
 * relative to 'now', so passing a 'now' in the past makes the entry expired
 * (and, with serve-stale enabled, stale).
 */
static void
servestale_addrdataset(dns_db_t *db, const dns_name_t *name, isc_stdtime_t now,
		       dns_rdatatype_t rtype, const char *rdatastr,
		       dns_ttl_t ttl, dns_trust_t trust) {
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
	assert_true(result == ISC_R_SUCCESS || result == DNS_R_CNAME);
	assert_non_null(node);

	dns_rdataset_init(&added);
	result = dns_db_addrdataset(db, node, NULL, now, &rdataset, 0, &added);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_rdataset_disassociate(&added);
	dns_db_detachnode(db, &node);
}

/*
 * Create a cache DB with serve-stale enabled and bind 'name' to a freshly
 * initialized name pointing into 'fname'.
 */
static dns_db_t *
servestale_setup(isc_mem_t *dbmctx, dns_fixedname_t *fname,
		 dns_name_t **namep) {
	isc_result_t result;
	dns_db_t *db = NULL;

	result = dns_db_create(dbmctx, CACHEDB_DEFAULT, dns_rootname,
			       dns_dbtype_cache, dns_rdataclass_in, 0, NULL,
			       &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Keep expired entries for a day as a last-resort fallback. */
	dns_db_setservestalettl(db, 86400);

	dns_test_namefromstring("example.com.", fname);
	*namep = dns_fixedname_name(fname);

	return db;
}

/*
 * Regression test for the find loop accepting a stale CNAME as a final answer
 * and stopping early even though a fresh record of the requested type exists
 * at the same node.
 *
 * A stale CNAME that expired two hours ago (but is still inside the stale
 * window) is added first and a fresh non-priority type is (HINFO) added last
 * last, so the stale CNAME sits at the head of the node's type list and is
 * visited first by the find loop. With serve-stale enabled, the search must
 * skip the stale CNAME and return the fresh HINFO rather than the stale CNAME.
 */
ISC_LOOP_TEST_IMPL(servestale_fresh_over_stale_cname) {
	isc_result_t result;
	dns_db_t *db = NULL;
	isc_mem_t *dbmctx = NULL;
	isc_stdtime_t now = isc_stdtime_now();
	dns_fixedname_t fname, ffound;
	dns_name_t *name = NULL, *foundname = NULL;
	dns_rdataset_t rdataset;

	isc_mem_create(&dbmctx);
	db = servestale_setup(dbmctx, &fname, &name);

	servestale_addrdataset(db, name, now - 7200, dns_rdatatype_cname,
			       "target.example.com.", 3600, dns_trust_answer);
	servestale_addrdataset(db, name, now, dns_rdatatype_hinfo,
			       "CRAY-1 NEXUS", 3600, dns_trust_answer);

	foundname = dns_fixedname_initname(&ffound);
	dns_rdataset_init(&rdataset);
	result = dns_db_find(db, name, NULL, dns_rdatatype_hinfo,
			     DNS_DBFIND_STALEOK, now, NULL, foundname,
			     &rdataset, NULL);

	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(rdataset.type, dns_rdatatype_hinfo);
	assert_int_equal(rdataset.attributes & DNS_RDATASETATTR_STALE, 0);

	dns_rdataset_disassociate(&rdataset);
	dns_db_detach(&db);
	isc_mem_detach(&dbmctx);
	isc_loopmgr_shutdown(loopmgr);
}

/*
 * Same regression, but for a stale record of the requested type masking a
 * fresh CNAME. A fresh CNAME is added first and a stale A is added last; the
 * stale A is visited first and must not short-circuit the search. The fresh
 * CNAME has to win, returning DNS_R_CNAME instead of the stale A.
 */
ISC_LOOP_TEST_IMPL(servestale_fresh_cname_over_stale_type) {
	isc_result_t result;
	dns_db_t *db = NULL;
	isc_mem_t *dbmctx = NULL;
	isc_stdtime_t now = isc_stdtime_now();
	dns_fixedname_t fname, ffound;
	dns_name_t *name = NULL, *foundname = NULL;
	dns_rdataset_t rdataset;

	isc_mem_create(&dbmctx);
	db = servestale_setup(dbmctx, &fname, &name);

	servestale_addrdataset(db, name, now, dns_rdatatype_cname,
			       "target.example.com.", 3600, dns_trust_answer);
	servestale_addrdataset(db, name, now - 7200, dns_rdatatype_a,
			       "10.53.0.1", 3600, dns_trust_answer);

	foundname = dns_fixedname_initname(&ffound);
	dns_rdataset_init(&rdataset);
	result = dns_db_find(db, name, NULL, dns_rdatatype_a,
			     DNS_DBFIND_STALEOK, now, NULL, foundname,
			     &rdataset, NULL);

	assert_int_equal(result, DNS_R_CNAME);
	assert_int_equal(rdataset.type, dns_rdatatype_cname);
	assert_int_equal(rdataset.attributes & DNS_RDATASETATTR_STALE, 0);

	dns_rdataset_disassociate(&rdataset);
	dns_db_detach(&db);
	isc_mem_detach(&dbmctx);
	isc_loopmgr_shutdown(loopmgr);
}

/*
 * Regression test for the secure-data check in add() binding an ancient
 * header. A validated RRset whose TTL passed more than QPDB_VIRTUAL
 * seconds ago is marked ancient during a lookup: its reference count
 * drops to zero, but it stays linked in the node as long as the node
 * itself is referenced. Caching an unvalidated negative entry covering
 * all types at that node then walks the node's headers looking for
 * secure data to protect; binding the ancient header would trip the
 * reference counting INSIST in bindrdataset(). The ancient header must
 * be skipped and the negative entry cached.
 */
ISC_LOOP_TEST_IMPL(ncache_add_over_ancient_secure) {
	isc_result_t result;
	dns_db_t *db = NULL;
	isc_mem_t *dbmctx = NULL;
	isc_stdtime_t now = isc_stdtime_now();
	isc_stdtime_t future = now + 3600 + QPDB_VIRTUAL + 2;
	dns_fixedname_t fname;
	dns_name_t *name = NULL;
	dns_dbnode_t *node = NULL;
	dns_slabheader_t *header = NULL;
	dns_rdatalist_t ncrdatalist;
	dns_rdataset_t ncrdataset, rdataset, added;

	isc_mem_create(&dbmctx);

	result = dns_db_create(dbmctx, "qpcache", dns_rootname,
			       dns_dbtype_cache, dns_rdataclass_in, 0, NULL,
			       &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_test_namefromstring("example.com.", &fname);
	name = dns_fixedname_name(&fname);

	servestale_addrdataset(db, name, now, dns_rdatatype_a, "10.53.0.1",
			       3600, dns_trust_secure);

	/*
	 * Hold the node and look the type up again after both the TTL and
	 * the QPDB_VIRTUAL grace period have passed: the expired RRset can
	 * no longer be found, and the node reference keeps its dead header
	 * linked.
	 */
	result = dns_db_findnode(db, name, false, &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_rdataset_init(&rdataset);
	result = dns_db_findrdataset(db, node, NULL, dns_rdatatype_a, 0, future,
				     &rdataset, NULL);
	assert_int_equal(result, ISC_R_NOTFOUND);

	header = ((qpcnode_t *)node)->data;
	assert_non_null(header);
	assert_true(header->trust >= dns_trust_secure);

	/*
	 * The lookup marks the expired header ancient only when it can
	 * upgrade the node lock; with the pthread rwlock implementation
	 * the upgrade never succeeds, so mark the header directly (a
	 * no-op when the lookup already did it).
	 */
	mark_ancient(header);
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

/*
 * A noqname or closest-encloser proof rdataset is a view into memory owned by
 * the slabheader of its parent rdataset.  Replacing the parent must not free
 * that memory while a proof view or one of its clones remains associated.
 */
ISC_LOOP_TEST_IMPL(proof_rdataset_keeps_slabheader) {
	isc_result_t result;
	dns_db_t *db = NULL;
	isc_mem_t *dbmctx = NULL;
	isc_stdtime_t now = isc_stdtime_now();
	dns_fixedname_t fname, fproof, ffound;
	dns_name_t *name = NULL, *proofname = NULL;
	dns_dbnode_t *node = NULL;
	dns_slabheader_t *oldheader = NULL, *newheader = NULL;
	dns_rdatalist_t oldlist, newlist, nseclist, siglist;
	dns_rdataset_t oldset, newset, nsecset, sigset;
	dns_rdataset_t oldbound, newbound;
	dns_rdataset_t noqname, noqnamesig, noqnameclone;
	dns_rdata_t oldrdata, newrdata, nsecrdata, sigrdata;
	unsigned char olddata[] = { 192, 0, 2, 1 };
	unsigned char newdata[] = { 192, 0, 2, 2 };
	unsigned char nsecdata[] = { 0 };
	unsigned char sigdata[] = { 0 };

	isc_mem_create(&dbmctx);
	result = dns_db_create(dbmctx, CACHEDB_DEFAULT, dns_rootname,
			       dns_dbtype_cache, dns_rdataclass_in, 0, NULL,
			       &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_test_namefromstring("proof.example.", &fname);
	name = dns_fixedname_name(&fname);
	dns_test_namefromstring("nsec.example.", &fproof);
	proofname = dns_fixedname_name(&fproof);

	make_rdatalist(&oldlist, &oldset, &oldrdata, dns_rdatatype_a, 0,
		       olddata, sizeof(olddata));
	make_rdatalist(&newlist, &newset, &newrdata, dns_rdatatype_a, 0,
		       newdata, sizeof(newdata));
	make_rdatalist(&nseclist, &nsecset, &nsecrdata, dns_rdatatype_nsec, 0,
		       nsecdata, sizeof(nsecdata));
	make_rdatalist(&siglist, &sigset, &sigrdata, dns_rdatatype_rrsig,
		       dns_rdatatype_nsec, sigdata, sizeof(sigdata));

	ISC_LIST_APPEND(proofname->list, &nsecset, link);
	ISC_LIST_APPEND(proofname->list, &sigset, link);
	result = dns_rdataset_addnoqname(&oldset, proofname,
					 dns_rdatatype_nsec);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_findnode(db, name, true, &node);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_rdataset_init(&oldbound);
	result = dns_db_addrdataset(db, node, NULL, now, &oldset, 0, &oldbound);
	assert_int_equal(result, ISC_R_SUCCESS);
	oldheader = dns_slabheader_fromrdataset(&oldbound);

	dns_rdataset_init(&noqname);
	dns_rdataset_init(&noqnamesig);
	result = dns_rdataset_getnoqname(&oldbound,
					 dns_fixedname_initname(&ffound),
					 &noqname, &noqnamesig);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_ptr_equal(noqname.proof.header, oldheader);
	assert_ptr_equal(noqnamesig.proof.header, oldheader);

	dns_rdataset_init(&noqnameclone);
	dns_rdataset_clone(&noqname, &noqnameclone);
	assert_ptr_equal(noqnameclone.proof.header, oldheader);

	/* Leave only the cache and proof views holding the old header. */
	dns_rdataset_disassociate(&oldbound);
	assert_int_equal(isc_refcount_current(&oldheader->references), 4);

	/*
	 * Replace the parent.  The old header becomes ancient and loses its
	 * cache-owned reference, but the five proof views keep it alive.
	 */
	dns_rdataset_init(&newbound);
	result = dns_db_addrdataset(db, node, NULL, now, &newset, 0, &newbound);
	assert_int_equal(result, ISC_R_SUCCESS);
	newheader = dns_slabheader_fromrdataset(&newbound);
	assert_ptr_equal(newheader->down, oldheader);
	assert_int_equal(isc_refcount_current(&oldheader->references), 3);

	clean_stale_headers(newheader);
	assert_ptr_equal(newheader->down, oldheader);
	assert_int_equal(dns_rdataset_count(&noqname), 1);
	assert_int_equal(dns_rdataset_count(&noqnamesig), 1);
	assert_int_equal(dns_rdataset_count(&noqnameclone), 1);

	dns_rdataset_disassociate(&noqnameclone);
	dns_rdataset_disassociate(&noqname);
	dns_rdataset_disassociate(&noqnamesig);
	assert_int_equal(isc_refcount_current(&oldheader->references), 0);

	clean_stale_headers(newheader);
	assert_null(newheader->down);

	dns_rdataset_disassociate(&newbound);
	dns_db_detachnode(db, &node);
	dns_db_detach(&db);
	isc_mem_detach(&dbmctx);
	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(overmempurge_bigrdata) {
	size_t maxcache = 2097152U; /* 2MB - same as DNS_CACHE_MINSIZE */
	size_t hiwater = maxcache - (maxcache >> 3); /* borrowed from cache.c */
	size_t lowater = maxcache - (maxcache >> 2); /* ditto */
	isc_result_t result;
	dns_db_t *db = NULL;
	isc_mem_t *mctx2 = NULL;
	isc_stdtime_t now = isc_stdtime_now();
	size_t i;

	isc_mem_create(&mctx2);

	result = dns_db_create(mctx2, CACHEDB_DEFAULT, dns_rootname,
			       dns_dbtype_cache, dns_rdataclass_in, 0, NULL,
			       &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_mem_setwater(mctx2, hiwater, lowater);

	/*
	 * Add cache entries with minimum size of data until 'overmem'
	 * condition is triggered.
	 * This should eventually happen, but we also limit the number of
	 * iteration to avoid an infinite loop in case something gets wrong.
	 */
	for (i = 0; !isc_mem_isovermem(mctx2) && i < (maxcache / 10); i++) {
		overmempurge_addrdataset(db, now, i, 50053, 0, false);
	}

	/*
	 * Then try to add the same number of entries, each has very large data.
	 * 'overmem purge' should keep the total cache size from exceeding
	 * the 'hiwater' mark too much. So we should be able to assume the
	 * cache size doesn't reach the "max".
	 */
	while (i-- > 0) {
		overmempurge_addrdataset(db, now, i, 50054,
					 DNS_RDATA_MAXLENGTH - 8, false);
		if (verbose) {
			print_message("# inuse: %zd max: %zd\n",
				      isc_mem_inuse(mctx2), maxcache);
		}
		assert_true(isc_mem_inuse(mctx2) < maxcache);
	}

	dns_db_detach(&db);
	isc_mem_destroy(&mctx2);
	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(overmempurge_longname) {
	size_t maxcache = 2097152U; /* 2MB - same as DNS_CACHE_MINSIZE */
	size_t hiwater = maxcache - (maxcache >> 3); /* borrowed from cache.c */
	size_t lowater = maxcache - (maxcache >> 2); /* ditto */
	isc_result_t result;
	dns_db_t *db = NULL;
	isc_mem_t *mctx2 = NULL;
	isc_stdtime_t now = isc_stdtime_now();
	size_t i;

	isc_mem_create(&mctx2);

	result = dns_db_create(mctx2, CACHEDB_DEFAULT, dns_rootname,
			       dns_dbtype_cache, dns_rdataclass_in, 0, NULL,
			       &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_mem_setwater(mctx2, hiwater, lowater);

	/*
	 * Add cache entries with minimum size of data until 'overmem'
	 * condition is triggered.
	 * This should eventually happen, but we also limit the number of
	 * iteration to avoid an infinite loop in case something gets wrong.
	 */
	for (i = 0; !isc_mem_isovermem(mctx2) && i < (maxcache / 10); i++) {
		overmempurge_addrdataset(db, now, i, 50053, 0, false);
	}

	/*
	 * Then try to add the same number of entries, each has very long name.
	 * 'overmem purge' should keep the total cache size from not exceeding
	 * the 'hiwater' mark too much. So we should be able to assume the cache
	 * size doesn't reach the "max".
	 */
	while (i-- > 0) {
		overmempurge_addrdataset(db, now, i, 50054, 0, true);
		if (verbose) {
			print_message("# inuse: %zd max: %zd\n",
				      isc_mem_inuse(mctx2), maxcache);
		}
		assert_true(isc_mem_inuse(mctx2) < maxcache);
	}

	dns_db_detach(&db);
	isc_mem_destroy(&mctx2);
	isc_loopmgr_shutdown(loopmgr);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(overmempurge_bigrdata, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(overmempurge_longname, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(servestale_fresh_over_stale_cname, setup_managers,
		      teardown_managers)
ISC_TEST_ENTRY_CUSTOM(servestale_fresh_cname_over_stale_type, setup_managers,
		      teardown_managers)
ISC_TEST_ENTRY_CUSTOM(ncache_add_over_ancient_secure, setup_managers,
		      teardown_managers)
ISC_TEST_ENTRY_CUSTOM(proof_rdataset_keeps_slabheader, setup_managers,
		      teardown_managers)
ISC_TEST_LIST_END

ISC_TEST_MAIN
