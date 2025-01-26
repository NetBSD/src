/*	$NetBSD: badcache.c,v 1.9 2025/01/26 16:25:21 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>

#include <isc/async.h>
#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/log.h>
#include <isc/loop.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/rwlock.h>
#include <isc/spinlock.h>
#include <isc/stdtime.h>
#include <isc/string.h>
#include <isc/thread.h>
#include <isc/time.h>
#include <isc/urcu.h>
#include <isc/util.h>

#include <dns/badcache.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/rdatatype.h>
#include <dns/types.h>

typedef struct dns_bcentry dns_bcentry_t;

typedef struct dns_bckey {
	const dns_name_t *name;
	dns_rdatatype_t type;
} dns__bckey_t;

struct dns_badcache {
	unsigned int magic;
	isc_mem_t *mctx;
	struct cds_lfht *ht;
	struct cds_list_head *lru;
	uint32_t nloops;
};

#define BADCACHE_MAGIC	  ISC_MAGIC('B', 'd', 'C', 'a')
#define VALID_BADCACHE(m) ISC_MAGIC_VALID(m, BADCACHE_MAGIC)

#define BADCACHE_INIT_SIZE (1 << 10) /* Must be power of 2 */
#define BADCACHE_MIN_SIZE  (1 << 8)  /* Must be power of 2 */

struct dns_bcentry {
	isc_loop_t *loop;
	isc_stdtime_t expire;
	uint32_t flags;

	struct cds_lfht_node ht_node;
	struct rcu_head rcu_head;
	struct cds_list_head lru_head;

	dns_name_t name;
	dns_rdatatype_t type;
};

static void
bcentry_print(dns_bcentry_t *bad, isc_stdtime_t now, FILE *fp);

static void
bcentry_destroy(struct rcu_head *rcu_head);

static bool
bcentry_alive(struct cds_lfht *ht, dns_bcentry_t *bad, isc_stdtime_t now);

dns_badcache_t *
dns_badcache_new(isc_mem_t *mctx, isc_loopmgr_t *loopmgr) {
	REQUIRE(loopmgr != NULL);

	uint32_t nloops = isc_loopmgr_nloops(loopmgr);
	dns_badcache_t *bc = isc_mem_get(mctx, sizeof(*bc));
	*bc = (dns_badcache_t){
		.magic = BADCACHE_MAGIC,
		.nloops = nloops,
	};

	bc->ht = cds_lfht_new(BADCACHE_INIT_SIZE, BADCACHE_MIN_SIZE, 0,
			      CDS_LFHT_AUTO_RESIZE | CDS_LFHT_ACCOUNTING, NULL);
	INSIST(bc->ht != NULL);

	bc->lru = isc_mem_cget(mctx, bc->nloops, sizeof(bc->lru[0]));
	for (size_t i = 0; i < bc->nloops; i++) {
		CDS_INIT_LIST_HEAD(&bc->lru[i]);
	}

	isc_mem_attach(mctx, &bc->mctx);

	return bc;
}

void
dns_badcache_destroy(dns_badcache_t **bcp) {
	REQUIRE(bcp != NULL && *bcp != NULL);
	REQUIRE(VALID_BADCACHE(*bcp));

	dns_badcache_t *bc = *bcp;
	*bcp = NULL;
	bc->magic = 0;

	dns_bcentry_t *bad = NULL;
	struct cds_lfht_iter iter;
	cds_lfht_for_each_entry(bc->ht, &iter, bad, ht_node) {
		INSIST(!cds_lfht_del(bc->ht, &bad->ht_node));
		bcentry_destroy(&bad->rcu_head);
	}
	RUNTIME_CHECK(!cds_lfht_destroy(bc->ht, NULL));

	isc_mem_cput(bc->mctx, bc->lru, bc->nloops, sizeof(bc->lru[0]));

	isc_mem_putanddetach(&bc->mctx, bc, sizeof(dns_badcache_t));
}

static int
bcentry_match(struct cds_lfht_node *ht_node, const void *key0) {
	const dns__bckey_t *key = key0;
	dns_bcentry_t *bad = caa_container_of(ht_node, dns_bcentry_t, ht_node);

	return (bad->type == key->type) &&
	       dns_name_equal(&bad->name, key->name);
}

static uint32_t
bcentry_hash(const dns__bckey_t *key) {
	isc_hash32_t state;
	isc_hash32_init(&state);
	isc_hash32_hash(&state, key->name->ndata, key->name->length, false);
	isc_hash32_hash(&state, &key->type, sizeof(key->type), true);
	return isc_hash32_finalize(&state);
}

static dns_bcentry_t *
bcentry_lookup(struct cds_lfht *ht, uint32_t hashval, dns__bckey_t *key) {
	struct cds_lfht_iter iter;

	cds_lfht_lookup(ht, hashval, bcentry_match, key, &iter);

	return cds_lfht_entry(cds_lfht_iter_get_node(&iter), dns_bcentry_t,
			      ht_node);
}

static dns_bcentry_t *
bcentry_new(isc_loop_t *loop, const dns_name_t *name,
	    const dns_rdatatype_t type, const uint32_t flags,
	    const isc_stdtime_t expire) {
	isc_mem_t *mctx = isc_loop_getmctx(loop);
	dns_bcentry_t *bad = isc_mem_get(mctx, sizeof(*bad));
	*bad = (dns_bcentry_t){
		.type = type,
		.flags = flags,
		.expire = expire,
		.loop = isc_loop_ref(loop),
		.lru_head = CDS_LIST_HEAD_INIT(bad->lru_head),
	};

	dns_name_init(&bad->name, NULL);
	dns_name_dup(name, mctx, &bad->name);

	return bad;
}

static void
bcentry_destroy(struct rcu_head *rcu_head) {
	dns_bcentry_t *bad = caa_container_of(rcu_head, dns_bcentry_t,
					      rcu_head);
	isc_loop_t *loop = bad->loop;
	isc_mem_t *mctx = isc_loop_getmctx(loop);

	dns_name_free(&bad->name, mctx);
	isc_mem_put(mctx, bad, sizeof(*bad));

	isc_loop_unref(loop);
}

static void
bcentry_evict_async(void *arg) {
	dns_bcentry_t *bad = arg;

	RUNTIME_CHECK(bad->loop == isc_loop());

	cds_list_del(&bad->lru_head);
	call_rcu(&bad->rcu_head, bcentry_destroy);
}

static void
bcentry_evict(struct cds_lfht *ht, dns_bcentry_t *bad) {
	if (!cds_lfht_del(ht, &bad->ht_node)) {
		if (bad->loop == isc_loop()) {
			bcentry_evict_async(bad);
			return;
		}

		isc_async_run(bad->loop, bcentry_evict_async, bad);
	}
}

static bool
bcentry_alive(struct cds_lfht *ht, dns_bcentry_t *bad, isc_stdtime_t now) {
	if (cds_lfht_is_node_deleted(&bad->ht_node)) {
		return false;
	} else if (bad->expire < now) {
		bcentry_evict(ht, bad);
		return false;
	}

	return true;
}

#define cds_lfht_for_each_entry_next(ht, iter, pos, member)     \
	for (cds_lfht_next(ht, iter),                           \
	     pos = cds_lfht_entry(cds_lfht_iter_get_node(iter), \
				  __typeof__(*(pos)), member);  \
	     pos != NULL; /**/                                  \
	     cds_lfht_next(ht, iter),                           \
	     pos = cds_lfht_entry(cds_lfht_iter_get_node(iter), \
				  __typeof__(*(pos)), member))

static void
bcentry_purge(struct cds_lfht *ht, struct cds_list_head *lru,
	      isc_stdtime_t now) {
	size_t count = 10;
	dns_bcentry_t *bad;
	cds_list_for_each_entry_rcu(bad, lru, lru_head) {
		if (bcentry_alive(ht, bad, now)) {
			break;
		}
		if (--count == 0) {
			break;
		}
	}
}

void
dns_badcache_add(dns_badcache_t *bc, const dns_name_t *name,
		 dns_rdatatype_t type, uint32_t flags, isc_stdtime_t expire) {
	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(name != NULL);

	isc_loop_t *loop = isc_loop();
	uint32_t tid = isc_tid();
	struct cds_list_head *lru = &bc->lru[tid];

	isc_stdtime_t now = isc_stdtime_now();
	if (expire < now) {
		expire = now;
	}

	rcu_read_lock();
	struct cds_lfht *ht = rcu_dereference(bc->ht);
	INSIST(ht != NULL);

	dns__bckey_t key = {
		.name = name,
		.type = type,
	};
	uint32_t hashval = bcentry_hash(&key);

	/* struct cds_lfht_iter iter; */
	dns_bcentry_t *bad = bcentry_new(loop, name, type, flags, expire);
	struct cds_lfht_node *ht_node;
	do {
		ht_node = cds_lfht_add_unique(ht, hashval, bcentry_match, &key,
					      &bad->ht_node);
		if (ht_node != &bad->ht_node) {
			dns_bcentry_t *found = caa_container_of(
				ht_node, dns_bcentry_t, ht_node);
			bcentry_evict(ht, found);
		}
	} while (ht_node != &bad->ht_node);

	/* No locking, instead we are using per-thread lists */
	cds_list_add_tail_rcu(&bad->lru_head, lru);

	bcentry_purge(ht, lru, now);

	rcu_read_unlock();
}

isc_result_t
dns_badcache_find(dns_badcache_t *bc, const dns_name_t *name,
		  dns_rdatatype_t type, uint32_t *flagp, isc_stdtime_t now) {
	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(name != NULL);

	isc_result_t result = ISC_R_NOTFOUND;

	rcu_read_lock();
	struct cds_lfht *ht = rcu_dereference(bc->ht);
	INSIST(ht != NULL);

	dns__bckey_t key = {
		.name = name,
		.type = type,
	};
	uint32_t hashval = bcentry_hash(&key);

	dns_bcentry_t *found = bcentry_lookup(ht, hashval, &key);

	if (found != NULL && bcentry_alive(ht, found, now)) {
		result = ISC_R_SUCCESS;
		if (flagp != NULL) {
			*flagp = found->flags;
		}
	}

	uint32_t tid = isc_tid();
	struct cds_list_head *lru = &bc->lru[tid];
	bcentry_purge(ht, lru, now);

	rcu_read_unlock();

	return result;
}

void
dns_badcache_flush(dns_badcache_t *bc) {
	REQUIRE(VALID_BADCACHE(bc));

	rcu_read_lock();
	struct cds_lfht *ht = rcu_dereference(bc->ht);
	INSIST(ht != NULL);

	/* Flush the hash table */
	dns_bcentry_t *bad;
	struct cds_lfht_iter iter;
	cds_lfht_for_each_entry(ht, &iter, bad, ht_node) {
		bcentry_evict(ht, bad);
	}

	rcu_read_unlock();
}

void
dns_badcache_flushname(dns_badcache_t *bc, const dns_name_t *name) {
	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(name != NULL);

	isc_stdtime_t now = isc_stdtime_now();

	rcu_read_lock();
	struct cds_lfht *ht = rcu_dereference(bc->ht);
	INSIST(ht != NULL);

	dns_bcentry_t *bad;
	struct cds_lfht_iter iter;
	cds_lfht_for_each_entry(ht, &iter, bad, ht_node) {
		if (dns_name_equal(&bad->name, name)) {
			bcentry_evict(ht, bad);
			continue;
		}

		/* Flush all the expired entries */
		(void)bcentry_alive(ht, bad, now);
	}

	rcu_read_unlock();
}

void
dns_badcache_flushtree(dns_badcache_t *bc, const dns_name_t *name) {
	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(name != NULL);

	isc_stdtime_t now = isc_stdtime_now();

	rcu_read_lock();
	struct cds_lfht *ht = rcu_dereference(bc->ht);
	INSIST(ht != NULL);

	dns_bcentry_t *bad;
	struct cds_lfht_iter iter;
	cds_lfht_for_each_entry(ht, &iter, bad, ht_node) {
		if (dns_name_issubdomain(&bad->name, name)) {
			bcentry_evict(ht, bad);
			continue;
		}

		/* Flush all the expired entries */
		(void)bcentry_alive(ht, bad, now);
	}

	rcu_read_unlock();
}

static void
bcentry_print(dns_bcentry_t *bad, isc_stdtime_t now, FILE *fp) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];

	dns_name_format(&bad->name, namebuf, sizeof(namebuf));
	dns_rdatatype_format(bad->type, typebuf, sizeof(typebuf));
	fprintf(fp, "; %s/%s [ttl %" PRIu32 "]\n", namebuf, typebuf,
		bad->expire - now);
}

void
dns_badcache_print(dns_badcache_t *bc, const char *cachename, FILE *fp) {
	dns_bcentry_t *bad;
	isc_stdtime_t now = isc_stdtime_now();

	REQUIRE(VALID_BADCACHE(bc));
	REQUIRE(fp != NULL);

	fprintf(fp, ";\n; %s\n;\n", cachename);

	rcu_read_lock();
	struct cds_lfht *ht = rcu_dereference(bc->ht);
	INSIST(ht != NULL);

	struct cds_lfht_iter iter;
	cds_lfht_for_each_entry(ht, &iter, bad, ht_node) {
		if (bcentry_alive(ht, bad, now)) {
			bcentry_print(bad, now, fp);
		}
	}

	rcu_read_unlock();
}
