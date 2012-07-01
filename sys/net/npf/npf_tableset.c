/*	$NetBSD: npf_tableset.c,v 1.12 2012/07/01 23:21:06 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * NPF tableset module.
 *
 * TODO:
 * - Convert to Patricia tree.
 * - Dynamic hash growing/shrinking (i.e. re-hash functionality), maybe?
 * - Dynamic array resize.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_tableset.c,v 1.12 2012/07/01 23:21:06 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/hash.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/rwlock.h>
#include <sys/systm.h>
#include <sys/types.h>

#include "npf_impl.h"

/* Table entry structure. */
struct npf_tblent {
	/* Hash/tree entry. */
	union {
		LIST_ENTRY(npf_tblent)	hashq;
		rb_node_t		rbnode;
	} te_entry;
	/* CIDR block. */
	npf_addr_t			te_addr;
	npf_netmask_t			te_mask;
};

LIST_HEAD(npf_hashl, npf_tblent);

/* Table structure. */
struct npf_table {
	char				t_name[16];
	/* Lock and reference count. */
	krwlock_t			t_lock;
	u_int				t_refcnt;
	/* Table ID. */
	u_int				t_id;
	/* The storage type can be: 1. Hash 2. RB-tree. */
	int				t_type;
	struct npf_hashl *		t_hashl;
	u_long				t_hashmask;
	rb_tree_t			t_rbtree;
};

static pool_cache_t			tblent_cache	__read_mostly;

/*
 * npf_table_sysinit: initialise tableset structures.
 */
void
npf_tableset_sysinit(void)
{

	tblent_cache = pool_cache_init(sizeof(npf_tblent_t), coherency_unit,
	    0, 0, "npftenpl", NULL, IPL_NONE, NULL, NULL, NULL);
}

void
npf_tableset_sysfini(void)
{

	pool_cache_destroy(tblent_cache);
}

npf_tableset_t *
npf_tableset_create(void)
{
	const size_t sz = NPF_TABLE_SLOTS * sizeof(npf_table_t *);

	return kmem_zalloc(sz, KM_SLEEP);
}

void
npf_tableset_destroy(npf_tableset_t *tblset)
{
	const size_t sz = NPF_TABLE_SLOTS * sizeof(npf_table_t *);
	npf_table_t *t;
	u_int tid;

	/*
	 * Destroy all tables (no references should be held, as ruleset
	 * should be destroyed before).
	 */
	for (tid = 0; tid < NPF_TABLE_SLOTS; tid++) {
		t = tblset[tid];
		if (t != NULL) {
			npf_table_destroy(t);
		}
	}
	kmem_free(tblset, sz);
}

/*
 * npf_tableset_insert: insert the table into the specified tableset.
 *
 * => Returns 0 on success, fails and returns errno if ID is already used.
 */
int
npf_tableset_insert(npf_tableset_t *tblset, npf_table_t *t)
{
	const u_int tid = t->t_id;
	int error;

	KASSERT((u_int)tid < NPF_TABLE_SLOTS);

	if (tblset[tid] == NULL) {
		tblset[tid] = t;
		error = 0;
	} else {
		error = EEXIST;
	}
	return error;
}

/*
 * Red-black tree storage.
 */

static signed int
table_rbtree_cmp_nodes(void *ctx, const void *n1, const void *n2)
{
	const npf_tblent_t * const te1 = n1;
	const npf_tblent_t * const te2 = n2;

	return npf_addr_cmp(&te1->te_addr, te1->te_mask,
	    &te2->te_addr, te2->te_mask, sizeof(npf_addr_t));
}

static signed int
table_rbtree_cmp_key(void *ctx, const void *n1, const void *key)
{
	const npf_tblent_t * const te = n1;
	const npf_addr_t *t2 = key;

	return npf_addr_cmp(&te->te_addr, te->te_mask,
	    t2, NPF_NO_NETMASK, sizeof(npf_addr_t));
}

static const rb_tree_ops_t table_rbtree_ops = {
	.rbto_compare_nodes = table_rbtree_cmp_nodes,
	.rbto_compare_key = table_rbtree_cmp_key,
	.rbto_node_offset = offsetof(npf_tblent_t, te_entry.rbnode),
	.rbto_context = NULL
};

/*
 * Hash helper routine.
 */

static inline struct npf_hashl *
table_hash_bucket(npf_table_t *t, const void *buf, size_t sz)
{
	const uint32_t hidx = hash32_buf(buf, sz, HASH32_BUF_INIT);

	return &t->t_hashl[hidx & t->t_hashmask];
}

/*
 * npf_table_create: create table with a specified ID.
 */
npf_table_t *
npf_table_create(u_int tid, int type, size_t hsize)
{
	npf_table_t *t;

	KASSERT((u_int)tid < NPF_TABLE_SLOTS);

	t = kmem_zalloc(sizeof(npf_table_t), KM_SLEEP);
	switch (type) {
	case NPF_TABLE_TREE:
		rb_tree_init(&t->t_rbtree, &table_rbtree_ops);
		break;
	case NPF_TABLE_HASH:
		t->t_hashl = hashinit(hsize, HASH_LIST, true, &t->t_hashmask);
		if (t->t_hashl == NULL) {
			kmem_free(t, sizeof(npf_table_t));
			return NULL;
		}
		break;
	default:
		KASSERT(false);
	}
	rw_init(&t->t_lock);
	t->t_type = type;
	t->t_refcnt = 1;
	t->t_id = tid;
	return t;
}

/*
 * npf_table_destroy: free all table entries and table itself.
 */
void
npf_table_destroy(npf_table_t *t)
{
	npf_tblent_t *e;
	u_int n;

	switch (t->t_type) {
	case NPF_TABLE_HASH:
		for (n = 0; n <= t->t_hashmask; n++) {
			while ((e = LIST_FIRST(&t->t_hashl[n])) != NULL) {
				LIST_REMOVE(e, te_entry.hashq);
				pool_cache_put(tblent_cache, e);
			}
		}
		hashdone(t->t_hashl, HASH_LIST, t->t_hashmask);
		break;
	case NPF_TABLE_TREE:
		while ((e = rb_tree_iterate(&t->t_rbtree, NULL,
		    RB_DIR_LEFT)) != NULL) {
			rb_tree_remove_node(&t->t_rbtree, e);
			pool_cache_put(tblent_cache, e);
		}
		break;
	default:
		KASSERT(false);
	}
	rw_destroy(&t->t_lock);
	kmem_free(t, sizeof(npf_table_t));
}

/*
 * npf_table_ref: holds the reference on table.
 *
 * => Table must be locked.
 */
void
npf_table_ref(npf_table_t *t)
{

	KASSERT(rw_lock_held(&t->t_lock));
	atomic_inc_uint(&t->t_refcnt);
}

/*
 * npf_table_unref: drop reference from the table and destroy the table if
 * it is the last reference.
 */
void
npf_table_unref(npf_table_t *t)
{

	if (atomic_dec_uint_nv(&t->t_refcnt) != 0) {
		return;
	}
	npf_table_destroy(t);
}

/*
 * npf_table_get: find the table according to ID and "get it" by locking it.
 */
npf_table_t *
npf_table_get(npf_tableset_t *tset, u_int tid)
{
	npf_table_t *t;

	KASSERT(tset != NULL);

	if ((u_int)tid >= NPF_TABLE_SLOTS) {
		return NULL;
	}
	t = tset[tid];
	if (t != NULL) {
		rw_enter(&t->t_lock, RW_READER);
	}
	return t;
}

/*
 * npf_table_put: "put table back" by unlocking it.
 */
void
npf_table_put(npf_table_t *t)
{

	rw_exit(&t->t_lock);
}

/*
 * npf_table_check: validate ID and type.
 * */
int
npf_table_check(npf_tableset_t *tset, u_int tid, int type)
{

	if ((u_int)tid >= NPF_TABLE_SLOTS) {
		return EINVAL;
	}
	if (tset[tid] != NULL) {
		return EEXIST;
	}
	if (type != NPF_TABLE_TREE && type != NPF_TABLE_HASH) {
		return EINVAL;
	}
	return 0;
}

/*
 * npf_table_add_cidr: add an IP CIDR into the table.
 */
int
npf_table_add_cidr(npf_tableset_t *tset, u_int tid,
    const npf_addr_t *addr, const npf_netmask_t mask)
{
	struct npf_hashl *htbl;
	npf_tblent_t *ent, *it;
	npf_table_t *t;
	npf_addr_t val;
	int error = 0;

	if (mask > NPF_MAX_NETMASK) {
		return EINVAL;
	}
	ent = pool_cache_get(tblent_cache, PR_WAITOK);
	memcpy(&ent->te_addr, addr, sizeof(npf_addr_t));
	ent->te_mask = mask;

	/* Get the table (acquire the lock). */
	t = npf_table_get(tset, tid);
	if (t == NULL) {
		pool_cache_put(tblent_cache, ent);
		return EINVAL;
	}

	switch (t->t_type) {
	case NPF_TABLE_HASH:
		/* Generate hash value from: address & mask. */
		npf_addr_mask(addr, mask, sizeof(npf_addr_t), &val);
		htbl = table_hash_bucket(t, &val, sizeof(npf_addr_t));

		/* Lookup to check for duplicates. */
		LIST_FOREACH(it, htbl, te_entry.hashq) {
			if (it->te_mask != mask) {
				continue;
			}
			if (!memcmp(&it->te_addr, addr, sizeof(npf_addr_t))) {
				break;
			}
		}

		/* If no duplicate - insert entry. */
		if (__predict_true(it == NULL)) {
			LIST_INSERT_HEAD(htbl, ent, te_entry.hashq);
		} else {
			error = EEXIST;
		}
		break;
	case NPF_TABLE_TREE:
		/* Insert entry.  Returns false, if duplicate. */
		if (rb_tree_insert_node(&t->t_rbtree, ent) != ent) {
			error = EEXIST;
		}
		break;
	default:
		KASSERT(false);
	}
	npf_table_put(t);

	if (error) {
		pool_cache_put(tblent_cache, ent);
	}
	return error;
}

/*
 * npf_table_rem_cidr: remove an IP CIDR from the table.
 */
int
npf_table_rem_cidr(npf_tableset_t *tset, u_int tid,
    const npf_addr_t *addr, const npf_netmask_t mask)
{
	struct npf_hashl *htbl;
	npf_tblent_t *ent;
	npf_table_t *t;
	npf_addr_t val;

	if (mask > NPF_MAX_NETMASK) {
		return EINVAL;
	}

	/* Get the table (acquire the lock). */
	t = npf_table_get(tset, tid);
	if (__predict_false(t == NULL)) {
		return EINVAL;
	}

	/* Key: (address & mask). */
	npf_addr_mask(addr, mask, sizeof(npf_addr_t), &val);
	ent = NULL;

	switch (t->t_type) {
	case NPF_TABLE_HASH:
		/* Generate hash value from: (address & mask). */
		htbl = table_hash_bucket(t, &val, sizeof(npf_addr_t));
		LIST_FOREACH(ent, htbl, te_entry.hashq) {
			if (ent->te_mask != mask) {
				continue;
			}
			if (!memcmp(&ent->te_addr, addr, sizeof(npf_addr_t))) {
				break;
			}
		}
		if (__predict_true(ent != NULL)) {
			LIST_REMOVE(ent, te_entry.hashq);
		}
		break;
	case NPF_TABLE_TREE:
		ent = rb_tree_find_node(&t->t_rbtree, &val);
		if (__predict_true(ent != NULL)) {
			rb_tree_remove_node(&t->t_rbtree, ent);
		}
		break;
	default:
		KASSERT(false);
	}
	npf_table_put(t);

	if (ent == NULL) {
		return ENOENT;
	}
	pool_cache_put(tblent_cache, ent);
	return 0;
}

/*
 * npf_table_match_addr: find the table according to ID, lookup and
 * match the contents with specified IPv4 address.
 */
int
npf_table_match_addr(npf_tableset_t *tset, u_int tid, const npf_addr_t *addr)
{
	struct npf_hashl *htbl;
	npf_tblent_t *ent = NULL;
	npf_table_t *t;

	/* Get the table (acquire the lock). */
	t = npf_table_get(tset, tid);
	if (__predict_false(t == NULL)) {
		return EINVAL;
	}
	switch (t->t_type) {
	case NPF_TABLE_HASH:
		htbl = table_hash_bucket(t, addr, sizeof(npf_addr_t));
		LIST_FOREACH(ent, htbl, te_entry.hashq) {
			if (npf_addr_cmp(addr, ent->te_mask, &ent->te_addr,
			    NPF_NO_NETMASK, sizeof(npf_addr_t)) == 0) {
				break;
			}
		}
		break;
	case NPF_TABLE_TREE:
		ent = rb_tree_find_node(&t->t_rbtree, addr);
		break;
	default:
		KASSERT(false);
	}
	npf_table_put(t);

	if (ent == NULL) {
		return ENOENT;
	}
	KASSERT(npf_addr_cmp(addr, ent->te_mask, &ent->te_addr,
	    NPF_NO_NETMASK, sizeof(npf_addr_t)) == 0);
	return 0;
}
