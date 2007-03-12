/*	$NetBSD: if_ecosubr.c,v 1.21.12.1 2007/03/12 05:59:10 rmind Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
/*
 * Copyright (c) 1982, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if_ethersubr.c	8.2 (Berkeley) 4/4/96
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ecosubr.c,v 1.21.12.1 2007/03/12 05:59:10 rmind Exp $");

#include "bpfilter.h"
#include "opt_inet.h"
#include "opt_pfil_hooks.h"

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_eco.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/route.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <net/ethertypes.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#endif

struct eco_retryparms {
	int	erp_delay;
	int	erp_count;
};

/* Default broadcast address */
static const u_int8_t eco_broadcastaddr[] = { 0xff, 0xff };

static int eco_output(struct ifnet *, struct mbuf *, struct sockaddr *,
    struct rtentry *);
static void eco_input(struct ifnet *, struct mbuf *);
static void eco_start(struct ifnet *);
static int eco_ioctl(struct ifnet *, u_long, void *);

static int eco_interestingp(struct ifnet *ifp, struct mbuf *m);
static struct mbuf *eco_immediate(struct ifnet *ifp, struct mbuf *m);
static struct mbuf *eco_ack(struct ifnet *ifp, struct mbuf *m);

static void eco_defer(struct ifnet *, struct mbuf *, int);
static void eco_retry_free(struct eco_retry *er);
static void eco_retry(void *);

void
eco_ifattach(struct ifnet *ifp, const u_int8_t *lla)
{
	struct ecocom *ec = (void *)ifp;

	ifp->if_type = IFT_ECONET;
	ifp->if_addrlen = ECO_ADDR_LEN;
	ifp->if_hdrlen = ECO_HDR_LEN;
	ifp->if_dlt = DLT_ECONET;
	ifp->if_mtu = ECO_MTU;

	ifp->if_output	 = eco_output;
	ifp->if_input	 = eco_input;
	ifp->if_start	 = eco_start;
	ifp->if_ioctl	 = eco_ioctl;

/*	ifp->if_baudrate...; */
	if_alloc_sadl(ifp);
	memcpy(LLADDR(ifp->if_sadl), lla, ifp->if_addrlen);

	ifp->if_broadcastaddr = eco_broadcastaddr;

	LIST_INIT(&ec->ec_retries);

#if NBPFILTER > 0
	bpfattach(ifp, ifp->if_dlt, ECO_HDR_LEN);
#endif
}

#define senderr(e) do {							\
	error = (e);							\
	goto bad;							\
} while (/*CONSTCOND*/0)

int
eco_init(struct ifnet *ifp) {
	struct ecocom *ec = (struct ecocom *)ifp;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		ec->ec_state = ECO_UNKNOWN;
	return 0;
}

void
eco_stop(struct ifnet *ifp, int disable)
{
	struct ecocom *ec = (struct ecocom *)ifp;

	while (!LIST_EMPTY(&ec->ec_retries))
		eco_retry_free(LIST_FIRST(&ec->ec_retries));
}

static int
eco_output(struct ifnet *ifp, struct mbuf *m0, struct sockaddr *dst,
    struct rtentry *rt0)
{
	struct eco_header ehdr, *eh;
	int error;
	struct mbuf *m = m0, *mcopy = NULL;
	struct rtentry *rt;
	int hdrcmplt;
	int retry_delay, retry_count;
	struct m_tag *mtag;
	struct eco_retryparms *erp;
#ifdef INET
	struct mbuf *m1;
	struct arphdr *ah;
	struct eco_arp *ecah;
#endif
	ALTQ_DECL(struct altq_pktattr pktattr;)

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		senderr(ENETDOWN);
	if ((rt = rt0) != NULL) {
		if ((rt->rt_flags & RTF_UP) == 0) {
			if ((rt0 = rt = rtalloc1(dst, 1)) != NULL) {
				rt->rt_refcnt--;
				if (rt->rt_ifp != ifp)
					return (*rt->rt_ifp->if_output)
							(ifp, m0, dst, rt);
			} else
				senderr(EHOSTUNREACH);
		}
		if ((rt->rt_flags & RTF_GATEWAY) && dst->sa_family != AF_NS) {
			if (rt->rt_gwroute == 0)
				goto lookup;
			if (((rt = rt->rt_gwroute)->rt_flags & RTF_UP) == 0) {
				rtfree(rt); rt = rt0;
			lookup: rt->rt_gwroute = rtalloc1(rt->rt_gateway, 1);
				if ((rt = rt->rt_gwroute) == 0)
					senderr(EHOSTUNREACH);
				/* the "G" test below also prevents rt == rt0 */
				if ((rt->rt_flags & RTF_GATEWAY) ||
				    (rt->rt_ifp != ifp)) {
					rt->rt_refcnt--;
					rt0->rt_gwroute = 0;
					senderr(EHOSTUNREACH);
				}
			}
		}
		if (rt->rt_flags & RTF_REJECT)
			if (rt->rt_rmx.rmx_expire == 0 ||
			    time_second < rt->rt_rmx.rmx_expire)
				senderr(rt == rt0 ? EHOSTDOWN : EHOSTUNREACH);
	}
	/*
	 * If the queueing discipline needs packet classification,
	 * do it before prepending link headers.
	 */
	IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family, &pktattr);

	hdrcmplt = 0;
	retry_delay = hz / 16;
	retry_count = 16;
	switch (dst->sa_family) {
#ifdef INET
	case AF_INET:
		if (m->m_flags & M_BCAST)
                	memcpy(ehdr.eco_dhost, eco_broadcastaddr,
			    ECO_ADDR_LEN);

		else if (!arpresolve(ifp, rt, m, dst, ehdr.eco_dhost))
			return (0);	/* if not yet resolved */
		/* If broadcasting on a simplex interface, loopback a copy */
		if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX))
			mcopy = m_copy(m, 0, (int)M_COPYALL);
		ehdr.eco_port = ECO_PORT_IP;
		ehdr.eco_control = ECO_CTL_IP;
		break;

	case AF_ARP:
		ah = mtod(m, struct arphdr *);

		if (ntohs(ah->ar_pro) != ETHERTYPE_IP)
			return EAFNOSUPPORT;
		ehdr.eco_port = ECO_PORT_IP;
		switch (ntohs(ah->ar_op)) {
		case ARPOP_REQUEST:
			ehdr.eco_control = ECO_CTL_ARP_REQUEST;
			break;
		case ARPOP_REPLY:
			ehdr.eco_control = ECO_CTL_ARP_REPLY;
			break;
		default:
			return EOPNOTSUPP;
		}

		if (m->m_flags & M_BCAST)
			memcpy(ehdr.eco_dhost, eco_broadcastaddr,
			    ECO_ADDR_LEN);
		else
			memcpy(ehdr.eco_dhost, ar_tha(ah), ECO_ADDR_LEN);

		MGETHDR(m1, M_DONTWAIT, MT_DATA);
		if (m1 == NULL)
			senderr(ENOBUFS);
		M_MOVE_PKTHDR(m1, m);
		m1->m_len = sizeof(*ecah);
		m1->m_pkthdr.len = m1->m_len;
		MH_ALIGN(m1, m1->m_len);
		ecah = mtod(m1, struct eco_arp *);
		memset(ecah, 0, m1->m_len);
		memcpy(ecah->ecar_spa, ar_spa(ah), ah->ar_pln);
		memcpy(ecah->ecar_tpa, ar_tpa(ah), ah->ar_pln);
		m_freem(m);
		m = m1;
		break;
#endif
	case pseudo_AF_HDRCMPLT:
		hdrcmplt = 1;
		/* FALLTHROUGH */
	case AF_UNSPEC:
		eh = (struct eco_header *)dst->sa_data;
		ehdr = *eh;
		break;
	default:
		log(LOG_ERR, "%s: can't handle af%d\n", ifp->if_xname,
		    dst->sa_family);
		senderr(EAFNOSUPPORT);
	}

	if (mcopy)
		(void) looutput(ifp, mcopy, dst, rt);

	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */
	M_PREPEND(m, sizeof (struct eco_header), M_DONTWAIT);
	if (m == 0)
		senderr(ENOBUFS);
	eh = mtod(m, struct eco_header *);
	*eh = ehdr;
	if (!hdrcmplt)
		memcpy(eh->eco_shost, LLADDR(ifp->if_sadl),
		    ECO_ADDR_LEN);

	if ((m->m_flags & M_BCAST) == 0) {
		/* Attach retry info to packet. */
		mtag = m_tag_get(PACKET_TAG_ECO_RETRYPARMS,
		    sizeof(struct eco_retryparms), M_NOWAIT);
		if (mtag == NULL)
			senderr(ENOBUFS);
		erp = (struct eco_retryparms *)(mtag + 1);
		erp->erp_delay = retry_delay;
		erp->erp_count = retry_count;
	}

#ifdef PFIL_HOOKS
	if ((error = pfil_run_hooks(&ifp->if_pfil, &m, ifp, PFIL_OUT)) != 0)
		return (error);
	if (m == NULL)
		return (0);
#endif

	return ifq_enqueue(ifp, m ALTQ_COMMA ALTQ_DECL(&pktattr));

bad:
	if (m)
		m_freem(m);
	return error;
}

/*
 * Given a scout, decide if we want the rest of the packet.
 */
static int
eco_interestingp(struct ifnet *ifp, struct mbuf *m)
{
	struct eco_header *eh;

	eh = mtod(m, struct eco_header *);
	switch (eh->eco_port) {
#ifdef INET
	case ECO_PORT_IP:
		return 1;
#endif
	}
	return 0;
}

static void
eco_input(struct ifnet *ifp, struct mbuf *m)
{
	struct ifqueue *inq;
	struct eco_header ehdr, *eh;
	int s, i;
#ifdef INET
	struct arphdr *ah;
	struct eco_arp *ecah;
	struct mbuf *m1;
#endif

#ifdef PFIL_HOOKS
	if (pfil_run_hooks(&ifp->if_pfil, &m, ifp, PFIL_IN) != 0)
		return;
	if (m == NULL)
		return;
#endif

	/* Copy the mbuf header and trim it off. */
	/* XXX use m_split? */
	eh = &ehdr;
	m_copydata(m, 0, ECO_HDR_LEN, (void *)eh);
	m_adj(m, ECO_HDR_LEN);

	switch (eh->eco_port) {
#ifdef INET
	case ECO_PORT_IP:
		switch (eh->eco_control) {
		case ECO_CTL_IP:
			schednetisr(NETISR_IP);
			inq = &ipintrq;
			break;
		case ECO_CTL_ARP_REQUEST:
		case ECO_CTL_ARP_REPLY:
			/*
			 * ARP over Econet is strange, because Econet only
			 * supports 8 bytes of data in a broadcast packet.
			 * To cope with this, only the source and destination
			 * IP addresses are actually contained in the packet
			 * and we have to infer the rest and build a fake ARP
			 * packet to pass upwards.
			 */
			if (m->m_pkthdr.len != sizeof(struct eco_arp))
				goto drop;
			if (m->m_len < sizeof(struct eco_arp)) {
				m = m_pullup(m, sizeof(struct eco_arp));
				if (m == NULL) goto drop;
			}
			ecah = mtod(m, struct eco_arp *);
			/* This code derived from arprequest() */
	       		MGETHDR(m1, M_DONTWAIT, MT_DATA);
			if (m1 == NULL)
				goto drop;
			M_MOVE_PKTHDR(m1, m);
			m1->m_len = sizeof(*ah) + 2*sizeof(struct in_addr) +
			    2*ifp->if_data.ifi_addrlen;
			m1->m_pkthdr.len = m1->m_len;
			MH_ALIGN(m1, m1->m_len);
			ah = mtod(m1, struct arphdr *);
			bzero((void *)ah, m1->m_len);
			ah->ar_pro = htons(ETHERTYPE_IP);
			ah->ar_hln = ifp->if_data.ifi_addrlen;
			ah->ar_pln = sizeof(struct in_addr);
			if (eh->eco_control == ECO_CTL_ARP_REQUEST)
				ah->ar_op = htons(ARPOP_REQUEST);
			else
				ah->ar_op = htons(ARPOP_REPLY);
			memcpy(ar_sha(ah), eh->eco_shost, ah->ar_hln);
			memcpy(ar_tha(ah), eh->eco_dhost, ah->ar_hln);
			memcpy(ar_spa(ah), ecah->ecar_spa, ah->ar_pln);
			memcpy(ar_tpa(ah), ecah->ecar_tpa, ah->ar_pln);
			m_freem(m);
			m = m1;
			schednetisr(NETISR_ARP);
			inq = &arpintrq;
			break;
		case ECO_CTL_IPBCAST_REQUEST:
		{
			struct sockaddr_storage dst_store;
			struct sockaddr *dst = (struct sockaddr *)&dst_store;

			/* Queue? */
			memcpy(eh->eco_dhost, eh->eco_shost, ECO_ADDR_LEN);
			eh->eco_control = ECO_CTL_IPBCAST_REPLY;
			/* dst->sa_len??? */
			dst->sa_family = AF_UNSPEC;
			memcpy(dst->sa_data, eh, ECO_HDR_LEN);
			ifp->if_output(ifp, m, dst, NULL);
			return;
		}
		default:
			printf("%s: unknown IP stn %s ctl 0x%02x len %d:",
			    ifp->if_xname, eco_sprintf(eh->eco_shost),
			    eh->eco_control, m->m_pkthdr.len);
			if (m->m_len == 0) {
				m = m_pullup(m, 1);
				if (m == 0) {
					printf("\n");
					goto drop;
				}
			}
			for (i = 0; i < m->m_len; i++)
				printf(" %02x", mtod(m, u_int8_t *)[i]);
			printf("\n");
			goto drop;
		}
		break;
#endif
	default:
		printf("%s: unknown port stn %s port 0x%02x ctl 0x%02x\n",
		    ifp->if_xname, eco_sprintf(eh->eco_shost),
		    eh->eco_port, eh->eco_control);
	drop:
		m_freem(m);
		return;
	}

	s = splnet();
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
	} else
		IF_ENQUEUE(inq, m);
	splx(s);
}

static void
eco_start(struct ifnet *ifp)
{
	struct ecocom *ec = (void *)ifp;
	struct mbuf *m;
	struct eco_header *eh;

	if (ec->ec_state != ECO_IDLE) return;
	IFQ_DEQUEUE(&ifp->if_snd, m);
	if (m == NULL) return;
	if (ec->ec_claimwire(ifp) == 0) {
		eh = mtod(m, struct eco_header *);
		if (eh->eco_port == ECO_PORT_IMMEDIATE) {
			ec->ec_txframe(ifp, m);
			ec->ec_state = ECO_IMMED_SENT;
		} else if (eh->eco_dhost[0] == 255) {
			ec->ec_txframe(ifp, m);
			ec->ec_state = ECO_DONE;
		} else {
			ec->ec_packet = m;
			m = m_copym(m, 0, ECO_HDR_LEN, M_DONTWAIT);
			if (m == NULL) {
				m_freem(ec->ec_packet);
				ec->ec_packet = NULL;
				return;
			}
			ec->ec_txframe(ifp, m);
			ec->ec_state = ECO_SCOUT_SENT;
		}
		ifp->if_flags |= IFF_OACTIVE;
	} else {
		log(LOG_ERR, "%s: line jammed\n", ifp->if_xname);
		m_freem(m);
	}
}

static int
eco_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int error = 0;

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			if ((ifp->if_flags & IFF_RUNNING) == 0 &&
			    (error = (*ifp->if_init)(ifp)) != 0)
				break;
			arp_ifinit(ifp, ifa);
			break;
#endif
		default:
			if ((ifp->if_flags & IFF_RUNNING) == 0)
				error = (*ifp->if_init)(ifp);
			break;
		}
		break;
	case SIOCSIFMTU:
		ifp->if_mtu = ifr->ifr_mtu;

		/* Make sure the device notices the MTU change. */
		if (ifp->if_flags & IFF_UP)
			error = (*ifp->if_init)(ifp);
		break;
	case SIOCSIFFLAGS:
		if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_RUNNING) {
			/*
			 * If interface is marked down and it is running,
			 * then stop and disable it.
			 */
			(*ifp->if_stop)(ifp, 1);
		} else if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_UP) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			error = (*ifp->if_init)(ifp);
		} else if ((ifp->if_flags & IFF_UP) != 0) {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect the hardware state.
			 */
			error = (*ifp->if_init)(ifp);
		}
		break;
	default:
		error = ENOTTY;
	}

	return error;
}

/*
 * Handle a raw Econet frame off the interface.  The interface may be
 * flag-filling for a response.
 *
 * May be called from IPL_NET or IPL_SOFTNET.
 */

struct mbuf *
eco_inputframe(struct ifnet *ifp, struct mbuf *m)
{
	struct ecocom *ec = (void *)ifp;
	struct eco_header *eh, *eh0;
	struct mbuf *m0;
	struct mbuf *reply;
	int len;

	eh = mtod(m, struct eco_header *);
	switch (ec->ec_state) {
	case ECO_IDLE: /* Start of a packet (bcast, immed, scout) */
		if (m->m_pkthdr.len < ECO_HDR_LEN) {
			log(LOG_NOTICE, "%s: undersize scout\n",
			    ifp->if_xname);
			goto drop;
		}
		if (memcmp(eh->eco_dhost, eco_broadcastaddr,
		    ECO_ADDR_LEN) == 0) {
			/* Broadcast */
			eco_input(ifp, m);
		} else if (memcmp(eh->eco_dhost, LLADDR(ifp->if_sadl),
		    ECO_ADDR_LEN) == 0) {
			/* Unicast for us */
			if (eh->eco_port == ECO_PORT_IMMEDIATE)
				return eco_immediate(ifp, m);
			else {
				if (eco_interestingp(ifp, m)) {
					reply = eco_ack(ifp, m);
					if (reply == NULL) {
						m_freem(m);
						return NULL;
					}
					ec->ec_state = ECO_SCOUT_RCVD;
					ec->ec_scout = m;
					return reply;
				} else {
					m_freem(m);
					return NULL;
				}
			}
		} else
			/* Not for us.  Throw it away. */
			m_freem(m);
		break;
	case ECO_SCOUT_RCVD: /* Packet data */
		KASSERT(ec->ec_scout != NULL);
		m0 = ec->ec_scout;
		eh0 = mtod(m0, struct eco_header *);
		if (m->m_pkthdr.len < ECO_SHDR_LEN ||
		    memcmp(eh->eco_shost, eh0->eco_shost, ECO_ADDR_LEN) != 0 ||
		    memcmp(eh->eco_dhost, eh0->eco_dhost, ECO_ADDR_LEN) != 0) {
			log(LOG_NOTICE, "%s: garbled data packet header\n",
			    ifp->if_xname);
			goto drop;
		}
		reply = eco_ack(ifp, m);
		/*
		 * Chop off the small header from this frame, and put
		 * the scout (which holds the control byte and port)
		 * in its place.
		 */
		ec->ec_scout = NULL;
		m_adj(m, ECO_SHDR_LEN);
		len = m0->m_pkthdr.len + m->m_pkthdr.len;
		m_cat(m0, m);
		m0->m_pkthdr.len = len;
		ec->ec_state = ECO_DONE;
		eco_input(ifp, m0);
		return reply;
	case ECO_SCOUT_SENT: /* Scout ack */
		KASSERT(ec->ec_packet != NULL);
		m0 = ec->ec_packet;
		eh0 = mtod(m0, struct eco_header *);
		if (m->m_pkthdr.len != ECO_SHDR_LEN ||
		    memcmp(eh->eco_shost, eh0->eco_dhost, ECO_ADDR_LEN) != 0 ||
		    memcmp(eh->eco_dhost, eh0->eco_shost, ECO_ADDR_LEN) != 0) {
			log(LOG_NOTICE, "%s: garbled scout ack\n",
			    ifp->if_xname);
			goto drop;
		}
		m_freem(m);
		/* Chop out the control and port bytes. */
		m0 = m_copym(ec->ec_packet, 0, ECO_SHDR_LEN, M_DONTWAIT);
		if (m0 == NULL) {
			m_freem(ec->ec_packet);
			return NULL;
		}
		m = ec->ec_packet;
		ec->ec_packet = m_copypacket(m, M_DONTWAIT);
		if (ec->ec_packet == NULL) {
			m_freem(m0);
			m_freem(m);
			return NULL;
		}
		m_adj(m, ECO_HDR_LEN);
		len = m0->m_pkthdr.len + m->m_pkthdr.len;
		m_cat(m0, m); /* Doesn't update packet header */
		m0->m_pkthdr.len = len;
		ec->ec_state = ECO_DATA_SENT;
		return m0;
	case ECO_DATA_SENT: /* Data ack */
		KASSERT(ec->ec_packet != NULL);
		m0 = ec->ec_packet;
		eh0 = mtod(m0, struct eco_header *);
		if (m->m_pkthdr.len != ECO_SHDR_LEN ||
		    memcmp(eh->eco_shost, eh0->eco_dhost, ECO_ADDR_LEN) != 0 ||
		    memcmp(eh->eco_dhost, eh0->eco_shost, ECO_ADDR_LEN) != 0) {
			log(LOG_NOTICE, "%s: garbled data ack\n",
			    ifp->if_xname);
			goto drop;
		}
		m_freem(m);
		m_freem(ec->ec_packet);
		ec->ec_packet = NULL;
		ec->ec_state = ECO_DONE;
		return NULL;
	default:
	drop:
		m_freem(m);
		break;
	}
	return NULL;
}

/*
 * Handle an immediate operation, and return the reply, or NULL not to reply.
 * Frees the incoming mbuf.
 */

static struct mbuf *
eco_immediate(struct ifnet *ifp, struct mbuf *m)
{
	struct eco_header *eh, *reh;
	struct mbuf *n;
	static const u_int8_t machinepeek_data[] = { 42, 0, 0, 1 };

	eh = mtod(m, struct eco_header *);
	switch (eh->eco_control) {
	case ECO_CTL_MACHINEPEEK:
		MGETHDR(n, M_DONTWAIT, MT_DATA);
		if (n == NULL)
			goto bad;
		n->m_len = n->m_pkthdr.len = ECO_SHDR_LEN + 4;
		reh = mtod(n, struct eco_header *);
		memcpy(reh->eco_dhost, eh->eco_shost,
		    ECO_ADDR_LEN);
		memcpy(reh->eco_shost, LLADDR(ifp->if_sadl),
		    ECO_ADDR_LEN);
		memcpy(mtod(n, void *) + ECO_SHDR_LEN, machinepeek_data,
		    sizeof(machinepeek_data));
		m_freem(m);
		return n;
	default:
	bad:
		m_freem(m);
		return NULL;
	}
}

/*
 * Generate (and return) an acknowledgement for a frame.  Doesn't free the
 * original frame, since it's probably needed elsewhere.
 */
static struct mbuf *
eco_ack(struct ifnet *ifp, struct mbuf *m)
{
	struct eco_header *eh, *reh;
	struct mbuf *n;

	eh = mtod(m, struct eco_header *);
	MGETHDR(n, M_DONTWAIT, MT_DATA);
	if (n == NULL)
		return NULL;
	n->m_len = n->m_pkthdr.len = ECO_SHDR_LEN;
	reh = mtod(n, struct eco_header *);
	memcpy(reh->eco_dhost, eh->eco_shost, ECO_ADDR_LEN);
	memcpy(reh->eco_shost, LLADDR(ifp->if_sadl), ECO_ADDR_LEN);
	return n;
}

void
eco_inputidle(struct ifnet *ifp)
{
	struct ecocom *ec = (void *)ifp;
	struct mbuf *m;
	struct m_tag *mtag;
	struct eco_retryparms *erp;

	switch (ec->ec_state) {
	case ECO_SCOUT_SENT:
	case ECO_DATA_SENT:
	case ECO_IMMED_SENT:
		/* Outgoing packet failed.  Check if we should retry. */
		m = ec->ec_packet;
		ec->ec_packet = NULL;
		mtag = m_tag_find(m, PACKET_TAG_ECO_RETRYPARMS, NULL);
		if (mtag == NULL)
			m_freem(m);
		else {
			erp = (struct eco_retryparms *)(mtag + 1);
			if (--erp->erp_count > 0)
				eco_defer(ifp, m, erp->erp_delay);
			else
				printf("%s: pkt failed\n", ifp->if_xname);
		}
		break;
	case ECO_SCOUT_RCVD:
		m_freem(ec->ec_scout);
		ec->ec_scout = NULL;
		break;
	default:
		break;
	}
	ec->ec_state = ECO_IDLE;
	ifp->if_start(ifp);
}

/*
 * Convert Econet address to printable (loggable) representation.
 */
char *
eco_sprintf(const u_int8_t *ea)
{
	static char buf[8];

	if (ea[1] == 0)
		snprintf(buf, sizeof(buf), "%d", ea[0]);
	else
		snprintf(buf, sizeof(buf), "%d.%d", ea[1], ea[0]);
	return buf;
}

/*
 * Econet retry handling.
 */
static void
eco_defer(struct ifnet *ifp, struct mbuf *m, int retry_delay)
{
	struct ecocom *ec = (struct ecocom *)ifp;
	struct eco_retry *er;
	int s;

	MALLOC(er, struct eco_retry *, sizeof(*er), M_TEMP, M_NOWAIT);
	if (er == NULL) {
		m_freem(m);
		return;
	}
	callout_init(&er->er_callout);
	er->er_packet = m;
	er->er_ifp = ifp;
	s = splnet();
	LIST_INSERT_HEAD(&ec->ec_retries, er, er_link);
	splx(s);
	callout_reset(&er->er_callout, retry_delay, eco_retry, er);
}

static void
eco_retry_free(struct eco_retry *er)
{
	int s;

	callout_stop(&er->er_callout);
	m_freem(er->er_packet);
	s = splnet();
	LIST_REMOVE(er, er_link);
	splx(s);
	FREE(er, M_TEMP);
}

static void
eco_retry(void *arg)
{
	struct eco_retry *er = arg;
	struct mbuf *m;
	struct ifnet *ifp;

	ifp = er->er_ifp;
	m = er->er_packet;
	LIST_REMOVE(er, er_link);
	(void)ifq_enqueue(ifp, m ALTQ_COMMA ALTQ_DECL(NULL));
	FREE(er, M_TEMP);
}
