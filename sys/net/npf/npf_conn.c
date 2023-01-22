/*-
 * Copyright (c) 2014-2020 Mindaugas Rasiukevicius <rmind at noxt eu>
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
 *	the IPv4 or IPv6 addresses are used.
 *
 *	The key is an n-tuple used to identify the connection flow: see the
 *	npf_connkey.c source file for the description of the key layouts.
 *	The key may be formed using translated values in a case of NAT.
 *
 *	Connections can serve two purposes: for the implicit passing and/or
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
 * Synchronization
 *
 *	Connection database is accessed in a lock-free manner by the main
 *	routines: npf_conn_inspect() and npf_conn_establish().  Since they
 *	are always called from a software interrupt, the database is
 *	protected using EBR.  The main place which can destroy a connection
 *	is npf_conn_worker().  The database itself can be replaced and
 *	destroyed in npf_conn_reload().
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
 *	npf_t::config_lock ->
 *		conn_lock ->
 *			npf_conn_t::c_lock
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_conn.c,v 1.35 2023/01/22 18:39:35 riastradh Exp $");

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

static int	npf_conn_export(npf_t *, npf_conn_t *, nvlist_t *);

/*
 * npf_conn_sys{init,fini}: initialize/destroy connection tracking.
 */

void
npf_conn_init(npf_t *npf)
{
	npf_conn_params_t *params = npf_param_allocgroup(npf,
	    NPF_PARAMS_CONN, sizeof(npf_conn_params_t));
	npf_param_t param_map[] = {
		{
			"state.key.interface",
			&params->connkey_interface,
			.default_val = 1, // true
			.min = 0, .max = 1
		},
		{
			"state.key.direction",
			&params->connkey_direction,
			.default_val = 1, // true
			.min = 0, .max = 1
		},
	};
	npf_param_register(npf, param_map, __arraycount(param_map));

	npf->conn_cache[0] = pool_cache_init(
	    offsetof(npf_conn_t, c_keys[NPF_CONNKEY_V4WORDS * 2]),
	    0, 0, 0, "npfcn4pl", NULL, IPL_NET, NULL, NULL, NULL);
	npf->conn_cache[1] = pool_cache_init(
	    offsetof(npf_conn_t, c_keys[NPF_CONNKEY_V6WORDS * 2]),
	    0, 0, 0, "npfcn6pl", NULL, IPL_NET, NULL, NULL, NULL);

	mutex_init(&npf->conn_lock, MUTEX_DEFAULT, IPL_NONE);
	atomic_store_relaxed(&npf->conn_tracking, CONN_TRACKING_OFF);
	npf->conn_db = npf_conndb_create();
	npf_conndb_sysinit(npf);

	npf_worker_addfunc(npf, npf_conn_worker);
}

void
npf_conn_fini(npf_t *npf)
{
	const size_t len = sizeof(npf_conn_params_t);

	/* Note: the caller should have flushed the connections. */
	KASSERT(atomic_load_relaxed(&npf->conn_tracking) == CONN_TRACKING_OFF);

	npf_conndb_destroy(npf->conn_db);
	pool_cache_destroy(npf->conn_cache[0]);
	pool_cache_destroy(npf->conn_cache[1]);
	mutex_destroy(&npf->conn_lock);

	npf_param_freegroup(npf, NPF_PARAMS_CONN, len);
	npf_conndb_sysfini(npf);
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
		KASSERT(atomic_load_relaxed(&npf->conn_tracking)
		    == CONN_TRACKING_OFF);
		odb = atomic_load_relaxed(&npf->conn_db);
		atomic_store_release(&npf->conn_db, ndb);
	}
	if (track) {
		/* After this point lookups start flying in. */
		membar_producer();
		atomic_store_relaxed(&npf->conn_tracking, CONN_TRACKING_ON);
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
	atomic_store_relaxed(&npf->conn_tracking,
	    track ? CONN_TRACKING_ON : CONN_TRACKING_OFF);
}

static inline bool
npf_conn_trackable_p(const npf_cache_t *npc)
{
	const npf_t *npf = npc->npc_ctx;

	/*
	 * Check if connection tracking is on.  Also, if layer 3 and 4 are
	 * not cached - protocol is not supported or packet is invalid.
	 */
	if (atomic_load_relaxed(&npf->conn_tracking) != CONN_TRACKING_ON) {
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
	atomic_store_relaxed(&con->c_atime, tsnow.tv_sec);
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
    const unsigned di, const npf_flow_t flow)
{
	const uint32_t flags = atomic_load_relaxed(&con->c_flags);
	const unsigned ifid = atomic_load_relaxed(&con->c_ifid);
	bool active;

	active = (flags & (CONN_ACTIVE | CONN_EXPIRE)) == CONN_ACTIVE;
	if (__predict_false(!active)) {
		return false;
	}
	if (ifid && nbuf) {
		const bool match = (flags & PFIL_ALL) == di;
		npf_flow_t pflow = match ? NPF_FLOW_FORW : NPF_FLOW_BACK;

		if (__predict_false(flow != pflow)) {
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
npf_conn_lookup(const npf_cache_t *npc, const unsigned di, npf_flow_t *flow)
{
	npf_t *npf = npc->npc_ctx;
	const nbuf_t *nbuf = npc->npc_nbuf;
	npf_conn_t *con;
	npf_connkey_t key;

	/* Construct a key and lookup for a connection in the store. */
	if (!npf_conn_conkey(npc, &key, di, NPF_FLOW_FORW)) {
		return NULL;
	}
	con = npf_conndb_lookup(npf, &key, flow);
	if (con == NULL) {
		return NULL;
	}
	KASSERT(npc->npc_proto == atomic_load_relaxed(&con->c_proto));

	/* Extra checks for the connection and packet. */
	if (!npf_conn_check(con, nbuf, di, *flow)) {
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
npf_conn_inspect(npf_cache_t *npc, const unsigned di, int *error)
{
	nbuf_t *nbuf = npc->npc_nbuf;
	npf_flow_t flow;
	npf_conn_t *con;
	bool ok;

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

	/* The main lookup of the connection (acquires a reference). */
	if ((con = npf_conn_lookup(npc, di, &flow)) == NULL) {
		return NULL;
	}

	/* Inspect the protocol data and handle state changes. */
	mutex_enter(&con->c_lock);
	ok = npf_state_inspect(npc, &con->c_state, flow);
	mutex_exit(&con->c_lock);

	/* If invalid state: let the rules deal with it. */
	if (__predict_false(!ok)) {
		npf_conn_release(con);
		npf_stats_inc(npc->npc_ctx, NPF_STAT_INVALID_STATE);
		return NULL;
	}
#if 0
	/*
	 * TODO -- determine when this might be wanted/used.
	 *
	 * Note: skipping the connection lookup and ruleset inspection
	 * on other interfaces will also bypass dynamic NAT.
	 */
	if (atomic_load_relaxed(&con->c_flags) & CONN_GPASS) {
		/*
		 * Note: if tagging fails, then give this packet a chance
		 * to go through a regular ruleset.
		 */
		(void)nbuf_add_tag(nbuf, NPF_NTAG_PASS);
	}
#endif
	return con;
}

/*
 * npf_conn_establish: create a new connection, insert into the global list.
 *
 * => Connection is created with the reference held for the caller.
 * => Connection will be activated on the first reference release.
 */
npf_conn_t *
npf_conn_establish(npf_cache_t *npc, const unsigned di, bool global)
{
	npf_t *npf = npc->npc_ctx;
	const unsigned alen = npc->npc_alen;
	const unsigned idx = NPF_CONNCACHE(alen);
	const nbuf_t *nbuf = npc->npc_nbuf;
	npf_connkey_t *fw, *bk;
	npf_conndb_t *conn_db;
	npf_conn_t *con;
	int error = 0;

	KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));

	if (!npf_conn_trackable_p(npc)) {
		return NULL;
	}

	/* Allocate and initialize the new connection. */
	con = pool_cache_get(npf->conn_cache[idx], PR_NOWAIT);
	if (__predict_false(!con)) {
		npf_worker_signal(npf);
		return NULL;
	}
	NPF_PRINTF(("NPF: create conn %p\n", con));
	npf_stats_inc(npf, NPF_STAT_CONN_CREATE);

	mutex_init(&con->c_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	atomic_store_relaxed(&con->c_flags, di & PFIL_ALL);
	atomic_store_relaxed(&con->c_refcnt, 0);
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
	if (!npf_conn_conkey(npc, fw, di, NPF_FLOW_FORW) ||
	    !npf_conn_conkey(npc, bk, di ^ PFIL_ALL, NPF_FLOW_BACK)) {
		npf_conn_destroy(npf, con);
		return NULL;
	}
	con->c_ifid = global ? nbuf->nb_ifid : 0;

	/*
	 * Set last activity time for a new connection and acquire
	 * a reference for the caller before we make it visible.
	 */
	conn_update_atime(con);
	atomic_store_relaxed(&con->c_refcnt, 1);

	/*
	 * Insert both keys (entries representing directions) of the
	 * connection.  At this point it becomes visible, but we activate
	 * the connection later.
	 */
	mutex_enter(&con->c_lock);
	conn_db = atomic_load_consume(&npf->conn_db);
	if (!npf_conndb_insert(conn_db, fw, con, NPF_FLOW_FORW)) {
		error = EISCONN;
		goto err;
	}
	if (!npf_conndb_insert(conn_db, bk, con, NPF_FLOW_BACK)) {
		npf_conn_t *ret __diagused;
		ret = npf_conndb_remove(conn_db, fw);
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
	npf_conndb_enqueue(conn_db, con);
	mutex_exit(&con->c_lock);

	return error ? NULL : con;
}

void
npf_conn_destroy(npf_t *npf, npf_conn_t *con)
{
	const unsigned idx __unused = NPF_CONNCACHE(con->c_alen);

	KASSERT(atomic_load_relaxed(&con->c_refcnt) == 0);

	if (con->c_nat) {
		/* Release any NAT structures. */
		npf_nat_destroy(con, con->c_nat);
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
	static const unsigned nat_type_which[] = {
		/* See the description in npf_nat_which(). */
		[NPF_NATOUT] = NPF_DST,
		[NPF_NATIN] = NPF_SRC,
	};
	npf_t *npf = npc->npc_ctx;
	npf_conn_t *ret __diagused;
	npf_conndb_t *conn_db;
	npf_connkey_t *bk;
	npf_addr_t *taddr;
	in_port_t tport;
	uint32_t flags;

	KASSERT(atomic_load_relaxed(&con->c_refcnt) > 0);

	npf_nat_gettrans(nt, &taddr, &tport);
	KASSERT(ntype == NPF_NATOUT || ntype == NPF_NATIN);

	/* Acquire the lock and check for the races. */
	mutex_enter(&con->c_lock);
	flags = atomic_load_relaxed(&con->c_flags);
	if (__predict_false(flags & CONN_EXPIRE)) {
		/* The connection got expired. */
		mutex_exit(&con->c_lock);
		return EINVAL;
	}
	KASSERT((flags & CONN_REMOVED) == 0);

	if (__predict_false(con->c_nat != NULL)) {
		/* Race with a duplicate packet. */
		mutex_exit(&con->c_lock);
		npf_stats_inc(npc->npc_ctx, NPF_STAT_RACE_NAT);
		return EISCONN;
	}

	/* Remove the "backwards" key. */
	conn_db = atomic_load_consume(&npf->conn_db);
	bk = npf_conn_getbackkey(con, con->c_alen);
	ret = npf_conndb_remove(conn_db, bk);
	KASSERT(ret == con);

	/* Set the source/destination IDs to the translation values. */
	npf_conn_adjkey(bk, taddr, tport, nat_type_which[ntype]);

	/* Finally, re-insert the "backwards" key. */
	if (!npf_conndb_insert(conn_db, bk, con, NPF_FLOW_BACK)) {
		/*
		 * Race: we have hit the duplicate, remove the "forwards"
		 * key and expire our connection; it is no longer valid.
		 */
		npf_connkey_t *fw = npf_conn_getforwkey(con);
		ret = npf_conndb_remove(conn_db, fw);
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
 *
 * => Must be called with: a) reference held  b) the relevant lock held.
 *    The relevant lock should prevent from connection destruction, e.g.
 *    npf_t::conn_lock or npf_natpolicy_t::n_lock.
 */
void
npf_conn_expire(npf_conn_t *con)
{
	atomic_or_uint(&con->c_flags, CONN_EXPIRE);
}

/*
 * npf_conn_pass: return true if connection is "pass" one, otherwise false.
 */
bool
npf_conn_pass(const npf_conn_t *con, npf_match_info_t *mi, npf_rproc_t **rp)
{
	KASSERT(atomic_load_relaxed(&con->c_refcnt) > 0);
	if (__predict_true(atomic_load_relaxed(&con->c_flags) & CONN_PASS)) {
		mi->mi_retfl = atomic_load_relaxed(&con->c_retfl);
		mi->mi_rid = con->c_rid;
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
	KASSERT((atomic_load_relaxed(&con->c_flags) & CONN_ACTIVE) == 0);
	KASSERT(atomic_load_relaxed(&con->c_refcnt) > 0);
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
	const unsigned flags = atomic_load_relaxed(&con->c_flags);

	if ((flags & (CONN_ACTIVE | CONN_EXPIRE)) == 0) {
		/* Activate: after this, connection is globally visible. */
		atomic_or_uint(&con->c_flags, CONN_ACTIVE);
	}
	KASSERT(atomic_load_relaxed(&con->c_refcnt) > 0);
	atomic_dec_uint(&con->c_refcnt);
}

/*
 * npf_conn_getnat: return the associated NAT entry, if any.
 */
npf_nat_t *
npf_conn_getnat(const npf_conn_t *con)
{
	return con->c_nat;
}

/*
 * npf_conn_expired: criterion to check if connection is expired.
 */
bool
npf_conn_expired(npf_t *npf, const npf_conn_t *con, uint64_t tsnow)
{
	const unsigned flags = atomic_load_relaxed(&con->c_flags);
	const int etime = npf_state_etime(npf, &con->c_state, con->c_proto);
	int elapsed;

	if (__predict_false(flags & CONN_EXPIRE)) {
		/* Explicitly marked to be expired. */
		return true;
	}

	/*
	 * Note: another thread may update 'atime' and it might
	 * become greater than 'now'.
	 */
	elapsed = (int64_t)tsnow - atomic_load_relaxed(&con->c_atime);
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
	if ((atomic_load_relaxed(&con->c_flags) & CONN_REMOVED) == 0) {
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
 * npf_conn_worker: G/C to run from a worker thread or via npfk_gc().
 */
void
npf_conn_worker(npf_t *npf)
{
	npf_conndb_t *conn_db = atomic_load_consume(&npf->conn_db);
	npf_conndb_gc(npf, conn_db, false, true);
}

/*
 * npf_conndb_export: construct a list of connections prepared for saving.
 * Note: this is expected to be an expensive operation.
 */
int
npf_conndb_export(npf_t *npf, nvlist_t *nvl)
{
	npf_conn_t *head, *con;
	npf_conndb_t *conn_db;

	/*
	 * Note: acquire conn_lock to prevent from the database
	 * destruction and G/C thread.
	 */
	mutex_enter(&npf->conn_lock);
	if (atomic_load_relaxed(&npf->conn_tracking) != CONN_TRACKING_ON) {
		mutex_exit(&npf->conn_lock);
		return 0;
	}
	conn_db = atomic_load_relaxed(&npf->conn_db);
	head = npf_conndb_getlist(conn_db);
	con = head;
	while (con) {
		nvlist_t *con_nvl;

		con_nvl = nvlist_create(0);
		if (npf_conn_export(npf, con, con_nvl) == 0) {
			nvlist_append_nvlist_array(nvl, "conn-list", con_nvl);
		}
		nvlist_destroy(con_nvl);

		if ((con = npf_conndb_getnext(conn_db, con)) == head) {
			break;
		}
	}
	mutex_exit(&npf->conn_lock);
	return 0;
}

/*
 * npf_conn_export: serialize a single connection.
 */
static int
npf_conn_export(npf_t *npf, npf_conn_t *con, nvlist_t *nvl)
{
	nvlist_t *knvl;
	npf_connkey_t *fw, *bk;
	unsigned flags, alen;

	flags = atomic_load_relaxed(&con->c_flags);
	if ((flags & (CONN_ACTIVE|CONN_EXPIRE)) != CONN_ACTIVE) {
		return ESRCH;
	}
	nvlist_add_number(nvl, "flags", flags);
	nvlist_add_number(nvl, "proto", con->c_proto);
	if (con->c_ifid) {
		char ifname[IFNAMSIZ];
		npf_ifmap_copyname(npf, con->c_ifid, ifname, sizeof(ifname));
		nvlist_add_string(nvl, "ifname", ifname);
	}
	nvlist_add_binary(nvl, "state", &con->c_state, sizeof(npf_state_t));

	fw = npf_conn_getforwkey(con);
	alen = NPF_CONNKEY_ALEN(fw);
	KASSERT(alen == con->c_alen);
	bk = npf_conn_getbackkey(con, alen);

	knvl = npf_connkey_export(npf, fw);
	nvlist_move_nvlist(nvl, "forw-key", knvl);

	knvl = npf_connkey_export(npf, bk);
	nvlist_move_nvlist(nvl, "back-key", knvl);

	/* Let the address length be based on on first key. */
	nvlist_add_number(nvl, "alen", alen);

	if (con->c_nat) {
		npf_nat_export(npf, con->c_nat, nvl);
	}
	return 0;
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
	unsigned flags, alen, idx;
	const char *ifname;
	const void *state;
	size_t len;

	/*
	 * To determine the length of the connection, which depends
	 * on the address length in the connection keys.
	 */
	alen = dnvlist_get_number(cdict, "alen", 0);
	idx = NPF_CONNCACHE(alen);

	/* Allocate a connection and initialize it (clear first). */
	con = pool_cache_get(npf->conn_cache[idx], PR_WAITOK);
	memset(con, 0, sizeof(npf_conn_t));
	mutex_init(&con->c_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	npf_stats_inc(npf, NPF_STAT_CONN_CREATE);

	con->c_proto = dnvlist_get_number(cdict, "proto", 0);
	flags = dnvlist_get_number(cdict, "flags", 0);
	flags &= PFIL_ALL | CONN_ACTIVE | CONN_PASS;
	atomic_store_relaxed(&con->c_flags, flags);
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
	if (conkey == NULL || !npf_connkey_import(npf, conkey, fw)) {
		goto err;
	}
	bk = npf_conn_getbackkey(con, NPF_CONNKEY_ALEN(fw));
	conkey = dnvlist_get_nvlist(cdict, "back-key", NULL);
	if (conkey == NULL || !npf_connkey_import(npf, conkey, bk)) {
		goto err;
	}

	/* Guard against the contradicting address lengths. */
	if (NPF_CONNKEY_ALEN(fw) != alen || NPF_CONNKEY_ALEN(bk) != alen) {
		goto err;
	}

	/* Insert the entries and the connection itself. */
	if (!npf_conndb_insert(cd, fw, con, NPF_FLOW_FORW)) {
		goto err;
	}
	if (!npf_conndb_insert(cd, bk, con, NPF_FLOW_BACK)) {
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

/*
 * npf_conn_find: lookup a connection in the list of connections
 */
int
npf_conn_find(npf_t *npf, const nvlist_t *req, nvlist_t *resp)
{
	const nvlist_t *key_nv;
	npf_conn_t *con;
	npf_connkey_t key;
	npf_flow_t flow;
	int error;

	key_nv = dnvlist_get_nvlist(req, "key", NULL);
	if (!key_nv || !npf_connkey_import(npf, key_nv, &key)) {
		return EINVAL;
	}
	con = npf_conndb_lookup(npf, &key, &flow);
	if (con == NULL) {
		return ESRCH;
	}
	if (!npf_conn_check(con, NULL, 0, NPF_FLOW_FORW)) {
		atomic_dec_uint(&con->c_refcnt);
		return ESRCH;
	}
	error = npf_conn_export(npf, con, resp);
	nvlist_add_number(resp, "flow", flow);
	atomic_dec_uint(&con->c_refcnt);
	return error;
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_conn_print(npf_conn_t *con)
{
	const npf_connkey_t *fw = npf_conn_getforwkey(con);
	const npf_connkey_t *bk = npf_conn_getbackkey(con, NPF_CONNKEY_ALEN(fw));
	const unsigned flags = atomic_load_relaxed(&con->c_flags);
	const unsigned proto = con->c_proto;
	struct timespec tspnow;

	getnanouptime(&tspnow);
	printf("%p:\n\tproto %d flags 0x%x tsdiff %ld etime %d\n", con,
	    proto, flags, (long)(tspnow.tv_sec - con->c_atime),
	    npf_state_etime(npf_getkernctx(), &con->c_state, proto));
	npf_connkey_print(fw);
	npf_connkey_print(bk);
	npf_state_dump(&con->c_state);
	if (con->c_nat) {
		npf_nat_dump(con->c_nat);
	}
}

#endif
