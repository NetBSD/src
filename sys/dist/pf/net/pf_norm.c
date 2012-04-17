/*	$NetBSD: pf_norm.c,v 1.25.2.1 2012/04/17 00:08:14 yamt Exp $	*/
/*	$OpenBSD: pf_norm.c,v 1.109 2007/05/28 17:16:39 henning Exp $ */

/*
 * Copyright 2001 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pf_norm.c,v 1.25.2.1 2012/04/17 00:08:14 yamt Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#endif

#include "pflog.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/filio.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/pool.h>

#ifdef __NetBSD__
#include <sys/rnd.h>
#include <sys/cprng.h>
#else
#include <dev/rndvar.h>
#endif /* !__NetBSD__ */
#include <net/if.h>
#include <net/if_types.h>
#include <net/bpf.h>
#include <net/route.h>
#include <net/if_pflog.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_seq.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

#ifdef INET6
#include <netinet/ip6.h>
#endif /* INET6 */

#include <net/pfvar.h>

struct pf_frent {
	LIST_ENTRY(pf_frent) fr_next;
	struct ip *fr_ip;
	struct mbuf *fr_m;
};

struct pf_frcache {
	LIST_ENTRY(pf_frcache) fr_next;
	uint16_t	fr_off;
	uint16_t	fr_end;
};

#define PFFRAG_SEENLAST	0x0001		/* Seen the last fragment for this */
#define PFFRAG_NOBUFFER	0x0002		/* Non-buffering fragment cache */
#define PFFRAG_DROP	0x0004		/* Drop all fragments */
#define BUFFER_FRAGMENTS(fr)	(!((fr)->fr_flags & PFFRAG_NOBUFFER))

struct pf_fragment {
	RB_ENTRY(pf_fragment) fr_entry;
	TAILQ_ENTRY(pf_fragment) frag_next;
	struct in_addr	fr_src;
	struct in_addr	fr_dst;
	u_int8_t	fr_p;		/* protocol of this fragment */
	u_int8_t	fr_flags;	/* status flags */
	u_int16_t	fr_id;		/* fragment id for reassemble */
	u_int16_t	fr_max;		/* fragment data max */
	u_int32_t	fr_timeout;
#define fr_queue	fr_u.fru_queue
#define fr_cache	fr_u.fru_cache
	union {
		LIST_HEAD(pf_fragq, pf_frent) fru_queue;	/* buffering */
		LIST_HEAD(pf_cacheq, pf_frcache) fru_cache;	/* non-buf */
	} fr_u;
};

TAILQ_HEAD(pf_fragqueue, pf_fragment)	pf_fragqueue;
TAILQ_HEAD(pf_cachequeue, pf_fragment)	pf_cachequeue;

static __inline int	 pf_frag_compare(struct pf_fragment *,
			    struct pf_fragment *);
RB_HEAD(pf_frag_tree, pf_fragment)	pf_frag_tree, pf_cache_tree;
RB_PROTOTYPE(pf_frag_tree, pf_fragment, fr_entry, pf_frag_compare);
RB_GENERATE(pf_frag_tree, pf_fragment, fr_entry, pf_frag_compare);

/* Private prototypes */
void			 pf_ip2key(struct pf_fragment *, struct ip *);
void			 pf_remove_fragment(struct pf_fragment *);
void			 pf_flush_fragments(void);
void			 pf_free_fragment(struct pf_fragment *);
struct pf_fragment	*pf_find_fragment(struct ip *, struct pf_frag_tree *);
struct mbuf		*pf_reassemble(struct mbuf **, struct pf_fragment **,
			    struct pf_frent *, int);
struct mbuf		*pf_fragcache(struct mbuf **, struct ip*,
			    struct pf_fragment **, int, int, int *);
int			 pf_normalize_tcpopt(struct pf_rule *, struct mbuf *,
			    struct tcphdr *, int);

#define	DPFPRINTF(x) do {				\
	if (pf_status.debug >= PF_DEBUG_MISC) {		\
		printf("%s: ", __func__);		\
		printf x ;				\
	}						\
} while(0)

/* Globals */
struct pool		 pf_frent_pl, pf_frag_pl, pf_cache_pl, pf_cent_pl;
struct pool		 pf_state_scrub_pl;
int			 pf_nfrents, pf_ncache;

void
pf_normalize_init(void)
{
#ifdef __NetBSD__
	pool_init(&pf_frent_pl, sizeof(struct pf_frent), 0, 0, 0, "pffrent",
	    NULL, IPL_SOFTNET);
	pool_init(&pf_frag_pl, sizeof(struct pf_fragment), 0, 0, 0, "pffrag",
	    NULL, IPL_SOFTNET);
	pool_init(&pf_cache_pl, sizeof(struct pf_fragment), 0, 0, 0,
	    "pffrcache", NULL, IPL_SOFTNET);
	pool_init(&pf_cent_pl, sizeof(struct pf_frcache), 0, 0, 0, "pffrcent",
	    NULL, IPL_SOFTNET);
	pool_init(&pf_state_scrub_pl, sizeof(struct pf_state_scrub), 0, 0, 0,
	    "pfstscr", NULL, IPL_SOFTNET);
#else
	pool_init(&pf_frent_pl, sizeof(struct pf_frent), 0, 0, 0, "pffrent",
	    NULL);
	pool_init(&pf_frag_pl, sizeof(struct pf_fragment), 0, 0, 0, "pffrag",
	    NULL);
	pool_init(&pf_cache_pl, sizeof(struct pf_fragment), 0, 0, 0,
	    "pffrcache", NULL);
	pool_init(&pf_cent_pl, sizeof(struct pf_frcache), 0, 0, 0, "pffrcent",
	    NULL);
	pool_init(&pf_state_scrub_pl, sizeof(struct pf_state_scrub), 0, 0, 0,
	    "pfstscr", NULL);
#endif /* !__NetBSD__ */

	pool_sethiwat(&pf_frag_pl, PFFRAG_FRAG_HIWAT);
	pool_sethardlimit(&pf_frent_pl, PFFRAG_FRENT_HIWAT, NULL, 0);
	pool_sethardlimit(&pf_cache_pl, PFFRAG_FRCACHE_HIWAT, NULL, 0);
	pool_sethardlimit(&pf_cent_pl, PFFRAG_FRCENT_HIWAT, NULL, 0);

	TAILQ_INIT(&pf_fragqueue);
	TAILQ_INIT(&pf_cachequeue);
}

#ifdef _MODULE
void
pf_normalize_destroy(void)
{
	pool_destroy(&pf_state_scrub_pl);
	pool_destroy(&pf_cent_pl);
	pool_destroy(&pf_cache_pl);
	pool_destroy(&pf_frag_pl);
	pool_destroy(&pf_frent_pl);
}
#endif /* _MODULE */

static __inline int
pf_frag_compare(struct pf_fragment *a, struct pf_fragment *b)
{
	int	diff;

	if ((diff = a->fr_id - b->fr_id))
		return (diff);
	else if ((diff = a->fr_p - b->fr_p))
		return (diff);
	else if (a->fr_src.s_addr < b->fr_src.s_addr)
		return (-1);
	else if (a->fr_src.s_addr > b->fr_src.s_addr)
		return (1);
	else if (a->fr_dst.s_addr < b->fr_dst.s_addr)
		return (-1);
	else if (a->fr_dst.s_addr > b->fr_dst.s_addr)
		return (1);
	return (0);
}

void
pf_purge_expired_fragments(void)
{
	struct pf_fragment	*frag;
	u_int32_t		 expire = time_second -
				    pf_default_rule.timeout[PFTM_FRAG];

	while ((frag = TAILQ_LAST(&pf_fragqueue, pf_fragqueue)) != NULL) {
		KASSERT(BUFFER_FRAGMENTS(frag));
		if (frag->fr_timeout > expire)
			break;

		DPFPRINTF(("expiring %d(%p)\n", frag->fr_id, frag));
		pf_free_fragment(frag);
	}

	while ((frag = TAILQ_LAST(&pf_cachequeue, pf_cachequeue)) != NULL) {
		KASSERT(!BUFFER_FRAGMENTS(frag));
		if (frag->fr_timeout > expire)
			break;

		DPFPRINTF(("expiring %d(%p)\n", frag->fr_id, frag));
		pf_free_fragment(frag);
		KASSERT(TAILQ_EMPTY(&pf_cachequeue) ||
		    TAILQ_LAST(&pf_cachequeue, pf_cachequeue) != frag);
	}
}

/*
 * Try to flush old fragments to make space for new ones
 */

void
pf_flush_fragments(void)
{
	struct pf_fragment	*frag;
	int			 goal;

	goal = pf_nfrents * 9 / 10;
	DPFPRINTF(("trying to free > %d frents\n",
	    pf_nfrents - goal));
	while (goal < pf_nfrents) {
		frag = TAILQ_LAST(&pf_fragqueue, pf_fragqueue);
		if (frag == NULL)
			break;
		pf_free_fragment(frag);
	}


	goal = pf_ncache * 9 / 10;
	DPFPRINTF(("trying to free > %d cache entries\n",
	    pf_ncache - goal));
	while (goal < pf_ncache) {
		frag = TAILQ_LAST(&pf_cachequeue, pf_cachequeue);
		if (frag == NULL)
			break;
		pf_free_fragment(frag);
	}
}

/* Frees the fragments and all associated entries */

void
pf_free_fragment(struct pf_fragment *frag)
{
	struct pf_frent		*frent;
	struct pf_frcache	*frcache;

	/* Free all fragments */
	if (BUFFER_FRAGMENTS(frag)) {
		for (frent = LIST_FIRST(&frag->fr_queue); frent;
		    frent = LIST_FIRST(&frag->fr_queue)) {
			LIST_REMOVE(frent, fr_next);

			m_freem(frent->fr_m);
			pool_put(&pf_frent_pl, frent);
			pf_nfrents--;
		}
	} else {
		for (frcache = LIST_FIRST(&frag->fr_cache); frcache;
		    frcache = LIST_FIRST(&frag->fr_cache)) {
			LIST_REMOVE(frcache, fr_next);

			KASSERT(LIST_EMPTY(&frag->fr_cache) ||
			    LIST_FIRST(&frag->fr_cache)->fr_off >
			    frcache->fr_end);

			pool_put(&pf_cent_pl, frcache);
			pf_ncache--;
		}
	}

	pf_remove_fragment(frag);
}

void
pf_ip2key(struct pf_fragment *key, struct ip *ip)
{
	key->fr_p = ip->ip_p;
	key->fr_id = ip->ip_id;
	key->fr_src.s_addr = ip->ip_src.s_addr;
	key->fr_dst.s_addr = ip->ip_dst.s_addr;
}

struct pf_fragment *
pf_find_fragment(struct ip *ip, struct pf_frag_tree *tree)
{
	struct pf_fragment	 key;
	struct pf_fragment	*frag;

	pf_ip2key(&key, ip);

	frag = RB_FIND(pf_frag_tree, tree, &key);
	if (frag != NULL) {
		/* XXX Are we sure we want to update the timeout? */
		frag->fr_timeout = time_second;
		if (BUFFER_FRAGMENTS(frag)) {
			TAILQ_REMOVE(&pf_fragqueue, frag, frag_next);
			TAILQ_INSERT_HEAD(&pf_fragqueue, frag, frag_next);
		} else {
			TAILQ_REMOVE(&pf_cachequeue, frag, frag_next);
			TAILQ_INSERT_HEAD(&pf_cachequeue, frag, frag_next);
		}
	}

	return (frag);
}

/* Removes a fragment from the fragment queue and frees the fragment */

void
pf_remove_fragment(struct pf_fragment *frag)
{
	if (BUFFER_FRAGMENTS(frag)) {
		RB_REMOVE(pf_frag_tree, &pf_frag_tree, frag);
		TAILQ_REMOVE(&pf_fragqueue, frag, frag_next);
		pool_put(&pf_frag_pl, frag);
	} else {
		RB_REMOVE(pf_frag_tree, &pf_cache_tree, frag);
		TAILQ_REMOVE(&pf_cachequeue, frag, frag_next);
		pool_put(&pf_cache_pl, frag);
	}
}

#define FR_IP_OFF(fr)	((ntohs((fr)->fr_ip->ip_off) & IP_OFFMASK) << 3)
struct mbuf *
pf_reassemble(struct mbuf **m0, struct pf_fragment **frag,
    struct pf_frent *frent, int mff)
{
	struct mbuf	*m = *m0, *m2;
	struct pf_frent	*frea, *next;
	struct pf_frent	*frep = NULL;
	struct ip	*ip = frent->fr_ip;
	int		 hlen = ip->ip_hl << 2;
	u_int16_t	 off = (ntohs(ip->ip_off) & IP_OFFMASK) << 3;
	u_int16_t	 ip_len = ntohs(ip->ip_len) - ip->ip_hl * 4;
	u_int16_t	 frmax = ip_len + off;

	KASSERT(*frag == NULL || BUFFER_FRAGMENTS(*frag));

	/* Strip off ip header */
	m->m_data += hlen;
	m->m_len -= hlen;

	/* Create a new reassembly queue for this packet */
	if (*frag == NULL) {
		*frag = pool_get(&pf_frag_pl, PR_NOWAIT);
		if (*frag == NULL) {
			pf_flush_fragments();
			*frag = pool_get(&pf_frag_pl, PR_NOWAIT);
			if (*frag == NULL)
				goto drop_fragment;
		}

		(*frag)->fr_flags = 0;
		(*frag)->fr_max = 0;
		(*frag)->fr_src = frent->fr_ip->ip_src;
		(*frag)->fr_dst = frent->fr_ip->ip_dst;
		(*frag)->fr_p = frent->fr_ip->ip_p;
		(*frag)->fr_id = frent->fr_ip->ip_id;
		(*frag)->fr_timeout = time_second;
		LIST_INIT(&(*frag)->fr_queue);

		RB_INSERT(pf_frag_tree, &pf_frag_tree, *frag);
		TAILQ_INSERT_HEAD(&pf_fragqueue, *frag, frag_next);

		/* We do not have a previous fragment */
		frep = NULL;
		goto insert;
	}

	/*
	 * Find a fragment after the current one:
	 *  - off contains the real shifted offset.
	 */
	LIST_FOREACH(frea, &(*frag)->fr_queue, fr_next) {
		if (FR_IP_OFF(frea) > off)
			break;
		frep = frea;
	}

	KASSERT(frep != NULL || frea != NULL);

	if (frep != NULL &&
	    FR_IP_OFF(frep) + ntohs(frep->fr_ip->ip_len) - frep->fr_ip->ip_hl *
	    4 > off)
	{
		u_int16_t	precut;

		precut = FR_IP_OFF(frep) + ntohs(frep->fr_ip->ip_len) -
		    frep->fr_ip->ip_hl * 4 - off;
		if (precut >= ip_len)
			goto drop_fragment;
		m_adj(frent->fr_m, precut);
		DPFPRINTF(("overlap -%d\n", precut));
		/* Enforce 8 byte boundaries */
		ip->ip_off = htons(ntohs(ip->ip_off) + (precut >> 3));
		off = (ntohs(ip->ip_off) & IP_OFFMASK) << 3;
		ip_len -= precut;
		ip->ip_len = htons(ip_len);
	}

	for (; frea != NULL && ip_len + off > FR_IP_OFF(frea);
	    frea = next)
	{
		u_int16_t	aftercut;

		aftercut = ip_len + off - FR_IP_OFF(frea);
		DPFPRINTF(("adjust overlap %d\n", aftercut));
		if (aftercut < ntohs(frea->fr_ip->ip_len) - frea->fr_ip->ip_hl
		    * 4)
		{
			frea->fr_ip->ip_len =
			    htons(ntohs(frea->fr_ip->ip_len) - aftercut);
			frea->fr_ip->ip_off = htons(ntohs(frea->fr_ip->ip_off) +
			    (aftercut >> 3));
			m_adj(frea->fr_m, aftercut);
			break;
		}

		/* This fragment is completely overlapped, lose it */
		next = LIST_NEXT(frea, fr_next);
		m_freem(frea->fr_m);
		LIST_REMOVE(frea, fr_next);
		pool_put(&pf_frent_pl, frea);
		pf_nfrents--;
	}

 insert:
	/* Update maximum data size */
	if ((*frag)->fr_max < frmax)
		(*frag)->fr_max = frmax;
	/* This is the last segment */
	if (!mff)
		(*frag)->fr_flags |= PFFRAG_SEENLAST;

	if (frep == NULL)
		LIST_INSERT_HEAD(&(*frag)->fr_queue, frent, fr_next);
	else
		LIST_INSERT_AFTER(frep, frent, fr_next);

	/* Check if we are completely reassembled */
	if (!((*frag)->fr_flags & PFFRAG_SEENLAST))
		return (NULL);

	/* Check if we have all the data */
	off = 0;
	for (frep = LIST_FIRST(&(*frag)->fr_queue); frep; frep = next) {
		next = LIST_NEXT(frep, fr_next);

		off += ntohs(frep->fr_ip->ip_len) - frep->fr_ip->ip_hl * 4;
		if (off < (*frag)->fr_max &&
		    (next == NULL || FR_IP_OFF(next) != off))
		{
			DPFPRINTF(("missing fragment at %d, next %d, max %d\n",
			    off, next == NULL ? -1 : FR_IP_OFF(next),
			    (*frag)->fr_max));
			return (NULL);
		}
	}
	DPFPRINTF(("%d < %d?\n", off, (*frag)->fr_max));
	if (off < (*frag)->fr_max)
		return (NULL);

	/* We have all the data */
	frent = LIST_FIRST(&(*frag)->fr_queue);
	KASSERT(frent != NULL);
	if ((frent->fr_ip->ip_hl << 2) + off > IP_MAXPACKET) {
		DPFPRINTF(("drop: too big: %d\n", off));
		pf_free_fragment(*frag);
		*frag = NULL;
		return (NULL);
	}
	next = LIST_NEXT(frent, fr_next);

	/* Magic from ip_input */
	ip = frent->fr_ip;
	m = frent->fr_m;
	m2 = m->m_next;
	m->m_next = NULL;
	m_cat(m, m2);
	pool_put(&pf_frent_pl, frent);
	pf_nfrents--;
	for (frent = next; frent != NULL; frent = next) {
		next = LIST_NEXT(frent, fr_next);

		m2 = frent->fr_m;
		pool_put(&pf_frent_pl, frent);
		pf_nfrents--;
		m_cat(m, m2);
	}

	ip->ip_src = (*frag)->fr_src;
	ip->ip_dst = (*frag)->fr_dst;

	/* Remove from fragment queue */
	pf_remove_fragment(*frag);
	*frag = NULL;

	hlen = ip->ip_hl << 2;
	ip->ip_len = htons(off + hlen);
	m->m_len += hlen;
	m->m_data -= hlen;

	/* some debugging cruft by sklower, below, will go away soon */
	/* XXX this should be done elsewhere */
	if (m->m_flags & M_PKTHDR) {
		int plen = 0;
		for (m2 = m; m2; m2 = m2->m_next)
			plen += m2->m_len;
		m->m_pkthdr.len = plen;
#ifdef __NetBSD__
		m->m_pkthdr.csum_flags = 0;
#endif /* __NetBSD__ */
	}

	DPFPRINTF(("complete: %p(%d)\n", m, ntohs(ip->ip_len)));
	return (m);

 drop_fragment:
	/* Oops - fail safe - drop packet */
	pool_put(&pf_frent_pl, frent);
	pf_nfrents--;
	m_freem(m);
	return (NULL);
}

struct mbuf *
pf_fragcache(struct mbuf **m0, struct ip *h, struct pf_fragment **frag, int mff,
    int drop, int *nomem)
{
	struct mbuf		*m = *m0;
	struct pf_frcache	*frp, *fra, *cur = NULL;
	int			 ip_len = ntohs(h->ip_len) - (h->ip_hl << 2);
	u_int16_t		 off = ntohs(h->ip_off) << 3;
	u_int16_t		 frmax = ip_len + off;
	int			 hosed = 0;

	KASSERT(*frag == NULL || !BUFFER_FRAGMENTS(*frag));

	/* Create a new range queue for this packet */
	if (*frag == NULL) {
		*frag = pool_get(&pf_cache_pl, PR_NOWAIT);
		if (*frag == NULL) {
			pf_flush_fragments();
			*frag = pool_get(&pf_cache_pl, PR_NOWAIT);
			if (*frag == NULL)
				goto no_mem;
		}

		/* Get an entry for the queue */
		cur = pool_get(&pf_cent_pl, PR_NOWAIT);
		if (cur == NULL) {
			pool_put(&pf_cache_pl, *frag);
			*frag = NULL;
			goto no_mem;
		}
		pf_ncache++;

		(*frag)->fr_flags = PFFRAG_NOBUFFER;
		(*frag)->fr_max = 0;
		(*frag)->fr_src = h->ip_src;
		(*frag)->fr_dst = h->ip_dst;
		(*frag)->fr_p = h->ip_p;
		(*frag)->fr_id = h->ip_id;
		(*frag)->fr_timeout = time_second;

		cur->fr_off = off;
		cur->fr_end = frmax;
		LIST_INIT(&(*frag)->fr_cache);
		LIST_INSERT_HEAD(&(*frag)->fr_cache, cur, fr_next);

		RB_INSERT(pf_frag_tree, &pf_cache_tree, *frag);
		TAILQ_INSERT_HEAD(&pf_cachequeue, *frag, frag_next);

		DPFPRINTF(("fragcache[%d]: new %d-%d\n", h->ip_id, off, frmax));

		goto pass;
	}

	/*
	 * Find a fragment after the current one:
	 *  - off contains the real shifted offset.
	 */
	frp = NULL;
	LIST_FOREACH(fra, &(*frag)->fr_cache, fr_next) {
		if (fra->fr_off > off)
			break;
		frp = fra;
	}

	KASSERT(frp != NULL || fra != NULL);

	if (frp != NULL) {
		int	precut;

		precut = frp->fr_end - off;
		if (precut >= ip_len) {
			/* Fragment is entirely a duplicate */
			DPFPRINTF(("fragcache[%d]: dead (%d-%d) %d-%d\n",
			    h->ip_id, frp->fr_off, frp->fr_end, off, frmax));
			goto drop_fragment;
		}
		if (precut == 0) {
			/* They are adjacent.  Fixup cache entry */
			DPFPRINTF(("fragcache[%d]: adjacent (%d-%d) %d-%d\n",
			    h->ip_id, frp->fr_off, frp->fr_end, off, frmax));
			frp->fr_end = frmax;
		} else if (precut > 0) {
			/* The first part of this payload overlaps with a
			 * fragment that has already been passed.
			 * Need to trim off the first part of the payload.
			 * But to do so easily, we need to create another
			 * mbuf to throw the original header into.
			 */

			DPFPRINTF(("fragcache[%d]: chop %d (%d-%d) %d-%d\n",
			    h->ip_id, precut, frp->fr_off, frp->fr_end, off,
			    frmax));

			off += precut;
			frmax -= precut;
			/* Update the previous frag to encompass this one */
			frp->fr_end = frmax;

			if (!drop) {
				/* XXX Optimization opportunity
				 * This is a very heavy way to trim the payload.
				 * we could do it much faster by diddling mbuf
				 * internals but that would be even less legible
				 * than this mbuf magic.  For my next trick,
				 * I'll pull a rabbit out of my laptop.
				 */
				*m0 = m_dup(m, 0, h->ip_hl << 2, M_NOWAIT);
				if (*m0 == NULL)
					goto no_mem;
				KASSERT((*m0)->m_next == NULL);
				m_adj(m, precut + (h->ip_hl << 2));
				m_cat(*m0, m);
				m = *m0;
				if (m->m_flags & M_PKTHDR) {
					int plen = 0;
					struct mbuf *t;
					for (t = m; t; t = t->m_next)
						plen += t->m_len;
					m->m_pkthdr.len = plen;
				}


				h = mtod(m, struct ip *);


				KASSERT((int)m->m_len ==
				    ntohs(h->ip_len) - precut);
				h->ip_off = htons(ntohs(h->ip_off) +
				    (precut >> 3));
				h->ip_len = htons(ntohs(h->ip_len) - precut);
			} else {
				hosed++;
			}
		} else {
			/* There is a gap between fragments */

			DPFPRINTF(("fragcache[%d]: gap %d (%d-%d) %d-%d\n",
			    h->ip_id, -precut, frp->fr_off, frp->fr_end, off,
			    frmax));

			cur = pool_get(&pf_cent_pl, PR_NOWAIT);
			if (cur == NULL)
				goto no_mem;
			pf_ncache++;

			cur->fr_off = off;
			cur->fr_end = frmax;
			LIST_INSERT_AFTER(frp, cur, fr_next);
		}
	}

	if (fra != NULL) {
		int	aftercut;
		int	merge = 0;

		aftercut = frmax - fra->fr_off;
		if (aftercut == 0) {
			/* Adjacent fragments */
			DPFPRINTF(("fragcache[%d]: adjacent %d-%d (%d-%d)\n",
			    h->ip_id, off, frmax, fra->fr_off, fra->fr_end));
			fra->fr_off = off;
			merge = 1;
		} else if (aftercut > 0) {
			/* Need to chop off the tail of this fragment */
			DPFPRINTF(("fragcache[%d]: chop %d %d-%d (%d-%d)\n",
			    h->ip_id, aftercut, off, frmax, fra->fr_off,
			    fra->fr_end));
			fra->fr_off = off;
			frmax -= aftercut;

			merge = 1;

			if (!drop) {
				m_adj(m, -aftercut);
				if (m->m_flags & M_PKTHDR) {
					int plen = 0;
					struct mbuf *t;
					for (t = m; t; t = t->m_next)
						plen += t->m_len;
					m->m_pkthdr.len = plen;
				}
				h = mtod(m, struct ip *);
				KASSERT((int)m->m_len ==
				    ntohs(h->ip_len) - aftercut);
				h->ip_len = htons(ntohs(h->ip_len) - aftercut);
			} else {
				hosed++;
			}
		} else if (frp == NULL) {
			/* There is a gap between fragments */
			DPFPRINTF(("fragcache[%d]: gap %d %d-%d (%d-%d)\n",
			    h->ip_id, -aftercut, off, frmax, fra->fr_off,
			    fra->fr_end));

			cur = pool_get(&pf_cent_pl, PR_NOWAIT);
			if (cur == NULL)
				goto no_mem;
			pf_ncache++;

			cur->fr_off = off;
			cur->fr_end = frmax;
			LIST_INSERT_BEFORE(fra, cur, fr_next);
		}


		/* Need to glue together two separate fragment descriptors */
		if (merge) {
			if (cur && fra->fr_off <= cur->fr_end) {
				/* Need to merge in a previous 'cur' */
				DPFPRINTF(("fragcache[%d]: adjacent(merge "
				    "%d-%d) %d-%d (%d-%d)\n",
				    h->ip_id, cur->fr_off, cur->fr_end, off,
				    frmax, fra->fr_off, fra->fr_end));
				fra->fr_off = cur->fr_off;
				LIST_REMOVE(cur, fr_next);
				pool_put(&pf_cent_pl, cur);
				pf_ncache--;
				cur = NULL;

			} else if (frp && fra->fr_off <= frp->fr_end) {
				/* Need to merge in a modified 'frp' */
				KASSERT(cur == NULL);
				DPFPRINTF(("fragcache[%d]: adjacent(merge "
				    "%d-%d) %d-%d (%d-%d)\n",
				    h->ip_id, frp->fr_off, frp->fr_end, off,
				    frmax, fra->fr_off, fra->fr_end));
				fra->fr_off = frp->fr_off;
				LIST_REMOVE(frp, fr_next);
				pool_put(&pf_cent_pl, frp);
				pf_ncache--;
				frp = NULL;

			}
		}
	}

	if (hosed) {
		/*
		 * We must keep tracking the overall fragment even when
		 * we're going to drop it anyway so that we know when to
		 * free the overall descriptor.  Thus we drop the frag late.
		 */
		goto drop_fragment;
	}


 pass:
	/* Update maximum data size */
	if ((*frag)->fr_max < frmax)
		(*frag)->fr_max = frmax;

	/* This is the last segment */
	if (!mff)
		(*frag)->fr_flags |= PFFRAG_SEENLAST;

	/* Check if we are completely reassembled */
	if (((*frag)->fr_flags & PFFRAG_SEENLAST) &&
	    LIST_FIRST(&(*frag)->fr_cache)->fr_off == 0 &&
	    LIST_FIRST(&(*frag)->fr_cache)->fr_end == (*frag)->fr_max) {
		/* Remove from fragment queue */
		DPFPRINTF(("fragcache[%d]: done 0-%d\n", h->ip_id,
		    (*frag)->fr_max));
		pf_free_fragment(*frag);
		*frag = NULL;
	}

	return (m);

 no_mem:
	*nomem = 1;

	/* Still need to pay attention to !IP_MF */
	if (!mff && *frag != NULL)
		(*frag)->fr_flags |= PFFRAG_SEENLAST;

	m_freem(m);
	return (NULL);

 drop_fragment:

	/* Still need to pay attention to !IP_MF */
	if (!mff && *frag != NULL)
		(*frag)->fr_flags |= PFFRAG_SEENLAST;

	if (drop) {
		/* This fragment has been deemed bad.  Don't reass */
		if (((*frag)->fr_flags & PFFRAG_DROP) == 0)
			DPFPRINTF(("fragcache[%d]: dropping overall fragment\n",
			    h->ip_id));
		(*frag)->fr_flags |= PFFRAG_DROP;
	}

	m_freem(m);
	return (NULL);
}

int
pf_normalize_ip(struct mbuf **m0, int dir, struct pfi_kif *kif, u_short *reason,
    struct pf_pdesc *pd)
{
	struct mbuf		*m = *m0;
	struct pf_rule		*r;
	struct pf_frent		*frent;
	struct pf_fragment	*frag = NULL;
	struct ip		*h = mtod(m, struct ip *);
	int			 mff = (ntohs(h->ip_off) & IP_MF);
	int			 hlen = h->ip_hl << 2;
	u_int16_t		 fragoff = (ntohs(h->ip_off) & IP_OFFMASK) << 3;
	u_int16_t		 frmax;
	int			 ip_len;
	int			 ip_off;

	r = TAILQ_FIRST(pf_main_ruleset.rules[PF_RULESET_SCRUB].active.ptr);
	while (r != NULL) {
		r->evaluations++;
		if (pfi_kif_match(r->kif, kif) == r->ifnot)
			r = r->skip[PF_SKIP_IFP].ptr;
		else if (r->direction && r->direction != dir)
			r = r->skip[PF_SKIP_DIR].ptr;
		else if (r->af && r->af != AF_INET)
			r = r->skip[PF_SKIP_AF].ptr;
		else if (r->proto && r->proto != h->ip_p)
			r = r->skip[PF_SKIP_PROTO].ptr;
		else if (PF_MISMATCHAW(&r->src.addr,
		    (struct pf_addr *)&h->ip_src.s_addr, AF_INET,
		    r->src.neg, kif))
			r = r->skip[PF_SKIP_SRC_ADDR].ptr;
		else if (PF_MISMATCHAW(&r->dst.addr,
		    (struct pf_addr *)&h->ip_dst.s_addr, AF_INET,
		    r->dst.neg, NULL))
			r = r->skip[PF_SKIP_DST_ADDR].ptr;
		else
			break;
	}

	if (r == NULL || r->action == PF_NOSCRUB)
		return (PF_PASS);
	else {
		r->packets[dir == PF_OUT]++;
		r->bytes[dir == PF_OUT] += pd->tot_len;
	}

	/* Check for illegal packets */
	if (hlen < (int)sizeof(struct ip))
		goto drop;

	if (hlen > ntohs(h->ip_len))
		goto drop;

	/* Clear IP_DF if the rule uses the no-df option */
	if (r->rule_flag & PFRULE_NODF && h->ip_off & htons(IP_DF)) {
		u_int16_t off = h->ip_off;

		h->ip_off &= htons(~IP_DF);
		h->ip_sum = pf_cksum_fixup(h->ip_sum, off, h->ip_off, 0);
	}

	/* We will need other tests here */
	if (!fragoff && !mff)
		goto no_fragment;

	/* We're dealing with a fragment now. Don't allow fragments
	 * with IP_DF to enter the cache. If the flag was cleared by
	 * no-df above, fine. Otherwise drop it.
	 */
	if (h->ip_off & htons(IP_DF)) {
		DPFPRINTF(("IP_DF\n"));
		goto bad;
	}

	ip_len = ntohs(h->ip_len) - hlen;
	ip_off = (ntohs(h->ip_off) & IP_OFFMASK) << 3;

	/* All fragments are 8 byte aligned */
	if (mff && (ip_len & 0x7)) {
		DPFPRINTF(("mff and %d\n", ip_len));
		goto bad;
	}

	/* Respect maximum length */
	if (fragoff + ip_len > IP_MAXPACKET) {
		DPFPRINTF(("max packet %d\n", fragoff + ip_len));
		goto bad;
	}
	frmax = fragoff + ip_len;

	if ((r->rule_flag & (PFRULE_FRAGCROP|PFRULE_FRAGDROP)) == 0) {
		/* Fully buffer all of the fragments */

		frag = pf_find_fragment(h, &pf_frag_tree);

		/* Check if we saw the last fragment already */
		if (frag != NULL && (frag->fr_flags & PFFRAG_SEENLAST) &&
		    frmax > frag->fr_max)
			goto bad;

		/* Get an entry for the fragment queue */
		frent = pool_get(&pf_frent_pl, PR_NOWAIT);
		if (frent == NULL) {
			REASON_SET(reason, PFRES_MEMORY);
			return (PF_DROP);
		}
		pf_nfrents++;
		frent->fr_ip = h;
		frent->fr_m = m;

		/* Might return a completely reassembled mbuf, or NULL */
		DPFPRINTF(("reass frag %d @ %d-%d\n", h->ip_id, fragoff, frmax));
		*m0 = m = pf_reassemble(m0, &frag, frent, mff);

		if (m == NULL)
			return (PF_DROP);

		if (frag != NULL && (frag->fr_flags & PFFRAG_DROP))
			goto drop;

		h = mtod(m, struct ip *);
	} else {
		/* non-buffering fragment cache (drops or masks overlaps) */
		int	nomem = 0;

#ifdef __NetBSD__
		struct pf_mtag *pf_mtag = pf_find_mtag(m);
		KASSERT(pf_mtag != NULL);

		if (dir == PF_OUT && pf_mtag->flags & PF_TAG_FRAGCACHE) {
#else
		if (dir == PF_OUT && m->m_pkthdr.pf.flags & PF_TAG_FRAGCACHE) {
#endif /* !__NetBSD__ */
			/*
			 * Already passed the fragment cache in the
			 * input direction.  If we continued, it would
			 * appear to be a dup and would be dropped.
			 */
			goto fragment_pass;
		}

		frag = pf_find_fragment(h, &pf_cache_tree);

		/* Check if we saw the last fragment already */
		if (frag != NULL && (frag->fr_flags & PFFRAG_SEENLAST) &&
		    frmax > frag->fr_max) {
			if (r->rule_flag & PFRULE_FRAGDROP)
				frag->fr_flags |= PFFRAG_DROP;
			goto bad;
		}

		*m0 = m = pf_fragcache(m0, h, &frag, mff,
		    (r->rule_flag & PFRULE_FRAGDROP) ? 1 : 0, &nomem);
		if (m == NULL) {
			if (nomem)
				goto no_mem;
			goto drop;
		}

		if (dir == PF_IN)
#ifdef __NetBSD__
			pf_mtag = pf_find_mtag(m);
			KASSERT(pf_mtag != NULL);

			pf_mtag->flags |= PF_TAG_FRAGCACHE;
#else
			m->m_pkthdr.pf.flags |= PF_TAG_FRAGCACHE;
#endif /* !__NetBSD__ */

		if (frag != NULL && (frag->fr_flags & PFFRAG_DROP))
			goto drop;
		goto fragment_pass;
	}

 no_fragment:
	/* At this point, only IP_DF is allowed in ip_off */
	if (h->ip_off & ~htons(IP_DF)) {
		u_int16_t off = h->ip_off;

		h->ip_off &= htons(IP_DF);
		h->ip_sum = pf_cksum_fixup(h->ip_sum, off, h->ip_off, 0);
	}

	/* Enforce a minimum ttl, may cause endless packet loops */
	if (r->min_ttl && h->ip_ttl < r->min_ttl) {
		u_int16_t ip_ttl = h->ip_ttl;

		h->ip_ttl = r->min_ttl;
		h->ip_sum = pf_cksum_fixup(h->ip_sum, ip_ttl, h->ip_ttl, 0);
	}

	if (r->rule_flag & PFRULE_RANDOMID) {
		u_int16_t id = h->ip_id;

		h->ip_id = ip_randomid(ip_ids, 0);
		h->ip_sum = pf_cksum_fixup(h->ip_sum, id, h->ip_id, 0);
	}
	if ((r->rule_flag & (PFRULE_FRAGCROP|PFRULE_FRAGDROP)) == 0)
		pd->flags |= PFDESC_IP_REAS;

	return (PF_PASS);

 fragment_pass:
	/* Enforce a minimum ttl, may cause endless packet loops */
	if (r->min_ttl && h->ip_ttl < r->min_ttl) {
		u_int16_t ip_ttl = h->ip_ttl;

		h->ip_ttl = r->min_ttl;
		h->ip_sum = pf_cksum_fixup(h->ip_sum, ip_ttl, h->ip_ttl, 0);
	}
	if ((r->rule_flag & (PFRULE_FRAGCROP|PFRULE_FRAGDROP)) == 0)
		pd->flags |= PFDESC_IP_REAS;
	return (PF_PASS);

 no_mem:
	REASON_SET(reason, PFRES_MEMORY);
	if (r != NULL && r->log)
		PFLOG_PACKET(kif, h, m, AF_INET, dir, *reason, r, NULL, NULL, pd);
	return (PF_DROP);

 drop:
	REASON_SET(reason, PFRES_NORM);
	if (r != NULL && r->log)
		PFLOG_PACKET(kif, h, m, AF_INET, dir, *reason, r, NULL, NULL, pd);
	return (PF_DROP);

 bad:
	DPFPRINTF(("dropping bad fragment\n"));

	/* Free associated fragments */
	if (frag != NULL)
		pf_free_fragment(frag);

	REASON_SET(reason, PFRES_FRAG);
	if (r != NULL && r->log)
		PFLOG_PACKET(kif, h, m, AF_INET, dir, *reason, r, NULL, NULL, pd);

	return (PF_DROP);
}

#ifdef INET6
int
pf_normalize_ip6(struct mbuf **m0, int dir, struct pfi_kif *kif,
    u_short *reason, struct pf_pdesc *pd)
{
	struct mbuf		*m = *m0;
	struct pf_rule		*r;
	struct ip6_hdr		*h = mtod(m, struct ip6_hdr *);
	int			 off;
	struct ip6_ext		 ext;
	struct ip6_opt		 opt;
	struct ip6_opt_jumbo	 jumbo;
	struct ip6_frag		 frag;
	u_int32_t		 jumbolen = 0, plen;
	u_int16_t		 fragoff = 0;
	int			 optend;
	int			 ooff;
	u_int8_t		 proto;
	int			 terminal;

	r = TAILQ_FIRST(pf_main_ruleset.rules[PF_RULESET_SCRUB].active.ptr);
	while (r != NULL) {
		r->evaluations++;
		if (pfi_kif_match(r->kif, kif) == r->ifnot)
			r = r->skip[PF_SKIP_IFP].ptr;
		else if (r->direction && r->direction != dir)
			r = r->skip[PF_SKIP_DIR].ptr;
		else if (r->af && r->af != AF_INET6)
			r = r->skip[PF_SKIP_AF].ptr;
#if 0 /* header chain! */
		else if (r->proto && r->proto != h->ip6_nxt)
			r = r->skip[PF_SKIP_PROTO].ptr;
#endif
		else if (PF_MISMATCHAW(&r->src.addr,
		    (struct pf_addr *)&h->ip6_src, AF_INET6,
		    r->src.neg, kif))
			r = r->skip[PF_SKIP_SRC_ADDR].ptr;
		else if (PF_MISMATCHAW(&r->dst.addr,
		    (struct pf_addr *)&h->ip6_dst, AF_INET6,
		    r->dst.neg, NULL))
			r = r->skip[PF_SKIP_DST_ADDR].ptr;
		else
			break;
	}

	if (r == NULL || r->action == PF_NOSCRUB)
		return (PF_PASS);
	else {
		r->packets[dir == PF_OUT]++;
		r->bytes[dir == PF_OUT] += pd->tot_len;
	}

	/* Check for illegal packets */
	if (sizeof(struct ip6_hdr) + IPV6_MAXPACKET < m->m_pkthdr.len)
		goto drop;

	off = sizeof(struct ip6_hdr);
	proto = h->ip6_nxt;
	terminal = 0;
	do {
		switch (proto) {
		case IPPROTO_FRAGMENT:
			goto fragment;
			break;
		case IPPROTO_AH:
		case IPPROTO_ROUTING:
		case IPPROTO_DSTOPTS:
			if (!pf_pull_hdr(m, off, &ext, sizeof(ext), NULL,
			    NULL, AF_INET6))
				goto shortpkt;
			if (proto == IPPROTO_AH)
				off += (ext.ip6e_len + 2) * 4;
			else
				off += (ext.ip6e_len + 1) * 8;
			proto = ext.ip6e_nxt;
			break;
		case IPPROTO_HOPOPTS:
			if (!pf_pull_hdr(m, off, &ext, sizeof(ext), NULL,
			    NULL, AF_INET6))
				goto shortpkt;
			optend = off + (ext.ip6e_len + 1) * 8;
			ooff = off + sizeof(ext);
			do {
				if (!pf_pull_hdr(m, ooff, &opt.ip6o_type,
				    sizeof(opt.ip6o_type), NULL, NULL,
				    AF_INET6))
					goto shortpkt;
				if (opt.ip6o_type == IP6OPT_PAD1) {
					ooff++;
					continue;
				}
				if (!pf_pull_hdr(m, ooff, &opt, sizeof(opt),
				    NULL, NULL, AF_INET6))
					goto shortpkt;
				if (ooff + sizeof(opt) + opt.ip6o_len > optend)
					goto drop;
				switch (opt.ip6o_type) {
				case IP6OPT_JUMBO:
					if (h->ip6_plen != 0)
						goto drop;
					if (!pf_pull_hdr(m, ooff, &jumbo,
					    sizeof(jumbo), NULL, NULL,
					    AF_INET6))
						goto shortpkt;
					memcpy(&jumbolen, jumbo.ip6oj_jumbo_len,
					    sizeof(jumbolen));
					jumbolen = ntohl(jumbolen);
					if (jumbolen <= IPV6_MAXPACKET)
						goto drop;
					if (sizeof(struct ip6_hdr) + jumbolen !=
					    m->m_pkthdr.len)
						goto drop;
					break;
				default:
					break;
				}
				ooff += sizeof(opt) + opt.ip6o_len;
			} while (ooff < optend);

			off = optend;
			proto = ext.ip6e_nxt;
			break;
		default:
			terminal = 1;
			break;
		}
	} while (!terminal);

	/* jumbo payload option must be present, or plen > 0 */
	if (ntohs(h->ip6_plen) == 0)
		plen = jumbolen;
	else
		plen = ntohs(h->ip6_plen);
	if (plen == 0)
		goto drop;
	if (sizeof(struct ip6_hdr) + plen > m->m_pkthdr.len)
		goto shortpkt;

	/* Enforce a minimum ttl, may cause endless packet loops */
	if (r->min_ttl && h->ip6_hlim < r->min_ttl)
		h->ip6_hlim = r->min_ttl;

	return (PF_PASS);

 fragment:
	if (ntohs(h->ip6_plen) == 0 || jumbolen)
		goto drop;
	plen = ntohs(h->ip6_plen);

	if (!pf_pull_hdr(m, off, &frag, sizeof(frag), NULL, NULL, AF_INET6))
		goto shortpkt;
	fragoff = ntohs(frag.ip6f_offlg & IP6F_OFF_MASK);
	if (fragoff + (plen - off - sizeof(frag)) > IPV6_MAXPACKET)
		goto badfrag;

	/* do something about it */
	/* remember to set pd->flags |= PFDESC_IP_REAS */
	return (PF_PASS);

 shortpkt:
	REASON_SET(reason, PFRES_SHORT);
	if (r != NULL && r->log)
		PFLOG_PACKET(kif, h, m, AF_INET6, dir, *reason, r, NULL, NULL, pd);
	return (PF_DROP);

 drop:
	REASON_SET(reason, PFRES_NORM);
	if (r != NULL && r->log)
		PFLOG_PACKET(kif, h, m, AF_INET6, dir, *reason, r, NULL, NULL, pd);
	return (PF_DROP);

 badfrag:
	REASON_SET(reason, PFRES_FRAG);
	if (r != NULL && r->log)
		PFLOG_PACKET(kif, h, m, AF_INET6, dir, *reason, r, NULL, NULL, pd);
	return (PF_DROP);
}
#endif /* INET6 */

int
pf_normalize_tcp(int dir, struct pfi_kif *kif, struct mbuf *m,
    int ipoff, int off, void *h, struct pf_pdesc *pd)
{
	struct pf_rule	*r, *rm = NULL;
	struct tcphdr	*th = pd->hdr.tcp;
	int		 rewrite = 0;
	u_short		 reason;
	u_int8_t	 flags;
	sa_family_t	 af = pd->af;

	r = TAILQ_FIRST(pf_main_ruleset.rules[PF_RULESET_SCRUB].active.ptr);
	while (r != NULL) {
		r->evaluations++;
		if (pfi_kif_match(r->kif, kif) == r->ifnot)
			r = r->skip[PF_SKIP_IFP].ptr;
		else if (r->direction && r->direction != dir)
			r = r->skip[PF_SKIP_DIR].ptr;
		else if (r->af && r->af != af)
			r = r->skip[PF_SKIP_AF].ptr;
		else if (r->proto && r->proto != pd->proto)
			r = r->skip[PF_SKIP_PROTO].ptr;
		else if (PF_MISMATCHAW(&r->src.addr, pd->src, af,
		    r->src.neg, kif))
			r = r->skip[PF_SKIP_SRC_ADDR].ptr;
		else if (r->src.port_op && !pf_match_port(r->src.port_op,
			    r->src.port[0], r->src.port[1], th->th_sport))
			r = r->skip[PF_SKIP_SRC_PORT].ptr;
		else if (PF_MISMATCHAW(&r->dst.addr, pd->dst, af,
		    r->dst.neg, NULL))
			r = r->skip[PF_SKIP_DST_ADDR].ptr;
		else if (r->dst.port_op && !pf_match_port(r->dst.port_op,
			    r->dst.port[0], r->dst.port[1], th->th_dport))
			r = r->skip[PF_SKIP_DST_PORT].ptr;
		else if (r->os_fingerprint != PF_OSFP_ANY && !pf_osfp_match(
			    pf_osfp_fingerprint(pd, m, off, th),
			    r->os_fingerprint))
			r = TAILQ_NEXT(r, entries);
		else {
			rm = r;
			break;
		}
	}

	if (rm == NULL || rm->action == PF_NOSCRUB)
		return (PF_PASS);
	else {
		r->packets[dir == PF_OUT]++;
		r->bytes[dir == PF_OUT] += pd->tot_len;
	}

	if (rm->rule_flag & PFRULE_REASSEMBLE_TCP)
		pd->flags |= PFDESC_TCP_NORM;

	flags = th->th_flags;
	if (flags & TH_SYN) {
		/* Illegal packet */
		if (flags & TH_RST)
			goto tcp_drop;

		if (flags & TH_FIN)
			flags &= ~TH_FIN;
	} else {
		/* Illegal packet */
		if (!(flags & (TH_ACK|TH_RST)))
			goto tcp_drop;
	}

	if (!(flags & TH_ACK)) {
		/* These flags are only valid if ACK is set */
		if ((flags & TH_FIN) || (flags & TH_PUSH) || (flags & TH_URG))
			goto tcp_drop;
	}

	/* Check for illegal header length */
	if (th->th_off < (sizeof(struct tcphdr) >> 2))
		goto tcp_drop;

	/* If flags changed, or reserved data set, then adjust */
	if (flags != th->th_flags || th->th_x2 != 0) {
		u_int16_t	ov, nv;

		ov = *(u_int16_t *)(&th->th_ack + 1);
		th->th_flags = flags;
		th->th_x2 = 0;
		nv = *(u_int16_t *)(&th->th_ack + 1);

		th->th_sum = pf_cksum_fixup(th->th_sum, ov, nv, 0);
		rewrite = 1;
	}

	/* Remove urgent pointer, if TH_URG is not set */
	if (!(flags & TH_URG) && th->th_urp) {
		th->th_sum = pf_cksum_fixup(th->th_sum, th->th_urp, 0, 0);
		th->th_urp = 0;
		rewrite = 1;
	}

	/* Process options */
	if (r->max_mss && pf_normalize_tcpopt(r, m, th, off))
		rewrite = 1;

	/* copy back packet headers if we sanitized */
	if (rewrite)
		m_copyback(m, off, sizeof(*th), th);

	return (PF_PASS);

 tcp_drop:
	REASON_SET(&reason, PFRES_NORM);
	if (rm != NULL && r->log)
		PFLOG_PACKET(kif, h, m, AF_INET, dir, reason, r, NULL, NULL, pd);
	return (PF_DROP);
}

int
pf_normalize_tcp_init(struct mbuf *m, int off, struct pf_pdesc *pd,
    struct tcphdr *th, struct pf_state_peer *src,
    struct pf_state_peer *dst)
{
	u_int32_t tsval, tsecr;
	u_int8_t hdr[60];
	u_int8_t *opt;

	KASSERT(src->scrub == NULL);

	src->scrub = pool_get(&pf_state_scrub_pl, PR_NOWAIT);
	if (src->scrub == NULL)
		return (1);
	bzero(src->scrub, sizeof(*src->scrub));

	switch (pd->af) {
#ifdef INET
	case AF_INET: {
		struct ip *h = mtod(m, struct ip *);
		src->scrub->pfss_ttl = h->ip_ttl;
		break;
	}
#endif /* INET */
#ifdef INET6
	case AF_INET6: {
		struct ip6_hdr *h = mtod(m, struct ip6_hdr *);
		src->scrub->pfss_ttl = h->ip6_hlim;
		break;
	}
#endif /* INET6 */
	}


	/*
	 * All normalizations below are only begun if we see the start of
	 * the connections.  They must all set an enabled bit in pfss_flags
	 */
	if ((th->th_flags & TH_SYN) == 0)
		return (0);


	if (th->th_off > (sizeof(struct tcphdr) >> 2) && src->scrub &&
	    pf_pull_hdr(m, off, hdr, th->th_off << 2, NULL, NULL, pd->af)) {
		/* Diddle with TCP options */
		int hlen;
		opt = hdr + sizeof(struct tcphdr);
		hlen = (th->th_off << 2) - sizeof(struct tcphdr);
		while (hlen >= TCPOLEN_TIMESTAMP) {
			switch (*opt) {
			case TCPOPT_EOL:	/* FALLTHROUGH */
			case TCPOPT_NOP:
				opt++;
				hlen--;
				break;
			case TCPOPT_TIMESTAMP:
				if (opt[1] >= TCPOLEN_TIMESTAMP) {
					src->scrub->pfss_flags |=
					    PFSS_TIMESTAMP;
					src->scrub->pfss_ts_mod =
					    htonl(cprng_fast32());

					/* note PFSS_PAWS not set yet */
					memcpy(&tsval, &opt[2],
					    sizeof(u_int32_t));
					memcpy(&tsecr, &opt[6],
					    sizeof(u_int32_t));
					src->scrub->pfss_tsval0 = ntohl(tsval);
					src->scrub->pfss_tsval = ntohl(tsval);
					src->scrub->pfss_tsecr = ntohl(tsecr);
					getmicrouptime(&src->scrub->pfss_last);
				}
				/* FALLTHROUGH */
			default:
				hlen -= MAX(opt[1], 2);
				opt += MAX(opt[1], 2);
				break;
			}
		}
	}

	return (0);
}

void
pf_normalize_tcp_cleanup(struct pf_state *state)
{
	if (state->src.scrub)
		pool_put(&pf_state_scrub_pl, state->src.scrub);
	if (state->dst.scrub)
		pool_put(&pf_state_scrub_pl, state->dst.scrub);

	/* Someday... flush the TCP segment reassembly descriptors. */
}

int
pf_normalize_tcp_stateful(struct mbuf *m, int off, struct pf_pdesc *pd,
    u_short *reason, struct tcphdr *th, struct pf_state *state,
    struct pf_state_peer *src, struct pf_state_peer *dst, int *writeback)
{
	struct timeval uptime;
	u_int32_t tsval = 0, tsecr = 0;
	u_int tsval_from_last;
	u_int8_t hdr[60];
	u_int8_t *opt;
	int copyback = 0;
	int got_ts = 0;

	KASSERT(src->scrub || dst->scrub);

	/*
	 * Enforce the minimum TTL seen for this connection.  Negate a common
	 * technique to evade an intrusion detection system and confuse
	 * firewall state code.
	 */
	switch (pd->af) {
#ifdef INET
	case AF_INET: {
		if (src->scrub) {
			struct ip *h = mtod(m, struct ip *);
			if (h->ip_ttl > src->scrub->pfss_ttl)
				src->scrub->pfss_ttl = h->ip_ttl;
			h->ip_ttl = src->scrub->pfss_ttl;
		}
		break;
	}
#endif /* INET */
#ifdef INET6
	case AF_INET6: {
		if (src->scrub) {
			struct ip6_hdr *h = mtod(m, struct ip6_hdr *);
			if (h->ip6_hlim > src->scrub->pfss_ttl)
				src->scrub->pfss_ttl = h->ip6_hlim;
			h->ip6_hlim = src->scrub->pfss_ttl;
		}
		break;
	}
#endif /* INET6 */
	}

	if (th->th_off > (sizeof(struct tcphdr) >> 2) &&
	    ((src->scrub && (src->scrub->pfss_flags & PFSS_TIMESTAMP)) ||
	    (dst->scrub && (dst->scrub->pfss_flags & PFSS_TIMESTAMP))) &&
	    pf_pull_hdr(m, off, hdr, th->th_off << 2, NULL, NULL, pd->af)) {
		/* Diddle with TCP options */
		int hlen;
		opt = hdr + sizeof(struct tcphdr);
		hlen = (th->th_off << 2) - sizeof(struct tcphdr);
		while (hlen >= TCPOLEN_TIMESTAMP) {
			switch (*opt) {
			case TCPOPT_EOL:	/* FALLTHROUGH */
			case TCPOPT_NOP:
				opt++;
				hlen--;
				break;
			case TCPOPT_TIMESTAMP:
				/* Modulate the timestamps.  Can be used for
				 * NAT detection, OS uptime determination or
				 * reboot detection.
				 */

				if (got_ts) {
					/* Huh?  Multiple timestamps!? */
					if (pf_status.debug >= PF_DEBUG_MISC) {
						DPFPRINTF(("multiple TS??"));
						pf_print_state(state);
						printf("\n");
					}
					REASON_SET(reason, PFRES_TS);
					return (PF_DROP);
				}
				if (opt[1] >= TCPOLEN_TIMESTAMP) {
					memcpy(&tsval, &opt[2],
					    sizeof(u_int32_t));
					if (tsval && src->scrub &&
					    (src->scrub->pfss_flags &
					    PFSS_TIMESTAMP)) {
						tsval = ntohl(tsval);
						pf_change_a(&opt[2],
						    &th->th_sum,
						    htonl(tsval +
						    src->scrub->pfss_ts_mod),
						    0);
						copyback = 1;
					}

					/* Modulate TS reply iff valid (!0) */
					memcpy(&tsecr, &opt[6],
					    sizeof(u_int32_t));
					if (tsecr && dst->scrub &&
					    (dst->scrub->pfss_flags &
					    PFSS_TIMESTAMP)) {
						tsecr = ntohl(tsecr)
						    - dst->scrub->pfss_ts_mod;
						pf_change_a(&opt[6],
						    &th->th_sum, htonl(tsecr),
						    0);
						copyback = 1;
					}
					got_ts = 1;
				}
				/* FALLTHROUGH */
			default:
				hlen -= MAX(opt[1], 2);
				opt += MAX(opt[1], 2);
				break;
			}
		}
		if (copyback) {
			/* Copyback the options, caller copys back header */
			*writeback = 1;
			m_copyback(m, off + sizeof(struct tcphdr),
			    (th->th_off << 2) - sizeof(struct tcphdr), hdr +
			    sizeof(struct tcphdr));
		}
	}


	/*
	 * Must invalidate PAWS checks on connections idle for too long.
	 * The fastest allowed timestamp clock is 1ms.  That turns out to
	 * be about 24 days before it wraps.  XXX Right now our lowerbound
	 * TS echo check only works for the first 12 days of a connection
	 * when the TS has exhausted half its 32bit space
	 */
#define TS_MAX_IDLE	(24*24*60*60)
#define TS_MAX_CONN	(12*24*60*60)	/* XXX remove when better tsecr check */

	getmicrouptime(&uptime);
	if (src->scrub && (src->scrub->pfss_flags & PFSS_PAWS) &&
	    (uptime.tv_sec - src->scrub->pfss_last.tv_sec > TS_MAX_IDLE ||
	    time_second - state->creation > TS_MAX_CONN))  {
		if (pf_status.debug >= PF_DEBUG_MISC) {
			DPFPRINTF(("src idled out of PAWS\n"));
			pf_print_state(state);
			printf("\n");
		}
		src->scrub->pfss_flags = (src->scrub->pfss_flags & ~PFSS_PAWS)
		    | PFSS_PAWS_IDLED;
	}
	if (dst->scrub && (dst->scrub->pfss_flags & PFSS_PAWS) &&
	    uptime.tv_sec - dst->scrub->pfss_last.tv_sec > TS_MAX_IDLE) {
		if (pf_status.debug >= PF_DEBUG_MISC) {
			DPFPRINTF(("dst idled out of PAWS\n"));
			pf_print_state(state);
			printf("\n");
		}
		dst->scrub->pfss_flags = (dst->scrub->pfss_flags & ~PFSS_PAWS)
		    | PFSS_PAWS_IDLED;
	}

	if (got_ts && src->scrub && dst->scrub &&
	    (src->scrub->pfss_flags & PFSS_PAWS) &&
	    (dst->scrub->pfss_flags & PFSS_PAWS)) {
		/* Validate that the timestamps are "in-window".
		 * RFC1323 describes TCP Timestamp options that allow
		 * measurement of RTT (round trip time) and PAWS
		 * (protection against wrapped sequence numbers).  PAWS
		 * gives us a set of rules for rejecting packets on
		 * long fat pipes (packets that were somehow delayed 
		 * in transit longer than the time it took to send the
		 * full TCP sequence space of 4Gb).  We can use these
		 * rules and infer a few others that will let us treat
		 * the 32bit timestamp and the 32bit echoed timestamp
		 * as sequence numbers to prevent a blind attacker from
		 * inserting packets into a connection.
		 *
		 * RFC1323 tells us:
		 *  - The timestamp on this packet must be greater than
		 *    or equal to the last value echoed by the other
		 *    endpoint.  The RFC says those will be discarded
		 *    since it is a dup that has already been acked.
		 *    This gives us a lowerbound on the timestamp.
		 *        timestamp >= other last echoed timestamp
		 *  - The timestamp will be less than or equal to
		 *    the last timestamp plus the time between the
		 *    last packet and now.  The RFC defines the max
		 *    clock rate as 1ms.  We will allow clocks to be
		 *    up to 10% fast and will allow a total difference
		 *    or 30 seconds due to a route change.  And this
		 *    gives us an upperbound on the timestamp.
		 *        timestamp <= last timestamp + max ticks
		 *    We have to be careful here.  Windows will send an
		 *    initial timestamp of zero and then initialize it
		 *    to a random value after the 3whs; presumably to
		 *    avoid a DoS by having to call an expensive RNG
		 *    during a SYN flood.  Proof MS has at least one
		 *    good security geek.
		 *
		 *  - The TCP timestamp option must also echo the other
		 *    endpoints timestamp.  The timestamp echoed is the
		 *    one carried on the earliest unacknowledged segment
		 *    on the left edge of the sequence window.  The RFC
		 *    states that the host will reject any echoed
		 *    timestamps that were larger than any ever sent.
		 *    This gives us an upperbound on the TS echo.
		 *        tescr <= largest_tsval
		 *  - The lowerbound on the TS echo is a little more
		 *    tricky to determine.  The other endpoint's echoed
		 *    values will not decrease.  But there may be
		 *    network conditions that re-order packets and
		 *    cause our view of them to decrease.  For now the
		 *    only lowerbound we can safely determine is that
		 *    the TS echo will never be less than the original
		 *    TS.  XXX There is probably a better lowerbound.
		 *    Remove TS_MAX_CONN with better lowerbound check.
		 *        tescr >= other original TS
		 *
		 * It is also important to note that the fastest
		 * timestamp clock of 1ms will wrap its 32bit space in
		 * 24 days.  So we just disable TS checking after 24
		 * days of idle time.  We actually must use a 12d
		 * connection limit until we can come up with a better
		 * lowerbound to the TS echo check.
		 */
		struct timeval delta_ts;
		int ts_fudge;


		/*
		 * PFTM_TS_DIFF is how many seconds of leeway to allow
		 * a host's timestamp.  This can happen if the previous
		 * packet got delayed in transit for much longer than
		 * this packet.
		 */
		if ((ts_fudge = state->rule.ptr->timeout[PFTM_TS_DIFF]) == 0)
			ts_fudge = pf_default_rule.timeout[PFTM_TS_DIFF];


		/* Calculate max ticks since the last timestamp */
#define TS_MAXFREQ	1100		/* RFC max TS freq of 1 kHz + 10% skew */
#define TS_MICROSECS	1000000		/* microseconds per second */
		timersub(&uptime, &src->scrub->pfss_last, &delta_ts);
		tsval_from_last = (delta_ts.tv_sec + ts_fudge) * TS_MAXFREQ;
		tsval_from_last += delta_ts.tv_usec / (TS_MICROSECS/TS_MAXFREQ);


		if ((src->state >= TCPS_ESTABLISHED &&
		    dst->state >= TCPS_ESTABLISHED) &&
		    (SEQ_LT(tsval, dst->scrub->pfss_tsecr) ||
		    SEQ_GT(tsval, src->scrub->pfss_tsval + tsval_from_last) ||
		    (tsecr && (SEQ_GT(tsecr, dst->scrub->pfss_tsval) ||
		    SEQ_LT(tsecr, dst->scrub->pfss_tsval0))))) {
			/* Bad RFC1323 implementation or an insertion attack.
			 *
			 * - Solaris 2.6 and 2.7 are known to send another ACK
			 *   after the FIN,FIN|ACK,ACK closing that carries
			 *   an old timestamp.
			 */

			DPFPRINTF(("Timestamp failed %c%c%c%c\n",
			    SEQ_LT(tsval, dst->scrub->pfss_tsecr) ? '0' : ' ',
			    SEQ_GT(tsval, src->scrub->pfss_tsval +
			    tsval_from_last) ? '1' : ' ',
			    SEQ_GT(tsecr, dst->scrub->pfss_tsval) ? '2' : ' ',
			    SEQ_LT(tsecr, dst->scrub->pfss_tsval0)? '3' : ' '));
			DPFPRINTF((" tsval: %" PRIu32 "  tsecr: %" PRIu32
			    "  +ticks: %" PRIu32 "  idle: %"PRIx64"s %ums\n",
			    tsval, tsecr, tsval_from_last, delta_ts.tv_sec,
			    delta_ts.tv_usec / 1000U));
			DPFPRINTF((" src->tsval: %" PRIu32 "  tsecr: %" PRIu32
			    "\n",
			    src->scrub->pfss_tsval, src->scrub->pfss_tsecr));
			DPFPRINTF((" dst->tsval: %" PRIu32 "  tsecr: %" PRIu32
			    "  tsval0: %" PRIu32 "\n",
			    dst->scrub->pfss_tsval,
			    dst->scrub->pfss_tsecr, dst->scrub->pfss_tsval0));
			if (pf_status.debug >= PF_DEBUG_MISC) {
				pf_print_state(state);
				pf_print_flags(th->th_flags);
				printf("\n");
			}
			REASON_SET(reason, PFRES_TS);
			return (PF_DROP);
		}

		/* XXX I'd really like to require tsecr but it's optional */

	} else if (!got_ts && (th->th_flags & TH_RST) == 0 &&
	    ((src->state == TCPS_ESTABLISHED && dst->state == TCPS_ESTABLISHED)
	    || pd->p_len > 0 || (th->th_flags & TH_SYN)) &&
	    src->scrub && dst->scrub &&
	    (src->scrub->pfss_flags & PFSS_PAWS) &&
	    (dst->scrub->pfss_flags & PFSS_PAWS)) {
		/* Didn't send a timestamp.  Timestamps aren't really useful
		 * when:
		 *  - connection opening or closing (often not even sent).
		 *    but we must not let an attacker to put a FIN on a
		 *    data packet to sneak it through our ESTABLISHED check.
		 *  - on a TCP reset.  RFC suggests not even looking at TS.
		 *  - on an empty ACK.  The TS will not be echoed so it will
		 *    probably not help keep the RTT calculation in sync and
		 *    there isn't as much danger when the sequence numbers
		 *    got wrapped.  So some stacks don't include TS on empty
		 *    ACKs :-(
		 *
		 * To minimize the disruption to mostly RFC1323 conformant
		 * stacks, we will only require timestamps on data packets.
		 *
		 * And what do ya know, we cannot require timestamps on data
		 * packets.  There appear to be devices that do legitimate
		 * TCP connection hijacking.  There are HTTP devices that allow
		 * a 3whs (with timestamps) and then buffer the HTTP request.
		 * If the intermediate device has the HTTP response cache, it
		 * will spoof the response but not bother timestamping its
		 * packets.  So we can look for the presence of a timestamp in
		 * the first data packet and if there, require it in all future
		 * packets.
		 */

		if (pd->p_len > 0 && (src->scrub->pfss_flags & PFSS_DATA_TS)) {
			/*
			 * Hey!  Someone tried to sneak a packet in.  Or the
			 * stack changed its RFC1323 behavior?!?!
			 */
			if (pf_status.debug >= PF_DEBUG_MISC) {
				DPFPRINTF(("Did not receive expected RFC1323 "
				    "timestamp\n"));
				pf_print_state(state);
				pf_print_flags(th->th_flags);
				printf("\n");
			}
			REASON_SET(reason, PFRES_TS);
			return (PF_DROP);
		}
	}


	/*
	 * We will note if a host sends his data packets with or without
	 * timestamps.  And require all data packets to contain a timestamp
	 * if the first does.  PAWS implicitly requires that all data packets be
	 * timestamped.  But I think there are middle-man devices that hijack
	 * TCP streams immediately after the 3whs and don't timestamp their
	 * packets (seen in a WWW accelerator or cache).
	 */
	if (pd->p_len > 0 && src->scrub && (src->scrub->pfss_flags &
	    (PFSS_TIMESTAMP|PFSS_DATA_TS|PFSS_DATA_NOTS)) == PFSS_TIMESTAMP) {
		if (got_ts)
			src->scrub->pfss_flags |= PFSS_DATA_TS;
		else {
			src->scrub->pfss_flags |= PFSS_DATA_NOTS;
			if (pf_status.debug >= PF_DEBUG_MISC && dst->scrub &&
			    (dst->scrub->pfss_flags & PFSS_TIMESTAMP)) {
				/* Don't warn if other host rejected RFC1323 */
				DPFPRINTF(("Broken RFC1323 stack did not "
				    "timestamp data packet. Disabled PAWS "
				    "security.\n"));
				pf_print_state(state);
				pf_print_flags(th->th_flags);
				printf("\n");
			}
		}
	}


	/*
	 * Update PAWS values
	 */
	if (got_ts && src->scrub && PFSS_TIMESTAMP == (src->scrub->pfss_flags &
	    (PFSS_PAWS_IDLED|PFSS_TIMESTAMP))) {
		getmicrouptime(&src->scrub->pfss_last);
		if (SEQ_GEQ(tsval, src->scrub->pfss_tsval) ||
		    (src->scrub->pfss_flags & PFSS_PAWS) == 0)
			src->scrub->pfss_tsval = tsval;

		if (tsecr) {
			if (SEQ_GEQ(tsecr, src->scrub->pfss_tsecr) ||
			    (src->scrub->pfss_flags & PFSS_PAWS) == 0)
				src->scrub->pfss_tsecr = tsecr;

			if ((src->scrub->pfss_flags & PFSS_PAWS) == 0 &&
			    (SEQ_LT(tsval, src->scrub->pfss_tsval0) ||
			    src->scrub->pfss_tsval0 == 0)) {
				/* tsval0 MUST be the lowest timestamp */
				src->scrub->pfss_tsval0 = tsval;
			}

			/* Only fully initialized after a TS gets echoed */
			if ((src->scrub->pfss_flags & PFSS_PAWS) == 0)
				src->scrub->pfss_flags |= PFSS_PAWS;
		}
	}

	/* I have a dream....  TCP segment reassembly.... */
	return (0);
}

int
pf_normalize_tcpopt(struct pf_rule *r, struct mbuf *m, struct tcphdr *th,
    int off)
{
	u_int16_t	*mss;
	int		 thoff;
	int		 opt, cnt, optlen = 0;
	int		 rewrite = 0;
	u_char		*optp;

	thoff = th->th_off << 2;
	cnt = thoff - sizeof(struct tcphdr);
	optp = mtod(m, u_char *) + off + sizeof(struct tcphdr);

	for (; cnt > 0; cnt -= optlen, optp += optlen) {
		opt = optp[0];
		if (opt == TCPOPT_EOL)
			break;
		if (opt == TCPOPT_NOP)
			optlen = 1;
		else {
			if (cnt < 2)
				break;
			optlen = optp[1];
			if (optlen < 2 || optlen > cnt)
				break;
		}
		switch (opt) {
		case TCPOPT_MAXSEG:
			mss = (u_int16_t *)(optp + 2);
			if ((ntohs(*mss)) > r->max_mss) {
				th->th_sum = pf_cksum_fixup(th->th_sum,
				    *mss, htons(r->max_mss), 0);
				*mss = htons(r->max_mss);
				rewrite = 1;
			}
			break;
		default:
			break;
		}
	}

	return (rewrite);
}
