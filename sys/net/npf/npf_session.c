/*	$NetBSD: npf_session.c,v 1.13 2012/06/22 13:43:17 rmind Exp $	*/

/*-
 * Copyright (c) 2010-2012 The NetBSD Foundation, Inc.
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
 *	former purpose are created according to the rules with "keep state"
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
 * External session identifiers
 *
 *	Application-level gateways (ALGs) can inspect the packet and fill
 *	the packet cache (npf_cache_t) representing the IDs.  It is done
 *	via npf_alg_sessionid() call.  In such case, ALGs are responsible
 *	for correct filling of protocol, addresses and ports/IDs.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_session.c,v 1.13 2012/06/22 13:43:17 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/atomic.h>
#include <sys/condvar.h>
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

/*
 * Session structures: entry for embedding and the main structure.
 * WARNING: update npf_session_restore() when adding fields.
 */

typedef struct {
	/* Session entry node and backpointer to the actual session. */
	rb_node_t		se_rbnode;
	npf_session_t *		se_backptr;
	/* Size of the addresses. */
	int			se_addr_sz;
	/* Source and destination addresses. */
	npf_addr_t		se_src_addr;
	npf_addr_t		se_dst_addr;
	/* Source and destination ports (TCP / UDP) or generic IDs. */
	uint32_t		se_src_id;
	uint32_t		se_dst_id;
} npf_sentry_t;

struct npf_session {
	/* Session "forwards" and "backwards" entries. */
	npf_sentry_t		s_forw_entry;
	npf_sentry_t		s_back_entry;
	/* Entry in the session hash or G/C list. */
	LIST_ENTRY(npf_session)	s_list;
	u_int			s_refcnt;
	/* Session type.  Supported: TCP, UDP, ICMP. */
	int			s_type;
	int			s_flags;
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

struct npf_sehash {
	rb_tree_t		sh_tree;
	struct npf_sesslist	sh_list;
	krwlock_t		sh_lock;
	u_int			sh_count;
};

/*
 * Session flags:
 * - PFIL_IN and PFIL_OUT values are reserved for direction.
 * - SE_PASSING: a "pass" session.
 * - SE_EXPIRE: explicitly expire the session.
 * - SE_REMOVING: session is being removed (indicate need to enter G/C list).
 */
CTASSERT(PFIL_ALL == (0x001 | 0x002));
#define	SE_PASSSING		0x004
#define	SE_EXPIRE		0x008
#define	SE_REMOVING		0x010

static int			sess_tracking	__cacheline_aligned;

/* Session hash table, lock and session cache. */
static npf_sehash_t *		sess_hashtbl	__read_mostly;
static pool_cache_t		sess_cache	__read_mostly;

static kmutex_t			sess_lock;
static kcondvar_t		sess_cv;
static lwp_t *			sess_gc_lwp;

#define	SESS_GC_INTERVAL	5		/* 5 sec */

static void	sess_tracking_stop(void);
static void	npf_session_destroy(npf_session_t *);
static void	npf_session_worker(void *);

/*
 * npf_session_sys{init,fini}: initialise/destroy session handling structures.
 *
 * Session table and G/C thread are initialised when session tracking gets
 * actually enabled via npf_session_tracking() interface.
 */

void
npf_session_sysinit(void)
{

	mutex_init(&sess_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sess_cv, "npfgccv");
	sess_gc_lwp = NULL;
	sess_tracking = 0;
}

void
npf_session_sysfini(void)
{

	/* Disable tracking, flush all sessions. */
	npf_session_tracking(false);
	KASSERT(sess_tracking == 0);
	KASSERT(sess_gc_lwp == NULL);

	cv_destroy(&sess_cv);
	mutex_destroy(&sess_lock);
}

/*
 * Session hash table and RB-tree helper routines.
 * Order: (src.id, dst.id, src.addr, dst.addr), where (node1 < node2).
 */

static signed int
sess_rbtree_cmp_nodes(void *ctx, const void *n1, const void *n2)
{
	const npf_sentry_t * const sen1 = n1;
	const npf_sentry_t * const sen2 = n2;
	const int sz = sen1->se_addr_sz;
	int ret;

	/*
	 * Ports are the main criteria and are first.
	 */
	if (sen1->se_src_id != sen2->se_src_id) {
		return (sen1->se_src_id < sen2->se_src_id) ? -1 : 1;
	}
	if (sen1->se_dst_id != sen2->se_dst_id) {
		return (sen1->se_dst_id < sen2->se_dst_id) ? -1 : 1;
	}
	/*
	 * Note that hash should minimise differentiation on addresses.
	 */
	if (sen1->se_addr_sz != sen2->se_addr_sz) {
		return (sen1->se_addr_sz < sen2->se_addr_sz) ? -1 : 1;
	}
	if ((ret = memcmp(&sen1->se_src_addr, &sen2->se_src_addr, sz)) != 0) {
		return ret;
	}
	if ((ret = memcmp(&sen1->se_dst_addr, &sen2->se_dst_addr, sz)) != 0) {
		return ret;
	}
	return 0;
}

static signed int
sess_rbtree_cmp_key(void *ctx, const void *n1, const void *key)
{
	const npf_sentry_t * const sen1 = n1;
	const npf_sentry_t * const sen2 = key;

	KASSERT(sen1->se_addr_sz != 0 && sen2->se_addr_sz != 0);
	return sess_rbtree_cmp_nodes(NULL, sen1, sen2);
}

static const rb_tree_ops_t sess_rbtree_ops = {
	.rbto_compare_nodes = sess_rbtree_cmp_nodes,
	.rbto_compare_key = sess_rbtree_cmp_key,
	.rbto_node_offset = offsetof(npf_sentry_t, se_rbnode),
	.rbto_context = NULL
};

static inline npf_sehash_t *
sess_hash_bucket(npf_sehash_t *stbl, const int proto, npf_sentry_t *sen)
{
	const int sz = sen->se_addr_sz;
	uint32_t hash, mix;

	/* Sum protocol and both addresses (for both directions). */
	mix = proto + npf_addr_sum(sz, &sen->se_src_addr, &sen->se_dst_addr);
	hash = hash32_buf(&mix, sizeof(uint32_t), HASH32_BUF_INIT);
	return &stbl[hash & SESS_HASH_MASK];
}

npf_sehash_t *
sess_htable_create(void)
{
	npf_sehash_t *stbl, *sh;
	u_int i;

	stbl = kmem_zalloc(SESS_HASH_BUCKETS * sizeof(*sh), KM_SLEEP);
	if (stbl == NULL) {
		return NULL;
	}
	for (i = 0; i < SESS_HASH_BUCKETS; i++) {
		sh = &stbl[i];
		LIST_INIT(&sh->sh_list);
		rb_tree_init(&sh->sh_tree, &sess_rbtree_ops);
		rw_init(&sh->sh_lock);
		sh->sh_count = 0;
	}
	return stbl;
}

void
sess_htable_destroy(npf_sehash_t *stbl)
{
	npf_sehash_t *sh;
	u_int i;

	for (i = 0; i < SESS_HASH_BUCKETS; i++) {
		sh = &stbl[i];
		KASSERT(sh->sh_count == 0);
		KASSERT(LIST_EMPTY(&sh->sh_list));
		KASSERT(!rb_tree_iterate(&sh->sh_tree, NULL, RB_DIR_LEFT));
		rw_destroy(&sh->sh_lock);
	}
	kmem_free(stbl, SESS_HASH_BUCKETS * sizeof(*sh));
}

void
sess_htable_reload(npf_sehash_t *stbl)
{
	npf_sehash_t *oldstbl;

	mutex_enter(&sess_lock);
	/* Flush all existing entries. */
	sess_tracking = -1;	/* XXX */
	cv_signal(&sess_cv);
	cv_wait(&sess_cv, &sess_lock);
	sess_tracking = 1;
	/* Set a new session table. */
	oldstbl = sess_hashtbl;
	sess_hashtbl = stbl;
	mutex_exit(&sess_lock);
	/* Destroy the old table. */
	sess_htable_destroy(oldstbl);
}

/*
 * Session tracking routines.  Note: manages tracking structures.
 */

static int
sess_tracking_start(void)
{

	sess_cache = pool_cache_init(sizeof(npf_session_t), coherency_unit,
	    0, 0, "npfsespl", NULL, IPL_NET, NULL, NULL, NULL);
	if (sess_cache == NULL)
		return ENOMEM;

	sess_hashtbl = sess_htable_create();
	if (sess_hashtbl == NULL) {
		pool_cache_destroy(sess_cache);
		return ENOMEM;
	}

	/* Make it visible before thread start. */
	sess_tracking = 1;

	if (kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
	    npf_session_worker, NULL, &sess_gc_lwp, "npfgc")) {
		sess_tracking_stop();
		return ENOMEM;
	}
	return 0;
}

static void
sess_tracking_stop(void)
{

	/* Notify G/C thread to flush all sessions, wait for the exit. */
	mutex_enter(&sess_lock);
	sess_tracking = 0;
	cv_signal(&sess_cv);
	while (sess_gc_lwp != NULL) {
		cv_wait(&sess_cv, &sess_lock);
	}
	mutex_exit(&sess_lock);

	sess_htable_destroy(sess_hashtbl);
	pool_cache_destroy(sess_cache);
}

/*
 * npf_session_tracking: enable/disable session tracking.
 *
 * => XXX: serialize at upper layer; ignore for now.
 */
int
npf_session_tracking(bool track)
{

	if (!sess_tracking && track) {
		/* Disabled -> Enable. */
		return sess_tracking_start();
	}
	if (sess_tracking && !track) {
		/* Enabled -> Disable. */
		sess_tracking_stop();
		return 0;
	}
	return 0;
}

/*
 * npf_session_inspect: look if there is an established session (connection).
 *
 * => If found, we will hold a reference for caller.
 */
npf_session_t *
npf_session_inspect(npf_cache_t *npc, nbuf_t *nbuf, const int di, int *error)
{
	npf_sehash_t *sh;
	npf_sentry_t *sen;
	npf_session_t *se;

	/*
	 * If layer 3 and 4 are not cached - protocol is not supported
	 * or packet is invalid.
	 */
	if (!sess_tracking || !npf_iscached(npc, NPC_IP46) ||
	    !npf_iscached(npc, NPC_LAYER4)) {
		return NULL;
	}

	/*
	 * Construct a key for hash and tree lookup.  Execute ALG session
	 * helpers, which may construct a custom key.
	 */
	npf_cache_t algkey = { .npc_info = 0 }, *key;
	npf_sentry_t senkey;

	if (!npf_alg_sessionid(npc, nbuf, &algkey)) {
		/* Default: use the cache data of original packet. */
		key = npc;
	} else {
		/* Unique IDs filled by ALG in a separate key cache. */
		key = &algkey;
	}

	/* Note: take protocol from the key. */
	const int proto = npf_cache_ipproto(key);

	if (proto == IPPROTO_TCP) {
		const struct tcphdr *th = &key->npc_l4.tcp;
		senkey.se_src_id = th->th_sport;
		senkey.se_dst_id = th->th_dport;
	} else if (proto == IPPROTO_UDP) {
		const struct udphdr *uh = &key->npc_l4.udp;
		senkey.se_src_id = uh->uh_sport;
		senkey.se_dst_id = uh->uh_dport;
	} else if (npf_iscached(key, NPC_ICMP_ID)) {
		const struct icmp *ic = &key->npc_l4.icmp;
		senkey.se_src_id = ic->icmp_id;
		senkey.se_dst_id = ic->icmp_id;
	}
	KASSERT(key->npc_srcip && key->npc_dstip && key->npc_ipsz > 0);
	memcpy(&senkey.se_src_addr, key->npc_srcip, key->npc_ipsz);
	memcpy(&senkey.se_dst_addr, key->npc_dstip, key->npc_ipsz);
	senkey.se_addr_sz = key->npc_ipsz;

	/*
	 * Get a hash bucket from the cached key data.
	 * Pre-check if there are any entries in the hash table.
	 */
	sh = sess_hash_bucket(sess_hashtbl, proto, &senkey);
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

	/* Match direction and check if not explicitly expired. */
	const bool forw = (sen == &se->s_forw_entry);
	const int se_di = se->s_flags & PFIL_ALL;
	if (forw != (se_di == di) || (se->s_flags & SE_EXPIRE) != 0) {
		rw_exit(&sh->sh_lock);
		return NULL;
	}

	/* Inspect the protocol data and handle state changes. */
	if (npf_state_inspect(npc, nbuf, &se->s_state, forw)) {
		/* Update the last activity time and hold a reference. */
		getnanouptime(&se->s_atime);
		atomic_inc_uint(&se->s_refcnt);
	} else {
		/* Silently block invalid packets. */
		npf_stats_inc(NPF_STAT_INVALID_STATE);
		*error = ENETUNREACH;
		se = NULL;
	}
	rw_exit(&sh->sh_lock);
	return se;
}

/*
 * npf_establish_session: create a new session, insert into the global list.
 *
 * => Session is created with the reference held for the caller.
 */
npf_session_t *
npf_session_establish(const npf_cache_t *npc, nbuf_t *nbuf, const int di)
{
	const struct tcphdr *th;
	const struct udphdr *uh;
	npf_sentry_t *fw, *bk;
	npf_sehash_t *sh;
	npf_session_t *se;
	int proto, sz;
	bool ok;

	/*
	 * If layer 3 and 4 are not cached - protocol is not supported
	 * or packet is invalid.
	 */
	if (!sess_tracking || !npf_iscached(npc, NPC_IP46) ||
	    !npf_iscached(npc, NPC_LAYER4)) {
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
	se->s_refcnt = 1;
	se->s_flags = (di & PFIL_ALL);
	se->s_rproc = NULL;
	se->s_nat = NULL;

	/* Unique IDs: IP addresses.  Setup "forwards" entry first. */
	KASSERT(npf_iscached(npc, NPC_IP46));
	sz = npc->npc_ipsz;
	fw = &se->s_forw_entry;
	memcpy(&fw->se_src_addr, npc->npc_srcip, sz);
	memcpy(&fw->se_dst_addr, npc->npc_dstip, sz);

	/* Initialize protocol state. */
	if (!npf_state_init(npc, nbuf, &se->s_state)) {
		ok = false;
		goto out;
	}

	/* Procotol. */
	proto = npf_cache_ipproto(npc);
	se->s_type = proto;

	switch (proto) {
	case IPPROTO_TCP:
		KASSERT(npf_iscached(npc, NPC_TCP));
		th = &npc->npc_l4.tcp;
		/* Additional IDs: ports. */
		fw->se_src_id = th->th_sport;
		fw->se_dst_id = th->th_dport;
		break;
	case IPPROTO_UDP:
		KASSERT(npf_iscached(npc, NPC_UDP));
		/* Additional IDs: ports. */
		uh = &npc->npc_l4.udp;
		fw->se_src_id = uh->uh_sport;
		fw->se_dst_id = uh->uh_dport;
		break;
	case IPPROTO_ICMP:
		if (npf_iscached(npc, NPC_ICMP_ID)) {
			/* ICMP query ID. */
			const struct icmp *ic = &npc->npc_l4.icmp;
			fw->se_src_id = ic->icmp_id;
			fw->se_dst_id = ic->icmp_id;
			break;
		}
		/* FALLTHROUGH */
	default:
		/* Unsupported. */
		ok = false;
		goto out;
	}

	/* Set last activity time for a new session. */
	getnanouptime(&se->s_atime);

	/* Setup inverted "backwards". */
	bk = &se->s_back_entry;
	memcpy(&bk->se_src_addr, &fw->se_dst_addr, sz);
	memcpy(&bk->se_dst_addr, &fw->se_src_addr, sz);
	bk->se_src_id = fw->se_dst_id;
	bk->se_dst_id = fw->se_src_id;

	/* Finish the setup of entries. */
	fw->se_backptr = bk->se_backptr = se;
	fw->se_addr_sz = bk->se_addr_sz = sz;

	/*
	 * Insert the session and both entries into the tree.
	 */
	sh = sess_hash_bucket(sess_hashtbl, se->s_type, fw);
	KASSERT(sh == sess_hash_bucket(sess_hashtbl, se->s_type, bk));

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
		npf_nat_expire(se->s_nat);
	}
	if (se->s_rproc) {
		/* Release rule procedure. */
		npf_rproc_release(se->s_rproc);
	}

	/* Destroy the state. */
	npf_state_destroy(&se->s_state);

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
npf_session_setnat(npf_session_t *se, npf_nat_t *nt, const int di)
{
	npf_sehash_t *sh;
	npf_sentry_t *sen;
	npf_addr_t *taddr;
	in_port_t tport;
	bool ok;

	KASSERT(se->s_refcnt > 0);

	/* First, atomically check and associate NAT entry. */
	if (atomic_cas_ptr(&se->s_nat, NULL, nt) != NULL) {
		/* Race: see below for description. */
		npf_stats_inc(NPF_STAT_RACE_NAT);
		return EISCONN;
	}

	/*
	 * Update, re-hash and re-insert "backwards" entry, according to
	 * the translation.  First, remove the entry from tree.  Note that
	 * a duplicate packet may establish a duplicate session while lock
	 * will be released.  In such case, caller will drop this packet
	 * and structures associated with it.  Such race condition should
	 * never happen in practice, though.
	 */
	sen = &se->s_back_entry;
	sh = sess_hash_bucket(sess_hashtbl, se->s_type, sen);

	rw_enter(&sh->sh_lock, RW_WRITER);
	rb_tree_remove_node(&sh->sh_tree, sen);
	sh->sh_count--;
	rw_exit(&sh->sh_lock);

	/*
	 * New source/destination and hash.  Note that source/destination
	 * are inverted, since we are handling "backwards" entry.
	 */
	npf_nat_gettrans(nt, &taddr, &tport);
	if (di == PFIL_OUT) {
		/* NPF_NATOUT: source in "forwards" = destination. */
		memcpy(&sen->se_dst_addr, taddr, sen->se_addr_sz);
		if (tport) {
			sen->se_dst_id = tport;
		}
	} else {
		/* NPF_NATIN: destination in "forwards" = source. */
		memcpy(&sen->se_src_addr, taddr, sen->se_addr_sz);
		if (tport) {
			sen->se_src_id = tport;
		}
	}
	sh = sess_hash_bucket(sess_hashtbl, se->s_type, sen);

	/* Insert into the new bucket. */
	rw_enter(&sh->sh_lock, RW_WRITER);
	ok = (rb_tree_insert_node(&sh->sh_tree, sen) == sen);
	if (__predict_true(ok)) {
		sh->sh_count++;
		NPF_PRINTF(("NPF: se %p assoc with nat %p\n", se, se->s_nat));
	} else {
		/* FIXMEgc */
		printf("npf_session_setnat: Houston, we've had a problem.\n");
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
	se->s_flags |= SE_EXPIRE;		/* XXXSMP */
}

/*
 * npf_session_pass: return true if session is "pass" one, otherwise false.
 */
bool
npf_session_pass(const npf_session_t *se, npf_rproc_t **rp)
{

	KASSERT(se->s_refcnt > 0);
	if ((se->s_flags & SE_PASSSING) != 0) {
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

	KASSERT(se->s_refcnt > 0);
	KASSERT(se->s_rproc == NULL);
	se->s_flags |= SE_PASSSING;		/* XXXSMP */
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
	const int etime = npf_state_etime(&se->s_state, se->s_type);
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
	npf_sentry_t *sen, *nsen;
	npf_session_t *se;
	u_int i;

	getnanouptime(&tsnow);

	/* Scan each session entry in the hash table. */
	for (i = 0; i < SESS_HASH_BUCKETS; i++) {
		npf_sehash_t *sh;

		sh = &sess_hashtbl[i];
		if (sh->sh_count == 0) {
			continue;
		}
		rw_enter(&sh->sh_lock, RW_WRITER);
		/* For each (left -> right) ... */
		sen = rb_tree_iterate(&sh->sh_tree, NULL, RB_DIR_LEFT);
		while (sen != NULL) {
			/* Get session, pre-iterate, skip if not expired. */
			se = sen->se_backptr;
			nsen = rb_tree_iterate(&sh->sh_tree, sen, RB_DIR_RIGHT);
			if (!npf_session_expired(se, &tsnow) && !flushall) {
				KASSERT((se->s_flags & SE_REMOVING) == 0);
				sen = nsen;
				continue;
			}

			/* Expired - remove from the tree. */
			rb_tree_remove_node(&sh->sh_tree, sen);
			sh->sh_count--;

			/*
			 * Remove session, if forwards entry.  Set removal bit
			 * when first entry is removed.  If it is already set,
			 * then it is a second entry removal, therefore move
			 * the session into the G/C list.
			 */
			if (sen == &se->s_forw_entry) {
				LIST_REMOVE(se, s_list);
			}
			if (se->s_flags & SE_REMOVING) {
				LIST_INSERT_HEAD(gc_list, se, s_list);
			} else {
				se->s_flags |= SE_REMOVING;
			}

			/* Next.. */
			sen = nsen;
		}
		KASSERT(!flushall || sh->sh_count == 0);
		rw_exit(&sh->sh_lock);
	}
}

/*
 * npf_session_freelist: destroy all sessions, which have no references,
 * in the given G/C list.  Return true, if the list is empty.
 */
static void
npf_session_freelist(struct npf_sesslist *gc_list)
{
	npf_session_t *se, *nse;

	se = LIST_FIRST(gc_list);
	while (se != NULL) {
		nse = LIST_NEXT(se, s_list);
		if (se->s_refcnt == 0) {
			/* Destroy only if no references. */
			LIST_REMOVE(se, s_list);
			npf_session_destroy(se);
		}
		se = nse;
	}
}

/*
 * npf_session_worker: G/C worker thread.
 */
static void
npf_session_worker(void *arg)
{
	struct npf_sesslist gc_list;
	bool flushreq = false;

	LIST_INIT(&gc_list);
	do {
		/* Periodically wake up, unless get notified. */
		mutex_enter(&sess_lock);
		if (flushreq) {
			/* Flush was performed, notify waiter. */
			cv_signal(&sess_cv);
		}
		(void)cv_timedwait(&sess_cv, &sess_lock, SESS_GC_INTERVAL);
		flushreq = (sess_tracking != 1);	/* XXX */
		npf_session_gc(&gc_list, flushreq);
		mutex_exit(&sess_lock);

		npf_session_freelist(&gc_list);

	} while (sess_tracking);

	/* Wait for any referenced sessions to be released. */
	while (!LIST_EMPTY(&gc_list)) {
		kpause("npfgcfr", false, 1, NULL);
		npf_session_freelist(&gc_list);
	}

	/* Notify that we are done. */
	mutex_enter(&sess_lock);
	sess_gc_lwp = NULL;
	cv_signal(&sess_cv);
	mutex_exit(&sess_lock);

	kthread_exit(0);
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

	/* If not tracking - empty. */
	if (!sess_tracking) {
		return 0;
	}

	/*
	 * Note: hold the session lock to prevent G/C thread from session
	 * expiring and removing.  Therefore, no need to exclusively lock
	 * the entire hash table.
	 */
	mutex_enter(&sess_lock);
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
	npf_state_t *nst;
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
	se->s_refcnt = 0;
	se->s_rproc = NULL;

	nst = &se->s_state;
	mutex_init(&nst->nst_lock, MUTEX_DEFAULT, IPL_SOFTNET);

	/*
	 * Reconstruct NAT association, if any, or return NULL.
	 * Warning: must not leave stale entry.
	 */
	se->s_nat = npf_nat_restore(sedict, se);

	/*
	 * Find a hash bucket and insert each entry.
	 * Warning: must reset back pointer.
	 */
	fw = &se->s_forw_entry;
	fw->se_backptr = se;
	fsh = sess_hash_bucket(stbl, se->s_type, fw);
	if (rb_tree_insert_node(&fsh->sh_tree, fw) != fw) {
		error = EINVAL;
		goto out;
	}
	fsh->sh_count++;

	bk = &se->s_back_entry;
	bk->se_backptr = se;
	bsh = sess_hash_bucket(stbl, se->s_type, bk);
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
			int etime;

			se = sen->se_backptr;
			timespecsub(&tsnow, &se->s_atime, &tsdiff);
			etime = npf_state_etime(&se->s_state, se->s_type);

			printf("    %p[%p]: %s proto %d flags 0x%x tsdiff %d "
			    "etime %d\n", sen, se, sen == &se->s_forw_entry ?
			    "forw" : "back",  se->s_type, se->s_flags,
			    (int)tsdiff.tv_sec, etime);
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
