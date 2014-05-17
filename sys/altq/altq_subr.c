/*	$NetBSD: altq_subr.c,v 1.29 2014/05/17 20:44:24 rmind Exp $	*/
/*	$KAME: altq_subr.c,v 1.24 2005/04/13 03:44:25 suz Exp $	*/

/*
 * Copyright (C) 1997-2003
 *	Sony Computer Science Laboratories Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY SONY CSL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL SONY CSL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: altq_subr.c,v 1.29 2014/05/17 20:44:24 rmind Exp $");

#ifdef _KERNEL_OPT
#include "opt_altq.h"
#include "opt_inet.h"
#include "pf.h"
#endif

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/sysctl.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef INET6
#include <netinet/ip6.h>
#endif
#include <netinet/tcp.h>
#include <netinet/udp.h>

#if NPF > 0
#include <net/pfvar.h>
#endif
#include <altq/altq.h>
#ifdef ALTQ3_COMPAT
#include <altq/altq_conf.h>
#endif

/*
 * internal function prototypes
 */
static void	tbr_timeout(void *);
int (*altq_input)(struct mbuf *, int) = NULL;
static int tbr_timer = 0;	/* token bucket regulator timer */
static struct callout tbr_callout;

#ifdef ALTQ3_CLFIER_COMPAT
static int 	extract_ports4(struct mbuf *, struct ip *, struct flowinfo_in *);
#ifdef INET6
static int 	extract_ports6(struct mbuf *, struct ip6_hdr *,
			       struct flowinfo_in6 *);
#endif
static int	apply_filter4(u_int32_t, struct flow_filter *,
			      struct flowinfo_in *);
static int	apply_ppfilter4(u_int32_t, struct flow_filter *,
				struct flowinfo_in *);
#ifdef INET6
static int	apply_filter6(u_int32_t, struct flow_filter6 *,
			      struct flowinfo_in6 *);
#endif
static int	apply_tosfilter4(u_int32_t, struct flow_filter *,
				 struct flowinfo_in *);
static u_long	get_filt_handle(struct acc_classifier *, int);
static struct acc_filter *filth_to_filtp(struct acc_classifier *, u_long);
static u_int32_t filt2fibmask(struct flow_filter *);

static void 	ip4f_cache(struct ip *, struct flowinfo_in *);
static int 	ip4f_lookup(struct ip *, struct flowinfo_in *);
static int 	ip4f_init(void);
static struct ip4_frag	*ip4f_alloc(void);
static void 	ip4f_free(struct ip4_frag *);
#endif /* ALTQ3_CLFIER_COMPAT */

/*
 * alternate queueing support routines
 */

/* look up the queue state by the interface name and the queueing type. */
void *
altq_lookup(char *name, int type)
{
	struct ifnet *ifp;

	if ((ifp = ifunit(name)) != NULL) {
		if (type != ALTQT_NONE && ifp->if_snd.altq_type == type)
			return (ifp->if_snd.altq_disc);
	}

	return NULL;
}

int
altq_attach(struct ifaltq *ifq, int type, void *discipline,
    int (*enqueue)(struct ifaltq *, struct mbuf *, struct altq_pktattr *),
    struct mbuf *(*dequeue)(struct ifaltq *, int),
    int (*request)(struct ifaltq *, int, void *),
    void *clfier, void *(*classify)(void *, struct mbuf *, int))
{
	if (!ALTQ_IS_READY(ifq))
		return ENXIO;

#ifdef ALTQ3_COMPAT
	/*
	 * pfaltq can override the existing discipline, but altq3 cannot.
	 * check these if clfier is not NULL (which implies altq3).
	 */
	if (clfier != NULL) {
		if (ALTQ_IS_ENABLED(ifq))
			return EBUSY;
		if (ALTQ_IS_ATTACHED(ifq))
			return EEXIST;
	}
#endif
	ifq->altq_type     = type;
	ifq->altq_disc     = discipline;
	ifq->altq_enqueue  = enqueue;
	ifq->altq_dequeue  = dequeue;
	ifq->altq_request  = request;
	ifq->altq_clfier   = clfier;
	ifq->altq_classify = classify;
	ifq->altq_flags &= (ALTQF_CANTCHANGE|ALTQF_ENABLED);
#ifdef ALTQ3_COMPAT
#ifdef ALTQ_KLD
	altq_module_incref(type);
#endif
#endif
	return 0;
}

int
altq_detach(struct ifaltq *ifq)
{
	if (!ALTQ_IS_READY(ifq))
		return ENXIO;
	if (ALTQ_IS_ENABLED(ifq))
		return EBUSY;
	if (!ALTQ_IS_ATTACHED(ifq))
		return (0);
#ifdef ALTQ3_COMPAT
#ifdef ALTQ_KLD
	altq_module_declref(ifq->altq_type);
#endif
#endif

	ifq->altq_type     = ALTQT_NONE;
	ifq->altq_disc     = NULL;
	ifq->altq_enqueue  = NULL;
	ifq->altq_dequeue  = NULL;
	ifq->altq_request  = NULL;
	ifq->altq_clfier   = NULL;
	ifq->altq_classify = NULL;
	ifq->altq_flags &= ALTQF_CANTCHANGE;
	return 0;
}

int
altq_enable(struct ifaltq *ifq)
{
	int s;

	if (!ALTQ_IS_READY(ifq))
		return ENXIO;
	if (ALTQ_IS_ENABLED(ifq))
		return 0;

	s = splnet();
	IFQ_PURGE(ifq);
	ASSERT(ifq->ifq_len == 0);
	ifq->altq_flags |= ALTQF_ENABLED;
	if (ifq->altq_clfier != NULL)
		ifq->altq_flags |= ALTQF_CLASSIFY;
	splx(s);

	return 0;
}

int
altq_disable(struct ifaltq *ifq)
{
	int s;

	if (!ALTQ_IS_ENABLED(ifq))
		return 0;

	s = splnet();
	IFQ_PURGE(ifq);
	ASSERT(ifq->ifq_len == 0);
	ifq->altq_flags &= ~(ALTQF_ENABLED|ALTQF_CLASSIFY);
	splx(s);
	return 0;
}

#ifdef ALTQ_DEBUG
void
altq_assert(const char *file, int line, const char *failedexpr)
{
	(void)printf("altq assertion \"%s\" failed: file \"%s\", line %d\n",
		     failedexpr, file, line);
	panic("altq assertion");
	/* NOTREACHED */
}
#endif

/*
 * internal representation of token bucket parameters
 *	rate:	byte_per_unittime << 32
 *		(((bits_per_sec) / 8) << 32) / machclk_freq
 *	depth:	byte << 32
 *
 */
#define	TBR_SHIFT	32
#define	TBR_SCALE(x)	((int64_t)(x) << TBR_SHIFT)
#define	TBR_UNSCALE(x)	((x) >> TBR_SHIFT)

struct mbuf *
tbr_dequeue(struct ifaltq *ifq, int op)
{
	struct tb_regulator *tbr;
	struct mbuf *m;
	int64_t interval;
	u_int64_t now;

	tbr = ifq->altq_tbr;
	if (op == ALTDQ_REMOVE && tbr->tbr_lastop == ALTDQ_POLL) {
		/* if this is a remove after poll, bypass tbr check */
	} else {
		/* update token only when it is negative */
		if (tbr->tbr_token <= 0) {
			now = read_machclk();
			interval = now - tbr->tbr_last;
			if (interval >= tbr->tbr_filluptime)
				tbr->tbr_token = tbr->tbr_depth;
			else {
				tbr->tbr_token += interval * tbr->tbr_rate;
				if (tbr->tbr_token > tbr->tbr_depth)
					tbr->tbr_token = tbr->tbr_depth;
			}
			tbr->tbr_last = now;
		}
		/* if token is still negative, don't allow dequeue */
		if (tbr->tbr_token <= 0)
			return (NULL);
	}

	if (ALTQ_IS_ENABLED(ifq))
		m = (*ifq->altq_dequeue)(ifq, op);
	else {
		if (op == ALTDQ_POLL)
			IF_POLL(ifq, m);
		else
			IF_DEQUEUE(ifq, m);
	}

	if (m != NULL && op == ALTDQ_REMOVE)
		tbr->tbr_token -= TBR_SCALE(m_pktlen(m));
	tbr->tbr_lastop = op;
	return (m);
}

/*
 * set a token bucket regulator.
 * if the specified rate is zero, the token bucket regulator is deleted.
 */
int
tbr_set(struct ifaltq *ifq, struct tb_profile *profile)
{
	struct tb_regulator *tbr, *otbr;

	if (machclk_freq == 0)
		init_machclk();
	if (machclk_freq == 0) {
		printf("tbr_set: no CPU clock available!\n");
		return (ENXIO);
	}

	if (profile->rate == 0) {
		/* delete this tbr */
		if ((tbr = ifq->altq_tbr) == NULL)
			return (ENOENT);
		ifq->altq_tbr = NULL;
		free(tbr, M_DEVBUF);
		return (0);
	}

	tbr = malloc(sizeof(struct tb_regulator), M_DEVBUF, M_WAITOK|M_ZERO);
	if (tbr == NULL)
		return (ENOMEM);

	tbr->tbr_rate = TBR_SCALE(profile->rate / 8) / machclk_freq;
	tbr->tbr_depth = TBR_SCALE(profile->depth);
	if (tbr->tbr_rate > 0)
		tbr->tbr_filluptime = tbr->tbr_depth / tbr->tbr_rate;
	else
		tbr->tbr_filluptime = 0xffffffffffffffffLL;
	tbr->tbr_token = tbr->tbr_depth;
	tbr->tbr_last = read_machclk();
	tbr->tbr_lastop = ALTDQ_REMOVE;

	otbr = ifq->altq_tbr;
	ifq->altq_tbr = tbr;	/* set the new tbr */

	if (otbr != NULL) {
		free(otbr, M_DEVBUF);
	} else {
		if (tbr_timer == 0) {
			CALLOUT_RESET(&tbr_callout, 1, tbr_timeout, (void *)0);
			tbr_timer = 1;
		}
	}
	return (0);
}

/*
 * tbr_timeout goes through the interface list, and kicks the drivers
 * if necessary.
 */
static void
tbr_timeout(void *arg)
{
	struct ifnet *ifp;
	int active, s;

	active = 0;
	s = splnet();
	IFNET_FOREACH(ifp) {
		if (!TBR_IS_ENABLED(&ifp->if_snd))
			continue;
		active++;
		if (!IFQ_IS_EMPTY(&ifp->if_snd) && ifp->if_start != NULL)
			(*ifp->if_start)(ifp);
	}
	splx(s);
	if (active > 0)
		CALLOUT_RESET(&tbr_callout, 1, tbr_timeout, (void *)0);
	else
		tbr_timer = 0;	/* don't need tbr_timer anymore */
}

/*
 * get token bucket regulator profile
 */
int
tbr_get(struct ifaltq *ifq, struct tb_profile *profile)
{
	struct tb_regulator *tbr;

	if ((tbr = ifq->altq_tbr) == NULL) {
		profile->rate = 0;
		profile->depth = 0;
	} else {
		profile->rate =
		    (u_int)TBR_UNSCALE(tbr->tbr_rate * 8 * machclk_freq);
		profile->depth = (u_int)TBR_UNSCALE(tbr->tbr_depth);
	}
	return (0);
}

#if NPF > 0
/*
 * attach a discipline to the interface.  if one already exists, it is
 * overridden.
 */
int
altq_pfattach(struct pf_altq *a)
{
	int error = 0;

	switch (a->scheduler) {
	case ALTQT_NONE:
		break;
#ifdef ALTQ_CBQ
	case ALTQT_CBQ:
		error = cbq_pfattach(a);
		break;
#endif
#ifdef ALTQ_PRIQ
	case ALTQT_PRIQ:
		error = priq_pfattach(a);
		break;
#endif
#ifdef ALTQ_HFSC
	case ALTQT_HFSC:
		error = hfsc_pfattach(a);
		break;
#endif
	default:
		error = ENXIO;
	}

	return (error);
}

/*
 * detach a discipline from the interface.
 * it is possible that the discipline was already overridden by another
 * discipline.
 */
int
altq_pfdetach(struct pf_altq *a)
{
	struct ifnet *ifp;
	int s, error = 0;

	if ((ifp = ifunit(a->ifname)) == NULL)
		return (EINVAL);

	/* if this discipline is no longer referenced, just return */
	if (a->altq_disc == NULL || a->altq_disc != ifp->if_snd.altq_disc)
		return (0);

	s = splnet();
	if (ALTQ_IS_ENABLED(&ifp->if_snd))
		error = altq_disable(&ifp->if_snd);
	if (error == 0)
		error = altq_detach(&ifp->if_snd);
	splx(s);

	return (error);
}

/*
 * add a discipline or a queue
 */
int
altq_add(struct pf_altq *a)
{
	int error = 0;

	if (a->qname[0] != 0)
		return (altq_add_queue(a));

	if (machclk_freq == 0)
		init_machclk();
	if (machclk_freq == 0)
		panic("altq_add: no CPU clock");

	switch (a->scheduler) {
#ifdef ALTQ_CBQ
	case ALTQT_CBQ:
		error = cbq_add_altq(a);
		break;
#endif
#ifdef ALTQ_PRIQ
	case ALTQT_PRIQ:
		error = priq_add_altq(a);
		break;
#endif
#ifdef ALTQ_HFSC
	case ALTQT_HFSC:
		error = hfsc_add_altq(a);
		break;
#endif
	default:
		error = ENXIO;
	}

	return (error);
}

/*
 * remove a discipline or a queue
 */
int
altq_remove(struct pf_altq *a)
{
	int error = 0;

	if (a->qname[0] != 0)
		return (altq_remove_queue(a));

	switch (a->scheduler) {
#ifdef ALTQ_CBQ
	case ALTQT_CBQ:
		error = cbq_remove_altq(a);
		break;
#endif
#ifdef ALTQ_PRIQ
	case ALTQT_PRIQ:
		error = priq_remove_altq(a);
		break;
#endif
#ifdef ALTQ_HFSC
	case ALTQT_HFSC:
		error = hfsc_remove_altq(a);
		break;
#endif
	default:
		error = ENXIO;
	}

	return (error);
}

/*
 * add a queue to the discipline
 */
int
altq_add_queue(struct pf_altq *a)
{
	int error = 0;

	switch (a->scheduler) {
#ifdef ALTQ_CBQ
	case ALTQT_CBQ:
		error = cbq_add_queue(a);
		break;
#endif
#ifdef ALTQ_PRIQ
	case ALTQT_PRIQ:
		error = priq_add_queue(a);
		break;
#endif
#ifdef ALTQ_HFSC
	case ALTQT_HFSC:
		error = hfsc_add_queue(a);
		break;
#endif
	default:
		error = ENXIO;
	}

	return (error);
}

/*
 * remove a queue from the discipline
 */
int
altq_remove_queue(struct pf_altq *a)
{
	int error = 0;

	switch (a->scheduler) {
#ifdef ALTQ_CBQ
	case ALTQT_CBQ:
		error = cbq_remove_queue(a);
		break;
#endif
#ifdef ALTQ_PRIQ
	case ALTQT_PRIQ:
		error = priq_remove_queue(a);
		break;
#endif
#ifdef ALTQ_HFSC
	case ALTQT_HFSC:
		error = hfsc_remove_queue(a);
		break;
#endif
	default:
		error = ENXIO;
	}

	return (error);
}

/*
 * get queue statistics
 */
int
altq_getqstats(struct pf_altq *a, void *ubuf, int *nbytes)
{
	int error = 0;

	switch (a->scheduler) {
#ifdef ALTQ_CBQ
	case ALTQT_CBQ:
		error = cbq_getqstats(a, ubuf, nbytes);
		break;
#endif
#ifdef ALTQ_PRIQ
	case ALTQT_PRIQ:
		error = priq_getqstats(a, ubuf, nbytes);
		break;
#endif
#ifdef ALTQ_HFSC
	case ALTQT_HFSC:
		error = hfsc_getqstats(a, ubuf, nbytes);
		break;
#endif
	default:
		error = ENXIO;
	}

	return (error);
}
#endif /* NPF > 0 */

/*
 * read and write diffserv field in IPv4 or IPv6 header
 */
u_int8_t
read_dsfield(struct mbuf *m, struct altq_pktattr *pktattr)
{
	struct mbuf *m0;
	u_int8_t ds_field = 0;

	if (pktattr == NULL ||
	    (pktattr->pattr_af != AF_INET && pktattr->pattr_af != AF_INET6))
		return ((u_int8_t)0);

	/* verify that pattr_hdr is within the mbuf data */
	for (m0 = m; m0 != NULL; m0 = m0->m_next)
		if (((char *)pktattr->pattr_hdr >= m0->m_data) &&
		    ((char *)pktattr->pattr_hdr < m0->m_data + m0->m_len))
			break;
	if (m0 == NULL) {
		/* ick, pattr_hdr is stale */
		pktattr->pattr_af = AF_UNSPEC;
#ifdef ALTQ_DEBUG
		printf("read_dsfield: can't locate header!\n");
#endif
		return ((u_int8_t)0);
	}

	if (pktattr->pattr_af == AF_INET) {
		struct ip *ip = (struct ip *)pktattr->pattr_hdr;

		if (ip->ip_v != 4)
			return ((u_int8_t)0);	/* version mismatch! */
		ds_field = ip->ip_tos;
	}
#ifdef INET6
	else if (pktattr->pattr_af == AF_INET6) {
		struct ip6_hdr *ip6 = (struct ip6_hdr *)pktattr->pattr_hdr;
		u_int32_t flowlabel;

		flowlabel = ntohl(ip6->ip6_flow);
		if ((flowlabel >> 28) != 6)
			return ((u_int8_t)0);	/* version mismatch! */
		ds_field = (flowlabel >> 20) & 0xff;
	}
#endif
	return (ds_field);
}

void
write_dsfield(struct mbuf *m, struct altq_pktattr *pktattr, u_int8_t dsfield)
{
	struct mbuf *m0;

	if (pktattr == NULL ||
	    (pktattr->pattr_af != AF_INET && pktattr->pattr_af != AF_INET6))
		return;

	/* verify that pattr_hdr is within the mbuf data */
	for (m0 = m; m0 != NULL; m0 = m0->m_next)
		if (((char *)pktattr->pattr_hdr >= m0->m_data) &&
		    ((char *)pktattr->pattr_hdr < m0->m_data + m0->m_len))
			break;
	if (m0 == NULL) {
		/* ick, pattr_hdr is stale */
		pktattr->pattr_af = AF_UNSPEC;
#ifdef ALTQ_DEBUG
		printf("write_dsfield: can't locate header!\n");
#endif
		return;
	}

	if (pktattr->pattr_af == AF_INET) {
		struct ip *ip = (struct ip *)pktattr->pattr_hdr;
		u_int8_t old;
		int32_t sum;

		if (ip->ip_v != 4)
			return;		/* version mismatch! */
		old = ip->ip_tos;
		dsfield |= old & 3;	/* leave CU bits */
		if (old == dsfield)
			return;
		ip->ip_tos = dsfield;
		/*
		 * update checksum (from RFC1624)
		 *	   HC' = ~(~HC + ~m + m')
		 */
		sum = ~ntohs(ip->ip_sum) & 0xffff;
		sum += 0xff00 + (~old & 0xff) + dsfield;
		sum = (sum >> 16) + (sum & 0xffff);
		sum += (sum >> 16);  /* add carry */

		ip->ip_sum = htons(~sum & 0xffff);
	}
#ifdef INET6
	else if (pktattr->pattr_af == AF_INET6) {
		struct ip6_hdr *ip6 = (struct ip6_hdr *)pktattr->pattr_hdr;
		u_int32_t flowlabel;

		flowlabel = ntohl(ip6->ip6_flow);
		if ((flowlabel >> 28) != 6)
			return;		/* version mismatch! */
		flowlabel = (flowlabel & 0xf03fffff) | (dsfield << 20);
		ip6->ip6_flow = htonl(flowlabel);
	}
#endif
	return;
}

#define BINTIME_SHIFT	2

u_int32_t machclk_freq = 0;
u_int32_t machclk_per_tick = 0;

void
init_machclk(void)
{

	callout_init(&tbr_callout, 0);

	/*
	 * Always emulate 1GiHz counter using bintime(9)
	 * since it has enough resolution via timecounter(9).
	 * Using machine dependent cpu_counter() is not MP safe
	 * and it won't work even on UP with Speedstep etc.
	 */
	machclk_freq = 1024 * 1024 * 1024;	/* 2^30 to emulate ~1GHz */
	machclk_per_tick = machclk_freq / hz;
#ifdef ALTQ_DEBUG
	printf("altq: emulate %uHz CPU clock\n", machclk_freq);
#endif
}

u_int64_t
read_machclk(void)
{
	struct bintime bt;
	u_int64_t val;

	binuptime(&bt);
	val = (((u_int64_t)bt.sec << 32) + (bt.frac >> 32)) >> BINTIME_SHIFT;
	return (val);
}

#ifdef ALTQ3_CLFIER_COMPAT

#ifndef IPPROTO_ESP
#define	IPPROTO_ESP	50		/* encapsulating security payload */
#endif
#ifndef IPPROTO_AH
#define	IPPROTO_AH	51		/* authentication header */
#endif

/*
 * extract flow information from a given packet.
 * filt_mask shows flowinfo fields required.
 * we assume the ip header is in one mbuf, and addresses and ports are
 * in network byte order.
 */
int
altq_extractflow(struct mbuf *m, int af, struct flowinfo *flow,
    u_int32_t filt_bmask)
{

	switch (af) {
	case PF_INET: {
		struct flowinfo_in *fin;
		struct ip *ip;

		ip = mtod(m, struct ip *);

		if (ip->ip_v != 4)
			break;

		fin = (struct flowinfo_in *)flow;
		fin->fi_len = sizeof(struct flowinfo_in);
		fin->fi_family = AF_INET;

		fin->fi_proto = ip->ip_p;
		fin->fi_tos = ip->ip_tos;

		fin->fi_src.s_addr = ip->ip_src.s_addr;
		fin->fi_dst.s_addr = ip->ip_dst.s_addr;

		if (filt_bmask & FIMB4_PORTS)
			/* if port info is required, extract port numbers */
			extract_ports4(m, ip, fin);
		else {
			fin->fi_sport = 0;
			fin->fi_dport = 0;
			fin->fi_gpi = 0;
		}
		return (1);
	}

#ifdef INET6
	case PF_INET6: {
		struct flowinfo_in6 *fin6;
		struct ip6_hdr *ip6;

		ip6 = mtod(m, struct ip6_hdr *);
		/* should we check the ip version? */

		fin6 = (struct flowinfo_in6 *)flow;
		fin6->fi6_len = sizeof(struct flowinfo_in6);
		fin6->fi6_family = AF_INET6;

		fin6->fi6_proto = ip6->ip6_nxt;
		fin6->fi6_tclass   = (ntohl(ip6->ip6_flow) >> 20) & 0xff;

		fin6->fi6_flowlabel = ip6->ip6_flow & htonl(0x000fffff);
		fin6->fi6_src = ip6->ip6_src;
		fin6->fi6_dst = ip6->ip6_dst;

		if ((filt_bmask & FIMB6_PORTS) ||
		    ((filt_bmask & FIMB6_PROTO)
		     && ip6->ip6_nxt > IPPROTO_IPV6))
			/*
			 * if port info is required, or proto is required
			 * but there are option headers, extract port
			 * and protocol numbers.
			 */
			extract_ports6(m, ip6, fin6);
		else {
			fin6->fi6_sport = 0;
			fin6->fi6_dport = 0;
			fin6->fi6_gpi = 0;
		}
		return (1);
	}
#endif /* INET6 */

	default:
		break;
	}

	/* failed */
	flow->fi_len = sizeof(struct flowinfo);
	flow->fi_family = AF_UNSPEC;
	return (0);
}

/*
 * helper routine to extract port numbers
 */
/* structure for ipsec and ipv6 option header template */
struct _opt6 {
	u_int8_t	opt6_nxt;	/* next header */
	u_int8_t	opt6_hlen;	/* header extension length */
	u_int16_t	_pad;
	u_int32_t	ah_spi;		/* security parameter index
					   for authentication header */
};

/*
 * extract port numbers from a ipv4 packet.
 */
static int
extract_ports4(struct mbuf *m, struct ip *ip, struct flowinfo_in *fin)
{
	struct mbuf *m0;
	u_short ip_off;
	u_int8_t proto;
	int 	off;

	fin->fi_sport = 0;
	fin->fi_dport = 0;
	fin->fi_gpi = 0;

	ip_off = ntohs(ip->ip_off);
	/* if it is a fragment, try cached fragment info */
	if (ip_off & IP_OFFMASK) {
		ip4f_lookup(ip, fin);
		return (1);
	}

	/* locate the mbuf containing the protocol header */
	for (m0 = m; m0 != NULL; m0 = m0->m_next)
		if (((char *)ip >= m0->m_data) &&
		    ((char *)ip < m0->m_data + m0->m_len))
			break;
	if (m0 == NULL) {
#ifdef ALTQ_DEBUG
		printf("extract_ports4: can't locate header! ip=%p\n", ip);
#endif
		return (0);
	}
	off = ((char *)ip - m0->m_data) + (ip->ip_hl << 2);
	proto = ip->ip_p;

#ifdef ALTQ_IPSEC
 again:
#endif
	while (off >= m0->m_len) {
		off -= m0->m_len;
		m0 = m0->m_next;
		if (m0 == NULL)
			return (0);  /* bogus ip_hl! */
	}
	if (m0->m_len < off + 4)
		return (0);

	switch (proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP: {
		struct udphdr *udp;

		udp = (struct udphdr *)(mtod(m0, char *) + off);
		fin->fi_sport = udp->uh_sport;
		fin->fi_dport = udp->uh_dport;
		fin->fi_proto = proto;
		}
		break;

#ifdef ALTQ_IPSEC
	case IPPROTO_ESP:
		if (fin->fi_gpi == 0){
			u_int32_t *gpi;

			gpi = (u_int32_t *)(mtod(m0, char *) + off);
			fin->fi_gpi   = *gpi;
		}
		fin->fi_proto = proto;
		break;

	case IPPROTO_AH: {
			/* get next header and header length */
			struct _opt6 *opt6;

			opt6 = (struct _opt6 *)(mtod(m0, char *) + off);
			proto = opt6->opt6_nxt;
			off += 8 + (opt6->opt6_hlen * 4);
			if (fin->fi_gpi == 0 && m0->m_len >= off + 8)
				fin->fi_gpi = opt6->ah_spi;
		}
		/* goto the next header */
		goto again;
#endif  /* ALTQ_IPSEC */

	default:
		fin->fi_proto = proto;
		return (0);
	}

	/* if this is a first fragment, cache it. */
	if (ip_off & IP_MF)
		ip4f_cache(ip, fin);

	return (1);
}

#ifdef INET6
static int
extract_ports6(struct mbuf *m, struct ip6_hdr *ip6, struct flowinfo_in6 *fin6)
{
	struct mbuf *m0;
	int	off;
	u_int8_t proto;

	fin6->fi6_gpi   = 0;
	fin6->fi6_sport = 0;
	fin6->fi6_dport = 0;

	/* locate the mbuf containing the protocol header */
	for (m0 = m; m0 != NULL; m0 = m0->m_next)
		if (((char *)ip6 >= m0->m_data) &&
		    ((char *)ip6 < m0->m_data + m0->m_len))
			break;
	if (m0 == NULL) {
#ifdef ALTQ_DEBUG
		printf("extract_ports6: can't locate header! ip6=%p\n", ip6);
#endif
		return (0);
	}
	off = ((char *)ip6 - m0->m_data) + sizeof(struct ip6_hdr);

	proto = ip6->ip6_nxt;
	do {
		while (off >= m0->m_len) {
			off -= m0->m_len;
			m0 = m0->m_next;
			if (m0 == NULL)
				return (0);
		}
		if (m0->m_len < off + 4)
			return (0);

		switch (proto) {
		case IPPROTO_TCP:
		case IPPROTO_UDP: {
			struct udphdr *udp;

			udp = (struct udphdr *)(mtod(m0, char *) + off);
			fin6->fi6_sport = udp->uh_sport;
			fin6->fi6_dport = udp->uh_dport;
			fin6->fi6_proto = proto;
			}
			return (1);

		case IPPROTO_ESP:
			if (fin6->fi6_gpi == 0) {
				u_int32_t *gpi;

				gpi = (u_int32_t *)(mtod(m0, char *) + off);
				fin6->fi6_gpi   = *gpi;
			}
			fin6->fi6_proto = proto;
			return (1);

		case IPPROTO_AH: {
			/* get next header and header length */
			struct _opt6 *opt6;

			opt6 = (struct _opt6 *)(mtod(m0, char *) + off);
			if (fin6->fi6_gpi == 0 && m0->m_len >= off + 8)
				fin6->fi6_gpi = opt6->ah_spi;
			proto = opt6->opt6_nxt;
			off += 8 + (opt6->opt6_hlen * 4);
			/* goto the next header */
			break;
			}

		case IPPROTO_HOPOPTS:
		case IPPROTO_ROUTING:
		case IPPROTO_DSTOPTS: {
			/* get next header and header length */
			struct _opt6 *opt6;

			opt6 = (struct _opt6 *)(mtod(m0, char *) + off);
			proto = opt6->opt6_nxt;
			off += (opt6->opt6_hlen + 1) * 8;
			/* goto the next header */
			break;
			}

		case IPPROTO_FRAGMENT:
			/* ipv6 fragmentations are not supported yet */
		default:
			fin6->fi6_proto = proto;
			return (0);
		}
	} while (1);
	/*NOTREACHED*/
}
#endif /* INET6 */

/*
 * altq common classifier
 */
int
acc_add_filter(struct acc_classifier *classifier, struct flow_filter *filter,
    void *class, u_long *phandle)
{
	struct acc_filter *afp, *prev, *tmp;
	int	i, s;

#ifdef INET6
	if (filter->ff_flow.fi_family != AF_INET &&
	    filter->ff_flow.fi_family != AF_INET6)
		return (EINVAL);
#else
	if (filter->ff_flow.fi_family != AF_INET)
		return (EINVAL);
#endif

	afp = malloc(sizeof(struct acc_filter), M_DEVBUF, M_WAITOK|M_ZERO);
	if (afp == NULL)
		return (ENOMEM);

	afp->f_filter = *filter;
	afp->f_class = class;

	i = ACC_WILDCARD_INDEX;
	if (filter->ff_flow.fi_family == AF_INET) {
		struct flow_filter *filter4 = &afp->f_filter;

		/*
		 * if address is 0, it's a wildcard.  if address mask
		 * isn't set, use full mask.
		 */
		if (filter4->ff_flow.fi_dst.s_addr == 0)
			filter4->ff_mask.mask_dst.s_addr = 0;
		else if (filter4->ff_mask.mask_dst.s_addr == 0)
			filter4->ff_mask.mask_dst.s_addr = 0xffffffff;
		if (filter4->ff_flow.fi_src.s_addr == 0)
			filter4->ff_mask.mask_src.s_addr = 0;
		else if (filter4->ff_mask.mask_src.s_addr == 0)
			filter4->ff_mask.mask_src.s_addr = 0xffffffff;

		/* clear extra bits in addresses  */
		   filter4->ff_flow.fi_dst.s_addr &=
		       filter4->ff_mask.mask_dst.s_addr;
		   filter4->ff_flow.fi_src.s_addr &=
		       filter4->ff_mask.mask_src.s_addr;

		/*
		 * if dst address is a wildcard, use hash-entry
		 * ACC_WILDCARD_INDEX.
		 */
		if (filter4->ff_mask.mask_dst.s_addr != 0xffffffff)
			i = ACC_WILDCARD_INDEX;
		else
			i = ACC_GET_HASH_INDEX(filter4->ff_flow.fi_dst.s_addr);
	}
#ifdef INET6
	else if (filter->ff_flow.fi_family == AF_INET6) {
		struct flow_filter6 *filter6 =
			(struct flow_filter6 *)&afp->f_filter;
#ifndef IN6MASK0 /* taken from kame ipv6 */
#define	IN6MASK0	{{{ 0, 0, 0, 0 }}}
#define	IN6MASK128	{{{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }}}
		const struct in6_addr in6mask0 = IN6MASK0;
		const struct in6_addr in6mask128 = IN6MASK128;
#endif

		if (IN6_IS_ADDR_UNSPECIFIED(&filter6->ff_flow6.fi6_dst))
			filter6->ff_mask6.mask6_dst = in6mask0;
		else if (IN6_IS_ADDR_UNSPECIFIED(&filter6->ff_mask6.mask6_dst))
			filter6->ff_mask6.mask6_dst = in6mask128;
		if (IN6_IS_ADDR_UNSPECIFIED(&filter6->ff_flow6.fi6_src))
			filter6->ff_mask6.mask6_src = in6mask0;
		else if (IN6_IS_ADDR_UNSPECIFIED(&filter6->ff_mask6.mask6_src))
			filter6->ff_mask6.mask6_src = in6mask128;

		/* clear extra bits in addresses  */
		for (i = 0; i < 16; i++)
			filter6->ff_flow6.fi6_dst.s6_addr[i] &=
			    filter6->ff_mask6.mask6_dst.s6_addr[i];
		for (i = 0; i < 16; i++)
			filter6->ff_flow6.fi6_src.s6_addr[i] &=
			    filter6->ff_mask6.mask6_src.s6_addr[i];

		if (filter6->ff_flow6.fi6_flowlabel == 0)
			i = ACC_WILDCARD_INDEX;
		else
			i = ACC_GET_HASH_INDEX(filter6->ff_flow6.fi6_flowlabel);
	}
#endif /* INET6 */

	afp->f_handle = get_filt_handle(classifier, i);

	/* update filter bitmask */
	afp->f_fbmask = filt2fibmask(filter);
	classifier->acc_fbmask |= afp->f_fbmask;

	/*
	 * add this filter to the filter list.
	 * filters are ordered from the highest rule number.
	 */
	s = splnet();
	prev = NULL;
	LIST_FOREACH(tmp, &classifier->acc_filters[i], f_chain) {
		if (tmp->f_filter.ff_ruleno > afp->f_filter.ff_ruleno)
			prev = tmp;
		else
			break;
	}
	if (prev == NULL)
		LIST_INSERT_HEAD(&classifier->acc_filters[i], afp, f_chain);
	else
		LIST_INSERT_AFTER(prev, afp, f_chain);
	splx(s);

	*phandle = afp->f_handle;
	return (0);
}

int
acc_delete_filter(struct acc_classifier *classifier, u_long handle)
{
	struct acc_filter *afp;
	int	s;

	if ((afp = filth_to_filtp(classifier, handle)) == NULL)
		return (EINVAL);

	s = splnet();
	LIST_REMOVE(afp, f_chain);
	splx(s);

	free(afp, M_DEVBUF);

	/* todo: update filt_bmask */

	return (0);
}

/*
 * delete filters referencing to the specified class.
 * if the all flag is not 0, delete all the filters.
 */
int
acc_discard_filters(struct acc_classifier *classifier, void *class, int all)
{
	struct acc_filter *afp;
	int	i, s;

	s = splnet();
	for (i = 0; i < ACC_FILTER_TABLESIZE; i++) {
		do {
			LIST_FOREACH(afp, &classifier->acc_filters[i], f_chain)
				if (all || afp->f_class == class) {
					LIST_REMOVE(afp, f_chain);
					free(afp, M_DEVBUF);
					/* start again from the head */
					break;
				}
		} while (afp != NULL);
	}
	splx(s);

	if (all)
		classifier->acc_fbmask = 0;

	return (0);
}

void *
acc_classify(void *clfier, struct mbuf *m, int af)
{
	struct acc_classifier *classifier;
	struct flowinfo flow;
	struct acc_filter *afp;
	int	i;

	classifier = (struct acc_classifier *)clfier;
	altq_extractflow(m, af, &flow, classifier->acc_fbmask);

	if (flow.fi_family == AF_INET) {
		struct flowinfo_in *fp = (struct flowinfo_in *)&flow;

		if ((classifier->acc_fbmask & FIMB4_ALL) == FIMB4_TOS) {
			/* only tos is used */
			LIST_FOREACH(afp,
				 &classifier->acc_filters[ACC_WILDCARD_INDEX],
				 f_chain)
				if (apply_tosfilter4(afp->f_fbmask,
						     &afp->f_filter, fp))
					/* filter matched */
					return (afp->f_class);
		} else if ((classifier->acc_fbmask &
			(~(FIMB4_PROTO|FIMB4_SPORT|FIMB4_DPORT) & FIMB4_ALL))
		    == 0) {
			/* only proto and ports are used */
			LIST_FOREACH(afp,
				 &classifier->acc_filters[ACC_WILDCARD_INDEX],
				 f_chain)
				if (apply_ppfilter4(afp->f_fbmask,
						    &afp->f_filter, fp))
					/* filter matched */
					return (afp->f_class);
		} else {
			/* get the filter hash entry from its dest address */
			i = ACC_GET_HASH_INDEX(fp->fi_dst.s_addr);
			do {
				/*
				 * go through this loop twice.  first for dst
				 * hash, second for wildcards.
				 */
				LIST_FOREACH(afp, &classifier->acc_filters[i],
					     f_chain)
					if (apply_filter4(afp->f_fbmask,
							  &afp->f_filter, fp))
						/* filter matched */
						return (afp->f_class);

				/*
				 * check again for filters with a dst addr
				 * wildcard.
				 * (daddr == 0 || dmask != 0xffffffff).
				 */
				if (i != ACC_WILDCARD_INDEX)
					i = ACC_WILDCARD_INDEX;
				else
					break;
			} while (1);
		}
	}
#ifdef INET6
	else if (flow.fi_family == AF_INET6) {
		struct flowinfo_in6 *fp6 = (struct flowinfo_in6 *)&flow;

		/* get the filter hash entry from its flow ID */
		if (fp6->fi6_flowlabel != 0)
			i = ACC_GET_HASH_INDEX(fp6->fi6_flowlabel);
		else
			/* flowlable can be zero */
			i = ACC_WILDCARD_INDEX;

		/* go through this loop twice.  first for flow hash, second
		   for wildcards. */
		do {
			LIST_FOREACH(afp, &classifier->acc_filters[i], f_chain)
				if (apply_filter6(afp->f_fbmask,
					(struct flow_filter6 *)&afp->f_filter,
					fp6))
					/* filter matched */
					return (afp->f_class);

			/*
			 * check again for filters with a wildcard.
			 */
			if (i != ACC_WILDCARD_INDEX)
				i = ACC_WILDCARD_INDEX;
			else
				break;
		} while (1);
	}
#endif /* INET6 */

	/* no filter matched */
	return (NULL);
}

static int
apply_filter4(u_int32_t fbmask, struct flow_filter *filt,
    struct flowinfo_in *pkt)
{
	if (filt->ff_flow.fi_family != AF_INET)
		return (0);
	if ((fbmask & FIMB4_SPORT) && filt->ff_flow.fi_sport != pkt->fi_sport)
		return (0);
	if ((fbmask & FIMB4_DPORT) && filt->ff_flow.fi_dport != pkt->fi_dport)
		return (0);
	if ((fbmask & FIMB4_DADDR) &&
	    filt->ff_flow.fi_dst.s_addr !=
	    (pkt->fi_dst.s_addr & filt->ff_mask.mask_dst.s_addr))
		return (0);
	if ((fbmask & FIMB4_SADDR) &&
	    filt->ff_flow.fi_src.s_addr !=
	    (pkt->fi_src.s_addr & filt->ff_mask.mask_src.s_addr))
		return (0);
	if ((fbmask & FIMB4_PROTO) && filt->ff_flow.fi_proto != pkt->fi_proto)
		return (0);
	if ((fbmask & FIMB4_TOS) && filt->ff_flow.fi_tos !=
	    (pkt->fi_tos & filt->ff_mask.mask_tos))
		return (0);
	if ((fbmask & FIMB4_GPI) && filt->ff_flow.fi_gpi != (pkt->fi_gpi))
		return (0);
	/* match */
	return (1);
}

/*
 * filter matching function optimized for a common case that checks
 * only protocol and port numbers
 */
static int
apply_ppfilter4(u_int32_t fbmask, struct flow_filter *filt,
    struct flowinfo_in *pkt)
{
	if (filt->ff_flow.fi_family != AF_INET)
		return (0);
	if ((fbmask & FIMB4_SPORT) && filt->ff_flow.fi_sport != pkt->fi_sport)
		return (0);
	if ((fbmask & FIMB4_DPORT) && filt->ff_flow.fi_dport != pkt->fi_dport)
		return (0);
	if ((fbmask & FIMB4_PROTO) && filt->ff_flow.fi_proto != pkt->fi_proto)
		return (0);
	/* match */
	return (1);
}

/*
 * filter matching function only for tos field.
 */
static int
apply_tosfilter4(u_int32_t fbmask, struct flow_filter *filt,
    struct flowinfo_in *pkt)
{
	if (filt->ff_flow.fi_family != AF_INET)
		return (0);
	if ((fbmask & FIMB4_TOS) && filt->ff_flow.fi_tos !=
	    (pkt->fi_tos & filt->ff_mask.mask_tos))
		return (0);
	/* match */
	return (1);
}

#ifdef INET6
static int
apply_filter6(u_int32_t fbmask, struct flow_filter6 *filt,
    struct flowinfo_in6 *pkt)
{
	int i;

	if (filt->ff_flow6.fi6_family != AF_INET6)
		return (0);
	if ((fbmask & FIMB6_FLABEL) &&
	    filt->ff_flow6.fi6_flowlabel != pkt->fi6_flowlabel)
		return (0);
	if ((fbmask & FIMB6_PROTO) &&
	    filt->ff_flow6.fi6_proto != pkt->fi6_proto)
		return (0);
	if ((fbmask & FIMB6_SPORT) &&
	    filt->ff_flow6.fi6_sport != pkt->fi6_sport)
		return (0);
	if ((fbmask & FIMB6_DPORT) &&
	    filt->ff_flow6.fi6_dport != pkt->fi6_dport)
		return (0);
	if (fbmask & FIMB6_SADDR) {
		for (i = 0; i < 4; i++)
			if (filt->ff_flow6.fi6_src.s6_addr32[i] !=
			    (pkt->fi6_src.s6_addr32[i] &
			     filt->ff_mask6.mask6_src.s6_addr32[i]))
				return (0);
	}
	if (fbmask & FIMB6_DADDR) {
		for (i = 0; i < 4; i++)
			if (filt->ff_flow6.fi6_dst.s6_addr32[i] !=
			    (pkt->fi6_dst.s6_addr32[i] &
			     filt->ff_mask6.mask6_dst.s6_addr32[i]))
				return (0);
	}
	if ((fbmask & FIMB6_TCLASS) &&
	    filt->ff_flow6.fi6_tclass !=
	    (pkt->fi6_tclass & filt->ff_mask6.mask6_tclass))
		return (0);
	if ((fbmask & FIMB6_GPI) &&
	    filt->ff_flow6.fi6_gpi != pkt->fi6_gpi)
		return (0);
	/* match */
	return (1);
}
#endif /* INET6 */

/*
 *  filter handle:
 *	bit 20-28: index to the filter hash table
 *	bit  0-19: unique id in the hash bucket.
 */
static u_long
get_filt_handle(struct acc_classifier *classifier, int i)
{
	static u_long handle_number = 1;
	u_long 	handle;
	struct acc_filter *afp;

	while (1) {
		handle = handle_number++ & 0x000fffff;

		if (LIST_EMPTY(&classifier->acc_filters[i]))
			break;

		LIST_FOREACH(afp, &classifier->acc_filters[i], f_chain)
			if ((afp->f_handle & 0x000fffff) == handle)
				break;
		if (afp == NULL)
			break;
		/* this handle is already used, try again */
	}

	return ((i << 20) | handle);
}

/* convert filter handle to filter pointer */
static struct acc_filter *
filth_to_filtp(struct acc_classifier *classifier, u_long handle)
{
	struct acc_filter *afp;
	int	i;

	i = ACC_GET_HINDEX(handle);

	LIST_FOREACH(afp, &classifier->acc_filters[i], f_chain)
		if (afp->f_handle == handle)
			return (afp);

	return (NULL);
}

/* create flowinfo bitmask */
static u_int32_t
filt2fibmask(struct flow_filter *filt)
{
	u_int32_t mask = 0;
#ifdef INET6
	struct flow_filter6 *filt6;
#endif

	switch (filt->ff_flow.fi_family) {
	case AF_INET:
		if (filt->ff_flow.fi_proto != 0)
			mask |= FIMB4_PROTO;
		if (filt->ff_flow.fi_tos != 0)
			mask |= FIMB4_TOS;
		if (filt->ff_flow.fi_dst.s_addr != 0)
			mask |= FIMB4_DADDR;
		if (filt->ff_flow.fi_src.s_addr != 0)
			mask |= FIMB4_SADDR;
		if (filt->ff_flow.fi_sport != 0)
			mask |= FIMB4_SPORT;
		if (filt->ff_flow.fi_dport != 0)
			mask |= FIMB4_DPORT;
		if (filt->ff_flow.fi_gpi != 0)
			mask |= FIMB4_GPI;
		break;
#ifdef INET6
	case AF_INET6:
		filt6 = (struct flow_filter6 *)filt;

		if (filt6->ff_flow6.fi6_proto != 0)
			mask |= FIMB6_PROTO;
		if (filt6->ff_flow6.fi6_tclass != 0)
			mask |= FIMB6_TCLASS;
		if (!IN6_IS_ADDR_UNSPECIFIED(&filt6->ff_flow6.fi6_dst))
			mask |= FIMB6_DADDR;
		if (!IN6_IS_ADDR_UNSPECIFIED(&filt6->ff_flow6.fi6_src))
			mask |= FIMB6_SADDR;
		if (filt6->ff_flow6.fi6_sport != 0)
			mask |= FIMB6_SPORT;
		if (filt6->ff_flow6.fi6_dport != 0)
			mask |= FIMB6_DPORT;
		if (filt6->ff_flow6.fi6_gpi != 0)
			mask |= FIMB6_GPI;
		if (filt6->ff_flow6.fi6_flowlabel != 0)
			mask |= FIMB6_FLABEL;
		break;
#endif /* INET6 */
	}
	return (mask);
}


/*
 * helper functions to handle IPv4 fragments.
 * currently only in-sequence fragments are handled.
 *	- fragment info is cached in a LRU list.
 *	- when a first fragment is found, cache its flow info.
 *	- when a non-first fragment is found, lookup the cache.
 */

struct ip4_frag {
    TAILQ_ENTRY(ip4_frag) ip4f_chain;
    char    ip4f_valid;
    u_short ip4f_id;
    struct flowinfo_in ip4f_info;
};

static TAILQ_HEAD(ip4f_list, ip4_frag) ip4f_list; /* IPv4 fragment cache */

#define	IP4F_TABSIZE		16	/* IPv4 fragment cache size */


static void
ip4f_cache(struct ip *ip, struct flowinfo_in *fin)
{
	struct ip4_frag *fp;

	if (TAILQ_EMPTY(&ip4f_list)) {
		/* first time call, allocate fragment cache entries. */
		if (ip4f_init() < 0)
			/* allocation failed! */
			return;
	}

	fp = ip4f_alloc();
	fp->ip4f_id = ip->ip_id;
	fp->ip4f_info.fi_proto = ip->ip_p;
	fp->ip4f_info.fi_src.s_addr = ip->ip_src.s_addr;
	fp->ip4f_info.fi_dst.s_addr = ip->ip_dst.s_addr;

	/* save port numbers */
	fp->ip4f_info.fi_sport = fin->fi_sport;
	fp->ip4f_info.fi_dport = fin->fi_dport;
	fp->ip4f_info.fi_gpi   = fin->fi_gpi;
}

static int
ip4f_lookup(struct ip *ip, struct flowinfo_in *fin)
{
	struct ip4_frag *fp;

	for (fp = TAILQ_FIRST(&ip4f_list); fp != NULL && fp->ip4f_valid;
	     fp = TAILQ_NEXT(fp, ip4f_chain))
		if (ip->ip_id == fp->ip4f_id &&
		    ip->ip_src.s_addr == fp->ip4f_info.fi_src.s_addr &&
		    ip->ip_dst.s_addr == fp->ip4f_info.fi_dst.s_addr &&
		    ip->ip_p == fp->ip4f_info.fi_proto) {

			/* found the matching entry */
			fin->fi_sport = fp->ip4f_info.fi_sport;
			fin->fi_dport = fp->ip4f_info.fi_dport;
			fin->fi_gpi   = fp->ip4f_info.fi_gpi;

			if ((ntohs(ip->ip_off) & IP_MF) == 0)
				/* this is the last fragment,
				   release the entry. */
				ip4f_free(fp);

			return (1);
		}

	/* no matching entry found */
	return (0);
}

static int
ip4f_init(void)
{
	struct ip4_frag *fp;
	int i;

	TAILQ_INIT(&ip4f_list);
	for (i=0; i<IP4F_TABSIZE; i++) {
		fp = malloc(sizeof(struct ip4_frag), M_DEVBUF, M_NOWAIT);
		if (fp == NULL) {
			printf("ip4f_init: can't alloc %dth entry!\n", i);
			if (i == 0)
				return (-1);
			return (0);
		}
		fp->ip4f_valid = 0;
		TAILQ_INSERT_TAIL(&ip4f_list, fp, ip4f_chain);
	}
	return (0);
}

static struct ip4_frag *
ip4f_alloc(void)
{
	struct ip4_frag *fp;

	/* reclaim an entry at the tail, put it at the head */
	fp = TAILQ_LAST(&ip4f_list, ip4f_list);
	TAILQ_REMOVE(&ip4f_list, fp, ip4f_chain);
	fp->ip4f_valid = 1;
	TAILQ_INSERT_HEAD(&ip4f_list, fp, ip4f_chain);
	return (fp);
}

static void
ip4f_free(struct ip4_frag *fp)
{
	TAILQ_REMOVE(&ip4f_list, fp, ip4f_chain);
	fp->ip4f_valid = 0;
	TAILQ_INSERT_TAIL(&ip4f_list, fp, ip4f_chain);
}

#endif /* ALTQ3_CLFIER_COMPAT */
