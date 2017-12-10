/*	$NetBSD: npf_conn.c,v 1.24 2017/12/10 00:07:36 rmind Exp $	*/

/*-
 * Copyright (c) 2014-2015 Mindaugas Rasiukevicius <rmind at netbsd org>
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
 *	Connection direction is identified by the direction of its first
 *	packet.  Packets can be incoming or outgoing with respect to an
 *	interface.  To describe the packet in the context of connection
 *	direction we will use the terms "forwards stream" and "backwards
 *	stream".  All connections have two keys and thus two entries:
 *
 *		npf_conn_t::c_forw_entry for the forwards stream and
 *		npf_conn_t::c_back_entry for the backwards stream.
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
__KERNEL_RCSID(0, "$NetBSD: npf_conn.c,v 1.24 2017/12/10 00:07:36 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/atomic.h>
#include <sys/condvar.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <net/pfil.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/systm.h>
#endif

#define __NPF_CONN_PRIVATE
#include "npf_conn.h"
#include "npf_impl.h"

/*
 * Connection flags: PFIL_IN and PFIL_OUT values are reserved for direction.
 */
CTASSERT(PFIL_ALL == (0x001 | 0x002));
#define	CONN_ACTIVE	0x004	/* visible on inspection */
#define	CONN_PASS	0x008	/* perform implicit passing */
#define	CONN_EXPIRE	0x010	/* explicitly expire */
#define	CONN_REMOVED	0x020	/* "forw/back" entries removed */

enum { CONN_TRACKING_OFF, CONN_TRACKING_ON };

static void	npf_conn_destroy(npf_t *, npf_conn_t *);

/*
 * npf_conn_sys{init,fini}: initialise/destroy connection tracking.
 */

void
npf_conn_init(npf_t *npf, int flags)
{
	npf->conn_cache = pool_cache_init(sizeof(npf_conn_t), coherency_unit,
	    0, 0, "npfconpl", NULL, IPL_NET, NULL, NULL, NULL);
	mutex_init(&npf->conn_lock, MUTEX_DEFAULT, IPL_NONE);
	npf->conn_tracking = CONN_TRACKING_OFF;
	npf->conn_db = npf_conndb_create();

	if ((flags & NPF_NO_GC) == 0) {
		npf_worker_register(npf, npf_conn_worker);
	}
}

void
npf_conn_fini(npf_t *npf)
{
	/* Note: the caller should have flushed the connections. */
	KASSERT(npf->conn_tracking == CONN_TRACKING_OFF);
	npf_worker_unregister(npf, npf_conn_worker);

	npf_conndb_destroy(npf->conn_db);
	pool_cache_destroy(npf->conn_cache);
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
		npf_conn_gc(npf, odb, true, false);
		npf_conndb_destroy(odb);
		pool_cache_invalidate(npf->conn_cache);
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

static uint32_t
connkey_setkey(npf_connkey_t *key, uint16_t proto, const void *ipv,
    const uint16_t *id, unsigned alen, bool forw)
{
	uint32_t isrc, idst, *k = key->ck_key;
	const npf_addr_t * const *ips = ipv;

	if (__predict_true(forw)) {
		isrc = NPF_SRC, idst = NPF_DST;
	} else {
		isrc = NPF_DST, idst = NPF_SRC;
	}

	/*
	 * Construct a key formed out of 32-bit integers.  The key layout:
	 *
	 * Field: | proto  |  alen  | src-id | dst-id | src-addr | dst-addr |
	 *        +--------+--------+--------+--------+----------+----------+
	 * Bits:  |   16   |   16   |   16   |   16   |  32-128  |  32-128  |
	 *
	 * The source and destination are inverted if they key is for the
	 * backwards stream (forw == false).  The address length depends
	 * on the 'alen' field; it is a length in bytes, either 4 or 16.
	 */

	k[0] = ((uint32_t)proto << 16) | (alen & 0xffff);
	k[1] = ((uint32_t)id[isrc] << 16) | id[idst];

	if (__predict_true(alen == sizeof(in_addr_t))) {
		k[2] = ips[isrc]->word32[0];
		k[3] = ips[idst]->word32[0];
		return 4 * sizeof(uint32_t);
	} else {
		const u_int nwords = alen >> 2;
		memcpy(&k[2], ips[isrc], alen);
		memcpy(&k[2 + nwords], ips[idst], alen);
		return (2 + (nwords * 2)) * sizeof(uint32_t);
	}
}

static void
connkey_getkey(const npf_connkey_t *key, uint16_t *proto, npf_addr_t *ips,
    uint16_t *id, uint16_t *alen)
{
	const uint32_t *k = key->ck_key;

	*proto = k[0] >> 16;
	*alen = k[0] & 0xffff;
	id[NPF_SRC] = k[1] >> 16;
	id[NPF_DST] = k[1] & 0xffff;

	switch (*alen) {
	case sizeof(struct in6_addr):
	case sizeof(struct in_addr):
		memcpy(&ips[NPF_SRC], &k[2], *alen);
		memcpy(&ips[NPF_DST], &k[2 + ((unsigned)*alen >> 2)], *alen);
		return;
	default:
		KASSERT(0);
	}
}

/*
 * npf_conn_conkey: construct a key for the connection lookup.
 *
 * => Returns the key length in bytes or zero on failure.
 */
unsigned
npf_conn_conkey(const npf_cache_t *npc, npf_connkey_t *key, const bool forw)
{
	const u_int proto = npc->npc_proto;
	const u_int alen = npc->npc_alen;
	const struct tcphdr *th;
	const struct udphdr *uh;
	uint16_t id[2];

	switch (proto) {
	case IPPROTO_TCP:
		KASSERT(npf_iscached(npc, NPC_TCP));
		th = npc->npc_l4.tcp;
		id[NPF_SRC] = th->th_sport;
		id[NPF_DST] = th->th_dport;
		break;
	case IPPROTO_UDP:
		KASSERT(npf_iscached(npc, NPC_UDP));
		uh = npc->npc_l4.udp;
		id[NPF_SRC] = uh->uh_sport;
		id[NPF_DST] = uh->uh_dport;
		break;
	case IPPROTO_ICMP:
		if (npf_iscached(npc, NPC_ICMP_ID)) {
			const struct icmp *ic = npc->npc_l4.icmp;
			id[NPF_SRC] = ic->icmp_id;
			id[NPF_DST] = ic->icmp_id;
			break;
		}
		return 0;
	case IPPROTO_ICMPV6:
		if (npf_iscached(npc, NPC_ICMP_ID)) {
			const struct icmp6_hdr *ic6 = npc->npc_l4.icmp6;
			id[NPF_SRC] = ic6->icmp6_id;
			id[NPF_DST] = ic6->icmp6_id;
			break;
		}
		return 0;
	default:
		/* Unsupported protocol. */
		return 0;
	}
	return connkey_setkey(key, proto, npc->npc_ips, id, alen, forw);
}

static __inline void
connkey_set_addr(npf_connkey_t *key, const npf_addr_t *naddr, const int di)
{
	const u_int alen = key->ck_key[0] & 0xffff;
	uint32_t *addr = &key->ck_key[2 + ((alen >> 2) * di)];

	KASSERT(alen > 0);
	memcpy(addr, naddr, alen);
}

static __inline void
connkey_set_id(npf_connkey_t *key, const uint16_t id, const int di)
{
	const uint32_t oid = key->ck_key[1];
	const u_int shift = 16 * !di;
	const uint32_t mask = 0xffff0000 >> shift;

	key->ck_key[1] = ((uint32_t)id << shift) | (oid & mask);
}

static inline void
conn_update_atime(npf_conn_t *con)
{
	struct timespec tsnow;

	getnanouptime(&tsnow);
	con->c_atime = tsnow.tv_sec;
}

/*
 * npf_conn_ok: check if the connection is active, and has the right direction.
 */
static bool
npf_conn_ok(const npf_conn_t *con, const int di, bool forw)
{
	const uint32_t flags = con->c_flags;

	/* Check if connection is active and not expired. */
	bool ok = (flags & (CONN_ACTIVE | CONN_EXPIRE)) == CONN_ACTIVE;
	if (__predict_false(!ok)) {
		return false;
	}

	/* Check if the direction is consistent */
	bool pforw = (flags & PFIL_ALL) == (unsigned)di;
	if (__predict_false(forw != pforw)) {
		return false;
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
	u_int cifid;

	/* Construct a key and lookup for a connection in the store. */
	if (!npf_conn_conkey(npc, &key, true)) {
		return NULL;
	}
	con = npf_conndb_lookup(npf->conn_db, &key, forw);
	if (con == NULL) {
		return NULL;
	}
	KASSERT(npc->npc_proto == con->c_proto);

	/* Check if connection is active and not expired. */
	if (!npf_conn_ok(con, di, *forw)) {
		atomic_dec_uint(&con->c_refcnt);
		return NULL;
	}

	/*
	 * Match the interface and the direction of the connection entry
	 * and the packet.
	 */
	cifid = con->c_ifid;
	if (__predict_false(cifid && cifid != nbuf->nb_ifid)) {
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
npf_conn_establish(npf_cache_t *npc, int di, bool per_if)
{
	npf_t *npf = npc->npc_ctx;
	const nbuf_t *nbuf = npc->npc_nbuf;
	npf_conn_t *con;
	int error = 0;

	KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));

	if (!npf_conn_trackable_p(npc)) {
		return NULL;
	}

	/* Allocate and initialise the new connection. */
	con = pool_cache_get(npf->conn_cache, PR_NOWAIT);
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

	/* Initialize the protocol state. */
	if (!npf_state_init(npc, &con->c_state)) {
		npf_conn_destroy(npf, con);
		return NULL;
	}

	KASSERT(npf_iscached(npc, NPC_IP46));
	npf_connkey_t *fw = &con->c_forw_entry;
	npf_connkey_t *bk = &con->c_back_entry;

	/*
	 * Construct "forwards" and "backwards" keys.  Also, set the
	 * interface ID for this connection (unless it is global).
	 */
	if (!npf_conn_conkey(npc, fw, true) ||
	    !npf_conn_conkey(npc, bk, false)) {
		npf_conn_destroy(npf, con);
		return NULL;
	}
	fw->ck_backptr = bk->ck_backptr = con;
	con->c_ifid = per_if ? nbuf->nb_ifid : 0;
	con->c_proto = npc->npc_proto;

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
	if (!npf_conndb_insert(npf->conn_db, fw, con)) {
		error = EISCONN;
		goto err;
	}
	if (!npf_conndb_insert(npf->conn_db, bk, con)) {
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

static void
npf_conn_destroy(npf_t *npf, npf_conn_t *con)
{
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
	pool_cache_put(npf->conn_cache, con);
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
    npf_nat_t *nt, u_int ntype)
{
	static const u_int nat_type_dimap[] = {
		[NPF_NATOUT] = NPF_DST,
		[NPF_NATIN] = NPF_SRC,
	};
	npf_t *npf = npc->npc_ctx;
	npf_connkey_t key, *bk;
	npf_conn_t *ret __diagused;
	npf_addr_t *taddr;
	in_port_t tport;
	u_int tidx;

	KASSERT(con->c_refcnt > 0);

	npf_nat_gettrans(nt, &taddr, &tport);
	KASSERT(ntype == NPF_NATOUT || ntype == NPF_NATIN);
	tidx = nat_type_dimap[ntype];

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

	/* Remove the "backwards" entry. */
	ret = npf_conndb_remove(npf->conn_db, &con->c_back_entry);
	KASSERT(ret == con);

	/* Set the source/destination IDs to the translation values. */
	bk = &con->c_back_entry;
	connkey_set_addr(bk, taddr, tidx);
	if (tport) {
		connkey_set_id(bk, tport, tidx);
	}

	/* Finally, re-insert the "backwards" entry. */
	if (!npf_conndb_insert(npf->conn_db, bk, con)) {
		/*
		 * Race: we have hit the duplicate, remove the "forwards"
		 * entry and expire our connection; it is no longer valid.
		 */
		ret = npf_conndb_remove(npf->conn_db, &con->c_forw_entry);
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
static inline bool
npf_conn_expired(const npf_conn_t *con, uint64_t tsnow)
{
	const int etime = npf_state_etime(&con->c_state, con->c_proto);
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
 * npf_conn_gc: garbage collect the expired connections.
 *
 * => Must run in a single-threaded manner.
 * => If it is a flush request, then destroy all connections.
 * => If 'sync' is true, then perform passive serialisation.
 */
void
npf_conn_gc(npf_t *npf, npf_conndb_t *cd, bool flush, bool sync)
{
	npf_conn_t *con, *prev, *gclist = NULL;
	struct timespec tsnow;

	getnanouptime(&tsnow);

	/*
	 * Scan all connections and check them for expiration.
	 */
	prev = NULL;
	con = npf_conndb_getlist(cd);
	while (con) {
		npf_conn_t *next = con->c_next;

		/* Expired?  Flushing all? */
		if (!npf_conn_expired(con, tsnow.tv_sec) && !flush) {
			prev = con;
			con = next;
			continue;
		}

		/* Remove both entries of the connection. */
		mutex_enter(&con->c_lock);
		if ((con->c_flags & CONN_REMOVED) == 0) {
			npf_conn_t *ret __diagused;

			ret = npf_conndb_remove(cd, &con->c_forw_entry);
			KASSERT(ret == con);
			ret = npf_conndb_remove(cd, &con->c_back_entry);
			KASSERT(ret == con);
		}

		/* Flag the removal and expiration. */
		atomic_or_uint(&con->c_flags, CONN_REMOVED | CONN_EXPIRE);
		mutex_exit(&con->c_lock);

		/* Move to the G/C list. */
		npf_conndb_dequeue(cd, con, prev);
		con->c_next = gclist;
		gclist = con;

		/* Next.. */
		con = next;
	}
	npf_conndb_settail(cd, prev);

	/*
	 * Ensure it is safe to destroy the connections.
	 * Note: drop the conn_lock (see the lock order).
	 */
	if (sync) {
		mutex_exit(&npf->conn_lock);
		if (gclist) {
			npf_config_enter(npf);
			npf_config_sync(npf);
			npf_config_exit(npf);
		}
	}

	/*
	 * Garbage collect all expired connections.
	 * May need to wait for the references to drain.
	 */
	con = gclist;
	while (con) {
		npf_conn_t *next = con->c_next;

		/*
		 * Destroy only if removed and no references.
		 * Otherwise, wait for a tiny moment.
		 */
		if (__predict_false(con->c_refcnt)) {
			kpause("npfcongc", false, 1, NULL);
			continue;
		}
		npf_conn_destroy(npf, con);
		con = next;
	}
}

/*
 * npf_conn_worker: G/C to run from a worker thread.
 */
void
npf_conn_worker(npf_t *npf)
{
	mutex_enter(&npf->conn_lock);
	/* Note: the conn_lock will be released (sync == true). */
	npf_conn_gc(npf, npf->conn_db, false, true);
}

/*
 * npf_conndb_export: construct a list of connections prepared for saving.
 * Note: this is expected to be an expensive operation.
 */
int
npf_conndb_export(npf_t *npf, prop_array_t conlist)
{
	npf_conn_t *con, *prev;

	/*
	 * Note: acquire conn_lock to prevent from the database
	 * destruction and G/C thread.
	 */
	mutex_enter(&npf->conn_lock);
	if (npf->conn_tracking != CONN_TRACKING_ON) {
		mutex_exit(&npf->conn_lock);
		return 0;
	}
	prev = NULL;
	con = npf_conndb_getlist(npf->conn_db);
	while (con) {
		npf_conn_t *next = con->c_next;
		prop_dictionary_t cdict;

		if ((cdict = npf_conn_export(npf, con)) != NULL) {
			prop_array_add(conlist, cdict);
			prop_object_release(cdict);
		}
		prev = con;
		con = next;
	}
	npf_conndb_settail(npf->conn_db, prev);
	mutex_exit(&npf->conn_lock);
	return 0;
}

static prop_dictionary_t
npf_connkey_export(const npf_connkey_t *key)
{
	uint16_t id[2], alen, proto;
	prop_dictionary_t kdict;
	npf_addr_t ips[2];
	prop_data_t d;

	kdict = prop_dictionary_create();
	connkey_getkey(key, &proto, ips, id, &alen);

	prop_dictionary_set_uint16(kdict, "proto", proto);

	prop_dictionary_set_uint16(kdict, "sport", id[NPF_SRC]);
	prop_dictionary_set_uint16(kdict, "dport", id[NPF_DST]);

	d = prop_data_create_data(&ips[NPF_SRC], alen);
	prop_dictionary_set_and_rel(kdict, "saddr", d);

	d = prop_data_create_data(&ips[NPF_DST], alen);
	prop_dictionary_set_and_rel(kdict, "daddr", d);

	return kdict;
}

/*
 * npf_conn_export: serialise a single connection.
 */
prop_dictionary_t
npf_conn_export(npf_t *npf, const npf_conn_t *con)
{
	prop_dictionary_t cdict, kdict;
	prop_data_t d;

	if ((con->c_flags & (CONN_ACTIVE|CONN_EXPIRE)) != CONN_ACTIVE) {
		return NULL;
	}
	cdict = prop_dictionary_create();
	prop_dictionary_set_uint32(cdict, "flags", con->c_flags);
	prop_dictionary_set_uint32(cdict, "proto", con->c_proto);
	if (con->c_ifid) {
		const char *ifname = npf_ifmap_getname(npf, con->c_ifid);
		prop_dictionary_set_cstring(cdict, "ifname", ifname);
	}

	d = prop_data_create_data(&con->c_state, sizeof(npf_state_t));
	prop_dictionary_set_and_rel(cdict, "state", d);

	kdict = npf_connkey_export(&con->c_forw_entry);
	prop_dictionary_set_and_rel(cdict, "forw-key", kdict);

	kdict = npf_connkey_export(&con->c_back_entry);
	prop_dictionary_set_and_rel(cdict, "back-key", kdict);

	if (con->c_nat) {
		npf_nat_export(cdict, con->c_nat);
	}
	return cdict;
}

static uint32_t
npf_connkey_import(prop_dictionary_t kdict, npf_connkey_t *key)
{
	prop_object_t sobj, dobj;
	npf_addr_t const * ips[2];
	uint16_t alen, proto, id[2];

	if (!prop_dictionary_get_uint16(kdict, "proto", &proto))
		return 0;

	if (!prop_dictionary_get_uint16(kdict, "sport", &id[NPF_SRC]))
		return 0;

	if (!prop_dictionary_get_uint16(kdict, "dport", &id[NPF_DST]))
		return 0;

	sobj = prop_dictionary_get(kdict, "saddr");
	if ((ips[NPF_SRC] = prop_data_data_nocopy(sobj)) == NULL)
		return 0;

	dobj = prop_dictionary_get(kdict, "daddr");
	if ((ips[NPF_DST] = prop_data_data_nocopy(dobj)) == NULL)
		return 0;

	alen = prop_data_size(sobj);
	if (alen != prop_data_size(dobj))
		return 0;

	return connkey_setkey(key, proto, ips, id, alen, true);
}

/*
 * npf_conn_import: fully reconstruct a single connection from a
 * directory and insert into the given database.
 */
int
npf_conn_import(npf_t *npf, npf_conndb_t *cd, prop_dictionary_t cdict,
    npf_ruleset_t *natlist)
{
	npf_conn_t *con;
	npf_connkey_t *fw, *bk;
	prop_object_t obj;
	const char *ifname;
	const void *d;

	/* Allocate a connection and initialise it (clear first). */
	con = pool_cache_get(npf->conn_cache, PR_WAITOK);
	memset(con, 0, sizeof(npf_conn_t));
	mutex_init(&con->c_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	npf_stats_inc(npf, NPF_STAT_CONN_CREATE);

	prop_dictionary_get_uint32(cdict, "proto", &con->c_proto);
	prop_dictionary_get_uint32(cdict, "flags", &con->c_flags);
	con->c_flags &= PFIL_ALL | CONN_ACTIVE | CONN_PASS;
	conn_update_atime(con);

	if (prop_dictionary_get_cstring_nocopy(cdict, "ifname", &ifname) &&
	    (con->c_ifid = npf_ifmap_register(npf, ifname)) == 0) {
		goto err;
	}

	obj = prop_dictionary_get(cdict, "state");
	if ((d = prop_data_data_nocopy(obj)) == NULL ||
	    prop_data_size(obj) != sizeof(npf_state_t)) {
		goto err;
	}
	memcpy(&con->c_state, d, sizeof(npf_state_t));

	/* Reconstruct NAT association, if any. */
	if ((obj = prop_dictionary_get(cdict, "nat")) != NULL &&
	    (con->c_nat = npf_nat_import(npf, obj, natlist, con)) == NULL) {
		goto err;
	}

	/*
	 * Fetch and copy the keys for each direction.
	 */
	obj = prop_dictionary_get(cdict, "forw-key");
	fw = &con->c_forw_entry;
	if (obj == NULL || !npf_connkey_import(obj, fw)) {
		goto err;
	}

	obj = prop_dictionary_get(cdict, "back-key");
	bk = &con->c_back_entry;
	if (obj == NULL || !npf_connkey_import(obj, bk)) {
		goto err;
	}

	fw->ck_backptr = bk->ck_backptr = con;

	/* Insert the entries and the connection itself. */
	if (!npf_conndb_insert(cd, fw, con)) {
		goto err;
	}
	if (!npf_conndb_insert(cd, bk, con)) {
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
npf_conn_find(npf_t *npf, prop_dictionary_t idict, prop_dictionary_t *odict)
{
	prop_dictionary_t kdict;
	npf_connkey_t key;
	npf_conn_t *con;
	uint16_t dir;
	bool forw;

	if ((kdict = prop_dictionary_get(idict, "key")) == NULL)
		return EINVAL;

	if (!npf_connkey_import(kdict, &key))
		return EINVAL;

	if (!prop_dictionary_get_uint16(idict, "direction", &dir))
		return EINVAL;

	con = npf_conndb_lookup(npf->conn_db, &key, &forw);
	if (con == NULL) {
		return ESRCH;
	}

	if (!npf_conn_ok(con, dir, true)) {
		atomic_dec_uint(&con->c_refcnt);
		return ESRCH;
	}

	*odict = npf_conn_export(npf, con);
	if (*odict == NULL) {
		atomic_dec_uint(&con->c_refcnt);
		return ENOSPC;
	}
	atomic_dec_uint(&con->c_refcnt);

	return 0;
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_conn_print(const npf_conn_t *con)
{
	const u_int alen = NPF_CONN_GETALEN(&con->c_forw_entry);
	const uint32_t *fkey = con->c_forw_entry.ck_key;
	const uint32_t *bkey = con->c_back_entry.ck_key;
	const u_int proto = con->c_proto;
	struct timespec tspnow;
	const void *src, *dst;
	int etime;

	getnanouptime(&tspnow);
	etime = npf_state_etime(&con->c_state, proto);

	printf("%p:\n\tproto %d flags 0x%x tsdiff %ld etime %d\n", con,
	    proto, con->c_flags, (long)(tspnow.tv_sec - con->c_atime), etime);

	src = &fkey[2], dst = &fkey[2 + (alen >> 2)];
	printf("\tforw %s:%d", npf_addr_dump(src, alen), ntohs(fkey[1] >> 16));
	printf("-> %s:%d\n", npf_addr_dump(dst, alen), ntohs(fkey[1] & 0xffff));

	src = &bkey[2], dst = &bkey[2 + (alen >> 2)];
	printf("\tback %s:%d", npf_addr_dump(src, alen), ntohs(bkey[1] >> 16));
	printf("-> %s:%d\n", npf_addr_dump(dst, alen), ntohs(bkey[1] & 0xffff));

	npf_state_dump(&con->c_state);
	if (con->c_nat) {
		npf_nat_dump(con->c_nat);
	}
}

#endif
