/*	$NetBSD: db_p.h,v 1.3 2025/05/21 14:48:02 christos Exp $	*/

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
#include <isc/urcu.h>

#include <dns/nsec3.h>
#include <dns/rbt.h>
#include <dns/types.h>

#define RDATATYPE_NCACHEANY DNS_TYPEPAIR_VALUE(0, dns_rdatatype_any)

#ifdef STRONG_RWLOCK_CHECK
#define STRONG_RWLOCK_CHECK(cond) REQUIRE(cond)
#else
#define STRONG_RWLOCK_CHECK(cond)
#endif

#define NODE_INITLOCK(l)    isc_rwlock_init((l))
#define NODE_DESTROYLOCK(l) isc_rwlock_destroy(l)
#define NODE_LOCK(l, t, tp)                                      \
	{                                                        \
		STRONG_RWLOCK_CHECK(*tp == isc_rwlocktype_none); \
		RWLOCK((l), (t));                                \
		*tp = t;                                         \
	}
#define NODE_UNLOCK(l, tp)                                       \
	{                                                        \
		STRONG_RWLOCK_CHECK(*tp != isc_rwlocktype_none); \
		RWUNLOCK(l, *tp);                                \
		*tp = isc_rwlocktype_none;                       \
	}
#define NODE_RDLOCK(l, tp) NODE_LOCK(l, isc_rwlocktype_read, tp);
#define NODE_WRLOCK(l, tp) NODE_LOCK(l, isc_rwlocktype_write, tp);
#define NODE_TRYLOCK(l, t, tp)                                   \
	({                                                       \
		STRONG_RWLOCK_CHECK(*tp == isc_rwlocktype_none); \
		isc_result_t _result = isc_rwlock_trylock(l, t); \
		if (_result == ISC_R_SUCCESS) {                  \
			*tp = t;                                 \
		};                                               \
		_result;                                         \
	})
#define NODE_TRYRDLOCK(l, tp) NODE_TRYLOCK(l, isc_rwlocktype_read, tp)
#define NODE_TRYWRLOCK(l, tp) NODE_TRYLOCK(l, isc_rwlocktype_write, tp)
#define NODE_TRYUPGRADE(l, tp)                                   \
	({                                                       \
		STRONG_RWLOCK_CHECK(*tp == isc_rwlocktype_read); \
		isc_result_t _result = isc_rwlock_tryupgrade(l); \
		if (_result == ISC_R_SUCCESS) {                  \
			*tp = isc_rwlocktype_write;              \
		};                                               \
		_result;                                         \
	})
#define NODE_FORCEUPGRADE(l, tp)                       \
	if (NODE_TRYUPGRADE(l, tp) != ISC_R_SUCCESS) { \
		NODE_UNLOCK(l, tp);                    \
		NODE_WRLOCK(l, tp);                    \
	}

#define TREE_INITLOCK(l)    isc_rwlock_init(l)
#define TREE_DESTROYLOCK(l) isc_rwlock_destroy(l)
#define TREE_LOCK(l, t, tp)                                      \
	{                                                        \
		STRONG_RWLOCK_CHECK(*tp == isc_rwlocktype_none); \
		RWLOCK(l, t);                                    \
		*tp = t;                                         \
	}
#define TREE_UNLOCK(l, tp)                                       \
	{                                                        \
		STRONG_RWLOCK_CHECK(*tp != isc_rwlocktype_none); \
		RWUNLOCK(l, *tp);                                \
		*tp = isc_rwlocktype_none;                       \
	}
#define TREE_RDLOCK(l, tp) TREE_LOCK(l, isc_rwlocktype_read, tp);
#define TREE_WRLOCK(l, tp) TREE_LOCK(l, isc_rwlocktype_write, tp);
#define TREE_TRYLOCK(l, t, tp)                                   \
	({                                                       \
		STRONG_RWLOCK_CHECK(*tp == isc_rwlocktype_none); \
		isc_result_t _result = isc_rwlock_trylock(l, t); \
		if (_result == ISC_R_SUCCESS) {                  \
			*tp = t;                                 \
		};                                               \
		_result;                                         \
	})
#define TREE_TRYRDLOCK(l, tp) TREE_TRYLOCK(l, isc_rwlocktype_read, tp)
#define TREE_TRYWRLOCK(l, tp) TREE_TRYLOCK(l, isc_rwlocktype_write, tp)
#define TREE_TRYUPGRADE(l, tp)                                   \
	({                                                       \
		STRONG_RWLOCK_CHECK(*tp == isc_rwlocktype_read); \
		isc_result_t _result = isc_rwlock_tryupgrade(l); \
		if (_result == ISC_R_SUCCESS) {                  \
			*tp = isc_rwlocktype_write;              \
		};                                               \
		_result;                                         \
	})
#define TREE_FORCEUPGRADE(l, tp)                       \
	if (TREE_TRYUPGRADE(l, tp) != ISC_R_SUCCESS) { \
		TREE_UNLOCK(l, tp);                    \
		TREE_WRLOCK(l, tp);                    \
	}

#define IS_STUB(db)  (((db)->common.attributes & DNS_DBATTR_STUB) != 0)
#define IS_CACHE(db) (((db)->common.attributes & DNS_DBATTR_CACHE) != 0)

ISC_LANG_BEGINDECLS

struct dns_glue {
	struct dns_glue *next;
	dns_name_t name;
	dns_rdataset_t rdataset_a;
	dns_rdataset_t sigrdataset_a;
	dns_rdataset_t rdataset_aaaa;
	dns_rdataset_t sigrdataset_aaaa;
};

struct dns_gluelist {
	isc_mem_t *mctx;

	const dns_dbversion_t *version;
	dns_slabheader_t *header;

	struct dns_glue *glue;

	struct rcu_head rcu_head;
	struct cds_wfs_node wfs_node;
};

typedef struct dns_glue_additionaldata_ctx {
	dns_db_t *db;
	dns_dbversion_t *version;
	dns_dbnode_t *node;

	dns_glue_t *glue;
} dns_glue_additionaldata_ctx_t;

typedef struct {
	isc_rwlock_t lock;
	/* Protected in the refcount routines. */
	isc_refcount_t references;
	/* Locked by lock. */
	bool exiting;
} db_nodelock_t;

static inline bool
prio_type(dns_typepair_t type) {
	switch (type) {
	case dns_rdatatype_soa:
	case DNS_SIGTYPE(dns_rdatatype_soa):
	case dns_rdatatype_a:
	case DNS_SIGTYPE(dns_rdatatype_a):
	case dns_rdatatype_mx:
	case DNS_SIGTYPE(dns_rdatatype_mx):
	case dns_rdatatype_aaaa:
	case DNS_SIGTYPE(dns_rdatatype_aaaa):
	case dns_rdatatype_nsec:
	case DNS_SIGTYPE(dns_rdatatype_nsec):
	case dns_rdatatype_nsec3:
	case DNS_SIGTYPE(dns_rdatatype_nsec3):
	case dns_rdatatype_ns:
	case DNS_SIGTYPE(dns_rdatatype_ns):
	case dns_rdatatype_ds:
	case DNS_SIGTYPE(dns_rdatatype_ds):
	case dns_rdatatype_cname:
	case DNS_SIGTYPE(dns_rdatatype_cname):
	case dns_rdatatype_dname:
	case DNS_SIGTYPE(dns_rdatatype_dname):
	case dns_rdatatype_svcb:
	case DNS_SIGTYPE(dns_rdatatype_svcb):
	case dns_rdatatype_https:
	case DNS_SIGTYPE(dns_rdatatype_https):
	case dns_rdatatype_dnskey:
	case DNS_SIGTYPE(dns_rdatatype_dnskey):
	case dns_rdatatype_srv:
	case DNS_SIGTYPE(dns_rdatatype_srv):
	case dns_rdatatype_txt:
	case DNS_SIGTYPE(dns_rdatatype_txt):
	case dns_rdatatype_ptr:
	case DNS_SIGTYPE(dns_rdatatype_ptr):
	case dns_rdatatype_naptr:
	case DNS_SIGTYPE(dns_rdatatype_naptr):
		return true;
	}
	return false;
}

void
dns__db_logtoomanyrecords(dns_db_t *db, const dns_name_t *name,
			  dns_rdatatype_t type, const char *op, uint32_t limit);
/*
 * Emit a log message when adding an rdataset of name/type would exceed the
 * 'maxrrperset' limit. 'op' is 'adding' or 'updating' depending on whether
 * the addition is to create a new rdataset or to merge to an existing one.
 */

void
dns__db_free_glue(isc_mem_t *mctx, dns_glue_t *glue);
void
dns__db_destroy_gluelist(dns_gluelist_t **gluelistp);
void
dns__db_free_gluelist_rcu(struct rcu_head *rcu_head);
void
dns__db_cleanup_gluelists(struct cds_wfs_stack *glue_stack);
isc_result_t
dns__db_addglue(dns_db_t *db, dns_dbversion_t *dbversion,
		dns_rdataset_t *rdataset, dns_message_t *msg,
		dns_additionaldatafunc_t add, struct cds_wfs_stack *glue_stack);

dns_glue_t *
dns__db_new_glue(isc_mem_t *mctx, const dns_name_t *name);

ISC_LANG_ENDDECLS
