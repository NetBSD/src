/*	$NetBSD: ip_mroute.c,v 1.55.2.1 2001/08/03 04:13:55 lukem Exp $	*/

/*
 * IP multicast forwarding procedures
 *
 * Written by David Waitzman, BBN Labs, August 1988.
 * Modified by Steve Deering, Stanford, February 1989.
 * Modified by Mark J. Steiglitz, Stanford, May, 1991
 * Modified by Van Jacobson, LBL, January 1993
 * Modified by Ajit Thyagarajan, PARC, August 1993
 * Modified by Bill Fenner, PARC, April 1994
 * Modified by Charles M. Hannum, NetBSD, May 1995.
 *
 * MROUTING Revision: 1.2
 */

#include "opt_ipsec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <net/if.h>
#include <net/route.h>
#include <net/raw_cb.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_pcb.h>
#include <netinet/udp.h>
#include <netinet/igmp.h>
#include <netinet/igmp_var.h>
#include <netinet/ip_mroute.h>
#include <netinet/ip_encap.h>

#include <machine/stdarg.h>

#define IP_MULTICASTOPTS 0
#define	M_PULLUP(m, len) \
	do { \
		if ((m) && ((m)->m_flags & M_EXT || (m)->m_len < (len))) \
			(m) = m_pullup((m), (len)); \
	} while (0)

/*
 * Globals.  All but ip_mrouter and ip_mrtproto could be static,
 * except for netstat or debugging purposes.
 */
struct socket  *ip_mrouter  = 0;
int		ip_mrtproto = IGMP_DVMRP;    /* for netstat only */

#define NO_RTE_FOUND 	0x1
#define RTE_FOUND	0x2

#define	MFCHASH(a, g) \
	((((a).s_addr >> 20) ^ ((a).s_addr >> 10) ^ (a).s_addr ^ \
	  ((g).s_addr >> 20) ^ ((g).s_addr >> 10) ^ (g).s_addr) & mfchash)
LIST_HEAD(mfchashhdr, mfc) *mfchashtbl;
u_long	mfchash;

u_char		nexpire[MFCTBLSIZ];
struct vif	viftable[MAXVIFS];
struct mrtstat	mrtstat;
u_int		mrtdebug = 0;	  /* debug level 	*/
#define		DEBUG_MFC	0x02
#define		DEBUG_FORWARD	0x04
#define		DEBUG_EXPIRE	0x08
#define		DEBUG_XMIT	0x10
u_int       	tbfdebug = 0;     /* tbf debug level 	*/
#ifdef RSVP_ISI
u_int		rsvpdebug = 0;	  /* rsvp debug level   */
extern struct socket *ip_rsvpd;
extern int rsvp_on;
#endif /* RSVP_ISI */

/* vif attachment using sys/netinet/ip_encap.c */
extern struct domain inetdomain;
static void vif_input __P((struct mbuf *, ...));
static int vif_encapcheck __P((const struct mbuf *, int, int, void *));
static struct protosw vif_protosw =
{ SOCK_RAW,	&inetdomain,	IPPROTO_IPV4,	PR_ATOMIC|PR_ADDR,
  vif_input,	rip_output,	0,		rip_ctloutput,
  rip_usrreq,
  0,            0,              0,              0,
};

#define		EXPIRE_TIMEOUT	(hz / 4)	/* 4x / second */
#define		UPCALL_EXPIRE	6		/* number of timeouts */

/*
 * Define the token bucket filter structures
 */

#define		TBF_REPROCESS	(hz / 100)	/* 100x / second */

static int get_sg_cnt __P((struct sioc_sg_req *));
static int get_vif_cnt __P((struct sioc_vif_req *));
static int ip_mrouter_init __P((struct socket *, struct mbuf *));
static int get_version __P((struct mbuf *));
static int set_assert __P((struct mbuf *));
static int get_assert __P((struct mbuf *));
static int add_vif __P((struct mbuf *));
static int del_vif __P((struct mbuf *));
static void update_mfc __P((struct mfcctl *, struct mfc *));
static void expire_mfc __P((struct mfc *));
static int add_mfc __P((struct mbuf *));
#ifdef UPCALL_TIMING
static void collate __P((struct timeval *));
#endif
static int del_mfc __P((struct mbuf *));
static int socket_send __P((struct socket *, struct mbuf *,
			    struct sockaddr_in *));
static void expire_upcalls __P((void *));
#ifdef RSVP_ISI
static int ip_mdq __P((struct mbuf *, struct ifnet *, struct mfc *, vifi_t));
#else
static int ip_mdq __P((struct mbuf *, struct ifnet *, struct mfc *));
#endif
static void phyint_send __P((struct ip *, struct vif *, struct mbuf *));
static void encap_send __P((struct ip *, struct vif *, struct mbuf *));
static void tbf_control __P((struct vif *, struct mbuf *, struct ip *,
			     u_int32_t));
static void tbf_queue __P((struct vif *, struct mbuf *));
static void tbf_process_q __P((struct vif *));
static void tbf_reprocess_q __P((void *));
static int tbf_dq_sel __P((struct vif *, struct ip *));
static void tbf_send_packet __P((struct vif *, struct mbuf *));
static void tbf_update_tokens __P((struct vif *));
static int priority __P((struct vif *, struct ip *));

/*
 * 'Interfaces' associated with decapsulator (so we can tell
 * packets that went through it from ones that get reflected
 * by a broken gateway).  These interfaces are never linked into
 * the system ifnet list & no routes point to them.  I.e., packets
 * can't be sent this way.  They only exist as a placeholder for
 * multicast source verification.
 */
#if 0
struct ifnet multicast_decap_if[MAXVIFS];
#endif

#define	ENCAP_TTL	64
#define	ENCAP_PROTO	IPPROTO_IPIP	/* 4 */

/* prototype IP hdr for encapsulated packets */
struct ip multicast_encap_iphdr = {
#if BYTE_ORDER == LITTLE_ENDIAN
	sizeof(struct ip) >> 2, IPVERSION,
#else
	IPVERSION, sizeof(struct ip) >> 2,
#endif
	0,				/* tos */
	sizeof(struct ip),		/* total length */
	0,				/* id */
	0,				/* frag offset */
	ENCAP_TTL, ENCAP_PROTO,	
	0,				/* checksum */
};

/*
 * Private variables.
 */
static vifi_t	   numvifs = 0;
static int have_encap_tunnel = 0;

static struct callout expire_upcalls_ch;

/*
 * one-back cache used by mrt_ipip_input to locate a tunnel's vif
 * given a datagram's src ip address.
 */
static struct in_addr last_encap_src;
static struct vif *last_encap_vif;

/*
 * whether or not special PIM assert processing is enabled.
 */
static int pim_assert;
/*
 * Rate limit for assert notification messages, in usec
 */
#define ASSERT_MSG_TIME		3000000

/*
 * Find a route for a given origin IP address and Multicast group address
 * Type of service parameter to be added in the future!!!
 */

#define MFCFIND(o, g, rt) { \
	struct mfc *_rt; \
	(rt) = 0; \
	++mrtstat.mrts_mfc_lookups; \
	for (_rt = mfchashtbl[MFCHASH(o, g)].lh_first; \
	     _rt; _rt = _rt->mfc_hash.le_next) { \
		if (in_hosteq(_rt->mfc_origin, (o)) && \
		    in_hosteq(_rt->mfc_mcastgrp, (g)) && \
		    _rt->mfc_stall == 0) { \
			(rt) = _rt; \
			break; \
		} \
	} \
	if ((rt) == 0) \
		++mrtstat.mrts_mfc_misses; \
}

/*
 * Macros to compute elapsed time efficiently
 * Borrowed from Van Jacobson's scheduling code
 */
#define TV_DELTA(a, b, delta) { \
	int xxs; \
	delta = (a).tv_usec - (b).tv_usec; \
	xxs = (a).tv_sec - (b).tv_sec; \
	switch (xxs) { \
	case 2: \
		delta += 1000000; \
		/* fall through */ \
	case 1: \
		delta += 1000000; \
		/* fall through */ \
	case 0: \
		break; \
	default: \
		delta += (1000000 * xxs); \
		break; \
	} \
}

#ifdef UPCALL_TIMING
u_int32_t upcall_data[51];
#endif /* UPCALL_TIMING */

/*
 * Handle MRT setsockopt commands to modify the multicast routing tables.
 */
int
ip_mrouter_set(so, optname, m)
	struct socket *so;
	int optname;
	struct mbuf **m;
{
	int error;

	if (optname != MRT_INIT && so != ip_mrouter)
		error = ENOPROTOOPT;
	else
		switch (optname) {
		case MRT_INIT:
			error = ip_mrouter_init(so, *m);
			break;
		case MRT_DONE:
			error = ip_mrouter_done();
			break;
		case MRT_ADD_VIF:
			error = add_vif(*m);
			break;
		case MRT_DEL_VIF:
			error = del_vif(*m);
			break;
		case MRT_ADD_MFC:
			error = add_mfc(*m);
			break;
		case MRT_DEL_MFC:
			error = del_mfc(*m);
			break;
		case MRT_ASSERT:
			error = set_assert(*m);
			break;
		default:
			error = ENOPROTOOPT;
			break;
		}

	if (*m)
		m_free(*m);
	return (error);
}

/*
 * Handle MRT getsockopt commands
 */
int
ip_mrouter_get(so, optname, m)
	struct socket *so;
	int optname;
	struct mbuf **m;
{
	int error;

	if (so != ip_mrouter)
		error = ENOPROTOOPT;
	else {
		*m = m_get(M_WAIT, MT_SOOPTS);

		switch (optname) {
		case MRT_VERSION:
			error = get_version(*m);
			break;
		case MRT_ASSERT:
			error = get_assert(*m);
			break;
		default:
			error = ENOPROTOOPT;
			break;
		}

		if (error)
			m_free(*m);
	}

	return (error);
}

/*
 * Handle ioctl commands to obtain information from the cache
 */
int
mrt_ioctl(so, cmd, data)
	struct socket *so;
	u_long cmd;
	caddr_t data;
{
	int error;

	if (so != ip_mrouter)
		error = EINVAL;
	else
		switch (cmd) {
		case SIOCGETVIFCNT:
			error = get_vif_cnt((struct sioc_vif_req *)data);
			break;
		case SIOCGETSGCNT:
			error = get_sg_cnt((struct sioc_sg_req *)data);
			break;
		default:
			error = EINVAL;
			break;
		}

	return (error);
}

/*
 * returns the packet, byte, rpf-failure count for the source group provided
 */
static int
get_sg_cnt(req)
	struct sioc_sg_req *req;
{
	struct mfc *rt;
	int s;

	s = splsoftnet();
	MFCFIND(req->src, req->grp, rt);
	splx(s);
	if (rt != 0) {
		req->pktcnt = rt->mfc_pkt_cnt;
		req->bytecnt = rt->mfc_byte_cnt;
		req->wrong_if = rt->mfc_wrong_if;
	} else
		req->pktcnt = req->bytecnt = req->wrong_if = 0xffffffff;

	return (0);
}

/*
 * returns the input and output packet and byte counts on the vif provided
 */
static int
get_vif_cnt(req)
	struct sioc_vif_req *req;
{
	vifi_t vifi = req->vifi;

	if (vifi >= numvifs)
		return (EINVAL);

	req->icount = viftable[vifi].v_pkt_in;
	req->ocount = viftable[vifi].v_pkt_out;
	req->ibytes = viftable[vifi].v_bytes_in;
	req->obytes = viftable[vifi].v_bytes_out;

	return (0);
}

/*
 * Enable multicast routing
 */
static int
ip_mrouter_init(so, m)
	struct socket *so;
	struct mbuf *m;
{
	int *v;

	if (mrtdebug)
		log(LOG_DEBUG,
		    "ip_mrouter_init: so_type = %d, pr_protocol = %d\n",
		    so->so_type, so->so_proto->pr_protocol);

	if (so->so_type != SOCK_RAW ||
	    so->so_proto->pr_protocol != IPPROTO_IGMP)
		return (EOPNOTSUPP);

	if (m == 0 || m->m_len < sizeof(int))
		return (EINVAL);

	v = mtod(m, int *);
	if (*v != 1)
		return (EINVAL);

	if (ip_mrouter != 0)
		return (EADDRINUSE);

	ip_mrouter = so;

	mfchashtbl =
	    hashinit(MFCTBLSIZ, HASH_LIST, M_MRTABLE, M_WAITOK, &mfchash);
	bzero((caddr_t)nexpire, sizeof(nexpire));

	pim_assert = 0;

	callout_init(&expire_upcalls_ch);
	callout_reset(&expire_upcalls_ch, EXPIRE_TIMEOUT,
	    expire_upcalls, NULL);

	if (mrtdebug)
		log(LOG_DEBUG, "ip_mrouter_init\n");

	return (0);
}

/*
 * Disable multicast routing
 */
int
ip_mrouter_done()
{
	vifi_t vifi;
	struct vif *vifp;
	int i;
	int s;
	
	s = splsoftnet();

	/* Clear out all the vifs currently in use. */
	for (vifi = 0; vifi < numvifs; vifi++) {
		vifp = &viftable[vifi];
		if (!in_nullhost(vifp->v_lcl_addr))
			reset_vif(vifp);
	}

	numvifs = 0;
	pim_assert = 0;
	
	callout_stop(&expire_upcalls_ch);
	
	/*
	 * Free all multicast forwarding cache entries.
	 */
	for (i = 0; i < MFCTBLSIZ; i++) {
		struct mfc *rt, *nrt;

		for (rt = mfchashtbl[i].lh_first; rt; rt = nrt) {
			nrt = rt->mfc_hash.le_next;
			
			expire_mfc(rt);
		}
	}

	free(mfchashtbl, M_MRTABLE);
	mfchashtbl = 0;
	
	/* Reset de-encapsulation cache. */
	have_encap_tunnel = 0;
	
	ip_mrouter = 0;
	
	splx(s);
	
	if (mrtdebug)
		log(LOG_DEBUG, "ip_mrouter_done\n");
	
	return (0);
}

static int
get_version(m)
	struct mbuf *m;
{
	int *v = mtod(m, int *);

	*v = 0x0305;	/* XXX !!!! */
	m->m_len = sizeof(int);
	return (0);
}

/*
 * Set PIM assert processing global
 */
static int
set_assert(m)
	struct mbuf *m;
{
	int *i;

	if (m == 0 || m->m_len < sizeof(int))
		return (EINVAL);

	i = mtod(m, int *);
	pim_assert = !!*i;
	return (0);
}

/*
 * Get PIM assert processing global
 */
static int
get_assert(m)
	struct mbuf *m;
{
	int *i = mtod(m, int *);

	*i = pim_assert;
	m->m_len = sizeof(int);
	return (0);
}

static struct sockaddr_in sin = { sizeof(sin), AF_INET };

/*
 * Add a vif to the vif table
 */
static int
add_vif(m)
	struct mbuf *m;
{
	struct vifctl *vifcp;
	struct vif *vifp;
	struct ifaddr *ifa;
	struct ifnet *ifp;
	struct ifreq ifr;
	int error, s;
	
	if (m == 0 || m->m_len < sizeof(struct vifctl))
		return (EINVAL);

	vifcp = mtod(m, struct vifctl *);
	if (vifcp->vifc_vifi >= MAXVIFS)
		return (EINVAL);

	vifp = &viftable[vifcp->vifc_vifi];
	if (!in_nullhost(vifp->v_lcl_addr))
		return (EADDRINUSE);
	
	/* Find the interface with an address in AF_INET family. */
	sin.sin_addr = vifcp->vifc_lcl_addr;
	ifa = ifa_ifwithaddr(sintosa(&sin));
	if (ifa == 0)
		return (EADDRNOTAVAIL);
	
	if (vifcp->vifc_flags & VIFF_TUNNEL) {
		if (vifcp->vifc_flags & VIFF_SRCRT) {
			log(LOG_ERR, "Source routed tunnels not supported\n");
			return (EOPNOTSUPP);
		}

		/* attach this vif to decapsulator dispatch table */
		vifp->v_encap_cookie = encap_attach_func(AF_INET, IPPROTO_IPV4,
		    vif_encapcheck, &vif_protosw, vifp);
		if (!vifp->v_encap_cookie)
			return (EINVAL);

		/* Create a fake encapsulation interface. */
		ifp = (struct ifnet *)malloc(sizeof(*ifp), M_MRTABLE, M_WAITOK);
		bzero(ifp, sizeof(*ifp));
		sprintf(ifp->if_xname, "mdecap%d", vifcp->vifc_vifi);

		/* Prepare cached route entry. */
		bzero(&vifp->v_route, sizeof(vifp->v_route));

		/*
		 * Tell mrt_ipip_input() to start looking at encapsulated
		 * packets.
		 */
		have_encap_tunnel = 1;
	} else {
		/* Use the physical interface associated with the address. */
		ifp = ifa->ifa_ifp;

		/* Make sure the interface supports multicast. */
		if ((ifp->if_flags & IFF_MULTICAST) == 0)
			return (EOPNOTSUPP);

		/* Enable promiscuous reception of all IP multicasts. */
		satosin(&ifr.ifr_addr)->sin_len = sizeof(struct sockaddr_in);
		satosin(&ifr.ifr_addr)->sin_family = AF_INET;
		satosin(&ifr.ifr_addr)->sin_addr = zeroin_addr;
		error = (*ifp->if_ioctl)(ifp, SIOCADDMULTI, (caddr_t)&ifr);
		if (error)
			return (error);
	}

	s = splsoftnet();

	/* Define parameters for the tbf structure. */
	vifp->tbf_q = 0;
	vifp->tbf_t = &vifp->tbf_q;
	microtime(&vifp->tbf_last_pkt_t);
	vifp->tbf_n_tok = 0;
	vifp->tbf_q_len = 0;
	vifp->tbf_max_q_len = MAXQSIZE;
	
	vifp->v_flags = vifcp->vifc_flags;
	vifp->v_threshold = vifcp->vifc_threshold;
	/* scaling up here allows division by 1024 in critical code */
	vifp->v_rate_limit = vifcp->vifc_rate_limit * 1024 / 1000;
	vifp->v_lcl_addr = vifcp->vifc_lcl_addr;
	vifp->v_rmt_addr = vifcp->vifc_rmt_addr;
	vifp->v_ifp = ifp;
	/* Initialize per vif pkt counters. */
	vifp->v_pkt_in = 0;
	vifp->v_pkt_out = 0;
	vifp->v_bytes_in = 0;
	vifp->v_bytes_out = 0;

	callout_init(&vifp->v_repq_ch);

#ifdef RSVP_ISI
	vifp->v_rsvp_on = 0;
	vifp->v_rsvpd = 0;
#endif /* RSVP_ISI */

	splx(s);
	
	/* Adjust numvifs up if the vifi is higher than numvifs. */
	if (numvifs <= vifcp->vifc_vifi)
		numvifs = vifcp->vifc_vifi + 1;
	
	if (mrtdebug)
		log(LOG_DEBUG, "add_vif #%d, lcladdr %x, %s %x, thresh %x, rate %d\n",
		    vifcp->vifc_vifi, 
		    ntohl(vifcp->vifc_lcl_addr.s_addr),
		    (vifcp->vifc_flags & VIFF_TUNNEL) ? "rmtaddr" : "mask",
		    ntohl(vifcp->vifc_rmt_addr.s_addr),
		    vifcp->vifc_threshold,
		    vifcp->vifc_rate_limit);    
	
	return (0);
}

void
reset_vif(vifp)
	struct vif *vifp;
{
	struct mbuf *m, *n;
	struct ifnet *ifp;
	struct ifreq ifr;

	callout_stop(&vifp->v_repq_ch);

	/* detach this vif from decapsulator dispatch table */
	encap_detach(vifp->v_encap_cookie);
	vifp->v_encap_cookie = NULL;

	for (m = vifp->tbf_q; m != 0; m = n) {
		n = m->m_nextpkt;
		m_freem(m);
	}

	if (vifp->v_flags & VIFF_TUNNEL) {
		free(vifp->v_ifp, M_MRTABLE);
		if (vifp == last_encap_vif) {
			last_encap_vif = 0;
			last_encap_src = zeroin_addr;
		}
	} else {
		satosin(&ifr.ifr_addr)->sin_len = sizeof(struct sockaddr_in);
		satosin(&ifr.ifr_addr)->sin_family = AF_INET;
		satosin(&ifr.ifr_addr)->sin_addr = zeroin_addr;
		ifp = vifp->v_ifp;
		(*ifp->if_ioctl)(ifp, SIOCDELMULTI, (caddr_t)&ifr);
	}
	bzero((caddr_t)vifp, sizeof(*vifp));
}

/*
 * Delete a vif from the vif table
 */
static int
del_vif(m)
	struct mbuf *m;
{
	vifi_t *vifip;
	struct vif *vifp;
	vifi_t vifi;
	int s;
	
	if (m == 0 || m->m_len < sizeof(vifi_t))
		return (EINVAL);

	vifip = mtod(m, vifi_t *);
	if (*vifip >= numvifs)
		return (EINVAL);

	vifp = &viftable[*vifip];
	if (in_nullhost(vifp->v_lcl_addr))
		return (EADDRNOTAVAIL);
	
	s = splsoftnet();
	
	reset_vif(vifp);
	
	/* Adjust numvifs down */
	for (vifi = numvifs; vifi > 0; vifi--)
		if (!in_nullhost(viftable[vifi-1].v_lcl_addr))
			break;
	numvifs = vifi;
	
	splx(s);
	
	if (mrtdebug)
		log(LOG_DEBUG, "del_vif %d, numvifs %d\n", *vifip, numvifs);
	
	return (0);
}

static void
update_mfc(mfccp, rt)
	struct mfcctl *mfccp;
	struct mfc *rt;
{
	vifi_t vifi;

	rt->mfc_parent = mfccp->mfcc_parent;
	for (vifi = 0; vifi < numvifs; vifi++)
		rt->mfc_ttls[vifi] = mfccp->mfcc_ttls[vifi];
	rt->mfc_expire = 0;
	rt->mfc_stall = 0;
}

static void
expire_mfc(rt)
	struct mfc *rt;
{
	struct rtdetq *rte, *nrte;

	for (rte = rt->mfc_stall; rte != 0; rte = nrte) {
		nrte = rte->next;
		m_freem(rte->m);
		free(rte, M_MRTABLE);
	}

	LIST_REMOVE(rt, mfc_hash);
	free(rt, M_MRTABLE);
}

/*
 * Add an mfc entry
 */
static int
add_mfc(m)
	struct mbuf *m;
{
	struct mfcctl *mfccp;
	struct mfc *rt;
	u_int32_t hash = 0;
	struct rtdetq *rte, *nrte;
	u_short nstl;
	int s;

	if (m == 0 || m->m_len < sizeof(struct mfcctl))
		return (EINVAL);

	mfccp = mtod(m, struct mfcctl *);

	s = splsoftnet();
	MFCFIND(mfccp->mfcc_origin, mfccp->mfcc_mcastgrp, rt);

	/* If an entry already exists, just update the fields */
	if (rt) {
		if (mrtdebug & DEBUG_MFC)
			log(LOG_DEBUG,"add_mfc update o %x g %x p %x\n",
			    ntohl(mfccp->mfcc_origin.s_addr),
			    ntohl(mfccp->mfcc_mcastgrp.s_addr),
			    mfccp->mfcc_parent);

		if (rt->mfc_expire)
			nexpire[hash]--;

		update_mfc(mfccp, rt);

		splx(s);
		return (0);
	}

	/* 
	 * Find the entry for which the upcall was made and update
	 */
	nstl = 0;
	hash = MFCHASH(mfccp->mfcc_origin, mfccp->mfcc_mcastgrp);
	for (rt = mfchashtbl[hash].lh_first; rt; rt = rt->mfc_hash.le_next) {
		if (in_hosteq(rt->mfc_origin, mfccp->mfcc_origin) &&
		    in_hosteq(rt->mfc_mcastgrp, mfccp->mfcc_mcastgrp) &&
		    rt->mfc_stall != 0) {
			if (nstl++)
				log(LOG_ERR, "add_mfc %s o %x g %x p %x dbx %p\n",
				    "multiple kernel entries",
				    ntohl(mfccp->mfcc_origin.s_addr),
				    ntohl(mfccp->mfcc_mcastgrp.s_addr),
				    mfccp->mfcc_parent, rt->mfc_stall);

			if (mrtdebug & DEBUG_MFC)
				log(LOG_DEBUG,"add_mfc o %x g %x p %x dbg %p\n",
				    ntohl(mfccp->mfcc_origin.s_addr),
				    ntohl(mfccp->mfcc_mcastgrp.s_addr),
				    mfccp->mfcc_parent, rt->mfc_stall);

			if (rt->mfc_expire)
				nexpire[hash]--;

			rte = rt->mfc_stall;
			update_mfc(mfccp, rt);

			/* free packets Qed at the end of this entry */
			for (; rte != 0; rte = nrte) {
				nrte = rte->next;
#ifdef RSVP_ISI
				ip_mdq(rte->m, rte->ifp, rt, -1);
#else
				ip_mdq(rte->m, rte->ifp, rt);
#endif /* RSVP_ISI */
				m_freem(rte->m);
#ifdef UPCALL_TIMING
				collate(&rte->t);
#endif /* UPCALL_TIMING */
				free(rte, M_MRTABLE);
			}
		}
	}

	if (nstl == 0) {
		/*
		 * No mfc; make a new one
		 */
		if (mrtdebug & DEBUG_MFC)
			log(LOG_DEBUG,"add_mfc no upcall o %x g %x p %x\n",
			    ntohl(mfccp->mfcc_origin.s_addr),
			    ntohl(mfccp->mfcc_mcastgrp.s_addr),
			    mfccp->mfcc_parent);
	
		rt = (struct mfc *)malloc(sizeof(*rt), M_MRTABLE, M_NOWAIT);
		if (rt == 0) {
			splx(s);
			return (ENOBUFS);
		}

		rt->mfc_origin = mfccp->mfcc_origin;
		rt->mfc_mcastgrp = mfccp->mfcc_mcastgrp;
		/* initialize pkt counters per src-grp */
		rt->mfc_pkt_cnt = 0;
		rt->mfc_byte_cnt = 0;
		rt->mfc_wrong_if = 0;
		timerclear(&rt->mfc_last_assert);
		update_mfc(mfccp, rt);
	    
		/* insert new entry at head of hash chain */
		LIST_INSERT_HEAD(&mfchashtbl[hash], rt, mfc_hash);
	}

	splx(s);
	return (0);
}

#ifdef UPCALL_TIMING
/*
 * collect delay statistics on the upcalls 
 */
static void collate(t)
struct timeval *t;
{
    u_int32_t d;
    struct timeval tp;
    u_int32_t delta;
    
    microtime(&tp);
    
    if (timercmp(t, &tp, <)) {
	TV_DELTA(tp, *t, delta);
	
	d = delta >> 10;
	if (d > 50)
	    d = 50;
	
	++upcall_data[d];
    }
}
#endif /* UPCALL_TIMING */

/*
 * Delete an mfc entry
 */
static int
del_mfc(m)
	struct mbuf *m;
{
	struct mfcctl *mfccp;
	struct mfc *rt;
	int s;

	if (m == 0 || m->m_len < sizeof(struct mfcctl))
		return (EINVAL);

	mfccp = mtod(m, struct mfcctl *);

	if (mrtdebug & DEBUG_MFC)
		log(LOG_DEBUG, "del_mfc origin %x mcastgrp %x\n",
		    ntohl(mfccp->mfcc_origin.s_addr),
		    ntohl(mfccp->mfcc_mcastgrp.s_addr));

	s = splsoftnet();

	MFCFIND(mfccp->mfcc_origin, mfccp->mfcc_mcastgrp, rt);
	if (rt == 0) {
		splx(s);
		return (EADDRNOTAVAIL);
	}

	LIST_REMOVE(rt, mfc_hash);
	free(rt, M_MRTABLE);

	splx(s);
	return (0);
}

static int
socket_send(s, mm, src)
    struct socket *s;
    struct mbuf *mm;
    struct sockaddr_in *src;
{
    if (s) {
	if (sbappendaddr(&s->so_rcv, sintosa(src), mm, (struct mbuf *)0) != 0) {
	    sorwakeup(s);
	    return (0);
	}
    }
    m_freem(mm);
    return (-1);
}

/*
 * IP multicast forwarding function. This function assumes that the packet
 * pointed to by "ip" has arrived on (or is about to be sent to) the interface
 * pointed to by "ifp", and the packet is to be relayed to other networks
 * that have members of the packet's destination IP multicast group.
 *
 * The packet is returned unscathed to the caller, unless it is
 * erroneous, in which case a non-zero return value tells the caller to
 * discard it.
 */

#define IP_HDR_LEN  20	/* # bytes of fixed IP header (excluding options) */
#define TUNNEL_LEN  12  /* # bytes of IP option for tunnel encapsulation  */

int
#ifdef RSVP_ISI
ip_mforward(m, ifp, imo)
#else
ip_mforward(m, ifp)
#endif /* RSVP_ISI */
    struct mbuf *m;
    struct ifnet *ifp;
#ifdef RSVP_ISI
    struct ip_moptions *imo;
#endif /* RSVP_ISI */
{
    struct ip *ip = mtod(m, struct ip *);
    struct mfc *rt;
    u_char *ipoptions;
    static int srctun = 0;
    struct mbuf *mm;
    int s;
#ifdef RSVP_ISI
    struct vif *vifp;
    vifi_t vifi;
#endif /* RSVP_ISI */

    /*
     * Clear any in-bound checksum flags for this packet.
     */
    m->m_pkthdr.csum_flags = 0;

    if (mrtdebug & DEBUG_FORWARD)
	log(LOG_DEBUG, "ip_mforward: src %x, dst %x, ifp %p\n",
	    ntohl(ip->ip_src.s_addr), ntohl(ip->ip_dst.s_addr), ifp);

    if (ip->ip_hl < (IP_HDR_LEN + TUNNEL_LEN) >> 2 ||
	(ipoptions = (u_char *)(ip + 1))[1] != IPOPT_LSRR) {
	/*
	 * Packet arrived via a physical interface or
	 * an encapuslated tunnel.
	 */
    } else {
	/*
	 * Packet arrived through a source-route tunnel.
	 * Source-route tunnels are no longer supported.
	 */
	if ((srctun++ % 1000) == 0)
	    log(LOG_ERR, "ip_mforward: received source-routed packet from %x\n",
		ntohl(ip->ip_src.s_addr));

	return (1);
    }

#ifdef RSVP_ISI
    if (imo && ((vifi = imo->imo_multicast_vif) < numvifs)) {
	if (ip->ip_ttl < 255)
	    ip->ip_ttl++;	/* compensate for -1 in *_send routines */
	if (rsvpdebug && ip->ip_p == IPPROTO_RSVP) {
	    vifp = viftable + vifi;
	    printf("Sending IPPROTO_RSVP from %x to %x on vif %d (%s%s)\n",
		ntohl(ip->ip_src), ntohl(ip->ip_dst), vifi,
		(vifp->v_flags & VIFF_TUNNEL) ? "tunnel on " : "",
		vifp->v_ifp->if_xname);
	}
	return (ip_mdq(m, ifp, (struct mfc *)0, vifi));
    }
    if (rsvpdebug && ip->ip_p == IPPROTO_RSVP) {
	printf("Warning: IPPROTO_RSVP from %x to %x without vif option\n",
	    ntohl(ip->ip_src), ntohl(ip->ip_dst));
    }
#endif /* RSVP_ISI */

    /*
     * Don't forward a packet with time-to-live of zero or one,
     * or a packet destined to a local-only group.
     */
    if (ip->ip_ttl <= 1 ||
	IN_LOCAL_GROUP(ip->ip_dst.s_addr))
	return (0);

    /*
     * Determine forwarding vifs from the forwarding cache table
     */
    s = splsoftnet();
    MFCFIND(ip->ip_src, ip->ip_dst, rt);

    /* Entry exists, so forward if necessary */
    if (rt != 0) {
	splx(s);
#ifdef RSVP_ISI
	return (ip_mdq(m, ifp, rt, -1));
#else
	return (ip_mdq(m, ifp, rt));
#endif /* RSVP_ISI */
    } else {
	/*
	 * If we don't have a route for packet's origin,
	 * Make a copy of the packet &
	 * send message to routing daemon
	 */

	struct mbuf *mb0;
	struct rtdetq *rte;
	u_int32_t hash;
	int hlen = ip->ip_hl << 2;
#ifdef UPCALL_TIMING
	struct timeval tp;

	microtime(&tp);
#endif /* UPCALL_TIMING */

	mrtstat.mrts_no_route++;
	if (mrtdebug & (DEBUG_FORWARD | DEBUG_MFC))
	    log(LOG_DEBUG, "ip_mforward: no rte s %x g %x\n",
		ntohl(ip->ip_src.s_addr),
		ntohl(ip->ip_dst.s_addr));

	/*
	 * Allocate mbufs early so that we don't do extra work if we are
	 * just going to fail anyway.  Make sure to pullup the header so
	 * that other people can't step on it.
	 */
	rte = (struct rtdetq *)malloc(sizeof(*rte), M_MRTABLE, M_NOWAIT);
	if (rte == 0) {
	    splx(s);
	    return (ENOBUFS);
	}
	mb0 = m_copy(m, 0, M_COPYALL);
	M_PULLUP(mb0, hlen);
	if (mb0 == 0) {
	    free(rte, M_MRTABLE);
	    splx(s);
	    return (ENOBUFS);
	}
	    
	/* is there an upcall waiting for this packet? */
	hash = MFCHASH(ip->ip_src, ip->ip_dst);
	for (rt = mfchashtbl[hash].lh_first; rt; rt = rt->mfc_hash.le_next) {
	    if (in_hosteq(ip->ip_src, rt->mfc_origin) &&
		in_hosteq(ip->ip_dst, rt->mfc_mcastgrp) &&
		rt->mfc_stall != 0)
		break;
	}

	if (rt == 0) {
	    int i;
	    struct igmpmsg *im;

	    /* no upcall, so make a new entry */
	    rt = (struct mfc *)malloc(sizeof(*rt), M_MRTABLE, M_NOWAIT);
	    if (rt == 0) {
		free(rte, M_MRTABLE);
		m_freem(mb0);
		splx(s);
		return (ENOBUFS);
	    }
	    /* Make a copy of the header to send to the user level process */
	    mm = m_copy(m, 0, hlen);
	    M_PULLUP(mm, hlen);
	    if (mm == 0) {
		free(rte, M_MRTABLE);
		m_freem(mb0);
		free(rt, M_MRTABLE);
		splx(s);
		return (ENOBUFS);
	    }

	    /* 
	     * Send message to routing daemon to install 
	     * a route into the kernel table
	     */
	    sin.sin_addr = ip->ip_src;
	    
	    im = mtod(mm, struct igmpmsg *);
	    im->im_msgtype	= IGMPMSG_NOCACHE;
	    im->im_mbz		= 0;

	    mrtstat.mrts_upcalls++;

	    if (socket_send(ip_mrouter, mm, &sin) < 0) {
		log(LOG_WARNING, "ip_mforward: ip_mrouter socket queue full\n");
		++mrtstat.mrts_upq_sockfull;
		free(rte, M_MRTABLE);
		m_freem(mb0);
		free(rt, M_MRTABLE);
		splx(s);
		return (ENOBUFS);
	    }

	    /* insert new entry at head of hash chain */
	    rt->mfc_origin = ip->ip_src;
	    rt->mfc_mcastgrp = ip->ip_dst;
	    rt->mfc_pkt_cnt = 0;
	    rt->mfc_byte_cnt = 0;
	    rt->mfc_wrong_if = 0;
	    rt->mfc_expire = UPCALL_EXPIRE;
	    nexpire[hash]++;
	    for (i = 0; i < numvifs; i++)
		rt->mfc_ttls[i] = 0;
	    rt->mfc_parent = -1;

	    /* link into table */
	    LIST_INSERT_HEAD(&mfchashtbl[hash], rt, mfc_hash);
	    /* Add this entry to the end of the queue */
	    rt->mfc_stall = rte;
	} else {
	    /* determine if q has overflowed */
	    struct rtdetq **p;
	    int npkts = 0;

	    for (p = &rt->mfc_stall; *p != 0; p = &(*p)->next)
		if (++npkts > MAX_UPQ) {
		    mrtstat.mrts_upq_ovflw++;
		    free(rte, M_MRTABLE);
		    m_freem(mb0);
		    splx(s);
		    return (0);
	        }

	    /* Add this entry to the end of the queue */
	    *p = rte;
	}

	rte->next		= 0;
	rte->m 			= mb0;
	rte->ifp 		= ifp;
#ifdef UPCALL_TIMING
	rte->t			= tp;
#endif /* UPCALL_TIMING */


	splx(s);

	return (0);
    }
}


/*ARGSUSED*/
static void
expire_upcalls(v)
	void *v;
{
	int i;
	int s;

	s = splsoftnet();

	for (i = 0; i < MFCTBLSIZ; i++) {
		struct mfc *rt, *nrt;

		if (nexpire[i] == 0)
			continue;

		for (rt = mfchashtbl[i].lh_first; rt; rt = nrt) {
			nrt = rt->mfc_hash.le_next;

			if (rt->mfc_expire == 0 ||
			    --rt->mfc_expire > 0)
				continue;
			nexpire[i]--;

			++mrtstat.mrts_cache_cleanups;
			if (mrtdebug & DEBUG_EXPIRE)
				log(LOG_DEBUG,
				    "expire_upcalls: expiring (%x %x)\n",
				    ntohl(rt->mfc_origin.s_addr),
				    ntohl(rt->mfc_mcastgrp.s_addr));

			expire_mfc(rt);
		}
	}

	splx(s);
	callout_reset(&expire_upcalls_ch, EXPIRE_TIMEOUT,
	    expire_upcalls, NULL);
}

/*
 * Packet forwarding routine once entry in the cache is made
 */
static int
#ifdef RSVP_ISI
ip_mdq(m, ifp, rt, xmt_vif)
#else
ip_mdq(m, ifp, rt)
#endif /* RSVP_ISI */
    struct mbuf *m;
    struct ifnet *ifp;
    struct mfc *rt;
#ifdef RSVP_ISI
    vifi_t xmt_vif;
#endif /* RSVP_ISI */
{
    struct ip  *ip = mtod(m, struct ip *);
    vifi_t vifi;
    struct vif *vifp;
    int plen = ntohs(ip->ip_len);

/*
 * Macro to send packet on vif.  Since RSVP packets don't get counted on
 * input, they shouldn't get counted on output, so statistics keeping is
 * separate.
 */
#define MC_SEND(ip,vifp,m) {                             \
                if ((vifp)->v_flags & VIFF_TUNNEL)	 \
                    encap_send((ip), (vifp), (m));       \
                else                                     \
                    phyint_send((ip), (vifp), (m));      \
}

#ifdef RSVP_ISI
    /*
     * If xmt_vif is not -1, send on only the requested vif.
     *
     * (since vifi_t is u_short, -1 becomes MAXUSHORT, which > numvifs.
     */
    if (xmt_vif < numvifs) {
        MC_SEND(ip, viftable + xmt_vif, m);
	return (1);
    }
#endif /* RSVP_ISI */

    /*
     * Don't forward if it didn't arrive from the parent vif for its origin.
     */
    vifi = rt->mfc_parent;
    if ((vifi >= numvifs) || (viftable[vifi].v_ifp != ifp)) {
	/* came in the wrong interface */
	if (mrtdebug & DEBUG_FORWARD)
	    log(LOG_DEBUG, "wrong if: ifp %p vifi %d vififp %p\n",
		ifp, vifi, viftable[vifi].v_ifp); 
	++mrtstat.mrts_wrong_if;
	++rt->mfc_wrong_if;
	/*
	 * If we are doing PIM assert processing, and we are forwarding
	 * packets on this interface, and it is a broadcast medium
	 * interface (and not a tunnel), send a message to the routing daemon.
	 */
	if (pim_assert && rt->mfc_ttls[vifi] &&
		(ifp->if_flags & IFF_BROADCAST) &&
		!(viftable[vifi].v_flags & VIFF_TUNNEL)) {
	    struct mbuf *mm;
	    struct igmpmsg *im;
	    int hlen = ip->ip_hl << 2;
	    struct timeval now;
	    u_int32_t delta;

	    microtime(&now);

	    TV_DELTA(rt->mfc_last_assert, now, delta);

	    if (delta > ASSERT_MSG_TIME) {
		mm = m_copy(m, 0, hlen);
		M_PULLUP(mm, hlen);
		if (mm == 0) {
		    return (ENOBUFS);
		}

		rt->mfc_last_assert = now;
		
		im = mtod(mm, struct igmpmsg *);
		im->im_msgtype	= IGMPMSG_WRONGVIF;
		im->im_mbz	= 0;
		im->im_vif	= vifi;

		sin.sin_addr = im->im_src;

		socket_send(ip_mrouter, mm, &sin);
	    }
	}
	return (0);
    }

    /* If I sourced this packet, it counts as output, else it was input. */
    if (in_hosteq(ip->ip_src, viftable[vifi].v_lcl_addr)) {
	viftable[vifi].v_pkt_out++;
	viftable[vifi].v_bytes_out += plen;
    } else {
	viftable[vifi].v_pkt_in++;
	viftable[vifi].v_bytes_in += plen;
    }
    rt->mfc_pkt_cnt++;
    rt->mfc_byte_cnt += plen;

    /*
     * For each vif, decide if a copy of the packet should be forwarded.
     * Forward if:
     *		- the ttl exceeds the vif's threshold
     *		- there are group members downstream on interface
     */
    for (vifp = viftable, vifi = 0; vifi < numvifs; vifp++, vifi++)
	if ((rt->mfc_ttls[vifi] > 0) &&
	    (ip->ip_ttl > rt->mfc_ttls[vifi])) {
	    vifp->v_pkt_out++;
	    vifp->v_bytes_out += plen;
	    MC_SEND(ip, vifp, m);
	}

    return (0);
}

#ifdef RSVP_ISI
/*
 * check if a vif number is legal/ok. This is used by ip_output, to export
 * numvifs there, 
 */
int
legal_vif_num(vif)
    int vif;
{
    if (vif >= 0 && vif < numvifs)
       return (1);
    else
       return (0);
}
#endif /* RSVP_ISI */

static void
phyint_send(ip, vifp, m)
	struct ip *ip;
	struct vif *vifp;
	struct mbuf *m;
{
	struct mbuf *mb_copy;
	int hlen = ip->ip_hl << 2;

	/*
	 * Make a new reference to the packet; make sure that
	 * the IP header is actually copied, not just referenced,
	 * so that ip_output() only scribbles on the copy.
	 */
	mb_copy = m_copy(m, 0, M_COPYALL);
	M_PULLUP(mb_copy, hlen);
	if (mb_copy == 0)
		return;

	if (vifp->v_rate_limit <= 0)
		tbf_send_packet(vifp, mb_copy);
	else
		tbf_control(vifp, mb_copy, mtod(mb_copy, struct ip *), ip->ip_len);
}

static void
encap_send(ip, vifp, m)
	struct ip *ip;
	struct vif *vifp;
	struct mbuf *m;
{
	struct mbuf *mb_copy;
	struct ip *ip_copy;
	int i, len = ip->ip_len + sizeof(multicast_encap_iphdr);

	/*
	 * copy the old packet & pullup it's IP header into the
	 * new mbuf so we can modify it.  Try to fill the new
	 * mbuf since if we don't the ethernet driver will.
	 */
	MGETHDR(mb_copy, M_DONTWAIT, MT_DATA);
	if (mb_copy == 0)
		return;
	mb_copy->m_data += max_linkhdr;
	mb_copy->m_pkthdr.len = len;
	mb_copy->m_len = sizeof(multicast_encap_iphdr);
	
	if ((mb_copy->m_next = m_copy(m, 0, M_COPYALL)) == 0) {
		m_freem(mb_copy);
		return;
	}
	i = MHLEN - max_linkhdr;
	if (i > len)
		i = len;
	mb_copy = m_pullup(mb_copy, i);
	if (mb_copy == 0)
		return;
	
	/*
	 * fill in the encapsulating IP header.
	 */
	ip_copy = mtod(mb_copy, struct ip *);
	*ip_copy = multicast_encap_iphdr;
	ip_copy->ip_id = htons(ip_id++);
	ip_copy->ip_len = len;
	ip_copy->ip_src = vifp->v_lcl_addr;
	ip_copy->ip_dst = vifp->v_rmt_addr;
	
	/*
	 * turn the encapsulated IP header back into a valid one.
	 */
	ip = (struct ip *)((caddr_t)ip_copy + sizeof(multicast_encap_iphdr));
	--ip->ip_ttl;
	HTONS(ip->ip_len);
	HTONS(ip->ip_off);
	ip->ip_sum = 0;
	mb_copy->m_data += sizeof(multicast_encap_iphdr);
	ip->ip_sum = in_cksum(mb_copy, ip->ip_hl << 2);
	mb_copy->m_data -= sizeof(multicast_encap_iphdr);
	
	if (vifp->v_rate_limit <= 0)
		tbf_send_packet(vifp, mb_copy);
	else
		tbf_control(vifp, mb_copy, ip, ip_copy->ip_len);
}

/*
 * De-encapsulate a packet and feed it back through ip input.
 */
static void
#if __STDC__
vif_input(struct mbuf *m, ...)
#else
vif_input(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	int off, proto;
	va_list ap;
	struct ip *ip;
	struct vif *vifp;
	int s;
	struct ifqueue *ifq;

	va_start(ap, m);
	off = va_arg(ap, int);
	proto = va_arg(ap, int);
	va_end(ap);

	vifp = (struct vif *)encap_getarg(m);
	if (!vifp || proto != AF_INET) {
		m_freem(m);
		mrtstat.mrts_bad_tunnel++;
		return;
	}

	ip = mtod(m, struct ip *);

	m_adj(m, off);
	m->m_pkthdr.rcvif = vifp->v_ifp;
	ifq = &ipintrq;
	s = splnet();
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);
		m_freem(m);
	} else {
		IF_ENQUEUE(ifq, m);
		/*
		 * normally we would need a "schednetisr(NETISR_IP)"
		 * here but we were called by ip_input and it is going
		 * to loop back & try to dequeue the packet we just
		 * queued as soon as we return so we avoid the
		 * unnecessary software interrrupt.
		 */
	}
	splx(s);
}

/*
 * Check if the packet should be grabbed by us.
 */
static int
vif_encapcheck(m, off, proto, arg)
	const struct mbuf *m;
	int off;
	int proto;
	void *arg;
{
	struct vif *vifp;
	struct ip ip;

#ifdef DIAGNOSTIC
	if (!arg || proto != IPPROTO_IPV4)
		panic("unexpected arg in vif_encapcheck");
#endif

	/*
	 * do not grab the packet if it's not to a multicast destination or if
	 * we don't have an encapsulating tunnel with the source.
	 * Note:  This code assumes that the remote site IP address
	 * uniquely identifies the tunnel (i.e., that this site has
	 * at most one tunnel with the remote site).
	 */

	/* LINTED const cast */
	m_copydata((struct mbuf *)m, off, sizeof(ip), (caddr_t)&ip);
	if (!IN_MULTICAST(ip.ip_dst.s_addr))
		return 0;

	/* LINTED const cast */
	m_copydata((struct mbuf *)m, 0, sizeof(ip), (caddr_t)&ip);
	if (!in_hosteq(ip.ip_src, last_encap_src)) {
		vifp = (struct vif *)arg;
		if (vifp->v_flags & VIFF_TUNNEL &&
		    in_hosteq(vifp->v_rmt_addr, ip.ip_src))
			;
		else
			return 0;
		last_encap_vif = vifp;
		last_encap_src = ip.ip_src;
	} else
		vifp = last_encap_vif;

	/* 32bit match, since we have checked ip_src only */
	return 32;
}

/*
 * Token bucket filter module
 */
static void
tbf_control(vifp, m, ip, len)
	struct vif *vifp;
	struct mbuf *m;
	struct ip *ip;
	u_int32_t len;
{

	if (len > MAX_BKT_SIZE) {
		/* drop if packet is too large */
		mrtstat.mrts_pkt2large++;
		m_freem(m);
		return;
	}

	tbf_update_tokens(vifp);

	/*
	 * If there are enough tokens, and the queue is empty, send this packet
	 * out immediately.  Otherwise, try to insert it on this vif's queue.
	 */
	if (vifp->tbf_q_len == 0) {
		if (len <= vifp->tbf_n_tok) {
			vifp->tbf_n_tok -= len;
			tbf_send_packet(vifp, m);
		} else {
			/* queue packet and timeout till later */
			tbf_queue(vifp, m);
			callout_reset(&vifp->v_repq_ch, TBF_REPROCESS,
			    tbf_reprocess_q, vifp);
		}
	} else {
		if (vifp->tbf_q_len >= vifp->tbf_max_q_len &&
		    !tbf_dq_sel(vifp, ip)) {
			/* queue length too much, and couldn't make room */
			mrtstat.mrts_q_overflow++;
			m_freem(m);
		} else {
			/* queue length low enough, or made room */
			tbf_queue(vifp, m);
			tbf_process_q(vifp);
		}
	}
}

/* 
 * adds a packet to the queue at the interface
 */
static void
tbf_queue(vifp, m) 
	struct vif *vifp;
	struct mbuf *m;
{
	int s = splsoftnet();

	/* insert at tail */
	*vifp->tbf_t = m;
	vifp->tbf_t = &m->m_nextpkt;
	vifp->tbf_q_len++;

	splx(s);
}


/* 
 * processes the queue at the interface
 */
static void
tbf_process_q(vifp)
	struct vif *vifp;
{
	struct mbuf *m;
	int len;
	int s = splsoftnet();

	/*
	 * Loop through the queue at the interface and send as many packets
	 * as possible.
	 */
	for (m = vifp->tbf_q;
	    m != 0;
	    m = vifp->tbf_q) {
		len = mtod(m, struct ip *)->ip_len;

		/* determine if the packet can be sent */
		if (len <= vifp->tbf_n_tok) {
			/* if so,
			 * reduce no of tokens, dequeue the packet,
			 * send the packet.
			 */
			if ((vifp->tbf_q = m->m_nextpkt) == 0)
				vifp->tbf_t = &vifp->tbf_q;
			--vifp->tbf_q_len;

			m->m_nextpkt = 0;
			vifp->tbf_n_tok -= len;
			tbf_send_packet(vifp, m);
		} else
			break;
	}
	splx(s);
}

static void
tbf_reprocess_q(arg)
	void *arg;
{
	struct vif *vifp = arg;

	if (ip_mrouter == 0) 
		return;

	tbf_update_tokens(vifp);
	tbf_process_q(vifp);

	if (vifp->tbf_q_len != 0)
		callout_reset(&vifp->v_repq_ch, TBF_REPROCESS,
		    tbf_reprocess_q, vifp);
}

/* function that will selectively discard a member of the queue
 * based on the precedence value and the priority
 */
static int
tbf_dq_sel(vifp, ip)
	struct vif *vifp;
	struct ip *ip;
{
	u_int p;
	struct mbuf **mp, *m;
	int s = splsoftnet();

	p = priority(vifp, ip);

	for (mp = &vifp->tbf_q, m = *mp;
	    m != 0;
	    mp = &m->m_nextpkt, m = *mp) {
		if (p > priority(vifp, mtod(m, struct ip *))) {
			if ((*mp = m->m_nextpkt) == 0)
				vifp->tbf_t = mp;
			--vifp->tbf_q_len;

			m_freem(m);
			mrtstat.mrts_drop_sel++;
			splx(s);
			return (1);
		}
	}
	splx(s);
	return (0);
}

static void
tbf_send_packet(vifp, m)
	struct vif *vifp;
	struct mbuf *m;
{
	int error;
	int s = splsoftnet();

	if (vifp->v_flags & VIFF_TUNNEL) {
		/* If tunnel options */
#ifdef IPSEC
		/* Don't lookup socket in forwading case */
		(void)ipsec_setsocket(m, NULL);
#endif
		ip_output(m, (struct mbuf *)0, &vifp->v_route,
			  IP_FORWARDING, (struct ip_moptions *)0);
	} else {
		/* if physical interface option, extract the options and then send */
		struct ip_moptions imo;

		imo.imo_multicast_ifp = vifp->v_ifp;
		imo.imo_multicast_ttl = mtod(m, struct ip *)->ip_ttl - 1;
		imo.imo_multicast_loop = 1;
#ifdef RSVP_ISI
		imo.imo_multicast_vif = -1;
#endif

#ifdef IPSEC
		/* Don't lookup socket in forwading case */
		(void)ipsec_setsocket(m, NULL);
#endif
		error = ip_output(m, (struct mbuf *)0, (struct route *)0,
				  IP_FORWARDING|IP_MULTICASTOPTS, &imo);

		if (mrtdebug & DEBUG_XMIT)
			log(LOG_DEBUG, "phyint_send on vif %ld err %d\n",
			    (long)(vifp-viftable), error);
	}
	splx(s);
}

/* determine the current time and then
 * the elapsed time (between the last time and time now)
 * in milliseconds & update the no. of tokens in the bucket
 */
static void
tbf_update_tokens(vifp)
	struct vif *vifp;
{
	struct timeval tp;
	u_int32_t tm;
	int s = splsoftnet();

	microtime(&tp);

	TV_DELTA(tp, vifp->tbf_last_pkt_t, tm);

	/*
	 * This formula is actually
	 * "time in seconds" * "bytes/second".
	 *
	 * (tm / 1000000) * (v_rate_limit * 1000 * (1000/1024) / 8)
	 *
	 * The (1000/1024) was introduced in add_vif to optimize
	 * this divide into a shift.
	 */
	vifp->tbf_n_tok += tm * vifp->v_rate_limit / 8192;
	vifp->tbf_last_pkt_t = tp;

	if (vifp->tbf_n_tok > MAX_BKT_SIZE)
		vifp->tbf_n_tok = MAX_BKT_SIZE;

	splx(s);
}

static int
priority(vifp, ip)
    struct vif *vifp;
    struct ip *ip;
{
    int prio;

    /* temporary hack; may add general packet classifier some day */
    
    /*
     * The UDP port space is divided up into four priority ranges:
     * [0, 16384)     : unclassified - lowest priority
     * [16384, 32768) : audio - highest priority
     * [32768, 49152) : whiteboard - medium priority
     * [49152, 65536) : video - low priority
     */
    if (ip->ip_p == IPPROTO_UDP) {
	struct udphdr *udp = (struct udphdr *)(((char *)ip) + (ip->ip_hl << 2));

	switch (ntohs(udp->uh_dport) & 0xc000) {
	    case 0x4000:
		prio = 70;
		break;
	    case 0x8000:
		prio = 60;
		break;
	    case 0xc000:
		prio = 55;
		break;
	    default:
		prio = 50;
		break;
	}

	if (tbfdebug > 1)
	    log(LOG_DEBUG, "port %x prio %d\n", ntohs(udp->uh_dport), prio);
    } else
	prio = 50;


    return (prio);
}

/*
 * End of token bucket filter modifications 
 */

#ifdef RSVP_ISI

int
ip_rsvp_vif_init(so, m)
    struct socket *so;
    struct mbuf *m;
{
    int i;
    int s;

    if (rsvpdebug)
	printf("ip_rsvp_vif_init: so_type = %d, pr_protocol = %d\n",
	    so->so_type, so->so_proto->pr_protocol);

    if (so->so_type != SOCK_RAW || so->so_proto->pr_protocol != IPPROTO_RSVP)
	return (EOPNOTSUPP);

    /* Check mbuf. */
    if (m == 0 || m->m_len != sizeof(int)) {
	return (EINVAL);
    }
    i = *(mtod(m, int *));

    if (rsvpdebug)
	printf("ip_rsvp_vif_init: vif = %d rsvp_on = %d\n",i,rsvp_on);

    s = splsoftnet();

    /* Check vif. */
    if (!legal_vif_num(i)) {
	splx(s);
	return (EADDRNOTAVAIL);
    }

    /* Check if socket is available. */
    if (viftable[i].v_rsvpd != 0) {
	splx(s);
	return (EADDRINUSE);
    }

    viftable[i].v_rsvpd = so;
    /* This may seem silly, but we need to be sure we don't over-increment
     * the RSVP counter, in case something slips up.
     */
    if (!viftable[i].v_rsvp_on) {
	viftable[i].v_rsvp_on = 1;
	rsvp_on++;
    }

    splx(s);
    return (0);
}

int
ip_rsvp_vif_done(so, m)
    struct socket *so;
    struct mbuf *m;
{
    int i;
    int s;

    if (rsvpdebug)
	printf("ip_rsvp_vif_done: so_type = %d, pr_protocol = %d\n",
	       so->so_type, so->so_proto->pr_protocol);

    if (so->so_type != SOCK_RAW || so->so_proto->pr_protocol != IPPROTO_RSVP)
	return (EOPNOTSUPP);

    /* Check mbuf. */
    if (m == 0 || m->m_len != sizeof(int)) {
	return (EINVAL);
    }
    i = *(mtod(m, int *));

    s = splsoftnet();

    /* Check vif. */
    if (!legal_vif_num(i)) {
	splx(s);
        return (EADDRNOTAVAIL);
    }

    if (rsvpdebug)
	printf("ip_rsvp_vif_done: v_rsvpd = %x so = %x\n",
	    viftable[i].v_rsvpd, so);

    viftable[i].v_rsvpd = 0;
    /* This may seem silly, but we need to be sure we don't over-decrement
     * the RSVP counter, in case something slips up.
     */
    if (viftable[i].v_rsvp_on) {
	viftable[i].v_rsvp_on = 0;
	rsvp_on--;
    }

    splx(s);
    return (0);
}

void
ip_rsvp_force_done(so)
    struct socket *so;
{
    int vifi;
    int s;

    /* Don't bother if it is not the right type of socket. */
    if (so->so_type != SOCK_RAW || so->so_proto->pr_protocol != IPPROTO_RSVP)
	return;

    s = splsoftnet();

    /* The socket may be attached to more than one vif...this
     * is perfectly legal.
     */
    for (vifi = 0; vifi < numvifs; vifi++) {
	if (viftable[vifi].v_rsvpd == so) {
	    viftable[vifi].v_rsvpd = 0;
	    /* This may seem silly, but we need to be sure we don't
	     * over-decrement the RSVP counter, in case something slips up.
	     */
	    if (viftable[vifi].v_rsvp_on) {
		viftable[vifi].v_rsvp_on = 0;
		rsvp_on--;
	    }
	}
    }

    splx(s);
    return;
}

void
rsvp_input(m, ifp)
    struct mbuf *m;
    struct ifnet *ifp;
{
    int vifi;
    struct ip *ip = mtod(m, struct ip *);
    static struct sockaddr_in rsvp_src = { sizeof(sin), AF_INET };
    int s;

    if (rsvpdebug)
	printf("rsvp_input: rsvp_on %d\n",rsvp_on);

    /* Can still get packets with rsvp_on = 0 if there is a local member
     * of the group to which the RSVP packet is addressed.  But in this
     * case we want to throw the packet away.
     */
    if (!rsvp_on) {
	m_freem(m);
	return;
    }

    /* If the old-style non-vif-associated socket is set, then use
     * it and ignore the new ones.
     */
    if (ip_rsvpd != 0) {
	if (rsvpdebug)
	    printf("rsvp_input: Sending packet up old-style socket\n");
	rip_input(m);	/*XXX*/
	return;
    }

    s = splsoftnet();

    if (rsvpdebug)
	printf("rsvp_input: check vifs\n");

    /* Find which vif the packet arrived on. */
    for (vifi = 0; vifi < numvifs; vifi++) {
	if (viftable[vifi].v_ifp == ifp)
	    break;
    }

    if (vifi == numvifs) {
	/* Can't find vif packet arrived on. Drop packet. */
	if (rsvpdebug)
	    printf("rsvp_input: Can't find vif for packet...dropping it.\n");
	m_freem(m);
	splx(s);
	return;
    }

    if (rsvpdebug)
	printf("rsvp_input: check socket\n");

    if (viftable[vifi].v_rsvpd == 0) {
	/* drop packet, since there is no specific socket for this
	 * interface */
	if (rsvpdebug)
	    printf("rsvp_input: No socket defined for vif %d\n",vifi);
	m_freem(m);
	splx(s);
	return;
    }

    rsvp_src.sin_addr = ip->ip_src;

    if (rsvpdebug && m)
	printf("rsvp_input: m->m_len = %d, sbspace() = %d\n",
	       m->m_len,sbspace(&viftable[vifi].v_rsvpd->so_rcv));

    if (socket_send(viftable[vifi].v_rsvpd, m, &rsvp_src) < 0)
	if (rsvpdebug)
	    printf("rsvp_input: Failed to append to socket\n");
    else
	if (rsvpdebug)
	    printf("rsvp_input: send packet up\n");
    
    splx(s);
}
#endif /* RSVP_ISI */
