/*	$NetBSD: npf_tableset.c,v 1.2.2.2 2010/10/09 03:32:37 yamt Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
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
 * NPF table module.
 *
 *	table_lock ->
 *		npf_table_t::t_lock
 *
 * TODO:
 * - Currently, code is modeled to handle IPv4 CIDR blocks.
 * - Dynamic hash growing/shrinking (i.e. re-hash functionality), maybe?
 * - Dynamic array resize.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_tableset.c,v 1.2.2.2 2010/10/09 03:32:37 yamt Exp $");
#endif

#include <sys/param.h>
#include <sys/kernel.h>

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
		struct rb_node		rbnode;
	} te_entry;
	/* IPv4 CIDR block. */
	in_addr_t			te_addr;
	in_addr_t			te_mask;
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
	u_int				t_type;
	struct npf_hashl *		t_hashl;
	u_long				t_hashmask;
	rb_tree_t			t_rbtree;
};

/* Global table array and its lock. */
static npf_tableset_t *		table_array;
static krwlock_t		table_lock;
static pool_cache_t		tblent_cache;

/*
 * npf_table_sysinit: initialise tableset structures.
 */
int
npf_tableset_sysinit(void)
{

	tblent_cache = pool_cache_init(sizeof(npf_tblent_t), coherency_unit,
	    0, 0, "npftenpl", NULL, IPL_NONE, NULL, NULL, NULL);
	if (tblent_cache == NULL) {
		return ENOMEM;
	}
	table_array = npf_tableset_create();
	if (table_array == NULL) {
		pool_cache_destroy(tblent_cache);
		return ENOMEM;
	}
	rw_init(&table_lock);
	return 0;
}

void
npf_tableset_sysfini(void)
{

	npf_tableset_destroy(table_array);
	pool_cache_destroy(tblent_cache);
	rw_destroy(&table_lock);
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
 * npf_tableset_reload: replace old tableset array with a new one.
 *
 * => Called from npf_ruleset_reload() with a global ruleset lock held.
 * => Returns pointer to the old tableset, caller will destroy it.
 */
npf_tableset_t *
npf_tableset_reload(npf_tableset_t *tblset)
{
	npf_tableset_t *oldtblset;

	rw_enter(&table_lock, RW_WRITER);
	oldtblset = table_array;
	table_array = tblset;
	rw_exit(&table_lock);

	return oldtblset;
}

/*
 * Red-black tree storage.
 */

static signed int
table_rbtree_cmp_nodes(void *ctx, const void *n1, const void *n2)
{
	const npf_tblent_t * const te1 = n1;
	const npf_tblent_t * const te2 = n2;
	const in_addr_t x = te1->te_addr & te1->te_mask;
	const in_addr_t y = te2->te_addr & te2->te_mask;

	if (x < y)
		return -1;
	if (x > y)
		return 1;
	return 0;
}

static signed int
table_rbtree_cmp_key(void *ctx, const void *n1, const void *key)
{
	const npf_tblent_t * const te = n1;
	const in_addr_t x = te->te_addr & te->te_mask;
	const in_addr_t y = *(const in_addr_t *)key;

	if (x < y)
		return -1;
	if (x > y)
		return 1;
	return 0;
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
table_hash_bucket(npf_table_t *t, void *buf, size_t sz)
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
	case NPF_TABLE_RBTREE:
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
	case NPF_TABLE_RBTREE:
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

	if ((u_int)tid >= NPF_TABLE_SLOTS) {
		return NULL;
	}
	if (tset) {
		t = tset[tid];
		if (t != NULL) {
			rw_enter(&t->t_lock, RW_READER);
		}
		return t;
	}
	rw_enter(&table_lock, RW_READER);
	t = table_array[tid];
	if (t != NULL) {
		rw_enter(&t->t_lock, RW_READER);
	}
	rw_exit(&table_lock);
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
	if (type != NPF_TABLE_RBTREE && type != NPF_TABLE_HASH) {
		return EINVAL;
	}
	return 0;
}

/*
 * npf_table_add_v4cidr: add an IPv4 CIDR into the table.
 */
int
npf_table_add_v4cidr(npf_tableset_t *tset, u_int tid,
    in_addr_t addr, in_addr_t mask)
{
	struct npf_hashl *htbl;
	npf_tblent_t *e, *it;
	npf_table_t *t;
	in_addr_t val;
	int error = 0;

	/* Allocate and setup entry. */
	e = pool_cache_get(tblent_cache, PR_WAITOK);
	if (e == NULL) {
		return ENOMEM;
	}
	e->te_addr = addr;
	e->te_mask = mask;

	/* Locks the table. */
	t = npf_table_get(tset, tid);
	if (__predict_false(t == NULL)) {
		pool_cache_put(tblent_cache, e);
		return EINVAL;
	}
	switch (t->t_type) {
	case NPF_TABLE_HASH:
		/* Generate hash value from: address & mask. */
		val = addr & mask;
		htbl = table_hash_bucket(t, &val, sizeof(in_addr_t));
		/* Lookup to check for duplicates. */
		LIST_FOREACH(it, htbl, te_entry.hashq) {
			if (it->te_addr == addr && it->te_mask == mask)
				break;
		}
		/* If no duplicate - insert entry. */
		if (__predict_true(it == NULL)) {
			LIST_INSERT_HEAD(htbl, e, te_entry.hashq);
		} else {
			error = EEXIST;
		}
		break;
	case NPF_TABLE_RBTREE:
		/* Insert entry.  Returns false, if duplicate. */
		if (rb_tree_insert_node(&t->t_rbtree, e) != e) {
			error = EEXIST;
		}
		break;
	default:
		KASSERT(false);
	}
	npf_table_put(t);

	if (__predict_false(error)) {
		pool_cache_put(tblent_cache, e);
	}
	return error;
}

/*
 * npf_table_rem_v4cidr: remove an IPv4 CIDR from the table.
 */
int
npf_table_rem_v4cidr(npf_tableset_t *tset, u_int tid,
    in_addr_t addr, in_addr_t mask)
{
	struct npf_hashl *htbl;
	npf_tblent_t *e;
	npf_table_t *t;
	in_addr_t val;
	int error;

	e = NULL;

	/* Locks the table. */
	t = npf_table_get(tset, tid);
	if (__predict_false(t == NULL)) {
		return EINVAL;
	}
	/* Lookup & remove. */
	switch (t->t_type) {
	case NPF_TABLE_HASH:
		/* Generate hash value from: (address & mask). */
		val = addr & mask;
		htbl = table_hash_bucket(t, &val, sizeof(in_addr_t));
		LIST_FOREACH(e, htbl, te_entry.hashq) {
			if (e->te_addr == addr && e->te_mask == mask)
				break;
		}
		if (__predict_true(e != NULL)) {
			LIST_REMOVE(e, te_entry.hashq);
		} else {
			error = ESRCH;
		}
		break;
	case NPF_TABLE_RBTREE:
		/* Key: (address & mask). */
		val = addr & mask;
		e = rb_tree_find_node(&t->t_rbtree, &val);
		if (__predict_true(e != NULL)) {
			rb_tree_remove_node(&t->t_rbtree, e);
		} else {
			error = ESRCH;
		}
		break;
	default:
		KASSERT(false);
	}
	npf_table_put(t);

	/* Free table the entry. */
	if (__predict_true(e != NULL)) {
		pool_cache_put(tblent_cache, e);
	}
	return e ? 0 : -1;
}

/*
 * npf_table_match_v4addr: find the table according to ID, lookup and
 * match the contents with specified IPv4 address.
 */
int
npf_table_match_v4addr(u_int tid, in_addr_t ip4addr)
{
	struct npf_hashl *htbl;
	npf_tblent_t *e;
	npf_table_t *t;

	e = NULL;

	/* Locks the table. */
	t = npf_table_get(NULL, tid);
	if (__predict_false(t == NULL)) {
		return EINVAL;
	}
	switch (t->t_type) {
	case NPF_TABLE_HASH:
		htbl = table_hash_bucket(t, &ip4addr, sizeof(in_addr_t));
		LIST_FOREACH(e, htbl, te_entry.hashq) {
			if ((ip4addr & e->te_mask) == e->te_addr) {
				break;
			}
		}
		break;
	case NPF_TABLE_RBTREE:
		e = rb_tree_find_node(&t->t_rbtree, &ip4addr);
		KASSERT((ip4addr & e->te_mask) == e->te_addr);
		break;
	default:
		KASSERT(false);
	}
	npf_table_put(t);

	return e ? 0 : -1;
}
