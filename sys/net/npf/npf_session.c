/*	$NetBSD: npf_session.c,v 1.33 2014/07/19 18:24:16 rmind Exp $	*/

/*-
 * Copyright (c) 2010-2013 The NetBSD Foundation, Inc.
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
 * NPF session tracking for stateful filtering and translation.
 *
 * Overview
 *
 *	Session direction is identified by the direction of its first packet.
 *	Packets can be incoming or outgoing with respect to an interface.
 *	To describe the packet in the context of session direction, we will
 *	use the terms "forwards stream" and "backwards stream".  All sessions
 *	have two embedded entries - npf_session_t::s_forw_entry for forwards
 *	stream and npf_session_t::s_back_entry for backwards stream.  These
 *	entries (npf_sentry_t) contain source and destination identifiers.
 *	Note that entry may contain translated values in a case of NAT.
 *
 *	Sessions can serve two purposes: "pass" or "NAT".  Sessions for the
 *	former purpose are created according to the rules with "stateful"
 *	attribute and are used for stateful filtering.  Such sessions
 *	indicate that the packet of the backwards stream should be passed
 *	without inspection of the ruleset.  Another purpose is to associate
 *	NAT with a connection (which implies connection tracking).  Such
 *	sessions are created according to the NAT policies and they have a
 *	relationship with NAT translation structure via npf_session_t::s_nat.
 *	A single session can serve both purposes, which is a common case.
 *
 * Session life-cycle
 *
 *	Sessions are established when a packet matches said rule or NAT policy.
 *	Both entries of established session are inserted into the hashed tree.
 *	A garbage collection thread periodically scans all session entries and
 *	depending on session properties (e.g. last activity time, protocol)
 *	removes session entries and expires the actual sessions.
 *
 *	Each session has a reference count.  Reference is acquired on lookup
 *	and should be released by the caller.  Reference guarantees that the
 *	session will not be destroyed, although it may be expired.
 *
 * Synchronisation
 *
 *	Session hash table is accessed in a lock-less manner by the main
 *	operations: npf_session_inspect() and npf_session_establish().
 *	Since they are always called from a software interrupt, the hash
 *	table is protected using passive serialisation.  The main place
 *	which can destroy the hash table is npf_session_reload().  It has
 *	to synchronise with other readers and writers using sess_lock,
 *	primarily the G/C thread.
 *
 * ALG support
 *
 *	Application-level gateways (ALGs) can override generic session
 *	inspection (npf_alg_session() in npf_session_inspect() function)
 *	by performing their own lookup using different identifiers.
 *	Recursive call to npf_session_inspect() is not allowed, they
 *	ought to use npf_session_lookup() for this purpose.
 *
 * Lock order
 *
 *	sess_lock ->
 *		[ npf_config_lock -> ]
 *			npf_sehash_t::sh_lock ->
 *				npf_state_t::nst_lock
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_session.c,v 1.33 2014/07/19 18:24:16 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/atomic.h>
#include <sys/condvar.h>
#include <sys/cprng.h>
#include <sys/hash.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <net/pfil.h>
#include <sys/pool.h>
#include <sys/rwlock.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include "npf_impl.h"
#include "npf_conn.h"

#define	npf_session_t		npf_conn_t
#define	npf_session		npf_conn
#define	npf_sehash_t		npf_conndb_t

#define	npf_session_sysinit	npf_conn_sysinit
#define	npf_session_sysfini	npf_conn_sysfini
#define	npf_session_tracking	npf_conn_tracking
#define	npf_session_lookup	npf_conn_lookup
#define	npf_session_inspect	npf_conn_inspect
#define	npf_session_release	npf_conn_release
#define	npf_session_establish	npf_conn_establish
#define	npf_session_setnat	npf_conn_setnat
#define	npf_session_expire	npf_conn_expire
#define	npf_session_pass	npf_conn_pass
#define	npf_session_setpass	npf_conn_setpass
#define	npf_session_release	npf_conn_release
#define	npf_session_retnat	npf_conn_retnat
#define	npf_session_load	npf_conn_load
#define	npf_session_save	npf_conn_save
#define	npf_session_restore	npf_conn_restore
#define	sess_htable_create	npf_conndb_create
#define	sess_htable_destroy	npf_conndb_destroy
#define	npf_alg_session		npf_alg_conn

/*
 * Session structures: entry for embedding and the main structure.
 * WARNING: update npf_session_restore() when adding fields.
 */

struct npf_secomid;
typedef struct npf_secomid npf_secomid_t;

typedef struct {
	/* Session entry node and back-pointer to the actual session. */
	rb_node_t		se_rbnode;
	union {
		npf_session_t *	se_backptr;
		void *		se_common_id;
	};
	/* Size of the addresses. */
	int			se_alen;
	/* Source and destination addresses. */
	npf_addr_t		se_src_addr;
	npf_addr_t		se_dst_addr;
	/* Source and destination ports (TCP / UDP) or generic IDs. */
	uint16_t		se_src_id;
	uint16_t		se_dst_id;
} npf_sentry_t;

struct npf_conn {
	/* Session "forwards" and "backwards" entries. */
	npf_sentry_t		s_forw_entry;
	npf_sentry_t		s_back_entry;
	/* Entry in the session hash or G/C list. */
	LIST_ENTRY(npf_session)	s_list;
	u_int			s_refcnt;
	/* Protocol and interface (common IDs). */
	struct npf_secomid {
		uint16_t	proto;
		uint16_t	ifid;
	} s_common_id;
	/* Flags and the protocol state. */
	u_int			s_flags;
	kmutex_t		s_lock;
	npf_state_t		s_state;
	/* Association of rule procedure data. */
	npf_rproc_t *		s_rproc;
	/* NAT associated with this session (if any). */
	npf_nat_t *		s_nat;
	/* Last activity time (used to calculate expiration time). */
	struct timespec 	s_atime;
};

#define	SESS_HASH_BUCKETS	1024	/* XXX tune + make tunable */
#define	SESS_HASH_MASK		(SESS_HASH_BUCKETS - 1)

LIST_HEAD(npf_sesslist, npf_session);

struct npf_conndb {
	rb_tree_t		sh_tree;
	struct npf_sesslist	sh_list;
	krwlock_t		sh_lock;
	u_int			sh_count;
};

/*
 * Session flags: PFIL_IN and PFIL_OUT values are reserved for direction.
 */
CTASSERT(PFIL_ALL == (0x001 | 0x002));
#define	SE_ACTIVE		0x004	/* visible on inspection */
#define	SE_PASS			0x008	/* perform implicit passing */
#define	SE_EXPIRE		0x010	/* explicitly expire */

/*
 * Flags to indicate removal of forwards/backwards session entries or
 * completion of session removal itself (i.e. both entries).
 */
#define	SE_REMFORW		0x020
#define	SE_REMBACK		0x040
#define	SE_REMOVED		(SE_REMFORW | SE_REMBACK)

/*
 * Session tracking state: disabled (off), enabled (on) or flush request.
 */
enum { SESS_TRACKING_OFF, SESS_TRACKING_ON, SESS_TRACKING_FLUSH };
static volatile int		sess_tracking	__cacheline_aligned;

/* Session hash table, session cache and the lock. */
static npf_sehash_t *		sess_hashtbl	__read_mostly;
static pool_cache_t		sess_cache	__read_mostly;
static kmutex_t			sess_lock	__cacheline_aligned;
static kcondvar_t		sess_cv		__cacheline_aligned;
static struct npf_sesslist	sess_gc_list	__cacheline_aligned;
static uint32_t			sess_hash_seed	__read_mostly;

static void	npf_session_worker(void);
static void	npf_session_destroy(npf_session_t *);

/*
 * npf_session_sys{init,fini}: initialise/destroy session handling structures.
 *
 * Session table is initialised when session tracking gets enabled via
 * npf_session_tracking() interface.
 */

void
npf_session_sysinit(void)
{
	sess_cache = pool_cache_init(sizeof(npf_session_t), coherency_unit,
	    0, 0, "npfsespl", NULL, IPL_NET, NULL, NULL, NULL);
	mutex_init(&sess_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sess_cv, "npfsecv");
	sess_tracking = SESS_TRACKING_OFF;
	LIST_INIT(&sess_gc_list);
	sess_hashtbl = NULL;

	sess_hash_seed = cprng_fast32();
	npf_worker_register(npf_session_worker);
}

void
npf_session_sysfini(void)
{
	/* Disable tracking, flush all sessions. */
	npf_session_tracking(false);
	npf_worker_unregister(npf_session_worker);

	KASSERT(sess_tracking == SESS_TRACKING_OFF);
	KASSERT(LIST_EMPTY(&sess_gc_list));
	KASSERT(sess_hashtbl == NULL);

	pool_cache_destroy(sess_cache);
	mutex_destroy(&sess_lock);
	cv_destroy(&sess_cv);
}

/*
 * Session hash table and RB-tree helper routines.
 * The order is (src.id, dst.id, src.addr, dst.addr, common_id),
 * where (node1 < node2) shall be negative.
 */

static signed int
sess_rbtree_cmp_nodes(void *ctx, const void *n1, const void *n2)
{
	const npf_sentry_t * const sen1 = n1;
	const npf_sentry_t * const sen2 = n2;
	const int sz = sen1->se_alen;
	int ret;

	if (sen1->se_src_id != sen2->se_src_id) {
		return (sen1->se_src_id < sen2->se_src_id) ? -1 : 1;
	}
	if (sen1->se_dst_id != sen2->se_dst_id) {
		return (sen1->se_dst_id < sen2->se_dst_id) ? -1 : 1;
	}
	if (sen1->se_alen != sen2->se_alen) {
		return (sen1->se_alen < sen2->se_alen) ? -1 : 1;
	}
	if ((ret = memcmp(&sen1->se_src_addr, &sen2->se_src_addr, sz)) != 0) {
		return ret;
	}
	if ((ret = memcmp(&sen1->se_dst_addr, &sen2->se_dst_addr, sz)) != 0) {
		return ret;
	}

	const npf_secomid_t *id1 = &sen1->se_backptr->s_common_id;
	const npf_secomid_t *id2 = ctx ? ctx : &sen2->se_backptr->s_common_id;

	if (id1->proto != id2->proto) {
		return (id1->proto < id2->proto) ? -1 : 1;
	}

	/*
	 * Zero interface ID is a special case indicating a global state,
	 * in which case we match straight away.
	 */
	if (id1->ifid && id1->ifid != id2->ifid) {
		return (id1->ifid < id2->ifid) ? -1 : 1;
	}
	return 0;
}

static signed int
sess_rbtree_cmp_key(void *ctx, const void *n1, const void *key)
{
	const npf_sentry_t * const sen1 = n1;
	const npf_sentry_t * const sen2 = key;

	KASSERT(sen1->se_alen != 0 && sen2->se_alen != 0);
	return sess_rbtree_cmp_nodes(sen2->se_common_id, sen1, sen2);
}

static const rb_tree_ops_t sess_rbtree_ops = {
	.rbto_compare_nodes = sess_rbtree_cmp_nodes,
	.rbto_compare_key = sess_rbtree_cmp_key,
	.rbto_node_offset = offsetof(npf_sentry_t, se_rbnode),
	.rbto_context = NULL
};

static inline npf_sehash_t *
sess_hash_bucket(npf_sehash_t *stbl, const npf_secomid_t *scid,
    const npf_sentry_t *sen)
{
	const int sz = sen->se_alen;
	uint32_t hash, mix[2];

	mix[0] = (scid->proto << 16) | (sen->se_src_id ^ sen->se_dst_id);
	mix[1] = npf_addr_mix(sz, &sen->se_src_addr, &sen->se_dst_addr);
	hash = murmurhash2(mix, sizeof(mix), sess_hash_seed);

	return &stbl[hash & SESS_HASH_MASK];
}

npf_sehash_t *
sess_htable_create(void)
{
	npf_sehash_t *tbl;

	tbl = kmem_zalloc(SESS_HASH_BUCKETS * sizeof(npf_sehash_t), KM_SLEEP);
	for (u_int i = 0; i < SESS_HASH_BUCKETS; i++) {
		npf_sehash_t *sh = &tbl[i];

		LIST_INIT(&sh->sh_list);
		rb_tree_init(&sh->sh_tree, &sess_rbtree_ops);
		rw_init(&sh->sh_lock);
		sh->sh_count = 0;
	}
	return tbl;
}

void
sess_htable_destroy(npf_sehash_t *tbl)
{
	for (u_int i = 0; i < SESS_HASH_BUCKETS; i++) {
		npf_sehash_t *sh = &tbl[i];

		KASSERT(sh->sh_count == 0);
		KASSERT(LIST_EMPTY(&sh->sh_list));
		KASSERT(!rb_tree_iterate(&sh->sh_tree, NULL, RB_DIR_LEFT));
		rw_destroy(&sh->sh_lock);
	}
	kmem_free(tbl, SESS_HASH_BUCKETS * sizeof(npf_sehash_t));
}

/*
 * npf_session_reload: perform reload by flushing the current hash table
 * of the sessions and replacing with the new one or just destroying.
 *
 * Key routine synchronising with all other readers and writers.
 */
static void
npf_session_reload(npf_sehash_t *newtbl, int tracking)
{
	npf_sehash_t *oldtbl;

	/* Must synchronise with G/C thread and session saving/restoring. */
	mutex_enter(&sess_lock);
	while (sess_tracking == SESS_TRACKING_FLUSH) {
		cv_wait(&sess_cv, &sess_lock);
	}

	/*
	 * Set the flush status.  It disables session inspection as well as
	 * creation.  There may be some operations in-flight, drain them.
	 */
	npf_config_enter();
	sess_tracking = SESS_TRACKING_FLUSH;
	npf_config_sync();
	npf_config_exit();

	/* Notify the worker to G/C all sessions. */
	npf_worker_signal();
	while (sess_tracking == SESS_TRACKING_FLUSH) {
		cv_wait(&sess_cv, &sess_lock);
	}

	/* Install the new hash table, make it visible. */
	oldtbl = atomic_swap_ptr(&sess_hashtbl, newtbl);
	membar_sync();
	sess_tracking = tracking;

	/* Done.  Destroy the old table, if any. */
	mutex_exit(&sess_lock);
	if (oldtbl) {
		sess_htable_destroy(oldtbl);
	}
}

/*
 * npf_session_tracking: enable/disable session tracking.
 */
void
npf_session_tracking(bool track)
{
	if (sess_tracking == SESS_TRACKING_OFF && track) {
		/* Disabled -> Enable. */
		npf_sehash_t *newtbl = sess_htable_create();
		npf_session_reload(newtbl, SESS_TRACKING_ON);
		return;
	}
	if (sess_tracking == SESS_TRACKING_ON && !track) {
		/* Enabled -> Disable. */
		npf_session_reload(NULL, SESS_TRACKING_OFF);
		pool_cache_invalidate(sess_cache);
		return;
	}
}

static bool
npf_session_trackable_p(const npf_cache_t *npc)
{
	/*
	 * Check if session tracking is on.  Also, if layer 3 and 4 are not
	 * cached - protocol is not supported or packet is invalid.
	 */
	if (sess_tracking != SESS_TRACKING_ON) {
		return false;
	}
	if (!npf_iscached(npc, NPC_IP46) || !npf_iscached(npc, NPC_LAYER4)) {
		return false;
	}
	return true;
}

/*
 * npf_session_fillent: fill a session entry with the protocol data.
 */
static bool
npf_session_fillent(const npf_cache_t *npc, npf_sentry_t *sen)
{
	const struct tcphdr *th;
	const struct udphdr *uh;

	switch (npc->npc_proto) {
	case IPPROTO_TCP:
		KASSERT(npf_iscached(npc, NPC_TCP));
		th = npc->npc_l4.tcp;
		sen->se_src_id = th->th_sport;
		sen->se_dst_id = th->th_dport;
		break;
	case IPPROTO_UDP:
		KASSERT(npf_iscached(npc, NPC_UDP));
		uh = npc->npc_l4.udp;
		sen->se_src_id = uh->uh_sport;
		sen->se_dst_id = uh->uh_dport;
		break;
	case IPPROTO_ICMP:
		if (npf_iscached(npc, NPC_ICMP_ID)) {
			const struct icmp *ic = npc->npc_l4.icmp;
			sen->se_src_id = ic->icmp_id;
			sen->se_dst_id = ic->icmp_id;
			break;
		}
		return false;
	case IPPROTO_ICMPV6:
		if (npf_iscached(npc, NPC_ICMP_ID)) {
			const struct icmp6_hdr *ic6 = npc->npc_l4.icmp6;
			sen->se_src_id = ic6->icmp6_id;
			sen->se_dst_id = ic6->icmp6_id;
			break;
		}
		return false;
	default:
		/* Unsupported protocol. */
		return false;
	}
	return true;
}

/*
 * npf_session_lookup: lookup for an established session (connection).
 *
 * => If found, we will hold a reference for the caller.
 */
npf_session_t *
npf_session_lookup(const npf_cache_t *npc, const nbuf_t *nbuf,
    const int di, bool *forw)
{
	const u_int proto = npc->npc_proto;
	const u_int ifid = nbuf->nb_ifid;
	npf_sentry_t senkey, *sen;
	npf_session_t *se;
	npf_sehash_t *sh;
	u_int flags;

	if (!npf_session_fillent(npc, &senkey)) {
		return NULL;
	}
	KASSERT(npc->npc_ips[NPF_SRC] && npc->npc_ips[NPF_DST]);
	KASSERT(npc->npc_alen > 0);

	memcpy(&senkey.se_src_addr, npc->npc_ips[NPF_SRC], npc->npc_alen);
	memcpy(&senkey.se_dst_addr, npc->npc_ips[NPF_DST], npc->npc_alen);
	senkey.se_alen = npc->npc_alen;

	/*
	 * Note: this is a special case where we use common ID pointer
	 * to pass the structure for the key comparator.
	 */
	npf_secomid_t scid = { .proto = proto, .ifid = ifid };
	senkey.se_common_id = &scid;

	/*
	 * Get a hash bucket from the cached key data.
	 * Pre-check if there are any entries in the hash table.
	 */
	sh = sess_hash_bucket(sess_hashtbl, &scid, &senkey);
	if (sh->sh_count == 0) {
		return NULL;
	}

	/* Lookup the tree for a session entry and get the actual session. */
	rw_enter(&sh->sh_lock, RW_READER);
	sen = rb_tree_find_node(&sh->sh_tree, &senkey);
	if (sen == NULL) {
		rw_exit(&sh->sh_lock);
		return NULL;
	}
	se = sen->se_backptr;
	KASSERT(se->s_common_id.proto == proto);
	KASSERT(se->s_common_id.ifid == 0 || se->s_common_id.ifid == ifid);
	flags = se->s_flags;

	/* Check if session is active and not expired. */
	if (__predict_false((flags & (SE_ACTIVE | SE_EXPIRE)) != SE_ACTIVE)) {
		rw_exit(&sh->sh_lock);
		return NULL;
	}

	/* Match directions of the session entry and the packet. */
	const bool sforw = (sen == &se->s_forw_entry);
	const bool pforw = (flags & PFIL_ALL) == di;
	if (__predict_false(sforw != pforw)) {
		rw_exit(&sh->sh_lock);
		return NULL;
	}
	*forw = sforw;

	/* Update the last activity time, hold a reference and unlock. */
	getnanouptime(&se->s_atime);
	atomic_inc_uint(&se->s_refcnt);
	rw_exit(&sh->sh_lock);
	return se;
}

/*
 * npf_session_inspect: lookup a session inspecting the protocol data.
 *
 * => If found, we will hold a reference for the caller.
 */
npf_session_t *
npf_session_inspect(npf_cache_t *npc, nbuf_t *nbuf, const int di, int *error)
{
	npf_session_t *se;
	bool forw;

	KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));
	if (!npf_session_trackable_p(npc)) {
		return NULL;
	}

	/* Query ALG which may lookup session for us. */
	if ((se = npf_alg_session(npc, nbuf, di)) != NULL) {
		/* Note: reference is held. */
		return se;
	}
	if (nbuf_head_mbuf(nbuf) == NULL) {
		*error = ENOMEM;
		return NULL;
	}
	KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));

	/* Main lookup of the session. */
	if ((se = npf_session_lookup(npc, nbuf, di, &forw)) == NULL) {
		return NULL;
	}

	/* Inspect the protocol data and handle state changes. */
	mutex_enter(&se->s_lock);
	if (!npf_state_inspect(npc, nbuf, &se->s_state, forw)) {
		/* Invalid: let the rules deal with it. */
		npf_session_release(se);
		npf_stats_inc(NPF_STAT_INVALID_STATE);
		se = NULL;
	}
	mutex_exit(&se->s_lock);
	return se;
}

/*
 * npf_session_establish: create a new session, insert into the global list.
 *
 * => Session is created with the reference held for the caller.
 * => Session will be activated on the first reference release.
 */
npf_session_t *
npf_session_establish(npf_cache_t *npc, nbuf_t *nbuf, int di, bool per_if)
{
	npf_sentry_t *fw, *bk;
	npf_sehash_t *sh;
	npf_session_t *se;
	u_int alen;
	bool ok;

	KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));

	if (!npf_session_trackable_p(npc)) {
		return NULL;
	}

	/* Allocate and initialise new state. */
	se = pool_cache_get(sess_cache, PR_NOWAIT);
	if (__predict_false(se == NULL)) {
		return NULL;
	}
	NPF_PRINTF(("NPF: create se %p\n", se));
	npf_stats_inc(NPF_STAT_SESSION_CREATE);

	/* Reference count and flags (indicate direction). */
	mutex_init(&se->s_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	se->s_refcnt = 1;
	se->s_flags = (di & PFIL_ALL);
	se->s_rproc = NULL;
	se->s_nat = NULL;

	/* Initialize protocol state. */
	if (!npf_state_init(npc, nbuf, &se->s_state)) {
		ok = false;
		goto out;
	}

	/* Unique IDs: IP addresses.  Setup "forwards" entry first. */
	KASSERT(npf_iscached(npc, NPC_IP46));
	alen = npc->npc_alen;
	fw = &se->s_forw_entry;
	memcpy(&fw->se_src_addr, npc->npc_ips[NPF_SRC], alen);
	memcpy(&fw->se_dst_addr, npc->npc_ips[NPF_DST], alen);

	/* Protocol and interface. */
	se->s_common_id.proto = npc->npc_proto;
	se->s_common_id.ifid = per_if ? nbuf->nb_ifid : 0;

	/* Setup "forwards" entry. */
	if (!npf_session_fillent(npc, fw)) {
		ok = false;
		goto out;
	}

	/* Setup inverted "backwards". */
	bk = &se->s_back_entry;
	memcpy(&bk->se_src_addr, &fw->se_dst_addr, alen);
	memcpy(&bk->se_dst_addr, &fw->se_src_addr, alen);
	bk->se_src_id = fw->se_dst_id;
	bk->se_dst_id = fw->se_src_id;

	/* Finish the setup of entries. */
	fw->se_backptr = bk->se_backptr = se;
	fw->se_alen = bk->se_alen = alen;

	/* Set last activity time for a new session. */
	getnanouptime(&se->s_atime);

	/*
	 * Insert the session and both entries into the tree.
	 */
	sh = sess_hash_bucket(sess_hashtbl, &se->s_common_id, fw);
	KASSERT(sh == sess_hash_bucket(sess_hashtbl, &se->s_common_id, bk));

	rw_enter(&sh->sh_lock, RW_WRITER);
	ok = (rb_tree_insert_node(&sh->sh_tree, fw) == fw);
	if (__predict_true(ok)) {
		ok = (rb_tree_insert_node(&sh->sh_tree, bk) == bk);
		if (__predict_true(ok)) {
			/* Success: insert session, count both entries. */
			LIST_INSERT_HEAD(&sh->sh_list, se, s_list);
			sh->sh_count += 2;
			NPF_PRINTF(("NPF: establish se %p\n", se));
		} else {
			/* Race with duplicate packet. */
			rb_tree_remove_node(&sh->sh_tree, fw);
			npf_stats_inc(NPF_STAT_RACE_SESSION);
		}
	}
	rw_exit(&sh->sh_lock);
out:
	if (__predict_false(!ok)) {
		npf_session_destroy(se);
		return NULL;
	}
	return se;
}

static void
npf_session_destroy(npf_session_t *se)
{
	if (se->s_nat) {
		/* Release any NAT related structures. */
		npf_nat_destroy(se->s_nat);
	}
	if (se->s_rproc) {
		/* Release rule procedure. */
		npf_rproc_release(se->s_rproc);
	}

	/* Destroy the state. */
	npf_state_destroy(&se->s_state);
	mutex_destroy(&se->s_lock);

	/* Free the structure, increase the counter. */
	pool_cache_put(sess_cache, se);
	npf_stats_inc(NPF_STAT_SESSION_DESTROY);
	NPF_PRINTF(("NPF: se %p destroyed\n", se));
}

/*
 * npf_session_setnat: associate NAT entry with the session, update
 * and re-insert session entry accordingly.
 */
int
npf_session_setnat(const npf_cache_t *npc,
    npf_session_t *se, npf_nat_t *nt, u_int ntype)
{
	npf_sehash_t *sh;
	npf_sentry_t *sen;
	npf_addr_t *taddr;
	in_port_t tport;
	bool ok;

	KASSERT(se->s_refcnt > 0);

	/* First, atomically check and associate NAT entry. */
	if (atomic_cas_ptr(&se->s_nat, NULL, nt) != NULL) {
		/* Race with a duplicate packet. */
		npf_stats_inc(NPF_STAT_RACE_NAT);
		return EISCONN;
	}

	sen = &se->s_back_entry;
	sh = sess_hash_bucket(sess_hashtbl, &se->s_common_id, sen);

	/*
	 * Note: once the lock is release, the session might be a G/C
	 * target, therefore keep SE_REMBACK bit set until re-insert.
	 */
	rw_enter(&sh->sh_lock, RW_WRITER);
	rb_tree_remove_node(&sh->sh_tree, sen);
	sh->sh_count--;
	rw_exit(&sh->sh_lock);

	/*
	 * Update the source/destination IDs and rehash.  Note that we are
	 * handling the "backwards" entry, therefore the opposite mapping.
	 */
	npf_nat_gettrans(nt, &taddr, &tport);
	switch (ntype) {
	case NPF_NATOUT:
		/* Source in "forwards" => destination. */
		memcpy(&sen->se_dst_addr, taddr, sen->se_alen);
		if (tport)
			sen->se_dst_id = tport;
		break;
	case NPF_NATIN:
		/* Destination in "forwards" => source. */
		memcpy(&sen->se_src_addr, taddr, sen->se_alen);
		if (tport)
			sen->se_src_id = tport;
		break;
	}
	sh = sess_hash_bucket(sess_hashtbl, &se->s_common_id, sen);

	/*
	 * Insert the entry back into a potentially new bucket.
	 *
	 * Note: synchronise with the G/C thread here for a case when the
	 * old session is still being expired while a duplicate is being
	 * created here.  This race condition is rare.
	 */
	rw_enter(&sh->sh_lock, RW_WRITER);
	ok = rb_tree_insert_node(&sh->sh_tree, sen) == sen;
	if (__predict_true(ok)) {
		sh->sh_count++;
		NPF_PRINTF(("NPF: se %p assoc with nat %p\n", se, se->s_nat));
	} else {
		/* Race: mark a removed entry and explicitly expire. */
		atomic_or_uint(&se->s_flags, SE_REMBACK | SE_EXPIRE);
		npf_stats_inc(NPF_STAT_RACE_NAT);
		se->s_nat = NULL;
	}
	rw_exit(&sh->sh_lock);
	return ok ? 0 : EISCONN;
}

/*
 * npf_session_expire: explicitly mark session as expired.
 */
void
npf_session_expire(npf_session_t *se)
{
	/* KASSERT(se->s_refcnt > 0); XXX: npf_nat_freepolicy() */
	atomic_or_uint(&se->s_flags, SE_EXPIRE);
}

/*
 * npf_session_pass: return true if session is "pass" one, otherwise false.
 */
bool
npf_session_pass(const npf_session_t *se, npf_rproc_t **rp)
{
	KASSERT(se->s_refcnt > 0);
	if ((se->s_flags & SE_PASS) != 0) {
		*rp = se->s_rproc;
		return true;
	}
	return false;
}

/*
 * npf_session_setpass: mark session as a "pass" one and associate rule
 * procedure with it.
 */
void
npf_session_setpass(npf_session_t *se, npf_rproc_t *rp)
{
	KASSERT((se->s_flags & SE_ACTIVE) == 0);
	KASSERT(se->s_refcnt > 0);
	KASSERT(se->s_rproc == NULL);

	/*
	 * No need for atomic since the session is not yet active.
	 * If rproc is set, the caller transfers its reference to us,
	 * which will be released on npf_session_destroy().
	 */
	se->s_flags |= SE_PASS;
	se->s_rproc = rp;
}

/*
 * npf_session_release: release a reference, which might allow G/C thread
 * to destroy this session.
 */
void
npf_session_release(npf_session_t *se)
{
	KASSERT(se->s_refcnt > 0);
	if ((se->s_flags & (SE_ACTIVE | SE_EXPIRE)) == 0) {
		/* Activate: after this point, session is globally visible. */
		se->s_flags |= SE_ACTIVE;
	}
	atomic_dec_uint(&se->s_refcnt);
}

/*
 * npf_session_retnat: return associated NAT data entry and indicate
 * whether it is a "forwards" or "backwards" stream.
 */
npf_nat_t *
npf_session_retnat(npf_session_t *se, const int di, bool *forw)
{
	KASSERT(se->s_refcnt > 0);
	*forw = (se->s_flags & PFIL_ALL) == di;
	return se->s_nat;
}

/*
 * npf_session_expired: criterion to check if session is expired.
 */
static inline bool
npf_session_expired(const npf_session_t *se, const struct timespec *tsnow)
{
	const u_int proto = se->s_common_id.proto;
	const int etime = npf_state_etime(&se->s_state, proto);
	struct timespec tsdiff;

	if (__predict_false(se->s_flags & SE_EXPIRE)) {
		/* Explicitly marked to be expired. */
		return true;
	}
	timespecsub(tsnow, &se->s_atime, &tsdiff);
	return __predict_false(tsdiff.tv_sec > etime);
}

/*
 * npf_session_gc: scan all sessions, insert into G/C list all expired ones.
 */
static void
npf_session_gc(struct npf_sesslist *gc_list, bool flushall)
{
	struct timespec tsnow;
	u_int i;

	KASSERT(mutex_owned(&sess_lock));
	getnanouptime(&tsnow);

	/* Scan each session entry in the hash table. */
	for (i = 0; i < SESS_HASH_BUCKETS; i++) {
		npf_sentry_t *sen, *nsen;
		npf_sehash_t *sh;

		sh = &sess_hashtbl[i];
		if (sh->sh_count == 0) {
			continue;
		}

		rw_enter(&sh->sh_lock, RW_WRITER);
		/* For each (left -> right) ... */
		sen = rb_tree_iterate(&sh->sh_tree, NULL, RB_DIR_LEFT);
		while (sen != NULL) {
			npf_session_t *se;

			/* Get session, pre-iterate, skip if not expired. */
			se = sen->se_backptr;
			nsen = rb_tree_iterate(&sh->sh_tree, sen, RB_DIR_RIGHT);
			if (!npf_session_expired(se, &tsnow) && !flushall) {
				KASSERT((se->s_flags & SE_REMOVED) == 0);
				sen = nsen;
				continue;
			}

			/* Expired: remove from the tree. */
			atomic_or_uint(&se->s_flags, SE_EXPIRE);
			rb_tree_remove_node(&sh->sh_tree, sen);
			sh->sh_count--;

			/*
			 * Remove the session and move it to the G/C list,
			 * if we are removing the forwards entry.  The list
			 * is protected by its bucket lock.
			 */
			if (&se->s_forw_entry == sen) {
				atomic_or_uint(&se->s_flags, SE_REMFORW);
				LIST_REMOVE(se, s_list);
				LIST_INSERT_HEAD(gc_list, se, s_list);
			} else {
				atomic_or_uint(&se->s_flags, SE_REMBACK);
			}

			/* Next.. */
			sen = nsen;
		}
		KASSERT(!flushall || sh->sh_count == 0);
		rw_exit(&sh->sh_lock);
	}
}

/*
 * npf_session_worker: G/C to run from a worker thread.
 */
static void
npf_session_worker(void)
{
	npf_session_t *se, *nse;

	/*
	 * Garbage collect expired sessions.
	 */
	mutex_enter(&sess_lock);
	if (sess_hashtbl) {
		bool flush = (sess_tracking != SESS_TRACKING_ON);
		npf_session_gc(&sess_gc_list, flush);
	}
	if (sess_tracking == SESS_TRACKING_FLUSH) {
		/* Flush was requested - indicate we are done. */
		sess_tracking = SESS_TRACKING_OFF;
		cv_broadcast(&sess_cv);
	}
	mutex_exit(&sess_lock);
again:
	/*
	 * Destroy all sessions in the G/C list.
	 * May need to wait for the references to drain.
	 */
	se = LIST_FIRST(&sess_gc_list);
	while (se != NULL) {
		bool removed = (se->s_flags & SE_REMOVED) == SE_REMOVED;

		nse = LIST_NEXT(se, s_list);
		KASSERT((se->s_flags & SE_EXPIRE) != 0);
		if (removed && se->s_refcnt == 0) {
			/* Destroy only if removed and no references. */
			LIST_REMOVE(se, s_list);
			npf_session_destroy(se);
		}
		se = nse;
	}
	if (!LIST_EMPTY(&sess_gc_list)) {
		kpause("npfcongc", false, 1, NULL);
		goto again;
	}
}

void
npf_session_load(npf_sehash_t *newtbl)
{
	KASSERT(newtbl != NULL);
	npf_session_reload(newtbl, SESS_TRACKING_ON);
}

/*
 * npf_session_save: construct a list of sessions prepared for saving.
 * Note: this is expected to be an expensive operation.
 */
int
npf_session_save(prop_array_t selist, prop_array_t nplist)
{
	npf_sehash_t *sh;
	npf_session_t *se;
	int error = 0, i;

	/*
	 * If not tracking - empty.  Note: must acquire sess_lock to
	 * prevent from hash table destruction as well as expiring or
	 * removing of sessions by the G/C thread.
	 */
	mutex_enter(&sess_lock);
	if (sess_tracking != SESS_TRACKING_ON) {
		mutex_exit(&sess_lock);
		return 0;
	}
	for (i = 0; i < SESS_HASH_BUCKETS; i++) {
		sh = &sess_hashtbl[i];
		if (sh->sh_count == 0) {
			/* Empty bucket, next. */
			continue;
		}
		rw_enter(&sh->sh_lock, RW_READER);
		LIST_FOREACH(se, &sh->sh_list, s_list) {
			prop_dictionary_t sedict;
			prop_data_t sd;
			/*
			 * Create a copy of npf_session_t binary data and the
			 * unique identifier, which may be a pointer value.
			 * Set the data, insert into the array.
			 */
			sedict = prop_dictionary_create();
			sd = prop_data_create_data(se, sizeof(npf_session_t));
			prop_dictionary_set(sedict, "data", sd);
			prop_object_release(sd);

			CTASSERT(sizeof(uintptr_t) <= sizeof(uint64_t));
			prop_dictionary_set_uint64(
			    sedict, "id-ptr", (uintptr_t)se);

			if (se->s_nat) {
				/* Save NAT entry and policy, if any. */
				error = npf_nat_save(sedict, nplist, se->s_nat);
				if (error) {
					prop_object_release(sedict);
					break;
				}
			}
			prop_array_add(selist, sedict);
			prop_object_release(sedict);
		}
		rw_exit(&sh->sh_lock);
		if (error) {
			/* Note: caller will free the array. */
			break;
		}
	}
	mutex_exit(&sess_lock);
	return error;
}

/*
 * npf_session_restore: fully reconstruct a single session from a directory
 * and insert into the given hash table.
 */
int
npf_session_restore(npf_sehash_t *stbl, prop_dictionary_t sedict)
{
	npf_session_t *se;
	npf_sehash_t *fsh, *bsh;
	npf_sentry_t *fw, *bk;
	prop_object_t obj;
	const void *d;
	int error = 0;

	/* Get the pointer to the npf_session_t data and check size. */
	obj = prop_dictionary_get(sedict, "data");
	d = prop_data_data_nocopy(obj);
	if (d == NULL || prop_data_size(obj) != sizeof(npf_session_t)) {
		return EINVAL;
	}

	/*
	 * Copy the binary data of the structure.  Warning: must reset
	 * reference count, rule procedure and state lock.
	 */
	se = pool_cache_get(sess_cache, PR_WAITOK);
	memcpy(se, d, sizeof(npf_session_t));
	mutex_init(&se->s_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	se->s_refcnt = 0;
	se->s_rproc = NULL;

	/*
	 * Reconstruct NAT association, if any, or return NULL.
	 * Warning: must not leave stale entry.
	 */
	se->s_nat = npf_nat_restore(sedict, se);

	/*
	 * Find a hash bucket and insert each entry.
	 * Warning: must reset back pointers.
	 */
	fw = &se->s_forw_entry;
	fw->se_backptr = se;
	fsh = sess_hash_bucket(stbl, &se->s_common_id, fw);
	if (rb_tree_insert_node(&fsh->sh_tree, fw) != fw) {
		error = EINVAL;
		goto out;
	}
	fsh->sh_count++;

	bk = &se->s_back_entry;
	bk->se_backptr = se;
	bsh = sess_hash_bucket(stbl, &se->s_common_id, bk);
	if (rb_tree_insert_node(&bsh->sh_tree, bk) != bk) {
		rb_tree_remove_node(&fsh->sh_tree, fw);
		error = EINVAL;
		goto out;
	}
	bsh->sh_count++;

	/* Note: bucket of the forwards entry is for session list. */
	LIST_INSERT_HEAD(&fsh->sh_list, se, s_list);
out:
	if (error) {
		/* Drop, in a case of duplicate. */
		npf_session_destroy(se);
	}
	return error;
}

#if defined(DDB) || defined(_NPF_TESTING)

void npf_sessions_dump(void);

void
npf_sessions_dump(void)
{
	npf_sehash_t *sh;
	npf_sentry_t *sen;
	npf_session_t *se;
	struct timespec tsnow;

	getnanouptime(&tsnow);
	for (u_int i = 0; i < SESS_HASH_BUCKETS; i++) {
		sh = &sess_hashtbl[i];
		if (sh->sh_count == 0) {
			KASSERT(rb_tree_iterate(&sh->sh_tree,
			    NULL, RB_DIR_LEFT) == NULL);
			continue;
		}
		printf("s_bucket %d (%p, count %d)\n", i, sh, sh->sh_count);
		RB_TREE_FOREACH(sen, &sh->sh_tree) {
			struct timespec tsdiff;
			struct in_addr ip;
			int proto, etime;

			se = sen->se_backptr;
			proto = se->s_common_id.proto;
			timespecsub(&tsnow, &se->s_atime, &tsdiff);
			etime = npf_state_etime(&se->s_state, proto);

			printf("    %p[%p]:\n\t%s proto %d flags 0x%x "
			    "tsdiff %d etime %d\n", sen, se,
			    sen == &se->s_forw_entry ? "forw" : "back",
			    proto, se->s_flags, (int)tsdiff.tv_sec, etime);
			memcpy(&ip, &sen->se_src_addr, sizeof(ip));
			printf("\tsrc (%s, %d) ",
			    inet_ntoa(ip), ntohs(sen->se_src_id));
			memcpy(&ip, &sen->se_dst_addr, sizeof(ip));
			printf("dst (%s, %d)\n",
			    inet_ntoa(ip), ntohs(sen->se_dst_id));
			npf_state_dump(&se->s_state);
			if (se->s_nat != NULL) {
				npf_nat_dump(se->s_nat);
			}
		}
	}
}

#endif
