/*	$NetBSD: npf_session.c,v 1.4.4.2 2010/10/22 09:23:15 uebayasi Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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
 *	use the terms "forwards stream" and "backwards stream".
 *
 *	There are two types of sessions: "pass" and "NAT".  The former are
 *	sessions created according to the rules with "keep state" attribute
 *	and are used for stateful filtering.  Such sessions indicate that
 *	packet of the "backwards" stream should be passed without inspection
 *	of the ruleset.
 *
 *	NAT sessions are created according to the NAT policies.  Since they
 *	are used to perform translation, such sessions have 1:1 relationship
 *	with NAT translation structure via npf_session_t::s_nat.  Therefore,
 *	non-NULL value of npf_session_t::s_nat indicates this session type.
 *
 * Session life-cycle
 *
 *	Sessions are established when packet matches said rule or NAT policy.
 *	Established session is inserted into the hashed tree.  A garbage
 *	collection thread periodically scans all sessions and depending on
 *	their properties (e.g. last activity time, protocol) expires them.
 *
 *	Each session has a reference count, which is taken on lookup and
 *	needs to be released by the caller.  Reference guarantees that
 *	session will not be destroyed, although it might be expired.
 *
 * Linked sessions
 *
 *	Often NAT policies have overlapping stateful filtering rules.  In
 *	order to avoid unnecessary lookups, "pass" session can be linked
 *	with a "NAT" session (npf_session_t::s_nat_se pointer).  Such link
 *	is used to detect translation on "forwards" stream.
 *
 *	Additional reference is held on linked "NAT" sessions to prevent
 *	them from destruction while linked.  Link is broken and reference
 *	is dropped when "pass" session expires.
 *
 * External session identifiers
 *
 *	Application-level gateways (ALGs) can inspect the packet and fill
 *	the packet cache (npf_cache_t) representing the IDs.  It is done
 *	via npf_alg_sessionid() call.  In such case, ALGs are responsible
 *	for correct filling of protocol, addresses and ports/IDs.
 *
 * TODO:
 * - Session monitoring via descriptor.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_session.c,v 1.4.4.2 2010/10/22 09:23:15 uebayasi Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#endif
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
#include <sys/types.h>

#include "npf_impl.h"

struct npf_session {
	/* Session node / list entry and reference count. */
	union {
		struct rb_node		rbnode;
		LIST_ENTRY(npf_session)	gclist;
	} se_entry;
	u_int				s_refcnt;
	/* Session type.  Supported: TCP, UDP, ICMP. */
	int				s_type;
	int				s_direction;
	uint16_t			s_state;
	uint16_t			s_flags;
	/* NAT data associated with this session (if any). */
	npf_nat_t *			s_nat;
	npf_session_t *			s_nat_se;
	/* Source and destination addresses. */
	in_addr_t			s_src_addr;
	in_addr_t			s_dst_addr;
	/* Source and destination ports (TCP / UDP) or generic IDs. */
	union {
		in_port_t		port;
		uint32_t		id;
	} s_src;
	union {
		in_port_t		port;
		uint32_t		id;
	} s_dst;
	/* Last activity time (used to calculate expiration time). */
	struct timespec 		s_atime;
};

LIST_HEAD(npf_sesslist, npf_session);

#define	SESS_HASH_BUCKETS		1024	/* XXX tune + make tunable */
#define	SESS_HASH_MASK			(SESS_HASH_BUCKETS - 1)

typedef struct {
	rb_tree_t			sh_tree;
	krwlock_t			sh_lock;
	u_int				sh_count;
} npf_sess_hash_t;

static int				sess_tracking	__cacheline_aligned;

/* Session hash table, lock and session cache. */
static npf_sess_hash_t *		sess_hashtbl	__read_mostly;
static pool_cache_t			sess_cache	__read_mostly;

static kmutex_t				sess_lock;
static kcondvar_t			sess_cv;
static lwp_t *				sess_gc_lwp;

#define	SESS_GC_INTERVAL		5		/* 5 sec */

/* Session expiration table.  XXX: TCP close: 2 * tcp_msl (e.g. 120)?  Maybe. */
static const u_int sess_expire_table[ ] = {
	[IPPROTO_TCP]		= 600,		/* 10 min */
	[IPPROTO_UDP]		= 300,		/*  5 min */
	[IPPROTO_ICMP]		= 30		/*  1 min */
};

/* Session states and flags. */
#define	SE_OPENING		1
#define	SE_ACKNOWLEDGE		2
#define	SE_ESTABLISHED		3
#define	SE_CLOSING		4

#define	SE_PASSSING		0x01

static void	sess_tracking_stop(void);
static void	npf_session_worker(void *);

#ifdef SE_DEBUG
#define	DPRINTF(x)	printf x
#else
#define	DPRINTF(x)
#endif

/*
 * npf_session_sys{init,fini}: initialise/destroy session handling structures.
 *
 * Session table and G/C thread are initialised when session tracking gets
 * actually enabled via npf_session_tracking() interface.
 */

int
npf_session_sysinit(void)
{

	mutex_init(&sess_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sess_cv, "npfgccv");
	sess_gc_lwp = NULL;
	sess_tracking = 0;
	return 0;
}

void
npf_session_sysfini(void)
{
	int error;

	/* Disable tracking to destroy all structures. */
	error = npf_session_tracking(false);
	KASSERT(error == 0);
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
	const npf_session_t * const se1 = n1;
	const npf_session_t * const se2 = n2;

	/*
	 * Note: must compare equivalent streams.
	 * See sess_rbtree_cmp_key() below.
	 */
	if (se1->s_direction == se2->s_direction) {
		/*
		 * Direction "forwards".
		 */
		if (se1->s_src.id != se2->s_src.id)
			return (se1->s_src.id < se2->s_src.id) ? -1 : 1;
		if (se1->s_dst.id != se2->s_dst.id)
			return (se1->s_dst.id < se2->s_dst.id) ? -1 : 1;

		if (__predict_false(se1->s_src_addr != se2->s_src_addr))
			return (se1->s_src_addr < se2->s_src_addr) ? -1 : 1;
		if (__predict_false(se1->s_dst_addr != se2->s_dst_addr))
			return (se1->s_dst_addr < se2->s_dst_addr) ? -1 : 1;
	} else {
		/*
		 * Direction "backwards".
		 */
		if (se1->s_src.id != se2->s_dst.id)
			return (se1->s_src.id < se2->s_dst.id) ? -1 : 1;
		if (se1->s_dst.id != se2->s_src.id)
			return (se1->s_dst.id < se2->s_src.id) ? -1 : 1;

		if (__predict_false(se1->s_src_addr != se2->s_dst_addr))
			return (se1->s_src_addr < se2->s_dst_addr) ? -1 : 1;
		if (__predict_false(se1->s_dst_addr != se2->s_src_addr))
			return (se1->s_dst_addr < se2->s_src_addr) ? -1 : 1;
	}
	return 0;
}

static signed int
sess_rbtree_cmp_key(void *ctx, const void *n1, const void *key)
{
	const npf_session_t * const se = n1;
	const npf_cache_t * const npc = key;
	in_port_t sport, dport;
	in_addr_t src, dst;

	if (se->s_direction == npc->npc_dir) {
		/* Direction "forwards". */
		src = npc->npc_srcip; sport = npc->npc_sport;
		dst = npc->npc_dstip; dport = npc->npc_dport;
	} else {
		/* Direction "backwards". */
		src = npc->npc_dstip; sport = npc->npc_dport;
		dst = npc->npc_srcip; dport = npc->npc_sport;
	}

	/* Ports are the main criteria and are first. */
	if (se->s_src.id != sport)
		return (se->s_src.id < sport) ? -1 : 1;

	if (se->s_dst.id != dport)
		return (se->s_dst.id < dport) ? -1 : 1;

	/* Note that hash should minimise differentiation on these. */
	if (__predict_false(se->s_src_addr != src))
		return (se->s_src_addr < src) ? -1 : 1;
	if (__predict_false(se->s_dst_addr < dst))
		return (se->s_dst_addr < dst) ? -1 : 1;

	return 0;
}

static const rb_tree_ops_t sess_rbtree_ops = {
	.rbto_compare_nodes = sess_rbtree_cmp_nodes,
	.rbto_compare_key = sess_rbtree_cmp_key,
	.rbto_node_offset = offsetof(npf_session_t, se_entry.rbnode),
	.rbto_context = NULL
};

static inline npf_sess_hash_t *
sess_hash_bucket(const npf_cache_t *key)
{
	uint32_t hash, mix;

	KASSERT(npf_iscached(key, NPC_IP46 | NPC_ADDRS));

	/* Sum addresses for both directions and mix in protocol. */
	mix = key->npc_srcip + key->npc_dstip + key->npc_proto;
	hash = hash32_buf(&mix, sizeof(uint32_t), HASH32_BUF_INIT);
	return &sess_hashtbl[hash & SESS_HASH_MASK];
}

static npf_sess_hash_t *
sess_hash_construct(void)
{
	npf_sess_hash_t *ht, *sh;
	u_int i;

	ht = kmem_alloc(SESS_HASH_BUCKETS * sizeof(*sh), KM_SLEEP);
	if (ht == NULL) {
		return NULL;
	}
	for (i = 0; i < SESS_HASH_BUCKETS; i++) {
		sh = &ht[i];
		rb_tree_init(&sh->sh_tree, &sess_rbtree_ops);
		rw_init(&sh->sh_lock);
		sh->sh_count = 0;
	}
	return ht;
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

	sess_hashtbl = sess_hash_construct();
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
	npf_sess_hash_t *sh;
	u_int i;

	/* Notify G/C thread to flush all sessions, wait for the exit. */
	mutex_enter(&sess_lock);
	sess_tracking = 0;
	cv_signal(&sess_cv);
	while (sess_gc_lwp != NULL) {
		cv_wait(&sess_cv, &sess_lock);
	}
	mutex_exit(&sess_lock);

	/* Destroy and free the hash table with other structures. */
	for (i = 0; i < SESS_HASH_BUCKETS; i++) {
		sh = &sess_hashtbl[i];
		rw_destroy(&sh->sh_lock);
	}
	kmem_free(sess_hashtbl, SESS_HASH_BUCKETS * sizeof(*sh));
	pool_cache_destroy(sess_cache);
}

/*
 * npf_session_tracking: enable/disable session tracking.
 *
 * => Called before ruleset reload.
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
	if (sess_tracking && track) {
		/*
		 * Enabled -> Re-enable.
		 * Flush existing entries.
		 */
		mutex_enter(&sess_lock);
		sess_tracking = -1;	/* XXX */
		cv_signal(&sess_cv);
		cv_wait(&sess_cv, &sess_lock);
		sess_tracking = 1;
		mutex_exit(&sess_lock);
	} else {
		/* Disabled -> Disable. */
	}
	return 0;
}

/*
 * npf_session_pstate: handle session state according to protocol data.
 */
static inline bool
npf_session_pstate(const npf_cache_t *npc, npf_session_t *se, const int dir)
{
	const bool backwards = (se->s_direction != dir);
	const int proto = npc->npc_proto;

	if (proto != IPPROTO_TCP) {
		/* Handle UDP or ICMP response for opening session. */
		if (se->s_state == SE_OPENING && backwards) {
			se->s_state = SE_ESTABLISHED;
		}
		return true;
	}

	const int tcpfl = npc->npc_tcp_flags & (TH_SYN|TH_ACK|TH_RST|TH_FIN);

	switch (tcpfl) {
	case TH_ACK:
		/* Common case. */
		if (__predict_true(se->s_state == SE_ESTABLISHED)) {
			return true;
		}
		/* ACK seen after SYN-ACK: session fully established. */
		if (se->s_state == SE_ACKNOWLEDGE && !backwards) {
			se->s_state = SE_ESTABLISHED;
		}
		break;
	case TH_SYN | TH_ACK:
		/* SYN-ACK seen, wait for ACK. */
		if (se->s_state == SE_OPENING && backwards) {
			se->s_state = SE_ACKNOWLEDGE;
		}
		break;
	case TH_RST:
	case TH_FIN:
		/* XXX/TODO: Handle TCP reset attacks; later. */
		se->s_state = SE_CLOSING;
		break;
	}
	return true;
}

/*
 * npf_session_inspect: look if there is an established session (connection).
 *
 * => If found, we will hold a reference for caller.
 */
npf_session_t *
npf_session_inspect(npf_cache_t *npc, nbuf_t *nbuf,
    struct ifnet *ifp, const int di)
{
	npf_sess_hash_t *sh;
	npf_session_t *se;

	/* Attempt to fetch and cache all relevant IPv4 data. */
	if (!sess_tracking || !npf_cache_all(npc, nbuf)) {
		return NULL;
	}
	KASSERT(npf_iscached(npc, NPC_IP46 | NPC_ADDRS));
	KASSERT(npf_iscached(npc, NPC_PORTS) || npf_iscached(npc, NPC_ICMP));

	/*
	 * Execute ALG session helpers.
	 */
	npf_cache_t algkey, *key;

	if (npf_alg_sessionid(npc, nbuf, &algkey)) {
		/* Unique IDs filled by ALG in a separate key cache. */
		key = &algkey;
	} else {
		/* Default: original packet, pass its cache. */
		key = npc;
	}
	key->npc_dir = di;

	/*
	 * Get a hash bucket from the cached key data.
	 * Pre-check if there are any entries in the hash table.
	 */
	sh = sess_hash_bucket(key);
	if (sh->sh_count == 0) {
		return NULL;
	}

	/* Lookup the tree for a state entry. */
	rw_enter(&sh->sh_lock, RW_READER);
	se = rb_tree_find_node(&sh->sh_tree, key);
	if (se == NULL) {
		rw_exit(&sh->sh_lock);
		return NULL;
	}

	/* Inspect the protocol data and handle state changes. */
	if (npf_session_pstate(npc, se, di)) {
		/* Must update the last activity time. */
		getnanouptime(&se->s_atime);
		/* Hold a reference. */
		atomic_inc_uint(&se->s_refcnt);
	} else {
		se = NULL;
	}
	rw_exit(&sh->sh_lock);

	return se;
}

/*
 * npf_establish_session: create a new session, insert into the global list.
 *
 * => Sessions is created with the held reference (for caller).
 */
npf_session_t *
npf_session_establish(const npf_cache_t *npc, npf_nat_t *nt, const int di)
{
	npf_sess_hash_t *sh;
	npf_session_t *se;
	bool ok;

	if (!sess_tracking)	/* XXX */
		return NULL;

	/* Allocate and initialise new state. */
	se = pool_cache_get(sess_cache, PR_NOWAIT);
	if (__predict_false(se == NULL)) {
		return NULL;
	}
	/* Reference count and direction. */
	se->s_refcnt = 1;
	se->s_direction = di;
	se->s_flags = 0;

	/* NAT and backwards session. */
	se->s_nat = nt;
	se->s_nat_se = NULL;

	/* Unique IDs: IP addresses. */
	KASSERT(npf_iscached(npc, NPC_IP46 | NPC_ADDRS));
	se->s_src_addr = npc->npc_srcip;
	se->s_dst_addr = npc->npc_dstip;

	/* Procotol. */
	se->s_type = npc->npc_proto;

	switch (npc->npc_proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
		KASSERT(npf_iscached(npc, NPC_PORTS));
		/* Additional IDs: ports. */
		se->s_src.id = npc->npc_sport;
		se->s_dst.id = npc->npc_dport;
		break;
	case IPPROTO_ICMP:
		if (npf_iscached(npc, NPC_ICMP_ID)) {
			/* ICMP query ID. (XXX) */
			se->s_src.id = npc->npc_icmp_id;
			se->s_dst.id = npc->npc_icmp_id;
			break;
		}
		/* FALLTHROUGH */
	default:
		/* Unsupported. */
		pool_cache_put(sess_cache, se);
		return NULL;
	}

	/* Set last activity time for a new session. */
	se->s_state = SE_OPENING;
	getnanouptime(&se->s_atime);

	/* Find the hash bucket and insert the state into the tree. */
	sh = sess_hash_bucket(npc);
	rw_enter(&sh->sh_lock, RW_WRITER);
	ok = rb_tree_insert_node(&sh->sh_tree, se) == se;
	if (__predict_true(ok)) {
		sh->sh_count++;
		DPRINTF(("NPF: new se %p (link %p, nat %p)\n",
		    se, se->s_nat_se, se->s_nat));
	}
	rw_exit(&sh->sh_lock);

	if (__predict_false(!ok)) {
		/* Race with duplicate packet. */
		pool_cache_put(sess_cache, se);
		return NULL;
	}
	return se;
}

/*
 * npf_session_pass: return true if session is "pass" one, otherwise false.
 */
bool
npf_session_pass(const npf_session_t *se)
{

	KASSERT(se->s_refcnt > 0);
	return (se->s_flags & SE_PASSSING) != 0;
}

/*
 * npf_session_setpass: mark session as a "pass" one, also mark the
 * linked session if there is one.
 */
void
npf_session_setpass(npf_session_t *se)
{

	KASSERT(se->s_refcnt > 0);
	se->s_flags |= SE_PASSSING;		/* XXXSMP */
	if (se->s_nat_se) {
		se = se->s_nat_se;
		se->s_flags |= SE_PASSSING;	/* XXXSMP */
	}
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
 * npf_session_link: create a link between regular and NAT sessions.
 * Note: NAT session inherits the flags, including "pass" bit.
 */
void
npf_session_link(npf_session_t *se, npf_session_t *natse)
{

	/* Hold a reference on the session we link.  Inherit the flags. */
	KASSERT(se->s_refcnt > 0 && natse->s_refcnt > 0);
	atomic_inc_uint(&natse->s_refcnt);
	natse->s_flags = se->s_flags;

	KASSERT(se->s_nat_se == NULL);
	se->s_nat_se = natse;
}

/*
 * npf_session_retnat: return associated NAT data entry and indicate
 * whether it is a "forwards" or "backwards" stream.
 */
npf_nat_t *
npf_session_retnat(npf_session_t *se, const int di, bool *forw)
{

	KASSERT(se->s_refcnt > 0);
	*forw = (se->s_direction == di);
	if (se->s_nat_se) {
		se = se->s_nat_se;
		KASSERT(se->s_refcnt > 0);
	}
	return se->s_nat;
}

/*
 * npf_session_expired: criterion to check if session is expired.
 */
static inline bool
npf_session_expired(const npf_session_t *se, const struct timespec *tsnow)
{
	struct timespec tsdiff;
	int etime = 0;

	switch (se->s_state) {
	case SE_ESTABLISHED:
		etime = sess_expire_table[se->s_type];
		break;
	case SE_OPENING:
	case SE_ACKNOWLEDGE:
	case SE_CLOSING:
		etime = 10;	/* XXX: figure out reasonable time */
		break;
	default:
		KASSERT(false);
	}
	timespecsub(tsnow, &se->s_atime, &tsdiff);
	return (tsdiff.tv_sec > etime);
}

/*
 * npf_session_gc: scan all sessions, insert into G/C list all expired ones.
 */
static void
npf_session_gc(struct npf_sesslist *gc_list, bool flushall)
{
	struct timespec tsnow;
	npf_session_t *se, *nse;
	u_int i;

	getnanouptime(&tsnow);

	/* Scan each session in the hash table. */
	for (i = 0; i < SESS_HASH_BUCKETS; i++) {
		npf_sess_hash_t *sh;

		sh = &sess_hashtbl[i];
		if (sh->sh_count == 0) {
			continue;
		}
		rw_enter(&sh->sh_lock, RW_WRITER);
		/* For each (left -> right) ... */
		se = rb_tree_iterate(&sh->sh_tree, NULL, RB_DIR_LEFT);
		while (se != NULL) {
			/* Get item, pre-iterate, skip if not expired. */
			nse = rb_tree_iterate(&sh->sh_tree, se, RB_DIR_RIGHT);
			if (!npf_session_expired(se, &tsnow) && !flushall) {
				se = nse;
				continue;
			}

			/* Expired - move to G/C list. */
			rb_tree_remove_node(&sh->sh_tree, se);
			LIST_INSERT_HEAD(gc_list, se, se_entry.gclist);
			sh->sh_count--;

			/* If linked, drop the reference. */
			DPRINTF(("NPF: se %p expired\n", se));
			if (se->s_nat_se) {
				npf_session_release(se->s_nat_se);
				DPRINTF(("NPF: se %p unlinked %p\n",
				    se, se->s_nat_se));
				se->s_nat_se = NULL;
			}
			se = nse;
		}
		KASSERT(!flushall || sh->sh_count == 0);
		rw_exit(&sh->sh_lock);
	}
}

/*
 * npf_session_free: destroy all sessions in the G/C list, which
 * have no references.  Return true, if list is empty.
 */
static void
npf_session_free(struct npf_sesslist *gc_list)
{
	npf_session_t *se, *nse;

	se = LIST_FIRST(gc_list);
	while (se != NULL) {
		nse = LIST_NEXT(se, se_entry.gclist);
		if (se->s_refcnt == 0) {
			/* Destroy only if no references. */
			LIST_REMOVE(se, se_entry.gclist);
			if (se->s_nat) {
				/* Release any NAT related structures. */
				npf_nat_expire(se->s_nat);
			}
			DPRINTF(("NPF: se %p destroyed\n", se));
			pool_cache_put(sess_cache, se);
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
		mutex_exit(&sess_lock);

		/* Flush all if session tracking got disabled. */
		npf_session_gc(&gc_list, flushreq);
		npf_session_free(&gc_list);

	} while (sess_tracking);

	/* Wait for any referenced sessions to be released. */
	while (!LIST_EMPTY(&gc_list)) {
		kpause("npfgcfr", false, 1, NULL);
		npf_session_free(&gc_list);
	}

	/* Notify that we are done. */
	mutex_enter(&sess_lock);
	sess_gc_lwp = NULL;
	cv_signal(&sess_cv);
	mutex_exit(&sess_lock);

	kthread_exit(0);
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_sessions_dump(void)
{
	npf_sess_hash_t *sh;
	npf_session_t *se;
	struct timespec tsnow;

	if (!sess_tracking) {
		return;
	}

	getnanouptime(&tsnow);
	for (u_int i = 0; i < SESS_HASH_BUCKETS; i++) {
		sh = &sess_hashtbl[i];
		if (sh->sh_count == 0) {
			KASSERT(rb_tree_iterate(&sh->sh_tree,
			    NULL, RB_DIR_RIGHT) == NULL);
			continue;
		}
		printf("s_bucket %d (count = %d)\n", i, sh->sh_count);
		RB_TREE_FOREACH(se, &sh->sh_tree) {
			struct timespec tsdiff;
			struct in_addr ip;
			int etime;

			timespecsub(&tsnow, &se->s_atime, &tsdiff);
			etime = (se->s_state == SE_ESTABLISHED) ?
			    sess_expire_table[se->s_type] : 10;

			printf("\t%p: type(%d) di %d, pass %d, tsdiff %d, "
			    "etime %d\n", se, se->s_type, se->s_direction,
			    se->s_flags, (int)tsdiff.tv_sec, etime);
			ip.s_addr = se->s_src_addr;
			printf("\tsrc (%s, %d) ",
			    inet_ntoa(ip), ntohs(se->s_src.port));
			ip.s_addr = se->s_dst_addr;
			printf("dst (%s, %d)\n", 
			    inet_ntoa(ip), ntohs(se->s_dst.port));
			if (se->s_nat_se != NULL) {
				printf("\tlinked with %p\n", se->s_nat_se);
			}
			if (se->s_nat != NULL) {
				npf_nat_dump(se->s_nat);
			}
		}
	}
}

#endif
