/*	$NetBSD: db.c,v 1.13 2025/05/21 14:48:02 christos Exp $	*/

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

/***
 *** Imports
 ***/

#include <inttypes.h>
#include <stdbool.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/result.h>
#include <isc/rwlock.h>
#include <isc/string.h>
#include <isc/tid.h>
#include <isc/urcu.h>
#include <isc/util.h>

#include <dns/callbacks.h>
#include <dns/clientinfo.h>
#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/log.h>
#include <dns/master.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdataslab.h>
#include <dns/stats.h>

/***
 *** Private Types
 ***/

struct dns_dbimplementation {
	const char *name;
	dns_dbcreatefunc_t create;
	isc_mem_t *mctx;
	void *driverarg;
	ISC_LINK(dns_dbimplementation_t) link;
};

/***
 *** Supported DB Implementations Registry
 ***/

/*
 * Built in database implementations are registered here.
 */

#include "db_p.h"
#include "qpcache_p.h"
#include "qpzone_p.h"
#include "rbtdb_p.h"

unsigned int dns_pps = 0U;

static ISC_LIST(dns_dbimplementation_t) implementations;
static isc_rwlock_t implock;
static isc_once_t once = ISC_ONCE_INIT;

static dns_dbimplementation_t rbtimp;
static dns_dbimplementation_t qpimp;
static dns_dbimplementation_t qpzoneimp;

static void
initialize(void) {
	isc_rwlock_init(&implock);

	ISC_LIST_INIT(implementations);

	rbtimp = (dns_dbimplementation_t){
		.name = "rbt",
		.create = dns__rbtdb_create,
		.link = ISC_LINK_INITIALIZER,
	};

	qpimp = (dns_dbimplementation_t){
		.name = "qpcache",
		.create = dns__qpcache_create,
		.link = ISC_LINK_INITIALIZER,
	};

	qpzoneimp = (dns_dbimplementation_t){
		.name = "qpzone",
		.create = dns__qpzone_create,
		.link = ISC_LINK_INITIALIZER,
	};

	ISC_LIST_APPEND(implementations, &rbtimp, link);
	ISC_LIST_APPEND(implementations, &qpimp, link);
	ISC_LIST_APPEND(implementations, &qpzoneimp, link);
}

static dns_dbimplementation_t *
impfind(const char *name) {
	dns_dbimplementation_t *imp;

	for (imp = ISC_LIST_HEAD(implementations); imp != NULL;
	     imp = ISC_LIST_NEXT(imp, link))
	{
		if (strcasecmp(name, imp->name) == 0) {
			return imp;
		}
	}
	return NULL;
}

static void
call_updatenotify(dns_db_t *db);

/***
 *** Basic DB Methods
 ***/

isc_result_t
dns_db_create(isc_mem_t *mctx, const char *db_type, const dns_name_t *origin,
	      dns_dbtype_t type, dns_rdataclass_t rdclass, unsigned int argc,
	      char *argv[], dns_db_t **dbp) {
	dns_dbimplementation_t *impinfo = NULL;

	isc_once_do(&once, initialize);

	/*
	 * Create a new database using implementation 'db_type'.
	 */

	REQUIRE(dbp != NULL && *dbp == NULL);
	REQUIRE(dns_name_isabsolute(origin));

	RWLOCK(&implock, isc_rwlocktype_read);
	impinfo = impfind(db_type);
	if (impinfo != NULL) {
		isc_result_t result;
		result = ((impinfo->create)(mctx, origin, type, rdclass, argc,
					    argv, impinfo->driverarg, dbp));
		RWUNLOCK(&implock, isc_rwlocktype_read);

#if DNS_DB_TRACE
		fprintf(stderr, "dns_db_create:%s:%s:%d:%p->references = 1\n",
			__func__, __FILE__, __LINE__ + 1, *dbp);
#endif
		return result;
	}

	RWUNLOCK(&implock, isc_rwlocktype_read);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_DB,
		      ISC_LOG_ERROR, "unsupported database type '%s'", db_type);

	return ISC_R_NOTFOUND;
}

static void
dns__db_destroy(dns_db_t *db) {
	(db->methods->destroy)(db);
}

#if DNS_DB_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_db, dns__db_destroy);
#else
ISC_REFCOUNT_IMPL(dns_db, dns__db_destroy);
#endif

bool
dns_db_iscache(dns_db_t *db) {
	/*
	 * Does 'db' have cache semantics?
	 */

	REQUIRE(DNS_DB_VALID(db));

	if ((db->attributes & DNS_DBATTR_CACHE) != 0) {
		return true;
	}

	return false;
}

bool
dns_db_iszone(dns_db_t *db) {
	/*
	 * Does 'db' have zone semantics?
	 */

	REQUIRE(DNS_DB_VALID(db));

	if ((db->attributes & (DNS_DBATTR_CACHE | DNS_DBATTR_STUB)) == 0) {
		return true;
	}

	return false;
}

bool
dns_db_isstub(dns_db_t *db) {
	/*
	 * Does 'db' have stub semantics?
	 */

	REQUIRE(DNS_DB_VALID(db));

	if ((db->attributes & DNS_DBATTR_STUB) != 0) {
		return true;
	}

	return false;
}

bool
dns_db_issecure(dns_db_t *db) {
	/*
	 * Is 'db' secure?
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE((db->attributes & DNS_DBATTR_CACHE) == 0);

	if (db->methods->issecure != NULL) {
		return (db->methods->issecure)(db);
	}
	return false;
}

bool
dns_db_ispersistent(dns_db_t *db) {
	/*
	 * Is 'db' persistent?
	 */

	REQUIRE(DNS_DB_VALID(db));

	if (db->methods->beginload == NULL) {
		/* If the database can't be loaded, assume it's persistent */
		return true;
	}

	return false;
}

dns_name_t *
dns_db_origin(dns_db_t *db) {
	/*
	 * The origin of the database.
	 */

	REQUIRE(DNS_DB_VALID(db));

	return &db->origin;
}

dns_rdataclass_t
dns_db_class(dns_db_t *db) {
	/*
	 * The class of the database.
	 */

	REQUIRE(DNS_DB_VALID(db));

	return db->rdclass;
}

isc_result_t
dns_db_beginload(dns_db_t *db, dns_rdatacallbacks_t *callbacks) {
	/*
	 * Begin loading 'db'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(DNS_CALLBACK_VALID(callbacks));

	if (db->methods->beginload != NULL) {
		return (db->methods->beginload)(db, callbacks);
	}
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dns_db_endload(dns_db_t *db, dns_rdatacallbacks_t *callbacks) {
	/*
	 * Finish loading 'db'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(DNS_CALLBACK_VALID(callbacks));
	REQUIRE(callbacks->add_private != NULL);

	/*
	 * When dns_db_endload() is called, we call the onupdate function
	 * for all registered listeners, regardless of whether the underlying
	 * database has an 'endload' implementation.
	 */
	call_updatenotify(db);

	if (db->methods->endload != NULL) {
		return (db->methods->endload)(db, callbacks);
	}

	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dns_db_load(dns_db_t *db, const char *filename, dns_masterformat_t format,
	    unsigned int options) {
	isc_result_t result, eresult;
	dns_rdatacallbacks_t callbacks;

	/*
	 * Load master file 'filename' into 'db'.
	 */

	REQUIRE(DNS_DB_VALID(db));

	if ((db->attributes & DNS_DBATTR_CACHE) != 0) {
		options |= DNS_MASTER_AGETTL;
	}

	dns_rdatacallbacks_init(&callbacks);
	result = dns_db_beginload(db, &callbacks);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	result = dns_master_loadfile(filename, &db->origin, &db->origin,
				     db->rdclass, options, 0, &callbacks, NULL,
				     NULL, db->mctx, format, 0);
	eresult = dns_db_endload(db, &callbacks);
	/*
	 * We always call dns_db_endload(), but we only want to return its
	 * result if dns_master_loadfile() succeeded.  If dns_master_loadfile()
	 * failed, we want to return the result code it gave us.
	 */
	if (eresult != ISC_R_SUCCESS &&
	    (result == ISC_R_SUCCESS || result == DNS_R_SEENINCLUDE))
	{
		result = eresult;
	}

	return result;
}

/***
 *** Version Methods
 ***/

void
dns_db_currentversion(dns_db_t *db, dns_dbversion_t **versionp) {
	/*
	 * Open the current version for reading.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE((db->attributes & DNS_DBATTR_CACHE) == 0);
	REQUIRE(versionp != NULL && *versionp == NULL);

	(db->methods->currentversion)(db, versionp);
}

isc_result_t
dns_db_newversion(dns_db_t *db, dns_dbversion_t **versionp) {
	/*
	 * Open a new version for reading and writing.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE((db->attributes & DNS_DBATTR_CACHE) == 0);
	REQUIRE(versionp != NULL && *versionp == NULL);

	if (db->methods->newversion != NULL) {
		return (db->methods->newversion)(db, versionp);
	}
	return ISC_R_NOTIMPLEMENTED;
}

void
dns_db_attachversion(dns_db_t *db, dns_dbversion_t *source,
		     dns_dbversion_t **targetp) {
	/*
	 * Attach '*targetp' to 'source'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE((db->attributes & DNS_DBATTR_CACHE) == 0);
	REQUIRE(source != NULL);
	REQUIRE(targetp != NULL && *targetp == NULL);

	(db->methods->attachversion)(db, source, targetp);

	ENSURE(*targetp != NULL);
}

void
dns__db_closeversion(dns_db_t *db, dns_dbversion_t **versionp,
		     bool commit DNS__DB_FLARG) {
	/*
	 * Close version '*versionp'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE((db->attributes & DNS_DBATTR_CACHE) == 0);
	REQUIRE(versionp != NULL && *versionp != NULL);

	(db->methods->closeversion)(db, versionp, commit DNS__DB_FLARG_PASS);

	if (commit) {
		call_updatenotify(db);
	}

	ENSURE(*versionp == NULL);
}

/***
 *** Node Methods
 ***/

isc_result_t
dns__db_findnode(dns_db_t *db, const dns_name_t *name, bool create,
		 dns_dbnode_t **nodep DNS__DB_FLARG) {
	/*
	 * Find the node with name 'name'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(nodep != NULL && *nodep == NULL);

	if (db->methods->findnode != NULL) {
		return (db->methods->findnode)(db, name, create,
					       nodep DNS__DB_FLARG_PASS);
	} else {
		return (db->methods->findnodeext)(db, name, create, NULL, NULL,
						  nodep DNS__DB_FLARG_PASS);
	}
}

isc_result_t
dns__db_findnodeext(dns_db_t *db, const dns_name_t *name, bool create,
		    dns_clientinfomethods_t *methods,
		    dns_clientinfo_t *clientinfo,
		    dns_dbnode_t **nodep DNS__DB_FLARG) {
	/*
	 * Find the node with name 'name', passing 'arg' to the database
	 * implementation.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(nodep != NULL && *nodep == NULL);

	if (db->methods->findnodeext != NULL) {
		return (db->methods->findnodeext)(db, name, create, methods,
						  clientinfo,
						  nodep DNS__DB_FLARG_PASS);
	} else {
		return (db->methods->findnode)(db, name, create,
					       nodep DNS__DB_FLARG_PASS);
	}
}

isc_result_t
dns__db_findnsec3node(dns_db_t *db, const dns_name_t *name, bool create,
		      dns_dbnode_t **nodep DNS__DB_FLARG) {
	/*
	 * Find the node with name 'name'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(nodep != NULL && *nodep == NULL);

	return (db->methods->findnsec3node)(db, name, create,
					    nodep DNS__DB_FLARG_PASS);
}

isc_result_t
dns__db_find(dns_db_t *db, const dns_name_t *name, dns_dbversion_t *version,
	     dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
	     dns_dbnode_t **nodep, dns_name_t *foundname,
	     dns_rdataset_t *rdataset,
	     dns_rdataset_t *sigrdataset DNS__DB_FLARG) {
	/*
	 * Find the best match for 'name' and 'type' in version 'version'
	 * of 'db'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(type != dns_rdatatype_rrsig);
	REQUIRE(nodep == NULL || *nodep == NULL);
	REQUIRE(dns_name_hasbuffer(foundname));
	REQUIRE(rdataset == NULL || (DNS_RDATASET_VALID(rdataset) &&
				     !dns_rdataset_isassociated(rdataset)));
	REQUIRE(sigrdataset == NULL ||
		(DNS_RDATASET_VALID(sigrdataset) &&
		 !dns_rdataset_isassociated(sigrdataset)));

	if (db->methods->find != NULL) {
		return (db->methods->find)(db, name, version, type, options,
					   now, nodep, foundname, rdataset,
					   sigrdataset DNS__DB_FLARG_PASS);
	} else {
		return (db->methods->findext)(
			db, name, version, type, options, now, nodep, foundname,
			NULL, NULL, rdataset, sigrdataset DNS__DB_FLARG_PASS);
	}
}

isc_result_t
dns__db_findext(dns_db_t *db, const dns_name_t *name, dns_dbversion_t *version,
		dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
		dns_dbnode_t **nodep, dns_name_t *foundname,
		dns_clientinfomethods_t *methods, dns_clientinfo_t *clientinfo,
		dns_rdataset_t *rdataset,
		dns_rdataset_t *sigrdataset DNS__DB_FLARG) {
	/*
	 * Find the best match for 'name' and 'type' in version 'version'
	 * of 'db', passing in 'arg'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(type != dns_rdatatype_rrsig);
	REQUIRE(nodep == NULL || *nodep == NULL);
	REQUIRE(dns_name_hasbuffer(foundname));
	REQUIRE(rdataset == NULL || (DNS_RDATASET_VALID(rdataset) &&
				     !dns_rdataset_isassociated(rdataset)));
	REQUIRE(sigrdataset == NULL ||
		(DNS_RDATASET_VALID(sigrdataset) &&
		 !dns_rdataset_isassociated(sigrdataset)));

	if (db->methods->findext != NULL) {
		return (db->methods->findext)(db, name, version, type, options,
					      now, nodep, foundname, methods,
					      clientinfo, rdataset,
					      sigrdataset DNS__DB_FLARG_PASS);
	} else {
		return (db->methods->find)(db, name, version, type, options,
					   now, nodep, foundname, rdataset,
					   sigrdataset DNS__DB_FLARG_PASS);
	}
}

isc_result_t
dns__db_findzonecut(dns_db_t *db, const dns_name_t *name, unsigned int options,
		    isc_stdtime_t now, dns_dbnode_t **nodep,
		    dns_name_t *foundname, dns_name_t *dcname,
		    dns_rdataset_t *rdataset,
		    dns_rdataset_t *sigrdataset DNS__DB_FLARG) {
	/*
	 * Find the deepest known zonecut which encloses 'name' in 'db'.
	 * foundname is the zonecut, dcname is the deepest name we have
	 * in database that is part of queried name.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE((db->attributes & DNS_DBATTR_CACHE) != 0);
	REQUIRE(nodep == NULL || *nodep == NULL);
	REQUIRE(dns_name_hasbuffer(foundname));
	REQUIRE(sigrdataset == NULL ||
		(DNS_RDATASET_VALID(sigrdataset) &&
		 !dns_rdataset_isassociated(sigrdataset)));

	if (db->methods->findzonecut != NULL) {
		return (db->methods->findzonecut)(
			db, name, options, now, nodep, foundname, dcname,
			rdataset, sigrdataset DNS__DB_FLARG_PASS);
	}
	return ISC_R_NOTIMPLEMENTED;
}

void
dns__db_attachnode(dns_db_t *db, dns_dbnode_t *source,
		   dns_dbnode_t **targetp DNS__DB_FLARG) {
	/*
	 * Attach *targetp to source.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(source != NULL);
	REQUIRE(targetp != NULL && *targetp == NULL);

	(db->methods->attachnode)(db, source, targetp DNS__DB_FLARG_PASS);
}

void
dns__db_detachnode(dns_db_t *db, dns_dbnode_t **nodep DNS__DB_FLARG) {
	/*
	 * Detach *nodep from its node.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(nodep != NULL && *nodep != NULL);

	(db->methods->detachnode)(db, nodep DNS__DB_FLARG_PASS);

	ENSURE(*nodep == NULL);
}

void
dns_db_transfernode(dns_db_t *db, dns_dbnode_t **sourcep,
		    dns_dbnode_t **targetp) {
	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(targetp != NULL && *targetp == NULL);
	REQUIRE(sourcep != NULL && *sourcep != NULL);

	*targetp = *sourcep;
	*sourcep = NULL;
}

/***
 *** DB Iterator Creation
 ***/

isc_result_t
dns_db_createiterator(dns_db_t *db, unsigned int flags,
		      dns_dbiterator_t **iteratorp) {
	/*
	 * Create an iterator for version 'version' of 'db'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(iteratorp != NULL && *iteratorp == NULL);
	REQUIRE((flags & (DNS_DB_NSEC3ONLY | DNS_DB_NONSEC3)) !=
		(DNS_DB_NSEC3ONLY | DNS_DB_NONSEC3));

	if (db->methods->createiterator != NULL) {
		return db->methods->createiterator(db, flags, iteratorp);
	}
	return ISC_R_NOTIMPLEMENTED;
}

/***
 *** Rdataset Methods
 ***/

isc_result_t
dns__db_findrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		     dns_rdatatype_t type, dns_rdatatype_t covers,
		     isc_stdtime_t now, dns_rdataset_t *rdataset,
		     dns_rdataset_t *sigrdataset DNS__DB_FLARG) {
	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(node != NULL);
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(!dns_rdataset_isassociated(rdataset));
	REQUIRE(covers == 0 || type == dns_rdatatype_rrsig);
	REQUIRE(type != dns_rdatatype_any);
	REQUIRE(sigrdataset == NULL ||
		(DNS_RDATASET_VALID(sigrdataset) &&
		 !dns_rdataset_isassociated(sigrdataset)));

	return (db->methods->findrdataset)(db, node, version, type, covers, now,
					   rdataset,
					   sigrdataset DNS__DB_FLARG_PASS);
}

isc_result_t
dns__db_allrdatasets(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		     unsigned int options, isc_stdtime_t now,
		     dns_rdatasetiter_t **iteratorp DNS__DB_FLARG) {
	/*
	 * Make '*iteratorp' an rdataset iteratator for all rdatasets at
	 * 'node' in version 'version' of 'db'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(iteratorp != NULL && *iteratorp == NULL);

	return (db->methods->allrdatasets)(db, node, version, options, now,
					   iteratorp DNS__DB_FLARG_PASS);
}

isc_result_t
dns__db_addrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		    isc_stdtime_t now, dns_rdataset_t *rdataset,
		    unsigned int options,
		    dns_rdataset_t *addedrdataset DNS__DB_FLARG) {
	/*
	 * Add 'rdataset' to 'node' in version 'version' of 'db'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(node != NULL);
	REQUIRE(((db->attributes & DNS_DBATTR_CACHE) == 0 && version != NULL) ||
		((db->attributes & DNS_DBATTR_CACHE) != 0 && version == NULL &&
		 (options & DNS_DBADD_MERGE) == 0));
	REQUIRE((options & DNS_DBADD_EXACT) == 0 ||
		(options & DNS_DBADD_MERGE) != 0);
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(dns_rdataset_isassociated(rdataset));
	REQUIRE(rdataset->rdclass == db->rdclass);
	REQUIRE(addedrdataset == NULL ||
		(DNS_RDATASET_VALID(addedrdataset) &&
		 !dns_rdataset_isassociated(addedrdataset)));

	if (db->methods->addrdataset != NULL) {
		return (db->methods->addrdataset)(
			db, node, version, now, rdataset, options,
			addedrdataset DNS__DB_FLARG_PASS);
	}
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dns__db_subtractrdataset(dns_db_t *db, dns_dbnode_t *node,
			 dns_dbversion_t *version, dns_rdataset_t *rdataset,
			 unsigned int options,
			 dns_rdataset_t *newrdataset DNS__DB_FLARG) {
	/*
	 * Remove any rdata in 'rdataset' from 'node' in version 'version' of
	 * 'db'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(node != NULL);
	REQUIRE((db->attributes & DNS_DBATTR_CACHE) == 0 && version != NULL);
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(dns_rdataset_isassociated(rdataset));
	REQUIRE(rdataset->rdclass == db->rdclass);
	REQUIRE(newrdataset == NULL ||
		(DNS_RDATASET_VALID(newrdataset) &&
		 !dns_rdataset_isassociated(newrdataset)));

	if (db->methods->subtractrdataset != NULL) {
		return (db->methods->subtractrdataset)(
			db, node, version, rdataset, options,
			newrdataset DNS__DB_FLARG_PASS);
	}
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dns__db_deleterdataset(dns_db_t *db, dns_dbnode_t *node,
		       dns_dbversion_t *version, dns_rdatatype_t type,
		       dns_rdatatype_t covers DNS__DB_FLARG) {
	/*
	 * Make it so that no rdataset of type 'type' exists at 'node' in
	 * version version 'version' of 'db'.
	 */

	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(node != NULL);
	REQUIRE(((db->attributes & DNS_DBATTR_CACHE) == 0 && version != NULL) ||
		((db->attributes & DNS_DBATTR_CACHE) != 0 && version == NULL));

	if (db->methods->deleterdataset != NULL) {
		return (db->methods->deleterdataset)(db, node, version, type,
						     covers DNS__DB_FLARG_PASS);
	}
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dns_db_getsoaserial(dns_db_t *db, dns_dbversion_t *ver, uint32_t *serialp) {
	isc_result_t result;
	dns_dbnode_t *node = NULL;
	dns_rdataset_t rdataset;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_buffer_t buffer;

	REQUIRE(dns_db_iszone(db) || dns_db_isstub(db));

	result = dns_db_findnode(db, dns_db_origin(db), false, &node);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	dns_rdataset_init(&rdataset);
	result = dns_db_findrdataset(db, node, ver, dns_rdatatype_soa, 0,
				     (isc_stdtime_t)0, &rdataset, NULL);
	if (result != ISC_R_SUCCESS) {
		goto freenode;
	}

	result = dns_rdataset_first(&rdataset);
	if (result != ISC_R_SUCCESS) {
		goto freerdataset;
	}
	dns_rdataset_current(&rdataset, &rdata);
	result = dns_rdataset_next(&rdataset);
	INSIST(result == ISC_R_NOMORE);

	INSIST(rdata.length > 20);
	isc_buffer_init(&buffer, rdata.data, rdata.length);
	isc_buffer_add(&buffer, rdata.length);
	isc_buffer_forward(&buffer, rdata.length - 20);
	*serialp = isc_buffer_getuint32(&buffer);

	result = ISC_R_SUCCESS;

freerdataset:
	dns_rdataset_disassociate(&rdataset);

freenode:
	dns_db_detachnode(db, &node);
	return result;
}

unsigned int
dns_db_nodecount(dns_db_t *db, dns_dbtree_t tree) {
	REQUIRE(DNS_DB_VALID(db));

	if (db->methods->nodecount != NULL) {
		return (db->methods->nodecount)(db, tree);
	}
	return 0;
}

size_t
dns_db_hashsize(dns_db_t *db) {
	REQUIRE(DNS_DB_VALID(db));

	if (db->methods->hashsize == NULL) {
		return 0;
	}

	return (db->methods->hashsize)(db);
}

void
dns_db_setloop(dns_db_t *db, isc_loop_t *loop) {
	REQUIRE(DNS_DB_VALID(db));

	if (db->methods->setloop != NULL) {
		(db->methods->setloop)(db, loop);
	}
}

isc_result_t
dns_db_register(const char *name, dns_dbcreatefunc_t create, void *driverarg,
		isc_mem_t *mctx, dns_dbimplementation_t **dbimp) {
	dns_dbimplementation_t *imp;

	REQUIRE(name != NULL);
	REQUIRE(dbimp != NULL && *dbimp == NULL);

	isc_once_do(&once, initialize);

	RWLOCK(&implock, isc_rwlocktype_write);
	imp = impfind(name);
	if (imp != NULL) {
		RWUNLOCK(&implock, isc_rwlocktype_write);
		return ISC_R_EXISTS;
	}

	imp = isc_mem_get(mctx, sizeof(dns_dbimplementation_t));
	imp->name = name;
	imp->create = create;
	imp->mctx = NULL;
	imp->driverarg = driverarg;
	isc_mem_attach(mctx, &imp->mctx);
	ISC_LINK_INIT(imp, link);
	ISC_LIST_APPEND(implementations, imp, link);
	RWUNLOCK(&implock, isc_rwlocktype_write);

	*dbimp = imp;

	return ISC_R_SUCCESS;
}

void
dns_db_unregister(dns_dbimplementation_t **dbimp) {
	dns_dbimplementation_t *imp;

	REQUIRE(dbimp != NULL && *dbimp != NULL);

	isc_once_do(&once, initialize);

	imp = *dbimp;
	*dbimp = NULL;
	RWLOCK(&implock, isc_rwlocktype_write);
	ISC_LIST_UNLINK(implementations, imp, link);
	isc_mem_putanddetach(&imp->mctx, imp, sizeof(dns_dbimplementation_t));
	RWUNLOCK(&implock, isc_rwlocktype_write);
	ENSURE(*dbimp == NULL);
}

isc_result_t
dns__db_getoriginnode(dns_db_t *db, dns_dbnode_t **nodep DNS__DB_FLARG) {
	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(dns_db_iszone(db));
	REQUIRE(nodep != NULL && *nodep == NULL);

	if (db->methods->getoriginnode != NULL) {
		return (db->methods->getoriginnode)(db,
						    nodep DNS__DB_FLARG_PASS);
	}

	return ISC_R_NOTFOUND;
}

dns_stats_t *
dns_db_getrrsetstats(dns_db_t *db) {
	REQUIRE(DNS_DB_VALID(db));

	if (db->methods->getrrsetstats != NULL) {
		return (db->methods->getrrsetstats)(db);
	}

	return NULL;
}

isc_result_t
dns_db_setcachestats(dns_db_t *db, isc_stats_t *stats) {
	REQUIRE(DNS_DB_VALID(db));

	if (db->methods->setcachestats != NULL) {
		return (db->methods->setcachestats)(db, stats);
	}

	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dns_db_getnsec3parameters(dns_db_t *db, dns_dbversion_t *version,
			  dns_hash_t *hash, uint8_t *flags,
			  uint16_t *iterations, unsigned char *salt,
			  size_t *salt_length) {
	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(dns_db_iszone(db));

	if (db->methods->getnsec3parameters != NULL) {
		return (db->methods->getnsec3parameters)(db, version, hash,
							 flags, iterations,
							 salt, salt_length);
	}

	return ISC_R_NOTFOUND;
}

isc_result_t
dns_db_getsize(dns_db_t *db, dns_dbversion_t *version, uint64_t *records,
	       uint64_t *bytes) {
	REQUIRE(DNS_DB_VALID(db));
	REQUIRE(dns_db_iszone(db));

	if (db->methods->getsize != NULL) {
		return (db->methods->getsize)(db, version, records, bytes);
	}

	return ISC_R_NOTFOUND;
}

isc_result_t
dns_db_setsigningtime(dns_db_t *db, dns_rdataset_t *rdataset,
		      isc_stdtime_t resign) {
	if (db->methods->setsigningtime != NULL) {
		return (db->methods->setsigningtime)(db, rdataset, resign);
	}
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dns_db_getsigningtime(dns_db_t *db, isc_stdtime_t *resign, dns_name_t *name,
		      dns_typepair_t *typepair) {
	if (db->methods->getsigningtime != NULL) {
		return (db->methods->getsigningtime)(db, resign, name,
						     typepair);
	}
	return ISC_R_NOTFOUND;
}

static void
call_updatenotify(dns_db_t *db) {
	rcu_read_lock();
	struct cds_lfht *update_listeners =
		rcu_dereference(db->update_listeners);
	if (update_listeners != NULL) {
		struct cds_lfht_iter iter;
		dns_dbonupdatelistener_t *listener;
		cds_lfht_for_each_entry(update_listeners, &iter, listener,
					ht_node) {
			if (!cds_lfht_is_node_deleted(&listener->ht_node)) {
				listener->onupdate(db, listener->onupdate_arg);
			}
		}
	}
	rcu_read_unlock();
}

static void
updatenotify_free(struct rcu_head *rcu_head) {
	dns_dbonupdatelistener_t *listener =
		caa_container_of(rcu_head, dns_dbonupdatelistener_t, rcu_head);
	isc_mem_putanddetach(&listener->mctx, listener, sizeof(*listener));
}

static int
updatenotify_match(struct cds_lfht_node *ht_node, const void *_key) {
	const dns_dbonupdatelistener_t *listener =
		caa_container_of(ht_node, dns_dbonupdatelistener_t, ht_node);
	const dns_dbonupdatelistener_t *key = _key;

	return listener->onupdate == key->onupdate &&
	       listener->onupdate_arg == key->onupdate_arg;
}

/*
 * Attach a notify-on-update function the database
 */
void
dns_db_updatenotify_register(dns_db_t *db, dns_dbupdate_callback_t fn,
			     void *fn_arg) {
	REQUIRE(db != NULL);
	REQUIRE(fn != NULL);

	dns_dbonupdatelistener_t key = { .onupdate = fn,
					 .onupdate_arg = fn_arg };
	uint32_t hash = isc_hash32(&key, sizeof(key), true);
	dns_dbonupdatelistener_t *listener = isc_mem_get(db->mctx,
							 sizeof(*listener));
	*listener = key;

	isc_mem_attach(db->mctx, &listener->mctx);

	rcu_read_lock();
	struct cds_lfht *update_listeners =
		rcu_dereference(db->update_listeners);
	INSIST(update_listeners != NULL);
	struct cds_lfht_node *ht_node =
		cds_lfht_add_unique(update_listeners, hash, updatenotify_match,
				    &key, &listener->ht_node);
	rcu_read_unlock();

	if (ht_node != &listener->ht_node) {
		updatenotify_free(&listener->rcu_head);
	}
}

void
dns_db_updatenotify_unregister(dns_db_t *db, dns_dbupdate_callback_t fn,
			       void *fn_arg) {
	REQUIRE(db != NULL);

	dns_dbonupdatelistener_t key = { .onupdate = fn,
					 .onupdate_arg = fn_arg };
	uint32_t hash = isc_hash32(&key, sizeof(key), true);
	struct cds_lfht_iter iter;

	rcu_read_lock();
	struct cds_lfht *update_listeners =
		rcu_dereference(db->update_listeners);
	INSIST(update_listeners != NULL);
	cds_lfht_lookup(update_listeners, hash, updatenotify_match, &key,
			&iter);

	struct cds_lfht_node *ht_node = cds_lfht_iter_get_node(&iter);
	if (ht_node != NULL && !cds_lfht_del(update_listeners, ht_node)) {
		dns_dbonupdatelistener_t *listener = caa_container_of(
			ht_node, dns_dbonupdatelistener_t, ht_node);
		call_rcu(&listener->rcu_head, updatenotify_free);
	}
	rcu_read_unlock();
}

isc_result_t
dns_db_setservestalettl(dns_db_t *db, dns_ttl_t ttl) {
	REQUIRE(DNS_DB_VALID(db));
	REQUIRE((db->attributes & DNS_DBATTR_CACHE) != 0);

	if (db->methods->setservestalettl != NULL) {
		return (db->methods->setservestalettl)(db, ttl);
	}
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dns_db_getservestalettl(dns_db_t *db, dns_ttl_t *ttl) {
	REQUIRE(DNS_DB_VALID(db));
	REQUIRE((db->attributes & DNS_DBATTR_CACHE) != 0);

	if (db->methods->getservestalettl != NULL) {
		return (db->methods->getservestalettl)(db, ttl);
	}
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dns_db_setservestalerefresh(dns_db_t *db, uint32_t interval) {
	REQUIRE(DNS_DB_VALID(db));
	REQUIRE((db->attributes & DNS_DBATTR_CACHE) != 0);

	if (db->methods->setservestalerefresh != NULL) {
		return (db->methods->setservestalerefresh)(db, interval);
	}
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dns_db_getservestalerefresh(dns_db_t *db, uint32_t *interval) {
	REQUIRE(DNS_DB_VALID(db));
	REQUIRE((db->attributes & DNS_DBATTR_CACHE) != 0);

	if (db->methods->getservestalerefresh != NULL) {
		return (db->methods->getservestalerefresh)(db, interval);
	}
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dns_db_setgluecachestats(dns_db_t *db, isc_stats_t *stats) {
	REQUIRE(dns_db_iszone(db));
	REQUIRE(stats != NULL);

	if (db->methods->setgluecachestats != NULL) {
		return (db->methods->setgluecachestats)(db, stats);
	}

	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
dns_db_addglue(dns_db_t *db, dns_dbversion_t *version, dns_rdataset_t *rdataset,
	       dns_message_t *msg) {
	REQUIRE(DNS_DB_VALID(db));
	REQUIRE((db->attributes & DNS_DBATTR_CACHE) == 0);
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(rdataset->methods != NULL);
	REQUIRE(rdataset->type == dns_rdatatype_ns);

	if (db->methods->addglue != NULL) {
		(db->methods->addglue)(db, version, rdataset, msg);

		return ISC_R_SUCCESS;
	}

	return ISC_R_NOTIMPLEMENTED;
}

void
dns_db_locknode(dns_db_t *db, dns_dbnode_t *node, isc_rwlocktype_t type) {
	if (db->methods->locknode != NULL) {
		(db->methods->locknode)(db, node, type);
	}
}

void
dns_db_unlocknode(dns_db_t *db, dns_dbnode_t *node, isc_rwlocktype_t type) {
	if (db->methods->unlocknode != NULL) {
		(db->methods->unlocknode)(db, node, type);
	}
}

void
dns_db_expiredata(dns_db_t *db, dns_dbnode_t *node, void *data) {
	if (db->methods->expiredata != NULL) {
		(db->methods->expiredata)(db, node, data);
	}
}

void
dns_db_deletedata(dns_db_t *db, dns_dbnode_t *node, void *data) {
	if (db->methods->deletedata != NULL) {
		(db->methods->deletedata)(db, node, data);
	}
}

isc_result_t
dns_db_nodefullname(dns_db_t *db, dns_dbnode_t *node, dns_name_t *name) {
	REQUIRE(db != NULL);
	REQUIRE(node != NULL);
	REQUIRE(name != NULL);

	if (db->methods->nodefullname != NULL) {
		return (db->methods->nodefullname)(db, node, name);
	}
	return ISC_R_NOTIMPLEMENTED;
}

void
dns_db_setmaxrrperset(dns_db_t *db, uint32_t value) {
	REQUIRE(DNS_DB_VALID(db));

	if (db->methods->setmaxrrperset != NULL) {
		(db->methods->setmaxrrperset)(db, value);
	}
}

void
dns_db_setmaxtypepername(dns_db_t *db, uint32_t value) {
	REQUIRE(DNS_DB_VALID(db));

	if (db->methods->setmaxtypepername != NULL) {
		(db->methods->setmaxtypepername)(db, value);
	}
}

void
dns__db_logtoomanyrecords(dns_db_t *db, const dns_name_t *name,
			  dns_rdatatype_t type, const char *op,
			  uint32_t limit) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char originbuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	char clsbuf[DNS_RDATACLASS_FORMATSIZE];

	dns_name_format(name, namebuf, sizeof(namebuf));
	dns_name_format(&db->origin, originbuf, sizeof(originbuf));
	dns_rdatatype_format(type, typebuf, sizeof(typebuf));
	dns_rdataclass_format(db->rdclass, clsbuf, sizeof(clsbuf));

	isc_log_write(
		dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_DB,
		ISC_LOG_ERROR,
		"error %s '%s/%s' in '%s/%s' (%s): %s (must not exceed %u)", op,
		namebuf, typebuf, originbuf, clsbuf,
		(db->attributes & DNS_DBATTR_CACHE) != 0 ? "cache" : "zone",
		isc_result_totext(DNS_R_TOOMANYRECORDS), limit);
}

void
dns__db_free_glue(isc_mem_t *mctx, dns_glue_t *glue) {
	while (glue != NULL) {
		dns_glue_t *next = glue->next;

		if (dns_rdataset_isassociated(&glue->rdataset_a)) {
			dns_rdataset_disassociate(&glue->rdataset_a);
		}
		if (dns_rdataset_isassociated(&glue->sigrdataset_a)) {
			dns_rdataset_disassociate(&glue->sigrdataset_a);
		}

		if (dns_rdataset_isassociated(&glue->rdataset_aaaa)) {
			dns_rdataset_disassociate(&glue->rdataset_aaaa);
		}
		if (dns_rdataset_isassociated(&glue->sigrdataset_aaaa)) {
			dns_rdataset_disassociate(&glue->sigrdataset_aaaa);
		}

		dns_rdataset_invalidate(&glue->rdataset_a);
		dns_rdataset_invalidate(&glue->sigrdataset_a);
		dns_rdataset_invalidate(&glue->rdataset_aaaa);
		dns_rdataset_invalidate(&glue->sigrdataset_aaaa);

		dns_name_free(&glue->name, mctx);

		isc_mem_put(mctx, glue, sizeof(*glue));

		glue = next;
	}
}

void
dns__db_destroy_gluelist(dns_gluelist_t **gluelistp) {
	REQUIRE(gluelistp != NULL);
	if (*gluelistp == NULL) {
		return;
	}

	dns_gluelist_t *gluelist = *gluelistp;

	dns__db_free_glue(gluelist->mctx, gluelist->glue);

	isc_mem_putanddetach(&gluelist->mctx, gluelist, sizeof(*gluelist));
}

void
dns__db_free_gluelist_rcu(struct rcu_head *rcu_head) {
	dns_gluelist_t *gluelist = caa_container_of(rcu_head, dns_gluelist_t,
						    rcu_head);
	dns__db_destroy_gluelist(&gluelist);
}

void
dns__db_cleanup_gluelists(struct cds_wfs_stack *glue_stack) {
	struct cds_wfs_head *head = __cds_wfs_pop_all(glue_stack);
	struct cds_wfs_node *node = NULL, *next = NULL;

	rcu_read_lock();
	cds_wfs_for_each_blocking_safe(head, node, next) {
		dns_gluelist_t *gluelist =
			caa_container_of(node, dns_gluelist_t, wfs_node);
		dns_slabheader_t *header = rcu_xchg_pointer(&gluelist->header,
							    NULL);
		(void)rcu_cmpxchg_pointer(&header->gluelist, gluelist, NULL);

		call_rcu(&gluelist->rcu_head, dns__db_free_gluelist_rcu);
	}
	rcu_read_unlock();
}

#define IS_REQUIRED_GLUE(r) (((r)->attributes & DNS_RDATASETATTR_REQUIRED) != 0)

static void
addglue_to_message(dns_glue_t *ge, dns_message_t *msg) {
	for (; ge != NULL; ge = ge->next) {
		dns_name_t *name = NULL;
		dns_rdataset_t *rdataset_a = NULL;
		dns_rdataset_t *sigrdataset_a = NULL;
		dns_rdataset_t *rdataset_aaaa = NULL;
		dns_rdataset_t *sigrdataset_aaaa = NULL;
		bool prepend_name = false;

		dns_message_gettempname(msg, &name);

		dns_name_copy(&ge->name, name);

		if (dns_rdataset_isassociated(&ge->rdataset_a)) {
			dns_message_gettemprdataset(msg, &rdataset_a);
		}

		if (dns_rdataset_isassociated(&ge->sigrdataset_a)) {
			dns_message_gettemprdataset(msg, &sigrdataset_a);
		}

		if (dns_rdataset_isassociated(&ge->rdataset_aaaa)) {
			dns_message_gettemprdataset(msg, &rdataset_aaaa);
		}

		if (dns_rdataset_isassociated(&ge->sigrdataset_aaaa)) {
			dns_message_gettemprdataset(msg, &sigrdataset_aaaa);
		}

		if (rdataset_a != NULL) {
			dns_rdataset_clone(&ge->rdataset_a, rdataset_a);
			ISC_LIST_APPEND(name->list, rdataset_a, link);
			if (IS_REQUIRED_GLUE(rdataset_a)) {
				prepend_name = true;
			}
		}

		if (sigrdataset_a != NULL) {
			dns_rdataset_clone(&ge->sigrdataset_a, sigrdataset_a);
			ISC_LIST_APPEND(name->list, sigrdataset_a, link);
		}

		if (rdataset_aaaa != NULL) {
			dns_rdataset_clone(&ge->rdataset_aaaa, rdataset_aaaa);
			ISC_LIST_APPEND(name->list, rdataset_aaaa, link);
			if (IS_REQUIRED_GLUE(rdataset_aaaa)) {
				prepend_name = true;
			}
		}
		if (sigrdataset_aaaa != NULL) {
			dns_rdataset_clone(&ge->sigrdataset_aaaa,
					   sigrdataset_aaaa);
			ISC_LIST_APPEND(name->list, sigrdataset_aaaa, link);
		}

		dns_message_addname(msg, name, DNS_SECTION_ADDITIONAL);

		/*
		 * When looking for required glue, dns_message_rendersection()
		 * only processes the first rdataset associated with the first
		 * name added to the ADDITIONAL section.  dns_message_addname()
		 * performs an append on the list of names in a given section,
		 * so if any glue record was marked as required, we need to
		 * move the name it is associated with to the beginning of the
		 * list for the ADDITIONAL section or else required glue might
		 * not be rendered.
		 */
		if (prepend_name) {
			ISC_LIST_UNLINK(msg->sections[DNS_SECTION_ADDITIONAL],
					name, link);
			ISC_LIST_PREPEND(msg->sections[DNS_SECTION_ADDITIONAL],
					 name, link);
		}
	}
}

static dns_gluelist_t *
new_gluelist(dns_db_t *db, dns_slabheader_t *header,
	     const dns_dbversion_t *dbversion) {
	dns_gluelist_t *gluelist = isc_mem_get(db->mctx, sizeof(*gluelist));
	*gluelist = (dns_gluelist_t){
		.version = dbversion,
		.header = header,
	};

	isc_mem_attach(db->mctx, &gluelist->mctx);

	cds_wfs_node_init(&gluelist->wfs_node);

	return gluelist;
}

static dns_gluelist_t *
create_gluelist(dns_db_t *db, dns_dbversion_t *dbversion, dns_dbnode_t *dbnode,
		dns_rdataset_t *rdataset, dns_additionaldatafunc_t add) {
	dns_slabheader_t *header = dns_slabheader_fromrdataset(rdataset);
	dns_glue_additionaldata_ctx_t ctx = {
		.db = db,
		.version = dbversion,
		.node = dbnode,
	};
	dns_gluelist_t *gluelist = new_gluelist(ctx.db, header, ctx.version);

	/*
	 * Get the owner name of the NS RRset - it will be necessary for
	 * identifying required glue in glue_nsdname_cb() (by
	 * determining which NS records in the delegation are
	 * in-bailiwick).
	 */

	(void)dns_rdataset_additionaldata(rdataset, dns_rootname, add, &ctx, 0);

	CMM_STORE_SHARED(gluelist->glue, ctx.glue);

	return gluelist;
}

isc_result_t
dns__db_addglue(dns_db_t *db, dns_dbversion_t *dbversion,
		dns_rdataset_t *rdataset, dns_message_t *msg,
		dns_additionaldatafunc_t add,
		struct cds_wfs_stack *glue_stack) {
	dns_dbnode_t *dbnode = (dns_dbnode_t *)rdataset->slab.node;
	dns_slabheader_t *header = dns_slabheader_fromrdataset(rdataset);
	dns_glue_t *glue = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(rdataset->type == dns_rdatatype_ns);

	rcu_read_lock();

	dns_gluelist_t *gluelist = rcu_dereference(header->gluelist);
	if (gluelist == NULL || gluelist->version != dbversion) {
		/* No or old glue list was found in the table. */

		dns_gluelist_t *xchg_gluelist = gluelist;
		dns_gluelist_t *old_gluelist = (void *)-1;
		dns_gluelist_t *new_gluelist =
			create_gluelist(db, dbversion, dbnode, rdataset, add);

		while (old_gluelist != xchg_gluelist &&
		       (xchg_gluelist == NULL ||
			xchg_gluelist->version != dbversion))
		{
			old_gluelist = xchg_gluelist;
			xchg_gluelist = rcu_cmpxchg_pointer(
				&header->gluelist, old_gluelist, new_gluelist);
		}

		if (old_gluelist == xchg_gluelist) {
			/* CAS was successful */
			cds_wfs_push(glue_stack, &new_gluelist->wfs_node);
			gluelist = new_gluelist;
		} else {
			dns__db_destroy_gluelist(&new_gluelist);
			gluelist = xchg_gluelist;
		}
	}

	glue = CMM_LOAD_SHARED(gluelist->glue);

	if (glue != NULL) {
		addglue_to_message(glue, msg);
		result = ISC_R_NOTFOUND;
	}

	rcu_read_unlock();

	return result;
}

dns_glue_t *
dns__db_new_glue(isc_mem_t *mctx, const dns_name_t *name) {
	dns_glue_t *glue = isc_mem_get(mctx, sizeof(*glue));
	*glue = (dns_glue_t){
		.name = DNS_NAME_INITEMPTY,
	};

	dns_name_dup(name, mctx, &glue->name);

	return glue;
}
