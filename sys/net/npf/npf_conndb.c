/*-
 * Copyright (c) 2010-2018 The NetBSD Foundation, Inc.
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
 * NPF connection storage.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_conndb.c,v 1.3.16.2 2020/04/08 14:08:57 martin Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/thmap.h>
#endif

#define __NPF_CONN_PRIVATE
#include "npf_conn.h"
#include "npf_impl.h"

#define	NPF_GC_STEP		256

struct npf_conndb {
	thmap_t *		cd_map;

	/*
	 * There are three lists for connections: new, all and G/C.
	 *
	 * New connections are atomically inserted into the "new-list".
	 * The G/C worker will move them to the doubly-linked list of all
	 * active connections.
	 */
	npf_conn_t *		cd_new;
	LIST_HEAD(, npf_conn)	cd_list;
	LIST_HEAD(, npf_conn)	cd_gclist;

	/* The last inspected connection (for circular iteration). */
	npf_conn_t *		cd_marker;
};

npf_conndb_t *
npf_conndb_create(void)
{
	npf_conndb_t *cd;

	cd = kmem_zalloc(sizeof(npf_conndb_t), KM_SLEEP);
	cd->cd_map = thmap_create(0, NULL, THMAP_NOCOPY);
	KASSERT(cd->cd_map != NULL);

	LIST_INIT(&cd->cd_list);
	LIST_INIT(&cd->cd_gclist);
	return cd;
}

void
npf_conndb_destroy(npf_conndb_t *cd)
{
	KASSERT(cd->cd_new == NULL);
	KASSERT(cd->cd_marker == NULL);
	KASSERT(LIST_EMPTY(&cd->cd_list));
	KASSERT(LIST_EMPTY(&cd->cd_gclist));

	thmap_destroy(cd->cd_map);
	kmem_free(cd, sizeof(npf_conndb_t));
}

/*
 * npf_conndb_lookup: find a connection given the key.
 */
npf_conn_t *
npf_conndb_lookup(npf_conndb_t *cd, const npf_connkey_t *ck, bool *forw)
{
	const unsigned keylen = NPF_CONN_KEYLEN(ck);
	npf_connkey_t *foundkey;
	npf_conn_t *con;

	/*
	 * Lookup the connection key in the key-value map.
	 */
	foundkey = thmap_get(cd->cd_map, ck->ck_key, keylen);
	if (!foundkey) {
		return NULL;
	}
	con = foundkey->ck_backptr;
	KASSERT(con != NULL);

	/*
	 * Acquire a reference and return the connection.
	 */
	atomic_inc_uint(&con->c_refcnt);
	*forw = (foundkey == &con->c_forw_entry);
	return con;
}

/*
 * npf_conndb_insert: insert the key representing the connection.
 *
 * => Returns true on success and false on failure.
 */
bool
npf_conndb_insert(npf_conndb_t *cd, npf_connkey_t *ck)
{
	const unsigned keylen = NPF_CONN_KEYLEN(ck);
	return thmap_put(cd->cd_map, ck->ck_key, keylen, ck) == ck;
}

/*
 * npf_conndb_remove: find and delete connection key, returning the
 * connection it represents.
 */
npf_conn_t *
npf_conndb_remove(npf_conndb_t *cd, npf_connkey_t *ck)
{
	const unsigned keylen = NPF_CONN_KEYLEN(ck);
	npf_connkey_t *foundkey;
	npf_conn_t *con;

	foundkey = thmap_del(cd->cd_map, ck->ck_key, keylen);
	if (!foundkey) {
		return NULL;
	}
	con = foundkey->ck_backptr;
	KASSERT(con != NULL);
	return con;
}

/*
 * npf_conndb_enqueue: atomically insert the connection into the
 * singly-linked list of the "new" connections.
 */
void
npf_conndb_enqueue(npf_conndb_t *cd, npf_conn_t *con)
{
	npf_conn_t *head;

	do {
		head = cd->cd_new;
		con->c_next = head;
	} while (atomic_cas_ptr(&cd->cd_new, head, con) != head);
}

/*
 * npf_conndb_update: migrate all new connections to the list of all
 * connections; this must also be performed on npf_conndb_getlist()
 * to provide a complete list of connections.
 */
static void
npf_conndb_update(npf_conndb_t *cd)
{
	npf_conn_t *con;

	con = atomic_swap_ptr(&cd->cd_new, NULL);
	while (con) {
		npf_conn_t *next = con->c_next; // union
		LIST_INSERT_HEAD(&cd->cd_list, con, c_entry);
		con = next;
	}
}

/*
 * npf_conndb_getlist: return the list of all connections.
 */
npf_conn_t *
npf_conndb_getlist(npf_conndb_t *cd)
{
	npf_conndb_update(cd);
	return LIST_FIRST(&cd->cd_list);
}

/*
 * npf_conndb_getnext: return the next connection, implementing
 * the circular iteration.
 */
npf_conn_t *
npf_conndb_getnext(npf_conndb_t *cd, npf_conn_t *con)
{
	/* Next.. */
	if (con == NULL || (con = LIST_NEXT(con, c_entry)) == NULL) {
		con = LIST_FIRST(&cd->cd_list);
	}
	return con;
}

/*
 * npf_conndb_gc_incr: incremental G/C of the expired connections.
 */
static void
npf_conndb_gc_incr(npf_conndb_t *cd, const time_t now)
{
	unsigned target = NPF_GC_STEP;
	npf_conn_t *con;

	/*
	 * Second, start from the "last" (marker) connection.
	 * We must initialise the marker if it is not set yet.
	 */
	if ((con = cd->cd_marker) == NULL) {
		con = npf_conndb_getnext(cd, NULL);
		cd->cd_marker = con;
	}

	/*
	 * Scan the connections:
	 * - Limit the scan to the G/C step size.
	 * - Stop if we scanned all of them.
	 * - Update the marker connection.
	 */
	while (con && target--) {
		npf_conn_t *next = npf_conndb_getnext(cd, con);

		/*
		 * Can we G/C this connection?
		 */
		if (npf_conn_expired(con, now)) {
			/* Yes: move to the G/C list. */
			LIST_REMOVE(con, c_entry);
			LIST_INSERT_HEAD(&cd->cd_gclist, con, c_entry);
			npf_conn_remove(cd, con);

			/* This connection cannot be a new marker anymore. */
			if (con == next) {
				next = NULL;
			}
			if (con == cd->cd_marker) {
				cd->cd_marker = next;
				con = next;
				continue;
			}
		}
		con = next;

		/*
		 * Circular iteration: if we returned back to the
		 * marker connection, then stop.
		 */
		if (con == cd->cd_marker) {
			break;
		}
	}
	cd->cd_marker = con;
}

/*
 * npf_conndb_gc: garbage collect the expired connections.
 *
 * => Must run in a single-threaded manner.
 * => If 'flush' is true, then destroy all connections.
 * => If 'sync' is true, then perform passive serialisation.
 */
void
npf_conndb_gc(npf_t *npf, npf_conndb_t *cd, bool flush, bool sync)
{
	struct timespec tsnow;
	npf_conn_t *con;
	void *gcref;

	getnanouptime(&tsnow);

	/* First, migrate all new connections. */
	mutex_enter(&npf->conn_lock);
	npf_conndb_update(cd);
	if (flush) {
		/* Just unlink and move all connections to the G/C list. */
		while ((con = LIST_FIRST(&cd->cd_list)) != NULL) {
			LIST_REMOVE(con, c_entry);
			LIST_INSERT_HEAD(&cd->cd_gclist, con, c_entry);
			npf_conn_remove(cd, con);
		}
		cd->cd_marker = NULL;
	} else {
		/* Incremental G/C of the expired connections. */
		npf_conndb_gc_incr(cd, tsnow.tv_sec);
	}
	mutex_exit(&npf->conn_lock);

	/*
	 * Ensure it is safe to destroy the connections.
	 * Note: drop the conn_lock (see the lock order).
	 */
	gcref = thmap_stage_gc(cd->cd_map);
	if (sync && (gcref || !LIST_EMPTY(&cd->cd_gclist))) {
		npf_config_enter(npf);
		npf_config_sync(npf);
		npf_config_exit(npf);
	}
	thmap_gc(cd->cd_map, gcref);

	/*
	 * If there is nothing to G/C, then reduce the worker interval.
	 * We do not go below the lower watermark.
	 */
	if (LIST_EMPTY(&cd->cd_gclist)) {
		// TODO: cd->next_gc = MAX(cd->next_gc >> 1, NPF_MIN_GC_TIME);
		return;
	}

	/*
	 * Garbage collect all expired connections.
	 * May need to wait for the references to drain.
	 */
	while ((con = LIST_FIRST(&cd->cd_gclist)) != NULL) {
		/*
		 * Destroy only if removed and no references.  Otherwise,
		 * just do it next time, unless we are destroying all.
		 */
		if (__predict_false(con->c_refcnt)) {
			if (!flush) {
				break;
			}
			kpause("npfcongc", false, 1, NULL);
			continue;
		}
		LIST_REMOVE(con, c_entry);
		npf_conn_destroy(npf, con);
	}
}
