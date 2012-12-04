/*	$NetBSD: npf_tableset.c,v 1.16 2012/12/04 19:28:16 rmind Exp $	*/

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
 * Notes
 *
 *	The tableset is an array of tables.  After the creation, the array
 *	is immutable.  The caller is responsible to synchronise the access
 *	to the tableset.  The table can either be a hash or a tree.  Its
 *	entries are protected by a read-write lock.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_tableset.c,v 1.16 2012/12/04 19:28:16 rmind Exp $");

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

typedef struct npf_tblent {
	union {
		LIST_ENTRY(npf_tblent) hashq;
		pt_node_t	node;
	} te_entry;
	int			te_alen;
	npf_addr_t		te_addr;
} npf_tblent_t;

LIST_HEAD(npf_hashl, npf_tblent);

struct npf_table {
	char			t_name[16];
	/* Lock and reference count. */
	krwlock_t		t_lock;
	u_int			t_refcnt;
	/* Total number of items. */
	u_int			t_nitems;
	/* Table ID. */
	u_int			t_id;
	/* The storage type can be: a) hash b) tree. */
	int			t_type;
	struct npf_hashl *	t_hashl;
	u_long			t_hashmask;
	/* Separate trees for IPv4 and IPv6. */
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
		if (t && --t->t_refcnt == 0) {
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
		t->t_refcnt++;
		error = 0;
	} else {
		error = EEXIST;
	}
	return error;
}

/*
 * npf_tableset_reload: iterate all tables and if the new table is of the
 * same type and has no items, then we preserve the old one and its entries.
 *
 * => The caller is responsible for providing synchronisation.
 */
void
npf_tableset_reload(npf_tableset_t *ntset, npf_tableset_t *otset)
{
	for (int i = 0; i < NPF_TABLE_SLOTS; i++) {
		npf_table_t *t = ntset[i], *ot = otset[i];

		if (t == NULL || ot == NULL) {
			continue;
		}
		if (t->t_nitems || t->t_type != ot->t_type) {
			continue;
		}
		ntset[i] = ot;
		ot->t_refcnt++;
		npf_table_destroy(t);
	}
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
	case NPF_TABLE_HASH:
		for (unsigned n = 0; n <= t->t_hashmask; n++) {
			npf_tblent_t *ent;

			while ((ent = LIST_FIRST(&t->t_hashl[n])) != NULL) {
				LIST_REMOVE(ent, te_entry.hashq);
				pool_cache_put(tblent_cache, ent);
			}
		}
		hashdone(t->t_hashl, HASH_LIST, t->t_hashmask);
		break;
	case NPF_TABLE_TREE:
		table_tree_destroy(&t->t_tree[0]);
		table_tree_destroy(&t->t_tree[1]);
		break;
	default:
		KASSERT(false);
	}
	rw_destroy(&t->t_lock);
	kmem_free(t, sizeof(npf_table_t));
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
table_cidr_check(const u_int aidx, const npf_addr_t *addr,
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

	if ((u_int)tid >= NPF_TABLE_SLOTS || (t = tset[tid]) == NULL) {
		return EINVAL;
	}

	error = table_cidr_check(aidx, addr, mask);
	if (error) {
		return error;
	}
	ent = pool_cache_get(tblent_cache, PR_WAITOK);
	memcpy(&ent->te_addr, addr, alen);
	ent->te_alen = alen;

	/*
	 * Insert the entry.  Return an error on duplicate.
	 */
	rw_enter(&t->t_lock, RW_WRITER);
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
			t->t_nitems++;
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
		ok = (mask != NPF_NO_NETMASK) ?
		    ptree_insert_mask_node(tree, ent, mask) :
		    ptree_insert_node(tree, ent);
		if (ok) {
			t->t_nitems++;
			error = 0;
		} else {
			error = EEXIST;
		}
		break;
	}
	default:
		KASSERT(false);
	}
	rw_exit(&t->t_lock);

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

	error = table_cidr_check(aidx, addr, mask);
	if (error) {
		return error;
	}

	if ((u_int)tid >= NPF_TABLE_SLOTS || (t = tset[tid]) == NULL) {
		return EINVAL;
	}

	rw_enter(&t->t_lock, RW_WRITER);
	switch (t->t_type) {
	case NPF_TABLE_HASH: {
		struct npf_hashl *htbl;

		ent = table_hash_lookup(t, addr, alen, &htbl);
		if (__predict_true(ent != NULL)) {
			LIST_REMOVE(ent, te_entry.hashq);
			t->t_nitems--;
		}
		break;
	}
	case NPF_TABLE_TREE: {
		pt_tree_t *tree = &t->t_tree[aidx];

		ent = ptree_find_node(tree, addr);
		if (__predict_true(ent != NULL)) {
			ptree_remove_node(tree, ent);
			t->t_nitems--;
		}
		break;
	}
	default:
		KASSERT(false);
		ent = NULL;
	}
	rw_exit(&t->t_lock);

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

	if ((u_int)tid >= NPF_TABLE_SLOTS || (t = tset[tid]) == NULL) {
		return EINVAL;
	}

	rw_enter(&t->t_lock, RW_READER);
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
	rw_exit(&t->t_lock);

	return ent ? 0 : ENOENT;
}

static int
table_ent_copyout(npf_tblent_t *ent, npf_netmask_t mask,
    void *ubuf, size_t len, size_t *off)
{
	void *ubufp = (uint8_t *)ubuf + *off;
	npf_ioctl_ent_t uent;

	if ((*off += sizeof(npf_ioctl_ent_t)) > len) {
		return ENOMEM;
	}
	uent.alen = ent->te_alen;
	memcpy(&uent.addr, &ent->te_addr, sizeof(npf_addr_t));
	uent.mask = mask;

	return copyout(&uent, ubufp, sizeof(npf_ioctl_ent_t));
}

static int
table_tree_list(pt_tree_t *tree, npf_netmask_t maxmask, void *ubuf,
    size_t len, size_t *off)
{
	npf_tblent_t *ent = NULL;
	int error = 0;

	while ((ent = ptree_iterate(tree, ent, PT_ASCENDING)) != NULL) {
		pt_bitlen_t blen;

		if (!ptree_mask_node_p(tree, ent, &blen)) {
			blen = maxmask;
		}
		error = table_ent_copyout(ent, blen, ubuf, len, off);
		if (error)
			break;
	}
	return error;
}

/*
 * npf_table_list: copy a list of all table entries into a userspace buffer.
 */
int
npf_table_list(npf_tableset_t *tset, u_int tid, void *ubuf, size_t len)
{
	npf_table_t *t;
	size_t off = 0;
	int error = 0;

	if ((u_int)tid >= NPF_TABLE_SLOTS || (t = tset[tid]) == NULL) {
		return EINVAL;
	}

	rw_enter(&t->t_lock, RW_READER);
	switch (t->t_type) {
	case NPF_TABLE_HASH:
		for (unsigned n = 0; n <= t->t_hashmask; n++) {
			npf_tblent_t *ent;

			LIST_FOREACH(ent, &t->t_hashl[n], te_entry.hashq)
				if ((error = table_ent_copyout(ent, 0, ubuf,
				    len, &off)) != 0)
					break;
		}
		break;
	case NPF_TABLE_TREE:
		error = table_tree_list(&t->t_tree[0], 32, ubuf, len, &off);
		if (error)
			break;
		error = table_tree_list(&t->t_tree[1], 128, ubuf, len, &off);
		break;
	default:
		KASSERT(false);
	}
	rw_exit(&t->t_lock);

	return error;
}
