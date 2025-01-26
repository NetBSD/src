/*	$NetBSD: db.h,v 1.12 2025/01/26 16:25:26 christos Exp $	*/

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

#pragma once

/*****
***** Module Info
*****/

/*! \file dns/db.h
 * \brief
 * The DNS DB interface allows named rdatasets to be stored and retrieved.
 *
 * The dns_db_t type is like a "virtual class".  To actually use
 * DBs, an implementation of the class is required.
 *
 * XXX more XXX
 *
 * MP:
 * \li	The module ensures appropriate synchronization of data structures it
 *	creates and manipulates.
 *
 * Reliability:
 * \li	No anticipated impact.
 *
 * Resources:
 * \li	TBS
 *
 * Security:
 * \li	No anticipated impact.
 *
 * Standards:
 * \li	None.
 */

/*****
***** Imports
*****/

/* Add -DDNS_DB_TRACE=1 to CFLAGS for detailed reference tracing */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/lang.h>
#include <isc/loop.h>
#include <isc/magic.h>
#include <isc/rwlock.h>
#include <isc/stats.h>
#include <isc/stdtime.h>
#include <isc/urcu.h>

#include <dns/clientinfo.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/types.h>

ISC_LANG_BEGINDECLS

/*%
 * Tuning: external query load in packets per seconds.
 */
extern unsigned int dns_pps;

/*****
***** Types
*****/

typedef struct dns_dbmethods {
	void (*destroy)(dns_db_t *db);
	isc_result_t (*beginload)(dns_db_t	       *db,
				  dns_rdatacallbacks_t *callbacks);
	isc_result_t (*endload)(dns_db_t *db, dns_rdatacallbacks_t *callbacks);
	void (*currentversion)(dns_db_t *db, dns_dbversion_t **versionp);
	isc_result_t (*newversion)(dns_db_t *db, dns_dbversion_t **versionp);
	void (*attachversion)(dns_db_t *db, dns_dbversion_t *source,
			      dns_dbversion_t **targetp);
	void (*closeversion)(dns_db_t *db, dns_dbversion_t **versionp,
			     bool commit DNS__DB_FLARG);
	isc_result_t (*findnode)(dns_db_t *db, const dns_name_t *name,
				 bool		      create,
				 dns_dbnode_t **nodep DNS__DB_FLARG);
	isc_result_t (*find)(dns_db_t *db, const dns_name_t *name,
			     dns_dbversion_t *version, dns_rdatatype_t type,
			     unsigned int options, isc_stdtime_t now,
			     dns_dbnode_t **nodep, dns_name_t *foundname,
			     dns_rdataset_t		*rdataset,
			     dns_rdataset_t *sigrdataset DNS__DB_FLARG);
	isc_result_t (*findzonecut)(dns_db_t *db, const dns_name_t *name,
				    unsigned int options, isc_stdtime_t now,
				    dns_dbnode_t **nodep, dns_name_t *foundname,
				    dns_name_t		       *dcname,
				    dns_rdataset_t	       *rdataset,
				    dns_rdataset_t *sigrdataset DNS__DB_FLARG);
	void (*attachnode)(dns_db_t *db, dns_dbnode_t *source,
			   dns_dbnode_t **targetp DNS__DB_FLARG);
	void (*detachnode)(dns_db_t *db, dns_dbnode_t **targetp DNS__DB_FLARG);
	isc_result_t (*createiterator)(dns_db_t *db, unsigned int options,
				       dns_dbiterator_t **iteratorp);
	isc_result_t (*findrdataset)(dns_db_t *db, dns_dbnode_t *node,
				     dns_dbversion_t *version,
				     dns_rdatatype_t  type,
				     dns_rdatatype_t covers, isc_stdtime_t now,
				     dns_rdataset_t		*rdataset,
				     dns_rdataset_t *sigrdataset DNS__DB_FLARG);
	isc_result_t (*allrdatasets)(
		dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		unsigned int options, isc_stdtime_t now,
		dns_rdatasetiter_t **iteratorp DNS__DB_FLARG);
	isc_result_t (*addrdataset)(dns_db_t *db, dns_dbnode_t *node,
				    dns_dbversion_t *version, isc_stdtime_t now,
				    dns_rdataset_t *rdataset,
				    unsigned int    options,
				    dns_rdataset_t *addedrdataset DNS__DB_FLARG);
	isc_result_t (*subtractrdataset)(
		dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		dns_rdataset_t *rdataset, unsigned int options,
		dns_rdataset_t *newrdataset DNS__DB_FLARG);
	isc_result_t (*deleterdataset)(dns_db_t *db, dns_dbnode_t *node,
				       dns_dbversion_t	     *version,
				       dns_rdatatype_t	      type,
				       dns_rdatatype_t covers DNS__DB_FLARG);
	bool (*issecure)(dns_db_t *db);
	unsigned int (*nodecount)(dns_db_t *db, dns_dbtree_t);
	void (*setloop)(dns_db_t *db, isc_loop_t *);
	isc_result_t (*getoriginnode)(dns_db_t		  *db,
				      dns_dbnode_t **nodep DNS__DB_FLARG);
	isc_result_t (*getnsec3parameters)(dns_db_t	   *db,
					   dns_dbversion_t *version,
					   dns_hash_t *hash, uint8_t *flags,
					   uint16_t	 *iterations,
					   unsigned char *salt,
					   size_t	 *salt_len);
	isc_result_t (*findnsec3node)(dns_db_t *db, const dns_name_t *name,
				      bool		   create,
				      dns_dbnode_t **nodep DNS__DB_FLARG);
	isc_result_t (*setsigningtime)(dns_db_t *db, dns_rdataset_t *rdataset,
				       isc_stdtime_t resign);
	isc_result_t (*getsigningtime)(dns_db_t *db, isc_stdtime_t *resign,
				       dns_name_t     *name,
				       dns_typepair_t *typepair);
	dns_stats_t *(*getrrsetstats)(dns_db_t *db);
	isc_result_t (*findnodeext)(dns_db_t *db, const dns_name_t *name,
				    bool		     create,
				    dns_clientinfomethods_t *methods,
				    dns_clientinfo_t	    *clientinfo,
				    dns_dbnode_t **nodep     DNS__DB_FLARG);
	isc_result_t (*findext)(dns_db_t *db, const dns_name_t *name,
				dns_dbversion_t *version, dns_rdatatype_t type,
				unsigned int options, isc_stdtime_t now,
				dns_dbnode_t **nodep, dns_name_t *foundname,
				dns_clientinfomethods_t	   *methods,
				dns_clientinfo_t	   *clientinfo,
				dns_rdataset_t		   *rdataset,
				dns_rdataset_t *sigrdataset DNS__DB_FLARG);
	isc_result_t (*setcachestats)(dns_db_t *db, isc_stats_t *stats);
	size_t (*hashsize)(dns_db_t *db);
	isc_result_t (*getsize)(dns_db_t *db, dns_dbversion_t *version,
				uint64_t *records, uint64_t *bytes);
	isc_result_t (*setservestalettl)(dns_db_t *db, dns_ttl_t ttl);
	isc_result_t (*getservestalettl)(dns_db_t *db, dns_ttl_t *ttl);
	isc_result_t (*setservestalerefresh)(dns_db_t *db, uint32_t interval);
	isc_result_t (*getservestalerefresh)(dns_db_t *db, uint32_t *interval);
	isc_result_t (*setgluecachestats)(dns_db_t *db, isc_stats_t *stats);
	void (*locknode)(dns_db_t *db, dns_dbnode_t *node, isc_rwlocktype_t t);
	void (*unlocknode)(dns_db_t *db, dns_dbnode_t *node,
			   isc_rwlocktype_t t);
	void (*addglue)(dns_db_t *db, dns_dbversion_t *version,
			dns_rdataset_t *rdataset, dns_message_t *msg);
	void (*expiredata)(dns_db_t *db, dns_dbnode_t *node, void *data);
	void (*deletedata)(dns_db_t *db, dns_dbnode_t *node, void *data);
	isc_result_t (*nodefullname)(dns_db_t *db, dns_dbnode_t *node,
				     dns_name_t *name);
	void (*setmaxrrperset)(dns_db_t *db, uint32_t value);
	void (*setmaxtypepername)(dns_db_t *db, uint32_t value);
} dns_dbmethods_t;

typedef isc_result_t (*dns_dbcreatefunc_t)(isc_mem_t	    *mctx,
					   const dns_name_t *name,
					   dns_dbtype_t	     type,
					   dns_rdataclass_t  rdclass,
					   unsigned int argc, char *argv[],
					   void *driverarg, dns_db_t **dbp);

typedef isc_result_t (*dns_dbupdate_callback_t)(dns_db_t *db, void *fn_arg);

#define DNS_DB_MAGIC	 ISC_MAGIC('D', 'N', 'S', 'D')
#define DNS_DB_VALID(db) ISC_MAGIC_VALID(db, DNS_DB_MAGIC)

/*%
 * This structure is actually just the common prefix of a DNS db
 * implementation's version of a dns_db_t.
 * \brief
 * Direct use of this structure by clients is forbidden.  DB implementations
 * may change the structure.  'magic' must be DNS_DB_MAGIC for any of the
 * dns_db_ routines to work.  DB implementations must maintain all DB
 * invariants.
 */
struct dns_db {
	unsigned int	 magic;
	unsigned int	 impmagic;
	dns_dbmethods_t *methods;
	uint16_t	 attributes;
	dns_rdataclass_t rdclass;
	dns_name_t	 origin;
	dns_ttl_t	 serve_stale_ttl; /* for cache DB's only */
	isc_mem_t	*mctx;
	isc_refcount_t	 references;
	struct cds_lfht *update_listeners;
};

enum {
	DNS_DBATTR_CACHE = 1 << 0,
	DNS_DBATTR_STUB = 1 << 1,
};

struct dns_dbonupdatelistener {
	isc_mem_t	       *mctx;
	dns_dbupdate_callback_t onupdate;
	void		       *onupdate_arg;
	struct cds_lfht_node	ht_node;
	struct rcu_head		rcu_head;
};

/*%
 * Used in composite databases such as RBTDB to indicate whether a node
 * exists in a specal tree for NSEC or NSEC3.
 */
enum {
	DNS_DB_NSEC_NORMAL = 0,	  /* in main tree */
	DNS_DB_NSEC_HAS_NSEC = 1, /* also has node in nsec tree */
	DNS_DB_NSEC_NSEC = 2,	  /* in nsec tree */
	DNS_DB_NSEC_NSEC3 = 3	  /* in nsec3 tree */
};

/*@{*/
/*%
 * Options that can be specified for dns_db_find().
 */
enum {
	DNS_DBFIND_GLUEOK = 1 << 0,
	DNS_DBFIND_NOWILD = 1 << 1,
	DNS_DBFIND_PENDINGOK = 1 << 2,
	DNS_DBFIND_NOEXACT = 1 << 3,
	DNS_DBFIND_COVERINGNSEC = 1 << 4,
	DNS_DBFIND_FORCENSEC3 = 1 << 5,
	DNS_DBFIND_ADDITIONALOK = 1 << 6,
	DNS_DBFIND_NOZONECUT = 1 << 7,
};

/*
 * DNS_DBFIND_STALEOK: This flag is set when BIND fails to refresh a RRset due
 * to timeout (resolver-query-timeout). Its intent is to try to look for stale
 * data in cache as a fallback, but only if stale answers are enabled in
 * configuration.
 */
#define DNS_DBFIND_STALEOK 0x0400

/*
 * DNS_DBFIND_STALEENABLED: This flag is used as a hint to the database that
 * it may use stale data. It is always set during query lookup if stale
 * answers are enabled, but only effectively used during stale-refresh-time
 * window. Also during this window, the resolver will not try to resolve the
 * query, in other words no attempt to refresh the data in cache is made when
 * the stale-refresh-time window is active.
 */
#define DNS_DBFIND_STALEENABLED 0x0800

/*
 * DNS_DBFIND_STALETIMEOUT: This flag is used when we want stale data from the
 * database, but not due to a failure in resolution, it also doesn't require
 * stale-refresh-time window timer to be active. As long as there is stale
 * data available, it should be returned.
 */
#define DNS_DBFIND_STALETIMEOUT 0x1000

/*
 * DNS_DBFIND_STALESTART: This flag is used to activate stale-refresh-time
 * window.
 */
#define DNS_DBFIND_STALESTART 0x2000
/*@}*/

/*@{*/
/*%
 * Options that can be specified for dns_db_addrdataset().
 */
#define DNS_DBADD_MERGE	   0x01
#define DNS_DBADD_FORCE	   0x02
#define DNS_DBADD_EXACT	   0x04
#define DNS_DBADD_EXACTTTL 0x08
#define DNS_DBADD_PREFETCH 0x10
/*@}*/

/*%
 * Options that can be specified for dns_db_subtractrdataset().
 */
#define DNS_DBSUB_EXACT	  0x01
#define DNS_DBSUB_WANTOLD 0x02

/*@{*/
/*%
 * Iterator options
 */
#define DNS_DB_RELATIVENAMES 0x1
#define DNS_DB_NSEC3ONLY     0x2
#define DNS_DB_NONSEC3	     0x4
/*@}*/

#define DNS_DB_STALEOK	 0x01
#define DNS_DB_EXPIREDOK 0x02

/*****
***** Methods
*****/

/***
 *** Basic DB Methods
 ***/

#if DNS_DB_TRACE
#define dns_db_ref(ptr)	  dns_db__ref(ptr, __func__, __FILE__, __LINE__)
#define dns_db_unref(ptr) dns_db__unref(ptr, __func__, __FILE__, __LINE__)
#define dns_db_attach(ptr, ptrp) \
	dns_db__attach(ptr, ptrp, __func__, __FILE__, __LINE__)
#define dns_db_detach(ptrp) dns_db__detach(ptrp, __func__, __FILE__, __LINE__)
ISC_REFCOUNT_TRACE_DECL(dns_db);
#else
ISC_REFCOUNT_DECL(dns_db);
#endif

isc_result_t
dns_db_create(isc_mem_t *mctx, const char *db_type, const dns_name_t *origin,
	      dns_dbtype_t type, dns_rdataclass_t rdclass, unsigned int argc,
	      char *argv[], dns_db_t **dbp);
/*%<
 * Create a new database using implementation 'db_type'.
 *
 * Notes:
 * \li	All names in the database must be subdomains of 'origin' and in class
 *	'rdclass'.  The database makes its own copy of the origin, so the
 *	caller may do whatever they like with 'origin' and its storage once the
 *	call returns.
 *
 * \li	DB implementation-specific parameters are passed using argc and argv.
 *
 * Requires:
 *
 * \li	dbp != NULL and *dbp == NULL
 *
 * \li	'origin' is a valid absolute domain name.
 *
 * \li	mctx is a valid memory context
 *
 * Ensures:
 *
 * \li	A copy of 'origin' has been made for the databases use, and the
 *	caller is free to do whatever they want with the name and storage
 *	associated with 'origin'.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 * \li	#ISC_R_NOTFOUND				db_type not found
 *
 * \li	Many other errors are possible, depending on what db_type was
 *	specified.
 */

bool
dns_db_iscache(dns_db_t *db);
/*%<
 * Does 'db' have cache semantics?
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 * \li	#true	'db' has cache semantics
 * \li	#false	otherwise
 */

bool
dns_db_iszone(dns_db_t *db);
/*%<
 * Does 'db' have zone semantics?
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 * \li	#true	'db' has zone semantics
 * \li	#false	otherwise
 */

bool
dns_db_isstub(dns_db_t *db);
/*%<
 * Does 'db' have stub semantics?
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 * \li	#true	'db' has zone semantics
 * \li	#false	otherwise
 */

bool
dns_db_issecure(dns_db_t *db);
/*%<
 * Is 'db' secure?
 *
 * Requires:
 *
 * \li	'db' is a valid database with zone semantics.
 *
 * Returns:
 * \li	#true	'db' is secure.
 * \li	#false	'db' is not secure.
 */

bool
dns_db_isdnssec(dns_db_t *db);
/*%<
 * Is 'db' secure or partially secure?
 *
 * Requires:
 *
 * \li	'db' is a valid database with zone semantics.
 *
 * Returns:
 * \li	#true	'db' is secure or is partially.
 * \li	#false	'db' is not secure.
 */

dns_name_t *
dns_db_origin(dns_db_t *db);
/*%<
 * The origin of the database.
 *
 * Note: caller must not try to change this name.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 *
 * \li	The origin of the database.
 */

dns_rdataclass_t
dns_db_class(dns_db_t *db);
/*%<
 * The class of the database.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 *
 * \li	The class of the database.
 */

isc_result_t
dns_db_beginload(dns_db_t *db, dns_rdatacallbacks_t *callbacks);
/*%<
 * Begin loading 'db'.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	This is the first attempt to load 'db'.
 *
 * \li  'callbacks' is a pointer to an initialized dns_rdatacallbacks_t
 *       structure.
 *
 * Ensures:
 *
 * \li	On success, callbacks->add will be a valid dns_addrdatasetfunc_t
 *      suitable for loading records into 'db' from a raw or text zone
 *      file. callbacks->add_private will be a valid DB load context
 *      which should be used as 'arg' when callbacks->add is called.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used, syntax errors in the master file, etc.
 */

isc_result_t
dns_db_endload(dns_db_t *db, dns_rdatacallbacks_t *callbacks);
/*%<
 * Finish loading 'db'.
 *
 * Requires:
 *
 * \li	'db' is a valid database that is being loaded.
 *
 * \li	'callbacks' is a valid dns_rdatacallbacks_t structure.
 *
 * \li	callbacks->add_private is not NULL and is a valid database load context.
 *
 * Ensures:
 *
 * \li	'callbacks' is returned to its state prior to calling dns_db_beginload()
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used, syntax errors in the master file, etc.
 */

isc_result_t
dns_db_load(dns_db_t *db, const char *filename, dns_masterformat_t format,
	    unsigned int options);
/*%<
 * Load master file 'filename' into 'db'.
 *
 * Notes:
 * \li	This routine is equivalent to calling
 *
 *\code
 *		dns_db_beginload();
 *		dns_master_loadfile();
 *		dns_db_endload();
 *\endcode
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	This is the first attempt to load 'db'.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used, syntax errors in the master file, etc.
 */

isc_result_t
dns_db_dump(dns_db_t *db, dns_dbversion_t *version, const char *filename);
/*%<
 * Dump version 'version' of 'db' to master file 'filename'.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'version' is a valid version.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used, OS file errors, etc.
 */

/***
 *** Version Methods
 ***/

void
dns_db_currentversion(dns_db_t *db, dns_dbversion_t **versionp);
/*%<
 * Open the current version for reading.
 *
 * Requires:
 *
 * \li	'db' is a valid database with zone semantics.
 *
 * \li	versionp != NULL && *verisonp == NULL
 *
 * Ensures:
 *
 * \li	On success, '*versionp' is attached to the current version.
 *
 */

isc_result_t
dns_db_newversion(dns_db_t *db, dns_dbversion_t **versionp);
/*%<
 * Open a new version for reading and writing.
 *
 * Requires:
 *
 * \li	'db' is a valid database with zone semantics.
 *
 * \li	versionp != NULL && *verisonp == NULL
 *
 * Ensures:
 *
 * \li	On success, '*versionp' is attached to the current version.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

void
dns_db_attachversion(dns_db_t *db, dns_dbversion_t *source,
		     dns_dbversion_t **targetp);
/*%<
 * Attach '*targetp' to 'source'.
 *
 * Requires:
 *
 * \li	'db' is a valid database with zone semantics.
 *
 * \li	source is a valid open version
 *
 * \li	targetp != NULL && *targetp == NULL
 *
 * Ensures:
 *
 * \li	'*targetp' is attached to source.
 */

#define dns_db_closeversion(db, versionp, commit) \
	dns__db_closeversion(db, versionp, commit DNS__DB_FILELINE)
void
dns__db_closeversion(dns_db_t *db, dns_dbversion_t **versionp,
		     bool commit DNS__DB_FLARG);
/*%<
 * Close version '*versionp'.
 *
 * Note: if '*versionp' is a read-write version and 'commit' is true,
 * then all changes made in the version will take effect, otherwise they
 * will be rolled back.  The value of 'commit' is ignored for read-only
 * versions.
 *
 * Requires:
 *
 * \li	'db' is a valid database with zone semantics.
 *
 * \li	'*versionp' refers to a valid version.
 *
 * \li	If committing a writable version, then there must be no other
 *	outstanding references to the version (e.g. an active rdataset
 *	iterator).
 *
 * Ensures:
 *
 * \li	*versionp == NULL
 *
 * \li	If *versionp is a read-write version, and commit is true, then
 *	the version will become the current version.  If !commit, then all
 *	changes made in the version will be undone, and the version will
 *	not become the current version.
 */

/***
 *** Node Methods
 ***/

#define dns_db_findnode(db, name, create, nodep) \
	dns__db_findnode(db, name, create, nodep DNS__DB_FILELINE)
isc_result_t
dns__db_findnode(dns_db_t *db, const dns_name_t *name, bool create,
		 dns_dbnode_t **nodep DNS__DB_FLARG);

#define dns_db_findnodeext(db, name, create, methods, clientinfo, nodep) \
	dns__db_findnodeext(db, name, create, methods, clientinfo,       \
			    nodep DNS__DB_FILELINE)
isc_result_t
dns__db_findnodeext(dns_db_t *db, const dns_name_t *name, bool create,
		    dns_clientinfomethods_t *methods,
		    dns_clientinfo_t	    *clientinfo,
		    dns_dbnode_t **nodep     DNS__DB_FLARG);
/*%<
 * Find the node with name 'name'.
 *
 * dns_db_findnodeext() (findnode extended) also accepts parameters
 * 'methods' and 'clientinfo', which, when provided, enable the database to
 * retrieve information about the client from the caller, and modify its
 * response on the basis of that information.
 *
 * Notes:
 * \li	If 'create' is true and no node with name 'name' exists, then
 *	such a node will be created.
 *
 * \li	This routine is for finding or creating a node with the specified
 *	name.  There are no partial matches.  It is not suitable for use
 *	in building responses to ordinary DNS queries; clients which wish
 *	to do that should use dns_db_find() instead.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'name' is a valid, non-empty, absolute name.
 *
 * \li	nodep != NULL && *nodep == NULL
 *
 * Ensures:
 *
 * \li	On success, *nodep is attached to the node with name 'name'.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND			If !create and name not found.
 * \li	#ISC_R_NOMEMORY			Can only happen if create is true.
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

#define dns_db_find(db, name, version, type, options, now, nodep, foundname,  \
		    rdataset, sigrdataset)                                    \
	dns__db_find(db, name, version, type, options, now, nodep, foundname, \
		     rdataset, sigrdataset DNS__DB_FILELINE)
isc_result_t
dns__db_find(dns_db_t *db, const dns_name_t *name, dns_dbversion_t *version,
	     dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
	     dns_dbnode_t **nodep, dns_name_t *foundname,
	     dns_rdataset_t		*rdataset,
	     dns_rdataset_t *sigrdataset DNS__DB_FLARG);

#define dns_db_findext(db, name, version, type, options, now, nodep,          \
		       foundname, methods, clientinfo, rdataset, sigrdataset) \
	dns__db_findext(db, name, version, type, options, now, nodep,         \
			foundname, methods, clientinfo, rdataset,             \
			sigrdataset DNS__DB_FILELINE)
isc_result_t
dns__db_findext(dns_db_t *db, const dns_name_t *name, dns_dbversion_t *version,
		dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
		dns_dbnode_t **nodep, dns_name_t *foundname,
		dns_clientinfomethods_t *methods, dns_clientinfo_t *clientinfo,
		dns_rdataset_t		   *rdataset,
		dns_rdataset_t *sigrdataset DNS__DB_FLARG);
/*%<
 * Find the best match for 'name' and 'type' in version 'version' of 'db'.
 *
 * dns_db_findext() (find extended) also accepts parameters 'methods'
 * and 'clientinfo', which when provided enable the database to retrieve
 * information about the client from the caller, and modify its response
 * on the basis of this information.
 *
 * Notes:
 *
 * \li	If type == dns_rdataset_any, then rdataset will not be bound.
 *
 * \li	If 'options' does not have #DNS_DBFIND_GLUEOK set, then no glue will
 *	be returned.  For zone databases, glue is as defined in RFC2181.
 *	For cache databases, glue is any rdataset with a trust of
 *	dns_trust_glue.
 *
 * \li	If 'options' does not have #DNS_DBFIND_ADDITIONALOK set, then no
 *	additional records will be returned.  Only caches can have
 *	rdataset with trust dns_trust_additional.
 *
 * \li	If 'options' does not have #DNS_DBFIND_PENDINGOK set, then no
 *	pending data will be returned.  This option is only meaningful for
 *	cache databases.
 *
 * \li	If the #DNS_DBFIND_NOWILD option is set, then wildcard matching will
 *	be disabled.  This option is only meaningful for zone databases.
 *
 * \li  If the #DNS_DBFIND_NOZONECUT option is set, the database is
 *	assumed to contain no zone cuts above 'name'.  An implementation
 *	may therefore choose to search for a match beginning at 'name'
 *	rather than walking down the tree to check check for delegations.
 *	If #DNS_DBFIND_NOWILD is not set, wildcard matching will be
 *	attempted at each node starting at the direct ancestor of 'name'
 *	and working up to the zone origin.  This option is only meaningful
 *	when querying redirect zones.
 *
 * \li	If the DNS_DBFIND_COVERINGNSEC option is set, then look for a
 *	NSEC record that potentially covers 'name' if a answer cannot
 *	be found.  Note the returned NSEC needs to be checked to ensure
 *	that it is correct.  This only affects answers returned from the
 *	cache.
 *
 * \li	If the #DNS_DBFIND_FORCENSEC3 option is set, then we are looking
 *	in the NSEC3 tree and not the main tree.  Without this option being
 *	set NSEC3 records will not be found.
 *
 * \li	To respond to a query for SIG records, the caller should create a
 *	rdataset iterator and extract the signatures from each rdataset.
 *
 * \li	Making queries of type ANY with #DNS_DBFIND_GLUEOK is not recommended,
 *	because the burden of determining whether a given rdataset is valid
 *	glue or not falls upon the caller.
 *
 * \li	The 'now' field is ignored if 'db' is a zone database.  If 'db' is a
 *	cache database, an rdataset will not be found unless it expires after
 *	'now'.  Any ANY query will not match unless at least one rdataset at
 *	the node expires after 'now'.  If 'now' is zero, then the current time
 *	will be used.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'type' is not SIG, or a meta-RR type other than 'ANY' (e.g. 'OPT').
 *
 * \li	'nodep' is NULL, or nodep is a valid pointer and *nodep == NULL.
 *
 * \li	'foundname' is a valid name with a dedicated buffer.
 *
 * \li	'rdataset' is NULL, or is a valid unassociated rdataset.
 *
 * Ensures,
 *	on a non-error completion:
 *
 *	\li	If nodep != NULL, then it is bound to the found node.
 *
 *	\li	If foundname != NULL, then it contains the full name of the
 *		found node.
 *
 *	\li	If rdataset != NULL and type != dns_rdatatype_any, then
 *		rdataset is bound to the found rdataset.
 *
 *	Non-error results are:
 *
 *	\li	#ISC_R_SUCCESS			The desired node and type were
 *						found.
 *
 *	\li	#DNS_R_GLUE			The desired node and type were
 *						found, but are glue.  This
 *						result can only occur if
 *						the DNS_DBFIND_GLUEOK option
 *						is set.  This result can only
 *						occur if 'db' is a zone
 *						database.  If type ==
 *						dns_rdatatype_any, then the
 *						node returned may contain, or
 *						consist entirely of invalid
 *						glue (i.e. data occluded by a
 *						zone cut).  The caller must
 *						take care not to return invalid
 *						glue to a client.
 *
 *	\li	#DNS_R_DELEGATION		The data requested is beneath
 *						a zone cut.  node, foundname,
 *						and rdataset reference the
 *						NS RRset of the zone cut.
 *						If 'db' is a cache database,
 *						then this is the deepest known
 *						delegation.
 *
 *	\li	#DNS_R_ZONECUT			type == dns_rdatatype_any, and
 *						the desired node is a zonecut.
 *						The caller must take care not
 *						to return inappropriate glue
 *						to a client.  This result can
 *						only occur if 'db' is a zone
 *						database and DNS_DBFIND_GLUEOK
 *						is set.
 *
 *	\li	#DNS_R_DNAME			The data requested is beneath
 *						a DNAME.  node, foundname,
 *						and rdataset reference the
 *						DNAME RRset.
 *
 *	\li	#DNS_R_CNAME			The rdataset requested was not
 *						found, but there is a CNAME
 *						at the desired name.  node,
 *						foundname, and rdataset
 *						reference the CNAME RRset.
 *
 *	\li	#DNS_R_NXDOMAIN			The desired name does not
 *						exist.
 *
 *	\li	#DNS_R_NXRRSET			The desired name exists, but
 *						the desired type does not.
 *
 *	\li	#ISC_R_NOTFOUND			The desired name does not
 *						exist, and no delegation could
 *						be found.  This result can only
 *						occur if 'db' is a cache
 *						database.  The caller should
 *						use its nameserver(s) of last
 *						resort (e.g. root hints).
 *
 *	\li	#DNS_R_NCACHENXDOMAIN		The desired name does not
 *						exist.  'node' is bound to the
 *						cache node with the desired
 *						name, and 'rdataset' contains
 *						the negative caching proof.
 *
 *	\li	#DNS_R_NCACHENXRRSET		The desired type does not
 *						exist.  'node' is bound to the
 *						cache node with the desired
 *						name, and 'rdataset' contains
 *						the negative caching proof.
 *
 *	\li	#DNS_R_EMPTYNAME		The name exists but there is
 *						no data at the name.
 *
 *	\li	#DNS_R_COVERINGNSEC		The returned data is a NSEC
 *						that potentially covers 'name'.
 *
 *	\li	#DNS_R_EMPTYWILD		The name is a wildcard without
 *						resource records.
 *
 *	Error results:
 *
 *	\li	#ISC_R_NOMEMORY
 *
 *	\li	#DNS_R_BADDB			Data that is required to be
 *						present in the DB, e.g. an NSEC
 *						record in a secure zone, is not
 *						present.
 *
 *	\li	Other results are possible, and should all be treated as
 *		errors.
 */

#define dns_db_findzonecut(db, name, options, now, nodep, foundname, dcname,  \
			   rdataset, sigrdataset)                             \
	dns__db_findzonecut(db, name, options, now, nodep, foundname, dcname, \
			    rdataset, sigrdataset DNS__DB_FILELINE)
isc_result_t
dns__db_findzonecut(dns_db_t *db, const dns_name_t *name, unsigned int options,
		    isc_stdtime_t now, dns_dbnode_t **nodep,
		    dns_name_t *foundname, dns_name_t *dcname,
		    dns_rdataset_t	       *rdataset,
		    dns_rdataset_t *sigrdataset DNS__DB_FLARG);
/*%<
 * Find the deepest known zonecut which encloses 'name' in 'db'.
 *
 * Notes:
 *
 * \li	If the #DNS_DBFIND_NOEXACT option is set, then the zonecut returned
 *	(if any) will be the deepest known ancestor of 'name'.
 *
 * \li	If 'now' is zero, then the current time will be used.
 *
 * Requires:
 *
 * \li	'db' is a valid database with cache semantics.
 *
 * \li	'nodep' is NULL, or nodep is a valid pointer and *nodep == NULL.
 *
 * \li	'foundname' is a valid name with a dedicated buffer.
 *
 * \li	'dcname' is a valid name with a dedicated buffer.
 *
 * \li	'rdataset' is NULL, or is a valid unassociated rdataset.
 *
 * Ensures, on a non-error completion:
 *
 * \li	If nodep != NULL, then it is bound to the found node.
 *
 * \li	If foundname != NULL, then it contains the full name of the
 *	found node.
 *
 * \li	If dcname != NULL, then it contains the deepest cached name
 *      that exists in the database.
 *
 * \li	If rdataset != NULL and type != dns_rdatatype_any, then
 *	rdataset is bound to the found rdataset.
 *
 * Non-error results are:
 *
 * \li	#ISC_R_SUCCESS
 *
 * \li	#ISC_R_NOTFOUND
 *
 * \li	Other results are possible, and should all be treated as
 *	errors.
 */

#define dns_db_attachnode(db, source, targetp) \
	dns__db_attachnode(db, source, targetp DNS__DB_FILELINE)
void
dns__db_attachnode(dns_db_t *db, dns_dbnode_t *source,
		   dns_dbnode_t **targetp DNS__DB_FLARG);
/*%<
 * Attach *targetp to source.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'source' is a valid node.
 *
 * \li	'targetp' points to a NULL dns_dbnode_t *.
 *
 * Ensures:
 *
 * \li	*targetp is attached to source.
 */

#define dns_db_detachnode(db, nodep) \
	dns__db_detachnode(db, nodep DNS__DB_FILELINE)
void
dns__db_detachnode(dns_db_t *db, dns_dbnode_t **nodep DNS__DB_FLARG);
/*%<
 * Detach *nodep from its node.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'nodep' points to a valid node.
 *
 * Ensures:
 *
 * \li	*nodep is NULL.
 */

void
dns_db_transfernode(dns_db_t *db, dns_dbnode_t **sourcep,
		    dns_dbnode_t **targetp);
/*%<
 * Transfer a node between pointer.
 *
 * This is equivalent to calling dns_db_attachnode() then dns_db_detachnode().
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'*sourcep' is a valid node.
 *
 * \li	'targetp' points to a NULL dns_dbnode_t *.
 *
 * Ensures:
 *
 * \li	'*sourcep' is NULL.
 */

void
dns_db_printnode(dns_db_t *db, dns_dbnode_t *node, FILE *out);
/*%<
 * Print a textual representation of the contents of the node to
 * 'out'.
 *
 * Note: this function is intended for debugging, not general use.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'node' is a valid node.
 */

/***
 *** DB Iterator Creation
 ***/

isc_result_t
dns_db_createiterator(dns_db_t *db, unsigned int options,
		      dns_dbiterator_t **iteratorp);
/*%<
 * Create an iterator for 'db'.
 *
 * Notes:
 *
 * \li	One or more of the following options can be set:
 *
 *	#DNS_DB_RELATIVENAMES
 *	#DNS_DB_NSEC3ONLY
 *	#DNS_DB_NONSEC3
 *
 *	(Note that it is not mandatory to implement these flags;
 *	some databases will ignore them.)
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	iteratorp != NULL && *iteratorp == NULL
 *
 * \li	'flags' contains at most one of #DNS_DB_NSEC3ONLY and #DNS_DB_NONSEC3.
 *
 * Ensures:
 *
 * \li	On success, *iteratorp will be a valid database iterator.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 */

/***
 *** Rdataset Methods
 ***/

/*
 * XXXRTH  Should we check for glue and pending data in dns_db_findrdataset()?
 */

#define dns_db_findrdataset(db, node, version, type, covers, now, rdataset,  \
			    sigrdataset)                                     \
	dns__db_findrdataset(db, node, version, type, covers, now, rdataset, \
			     sigrdataset DNS__DB_FILELINE)
isc_result_t
dns__db_findrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		     dns_rdatatype_t type, dns_rdatatype_t covers,
		     isc_stdtime_t now, dns_rdataset_t *rdataset,
		     dns_rdataset_t *sigrdataset DNS__DB_FLARG);

/*%<
 * Search for an rdataset of type 'type' at 'node' that are in version
 * 'version' of 'db'.  If found, make 'rdataset' refer to it.
 *
 * Notes:
 *
 * \li	If 'version' is NULL, then the current version will be used.
 *
 * \li	Care must be used when using this routine to build a DNS response:
 *	'node' should have been found with dns_db_find(), not
 *	dns_db_findnode().  No glue checking is done.  No checking for
 *	pending data is done.
 *
 * \li	The 'now' field is ignored if 'db' is a zone database.  If 'db' is a
 *	cache database, an rdataset will not be found unless it expires after
 *	'now'.  If 'now' is zero, then the current time will be used.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'node' is a valid node.
 *
 * \li	'rdataset' is a valid, disassociated rdataset.
 *
 * \li	'sigrdataset' is a valid, disassociated rdataset, or it is NULL.
 *
 * \li	If 'covers' != 0, 'type' must be RRSIG.
 *
 * \li	'type' is not a meta-RR type such as 'ANY' or 'OPT'.
 *
 * Ensures:
 *
 * \li	On success, 'rdataset' is associated with the found rdataset.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

#define dns_db_allrdatasets(db, node, version, options, now, iteratorp) \
	dns__db_allrdatasets(db, node, version, options, now,           \
			     iteratorp DNS__DB_FILELINE)
isc_result_t
dns__db_allrdatasets(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		     unsigned int options, isc_stdtime_t now,
		     dns_rdatasetiter_t **iteratorp DNS__DB_FLARG);
/*%<
 * Make '*iteratorp' an rdataset iterator for all rdatasets at 'node' in
 * version 'version' of 'db'.
 *
 * Notes:
 *
 * \li	If 'version' is NULL, then the current version will be used.
 *
 * \li	'options' controls which rdatasets are selected when interating over
 *	the node.
 *	'DNS_DB_STALEOK' return stale rdatasets as well as current rdatasets.
 *	'DNS_DB_EXPIREDOK' return expired rdatasets as well as current
 *	rdatasets.
 *
 * \li	The 'now' field is ignored if 'db' is a zone database.  If 'db' is a
 *	cache database, an rdataset will not be found unless it expires after
 *	'now'.  Any ANY query will not match unless at least one rdataset at
 *	the node expires after 'now'.  If 'now' is zero, then the current time
 *	will be used.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'node' is a valid node.
 *
 * \li	iteratorp != NULL && *iteratorp == NULL
 *
 * Ensures:
 *
 * \li	On success, '*iteratorp' is a valid rdataset iterator.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

#define dns_db_addrdataset(db, node, version, now, rdataset, options,  \
			   addedrdataset)                              \
	dns__db_addrdataset(db, node, version, now, rdataset, options, \
			    addedrdataset DNS__DB_FILELINE)
isc_result_t
dns__db_addrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		    isc_stdtime_t now, dns_rdataset_t *rdataset,
		    unsigned int		  options,
		    dns_rdataset_t *addedrdataset DNS__DB_FLARG);
/*%<
 * Add 'rdataset' to 'node' in version 'version' of 'db'.
 *
 * Notes:
 *
 * \li	If the database has zone semantics, the #DNS_DBADD_MERGE option is set,
 *	and an rdataset of the same type as 'rdataset' already exists at
 *	'node' then the contents of 'rdataset' will be merged with the existing
 *	rdataset.  If the option is not set, then rdataset will replace any
 *	existing rdataset of the same type.  If not merging and the
 *	#DNS_DBADD_FORCE option is set, then the data will update the database
 *	without regard to trust levels.  If not forcing the data, then the
 *	rdataset will only be added if its trust level is >= the trust level of
 *	any existing rdataset.  Forcing is only meaningful for cache databases.
 *	If #DNS_DBADD_EXACT is set then there must be no rdata in common between
 *	the old and new rdata sets.  If #DNS_DBADD_EXACTTTL is set then both
 *	the old and new rdata sets must have the same ttl.
 *
 * \li	The 'now' field is ignored if 'db' is a zone database.  If 'db' is
 *	a cache database, then the added rdataset will expire no later than
 *	now + rdataset->ttl.
 *
 * \li	If 'addedrdataset' is not NULL, then it will be attached to the
 *	resulting new rdataset in the database, or to the existing data if
 *	the existing data was better.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'node' is a valid node.
 *
 * \li	'rdataset' is a valid, associated rdataset with the same class
 *	as 'db'.
 *
 * \li	'addedrdataset' is NULL, or a valid, unassociated rdataset.
 *
 * \li	The database has zone semantics and 'version' is a valid
 *	read-write version, or the database has cache semantics
 *	and version is NULL.
 *
 * \li	If the database has cache semantics, the #DNS_DBADD_MERGE option must
 *	not be set.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#DNS_R_UNCHANGED			The operation did not change
 * anything. \li	#ISC_R_NOMEMORY \li	#DNS_R_NOTEXACT
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

#define dns_db_subtractrdataset(db, node, version, rdataset, options,  \
				newrdataset)                           \
	dns__db_subtractrdataset(db, node, version, rdataset, options, \
				 newrdataset DNS__DB_FILELINE)
isc_result_t
dns__db_subtractrdataset(dns_db_t *db, dns_dbnode_t *node,
			 dns_dbversion_t *version, dns_rdataset_t *rdataset,
			 unsigned int		     options,
			 dns_rdataset_t *newrdataset DNS__DB_FLARG);
/*%<
 * Remove any rdata in 'rdataset' from 'node' in version 'version' of
 * 'db'.
 *
 * Notes:
 *
 * \li	If 'newrdataset' is not NULL, then it will be attached to the
 *	resulting new rdataset in the database, unless the rdataset has
 *	become nonexistent.  If DNS_DBSUB_EXACT is set then all elements
 *	of 'rdataset' must exist at 'node'.
 *
 *\li	If DNS_DBSUB_WANTOLD is set and the entire rdataset was deleted
 *	then return the original rdatatset in newrdataset if that existed.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'node' is a valid node.
 *
 * \li	'rdataset' is a valid, associated rdataset with the same class
 *	as 'db'.
 *
 * \li	'newrdataset' is NULL, or a valid, unassociated rdataset.
 *
 * \li	The database has zone semantics and 'version' is a valid
 *	read-write version.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#DNS_R_UNCHANGED			The operation did not change
 * anything. \li	#DNS_R_NXRRSET			All rdata of the same
 *type as
 * those in 'rdataset' have been deleted. \li	#DNS_R_NOTEXACT
 * Some part of 'rdataset' did not exist and DNS_DBSUB_EXACT was set.
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

#define dns_db_deleterdataset(db, node, version, type, covers) \
	dns__db_deleterdataset(db, node, version, type, covers DNS__DB_FILELINE)
isc_result_t
dns__db_deleterdataset(dns_db_t *db, dns_dbnode_t *node,
		       dns_dbversion_t *version, dns_rdatatype_t type,
		       dns_rdatatype_t covers DNS__DB_FLARG);
/*%<
 * Make it so that no rdataset of type 'type' exists at 'node' in version
 * version 'version' of 'db'.
 *
 * Notes:
 *
 * \li	If 'type' is dns_rdatatype_any, then no rdatasets will exist in
 *	'version' (provided that the dns_db_deleterdataset() isn't followed
 *	by one or more dns_db_addrdataset() calls).
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'node' is a valid node.
 *
 * \li	The database has zone semantics and 'version' is a valid
 *	read-write version, or the database has cache semantics
 *	and version is NULL.
 *
 * \li	'type' is not a meta-RR type, except for dns_rdatatype_any, which is
 *	allowed.
 *
 * \li	If 'covers' != 0, 'type' must be SIG.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#DNS_R_UNCHANGED			No rdatasets of 'type' existed
 * before the operation was attempted.
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

isc_result_t
dns_db_getsoaserial(dns_db_t *db, dns_dbversion_t *ver, uint32_t *serialp);
/*%<
 * Get the current SOA serial number from a zone database.
 *
 * Requires:
 * \li	'db' is a valid database with zone semantics.
 * \li	'ver' is a valid version.
 */

unsigned int
dns_db_nodecount(dns_db_t *db, dns_dbtree_t tree);
/*%<
 * Count the number of nodes in 'db' or its auxiliary trees.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 * \li	The number of nodes in the database
 */

size_t
dns_db_hashsize(dns_db_t *db);
/*%<
 * For database implementations using a hash table, report the
 * current number of buckets.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 * \li	The number of buckets in the database's hash table, or
 *      0 if not implemented.
 */

void
dns_db_setloop(dns_db_t *db, isc_loop_t *loop);
/*%<
 * If loop is set then the final detach may be performed asynchronously.
 *
 * Requires:
 * \li	'db' is a valid database.
 * \li	'loop' to be valid or NULL.
 */

bool
dns_db_ispersistent(dns_db_t *db);
/*%<
 * Is 'db' persistent?  A persistent database does not need to be loaded
 * from disk or written to disk.
 *
 * By default, return false if the database implementation has a
 * 'beginload' function and true if it doesn't.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 * \li	#true	'db' is persistent.
 * \li	#false	'db' is not persistent.
 */

isc_result_t
dns_db_register(const char *name, dns_dbcreatefunc_t create, void *driverarg,
		isc_mem_t *mctx, dns_dbimplementation_t **dbimp);

/*%<
 * Register a new database implementation and add it to the list of
 * supported implementations.
 *
 * Requires:
 *
 * \li 	'name' is not NULL
 * \li	'order' is a valid function pointer
 * \li	'mctx' is a valid memory context
 * \li	dbimp != NULL && *dbimp == NULL
 *
 * Returns:
 * \li	#ISC_R_SUCCESS	The registration succeeded
 * \li	#ISC_R_NOMEMORY	Out of memory
 * \li	#ISC_R_EXISTS	A database implementation with the same name exists
 *
 * Ensures:
 *
 * \li	*dbimp points to an opaque structure which must be passed to
 *	dns_db_unregister().
 */

void
dns_db_unregister(dns_dbimplementation_t **dbimp);
/*%<
 * Remove a database implementation from the list of supported
 * implementations.  No databases of this type can be active when this
 * is called.
 *
 * Requires:
 * \li 	dbimp != NULL && *dbimp == NULL
 *
 * Ensures:
 *
 * \li	Any memory allocated in *dbimp will be freed.
 */

#define dns_db_getoriginnode(db, nodep) \
	dns__db_getoriginnode(db, nodep DNS__DB_FILELINE)
isc_result_t
dns__db_getoriginnode(dns_db_t *db, dns_dbnode_t **nodep DNS__DB_FLARG);
/*%<
 * Get the origin DB node corresponding to the DB's zone.  This function
 * should typically succeed unless the underlying DB implementation doesn't
 * support the feature.
 *
 * Requires:
 *
 * \li	'db' is a valid zone database.
 * \li	'nodep' != NULL && '*nodep' == NULL
 *
 * Ensures:
 * \li	On success, '*nodep' will point to the DB node of the zone's origin.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND - the DB implementation does not support this feature.
 */

isc_result_t
dns_db_getnsec3parameters(dns_db_t *db, dns_dbversion_t *version,
			  dns_hash_t *hash, uint8_t *flags,
			  uint16_t *iterations, unsigned char *salt,
			  size_t *salt_length);
/*%<
 * Get the NSEC3 parameters that are associated with this zone.
 *
 * Requires:
 * \li	'db' is a valid zone database.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND - the DB implementation does not support this feature
 *			  or this zone does not have NSEC3 records.
 */

isc_result_t
dns_db_getsize(dns_db_t *db, dns_dbversion_t *version, uint64_t *records,
	       uint64_t *xfrsize);
/*%<
 * On success if 'records' is not NULL, it is set to the number of records
 * in the given version of the database. If 'xfrisize' is not NULL, it is
 * set to the approximate number of bytes needed to transfer the records,
 * counting name, TTL, type, class, and rdata for each RR.  (This is meant
 * to be a rough approximation of the size of a full zone transfer, though
 * it does not take into account DNS message overhead or name compression.)
 *
 * Requires:
 * \li	'db' is a valid zone database.
 * \li	'version' is NULL or a valid version.
 * \li	'records' is NULL or a pointer to return the record count in.
 * \li	'xfrsize' is NULL or a pointer to return the byte count in.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTIMPLEMENTED
 */

#define dns_db_findnsec3node(db, name, create, nodep) \
	dns__db_findnsec3node(db, name, create, nodep DNS__DB_FILELINE)
isc_result_t
dns__db_findnsec3node(dns_db_t *db, const dns_name_t *name, bool create,
		      dns_dbnode_t **nodep DNS__DB_FLARG);
/*%<
 * Find the NSEC3 node with name 'name'.
 *
 * Notes:
 * \li	If 'create' is true and no node with name 'name' exists, then
 *	such a node will be created.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'name' is a valid, non-empty, absolute name.
 *
 * \li	nodep != NULL && *nodep == NULL
 *
 * Ensures:
 *
 * \li	On success, *nodep is attached to the node with name 'name'.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND			If !create and name not found.
 * \li	#ISC_R_NOMEMORY			Can only happen if create is true.
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

isc_result_t
dns_db_setsigningtime(dns_db_t *db, dns_rdataset_t *rdataset,
		      isc_stdtime_t resign);
/*%<
 * Sets the re-signing time associated with 'rdataset' to 'resign'.
 *
 * Requires:
 * \li	'db' is a valid zone database.
 * \li	'rdataset' is or is to be associated with 'db'.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 * \li	#ISC_R_NOTIMPLEMENTED - Not supported by this DB implementation.
 */

isc_result_t
dns_db_getsigningtime(dns_db_t *db, isc_stdtime_t *resign,
		      dns_name_t *foundname, dns_typepair_t *typepair);
/*%<
 * Find the rdataset header with the earliest signing time in a zone
 * database. Update 'foundname' and 'typepair' with its name and
 * type, and update 'resign' with the time at which it is to be signed.
 *
 * Requires:
 * \li	'db' is a valid zone database.
 * \li	'resign' is not NULL.
 * \li	'foundname' is not NULL.
 * \li	'typepair' is not NULL.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND - No dataset exists.
 */

dns_stats_t *
dns_db_getrrsetstats(dns_db_t *db);
/*%<
 * Get statistics information counting RRsets stored in the DB, when available.
 * The statistics may not be available depending on the DB implementation.
 *
 * Requires:
 *
 * \li	'db' is a valid database (cache only).
 *
 * Returns:
 * \li	when available, a pointer to a statistics object created by
 *	dns_rdatasetstats_create(); otherwise NULL.
 */

isc_result_t
dns_db_setcachestats(dns_db_t *db, isc_stats_t *stats);
/*%<
 * Set the location in which to collect cache statistics.
 * This option may not exist depending on the DB implementation.
 *
 * Requires:
 *
 * \li	'db' is a valid database (cache only).
 *
 * Returns:
 * \li	when available, a pointer to a statistics object created by
 *	dns_rdatasetstats_create(); otherwise NULL.
 */

void
dns_db_updatenotify_register(dns_db_t *db, dns_dbupdate_callback_t fn,
			     void *fn_arg);
/*%<
 * Register a notify-on-update callback function to a database.
 * Duplicate callbacks are suppressed.
 *
 * Requires:
 *
 * \li	'db' is a valid database
 * \li	'fn' is not NULL
 *
 */

void
dns_db_updatenotify_unregister(dns_db_t *db, dns_dbupdate_callback_t fn,
			       void *fn_arg);
/*%<
 * Unregister a notify-on-update callback.
 *
 * Requires:
 *
 * \li	'db' is a valid database
 * \li	'db' has update callback registered
 *
 */

isc_result_t
dns_db_setservestalettl(dns_db_t *db, dns_ttl_t ttl);
/*%<
 * Sets the maximum length of time that cached answers may be retained
 * past their normal TTL. Default value for the library is 0, disabling
 * the use of stale data.
 *
 * Requires:
 * \li	'db' is a valid cache database.
 * \li	'ttl' is the number of seconds to retain data past its normal expiry.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTIMPLEMENTED - Not supported by this DB implementation.
 */

isc_result_t
dns_db_getservestalettl(dns_db_t *db, dns_ttl_t *ttl);
/*%<
 * Gets maximum length of time that cached answers may be kept past
 * normal TTL expiration.
 *
 * Requires:
 * \li	'db' is a valid cache database.
 * \li	'ttl' is the number of seconds to retain data past its normal expiry.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTIMPLEMENTED - Not supported by this DB implementation.
 */

isc_result_t
dns_db_setservestalerefresh(dns_db_t *db, uint32_t interval);
/*%<
 * Sets the length of time to wait before attempting to refresh a rrset
 * if a previous attempt in doing so has failed.
 * During this time window if stale rrset are available in cache they
 * will be directly returned to client.
 *
 * Requires:
 * \li	'db' is a valid cache database.
 * \li	'interval' is number of seconds before attempting to refresh data.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTIMPLEMENTED - Not supported by this DB implementation.
 */

isc_result_t
dns_db_getservestalerefresh(dns_db_t *db, uint32_t *interval);
/*%<
 * Gets the length of time in which stale answers are directly returned from
 * cache before attempting to refresh them, in case a previous attempt in
 * doing so has failed.
 *
 * Requires:
 * \li	'db' is a valid cache database.
 * \li	'interval' is number of seconds before attempting to refresh data.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTIMPLEMENTED - Not supported by this DB implementation.
 */

isc_result_t
dns_db_setgluecachestats(dns_db_t *db, isc_stats_t *stats);
/*%<
 * Set the location in which to collect glue cache statistics.
 * This option may not exist depending on the DB implementation.
 *
 * Requires:
 *
 * \li	'db' is a valid database (cache only).
 *
 * Returns:
 * \li	when available, a pointer to a statistics object created by
 *	dns_rdatasetstats_create(); otherwise NULL.
 */

void
dns_db_locknode(dns_db_t *db, dns_dbnode_t *node, isc_rwlocktype_t type);
void
dns_db_unlocknode(dns_db_t *db, dns_dbnode_t *node, isc_rwlocktype_t type);
/*%<
 * Lock/unlock a single node within a database so that data stored
 * there can be manipulated directly.
 */

isc_result_t
dns_db_addglue(dns_db_t *db, dns_dbversion_t *version, dns_rdataset_t *rdataset,
	       dns_message_t *msg);
/*%<
 * Add glue records for rdataset to the additional section of message in
 * 'msg'. 'rdataset' must be of type NS.
 *
 * Requires:
 * \li	'db' is a database with 'zone' semantics.
 * \li	'version' is the DB version.
 * \li	'rdataset' is a valid NS rdataset.
 * \li	'msg' is the DNS message to which the glue should be added.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOTIMPLEMENTED
 *\li	#ISC_R_FAILURE
 *\li	Any error that dns_rdata_additionaldata() can return.
 */

void
dns_db_expiredata(dns_db_t *db, dns_dbnode_t *node, void *data);
/*%<
 * Tell the database 'db' to mark a block of data 'data' stored at
 * node 'node' as expired.
 */

void
dns_db_deletedata(dns_db_t *db, dns_dbnode_t *node, void *data);
/*%<
 * Tell the database 'db' to prepare to delete the block of data 'data'
 * stored at node 'node. This may include, for example, removing the
 * data from an LRU list or a heap.
 */

isc_result_t
dns_db_nodefullname(dns_db_t *db, dns_dbnode_t *node, dns_name_t *name);
/*%<
 * Get the name associated with a database node.
 *
 * Requires:
 *
 * \li 'db' is a valid database
 * \li 'node' and 'name' are not NULL
 */

void
dns_db_setmaxrrperset(dns_db_t *db, uint32_t value);
/*%<
 * Set the maximum permissible number of RRs per RRset.
 *
 * If 'value' is nonzero, then any subsequent attempt to add an rdataset
 * with more than 'value' RRs will return ISC_R_TOOMANYRECORDS.
 */

void
dns_db_setmaxtypepername(dns_db_t *db, uint32_t value);
/*%<
 * Set the maximum permissible number of RR types per owner name.
 *
 * If 'value' is nonzero, and if there are already 'value' RR types
 * stored at a given node, then any subsequent attempt to add an rdataset
 * with a new RR type will return ISC_R_TOOMANYRECORDS.
 */
ISC_LANG_ENDDECLS
