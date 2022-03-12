/*-
 * Copyright (c) 2009-2019 The NetBSD Foundation, Inc.
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
 *	to the tableset.
 *
 * Warning (not applicable for the userspace npfkern):
 *
 *	The thmap_put()/thmap_del() are not called from the interrupt
 *	context and are protected by an IPL_NET mutex(9), therefore they
 *	do not need SPL wrappers -- see the comment at the top of the
 *	npf_conndb.c source file.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_tableset.c,v 1.37 2022/03/12 15:32:32 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/cdbr.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/thmap.h>

#include "lpm.h"
#endif

#include "npf_impl.h"

typedef struct npf_tblent {
	LIST_ENTRY(npf_tblent)	te_listent;
	uint16_t		te_preflen;
	uint16_t		te_alen;
	npf_addr_t		te_addr;
} npf_tblent_t;

#define	NPF_ADDRLEN2IDX(alen)	((alen) >> 4)
#define	NPF_ADDR_SLOTS		(2)

struct npf_table {
	/*
	 * The storage type can be: a) hashmap b) LPM c) cdb.
	 * There are separate trees for IPv4 and IPv6.
	 */
	union {
		struct {
			thmap_t *	t_map;
			LIST_HEAD(, npf_tblent) t_gc;
		};
		lpm_t *			t_lpm;
		struct {
			void *		t_blob;
			size_t		t_bsize;
			struct cdbr *	t_cdb;
		};
		struct {
			npf_tblent_t **	t_elements[NPF_ADDR_SLOTS];
			unsigned	t_allocated[NPF_ADDR_SLOTS];
			unsigned	t_used[NPF_ADDR_SLOTS];
		};
	} /* C11 */;
	LIST_HEAD(, npf_tblent)		t_list;
	unsigned			t_nitems;

	/*
	 * Table ID, type and lock.  The ID may change during the
	 * config reload, it is protected by the npf->config_lock.
	 */
	int			t_type;
	unsigned		t_id;
	kmutex_t		t_lock;

	/* Reference count and table name. */
	unsigned		t_refcnt;
	char			t_name[NPF_TABLE_MAXNAMELEN];
};

struct npf_tableset {
	unsigned		ts_nitems;
	npf_table_t *		ts_map[];
};

#define	NPF_TABLESET_SIZE(n)	\
    (offsetof(npf_tableset_t, ts_map[n]) * sizeof(npf_table_t *))

#define	NPF_IFADDR_STEP		4

static pool_cache_t		tblent_cache	__read_mostly;

/*
 * npf_table_sysinit: initialise tableset structures.
 */
void
npf_tableset_sysinit(void)
{
	tblent_cache = pool_cache_init(sizeof(npf_tblent_t), 0,
	    0, 0, "npftblpl", NULL, IPL_NONE, NULL, NULL, NULL);
}

void
npf_tableset_sysfini(void)
{
	pool_cache_destroy(tblent_cache);
}

npf_tableset_t *
npf_tableset_create(u_int nitems)
{
	npf_tableset_t *ts = kmem_zalloc(NPF_TABLESET_SIZE(nitems), KM_SLEEP);
	ts->ts_nitems = nitems;
	return ts;
}

void
npf_tableset_destroy(npf_tableset_t *ts)
{
	/*
	 * Destroy all tables (no references should be held, since the
	 * ruleset should be destroyed before).
	 */
	for (u_int tid = 0; tid < ts->ts_nitems; tid++) {
		npf_table_t *t = ts->ts_map[tid];

		if (t == NULL)
			continue;
		membar_exit();
		if (atomic_dec_uint_nv(&t->t_refcnt) > 0)
			continue;
		membar_enter();
		npf_table_destroy(t);
	}
	kmem_free(ts, NPF_TABLESET_SIZE(ts->ts_nitems));
}

/*
 * npf_tableset_insert: insert the table into the specified tableset.
 *
 * => Returns 0 on success.  Fails and returns error if ID is already used.
 */
int
npf_tableset_insert(npf_tableset_t *ts, npf_table_t *t)
{
	const u_int tid = t->t_id;
	int error;

	KASSERT((u_int)tid < ts->ts_nitems);

	if (ts->ts_map[tid] == NULL) {
		atomic_inc_uint(&t->t_refcnt);
		ts->ts_map[tid] = t;
		error = 0;
	} else {
		error = EEXIST;
	}
	return error;
}

npf_table_t *
npf_tableset_swap(npf_tableset_t *ts, npf_table_t *newt)
{
	const u_int tid = newt->t_id;
	npf_table_t *oldt = ts->ts_map[tid];

	KASSERT(tid < ts->ts_nitems);
	KASSERT(oldt->t_id == newt->t_id);

	newt->t_refcnt = oldt->t_refcnt;
	oldt->t_refcnt = 0;
	membar_producer();

	return atomic_swap_ptr(&ts->ts_map[tid], newt);
}

/*
 * npf_tableset_getbyname: look for a table in the set given the name.
 */
npf_table_t *
npf_tableset_getbyname(npf_tableset_t *ts, const char *name)
{
	npf_table_t *t;

	for (u_int tid = 0; tid < ts->ts_nitems; tid++) {
		if ((t = ts->ts_map[tid]) == NULL)
			continue;
		if (strcmp(name, t->t_name) == 0)
			return t;
	}
	return NULL;
}

npf_table_t *
npf_tableset_getbyid(npf_tableset_t *ts, unsigned tid)
{
	if (__predict_true(tid < ts->ts_nitems)) {
		return atomic_load_relaxed(&ts->ts_map[tid]);
	}
	return NULL;
}

/*
 * npf_tableset_reload: iterate all tables and if the new table is of the
 * same type and has no items, then we preserve the old one and its entries.
 *
 * => The caller is responsible for providing synchronisation.
 */
void
npf_tableset_reload(npf_t *npf, npf_tableset_t *nts, npf_tableset_t *ots)
{
	for (u_int tid = 0; tid < nts->ts_nitems; tid++) {
		npf_table_t *t, *ot;

		if ((t = nts->ts_map[tid]) == NULL) {
			continue;
		}

		/* If our table has entries, just load it. */
		if (t->t_nitems) {
			continue;
		}

		/* Look for a currently existing table with such name. */
		ot = npf_tableset_getbyname(ots, t->t_name);
		if (ot == NULL) {
			/* Not found: we have a new table. */
			continue;
		}

		/* Found.  Did the type change? */
		if (t->t_type != ot->t_type) {
			/* Yes, load the new. */
			continue;
		}

		/*
		 * Preserve the current table.  Acquire a reference since
		 * we are keeping it in the old table set.  Update its ID.
		 */
		atomic_inc_uint(&ot->t_refcnt);
		nts->ts_map[tid] = ot;

		KASSERT(npf_config_locked_p(npf));
		ot->t_id = tid;

		/* Destroy the new table (we hold the only reference). */
		t->t_refcnt--;
		npf_table_destroy(t);
	}
}

int
npf_tableset_export(npf_t *npf, const npf_tableset_t *ts, nvlist_t *nvl)
{
	const npf_table_t *t;

	KASSERT(npf_config_locked_p(npf));

	for (u_int tid = 0; tid < ts->ts_nitems; tid++) {
		nvlist_t *table;

		if ((t = ts->ts_map[tid]) == NULL) {
			continue;
		}
		table = nvlist_create(0);
		nvlist_add_string(table, "name", t->t_name);
		nvlist_add_number(table, "type", t->t_type);
		nvlist_add_number(table, "id", tid);

		nvlist_append_nvlist_array(nvl, "tables", table);
		nvlist_destroy(table);
	}
	return 0;
}

/*
 * Few helper routines.
 */

static void
table_ipset_flush(npf_table_t *t)
{
	npf_tblent_t *ent;

	while ((ent = LIST_FIRST(&t->t_list)) != NULL) {
		thmap_del(t->t_map, &ent->te_addr, ent->te_alen);
		LIST_REMOVE(ent, te_listent);
		pool_cache_put(tblent_cache, ent);
	}
	t->t_nitems = 0;
}

static void
table_tree_flush(npf_table_t *t)
{
	npf_tblent_t *ent;

	while ((ent = LIST_FIRST(&t->t_list)) != NULL) {
		LIST_REMOVE(ent, te_listent);
		pool_cache_put(tblent_cache, ent);
	}
	lpm_clear(t->t_lpm, NULL, NULL);
	t->t_nitems = 0;
}

static void
table_ifaddr_flush(npf_table_t *t)
{
	npf_tblent_t *ent;

	for (unsigned i = 0; i < NPF_ADDR_SLOTS; i++) {
		size_t len;

		if (!t->t_allocated[i]) {
			KASSERT(t->t_elements[i] == NULL);
			continue;
		}
		len = t->t_allocated[i] * sizeof(npf_tblent_t *);
		kmem_free(t->t_elements[i], len);
		t->t_elements[i] = NULL;
		t->t_allocated[i] = 0;
		t->t_used[i] = 0;
	}
	while ((ent = LIST_FIRST(&t->t_list)) != NULL) {
		LIST_REMOVE(ent, te_listent);
		pool_cache_put(tblent_cache, ent);
	}
	t->t_nitems = 0;
}

/*
 * npf_table_create: create table with a specified ID.
 */
npf_table_t *
npf_table_create(const char *name, u_int tid, int type,
    const void *blob, size_t size)
{
	npf_table_t *t;

	t = kmem_zalloc(sizeof(npf_table_t), KM_SLEEP);
	strlcpy(t->t_name, name, NPF_TABLE_MAXNAMELEN);

	switch (type) {
	case NPF_TABLE_LPM:
		t->t_lpm = lpm_create(KM_NOSLEEP);
		if (t->t_lpm == NULL) {
			goto out;
		}
		LIST_INIT(&t->t_list);
		break;
	case NPF_TABLE_IPSET:
		t->t_map = thmap_create(0, NULL, THMAP_NOCOPY);
		if (t->t_map == NULL) {
			goto out;
		}
		break;
	case NPF_TABLE_CONST:
		t->t_blob = kmem_alloc(size, KM_SLEEP);
		if (t->t_blob == NULL) {
			goto out;
		}
		memcpy(t->t_blob, blob, size);
		t->t_bsize = size;

		t->t_cdb = cdbr_open_mem(t->t_blob, size,
		    CDBR_DEFAULT, NULL, NULL);
		if (t->t_cdb == NULL) {
			kmem_free(t->t_blob, t->t_bsize);
			goto out;
		}
		t->t_nitems = cdbr_entries(t->t_cdb);
		break;
	case NPF_TABLE_IFADDR:
		break;
	default:
		KASSERT(false);
	}
	mutex_init(&t->t_lock, MUTEX_DEFAULT, IPL_NET);
	t->t_type = type;
	t->t_id = tid;
	return t;
out:
	kmem_free(t, sizeof(npf_table_t));
	return NULL;
}

/*
 * npf_table_destroy: free all table entries and table itself.
 */
void
npf_table_destroy(npf_table_t *t)
{
	KASSERT(t->t_refcnt == 0);

	switch (t->t_type) {
	case NPF_TABLE_IPSET:
		table_ipset_flush(t);
		npf_table_gc(NULL, t);
		thmap_destroy(t->t_map);
		break;
	case NPF_TABLE_LPM:
		table_tree_flush(t);
		lpm_destroy(t->t_lpm);
		break;
	case NPF_TABLE_CONST:
		cdbr_close(t->t_cdb);
		kmem_free(t->t_blob, t->t_bsize);
		break;
	case NPF_TABLE_IFADDR:
		table_ifaddr_flush(t);
		break;
	default:
		KASSERT(false);
	}
	mutex_destroy(&t->t_lock);
	kmem_free(t, sizeof(npf_table_t));
}

u_int
npf_table_getid(npf_table_t *t)
{
	return t->t_id;
}

/*
 * npf_table_check: validate the name, ID and type.
 */
int
npf_table_check(npf_tableset_t *ts, const char *name, uint64_t tid,
    uint64_t type, bool replacing)
{
	const npf_table_t *t;

	if (tid >= ts->ts_nitems) {
		return EINVAL;
	}
	if (!replacing && ts->ts_map[tid] != NULL) {
		return EEXIST;
	}
	switch (type) {
	case NPF_TABLE_LPM:
	case NPF_TABLE_IPSET:
	case NPF_TABLE_CONST:
	case NPF_TABLE_IFADDR:
		break;
	default:
		return EINVAL;
	}
	if (strlen(name) >= NPF_TABLE_MAXNAMELEN) {
		return ENAMETOOLONG;
	}
	if ((t = npf_tableset_getbyname(ts, name)) != NULL) {
		if (!replacing || t->t_id != tid) {
			return EEXIST;
		}
	}
	return 0;
}

static int
table_ifaddr_insert(npf_table_t *t, const int alen, npf_tblent_t *ent)
{
	const unsigned aidx = NPF_ADDRLEN2IDX(alen);
	const unsigned allocated = t->t_allocated[aidx];
	const unsigned used = t->t_used[aidx];

	/*
	 * No need to check for duplicates.
	 */
	if (allocated <= used) {
		npf_tblent_t **old_elements = t->t_elements[aidx];
		npf_tblent_t **elements;
		size_t toalloc, newsize;

		toalloc = roundup2(allocated + 1, NPF_IFADDR_STEP);
		newsize = toalloc * sizeof(npf_tblent_t *);

		elements = kmem_zalloc(newsize, KM_NOSLEEP);
		if (elements == NULL) {
			return ENOMEM;
		}
		for (unsigned i = 0; i < used; i++) {
			elements[i] = old_elements[i];
		}
		if (allocated) {
			const size_t len = allocated * sizeof(npf_tblent_t *);
			KASSERT(old_elements != NULL);
			kmem_free(old_elements, len);
		}
		t->t_elements[aidx] = elements;
		t->t_allocated[aidx] = toalloc;
	}
	t->t_elements[aidx][used] = ent;
	t->t_used[aidx]++;
	return 0;
}

/*
 * npf_table_insert: add an IP CIDR entry into the table.
 */
int
npf_table_insert(npf_table_t *t, const int alen,
    const npf_addr_t *addr, const npf_netmask_t mask)
{
	npf_tblent_t *ent;
	int error;

	error = npf_netmask_check(alen, mask);
	if (error) {
		return error;
	}
	ent = pool_cache_get(tblent_cache, PR_WAITOK);
	memcpy(&ent->te_addr, addr, alen);
	ent->te_alen = alen;
	ent->te_preflen = 0;

	/*
	 * Insert the entry.  Return an error on duplicate.
	 */
	mutex_enter(&t->t_lock);
	switch (t->t_type) {
	case NPF_TABLE_IPSET:
		/*
		 * Hashmap supports only IPs.
		 *
		 * Note: the key must be already persistent, since we
		 * use THMAP_NOCOPY.
		 */
		if (mask != NPF_NO_NETMASK) {
			error = EINVAL;
			break;
		}
		if (thmap_put(t->t_map, &ent->te_addr, alen, ent) == ent) {
			LIST_INSERT_HEAD(&t->t_list, ent, te_listent);
			t->t_nitems++;
		} else {
			error = EEXIST;
		}
		break;
	case NPF_TABLE_LPM: {
		const unsigned preflen =
		    (mask == NPF_NO_NETMASK) ? (alen * 8) : mask;
		ent->te_preflen = preflen;

		if (lpm_lookup(t->t_lpm, addr, alen) == NULL &&
		    lpm_insert(t->t_lpm, addr, alen, preflen, ent) == 0) {
			LIST_INSERT_HEAD(&t->t_list, ent, te_listent);
			t->t_nitems++;
			error = 0;
		} else {
			error = EEXIST;
		}
		break;
	}
	case NPF_TABLE_CONST:
		error = EINVAL;
		break;
	case NPF_TABLE_IFADDR:
		if ((error = table_ifaddr_insert(t, alen, ent)) != 0) {
			break;
		}
		LIST_INSERT_HEAD(&t->t_list, ent, te_listent);
		t->t_nitems++;
		break;
	default:
		KASSERT(false);
	}
	mutex_exit(&t->t_lock);

	if (error) {
		pool_cache_put(tblent_cache, ent);
	}
	return error;
}

/*
 * npf_table_remove: remove the IP CIDR entry from the table.
 */
int
npf_table_remove(npf_table_t *t, const int alen,
    const npf_addr_t *addr, const npf_netmask_t mask)
{
	npf_tblent_t *ent = NULL;
	int error;

	error = npf_netmask_check(alen, mask);
	if (error) {
		return error;
	}

	mutex_enter(&t->t_lock);
	switch (t->t_type) {
	case NPF_TABLE_IPSET:
		ent = thmap_del(t->t_map, addr, alen);
		if (__predict_true(ent != NULL)) {
			LIST_REMOVE(ent, te_listent);
			LIST_INSERT_HEAD(&t->t_gc, ent, te_listent);
			ent = NULL; // to be G/C'ed
			t->t_nitems--;
		} else {
			error = ENOENT;
		}
		break;
	case NPF_TABLE_LPM:
		ent = lpm_lookup(t->t_lpm, addr, alen);
		if (__predict_true(ent != NULL)) {
			LIST_REMOVE(ent, te_listent);
			lpm_remove(t->t_lpm, &ent->te_addr,
			    ent->te_alen, ent->te_preflen);
			t->t_nitems--;
		} else {
			error = ENOENT;
		}
		break;
	case NPF_TABLE_CONST:
	case NPF_TABLE_IFADDR:
		error = EINVAL;
		break;
	default:
		KASSERT(false);
		ent = NULL;
	}
	mutex_exit(&t->t_lock);

	if (ent) {
		pool_cache_put(tblent_cache, ent);
	}
	return error;
}

/*
 * npf_table_lookup: find the table according to ID, lookup and match
 * the contents with the specified IP address.
 */
int
npf_table_lookup(npf_table_t *t, const int alen, const npf_addr_t *addr)
{
	const void *data;
	size_t dlen;
	bool found;
	int error;

	error = npf_netmask_check(alen, NPF_NO_NETMASK);
	if (error) {
		return error;
	}

	switch (t->t_type) {
	case NPF_TABLE_IPSET:
		/* Note: the caller is in the npf_config_read_enter(). */
		found = thmap_get(t->t_map, addr, alen) != NULL;
		break;
	case NPF_TABLE_LPM:
		mutex_enter(&t->t_lock);
		found = lpm_lookup(t->t_lpm, addr, alen) != NULL;
		mutex_exit(&t->t_lock);
		break;
	case NPF_TABLE_CONST:
		if (cdbr_find(t->t_cdb, addr, alen, &data, &dlen) == 0) {
			found = dlen == (unsigned)alen &&
			    memcmp(addr, data, dlen) == 0;
		} else {
			found = false;
		}
		break;
	case NPF_TABLE_IFADDR: {
		const unsigned aidx = NPF_ADDRLEN2IDX(alen);

		found = false;
		for (unsigned i = 0; i < t->t_used[aidx]; i++) {
			const npf_tblent_t *elm = t->t_elements[aidx][i];

			KASSERT(elm->te_alen == alen);

			if (memcmp(&elm->te_addr, addr, alen) == 0) {
				found = true;
				break;
			}
		}
		break;
	}
	default:
		KASSERT(false);
		found = false;
	}

	return found ? 0 : ENOENT;
}

npf_addr_t *
npf_table_getsome(npf_table_t *t, const int alen, unsigned idx)
{
	const unsigned aidx = NPF_ADDRLEN2IDX(alen);
	npf_tblent_t *elm;
	unsigned nitems;

	KASSERT(t->t_type == NPF_TABLE_IFADDR);
	KASSERT(aidx < NPF_ADDR_SLOTS);

	nitems = t->t_used[aidx];
	if (nitems == 0) {
		return NULL;
	}

	/*
	 * No need to acquire the lock, since the table is immutable.
	 */
	elm = t->t_elements[aidx][idx % nitems];
	return &elm->te_addr;
}

static int
table_ent_copyout(const npf_addr_t *addr, const int alen, npf_netmask_t mask,
    void *ubuf, size_t len, size_t *off)
{
	void *ubufp = (uint8_t *)ubuf + *off;
	npf_ioctl_ent_t uent;

	if ((*off += sizeof(npf_ioctl_ent_t)) > len) {
		return ENOMEM;
	}
	uent.alen = alen;
	memcpy(&uent.addr, addr, sizeof(npf_addr_t));
	uent.mask = mask;

	return copyout(&uent, ubufp, sizeof(npf_ioctl_ent_t));
}

static int
table_generic_list(const npf_table_t *t, void *ubuf, size_t len)
{
	npf_tblent_t *ent;
	size_t off = 0;
	int error = 0;

	LIST_FOREACH(ent, &t->t_list, te_listent) {
		error = table_ent_copyout(&ent->te_addr,
		    ent->te_alen, ent->te_preflen, ubuf, len, &off);
		if (error)
			break;
	}
	return error;
}

static int
table_cdb_list(npf_table_t *t, void *ubuf, size_t len)
{
	size_t off = 0, dlen;
	const void *data;
	int error = 0;

	for (size_t i = 0; i < t->t_nitems; i++) {
		if (cdbr_get(t->t_cdb, i, &data, &dlen) != 0) {
			return EINVAL;
		}
		error = table_ent_copyout(data, dlen, 0, ubuf, len, &off);
		if (error)
			break;
	}
	return error;
}

/*
 * npf_table_list: copy a list of all table entries into a userspace buffer.
 */
int
npf_table_list(npf_table_t *t, void *ubuf, size_t len)
{
	int error = 0;

	mutex_enter(&t->t_lock);
	switch (t->t_type) {
	case NPF_TABLE_IPSET:
		error = table_generic_list(t, ubuf, len);
		break;
	case NPF_TABLE_LPM:
		error = table_generic_list(t, ubuf, len);
		break;
	case NPF_TABLE_CONST:
		error = table_cdb_list(t, ubuf, len);
		break;
	case NPF_TABLE_IFADDR:
		error = table_generic_list(t, ubuf, len);
		break;
	default:
		KASSERT(false);
	}
	mutex_exit(&t->t_lock);

	return error;
}

/*
 * npf_table_flush: remove all table entries.
 */
int
npf_table_flush(npf_table_t *t)
{
	int error = 0;

	mutex_enter(&t->t_lock);
	switch (t->t_type) {
	case NPF_TABLE_IPSET:
		table_ipset_flush(t);
		break;
	case NPF_TABLE_LPM:
		table_tree_flush(t);
		break;
	case NPF_TABLE_CONST:
	case NPF_TABLE_IFADDR:
		error = EINVAL;
		break;
	default:
		KASSERT(false);
	}
	mutex_exit(&t->t_lock);
	return error;
}

void
npf_table_gc(npf_t *npf, npf_table_t *t)
{
	npf_tblent_t *ent;
	void *ref;

	if (t->t_type != NPF_TABLE_IPSET || LIST_EMPTY(&t->t_gc)) {
		return;
	}

	ref = thmap_stage_gc(t->t_map);
	if (npf) {
		npf_config_locked_p(npf);
		npf_config_sync(npf);
	}
	thmap_gc(t->t_map, ref);

	while ((ent = LIST_FIRST(&t->t_gc)) != NULL) {
		LIST_REMOVE(ent, te_listent);
		pool_cache_put(tblent_cache, ent);
	}
}
