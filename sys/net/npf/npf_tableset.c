/*	$NetBSD: npf_tableset.c,v 1.9.2.5 2012/08/13 17:49:52 riz Exp $	*/

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
 * - Dynamic hash growing/shrinking (i.e. re-hash functionality), maybe?
 * - Dynamic array resize.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_tableset.c,v 1.9.2.5 2012/08/13 17:49:52 riz Exp $");

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

/*
 * Table structures.
 */

struct npf_tblent {
	union {
		LIST_ENTRY(npf_tblent) hashq;
		pt_node_t	node;
	} te_entry;
	int			te_alen;
	npf_addr_t		te_addr;
};

LIST_HEAD(npf_hashl, npf_tblent);

struct npf_table {
	char			t_name[16];
	/* Lock and reference count. */
	krwlock_t		t_lock;
	u_int			t_refcnt;
	/* Table ID. */
	u_int			t_id;
	/* The storage type can be: a) hash b) tree. */
	int			t_type;
	struct npf_hashl *	t_hashl;
	u_long			t_hashmask;
	pt_tree_t		t_tree[2];
};

#define	NPF_ADDRLEN2TREE(alen)	((alen) >> 4)

static pool_cache_t		tblent_cache	__read_mostly;

/*
 * npf_table_sysinit: initialise tableset structures.
 */
void
npf_tableset_sysinit(void)
{

	tblent_cache = pool_cache_init(sizeof(npf_tblent_t), coherency_unit,
	    0, 0, "npftblpl", NULL, IPL_NONE, NULL, NULL, NULL);
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
 * => Returns 0 on success.  Fails and returns error if ID is already used.
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
 * Few helper routines.
 */

static npf_tblent_t *
table_hash_lookup(const npf_table_t *t, const npf_addr_t *addr,
    const int alen, struct npf_hashl **rhtbl)
{
	const uint32_t hidx = hash32_buf(addr, alen, HASH32_BUF_INIT);
	struct npf_hashl *htbl = &t->t_hashl[hidx & t->t_hashmask];
	npf_tblent_t *ent;

	/*
	 * Lookup the hash table and check for duplicates.
	 * Note: mask is ignored for the hash storage.
	 */
	LIST_FOREACH(ent, htbl, te_entry.hashq) {
		if (ent->te_alen != alen) {
			continue;
		}
		if (memcmp(&ent->te_addr, addr, alen) == 0) {
			break;
		}
	}
	*rhtbl = htbl;
	return ent;
}

static void
table_tree_destroy(pt_tree_t *tree)
{
	npf_tblent_t *ent;

	while ((ent = ptree_iterate(tree, NULL, PT_ASCENDING)) != NULL) {
		ptree_remove_node(tree, ent);
		pool_cache_put(tblent_cache, ent);
	}
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
		ptree_init(&t->t_tree[0], &npf_table_ptree_ops,
		    (void *)(sizeof(struct in_addr) / sizeof(uint32_t)),
		    offsetof(npf_tblent_t, te_entry.node),
		    offsetof(npf_tblent_t, te_addr));
		ptree_init(&t->t_tree[1], &npf_table_ptree_ops,
		    (void *)(sizeof(struct in6_addr) / sizeof(uint32_t)),
		    offsetof(npf_tblent_t, te_entry.node),
		    offsetof(npf_tblent_t, te_addr));
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

	switch (t->t_type) {
	case NPF_TABLE_HASH: {
		for (unsigned n = 0; n <= t->t_hashmask; n++) {
			npf_tblent_t *ent;

			while ((ent = LIST_FIRST(&t->t_hashl[n])) != NULL) {
				LIST_REMOVE(ent, te_entry.hashq);
				pool_cache_put(tblent_cache, ent);
			}
		}
		hashdone(t->t_hashl, HASH_LIST, t->t_hashmask);
		break;
	}
	case NPF_TABLE_TREE: {
		table_tree_destroy(&t->t_tree[0]);
		table_tree_destroy(&t->t_tree[1]);
		break;
	}
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
 */
int
npf_table_check(const npf_tableset_t *tset, u_int tid, int type)
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

static int
npf_table_validate_cidr(const u_int aidx, const npf_addr_t *addr,
    const npf_netmask_t mask)
{

	if (mask > NPF_MAX_NETMASK && mask != NPF_NO_NETMASK) {
		return EINVAL;
	}
	if (aidx > 1) {
		return EINVAL;
	}

	/*
	 * For IPv4 (aidx = 0) - 32 and for IPv6 (aidx = 1) - 128.
	 * If it is a host - shall use NPF_NO_NETMASK.
	 */
	if (mask >= (aidx ? 128 : 32) && mask != NPF_NO_NETMASK) {
		return EINVAL;
	}
	return 0;
}

/*
 * npf_table_insert: add an IP CIDR entry into the table.
 */
int
npf_table_insert(npf_tableset_t *tset, u_int tid, const int alen,
    const npf_addr_t *addr, const npf_netmask_t mask)
{
	const u_int aidx = NPF_ADDRLEN2TREE(alen);
	npf_tblent_t *ent;
	npf_table_t *t;
	int error;

	error = npf_table_validate_cidr(aidx, addr, mask);
	if (error) {
		return error;
	}
	ent = pool_cache_get(tblent_cache, PR_WAITOK);
	memcpy(&ent->te_addr, addr, alen);
	ent->te_alen = alen;

	/* Get the table (acquire the lock). */
	t = npf_table_get(tset, tid);
	if (t == NULL) {
		pool_cache_put(tblent_cache, ent);
		return EINVAL;
	}

	/*
	 * Insert the entry.  Return an error on duplicate.
	 */
	switch (t->t_type) {
	case NPF_TABLE_HASH: {
		struct npf_hashl *htbl;

		/*
		 * Hash tables by the concept support only IPs.
		 */
		if (mask != NPF_NO_NETMASK) {
			error = EINVAL;
			break;
		}
		if (!table_hash_lookup(t, addr, alen, &htbl)) {
			LIST_INSERT_HEAD(htbl, ent, te_entry.hashq);
		} else {
			error = EEXIST;
		}
		break;
	}
	case NPF_TABLE_TREE: {
		pt_tree_t *tree = &t->t_tree[aidx];
		bool ok;

		/*
		 * If no mask specified, use maximum mask.
		 */
		if (mask != NPF_NO_NETMASK) {
			ok = ptree_insert_mask_node(tree, ent, mask);
		} else {
			ok = ptree_insert_node(tree, ent);
		}
		error = ok ? 0 : EEXIST;
		break;
	}
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
 * npf_table_remove: remove the IP CIDR entry from the table.
 */
int
npf_table_remove(npf_tableset_t *tset, u_int tid, const int alen,
    const npf_addr_t *addr, const npf_netmask_t mask)
{
	const u_int aidx = NPF_ADDRLEN2TREE(alen);
	npf_tblent_t *ent;
	npf_table_t *t;
	int error;

	error = npf_table_validate_cidr(aidx, addr, mask);
	if (error) {
		return error;
	}
	t = npf_table_get(tset, tid);
	if (t == NULL) {
		return EINVAL;
	}

	switch (t->t_type) {
	case NPF_TABLE_HASH: {
		struct npf_hashl *htbl;

		ent = table_hash_lookup(t, addr, alen, &htbl);
		if (__predict_true(ent != NULL)) {
			LIST_REMOVE(ent, te_entry.hashq);
		}
		break;
	}
	case NPF_TABLE_TREE: {
		pt_tree_t *tree = &t->t_tree[aidx];

		ent = ptree_find_node(tree, addr);
		if (__predict_true(ent != NULL)) {
			ptree_remove_node(tree, ent);
		}
		break;
	}
	default:
		KASSERT(false);
		ent = NULL;
	}
	npf_table_put(t);

	if (ent == NULL) {
		return ENOENT;
	}
	pool_cache_put(tblent_cache, ent);
	return 0;
}

/*
 * npf_table_lookup: find the table according to ID, lookup and match
 * the contents with the specified IP address.
 */
int
npf_table_lookup(npf_tableset_t *tset, u_int tid,
    const int alen, const npf_addr_t *addr)
{
	const u_int aidx = NPF_ADDRLEN2TREE(alen);
	npf_tblent_t *ent;
	npf_table_t *t;

	if (__predict_false(aidx > 1)) {
		return EINVAL;
	}

	t = npf_table_get(tset, tid);
	if (__predict_false(t == NULL)) {
		return EINVAL;
	}
	switch (t->t_type) {
	case NPF_TABLE_HASH: {
		struct npf_hashl *htbl;
		ent = table_hash_lookup(t, addr, alen, &htbl);
		break;
	}
	case NPF_TABLE_TREE: {
		ent = ptree_find_node(&t->t_tree[aidx], addr);
		break;
	}
	default:
		KASSERT(false);
		ent = NULL;
	}
	npf_table_put(t);

	return ent ? 0 : ENOENT;
}
