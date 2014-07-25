/*	$NetBSD: pf_ioctl.c,v 1.50 2014/07/25 08:10:39 dholland Exp $	*/
/*	$OpenBSD: pf_ioctl.c,v 1.182 2007/06/24 11:17:13 mcbride Exp $ */

/*
 * Copyright (c) 2001 Daniel Hartmeier
 * Copyright (c) 2002,2003 Henning Brauer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Effort sponsored in part by the Defense Advanced Research Projects
 * Agency (DARPA) and Air Force Research Laboratory, Air Force
 * Materiel Command, USAF, under agreement number F30602-01-2-0537.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pf_ioctl.c,v 1.50 2014/07/25 08:10:39 dholland Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#endif

#include "pfsync.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/filio.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/kthread.h>
#include <sys/rwlock.h>
#include <uvm/uvm_extern.h>
#ifdef __NetBSD__
#include <sys/conf.h>
#include <sys/lwp.h>
#include <sys/kauth.h>
#include <sys/module.h>
#include <sys/cprng.h>
#endif /* __NetBSD__ */

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>

#ifndef __NetBSD__
#include <dev/rndvar.h>
#include <crypto/md5.h>
#else
#include <sys/md5.h>
#endif /* __NetBSD__ */
#include <net/pfvar.h>

#if NPFSYNC > 0
#include <net/if_pfsync.h>
#endif /* NPFSYNC > 0 */

#if NPFLOG > 0
#include <net/if_pflog.h>
#endif /* NPFLOG > 0 */

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet/in_pcb.h>
#endif /* INET6 */

#ifdef ALTQ
#include <altq/altq.h>
#endif

void			 pfattach(int);
#ifdef _MODULE
void			 pfdetach(void);
#endif /* _MODULE */
#ifndef __NetBSD__
void			 pf_thread_create(void *);
#endif /* !__NetBSD__ */
int			 pfopen(dev_t, int, int, struct lwp *);
int			 pfclose(dev_t, int, int, struct lwp *);
struct pf_pool		*pf_get_pool(char *, u_int32_t, u_int8_t, u_int32_t,
			    u_int8_t, u_int8_t, u_int8_t);

void			 pf_mv_pool(struct pf_palist *, struct pf_palist *);
void			 pf_empty_pool(struct pf_palist *);
int			 pfioctl(dev_t, u_long, void *, int, struct lwp *);
#ifdef ALTQ
int			 pf_begin_altq(u_int32_t *);
int			 pf_rollback_altq(u_int32_t);
int			 pf_commit_altq(u_int32_t);
int			 pf_enable_altq(struct pf_altq *);
int			 pf_disable_altq(struct pf_altq *);
#endif /* ALTQ */
int			 pf_begin_rules(u_int32_t *, int, const char *);
int			 pf_rollback_rules(u_int32_t, int, char *);
int			 pf_setup_pfsync_matching(struct pf_ruleset *);
void			 pf_hash_rule(MD5_CTX *, struct pf_rule *);
void			 pf_hash_rule_addr(MD5_CTX *, struct pf_rule_addr *);
int			 pf_commit_rules(u_int32_t, int, char *);
void			 pf_state_export(struct pfsync_state *,
			    struct pf_state_key *, struct pf_state *);
void			 pf_state_import(struct pfsync_state *,
			    struct pf_state_key *, struct pf_state *);

static	int		pf_state_add(struct pfsync_state*);

struct pf_rule		 pf_default_rule;
#ifdef __NetBSD__
krwlock_t		 pf_consistency_lock;
#else
struct rwlock		 pf_consistency_lock = RWLOCK_INITIALIZER("pfcnslk");
#endif /* __NetBSD__ */
#ifdef ALTQ
static int		 pf_altq_running;
#endif

int		pf_state_lock = 0;

#define	TAGID_MAX	 50000
TAILQ_HEAD(pf_tags, pf_tagname)	pf_tags = TAILQ_HEAD_INITIALIZER(pf_tags),
				pf_qids = TAILQ_HEAD_INITIALIZER(pf_qids);

#if (PF_QNAME_SIZE != PF_TAG_NAME_SIZE)
#error PF_QNAME_SIZE must be equal to PF_TAG_NAME_SIZE
#endif
u_int16_t		 tagname2tag(struct pf_tags *, char *);
void			 tag2tagname(struct pf_tags *, u_int16_t, char *);
void			 tag_unref(struct pf_tags *, u_int16_t);
int			 pf_rtlabel_add(struct pf_addr_wrap *);
void			 pf_rtlabel_remove(struct pf_addr_wrap *);
void			 pf_rtlabel_copyout(struct pf_addr_wrap *);

#define DPFPRINTF(n, x) if (pf_status.debug >= (n)) printf x

#ifdef __NetBSD__
const struct cdevsw pf_cdevsw = {
	.d_open = pfopen,
	.d_close = pfclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = pfioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static int pfil4_wrapper(void *, struct mbuf **, struct ifnet *, int);
#ifdef INET6
static int pfil6_wrapper(void *, struct mbuf **, struct ifnet *, int);
#endif /* INET6 */

static int pf_pfil_attach(void);
static int pf_pfil_detach(void);

static int pf_pfil_attached;

static kauth_listener_t pf_listener;
#endif /* __NetBSD__ */

#ifdef __NetBSD__
static int
pf_listener_cb(kauth_cred_t cred, kauth_action_t action, void *cookie,
    void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;
	enum kauth_network_req req;

	result = KAUTH_RESULT_DEFER;
	req = (enum kauth_network_req)arg0;

	if (action != KAUTH_NETWORK_FIREWALL)
		return result;

	/* These must have came from device context. */
	if ((req == KAUTH_REQ_NETWORK_FIREWALL_FW) ||
	    (req == KAUTH_REQ_NETWORK_FIREWALL_NAT))
		result = KAUTH_RESULT_ALLOW;

	return result;
}
#endif /* __NetBSD__ */

void
pfattach(int num)
{
	u_int32_t *timeout = pf_default_rule.timeout;

#ifdef __NetBSD__
	pool_init(&pf_rule_pl, sizeof(struct pf_rule), 0, 0, 0, "pfrulepl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&pf_src_tree_pl, sizeof(struct pf_src_node), 0, 0, 0,
	    "pfsrctrpl", NULL, IPL_SOFTNET);
	pool_init(&pf_state_pl, sizeof(struct pf_state), 0, 0, 0, "pfstatepl",
	    NULL, IPL_SOFTNET);
	pool_init(&pf_state_key_pl, sizeof(struct pf_state_key), 0, 0, 0,
	    "pfstatekeypl", NULL, IPL_SOFTNET);
	pool_init(&pf_altq_pl, sizeof(struct pf_altq), 0, 0, 0, "pfaltqpl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&pf_pooladdr_pl, sizeof(struct pf_pooladdr), 0, 0, 0,
	    "pfpooladdrpl", &pool_allocator_nointr, IPL_NONE);
#else
	pool_init(&pf_rule_pl, sizeof(struct pf_rule), 0, 0, 0, "pfrulepl",
	    &pool_allocator_nointr);
	pool_init(&pf_src_tree_pl, sizeof(struct pf_src_node), 0, 0, 0,
	    "pfsrctrpl", NULL);
	pool_init(&pf_state_pl, sizeof(struct pf_state), 0, 0, 0, "pfstatepl",
	    NULL);
	pool_init(&pf_state_key_pl, sizeof(struct pf_state_key), 0, 0, 0, 
	    "pfstatekeypl", NULL);
	pool_init(&pf_altq_pl, sizeof(struct pf_altq), 0, 0, 0, "pfaltqpl",
	    &pool_allocator_nointr);
	pool_init(&pf_pooladdr_pl, sizeof(struct pf_pooladdr), 0, 0, 0,
	    "pfpooladdrpl", &pool_allocator_nointr);
#endif /* !__NetBSD__ */

	pfr_initialize();
	pfi_initialize();
	pf_osfp_initialize();

	pool_sethardlimit(pf_pool_limits[PF_LIMIT_STATES].pp,
	    pf_pool_limits[PF_LIMIT_STATES].limit, NULL, 0);

	if (ctob(physmem) <= 100*1024*1024)
		pf_pool_limits[PF_LIMIT_TABLE_ENTRIES].limit =
		    PFR_KENTRY_HIWAT_SMALL;

	RB_INIT(&tree_src_tracking);
	RB_INIT(&pf_anchors);
	pf_init_ruleset(&pf_main_ruleset);
	TAILQ_INIT(&pf_altqs[0]);
	TAILQ_INIT(&pf_altqs[1]);
	TAILQ_INIT(&pf_pabuf);
	pf_altqs_active = &pf_altqs[0];
	pf_altqs_inactive = &pf_altqs[1];
	TAILQ_INIT(&state_list);

#ifdef __NetBSD__
	rw_init(&pf_consistency_lock);
#endif /* __NetBSD__ */

	/* default rule should never be garbage collected */
	pf_default_rule.entries.tqe_prev = &pf_default_rule.entries.tqe_next;
	pf_default_rule.action = PF_PASS;
	pf_default_rule.nr = -1;
	pf_default_rule.rtableid = -1;

	/* initialize default timeouts */
	timeout[PFTM_TCP_FIRST_PACKET] = PFTM_TCP_FIRST_PACKET_VAL;
	timeout[PFTM_TCP_OPENING] = PFTM_TCP_OPENING_VAL;
	timeout[PFTM_TCP_ESTABLISHED] = PFTM_TCP_ESTABLISHED_VAL;
	timeout[PFTM_TCP_CLOSING] = PFTM_TCP_CLOSING_VAL;
	timeout[PFTM_TCP_FIN_WAIT] = PFTM_TCP_FIN_WAIT_VAL;
	timeout[PFTM_TCP_CLOSED] = PFTM_TCP_CLOSED_VAL;
	timeout[PFTM_UDP_FIRST_PACKET] = PFTM_UDP_FIRST_PACKET_VAL;
	timeout[PFTM_UDP_SINGLE] = PFTM_UDP_SINGLE_VAL;
	timeout[PFTM_UDP_MULTIPLE] = PFTM_UDP_MULTIPLE_VAL;
	timeout[PFTM_ICMP_FIRST_PACKET] = PFTM_ICMP_FIRST_PACKET_VAL;
	timeout[PFTM_ICMP_ERROR_REPLY] = PFTM_ICMP_ERROR_REPLY_VAL;
	timeout[PFTM_OTHER_FIRST_PACKET] = PFTM_OTHER_FIRST_PACKET_VAL;
	timeout[PFTM_OTHER_SINGLE] = PFTM_OTHER_SINGLE_VAL;
	timeout[PFTM_OTHER_MULTIPLE] = PFTM_OTHER_MULTIPLE_VAL;
	timeout[PFTM_FRAG] = PFTM_FRAG_VAL;
	timeout[PFTM_INTERVAL] = PFTM_INTERVAL_VAL;
	timeout[PFTM_SRC_NODE] = PFTM_SRC_NODE_VAL;
	timeout[PFTM_TS_DIFF] = PFTM_TS_DIFF_VAL;
	timeout[PFTM_ADAPTIVE_START] = PFSTATE_ADAPT_START;
	timeout[PFTM_ADAPTIVE_END] = PFSTATE_ADAPT_END;

	pf_normalize_init();
	bzero(&pf_status, sizeof(pf_status));
	pf_status.debug = PF_DEBUG_URGENT;

	/* XXX do our best to avoid a conflict */
	pf_status.hostid = cprng_fast32();

	/* require process context to purge states, so perform in a thread */
#ifdef __NetBSD__
	if (kthread_create(PRI_NONE, 0, NULL, pf_purge_thread, NULL, NULL,
	    "pfpurge"))
		panic("pfpurge thread");
#else
	kthread_create_deferred(pf_thread_create, NULL);
#endif /* !__NetBSD__ */

#ifdef __NetBSD__
	pf_listener = kauth_listen_scope(KAUTH_SCOPE_NETWORK,
	    pf_listener_cb, NULL);
#endif /* __NetBSD__ */
}

#ifdef _MODULE
void
pfdetach(void)
{
	extern int		 pf_purge_thread_running;
	extern int		 pf_purge_thread_stop;
	struct pf_anchor	*anchor;
	struct pf_state		*state;
	struct pf_src_node	*node;
	struct pfioc_table	 pt;
	u_int32_t		 ticket;
	int			 i;
	char			 r = '\0';

	pf_purge_thread_stop = 1;
	wakeup(pf_purge_thread);

	/* wait until the kthread exits */
	while (pf_purge_thread_running)
		tsleep(&pf_purge_thread_running, PWAIT, "pfdown", 0);

	(void)pf_pfil_detach();

	pf_status.running = 0;

	/* clear the rulesets */
	for (i = 0; i < PF_RULESET_MAX; i++)
		if (pf_begin_rules(&ticket, i, &r) == 0)
			pf_commit_rules(ticket, i, &r);
#ifdef ALTQ
	if (pf_begin_altq(&ticket) == 0)
		pf_commit_altq(ticket);
#endif /* ALTQ */

	/* clear states */
	RB_FOREACH(state, pf_state_tree_id, &tree_id) {
		state->timeout = PFTM_PURGE;
#if NPFSYNC > 0
		state->sync_flags = PFSTATE_NOSYNC;
#endif /* NPFSYNC > 0 */
	}
	pf_purge_expired_states(pf_status.states);
#if NPFSYNC > 0
	pfsync_clear_states(pf_status.hostid, NULL);
#endif /* NPFSYNC > 0 */

	/* clear source nodes */
	RB_FOREACH(state, pf_state_tree_id, &tree_id) {
		state->src_node = NULL;
		state->nat_src_node = NULL;
	}
	RB_FOREACH(node, pf_src_tree, &tree_src_tracking) {
		node->expire = 1;
		node->states = 0;
	}
	pf_purge_expired_src_nodes(0);

	/* clear tables */
	memset(&pt, '\0', sizeof(pt));
	pfr_clr_tables(&pt.pfrio_table, &pt.pfrio_ndel, pt.pfrio_flags);

	/* destroy anchors */
	while ((anchor = RB_MIN(pf_anchor_global, &pf_anchors)) != NULL) {
		for (i = 0; i < PF_RULESET_MAX; i++)
			if (pf_begin_rules(&ticket, i, anchor->name) == 0)
				pf_commit_rules(ticket, i, anchor->name);
	}

	/* destroy main ruleset */
	pf_remove_if_empty_ruleset(&pf_main_ruleset);

	/* destroy the pools */
	pool_destroy(&pf_pooladdr_pl);
	pool_destroy(&pf_altq_pl);
	pool_destroy(&pf_state_key_pl);
	pool_destroy(&pf_state_pl);
	pool_destroy(&pf_rule_pl);
	pool_destroy(&pf_src_tree_pl);

	rw_destroy(&pf_consistency_lock);

	/* destroy subsystems */
	pf_normalize_destroy();
	pf_osfp_destroy();
	pfr_destroy();
	pfi_destroy();

	/* cleanup kauth listener */
	kauth_unlisten_scope(pf_listener);
}
#endif /* _MODULE */

#ifndef __NetBSD__
void
pf_thread_create(void *v)
{
	if (kthread_create(pf_purge_thread, NULL, NULL, "pfpurge"))
		panic("pfpurge thread");
}
#endif /* !__NetBSD__ */

int
pfopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	if (minor(dev) >= 1)
		return (ENXIO);
	return (0);
}

int
pfclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	if (minor(dev) >= 1)
		return (ENXIO);
	return (0);
}

struct pf_pool *
pf_get_pool(char *anchor, u_int32_t ticket, u_int8_t rule_action,
    u_int32_t rule_number, u_int8_t r_last, u_int8_t active,
    u_int8_t check_ticket)
{
	struct pf_ruleset	*ruleset;
	struct pf_rule		*rule;
	int			 rs_num;

	ruleset = pf_find_ruleset(anchor);
	if (ruleset == NULL)
		return (NULL);
	rs_num = pf_get_ruleset_number(rule_action);
	if (rs_num >= PF_RULESET_MAX)
		return (NULL);
	if (active) {
		if (check_ticket && ticket !=
		    ruleset->rules[rs_num].active.ticket)
			return (NULL);
		if (r_last)
			rule = TAILQ_LAST(ruleset->rules[rs_num].active.ptr,
			    pf_rulequeue);
		else
			rule = TAILQ_FIRST(ruleset->rules[rs_num].active.ptr);
	} else {
		if (check_ticket && ticket !=
		    ruleset->rules[rs_num].inactive.ticket)
			return (NULL);
		if (r_last)
			rule = TAILQ_LAST(ruleset->rules[rs_num].inactive.ptr,
			    pf_rulequeue);
		else
			rule = TAILQ_FIRST(ruleset->rules[rs_num].inactive.ptr);
	}
	if (!r_last) {
		while ((rule != NULL) && (rule->nr != rule_number))
			rule = TAILQ_NEXT(rule, entries);
	}
	if (rule == NULL)
		return (NULL);

	return (&rule->rpool);
}

void
pf_mv_pool(struct pf_palist *poola, struct pf_palist *poolb)
{
	struct pf_pooladdr	*mv_pool_pa;

	while ((mv_pool_pa = TAILQ_FIRST(poola)) != NULL) {
		TAILQ_REMOVE(poola, mv_pool_pa, entries);
		TAILQ_INSERT_TAIL(poolb, mv_pool_pa, entries);
	}
}

void
pf_empty_pool(struct pf_palist *poola)
{
	struct pf_pooladdr	*empty_pool_pa;

	while ((empty_pool_pa = TAILQ_FIRST(poola)) != NULL) {
		pfi_dynaddr_remove(&empty_pool_pa->addr);
		pf_tbladdr_remove(&empty_pool_pa->addr);
		pfi_kif_unref(empty_pool_pa->kif, PFI_KIF_REF_RULE);
		TAILQ_REMOVE(poola, empty_pool_pa, entries);
		pool_put(&pf_pooladdr_pl, empty_pool_pa);
	}
}

void
pf_rm_rule(struct pf_rulequeue *rulequeue, struct pf_rule *rule)
{
	if (rulequeue != NULL) {
		if (rule->states <= 0) {
			/*
			 * XXX - we need to remove the table *before* detaching
			 * the rule to make sure the table code does not delete
			 * the anchor under our feet.
			 */
			pf_tbladdr_remove(&rule->src.addr);
			pf_tbladdr_remove(&rule->dst.addr);
			if (rule->overload_tbl)
				pfr_detach_table(rule->overload_tbl);
		}
		TAILQ_REMOVE(rulequeue, rule, entries);
		rule->entries.tqe_prev = NULL;
		rule->nr = -1;
	}

	if (rule->states > 0 || rule->src_nodes > 0 ||
	    rule->entries.tqe_prev != NULL)
		return;
	pf_tag_unref(rule->tag);
	pf_tag_unref(rule->match_tag);
#ifdef ALTQ
	if (rule->pqid != rule->qid)
		pf_qid_unref(rule->pqid);
	pf_qid_unref(rule->qid);
#endif
	pf_rtlabel_remove(&rule->src.addr);
	pf_rtlabel_remove(&rule->dst.addr);
	pfi_dynaddr_remove(&rule->src.addr);
	pfi_dynaddr_remove(&rule->dst.addr);
	if (rulequeue == NULL) {
		pf_tbladdr_remove(&rule->src.addr);
		pf_tbladdr_remove(&rule->dst.addr);
		if (rule->overload_tbl)
			pfr_detach_table(rule->overload_tbl);
	}
	pfi_kif_unref(rule->kif, PFI_KIF_REF_RULE);
	pf_anchor_remove(rule);
	pf_empty_pool(&rule->rpool.list);
	pool_put(&pf_rule_pl, rule);
}

u_int16_t
tagname2tag(struct pf_tags *head, char *tagname)
{
	struct pf_tagname	*tag, *p = NULL;
	u_int16_t		 new_tagid = 1;

	TAILQ_FOREACH(tag, head, entries)
		if (strcmp(tagname, tag->name) == 0) {
			tag->ref++;
			return (tag->tag);
		}

	/*
	 * to avoid fragmentation, we do a linear search from the beginning
	 * and take the first free slot we find. if there is none or the list
	 * is empty, append a new entry at the end.
	 */

	/* new entry */
	if (!TAILQ_EMPTY(head))
		for (p = TAILQ_FIRST(head); p != NULL &&
		    p->tag == new_tagid; p = TAILQ_NEXT(p, entries))
			new_tagid = p->tag + 1;

	if (new_tagid > TAGID_MAX)
		return (0);

	/* allocate and fill new struct pf_tagname */
	tag = (struct pf_tagname *)malloc(sizeof(struct pf_tagname),
	    M_TEMP, M_NOWAIT);
	if (tag == NULL)
		return (0);
	bzero(tag, sizeof(struct pf_tagname));
	strlcpy(tag->name, tagname, sizeof(tag->name));
	tag->tag = new_tagid;
	tag->ref++;

	if (p != NULL)	/* insert new entry before p */
		TAILQ_INSERT_BEFORE(p, tag, entries);
	else	/* either list empty or no free slot in between */
		TAILQ_INSERT_TAIL(head, tag, entries);

	return (tag->tag);
}

void
tag2tagname(struct pf_tags *head, u_int16_t tagid, char *p)
{
	struct pf_tagname	*tag;

	TAILQ_FOREACH(tag, head, entries)
		if (tag->tag == tagid) {
			strlcpy(p, tag->name, PF_TAG_NAME_SIZE);
			return;
		}
}

void
tag_unref(struct pf_tags *head, u_int16_t tag)
{
	struct pf_tagname	*p, *next;

	if (tag == 0)
		return;

	for (p = TAILQ_FIRST(head); p != NULL; p = next) {
		next = TAILQ_NEXT(p, entries);
		if (tag == p->tag) {
			if (--p->ref == 0) {
				TAILQ_REMOVE(head, p, entries);
				free(p, M_TEMP);
			}
			break;
		}
	}
}

u_int16_t
pf_tagname2tag(char *tagname)
{
	return (tagname2tag(&pf_tags, tagname));
}

void
pf_tag2tagname(u_int16_t tagid, char *p)
{
	tag2tagname(&pf_tags, tagid, p);
}

void
pf_tag_ref(u_int16_t tag)
{
	struct pf_tagname *t;

	TAILQ_FOREACH(t, &pf_tags, entries)
		if (t->tag == tag)
			break;
	if (t != NULL)
		t->ref++;
}

void
pf_tag_unref(u_int16_t tag)
{
	tag_unref(&pf_tags, tag);
}

int
pf_rtlabel_add(struct pf_addr_wrap *a)
{
#ifndef __NetBSD__
	if (a->type == PF_ADDR_RTLABEL &&
	    (a->v.rtlabel = rtlabel_name2id(a->v.rtlabelname)) == 0)
		return (-1);
#endif /* !__NetBSD__ */
	return (0);
}

void
pf_rtlabel_remove(struct pf_addr_wrap *a)
{
#ifndef __NetBSD__
	if (a->type == PF_ADDR_RTLABEL)
		rtlabel_unref(a->v.rtlabel);
#endif /* !__NetBSD__ */
}

void
pf_rtlabel_copyout(struct pf_addr_wrap *a)
{
#ifndef __NetBSD__
	const char	*name;

	if (a->type == PF_ADDR_RTLABEL && a->v.rtlabel) {
		if ((name = rtlabel_id2name(a->v.rtlabel)) == NULL)
			strlcpy(a->v.rtlabelname, "?",
			    sizeof(a->v.rtlabelname));
		else
			strlcpy(a->v.rtlabelname, name,
			    sizeof(a->v.rtlabelname));
	}
#endif /* !__NetBSD__ */
}

#ifdef ALTQ
u_int32_t
pf_qname2qid(char *qname)
{
	return ((u_int32_t)tagname2tag(&pf_qids, qname));
}

void
pf_qid2qname(u_int32_t qid, char *p)
{
	tag2tagname(&pf_qids, (u_int16_t)qid, p);
}

void
pf_qid_unref(u_int32_t qid)
{
	tag_unref(&pf_qids, (u_int16_t)qid);
}

int
pf_begin_altq(u_int32_t *ticket)
{
	struct pf_altq	*altq;
	int		 error = 0;

	/* Purge the old altq list */
	while ((altq = TAILQ_FIRST(pf_altqs_inactive)) != NULL) {
		TAILQ_REMOVE(pf_altqs_inactive, altq, entries);
		if (altq->qname[0] == 0) {
			/* detach and destroy the discipline */
			error = altq_remove(altq);
		} else
			pf_qid_unref(altq->qid);
		pool_put(&pf_altq_pl, altq);
	}
	if (error)
		return (error);
	*ticket = ++ticket_altqs_inactive;
	altqs_inactive_open = 1;
	return (0);
}

int
pf_rollback_altq(u_int32_t ticket)
{
	struct pf_altq	*altq;
	int		 error = 0;

	if (!altqs_inactive_open || ticket != ticket_altqs_inactive)
		return (0);
	/* Purge the old altq list */
	while ((altq = TAILQ_FIRST(pf_altqs_inactive)) != NULL) {
		TAILQ_REMOVE(pf_altqs_inactive, altq, entries);
		if (altq->qname[0] == 0) {
			/* detach and destroy the discipline */
			error = altq_remove(altq);
		} else
			pf_qid_unref(altq->qid);
		pool_put(&pf_altq_pl, altq);
	}
	altqs_inactive_open = 0;
	return (error);
}

int
pf_commit_altq(u_int32_t ticket)
{
	struct pf_altqqueue	*old_altqs;
	struct pf_altq		*altq;
	int			 s, err, error = 0;

	if (!altqs_inactive_open || ticket != ticket_altqs_inactive)
		return (EBUSY);

	/* swap altqs, keep the old. */
	s = splsoftnet();
	old_altqs = pf_altqs_active;
	pf_altqs_active = pf_altqs_inactive;
	pf_altqs_inactive = old_altqs;
	ticket_altqs_active = ticket_altqs_inactive;

	/* Attach new disciplines */
	TAILQ_FOREACH(altq, pf_altqs_active, entries) {
		if (altq->qname[0] == 0) {
			/* attach the discipline */
			error = altq_pfattach(altq);
			if (error == 0 && pf_altq_running)
				error = pf_enable_altq(altq);
			if (error != 0) {
				splx(s);
				return (error);
			}
		}
	}

	/* Purge the old altq list */
	while ((altq = TAILQ_FIRST(pf_altqs_inactive)) != NULL) {
		TAILQ_REMOVE(pf_altqs_inactive, altq, entries);
		if (altq->qname[0] == 0) {
			/* detach and destroy the discipline */
			if (pf_altq_running)
				error = pf_disable_altq(altq);
			err = altq_pfdetach(altq);
			if (err != 0 && error == 0)
				error = err;
			err = altq_remove(altq);
			if (err != 0 && error == 0)
				error = err;
		} else
			pf_qid_unref(altq->qid);
		pool_put(&pf_altq_pl, altq);
	}
	splx(s);

	altqs_inactive_open = 0;
	return (error);
}

int
pf_enable_altq(struct pf_altq *altq)
{
	struct ifnet		*ifp;
	struct tb_profile	 tb;
	int			 s, error = 0;

	if ((ifp = ifunit(altq->ifname)) == NULL)
		return (EINVAL);

	if (ifp->if_snd.altq_type != ALTQT_NONE)
		error = altq_enable(&ifp->if_snd);

	/* set tokenbucket regulator */
	if (error == 0 && ifp != NULL && ALTQ_IS_ENABLED(&ifp->if_snd)) {
		tb.rate = altq->ifbandwidth;
		tb.depth = altq->tbrsize;
		s = splnet();
		error = tbr_set(&ifp->if_snd, &tb);
		splx(s);
	}

	return (error);
}

int
pf_disable_altq(struct pf_altq *altq)
{
	struct ifnet		*ifp;
	struct tb_profile	 tb;
	int			 s, error;

	if ((ifp = ifunit(altq->ifname)) == NULL)
		return (EINVAL);

	/*
	 * when the discipline is no longer referenced, it was overridden
	 * by a new one.  if so, just return.
	 */
	if (altq->altq_disc != ifp->if_snd.altq_disc)
		return (0);

	error = altq_disable(&ifp->if_snd);

	if (error == 0) {
		/* clear tokenbucket regulator */
		tb.rate = 0;
		s = splnet();
		error = tbr_set(&ifp->if_snd, &tb);
		splx(s);
	}

	return (error);
}
#endif /* ALTQ */

int
pf_begin_rules(u_int32_t *ticket, int rs_num, const char *anchor)
{
	struct pf_ruleset	*rs;
	struct pf_rule		*rule;

	if (rs_num < 0 || rs_num >= PF_RULESET_MAX)
		return (EINVAL);
	rs = pf_find_or_create_ruleset(anchor);
	if (rs == NULL)
		return (EINVAL);
	while ((rule = TAILQ_FIRST(rs->rules[rs_num].inactive.ptr)) != NULL) {
		pf_rm_rule(rs->rules[rs_num].inactive.ptr, rule);
		rs->rules[rs_num].inactive.rcount--;
	}
	*ticket = ++rs->rules[rs_num].inactive.ticket;
	rs->rules[rs_num].inactive.open = 1;
	return (0);
}

int
pf_rollback_rules(u_int32_t ticket, int rs_num, char *anchor)
{
	struct pf_ruleset	*rs;
	struct pf_rule		*rule;

	if (rs_num < 0 || rs_num >= PF_RULESET_MAX)
		return (EINVAL);
	rs = pf_find_ruleset(anchor);
	if (rs == NULL || !rs->rules[rs_num].inactive.open ||
	    rs->rules[rs_num].inactive.ticket != ticket)
		return (0);
	while ((rule = TAILQ_FIRST(rs->rules[rs_num].inactive.ptr)) != NULL) {
		pf_rm_rule(rs->rules[rs_num].inactive.ptr, rule);
		rs->rules[rs_num].inactive.rcount--;
	}
	rs->rules[rs_num].inactive.open = 0;
	return (0);
}

#define PF_MD5_UPD(st, elm)						\
		MD5Update(ctx, (u_int8_t *) &(st)->elm, sizeof((st)->elm))

#define PF_MD5_UPD_STR(st, elm)						\
		MD5Update(ctx, (u_int8_t *) (st)->elm, strlen((st)->elm))

#define PF_MD5_UPD_HTONL(st, elm, stor) do {				\
		(stor) = htonl((st)->elm);				\
		MD5Update(ctx, (u_int8_t *) &(stor), sizeof(u_int32_t));\
} while (0)

#define PF_MD5_UPD_HTONS(st, elm, stor) do {				\
		(stor) = htons((st)->elm);				\
		MD5Update(ctx, (u_int8_t *) &(stor), sizeof(u_int16_t));\
} while (0)

void
pf_hash_rule_addr(MD5_CTX *ctx, struct pf_rule_addr *pfr)
{
	PF_MD5_UPD(pfr, addr.type);
	switch (pfr->addr.type) {
		case PF_ADDR_DYNIFTL:
			PF_MD5_UPD(pfr, addr.v.ifname);
			PF_MD5_UPD(pfr, addr.iflags);
			break;
		case PF_ADDR_TABLE:
			PF_MD5_UPD(pfr, addr.v.tblname);
			break;
		case PF_ADDR_ADDRMASK:
			/* XXX ignore af? */
			PF_MD5_UPD(pfr, addr.v.a.addr.addr32);
			PF_MD5_UPD(pfr, addr.v.a.mask.addr32);
			break;
		case PF_ADDR_RTLABEL:
			PF_MD5_UPD(pfr, addr.v.rtlabelname);
			break;
	}

	PF_MD5_UPD(pfr, port[0]);
	PF_MD5_UPD(pfr, port[1]);
	PF_MD5_UPD(pfr, neg);
	PF_MD5_UPD(pfr, port_op);
}

void
pf_hash_rule(MD5_CTX *ctx, struct pf_rule *rule)
{
	u_int16_t x;
	u_int32_t y;

	pf_hash_rule_addr(ctx, &rule->src);
	pf_hash_rule_addr(ctx, &rule->dst);
	PF_MD5_UPD_STR(rule, label);
	PF_MD5_UPD_STR(rule, ifname);
	PF_MD5_UPD_STR(rule, match_tagname);
	PF_MD5_UPD_HTONS(rule, match_tag, x); /* dup? */
	PF_MD5_UPD_HTONL(rule, os_fingerprint, y);
	PF_MD5_UPD_HTONL(rule, prob, y);
	PF_MD5_UPD_HTONL(rule, uid.uid[0], y);
	PF_MD5_UPD_HTONL(rule, uid.uid[1], y);
	PF_MD5_UPD(rule, uid.op);
	PF_MD5_UPD_HTONL(rule, gid.gid[0], y);
	PF_MD5_UPD_HTONL(rule, gid.gid[1], y);
	PF_MD5_UPD(rule, gid.op);
	PF_MD5_UPD_HTONL(rule, rule_flag, y);
	PF_MD5_UPD(rule, action);
	PF_MD5_UPD(rule, direction);
	PF_MD5_UPD(rule, af);
	PF_MD5_UPD(rule, quick);
	PF_MD5_UPD(rule, ifnot);
	PF_MD5_UPD(rule, match_tag_not);
	PF_MD5_UPD(rule, natpass);
	PF_MD5_UPD(rule, keep_state);
	PF_MD5_UPD(rule, proto);
	PF_MD5_UPD(rule, type);
	PF_MD5_UPD(rule, code);
	PF_MD5_UPD(rule, flags);
	PF_MD5_UPD(rule, flagset);
	PF_MD5_UPD(rule, allow_opts);
	PF_MD5_UPD(rule, rt);
	PF_MD5_UPD(rule, tos);
}

int
pf_commit_rules(u_int32_t ticket, int rs_num, char *anchor)
{
	struct pf_ruleset	*rs;
	struct pf_rule		*rule, **old_array;
	struct pf_rulequeue	*old_rules;
	int			 s, error;
	u_int32_t		 old_rcount;

	if (rs_num < 0 || rs_num >= PF_RULESET_MAX)
		return (EINVAL);
	rs = pf_find_ruleset(anchor);
	if (rs == NULL || !rs->rules[rs_num].inactive.open ||
	    ticket != rs->rules[rs_num].inactive.ticket)
		return (EBUSY);

	/* Calculate checksum for the main ruleset */
	if (rs == &pf_main_ruleset) {
		error = pf_setup_pfsync_matching(rs);
		if (error != 0)
			return (error);
	}

	/* Swap rules, keep the old. */
	s = splsoftnet();
	old_rules = rs->rules[rs_num].active.ptr;
	old_rcount = rs->rules[rs_num].active.rcount;
	old_array = rs->rules[rs_num].active.ptr_array;

	rs->rules[rs_num].active.ptr =
	    rs->rules[rs_num].inactive.ptr;
	rs->rules[rs_num].active.ptr_array =
	    rs->rules[rs_num].inactive.ptr_array;
	rs->rules[rs_num].active.rcount =
	    rs->rules[rs_num].inactive.rcount;
	rs->rules[rs_num].inactive.ptr = old_rules;
	rs->rules[rs_num].inactive.ptr_array = old_array;
	rs->rules[rs_num].inactive.rcount = old_rcount;

	rs->rules[rs_num].active.ticket =
	    rs->rules[rs_num].inactive.ticket;
	pf_calc_skip_steps(rs->rules[rs_num].active.ptr);


	/* Purge the old rule list. */
	while ((rule = TAILQ_FIRST(old_rules)) != NULL)
		pf_rm_rule(old_rules, rule);
	if (rs->rules[rs_num].inactive.ptr_array)
		free(rs->rules[rs_num].inactive.ptr_array, M_TEMP);
	rs->rules[rs_num].inactive.ptr_array = NULL;
	rs->rules[rs_num].inactive.rcount = 0;
	rs->rules[rs_num].inactive.open = 0;
	pf_remove_if_empty_ruleset(rs);
	splx(s);
	return (0);
}

void
pf_state_export(struct pfsync_state *sp, struct pf_state_key *sk,
   struct pf_state *s) 
{
	int secs = time_second;
	bzero(sp, sizeof(struct pfsync_state));

	/* copy from state key */
	sp->lan.addr = sk->lan.addr;
	sp->lan.port = sk->lan.port;
	sp->gwy.addr = sk->gwy.addr;
	sp->gwy.port = sk->gwy.port;
	sp->ext.addr = sk->ext.addr;
	sp->ext.port = sk->ext.port;
	sp->proto = sk->proto;
	sp->af = sk->af;
	sp->direction = sk->direction;

	/* copy from state */
	memcpy(&sp->id, &s->id, sizeof(sp->id));
	sp->creatorid = s->creatorid;
	strlcpy(sp->ifname, s->kif->pfik_name, sizeof(sp->ifname));
	pf_state_peer_to_pfsync(&s->src, &sp->src);
	pf_state_peer_to_pfsync(&s->dst, &sp->dst);

	sp->rule = s->rule.ptr->nr;
	sp->nat_rule = (s->nat_rule.ptr == NULL) ?  -1 : s->nat_rule.ptr->nr;
	sp->anchor = (s->anchor.ptr == NULL) ?  -1 : s->anchor.ptr->nr;

	pf_state_counter_to_pfsync(s->bytes[0], sp->bytes[0]);
	pf_state_counter_to_pfsync(s->bytes[1], sp->bytes[1]);
	pf_state_counter_to_pfsync(s->packets[0], sp->packets[0]);
	pf_state_counter_to_pfsync(s->packets[1], sp->packets[1]);
	sp->creation = secs - s->creation;
	sp->expire = pf_state_expires(s);
	sp->log = s->log;
	sp->allow_opts = s->allow_opts;
	sp->timeout = s->timeout;

	if (s->src_node)
		sp->sync_flags |= PFSYNC_FLAG_SRCNODE;
	if (s->nat_src_node)
		sp->sync_flags |= PFSYNC_FLAG_NATSRCNODE;

	if (sp->expire > secs)
		sp->expire -= secs;
	else
		sp->expire = 0;

}

void
pf_state_import(struct pfsync_state *sp, struct pf_state_key *sk,
   struct pf_state *s) 
{
	/* copy to state key */
	sk->lan.addr = sp->lan.addr;
	sk->lan.port = sp->lan.port;
	sk->gwy.addr = sp->gwy.addr;
	sk->gwy.port = sp->gwy.port;
	sk->ext.addr = sp->ext.addr;
	sk->ext.port = sp->ext.port;
	sk->proto = sp->proto;
	sk->af = sp->af;
	sk->direction = sp->direction;

	/* copy to state */
	memcpy(&s->id, &sp->id, sizeof(sp->id));
	s->creatorid = sp->creatorid;
	pf_state_peer_from_pfsync(&sp->src, &s->src);
	pf_state_peer_from_pfsync(&sp->dst, &s->dst);

	s->rule.ptr = &pf_default_rule;
	s->rule.ptr->states++;
	s->nat_rule.ptr = NULL;
	s->anchor.ptr = NULL;
	s->rt_kif = NULL;
	s->creation = time_second;
	s->expire = time_second;
	s->timeout = sp->timeout;
	if (sp->expire > 0)
		s->expire -= pf_default_rule.timeout[sp->timeout] - sp->expire;
	s->pfsync_time = 0;
	s->packets[0] = s->packets[1] = 0;
	s->bytes[0] = s->bytes[1] = 0;
}

int
pf_state_add(struct pfsync_state* sp)
{
	struct pf_state		*s;
	struct pf_state_key	*sk;
	struct pfi_kif		*kif;

	if (sp->timeout >= PFTM_MAX &&
			sp->timeout != PFTM_UNTIL_PACKET) {
		return EINVAL;
	}
	s = pool_get(&pf_state_pl, PR_NOWAIT);
	if (s == NULL) {
		return ENOMEM;
	}
	bzero(s, sizeof(struct pf_state));
	if ((sk = pf_alloc_state_key(s)) == NULL) {
		pool_put(&pf_state_pl, s);
		return ENOMEM;
	}
	pf_state_import(sp, sk, s);
	kif = pfi_kif_get(sp->ifname);
	if (kif == NULL) {
		pool_put(&pf_state_pl, s);
		pool_put(&pf_state_key_pl, sk);
		return ENOENT;
	}
	if (pf_insert_state(kif, s)) {
		pfi_kif_unref(kif, PFI_KIF_REF_NONE);
		pool_put(&pf_state_pl, s);
		return ENOMEM;
	}

	return 0;
}


int
pf_setup_pfsync_matching(struct pf_ruleset *rs)
{
	MD5_CTX			 ctx;
	struct pf_rule		*rule;
	int			 rs_cnt;
	u_int8_t		 digest[PF_MD5_DIGEST_LENGTH];

	MD5Init(&ctx);
	for (rs_cnt = 0; rs_cnt < PF_RULESET_MAX; rs_cnt++) {
		/* XXX PF_RULESET_SCRUB as well? */
		if (rs_cnt == PF_RULESET_SCRUB)
			continue;

		if (rs->rules[rs_cnt].inactive.ptr_array)
			free(rs->rules[rs_cnt].inactive.ptr_array, M_TEMP);
		rs->rules[rs_cnt].inactive.ptr_array = NULL;

		if (rs->rules[rs_cnt].inactive.rcount) {
			rs->rules[rs_cnt].inactive.ptr_array =
			    malloc(sizeof(void *) *
			    rs->rules[rs_cnt].inactive.rcount,
			    M_TEMP, M_NOWAIT);

			if (!rs->rules[rs_cnt].inactive.ptr_array)
				return (ENOMEM);
		}

		TAILQ_FOREACH(rule, rs->rules[rs_cnt].inactive.ptr,
		    entries) {
			pf_hash_rule(&ctx, rule);
			(rs->rules[rs_cnt].inactive.ptr_array)[rule->nr] = rule;
		}
	}

	MD5Final(digest, &ctx);
	memcpy(pf_status.pf_chksum, digest, sizeof(pf_status.pf_chksum));
	return (0);
}

int
pfioctl(dev_t dev, u_long cmd, void *addr, int flags, struct lwp *l)
{
	struct pf_pooladdr	*pa = NULL;
	struct pf_pool		*pool = NULL;
	int			 s;
	int			 error = 0;

	/* XXX keep in sync with switch() below */
	if (kauth_authorize_network(l->l_cred, KAUTH_NETWORK_FIREWALL,
	    KAUTH_REQ_NETWORK_FIREWALL_FW, NULL, NULL, NULL)) 
		switch (cmd) {
		case DIOCGETRULES:
		case DIOCGETRULE:
		case DIOCGETADDRS:
		case DIOCGETADDR:
		case DIOCGETSTATE:
		case DIOCSETSTATUSIF:
		case DIOCGETSTATUS:
		case DIOCCLRSTATUS:
		case DIOCNATLOOK:
		case DIOCSETDEBUG:
		case DIOCGETSTATES:
		case DIOCGETTIMEOUT:
		case DIOCCLRRULECTRS:
		case DIOCGETLIMIT:
		case DIOCGETALTQS:
		case DIOCGETALTQ:
		case DIOCGETQSTATS:
		case DIOCGETRULESETS:
		case DIOCGETRULESET:
		case DIOCRGETTABLES:
		case DIOCRGETTSTATS:
		case DIOCRCLRTSTATS:
		case DIOCRCLRADDRS:
		case DIOCRADDADDRS:
		case DIOCRDELADDRS:
		case DIOCRSETADDRS:
		case DIOCRGETADDRS:
		case DIOCRGETASTATS:
		case DIOCRCLRASTATS:
		case DIOCRTSTADDRS:
		case DIOCOSFPGET:
		case DIOCGETSRCNODES:
		case DIOCCLRSRCNODES:
		case DIOCIGETIFACES:
		case DIOCSETIFFLAG:
		case DIOCCLRIFFLAG:
		case DIOCSETLCK:
		case DIOCADDSTATES:
			break;
		case DIOCRCLRTABLES:
		case DIOCRADDTABLES:
		case DIOCRDELTABLES:
		case DIOCRSETTFLAGS:
			if (((struct pfioc_table *)addr)->pfrio_flags &
			    PFR_FLAG_DUMMY)
				break; /* dummy operation ok */
			return (EPERM);
		default:
			return (EPERM);
		}

	if (!(flags & FWRITE))
		switch (cmd) {
		case DIOCGETRULES:
		case DIOCGETADDRS:
		case DIOCGETADDR:
		case DIOCGETSTATE:
		case DIOCGETSTATUS:
		case DIOCGETSTATES:
		case DIOCGETTIMEOUT:
		case DIOCGETLIMIT:
		case DIOCGETALTQS:
		case DIOCGETALTQ:
		case DIOCGETQSTATS:
		case DIOCGETRULESETS:
		case DIOCGETRULESET:
		case DIOCNATLOOK:
		case DIOCRGETTABLES:
		case DIOCRGETTSTATS:
		case DIOCRGETADDRS:
		case DIOCRGETASTATS:
		case DIOCRTSTADDRS:
		case DIOCOSFPGET:
		case DIOCGETSRCNODES:
		case DIOCIGETIFACES:
		case DIOCSETLCK:
			break;
		case DIOCRCLRTABLES:
		case DIOCRADDTABLES:
		case DIOCRDELTABLES:
		case DIOCRCLRTSTATS:
		case DIOCRCLRADDRS:
		case DIOCRADDADDRS:
		case DIOCRDELADDRS:
		case DIOCRSETADDRS:
		case DIOCRSETTFLAGS:
		case DIOCADDSTATES:
			if (((struct pfioc_table *)addr)->pfrio_flags &
			    PFR_FLAG_DUMMY) {
				flags |= FWRITE; /* need write lock for dummy */
				break; /* dummy operation ok */
			}
			return (EACCES);
		case DIOCGETRULE:
			if (((struct pfioc_rule *)addr)->action == PF_GET_CLR_CNTR)
				return (EACCES);
			break;
		default:
			return (EACCES);
		}

	if (flags & FWRITE)
		rw_enter_write(&pf_consistency_lock);
	else
		rw_enter_read(&pf_consistency_lock);

	s = splsoftnet();
	switch (cmd) {

	case DIOCSTART:
		if (pf_status.running)
			error = EEXIST;
		else {
#ifdef __NetBSD__
			error = pf_pfil_attach();
			if (error)
				break;
#endif /* __NetBSD__ */
			pf_status.running = 1;
			pf_status.since = time_second;
			if (pf_status.stateid == 0) {
				pf_status.stateid = time_second;
				pf_status.stateid = pf_status.stateid << 32;
			}
			DPFPRINTF(PF_DEBUG_MISC, ("pf: started\n"));
		}
		break;

	case DIOCSTOP:
		if (!pf_status.running)
			error = ENOENT;
		else {
#ifdef __NetBSD__
			error = pf_pfil_detach();
			if (error)
				break;
#endif /* __NetBSD__ */
			pf_status.running = 0;
			pf_status.since = time_second;
			DPFPRINTF(PF_DEBUG_MISC, ("pf: stopped\n"));
		}
		break;

	case DIOCADDRULE: {
		struct pfioc_rule	*pr = (struct pfioc_rule *)addr;
		struct pf_ruleset	*ruleset;
		struct pf_rule		*rule, *tail;
		int			 rs_num;

		pr->anchor[sizeof(pr->anchor) - 1] = 0;
		ruleset = pf_find_ruleset(pr->anchor);
		if (ruleset == NULL) {
			error = EINVAL;
			break;
		}
		rs_num = pf_get_ruleset_number(pr->rule.action);
		if (rs_num >= PF_RULESET_MAX) {
			error = EINVAL;
			break;
		}
		if (pr->rule.return_icmp >> 8 > ICMP_MAXTYPE) {
			error = EINVAL;
			break;
		}
		if (pr->ticket != ruleset->rules[rs_num].inactive.ticket) {
			error = EBUSY;
			break;
		}
		if (pr->pool_ticket != ticket_pabuf) {
			error = EBUSY;
			break;
		}
		rule = pool_get(&pf_rule_pl, PR_NOWAIT);
		if (rule == NULL) {
			error = ENOMEM;
			break;
		}
		bcopy(&pr->rule, rule, sizeof(struct pf_rule));
#ifdef __NetBSD__
		rule->cuid = kauth_cred_getuid(l->l_cred);
		rule->cpid = l->l_proc->p_pid;
#else
		rule->cuid = p->p_cred->p_ruid;
		rule->cpid = p->p_pid;
#endif /* !__NetBSD__ */
		rule->anchor = NULL;
		rule->kif = NULL;
		TAILQ_INIT(&rule->rpool.list);
		/* initialize refcounting */
		rule->states = 0;
		rule->src_nodes = 0;
		rule->entries.tqe_prev = NULL;
#ifndef INET
		if (rule->af == AF_INET) {
			pool_put(&pf_rule_pl, rule);
			error = EAFNOSUPPORT;
			break;
		}
#endif /* INET */
#ifndef INET6
		if (rule->af == AF_INET6) {
			pool_put(&pf_rule_pl, rule);
			error = EAFNOSUPPORT;
			break;
		}
#endif /* INET6 */
		tail = TAILQ_LAST(ruleset->rules[rs_num].inactive.ptr,
		    pf_rulequeue);
		if (tail)
			rule->nr = tail->nr + 1;
		else
			rule->nr = 0;
		if (rule->ifname[0]) {
			rule->kif = pfi_kif_get(rule->ifname);
			if (rule->kif == NULL) {
				pool_put(&pf_rule_pl, rule);
				error = EINVAL;
				break;
			}
			pfi_kif_ref(rule->kif, PFI_KIF_REF_RULE);
		}

#ifndef __NetBSD__
		if (rule->rtableid > 0 && !rtable_exists(rule->rtableid))
			error = EBUSY;
#endif /* !__NetBSD__ */

#ifdef ALTQ
		/* set queue IDs */
		if (rule->qname[0] != 0) {
			if ((rule->qid = pf_qname2qid(rule->qname)) == 0)
				error = EBUSY;
			else if (rule->pqname[0] != 0) {
				if ((rule->pqid =
				    pf_qname2qid(rule->pqname)) == 0)
					error = EBUSY;
			} else
				rule->pqid = rule->qid;
		}
#endif
		if (rule->tagname[0])
			if ((rule->tag = pf_tagname2tag(rule->tagname)) == 0)
				error = EBUSY;
		if (rule->match_tagname[0])
			if ((rule->match_tag =
			    pf_tagname2tag(rule->match_tagname)) == 0)
				error = EBUSY;
		if (rule->rt && !rule->direction)
			error = EINVAL;
#if NPFLOG > 0
		if (!rule->log)
			rule->logif = 0;
		if (rule->logif >= PFLOGIFS_MAX)
			error = EINVAL;
#endif
		if (pf_rtlabel_add(&rule->src.addr) ||
		    pf_rtlabel_add(&rule->dst.addr))
			error = EBUSY;
		if (pfi_dynaddr_setup(&rule->src.addr, rule->af))
			error = EINVAL;
		if (pfi_dynaddr_setup(&rule->dst.addr, rule->af))
			error = EINVAL;
		if (pf_tbladdr_setup(ruleset, &rule->src.addr))
			error = EINVAL;
		if (pf_tbladdr_setup(ruleset, &rule->dst.addr))
			error = EINVAL;
		if (pf_anchor_setup(rule, ruleset, pr->anchor_call))
			error = EINVAL;
		TAILQ_FOREACH(pa, &pf_pabuf, entries)
			if (pf_tbladdr_setup(ruleset, &pa->addr))
				error = EINVAL;

		rule->overload_tbl = NULL;
		if (rule->overload_tblname[0]) {
			if ((rule->overload_tbl = pfr_attach_table(ruleset,
			    rule->overload_tblname)) == NULL)
				error = EINVAL;
			else
				rule->overload_tbl->pfrkt_flags |=
				    PFR_TFLAG_ACTIVE;
		}

		pf_mv_pool(&pf_pabuf, &rule->rpool.list);
		if (((((rule->action == PF_NAT) || (rule->action == PF_RDR) ||
		    (rule->action == PF_BINAT)) && rule->anchor == NULL) ||
		    (rule->rt > PF_FASTROUTE)) &&
		    (TAILQ_FIRST(&rule->rpool.list) == NULL))
			error = EINVAL;

		if (error) {
			pf_rm_rule(NULL, rule);
			break;
		}
		rule->rpool.cur = TAILQ_FIRST(&rule->rpool.list);
		rule->evaluations = rule->packets[0] = rule->packets[1] =
		    rule->bytes[0] = rule->bytes[1] = 0;
		TAILQ_INSERT_TAIL(ruleset->rules[rs_num].inactive.ptr,
		    rule, entries);
		ruleset->rules[rs_num].inactive.rcount++;
		break;
	}

	case DIOCGETRULES: {
		struct pfioc_rule	*pr = (struct pfioc_rule *)addr;
		struct pf_ruleset	*ruleset;
		struct pf_rule		*tail;
		int			 rs_num;

		pr->anchor[sizeof(pr->anchor) - 1] = 0;
		ruleset = pf_find_ruleset(pr->anchor);
		if (ruleset == NULL) {
			error = EINVAL;
			break;
		}
		rs_num = pf_get_ruleset_number(pr->rule.action);
		if (rs_num >= PF_RULESET_MAX) {
			error = EINVAL;
			break;
		}
		tail = TAILQ_LAST(ruleset->rules[rs_num].active.ptr,
		    pf_rulequeue);
		if (tail)
			pr->nr = tail->nr + 1;
		else
			pr->nr = 0;
		pr->ticket = ruleset->rules[rs_num].active.ticket;
		break;
	}

	case DIOCGETRULE: {
		struct pfioc_rule	*pr = (struct pfioc_rule *)addr;
		struct pf_ruleset	*ruleset;
		struct pf_rule		*rule;
		int			 rs_num, i;

		pr->anchor[sizeof(pr->anchor) - 1] = 0;
		ruleset = pf_find_ruleset(pr->anchor);
		if (ruleset == NULL) {
			error = EINVAL;
			break;
		}
		rs_num = pf_get_ruleset_number(pr->rule.action);
		if (rs_num >= PF_RULESET_MAX) {
			error = EINVAL;
			break;
		}
		if (pr->ticket != ruleset->rules[rs_num].active.ticket) {
			error = EBUSY;
			break;
		}
		rule = TAILQ_FIRST(ruleset->rules[rs_num].active.ptr);
		while ((rule != NULL) && (rule->nr != pr->nr))
			rule = TAILQ_NEXT(rule, entries);
		if (rule == NULL) {
			error = EBUSY;
			break;
		}
		bcopy(rule, &pr->rule, sizeof(struct pf_rule));
		if (pf_anchor_copyout(ruleset, rule, pr)) {
			error = EBUSY;
			break;
		}
		pfi_dynaddr_copyout(&pr->rule.src.addr);
		pfi_dynaddr_copyout(&pr->rule.dst.addr);
		pf_tbladdr_copyout(&pr->rule.src.addr);
		pf_tbladdr_copyout(&pr->rule.dst.addr);
		pf_rtlabel_copyout(&pr->rule.src.addr);
		pf_rtlabel_copyout(&pr->rule.dst.addr);
		for (i = 0; i < PF_SKIP_COUNT; ++i)
			if (rule->skip[i].ptr == NULL)
				pr->rule.skip[i].nr = -1;
			else
				pr->rule.skip[i].nr =
				    rule->skip[i].ptr->nr;

		if (pr->action == PF_GET_CLR_CNTR) {
			rule->evaluations = 0;
			rule->packets[0] = rule->packets[1] = 0;
			rule->bytes[0] = rule->bytes[1] = 0;
		}
		break;
	}

	case DIOCCHANGERULE: {
		struct pfioc_rule	*pcr = (struct pfioc_rule *)addr;
		struct pf_ruleset	*ruleset;
		struct pf_rule		*oldrule = NULL, *newrule = NULL;
		u_int32_t		 nr = 0;
		int			 rs_num;

		if (!(pcr->action == PF_CHANGE_REMOVE ||
		    pcr->action == PF_CHANGE_GET_TICKET) &&
		    pcr->pool_ticket != ticket_pabuf) {
			error = EBUSY;
			break;
		}

		if (pcr->action < PF_CHANGE_ADD_HEAD ||
		    pcr->action > PF_CHANGE_GET_TICKET) {
			error = EINVAL;
			break;
		}
		ruleset = pf_find_ruleset(pcr->anchor);
		if (ruleset == NULL) {
			error = EINVAL;
			break;
		}
		rs_num = pf_get_ruleset_number(pcr->rule.action);
		if (rs_num >= PF_RULESET_MAX) {
			error = EINVAL;
			break;
		}

		if (pcr->action == PF_CHANGE_GET_TICKET) {
			pcr->ticket = ++ruleset->rules[rs_num].active.ticket;
			break;
		} else {
			if (pcr->ticket !=
			    ruleset->rules[rs_num].active.ticket) {
				error = EINVAL;
				break;
			}
			if (pcr->rule.return_icmp >> 8 > ICMP_MAXTYPE) {
				error = EINVAL;
				break;
			}
		}

		if (pcr->action != PF_CHANGE_REMOVE) {
			newrule = pool_get(&pf_rule_pl, PR_NOWAIT);
			if (newrule == NULL) {
				error = ENOMEM;
				break;
			}
			bcopy(&pcr->rule, newrule, sizeof(struct pf_rule));
#ifdef __NetBSD__
			newrule->cuid = kauth_cred_getuid(l->l_cred);
			newrule->cpid = l->l_proc->p_pid;
#else
			newrule->cuid = p->p_cred->p_ruid;
			newrule->cpid = p->p_pid;
#endif /* !__NetBSD__ */
			TAILQ_INIT(&newrule->rpool.list);
			/* initialize refcounting */
			newrule->states = 0;
			newrule->entries.tqe_prev = NULL;
#ifndef INET
			if (newrule->af == AF_INET) {
				pool_put(&pf_rule_pl, newrule);
				error = EAFNOSUPPORT;
				break;
			}
#endif /* INET */
#ifndef INET6
			if (newrule->af == AF_INET6) {
				pool_put(&pf_rule_pl, newrule);
				error = EAFNOSUPPORT;
				break;
			}
#endif /* INET6 */
			if (newrule->ifname[0]) {
				newrule->kif = pfi_kif_get(newrule->ifname);
				if (newrule->kif == NULL) {
					pool_put(&pf_rule_pl, newrule);
					error = EINVAL;
					break;
				}
				pfi_kif_ref(newrule->kif, PFI_KIF_REF_RULE);
			} else
				newrule->kif = NULL;

#ifndef __NetBSD__
			if (newrule->rtableid > 0 &&
			    !rtable_exists(newrule->rtableid))
				error = EBUSY;
#endif /* !__NetBSD__ */

#ifdef ALTQ
			/* set queue IDs */
			if (newrule->qname[0] != 0) {
				if ((newrule->qid =
				    pf_qname2qid(newrule->qname)) == 0)
					error = EBUSY;
				else if (newrule->pqname[0] != 0) {
					if ((newrule->pqid =
					    pf_qname2qid(newrule->pqname)) == 0)
						error = EBUSY;
				} else
					newrule->pqid = newrule->qid;
			}
#endif /* ALTQ */
			if (newrule->tagname[0])
				if ((newrule->tag =
				    pf_tagname2tag(newrule->tagname)) == 0)
					error = EBUSY;
			if (newrule->match_tagname[0])
				if ((newrule->match_tag = pf_tagname2tag(
				    newrule->match_tagname)) == 0)
					error = EBUSY;
			if (newrule->rt && !newrule->direction)
				error = EINVAL;
#if NPFLOG > 0
			if (!newrule->log)
				newrule->logif = 0;
			if (newrule->logif >= PFLOGIFS_MAX)
				error = EINVAL;
#endif
			if (pf_rtlabel_add(&newrule->src.addr) ||
			    pf_rtlabel_add(&newrule->dst.addr))
				error = EBUSY;
			if (pfi_dynaddr_setup(&newrule->src.addr, newrule->af))
				error = EINVAL;
			if (pfi_dynaddr_setup(&newrule->dst.addr, newrule->af))
				error = EINVAL;
			if (pf_tbladdr_setup(ruleset, &newrule->src.addr))
				error = EINVAL;
			if (pf_tbladdr_setup(ruleset, &newrule->dst.addr))
				error = EINVAL;
			if (pf_anchor_setup(newrule, ruleset, pcr->anchor_call))
				error = EINVAL;
			TAILQ_FOREACH(pa, &pf_pabuf, entries)
				if (pf_tbladdr_setup(ruleset, &pa->addr))
					error = EINVAL;

			newrule->overload_tbl = NULL;
			if (newrule->overload_tblname[0]) {
				if ((newrule->overload_tbl = pfr_attach_table(
				    ruleset, newrule->overload_tblname)) ==
				    NULL)
					error = EINVAL;
				else
					newrule->overload_tbl->pfrkt_flags |=
					    PFR_TFLAG_ACTIVE;
			}

			pf_mv_pool(&pf_pabuf, &newrule->rpool.list);
			if (((((newrule->action == PF_NAT) ||
			    (newrule->action == PF_RDR) ||
			    (newrule->action == PF_BINAT) ||
			    (newrule->rt > PF_FASTROUTE)) &&
			    !newrule->anchor)) &&
			    (TAILQ_FIRST(&newrule->rpool.list) == NULL))
				error = EINVAL;

			if (error) {
				pf_rm_rule(NULL, newrule);
				break;
			}
			newrule->rpool.cur = TAILQ_FIRST(&newrule->rpool.list);
			newrule->evaluations = 0;
			newrule->packets[0] = newrule->packets[1] = 0;
			newrule->bytes[0] = newrule->bytes[1] = 0;
		}
		pf_empty_pool(&pf_pabuf);

		if (pcr->action == PF_CHANGE_ADD_HEAD)
			oldrule = TAILQ_FIRST(
			    ruleset->rules[rs_num].active.ptr);
		else if (pcr->action == PF_CHANGE_ADD_TAIL)
			oldrule = TAILQ_LAST(
			    ruleset->rules[rs_num].active.ptr, pf_rulequeue);
		else {
			oldrule = TAILQ_FIRST(
			    ruleset->rules[rs_num].active.ptr);
			while ((oldrule != NULL) && (oldrule->nr != pcr->nr))
				oldrule = TAILQ_NEXT(oldrule, entries);
			if (oldrule == NULL) {
				if (newrule != NULL)
					pf_rm_rule(NULL, newrule);
				error = EINVAL;
				break;
			}
		}

		if (pcr->action == PF_CHANGE_REMOVE) {
			pf_rm_rule(ruleset->rules[rs_num].active.ptr, oldrule);
			ruleset->rules[rs_num].active.rcount--;
		} else {
			if (oldrule == NULL)
				TAILQ_INSERT_TAIL(
				    ruleset->rules[rs_num].active.ptr,
				    newrule, entries);
			else if (pcr->action == PF_CHANGE_ADD_HEAD ||
			    pcr->action == PF_CHANGE_ADD_BEFORE)
				TAILQ_INSERT_BEFORE(oldrule, newrule, entries);
			else
				TAILQ_INSERT_AFTER(
				    ruleset->rules[rs_num].active.ptr,
				    oldrule, newrule, entries);
			ruleset->rules[rs_num].active.rcount++;
		}

		nr = 0;
		TAILQ_FOREACH(oldrule,
		    ruleset->rules[rs_num].active.ptr, entries)
			oldrule->nr = nr++;

		ruleset->rules[rs_num].active.ticket++;

		pf_calc_skip_steps(ruleset->rules[rs_num].active.ptr);
		pf_remove_if_empty_ruleset(ruleset);

		break;
	}

	case DIOCCLRSTATES: {
		struct pf_state		*ps, *nexts;
		struct pfioc_state_kill *psk = (struct pfioc_state_kill *)addr;
		int			 killed = 0;

		for (ps = RB_MIN(pf_state_tree_id, &tree_id); ps; ps = nexts) {
			nexts = RB_NEXT(pf_state_tree_id, &tree_id, ps);

			if (!psk->psk_ifname[0] || !strcmp(psk->psk_ifname,
			    ps->kif->pfik_name)) {
#if NPFSYNC
				/* don't send out individual delete messages */
				ps->sync_flags = PFSTATE_NOSYNC;
#endif
				pf_unlink_state(ps);
				killed++;
			}
		}
		psk->psk_af = killed;
#if NPFSYNC
		pfsync_clear_states(pf_status.hostid, psk->psk_ifname);
#endif
		break;
	}

	case DIOCKILLSTATES: {
		struct pf_state		*ps, *nexts;
		struct pf_state_key	*sk;
		struct pf_state_host	*src, *dst;
		struct pfioc_state_kill	*psk = (struct pfioc_state_kill *)addr;
		int			 killed = 0;

		for (ps = RB_MIN(pf_state_tree_id, &tree_id); ps;
		    ps = nexts) {
			nexts = RB_NEXT(pf_state_tree_id, &tree_id, ps);
			sk = ps->state_key;

			if (sk->direction == PF_OUT) {
				src = &sk->lan;
				dst = &sk->ext;
			} else {
				src = &sk->ext;
				dst = &sk->lan;
			}
			if ((!psk->psk_af || sk->af == psk->psk_af)
			    && (!psk->psk_proto || psk->psk_proto ==
			    sk->proto) &&
			    PF_MATCHA(psk->psk_src.neg,
			    &psk->psk_src.addr.v.a.addr,
			    &psk->psk_src.addr.v.a.mask,
			    &src->addr, sk->af) &&
			    PF_MATCHA(psk->psk_dst.neg,
			    &psk->psk_dst.addr.v.a.addr,
			    &psk->psk_dst.addr.v.a.mask,
			    &dst->addr, sk->af) &&
			    (psk->psk_src.port_op == 0 ||
			    pf_match_port(psk->psk_src.port_op,
			    psk->psk_src.port[0], psk->psk_src.port[1],
			    src->port)) &&
			    (psk->psk_dst.port_op == 0 ||
			    pf_match_port(psk->psk_dst.port_op,
			    psk->psk_dst.port[0], psk->psk_dst.port[1],
			    dst->port)) &&
			    (!psk->psk_ifname[0] || !strcmp(psk->psk_ifname,
			    ps->kif->pfik_name))) {
#if NPFSYNC > 0
				/* send immediate delete of state */
				pfsync_delete_state(ps);
				ps->sync_flags |= PFSTATE_NOSYNC;
#endif
				pf_unlink_state(ps);
				killed++;
			}
		}
		psk->psk_af = killed;
		break;
	}

	case DIOCADDSTATE: {
		struct pfioc_state	*ps = (struct pfioc_state *)addr;
		struct pfsync_state 	*sp = (struct pfsync_state *)ps->state;

		error = pf_state_add(sp);
		break;
	}

	case DIOCADDSTATES: {
		struct pfioc_states	*ps = (struct pfioc_states *)addr;
		struct pfsync_state	*p = (struct pfsync_state *) ps->ps_states;
		struct pfsync_state *pk;
		int size = ps->ps_len;
		int i = 0;
		error = 0;

		pk = malloc(sizeof(*pk), M_TEMP,M_WAITOK);

		while (error == 0 && i < size) 
		{
			if (copyin(p, pk, sizeof(struct pfsync_state))) 
			{
				error = EFAULT;
				free(pk, M_TEMP);
			} else {
				error = pf_state_add(pk);
				i += sizeof(*p);
				p++;
			}
		}

		free(pk, M_TEMP);
		break;
	}


	case DIOCGETSTATE: {
		struct pfioc_state	*ps = (struct pfioc_state *)addr;
		struct pf_state		*pfs;
		u_int32_t		 nr;

		nr = 0;
		RB_FOREACH(pfs, pf_state_tree_id, &tree_id) {
			if (nr >= ps->nr)
				break;
			nr++;
		}
		if (pfs == NULL) {
			error = EBUSY;
			break;
		}

		pf_state_export((struct pfsync_state *)&ps->state,
		    pfs->state_key, pfs);
		break;
	}

	case DIOCGETSTATES: {
		struct pfioc_states	*ps = (struct pfioc_states *)addr;
		struct pf_state		*state;
		struct pfsync_state	*p, *pstore;
		u_int32_t		 nr = 0;

		if (ps->ps_len == 0) {
			nr = pf_status.states;
			ps->ps_len = sizeof(struct pfsync_state) * nr;
			break;
		}

		pstore = malloc(sizeof(*pstore), M_TEMP, M_WAITOK);

		p = ps->ps_states;

		state = TAILQ_FIRST(&state_list);
		while (state) {
			if (state->timeout != PFTM_UNLINKED) {
				if ((nr+1) * sizeof(*p) > (unsigned)ps->ps_len)
					break;

				pf_state_export(pstore,
				    state->state_key, state);
				error = copyout(pstore, p, sizeof(*p));
				if (error) {
					free(pstore, M_TEMP);
					goto fail;
				}
				p++;
				nr++;
			}
			state = TAILQ_NEXT(state, entry_list);
		}

		ps->ps_len = sizeof(struct pfsync_state) * nr;

		free(pstore, M_TEMP);
		break;
	}

	case DIOCGETSTATUS: {
		struct pf_status *ps = (struct pf_status *)addr;
		bcopy(&pf_status, ps, sizeof(struct pf_status));
		pfi_fill_oldstatus(ps);
		break;
	}

	case DIOCSETSTATUSIF: {
		struct pfioc_if	*pi = (struct pfioc_if *)addr;

		if (pi->ifname[0] == 0) {
			bzero(pf_status.ifname, IFNAMSIZ);
			break;
		}
		if (ifunit(pi->ifname) == NULL) {
			error = EINVAL;
			break;
		}
		strlcpy(pf_status.ifname, pi->ifname, IFNAMSIZ);
		break;
	}

	case DIOCCLRSTATUS: {
		bzero(pf_status.counters, sizeof(pf_status.counters));
		bzero(pf_status.fcounters, sizeof(pf_status.fcounters));
		bzero(pf_status.scounters, sizeof(pf_status.scounters));
		pf_status.since = time_second;
		if (*pf_status.ifname)
			pfi_clr_istats(pf_status.ifname);
		break;
	}

	case DIOCNATLOOK: {
		struct pfioc_natlook	*pnl = (struct pfioc_natlook *)addr;
		struct pf_state_key	*sk;
		struct pf_state		*state;
		struct pf_state_key_cmp	 key;
		int			 m = 0, direction = pnl->direction;

		key.af = pnl->af;
		key.proto = pnl->proto;

		if (!pnl->proto ||
		    PF_AZERO(&pnl->saddr, pnl->af) ||
		    PF_AZERO(&pnl->daddr, pnl->af) ||
		    ((pnl->proto == IPPROTO_TCP ||
		    pnl->proto == IPPROTO_UDP) &&
		    (!pnl->dport || !pnl->sport)))
			error = EINVAL;
		else {
			/*
			 * userland gives us source and dest of connection,
			 * reverse the lookup so we ask for what happens with
			 * the return traffic, enabling us to find it in the
			 * state tree.
			 */
			if (direction == PF_IN) {
				PF_ACPY(&key.ext.addr, &pnl->daddr, pnl->af);
				key.ext.port = pnl->dport;
				PF_ACPY(&key.gwy.addr, &pnl->saddr, pnl->af);
				key.gwy.port = pnl->sport;
				state = pf_find_state_all(&key, PF_EXT_GWY, &m);
			} else {
				PF_ACPY(&key.lan.addr, &pnl->daddr, pnl->af);
				key.lan.port = pnl->dport;
				PF_ACPY(&key.ext.addr, &pnl->saddr, pnl->af);
				key.ext.port = pnl->sport;
				state = pf_find_state_all(&key, PF_LAN_EXT, &m);
			}
			if (m > 1)
				error = E2BIG;	/* more than one state */
			else if (state != NULL) {
				sk = state->state_key;
				if (direction == PF_IN) {
					PF_ACPY(&pnl->rsaddr, &sk->lan.addr,
					    sk->af);
					pnl->rsport = sk->lan.port;
					PF_ACPY(&pnl->rdaddr, &pnl->daddr,
					    pnl->af);
					pnl->rdport = pnl->dport;
				} else {
					PF_ACPY(&pnl->rdaddr, &sk->gwy.addr,
					    sk->af);
					pnl->rdport = sk->gwy.port;
					PF_ACPY(&pnl->rsaddr, &pnl->saddr,
					    pnl->af);
					pnl->rsport = pnl->sport;
				}
			} else
				error = ENOENT;
		}
		break;
	}

	case DIOCSETTIMEOUT: {
		struct pfioc_tm	*pt = (struct pfioc_tm *)addr;
		int		 old;

		if (pt->timeout < 0 || pt->timeout >= PFTM_MAX ||
		    pt->seconds < 0) {
			error = EINVAL;
			goto fail;
		}
		old = pf_default_rule.timeout[pt->timeout];
		if (pt->timeout == PFTM_INTERVAL && pt->seconds == 0)
			pt->seconds = 1;
		pf_default_rule.timeout[pt->timeout] = pt->seconds;
		if (pt->timeout == PFTM_INTERVAL && pt->seconds < old)
			wakeup(pf_purge_thread);
		pt->seconds = old;
		break;
	}

	case DIOCGETTIMEOUT: {
		struct pfioc_tm	*pt = (struct pfioc_tm *)addr;

		if (pt->timeout < 0 || pt->timeout >= PFTM_MAX) {
			error = EINVAL;
			goto fail;
		}
		pt->seconds = pf_default_rule.timeout[pt->timeout];
		break;
	}

	case DIOCGETLIMIT: {
		struct pfioc_limit	*pl = (struct pfioc_limit *)addr;

		if (pl->index < 0 || pl->index >= PF_LIMIT_MAX) {
			error = EINVAL;
			goto fail;
		}
		pl->limit = pf_pool_limits[pl->index].limit;
		break;
	}

	case DIOCSETLIMIT: {
		struct pfioc_limit	*pl = (struct pfioc_limit *)addr;
		int			 old_limit;

		if (pl->index < 0 || pl->index >= PF_LIMIT_MAX ||
		    pf_pool_limits[pl->index].pp == NULL) {
			error = EINVAL;
			goto fail;
		}
#ifdef __NetBSD__
		pool_sethardlimit(pf_pool_limits[pl->index].pp,
		    pl->limit, NULL, 0);
#else
		if (pool_sethardlimit(pf_pool_limits[pl->index].pp,
		    pl->limit, NULL, 0) != 0) {
			error = EBUSY;
			goto fail;
		}
#endif /* !__NetBSD__ */
		old_limit = pf_pool_limits[pl->index].limit;
		pf_pool_limits[pl->index].limit = pl->limit;
		pl->limit = old_limit;
		break;
	}

	case DIOCSETDEBUG: {
		u_int32_t	*level = (u_int32_t *)addr;

		pf_status.debug = *level;
		break;
	}

	case DIOCCLRRULECTRS: {
		/* obsoleted by DIOCGETRULE with action=PF_GET_CLR_CNTR */
		struct pf_ruleset	*ruleset = &pf_main_ruleset;
		struct pf_rule		*rule;

		TAILQ_FOREACH(rule,
		    ruleset->rules[PF_RULESET_FILTER].active.ptr, entries) {
			rule->evaluations = 0;
			rule->packets[0] = rule->packets[1] = 0;
			rule->bytes[0] = rule->bytes[1] = 0;
		}
		break;
	}

#ifdef ALTQ
	case DIOCSTARTALTQ: {
		struct pf_altq		*altq;

		/* enable all altq interfaces on active list */
		TAILQ_FOREACH(altq, pf_altqs_active, entries) {
			if (altq->qname[0] == 0) {
				error = pf_enable_altq(altq);
				if (error != 0)
					break;
			}
		}
		if (error == 0)
			pf_altq_running = 1;
		DPFPRINTF(PF_DEBUG_MISC, ("altq: started\n"));
		break;
	}

	case DIOCSTOPALTQ: {
		struct pf_altq		*altq;

		/* disable all altq interfaces on active list */
		TAILQ_FOREACH(altq, pf_altqs_active, entries) {
			if (altq->qname[0] == 0) {
				error = pf_disable_altq(altq);
				if (error != 0)
					break;
			}
		}
		if (error == 0)
			pf_altq_running = 0;
		DPFPRINTF(PF_DEBUG_MISC, ("altq: stopped\n"));
		break;
	}

	case DIOCADDALTQ: {
		struct pfioc_altq	*paa = (struct pfioc_altq *)addr;
		struct pf_altq		*altq, *a;

		if (paa->ticket != ticket_altqs_inactive) {
			error = EBUSY;
			break;
		}
		altq = pool_get(&pf_altq_pl, PR_NOWAIT);
		if (altq == NULL) {
			error = ENOMEM;
			break;
		}
		bcopy(&paa->altq, altq, sizeof(struct pf_altq));

		/*
		 * if this is for a queue, find the discipline and
		 * copy the necessary fields
		 */
		if (altq->qname[0] != 0) {
			if ((altq->qid = pf_qname2qid(altq->qname)) == 0) {
				error = EBUSY;
				pool_put(&pf_altq_pl, altq);
				break;
			}
			TAILQ_FOREACH(a, pf_altqs_inactive, entries) {
				if (strncmp(a->ifname, altq->ifname,
				    IFNAMSIZ) == 0 && a->qname[0] == 0) {
					altq->altq_disc = a->altq_disc;
					break;
				}
			}
		}

		error = altq_add(altq);
		if (error) {
			pool_put(&pf_altq_pl, altq);
			break;
		}

		TAILQ_INSERT_TAIL(pf_altqs_inactive, altq, entries);
		bcopy(altq, &paa->altq, sizeof(struct pf_altq));
		break;
	}

	case DIOCGETALTQS: {
		struct pfioc_altq	*paa = (struct pfioc_altq *)addr;
		struct pf_altq		*altq;

		paa->nr = 0;
		TAILQ_FOREACH(altq, pf_altqs_active, entries)
			paa->nr++;
		paa->ticket = ticket_altqs_active;
		break;
	}

	case DIOCGETALTQ: {
		struct pfioc_altq	*paa = (struct pfioc_altq *)addr;
		struct pf_altq		*altq;
		u_int32_t		 nr;

		if (paa->ticket != ticket_altqs_active) {
			error = EBUSY;
			break;
		}
		nr = 0;
		altq = TAILQ_FIRST(pf_altqs_active);
		while ((altq != NULL) && (nr < paa->nr)) {
			altq = TAILQ_NEXT(altq, entries);
			nr++;
		}
		if (altq == NULL) {
			error = EBUSY;
			break;
		}
		bcopy(altq, &paa->altq, sizeof(struct pf_altq));
		break;
	}

	case DIOCCHANGEALTQ:
		/* CHANGEALTQ not supported yet! */
		error = ENODEV;
		break;

	case DIOCGETQSTATS: {
		struct pfioc_qstats	*pq = (struct pfioc_qstats *)addr;
		struct pf_altq		*altq;
		u_int32_t		 nr;
		int			 nbytes;

		if (pq->ticket != ticket_altqs_active) {
			error = EBUSY;
			break;
		}
		nbytes = pq->nbytes;
		nr = 0;
		altq = TAILQ_FIRST(pf_altqs_active);
		while ((altq != NULL) && (nr < pq->nr)) {
			altq = TAILQ_NEXT(altq, entries);
			nr++;
		}
		if (altq == NULL) {
			error = EBUSY;
			break;
		}
		error = altq_getqstats(altq, pq->buf, &nbytes);
		if (error == 0) {
			pq->scheduler = altq->scheduler;
			pq->nbytes = nbytes;
		}
		break;
	}
#endif /* ALTQ */

	case DIOCBEGINADDRS: {
		struct pfioc_pooladdr	*pp = (struct pfioc_pooladdr *)addr;

		pf_empty_pool(&pf_pabuf);
		pp->ticket = ++ticket_pabuf;
		break;
	}

	case DIOCADDADDR: {
		struct pfioc_pooladdr	*pp = (struct pfioc_pooladdr *)addr;

		if (pp->ticket != ticket_pabuf) {
			error = EBUSY;
			break;
		}
#ifndef INET
		if (pp->af == AF_INET) {
			error = EAFNOSUPPORT;
			break;
		}
#endif /* INET */
#ifndef INET6
		if (pp->af == AF_INET6) {
			error = EAFNOSUPPORT;
			break;
		}
#endif /* INET6 */
		if (pp->addr.addr.type != PF_ADDR_ADDRMASK &&
		    pp->addr.addr.type != PF_ADDR_DYNIFTL &&
		    pp->addr.addr.type != PF_ADDR_TABLE) {
			error = EINVAL;
			break;
		}
		pa = pool_get(&pf_pooladdr_pl, PR_NOWAIT);
		if (pa == NULL) {
			error = ENOMEM;
			break;
		}
		bcopy(&pp->addr, pa, sizeof(struct pf_pooladdr));
		if (pa->ifname[0]) {
			pa->kif = pfi_kif_get(pa->ifname);
			if (pa->kif == NULL) {
				pool_put(&pf_pooladdr_pl, pa);
				error = EINVAL;
				break;
			}
			pfi_kif_ref(pa->kif, PFI_KIF_REF_RULE);
		}
		if (pfi_dynaddr_setup(&pa->addr, pp->af)) {
			pfi_dynaddr_remove(&pa->addr);
			pfi_kif_unref(pa->kif, PFI_KIF_REF_RULE);
			pool_put(&pf_pooladdr_pl, pa);
			error = EINVAL;
			break;
		}
		TAILQ_INSERT_TAIL(&pf_pabuf, pa, entries);
		break;
	}

	case DIOCGETADDRS: {
		struct pfioc_pooladdr	*pp = (struct pfioc_pooladdr *)addr;

		pp->nr = 0;
		pool = pf_get_pool(pp->anchor, pp->ticket, pp->r_action,
		    pp->r_num, 0, 1, 0);
		if (pool == NULL) {
			error = EBUSY;
			break;
		}
		TAILQ_FOREACH(pa, &pool->list, entries)
			pp->nr++;
		break;
	}

	case DIOCGETADDR: {
		struct pfioc_pooladdr	*pp = (struct pfioc_pooladdr *)addr;
		u_int32_t		 nr = 0;

		pool = pf_get_pool(pp->anchor, pp->ticket, pp->r_action,
		    pp->r_num, 0, 1, 1);
		if (pool == NULL) {
			error = EBUSY;
			break;
		}
		pa = TAILQ_FIRST(&pool->list);
		while ((pa != NULL) && (nr < pp->nr)) {
			pa = TAILQ_NEXT(pa, entries);
			nr++;
		}
		if (pa == NULL) {
			error = EBUSY;
			break;
		}
		bcopy(pa, &pp->addr, sizeof(struct pf_pooladdr));
		pfi_dynaddr_copyout(&pp->addr.addr);
		pf_tbladdr_copyout(&pp->addr.addr);
		pf_rtlabel_copyout(&pp->addr.addr);
		break;
	}

	case DIOCCHANGEADDR: {
		struct pfioc_pooladdr	*pca = (struct pfioc_pooladdr *)addr;
		struct pf_pooladdr	*oldpa = NULL, *newpa = NULL;
		struct pf_ruleset	*ruleset;

		if (pca->action < PF_CHANGE_ADD_HEAD ||
		    pca->action > PF_CHANGE_REMOVE) {
			error = EINVAL;
			break;
		}
		if (pca->addr.addr.type != PF_ADDR_ADDRMASK &&
		    pca->addr.addr.type != PF_ADDR_DYNIFTL &&
		    pca->addr.addr.type != PF_ADDR_TABLE) {
			error = EINVAL;
			break;
		}

		ruleset = pf_find_ruleset(pca->anchor);
		if (ruleset == NULL) {
			error = EBUSY;
			break;
		}
		pool = pf_get_pool(pca->anchor, pca->ticket, pca->r_action,
		    pca->r_num, pca->r_last, 1, 1);
		if (pool == NULL) {
			error = EBUSY;
			break;
		}
		if (pca->action != PF_CHANGE_REMOVE) {
			newpa = pool_get(&pf_pooladdr_pl, PR_NOWAIT);
			if (newpa == NULL) {
				error = ENOMEM;
				break;
			}
			bcopy(&pca->addr, newpa, sizeof(struct pf_pooladdr));
#ifndef INET
			if (pca->af == AF_INET) {
				pool_put(&pf_pooladdr_pl, newpa);
				error = EAFNOSUPPORT;
				break;
			}
#endif /* INET */
#ifndef INET6
			if (pca->af == AF_INET6) {
				pool_put(&pf_pooladdr_pl, newpa);
				error = EAFNOSUPPORT;
				break;
			}
#endif /* INET6 */
			if (newpa->ifname[0]) {
				newpa->kif = pfi_kif_get(newpa->ifname);
				if (newpa->kif == NULL) {
					pool_put(&pf_pooladdr_pl, newpa);
					error = EINVAL;
					break;
				}
				pfi_kif_ref(newpa->kif, PFI_KIF_REF_RULE);
			} else
				newpa->kif = NULL;
			if (pfi_dynaddr_setup(&newpa->addr, pca->af) ||
			    pf_tbladdr_setup(ruleset, &newpa->addr)) {
				pfi_dynaddr_remove(&newpa->addr);
				pfi_kif_unref(newpa->kif, PFI_KIF_REF_RULE);
				pool_put(&pf_pooladdr_pl, newpa);
				error = EINVAL;
				break;
			}
		}

		if (pca->action == PF_CHANGE_ADD_HEAD)
			oldpa = TAILQ_FIRST(&pool->list);
		else if (pca->action == PF_CHANGE_ADD_TAIL)
			oldpa = TAILQ_LAST(&pool->list, pf_palist);
		else {
			int	i = 0;

			oldpa = TAILQ_FIRST(&pool->list);
			while ((oldpa != NULL) && (i < pca->nr)) {
				oldpa = TAILQ_NEXT(oldpa, entries);
				i++;
			}
			if (oldpa == NULL) {
				error = EINVAL;
				break;
			}
		}

		if (pca->action == PF_CHANGE_REMOVE) {
			TAILQ_REMOVE(&pool->list, oldpa, entries);
			pfi_dynaddr_remove(&oldpa->addr);
			pf_tbladdr_remove(&oldpa->addr);
			pfi_kif_unref(oldpa->kif, PFI_KIF_REF_RULE);
			pool_put(&pf_pooladdr_pl, oldpa);
		} else {
			if (oldpa == NULL)
				TAILQ_INSERT_TAIL(&pool->list, newpa, entries);
			else if (pca->action == PF_CHANGE_ADD_HEAD ||
			    pca->action == PF_CHANGE_ADD_BEFORE)
				TAILQ_INSERT_BEFORE(oldpa, newpa, entries);
			else
				TAILQ_INSERT_AFTER(&pool->list, oldpa,
				    newpa, entries);
		}

		pool->cur = TAILQ_FIRST(&pool->list);
		PF_ACPY(&pool->counter, &pool->cur->addr.v.a.addr,
		    pca->af);
		break;
	}

	case DIOCGETRULESETS: {
		struct pfioc_ruleset	*pr = (struct pfioc_ruleset *)addr;
		struct pf_ruleset	*ruleset;
		struct pf_anchor	*anchor;

		pr->path[sizeof(pr->path) - 1] = 0;
		if ((ruleset = pf_find_ruleset(pr->path)) == NULL) {
			error = EINVAL;
			break;
		}
		pr->nr = 0;
		if (ruleset->anchor == NULL) {
			/* XXX kludge for pf_main_ruleset */
			RB_FOREACH(anchor, pf_anchor_global, &pf_anchors)
				if (anchor->parent == NULL)
					pr->nr++;
		} else {
			RB_FOREACH(anchor, pf_anchor_node,
			    &ruleset->anchor->children)
				pr->nr++;
		}
		break;
	}

	case DIOCGETRULESET: {
		struct pfioc_ruleset	*pr = (struct pfioc_ruleset *)addr;
		struct pf_ruleset	*ruleset;
		struct pf_anchor	*anchor;
		u_int32_t		 nr = 0;

		pr->path[sizeof(pr->path) - 1] = 0;
		if ((ruleset = pf_find_ruleset(pr->path)) == NULL) {
			error = EINVAL;
			break;
		}
		pr->name[0] = 0;
		if (ruleset->anchor == NULL) {
			/* XXX kludge for pf_main_ruleset */
			RB_FOREACH(anchor, pf_anchor_global, &pf_anchors)
				if (anchor->parent == NULL && nr++ == pr->nr) {
					strlcpy(pr->name, anchor->name,
					    sizeof(pr->name));
					break;
				}
		} else {
			RB_FOREACH(anchor, pf_anchor_node,
			    &ruleset->anchor->children)
				if (nr++ == pr->nr) {
					strlcpy(pr->name, anchor->name,
					    sizeof(pr->name));
					break;
				}
		}
		if (!pr->name[0])
			error = EBUSY;
		break;
	}

	case DIOCRCLRTABLES: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != 0) {
			error = ENODEV;
			break;
		}
		error = pfr_clr_tables(&io->pfrio_table, &io->pfrio_ndel,
		    io->pfrio_flags | PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRADDTABLES: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_table)) {
			error = ENODEV;
			break;
		}
		error = pfr_add_tables(io->pfrio_buffer, io->pfrio_size,
		    &io->pfrio_nadd, io->pfrio_flags | PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRDELTABLES: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_table)) {
			error = ENODEV;
			break;
		}
		error = pfr_del_tables(io->pfrio_buffer, io->pfrio_size,
		    &io->pfrio_ndel, io->pfrio_flags | PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRGETTABLES: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_table)) {
			error = ENODEV;
			break;
		}
		error = pfr_get_tables(&io->pfrio_table, io->pfrio_buffer,
		    &io->pfrio_size, io->pfrio_flags | PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRGETTSTATS: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_tstats)) {
			error = ENODEV;
			break;
		}
		error = pfr_get_tstats(&io->pfrio_table, io->pfrio_buffer,
		    &io->pfrio_size, io->pfrio_flags | PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRCLRTSTATS: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_table)) {
			error = ENODEV;
			break;
		}
		error = pfr_clr_tstats(io->pfrio_buffer, io->pfrio_size,
		    &io->pfrio_nzero, io->pfrio_flags | PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRSETTFLAGS: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_table)) {
			error = ENODEV;
			break;
		}
		error = pfr_set_tflags(io->pfrio_buffer, io->pfrio_size,
		    io->pfrio_setflag, io->pfrio_clrflag, &io->pfrio_nchange,
		    &io->pfrio_ndel, io->pfrio_flags | PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRCLRADDRS: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != 0) {
			error = ENODEV;
			break;
		}
		error = pfr_clr_addrs(&io->pfrio_table, &io->pfrio_ndel,
		    io->pfrio_flags | PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRADDADDRS: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_addr)) {
			error = ENODEV;
			break;
		}
		error = pfr_add_addrs(&io->pfrio_table, io->pfrio_buffer,
		    io->pfrio_size, &io->pfrio_nadd, io->pfrio_flags |
		    PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRDELADDRS: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_addr)) {
			error = ENODEV;
			break;
		}
		error = pfr_del_addrs(&io->pfrio_table, io->pfrio_buffer,
		    io->pfrio_size, &io->pfrio_ndel, io->pfrio_flags |
		    PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRSETADDRS: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_addr)) {
			error = ENODEV;
			break;
		}
		error = pfr_set_addrs(&io->pfrio_table, io->pfrio_buffer,
		    io->pfrio_size, &io->pfrio_size2, &io->pfrio_nadd,
		    &io->pfrio_ndel, &io->pfrio_nchange, io->pfrio_flags |
		    PFR_FLAG_USERIOCTL, 0);
		break;
	}

	case DIOCRGETADDRS: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_addr)) {
			error = ENODEV;
			break;
		}
		error = pfr_get_addrs(&io->pfrio_table, io->pfrio_buffer,
		    &io->pfrio_size, io->pfrio_flags | PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRGETASTATS: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_astats)) {
			error = ENODEV;
			break;
		}
		error = pfr_get_astats(&io->pfrio_table, io->pfrio_buffer,
		    &io->pfrio_size, io->pfrio_flags | PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRCLRASTATS: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_addr)) {
			error = ENODEV;
			break;
		}
		error = pfr_clr_astats(&io->pfrio_table, io->pfrio_buffer,
		    io->pfrio_size, &io->pfrio_nzero, io->pfrio_flags |
		    PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRTSTADDRS: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_addr)) {
			error = ENODEV;
			break;
		}
		error = pfr_tst_addrs(&io->pfrio_table, io->pfrio_buffer,
		    io->pfrio_size, &io->pfrio_nmatch, io->pfrio_flags |
		    PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCRINADEFINE: {
		struct pfioc_table *io = (struct pfioc_table *)addr;

		if (io->pfrio_esize != sizeof(struct pfr_addr)) {
			error = ENODEV;
			break;
		}
		error = pfr_ina_define(&io->pfrio_table, io->pfrio_buffer,
		    io->pfrio_size, &io->pfrio_nadd, &io->pfrio_naddr,
		    io->pfrio_ticket, io->pfrio_flags | PFR_FLAG_USERIOCTL);
		break;
	}

	case DIOCOSFPADD: {
		struct pf_osfp_ioctl *io = (struct pf_osfp_ioctl *)addr;
		error = pf_osfp_add(io);
		break;
	}

	case DIOCOSFPGET: {
		struct pf_osfp_ioctl *io = (struct pf_osfp_ioctl *)addr;
		error = pf_osfp_get(io);
		break;
	}

	case DIOCXBEGIN: {
		struct pfioc_trans	*io = (struct pfioc_trans *)addr;
		struct pfioc_trans_e	*ioe;
		struct pfr_table	*table;
		int			 i;

		if (io->esize != sizeof(*ioe)) {
			error = ENODEV;
			goto fail;
		}
		ioe = (struct pfioc_trans_e *)malloc(sizeof(*ioe),
		    M_TEMP, M_WAITOK);
		table = (struct pfr_table *)malloc(sizeof(*table),
		    M_TEMP, M_WAITOK);
		for (i = 0; i < io->size; i++) {
			if (copyin(io->array+i, ioe, sizeof(*ioe))) {
				free(table, M_TEMP);
				free(ioe, M_TEMP);
				error = EFAULT;
				goto fail;
			}
			switch (ioe->rs_num) {
#ifdef ALTQ
			case PF_RULESET_ALTQ:
				if (ioe->anchor[0]) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					error = EINVAL;
					goto fail;
				}
				if ((error = pf_begin_altq(&ioe->ticket))) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					goto fail;
				}
				break;
#endif /* ALTQ */
			case PF_RULESET_TABLE:
				bzero(table, sizeof(*table));
				strlcpy(table->pfrt_anchor, ioe->anchor,
				    sizeof(table->pfrt_anchor));
				if ((error = pfr_ina_begin(table,
				    &ioe->ticket, NULL, 0))) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					goto fail;
				}
				break;
			default:
				if ((error = pf_begin_rules(&ioe->ticket,
				    ioe->rs_num, ioe->anchor))) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					goto fail;
				}
				break;
			}
			if (copyout(ioe, io->array+i, sizeof(io->array[i]))) {
				free(table, M_TEMP);
				free(ioe, M_TEMP);
				error = EFAULT;
				goto fail;
			}
		}
		free(table, M_TEMP);
		free(ioe, M_TEMP);
		break;
	}

	case DIOCXROLLBACK: {
		struct pfioc_trans	*io = (struct pfioc_trans *)addr;
		struct pfioc_trans_e	*ioe;
		struct pfr_table	*table;
		int			 i;

		if (io->esize != sizeof(*ioe)) {
			error = ENODEV;
			goto fail;
		}
		ioe = (struct pfioc_trans_e *)malloc(sizeof(*ioe),
		    M_TEMP, M_WAITOK);
		table = (struct pfr_table *)malloc(sizeof(*table),
		    M_TEMP, M_WAITOK);
		for (i = 0; i < io->size; i++) {
			if (copyin(io->array+i, ioe, sizeof(*ioe))) {
				free(table, M_TEMP);
				free(ioe, M_TEMP);
				error = EFAULT;
				goto fail;
			}
			switch (ioe->rs_num) {
#ifdef ALTQ
			case PF_RULESET_ALTQ:
				if (ioe->anchor[0]) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					error = EINVAL;
					goto fail;
				}
				if ((error = pf_rollback_altq(ioe->ticket))) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					goto fail; /* really bad */
				}
				break;
#endif /* ALTQ */
			case PF_RULESET_TABLE:
				bzero(table, sizeof(*table));
				strlcpy(table->pfrt_anchor, ioe->anchor,
				    sizeof(table->pfrt_anchor));
				if ((error = pfr_ina_rollback(table,
				    ioe->ticket, NULL, 0))) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					goto fail; /* really bad */
				}
				break;
			default:
				if ((error = pf_rollback_rules(ioe->ticket,
				    ioe->rs_num, ioe->anchor))) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					goto fail; /* really bad */
				}
				break;
			}
		}
		free(table, M_TEMP);
		free(ioe, M_TEMP);
		break;
	}

	case DIOCXCOMMIT: {
		struct pfioc_trans	*io = (struct pfioc_trans *)addr;
		struct pfioc_trans_e	*ioe;
		struct pfr_table	*table;
		struct pf_ruleset	*rs;
		int			 i;

		if (io->esize != sizeof(*ioe)) {
			error = ENODEV;
			goto fail;
		}
		ioe = (struct pfioc_trans_e *)malloc(sizeof(*ioe),
		    M_TEMP, M_WAITOK);
		table = (struct pfr_table *)malloc(sizeof(*table),
		    M_TEMP, M_WAITOK);
		/* first makes sure everything will succeed */
		for (i = 0; i < io->size; i++) {
			if (copyin(io->array+i, ioe, sizeof(*ioe))) {
				free(table, M_TEMP);
				free(ioe, M_TEMP);
				error = EFAULT;
				goto fail;
			}
			switch (ioe->rs_num) {
#ifdef ALTQ
			case PF_RULESET_ALTQ:
				if (ioe->anchor[0]) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					error = EINVAL;
					goto fail;
				}
				if (!altqs_inactive_open || ioe->ticket !=
				    ticket_altqs_inactive) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					error = EBUSY;
					goto fail;
				}
				break;
#endif /* ALTQ */
			case PF_RULESET_TABLE:
				rs = pf_find_ruleset(ioe->anchor);
				if (rs == NULL || !rs->topen || ioe->ticket !=
				     rs->tticket) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					error = EBUSY;
					goto fail;
				}
				break;
			default:
				if (ioe->rs_num < 0 || ioe->rs_num >=
				    PF_RULESET_MAX) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					error = EINVAL;
					goto fail;
				}
				rs = pf_find_ruleset(ioe->anchor);
				if (rs == NULL ||
				    !rs->rules[ioe->rs_num].inactive.open ||
				    rs->rules[ioe->rs_num].inactive.ticket !=
				    ioe->ticket) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					error = EBUSY;
					goto fail;
				}
				break;
			}
		}
		/* now do the commit - no errors should happen here */
		for (i = 0; i < io->size; i++) {
			if (copyin(io->array+i, ioe, sizeof(*ioe))) {
				free(table, M_TEMP);
				free(ioe, M_TEMP);
				error = EFAULT;
				goto fail;
			}
			switch (ioe->rs_num) {
#ifdef ALTQ
			case PF_RULESET_ALTQ:
				if ((error = pf_commit_altq(ioe->ticket))) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					goto fail; /* really bad */
				}
				break;
#endif /* ALTQ */
			case PF_RULESET_TABLE:
				bzero(table, sizeof(*table));
				strlcpy(table->pfrt_anchor, ioe->anchor,
				    sizeof(table->pfrt_anchor));
				if ((error = pfr_ina_commit(table, ioe->ticket,
				    NULL, NULL, 0))) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					goto fail; /* really bad */
				}
				break;
			default:
				if ((error = pf_commit_rules(ioe->ticket,
				    ioe->rs_num, ioe->anchor))) {
					free(table, M_TEMP);
					free(ioe, M_TEMP);
					goto fail; /* really bad */
				}
				break;
			}
		}
		free(table, M_TEMP);
		free(ioe, M_TEMP);
		break;
	}

	case DIOCGETSRCNODES: {
		struct pfioc_src_nodes	*psn = (struct pfioc_src_nodes *)addr;
		struct pf_src_node	*n, *p, *pstore;
		u_int32_t		 nr = 0;
		int			 space = psn->psn_len;

		if (space == 0) {
			RB_FOREACH(n, pf_src_tree, &tree_src_tracking)
				nr++;
			psn->psn_len = sizeof(struct pf_src_node) * nr;
			break;
		}

		pstore = malloc(sizeof(*pstore), M_TEMP, M_WAITOK);

		p = psn->psn_src_nodes;
		RB_FOREACH(n, pf_src_tree, &tree_src_tracking) {
			int	secs = time_second, diff;

			if ((nr + 1) * sizeof(*p) > (unsigned)psn->psn_len)
				break;

			bcopy(n, pstore, sizeof(*pstore));
			if (n->rule.ptr != NULL)
				pstore->rule.nr = n->rule.ptr->nr;
			pstore->creation = secs - pstore->creation;
			if (pstore->expire > secs)
				pstore->expire -= secs;
			else
				pstore->expire = 0;

			/* adjust the connection rate estimate */
			diff = secs - n->conn_rate.last;
			if (diff >= n->conn_rate.seconds)
				pstore->conn_rate.count = 0;
			else
				pstore->conn_rate.count -=
				    n->conn_rate.count * diff /
				    n->conn_rate.seconds;

			error = copyout(pstore, p, sizeof(*p));
			if (error) {
				free(pstore, M_TEMP);
				goto fail;
			}
			p++;
			nr++;
		}
		psn->psn_len = sizeof(struct pf_src_node) * nr;

		free(pstore, M_TEMP);
		break;
	}

	case DIOCCLRSRCNODES: {
		struct pf_src_node	*n;
		struct pf_state		*state;

		RB_FOREACH(state, pf_state_tree_id, &tree_id) {
			state->src_node = NULL;
			state->nat_src_node = NULL;
		}
		RB_FOREACH(n, pf_src_tree, &tree_src_tracking) {
			n->expire = 1;
			n->states = 0;
		}
		pf_purge_expired_src_nodes(1);
		pf_status.src_nodes = 0;
		break;
	}

	case DIOCKILLSRCNODES: {
		struct pf_src_node	*sn;
		struct pf_state		*ps;
		struct pfioc_src_node_kill *psnk = \
			(struct pfioc_src_node_kill *) addr;
		int			killed = 0;

		RB_FOREACH(sn, pf_src_tree, &tree_src_tracking) {
        		if (PF_MATCHA(psnk->psnk_src.neg, \
				      &psnk->psnk_src.addr.v.a.addr, \
				      &psnk->psnk_src.addr.v.a.mask, \
				      &sn->addr, sn->af) &&
			    PF_MATCHA(psnk->psnk_dst.neg, \
				      &psnk->psnk_dst.addr.v.a.addr, \
				      &psnk->psnk_dst.addr.v.a.mask, \
				      &sn->raddr, sn->af)) {
				/* Handle state to src_node linkage */
				if (sn->states != 0) {
					RB_FOREACH(ps, pf_state_tree_id, 
					    &tree_id) {
						if (ps->src_node == sn)
							ps->src_node = NULL;
						if (ps->nat_src_node == sn)
							ps->nat_src_node = NULL;
					}
					sn->states = 0;
				}
				sn->expire = 1;
				killed++;
			}
		}

		if (killed > 0)
			pf_purge_expired_src_nodes(1);

		psnk->psnk_af = killed;
		break;
	}

	case DIOCSETHOSTID: {
		u_int32_t	*hid = (u_int32_t *)addr;

		if (*hid == 0)
			pf_status.hostid = cprng_fast32();
		else
			pf_status.hostid = *hid;
		break;
	}

	case DIOCOSFPFLUSH:
		pf_osfp_flush();
		break;

	case DIOCIGETIFACES: {
		struct pfioc_iface *io = (struct pfioc_iface *)addr;

		if (io->pfiio_esize != sizeof(struct pfi_kif)) {
			error = ENODEV;
			break;
		}
		error = pfi_get_ifaces(io->pfiio_name, io->pfiio_buffer,
		    &io->pfiio_size);
		break;
	}

	case DIOCSETIFFLAG: {
		struct pfioc_iface *io = (struct pfioc_iface *)addr;

		error = pfi_set_flags(io->pfiio_name, io->pfiio_flags);
		break;
	}

	case DIOCCLRIFFLAG: {
		struct pfioc_iface *io = (struct pfioc_iface *)addr;

		error = pfi_clear_flags(io->pfiio_name, io->pfiio_flags);
		break;
	}

	case DIOCSETLCK: {
		pf_state_lock = *(uint32_t*)addr;
		break;
	}

	default:
		error = ENODEV;
		break;
	}
fail:
	splx(s);
	if (flags & FWRITE)
		rw_exit_write(&pf_consistency_lock);
	else
		rw_exit_read(&pf_consistency_lock);
	return (error);
}

#ifdef __NetBSD__
#ifdef INET
static int
pfil4_wrapper(void *arg, struct mbuf **mp, struct ifnet *ifp, int dir)
{
	int error;

	/*
	 * ensure that mbufs are writable beforehand
	 * as it's assumed by pf code.
	 * ip hdr (60 bytes) + tcp hdr (60 bytes) should be enough.
	 * XXX inefficient
	 */
	error = m_makewritable(mp, 0, 60 + 60, M_DONTWAIT);
	if (error) {
		m_freem(*mp);
		*mp = NULL;
		return error;
	}

	/*
	 * If the packet is out-bound, we can't delay checksums
	 * here.  For in-bound, the checksum has already been
	 * validated.
	 */
	if (dir == PFIL_OUT) {
		if ((*mp)->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4)) {
			in_delayed_cksum(*mp);
			(*mp)->m_pkthdr.csum_flags &=
			    ~(M_CSUM_TCPv4|M_CSUM_UDPv4);
		}
	}

	if (pf_test(dir == PFIL_OUT ? PF_OUT : PF_IN, ifp, mp, NULL)
	    != PF_PASS) {
		m_freem(*mp);
		*mp = NULL;
		return EHOSTUNREACH;
	}

	/*
	 * we're not compatible with fast-forward.
	 */

	if (dir == PFIL_IN && *mp) {
		(*mp)->m_flags &= ~M_CANFASTFWD;
	}

	return (0);
}
#endif /* INET */

#ifdef INET6
static int
pfil6_wrapper(void *arg, struct mbuf **mp, struct ifnet *ifp, int dir)
{
	int error;

	/*
	 * ensure that mbufs are writable beforehand
	 * as it's assumed by pf code.
	 * XXX inefficient
	 */
	error = m_makewritable(mp, 0, M_COPYALL, M_DONTWAIT);
	if (error) {
		m_freem(*mp);
		*mp = NULL;
		return error;
	}

	/*
	 * If the packet is out-bound, we can't delay checksums
	 * here.  For in-bound, the checksum has already been
	 * validated.
	 */
	if (dir == PFIL_OUT) {
		if ((*mp)->m_pkthdr.csum_flags & (M_CSUM_TCPv6|M_CSUM_UDPv6)) {
			in6_delayed_cksum(*mp);
			(*mp)->m_pkthdr.csum_flags &=
			    ~(M_CSUM_TCPv6|M_CSUM_UDPv6);
		}
	}

	if (pf_test6(dir == PFIL_OUT ? PF_OUT : PF_IN, ifp, mp, NULL)
	    != PF_PASS) {
		m_freem(*mp);
		*mp = NULL;
		return EHOSTUNREACH;
	} else
		return (0);
}
#endif /* INET6 */

static int
pf_pfil_attach(void)
{
	pfil_head_t *ph_inet;
#ifdef INET6
	pfil_head_t *ph_inet6;
#endif /* INET6 */
	int error;

	if (pf_pfil_attached)
		return (EBUSY);

	ph_inet = pfil_head_get(PFIL_TYPE_AF, (void *)AF_INET);
	if (ph_inet)
		error = pfil_add_hook((void *)pfil4_wrapper, NULL,
		    PFIL_IN|PFIL_OUT, ph_inet);
	else
		error = ENOENT;
	if (error)
		return (error);

#ifdef INET6
	ph_inet6 = pfil_head_get(PFIL_TYPE_AF, (void *)AF_INET6);
	if (ph_inet6)
		error = pfil_add_hook((void *)pfil6_wrapper, NULL,
		    PFIL_IN|PFIL_OUT, ph_inet6);
	else
		error = ENOENT;
	if (error)
		goto bad;
#endif /* INET6 */

	pf_pfil_attached = 1;

	return (0);

#ifdef INET6
bad:
	pfil_remove_hook(pfil4_wrapper, NULL, PFIL_IN|PFIL_OUT, ph_inet);
#endif /* INET6 */

	return (error);
}

static int
pf_pfil_detach(void)
{
	pfil_head_t *ph_inet;
#ifdef INET6
	pfil_head_t *ph_inet6;
#endif /* INET6 */

	if (pf_pfil_attached == 0)
		return (EBUSY);

	ph_inet = pfil_head_get(PFIL_TYPE_AF, (void *)AF_INET);
	if (ph_inet)
		pfil_remove_hook((void *)pfil4_wrapper, NULL,
		    PFIL_IN|PFIL_OUT, ph_inet);
#ifdef INET6
	ph_inet6 = pfil_head_get(PFIL_TYPE_AF, (void *)AF_INET6);
	if (ph_inet6)
		pfil_remove_hook((void *)pfil6_wrapper, NULL,
		    PFIL_IN|PFIL_OUT, ph_inet6);
#endif /* INET6 */
	pf_pfil_attached = 0;

	return (0);
}
#endif /* __NetBSD__ */

#if defined(__NetBSD__)
MODULE(MODULE_CLASS_DRIVER, pf, "bpf");

static int
pf_modcmd(modcmd_t cmd, void *opaque)
{
#ifdef _MODULE
	extern void pflogattach(int);
	extern void pflogdetach(void);

	devmajor_t cmajor = NODEVMAJOR, bmajor = NODEVMAJOR;
	int err;

	switch (cmd) {
	case MODULE_CMD_INIT:
		err = devsw_attach("pf", NULL, &bmajor, &pf_cdevsw, &cmajor);
		if (err)
			return err;
		pfattach(1);
		pflogattach(1);
		return 0;
	case MODULE_CMD_FINI:
		if (pf_status.running) {
			return EBUSY;
		} else {
			pfdetach();
			pflogdetach();
			return devsw_detach(NULL, &pf_cdevsw);
		}
	default:
		return ENOTTY;
	}
#else
	if (cmd == MODULE_CMD_INIT)
		return 0;
	return ENOTTY;
#endif
}
#endif
