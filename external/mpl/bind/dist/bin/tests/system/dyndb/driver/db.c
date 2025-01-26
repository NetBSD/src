/*	$NetBSD: db.c,v 1.10 2025/01/26 16:24:47 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0 AND ISC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Copyright (C) 2009-2015 Red Hat
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND AUTHORS DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Database API implementation. The interface is defined in lib/dns/db.h.
 *
 * dns_db_*() calls on database instances backed by this driver use
 * struct sampledb_methods to find appropriate function implementation.
 *
 * This example re-uses RBT DB implementation from original BIND and blindly
 * proxies most of dns_db_*() calls to this underlying RBT DB.
 * See struct sampledb below.
 */

#include "db.h"
#include <inttypes.h>
#include <stdbool.h>

#include <isc/string.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/diff.h>
#include <dns/enumclass.h>
#include <dns/rbt.h>
#include <dns/rdatalist.h>
#include <dns/rdatastruct.h>
#include <dns/soa.h>
#include <dns/types.h>

#include "instance.h"
#include "syncptr.h"
#include "util.h"

#define SAMPLEDB_MAGIC ISC_MAGIC('S', 'M', 'D', 'B')
#define VALID_SAMPLEDB(sampledb) \
	((sampledb) != NULL && (sampledb)->common.impmagic == SAMPLEDB_MAGIC)

struct sampledb {
	dns_db_t common;
	sample_instance_t *inst;

	/*
	 * Internal RBT database implementation provided by BIND.
	 * Most dns_db_* calls (find(), createiterator(), etc.)
	 * are blindly forwarded to this RBT DB.
	 */
	dns_db_t *rbtdb;
};

typedef struct sampledb sampledb_t;

static void
destroy(dns_db_t *db) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_detach(&sampledb->rbtdb);
	dns_name_free(&sampledb->common.origin, sampledb->common.mctx);
	isc_mem_putanddetach(&sampledb->common.mctx, sampledb,
			     sizeof(*sampledb));
}

static void
currentversion(dns_db_t *db, dns_dbversion_t **versionp) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_currentversion(sampledb->rbtdb, versionp);
}

static isc_result_t
newversion(dns_db_t *db, dns_dbversion_t **versionp) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns_db_newversion(sampledb->rbtdb, versionp);
}

static void
attachversion(dns_db_t *db, dns_dbversion_t *source,
	      dns_dbversion_t **targetp) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_attachversion(sampledb->rbtdb, source, targetp);
}

static void
closeversion(dns_db_t *db, dns_dbversion_t **versionp,
	     bool commit DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns__db_closeversion(sampledb->rbtdb, versionp,
			     commit DNS__DB_FLARG_PASS);
}

static isc_result_t
findnode(dns_db_t *db, const dns_name_t *name, bool create,
	 dns_dbnode_t **nodep DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns__db_findnode(sampledb->rbtdb, name, create,
				nodep DNS__DB_FLARG_PASS);
}

static isc_result_t
find(dns_db_t *db, const dns_name_t *name, dns_dbversion_t *version,
     dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
     dns_dbnode_t **nodep, dns_name_t *foundname, dns_rdataset_t *rdataset,
     dns_rdataset_t *sigrdataset DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns__db_find(sampledb->rbtdb, name, version, type, options, now,
			    nodep, foundname, rdataset,
			    sigrdataset DNS__DB_FLARG_PASS);
}

static isc_result_t
findzonecut(dns_db_t *db, const dns_name_t *name, unsigned int options,
	    isc_stdtime_t now, dns_dbnode_t **nodep, dns_name_t *foundname,
	    dns_name_t *dcname, dns_rdataset_t *rdataset,
	    dns_rdataset_t *sigrdataset DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns__db_findzonecut(sampledb->rbtdb, name, options, now, nodep,
				   foundname, dcname, rdataset,
				   sigrdataset DNS__DB_FLARG_PASS);
}

static void
attachnode(dns_db_t *db, dns_dbnode_t *source,
	   dns_dbnode_t **targetp DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns__db_attachnode(sampledb->rbtdb, source, targetp DNS__DB_FLARG_PASS);
}

static void
detachnode(dns_db_t *db, dns_dbnode_t **targetp DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns__db_detachnode(sampledb->rbtdb, targetp DNS__DB_FLARG_PASS);
}

static isc_result_t
createiterator(dns_db_t *db, unsigned int options,
	       dns_dbiterator_t **iteratorp) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns_db_createiterator(sampledb->rbtdb, options, iteratorp);
}

static isc_result_t
findrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     dns_rdatatype_t type, dns_rdatatype_t covers, isc_stdtime_t now,
	     dns_rdataset_t *rdataset,
	     dns_rdataset_t *sigrdataset DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns__db_findrdataset(sampledb->rbtdb, node, version, type,
				    covers, now, rdataset,
				    sigrdataset DNS__DB_FLARG_PASS);
}

static isc_result_t
allrdatasets(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     unsigned int options, isc_stdtime_t now,
	     dns_rdatasetiter_t **iteratorp DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns__db_allrdatasets(sampledb->rbtdb, node, version, options,
				    now, iteratorp DNS__DB_FLARG_PASS);
}

static isc_result_t
addrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	    isc_stdtime_t now, dns_rdataset_t *rdataset, unsigned int options,
	    dns_rdataset_t *addedrdataset DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;
	isc_result_t result;
	dns_fixedname_t name;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_fixedname_init(&name);
	CHECK(dns__db_addrdataset(sampledb->rbtdb, node, version, now, rdataset,
				  options, addedrdataset DNS__DB_FLARG_PASS));
	if (rdataset->type == dns_rdatatype_a ||
	    rdataset->type == dns_rdatatype_aaaa)
	{
		CHECK(dns_db_nodefullname(sampledb->rbtdb, node,
					  dns_fixedname_name(&name)));
		CHECK(syncptrs(sampledb->inst, dns_fixedname_name(&name),
			       rdataset, DNS_DIFFOP_ADD));
	}

cleanup:
	return result;
}

static isc_result_t
subtractrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		 dns_rdataset_t *rdataset, unsigned int options,
		 dns_rdataset_t *newrdataset DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;
	isc_result_t result;
	dns_fixedname_t name;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_fixedname_init(&name);
	result = dns__db_subtractrdataset(sampledb->rbtdb, node, version,
					  rdataset, options,
					  newrdataset DNS__DB_FLARG_PASS);
	if (result != ISC_R_SUCCESS && result != DNS_R_NXRRSET) {
		goto cleanup;
	}

	if (rdataset->type == dns_rdatatype_a ||
	    rdataset->type == dns_rdatatype_aaaa)
	{
		CHECK(dns_db_nodefullname(sampledb->rbtdb, node,
					  dns_fixedname_name(&name)));
		CHECK(syncptrs(sampledb->inst, dns_fixedname_name(&name),
			       rdataset, DNS_DIFFOP_DEL));
	}

cleanup:
	return result;
}

/*
 * deleterdataset() function is not used during DNS update processing so syncptr
 * implementation is left as an exercise to the reader.
 */
static isc_result_t
deleterdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	       dns_rdatatype_t type, dns_rdatatype_t covers DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns__db_deleterdataset(sampledb->rbtdb, node, version, type,
				      covers DNS__DB_FLARG_PASS);
}

static bool
issecure(dns_db_t *db) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns_db_issecure(sampledb->rbtdb);
}

static unsigned int
nodecount(dns_db_t *db, dns_dbtree_t tree) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns_db_nodecount(sampledb->rbtdb, tree);
}

static void
setloop(dns_db_t *db, isc_loop_t *loop) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_setloop(sampledb->rbtdb, loop);
}

static isc_result_t
getoriginnode(dns_db_t *db, dns_dbnode_t **nodep DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns__db_getoriginnode(sampledb->rbtdb, nodep DNS__DB_FLARG_PASS);
}

static isc_result_t
getnsec3parameters(dns_db_t *db, dns_dbversion_t *version, dns_hash_t *hash,
		   uint8_t *flags, uint16_t *iterations, unsigned char *salt,
		   size_t *salt_length) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns_db_getnsec3parameters(sampledb->rbtdb, version, hash, flags,
					 iterations, salt, salt_length);
}

static isc_result_t
findnsec3node(dns_db_t *db, const dns_name_t *name, bool create,
	      dns_dbnode_t **nodep DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns__db_findnsec3node(sampledb->rbtdb, name, create,
				     nodep DNS__DB_FLARG_PASS);
}

static isc_result_t
setsigningtime(dns_db_t *db, dns_rdataset_t *rdataset, isc_stdtime_t resign) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns_db_setsigningtime(sampledb->rbtdb, rdataset, resign);
}

static isc_result_t
getsigningtime(dns_db_t *db, isc_stdtime_t *resign, dns_name_t *name,
	       dns_typepair_t *type) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns_db_getsigningtime(sampledb->rbtdb, resign, name, type);
}

static dns_stats_t *
getrrsetstats(dns_db_t *db) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns_db_getrrsetstats(sampledb->rbtdb);
}

static isc_result_t
findnodeext(dns_db_t *db, const dns_name_t *name, bool create,
	    dns_clientinfomethods_t *methods, dns_clientinfo_t *clientinfo,
	    dns_dbnode_t **nodep DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns__db_findnodeext(sampledb->rbtdb, name, create, methods,
				   clientinfo, nodep DNS__DB_FLARG_PASS);
}

static isc_result_t
findext(dns_db_t *db, const dns_name_t *name, dns_dbversion_t *version,
	dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
	dns_dbnode_t **nodep, dns_name_t *foundname,
	dns_clientinfomethods_t *methods, dns_clientinfo_t *clientinfo,
	dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset DNS__DB_FLARG) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns__db_findext(sampledb->rbtdb, name, version, type, options,
			       now, nodep, foundname, methods, clientinfo,
			       rdataset, sigrdataset DNS__DB_FLARG_PASS);
}

static isc_result_t
setcachestats(dns_db_t *db, isc_stats_t *stats) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns_db_setcachestats(sampledb->rbtdb, stats);
}

static isc_result_t
nodefullname(dns_db_t *db, dns_dbnode_t *node, dns_name_t *name) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return dns_db_nodefullname(sampledb->rbtdb, node, name);
}

/*
 * DB interface definition. Database driver uses this structure to
 * determine which implementation of dns_db_*() function to call.
 */
static dns_dbmethods_t sampledb_methods = {
	.destroy = destroy,
	.currentversion = currentversion,
	.newversion = newversion,
	.attachversion = attachversion,
	.closeversion = closeversion,
	.findnode = findnode,
	.find = find,
	.findzonecut = findzonecut,
	.attachnode = attachnode,
	.detachnode = detachnode,
	.createiterator = createiterator,
	.findrdataset = findrdataset,
	.allrdatasets = allrdatasets,
	.addrdataset = addrdataset,
	.subtractrdataset = subtractrdataset,
	.deleterdataset = deleterdataset,
	.issecure = issecure,
	.nodecount = nodecount,
	.setloop = setloop,
	.getoriginnode = getoriginnode,
	.getnsec3parameters = getnsec3parameters,
	.findnsec3node = findnsec3node,
	.setsigningtime = setsigningtime,
	.getsigningtime = getsigningtime,
	.getrrsetstats = getrrsetstats,
	.findnodeext = findnodeext,
	.findext = findext,
	.setcachestats = setcachestats,
	.nodefullname = nodefullname,
};

/* Auxiliary driver functions. */

/*
 * Auxiliary functions add_*() create minimal database which can be loaded.
 * This is necessary because this driver create empty 'fake' zone which
 * is not loaded from disk so there is no way for user to supply SOA, NS and A
 * records.
 *
 * Following functions were copied from BIND 9.10.2rc1 named/server.c,
 * credit goes to ISC.
 */
static isc_result_t
add_soa(dns_db_t *db, dns_dbversion_t *version, const dns_name_t *name,
	const dns_name_t *origin, const dns_name_t *contact) {
	dns_dbnode_t *node = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdatalist_t rdatalist;
	dns_rdataset_t rdataset;
	isc_result_t result;
	unsigned char buf[DNS_SOA_BUFFERSIZE];

	dns_rdataset_init(&rdataset);
	dns_rdatalist_init(&rdatalist);
	CHECK(dns_soa_buildrdata(origin, contact, dns_db_class(db), 0, 28800,
				 7200, 604800, 86400, buf, &rdata));
	rdatalist.type = rdata.type;
	rdatalist.covers = 0;
	rdatalist.rdclass = rdata.rdclass;
	rdatalist.ttl = 86400;
	ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);
	dns_rdatalist_tordataset(&rdatalist, &rdataset);
	CHECK(dns_db_findnode(db, name, true, &node));
	CHECK(dns_db_addrdataset(db, node, version, 0, &rdataset, 0, NULL));
cleanup:
	if (node != NULL) {
		dns_db_detachnode(db, &node);
	}
	return result;
}

static isc_result_t
add_ns(dns_db_t *db, dns_dbversion_t *version, const dns_name_t *name,
       const dns_name_t *nsname) {
	dns_dbnode_t *node = NULL;
	dns_rdata_ns_t ns;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdatalist_t rdatalist;
	dns_rdataset_t rdataset;
	isc_result_t result;
	isc_buffer_t b;
	unsigned char buf[DNS_NAME_MAXWIRE];

	isc_buffer_init(&b, buf, sizeof(buf));

	dns_rdataset_init(&rdataset);
	dns_rdatalist_init(&rdatalist);
	ns.common.rdtype = dns_rdatatype_ns;
	ns.common.rdclass = dns_db_class(db);
	ns.mctx = NULL;
	dns_name_init(&ns.name, NULL);
	dns_name_clone(nsname, &ns.name);
	CHECK(dns_rdata_fromstruct(&rdata, dns_db_class(db), dns_rdatatype_ns,
				   &ns, &b));
	rdatalist.type = rdata.type;
	rdatalist.covers = 0;
	rdatalist.rdclass = rdata.rdclass;
	rdatalist.ttl = 86400;
	ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);
	dns_rdatalist_tordataset(&rdatalist, &rdataset);
	CHECK(dns_db_findnode(db, name, true, &node));
	CHECK(dns_db_addrdataset(db, node, version, 0, &rdataset, 0, NULL));
cleanup:
	if (node != NULL) {
		dns_db_detachnode(db, &node);
	}
	return result;
}

static isc_result_t
add_a(dns_db_t *db, dns_dbversion_t *version, const dns_name_t *name,
      struct in_addr addr) {
	dns_dbnode_t *node = NULL;
	dns_rdata_in_a_t a;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdatalist_t rdatalist;
	dns_rdataset_t rdataset;
	isc_result_t result;
	isc_buffer_t b;
	unsigned char buf[DNS_NAME_MAXWIRE];

	isc_buffer_init(&b, buf, sizeof(buf));

	dns_rdataset_init(&rdataset);
	dns_rdatalist_init(&rdatalist);
	a.common.rdtype = dns_rdatatype_a;
	a.common.rdclass = dns_db_class(db);
	a.in_addr = addr;
	CHECK(dns_rdata_fromstruct(&rdata, dns_db_class(db), dns_rdatatype_a,
				   &a, &b));
	rdatalist.type = rdata.type;
	rdatalist.covers = 0;
	rdatalist.rdclass = rdata.rdclass;
	rdatalist.ttl = 86400;
	ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);
	dns_rdatalist_tordataset(&rdatalist, &rdataset);
	CHECK(dns_db_findnode(db, name, true, &node));
	CHECK(dns_db_addrdataset(db, node, version, 0, &rdataset, 0, NULL));
cleanup:
	if (node != NULL) {
		dns_db_detachnode(db, &node);
	}
	return result;
}

/*
 * Driver-specific implementation of dns_db_create().
 *
 * @param[in] argv      Database-specific parameters from dns_db_create().
 * @param[in] driverarg Driver-specific parameter from dns_db_register().
 */
isc_result_t
create_db(isc_mem_t *mctx, const dns_name_t *origin, dns_dbtype_t type,
	  dns_rdataclass_t rdclass, unsigned int argc, char *argv[],
	  void *driverarg, dns_db_t **dbp) {
	sampledb_t *sampledb = NULL;
	isc_result_t result;
	dns_dbversion_t *version = NULL;
	struct in_addr a_addr;

	REQUIRE(type == dns_dbtype_zone);
	REQUIRE(rdclass == dns_rdataclass_in);
	REQUIRE(argc == 0);
	REQUIRE(argv != NULL);
	REQUIRE(driverarg != NULL); /* pointer to driver instance */
	REQUIRE(dbp != NULL && *dbp == NULL);

	UNUSED(driverarg); /* no driver-specific configuration */

	a_addr.s_addr = 0x0100007fU;

	sampledb = isc_mem_get(mctx, sizeof(*sampledb));
	*sampledb = (sampledb_t){
		.common.magic = DNS_DB_MAGIC,
		.common.impmagic = SAMPLEDB_MAGIC,
		.common.methods = &sampledb_methods,
		.common.rdclass = rdclass,
	};

	isc_mem_attach(mctx, &sampledb->common.mctx);
	dns_name_init(&sampledb->common.origin, NULL);

	dns_name_dupwithoffsets(origin, mctx, &sampledb->common.origin);

	isc_refcount_init(&sampledb->common.references, 1);

	/* Translate instance name to instance pointer. */
	sampledb->inst = driverarg;

	/* Create internal instance of DB implementation from BIND. */
	CHECK(dns_db_create(mctx, ZONEDB_DEFAULT, origin, dns_dbtype_zone,
			    dns_rdataclass_in, 0, NULL, &sampledb->rbtdb));

	/* Create fake SOA, NS, and A records to make database loadable. */
	CHECK(dns_db_newversion(sampledb->rbtdb, &version));
	CHECK(add_soa(sampledb->rbtdb, version, origin, origin, origin));
	CHECK(add_ns(sampledb->rbtdb, version, origin, origin));
	CHECK(add_a(sampledb->rbtdb, version, origin, a_addr));
	dns_db_closeversion(sampledb->rbtdb, &version, true);

	*dbp = (dns_db_t *)sampledb;

	return ISC_R_SUCCESS;

cleanup:
	if (dns_name_dynamic(&sampledb->common.origin)) {
		dns_name_free(&sampledb->common.origin, mctx);
	}

	isc_mem_putanddetach(&sampledb->common.mctx, sampledb,
			     sizeof(*sampledb));

	return result;
}
