/*	$NetBSD: rbtdb_p.h,v 1.1.1.1 2025/01/26 16:12:32 christos Exp $	*/

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

#include <isc/heap.h>
#include <isc/lang.h>
#include <isc/rwlock.h>
#include <isc/urcu.h>

#include <dns/nsec3.h>
#include <dns/rbt.h>
#include <dns/types.h>

#include "db_p.h" /* for db_nodelock_t */

/*%
 * Note that "impmagic" is not the first four bytes of the struct, so
 * ISC_MAGIC_VALID cannot be used.
 */
#define RBTDB_MAGIC ISC_MAGIC('R', 'B', 'D', '4')
#define VALID_RBTDB(rbtdb) \
	((rbtdb) != NULL && (rbtdb)->common.impmagic == RBTDB_MAGIC)

#define RBTDB_HEADERNODE(h) ((dns_rbtnode_t *)((h)->node))

/*
 * Allow clients with a virtual time of up to 5 minutes in the past to see
 * records that would have otherwise have expired.
 */
#define RBTDB_VIRTUAL 300

/*****
***** Module Info
*****/

/*! \file
 * \brief
 * DNS Red-Black Tree DB Implementation
 */

ISC_LANG_BEGINDECLS

typedef struct rbtdb_changed {
	dns_rbtnode_t *node;
	bool dirty;
	ISC_LINK(struct rbtdb_changed) link;
} rbtdb_changed_t;

typedef ISC_LIST(rbtdb_changed_t) rbtdb_changedlist_t;

struct dns_rbtdb_version {
	/* Not locked */
	uint32_t serial;
	dns_rbtdb_t *rbtdb;
	/*
	 * Protected in the refcount routines.
	 * XXXJT: should we change the lock policy based on the refcount
	 * performance?
	 */
	isc_refcount_t references;
	/* Locked by database lock. */
	bool writer;
	bool commit_ok;
	rbtdb_changedlist_t changed_list;
	dns_slabheaderlist_t resigned_list;
	ISC_LINK(dns_rbtdb_version_t) link;
	bool secure;
	bool havensec3;
	/* NSEC3 parameters */
	dns_hash_t hash;
	uint8_t flags;
	uint16_t iterations;
	uint8_t salt_length;
	unsigned char salt[DNS_NSEC3_SALTSIZE];

	/*
	 * records and xfrsize are covered by rwlock.
	 */
	isc_rwlock_t rwlock;
	uint64_t records;
	uint64_t xfrsize;

	struct cds_lfht *glue_table;
};

typedef ISC_LIST(dns_rbtdb_version_t) rbtdb_versionlist_t;

struct dns_rbtdb {
	/* Unlocked. */
	dns_db_t common;
	/* Locks the data in this struct */
	isc_rwlock_t lock;
	/* Locks the tree structure (prevents nodes appearing/disappearing) */
	isc_rwlock_t tree_lock;
	/* Locks for individual tree nodes */
	unsigned int node_lock_count;
	db_nodelock_t *node_locks;
	dns_rbtnode_t *origin_node;
	dns_rbtnode_t *nsec3_origin_node;
	dns_stats_t *rrsetstats;     /* cache DB only */
	isc_stats_t *cachestats;     /* cache DB only */
	isc_stats_t *gluecachestats; /* zone DB only */
	/* Locked by lock. */
	unsigned int active;
	unsigned int attributes;
	uint32_t current_serial;
	uint32_t least_serial;
	uint32_t next_serial;
	uint32_t maxrrperset;
	uint32_t maxtypepername;
	dns_rbtdb_version_t *current_version;
	dns_rbtdb_version_t *future_version;
	rbtdb_versionlist_t open_versions;
	isc_loop_t *loop;
	dns_dbnode_t *soanode;
	dns_dbnode_t *nsnode;

	/*
	 * The time after a failed lookup, where stale answers from cache
	 * may be used directly in a DNS response without attempting a
	 * new iterative lookup.
	 */
	uint32_t serve_stale_refresh;

	/*
	 * This is an array of linked lists used to implement the LRU cache.
	 * There will be node_lock_count linked lists here.  Nodes in bucket 1
	 * will be placed on the linked list lru[1].
	 */
	dns_slabheaderlist_t *lru;

	/*
	 * Start point % node_lock_count for next LRU cleanup.
	 */
	atomic_uint lru_sweep;

	/*
	 * When performing LRU cleaning limit cleaning to headers that were
	 * last used at or before this.
	 */
	_Atomic(isc_stdtime_t) last_used;

	/*%
	 * Temporary storage for stale cache nodes and dynamically deleted
	 * nodes that await being cleaned up.
	 */
	dns_rbtnodelist_t *deadnodes;

	/*
	 * Heaps.  These are used for TTL based expiry in a cache,
	 * or for zone resigning in a zone DB.  hmctx is the memory
	 * context to use for the heap (which differs from the main
	 * database memory context in the case of a cache).
	 */
	isc_mem_t *hmctx;
	isc_heap_t **heaps;
	isc_heapcompare_t sooner;

	/* Locked by tree_lock. */
	dns_rbt_t *tree;
	dns_rbt_t *nsec;
	dns_rbt_t *nsec3;

	/* Unlocked */
	unsigned int quantum;
};

/*%
 * Search Context
 */
typedef struct {
	dns_rbtdb_t *rbtdb;
	dns_rbtdb_version_t *rbtversion;
	uint32_t serial;
	unsigned int options;
	dns_rbtnodechain_t chain;
	bool copy_name;
	bool need_cleanup;
	bool wild;
	dns_rbtnode_t *zonecut;
	dns_slabheader_t *zonecut_header;
	dns_slabheader_t *zonecut_sigheader;
	dns_fixedname_t zonecut_name;
	isc_stdtime_t now;
} rbtdb_search_t;

/*%
 * Load Context
 */
typedef struct {
	dns_db_t *db;
	isc_stdtime_t now;
} rbtdb_load_t;

/*%
 * Prune context
 */
typedef struct {
	dns_db_t *db;
	dns_rbtnode_t *node;
} rbtdb_prune_t;

extern dns_dbmethods_t dns__rbtdb_zonemethods;
extern dns_dbmethods_t dns__rbtdb_cachemethods;

typedef struct dns_gluenode_t {
	isc_mem_t *mctx;

	struct dns_glue *glue;

	dns_db_t *db;
	dns_rbtnode_t *node;

	struct cds_lfht_node ht_node;
	struct rcu_head rcu_head;
} dns_gluenode_t;

/*
 * Common DB implementation methods shared by both cache and zone RBT
 * databases:
 */

isc_result_t
dns__rbtdb_create(isc_mem_t *mctx, const dns_name_t *base, dns_dbtype_t type,
		  dns_rdataclass_t rdclass, unsigned int argc, char *argv[],
		  void *driverarg, dns_db_t **dbp);
/*%<
 * Create a new database of type "rbt". Called via dns_db_create();
 * see documentation for that function for more details.
 *
 * If argv[0] is set, it points to a valid memory context to be used for
 * allocation of heap memory.  Generally this is used for cache databases
 * only.
 *
 * Requires:
 *
 * \li argc == 0 or argv[0] is a valid memory context.
 */

void
dns__rbtdb_destroy(dns_db_t *arg);
/*%<
 * Implement dns_db_destroy() for RBT databases, see documentation
 * for that function for more details.
 */

void
dns__rbtdb_currentversion(dns_db_t *db, dns_dbversion_t **versionp);
isc_result_t
dns__rbtdb_newversion(dns_db_t *db, dns_dbversion_t **versionp);
void
dns__rbtdb_attachversion(dns_db_t *db, dns_dbversion_t *source,
			 dns_dbversion_t **targetp);
void
dns__rbtdb_closeversion(dns_db_t *db, dns_dbversion_t **versionp,
			bool commit DNS__DB_FLARG);
/*%<
 * Implement the dns_db_currentversion(), _newversion(),
 * _attachversion() and _closeversion() methods for RBT databases;
 * see documentation of those functions for more details.
 */

isc_result_t
dns__rbtdb_findnode(dns_db_t *db, const dns_name_t *name, bool create,
		    dns_dbnode_t **nodep DNS__DB_FLARG);
isc_result_t
dns__rbtdb_findnodeintree(dns_rbtdb_t *rbtdb, dns_rbt_t *tree,
			  const dns_name_t *name, bool create,
			  dns_dbnode_t **nodep DNS__DB_FLARG);
/*%<
 * Implement the dns_db_findnode() and _findnodeintree() methods for
 * RBT databases; see documentation of those functions for more details.
 */

void
dns__rbtdb_attachnode(dns_db_t *db, dns_dbnode_t *source,
		      dns_dbnode_t **targetp DNS__DB_FLARG);
void
dns__rbtdb_detachnode(dns_db_t *db, dns_dbnode_t **targetp DNS__DB_FLARG);
/*%<
 * Implement the dns_db_attachnode() and _detachnode() methods for
 * RBT databases; see documentation of those functions for more details.
 */

isc_result_t
dns__rbtdb_createiterator(dns_db_t *db, unsigned int options,
			  dns_dbiterator_t **iteratorp);
/*%<
 * Implement dns_db_createiterator() for RBT databases; see documentation of
 * that function for more details.
 */

isc_result_t
dns__rbtdb_allrdatasets(dns_db_t *db, dns_dbnode_t *node,
			dns_dbversion_t *version, unsigned int options,
			isc_stdtime_t now,
			dns_rdatasetiter_t **iteratorp DNS__DB_FLARG);
/*%<
 * Implement dns_db_allrdatasets() for RBT databases; see documentation of
 * that function for more details.
 */
isc_result_t
dns__rbtdb_addrdataset(dns_db_t *db, dns_dbnode_t *node,
		       dns_dbversion_t *version, isc_stdtime_t now,
		       dns_rdataset_t *rdataset, unsigned int options,
		       dns_rdataset_t *addedrdataset DNS__DB_FLARG);
isc_result_t
dns__rbtdb_subtractrdataset(dns_db_t *db, dns_dbnode_t *node,
			    dns_dbversion_t *version, dns_rdataset_t *rdataset,
			    unsigned int options,
			    dns_rdataset_t *newrdataset DNS__DB_FLARG);
isc_result_t
dns__rbtdb_deleterdataset(dns_db_t *db, dns_dbnode_t *node,
			  dns_dbversion_t *version, dns_rdatatype_t type,
			  dns_rdatatype_t covers DNS__DB_FLARG);
/*%<
 * Implement the dns_db_addrdataset(), _subtractrdataset() and
 * _deleterdataset() methods for RBT databases; see documentation of
 * those functions for more details.
 */

unsigned int
dns__rbtdb_nodecount(dns_db_t *db, dns_dbtree_t tree);
/*%<
 * Implement dns_db_nodecount() for RBT databases; see documentation of
 * that function for more details.
 */

void
dns__rbtdb_setloop(dns_db_t *db, isc_loop_t *loop);
/*%<
 * Implement dns_db_setloop() for RBT databases; see documentation of
 * that function for more details.
 */

isc_result_t
dns__rbtdb_getoriginnode(dns_db_t *db, dns_dbnode_t **nodep DNS__DB_FLARG);
/*%<
 * Implement dns_db_getoriginnode() for RBT databases; see documentation of
 * that function for more details.
 */

void
dns__rbtdb_deletedata(dns_db_t *db ISC_ATTR_UNUSED,
		      dns_dbnode_t *node ISC_ATTR_UNUSED, void *data);
/*%<
 * Implement dns_db_deletedata() for RBT databases; see documentation of
 * that function for more details.
 */

void
dns__rbtdb_locknode(dns_db_t *db, dns_dbnode_t *node, isc_rwlocktype_t type);
void
dns__rbtdb_unlocknode(dns_db_t *db, dns_dbnode_t *node, isc_rwlocktype_t type);
/*%<
 * Implement the dns_db_locknode() and _unlocknode() methods for
 * RBT databases; see documentation of those functions for more details.
 */

/*%
 * Functions used for the RBT implementation which are defined and
 * used in rbtdb.c but may also be called from rbt-zonedb.c or
 * rbt-cachedb.c:
 */
void
dns__rbtdb_bindrdataset(dns_rbtdb_t *rbtdb, dns_rbtnode_t *node,
			dns_slabheader_t *header, isc_stdtime_t now,
			isc_rwlocktype_t locktype,
			dns_rdataset_t *rdataset DNS__DB_FLARG);

isc_result_t
dns__rbtdb_nodefullname(dns_db_t *db, dns_dbnode_t *node, dns_name_t *name);

void
dns__rbtdb_free_gluenode_rcu(struct rcu_head *rcu_head);
void
dns__rbtdb_free_gluenode(dns_gluenode_t *gluenode);

void
dns__rbtdb_newref(dns_rbtdb_t *rbtdb, dns_rbtnode_t *node,
		  isc_rwlocktype_t locktype DNS__DB_FLARG);
/*%<
 * Increment the reference counter to a node in an RBT database.
 * If the caller holds a node lock then its lock type is specified
 * as 'locktype'. If the node is write-locked, then the node can
 * be removed from the dead nodes list. If not, the list can be
 * cleaned up later.
 */

bool
dns__rbtdb_decref(dns_rbtdb_t *rbtdb, dns_rbtnode_t *node,
		  uint32_t least_serial, isc_rwlocktype_t *nlocktypep,
		  isc_rwlocktype_t *tlocktypep, bool tryupgrade,
		  bool pruning DNS__DB_FLARG);
/*%<
 * Decrement the reference counter to a node in an RBT database.
 * 'nlocktypep' and 'tlocktypep' are pointers to the current status
 * of the node lock and tree lock.
 *
 * If references go to 0, the node will be cleaned up, which may
 * necessitate upgrading the locks.
 */

isc_result_t
dns__rbtdb_add(dns_rbtdb_t *rbtdb, dns_rbtnode_t *rbtnode,
	       const dns_name_t *nodename, dns_rbtdb_version_t *rbtversion,
	       dns_slabheader_t *newheader, unsigned int options, bool loading,
	       dns_rdataset_t *addedrdataset, isc_stdtime_t now DNS__DB_FLARG);
/*%<
 * Add a slab header 'newheader' to a node in an RBT database.
 * The caller must have the node write-locked.
 */

void
dns__rbtdb_setsecure(dns_db_t *db, dns_rbtdb_version_t *version,
		     dns_dbnode_t *origin);
/*%<
 * Update the secure status for an RBT database version 'version'.
 * The version will be marked secure if it is fully signed and
 * and contains a complete NSEC/NSEC3 chain.
 */

void
dns__rbtdb_mark(dns_slabheader_t *header, uint_least16_t flag);
/*%<
 * Set attribute 'flag' in a slab header 'header' - for example,
 * DNS_SLABHEADERATTR_STALE or DNS_SLABHEADERATTR_ANCIENT - and,
 * in a cache database, update the rrset stats accordingly.
 */

void
dns__rbtdb_setttl(dns_slabheader_t *header, dns_ttl_t newttl);
/*%<
 * Set the TTL in a slab header 'header'. In a cache database,
 * also update the TTL heap accordingly.
 */

void
dns__rbtdb_setmaxrrperset(dns_db_t *db, uint32_t maxrrperset);
/*%<
 * Set the max RRs per RRset limit.
 */

void
dns__rbtdb_setmaxtypepername(dns_db_t *db, uint32_t maxtypepername);
/*%<
 * Set the max RRs per RRset limit.
 */

/*
 * Functions specific to zone databases that are also called from rbtdb.c.
 */
void
dns__zonerbt_resigninsert(dns_rbtdb_t *rbtdb, int idx,
			  dns_slabheader_t *newheader);
void
dns__zonerbt_resigndelete(dns_rbtdb_t *rbtdb, dns_rbtdb_version_t *version,
			  dns_slabheader_t *header DNS__DB_FLARG);
/*%<
 * Insert/delete a node from the zone database's resigning heap.
 */

isc_result_t
dns__zonerbt_wildcardmagic(dns_rbtdb_t *rbtdb, const dns_name_t *name,
			   bool lock);
/*%<
 * Add the necessary magic for the wildcard name 'name'
 * to be found in 'rbtdb'.
 *
 * In order for wildcard matching to work correctly in
 * zone_find(), we must ensure that a node for the wildcarding
 * level exists in the database, and has its 'find_callback'
 * and 'wild' bits set.
 *
 * E.g. if the wildcard name is "*.sub.example." then we
 * must ensure that "sub.example." exists and is marked as
 * a wildcard level.
 *
 * The tree must be write-locked.
 */
isc_result_t
dns__zonerbt_addwildcards(dns_rbtdb_t *rbtdb, const dns_name_t *name,
			  bool lock);
/*%<
 * If 'name' is or contains a wildcard name, create a node for it in the
 * database. The tree must be write-locked.
 */

/*
 * Cache-specific functions that are called from rbtdb.c
 */
void
dns__cacherbt_expireheader(dns_slabheader_t *header,
			   isc_rwlocktype_t *tlocktypep,
			   dns_expire_t reason DNS__DB_FLARG);
void
dns__cacherbt_overmem(dns_rbtdb_t *rbtdb, dns_slabheader_t *newheader,
		      isc_rwlocktype_t *tlocktypep DNS__DB_FLARG);

ISC_LANG_ENDDECLS
