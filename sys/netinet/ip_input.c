/*	$NetBSD: ip_input.c,v 1.175 2003/08/22 22:00:37 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Public Access Networks Corporation ("Panix").  It was developed under
 * contract to Panix by Eric Haszlakiewicz and Thor Lancelot Simon.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Copyright (c) 1982, 1986, 1988, 1993
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
 *	@(#)ip_input.c	8.2 (Berkeley) 1/4/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ip_input.c,v 1.175 2003/08/22 22:00:37 itojun Exp $");

#include "opt_gateway.h"
#include "opt_pfil_hooks.h"
#include "opt_ipsec.h"
#include "opt_mrouting.h"
#include "opt_mbuftrace.h"
#include "opt_inet_csum.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <net/pfil.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
/* just for gif_ttl */
#include <netinet/in_gif.h>
#include "gif.h"
#include <net/if_gre.h>
#include "gre.h"

#ifdef MROUTING
#include <netinet/ip_mroute.h>
#endif

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#endif
#ifdef FAST_IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#endif	/* FAST_IPSEC*/

#ifndef	IPFORWARDING
#ifdef GATEWAY
#define	IPFORWARDING	1	/* forward IP packets not for us */
#else /* GATEWAY */
#define	IPFORWARDING	0	/* don't forward IP packets not for us */
#endif /* GATEWAY */
#endif /* IPFORWARDING */
#ifndef	IPSENDREDIRECTS
#define	IPSENDREDIRECTS	1
#endif
#ifndef IPFORWSRCRT
#define	IPFORWSRCRT	1	/* forward source-routed packets */
#endif
#ifndef IPALLOWSRCRT
#define	IPALLOWSRCRT	1	/* allow source-routed packets */
#endif
#ifndef IPMTUDISC
#define IPMTUDISC	1
#endif
#ifndef IPMTUDISCTIMEOUT
#define IPMTUDISCTIMEOUT (10 * 60)	/* as per RFC 1191 */
#endif

/*
 * Note: DIRECTED_BROADCAST is handled this way so that previous
 * configuration using this option will Just Work.
 */
#ifndef IPDIRECTEDBCAST
#ifdef DIRECTED_BROADCAST
#define IPDIRECTEDBCAST	1
#else
#define	IPDIRECTEDBCAST	0
#endif /* DIRECTED_BROADCAST */
#endif /* IPDIRECTEDBCAST */
int	ipforwarding = IPFORWARDING;
int	ipsendredirects = IPSENDREDIRECTS;
int	ip_defttl = IPDEFTTL;
int	ip_forwsrcrt = IPFORWSRCRT;
int	ip_directedbcast = IPDIRECTEDBCAST;
int	ip_allowsrcrt = IPALLOWSRCRT;
int	ip_mtudisc = IPMTUDISC;
int	ip_mtudisc_timeout = IPMTUDISCTIMEOUT;
#ifdef DIAGNOSTIC
int	ipprintfs = 0;
#endif
/*
 * XXX - Setting ip_checkinterface mostly implements the receive side of
 * the Strong ES model described in RFC 1122, but since the routing table
 * and transmit implementation do not implement the Strong ES model,
 * setting this to 1 results in an odd hybrid.
 *
 * XXX - ip_checkinterface currently must be disabled if you use ipnat
 * to translate the destination address to another local interface.
 *
 * XXX - ip_checkinterface must be disabled if you add IP aliases
 * to the loopback interface instead of the interface where the
 * packets for those addresses are received.
 */
int	ip_checkinterface = 0;


struct rttimer_queue *ip_mtudisc_timeout_q = NULL;

extern	struct domain inetdomain;
int	ipqmaxlen = IFQ_MAXLEN;
u_long	in_ifaddrhash;				/* size of hash table - 1 */
int	in_ifaddrentries;			/* total number of addrs */
struct	in_ifaddrhead in_ifaddr;
struct	in_ifaddrhashhead *in_ifaddrhashtbl;
u_long	in_multihash;				/* size of hash table - 1 */
int	in_multientries;			/* total number of addrs */
struct	in_multihashhead *in_multihashtbl;
struct	ifqueue ipintrq;
struct	ipstat	ipstat;
u_int16_t	ip_id;

#ifdef PFIL_HOOKS
struct pfil_head inet_pfil_hook;
#endif

struct ipqhead ipq;
int	ipq_locked;
int	ip_nfragpackets = 0;
int	ip_maxfragpackets = 200;

static __inline int ipq_lock_try __P((void));
static __inline void ipq_unlock __P((void));

static __inline int
ipq_lock_try()
{
	int s;

	/*
	 * Use splvm() -- we're blocking things that would cause
	 * mbuf allocation.
	 */
	s = splvm();
	if (ipq_locked) {
		splx(s);
		return (0);
	}
	ipq_locked = 1;
	splx(s);
	return (1);
}

static __inline void
ipq_unlock()
{
	int s;

	s = splvm();
	ipq_locked = 0;
	splx(s);
}

#ifdef DIAGNOSTIC
#define	IPQ_LOCK()							\
do {									\
	if (ipq_lock_try() == 0) {					\
		printf("%s:%d: ipq already locked\n", __FILE__, __LINE__); \
		panic("ipq_lock");					\
	}								\
} while (/*CONSTCOND*/ 0)
#define	IPQ_LOCK_CHECK()						\
do {									\
	if (ipq_locked == 0) {						\
		printf("%s:%d: ipq lock not held\n", __FILE__, __LINE__); \
		panic("ipq lock check");				\
	}								\
} while (/*CONSTCOND*/ 0)
#else
#define	IPQ_LOCK()		(void) ipq_lock_try()
#define	IPQ_LOCK_CHECK()	/* nothing */
#endif

#define	IPQ_UNLOCK()		ipq_unlock()

struct pool inmulti_pool;
struct pool ipqent_pool;

#ifdef INET_CSUM_COUNTERS
#include <sys/device.h>

struct evcnt ip_hwcsum_bad = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "inet", "hwcsum bad");
struct evcnt ip_hwcsum_ok = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "inet", "hwcsum ok");
struct evcnt ip_swcsum = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "inet", "swcsum");

#define	INET_CSUM_COUNTER_INCR(ev)	(ev)->ev_count++

#else

#define	INET_CSUM_COUNTER_INCR(ev)	/* nothing */

#endif /* INET_CSUM_COUNTERS */

/*
 * We need to save the IP options in case a protocol wants to respond
 * to an incoming packet over the same route if the packet got here
 * using IP source routing.  This allows connection establishment and
 * maintenance when the remote end is on a network that is not known
 * to us.
 */
int	ip_nhops = 0;
static	struct ip_srcrt {
	struct	in_addr dst;			/* final destination */
	char	nop;				/* one NOP to align */
	char	srcopt[IPOPT_OFFSET + 1];	/* OPTVAL, OLEN and OFFSET */
	struct	in_addr route[MAX_IPOPTLEN/sizeof(struct in_addr)];
} ip_srcrt;

static void save_rte __P((u_char *, struct in_addr));

#ifdef MBUFTRACE
struct mowner ip_rx_mowner = { "internet", "rx" };
struct mowner ip_tx_mowner = { "internet", "tx" };
#endif

/*
 * IP initialization: fill in IP protocol switch table.
 * All protocols not implemented in kernel go to raw IP protocol handler.
 */
void
ip_init()
{
	struct protosw *pr;
	int i;

	pool_init(&inmulti_pool, sizeof(struct in_multi), 0, 0, 0, "inmltpl",
	    NULL);
	pool_init(&ipqent_pool, sizeof(struct ipqent), 0, 0, 0, "ipqepl",
	    NULL);

	pr = pffindproto(PF_INET, IPPROTO_RAW, SOCK_RAW);
	if (pr == 0)
		panic("ip_init");
	for (i = 0; i < IPPROTO_MAX; i++)
		ip_protox[i] = pr - inetsw;
	for (pr = inetdomain.dom_protosw;
	    pr < inetdomain.dom_protoswNPROTOSW; pr++)
		if (pr->pr_domain->dom_family == PF_INET &&
		    pr->pr_protocol && pr->pr_protocol != IPPROTO_RAW)
			ip_protox[pr->pr_protocol] = pr - inetsw;
	LIST_INIT(&ipq);
	ip_id = time.tv_sec & 0xffff;
	ipintrq.ifq_maxlen = ipqmaxlen;
	TAILQ_INIT(&in_ifaddr);
	in_ifaddrhashtbl = hashinit(IN_IFADDR_HASH_SIZE, HASH_LIST, M_IFADDR,
	    M_WAITOK, &in_ifaddrhash);
	in_multihashtbl = hashinit(IN_IFADDR_HASH_SIZE, HASH_LIST, M_IPMADDR,
	    M_WAITOK, &in_multihash);
	ip_mtudisc_timeout_q = rt_timer_queue_create(ip_mtudisc_timeout);
#ifdef GATEWAY
	ipflow_init();
#endif

#ifdef PFIL_HOOKS
	/* Register our Packet Filter hook. */
	inet_pfil_hook.ph_type = PFIL_TYPE_AF;
	inet_pfil_hook.ph_af   = AF_INET;
	i = pfil_head_register(&inet_pfil_hook);
	if (i != 0)
		printf("ip_init: WARNING: unable to register pfil hook, "
		    "error %d\n", i);
#endif /* PFIL_HOOKS */

#ifdef INET_CSUM_COUNTERS
	evcnt_attach_static(&ip_hwcsum_bad);
	evcnt_attach_static(&ip_hwcsum_ok);
	evcnt_attach_static(&ip_swcsum);
#endif /* INET_CSUM_COUNTERS */

#ifdef MBUFTRACE
	MOWNER_ATTACH(&ip_tx_mowner);
	MOWNER_ATTACH(&ip_rx_mowner);
#endif /* MBUFTRACE */
}

struct	sockaddr_in ipaddr = { sizeof(ipaddr), AF_INET };
struct	route ipforward_rt;

/*
 * IP software interrupt routine
 */
void
ipintr()
{
	int s;
	struct mbuf *m;

	while (1) {
		s = splnet();
		IF_DEQUEUE(&ipintrq, m);
		splx(s);
		if (m == 0)
			return;
		MCLAIM(m, &ip_rx_mowner);
		ip_input(m);
	}
}

/*
 * Ip input routine.  Checksum and byte swap header.  If fragmented
 * try to reassemble.  Process options.  Pass to next level.
 */
void
ip_input(struct mbuf *m)
{
	struct ip *ip = NULL;
	struct ipq *fp;
	struct in_ifaddr *ia;
	struct ifaddr *ifa;
	struct ipqent *ipqe;
	int hlen = 0, mff, len;
	int downmatch;
	int checkif;
	int srcrt = 0;
#ifdef FAST_IPSEC
	struct m_tag *mtag;
	struct tdb_ident *tdbi;
	struct secpolicy *sp;
	int s, error;
#endif /* FAST_IPSEC */

	MCLAIM(m, &ip_rx_mowner);
#ifdef	DIAGNOSTIC
	if ((m->m_flags & M_PKTHDR) == 0)
		panic("ipintr no HDR");
#endif
#ifdef IPSEC
	/*
	 * should the inner packet be considered authentic?
	 * see comment in ah4_input().
	 */
	if (m) {
		m->m_flags &= ~M_AUTHIPHDR;
		m->m_flags &= ~M_AUTHIPDGM;
	}
#endif

	/*
	 * If no IP addresses have been set yet but the interfaces
	 * are receiving, can't do anything with incoming packets yet.
	 */
	if (TAILQ_FIRST(&in_ifaddr) == 0)
		goto bad;
	ipstat.ips_total++;
	/*
	 * If the IP header is not aligned, slurp it up into a new
	 * mbuf with space for link headers, in the event we forward
	 * it.  Otherwise, if it is aligned, make sure the entire
	 * base IP header is in the first mbuf of the chain.
	 */
	if (IP_HDR_ALIGNED_P(mtod(m, caddr_t)) == 0) {
		if ((m = m_copyup(m, sizeof(struct ip),
				  (max_linkhdr + 3) & ~3)) == NULL) {
			/* XXXJRT new stat, please */
			ipstat.ips_toosmall++;
			return;
		}
	} else if (__predict_false(m->m_len < sizeof (struct ip))) {
		if ((m = m_pullup(m, sizeof (struct ip))) == NULL) {
			ipstat.ips_toosmall++;
			return;
		}
	}
	ip = mtod(m, struct ip *);
	if (ip->ip_v != IPVERSION) {
		ipstat.ips_badvers++;
		goto bad;
	}
	hlen = ip->ip_hl << 2;
	if (hlen < sizeof(struct ip)) {	/* minimum header length */
		ipstat.ips_badhlen++;
		goto bad;
	}
	if (hlen > m->m_len) {
		if ((m = m_pullup(m, hlen)) == 0) {
			ipstat.ips_badhlen++;
			return;
		}
		ip = mtod(m, struct ip *);
	}

	/*
	 * RFC1122: packets with a multicast source address are
	 * not allowed.
	 */
	if (IN_MULTICAST(ip->ip_src.s_addr)) {
		ipstat.ips_badaddr++;
		goto bad;
	}

	/* 127/8 must not appear on wire - RFC1122 */
	if ((ntohl(ip->ip_dst.s_addr) >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET ||
	    (ntohl(ip->ip_src.s_addr) >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET) {
		if ((m->m_pkthdr.rcvif->if_flags & IFF_LOOPBACK) == 0) {
			ipstat.ips_badaddr++;
			goto bad;
		}
	}

	switch (m->m_pkthdr.csum_flags &
		((m->m_pkthdr.rcvif->if_csum_flags_rx & M_CSUM_IPv4) |
		 M_CSUM_IPv4_BAD)) {
	case M_CSUM_IPv4|M_CSUM_IPv4_BAD:
		INET_CSUM_COUNTER_INCR(&ip_hwcsum_bad);
		goto badcsum;

	case M_CSUM_IPv4:
		/* Checksum was okay. */
		INET_CSUM_COUNTER_INCR(&ip_hwcsum_ok);
		break;

	default:
		/* Must compute it ourselves. */
		INET_CSUM_COUNTER_INCR(&ip_swcsum);
		if (in_cksum(m, hlen) != 0)
			goto bad;
		break;
	}

	/* Retrieve the packet length. */
	len = ntohs(ip->ip_len);

	/*
	 * Check for additional length bogosity
	 */
	if (len < hlen) {
	 	ipstat.ips_badlen++;
		goto bad;
	}

	/*
	 * Check that the amount of data in the buffers
	 * is as at least much as the IP header would have us expect.
	 * Trim mbufs if longer than we expect.
	 * Drop packet if shorter than we expect.
	 */
	if (m->m_pkthdr.len < len) {
		ipstat.ips_tooshort++;
		goto bad;
	}
	if (m->m_pkthdr.len > len) {
		if (m->m_len == m->m_pkthdr.len) {
			m->m_len = len;
			m->m_pkthdr.len = len;
		} else
			m_adj(m, len - m->m_pkthdr.len);
	}

#ifdef IPSEC
	/* ipflow (IP fast forwarding) is not compatible with IPsec. */
	m->m_flags &= ~M_CANFASTFWD;
#else
	/*
	 * Assume that we can create a fast-forward IP flow entry
	 * based on this packet.
	 */
	m->m_flags |= M_CANFASTFWD;
#endif

#ifdef PFIL_HOOKS
	/*
	 * Run through list of hooks for input packets.  If there are any
	 * filters which require that additional packets in the flow are
	 * not fast-forwarded, they must clear the M_CANFASTFWD flag.
	 * Note that filters must _never_ set this flag, as another filter
	 * in the list may have previously cleared it.
	 */
	/*
	 * let ipfilter look at packet on the wire,
	 * not the decapsulated packet.
	 */
#ifdef IPSEC
	if (!ipsec_getnhist(m))
#else
	if (1)
#endif
	{
		struct in_addr odst;

		odst = ip->ip_dst;
		if (pfil_run_hooks(&inet_pfil_hook, &m, m->m_pkthdr.rcvif,
		    PFIL_IN) != 0)
			return;
		if (m == NULL)
			return;
		ip = mtod(m, struct ip *);
		hlen = ip->ip_hl << 2;
		srcrt = (odst.s_addr != ip->ip_dst.s_addr);
	}
#endif /* PFIL_HOOKS */

#ifdef ALTQ
	/* XXX Temporary until ALTQ is changed to use a pfil hook */
	if (altq_input != NULL && (*altq_input)(m, AF_INET) == 0) {
		/* packet dropped by traffic conditioner */
		return;
	}
#endif

	/*
	 * Process options and, if not destined for us,
	 * ship it on.  ip_dooptions returns 1 when an
	 * error was detected (causing an icmp message
	 * to be sent and the original packet to be freed).
	 */
	ip_nhops = 0;		/* for source routed packets */
	if (hlen > sizeof (struct ip) && ip_dooptions(m))
		return;

	/*
	 * Enable a consistency check between the destination address
	 * and the arrival interface for a unicast packet (the RFC 1122
	 * strong ES model) if IP forwarding is disabled and the packet
	 * is not locally generated.
	 *
	 * XXX - Checking also should be disabled if the destination
	 * address is ipnat'ed to a different interface.
	 *
	 * XXX - Checking is incompatible with IP aliases added
	 * to the loopback interface instead of the interface where
	 * the packets are received.
	 *
	 * XXX - We need to add a per ifaddr flag for this so that
	 * we get finer grain control.
	 */
	checkif = ip_checkinterface && (ipforwarding == 0) &&
	    (m->m_pkthdr.rcvif != NULL) &&
	    ((m->m_pkthdr.rcvif->if_flags & IFF_LOOPBACK) == 0);

	/*
	 * Check our list of addresses, to see if the packet is for us.
	 *
	 * Traditional 4.4BSD did not consult IFF_UP at all.
	 * The behavior here is to treat addresses on !IFF_UP interface
	 * as not mine.
	 */
	downmatch = 0;
	LIST_FOREACH(ia, &IN_IFADDR_HASH(ip->ip_dst.s_addr), ia_hash) {
		if (in_hosteq(ia->ia_addr.sin_addr, ip->ip_dst)) {
			if (checkif && ia->ia_ifp != m->m_pkthdr.rcvif)
				continue;
			if ((ia->ia_ifp->if_flags & IFF_UP) != 0)
				break;
			else
				downmatch++;
		}
	}
	if (ia != NULL)
		goto ours;
	if (m->m_pkthdr.rcvif->if_flags & IFF_BROADCAST) {
		TAILQ_FOREACH(ifa, &m->m_pkthdr.rcvif->if_addrlist, ifa_list) {
			if (ifa->ifa_addr->sa_family != AF_INET)
				continue;
			ia = ifatoia(ifa);
			if (in_hosteq(ip->ip_dst, ia->ia_broadaddr.sin_addr) ||
			    in_hosteq(ip->ip_dst, ia->ia_netbroadcast) ||
			    /*
			     * Look for all-0's host part (old broadcast addr),
			     * either for subnet or net.
			     */
			    ip->ip_dst.s_addr == ia->ia_subnet ||
			    ip->ip_dst.s_addr == ia->ia_net)
				goto ours;
			/*
			 * An interface with IP address zero accepts
			 * all packets that arrive on that interface.
			 */
			if (in_nullhost(ia->ia_addr.sin_addr))
				goto ours;
		}
	}
	if (IN_MULTICAST(ip->ip_dst.s_addr)) {
		struct in_multi *inm;
#ifdef MROUTING
		extern struct socket *ip_mrouter;

		if (M_READONLY(m)) {
			if ((m = m_pullup(m, hlen)) == 0) {
				ipstat.ips_toosmall++;
				return;
			}
			ip = mtod(m, struct ip *);
		}

		if (ip_mrouter) {
			/*
			 * If we are acting as a multicast router, all
			 * incoming multicast packets are passed to the
			 * kernel-level multicast forwarding function.
			 * The packet is returned (relatively) intact; if
			 * ip_mforward() returns a non-zero value, the packet
			 * must be discarded, else it may be accepted below.
			 *
			 * (The IP ident field is put in the same byte order
			 * as expected when ip_mforward() is called from
			 * ip_output().)
			 */
			if (ip_mforward(m, m->m_pkthdr.rcvif) != 0) {
				ipstat.ips_cantforward++;
				m_freem(m);
				return;
			}

			/*
			 * The process-level routing demon needs to receive
			 * all multicast IGMP packets, whether or not this
			 * host belongs to their destination groups.
			 */
			if (ip->ip_p == IPPROTO_IGMP)
				goto ours;
			ipstat.ips_forward++;
		}
#endif
		/*
		 * See if we belong to the destination multicast group on the
		 * arrival interface.
		 */
		IN_LOOKUP_MULTI(ip->ip_dst, m->m_pkthdr.rcvif, inm);
		if (inm == NULL) {
			ipstat.ips_cantforward++;
			m_freem(m);
			return;
		}
		goto ours;
	}
	if (ip->ip_dst.s_addr == INADDR_BROADCAST ||
	    in_nullhost(ip->ip_dst))
		goto ours;

	/*
	 * Not for us; forward if possible and desirable.
	 */
	if (ipforwarding == 0) {
		ipstat.ips_cantforward++;
		m_freem(m);
	} else {
		/*
		 * If ip_dst matched any of my address on !IFF_UP interface,
		 * and there's no IFF_UP interface that matches ip_dst,
		 * send icmp unreach.  Forwarding it will result in in-kernel
		 * forwarding loop till TTL goes to 0.
		 */
		if (downmatch) {
			icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_HOST, 0, 0);
			ipstat.ips_cantforward++;
			return;
		}
#ifdef IPSEC
		if (ipsec4_in_reject(m, NULL)) {
			ipsecstat.in_polvio++;
			goto bad;
		}
#endif
#ifdef FAST_IPSEC
		mtag = m_tag_find(m, PACKET_TAG_IPSEC_IN_DONE, NULL);
		s = splsoftnet();
		if (mtag != NULL) {
			tdbi = (struct tdb_ident *)(mtag + 1);
			sp = ipsec_getpolicy(tdbi, IPSEC_DIR_INBOUND);
		} else {
			sp = ipsec_getpolicybyaddr(m, IPSEC_DIR_INBOUND,
						   IP_FORWARDING, &error);   
		}
		if (sp == NULL) {	/* NB: can happen if error */
			splx(s);
			/*XXX error stat???*/
			DPRINTF(("ip_input: no SP for forwarding\n"));	/*XXX*/
			goto bad;
		}

		/*
		 * Check security policy against packet attributes.
		 */
		error = ipsec_in_reject(sp, m);
		KEY_FREESP(&sp);
		splx(s);
		if (error) {
			ipstat.ips_cantforward++;
			goto bad;
		}
#endif	/* FAST_IPSEC */

		ip_forward(m, srcrt);
	}
	return;

ours:
	/*
	 * If offset or IP_MF are set, must reassemble.
	 * Otherwise, nothing need be done.
	 * (We could look in the reassembly queue to see
	 * if the packet was previously fragmented,
	 * but it's not worth the time; just let them time out.)
	 */
	if (ip->ip_off & ~htons(IP_DF|IP_RF)) {
		if (M_READONLY(m)) {
			if ((m = m_pullup(m, hlen)) == NULL) {
				ipstat.ips_toosmall++;
				goto bad;
			}
			ip = mtod(m, struct ip *);
		}

		/*
		 * Look for queue of fragments
		 * of this datagram.
		 */
		IPQ_LOCK();
		LIST_FOREACH(fp, &ipq, ipq_q)
			if (ip->ip_id == fp->ipq_id &&
			    in_hosteq(ip->ip_src, fp->ipq_src) &&
			    in_hosteq(ip->ip_dst, fp->ipq_dst) &&
			    ip->ip_p == fp->ipq_p)
				goto found;
		fp = 0;
found:

		/*
		 * Adjust ip_len to not reflect header,
		 * set ipqe_mff if more fragments are expected,
		 * convert offset of this to bytes.
		 */
		ip->ip_len = htons(ntohs(ip->ip_len) - hlen);
		mff = (ip->ip_off & htons(IP_MF)) != 0;
		if (mff) {
		        /*
		         * Make sure that fragments have a data length
			 * that's a non-zero multiple of 8 bytes.
		         */
			if (ntohs(ip->ip_len) == 0 ||
			    (ntohs(ip->ip_len) & 0x7) != 0) {
				ipstat.ips_badfrags++;
				IPQ_UNLOCK();
				goto bad;
			}
		}
		ip->ip_off = htons((ntohs(ip->ip_off) & IP_OFFMASK) << 3);

		/*
		 * If datagram marked as having more fragments
		 * or if this is not the first fragment,
		 * attempt reassembly; if it succeeds, proceed.
		 */
		if (mff || ip->ip_off != htons(0)) {
			ipstat.ips_fragments++;
			ipqe = pool_get(&ipqent_pool, PR_NOWAIT);
			if (ipqe == NULL) {
				ipstat.ips_rcvmemdrop++;
				IPQ_UNLOCK();
				goto bad;
			}
			ipqe->ipqe_mff = mff;
			ipqe->ipqe_m = m;
			ipqe->ipqe_ip = ip;
			m = ip_reass(ipqe, fp);
			if (m == 0) {
				IPQ_UNLOCK();
				return;
			}
			ipstat.ips_reassembled++;
			ip = mtod(m, struct ip *);
			hlen = ip->ip_hl << 2;
			ip->ip_len = htons(ntohs(ip->ip_len) + hlen);
		} else
			if (fp)
				ip_freef(fp);
		IPQ_UNLOCK();
	}

#if defined(IPSEC)
	/*
	 * enforce IPsec policy checking if we are seeing last header.
	 * note that we do not visit this with protocols with pcb layer
	 * code - like udp/tcp/raw ip.
	 */
	if ((inetsw[ip_protox[ip->ip_p]].pr_flags & PR_LASTHDR) != 0 &&
	    ipsec4_in_reject(m, NULL)) {
		ipsecstat.in_polvio++;
		goto bad;
	}
#endif
#if FAST_IPSEC
	/*
	 * enforce IPsec policy checking if we are seeing last header.
	 * note that we do not visit this with protocols with pcb layer
	 * code - like udp/tcp/raw ip.
	 */
	if ((inetsw[ip_protox[ip->ip_p]].pr_flags & PR_LASTHDR) != 0) {
		/*
		 * Check if the packet has already had IPsec processing
		 * done.  If so, then just pass it along.  This tag gets
		 * set during AH, ESP, etc. input handling, before the
		 * packet is returned to the ip input queue for delivery.
		 */ 
		mtag = m_tag_find(m, PACKET_TAG_IPSEC_IN_DONE, NULL);
		s = splsoftnet();
		if (mtag != NULL) {
			tdbi = (struct tdb_ident *)(mtag + 1);
			sp = ipsec_getpolicy(tdbi, IPSEC_DIR_INBOUND);
		} else {
			sp = ipsec_getpolicybyaddr(m, IPSEC_DIR_INBOUND,
						   IP_FORWARDING, &error);   
		}
		if (sp != NULL) {
			/*
			 * Check security policy against packet attributes.
			 */
			error = ipsec_in_reject(sp, m);
			KEY_FREESP(&sp);
		} else {
			/* XXX error stat??? */
			error = EINVAL;
DPRINTF(("ip_input: no SP, packet discarded\n"));/*XXX*/
			goto bad;
		}
		splx(s);
		if (error)
			goto bad;
	}
#endif /* FAST_IPSEC */

	/*
	 * Switch out to protocol's input routine.
	 */
#if IFA_STATS
	if (ia && ip)
		ia->ia_ifa.ifa_data.ifad_inbytes += ntohs(ip->ip_len);
#endif
	ipstat.ips_delivered++;
    {
	int off = hlen, nh = ip->ip_p;

	(*inetsw[ip_protox[nh]].pr_input)(m, off, nh);
	return;
    }
bad:
	m_freem(m);
	return;

badcsum:
	ipstat.ips_badsum++;
	m_freem(m);
}

/*
 * Take incoming datagram fragment and try to
 * reassemble it into whole datagram.  If a chain for
 * reassembly of this datagram already exists, then it
 * is given as fp; otherwise have to make a chain.
 */
struct mbuf *
ip_reass(ipqe, fp)
	struct ipqent *ipqe;
	struct ipq *fp;
{
	struct mbuf *m = ipqe->ipqe_m;
	struct ipqent *nq, *p, *q;
	struct ip *ip;
	struct mbuf *t;
	int hlen = ipqe->ipqe_ip->ip_hl << 2;
	int i, next;

	IPQ_LOCK_CHECK();

	/*
	 * Presence of header sizes in mbufs
	 * would confuse code below.
	 */
	m->m_data += hlen;
	m->m_len -= hlen;

	/*
	 * If first fragment to arrive, create a reassembly queue.
	 */
	if (fp == 0) {
		/*
		 * Enforce upper bound on number of fragmented packets
		 * for which we attempt reassembly;
		 * If maxfrag is 0, never accept fragments.
		 * If maxfrag is -1, accept all fragments without limitation.
		 */
		if (ip_maxfragpackets < 0)
			;
		else if (ip_nfragpackets >= ip_maxfragpackets)
			goto dropfrag;
		ip_nfragpackets++;
		MALLOC(fp, struct ipq *, sizeof (struct ipq),
		    M_FTABLE, M_NOWAIT);
		if (fp == NULL)
			goto dropfrag;
		LIST_INSERT_HEAD(&ipq, fp, ipq_q);
		fp->ipq_ttl = IPFRAGTTL;
		fp->ipq_p = ipqe->ipqe_ip->ip_p;
		fp->ipq_id = ipqe->ipqe_ip->ip_id;
		TAILQ_INIT(&fp->ipq_fragq);
		fp->ipq_src = ipqe->ipqe_ip->ip_src;
		fp->ipq_dst = ipqe->ipqe_ip->ip_dst;
		p = NULL;
		goto insert;
	}

	/*
	 * Find a segment which begins after this one does.
	 */
	for (p = NULL, q = TAILQ_FIRST(&fp->ipq_fragq); q != NULL;
	    p = q, q = TAILQ_NEXT(q, ipqe_q))
		if (ntohs(q->ipqe_ip->ip_off) > ntohs(ipqe->ipqe_ip->ip_off))
			break;

	/*
	 * If there is a preceding segment, it may provide some of
	 * our data already.  If so, drop the data from the incoming
	 * segment.  If it provides all of our data, drop us.
	 */
	if (p != NULL) {
		i = ntohs(p->ipqe_ip->ip_off) + ntohs(p->ipqe_ip->ip_len) -
		    ntohs(ipqe->ipqe_ip->ip_off);
		if (i > 0) {
			if (i >= ntohs(ipqe->ipqe_ip->ip_len))
				goto dropfrag;
			m_adj(ipqe->ipqe_m, i);
			ipqe->ipqe_ip->ip_off =
			    htons(ntohs(ipqe->ipqe_ip->ip_off) + i);
			ipqe->ipqe_ip->ip_len =
			    htons(ntohs(ipqe->ipqe_ip->ip_len) - i);
		}
	}

	/*
	 * While we overlap succeeding segments trim them or,
	 * if they are completely covered, dequeue them.
	 */
	for (; q != NULL &&
	    ntohs(ipqe->ipqe_ip->ip_off) + ntohs(ipqe->ipqe_ip->ip_len) >
	    ntohs(q->ipqe_ip->ip_off); q = nq) {
		i = (ntohs(ipqe->ipqe_ip->ip_off) +
		    ntohs(ipqe->ipqe_ip->ip_len)) - ntohs(q->ipqe_ip->ip_off);
		if (i < ntohs(q->ipqe_ip->ip_len)) {
			q->ipqe_ip->ip_len =
			    htons(ntohs(q->ipqe_ip->ip_len) - i);
			q->ipqe_ip->ip_off =
			    htons(ntohs(q->ipqe_ip->ip_off) + i);
			m_adj(q->ipqe_m, i);
			break;
		}
		nq = TAILQ_NEXT(q, ipqe_q);
		m_freem(q->ipqe_m);
		TAILQ_REMOVE(&fp->ipq_fragq, q, ipqe_q);
		pool_put(&ipqent_pool, q);
	}

insert:
	/*
	 * Stick new segment in its place;
	 * check for complete reassembly.
	 */
	if (p == NULL) {
		TAILQ_INSERT_HEAD(&fp->ipq_fragq, ipqe, ipqe_q);
	} else {
		TAILQ_INSERT_AFTER(&fp->ipq_fragq, p, ipqe, ipqe_q);
	}
	next = 0;
	for (p = NULL, q = TAILQ_FIRST(&fp->ipq_fragq); q != NULL;
	    p = q, q = TAILQ_NEXT(q, ipqe_q)) {
		if (ntohs(q->ipqe_ip->ip_off) != next)
			return (0);
		next += ntohs(q->ipqe_ip->ip_len);
	}
	if (p->ipqe_mff)
		return (0);

	/*
	 * Reassembly is complete.  Check for a bogus message size and
	 * concatenate fragments.
	 */
	q = TAILQ_FIRST(&fp->ipq_fragq);
	ip = q->ipqe_ip;
	if ((next + (ip->ip_hl << 2)) > IP_MAXPACKET) {
		ipstat.ips_toolong++;
		ip_freef(fp);
		return (0);
	}
	m = q->ipqe_m;
	t = m->m_next;
	m->m_next = 0;
	m_cat(m, t);
	nq = TAILQ_NEXT(q, ipqe_q);
	pool_put(&ipqent_pool, q);
	for (q = nq; q != NULL; q = nq) {
		t = q->ipqe_m;
		nq = TAILQ_NEXT(q, ipqe_q);
		pool_put(&ipqent_pool, q);
		m_cat(m, t);
	}

	/*
	 * Create header for new ip packet by
	 * modifying header of first packet;
	 * dequeue and discard fragment reassembly header.
	 * Make header visible.
	 */
	ip->ip_len = htons(next);
	ip->ip_src = fp->ipq_src;
	ip->ip_dst = fp->ipq_dst;
	LIST_REMOVE(fp, ipq_q);
	FREE(fp, M_FTABLE);
	ip_nfragpackets--;
	m->m_len += (ip->ip_hl << 2);
	m->m_data -= (ip->ip_hl << 2);
	/* some debugging cruft by sklower, below, will go away soon */
	if (m->m_flags & M_PKTHDR) { /* XXX this should be done elsewhere */
		int plen = 0;
		for (t = m; t; t = t->m_next)
			plen += t->m_len;
		m->m_pkthdr.len = plen;
	}
	return (m);

dropfrag:
	ipstat.ips_fragdropped++;
	m_freem(m);
	pool_put(&ipqent_pool, ipqe);
	return (0);
}

/*
 * Free a fragment reassembly header and all
 * associated datagrams.
 */
void
ip_freef(fp)
	struct ipq *fp;
{
	struct ipqent *q, *p;

	IPQ_LOCK_CHECK();

	for (q = TAILQ_FIRST(&fp->ipq_fragq); q != NULL; q = p) {
		p = TAILQ_NEXT(q, ipqe_q);
		m_freem(q->ipqe_m);
		TAILQ_REMOVE(&fp->ipq_fragq, q, ipqe_q);
		pool_put(&ipqent_pool, q);
	}
	LIST_REMOVE(fp, ipq_q);
	FREE(fp, M_FTABLE);
	ip_nfragpackets--;
}

/*
 * IP timer processing;
 * if a timer expires on a reassembly
 * queue, discard it.
 */
void
ip_slowtimo()
{
	struct ipq *fp, *nfp;
	int s = splsoftnet();

	IPQ_LOCK();
	for (fp = LIST_FIRST(&ipq); fp != NULL; fp = nfp) {
		nfp = LIST_NEXT(fp, ipq_q);
		if (--fp->ipq_ttl == 0) {
			ipstat.ips_fragtimeout++;
			ip_freef(fp);
		}
	}
	/*
	 * If we are over the maximum number of fragments
	 * (due to the limit being lowered), drain off
	 * enough to get down to the new limit.
	 */
	if (ip_maxfragpackets < 0)
		;
	else {
		while (ip_nfragpackets > ip_maxfragpackets && LIST_FIRST(&ipq))
			ip_freef(LIST_FIRST(&ipq));
	}
	IPQ_UNLOCK();
#ifdef GATEWAY
	ipflow_slowtimo();
#endif
	splx(s);
}

/*
 * Drain off all datagram fragments.
 */
void
ip_drain()
{

	/*
	 * We may be called from a device's interrupt context.  If
	 * the ipq is already busy, just bail out now.
	 */
	if (ipq_lock_try() == 0)
		return;

	while (LIST_FIRST(&ipq) != NULL) {
		ipstat.ips_fragdropped++;
		ip_freef(LIST_FIRST(&ipq));
	}

	IPQ_UNLOCK();
}

/*
 * Do option processing on a datagram,
 * possibly discarding it if bad options are encountered,
 * or forwarding it if source-routed.
 * Returns 1 if packet has been forwarded/freed,
 * 0 if the packet should be processed further.
 */
int
ip_dooptions(m)
	struct mbuf *m;
{
	struct ip *ip = mtod(m, struct ip *);
	u_char *cp, *cp0;
	struct ip_timestamp *ipt;
	struct in_ifaddr *ia;
	int opt, optlen, cnt, off, code, type = ICMP_PARAMPROB, forward = 0;
	struct in_addr dst;
	n_time ntime;

	dst = ip->ip_dst;
	cp = (u_char *)(ip + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			if (cnt < IPOPT_OLEN + sizeof(*cp)) {
				code = &cp[IPOPT_OLEN] - (u_char *)ip;
				goto bad;
			}
			optlen = cp[IPOPT_OLEN];
			if (optlen < IPOPT_OLEN + sizeof(*cp) || optlen > cnt) {
				code = &cp[IPOPT_OLEN] - (u_char *)ip;
				goto bad;
			}
		}
		switch (opt) {

		default:
			break;

		/*
		 * Source routing with record.
		 * Find interface with current destination address.
		 * If none on this machine then drop if strictly routed,
		 * or do nothing if loosely routed.
		 * Record interface address and bring up next address
		 * component.  If strictly routed make sure next
		 * address is on directly accessible net.
		 */
		case IPOPT_LSRR:
		case IPOPT_SSRR:
			if (ip_allowsrcrt == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_NET_PROHIB;
				goto bad;
			}
			if (optlen < IPOPT_OFFSET + sizeof(*cp)) {
				code = &cp[IPOPT_OLEN] - (u_char *)ip;
				goto bad;
			}
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *)ip;
				goto bad;
			}
			ipaddr.sin_addr = ip->ip_dst;
			ia = ifatoia(ifa_ifwithaddr(sintosa(&ipaddr)));
			if (ia == 0) {
				if (opt == IPOPT_SSRR) {
					type = ICMP_UNREACH;
					code = ICMP_UNREACH_SRCFAIL;
					goto bad;
				}
				/*
				 * Loose routing, and not at next destination
				 * yet; nothing to do except forward.
				 */
				break;
			}
			off--;			/* 0 origin */
			if ((off + sizeof(struct in_addr)) > optlen) {
				/*
				 * End of source route.  Should be for us.
				 */
				save_rte(cp, ip->ip_src);
				break;
			}
			/*
			 * locate outgoing interface
			 */
			bcopy((caddr_t)(cp + off), (caddr_t)&ipaddr.sin_addr,
			    sizeof(ipaddr.sin_addr));
			if (opt == IPOPT_SSRR)
				ia = ifatoia(ifa_ifwithaddr(sintosa(&ipaddr)));
			else
				ia = ip_rtaddr(ipaddr.sin_addr);
			if (ia == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_SRCFAIL;
				goto bad;
			}
			ip->ip_dst = ipaddr.sin_addr;
			bcopy((caddr_t)&ia->ia_addr.sin_addr,
			    (caddr_t)(cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			/*
			 * Let ip_intr's mcast routing check handle mcast pkts
			 */
			forward = !IN_MULTICAST(ip->ip_dst.s_addr);
			break;

		case IPOPT_RR:
			if (optlen < IPOPT_OFFSET + sizeof(*cp)) {
				code = &cp[IPOPT_OLEN] - (u_char *)ip;
				goto bad;
			}
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *)ip;
				goto bad;
			}
			/*
			 * If no space remains, ignore.
			 */
			off--;			/* 0 origin */
			if ((off + sizeof(struct in_addr)) > optlen)
				break;
			bcopy((caddr_t)(&ip->ip_dst), (caddr_t)&ipaddr.sin_addr,
			    sizeof(ipaddr.sin_addr));
			/*
			 * locate outgoing interface; if we're the destination,
			 * use the incoming interface (should be same).
			 */
			if ((ia = ifatoia(ifa_ifwithaddr(sintosa(&ipaddr))))
			    == NULL &&
			    (ia = ip_rtaddr(ipaddr.sin_addr)) == NULL) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_HOST;
				goto bad;
			}
			bcopy((caddr_t)&ia->ia_addr.sin_addr,
			    (caddr_t)(cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			break;

		case IPOPT_TS:
			code = cp - (u_char *)ip;
			ipt = (struct ip_timestamp *)cp;
			if (ipt->ipt_len < 4 || ipt->ipt_len > 40) {
				code = (u_char *)&ipt->ipt_len - (u_char *)ip;
				goto bad;
			}
			if (ipt->ipt_ptr < 5) {
				code = (u_char *)&ipt->ipt_ptr - (u_char *)ip;
				goto bad;
			}
			if (ipt->ipt_ptr > ipt->ipt_len - sizeof (int32_t)) {
				if (++ipt->ipt_oflw == 0) {
					code = (u_char *)&ipt->ipt_ptr -
					    (u_char *)ip;
					goto bad;
				}
				break;
			}
			cp0 = (cp + ipt->ipt_ptr - 1);
			switch (ipt->ipt_flg) {

			case IPOPT_TS_TSONLY:
				break;

			case IPOPT_TS_TSANDADDR:
				if (ipt->ipt_ptr - 1 + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len) {
					code = (u_char *)&ipt->ipt_ptr -
					    (u_char *)ip;
					goto bad;
				}
				ipaddr.sin_addr = dst;
				ia = ifatoia(ifaof_ifpforaddr(sintosa(&ipaddr),
				    m->m_pkthdr.rcvif));
				if (ia == 0)
					continue;
				bcopy(&ia->ia_addr.sin_addr,
				    cp0, sizeof(struct in_addr));
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			case IPOPT_TS_PRESPEC:
				if (ipt->ipt_ptr - 1 + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len) {
					code = (u_char *)&ipt->ipt_ptr -
					    (u_char *)ip;
					goto bad;
				}
				bcopy(cp0, &ipaddr.sin_addr,
				    sizeof(struct in_addr));
				if (ifatoia(ifa_ifwithaddr(sintosa(&ipaddr)))
				    == NULL)
					continue;
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			default:
				/* XXX can't take &ipt->ipt_flg */
				code = (u_char *)&ipt->ipt_ptr -
				    (u_char *)ip + 1;
				goto bad;
			}
			ntime = iptime();
			cp0 = (u_char *) &ntime; /* XXX grumble, GCC... */
			bcopy(cp0, (caddr_t)cp + ipt->ipt_ptr - 1,
			    sizeof(n_time));
			ipt->ipt_ptr += sizeof(n_time);
		}
	}
	if (forward) {
		if (ip_forwsrcrt == 0) {
			type = ICMP_UNREACH;
			code = ICMP_UNREACH_SRCFAIL;
			goto bad;
		}
		ip_forward(m, 1);
		return (1);
	}
	return (0);
bad:
	icmp_error(m, type, code, 0, 0);
	ipstat.ips_badoptions++;
	return (1);
}

/*
 * Given address of next destination (final or next hop),
 * return internet address info of interface to be used to get there.
 */
struct in_ifaddr *
ip_rtaddr(dst)
	 struct in_addr dst;
{
	struct sockaddr_in *sin;

	sin = satosin(&ipforward_rt.ro_dst);

	if (ipforward_rt.ro_rt == 0 || !in_hosteq(dst, sin->sin_addr)) {
		if (ipforward_rt.ro_rt) {
			RTFREE(ipforward_rt.ro_rt);
			ipforward_rt.ro_rt = 0;
		}
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(*sin);
		sin->sin_addr = dst;

		rtalloc(&ipforward_rt);
	}
	if (ipforward_rt.ro_rt == 0)
		return ((struct in_ifaddr *)0);
	return (ifatoia(ipforward_rt.ro_rt->rt_ifa));
}

/*
 * Save incoming source route for use in replies,
 * to be picked up later by ip_srcroute if the receiver is interested.
 */
void
save_rte(option, dst)
	u_char *option;
	struct in_addr dst;
{
	unsigned olen;

	olen = option[IPOPT_OLEN];
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf("save_rte: olen %d\n", olen);
#endif /* 0 */
	if (olen > sizeof(ip_srcrt) - (1 + sizeof(dst)))
		return;
	bcopy((caddr_t)option, (caddr_t)ip_srcrt.srcopt, olen);
	ip_nhops = (olen - IPOPT_OFFSET - 1) / sizeof(struct in_addr);
	ip_srcrt.dst = dst;
}

/*
 * Retrieve incoming source route for use in replies,
 * in the same form used by setsockopt.
 * The first hop is placed before the options, will be removed later.
 */
struct mbuf *
ip_srcroute()
{
	struct in_addr *p, *q;
	struct mbuf *m;

	if (ip_nhops == 0)
		return ((struct mbuf *)0);
	m = m_get(M_DONTWAIT, MT_SOOPTS);
	if (m == 0)
		return ((struct mbuf *)0);

	MCLAIM(m, &inetdomain.dom_mowner);
#define OPTSIZ	(sizeof(ip_srcrt.nop) + sizeof(ip_srcrt.srcopt))

	/* length is (nhops+1)*sizeof(addr) + sizeof(nop + srcrt header) */
	m->m_len = ip_nhops * sizeof(struct in_addr) + sizeof(struct in_addr) +
	    OPTSIZ;
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf("ip_srcroute: nhops %d mlen %d", ip_nhops, m->m_len);
#endif

	/*
	 * First save first hop for return route
	 */
	p = &ip_srcrt.route[ip_nhops - 1];
	*(mtod(m, struct in_addr *)) = *p--;
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf(" hops %x", ntohl(mtod(m, struct in_addr *)->s_addr));
#endif

	/*
	 * Copy option fields and padding (nop) to mbuf.
	 */
	ip_srcrt.nop = IPOPT_NOP;
	ip_srcrt.srcopt[IPOPT_OFFSET] = IPOPT_MINOFF;
	bcopy((caddr_t)&ip_srcrt.nop,
	    mtod(m, caddr_t) + sizeof(struct in_addr), OPTSIZ);
	q = (struct in_addr *)(mtod(m, caddr_t) +
	    sizeof(struct in_addr) + OPTSIZ);
#undef OPTSIZ
	/*
	 * Record return path as an IP source route,
	 * reversing the path (pointers are now aligned).
	 */
	while (p >= ip_srcrt.route) {
#ifdef DIAGNOSTIC
		if (ipprintfs)
			printf(" %x", ntohl(q->s_addr));
#endif
		*q++ = *p--;
	}
	/*
	 * Last hop goes to final destination.
	 */
	*q = ip_srcrt.dst;
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf(" %x\n", ntohl(q->s_addr));
#endif
	return (m);
}

/*
 * Strip out IP options, at higher
 * level protocol in the kernel.
 * Second argument is buffer to which options
 * will be moved, and return value is their length.
 * XXX should be deleted; last arg currently ignored.
 */
void
ip_stripoptions(m, mopt)
	struct mbuf *m;
	struct mbuf *mopt;
{
	int i;
	struct ip *ip = mtod(m, struct ip *);
	caddr_t opts;
	int olen;

	olen = (ip->ip_hl << 2) - sizeof (struct ip);
	opts = (caddr_t)(ip + 1);
	i = m->m_len - (sizeof (struct ip) + olen);
	bcopy(opts  + olen, opts, (unsigned)i);
	m->m_len -= olen;
	if (m->m_flags & M_PKTHDR)
		m->m_pkthdr.len -= olen;
	ip->ip_len = htons(ntohs(ip->ip_len) - olen);
	ip->ip_hl = sizeof (struct ip) >> 2;
}

const int inetctlerrmap[PRC_NCMDS] = {
	0,		0,		0,		0,
	0,		EMSGSIZE,	EHOSTDOWN,	EHOSTUNREACH,
	EHOSTUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	EHOSTUNREACH,	0,		0,
	0,		0,		0,		0,
	ENOPROTOOPT
};

/*
 * Forward a packet.  If some error occurs return the sender
 * an icmp packet.  Note we can't always generate a meaningful
 * icmp message because icmp doesn't have a large enough repertoire
 * of codes and types.
 *
 * If not forwarding, just drop the packet.  This could be confusing
 * if ipforwarding was zero but some routing protocol was advancing
 * us as a gateway to somewhere.  However, we must let the routing
 * protocol deal with that.
 *
 * The srcrt parameter indicates whether the packet is being forwarded
 * via a source route.
 */
void
ip_forward(m, srcrt)
	struct mbuf *m;
	int srcrt;
{
	struct ip *ip = mtod(m, struct ip *);
	struct sockaddr_in *sin;
	struct rtentry *rt;
	int error, type = 0, code = 0;
	struct mbuf *mcopy;
	n_long dest;
	struct ifnet *destifp;
#if defined(IPSEC) || defined(FAST_IPSEC)
	struct ifnet dummyifp;
#endif

	/*
	 * We are now in the output path.
	 */
	MCLAIM(m, &ip_tx_mowner);

	/*
	 * Clear any in-bound checksum flags for this packet.
	 */
	m->m_pkthdr.csum_flags = 0;

	dest = 0;
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf("forward: src %2.2x dst %2.2x ttl %x\n",
		    ntohl(ip->ip_src.s_addr),
		    ntohl(ip->ip_dst.s_addr), ip->ip_ttl);
#endif
	if (m->m_flags & (M_BCAST|M_MCAST) || in_canforward(ip->ip_dst) == 0) {
		ipstat.ips_cantforward++;
		m_freem(m);
		return;
	}
	if (ip->ip_ttl <= IPTTLDEC) {
		icmp_error(m, ICMP_TIMXCEED, ICMP_TIMXCEED_INTRANS, dest, 0);
		return;
	}
	ip->ip_ttl -= IPTTLDEC;

	sin = satosin(&ipforward_rt.ro_dst);
	if ((rt = ipforward_rt.ro_rt) == 0 ||
	    !in_hosteq(ip->ip_dst, sin->sin_addr)) {
		if (ipforward_rt.ro_rt) {
			RTFREE(ipforward_rt.ro_rt);
			ipforward_rt.ro_rt = 0;
		}
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(struct sockaddr_in);
		sin->sin_addr = ip->ip_dst;

		rtalloc(&ipforward_rt);
		if (ipforward_rt.ro_rt == 0) {
			icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_HOST, dest, 0);
			return;
		}
		rt = ipforward_rt.ro_rt;
	}

	/*
	 * Save at most 68 bytes of the packet in case
	 * we need to generate an ICMP message to the src.
	 * Pullup to avoid sharing mbuf cluster between m and mcopy.
	 */
	mcopy = m_copym(m, 0, imin(ntohs(ip->ip_len), 68), M_DONTWAIT);
	if (mcopy)
		mcopy = m_pullup(mcopy, ip->ip_hl << 2);

	/*
	 * If forwarding packet using same interface that it came in on,
	 * perhaps should send a redirect to sender to shortcut a hop.
	 * Only send redirect if source is sending directly to us,
	 * and if packet was not source routed (or has any options).
	 * Also, don't send redirect if forwarding using a default route
	 * or a route modified by a redirect.
	 */
	if (rt->rt_ifp == m->m_pkthdr.rcvif &&
	    (rt->rt_flags & (RTF_DYNAMIC|RTF_MODIFIED)) == 0 &&
	    !in_nullhost(satosin(rt_key(rt))->sin_addr) &&
	    ipsendredirects && !srcrt) {
		if (rt->rt_ifa &&
		    (ip->ip_src.s_addr & ifatoia(rt->rt_ifa)->ia_subnetmask) ==
		    ifatoia(rt->rt_ifa)->ia_subnet) {
			if (rt->rt_flags & RTF_GATEWAY)
				dest = satosin(rt->rt_gateway)->sin_addr.s_addr;
			else
				dest = ip->ip_dst.s_addr;
			/*
			 * Router requirements says to only send host
			 * redirects.
			 */
			type = ICMP_REDIRECT;
			code = ICMP_REDIRECT_HOST;
#ifdef DIAGNOSTIC
			if (ipprintfs)
				printf("redirect (%d) to %x\n", code,
				    (u_int32_t)dest);
#endif
		}
	}

	error = ip_output(m, (struct mbuf *)0, &ipforward_rt,
	    (IP_FORWARDING | (ip_directedbcast ? IP_ALLOWBROADCAST : 0)),
	    (struct ip_moptions *)NULL, (struct socket *)NULL);

	if (error)
		ipstat.ips_cantforward++;
	else {
		ipstat.ips_forward++;
		if (type)
			ipstat.ips_redirectsent++;
		else {
			if (mcopy) {
#ifdef GATEWAY
				if (mcopy->m_flags & M_CANFASTFWD)
					ipflow_create(&ipforward_rt, mcopy);
#endif
				m_freem(mcopy);
			}
			return;
		}
	}
	if (mcopy == NULL)
		return;
	destifp = NULL;

	switch (error) {

	case 0:				/* forwarded, but need redirect */
		/* type, code set above */
		break;

	case ENETUNREACH:		/* shouldn't happen, checked above */
	case EHOSTUNREACH:
	case ENETDOWN:
	case EHOSTDOWN:
	default:
		type = ICMP_UNREACH;
		code = ICMP_UNREACH_HOST;
		break;

	case EMSGSIZE:
		type = ICMP_UNREACH;
		code = ICMP_UNREACH_NEEDFRAG;
#if !defined(IPSEC) && !defined(FAST_IPSEC)
		if (ipforward_rt.ro_rt)
			destifp = ipforward_rt.ro_rt->rt_ifp;
#else
		/*
		 * If the packet is routed over IPsec tunnel, tell the
		 * originator the tunnel MTU.
		 *	tunnel MTU = if MTU - sizeof(IP) - ESP/AH hdrsiz
		 * XXX quickhack!!!
		 */
		if (ipforward_rt.ro_rt) {
			struct secpolicy *sp;
			int ipsecerror;
			size_t ipsechdr;
			struct route *ro;

			sp = ipsec4_getpolicybyaddr(mcopy,
			    IPSEC_DIR_OUTBOUND, IP_FORWARDING,
			    &ipsecerror);

			if (sp == NULL)
				destifp = ipforward_rt.ro_rt->rt_ifp;
			else {
				/* count IPsec header size */
				ipsechdr = ipsec4_hdrsiz(mcopy,
				    IPSEC_DIR_OUTBOUND, NULL);

				/*
				 * find the correct route for outer IPv4
				 * header, compute tunnel MTU.
				 *
				 * XXX BUG ALERT
				 * The "dummyifp" code relies upon the fact
				 * that icmp_error() touches only ifp->if_mtu.
				 */
				/*XXX*/
				destifp = NULL;
				if (sp->req != NULL
				 && sp->req->sav != NULL
				 && sp->req->sav->sah != NULL) {
					ro = &sp->req->sav->sah->sa_route;
					if (ro->ro_rt && ro->ro_rt->rt_ifp) {
						dummyifp.if_mtu =
						    ro->ro_rt->rt_rmx.rmx_mtu ?
						    ro->ro_rt->rt_rmx.rmx_mtu :
						    ro->ro_rt->rt_ifp->if_mtu;
						dummyifp.if_mtu -= ipsechdr;
						destifp = &dummyifp;
					}
				}

#ifdef	IPSEC
				key_freesp(sp);
#else
				KEY_FREESP(&sp);
#endif
			}
		}
#endif /*IPSEC*/
		ipstat.ips_cantfrag++;
		break;

	case ENOBUFS:
#if 1
		/*
		 * a router should not generate ICMP_SOURCEQUENCH as
		 * required in RFC1812 Requirements for IP Version 4 Routers.
		 * source quench could be a big problem under DoS attacks,
		 * or if the underlying interface is rate-limited.
		 */
		if (mcopy)
			m_freem(mcopy);
		return;
#else
		type = ICMP_SOURCEQUENCH;
		code = 0;
		break;
#endif
	}
	icmp_error(mcopy, type, code, dest, destifp);
}

void
ip_savecontrol(inp, mp, ip, m)
	struct inpcb *inp;
	struct mbuf **mp;
	struct ip *ip;
	struct mbuf *m;
{

	if (inp->inp_socket->so_options & SO_TIMESTAMP) {
		struct timeval tv;

		microtime(&tv);
		*mp = sbcreatecontrol((caddr_t) &tv, sizeof(tv),
		    SCM_TIMESTAMP, SOL_SOCKET);
		if (*mp)
			mp = &(*mp)->m_next;
	}
	if (inp->inp_flags & INP_RECVDSTADDR) {
		*mp = sbcreatecontrol((caddr_t) &ip->ip_dst,
		    sizeof(struct in_addr), IP_RECVDSTADDR, IPPROTO_IP);
		if (*mp)
			mp = &(*mp)->m_next;
	}
#ifdef notyet
	/*
	 * XXX
	 * Moving these out of udp_input() made them even more broken
	 * than they already were.
	 *	- fenner@parc.xerox.com
	 */
	/* options were tossed already */
	if (inp->inp_flags & INP_RECVOPTS) {
		*mp = sbcreatecontrol((caddr_t) opts_deleted_above,
		    sizeof(struct in_addr), IP_RECVOPTS, IPPROTO_IP);
		if (*mp)
			mp = &(*mp)->m_next;
	}
	/* ip_srcroute doesn't do what we want here, need to fix */
	if (inp->inp_flags & INP_RECVRETOPTS) {
		*mp = sbcreatecontrol((caddr_t) ip_srcroute(),
		    sizeof(struct in_addr), IP_RECVRETOPTS, IPPROTO_IP);
		if (*mp)
			mp = &(*mp)->m_next;
	}
#endif
	if (inp->inp_flags & INP_RECVIF) {
		struct sockaddr_dl sdl;

		sdl.sdl_len = offsetof(struct sockaddr_dl, sdl_data[0]);
		sdl.sdl_family = AF_LINK;
		sdl.sdl_index = m->m_pkthdr.rcvif ?
		    m->m_pkthdr.rcvif->if_index : 0;
		sdl.sdl_nlen = sdl.sdl_alen = sdl.sdl_slen = 0;
		*mp = sbcreatecontrol((caddr_t) &sdl, sdl.sdl_len,
		    IP_RECVIF, IPPROTO_IP);
		if (*mp)
			mp = &(*mp)->m_next;
	}
}

int
ip_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	extern int subnetsarelocal, hostzeroisbroadcast;

	int error, old;

	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return (ENOTDIR);

	switch (name[0]) {
	case IPCTL_FORWARDING:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &ipforwarding));
	case IPCTL_SENDREDIRECTS:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
			&ipsendredirects));
	case IPCTL_DEFTTL:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &ip_defttl));
#ifdef notyet
	case IPCTL_DEFMTU:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &ip_mtu));
#endif
	case IPCTL_FORWSRCRT:
		/* Don't allow this to change in a secure environment.  */
		if (securelevel > 0)
			return (sysctl_rdint(oldp, oldlenp, newp,
			    ip_forwsrcrt));
		else
			return (sysctl_int(oldp, oldlenp, newp, newlen,
			    &ip_forwsrcrt));
	case IPCTL_DIRECTEDBCAST:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &ip_directedbcast));
	case IPCTL_ALLOWSRCRT:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &ip_allowsrcrt));
	case IPCTL_SUBNETSARELOCAL:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &subnetsarelocal));
	case IPCTL_MTUDISC:
		error = sysctl_int(oldp, oldlenp, newp, newlen,
		    &ip_mtudisc);
		if (error == 0 && ip_mtudisc == 0)
			rt_timer_queue_remove_all(ip_mtudisc_timeout_q, TRUE);
		return error;
	case IPCTL_ANONPORTMIN:
		old = anonportmin;
		error = sysctl_int(oldp, oldlenp, newp, newlen, &anonportmin);
		if (anonportmin >= anonportmax || anonportmin < 0
		    || anonportmin > 65535
#ifndef IPNOPRIVPORTS
		    || anonportmin < IPPORT_RESERVED
#endif
		    ) {
			anonportmin = old;
			return (EINVAL);
		}
		return (error);
	case IPCTL_ANONPORTMAX:
		old = anonportmax;
		error = sysctl_int(oldp, oldlenp, newp, newlen, &anonportmax);
		if (anonportmin >= anonportmax || anonportmax < 0
		    || anonportmax > 65535
#ifndef IPNOPRIVPORTS
		    || anonportmax < IPPORT_RESERVED
#endif
		    ) {
			anonportmax = old;
			return (EINVAL);
		}
		return (error);
	case IPCTL_MTUDISCTIMEOUT:
		old = ip_mtudisc_timeout;
		error = sysctl_int(oldp, oldlenp, newp, newlen,
		   &ip_mtudisc_timeout);
		if (ip_mtudisc_timeout < 0) {
			ip_mtudisc_timeout = old;
			return (EINVAL);
		}
		if (error == 0)
			rt_timer_queue_change(ip_mtudisc_timeout_q,
					      ip_mtudisc_timeout);
		return (error);
#ifdef GATEWAY
	case IPCTL_MAXFLOWS:
	    {
		int s;

		error = sysctl_int(oldp, oldlenp, newp, newlen,
		   &ip_maxflows);
		s = splsoftnet();
		ipflow_reap(0);
		splx(s);
		return (error);
	    }
#endif
	case IPCTL_HOSTZEROBROADCAST:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &hostzeroisbroadcast));
#if NGIF > 0
	case IPCTL_GIF_TTL:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip_gif_ttl));
#endif

#if NGRE > 0
	case IPCTL_GRE_TTL:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip_gre_ttl));
#endif

#ifndef IPNOPRIVPORTS
	case IPCTL_LOWPORTMIN:
		old = lowportmin;
		error = sysctl_int(oldp, oldlenp, newp, newlen, &lowportmin);
		if (lowportmin >= lowportmax
		    || lowportmin > IPPORT_RESERVEDMAX
		    || lowportmin < IPPORT_RESERVEDMIN
		    ) {
			lowportmin = old;
			return (EINVAL);
		}
		return (error);
	case IPCTL_LOWPORTMAX:
		old = lowportmax;
		error = sysctl_int(oldp, oldlenp, newp, newlen, &lowportmax);
		if (lowportmin >= lowportmax
		    || lowportmax > IPPORT_RESERVEDMAX
		    || lowportmax < IPPORT_RESERVEDMIN
		    ) {
			lowportmax = old;
			return (EINVAL);
		}
		return (error);
#endif

	case IPCTL_MAXFRAGPACKETS:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &ip_maxfragpackets));

	case IPCTL_CHECKINTERFACE:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &ip_checkinterface));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
