/*	$NetBSD: db.c,v 1.2 2018/08/12 13:02:29 christos Exp $	*/

/*
 * Database API implementation. The interface is defined in lib/dns/db.h.
 *
 * dns_db_*() calls on database instances backed by this driver use
 * struct sampledb_methods to find appropriate function implementation.
 *
 * This example re-uses RBT DB implementation from original BIND and blindly
 * proxies most of dns_db_*() calls to this underlying RBT DB.
 * See struct sampledb below.
 *
 * Copyright (C) 2009-2015  Red Hat ; see COPYRIGHT for license
 */
#include <config.h>

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

#include "db.h"
#include "instance.h"
#include "syncptr.h"
#include "util.h"

#define SAMPLEDB_MAGIC			ISC_MAGIC('S', 'M', 'D', 'B')
#define VALID_SAMPLEDB(sampledb) \
	((sampledb) != NULL && (sampledb)->common.impmagic == SAMPLEDB_MAGIC)

struct sampledb {
	dns_db_t			common;
	isc_refcount_t			refs;
	sample_instance_t		*inst;

	/*
	 * Internal RBT database implementation provided by BIND.
	 * Most dns_db_* calls (find(), createiterator(), etc.)
	 * are blindly forwarded to this RBT DB.
	 */
	dns_db_t			*rbtdb;
};

typedef struct sampledb sampledb_t;

/*
 * Get full DNS name from the node.
 *
 * @warning
 * The code silently expects that "node" came from RBTDB and thus
 * assumption dns_dbnode_t (from RBTDB) == dns_rbtnode_t is correct.
 *
 * This should work as long as we use only RBTDB and nothing else.
 */
static isc_result_t
sample_name_fromnode(dns_dbnode_t *node, dns_name_t *name) {
	dns_rbtnode_t *rbtnode = (dns_rbtnode_t *) node;
	return (dns_rbt_fullnamefromnode(rbtnode, name));
}

static void
attach(dns_db_t *source, dns_db_t **targetp) {
	sampledb_t *sampledb = (sampledb_t *)source;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	isc_refcount_increment(&sampledb->refs, NULL);
	*targetp = source;
}

static void
free_sampledb(sampledb_t *sampledb) {
	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_detach(&sampledb->rbtdb);
	dns_name_free(&sampledb->common.origin, sampledb->common.mctx);
	isc_mem_putanddetach(&sampledb->common.mctx, sampledb, sizeof(*sampledb));
}

static void
detach(dns_db_t **dbp) {
	sampledb_t *sampledb = (sampledb_t *)(*dbp);
	unsigned int refs;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	isc_refcount_decrement(&sampledb->refs, &refs);
	if (refs == 0)
		free_sampledb(sampledb);
	*dbp = NULL;
}

/*
 * This method should never be called, because DB is "persistent".
 * See ispersistent() function. It means that database do not need to be
 * loaded in the usual sense.
 */
static isc_result_t
beginload(dns_db_t *db, dns_rdatacallbacks_t *callbacks) {
	UNUSED(db);
	UNUSED(callbacks);

	fatal_error("current implementation should never call beginload()");

	/* Not reached */
	return (ISC_R_SUCCESS);
}

/*
 * This method should never be called, because DB is "persistent".
 * See ispersistent() function. It means that database do not need to be
 * loaded in the usual sense.
 */
static isc_result_t
endload(dns_db_t *db, dns_rdatacallbacks_t *callbacks) {
	UNUSED(db);
	UNUSED(callbacks);

	fatal_error("current implementation should never call endload()");

	/* Not reached */
	return (ISC_R_SUCCESS);
}

static isc_result_t
serialize(dns_db_t *db, dns_dbversion_t *version, FILE *file) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_serialize(sampledb->rbtdb, version, file));
}

static isc_result_t
dump(dns_db_t *db, dns_dbversion_t *version, const char *filename,
     dns_masterformat_t masterformat)
{

	UNUSED(db);
	UNUSED(version);
	UNUSED(filename);
	UNUSED(masterformat);

	fatal_error("current implementation should never call dump()");

	/* Not reached */
	return (ISC_R_SUCCESS);
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

	return (dns_db_newversion(sampledb->rbtdb, versionp));
}

static void
attachversion(dns_db_t *db, dns_dbversion_t *source,
	      dns_dbversion_t **targetp)
{
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_attachversion(sampledb->rbtdb, source, targetp);
}

static void
closeversion(dns_db_t *db, dns_dbversion_t **versionp, isc_boolean_t commit) {
	sampledb_t *sampledb = (sampledb_t *)db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_closeversion(sampledb->rbtdb, versionp, commit);
}

static isc_result_t
findnode(dns_db_t *db, const dns_name_t *name, isc_boolean_t create,
	 dns_dbnode_t **nodep)
{
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_findnode(sampledb->rbtdb, name, create, nodep));
}

static isc_result_t
find(dns_db_t *db, const dns_name_t *name, dns_dbversion_t *version,
     dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
     dns_dbnode_t **nodep, dns_name_t *foundname, dns_rdataset_t *rdataset,
     dns_rdataset_t *sigrdataset)
{
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_find(sampledb->rbtdb, name, version, type,
			    options, now, nodep, foundname,
			    rdataset, sigrdataset));
}

static isc_result_t
findzonecut(dns_db_t *db, const dns_name_t *name, unsigned int options,
	    isc_stdtime_t now, dns_dbnode_t **nodep, dns_name_t *foundname,
	    dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_findzonecut(sampledb->rbtdb, name, options,
				   now, nodep, foundname, rdataset,
				   sigrdataset));
}

static void
attachnode(dns_db_t *db, dns_dbnode_t *source, dns_dbnode_t **targetp) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_attachnode(sampledb->rbtdb, source, targetp);

}

static void
detachnode(dns_db_t *db, dns_dbnode_t **targetp) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_detachnode(sampledb->rbtdb, targetp);
}

static isc_result_t
expirenode(dns_db_t *db, dns_dbnode_t *node, isc_stdtime_t now) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_expirenode(sampledb->rbtdb, node, now));
}

static void
printnode(dns_db_t *db, dns_dbnode_t *node, FILE *out) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_printnode(sampledb->rbtdb, node, out);
}

static isc_result_t
createiterator(dns_db_t *db, unsigned int options,
	       dns_dbiterator_t **iteratorp)
{
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_createiterator(sampledb->rbtdb, options, iteratorp));
}

static isc_result_t
findrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     dns_rdatatype_t type, dns_rdatatype_t covers, isc_stdtime_t now,
	     dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_findrdataset(sampledb->rbtdb, node, version, type,
				    covers, now, rdataset, sigrdataset));
}

static isc_result_t
allrdatasets(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     isc_stdtime_t now, dns_rdatasetiter_t **iteratorp)
{
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_allrdatasets(sampledb->rbtdb, node, version,
				    now, iteratorp));
}

static isc_result_t
addrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	    isc_stdtime_t now, dns_rdataset_t *rdataset, unsigned int options,
	    dns_rdataset_t *addedrdataset)
{
	sampledb_t *sampledb = (sampledb_t *) db;
	isc_result_t result;
	dns_fixedname_t name;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_fixedname_init(&name);
	CHECK(dns_db_addrdataset(sampledb->rbtdb, node, version, now,
				 rdataset, options, addedrdataset));
	if (rdataset->type == dns_rdatatype_a ||
	    rdataset->type == dns_rdatatype_aaaa) {
		CHECK(sample_name_fromnode(node, dns_fixedname_name(&name)));
		CHECK(syncptrs(sampledb->inst, dns_fixedname_name(&name),
			       rdataset, DNS_DIFFOP_ADD));
	}

cleanup:
	return (result);
}

static isc_result_t
subtractrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		 dns_rdataset_t *rdataset, unsigned int options,
		 dns_rdataset_t *newrdataset)
{
	sampledb_t *sampledb = (sampledb_t *) db;
	isc_result_t result;
	dns_fixedname_t name;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_fixedname_init(&name);
	result = dns_db_subtractrdataset(sampledb->rbtdb, node, version,
					 rdataset, options, newrdataset);
	if (result != ISC_R_SUCCESS && result != DNS_R_NXRRSET)
		goto cleanup;

	if (rdataset->type == dns_rdatatype_a ||
	    rdataset->type == dns_rdatatype_aaaa) {
		CHECK(sample_name_fromnode(node, dns_fixedname_name(&name)));
		CHECK(syncptrs(sampledb->inst, dns_fixedname_name(&name),
			       rdataset, DNS_DIFFOP_DEL));
	}

cleanup:
	return (result);
}

/*
 * deleterdataset() function is not used during DNS update processing so syncptr
 * implementation is left as an exercise to the reader.
 */
static isc_result_t
deleterdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	       dns_rdatatype_t type, dns_rdatatype_t covers)
{
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_deleterdataset(sampledb->rbtdb, node, version,
				      type, covers));
}

static isc_boolean_t
issecure(dns_db_t *db) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_issecure(sampledb->rbtdb));
}

static unsigned int
nodecount(dns_db_t *db) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_nodecount(sampledb->rbtdb));
}

/*
 * The database does not need to be loaded from disk or written to disk.
 * Always return ISC_TRUE.
 */
static isc_boolean_t
ispersistent(dns_db_t *db) {
	UNUSED(db);

	return (ISC_TRUE);
}

static void
overmem(dns_db_t *db, isc_boolean_t over) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_overmem(sampledb->rbtdb, over);
}

static void
settask(dns_db_t *db, isc_task_t *task) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_settask(sampledb->rbtdb, task);
}

static isc_result_t
getoriginnode(dns_db_t *db, dns_dbnode_t **nodep) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_getoriginnode(sampledb->rbtdb, nodep));
}

static void
transfernode(dns_db_t *db, dns_dbnode_t **sourcep, dns_dbnode_t **targetp) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_transfernode(sampledb->rbtdb, sourcep, targetp);

}

static isc_result_t
getnsec3parameters(dns_db_t *db, dns_dbversion_t *version,
		   dns_hash_t *hash, isc_uint8_t *flags,
		   isc_uint16_t *iterations,
		   unsigned char *salt, size_t *salt_length)
{
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_getnsec3parameters(sampledb->rbtdb, version,
					  hash, flags, iterations,
					  salt, salt_length));

}

static isc_result_t
findnsec3node(dns_db_t *db, const dns_name_t *name, isc_boolean_t create,
	      dns_dbnode_t **nodep)
{
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_findnsec3node(sampledb->rbtdb, name, create, nodep));
}

static isc_result_t
setsigningtime(dns_db_t *db, dns_rdataset_t *rdataset, isc_stdtime_t resign) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_setsigningtime(sampledb->rbtdb, rdataset, resign));
}

static isc_result_t
getsigningtime(dns_db_t *db, dns_rdataset_t *rdataset, dns_name_t *name) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_getsigningtime(sampledb->rbtdb, rdataset, name));
}

static void
resigned(dns_db_t *db, dns_rdataset_t *rdataset, dns_dbversion_t *version) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	dns_db_resigned(sampledb->rbtdb, rdataset, version);
}

static isc_boolean_t
isdnssec(dns_db_t *db) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_isdnssec(sampledb->rbtdb));
}

static dns_stats_t *
getrrsetstats(dns_db_t *db) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_getrrsetstats(sampledb->rbtdb));

}

static isc_result_t
findnodeext(dns_db_t *db, const dns_name_t *name,
	    isc_boolean_t create, dns_clientinfomethods_t *methods,
	    dns_clientinfo_t *clientinfo, dns_dbnode_t **nodep)
{
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_findnodeext(sampledb->rbtdb, name, create,
				   methods, clientinfo, nodep));
}

static isc_result_t
findext(dns_db_t *db, const dns_name_t *name, dns_dbversion_t *version,
	dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
	dns_dbnode_t **nodep, dns_name_t *foundname,
	dns_clientinfomethods_t *methods, dns_clientinfo_t *clientinfo,
	dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_findext(sampledb->rbtdb, name, version, type,
			       options, now, nodep, foundname, methods,
			       clientinfo, rdataset, sigrdataset));
}

static isc_result_t
setcachestats(dns_db_t *db, isc_stats_t *stats) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_setcachestats(sampledb->rbtdb, stats));
}

static size_t
hashsize(dns_db_t *db) {
	sampledb_t *sampledb = (sampledb_t *) db;

	REQUIRE(VALID_SAMPLEDB(sampledb));

	return (dns_db_hashsize(sampledb->rbtdb));
}

/*
 * DB interface definition. Database driver uses this structure to
 * determine which implementation of dns_db_*() function to call.
 */
static dns_dbmethods_t sampledb_methods = {
	attach,
	detach,
	beginload,
	endload,
	serialize,
	dump,
	currentversion,
	newversion,
	attachversion,
	closeversion,
	findnode,
	find,
	findzonecut,
	attachnode,
	detachnode,
	expirenode,
	printnode,
	createiterator,
	findrdataset,
	allrdatasets,
	addrdataset,
	subtractrdataset,
	deleterdataset,
	issecure,
	nodecount,
	ispersistent,
	overmem,
	settask,
	getoriginnode,
	transfernode,
	getnsec3parameters,
	findnsec3node,
	setsigningtime,
	getsigningtime,
	resigned,
	isdnssec,
	getrrsetstats,
	NULL,			/* rpz_attach */
	NULL,			/* rpz_ready */
	findnodeext,
	findext,
	setcachestats,
	hashsize,
	NULL,			/* nodefullname */
	NULL,			/* getsize */
	NULL,			/* setservestalettl */
	NULL,			/* getservestalettl */
	NULL			/* setgluecachestats */
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
	const dns_name_t *origin, const dns_name_t *contact)
{
	dns_dbnode_t *node = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdatalist_t rdatalist;
	dns_rdataset_t rdataset;
	isc_result_t result;
	unsigned char buf[DNS_SOA_BUFFERSIZE];

	dns_rdataset_init(&rdataset);
	dns_rdatalist_init(&rdatalist);
	CHECK(dns_soa_buildrdata(origin, contact, dns_db_class(db),
				 0, 28800, 7200, 604800, 86400, buf, &rdata));
	rdatalist.type = rdata.type;
	rdatalist.covers = 0;
	rdatalist.rdclass = rdata.rdclass;
	rdatalist.ttl = 86400;
	ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);
	CHECK(dns_rdatalist_tordataset(&rdatalist, &rdataset));
	CHECK(dns_db_findnode(db, name, ISC_TRUE, &node));
	CHECK(dns_db_addrdataset(db, node, version, 0, &rdataset, 0, NULL));
 cleanup:
	if (node != NULL)
		dns_db_detachnode(db, &node);
	return (result);
}


static isc_result_t
add_ns(dns_db_t *db, dns_dbversion_t *version, const dns_name_t *name,
       const dns_name_t *nsname)
{
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
	CHECK(dns_rdatalist_tordataset(&rdatalist, &rdataset));
	CHECK(dns_db_findnode(db, name, ISC_TRUE, &node));
	CHECK(dns_db_addrdataset(db, node, version, 0, &rdataset, 0, NULL));
 cleanup:
	if (node != NULL)
		dns_db_detachnode(db, &node);
	return (result);
}

static isc_result_t
add_a(dns_db_t *db, dns_dbversion_t *version, const dns_name_t *name,
      struct in_addr addr)
{
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
	CHECK(dns_rdatalist_tordataset(&rdatalist, &rdataset));
	CHECK(dns_db_findnode(db, name, ISC_TRUE, &node));
	CHECK(dns_db_addrdataset(db, node, version, 0, &rdataset, 0, NULL));
 cleanup:
	if (node != NULL)
		dns_db_detachnode(db, &node);
	return (result);
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
	  void *driverarg, dns_db_t **dbp)
{
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

	CHECKED_MEM_GET_PTR(mctx, sampledb);
	ZERO_PTR(sampledb);

	isc_mem_attach(mctx, &sampledb->common.mctx);
	dns_name_init(&sampledb->common.origin, NULL);
	isc_ondestroy_init(&sampledb->common.ondest);

	sampledb->common.magic = DNS_DB_MAGIC;
	sampledb->common.impmagic = SAMPLEDB_MAGIC;

	sampledb->common.methods = &sampledb_methods;
	sampledb->common.attributes = 0;
	sampledb->common.rdclass = rdclass;

	CHECK(dns_name_dupwithoffsets(origin, mctx, &sampledb->common.origin));

	CHECK(isc_refcount_init(&sampledb->refs, 1));

	/* Translate instance name to instance pointer. */
	sampledb->inst = driverarg;

	/* Create internal instance of RBT DB implementation from BIND. */
	CHECK(dns_db_create(mctx, "rbt", origin, dns_dbtype_zone,
			    dns_rdataclass_in, 0, NULL, &sampledb->rbtdb));

	/* Create fake SOA, NS, and A records to make database loadable. */
	CHECK(dns_db_newversion(sampledb->rbtdb, &version));
	CHECK(add_soa(sampledb->rbtdb, version, origin, origin, origin));
	CHECK(add_ns(sampledb->rbtdb, version, origin, origin));
	CHECK(add_a(sampledb->rbtdb, version, origin, a_addr));
	dns_db_closeversion(sampledb->rbtdb, &version, ISC_TRUE);

	*dbp = (dns_db_t *)sampledb;

	return (ISC_R_SUCCESS);

cleanup:
	if (sampledb != NULL) {
		if (dns_name_dynamic(&sampledb->common.origin))
			dns_name_free(&sampledb->common.origin, mctx);

		isc_mem_putanddetach(&sampledb->common.mctx, sampledb,
				     sizeof(*sampledb));
	}

	return (result);
}
