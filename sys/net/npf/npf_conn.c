/*-
 * Copyright (c) 2014-2018 Mindaugas Rasiukevicius <rmind at netbsd org>
 * Copyright (c) 2010-2014 The NetBSD Foundation, Inc.
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
 * NPF connection tracking for stateful filtering and translation.
 *
 * Overview
 *
 *	Packets can be incoming or outgoing with respect to an interface.
 *	Connection direction is identified by the direction of its first
 *	packet.  The meaning of incoming/outgoing packet in the context of
 *	connection direction can be confusing.  Therefore, we will use the
 *	terms "forwards stream" and "backwards stream", where packets in
 *	the forwards stream mean the packets travelling in the direction
 *	as the connection direction.
 *
 *	All connections have two keys and thus two entries:
 *
 *	- npf_conn_getforwkey(con)        -- for the forwards stream;
 *	- npf_conn_getbackkey(con, alen)  -- for the backwards stream.
 *
 *	Note: the keys are stored in npf_conn_t::c_keys[], which is used
 *	to allocate variable-length npf_conn_t structures based on whether
 *	the IPv4 or IPv6 addresses are used.  See the npf_connkey.c source
 *	file for the description of the key layouts.
 *
 *	The keys are formed from the 5-tuple (source/destination address,
 *	source/destination port and the protocol).  Additional matching
 *	is performed for the interface (a common behaviour is equivalent
 *	to the 6-tuple lookup including the interface ID).  Note that the
 *	key may be formed using translated values in a case of NAT.
 *
 *	Connections can serve two purposes: for the implicit passing or
 *	to accommodate the dynamic NAT.  Connections for the former purpose
 *	are created by the rules with "stateful" attribute and are used for
 *	stateful filtering.  Such connections indicate that the packet of
 *	the backwards stream should be passed without inspection of the
 *	ruleset.  The other purpose is to associate a dynamic NAT mechanism
 *	with a connection.  Such connections are created by the NAT policies
 *	and they have a relationship with NAT translation structure via
 *	npf_conn_t::c_nat.  A single connection can serve both purposes,
 *	which is a common case.
 *
 * Connection life-cycle
 *
 *	Connections are established when a packet matches said rule or
 *	NAT policy.  Both keys of the established connection are inserted
 *	into the connection database.  A garbage collection thread
 *	periodically scans all connections and depending on connection
 *	properties (e.g. last activity time, protocol) removes connection
 *	entries and expires the actual connections.
 *
 *	Each connection has a reference count.  The reference is acquired
 *	on lookup and should be released by the caller.  It guarantees that
 *	the connection will not be destroyed, although it may be expired.
 *
 * Synchronisation
 *
 *	Connection database is accessed in a lock-less manner by the main
 *	routines: npf_conn_inspect() and npf_conn_establish().  Since they
 *	are always called from a software interrupt, the database is
 *	protected using passive serialisation.  The main place which can
 *	destroy a connection is npf_conn_worker().  The database itself
 *	can be replaced and destroyed in npf_conn_reload().
 *
 * ALG support
 *
 *	Application-level gateways (ALGs) can override generic connection
 *	inspection (npf_alg_conn() call in npf_conn_inspect() function) by
 *	performing their own lookup using different key.  Recursive call
 *	to npf_conn_inspect() is not allowed.  The ALGs ought to use the
 *	npf_conn_lookup() function for this purpose.
 *
 * Lock order
 *
 *	npf_config_lock ->
 *		conn_lock ->
 *			npf_conn_t::c_lock
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_conn.c,v 1.27.2.2 2019/10/04 08:06:35 martin Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <net/pfil.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/systm.h>
#endif

#define __NPF_CONN_PRIVATE
#include "npf_conn.h"
#include "npf_impl.h"

/* A helper to select the IPv4 or IPv6 connection cache. */
#define	NPF_CONNCACHE(alen)	(((alen) >> 4) & 0x1)

/*
 * Connection flags: PFIL_IN and PFIL_OUT values are reserved for direction.
 */
CTASSERT(PFIL_ALL == (0x001 | 0x002));
#define	CONN_ACTIVE	0x004	/* visible on inspection */
#define	CONN_PASS	0x008	/* perform implicit passing */
#define	CONN_EXPIRE	0x010	/* explicitly expire */
#define	CONN_REMOVED	0x020	/* "forw/back" entries removed */

enum { CONN_TRACKING_OFF, CONN_TRACKING_ON };

static nvlist_t *npf_conn_export(npf_t *, npf_conn_t *);

/*
 * npf_conn_sys{init,fini}: initialise/destroy connection tracking.
 */

void
npf_conn_init(npf_t *npf)
{
	npf->conn_cache[0] = pool_cache_init(
	    offsetof(npf_conn_t, c_keys[NPF_CONNKEY_V4WORDS * 2]),
	    0, 0, 0, "npfcn4pl", NULL, IPL_NET, NULL, NULL, NULL);
	npf->conn_cache[1] = pool_cache_init(
	    offsetof(npf_conn_t, c_keys[NPF_CONNKEY_V6WORDS * 2]),
	    0, 0, 0, "npfcn6pl", NULL, IPL_NET, NULL, NULL, NULL);

	mutex_init(&npf->conn_lock, MUTEX_DEFAULT, IPL_NONE);
	npf->conn_tracking = CONN_TRACKING_OFF;
	npf->conn_db = npf_conndb_create();
	npf_conndb_sysinit(npf);
}

void
npf_conn_fini(npf_t *npf)
{
	npf_conndb_sysfini(npf);

	/* Note: the caller should have flushed the connections. */
	KASSERT(npf->conn_tracking == CONN_TRACKING_OFF);
	npf_worker_unregister(npf, npf_conn_worker);

	npf_conndb_destroy(npf->conn_db);
	pool_cache_destroy(npf->conn_cache[0]);
	pool_cache_destroy(npf->conn_cache[1]);
	mutex_destroy(&npf->conn_lock);
}

/*
 * npf_conn_load: perform the load by flushing the current connection
 * database and replacing it with the new one or just destroying.
 *
 * => The caller must disable the connection tracking and ensure that
 *    there are no connection database lookups or references in-flight.
 */
void
npf_conn_load(npf_t *npf, npf_conndb_t *ndb, bool track)
{
	npf_conndb_t *odb = NULL;

	KASSERT(npf_config_locked_p(npf));

	/*
	 * The connection database is in the quiescent state.
	 * Prevent G/C thread from running and install a new database.
	 */
	mutex_enter(&npf->conn_lock);
	if (ndb) {
		KASSERT(npf->conn_tracking == CONN_TRACKING_OFF);
		odb = npf->conn_db;
		npf->conn_db = ndb;
		membar_sync();
	}
	if (track) {
		/* After this point lookups start flying in. */
		npf->conn_tracking = CONN_TRACKING_ON;
	}
	mutex_exit(&npf->conn_lock);

	if (odb) {
		/*
		 * Flush all, no sync since the caller did it for us.
		 * Also, release the pool cache memory.
		 */
		npf_conndb_gc(npf, odb, true, false);
		npf_conndb_destroy(odb);
		pool_cache_invalidate(npf->conn_cache[0]);
		pool_cache_invalidate(npf->conn_cache[1]);
	}
}

/*
 * npf_conn_tracking: enable/disable connection tracking.
 */
void
npf_conn_tracking(npf_t *npf, bool track)
{
	KASSERT(npf_config_locked_p(npf));
	npf->conn_tracking = track ? CONN_TRACKING_ON : CONN_TRACKING_OFF;
}

static inline bool
npf_conn_trackable_p(const npf_cache_t *npc)
{
	const npf_t *npf = npc->npc_ctx;

	/*
	 * Check if connection tracking is on.  Also, if layer 3 and 4 are
	 * not cached - protocol is not supported or packet is invalid.
	 */
	if (npf->conn_tracking != CONN_TRACKING_ON) {
		return false;
	}
	if (!npf_iscached(npc, NPC_IP46) || !npf_iscached(npc, NPC_LAYER4)) {
		return false;
	}
	return true;
}

static inline void
conn_update_atime(npf_conn_t *con)
{
	struct timespec tsnow;

	getnanouptime(&tsnow);
	con->c_atime = tsnow.tv_sec;
}

/*
 * npf_conn_check: check that:
 *
 *	- the connection is active;
 *
 *	- the packet is travelling in the right direction with the respect
 *	  to the connection direction (if interface-id is not zero);
 *
 *	- the packet is travelling on the same interface as the
 *	  connection interface (if interface-id is not zero).
 */
static bool
npf_conn_check(const npf_conn_t *con, const nbuf_t *nbuf,
    const unsigned di, const bool forw)
{
	const uint32_t flags = con->c_flags;
	const unsigned ifid = con->c_ifid;
	bool active, pforw;

	active = (flags & (CONN_ACTIVE | CONN_EXPIRE)) == CONN_ACTIVE;
	if (__predict_false(!active)) {
		return false;
	}
	if (ifid && nbuf) {
		pforw = (flags & PFIL_ALL) == (unsigned)di;
		if (__predict_false(forw != pforw)) {
			return false;
		}
		if (__predict_false(ifid != nbuf->nb_ifid)) {
			return false;
		}
	}
	return true;
}

/*
 * npf_conn_lookup: lookup if there is an established connection.
 *
 * => If found, we will hold a reference for the caller.
 */
npf_conn_t *
npf_conn_lookup(const npf_cache_t *npc, const int di, bool *forw)
{
	npf_t *npf = npc->npc_ctx;
	const nbuf_t *nbuf = npc->npc_nbuf;
	npf_conn_t *con;
	npf_connkey_t key;

	/* Construct a key and lookup for a connection in the store. */
	if (!npf_conn_conkey(npc, &key, true)) {
		return NULL;
	}
	con = npf_conndb_lookup(npf->conn_db, &key, forw);
	if (con == NULL) {
		return NULL;
	}
	KASSERT(npc->npc_proto == con->c_proto);

	/* Extra checks for the connection and packet. */
	if (!npf_conn_check(con, nbuf, di, *forw)) {
		atomic_dec_uint(&con->c_refcnt);
		return NULL;
	}

	/* Update the last activity time. */
	conn_update_atime(con);
	return con;
}

/*
 * npf_conn_inspect: lookup a connection and inspecting the protocol data.
 *
 * => If found, we will hold a reference for the caller.
 */
npf_conn_t *
npf_conn_inspect(npf_cache_t *npc, const int di, int *error)
{
	nbuf_t *nbuf = npc->npc_nbuf;
	npf_conn_t *con;
	bool forw, ok;

	KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));
	if (!npf_conn_trackable_p(npc)) {
		return NULL;
	}

	/* Query ALG which may lookup connection for us. */
	if ((con = npf_alg_conn(npc, di)) != NULL) {
		/* Note: reference is held. */
		return con;
	}
	if (nbuf_head_mbuf(nbuf) == NULL) {
		*error = ENOMEM;
		return NULL;
	}
	KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));

	/* Main lookup of the connection. */
	if ((con = npf_conn_lookup(npc, di, &forw)) == NULL) {
		return NULL;
	}

	/* Inspect the protocol data and handle state changes. */
	mutex_enter(&con->c_lock);
	ok = npf_state_inspect(npc, &con->c_state, forw);
	mutex_exit(&con->c_lock);

	/* If invalid state: let the rules deal with it. */
	if (__predict_false(!ok)) {
		npf_conn_release(con);
		npf_stats_inc(npc->npc_ctx, NPF_STAT_INVALID_STATE);
		return NULL;
	}

	/*
	 * If this is multi-end state, then specially tag the packet
	 * so it will be just passed-through on other interfaces.
	 */
	if (con->c_ifid == 0 && nbuf_add_tag(nbuf, NPF_NTAG_PASS) != 0) {
		npf_conn_release(con);
		*error = ENOMEM;
		return NULL;
	}
	return con;
}

/*
 * npf_conn_establish: create a new connection, insert into the global list.
 *
 * => Connection is created with the reference held for the caller.
 * => Connection will be activated on the first reference release.
 */
npf_conn_t *
npf_conn_establish(npf_cache_t *npc, int di, bool global)
{
	npf_t *npf = npc->npc_ctx;
	const unsigned alen = npc->npc_alen;
	const unsigned idx = NPF_CONNCACHE(alen);
	const nbuf_t *nbuf = npc->npc_nbuf;
	npf_connkey_t *fw, *bk;
	npf_conn_t *con;
	int error = 0;

	KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));

	if (!npf_conn_trackable_p(npc)) {
		return NULL;
	}

	/* Allocate and initialise the new connection. */
	con = pool_cache_get(npf->conn_cache[idx], PR_NOWAIT);
	if (__predict_false(!con)) {
		npf_worker_signal(npf);
		return NULL;
	}
	NPF_PRINTF(("NPF: create conn %p\n", con));
	npf_stats_inc(npf, NPF_STAT_CONN_CREATE);

	mutex_init(&con->c_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	con->c_flags = (di & PFIL_ALL);
	con->c_refcnt = 0;
	con->c_rproc = NULL;
	con->c_nat = NULL;

	con->c_proto = npc->npc_proto;
	CTASSERT(sizeof(con->c_proto) >= sizeof(npc->npc_proto));
	con->c_alen = alen;

	/* Initialize the protocol state. */
	if (!npf_state_init(npc, &con->c_state)) {
		npf_conn_destroy(npf, con);
		return NULL;
	}
	KASSERT(npf_iscached(npc, NPC_IP46));

	fw = npf_conn_getforwkey(con);
	bk = npf_conn_getbackkey(con, alen);

	/*
	 * Construct "forwards" and "backwards" keys.  Also, set the
	 * interface ID for this connection (unless it is global).
	 */
	if (!npf_conn_conkey(npc, fw, true) ||
	    !npf_conn_conkey(npc, bk, false)) {
		npf_conn_destroy(npf, con);
		return NULL;
	}
	con->c_ifid = global ? nbuf->nb_ifid : 0;

	/*
	 * Set last activity time for a new connection and acquire
	 * a reference for the caller before we make it visible.
	 */
	conn_update_atime(con);
	con->c_refcnt = 1;

	/*
	 * Insert both keys (entries representing directions) of the
	 * connection.  At this point it becomes visible, but we activate
	 * the connection later.
	 */
	mutex_enter(&con->c_lock);
	if (!npf_conndb_insert(npf->conn_db, fw, con, true)) {
		error = EISCONN;
		goto err;
	}
	if (!npf_conndb_insert(npf->conn_db, bk, con, false)) {
		npf_conn_t *ret __diagused;
		ret = npf_conndb_remove(npf->conn_db, fw);
		KASSERT(ret == con);
		error = EISCONN;
		goto err;
	}
err:
	/*
	 * If we have hit the duplicate: mark the connection as expired
	 * and let the G/C thread to take care of it.  We cannot do it
	 * here since there might be references acquired already.
	 */
	if (error) {
		atomic_or_uint(&con->c_flags, CONN_REMOVED | CONN_EXPIRE);
		atomic_dec_uint(&con->c_refcnt);
		npf_stats_inc(npf, NPF_STAT_RACE_CONN);
	} else {
		NPF_PRINTF(("NPF: establish conn %p\n", con));
	}

	/* Finally, insert into the connection list. */
	npf_conndb_enqueue(npf->conn_db, con);
	mutex_exit(&con->c_lock);

	return error ? NULL : con;
}

void
npf_conn_destroy(npf_t *npf, npf_conn_t *con)
{
	const unsigned idx __unused = NPF_CONNCACHE(con->c_alen);

	KASSERT(con->c_refcnt == 0);

	if (con->c_nat) {
		/* Release any NAT structures. */
		npf_nat_destroy(con->c_nat);
	}
	if (con->c_rproc) {
		/* Release the rule procedure. */
		npf_rproc_release(con->c_rproc);
	}

	/* Destroy the state. */
	npf_state_destroy(&con->c_state);
	mutex_destroy(&con->c_lock);

	/* Free the structure, increase the counter. */
	pool_cache_put(npf->conn_cache[idx], con);
	npf_stats_inc(npf, NPF_STAT_CONN_DESTROY);
	NPF_PRINTF(("NPF: conn %p destroyed\n", con));
}

/*
 * npf_conn_setnat: associate NAT entry with the connection, update and
 * re-insert connection entry using the translation values.
 *
 * => The caller must be holding a reference.
 */
int
npf_conn_setnat(const npf_cache_t *npc, npf_conn_t *con,
    npf_nat_t *nt, unsigned ntype)
{
	static const u_int nat_type_dimap[] = {
		[NPF_NATOUT] = NPF_DST,
		[NPF_NATIN] = NPF_SRC,
	};
	npf_t *npf = npc->npc_ctx;
	npf_connkey_t key, *fw, *bk;
	npf_conn_t *ret __diagused;
	npf_addr_t *taddr;
	in_port_t tport;

	KASSERT(con->c_refcnt > 0);

	npf_nat_gettrans(nt, &taddr, &tport);
	KASSERT(ntype == NPF_NATOUT || ntype == NPF_NATIN);

	/* Construct a "backwards" key. */
	if (!npf_conn_conkey(npc, &key, false)) {
		return EINVAL;
	}

	/* Acquire the lock and check for the races. */
	mutex_enter(&con->c_lock);
	if (__predict_false(con->c_flags & CONN_EXPIRE)) {
		/* The connection got expired. */
		mutex_exit(&con->c_lock);
		return EINVAL;
	}
	KASSERT((con->c_flags & CONN_REMOVED) == 0);

	if (__predict_false(con->c_nat != NULL)) {
		/* Race with a duplicate packet. */
		mutex_exit(&con->c_lock);
		npf_stats_inc(npc->npc_ctx, NPF_STAT_RACE_NAT);
		return EISCONN;
	}

	/* Remove the "backwards" key. */
	fw = npf_conn_getforwkey(con);
	bk = npf_conn_getbackkey(con, NPF_CONNKEY_ALEN(fw));
	ret = npf_conndb_remove(npf->conn_db, bk);
	KASSERT(ret == con);

	/* Set the source/destination IDs to the translation values. */
	npf_conn_adjkey(bk, taddr, tport, nat_type_dimap[ntype]);

	/* Finally, re-insert the "backwards" key. */
	if (!npf_conndb_insert(npf->conn_db, bk, con, false)) {
		/*
		 * Race: we have hit the duplicate, remove the "forwards"
		 * key and expire our connection; it is no longer valid.
		 */
		ret = npf_conndb_remove(npf->conn_db, fw);
		KASSERT(ret == con);

		atomic_or_uint(&con->c_flags, CONN_REMOVED | CONN_EXPIRE);
		mutex_exit(&con->c_lock);

		npf_stats_inc(npc->npc_ctx, NPF_STAT_RACE_NAT);
		return EISCONN;
	}

	/* Associate the NAT entry and release the lock. */
	con->c_nat = nt;
	mutex_exit(&con->c_lock);
	return 0;
}

/*
 * npf_conn_expire: explicitly mark connection as expired.
 */
void
npf_conn_expire(npf_conn_t *con)
{
	/* KASSERT(con->c_refcnt > 0); XXX: npf_nat_freepolicy() */
	atomic_or_uint(&con->c_flags, CONN_EXPIRE);
}

/*
 * npf_conn_pass: return true if connection is "pass" one, otherwise false.
 */
bool
npf_conn_pass(const npf_conn_t *con, npf_match_info_t *mi, npf_rproc_t **rp)
{
	KASSERT(con->c_refcnt > 0);
	if (__predict_true(con->c_flags & CONN_PASS)) {
		mi->mi_rid = con->c_rid;
		mi->mi_retfl = con->c_retfl;
		*rp = con->c_rproc;
		return true;
	}
	return false;
}

/*
 * npf_conn_setpass: mark connection as a "pass" one and associate the
 * rule procedure with it.
 */
void
npf_conn_setpass(npf_conn_t *con, const npf_match_info_t *mi, npf_rproc_t *rp)
{
	KASSERT((con->c_flags & CONN_ACTIVE) == 0);
	KASSERT(con->c_refcnt > 0);
	KASSERT(con->c_rproc == NULL);

	/*
	 * No need for atomic since the connection is not yet active.
	 * If rproc is set, the caller transfers its reference to us,
	 * which will be released on npf_conn_destroy().
	 */
	atomic_or_uint(&con->c_flags, CONN_PASS);
	con->c_rproc = rp;
	if (rp) {
		con->c_rid = mi->mi_rid;
		con->c_retfl = mi->mi_retfl;
	}
}

/*
 * npf_conn_release: release a reference, which might allow G/C thread
 * to destroy this connection.
 */
void
npf_conn_release(npf_conn_t *con)
{
	if ((con->c_flags & (CONN_ACTIVE | CONN_EXPIRE)) == 0) {
		/* Activate: after this, connection is globally visible. */
		atomic_or_uint(&con->c_flags, CONN_ACTIVE);
	}
	KASSERT(con->c_refcnt > 0);
	atomic_dec_uint(&con->c_refcnt);
}

/*
 * npf_conn_getnat: return associated NAT data entry and indicate
 * whether it is a "forwards" or "backwards" stream.
 */
npf_nat_t *
npf_conn_getnat(npf_conn_t *con, const int di, bool *forw)
{
	KASSERT(con->c_refcnt > 0);
	*forw = (con->c_flags & PFIL_ALL) == (u_int)di;
	return con->c_nat;
}

/*
 * npf_conn_expired: criterion to check if connection is expired.
 */
bool
npf_conn_expired(npf_t *npf, const npf_conn_t *con, uint64_t tsnow)
{
	const int etime = npf_state_etime(npf, &con->c_state, con->c_proto);
	int elapsed;

	if (__predict_false(con->c_flags & CONN_EXPIRE)) {
		/* Explicitly marked to be expired. */
		return true;
	}

	/*
	 * Note: another thread may update 'atime' and it might
	 * become greater than 'now'.
	 */
	elapsed = (int64_t)tsnow - con->c_atime;
	return elapsed > etime;
}

/*
 * npf_conn_remove: unlink the connection and mark as expired.
 */
void
npf_conn_remove(npf_conndb_t *cd, npf_conn_t *con)
{
	/* Remove both entries of the connection. */
	mutex_enter(&con->c_lock);
	if ((con->c_flags & CONN_REMOVED) == 0) {
		npf_connkey_t *fw, *bk;
		npf_conn_t *ret __diagused;

		fw = npf_conn_getforwkey(con);
		ret = npf_conndb_remove(cd, fw);
		KASSERT(ret == con);

		bk = npf_conn_getbackkey(con, NPF_CONNKEY_ALEN(fw));
		ret = npf_conndb_remove(cd, bk);
		KASSERT(ret == con);
	}

	/* Flag the removal and expiration. */
	atomic_or_uint(&con->c_flags, CONN_REMOVED | CONN_EXPIRE);
	mutex_exit(&con->c_lock);
}

/*
 * npf_conn_worker: G/C to run from a worker thread.
 */
void
npf_conn_worker(npf_t *npf)
{
	npf_conndb_gc(npf, npf->conn_db, false, true);
}

/*
 * npf_conndb_export: construct a list of connections prepared for saving.
 * Note: this is expected to be an expensive operation.
 */
int
npf_conndb_export(npf_t *npf, nvlist_t *npf_dict)
{
	npf_conn_t *head, *con;

	/*
	 * Note: acquire conn_lock to prevent from the database
	 * destruction and G/C thread.
	 */
	mutex_enter(&npf->conn_lock);
	if (npf->conn_tracking != CONN_TRACKING_ON) {
		mutex_exit(&npf->conn_lock);
		return 0;
	}
	head = npf_conndb_getlist(npf->conn_db);
	con = head;
	while (con) {
		nvlist_t *cdict;

		if ((cdict = npf_conn_export(npf, con)) != NULL) {
			nvlist_append_nvlist_array(npf_dict, "conn-list", cdict);
			nvlist_destroy(cdict);
		}
		if ((con = npf_conndb_getnext(npf->conn_db, con)) == head) {
			break;
		}
	}
	mutex_exit(&npf->conn_lock);
	return 0;
}

/*
 * npf_conn_export: serialise a single connection.
 */
static nvlist_t *
npf_conn_export(npf_t *npf, npf_conn_t *con)
{
	nvlist_t *cdict, *kdict;
	npf_connkey_t *fw, *bk;
	unsigned alen;

	if ((con->c_flags & (CONN_ACTIVE|CONN_EXPIRE)) != CONN_ACTIVE) {
		return NULL;
	}
	cdict = nvlist_create(0);
	nvlist_add_number(cdict, "flags", con->c_flags);
	nvlist_add_number(cdict, "proto", con->c_proto);
	if (con->c_ifid) {
		char ifname[IFNAMSIZ];
		npf_ifmap_copyname(npf, con->c_ifid, ifname, sizeof(ifname));
		nvlist_add_string(cdict, "ifname", ifname);
	}
	nvlist_add_binary(cdict, "state", &con->c_state, sizeof(npf_state_t));

	fw = npf_conn_getforwkey(con);
	alen = NPF_CONNKEY_ALEN(fw);
	KASSERT(alen == con->c_alen);
	bk = npf_conn_getbackkey(con, alen);

	kdict = npf_connkey_export(fw);
	nvlist_move_nvlist(cdict, "forw-key", kdict);

	kdict = npf_connkey_export(bk);
	nvlist_move_nvlist(cdict, "back-key", kdict);

	/* Let the address length be based on on first key. */
	nvlist_add_number(cdict, "alen", alen);

	if (con->c_nat) {
		npf_nat_export(cdict, con->c_nat);
	}
	return cdict;
}

/*
 * npf_conn_import: fully reconstruct a single connection from a
 * nvlist and insert into the given database.
 */
int
npf_conn_import(npf_t *npf, npf_conndb_t *cd, const nvlist_t *cdict,
    npf_ruleset_t *natlist)
{
	npf_conn_t *con;
	npf_connkey_t *fw, *bk;
	const nvlist_t *nat, *conkey;
	const char *ifname;
	const void *state;
	unsigned alen, idx;
	size_t len;

	/*
	 * To determine the length of the connection, which depends
	 * on the address length in the connection keys.
	 */
	alen = dnvlist_get_number(cdict, "alen", 0);
	idx = NPF_CONNCACHE(alen);

	/* Allocate a connection and initialise it (clear first). */
	con = pool_cache_get(npf->conn_cache[idx], PR_WAITOK);
	memset(con, 0, sizeof(npf_conn_t));
	mutex_init(&con->c_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	npf_stats_inc(npf, NPF_STAT_CONN_CREATE);

	con->c_proto = dnvlist_get_number(cdict, "proto", 0);
	con->c_flags = dnvlist_get_number(cdict, "flags", 0);
	con->c_flags &= PFIL_ALL | CONN_ACTIVE | CONN_PASS;
	conn_update_atime(con);

	ifname = dnvlist_get_string(cdict, "ifname", NULL);
	if (ifname && (con->c_ifid = npf_ifmap_register(npf, ifname)) == 0) {
		goto err;
	}

	state = dnvlist_get_binary(cdict, "state", &len, NULL, 0);
	if (!state || len != sizeof(npf_state_t)) {
		goto err;
	}
	memcpy(&con->c_state, state, sizeof(npf_state_t));

	/* Reconstruct NAT association, if any. */
	if ((nat = dnvlist_get_nvlist(cdict, "nat", NULL)) != NULL &&
	    (con->c_nat = npf_nat_import(npf, nat, natlist, con)) == NULL) {
		goto err;
	}

	/*
	 * Fetch and copy the keys for each direction.
	 */
	fw = npf_conn_getforwkey(con);
	conkey = dnvlist_get_nvlist(cdict, "forw-key", NULL);
	if (conkey == NULL || !npf_connkey_import(conkey, fw)) {
		goto err;
	}
	bk = npf_conn_getbackkey(con, NPF_CONNKEY_ALEN(fw));
	conkey = dnvlist_get_nvlist(cdict, "back-key", NULL);
	if (conkey == NULL || !npf_connkey_import(conkey, bk)) {
		goto err;
	}

	/* Guard against the contradicting address lengths. */
	if (NPF_CONNKEY_ALEN(fw) != alen || NPF_CONNKEY_ALEN(bk) != alen) {
		goto err;
	}

	/* Insert the entries and the connection itself. */
	if (!npf_conndb_insert(cd, fw, con, true)) {
		goto err;
	}
	if (!npf_conndb_insert(cd, bk, con, false)) {
		npf_conndb_remove(cd, fw);
		goto err;
	}

	NPF_PRINTF(("NPF: imported conn %p\n", con));
	npf_conndb_enqueue(cd, con);
	return 0;
err:
	npf_conn_destroy(npf, con);
	return EINVAL;
}

int
npf_conn_find(npf_t *npf, const nvlist_t *idict, nvlist_t **odict)
{
	const nvlist_t *kdict;
	npf_connkey_t key;
	npf_conn_t *con;
	uint16_t dir;
	bool forw;

	kdict = dnvlist_get_nvlist(idict, "key", NULL);
	if (!kdict || !npf_connkey_import(kdict, &key)) {
		return EINVAL;
	}
	con = npf_conndb_lookup(npf->conn_db, &key, &forw);
	if (con == NULL) {
		return ESRCH;
	}
	dir = dnvlist_get_number(idict, "direction", 0);
	if (!npf_conn_check(con, NULL, dir, true)) {
		atomic_dec_uint(&con->c_refcnt);
		return ESRCH;
	}
	*odict = npf_conn_export(npf, con);
	atomic_dec_uint(&con->c_refcnt);
	return *odict ? 0 : ENOSPC;
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_conn_print(npf_conn_t *con)
{
	const npf_connkey_t *fw = npf_conn_getforwkey(con);
	const npf_connkey_t *bk = npf_conn_getbackkey(con, NPF_CONNKEY_ALEN(fw));
	const unsigned proto = con->c_proto;
	struct timespec tspnow;

	getnanouptime(&tspnow);
	printf("%p:\n\tproto %d flags 0x%x tsdiff %ld etime %d\n", con,
	    proto, con->c_flags, (long)(tspnow.tv_sec - con->c_atime),
	    npf_state_etime(npf_getkernctx(), &con->c_state, proto));
	npf_connkey_print(fw);
	npf_connkey_print(bk);
	npf_state_dump(&con->c_state);
	if (con->c_nat) {
		npf_nat_dump(con->c_nat);
	}
}

#endif
